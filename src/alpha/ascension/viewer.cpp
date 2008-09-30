/**
 * @file viewer.cpp
 * @author exeal
 * @date 2003-2006 (was EditView.cpp and EditViewWindowMessages.cpp)
 * @date 2006-2008
 */

#include "viewer.hpp"
#include "rules.hpp"
#include "text-editor.hpp"
#include "../../manah/win32/ui/menu.hpp"
#include <limits>	// std.numeric_limit
#include <zmouse.h>
#include <msctf.h>
#include "../../manah/win32/ui/wait-cursor.hpp"
#ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY
#include "../../manah/com/dispatch-impl.hpp"
#endif // !ASCENSION_NO_ACTIVE_ACCESSIBILITY
#ifndef ASCENSION_NO_TEXT_SERVICES_FRAMEWORK
#include <Textstor.h>
#endif // !ASCENSION_NO_TEXT_SERVICES_FRAMEWORK

using namespace ascension;
using namespace ascension::viewers;
using namespace ascension::viewers::internal;
using namespace ascension::presentation;
using namespace ascension::layout;
using namespace ascension::kernel;
using namespace manah::win32;
using namespace manah::win32::gdi;
#ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY
using namespace manah::com;
#endif // !ASCENSION_NO_ACTIVE_ACCESSIBILITY
using namespace std;
using manah::toBoolean;
using manah::AutoBuffer;

#pragma comment(lib, "version.lib")

#ifdef _DEBUG
bool DIAGNOSE_INHERENT_DRAWING = false;	// 余計な描画を行っていないか診断するフラグ
//#define TRACE_DRAWING_STRING	// テキスト (代替グリフ以外) のレンダリングをトレース
#endif /* _DEBUG */


namespace {
	// すぐ下で使う
	BOOL CALLBACK enumResLangProc(HMODULE, const WCHAR*, const WCHAR* name, WORD langID, LONG_PTR param) {
		if(name == 0)
			return false;
		else if(langID != MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US))
			*reinterpret_cast<LANGID*>(param) = langID;
		return true;
	}
} // namespace @0

LANGID ASCENSION_FASTCALL ascension::getUserDefaultUILanguage() /*throw()*/ {
	// references (from Global Dev)
	// - Writing Win32 Multilingual User Interface Applications (http://www.microsoft.com/globaldev/handson/dev/muiapp.mspx)
	// - Ask Dr. International Column #9 (http://www.microsoft.com/globaldev/drintl/columns/009/default.mspx#EPD)
	static LANGID id = 0;
	if(id != 0)
		return id;
	id = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
	OSVERSIONINFOW version;
	version.dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);
	::GetVersionExW(&version);
	assert(version.dwPlatformId == VER_PLATFORM_WIN32_NT);

	// 2000/XP/Server 2003 以降の場合 -> kernel32.dll の GetUserDefaultUILanguage に転送
	if(version.dwMajorVersion >= 5) {
		if(HMODULE dll = ::LoadLibraryW(L"kernel32.dll")) {
			if(LANGID(WINAPI *function)(void) = reinterpret_cast<LANGID(WINAPI*)(void)>(::GetProcAddress(dll, "GetUserDefaultUILanguage")))
				id = (*function)();
			::FreeLibrary(dll);
		}
	}

	// NT 3.51-4.0 の場合 -> ntdll.dll のバージョン情報の言語
	else if(HMODULE dll = ::LoadLibraryW(L"ntdll.dll")) {
		::EnumResourceLanguagesW(dll, MAKEINTRESOURCEW(16)/*RT_VERSION*/,
			MAKEINTRESOURCEW(1), enumResLangProc, reinterpret_cast<LONG_PTR>(&id));
		::FreeLibrary(dll);
		if(id == MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US)) {	// special cases
			const UINT cp = ::GetACP();
			if(cp == 874)	// Thai
				id = MAKELANGID(LANG_THAI, SUBLANG_DEFAULT);
			else if(cp == 1255)	// Hebrew
				id = MAKELANGID(LANG_HEBREW, SUBLANG_DEFAULT);
			else if(cp == 1256)	// Arabic
				id = MAKELANGID(LANG_ARABIC, SUBLANG_ARABIC_SAUDI_ARABIA);
		}
	}

	return id;	// ちなみに Win 95/98 では HKCU\Control Panel\Desktop\ResourceLocale の値を使う
}


// LineStyle ////////////////////////////////////////////////////////////////

const LineStyle LineStyle::NULL_STYLE = {0, 0};


#ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY

// TextViewerAccessibleProxy ////////////////////////////////////////////////

/**
 * @c TextViewerAccessibleProxy is proxy object for @c IAccessible interface of @c TextViewer instance.
 * @see TextViewer#getAccessibleObject, ASCENSION_NO_ACTIVE_ACCESSIBILITY
 */
class viewers::internal::TextViewerAccessibleProxy :
		public IDocumentListener,
		public manah::com::ole::IDispatchImpl<
			IAccessible, manah::com::ole::RegTypeLibTypeInfoHolder<&LIBID_Accessibility, &IID_IAccessible> >,
		public IOleWindow {
	// IAccessible の実装については以下を参考にした:
	//   MSAA サーバーを実装する - 開発者のための実用的助言と、 Mozilla による MSAA サーバーの実装方法
	//   (http://www.geocities.jp/nobu586/archive/msaa-server.html)
	//   Mozilla アクセシビリティ・アーキテクチャー
	//   (http://www.mozilla-japan.org/access/architecture.html)
	//   アクセシビリティのツールキット　チェックリスト - 新ツールキット実装時の必要事項
	//   (http://www.mozilla-japan.org/access/toolkit-checklist.html)
	//   IAccessible Implementation Sample for a Custom Push Button
	//   (http://www.gotdotnet.com/workspaces/workspace.aspx?id=4b5530a0-c900-421b-8ed6-7407997fa979)
	MANAH_UNASSIGNABLE_TAG(TextViewerAccessibleProxy);
public:
	// constructor
	TextViewerAccessibleProxy(TextViewer& view);
	// method
	void dispose();
	// IUnknown
	MANAH_IMPLEMENT_UNKNOWN_MULTI_THREADED()
	MANAH_BEGIN_INTERFACE_TABLE()
		MANAH_IMPLEMENTS_LEFTMOST_INTERFACE(IAccessible)
		MANAH_IMPLEMENTS_INTERFACE(IDispatch)
		MANAH_IMPLEMENTS_INTERFACE(IOleWindow)
	MANAH_END_INTERFACE_TABLE()
	// IAccessible
	STDMETHODIMP get_accParent(::IDispatch** ppdispParent);
	STDMETHODIMP get_accChildCount(long* pcountChildren);
	STDMETHODIMP get_accChild(VARIANT varChild, ::IDispatch** ppdispChild);
	STDMETHODIMP get_accName(VARIANT varChild, BSTR* pszName);
	STDMETHODIMP get_accValue(VARIANT varChild, BSTR* pszValue);
	STDMETHODIMP get_accDescription(VARIANT varChild, BSTR* pszDescription);
	STDMETHODIMP get_accRole(VARIANT varChild, VARIANT* pvarRole);
	STDMETHODIMP get_accState(VARIANT varChild, VARIANT* pvarState);
	STDMETHODIMP get_accHelp(VARIANT varChild, BSTR* pszHelp);
	STDMETHODIMP get_accHelpTopic(BSTR* pszHelpFile, VARIANT varChild, long* pidTopic);
	STDMETHODIMP get_accKeyboardShortcut(VARIANT varChild, BSTR* pszKeyboardShortcut);
	STDMETHODIMP get_accFocus(VARIANT* pvarChild);
	STDMETHODIMP get_accSelection(VARIANT* pvarChildren);
	STDMETHODIMP get_accDefaultAction(VARIANT varChild, BSTR* pszDefaultAction);
	STDMETHODIMP accSelect(long flagsSelect, VARIANT varChild);
	STDMETHODIMP accLocation(long* pxLeft, long* pyTop, long* pcxWidth, long* pcyHeight, VARIANT varChild);
	STDMETHODIMP accNavigate(long navDir, VARIANT varStart, VARIANT* pvarEndUpAt);
	STDMETHODIMP accHitTest(long xLeft, long yTop, VARIANT* pvarChild);
	STDMETHODIMP accDoDefaultAction(VARIANT varChild);
	STDMETHODIMP put_accName(VARIANT varChild, BSTR szName);
	STDMETHODIMP put_accValue(VARIANT varChild, BSTR szValue);
	// IOleWindow
	STDMETHODIMP GetWindow(HWND* phwnd);
	STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode);
private:
	// IDocumentListener
	void documentAboutToBeChanged(const Document& document, const DocumentChange& change);
	void documentChanged(const Document& document, const DocumentChange& change);
private:
	TextViewer& view_;
	bool available_;
	ComQIPtr<::IAccessible> defaultServer_;
//	enum {CHILDID_SELECTION = 1};
};

namespace {
	class AccLib {
	public:
		AccLib() /*throw()*/ : oleaccDLL_(::LoadLibraryA("oleacc.dll")), user32DLL_(::LoadLibraryA("user32.dll")) {
			if(oleaccDLL_ == 0 || user32DLL_ == 0) {
				::FreeLibrary(oleaccDLL_);
				::FreeLibrary(user32DLL_);
				oleaccDLL_ = user32DLL_ = 0;
			} else {
				accessibleObjectFromWindowPtr_ =
					reinterpret_cast<LPFNACCESSIBLEOBJECTFROMWINDOW>(::GetProcAddress(oleaccDLL_, "AccessibleObjectFromWindow"));
				createStdAccessibleObjectPtr_ =
					reinterpret_cast<LPFNCREATESTDACCESSIBLEOBJECT>(::GetProcAddress(oleaccDLL_, "CreateStdAccessibleObject"));
				lresultFromObjectPtr_ = reinterpret_cast<LPFNLRESULTFROMOBJECT>(::GetProcAddress(oleaccDLL_, "LresultFromObject"));
				notifyWinEventPtr_ =
					reinterpret_cast<VOID(WINAPI *)(DWORD, HWND, LONG, LONG)>(::GetProcAddress(user32DLL_, "NotifyWinEvent"));
			}
		}
		~AccLib() {::FreeLibrary(oleaccDLL_); ::FreeLibrary(user32DLL_);}
		bool isAvailable() const /*throw()*/ {return oleaccDLL_ != 0;}
		HRESULT accessibleObjectFromWindow(HWND window, DWORD objectID, REFIID iid, void** object) {
			assert(isAvailable()); return (*accessibleObjectFromWindowPtr_)(window, objectID, iid, object);}
		void createStdAccessibleObject(HWND window, long objectID, REFIID iid, void** object) {
			assert(isAvailable()); (*createStdAccessibleObjectPtr_)(window, objectID, iid, object);}
		LRESULT lresultFromObject(REFIID iid, WPARAM wParam, LPUNKNOWN object) {
			assert(isAvailable()); return (*lresultFromObjectPtr_)(iid, wParam, object);}
		void notifyWinEvent(DWORD event, HWND window, long objectID, long childID) {
			assert(isAvailable()); return (*notifyWinEventPtr_)(event, window, objectID, childID);}
	private:
		HMODULE oleaccDLL_, user32DLL_;
		LPFNACCESSIBLEOBJECTFROMWINDOW accessibleObjectFromWindowPtr_;
		LPFNCREATESTDACCESSIBLEOBJECT createStdAccessibleObjectPtr_;
		LPFNLRESULTFROMOBJECT lresultFromObjectPtr_;
		VOID(WINAPI *notifyWinEventPtr_)(DWORD, HWND, LONG, LONG);
	} accLib;
} // namespace @0

#endif // !ASCENSION_NO_ACTIVE_ACCESSIBILITY

#ifndef ASCENSION_NO_TEXT_SERVICES_FRAMEWORK
namespace {
	class TextServiceApplicationAdapter : public ITextStoreACP, public ITextStoreAnchor {
	public:
		// constructor
		explicit TextServiceApplicationAdapter(Viewer& view);
		// IUnknown
		IMPLEMENT_UNKNOWN_NO_REF_COUNT()
		BEGIN_INTERFACE_TABLE()
			IMPLEMENTS_LEFTMOST_INTERFACE(ITextStoreACP)
			IMPLEMENTS_INTERFACE(ITextStoreAnchor)
		END_INTERFACE_TABLE()
		// ITextStoreACP
		STDMETHODIMP AdviseSink(REFIID riid, IUnknown* punk, DWORD dwMask);
		STDMETHODIMP UnadviseSink(IUnknown* punk);
		STDMETHODIMP RequestLock(DWORD dwLockFlags, HRESULT* phrSession);
		STDMETHODIMP GetStatus(TS_STATUS* pdcs);
		STDMETHODIMP QueryInsert(LONG acpTestStart, LONG acpTestEnd, ULONG cch, LONG* pacpResultStart, LONG* pacpResultEnd);
		STDMETHODIMP GetSelection(ULONG ulIndex, ULONG ulCount, TS_SELECTION_ACP* pSelection, ULONG* pcFetched);
		STDMETHODIMP SetSelection(ULONG ulCount, const TS_SELECTION_ACP* pSelection);
		STDMETHODIMP GetText(LONG acpStart, LONG acpEnd, WCHAR* pchPlain, ULONG cchPlainReq,
			ULONG* pcchPlainRet, TS_RUNINFO* prgRunInfo, ULONG cRunInfoReq, ULONG* pcRunInfoRet, LONG* pacpNext);
		STDMETHODIMP SetText(DWORD dwFlags, LONG acpStart, LONG acpEnd, const WCHAR* pchText, ULONG cch, TS_TEXTCHANGE* pChange);
		STDMETHODIMP GetFormattedText(LONG acpStart, LONG acpEnd, IDataObject** ppDataObject);
		STDMETHODIMP GetEmbedded(LONG acpPos, REFGUID rguidService, REFIID riid, IUnknown** ppunk);
		STDMETHODIMP QueryInsertEmbedded(const GUID* pguidService, const FORMATETC* pFormatEtc, BOOL* pfInsertable);
		STDMETHODIMP InsertEmbedded(DWORD dwFlags, LONG acpStart, LONG acpEnd, IDataObject* pDataObject, TS_TEXTCHANGE* pChange);
		STDMETHODIMP InsertTextAtSelection(DWORD dwFlags,
			const WCHAR* pchText, ULONG cch, LONG* pacpStart, LONG* pacpEnd, TS_TEXTCHANGE* pChange);
		STDMETHODIMP InsertEmbeddedAtSelection(DWORD dwFlags,
			IDataObject* pDataObject, LONG* pacpStart, LONG* pacpEnd, TS_TEXTCHANGE* pChange);
		STDMETHODIMP RequestSupportedAttrs(DWORD dwFlags, ULONG cFilterAttrs, const TS_ATTRID* paFilterAttrs);
		STDMETHODIMP RequestAttrsAtPosition(LONG acpPos, ULONG cFilterAttrs, const TS_ATTRID* paFilterAttrs, DWORD dwFlags);
		STDMETHODIMP RequestAttrsTransitioningAtPosition(LONG acpPos, ULONG cFilterAttrs, const TS_ATTRID* paFilterAttrs, DWORD dwFlags);
		STDMETHODIMP FindNextAttrTransition(LONG acpStart, LONG acpHalt, ULONG cFilterAttrs,
			const TS_ATTRID* paFilterAttrs, DWORD dwFlags, LONG* pacpNext, BOOL* pfFound, LONG* plFoundOffset);
		STDMETHODIMP RetrieveRequestedAttrs(ULONG ulCount, TS_ATTRVAL* paAttrVals, ULONG* pcFetched);
		STDMETHODIMP GetEndACP(LONG* pacp);
		STDMETHODIMP GetActiveView(TsViewCookie* pvcView);
		STDMETHODIMP GetACPFromPoint(TsViewCookie vcView, const POINT* ptScreen, DWORD dwFlags, LONG* pacp);
		STDMETHODIMP GetTextExt(TsViewCookie vcView, LONG acpStart, LONG acpEnd, RECT* prc, BOOL* pfClipped);
		STDMETHODIMP GetScreenExt(TsViewCookie vcView, RECT* prc);
		STDMETHODIMP GetWnd(TsViewCookie vcView, HWND* phwnd);
	};
} // namespace @0
#endif // !ASCENSION_NO_TEXT_SERVICES_FRAMEWORK


// TextViewer ///////////////////////////////////////////////////////////////

/**
 * @class ascension::viewers::TextViewer
 *
 * The view of Ascension framework. @c TextViewer displays the content of the document, manipulates
 * the document with the caret and selection, and provides other visual presentations.
 *
 * @c TextViewer provides two methods #freeze and #unfreeze to freeze of the screen of the window.
 * While the viewer is frozen, the window does not redraw the content.
 *
 * <h3>Duplication</h3>
 *
 * Unlike @c manah#win32#ui#Window class, the copy-constructor does not copy the window handle of
 * the original (attachement is not performed). This semantics is derived from
 * @c manah#win32#ui#CustomControl super class.
 *
 * So an object just created by copy-constructor does not have valid window handle. Call @c #create
 * method to construct as a window.
 *
 * <h3>Window styles related to bidirectional</h3>
 *
 * テキストを右寄せで表示するための @c WS_EX_RIGHT 、右から左に表示するための @c WS_EX_RTLREADING
 * はいずれも無視される。これらの設定には代わりに @c LayoutSettings の該当メンバを使用しなければならない
 *
 * @c WS_EX_LAYOUTRTL is also not supported. The result is undefined if you used.
 *
 * 垂直スクロールバーを左側に表示するにはクライアントが @c WS_EX_LEFTSCROLLBAR を設定しなければならない
 *
 * これらの設定を一括して変更する場合 @c #setTextDirection を使うことができる
 *
 * 垂直ルーラ (インジケータマージンと行番号) の位置はテキストが左寄せであれば左端、
 * 右寄せであれば右端になる
 *
 * <h3>Subclassing</h3>
 *
 * @c TextViewer and @c SourceViewer are intended to be subclassed. You can override the virtual
 * member functions in your derived class. Note that @c TextViewer implements interfaces defined
 * in Ascension by virtual functions. These are also overridable but you must call base
 * implementation. For example, you are overriding @c documentChanged:
 *
 * @code
 * void YourViewer::documentChanged(const Document& document, const DocumentChange& change) {
 *   // ...your own code
 *   TextViewer::documentChanged(document, change);
 * }
 * @endcode
 *
 * <h3>Windows specific features</h3>
 *
 * @c TextViewer supports OLE drag-and-drop. If you want to enable this feature, call Win32
 * @c OleInitialize in your thread.
 *
 * If you want to enable tooltips, call Win32 @c InitCommonControlsEx.
 *
 * @see presentation#Presentation, Caret
 */

#define RESTORE_HIDDEN_CURSOR()				\
	if(modeState_.cursorVanished) {			\
		modeState_.cursorVanished = false;	\
		::ShowCursor(true);					\
		releaseCapture();					\
	}

// local helpers
namespace {
	inline void abortIncrementalSearch(TextViewer& viewer) /*throw()*/ {
		if(texteditor::Session* session = viewer.document().session()) {
			if(session->incrementalSearcher().isRunning())
				session->incrementalSearcher().abort();
		}
	}
	inline void closeCompletionProposalsPopup(TextViewer& viewer) /*throw()*/ {
		if(contentassist::IContentAssistant* ca = viewer.contentAssistant()) {
			if(contentassist::IContentAssistant::ICompletionProposalsUI* cpui = ca->getCompletionProposalsUI())
				cpui->close();
		}
	}
	inline void endIncrementalSearch(TextViewer& viewer) /*throw()*/ {
		if(texteditor::Session* session = viewer.document().session()) {
			if(session->incrementalSearcher().isRunning())
				session->incrementalSearcher().end();
		}
	}
	inline const hyperlink::IHyperlink* getPointedHyperlink(const TextViewer& viewer, const Position& at) {
		size_t numberOfHyperlinks;
		if(const hyperlink::IHyperlink* const* hyperlinks = viewer.presentation().getHyperlinks(at.line, numberOfHyperlinks)) {
			for(size_t i = 0; i < numberOfHyperlinks; ++i) {
				if(at.column >= hyperlinks[i]->region().beginning() && at.column <= hyperlinks[i]->region().end())
					return hyperlinks[i];
			}
		}
		return 0;
	}
	inline void toggleOrientation(TextViewer& viewer) /*throw()*/ {
		TextViewer::VerticalRulerConfiguration vrc = viewer.verticalRulerConfiguration();
		if(viewer.configuration().orientation == LEFT_TO_RIGHT) {
			vrc.alignment = ALIGN_RIGHT;
			if(vrc.lineNumbers.alignment != ALIGN_AUTO)
				vrc.lineNumbers.alignment = ALIGN_LEFT;
			viewer.setConfiguration(0, &vrc);
			viewer.modifyStyleEx(WS_EX_LEFT | WS_EX_LTRREADING, WS_EX_RIGHT | WS_EX_RTLREADING | WS_EX_LEFTSCROLLBAR);
//			if(config.lineWrap.wrapsAtWindowEdge()) {
//				MANAH_AUTO_STRUCT_SIZE(SCROLLINFO, scroll);
//				viewer.getScrollInformation(SB_HORZ, scroll);
//				viewer.setScrollInformation(SB_HORZ, scroll);
//			}
		} else {
			vrc.alignment = ALIGN_LEFT;
			if(vrc.lineNumbers.alignment != ALIGN_AUTO)
				vrc.lineNumbers.alignment = ALIGN_RIGHT;
			viewer.setConfiguration(0, &vrc);
			viewer.modifyStyleEx(WS_EX_RIGHT | WS_EX_RTLREADING, WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR);
//			if(config.lineWrap.wrapsAtWindowEdge()) {
//				MANAH_AUTO_STRUCT_SIZE(SCROLLINFO, scroll);
//				viewer.getScrollInformation(SB_HORZ, scroll);
//				viewer.setScrollInformation(SB_HORZ, scroll);
//			}
		}
	}
	inline void getCurrentCharacterSize(const TextViewer& viewer, SIZE& result) {
		const Caret& caret = viewer.caret();
		if(caret.isEndOfLine())	// EOL
			result.cx = viewer.textRenderer().averageCharacterWidth();
		else {
			const LineLayout& layout = viewer.textRenderer().lineLayout(caret.lineNumber());
			const int leading = layout.location(caret.columnNumber(), LineLayout::LEADING).x;
			const int trailing = layout.location(caret.columnNumber(), LineLayout::TRAILING).x;
			result.cx = static_cast<int>(ascension::internal::distance(leading, trailing));
		}
		result.cy = viewer.textRenderer().lineHeight();
	}
} // namespace @0

MANAH_BEGIN_WINDOW_MESSAGE_MAP(TextViewer, BaseControl)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_CAPTURECHANGED)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_CHAR)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_COMMAND)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_CONTEXTMENU)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_DESTROY)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_ERASEBKGND)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_GETFONT)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_HSCROLL)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_IME_COMPOSITION)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_IME_ENDCOMPOSITION)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_IME_NOTIFY)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_IME_REQUEST)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_IME_STARTCOMPOSITION)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_KEYDOWN)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_KILLFOCUS)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_LBUTTONDBLCLK)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_LBUTTONDOWN)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_LBUTTONUP)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_MBUTTONDBLCLK)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_MBUTTONDOWN)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_MBUTTONUP)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_MOUSEMOVE)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_MOUSEWHEEL)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_NCCREATE)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_NOTIFY)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_RBUTTONDBLCLK)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_RBUTTONDOWN)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_RBUTTONUP)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_SETCURSOR)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_SETFOCUS)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_SIZE)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_STYLECHANGED)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_STYLECHANGING)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_SYSCHAR)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_SYSCOLORCHANGE)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_SYSKEYDOWN)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_SYSKEYUP)
#ifdef WM_THEMECHANGED
	MANAH_WINDOW_MESSAGE_ENTRY(WM_THEMECHANGED)
#endif /* WM_THEMECHANGED */
	MANAH_WINDOW_MESSAGE_ENTRY(WM_TIMER)
#ifdef WM_UNICHAR
	MANAH_WINDOW_MESSAGE_ENTRY(WM_UNICHAR)
#endif /* WM_UNICHAR */
	MANAH_WINDOW_MESSAGE_ENTRY(WM_VSCROLL)
#ifdef WM_XBUTTONDBLCLK
	MANAH_WINDOW_MESSAGE_ENTRY(WM_XBUTTONDBLCLK)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_XBUTTONDOWN)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_XBUTTONUP)
#endif /* WM_XBUTTONDBLCLK */
MANAH_END_WINDOW_MESSAGE_MAP()

/**
 * Constructor.
 * @param presentation the presentation
 */
TextViewer::TextViewer(Presentation& presentation) : presentation_(presentation), tipText_(0), autoScrollOriginMark_(0),
#ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY
		accessibleProxy_(0),
#endif // !ASCENSION_NO_ACTIVE_ACCESSIBILITY
		imeCompositionActivated_(false), imeComposingCharacter_(false), mouseInputDisabledCount_(0) {
	renderer_.reset(new Renderer(*this));
//	renderer_->addFontListener(*this);
//	renderer_->addVisualLinesListener(*this);
	caret_.reset(new Caret(*this));
	caret_->addListener(*this);
	caret_->addStateListener(*this);
	verticalRulerDrawer_.reset(new VerticalRulerDrawer(*this, true));

	static_cast<presentation::internal::ITextViewerCollection&>(presentation_).addTextViewer(*this);
	document().addListener(*this);
	document().addStateListener(*this);
	document().addRollbackListener(*this);

	// initializations of renderer_ and mouseInputStrategy_ are in initializeWindow()
}

/**
 * Copy-constructor. Unlike @c manah#win32#Handle class, this does not copy the window handle. For
 * more details, see the description of @c TextViewer.
 */
TextViewer::TextViewer(const TextViewer& rhs) : ui::CustomControl<TextViewer>(0), presentation_(rhs.presentation_), tipText_(0)
#ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY
		, accessibleProxy_(0)
#endif // !ASCENSION_NO_ACTIVE_ACCESSIBILITY
{
	renderer_.reset(new Renderer(*rhs.renderer_, *this));
//	renderer_->addFontListener(*this);
//	renderer_->addVisualLinesListener(*this);
	caret_.reset(new Caret(*this));
	caret_->addListener(*this);
	caret_->addStateListener(*this);
	verticalRulerDrawer_.reset(new VerticalRulerDrawer(*this, true));

	modeState_ = rhs.modeState_;

	imeCompositionActivated_ = imeComposingCharacter_ = false;
	mouseInputDisabledCount_ = 0;
	static_cast<presentation::internal::ITextViewerCollection&>(presentation_).addTextViewer(*this);
	document().addListener(*this);
	document().addStateListener(*this);
	document().addRollbackListener(*this);
}

/// Destructor.
TextViewer::~TextViewer() {
	static_cast<presentation::internal::ITextViewerCollection&>(presentation_).removeTextViewer(*this);
	document().removeListener(*this);
	document().removeStateListener(*this);
	document().removeRollbackListener(*this);
	renderer_->removeFontListener(*this);
	renderer_->removeVisualLinesListener(*this);
	caret_->removeListener(*this);
	caret_->removeStateListener(*this);
	for(set<VisualPoint*>::iterator it = points_.begin(); it != points_.end(); ++it)
		(*it)->viewerDisposed();

	// 非共有データ
//	delete selection_;
	delete[] tipText_;
#ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY
	if(accessibleProxy_ != 0)
		accessibleProxy_->Release();
#endif // !ASCENSION_NO_ACTIVE_ACCESSIBILITY
}

#ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY
/// Returns the accessible proxy of the viewer.
HRESULT TextViewer::accessibleObject(IAccessible*& acc) const /*throw()*/ {
	TextViewer& self = *const_cast<TextViewer*>(this);
	acc = 0;
	if(accessibleProxy_ == 0 && isWindow() && accLib.isAvailable()) {
		if(self.accessibleProxy_ = new TextViewerAccessibleProxy(self)) {
			self.accessibleProxy_->AddRef();
//			accLib.notifyWinEvent(EVENT_OBJECT_CREATE, *this, OBJID_CLIENT, CHILDID_SELF);
		} else
			return E_OUTOFMEMORY;
	}
	if(accessibleProxy_ == 0)
		return E_FAIL;
	(acc = self.accessibleProxy_)->AddRef();
	return S_OK;
}
#endif // !ASCENSION_NO_ACTIVE_ACCESSIBILITY

/// Starts the auto scroll.
void TextViewer::beginAutoScroll() {
	assertValidAsWindow();
	if(!hasFocus() || document().numberOfLines() <= numberOfVisibleLines())
		return;

	RECT rect;
	POINT pt;
	autoScrollOriginMark_->getRect(rect);
	::GetCursorPos(&pt);
	autoScroll_.indicatorPosition = pt;
	screenToClient(autoScroll_.indicatorPosition);
	autoScrollOriginMark_->setPosition(HWND_TOP,
		pt.x - (rect.right - rect.left) / 2, pt.y - (rect.bottom - rect.top) / 2,
		0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_SHOWWINDOW);
	autoScroll_.scrolling = true;
	setCapture();
	setTimer(TIMERID_AUTOSCROLL, 0, 0);
}

/// @see ICaretListener#caretMoved
void TextViewer::caretMoved(const Caret& self, const Region& oldRegion) {
	if(!isVisible())
		return;
	const Region newRegion = self.selectionRegion();
	bool changed = false;

	// adjust the caret
	if(!isFrozen() && (hasFocus() /*|| completionWindow_->hasFocus()*/))
		updateCaretPosition();

	// redraw the selected region
	if(self.isSelectionRectangle()) {	// rectangle
		if(!oldRegion.isEmpty())
			redrawLines(oldRegion.beginning().line, oldRegion.end().line);
		if(!newRegion.isEmpty())
			redrawLines(newRegion.beginning().line, newRegion.end().line);
	} else if(newRegion != oldRegion) {	// the selection actually changed
		if(oldRegion.isEmpty()) {	// the selection was empty...
			if(!newRegion.isEmpty())	// the selection is not empty now
				redrawLines(newRegion.beginning().line, newRegion.end().line);
		} else {	// ...if the there is selection
			if(newRegion.isEmpty()) {	// the selection became empty
				redrawLines(oldRegion.beginning().line, oldRegion.end().line);
				if(!isFrozen())
					update();
			} else if(oldRegion.beginning() == newRegion.beginning()) {	// the beginning point didn't change
				const length_t i[2] = {oldRegion.end().line, newRegion.end().line};
				redrawLines(min(i[0], i[1]), max(i[0], i[1]));
			} else if(oldRegion.end() == newRegion.end()) {	// the end point didn't change
				const length_t i[2] = {oldRegion.beginning().line, newRegion.beginning().line};
				redrawLines(min(i[0], i[1]), max(i[0], i[1]));
			} else {	// the both points changed
				if((oldRegion.beginning().line >= newRegion.beginning().line && oldRegion.beginning().line <= newRegion.end().line)
						|| (oldRegion.end().line >= newRegion.beginning().line && oldRegion.end().line <= newRegion.end().line)) {
					const length_t i[2] = {
						min(oldRegion.beginning().line, newRegion.beginning().line), max(oldRegion.end().line, newRegion.end().line)
					};
					redrawLines(min(i[0], i[1]), max(i[0], i[1]));
				} else {
					redrawLines(oldRegion.beginning().line, oldRegion.end().line);
					if(!isFrozen())
						update();
					redrawLines(newRegion.beginning().line, newRegion.end().line);
				}
			}
		}
		changed = true;
	}

	if(changed && !isFrozen())
		update();
}

/**
 * Returns the document position nearest from the specified point.
 * @param pt the coordinates of the point. can be outside of the window
 * @param edge if set @c LineLayout#LEADING, the result is the leading of the character at @a pt.
 * otherwise the result is the position nearest @a pt
 * @param abortNoCharacter if set to true, this method returns @c Position#INVALID_POSITION
 * immediately when @a pt hovered outside of the text layout (e.g. far left or right of the line,
 * beyond the last line, ...).
 * @param snapPolicy which character boundary the returned position snapped to. if
 * EditPoint#DEFAULT_UNIT is set, obtains by Caret#characterUnit()
 * @return returns the document position
 * @throw UnknownValueException @a edge and/or snapPolicy are invalid
 * @see #clientXYForCharacter, #hitTest, layout#LineLayout#offset
 */
Position TextViewer::characterForClientXY(const POINT& pt, LineLayout::Edge edge,
		bool abortNoCharacter /* = false */, EditPoint::CharacterUnit snapPolicy /* = EditPoint::DEFAULT_UNIT */) const {
	assertValidAsWindow();
	if(snapPolicy == EditPoint::DEFAULT_UNIT)
		snapPolicy = caret().characterUnit();
	Position result;

	// determine the logical line
	length_t subline;
	bool outside;
	mapClientYToLine(pt.y, &result.line, &subline, &outside);
	if(abortNoCharacter && outside)
		return Position::INVALID_POSITION;
	const LineLayout& layout = renderer_->lineLayout(result.line);

	// determine the column
	const long x = pt.x - getDisplayXOffset(result.line);
	if(edge == LineLayout::LEADING)
		result.column = layout.offset(x, static_cast<int>(renderer_->linePitch() * subline), LineLayout::LEADING, &outside);
	else if(edge == LineLayout::TRAILING) {
		length_t trailing;
		result.column = layout.offset(x, static_cast<int>(renderer_->linePitch() * subline), trailing, &outside);
		result.column += trailing;
	} else
		throw UnknownValueException("edge");
	if(abortNoCharacter && outside)
		return Position::INVALID_POSITION;

	// snap intervening position to the boundary
	if(result.column != 0 && snapPolicy != EditPoint::UTF16_CODE_UNIT) {
		using namespace text;
		const String& s = document().line(result.line);
		const bool interveningSurrogates =
			surrogates::isLowSurrogate(s[result.column]) && surrogates::isHighSurrogate(s[result.column - 1]);
		if(snapPolicy == EditPoint::UTF32_CODE_UNIT) {
			if(interveningSurrogates) {
				if(edge == LineLayout::LEADING)
					--result.column;
				else if(ascension::internal::distance(x, layout.location(result.column - 1).x)
						<= ascension::internal::distance(x, layout.location(result.column + 1).x))
					--result.column;
				else
					++result.column;
			}
		} else if(snapPolicy == EditPoint::GRAPHEME_CLUSTER) {
			GraphemeBreakIterator<DocumentCharacterIterator> i(
				DocumentCharacterIterator(document(), Region(result.line, make_pair(0, s.length())), result));
			if(interveningSurrogates || !i.isBoundary(i.base())) {
				--i;
				if(edge == LineLayout::LEADING)
					result.column = i.base().tell().column;
				else {
					const Position backward(i.base().tell());
					const Position forward((++i).base().tell());
					result.column = ((ascension::internal::distance(x, layout.location(backward.column).x)
						<= ascension::internal::distance(x, layout.location(forward.column).x)) ? backward : forward).column;
				}
			}
		} else
			throw UnknownValueException("snapPolicy");
	}
	return result;
}

/**
 * Returns the point nearest from the specified document position.
 * @param position the document position. can be outside of the window
 * @param fullSearchY if this is false, this method stops at top or bottom of the client area.
 * otherwise, the calculation of y-coordinate is performed completely. but in this case, may be
 * very slow. see the description of return value
 * @param edge the edge of the character
 * @return the client coordinates of the point. about the y-coordinate of the point, if
 * @a fullSearchY is false and @a position.line is outside of the client area, the result is 32767
 * (for upward) or -32768 (for downward)
 * @throw BadPositionException @a position is outside of the document
 * @see #characterForClientXY, #hitTest, layout#LineLayout#location
 */
POINT TextViewer::clientXYForCharacter(const Position& position, bool fullSearchY, LineLayout::Edge edge) const {
	assertValidAsWindow();
	const LineLayout& layout = renderer_->lineLayout(position.line);
	POINT pt = layout.location(position.column, edge);
	pt.x += getDisplayXOffset(position.line);
	const int y = mapLineToClientY(position.line, fullSearchY);
	if(y == 32767 || y == -32768)
		pt.y = y;
	else
		pt.y += y;
	return pt;
}

/**
 * Creates the window of the viewer.
 * @param parent handle to the parent or owner window of the window
 * @param rect the position and size of the window
 * @param style the style of the window
 * @param exStyle the extended style of the window
 * @return true if succeeded
 * @see manah#windows#controls#Window#create
 */
bool TextViewer::create(HWND parent, const RECT& rect, DWORD style, DWORD exStyle) {
	if(isWindow())
		return false;

	const bool visible = toBoolean(style & WS_VISIBLE);
	style &= ~WS_VISIBLE;	// 後で足す
	if(!ui::CustomControl<TextViewer>::create(parent, rect, 0, style, exStyle))
		return false;

	scrollInfo_.updateVertical(*this);
	updateScrollBars();

	// create the tooltip belongs to the window
	toolTip_ = ::CreateWindowExW(
		WS_EX_TOOLWINDOW | WS_EX_TOPMOST, TOOLTIPS_CLASSW, 0, WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, getHandle(), 0,
		reinterpret_cast<HINSTANCE>(static_cast<HANDLE_PTR>(::GetWindowLongPtr(getHandle(), GWLP_HINSTANCE))), 0);
	if(toolTip_ != 0) {
		MANAH_AUTO_STRUCT_SIZE(TOOLINFOW, ti);
		RECT margins = {1, 1, 1, 1};
		ti.hwnd = getHandle();
		ti.lpszText = LPSTR_TEXTCALLBACKW;
		ti.uFlags = TTF_SUBCLASS;
		ti.uId = 1;
		::SetRect(&ti.rect, 0, 0, 0, 0);
		::SendMessageW(toolTip_, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&ti));
		::SendMessageW(toolTip_, TTM_SETDELAYTIME, TTDT_AUTOPOP, 30000);	// 30 秒間 (根拠なし) 表示されるように
//		::SendMessageW(toolTip_, TTM_SETDELAYTIME, TTDT_INITIAL, 1500);
		::SendMessageW(toolTip_, TTM_SETMARGIN, 0, reinterpret_cast<LPARAM>(&margins));
		::SendMessageW(toolTip_, TTM_ACTIVATE, true, 0L);
	}

	// create the window for the auto scroll origin mark
	autoScrollOriginMark_.reset(new AutoScrollOriginMark);
	autoScrollOriginMark_->create(*this);

	setMouseInputStrategy(0, true);

	VerticalRulerConfiguration vrc;
	vrc.lineNumbers.visible = true;
	vrc.indicatorMargin.visible = true;
	vrc.lineNumbers.textColor = Colors(Color(0x00, 0x80, 0x80), Color(0xFF, 0xFF, 0xFF));
	vrc.lineNumbers.borderColor = Color(0x00, 0x80, 0x80);
	vrc.lineNumbers.borderStyle = VerticalRulerConfiguration::LineNumbers::DOTTED;
	vrc.lineNumbers.borderWidth = 1;
	setConfiguration(0, &vrc);

#if 1
	// this is JavaScript partitioning and lexing settings for test
	using namespace contentassist;
	using namespace rules;
	using namespace text;

	static const ContentType JS_MULTILINE_DOC_COMMENT = 40,
		JS_MULTILINE_COMMENT = 42, JS_SINGLELINE_COMMENT = 43, JS_DQ_STRING = 44, JS_SQ_STRING = 45;

	class JSContentTypeInformation : public IContentTypeInformationProvider {
	public:
		JSContentTypeInformation()  {
			jsIDs_.overrideIdentifierStartCharacters(L"_", L""); jsdocIDs_.overrideIdentifierStartCharacters(L"$@", L"");}
		const IdentifierSyntax& getIdentifierSyntax(ContentType contentType) const {
			return (contentType != JS_MULTILINE_DOC_COMMENT) ? jsIDs_ : jsdocIDs_;}
	private:
		IdentifierSyntax jsIDs_, jsdocIDs_;
	};
	JSContentTypeInformation* cti = new JSContentTypeInformation;

	TransitionRule* rules[12];
	rules[0] = new LiteralTransitionRule(DEFAULT_CONTENT_TYPE, JS_MULTILINE_DOC_COMMENT, L"/**");
	rules[1] = new LiteralTransitionRule(JS_MULTILINE_DOC_COMMENT, DEFAULT_CONTENT_TYPE, L"*/");
	rules[2] = new LiteralTransitionRule(DEFAULT_CONTENT_TYPE, JS_MULTILINE_COMMENT, L"/*");
	rules[3] = new LiteralTransitionRule(JS_MULTILINE_COMMENT, DEFAULT_CONTENT_TYPE, L"*/");
	rules[4] = new LiteralTransitionRule(DEFAULT_CONTENT_TYPE, JS_SINGLELINE_COMMENT, L"//");
	rules[5] = new LiteralTransitionRule(JS_SINGLELINE_COMMENT, DEFAULT_CONTENT_TYPE, L"", L'\\');
	rules[6] = new LiteralTransitionRule(DEFAULT_CONTENT_TYPE, JS_DQ_STRING, L"\"");
	rules[7] = new LiteralTransitionRule(JS_DQ_STRING, DEFAULT_CONTENT_TYPE, L"\"", L'\\');
	rules[8] = new LiteralTransitionRule(JS_DQ_STRING, DEFAULT_CONTENT_TYPE, L"");
	rules[9] = new LiteralTransitionRule(DEFAULT_CONTENT_TYPE, JS_SQ_STRING, L"\'");
	rules[10] = new LiteralTransitionRule(JS_SQ_STRING, DEFAULT_CONTENT_TYPE, L"\'", L'\\');
	rules[11] = new LiteralTransitionRule(JS_SQ_STRING, DEFAULT_CONTENT_TYPE, L"");
	auto_ptr<LexicalPartitioner> p(new LexicalPartitioner());
	p->setRules(rules, MANAH_ENDOF(rules));
	document().setPartitioner(p);

	PresentationReconstructor* pr = new PresentationReconstructor(presentation());

	// JSDoc syntax highlight test
	static const Char JSDOC_ATTRIBUTES[] = L"@addon @argument @author @base @class @constructor @deprecated @exception @exec @extends"
		L" @fileoverview @final @ignore @link @member @param @private @requires @return @returns @see @throws @type @version";
	auto_ptr<WordRule> jsdocAttributes(new WordRule(220, JSDOC_ATTRIBUTES, MANAH_ENDOF(JSDOC_ATTRIBUTES) - 1, L' ', true));
	auto_ptr<LexicalTokenScanner> scanner(new LexicalTokenScanner(JS_MULTILINE_DOC_COMMENT));
	scanner->addWordRule(jsdocAttributes);
	scanner->addRule(auto_ptr<Rule>(new URIRule(219, URIDetector::defaultIANAURIInstance(), false)));
	map<Token::ID, const TextStyle> jsdocStyles;
	jsdocStyles.insert(make_pair(Token::DEFAULT_TOKEN, TextStyle(Colors(Color(0x00, 0x80, 0x00)))));
	jsdocStyles.insert(make_pair(219, TextStyle(Colors(Color(0x00, 0x80, 0x00)), false, false, false, SOLID_UNDERLINE)));
	jsdocStyles.insert(make_pair(220, TextStyle(Colors(Color(0x00, 0x80, 0x00)), true)));
	auto_ptr<LexicalPartitionPresentationReconstructor> ppr(
		new LexicalPartitionPresentationReconstructor(document(), scanner, jsdocStyles));
	pr->setPartitionReconstructor(JS_MULTILINE_DOC_COMMENT, ppr);

	// JavaScript syntax highlight test
	static const Char JS_KEYWORDS[] = L"Infinity break case catch continue default delete do else false finally for function"
		L" if in instanceof new null return switch this throw true try typeof undefined var void while with";
	static const Char JS_FUTURE_KEYWORDS[] = L"abstract boolean byte char class double enum extends final float goto"
		L" implements int interface long native package private protected public short static super synchronized throws transient volatile";
	auto_ptr<WordRule> jsKeywords(new WordRule(221, JS_KEYWORDS, MANAH_ENDOF(JS_KEYWORDS) - 1, L' ', true));
	auto_ptr<WordRule> jsFutureKeywords(new WordRule(222, JS_FUTURE_KEYWORDS, MANAH_ENDOF(JS_FUTURE_KEYWORDS) - 1, L' ', true));
	scanner.reset(new LexicalTokenScanner(DEFAULT_CONTENT_TYPE));
	scanner->addWordRule(jsKeywords);
	scanner->addWordRule(jsFutureKeywords);
	scanner->addRule(auto_ptr<const Rule>(new NumberRule(223)));
	map<Token::ID, const TextStyle> jsStyles;
	jsStyles.insert(make_pair(Token::DEFAULT_TOKEN, TextStyle()));
	jsStyles.insert(make_pair(221, TextStyle(Colors(Color(0x00, 0x00, 0xFF)))));
	jsStyles.insert(make_pair(222, TextStyle(Colors(Color(0x00, 0x00, 0xFF)), false, false, false, DASHED_UNDERLINE)));
	jsStyles.insert(make_pair(223, TextStyle(Colors(Color(0x80, 0x00, 0x00)))));
	pr->setPartitionReconstructor(DEFAULT_CONTENT_TYPE,
		auto_ptr<IPartitionPresentationReconstructor>(new LexicalPartitionPresentationReconstructor(document(), scanner, jsStyles)));

	// other contents
	pr->setPartitionReconstructor(JS_MULTILINE_COMMENT, auto_ptr<IPartitionPresentationReconstructor>(
		new SingleStyledPartitionPresentationReconstructor(TextStyle(Colors(Color(0x00, 0x80, 0x00))))));
	pr->setPartitionReconstructor(JS_SINGLELINE_COMMENT, auto_ptr<IPartitionPresentationReconstructor>(
		new SingleStyledPartitionPresentationReconstructor(TextStyle(Colors(Color(0x00, 0x80, 0x00))))));
	pr->setPartitionReconstructor(JS_DQ_STRING, auto_ptr<IPartitionPresentationReconstructor>(
		new SingleStyledPartitionPresentationReconstructor(TextStyle(Colors(Color(0x00, 0x00, 0x80))))));
	pr->setPartitionReconstructor(JS_SQ_STRING, auto_ptr<IPartitionPresentationReconstructor>(
		new SingleStyledPartitionPresentationReconstructor(TextStyle(Colors(Color(0x00, 0x00, 0x80))))));
	new CurrentLineHighlighter(*caret_);

	// URL hyperlinks test
	auto_ptr<hyperlink::CompositeHyperlinkDetector> hld(new hyperlink::CompositeHyperlinkDetector);
	hld->setDetector(JS_MULTILINE_DOC_COMMENT, auto_ptr<hyperlink::IHyperlinkDetector>(
		new hyperlink::URIHyperlinkDetector(URIDetector::defaultIANAURIInstance(), false)));
	presentation().setHyperlinkDetector(hld.release(), true);

	// content assist test
	class JSDocProposals : public IdentifiersProposalProcessor {
	public:
		JSDocProposals(const IdentifierSyntax& ids) : IdentifiersProposalProcessor(JS_MULTILINE_DOC_COMMENT, ids) {}
		void computeCompletionProposals(const Caret& caret, bool& incremental,
				Region& replacementRegion, set<ICompletionProposal*>& proposals) const {
			basic_istringstream<Char> s(JSDOC_ATTRIBUTES);
			String p;
			while(s >> p)
				proposals.insert(new CompletionProposal(p));
			IdentifiersProposalProcessor::computeCompletionProposals(caret, incremental = true, replacementRegion, proposals);
		}
		bool isCompletionProposalAutoActivationCharacter(CodePoint c) const /*throw()*/ {return c == L'@';}
	};
	class JSProposals : public IdentifiersProposalProcessor {
	public:
		JSProposals(const IdentifierSyntax& ids) : IdentifiersProposalProcessor(DEFAULT_CONTENT_TYPE, ids) {}
		void computeCompletionProposals(const Caret& caret, bool& incremental,
				Region& replacementRegion, set<ICompletionProposal*>& proposals) const {
			basic_istringstream<Char> s(JS_KEYWORDS);
			String p;
			while(s >> p)
				proposals.insert(new CompletionProposal(p));
			IdentifiersProposalProcessor::computeCompletionProposals(caret, incremental = true, replacementRegion, proposals);
		}
		bool isCompletionProposalAutoActivationCharacter(CodePoint c) const /*throw()*/ {return c == L'.';}
	};
	auto_ptr<contentassist::ContentAssistant> ca(new contentassist::ContentAssistant());
	ca->setContentAssistProcessor(JS_MULTILINE_DOC_COMMENT, auto_ptr<contentassist::IContentAssistProcessor>(new JSDocProposals(cti->getIdentifierSyntax(JS_MULTILINE_DOC_COMMENT))));
	ca->setContentAssistProcessor(DEFAULT_CONTENT_TYPE, auto_ptr<contentassist::IContentAssistProcessor>(new JSProposals(cti->getIdentifierSyntax(DEFAULT_CONTENT_TYPE))));
	setContentAssistant(ca);
	document().setContentTypeInformation(auto_ptr<IContentTypeInformationProvider>(cti));
#endif /* _DEBUG */
	
	renderer_->addFontListener(*this);
	renderer_->addVisualLinesListener(*this);

	// placement and display
	move(rect, false);
	if(visible)
		show(SW_SHOW);

	return true;
}

/// Implementation of @c #beep method. The subclasses can override to customize the behavior.
void TextViewer::doBeep() /*throw()*/ {
	::MessageBeep(MB_OK);
}

/// @see kernel#IDocumentStateListener#documentAccessibleRegionChanged
void TextViewer::documentAccessibleRegionChanged(const Document&) {
	if(document().isNarrowed())
		scrollTo(-1, -1, false);
	invalidateRect(0, false);
}

/// @see kernel#IDocumentListener#documentAboutToBeChanged
void TextViewer::documentAboutToBeChanged(const Document&, const DocumentChange&) {
	// do nothing
}

/// @see kernel#IDocumentListener#documentChanged
void TextViewer::documentChanged(const Document&, const DocumentChange& change) {
	// cancel the active incremental search
	if(texteditor::Session* session = document().session()) {	// TODO: why is the code here?
		if(session->incrementalSearcher().isRunning())
			session->incrementalSearcher().abort();
	}

	const Region& region = change.region();
	const bool multiLine = region.beginning().line != region.end().line;
	if(isFrozen() && multiLine && freezeInfo_.invalidLines.first != INVALID_INDEX) {
		// slide the frozen lines to be drawn
		const length_t first = region.beginning().line + 1, last = region.end().line;
		if(change.isDeletion()) {
			if(freezeInfo_.invalidLines.first > last)
				freezeInfo_.invalidLines.first -= last - first + 1;
			else if(freezeInfo_.invalidLines.first > first)
				freezeInfo_.invalidLines.first = first;
			if(freezeInfo_.invalidLines.second != numeric_limits<length_t>::max()) {
				if(freezeInfo_.invalidLines.second > last)
					freezeInfo_.invalidLines.second -= last - first + 1;
				else if(freezeInfo_.invalidLines.second > first)
					freezeInfo_.invalidLines.second = first;
			}
		} else {
			if(freezeInfo_.invalidLines.first >= first)
				freezeInfo_.invalidLines.first += last - first + 1;
			if(freezeInfo_.invalidLines.second >= first && freezeInfo_.invalidLines.second != numeric_limits<length_t>::max())
				freezeInfo_.invalidLines.second += last - first + 1;
		}
	}
//	invalidateLines(region.beginning().line, !multiLine ? region.end().line : INVALID_INDEX);
	if(!isFrozen())
		verticalRulerDrawer_->update();
	if(scrollInfo_.changed)
		updateScrollBars();
}

/// @see kernel#IDocumentStateListener#documentModificationSignChanged
void TextViewer::documentModificationSignChanged(const Document&) {
	// do nothing
}

/// @see ascension#text#IDocumentStateListenerdocumentPropertyChanged
void TextViewer::documentPropertyChanged(const Document&, const DocumentPropertyKey&) {
	// do nothing
}

/// @see kernel#IDocumentStateListener#documentReadOnlySignChanged
void TextViewer::documentReadOnlySignChanged(const Document&) {
	// do nothing
}

/// @see kernel#IDocumentRollbackListener#documentUndoSequenceStarted
void TextViewer::documentUndoSequenceStarted(const Document&) {
	freeze(false);
}

/// @see kernel#IDocumentRollbackListener#documentUndoSequenceStopped
void TextViewer::documentUndoSequenceStopped(const Document&, const Position& resultPosition) {
	unfreeze(false);
	if(resultPosition != Position::INVALID_POSITION && hasFocus()) {
		closeCompletionProposalsPopup(*this);
		caret_->moveTo(resultPosition);
	}
}

/**
 * Additionally draws the indicator margin on the vertical ruler.
 * @param line the line number
 * @param dc the device context
 * @param rect the rectangle to draw
 */
void TextViewer::drawIndicatorMargin(length_t /* line */, DC& /* dc */, const RECT& /* rect */) {
}

/**
 * Ends the auto scroll.
 * @return true if the auto scroll was active
 */
bool TextViewer::endAutoScroll() {
	assertValidAsWindow();
	if(autoScroll_.scrolling) {
		killTimer(TIMERID_AUTOSCROLL);
		autoScroll_.scrolling = false;
		autoScrollOriginMark_->show(SW_HIDE);
		releaseCapture();
		return true;
	}
	return false;
}

/// @see IFontSelectorListener#fontChanged
void TextViewer::fontChanged() /*throw()*/ {
	verticalRulerDrawer_->update();
	scrollInfo_.resetBars(*this, SB_BOTH, true);
	updateScrollBars();
	recreateCaret();
	redrawLine(0, true);
}

/**
 * Freezes the drawing of the viewer.
 * @param forAllClones set true to freeze also all clones of the viewer
 * @see #isFrozen, #unfreeze
 */
void TextViewer::freeze(bool forAllClones /* = true */) {
	assertValidAsWindow();
	if(!forAllClones)
		++freezeInfo_.count;
	else {
		for(Presentation::TextViewerIterator i(presentation_.firstTextViewer()), e(presentation_.lastTextViewer()); i != e; ++i)
			++(*i)->freezeInfo_.count;
	}
}

/**
 * Returns the horizontal display offset from @c LineLayout coordinates to client coordinates.
 * @param line the line number
 * @return the offset
 */
int TextViewer::getDisplayXOffset(length_t line) const {
	const RECT margins = textAreaMargins();
	if(configuration_.alignment == ALIGN_LEFT || configuration_.justifiesLines)
		return margins.left - scrollInfo_.x() * renderer_->averageCharacterWidth();

	int indent;
	Rect clientRect;
	getClientRect(clientRect);
	if(renderer_->longestLineWidth() + margins.left + margins.right > clientRect.getWidth()) {
		indent = renderer_->longestLineWidth() - renderer_->lineLayout(line).sublineWidth(0) + margins.left;
		indent += (clientRect.getWidth() - margins.left - margins.right) % renderer_->averageCharacterWidth();
	} else
		indent = clientRect.getWidth() - renderer_->lineLayout(line).sublineWidth(0) - margins.right;
	if(configuration_.alignment == ALIGN_CENTER)
		indent /= 2;
	else
		assert(configuration_.alignment == ALIGN_RIGHT);
	return indent - static_cast<long>(scrollInfo_.x()) * renderer_->averageCharacterWidth();
}

#if 0
/**
 * Returns the text and the region of a link near the cursor.
 * @param[out] region the region of the link
 * @param[out] text the text of the link. if the link is mail address, "mailto:" will be added to the head
 * @return true if the cursor is on link
 * @deprecated 0.8
 */
bool TextViewer::getPointedLinkText(Region& region, AutoBuffer<Char>& text) const {
	assertValidAsWindow();
	const Document& document = document();
	const Position pos = getCharacterForClientXY(getCursorPosition(), false);	// カーソル位置に最も近い文字位置

	if(pos.column == document.getLineLength(pos.line))	// 指定位置に文字が無い
		return false;

	const LineLayout& layout = renderer_->getLineLayout(pos.line);
	const length_t subline = layout.getSubline(pos.column);
	const Char* const line = document.getLine(pos.line).data();
	const Char* const first = line + layout.getSublineOffset(subline);
	const Char* const last =
		line + ((subline < layout.getNumberOfSublines() - 1) ? layout.getSublineOffset(subline + 1) : document.getLineLength(pos.line));
	length_t linkLength;	// URIDetector の eatMailAddress 、eatUrlString で見つけたリンクテキストの長さ

	for(const Char* p = (pos.column > 200) ? first + pos.column - 200 : first; p <= first + pos.column; ) {
		if(p != first) {
			if((p[-1] >= L'A' && p[-1] <= L'Z')
					|| (p[-1] >= L'a' && p[-1] <= L'z')
					|| p[-1] == L'_') {
				++p;
				continue;
			}
		}
		if(0 != (linkLength = rules::URIDetector::eatURL(p, last, true) - p)) {
			if(p - first + linkLength > pos.column) {	// カーソル位置を越えた
				region.first.line = region.second.line = pos.line;
				region.first.column = p - line;
				region.second.column = region.first.column + linkLength;
				text.reset(new Char[linkLength + 1]);
				wmemcpy(text.get(), p, linkLength);
				text[linkLength] = 0;
				return true;
			}
			p += linkLength;	// 届かない場合は続行
		} else if(0 != (linkLength = rules::URIDetector::eatMailAddress(p, last, true) - p)) {
			if(p - first + linkLength > pos.column) {	// カーソル位置を越えた
				static const wchar_t MAILTO_PREFIX[] = L"mailto:";
				region.first.line = region.second.line = pos.line;
				region.first.column = p - line;
				region.second.column = region.first.column + linkLength;
				text.reset(new Char[linkLength + 7 + 1]);
				wmemcpy(text.get(), MAILTO_PREFIX, countof(MAILTO_PREFIX) - 1);
				wmemcpy(text.get() + countof(MAILTO_PREFIX) - 1, p, linkLength);
				text[countof(MAILTO_PREFIX) - 1 + linkLength] = 0;
				return true;
			}
			p += linkLength;	// 届かない場合は続行
		} else
			++p;
	}
	return false;
}
#endif

/// Handles @c WM_CHAR and @c WM_UNICHAR window messages.
void TextViewer::handleGUICharacterInput(CodePoint c) {
	// vanish the cursor when the GUI user began typing
	if(texteditor::commands::CharacterInputCommand(*this, c).execute() != 0
			&& !modeState_.cursorVanished
			&& configuration_.vanishesCursor
			&& hasFocus()) {
		// ignore if the cursor is not over a window belongs to the same thread
		POINT pt;
		::GetCursorPos(&pt);
		Window pointedWindow(Window::fromPoint(pt));
		if(pointedWindow.getHandle() != 0 && pointedWindow.getThreadID() == getThreadID()) {
			modeState_.cursorVanished = true;
			::ShowCursor(false);
			setCapture();
		}
	}
}

/**
 * Translates key down message to a command.
 *
 * This method provides a default implementtation of "key combination to command" map.
 * Default @c #onKeyDown calls this method.
 *
 * This method is not overiddable (not virtual).
 * To customize key bindings, the derevied class must override @c #onKeyDown method instead.
 * @param key the virtual-keycode of the key
 * @param controlPressed true if CTRL key is pressed
 * @param shiftPressed true if SHIFT key is pressed
 * @param altPressed true if ALT key is pressed
 * @return true if the key down was handled
 */
bool TextViewer::handleKeyDown(UINT key, bool controlPressed, bool shiftPressed, bool altPressed) /*throw()*/ {
	using namespace ascension::texteditor::commands;
//	if(altPressed) {
//		if(!shiftPressed || (key != VK_LEFT && key != VK_UP && key != VK_RIGHT && key != VK_DOWN))
//			return false;
//	}
	switch(key) {
	case VK_BACK:	// [BackSpace]
	case VK_F16:	// [F16]
		if(controlPressed)
			WordDeletionCommand(*this, Direction::BACKWARD).execute();
		else
			CharacterDeletionCommand(*this, Direction::BACKWARD).execute();
		return true;
	case VK_CLEAR:	// [Clear]
		if(controlPressed) {
			EntireDocumentSelectionCreationCommand(*this).execute();
			return true;
		}
		break;
	case VK_RETURN:	// [Enter]
		NewlineCommand(*this, controlPressed).execute();
		return true;
	case VK_SHIFT:	// [Shift]
		if(controlPressed
				&& ((toBoolean(::GetAsyncKeyState(VK_LSHIFT) & 0x8000) && configuration_.orientation == RIGHT_TO_LEFT)
				|| (toBoolean(::GetAsyncKeyState(VK_RSHIFT) & 0x8000) && configuration_.orientation == LEFT_TO_RIGHT))) {
			toggleOrientation(*this);
			return true;
		}
		return false;
	case VK_ESCAPE:	// [Esc]
		CancelCommand(*this).execute();
		return true;
	case VK_PRIOR:	// [PageUp]
		if(controlPressed)	onVScroll(SB_PAGEUP, 0, 0);
		else				CaretMovementCommand(*this, &Caret::backwardPage, shiftPressed).execute();
		return true;
	case VK_NEXT:	// [PageDown]
		if(controlPressed)	onVScroll(SB_PAGEDOWN, 0, 0);
		else				CaretMovementCommand(*this, &Caret::forwardPage, shiftPressed).execute();
		return true;
	case VK_HOME:	// [Home]
		CaretMovementCommand(*this, controlPressed ?
			&Caret::beginningOfDocument : &Caret::beginningOfVisualLine, shiftPressed).execute();
		return true;
	case VK_END:	// [End]
		CaretMovementCommand(*this, controlPressed ?
			&Caret::endOfDocument : &Caret::endOfVisualLine, shiftPressed).execute();
		return true;
	case VK_LEFT:	// [Left]
		if(altPressed && shiftPressed)
			RowSelectionExtensionCommand(*this, controlPressed ? &Caret::leftWord : &Caret::leftCharacter).execute();
		else
			CaretMovementCommand(*this, controlPressed ? &Caret::leftWord : &Caret::leftCharacter, shiftPressed).execute();
		return true;
	case VK_UP:		// [Up]
		if(altPressed && shiftPressed && !controlPressed)
			RowSelectionExtensionCommand(*this, &Caret::backwardVisualLine).execute();
		else if(controlPressed && !shiftPressed)
			scroll(0, -1, true);
		else
			CaretMovementCommand(*this, &Caret::backwardVisualLine, shiftPressed).execute();
		return true;
	case VK_RIGHT:	// [Right]
		if(altPressed) {
			if(shiftPressed)
				RowSelectionExtensionCommand(*this, controlPressed ? &Caret::rightWord : &Caret::rightCharacter).execute();
			else
				CompletionProposalPopupCommand(*this).execute();
		} else
			CaretMovementCommand(*this, controlPressed ? &Caret::rightWord: &Caret::rightCharacter, shiftPressed).execute();
		return true;
	case VK_DOWN:	// [Down]
		if(altPressed && shiftPressed && !controlPressed)
			RowSelectionExtensionCommand(*this, &Caret::forwardVisualLine).execute();
		else if(controlPressed && !shiftPressed)
			onVScroll(SB_LINEDOWN, 0, 0);
		else
			CaretMovementCommand(*this, &Caret::forwardVisualLine, shiftPressed).execute();
		return true;
	case VK_INSERT:	// [Insert]
		if(altPressed)
			break;
		else if(!shiftPressed) {
			if(controlPressed)	caret().copySelection(true);
			else				OvertypeModeToggleCommand(*this).execute();
		} else if(controlPressed)
			PasteCommand(*this, false).execute();
		else						break;
		return true;
	case VK_DELETE:	// [Delete]
		if(!shiftPressed) {
			if(controlPressed)
				WordDeletionCommand(*this, Direction::FORWARD).execute();
			else
				CharacterDeletionCommand(*this, Direction::FORWARD).execute();
		} else if(!controlPressed)
			caret().cutSelection(true);
		else
			break;
		return true;
	case 'A':	// ^A -> Select All
		if(controlPressed)
			return EntireDocumentSelectionCreationCommand(*this).execute(), true;
		break;
	case 'C':	// ^C -> Copy
		if(controlPressed)
			return caret().copySelection(true), true;
		break;
	case 'H':	// ^H -> Backspace
		if(controlPressed)
			CharacterDeletionCommand(*this, Direction::BACKWARD).execute(), true;
		break;
	case 'I':	// ^I -> Tab
		if(controlPressed)
			return CharacterInputCommand(*this, 0x0009).execute(), true;
		break;
	case 'J':	// ^J -> New Line
	case 'M':	// ^M -> New Line
		if(controlPressed)
			return NewlineCommand(*this, false).execute(), true;
		break;
	case 'V':	// ^V -> Paste
		if(controlPressed)
			return PasteCommand(*this, false).execute(), true;
		break;
	case 'X':	// ^X -> Cut
		if(controlPressed)
			return caret().cutSelection(true), true;
		break;
	case 'Y':	// ^Y -> Redo
		if(controlPressed)
			return UndoCommand(*this, true).execute(), true;
		break;
	case 'Z':	// ^Z -> Undo
		if(controlPressed)
			return UndoCommand(*this, false).execute(), true;
		break;
	case VK_NUMPAD5:	// [Number Pad 5]
		if(controlPressed)
			return EntireDocumentSelectionCreationCommand(*this).execute(), true;
		break;
	case VK_F12:	// [F12]
		if(controlPressed && shiftPressed)
			return CodePointToCharacterConversionCommand(*this).execute(), true;
		break;
	}
	return false;
}

/// Hides the tool tip.
void TextViewer::hideToolTip() {
	assertValidAsWindow();

	if(tipText_ == 0)
		tipText_ = new Char[1];
	wcscpy(tipText_, L"");
	killTimer(TIMERID_CALLTIP);	// 念のため...
	::SendMessageW(toolTip_, TTM_UPDATE, 0, 0L);
}

/**
 * Determines which part is at the specified position.
 * @param pt the position to hit test, in client coordinates
 * @return the result
 * @see TextViewer#HitTestResult
 */
TextViewer::HitTestResult TextViewer::hitTest(const POINT& pt) const {
	assertValidAsWindow();

	const VerticalRulerConfiguration& vrc = verticalRulerDrawer_->configuration();
	RECT clientRect;
	getClientRect(clientRect);
	if(!toBoolean(::PtInRect(&clientRect, pt)))
		return OUT_OF_VIEW;

	if(vrc.indicatorMargin.visible
			&& ((vrc.alignment == ALIGN_LEFT && pt.x < vrc.indicatorMargin.width)
			|| (vrc.alignment == ALIGN_RIGHT && pt.x >= clientRect.right - vrc.indicatorMargin.width)))
		return INDICATOR_MARGIN;
	else if(vrc.lineNumbers.visible
			&& ((vrc.alignment == ALIGN_LEFT && pt.x < verticalRulerDrawer_->width())
			|| (vrc.alignment == ALIGN_RIGHT && pt.x >= clientRect.right - verticalRulerDrawer_->width())))
		return LINE_NUMBERS;
	else if((vrc.alignment == ALIGN_LEFT && pt.x < verticalRulerDrawer_->width() + configuration_.leadingMargin)
			|| (vrc.alignment == ALIGN_RIGHT && pt.x >= clientRect.right - verticalRulerDrawer_->width() - configuration_.leadingMargin))
		return LEADING_MARGIN;
	else if(pt.y < textAreaMargins().top)
		return TOP_MARGIN;
	else
		return TEXT_AREA;
}

/// Revokes the frozen state of the viewer actually.
inline void TextViewer::internalUnfreeze() {
	assertValidAsWindow();

	if(scrollInfo_.changed) {
		updateScrollBars();
		invalidateRect(0, false);
	} else if(freezeInfo_.invalidLines.first != INVALID_INDEX)
		redrawLines(freezeInfo_.invalidLines.first, freezeInfo_.invalidLines.second);
	freezeInfo_.invalidLines.first = freezeInfo_.invalidLines.second = INVALID_INDEX;

	verticalRulerDrawer_->update();

	caretMoved(caret(), caret().selectionRegion());
	update();
}

/**
 * Converts the distance from the window top to the logical line.
 * @param y the distance
 * @param[out] logicalLine the logical line index. can be @c null if not needed
 * @param[out] visualSublineOffset the offset from the first line in @a logicalLine. can be @c null if not needed
 * @param[out] snapped true if there was not a line at @a y. optional
 * @see #mapLineToClientY, TextRenderer#offsetVisualLine
 */
void TextViewer::mapClientYToLine(int y, length_t* logicalLine, length_t* visualSublineOffset, bool* snapped /* = 0 */) const /*throw()*/ {
	if(logicalLine == 0 && visualSublineOffset == 0)
		return;
	const RECT margins = textAreaMargins();
	if(snapped != 0) {
		RECT clientRect;
		getClientRect(clientRect);
		*snapped = y < clientRect.top + margins.top || y >= clientRect.bottom - margins.bottom;
	}
	y -= margins.top;
	length_t line, subline;
	firstVisibleLine(&line, 0, &subline);
	renderer_->offsetVisualLine(line, subline, y / renderer_->linePitch(), (snapped == 0 || *snapped) ? 0 : snapped);
	if(logicalLine != 0)
		*logicalLine = line;
	if(visualSublineOffset != 0)
		*visualSublineOffset = subline;
}

/**
 * Returns the client y-coordinate of the logical line.
 * @param line the logical line number
 * @param fullSearch false to return special value for the line outside of the client area
 * @return the y-coordinate of the top of the line
 * @retval 32767 @a fullSearch is false and @a line is outside of the client area upward
 * @retval -32768 @a fullSearch is false and @a line is outside of the client area downward
 * @throw BadPositionException @a line is outside of the document
 * @see #mapClientYToLine, TextRenderer#offsetVisualLine
 */
int TextViewer::mapLineToClientY(length_t line, bool fullSearch) const {
	const RECT margins = textAreaMargins();
	if(line == scrollInfo_.firstVisibleLine) {
		if(scrollInfo_.firstVisibleSubline == 0)
			return margins.top;
		else
			return fullSearch ? margins.top - static_cast<int>(renderer_->linePitch() * scrollInfo_.firstVisibleSubline) : -32768;
	} else if(line > scrollInfo_.firstVisibleLine) {
		const int lineSpan = renderer_->linePitch();
		RECT clientRect;
		getClientRect(clientRect);
		int y = margins.top;
		y += lineSpan * static_cast<int>((renderer_->numberOfSublinesOfLine(scrollInfo_.firstVisibleLine) - scrollInfo_.firstVisibleSubline));
		for(length_t i = scrollInfo_.firstVisibleLine + 1; i < line; ++i) {
			y += lineSpan * static_cast<int>(renderer_->numberOfSublinesOfLine(i));
			if(y >= clientRect.bottom - clientRect.top && !fullSearch)
				return 32767;
		}
		return y;
	} else if(!fullSearch)
		return -32768;
	else {
		const int linePitch = renderer_->linePitch();
		int y = margins.top - static_cast<int>(linePitch * scrollInfo_.firstVisibleSubline);
		for(length_t i = scrollInfo_.firstVisibleLine - 1; ; --i) {
			y -= static_cast<int>(renderer_->numberOfSublinesOfLine(i) * linePitch);
			if(i == line)
				break;
		}
		return y;
	}

}

/// @see ICaretStateListener#matchBracketsChanged
void TextViewer::matchBracketsChanged(const Caret& self, const pair<Position, Position>& oldPair, bool outsideOfView) {
	const pair<Position, Position>& newPair = self.matchBrackets();
	if(newPair.first != Position::INVALID_POSITION) {
		assert(newPair.second != Position::INVALID_POSITION);
		redrawLine(newPair.first.line);
		if(!isFrozen())
			update();
		if(newPair.second.line != newPair.first.line) {
			redrawLine(newPair.second.line);
			if(!isFrozen())
				update();
		}
		if(oldPair.first != Position::INVALID_POSITION	// clear the previous highlight
				&& oldPair.first.line != newPair.first.line && oldPair.first.line != newPair.second.line) {
			redrawLine(oldPair.first.line);
			if(!isFrozen())
				update();
		}
		if(oldPair.second != Position::INVALID_POSITION && oldPair.second.line != newPair.first.line
				&& oldPair.second.line != newPair.second.line && oldPair.second.line != oldPair.first.line)
			redrawLine(oldPair.second.line);
	} else {
		if(oldPair.first != Position::INVALID_POSITION) {	// clear the previous highlight
			assert(oldPair.second != Position::INVALID_POSITION);
			redrawLine(oldPair.first.line);
			if(!isFrozen())
				update();
			if(oldPair.second.line != oldPair.first.line)
				redrawLine(oldPair.second.line);
		}
	}
}

/// @see WM_CAPTURECHANGED
void TextViewer::onCaptureChanged(HWND) {
	killTimer(TIMERID_AUTOSCROLL);
	if(mouseInputStrategy_.get() != 0)
		mouseInputStrategy_->captureChanged();
}

/// @see WM_CHAR
void TextViewer::onChar(UINT ch, UINT) {
	handleGUICharacterInput(ch);
}

/// @see Window#onCommand
bool TextViewer::onCommand(WORD id, WORD, HWND) {
	using namespace ascension::texteditor::commands;
	switch(id) {
	case WM_UNDO:	// "Undo"
		UndoCommand(*this, false).execute();
		break;
	case WM_REDO:	// "Redo"
		UndoCommand(*this, true).execute();
		break;
	case WM_CUT:	// "Cut"
		caret().cutSelection(true);
		break;
	case WM_COPY:	// "Copy"
		caret().copySelection(true);
		break;
	case WM_PASTE:	// "Paste"
		PasteCommand(*this, false).execute();
		break;
	case WM_CLEAR:	// "Delete"
		CharacterDeletionCommand(*this, Direction::FORWARD).execute();
		break;
	case WM_SELECTALL:	// "Select All"
		EntireDocumentSelectionCreationCommand(*this).execute();
		break;
	case ID_RTLREADING:	// "Right to left Reading order"
		toggleOrientation(*this);
		break;
	case ID_DISPLAYSHAPINGCONTROLS: {	// "Show Unicode control characters"
		Configuration c(configuration());
		c.displaysShapingControls = !c.displaysShapingControls;
		setConfiguration(&c, 0);
		break;
	}
	case ID_INSERT_LRM:		CharacterInputCommand(*this, 0x200E).execute();	break;
	case ID_INSERT_RLM:		CharacterInputCommand(*this, 0x200F).execute();	break;
	case ID_INSERT_ZWJ:		CharacterInputCommand(*this, 0x200D).execute();	break;
	case ID_INSERT_ZWNJ:	CharacterInputCommand(*this, 0x200C).execute();	break;
	case ID_INSERT_LRE:		CharacterInputCommand(*this, 0x202A).execute();	break;
	case ID_INSERT_RLE:		CharacterInputCommand(*this, 0x202B).execute();	break;
	case ID_INSERT_LRO:		CharacterInputCommand(*this, 0x202D).execute();	break;
	case ID_INSERT_RLO:		CharacterInputCommand(*this, 0x202E).execute();	break;
	case ID_INSERT_PDF:		CharacterInputCommand(*this, 0x202C).execute();	break;
	case ID_INSERT_WJ:		CharacterInputCommand(*this, 0x2060).execute();	break;
	case ID_INSERT_NADS:	CharacterInputCommand(*this, 0x206E).execute();	break;
	case ID_INSERT_NODS:	CharacterInputCommand(*this, 0x206F).execute();	break;
	case ID_INSERT_ASS:		CharacterInputCommand(*this, 0x206B).execute();	break;
	case ID_INSERT_ISS:		CharacterInputCommand(*this, 0x206A).execute();	break;
	case ID_INSERT_AAFS:	CharacterInputCommand(*this, 0x206D).execute();	break;
	case ID_INSERT_IAFS:	CharacterInputCommand(*this, 0x206C).execute();	break;
	case ID_INSERT_RS:		CharacterInputCommand(*this, 0x001E).execute();	break;
	case ID_INSERT_US:		CharacterInputCommand(*this, 0x001F).execute();	break;
	case ID_INSERT_IAA:		CharacterInputCommand(*this, 0xFFF9).execute();	break;
	case ID_INSERT_IAT:		CharacterInputCommand(*this, 0xFFFA).execute();	break;
	case ID_INSERT_IAS:		CharacterInputCommand(*this, 0xFFFB).execute();	break;
	case ID_INSERT_U0020:	CharacterInputCommand(*this, 0x0020).execute();	break;
	case ID_INSERT_NBSP:	CharacterInputCommand(*this, 0x00A0).execute();	break;
	case ID_INSERT_U1680:	CharacterInputCommand(*this, 0x1680).execute();	break;
	case ID_INSERT_MVS:		CharacterInputCommand(*this, 0x180E).execute();	break;
	case ID_INSERT_U2000:	CharacterInputCommand(*this, 0x2000).execute();	break;
	case ID_INSERT_U2001:	CharacterInputCommand(*this, 0x2001).execute();	break;
	case ID_INSERT_U2002:	CharacterInputCommand(*this, 0x2002).execute();	break;
	case ID_INSERT_U2003:	CharacterInputCommand(*this, 0x2003).execute();	break;
	case ID_INSERT_U2004:	CharacterInputCommand(*this, 0x2004).execute();	break;
	case ID_INSERT_U2005:	CharacterInputCommand(*this, 0x2005).execute();	break;
	case ID_INSERT_U2006:	CharacterInputCommand(*this, 0x2006).execute();	break;
	case ID_INSERT_U2007:	CharacterInputCommand(*this, 0x2007).execute();	break;
	case ID_INSERT_U2008:	CharacterInputCommand(*this, 0x2008).execute();	break;
	case ID_INSERT_U2009:	CharacterInputCommand(*this, 0x2009).execute();	break;
	case ID_INSERT_U200A:	CharacterInputCommand(*this, 0x200A).execute();	break;
	case ID_INSERT_ZWSP:	CharacterInputCommand(*this, 0x200B).execute();	break;
	case ID_INSERT_NNBSP:	CharacterInputCommand(*this, 0x202F).execute();	break;
	case ID_INSERT_MMSP:	CharacterInputCommand(*this, 0x205F).execute();	break;
	case ID_INSERT_U3000:	CharacterInputCommand(*this, 0x3000).execute();	break;
	case ID_INSERT_NEL:		CharacterInputCommand(*this, NEXT_LINE).execute();	break;
	case ID_INSERT_LS:		CharacterInputCommand(*this, LINE_SEPARATOR).execute();	break;
	case ID_INSERT_PS:		CharacterInputCommand(*this, PARAGRAPH_SEPARATOR).execute();	break;
	case ID_TOGGLEIMESTATUS:	// "Open IME" / "Close IME"
		InputMethodOpenStatusToggleCommand(*this).execute();
		break;
	case ID_TOGGLESOFTKEYBOARD:	// "Open soft keyboard" / "Close soft keyboard"
		InputMethodSoftKeyboardModeToggleCommand(*this).execute();
		break;
	case ID_RECONVERT:	// "Reconvert"
		ReconversionCommand(*this).execute();
		break;
	case ID_INVOKE_HYPERLINK:	// "Open <hyperlink>"
		if(const hyperlink::IHyperlink* const link = getPointedHyperlink(*this, caret()))
			link->invoke();
		break;
	default:
//		getParent()->sendMessage(WM_COMMAND, MAKEWPARAM(id, notifyCode), reinterpret_cast<LPARAM>(control));
		return true;
	}

	return false;
}

namespace {
	// replaces single "&" with "&&".
	template<typename CharType>
	basic_string<CharType> escapeAmpersands(const basic_string<CharType>& s) {
		static const ctype<CharType>& ct = use_facet<ctype<CharType> >(locale::classic());
		basic_string<CharType> result;
		result.reserve(s.length() * 2);
		for(basic_string<CharType>::size_type i = 0; i < s.length(); ++i) {
			result += s[i];
			if(s[i] == ct.widen('&'))
				result += s[i];
		}
		return result;
	}
} // namespace 0@

/// @see WM_CONTEXTMENU
bool TextViewer::onContextMenu(HWND, const POINT& pt) {
	using manah::win32::ui::Menu;

	if(!allowsMouseInput())	// however, may be invoked by other than the mouse...
		return true;
	closeCompletionProposalsPopup(*this);
	abortIncrementalSearch(*this);

	// invoked by the keyboard
	if(pt.x == 0xFFFF && pt.y == 0xFFFF) {
		const_cast<POINT&>(pt).x = const_cast<POINT&>(pt).y = 1;	// hmm...
		clientToScreen(const_cast<POINT&>(pt));
	}

	// ignore if the point is over the scroll bars
	RECT rect;
	getClientRect(rect);
	clientToScreen(rect);
	if(!toBoolean(::PtInRect(&rect, pt)))
		return false;

	const Document& doc = document();
	const bool hasSelection = !caret().isSelectionEmpty();
	const bool readOnly = doc.isReadOnly();
	const bool japanese = PRIMARYLANGID(getUserDefaultUILanguage()) == LANG_JAPANESE;

	static ui::PopupMenu menu;
	static const WCHAR* captions[] = {
		L"&Undo",									L"\x5143\x306B\x623B\x3059(&U)",
		L"&Redo",									L"\x3084\x308A\x76F4\x3057(&R)",
		0,											0,
		L"Cu&t",									L"\x5207\x308A\x53D6\x308A(&T)",
		L"&Copy",									L"\x30B3\x30D4\x30FC(&C)",
		L"&Paste",									L"\x8CBC\x308A\x4ED8\x3051(&P)",
		L"&Delete",									L"\x524A\x9664(&D)",
		0,											0,
		L"Select &All",								L"\x3059\x3079\x3066\x9078\x629E(&A)",
		0,											0,
		L"&Right to left Reading order",			L"\x53F3\x304B\x3089\x5DE6\x306B\x8AAD\x3080(&R)",
		L"&Show Unicode control characters",		L"Unicode \x5236\x5FA1\x6587\x5B57\x306E\x8868\x793A(&S)",
		L"&Insert Unicode control character",		L"Unicode \x5236\x5FA1\x6587\x5B57\x306E\x633F\x5165(&I)",
		L"Insert Unicode &white space character",	L"Unicode \x7A7A\x767D\x6587\x5B57\x306E\x633F\x5165(&W)",
	};																	
#define GET_CAPTION(index)	captions[(index) * 2 + (japanese ? 1 : 0)]

	if(menu.getNumberOfItems() == 0) {	// first initialization
		menu << Menu::StringItem(WM_UNDO, GET_CAPTION(0))
			<< Menu::StringItem(WM_REDO, GET_CAPTION(1))
			<< Menu::SeparatorItem()
			<< Menu::StringItem(WM_CUT, GET_CAPTION(3))
			<< Menu::StringItem(WM_COPY, GET_CAPTION(4))
			<< Menu::StringItem(WM_PASTE, GET_CAPTION(5))
			<< Menu::StringItem(WM_CLEAR, GET_CAPTION(6))
			<< Menu::SeparatorItem()
			<< Menu::StringItem(WM_SELECTALL, GET_CAPTION(8))
			<< Menu::SeparatorItem()
			<< Menu::StringItem(ID_RTLREADING, GET_CAPTION(10))
			<< Menu::StringItem(ID_DISPLAYSHAPINGCONTROLS, GET_CAPTION(11))
			<< Menu::StringItem(0, GET_CAPTION(12))
			<< Menu::StringItem(0, GET_CAPTION(13));

		// under "Insert Unicode control character"
		auto_ptr<Menu> subMenu(new ui::PopupMenu);
		*subMenu << Menu::StringItem(ID_INSERT_LRM, L"LRM\t&Left-To-Right Mark")
			<< Menu::StringItem(ID_INSERT_RLM, L"RLM\t&Right-To-Left Mark")
			<< Menu::StringItem(ID_INSERT_ZWJ, L"ZWJ\t&Zero Width Joiner")
			<< Menu::StringItem(ID_INSERT_ZWNJ, L"ZWNJ\tZero Width &Non-Joiner")
			<< Menu::StringItem(ID_INSERT_LRE, L"LRE\tLeft-To-Right &Embedding")
			<< Menu::StringItem(ID_INSERT_RLE, L"RLE\tRight-To-Left E&mbedding")
			<< Menu::StringItem(ID_INSERT_LRO, L"LRO\tLeft-To-Right &Override")
			<< Menu::StringItem(ID_INSERT_RLO, L"RLO\tRight-To-Left O&verride")
			<< Menu::StringItem(ID_INSERT_PDF, L"PDF\t&Pop Directional Formatting")
			<< Menu::StringItem(ID_INSERT_WJ, L"WJ\t&Word Joiner")
			<< Menu::StringItem(ID_INSERT_NADS, L"NADS\tN&ational Digit Shapes (deprecated)")
			<< Menu::StringItem(ID_INSERT_NODS, L"NODS\tNominal &Digit Shapes (deprecated)")
			<< Menu::StringItem(ID_INSERT_ASS, L"ASS\tActivate &Symmetric Swapping (deprecated)")
			<< Menu::StringItem(ID_INSERT_ISS, L"ISS\tInhibit S&ymmetric Swapping (deprecated)")
			<< Menu::StringItem(ID_INSERT_AAFS, L"AAFS\tActivate Arabic &Form Shaping (deprecated)")
			<< Menu::StringItem(ID_INSERT_IAFS, L"IAFS\tInhibit Arabic Form S&haping (deprecated)")
			<< Menu::StringItem(ID_INSERT_RS, L"RS\tRe&cord Separator")
			<< Menu::StringItem(ID_INSERT_US, L"US\tUnit &Separator")
			<< Menu::SeparatorItem()
			<< Menu::StringItem(ID_INSERT_IAA, L"IAA\tInterlinear Annotation Anchor")
			<< Menu::StringItem(ID_INSERT_IAT, L"IAT\tInterlinear Annotation Terminator")
			<< Menu::StringItem(ID_INSERT_IAS, L"IAS\tInterlinear Annotation Separator");
		menu.setChildPopup<Menu::BY_POSITION>(12, subMenu);

		// under "Insert Unicode white space character"
		subMenu.reset(new ui::PopupMenu);
		*subMenu << Menu::StringItem(ID_INSERT_U0020, L"U+0020\tSpace")
			<< Menu::StringItem(ID_INSERT_NBSP, L"NBSP\tNo-Break Space")
			<< Menu::StringItem(ID_INSERT_U1680, L"U+1680\tOgham Space Mark")
			<< Menu::StringItem(ID_INSERT_MVS, L"MVS\tMongolian Vowel Separator")
			<< Menu::StringItem(ID_INSERT_U2000, L"U+2000\tEn Quad")
			<< Menu::StringItem(ID_INSERT_U2001, L"U+2001\tEm Quad")
			<< Menu::StringItem(ID_INSERT_U2002, L"U+2002\tEn Space")
			<< Menu::StringItem(ID_INSERT_U2003, L"U+2003\tEm Space")
			<< Menu::StringItem(ID_INSERT_U2004, L"U+2004\tThree-Per-Em Space")
			<< Menu::StringItem(ID_INSERT_U2005, L"U+2005\tFour-Per-Em Space")
			<< Menu::StringItem(ID_INSERT_U2006, L"U+2006\tSix-Per-Em Space")
			<< Menu::StringItem(ID_INSERT_U2007, L"U+2007\tFigure Space")
			<< Menu::StringItem(ID_INSERT_U2008, L"U+2008\tPunctuation Space")
			<< Menu::StringItem(ID_INSERT_U2009, L"U+2009\tThin Space")
			<< Menu::StringItem(ID_INSERT_U200A, L"U+200A\tHair Space")
			<< Menu::StringItem(ID_INSERT_ZWSP, L"ZWSP\tZero Width Space")
			<< Menu::StringItem(ID_INSERT_NNBSP, L"NNBSP\tNarrow No-Break Space")
			<< Menu::StringItem(ID_INSERT_MMSP, L"MMSP\tMedium Mathematical Space")
			<< Menu::StringItem(ID_INSERT_U3000, L"U+3000\tIdeographic Space")
			<< Menu::SeparatorItem()
			<< Menu::StringItem(ID_INSERT_NEL, L"NEL\tNext Line")
			<< Menu::StringItem(ID_INSERT_LS, L"LS\tLine Separator")
			<< Menu::StringItem(ID_INSERT_PS, L"PS\tParagraph Separator");
		menu.setChildPopup<Menu::BY_POSITION>(13, subMenu);

		// check if the system supports bidi
		if(!supportsComplexScripts()) {
			menu.enable<Menu::BY_COMMAND>(ID_RTLREADING, false);
			menu.enable<Menu::BY_COMMAND>(ID_DISPLAYSHAPINGCONTROLS, false);
			menu.enable<Menu::BY_POSITION>(12, false);
			menu.enable<Menu::BY_POSITION>(13, false);
		}
	}
#undef GET_CAPTION

	// modify menu items
	menu.enable<Menu::BY_COMMAND>(WM_UNDO, !readOnly && doc.numberOfUndoableChanges() != 0);
	menu.enable<Menu::BY_COMMAND>(WM_REDO, !readOnly && doc.numberOfRedoableChanges() != 0);
	menu.enable<Menu::BY_COMMAND>(WM_CUT, !readOnly && hasSelection);
	menu.enable<Menu::BY_COMMAND>(WM_COPY, hasSelection);
	menu.enable<Menu::BY_COMMAND>(WM_PASTE, !readOnly && caret_->canPaste());
	menu.enable<Menu::BY_COMMAND>(WM_CLEAR, !readOnly && hasSelection);
	menu.enable<Menu::BY_COMMAND>(WM_SELECTALL, doc.numberOfLines() > 1 || doc.lineLength(0) > 0);
	menu.check<Menu::BY_COMMAND>(ID_RTLREADING, configuration_.orientation == RIGHT_TO_LEFT);
	menu.check<Menu::BY_COMMAND>(ID_DISPLAYSHAPINGCONTROLS, configuration_.displaysShapingControls);

	// IME commands
	HKL keyboardLayout = ::GetKeyboardLayout(::GetCurrentThreadId());
	if(//toBoolean(::ImmIsIME(keyboardLayout)) &&
			::ImmGetProperty(keyboardLayout, IGP_SENTENCE) != IME_SMODE_NONE) {
		HIMC imc = ::ImmGetContext(getHandle());
		WCHAR* openIme = japanese ? L"IME \x3092\x958B\x304F(&O)" : L"&Open IME";
		WCHAR* closeIme = japanese ? L"IME \x3092\x9589\x3058\x308B(&L)" : L"C&lose IME";
		WCHAR* openSftKbd = japanese ? L"\x30BD\x30D5\x30C8\x30AD\x30FC\x30DC\x30FC\x30C9\x3092\x958B\x304F(&E)" : L"Op&en soft keyboard";
		WCHAR* closeSftKbd = japanese ? L"\x30BD\x30D5\x30C8\x30AD\x30FC\x30DC\x30FC\x30C9\x3092\x9589\x3058\x308B(&F)" : L"Close so&ft keyboard";
		WCHAR* reconvert = japanese ? L"\x518D\x5909\x63DB(&R)" : L"&Reconvert";

		menu << Menu::SeparatorItem()
			<< Menu::StringItem(ID_TOGGLEIMESTATUS, toBoolean(::ImmGetOpenStatus(imc)) ? closeIme : openIme);

		if(toBoolean(::ImmGetProperty(keyboardLayout, IGP_CONVERSION) & IME_CMODE_SOFTKBD)) {
			DWORD convMode;
			::ImmGetConversionStatus(imc, &convMode, 0);
			menu << Menu::StringItem(ID_TOGGLESOFTKEYBOARD, toBoolean(convMode & IME_CMODE_SOFTKBD) ? closeSftKbd : openSftKbd);
		}

		if(toBoolean(::ImmGetProperty(keyboardLayout, IGP_SETCOMPSTR) & SCS_CAP_SETRECONVERTSTRING))
			menu << Menu::StringItem(ID_RECONVERT, reconvert, (!readOnly && hasSelection) ? MFS_ENABLED : MFS_GRAYED);

		::ImmReleaseContext(getHandle(), imc);
	}

	// hyperlink
	if(const hyperlink::IHyperlink* link = getPointedHyperlink(*this, caret())) {
		AutoBuffer<WCHAR> caption(	// TODO: this code can have buffer overflow in future
			new WCHAR[(link->region().end() - link->region().beginning()) * 2 + 8]);
		swprintf(caption.get(),
			japanese ? L"\x202A%s\x202C \x3092\x958B\x304F" : L"Open \x202A%s\x202C", escapeAmpersands(doc.line(
				caret().lineNumber()).substr(link->region().beginning(), link->region().end() - link->region().beginning())).c_str());
		menu << Menu::SeparatorItem() << Menu::StringItem(ID_INVOKE_HYPERLINK, caption.get());
	}

	menu.trackPopup(TPM_LEFTALIGN, pt.x, pt.y, getHandle());

	// ...finally erase all items
	int c = menu.getNumberOfItems();
	while(c > 13)
		menu.erase<Menu::BY_POSITION>(c--);

	return true;
}

/// @see WM_DESTROY
void TextViewer::onDestroy() {
	endAutoScroll();
	if(mouseInputStrategy_.get() != 0) {
		mouseInputStrategy_->uninstall();
		mouseInputStrategy_.reset();
	}

	// destroy children
	::DestroyWindow(toolTip_);
	if(autoScrollOriginMark_.get() != 0)
		autoScrollOriginMark_->destroy();

#ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY
	if(accessibleProxy_ != 0)
		accessibleProxy_->dispose();
//	if(accLib.isAvailable())
//		accLib.notifyWinEvent(EVENT_OBJECT_DESTROY, *this, OBJID_CLIENT, CHILDID_SELF);
#endif // !ASCENSION_NO_ACTIVE_ACCESSIBILITY
}

/// @see WM_ERASEGKGND
bool TextViewer::onEraseBkgnd(HDC) {
	return false;
}

/// @see WM_GETFONT
HFONT TextViewer::onGetFont() {
	return renderer_->font();
}

/// @see WM_HSCROLL
void TextViewer::onHScroll(UINT sbCode, UINT, HWND) {
	switch(sbCode) {
	case SB_LINELEFT:	// 1 列分左
		scroll(-1, 0, true); break;
	case SB_LINERIGHT:	// 1 列分右
		scroll(+1, 0, true); break;
	case SB_PAGELEFT:	// 1 ページ左
		scroll(-static_cast<int>(numberOfVisibleColumns()), 0, true); break;
	case SB_PAGERIGHT:	// 1 ページ右
		scroll(+static_cast<int>(numberOfVisibleColumns()), 0, true); break;
	case SB_LEFT:		// 左端
	case SB_RIGHT: {	// 右端
		int left, right;
		getScrollRange(SB_HORZ, left, right);
		scrollTo((sbCode == SB_LEFT) ? left : right, -1, true); break;
	}
	case SB_THUMBTRACK:	// by drag or wheel
		scrollTo(getScrollTrackPosition(SB_HORZ), -1, false); break;	// use 32-bit value
	}
}

/// @see WM_IME_COMPOSITION
void TextViewer::onIMEComposition(WPARAM wParam, LPARAM lParam, bool& handled) {
	if(document().isReadOnly())
		return;
	else if(/*lParam == 0 ||*/ toBoolean(lParam & GCS_RESULTSTR)) {	// completed
		if(HIMC imc = ::ImmGetContext(getHandle())) {
			if(const length_t len = ::ImmGetCompositionStringW(imc, GCS_RESULTSTR, 0, 0) / sizeof(WCHAR)) {
				// this was not canceled
				const AutoBuffer<Char> text(new Char[len + 1]);
				::ImmGetCompositionStringW(imc, GCS_RESULTSTR, text.get(), static_cast<DWORD>(len * sizeof(WCHAR)));
				text[len] = 0;
				if(!imeComposingCharacter_)
					texteditor::commands::TextInputCommand(*this, text.get()).execute();
				else {
					Document& doc = document();
					doc.insertUndoBoundary();
					if(doc.erase(*caret_, static_cast<DocumentCharacterIterator&>(
							DocumentCharacterIterator(doc, caret()).next()).tell())) {
						doc.insert(*caret_, String(1, static_cast<Char>(wParam)));
						doc.insertUndoBoundary();
						imeComposingCharacter_ = false;
						recreateCaret();
					}
				}
			}
//			updateIMECompositionWindowPosition();
			::ImmReleaseContext(getHandle(), imc);
			handled = true;	// prevent to be send WM_CHARs
		}
	} else if(toBoolean(GCS_COMPSTR & lParam)) {
		if(toBoolean(lParam & CS_INSERTCHAR)) {
			Document& doc = document();
			if(imeComposingCharacter_)
				doc.erase(*caret_,
					static_cast<DocumentCharacterIterator&>(DocumentCharacterIterator(doc, caret()).next()).tell());
			doc.insert(*caret_, String(1, static_cast<Char>(wParam)));
			imeComposingCharacter_ = true;
			handled = true;
			if(toBoolean(lParam & CS_NOMOVECARET))
				caret_->backwardCharacter();
			recreateCaret();
		}
	}
}

/// @see WM_IME_ENDCOMPOSITION
void TextViewer::onIMEEndComposition() {
	imeCompositionActivated_ = false;
	recreateCaret();
}

/// @see WM_IME_NOTIFY
LRESULT TextViewer::onIMENotify(WPARAM command, LPARAM, bool&) {
	if(command == IMN_SETOPENSTATUS)
		inputStatusListeners_.notify(&ITextViewerInputStatusListener::textViewerIMEOpenStatusChanged);
	return 0L;
}

/// @see WM_IME_REQUEST
LRESULT TextViewer::onIMERequest(WPARAM command, LPARAM lParam, bool& handled) {
	const Document& doc = document();

	// this command will be sent two times when reconversion is invoked
	if(command == IMR_RECONVERTSTRING) {
		if(doc.isReadOnly() || caret().isSelectionRectangle()) {
			beep();
			return 0L;
		}
		handled = true;
		if(caret_->isSelectionEmpty()) {	// IME selects the composition target automatically if no selection
			if(RECONVERTSTRING* const rcs = reinterpret_cast<RECONVERTSTRING*>(lParam)) {
				const String& line = doc.line(caret().lineNumber());
				rcs->dwStrLen = static_cast<DWORD>(line.length());
				rcs->dwStrOffset = sizeof(RECONVERTSTRING);
				rcs->dwTargetStrOffset = rcs->dwCompStrOffset = static_cast<DWORD>(sizeof(Char) * caret().columnNumber());
				rcs->dwTargetStrLen = rcs->dwCompStrLen = 0;
				line.copy(reinterpret_cast<Char*>(reinterpret_cast<char*>(rcs) + rcs->dwStrOffset), rcs->dwStrLen);
			}
			return sizeof(RECONVERTSTRING) + sizeof(Char) * doc.lineLength(caret().lineNumber());
		} else {
			const String selection(caret().selectionText(NLF_RAW_VALUE));
			if(RECONVERTSTRING* const rcs = reinterpret_cast<RECONVERTSTRING*>(lParam)) {
				rcs->dwStrLen = rcs->dwTargetStrLen = rcs->dwCompStrLen = static_cast<DWORD>(selection.length());
				rcs->dwStrOffset = sizeof(RECONVERTSTRING);
				rcs->dwTargetStrOffset = rcs->dwCompStrOffset = 0;
				selection.copy(reinterpret_cast<Char*>(reinterpret_cast<char*>(rcs) + rcs->dwStrOffset), rcs->dwStrLen);
			}
			return sizeof(RECONVERTSTRING) + sizeof(Char) * selection.length();
		}
	}

	// before reconversion. a RECONVERTSTRING contains the ranges of the composition
	else if(command == IMR_CONFIRMRECONVERTSTRING) {
		if(RECONVERTSTRING* const rcs = reinterpret_cast<RECONVERTSTRING*>(lParam)) {
			const Region region(doc.accessibleRegion());
			if(!caret().isSelectionEmpty()) {
				// reconvert the selected region. the selection may be multi-line
				if(rcs->dwCompStrLen < rcs->dwStrLen)	// the composition region was truncated.
					rcs->dwCompStrLen = rcs->dwStrLen;	// IME will alert and reconversion will not be happen if do this
														// (however, NotePad narrows the selection...)
			} else {
				// reconvert the region IME passed if no selection (and create the new selection).
				// in this case, reconversion across multi-line (prcs->dwStrXxx represents the entire line)
				if(doc.isNarrowed() && caret().lineNumber() == region.first.line) {	// the document is narrowed
					if(rcs->dwCompStrOffset / sizeof(Char) < region.first.column) {
						rcs->dwCompStrLen += static_cast<DWORD>(sizeof(Char) * region.first.column - rcs->dwCompStrOffset);
						rcs->dwTargetStrLen = rcs->dwCompStrOffset;
						rcs->dwCompStrOffset = rcs->dwTargetStrOffset = static_cast<DWORD>(sizeof(Char) * region.first.column);
					} else if(rcs->dwCompStrOffset / sizeof(Char) > region.second.column) {
						rcs->dwCompStrOffset -= rcs->dwCompStrOffset - sizeof(Char) * region.second.column;
						rcs->dwTargetStrOffset = rcs->dwCompStrOffset;
						rcs->dwCompStrLen = rcs->dwTargetStrLen
							= static_cast<DWORD>(sizeof(Char) * region.second.column - rcs->dwCompStrOffset);
					}
				}
				caret().select(
					Position(caret().lineNumber(), rcs->dwCompStrOffset / sizeof(Char)),
					Position(caret().lineNumber(), rcs->dwCompStrOffset / sizeof(Char) + rcs->dwCompStrLen));
			}
			handled = true;
			return true;
		}
	}

	// queried position of the composition window
	else if(command == IMR_QUERYCHARPOSITION)
		return false;	// handled by updateIMECompositionWindowPosition...

	// queried document content for higher conversion accuracy
	else if(command == IMR_DOCUMENTFEED) {
		if(caret().lineNumber() == caret().anchor().lineNumber()) {
			handled = true;
			if(RECONVERTSTRING* const rcs = reinterpret_cast<RECONVERTSTRING*>(lParam)) {
				rcs->dwStrLen = static_cast<DWORD>(doc.lineLength(caret().lineNumber()));
				rcs->dwStrOffset = sizeof(RECONVERTSTRING);
				rcs->dwCompStrLen = rcs->dwTargetStrLen = 0;
				rcs->dwCompStrOffset = rcs->dwTargetStrOffset = sizeof(Char) * static_cast<DWORD>(caret().beginning().columnNumber());
				doc.line(caret().lineNumber()).copy(reinterpret_cast<Char*>(reinterpret_cast<char*>(rcs) + rcs->dwStrOffset), rcs->dwStrLen);
			}
			return sizeof(RECONVERTSTRING) + sizeof(Char) * doc.lineLength(caret().lineNumber());
		}
	}

	return 0L;
}

/// @see WM_IME_STARTCOMPOSITION
void TextViewer::onIMEStartComposition() {
	imeCompositionActivated_ = true;
	updateIMECompositionWindowPosition();
	closeCompletionProposalsPopup(*this);
}

/// @see WM_KEYDOWN
void TextViewer::onKeyDown(UINT vkey, UINT, bool& handled) {
	endAutoScroll();
	handled = handleKeyDown(vkey, toBoolean(::GetKeyState(VK_CONTROL) & 0x8000), toBoolean(::GetKeyState(VK_SHIFT) & 0x8000), false);
}

/// @see WM_KILLFOCUS
void TextViewer::onKillFocus(HWND newWindow) {
	RESTORE_HIDDEN_CURSOR();
	endAutoScroll();
/*	if(caret_->getMatchBracketsTrackingMode() != Caret::DONT_TRACK
			&& getCaret().getMatchBrackets().first != Position::INVALID_POSITION) {	// 対括弧の通知を終了
		FOR_EACH_LISTENERS()
			(*it)->onMatchBracketFoundOutOfView(Position::INVALID_POSITION);
	}
	if(completionWindow_->isWindow() && newWindow != completionWindow_->getSafeHwnd())
		closeCompletionProposalsPopup(*this);
*/	abortIncrementalSearch(*this);
	if(imeCompositionActivated_) {	// stop IME input
		HIMC imc = ::ImmGetContext(getHandle());
		::ImmNotifyIME(imc, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
		::ImmReleaseContext(getHandle(), imc);
	}
	if(newWindow != getHandle()) {
		hideCaret();
		::DestroyCaret();
	}
	redrawLines(caret().beginning().lineNumber(), caret().end().lineNumber());
	update();
}

/// @see WM_LBUTTONDBLCLK
void TextViewer::onLButtonDblClk(UINT keyState, const POINT& pt) {
	if(allowsMouseInput() && mouseInputStrategy_.get() != 0)
		mouseInputStrategy_->mouseButtonInput(IMouseInputStrategy::LEFT_BUTTON, IMouseInputStrategy::DOUBLE_CLICKED, pt, keyState);
}

/// @see WM_LBUTTONDOWN
void TextViewer::onLButtonDown(UINT keyState, const POINT& pt) {
	RESTORE_HIDDEN_CURSOR();
	if(allowsMouseInput() && mouseInputStrategy_.get() != 0)
		mouseInputStrategy_->mouseButtonInput(IMouseInputStrategy::LEFT_BUTTON, IMouseInputStrategy::PRESSED, pt, keyState);
}

/// @see WM_LBUTTONUP
void TextViewer::onLButtonUp(UINT keyState, const POINT& pt) {
	RESTORE_HIDDEN_CURSOR();
	if(allowsMouseInput() && mouseInputStrategy_.get() != 0)
		mouseInputStrategy_->mouseButtonInput(IMouseInputStrategy::LEFT_BUTTON, IMouseInputStrategy::RELEASED, pt, keyState);
}

/// @see WM_MBUTTONDBLCLK
void TextViewer::onMButtonDblClk(UINT keyState, const POINT& pt) {
	if(allowsMouseInput() && mouseInputStrategy_.get() != 0)
		mouseInputStrategy_->mouseButtonInput(IMouseInputStrategy::MIDDLE_BUTTON, IMouseInputStrategy::DOUBLE_CLICKED, pt, keyState);
}

/// @see WM_MBUTTONDOWN
void TextViewer::onMButtonDown(UINT keyState, const POINT& pt) {
	RESTORE_HIDDEN_CURSOR();
	if(allowsMouseInput() && mouseInputStrategy_.get() != 0)
		mouseInputStrategy_->mouseButtonInput(IMouseInputStrategy::MIDDLE_BUTTON, IMouseInputStrategy::PRESSED, pt, keyState);
}

/// @see WM_MBUTTONUP
void TextViewer::onMButtonUp(UINT keyState, const POINT& pt) {
	RESTORE_HIDDEN_CURSOR();
	if(allowsMouseInput() && mouseInputStrategy_.get() != 0)
		mouseInputStrategy_->mouseButtonInput(IMouseInputStrategy::MIDDLE_BUTTON, IMouseInputStrategy::RELEASED, pt, keyState);
}

/// @see WM_MOUSEMOVE
void TextViewer::onMouseMove(UINT keyState, const POINT& position) {
	RESTORE_HIDDEN_CURSOR();
	if(allowsMouseInput() && mouseInputStrategy_.get() != 0)
		mouseInputStrategy_->mouseMoved(position, keyState);
}

/// @see WM_MOUSEWHEEL
void TextViewer::onMouseWheel(UINT keyState, short delta, const POINT& pt) {
	RESTORE_HIDDEN_CURSOR();
	if(allowsMouseInput() && mouseInputStrategy_.get() != 0)
		mouseInputStrategy_->mouseWheelRotated(delta, pt, keyState);
}

/// @see WM_NCCREATE
bool TextViewer::onNcCreate(CREATESTRUCTW&) {
	modifyStyleEx(WS_EX_LAYOUTRTL, 0L);
	return true;
}

/// @see WM_NOTIFY
bool TextViewer::onNotify(int, NMHDR& nmhdr) {
	// tooltip text
	if(nmhdr.hwndFrom == toolTip_ && nmhdr.code == TTN_GETDISPINFOW) {
		::SendMessageW(toolTip_, TTM_SETMAXTIPWIDTH, 0, 1000);	// make line breaks effective
		reinterpret_cast<LPNMTTDISPINFOW>(&nmhdr)->lpszText = tipText_;
		return false;
	}
	return true;
}

/// @see CustomControl#onPaint
void TextViewer::onPaint(PaintDC& dc) {
	if(isFrozen())	// skip if frozen
		return;
	else if(toBoolean(::IsRectEmpty(&dc.getPaintStruct().rcPaint)))	// skip if the region to paint is empty
		return;

	const Document& doc = document();
	RECT clientRect;
	getClientRect(clientRect);

//	Timer tm(L"onPaint");

	const length_t lines = doc.numberOfLines();
	const RECT& paintRect = dc.getPaintStruct().rcPaint;
	const int linePitch = renderer_->linePitch();

	// draw the vertical ruler
	verticalRulerDrawer_->draw(dc);

	// draw horizontal margins
	const RECT margins = textAreaMargins();
	const COLORREF marginColor = configuration_.color.background.isValid() ?
		configuration_.color.background.asCOLORREF() : ::GetSysColor(COLOR_WINDOW);
	if(margins.left > 0) {
		const int vrWidth = (verticalRulerDrawer_->configuration().alignment == ALIGN_LEFT) ? verticalRulerDrawer_->width() : 0;
		dc.fillSolidRect(clientRect.left + vrWidth, paintRect.top, margins.left - vrWidth, paintRect.bottom - paintRect.top, marginColor);
	}
	if(margins.right > 0) {
		const int vrWidth = (verticalRulerDrawer_->configuration().alignment == ALIGN_RIGHT) ? verticalRulerDrawer_->width() : 0;
		dc.fillSolidRect(clientRect.right - margins.right, paintRect.top, margins.right - vrWidth, paintRect.bottom - paintRect.top, marginColor);
	}

	// draw lines
	const Colors selectionColor(
		configuration_.selectionColor.foreground.isValid() ?
			configuration_.selectionColor.foreground :
				Color::fromCOLORREF(::GetSysColor(hasFocus() ? COLOR_HIGHLIGHTTEXT : COLOR_INACTIVECAPTIONTEXT)),
		configuration_.selectionColor.background.isValid() ?
			configuration_.selectionColor.background :
				Color::fromCOLORREF(::GetSysColor(hasFocus() ? COLOR_HIGHLIGHT : COLOR_INACTIVECAPTION)));
	RECT lineRect = clientRect;
	lineRect.left += margins.left; lineRect.top += margins.top; lineRect.right -= margins.right; lineRect.bottom -= margins.bottom;
	length_t line, subline;
	mapClientYToLine(paintRect.top, &line, &subline);
	int y = mapLineToClientY(line, true);
	if(line < lines) {
		while(y < paintRect.bottom && line < lines) {
			// draw a logical line
			LineLayout::Selection selection(*caret_, selectionColor);
			renderer_->renderLine(line, dc, getDisplayXOffset(line), y, dc.getPaintStruct().rcPaint, lineRect, &selection);
			y += linePitch * static_cast<int>(renderer_->numberOfSublinesOfLine(line++));
			subline = 0;
		}
	}

	// paint behind the last
	if(paintRect.bottom > y && y > margins.top + linePitch - 1)
		dc.fillSolidRect(clientRect.left + margins.left, y,
			clientRect.right - clientRect.left - margins.left - margins.right, paintRect.bottom - y, marginColor);

	// draw top margin
	if(margins.top > 0)
		dc.fillSolidRect(clientRect.left + margins.left, clientRect.top,
			clientRect.right - clientRect.left - margins.left - margins.right, margins.top, marginColor);
}

/// @see WM_RBUTTONDBLCLK
void TextViewer::onRButtonDblClk(UINT keyState, const POINT& pt) {
	if(allowsMouseInput() && mouseInputStrategy_.get() != 0)
		mouseInputStrategy_->mouseButtonInput(IMouseInputStrategy::RIGHT_BUTTON, IMouseInputStrategy::DOUBLE_CLICKED, pt, keyState);
}

/// @see WM_RBUTTONDOWN
void TextViewer::onRButtonDown(UINT keyState, const POINT& pt) {
	RESTORE_HIDDEN_CURSOR();
	if(allowsMouseInput() && mouseInputStrategy_.get() != 0)
		mouseInputStrategy_->mouseButtonInput(IMouseInputStrategy::RIGHT_BUTTON, IMouseInputStrategy::PRESSED, pt, keyState);
}

/// @see WM_RBUTTONUP
void TextViewer::onRButtonUp(UINT keyState, const POINT& pt) {
	if(allowsMouseInput()) {
		RESTORE_HIDDEN_CURSOR();
		if(mouseInputStrategy_.get() != 0)
			mouseInputStrategy_->mouseButtonInput(IMouseInputStrategy::RIGHT_BUTTON, IMouseInputStrategy::RELEASED, pt, keyState);
	}
}

/// @see WM_SETCURSOR
bool TextViewer::onSetCursor(HWND, UINT, UINT) {
	static length_t detectedUriLineLast = INVALID_INDEX;
	POINT pt = getCursorPosition();	// カーソル位置
	bool cursorChanged = false;

	RESTORE_HIDDEN_CURSOR();

	// リンクのポップアップやカーソルを変える場合
//	const HitTestResult htr = hitTest(pt);
/*	if(htr != INDICATOR_MARGIN && htr != LINE_NUMBERS && !autoScroll_.scrolling && linkTextStrategy_.get() != 0) {
		Region region;
		AutoBuffer<Char> uri;
		if(getPointedLinkText(region, uri)) {	// URI
			// ポップアップを出す
			const length_t cursorDisplayLine = (pt.y - getTextAreaMargins().top) / renderer_->getLinePitch();
			String description;
			HCURSOR cursor = 0;
			if(cursorDisplayLine != detectedUriLineLast
					&& linkTextStrategy_->getLinkInformation(region, uri.get(), description, cursor)) {
				if(!description.empty()) {
					detectedUriLineLast = cursorDisplayLine;
					showToolTip(description, 1000, 30000);
				}
				// カーソルを変える
				if(cursor != 0) {
					::SetCursor(cursor);
					cursorChanged = true;
				}
			}
		} else {
			detectedUriLineLast = INVALID_INDEX;
			hideToolTip();
		}
	}
*/
	if(!cursorChanged && mouseInputStrategy_.get() != 0)
		cursorChanged = mouseInputStrategy_->showCursor(pt);

	return cursorChanged;
}

/// @see WM_SETFOCUS
void TextViewer::onSetFocus(HWND oldWindow) {
	// restore the scroll positions
	setScrollPosition(SB_HORZ, scrollInfo_.horizontal.position, false);
	setScrollPosition(SB_VERT, scrollInfo_.vertical.position, true);

	// hmm...
//	if(/*sharedData_->options.appearance[SHOW_CURRENT_UNDERLINE] ||*/ !getCaret().isSelectionEmpty()) {
		redrawLines(caret().beginning().lineNumber(), caret().end().lineNumber());
		update();
//	}

	if(oldWindow != getHandle()) {
		// resurrect the caret
		recreateCaret();
		updateCaretPosition();
		if(texteditor::Session* const session = document().session()) {
			if(texteditor::InputSequenceCheckers* const isc = session->inputSequenceCheckers())
				isc->setKeyboardLayout(::GetKeyboardLayout(::GetCurrentThreadId()));
		}
	}
}

/// @see WM_SIZE
void TextViewer::onSize(UINT type, int, int) {
	closeCompletionProposalsPopup(*this);
	if(type == SIZE_MINIMIZED)
		return;

	// ツールチップに通知
	MANAH_AUTO_STRUCT_SIZE(TOOLINFOW, ti);
	RECT viewRect;
	getClientRect(viewRect);
	ti.hwnd = getHandle();
	ti.uId = 1;
	ti.rect = viewRect;
	::SendMessageW(toolTip_, TTM_NEWTOOLRECT, 0, reinterpret_cast<LPARAM>(&ti));

	if(renderer_.get() == 0)
		return;

	if(configuration_.lineWrap.wrapsAtWindowEdge())
		renderer_->invalidate();
	displaySizeListeners_.notify(&IDisplaySizeListener::viewerDisplaySizeChanged);
	scrollInfo_.resetBars(*this, SB_BOTH, true);
	updateScrollBars();
	verticalRulerDrawer_->update();
	if(verticalRulerDrawer_->configuration().alignment != ALIGN_LEFT) {
		recreateCaret();
//		redrawVerticalRuler();
		invalidateRect(0, false);	// うーむ...
	}
}

/// @see WM_STYLECHANGED
void TextViewer::onStyleChanged(int type, const STYLESTRUCT& style) {
	if(type == GWL_EXSTYLE
			&& (((style.styleOld ^ style.styleNew) & (WS_EX_RIGHT | WS_EX_RTLREADING)) != 0)) {
		// ウィンドウスタイルの変更をに Presentation に反映する
		Configuration c(configuration());
		c.orientation = ((style.styleNew & WS_EX_RTLREADING) != 0) ? RIGHT_TO_LEFT : LEFT_TO_RIGHT;
		c.alignment = ((style.styleNew & WS_EX_RIGHT) != 0) ? ALIGN_RIGHT : ALIGN_LEFT;
		setConfiguration(&c, 0);
	}
}

/// @see WM_STYLECHANGING
void TextViewer::onStyleChanging(int type, STYLESTRUCT& style) {
	if(type == GWL_EXSTYLE)
		style.styleNew &= ~WS_EX_LAYOUTRTL;	// このウィンドウの DC のレイアウトは常に LTR でなければならぬ
}

/// @see WM_SYSCHAR
void TextViewer::onSysChar(UINT, UINT) {
	RESTORE_HIDDEN_CURSOR();
}

/// @see WM_SYSCOLORCHANGE
void TextViewer::onSysColorChange() {
//	if(this == originalView_)
//		presentation_.updateSystemColors();
}

/// @see WM_SYSKEYDOWN
bool TextViewer::onSysKeyDown(UINT vkey, UINT) {
	endAutoScroll();
	return handleKeyDown(vkey, toBoolean(::GetKeyState(VK_CONTROL) & 0x8000), toBoolean(::GetKeyState(VK_SHIFT) & 0x8000), true);;
}

/// @see WM_SYSKEYUP
bool TextViewer::onSysKeyUp(UINT, UINT) {
	RESTORE_HIDDEN_CURSOR();
	return false;
}

#ifdef WM_THEMECHANGED
/// @see WM_THEMECHANGED
void TextViewer::onThemeChanged() {
	// see onSysColorChange()
}
#endif /* WM_THEMECHANGED */

/// @see WM_TIMER
void TextViewer::onTimer(UINT_PTR eventID, TIMERPROC) {
	if(eventID == TIMERID_CALLTIP) {	// show the tooltip
		killTimer(TIMERID_CALLTIP);
		::SendMessageW(toolTip_, TTM_UPDATE, 0, 0L);
	} else if(eventID == TIMERID_AUTOSCROLL) {	// auto-scroll
		killTimer(TIMERID_AUTOSCROLL);
		const POINT pt = getCursorPosition();
		const long yScrollDegree = (pt.y - autoScroll_.indicatorPosition.y) / renderer_->linePitch();
//		const long xScrollDegree = (pt.x - autoScroll_.indicatorPosition.x) / presentation_.lineHeight();
//		const long scrollDegree = max(abs(yScrollDegree), abs(xScrollDegree));

		if(yScrollDegree != 0 /*&& abs(yScrollDegree) >= abs(xScrollDegree)*/)
			scroll(0, yScrollDegree > 0 ? +1 : -1, true);
//		else if(xScrollDegree != 0)
//			scroll(xScrollDegree > 0 ? +1 : -1, 0, true);

		if(yScrollDegree != 0) {
			setTimer(TIMERID_AUTOSCROLL, 500 / static_cast<uint>((pow(2, abs(yScrollDegree) / 2))), 0);
			::SetCursor(AutoScrollOriginMark::cursorForScrolling(
				(yScrollDegree > 0) ? AutoScrollOriginMark::CURSOR_DOWNWARD : AutoScrollOriginMark::CURSOR_UPWARD));
		} else {
			setTimer(TIMERID_AUTOSCROLL, 300, 0);
			::SetCursor(AutoScrollOriginMark::cursorForScrolling(AutoScrollOriginMark::CURSOR_NEUTRAL));
		}
	}
}

#ifdef WM_UNICHAR
/// @see WM_UNICHAR
void TextViewer::onUniChar(UINT ch, UINT) {
	if(ch != UNICODE_NOCHAR)
		handleGUICharacterInput(ch);
}
#endif /* WM_UNICHAR */

/// @see Window#onVScroll
void TextViewer::onVScroll(UINT sbCode, UINT, HWND) {
	switch(sbCode) {
	case SB_LINEUP:		// 1 行上
		scroll(0, -1, true); break;
	case SB_LINEDOWN:	// 1 行下
		scroll(0, +1, true); break;
	case SB_PAGEUP:		// 1 ページ上
		scroll(0, -static_cast<int>(numberOfVisibleLines()), true); break;
	case SB_PAGEDOWN:	// 1 ページ下
		scroll(0, +static_cast<int>(numberOfVisibleLines()), true); break;
	case SB_TOP:		// 上端
	case SB_BOTTOM: {	// 下端
		int top, bottom;
		getScrollRange(SB_VERT, top, bottom);
		scrollTo(-1, (sbCode == SB_TOP) ? top : bottom, true); break;
	}
	case SB_THUMBTRACK:	// by drag or wheel
		scrollTo(-1, getScrollTrackPosition(SB_VERT), true); break;	// use 32-bit value
	}
}

/// @see WM_XBUTTONDBLCLK
bool TextViewer::onXButtonDblClk(WORD xButton, WORD keyState, const POINT& pt) {
	if(allowsMouseInput() && mouseInputStrategy_.get() != 0)
		return mouseInputStrategy_->mouseButtonInput((xButton == XBUTTON1) ?
			IMouseInputStrategy::X1_BUTTON : IMouseInputStrategy::X2_BUTTON, IMouseInputStrategy::DOUBLE_CLICKED, pt, keyState);
	return false;
}

/// @see WM_XBUTTONDOWN
bool TextViewer::onXButtonDown(WORD xButton, WORD keyState, const POINT& pt) {
	RESTORE_HIDDEN_CURSOR();
	if(allowsMouseInput() && mouseInputStrategy_.get() != 0)
		return mouseInputStrategy_->mouseButtonInput((xButton == XBUTTON1) ?
			IMouseInputStrategy::X1_BUTTON : IMouseInputStrategy::X2_BUTTON, IMouseInputStrategy::PRESSED, pt, keyState);
	return false;
}

/// @see WM_XBUTTONUP
bool TextViewer::onXButtonUp(WORD xButton, WORD keyState, const POINT& pt) {
	RESTORE_HIDDEN_CURSOR();
	if(allowsMouseInput() && mouseInputStrategy_.get() != 0)
		return mouseInputStrategy_->mouseButtonInput((xButton == XBUTTON1) ?
			IMouseInputStrategy::X1_BUTTON : IMouseInputStrategy::X2_BUTTON, IMouseInputStrategy::RELEASED, pt, keyState);
	return false;
}

/// @see ICaretStateListener#overtypeModeChanged
void TextViewer::overtypeModeChanged(const Caret&) {
}

/// @see Window#preTranslateMessage
LRESULT TextViewer::preTranslateWindowMessage(UINT message, WPARAM wParam, LPARAM lParam, bool& handled) {
	using namespace ascension::texteditor::commands;

	switch(message) {
#ifdef ASCENSION_HANDLE_STANDARD_EDIT_CONTROL_MESSAGES
	case WM_CLEAR:
		if(toBoolean(::GetKeyState(VK_SHIFT) & 0x8000))
			caret().cutSelection(true);
		else
			CharacterDeletionCommand(*this, Direction::FORWARD).execute();
		handled = true;
		return 0L;
	case WM_COPY:
		caret().copySelection(true);
		handled = true;
		return 0L;
	case WM_CUT:
		caret().cutSelection(true);
		handled = true;
		return 0L;
#endif /* ASCENSION_HANDLE_STANDARD_EDIT_CONTROL_MESSAGES */
#ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY
	case WM_GETOBJECT:
		if(lParam == OBJID_CLIENT) {
			ComPtr<::IAccessible> acc;
			if(SUCCEEDED(accessibleObject(*&acc)) && accLib.isAvailable())
				return accLib.lresultFromObject(IID_IAccessible, wParam, acc.get());
		} else if(lParam == OBJID_WINDOW) {
		}
		return 0;
#endif // !ASCENSION_NO_ACTIVE_ACCESSIBILITY
	case WM_GETTEXT: {
		basic_ostringstream<Char> s;
		writeDocumentToStream(s, document(), document().region(), NLF_CR_LF);
		handled = true;
		return reinterpret_cast<LRESULT>(s.str().c_str());
	}
	case WM_GETTEXTLENGTH:
		// ウィンドウ関係だし改行は CRLF でいいか。NLR_RAW_VALUE だと遅いし
		handled = true;
		return document().length(NLF_CR_LF);
	case WM_INPUTLANGCHANGE:
		inputStatusListeners_.notify(&ITextViewerInputStatusListener::textViewerInputLanguageChanged);
		if(hasFocus()) {
			if(texteditor::Session* const session = document().session()) {
				if(texteditor::InputSequenceCheckers* const isc = session->inputSequenceCheckers())
					isc->setKeyboardLayout(::GetKeyboardLayout(::GetCurrentThreadId()));
			}
		}
		break;
//	case WM_NCPAINT:
//		return 0;
#ifdef ASCENSION_HANDLE_STANDARD_EDIT_CONTROL_MESSAGES
	case WM_PASTE:
		PasteCommand(*this, false).execute();
		handled = true;
		return 0L;
#endif /* ASCENSION_HANDLE_STANDARD_EDIT_CONTROL_MESSAGES */
	case WM_SETTEXT:
		EntireDocumentSelectionCreationCommand(*this).execute();
		caret().replaceSelection(String(reinterpret_cast<const wchar_t*>(lParam)), false);
		handled = true;
		return 0L;
#ifdef ASCENSION_HANDLE_STANDARD_EDIT_CONTROL_MESSAGES
	case WM_UNDO:
		UndoCommand(*this, false).execute();
		handled = true;
		return 0L;
#endif /* ASCENSION_HANDLE_STANDARD_EDIT_CONTROL_MESSAGES */
	}

	return BaseControl::preTranslateWindowMessage(message, wParam, lParam, handled);
}

/// Recreates and shows the caret. If the viewer does not have focus, nothing heppen.
void TextViewer::recreateCaret() {
	assertValidAsWindow();

	if(!hasFocus())
		return;
	::DestroyCaret();
	caretShape_.bitmap.reset();

	SIZE solidSize = {0, 0};
	if(imeComposingCharacter_)
		getCurrentCharacterSize(*this, solidSize);
	else if(imeCompositionActivated_)
		solidSize.cx = solidSize.cy = 1;
	else if(caretShape_.shaper.get() != 0)
		caretShape_.shaper->getCaretShape(caretShape_.bitmap, solidSize, caretShape_.orientation);
	else {
		DefaultCaretShaper s;
		CaretShapeUpdater u(*this);
		static_cast<ICaretShapeProvider&>(s).install(u);
		static_cast<ICaretShapeProvider&>(s).getCaretShape(caretShape_.bitmap, solidSize, caretShape_.orientation);
		static_cast<ICaretShapeProvider&>(s).uninstall();
	}

	if(caretShape_.bitmap.get() != 0 && caretShape_.bitmap->getHandle() != 0) {
		createCaret(caretShape_.bitmap->getHandle(), 0, 0);
		BITMAP bmp;
		caretShape_.bitmap->getBitmap(bmp);
		caretShape_.width = bmp.bmWidth;
	} else
		createSolidCaret(caretShape_.width = solidSize.cx, solidSize.cy);
	showCaret();
	updateCaretPosition();
}

/**
 * Redraws the specified line on the view.
 * If the viewer is frozen, redraws after unfrozen.
 * @param line the line to be redrawn
 * @param following true to redraw also the all lines follow to @a line
 */
void TextViewer::redrawLine(length_t line, bool following) {
	redrawLines(line, following ? numeric_limits<length_t>::max() : line);
}

/**
 * Redraws the specified lines on the view.
 * If the viewer is frozen, redraws after unfrozen.
 * @param first the start of the lines to be redrawn
 * @param last the end of the lines to be redrawn. this value is inclusive and this line will be redrawn.
 * if this value is @c std#numeric_limits<length_t>#max(), redraws the @a first line and the below lines
 * @throw std#invalid_argument @a first is gretaer than @a last
 */
void TextViewer::redrawLines(length_t first, length_t last) {
	if(first > last)
		throw invalid_argument("first is greater than last.");
	assertValidAsWindow();

	if(isFrozen()) {	// 凍結中
		freezeInfo_.invalidLines.first =
			(freezeInfo_.invalidLines.first == INVALID_INDEX) ? first : min(first, freezeInfo_.invalidLines.first);
		freezeInfo_.invalidLines.second = 
			(freezeInfo_.invalidLines.second == INVALID_INDEX) ? last : max(last, freezeInfo_.invalidLines.second);
		return;
	}

	const length_t lines = document().numberOfLines();
	if(first >= lines || last < scrollInfo_.firstVisibleLine)
		return;

#ifdef _DEBUG
	if(DIAGNOSE_INHERENT_DRAWING)
		DumpContext() << L"@TextViewer.redrawLines invalidates lines ["
			<< static_cast<ulong>(first) << L".." << static_cast<ulong>(last) << L"]\n";
#endif /* _DEBUG */

	RECT rect;
	getClientRect(rect);

	// 上端
	rect.top = max(mapLineToClientY(first, false), configuration_.topMargin);
	if(rect.top >= rect.bottom)
		return;
	// 下端
	if(last != numeric_limits<length_t>::max()) {
		long bottom = rect.top + static_cast<long>(renderer_->numberOfSublinesOfLine(first) * renderer_->linePitch());
		for(length_t line = first + 1; line <= last; ++line) {
			bottom += static_cast<long>(renderer_->numberOfSublinesOfLine(line) * renderer_->linePitch());
			if(bottom >= rect.bottom)
				break;
		}
		rect.bottom = min(bottom, rect.bottom);
	}
	invalidateRect(&rect, false);
}

/// Redraws the vertical ruler.
void TextViewer::redrawVerticalRuler() {
	RECT r;
	getClientRect(r);
	if(verticalRulerDrawer_->configuration().alignment == ALIGN_LEFT)
		r.right = r.left + verticalRulerDrawer_->width();
	else
		r.left = r.right - verticalRulerDrawer_->width();
	invalidateRect(&r, false);
}

/**
 * Scrolls the viewer.
 * @param dx the number of columns to scroll horizontally
 * @param dy the number of visual lines to scroll vertically
 * @param redraw if redraws after scroll
 */
void TextViewer::scroll(int dx, int dy, bool redraw) {
	assertValidAsWindow();

	// 前処理とスクロールバーの更新
	if(dx != 0) {
		dx = min<int>(dx, scrollInfo_.horizontal.maximum - scrollInfo_.horizontal.pageSize - scrollInfo_.horizontal.position + 1);
		dx = max(dx, -scrollInfo_.horizontal.position);
		if(dx != 0) {
			scrollInfo_.horizontal.position += dx;
			if(!isFrozen())
				setScrollPosition(SB_HORZ, scrollInfo_.horizontal.position, true);
		}
	}
	if(dy != 0) {
		dy = min<int>(dy, scrollInfo_.vertical.maximum - scrollInfo_.vertical.pageSize - scrollInfo_.vertical.position + 1);
		dy = max(dy, -scrollInfo_.vertical.position);
		if(dy != 0) {
			scrollInfo_.vertical.position += dy;
			renderer_->offsetVisualLine(scrollInfo_.firstVisibleLine, scrollInfo_.firstVisibleSubline, dy);
			if(!isFrozen())
				setScrollPosition(SB_VERT, scrollInfo_.vertical.position, true);
		}
	}
	if(dx == 0 && dy == 0)
		return;
	else if(isFrozen()) {
		scrollInfo_.changed = true;
		return;
	}
//	closeCompletionProposalsPopup(*this);
	hideToolTip();

	// 編集領域のスクロール:
	RECT clipRect, clientRect;
	const RECT margins = textAreaMargins();
	getClientRect(clientRect);
	clipRect = clientRect;
	clipRect.top += margins.top;
	clipRect.bottom -= margins.bottom;
	if(static_cast<uint>(abs(dy)) >= numberOfVisibleLines())
		invalidateRect(&clipRect, false);	// スクロール量が 1 ページ以上であれば全体を再描画
	else if(dx == 0)	// 垂直方向のみ
		scrollEx(0, -dy * scrollRate(false) * renderer_->linePitch(), 0, &clipRect, 0, 0, SW_INVALIDATE);
	else {	// 先行マージンと編集領域を分けて処理する
		// 編集領域のスクロール
		clipRect.left += margins.left;
		clipRect.right -= margins.right;
		if(static_cast<uint>(abs(dx)) >= numberOfVisibleColumns())
			invalidateRect(&clipRect, false);	// スクロール量が 1 ページ以上であれば全体を再描画
		else
			scrollEx(-dx * scrollRate(true) * renderer_->averageCharacterWidth(),
				-dy * scrollRate(false) * renderer_->linePitch(), 0, &clipRect, 0, 0, SW_INVALIDATE);
		// 垂直ルーラのスクロール
		if(dy != 0) {
			if(verticalRulerDrawer_->configuration().alignment == ALIGN_LEFT) {
				clipRect.left = clientRect.left;
				clipRect.right = clipRect.left + verticalRulerDrawer_->width();
			} else {
				clipRect.right = clientRect.right;
				clipRect.left = clipRect.right - verticalRulerDrawer_->width();
			}
			scrollEx(0, -dy * scrollRate(false) * renderer_->linePitch(), 0, &clipRect, 0, 0, SW_INVALIDATE);
		}
	}

	// 後処理
	updateCaretPosition();
	if(redraw)
		update();
	viewportListeners_.notify<bool, bool>(&IViewportListener::viewportChanged, dx != 0, dy != 0);
}

/**
 * Scrolls the viewer to the specified position.
 * @param x the visual line of the position. if set -1, does not scroll in this direction
 * @param y the column of the position. if set -1, does not scroll in this direction
 * @param redraw true to redraw the window after scroll
 * @see #scroll
 */
void TextViewer::scrollTo(int x, int y, bool redraw) {
	assertValidAsWindow();

	if(x != -1)
		x = max(min<int>(x, scrollInfo_.horizontal.maximum - scrollInfo_.horizontal.pageSize + 1), 0);
	if(y != -1)
		y = max(min<int>(y, scrollInfo_.vertical.maximum - scrollInfo_.vertical.pageSize + 1), 0);
	const int dx = (x != -1) ? x - scrollInfo_.horizontal.position : 0;
	const int dy = (y != -1) ? y - scrollInfo_.vertical.position : 0;
	if(dx != 0 || dy != 0)
		scroll(dx, dy, redraw);
}

/**
 * Scrolls the viewer to the specified line.
 * @param line the logical line
 * @param redraw true to redraw the window after scroll
 * @throw BadPositionException @a line is outside of the document
 */
void TextViewer::scrollTo(length_t line, bool redraw) {
	// TODO: not implemented.
	assertValidAsWindow();
	if(line >= document().numberOfLines())
		throw BadPositionException();
	scrollInfo_.firstVisibleLine = line;
	scrollInfo_.firstVisibleSubline = 0;
	length_t visualLine;
	if(configuration_.lineWrap.wraps())
		visualLine = line;
	else {
		// TODO: this code can be more faster.
		visualLine = 0;
		for(length_t i = 0; i < line; ++i)
			visualLine += renderer_->numberOfSublinesOfLine(i);
	}
	viewportListeners_.notify<bool, bool>(&IViewportListener::viewportChanged, true, true);
}

/// @see ICaretStateListener#selectionShapeChanged
void TextViewer::selectionShapeChanged(const Caret& self) {
	if(!isFrozen() && !self.isSelectionEmpty())
		redrawLines(self.beginning().lineNumber(), self.end().lineNumber());
}

/**
 * Updates the configurations.
 * @param general the general configurations. @c null to unchange
 * @param verticalRuler the configurations about the vertical ruler. @c null to unchange
 * @throw std#invalid_argument the content of @a verticalRuler is invalid
 */
void TextViewer::setConfiguration(const Configuration* general, const VerticalRulerConfiguration* verticalRuler) {
	if(verticalRuler != 0) {
		if(!verticalRuler->verify())
			throw invalid_argument("The content of `verticalRuler' is invalid.");
		verticalRulerDrawer_->setConfiguration(*verticalRuler);
	}
	if(general != 0) {
		const Alignment oldAlignment = configuration_.alignment;
		configuration_ = *general;
		displaySizeListeners_.notify(&IDisplaySizeListener::viewerDisplaySizeChanged);
		renderer_->invalidate();
		if((oldAlignment == ALIGN_LEFT && configuration_.alignment == ALIGN_RIGHT)
				|| (oldAlignment == ALIGN_RIGHT && configuration_.alignment == ALIGN_LEFT))
			scrollInfo_.horizontal.position = scrollInfo_.horizontal.maximum
				- scrollInfo_.horizontal.pageSize - scrollInfo_.horizontal.position + 1;
		scrollInfo_.resetBars(*this, SB_BOTH, false);
		updateScrollBars();

		if(!isFrozen() && (hasFocus() /*|| getHandle() == Viewer::completionWindow_->getSafeHwnd()*/)) {
			recreateCaret();
			updateCaretPosition();
		}
		const bool rightAlign = configuration_.alignment == ALIGN_RIGHT;
		modifyStyleEx(
			rightAlign ? WS_EX_RIGHTSCROLLBAR : WS_EX_LEFTSCROLLBAR,
			rightAlign ? WS_EX_LEFTSCROLLBAR : WS_EX_RIGHTSCROLLBAR);
	}
	invalidateRect(0, false);
}

/**
 * Sets the new content assistant.
 * @param newContentAssistant the content assistant to set. the ownership will be transferred to the callee.
 */
void TextViewer::setContentAssistant(auto_ptr<contentassist::IContentAssistant> newContentAssistant) /*throw()*/ {
	if(contentAssistant_.get() != 0)
		contentAssistant_->uninstall();	// $friendly-access
	(contentAssistant_ = newContentAssistant)->install(*this);	// $friendly-access
}

/**
 * Sets the mouse input strategy. An instance of @c TextViewer has the default strategy implemented
 * by @c DefaultMouseInputStrategy class as the construction.
 * @param newStrategy the new strategy or @c null
 * @param delegateOwnership set true to transfer the ownership into the callee
 * @throw IllegalStateException the window is not created yet
 */
void TextViewer::setMouseInputStrategy(IMouseInputStrategy* newStrategy, bool delegateOwnership) {
	if(!isWindow())
		throw IllegalStateException("The window is not created yet.");
	if(mouseInputStrategy_.get() != 0)
		mouseInputStrategy_->uninstall();
	if(newStrategy != 0)
		mouseInputStrategy_.reset(newStrategy, delegateOwnership);
	else
		mouseInputStrategy_.reset(new DefaultMouseInputStrategy(true, true), true);	// TODO: the two parameters don't have rationales.
	mouseInputStrategy_->install(*this);
}

/**
 * Shows the tool tip at the cursor position.
 * @param text the text to be shown. CRLF represents a line break. this can not contain any NUL character
 * @param timeToWait the time to wait in milliseconds. -1 to use the system default value
 * @param timeRemainsVisible the time remains visible in milliseconds. -1 to use the system default value
 */
void TextViewer::showToolTip(const String& text, ulong timeToWait /* = -1 */, ulong timeRemainsVisible /* = -1 */) {
	assertValidAsWindow();

	delete[] tipText_;
	tipText_ = new wchar_t[text.length() + 1];
	hideToolTip();
	if(timeToWait == -1)
		timeToWait = ::GetDoubleClickTime();
	wcscpy(tipText_, text.c_str());
	setTimer(TIMERID_CALLTIP, timeToWait, 0);
}

#ifndef ASCENSION_NO_TEXT_SERVICES_FRAMEWORK
HRESULT TextViewer::startTextServices() {
	assertValid();
	ComPtr<ITfThreadMgr> threadManager;
	HRESULT hr = threadManager.createInstance(CLSID_TF_ThreadMgr, 0, CLSCTX_INPROC_SERVER);
	if(FAILED(hr))
		return hr;
	ComPtr<ITfDocumentMgr> documentManager;
	hr = threadManager->CreateDocumentMgr(&documentManager);
	if(FAILED(hr))
		return hr;
	ComPtr<ITfContext> context;
	documentManager->CreateContext(...);
	...
}
#endif // !ASCENSION_NO_TEXT_SERVICES_FRAMEWORK

/**
 * Returns the margins of text area.
 * @return the rectangle whose members correspond to each margins
 */
RECT TextViewer::textAreaMargins() const /*throw()*/ {
	RECT margins = {0, 0, 0, 0};
	((verticalRulerDrawer_->configuration().alignment == ALIGN_LEFT) ? margins.left : margins.right) += verticalRulerDrawer_->width();
	Alignment alignment = configuration_.alignment;
	if(alignment != ALIGN_LEFT && alignment != ALIGN_RIGHT)
		alignment = (configuration_.orientation == LEFT_TO_RIGHT) ? ALIGN_LEFT : ALIGN_RIGHT;
	if(alignment == ALIGN_LEFT)
		margins.left += configuration_.leadingMargin;
	else if(alignment == ALIGN_RIGHT)
		margins.right += configuration_.leadingMargin;
	margins.top += configuration_.topMargin;
	return margins;
}

/**
 * Revokes the frozen state of the viewer.
 * @param forAllClones true to revoke also all clones of the viewer
 * @see #freeze, #isFrozen
 */
void TextViewer::unfreeze(bool forAllClones /* = true */) {
	assertValidAsWindow();
	if(!forAllClones) {
		if(freezeInfo_.count > 0 && --freezeInfo_.count == 0)
			internalUnfreeze();
	} else {
		for(Presentation::TextViewerIterator i(presentation_.firstTextViewer()), e(presentation_.lastTextViewer()); i != e; ++i) {
			if((*i)->freezeInfo_.count > 0 && --(*i)->freezeInfo_.count == 0)
				(*i)->internalUnfreeze();
		}
	}
}

/// Moves the caret to valid position with current position, scroll context, and the fonts.
void TextViewer::updateCaretPosition() {
	if(!hasFocus() || isFrozen())
		return;

	POINT pt = clientXYForCharacter(caret(), false, LineLayout::LEADING);
	const RECT margins = textAreaMargins();
	RECT textArea;
	getClientRect(textArea);
	textArea.left += margins.left; textArea.top += margins.top;
	textArea.right -= margins.right - 1; textArea.bottom -= margins.bottom;

	if(!toBoolean(::PtInRect(&textArea, pt)))	// "hide" the caret
		pt.y = -renderer_->linePitch();
	else if(caretShape_.orientation == RIGHT_TO_LEFT
			|| renderer_->lineLayout(caret().lineNumber()).bidiEmbeddingLevel(caret().columnNumber()) % 2 == 1)
		pt.x -= caretShape_.width;
	setCaretPosition(pt);
	updateIMECompositionWindowPosition();
}

/// Moves the IME form to valid position.
void TextViewer::updateIMECompositionWindowPosition() {
	assertValidAsWindow();
	if(!imeCompositionActivated_)
		return;
	else if(HIMC imc = ::ImmGetContext(getHandle())) {
		// composition window placement
		COMPOSITIONFORM cf;
		getClientRect(cf.rcArea);
		RECT margins = textAreaMargins();
		cf.rcArea.left += margins.left;
		cf.rcArea.top += margins.top;
		cf.rcArea.right -= margins.right;
		cf.rcArea.bottom -= margins.bottom;
		cf.dwStyle = CFS_POINT;
		cf.ptCurrentPos = clientXYForCharacter(caret().beginning(), false, LineLayout::LEADING);
		if(cf.ptCurrentPos.y == 32767 || cf.ptCurrentPos.y == -32768)
			cf.ptCurrentPos.y = (cf.ptCurrentPos.y == -32768) ? cf.rcArea.top : cf.rcArea.bottom;
		else
			cf.ptCurrentPos.y = max(cf.ptCurrentPos.y, cf.rcArea.top);
		::ImmSetCompositionWindow(imc, &cf);
		cf.dwStyle = CFS_RECT;
		::ImmSetCompositionWindow(imc, &cf);

		// composition font
		LOGFONTW font;
		::GetObjectW(renderer_->font(), sizeof(LOGFONTW), &font);
		::ImmSetCompositionFontW(imc, &font);	// this may be ineffective for IME settings
		
		::ImmReleaseContext(getHandle(), imc);
	}
}

/// Updates the scroll information.
void TextViewer::updateScrollBars() {
	assertValidAsWindow();
	if(renderer_.get() == 0)
		return;

#define GET_SCROLL_MINIMUM(s)	(s.maximum/* * s.rate*/ - s.pageSize + 1)

	// about horizontal scroll bar
	bool wasNeededScrollbar = GET_SCROLL_MINIMUM(scrollInfo_.horizontal) > 0;
	// scroll to leftmost/rightmost before the scroll bar vanishes
	long minimum = GET_SCROLL_MINIMUM(scrollInfo_.horizontal);
	if(wasNeededScrollbar && minimum <= 0) {
		scrollInfo_.horizontal.position = 0;
		if(!isFrozen()) {
			invalidateRect(0, false);
			updateCaretPosition();
		}
	} else if(scrollInfo_.horizontal.position > minimum)
		scrollTo(minimum, -1, true);
	assert(GET_SCROLL_MINIMUM(scrollInfo_.horizontal) > 0 || scrollInfo_.horizontal.position == 0);
	if(!isFrozen()) {
		MANAH_AUTO_STRUCT_SIZE(SCROLLINFO, scroll);
		scroll.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
		scroll.nMax = configuration_.lineWrap.wrapsAtWindowEdge() ? 0 : scrollInfo_.horizontal.maximum;
		scroll.nPage = scrollInfo_.horizontal.pageSize;
		scroll.nPos = scrollInfo_.horizontal.position;
		setScrollInformation(SB_HORZ, scroll, true);
	}

	// about vertical scroll bar
	wasNeededScrollbar = GET_SCROLL_MINIMUM(scrollInfo_.vertical) > 0;
	minimum = GET_SCROLL_MINIMUM(scrollInfo_.vertical);
	// validate scroll position
	if(minimum <= 0) {
		scrollInfo_.vertical.position = 0;
		scrollInfo_.firstVisibleLine = scrollInfo_.firstVisibleSubline = 0;
		if(!isFrozen()) {
			invalidateRect(0, false);
			updateCaretPosition();
		}
	} else if(scrollInfo_.vertical.position > minimum)
		scrollTo(-1, minimum, true);
	assert(GET_SCROLL_MINIMUM(scrollInfo_.vertical) > 0 || scrollInfo_.vertical.position == 0);
	if(!isFrozen()) {
		MANAH_AUTO_STRUCT_SIZE(SCROLLINFO, scroll);
		scroll.fMask = SIF_DISABLENOSCROLL | SIF_PAGE | SIF_POS | SIF_RANGE;
		scroll.nMax = scrollInfo_.vertical.maximum;
		scroll.nPage = scrollInfo_.vertical.pageSize;
		scroll.nPos = scrollInfo_.vertical.position;
		setScrollInformation(SB_VERT, scroll, true);
	}

	scrollInfo_.changed = isFrozen();

#undef GET_SCROLL_MINIMUM
}

/// @see IVisualLinesListener#visualLinesDeleted
void TextViewer::visualLinesDeleted(length_t first, length_t last, length_t sublines, bool longestLineChanged) /*throw()*/ {
	scrollInfo_.changed = true;
	if(last < scrollInfo_.firstVisibleLine) {	// 可視領域より前が削除された
		scrollInfo_.firstVisibleLine -= last - first;
		scrollInfo_.vertical.position -= static_cast<int>(sublines);
		scrollInfo_.vertical.maximum -= static_cast<int>(sublines);
		redrawVerticalRuler();
	} else if(first > scrollInfo_.firstVisibleLine
			|| (first == scrollInfo_.firstVisibleLine && scrollInfo_.firstVisibleSubline == 0)) {	// 可視先頭行以降が削除された
		scrollInfo_.vertical.maximum -= static_cast<int>(sublines);
		redrawLine(first, true);
	} else {	// 可視先頭行を含む範囲が削除された
		scrollInfo_.firstVisibleLine = first;
		scrollInfo_.updateVertical(*this);
		redrawLine(first, true);
	}
	if(longestLineChanged)
		scrollInfo_.resetBars(*this, SB_HORZ, false);
}

/// @see IVisualLinesListener#visualLinesInserted
void TextViewer::visualLinesInserted(length_t first, length_t last) /*throw()*/ {
	scrollInfo_.changed = true;
	if(last < scrollInfo_.firstVisibleLine) {	// 可視領域より前に挿入された
		scrollInfo_.firstVisibleLine += last - first;
		scrollInfo_.vertical.position += static_cast<int>(last - first);
		scrollInfo_.vertical.maximum += static_cast<int>(last - first);
		redrawVerticalRuler();
	} else if(first > scrollInfo_.firstVisibleLine
			|| (first == scrollInfo_.firstVisibleLine && scrollInfo_.firstVisibleSubline == 0)) {	// 可視先頭行以降に挿入された
		scrollInfo_.vertical.maximum += static_cast<int>(last - first);
		redrawLine(first, true);
	} else {	// 可視先頭行の前後に挿入された
		scrollInfo_.firstVisibleLine += last - first;
		scrollInfo_.updateVertical(*this);
		redrawLine(first, true);
	}
}

/// @see IVisualLinesListener#visualLinesModified
void TextViewer::visualLinesModified(length_t first, length_t last,
		signed_length_t sublinesDifference, bool documentChanged, bool longestLineChanged) /*throw()*/ {
	if(sublinesDifference == 0)	// 表示上の行数が変化しなかった
		redrawLines(first, last - 1);
	else {
		scrollInfo_.changed = true;
		if(last < scrollInfo_.firstVisibleLine) {	// 可視領域より前が変更された
			scrollInfo_.vertical.position += sublinesDifference;
			scrollInfo_.vertical.maximum += sublinesDifference;
			redrawVerticalRuler();
		} else if(first > scrollInfo_.firstVisibleLine
				|| (first == scrollInfo_.firstVisibleLine && scrollInfo_.firstVisibleSubline == 0)) {	// 可視先頭行以降が変更された
			scrollInfo_.vertical.maximum += sublinesDifference;
			redrawLine(first, true);
		} else {	// 可視先頭行を含む範囲が変更された
			scrollInfo_.updateVertical(*this);
			redrawLine(first, true);
		}
	}
	if(longestLineChanged) {
		scrollInfo_.resetBars(*this, SB_HORZ, false);
		scrollInfo_.changed = true;
	}
	if(!documentChanged && scrollInfo_.changed)
		updateScrollBars();
}


// TextViewerAccessibleProxy ////////////////////////////////////////////////

#ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY
#define VERIFY_AVAILABILITY()	\
	if(!available_) return RPC_E_DISCONNECTED

/**
 * Constructor.
 * @param view the viewer
 */
TextViewerAccessibleProxy::TextViewerAccessibleProxy(TextViewer& view) /*throw()*/ : view_(view), available_(true) {
	assert(accLib.isAvailable());
	accLib.createStdAccessibleObject(view.getHandle(), OBJID_CLIENT, IID_IAccessible, &defaultServer_);
}

/// @see IAccessible#accDoDefaultAction
STDMETHODIMP TextViewerAccessibleProxy::accDoDefaultAction(VARIANT) {
	VERIFY_AVAILABILITY();
	return DISP_E_MEMBERNOTFOUND;
}

/// @see IAccessible#accHitTest
STDMETHODIMP TextViewerAccessibleProxy::accHitTest(long xLeft, long yTop, VARIANT* pvarChild) {
	VERIFY_AVAILABILITY();
	// ウィンドウが矩形であることを前提としている
	MANAH_VERIFY_POINTER(pvarChild);
	POINT pt = {xLeft, yTop};
	RECT rect;
	view_.getClientRect(rect);
	view_.clientToScreen(rect);
	if(toBoolean(::PtInRect(&rect, pt))) {
		pvarChild->vt = VT_I4;
		pvarChild->lVal = CHILDID_SELF;
		return S_OK;
	} else {
		pvarChild->vt = VT_EMPTY;
		return S_FALSE;
	}
}

/// @see IAccessible#accLocation
STDMETHODIMP TextViewerAccessibleProxy::accLocation(long* pxLeft, long* pyTop, long* pcxWidth, long* pcyHeight, VARIANT varChild) {
	VERIFY_AVAILABILITY();
	MANAH_VERIFY_POINTER(pxLeft);
	MANAH_VERIFY_POINTER(pyTop);
	MANAH_VERIFY_POINTER(pcxWidth);
	MANAH_VERIFY_POINTER(pcyHeight);
	if(varChild.vt != VT_I4 || varChild.lVal != CHILDID_SELF)
		return E_INVALIDARG;
	RECT rect;
	view_.getClientRect(rect);
	view_.clientToScreen(rect);
	*pxLeft = rect.left;
	*pyTop = rect.top;
	*pcxWidth = rect.right - rect.left;
	*pcyHeight = rect.bottom - rect.top;
	return S_OK;
}

/// @see IAccessible#accNavigate
STDMETHODIMP TextViewerAccessibleProxy::accNavigate(long navDir, VARIANT varStart, VARIANT* pvarEndUpAt) {
	VERIFY_AVAILABILITY();
	return defaultServer_->accNavigate(navDir, varStart, pvarEndUpAt);
}

/// @see IAccessible#accSelect
STDMETHODIMP TextViewerAccessibleProxy::accSelect(long flagsSelect, VARIANT varChild) {
	VERIFY_AVAILABILITY();
	return (varChild.vt == VT_I4 && varChild.lVal == CHILDID_SELF) ?
		defaultServer_->accSelect(flagsSelect, varChild) : E_INVALIDARG;
}

/// @see IOleWindow#ContextSensitiveHelp
STDMETHODIMP TextViewerAccessibleProxy::ContextSensitiveHelp(BOOL fEnterMode) {
	return S_OK;	// not supported
}

/// Informs that the viewer is inavailable to the proxy.
void TextViewerAccessibleProxy::dispose() {
	if(!available_)
		throw IllegalStateException("This proxy is already disposed.");
	available_ = false;
}

/// @see Document#IListener#documentAboutToBeChanged
void TextViewerAccessibleProxy::documentAboutToBeChanged(const Document&, const DocumentChange&) {
	// do nothing
}

/// @see Document#IListener#documentChanged
void TextViewerAccessibleProxy::documentChanged(const Document&, const DocumentChange&) {
	assert(accLib.isAvailable());
	accLib.notifyWinEvent(EVENT_OBJECT_VALUECHANGE, view_.getHandle(), OBJID_CLIENT, CHILDID_SELF);
}

/// @see IAccessible#get_accChild
STDMETHODIMP TextViewerAccessibleProxy::get_accChild(VARIANT, IDispatch** ppdispChild) {
	VERIFY_AVAILABILITY();
	MANAH_VERIFY_POINTER(ppdispChild);
	*ppdispChild = 0;
	return S_OK;
}

/// @see IAccessible#get_accChildCount
STDMETHODIMP TextViewerAccessibleProxy::get_accChildCount(long* pcountChildren) {
	VERIFY_AVAILABILITY();
	MANAH_VERIFY_POINTER(pcountChildren);
	*pcountChildren = 0;
	return S_OK;
}

/// @see IAccessible#get_accDefaultAction
STDMETHODIMP TextViewerAccessibleProxy::get_accDefaultAction(VARIANT, BSTR*) {
	VERIFY_AVAILABILITY();
	return DISP_E_MEMBERNOTFOUND;
}

/// @see IAccessible#get_accDescription
STDMETHODIMP TextViewerAccessibleProxy::get_accDescription(VARIANT, BSTR*) {
	VERIFY_AVAILABILITY();
	return DISP_E_MEMBERNOTFOUND;
}

/// @see IAccessible#get_accFocus
STDMETHODIMP TextViewerAccessibleProxy::get_accFocus(VARIANT* pvarChild) {
	VERIFY_AVAILABILITY();
	MANAH_VERIFY_POINTER(pvarChild);
	pvarChild->vt = VT_I4;
	pvarChild->lVal = CHILDID_SELF;
	return S_OK;
}

/// @see IAccessible#get_accHelp
STDMETHODIMP TextViewerAccessibleProxy::get_accHelp(VARIANT, BSTR*) {
	VERIFY_AVAILABILITY();
	return DISP_E_MEMBERNOTFOUND;
}

/// @see IAccessible#get_accHelpTopic
STDMETHODIMP TextViewerAccessibleProxy::get_accHelpTopic(BSTR*, VARIANT, long*) {
	VERIFY_AVAILABILITY();
	return DISP_E_MEMBERNOTFOUND;
}

/// @see IAccessible#get_accKeyboardShortcut
STDMETHODIMP TextViewerAccessibleProxy::get_accKeyboardShortcut(VARIANT varChild, BSTR* pszKeyboardShortcut) {
	VERIFY_AVAILABILITY();
	MANAH_VERIFY_POINTER(pszKeyboardShortcut);
	*pszKeyboardShortcut = 0;
	if(varChild.vt != VT_I4 || varChild.lVal != CHILDID_SELF)
		return E_INVALIDARG;
	return S_FALSE;
}

/// @see IAccessible#get_accName
STDMETHODIMP TextViewerAccessibleProxy::get_accName(VARIANT varChild, BSTR* pszName) {
	VERIFY_AVAILABILITY();
	MANAH_VERIFY_POINTER(pszName);
	*pszName = 0;
	if(varChild.vt != VT_I4 || varChild.lVal != CHILDID_SELF)
		return E_INVALIDARG;
	return S_FALSE;
}

/// @see IAccessible#get_accParent
STDMETHODIMP TextViewerAccessibleProxy::get_accParent(IDispatch** ppdispParent) {
	VERIFY_AVAILABILITY();
	if(accLib.isAvailable())
		return accLib.accessibleObjectFromWindow(view_.getHandle(), OBJID_WINDOW, IID_IAccessible, reinterpret_cast<void**>(ppdispParent));
	return defaultServer_->get_accParent(ppdispParent);
}

/// @see IAccessible#get_accRole
STDMETHODIMP TextViewerAccessibleProxy::get_accRole(VARIANT varChild, VARIANT* pvarRole) {
	VERIFY_AVAILABILITY();
	MANAH_VERIFY_POINTER(pvarRole);
	if(varChild.vt != VT_I4 || varChild.lVal != CHILDID_SELF)
		return E_INVALIDARG;
	pvarRole->vt = VT_I4;
	pvarRole->lVal = ROLE_SYSTEM_TEXT;
	return S_OK;
}

/// @see IAccessible#get_accSelection
STDMETHODIMP TextViewerAccessibleProxy::get_accSelection(VARIANT* pvarChildren) {
	VERIFY_AVAILABILITY();
	MANAH_VERIFY_POINTER(pvarChildren);
	pvarChildren->vt = VT_EMPTY;
	return S_FALSE;
}

/// @see IAccessible#get_accState
STDMETHODIMP TextViewerAccessibleProxy::get_accState(VARIANT varChild, VARIANT* pvarState) {
	VERIFY_AVAILABILITY();
	if(varChild.vt != VT_I4 || varChild.lVal != CHILDID_SELF)
		return E_INVALIDARG;
	pvarState->vt = VT_I4;
	pvarState->lVal = 0;	// STATE_SYSTEM_NORMAL;
	if(!view_.isVisible())
		pvarState->lVal |= STATE_SYSTEM_INVISIBLE;
	if(view_.getTop().getHandle() == ::GetActiveWindow())
		pvarState->lVal |= STATE_SYSTEM_FOCUSABLE;
	if(view_.hasFocus())
		pvarState->lVal |= STATE_SYSTEM_FOCUSED;
	if(view_.document().isReadOnly())
		pvarState->lVal |= STATE_SYSTEM_READONLY;
	return S_OK;
}

/// @see IAccessible#get_accValue
STDMETHODIMP TextViewerAccessibleProxy::get_accValue(VARIANT varChild, BSTR* pszValue) {
	VERIFY_AVAILABILITY();
	MANAH_VERIFY_POINTER(pszValue);
	if(varChild.vt != VT_I4 || varChild.lVal != CHILDID_SELF)
		return E_INVALIDARG;
	basic_ostringstream<Char> s;
	writeDocumentToStream(s, view_.document(), view_.document().region());
	*pszValue = ::SysAllocString(s.str().c_str());
	return (*pszValue != 0) ? S_OK : E_OUTOFMEMORY;
}

/// @see IOleWindow#GetWindow
STDMETHODIMP TextViewerAccessibleProxy::GetWindow(HWND* phwnd) {
	VERIFY_AVAILABILITY();
	MANAH_VERIFY_POINTER(phwnd);
	*phwnd = view_.getHandle();
	return S_OK;
}

/// @see IAccessible#put_accName
STDMETHODIMP TextViewerAccessibleProxy::put_accName(VARIANT, BSTR) {
	VERIFY_AVAILABILITY();
	return DISP_E_MEMBERNOTFOUND;
}

/// @see IAccessible#put_accValue
STDMETHODIMP TextViewerAccessibleProxy::put_accValue(VARIANT varChild, BSTR szValue) {
	VERIFY_AVAILABILITY();
	if(varChild.vt != VT_I4 || varChild.lVal != CHILDID_SELF)
		return E_INVALIDARG;
	else if(view_.document().isReadOnly())
		return E_ACCESSDENIED;
	view_.caret().replaceSelection(safeBSTR(szValue));
	return S_OK;
}

#undef VERIFY_AVAILABILITY
#endif // !ASCENSION_NO_ACTIVE_ACCESSIBILITY


// TextViewer.Renderer //////////////////////////////////////////////////////

/// Constructor.
TextViewer::Renderer::Renderer(TextViewer& viewer) : TextRenderer(viewer.presentation(), true), viewer_(viewer) {
#if 0
	// for test
	setSpecialCharacterRenderer(new DefaultSpecialCharacterRenderer, true);
#endif
}

/// Copy-constructor with a parameter.
TextViewer::Renderer::Renderer(const Renderer& rhs, TextViewer& viewer) : TextRenderer(rhs), viewer_(viewer) {
}

/// @see layout#FontSelector#getDeviceContext
auto_ptr<DC> TextViewer::Renderer::getDeviceContext() const {
	return auto_ptr<DC>(viewer_.isWindow() ? new ClientDC(const_cast<TextViewer&>(viewer_).getDC()) : new ScreenDC());
}

/// @see layout#ILayoutInformationProvider#getLayoutSettings
const LayoutSettings& TextViewer::Renderer::getLayoutSettings() const /*throw()*/ {
	return viewer_.configuration();
}

/// @see layout#ILayoutInformationProvider#getWidth
int TextViewer::Renderer::getWidth() const /*throw()*/ {
	const LineWrapConfiguration& lwc = viewer_.configuration().lineWrap;
	if(!lwc.wraps()) {
		MANAH_AUTO_STRUCT_SIZE(SCROLLINFO, si);
		si.fMask = SIF_RANGE;
		viewer_.getScrollInformation(SB_HORZ, si);
		return (si.nMax + 1) * viewer_.textRenderer().averageCharacterWidth();
	} else if(lwc.wrapsAtWindowEdge()) {
		RECT rc, margins = viewer_.textAreaMargins();
		viewer_.getClientRect(rc);
		return rc.right - rc.left - margins.left - margins.right;	// $friendly-access
	} else
		return lwc.width;
}

/// Rewraps the visual lines at the window's edge.
void TextViewer::Renderer::rewrapAtWindowEdge() {
	if(viewer_.configuration().lineWrap.wrapsAtWindowEdge()) {
		Rect clientRect;
		viewer_.getClientRect(clientRect);
		RECT margins = viewer_.textAreaMargins();
		const int newWidth = clientRect.getWidth() - margins.left - margins.right;
		for(Iterator i(firstCachedLine()), e(lastCachedLine()); i != e; ) {
			const LineLayout& layout = **i;
			++i;	// invalidate() may break iterator
			if(layout.numberOfSublines() != 1
					|| viewer_.configuration().justifiesLines || layout.longestSublineWidth() > newWidth)
//				layout.rewrap();
				invalidate(layout.lineNumber(), layout.lineNumber() + 1);
		}
	}
}


// TextViewer.AutoScrollOriginMark //////////////////////////////////////////

const long TextViewer::AutoScrollOriginMark::WINDOW_WIDTH = 28;

/// Default constructor.
TextViewer::AutoScrollOriginMark::AutoScrollOriginMark() /*throw()*/ {
}

/**
 * Creates the window.
 * @param view the viewer
 * @return succeeded or not
 * @see Window#create
 */
bool TextViewer::AutoScrollOriginMark::create(const TextViewer& view) {
	RECT rc = {0, 0, WINDOW_WIDTH + 1, WINDOW_WIDTH + 1};

	if(!ui::CustomControl<AutoScrollOriginMark>::create(view.getHandle(),
			rc, 0, WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP, WS_EX_TOOLWINDOW))
		return false;
	modifyStyleEx(0, WS_EX_LAYERED);	// いきなり CreateWindowEx(WS_EX_LAYERED) とすると NT 4.0 で失敗する

	HRGN rgn = ::CreateEllipticRgn(0, 0, WINDOW_WIDTH + 1, WINDOW_WIDTH + 1);
	setRegion(rgn, false);
	::DeleteObject(rgn);
	setLayeredAttributes(::GetSysColor(COLOR_WINDOW), 0, LWA_COLORKEY);

	return true;
}

/**
 * Returns the cursor should be shown when the auto-scroll is active.
 * @param type the type of the cursor to obtain
 * @return the cursor. do not destroy the returned value
 * @throw UnknownValueException @a type is unknown
 */
HCURSOR TextViewer::AutoScrollOriginMark::cursorForScrolling(CursorType type) {
	static Handle<HCURSOR, ::DestroyCursor> instances[3];
	if(type >= MANAH_COUNTOF(instances))
		throw UnknownValueException("type");
	if(instances[type].getHandle() == 0) {
		static const byte AND_LINE_3_TO_11[] = {
			0xFF, 0xFE, 0x7F, 0xFF,
			0xFF, 0xFC, 0x3F, 0xFF,
			0xFF, 0xF8, 0x1F, 0xFF,
			0xFF, 0xF0, 0x0F, 0xFF,
			0xFF, 0xE0, 0x07, 0xFF,
			0xFF, 0xC0, 0x03, 0xFF,
			0xFF, 0x80, 0x01, 0xFF,
			0xFF, 0x00, 0x00, 0xFF,
			0xFF, 0x80, 0x01, 0xFF
		};
		static const byte XOR_LINE_3_TO_11[] = {
			0x00, 0x01, 0x80, 0x00,
			0x00, 0x02, 0x40, 0x00,
			0x00, 0x04, 0x20, 0x00,
			0x00, 0x08, 0x10, 0x00,
			0x00, 0x10, 0x08, 0x00,
			0x00, 0x20, 0x04, 0x00,
			0x00, 0x40, 0x02, 0x00,
			0x00, 0x80, 0x01, 0x00,
			0x00, 0x7F, 0xFE, 0x00
		};
		static const byte AND_LINE_13_TO_18[] = {
			0xFF, 0xFE, 0x7F, 0xFF,
			0xFF, 0xFC, 0x3F, 0xFF,
			0xFF, 0xF8, 0x1F, 0xFF,
			0xFF, 0xF8, 0x1F, 0xFF,
			0xFF, 0xFC, 0x3F, 0xFF,
			0xFF, 0xFE, 0x7F, 0xFF,
		};
		static const byte XOR_LINE_13_TO_18[] = {
			0x00, 0x01, 0x80, 0x00,
			0x00, 0x02, 0x40, 0x00,
			0x00, 0x04, 0x20, 0x00,
			0x00, 0x04, 0x20, 0x00,
			0x00, 0x02, 0x40, 0x00,
			0x00, 0x01, 0x80, 0x00
		};
		static const byte AND_LINE_20_TO_28[] = {
			0xFF, 0x80, 0x01, 0xFF,
			0xFF, 0x00, 0x00, 0xFF,
			0xFF, 0x80, 0x01, 0xFF,
			0xFF, 0xC0, 0x03, 0xFF,
			0xFF, 0xE0, 0x07, 0xFF,
			0xFF, 0xF0, 0x0F, 0xFF,
			0xFF, 0xF8, 0x1F, 0xFF,
			0xFF, 0xFC, 0x3F, 0xFF,
			0xFF, 0xFE, 0x7F, 0xFF
		};
		static const byte XOR_LINE_20_TO_28[] = {
			0x00, 0x7F, 0xFE, 0x00,
			0x00, 0x80, 0x01, 0x00,
			0x00, 0x40, 0x02, 0x00,
			0x00, 0x20, 0x04, 0x00,
			0x00, 0x10, 0x08, 0x00,
			0x00, 0x08, 0x10, 0x00,
			0x00, 0x04, 0x20, 0x00,
			0x00, 0x02, 0x40, 0x00,
			0x00, 0x01, 0x80, 0x00
		};
		byte andBits[4 * 32], xorBits[4 * 32];
		// fill canvases
		memset(andBits, 0xFF, 4 * 32);
		memset(xorBits, 0x00, 4 * 32);
		// draw lines
		if(type == CURSOR_NEUTRAL || type == CURSOR_UPWARD) {
			memcpy(andBits + 4 * 3, AND_LINE_3_TO_11, sizeof(AND_LINE_3_TO_11));
			memcpy(xorBits + 4 * 3, XOR_LINE_3_TO_11, sizeof(XOR_LINE_3_TO_11));
		}
		memcpy(andBits + 4 * 13, AND_LINE_13_TO_18, sizeof(AND_LINE_13_TO_18));
		memcpy(xorBits + 4 * 13, XOR_LINE_13_TO_18, sizeof(XOR_LINE_13_TO_18));
		if(type == CURSOR_NEUTRAL || type == CURSOR_DOWNWARD) {
			memcpy(andBits + 4 * 20, AND_LINE_20_TO_28, sizeof(AND_LINE_20_TO_28));
			memcpy(xorBits + 4 * 20, XOR_LINE_20_TO_28, sizeof(XOR_LINE_20_TO_28));
		}
		instances[type].reset(::CreateCursor(::GetModuleHandleW(0), 16, 16, 32, 32, andBits, xorBits));
	}
	return instances[type].getHandle();
}

/// @see Window#onPaint
void TextViewer::AutoScrollOriginMark::onPaint(PaintDC& dc) {
	const COLORREF color = ::GetSysColor(COLOR_APPWORKSPACE);
	HPEN pen = ::CreatePen(PS_SOLID, 1, color), oldPen = dc.selectObject(pen);
	HBRUSH brush = ::CreateSolidBrush(color), oldBrush = dc.selectObject(brush);
	POINT points[4];

	points[0].x = 13; points[0].y = 3;
	points[1].x = 7; points[1].y = 9;
	points[2].x = 20; points[2].y = 9;
	points[3].x = 14; points[3].y = 3;
	dc.polygon(points, 4);

	points[0].x = 13; points[0].y = 24;
	points[1].x = 7; points[1].y = 18;
	points[2].x = 20; points[2].y = 18;
	points[3].x = 14; points[3].y = 24;
	dc.polygon(points, 4);

	dc.moveTo(13, 12); dc.lineTo(15, 12);
	dc.moveTo(12, 13); dc.lineTo(16, 13);
	dc.moveTo(12, 14); dc.lineTo(16, 14);
	dc.moveTo(13, 15); dc.lineTo(15, 15);

	dc.selectObject(oldPen);
	dc.selectObject(oldBrush);
	::DeleteObject(pen);
	::DeleteObject(brush);
}


// TextViewer.VerticalRulerDrawer ///////////////////////////////////////////

// some methods are defined in layout.cpp

/**
 * Constructor.
 * @param viewer the viewer
 * @param enableDoubleBuffering set true to use double-buffering for non-flicker drawing
 */
TextViewer::VerticalRulerDrawer::VerticalRulerDrawer(TextViewer& viewer, bool enableDoubleBuffering)
		: viewer_(viewer), width_(0), lineNumberDigitsCache_(0), enablesDoubleBuffering_(enableDoubleBuffering) {
	recalculateWidth();
}

/// Returns the maximum number of digits of line numbers.
uchar TextViewer::VerticalRulerDrawer::getLineNumberMaxDigits() const /*throw()*/ {
	uint n = 1;
	length_t lines = viewer_.document().numberOfLines() + configuration_.lineNumbers.startValue - 1;
	while(lines >= 10) {
		lines /= 10;
		++n;
	}
	return static_cast<uchar>(n);	// hmm...
}

void TextViewer::VerticalRulerDrawer::setConfiguration(const VerticalRulerConfiguration& configuration) {
	if(!configuration.lineNumbers.verify())
		throw invalid_argument("Any member of the specified VerticalRulerConfiguration is invalid.");
	configuration_ = configuration;
	update();
}

void TextViewer::VerticalRulerDrawer::update() /*throw()*/ {
	lineNumberDigitsCache_ = 0;
	recalculateWidth();
	updateGDIObjects();
	if(enablesDoubleBuffering_ && memoryBitmap_.getHandle() != 0)
		memoryBitmap_.reset();
}

#undef RESTORE_HIDDEN_CURSOR


// TextViewer.ScrollInfo ////////////////////////////////////////////////////

void TextViewer::ScrollInfo::resetBars(const TextViewer& viewer, int bars, bool pageSizeChanged) /*throw()*/ {
	// 水平方向
	if(bars == SB_HORZ || bars == SB_BOTH) {
		// テキストが左揃えでない場合は、スクロールボックスの位置を補正する必要がある
		// (ウィンドウが常に LTR である仕様のため)
		const Alignment alignment = viewer.configuration().alignment;
		const int dx = viewer.textRenderer().averageCharacterWidth();
		assert(dx > 0);
		const ulong columns = (!viewer.configuration().lineWrap.wrapsAtWindowEdge()) ? viewer.textRenderer().longestLineWidth() / dx : 0;
//		horizontal.rate = columns / numeric_limits<int>::max() + 1;
//		assert(horizontal.rate != 0);
		const int oldMaximum = horizontal.maximum;
		horizontal.maximum = max(static_cast<int>(columns/* / horizontal.rate*/), static_cast<int>(viewer.numberOfVisibleColumns() - 1));
		if(alignment == ALIGN_RIGHT)
			horizontal.position += horizontal.maximum - oldMaximum;
		else if(alignment == ALIGN_CENTER)
//			horizontal.position += (horizontal.maximum - oldMaximum) / 2;
			horizontal.position += horizontal.maximum / 2 - oldMaximum / 2;
		horizontal.position = max(horizontal.position, 0);
		if(pageSizeChanged) {
			const UINT oldPageSize = horizontal.pageSize;
			horizontal.pageSize = static_cast<UINT>(viewer.numberOfVisibleColumns());
			if(alignment == ALIGN_RIGHT)
				horizontal.position -= horizontal.pageSize - oldPageSize;
			else if(alignment == ALIGN_CENTER)
//				horizontal.position -= (horizontal.pageSize - oldPageSize) / 2;
				horizontal.position -= horizontal.pageSize / 2 - oldPageSize / 2;
			horizontal.position = max(horizontal.position, 0);
		}
	}
	// 垂直方向
	if(bars == SB_VERT || bars == SB_BOTH) {
		const length_t lines = viewer.textRenderer().numberOfVisualLines();
		assert(lines > 0);
//		vertical.rate = static_cast<ulong>(lines) / numeric_limits<int>::max() + 1;
//		assert(vertical.rate != 0);
		vertical.maximum = max(static_cast<int>((lines - 1)/* / vertical.rate*/), 0/*static_cast<int>(viewer.numberOfVisibleLines() - 1)*/);
		if(pageSizeChanged)
			vertical.pageSize = static_cast<UINT>(viewer.numberOfVisibleLines());
	}
}

void TextViewer::ScrollInfo::updateVertical(const TextViewer& viewer) /*throw()*/ {
	vertical.maximum = static_cast<int>(viewer.textRenderer().numberOfVisualLines());
	firstVisibleLine = min(firstVisibleLine, viewer.document().numberOfLines() - 1);
	firstVisibleSubline = min(viewer.textRenderer().numberOfSublinesOfLine(firstVisibleLine) - 1, firstVisibleSubline);
	vertical.position = static_cast<int>(viewer.textRenderer().mapLogicalLineToVisualLine(firstVisibleLine) + firstVisibleSubline);
}


// VirtualBox ///////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param view the viewer
 * @param region the region consists the rectangle
 */
VirtualBox::VirtualBox(const TextViewer& view, const Region& region) /*throw()*/ : view_(view) {
	update(region);
}

/**
 * Returns if the specified point is on the virtual box.
 * @param pt the client coordinates of the point
 * @return true if the point is on the virtual box
 */
bool VirtualBox::isPointOver(const POINT& pt) const /*throw()*/ {
	assert(view_.isWindow());
	if(view_.hitTest(pt) != TextViewer::TEXT_AREA)	// ignore if not in text area
		return false;
	const int leftMargin = view_.textAreaMargins().left;
	if(pt.x < left() + leftMargin || pt.x >= right() + leftMargin)	// about x-coordinate
		return false;

	// about y-coordinate
	const Point& top = beginning();
	const Point& bottom = end();
	length_t line, subline;
	view_.mapClientYToLine(pt.y, &line, &subline);	// $friendly-access
	if(line < top.line || (line == top.line && subline < top.subline))
		return false;
	else if(line > bottom.line || (line == bottom.line && subline > bottom.subline))
		return false;
	else
		return true;
}

/**
 * Returns the range which the box overlaps with in specified visual line.
 * @param line the logical line
 * @param subline the visual subline
 * @param[out] first the start of range
 * @param[out] last the end of range
 * @return true if the box and the visual line overlap
 */
bool VirtualBox::overlappedSubline(length_t line, length_t subline, length_t& first, length_t& last) const /*throw()*/ {
	assert(view_.isWindow());
	const Point& top = beginning();
	const Point& bottom = end();
	if(line < top.line || (line == top.line && subline < top.subline)	// out of the region
			|| line > bottom.line || (line == bottom.line && subline > bottom.subline))
		return false;
	else {
		const TextRenderer& renderer = view_.textRenderer();
		const LineLayout& layout = renderer.lineLayout(line);
		first = layout.offset(points_[0].x - renderer.lineIndent(line, 0), static_cast<int>(renderer.linePitch() * subline));
		last = layout.offset(points_[1].x - renderer.lineIndent(line, 0), static_cast<int>(renderer.linePitch() * subline));
		if(first > last)
			swap(first, last);
		return first != last;
	}
}

/**
 * Updates the rectangle of the virtual box.
 * @param region the region consists the rectangle
 */
void VirtualBox::update(const Region& region) /*throw()*/ {
	const TextRenderer& r = view_.textRenderer();
	const LineLayout* layout = &r.lineLayout(points_[0].line = region.first.line);
	POINT location = layout->location(region.first.column);
	points_[0].x = location.x + r.lineIndent(points_[0].line, 0);
	points_[0].subline = location.y / r.linePitch();
	layout = &r.lineLayout(points_[1].line = region.second.line);
	location = layout->location(region.second.column);
	points_[1].x = location.x + r.lineIndent(points_[1].line, 0);
	points_[1].subline = location.y / r.linePitch();
}


// DefaultMouseInputStrategy ////////////////////////////////////////////////

map<UINT_PTR, DefaultMouseInputStrategy*> DefaultMouseInputStrategy::timerTable_;
const UINT DefaultMouseInputStrategy::SELECTION_EXPANSION_INTERVAL = 100;
const UINT DefaultMouseInputStrategy::OLE_DRAGGING_TRACK_INTERVAL = 100;

/**
 * Constructor.
 * @param enableOLEDragAndDrop set true to enable OLE Drag-and-Drop feature.
 * @param showDraggingImage set true to display OLE dragging image
 */
DefaultMouseInputStrategy::DefaultMouseInputStrategy(bool enableOLEDragAndDrop,
		bool showDraggingImage) : viewer_(0), leftButtonPressed_(false), lastHoveredHyperlink_(0) {
	if(dnd_.enabled = enableOLEDragAndDrop && showDraggingImage) {
		if(S_OK == dnd_.dragSourceHelper.createInstance(CLSID_DragDropHelper, IID_IDragSourceHelper, CLSCTX_INPROC_SERVER)) {
			if(S_OK != dnd_.dropTargetHelper.createInstance(CLSID_DragDropHelper, IID_IDropTargetHelper, CLSCTX_INPROC_SERVER))
				dnd_.dragSourceHelper.release();
		}
	}
	dnd_.state = DragAndDrop::INACTIVE;
}

/// 
void DefaultMouseInputStrategy::beginTimer(UINT interval) {
	endTimer();
	if(const UINT_PTR id = ::SetTimer(0, 0, interval, timeElapsed))
		timerTable_[id] = this;
}

/// @see IMouseInputStrategy#captureChanged
void DefaultMouseInputStrategy::captureChanged() {
	endTimer();
	leftButtonPressed_ = false;
	selectionExtendingUnit_ = CHARACTERS;
}

///
void DefaultMouseInputStrategy::doDragAndDrop() {
	ComPtr<IDataObject> draggingContent;
	const Region selection(viewer_->caret().selectionRegion());

	if(FAILED(viewer_->caret().createTextObject(true, *draggingContent.initialize())))
		return;
	dnd_.numberOfRectangleLines = selection.end().line - selection.beginning().line + 1;

	// setup dragging ghost ixyzzymage
	if(dnd_.dragSourceHelper.get() != 0) {
		MANAH_AUTO_STRUCT(BITMAPV5HEADER, bh);
		bh.bV5Size = sizeof(BITMAPV5HEADER);
		bh.bV5Width = 40;
		bh.bV5Height = 40;
		bh.bV5Planes = 1;
		bh.bV5BitCount = 32;
		bh.bV5Compression = BI_BITFIELDS;
		bh.bV5RedMask = 0x00FF0000;
		bh.bV5GreenMask = 0x0000FF00;
		bh.bV5BlueMask = 0x000000FF;
		bh.bV5AlphaMask = 0xFF000000;

		SHDRAGIMAGE image;
		image.sizeDragImage.cx = bh.bV5Width;
		image.sizeDragImage.cy = bh.bV5Height;
		image.ptOffset.x = image.sizeDragImage.cx / 2;
		image.ptOffset.y = image.sizeDragImage.cy / 2;
		image.crColorKey = CLR_NONE;

		if(HDC dc = ::CreateCompatibleDC(0)) {
			void* bits;
			HBITMAP bitmap = ::CreateDIBSection(dc, reinterpret_cast<BITMAPINFO*>(&bh), DIB_RGB_COLORS, &bits, 0, 0);
			if(bitmap != 0 && bits != 0) {
				HBITMAP memoryBitmap = ::CreateCompatibleBitmap(viewer_->getDC().getHandle(), bh.bV5Width, bh.bV5Height);
				HBITMAP oldBitmap = static_cast<HBITMAP>(::SelectObject(dc, memoryBitmap));
				::SetTextColor(dc, ::GetSysColor(COLOR_HIGHLIGHTTEXT));
				::SetBkColor(dc, ::GetSysColor(COLOR_HIGHLIGHT));
				::SetBkMode(dc, OPAQUE);
				RECT rc;
				rc.left = rc.top = 0;
				rc.right = bh.bV5Width;
				rc.bottom = bh.bV5Height;
				::ExtTextOutW(dc, 0, 0, ETO_OPAQUE, &rc, L"xyzzy", 5, 0);

				MANAH_AUTO_STRUCT(BITMAPINFOHEADER, bih);
				bih.biSize = sizeof(BITMAPINFOHEADER);
				::GetDIBits(dc, memoryBitmap, 0, bh.bV5Height, 0, reinterpret_cast<BITMAPINFO*>(&bih), DIB_RGB_COLORS);
				byte* memoryBits = new byte[bih.biWidth * bih.biHeight * bih.biBitCount / 8];
				bih.biCompression = BI_RGB;
				::GetDIBits(dc, memoryBitmap, 0, bih.biHeight, memoryBits, reinterpret_cast<BITMAPINFO*>(&bih), DIB_RGB_COLORS);
				::SelectObject(dc, oldBitmap);
				for(int x = 0; x < bih.biWidth; ++x) {
					for(int y = 0; y < bih.biHeight; ++y) {
						const byte* pixel = memoryBits + (x + y * bih.biWidth) * 3;
						static_cast<ulong*>(bits)[x + y * bih.biWidth] = RGB(pixel[0], pixel[1], pixel[2]) + 0xFF000000;
					}
				}
				delete[] memoryBits;

				image.hbmpDragImage = bitmap;
				dnd_.dragSourceHelper->InitializeFromBitmap(&image, draggingContent.get());
				::DeleteObject(memoryBitmap);
				::DeleteObject(bitmap);
			}
			::DeleteDC(dc);
		}
	}

	// operation
	assert(leftButtonPressed_);
	beginTimer(viewer_->caret().isSelectionRectangle() ? OLE_DRAGGING_TRACK_INTERVAL * 2 : OLE_DRAGGING_TRACK_INTERVAL);
	dnd_.state = DragAndDrop::DRAGGING_BY_SELF;
	DWORD effectOwn;	// dummy
	::DoDragDrop(draggingContent.get(), this, DROPEFFECT_COPY | DROPEFFECT_MOVE | DROPEFFECT_SCROLL, &effectOwn);
	endTimer();
	dnd_.state = DragAndDrop::INACTIVE;
	leftButtonPressed_ = false;
	if(viewer_->isVisible())
		viewer_->setFocus();
}

/// @see IDropTarget#DragEnter
STDMETHODIMP DefaultMouseInputStrategy::DragEnter(::IDataObject* data, DWORD keyState, POINTL pt, DWORD* effect) {
	if(data == 0)
		return E_INVALIDARG;
	MANAH_VERIFY_POINTER(effect);
	*effect = DROPEFFECT_NONE;

	if(!dnd_.enabled || viewer_->document().isReadOnly()
			|| !viewer_->allowsMouseInput() || viewer_->configuration().alignment != ALIGN_LEFT)
		return S_OK;

	// validate the dragged data if can drop
	FORMATETC fe = {CF_UNICODETEXT, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
	if(data->QueryGetData(&fe) != S_OK) {
		fe.cfFormat = CF_TEXT;
		if(data->QueryGetData(&fe) != S_OK)
			return S_OK;	// can't accept
	}

	if(dnd_.state != DragAndDrop::DRAGGING_BY_SELF) {
		assert(dnd_.state == DragAndDrop::INACTIVE);
		// retrieve number of lines if text is rectangle
		dnd_.numberOfRectangleLines = 0;
		fe.cfFormat = static_cast<CLIPFORMAT>(::RegisterClipboardFormatW(ASCENSION_RECTANGLE_TEXT_CLIP_FORMAT));
		if(fe.cfFormat != 0 && data->QueryGetData(&fe) == S_OK) {
			pair<HRESULT, String> text(getTextFromDataObject(*data));
			if(SUCCEEDED(text.first))
				dnd_.numberOfRectangleLines = getNumberOfLines(text.second) - 1;
		}
		dnd_.state = DragAndDrop::DRAGGING_FROM_OTHER;
	}

	viewer_->setFocus();
	beginTimer(OLE_DRAGGING_TRACK_INTERVAL);
	if(dnd_.dropTargetHelper.get() != 0) {
		POINT p = {pt.x, pt.y};
		dnd_.dropTargetHelper->DragEnter(viewer_->getHandle(), data, &p, *effect);
	}
	return DragOver(keyState, pt, effect);
}

/// @see IDropTarget#DragLeave
STDMETHODIMP DefaultMouseInputStrategy::DragLeave() {
	::SetFocus(0);
	endTimer();
	if(dnd_.enabled) {
		if(dnd_.state == DragAndDrop::DRAGGING_FROM_OTHER)
			dnd_.state = DragAndDrop::INACTIVE;
		if(dnd_.dropTargetHelper.get() != 0)
			dnd_.dropTargetHelper->DragLeave();
	}
	return S_OK;
}

/// @see IDropTarget#DragOver
STDMETHODIMP DefaultMouseInputStrategy::DragOver(DWORD keyState, POINTL pt, DWORD* effect) {
	MANAH_VERIFY_POINTER(effect);
	*effect = DROPEFFECT_NONE;

	if(!dnd_.isDragging() || viewer_->document().isReadOnly() || !viewer_->allowsMouseInput())
		return S_OK;

	POINT caretPoint = {pt.x, pt.y};
	viewer_->screenToClient(caretPoint);
	const Position p(viewer_->characterForClientXY(caretPoint, LineLayout::TRAILING));
	viewer_->setCaretPosition(viewer_->clientXYForCharacter(p, true, LineLayout::LEADING));

	// drop rectangle text into bidirectional line is not supported...
	const length_t lines = min(viewer_->document().numberOfLines(), p.line + dnd_.numberOfRectangleLines);
	for(length_t line = p.line; line < lines; ++line) {
		if(viewer_->textRenderer().lineLayout(line).isBidirectional()) {
			*effect = DROPEFFECT_NONE;
			return S_OK;
		}
	}

	if(toBoolean(keyState & MK_CONTROL & MK_SHIFT))
		*effect = DROPEFFECT_NONE;
	else
		*effect = (!leftButtonPressed_ || toBoolean(keyState & MK_CONTROL)) ? DROPEFFECT_COPY : DROPEFFECT_MOVE;
	if(dnd_.dropTargetHelper.get() != 0) {
		POINT p = {pt.x, pt.y};
		dnd_.dropTargetHelper->DragOver(&p, *effect);
	}
	return S_OK;
}

/// @see IDropTarget#Drop
STDMETHODIMP DefaultMouseInputStrategy::Drop(::IDataObject* data, DWORD keyState, POINTL pt, DWORD* effect) {
	if(dnd_.dropTargetHelper.get() != 0) {
		POINT p = {pt.x, pt.y};
		dnd_.dropTargetHelper->Drop(data, &p, *effect);
	}
	if(data == 0)
		return E_INVALIDARG;
	MANAH_VERIFY_POINTER(effect);
	*effect = DROPEFFECT_NONE;
/*
	FORMATETC fe = {::RegisterClipboardFormatA("Rich Text Format"), 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
	STGMEDIUM stg;
	data->GetData(&fe, &stg);
	const char* bytes = static_cast<char*>(::GlobalLock(stg.hGlobal));
	manah::win32::DumpContext dout;
	dout << bytes;
	::GlobalUnlock(stg.hGlobal);
	::ReleaseStgMedium(&stg);
*/
	Document& document = viewer_->document();
	if(!dnd_.enabled || document.isReadOnly() || !viewer_->allowsMouseInput())
		return S_OK;
	Caret& ca = viewer_->caret();
	POINT caretPoint = {pt.x, pt.y};
	viewer_->screenToClient(caretPoint);
	const Position destination(viewer_->characterForClientXY(caretPoint, LineLayout::TRAILING));

	if(!document.accessibleRegion().includes(destination))
		return S_OK;

	if(dnd_.state == DragAndDrop::DRAGGING_FROM_OTHER) {	// dropped from the other widget
		endTimer();
		ca.moveTo(destination);

		bool rectangle;
		pair<HRESULT, String> text(getTextFromDataObject(*data, &rectangle));
		if(SUCCEEDED(text.first)) {
			viewer_->freeze();
			if(rectangle ? ca.insertRectangle(text.second) : ca.insert(text.second)) {
				if(rectangle)
					ca.beginRectangleSelection();
				ca.select(destination, ca);
				*effect = DROPEFFECT_COPY;
			}
			viewer_->unfreeze();
		}
		dnd_.state = DragAndDrop::INACTIVE;
	} else {	// drop from the same widget
		assert(dnd_.state == DragAndDrop::DRAGGING_BY_SELF);
		String text(ca.selectionText(NLF_RAW_VALUE));

		// can't drop into the selection
		if(ca.isPointOverSelection(caretPoint)) {
			ca.moveTo(destination);
			dnd_.state = DragAndDrop::INACTIVE;
		} else {
			const bool rectangle = ca.isSelectionRectangle();
			document.insertUndoBoundary();
			viewer_->freeze();
			if(toBoolean(keyState & MK_CONTROL)) {	// copy
//				viewer_->redrawLines(ca.beginning().lineNumber(), ca.end().lineNumber());
				ca.enableAutoShow(false);
				ca.moveTo(destination);
				if(rectangle)	// copy as a rectangle
					ca.insertRectangle(text);
				else	// copy as linear
					ca.insert(text);
				ca.enableAutoShow(true);
				ca.select(destination, ca);
				*effect = DROPEFFECT_COPY;
			} else if(rectangle) {	// move as a rectangle
				kernel::Point p(document);
				p.moveTo(destination);
				if(ca.eraseSelection()) {
					p.adaptToDocument(false);
					ca.enableAutoShow(false);
					ca.extendSelection(p);
					ca.insertRectangle(text);
					ca.enableAutoShow(true);
					ca.select(p, ca);
					*effect = DROPEFFECT_MOVE;
				}
			} else {	// move as linear
				VisualPoint activePointOrg(*viewer_);
				const Position anchorPointOrg = ca.anchor();
				activePointOrg.moveTo(ca);
				ca.enableAutoShow(false);
				ca.moveTo(destination);
				if(document.erase(anchorPointOrg, activePointOrg)) {
					const Position temp = ca;
					ca.endRectangleSelection();
					ca.insert(text);
					ca.enableAutoShow(true);
					ca.select(temp, ca);
					*effect = DROPEFFECT_MOVE;
				}
			}
			viewer_->unfreeze();
			document.insertUndoBoundary();
		}
	}
	return S_OK;
}

///
void DefaultMouseInputStrategy::endTimer() {
	for(map<UINT_PTR, DefaultMouseInputStrategy*>::iterator i = timerTable_.begin(); i != timerTable_.end(); ++i) {
		if(i->second == this) {
			::KillTimer(0, i->first);
			timerTable_.erase(i);
			return;
		}
	}
}

/// Extends the selection to the current cursor position.
void DefaultMouseInputStrategy::extendSelection(const Position* to /* = 0 */) {
	Position destination;
	if(to == 0) {
		RECT rc;
		const RECT margins = viewer_->textAreaMargins();
		viewer_->getClientRect(rc);
		POINT p = viewer_->getCursorPosition();
		Caret& caret = viewer_->caret();
		if(selectionExtendingUnit_ != CHARACTERS) {
			const TextViewer::HitTestResult htr = viewer_->hitTest(p);
			if(selectionExtendingUnit_ == LINES && htr != TextViewer::INDICATOR_MARGIN && htr != TextViewer::LINE_NUMBERS)
				// end line selection
				selectionExtendingUnit_ = CHARACTERS;
		}
		p.x = min(max(p.x, rc.left + margins.left), rc.right - margins.right);
		p.y = min(max(p.y, rc.top + margins.top), rc.bottom - margins.bottom);
		destination = viewer_->characterForClientXY(p, LineLayout::TRAILING);
	} else
		destination = *to;

	const Document& document = viewer_->document();
	Caret& caret = viewer_->caret();
	if(selectionExtendingUnit_ == CHARACTERS)
		caret.extendSelection(destination);
	else if(selectionExtendingUnit_ == LINES) {
		const length_t lines = document.numberOfLines();
		Region s;
		s.first.line = (destination.line >= noncharacterSelectionExtendingInitialLine_) ?
			noncharacterSelectionExtendingInitialLine_ : noncharacterSelectionExtendingInitialLine_ + 1;
		s.first.column = (s.first.line > lines - 1) ? document.lineLength(--s.first.line) : 0;
		s.second.line = (destination.line >= noncharacterSelectionExtendingInitialLine_) ? destination.line + 1 : destination.line;
		s.second.column = (s.second.line > lines - 1) ? document.lineLength(--s.second.line) : 0;
		caret.select(s);
	} else if(selectionExtendingUnit_ == WORDS) {
		using namespace text;
		const IdentifierSyntax& id = document.contentTypeInformation().getIdentifierSyntax(caret.getContentType());
		if(destination.line < noncharacterSelectionExtendingInitialLine_
				|| (destination.line == noncharacterSelectionExtendingInitialLine_
					&& destination.column < wordSelectionInitialColumns_.first)) {
			WordBreakIterator<DocumentCharacterIterator> i(
				DocumentCharacterIterator(document, destination), AbstractWordBreakIterator::BOUNDARY_OF_SEGMENT, id);
			--i;
			caret.select(Position(noncharacterSelectionExtendingInitialLine_, wordSelectionInitialColumns_.second),
				(i.base().tell().line == destination.line) ? i.base().tell() : Position(destination.line, 0));
		} else if(destination.line > noncharacterSelectionExtendingInitialLine_
				|| (destination.line == noncharacterSelectionExtendingInitialLine_
					&& destination.column > wordSelectionInitialColumns_.second)) {
			WordBreakIterator<DocumentCharacterIterator> i(
				DocumentCharacterIterator(document, destination), AbstractWordBreakIterator::BOUNDARY_OF_SEGMENT, id);
			++i;
			caret.select(Position(noncharacterSelectionExtendingInitialLine_, wordSelectionInitialColumns_.first),
				(i.base().tell().line == destination.line) ?
					i.base().tell() : Position(destination.line, document.lineLength(destination.line)));
		} else
			caret.select(Position(noncharacterSelectionExtendingInitialLine_, wordSelectionInitialColumns_.first),
				Position(noncharacterSelectionExtendingInitialLine_, wordSelectionInitialColumns_.second));
	}
}

/// @see IDropSource#GiveFeedback
STDMETHODIMP DefaultMouseInputStrategy::GiveFeedback(DWORD) {
	return DRAGDROP_S_USEDEFAULTCURSORS;	// use the system default cursor
}

/// Handles @c WM_LBUTTONDOWN.
void DefaultMouseInputStrategy::handleLeftButtonPressed(const POINT& position, uint keyState) {
	bool boxDragging = false;
	Caret& caret = viewer_->caret();
	const TextViewer::HitTestResult htr = viewer_->hitTest(position);

	closeCompletionProposalsPopup(*viewer_);
	endIncrementalSearch(*viewer_);
	leftButtonPressed_ = true;

	// select line(s)
	if(htr == TextViewer::INDICATOR_MARGIN || htr == TextViewer::LINE_NUMBERS) {
		const Position to(viewer_->characterForClientXY(position, LineLayout::LEADING));
		const bool extend = toBoolean(keyState & MK_SHIFT) && to.line != caret.anchor().lineNumber();
		selectionExtendingUnit_ = LINES;
		noncharacterSelectionExtendingInitialLine_ = extend ? caret.anchor().lineNumber() : to.line;
		extendSelection(&to);
		viewer_->setCapture();
		beginTimer(SELECTION_EXPANSION_INTERVAL);
	}

	// approach OLE drag-and-drop
	else if(dnd_.enabled && !caret.isSelectionEmpty() && caret.isPointOverSelection(position)) {
		dnd_.state = DragAndDrop::APPROACHING;
		dnd_.approachedPosition = position;
		if(caret.isSelectionRectangle())
			boxDragging = true;
	}

	else {
		// try hyperlink
		bool hyperlinkInvoked = false;
		if(toBoolean(keyState & MK_CONTROL)) {
			if(!caret.isPointOverSelection(position)) {
				const Position p(viewer_->characterForClientXY(position, LineLayout::TRAILING, true));
				if(p != Position::INVALID_POSITION) {
					if(const hyperlink::IHyperlink* link = getPointedHyperlink(*viewer_, p)) {
						link->invoke();
						hyperlinkInvoked = true;
					}
				}
			}
		}

		if(!hyperlinkInvoked) {
			// modification keys and result
			//
			// shift => keep the anchor and move the caret to the cursor position
			// ctrl  => begin word selection
			// alt   => begin rectangle selection
			const Position to(viewer_->characterForClientXY(position, LineLayout::TRAILING));
			if(toBoolean(keyState & MK_CONTROL)) {
				// begin word selection
				selectionExtendingUnit_ = WORDS;
				caret.moveTo(toBoolean(keyState & MK_SHIFT) ? caret.anchor() : to);
				caret.selectWord();
				noncharacterSelectionExtendingInitialLine_ = caret.lineNumber();
				wordSelectionInitialColumns_ = make_pair(caret.beginning().columnNumber(), caret.end().columnNumber());
			}
			if(toBoolean(keyState & MK_SHIFT))
				extendSelection(&to);
			if(toBoolean(::GetKeyState(VK_MENU) & 0x8000))	// make the selection reactangle
				caret.beginRectangleSelection();
			viewer_->setCapture();
			beginTimer(SELECTION_EXPANSION_INTERVAL);
		}
	}

//	if(!caret.isSelectionRectangle() && !boxDragging)
//		viewer_->redrawLine(caret.getLineNumber());
	viewer_->setFocus();
}

/// Handles @c WM_LBUTTONUP.
void DefaultMouseInputStrategy::handleLeftButtonReleased(const POINT& position, uint) {
	// OLE ドラッグ開始か -> キャンセル
	if(dnd_.enabled
			&& (dnd_.state == DragAndDrop::APPROACHING
			|| dnd_.state == DragAndDrop::DRAGGING_BY_SELF)) {	// TODO: this should handle only case APPROACHING?
		dnd_.state = DragAndDrop::INACTIVE;
		viewer_->caret().moveTo(viewer_->characterForClientXY(position, LineLayout::TRAILING));
		::SetCursor(::LoadCursor(0, IDC_IBEAM));	// hmm...
	}

	endTimer();
	if(leftButtonPressed_) {
		leftButtonPressed_ = false;
		// 選択範囲拡大中に画面外でボタンを離すとキャレット位置までスクロールしないことがある
		viewer_->caret().show();
	}
	viewer_->releaseCapture();
}

/// @see IMouseInputStrategy#install
void DefaultMouseInputStrategy::install(TextViewer& viewer) {
	if(viewer_ != 0)
		uninstall();
	(viewer_ = &viewer)->registerDragDrop(*this);
	selectionExtendingUnit_ = CHARACTERS;
}

/// @see IMouseInputStrategy#mouseButtonInput
bool DefaultMouseInputStrategy::mouseButtonInput(Button button, Action action, const POINT& position, uint keyState) {
	if(action != RELEASED && viewer_->isAutoScrolling()) {
		viewer_->endAutoScroll();
		return true;
	} else if(button == LEFT_BUTTON) {
		if(action == PRESSED)
			handleLeftButtonPressed(position, keyState);
		else if(action == RELEASED)
			handleLeftButtonReleased(position, keyState);
		else if(action == DOUBLE_CLICKED) {
			abortIncrementalSearch(*viewer_);
			const TextViewer::HitTestResult htr = viewer_->hitTest(viewer_->getCursorPosition());
			if(htr == TextViewer::LEADING_MARGIN || htr == TextViewer::TOP_MARGIN || htr == TextViewer::TEXT_AREA) {
				// begin word selection
				Caret& caret = viewer_->caret();
				caret.selectWord();
				selectionExtendingUnit_ = WORDS;
				noncharacterSelectionExtendingInitialLine_ = caret.lineNumber();
				wordSelectionInitialColumns_ = make_pair(caret.anchor().columnNumber(), caret.columnNumber());
				viewer_->setCapture();
				beginTimer(SELECTION_EXPANSION_INTERVAL);
			}
		}
	} else if(button == MIDDLE_BUTTON) {
		if(action == PRESSED) {
			viewer_->setFocus();
			viewer_->beginAutoScroll();
			return true;
		}
	}
	return false;
}

/// @see IMouseInputStrategy#mouseMoved
void DefaultMouseInputStrategy::mouseMoved(const POINT& position, uint) {
	if(dnd_.enabled && dnd_.state == DragAndDrop::APPROACHING) {	// OLE dragging starts?
		if(viewer_->caret().isSelectionEmpty())
			dnd_.state = DragAndDrop::INACTIVE;	// approaching... => cancel
		else {
			const int cxDragBox = ::GetSystemMetrics(SM_CXDRAG);
			const int cyDragBox = ::GetSystemMetrics(SM_CYDRAG);
			if((position.x > dnd_.approachedPosition.x + cxDragBox / 2)
					|| (position.x < dnd_.approachedPosition.x - cxDragBox / 2)
					|| (position.y > dnd_.approachedPosition.y + cyDragBox / 2)
					|| (position.y < dnd_.approachedPosition.y - cyDragBox / 2))
				doDragAndDrop();
		}
	} else if(leftButtonPressed_)
		extendSelection();
}

/// @see IMouseInputStrategy#mouseWheelRotated
void DefaultMouseInputStrategy::mouseWheelRotated(short delta, const POINT&, uint) {
	if(!viewer_->endAutoScroll()) {
		// use system settings
		UINT lines;	// the number of lines to scroll
		if(!toBoolean(::SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &lines, 0)))
			lines = 3;
		if(lines == WHEEL_PAGESCROLL)
			lines = static_cast<UINT>(viewer_->numberOfVisibleLines());
		delta *= static_cast<short>(lines);
		viewer_->scroll(0, -delta / WHEEL_DELTA, true);
	}
}

/// @see IDropSource#QueryContinueDrag
STDMETHODIMP DefaultMouseInputStrategy::QueryContinueDrag(BOOL escapePressed, DWORD keyState) {
	if(toBoolean(escapePressed) || toBoolean(keyState & MK_RBUTTON))	// cancel
		return DRAGDROP_S_CANCEL;
	if(!toBoolean(keyState & MK_LBUTTON))	// drop
		return DRAGDROP_S_DROP;
	return S_OK;
}

/// @see IMouseInputStrategy#showCursor
bool DefaultMouseInputStrategy::showCursor(const POINT& position) {
	using namespace hyperlink;
	LPCTSTR cursorName = 0;
	const IHyperlink* newlyHoveredHyperlink = 0;

	// on the vertical ruler?
	const TextViewer::HitTestResult htr = viewer_->hitTest(position);
	if(htr == TextViewer::INDICATOR_MARGIN || htr == TextViewer::LINE_NUMBERS)
		cursorName = IDC_ARROW;
	// on a draggable text selection?
	else if(dnd_.enabled && !viewer_->caret().isSelectionEmpty() && viewer_->caret().isPointOverSelection(position))
		cursorName = IDC_ARROW;
	else if(htr == TextViewer::TEXT_AREA) {
		// on a hyperlink?
		const Position p(viewer_->characterForClientXY(position, LineLayout::TRAILING, true, EditPoint::UTF16_CODE_UNIT));
		if(p != Position::INVALID_POSITION)
			newlyHoveredHyperlink = getPointedHyperlink(*viewer_, p);
		if(newlyHoveredHyperlink != 0 && toBoolean(::GetAsyncKeyState(VK_CONTROL) & 0x8000))
			cursorName = IDC_HAND;
	}

	if(cursorName != 0) {
		::SetCursor(static_cast<HCURSOR>(::LoadImage(
			0, cursorName, IMAGE_CURSOR, 0, 0, LR_DEFAULTCOLOR | LR_DEFAULTSIZE | LR_SHARED)));
		return true;
	}
	if(newlyHoveredHyperlink != 0) {
		if(newlyHoveredHyperlink != lastHoveredHyperlink_)
			viewer_->showToolTip(newlyHoveredHyperlink->description(), 1000, 30000);
	} else
		viewer_->hideToolTip();
	lastHoveredHyperlink_ = newlyHoveredHyperlink;
	return false;
}

///
void CALLBACK DefaultMouseInputStrategy::timeElapsed(HWND, UINT, UINT_PTR eventID, DWORD) {
	map<UINT_PTR, DefaultMouseInputStrategy*>::iterator i = timerTable_.find(eventID);
	if(i == timerTable_.end())
		return;
	DefaultMouseInputStrategy& self = *i->second;
	if(self.dnd_.enabled && self.dnd_.isDragging()) {	// scroll automatically during OLE dragging
		const POINT pt = self.viewer_->getCursorPosition();
		RECT clientRect;
		self.viewer_->getClientRect(clientRect);
		RECT margins = self.viewer_->textAreaMargins();
		const TextRenderer& renderer = self.viewer_->textRenderer();
		margins.left = max<long>(renderer.averageCharacterWidth(), margins.left);
		margins.top = max<long>(renderer.linePitch() / 2, margins.top);
		margins.right = max<long>(renderer.averageCharacterWidth(), margins.right);
		margins.bottom = max<long>(renderer.linePitch() / 2, margins.bottom);

		// 以下のスクロール量には根拠は無い
		if(pt.y >= clientRect.top && pt.y < clientRect.top + margins.top)
			self.viewer_->scroll(0, -1, true);
		else if(pt.y >= clientRect.bottom - margins.bottom && pt.y < clientRect.bottom)
			self.viewer_->scroll(0, +1, true);
		else if(pt.x >= clientRect.left && pt.x < clientRect.left + margins.left)
			self.viewer_->scroll(-3/*viewer_->numberOfVisibleColumns()*/, 0, true);
		else if(pt.x >= clientRect.right - margins.right && pt.y < clientRect.right)
			self.viewer_->scroll(+3/*viewer_->numberOfVisibleColumns()*/, 0, true);
	} else if(self.leftButtonPressed_) {	// scroll automatically during extending the selection
		const POINT pt = self.viewer_->getCursorPosition();
		RECT rc;
		const RECT margins = self.viewer_->textAreaMargins();
		self.viewer_->getClientRect(rc);
		// 以下のスクロール量には根拠は無い
		if(pt.y < rc.top + margins.top)
			self.viewer_->scroll(0, (pt.y - (rc.top + margins.top)) / self.viewer_->textRenderer().linePitch() - 1, true);
		else if(pt.y >= rc.bottom - margins.bottom)
			self.viewer_->scroll(0, (pt.y - (rc.bottom - margins.bottom)) / self.viewer_->textRenderer().linePitch() + 1, true);
		else if(pt.x < rc.left + margins.left)
			self.viewer_->scroll((pt.x - (rc.left + margins.left)) / self.viewer_->textRenderer().averageCharacterWidth() - 1, 0, true);
		else if(pt.x >= rc.right - margins.right)
			self.viewer_->scroll((pt.x - (rc.right - margins.right)) / self.viewer_->textRenderer().averageCharacterWidth() + 1, 0, true);
		self.extendSelection();
	}
}

/// @see IMouseInputStrategy#uninstall
void DefaultMouseInputStrategy::uninstall() {
	endTimer();
	viewer_->revokeDragDrop();
	viewer_ = 0;
}


// CaretShapeUpdater ////////////////////////////////////////////////////////

/**
 * Private constructor.
 * @param viewer the text viewer
 */
CaretShapeUpdater::CaretShapeUpdater(TextViewer& viewer) /*throw()*/ : viewer_(viewer) {
}

/// Notifies the text viewer to update the shape of the caret.
void CaretShapeUpdater::update() /*throw()*/ {
	viewer_.recreateCaret();	// $friendly-access
}

/// Returns the text viewer.
TextViewer& CaretShapeUpdater::textViewer() /*throw()*/ {
	return viewer_;
}


// DefaultCaretShaper ///////////////////////////////////////////////////////

/// Constructor.
DefaultCaretShaper::DefaultCaretShaper() /*throw()*/ : viewer_(0) {
}

/// @see ICaretShapeProvider#getCaretShape
void DefaultCaretShaper::getCaretShape(auto_ptr<Bitmap>&, SIZE& solidSize, Orientation& orientation) /*throw()*/ {
	DWORD width;
	if(::SystemParametersInfo(SPI_GETCARETWIDTH, 0, &width, 0) == 0)
		width = 1;	// NT4 does not support SPI_GETCARETWIDTH
	solidSize.cx = width;
	solidSize.cy = viewer_->textRenderer().lineHeight();
	orientation = LEFT_TO_RIGHT;	// no matter
}

/// @see ICaretShapeProvider#install
void DefaultCaretShaper::install(CaretShapeUpdater& updater) /*throw()*/ {
	viewer_ = &updater.textViewer();
}

/// @see ICaretShapeProvider#uninstall
void DefaultCaretShaper::uninstall() /*throw()*/ {
	viewer_ = 0;
}


// LocaleSensitiveCaretShaper ///////////////////////////////////////////////

namespace {
	/// Returns true if the specified language is RTL.
	inline bool isRTLLanguage(LANGID id) /*throw()*/ {
		return id == LANG_ARABIC || id == LANG_FARSI || id == LANG_HEBREW || id == LANG_SYRIAC || id == LANG_URDU;
	}
	/// Returns true if the specified language is Thai or Lao.
	inline bool isTISLanguage(LANGID id) /*throw()*/ {
#ifndef LANG_LAO
		const LANGID LANG_LAO = 0x54;
#endif // !LANG_LAO
		return id == LANG_THAI || id == LANG_LAO;
	}
	/**
	 * Returns the bitmap has specified size.
	 * @param dc the device context
	 * @param width the width of the bitmap
	 * @param height the height of the bitmap
	 * @return the bitmap. this value is allocated via the global @c operator @c new
	 */
	inline BITMAPINFO* prepareCaretBitmap(const DC& dc, ushort width, ushort height) {
		BITMAPINFO* info = static_cast<BITMAPINFO*>(operator new(sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * width * height));
		BITMAPINFOHEADER& header = info->bmiHeader;
		memset(&header, 0, sizeof(BITMAPINFOHEADER));
		header.biSize = sizeof(BITMAPINFOHEADER);
		header.biWidth = width;
		header.biHeight = -height;
		header.biBitCount = sizeof(RGBQUAD) * 8;//::GetDeviceCaps(hDC, BITSPIXEL);
		header.biPlanes = static_cast<WORD>(dc.getDeviceCaps(PLANES));
		return info;
	}
	/**
	 * Creates the bitmap for solid caret.
	 * @param[in,out] bitmap the bitmap
	 * @param width the width of the rectangle in pixels
	 * @param height the height of the rectangle in pixels
	 * @param color the color
	 */
	inline void createSolidCaretBitmap(Bitmap& bitmap, ushort width, ushort height, const RGBQUAD& color) {
		ScreenDC dc;
		BITMAPINFO* info = prepareCaretBitmap(dc, width, height);
		uninitialized_fill(info->bmiColors, info->bmiColors + width * height, color);
		bitmap.createDIBitmap(dc, info->bmiHeader, CBM_INIT, info->bmiColors, *info, DIB_RGB_COLORS);
		operator delete(info);
	}
	/**
	 * Creates the bitmap for RTL caret.
	 * @param[in,out] bitmap the bitmap
	 * @param height the height of the image in pixels
	 * @param bold set true to create a bold shape
	 * @param color the color
	 */
	inline void createRTLCaretBitmap(Bitmap& bitmap, ushort height, bool bold, const RGBQUAD& color) {
		ScreenDC dc;
		const RGBQUAD white = {0x00, 0x00, 0x00, 0x00};
		BITMAPINFO* info = prepareCaretBitmap(dc, 5, height);
		assert(height > 3);
		uninitialized_fill(info->bmiColors, info->bmiColors + 5 * height, white);
		info->bmiColors[0] = info->bmiColors[1] = info->bmiColors[2]
			= info->bmiColors[6] = info->bmiColors[7] = info->bmiColors[12] = color;
		for(ushort i = 0; i < height; ++i) {
			info->bmiColors[i * 5 + 3] = color;
			if(bold)
				info->bmiColors[i * 5 + 4] = color;
		}
		bitmap.createDIBitmap(dc, info->bmiHeader, CBM_INIT, info->bmiColors, *info, DIB_RGB_COLORS);
		operator delete(info);
	}
	/**
	 * Creates the bitmap for Thai or Lao caret.
	 * @param[in,out] bitmap the bitmap
	 * @param height the height of the image in pixels
	 * @param bold set true to create a bold shape
	 * @param color the color
	 */
	inline void createTISCaretBitmap(Bitmap& bitmap, ushort height, bool bold, const RGBQUAD& color) {
		ScreenDC dc;
		const RGBQUAD white = {0x00, 0x00, 0x00, 0x00};
		const ushort width = max<ushort>(height / 8, 3);
		BITMAPINFO* info = prepareCaretBitmap(dc, width, height);
		assert(height > 3);
		uninitialized_fill(info->bmiColors, info->bmiColors + width * height, white);
		for(ushort y = 0; y < height - 1; ++y) {
			info->bmiColors[y * width] = color;
			if(bold) info->bmiColors[y * width + 1] = color;
		}
		if(bold)
			for(ushort x = 2; x < width; ++x)
				info->bmiColors[width * (height - 2) + x] = color;
		for(ushort x = 0; x < width; ++x)
			info->bmiColors[width * (height - 1) + x] = color;
		bitmap.createDIBitmap(dc, info->bmiHeader, CBM_INIT, info->bmiColors, *info, DIB_RGB_COLORS);
		operator delete(info);
	}
} // namespace @0

/// Constructor.
LocaleSensitiveCaretShaper::LocaleSensitiveCaretShaper(bool bold /* = false */) /*throw()*/ : updater_(0), bold_(bold) {
}

/// @see ICaretListener#caretMoved
void LocaleSensitiveCaretShaper::caretMoved(const Caret& self, const Region&) {
	if(self.isOvertypeMode())
		updater_->update();
}

/// @see ICaretShapeProvider#getCaretShape
void LocaleSensitiveCaretShaper::getCaretShape(auto_ptr<Bitmap>& bitmap, SIZE& solidSize, Orientation& orientation) /*throw()*/ {
	const Caret& caret = updater_->textViewer().caret();
	const bool overtype = caret.isOvertypeMode() && caret.isSelectionEmpty();

	if(!overtype) {
		solidSize.cx = bold_ ? 2 : 1;	// this ignores the system setting...
		solidSize.cy = updater_->textViewer().textRenderer().lineHeight();
	} else	// use the width of the glyph when overtype mode
		getCurrentCharacterSize(updater_->textViewer(), solidSize);
	orientation = LEFT_TO_RIGHT;

	HIMC imc = ::ImmGetContext(updater_->textViewer().getHandle());
	const bool imeOpened = toBoolean(::ImmGetOpenStatus(imc));
	::ImmReleaseContext(updater_->textViewer().getHandle(), imc);
	if(imeOpened) {	// CJK and IME is open
		static const RGBQUAD red = {0xFF, 0xFF, 0x80, 0x00};
		bitmap.reset(new Bitmap);
		createSolidCaretBitmap(*bitmap.get(), static_cast<ushort>(solidSize.cx), static_cast<ushort>(solidSize.cy), red);
	} else if(!overtype && solidSize.cy > 3) {
		static const RGBQUAD black = {0xFF, 0xFF, 0xFF, 0x00};
		const WORD langID = PRIMARYLANGID(LOWORD(::GetKeyboardLayout(::GetCurrentThreadId())));
		if(isRTLLanguage(langID)) {	// RTL
			bitmap.reset(new Bitmap);
			createRTLCaretBitmap(*bitmap.get(), static_cast<ushort>(solidSize.cy), bold_, black);
			orientation = RIGHT_TO_LEFT;
		} else if(isTISLanguage(langID)) {	// Thai relations
			bitmap.reset(new Bitmap);
			createTISCaretBitmap(*bitmap.get(), static_cast<ushort>(solidSize.cy), bold_, black);
		}
	}
}

/// @see ICaretShapeProvider#install
void LocaleSensitiveCaretShaper::install(CaretShapeUpdater& updater) {
	updater_ = &updater;
}

/// @see ICaretShapeProvider#matchBracketsChanged
void LocaleSensitiveCaretShaper::matchBracketsChanged(const Caret&, const std::pair<Position, Position>&, bool) {
}

/// @see ICaretStateListener#overtypeModeChanged
void LocaleSensitiveCaretShaper::overtypeModeChanged(const Caret&) {
	updater_->update();
}

/// @see ICaretShapeProvider#selectionShapeChanged
void LocaleSensitiveCaretShaper::selectionShapeChanged(const Caret&) {
}

/// @see ITextViewerInputStatusListener#textViewerIMEOpenStatusChanged
void LocaleSensitiveCaretShaper::textViewerIMEOpenStatusChanged() /*throw()*/ {
	updater_->update();
}

/// @see ITextViewerInputStatusListener#textViewerInputLanguageChanged
void LocaleSensitiveCaretShaper::textViewerInputLanguageChanged() /*throw()*/ {
	updater_->update();
}

/// @see ICaretShapeProvider#uninstall
void LocaleSensitiveCaretShaper::uninstall() {
	updater_ = 0;
}


// CurrentLineHighlighter ///////////////////////////////////////////////////

/**
 * @class ascension::presentation::CurrentLineHighlighter
 * Highlights a line the caret is on with the specified background color.
 *
 * Because an instance automatically registers itself as a line color director, you should not call
 * @c Presentation#addLineColorDirector method. Usual usage is as follows.
 *
 * @code
 * Caret& caret = ...;
 * new CurrentLineHighlighter(caret);
 * @endcode
 *
 * When the caret has a selection, highlight is canceled.
 */

/// The priority value this class returns.
const ILineColorDirector::Priority CurrentLineHighlighter::LINE_COLOR_PRIORITY = 0x40;

/**
 * Constructor.
 * @param caret the caret
 * @param color the initial color
 */
CurrentLineHighlighter::CurrentLineHighlighter(Caret& caret,
		const Colors& color /* = Colors(STANDARD_COLOR, COLOR_INFOBK | SYSTEM_COLOR_MASK) */) : caret_(&caret), color_(color) {
	ASCENSION_SHARED_POINTER<ILineColorDirector> temp(this);
	caret.textViewer().presentation().addLineColorDirector(temp);
	caret.addListener(*this);
	caret.addStateListener(*this);
	caret.addLifeCycleListener(*this);
}

/// Destructor.
CurrentLineHighlighter::~CurrentLineHighlighter() /*throw()*/ {
	if(caret_ != 0) {
		caret_->removeListener(*this);
		caret_->removeStateListener(*this);
		caret_->textViewer().presentation().removeLineColorDirector(*this);
	}
}

/// @see ICaretListener#caretMoved
void CurrentLineHighlighter::caretMoved(const Caret&, const Region& oldRegion) {
	if(oldRegion.isEmpty()) {
		if(!caret_->isSelectionEmpty() || caret_->lineNumber() != oldRegion.first.line)
			caret_->textViewer().redrawLine(oldRegion.first.line, false);
	}
	if(caret_->isSelectionEmpty()) {
		if(!oldRegion.isEmpty() || caret_->lineNumber() != oldRegion.first.line)
			caret_->textViewer().redrawLine(caret_->lineNumber(), false);
	}
}

/// Returns the color.
const Colors& CurrentLineHighlighter::color() const /*throw()*/ {
	return color_;
}

/// @see ICaretStateListener#matchBracketsChanged
void CurrentLineHighlighter::matchBracketsChanged(const Caret&, const pair<Position, Position>&, bool) {
}

/// @see ICaretStateListener#overtypeModeChanged
void CurrentLineHighlighter::overtypeModeChanged(const Caret&) {
}

/// @see IPointLifeCycleListener#pointDestroyed
void CurrentLineHighlighter::pointDestroyed() {
//	caret_->removeListener(*this);
//	caret_->removeStateListener(*this);
	caret_ = 0;
}

/// @see ILineColorDirector#queryLineColor
ILineColorDirector::Priority CurrentLineHighlighter::queryLineColor(length_t line, Colors& color) const {
	if(caret_ != 0 && caret_->isSelectionEmpty() && caret_->lineNumber() == line && caret_->textViewer().hasFocus()) {
		color = color_;
		return LINE_COLOR_PRIORITY;
	} else {
		color = Colors();
		return 0;
	}
}

/// @see ICaretStateListener#selectionShapeChanged
void CurrentLineHighlighter::selectionShapeChanged(const Caret&) {
}

/**
 * Sets the color and redraws the window.
 * @param color the color
 */
void CurrentLineHighlighter::setColor(const Colors& color) /*throw()*/ {
	color_ = color;
}


// ascension.source free functions //////////////////////////////////////////

/**
 * Returns the identifier near the specified position in the document.
 * @param document the document
 * @param position the position
 * @param[out] startColumn the start of the identifier. can be @c null if not needed
 * @param[out] endColumn the end of the identifier. can be @c null if not needed
 * @return false if the identifier is not found (in this case, the values of the output parameters are undefined)
 * @see #getPointedIdentifier
 */
bool source::getNearestIdentifier(const Document& document, const Position& position, length_t* startColumn, length_t* endColumn) {
	using namespace text;
	static const length_t MAXIMUM_IDENTIFIER_HALF_LENGTH = 100;

	DocumentPartition partition;
	document.partitioner().partition(position, partition);
	const IdentifierSyntax& syntax = document.contentTypeInformation().getIdentifierSyntax(partition.contentType);
	length_t start = position.column, end = position.column;

	// find the start of the identifier
	if(startColumn != 0) {
		DocumentCharacterIterator i(document, Region(max(partition.region.beginning(), Position(position.line, 0)), position), position);
		do {
			i.previous();
			if(!syntax.isIdentifierContinueCharacter(i.current())) {
				i.next();
				start = i.tell().column;
				break;
			} else if(position.column - i.tell().column > MAXIMUM_IDENTIFIER_HALF_LENGTH)	// too long identifier
				return false;
		} while(i.hasPrevious());
		if(!i.hasPrevious())
			start = i.tell().column;
		if(startColumn!= 0)
			*startColumn = start;
	}

	// find the end of the identifier
	if(endColumn != 0) {
		DocumentCharacterIterator i(document, Region(position,
			min(partition.region.end(), Position(position.line, document.lineLength(position.line)))), position);
		while(i.hasNext()) {
			if(!syntax.isIdentifierContinueCharacter(i.current())) {
				end = i.tell().column;
				break;
			}
			i.next();
			if(i.tell().column - position.column > MAXIMUM_IDENTIFIER_HALF_LENGTH)	// too long identifier
				return false;
		}
		if(!i.hasNext())
			end = i.tell().column;
		if(endColumn != 0)
			*endColumn = end;
	}

	return true;
}

/**
 * Returns the identifier near the cursor.
 * @param viewer the text viewer
 * @param[out] startPosition the start of the identifier. can be @c null if not needed
 * @param[out] endPosition the end of the identifier. can be @c null if not needed
 * @return false if the identifier is not found (in this case, the values of the output parameters are undefined)
 * @see #getNearestIdentifier
 */
bool source::getPointedIdentifier(const TextViewer& viewer, Position* startPosition, Position* endPosition) {
	if(viewer.isWindow()) {
		POINT cursorPoint;
		::GetCursorPos(&cursorPoint);
		viewer.screenToClient(cursorPoint);
		const Position cursor = viewer.characterForClientXY(cursorPoint, LineLayout::LEADING);
		if(source::getNearestIdentifier(viewer.document(), cursor,
				(startPosition != 0) ? &startPosition->column : 0, (endPosition != 0) ? &endPosition->column : 0)) {
			if(startPosition != 0)
				startPosition->line = cursor.line;
			if(endPosition != 0)
				endPosition->line = cursor.line;
			return true;
		}
	}
	return false;
}
