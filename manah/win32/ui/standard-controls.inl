// StandardControls.inl
// (c) 2002-2008 exeal

namespace manah {
namespace win32 {
namespace ui {


// Button ///////////////////////////////////////////////////////////////////

inline void Button::click() {sendMessage(BM_CLICK);}

inline HBITMAP Button::getBitmap() const {return reinterpret_cast<HBITMAP>(sendMessageC<LRESULT>(BM_GETIMAGE, IMAGE_BITMAP));}

inline UINT Button::getButtonStyle() const {return getWindowLong(GWL_STYLE) & 0xFF;}

inline int Button::getCheck() const {return sendMessageC<int>(BM_GETCHECK);}

inline HCURSOR Button::getCursor() const {return reinterpret_cast<HCURSOR>(sendMessageC<LRESULT>(BM_GETIMAGE, IMAGE_CURSOR));}

inline HICON Button::getIcon() const {return reinterpret_cast<HICON>(sendMessageC<LRESULT>(BM_GETIMAGE, IMAGE_ICON));}

inline bool Button::getIdealSize(SIZE& size) const {
#ifndef BCM_GETIDEALSIZE
	const UINT BCM_GETIDEALSIZE = 0x1601;
#endif// !BCM_GETIDEALSIZE
	return sendMessageC<bool>(BCM_GETIDEALSIZE, 0, reinterpret_cast<LPARAM>(&size));
}

inline UINT Button::getState() const {return sendMessageC<UINT>(BM_GETSTATE);}

inline bool Button::getTextMargin(RECT& margin) const {
#ifndef BCM_GETTEXTMARGIN
	const UINT BCM_GETTEXTMARGIN = 0x1605;
#endif // !BCM_GETTEXTMARGIN
	return sendMessageC<bool>(BCM_GETTEXTMARGIN, 0, reinterpret_cast<LPARAM>(&margin));}

inline HBITMAP Button::setBitmap(HBITMAP bitmap) {
	return reinterpret_cast<HBITMAP>(sendMessage(BM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(bitmap)));}

inline void Button::setButtonStyle(UINT style, bool redraw /* = true */) {sendMessage(BM_SETSTYLE, style, redraw);}

inline void Button::setCheck(int check) {sendMessage(BM_SETCHECK, check);}

inline HCURSOR Button::setCursor(HCURSOR cursor) {
	return reinterpret_cast<HCURSOR>(sendMessage(BM_SETIMAGE, IMAGE_CURSOR, reinterpret_cast<LPARAM>(cursor)));}

inline HICON Button::setIcon(HICON icon) {
	return reinterpret_cast<HICON>(sendMessage(BM_SETIMAGE, IMAGE_ICON, reinterpret_cast<LPARAM>(icon)));}

inline void Button::setState(bool highlight) {sendMessage(BM_SETSTATE, highlight);}

inline bool Button::setTextMargin(const RECT& margin) {
#ifndef BCM_SETTEXTMARGIN
	const UINT BCM_SETTEXTMARGIN = 0x1604;
#endif // !BCM_SETTEXTMARGIN
	return sendMessageR<bool>(BCM_SETTEXTMARGIN, 0, reinterpret_cast<LPARAM>(&margin));
}

#ifdef BCM_FIRST
inline bool Button::getImageList(BUTTON_IMAGELIST& bi) const {return sendMessageC<bool>(BCM_GETIMAGELIST, 0, reinterpret_cast<LPARAM>(&bi));}

inline bool Button::setImageList(const BUTTON_IMAGELIST& bi) {return sendMessageR<bool>(BCM_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(&bi));}
#endif // BCM_FIRST


// ComboBox /////////////////////////////////////////////////////////////////

inline int ComboBox::addString(const WCHAR* text) {return sendMessageR<int>(CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text));}

inline void ComboBox::clear() {sendMessage(WM_CLEAR);}

inline void ComboBox::copy() {sendMessage(WM_COPY);}

inline void ComboBox::cut() {sendMessage(WM_CUT);}

inline int ComboBox::deleteString(UINT index) {return sendMessageR<int>(CB_DELETESTRING, index);}

inline int ComboBox::dir(UINT attributes, const WCHAR* fileSpec) {return sendMessageR<int>(CB_DIR, attributes, reinterpret_cast<LPARAM>(fileSpec));}

inline int ComboBox::findString(int startAfter, const WCHAR* text) const {return sendMessageC<int>(CB_FINDSTRING, startAfter, reinterpret_cast<LPARAM>(text));}

inline int ComboBox::findStringExact(int start, const WCHAR* text) const {return sendMessageC<int>(CB_FINDSTRINGEXACT, start, reinterpret_cast<LPARAM>(text));}

#ifdef CB_GETCOMBOBOXINFO
inline bool ComboBox::getComboBoxInformation(COMBOBOXINFO& cbi) const {
	return sendMessageC<bool>(CB_GETCOMBOBOXINFO, 0, reinterpret_cast<LPARAM>(&cbi));}
#endif // !CB_GETCOMBOBOXINFO

inline int ComboBox::getCount() const {return sendMessageC<int>(CB_GETCOUNT);}

inline int ComboBox::getCurSel() const {return sendMessageC<int>(CB_GETCURSEL);}

inline void ComboBox::getDroppedControlRect(RECT& rect) const {sendMessageC<int>(CB_GETDROPPEDCONTROLRECT, 0, reinterpret_cast<LPARAM>(&rect));}

inline bool ComboBox::getDroppedState() const {return sendMessageC<bool>(CB_GETDROPPEDSTATE);}

inline int ComboBox::getDroppedWidth() const {return sendMessageC<int>(CB_GETDROPPEDWIDTH);}

inline DWORD ComboBox::getEditSel() const {
	WORD start, end;
	sendMessageC<int>(CB_GETEDITSEL, reinterpret_cast<WPARAM>(&start), reinterpret_cast<LPARAM>(&end));
	return (start & 0xFFFF) | ((end & 0xFFFF) << 16);
}

inline bool ComboBox::getExtendedUI() const {return sendMessageC<bool>(CB_GETEXTENDEDUI);}

inline UINT ComboBox::getHorizontalExtent() const {return sendMessageC<UINT>(CB_GETHORIZONTALEXTENT);}

inline DWORD ComboBox::getItemData(int index) const {return sendMessageC<DWORD>(CB_GETITEMDATA, index);}

inline void* ComboBox::getItemDataPtr(int index) const {return reinterpret_cast<void*>(sendMessageC<LRESULT>(CB_GETITEMDATA, index));}

inline int ComboBox::getItemHeight(int index) const {return sendMessageC<int>(CB_GETITEMHEIGHT, index);}

inline int ComboBox::getLBText(int index, WCHAR* text) const {return sendMessageC<int>(CB_GETLBTEXT, index, reinterpret_cast<LPARAM>(text));}

inline int ComboBox::getLBTextLen(int index) const {return sendMessageC<int>(CB_GETLBTEXTLEN, index);}

inline LCID ComboBox::getLocale() const {return sendMessageC<LCID>(CB_GETLOCALE);}

inline int ComboBox::getMinVisible() const {
#ifndef CB_GETMINVISIBLE
	const UINT CB_GETMINVISIBLE = 0x1702;
#endif // !CB_GETMINVISIBLE
	return sendMessageC<int>(CB_GETMINVISIBLE);
}

inline int ComboBox::getTopIndex() const {return sendMessageC<int>(CB_GETTOPINDEX);}

inline int ComboBox::initStorage(int itemCount, UINT bytes) {return sendMessageC<int>(CB_INITSTORAGE, itemCount, bytes);}

inline int ComboBox::insertString(int index, const WCHAR* text) {return sendMessageR<int>(CB_INSERTSTRING, index, reinterpret_cast<LPARAM>(text));}

inline bool ComboBox::limitText(int maxLength) {return sendMessageR<bool>(CB_LIMITTEXT, maxLength);}

inline void ComboBox::paste() {sendMessage(WM_PASTE);}

inline void ComboBox::resetContent() {sendMessage(CB_RESETCONTENT);}

inline int ComboBox::selectString(int startAfter, const WCHAR* text) {
	return sendMessageR<int>(CB_SELECTSTRING, startAfter, reinterpret_cast<LPARAM>(text));}

inline int ComboBox::setCurSel(int select) {return sendMessageR<int>(CB_SETCURSEL, select);}

inline int ComboBox::setDroppedWidth(UINT width) {return sendMessageR<int>(CB_SETDROPPEDWIDTH, width);}

inline bool ComboBox::setEditSel(int startChar, int endChar) {return sendMessageR<bool>(CB_SETEDITSEL, startChar, endChar);}

inline int ComboBox::setExtendedUI(bool extended /* = true */) {return sendMessageR<int>(CB_SETEXTENDEDUI, extended);}

inline void ComboBox::setHorizontalExtent(UINT extent) {sendMessage(CB_SETHORIZONTALEXTENT, extent);}

inline int ComboBox::setItemData(int index, DWORD itemData) {return sendMessageR<int>(CB_SETITEMDATA, index, itemData);}

inline int ComboBox::setItemDataPtr(int index, const void* itemData) {return sendMessageR<int>(CB_SETITEMDATA, index, reinterpret_cast<LPARAM>(itemData));}

inline int ComboBox::setItemHeight(int index, UINT itemHeight) {return sendMessageR<int>(CB_SETITEMHEIGHT, index, itemHeight);}

inline LCID ComboBox::setLocale(LCID lcid) {return sendMessageR<LCID>(CB_SETLOCALE, lcid);}

inline bool ComboBox::setMinVisible(int minVisible) {
#ifndef CB_SETMINVISIBLE
	const UINT CB_SETMINVISIBLE = 0x1701;
#endif // !CB_SETMINVISIBLE
	return sendMessageR<bool>(CB_SETMINVISIBLE, minVisible);
}

inline int ComboBox::setTopIndex(int index) {return sendMessageR<int>(CB_SETTOPINDEX, index);}

inline void ComboBox::showDropDown(bool show /* = true */) {sendMessage(CB_SHOWDROPDOWN, show);}


// DragListBox //////////////////////////////////////////////////////////////

inline void DragListBox::drawInsert(int index) {::DrawInsert(getParent().use(), use(), index);}

inline UINT DragListBox::getDragListMessage() {
	static UINT message;
	if(message == 0)
		message = ::RegisterWindowMessage(DRAGLISTMSGSTRING);
	return message;
}

inline int DragListBox::lbItemFromPtr(const POINT& pt, bool autoScroll /* = true */) {return LBItemFromPt(use(), pt, autoScroll);}

inline bool DragListBox::makeDragList() {
	if(!toBoolean(getStyle() & (LBS_MULTIPLESEL | LBS_EXTENDEDSEL)))
		return false;
	return toBoolean(::MakeDragList(use()));
}


// Edit /////////////////////////////////////////////////////////////////////

inline bool Edit::canUndo() const {return sendMessageC<bool>(EM_CANUNDO);}

inline void Edit::getEditRect(RECT& rect) const {sendMessageC<int>(EM_GETRECT, 0, reinterpret_cast<LPARAM>(&rect));}

inline int Edit::getLineCount() const {return sendMessageC<int>(EM_GETLINECOUNT);}

inline bool Edit::getModify() const {return sendMessageC<bool>(EM_GETMODIFY);}

inline void Edit::setModify(bool modified /* = true */) {sendMessage(EM_SETMODIFY, modified);}

inline int Edit::getThumb() const {return sendMessageC<int>(EM_GETTHUMB);}

inline DWORD Edit::getSel() const {return sendMessageC<DWORD>(EM_GETSEL);}

inline void Edit::getSel(int& startChar, int& endChar) const {sendMessageC<int>(EM_GETSEL, startChar, endChar);}

inline HLOCAL Edit::getBufferHandle() const {return reinterpret_cast<HLOCAL>(sendMessageC<LRESULT>(EM_GETHANDLE));}

inline void Edit::setHandle(HLOCAL buffer) {sendMessage(EM_SETHANDLE, reinterpret_cast<WPARAM>(buffer));}

inline void Edit::setMargins(UINT left, UINT right) {sendMessage(EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(left, right));}

inline DWORD Edit::getMargins() const {return sendMessageC<DWORD>(EM_GETMARGINS);}

inline void Edit::setLimitText(UINT maxLength) {sendMessage(EM_SETLIMITTEXT, maxLength);}

inline UINT Edit::getLimitText() const {return sendMessageC<UINT>(EM_GETLIMITTEXT);}

inline POINT Edit::posFromChar(UINT charPos) const {
	const DWORD dw = sendMessageC<DWORD>(EM_POSFROMCHAR, charPos);
	POINT point;
	point.x = LOWORD(dw);
	point.y = HIWORD(dw);
	return point;
}

inline int Edit::charFromPos(const POINT& pt) const {return sendMessageC<int>(EM_CHARFROMPOS, 0, MAKELPARAM(pt.x, pt.y));}

inline int Edit::getLine(int index, WCHAR* buffer) const {return sendMessageC<int>(EM_GETLINE, index, reinterpret_cast<LPARAM>(buffer));}

inline int Edit::getLine(int index, WCHAR* buffer, int maxLength) const {
	*reinterpret_cast<int*>(buffer) = maxLength;
	return sendMessageC<int>(EM_GETLINE, index, reinterpret_cast<LPARAM>(buffer));
}

inline WCHAR Edit::getPasswordChar() const {return sendMessageC<WCHAR>(EM_GETPASSWORDCHAR);}

inline int Edit::getFirstVisibleLine() const {return sendMessageC<int>(EM_GETFIRSTVISIBLELINE);}

inline void Edit::emptyUndoBuffer() {sendMessage(EM_EMPTYUNDOBUFFER);}

inline bool Edit::fmtLines(bool addEol) {return sendMessageR<bool>(EM_FMTLINES, addEol);}

inline void Edit::limitText(int nChars /* = 0 */) {sendMessage(EM_LIMITTEXT, nChars);}

inline int Edit::lineFromChar(int index /* = -1 */) const {return sendMessageC<int>(EM_LINEFROMCHAR, index);}

inline int Edit::lineIndex(int index /* = -1 */) const {return sendMessageC<int>(EM_LINEINDEX, index);}

inline int Edit::lineLength(int line /* = -1 */) const {return sendMessageC<int>(EM_LINELENGTH, sendMessageC<int>(EM_LINEINDEX, line));}

inline void Edit::lineScroll(int lines, int chars /* = 0 */) {sendMessage(EM_LINESCROLL, chars, lines);}

inline void Edit::replaceSel(const WCHAR* newText, bool canUndo /* = false */) {sendMessage(EM_REPLACESEL, canUndo, reinterpret_cast<LPARAM>(newText));}

inline void Edit::setPasswordChar(WCHAR ch) {sendMessage(EM_SETPASSWORDCHAR, ch);}

inline void Edit::setRect(const RECT& rect) {sendMessage(EM_SETRECT, 0, reinterpret_cast<LPARAM>(&rect));}

inline void Edit::setRectNP(const RECT& rect) {sendMessage(EM_SETRECTNP, 0, reinterpret_cast<LPARAM>(&rect));}

inline void Edit::setSel(DWORD selection, bool noScroll /* = false */) {setSel(LOWORD(selection), HIWORD(selection), noScroll);}

inline void Edit::setSel(int startChar, int endChar, bool noScroll /* = false */) {
	sendMessage(EM_SETSEL, startChar, endChar); if(!noScroll) scrollCaret();}

inline void Edit::setTabStops() {sendMessage(EM_SETTABSTOPS);}

inline bool Edit::setTabStops(uint cxEachStop) {return sendMessageR<bool>(EM_SETTABSTOPS, 1, reinterpret_cast<LPARAM>(&cxEachStop));}

inline bool Edit::setTabStops(int count, uint tabStops[]) {return sendMessageR<bool>(EM_SETTABSTOPS, count, reinterpret_cast<LPARAM>(tabStops));}

inline void Edit::setReadOnly(bool readOnly /* = true */) {sendMessage(EM_SETREADONLY, readOnly);}

inline bool Edit::scrollCaret() {return toBoolean(sendMessage(EM_SCROLLCARET));}

inline EDITWORDBREAKPROCW Edit::getWordBreakProc() const {return reinterpret_cast<EDITWORDBREAKPROCW>(sendMessageC<LRESULT>(EM_GETWORDBREAKPROC));}

inline void Edit::setWordBreakProc(EDITWORDBREAKPROCW proc) {sendMessage(EM_SETWORDBREAKPROC, 0, reinterpret_cast<LPARAM>(proc));}

#ifdef EM_GETIMESTATUS
inline DWORD Edit::getImeStatus(DWORD type) const {return sendMessageC<DWORD>(EM_GETIMESTATUS, type);}

inline DWORD Edit::setImeStatus(DWORD type, DWORD data) {return sendMessageR<DWORD>(EM_SETIMESTATUS, type, data);}
#endif // EM_GETIMESTATUS

#ifdef EM_GETCUEBANNER
inline bool Edit::getCueBanner(WCHAR* text, int maxLength) const {
	return toBoolean(Edit_GetCueBannerText(use(), text, maxLength));	// parameters confused in document?
}

inline bool Edit::setCueBanner(const WCHAR* text) {return sendMessageR<bool>(EM_SETCUEBANNER, 0, reinterpret_cast<LPARAM>(text));}

inline bool Edit::showBalloonTip(const EDITBALLOONTIP& ebt) {return sendMessageR<bool>(EM_SHOWBALLOONTIP, 0, reinterpret_cast<LPARAM>(&ebt));}

inline bool Edit::hideBalloonTip() {return sendMessageR<bool>(EM_HIDEBALLOONTIP);}
#endif // EM_GETCUEBANNER


// ListBox //////////////////////////////////////////////////////////////////

inline int ListBox::initStorage(int itemCount, UINT bytes) {return sendMessageR<int>(LB_INITSTORAGE, itemCount, static_cast<LPARAM>(bytes));}

inline int ListBox::getCount() const {return sendMessageC<int>(LB_GETCOUNT);}

inline int ListBox::getHorizontalExtent() const {return sendMessageC<int>(LB_GETHORIZONTALEXTENT);}

inline void ListBox::setHorizontalExtent(int extent) {sendMessage(LB_SETHORIZONTALEXTENT, extent);}

inline int ListBox::getTopIndex() const {return sendMessageC<int>(LB_GETTOPINDEX);}

inline int ListBox::setTopIndex(int index) {return sendMessageR<int>(LB_SETTOPINDEX, index);}

inline DWORD ListBox::getItemData(int index) const {return sendMessageC<DWORD>(LB_GETITEMDATA, index);}

inline void* ListBox::getItemDataPtr(int index) const {return reinterpret_cast<void*>(sendMessageC<LRESULT>(LB_GETITEMDATA, index));}

inline int ListBox::setItemData(int index, DWORD itemData) {return sendMessageR<int>(LB_SETITEMDATA, index, static_cast<LPARAM>(itemData));}

inline int ListBox::setItemDataPtr(int index, void* data) {return sendMessageR<int>(LB_SETITEMDATA, index, reinterpret_cast<LPARAM>(data));}

inline int ListBox::getItemRect(int index, RECT& rect) const {return sendMessageC<int>(LB_GETITEMRECT, index, reinterpret_cast<LPARAM>(&rect));}

inline UINT ListBox::itemFromPoint(const POINT& pt, bool& outSide) const {
	const UINT result = sendMessageC<UINT>(LB_ITEMFROMPOINT, 0, MAKELPARAM(pt.x, pt.y));
	outSide = toBoolean(HIWORD(result));
	return result;
}

inline int ListBox::setItemHeight(int index, UINT itemHeight) {return sendMessageR<int>(LB_SETITEMHEIGHT, index, static_cast<LPARAM>(itemHeight));}

inline int ListBox::getItemHeight(int index) const {return sendMessageC<int>(LB_GETITEMHEIGHT, index);}

inline int ListBox::getSel(int index) const {return sendMessageC<int>(LB_GETSEL, index);}

inline int ListBox::getText(int index, WCHAR* buffer) const {return sendMessageC<int>(LB_GETTEXT, index, reinterpret_cast<LPARAM>(buffer));}

inline int ListBox::getTextLen(int index) const {return sendMessageC<int>(LB_GETTEXTLEN, index);}

inline void ListBox::setColumnWidth(int width) {sendMessage(LB_SETCOLUMNWIDTH, width);}

inline void ListBox::setTabStops() {sendMessage(LB_SETTABSTOPS);}

inline bool ListBox::setTabStops(int cxEachTabStop) {return sendMessageR<bool>(LB_GETSEL, 1, reinterpret_cast<LPARAM>(&cxEachTabStop));}

inline bool ListBox::setTabStops(int count, INT* tabStops) {return toBoolean(sendMessage(LB_GETSEL, count, reinterpret_cast<LPARAM>(tabStops)));}

inline LCID ListBox::getLocale() const {return sendMessageC<LCID>(LB_GETLOCALE);}

inline LCID ListBox::setLocale(LCID newLocale) {return sendMessageR<LCID>(LB_SETLOCALE, newLocale);}

inline int ListBox::getCurSel() const {return sendMessageC<int>(LB_GETCURSEL);}

inline int ListBox::setCurSel(int select) {return sendMessageR<int>(LB_SETCURSEL, select);}

inline int ListBox::setSel(int index, bool select /* = true */) {return sendMessageR<int>(LB_SETSEL, select, index);}

inline int ListBox::getCaretIndex() const {return sendMessageC<int>(LB_GETCARETINDEX);}

inline int ListBox::setCaretIndex(int index, bool scroll /* = true */) {return sendMessageR<int>(LB_SETCARETINDEX, index, scroll);}

inline int ListBox::getSelCount() const {return sendMessageC<int>(LB_GETSELCOUNT);}

inline int ListBox::getSelItems(int maxItems, INT* indices) const {return sendMessageC<int>(LB_GETSELITEMS, maxItems, reinterpret_cast<LPARAM>(indices));}

inline int ListBox::selItemRange(int firstItem, int lastItem, bool select /* = true */) {return sendMessageR<int>(LB_SELITEMRANGE, select, MAKELPARAM(firstItem, lastItem));}

inline void ListBox::setAnchorIndex(int index) {sendMessage(LB_SETANCHORINDEX, index);}

inline int ListBox::getAnchorIndex() const {return sendMessageC<int>(LB_GETANCHORINDEX);}

inline int ListBox::addString(const WCHAR* text) {return sendMessageR<int>(LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text));}

inline int ListBox::deleteString(UINT index) {return sendMessageR<int>(LB_DELETESTRING, index);}

inline int ListBox::insertString(int index, const WCHAR* text) {return sendMessageR<int>(LB_INSERTSTRING, index, reinterpret_cast<LPARAM>(text));}

inline void ListBox::resetContent() {sendMessage(LB_RESETCONTENT);}

inline int ListBox::dir(UINT attributes, const WCHAR* fileSpec) {return sendMessageR<int>(LB_DIR, attributes, reinterpret_cast<LPARAM>(fileSpec));}

inline int ListBox::findString(int startAfter, const WCHAR* text) const {return sendMessageC<int>(LB_FINDSTRING, startAfter, reinterpret_cast<LPARAM>(text));}

inline int ListBox::findStringExact(int startAfter, const WCHAR* text) const {return sendMessageC<int>(LB_FINDSTRINGEXACT, startAfter, reinterpret_cast<LPARAM>(text));}

inline int ListBox::selectString(int startAfter, const WCHAR* text) {return sendMessageR<int>(LB_SELECTSTRING, startAfter, reinterpret_cast<LPARAM>(text));}


// ScrollBar ////////////////////////////////////////////////////////////////

inline bool ScrollBar::enableScrollBar(UINT arrowFlags /* = ESB_ENABLE_BOTH */) {return toBoolean(::EnableScrollBar(use(), SB_CTL, arrowFlags));}

#if(WINVER >= 0x0500)
inline bool ScrollBar::getScrollBarInformation(SCROLLBARINFO& scrollInfo) const {
#ifdef SBM_GETSCROLLBARINFO
	return sendMessageC<bool>(SBM_GETSCROLLBARINFO, 0, reinterpret_cast<LPARAM>(&scrollInfo));
#else
	return toBoolean(::GetScrollBarInfo(use(), OBJID_CLIENT, &scrollInfo));
#endif // !SBM_GETSCROLLBARINFO
}
#endif // WINVER >= 0x0500

inline bool ScrollBar::getScrollInformation(SCROLLINFO& scrollInfo) const {return toBoolean(::GetScrollInfo(use(), SB_CTL, &scrollInfo));}

inline int ScrollBar::getScrollLimit() const {
	AutoZeroSize<SCROLLINFO> scrollInfo;
	int minPos = 0, maxPos = 0;
	getScrollRange(&minPos, &maxPos);
	scrollInfo.fMask = SIF_PAGE;
	if(getScrollInformation(scrollInfo))
		maxPos -= (scrollInfo.nPage - 1 > 0) ? scrollInfo.nPage - 1 : 0;
	return maxPos;
}

inline int ScrollBar::getScrollPosition() const {return ::GetScrollPos(use(), SB_CTL);}

inline void ScrollBar::getScrollRange(int* minPos, int* maxPos) const {::GetScrollRange(use(), SB_CTL, minPos, maxPos);}

inline int ScrollBar::setScrollInformation(const SCROLLINFO& scrollInfo, bool redraw /* = true */) {return ::SetScrollInfo(use(), SB_CTL, &scrollInfo, redraw);}

inline int ScrollBar::setScrollPosition(int pos, bool redraw /* = true */) {return ::SetScrollPos(use(), SB_CTL, pos, redraw);}

inline void ScrollBar::setScrollRange(int minPos, int maxPos, bool redraw /* = true */) {::SetScrollRange(use(), SB_CTL, minPos, maxPos, redraw);}

inline void ScrollBar::showScrollBar(bool show /* = true */) {::ShowScrollBar(use(), SB_CTL, show);}


// Static ///////////////////////////////////////////////////////////////////

inline HBITMAP Static::getBitmap() const {return reinterpret_cast<HBITMAP>(sendMessageC<LRESULT>(STM_GETIMAGE, IMAGE_BITMAP));}

inline HCURSOR Static::getCursor() const {return reinterpret_cast<HCURSOR>(sendMessageC<LRESULT>(STM_GETIMAGE, IMAGE_CURSOR));}

inline HENHMETAFILE Static::getEnhMetaFile() const {return reinterpret_cast<HENHMETAFILE>(sendMessageC<LRESULT>(STM_GETIMAGE, IMAGE_ENHMETAFILE));}

inline HICON Static::getIcon() const {return reinterpret_cast<HICON>(sendMessageC<LRESULT>(STM_GETICON));}

inline HBITMAP Static::setBitmap(HBITMAP bitmap) {
	return reinterpret_cast<HBITMAP>(sendMessageC<LRESULT>(STM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(bitmap)));}

inline HCURSOR Static::setCursor(HCURSOR cursor) {
	return reinterpret_cast<HCURSOR>(sendMessageC<LRESULT>(STM_SETIMAGE, IMAGE_CURSOR, reinterpret_cast<LPARAM>(cursor)));}

inline HENHMETAFILE Static::setEnhMetaFile(HENHMETAFILE metaFile) {
	return reinterpret_cast<HENHMETAFILE>(sendMessageC<LRESULT>(STM_GETIMAGE, IMAGE_ENHMETAFILE, reinterpret_cast<LPARAM>(metaFile)));}

inline HICON Static::setIcon(HICON icon) {return reinterpret_cast<HICON>(sendMessageC<LRESULT>(STM_SETICON, reinterpret_cast<WPARAM>(icon)));}

}}} // namespace manah.win32.ui
