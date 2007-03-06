/**
 * @file viewer.cpp
 * @author exeal
 * @date 2003-2006 (was EditView.cpp and EditViewWindowMessages.cpp)
 * @date 2006-2007
 */

#include "stdafx.h"
#include "viewer.hpp"
#include "rules.hpp"
#include "text-editor.hpp"
#include "../../manah/win32/ui/menu.hpp"
#include <limits>	// std::numeric_limit
#include <zmouse.h>
#include <msctf.h>
#include "../../manah/win32/ui/wait-cursor.hpp"
#ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY
#include "../../manah/com/dispatch-impl.hpp"
#endif /* !ASCENSION_NO_ACTIVE_ACCESSIBILITY */
#ifndef ASCENSION_NO_TEXT_SERVICES_FRAMEWORK
#include <Textstor.h>
#endif /* !ASCENSION_NO_TEXT_SERVICES_FRAMEWORK */

using namespace ascension;
using namespace ascension::viewers;
using namespace ascension::presentation;
using namespace ascension::text;
using namespace manah::win32;
using namespace manah::win32::gdi;
#ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY
using namespace manah::com;
#endif /* !ASCENSION_NO_ACTIVE_ACCESSIBILITY */
using namespace std;
using manah::AutoBuffer;
using manah::com::ole::TextDataObject;

#pragma comment(lib, "version.lib")

#ifdef _DEBUG
bool DIAGNOSE_INHERENT_DRAWING = false;	// 余計な描画を行っていないか診断するフラグ
//#define TRACE_DRAWING_STRING	// テキスト (代替グリフ以外) のレンダリングをトレース
#endif /* _DEBUG */


namespace {
	// 選択作成後にタイマーで監視する時間間隔
	const uint SELECTION_OBSERVATION_INTERVAL = 100;
	// すぐ下で使う
	BOOL CALLBACK enumResLangProc(HMODULE, const WCHAR*, const WCHAR* name, WORD langID, LONG_PTR param) {
		if(name == 0)
			return false;
		else if(langID != MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US))
			*reinterpret_cast<LANGID*>(param) = langID;
		return true;
	}
} // namespace @0

LANGID ascension::getUserDefaultUILanguage() throw() {
	// 参考 (いずれも Global Dev)
	// Writing Win32 Multilingual User Interface Applications (http://www.microsoft.com/globaldev/handson/dev/muiapp.mspx)
	// Ask Dr. International Column #9 (http://www.microsoft.com/globaldev/drintl/columns/009/default.mspx#EPD)
	static LANGID id = 0;
	if(id != 0)
		return id;
	id = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
	::OSVERSIONINFOW version;
	version.dwOSVersionInfoSize = sizeof(::OSVERSIONINFOW);
	::GetVersionEx(&version);
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
		::EnumResourceLanguagesW(dll, RT_VERSION, MAKEINTRESOURCE(1), enumResLangProc, reinterpret_cast<LONG_PTR>(&id));
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


// Colors ///////////////////////////////////////////////////////////////////

const Colors Colors::STANDARD;


// LineStyle ////////////////////////////////////////////////////////////////

const LineStyle LineStyle::NULL_STYLE = {0, 0};


// TextViewer::CloneIterator ////////////////////////////////////////////////

class TextViewer::CloneIterator {
public:
	explicit CloneIterator(TextViewer& view) throw() :
		original_(view.originalView_), it_(view.originalView_->clones_->begin()), isHead_(true) {}
	TextViewer& get() const throw() {assert(!isEnd()); return isHead_ ? *original_ : **it_;}
	bool isEnd() const throw() {return !isHead_ && it_ == original_->clones_->end();}
	void next() {
		if(isHead_) isHead_ = false;
		else if(it_ != original_->clones_->end()) ++it_;
	}
	TextViewer& operator*() const throw() {return get();}
	TextViewer* operator->() throw() {return &get();}
private:
	TextViewer* original_;
	std::set<TextViewer*>::iterator it_;
	bool isHead_;
};


#ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY

// TextViewer::AccessibleProxy //////////////////////////////////////////////

/**
 * @c TextViewer#AccessibleProxy is proxy object for @c IAccessible interface of @c TextViewer instance.
 * @see TextViewer#getAccessibleObject, ASCENSION_NO_ACTIVE_ACCESSIBILITY
 */
class TextViewer::AccessibleProxy :
		virtual public IDocumentListener,
		public manah::com::ole::IDispatchImpl<
			IAccessible, manah::com::ole::RegTypeLibTypeInfoHolder<&LIBID_Accessibility, &IID_IAccessible>
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
public:
	// コンストラクタ
	AccessibleProxy(TextViewer& view);
	// メソッド
	void	dispose();
	// IUnknown
	IMPLEMENT_UNKNOWN_MULTI_THREADED()
	BEGIN_INTERFACE_TABLE()
		IMPLEMENTS_LEFTMOST_INTERFACE(IAccessible)
		IMPLEMENTS_INTERFACE(IDispatch)
	END_INTERFACE_TABLE()
	// IAccessible
	STDMETHODIMP	get_accParent(IDispatch** ppdispParent);
	STDMETHODIMP	get_accChildCount(long* pcountChildren);
	STDMETHODIMP	get_accChild(VARIANT varChild, IDispatch** ppdispChild);
	STDMETHODIMP	get_accName(VARIANT varChild, BSTR* pszName);
	STDMETHODIMP	get_accValue(VARIANT varChild, BSTR* pszValue);
	STDMETHODIMP	get_accDescription(VARIANT varChild, BSTR* pszDescription);
	STDMETHODIMP	get_accRole(VARIANT varChild, VARIANT* pvarRole);
	STDMETHODIMP	get_accState(VARIANT varChild, VARIANT* pvarState);
	STDMETHODIMP	get_accHelp(VARIANT varChild, BSTR* pszHelp);
	STDMETHODIMP	get_accHelpTopic(BSTR* pszHelpFile, VARIANT varChild, long* pidTopic);
	STDMETHODIMP	get_accKeyboardShortcut(VARIANT varChild, BSTR* pszKeyboardShortcut);
	STDMETHODIMP	get_accFocus(VARIANT* pvarChild);
	STDMETHODIMP	get_accSelection(VARIANT* pvarChildren);
	STDMETHODIMP	get_accDefaultAction(VARIANT varChild, BSTR* pszDefaultAction);
	STDMETHODIMP	accSelect(long flagsSelect, VARIANT varChild);
	STDMETHODIMP	accLocation(long* pxLeft, long* pyTop, long* pcxWidth, long* pcyHeight, VARIANT varChild);
	STDMETHODIMP	accNavigate(long navDir, VARIANT varStart, VARIANT* pvarEndUpAt);
	STDMETHODIMP	accHitTest(long xLeft, long yTop, VARIANT* pvarChild);
	STDMETHODIMP	accDoDefaultAction(VARIANT varChild);
	STDMETHODIMP	put_accName(VARIANT varChild, BSTR szName);
	STDMETHODIMP	put_accValue(VARIANT varChild, BSTR szValue);
private:
	// IDocumentListener
	void	documentAboutToBeChanged(const Document& document);
	void	documentChanged(const Document& document, const DocumentChange& change);
private:
	TextViewer& view_;
	bool available_;
	ComQIPtr<IAccessible> defaultServer_;
//	enum {CHILDID_SELECTION = 1};
};

namespace {
	class AccLib {
	public:
		AccLib() throw() : oleaccDLL_(::LoadLibraryA("oleacc.dll")), user32DLL_(::LoadLibraryA("user32.dll")) {
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
		~AccLib() throw() {::FreeLibrary(oleaccDLL_); ::FreeLibrary(user32DLL_);}
		bool isAvailable() const throw() {return oleaccDLL_ != 0;}
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

#endif /* !ASCENSION_NO_ACTIVE_ACCESSIBILITY */

#ifndef ASCENSION_NO_TEXT_SERVICES_FRAMEWORK
namespace {
	class TextServiceApplicationAdapter : virtual public ITextStoreACP, virtual public ITextStoreAnchor {
	public:
		// コンストラクタ
		explicit TextServiceApplicationAdapter(Viewer& view);
		// IUnknown
		IMPLEMENT_UNKNOWN_NO_REF_COUNT()
		BEGIN_INTERFACE_TABLE()
			IMPLEMENTS_LEFTMOST_INTERFACE(ITextStoreACP)
			IMPLEMENTS_INTERFACE(ITextStoreAnchor)
		END_INTERFACE_TABLE()
		// ITextStoreACP
		STDMETHODIMP	AdviseSink(REFIID riid, IUnknown* punk, DWORD dwMask);
		STDMETHODIMP	UnadviseSink(IUnknown* punk);
		STDMETHODIMP	RequestLock(DWORD dwLockFlags, HRESULT* phrSession);
		STDMETHODIMP	GetStatus(TS_STATUS* pdcs);
		STDMETHODIMP	QueryInsert(LONG acpTestStart, LONG acpTestEnd, ULONG cch, LONG* pacpResultStart, LONG* pacpResultEnd);
		STDMETHODIMP	GetSelection(ULONG ulIndex, ULONG ulCount, TS_SELECTION_ACP* pSelection, ULONG* pcFetched);
		STDMETHODIMP	SetSelection(ULONG ulCount, const TS_SELECTION_ACP* pSelection);
		STDMETHODIMP	GetText(LONG acpStart, LONG acpEnd,
							WCHAR* pchPlain, ULONG cchPlainReq, ULONG* pcchPlainRet,
							TS_RUNINFO* prgRunInfo, ULONG cRunInfoReq, ULONG* pcRunInfoRet, LONG* pacpNext);
		STDMETHODIMP	SetText(DWORD dwFlags, LONG acpStart, LONG acpEnd, const WCHAR* pchText, ULONG cch, TS_TEXTCHANGE* pChange);
		STDMETHODIMP	GetFormattedText(LONG acpStart, LONG acpEnd, IDataObject** ppDataObject);
		STDMETHODIMP	GetEmbedded(LONG acpPos, REFGUID rguidService, REFIID riid, IUnknown** ppunk);
		STDMETHODIMP	QueryInsertEmbedded(const GUID* pguidService, const FORMATETC* pFormatEtc, BOOL* pfInsertable);
		STDMETHODIMP	InsertEmbedded(DWORD dwFlags, LONG acpStart, LONG acpEnd, IDataObject* pDataObject, TS_TEXTCHANGE* pChange);
		STDMETHODIMP	InsertTextAtSelection(DWORD dwFlags,
							const WCHAR* pchText, ULONG cch, LONG* pacpStart, LONG* pacpEnd, TS_TEXTCHANGE* pChange);
		STDMETHODIMP	InsertEmbeddedAtSelection(DWORD dwFlags,
							IDataObject* pDataObject, LONG* pacpStart, LONG* pacpEnd, TS_TEXTCHANGE* pChange);
		STDMETHODIMP	RequestSupportedAttrs(DWORD dwFlags, ULONG cFilterAttrs, const TS_ATTRID* paFilterAttrs);
		STDMETHODIMP	RequestAttrsAtPosition(LONG acpPos, ULONG cFilterAttrs, const TS_ATTRID* paFilterAttrs, DWORD dwFlags);
		STDMETHODIMP	RequestAttrsTransitioningAtPosition(LONG acpPos, ULONG cFilterAttrs, const TS_ATTRID* paFilterAttrs, DWORD dwFlags);
		STDMETHODIMP	FindNextAttrTransition(LONG acpStart, LONG acpHalt,
							ULONG cFilterAttrs, const TS_ATTRID* paFilterAttrs, DWORD dwFlags,
							LONG* pacpNext, BOOL* pfFound, LONG* plFoundOffset);
		STDMETHODIMP	RetrieveRequestedAttrs(ULONG ulCount, TS_ATTRVAL* paAttrVals, ULONG* pcFetched);
		STDMETHODIMP	GetEndACP(LONG* pacp);
		STDMETHODIMP	GetActiveView(TsViewCookie* pvcView);
		STDMETHODIMP	GetACPFromPoint(TsViewCookie vcView, const POINT* ptScreen, DWORD dwFlags, LONG* pacp);
		STDMETHODIMP	GetTextExt(TsViewCookie vcView, LONG acpStart, LONG acpEnd, RECT* prc, BOOL* pfClipped);
		STDMETHODIMP	GetScreenExt(TsViewCookie vcView, RECT* prc);
		STDMETHODIMP	GetWnd(TsViewCookie vcView, HWND* phwnd);
	};
} // namespace @0
#endif /* !ASCENSION_NO_TEXT_SERVICES_FRAMEWORK */


// TextViewer ///////////////////////////////////////////////////////////////

/**
 * @class ascension::viewers::TextViewer The view of Ascension framework.
 *
 * @c TextViewer displays the content of the document, manipulates the document with the caret and
 * selection, and provides other visual presentations.
 *
 * <h3>双方向テキスト関連のウィンドウスタイル</h3>
 *
 * テキストを右寄せで表示するための @c WS_EX_RIGHT 、右から左に表示するための @c WS_EX_RTLREADING
 * はいずれも無視される。これらの設定には代わりに @c LayoutSettings の該当メンバを使用しなければならない
 *
 * また @c WS_EX_LAYOUTRTL も使用できない。このスタイルを設定した場合の動作は未定義である
 *
 * 垂直スクロールバーを左側に表示するにはクライアントが @c WS_EX_LEFTSCROLLBAR を設定しなければならない
 *
 * これらの設定を一括して変更する場合 @c #setTextDirection を使うことができる
 *
 * 垂直ルーラ (インジケータマージンと行番号) の位置はテキストが左寄せであれば左端、
 * 右寄せであれば右端になる
 *
 * @c TextViewer provides two methods #freeze and #unfreeze to freeze of the screen of the window.
 * While the viewer is frozen, the window does not redraw the content.
 * If the document is reset (@c text#Document#resetContent), the viewer is unfreezed.
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

#define EXTEND_SELECTION()																			\
	const Position dest = getCharacterForClientXY(pt, true);										\
	if(leftDownMode_ == LDM_SELECTION_LINE || leftDownMode_ == LDM_SELECTION_WORD) {				\
		const HitTestResult htr = hitTest(pt);														\
		if(leftDownMode_ == LDM_SELECTION_LINE && htr != INDICATOR_MARGIN && htr != LINE_NUMBERS) {	\
			killTimer(TIMERID_EXPANDLINESELECTION);													\
			setTimer(TIMERID_EXPANDSELECTION, 50, 0);												\
			leftDownMode_ = LDM_SELECTION_CHARACTER;												\
			getCaret().restoreSelectionMode();														\
		}																							\
	} else																							\
		assert(leftDownMode_ == LDM_SELECTION_CHARACTER);											\
	getCaret().extendSelection(dest)

#define RESTORE_HIDDEN_CURSOR()				\
	if(modeState_.cursorVanished) {			\
		modeState_.cursorVanished = false;	\
		::ShowCursor(true);					\
		releaseCapture();					\
	}

// local helpers
namespace {
	inline void abortIncrementalSearch(TextViewer& viewer) throw() {
		if(texteditor::Session* session = viewer.getDocument().getSession()) {
			if(session->getIncrementalSearcher().isRunning())
				session->getIncrementalSearcher().abort();
		}
	}
	inline void endIncrementalSearch(TextViewer& viewer) throw() {
		if(texteditor::Session* session = viewer.getDocument().getSession()) {
			if(session->getIncrementalSearcher().isRunning())
				session->getIncrementalSearcher().end();
		}
	}
	inline void toggleOrientation(TextViewer& viewer) throw() {
		TextViewer::VerticalRulerConfiguration vrc = viewer.getVerticalRulerConfiguration();
		if(viewer.getConfiguration().orientation == LEFT_TO_RIGHT) {
			vrc.alignment = ALIGN_RIGHT;
			if(vrc.lineNumbers.alignment != ALIGN_AUTO)
				vrc.lineNumbers.alignment = ALIGN_LEFT;
			viewer.setConfiguration(0, &vrc);
			viewer.modifyStyleEx(WS_EX_LEFT | WS_EX_LTRREADING, WS_EX_RIGHT | WS_EX_RTLREADING | WS_EX_LEFTSCROLLBAR);
//			if(config.lineWrap.wrapsAtWindowEdge()) {
//				AutoZeroCB<::SCROLLINFO> scroll;
//				viewer.getScrollInfo(SB_HORZ, scroll);
//				viewer.setScrollInfo(SB_HORZ, scroll);
//			}
		} else {
			vrc.alignment = ALIGN_LEFT;
			if(vrc.lineNumbers.alignment != ALIGN_AUTO)
				vrc.lineNumbers.alignment = ALIGN_RIGHT;
			viewer.setConfiguration(0, &vrc);
			viewer.modifyStyleEx(WS_EX_RIGHT | WS_EX_RTLREADING, WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR);
//			if(config.lineWrap.wrapsAtWindowEdge()) {
//				AutoZeroCB<::SCROLLINFO> scroll;
//				viewer.getScrollInfo(SB_HORZ, scroll);
//				viewer.setScrollInfo(SB_HORZ, scroll);
//			}
		}
	}
} // namespace @0

/**
 * Constructor.
 * @param presentation the presentation
 */
TextViewer::TextViewer(Presentation& presentation) : presentation_(presentation), tipText_(0), autoScrollOriginMark_(0),
#ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY
		accessibleProxy_(0),
#endif /* !ASCENSION_NO_ACTIVE_ACCESSIBILITY */
		clones_(new set<TextViewer*>), imeCompositionActivated_(false),
#ifndef ASCENSION_NO_DOUBLE_BUFFERING
		lineBitmap_(0), oldLineBitmap_(0),
#endif /* !ASCENSION_NO_DOUBLE_BUFFERING */
		leftDownMode_(LDM_NONE), mouseOperationDisabledCount_(0) {
	caret_.reset(new Caret(*this));
	caret_->addListener(*this);
	originalView_ = this;
	dragging_ = new TextDataObject(*this);
	verticalRulerDrawer_.reset(new VerticalRulerDrawer(*this));

	static_cast<presentation::internal::ITextViewerCollection&>(presentation_).addTextViewer(*this);
	getDocument().addListener(*this);
	getDocument().addStateListener(*this);
	getDocument().addSequentialEditListener(*this);

	// renderer_ はウィンドウ作成時に構築する (コピコンも同様)
}

/// Copy-constructor.
TextViewer::TextViewer(const TextViewer& rhs) : ui::CustomControl<TextViewer>(rhs), presentation_(rhs.presentation_), tipText_(0)
#ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY
		, accessibleProxy_(0)
#endif /* !ASCENSION_NO_ACTIVE_ACCESSIBILITY */
#ifndef ASCENSION_NO_DOUBLE_BUFFERING
		, lineBitmap_(0), oldLineBitmap_(0)
#endif /* !ASCENSION_NO_DOUBLE_BUFFERING */
{
	// 非共有メンバは自分で作成。共有メンバはコピー
	caret_.reset(new Caret(*this));
	caret_->addListener(*this);
	dragging_ = new TextDataObject(*this);

	originalView_ = rhs.originalView_;
	originalView_->clones_->insert(this);
	verticalRulerDrawer_.reset(new VerticalRulerDrawer(*this));

	modeState_ = rhs.modeState_;

	imeCompositionActivated_ = false;
	leftDownMode_ = LDM_NONE;
	mouseOperationDisabledCount_ = 0;
	static_cast<presentation::internal::ITextViewerCollection&>(presentation_).addTextViewer(*this);
	getDocument().addListener(*this);
	getDocument().addStateListener(*this);
	getDocument().addSequentialEditListener(*this);
}

/// Destructor.
TextViewer::~TextViewer() {
	static_cast<presentation::internal::ITextViewerCollection&>(presentation_).removeTextViewer(*this);
	getDocument().removeListener(*this);
	getDocument().removeStateListener(*this);
	getDocument().removeSequentialEditListener(*this);
	renderer_->removeVisualLinesListener(*this);
	caret_->removeListener(*this);
	for(set<VisualPoint*>::iterator it = points_.begin(); it != points_.end(); ++it)
		(*it)->viewerDisposed();

	// 非共有データ
//	delete selection_;
	delete[] tipText_;
#ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY
	if(accessibleProxy_ != 0)
		accessibleProxy_->Release();
#endif /* !ASCENSION_NO_ACTIVE_ACCESSIBILITY */

	// 所有権
	if(originalView_ == this) {	// 自分が複製元
		if(clones_->empty())	// 自分が最後
			delete clones_;		// クローンのリストも破壊
		else {	// 自分の複製がまだ残っている
			TextViewer* newOriginal = *clones_->begin();	// 新しい複製元
			clones_->erase(clones_->begin());
			newOriginal->originalView_ = newOriginal;
			newOriginal->clones_ = clones_;
			for(set<TextViewer*>::iterator it = newOriginal->clones_->begin(); it != newOriginal->clones_->end(); ++it) {
				if(*it != newOriginal)
					(*it)->originalView_ = newOriginal;
			}
		}
	} else {	// オリジナルに死亡通知
		set<TextViewer*>::iterator it = originalView_->clones_->find(this);
		assert(it != originalView_->clones_->end());
		originalView_->clones_->erase(it);
	}
}

/// Starts the auto scroll.
void TextViewer::beginAutoScroll() {
	assertValidAsWindow();
	if(!hasFocus() || getDocument().getNumberOfLines() <= getNumberOfVisibleLines())
		return;

	::RECT rect;
	::POINT pt;
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

/**
 *	@brief 指定した仮想ポイントに達するのに行の末尾に追加する必要のある空白文字列を返す
 *
 *	行の終端がすでに仮想ポイントを超えている場合は空文字列を返す。
 *	矩形貼り付けやフリーカーソルの実装に使用
 *	@param line		行番号 (表示行)
 *	@param virtualX	行の左端からの位置
 *	@return			必要な空白文字列。水平タブ (U+0009) と半角空白 (U+0020) のみで構成される
 */
String TextViewer::calculateSpacesReachingVirtualPoint(length_t line, ulong virtualX) const {
	assertValidAsWindow();
	return L"";	// TODO: not implemented (or no-need?)
}

/// @see ICaretListener#caretMoved
void TextViewer::caretMoved(const Caret& self, const Region& oldRegion) {
	if(!isVisible())
		return;
	const Region newRegion = self.getSelectionRegion();
	bool changed = false;

	// キャレットの調整
	if(!isFrozen() && (hasFocus() /*|| completionWindow_->hasFocus()*/))
		updateCaretPosition();

	// 選択の強調表示の再描画
	if(self.isSelectionRectangle()) {	// 矩形選択の場合
		if(!oldRegion.isEmpty())
			redrawLines(oldRegion.getTop().line, oldRegion.getBottom().line);
		if(!newRegion.isEmpty())
			redrawLines(newRegion.getTop().line, newRegion.getBottom().line);
	} else if(newRegion != oldRegion) {	// 本当に範囲が変化した
		if(oldRegion.isEmpty()) {	// 元々選択が空だった場合
			if(!newRegion.isEmpty())	// 選択を作成した
				redrawLines(newRegion.getTop().line, newRegion.getBottom().line);
		} else {	// 元々選択があった場合
			if(newRegion.isEmpty()) {	// 選択を解除した
				redrawLines(oldRegion.getTop().line, oldRegion.getBottom().line);
				if(!isFrozen())
					update();
			} else if(oldRegion.getTop() == newRegion.getTop()) {	// 始点固定
				const length_t i[2] = {oldRegion.getBottom().line, newRegion.getBottom().line};
				redrawLines(min(i[0], i[1]), max(i[0], i[1]));
			} else if(oldRegion.getBottom() == newRegion.getBottom()) {	// 終点固定
				const length_t i[2] = {oldRegion.getTop().line, newRegion.getTop().line};
				redrawLines(min(i[0], i[1]), max(i[0], i[1]));
			} else {	// いずれも変化
				if((oldRegion.getTop().line >= newRegion.getTop().line && oldRegion.getTop().line <= newRegion.getBottom().line)
						|| (oldRegion.getBottom().line >= newRegion.getTop().line && oldRegion.getBottom().line <= newRegion.getBottom().line)) {
					const length_t i[2] = {
						min(oldRegion.getTop().line, newRegion.getTop().line), max(oldRegion.getBottom().line, newRegion.getBottom().line)
					};
					redrawLines(min(i[0], i[1]), max(i[0], i[1]));
				} else {
					redrawLines(oldRegion.getTop().line, oldRegion.getBottom().line);
					if(!isFrozen())
						update();
					redrawLines(newRegion.getTop().line, newRegion.getBottom().line);
				}
			}
		}
		changed = true;
	}

	if(changed && !isFrozen())
		update();

	// IME で入力中の場合は編集ウィンドウの位置を修正
	if(imeCompositionActivated_)
		updateIMECompositionWindowPosition();
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
	renderer_.reset(new TextRenderer(*this));
	renderer_->addVisualLinesListener(*this);
	initializeWindow(originalView_ != this);

#ifdef _DEBUG
	// partitioning test
	VerticalRulerConfiguration vrc;
	vrc.lineNumbers.visible = true;
	vrc.indicatorMargin.visible = true;
	vrc.lineNumbers.textColor = Colors(RGB(0x00, 0x80, 0x80), RGB(0xFF, 0xFF, 0xFF));
	vrc.lineNumbers.borderColor = RGB(0x00, 0x80, 0x80);
	vrc.lineNumbers.borderStyle = VerticalRulerConfiguration::LineNumbers::DOTTED;
	vrc.lineNumbers.borderWidth = 1;
	setConfiguration(0, &vrc);

	using namespace rules;
	TransitionRule* rules[8];
	rules[0] = new TransitionRule(DEFAULT_CONTENT_TYPE, 42, L"\\/\\*");
	rules[1] = new TransitionRule(42, DEFAULT_CONTENT_TYPE, L"\\*\\/");
	rules[2] = new TransitionRule(DEFAULT_CONTENT_TYPE, 43, L"//");
	rules[3] = new TransitionRule(43, DEFAULT_CONTENT_TYPE, L"$");
	rules[4] = new TransitionRule(DEFAULT_CONTENT_TYPE, 44, L"\"");
	rules[5] = new TransitionRule(44, DEFAULT_CONTENT_TYPE, L"((?<!\\\\)\"|$)");
	rules[6] = new TransitionRule(DEFAULT_CONTENT_TYPE, 45, L"\'");
	rules[7] = new TransitionRule(45, DEFAULT_CONTENT_TYPE, L"((?<!\\\\)\'|$)");
	auto_ptr<LexicalPartitioner> p(new LexicalPartitioner());
	p->setRules(rules, endof(rules));
	getDocument().setPartitioner(p);

	class Orz : virtual public presentation::ILineStyleDirector, virtual public IDocumentPartitioningListener {
	public:
		Orz(Presentation& p) : p_(p) {}
		void documentPartitioningChanged(const Region& changedRegion) {
			for(Presentation::TextViewerIterator i(p_.getFirstTextViewer()); i != p_.getLastTextViewer(); ++i)
				(*i)->getTextRenderer().invalidate(changedRegion.getTop().line, changedRegion.getBottom().line + 1);
		}
		const LineStyle& queryLineStyle(length_t line, bool& delegates) const {
			delegates = true;
			vector<DocumentPartition> ps;
			const DocumentPartitioner& partitioner = p_.getDocument().getPartitioner();
			for(length_t column = 0; column < p_.getDocument().getLineLength(line); ) {
				DocumentPartition temp;
				partitioner.getPartition(Position(line, column), temp);
				ps.push_back(temp);
				if(temp.region.getBottom().line != line)
					break;
				column = temp.region.getBottom().column;
			}
			if(ps.empty()) {
				delegates = false;
				return LineStyle::NULL_STYLE;
			}
			LineStyle* styles = new LineStyle;
			styles->count = ps.size();
			styles->array = new StyledText[ps.size()];
			for(size_t i = 0; i < styles->count; ++i) {
				StyledText& st = styles->array[i];
				st.column = ps[i].region.getTop().column;
				st.style.color.foreground = (ps[i].contentType != DEFAULT_CONTENT_TYPE) ? RGB(0x00, 0x66, 0x00) : STANDARD_COLOR;
			}
			return *styles;
		}
	private:
		Presentation& p_;
	};
	Orz* orz = new Orz(getPresentation());
	getDocument().addPartitioningListener(*orz);
	getPresentation().setLineStyleDirector(orz, true);
#endif /* _DEBUG */

	// 位置決めと表示
	move(rect, false);
	if(originalView_ != this)
		scrollTo(originalView_->scrollInfo_.horizontal.position, originalView_->scrollInfo_.horizontal.position, false);
	if(visible)
		show(SW_SHOW);

	return true;
}

/// @see Window#dispatchEvent
LRESULT TextViewer::dispatchEvent(UINT message, WPARAM wParam, LPARAM lParam) {
#ifndef WM_THEMECHANGED
	static const UINT WM_THEMECHANGED = 0x031A;
#endif /* !WM_THEMECHANGED */
#ifndef WM_UNICHAR
	static const UINT WM_UNICHAR = 0x0109;
#endif /* !WM_UNICHAR */

	using namespace ascension::texteditor::commands;

	switch(message) {
	case WM_CAPTURECHANGED:
		onCaptureChanged(reinterpret_cast<HWND>(lParam));
		break;
	case WM_CHAR:
		onChar(static_cast<UINT>(wParam), static_cast<UINT>(lParam));
		return 0L;
	case WM_CLEAR:
		if(toBoolean(::GetKeyState(VK_SHIFT) & 0x8000))
			ClipboardCommand(*this, ClipboardCommand::CUT, true).execute();
		else
			DeletionCommand(*this, DeletionCommand::NEXT_CHARACTER).execute();
		return 0L;
	case WM_COPY:
		ClipboardCommand(*this, ClipboardCommand::COPY, true).execute();
		return 0L;
	case WM_CUT:
		ClipboardCommand(*this, ClipboardCommand::CUT, true).execute();
		return 0L;
	case WM_ERASEBKGND:
//		invalidateRect(0, false);
		return true;
	case WM_GETFONT:
		return reinterpret_cast<LRESULT>(renderer_->getFont());
#ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY
	case WM_GETOBJECT:
		if(lParam == OBJID_CLIENT) {
			ComPtr<IAccessible> acc;
			if(SUCCEEDED(getAccessibleObject(*&acc)) && accLib.isAvailable())
				return accLib.lresultFromObject(IID_IAccessible, wParam, acc);
		} else if(lParam == OBJID_WINDOW) {
		}
		return 0;
#endif /* !ASCENSION_NO_ACTIVE_ACCESSIBILITY */
	case WM_GETTEXT: {
		OutputStringStream s;
		getDocument().writeToStream(s, LBR_CRLF);
		return reinterpret_cast<LRESULT>(s.str().c_str());
	}
	case WM_GETTEXTLENGTH:
		// ウィンドウ関係だし改行は CRLF でいいか。LBR_PHYSICAL_DATA だと遅いし
		return getDocument().getLength(LBR_CRLF);
	case WM_HSCROLL:
		onHScroll(LOWORD(wParam), HIWORD(wParam), reinterpret_cast<HWND>(lParam));
		return 0L;
	case WM_IME_COMPOSITION:
		if(onIMEComposition(wParam, lParam))
			return false;
		break;
	case WM_IME_ENDCOMPOSITION:
		onIMEEndComposition();
		break;
	case WM_IME_NOTIFY:
		if(wParam == IMN_SETOPENSTATUS)
			inputStatusListeners_.notify(ITextViewerInputStatusListener::textViewerIMEOpenStatusChanged);
		break;
	case WM_IME_REQUEST:
		return onIMERequest(wParam, lParam);
	case WM_IME_STARTCOMPOSITION:
		onIMEStartComposition();
		break;
	case WM_INPUTLANGCHANGE:
		inputStatusListeners_.notify(ITextViewerInputStatusListener::textViewerInputLanguageChanged);
		if(hasFocus()) {
			if(texteditor::Session* session = getDocument().getSession())
				session->getInputSequenceCheckers()->setKeyboardLayout(::GetKeyboardLayout(::GetCurrentThreadId()));
		}
		break;
	case WM_KEYDOWN:
		endAutoScroll();
		if(onKeyDown(static_cast<UINT>(wParam), static_cast<UINT>(lParam)))
			return false;
		break;
	case WM_MBUTTONDOWN:
		if(mouseOperationDisabledCount_ == 0) {
			if(modeState_.cursorVanished) {	
				modeState_.cursorVanished = false;
				::ShowCursor(true);
				releaseCapture();
			}
			endAutoScroll();
			setFocus();
			beginAutoScroll();
		}
		return 0L;
	case WM_MOUSEWHEEL: {
		::POINT pt = {LOWORD(lParam), HIWORD(lParam)};
		onMouseWheel(GET_KEYSTATE_WPARAM(wParam), GET_WHEEL_DELTA_WPARAM(wParam), pt);
		return 0L;
	}
	case WM_PASTE:
		ClipboardCommand(*this, ClipboardCommand::PASTE, false).execute();
		return 0L;
	case WM_RBUTTONDOWN: {
		::POINT pt = {LOWORD(lParam), LOWORD(lParam)};
		onRButtonDown(static_cast<UINT>(wParam), pt);
		return 0L;
	}
	case WM_SETTEXT:
		SelectionCreationCommand(*this, SelectionCreationCommand::ALL).execute();
		getCaret().replaceSelection(String(reinterpret_cast<const wchar_t*>(lParam)), false);
		return 0L;
	case WM_SIZING:
		onSizing(static_cast<UINT>(wParam), *reinterpret_cast<::RECT*>(lParam));
		return true;
	case WM_STYLECHANGED:
		onStyleChanged(static_cast<int>(wParam), *reinterpret_cast<::STYLESTRUCT*>(lParam));
		return true;
	case WM_STYLECHANGING:
		onStyleChanging(static_cast<int>(wParam), *reinterpret_cast<::STYLESTRUCT*>(lParam));
		return true;
	case WM_SYSCHAR:
		if(onSysChar(static_cast<UINT>(wParam), static_cast<UINT>(lParam)))
			return true;
		break;
	case WM_SYSCOLORCHANGE:
	case WM_THEMECHANGED:
		onSysColorChange();
		return 0L;
	case WM_SYSKEYDOWN:
		if(onSysKeyDown(static_cast<UINT>(wParam), static_cast<UINT>(lParam)))
			return true;
		break;
	case WM_SYSKEYUP:
		if(onSysKeyUp(static_cast<UINT>(wParam), static_cast<UINT>(lParam)))
			return true;
		break;
	case WM_UNDO:
		UndoCommand(*this, true).execute();
		return 0L;
	case WM_UNICHAR:
		onUniChar(static_cast<UINT>(wParam), static_cast<UINT>(lParam));
		return 0L;
	case WM_VSCROLL:
		onVScroll(LOWORD(wParam), HIWORD(wParam), reinterpret_cast<HWND>(lParam));
		return 0L;
	}

	return BaseControl::dispatchEvent(message, wParam, lParam);
}

/// Implementation of #beep method. The subclasses can override to customize the behavior.
void TextViewer::doBeep() throw() {
	::MessageBeep(MB_OK);
}

/// @see Document#IStateListener#documentAccessibleRegionChanged
void TextViewer::documentAccessibleRegionChanged(Document& document) {
	if(getDocument().isNarrowed())
		scrollTo(-1, -1, false);
	invalidateRect(0, false);
}

/// @see Document#IListener#documentAboutToBeChanged
void TextViewer::documentAboutToBeChanged(const Document&) {
}

/// @see Document#IListener#documentChanged
void TextViewer::documentChanged(const Document& document, const DocumentChange& change) {
	// インクリメンタル検索中であればやめさせる
	if(texteditor::Session* session = getDocument().getSession()) {
		if(session->getIncrementalSearcher().isRunning())
			session->getIncrementalSearcher().abort();
	}

	const Region& region = change.getRegion();
	const bool multiLine = region.getTop().line != region.getBottom().line;
	if(isFrozen() && multiLine && freezeInfo_.invalidLines.first != INVALID_INDEX) {
		// 凍結中の描画待ち行のずらす
		const length_t first = region.getTop().line + 1, last = region.getBottom().line;
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
//	invalidateLines(region.getTop().line, !multiLine ? region.getBottom().line : INVALID_INDEX);
	if(scrollInfo_.changed)
		updateScrollBars();
}

/// @see text#Document#IDocumentStateListener#documentEncodingChanged
void TextViewer::documentEncodingChanged(Document& document) {
}

/// @see text#Document#IDocumentStateListener#documentFileNameChanged
void TextViewer::documentFileNameChanged(Document& document) {
}

/// @see text#Document#IDocumentStateListener#documentModificationSignChanged
void TextViewer::documentModificationSignChanged(Document& document) {
}

/// @see text#Document#IDocumentStateListener#documentReadOnlySignChanged
void TextViewer::documentReadOnlySignChanged(Document& document) {
}

/// @see text#Document#ISequentialEditListener#documentSequentialEditStarted
void TextViewer::documentSequentialEditStarted(Document& document) {
}

/// @see text#Document#ISequentialEditListener#documentSequentialEditStopped
void TextViewer::documentSequentialEditStopped(Document& document) {
}

/// @see text#Document#ISequentialEditListener#documentUndoSequenceStarted
void TextViewer::documentUndoSequenceStarted(Document& document) {
	freeze(false);
}

/// @see text#Document#ISequentialEditListener#documentUndoSequenceStopped
void TextViewer::documentUndoSequenceStopped(Document& document, const Position& resultPosition) {
	unfreeze(false);
	if(resultPosition != Position::INVALID_POSITION && hasFocus()) {
//		caret_->endAutoCompletion();
		caret_->moveTo(resultPosition);
	}
}

/// @see IDropTarget#DragEnter
STDMETHODIMP TextViewer::DragEnter(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) {
	if(pDataObj == 0)
		return E_INVALIDARG;
	VERIFY_POINTER(pdwEffect);

	if(mouseOperationDisabledCount_ != 0) {
		*pdwEffect = DROPEFFECT_NONE;
		return S_OK;
	}

	FORMATETC fe = {CF_TEXT, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

	if(leftDownMode_ == LDM_NONE)
		leftDownMode_ = LDM_DRAGANDDROP;
	setFocus();

	// ドラッグされてきたデータが有効かどうか調べる
	if(!getDocument().isReadOnly()
			&& (leftDownMode_ == LDM_DRAGANDDROPSELF
			|| pDataObj->QueryGetData(&fe) == S_OK
			|| (fe.cfFormat = CF_UNICODETEXT, pDataObj->QueryGetData(&fe) == S_OK))) {
		setTimer(TIMERID_DRAGSCROLL, 50, 0);
		return DragOver(grfKeyState, pt, pdwEffect);
	}
	leftDownMode_ = LDM_NONE;
	*pdwEffect = DROPEFFECT_NONE;
	return S_OK;
}

/// @see IDropTarget#DragLeave
STDMETHODIMP TextViewer::DragLeave() {
	::SetFocus(0);
	killTimer(TIMERID_DRAGSCROLL);
	if(leftDownMode_ != LDM_DRAGANDDROPSELF
			&& leftDownMode_ != LDM_DRAGANDDROPBOXSELF)
		leftDownMode_ = LDM_NONE;
	return S_OK;
}

/// @see IDropTarget#DragOver
STDMETHODIMP TextViewer::DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) {
	VERIFY_POINTER(pdwEffect);

	if(mouseOperationDisabledCount_ == 0
			&& (leftDownMode_ == LDM_DRAGANDDROP
			|| leftDownMode_ == LDM_DRAGANDDROPSELF
			|| leftDownMode_ == LDM_DRAGANDDROPBOXSELF)) {	// ドロップ可能な場合以外は何もしない
		::POINT caretPoint = {pt.x, pt.y};
		screenToClient(caretPoint);
		setCaretPosition(getClientXYForCharacter(getCharacterForClientXY(caretPoint, true), LineLayout::LEADING));
		if(toBoolean(::GetKeyState(VK_CONTROL) & 0x8000) && toBoolean(::GetKeyState(VK_SHIFT) & 0x8000))
			*pdwEffect = DROPEFFECT_NONE;
		else if(leftDownMode_ != LDM_DRAGANDDROP)
			*pdwEffect = toBoolean(::GetKeyState(VK_CONTROL) & 0x8000) ? DROPEFFECT_COPY : DROPEFFECT_MOVE;
		else
			*pdwEffect = DROPEFFECT_COPY;
	} else
		*pdwEffect = DROPEFFECT_NONE;
	return S_OK;
}

/**
 *	垂直ルーラのインジケータマージン描画後に呼び出される
 *	@param line	論理行
 *	@param dc	デバイスコンテキスト
 *	@param rect	描画範囲
 */
void TextViewer::drawIndicatorMargin(length_t line, DC& dc, const ::RECT& rect) {
}
/*
namespace {
	class TokenIndexer : public unary_function<size_t, length_t> {
	public:
		explicit TokenIndexer(const Tokens& tokens) throw() : tokens_(tokens) {}
		length_t operator()(size_t i) throw() {return tokens_.array[i].getIndex();}
	private:
		const Tokens& tokens_;
	};
} // namespace @0
*/
/// @see IDropTarget#Drop
STDMETHODIMP TextViewer::Drop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) {
	if(pDataObj == 0)
		return E_INVALIDARG;
	VERIFY_POINTER(pdwEffect);

	*pdwEffect = DROPEFFECT_NONE;
	if(mouseOperationDisabledCount_ != 0)
		return S_OK;

	Document& document = getDocument();

	if(leftDownMode_ == LDM_DRAGANDDROP) {	// 他コントローラからのデータ
		::FORMATETC fe = {CF_UNICODETEXT, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
		::STGMEDIUM stm = {TYMED_HGLOBAL, 0};
		::POINT caretPoint = {pt.x, pt.y};

		killTimer(TIMERID_DRAGSCROLL);
		screenToClient(caretPoint);
		const Position pos = getCharacterForClientXY(caretPoint, true);
		caret_->moveTo(pos);
		const UINT boxClipFormat = ::RegisterClipboardFormatW(ASCENSION_RECTANGLE_TEXT_CLIP_FORMAT);

		if(pDataObj->QueryGetData(&fe) == S_OK) {	// CF_UNICODETEXT サポート
			if(SUCCEEDED(pDataObj->GetData(&fe, &stm))) {
				const Char* buffer = static_cast<Char*>(::GlobalLock(stm.hGlobal));

				document.endSequentialEdit();
				freeze();
				if(fe.cfFormat = boxClipFormat, pDataObj->QueryGetData(&fe) == S_OK) {	// 矩形ドロップ
					document.beginSequentialEdit();
					caret_->insertBox(buffer);
					document.endSequentialEdit();
					caret_->beginBoxSelection();
				} else
					caret_->insert(buffer);
				caret_->select(pos, *caret_);
				unfreeze();
				::GlobalUnlock(stm.hGlobal);
				::ReleaseStgMedium(&stm);
				*pdwEffect = DROPEFFECT_COPY;
			}
		} else if(fe.cfFormat = CF_TEXT, pDataObj->QueryGetData(&fe) == S_OK) {	// CF_TEXT サポート
			if(SUCCEEDED(pDataObj->GetData(&fe, &stm))) {
				const char* nativeBuffer = static_cast<char*>(::GlobalLock(stm.hGlobal));
				const length_t len = min<length_t>(strlen(nativeBuffer), ::GlobalSize(stm.hGlobal) / sizeof(char));
				Char* buffer = new Char[len];

				::MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, nativeBuffer, -1, buffer, static_cast<int>(len));
				freeze();
				if(fe.cfFormat = boxClipFormat, pDataObj->QueryGetData(&fe) == S_OK) {	// 矩形ドロップ
					document.beginSequentialEdit();
					caret_->insertBox(String(buffer, len));
					document.endSequentialEdit();
					caret_->beginBoxSelection();
				} else
					caret_->insert(String(buffer, len));
				caret_->select(pos, *caret_);
				unfreeze();
				delete[] buffer;
				::GlobalUnlock(stm.hGlobal);
				::ReleaseStgMedium(&stm);
				*pdwEffect = DROPEFFECT_COPY;
			}
		}
	} else if(leftDownMode_ == LDM_DRAGANDDROPSELF
			|| leftDownMode_ == LDM_DRAGANDDROPBOXSELF) {	// 自プロセスからのデータ (pDataObj を使わない簡単な処理)
		String text = caret_->getSelectionText(LBR_PHYSICAL_DATA);
		::POINT caretPoint = {pt.x, pt.y};

		screenToClient(caretPoint);
		const Position pos = getCharacterForClientXY(caretPoint, true);

		// 自プロセスのデータは選択範囲上にドロップ不可
		if(getCaret().isPointOverSelection(caretPoint)) {
			*pdwEffect = DROPEFFECT_NONE;
			leftDownMode_ = LDM_NONE;
			caret_->moveTo(pos);
			return S_OK;
		}

		document.beginSequentialEdit();
		freeze();
		if(toBoolean(grfKeyState & MK_CONTROL)) {	// 複写
			redrawLines(caret_->getTopPoint().getLineNumber(), caret_->getBottomPoint().getLineNumber());
			caret_->enableAutoShow(false);
			caret_->moveTo(pos);
			if(leftDownMode_ == LDM_DRAGANDDROPBOXSELF)	// 矩形複写
				caret_->insertBox(text);
			else	// 線形複写
				caret_->insert(text);
			caret_->enableAutoShow(true);
			caret_->select(pos, *caret_);
			*pdwEffect = DROPEFFECT_COPY;
		} else if(leftDownMode_ == LDM_DRAGANDDROPBOXSELF) {	// 矩形移動
			text::Point p(document);
			p.moveTo(pos);
			caret_->eraseSelection();
			p.adaptToDocument(false);
			caret_->enableAutoShow(false);
			caret_->extendSelection(p);
			caret_->insertBox(text);
			caret_->enableAutoShow(true);
			caret_->select(p, *caret_);
			*pdwEffect = DROPEFFECT_MOVE;
		} else {	// 移動
			VisualPoint activePointOrg(*this);
			const Position anchorPointOrg = caret_->getAnchor();
			activePointOrg.moveTo(getCaret());
			caret_->enableAutoShow(false);
			caret_->moveTo(pos);
			activePointOrg.erase(anchorPointOrg);
			const Position temp = *caret_;
			caret_->endBoxSelection();
			caret_->insert(text);
			caret_->enableAutoShow(true);
			caret_->select(temp, *caret_);
			*pdwEffect = DROPEFFECT_MOVE;
		}
		unfreeze();
		document.endSequentialEdit();
	}
	leftDownMode_ = LDM_NONE;
	return S_OK;
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

/**
 * Freezes the drawing of the viewer.
 * @param forAllClones true to freeze also all clones of the viewer
 * @see #isFrozen, #unfreeze
 */
void TextViewer::freeze(bool forAllClones /* = true */) {
	assertValidAsWindow();
	if(!forAllClones)
		++freezeInfo_.count;
	else {
		for(CloneIterator i(*this); !i.isEnd(); i.next())
			++i.get().freezeInfo_.count;
	}
}

#ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY
/// Returns the accessible proxy of the viewer.
HRESULT TextViewer::getAccessibleObject(IAccessible*& acc) const throw() {
	TextViewer& self = *const_cast<TextViewer*>(this);
	acc = 0;
	if(accessibleProxy_ == 0 && isWindow() && accLib.isAvailable()) {
		if(self.accessibleProxy_ = new AccessibleProxy(self)) {
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
#endif /* !ASCENSION_NO_ACTIVE_ACCESSIBILITY */

/**
 * Returns the document position nearest from the specified point.
 * @param pt the coordinates of the point. can be outside of the window
 * @param nearestLeading if set false, the result is the position nearest @a pt.
 * otherwise the result is the position has leading nearest @a pt
 * @return returns the document position
 * @see #getClientXYForCharacter, #hitTest, LineLayout#getOffset
 */
Position TextViewer::getCharacterForClientXY(const ::POINT& pt, bool nearestLeading) const {
	assertValidAsWindow();

	// 論理行の特定
	length_t line, subline;
	mapClientYToLine(pt.y, &line, &subline);
	const LineLayout& layout = renderer_->getLineLayout(line);
	// 文字の確定
	const long x = pt.x + getDisplayXOffset();
	length_t column;
	if(!nearestLeading)
		column = layout.getOffset(x, static_cast<int>(renderer_->getLinePitch() * subline));
	else {
		length_t trailing;
		column = layout.getOffset(x, static_cast<int>(renderer_->getLinePitch() * subline), trailing);
//		if(ascension::internal::advance(x, layout.getLocation(column).x)
//				> ascension::internal::advance(x, layout.getLocation(column + trailing).x))
			column += trailing;
	}
	return Position(line, column);
}

/**
 * Returns the point nearest from the specified document position.
 * @param position the document position. can be outside of the window
 * @param edge the edge of the character
 * @return the client coordinates of the point. about the y-coordinate of the point,
 * if @a position.line is outside of the client area, the result is 32767 (upward) or -32768 (downward)
 * @throw BadPositionException @a position is outside of the document
 * @see #getCharacterForClientXY, #hitTest, LineLayout#getLocation
 */
::POINT TextViewer::getClientXYForCharacter(const Position& position, LineLayout::Edge edge) const {
	assertValidAsWindow();
	::POINT pt;
	if(renderer_->isLineCached(position.line))
		pt = renderer_->getLineLayout(position.line).getLocation(position.column, edge);
	else {
		auto_ptr<LineLayout> layout(new LineLayout(*renderer_, position.line));
		pt = layout->getLocation(position.column, edge);
	}
	pt.x -= getDisplayXOffset();
	const int y = mapLineToClientY(position.line, false);
	if(y == 32767 || y == -32768)
		pt.y = y;
	else
		pt.y += y;
	return pt;
}

///
int TextViewer::getDisplayXOffset() const throw() {
	if(configuration_.alignment == ALIGN_LEFT)
		return scrollInfo_.getX() * renderer_->getAverageCharacterWidth() - getTextAreaMargins().left;
	else if(configuration_.alignment == ALIGN_RIGHT) {
		const ::RECT margins = getTextAreaMargins();
		::RECT clientRect;
		getClientRect(clientRect);
		return scrollInfo_.getX() * renderer_->getAverageCharacterWidth() - getTextAreaMargins().left
			- (clientRect.right - clientRect.left - margins.left - margins.right) % renderer_->getAverageCharacterWidth() - 1;
	} else if(configuration_.alignment == ALIGN_CENTER) {
		// TODO: not implemented.
	}
	assert(false);
	return 0;	// 無意味
}

/**
 * Returns the text and the region of a link near the cursor.
 * @param[out] region the region of the link
 * @param[out] text the text of the link. if the link is mail address, "mailto:" will be added to the head
 * @return true if the cursor is on link
 */
bool TextViewer::getPointedLinkText(Region& region, AutoBuffer<Char>& text) const {
	assertValidAsWindow();
	const Document& document = getDocument();
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

/**
 * Returns the margins of text area.
 * @return the rectangle whose members correspond to each margins
 */
::RECT TextViewer::getTextAreaMargins() const throw() {
	::RECT margins = {0, 0, 0, 0};
	((verticalRulerDrawer_->getConfiguration().alignment == ALIGN_LEFT) ? margins.left : margins.right) += verticalRulerDrawer_->getWidth();
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

/// @see IDropSource#GiveFeedback
STDMETHODIMP TextViewer::GiveFeedback(DWORD) {
	return DRAGDROP_S_USEDEFAULTCURSORS;	// システムのデフォルトのカーソルを使う 
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
 * @return true if the key down was handled
 */
bool TextViewer::handleKeyDown(UINT key, bool controlPressed, bool shiftPressed) throw() {
	using namespace ascension::texteditor::commands;
	switch(key) {
	case VK_BACK:	// [BackSpace]
	case VK_F16:	// [F16]
		DeletionCommand(*this, controlPressed ? DeletionCommand::PREVIOUS_WORD : DeletionCommand::PREVIOUS_CHARACTER).execute();
		return true;
	case VK_CLEAR:	// [Clear]
		if(controlPressed) {
			SelectionCreationCommand(*this, SelectionCreationCommand::ALL).execute();
			return true;
		}
		break;
	case VK_RETURN:	// [Enter]
		LineBreakCommand(*this, controlPressed).execute();
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
		else				CaretMovementCommand(*this, CaretMovementCommand::PREVIOUS_PAGE, shiftPressed).execute();
		return true;
	case VK_NEXT:	// [PageDown]
		if(controlPressed)	onVScroll(SB_PAGEDOWN, 0, 0);
		else				CaretMovementCommand(*this, CaretMovementCommand::NEXT_PAGE, shiftPressed).execute();
		return true;
	case VK_HOME:	// [Home]
		CaretMovementCommand(*this, controlPressed ?
			CaretMovementCommand::START_OF_DOCUMENT : CaretMovementCommand::START_OF_LINE, shiftPressed).execute();
		return true;
	case VK_END:	// [End]
		CaretMovementCommand(*this, controlPressed ?
			CaretMovementCommand::END_OF_DOCUMENT : CaretMovementCommand::END_OF_LINE, shiftPressed).execute();
		return true;
	case VK_LEFT:	// [Left]
		CaretMovementCommand(*this, controlPressed ?
			CaretMovementCommand::LEFT_WORD : CaretMovementCommand::LEFT_CHARACTER, shiftPressed).execute();
		return true;
	case VK_UP:		// [Up]
		if(controlPressed && !shiftPressed)	onVScroll(SB_LINEUP, 0, 0);
		else								CaretMovementCommand(*this, CaretMovementCommand::VISUAL_PREVIOUS_LINE, shiftPressed).execute();
		return true;
	case VK_RIGHT:	// [Right]
		CaretMovementCommand(*this, controlPressed ?
			CaretMovementCommand::RIGHT_WORD : CaretMovementCommand::RIGHT_CHARACTER, shiftPressed).execute();
		return true;
	case VK_DOWN:	// [Down]
		if(controlPressed && !shiftPressed)	onVScroll(SB_LINEDOWN, 0, 0);
		else								CaretMovementCommand(*this, CaretMovementCommand::VISUAL_NEXT_LINE, shiftPressed).execute();
		return true;
	case VK_INSERT:	// [Insert]
		if(!shiftPressed) {
			if(controlPressed)		ClipboardCommand(*this, ClipboardCommand::COPY, true).execute();
			else					InputStatusToggleCommand(*this, InputStatusToggleCommand::OVERTYPE_MODE).execute();
		} else if(controlPressed)	ClipboardCommand(*this, ClipboardCommand::PASTE, false).execute();
		else						break;
		return true;
	case VK_DELETE:	// [Delete]
		if(!shiftPressed)
			DeletionCommand(*this, controlPressed ? DeletionCommand::NEXT_WORD : DeletionCommand::NEXT_CHARACTER).execute();
		else if(!controlPressed)
			ClipboardCommand(*this, ClipboardCommand::CUT, true).execute();
		else
			break;
		return true;
	case 'A':	// ^A -> Select All
		if(controlPressed)
			return SelectionCreationCommand(*this, SelectionCreationCommand::ALL).execute(), true;
		break;
	case 'C':	// ^C -> Copy
		if(controlPressed)
			return ClipboardCommand(*this, ClipboardCommand::COPY, true).execute(), true;
		break;
	case 'H':	// ^H -> Backspace
		if(controlPressed)
			DeletionCommand(*this, DeletionCommand::PREVIOUS_CHARACTER).execute(), true;
		break;
	case 'I':	// ^I -> Tab
		if(controlPressed)
			return CharacterInputCommand(*this, 0x0009).execute(), true;
		break;
	case 'J':	// ^J -> New Line
	case 'M':	// ^M -> New Line
		if(controlPressed)
			return LineBreakCommand(*this, false).execute(), true;
		break;
	case 'V':	// ^V -> Paste
		if(controlPressed)
			return ClipboardCommand(*this, ClipboardCommand::PASTE, false).execute(), true;
		break;
	case 'X':	// ^X -> Cut
		if(controlPressed)
			return ClipboardCommand(*this, ClipboardCommand::CUT, true).execute(), true;
		break;
	case 'Y':	// ^Y -> Redo
		if(controlPressed)
			return UndoCommand(*this, false).execute(), true;
		break;
	case 'Z':	// ^Z -> Undo
		if(controlPressed)
			return UndoCommand(*this, true).execute(), true;
		break;
	case VK_NUMPAD5:	// [Number Pad 5]
		if(controlPressed)
			return SelectionCreationCommand(*this, SelectionCreationCommand::ALL).execute(), true;
		break;
	case VK_F12:	// [F12]
		if(controlPressed && shiftPressed)
			return CharacterCodePointConversionCommand(*this, false).execute(), true;
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
TextViewer::HitTestResult TextViewer::hitTest(const ::POINT& pt) const {
	assertValidAsWindow();

	const VerticalRulerConfiguration& vrc = verticalRulerDrawer_->getConfiguration();
	::RECT clientRect;
	getClientRect(clientRect);
	if(!toBoolean(::PtInRect(&clientRect, pt)))
		return OUT_OF_VIEW;

	if(vrc.indicatorMargin.visible
			&& ((vrc.alignment == ALIGN_LEFT && pt.x < vrc.indicatorMargin.width)
			|| (vrc.alignment == ALIGN_RIGHT && pt.x >= clientRect.right - vrc.indicatorMargin.width)))
		return INDICATOR_MARGIN;
	else if(vrc.lineNumbers.visible
			&& ((vrc.alignment == ALIGN_LEFT && pt.x < verticalRulerDrawer_->getWidth())
			|| (vrc.alignment == ALIGN_RIGHT && pt.x >= clientRect.right - verticalRulerDrawer_->getWidth())))
		return LINE_NUMBERS;
	else if((vrc.alignment == ALIGN_LEFT && pt.x < verticalRulerDrawer_->getWidth() + configuration_.leadingMargin)
			|| (vrc.alignment == ALIGN_RIGHT && pt.x >= clientRect.right - verticalRulerDrawer_->getWidth() - configuration_.leadingMargin))
		return LEADING_MARGIN;
	else if(pt.y < getTextAreaMargins().top)
		return TOP_MARGIN;
	else
		return TEXT_AREA;
}

/**
 * Initializes the window.
 * @param copyConstructing true if called by the clone
 */
void TextViewer::initializeWindow(bool copyConstructing) {
	assertValidAsWindow();

	if(copyConstructing)	// 他のビューから複製する場合
		setConfiguration(&getConfiguration(), 0);

#ifndef ASCENSION_NO_DOUBLE_BUFFERING
	// メモリデバイスコンテキストの用意
	memDC_ = getDC().createCompatibleDC();
#endif /* !ASCENSION_NO_DOUBLE_BUFFERING */

//	if(copyConstructing)
//		recalcLeftTabWidth();

	// ツールチップの作成
	toolTip_ = ::CreateWindowExW(
		WS_EX_TOOLWINDOW | WS_EX_TOPMOST, TOOLTIPS_CLASSW, 0,
		WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, get(), 0,
		reinterpret_cast<HINSTANCE>(static_cast<HANDLE_PTR>(::GetWindowLongPtr(get(), GWLP_HINSTANCE))), 0);
	if(toolTip_ != 0) {
		AutoZeroCB<::TOOLINFOW> ti;
		::RECT margins = {1, 1, 1, 1};

		ti.hwnd = get();
		ti.lpszText = LPSTR_TEXTCALLBACKW;
		ti.uFlags = TTF_SUBCLASS;
		ti.uId = 1;
		::SetRect(&ti.rect, 0, 0, 0, 0);
		::SendMessageW(toolTip_, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&ti));
		::SendMessageW(toolTip_, TTM_SETDELAYTIME, TTDT_AUTOPOP, 30000);	// 30秒間表示されるように
//		::SendMessageW(toolTip_, TTM_SETDELAYTIME, TTDT_INITIAL, 1500);
		::SendMessageW(toolTip_, TTM_SETMARGIN, 0, reinterpret_cast<LPARAM>(&margins));
		::SendMessageW(toolTip_, TTM_ACTIVATE, true, 0L);
	}

	// 自動スクロールの原点ウィンドウの作成
	autoScrollOriginMark_.reset(new AutoScrollOriginMark);
	autoScrollOriginMark_->create(*this);
	
	// ドロップ対象に登録
	registerDragDrop(*this);
}

/// Revokes the frozen state of the viewer actually.
void TextViewer::internalUnfreeze() {
	assertValidAsWindow();

	if(scrollInfo_.changed) {
		updateScrollBars();
		invalidateRect(0, false);
	} else if(freezeInfo_.invalidLines.first != INVALID_INDEX)
		redrawLines(freezeInfo_.invalidLines.first, freezeInfo_.invalidLines.second);
	freezeInfo_.invalidLines.first = freezeInfo_.invalidLines.second = INVALID_INDEX;

	caretMoved(getCaret(), getCaret().getSelectionRegion());
	update();
}

/**
 * Converts the distance from the window top to the logical line.
 * @param y the distance
 * @param[out] logicalLine the logical line index. can be @c null if not needed
 * @param[out] visualSublineOffset the offset from the first line in @a logicalLine. can be @c null if not needed
 * @see #mapLineToClientY, TextRenderer#offsetVisualLine
 */
void TextViewer::mapClientYToLine(int y, length_t* logicalLine, length_t* visualSublineOffset) const throw() {
	if(logicalLine == 0 && visualSublineOffset == 0)
		return;
	y -= getTextAreaMargins().top;
	length_t line, subline;
	getFirstVisibleLine(&line, 0, &subline);
	renderer_->offsetVisualLine(line, subline, y / renderer_->getLinePitch());
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
	const ::RECT margins = getTextAreaMargins();
	if(line == scrollInfo_.firstVisibleLine) {
		if(scrollInfo_.firstVisibleSubline == 0)
			return margins.top;
		else
			return fullSearch ? margins.top - static_cast<int>(renderer_->getLinePitch() * scrollInfo_.firstVisibleSubline) : -32768;
	} else if(line > scrollInfo_.firstVisibleLine) {
		const int lineSpan = renderer_->getLinePitch();
		::RECT clientRect;
		getClientRect(clientRect);
		int y = margins.top;
		y += lineSpan * static_cast<int>((renderer_->getNumberOfSublinesOfLine(scrollInfo_.firstVisibleLine) - scrollInfo_.firstVisibleSubline));
		for(length_t i = scrollInfo_.firstVisibleLine + 1; i < line; ++i) {
			y += lineSpan * static_cast<int>(renderer_->getNumberOfSublinesOfLine(i));
			if(y >= clientRect.bottom - clientRect.top && !fullSearch)
				return 32767;
		}
		return y;
	} else if(!fullSearch)
		return -32768;
	else {
		const int lineSpan = renderer_->getLinePitch();
		int y = margins.top - static_cast<int>(lineSpan * scrollInfo_.firstVisibleSubline);
		for(length_t i = scrollInfo_.firstVisibleLine - 1; i >= line; --i)
			y -= static_cast<int>(renderer_->getNumberOfSublinesOfLine(i) * lineSpan);
		return y;
	}

}

/// @see ICaretListener#matchBracketsChanged
void TextViewer::matchBracketsChanged(const Caret& self, const pair<Position, Position>& oldPair, bool outsideOfView) {
	const pair<Position, Position>& newPair = self.getMatchBrackets();
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
		if(oldPair.first != Position::INVALID_POSITION	// 前回分を削除
				&& oldPair.first.line != newPair.first.line && oldPair.first.line != newPair.second.line) {
			redrawLine(oldPair.first.line);
			if(!isFrozen())
				update();
		}
		if(oldPair.second != Position::INVALID_POSITION && oldPair.second.line != newPair.first.line
				&& oldPair.second.line != newPair.second.line && oldPair.second.line != oldPair.first.line)
			redrawLine(oldPair.second.line);
	} else {
		if(oldPair.first != Position::INVALID_POSITION) {	// 前回分を削除
			assert(oldPair.second != Position::INVALID_POSITION);
			redrawLine(oldPair.first.line);
			if(!isFrozen())
				update();
			if(oldPair.second.line != oldPair.first.line)
				redrawLine(oldPair.second.line);
		}
	}
}

/// @see Window#onCaptureChanged
void TextViewer::onCaptureChanged(HWND) {
	leftDownMode_ = LDM_NONE;
	killTimer(TIMERID_EXPANDSELECTION);
	killTimer(TIMERID_EXPANDLINESELECTION);
	killTimer(TIMERID_AUTOSCROLL);
	caret_->restoreSelectionMode();
}

/**
 * This method can't be overriden (override @c #onUniChar instead).
 * @see Window#onChar
 */
void TextViewer::onChar(UINT ch, UINT flags) {
	onUniChar(ch, flags);
}

/// @see Window#onCommand
bool TextViewer::onCommand(WORD id, WORD notifyCode, HWND control) {
	using namespace ascension::texteditor::commands;
	switch(id) {
	case WM_UNDO:	// [元に戻す]
		UndoCommand(*this, true).execute();
		break;
	case WM_REDO:	// [やり直し]
		UndoCommand(*this, false).execute();
		break;
	case WM_CUT:	// [切り取り]
		ClipboardCommand(*this, ClipboardCommand::CUT, true).execute();
		break;
	case WM_COPY:	// [コピー]
		ClipboardCommand(*this, ClipboardCommand::COPY, true).execute();
		break;
	case WM_PASTE:	// [貼り付け]
		ClipboardCommand(*this, ClipboardCommand::PASTE, false).execute();
		break;
	case WM_CLEAR:	// [削除]
		DeletionCommand(*this, DeletionCommand::NEXT_CHARACTER).execute();
		break;
	case WM_SELECTALL:	// [すべて選択]
		SelectionCreationCommand(*this, SelectionCreationCommand::ALL).execute();
		break;
	case ID_RTLREADING:	// [右から左に読む]
		toggleOrientation(*this);
		break;
	case ID_DISPLAYSHAPINGCONTROLS: {	// [Unicode 制御文字の表示]
		Configuration c = getConfiguration();
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
	case ID_TOGGLEIMESTATUS:	// [IME を開く] / [IME を閉じる]
		InputStatusToggleCommand(*this, InputStatusToggleCommand::IME_STATUS).execute();
		break;
	case ID_TOGGLESOFTKEYBOARD:	// [ソフトキーボードを開く] / [ソフトキーボードを閉じる]
		InputStatusToggleCommand(*this, InputStatusToggleCommand::SOFT_KEYBOARD).execute();
		break;
	case ID_RECONVERT:	// [再変換]
		ReconversionCommand(*this).execute();
		break;
	default:
		getParent()->sendMessage(WM_COMMAND, MAKEWPARAM(id, notifyCode), reinterpret_cast<LPARAM>(control));
	}

	return BaseControl::onCommand(id, notifyCode, control);
}

/// @see Window#onContextMenu
bool TextViewer::onContextMenu(HWND window, const ::POINT& pt) {
	using manah::win32::ui::Menu;

	if(mouseOperationDisabledCount_ != 0)	// マウス操作とは限らないが...
		return true;
//	caret_->endAutoCompletion();
	abortIncrementalSearch(*this);

	// キーボードによる場合
	if(pt.x == 65535 && pt.y == 65535) {
		const_cast<::POINT&>(pt).x = const_cast<POINT&>(pt).y = 1;	// 適当に...
		clientToScreen(const_cast<::POINT&>(pt));
	}

	// スクロールバー上であれば処理しない
	::RECT rect;
	getClientRect(rect);
	clientToScreen(rect);
	if(!toBoolean(::PtInRect(&rect, pt)))
		return false;

	const Document& document = getDocument();
	const bool hasSelection = !getCaret().isSelectionEmpty();
	const bool readOnly = document.isReadOnly();
	const bool japanese = PRIMARYLANGID(getUserDefaultUILanguage()) == LANG_JAPANESE;

	static ui::PopupMenu menu;
	static const WCHAR* captions[] = {
		L"&Undo",								L"\x5143\x306B\x623B\x3059(&U)",
		L"&Redo",								L"\x3084\x308A\x76F4\x3057(&R)",
		0,										0,
		L"Cu&t",								L"\x5207\x308A\x53D6\x308A(&T)",
		L"&Copy",								L"\x30B3\x30D4\x30FC(&C)",
		L"&Paste",								L"\x8CBC\x308A\x4ED8\x3051(&P)",
		L"&Delete",								L"\x524A\x9664(&D)",
		0,										0,
		L"Select &All",							L"\x3059\x3079\x3066\x9078\x629E(&A)",
		0,										0,
		L"&Right to left Reading order",		L"\x53F3\x304B\x3089\x5DE6\x306B\x8AAD\x3080(&R)",
		L"&Show Unicode control characters",	L"Unicode \x5236\x5FA1\x6587\x5B57\x306E\x8868\x793A(&S)",
		L"&Insert Unicode control character",	L"Unicode \x5236\x5FA1\x6587\x5B57\x306E\x633F\x5165(&I)",
		L"Insert Unicode &whitespace character",L"Unicode \x7A7A\x767D\x6587\x5B57\x306E\x633F\x5165(&W)",
	};																	
#define GET_CAPTION(index)	captions[(index) * 2 + (japanese ? 1 : 0)]

	if(menu.getNumberOfItems() == 0) {	// 初期化
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

		// [Unicode 制御文字の挿入] 以下
		Menu* subMenu = new ui::PopupMenu;
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
		menu.setChildPopup<Menu::BY_POSITION>(12, *subMenu);

		// [Unicode 空白文字の挿入] 以下
		subMenu = new ui::PopupMenu;
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
		menu.setChildPopup<Menu::BY_POSITION>(13, *subMenu);

		// bidi サポートがあるか?
		if(!renderer_->supportsComplexScript()) {
			menu.enable<Menu::BY_COMMAND>(ID_RTLREADING, false);
			menu.enable<Menu::BY_COMMAND>(ID_DISPLAYSHAPINGCONTROLS, false);
			menu.enable<Menu::BY_POSITION>(12, false);
			menu.enable<Menu::BY_POSITION>(13, false);
		}
	}
#undef GET_CAPTION

	// メニュー項目の修正
	menu.enable<Menu::BY_COMMAND>(WM_UNDO, !readOnly && document.getUndoHistoryLength(false) != 0);
	menu.enable<Menu::BY_COMMAND>(WM_REDO, !readOnly && document.getUndoHistoryLength(true) != 0);
	menu.enable<Menu::BY_COMMAND>(WM_CUT, !readOnly && hasSelection);
	menu.enable<Menu::BY_COMMAND>(WM_COPY, hasSelection);
	menu.enable<Menu::BY_COMMAND>(WM_PASTE, !readOnly && caret_->canPaste());
	menu.enable<Menu::BY_COMMAND>(WM_CLEAR, !readOnly && hasSelection);
	menu.enable<Menu::BY_COMMAND>(WM_SELECTALL, document.getNumberOfLines() > 1 || document.getLineLength(0) > 0);
	menu.check<Menu::BY_COMMAND>(ID_RTLREADING, configuration_.orientation == RIGHT_TO_LEFT);
	menu.check<Menu::BY_COMMAND>(ID_DISPLAYSHAPINGCONTROLS, configuration_.displaysShapingControls);

	// IME 関連
	HKL keyboardLayout = ::GetKeyboardLayout(::GetCurrentThreadId());
	if(//toBoolean(::ImmIsIME(keyboardLayout)) &&
			::ImmGetProperty(keyboardLayout, IGP_SENTENCE) != IME_SMODE_NONE) {
		const bool isJapanese = PRIMARYLANGID(getUserDefaultUILanguage()) == LANG_JAPANESE;
		HIMC imc = ::ImmGetContext(get());
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

		if(toBoolean(::ImmGetProperty(keyboardLayout, IGP_SETCOMPSTR) & SCS_CAP_COMPSTR))
			menu << Menu::StringItem(ID_RECONVERT, reconvert, (!readOnly && hasSelection) ? MFS_ENABLED : MFS_GRAYED);

		::ImmReleaseContext(get(), imc);
	}
	menu.trackPopup(TPM_LEFTALIGN, pt.x, pt.y, get());

	// 消しとく
	int c = menu.getNumberOfItems();
	while(c > 13)
		menu.erase<Menu::BY_POSITION>(c--);

	return true;
}

/// @see Window#onDestroy
void TextViewer::onDestroy() {
	endAutoScroll();

	// D&D 解除
	revokeDragDrop();

	// 従属ウィンドウを削除
	::DestroyWindow(toolTip_);
	if(autoScrollOriginMark_.get() != 0)
		autoScrollOriginMark_->destroy();
	
#ifndef ASCENSION_NO_DOUBLE_BUFFERING
	memDC_->selectObject(oldLineBitmap_);
	::DeleteObject(lineBitmap_);
#endif /* !ASCENSION_NO_DOUBLE_BUFFERING */

#ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY
	if(accessibleProxy_ != 0)
		accessibleProxy_->dispose();
//	if(accLib.isAvailable())
//		accLib.notifyWinEvent(EVENT_OBJECT_DESTROY, *this, OBJID_CLIENT, CHILDID_SELF);
#endif /* !ASCENSION_NO_ACTIVE_ACCESSIBILITY */

	Window::onDestroy();
}

/// @see Window#onHScroll
void TextViewer::onHScroll(UINT sbCode, UINT pos, HWND) {
	switch(sbCode) {
	case SB_LINELEFT:	// 1列分左
		scroll(-1, 0, true); break;
	case SB_LINERIGHT:	// 1列分右
		scroll(+1, 0, true); break;
	case SB_PAGELEFT:	// 1ページ左
		scroll(-static_cast<int>(getNumberOfVisibleColumns()), 0, true); break;
	case SB_PAGERIGHT:	// 1ページ右
		scroll(+static_cast<int>(getNumberOfVisibleColumns()), 0, true); break;
	case SB_LEFT:		// 左端
	case SB_RIGHT: {	// 右端
		int left, right;
		getScrollRange(SB_HORZ, left, right);
		scrollTo((sbCode == SB_LEFT) ? left : right, -1, true); break;
	}
	case SB_THUMBTRACK:	// ドラッグ or ホイール
		scrollTo(getScrollTrackPosition(SB_HORZ), -1, false); break;	// 32ビット値を使う
	}
}

/// @see WM_IME_COMPOSITION
bool TextViewer::onIMEComposition(WPARAM wParam, LPARAM lParam) {
	if(lParam == 0 || toBoolean(lParam & GCS_RESULTSTR)) {	// 確定
		if(HIMC	imc = ::ImmGetContext(get())) {
			if(const length_t len = ::ImmGetCompositionStringW(imc, GCS_RESULTSTR, 0, 0) / sizeof(WCHAR)) {
				// キャンセルされなかった場合入力
				const AutoBuffer<Char> text(new Char[len + 1]);
				::ImmGetCompositionStringW(imc, GCS_RESULTSTR, text.get(), static_cast<DWORD>(len * sizeof(WCHAR)));
				text[len] = 0;
				texteditor::commands::TextInputCommand(*this, text.get()).execute();
			}
			updateIMECompositionWindowPosition();
			::ImmReleaseContext(get(), imc);
		}
		return true;
	}
	return false;
}

/// @see WM_IME_ENDCOMPOSITION
void TextViewer::onIMEEndComposition() {
	showCaret();
	imeCompositionActivated_ = false;
}

/// @see WM_IME_REQUEST
LRESULT TextViewer::onIMERequest(WPARAM command, LPARAM lParam) {
	const Document& document = getDocument();

	// 再変換を行うときにまずこのコマンドが2回飛んでくる
	if(command == IMR_RECONVERTSTRING) {
		if(document.isReadOnly())
			beep();
		else if(caret_->isSelectionEmpty()) {	// 選択が無い場合は IME に再変換範囲を決めてもらう
			const VisualPoint& caret = getCaret();
			if(::RECONVERTSTRING* const prcs = reinterpret_cast<::RECONVERTSTRING*>(lParam)) {
				const String&	line = document.getLine(caret.getLineNumber());
				prcs->dwStrLen = static_cast<DWORD>(line.length());
				prcs->dwStrOffset = sizeof(::RECONVERTSTRING);
				prcs->dwTargetStrOffset = prcs->dwCompStrOffset = static_cast<DWORD>(sizeof(Char) * caret.getColumnNumber());
				prcs->dwTargetStrLen = prcs->dwCompStrLen = 0;
				line.copy(reinterpret_cast<Char*>(reinterpret_cast<char*>(prcs) + prcs->dwStrOffset), prcs->dwStrLen);
			}
			return sizeof(::RECONVERTSTRING) + sizeof(Char) * document.getLineLength(caret.getLineNumber());
		} else if(!getCaret().isSelectionRectangle()) {
			const String selection = getCaret().getSelectionText(LBR_PHYSICAL_DATA);
			if(::RECONVERTSTRING* const prcs = reinterpret_cast<::RECONVERTSTRING*>(lParam)) {
				prcs->dwStrLen = prcs->dwTargetStrLen = prcs->dwCompStrLen = static_cast<DWORD>(selection.length());
				prcs->dwStrOffset = sizeof(::RECONVERTSTRING);
				prcs->dwTargetStrOffset = prcs->dwCompStrOffset = 0;
				selection.copy(reinterpret_cast<Char*>(reinterpret_cast<char*>(prcs) + prcs->dwStrOffset), prcs->dwStrLen);
			}
			return sizeof(::RECONVERTSTRING) + sizeof(Char) * selection.length();
		}
		return 0L;
	}

	// 再変換の直前。RECONVERTSTRING に再変換の範囲が設定されている
	else if(command == IMR_CONFIRMRECONVERTSTRING) {
		::RECONVERTSTRING* const prcs = reinterpret_cast<::RECONVERTSTRING*>(lParam);
		const Position start = document.getStartPosition();
		const Position end = document.getEndPosition();
		if(!caret_->isSelectionEmpty()) {
			// 既に選択がある場合は選択範囲を再変換する。
			// 選択は複数行になっている可能性がある
			if(prcs->dwCompStrLen < prcs->dwStrLen)	// 再変換範囲が狭められてる (長過ぎた)
				prcs->dwCompStrLen = prcs->dwStrLen;	// こうすると IME から警告が出て再変換は行われない。
														// 選択を狭めるのがメモ帳互換の動作だが...
		} else {
			// 選択が無い場合は IME が示唆してきた範囲を再変換する (選択も作成しておく)。
			// この場合は複数行の再変換は発生しない (prcs->dwStrXxx は現在行全体)
			if(document.isNarrowed() && caret_->getLineNumber() == start.line) {	// ナローイング
				if(prcs->dwCompStrOffset / sizeof(Char) < start.column) {
					prcs->dwCompStrLen += static_cast<DWORD>(sizeof(Char) * start.column - prcs->dwCompStrOffset);
					prcs->dwTargetStrLen = prcs->dwCompStrOffset;
					prcs->dwCompStrOffset = prcs->dwTargetStrOffset = static_cast<DWORD>(sizeof(Char) * start.column);
				} else if(prcs->dwCompStrOffset / sizeof(Char) > end.column) {
					prcs->dwCompStrOffset -= prcs->dwCompStrOffset - sizeof(Char) * end.column;
					prcs->dwTargetStrOffset = prcs->dwCompStrOffset;
					prcs->dwCompStrLen = prcs->dwTargetStrLen = static_cast<DWORD>(sizeof(Char) * end.column - prcs->dwCompStrOffset);
				}
			}
			caret_->select(
				Position(caret_->getLineNumber(), prcs->dwCompStrOffset / sizeof(Char)),
				Position(caret_->getLineNumber(), prcs->dwCompStrOffset / sizeof(Char) + prcs->dwCompStrLen));
		}
		return true;
	}

	// 変換ウィンドウの位置決めが必要なときに実行される
	else if(command == IMR_QUERYCHARPOSITION)
		return false;	// updateIMECompositionWindowPosition で何とかする...

	return 0L;
}

/// @see WM_IME_STARTCOMPOSITION
void TextViewer::onIMEStartComposition() {
	if(HIMC imc = ::ImmGetContext(get())) {
		::LOGFONTW font;
		::GetObject(renderer_->getFont(), sizeof(::LOGFONTW), &font);
		::ImmSetCompositionFontW(imc, &font);	// IME の設定によっては反映されるだろう
		hideCaret();
		::ImmReleaseContext(get(), imc);
	}
	imeCompositionActivated_ = true;
	updateIMECompositionWindowPosition();
//	caret_->endAutoCompletion();
}

/// @see Window#onKeyDown
bool TextViewer::onKeyDown(UINT ch, UINT flags) {
	return handleKeyDown(ch, toBoolean(::GetKeyState(VK_CONTROL) & 0x8000), toBoolean(::GetKeyState(VK_SHIFT) & 0x8000));
}

/// @see Window#onKillFocus
void TextViewer::onKillFocus(HWND newWindow) {
	RESTORE_HIDDEN_CURSOR();
	endAutoScroll();
/*	if(caret_->getMatchBracketsTrackingMode() != Caret::DONT_TRACK
			&& getCaret().getMatchBrackets().first != Position::INVALID_POSITION) {	// 対括弧の通知を終了
		FOR_EACH_LISTENERS()
			(*it)->onMatchBracketFoundOutOfView(Position::INVALID_POSITION);
	}
	if(completionWindow_->isWindow() && newWindow != completionWindow_->getSafeHwnd())
		closeCompletionWindow();
*/	abortIncrementalSearch(*this);
	if(imeCompositionActivated_) {	// IME で入力中であればやめさせる
		HIMC imc = ::ImmGetContext(get());
		::ImmNotifyIME(imc, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
		::ImmReleaseContext(get(), imc);
	}
	if(newWindow != get()) {
		hideCaret();
		::DestroyCaret();
	}
	redrawLines(getCaret().getTopPoint().getLineNumber(), getCaret().getBottomPoint().getLineNumber());
	update();
}

/**
 *	ダブルクリックへのコマンド割り当てのためにこのメソッドをオーバーライドすべきではない
 *	@see Window#onLButtonBblClk
 */
void TextViewer::onLButtonDblClk(UINT, const ::POINT& pt) {
	if(mouseOperationDisabledCount_ == 0) {
		abortIncrementalSearch(*this);
		const HitTestResult htr = hitTest(pt);
		if(htr == LEADING_MARGIN || htr == TOP_MARGIN || htr == TEXT_AREA) {
			leftDownMode_ = LDM_SELECTION_WORD;
			caret_->beginWordSelection();
			setCapture();
			setTimer(TIMERID_EXPANDSELECTION, SELECTION_OBSERVATION_INTERVAL, 0);
		}
	}
}

/**
 *	マウス左ボタンへのコマンド割り当てのためにこのメソッドをオーバーライドすべきではない
 *	@see Window#onLButtonDown
 */
void TextViewer::onLButtonDown(UINT flags, const ::POINT& pt) {
	RESTORE_HIDDEN_CURSOR();
	if(mouseOperationDisabledCount_ != 0)
		return;
	if(endAutoScroll())
		return;

	bool boxDragging = false;
	AutoBuffer<Char> uri;
	const HitTestResult htr = hitTest(pt);

//	caret_->endAutoCompletion();
	endIncrementalSearch(*this);

	// 行選択
	if(htr == INDICATOR_MARGIN || htr == LINE_NUMBERS) {
		if(toBoolean(flags & MK_CONTROL))	// 全行選択
			texteditor::commands::SelectionCreationCommand(*this, texteditor::commands::SelectionCreationCommand::ALL).execute();
		else {
			leftDownMode_ = LDM_SELECTION_LINE;
			caret_->moveTo(getCharacterForClientXY(pt, false));
			caret_->beginLineSelection();
		}
		setCapture();
		setTimer(TIMERID_EXPANDLINESELECTION, SELECTION_OBSERVATION_INTERVAL, 0);
	}

	// OLE ドラッグ開始?
	else if(configuration_.enablesOLEDragAndDrop && !caret_->isSelectionEmpty() && caret_->isPointOverSelection(pt)) {
		::GetCursorPos(&modeState_.lastMouseDownPoint);
		screenToClient(modeState_.lastMouseDownPoint);
		if(caret_->isSelectionRectangle())
			boxDragging = true;
	}

	// リンクの起動
//	else if(toBoolean(flags & MK_CONTROL) && linkTextStrategy_ != 0 && isOverInvokableLink(pt, uri))
//		linkTextStrategy_->invokeLink(..., uri.get());

	// 矩形選択開始
	else if(!toBoolean(flags & MK_SHIFT) && toBoolean(::GetKeyState(VK_MENU) & 0x8000)) {
		leftDownMode_ = LDM_SELECTION_CHARACTER;
		caret_->beginBoxSelection();
		caret_->moveTo(getCharacterForClientXY(pt, true));
		setCapture();
		setTimer(TIMERID_EXPANDSELECTION, SELECTION_OBSERVATION_INTERVAL, 0);
	}

	// その他。線形選択開始、キャレット移動
	else {
		leftDownMode_ = LDM_SELECTION_CHARACTER;
		const Position pos = getCharacterForClientXY(pt, true);
		if(toBoolean(flags & MK_CONTROL)) {	// Ctrl -> 現在の単語を選択
			leftDownMode_ = LDM_SELECTION_WORD;
			caret_->moveTo(pos);
			caret_->beginWordSelection();
		} else if(toBoolean(flags & MK_SHIFT)) {	// Shift -> カーソル位置まで選択
			if(toBoolean(::GetKeyState(VK_MENU) & 0x8000)) {	// Shift+Alt -> カーソル位置まで矩形選択
				leftDownMode_ = LDM_SELECTION_CHARACTER;
				caret_->beginBoxSelection();
			}
			caret_->extendSelection(pos);
		} else {
			caret_->moveTo(pos);
			caret_->endBoxSelection();
		}
		setCapture();
		setTimer(TIMERID_EXPANDSELECTION, SELECTION_OBSERVATION_INTERVAL, 0);
	}

	if(!caret_->isSelectionRectangle() && !boxDragging)
		redrawLine(caret_->getLineNumber());
	setFocus();

#undef SELECTION_EXPANSION_INTERVAL
}

/// @see Window#onLButtonUp
void TextViewer::onLButtonUp(UINT, const ::POINT& pt) {
	if(mouseOperationDisabledCount_ != 0)
		return;
	const LeftDownMode original = leftDownMode_;

	if(modeState_.lastMouseDownPoint.x != -1) {	// OLE ドラッグ開始か -> キャンセル
		modeState_.lastMouseDownPoint.x = modeState_.lastMouseDownPoint.y = -1;
		caret_->moveTo(getCharacterForClientXY(pt, true));
		::SetCursor(::LoadCursor(0, IDC_IBEAM));	// うーむ
	}
	releaseCapture();

	// 選択範囲拡大中に画面外でボタンを離すとキャレット位置までスクロールしないことがあるので
	if(original != LDM_NONE)
		caret_->show();
}

/// @see Window#onMouseMove
void TextViewer::onMouseMove(UINT, const ::POINT& pt) {
	RESTORE_HIDDEN_CURSOR();
	if(mouseOperationDisabledCount_ != 0)
		return;

	Position pos;	// カーソル位置から求まる行と列

	if(modeState_.lastMouseDownPoint.x != -1) {	// OLE ドラッグ開始?
		POINT& lastPoint = modeState_.lastMouseDownPoint;
		if(!configuration_.enablesOLEDragAndDrop || getCaret().isSelectionEmpty())
			lastPoint.x = lastPoint.y = -1;
		else {
			const int cxDragBox = ::GetSystemMetrics(SM_CXDRAG);
			const int cyDragBox = ::GetSystemMetrics(SM_CYDRAG);
			if((pt.x > lastPoint.x + cxDragBox / 2) || (pt.x < lastPoint.x - cxDragBox / 2)
					|| (pt.y > lastPoint.y + cyDragBox / 2) || (pt.y < lastPoint.y - cyDragBox / 2)) {
				const bool box = getCaret().isSelectionRectangle();
				const String selection = getCaret().getSelectionText(LBR_PHYSICAL_DATA);

				if(box) {
					set<CLIPFORMAT>	clipFormats;
					clipFormats.insert(CF_UNICODETEXT);
					clipFormats.insert(::RegisterClipboardFormatW(ASCENSION_RECTANGLE_TEXT_CLIP_FORMAT));
					dragging_->setAvailableFormatSet(clipFormats);
				}
				dragging_->setTextData(selection.c_str());
				leftDownMode_ = box ? LDM_DRAGANDDROPBOXSELF : LDM_DRAGANDDROPSELF;
				setTimer(TIMERID_DRAGSCROLL, box ? 100 : 50, 0);
				dragging_->doDragDrop(DROPEFFECT_COPY | DROPEFFECT_MOVE);
				killTimer(TIMERID_DRAGSCROLL);
				leftDownMode_ = LDM_NONE;	// onLButtonUp は呼ばれない
				lastPoint.x = lastPoint.y = -1;
				if(isVisible())
					setFocus();
			}
		}
		return;
	}

	if(toBoolean(pt.x & 0x8000))
		const_cast<::POINT&>(pt).x = 0;
	if(toBoolean(pt.y & 0x8000))
		const_cast<::POINT&>(pt).y = 0;

	if(leftDownMode_ != LDM_NONE) {	// スクロール
/*		const long marginWidth = layoutManager.getVerticalRulerWidth() + layout.leadMargin;
		if(pt.y < static_cast<long>(layout.topMargin + layoutManager.getLineHeight() / 2))
			onVScroll(SB_LINEUP, 0, 0);
		else if(pt.y > static_cast<long>(layout.topMargin + layoutManager.getLineHeight() * getVisibleLineCount()))
			onVScroll(SB_LINEDOWN, 0, 0);
		if(!layout.rightAlign) {
			if(pt.x < marginWidth)
				onHScroll(SB_SETPOS, scrollInfo_.position.x - layout.tabWidth, 0);
//			else if(pt.x >)
		} else {
		}
*/	}
	if(leftDownMode_ == LDM_SELECTION_CHARACTER
			|| leftDownMode_ == LDM_SELECTION_LINE
			|| leftDownMode_ == LDM_SELECTION_WORD) {	// 選択の拡大/縮小
		EXTEND_SELECTION();
	} else if(leftDownMode_ == LDM_DRAGANDDROP) {	// 選択内容をドラッグ中
	}
}

/// @see Window#onMouseWheel
bool TextViewer::onMouseWheel(UINT, short zDelta, const ::POINT& pt) {
	RESTORE_HIDDEN_CURSOR();
	if(mouseOperationDisabledCount_ != 0)
		return true;
	
	if(endAutoScroll())
		return true;

	// システムで設定されている量を使う
	UINT scrollLineCount;	// スクロールする行数
	if(!toBoolean(::SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &scrollLineCount, 0)))
		scrollLineCount = 3;
	zDelta *= (scrollLineCount != WHEEL_PAGESCROLL) ? scrollLineCount : static_cast<UINT>(getNumberOfVisibleLines());
	scroll(0, -zDelta / WHEEL_DELTA, true);

	return true;
}

/// @see Window#onNotify
bool TextViewer::onNotify(int, LPNMHDR nmhdr) {
	// ツールチップのテキスト
	if(nmhdr->hwndFrom == toolTip_ && nmhdr->code == TTN_GETDISPINFO) {
		::SendMessageW(toolTip_, TTM_SETMAXTIPWIDTH, 0, 1000);	// 改行を有効にする
		reinterpret_cast<LPNMTTDISPINFOW>(nmhdr)->lpszText = tipText_;
		return true;
	}
	return false;
}

/// @see Window#onPaint
void TextViewer::onPaint(PaintDC& dc) {
	if(isFrozen())	// 凍結中は無視
		return;
	else if(toBoolean(::IsRectEmpty(&dc.getPaintStruct().rcPaint)))	// 描画の必要な領域が空であれば終了
		return;

	const Document& document = getDocument();
	::RECT clientRect;
	getClientRect(clientRect);

//	Timer tm(L"onPaint");

	const length_t lineCount = document.getNumberOfLines();		// 総論理行数
	const ::RECT& paintRect = dc.getPaintStruct().rcPaint;	// 描画の必要な矩形
	const int linePitch = renderer_->getLinePitch();

	// 垂直ルーラの描画
	verticalRulerDrawer_->draw(dc);

#undef CLIENT_Y_TO_DISPLAY_LINE

	// 行頭・行末余白の描画
	const ::RECT margins = getTextAreaMargins();
	const COLORREF marginColor = internal::systemColors.getReal(configuration_.color.background, SYSTEM_COLOR_MASK | COLOR_WINDOW);
	if(margins.left > 0) {
		const int vrWidth = (verticalRulerDrawer_->getConfiguration().alignment == ALIGN_LEFT) ? verticalRulerDrawer_->getWidth() : 0;
		dc.fillSolidRect(clientRect.left + vrWidth, paintRect.top, margins.left - vrWidth, paintRect.bottom - paintRect.top, marginColor);
	}
	if(margins.right > 0) {
		const int vrWidth = (verticalRulerDrawer_->getConfiguration().alignment == ALIGN_RIGHT) ? verticalRulerDrawer_->getWidth() : 0;
		dc.fillSolidRect(clientRect.right - margins.right, paintRect.top, margins.right - vrWidth, paintRect.bottom - paintRect.top, marginColor);
	}

	// 描画開始行から最終可視行または文末まで行の本体を描画
	const Colors selectionColor(
		internal::systemColors.getReal(configuration_.selectionColor.foreground,
		SYSTEM_COLOR_MASK | (hasFocus() ? COLOR_HIGHLIGHTTEXT : COLOR_INACTIVECAPTIONTEXT)),
		internal::systemColors.getReal(configuration_.selectionColor.background,
		SYSTEM_COLOR_MASK | (hasFocus() ? COLOR_HIGHLIGHT : COLOR_INACTIVECAPTION)));
	::RECT lineRect = clientRect;
	lineRect.left += margins.left; lineRect.top += margins.top; lineRect.right -= margins.right; lineRect.bottom -= margins.bottom;
	length_t line, subline;
	mapClientYToLine(paintRect.top, &line, &subline);
	int y = mapLineToClientY(line, true);
	if(line < lineCount) {
#ifdef _DEBUG
		DumpContext dout;
		if(DIAGNOSE_INHERENT_DRAWING)
			dout << L"lines : ";
#endif /* _DEBUG */
		while(y < paintRect.bottom && line < lineCount) {
			// 論理行を1行描画
#ifdef _DEBUG
			if(DIAGNOSE_INHERENT_DRAWING)
				dout << static_cast<ulong>(line) << ",";
#endif /* _DEBUG */
			renderer_->getLineLayout(line).draw(dc, -getDisplayXOffset(), y - static_cast<int>(subline * linePitch), lineRect, selectionColor);
			y += linePitch * static_cast<int>(renderer_->getNumberOfSublinesOfLine(line++));
			subline = 0;
		}
#ifdef _DEBUG
		if(DIAGNOSE_INHERENT_DRAWING)
			dout << L"\n";
#endif /* _DEBUG */
	}

	// 最終行より後ろ
	if(paintRect.bottom > y && y > margins.top + linePitch - 1)
		dc.fillSolidRect(clientRect.left + margins.left, y,
			clientRect.right - clientRect.left - margins.left - margins.right, paintRect.bottom - y, marginColor);

	// 上余白の描画
	if(margins.top > 0)
		dc.fillSolidRect(clientRect.left + margins.left, clientRect.top,
			clientRect.right - clientRect.left - margins.left - margins.right, margins.top, marginColor);
}

/// @see Window#onRButtonDown
void TextViewer::onRButtonDown(UINT, const ::POINT& pt) {
	RESTORE_HIDDEN_CURSOR();
	if(mouseOperationDisabledCount_ != 0)
		return;
	endAutoScroll();
}

/// @see Window#onSetCursor
bool TextViewer::onSetCursor(HWND, UINT, UINT) {
	static length_t detectedUriLineLast = INVALID_INDEX;
	::POINT pt = getCursorPosition();	// カーソル位置
	bool cursorChanged = false;

	RESTORE_HIDDEN_CURSOR();

	// 垂直ルーラ上か
	const HitTestResult htr = hitTest(pt);
	if(htr == INDICATOR_MARGIN || htr == LINE_NUMBERS) {
		::SetCursor(::LoadCursor(0, IDC_ARROW));
		return true;
	}

	// 選択範囲 (ドラッグ可能かどうか)
	if(configuration_.enablesOLEDragAndDrop && !getCaret().isSelectionEmpty()) {
		if(getCaret().isPointOverSelection(pt)) {
			::SetCursor(::LoadCursor(0, IDC_ARROW));
			cursorChanged = true;
		}
	}

	// リンクのポップアップやカーソルを変える場合
	if(!autoScroll_.scrolling && linkTextStrategy_.get() != 0) {
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

	return cursorChanged;
}

/// @see Window#onSetFocus
void TextViewer::onSetFocus(HWND oldWindow) {
	BaseControl::onSetFocus(oldWindow);

	// スクロール位置を元に戻す
	setScrollPosition(SB_HORZ, scrollInfo_.horizontal.position, false);
	setScrollPosition(SB_VERT, scrollInfo_.vertical.position, true);

	// 選択範囲を再描画 (フォーカスの有無で強調属性が変化するため)
	if(/*sharedData_->options.appearance[SHOW_CURRENT_UNDERLINE] ||*/ !getCaret().isSelectionEmpty()) {
		redrawLines(getCaret().getTopPoint().getLineNumber(), getCaret().getBottomPoint().getLineNumber());
		update();
	}

	if(oldWindow != get()) {
		// キャレットを復活させる
		recreateCaret();
		updateCaretPosition();
		if(texteditor::Session* session = getDocument().getSession()) {
			if(texteditor::InputSequenceCheckers* isc = session->getInputSequenceCheckers())
				isc->setKeyboardLayout(::GetKeyboardLayout(::GetCurrentThreadId()));
		}
	}
}

/// @see Window#onSize
void TextViewer::onSize(UINT type, int cx, int cy) {
//	caret_->endAutoCompletion();

	if(type == SIZE_MINIMIZED)
		return;

	// ツールチップに通知
	AutoZeroCB<::TOOLINFOW> ti;
	::RECT viewRect;
	getClientRect(viewRect);
	ti.hwnd = get();
	ti.uId = 1;
	ti.rect = viewRect;
	::SendMessageW(toolTip_, TTM_NEWTOOLRECT, 0, reinterpret_cast<LPARAM>(&ti));

	if(renderer_.get() == 0)
		return;

	// 描画用ビットマップのサイズも変更する
#ifndef ASCENSION_NO_DOUBLE_BUFFERING
	updateMemoryDeviceContext();
#endif /* !ASCENSION_NO_DOUBLE_BUFFERING */

	renderer_->updateViewerSize();
	scrollInfo_.resetBars(*this, SB_BOTH, true);
	updateScrollBars();
	if(verticalRulerDrawer_->getConfiguration().alignment != ALIGN_LEFT) {
		recreateCaret();
//		redrawVerticalRuler();
		invalidateRect(0, false);	// うーむ...
	}
}

/// @see WM_SIZING
void TextViewer::onSizing(UINT side, ::RECT& rect) {
}

/// @see WM_STYLECHANGED
void TextViewer::onStyleChanged(int type, const ::STYLESTRUCT& style) {
	if(type == GWL_EXSTYLE
			&& (((style.styleOld ^ style.styleNew) & (WS_EX_RIGHT | WS_EX_RTLREADING)) != 0)) {
		// ウィンドウスタイルの変更をに Presentation に反映する
		Configuration c = getConfiguration();
		c.orientation = ((style.styleNew & WS_EX_RTLREADING) != 0) ? RIGHT_TO_LEFT : LEFT_TO_RIGHT;
		c.alignment = ((style.styleNew & WS_EX_RIGHT) != 0) ? ALIGN_RIGHT : ALIGN_LEFT;
		setConfiguration(&c, 0);
	}
}

/// @see WM_STYLECHANGING
void TextViewer::onStyleChanging(int type, ::STYLESTRUCT& style) {
	if(type == GWL_EXSTYLE)
		style.styleNew &= ~WS_EX_LAYOUTRTL;	// このウィンドウの DC のレイアウトは常に LTR でなければならぬ
}

/// @see Window#onSysChar
bool TextViewer::onSysChar(UINT, UINT) {
	RESTORE_HIDDEN_CURSOR();
	return false;
}

/// @see Window#onSysColorChange
void TextViewer::onSysColorChange() {
//	if(this == originalView_)
//		presentation_.updateSystemColors();
}

/// @see Window#onSysKeyDown
bool TextViewer::onSysKeyDown(UINT, UINT) {
	endAutoScroll();
	return false;
}

/// @see Window#onSysKeyUp
bool TextViewer::onSysKeyUp(UINT, UINT) {
	RESTORE_HIDDEN_CURSOR();
	return false;
}

/// @see Window#onTimer
void TextViewer::onTimer(UINT eventID) {
	if(eventID == TIMERID_EXPANDSELECTION
			|| eventID == TIMERID_EXPANDLINESELECTION) {	// 選択中の自動スクロール
		::POINT pt;
		::GetCursorPos(&pt);
		screenToClient(pt);
		const HitTestResult htr = hitTest(pt);
		if(htr != INDICATOR_MARGIN && htr != LINE_NUMBERS && htr != OUT_OF_VIEW)
			return;
		EXTEND_SELECTION();
	} else if(eventID == TIMERID_DRAGSCROLL) {	// ドラッグ中の自動スクロール
		::POINT pt;
		::GetCursorPos(&pt);
		screenToClient(pt);
		::RECT clientRect;
		getClientRect(clientRect);
		::RECT margins = getTextAreaMargins();
		margins.left = max<long>(renderer_->getAverageCharacterWidth(), margins.left);
		margins.top = max<long>(renderer_->getLinePitch() / 2, margins.top);
		margins.right = max<long>(renderer_->getAverageCharacterWidth(), margins.right);
		margins.bottom = max<long>(renderer_->getLinePitch() / 2, margins.bottom);

		// 以下のスクロール量には根拠は無い
		if(pt.y >= clientRect.top && pt.y < clientRect.top + margins.top)
			scroll(0, -1, true);
		else if(pt.y >= clientRect.bottom - margins.bottom && pt.y < clientRect.bottom)
			scroll(0, +1, true);
		else if(pt.x >= clientRect.left && pt.x < clientRect.left + margins.left)
			scroll(-3/*getVisibleColumnCount()*/, 0, true);
		else if(pt.x >= clientRect.right - margins.right && pt.y < clientRect.right)
			scroll(+3/*getVisibleColumnCount()*/, 0, true);
	} else if(eventID == TIMERID_LINEPARSE) {	// 未解析の行を先読みする
		// ...
	} else if(eventID == TIMERID_CALLTIP) {	// ツールチップを表示
		killTimer(TIMERID_CALLTIP);
		::SendMessageW(toolTip_, TTM_UPDATE, 0, 0L);
	} else if(eventID == TIMERID_AUTOSCROLL) {	// 自動スクロール
		::POINT pt;

		killTimer(TIMERID_AUTOSCROLL);
		::GetCursorPos(&pt);
		screenToClient(pt);

		const long	yScrollDegree = (pt.y - autoScroll_.indicatorPosition.y) / renderer_->getLinePitch();
//		const long	xScrollDegree = (pt.x - autoScroll_.indicatorPosition.x) / presentation_.getLineHeight();
//		const long	scrollDegree = max(abs(yScrollDegree), abs(xScrollDegree));

		if(yScrollDegree != 0 /*&& abs(yScrollDegree) >= abs(xScrollDegree)*/)
			onVScroll((yScrollDegree > 0) ? SB_LINEDOWN : SB_LINEUP, 0, 0);
//		else if(xScrollDegree != 0)
//			onHScroll((xScrollDegree > 0) ? SB_RIGHT : SB_LEFT, 0, 0);

		if(yScrollDegree != 0)
			setTimer(TIMERID_AUTOSCROLL, 500 / static_cast<uint>((pow(2, abs(yScrollDegree) / 2))), 0);
		else
			setTimer(TIMERID_AUTOSCROLL, 300, 0);
	}
}

/// @see WM_UNICHAR
void TextViewer::onUniChar(UINT ch, UINT) {
#ifndef UNICODE_NOCHAR
	const UINT UNICODE_NOCHAR = 0xFFFF;
#endif /* !UNICODE_NOCHAR */
	if(ch != UNICODE_NOCHAR) {
		// GUI ユーザが文字の入力を開始したらカーソルを消す
		if(texteditor::commands::CharacterInputCommand(*this, ch).execute() != 0
				&& !modeState_.cursorVanished
				&& configuration_.vanishesCursor
				&& hasFocus()) {
			// カーソルが同一スレッドのウィンドウ上に無いと駄目
			::POINT pt;
			::GetCursorPos(&pt);
			if(::GetWindowThreadProcessId(::WindowFromPoint(pt), 0) == getThreadID()) {
				modeState_.cursorVanished = true;
				::ShowCursor(false);
				setCapture();
			}
		}
		if(imeCompositionActivated_)
			updateIMECompositionWindowPosition();
	}
}

/// @see Window#onVScroll
void TextViewer::onVScroll(UINT sbCode, UINT pos, HWND) {
	switch(sbCode) {
	case SB_LINEUP:		// 1行上
		scroll(0, -1, true); break;
	case SB_LINEDOWN:	// 1行下
		scroll(0, +1, true); break;
	case SB_PAGEUP:		// 1ページ上
		scroll(0, -static_cast<int>(getNumberOfVisibleLines()), true); break;
	case SB_PAGEDOWN:	// 1ページ下
		scroll(0, +static_cast<int>(getNumberOfVisibleLines()), true); break;
	case SB_TOP:		// 上端
	case SB_BOTTOM: {	// 下端
		int top, bottom;
		getScrollRange(SB_VERT, top, bottom);
		scrollTo(-1, (sbCode == SB_TOP) ? top : bottom, true); break;
	}
	case SB_THUMBTRACK:	// ドラッグ or ホイール
		scrollTo(-1, getScrollTrackPosition(SB_VERT), true); break;	// 32ビット値を使う
	}
}

/// @see ICaretListener#overtypeModeChanged
void TextViewer::overtypeModeChanged(const Caret&) {
}

/// @see IDropSource#QueryContinueDrag
STDMETHODIMP TextViewer::QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState) {
	if(fEscapePressed || toBoolean(grfKeyState & MK_RBUTTON))	// キャンセル
		return DRAGDROP_S_CANCEL;
	if(!toBoolean(grfKeyState & MK_LBUTTON))	// ドロップ
		return DRAGDROP_S_DROP;
	return S_OK;
}

/// Recreates and shows the caret. If the viewer does not have focus, nothing heppen.
void TextViewer::recreateCaret() {
	assertValidAsWindow();

	if(!hasFocus())
		return;
	::DestroyCaret();
	if(caretShape_.bitmap.get() != 0) {	// 古いものを破棄 (あれば)
		caretShape_.bitmap->deleteObject();
		caretShape_.bitmap.release();
	}

	::SIZE solidSize = {0, 0};
	if(caretShape_.shaper.get() != 0)
		caretShape_.shaper->getCaretShape(caretShape_.bitmap, solidSize, caretShape_.orientation);
	else {
		DefaultCaretShaper s;
		CaretShapeUpdater u(*this);
		static_cast<ICaretShapeProvider&>(s).install(u);
		static_cast<ICaretShapeProvider&>(s).getCaretShape(caretShape_.bitmap, solidSize, caretShape_.orientation);
		static_cast<ICaretShapeProvider&>(s).uninstall();
	}

	if(caretShape_.bitmap.get() != 0 && caretShape_.bitmap->get() != 0) {
		createCaret(*caretShape_.bitmap, 0, 0);
		::BITMAP bmp;
		::GetObject(*caretShape_.bitmap, sizeof(::BITMAP), &bmp);
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

	const length_t lines = getDocument().getNumberOfLines();
	if(first >= lines || last < scrollInfo_.firstVisibleLine)
		return;

#ifdef _DEBUG
	if(DIAGNOSE_INHERENT_DRAWING)
		DumpContext() << L"inv : " << static_cast<ulong>(first) << L".." << static_cast<ulong>(last) << L"\n";
#endif /* _DEBUG */

	::RECT rect;
	getClientRect(rect);

	// 上端
	rect.top = max(mapLineToClientY(first, false), configuration_.topMargin);
	if(rect.top >= rect.bottom)
		return;
	// 下端
	if(last != numeric_limits<length_t>::max()) {
		long bottom = rect.top + static_cast<long>(renderer_->getNumberOfSublinesOfLine(first) * renderer_->getLinePitch());
		for(length_t line = first + 1; line <= last; ++line) {
			bottom += static_cast<long>(renderer_->getNumberOfSublinesOfLine(line) * renderer_->getLinePitch());
			if(bottom >= rect.bottom)
				break;
		}
		rect.bottom = min(bottom, rect.bottom);
	}
	invalidateRect(&rect, false);
}

/// Redraws the vertical ruler.
void TextViewer::redrawVerticalRuler() {
	::RECT r;
	getClientRect(r);
	if(verticalRulerDrawer_->getConfiguration().alignment == ALIGN_LEFT)
		r.right = r.left + verticalRulerDrawer_->getWidth();
	else
		r.left = r.right - verticalRulerDrawer_->getWidth();
	invalidateRect(&r, false);
}

/// @see IVisualLinesListener#rendererFontChanged
void TextViewer::rendererFontChanged() throw() {
	verticalRulerDrawer_->update();
	scrollInfo_.resetBars(*this, SB_BOTH, true);
	updateScrollBars();
	recreateCaret();
	redrawLine(0, true);
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
//	caret_->endAutoCompletion();
	hideToolTip();

	// 編集領域のスクロール:
	::RECT clipRect, clientRect;
	const ::RECT margins = getTextAreaMargins();
	getClientRect(clientRect);
	clipRect = clientRect;
	clipRect.top += margins.top;
	clipRect.bottom -= margins.bottom;
	if(static_cast<uint>(abs(dy)) >= getNumberOfVisibleLines())
		invalidateRect(&clipRect, false);	// スクロール量が 1 ページ以上であれば全体を再描画
	else if(dx == 0)	// 垂直方向のみ
		scrollEx(0, -dy * getScrollRate(false) * renderer_->getLinePitch(), 0, &clipRect, 0, 0, SW_INVALIDATE);
	else {	// 先行マージンと編集領域を分けて処理する
		// 編集領域のスクロール
		clipRect.left += margins.left;
		clipRect.right -= margins.right;
		if(static_cast<uint>(abs(dx)) >= getNumberOfVisibleColumns())
			invalidateRect(&clipRect, false);	// スクロール量が 1 ページ以上であれば全体を再描画
		else
			scrollEx(-dx * getScrollRate(true) * renderer_->getAverageCharacterWidth(),
				-dy * getScrollRate(false) * renderer_->getLinePitch(), 0, &clipRect, 0, 0, SW_INVALIDATE);
		// 垂直ルーラのスクロール
		if(dy != 0) {
			if(verticalRulerDrawer_->getConfiguration().alignment == ALIGN_LEFT) {
				clipRect.left = clientRect.left;
				clipRect.right = clipRect.left + verticalRulerDrawer_->getWidth();
			} else {
				clipRect.right = clientRect.right;
				clipRect.left = clipRect.right - verticalRulerDrawer_->getWidth();
			}
			scrollEx(0, -dy * getScrollRate(false) * renderer_->getLinePitch(), 0, &clipRect, 0, 0, SW_INVALIDATE);
		}
	}

	// 後処理
	updateCaretPosition();
	if(imeCompositionActivated_)
		updateIMECompositionWindowPosition();
	if(redraw)
		update();
	viewportListeners_.notify<bool, bool>(IViewportListener::viewportChanged, dx != 0, dy != 0);
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
	if(line >= getDocument().getNumberOfLines())
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
			visualLine += renderer_->getNumberOfSublinesOfLine(i);
	}
	viewportListeners_.notify<bool, bool>(IViewportListener::viewportChanged, true, true);
}

/// @see ICaretListener#selectionShapeChanged
void TextViewer::selectionShapeChanged(const Caret& self) {
	if(!isFrozen() && !self.isSelectionEmpty())
		redrawLines(self.getTopPoint().getLineNumber(), self.getBottomPoint().getLineNumber());
}

/**
 * Updates the configurations.
 * @param general the general configurations. @c null to unchange
 * @param verticalRuler	the configurations about the vertical ruler. @c null to unchange
 * @throw std#invalid_argument the content of @a verticalRuler is invalid
 */
void TextViewer::setConfiguration(const Configuration* general, const VerticalRulerConfiguration* verticalRuler) {
	if(verticalRuler != 0) {
		if(!verticalRuler->verify())
			throw invalid_argument("The content of `verticalRuler' is invalid.");
		verticalRulerDrawer_->setConfiguration(*verticalRuler);
	}
	if(general != 0) {
		configuration_ = *general;
		renderer_->updateViewerSize();
		renderer_->invalidate();
		scrollInfo_.resetBars(*this, SB_BOTH, false);
		updateScrollBars();
#ifndef ASCENSION_NO_DOUBLE_BUFFERING
		updateMemoryDeviceContext();
#endif /* !ASCENSION_NO_DOUBLE_BUFFERING */

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
#endif /* !ASCENSION_NO_TEXT_SERVICES_FRAMEWORK */

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
		for(CloneIterator i(*this); !i.isEnd(); i.next()) {
			if(i->freezeInfo_.count > 0 && --i->freezeInfo_.count == 0)
				i->internalUnfreeze();
		}
	}
}

#ifndef ASCENSION_NO_DOUBLE_BUFFERING
/// Updates the compatible device context used by double buffering.
void TextViewer::updateMemoryDeviceContext() {
	if(memDC_.get() == 0)	// まだ準備ができてない...
		return;

	::BITMAP bitmap;
	bool needRecreate = false;
	::RECT rect;

	getClientRect(rect);
	if(::GetObject(lineBitmap_, sizeof(::BITMAP), &bitmap) != 0) {
		if(bitmap.bmWidth < rect.right - rect.left	// 既存のビットマップが小さい場合
				|| bitmap.bmHeight < renderer_->getLinePitch())
			needRecreate = true;
		else if(bitmap.bmWidth / 2 > rect.right - rect.left	// 既存のビットマップが大きすぎる場合
				|| bitmap.bmHeight / 2 > renderer_->getLinePitch())
			needRecreate = true;
	} else
		needRecreate = true;

	if(needRecreate) {
		memDC_->selectObject(oldLineBitmap_);
		lineBitmap_.deleteObject();
		lineBitmap_.createCompatibleBitmap(getDC(), rect.right - rect.left + 20, renderer_->getLinePitch());	// 少し大きめに...
		memDC_->selectObject(lineBitmap_);
	}
}
#endif /* !ASCENSION_NO_DOUBLE_BUFFERING */

/// Moves the caret to valid position with current position, scroll context, and the fonts.
void TextViewer::updateCaretPosition() {
	if(!hasFocus() || isFrozen())
		return;

	::POINT pt = getClientXYForCharacter(getCaret(), LineLayout::LEADING);
	const ::RECT margins = getTextAreaMargins();
	::RECT textArea;
	getClientRect(textArea);
	textArea.left += margins.left; textArea.top += margins.top;
	textArea.right -= margins.right - 1; textArea.bottom -= margins.bottom;

	if(!toBoolean(::PtInRect(&textArea, pt)))	// キャレットを「隠す」
		pt.y = -renderer_->getLinePitch();
	else if(caretShape_.orientation == RIGHT_TO_LEFT
			|| renderer_->getLineLayout(caret_->getLineNumber()).getBidiEmbeddingLevel(caret_->getColumnNumber()) % 2 == 1)
		pt.x -= caretShape_.width;
	setCaretPosition(pt);
}

/// Moves the IME form to valid position.
void TextViewer::updateIMECompositionWindowPosition() {
	assertValidAsWindow();
	if(!imeCompositionActivated_)
		return;

	::COMPOSITIONFORM cf;
	if(HIMC imc = ::ImmGetContext(get())) {
		cf.dwStyle = CFS_POINT;
		cf.ptCurrentPos = getClientXYForCharacter(caret_->getTopPoint(), LineLayout::LEADING);
//		cf.ptCurrentPos.x -= 1;
		cf.ptCurrentPos.y -= 1;
		::ImmSetCompositionWindow(imc, &cf);
		::ImmReleaseContext(get(), imc);
	}
}

/// Updates the scroll information.
void TextViewer::updateScrollBars() {
	assertValidAsWindow();
	if(renderer_.get() == 0)
		return;

#define GET_SCROLL_MINIMUM(s)	(s.maximum/* * s.rate*/ - s.pageSize + 1)

	// 水平スクロールバー
	bool wasNeededScrollbar = GET_SCROLL_MINIMUM(scrollInfo_.horizontal) > 0;
	// スクロールバーが無くなる前に左端 (右端) にスクロールさせる
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
		AutoZeroCB<::SCROLLINFO> scroll;
		scroll.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
		scroll.nMax = configuration_.lineWrap.wrapsAtWindowEdge() ? 0 : scrollInfo_.horizontal.maximum;
		scroll.nPage = scrollInfo_.horizontal.pageSize;
		scroll.nPos = scrollInfo_.horizontal.position;
		setScrollInformation(SB_HORZ, scroll, true);
	}

	// 垂直スクロールバー
	wasNeededScrollbar = GET_SCROLL_MINIMUM(scrollInfo_.vertical) > 0;
	minimum = GET_SCROLL_MINIMUM(scrollInfo_.vertical);
	// 変なスクロール位置にならないようにする
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
		AutoZeroCB<::SCROLLINFO> scroll;
		scroll.fMask = SIF_DISABLENOSCROLL | SIF_PAGE | SIF_POS | SIF_RANGE;
		scroll.nMax = scrollInfo_.vertical.maximum;
		scroll.nPage = scrollInfo_.vertical.pageSize;
		scroll.nPos = scrollInfo_.vertical.position;
		setScrollInformation(SB_VERT, scroll, true);
	}

	scrollInfo_.changed = isFrozen();

#undef GET_SCROLL_MINIMUM
}

#define RECALC_VERTICAL_SCROLL()																						\
	scrollInfo_.vertical.maximum = static_cast<int>(renderer_->getNumberOfVisualLines());								\
	scrollInfo_.firstVisibleLine = min(scrollInfo_.firstVisibleLine, getDocument().getNumberOfLines() - 1);				\
	scrollInfo_.firstVisibleSubline =																					\
		min(renderer_->getNumberOfSublinesOfLine(scrollInfo_.firstVisibleLine) - 1, scrollInfo_.firstVisibleSubline);	\
	scrollInfo_.vertical.position =																						\
		static_cast<int>(renderer_->mapLogicalLineToVisualLine(scrollInfo_.firstVisibleLine) + scrollInfo_.firstVisibleSubline)

/// @see IVisualLinesListener#visualLinesDeleted
void TextViewer::visualLinesDeleted(length_t first, length_t last, length_t sublines, bool longestLineChanged) throw() {
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
		RECALC_VERTICAL_SCROLL();
		redrawLine(first, true);
	}
	if(longestLineChanged)
		scrollInfo_.resetBars(*this, SB_HORZ, false);
}

/// @see IVisualLinesListener#visualLinesInserted
void TextViewer::visualLinesInserted(length_t first, length_t last) throw() {
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
		RECALC_VERTICAL_SCROLL();
		redrawLine(first, true);
	}
}

/// @see IVisualLinesListener#visualLinesModified
void TextViewer::visualLinesModified(length_t first, length_t last,
		signed_length_t sublinesDifference, bool documentChanged, bool longestLineChanged) throw() {
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
			RECALC_VERTICAL_SCROLL();
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

#undef RECALC_VERTICAL_SCROLL


// Viewer::AccessibleProxy ////////////////////////////////////////////////

#ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY
#define VERIFY_AVAILABILITY()	\
	if(!available_) return RPC_E_DISCONNECTED

/**
 * Constructor.
 * @param view the viewer
 */
TextViewer::AccessibleProxy::AccessibleProxy(TextViewer& view) throw() : view_(view), available_(true) {
	assert(accLib.isAvailable());
	accLib.createStdAccessibleObject(view.get(), OBJID_CLIENT, IID_IAccessible, &defaultServer_);
}

/// @see IAccessible#accDoDefaultAction
STDMETHODIMP TextViewer::AccessibleProxy::accDoDefaultAction(VARIANT) {
	VERIFY_AVAILABILITY();
	return DISP_E_MEMBERNOTFOUND;
}

/// @see IAccessible#accHitTest
STDMETHODIMP TextViewer::AccessibleProxy::accHitTest(long xLeft, long yTop, VARIANT* pvarChild) {
	VERIFY_AVAILABILITY();
	// ウィンドウが矩形であることを前提としている
	VERIFY_POINTER(pvarChild);
	::POINT pt = {xLeft, yTop};
	::RECT rect;
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
STDMETHODIMP TextViewer::AccessibleProxy::accLocation(long* pxLeft, long* pyTop, long* pcxWidth, long* pcyHeight, VARIANT varChild) {
	VERIFY_AVAILABILITY();
	VERIFY_POINTER(pxLeft);
	VERIFY_POINTER(pyTop);
	VERIFY_POINTER(pcxWidth);
	VERIFY_POINTER(pcyHeight);
	if(varChild.vt != VT_I4 || varChild.lVal != CHILDID_SELF)
		return E_INVALIDARG;
	::RECT rect;
	view_.getClientRect(rect);
	view_.clientToScreen(rect);
	*pxLeft = rect.left;
	*pyTop = rect.top;
	*pcxWidth = rect.right - rect.left;
	*pcyHeight = rect.bottom - rect.top;
	return S_OK;
}

/// @see IAccessible#accNavigate
STDMETHODIMP TextViewer::AccessibleProxy::accNavigate(long navDir, VARIANT varStart, VARIANT* pvarEndUpAt) {
	VERIFY_AVAILABILITY();
	return defaultServer_->accNavigate(navDir, varStart, pvarEndUpAt);
}

/// @see IAccessible#accSelect
STDMETHODIMP TextViewer::AccessibleProxy::accSelect(long flagsSelect, VARIANT varChild) {
	VERIFY_AVAILABILITY();
	return (varChild.vt == VT_I4 && varChild.lVal == CHILDID_SELF) ?
		defaultServer_->accSelect(flagsSelect, varChild) : E_INVALIDARG;
}

/// Informs that the viewer is inavailable to the proxy.
void TextViewer::AccessibleProxy::dispose() {
	if(!available_)
		throw logic_error("This proxy is already disposed.");
	available_ = false;
}

/// @see Document#IListener#documentAboutToBeChanged
void TextViewer::AccessibleProxy::documentAboutToBeChanged(const Document&) {
}

/// @see Document#IListener#documentChanged
void TextViewer::AccessibleProxy::documentChanged(const Document&, const DocumentChange&) {
	assert(accLib.isAvailable());
	accLib.notifyWinEvent(EVENT_OBJECT_VALUECHANGE, view_.get(), OBJID_CLIENT, CHILDID_SELF);
}

/// @see IAccessible#get_accChild
STDMETHODIMP TextViewer::AccessibleProxy::get_accChild(VARIANT varChild, IDispatch** ppdispChild) {
	VERIFY_AVAILABILITY();
	VERIFY_POINTER(ppdispChild);
	*ppdispChild = 0;
	return S_OK;
}

/// @see IAccessible#get_accChildCount
STDMETHODIMP TextViewer::AccessibleProxy::get_accChildCount(long* pcountChildren) {
	VERIFY_AVAILABILITY();
	VERIFY_POINTER(pcountChildren);
	*pcountChildren = 0;
	return S_OK;
}

/// @see IAccessible#get_accDefaultAction
STDMETHODIMP TextViewer::AccessibleProxy::get_accDefaultAction(VARIANT, BSTR*) {
	VERIFY_AVAILABILITY();
	return DISP_E_MEMBERNOTFOUND;
}

/// @see IAccessible#get_accDescription
STDMETHODIMP TextViewer::AccessibleProxy::get_accDescription(VARIANT, BSTR*) {
	VERIFY_AVAILABILITY();
	return DISP_E_MEMBERNOTFOUND;
}

/// @see IAccessible#get_accFocus
STDMETHODIMP TextViewer::AccessibleProxy::get_accFocus(VARIANT* pvarChild) {
	VERIFY_AVAILABILITY();
	VERIFY_POINTER(pvarChild);
	pvarChild->vt = VT_I4;
	pvarChild->lVal = CHILDID_SELF;
	return S_OK;
}

/// @see IAccessible#get_accHelp
STDMETHODIMP TextViewer::AccessibleProxy::get_accHelp(VARIANT, BSTR*) {
	VERIFY_AVAILABILITY();
	return DISP_E_MEMBERNOTFOUND;
}

/// @see IAccessible#get_accHelpTopic
STDMETHODIMP TextViewer::AccessibleProxy::get_accHelpTopic(BSTR*, VARIANT, long*) {
	VERIFY_AVAILABILITY();
	return DISP_E_MEMBERNOTFOUND;
}

/// @see IAccessible#get_accKeyboardShortcut
STDMETHODIMP TextViewer::AccessibleProxy::get_accKeyboardShortcut(VARIANT varChild, BSTR* pszKeyboardShortcut) {
	VERIFY_AVAILABILITY();
	VERIFY_POINTER(pszKeyboardShortcut);
	*pszKeyboardShortcut = 0;
	if(varChild.vt != VT_I4 || varChild.lVal != CHILDID_SELF)
		return E_INVALIDARG;
	return S_FALSE;
}

/// @see IAccessible#get_accName
STDMETHODIMP TextViewer::AccessibleProxy::get_accName(VARIANT varChild, BSTR* pszName) {
	VERIFY_AVAILABILITY();
	VERIFY_POINTER(pszName);
	*pszName = 0;
	if(varChild.vt != VT_I4 || varChild.lVal != CHILDID_SELF)
		return E_INVALIDARG;
	return S_FALSE;
}

/// @see IAccessible#get_accParent
STDMETHODIMP TextViewer::AccessibleProxy::get_accParent(IDispatch** ppdispParent) {
	VERIFY_AVAILABILITY();
	if(accLib.isAvailable())
		return accLib.accessibleObjectFromWindow(view_.get(), OBJID_WINDOW, IID_IAccessible, reinterpret_cast<void**>(ppdispParent));
	return defaultServer_->get_accParent(ppdispParent);
}

/// @see IAccessible#get_accRole
STDMETHODIMP TextViewer::AccessibleProxy::get_accRole(VARIANT varChild, VARIANT* pvarRole) {
	VERIFY_AVAILABILITY();
	VERIFY_POINTER(pvarRole);
	if(varChild.vt != VT_I4 || varChild.lVal != CHILDID_SELF)
		return E_INVALIDARG;
	pvarRole->vt = VT_I4;
	pvarRole->lVal = ROLE_SYSTEM_TEXT;
	return S_OK;
}

/// @see IAccessible#get_accSelection
STDMETHODIMP TextViewer::AccessibleProxy::get_accSelection(VARIANT* pvarChildren) {
	VERIFY_AVAILABILITY();
	VERIFY_POINTER(pvarChildren);
	pvarChildren->vt = VT_EMPTY;
	return S_FALSE;
}

/// @see IAccessible#get_accState
STDMETHODIMP TextViewer::AccessibleProxy::get_accState(VARIANT varChild, VARIANT* pvarState) {
	VERIFY_AVAILABILITY();
	if(varChild.vt != VT_I4 || varChild.lVal != CHILDID_SELF)
		return E_INVALIDARG;
	pvarState->vt = VT_I4;
	pvarState->lVal = 0;	// STATE_SYSTEM_NORMAL;
	if(!view_.isVisible())
		pvarState->lVal |= STATE_SYSTEM_INVISIBLE;
	if(view_.getTop()->get() == ::GetActiveWindow())
		pvarState->lVal |= STATE_SYSTEM_FOCUSABLE;
	if(view_.hasFocus())
		pvarState->lVal |= STATE_SYSTEM_FOCUSED;
	if(view_.getDocument().isReadOnly())
		pvarState->lVal |= STATE_SYSTEM_READONLY;
	return S_OK;
}

/// @see IAccessible#get_accValue
STDMETHODIMP TextViewer::AccessibleProxy::get_accValue(VARIANT varChild, BSTR* pszValue) {
	VERIFY_AVAILABILITY();
	VERIFY_POINTER(pszValue);
	if(varChild.vt != VT_I4 || varChild.lVal != CHILDID_SELF)
		return E_INVALIDARG;
	OutputStringStream s;
	view_.getDocument().writeToStream(s);
	*pszValue = ::SysAllocString(s.str().c_str());
	return (*pszValue != 0) ? S_OK : E_OUTOFMEMORY;
}

/// @see IAccessible#put_accName
STDMETHODIMP TextViewer::AccessibleProxy::put_accName(VARIANT, BSTR) {
	VERIFY_AVAILABILITY();
	return DISP_E_MEMBERNOTFOUND;
}

/// @see IAccessible#put_accValue
STDMETHODIMP TextViewer::AccessibleProxy::put_accValue(VARIANT varChild, BSTR szValue) {
	VERIFY_AVAILABILITY();
	if(varChild.vt != VT_I4 || varChild.lVal != CHILDID_SELF)
		return E_INVALIDARG;
	else if(view_.getDocument().isReadOnly())
		return E_ACCESSDENIED;
	view_.getCaret().replaceSelection(safeBSTR(szValue));
	return S_OK;
}

#undef VERIFY_AVAILABILITY
#endif /* !ASCENSION_NO_ACTIVE_ACCESSIBILITY */


// TextViewer::AutoScrollOriginMark /////////////////////////////////////////

const long TextViewer::AutoScrollOriginMark::WINDOW_WIDTH = 28;

/**
 * Creates the window.
 * @param view the viewer
 * @return succeeded or not
 * @see Window#create
 */
bool TextViewer::AutoScrollOriginMark::create(const TextViewer& view) {
	HINSTANCE hinstance = reinterpret_cast<HINSTANCE>(static_cast<HANDLE_PTR>(::GetWindowLongPtr(get(), GWLP_HINSTANCE)));
	RECT rc = {0, 0, WINDOW_WIDTH + 1, WINDOW_WIDTH + 1};

	if(!ui::CustomControl<AutoScrollOriginMark>::create(view.get(),
			rc, 0, WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP, WS_EX_TOOLWINDOW))
		return false;
	modifyStyleEx(0, WS_EX_LAYERED);	// いきなり CreateWindowEx(WS_EX_LAYERED) とすると NT 4.0 で失敗する

	HRGN rgn = ::CreateEllipticRgn(0, 0, WINDOW_WIDTH + 1, WINDOW_WIDTH + 1);
	setRegion(rgn, false);
	::DeleteObject(rgn);
	setLayeredAttributes(::GetSysColor(COLOR_WINDOW), 0, LWA_COLORKEY);

	return true;
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


// TextViewer::VerticalRulerDrawer //////////////////////////////////////////

namespace {
	struct ABCComparer : binary_function<bool, ::ABC, ::ABC> {
		bool operator()(const ::ABC& lhs, const ::ABC& rhs) const throw() {
			return lhs.abcA + lhs.abcB + lhs.abcC < rhs.abcA + rhs.abcB + rhs.abcC;
		}
	};
} // namespace @0

/**
 * Constructor.
 * @param viewer the viewer
 */
TextViewer::VerticalRulerDrawer::VerticalRulerDrawer(TextViewer& viewer) : viewer_(viewer), width_(0), lineNumberDigitsCache_(0) {
	recalculateWidth();
}

/**
 * Draws the vertical ruler.
 * @param dc the device context
 */
void TextViewer::VerticalRulerDrawer::draw(PaintDC& dc) {
	if(getWidth() == 0)
		return;

	const ::RECT& paintRect = dc.getPaintStruct().rcPaint;
	const Presentation& presentation = viewer_.getPresentation();
	const TextRenderer& renderer = viewer_.getTextRenderer();
	::RECT clientRect;
	viewer_.getClientRect(clientRect);
	if((configuration_.alignment == ALIGN_LEFT && paintRect.left >= clientRect.left + getWidth())
			|| (configuration_.alignment == ALIGN_RIGHT && paintRect.right < clientRect.right - getWidth()))
		return;

#ifdef _DEBUG
	if(DIAGNOSE_INHERENT_DRAWING)
		DumpContext() << L"ruler rect : " << paintRect.top << L" ... " << paintRect.bottom << L"\n";
#endif /* _DEBUG */

	const int savedCookie = dc.save();
	const int imWidth = configuration_.indicatorMargin.visible ? configuration_.indicatorMargin.width : 0;

	// まず、描画領域全体を描いておく
	if(configuration_.indicatorMargin.visible) {
		// インジケータマージンの境界線と内側
		HPEN oldPen = dc.selectObject(indicatorMarginPen_);
		HBRUSH oldBrush = dc.selectObject(indicatorMarginBrush_);
		dc.patBlt((configuration_.alignment == ALIGN_LEFT) ?
			clientRect.left : clientRect.right - imWidth, paintRect.top, imWidth, paintRect.bottom - paintRect.top, PATCOPY);
		dc.moveTo((configuration_.alignment == ALIGN_LEFT) ? clientRect.left + imWidth - 1 : clientRect.right - imWidth, paintRect.top);
		dc.lineTo((configuration_.alignment == ALIGN_LEFT) ? clientRect.left + imWidth - 1 : clientRect.right - imWidth, paintRect.bottom);
		dc.selectObject(oldPen);
		dc.selectObject(oldBrush);
	}
	if(configuration_.lineNumbers.visible) {
		// 行番号の背景
		HBRUSH oldBrush = dc.selectObject(lineNumbersBrush_);
		dc.patBlt((configuration_.alignment == ALIGN_LEFT) ?
			clientRect.left + imWidth : clientRect.right - getWidth(), paintRect.top, getWidth() - imWidth, paintRect.bottom, PATCOPY);
		// 行番号の境界線
		if(configuration_.lineNumbers.borderStyle != VerticalRulerConfiguration::LineNumbers::NONE) {
			HPEN oldPen = dc.selectObject(lineNumbersPen_);
			const int x = ((configuration_.alignment == ALIGN_LEFT) ?
				clientRect.left + getWidth() : clientRect.right - getWidth() + 1) - configuration_.lineNumbers.borderWidth;
			dc.moveTo(x, paintRect.top);
			dc.lineTo(x, paintRect.bottom);
			dc.selectObject(oldPen);
		}
		dc.selectObject(oldBrush);

		// 次の準備...
		dc.setBkMode(TRANSPARENT);
		dc.setTextColor(configuration_.lineNumbers.textColor.foreground);
		dc.setTextCharacterExtra(0);	// 行番号表示は文字間隔の設定を無視
		dc.selectObject(viewer_.getTextRenderer().getFont());
	}

	// 行番号描画の準備
	Alignment lineNumbersAlignment;
	int lineNumbersX;
	if(configuration_.lineNumbers.visible) {
		lineNumbersAlignment = configuration_.lineNumbers.alignment;
		if(lineNumbersAlignment == ALIGN_AUTO)
			lineNumbersAlignment = (configuration_.alignment == ALIGN_LEFT) ? ALIGN_RIGHT : ALIGN_LEFT;
		switch(lineNumbersAlignment) {
		case ALIGN_LEFT:
			lineNumbersX = (configuration_.alignment == ALIGN_LEFT) ?
				clientRect.left + imWidth + configuration_.lineNumbers.leadingMargin
				: clientRect.right - getWidth() + configuration_.lineNumbers.trailingMargin + 1;
			dc.setTextAlign(TA_LEFT | TA_TOP | TA_NOUPDATECP);
			break;
		case ALIGN_RIGHT:
			lineNumbersX = ((configuration_.alignment == ALIGN_LEFT) ?
				clientRect.left + getWidth() - configuration_.lineNumbers.trailingMargin
				: clientRect.right - imWidth - configuration_.lineNumbers.leadingMargin);
			dc.setTextAlign(TA_RIGHT | TA_TOP | TA_NOUPDATECP);
			break;
		case ALIGN_CENTER:	// 中央揃えなんて誰も使わんと思うけど...
			lineNumbersX = (configuration_.alignment == ALIGN_LEFT) ?
				clientRect.left + (imWidth + configuration_.lineNumbers.leadingMargin + getWidth() - configuration_.lineNumbers.trailingMargin) / 2
				: clientRect.right - (getWidth() - configuration_.lineNumbers.trailingMargin + imWidth + configuration_.lineNumbers.leadingMargin) / 2;
			dc.setTextAlign(TA_CENTER | TA_TOP | TA_NOUPDATECP);
			break;
		}
	}

	// 1行ずつ細かい描画
//	const int scrollOffset = viewer_.getScrollPos(SB_VERT) * viewer_.getScrollRate(false);
	length_t line, visualSublineOffset;
	const length_t lines = viewer_.getDocument().getNumberOfLines();
	viewer_.mapClientYToLine(paintRect.top, &line, &visualSublineOffset);	// $friendly-access
	int y = paintRect.top - (paintRect.top - viewer_.getConfiguration().topMargin) % renderer.getLinePitch();
	if(visualSublineOffset > 0)	// 描画開始は次の論理行から...
		y += static_cast<int>(renderer.getLineLayout(line++).getNumberOfSublines() - visualSublineOffset) * renderer.getLinePitch();
	while(y < paintRect.bottom && line < lines) {
		// 派生クラスにインジケータマージンの描画機会を与える
		if(configuration_.indicatorMargin.visible) {
			::RECT rect = {
				(configuration_.alignment == ALIGN_LEFT) ? clientRect.left : clientRect.right - configuration_.indicatorMargin.width,
				y, (configuration_.alignment == ALIGN_LEFT) ? clientRect.left + configuration_.indicatorMargin.width : clientRect.right,
				y + renderer.getLinePitch()};
			viewer_.drawIndicatorMargin(line, dc, rect);
		}

		// 行番号の描画
		if(configuration_.lineNumbers.visible) {
			wchar_t buffer[32];
			swprintf(buffer, L"%lu", line + configuration_.lineNumbers.startValue);
			dc.textOut(lineNumbersX, y, buffer, static_cast<int>(wcslen(buffer)));
		}
		y += static_cast<int>(renderer.getLineLayout(line++).getNumberOfSublines() * renderer.getLinePitch());
	}
	dc.restore(savedCookie);
}

/// ドキュメントの現在の最終行番号が10進数で何文字になるかを返す
uchar TextViewer::VerticalRulerDrawer::getLineNumberMaxDigits() const throw() {
	uint n = 1;
	length_t lines = viewer_.getDocument().getNumberOfLines() + configuration_.lineNumbers.startValue - 1;
	while(lines >= 10) {
		lines /= 10;
		++n;
	}
	return n;
}

void TextViewer::VerticalRulerDrawer::recalculateWidth() throw() {
	int newWidth = 0;	// 新しい幅
	if(configuration_.lineNumbers.visible) {
		const uchar newLineNumberDigits = getLineNumberMaxDigits();
		if(newLineNumberDigits != lineNumberDigitsCache_) {
			// 行番号表示領域の幅は '0' から '9' で最大幅を持つグリフで決める
			// (設定によってはネイティブの代替グリフが選択され、意外に幅を食う場合がある)
			ClientDC dc = viewer_.getDC();
			int maxGlyphWidth = 0;
			int glyphWidths[10];
			HFONT oldFont = dc.selectObject(viewer_.getTextRenderer().getFont());
			if(dc.getCharWidth(L'0', L'9', glyphWidths))
				maxGlyphWidth = *max_element(glyphWidths, endof(glyphWidths));
			else {
				::ABC glyphABCWidths[10];
				if(dc.getCharABCWidths(L'0', L'9', glyphABCWidths)) {
					const ::ABC* const p = max_element(glyphABCWidths, endof(glyphABCWidths), ABCComparer());
					maxGlyphWidth = p->abcA + p->abcB + p->abcC;
				}
			}
			dc.selectObject(oldFont);
			lineNumberDigitsCache_ = newLineNumberDigits;
			if(maxGlyphWidth != 0) {
				newWidth += max<uchar>(newLineNumberDigits, configuration_.lineNumbers.minimumDigits) * maxGlyphWidth;
				newWidth += configuration_.lineNumbers.leadingMargin + configuration_.lineNumbers.trailingMargin;
				if(configuration_.lineNumbers.borderStyle != VerticalRulerConfiguration::LineNumbers::NONE)
					newWidth += configuration_.lineNumbers.borderWidth;
			}
		}
	}
	if(configuration_.indicatorMargin.visible)
		newWidth += configuration_.indicatorMargin.width;
	if(newWidth != width_) {
		width_ = newWidth;
		viewer_.invalidateRect(0, false);
		viewer_.updateCaretPosition();
	}
}

void TextViewer::VerticalRulerDrawer::setConfiguration(const VerticalRulerConfiguration& configuration) {
	if(!configuration.lineNumbers.verify())
		throw invalid_argument("Any member of the specified VerticalRulerConfiguration is invalid.");
	configuration_ = configuration;
	update();
}

void TextViewer::VerticalRulerDrawer::update() throw() {
	lineNumberDigitsCache_ = 0;
	recalculateWidth();
	updateGDIObjects();
}

///
void TextViewer::VerticalRulerDrawer::updateGDIObjects() throw() {
	using internal::systemColors;

	indicatorMarginPen_.deleteObject();
	indicatorMarginBrush_.deleteObject();
	if(configuration_.indicatorMargin.visible) {
		indicatorMarginPen_.createPen(PS_SOLID, 1,
			systemColors.getReal(configuration_.indicatorMargin.borderColor, SYSTEM_COLOR_MASK | COLOR_3DSHADOW));
		indicatorMarginBrush_.createSolidBrush(systemColors.getReal(configuration_.indicatorMargin.color, SYSTEM_COLOR_MASK | COLOR_3DFACE));
	}

	lineNumbersPen_.deleteObject();
	lineNumbersBrush_.deleteObject();
	if(configuration_.lineNumbers.visible) {
		if(configuration_.lineNumbers.borderStyle == VerticalRulerConfiguration::LineNumbers::SOLID)	// 実線
			lineNumbersPen_.createPen(PS_SOLID, configuration_.lineNumbers.borderWidth,
				systemColors.getReal(configuration_.lineNumbers.borderColor, SYSTEM_COLOR_MASK | COLOR_WINDOWTEXT));
		else if(configuration_.lineNumbers.borderStyle != VerticalRulerConfiguration::LineNumbers::NONE) {
			::LOGBRUSH brush;
			brush.lbColor = systemColors.getReal(configuration_.lineNumbers.borderColor, SYSTEM_COLOR_MASK | COLOR_WINDOWTEXT);
			brush.lbStyle = BS_SOLID;
			if(configuration_.lineNumbers.borderStyle == VerticalRulerConfiguration::LineNumbers::DASHED)	// 破線
				lineNumbersPen_.createPen(PS_GEOMETRIC | PS_DASH | PS_ENDCAP_FLAT, configuration_.lineNumbers.borderWidth, brush, 0, 0);
			else if(configuration_.lineNumbers.borderStyle == VerticalRulerConfiguration::LineNumbers::DASHED_ROUNDED)	// 丸破線
				lineNumbersPen_.createPen(PS_GEOMETRIC | PS_DASH | PS_ENDCAP_ROUND, configuration_.lineNumbers.borderWidth, brush, 0, 0);
			else if(configuration_.lineNumbers.borderStyle == VerticalRulerConfiguration::LineNumbers::DOTTED)	// 点線
				lineNumbersPen_.createPen(PS_GEOMETRIC | PS_DOT, configuration_.lineNumbers.borderWidth, brush, 0, 0);
		}
		lineNumbersBrush_.createSolidBrush(systemColors.getReal(
			configuration_.lineNumbers.textColor.background, SYSTEM_COLOR_MASK | COLOR_WINDOW));
	}
}

#undef EXTEND_SELECTION
#undef RESTORE_HIDDEN_CURSOR


// TextViewer::ScrollInfo ///////////////////////////////////////////////////

void TextViewer::ScrollInfo::resetBars(const TextViewer& viewer, int bars, bool pageSizeChanged) throw() {
	// 水平方向
	if(bars == SB_HORZ || bars == SB_BOTH) {
		// テキストが左揃えでない場合は、スクロールボックスの位置を補正する必要がある
		// (ウィンドウが常に LTR である仕様のため)
		const Alignment alignment = viewer.getConfiguration().alignment;
		const ulong columns =
			(!viewer.getConfiguration().lineWrap.wrapsAtWindowEdge()) ?
				viewer.getTextRenderer().getLongestLineWidth() / viewer.getTextRenderer().getAverageCharacterWidth() : 0;
//		horizontal.rate = columns / numeric_limits<int>::max() + 1;
//		assert(horizontal.rate != 0);
		const int oldMaximum = horizontal.maximum;
		horizontal.maximum = max(static_cast<int>(columns/* / horizontal.rate*/), static_cast<int>(viewer.getNumberOfVisibleColumns() - 1));
		if(alignment == ALIGN_RIGHT)
			horizontal.position += horizontal.maximum - oldMaximum;
		else if(alignment == ALIGN_CENTER)
			horizontal.position += (horizontal.maximum - oldMaximum) / 2;
		horizontal.position = max(horizontal.position, 0);
		if(pageSizeChanged) {
			const UINT oldPageSize = horizontal.pageSize;
			horizontal.pageSize = static_cast<UINT>(viewer.getNumberOfVisibleColumns());
			if(alignment == ALIGN_RIGHT)
				horizontal.position += horizontal.pageSize - oldPageSize;
			else if(alignment == ALIGN_CENTER)
				horizontal.position += (horizontal.pageSize - oldPageSize) / 2;
			horizontal.position = max(horizontal.position, 0);
		}
	}
	// 垂直方向
	if(bars == SB_VERT || bars == SB_BOTH) {
		const length_t lines = viewer.getTextRenderer().getNumberOfVisualLines();
//		vertical.rate = static_cast<ulong>(lines) / numeric_limits<int>::max() + 1;
//		assert(vertical.rate != 0);
		vertical.maximum = max(static_cast<int>(lines/* / vertical.rate*/), 0/*static_cast<int>(viewer.getVisibleLineCount() - 1)*/);
		if(pageSizeChanged)
			vertical.pageSize = static_cast<UINT>(viewer.getNumberOfVisibleLines());
	}
}


// SourceViewer /////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param presentation the presentation
 */
SourceViewer::SourceViewer(Presentation& presentation) throw() : TextViewer(presentation) {
}

/**
 * Returns the identifier near the specified position
 * @param position the position
 * @param[out] startChar the start of the identifier. can be @c null if not needed
 * @param[out] endChar the end of the identifier. can be @c null if not needed
 * @param[out] identifier the string of the found identifier. can be @c null if not needed
 * @return false if the identifier is not found (in this case, the values of the output parameters are undefined)
 * @see #getPointedIdentifier, Caret#getPrecedingIdentifier
 */
bool SourceViewer::getNearestIdentifier(const Position& position, length_t* startChar, length_t* endChar, String* identifier) const {
	using namespace unicode;

	DocumentPartition partition;
	getDocument().getPartitioner().getPartition(position, partition);
	const IdentifierSyntax& syntax = getDocument().getContentTypeInformation().getIdentifierSyntax(partition.contentType);
	length_t startColumn = position.column, endColumn = position.column;
	const String& line = getDocument().getLine(position.line);
	CodePoint cp;

	// 開始位置を調べる
	if(startChar != 0 || identifier != 0) {
		const length_t partitionStart = (position.line == partition.region.getTop().line) ? partition.region.getTop().column : 0;
		while(startColumn > partitionStart) {
			cp = surrogates::decodeLast(line.begin(), line.begin() + startColumn);
			if(syntax.isIdentifierContinueCharacter(cp))
				startColumn -= ((cp >= 0x010000) ? 2 : 1);
			else
				break;
		}
		if(startChar!= 0)
			*startChar = startColumn;
	}

	// 終了位置を調べる
	if(endChar != 0 || identifier != 0) {
		while(true) {
			cp = surrogates::decodeFirst(line.begin() + endColumn, line.end());
			if(syntax.isIdentifierContinueCharacter(cp))
				endColumn += ((cp >= 0x010000) ? 2 : 1);
			else
				break;
		}
		if(endChar != 0)
			*endChar = endColumn;
	}

	if(identifier != 0)
		identifier->assign(line.substr(startColumn, endColumn - startColumn));
	return true;
}

/**
 * Returns the identifier near the cursor.
 * @param[out] startPosition the start of the identifier. can be @c null if not needed
 * @param[out] endPosition the end of the identifier. can be @c null if not needed
 * @param[out] identifier the string of the found identifier. can be @c null if not needed
 * @return false if the identifier is not found (in this case, the values of the output parameters are undefined)
 * @see #getNearestIdentifier, Caret#getPrecedingIdentifier
 */
bool SourceViewer::getPointedIdentifier(Position* startPosition, Position* endPosition, String* identifier) const {
	assertValidAsWindow();

	::POINT cursorPoint;
	::GetCursorPos(&cursorPoint);
	screenToClient(cursorPoint);
	const Position cursor = getCharacterForClientXY(cursorPoint, false);

	if(getNearestIdentifier(cursor,
			(startPosition != 0) ? &startPosition->column : 0, (endPosition != 0) ? &endPosition->column : 0, identifier)) {
		if(startPosition != 0)
			startPosition->line = cursor.line;
		if(endPosition != 0)
			endPosition->line = cursor.line;
		return true;
	}
	return false;
}


// VirtualBox ///////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param view the viewer
 * @param region the region consists the rectangle
 */
VirtualBox::VirtualBox(const TextViewer& view, const Region& region) throw() : view_(view) {
	update(region);
}

/**
 * Returns the range which the box overlaps with in specified visual line.
 * @param line the logical line
 * @param subline the visual subline
 * @param[out] first the start of range
 * @param[out] last the end of range
 * return true if the box and the visual line overlap
 */
bool VirtualBox::getOverlappedSubline(length_t line, length_t subline, length_t& first, length_t& last) const throw() {
	assert(view_.isWindow());
	const Point& top = getTop();
	const Point& bottom = getBottom();
	if(line < top.line || (line == top.line && subline < top.subline)	// 範囲外
			|| line > bottom.line || (line == bottom.line && subline > bottom.subline))
		return false;
	else {
		const TextRenderer& renderer = view_.getTextRenderer();
		const LineLayout& layout = renderer.getLineLayout(line);
		first = layout.getOffset(points_[0].x, static_cast<int>(renderer.getLinePitch() * subline));
		last = layout.getOffset(points_[1].x, static_cast<int>(renderer.getLinePitch() * subline));
		if(first > last)
			swap(first, last);
		return first != last;
	}
}

/**
 * Returns if the specified point is on the virtual box.
 * @param pt the client coordinates of the point
 * @return true if the point is on the virtual box
 */
bool VirtualBox::isPointOver(const ::POINT& pt) const throw() {
	assert(view_.isWindow());
	if(view_.hitTest(pt) != TextViewer::TEXT_AREA)	// テキスト表示領域でなければ無視
		return false;
	const int leftMargin = view_.getTextAreaMargins().left;
	if(pt.x < getLeft() + leftMargin || pt.x >= getRight() + leftMargin)	// x 方向
		return false;

	// y 方向
	const Point& top = getTop();
	const Point& bottom = getBottom();
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
 * Updates the rectangle of the virtual box.
 * @param region the region consists the rectangle
 */
void VirtualBox::update(const Region& region) throw() {
	const TextRenderer& r = view_.getTextRenderer();
	::POINT location = r.getLineLayout(points_[0].line = region.first.line).getLocation(region.first.column);
	points_[0].x = location.x;
	points_[0].subline = location.y / r.getLinePitch();
	location = r.getLineLayout(points_[1].line = region.second.line).getLocation(region.second.column);
	points_[1].x = location.x;
	points_[1].subline = location.y / r.getLinePitch();
}


// CaretShapeUpdater ////////////////////////////////////////////////////////

/**
 * Private constructor.
 * @param viewer the text viewer
 */
CaretShapeUpdater::CaretShapeUpdater(TextViewer& viewer) throw() : viewer_(viewer) {
}

/// Returns the text viewer.
TextViewer& CaretShapeUpdater::getTextViewer() throw() {
	return viewer_;
}

/// Notifies the text viewer to update the shape of the caret.
void CaretShapeUpdater::update() throw() {
	viewer_.recreateCaret();	// $friendly access$
}


// DefaultCaretShaper ///////////////////////////////////////////////////////

/// Constructor.
DefaultCaretShaper::DefaultCaretShaper() throw() : viewer_(0) {
}

/// @see ICaretShapeProvider#getCaretShape
void DefaultCaretShaper::getCaretShape(auto_ptr<Bitmap>&, ::SIZE& solidSize, Orientation& orientation) throw() {
	DWORD width;
	if(::SystemParametersInfo(SPI_GETCARETWIDTH, 0, &width, 0) == 0)
		width = 1;	// NT4 は SPI_GETCARETWIDTH が使えない
	solidSize.cx = width;
	solidSize.cy = viewer_->getTextRenderer().getLineHeight();
	orientation = LEFT_TO_RIGHT;	// どっちでもいい
}

/// @see ICaretShapeProvider#install
void DefaultCaretShaper::install(CaretShapeUpdater& updater) throw() {
	viewer_ = &updater.getTextViewer();
}

/// @see ICaretShapeProvider#uninstall
void DefaultCaretShaper::uninstall() throw() {
	viewer_ = 0;
}


// LocaleSensitiveCaretShaper ///////////////////////////////////////////////

namespace {
	/// Returns true if the specified language is RTL.
	inline bool isRTLLanguage(LANGID id) throw() {
		return id == LANG_ARABIC || id == LANG_FARSI || id == LANG_HEBREW || id == LANG_SYRIAC || id == LANG_URDU;
	}
	/// Returns true if the specified language is Thai or Lao.
	inline bool isTISLanguage(LANGID id) throw() {
		return id == LANG_THAI /*|| id == LANG_LAO*/;
	}
	/**
	 * Returns the bitmap has specified size.
	 * @param dc the device context
	 * @param width the width of the bitmap
	 * @param height the height of the bitmap
	 * @return the bitmap. this value is allocated via @c std::get_temporary_buffer
	 */
	inline ::BITMAPINFO* prepareCaretBitmap(const DC& dc, ushort width, ushort height) {
		::BITMAPINFO* info = get_temporary_buffer<::BITMAPINFO>(sizeof(::BITMAPINFOHEADER) + sizeof(::RGBQUAD) * width * height).first;
		::BITMAPINFOHEADER& header = info->bmiHeader;
		memset(&header, 0, sizeof(::BITMAPINFOHEADER));
		header.biSize = sizeof(::BITMAPINFOHEADER);
		header.biWidth = width;
		header.biHeight = -height;
		header.biBitCount = sizeof(::RGBQUAD) * 8;//::GetDeviceCaps(hDC, BITSPIXEL);
		header.biPlanes = dc.getDeviceCaps(PLANES);
		return info;
	}
	/**
	 * Creates the bitmap for solid caret.
	 * @param[in,out] bitmap
	 * @param width
	 * @param height
	 * @param color
	 */
	inline void createSolidCaretBitmap(Bitmap& bitmap, ushort width, ushort height, const ::RGBQUAD& color) {
		ScreenDC dc;
		::BITMAPINFO* info = prepareCaretBitmap(dc, width, height);
		uninitialized_fill(info->bmiColors, info->bmiColors + width * height, color);
		bitmap.createDIBitmap(dc, info->bmiHeader, CBM_INIT, info->bmiColors, *info, DIB_RGB_COLORS);
		return_temporary_buffer<::BITMAPINFO>(info);
	}
	/**
	 * Creates the bitmap for RTL caret.
	 * @param[in,out] bitmap
	 * @param height
	 * @param bold
	 * @param color
	 */
	inline void createRTLCaretBitmap(Bitmap& bitmap, ushort height, bool bold, const ::RGBQUAD& color) {
		ScreenDC dc;
		const ::RGBQUAD white = {0x00, 0x00, 0x00, 0x00};
		::BITMAPINFO* info = prepareCaretBitmap(dc, 5, height);
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
		return_temporary_buffer<::BITMAPINFO>(info);
	}
	/**
	 * Creates the bitmap for Thai or Lao caret.
	 * @param[in,out] bitmap
	 * @param height
	 * @param bold
	 * @param color
	 */
	inline void createTISCaretBitmap(Bitmap& bitmap, ushort height, bool bold, const ::RGBQUAD& color) {
		ScreenDC dc;
		const ::RGBQUAD white = {0x00, 0x00, 0x00, 0x00};
		const ushort width = max(height / 8, 3);
		::BITMAPINFO* info = prepareCaretBitmap(dc, width, height);
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
		return_temporary_buffer<::BITMAPINFO>(info);
	}
} // namespace @0

/// @see ICaretListener#caretMoved
void LocaleSensitiveCaretShaper::caretMoved(const Caret& self, const Region&) {
	if(self.isOvertypeMode())
		updater_->update();
}

/// @see ICaretShapeProvider#getCaretShape
void LocaleSensitiveCaretShaper::getCaretShape(auto_ptr<Bitmap>& bitmap, ::SIZE& solidSize, Orientation& orientation) throw() {
	const Caret& caret = updater_->getTextViewer().getCaret();
	const bool overtype = caret.isOvertypeMode() && caret.isSelectionEmpty();

	if(!overtype)
		solidSize.cx = bold_ ? 2 : 1;	// 本当はシステムの設定値を使わなければならない
	else {	// 上書きモードのときはキャレットを次のグリフと同じ幅にする
		if(caret.isEndOfLine())	// 行末
			solidSize.cx = caret.getTextViewer().getTextRenderer().getAverageCharacterWidth();
		else {
			const LineLayout& layout = caret.getTextViewer().getTextRenderer().getLineLayout(caret.getLineNumber());
			const int leading = layout.getLocation(caret.getColumnNumber(), LineLayout::LEADING).x;
			const int trailing = layout.getLocation(caret.getColumnNumber(), LineLayout::TRAILING).x;
			solidSize.cx = static_cast<int>(ascension::internal::distance(leading, trailing));
		}
	}
	solidSize.cy = updater_->getTextViewer().getTextRenderer().getLineHeight();
	orientation = LEFT_TO_RIGHT;

	HIMC imc = ::ImmGetContext(updater_->getTextViewer().get());
	const bool imeOpened = toBoolean(::ImmGetOpenStatus(imc));
	::ImmReleaseContext(updater_->getTextViewer().get(), imc);
	if(imeOpened) {	// CJK and IME is open
		static const ::RGBQUAD red = {0xFF, 0xFF, 0x80, 0x00};
		bitmap.reset(new Bitmap);
		createSolidCaretBitmap(*bitmap.get(), static_cast<ushort>(solidSize.cx), static_cast<ushort>(solidSize.cy), red);
	} else if(!overtype && solidSize.cy > 3) {
		static const ::RGBQUAD black = {0xFF, 0xFF, 0xFF, 0x00};
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

/// @see ICaretShapeProvider#matchBracketsChanged
void LocaleSensitiveCaretShaper::matchBracketsChanged(const Caret&, const std::pair<Position, Position>&, bool) {
}

/// @see ICaretShapeProvider#selectionShapeChanged
void LocaleSensitiveCaretShaper::selectionShapeChanged(const Caret&) {
}

/// @see ITextViewerInputStatusListener#textViewerIMEOpenStatusChanged
void LocaleSensitiveCaretShaper::textViewerIMEOpenStatusChanged() throw() {
	updater_->update();
}

/// @see ITextViewerInputStatusListener#textViewerInputLanguageChanged
void LocaleSensitiveCaretShaper::textViewerInputLanguageChanged() throw() {
	updater_->update();
}
