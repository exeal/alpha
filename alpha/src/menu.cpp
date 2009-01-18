/**
 * @file menu.cpp
 * Implements @c Menu class.
 */

#include <objsafe.h>	// interface
#include "ambient-idl.hpp"
#include "application.hpp"
#include "ui.hpp"
#include <manah/com/unknown-impl.hpp>
#include <manah/com/dispatch-impl.hpp>
using namespace alpha;
using namespace alpha::ambient;
using namespace manah::com;
using namespace manah::com::ole;
using namespace std;


namespace {
	// returns true if the parameter with attribute "optional" was omiited when call.
	inline bool isParameterOmitted(const VARIANTARG& parameter) /*throw()*/ {
		return parameter.vt == VT_ERROR && parameter.scode == DISP_E_PARAMNOTFOUND;
	}
	// converts the given VARIANT into IDispatch*. if 'parameter' is as omitted, returns null.
	inline ComPtr<IDispatch> VARIANTtoDISPATCH(const VARIANTARG& parameter) {
		if(parameter.vt == VT_DISPATCH)
			return ComPtr<IDispatch>(parameter.pdispVal);
		else if(parameter.vt == VT_UNKNOWN) {
			ComPtr<IDispatch> temp;
			parameter.punkVal->QueryInterface(IID_IDispatch, temp.initializePPV());
			return temp;
		} else if(parameter.vt == VT_NULL || isParameterOmitted(parameter))
			return ComPtr<IDispatch>();
		else
			throw bad_cast("unable to convert into IDispatch*.");
	}
} // namespace @0


// Menu /////////////////////////////////////////////////////////////////////

template<typename Interface>
class IDispatchExStaticImpl : public Interface {
public:
	STDMETHODIMP GetDispID(BSTR name, DWORD flags, DISPID* id) {
		if((flags & fdexNameImplicit) != 0)
			return E_NOTIMPL;
		return GetIDsOfNames(IID_NULL, &name, 1, LOCALE_INVARIANT, id);
	}
	STDMETHODIMP InvokeEx(DISPID id, LCID lcid, WORD flags, DISPPARAMS* parameters, VARIANT* result, EXCEPINFO* exception, IServiceProvider*) {
		unsigned int dummy;
		if(flags == DISPATCH_CONSTRUCT)
			flags = DISPATCH_METHOD;
		return Invoke(id, IID_NULL, lcid, flags, parameters, result, exception, &dummy);
	}
	STDMETHODIMP DeleteMemberByName(BSTR name, DWORD flags) {
		DISPID id;
		const HRESULT hr = GetDispID(name, flags, &id);
		return SUCCEEDED(hr) ? S_FALSE : hr;
	}
	STDMETHODIMP DeleteMemberByDispID(DISPID id) {
		ComPtr<ITypeInfo> ti(typeInformation());
		if(ti.get() != 0) {
			const HRESULT hr = ti->GetDocumentation(id, 0, 0, 0, 0);
			return SUCCEEDED(hr) ? S_FALSE : hr;
		}
		return E_NOTIMPL;
	}
	STDMETHODIMP GetMemberProperties(DISPID, DWORD, DWORD*) {
		return E_NOTIMPL;	// yes, this method can be implemented by using ITypeInfo...
	}
	STDMETHODIMP GetMemberName(DISPID id, BSTR* name) {
		MANAH_VERIFY_POINTER(name);
		ComPtr<ITypeInfo> ti(typeInformation());
		if(ti.get() != 0) {
			unsigned int c;
			return ti->GetNames(id, name, 1, &c);
		}
		return E_NOTIMPL;
	}
	STDMETHODIMP GetNextDispID(DWORD flags, DISPID id, DISPID* next) {
		MANAH_VERIFY_POINTER(next);
		*next = DISPID_UNKNOWN;
		if((flags | fdexEnumDefault | fdexEnumAll) != (fdexEnumDefault | fdexEnumAll))
			return E_INVALIDARG;
		else if((flags & (fdexEnumDefault | fdexEnumAll)) == 0)
			return E_INVALIDARG;
		buildIdentifiers();
		vector<DISPID>::const_iterator i;
		if(id == DISPID_STARTENUM) {
			i = (((flags & fdexEnumAll) != 0) ? identifiers_ : defaultIdentifiers_).begin();
			if(i == identifiers_.end())
				i = defaultIdentifiers_.begin();
			if(i == defaultIdentifiers_.end())
				return S_FALSE;
		} else {
			if((flags & fdexEnumAll) != 0)
				i = lower_bound(identifiers_.begin(), identifiers_.end(), id);
			if((flags & fdexEnumAll) == 0 || i == identifiers_.end() || *i != id)
				i = lower_bound(defaultIdentifiers_.begin(), defaultIdentifiers_.end(), id);
		}
		if(i == defaultIdentifiers_.end() || *i != id)
			return E_INVALIDARG;
		if(++i == identifiers_.end())
			i = defaultIdentifiers_.begin();
		if(i == defaultIdentifiers_.end())
			return S_FALSE;
		*next = *i;
		return S_OK;
	}
	STDMETHODIMP GetNameSpaceParent(IUnknown** p) {
		MANAH_VERIFY_POINTER(p);
		*p = 0;
		return E_NOTIMPL;
	}
private:
	void buildIdentifiers() /*throw()*/ {
		if(!identifiers_.empty()) {
			ComPtr<ITypeInfo> ti(typeInformation());
			if(ti.get() != 0) {
				TYPEATTR* attributes;
				if(SUCCEEDED(ti->GetTypeAttr(&attributes))) {
					identifiers_.reserve(attributes->cFuncs + attributes->cVars);
					FUNCDESC* function;
					for(WORD i = 0; i < attributes->cFuncs; ++i) {
						if(SUCCEEDED(ti->GetFuncDesc(i, &function))) {
							(((function->wFuncFlags & FUNCFLAG_FDEFAULTBIND) == 0) ? identifiers_ : defaultIdentifiers_).push_back(function->memid);
							ti->ReleaseFuncDesc(function);
						}
					}
					VARDESC* variable;
					for(WORD i = 0; i < attributes->cVars; ++i) {
						if(SUCCEEDED(ti->GetVarDesc(i, &variable))) {
							(((variable->wVarFlags & VARFLAG_FDEFAULTBIND) == 0) ? identifiers_ : defaultIdentifiers_).push_back(variable->memid);
							ti->ReleaseVarDesc(variable);
						}
					}
					ti->ReleaseTypeAttr(attributes);
				}
			}
			sort(identifiers_.begin(), identifiers_.end());
			sort(defaultIdentifiers_.begin(), defaultIdentifiers_.end());
		}
	}
	ComPtr<ITypeInfo> typeInformation() /*throw()*/ {
		ComPtr<ITypeInfo> temp;
		GetTypeInfo(0, LOCALE_INVARIANT, temp.initialize());
		return temp;
	}
	vector<DISPID> identifiers_, defaultIdentifiers_;
};

namespace {
	template<typename Interface, const IID* iid>
	class Menu : public IUnknownDispatchImpl<
		manah::typelist::Cat<
			MANAH_INTERFACE_SIGNATURE(IMenu), manah::typelist::Cat<
				InterfaceSignature<Interface, iid>
			>
		>, TypeInformationFromExecutable<iid>
	> {
	public:
		virtual ~Menu() throw();
		// IMenu
		STDMETHODIMP Append(short identifier, BSTR caption, VARIANT* command, VARIANT_BOOL alternative, IMenu** self);
		STDMETHODIMP AppendSeparator(IMenu** self);
		STDMETHODIMP Check(short identifier, VARIANT_BOOL check, IMenu** self);
		STDMETHODIMP Enable(short identifier, VARIANT_BOOL enable, IMenu** self);
		STDMETHODIMP Erase(short identifier, IMenu** self);
		STDMETHODIMP GetHandle(LONG_PTR* handle);
		STDMETHODIMP SetChild(short identifier, IMenu* child, IMenu** self);
		STDMETHODIMP SetDefault(short identifier, IMenu** self);
	protected:
		explicit Menu(HMENU handle);
	private:
		HMENU handle_;
		set<IMenu*> children_;
	};

	class PopupMenu : public Menu<IPopupMenu, &IID_IPopupMenu> {
	public:
		explicit PopupMenu(IDispatch* popupHandler);
		// IPopupMenu
		STDMETHODIMP Update(short identifier);
	private:
		ComPtr<IDispatch> popupHandler_;
	};

	class MenuBar : public Menu<IMenuBar, &IID_IMenuBar> {
	public:
		MenuBar();
		// IMenuBar
		STDMETHODIMP SetAsMenuBar(IMenuBar** oldMenuBar);
	};

	class MenuConstructor : public IDispatchExStaticImpl<
		IUnknownDispatchImpl<
			manah::typelist::Cat<MANAH_INTERFACE_SIGNATURE(IPopupMenuConstructor),
			manah::typelist::Cat<MANAH_INTERFACE_SIGNATURE(IDispatchEx)> >,
			TypeInformationFromExecutable<&IID_IPopupMenuConstructor>
		>
	> {
	public:
		// IMenuConstructor
		STDMETHODIMP Construct(VARIANT popupHandler, IPopupMenu** instance);
	};

	class MenuBarConstructor : public IDispatchExStaticImpl<
		IUnknownDispatchImpl<
			manah::typelist::Cat<MANAH_INTERFACE_SIGNATURE(IMenuBarConstructor),
			manah::typelist::Cat<MANAH_INTERFACE_SIGNATURE(IDispatchEx)> >,
			TypeInformationFromExecutable<&IID_IMenuBarConstructor>
		>
	> {
	public:
		// IMenuBarConstructor
		STDMETHODIMP Construct(IMenuBar** instance);
	};

	struct Installer {
		Installer() {
			ServiceProvider& sp = ServiceProvider::instance();
			ComPtr<IDispatch> temp(new MenuConstructor);
			sp.registerService(OLESTR("ui:menu-constructor"), *temp.get());
			temp.reset(new MenuBarConstructor);
			sp.registerService(OLESTR("ui:menu-bar-constructor"), *temp.get());
		}
	} installer;
} // namespace @0

template<typename Interface, const IID* iid>
Menu<Interface, iid>::Menu(HMENU handle) : handle_(handle) {
	if(handle == 0)
		throw invalid_argument("handle");
	// enable WM_MENUCOMMAND
	manah::win32::AutoZeroSize<MENUINFO> mi;
	mi.fMask = MIM_STYLE;
	mi.dwStyle = MNS_NOTIFYBYPOS;
	::SetMenuInfo(handle_, &mi);
}

template<typename Interface, const IID* iid>
Menu<Interface, iid>::~Menu() throw() {
	if(handle_ != 0) {
		manah::win32::AutoZeroSize<MENUITEMINFOW> mi;
		mi.fMask = MIIM_DATA;
		while(::GetMenuItemCount(handle_) > 0) {
			if(::GetMenuItemInfoW(handle_, 0, true, &mi) != 0 && mi.dwItemData != 0)
				reinterpret_cast<IDispatch*>(mi.dwItemData)->Release();
			::RemoveMenu(handle_, 0, MF_BYPOSITION);
		}
		::DestroyMenu(handle_);
	}
	for(set<IMenu*>::iterator i(children_.begin()), e(children_.end()); i != e; ++i)
		(*i)->Release();
}

template<typename Interface, const IID* iid>
STDMETHODIMP Menu<Interface, iid>::Append(short identifier, BSTR caption, VARIANT* command, VARIANT_BOOL alternative, IMenu** self) {
	if(::SysStringLen(caption) == 0)
		return E_INVALIDARG;
	MANAH_VERIFY_POINTER(command);
	ComPtr<IDispatch> commandObject;
	try {
		commandObject = VARIANTtoDISPATCH(*command);
	} catch(bad_cast&) {
		return DISP_E_TYPEMISMATCH;
	}
	if(self != 0)
		(*self = this)->AddRef();

	IDispatch* const temp = commandObject.get();
	manah::win32::AutoZeroSize<MENUITEMINFOW> item;
	item.fMask = MIIM_DATA | MIIM_FTYPE | MIIM_ID | MIIM_STATE | MIIM_STRING;
	item.fType = MFT_STRING | (alternative ? MFT_RADIOCHECK : 0);
	item.fState = MFS_ENABLED | MFS_UNCHECKED;
	item.wID = identifier;
	item.dwItemData = reinterpret_cast<UINT_PTR>(temp);
	item.dwTypeData = caption;
	if(::InsertMenuItemW(handle_, ::GetMenuItemCount(handle_), true, &item) == 0)
		return HRESULT_FROM_WIN32(::GetLastError());
	if(item.dwItemData != 0)
		temp->AddRef();
	return S_OK;
}

template<typename Interface, const IID* iid>
STDMETHODIMP Menu<Interface, iid>::AppendSeparator(IMenu** self) {
	if(self != 0)
		(*self = this)->AddRef();
	manah::win32::AutoZeroSize<MENUITEMINFOW> item;
	item.fMask = MIIM_TYPE;
	item.fType = MFT_SEPARATOR;
	if(::InsertMenuItemW(handle_, ::GetMenuItemCount(handle_), true, &item) == 0)
		return HRESULT_FROM_WIN32(::GetLastError());
	return S_OK;
}

namespace {
	inline HRESULT setMenuItemState(HMENU menu, short id, UINT statesToAdd, UINT statesToRemove) {
		manah::win32::AutoZeroSize<MENUITEMINFOW> item;
		item.fMask = MIIM_STATE;
		if(manah::toBoolean(::GetMenuItemInfoW(menu, id, false, &item))) {
			item.fState &= ~statesToRemove;
			item.fState |= statesToAdd;
			if(manah::toBoolean(::SetMenuItemInfoW(menu, id, false, &item)))
				return S_OK;
		}
		return HRESULT_FROM_WIN32(::GetLastError());
	}
} // namespace @0

template<typename Interface, const IID* iid>
STDMETHODIMP Menu<Interface, iid>::Check(short identifier, VARIANT_BOOL check, IMenu** self) {
	if(self != 0)
		(*self = this)->AddRef();
	return setMenuItemState(handle_, identifier, manah::toBoolean(check) ? MFS_CHECKED : MFS_UNCHECKED, manah::toBoolean(check) ? MFS_UNCHECKED : MFS_CHECKED);
}

template<typename Interface, const IID* iid>
STDMETHODIMP Menu<Interface, iid>::Enable(short identifier, VARIANT_BOOL enable, IMenu** self) {
	if(self != 0)
		(*self = this)->AddRef();
	return setMenuItemState(handle_, identifier,
		manah::toBoolean(enable) ? MFS_ENABLED : (MFS_DISABLED | MFS_GRAYED), manah::toBoolean(enable) ? (MFS_DISABLED | MFS_GRAYED) : MFS_ENABLED);
}
/*
STDMETHODIMP Menu::Erase(IMenu** self) {
	if(self != 0)
		(*self = this)->AddRef();
	// TODO: this code is not exception-safe and can cause infinity-loop.
	while(::GetMenuItemCount(handle_) > 0)
		::RemoveMenu(handle_, 0, MF_BYPOSITION);
	return S_OK;
}
*/
template<typename Interface, const IID* iid>
STDMETHODIMP Menu<Interface, iid>::Erase(short identifier, IMenu** self) {
	if(self != 0)
		(*self = this)->AddRef();
	manah::win32::AutoZeroSize<MENUITEMINFOW> mi;
	mi.fMask = MIIM_DATA;
	if(::GetMenuItemInfoW(handle_, 0, true, &mi) != 0 && mi.dwItemData != 0)
		reinterpret_cast<IDispatch*>(mi.dwItemData)->Release();
	if(!manah::toBoolean(::RemoveMenu(handle_, identifier, MF_BYCOMMAND)))
		HRESULT_FROM_WIN32(::GetLastError());
	return S_OK;
}

template<typename Interface, const IID* iid>
STDMETHODIMP Menu<Interface, iid>::GetHandle(LONG_PTR* handle) {
	MANAH_VERIFY_POINTER(handle);
	return (*handle = reinterpret_cast<LONG_PTR>(handle_)), S_OK;
}

template<typename Interface, const IID* iid>
STDMETHODIMP Menu<Interface, iid>::SetChild(short identifier, IMenu* child, IMenu** self) {
	if(child == 0 || children_.find(child) != children_.end())
		return E_INVALIDARG;
	LONG_PTR handle;
	if(FAILED(child->GetHandle(&handle)))
		return E_INVALIDARG;
	ComQIPtr<IPopupMenu, &IID_IPopupMenu> dummy(child);
	manah::win32::AutoZeroSize<MENUITEMINFOW> item;
	item.fMask = MIIM_DATA | MIIM_SUBMENU;
	item.dwItemData = reinterpret_cast<ULONG_PTR>(child);
	item.hSubMenu = reinterpret_cast<HMENU>(handle);
	if(::SetMenuItemInfoW(handle_, identifier, false, &item) == 0)
		HRESULT_FROM_WIN32(::GetLastError());
	children_.insert(child);
	child->AddRef();
	if(self != 0)
		(*self = this)->AddRef();
	return S_OK;
}

template<typename Interface, const IID* iid>
STDMETHODIMP Menu<Interface, iid>::SetDefault(short identifier, IMenu** self) {
	manah::win32::AutoZeroSize<MENUITEMINFOW> item;
	item.fMask = MIIM_ID | MIIM_STATE;
	for(UINT i = 0, c = ::GetMenuItemCount(handle_); i < c; ++i) {
		if(manah::toBoolean(::GetMenuItemInfoW(handle_, i, true, &item))) {
			if(item.wID == identifier)
				item.fState |= MFS_DEFAULT;
			else
				item.fState &= ~MFS_DEFAULT;
			::SetMenuItemInfoW(handle_, i, true, &item);
		}
	}
	if(self != 0)
		(*self = this)->AddRef();
	return S_OK;
}


// MenuBar //////////////////////////////////////////////////////////////////

MenuBar::MenuBar() : Menu(::CreateMenu()) {
}

STDMETHODIMP MenuBar::SetAsMenuBar(IMenuBar** oldMenuBar) {
	static ComPtr<IMenuBar> singletonHolder;
	LONG_PTR handle;
	GetHandle(&handle);
	if(!Alpha::instance().getMainWindow().setMenu(reinterpret_cast<HMENU>(handle)))
		return HRESULT_FROM_WIN32(::GetLastError());
	if(oldMenuBar != 0) {
		if(*oldMenuBar = singletonHolder.get())
			(*oldMenuBar)->AddRef();
	}
	singletonHolder.reset(this);
	return S_OK;
}


// PopupMenu ////////////////////////////////////////////////////////////////

PopupMenu::PopupMenu(IDispatch* popupHandler) : Menu(::CreatePopupMenu()), popupHandler_(popupHandler) {
}

STDMETHODIMP PopupMenu::Update(short identifier) {
	if(popupHandler_ != 0) {
		manah::win32::AutoZero<DISPPARAMS> parameters;
		parameters.rgvarg = new VARIANTARG[parameters.cArgs = 2];
		::VariantInit(&parameters.rgvarg[0]);
		parameters.rgvarg[0].vt = VT_DISPATCH;
		QueryInterface(IID_IDispatch, reinterpret_cast<void**>(&parameters.rgvarg[0].pdispVal));
		::VariantInit(&parameters.rgvarg[1]);
		parameters.rgvarg[1].vt = VT_I2;
		parameters.rgvarg[1].iVal = identifier;

		VARIANT result;
		::VariantInit(&result);

		manah::win32::AutoZero<EXCEPINFO> exception;
		unsigned int argErr;

		// TODO: call InvokeEx instead for security.
		popupHandler_->Invoke(DISPID_VALUE, IID_NULL,
			LOCALE_USER_DEFAULT, DISPATCH_METHOD, &parameters, &result, &exception, &argErr);
		::VariantClear(&parameters.rgvarg[0]);
		::VariantClear(&parameters.rgvarg[1]);
		::VariantClear(&result);
		delete[] parameters.rgvarg;
	}
	return S_OK;
}


// MenuConstructor //////////////////////////////////////////////////////////

/// @see IPopupMenuConstructor#Construct
STDMETHODIMP MenuConstructor::Construct(VARIANT popupHandler, IPopupMenu** instance) {
	MANAH_VERIFY_POINTER(instance);
	try {
		ComPtr<IDispatch> handler(VARIANTtoDISPATCH(popupHandler));
		*instance = new PopupMenu(handler.get());
	} catch(const bad_cast&) {
		return DISP_E_TYPEMISMATCH;
	} catch(const bad_alloc&) {
		*instance = 0;
		return E_OUTOFMEMORY;
	}
	(*instance)->AddRef();
	return S_OK;
}


// MenuBarConstructor ///////////////////////////////////////////////////////

/// @see IMenuBarConstructor#Construct
STDMETHODIMP MenuBarConstructor::Construct(IMenuBar** instance) {
	MANAH_VERIFY_POINTER(instance);
	try {
		*instance = new MenuBar();
	} catch(bad_alloc&) {
		*instance = 0;
		return E_OUTOFMEMORY;
	}
	(*instance)->AddRef();
	return S_OK;
}


// free functions ///////////////////////////////////////////////////////////

namespace {
	pair<IPopupMenu*, short> findPopupMenu(HMENU parent, HMENU popup, UINT position) {
		pair<IPopupMenu*, short> result;
		manah::win32::AutoZeroSize<MENUITEMINFOW> item;
		// try an item at 'index'
		item.fMask = MIIM_DATA | MIIM_ID | MIIM_SUBMENU;
		if(manah::toBoolean(::GetMenuItemInfoW(parent, position, true, &item)) && item.hSubMenu == popup) {
			if(item.wID <= static_cast<UINT>(numeric_limits<short>::max()) && item.dwItemData != 0) {
				result.first = reinterpret_cast<IPopupMenu*>(item.dwItemData);
				result.second = static_cast<short>(item.wID);
				return result;
			}
		}
		// 
		for(UINT i = 0, c = ::GetMenuItemCount(parent); i < c; ++i) {
			if(::GetMenuItemInfoW(parent, i, true, &item) && item.hSubMenu != 0) {
				result = findPopupMenu(item.hSubMenu, popup, position);
				if(result.first != 0)
					return result;
			}
		}
		return make_pair<PopupMenu*, UINT>(0, 0U);
	}
}

void ambient::ui::handleINITMENUPOPUP(WPARAM wp, LPARAM lp) {
	if(HIWORD(lp) != 0)
		return;	// system menu
	HMENU menuBar = Alpha::instance().getMainWindow().getMenu().get();
	if(manah::toBoolean(::IsMenu(menuBar))) {
		pair<IPopupMenu*, short> popup(findPopupMenu(menuBar, reinterpret_cast<HMENU>(wp), LOWORD(lp)));
		if(popup.first != 0)
			popup.first->Update(popup.second);
	}
}

void ambient::ui::handleMENUCOMMAND(WPARAM wp, LPARAM lp) {
	manah::win32::AutoZeroSize<MENUITEMINFOW> mi;
	mi.fMask = MIIM_DATA;
	if(::GetMenuItemInfoW(reinterpret_cast<HMENU>(lp), static_cast<UINT>(wp), true, &mi) != 0) {
		if(IDispatch* command = reinterpret_cast<IDispatch*>(mi.dwItemData)) {
			manah::win32::AutoZero<DISPPARAMS> parameters;
			command->Invoke(DISPID_VALUE, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &parameters, 0, 0, 0);
		}
	}
}
