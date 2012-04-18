/**
 * @file viewer-windows.cpp
 * @author exeal
 * @date 2003-2006 was EditView.cpp and EditViewWindowMessages.cpp
 * @date 2006-2011 was viewer.cpp
 * @date 2011-05-16 separated from viewer.cpp
 * @date 2011-2012
 */

#define ASCENSION_TEST_TEXT_STYLES

#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/viewer.hpp>
#include <ascension/viewer/base/cursor.hpp>
#include <ascension/text-editor/command.hpp>
#include <ascension/text-editor/input-sequence-checker.hpp>
#include <ascension/win32/windows.hpp>
#include <ascension/win32/ui/menu.hpp>
#include <ascension/win32/ui/wait-cursor.hpp>
#ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY
#	include <ascension/win32/com/dispatch-impl.hpp>
#endif // !ASCENSION_NO_ACTIVE_ACCESSIBILITY
#include <msctf.h>
#include <zmouse.h>
#ifndef ASCENSION_NO_TEXT_SERVICES_FRAMEWORK
#	include <Textstor.h>
#endif // !ASCENSION_NO_TEXT_SERVICES_FRAMEWORK
#pragma comment(lib, "version.lib")

#ifdef ASCENSION_TEST_TEXT_STYLES
#	include <ascension/presentation/presentation-reconstructor.hpp>
#endif

using namespace ascension;
using namespace ascension::graphics;
using namespace ascension::graphics::font;
using namespace ascension::presentation;
using namespace ascension::viewers;
using namespace std;
namespace k = ascension::kernel;

namespace {
	// すぐ下で使う
	BOOL CALLBACK enumResLangProc(HMODULE, const WCHAR*, const WCHAR* name, WORD langID, LONG_PTR param) {
		if(name == nullptr)
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

	// forward to GetUserDefaultUILanguage (kernel32.dll) if after 2000/XP/Server 2003
	if(version.dwMajorVersion >= 5) {
		if(HMODULE dll = ::LoadLibraryW(L"kernel32.dll")) {
			if(LANGID(WINAPI *function)(void) = reinterpret_cast<LANGID(WINAPI*)(void)>(::GetProcAddress(dll, "GetUserDefaultUILanguage")))
				id = (*function)();
			::FreeLibrary(dll);
		}
	}

	// use language of version information of ntdll.dll if on NT 3.51-4.0
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

	return id;	// (... or use value of HKCU\Control Panel\Desktop\ResourceLocale if on Win 9x
}


#ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY

// TextViewer.AccessibleProxy /////////////////////////////////////////////////////////////////////

namespace {
	class AccLib {
	public:
		AccLib() /*throw()*/ : oleaccDLL_(::LoadLibraryA("oleacc.dll")), user32DLL_(::LoadLibraryA("user32.dll")) {
			if(oleaccDLL_ == nullptr || user32DLL_ == nullptr) {
				::FreeLibrary(oleaccDLL_);
				::FreeLibrary(user32DLL_);
				oleaccDLL_ = user32DLL_ = nullptr;
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
		bool isAvailable() const /*throw()*/ {return oleaccDLL_ != nullptr;}
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
				typelist::Cat<ASCENSION_WIN32_COM_INTERFACE_SIGNATURE(IOleWindow)>>>,
				win32::com::NoReferenceCounting
			>,
			win32::com::ole::TypeInformationFromRegistry<&LIBID_Accessibility, &IID_IAccessible>
		> {
	// references about implementation of IAccessible:
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
	explicit AccessibleProxy(TextViewer& viewer);
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
	// DocumentListener
	void documentAboutToBeChanged(const k::Document& document);
	void documentChanged(const k::Document& document, const k::DocumentChange& change);
private:
	TextViewer& viewer_;
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
TextViewer::AccessibleProxy::AccessibleProxy(TextViewer& viewer) /*throw()*/ : viewer_(viewer), available_(true) {
	assert(accLib.isAvailable());
	accLib.createStdAccessibleObject(viewer.identifier().get(), OBJID_CLIENT, IID_IAccessible, defaultServer_.initializePPV());
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
	if(geometry::includes(viewer_.bounds(false), viewer_.mapFromGlobal(geometry::make<NativePoint>(xLeft, yTop)))) {
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
	const NativeRectangle clientBounds(viewer_.bounds(false));
	const NativePoint origin(viewer_.mapToGlobal(geometry::topLeft(clientBounds)));
	*pxLeft = geometry::x(origin);
	*pyTop = geometry::y(origin);
	*pcxWidth = geometry::dx(clientBounds);
	*pcyHeight = geometry::dy(clientBounds);
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
	accLib.notifyWinEvent(EVENT_OBJECT_VALUECHANGE, viewer_.identifier().get(), OBJID_CLIENT, CHILDID_SELF);
}

/// @see IAccessible#get_accChild
STDMETHODIMP TextViewer::AccessibleProxy::get_accChild(VARIANT, IDispatch** ppdispChild) {
	ASCENSION_VERIFY_AVAILABILITY();
	ASCENSION_WIN32_VERIFY_COM_POINTER(ppdispChild);
	*ppdispChild = nullptr;
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
	*pszKeyboardShortcut = nullptr;
	if(varChild.vt != VT_I4 || varChild.lVal != CHILDID_SELF)
		return E_INVALIDARG;
	return S_FALSE;
}

/// @see IAccessible#get_accName
STDMETHODIMP TextViewer::AccessibleProxy::get_accName(VARIANT varChild, BSTR* pszName) {
	ASCENSION_VERIFY_AVAILABILITY();
	ASCENSION_WIN32_VERIFY_COM_POINTER(pszName);
	*pszName = nullptr;
	if(varChild.vt != VT_I4 || varChild.lVal != CHILDID_SELF)
		return E_INVALIDARG;
	return S_FALSE;
}

/// @see IAccessible#get_accParent
STDMETHODIMP TextViewer::AccessibleProxy::get_accParent(IDispatch** ppdispParent) {
	ASCENSION_VERIFY_AVAILABILITY();
	if(accLib.isAvailable())
		return accLib.accessibleObjectFromWindow(viewer_.identifier().get(),
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
	if(!viewer_.isVisible())
		pvarState->lVal |= STATE_SYSTEM_INVISIBLE;
	if(::GetTopWindow(viewer_.identifier().get()) == ::GetActiveWindow())
		pvarState->lVal |= STATE_SYSTEM_FOCUSABLE;
	if(viewer_.hasFocus())
		pvarState->lVal |= STATE_SYSTEM_FOCUSED;
	if(viewer_.document().isReadOnly())
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
	writeDocumentToStream(s, viewer_.document(), viewer_.document().region());
	*pszValue = ::SysAllocString(s.str().c_str());
	return (*pszValue != nullptr) ? S_OK : E_OUTOFMEMORY;
}

/// @see IOleWindow#GetWindow
STDMETHODIMP TextViewer::AccessibleProxy::GetWindow(HWND* phwnd) {
	ASCENSION_VERIFY_AVAILABILITY();
	ASCENSION_WIN32_VERIFY_COM_POINTER(phwnd);
	*phwnd = viewer_.identifier().get();
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
	else if(viewer_.document().isReadOnly())
		return E_ACCESSDENIED;
	viewer_.caret().replaceSelection((szValue != nullptr) ? szValue : L"");
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
	acc = nullptr;
	if(accessibleProxy_ == nullptr && win32::boole(::IsWindow(identifier().get())) && accLib.isAvailable()) {
		if(self.accessibleProxy_ = new AccessibleProxy(self)) {
			self.accessibleProxy_->AddRef();
//			accLib.notifyWinEvent(EVENT_OBJECT_CREATE, *this, OBJID_CLIENT, CHILDID_SELF);
		} else
			return E_OUTOFMEMORY;
	}
	if(accessibleProxy_ == nullptr)
		return E_FAIL;
	(acc = self.accessibleProxy_)->AddRef();
	return S_OK;
}
#endif // !ASCENSION_NO_ACTIVE_ACCESSIBILITY

/// Implementation of @c #beep method. The subclasses can override to customize the behavior.
void TextViewer::doBeep() /*throw()*/ {
	::MessageBeep(MB_OK);
}

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
			writeDocumentToStream(s, document(), document().region(), text::NLF_CR_LF);
			consumed = true;
			return reinterpret_cast<LRESULT>(s.str().c_str());
		}
		case WM_GETTEXTLENGTH:
			// ウィンドウ関係だし改行は CRLF でいいか。NLR_RAW_VALUE だと遅いし
			consumed = true;
			return document().length(text::NLF_CR_LF);
//		case WM_NCPAINT:
//			return 0;
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
		case WM_SYSCHAR:
#ifdef WM_UNICHAR:
		case WM_UNICHAR:
#endif // WM_UNICHAR
		{
			static_cast<detail::InputEventHandler&>(caret()).handleInputEvent(message, wp, lp, consumed);	// $friendly-access
			// vanish the cursor when the GUI user began typing
			if(consumed) {
				// ignore if the cursor is not over a window belongs to the same thread
				HWND pointedWindow = ::WindowFromPoint(base::Cursor::position());
				if(pointedWindow != nullptr
						&& ::GetWindowThreadProcessId(pointedWindow, nullptr) == ::GetWindowThreadProcessId(identifier().get(), nullptr))
					cursorVanisher_.vanish();
			}
			return consumed ? 0 : 1;
		}
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
		case WM_IME_CHAR:
		case WM_IME_COMPOSITION:
		case WM_IME_COMPOSITIONFULL:
		case WM_IME_CONTROL:
		case WM_IME_ENDCOMPOSITION:
		case WM_IME_KEYDOWN:
		case WM_IME_KEYUP:
		case WM_IME_NOTIFY:
		case WM_IME_REQUEST:
		case WM_IME_SELECT:
		case WM_IME_SETCONTEXT:
		case WM_IME_STARTCOMPOSITION:
		case WM_INPUTLANGCHANGE:
			return static_cast<detail::InputEventHandler&>(caret()).handleInputEvent(message, wp, lp, consumed);	// $friendly-access
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
		case WM_SYSCOLORCHANGE:
			return (consumed = true), onSysColorChange(), 0;
#ifdef WM_THEMECHANGED
		case WM_THEMECHANGED:
			return (consumed = true), onThemeChanged(), 0;
#endif // WM_THEMECHANGED
		case WM_TIMER:
			return (consumed = true), onTimer(static_cast<UINT_PTR>(wp), reinterpret_cast<TIMERPROC>(lp)), 0;
		case WM_VSCROLL:
			return (consumed = true), onVScroll(LOWORD(wp), HIWORD(wp), win32::Handle<HWND>(reinterpret_cast<HWND>(lp))), 0;
	}

	return Widget::handleWindowSystemEvent(message, wp, lp, consumed);
}

/// Hides the tool tip.
void TextViewer::hideToolTip() {
	assert(::IsWindow(identifier().get()));
	if(tipText_ == nullptr)
		tipText_ = new Char[1];
	wcscpy(tipText_, L"");
	::KillTimer(identifier().get(), TIMERID_CALLTIP);	// 念のため...
	::SendMessageW(toolTip_, TTM_UPDATE, 0, 0L);
}

/// @internal Initializes the window of the viewer.
void TextViewer::initialize() {
	scrollInfo_.updateVertical(*this);
	updateScrollBars();

	// create the tooltip belongs to the window
	toolTip_ = ::CreateWindowExW(
		WS_EX_TOOLWINDOW | WS_EX_TOPMOST, TOOLTIPS_CLASSW, 0, WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, identifier().get(), nullptr,
		reinterpret_cast<HINSTANCE>(static_cast<HANDLE_PTR>(::GetWindowLongPtr(identifier().get(), GWLP_HINSTANCE))), nullptr);
	if(toolTip_ != nullptr) {
		win32::AutoZeroSize<TOOLINFOW> ti;
		RECT margins = {1, 1, 1, 1};
		ti.hwnd = identifier().get();
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

	setMouseInputStrategy(shared_ptr<MouseInputStrategy>());

#ifdef ASCENSION_TEST_TEXT_STYLES
	RulerConfiguration rc;
	rc.lineNumbers.visible = true;
	rc.indicatorMargin.visible = true;
	rc.lineNumbers.foreground = Paint(Color(0x00, 0x80, 0x80));
	rc.lineNumbers.background = Paint(Color(0xff, 0xff, 0xff));
	rc.lineNumbers.border.color = Color(0x00, 0x80, 0x80);
	rc.lineNumbers.border.style = Border::DOTTED;
	rc.lineNumbers.border.width = Length(1);
	setConfiguration(nullptr, &rc, false);

#if 0
	// this is JavaScript partitioning and lexing settings for test
	using namespace contentassist;
	using namespace rules;
	using namespace text;

	static const ContentType JS_MULTILINE_DOC_COMMENT = 140,
		JS_MULTILINE_COMMENT = 142, JS_SINGLELINE_COMMENT = 143, JS_DQ_STRING = 144, JS_SQ_STRING = 145;

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
	p->setRules(rules, ASCENSION_ENDOF(rules));
	for(size_t i = 0; i < ASCENSION_COUNTOF(rules); ++i)
		delete rules[i];
	document().setPartitioner(unique_ptr<DocumentPartitioner>(p));

	PresentationReconstructor* pr = new PresentationReconstructor(presentation());

	// JSDoc syntax highlight test
	static const Char JSDOC_ATTRIBUTES[] = L"@addon @argument @author @base @class @constructor @deprecated @exception @exec @extends"
		L" @fileoverview @final @ignore @link @member @param @private @requires @return @returns @see @throws @type @version";
	{
		unique_ptr<const WordRule> jsdocAttributes(new WordRule(220, JSDOC_ATTRIBUTES, ASCENSION_ENDOF(JSDOC_ATTRIBUTES) - 1, L' ', true));
		unique_ptr<LexicalTokenScanner> scanner(new LexicalTokenScanner(JS_MULTILINE_DOC_COMMENT));
		scanner->addWordRule(jsdocAttributes);
		scanner->addRule(unique_ptr<Rule>(new URIRule(219, URIDetector::defaultIANAURIInstance(), false)));
		map<Token::ID, const TextStyle> jsdocStyles;
		jsdocStyles.insert(make_pair(Token::DEFAULT_TOKEN, TextStyle(Colors(Color(0x00, 0x80, 0x00)))));
		jsdocStyles.insert(make_pair(219, TextStyle(Colors(Color(0x00, 0x80, 0x00)), false, false, false, SOLID_UNDERLINE)));
		jsdocStyles.insert(make_pair(220, TextStyle(Colors(Color(0x00, 0x80, 0x00)), true)));
		unique_ptr<IPartitionPresentationReconstructor> ppr(
			new LexicalPartitionPresentationReconstructor(document(), unique_ptr<ITokenScanner>(scanner.release()), jsdocStyles));
		pr->setPartitionReconstructor(JS_MULTILINE_DOC_COMMENT, ppr);
	}

	// JavaScript syntax highlight test
	static const Char JS_KEYWORDS[] = L"Infinity break case catch continue default delete do else false finally for function"
		L" if in instanceof new null return switch this throw true try typeof undefined var void while with";
	static const Char JS_FUTURE_KEYWORDS[] = L"abstract boolean byte char class double enum extends final float goto"
		L" implements int interface long native package private protected public short static super synchronized throws transient volatile";
	{
		unique_ptr<const WordRule> jsKeywords(new WordRule(221, JS_KEYWORDS, ASCENSION_ENDOF(JS_KEYWORDS) - 1, L' ', true));
		unique_ptr<const WordRule> jsFutureKeywords(new WordRule(222, JS_FUTURE_KEYWORDS, ASCENSION_ENDOF(JS_FUTURE_KEYWORDS) - 1, L' ', true));
		unique_ptr<LexicalTokenScanner> scanner(new LexicalTokenScanner(DEFAULT_CONTENT_TYPE));
		scanner->addWordRule(jsKeywords);
		scanner->addWordRule(jsFutureKeywords);
		scanner->addRule(unique_ptr<const Rule>(new NumberRule(223)));
		map<Token::ID, const TextStyle> jsStyles;
		jsStyles.insert(make_pair(Token::DEFAULT_TOKEN, TextStyle()));
		jsStyles.insert(make_pair(221, TextStyle(Colors(Color(0x00, 0x00, 0xff)))));
		jsStyles.insert(make_pair(222, TextStyle(Colors(Color(0x00, 0x00, 0xff)), false, false, false, DASHED_UNDERLINE)));
		jsStyles.insert(make_pair(223, TextStyle(Colors(Color(0x80, 0x00, 0x00)))));
		pr->setPartitionReconstructor(DEFAULT_CONTENT_TYPE,
			unique_ptr<IPartitionPresentationReconstructor>(new LexicalPartitionPresentationReconstructor(document(),
				unique_ptr<ITokenScanner>(scanner.release()), jsStyles)));
	}

	// other contents
	pr->setPartitionReconstructor(JS_MULTILINE_COMMENT, unique_ptr<IPartitionPresentationReconstructor>(
		new SingleStyledPartitionPresentationReconstructor(TextStyle(Colors(Color(0x00, 0x80, 0x00))))));
	pr->setPartitionReconstructor(JS_SINGLELINE_COMMENT, unique_ptr<IPartitionPresentationReconstructor>(
		new SingleStyledPartitionPresentationReconstructor(TextStyle(Colors(Color(0x00, 0x80, 0x00))))));
	pr->setPartitionReconstructor(JS_DQ_STRING, unique_ptr<IPartitionPresentationReconstructor>(
		new SingleStyledPartitionPresentationReconstructor(TextStyle(Colors(Color(0x00, 0x00, 0x80))))));
	pr->setPartitionReconstructor(JS_SQ_STRING, unique_ptr<IPartitionPresentationReconstructor>(
		new SingleStyledPartitionPresentationReconstructor(TextStyle(Colors(Color(0x00, 0x00, 0x80))))));
	new CurrentLineHighlighter(*caret_, Colors(Color(), Color::fromCOLORREF(::GetSysColor(COLOR_INFOBK))));

	// URL hyperlinks test
	unique_ptr<hyperlink::CompositeHyperlinkDetector> hld(new hyperlink::CompositeHyperlinkDetector);
	hld->setDetector(JS_MULTILINE_DOC_COMMENT, unique_ptr<hyperlink::IHyperlinkDetector>(
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
	unique_ptr<contentassist::ContentAssistant> ca(new contentassist::ContentAssistant());
	ca->setContentAssistProcessor(JS_MULTILINE_DOC_COMMENT, unique_ptr<contentassist::IContentAssistProcessor>(new JSDocProposals(cti->getIdentifierSyntax(JS_MULTILINE_DOC_COMMENT))));
	ca->setContentAssistProcessor(DEFAULT_CONTENT_TYPE, unique_ptr<contentassist::IContentAssistProcessor>(new JSProposals(cti->getIdentifierSyntax(DEFAULT_CONTENT_TYPE))));
	setContentAssistant(unique_ptr<contentassist::IContentAssistant>(ca));
	document().setContentTypeInformation(unique_ptr<IContentTypeInformationProvider>(cti));
#endif // 1

	class ZebraTextRunStyleTest : public TextRunStyleDirector {
	private:
		class Iterator : public StyledTextRunIterator {
		public:
			Iterator(Index lineLength, bool beginningIsBlackBack) : length_(lineLength), beginningIsBlackBack_(beginningIsBlackBack) {
				current_ = StyledTextRun(0, current_.style());
				update();
			}
			StyledTextRun current() const {
				if(!hasNext())
					throw IllegalStateException("");
				return current_;
			}
			bool hasNext() const {
				return current_.position() != length_;
			}
			void next() {
				if(!hasNext())
					throw IllegalStateException("");
				current_ = StyledTextRun(current_.position() + 1, current_.style());
				update();
			}
		private:
			void update() {
				int temp = beginningIsBlackBack_ ? 0 : 1;
				temp += (current_.position() % 2 == 0) ? 0 : 1;
				unique_ptr<TextRunStyle> style(new TextRunStyle);
				style->foreground = Paint((temp % 2 == 0) ?
					Color(0xff, 0x00, 0x00) : SystemColors::get(SystemColors::WINDOW_TEXT));
				style->background = Paint((temp % 2 == 0) ?
					Color(0xff, 0xcc, 0xcc) : SystemColors::get(SystemColors::WINDOW));
				current_ = StyledTextRun(current_.position(), style);
			}
		private:
			const Index length_;
			const bool beginningIsBlackBack_;
			StyledTextRun current_;
		};
	public:
		ZebraTextRunStyleTest(const k::Document& document) : document_(document) {
		}
		unique_ptr<StyledTextRunIterator> queryTextRunStyle(Index line) const {
			return unique_ptr<StyledTextRunIterator>(new Iterator(document_.lineLength(line), line % 2 == 0));
		}
	private:
		const k::Document& document_;
	};
	presentation().setTextRunStyleDirector(
		shared_ptr<TextRunStyleDirector>(new ZebraTextRunStyleTest(document())));
#endif // ASCENSION_TEST_TEXT_STYLES
	
	renderer_->addDefaultFontListener(*this);
	renderer_->layouts().addVisualLinesListener(*this);
}

/// @see WM_CAPTURECHANGED
void TextViewer::onCaptureChanged(const win32::Handle<HWND>&, bool& consumed) {
	if(consumed = (mouseInputStrategy_.get() != nullptr))
		mouseInputStrategy_->captureChanged();
}

/// @see Window#onCommand
void TextViewer::onCommand(WORD id, WORD, const win32::Handle<HWND>&, bool& consumed) {
	using namespace ascension::texteditor::commands;
	switch(id) {
	case WM_UNDO:	// "Undo"
		UndoCommand(*this, false)();
		break;
	case WM_REDO:	// "Redo"
		UndoCommand(*this, true)();
		break;
	case WM_CUT:	// "Cut"
		cutSelection(caret(), true);
		break;
	case WM_COPY:	// "Copy"
		copySelection(caret(), true);
		break;
	case WM_PASTE:	// "Paste"
		PasteCommand(*this, false)();
		break;
	case WM_CLEAR:	// "Delete"
		CharacterDeletionCommand(*this, Direction::FORWARD)();
		break;
	case WM_SELECTALL:	// "Select All"
		EntireDocumentSelectionCreationCommand(*this)();
		break;
	case ID_RTLREADING:	// "Right to left Reading order"
		utils::toggleOrientation(*this);
		break;
	case ID_DISPLAYSHAPINGCONTROLS:	// "Show Unicode control characters"
		textRenderer().displayShapingControls(!textRenderer().displaysShapingControls());
		break;
	case ID_INSERT_LRM:		CharacterInputCommand(*this, 0x200eu)();	break;
	case ID_INSERT_RLM:		CharacterInputCommand(*this, 0x200fu)();	break;
	case ID_INSERT_ZWJ:		CharacterInputCommand(*this, 0x200du)();	break;
	case ID_INSERT_ZWNJ:	CharacterInputCommand(*this, 0x200cu)();	break;
	case ID_INSERT_LRE:		CharacterInputCommand(*this, 0x202au)();	break;
	case ID_INSERT_RLE:		CharacterInputCommand(*this, 0x202bu)();	break;
	case ID_INSERT_LRO:		CharacterInputCommand(*this, 0x202du)();	break;
	case ID_INSERT_RLO:		CharacterInputCommand(*this, 0x202eu)();	break;
	case ID_INSERT_PDF:		CharacterInputCommand(*this, 0x202cu)();	break;
	case ID_INSERT_WJ:		CharacterInputCommand(*this, 0x2060u)();	break;
	case ID_INSERT_NADS:	CharacterInputCommand(*this, 0x206eu)();	break;
	case ID_INSERT_NODS:	CharacterInputCommand(*this, 0x206fu)();	break;
	case ID_INSERT_ASS:		CharacterInputCommand(*this, 0x206bu)();	break;
	case ID_INSERT_ISS:		CharacterInputCommand(*this, 0x206au)();	break;
	case ID_INSERT_AAFS:	CharacterInputCommand(*this, 0x206du)();	break;
	case ID_INSERT_IAFS:	CharacterInputCommand(*this, 0x206cu)();	break;
	case ID_INSERT_RS:		CharacterInputCommand(*this, 0x001eu)();	break;
	case ID_INSERT_US:		CharacterInputCommand(*this, 0x001fu)();	break;
	case ID_INSERT_IAA:		CharacterInputCommand(*this, 0xfff9u)();	break;
	case ID_INSERT_IAT:		CharacterInputCommand(*this, 0xfffau)();	break;
	case ID_INSERT_IAS:		CharacterInputCommand(*this, 0xfffbu)();	break;
	case ID_INSERT_U0020:	CharacterInputCommand(*this, 0x0020u)();	break;
	case ID_INSERT_NBSP:	CharacterInputCommand(*this, 0x00a0u)();	break;
	case ID_INSERT_U1680:	CharacterInputCommand(*this, 0x1680u)();	break;
	case ID_INSERT_MVS:		CharacterInputCommand(*this, 0x180eu)();	break;
	case ID_INSERT_U2000:	CharacterInputCommand(*this, 0x2000u)();	break;
	case ID_INSERT_U2001:	CharacterInputCommand(*this, 0x2001u)();	break;
	case ID_INSERT_U2002:	CharacterInputCommand(*this, 0x2002u)();	break;
	case ID_INSERT_U2003:	CharacterInputCommand(*this, 0x2003u)();	break;
	case ID_INSERT_U2004:	CharacterInputCommand(*this, 0x2004u)();	break;
	case ID_INSERT_U2005:	CharacterInputCommand(*this, 0x2005u)();	break;
	case ID_INSERT_U2006:	CharacterInputCommand(*this, 0x2006u)();	break;
	case ID_INSERT_U2007:	CharacterInputCommand(*this, 0x2007u)();	break;
	case ID_INSERT_U2008:	CharacterInputCommand(*this, 0x2008u)();	break;
	case ID_INSERT_U2009:	CharacterInputCommand(*this, 0x2009u)();	break;
	case ID_INSERT_U200A:	CharacterInputCommand(*this, 0x200au)();	break;
	case ID_INSERT_ZWSP:	CharacterInputCommand(*this, 0x200bu)();	break;
	case ID_INSERT_NNBSP:	CharacterInputCommand(*this, 0x202fu)();	break;
	case ID_INSERT_MMSP:	CharacterInputCommand(*this, 0x205fu)();	break;
	case ID_INSERT_U3000:	CharacterInputCommand(*this, 0x3000u)();	break;
	case ID_INSERT_NEL:		CharacterInputCommand(*this, text::NEXT_LINE)();	break;
	case ID_INSERT_LS:		CharacterInputCommand(*this, text::LINE_SEPARATOR)();	break;
	case ID_INSERT_PS:		CharacterInputCommand(*this, text::PARAGRAPH_SEPARATOR)();	break;
	case ID_TOGGLEIMESTATUS:	// "Open IME" / "Close IME"
		InputMethodOpenStatusToggleCommand(*this)();
		break;
	case ID_TOGGLESOFTKEYBOARD:	// "Open soft keyboard" / "Close soft keyboard"
		InputMethodSoftKeyboardModeToggleCommand(*this)();
		break;
	case ID_RECONVERT:	// "Reconvert"
		ReconversionCommand(*this)();
		break;
	case ID_INVOKE_HYPERLINK:	// "Open <hyperlink>"
		if(const hyperlink::Hyperlink* const link = utils::getPointedHyperlink(*this, caret()))
			link->invoke();
		break;
	default:
//		getParent()->sendMessage(WM_COMMAND, MAKEWPARAM(id, notifyCode), reinterpret_cast<LPARAM>(control));
		consumed = false;
		return;
	}
	consumed = true;
}

/// @see WM_DESTROY
void TextViewer::onDestroy(bool& consumed) {
	if(mouseInputStrategy_.get() != nullptr) {
		mouseInputStrategy_->interruptMouseReaction(false);
		mouseInputStrategy_->uninstall();
		mouseInputStrategy_.reset();
	}

	// destroy children
	::DestroyWindow(toolTip_);

#ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY
	if(accessibleProxy_ != nullptr) {
		accessibleProxy_->dispose();
		accessibleProxy_->Release();
	}
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
	return textRenderer().defaultFont()->nativeObject();
}

/// @see WM_HSCROLL
void TextViewer::onHScroll(UINT sbCode, UINT, const win32::Handle<HWND>&) {
	const shared_ptr<TextViewport> viewport(textRenderer().viewport());
	switch(sbCode) {
		case SB_LINELEFT:	// 1 列分左
			viewport->scroll(geometry::make<NativeSize>(-1, 0));
			break;
		case SB_LINERIGHT:	// 1 列分右
			viewport->scroll(geometry::make<NativeSize>(+1, 0));
			break;
		case SB_PAGELEFT:	// 1 ページ左
			viewport->scroll(geometry::make<NativeSize>(-abs(pageSize<geometry::X_COORDINATE>(*viewport)), 0));
			break;
		case SB_PAGERIGHT:	// 1 ページ右
			viewport->scroll(geometry::make<NativeSize>(+abs(pageSize<geometry::X_COORDINATE>(*viewport)), 0));
			break;
		case SB_LEFT:		// 左端
			viewport->scrollTo(
		case SB_RIGHT: {	// 右端
			const Range<int> range(horizontalScrollBar().range());
			scrollTo((sbCode == SB_LEFT) ? range.beginning() : range.end(), -1, true);
			break;
		}
		case SB_THUMBTRACK: {	// by drag or wheel
			win32::AutoZeroSize<SCROLLINFO> si;
			si.fMask = SIF_TRACKPOS;
			if(win32::boole(::GetScrollInfo(identifier().get(), SB_HORZ, &si)))
				scrollTo(si.nTrackPos, -1, false);
			break;
		}
	}
//	consumed = false;
}

namespace {
	// replaces single "&" with "&&".
	template<typename CharType>
	basic_string<CharType> escapeAmpersands(const basic_string<CharType>& s) {
		static const ctype<CharType>& ct = use_facet<ctype<CharType>>(locale::classic());
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

/// @see WM_SETCURSOR
void TextViewer::onSetCursor(const win32::Handle<HWND>&, UINT, UINT, bool& consumed) {
	cursorVanisher_.restore();
	if(consumed = (mouseInputStrategy_.get() != nullptr))
		mouseInputStrategy_->showCursor(mapFromGlobal(base::Cursor::position()));
}

/// @see WM_STYLECHANGED
void TextViewer::onStyleChanged(int type, const STYLESTRUCT& style) {
	if(type == GWL_EXSTYLE
			&& (((style.styleOld ^ style.styleNew) & (WS_EX_RIGHT | WS_EX_RTLREADING)) != 0)) {
		// synchronize the reading direction with the window's style
		// (ignore the alignment)
		Configuration c(configuration());
		c.readingDirection = ((style.styleNew & WS_EX_RTLREADING) != 0) ? RIGHT_TO_LEFT : LEFT_TO_RIGHT;
		setConfiguration(&c, nullptr, false);
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
			const Range<int> range(verticalScrollBar().range());
			scrollTo(-1, (sbCode == SB_TOP) ? range.beginning() : range.end(), true); break;
		}
		case SB_THUMBTRACK: {	// by drag or wheel
			win32::AutoZeroSize<SCROLLINFO> si;
			si.fMask = SIF_TRACKPOS;
			if(win32::boole(::GetScrollInfo(identifier().get(), SB_VERT, &si)))
				scrollTo(-1, si.nTrackPos, true);
			break;
		}
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

/// @see Widget#showContextMenu
void TextViewer::showContextMenu(const base::LocatedUserInput& input, bool byKeyboard) {
	using namespace win32::ui;

	if(!allowsMouseInput() && !byKeyboard)	// however, may be invoked by other than the mouse...
		return;
	utils::closeCompletionProposalsPopup(*this);
	texteditor::abortIncrementalSearch(*this);

	NativePoint menuPosition;

	// invoked by the keyboard
	if(byKeyboard/*geometry::x(input.location()) == 0xffff && geometry::y(input.location()) == 0xffff*/) {
		// MSDN says "the application should display the context menu at the location of the current selection."
		menuPosition = localPointForCharacter(caret(), false);
		geometry::y(menuPosition) += textRenderer().defaultFont()->metrics().cellHeight() + 1;
		NativeRectangle clientBounds(bounds(false));
		const PhysicalFourSides<Scalar> spaces(spaceWidths());
		clientBounds = geometry::make<NativeRectangle>(
			geometry::translate(geometry::topLeft(clientBounds), geometry::make<NativeSize>(spaces.left, spaces.top)),
			geometry::translate(geometry::bottomRight(clientBounds), geometry::make<NativeSize>(-spaces.right + 1, -spaces.bottom)));
		if(!geometry::includes(clientBounds, menuPosition))
			menuPosition = geometry::make<NativePoint>(1, 1);
		mapToGlobal(menuPosition);
	} else
		menuPosition = input.location();

	// ignore if the point is over the scroll bars
	const NativeRectangle clientBounds(bounds(false));
	mapToGlobal(clientBounds);
	if(!geometry::includes(clientBounds, menuPosition))
		return;

	const k::Document& doc = document();
	const bool hasSelection = !isSelectionEmpty(caret());
	const bool readOnly = doc.isReadOnly();
	const bool japanese = PRIMARYLANGID(userDefaultUILanguage()) == LANG_JAPANESE;

	static PopupMenu menu;
	static const WCHAR* captions[] = {
		L"&Undo",									L"\x5143\x306b\x623b\x3059(&U)",
		L"&Redo",									L"\x3084\x308a\x76f4\x3057(&R)",
		nullptr,									nullptr,
		L"Cu&t",									L"\x5207\x308a\x53d6\x308a(&T)",
		L"&Copy",									L"\x30b3\x30d4\x30fc(&C)",
		L"&Paste",									L"\x8cbc\x308a\x4ed8\x3051(&P)",
		L"&Delete",									L"\x524a\x9664(&D)",
		nullptr,									nullptr,
		L"Select &All",								L"\x3059\x3079\x3066\x9078\x629e(&A)",
		nullptr,									nullptr,
		L"&Right to left Reading order",			L"\x53f3\x304b\x3089\x5de6\x306b\x8aad\x3080(&R)",
		L"&Show Unicode control characters",		L"Unicode \x5236\x5fa1\x6587\x5b57\x306e\x8868\x793a(&S)",
		L"&Insert Unicode control character",		L"Unicode \x5236\x5fa1\x6587\x5b57\x306e\x633f\x5165(&I)",
		L"Insert Unicode &white space character",	L"Unicode \x7a7a\x767d\x6587\x5b57\x306e\x633f\x5165(&W)",
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
		PopupMenu subMenu;
		subMenu << Menu::StringItem(ID_INSERT_LRM, L"LRM\t&Left-To-Right Mark")
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
		subMenu.reset(win32::managed(::CreatePopupMenu()));
		subMenu << Menu::StringItem(ID_INSERT_U0020, L"U+0020\tSpace")
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
		if(!font::supportsComplexScripts()) {
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
	menu.enable<Menu::BY_COMMAND>(WM_PASTE, !readOnly && caret_->canPaste(false));
	menu.enable<Menu::BY_COMMAND>(WM_CLEAR, !readOnly && hasSelection);
	menu.enable<Menu::BY_COMMAND>(WM_SELECTALL, doc.numberOfLines() > 1 || doc.lineLength(0) > 0);
	menu.check<Menu::BY_COMMAND>(ID_RTLREADING, configuration_.readingDirection == RIGHT_TO_LEFT);
	menu.check<Menu::BY_COMMAND>(ID_DISPLAYSHAPINGCONTROLS, textRenderer().displaysShapingControls());

	// IME commands
	HKL keyboardLayout = ::GetKeyboardLayout(::GetCurrentThreadId());
	if(//toBoolean(::ImmIsIME(keyboardLayout)) &&
			::ImmGetProperty(keyboardLayout, IGP_SENTENCE) != IME_SMODE_NONE) {
		HIMC imc = ::ImmGetContext(identifier().get());
		WCHAR* openIme = japanese ? L"IME \x3092\x958b\x304f(&O)" : L"&Open IME";
		WCHAR* closeIme = japanese ? L"IME \x3092\x9589\x3058\x308b(&L)" : L"C&lose IME";
		WCHAR* openSftKbd = japanese ? L"\x30bd\x30d5\x30c8\x30ad\x30fc\x30dc\x30fc\x30c9\x3092\x958b\x304f(&E)" : L"Op&en soft keyboard";
		WCHAR* closeSftKbd = japanese ? L"\x30bd\x30d5\x30c8\x30ad\x30fc\x30dc\x30fc\x30c9\x3092\x9589\x3058\x308b(&F)" : L"Close so&ft keyboard";
		WCHAR* reconvert = japanese ? L"\x518d\x5909\x63db(&R)" : L"&Reconvert";

		menu << Menu::SeparatorItem()
			<< Menu::StringItem(ID_TOGGLEIMESTATUS, win32::boole(::ImmGetOpenStatus(imc)) ? closeIme : openIme);

		if(win32::boole(::ImmGetProperty(keyboardLayout, IGP_CONVERSION) & IME_CMODE_SOFTKBD)) {
			DWORD convMode;
			::ImmGetConversionStatus(imc, &convMode, nullptr);
			menu << Menu::StringItem(ID_TOGGLESOFTKEYBOARD, win32::boole(convMode & IME_CMODE_SOFTKBD) ? closeSftKbd : openSftKbd);
		}

		if(win32::boole(::ImmGetProperty(keyboardLayout, IGP_SETCOMPSTR) & SCS_CAP_SETRECONVERTSTRING))
			menu << Menu::StringItem(ID_RECONVERT, reconvert, (!readOnly && hasSelection) ? MFS_ENABLED : MFS_GRAYED);

		::ImmReleaseContext(identifier().get(), imc);
	}

	// hyperlink
	if(const hyperlink::Hyperlink* link = utils::getPointedHyperlink(*this, caret())) {
		const Index len = (link->region().end() - link->region().beginning()) * 2 + 8;
		unique_ptr<WCHAR[]> caption(new WCHAR[len]);	// TODO: this code can have buffer overflow in future
		swprintf(caption.get(),
#if(_MSC_VER < 1400)
#else
			len,
#endif // _MSC_VER < 1400
			japanese ? L"\x202a%s\x202c \x3092\x958b\x304f" : L"Open \x202a%s\x202c", escapeAmpersands(doc.line(
				caret().line()).substr(link->region().beginning(), link->region().end() - link->region().beginning())).c_str());
		menu << Menu::SeparatorItem() << Menu::StringItem(ID_INVOKE_HYPERLINK, caption.get());
	}

	menu.trackPopup(TPM_LEFTALIGN, geometry::x(menuPosition), geometry::y(menuPosition), identifier().get());

	// ...finally erase all items
	int c = menu.getNumberOfItems();
	while(c > 13)
		menu.erase<Menu::BY_POSITION>(c--);
}


// DefaultMouseInputStrategy //////////////////////////////////////////////////////////////////////

namespace {
	HRESULT createSelectionImage(const TextViewer& viewer, const NativePoint& cursorPosition, bool highlightSelection, SHDRAGIMAGE& image) {
		win32::Handle<HDC> dc(::CreateCompatibleDC(nullptr), &::DeleteDC);
		if(dc.get() == nullptr)
			return E_FAIL;

		win32::AutoZero<BITMAPV5HEADER> bh;
		bh.bV5Size = sizeof(BITMAPV5HEADER);
		bh.bV5Planes = 1;
		bh.bV5BitCount = 32;
		bh.bV5Compression = BI_BITFIELDS;
		bh.bV5RedMask = 0x00ff0000ul;
		bh.bV5GreenMask = 0x0000ff00ul;
		bh.bV5BlueMask = 0x000000fful;
		bh.bV5AlphaMask = 0xff000000ul;

		// determine the range to draw
		const k::Region selectedRegion(viewer.caret());
		Index firstLine, firstSubline;
		viewer.firstVisibleLine(&firstLine, nullptr, &firstSubline);

		// calculate the size of the image
		const NativeRectangle clientBounds(viewer.bounds(false));
		const TextRenderer& renderer = viewer.textRenderer();
		NativeRectangle selectionBounds(geometry::make<NativeRectangle>(
			geometry::make<NativePoint>(numeric_limits<Scalar>::max(), 0),
			geometry::make<NativeSize>(numeric_limits<Scalar>::min(), 0)));
		for(Index line = selectedRegion.beginning().line, e = selectedRegion.end().line; line <= e; ++line) {
			selectionBounds.bottom += static_cast<LONG>(renderer.defaultFont()->metrics().linePitch() * renderer.layouts()[line].numberOfLines());
			if(geometry::dy(selectionBounds) > geometry::dy(clientBounds))
				return S_FALSE;	// overflow
			const TextLayout& layout = renderer.layouts()[line];
			const Scalar indent = renderer.lineIndent(line);
			Range<Index> range;
			for(Index subline = 0, sublines = layout.numberOfLines(); subline < sublines; ++subline) {
				if(selectedRangeOnVisualLine(viewer.caret(), line, subline, range)) {
					range = Range<Index>(
						range.beginning(),
						min(viewer.document().lineLength(line), range.end()));
					const NativeRectangle sublineBounds(layout.bounds(range));
					selectionBounds.left() = min(geometry::left(sublineBounds) + indent, geometry::left(selectionBounds));
					selectionBounds.right() = max(geometry::right(sublineBounds) + indent, geometry::right(selectionBounds));
					if(geometry::dx(selectionBounds) > geometry::dx(clientBounds))
						return S_FALSE;	// overflow
				}
			}
		}
		bh.bV5Width = geometry::dx(selectionBounds);
		bh.bV5Height = geometry::dy(selectionBounds);

		// create a mask
		win32::Handle<HBITMAP> mask(::CreateBitmap(bh.bV5Width, bh.bV5Height, 1, 1, 0), &::DeleteObject);	// monochrome
		if(mask.get() == nullptr)
			return E_FAIL;
		HBITMAP oldBitmap = static_cast<HBITMAP>(::SelectObject(dc.get(), mask.get()));
		dc.fillSolidRect(0, 0, bh.bV5Width, bh.bV5Height, RGB(0x00, 0x00, 0x00));
		int y = 0;
		for(Index line = selectedRegion.beginning().line, e = selectedRegion.end().line; line <= e; ++line) {
			const TextLayout& layout = renderer.layouts()[line];
			const int indent = renderer.lineIndent(line);
			Range<Index> range;
			for(Index subline = 0, sublines = layout.numberOfLines(); subline < sublines; ++subline) {
				if(selectedRangeOnVisualLine(viewer.caret(), line, subline, range)) {
					range = Range<Index>(
						range.beginning(),
						min(viewer.document().lineLength(line), range.end()));
					NativeRegion rgn(layout.blackBoxBounds(range));
					::OffsetRgn(rgn.get(), indent - geometry::left(selectionBounds), y - geometry::top(selectionBounds));
					::FillRgn(dc.get(), rgn.get(), static_cast<HBRUSH>(::GetStockObject(WHITE_BRUSH)));
				}
				y += renderer.defaultFont()->metrics().linePitch();
			}
		}
		::SelectObject(dc.get(), oldBitmap);
		BITMAPINFO* bi = nullptr;
		unique_ptr<byte[]> maskBuffer;
		uint8_t* maskBits;
		BYTE alphaChunnels[2] = {0xff, 0x01};
		try {
			bi = static_cast<BITMAPINFO*>(::operator new(sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * 2));
			memset(&bi->bmiHeader, 0, sizeof(BITMAPINFOHEADER));
			bi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			int r = ::GetDIBits(dc.get(), mask.get(), 0, bh.bV5Height, nullptr, bi, DIB_RGB_COLORS);
			if(r == 0 || r == ERROR_INVALID_PARAMETER)
				throw runtime_error("");
			assert(bi->bmiHeader.biBitCount == 1 && bi->bmiHeader.biClrUsed == 2);
			maskBuffer.reset(new uint8_t[bi->bmiHeader.biSizeImage + sizeof(DWORD)]);
			maskBits = maskBuffer.get() + sizeof(DWORD) - reinterpret_cast<ULONG_PTR>(maskBuffer.get()) % sizeof(DWORD);
			r = ::GetDIBits(dc.get(), mask.get(), 0, bh.bV5Height, maskBits, bi, DIB_RGB_COLORS);
			if(r == 0 || r == ERROR_INVALID_PARAMETER)
				throw runtime_error("");
			if(bi->bmiColors[0].rgbRed == 0xff && bi->bmiColors[0].rgbGreen == 0xff && bi->bmiColors[0].rgbBlue == 0xff)
				swap(alphaChunnels[0], alphaChunnels[1]);
		} catch(const bad_alloc&) {
			return E_OUTOFMEMORY;
		} catch(const runtime_error&) {
			::operator delete(bi);
			return E_FAIL;
		}
		::operator delete(bi);

		// create the result bitmap
		void* bits;
		win32::Handle<HBITMAP> bitmap(::CreateDIBSection(dc.get(), *reinterpret_cast<BITMAPINFO*>(&bh), DIB_RGB_COLORS, bits));
		if(bitmap.get() == nullptr)
			return E_FAIL;
		// render the lines
		oldBitmap = ::SelectObject(dc.get(), bitmap.get());
		NativeRectangle selectionExtent(selectionBounds);
		geometry::translate(selectionExtent, geometry::negate(geometry::topLeft(selectionExtent)));
		y = selectionBounds.top;
		const TextLayout::Selection selection(viewer.caret());
		for(Index line = selectedRegion.beginning().line, e = selectedRegion.end().line; line <= e; ++line) {
			renderer.renderLine(line, dc,
				renderer.lineIndent(line) - geometry::left(selectionBounds), y,
				selectionExtent, selectionExtent, highlightSelection ? &selection : nullptr);
			y += static_cast<int>(renderer.defaultFont()->metrics().linePitch() * renderer.numberOfLinesOfLine(line));
		}
		::SelectObject(dc.get(), oldBitmap);

		// set alpha chunnel
		const uint8_t* maskByte = maskBits;
		for(LONG y = 0; y < bh.bV5Height; ++y) {
			for(LONG x = 0; ; ) {
				RGBQUAD& pixel = static_cast<RGBQUAD*>(bits)[x + bh.bV5Width * y];
				pixel.rgbReserved = alphaChunnels[(*maskByte & (1 << ((8 - x % 8) - 1))) ? 0 : 1];
				if(x % 8 == 7)
					++maskByte;
				if(++x == bh.bV5Width) {
					if(x % 8 != 0)
						++maskByte;
					break;
				}
			}
			if(reinterpret_cast<ULONG_PTR>(maskByte) % sizeof(DWORD) != 0)
				maskByte += sizeof(DWORD) - reinterpret_cast<ULONG_PTR>(maskByte) % sizeof(DWORD);
		}

		// locate the hotspot of the image based on the cursor position
		const PhysicalFourSides<Scalar> spaces(viewer.spaceWidths());
		NativePoint hotspot(cursorPosition);
		geometry::x(hotspot) -= margins.left - viewer.horizontalScrollBar().position() * renderer.defaultFont()->metrics().averageCharacterWidth() + geometry::left(selectionBounds);
		geometry::y(hotspot) -= geometry::y(viewer.clientXYForCharacter(k::Position(selectedRegion.beginning().line, 0), true));

		memset(&image, 0, sizeof(SHDRAGIMAGE));
		image.sizeDragImage.cx = bh.bV5Width;
		image.sizeDragImage.cy = bh.bV5Height;
		image.ptOffset = hotspot;
		image.hbmpDragImage = static_cast<HBITMAP>(bitmap.release());
		image.crColorKey = CLR_NONE;

		return S_OK;
	}
}

HRESULT DefaultMouseInputStrategy::doDragAndDrop() {
	win32::com::ComPtr<IDataObject> draggingContent;
	const Caret& caret = viewer_->caret();
	HRESULT hr;

	if(FAILED(hr = utils::createTextObjectForSelectedString(viewer_->caret(), true, *draggingContent.initialize())))
		return hr;
	if(!caret.isSelectionRectangle())
		dnd_.numberOfRectangleLines = 0;
	else {
		const k::Region selection(caret.selectedRegion());
		dnd_.numberOfRectangleLines = selection.end().line - selection.beginning().line + 1;
	}

	// setup drag-image
	if(dnd_.dragSourceHelper.get() != nullptr) {
		SHDRAGIMAGE image;
		if(SUCCEEDED(hr = createSelectionImage(*viewer_,
				dragApproachedPosition_, dnd_.supportLevel >= SUPPORT_DND_WITH_SELECTED_DRAG_IMAGE, image))) {
			if(FAILED(hr = dnd_.dragSourceHelper->InitializeFromBitmap(&image, draggingContent.get())))
				::DeleteObject(image.hbmpDragImage);
		}
	}

	// operation
	state_ = DND_SOURCE;
	DWORD effectOwn;	// dummy
	hr = ::DoDragDrop(draggingContent.get(), this, DROPEFFECT_COPY | DROPEFFECT_MOVE | DROPEFFECT_SCROLL, &effectOwn);
	state_ = NONE;
	if(viewer_->isVisible())
		viewer_->setFocus();
	return hr;
}

/// @see IDropTarget#DragEnter
STDMETHODIMP DefaultMouseInputStrategy::DragEnter(IDataObject* data, DWORD keyState, POINTL pt, DWORD* effect) {
	if(data == nullptr)
		return E_INVALIDARG;
	ASCENSION_WIN32_VERIFY_COM_POINTER(effect);
	*effect = DROPEFFECT_NONE;

	HRESULT hr;

#ifdef _DEBUG
	{
		win32::DumpContext dout;
		win32::com::ComPtr<IEnumFORMATETC> formats;
		if(SUCCEEDED(hr = data->EnumFormatEtc(DATADIR_GET, formats.initialize()))) {
			FORMATETC format;
			ULONG fetched;
			dout << L"DragEnter received a data object exposes the following formats.\n";
			for(formats->Reset(); formats->Next(1, &format, &fetched) == S_OK; ) {
				WCHAR name[256];
				if(::GetClipboardFormatNameW(format.cfFormat, name, ASCENSION_COUNTOF(name) - 1) != 0)
					dout << L"\t" << name << L"\n";
				else
					dout << L"\t" << L"(unknown format : " << format.cfFormat << L")\n";
				if(format.ptd != nullptr)
					::CoTaskMemFree(format.ptd);
			}
		}
	}
#endif // _DEBUG

	if(dnd_.supportLevel == DONT_SUPPORT_DND || viewer_->document().isReadOnly() || !viewer_->allowsMouseInput())
		return S_OK;

	// validate the dragged data if can drop
	FORMATETC fe = {CF_UNICODETEXT, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
	if((hr = data->QueryGetData(&fe)) != S_OK) {
		fe.cfFormat = CF_TEXT;
		if(SUCCEEDED(hr = data->QueryGetData(&fe) != S_OK))
			return S_OK;	// can't accept
	}

	if(state_ != DND_SOURCE) {
		assert(state_ == NONE);
		// retrieve number of lines if text is rectangle
		dnd_.numberOfRectangleLines = 0;
		fe.cfFormat = static_cast<CLIPFORMAT>(::RegisterClipboardFormatW(ASCENSION_RECTANGLE_TEXT_CLIP_FORMAT));
		if(fe.cfFormat != 0 && data->QueryGetData(&fe) == S_OK) {
			const TextAlignment alignment = defaultTextAlignment(viewer_->presentation());
			const ReadingDirection readingDirection = defaultReadingDirection(viewer_->presentation());
			if(alignment == ALIGN_END
					|| (alignment == ALIGN_LEFT && readingDirection == RIGHT_TO_LEFT)
					|| (alignment == ALIGN_RIGHT && readingDirection == LEFT_TO_RIGHT))
				return S_OK;	// TODO: support alignments other than ALIGN_LEFT.
			pair<HRESULT, String> text(utils::getTextFromDataObject(*data));
			if(SUCCEEDED(text.first))
				dnd_.numberOfRectangleLines = text::calculateNumberOfLines(text.second) - 1;
		}
		state_ = DND_TARGET;
	}

	viewer_->setFocus();
	timer_.start(DRAGGING_TRACK_INTERVAL, *this);
	if(dnd_.dropTargetHelper.get() != nullptr) {
		POINT p = {pt.x, pt.y};
		hr = dnd_.dropTargetHelper->DragEnter(viewer_->identifier().get(), data, &p, *effect);
	}
	return DragOver(keyState, pt, effect);
}

/// @see IDropTarget#Drop
STDMETHODIMP DefaultMouseInputStrategy::Drop(IDataObject* data, DWORD keyState, POINTL pt, DWORD* effect) {
	if(dnd_.dropTargetHelper.get() != nullptr) {
		POINT p = {pt.x, pt.y};
		dnd_.dropTargetHelper->Drop(data, &p, *effect);
	}
	if(data == nullptr)
		return E_INVALIDARG;
	ASCENSION_WIN32_VERIFY_COM_POINTER(effect);
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
	k::Document& document = viewer_->document();
	if(dnd_.supportLevel == DONT_SUPPORT_DND || document.isReadOnly() || !viewer_->allowsMouseInput())
		return S_OK;
	Caret& ca = viewer_->caret();
	const NativePoint caretPoint(viewer_->mapFromGlobal(geometry::make<NativePoint>(pt.x, pt.y)));
	const k::Position destination(viewer_->characterForClientXY(caretPoint, TextLayout::TRAILING));

	if(!document.accessibleRegion().includes(destination))
		return S_OK;

	if(state_ == DND_TARGET) {	// dropped from the other widget
		timer_.stop();
		ca.moveTo(destination);

		bool rectangle;
		pair<HRESULT, String> content(utils::getTextFromDataObject(*data, &rectangle));
		if(SUCCEEDED(content.first)) {
			AutoFreeze af(viewer_);
			bool failed = false;
			ca.moveTo(destination);
			try {
				ca.replaceSelection(content.second, rectangle);
			} catch(...) {
				failed = true;
			}
			if(!failed) {
				if(rectangle)
					ca.beginRectangleSelection();
				ca.select(destination, ca);
				*effect = DROPEFFECT_COPY;
			}
		}
		state_ = NONE;
	} else {	// drop from the same widget
		assert(state_ == DND_SOURCE);
		String text(selectedString(ca, text::NLF_RAW_VALUE));

		// can't drop into the selection
		if(isPointOverSelection(ca, caretPoint)) {
			ca.moveTo(destination);
			state_ = NONE;
		} else {
			const bool rectangle = ca.isSelectionRectangle();
			document.insertUndoBoundary();
			AutoFreeze af(viewer_);
			if(win32::boole(keyState & MK_CONTROL)) {	// copy
//				viewer_->redrawLines(ca.beginning().line(), ca.end().line());
				bool failed = false;
				ca.enableAutoShow(false);
				ca.moveTo(destination);
				try {
					ca.replaceSelection(text, rectangle);
				} catch(...) {
					failed = true;
				}
				ca.enableAutoShow(true);
				if(!failed) {
					ca.select(destination, ca);
					*effect = DROPEFFECT_COPY;
				}
			} else {	// move as a rectangle or linear
				bool failed = false;
				pair<k::Point, k::Point> oldSelection(make_pair(k::Point(ca.anchor()), k::Point(ca)));
				ca.enableAutoShow(false);
				ca.moveTo(destination);
				try {
					ca.replaceSelection(text, rectangle);
				} catch(...) {
					failed = true;
				}
				if(!failed) {
					ca.select(destination, ca);
					if(rectangle)
						ca.beginRectangleSelection();
					try {
						erase(ca.document(), oldSelection.first, oldSelection.second);
					} catch(...) {
						failed = true;
					}
				}
				ca.enableAutoShow(true);
				if(!failed)
					*effect = DROPEFFECT_MOVE;
			}
			document.insertUndoBoundary();
		}
	}
	return S_OK;
}

/// @see IDropSource#GiveFeedback
STDMETHODIMP DefaultMouseInputStrategy::GiveFeedback(DWORD) {
	return DRAGDROP_S_USEDEFAULTCURSORS;	// use the system default cursor
}

/// @see IDropSource#QueryContinueDrag
STDMETHODIMP DefaultMouseInputStrategy::QueryContinueDrag(BOOL escapePressed, DWORD keyState) {
	if(win32::boole(escapePressed) || win32::boole(keyState & MK_RBUTTON))	// cancel
		return DRAGDROP_S_CANCEL;
	if(!win32::boole(keyState & MK_LBUTTON))	// drop
		return DRAGDROP_S_DROP;
	return S_OK;
}
