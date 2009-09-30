/**
 * @file menu.cpp
 * Implements @c Menu class.
 */

#include "application.hpp"
#include "ui.hpp"
#include "input.hpp"
using namespace alpha;
using namespace alpha::ambient;
using namespace manah;
using namespace std;
namespace py = boost::python;


// Menu /////////////////////////////////////////////////////////////////////

namespace {
	class PopupMenu;

	class Menu {
		MANAH_NONCOPYABLE_TAG(Menu);
	public:
		static const short NOT_IDENTIFIED;
	public:
		Menu();
		virtual ~Menu() /*throw()*/;
		py::object append(short identifier, const wstring& caption, py::object command, bool alternative);
		py::object appendSeparator();
		wstring caption(short identifier) const;
		py::object check(short identifier, bool check);
		py::object clear();
		py::object command(short identifier) const;
		short defaultItem() const;
		py::object enable(short identifier, bool enable);
		py::object erase(short identifier);
		py::ssize_t find(short identifier) const;
		short identifier(py::ssize_t position) const;
		py::object insert(short at, short identifier, const wstring& caption, py::object command, bool alternative);
		bool isAlternative(short identifier) const;
		bool isChecked(short identifier) const;
		bool isDisabled(short identifier) const;
		bool isSeparator(short identifier) const;
		py::ssize_t numberOfItems() const;
		py::object setAlternative(short identifier, bool alternative);
		py::object setCaption(short identifier, const wstring& caption);
		py::object setChild(short identifier, py::object child);
		py::object setCommand(short identifier, py::object command);
		py::object setDefault(short identifier);
		py::object subMenu(short identifier) const;
	protected:
		explicit Menu(HMENU handle);
		HMENU handle() const;
		py::object self();
		static void raiseItemNotFound();
	private:
		py::object insertItem(short at, short identifier, const wstring& caption, py::object command, bool alternative);
		void item(short identifier, win32::AutoZeroSize<MENUITEMINFOW>& mi) const;
		py::object setItem(short identifier, const win32::AutoZeroSize<MENUITEMINFOW>& mi);
		UINT itemState(short identifier) const;
		UINT itemType(short identifier) const;
		py::object setItemState(short identifier, UINT statesToAdd, UINT statesToRemove);
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
		MANAH_NONCOPYABLE_TAG(MenuBar);
	public:
		MenuBar();
		static py::object setAsMenuBar(py::object newMenuBar);
	};
} // namespace @0

const short Menu::NOT_IDENTIFIED = numeric_limits<short>::min();

Menu::Menu(HMENU handle) : handle_(handle) {
	if(!toBoolean(::IsMenu(handle)))
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
		while(numberOfItems() > 0) {
			if(toBoolean(::GetMenuItemInfoW(handle_, 0, true, &mi)))
				Py_XDECREF(reinterpret_cast<PyObject*>(mi.dwItemData));
			::RemoveMenu(handle_, 0, MF_BYPOSITION);
		}
		::DestroyMenu(handle_);
	}
}

py::object Menu::append(short identifier, const wstring& caption, py::object command, bool alternative) {
	return insertItem(NOT_IDENTIFIED, identifier, caption, command, alternative);
}

py::object Menu::appendSeparator() {
	win32::AutoZeroSize<MENUITEMINFOW> item;
	item.fMask = MIIM_TYPE;
	item.fType = MFT_SEPARATOR;
	if(!toBoolean(::InsertMenuItemW(handle_, static_cast<UINT>(numberOfItems()), true, &item)))
		Interpreter::instance().raiseLastWin32Error();
	return self();
}

py::object Menu::clear() {
	// TODO: this code is not exception-safe and can interrupt.
	while(numberOfItems() > 0)
		erase(identifier(0));
	return self();
}

wstring Menu::caption(short identifier) const {
	win32::AutoZeroSize<MENUITEMINFOW> mi;
	mi.fMask = MIIM_STRING;
	item(identifier, mi);
	AutoBuffer<WCHAR> s(new WCHAR[++mi.cch]);
	mi.dwTypeData = s.get();
	item(identifier, mi);
	return wstring(mi.dwTypeData);
}

py::object Menu::check(short identifier, bool check) {
	return setItemState(identifier, check ? MFS_CHECKED : MFS_UNCHECKED, check ? MFS_UNCHECKED : MFS_CHECKED);
}

py::object Menu::command(short identifier) const {
	win32::AutoZeroSize<MENUITEMINFOW> mi;
	mi.fMask = MIIM_DATA;
	item(identifier, mi);
	PyObject* const p = reinterpret_cast<PyObject*>(mi.dwItemData);
	if(p != 0) {
		Py_INCREF(p);
		return py::object(py::handle<>(p));
	}
	return py::object();
}

short Menu::defaultItem() const {
	const UINT i = ::GetMenuDefaultItem(handle_, false, GMDI_USEDISABLED);
	return (i != -1) ? static_cast<short>(i) : NOT_IDENTIFIED;
}

py::object Menu::enable(short identifier, bool enable) {
	return setItemState(identifier, enable ? MFS_ENABLED : (MFS_DISABLED | MFS_GRAYED), enable ? (MFS_DISABLED | MFS_GRAYED) : MFS_ENABLED);
}

py::object Menu::erase(short identifier) {
	win32::AutoZeroSize<MENUITEMINFOW> mi;
	mi.fMask = MIIM_DATA;
	item(identifier, mi);
	for(set<py::object>::iterator i(children_.begin()), e(children_.end()); i != e; ++i) {
		if(i->ptr() == reinterpret_cast<PyObject*>(mi.dwItemData)) {
			children_.erase(i);
			break;
		}
	}
	if(!toBoolean(::RemoveMenu(handle_, identifier, MF_BYCOMMAND)))
		Interpreter::instance().raiseLastWin32Error();
	return self();
}

py::ssize_t Menu::find(short identifier) const {
	for(py::ssize_t i = 0, c = numberOfItems(); i < c; ++i) {
		if(this->identifier(i) == identifier)
			return i;
	}
	return -1;
}

inline HMENU Menu::handle() const {
	return handle_;
}

short Menu::identifier(py::ssize_t position) const {
	if(position >= numberOfItems()) {
		::PyErr_SetString(PyExc_IndexError, "the specified position is out of range.");
		py::throw_error_already_set();
	}
	return ::GetMenuItemID(handle_, static_cast<int>(position));
}

py::object Menu::insert(short at, short identifier, const wstring& caption, py::object command, bool alternative) {
	if(at == NOT_IDENTIFIED || find(at) == -1) {
		::PyErr_SetString(PyExc_IndexError, "the given position is invalid.");
		py::throw_error_already_set();
	}
	return insertItem(at, identifier, caption, command, alternative);
}

py::object Menu::insertItem(short at, short identifier, const wstring& caption, py::object command, bool alternative) {
	if(caption.empty()) {
		::PyErr_SetString(PyExc_ValueError, "the caption string is empty.");
		py::throw_error_already_set();
	} else if(command != py::object() && !toBoolean(::PyCallable_Check(command.ptr()))) {
		::PyErr_SetString(PyExc_TypeError, "the command argument is not callable.");
		py::throw_error_already_set();
	}

	const bool append = at == NOT_IDENTIFIED;
	win32::AutoZeroSize<MENUITEMINFOW> item;
	item.fMask = MIIM_DATA | MIIM_FTYPE | MIIM_ID | MIIM_STATE | MIIM_STRING;
	item.fType = MFT_STRING | (alternative ? MFT_RADIOCHECK : 0);
	item.fState = MFS_ENABLED | MFS_UNCHECKED;
	item.wID = identifier;
	item.dwItemData = reinterpret_cast<UINT_PTR>(command.ptr());
	item.dwTypeData = const_cast<WCHAR*>(caption.c_str());
	if(::InsertMenuItemW(handle_, append ? static_cast<UINT>(numberOfItems()) : at, append, &item) == 0)
		Interpreter::instance().raiseLastWin32Error();
	Py_XINCREF(reinterpret_cast<PyObject*>(item.dwItemData));
	return self();
}

bool Menu::isAlternative(short identifier) const {
	return toBoolean(itemType(identifier) & MFT_RADIOCHECK);
}

bool Menu::isChecked(short identifier) const {
	return toBoolean(itemState(identifier) & MFS_CHECKED);
}

bool Menu::isDisabled(short identifier) const {
	return toBoolean(itemState(identifier) & (MFS_DISABLED));
}

bool Menu::isSeparator(short identifier) const {
	return toBoolean(itemType(identifier) & MFT_SEPARATOR);
}

inline void Menu::item(short identifier, win32::AutoZeroSize<MENUITEMINFOW>& mi) const {
	if(!toBoolean(::GetMenuItemInfoW(handle_, identifier, false, &mi)))
		Interpreter::instance().raiseLastWin32Error();
}

inline UINT Menu::itemState(short identifier) const {
	win32::AutoZeroSize<MENUITEMINFOW> mi;
	mi.fMask = MIIM_STATE;
	item(identifier, mi);
	return mi.fState;
}

inline UINT Menu::itemType(short identifier) const {
	win32::AutoZeroSize<MENUITEMINFOW> mi;
	mi.fMask = MIIM_FTYPE;
	item(identifier, mi);
	return mi.fType;
}

py::ssize_t Menu::numberOfItems() const {
	const int c = ::GetMenuItemCount(handle_);
	if(c == -1) {
		const DWORD e = ::GetLastError();
		Interpreter::instance().raiseLastWin32Error();
	}
	return c;

}

void Menu::raiseItemNotFound() {
	::PyErr_SetString(PyExc_KeyError, "the specified item is not found.");
	py::throw_error_already_set();
}

py::object Menu::self() {
	if(self_ == py::object())
		self_ = py::object(py::ptr(this));
	return self_;
}

py::object Menu::setAlternative(short identifier, bool alternative) {
	win32::AutoZeroSize<MENUITEMINFOW> mi;
	mi.fMask = MIIM_FTYPE;
	item(identifier, mi);
	if(alternative)
		mi.fType |= MFT_RADIOCHECK;
	else
		mi.fType &= ~MFT_RADIOCHECK;
	return setItem(identifier, mi);
}

py::object Menu::setCaption(short identifier, const wstring& caption) {
	if(caption.empty()) {
		::PyErr_SetString(PyExc_ValueError, "the caption string is empty.");
		py::throw_error_already_set();
	}
	win32::AutoZeroSize<MENUITEMINFOW> mi;
	mi.fMask = MIIM_STRING;
	mi.dwTypeData = const_cast<WCHAR*>(caption.c_str());
	return setItem(identifier, mi);
}

py::object Menu::setChild(short identifier, py::object child) {
	if(children_.find(child) != children_.end()) {
		::PyErr_BadArgument();
		py::throw_error_already_set();
	}
	HMENU childHandle = static_cast<Menu&>(py::extract<Menu&>(child)).handle();
	win32::AutoZeroSize<MENUITEMINFOW> mi;
	mi.fMask = MIIM_DATA | MIIM_SUBMENU;
	mi.dwItemData = reinterpret_cast<ULONG_PTR>(child.ptr());
	mi.hSubMenu = childHandle;
	setItem(identifier, mi);
	children_.insert(child);
	return self();
}

py::object Menu::setCommand(short identifier, py::object command) {
	if(command != py::object() && !toBoolean(::PyCallable_Check(command.ptr()))) {
		::PyErr_SetString(PyExc_TypeError, "the command argument is not callable.");
		py::throw_error_already_set();
	}
	win32::AutoZeroSize<MENUITEMINFOW> mi;
	mi.fMask = MIIM_DATA;
	item(identifier, mi);
	Py_XDECREF(reinterpret_cast<PyObject*>(mi.dwItemData));
	mi.dwItemData = reinterpret_cast<UINT_PTR>(command.ptr());
	setItem(identifier, mi);
	Py_XINCREF(reinterpret_cast<PyObject*>(mi.dwItemData));
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

py::object Menu::setItem(short identifier, const win32::AutoZeroSize<MENUITEMINFOW>& mi) {
	if(!toBoolean(::SetMenuItemInfoW(handle_, identifier, false, &mi)))
		Interpreter::instance().raiseLastWin32Error();
	return self();
}

py::object Menu::setItemState(short identifier, UINT statesToAdd, UINT statesToRemove) {
	win32::AutoZeroSize<MENUITEMINFOW> mi;
	mi.fMask = MIIM_STATE;
	item(identifier, mi);
	mi.fState &= ~statesToRemove;
	mi.fState |= statesToAdd;
	return setItem(identifier, mi);
}

py::object Menu::subMenu(short identifier) const {
	win32::AutoZeroSize<MENUITEMINFOW> item;
	item.fMask = MIIM_SUBMENU;
	if(!toBoolean(::GetMenuItemInfoW(handle_, identifier, false, &item)))
		Interpreter::instance().raiseLastWin32Error();
	if(item.hSubMenu != 0) {
		for(set<py::object>::const_iterator i(children_.begin()), e(children_.end()); i != e; ++i) {
			if(static_cast<const Menu&>(py::extract<const Menu&>(*i)).handle_ == item.hSubMenu)
				return *i;
		}
	}
	return py::object();
}


// MenuBar //////////////////////////////////////////////////////////////////

MenuBar::MenuBar() : Menu(::CreateMenu()) {
}

py::object MenuBar::setAsMenuBar(py::object newMenuBar) {
	static py::object singletonHolder;
	if(!Alpha::instance().getMainWindow().setMenu(static_cast<MenuBar&>(py::extract<MenuBar&>(newMenuBar)).handle()))
		Interpreter::instance().raiseLastWin32Error();
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
	::OutputDebugStringW(L"XXXX\n");
	if(popupHandler_ != py::object()) {
		try {
			popupHandler_(identifier, self());
		} catch(py::error_already_set&) {
			Interpreter::instance().handleException();
		}
	}

	// show bound input sequences
	py::object schemeObject(ui::InputManager::instance().mappingScheme());
	if(schemeObject != py::object()) {
		const ui::InputMappingScheme& scheme = py::extract<ui::InputMappingScheme&>(schemeObject);
		for(ssize_t i = 0, c = numberOfItems(); i < c; ++i) {
			const short id = this->identifier(i);
			if(id == -1)
				continue;
			py::object f(command(id));
			if(f != py::object()) {
				py::object s(scheme.inputSequencesForCommand(f));
				wstring inputSequence;
				if(py::len(s) != 0) {
					py::object i(py::handle<>(::PyObject_GetIter(s.ptr())));
					py::object k(py::handle<>(::PyIter_Next(i.ptr())));
					inputSequence = ui::KeyStroke::format(k);
				}
				const wstring oldCaption(caption(id));
				wstring newCaption(oldCaption);
				const size_t tab = oldCaption.find(L'\t');
				if(tab == wstring::npos) {
					if(!inputSequence.empty()) {
						newCaption += L'\t';
						newCaption += inputSequence;
					}
				} else if(inputSequence.empty())
					newCaption.erase(tab);
				else {
					newCaption.replace(tab, wstring::npos, L"\t");
					newCaption += inputSequence;
				}
				if(newCaption != oldCaption)
					setCaption(id, newCaption);
			}
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

void ui::handleINITMENUPOPUP(WPARAM wp, LPARAM lp) {
	if(HIWORD(lp) != 0)
		return;	// system menu
	HMENU menuBar = Alpha::instance().getMainWindow().getMenu()->get();
	if(manah::toBoolean(::IsMenu(menuBar))) {
		pair<py::object, short> popup(findPopupMenu(menuBar, reinterpret_cast<HMENU>(wp), LOWORD(lp)));
		if(popup.first != py::object())
			static_cast<PopupMenu&>(py::extract<PopupMenu&>(popup.first)).update(popup.second);
	}
}

void ui::handleMENUCOMMAND(WPARAM wp, LPARAM lp) {
	win32::AutoZeroSize<MENUITEMINFOW> mi;
	mi.fMask = MIIM_DATA;
	if(::GetMenuItemInfoW(reinterpret_cast<HMENU>(lp), static_cast<UINT>(wp), true, &mi) != 0) {
		if(PyObject* command = reinterpret_cast<PyObject*>(mi.dwItemData))
			Interpreter::instance().executeCommand(py::object(py::handle<>(py::borrowed(command))));
	}
}

void ui::handleMENUSELECT(WPARAM wp, LPARAM lp) {
	// TODO: show the description of the selected command on the status bar.
}

ALPHA_EXPOSE_PROLOGUE(21)
	py::scope temp(ambient::Interpreter::instance().module("ui"));

	py::class_<Menu, boost::noncopyable>("_Menu", py::no_init)
		.add_property("default", &Menu::defaultItem)
		.add_property("number_of_items", &Menu::numberOfItems)
		.def("append", &Menu::append, (py::arg("identifier"), py::arg("caption"), py::arg("command"), py::arg("alternative") = false))
		.def("append_separator", &Menu::appendSeparator)
		.def("caption", &Menu::caption)
		.def("check", &Menu::check)
		.def("clear", &Menu::clear)
		.def("enable", &Menu::enable)
		.def("erase", &Menu::erase)
		.def("find", &Menu::find)
		.def("identifier", &Menu::identifier)
		.def("insert", &Menu::insert, (py::arg("at"), py::arg("identifier"), py::arg("caption"), py::arg("command"), py::arg("alternative") = false))
		.def("is_alternative", &Menu::isAlternative)
		.def("is_checked", &Menu::isChecked)
		.def("is_disabled", &Menu::isDisabled)
		.def("is_separator", &Menu::isSeparator)
		.def("set_alternative", &Menu::setAlternative)
		.def("set_caption", &Menu::setCaption)
		.def("set_child", &Menu::setChild)
		.def("set_command", &Menu::setCommand)
		.def("set_default", &Menu::setDefault)
		.def("sub_menu", &Menu::subMenu);
	py::class_<PopupMenu, py::bases<Menu>, boost::noncopyable>("PopupMenu", py::init<py::object>(py::arg("popup_handler") = py::object()));
	py::class_<MenuBar, py::bases<Menu>, boost::noncopyable>("MenuBar")
		.def("set_as_menu_bar", &MenuBar::setAsMenuBar)
		.staticmethod("set_as_menu_bar");
ALPHA_EXPOSE_EPILOGUE()
