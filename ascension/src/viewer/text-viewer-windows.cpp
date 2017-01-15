/**
 * @file text-viewer-windows.cpp
 * @author exeal
 * @date 2003-2006 was EditView.cpp and EditViewWindowMessages.cpp
 * @date 2006-2011 was viewer.cpp
 * @date 2011-05-16 separated from viewer.cpp
 * @date 2011-2015
 */

#include <ascension/viewer/text-viewer.hpp>
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)

#include <ascension/graphics/font/font.hpp>
#include <ascension/graphics/font/font-metrics.hpp>
#include <ascension/graphics/font/text-viewport.hpp>
#include <ascension/graphics/geometry/named-parameters.hpp>
#include <ascension/graphics/geometry/native-conversions.hpp>
#include <ascension/graphics/geometry/point-xy.hpp>
#include <ascension/graphics/geometry/rectangle-corners.hpp>
#include <ascension/graphics/geometry/rectangle-odxdy.hpp>
#include <ascension/graphics/geometry/algorithms/make.hpp>
#include <ascension/graphics/paint-context.hpp>
#include <ascension/kernel/document.hpp>
#include <ascension/log.hpp>
#include <ascension/presentation/hyperlink/hyperlink.hpp>
#include <ascension/text-editor/commands/caret-motions.hpp>
#include <ascension/text-editor/commands/conversions.hpp>
#include <ascension/text-editor/commands/deletions.hpp>
#include <ascension/text-editor/commands/inputs.hpp>
#include <ascension/text-editor/commands/modals.hpp>
#include <ascension/text-editor/commands/rollbacks.hpp>
#include <ascension/text-editor/commands/yanks.hpp>
#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/text-area.hpp>
#include <ascension/viewer/text-viewer-utility.hpp>
#include <ascension/viewer/widgetapi/cursor.hpp>
#include <ascension/text-editor/command.hpp>
#include <ascension/text-editor/input-sequence-checker.hpp>
#include <ascension/win32/windows.hpp>
#include <ascension/win32/ui/wait-cursor.hpp>
#include <boost/geometry/algorithms/covered_by.hpp>
#ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY
#	include <ascension/win32/com/dispatch-impl.hpp>
#	include <boost/dll/shared_library.hpp>
#endif // !ASCENSION_NO_ACTIVE_ACCESSIBILITY
#include <msctf.h>
#include <zmouse.h>
#ifndef ASCENSION_NO_TEXT_SERVICES_FRAMEWORK
#	include <Textstor.h>
#endif // !ASCENSION_NO_TEXT_SERVICES_FRAMEWORK
#pragma comment(lib, "version.lib")

namespace ascension {
	namespace viewer {

#ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY

		// TextViewer.AccessibleProxy /////////////////////////////////////////////////////////////////////////////////

		namespace {
			class AccLib {
			public:
				AccLib() BOOST_NOEXCEPT {
					try {
						oleaccDll_.load("oleacc.dll", boost::dll::load_mode::search_system_folders);
						user32Dll_.load("user32.dll", boost::dll::load_mode::search_system_folders);
						accessibleObjectFromWindow_ = oleaccDll_.get<LPFNACCESSIBLEOBJECTFROMWINDOW>("AccessibleObjectFromWindow");
						createStdAccessibleObject_ = oleaccDll_.get<LPFNCREATESTDACCESSIBLEOBJECT>("CreateStdAccessibleObject");
						lresultFromObject_ = oleaccDll_.get<LPFNLRESULTFROMOBJECT>("LresultFromObject");
						notifyWinEvent_ = user32Dll_.get<VOID(WINAPI *)(DWORD, HWND, LONG, LONG)>("NotifyWinEvent");
					} catch(const std::system_error&) {
						oleaccDll_.unload();
						user32Dll_.unload();
					}
				}
				bool isAvailable() const BOOST_NOEXCEPT {
					return oleaccDll_.is_loaded() && user32Dll_.is_loaded();
				}
				HRESULT accessibleObjectFromWindow(HWND window, DWORD objectID, REFIID iid, void** object) {
					return (*accessibleObjectFromWindow_)(window, objectID, iid, object);
				}
				HRESULT createStdAccessibleObject(HWND window, long objectID, REFIID iid, void** object) {
					return (*createStdAccessibleObject_)(window, objectID, iid, object);
				}
				LRESULT lresultFromObject(REFIID iid, WPARAM wParam, LPUNKNOWN object) {
					return (*lresultFromObject_)(iid, wParam, object);
				}
				void notifyWinEvent(DWORD event, HWND window, long objectID, long childID) {
					return (*notifyWinEvent_)(event, window, objectID, childID);
				}

			private:
				boost::dll::shared_library oleaccDll_, user32Dll_;
				LPFNACCESSIBLEOBJECTFROMWINDOW accessibleObjectFromWindow_;
				LPFNCREATESTDACCESSIBLEOBJECT createStdAccessibleObject_;
				LPFNLRESULTFROMOBJECT lresultFromObject_;
				VOID(WINAPI *notifyWinEvent_)(DWORD, HWND, LONG, LONG);
			} accLib;

			/**
			 * @c TextViewer#AccessibleProxy is proxy object for @c IAccessible interface of @c TextViewer instance.
			 * @see TextViewer#getAccessibleObject, ASCENSION_NO_ACTIVE_ACCESSIBILITY
			 */
			class AccessibleProxy :
					public kernel::DocumentListener,
					public win32::com::IDispatchImpl<
						win32::com::IUnknownImpl<
							typelist::Cat<win32::com::Interface<detail::AbstractAccessibleProxy, &IID_IAccessible>,
							typelist::Cat<ASCENSION_WIN32_COM_INTERFACE(IDispatch),
							typelist::Cat<ASCENSION_WIN32_COM_INTERFACE(IOleWindow)>>>,
							win32::com::NoReferenceCounting
						>,
						win32::com::TypeInformationFromRegistry<&LIBID_Accessibility, &IID_IAccessible>
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
			public:
				// constructor
				explicit AccessibleProxy(TextViewer& viewer) BOOST_NOEXCEPT;
				// method
				void dispose();
				// IAccessible
				STDMETHODIMP get_accParent(IDispatch** ppdispParent) override;
				STDMETHODIMP get_accChildCount(long* pcountChildren) override;
				STDMETHODIMP get_accChild(VARIANT varChild, IDispatch** ppdispChild) override;
				STDMETHODIMP get_accName(VARIANT varChild, BSTR* pszName) override;
				STDMETHODIMP get_accValue(VARIANT varChild, BSTR* pszValue) override;
				STDMETHODIMP get_accDescription(VARIANT varChild, BSTR* pszDescription) override;
				STDMETHODIMP get_accRole(VARIANT varChild, VARIANT* pvarRole) override;
				STDMETHODIMP get_accState(VARIANT varChild, VARIANT* pvarState) override;
				STDMETHODIMP get_accHelp(VARIANT varChild, BSTR* pszHelp) override;
				STDMETHODIMP get_accHelpTopic(BSTR* pszHelpFile, VARIANT varChild, long* pidTopic) override;
				STDMETHODIMP get_accKeyboardShortcut(VARIANT varChild, BSTR* pszKeyboardShortcut) override;
				STDMETHODIMP get_accFocus(VARIANT* pvarChild) override;
				STDMETHODIMP get_accSelection(VARIANT* pvarChildren) override;
				STDMETHODIMP get_accDefaultAction(VARIANT varChild, BSTR* pszDefaultAction) override;
				STDMETHODIMP accSelect(long flagsSelect, VARIANT varChild) override;
				STDMETHODIMP accLocation(long* pxLeft, long* pyTop, long* pcxWidth, long* pcyHeight, VARIANT varChild) override;
				STDMETHODIMP accNavigate(long navDir, VARIANT varStart, VARIANT* pvarEndUpAt) override;
				STDMETHODIMP accHitTest(long xLeft, long yTop, VARIANT* pvarChild) override;
				STDMETHODIMP accDoDefaultAction(VARIANT varChild) override;
				STDMETHODIMP put_accName(VARIANT varChild, BSTR szName) override;
				STDMETHODIMP put_accValue(VARIANT varChild, BSTR szValue) override;
				// IOleWindow
				STDMETHODIMP GetWindow(HWND* phwnd) override;
				STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode) override;

			private:
				// DocumentListener
				void documentAboutToBeChanged(const kernel::Document& document) override;
				void documentChanged(const kernel::Document& document, const kernel::DocumentChange& change) override;
			private:
				TextViewer& viewer_;
				bool available_;
				win32::com::SmartPointer<IAccessible> defaultServer_;
//				enum {CHILDID_SELECTION = 1};
			};
		} // namespace @0

#define ASCENSION_VERIFY_AVAILABILITY()	\
	if(!available_) return RPC_E_DISCONNECTED

		/**
		 * Constructor.
		 * @param view the viewer
		 */
		AccessibleProxy::AccessibleProxy(TextViewer& viewer) BOOST_NOEXCEPT : viewer_(viewer), available_(true) {
			assert(accLib.isAvailable());
			accLib.createStdAccessibleObject(viewer.handle().get(), OBJID_CLIENT, IID_IAccessible, defaultServer_.initializePPV());
		}

		/// @see IAccessible#accDoDefaultAction
		STDMETHODIMP AccessibleProxy::accDoDefaultAction(VARIANT) {
			ASCENSION_VERIFY_AVAILABILITY();
			return DISP_E_MEMBERNOTFOUND;
		}

		/// @see IAccessible#accHitTest
		STDMETHODIMP AccessibleProxy::accHitTest(long xLeft, long yTop, VARIANT* pvarChild) {
			ASCENSION_VERIFY_AVAILABILITY();
			// this code does not support non-rectangular window
			ASCENSION_WIN32_VERIFY_COM_POINTER(pvarChild);
			auto p(graphics::geometry::make<graphics::Point>((graphics::geometry::_x = static_cast<graphics::Scalar>(xLeft), graphics::geometry::_y = static_cast<graphics::Scalar>(yTop))));
			p = widgetapi::mapFromGlobal(viewer_, p);
			if(boost::geometry::covered_by(p, widgetapi::bounds(viewer_, false))) {
				pvarChild->vt = VT_I4;
				pvarChild->lVal = CHILDID_SELF;
				return S_OK;
			} else {
				pvarChild->vt = VT_EMPTY;
				return S_FALSE;
			}
		}

		/// @see IAccessible#accLocation
		STDMETHODIMP AccessibleProxy::accLocation(long* pxLeft, long* pyTop, long* pcxWidth, long* pcyHeight, VARIANT varChild) {
			ASCENSION_VERIFY_AVAILABILITY();
			ASCENSION_WIN32_VERIFY_COM_POINTER(pxLeft);
			ASCENSION_WIN32_VERIFY_COM_POINTER(pyTop);
			ASCENSION_WIN32_VERIFY_COM_POINTER(pcxWidth);
			ASCENSION_WIN32_VERIFY_COM_POINTER(pcyHeight);
			if(varChild.vt != VT_I4 || varChild.lVal != CHILDID_SELF)
				return E_INVALIDARG;
			const auto clientBounds(widgetapi::bounds(viewer_, false));
			const auto origin(widgetapi::mapToGlobal(viewer_, graphics::geometry::topLeft(clientBounds)));
			*pxLeft = static_cast<long>(graphics::geometry::x(origin));
			*pyTop = static_cast<long>(graphics::geometry::y(origin));
			*pcxWidth = static_cast<long>(graphics::geometry::dx(clientBounds));
			*pcyHeight = static_cast<long>(graphics::geometry::dy(clientBounds));
			return S_OK;
		}

		/// @see IAccessible#accNavigate
		STDMETHODIMP AccessibleProxy::accNavigate(long navDir, VARIANT varStart, VARIANT* pvarEndUpAt) {
			ASCENSION_VERIFY_AVAILABILITY();
			return defaultServer_->accNavigate(navDir, varStart, pvarEndUpAt);
		}

		/// @see IAccessible#accSelect
		STDMETHODIMP AccessibleProxy::accSelect(long flagsSelect, VARIANT varChild) {
			ASCENSION_VERIFY_AVAILABILITY();
			return (varChild.vt == VT_I4 && varChild.lVal == CHILDID_SELF) ?
				defaultServer_->accSelect(flagsSelect, varChild) : E_INVALIDARG;
		}

		/// @see IOleWindow#ContextSensitiveHelp
		STDMETHODIMP AccessibleProxy::ContextSensitiveHelp(BOOL fEnterMode) {
			return S_OK;	// not supported
		}

		/// Informs that the viewer is inavailable to the proxy.
		void AccessibleProxy::dispose() {
			if(!available_)
				throw IllegalStateException("This proxy is already disposed.");
			available_ = false;
		}

		/// @see DocumentListener#documentAboutToBeChanged
		void AccessibleProxy::documentAboutToBeChanged(const kernel::Document&) {
			// do nothing
		}

		/// @see DocumentListener#documentChanged
		void AccessibleProxy::documentChanged(const kernel::Document&, const kernel::DocumentChange&) {
			assert(accLib.isAvailable());
			accLib.notifyWinEvent(EVENT_OBJECT_VALUECHANGE, viewer_.handle().get(), OBJID_CLIENT, CHILDID_SELF);
		}

		/// @see IAccessible#get_accChild
		STDMETHODIMP AccessibleProxy::get_accChild(VARIANT, IDispatch** ppdispChild) {
			ASCENSION_VERIFY_AVAILABILITY();
			ASCENSION_WIN32_VERIFY_COM_POINTER(ppdispChild);
			*ppdispChild = nullptr;
			return S_OK;
		}

		/// @see IAccessible#get_accChildCount
		STDMETHODIMP AccessibleProxy::get_accChildCount(long* pcountChildren) {
			ASCENSION_VERIFY_AVAILABILITY();
			ASCENSION_WIN32_VERIFY_COM_POINTER(pcountChildren);
			*pcountChildren = 0;
			return S_OK;
		}

		/// @see IAccessible#get_accDefaultAction
		STDMETHODIMP AccessibleProxy::get_accDefaultAction(VARIANT, BSTR*) {
			ASCENSION_VERIFY_AVAILABILITY();
			return DISP_E_MEMBERNOTFOUND;
		}

		/// @see IAccessible#get_accDescription
		STDMETHODIMP AccessibleProxy::get_accDescription(VARIANT, BSTR*) {
			ASCENSION_VERIFY_AVAILABILITY();
			return DISP_E_MEMBERNOTFOUND;
		}

		/// @see IAccessible#get_accFocus
		STDMETHODIMP AccessibleProxy::get_accFocus(VARIANT* pvarChild) {
			ASCENSION_VERIFY_AVAILABILITY();
			ASCENSION_WIN32_VERIFY_COM_POINTER(pvarChild);
			pvarChild->vt = VT_I4;
			pvarChild->lVal = CHILDID_SELF;
			return S_OK;
		}

		/// @see IAccessible#get_accHelp
		STDMETHODIMP AccessibleProxy::get_accHelp(VARIANT, BSTR*) {
			ASCENSION_VERIFY_AVAILABILITY();
			return DISP_E_MEMBERNOTFOUND;
		}

		/// @see IAccessible#get_accHelpTopic
		STDMETHODIMP AccessibleProxy::get_accHelpTopic(BSTR*, VARIANT, long*) {
			ASCENSION_VERIFY_AVAILABILITY();
			return DISP_E_MEMBERNOTFOUND;
		}

		/// @see IAccessible#get_accKeyboardShortcut
		STDMETHODIMP AccessibleProxy::get_accKeyboardShortcut(VARIANT varChild, BSTR* pszKeyboardShortcut) {
			ASCENSION_VERIFY_AVAILABILITY();
			ASCENSION_WIN32_VERIFY_COM_POINTER(pszKeyboardShortcut);
			*pszKeyboardShortcut = nullptr;
			if(varChild.vt != VT_I4 || varChild.lVal != CHILDID_SELF)
				return E_INVALIDARG;
			return S_FALSE;
		}

		/// @see IAccessible#get_accName
		STDMETHODIMP AccessibleProxy::get_accName(VARIANT varChild, BSTR* pszName) {
			ASCENSION_VERIFY_AVAILABILITY();
			ASCENSION_WIN32_VERIFY_COM_POINTER(pszName);
			*pszName = nullptr;
			if(varChild.vt != VT_I4 || varChild.lVal != CHILDID_SELF)
				return E_INVALIDARG;
			return S_FALSE;
		}

		/// @see IAccessible#get_accParent
		STDMETHODIMP AccessibleProxy::get_accParent(IDispatch** ppdispParent) {
			ASCENSION_VERIFY_AVAILABILITY();
			if(accLib.isAvailable())
				return accLib.accessibleObjectFromWindow(viewer_.handle().get(),
					OBJID_WINDOW, IID_IAccessible, reinterpret_cast<void**>(ppdispParent));
			return defaultServer_->get_accParent(ppdispParent);
		}

		/// @see IAccessible#get_accRole
		STDMETHODIMP AccessibleProxy::get_accRole(VARIANT varChild, VARIANT* pvarRole) {
			ASCENSION_VERIFY_AVAILABILITY();
			ASCENSION_WIN32_VERIFY_COM_POINTER(pvarRole);
			if(varChild.vt != VT_I4 || varChild.lVal != CHILDID_SELF)
				return E_INVALIDARG;
			pvarRole->vt = VT_I4;
			pvarRole->lVal = ROLE_SYSTEM_TEXT;
			return S_OK;
		}

		/// @see IAccessible#get_accSelection
		STDMETHODIMP AccessibleProxy::get_accSelection(VARIANT* pvarChildren) {
			ASCENSION_VERIFY_AVAILABILITY();
			ASCENSION_WIN32_VERIFY_COM_POINTER(pvarChildren);
			pvarChildren->vt = VT_EMPTY;
			return S_FALSE;
		}

		/// @see IAccessible#get_accState
		STDMETHODIMP AccessibleProxy::get_accState(VARIANT varChild, VARIANT* pvarState) {
			ASCENSION_VERIFY_AVAILABILITY();
			if(varChild.vt != VT_I4 || varChild.lVal != CHILDID_SELF)
				return E_INVALIDARG;
			pvarState->vt = VT_I4;
			pvarState->lVal = 0;	// STATE_SYSTEM_NORMAL;
			if(!widgetapi::isVisible(viewer_))
				pvarState->lVal |= STATE_SYSTEM_INVISIBLE;
			if(::GetTopWindow(viewer_.handle().get()) == ::GetActiveWindow())
				pvarState->lVal |= STATE_SYSTEM_FOCUSABLE;
			if(widgetapi::hasFocus(viewer_))
				pvarState->lVal |= STATE_SYSTEM_FOCUSED;
			if(viewer_.document()->isReadOnly())
				pvarState->lVal |= STATE_SYSTEM_READONLY;
			return S_OK;
		}

		/// @see IAccessible#get_accValue
		STDMETHODIMP AccessibleProxy::get_accValue(VARIANT varChild, BSTR* pszValue) {
			ASCENSION_VERIFY_AVAILABILITY();
			ASCENSION_WIN32_VERIFY_COM_POINTER(pszValue);
			if(varChild.vt != VT_I4 || varChild.lVal != CHILDID_SELF)
				return E_INVALIDARG;
			std::basic_ostringstream<Char> s;
			kernel::writeDocumentToStream(s, *viewer_.document(), viewer_.document()->region());
			*pszValue = ::SysAllocString(reinterpret_cast<const OLECHAR*>(s.str().c_str()));
			return (*pszValue != nullptr) ? S_OK : E_OUTOFMEMORY;
		}

		/// @see IOleWindow#GetWindow
		STDMETHODIMP AccessibleProxy::GetWindow(HWND* phwnd) {
			ASCENSION_VERIFY_AVAILABILITY();
			ASCENSION_WIN32_VERIFY_COM_POINTER(phwnd);
			*phwnd = viewer_.handle().get();
			return S_OK;
		}

		/// @see IAccessible#put_accName
		STDMETHODIMP AccessibleProxy::put_accName(VARIANT, BSTR) {
			ASCENSION_VERIFY_AVAILABILITY();
			return DISP_E_MEMBERNOTFOUND;
		}

		/// @see IAccessible#put_accValue
		STDMETHODIMP AccessibleProxy::put_accValue(VARIANT varChild, BSTR szValue) {
			ASCENSION_VERIFY_AVAILABILITY();
			if(varChild.vt != VT_I4 || varChild.lVal != CHILDID_SELF)
				return E_INVALIDARG;
			else if(viewer_.document()->isReadOnly())
				return E_ACCESSDENIED;
			if(::SysStringLen(szValue) != 0)
				viewer_.textArea()->caret()->replaceSelection(reinterpret_cast<const Char*>(szValue));
			else
				viewer_.textArea()->caret()->replaceSelection(String());
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


		// TextViewer /////////////////////////////////////////////////////////////////////////////////////////////////

		namespace {
			inline std::shared_ptr<Caret> tryCaret(TextViewer& textViewer) {
				if(auto textArea = textViewer.textArea()) {
					if(auto caret = textArea->caret())
						return caret;
				}
				return std::shared_ptr<Caret>();
			}
		}

#ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY
		/// Returns the accessible proxy of the viewer.
		HRESULT TextViewer::accessibleObject(IAccessible*& acc) const BOOST_NOEXCEPT {
			TextViewer& self = *const_cast<TextViewer*>(this);
			acc = nullptr;
			if(accessibleProxy_.get() == nullptr && win32::boole(::IsWindow(handle().get())) && accLib.isAvailable()) {
				try {
					self.accessibleProxy_.reset(new AccessibleProxy(self), IID_IAccessible);
				} catch(const std::bad_alloc&) {
					return E_OUTOFMEMORY;
				}
		//		accLib.notifyWinEvent(EVENT_OBJECT_CREATE, *this, OBJID_CLIENT, CHILDID_SELF);
			}
			if(accessibleProxy_ == nullptr)	// ???
				return E_FAIL;
			(acc = self.accessibleProxy_.get())->AddRef();
			return S_OK;
		}
#endif // !ASCENSION_NO_ACTIVE_ACCESSIBILITY

		/// Implementation of @c #beep method. The subclasses can override to customize the behavior.
		void TextViewer::doBeep() BOOST_NOEXCEPT {
			::MessageBeep(MB_OK);
		}

		/// Implements @c IDropTarget#DragEnter method.
		STDMETHODIMP TextViewer::DragEnter(IDataObject* data, DWORD keyState, POINTL location, DWORD* effect) {
			if(auto mouseInputStrategy = textArea()->mouseInputStrategy().lock()) {
				if(const auto dropTarget = mouseInputStrategy->handleDropTarget()) {
					widgetapi::detail::DragEventAdapter adapter(*dropTarget, widgetapi::Proxy<widgetapi::Widget>(*this));
					return adapter.adaptDragEnterEvent(data, keyState, location, effect);
				}
			}
			return S_OK;
		}

		/// Implements @c IDropTarget#DragLeave method.
		STDMETHODIMP TextViewer::DragLeave() {
			if(auto mouseInputStrategy = textArea()->mouseInputStrategy().lock()) {
				if(const auto dropTarget = mouseInputStrategy->handleDropTarget()) {
					widgetapi::detail::DragEventAdapter adapter(*dropTarget, widgetapi::Proxy<widgetapi::Widget>(*this));
					return adapter.adaptDragLeaveEvent();
				}
			}
			return S_OK;
		}

		/// Implements @c IDropTarget#DragOver method.
		STDMETHODIMP TextViewer::DragOver(DWORD keyState, POINTL location, DWORD* effect) {
			if(auto mouseInputStrategy = textArea()->mouseInputStrategy().lock()) {
				if(auto dropTarget = mouseInputStrategy->handleDropTarget()) {
					widgetapi::detail::DragEventAdapter adapter(*dropTarget, widgetapi::Proxy<widgetapi::Widget>(*this));
					const auto viewport(textArea()->viewport());
					viewport->lockScroll();
					const auto hr = adapter.adaptDragMoveEvent(keyState, location, effect);	// damn! IDropTargetHelper scrolls the view
					viewport->unlockScroll();
					return hr;
				}
			}
			return S_OK;
		}

		/// Implements @c IDropTarget#Drop method.
		STDMETHODIMP TextViewer::Drop(IDataObject* data, DWORD keyState, POINTL location, DWORD* effect) {
			if(auto mouseInputStrategy = textArea()->mouseInputStrategy().lock()) {
				if(auto dropTarget = mouseInputStrategy->handleDropTarget()) {
					widgetapi::detail::DragEventAdapter adapter(*dropTarget, widgetapi::Proxy<widgetapi::Widget>(*this));
					return adapter.adaptDropEvent(data, keyState, location, effect);
				}
			}
			return S_OK;
		}

		/// Hides the tool tip.
		void TextViewer::hideToolTip() {
			assert(::IsWindow(handle().get()));
			tipText_.erase();
			::KillTimer(handle().get(), TIMERID_CALLTIP);	// 念のため...
			::SendMessageW(toolTip_.get(), TTM_UPDATE, 0, 0L);
		}

		/// @internal Initializes the window of the viewer.
		void TextViewer::initializeNativeObjects() {
			// create the tooltip belongs to the window
			toolTip_.reset(::CreateWindowExW(
				WS_EX_TOOLWINDOW | WS_EX_TOPMOST, TOOLTIPS_CLASSW, 0, WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
				CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, handle().get(), nullptr,
				reinterpret_cast<HINSTANCE>(static_cast<HANDLE_PTR>(::GetWindowLongPtr(handle().get(), GWLP_HINSTANCE))), nullptr),
				&::DestroyWindow);
			if(toolTip_.get() != nullptr) {
				auto ti(win32::makeZeroSize<TOOLINFOW>());
				RECT margins = {1, 1, 1, 1};
				ti.hwnd = handle().get();
				ti.lpszText = LPSTR_TEXTCALLBACKW;
				ti.uFlags = TTF_SUBCLASS;
				ti.uId = 1;
				::SetRect(&ti.rect, 0, 0, 0, 0);
				::SendMessageW(toolTip_.get(), TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&ti));
				::SendMessageW(toolTip_.get(), TTM_SETDELAYTIME, TTDT_AUTOPOP, 30000);	// 30 秒間 (根拠なし) 表示されるように
//				::SendMessageW(toolTip_.get(), TTM_SETDELAYTIME, TTDT_INITIAL, 1500);
				::SendMessageW(toolTip_.get(), TTM_SETMARGIN, 0, reinterpret_cast<LPARAM>(&margins));
				::SendMessageW(toolTip_.get(), TTM_ACTIVATE, true, 0L);
			}

			::RegisterDragDrop(handle().get(), this);
		}

		/// @see WM_CAPTURECHANGED
		void TextViewer::onCaptureChanged(const win32::Handle<HWND>&, bool& consumed) {
			auto mouseInputStrategy = textArea()->mouseInputStrategy().lock();
			if(consumed = (mouseInputStrategy.get() != nullptr))
				mouseInputStrategy->mouseInputTargetUnlocked();
		}

		namespace {
			// identifiers of GUI commands
			enum {
				WM_REDO	= WM_APP + 1,		// Undo
				WM_SELECTALL,				// Select All
				ID_DISPLAYSHAPINGCONTROLS,	// Show Unicode control characters
				ID_RTLREADING,				// Right to left Reading order
				ID_TOGGLEIMESTATUS,			// Open/Close IME
				ID_TOGGLESOFTKEYBOARD,		// Open/Close soft keyboard
				ID_RECONVERT,				// Reconvert

				ID_INSERT_LRM,		// LRM (Left-to-right mark)
				ID_INSERT_RLM,		// RLM (Right-to-left mark)
				ID_INSERT_ZWJ,		// ZWJ (Zero width joiner)
				ID_INSERT_ZWNJ,		// ZWNJ (Zero width non-joiner)
				ID_INSERT_LRE,		// LRE (Left-to-right embedding)
				ID_INSERT_RLE,		// RLE (Right-to-left embedding)
				ID_INSERT_LRO,		// LRO (Left-to-right override)
				ID_INSERT_RLO,		// RLO (Right-to-left override)
				ID_INSERT_PDF,		// PDF (Pop directional formatting)
				ID_INSERT_WJ,		// WJ (Word Joiner)
				ID_INSERT_NADS,		// NADS (National digit shapes)	<- the following six are deprecated code points (Unicode 4.0)
				ID_INSERT_NODS,		// NODS (Nominal digit shapes)
				ID_INSERT_ASS,		// ASS (Activate symmetric swapping)
				ID_INSERT_ISS,		// ISS (Inhibit symmetric swapping)
				ID_INSERT_AAFS,		// AAFS (Activate Arabic form shaping)
				ID_INSERT_IAFS,		// IAFS (Inhibit Arabic form shaping)
				ID_INSERT_RS,		// RS (Record Separator)
				ID_INSERT_US,		// US (Unit Separator)
				ID_INSERT_IAA,		// Interlinear Annotation Anchor
				ID_INSERT_IAS,		// Interlinear Annotation Separator
				ID_INSERT_IAT,		// Interlinear Annotation Terminator

				ID_INSERT_U0020,	// U+0020 (Space)
				ID_INSERT_NBSP,		// NBSP (No-Break Space)
				ID_INSERT_U1680,	// U+1680 (Ogham Space Mark)
				ID_INSERT_MVS,		// MVS (Mongolian Vowel Separator)
				ID_INSERT_U2000,	// U+2000 (En Quad)
				ID_INSERT_U2001,	// U+2001 (Em Quad)
				ID_INSERT_U2002,	// U+2002 (En Space)
				ID_INSERT_U2003,	// U+2003 (Em Space)
				ID_INSERT_U2004,	// U+2004 (Three-Per-Em Space)
				ID_INSERT_U2005,	// U+2005 (Four-Per-Em Space)
				ID_INSERT_U2006,	// U+2006 (Six-Per-Em Space)
				ID_INSERT_U2007,	// U+2007 (Figure Space)
				ID_INSERT_U2008,	// U+2008 (Punctuation Space)
				ID_INSERT_U2009,	// U+2009 (Thin Space)
				ID_INSERT_U200A,	// U+200A (Hair Space)
				ID_INSERT_ZWSP,		// ZWSP (Zero Width Space)
				ID_INSERT_NNBSP,	// NNSBP (Narrwow No-Break Space)
				ID_INSERT_MMSP,		// MMSP (Medium Mathematical Space)
				ID_INSERT_U3000,	// U+3000 (Ideographic Space)
				ID_INSERT_NEL,		// NEL (Next Line)
				ID_INSERT_LS,		// LS (Line Separator)
				ID_INSERT_PS,		// PS (Paragraph Separator)

				ID_INVOKE_HYPERLINK	// Open <hyperlink>
			};
		}

		/// @see Window#onCommand
		void TextViewer::onCommand(WORD id, WORD, const win32::Handle<HWND>&, bool& consumed) {
			using namespace ascension::texteditor::commands;
			switch(id) {
			case WM_UNDO:	// "Undo"
				texteditor::commands::UndoCommand(*this, false)();
				break;
			case WM_REDO:	// "Redo"
				texteditor::commands::UndoCommand(*this, true)();
				break;
			case WM_CUT:	// "Cut"
				if(auto caret = tryCaret(*this))
					cutSelection(*caret, true);
				break;
			case WM_COPY:	// "Copy"
				if(auto caret = tryCaret(*this))
					copySelection(*caret, true);
				break;
			case WM_PASTE:	// "Paste"
				texteditor::commands::PasteCommand(*this, false)();
				break;
			case WM_CLEAR:	// "Delete"
				texteditor::commands::CharacterDeletionCommand(*this, Direction::forward())();
				break;
			case WM_SELECTALL:	// "Select All"
				texteditor::commands::EntireDocumentSelectionCreationCommand(*this)();
				break;
			case ID_RTLREADING:	// "Right to left Reading order"
				// TODO: Not implemented.
//				utils::toggleOrientation(*this);
				break;
			case ID_DISPLAYSHAPINGCONTROLS:	// "Show Unicode control characters"
				// TODO: Not implemented.
//				textArea()->textRenderer().displayShapingControls(!textArea()->textRenderer().displaysShapingControls());
				break;
			case ID_INSERT_LRM:		texteditor::commands::CharacterInputCommand(*this, 0x200eu)();	break;
			case ID_INSERT_RLM:		texteditor::commands::CharacterInputCommand(*this, 0x200fu)();	break;
			case ID_INSERT_ZWJ:		texteditor::commands::CharacterInputCommand(*this, 0x200du)();	break;
			case ID_INSERT_ZWNJ:	texteditor::commands::CharacterInputCommand(*this, 0x200cu)();	break;
			case ID_INSERT_LRE:		texteditor::commands::CharacterInputCommand(*this, 0x202au)();	break;
			case ID_INSERT_RLE:		texteditor::commands::CharacterInputCommand(*this, 0x202bu)();	break;
			case ID_INSERT_LRO:		texteditor::commands::CharacterInputCommand(*this, 0x202du)();	break;
			case ID_INSERT_RLO:		texteditor::commands::CharacterInputCommand(*this, 0x202eu)();	break;
			case ID_INSERT_PDF:		texteditor::commands::CharacterInputCommand(*this, 0x202cu)();	break;
			case ID_INSERT_WJ:		texteditor::commands::CharacterInputCommand(*this, 0x2060u)();	break;
			case ID_INSERT_NADS:	texteditor::commands::CharacterInputCommand(*this, 0x206eu)();	break;
			case ID_INSERT_NODS:	texteditor::commands::CharacterInputCommand(*this, 0x206fu)();	break;
			case ID_INSERT_ASS:		texteditor::commands::CharacterInputCommand(*this, 0x206bu)();	break;
			case ID_INSERT_ISS:		texteditor::commands::CharacterInputCommand(*this, 0x206au)();	break;
			case ID_INSERT_AAFS:	texteditor::commands::CharacterInputCommand(*this, 0x206du)();	break;
			case ID_INSERT_IAFS:	texteditor::commands::CharacterInputCommand(*this, 0x206cu)();	break;
			case ID_INSERT_RS:		texteditor::commands::CharacterInputCommand(*this, 0x001eu)();	break;
			case ID_INSERT_US:		texteditor::commands::CharacterInputCommand(*this, 0x001fu)();	break;
			case ID_INSERT_IAA:		texteditor::commands::CharacterInputCommand(*this, 0xfff9u)();	break;
			case ID_INSERT_IAT:		texteditor::commands::CharacterInputCommand(*this, 0xfffau)();	break;
			case ID_INSERT_IAS:		texteditor::commands::CharacterInputCommand(*this, 0xfffbu)();	break;
			case ID_INSERT_U0020:	texteditor::commands::CharacterInputCommand(*this, 0x0020u)();	break;
			case ID_INSERT_NBSP:	texteditor::commands::CharacterInputCommand(*this, 0x00a0u)();	break;
			case ID_INSERT_U1680:	texteditor::commands::CharacterInputCommand(*this, 0x1680u)();	break;
			case ID_INSERT_MVS:		texteditor::commands::CharacterInputCommand(*this, 0x180eu)();	break;
			case ID_INSERT_U2000:	texteditor::commands::CharacterInputCommand(*this, 0x2000u)();	break;
			case ID_INSERT_U2001:	texteditor::commands::CharacterInputCommand(*this, 0x2001u)();	break;
			case ID_INSERT_U2002:	texteditor::commands::CharacterInputCommand(*this, 0x2002u)();	break;
			case ID_INSERT_U2003:	texteditor::commands::CharacterInputCommand(*this, 0x2003u)();	break;
			case ID_INSERT_U2004:	texteditor::commands::CharacterInputCommand(*this, 0x2004u)();	break;
			case ID_INSERT_U2005:	texteditor::commands::CharacterInputCommand(*this, 0x2005u)();	break;
			case ID_INSERT_U2006:	texteditor::commands::CharacterInputCommand(*this, 0x2006u)();	break;
			case ID_INSERT_U2007:	texteditor::commands::CharacterInputCommand(*this, 0x2007u)();	break;
			case ID_INSERT_U2008:	texteditor::commands::CharacterInputCommand(*this, 0x2008u)();	break;
			case ID_INSERT_U2009:	texteditor::commands::CharacterInputCommand(*this, 0x2009u)();	break;
			case ID_INSERT_U200A:	texteditor::commands::CharacterInputCommand(*this, 0x200au)();	break;
			case ID_INSERT_ZWSP:	texteditor::commands::CharacterInputCommand(*this, 0x200bu)();	break;
			case ID_INSERT_NNBSP:	texteditor::commands::CharacterInputCommand(*this, 0x202fu)();	break;
			case ID_INSERT_MMSP:	texteditor::commands::CharacterInputCommand(*this, 0x205fu)();	break;
			case ID_INSERT_U3000:	texteditor::commands::CharacterInputCommand(*this, 0x3000u)();	break;
			case ID_INSERT_NEL:		texteditor::commands::CharacterInputCommand(*this, text::NEXT_LINE)();	break;
			case ID_INSERT_LS:		texteditor::commands::CharacterInputCommand(*this, text::LINE_SEPARATOR)();	break;
			case ID_INSERT_PS:		texteditor::commands::CharacterInputCommand(*this, text::PARAGRAPH_SEPARATOR)();	break;
			case ID_TOGGLEIMESTATUS:	// "Open IME" / "Close IME"
				texteditor::commands::InputMethodOpenStatusToggleCommand(*this)();
				break;
			case ID_TOGGLESOFTKEYBOARD:	// "Open soft keyboard" / "Close soft keyboard"
				texteditor::commands::InputMethodSoftKeyboardModeToggleCommand(*this)();
				break;
			case ID_RECONVERT:	// "Reconvert"
				texteditor::commands::ReconversionCommand(*this)();
				break;
			case ID_INVOKE_HYPERLINK:	// "Open <hyperlink>"
				if(auto caret = tryCaret(*this)) {
					if(const auto link = utils::getPointedHyperlink(*this, insertionPosition(*caret)))
						link->invoke();
				}
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
			::RevokeDragDrop(handle().get());
			static_cast<TextViewerComponent*>(textArea().get())->uninstall(*this);

			// destroy children
			toolTip_.reset();

#ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY
			if(accessibleProxy_.get() != nullptr) {
				accessibleProxy_->dispose();
				accessibleProxy_.reset();
			}
//			if(accLib.isAvailable())
//				accLib.notifyWinEvent(EVENT_OBJECT_DESTROY, *this, OBJID_CLIENT, CHILDID_SELF);
#endif // !ASCENSION_NO_ACTIVE_ACCESSIBILITY
			consumed = true;
		}

		/// @see WM_ERASEGKGND
		void TextViewer::onEraseBkgnd(const win32::Handle<HDC>&, bool& consumed) {
			consumed = false;
		}

		/// @see WM_GETFONT
		const win32::Handle<HFONT> TextViewer::onGetFont() const {
			return textArea()->textRenderer()->defaultFont()->native();
		}

		/// @see WM_HSCROLL
		void TextViewer::onHScroll(UINT sbCode, UINT, const win32::Handle<HWND>&) {
			const auto viewport(textArea()->viewport());
			switch(sbCode) {
				case SB_LINELEFT:
					viewport->scroll(graphics::PhysicalTwoAxes<graphics::font::TextViewport::SignedScrollOffset>(-1, 0));
					break;
				case SB_LINERIGHT:
					viewport->scroll(graphics::PhysicalTwoAxes<graphics::font::TextViewport::SignedScrollOffset>(+1, 0));
					break;
				case SB_PAGELEFT:	// a left page
					graphics::font::scrollPage(*viewport, graphics::PhysicalTwoAxes<graphics::font::TextViewport::SignedScrollOffset>(graphics::_x = -1, graphics::_y = 0));
					break;
				case SB_PAGERIGHT:	// a right page
					graphics::font::scrollPage(*viewport, graphics::PhysicalTwoAxes<graphics::font::TextViewport::SignedScrollOffset>(graphics::_x = +1, graphics::_y = 0));
					break;
				case SB_LEFT:		// leftmost
					viewport->scrollTo(
						graphics::PhysicalTwoAxes<boost::optional<graphics::font::TextViewport::ScrollOffset>>(
							graphics::_x = boost::make_optional(*boost::const_begin(graphics::font::scrollableRange<0>(*viewport)))));
					break;
				case SB_RIGHT:		// rightmost
					viewport->scrollTo(
						graphics::PhysicalTwoAxes<boost::optional<graphics::font::TextViewport::ScrollOffset>>(
							graphics::_x = boost::make_optional(*boost::const_end(graphics::font::scrollableRange<0>(*viewport)))));
					break;
				case SB_THUMBTRACK: {	// by drag or wheel
					auto si(win32::makeZeroSize<SCROLLINFO>());
					si.fMask = SIF_TRACKPOS;
					if(win32::boole(::GetScrollInfo(handle().get(), SB_HORZ, &si)))
						viewport->scrollTo(
							graphics::PhysicalTwoAxes<boost::optional<graphics::font::TextViewport::ScrollOffset>>(
								graphics::_x = boost::make_optional<graphics::font::TextViewport::ScrollOffset>(si.nTrackPos)));
					break;
				}
			}
//			consumed = false;
		}

		namespace {
			// replaces single "&" with "&&".
			template<typename CharType>
			std::basic_string<CharType> escapeAmpersands(const std::basic_string<CharType>& s) {
				static const auto& ct = std::use_facet<std::ctype<CharType>>(std::locale::classic());
				std::basic_string<CharType> result;
				result.reserve(s.length() * 2);
				for(std::basic_string<CharType>::size_type i = 0; i < s.length(); ++i) {
					result += s[i];
					if(s[i] == ct.widen('&'))
						result += s[i];
				}
				return result;
			}
		} // namespace 0@

		/// @see WM_NCCREATE
		bool TextViewer::onNcCreate(CREATESTRUCTW&) {
			const LONG s = ::GetWindowLongW(handle().get(), GWL_EXSTYLE);
			::SetWindowLongW(handle().get(), GWL_EXSTYLE, s & ~WS_EX_LAYOUTRTL);
			return true;
		}

		/// @see WM_NOTIFY
		void TextViewer::onNotify(int, NMHDR& nmhdr, bool& consumed) {
			// tooltip text
			if(nmhdr.hwndFrom == toolTip_.get() && nmhdr.code == TTN_GETDISPINFOW) {
				::SendMessageW(toolTip_.get(), TTM_SETMAXTIPWIDTH, 0, 1000);	// make line breaks effective
				reinterpret_cast<LPNMTTDISPINFOW>(&nmhdr)->lpszText = const_cast<WCHAR*>(tipText_.c_str());
				consumed = true;
			} else
				consumed = false;
		}

		/// @see WM_SETCURSOR
		void TextViewer::onSetCursor(const win32::Handle<HWND>&, UINT, UINT, bool& consumed) {
			restoreHiddenCursor();
			auto mouseInputStrategy(textArea()->mouseInputStrategy().lock());
			if(consumed = (mouseInputStrategy.get() != nullptr))
				mouseInputStrategy->showCursor(widgetapi::mapFromGlobal(*this, widgetapi::Cursor::position()));
		}

		/// @see WM_STYLECHANGED
		void TextViewer::onStyleChanged(int type, const STYLESTRUCT& style) {
			// see graphics.font.WidgetThemedTextRenderer
//			if(type == GWL_EXSTYLE
//					&& (((style.styleOld ^ style.styleNew) & (WS_EX_RIGHT | WS_EX_RTLREADING)) != 0)) {
//				// synchronize the reading direction with the window's style
//				// (ignore the alignment)
//				Configuration c(configuration());
//				c.readingDirection = ((style.styleNew & WS_EX_RTLREADING) != 0) ? presentation::RIGHT_TO_LEFT : presentation::LEFT_TO_RIGHT;
//				setConfiguration(&c, nullptr, false);
//			}
		}

		/// @see WM_STYLECHANGING
		void TextViewer::onStyleChanging(int type, STYLESTRUCT& style) {
			if(type == GWL_EXSTYLE)
				style.styleNew &= ~WS_EX_LAYOUTRTL;	// このウィンドウの DC のレイアウトは常に LTR でなければならぬ
		}

		/// @see WM_SYSCOLORCHANGE
		void TextViewer::onSysColorChange() {
//			if(this == originalView_)
//				presentation_.updateSystemColors();
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
				::KillTimer(handle().get(), TIMERID_CALLTIP);
				::SendMessageW(toolTip_.get(), TTM_UPDATE, 0, 0L);
			}
		}

		/// @see Window#onVScroll
		void TextViewer::onVScroll(UINT sbCode, UINT, const win32::Handle<HWND>&) {
			const auto viewport(textArea()->viewport());
			switch(sbCode) {
				case SB_LINEUP:
					viewport->scroll(graphics::PhysicalTwoAxes<graphics::font::TextViewport::SignedScrollOffset>(0, -1));
					break;
				case SB_LINEDOWN:
					viewport->scroll(graphics::PhysicalTwoAxes<graphics::font::TextViewport::SignedScrollOffset>(0, +1));
					break;
				case SB_PAGEUP:
					graphics::font::scrollPage(*viewport, graphics::PhysicalTwoAxes<graphics::font::TextViewport::SignedScrollOffset>(graphics::geometry::_x = 0, graphics::geometry::_y = -1));
					break;
				case SB_PAGEDOWN:
					graphics::font::scrollPage(*viewport, graphics::PhysicalTwoAxes<graphics::font::TextViewport::SignedScrollOffset>(graphics::geometry::_x = 0, graphics::geometry::_y = +1));
					break;
				case SB_TOP:
					viewport->scrollTo(
						graphics::PhysicalTwoAxes<boost::optional<graphics::font::TextViewport::ScrollOffset>>(
							graphics::_y = boost::make_optional(*boost::const_begin(graphics::font::scrollableRange<1>(*viewport)))));
					break;
				case SB_BOTTOM:
					viewport->scrollTo(
						graphics::PhysicalTwoAxes<boost::optional<graphics::font::TextViewport::ScrollOffset>>(
							graphics::_y = boost::make_optional(*boost::const_end(graphics::font::scrollableRange<1>(*viewport)))));
					break;
				case SB_THUMBTRACK: {	// by drag or wheel
					auto si(win32::makeZeroSize<SCROLLINFO>());
					si.fMask = SIF_TRACKPOS;
					if(win32::boole(::GetScrollInfo(handle().get(), SB_VERT, &si)))
						viewport->scrollTo(
							graphics::PhysicalTwoAxes<boost::optional<graphics::font::TextViewport::ScrollOffset>>(
								graphics::_y = boost::make_optional<graphics::font::TextViewport::ScrollOffset>(si.nTrackPos)));
					break;
				}
			}
		}

		namespace {
			inline widgetapi::event::KeyInput makeKeyInput(WPARAM wp, LPARAM lp) {
				std::tuple<widgetapi::event::KeyboardModifier> modifiers;
				return widgetapi::event::KeyInput(wp, win32::makeKeyboardModifiers());
			}

			void nativeMessage(TextViewer& self, UINT message, WPARAM wp, LPARAM lp, MSG& out) BOOST_NOEXCEPT {
				out.hwnd = self.handle().get();
				out.message = message;
				out.wParam = wp;
				out.lParam = lp;
				out.time = ::GetMessageTime();
				const auto p(::GetMessagePos());
				out.pt.x = GET_X_LPARAM(p);
				out.pt.y = GET_Y_LPARAM(p);

			}
		}

		/// @see win32#Window#processMessage
		LRESULT TextViewer::processMessage(UINT message, WPARAM wp, LPARAM lp, bool& consumed) {
#ifndef WM_UNICHAR
			static const UINT WM_UNICHAR = 0x109;
#endif
#ifndef WM_XBUTTONDOWN
			static const UINT WM_XBUTTONDOWN = 0x20b;
			static const UINT WM_XBUTTONUP = 0x20c;
			static const UINT WM_XBUTTONDBLCLK = 0x20d;
			static const int XBUTTON1 = 0x1, XBUTTON2 = 0x2;
#	define GET_KEYSTATE_WPARAM(wp) (LOWORD(wp))
#	define GET_XBUTTON_WPARAM(wp) (HIWORD(wp))
#endif
#ifndef WM_MOUSEHWHEEL
			static const UINT WM_MOUSEHWHEEL = 0x20e;
#endif
#ifndef WM_THEMECHANGED
			static const UINT WM_THEMECHANGED = 0x31a;
#endif

			using namespace ascension::texteditor::commands;

			switch(message) {
#ifdef ASCENSION_HANDLE_STANDARD_EDIT_CONTROL_MESSAGES
				case WM_CLEAR:
					if(auto caret = tryCaret(*this)) {
						if(::GetKeyState(VK_SHIFT) < 0)
							cutSelection(*caret, true);
						else
							texteditor::commands::CharacterDeletionCommand(*this, Direction::forward())();
					}
					consumed = true;
					return 0L;
				case WM_COPY:
					if(auto caret = tryCaret(*this)) {
						copySelection(*caret, true);
						consumed = true;
					}
					return 0L;
				case WM_CUT:
					if(auto caret = tryCaret(*this)) {
						cutSelection(*caret, true);
						consumed = true;
					}
					return 0L;
#endif // ASCENSION_HANDLE_STANDARD_EDIT_CONTROL_MESSAGES
#ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY
				case WM_GETOBJECT:
					if(lp == OBJID_CLIENT) {
						win32::com::SmartPointer<IAccessible> acc;
						if(SUCCEEDED(accessibleObject(*acc.initialize())) && accLib.isAvailable())
							return accLib.lresultFromObject(IID_IAccessible, wp, acc.get());
					} else if(lp == OBJID_WINDOW) {
					}
					return 0;
#endif // !ASCENSION_NO_ACTIVE_ACCESSIBILITY
				case WM_GETTEXT: {
					std::basic_ostringstream<Char> s;
					kernel::writeDocumentToStream(s, *document(), document()->region(), text::Newline::CARRIAGE_RETURN_FOLLOWED_BY_LINE_FEED);
					consumed = true;
					return reinterpret_cast<LRESULT>(s.str().c_str());
				}
				case WM_GETTEXTLENGTH:
					// ウィンドウ関係だし改行は CRLF でいいか。Newline.USE_INTRINSIC_VALUE だと遅いし
					consumed = true;
					return document()->length(text::Newline::CARRIAGE_RETURN_FOLLOWED_BY_LINE_FEED);
//				case WM_NCPAINT:
//					return 0;
#ifdef ASCENSION_HANDLE_STANDARD_EDIT_CONTROL_MESSAGES
				case WM_PASTE:
					texteditor::commands::PasteCommand(*this, false)();
					consumed = true;
					return 0L;
#endif // ASCENSION_HANDLE_STANDARD_EDIT_CONTROL_MESSAGES
				case WM_SETTEXT:
					if(auto caret = tryCaret(*this)) {
						texteditor::commands::EntireDocumentSelectionCreationCommand(*this)();
						caret->replaceSelection(String(reinterpret_cast<const Char*>(lp)), false);
						consumed = true;
					}
					return 0L;
#ifdef ASCENSION_HANDLE_STANDARD_EDIT_CONTROL_MESSAGES
				case WM_UNDO:
					texteditor::commands::UndoCommand(*this, false)();
					consumed = true;
					return 0L;
#endif // ASCENSION_HANDLE_STANDARD_EDIT_CONTROL_MESSAGES
					// dispatch message into handler
				case WM_CAPTURECHANGED:
					onCaptureChanged(win32::Handle<HWND>(reinterpret_cast<HWND>(lp)), consumed);
					return consumed ? 0 : 1;
				case WM_CHAR:
				case WM_SYSCHAR:
				case WM_UNICHAR:
					if(message == WM_UNICHAR && wp == UNICODE_NOCHAR)
						return TRUE;
					else if(auto caret = tryCaret(*this)) {
						consumed = texteditor::commands::CharacterInputCommand(*this, wp)() != 0;
						// vanish the cursor when the GUI user began typing
						if(consumed) {
							// ignore if the cursor is not over a window belongs to the same thread
							HWND pointedWindow = ::WindowFromPoint(toNative<POINT>(widgetapi::Cursor::position()));
							if(pointedWindow != nullptr
									&& ::GetWindowThreadProcessId(pointedWindow, nullptr) == ::GetWindowThreadProcessId(handle().get(), nullptr))
								hideCursor();
						}
					} else
						break;
					return consumed ? 0 : 1;
				case WM_COMMAND:
					onCommand(LOWORD(wp), HIWORD(wp), win32::Handle<HWND>(reinterpret_cast<HWND>(lp)), consumed);
					return consumed ? 0 : 1;
				case WM_CONTEXTMENU: {
					const widgetapi::event::LocatedUserInput input(win32::makeMouseLocation<graphics::Point>(lp), widgetapi::event::MouseButtons(), win32::makeKeyboardModifiers());
					MSG native;
					nativeMessage(*this, message, wp, lp, native);
					showContextMenu(input, &native);
					return (consumed = true), 0;
				}
				case WM_DESTROY:
					onDestroy(consumed);
					return consumed ? 0 : 1;
				case WM_ERASEBKGND:
					onEraseBkgnd(win32::Handle<HDC>(reinterpret_cast<HDC>(wp)), consumed);
					return consumed ? TRUE : FALSE;
				case WM_GETFONT:
					return (consumed = true), reinterpret_cast<LRESULT>(onGetFont().get());
				case WM_HSCROLL:
					return (consumed = true), onHScroll(LOWORD(wp), HIWORD(wp), win32::borrowed(reinterpret_cast<HWND>(lp))), 0;
//				case WM_INPUTLANGCHANGE:
//					if(auto caret = tryCaret(*this))
//						return static_cast<detail::InputEventHandler&>(*caret).handleInputEvent(message, wp, lp, consumed);	// $friendly-access
//					break;
				case WM_IME_COMPOSITION: {
					MSG native;
					nativeMessage(*this, message, wp, lp, native);
					if((lp & GCS_RESULTSTR) != 0) {	// completed
						if(auto im = win32::inputMethod(*this)) {
							auto nbytes = ::ImmGetCompositionStringW(im.get(), GCS_RESULTSTR, nullptr, 0);
							if(nbytes > 0) {
								const std::size_t length = nbytes / sizeof(WCHAR);
								std::unique_ptr<WCHAR[]> buffer(new WCHAR[length]);
								nbytes = ::ImmGetCompositionStringW(im.get(), GCS_RESULTSTR, buffer.get(), nbytes);
								if(nbytes > 0) {
									auto e(widgetapi::event::ConstantInputMethodEvent::createCompletedInstance(&native, String(win32::wideString<const Char>(buffer.get()))));
									handleInputMethodEvent(e);
									if(consumed = e.isConsumed())
										return 0L;	// block WM_CHARs
								}
							}
						}
					} else if((lp & (GCS_COMPSTR | CS_INSERTCHAR)) != 0)	// changed
						;
					break;
				}
				case WM_IME_ENDCOMPOSITION: {
					MSG native;
					nativeMessage(*this, message, wp, lp, native);
					handleInputMethodEvent(widgetapi::event::ConstantInputMethodEvent::createCanceledInstance(&native));
					break;
				}
				case WM_IME_REQUEST:
					break;
				case WM_IME_STARTCOMPOSITION: {
					MSG native;
					nativeMessage(*this, message, wp, lp, native);
					handleInputMethodEvent(widgetapi::event::ConstantInputMethodEvent::createStartedInstance(&native));
					break;
				}
				case WM_KEYDOWN:
				case WM_SYSKEYDOWN:
					return (consumed = true), keyPressed(makeKeyInput(wp, lp)), 0;
				case WM_KEYUP:
				case WM_SYSKEYUP:
					return (consumed = true), keyReleased(makeKeyInput(wp, lp)), 0;
				case WM_KILLFOCUS:
					return (consumed = true), focusAboutToBeLost(widgetapi::event::Event()), 0;
				case WM_LBUTTONDBLCLK:
					return (consumed = true), fireMouseDoubleClicked(win32::makeMouseButtonInput(widgetapi::event::BUTTON1_DOWN, wp, lp)), 0;
				case WM_LBUTTONDOWN:
					return (consumed = true), fireMousePressed(win32::makeMouseButtonInput(widgetapi::event::BUTTON1_DOWN, wp, lp)), 0;
				case WM_LBUTTONUP:
					return (consumed = true), fireMouseReleased(win32::makeMouseButtonInput(widgetapi::event::BUTTON1_DOWN, wp, lp)), 0;
				case WM_MBUTTONDBLCLK:
					return (consumed = true), fireMouseDoubleClicked(win32::makeMouseButtonInput(widgetapi::event::BUTTON2_DOWN, wp, lp)), 0;
				case WM_MBUTTONDOWN:
					return (consumed = true), fireMousePressed(win32::makeMouseButtonInput(widgetapi::event::BUTTON2_DOWN, wp, lp)), 0;
				case WM_MBUTTONUP:
					return (consumed = true), fireMouseReleased(win32::makeMouseButtonInput(widgetapi::event::BUTTON2_DOWN, wp, lp)), 0;
				case WM_MOUSEMOVE:
					return (consumed = true), fireMouseMoved(win32::makeLocatedUserInput(wp, win32::makeMouseLocation<graphics::Point>(lp))), 0;
				case WM_MOUSEWHEEL:
				case WM_MOUSEHWHEEL:
					return
						(consumed = true),
						fireMouseWheelChanged(widgetapi::event::MouseWheelInput(
							widgetapi::mapFromGlobal(*this, win32::makeMouseLocation<graphics::Point>(lp)),
							fromNative<widgetapi::event::MouseButtons>(GET_KEYSTATE_WPARAM(wp)),
							fromNative<widgetapi::event::KeyboardModifiers>(GET_KEYSTATE_WPARAM(wp)),
							graphics::geometry::BasicDimension<double>(
								graphics::geometry::_dx = (message == WM_MOUSEHWHEEL) ? GET_WHEEL_DELTA_WPARAM(wp) : 0,
								graphics::geometry::_dy = (message == WM_MOUSEWHEEL) ? GET_WHEEL_DELTA_WPARAM(wp) : 0))),
						0;
				case WM_NCCREATE:
					return (consumed = true), onNcCreate(*reinterpret_cast<CREATESTRUCTW*>(lp));
				case WM_NOTIFY:
					return onNotify(static_cast<int>(wp), *reinterpret_cast<NMHDR*>(lp), consumed), 0;
				case WM_PAINT: {
					PAINTSTRUCT ps;
					::BeginPaint(handle().get(), &ps);
					consumed = true;
					const auto dc(win32::borrowed(ps.hdc));
					paint(graphics::PaintContext(graphics::RenderingContext2D(dc), fromNative<graphics::Rectangle>(ps.rcPaint)));
					::EndPaint(handle().get(), &ps);
					return 0;
				}
				case WM_RBUTTONDBLCLK:
					return (consumed = true), fireMouseDoubleClicked(win32::makeMouseButtonInput(widgetapi::event::BUTTON3_DOWN, wp, lp)), 0;
				case WM_RBUTTONDOWN:
					return (consumed = true), fireMousePressed(win32::makeMouseButtonInput(widgetapi::event::BUTTON3_DOWN, wp, lp)), 0;
				case WM_RBUTTONUP:
					return (consumed = true), fireMouseReleased(win32::makeMouseButtonInput(widgetapi::event::BUTTON3_DOWN, wp, lp)), 0;
				case WM_SETCURSOR:
					onSetCursor(win32::borrowed(reinterpret_cast<HWND>(wp)), LOWORD(lp), HIWORD(lp), consumed);
					return consumed ? TRUE : FALSE;
				case WM_SETFOCUS:
					return (consumed = true), focusGained(widgetapi::event::Event()), 0;
				case WM_SIZE:
					return (consumed = true), resized(graphics::Dimension(graphics::geometry::_dx = LOWORD(lp), graphics::geometry::_dy = HIWORD(lp))), 0;
				case WM_STYLECHANGED:
					return (consumed = true), onStyleChanged(static_cast<int>(wp), *reinterpret_cast<STYLESTRUCT*>(lp)), 0;
				case WM_STYLECHANGING:
					return (consumed = true), onStyleChanging(static_cast<int>(wp), *reinterpret_cast<STYLESTRUCT*>(lp)), 0;
				case WM_SYSCOLORCHANGE:
					return (consumed = true), onSysColorChange(), 0;
				case WM_THEMECHANGED:
					return (consumed = true), onThemeChanged(), 0;
				case WM_TIMER:
					return (consumed = true), onTimer(static_cast<UINT_PTR>(wp), reinterpret_cast<TIMERPROC>(lp)), 0;
				case WM_VSCROLL:
					return (consumed = true), onVScroll(LOWORD(wp), HIWORD(wp), win32::borrowed(reinterpret_cast<HWND>(lp))), 0;
				case WM_XBUTTONDBLCLK:
					return (consumed = true), fireMouseDoubleClicked(win32::makeMouseButtonInput((GET_XBUTTON_WPARAM(wp) == XBUTTON1) ? widgetapi::event::BUTTON4_DOWN : widgetapi::event::BUTTON5_DOWN, GET_KEYSTATE_WPARAM(wp), lp)), 0;
				case WM_XBUTTONDOWN:
					return (consumed = true), fireMousePressed(win32::makeMouseButtonInput((GET_XBUTTON_WPARAM(wp) == XBUTTON1) ? widgetapi::event::BUTTON4_DOWN : widgetapi::event::BUTTON5_DOWN, GET_KEYSTATE_WPARAM(wp), lp)), 0;
				case WM_XBUTTONUP:
					return (consumed = true), fireMouseReleased(win32::makeMouseButtonInput((GET_XBUTTON_WPARAM(wp) == XBUTTON1) ? widgetapi::event::BUTTON4_DOWN : widgetapi::event::BUTTON5_DOWN, GET_KEYSTATE_WPARAM(wp), lp)), 0;
			}

			return win32::CustomControl::processMessage(message, wp, lp, consumed);
		}

		void TextViewer::provideClassInformation(win32::CustomControl::ClassInformation& classInformation) const {
			classInformation.style = CS_BYTEALIGNCLIENT | CS_BYTEALIGNWINDOW | CS_DBLCLKS;
			classInformation.background = COLOR_WINDOW;
			classInformation.cursor = MAKEINTRESOURCEW(32513);	// IDC_IBEAM
		}

		std::basic_string<WCHAR> TextViewer::provideClassName() const {
			return L"ascension.TextViewer";
		}

		/// @see Widget#showContextMenu
		void TextViewer::showContextMenu(const widgetapi::event::LocatedUserInput& input, const void* nativeEvent) {
			auto caret(tryCaret(*this));
			if(caret.get() == nullptr)
				return;
			const kernel::Document& doc = *document();
			const bool hasSelection = !isSelectionEmpty(*caret);
			const bool readOnly = doc.isReadOnly();
			const bool japanese = PRIMARYLANGID(win32::userDefaultUILanguage()) == LANG_JAPANESE;

			static auto toplevelPopup(win32::makeHandle(::CreatePopupMenu(), &::DestroyMenu));
			if(::GetMenuItemCount(toplevelPopup.get()) == 0) {	// first initialization
				// under "Insert Unicode control character"
				static const std::pair<UINT, const std::basic_string<WCHAR>> insertUnicodeControlCharacterItems[] = {
					std::make_pair(ID_INSERT_LRM, L"LRM\t&Left-To-Right Mark"),
					std::make_pair(ID_INSERT_RLM, L"RLM\t&Right-To-Left Mark"),
					std::make_pair(ID_INSERT_ZWJ, L"ZWJ\t&Zero Width Joiner"),
					std::make_pair(ID_INSERT_ZWNJ, L"ZWNJ\tZero Width &Non-Joiner"),
					std::make_pair(ID_INSERT_LRE, L"LRE\tLeft-To-Right &Embedding"),
					std::make_pair(ID_INSERT_RLE, L"RLE\tRight-To-Left E&mbedding"),
					std::make_pair(ID_INSERT_LRO, L"LRO\tLeft-To-Right &Override"),
					std::make_pair(ID_INSERT_RLO, L"RLO\tRight-To-Left O&verride"),
					std::make_pair(ID_INSERT_PDF, L"PDF\t&Pop Directional Formatting"),
					std::make_pair(ID_INSERT_WJ, L"WJ\t&Word Joiner"),
					std::make_pair(ID_INSERT_NADS, L"NADS\tN&ational Digit Shapes (deprecated)"),
					std::make_pair(ID_INSERT_NODS, L"NODS\tNominal &Digit Shapes (deprecated)"),
					std::make_pair(ID_INSERT_ASS, L"ASS\tActivate &Symmetric Swapping (deprecated)"),
					std::make_pair(ID_INSERT_ISS, L"ISS\tInhibit S&ymmetric Swapping (deprecated)"),
					std::make_pair(ID_INSERT_AAFS, L"AAFS\tActivate Arabic &Form Shaping (deprecated)"),
					std::make_pair(ID_INSERT_IAFS, L"IAFS\tInhibit Arabic Form S&haping (deprecated)"),
					std::make_pair(ID_INSERT_RS, L"RS\tRe&cord Separator"),
					std::make_pair(ID_INSERT_US, L"US\tUnit &Separator"),
					std::make_pair(0, L""),
					std::make_pair(ID_INSERT_IAA, L"IAA\tInterlinear Annotation Anchor"),
					std::make_pair(ID_INSERT_IAT, L"IAT\tInterlinear Annotation Terminator"),
					std::make_pair(ID_INSERT_IAS, L"IAS\tInterlinear Annotation Separator")
				};
				auto insertUnicodeControlCharacterPopup(win32::makeHandle(::CreatePopupMenu(), &::DestroyMenu));
				auto item(win32::makeZeroSize<MENUITEMINFOW>());
				for(size_t i = 0; i < std::extent<decltype(insertUnicodeControlCharacterItems)>::value; ++i) {
					if(!insertUnicodeControlCharacterItems[i].second.empty()) {
						item.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STRING;
						item.wID = insertUnicodeControlCharacterItems[i].first;
						item.dwTypeData = const_cast<WCHAR*>(insertUnicodeControlCharacterItems[i].second.c_str());
					} else {
						item.fMask = MIIM_FTYPE;
						item.fType = MFT_SEPARATOR;
					}
					::InsertMenuItemW(insertUnicodeControlCharacterPopup.get(), i, true, &item);
				}

				// under "Insert Unicode white space character"
				static const std::pair<UINT, const std::basic_string<WCHAR>> insertUnicodeWhiteSpaceCharacterItems[] = {
					std::make_pair(ID_INSERT_U0020, L"U+0020\tSpace"),
					std::make_pair(ID_INSERT_NBSP, L"NBSP\tNo-Break Space"),
					std::make_pair(ID_INSERT_U1680, L"U+1680\tOgham Space Mark"),
					std::make_pair(ID_INSERT_MVS, L"MVS\tMongolian Vowel Separator"),
					std::make_pair(ID_INSERT_U2000, L"U+2000\tEn Quad"),
					std::make_pair(ID_INSERT_U2001, L"U+2001\tEm Quad"),
					std::make_pair(ID_INSERT_U2002, L"U+2002\tEn Space"),
					std::make_pair(ID_INSERT_U2003, L"U+2003\tEm Space"),
					std::make_pair(ID_INSERT_U2004, L"U+2004\tThree-Per-Em Space"),
					std::make_pair(ID_INSERT_U2005, L"U+2005\tFour-Per-Em Space"),
					std::make_pair(ID_INSERT_U2006, L"U+2006\tSix-Per-Em Space"),
					std::make_pair(ID_INSERT_U2007, L"U+2007\tFigure Space"),
					std::make_pair(ID_INSERT_U2008, L"U+2008\tPunctuation Space"),
					std::make_pair(ID_INSERT_U2009, L"U+2009\tThin Space"),
					std::make_pair(ID_INSERT_U200A, L"U+200A\tHair Space"),
					std::make_pair(ID_INSERT_ZWSP, L"ZWSP\tZero Width Space"),
					std::make_pair(ID_INSERT_NNBSP, L"NNBSP\tNarrow No-Break Space"),
					std::make_pair(ID_INSERT_MMSP, L"MMSP\tMedium Mathematical Space"),
					std::make_pair(ID_INSERT_U3000, L"U+3000\tIdeographic Space"),
					std::make_pair(0, L""),
					std::make_pair(ID_INSERT_NEL, L"NEL\tNext Line"),
					std::make_pair(ID_INSERT_LS, L"LS\tLine Separator"),
					std::make_pair(ID_INSERT_PS, L"PS\tParagraph Separator")
				};
				auto insertUnicodeWhiteSpaceCharacterPopup(win32::makeHandle(::CreatePopupMenu(), &::DestroyMenu));
				for(size_t i = 0; i < std::extent<decltype(insertUnicodeWhiteSpaceCharacterItems)>::value; ++i) {
					if(!insertUnicodeWhiteSpaceCharacterItems[i].second.empty()) {
						item.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STRING;
						item.wID = insertUnicodeWhiteSpaceCharacterItems[i].first;
						item.dwTypeData = const_cast<WCHAR*>(insertUnicodeWhiteSpaceCharacterItems[i].second.c_str());
					} else {
						item.fMask = MIIM_FTYPE;
						item.fType = MFT_SEPARATOR;
					}
					::InsertMenuItemW(insertUnicodeWhiteSpaceCharacterPopup.get(), i, true, &item);
				}

				// toplevel
				static const std::pair<UINT, const std::basic_string<WCHAR>> toplevelItems[] = {
					std::make_pair(WM_UNDO, !japanese ? L"&Undo" : L"\x5143\x306b\x623b\x3059(&U)"),
					std::make_pair(WM_REDO, !japanese ? L"&Redo" : L"\x3084\x308a\x76f4\x3057(&R)"),
					std::make_pair(0, L""),
					std::make_pair(WM_CUT, !japanese ? L"Cu&t" : L"\x5207\x308a\x53d6\x308a(&T)"),
					std::make_pair(WM_COPY, !japanese ? L"&Copy" : L"\x30b3\x30d4\x30fc(&C)"),
					std::make_pair(WM_PASTE, !japanese ? L"&Paste" : L"\x8cbc\x308a\x4ed8\x3051(&P)"),
					std::make_pair(WM_CLEAR, !japanese ? L"&Delete" : L"\x524a\x9664(&D)"),
					std::make_pair(0, L""),
					std::make_pair(WM_SELECTALL, !japanese ? L"Select &All" : L"\x3059\x3079\x3066\x9078\x629e(&A)"),
					std::make_pair(0, L""),
					std::make_pair(ID_RTLREADING, !japanese ? L"&Right to left Reading order" : L"\x53f3\x304b\x3089\x5de6\x306b\x8aad\x3080(&R)"),
					std::make_pair(ID_DISPLAYSHAPINGCONTROLS, !japanese ? L"&Show Unicode control characters" : L"Unicode \x5236\x5fa1\x6587\x5b57\x306e\x8868\x793a(&S)"),
					std::make_pair(0, !japanese ? L"&Insert Unicode control character" : L"Unicode \x5236\x5fa1\x6587\x5b57\x306e\x633f\x5165(&I)"),
					std::make_pair(0, !japanese ? L"Insert Unicode &white space character" : L"Unicode \x7a7a\x767d\x6587\x5b57\x306e\x633f\x5165(&W)")
				};
				for(size_t i = 0; i < std::extent<decltype(toplevelItems)>::value; ++i) {
					if(toplevelItems[i].second.empty()) {
						item.fMask = MIIM_FTYPE;
						item.fType = MFT_SEPARATOR;
					} else {
						item.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STRING;
						item.wID = insertUnicodeWhiteSpaceCharacterItems[i].first;
						item.dwTypeData = const_cast<WCHAR*>(insertUnicodeWhiteSpaceCharacterItems[i].second.c_str());
						if(i == 12 || i == 13) {
							item.fMask |= MIIM_SUBMENU;
							item.hSubMenu = (i == 12) ? insertUnicodeControlCharacterPopup.get() : insertUnicodeWhiteSpaceCharacterPopup.get();
						}
					}
					::InsertMenuItemW(insertUnicodeControlCharacterPopup.get(), i, true, &item);
				}

				// check if the system supports bidi
				if(!graphics::font::supportsComplexScripts()) {
					::EnableMenuItem(toplevelPopup.get(), ID_RTLREADING, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
					::EnableMenuItem(toplevelPopup.get(), ID_DISPLAYSHAPINGCONTROLS, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
					::EnableMenuItem(toplevelPopup.get(), 12, MF_BYPOSITION | MF_DISABLED | MF_GRAYED);
					::EnableMenuItem(toplevelPopup.get(), 13, MF_BYPOSITION | MF_DISABLED | MF_GRAYED);
				}
			}
#undef GET_CAPTION

			// modify menu items
			::EnableMenuItem(toplevelPopup.get(), WM_UNDO, MF_BYCOMMAND | (!readOnly && doc.numberOfUndoableChanges() != 0) ? MF_ENABLED : MF_DISABLED | MF_GRAYED);
			::EnableMenuItem(toplevelPopup.get(), WM_REDO, MF_BYCOMMAND | (!readOnly && doc.numberOfRedoableChanges() != 0) ? MF_ENABLED : MF_DISABLED | MF_GRAYED);
			::EnableMenuItem(toplevelPopup.get(), WM_CUT, MF_BYCOMMAND | (!readOnly && hasSelection) ? MF_ENABLED : MF_DISABLED | MF_GRAYED);
			::EnableMenuItem(toplevelPopup.get(), WM_COPY, MF_BYCOMMAND | hasSelection ? MF_ENABLED : MF_DISABLED | MF_GRAYED);
			::EnableMenuItem(toplevelPopup.get(), WM_PASTE, MF_BYCOMMAND | (!readOnly && caret->canPaste(false)) ? MF_ENABLED : MF_DISABLED | MF_GRAYED);
			::EnableMenuItem(toplevelPopup.get(), WM_CLEAR, MF_BYCOMMAND | (!readOnly && hasSelection) ? MF_ENABLED : MF_DISABLED | MF_GRAYED);
			::EnableMenuItem(toplevelPopup.get(), WM_SELECTALL, MF_BYCOMMAND | (doc.numberOfLines() > 1 || doc.lineLength(0) > 0) ? MF_ENABLED : MF_DISABLED | MF_GRAYED);
			auto item(win32::makeZeroSize<MENUITEMINFOW>());
			item.fMask = MIIM_STATE;
			item.fState = ((textArea()->textRenderer()->inlineFlowDirection() == presentation::RIGHT_TO_LEFT) ? MFS_CHECKED : MFS_UNCHECKED) | MFS_ENABLED | MFS_UNHILITE;
			::SetMenuItemInfoW(toplevelPopup.get(), ID_RTLREADING, false, &item);
			// TODO: Not implemented.
//			item.fState = (textArea()->textRenderer().displaysShapingControls() ? MFS_CHECKED : MFS_UNCHECKED) | MFS_ENABLED | MFS_UNHILITE;
//			::SetMenuItemInfoW(toplevelPopup.get(), ID_DISPLAYSHAPINGCONTROLS, false, &item);

			// IME commands
			HKL keyboardLayout = ::GetKeyboardLayout(::GetCurrentThreadId());
			if(//win32::boole(::ImmIsIME(keyboardLayout)) &&
					::ImmGetProperty(keyboardLayout, IGP_SENTENCE) != IME_SMODE_NONE) {
				auto imc(win32::inputMethod(*this));
				const std::basic_string<WCHAR>
					openIme(japanese ? L"IME \x3092\x958b\x304f(&O)" : L"&Open IME"),
					closeIme(japanese ? L"IME \x3092\x9589\x3058\x308b(&L)" : L"C&lose IME"),
					openSoftKeyboard(japanese ? L"\x30bd\x30d5\x30c8\x30ad\x30fc\x30dc\x30fc\x30c9\x3092\x958b\x304f(&E)" : L"Op&en soft keyboard"),
					closeSoftKeyboard(japanese ? L"\x30bd\x30d5\x30c8\x30ad\x30fc\x30dc\x30fc\x30c9\x3092\x9589\x3058\x308b(&F)" : L"Close so&ft keyboard"),
					reconvert(japanese ? L"\x518d\x5909\x63db(&R)" : L"&Reconvert");

				auto item(win32::makeZeroSize<MENUITEMINFOW>());
				item.fMask = MIIM_FTYPE;
				item.fType = MFT_SEPARATOR;
				::InsertMenuItemW(toplevelPopup.get(), ::GetMenuItemCount(toplevelPopup.get()), true, &item);
				item.fMask = MIIM_ID | MIIM_STRING;
				item.wID = ID_TOGGLEIMESTATUS;
				item.dwTypeData = const_cast<WCHAR*>(win32::boole(::ImmGetOpenStatus(imc.get())) ? closeIme.c_str() : openIme.c_str());
				::InsertMenuItemW(toplevelPopup.get(), ::GetMenuItemCount(toplevelPopup.get()), true, &item);
				item.fMask = MIIM_ID | MIIM_STRING;

				if(win32::boole(::ImmGetProperty(keyboardLayout, IGP_CONVERSION) & IME_CMODE_SOFTKBD)) {
					DWORD convMode;
					::ImmGetConversionStatus(imc.get(), &convMode, nullptr);
					item.wID = ID_TOGGLESOFTKEYBOARD;
					item.dwTypeData = const_cast<WCHAR*>(win32::boole(convMode & IME_CMODE_SOFTKBD) ? closeSoftKeyboard.c_str() : openSoftKeyboard.c_str());
					::InsertMenuItemW(toplevelPopup.get(), ::GetMenuItemCount(toplevelPopup.get()), true, &item);
				}

				if(win32::boole(::ImmGetProperty(keyboardLayout, IGP_SETCOMPSTR) & SCS_CAP_SETRECONVERTSTRING)) {
					item.fMask |= MIIM_STATE;
					item.wID = ID_RECONVERT;
					item.dwTypeData = const_cast<WCHAR*>(reconvert.c_str());
					item.fState = (!readOnly && hasSelection) ? MFS_ENABLED : (MFS_DISABLED | MFS_GRAYED);
					::InsertMenuItemW(toplevelPopup.get(), ::GetMenuItemCount(toplevelPopup.get()), true, &item);
				}
			}

			// hyperlink
			if(const presentation::hyperlink::Hyperlink* link = utils::getPointedHyperlink(*this, insertionPosition(*caret))) {
				const Index len = link->region().size() * 2 + 8;
				std::unique_ptr<WCHAR[]> caption(new WCHAR[len]);	// TODO: this code can have buffer overflow in future
				swprintf(caption.get(),
#if(_MSC_VER < 1400)
#else
					len,
#endif // _MSC_VER < 1400
					japanese ? L"\x202a%s\x202c \x3092\x958b\x304f" : L"Open \x202a%s\x202c", escapeAmpersands(doc.lineString(
						kernel::line(*caret)).substr(link->region().front(), link->region().size())).c_str());
				auto item(win32::makeZeroSize<MENUITEMINFOW>());
				item.fMask = MIIM_FTYPE;
				item.fType = MFT_SEPARATOR;
				::InsertMenuItemW(toplevelPopup.get(), ::GetMenuItemCount(toplevelPopup.get()), true, &item);
				item.fMask = MIIM_ID | MIIM_STRING;
				item.wID = ID_INVOKE_HYPERLINK;
				item.dwTypeData = caption.get();
				::InsertMenuItemW(toplevelPopup.get(), ::GetMenuItemCount(toplevelPopup.get()), true, &item);
			}

			int x = static_cast<const MSG*>(nativeEvent)->pt.x, y = static_cast<const MSG*>(nativeEvent)->pt.y;
			if(x == 0xffffu && y == 0xffffu)
				x = y = 5;	// TODO: 
			::TrackPopupMenu(toplevelPopup.get(), TPM_LEFTALIGN, x, y, 0, handle().get(), 0);

			// ...finally erase all items
			int c = ::GetMenuItemCount(toplevelPopup.get());
			while(c > 13)
				::DeleteMenu(toplevelPopup.get(), c--, MF_BYPOSITION);
		}
	}
}

#endif // ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
