/**
 * @file viewer.cpp
 * @author exeal
 * @date 2003-2006 (was EditView.cpp and EditViewWindowMessages.cpp)
 * @date 2006-2009
 * @see user-input.cpp
 */

#include <ascension/viewer.hpp>
#include <ascension/rules.hpp>
#include <ascension/text-editor.hpp>	// texteditor.commands.*, texteditor.Session
#include <limits>	// std.numeric_limit
#include <msctf.h>
#include <manah/win32/ui/wait-cursor.hpp>
#ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY
#include <manah/com/dispatch-impl.hpp>
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
using namespace manah;
using namespace std;

#pragma comment(lib, "version.lib")

#ifdef _DEBUG
bool DIAGNOSE_INHERENT_DRAWING = false;	// 余計な描画を行っていないか診断するフラグ
//#define TRACE_DRAWING_STRING	// テキスト (代替グリフ以外) のレンダリングをトレース
#endif // _DEBUG


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
			manah::com::IUnknownImpl<
				manah::typelist::Cat<MANAH_INTERFACE_SIGNATURE(IAccessible),
				manah::typelist::Cat<MANAH_INTERFACE_SIGNATURE(IDispatch),
				manah::typelist::Cat<MANAH_INTERFACE_SIGNATURE(IOleWindow)> > >,
				manah::com::NoReferenceCounting
			>,
			manah::com::ole::TypeInformationFromRegistry<&LIBID_Accessibility, &IID_IAccessible>
		> {
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
	// IAccessible
	STDMETHODIMP get_accParent(IDispatch** ppdispParent);
	STDMETHODIMP get_accChildCount(long* pcountChildren);
	STDMETHODIMP get_accChild(VARIANT varChild, IDispatch** ppdispChild);
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
	com::ComPtr<IAccessible> defaultServer_;
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

// local helpers
namespace {
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
#endif // WM_THEMECHANGED
	MANAH_WINDOW_MESSAGE_ENTRY(WM_TIMER)
#ifdef WM_UNICHAR
	MANAH_WINDOW_MESSAGE_ENTRY(WM_UNICHAR)
#endif // WM_UNICHAR
	MANAH_WINDOW_MESSAGE_ENTRY(WM_VSCROLL)
#ifdef WM_XBUTTONDBLCLK
	MANAH_WINDOW_MESSAGE_ENTRY(WM_XBUTTONDBLCLK)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_XBUTTONDOWN)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_XBUTTONUP)
#endif // WM_XBUTTONDBLCLK
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
TextViewer::TextViewer(const TextViewer& rhs) : win32::ui::CustomControl<TextViewer>(0), presentation_(rhs.presentation_), tipText_(0)
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
	check();
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
	if(!win32::ui::CustomControl<TextViewer>::create(parent, rect, 0, style, exStyle))
		return false;

	scrollInfo_.updateVertical(*this);
	updateScrollBars();

	// create the tooltip belongs to the window
	toolTip_ = ::CreateWindowExW(
		WS_EX_TOOLWINDOW | WS_EX_TOPMOST, TOOLTIPS_CLASSW, 0, WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, use(), 0,
		reinterpret_cast<HINSTANCE>(static_cast<HANDLE_PTR>(::GetWindowLongPtr(get(), GWLP_HINSTANCE))), 0);
	if(toolTip_ != 0) {
		win32::AutoZeroSize<TOOLINFOW> ti;
		RECT margins = {1, 1, 1, 1};
		ti.hwnd = get();
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
	vrc.lineNumbers.textColor = Colors(Color(0x00, 0x80, 0x80), Color(0xff, 0xff, 0xff));
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
	LexicalPartitioner* p = new LexicalPartitioner();
	p->setRules(rules, MANAH_ENDOF(rules));
	document().setPartitioner(auto_ptr<DocumentPartitioner>(p));

	PresentationReconstructor* pr = new PresentationReconstructor(presentation());

	// JSDoc syntax highlight test
	static const Char JSDOC_ATTRIBUTES[] = L"@addon @argument @author @base @class @constructor @deprecated @exception @exec @extends"
		L" @fileoverview @final @ignore @link @member @param @private @requires @return @returns @see @throws @type @version";
	{
		auto_ptr<const WordRule> jsdocAttributes(new WordRule(220, JSDOC_ATTRIBUTES, MANAH_ENDOF(JSDOC_ATTRIBUTES) - 1, L' ', true));
		auto_ptr<LexicalTokenScanner> scanner(new LexicalTokenScanner(JS_MULTILINE_DOC_COMMENT));
		scanner->addWordRule(jsdocAttributes);
		scanner->addRule(auto_ptr<Rule>(new URIRule(219, URIDetector::defaultIANAURIInstance(), false)));
		map<Token::ID, const TextStyle> jsdocStyles;
		jsdocStyles.insert(make_pair(Token::DEFAULT_TOKEN, TextStyle(Colors(Color(0x00, 0x80, 0x00)))));
		jsdocStyles.insert(make_pair(219, TextStyle(Colors(Color(0x00, 0x80, 0x00)), false, false, false, SOLID_UNDERLINE)));
		jsdocStyles.insert(make_pair(220, TextStyle(Colors(Color(0x00, 0x80, 0x00)), true)));
		auto_ptr<IPartitionPresentationReconstructor> ppr(
			new LexicalPartitionPresentationReconstructor(document(), auto_ptr<ITokenScanner>(scanner.release()), jsdocStyles));
		pr->setPartitionReconstructor(JS_MULTILINE_DOC_COMMENT, ppr);
	}

	// JavaScript syntax highlight test
	static const Char JS_KEYWORDS[] = L"Infinity break case catch continue default delete do else false finally for function"
		L" if in instanceof new null return switch this throw true try typeof undefined var void while with";
	static const Char JS_FUTURE_KEYWORDS[] = L"abstract boolean byte char class double enum extends final float goto"
		L" implements int interface long native package private protected public short static super synchronized throws transient volatile";
	{
		auto_ptr<const WordRule> jsKeywords(new WordRule(221, JS_KEYWORDS, MANAH_ENDOF(JS_KEYWORDS) - 1, L' ', true));
		auto_ptr<const WordRule> jsFutureKeywords(new WordRule(222, JS_FUTURE_KEYWORDS, MANAH_ENDOF(JS_FUTURE_KEYWORDS) - 1, L' ', true));
		auto_ptr<LexicalTokenScanner> scanner(new LexicalTokenScanner(DEFAULT_CONTENT_TYPE));
		scanner->addWordRule(jsKeywords);
		scanner->addWordRule(jsFutureKeywords);
		scanner->addRule(auto_ptr<const Rule>(new NumberRule(223)));
		map<Token::ID, const TextStyle> jsStyles;
		jsStyles.insert(make_pair(Token::DEFAULT_TOKEN, TextStyle()));
		jsStyles.insert(make_pair(221, TextStyle(Colors(Color(0x00, 0x00, 0xff)))));
		jsStyles.insert(make_pair(222, TextStyle(Colors(Color(0x00, 0x00, 0xff)), false, false, false, DASHED_UNDERLINE)));
		jsStyles.insert(make_pair(223, TextStyle(Colors(Color(0x80, 0x00, 0x00)))));
		pr->setPartitionReconstructor(DEFAULT_CONTENT_TYPE,
			auto_ptr<IPartitionPresentationReconstructor>(new LexicalPartitionPresentationReconstructor(document(),
				auto_ptr<ITokenScanner>(scanner.release()), jsStyles)));
	}

	// other contents
	pr->setPartitionReconstructor(JS_MULTILINE_COMMENT, auto_ptr<IPartitionPresentationReconstructor>(
		new SingleStyledPartitionPresentationReconstructor(TextStyle(Colors(Color(0x00, 0x80, 0x00))))));
	pr->setPartitionReconstructor(JS_SINGLELINE_COMMENT, auto_ptr<IPartitionPresentationReconstructor>(
		new SingleStyledPartitionPresentationReconstructor(TextStyle(Colors(Color(0x00, 0x80, 0x00))))));
	pr->setPartitionReconstructor(JS_DQ_STRING, auto_ptr<IPartitionPresentationReconstructor>(
		new SingleStyledPartitionPresentationReconstructor(TextStyle(Colors(Color(0x00, 0x00, 0x80))))));
	pr->setPartitionReconstructor(JS_SQ_STRING, auto_ptr<IPartitionPresentationReconstructor>(
		new SingleStyledPartitionPresentationReconstructor(TextStyle(Colors(Color(0x00, 0x00, 0x80))))));
	new CurrentLineHighlighter(*caret_, Colors(Color(), Color::fromCOLORREF(::GetSysColor(COLOR_INFOBK))));

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
	setContentAssistant(auto_ptr<contentassist::IContentAssistant>(ca));
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
		utils::closeCompletionProposalsPopup(*this);
		caret_->moveTo(resultPosition);
	}
}

/**
 * Additionally draws the indicator margin on the vertical ruler.
 * @param line the line number
 * @param dc the device context
 * @param rect the rectangle to draw
 */
void TextViewer::drawIndicatorMargin(length_t /* line */, win32::gdi::DC& /* dc */, const RECT& /* rect */) {
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
	check();
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
	win32::Rect clientRect;
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
	check();
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

/// Hides the tool tip.
void TextViewer::hideToolTip() {
	check();
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
	check();
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
	check();
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
void TextViewer::onPaint(win32::gdi::PaintDC& dc) {
	if(isFrozen())	// skip if frozen
		return;
	else if(toBoolean(::IsRectEmpty(&dc.paintStruct().rcPaint)))	// skip if the region to paint is empty
		return;

	const Document& doc = document();
	RECT clientRect;
	getClientRect(clientRect);

//	Timer tm(L"onPaint");

	const length_t lines = doc.numberOfLines();
	const RECT& paintRect = dc.paintStruct().rcPaint;
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
			renderer_->renderLine(line, dc, getDisplayXOffset(line), y, dc.paintStruct().rcPaint, lineRect, &selection);
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

/// @see WM_SIZE
void TextViewer::onSize(UINT type, int, int) {
	utils::closeCompletionProposalsPopup(*this);
	if(type == SIZE_MINIMIZED)
		return;

	// notify the tooltip
	win32::AutoZeroSize<TOOLINFOW> ti;
	RECT viewRect;
	getClientRect(viewRect);
	ti.hwnd = get();
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
		invalidateRect(0, false);	// hmm...
	}
}

/// @see WM_STYLECHANGED
void TextViewer::onStyleChanged(int type, const STYLESTRUCT& style) {
	if(type == GWL_EXSTYLE
			&& (((style.styleOld ^ style.styleNew) & (WS_EX_RIGHT | WS_EX_RTLREADING)) != 0)) {
		// synchronize the resentation with the window style
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

/// @see WM_SYSCOLORCHANGE
void TextViewer::onSysColorChange() {
//	if(this == originalView_)
//		presentation_.updateSystemColors();
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
			setTimer(TIMERID_AUTOSCROLL, 500 / static_cast<uint>((pow(2.0f, abs(yScrollDegree) / 2))), 0);
			::SetCursor(AutoScrollOriginMark::cursorForScrolling(
				(yScrollDegree > 0) ? AutoScrollOriginMark::CURSOR_DOWNWARD : AutoScrollOriginMark::CURSOR_UPWARD));
		} else {
			setTimer(TIMERID_AUTOSCROLL, 300, 0);
			::SetCursor(AutoScrollOriginMark::cursorForScrolling(AutoScrollOriginMark::CURSOR_NEUTRAL));
		}
	}
}

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
			CharacterDeletionCommand(*this, Direction::FORWARD)();
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
			com::ComPtr<IAccessible> acc;
			if(SUCCEEDED(accessibleObject(*acc.initialize())) && accLib.isAvailable())
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
		PasteCommand(*this, false)();
		handled = true;
		return 0L;
#endif /* ASCENSION_HANDLE_STANDARD_EDIT_CONTROL_MESSAGES */
	case WM_SETTEXT:
		EntireDocumentSelectionCreationCommand(*this)();
		caret().replaceSelection(String(reinterpret_cast<const wchar_t*>(lParam)), false);
		handled = true;
		return 0L;
#ifdef ASCENSION_HANDLE_STANDARD_EDIT_CONTROL_MESSAGES
	case WM_UNDO:
		UndoCommand(*this, false)();
		handled = true;
		return 0L;
#endif /* ASCENSION_HANDLE_STANDARD_EDIT_CONTROL_MESSAGES */
	}

	return BaseControl::preTranslateWindowMessage(message, wParam, lParam, handled);
}

/// Recreates and shows the caret. If the viewer does not have focus, nothing heppen.
void TextViewer::recreateCaret() {
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

	if(caretShape_.bitmap.get() != 0 && caretShape_.bitmap->get() != 0) {
		createCaret(caretShape_.bitmap->get(), 0, 0);
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
	check();

	if(isFrozen()) {
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
		win32::DumpContext() << L"@TextViewer.redrawLines invalidates lines ["
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
	check();

	// preprocess and update the scroll bars
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

	// scroll
	RECT clipRect, clientRect;
	const RECT margins = textAreaMargins();
	getClientRect(clientRect);
	clipRect = clientRect;
	clipRect.top += margins.top;
	clipRect.bottom -= margins.bottom;
	if(static_cast<uint>(abs(dy)) >= numberOfVisibleLines())
		invalidateRect(&clipRect, false);	// redraw all if the amount of the scroll is over a page
	else if(dx == 0)	// only vertical
		scrollEx(0, -dy * scrollRate(false) * renderer_->linePitch(), 0, &clipRect, 0, 0, SW_INVALIDATE);
	else {	// process the leading margin and the edit region independently
		// scroll the edit region
		clipRect.left += margins.left;
		clipRect.right -= margins.right;
		if(static_cast<uint>(abs(dx)) >= numberOfVisibleColumns())
			invalidateRect(&clipRect, false);	// redraw all if the amount of the scroll is over a page
		else
			scrollEx(-dx * scrollRate(true) * renderer_->averageCharacterWidth(),
				-dy * scrollRate(false) * renderer_->linePitch(), 0, &clipRect, 0, 0, SW_INVALIDATE);
		// scroll the vertical ruler
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

	// postprocess
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
	check();
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
	check();
	if(line >= document().numberOfLines())
		throw BadPositionException(Position(line, 0));
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
	check();

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
	check();
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

/// Updates the scroll information.
void TextViewer::updateScrollBars() {
	check();
	if(renderer_.get() == 0)
		return;

#define ASCENSION_GET_SCROLL_MINIMUM(s)	(s.maximum/* * s.rate*/ - s.pageSize + 1)

	// about horizontal scroll bar
	bool wasNeededScrollbar = ASCENSION_GET_SCROLL_MINIMUM(scrollInfo_.horizontal) > 0;
	// scroll to leftmost/rightmost before the scroll bar vanishes
	long minimum = ASCENSION_GET_SCROLL_MINIMUM(scrollInfo_.horizontal);
	if(wasNeededScrollbar && minimum <= 0) {
		scrollInfo_.horizontal.position = 0;
		if(!isFrozen()) {
			invalidateRect(0, false);
			updateCaretPosition();
		}
	} else if(scrollInfo_.horizontal.position > minimum)
		scrollTo(minimum, -1, true);
	assert(ASCENSION_GET_SCROLL_MINIMUM(scrollInfo_.horizontal) > 0 || scrollInfo_.horizontal.position == 0);
	if(!isFrozen()) {
		win32::AutoZeroSize<SCROLLINFO> scroll;
		scroll.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
		scroll.nMax = configuration_.lineWrap.wrapsAtWindowEdge() ? 0 : scrollInfo_.horizontal.maximum;
		scroll.nPage = scrollInfo_.horizontal.pageSize;
		scroll.nPos = scrollInfo_.horizontal.position;
		setScrollInformation(SB_HORZ, scroll, true);
	}

	// about vertical scroll bar
	wasNeededScrollbar = ASCENSION_GET_SCROLL_MINIMUM(scrollInfo_.vertical) > 0;
	minimum = ASCENSION_GET_SCROLL_MINIMUM(scrollInfo_.vertical);
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
	assert(ASCENSION_GET_SCROLL_MINIMUM(scrollInfo_.vertical) > 0 || scrollInfo_.vertical.position == 0);
	if(!isFrozen()) {
		win32::AutoZeroSize<SCROLLINFO> scroll;
		scroll.fMask = SIF_DISABLENOSCROLL | SIF_PAGE | SIF_POS | SIF_RANGE;
		scroll.nMax = scrollInfo_.vertical.maximum;
		scroll.nPage = scrollInfo_.vertical.pageSize;
		scroll.nPos = scrollInfo_.vertical.position;
		setScrollInformation(SB_VERT, scroll, true);
	}

	scrollInfo_.changed = isFrozen();

#undef ASCENSION_GET_SCROLL_MINIMUM
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
	accLib.createStdAccessibleObject(view.use(), OBJID_CLIENT, IID_IAccessible, defaultServer_.initializePPV());
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
	accLib.notifyWinEvent(EVENT_OBJECT_VALUECHANGE, view_.use(), OBJID_CLIENT, CHILDID_SELF);
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
		return accLib.accessibleObjectFromWindow(view_.use(), OBJID_WINDOW, IID_IAccessible, reinterpret_cast<void**>(ppdispParent));
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
	if(view_.getTop()->use() == ::GetActiveWindow())
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
	*phwnd = view_.get();
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
	view_.caret().replaceSelection((szValue != 0) ? szValue : L"");
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
auto_ptr<win32::gdi::DC> TextViewer::Renderer::getDeviceContext() const {
	return auto_ptr<win32::gdi::DC>(viewer_.isWindow() ?
		new win32::gdi::ClientDC(const_cast<TextViewer&>(viewer_).getDC()) : new win32::gdi::ScreenDC());
}

/// @see layout#ILayoutInformationProvider#getLayoutSettings
const LayoutSettings& TextViewer::Renderer::getLayoutSettings() const /*throw()*/ {
	return viewer_.configuration();
}

/// @see layout#ILayoutInformationProvider#getWidth
int TextViewer::Renderer::getWidth() const /*throw()*/ {
	const LineWrapConfiguration& lwc = viewer_.configuration().lineWrap;
	if(!lwc.wraps()) {
		win32::AutoZeroSize<SCROLLINFO> si;
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
		win32::Rect clientRect;
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
	if(enablesDoubleBuffering_ && memoryBitmap_.get() != 0)
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
void DefaultCaretShaper::getCaretShape(auto_ptr<win32::gdi::Bitmap>&, SIZE& solidSize, Orientation& orientation) /*throw()*/ {
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
	inline BITMAPINFO* prepareCaretBitmap(const win32::gdi::DC& dc, ushort width, ushort height) {
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
	inline void createSolidCaretBitmap(win32::gdi::Bitmap& bitmap, ushort width, ushort height, const RGBQUAD& color) {
		win32::gdi::ScreenDC dc;
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
	inline void createRTLCaretBitmap(win32::gdi::Bitmap& bitmap, ushort height, bool bold, const RGBQUAD& color) {
		win32::gdi::ScreenDC dc;
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
	inline void createTISCaretBitmap(win32::gdi::Bitmap& bitmap, ushort height, bool bold, const RGBQUAD& color) {
		win32::gdi::ScreenDC dc;
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
void LocaleSensitiveCaretShaper::getCaretShape(
		auto_ptr<win32::gdi::Bitmap>& bitmap, SIZE& solidSize, Orientation& orientation) /*throw()*/ {
	const Caret& caret = updater_->textViewer().caret();
	const bool overtype = caret.isOvertypeMode() && caret.isSelectionEmpty();

	if(!overtype) {
		solidSize.cx = bold_ ? 2 : 1;	// this ignores the system setting...
		solidSize.cy = updater_->textViewer().textRenderer().lineHeight();
	} else	// use the width of the glyph when overtype mode
		getCurrentCharacterSize(updater_->textViewer(), solidSize);
	orientation = LEFT_TO_RIGHT;

	HIMC imc = ::ImmGetContext(updater_->textViewer().get());
	const bool imeOpened = toBoolean(::ImmGetOpenStatus(imc));
	::ImmReleaseContext(updater_->textViewer().get(), imc);
	if(imeOpened) {	// CJK and IME is open
		static const RGBQUAD red = {0xff, 0xff, 0x80, 0x00};
		bitmap.reset(new win32::gdi::Bitmap);
		createSolidCaretBitmap(*bitmap.get(), static_cast<ushort>(solidSize.cx), static_cast<ushort>(solidSize.cy), red);
	} else if(!overtype && solidSize.cy > 3) {
		static const RGBQUAD black = {0xff, 0xff, 0xff, 0x00};
		const WORD langID = PRIMARYLANGID(LOWORD(::GetKeyboardLayout(::GetCurrentThreadId())));
		if(isRTLLanguage(langID)) {	// RTL
			bitmap.reset(new win32::gdi::Bitmap);
			createRTLCaretBitmap(*bitmap.get(), static_cast<ushort>(solidSize.cy), bold_, black);
			orientation = RIGHT_TO_LEFT;
		} else if(isTISLanguage(langID)) {	// Thai relations
			bitmap.reset(new win32::gdi::Bitmap);
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
CurrentLineHighlighter::CurrentLineHighlighter(Caret& caret, const Colors& color) : caret_(&caret), color_(color) {
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


// ascension.viewers.utils //////////////////////////////////////////////////

/// Closes the opened completion proposals popup immediately.
void utils::closeCompletionProposalsPopup(TextViewer& viewer) /*throw()*/ {
	if(contentassist::IContentAssistant* ca = viewer.contentAssistant()) {
		if(contentassist::IContentAssistant::ICompletionProposalsUI* cpui = ca->getCompletionProposalsUI())
			cpui->close();
	}
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
