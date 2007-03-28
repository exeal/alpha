// menu.hpp
// (c) 2003-2007 exeal

#ifndef MANAH_MENU_HPP
#define MANAH_MENU_HPP

#pragma warning(disable : 4297)

#include "../dc.hpp"
#include "../../memory.hpp"
#include <commctrl.h>
#include <set>
#include <stdexcept>
#include <memory>

namespace manah {
namespace win32 {
namespace ui {


// caution! this class does not support Win 95/NT.
class Menu : public Handle<HMENU, ::DestroyMenu> {
public:
	struct ItemInfo : public AutoZero<::MENUITEMINFO> {
		ItemInfo() {cbSize = Menu::getSizeOfMENUITEMINFO();}
	};
	enum ItemIdentificationPolicy {BY_COMMAND, BY_POSITION};
	struct StringItem : public ItemInfo {
		StringItem(UINT id, const TCHAR* text, UINT state = MFS_ENABLED, bool radioCheck = false, ULONG_PTR data = 0) {
			fMask = MIIM_DATA | MIIM_ID | MIIM_STATE | MIIM_STRING | MIIM_TYPE;
			fType = MFT_STRING | (radioCheck ? MFT_RADIOCHECK : 0);
			fState = state;
			wID = id;
			dwItemData = data;
			dwTypeData = const_cast<TCHAR*>(text);
		}
	};
	struct BitmapItem : public ItemInfo {
		BitmapItem(UINT id, HBITMAP bitmap, UINT state = MFS_ENABLED, ULONG_PTR data = 0) {
			fMask = MIIM_BITMAP | MIIM_DATA | MIIM_ID | MIIM_STATE | MIIM_TYPE;
			fType = MFT_BITMAP;
			fState = state;
			wID = id;
			dwItemData = data;
			hbmpItem = bitmap;
		}
	};
	struct OwnerDrawnItem : public ItemInfo {
		explicit OwnerDrawnItem(UINT id, UINT state = MFS_ENABLED, UINT_PTR data = 0) {
			fMask = MIIM_DATA | MIIM_ID | MIIM_STATE | MIIM_TYPE;
			fType = MFT_OWNERDRAW;
			fState = state;
			wID = id;
			dwItemData = data;
		}
	};
	struct SeparatorItem : public ItemInfo {
		explicit SeparatorItem(bool ownerDraw = false) {
			fMask = MIIM_TYPE;
			fType = MFT_SEPARATOR;
			if(ownerDraw) fType |= MFT_OWNERDRAW;
		}
	};

	// constructors
	explicit Menu(HMENU menu = 0);
	virtual ~Menu();
	// constructions
	static std::auto_ptr<Menu>	load(HINSTANCE instance, const ResourceID& id);
	static std::auto_ptr<Menu>	load(const ::MENUTEMPLATE* menuTemplate);
	// attributes
	DWORD	getContextHelpID() const;
	UINT	getDefault(UINT flags) const;
	template<ItemIdentificationPolicy idPolicy>
	bool	getCaption(UINT item, TCHAR* caption, int maxLength) const;
	template<ItemIdentificationPolicy idPolicy>
	int		getCaptionLength(UINT item) const;
	UINT	getID(int index) const;
	template<ItemIdentificationPolicy idPolicy>
	bool	getItemInformation(UINT item, ::MENUITEMINFO& mii) const;
	bool	getRect(HWND window, UINT index, ::RECT& rect) const;
	int		getNumberOfItems() const;
	template<ItemIdentificationPolicy idPolicy>
	UINT	getState(UINT item) const;
	Menu	getSubMenu(UINT index) const;
	bool	hasSubMenu(UINT index) const;
	bool	isMenu() const;
	int		itemFromPoint(HWND window, const ::POINT& pt) const;
	bool	setContextHelpID(DWORD id);
	template<ItemIdentificationPolicy idPolicy>
	bool	setState(UINT item, UINT state);
#if(WINVER >= 0x0500)
	bool	getInformation(::MENUINFO& mi) const;
	bool	setInformation(const ::MENUINFO& mi);
#endif /* WINVER >= 0x0500 */
	// operations
	Menu&	operator<<(const ::MENUITEMINFO& item);
	bool	append(const ::MENUITEMINFO& item);
	template<ItemIdentificationPolicy idPolicy>
	DWORD	check(UINT item, bool check = true);
	template<ItemIdentificationPolicy idPolicy>
	bool	check(UINT firstItem, UINT lastItem, UINT item);
	template<ItemIdentificationPolicy idPolicy>
	bool	erase(UINT item);
	template<ItemIdentificationPolicy idPolicy>
	bool	enable(UINT item, bool enable = true);
	template<ItemIdentificationPolicy idPolicy>
	bool	hilite(HWND window, UINT item, bool hilite = true);
	template<ItemIdentificationPolicy idPolicy>
	bool	insert(UINT item, const ::MENUITEMINFO& info);
	template<ItemIdentificationPolicy idPolicy>
	bool	insert(UINT item, UINT previousItem, UINT type, UINT state, const TCHAR* caption);
	template<ItemIdentificationPolicy idPolicy>
	bool	insertSeparator(UINT item);
	template<ItemIdentificationPolicy idPolicy>
	bool	remove(UINT item);
	template<ItemIdentificationPolicy idPolicy>
	bool	setChildPopup(UINT item, const Menu& popup);
	template<ItemIdentificationPolicy idPolicy>
	bool	setChildPopup(UINT item, std::auto_ptr<Menu> popup);
	template<ItemIdentificationPolicy idPolicy>
	bool	setChildPopup(UINT item, HMENU popup, bool delegateOwnership);
	template<ItemIdentificationPolicy idPolicy>
	bool	setDefault(UINT item);
	template<ItemIdentificationPolicy idPolicy>
	bool	setBitmaps(UINT item, HBITMAP uncheckedBitmap, HBITMAP checkedBitmap);
	template<ItemIdentificationPolicy idPolicy>
	bool	setItemInformation(UINT item, const ::MENUITEMINFO& info);
	bool	trackPopup(UINT flags, int x, int y, HWND window, const RECT* rect = 0) const;
	bool	trackPopupEx(UINT flags, int x, int y, HWND window, const ::TPMPARAMS* params = 0) const;
	// owner draw
	static LRESULT	drawItem(const ::DRAWITEMSTRUCT& di, const TCHAR* text,
						const TCHAR* accelerator = 0, const HIMAGELIST icons = 0, int iconIndex = 0, HICON icon = 0);
	LRESULT			handleMenuChar(TCHAR charCode, UINT flag);
	static LRESULT	measureItem(::MEASUREITEMSTRUCT& mi, const TCHAR* text, const TCHAR* accelerator = 0);
	// misc
	static UINT	getSizeOfMENUITEMINFO();

protected:
	virtual void assertValidAsMenu() const {
#ifdef _DEBUG
		assert(isMenu());
#endif
	}
private:
	enum {TEXT_MARGIN = 2, BUTTON_GAP = 1};
	std::set<HMENU> managedChildren_;	// for ownership
};

class MenuBar : public Menu {
public:
	MenuBar() {reset(::CreateMenu());}
};

class PopupMenu : public Menu {
public:
	PopupMenu() {reset(::CreatePopupMenu());}
};


inline Menu::Menu(HMENU handle /* = 0*/) : Handle<HMENU, ::DestroyMenu>(handle) {
	if(getHandle() != 0 && !isMenu()) throw std::invalid_argument("the handle is not a menu.");}

inline Menu::~Menu() {
	if(isMenu() && !isAttached()) {
		// avoid auto destruction
		const UINT c = getNumberOfItems();
		for(UINT i = 0; i < c; ) {
			HMENU subMenu = ::GetSubMenu(getHandle(), i);
			if(managedChildren_.find(subMenu) != managedChildren_.end())
				::RemoveMenu(getHandle(), i, MF_BYPOSITION);
			else
				++i;
		}
	}
}

inline Menu& Menu::operator<<(const ::MENUITEMINFO& item) {append(item); return *this;}

inline bool Menu::append(const ::MENUITEMINFO& item) {return insert<BY_POSITION>(getNumberOfItems(), item);}

template<Menu::ItemIdentificationPolicy idPolicy> inline DWORD Menu::check(UINT item, bool check /* = true */) {
	UINT state = getState<idPolicy>(item); state &= ~(MFS_CHECKED | MFS_UNCHECKED); return setState<idPolicy>(item, state | (check ? MFS_CHECKED : MFS_UNCHECKED));}

template<Menu::ItemIdentificationPolicy idPolicy> inline bool Menu::check(UINT firstItem, UINT lastItem, UINT item) {
	assertValidAsMenu(); return toBoolean(::CheckMenuRadioItem(getHandle(), firstItem, lastItem, item, idPolicy == BY_COMMAND ? MF_BYCOMMAND : MF_BYPOSITION));}

inline LRESULT Menu::drawItem(const ::DRAWITEMSTRUCT& di, const TCHAR* text,
		const TCHAR* accelerator /* = 0 */, HIMAGELIST icons /* = 0 */, int iconIndex /* = 0 */, HICON icon /* = 0 */) {
	if(di.CtlType != ODT_MENU)
		return false;
	assert(icons == 0 || iconIndex < ::ImageList_GetImageCount(icons));
	gdi::DC dc;
	dc.attach(di.hDC);
	const bool selected = toBoolean(di.itemState & ODS_SELECTED);
	const bool checked = toBoolean(di.itemState & ODS_CHECKED);
	const bool disabled = toBoolean(di.itemState & ODS_GRAYED);

	// detect menu is flat (only XP or later)
#ifndef SPI_GETFLATMENU
	const UINT SPI_GETFLATMENU = 0x1022;
#endif
	BOOL flat = false;
	::SystemParametersInfo(SPI_GETFLATMENU, 0, &flat, 0);

	// draw background
	if(selected) {
		if(flat) {
#ifndef COLOR_MENUHILIGHT
			const int COLOR_MENUHILIGHT = 29;
#endif
			dc.fillRect(di.rcItem, ::GetSysColorBrush(COLOR_MENUHILIGHT));
			dc.frameRect(di.rcItem, ::GetSysColorBrush(COLOR_HIGHLIGHT));
		} else
			dc.fillRect(di.rcItem, ::GetSysColorBrush(COLOR_HIGHLIGHT));
	} else if(di.itemAction == ODA_SELECT)
		dc.fillRect(di.rcItem, ::GetSysColorBrush(COLOR_MENU));

	// separator
	if(text == 0) {
		::RECT rc = di.rcItem;
		rc.top += (rc.bottom - rc.top) / 2;
		dc.drawEdge(rc, EDGE_ETCHED, BF_TOP);
		return true;
	}

	// draw icon (if presented)
	int iconY, iconCx, iconCy;
	if(icons != 0) {
		::ImageList_GetIconSize(icons, &iconCx, &iconCy);
		iconY = (di.rcItem.bottom + di.rcItem.top) / 2 - iconCy / 2;
		::ImageList_DrawEx(icons, iconIndex, dc.getHandle(), di.rcItem.left + 2, iconY,
			0, 0, (selected && !checked) ? CLR_NONE : ::GetSysColor(COLOR_MENU), CLR_NONE, ILD_NORMAL);
	} else if(icon != 0) {
		iconCx = ::GetSystemMetrics(SM_CXSMICON);
		iconCy = ::GetSystemMetrics(SM_CYSMICON);
		iconY = (di.rcItem.bottom + di.rcItem.top) / 2 - iconCy / 2;
		if(checked)
			dc.fillSolidRect(di.rcItem.left + 2, iconY, iconCx, iconCy, ::GetSysColor(COLOR_MENU));
		dc.drawIconEx(di.rcItem.left + 2, iconY, icon, 0, 0, 0, 0, DI_NORMAL | DI_NOMIRROR);
	}

	// draw checkmark
	if(checked) {
		if(icons != 0 || icon != 0) {
			::RECT buttonRect;
			buttonRect.left = di.rcItem.left + 1;
			buttonRect.top = iconY - 1;
			buttonRect.right = buttonRect.left + iconCx + 1;
			buttonRect.bottom = buttonRect.top + iconCy + 1;
//			if(flat)
				dc.frameRect(buttonRect, ::GetSysColorBrush(COLOR_HIGHLIGHT));
//			else {
//			}
		} else {
			const int size = di.rcItem.bottom - di.rcItem.top - 4;
			HPEN pen = ::CreatePen(PS_SOLID, 1, ::GetSysColor(COLOR_MENUTEXT));
			HPEN oldPen = dc.selectObject(pen);
			dc.moveTo(di.rcItem.left + 2 + size / 2 - 3, di.rcItem.top + 2 + size / 2 - 1);
			dc.lineTo(di.rcItem.left + 2 + size / 2 - 1, di.rcItem.top + 2 + size / 2 + 1);
			dc.lineTo(di.rcItem.left + 2 + size / 2 + 4, di.rcItem.top + 2 + size / 2 - 4);
			dc.moveTo(di.rcItem.left + 2 + size / 2 - 3, di.rcItem.top + 2 + size / 2 + 0);
			dc.lineTo(di.rcItem.left + 2 + size / 2 - 1, di.rcItem.top + 2 + size / 2 + 2);
			dc.lineTo(di.rcItem.left + 2 + size / 2 + 4, di.rcItem.top + 2 + size / 2 - 3);
			dc.selectObject(oldPen);
			::DeleteObject(pen);
		}
	}

	// draw text
	dc.setTextColor(::GetSysColor(
		toBoolean(di.itemState & (ODS_DISABLED | ODS_GRAYED)) ? COLOR_GRAYTEXT : (selected ? COLOR_HIGHLIGHTTEXT : COLOR_MENUTEXT)));
	dc.setBkMode(TRANSPARENT);
	::RECT rc = di.rcItem;
	rc.left += rc.bottom - rc.top + 4;
	dc.drawText(text, -1, rc, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
	rc.right -= rc.bottom - rc.top;
	dc.drawText(accelerator, -1, rc, DT_RIGHT | DT_SINGLELINE | DT_VCENTER);

	return true;
}

template<Menu::ItemIdentificationPolicy idPolicy> inline bool Menu::enable(UINT item, bool enable /* = true */) {
	assertValidAsMenu(); return toBoolean(::EnableMenuItem(getHandle(), item, (idPolicy == BY_COMMAND ? MF_BYCOMMAND : MF_BYPOSITION) | (enable ? MFS_ENABLED : MFS_GRAYED)));}

template<Menu::ItemIdentificationPolicy idPolicy> inline bool Menu::erase(UINT item) {
	assertValidAsMenu(); return toBoolean(::DeleteMenu(getHandle(), item, (idPolicy == BY_COMMAND) ? MF_BYCOMMAND : MF_BYPOSITION));}

inline DWORD Menu::getContextHelpID() const {assertValidAsMenu(); return ::GetMenuContextHelpId(getHandle());}

inline UINT Menu::getDefault(UINT flags) const {assertValidAsMenu(); return ::GetMenuDefaultItem(getHandle(), false, flags);}

template<Menu::ItemIdentificationPolicy idPolicy> inline bool Menu::getCaption(UINT item, TCHAR* caption, int maxLength) const {
	ItemInfo mi; mi.fMask = MIIM_STRING; mi.dwTypeData = caption; return getItemInformation<idPolicy>(item, mi);}

template<Menu::ItemIdentificationPolicy idPolicy> inline int Menu::getCaptionLength(UINT item) const {
	ItemInfo mi; mi.fMask = MIIM_STRING; getItemInformation<idPolicy>(item, mi); return mi.cch;}

inline int Menu::getNumberOfItems() const {assertValidAsMenu(); return ::GetMenuItemCount(getHandle());}

inline UINT Menu::getID(int index) const {assertValidAsMenu(); return ::GetMenuItemID(getHandle(), index);}

template<Menu::ItemIdentificationPolicy idPolicy> inline bool Menu::getItemInformation(UINT item, ::MENUITEMINFO& info) const {
	assertValidAsMenu(); return toBoolean(::GetMenuItemInfo(getHandle(), item, idPolicy == Menu::BY_POSITION, &info));}

inline bool Menu::getRect(HWND window, UINT index, ::RECT& rect) const {
	assertValidAsMenu(); return toBoolean(::GetMenuItemRect(window, getHandle(), index, &rect));}

inline UINT Menu::getSizeOfMENUITEMINFO() {
	::OSVERSIONINFO version;
	version.dwOSVersionInfoSize = sizeof(::OSVERSIONINFO);
	::GetVersionEx(&version);
	return (version.dwMajorVersion >= 5) ? sizeof(::MENUITEMINFO) : MENUITEMINFO_SIZE_VERSION_400;
}

template<Menu::ItemIdentificationPolicy idPolicy> inline UINT Menu::getState(UINT item) const {
	ItemInfo mi; getItemInformation<idPolicy>(item, mi); return mi.fState;}

inline Menu Menu::getSubMenu(UINT index) const {
	assertValidAsMenu();
	HMENU handle = ::GetSubMenu(getHandle(), index);
	if(handle == 0)
		throw std::invalid_argument("Specified index is out of range or invalid.");
	return Menu(handle);
}

inline LRESULT Menu::handleMenuChar(TCHAR charCode, UINT flag) {
	assertValidAsMenu();
	const UINT c = getNumberOfItems();
//	// search selected item
//	for(activeItem = 0; activeItem < c; ++activeItem) {
//		if(toBoolean(getState<BY_POSITION>(activeItem) & MFS_HILITE))
//			break;
//	}

	charCode = static_cast<TCHAR>(LOWORD(::CharLower(reinterpret_cast<TCHAR*>(charCode))));	// fold case

	UINT i;
	for(i = /*(activeItem != c) ? activeItem + 1 :*/ 0; i < c; ++i) {
		const int len = getCaptionLength<BY_POSITION>(i);
		AutoBuffer<TCHAR> caption(new TCHAR[len + 1]);
		getCaption<BY_POSITION>(i, caption.get(), len + 1);
		// search '&'
		const TCHAR* p = caption.get();
		while(*p != 0 && *p != _T('&'))
			::CharNext(caption.get());
		if(*p != 0) {
			if(charCode == static_cast<TCHAR>(LOWORD(::CharLower(reinterpret_cast<TCHAR*>(p[1])))))
				break;
		}
	}
	return (i != c) ? MAKELONG(i, MNC_EXECUTE) : MAKELONG(0, MNC_IGNORE);
}

inline bool Menu::hasSubMenu(UINT index) const {assertValidAsMenu(); return toBoolean(::IsMenu(::GetSubMenu(getHandle(), index)));}

template<Menu::ItemIdentificationPolicy idPolicy> inline bool Menu::hilite(HWND window, UINT item, bool hilite /* = true */) {
	assertValidAsMenu(); return toBoolean(::HiliteMenuItem(window, getHandle(), item, (idPolicy == Menu::BY_COMMAND ? MF_BYCOMMAND : MF_BYPOSITION) | (hilite ? MF_HILITE : MF_UNHILITE)));}

template<Menu::ItemIdentificationPolicy idPolicy> inline bool Menu::insert(UINT item, const ::MENUITEMINFO& info) {
	assertValidAsMenu(); return toBoolean(::InsertMenuItem(getHandle(), item, idPolicy == Menu::BY_POSITION, &info));}

template<Menu::ItemIdentificationPolicy idPolicy>
inline bool Menu::insert(UINT item, UINT previousItem, UINT type, UINT state, const TCHAR* caption) {
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
	return insert<idPolicy>(previousItem, info));
}

template<Menu::ItemIdentificationPolicy idPolicy> inline bool Menu::insertSeparator(UINT item) {
	ItemInfo info;
	info.fMask = MIIM_TYPE;
	info.fType = MFT_SEPARATOR;
	return insertMenuItem<idPolicy>(item, info);
}

inline bool Menu::isMenu() const {return toBoolean(::IsMenu(getHandle()));}

inline std::auto_ptr<Menu> Menu::load(HINSTANCE instance, const ResourceID& id) {
	if(HMENU handle = ::LoadMenu(instance, id.name))
		return std::auto_ptr<Menu>(new Menu(handle));
	else
		return std::auto_ptr<Menu>(0);
}

inline std::auto_ptr<Menu> Menu::load(const ::MENUTEMPLATE* menuTemplate) {
	if(HMENU handle = ::LoadMenuIndirect(menuTemplate))
		return std::auto_ptr<Menu>(new Menu(handle));
	else
		return std::auto_ptr<Menu>(0);
}

inline int Menu::itemFromPoint(HWND window, const ::POINT& pt) const {assertValidAsMenu(); return ::MenuItemFromPoint(window, getHandle(), pt);}

inline LRESULT Menu::measureItem(::MEASUREITEMSTRUCT& mi, const TCHAR* text, const TCHAR* accelerator /* = 0 */) {
	if(mi.CtlType != ODT_MENU)
		return false;
	else if(text == 0) {	// separator
		mi.itemWidth = 0;
		mi.itemHeight = ::GetSystemMetrics(SM_CYMENU) / 2;
	} else {
		AutoZeroCB<::NONCLIENTMETRICS> ncm;
		::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);
		ncm.lfMenuFont.lfWeight = FW_BOLD;
		HFONT menuFont = ::CreateFontIndirect(&ncm.lfMenuFont);
		gdi::ScreenDC dc;
		HFONT oldFont = (dc.selectObject(menuFont));
		::RECT textRect, accelRect = {0, 0, 0, 0};
		dc.drawText(text, -1, textRect, DT_CALCRECT | DT_LEFT | DT_NOPREFIX | DT_SINGLELINE);
		if(accelerator != 0)
			dc.drawText(accelerator, -1, accelRect, DT_CALCRECT | DT_RIGHT | DT_NOPREFIX | DT_SINGLELINE);

		mi.itemWidth = textRect.right - textRect.left
			+ accelRect.right - accelRect.left + TEXT_MARGIN * 2 + BUTTON_GAP
			+ (::GetSystemMetrics(SM_CXSMICON) + 1) * 2
			+ dc.getTextExtent(_T("x"), 1).cx - ::GetSystemMetrics(SM_CXMENUCHECK) - 1;
		mi.itemHeight = std::max<long>(
			std::max(textRect.bottom - textRect.top, accelRect.bottom - accelRect.top),
			std::max(::GetSystemMetrics(SM_CYSMICON) + 4, ::GetSystemMetrics(SM_CYMENUCHECK) - 1));

		dc.selectObject(oldFont);
		::DeleteObject(menuFont);
	}
	return true;
}

template<> inline bool Menu::remove<Menu::BY_COMMAND>(UINT item) {assertValidAsMenu(); return toBoolean(::RemoveMenu(getHandle(), item, MF_BYCOMMAND));}

template<> inline bool Menu::remove<Menu::BY_POSITION>(UINT item) {assertValidAsMenu(); return toBoolean(::RemoveMenu(getHandle(), item, MF_BYPOSITION));}

template<> inline bool Menu::setBitmaps<Menu::BY_COMMAND>(UINT item, HBITMAP uncheckedBitmap, HBITMAP checkedBitmap) {
	assertValidAsMenu(); return toBoolean(::SetMenuItemBitmaps(getHandle(), item, MF_BYCOMMAND, uncheckedBitmap, checkedBitmap));}

template<> inline bool Menu::setBitmaps<Menu::BY_POSITION>(UINT item, HBITMAP uncheckedBitmap, HBITMAP checkedBitmap) {
	assertValidAsMenu(); return toBoolean(::SetMenuItemBitmaps(getHandle(), item, MF_BYPOSITION, uncheckedBitmap, checkedBitmap));}

template<Menu::ItemIdentificationPolicy idPolicy> inline bool Menu::setChildPopup(UINT item, const Menu& popup) {
	assertValidAsMenu();
	assert(popup.isMenu());
	ItemInfo info;
	info.fMask = MIIM_SUBMENU;
	info.hSubMenu = popup.getHandle();
	return setItemInformation<idPolicy>(item, info);
}

template<Menu::ItemIdentificationPolicy idPolicy> inline bool Menu::setChildPopup(UINT item, std::auto_ptr<Menu> popup) {
	assertValidAsMenu();
	assert(popup->isMenu());
	ItemInfo info;
	info.fMask = MIIM_SUBMENU;
	info.hSubMenu = popup->getHandle();
	if(setItemInformation<idPolicy>(item, info)) {
		managedChildren_.insert(popup.get()->release());
		return true;
	} else
		return false;
}

template<Menu::ItemIdentificationPolicy idPolicy> inline bool Menu::setChildPopup(UINT item, HMENU popup, bool delegateOwnership) {
	assertValidAsMenu();
	assert(popup.isMenu());
	ItemInfo info;
	info.fMask = MIIM_SUBMENU;
	info.hSubMenu = popup.getHandle();
	if(setItemInformation<idPolicy>(item, info)) {
		if(delegateOwnership)
			managedChildren_.insert(popup);
		return true;
	} else
		return false;
}

inline bool Menu::setContextHelpID(DWORD id) {assertValidAsMenu(); return toBoolean(::SetMenuContextHelpId(getHandle(), id));}

template<Menu::ItemIdentificationPolicy idPolicy> inline bool Menu::setDefault(UINT item) {
	assertValidAsMenu(); return toBoolean(::SetMenuDefaultItem(getHandle(), item, idPolicy == Menu::BY_POSITION));}

template<Menu::ItemIdentificationPolicy idPolicy> inline bool Menu::setItemInformation(UINT item, const ::MENUITEMINFO& info) {
	assertValidAsMenu(); return toBoolean(::SetMenuItemInfo(getHandle(), item, idPolicy == Menu::BY_POSITION, &info));}

template<Menu::ItemIdentificationPolicy idPolicy> inline bool Menu::setState(UINT item, UINT state) {
	assertValidAsMenu(); ItemInfo mi; mi.fMask = MIIM_STATE; mi.fState = state; return setItemInformation<idPolicy>(item, mi);}

inline bool Menu::trackPopup(UINT flags, int x, int y, HWND window, const RECT* rect /* = 0 */) const {
	assertValidAsMenu(); return toBoolean(::TrackPopupMenu(getHandle(), flags, x, y, 0, window, rect));}

inline bool Menu::trackPopupEx(UINT flags, int x, int y, HWND window, const ::TPMPARAMS* params /* = 0 */) const {
	assertValidAsMenu(); return toBoolean(::TrackPopupMenuEx(getHandle(), flags, x, y, window, const_cast<::TPMPARAMS*>(params)));}

#if(WINVER >= 0x0500)
inline bool Menu::getInformation(::MENUINFO& mi) const {assertValidAsMenu(); return toBoolean(::GetMenuInfo(getHandle(), &mi));}

inline bool Menu::setInformation(const ::MENUINFO& mi) {assertValidAsMenu(); return toBoolean(::SetMenuInfo(getHandle(), &mi));}
#endif /* WINVER >= 0x0500 */

}}} // namespace manah.win32.ui

#pragma warning(default : 4297)

#endif /* !MANAH_MENU_HPP */
