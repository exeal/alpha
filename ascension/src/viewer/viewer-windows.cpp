/**
 * @file viewer-windows.cpp
 * @author exeal
 * @date 2003-2006 was EditView.cpp and EditViewWindowMessages.cpp
 * @date 2006-2011 was viewer.cpp
 * @date 2011-05-16 separated from viewer.cpp
 */

#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/viewer.hpp>
#include <ascension/text-editor/command.hpp>
#include <msctf.h>
#include <ascension/win32/ui/wait-cursor.hpp>
#ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY
#	include <ascension/win32/com/dispatch-impl.hpp>
#endif // !ASCENSION_NO_ACTIVE_ACCESSIBILITY
#ifndef ASCENSION_NO_TEXT_SERVICES_FRAMEWORK
#	include <Textstor.h>
#endif // !ASCENSION_NO_TEXT_SERVICES_FRAMEWORK
#pragma comment(lib, "version.lib")

using namespace ascension;
using namespace ascension::viewers;
using namespace std;
namespace k = ascension::kernel;

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

// defined at ascension/win32/windows.hpp
LANGID ASCENSION_FASTCALL ascension::win32::userDefaultUILanguage() /*throw()*/ {
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


#ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY

// TextViewer.AccessibleProxy /////////////////////////////////////////////////////////////////////

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

/**
 * @c TextViewer#AccessibleProxy is proxy object for @c IAccessible interface of @c TextViewer instance.
 * @see TextViewer#getAccessibleObject, ASCENSION_NO_ACTIVE_ACCESSIBILITY
 */
class TextViewer::AccessibleProxy :
		public k::DocumentListener,
		public win32::com::ole::IDispatchImpl<
			win32::com::IUnknownImpl<
				typelist::Cat<ASCENSION_WIN32_COM_INTERFACE_SIGNATURE(IAccessible),
				typelist::Cat<ASCENSION_WIN32_COM_INTERFACE_SIGNATURE(IDispatch),
				typelist::Cat<ASCENSION_WIN32_COM_INTERFACE_SIGNATURE(IOleWindow)> > >,
				win32::com::NoReferenceCounting
			>,
			win32::com::ole::TypeInformationFromRegistry<&LIBID_Accessibility, &IID_IAccessible>
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
	ASCENSION_UNASSIGNABLE_TAG(AccessibleProxy);
public:
	// constructor
	AccessibleProxy(TextViewer& view);
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
	void documentAboutToBeChanged(const k::Document& document);
	void documentChanged(const k::Document& document, const k::DocumentChange& change);
private:
	TextViewer& view_;
	bool available_;
	win32::com::ComPtr<IAccessible> defaultServer_;
//	enum {CHILDID_SELECTION = 1};
};

#define ASCENSION_VERIFY_AVAILABILITY()	\
	if(!available_) return RPC_E_DISCONNECTED

/**
 * Constructor.
 * @param view the viewer
 */
TextViewer::AccessibleProxy::AccessibleProxy(TextViewer& view) /*throw()*/ : view_(view), available_(true) {
	assert(accLib.isAvailable());
	accLib.createStdAccessibleObject(view.identifier().get(), OBJID_CLIENT, IID_IAccessible, defaultServer_.initializePPV());
}

/// @see IAccessible#accDoDefaultAction
STDMETHODIMP TextViewer::AccessibleProxy::accDoDefaultAction(VARIANT) {
	ASCENSION_VERIFY_AVAILABILITY();
	return DISP_E_MEMBERNOTFOUND;
}

/// @see IAccessible#accHitTest
STDMETHODIMP TextViewer::AccessibleProxy::accHitTest(long xLeft, long yTop, VARIANT* pvarChild) {
	ASCENSION_VERIFY_AVAILABILITY();
	// ウィンドウが矩形であることを前提としている
	ASCENSION_WIN32_VERIFY_COM_POINTER(pvarChild);
	POINT p = {xLeft, yTop};
	::ScreenToClient(view_.identifier().get(), &p);
	RECT clientBounds;
	::GetClientRect(view_.identifier().get(), &clientBounds);
	if(win32::boole(::PtInRect(&clientBounds, p))) {
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
	ASCENSION_VERIFY_AVAILABILITY();
	ASCENSION_WIN32_VERIFY_COM_POINTER(pxLeft);
	ASCENSION_WIN32_VERIFY_COM_POINTER(pyTop);
	ASCENSION_WIN32_VERIFY_COM_POINTER(pcxWidth);
	ASCENSION_WIN32_VERIFY_COM_POINTER(pcyHeight);
	if(varChild.vt != VT_I4 || varChild.lVal != CHILDID_SELF)
		return E_INVALIDARG;
	RECT clientBounds;
	::GetClientRect(view_.identifier().get(), &clientBounds);
	POINT origin = {clientBounds.left, clientBounds.top};
	::ClientToScreen(view_.identifier().get(), &origin);
	*pxLeft = origin.x;
	*pyTop = origin.y;
	*pcxWidth = clientBounds.right - clientBounds.left;
	*pcyHeight = clientBounds.bottom - clientBounds.top;
	return S_OK;
}

/// @see IAccessible#accNavigate
STDMETHODIMP TextViewer::AccessibleProxy::accNavigate(long navDir, VARIANT varStart, VARIANT* pvarEndUpAt) {
	ASCENSION_VERIFY_AVAILABILITY();
	return defaultServer_->accNavigate(navDir, varStart, pvarEndUpAt);
}

/// @see IAccessible#accSelect
STDMETHODIMP TextViewer::AccessibleProxy::accSelect(long flagsSelect, VARIANT varChild) {
	ASCENSION_VERIFY_AVAILABILITY();
	return (varChild.vt == VT_I4 && varChild.lVal == CHILDID_SELF) ?
		defaultServer_->accSelect(flagsSelect, varChild) : E_INVALIDARG;
}

/// @see IOleWindow#ContextSensitiveHelp
STDMETHODIMP TextViewer::AccessibleProxy::ContextSensitiveHelp(BOOL fEnterMode) {
	return S_OK;	// not supported
}

/// Informs that the viewer is inavailable to the proxy.
void TextViewer::AccessibleProxy::dispose() {
	if(!available_)
		throw IllegalStateException("This proxy is already disposed.");
	available_ = false;
}

/// @see Document#IListener#documentAboutToBeChanged
void TextViewer::AccessibleProxy::documentAboutToBeChanged(const k::Document&) {
	// do nothing
}

/// @see Document#IListener#documentChanged
void TextViewer::AccessibleProxy::documentChanged(const k::Document&, const k::DocumentChange&) {
	assert(accLib.isAvailable());
	accLib.notifyWinEvent(EVENT_OBJECT_VALUECHANGE, view_.identifier().get(), OBJID_CLIENT, CHILDID_SELF);
}

/// @see IAccessible#get_accChild
STDMETHODIMP TextViewer::AccessibleProxy::get_accChild(VARIANT, IDispatch** ppdispChild) {
	ASCENSION_VERIFY_AVAILABILITY();
	ASCENSION_WIN32_VERIFY_COM_POINTER(ppdispChild);
	*ppdispChild = 0;
	return S_OK;
}

/// @see IAccessible#get_accChildCount
STDMETHODIMP TextViewer::AccessibleProxy::get_accChildCount(long* pcountChildren) {
	ASCENSION_VERIFY_AVAILABILITY();
	ASCENSION_WIN32_VERIFY_COM_POINTER(pcountChildren);
	*pcountChildren = 0;
	return S_OK;
}

/// @see IAccessible#get_accDefaultAction
STDMETHODIMP TextViewer::AccessibleProxy::get_accDefaultAction(VARIANT, BSTR*) {
	ASCENSION_VERIFY_AVAILABILITY();
	return DISP_E_MEMBERNOTFOUND;
}

/// @see IAccessible#get_accDescription
STDMETHODIMP TextViewer::AccessibleProxy::get_accDescription(VARIANT, BSTR*) {
	ASCENSION_VERIFY_AVAILABILITY();
	return DISP_E_MEMBERNOTFOUND;
}

/// @see IAccessible#get_accFocus
STDMETHODIMP TextViewer::AccessibleProxy::get_accFocus(VARIANT* pvarChild) {
	ASCENSION_VERIFY_AVAILABILITY();
	ASCENSION_WIN32_VERIFY_COM_POINTER(pvarChild);
	pvarChild->vt = VT_I4;
	pvarChild->lVal = CHILDID_SELF;
	return S_OK;
}

/// @see IAccessible#get_accHelp
STDMETHODIMP TextViewer::AccessibleProxy::get_accHelp(VARIANT, BSTR*) {
	ASCENSION_VERIFY_AVAILABILITY();
	return DISP_E_MEMBERNOTFOUND;
}

/// @see IAccessible#get_accHelpTopic
STDMETHODIMP TextViewer::AccessibleProxy::get_accHelpTopic(BSTR*, VARIANT, long*) {
	ASCENSION_VERIFY_AVAILABILITY();
	return DISP_E_MEMBERNOTFOUND;
}

/// @see IAccessible#get_accKeyboardShortcut
STDMETHODIMP TextViewer::AccessibleProxy::get_accKeyboardShortcut(VARIANT varChild, BSTR* pszKeyboardShortcut) {
	ASCENSION_VERIFY_AVAILABILITY();
	ASCENSION_WIN32_VERIFY_COM_POINTER(pszKeyboardShortcut);
	*pszKeyboardShortcut = 0;
	if(varChild.vt != VT_I4 || varChild.lVal != CHILDID_SELF)
		return E_INVALIDARG;
	return S_FALSE;
}

/// @see IAccessible#get_accName
STDMETHODIMP TextViewer::AccessibleProxy::get_accName(VARIANT varChild, BSTR* pszName) {
	ASCENSION_VERIFY_AVAILABILITY();
	ASCENSION_WIN32_VERIFY_COM_POINTER(pszName);
	*pszName = 0;
	if(varChild.vt != VT_I4 || varChild.lVal != CHILDID_SELF)
		return E_INVALIDARG;
	return S_FALSE;
}

/// @see IAccessible#get_accParent
STDMETHODIMP TextViewer::AccessibleProxy::get_accParent(IDispatch** ppdispParent) {
	ASCENSION_VERIFY_AVAILABILITY();
	if(accLib.isAvailable())
		return accLib.accessibleObjectFromWindow(view_.identifier().get(),
			OBJID_WINDOW, IID_IAccessible, reinterpret_cast<void**>(ppdispParent));
	return defaultServer_->get_accParent(ppdispParent);
}

/// @see IAccessible#get_accRole
STDMETHODIMP TextViewer::AccessibleProxy::get_accRole(VARIANT varChild, VARIANT* pvarRole) {
	ASCENSION_VERIFY_AVAILABILITY();
	ASCENSION_WIN32_VERIFY_COM_POINTER(pvarRole);
	if(varChild.vt != VT_I4 || varChild.lVal != CHILDID_SELF)
		return E_INVALIDARG;
	pvarRole->vt = VT_I4;
	pvarRole->lVal = ROLE_SYSTEM_TEXT;
	return S_OK;
}

/// @see IAccessible#get_accSelection
STDMETHODIMP TextViewer::AccessibleProxy::get_accSelection(VARIANT* pvarChildren) {
	ASCENSION_VERIFY_AVAILABILITY();
	ASCENSION_WIN32_VERIFY_COM_POINTER(pvarChildren);
	pvarChildren->vt = VT_EMPTY;
	return S_FALSE;
}

/// @see IAccessible#get_accState
STDMETHODIMP TextViewer::AccessibleProxy::get_accState(VARIANT varChild, VARIANT* pvarState) {
	ASCENSION_VERIFY_AVAILABILITY();
	if(varChild.vt != VT_I4 || varChild.lVal != CHILDID_SELF)
		return E_INVALIDARG;
	pvarState->vt = VT_I4;
	pvarState->lVal = 0;	// STATE_SYSTEM_NORMAL;
	if(!view_.isVisible())
		pvarState->lVal |= STATE_SYSTEM_INVISIBLE;
	if(::GetTopWindow(view_.identifier().get()) == ::GetActiveWindow())
		pvarState->lVal |= STATE_SYSTEM_FOCUSABLE;
	if(view_.hasFocus())
		pvarState->lVal |= STATE_SYSTEM_FOCUSED;
	if(view_.document().isReadOnly())
		pvarState->lVal |= STATE_SYSTEM_READONLY;
	return S_OK;
}

/// @see IAccessible#get_accValue
STDMETHODIMP TextViewer::AccessibleProxy::get_accValue(VARIANT varChild, BSTR* pszValue) {
	ASCENSION_VERIFY_AVAILABILITY();
	ASCENSION_WIN32_VERIFY_COM_POINTER(pszValue);
	if(varChild.vt != VT_I4 || varChild.lVal != CHILDID_SELF)
		return E_INVALIDARG;
	basic_ostringstream<Char> s;
	writeDocumentToStream(s, view_.document(), view_.document().region());
	*pszValue = ::SysAllocString(s.str().c_str());
	return (*pszValue != 0) ? S_OK : E_OUTOFMEMORY;
}

/// @see IOleWindow#GetWindow
STDMETHODIMP TextViewer::AccessibleProxy::GetWindow(HWND* phwnd) {
	ASCENSION_VERIFY_AVAILABILITY();
	ASCENSION_WIN32_VERIFY_COM_POINTER(phwnd);
	*phwnd = view_.identifier().get();
	return S_OK;
}

/// @see IAccessible#put_accName
STDMETHODIMP TextViewer::AccessibleProxy::put_accName(VARIANT, BSTR) {
	ASCENSION_VERIFY_AVAILABILITY();
	return DISP_E_MEMBERNOTFOUND;
}

/// @see IAccessible#put_accValue
STDMETHODIMP TextViewer::AccessibleProxy::put_accValue(VARIANT varChild, BSTR szValue) {
	ASCENSION_VERIFY_AVAILABILITY();
	if(varChild.vt != VT_I4 || varChild.lVal != CHILDID_SELF)
		return E_INVALIDARG;
	else if(view_.document().isReadOnly())
		return E_ACCESSDENIED;
	view_.caret().replaceSelection((szValue != 0) ? szValue : L"");
	return S_OK;
}

#undef ASCENSION_VERIFY_AVAILABILITY
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


// TextViewer /////////////////////////////////////////////////////////////////////////////////////

#ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY
/// Returns the accessible proxy of the viewer.
HRESULT TextViewer::accessibleObject(IAccessible*& acc) const /*throw()*/ {
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
#endif // !ASCENSION_NO_ACTIVE_ACCESSIBILITY

/// @see Widget#handleWindowSystemEvent
LRESULT TextViewer::handleWindowSystemEvent(UINT message, WPARAM wp, LPARAM lp, bool& consumed) {
	using namespace ascension::texteditor::commands;

	switch(message) {
#ifdef ASCENSION_HANDLE_STANDARD_EDIT_CONTROL_MESSAGES
	case WM_CLEAR:
		if(::GetKeyState(VK_SHIFT) < 0)
			cutSelection(caret(), true);
		else
			CharacterDeletionCommand(*this, Direction::FORWARD)();
		consumed = true;
		return 0L;
	case WM_COPY:
		copySelection(caret(), true);
		consumed = true;
		return 0L;
	case WM_CUT:
		cutSelection(caret(), true);
		consumed = true;
		return 0L;
#endif // ASCENSION_HANDLE_STANDARD_EDIT_CONTROL_MESSAGES
#ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY
	case WM_GETOBJECT:
		if(lp == OBJID_CLIENT) {
			win32::com::ComPtr<IAccessible> acc;
			if(SUCCEEDED(accessibleObject(*acc.initialize())) && accLib.isAvailable())
				return accLib.lresultFromObject(IID_IAccessible, wp, acc.get());
		} else if(lp == OBJID_WINDOW) {
		}
		return 0;
#endif // !ASCENSION_NO_ACTIVE_ACCESSIBILITY
	case WM_GETTEXT: {
		basic_ostringstream<Char> s;
		writeDocumentToStream(s, document(), document().region(), k::NLF_CR_LF);
		consumed = true;
		return reinterpret_cast<LRESULT>(s.str().c_str());
	}
	case WM_GETTEXTLENGTH:
		// ウィンドウ関係だし改行は CRLF でいいか。NLR_RAW_VALUE だと遅いし
		consumed = true;
		return document().length(k::NLF_CR_LF);
	case WM_INPUTLANGCHANGE:
		inputStatusListeners_.notify(&TextViewerInputStatusListener::textViewerInputLanguageChanged);
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
		consumed = true;
		return 0L;
#endif // ASCENSION_HANDLE_STANDARD_EDIT_CONTROL_MESSAGES
	case WM_SETTEXT:
		EntireDocumentSelectionCreationCommand(*this)();
		caret().replaceSelection(String(reinterpret_cast<const wchar_t*>(lp)), false);
		consumed = true;
		return 0L;
#ifdef ASCENSION_HANDLE_STANDARD_EDIT_CONTROL_MESSAGES
	case WM_UNDO:
		UndoCommand(*this, false)();
		consumed = true;
		return 0L;
#endif // ASCENSION_HANDLE_STANDARD_EDIT_CONTROL_MESSAGES
		// dispatch message into handler
	case WM_CAPTURECHANGED:
		onCaptureChanged(win32::Handle<HWND>(reinterpret_cast<HWND>(lp)), consumed);
		return consumed ? 0 : 1;
	case WM_CHAR:
		onChar(static_cast<UINT>(wp), static_cast<UINT>(lp), consumed);
		return consumed ? 0 : 1;
	case WM_COMMAND:
		onCommand(LOWORD(wp), HIWORD(wp), win32::Handle<HWND>(reinterpret_cast<HWND>(lp)), consumed);
		return consumed ? 0 : 1;
	case WM_DESTROY:
		onDestroy(consumed);
		return consumed ? 0 : 1;
	case WM_ERASEBKGND:
		onEraseBkgnd(win32::Handle<HDC>(reinterpret_cast<HDC>(wp)), consumed);
		return consumed ? TRUE : FALSE;
	case WM_GETFONT:
		return (consumed = true), reinterpret_cast<LRESULT>(onGetFont().get());
	case WM_HSCROLL:
		return (consumed = true), onHScroll(LOWORD(wp), HIWORD(wp), win32::Handle<HWND>(reinterpret_cast<HWND>(lp))), 0;
	case WM_IME_COMPOSITION:
		return onIMEComposition(wp, lp, consumed), 1;
	case WM_IME_ENDCOMPOSITION:
		return onIMEEndComposition(), 1;
	case WM_IME_NOTIFY:
		return onIMENotify(wp, lp, consumed);
	case WM_IME_REQUEST:
		return onIMERequest(wp, lp, consumed);
	case WM_IME_STARTCOMPOSITION:
		return onIMEStartComposition(), 1;
	case WM_NCCREATE:
		return (consumed = true), onNcCreate(*reinterpret_cast<CREATESTRUCTW*>(lp));
	case WM_NOTIFY:
		return onNotify(static_cast<int>(wp), *reinterpret_cast<NMHDR*>(lp), consumed), 0;
	case WM_SETCURSOR:
		onSetCursor(win32::Handle<HWND>(reinterpret_cast<HWND>(wp)), LOWORD(lp), HIWORD(lp), consumed);
		return consumed ? TRUE : FALSE;
	case WM_STYLECHANGED:
		return (consumed = true), onStyleChanged(static_cast<int>(wp), *reinterpret_cast<STYLESTRUCT*>(lp)), 0;
	case WM_STYLECHANGING:
		return (consumed = true), onStyleChanging(static_cast<int>(wp), *reinterpret_cast<STYLESTRUCT*>(lp)), 0;
	case WM_SYSCHAR:
		onSysChar(static_cast<UINT>(wp), static_cast<UINT>(lp), consumed);
		return consumed ? 0 : 1;
	case WM_SYSCOLORCHANGE:
		return (consumed = true), onSysColorChange(), 0;
#ifdef WM_THEMECHANGED
	case WM_THEMECHANGED:
		return (consumed = true), onThemeChanged(), 0;
#endif // WM_THEMECHANGED
	case WM_TIMER:
		return (consumed = true), onTimer(static_cast<UINT_PTR>(wp), reinterpret_cast<TIMERPROC>(lp)), 0;
#ifdef WM_UNICHAR
	case WM_UNICHAR:
		onUniChar(static_cast<UINT>(wp), static_cast<UINT>(lp), consumed);
		return consumed ? 0 : 1;
#endif // WM_UNICHAR
	case WM_VSCROLL:
		return (consumed = true), onVScroll(LOWORD(wp), HIWORD(wp), win32::Handle<HWND>(reinterpret_cast<HWND>(lp))), 0;
	}

	return Widget::handleWindowSystemEvent(message, wp, lp, consumed);
}

/// @see WM_DESTROY
void TextViewer::onDestroy(bool& consumed) {
	if(mouseInputStrategy_.get() != 0) {
		mouseInputStrategy_->interruptMouseReaction(false);
		mouseInputStrategy_->uninstall();
		mouseInputStrategy_.reset();
	}

	// destroy children
	::DestroyWindow(toolTip_);

#ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY
	if(accessibleProxy_ != 0)
		accessibleProxy_->dispose();
//	if(accLib.isAvailable())
//		accLib.notifyWinEvent(EVENT_OBJECT_DESTROY, *this, OBJID_CLIENT, CHILDID_SELF);
#endif // !ASCENSION_NO_ACTIVE_ACCESSIBILITY
	consumed = true;
}

/// @see WM_ERASEGKGND
void TextViewer::onEraseBkgnd(const win32::Handle<HDC>&, bool& consumed) {
	consumed = false;
}

/// @see WM_GETFONT
const win32::Handle<HFONT>& TextViewer::onGetFont() {
	return renderer_->defaultFont()->nativeHandle().get();
}

/// @see WM_HSCROLL
void TextViewer::onHScroll(UINT sbCode, UINT, const win32::Handle<HWND>&) {
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
			const Range<int> range(scrollRange(SB_HORZ));
			scrollTo((sbCode == SB_LEFT) ? range.beginning() : range.end(), -1, true);
			break;
		}
		case SB_THUMBTRACK:	// by drag or wheel
			scrollTo(scrollTrackPosition(SB_HORZ), -1, false);	// use 32-bit value
			break;
	}
//	consumed = false;
}

/// @see WM_NCCREATE
bool TextViewer::onNcCreate(CREATESTRUCTW&) {
	const LONG s = ::GetWindowLongW(identifier().get(), GWL_EXSTYLE);
	::SetWindowLongW(identifier().get(), GWL_EXSTYLE, s & ~WS_EX_LAYOUTRTL);
	return true;
}

/// @see WM_NOTIFY
void TextViewer::onNotify(int, NMHDR& nmhdr, bool& consumed) {
	// tooltip text
	if(nmhdr.hwndFrom == toolTip_ && nmhdr.code == TTN_GETDISPINFOW) {
		::SendMessageW(toolTip_, TTM_SETMAXTIPWIDTH, 0, 1000);	// make line breaks effective
		reinterpret_cast<LPNMTTDISPINFOW>(&nmhdr)->lpszText = tipText_;
		consumed = true;
	} else
		consumed = false;
}

/// @see WM_STYLECHANGED
void TextViewer::onStyleChanged(int type, const STYLESTRUCT& style) {
	if(type == GWL_EXSTYLE
			&& (((style.styleOld ^ style.styleNew) & (WS_EX_RIGHT | WS_EX_RTLREADING)) != 0)) {
		// synchronize the reading direction with the window's style
		// (ignore the alignment)
		Configuration c(configuration());
		c.readingDirection = ((style.styleNew & WS_EX_RTLREADING) != 0) ? RIGHT_TO_LEFT : LEFT_TO_RIGHT;
		setConfiguration(&c, 0, false);
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
#endif // WM_THEMECHANGED

/// @see WM_TIMER
void TextViewer::onTimer(UINT_PTR eventID, TIMERPROC) {
	if(eventID == TIMERID_CALLTIP) {	// show the tooltip
		::KillTimer(identifier().get(), TIMERID_CALLTIP);
		::SendMessageW(toolTip_, TTM_UPDATE, 0, 0L);
	}
}

/// @see Window#onVScroll
void TextViewer::onVScroll(UINT sbCode, UINT, const win32::Handle<HWND>&) {
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
			const Range<int> range(scrollRange(SB_VERT));
			scrollTo(-1, (sbCode == SB_TOP) ? range.beginning() : range.end(), true); break;
		}
		case SB_THUMBTRACK:	// by drag or wheel
			scrollTo(-1, scrollTrackPosition(SB_VERT), true);	// use 32-bit value
			break;
	}
}

void TextViewer::provideClassInformation(Widget::ClassInformation& classInformation) const {
	classInformation.style = CS_BYTEALIGNCLIENT | CS_BYTEALIGNWINDOW | CS_DBLCLKS;
	classInformation.background = COLOR_WINDOW;
	classInformation.cursor = MAKEINTRESOURCEW(32513);	// IDC_IBEAM
}

basic_string<WCHAR> TextViewer::provideClassName() const {
	return L"ascension.TextViewer";
}