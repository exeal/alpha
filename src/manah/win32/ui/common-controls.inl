// common-controls.inl
// (c) 2002-2007 exeal


namespace manah {
namespace windows {
namespace ui {


// AnimateCtrl //////////////////////////////////////////////////////////////

inline bool AnimateCtrl::close() {return open(ResourceID(0U));}

inline bool AnimateCtrl::open(const ResourceID& id, HINSTANCE hinstance /* = 0 */) {
	return sendMessageR<bool>(ACM_OPEN, reinterpret_cast<WPARAM>(hinstance), reinterpret_cast<LPARAM>(id.name));}

inline bool AnimateCtrl::play(UINT from, UINT to, UINT repeatCount) {return sendMessageR<bool>(ACM_PLAY, repeatCount, MAKELONG(from, to));}

inline bool AnimateCtrl::seek(UINT to) {return play(to, to, 1);}

inline bool AnimateCtrl::stop() {return sendMessageR<bool>(ACM_STOP);}



// DateTimePickerCtrl ///////////////////////////////////////////////////////

inline HWND DateTimePickerCtrl::getMonthCalendar() const {return reinterpret_cast<HWND>(sendMessageC<LRESULT>(DTM_GETMONTHCAL));}

inline COLORREF DateTimePickerCtrl::getMonthCalendarColor(int colorType) const {return sendMessageC<::COLORREF>(DTM_GETMCCOLOR, colorType);}

inline HFONT DateTimePickerCtrl::getMonthCalendarFont() const {return reinterpret_cast<HFONT>(sendMessageC<LRESULT>(DTM_GETMCFONT));}

inline DWORD DateTimePickerCtrl::getRange(SYSTEMTIME times[]) const {
	return sendMessageC<DWORD>(DTM_GETRANGE, 0, reinterpret_cast<LPARAM>(times));}

inline DWORD DateTimePickerCtrl::getSystemTime(SYSTEMTIME& time) const {
	return sendMessageC<DWORD>(DTM_GETSYSTEMTIME, 0, reinterpret_cast<LPARAM>(&time));}

inline bool DateTimePickerCtrl::setFormat(const TCHAR* format) {return sendMessageR<bool>(DTM_SETFORMAT, 0, reinterpret_cast<LPARAM>(format));}

inline COLORREF DateTimePickerCtrl::setMonthCalendarColor(int colorType, COLORREF color) {
	return sendMessageR<::COLORREF>(DTM_SETMCCOLOR, colorType, color);}

inline void DateTimePickerCtrl::setMonthCalendarFont(HFONT font, bool redraw /* = true */) {
	sendMessage(DTM_SETMCFONT, reinterpret_cast<WPARAM>(font), MAKELPARAM(redraw, 0));}

inline bool DateTimePickerCtrl::setRange(DWORD flags, const SYSTEMTIME times[]) {
	return sendMessageR<bool>(DTM_SETRANGE, flags, reinterpret_cast<LPARAM>(times));}

inline bool DateTimePickerCtrl::setSystemTime(DWORD flags, const SYSTEMTIME& time) {
	return sendMessageR<bool>(DTM_SETSYSTEMTIME, flags, reinterpret_cast<LPARAM>(&time));}


// HotKeyCtrl ///////////////////////////////////////////////////////////////

inline DWORD HotKeyCtrl::getHotKey() const {return sendMessageC<DWORD>(HKM_GETHOTKEY);}

inline void HotKeyCtrl::getHotKey(WORD& virtualKeyCode, WORD& modifiers) const {
	const DWORD	keys = getHotKey();
	virtualKeyCode = LOWORD(keys);
	modifiers = HIWORD(keys);
}

inline const TCHAR* HotKeyCtrl::getHotKeyName() const {
	assertValidAsWindow();
	static TCHAR hotKeyName[100];
	WORD vKey, modifiers;

	getHotKey(vKey, modifiers);
	hotKeyName[0] = 0;
	if(toBoolean(modifiers & HOTKEYF_CONTROL))	std::_tcscat(hotKeyName, _T("Ctrl+"));
	if(toBoolean(modifiers & HOTKEYF_SHIFT))	std::_tcscat(hotKeyName, _T("Shift+"));
	if(toBoolean(modifiers & HOTKEYF_ALT))		std::_tcscat(hotKeyName, _T("Alt+"));
	if(toBoolean(modifiers & HOTKEYF_EXT))		std::_tcscat(hotKeyName, _T("Ext+"));
	std::_tcscat(hotKeyName, getKeyName(vKey, false));

	return hotKeyName;
}

inline const TCHAR* HotKeyCtrl::getKeyName(UINT virtualKey, bool extended) {
	static TCHAR keyName[50];
	::GetKeyNameText((::MapVirtualKey(virtualKey, 0) << 16) | ((extended ? 1 : 0) << 24), keyName, 50);
	return keyName;
}

inline void HotKeyCtrl::setHotKey(WORD virtualKeyCode, WORD modifiers) {sendMessage(HKM_SETHOTKEY, MAKEWORD(virtualKeyCode, modifiers));}

inline void HotKeyCtrl::setRules(WORD invalidCombination, WORD modifiers) {
	sendMessage(HKM_SETRULES, invalidCombination, MAKELPARAM(modifiers, 0));}


// ImageList ////////////////////////////////////////////////////////////////

inline ImageList::~ImageList() {if(!isAttached()) destroy();}

inline int ImageList::add(HBITMAP bitmap, HBITMAP mask /* = 0 */) {
	assertValidAsImageList(); return ::ImageList_Add(get(), bitmap, mask);}

inline int ImageList::add(HBITMAP bitmap, COLORREF maskColor) {
	assertValidAsImageList(); return ::ImageList_AddMasked(get(), bitmap, maskColor);}

inline int ImageList::add(HICON icon) {assertValidAsImageList(); return ImageList_AddIcon(get(), icon);}

inline bool ImageList::beginDrag(int index, const POINT& hotSpot) const {return beginDrag(index, hotSpot.x, hotSpot.y);}

inline bool ImageList::beginDrag(int index, int xHotSpot, int yHotSpot) const {
	assertValidAsImageList(); return toBoolean(::ImageList_BeginDrag(get(), index, xHotSpot, yHotSpot));}

inline bool ImageList::copy(int dest, int src, UINT flags /* = ILCF_MOVE */) {return copy(dest, get(), src, flags);}

inline bool ImageList::copy(int dest, HIMAGELIST imageList, int src, UINT flags /* = ILCF_MOVE */) {
	assertValidAsImageList(); return toBoolean(::ImageList_Copy(get(), dest, imageList, src, flags));}

inline bool ImageList::create(int cx, int cy, UINT flags, int initial, int grow) {
	if(isImageList())
		return false;
	setHandle(::ImageList_Create(cx, cy, flags, initial, grow));
	return get() != 0;
}

inline bool ImageList::create(HINSTANCE hinstance, const ResourceID& bitmapName, int cx, int grow, COLORREF maskColor) {
	if(isImageList())
		return false;
	setHandle(ImageList_LoadBitmap(hinstance, bitmapName.name, cx, grow, maskColor));
	return get() != 0;
}

inline bool ImageList::createFromImage(HINSTANCE hinstance, const ResourceID& imageName,
		int cx, int grow, COLORREF maskColor, UINT type, UINT flags /* = LR_DEFAULTCOLOR | LR_DEFAULTSIZE */) {
	if(isImageList())
		return false;
	setHandle(::ImageList_LoadImage(hinstance, imageName.name, cx, grow, maskColor, type, flags));
	return get() != 0;
}

inline std::auto_ptr<ImageList> ImageList::createManagedObject(HIMAGELIST imageList) {
	std::auto_ptr<ImageList> p(new ImageList); p->setHandle(imageList); return p;}

inline bool ImageList::destroy() {
	if(isImageList() && toBoolean(::ImageList_Destroy(get()))) {
		setHandle(0);
		return true;
	}
	return false;
}

inline bool ImageList::dragEnter(HWND lockWindow, const POINT& pt) {return dragEnter(lockWindow, pt.x, pt.y);}

inline bool ImageList::dragEnter(HWND lockWindow, int x, int y) {return toBoolean(::ImageList_DragEnter(lockWindow, x, y));}

inline bool ImageList::dragLeave(HWND lockWindow) {return toBoolean(::ImageList_DragLeave(lockWindow));}

inline bool ImageList::dragMove(const POINT& pt) {return dragMove(pt.x, pt.y);}

inline bool ImageList::dragMove(int x, int y) {return toBoolean(::ImageList_DragMove(x, y));}

inline bool ImageList::dragShowNolock(bool show /* = true */) {return toBoolean(::ImageList_DragShowNolock(show));}

inline bool ImageList::draw(HDC dc, int index, const POINT& pt, UINT style) const {return draw(dc, index, pt.x, pt.y, style);}

inline bool ImageList::draw(HDC dc, int index, int x, int y, UINT style) const {
	assertValidAsImageList(); return toBoolean(::ImageList_Draw(get(), index, dc, x, y, style));}

inline bool ImageList::drawEx(HDC dc, int index, const RECT& rect, COLORREF bgColor, COLORREF fgColor, UINT style) const {
	return drawEx(dc, index, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, bgColor, fgColor, style);}

inline bool ImageList::drawEx(HDC dc, int index, int x, int y, int dx, int dy, COLORREF bgColor, COLORREF fgColor, UINT style) const {
	assertValidAsImageList(); return toBoolean(::ImageList_DrawEx(get(), index, dc, x, y, dx, dy, bgColor, fgColor, style));}

inline bool ImageList::drawIndirect(const IMAGELISTDRAWPARAMS& params) const {
	assertValidAsImageList(); return toBoolean(::ImageList_DrawIndirect(const_cast<IMAGELISTDRAWPARAMS*>(&params)));}

inline std::auto_ptr<ImageList> ImageList::duplicate() const {return duplicate(get());}

inline std::auto_ptr<ImageList> ImageList::duplicate(HIMAGELIST imageList) {return createManagedObject(::ImageList_Duplicate(imageList));}

inline void ImageList::endDrag() {::ImageList_EndDrag();}

inline HICON ImageList::extractIcon(int index) const {assertValidAsImageList(); return ImageList_ExtractIcon(0, get(), index);}

inline COLORREF ImageList::getBkColor() const {assertValidAsImageList(); return ::ImageList_GetBkColor(get());}

inline std::auto_ptr<ImageList> ImageList::getDragImage(POINT* pt, POINT* hotSpot) {
	return std::auto_ptr<ImageList>(new ImageList(::ImageList_GetDragImage(pt, hotSpot)));}

inline HICON ImageList::getIcon(int index, UINT flags /* = ILD_NORMAL */) const {
	assertValidAsImageList(); return ::ImageList_GetIcon(get(), index, flags);}

inline bool ImageList::getIconSize(SIZE& size) const {return getIconSize(size.cx, size.cy);}

inline bool ImageList::getIconSize(long& cx, long& cy) const {
	assertValidAsImageList(); toBoolean(::ImageList_GetIconSize(get(), reinterpret_cast<int*>(&cx), reinterpret_cast<int*>(&cy)));}

inline int ImageList::getImageCount() const {assertValidAsImageList(); return ::ImageList_GetImageCount(get());}

inline bool ImageList::getImageInfo(int index, IMAGEINFO& imageInfo) const {
	assertValidAsImageList(); return toBoolean(::ImageList_GetImageInfo(get(), index, &imageInfo));}

inline bool ImageList::merge(HIMAGELIST imageList1, int image1, HIMAGELIST imageList2, int image2, int dx, int dy) {
	if(get() != 0)
		return false;
	setHandle(::ImageList_Merge(imageList1, image1, imageList2, image2, dx, dy));
	return get() != 0;
}

#if 0/*defined(__IStream_INTERFACE_DEFINED__)*/
inline std::auto_ptr<ImageList> ImageList::readFromStream(IStream& stream) {return createManagedObject(::ImageList_Read(&stream));}

#if(_WIN32_WINNT >= 0x0501)
inline HRESULT ImageList::readFromStream(IStream& stream, REFIID riid, void*& pv, DWORD flags) {
	return ::ImageList_ReadEx(flags, &stream, riid, &pv);}
#endif /* _WIN32_WINNT >= 0x0501 */
#endif /* __IStream_INTERFACE_DEFINED__ */

inline bool ImageList::remove(int index) {assertValidAsImageList(); return toBoolean(::ImageList_Remove(get(), index));}

inline bool ImageList::removeAll() {assertValidAsImageList(); return toBoolean(ImageList_RemoveAll(get()));}

inline bool ImageList::replace(int index, HBITMAP bitmap, HBITMAP mask) {
	assertValidAsImageList(); return toBoolean(::ImageList_Replace(get(), index, bitmap, mask));}

inline int ImageList::replace(int index, HICON icon) {assertValidAsImageList(); return ::ImageList_ReplaceIcon(get(), index, icon);}

inline COLORREF ImageList::setBkColor(COLORREF color) {assertValidAsImageList(); return ::ImageList_SetBkColor(get(), color);}

inline bool ImageList::setDragCursorImage(int index, int xHotSpot, int yHotSpot) {
	assertValidAsImageList(); return toBoolean(::ImageList_SetDragCursorImage(get(), index, xHotSpot, yHotSpot));}

inline bool ImageList::setDragCursorImage(int index, const POINT& hotSpot) {return setDragCursorImage(index, hotSpot.x, hotSpot.y);}

inline bool ImageList::setIconSize(const SIZE& size) {return setIconSize(size.cx, size.cy);}

inline bool ImageList::setIconSize(long cx, long cy) {assertValidAsImageList(); return toBoolean(::ImageList_SetIconSize(get(), cx, cy));}

inline bool ImageList::setImageCount(UINT newCount) {
	assertValidAsImageList(); return toBoolean(::ImageList_SetImageCount(get(), newCount));}

inline bool ImageList::setOverlayImage(int index, int overlayIndex) {
	assertValidAsImageList(); return toBoolean(::ImageList_SetOverlayImage(get(), index, overlayIndex));}

#if 0/*defined(__IStream_INTERFACE_DEFINED__)*/
inline bool ImageList::writeToStream(IStream& stream) {assertValidAsImageList(); return toBoolean(::ImageList_Write(get(), &stream));}

inline HRESULT ImageList::writeToStream(IStream& stream, DWORD flags) {
	assertValidAsImageList(); return ::ImageList_WriteEx(get(), flags, &stream);}
#endif /* __IStream_INTERFACE_DEFINED__ */


// IpAddressCtrl ////////////////////////////////////////////////////////////

inline void IpAddressCtrl::clearAddress() {sendMessage(IPM_CLEARADDRESS);}

inline int IpAddressCtrl::getAddress(DWORD& address) const {return sendMessageC<int>(IPM_GETADDRESS, 0, reinterpret_cast<LPARAM>(&address));}

inline bool IpAddressCtrl::isBlank() const {return sendMessageC<bool>(IPM_ISBLANK);}

inline void IpAddressCtrl::setAddress(DWORD address) {sendMessage(IPM_SETADDRESS, 0, address);}

inline void IpAddressCtrl::setFocus(int field) {sendMessage(IPM_SETFOCUS, field);}

inline void IpAddressCtrl::setRange(int field, ushort range) {sendMessage(IPM_SETRANGE, field, range);}

inline void IpAddressCtrl::setRange(int field, uchar min, uchar max) {sendMessage(IPM_SETRANGE, field, MAKEIPRANGE(min, max));}


// ListCtrl /////////////////////////////////////////////////////////////////

inline SIZE ListCtrl::approximateViewRect(const SIZE& size, int count /* = -1 */) const {
	assertValidAsWindow();
	SIZE s;
	DWORD temp = sendMessageC<DWORD>(LVM_APPROXIMATEVIEWRECT, count, MAKELONG(size.cx, size.cy));
	s.cx = LOWORD(temp);
	s.cy = HIWORD(temp);
	return s;
}

inline bool ListCtrl::arrange(UINT code) {return sendMessageR<bool>(LVM_ARRANGE, code);}

inline HIMAGELIST ListCtrl::createDragImage(int index, LPPOINT point) {
	return reinterpret_cast<HIMAGELIST>(sendMessage(LVM_CREATEDRAGIMAGE, index, reinterpret_cast<LPARAM>(point)));}

inline bool ListCtrl::deleteAllItems() {return sendMessageR<bool>(LVM_DELETEALLITEMS);}

inline bool ListCtrl::deleteColumn(int column) {return sendMessageR<bool>(LVM_DELETECOLUMN, column);}

inline bool ListCtrl::deleteItem(int index) {return sendMessageR<bool>(LVM_DELETEITEM, index);}

inline HWND ListCtrl::editLabel(int index) {return reinterpret_cast<HWND>(sendMessage(LVM_EDITLABEL, index));}

inline bool ListCtrl::ensureVisible(int index, bool partialOK) {return sendMessageR<bool>(LVM_ENSUREVISIBLE, index, partialOK);}

inline int ListCtrl::findItem(LVFINDINFO& findInfo, int start /* = -1 */) const {
	return sendMessageC<int>(LVM_FINDITEM, start, reinterpret_cast<LPARAM>(&findInfo));}

inline COLORREF ListCtrl::getBkColor() const {return sendMessageC<COLORREF>(LVM_GETBKCOLOR);}

inline bool ListCtrl::getBkImage(LVBKIMAGE& image) const {return sendMessageC<bool>(LVM_GETBKIMAGE, 0, reinterpret_cast<LPARAM>(&image));}

inline UINT ListCtrl::getCallbackMask() const {return sendMessageC<UINT>(LVM_GETCALLBACKMASK);}

inline bool ListCtrl::getCheck(int index) const {return toBoolean(((sendMessageC<UINT>(LVM_GETITEMSTATE, index, LVIS_STATEIMAGEMASK) >> 12) - 1));}

inline bool ListCtrl::getColumn(int index, LVCOLUMN& column) const {
	return sendMessageC<bool>(LVM_GETCOLUMN, index, reinterpret_cast<LPARAM>(&column));}

inline bool ListCtrl::getColumnOrderArray(LPINT array, int count /* = -1 */) const {
	if(count == -1)
		count = static_cast<int>(::SendMessage(getHeaderControl(), HDM_GETITEMCOUNT, 0, 0L));
	return sendMessageC<bool>(LVM_GETCOLUMNORDERARRAY, count, reinterpret_cast<LPARAM>(array));
}

inline int ListCtrl::getColumnWidth(int column) const {return sendMessageC<int>(LVM_GETCOLUMNWIDTH, column);}

inline int ListCtrl::getCountPerPage() const {return sendMessageC<int>(LVM_GETCOUNTPERPAGE);}

inline HWND ListCtrl::getEditControl() const {return reinterpret_cast<HWND>(sendMessageC<LRESULT>(LVM_GETEDITCONTROL));}

inline DWORD ListCtrl::getExtendedStyle() const {sendMessageC<DWORD>(LVM_GETEXTENDEDLISTVIEWSTYLE);}

inline HWND ListCtrl::getHeaderControl() const {return reinterpret_cast<HWND>(sendMessageC<LRESULT>(LVM_GETHEADER));}

inline HCURSOR ListCtrl::getHotCursor() const {return reinterpret_cast<HCURSOR>(sendMessageC<LRESULT>(LVM_GETHOTCURSOR));}

inline int ListCtrl::getHotItem() const {return sendMessageC<int>(LVM_GETHOTITEM);}

inline DWORD ListCtrl::getHoverTime() const {return sendMessageC<DWORD>(LVM_GETHOVERTIME);}

inline HIMAGELIST ListCtrl::getImageList(int imageListType) const {
	return reinterpret_cast<HIMAGELIST>(sendMessageC<LRESULT>(LVM_GETIMAGELIST, imageListType));}

inline bool ListCtrl::getItem(LVITEM& item) const {return sendMessageC<bool>(LVM_GETITEM, 0, reinterpret_cast<LPARAM>(&item));}

inline int ListCtrl::getItemCount() const {return sendMessageC<int>(LVM_GETITEMCOUNT);}

inline LPARAM ListCtrl::getItemData(int index) const {
	LVITEM item;
	item.mask = LVIF_PARAM;
	item.iItem = index;
	item.iSubItem = 0;
	getItem(item);
	return item.lParam;
}

inline bool ListCtrl::getItemPosition(int index, POINT& point) const {
	return sendMessageC<bool>(LVM_GETITEMPOSITION, index, reinterpret_cast<LPARAM>(&point));}

inline bool ListCtrl::getItemRect(int index, RECT& rect, UINT code) const {
	rect.left = code;
	return sendMessageC<bool>(LVM_GETITEMRECT, index, reinterpret_cast<LPARAM>(&rect));
}

inline UINT ListCtrl::getItemState(int index, UINT mask) const {return sendMessageC<UINT>(LVM_GETITEMSTATE, index, mask);}

inline int ListCtrl::getItemText(int index, int subItem, TCHAR* text, int maxLength) const {
	LVITEM item;
	item.iSubItem = subItem;
	item.pszText = text;
	item.cchTextMax = maxLength;
	return sendMessageC<int>(LVM_GETITEMTEXT, index, reinterpret_cast<LPARAM>(&item));
}

inline std::basic_string<TCHAR> ListCtrl::getItemText(int index, int subItem) const {
	int len = 128;
	TCHAR* buffer = 0;
	int res;
	do {
		delete[] buffer;
		len *= 2;
		buffer = new TCHAR[len];
		res = getItemText(index, subItem, buffer, len);
	} while(res == len - 1);
	std::basic_string<TCHAR> temp(buffer);
	delete[] buffer;
	return temp;
}

inline int ListCtrl::getNextItem(int index, int flag) const {return sendMessageC<int>(LVM_GETNEXTITEM, index, flag);}

inline bool ListCtrl::getOrigin(POINT& point) const {return sendMessageC<bool>(LVM_GETORIGIN, 0, reinterpret_cast<LPARAM>(&point));}

inline UINT ListCtrl::getSelectedCount() const {return sendMessageC<UINT>(LVM_GETSELECTEDCOUNT);}

inline int ListCtrl::getSelectionMark() const {return sendMessageC<int>(LVM_GETSELECTIONMARK);}

inline int ListCtrl::getStringWidth(const TCHAR* text) const {return sendMessageC<int>(LVM_GETSTRINGWIDTH, 0, reinterpret_cast<LPARAM>(text));}

inline bool ListCtrl::getSubItemRect(int index, int subItem, int area, RECT& rect) const {
	rect.left = area;
	rect.top = subItem;
	return sendMessageC<bool>(LVM_GETSUBITEMRECT, index, reinterpret_cast<LPARAM>(&rect));
}

inline COLORREF ListCtrl::getTextBkColor() const {return sendMessageC<COLORREF>(LVM_GETTEXTBKCOLOR);}

inline COLORREF ListCtrl::getTextColor() const {return sendMessageC<COLORREF>(LVM_GETTEXTCOLOR);}

inline int ListCtrl::getTopIndex() const {return sendMessageC<int>(LVM_GETTOPINDEX);}

inline bool ListCtrl::getViewRect(RECT& rect) const {return sendMessageC<bool>(LVM_GETVIEWRECT, 0, reinterpret_cast<LPARAM>(&rect));}

inline void ListCtrl::getWorkAreas(int count, RECT rect[]) const {sendMessageC<int>(LVM_GETWORKAREAS, count, reinterpret_cast<LPARAM>(&rect));}

inline int ListCtrl::hitTest(LVHITTESTINFO& hitTestInfo) const {return sendMessageC<int>(LVM_HITTEST, 0, reinterpret_cast<LPARAM>(&hitTestInfo));}

inline int ListCtrl::hitTest(const POINT& pt, UINT* flags /* = 0 */) const {
	LVHITTESTINFO lvti;
	lvti.pt = pt;
	lvti.flags = (flags != 0) ? *flags : 0;
	return hitTest(lvti);
}

inline int ListCtrl::insertColumn(int index, const LVCOLUMN& column) {
	return sendMessageR<int>(LVM_INSERTCOLUMN, index, reinterpret_cast<LPARAM>(&column));}

inline int ListCtrl::insertColumn(int position,
		const TCHAR* columnHeading, int format /* = LVCFMT_LEFT */, int width /* = -1 */, int subItem /* = -1 */) {
	LVCOLUMN lvcol = {
		LVCF_FMT | LVCF_TEXT | ((subItem != -1) ? LVCF_SUBITEM : 0) | ((width != -1) ? LVCF_WIDTH : 0),
		format, width, const_cast<TCHAR*>(columnHeading), static_cast<int>(std::_tcslen(columnHeading)), subItem
	};
	return insertColumn(position, lvcol);
}

inline int ListCtrl::insertItem(const LVITEM& item) {return sendMessageR<int>(LVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&item));}

inline int ListCtrl::insertItem(int index, const TCHAR* text) {
	AutoZero<LVITEM> item;
	item.mask = LVIF_TEXT;
	item.iItem = index;
	item.pszText = const_cast<TCHAR*>(text);
	return insertItem(item);
}

inline int ListCtrl::insertItem(int index, const TCHAR* text, int image) {
	AutoZero<LVITEM> item;
	item.mask = LVIF_TEXT | LVIF_IMAGE;
	item.iItem = index;
	item.pszText = const_cast<TCHAR*>(text);
	item.iImage = image;
	return insertItem(item);
}

inline int ListCtrl::insertItem(UINT mask, int index, const TCHAR* text, UINT state, UINT stateMask, int image, LPARAM lParam) {
	LVITEM item = {mask, index, 0, state, stateMask, const_cast<TCHAR*>(text), 0, image, lParam};
	return insertItem(item);
}

inline bool ListCtrl::redrawItems(int first, int last) {return sendMessageR<bool>(LVM_REDRAWITEMS, first, last);}

inline bool ListCtrl::scroll(const SIZE& size) {return sendMessageR<bool>(LVM_SCROLL, size.cx, size.cy);}

inline bool ListCtrl::setBkColor(COLORREF color) {return sendMessageR<bool>(LVM_SETBKCOLOR, 0, color);}

inline bool ListCtrl::setBkImage(const LVBKIMAGE& image) {return sendMessageR<bool>(LVM_SETBKIMAGE, 0, reinterpret_cast<LPARAM>(&image));}

inline bool ListCtrl::setBkImage(HBITMAP bitmap, bool tile /* = true */, int xOffsetPercent /* = 0 */, int yOffsetPercent /* = 0 */) {
	LVBKIMAGE bkImage = {
		LVBKIF_SOURCE_HBITMAP | ((xOffsetPercent == 0 && yOffsetPercent == 0) ?
			LVBKIF_STYLE_TILE : LVBKIF_STYLE_NORMAL),
		bitmap, 0, 0, xOffsetPercent, yOffsetPercent
	};
	return setBkImage(bkImage);
}

inline bool ListCtrl::setBkImage(const TCHAR* url, bool tile /* = true */, int xOffsetPercent /* = 0 */, int yOffsetPercent /* = 0 */) {
	LVBKIMAGE bkImage = {
		LVBKIF_SOURCE_URL | ((xOffsetPercent == 0 && yOffsetPercent == 0) ?
			LVBKIF_STYLE_TILE : LVBKIF_STYLE_NORMAL),
		0, const_cast<TCHAR*>(url), 0, xOffsetPercent, yOffsetPercent
	};
	return setBkImage(bkImage);
}

inline bool ListCtrl::setCallbackMask(UINT mask) {return sendMessageR<bool>(LVM_SETCALLBACKMASK, mask);}

inline bool ListCtrl::setCheck(int index, bool check /* = true */) {
	LVITEM item = {0, index, 0, INDEXTOSTATEIMAGEMASK(check - 1), LVIS_STATEIMAGEMASK, 0, 0, 0, 0};
	return sendMessageR<bool>(LVM_SETITEMSTATE, index, reinterpret_cast<LPARAM>(&item));
}

inline bool ListCtrl::setColumn(int index, const LVCOLUMN& column) {
	return sendMessageR<bool>(LVM_SETCOLUMN, index, reinterpret_cast<LPARAM>(&column));}

inline bool ListCtrl::setColumnOrderArray(int count, INT array[]) {
	return sendMessageR<bool>(LVM_SETCOLUMNORDERARRAY, count, reinterpret_cast<LPARAM>(array));}

inline bool ListCtrl::setColumnWidth(int column, int cx) {return sendMessageR<bool>(LVM_SETCOLUMNWIDTH, column, cx);}

inline DWORD ListCtrl::setExtendedStyle(DWORD newStyle) {return sendMessageR<DWORD>(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, newStyle);}

inline DWORD ListCtrl::setExtendedStyleEx(DWORD exMask, DWORD exStyle) {
	return sendMessageR<DWORD>(LVM_SETEXTENDEDLISTVIEWSTYLE, exMask, exStyle);}

inline HCURSOR ListCtrl::setHotCursor(HCURSOR cursor) {
	return reinterpret_cast<HCURSOR>(sendMessage(LVM_SETHOTCURSOR, 0, reinterpret_cast<LPARAM>(cursor)));}

inline int ListCtrl::setHotItem(int index) {return sendMessageR<int>(LVM_SETHOTITEM, index);}

inline DWORD ListCtrl::setHoverTime(DWORD hoverTime /* = -1 */) {return sendMessageR<DWORD>(LVM_SETHOVERTIME, 0, hoverTime);}

inline SIZE ListCtrl::setIconSpacing(int cx, int cy) {
	SIZE size;
	const DWORD temp = sendMessageR<DWORD>(LVM_SETICONSPACING, 0, MAKELONG(cx, cy));
	size.cx = LOWORD(temp);
	size.cy = HIWORD(temp);
	return size;
}

inline SIZE ListCtrl::setIconSpacing(const SIZE& size) {return setIconSpacing(size.cx, size.cy);}

inline HIMAGELIST ListCtrl::setImageList(HIMAGELIST imageList, int imageListType) {
	return reinterpret_cast<HIMAGELIST>(sendMessage(LVM_SETIMAGELIST, imageListType, reinterpret_cast<LPARAM>(imageList)));}

inline bool ListCtrl::setItem(const LVITEM& item) {return sendMessageR<bool>(LVM_SETITEM, 0, reinterpret_cast<LPARAM>(&item));}

inline void ListCtrl::setItemCount(int count) {sendMessage(LVM_SETITEMCOUNT, count);}

inline void ListCtrl::setItemCountEx(int count, DWORD flags /* = LVSICF_NOINVALIDATEALL */) {sendMessage(LVM_SETITEMCOUNT, count, flags);}

inline bool ListCtrl::setItemData(int index, DWORD data) {
	LVITEM item;
	item.mask = LVIF_PARAM;
	item.iItem = index;
	item.iSubItem = 0;
	item.lParam = data;
	return setItem(item);
}

inline bool ListCtrl::setItem(int index, int subItem, UINT mask, const TCHAR* text, int image, UINT state, UINT stateMask, LPARAM lParam) {
	LVITEM item = {mask, index, subItem, state, stateMask, const_cast<TCHAR*>(text), 0, image};
	return sendMessageR<bool>(LVM_SETITEM, 0, reinterpret_cast<LPARAM>(&item));
}

inline bool ListCtrl::setItemPosition(int index, const POINT& pt) {
	return sendMessageR<bool>(LVM_SETITEMPOSITION32, index, MAKELPARAM(pt.x, pt.y));}

inline bool ListCtrl::setItemState(int index, const LVITEM& item) {
	return sendMessageR<bool>(LVM_SETITEMSTATE, index, reinterpret_cast<LPARAM>(&item));}

inline bool ListCtrl::setItemText(int index, int subItem, const TCHAR* text) {
	LVITEM item;
	item.iSubItem = subItem;
	item.pszText = const_cast<TCHAR*>(text);
	return sendMessageR<bool>(LVM_SETITEMTEXT, index, reinterpret_cast<LPARAM>(&item));
}

inline int ListCtrl::setSelectionMark(int index) {return sendMessageR<int>(LVM_SETSELECTIONMARK, 0, index);}

inline bool ListCtrl::setItemState(int index, UINT state, UINT mask) {
	LVITEM item;
	item.state = state;
	item.stateMask = mask;
	return sendMessageR<bool>(LVM_SETITEMSTATE, index, reinterpret_cast<LPARAM>(&item));
}

inline bool ListCtrl::setTextBkColor(COLORREF color) {return sendMessageR<bool>(LVM_SETTEXTBKCOLOR, 0, static_cast<LPARAM>(color));}

inline bool ListCtrl::setTextColor(COLORREF color) {return sendMessageR<bool>(LVM_SETTEXTCOLOR, 0, static_cast<LPARAM>(color));}

inline void ListCtrl::setWorkAreas(int count, const RECT rect[]) {sendMessage(LVM_SETWORKAREAS, count, reinterpret_cast<LPARAM>(rect));}

inline bool ListCtrl::sortItems(PFNLVCOMPARE compare, DWORD data) {
	return sendMessageR<bool>(LVM_SORTITEMS, data, reinterpret_cast<LPARAM>(compare));}

inline int ListCtrl::subItemHitTest(LVHITTESTINFO& info) {
	return sendMessageR<int>(LVM_SUBITEMHITTEST, 0, reinterpret_cast<LPARAM>(&info));}

inline bool ListCtrl::update(int index) {return toBoolean(sendMessage(LVM_UPDATE, index));}


// MonthCalendarCtrl ////////////////////////////////////////////////////////

inline COLORREF MonthCalendarCtrl::getColor(int colorType) const {return sendMessageC<COLORREF>(MCM_GETCOLOR, colorType);}

inline bool MonthCalendarCtrl::getCurSel(SYSTEMTIME& time) const {return sendMessageC<bool>(MCM_GETCURSEL, 0, reinterpret_cast<LPARAM>(&time));}

inline int MonthCalendarCtrl::getFirstDayOfWeek(bool* localeVal /* = 0 */) const {
	const DWORD temp = sendMessageC<DWORD>(MCM_GETFIRSTDAYOFWEEK);
	if(localeVal != 0)
		*localeVal = toBoolean(HIWORD(temp));
	return LOWORD(temp);
}

inline int MonthCalendarCtrl::getMaxSelCount() const {return sendMessageC<int>(MCM_GETMAXSELCOUNT);}

inline int MonthCalendarCtrl::getMaxTodayWidth() const {return sendMessageC<int>(MCM_GETMAXTODAYWIDTH);}

inline bool MonthCalendarCtrl::getMinReqRect(RECT& rect) const {return sendMessageC<bool>(MCM_GETMINREQRECT, 0, reinterpret_cast<LPARAM>(&rect));}

inline int MonthCalendarCtrl::getMonthDelta() const {return sendMessageC<int>(MCM_GETMONTHDELTA);}

inline int MonthCalendarCtrl::getMonthRange(DWORD flags, SYSTEMTIME times[]) const {
	return sendMessageC<int>(MCM_GETMONTHRANGE, flags, reinterpret_cast<LPARAM>(times));}

inline DWORD MonthCalendarCtrl::getRange(SYSTEMTIME times[]) const {return sendMessageC<DWORD>(MCM_GETRANGE, 0, reinterpret_cast<LPARAM>(times));}

inline bool MonthCalendarCtrl::getSelRange(SYSTEMTIME times[]) const {return sendMessageC<bool>(MCM_GETSELRANGE, 0, reinterpret_cast<LPARAM>(times));}

inline bool MonthCalendarCtrl::getToday(SYSTEMTIME& time) const {return sendMessageC<bool>(MCM_GETTODAY, 0, reinterpret_cast<LPARAM>(&time));}

inline bool MonthCalendarCtrl::getUnicodeFormat() const {return sendMessageC<bool>(MCM_GETUNICODEFORMAT);}

inline DWORD MonthCalendarCtrl::hitTest(MCHITTESTINFO& hitTest) {return sendMessageR<DWORD>(MCM_HITTEST, 0, reinterpret_cast<LPARAM>(&hitTest));}

inline COLORREF MonthCalendarCtrl::setColor(int colorType, COLORREF color) {return sendMessageR<COLORREF>(MCM_SETCOLOR, colorType, color);}

inline bool MonthCalendarCtrl::setCurSel(const SYSTEMTIME& time) {return sendMessageR<bool>(MCM_SETCURSEL, 0, reinterpret_cast<LPARAM>(&time));}

inline bool MonthCalendarCtrl::setDayState(int count, const MONTHDAYSTATE dayStates[]) {
	return sendMessageR<bool>(MCM_SETDAYSTATE, count, reinterpret_cast<LPARAM>(dayStates));}

inline int MonthCalendarCtrl::setFirstDayOfWeek(int day, bool* localeVal /* = 0 */) {
	const DWORD temp = sendMessageR<DWORD>(MCM_SETFIRSTDAYOFWEEK, 0, day);
	if(localeVal != 0)
		*localeVal = toBoolean(HIWORD(temp));
	return LOWORD(temp);
}

inline bool MonthCalendarCtrl::setMaxSelCount(int max) {return sendMessageR<bool>(MCM_SETMAXSELCOUNT, max);}

inline int MonthCalendarCtrl::setMonthDelta(int delta) {return sendMessageR<int>(MCM_SETMONTHDELTA, delta);}

inline bool MonthCalendarCtrl::setRange(DWORD flags, const SYSTEMTIME times[]) {
	return sendMessageR<bool>(MCM_SETRANGE, flags, reinterpret_cast<LPARAM>(times));}

inline bool MonthCalendarCtrl::setSelRange(const SYSTEMTIME times[]) {
	return sendMessageR<bool>(MCM_SETSELRANGE, 0, reinterpret_cast<LPARAM>(times));}

inline void MonthCalendarCtrl::setToday(const SYSTEMTIME& time) {sendMessage(MCM_SETTODAY, 0, reinterpret_cast<LPARAM>(&time));}

inline bool MonthCalendarCtrl::setUnicodeFormat(bool unicode /* = true */) {return sendMessageR<bool>(MCM_SETUNICODEFORMAT, unicode);}


// PagerCtrl ////////////////////////////////////////////////////////////////

inline void PagerCtrl::forwardMouse(bool forward) {sendMessage(PGM_FORWARDMOUSE, forward);}

inline COLORREF PagerCtrl::getBkColor() const {return sendMessageC<COLORREF>(PGM_GETBKCOLOR);}

inline int PagerCtrl::getBorder() const {return sendMessageC<int>(PGM_GETBORDER);}

inline int PagerCtrl::getButtonSize() const {return sendMessageC<int>(PGM_GETBUTTONSIZE);}

inline DWORD PagerCtrl::getButtonState(int button) const {return sendMessageC<DWORD>(PGM_GETBUTTONSTATE, 0, button);}

inline void PagerCtrl::getDropTarget(IDropTarget*& dropTarget) const {
	sendMessageC<int>(PGM_GETDROPTARGET, 0, reinterpret_cast<LPARAM>(&dropTarget));}

inline int PagerCtrl::getPos() const {return sendMessageC<int>(PGM_GETPOS);}

inline void PagerCtrl::recalcSize() {sendMessage(PGM_RECALCSIZE);}

inline COLORREF PagerCtrl::setBkColor(COLORREF bgColor) {return sendMessageR<COLORREF>(PGM_SETBKCOLOR, 0, bgColor);}

inline int PagerCtrl::setBorder(int border) {return sendMessageR<int>(PGM_SETBORDER, 0, border);}

inline int PagerCtrl::setButtonSize(int buttonSize) {return sendMessageR<int>(PGM_SETBUTTONSIZE, 0, buttonSize);}

inline void PagerCtrl::setChild(HWND child) {sendMessage(PGM_SETCHILD, 0, reinterpret_cast<LPARAM>(child));}

inline int PagerCtrl::setPos(int pos) {return sendMessageR<int>(PGM_SETPOS, 0, pos);}


// ProgressBarCtrl //////////////////////////////////////////////////////////

inline int ProgressBarCtrl::getHighLimit() const {return sendMessageC<int>(PBM_GETRANGE, false);}

inline int ProgressBarCtrl::getLowLimit() const {return sendMessageC<int>(PBM_GETRANGE, true);}

inline UINT ProgressBarCtrl::getPos() const {return sendMessageC<UINT>(PBM_GETPOS);}

inline void ProgressBarCtrl::getRange(PBRANGE& range) const {sendMessageC<LRESULT>(PBM_GETRANGE, true, reinterpret_cast<LPARAM>(&range));}

inline void ProgressBarCtrl::getRange(int* lower, int* upper) const {
	PBRANGE	range;
	getRange(range);
	if(lower != 0)	*lower = range.iLow;
	if(upper != 0)	*upper = range.iHigh;
}

inline int ProgressBarCtrl::offsetPos(int pos) {return sendMessageR<int>(PBM_DELTAPOS, pos);}

inline COLORREF ProgressBarCtrl::setBarColor(COLORREF color) {return sendMessageR<COLORREF>(PBM_SETBARCOLOR, 0, static_cast<LPARAM>(color));}

inline COLORREF ProgressBarCtrl::setBkColor(COLORREF color) {return sendMessageR<COLORREF>(PBM_SETBKCOLOR, 0, static_cast<LPARAM>(color));}

#ifdef PBM_SETMARQUEE
inline bool ProgressBarCtrl::setMarquee(bool marquee, UINT updateTime /* = 0 */) {sendMessageR<bool>(PBM_SETMARQUEE, marquee, updateTime);}
#endif /* PBM_SETMARQUEE */

inline int ProgressBarCtrl::setPos(int pos) {return sendMessageR<int>(PBM_SETPOS, pos);}

inline DWORD ProgressBarCtrl::setRange(const PBRANGE& range) {return setRange(range.iLow, range.iHigh);}

inline DWORD ProgressBarCtrl::setRange(int lower, int upper) {return sendMessageR<DWORD>(PBM_SETRANGE32, lower, upper);}

inline int ProgressBarCtrl::setStep(int step) {return sendMessageR<int>(PBM_SETSTEP, step);}

inline int ProgressBarCtrl::stepIt() {return sendMessageR<int>(PBM_STEPIT);}


// Rebar ////////////////////////////////////////////////////////////////////

inline void Rebar::beginDrag(UINT band, DWORD pos /* = -1 */) {sendMessage(RB_BEGINDRAG, band, pos);}

inline void Rebar::beginDrag(UINT band, int x, int y) {beginDrag(band, (x & 0xFFFF) | ((y & 0xFFFF) << 16));}

inline bool Rebar::deleteBand(UINT band) {return sendMessageR<bool>(RB_DELETEBAND, band);}

inline void Rebar::dragMove(DWORD pos /* = -1 */) {sendMessage(RB_DRAGMOVE, 0, pos);}

inline void Rebar::dragMove(int x, int y) {dragMove((x & 0xFFFF) | ((y & 0xFFFF) << 16));}

inline void Rebar::endDrag() {sendMessage(RB_ENDDRAG);}

inline void Rebar::getBandBorders(int band, RECT& rect) const {sendMessageC<int>(RB_GETBANDBORDERS, band, reinterpret_cast<LPARAM>(&rect));}

inline UINT Rebar::getBandCount() const {return sendMessageC<UINT>(RB_GETBANDCOUNT);}

inline bool Rebar::getBandInfo(int band, REBARBANDINFO& info) const {
	return sendMessageC<bool>(RB_GETBANDINFO, band, reinterpret_cast<LPARAM>(&info));}

inline UINT Rebar::getBarHeight() const {return sendMessageC<UINT>(RB_GETBARHEIGHT);}

inline bool Rebar::getBarInfo(REBARINFO& info) const {return sendMessageC<bool>(RB_GETBARINFO, 0, reinterpret_cast<LPARAM>(&info));}

inline COLORREF Rebar::getBkColor() const {return sendMessageC<int>(RB_GETBKCOLOR);}

inline bool Rebar::getColorScheme(COLORSCHEME& scheme) const {return sendMessageC<bool>(RB_GETCOLORSCHEME, 0, reinterpret_cast<LPARAM>(&scheme));}

inline void Rebar::getDropTarget(IDropTarget*& dropTarget) const {sendMessageC<int>(RB_GETDROPTARGET, 0, reinterpret_cast<LPARAM>(&dropTarget));}

inline HIMAGELIST Rebar::getImageList() const {
	AutoZeroCB<REBARINFO> rbi;
	rbi.fMask = RBIM_IMAGELIST;
	return getBarInfo(rbi) ? rbi.himl : 0;
}

inline HPALETTE Rebar::getPalette() const {return reinterpret_cast<HPALETTE>(sendMessageC<LRESULT>(RB_GETPALETTE));}

inline bool Rebar::getRect(int band, RECT& rect) const {return sendMessageC<bool>(RB_GETRECT, band, reinterpret_cast<LPARAM>(&rect));}

inline UINT Rebar::getRowCount() const {return sendMessageC<UINT>(RB_GETROWCOUNT);}

inline UINT Rebar::getRowHeight(int band) const {sendMessageC<UINT>(RB_GETROWHEIGHT, band);}

inline COLORREF Rebar::getTextColor() const {return sendMessageC<COLORREF>(RB_GETTEXTCOLOR);}

inline HWND Rebar::getToolTips() const {return reinterpret_cast<HWND>(sendMessageC<LRESULT>(RB_GETTOOLTIPS));}

inline bool Rebar::getUnicodeFormat() const {return sendMessageC<bool>(RB_GETUNICODEFORMAT);}

inline int Rebar::hitTest(RBHITTESTINFO& info) {return sendMessageC<int>(RB_HITTEST, 0, reinterpret_cast<LPARAM>(&info));}

inline int Rebar::idToIndex(UINT id) const {return sendMessageC<int>(RB_IDTOINDEX, id);}

inline bool Rebar::insertBand(UINT band, const REBARBANDINFO& info) {
	return sendMessageC<bool>(RB_INSERTBAND, band, reinterpret_cast<LPARAM>(&info));}

inline void Rebar::lockBands(bool lock) {
	const UINT count = getBandCount();
	AutoZeroCB<REBARBANDINFO> rbbi;

	rbbi.fMask = RBBIM_STYLE;
	for(UINT i = 0; i < count; ++i) {
		getBandInfo(i, rbbi);
		if(!toBoolean(rbbi.fStyle & RBBS_GRIPPERALWAYS)) {
			rbbi.fStyle |= RBBS_GRIPPERALWAYS;
			setBandInfo(i, rbbi);
			rbbi.fStyle &= ~RBBS_GRIPPERALWAYS;
			setBandInfo(i, rbbi);
		}
		if(lock)
			rbbi.fStyle |= RBBS_NOGRIPPER;
		else
			rbbi.fStyle &= ~RBBS_NOGRIPPER;
		setBandInfo(i, rbbi);
	}
}

inline void Rebar::maximizeBand(UINT band) {sendMessage(RB_MAXIMIZEBAND, band, false);}

inline void Rebar::minimizeBand(UINT band) {sendMessage(RB_MINIMIZEBAND, band);}

inline bool Rebar::moveBand(UINT from, UINT to) {assert(to >= 0 && to < getBandCount()); return sendMessageR<bool>(RB_MOVEBAND, from, to);}

inline void Rebar::pushChevron(UINT band, LPARAM lParam) {
#ifndef RB_PUSHCHEVRON
	const UINT RB_PUSHCHEVRON = WM_USER + 43;
#endif /* !RB_PUSHCHEVRON */
	sendMessage(RB_PUSHCHEVRON, band, lParam);
}

inline void Rebar::restoreBand(UINT band) {sendMessage(RB_MAXIMIZEBAND, band, true);}

inline bool Rebar::setBandInfo(UINT band, const REBARBANDINFO& info) {
	return sendMessageR<bool>(RB_SETBANDINFO, band, reinterpret_cast<LPARAM>(&info));}

inline bool Rebar::setBarInfo(const REBARINFO& info) {return sendMessageR<bool>(RB_SETBARINFO, 0, reinterpret_cast<LPARAM>(&info));}

inline COLORREF Rebar::setBkColor(COLORREF color) {return sendMessageR<COLORREF>(RB_SETBKCOLOR, 0, color);}

inline bool Rebar::setImageList(HIMAGELIST imageList) {
	AutoZeroCB<REBARINFO> rbi;
	rbi.fMask = RBIM_IMAGELIST;
	rbi.himl = imageList;
	return setBarInfo(rbi);
}

inline HWND Rebar::setOwner(HWND owner) {return reinterpret_cast<HWND>(sendMessage(RB_SETPARENT, reinterpret_cast<WPARAM>(owner)));}

inline HPALETTE Rebar::setPalette(HPALETTE palette) {
	return reinterpret_cast<HPALETTE>(sendMessage(RB_SETPALETTE, 0, reinterpret_cast<LPARAM>(palette)));}

inline void Rebar::setColorScheme(const COLORSCHEME& scheme) {sendMessage(RB_SETCOLORSCHEME, 0, reinterpret_cast<LPARAM>(&scheme));}

inline COLORREF Rebar::setTextColor(COLORREF color) {return sendMessageR<COLORREF>(RB_SETTEXTCOLOR, 0, color);}

inline void Rebar::setToolTips(HWND toolTips) {sendMessage(RB_SETTOOLTIPS, reinterpret_cast<WPARAM>(toolTips));}

inline bool Rebar::setUnicodeFormat(bool unicode /* = true */) {return sendMessageR<bool>(RB_SETUNICODEFORMAT, unicode);}

inline bool Rebar::showBand(UINT band, bool show) {return sendMessageR<bool>(RB_SHOWBAND, band, show);}

inline bool Rebar::sizeToRect(const RECT& rect) {return sendMessageR<bool>(RB_SIZETORECT, 0, reinterpret_cast<LPARAM>(&rect));}

#ifdef RB_GETMARGINS
inline void Rebar::getBandMargins(MARGINS& margins) const {SendMessage(RB_GETBANDMARGINS, 0, reinterpret_cast<LPARAM>(&margins));}

inline void Rebar::setWindowTheme(const WCHAR* styleName) {SendMessage(RB_SETWINDOWTHEME, 0, reinterpret_cast<LPARAM>(styleName));}
#endif /* RB_GETMARGINS */


// StatusBar ////////////////////////////////////////////////////////////////

inline bool StatusBar::getBorders(int borders[]) const {
	assert(borders != 0); return sendMessageC<bool>(SB_GETBORDERS, 0, reinterpret_cast<LPARAM>(borders));}

inline bool StatusBar::getBorders(int& horz, int& vert, int& spacing) const {
	int	borders[3];
	if(getBorders(borders)) {
		horz = borders[0];
		vert = borders[1];
		spacing = borders[2];
		return true;
	}
	return false;
}

inline HICON StatusBar::getIcon(int pane) const {return reinterpret_cast<HICON>(sendMessageC<LRESULT>(SB_GETICON, pane));}

inline int StatusBar::getParts(int count, int parts[]) const {
	assert(parts != 0); return sendMessageC<int>(SB_GETPARTS, count, reinterpret_cast<LPARAM>(parts));}

inline bool StatusBar::getRect(int pane, RECT& rect) const {return sendMessageC<bool>(SB_GETRECT, pane, reinterpret_cast<LPARAM>(&rect));}

inline int StatusBar::getText(int pane, TCHAR* text, int* type /* = 0 */) const {
	const DWORD temp = sendMessageC<DWORD>(SB_GETTEXT, pane, reinterpret_cast<LPARAM>(text));
	if(type != 0)
		*type = HIWORD(temp);
	return LOWORD(temp);
}

inline int StatusBar::getTextLength(int pane, int* type /* = 0 */) const {
	const DWORD temp = sendMessageC<DWORD>(SB_GETTEXT, pane);
	if(type != 0)
		*type = HIWORD(temp);
	return LOWORD(temp);
}

inline void StatusBar::getTipText(int pane, TCHAR* text, int maxLength) const {
	sendMessageC<int>(SB_GETTIPTEXT, MAKEWPARAM(pane, maxLength), reinterpret_cast<LPARAM>(text));}

inline bool StatusBar::getUnicodeFormat() const {return sendMessageC<bool>(SB_GETUNICODEFORMAT);}

inline bool StatusBar::isSimple() const {return sendMessageC<bool>(SB_ISSIMPLE);}

inline COLORREF StatusBar::setBkColor(COLORREF color) {return sendMessageR<COLORREF>(SB_SETBKCOLOR, color);}

inline bool StatusBar::setIcon(int pane, HICON icon) {return sendMessageR<bool>(SB_SETICON, pane, reinterpret_cast<LPARAM>(icon));}

inline void StatusBar::setMinHeight(int height) {sendMessage(SB_SETMINHEIGHT, height);}

inline bool StatusBar::setParts(int count, int parts[]) {return sendMessageR<bool>(SB_SETPARTS, count, reinterpret_cast<LPARAM>(parts));}

inline bool StatusBar::setSimple(bool simple /* = true */) {return sendMessageR<bool>(SB_SIMPLE, simple);}

inline bool StatusBar::setText(int pane, const TCHAR* text, int type /* = 0 */) {
	return sendMessageR<bool>(SB_SETTEXT, pane | type, reinterpret_cast<LPARAM>(text));}

inline void StatusBar::setTipText(int pane, const TCHAR* text) {sendMessage(SB_SETTIPTEXT, pane, reinterpret_cast<LPARAM>(text));}

inline bool StatusBar::setUnicodeFormat(bool unicode /* = true */) {return sendMessageR<bool>(SB_SETUNICODEFORMAT, unicode);}


// TabCtrl //////////////////////////////////////////////////////////////////

inline void TabCtrl::adjustRect(bool larger, RECT& rect) {sendMessage(TCM_ADJUSTRECT, larger, reinterpret_cast<LPARAM>(&rect));}

inline bool TabCtrl::deleteAllItems() {sendMessage(TCM_DELETEALLITEMS);}

inline bool TabCtrl::deleteItem(int iItem) {return sendMessageR<bool>(TCM_DELETEITEM, iItem);}

inline void TabCtrl::deselectAll(bool bExcludeFocus) {sendMessage(TCM_DESELECTALL, bExcludeFocus);}

inline void TabCtrl::drawItem(const DRAWITEMSTRUCT& drawItemStruct) {}

inline int TabCtrl::getCurFocus() const {return sendMessageC<int>(TCM_GETCURFOCUS);}

inline int TabCtrl::getCurSel() const {return sendMessageC<int>(TCM_GETCURSEL);}

inline DWORD TabCtrl::getExtendedStyle() const {return sendMessageC<DWORD>(TCM_GETEXTENDEDSTYLE);}

inline HIMAGELIST TabCtrl::getImageList() const {return reinterpret_cast<HIMAGELIST>(sendMessageC<LRESULT>(TCM_GETIMAGELIST));}

inline bool TabCtrl::getItem(int index, TCITEM& item) const {return sendMessageC<bool>(TCM_GETITEM, index, reinterpret_cast<LPARAM>(&item));}

inline int TabCtrl::getItemCount() const {return sendMessageC<int>(TCM_GETITEMCOUNT);}

inline bool TabCtrl::getItemRect(int index, RECT& rect) const {return sendMessageC<bool>(TCM_GETITEMRECT, index, reinterpret_cast<LPARAM>(&rect));}

inline bool TabCtrl::getItemState(int index, DWORD mask, DWORD& state) const {
	assertValidAsWindow();
	TCITEM item;
	item.mask = TCIF_STATE;
	item.dwStateMask = mask;
	if(!getItem(index, item))
		return false;
	state = item.dwState;
	return true;
}

inline int TabCtrl::getRowCount() const {return sendMessageC<int>(TCM_GETROWCOUNT);}

inline HWND TabCtrl::getToolTips() const {return reinterpret_cast<HWND>(sendMessageC<LRESULT>(TCM_GETTOOLTIPS));}

inline bool TabCtrl::highlightItem(int index, bool highlight /* = true */) {return sendMessageR<bool>(TCM_HIGHLIGHTITEM, index, highlight);}

inline int TabCtrl::hitTest(TCHITTESTINFO& info) const {return sendMessageC<int>(TCM_HITTEST, 0, reinterpret_cast<LPARAM>(&info));}

inline bool TabCtrl::insertItem(int index, const TCITEM& item) {return sendMessageR<bool>(TCM_INSERTITEM, index, reinterpret_cast<LPARAM>(&item));}

inline bool TabCtrl::insertItem(int index, const TCHAR* text) {
	TCITEM item;
	item.mask = TCIF_TEXT;
	item.pszText = const_cast<TCHAR*>(text);
	return insertItem(index, item);
}

inline bool TabCtrl::insertItem(int index, const TCHAR* text, int image) {
	TCITEM item;
	item.mask = TCIF_IMAGE | TCIF_TEXT;
	item.pszText = const_cast<TCHAR*>(text);
	item.iImage = image;
	return insertItem(index, item);
}

inline bool TabCtrl::insertItem(UINT mask, int index, const TCHAR* text, int image, LPARAM lParam) {
	TCITEM item;
	item.mask = mask;
	item.pszText = const_cast<TCHAR*>(text);
	item.iImage = image;
	item.lParam = lParam;
	return insertItem(index, item);
}

inline void TabCtrl::removeImage(int iImage) {sendMessage(TCM_REMOVEIMAGE, iImage);}

inline int TabCtrl::setCurFocus(int index) {sendMessage(TCM_SETCURFOCUS, index);}

inline int TabCtrl::setCurSel(int index) {sendMessage(TCM_SETCURSEL, index);}

inline DWORD TabCtrl::setExtendedStyle(DWORD newStyle, DWORD exMask /* = 0 */) {
	return sendMessageR<DWORD>(TCM_SETEXTENDEDSTYLE, exMask, newStyle);}

inline HIMAGELIST TabCtrl::setImageList(HIMAGELIST imageList) {
	return reinterpret_cast<HIMAGELIST>(sendMessage(TCM_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(imageList)));}

inline bool TabCtrl::setItem(int index, const TCITEM& item) {return sendMessageR<bool>(TCM_SETITEM, index, reinterpret_cast<LPARAM>(&item));}

inline bool TabCtrl::setItemExtra(int bytes) {return sendMessageR<bool>(TCM_SETITEMEXTRA, bytes);}

inline SIZE TabCtrl::setItemSize(const SIZE& size) {
	const DWORD temp = sendMessageR<DWORD>(TCM_SETITEMSIZE, 0, MAKELPARAM(size.cx, size.cy));
	SIZE s = {LOWORD(temp), HIWORD(temp)};
	return s;
}

inline bool TabCtrl::setItemState(int index, DWORD mask, DWORD state) {
	TCITEM item;
	item.mask = TCIF_STATE;
	item.dwState = state;
	item.dwStateMask = mask;
	return setItem(index, item);
}

inline int TabCtrl::setMinTabWidth(int cx) {return sendMessageR<int>(TCM_SETMINTABWIDTH, 0, cx);}

inline void TabCtrl::setPadding(const SIZE& size) {sendMessage(TCM_SETPADDING, 0, MAKELPARAM(size.cx, size.cy));}

inline void TabCtrl::setToolTips(HWND toolTips) {sendMessage(TCM_SETTOOLTIPS, 0, reinterpret_cast<LPARAM>(toolTips));}


// Toolbar //////////////////////////////////////////////////////////////////

inline int Toolbar::addBitmap(int count, UINT bitmapID) {
	TBADDBITMAP tbab;
	tbab.hInst = ::GetModuleHandle(0);
	tbab.nID = bitmapID;
	return sendMessageR<int>(TB_ADDBITMAP, count, reinterpret_cast<LPARAM>(&tbab));
}

inline int Toolbar::addBitmap(int count, HBITMAP bitmap) {
	TBADDBITMAP	tbab;
	tbab.hInst = 0;
	tbab.nID = reinterpret_cast<UINT_PTR>(bitmap);
	return sendMessageR<int>(TB_ADDBITMAP, count, reinterpret_cast<LPARAM>(&tbab));
}

inline bool Toolbar::addButtons(int count, const TBBUTTON buttons[]) {
	return sendMessageR<bool>(TB_ADDBUTTONS, count, reinterpret_cast<LPARAM>(buttons));}

inline int Toolbar::addString(UINT stringID) {
	return sendMessageR<int>(TB_ADDSTRING, reinterpret_cast<WPARAM>(::GetModuleHandle(0)), stringID);}

inline int Toolbar::addStrings(const TCHAR* strings) {return sendMessageR<int>(TB_ADDSTRING, 0, reinterpret_cast<LPARAM>(strings));}

inline void Toolbar::autoSize() {sendMessage(TB_AUTOSIZE);}

inline bool Toolbar::changeBitmap(int id, int bitmap) {return sendMessageR<bool>(TB_CHANGEBITMAP, id, bitmap);}

inline bool Toolbar::checkButton(int id, bool check /* = true */) {return sendMessageR<bool>(TB_CHECKBUTTON, id, check);}

inline UINT Toolbar::commandToIndex(int id) const {return sendMessageC<UINT>(TB_COMMANDTOINDEX, id);}

inline void Toolbar::customize() {sendMessage(TB_CUSTOMIZE);}

inline bool Toolbar::deleteButton(int index) {return sendMessageR<bool>(TB_DELETEBUTTON, index);}

inline bool Toolbar::enableButton(int id, bool enable /* = true */) {return sendMessageR<bool>(TB_ENABLEBUTTON, id, enable);}

inline bool Toolbar::getAnchorHighlight() const {return sendMessageC<bool>(TB_GETANCHORHIGHLIGHT);}

inline int Toolbar::getBitmap(int id) const {return sendMessageC<int>(TB_GETBITMAP, id);}

inline DWORD Toolbar::getBitmapFlags() const {return sendMessageC<DWORD>(TB_GETBITMAPFLAGS);}

inline bool Toolbar::getButton(int index, TBBUTTON& button) const {return sendMessageC<bool>(TB_GETBUTTON, index, reinterpret_cast<LPARAM>(&button));}

inline int Toolbar::getButtonCount() const {return sendMessageC<int>(TB_BUTTONCOUNT);}

inline bool Toolbar::getButtonInfo(int id, TBBUTTONINFO& info) const {
	return sendMessageC<bool>(TB_GETBUTTONINFO, id, reinterpret_cast<LPARAM>(&info));}

inline SIZE Toolbar::getButtonSize() const {
	const DWORD temp = sendMessageC<DWORD>(TB_GETBUTTONSIZE);
	const SIZE size = {LOWORD(temp), HIWORD(temp)};
	return size;
}

inline int Toolbar::getButtonText(int id, TCHAR* text) const {return sendMessageC<int>(TB_GETBUTTONTEXT, id, reinterpret_cast<LPARAM>(text));}

inline int Toolbar::getButtonTextLength(int id) const {return sendMessageC<int>(TB_GETBUTTONTEXT, id);}

inline bool Toolbar::getColorScheme(COLORSCHEME& scheme) const {return sendMessageC<bool>(TB_GETCOLORSCHEME, 0, reinterpret_cast<LPARAM>(&scheme));}

inline HIMAGELIST Toolbar::getDisabledImageList() const {return reinterpret_cast<HIMAGELIST>(sendMessageC<LRESULT>(TB_GETDISABLEDIMAGELIST));}

inline HRESULT Toolbar::getDropTarget(IDropTarget*& dropTarget) const {
	return static_cast<HRESULT>(sendMessageC<LRESULT>(
		TB_GETOBJECT, reinterpret_cast<WPARAM>(&IID_IDropTarget), reinterpret_cast<LPARAM>(&dropTarget)));}

inline DWORD Toolbar::getExtendedStyle() const {return sendMessageC<DWORD>(TB_GETEXTENDEDSTYLE);}

inline HIMAGELIST Toolbar::getHotImageList() const {return reinterpret_cast<HIMAGELIST>(sendMessageC<LRESULT>(TB_GETHOTIMAGELIST));}

inline int Toolbar::getHotItem() const {return sendMessageC<int>(TB_GETHOTITEM);}

inline HIMAGELIST Toolbar::getImageList() const {return reinterpret_cast<HIMAGELIST>(sendMessageC<LRESULT>(TB_GETIMAGELIST));}

inline void Toolbar::getInsertMark(TBINSERTMARK& mark) const {sendMessageC<int>(TB_GETINSERTMARK, 0, reinterpret_cast<LPARAM>(&mark));}

inline COLORREF Toolbar::getInsertMarkColor() const {return sendMessageC<COLORREF>(TB_GETINSERTMARKCOLOR);}

inline bool Toolbar::getItemRect(int index, RECT& rect) const {return sendMessageC<bool>(TB_GETITEMRECT, index, reinterpret_cast<LPARAM>(&rect));}

inline bool Toolbar::getMaxSize(SIZE& size) const {return sendMessageC<bool>(TB_GETMAXSIZE, 0, reinterpret_cast<LPARAM>(&size));}

inline int Toolbar::getMaxTextRows() const {return sendMessageC<int>(TB_GETTEXTROWS);}

inline void Toolbar::getPadding(SIZE& padding) const {
	const DWORD	temp = sendMessageC<DWORD>(TB_GETPADDING);
	padding.cx = LOWORD(temp);
	padding.cy = HIWORD(temp);
}

inline bool Toolbar::getRect(int id, RECT& rect) const {return sendMessageC<bool>(TB_GETRECT, id, reinterpret_cast<LPARAM>(&rect));}

inline int Toolbar::getRows() const {return sendMessageC<int>(TB_GETROWS);}

inline int Toolbar::getState(int id) const {return sendMessageC<int>(TB_GETSTATE, id);}

#ifdef TB_GETSTRING
inline int Toolbar::getString(int index, TCHAR* text, int maxLength) const {
	return sendMessageC<int>(TB_GETSTRING, MAKEWPARAM(maxLength, index), reinterpret_cast<LPARAM>(text));}
#endif

inline DWORD Toolbar::getStyle() const {return sendMessageC<DWORD>(TB_GETSTYLE);}

inline HWND Toolbar::getToolTips() const {return reinterpret_cast<HWND>(sendMessageC<LRESULT>(TB_GETTOOLTIPS));}

inline bool Toolbar::getUnicodeFormat() const {return sendMessageC<bool>(TB_GETUNICODEFORMAT);}

inline bool Toolbar::hideButton(int id, bool bHide /* = true */) {return sendMessageC<bool>(TB_HIDEBUTTON, id, bHide);}

inline int Toolbar::hitTest(const POINT& pt) const {return sendMessageC<int>(TB_HITTEST, 0, reinterpret_cast<LPARAM>(&pt));}

inline bool Toolbar::indeterminate(int id, bool isIndeterminate /* = true */) {return sendMessageC<bool>(TB_INDETERMINATE, id, isIndeterminate);}

inline bool Toolbar::insertButton(int index, const TBBUTTON& button) {
	return sendMessageC<bool>(TB_INSERTBUTTON, index, reinterpret_cast<LPARAM>(&button));}

inline bool Toolbar::insertMarkHitTest(const POINT& pt, const TBINSERTMARK& mark) const {
	return sendMessageC<bool>(TB_INSERTMARKHITTEST, reinterpret_cast<WPARAM>(&pt), reinterpret_cast<LPARAM>(&mark));}

inline bool Toolbar::isButtonChecked(int id) const {return sendMessageC<bool>(TB_ISBUTTONCHECKED, id);}

inline bool Toolbar::isButtonEnabled(int id) const {return sendMessageC<bool>(TB_ISBUTTONENABLED, id);}

inline bool Toolbar::isButtonHidden(int id) const {return sendMessageC<bool>(TB_ISBUTTONHIDDEN, id);}

inline bool Toolbar::isButtonHighlighted(int id) const {return sendMessageC<bool>(TB_ISBUTTONHIGHLIGHTED, id);}

inline bool Toolbar::isButtonIndeterminate(int id) const {return sendMessageC<bool>(TB_ISBUTTONINDETERMINATE, id);}

inline bool Toolbar::isButtonPressed(int id) const {return sendMessageC<bool>(TB_ISBUTTONPRESSED, id);}

inline void Toolbar::loadImages(int imageID) {sendMessage(TB_LOADIMAGES, imageID, reinterpret_cast<LPARAM>(::GetModuleHandle(0)));}

inline void Toolbar::loadStdImages(int imageID) {sendMessage(TB_LOADIMAGES, imageID, reinterpret_cast<LPARAM>(HINST_COMMCTRL));}

inline bool Toolbar::mapAccelerator(TCHAR ch, UINT& id) {return sendMessageR<bool>(TB_MAPACCELERATOR, ch, reinterpret_cast<LPARAM>(&id));}

inline bool Toolbar::markButton(int id, bool highlight /* = true */) {return sendMessageR<bool>(TB_MARKBUTTON, id, highlight);}

inline bool Toolbar::moveButton(int from, int to) {return sendMessageR<bool>(TB_MOVEBUTTON, from, to);}

inline bool Toolbar::pressButton(int id, bool press /* = true */) {return sendMessageR<bool>(TB_PRESSBUTTON, id, press);}

inline bool Toolbar::replaceBitmap(const TBREPLACEBITMAP& bitmap) {
	return sendMessageR<bool>(TB_REPLACEBITMAP, 0, reinterpret_cast<LPARAM>(&bitmap));}

inline void Toolbar::restoreState(HKEY keyRoot, const TCHAR* subKey, const TCHAR* valueName) {
	TBSAVEPARAMS tbsp = {keyRoot, subKey, valueName};
	sendMessage(TB_SAVERESTORE, false, reinterpret_cast<LPARAM>(&tbsp));
}

inline void Toolbar::saveState(HKEY keyRoot, const TCHAR* subKey, const TCHAR* valueName) {
	TBSAVEPARAMS tbsp = {keyRoot, subKey, valueName};
	sendMessage(TB_SAVERESTORE, true, reinterpret_cast<LPARAM>(&tbsp));
}

inline bool Toolbar::setAnchorHighlight(bool enable /* = true */) {return sendMessageR<bool>(TB_SETANCHORHIGHLIGHT, enable);}

inline bool Toolbar::setBitmapSize(const SIZE& size) {return setBitmapSize(size.cx, size.cy);}

inline bool Toolbar::setBitmapSize(int cx, int cy) {return sendMessageR<bool>(TB_SETBITMAPSIZE, 0, MAKELPARAM(cx, cy));}

inline bool Toolbar::setButtonInfo(int id, const TBBUTTONINFO& info) {
	return sendMessageR<bool>(TB_SETBUTTONINFO, id, reinterpret_cast<LPARAM>(&info));}

inline bool Toolbar::setButtonSize(const SIZE& size) {return sendMessageR<bool>(TB_SETBUTTONSIZE, 0, MAKELPARAM(size.cx, size.cy));}

inline bool Toolbar::setButtonSize(int cx, int cy) {return sendMessageR<bool>(TB_SETBUTTONSIZE, 0, MAKELPARAM(cx, cy));}

inline void Toolbar::setButtonStructSize(std::size_t size /* = sizeof(TBBUTTON) */) {sendMessage(TB_BUTTONSTRUCTSIZE, size);}

inline void Toolbar::setButtonText(int id, const TCHAR* text) {
	AutoZeroCB<TBBUTTONINFO> tbi;
	tbi.dwMask = TBIF_TEXT;
	tbi.pszText = const_cast<TCHAR*>(text);
	setButtonInfo(id, tbi);
}

inline bool Toolbar::setButtonWidth(int cxMin, int cxMax) {return sendMessageR<bool>(TB_SETBUTTONWIDTH, 0, MAKELPARAM(cxMin, cxMax));}

inline void Toolbar::setColorScheme(const COLORSCHEME& scheme) {sendMessage(TB_SETCOLORSCHEME, 0, reinterpret_cast<LPARAM>(&scheme));}

inline bool Toolbar::setCommandID(int index, UINT id) {return sendMessageR<bool>(TB_SETCMDID, index, id);}

inline HIMAGELIST Toolbar::setDisabledImageList(HIMAGELIST imageList) {
	return reinterpret_cast<HIMAGELIST>(sendMessage(TB_SETDISABLEDIMAGELIST, 0, reinterpret_cast<LPARAM>(imageList)));}

inline DWORD Toolbar::setDrawTextFlags(DWORD mask, DWORD flags) {return sendMessageR<DWORD>(TB_SETDRAWTEXTFLAGS, mask, flags);}

inline DWORD Toolbar::setExtendedStyle(DWORD exStyle) {return sendMessageR<DWORD>(TB_SETEXTENDEDSTYLE, 0, exStyle);}

inline HIMAGELIST Toolbar::setHotImageList(HIMAGELIST imageList) {
	return reinterpret_cast<HIMAGELIST>(sendMessage(TB_SETHOTIMAGELIST, 0, reinterpret_cast<LPARAM>(imageList)));}

inline int Toolbar::setHotItem(int index) {return sendMessageR<int>(TB_SETHOTITEM, index);}

inline HIMAGELIST Toolbar::setImageList(HIMAGELIST imageList) {
	return reinterpret_cast<HIMAGELIST>(sendMessage(TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(imageList)));}

inline bool Toolbar::setIndent(int indent) {return sendMessageR<bool>(TB_SETINDENT, indent);}

inline void Toolbar::setInsertMark(const TBINSERTMARK& mark) {sendMessage(TB_SETINSERTMARK, 0, reinterpret_cast<LPARAM>(&mark));}

inline COLORREF Toolbar::setInsertMarkColor(COLORREF color) {return sendMessageR<COLORREF>(TB_SETINSERTMARKCOLOR, 0, color);}

inline bool Toolbar::setMaxTextRows(int count) {return sendMessageR<bool>(TB_SETMAXTEXTROWS, count);}

inline void Toolbar::setOwner(HWND owner) {sendMessage(TB_SETPARENT, reinterpret_cast<WPARAM>(owner));}

inline void Toolbar::setPadding(int cx, int cy, SIZE* padding /* = 0 */) {
	const DWORD temp = sendMessageR<DWORD>(TB_SETPADDING, 0, MAKELPARAM(cx, cy));
	if(padding != 0) {
		padding->cx = LOWORD(temp);
		padding->cy = HIWORD(temp);
	}
}

inline void Toolbar::setRow(int count, bool larger, const RECT& rect) {
	sendMessage(TB_SETROWS, MAKELPARAM(count, larger), reinterpret_cast<LPARAM>(&rect));}

inline bool Toolbar::setState(int id, UINT state) {return sendMessageR<bool>(TB_SETSTATE, id, state);}

inline void Toolbar::setStyle(DWORD style) {sendMessage(TB_SETSTYLE, 0, style);}

inline void Toolbar::setToolTips(HWND toolTips) {sendMessage(TB_SETTOOLTIPS, reinterpret_cast<WPARAM>(toolTips));}

inline bool Toolbar::setUnicodeFormat(bool unicode /* = true */) {return sendMessageR<bool>(TB_SETUNICODEFORMAT, unicode);}

#ifdef TB_GETMETRICS
inline void Toolbar::getMetrics(TBMETRICS& metrics) const {sendMessageC<int>(TB_GETMETRICS, 0, reinterpret_cast<LPARAM>(&metrics));}

inline void Toolbar::setMetrics(const TBMETRICS& metrics) {sendMessage(TB_SETMETRICS, 0, reinterpret_cast<LPARAM>(&metrics));}

inline void Toolbar::setWindowTheme(const wchar_t* styleName) {sendMessage(TB_SETWINDOWTHEME, 0, reinterpret_cast<LPARAM>(styleName));}
#endif /* TB_GETMETRICS */


// ToolTipCtrl //////////////////////////////////////////////////////////////

inline bool ToolTipCtrl::activate(bool active) {return sendMessageR<bool>(TTM_ACTIVATE, active);}

inline bool ToolTipCtrl::addTool(const TOOLINFO& toolInfo) {return sendMessageR<bool>(TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&toolInfo));}

inline bool ToolTipCtrl::addTool(HWND container, UINT id,
		UINT flags, const RECT& toolRect, const TCHAR* text /* = LPSTR_TEXTCALLBACK */, LPARAM lParam /* = 0 */) {
	assert(toBoolean(::IsWindow(container)));
	AutoZeroCB<TOOLINFO> ti;
	ti.uFlags = flags & ~TTF_IDISHWND;
	ti.hwnd = container;
	ti.uId = id;
	ti.rect = toolRect;
	ti.hinst = ::GetModuleHandle(0);
	ti.lpszText = const_cast<TCHAR*>(text);
	ti.lParam = lParam;
	return addTool(ti);
}

inline bool ToolTipCtrl::addTool(HWND tool, UINT flags, const TCHAR* text /* = LPSTR_TEXTCALLBACK */, LPARAM lParam /* = 0 */) {
	assert(toBoolean(::IsWindow(tool)));
	AutoZeroCB<TOOLINFO> ti;
	ti.uFlags = flags | TTF_IDISHWND;
	ti.hwnd = ::GetParent(tool);
	ti.uId = reinterpret_cast<UINT_PTR>(tool);
	ti.hinst = ::GetModuleHandle(0);
	ti.lpszText = const_cast<TCHAR*>(text);
	ti.lParam = lParam;
	return addTool(ti);
}

inline bool ToolTipCtrl::adjustRect(RECT& rect, bool larger /* = true */) {
	return sendMessageR<bool>(TTM_ADJUSTRECT, larger, reinterpret_cast<LPARAM>(&rect));}

inline bool ToolTipCtrl::create(HWND parent, const RECT& rect /* = DefaultWindowRect() */,
		const TCHAR* windowName /* = 0 */, INT_PTR id /* = 0 */, DWORD style /* = 0 */, DWORD exStyle /*= 0 */) {
	if(CommonControl<ToolTipCtrl,
			AdditiveWindowStyles<WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, WS_EX_TOOLWINDOW> >::create(
			parent, rect, windowName, id, style, exStyle)) {
		setWindowPos(HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);	// make top explicitly
		return true;
	}
	return false;
}

inline void ToolTipCtrl::deleteTool(HWND window, UINT id /* = 0 */) {
	AutoZeroCB<TOOLINFO> ti;
	ti.hwnd = window;
	ti.uId = id;
	sendMessage(TTM_DELTOOL, 0, reinterpret_cast<LPARAM>(&ti));
}

inline void ToolTipCtrl::deleteTool(HWND window, HWND control) {
	AutoZeroCB<TOOLINFO> ti;
	ti.uFlags = TTF_IDISHWND;
	ti.hwnd = window;
	ti.uId = reinterpret_cast<UINT_PTR>(control);
	sendMessage(TTM_DELTOOL, 0, reinterpret_cast<LPARAM>(&ti));
}

inline bool ToolTipCtrl::enumTools(UINT index, TOOLINFO& toolInfo) const {
	return sendMessageC<bool>(TTM_ENUMTOOLS, index, reinterpret_cast<LPARAM>(&toolInfo));}

inline SIZE ToolTipCtrl::getBubbleSize(const TOOLINFO& toolInfo) const {
	const DWORD temp = sendMessageC<DWORD>(TTM_GETBUBBLESIZE, 0, reinterpret_cast<LPARAM>(&toolInfo));
	const SIZE size = {LOWORD(temp), HIWORD(temp)};
	return size;
}

inline bool ToolTipCtrl::getCurrentTool(TOOLINFO& toolInfo) const {
	return sendMessageC<bool>(TTM_GETCURRENTTOOL, 0, reinterpret_cast<LPARAM>(&toolInfo));}

inline int ToolTipCtrl::getDelayTime(DWORD duration) const {return sendMessageC<int>(TTM_GETDELAYTIME, duration);}

inline void ToolTipCtrl::getMargin(RECT& rect) const {sendMessageC<int>(TTM_GETMARGIN, 0, reinterpret_cast<LPARAM>(&rect));}

inline int ToolTipCtrl::getMaxTipWidth() const {return sendMessageC<int>(TTM_GETMAXTIPWIDTH);}

inline void ToolTipCtrl::getText(TCHAR* text, HWND window, UINT toolID /* = 0 */) const {
	assertValidAsWindow();
	AutoZero<TOOLINFO> ti;
	ti.hwnd = window;
	ti.uId = toolID;
	ti.lpszText = text;
	sendMessageC<int>(TTM_GETTEXT, 0, reinterpret_cast<LPARAM>(&ti));
}

inline COLORREF ToolTipCtrl::getTipBkColor() const {return sendMessageC<COLORREF>(TTM_GETTIPBKCOLOR);}

inline COLORREF ToolTipCtrl::getTipTextColor() const {return sendMessageC<COLORREF>(TTM_GETTIPTEXTCOLOR);}

#if(_WIN32_WINNT >= 0x0501)
inline void ToolTipCtrl::getTitle(TTGETTITLE& title) const {sendMessageC<LRESULT>(TTM_GETTITLE, 0, reinterpret_cast<LPARAM>(&title));}
#endif /* _WIN32_WINNT >= 0x0501 */

inline int ToolTipCtrl::getToolCount() const {return sendMessageC<int>(TTM_GETTOOLCOUNT);}

inline bool ToolTipCtrl::getToolInfo(TOOLINFO& toolInfo, HWND window, UINT toolID /* = 0 */) const {
	std::memset(&toolInfo, 0, sizeof(TOOLINFO));
	toolInfo.cbSize = sizeof(TOOLINFO);
	toolInfo.uId = toolID;
	toolInfo.hwnd = window;
	return sendMessageC<bool>(TTM_GETTOOLINFO, 0, reinterpret_cast<LPARAM>(&toolInfo));
}

inline bool ToolTipCtrl::hitTest(TTHITTESTINFO& hitTest) const {return sendMessageC<bool>(TTM_HITTEST, 0, reinterpret_cast<LPARAM>(&hitTest));}

inline bool ToolTipCtrl::hitTest(HWND window, const POINT& pt, TOOLINFO& toolInfo) const {
	TTHITTESTINFO tthi;
	tthi.hwnd = window;
	tthi.pt = pt;
	tthi.ti = toolInfo;
	return hitTest(tthi);
}

inline void ToolTipCtrl::pop() {sendMessage(TTM_POP);}

#ifdef TTM_POPUP
inline void ToolTipCtrl::popup() {sendMessage(TTM_POPUP);}
#endif /* TTM_POPUP */

inline void ToolTipCtrl::relayEvent(MSG& message) {sendMessage(TTM_RELAYEVENT, 0, reinterpret_cast<LPARAM>(&message));}

inline void ToolTipCtrl::setDelayTime(UINT delay) {sendMessage(TTM_SETDELAYTIME, TTDT_AUTOPOP, delay);}

inline void ToolTipCtrl::setDelayTime(DWORD duration, int time) {sendMessage(TTM_SETDELAYTIME, duration, time);}

inline void ToolTipCtrl::setMargin(const RECT& rect) {sendMessage(TTM_SETMARGIN, 0, reinterpret_cast<LPARAM>(&rect));}

inline int ToolTipCtrl::setMaxTipWidth(int width) {return sendMessageR<int>(TTM_SETMAXTIPWIDTH, 0, width);}

inline void ToolTipCtrl::setTipBkColor(COLORREF color) {sendMessage(TTM_SETTIPBKCOLOR, color);}

inline void ToolTipCtrl::setTipTextColor(COLORREF color) {sendMessage(TTM_SETTIPTEXTCOLOR, color);}

inline bool ToolTipCtrl::setTitle(UINT icon, const TCHAR* title) {return sendMessageR<bool>(TTM_SETTITLE, icon, reinterpret_cast<LPARAM>(title));}

inline void ToolTipCtrl::setToolInfo(const TOOLINFO& toolInfo) {sendMessage(TTM_SETTOOLINFO, 0, reinterpret_cast<LPARAM>(&toolInfo));}

inline void ToolTipCtrl::setToolRect(HWND window, UINT toolID, const RECT& rect) {
	AutoZeroCB<TOOLINFO> ti;
	ti.uId = toolID;
	ti.hwnd = window;
	sendMessage(TTM_GETTOOLINFO, 0, reinterpret_cast<LPARAM>(&ti));
	ti.rect = rect;
	sendMessage(TTM_SETTOOLINFO, 0, reinterpret_cast<LPARAM>(&ti));
}

#if(_WIN32_WINNT >= 0x0501)
inline void ToolTipCtrl::setWindowTheme(const WCHAR* theme) {sendMessage(TTM_SETWINDOWTHEME, 0, reinterpret_cast<LPARAM>(theme));}
#endif /* _WIN32_WINNT >= 0x0501 */

inline void ToolTipCtrl::trackActivate(const TOOLINFO& toolInfo, bool activate) {
	sendMessage(TTM_TRACKACTIVATE, activate, reinterpret_cast<LPARAM>(&toolInfo));}

inline void ToolTipCtrl::trackPosition(int x, int y) {sendMessage(TTM_TRACKPOSITION, 0, MAKELPARAM(x, y));}

inline void ToolTipCtrl::update() {sendMessage(TTM_UPDATE);}

inline void ToolTipCtrl::updateTipText(const TCHAR* text, HWND window, UINT toolID /* = 0 */) {
	AutoZeroCB<TOOLINFO> ti;
//	ti.hinst = reinterpret_cast<HINSTANCE>(::GetWindowLong(hWnd, GWL_HINSTANCE));
	ti.uId = toolID;
	ti.hwnd = window;
	ti.lpszText = const_cast<TCHAR*>(text);
	sendMessage(TTM_UPDATETIPTEXT, 0, reinterpret_cast<LPARAM>(&ti));
}

inline void ToolTipCtrl::updateTipText(const TCHAR* text, HWND window, HWND control) {
	AutoZeroCB<TOOLINFO> ti;
	ti.uFlags = TTF_IDISHWND;
//	ti.hinst = reinterpret_cast<HINSTANCE>(::GetWindowLong(hWnd, GWL_HINSTANCE));
	ti.uId = reinterpret_cast<UINT_PTR>(control);
	ti.hwnd = window;
	ti.lpszText = const_cast<TCHAR*>(text);
	sendMessage(TTM_UPDATETIPTEXT, 0, reinterpret_cast<LPARAM>(&ti));
}


// TreeCtrl /////////////////////////////////////////////////////////////////

inline UINT TreeCtrl::getCount() const {return sendMessageC<UINT>(TVM_GETCOUNT);}

inline UINT TreeCtrl::getIndent() const {return sendMessageC<UINT>(TVM_GETINDENT);}

inline void TreeCtrl::setIndent(UINT indent) {sendMessage(TVM_SETINDENT, indent);}

inline HIMAGELIST TreeCtrl::getImageList(UINT image) const {
	return reinterpret_cast<HIMAGELIST>(sendMessageC<LRESULT>(TVM_GETIMAGELIST, image));}

inline HIMAGELIST TreeCtrl::setImageList(HIMAGELIST imageList, UINT image) {
	return reinterpret_cast<HIMAGELIST>(sendMessage(TVM_SETIMAGELIST, image, reinterpret_cast<LPARAM>(imageList)));}

inline HTREEITEM TreeCtrl::getNextItem(HTREEITEM item, UINT code) const {
	return reinterpret_cast<HTREEITEM>(sendMessageC<LRESULT>(TVM_GETNEXTITEM, code, reinterpret_cast<LPARAM>(item)));}

inline bool TreeCtrl::itemHasChildren(HTREEITEM item) const {
	TVITEM tvi;
	tvi.hItem = item;
	tvi.mask = TVIF_CHILDREN;
	sendMessageC<int>(TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvi));
	return tvi.cChildren != 0;
}

inline HTREEITEM TreeCtrl::getChildItem(HTREEITEM item) const {return getNextItem(item, TVGN_CHILD);}

inline HTREEITEM TreeCtrl::getNextSiblingItem(HTREEITEM item) const {return getNextItem(item, TVGN_NEXT);}

inline HTREEITEM TreeCtrl::getPrevSiblingItem(HTREEITEM item) const {return getNextItem(item, TVGN_PREVIOUS);}

inline HTREEITEM TreeCtrl::getParentItem(HTREEITEM item) const {return getNextItem(item, TVGN_PARENT);}

inline HTREEITEM TreeCtrl::getFirstVisibleItem() const {return getNextItem(0, TVGN_FIRSTVISIBLE);}

inline HTREEITEM TreeCtrl::getLastVisibleItem() const {return getNextItem(0, TVGN_LASTVISIBLE);}

inline HTREEITEM TreeCtrl::getNextVisibleItem(HTREEITEM item) const {return getNextItem(item, TVGN_NEXTVISIBLE);}

inline HTREEITEM TreeCtrl::getPrevVisibleItem(HTREEITEM item) const {return getNextItem(item, TVGN_PREVIOUSVISIBLE);}

inline HTREEITEM TreeCtrl::getSelectedItem() const {return getNextItem(0, TVGN_CARET);}

inline HTREEITEM TreeCtrl::getDropHilightItem() const {return getNextItem(0, TVGN_DROPHILITE);}

inline HTREEITEM TreeCtrl::getRootItem() const {return getNextItem(0, TVGN_ROOT);}

inline bool TreeCtrl::getItem(TVITEM& item) const {return sendMessageC<bool>(TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&item));}

inline bool TreeCtrl::getItem(TVITEMEX& item) const {return sendMessageC<bool>(TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&item));}

inline bool TreeCtrl::setItem(const TVITEM& item) {return sendMessageR<bool>(TVM_SETITEM, 0, reinterpret_cast<LPARAM>(&item));}

inline bool TreeCtrl::setItem(const TVITEMEX& item) {return sendMessageR<bool>(TVM_SETITEM, 0, reinterpret_cast<LPARAM>(&item));}

inline bool TreeCtrl::setItem(HTREEITEM item, UINT mask, LPCTSTR text, int image, int selectedImage, UINT state, UINT stateMask, LPARAM lParam) {
	TVITEM tvi;
	tvi.hItem = item;
	tvi.mask = mask;
	tvi.pszText = const_cast<LPTSTR>(text);
	tvi.iImage = image;
	tvi.iSelectedImage = selectedImage;
	tvi.state = state;
	tvi.stateMask = stateMask;
	tvi.lParam = lParam;
	return sendMessageR<bool>(TVM_SETITEM, 0, reinterpret_cast<LPARAM>(&tvi));
}

inline UINT TreeCtrl::getItemState(HTREEITEM item, UINT stateMask) const {
	TVITEM tvi;
	tvi.hItem = item;
	tvi.mask = TVIF_STATE;
	tvi.stateMask = stateMask;
	tvi.state = 0;
	sendMessageC<int>(TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvi));
	return tvi.state;
}

inline bool TreeCtrl::setItemState(HTREEITEM item, UINT state, UINT stateMask) {
	TVITEM tvi;
	tvi.hItem = item;
	tvi.mask = TVIF_STATE;
	tvi.stateMask = stateMask;
	tvi.state = state;
	return sendMessageR<bool>(TVM_SETITEM, 0, reinterpret_cast<LPARAM>(&tvi));
}

inline bool TreeCtrl::getItemImage(HTREEITEM item, int& image, int& selectedImage) const {
	TVITEM tvi;
	tvi.hItem = item;
	tvi.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	if(sendMessageC<bool>(TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvi))) {
		image = tvi.iImage;
		selectedImage = tvi.iSelectedImage;
		return true;
	} else
		return false;
}

inline bool TreeCtrl::setItemImage(HTREEITEM item, int image, int selectedImage) {
	TVITEM tvi;
	tvi.hItem = item;
	tvi.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	tvi.iImage = image;
	tvi.iSelectedImage = selectedImage;
	return sendMessageR<bool>(TVM_SETITEM, 0, reinterpret_cast<LPARAM>(&tvi));
}

inline bool TreeCtrl::getItemText(HTREEITEM item, TCHAR* text, int maxLength) const {
	TVITEM tvi;
	tvi.hItem = item;
	tvi.mask = TVIF_TEXT;
	tvi.pszText = text;
	tvi.cchTextMax = maxLength;
	return sendMessageC<bool>(TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvi));
}

inline bool TreeCtrl::setItemText(HTREEITEM item, const TCHAR* text) {
	TVITEM tvi;
	tvi.hItem = item;
	tvi.mask = TVIF_TEXT;
	tvi.pszText = const_cast<LPTSTR>(text);
	return sendMessageR<bool>(TVM_SETITEM, 0, reinterpret_cast<LPARAM>(&tvi));
}

inline LPARAM TreeCtrl::getItemData(HTREEITEM item) const {
	TVITEM tvi;
	tvi.hItem = item;
	tvi.mask = TVIF_PARAM;
	sendMessageC<int>(TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvi));
	return tvi.lParam;
}

inline bool TreeCtrl::setItemData(HTREEITEM item, DWORD data) {
	TVITEM tvi;
	tvi.hItem = item;
	tvi.mask = TVIF_PARAM;
	tvi.lParam = data;
	return sendMessageR<bool>(TVM_SETITEM, 0, reinterpret_cast<LPARAM>(&tvi));
}

inline bool TreeCtrl::getItemRect(HTREEITEM item, RECT& rect, bool textOnly) {
	rect = *reinterpret_cast<RECT*>(item);
	return sendMessageR<bool>(TVM_GETITEMRECT, textOnly, reinterpret_cast<LPARAM>(&rect));
}

inline HWND TreeCtrl::getEditControl() const {return reinterpret_cast<HWND>(sendMessageC<LRESULT>(TVM_GETEDITCONTROL));}

inline UINT TreeCtrl::getVisibleCount() const {return sendMessageC<UINT>(TVM_GETVISIBLECOUNT);}

inline HWND TreeCtrl::getToolTips() const {return reinterpret_cast<HWND>(sendMessageC<LRESULT>(TVM_GETTOOLTIPS));}

inline HWND TreeCtrl::setToolTips(HWND toolTips) {
	return reinterpret_cast<HWND>(sendMessage(TVM_SETTOOLTIPS, 0, reinterpret_cast<LPARAM>(toolTips)));}

inline COLORREF TreeCtrl::getBkColor() const {return sendMessageC<COLORREF>(TVM_GETBKCOLOR);}

inline COLORREF TreeCtrl::setBkColor(COLORREF color) {return sendMessageR<COLORREF>(TVM_SETBKCOLOR, 0, static_cast<LPARAM>(color));}

inline SHORT TreeCtrl::getItemHeight() const {return sendMessageC<SHORT>(TVM_GETITEMHEIGHT);}

inline SHORT TreeCtrl::setItemHeight(SHORT height) {return sendMessageR<SHORT>(TVM_SETITEMHEIGHT, height);}

inline COLORREF TreeCtrl::getTextColor() const {return sendMessageC<COLORREF>(TVM_GETTEXTCOLOR);}

inline COLORREF TreeCtrl::setTextColor(COLORREF color) {return sendMessageR<COLORREF>(TVM_SETTEXTCOLOR, color);}

inline bool TreeCtrl::setInsertMark(HTREEITEM item, bool after /* = true */) {
	return sendMessageR<bool>(TVM_SETINSERTMARK, after, reinterpret_cast<LPARAM>(item));}

inline bool TreeCtrl::getCheck(HTREEITEM item) const {
	TVITEM tvi;
	tvi.mask = TVIF_HANDLE | TVIF_STATE;
	tvi.hItem = item;
	tvi.stateMask = TVIS_STATEIMAGEMASK;
	sendMessageC<int>(TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvi));
	return toBoolean(static_cast<UINT>(tvi.state >> 12) -1);
}

inline bool TreeCtrl::setCheck(HTREEITEM item, bool check /* = true */) {
	TVITEM tvi;
	tvi.mask = TVIF_HANDLE | TVIF_STATE;
	tvi.hItem = item;
	tvi.stateMask = TVIS_STATEIMAGEMASK;
	tvi.state = INDEXTOSTATEIMAGEMASK((check ? 2 : 1));
	return sendMessageR<bool>(TVM_SETITEM, 0, reinterpret_cast<LPARAM>(&tvi));
}

inline COLORREF TreeCtrl::getInsertMarkColor() const {return sendMessageC<COLORREF>(TVM_GETINSERTMARKCOLOR);}

inline COLORREF TreeCtrl::setInsertMarkColor(COLORREF color) {return sendMessageR<COLORREF>(TVM_SETINSERTMARKCOLOR, color);}

inline HTREEITEM TreeCtrl::insertItem(const TVINSERTSTRUCT& insertStruct) {
	return reinterpret_cast<HTREEITEM>(sendMessage(TVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&insertStruct)));}

inline HTREEITEM TreeCtrl::insertItem(UINT mask, LPCTSTR text, int image, int selectedImage,
	UINT state, UINT stateMask, LPARAM lParam, HTREEITEM parent, HTREEITEM insertAfter) {
	TVINSERTSTRUCT tvis;
	tvis.hParent = parent;
	tvis.hInsertAfter = insertAfter;
	tvis.item.mask = mask;
	tvis.item.pszText = const_cast<TCHAR*>(text);
	tvis.item.iImage = image;
	tvis.item.iSelectedImage = selectedImage;
	tvis.item.state = state;
	tvis.item.stateMask = stateMask;
	tvis.item.lParam = lParam;
	return insertItem(tvis);
}

inline HTREEITEM TreeCtrl::insertItem(const TCHAR* text,
		HTREEITEM parent /* = TVI_ROOT */, HTREEITEM insertAfter /* = TVI_LAST */) {
	TVINSERTSTRUCT tvis;
	tvis.hParent = parent;
	tvis.hInsertAfter = insertAfter;
	tvis.item.mask = TVIF_TEXT;
	tvis.item.pszText = const_cast<TCHAR*>(text);
	return insertItem(tvis);
}

inline HTREEITEM TreeCtrl::insertItem(const TCHAR* text, int image, int selectedImage,
		HTREEITEM parent /* = TVI_ROOT */, HTREEITEM insertAfter /* = TVI_LAST */) {
	TVINSERTSTRUCT tvis;
	tvis.hParent = parent;
	tvis.hInsertAfter = insertAfter;
	tvis.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	tvis.item.pszText = const_cast<TCHAR*>(text);
	tvis.item.iImage = image;
	tvis.item.iSelectedImage = selectedImage;
	return insertItem(tvis);
}

inline bool TreeCtrl::deleteItem(HTREEITEM item) {return sendMessageR<bool>(TVM_DELETEITEM, 0, reinterpret_cast<LPARAM>(item));}

inline bool TreeCtrl::deleteAllItems() {return sendMessageR<bool>(TVM_DELETEITEM, 0, reinterpret_cast<LPARAM>(TVI_ROOT));}

inline bool TreeCtrl::expandItem(HTREEITEM item, UINT code) {return sendMessageR<bool>(TVM_EXPAND, code, reinterpret_cast<LPARAM>(item));}

inline bool TreeCtrl::selectItem(HTREEITEM item, UINT code /* = TVGN_CARET */) {
	return sendMessageR<bool>(TVM_SELECTITEM, code, reinterpret_cast<LPARAM>(item));}

inline bool TreeCtrl::selectDropTarget(HTREEITEM item) {
	return sendMessageR<bool>(TVM_SELECTITEM, TVGN_DROPHILITE, reinterpret_cast<LPARAM>(item));}

inline bool TreeCtrl::selectSetFirstVisible(HTREEITEM item) {
	return sendMessageR<bool>(TVM_SELECTITEM, TVGN_FIRSTVISIBLE, reinterpret_cast<LPARAM>(item));}

inline HWND TreeCtrl::editLabel(HTREEITEM item) {return reinterpret_cast<HWND>(sendMessage(TVM_EDITLABEL, 0, reinterpret_cast<LPARAM>(item)));}

inline HTREEITEM TreeCtrl::hitTest(TVHITTESTINFO& tvhi) {
	return reinterpret_cast<HTREEITEM>(sendMessage(TVM_HITTEST, 0, reinterpret_cast<LPARAM>(&tvhi)));}

inline HTREEITEM TreeCtrl::hitTest(const POINT& pt, UINT flags) {
	TVHITTESTINFO tvhti;
	tvhti.flags = flags;
	tvhti.pt = pt;
	return hitTest(tvhti);
}

inline std::auto_ptr<ImageList> TreeCtrl::createDragImage(HTREEITEM item) {
	return std::auto_ptr<ImageList>(new ImageList(reinterpret_cast<HIMAGELIST>(sendMessage(TVM_CREATEDRAGIMAGE, 0, reinterpret_cast<LPARAM>(item)))));}

inline bool TreeCtrl::sortChildren(HTREEITEM item, bool recurse /* = false */) {
	return sendMessageR<bool>(TVM_SORTCHILDREN, recurse, reinterpret_cast<LPARAM>(item));}

inline bool TreeCtrl::ensureVisible(HTREEITEM item) {return sendMessageR<bool>(TVM_ENSUREVISIBLE, 0, reinterpret_cast<LPARAM>(item));}

inline bool TreeCtrl::sortChildrenCB(const TVSORTCB& sort, bool recurse /* = false */) {
	return sendMessageR<bool>(TVM_SORTCHILDRENCB, recurse, reinterpret_cast<LPARAM>(&sort));}

inline bool TreeCtrl::endEditLabelNow() {return sendMessageR<bool>(TVM_ENDEDITLABELNOW);}

inline UINT TreeCtrl::getISearchString(TCHAR* text) {return sendMessageC<UINT>(TVM_GETISEARCHSTRING, 0, reinterpret_cast<LPARAM>(text));}

inline UINT TreeCtrl::getScrollTime() const {return sendMessageC<UINT>(TVM_GETSCROLLTIME);}

inline UINT TreeCtrl::setScrollTime(UINT scrollTime) {return sendMessageR<UINT>(TVM_SETSCROLLTIME, scrollTime);}

inline bool TreeCtrl::getUnicodeFormat() const {return sendMessageC<bool>(TVM_GETUNICODEFORMAT);}

inline bool TreeCtrl::setUnicodeFormat(bool unicode /* = true */) {return sendMessageR<bool>(TVM_SETUNICODEFORMAT, unicode);}

inline COLORREF TreeCtrl::getLineColor() const {
#ifndef TVM_GETLINECOLOR
	const UINT TVM_GETLINECOLOR = TV_FIRST + 41;
#endif /* !TVM_GETLINECOLOR */
	return sendMessageC<COLORREF>(TVM_GETLINECOLOR);
}

inline COLORREF TreeCtrl::setLineColor(COLORREF color /* = CLR_DEFAULT */) {
#ifndef TVM_GETLINECOLOR
	const UINT TVM_SETLINECOLOR = TV_FIRST + 40;
#endif /* !TVM_GETLINECOLOR */
	return sendMessageR<COLORREF>(TVM_SETLINECOLOR, 0, color);
}

inline HTREEITEM TreeCtrl::mapAccIdToHTREEITEM(UINT id) const {
#ifndef TVM_MAPACCIDTOHTREEITEM
	const UINT TVM_MAPACCIDTOHTREEITEM = TV_FIRST + 42;
#endif /* !TVM_MAPACCIDTOHTREEITEM */
	return reinterpret_cast<HTREEITEM>(sendMessageC<LRESULT>(TVM_MAPACCIDTOHTREEITEM, id));
}

inline UINT TreeCtrl::mapHTREEITEMToAccId(HTREEITEM item) const {
#ifndef TVM_MAPHTREEITEMTOACCID
	const UINT TVM_MAPHTREEITEMTOACCID = TV_FIRST + 43;
#endif /* !TVM_MAPHTREEITEMTOACCID */
	return sendMessageC<UINT>(TVM_MAPHTREEITEMTOACCID, reinterpret_cast<WPARAM>(item));
}


// UpDownCtrl (SpinCtrl) ////////////////////////////////////////////////////

inline UINT UpDownCtrl::getAccel(int count, UDACCEL accel[]) const {
	return sendMessageC<UINT>(UDM_GETACCEL, count, reinterpret_cast<LPARAM>(accel));}

inline UINT UpDownCtrl::getBase() const {return sendMessageC<UINT>(UDM_GETBASE);}

inline Window UpDownCtrl::getBuddy() const {return Window(reinterpret_cast<HWND>(sendMessageC<LRESULT>(UDM_GETBUDDY)));}

inline int UpDownCtrl::getPos(bool* error /* = 0 */) const {return sendMessageC<int>(UDM_GETPOS32, 0, reinterpret_cast<LPARAM>(error));}

inline void UpDownCtrl::getRange(int& lower, int& upper) const {
	sendMessageC<LRESULT>(UDM_GETRANGE32, reinterpret_cast<WPARAM>(&lower), reinterpret_cast<LPARAM>(&upper));}

inline bool UpDownCtrl::getUnicodeFormat() const {return sendMessageC<bool>(UDM_GETUNICODEFORMAT);}

inline bool UpDownCtrl::setAccel(int count, const UDACCEL accel[]) {
	return sendMessageR<bool>(UDM_SETACCEL, count, reinterpret_cast<LPARAM>(accel));}

inline int UpDownCtrl::setBase(int base) {return sendMessageR<int>(UDM_SETBASE, base);}

inline Window UpDownCtrl::setBuddy(HWND buddy) {
	return Window(reinterpret_cast<HWND>(sendMessageR<LRESULT>(UDM_GETBUDDY, reinterpret_cast<WPARAM>(buddy))));}

inline int UpDownCtrl::setPos(int pos) {return sendMessageR<int>(UDM_SETPOS32, 0, static_cast<LPARAM>(pos));}

inline void UpDownCtrl::setRange(int lower, int upper) {sendMessage(UDM_SETRANGE32, lower, upper);}

inline bool UpDownCtrl::setUnicodeFormat(bool unicode /* = true */) {return sendMessageR<bool>(UDM_SETUNICODEFORMAT, unicode);}


}}} // namespace manah::windows::ui
