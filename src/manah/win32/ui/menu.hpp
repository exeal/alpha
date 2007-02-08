// menu.hpp
// (c) 2003-2007 exeal

#ifndef MANAH_MENU_HPP
#define MANAH_MENU_HPP

#pragma warning(disable : 4297)

#include "../windows.hpp"
#include <commctrl.h>
#include <set>
#include <stdexcept>
#include <memory>

namespace manah {
namespace windows {
namespace ui {


class Menu : public HandleHolder<HMENU> {
public:
	struct ItemInfo : public AutoZero<MENUITEMINFO> {
		ItemInfo() {cbSize = Menu::getSizeOfMENUITEMINFO();}
	};
	enum ItemIdentificationPolicy {BY_COMMAND, BY_POSITION};
	struct StringItem {
		StringItem(UINT id, const TCHAR* text, UINT state = MFS_ENABLED) : id_(id), text_(text), state_(state) {}
		UINT				id_;
		const TCHAR* const	text_;
		UINT				state_;
	};
	struct BitmapItem {
		BitmapItem(UINT id, HBITMAP bitmap, UINT state = MFS_ENABLED) : id_(id), bitmap_(bitmap), state_(state) {}
		UINT	id_;
		HBITMAP	bitmap_;
		UINT	state_;
	};
	struct OwnerDrawnItem {
		explicit OwnerDrawnItem(UINT id, UINT_PTR data = 0, UINT state = MFS_ENABLED) : id_(id), data_(data), state_(state) {}
		UINT		id_;
		UINT_PTR	data_;	// as MEASUREITEMSTRUCT::itemData and DRAWITEMSTRUCT::itemData
		UINT		state_;
	};
	struct SeparatorItem {
		explicit SeparatorItem(UINT flags = MFS_ENABLED) : flags_(flags) {}
		UINT flags_;
	};

	// コンストラクタ
	explicit Menu(bool popup = true);
	explicit Menu(HMENU menu);
	virtual ~Menu();
	// 構築
	static std::auto_ptr<Menu>	load(HINSTANCE instance, const ResourceID& id);
	static std::auto_ptr<Menu>	load(const MENUTEMPLATE* menuTemplate);
	// 属性
	DWORD	getContextHelpID() const;
	UINT	getDefaultMenuItem(UINT flags) const;
	int		getItemCount() const;
	template<ItemIdentificationPolicy idPolicy>
	bool	getMenuItemCaption(UINT item, TCHAR* caption, int maxLength) const;
	template<ItemIdentificationPolicy idPolicy>
	int		getMenuItemCaptionLength(UINT item) const;
	UINT	getMenuItemID(int index) const;
	template<ItemIdentificationPolicy idPolicy>
	bool	getMenuItemInfo(UINT item, MENUITEMINFO& mii) const;
	bool	getMenuItemRect(HWND window, UINT index, RECT& rect) const;
	template<ItemIdentificationPolicy idPolicy>
	UINT	getMenuItemState(UINT item) const;
	HMENU	getSafeHmenu() const;
	Menu	getSubMenu(UINT index) const;
	bool	hasSubMenu(UINT index) const;
	bool	isMenu() const;
	int		menuItemFromPoint(HWND window, const POINT& pt) const;
	bool	setContextHelpID(DWORD id);
#if(WINVER >= 0x0500)
	bool	getMenuInfo(MENUINFO& mi) const;
	bool	setMenuInfo(const MENUINFO& mi);
#endif /* WINVER >= 0x0500 */
	// 操作
	Menu&	operator <<(const StringItem& item);
	Menu&	operator <<(const BitmapItem& item);
	Menu&	operator <<(const OwnerDrawnItem& item);
	Menu&	operator <<(const SeparatorItem& item);
	bool	appendMenuItem(UINT id, const TCHAR* text, UINT flags = MFS_ENABLED);
	bool	appendMenuItem(UINT id, HBITMAP bitmap, UINT flags = MFS_ENABLED);
	bool	appendMenuItem(UINT id, UINT_PTR itemData, UINT flags = MFS_ENABLED);	// for owner-draw menu item
	bool	appendSeparator(UINT flags = MFS_ENABLED);
	template<ItemIdentificationPolicy idPolicy>
	DWORD	checkMenuItem(UINT item, bool check = true);
	template<ItemIdentificationPolicy idPolicy>
	bool	checkMenuRadioItem(UINT firstItem, UINT lastItem, UINT item);
	template<ItemIdentificationPolicy idPolicy>
	bool	deleteMenuItem(UINT item);
	template<ItemIdentificationPolicy idPolicy>
	bool	enableMenuItem(UINT item, bool enable = true);
	template<ItemIdentificationPolicy idPolicy>
	bool	hiliteMenuItem(HWND window, UINT item, bool hilite = true);
	template<ItemIdentificationPolicy idPolicy>
	bool	insertMenuItem(UINT item, const MENUITEMINFO& info);
	template<ItemIdentificationPolicy idPolicy>
	bool	insertMenuItem(UINT item, UINT previousItem, UINT type, UINT state, const TCHAR* caption);
	template<ItemIdentificationPolicy idPolicy>
	bool	insertSeparator(UINT item);
	template<ItemIdentificationPolicy idPolicy>
	bool	modifyMenuItem(UINT item, UINT flags, const TCHAR* prompt);
	template<ItemIdentificationPolicy idPolicy>
	bool	removeMenuItem(UINT item);
	template<ItemIdentificationPolicy idPolicy>
	bool	setChildPopup(UINT item, const Menu& popup, bool delegateOwnership = true);
	template<ItemIdentificationPolicy idPolicy>
	bool	setDefaultMenuItem(UINT item);
	template<ItemIdentificationPolicy idPolicy>
	bool	setMenuItemBitmaps(UINT item, HBITMAP uncheckedBitmap, HBITMAP checkedBitmap);
	template<ItemIdentificationPolicy idPolicy>
	bool	setMenuItemInfo(UINT item, const MENUITEMINFO& info);
	bool	trackPopupMenu(UINT flags, int x, int y, HWND window, const RECT* rect = 0) const;
	bool	trackPopupMenuEx(UINT flags, int x, int y, HWND window, const TPMPARAMS* params = 0) const;
	// ヘルパ
	LRESULT		handleMenuChar(TCHAR charCode, UINT flag);
	static UINT	getSizeOfMENUITEMINFO();

protected:
	virtual void assertValidAsMenu() const {
#ifdef _DEBUG
		assert(isMenu());
#endif
	}

	// データメンバ
private:
	std::set<const Menu*>	children_;				// for deletion
	const bool				managed_;				// if contained menu was created/will be destroyed by this object
	bool					createdByGetSubMenu_;	// if object created by GetSubMenu method
};


inline Menu::Menu(bool popup /* = true */) :
	HandleHolder<HMENU>(popup ? ::CreatePopupMenu() : ::CreateMenu()), managed_(true), createdByGetSubMenu_(false) {}

inline Menu::Menu(HMENU handle) : HandleHolder<HMENU>(handle), managed_(false), createdByGetSubMenu_(false) {
	if(!isMenu()) throw HandleHolder<HMENU>::InvalidHandleException();}

inline Menu::~Menu() {
	if(!createdByGetSubMenu_) {
		while(managed_) {
			const UINT c = getItemCount();
			if(c == -1 || c == 0)
				break;
			::RemoveMenu(get(), 0, MF_BYPOSITION);
		}
		for(std::set<const Menu*>::iterator it = children_.begin(); it != children_.end(); ++it)
			delete *it;
		if(managed_)
			::DestroyMenu(get());
	}
}

inline Menu& Menu::operator <<(const StringItem& item) {appendMenuItem(item.id_, item.text_, item.state_); return *this;}

inline Menu& Menu::operator <<(const BitmapItem& item) {appendMenuItem(item.id_, item.bitmap_, item.state_); return *this;}

inline Menu& Menu::operator <<(const OwnerDrawnItem& item) {appendMenuItem(item.id_, item.data_, item.state_); return *this;}

inline Menu& Menu::operator <<(const SeparatorItem& item) {appendSeparator(item.flags_); return *this;}

inline bool Menu::appendMenuItem(UINT item, const TCHAR* text, UINT state /* = MFS_ENABLED */) {
	assertValidAsMenu(); return toBoolean(::AppendMenu(get(), MFT_STRING | state, item, text));}

inline bool Menu::appendMenuItem(UINT item, HBITMAP bitmap, UINT state /* = MFS_ENABLED */) {
	assertValidAsMenu(); return toBoolean(::AppendMenu(get(), MFT_BITMAP | state, item, reinterpret_cast<TCHAR*>(bitmap)));}

inline bool Menu::appendMenuItem(UINT item, UINT_PTR data, UINT state /* = MFS_ENABLED */) {
	assertValidAsMenu(); return toBoolean(::AppendMenu(get(), MFT_OWNERDRAW | state, item, reinterpret_cast<TCHAR*>(data)));}

inline bool Menu::appendSeparator(UINT flags /* = MFS_ENABLED */) {
	assertValidAsMenu(); return toBoolean(::AppendMenu(get(), (flags | MFT_SEPARATOR) & ~(MFT_BITMAP | MFT_STRING), 0, 0));}

template<> inline DWORD Menu::checkMenuItem<Menu::BY_COMMAND>(UINT item, bool check /* = true */) {
	assertValidAsMenu(); return ::CheckMenuItem(get(), item, MF_BYCOMMAND | (check ? MFS_CHECKED : MFS_UNCHECKED));}

template<> inline DWORD Menu::checkMenuItem<Menu::BY_POSITION>(UINT item, bool check /* = true */) {
	assertValidAsMenu(); return ::CheckMenuItem(get(), item, MF_BYPOSITION | (check ? MFS_CHECKED : MFS_UNCHECKED));}

template<> inline bool Menu::checkMenuRadioItem<Menu::BY_COMMAND>(UINT firstItem, UINT lastItem, UINT item) {
	assertValidAsMenu(); return toBoolean(::CheckMenuRadioItem(get(), firstItem, lastItem, item, MF_BYCOMMAND));}

template<> inline bool Menu::checkMenuRadioItem<Menu::BY_POSITION>(UINT firstItem, UINT lastItem, UINT item) {
	assertValidAsMenu(); return toBoolean(::CheckMenuRadioItem(get(), firstItem, lastItem, item, MF_BYPOSITION));}

template<> inline bool Menu::deleteMenuItem<Menu::BY_COMMAND>(UINT item) {
	assertValidAsMenu(); return toBoolean(::DeleteMenu(get(), item, MF_BYCOMMAND));}

template<> inline bool Menu::deleteMenuItem<Menu::BY_POSITION>(UINT item) {
	assertValidAsMenu(); return toBoolean(::DeleteMenu(get(), item, MF_BYPOSITION));}

template<> inline bool Menu::enableMenuItem<Menu::BY_COMMAND>(UINT item, bool enable /* = true */) {
	assertValidAsMenu(); return toBoolean(::EnableMenuItem(get(), item, MF_BYCOMMAND | (enable ? MFS_ENABLED : MFS_GRAYED)));}

template<> inline bool Menu::enableMenuItem<Menu::BY_POSITION>(UINT item, bool enable /* = true */) {
	assertValidAsMenu(); return toBoolean(::EnableMenuItem(get(), item, MF_BYPOSITION | (enable ? MFS_ENABLED : MFS_GRAYED)));}

inline DWORD Menu::getContextHelpID() const {assertValidAsMenu(); return ::GetMenuContextHelpId(get());}

inline UINT Menu::getDefaultMenuItem(UINT flags) const {assertValidAsMenu(); return ::GetMenuDefaultItem(get(), false, flags);}

template<> inline bool Menu::getMenuItemCaption<Menu::BY_COMMAND>(UINT item, TCHAR* caption, int maxLength) const {
	assertValidAsMenu(); return toBoolean(::GetMenuString(get(), item, caption, maxLength, MF_BYCOMMAND));}

template<> inline bool Menu::getMenuItemCaption<Menu::BY_POSITION>(UINT item, TCHAR* caption, int maxLength) const {
	assertValidAsMenu(); return toBoolean(::GetMenuString(get(), item, caption, maxLength, MF_BYPOSITION));}

template<> inline int Menu::getMenuItemCaptionLength<Menu::BY_COMMAND>(UINT item) const {
	assertValidAsMenu(); return ::GetMenuString(get(), item, 0, 0, MF_BYCOMMAND);}

template<> inline int Menu::getMenuItemCaptionLength<Menu::BY_POSITION>(UINT item) const {
	assertValidAsMenu(); return ::GetMenuString(get(), item, 0, 0, MF_BYPOSITION);}

inline int Menu::getItemCount() const {assertValidAsMenu(); return ::GetMenuItemCount(get());}

inline UINT Menu::getMenuItemID(int index) const {assertValidAsMenu(); return ::GetMenuItemID(get(), index);}

template<> inline bool Menu::getMenuItemInfo<Menu::BY_COMMAND>(UINT item, MENUITEMINFO& info) const {
	assertValidAsMenu(); return toBoolean(::GetMenuItemInfo(get(), item, false, &info));}

template<> inline bool Menu::getMenuItemInfo<Menu::BY_POSITION>(UINT item, MENUITEMINFO& info) const {
	assertValidAsMenu(); return toBoolean(::GetMenuItemInfo(get(), item, true, &info));}

inline bool Menu::getMenuItemRect(HWND window, UINT index, RECT& rect) const {
	assertValidAsMenu(); return toBoolean(::GetMenuItemRect(window, get(), index, &rect));}

template<> inline UINT Menu::getMenuItemState<Menu::BY_COMMAND>(UINT item) const {
	assertValidAsMenu();
	ItemInfo info;
	info.fMask = MIIM_STATE;
	::GetMenuItemInfo(get(), item, false, &info);
	return info.fState;
}

template<> inline UINT Menu::getMenuItemState<Menu::BY_POSITION>(UINT item) const {
	assertValidAsMenu();
	ItemInfo info;
	info.fMask = MIIM_STATE;
	::GetMenuItemInfo(get(), item, true, &info);
	return info.fState;
}

inline HMENU Menu::getSafeHmenu() const {return (this != 0) ? get() : 0;}

inline UINT Menu::getSizeOfMENUITEMINFO() {
	OSVERSIONINFO version;
	version.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	::GetVersionEx(&version);
	return (version.dwMajorVersion >= 5) ? sizeof(MENUITEMINFO) : MENUITEMINFO_SIZE_VERSION_400;
}

inline Menu Menu::getSubMenu(UINT index) const {
	assertValidAsMenu();
	HMENU handle = ::GetSubMenu(get(), index);
	if(handle == 0)
		throw std::invalid_argument("Specified index is out of range or invalid.");
	Menu subMenu(handle);
	subMenu.createdByGetSubMenu_ = true;
	return subMenu;
}

inline LRESULT Menu::handleMenuChar(TCHAR charCode, UINT flag) {
	assertValidAsMenu();

	const UINT c = getItemCount();

	if(charCode >= _T('a') && charCode <= _T('z'))	// make upper
		charCode -= 0x20;
	for(UINT i = 0; i < c; ++i) {
		const int	length = getMenuItemCaptionLength<BY_POSITION>(i);
		TCHAR*		caption = new TCHAR[length + 1];
		getMenuItemCaption<BY_POSITION>(i, caption, length + 1);
		if(const TCHAR* accel = std::_tcschr(caption, charCode)) {
			if(accel[1] == charCode)
				return i | 0x00020000;
		}
	}
	return MNC_IGNORE;
}

inline bool Menu::hasSubMenu(UINT index) const {assertValidAsMenu(); return toBoolean(::IsMenu(::GetSubMenu(get(), index)));}

template<> inline bool Menu::hiliteMenuItem<Menu::BY_COMMAND>(HWND window, UINT item, bool hilite /* = true */) {
	assertValidAsMenu(); return toBoolean(::HiliteMenuItem(window, get(), item, MF_BYCOMMAND | (hilite ? MF_HILITE : MF_UNHILITE)));}

template<> inline bool Menu::hiliteMenuItem<Menu::BY_POSITION>(HWND window, UINT item, bool hilite /* = true */) {
	assertValidAsMenu(); return toBoolean(::HiliteMenuItem(window, get(), item, MF_BYPOSITION | (hilite ? MF_HILITE : MF_UNHILITE)));}

template<> inline bool Menu::insertMenuItem<Menu::BY_COMMAND>(UINT item, const MENUITEMINFO& info) {
	assertValidAsMenu(); return toBoolean(::InsertMenuItem(get(), item, false, &info));}

template<> inline bool Menu::insertMenuItem<Menu::BY_POSITION>(UINT item, const MENUITEMINFO& info) {
	assertValidAsMenu(); return toBoolean(::InsertMenuItem(get(), item, true, &info));}

template<Menu::ItemIdentificationPolicy idPolicy>
inline bool Menu::insertMenuItem(UINT item, UINT previousItem, UINT type, UINT state, const TCHAR* caption) {
	ItemInfo info;
	info.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STATE;
	info.fType = type;
	info.fState = state;
	info.wID = id;
	if(toBoolean(type & MFT_OWNERDRAW) && caption != 0) {
		info.fMask |= MIIM_DATA;
		info.dwItemData = reinterpret_cast<DWORD_PTR>(caption);
	}
	if(caption != 0) {
		info.fMask |= MIIM_STRING;
		info.dwTypeData = const_cast<TCHAR*>(caption);
	}
	return insertMenuItem<idPolicy>(previousItem, info));
}

template<Menu::ItemIdentificationPolicy idPolicy> inline bool Menu::insertSeparator(UINT item) {
	ItemInfo info;
	info.fMask = MIIM_TYPE;
	info.fType = MFT_SEPARATOR;
	return insertMenuItem<idPolicy>(item, info);
}

inline bool Menu::isMenu() const {return toBoolean(::IsMenu(get()));}

inline std::auto_ptr<Menu> Menu::load(HINSTANCE instance, const ResourceID& id) {
	if(HMENU handle = ::LoadMenu(instance, id.name)) {
		Menu* newMenu = new Menu(handle);
		const_cast<bool&>(newMenu->managed_) = true;
		return std::auto_ptr<Menu>(newMenu);
	} else
		return std::auto_ptr<Menu>(0);
}

inline std::auto_ptr<Menu> Menu::load(const MENUTEMPLATE* menuTemplate) {
	if(HMENU handle = ::LoadMenuIndirect(menuTemplate)) {
		Menu* newMenu = new Menu(handle);
		const_cast<bool&>(newMenu->managed_) = true;
		return std::auto_ptr<Menu>(newMenu);
	} else
		return std::auto_ptr<Menu>(0);
}

inline int Menu::menuItemFromPoint(HWND window, const POINT& pt) const {
	assertValidAsMenu(); return ::MenuItemFromPoint(window, get(), pt);}

template<> inline bool Menu::modifyMenuItem<Menu::BY_COMMAND>(UINT item, UINT flags, const TCHAR* caption) {
	assertValidAsMenu();
	assert(!toBoolean(flags & MF_POPUP));
	return toBoolean(::ModifyMenu(get(), item, MF_BYCOMMAND | flags, item, caption));
}

template<> inline bool Menu::modifyMenuItem<Menu::BY_POSITION>(UINT item, UINT flags, const TCHAR* caption) {
	assertValidAsMenu();
	assert(!toBoolean(flags & MF_POPUP));
	return toBoolean(::ModifyMenu(get(), item, MF_BYPOSITION | flags, item, caption));
}

template<> inline bool Menu::removeMenuItem<Menu::BY_COMMAND>(UINT item) {
	assertValidAsMenu(); return toBoolean(::RemoveMenu(get(), item, MF_BYCOMMAND));}

template<> inline bool Menu::removeMenuItem<Menu::BY_POSITION>(UINT item) {
	assertValidAsMenu(); return toBoolean(::RemoveMenu(get(), item, MF_BYPOSITION));}

template<> inline bool Menu::setMenuItemInfo<Menu::BY_COMMAND>(UINT item, const MENUITEMINFO& info) {
	assertValidAsMenu(); return toBoolean(::SetMenuItemInfo(get(), item, false, &info));}

template<> inline bool Menu::setMenuItemInfo<Menu::BY_POSITION>(UINT item, const MENUITEMINFO& info) {
	assertValidAsMenu(); return toBoolean(::SetMenuItemInfo(get(), item, true, &info));}

template<Menu::ItemIdentificationPolicy idPolicy>
inline bool Menu::setChildPopup(UINT item, const Menu& popup, bool delegateOwnership /* = true */) {
	assertValidAsMenu();

	ItemInfo info;
	info.fMask = MIIM_SUBMENU;
	info.hSubMenu = popup.get();
	if(setMenuItemInfo<idPolicy>(item, info)) {
		if(delegateOwnership)
			children_.insert(&popup);
		return true;
	} else
		return false;
}

template<> inline bool Menu::setDefaultMenuItem<Menu::BY_COMMAND>(UINT item) {
	assertValidAsMenu(); return toBoolean(::SetMenuDefaultItem(get(), item, false));}

template<> inline bool Menu::setDefaultMenuItem<Menu::BY_POSITION>(UINT item) {
	assertValidAsMenu(); return toBoolean(::SetMenuDefaultItem(get(), item, true));}

inline bool Menu::setContextHelpID(DWORD id) {assertValidAsMenu(); return toBoolean(::SetMenuContextHelpId(get(), id));}

template<> inline bool Menu::setMenuItemBitmaps<Menu::BY_COMMAND>(UINT item, HBITMAP uncheckedBitmap, HBITMAP checkedBitmap) {
	assertValidAsMenu(); return toBoolean(::SetMenuItemBitmaps(get(), item, MF_BYCOMMAND, uncheckedBitmap, checkedBitmap));}

template<> inline bool Menu::setMenuItemBitmaps<Menu::BY_POSITION>(UINT item, HBITMAP uncheckedBitmap, HBITMAP checkedBitmap) {
	assertValidAsMenu(); return toBoolean(::SetMenuItemBitmaps(get(), item, MF_BYPOSITION, uncheckedBitmap, checkedBitmap));}

inline bool Menu::trackPopupMenu(UINT flags, int x, int y, HWND window, const RECT* rect /* = 0 */) const {
	assertValidAsMenu(); return toBoolean(::TrackPopupMenu(get(), flags, x, y, 0, window, rect));}

inline bool Menu::trackPopupMenuEx(UINT flags, int x, int y, HWND window, const TPMPARAMS* params /* = 0 */) const {
	assertValidAsMenu(); return toBoolean(::TrackPopupMenuEx(get(), flags, x, y, window, const_cast<TPMPARAMS*>(params)));}

#if(WINVER >= 0x0500)
inline bool Menu::getMenuInfo(MENUINFO& mi) const {assertValidAsMenu(); return toBoolean(::GetMenuInfo(get(), &mi));}

inline bool Menu::setMenuInfo(const MENUINFO& mi) {assertValidAsMenu(); return toBoolean(::SetMenuInfo(get(), &mi));}
#endif /* WINVER >= 0x0500 */

}}} // namespace manah::windows::ui

#pragma warning(default : 4297)

#endif /* MANAH_MENU_HPP */
