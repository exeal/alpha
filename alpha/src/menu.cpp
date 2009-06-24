/**
 * @file menu.cpp
 * Implements @c Menu class.
 */

#include "application.hpp"
#include "ui.hpp"
using namespace alpha;
using namespace alpha::ambient;
using namespace manah;
using namespace std;
namespace py = boost::python;


// Menu /////////////////////////////////////////////////////////////////////

namespace {
	class PopupMenu;

	class Menu {
	public:
		Menu();
		virtual ~Menu() /*throw()*/;
		py::object append(short identifier, const wstring& caption, py::object command, bool alternative);
		py::object appendSeparator();
		py::object check(short identifier, bool check);
		py::object enable(short identifier, bool enable);
		py::object erase(short identifier);
		py::object setChild(short identifier, py::object child);
		py::object setDefault(short identifier);
	protected:
		explicit Menu(HMENU handle);
		HMENU handle() const;
		py::object self();
	private:
		HMENU handle_;
		py::object self_;
		set<py::object> children_;
	};

	class PopupMenu : public Menu {
	public:
		explicit PopupMenu(py::object popupHandler);
		void update(short identifier);
	private:
		py::object popupHandler_;
	};

	class MenuBar : public Menu {
	public:
		MenuBar();
		static py::object setAsMenuBar(py::object newMenuBar);
	};
} // namespace @0

Menu::Menu(HMENU handle) : handle_(handle) {
	if(handle == 0)
		throw invalid_argument("handle");
	// enable WM_MENUCOMMAND
	win32::AutoZeroSize<MENUINFO> mi;
	mi.fMask = MIM_STYLE;
	mi.dwStyle = MNS_NOTIFYBYPOS;
	::SetMenuInfo(handle_, &mi);
}

Menu::~Menu() /*throw()*/ {
	if(handle_ != 0) {
		win32::AutoZeroSize<MENUITEMINFOW> mi;
		mi.fMask = MIIM_DATA;
		while(::GetMenuItemCount(handle_) > 0) {
			if(::GetMenuItemInfoW(handle_, 0, true, &mi) != 0 && mi.dwItemData != 0)
				Py_DECREF(reinterpret_cast<PyObject*>(mi.dwItemData));
			::RemoveMenu(handle_, 0, MF_BYPOSITION);
		}
		::DestroyMenu(handle_);
	}
//	for(set<PopupMenu*>::iterator i(children_.begin()), e(children_.end()); i != e; ++i)
//		delete *i;
}

py::object Menu::append(short identifier, const wstring& caption, py::object command, bool alternative) {
	if(caption.empty() || (command != py::object() && !toBoolean(::PyCallable_Check(command.ptr())))) {
		::PyErr_BadArgument();
		py::throw_error_already_set();
	}

	win32::AutoZeroSize<MENUITEMINFOW> item;
	item.fMask = MIIM_DATA | MIIM_FTYPE | MIIM_ID | MIIM_STATE | MIIM_STRING;
	item.fType = MFT_STRING | (alternative ? MFT_RADIOCHECK : 0);
	item.fState = MFS_ENABLED | MFS_UNCHECKED;
	item.wID = identifier;
	item.dwItemData = reinterpret_cast<UINT_PTR>(command.ptr());
	item.dwTypeData = const_cast<WCHAR*>(caption.c_str());
	if(::InsertMenuItemW(handle_, ::GetMenuItemCount(handle_), true, &item) == 0)
		Interpreter::instance().throwLastWin32Error();
	Py_XINCREF(reinterpret_cast<PyObject*>(item.dwItemData));
	return self();
}

py::object Menu::appendSeparator() {
	win32::AutoZeroSize<MENUITEMINFOW> item;
	item.fMask = MIIM_TYPE;
	item.fType = MFT_SEPARATOR;
	if(::InsertMenuItemW(handle_, ::GetMenuItemCount(handle_), true, &item) == 0)
		Interpreter::instance().throwLastWin32Error();
	return self();
}

namespace {
	inline void setMenuItemState(HMENU menu, short id, UINT statesToAdd, UINT statesToRemove) {
		win32::AutoZeroSize<MENUITEMINFOW> item;
		item.fMask = MIIM_STATE;
		if(toBoolean(::GetMenuItemInfoW(menu, id, false, &item))) {
			item.fState &= ~statesToRemove;
			item.fState |= statesToAdd;
			if(toBoolean(::SetMenuItemInfoW(menu, id, false, &item)))
				return;
		}
		Interpreter::instance().throwLastWin32Error();
	}
} // namespace @0

py::object Menu::check(short identifier, bool check) {
	setMenuItemState(handle_, identifier, check ? MFS_CHECKED : MFS_UNCHECKED, check ? MFS_UNCHECKED : MFS_CHECKED);
	return self();
}

py::object Menu::enable(short identifier, bool enable) {
	setMenuItemState(handle_, identifier,
		enable ? MFS_ENABLED : (MFS_DISABLED | MFS_GRAYED), enable ? (MFS_DISABLED | MFS_GRAYED) : MFS_ENABLED);
	return self();
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
py::object Menu::erase(short identifier) {
	win32::AutoZeroSize<MENUITEMINFOW> mi;
	mi.fMask = MIIM_DATA;
	if(::GetMenuItemInfoW(handle_, 0, true, &mi) != 0) {
		for(set<py::object>::iterator i(children_.begin()), e(children_.end()); i != e; ++i) {
			if(i->ptr() == reinterpret_cast<PyObject*>(mi.dwItemData)) {
				children_.erase(i);
				break;
			}
		}
	}
	if(!toBoolean(::RemoveMenu(handle_, identifier, MF_BYCOMMAND)))
		Interpreter::instance().throwLastWin32Error();
	return self();
}

inline HMENU Menu::handle() const {
	return handle_;
}

py::object Menu::self() {
	if(self_ == py::object())
		self_ = py::object(py::ptr(this));
	return self_;
}

py::object Menu::setChild(short identifier, py::object child) {
	if(children_.find(child) != children_.end()) {
		::PyErr_BadArgument();
		py::throw_error_already_set();
	}
	HMENU childHandle = static_cast<Menu&>(py::extract<Menu&>(child)).handle();
	win32::AutoZeroSize<MENUITEMINFOW> item;
	item.fMask = MIIM_DATA | MIIM_SUBMENU;
	item.dwItemData = reinterpret_cast<ULONG_PTR>(child.ptr());
	item.hSubMenu = childHandle;
	if(::SetMenuItemInfoW(handle_, identifier, false, &item) == 0)
		Interpreter::instance().throwLastWin32Error();
	children_.insert(child);
	return self();
}

py::object Menu::setDefault(short identifier) {
	win32::AutoZeroSize<MENUITEMINFOW> item;
	item.fMask = MIIM_ID | MIIM_STATE;
	for(UINT i = 0, c = ::GetMenuItemCount(handle_); i < c; ++i) {
		if(toBoolean(::GetMenuItemInfoW(handle_, i, true, &item))) {
			if(item.wID == identifier)
				item.fState |= MFS_DEFAULT;
			else
				item.fState &= ~MFS_DEFAULT;
			::SetMenuItemInfoW(handle_, i, true, &item);
		}
	}
	return self();
}


// MenuBar //////////////////////////////////////////////////////////////////

MenuBar::MenuBar() : Menu(::CreateMenu()) {
}

py::object MenuBar::setAsMenuBar(py::object newMenuBar) {
	static py::object singletonHolder;
	if(!Alpha::instance().getMainWindow().setMenu(static_cast<MenuBar&>(py::extract<MenuBar&>(newMenuBar)).handle()))
		Interpreter::instance().throwLastWin32Error();
	py::object oldMenuBar(singletonHolder);
	singletonHolder = newMenuBar;
	return oldMenuBar;
}


// PopupMenu ////////////////////////////////////////////////////////////////

PopupMenu::PopupMenu(py::object popupHandler) : Menu(::CreatePopupMenu()), popupHandler_(popupHandler) {
	if(popupHandler != py::object() && !toBoolean(::PyCallable_Check(popupHandler_.ptr()))) {
		::PyErr_BadArgument();
		py::throw_error_already_set();
	}
}

void PopupMenu::update(short identifier) {
	if(popupHandler_ != py::object()) {
		try {
			popupHandler_(identifier, self());
		} catch(py::error_already_set&) {
			Interpreter::instance().handleException();
		}
	}
}


// free functions ///////////////////////////////////////////////////////////

namespace {
	pair<py::object, short> findPopupMenu(HMENU parent, HMENU popup, UINT position) {
		pair<py::object, short> result;
		manah::win32::AutoZeroSize<MENUITEMINFOW> item;
		// try an item at 'index'
		item.fMask = MIIM_DATA | MIIM_ID | MIIM_SUBMENU;
		if(toBoolean(::GetMenuItemInfoW(parent, position, true, &item)) && item.hSubMenu == popup) {
			if(item.wID <= static_cast<UINT>(numeric_limits<short>::max()) && item.dwItemData != 0) {
				result.first = py::object(py::borrowed(reinterpret_cast<PyObject*>(item.dwItemData)));
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
		return make_pair<py::object, UINT>(py::object(), 0U);
	}
}

void ambient::ui::handleINITMENUPOPUP(WPARAM wp, LPARAM lp) {
	if(HIWORD(lp) != 0)
		return;	// system menu
	HMENU menuBar = Alpha::instance().getMainWindow().getMenu()->get();
	if(manah::toBoolean(::IsMenu(menuBar))) {
		pair<py::object, short> popup(findPopupMenu(menuBar, reinterpret_cast<HMENU>(wp), LOWORD(lp)));
		if(popup.first != py::object())
			static_cast<PopupMenu&>(py::extract<PopupMenu&>(popup.first)).update(popup.second);
	}
}

void ambient::ui::handleMENUCOMMAND(WPARAM wp, LPARAM lp) {
	win32::AutoZeroSize<MENUITEMINFOW> mi;
	mi.fMask = MIIM_DATA;
	if(::GetMenuItemInfoW(reinterpret_cast<HMENU>(lp), static_cast<UINT>(wp), true, &mi) != 0) {
		if(PyObject* command = reinterpret_cast<PyObject*>(mi.dwItemData))
			Interpreter::instance().executeCommand(py::object(py::handle<>(py::borrowed(command))));
	}
}

ALPHA_EXPOSE_PROLOGUE(21)
	py::scope temp(ambient::Interpreter::instance().module("ui"));

	py::class_<Menu>("_Menu", py::no_init)
		.def("append", &Menu::append, (py::arg("identifier"), py::arg("caption"), py::arg("command"), py::arg("alternative") = false))
		.def("append_separator", &Menu::appendSeparator)
		.def("check", &Menu::check)
		.def("enable", &Menu::enable)
		.def("erase", &Menu::erase)
		.def("set_child", &Menu::setChild)
		.def("set_default", &Menu::setDefault);
	py::class_<PopupMenu, py::bases<Menu> >("PopupMenu", py::init<py::object>(py::arg("popup_handler") = py::object()));
	py::class_<MenuBar, py::bases<Menu> >("MenuBar")
		.def("set_as_menu_bar", &MenuBar::setAsMenuBar)
		.staticmethod("set_as_menu_bar");
ALPHA_EXPOSE_EPILOGUE()
