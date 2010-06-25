/**
 * @file common-controls.hpp
 * Defines common control classes.
 * @date 2002-2009 exeal
 */

#ifndef MANAH_COMMON_CONTROLS_HPP
#define MANAH_COMMON_CONTROLS_HPP
#include "window.hpp"
#include <commctrl.h>

namespace manah {
namespace win32 {
namespace ui {

#define CommonControl StandardControl

inline bool initCommonControls(DWORD controls) {
	INITCOMMONCONTROLSEX iccex;
	iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	iccex.dwICC = controls;
	return toBoolean(::InitCommonControlsEx(&iccex));
}

class AnimateCtrl : public CommonControl<AnimateCtrl> {
	DEFINE_CLASS_NAME(ANIMATE_CLASSW)
public:
	MANAH_WIN32_OBJECT_CONSTRUCTORS(AnimateCtrl)
	// methods
	bool close();
	bool open(const ResourceID& id, HINSTANCE instance = 0);
	bool play(UINT from, UINT to, UINT repeatCount);
	bool seek(UINT to);
	bool stop();
};

class DateTimePickerCtrl : public CommonControl<DateTimePickerCtrl> {
	DEFINE_CLASS_NAME(DATETIMEPICK_CLASSW)
public:
	MANAH_WIN32_OBJECT_CONSTRUCTORS(DateTimePickerCtrl)
	// methods
	HWND getMonthCalendar() const;
	COLORREF getMonthCalendarColor(int colorType) const;
	HFONT getMonthCalendarFont() const;
	DWORD getRange(SYSTEMTIME times[]) const;
	DWORD getSystemTime(SYSTEMTIME& time) const;
	bool setFormat(const WCHAR* format);
	COLORREF setMonthCalendarColor(int colorType, COLORREF color);
	void setMonthCalendarFont(HFONT font, bool redraw = true);
	bool setRange(DWORD flags, const SYSTEMTIME times[]);
	bool setSystemTime(DWORD flags, const SYSTEMTIME& time);
};

class HotKeyCtrl : public CommonControl<HotKeyCtrl> {
	DEFINE_CLASS_NAME(HOTKEY_CLASSW)
public:
	MANAH_WIN32_OBJECT_CONSTRUCTORS(HotKeyCtrl)
	// methods
	DWORD getHotKey() const;
	void getHotKey(WORD& virutalKeyCode, WORD& modifiers) const;
	const WCHAR* getHotKeyName() const;
	static const WCHAR* getKeyName(UINT virtualKey, bool extended);
	void setHotKey(WORD virtualKeyCode, WORD modifiers);
	void setRules(WORD invalidCombination, WORD modifiers);
};

class ImageList : public Object<HIMAGELIST, ::ImageList_Destroy> {
public:
	MANAH_WIN32_OBJECT_CONSTRUCTORS(ImageList)
	// constructions
	static ImageList create(int cx, int cy, UINT flags, int initial, int grow);
	static ImageList create(HINSTANCE hinstance, const ResourceID& bitmapName, int cx, int grow, COLORREF maskColor);
	static ImageList createFromImage(HINSTANCE hinstance, const ResourceID& imageName,
		int cx, int grow, COLORREF maskColor, UINT type, UINT flags = LR_DEFAULTCOLOR | LR_DEFAULTSIZE);
	bool destroy();
	static ImageList merge(HIMAGELIST imageList1, int image1, HIMAGELIST imageList2, int image2, int dx, int dy);
	// duplication
	ImageList duplicate() const;
	static ImageList duplicate(HIMAGELIST imageList);
	// persistent
#if 0/*defined(__IStream_INTERFACE_DEFINED__)*/
	static ImageList readFromStream(IStream& stream);
	bool writeToStream(IStream& stream);
#if(_WIN32_WINNT >= 0x0501)
	static HRESULT readFromStream(IStream& stream, REFIID riid, void*& pv, DWORD flags);
	HRESULT writeToStream(IStream& stream, DWORD flags);
#endif // _WIN32_WINNT >= 0x0501
#endif // __IStream_INTERFACE_DEFINED__
	// attributes
	COLORREF getBkColor() const;
	HICON getIcon(int index, UINT flags = ILD_NORMAL) const;
	bool getIconSize(SIZE& size) const;
	bool getIconSize(long& cx, long& cy) const;
	bool getImageInformation(int index, IMAGEINFO& info) const;
	int getNumberOfImages() const;
	COLORREF setBkColor(COLORREF color);
	bool setIconSize(const SIZE& size);
	bool setIconSize(long cx, long cy);
	bool setOverlayImage(int index, int overlayIndex);
	bool setNumberOfImages(UINT newCount);
	// operations
	int add(HBITMAP bitmap, HBITMAP mask = 0);
	int add(HBITMAP bitmap, COLORREF maskColor);
	int add(HICON icon);
	bool copy(int dest, int src, UINT flags = ILCF_MOVE);
	bool copy(int dest, HIMAGELIST imageList, int src, UINT flags = ILCF_MOVE);
	HICON extractIcon(int index) const;
	bool remove(int index);
	bool removeAll();
	bool replace(int index, HBITMAP bitmap, HBITMAP mask);
	int replace(int index, HICON icon);
	// paint
	bool draw(HDC dc, int index, const POINT& pt, UINT style) const;
	bool draw(HDC dc, int index, int x, int y, UINT style) const;
	bool drawEx(HDC dc, int index, const RECT& rect, COLORREF bgColor, COLORREF fgColor, UINT style) const;
	bool drawEx(HDC dc, int index, int x, int y, int dx, int dy, COLORREF bgColor, COLORREF fgColor, UINT style) const;
	bool drawIndirect(const IMAGELISTDRAWPARAMS& params) const;
	// dragging
	bool beginDrag(int index, const POINT& hotSpot) const;
	bool beginDrag(int index, int xHotSpot, int yHotSpot) const;
	static bool	dragEnter(HWND lockWindow, const POINT& pt);
	static bool	dragEnter(HWND lockWindow, int x, int y);
	static bool	dragLeave(HWND lockWindow);
	static bool	dragMove(const POINT& pt);
	static bool	dragMove(int x, int y);
	static bool	dragShowNolock(bool show = true);
	static void	endDrag();
	static ImageList getDragImage(POINT* pt, POINT* hotSpot);
	bool setDragCursorImage(int index, const POINT& hotSpot);
	bool setDragCursorImage(int index, int xHotSpot, int yHotSpot);
};

class IPAddressCtrl : public CommonControl<IPAddressCtrl> {
	DEFINE_CLASS_NAME(WC_IPADDRESSW)
public:
	MANAH_WIN32_OBJECT_CONSTRUCTORS(IPAddressCtrl)
	// methods
	void clearAddress();
	int getAddress(DWORD& address) const;
	bool isBlank() const;
	void setAddress(DWORD address);
	void setFocus(int field);
	void setRange(int filed, ushort range);
	void setRange(int field, uchar min, uchar max);
};

class ListCtrl : public CommonControl<ListCtrl> {
	DEFINE_CLASS_NAME(WC_LISTVIEWW)
public:
	MANAH_WIN32_OBJECT_CONSTRUCTORS(ListCtrl)
	// attributes
	SIZE approximateViewRect(const SIZE& size, int count = -1) const;
	COLORREF getBkColor() const;
	bool getBkImage(LVBKIMAGEW& image) const;
	UINT getCallbackMask() const;
	bool getCheck(int index) const;
	bool getColumn(int index, LVCOLUMNW& column) const;
	bool getColumnOrderArray(INT array[], int count = -1) const;
	int getColumnWidth(int column) const;
	int getCountPerPage() const;
	HWND getEditControl() const;
	DWORD getExtendedStyle() const;
	HWND getHeaderControl() const;
	HCURSOR getHotCursor() const;
	int getHotItem() const;
	DWORD getHoverTime() const;
	HIMAGELIST getImageList(int imageListType) const;
	bool getItem(LVITEMW& item) const;
	int getItemCount() const;
	LPARAM getItemData(int index) const;
	bool getItemPosition(int index, POINT& point) const;
	bool getItemRect(int index, RECT& rect, UINT code) const;
	UINT getItemState(int index, UINT mask) const;
	int getItemText(int index, int subItem, WCHAR* text, int maxLength) const;
	std::wstring getItemText(int index, int subItem) const;
	int getNextItem(int index, int flag) const;
	bool getOrigin(POINT& point) const;
	UINT getSelectedCount() const;
	int getSelectionMark() const;
	int getStringWidth(const WCHAR* text) const;
	bool getSubItemRect(int index, int subItem, int area, RECT& rect) const;
	COLORREF getTextBkColor() const;
	COLORREF getTextColor() const;
	int getTopIndex() const;
	bool getViewRect(RECT& rect) const;
	void getWorkAreas(int count, RECT rect[]) const;
	bool setBkColor(COLORREF color);
	bool setBkImage(const LVBKIMAGEW& image);
	bool setBkImage(HBITMAP bitmap, bool tile = true, int xOffsetPercent = 0, int yOffsetPercent = 0);
	bool setBkImage(const WCHAR* url, bool tile = true, int xOffsetPercent = 0, int yOffsetPercent = 0);
	bool setCallbackMask(UINT mask);
	bool setCheck(int index, bool check = true);
	bool setColumn(int index, const LVCOLUMNW& column);
	bool setColumnOrderArray(int count, INT array[]);
	bool setColumnWidth(int column, int cx);
	DWORD setExtendedStyle(DWORD newStyle);
	DWORD setExtendedStyleEx(DWORD exMask, DWORD exStyle);
	HCURSOR setHotCursor(HCURSOR cursor);
	int setHotItem(int index);
	DWORD setHoverTime(DWORD hoverTime = -1);
	SIZE setIconSpacing(int cx, int cy);
	SIZE setIconSpacing(const SIZE& size);
	HIMAGELIST setImageList(HIMAGELIST imageList, int imageListType);
	bool setItem(const LVITEMW& item);
	bool setItem(int index, int subItem, UINT mask, const WCHAR* text, int image, UINT state, UINT stateMask, LPARAM lParam);
	void setItemCount(int count);
	void setItemCountEx(int count, DWORD flags = LVSICF_NOINVALIDATEALL);
	bool setItemData(int index, DWORD data);
	bool setItemPosition(int index, const POINT& pt);
	bool setItemState(int index, const LVITEMW& item);
	bool setItemState(int index, UINT state, UINT mask);
	bool setItemText(int index, int subItem, const WCHAR* text);
	int setSelectionMark(int index);
	bool setTextBkColor(COLORREF color);
	bool setTextColor(COLORREF color);
	void setWorkAreas(int count, const RECT rect[]);
	int subItemHitTest(LVHITTESTINFO& info);
	// operations
	bool arrange(UINT code);
	HIMAGELIST createDragImage(int index, POINT* pt);	// returned pointer is permanent
	bool deleteAllItems();
	bool deleteColumn(int column);
	bool deleteItem(int index);
	HWND editLabel(int index);
	bool ensureVisible(int index, bool partialOK);
	int findItem(LVFINDINFOW& findInfo, int start = -1) const;
	int hitTest(LVHITTESTINFO& hitTestInfo) const;
	int hitTest(const POINT& pt, UINT* flags = 0) const;
	int insertColumn(int position, const LVCOLUMNW& column);
	int insertColumn(int position, const WCHAR* columnHeading, int format = LVCFMT_LEFT, int width = -1, int subItem = -1);
	int insertItem(const LVITEMW& item);
	int insertItem(int index, const WCHAR* text);
	int insertItem(int index, const WCHAR* text, int image);
	int insertItem(UINT mask, int index, const WCHAR* text, UINT state, UINT stateMask, int image, LPARAM lParam);
	bool redrawItems(int first, int last);
	bool scroll(const SIZE& size);
	bool sortItems(PFNLVCOMPARE compare, DWORD data);
	bool update(int index);
};

class MonthCalendarCtrl : public CommonControl<MonthCalendarCtrl> {
	DEFINE_CLASS_NAME(MONTHCAL_CLASSW)
public:
	MANAH_WIN32_OBJECT_CONSTRUCTORS(MonthCalendarCtrl)
	// attributes
	COLORREF getColor(int colorType) const;
	bool getCurSel(SYSTEMTIME& time) const;
	int getFirstDayOfWeek(bool* localeVal = 0) const;
	int getMaxSelCount() const;
	int getMaxTodayWidth() const;
	bool getMinReqRect(RECT& rect) const;
	int getMonthDelta() const;
	DWORD getRange(SYSTEMTIME times[]) const;
	bool getSelRange(SYSTEMTIME times[]) const;
	bool getToday(SYSTEMTIME& time) const;
	bool getUnicodeFormat() const;
	COLORREF setColor(int colorType, COLORREF color);
	bool setCurSel(const SYSTEMTIME& time);
	int setFirstDayOfWeek(int day, bool* localeVal = 0);
	bool setMaxSelCount(int max);
	int setMonthDelta(int delta);
	bool setRange(DWORD flags, const SYSTEMTIME times[]);
	bool setSelRange(const SYSTEMTIME times[]);
	void setToday(const SYSTEMTIME& time);
	bool setUnicodeFormat(bool unicode = true);
	// operations
	int getMonthRange(DWORD flags, SYSTEMTIME times[]) const;
	DWORD hitTest(MCHITTESTINFO& hitTest);
	bool setDayState(int monthCount, const MONTHDAYSTATE dayStates[]);
};

class PagerCtrl : public CommonControl<PagerCtrl> {
	DEFINE_CLASS_NAME(WC_PAGESCROLLERW)
public:
	MANAH_WIN32_OBJECT_CONSTRUCTORS(PagerCtrl)
	// attributes
	void forwardMouse(bool forward);
	COLORREF getBkColor() const;
	int getBorder() const;
	int getButtonSize() const;
	DWORD getButtonState(int button) const;
	void getDropTarget(::IDropTarget*& pDropTarget) const;
	int getPosition() const;
	COLORREF setBkColor(COLORREF bgColor);
	int setBorder(int border);
	int setButtonSize(int buttonSize);
	void setChild(HWND child);
	int setPosition(int pos);
	// operation
	void recalcSize();
};

class ProgressBarCtrl : public CommonControl<ProgressBarCtrl> {
	DEFINE_CLASS_NAME(PROGRESS_CLASSW)
public:
	MANAH_WIN32_OBJECT_CONSTRUCTORS(ProgressBarCtrl)
	// attributes
	int getHighLimit() const;
	int getLowLimit() const;
	UINT getPosition() const;
	void getRange(PBRANGE& range) const;
	void getRange(int* lower, int* upper) const;
	int offsetPosition(int pos);
	COLORREF setBarColor(COLORREF color);
	COLORREF setBkColor(COLORREF color);
	int setPosition(int pos);
	DWORD setRange(const PBRANGE& range);
	DWORD setRange(int lower, int upper);
	int setStep(int step);
#ifdef PBM_SETMARQUEE
	bool setMarquee(bool marquee, UINT updateTime = 0);
#endif // PBM_SETMARQUEE
	// operation
	int	stepIt();
};

class Rebar : public CommonControl<Rebar> {
	DEFINE_CLASS_NAME(REBARCLASSNAMEW)
public:
	MANAH_WIN32_OBJECT_CONSTRUCTORS(Rebar)
	// attributes
	void getBandBorders(int band, RECT& rect) const;
	UINT getBandCount() const;
	bool getBandInfo(int band, REBARBANDINFOW& info) const;
	UINT getBarHeight() const;
	bool getBarInfo(REBARINFO& info) const;
	COLORREF getBkColor() const;
	bool getColorScheme(COLORSCHEME& scheme) const;
	void getDropTarget(::IDropTarget*& dropTarget) const;
	HIMAGELIST getImageList() const;
	HPALETTE getPalette() const;
	bool getRect(int band, RECT& rect) const;
	using Window::getRect;
	UINT getRowCount() const;
	UINT getRowHeight(int band) const;
	COLORREF getTextColor() const;
	HWND getToolTips() const;
	bool getUnicodeFormat() const;
	int idToIndex(UINT id) const;
	bool setBandInfo(UINT band, const REBARBANDINFOW& info);
	bool setBarInfo(const REBARINFO& info);
	COLORREF setBkColor(COLORREF color);
	bool setImageList(HIMAGELIST imageList);
	HWND setOwner(HWND owner);
	HPALETTE setPalette(HPALETTE palette);
	void setColorScheme(const COLORSCHEME& scheme);
	COLORREF setTextColor(COLORREF color);
	void setToolTips(HWND toolTips);
	bool setUnicodeFormat(bool unicode = true);
	// operations
	void beginDrag(UINT band, DWORD pos = static_cast<DWORD>(-1));
	void beginDrag(UINT band, int x, int y);
	bool deleteBand(UINT band);
	void dragMove(DWORD pos = static_cast<DWORD>(-1));
	void dragMove(int x, int y);
	void endDrag();
	int hitTest(RBHITTESTINFO& info);
	bool insertBand(UINT band, const REBARBANDINFOW& info);
	void lockBands(bool lock);	// from WTL
	void maximizeBand(UINT band);
	void minimizeBand(UINT band);
	bool moveBand(UINT from, UINT to);
	void pushChevron(UINT band, LPARAM lParam);
	void restoreBand(UINT band);
	bool showBand(UINT band, bool show);
	bool sizeToRect(const RECT& rect);
#ifdef RB_GETMARGINS
	void getBandMargins(MARGINS& margins) const;
	void setWindowTheme(const WCHAR* styleName);
#endif // RB_GETMARGINS
};

class StatusBar : public CommonControl<StatusBar> {
	DEFINE_CLASS_NAME(STATUSCLASSNAMEW)
public:
	MANAH_WIN32_OBJECT_CONSTRUCTORS(StatusBar)
	// attributes
	using Window::getText;
	using Window::getTextLength;
	using Window::setText;
	bool getBorders(int borders[]) const;
	bool getBorders(int& horz, int& vert, int& spacing) const;
	HICON getIcon(int pane) const;
	int getParts(int count, int parts[]) const;
	bool getRect(int pane, RECT& rect) const;
	using Window::getRect;
	int getText(int pane, WCHAR* text, int* type = 0) const;
	int getTextLength(int pane, int* type = 0) const;
	void getTipText(int pane, WCHAR* text, int len) const;
	bool getUnicodeFormat() const;
	bool isSimple() const;
	COLORREF setBkColor(COLORREF color);
	bool setIcon(int pane, HICON icon);
	using Window::setIcon;
	void setMinHeight(int height);
	bool setParts(int count, int parts[]);
	bool setSimple(bool simple = true);
	bool setText(int pane, const WCHAR* text, int type = 0);
	void setTipText(int pane, const WCHAR* text);
	bool setUnicodeFormat(bool unicode = true);
	bool showTemporaryText(const WCHAR* text, UINT duration);
protected:
	void onSize(UINT type, int cx, int cy);
private:
	void restoreTemporaryText();
	static void CALLBACK timeElapsed(HWND window, UINT message, UINT_PTR eventID, DWORD time);
	AutoBuffer<WCHAR> originalText_;
};

class TabCtrl : public CommonControl<TabCtrl> {
	DEFINE_CLASS_NAME(WC_TABCONTROLW)
public:
	MANAH_WIN32_OBJECT_CONSTRUCTORS(TabCtrl)
	// attributes
	HIMAGELIST getImageList() const;
	HIMAGELIST setImageList(HIMAGELIST imageList);
	int getItemCount() const;
	bool getItem(int index, TCITEMW& tabCtrlItem) const;
	bool setItem(int index, const TCITEMW& tabCtrlItem);
	bool setItemExtra(int bytes);
	bool getItemRect(int index, RECT& rect) const;
	int getCurSel() const;
	int setCurFocus(int index);
	int setCurSel(int index);
	SIZE setItemSize(const SIZE& size);
	void setPadding(const SIZE& size);
	int getRowCount() const;
	HWND getToolTips() const;
	void setToolTips(HWND toolTip);
	int getCurFocus() const;
	int setMinTabWidth(int cx);
	DWORD getExtendedStyle() const;
	DWORD setExtendedStyle(DWORD newStyle, DWORD exMask = 0);
	bool getItemState(int index, DWORD mask, DWORD& state) const;
	bool setItemState(int index, DWORD mask, DWORD state);
	// operations
	void adjustRect(bool larger, RECT& rect);
	bool deleteItem(int index);
	bool deleteAllItems();
	void deselectAll(bool excludeFocus);
	bool insertItem(int index, const TCITEMW& item);
	bool insertItem(int index, const WCHAR* text);
	bool insertItem(int index, const WCHAR* text, int image);
	bool insertItem(UINT mask, int index, const WCHAR* text, int image, LPARAM lParam);
	bool highlightItem(int index, bool highlight = true);
	int hitTest(TCHITTESTINFO& hitTestInfo) const;
	void removeImage(int index);

protected:
	virtual void drawItem(const DRAWITEMSTRUCT& drawItemStruct);
};

class Toolbar : public CommonControl<Toolbar> {
	DEFINE_CLASS_NAME(TOOLBARCLASSNAMEW)
public:
	MANAH_WIN32_OBJECT_CONSTRUCTORS(Toolbar)
	// attributes
	bool getAnchorHighlight() const;
	int getBitmap(int id) const;
	DWORD getBitmapFlags() const;
	bool getButton(int index, TBBUTTON& button) const;
	int getButtonCount() const;
	SIZE getButtonSize() const;
	int getButtonText(int id, WCHAR* text) const;
	int getButtonTextLength(int id) const;
	bool getColorScheme(COLORSCHEME& scheme) const;
	HIMAGELIST getDisabledImageList() const;
	HRESULT getDropTarget(::IDropTarget*& dropTarget) const;
	DWORD getExtendedStyle() const;
	HIMAGELIST getHotImageList() const;
	int getHotItem() const;
	HIMAGELIST getImageList() const;
	void getInsertMark(TBINSERTMARK& mark) const;
	COLORREF getInsertMarkColor() const;
	bool getItemRect(int index, RECT& rect) const;
	bool getMaxSize(SIZE& size) const;
	int getMaxTextRows() const;
	void getPadding(SIZE& padding) const;
	bool getRect(int id, RECT& rect) const;
	using Window::getRect;
	int getRows() const;
	int getState(int id) const;
	DWORD getStyle() const;
	HWND getToolTips() const;
	bool getUnicodeFormat() const;
	int hitTest(const POINT& pt) const;
	bool insertMarkHitTest(const POINT& pt, const TBINSERTMARK& mark) const;
	bool isButtonChecked(int id) const;
	bool isButtonEnabled(int id) const;
	bool isButtonHidden(int id) const;
	bool isButtonHighlighted(int id) const;
	bool isButtonIndeterminate(int id) const;
	bool isButtonPressed(int id) const;
	bool mapAccelerator(WCHAR ch, UINT& id);
	bool moveButton(int from, int to);
	bool setAnchorHighlight(bool enable = true);
	bool setBitmapSize(const SIZE& size);
	bool setBitmapSize(int cx, int cy);
	bool setButtonSize(const SIZE& size);
	bool setButtonSize(int cx, int cy);
	void setButtonStructSize(std::size_t size = sizeof(TBBUTTON));
	void setButtonText(int id, const WCHAR* text);
	void setColorScheme(const COLORSCHEME& scheme);
	bool setButtonWidth(int cxMin, int cxMax);
	bool setCommandID(int index, UINT id);
	HIMAGELIST setDisabledImageList(HIMAGELIST imageList);
	DWORD setExtendedStyle(DWORD exStyle);
	HIMAGELIST setHotImageList(HIMAGELIST imageList);
	int setHotItem(int index);
	HIMAGELIST setImageList(HIMAGELIST imageList);
	bool setIndent(int indent);
	void setInsertMark(const TBINSERTMARK& mark);
	COLORREF setInsertMarkColor(COLORREF color);
	bool setMaxTextRows(int rows);
	void setOwner(HWND owner);
	void setPadding(int cx, int cy, SIZE* padding = 0);
	void setRow(int count, bool larger, const RECT& rect);
	bool setState(int id, UINT state);
	void setStyle(DWORD style);
	void setToolTips(HWND toolTips);
	bool setUnicodeFormat(bool unicode = true);
#ifdef TB_GETMETRICS
	void getMetrics(TBMETRICS& metrics) const;
	void setMetrics(const TBMETRICS& metrics);
	void setWindowTheme(const WCHAR* styleName);
#endif // TB_GETMETRICS
	// operations
	int addBitmap(int count, UINT bitmapID);
	int addBitmap(int count, HBITMAP bitmap);
	bool addButtons(int count, const TBBUTTON buttons[]);
	int addString(UINT stringID);
	int addStrings(const WCHAR* strings);
	void autoSize();
	bool changeBitmap(int id, int bitmap);
	bool checkButton(int id, bool check = true);
	UINT commandToIndex(int id) const;
	void customize();
	bool deleteButton(int index);
	bool enableButton(int id, bool enable = true);
	bool getButtonInfo(int id, TBBUTTONINFOW& info) const;
	int getString(int index, WCHAR* text, int maxLength) const;
	bool hideButton(int id, bool hide = true);
	bool indeterminate(int id, bool isIndeterminate = true);
	bool insertButton(int index, const TBBUTTON& button);
	void loadImages(int imageID);
	void loadStdImages(int imageID);
	bool markButton(int id, bool highlight = true);
	bool pressButton(int id, bool press = true);
	bool replaceBitmap(const TBREPLACEBITMAP& bitmap);
	void restoreState(HKEY keyRoot, const WCHAR* subKey, const WCHAR* valueName);
	void saveState(HKEY keyRoot, const WCHAR* subKey, const WCHAR* valueName);
	bool setButtonInfo(int id, const TBBUTTONINFOW& info);
	DWORD setDrawTextFlags(DWORD mask, DWORD flags);
};

class ToolTipCtrl :
		public CommonControl<ToolTipCtrl, AdditiveWindowStyles<WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, WS_EX_TOOLWINDOW> > {
	DEFINE_CLASS_NAME(TOOLTIPS_CLASSW)
public:
	// constructions
	MANAH_WIN32_OBJECT_CONSTRUCTORS(ToolTipCtrl)
	bool create(HWND parent, const RECT& rect = DefaultWindowRect(),
		const WCHAR* windowName = 0, INT_PTR id = 0, DWORD style = 0, DWORD exStyle = 0);
	// attributes
	SIZE getBubbleSize(const TOOLINFOW& toolInfo) const;
	bool getCurrentTool(TOOLINFOW& toolInfo) const;
	int getDelayTime(DWORD duration) const;
	void getMargin(RECT &rect) const;
	int getMaxTipWidth() const;
	void getText(WCHAR* text, HWND window, UINT toolID = 0) const;
	COLORREF getTipBkColor() const;
	COLORREF getTipTextColor() const;
	int getToolCount() const;
	bool getToolInfo(TOOLINFOW& toolInfo, HWND window, UINT toolID = 0) const;
	void setDelayTime(UINT delay);
	void setDelayTime(DWORD duration, int time);
	void setMargin(const RECT& rect);
	int setMaxTipWidth(int width);
	void setTipBkColor(COLORREF color);
	void setTipTextColor(COLORREF color);
	bool setTitle(UINT icon, const WCHAR* title);
	void setToolInfo(const TOOLINFOW& toolInfo);
#ifdef TTM_GETTITLE
	void getTitle(TTGETTITLE& title) const;
	void setWindowTheme(const WCHAR* theme);
#endif // TTM_GETTITLE
	// operation
	bool activate(bool active);
	bool adjustRect(RECT& rect, bool larger = true);
	bool addTool(const TOOLINFOW& toolInfo);
	bool addTool(HWND container, UINT id, UINT flags,
		const RECT& toolRect, const WCHAR* text = LPSTR_TEXTCALLBACKW, LPARAM lParam = 0);
	bool addTool(HWND tool, UINT flags, const WCHAR* text = LPSTR_TEXTCALLBACKW, LPARAM lParam = 0);
	void deleteTool(HWND window, UINT id = 0);
	void deleteTool(HWND window, HWND control);
	bool enumTools(UINT index, TOOLINFOW& toolInfo) const;
	bool hitTest(TTHITTESTINFOW& hitTestInfo) const;
	bool hitTest(HWND window, const POINT& pt, TOOLINFOW& toolInfo) const;
	void pop();
	void relayEvent(MSG& message);
	void setToolRect(HWND window, UINT toolID, const RECT& rect);
	void trackActivate(const TOOLINFOW& toolInfo, bool activate);
	void trackPosition(int x, int y);
	void update();
	void updateTipText(const WCHAR* text, HWND window, UINT toolID = 0);
	void updateTipText(const WCHAR* text, HWND window, HWND control);
#ifdef TTM_POPUP
	void popup();
#endif // TTM_POPUP
};

class TreeCtrl : public CommonControl<TreeCtrl> {
	DEFINE_CLASS_NAME(WC_TREEVIEWW)
public:
	MANAH_WIN32_OBJECT_CONSTRUCTORS(TreeCtrl)
	// attributes
	COLORREF getBkColor() const;
	bool getCheck(HTREEITEM item) const;
	HTREEITEM getChildItem(HTREEITEM item) const;
	UINT getCount() const;
	HTREEITEM getDropHilightItem() const;
	HWND getEditControl() const;
	HTREEITEM getFirstVisibleItem() const;
	HIMAGELIST getImageList(UINT image) const;
	UINT getIndent() const;
	COLORREF getInsertMarkColor() const;
	UINT getISearchString(WCHAR* text);
	bool getItem(TVITEMW& item) const;
	bool getItem(TVITEMEXW& item) const;
	LPARAM getItemData(HTREEITEM item) const;
	SHORT getItemHeight() const;
	bool getItemImage(HTREEITEM item, int& image, int& selectedImage) const;
	bool getItemRect(HTREEITEM item, RECT& rect, bool textOnly);
	UINT getItemState(HTREEITEM item, UINT stateMask) const;
	bool getItemText(HTREEITEM item, WCHAR* text, int maxLength) const;
	HTREEITEM getLastVisibleItem() const;
	COLORREF getLineColor() const;
	HTREEITEM getNextItem(HTREEITEM item, UINT code) const;
	HTREEITEM getNextSiblingItem(HTREEITEM item) const;
	HTREEITEM getNextVisibleItem(HTREEITEM item) const;
	HTREEITEM getParentItem(HTREEITEM item) const;
	HTREEITEM getPrevSiblingItem(HTREEITEM item) const;
	HTREEITEM getPrevVisibleItem(HTREEITEM item) const;
	HTREEITEM getRootItem() const;
	UINT getScrollTime() const;
	HTREEITEM getSelectedItem() const;
	COLORREF getTextColor() const;
	HWND getToolTips() const;
	bool getUnicodeFormat() const;
	UINT getVisibleCount() const;
	bool itemHasChildren(HTREEITEM item) const;
	COLORREF setBkColor(COLORREF color);
	bool setCheck(HTREEITEM item, bool check = true);
	HIMAGELIST setImageList(HIMAGELIST imageList, UINT image);
	void setIndent(UINT indent);
	bool setInsertMark(HTREEITEM item, bool after = true);
	COLORREF setInsertMarkColor(COLORREF color);
	bool setItem(const TVITEMW& item);
	bool setItem(const TVITEMEXW& item);
	bool setItem(HTREEITEM item, UINT mask, const WCHAR* text,
		int image, int selectedImage, UINT state, UINT stateMask, LPARAM lParam);
	bool setItemData(HTREEITEM item, DWORD data);
	SHORT setItemHeight(SHORT cyHeight);
	bool setItemState(HTREEITEM item, UINT state, UINT stateMask);
	bool setItemImage(HTREEITEM item, int image, int selectedImage);
	bool setItemText(HTREEITEM item, const WCHAR* text);
	COLORREF setLineColor(COLORREF color = CLR_DEFAULT);
	UINT setScrollTime(UINT scrollTime);
	COLORREF setTextColor(COLORREF color);
	HWND setToolTips(HWND toolTips);
	bool setUnicodeFormat(bool unicode = true);
	// operations
	ImageList createDragImage(HTREEITEM item);
	bool deleteAllItems();
	bool deleteItem(HTREEITEM item);
	HWND editLabel(HTREEITEM item);
	bool endEditLabelNow();
	bool ensureVisible(HTREEITEM item);
	bool expandItem(HTREEITEM item, UINT code);
	HTREEITEM hitTest(TVHITTESTINFO& info);
	HTREEITEM hitTest(const POINT& pt, UINT flags);
	HTREEITEM insertItem(const TVINSERTSTRUCTW& insertStruct);
	HTREEITEM insertItem(UINT mask, const WCHAR* text,
		int image, int selectedImage, UINT state, UINT stateMask,
		LPARAM lParam, HTREEITEM parent = TVI_ROOT, HTREEITEM insertAfter = TVI_LAST);
	HTREEITEM insertItem(const WCHAR* text, HTREEITEM parent = TVI_ROOT, HTREEITEM insertAfter = TVI_LAST);
	HTREEITEM insertItem(const WCHAR* text, int image, int selectedImage,
		HTREEITEM parent = TVI_ROOT, HTREEITEM insertAfter = TVI_LAST);
	HTREEITEM mapAccIdToHTREEITEM(UINT id) const;
	UINT mapHTREEITEMToAccId(HTREEITEM item) const;
	bool selectDropTarget(HTREEITEM item);
	bool selectItem(HTREEITEM item, UINT code = TVGN_CARET);
	bool selectSetFirstVisible(HTREEITEM item);
	bool sortChildren(HTREEITEM item, bool recurse = false);
	bool sortChildrenCB(const TVSORTCB& sort, bool recurse = false);
};

class UpDownCtrl : public CommonControl<UpDownCtrl> {
	DEFINE_CLASS_NAME(UPDOWN_CLASSW)
public:
	MANAH_WIN32_OBJECT_CONSTRUCTORS(UpDownCtrl)
	// methods
	UINT getAccel(int count, UDACCEL accel[]) const;
	UINT getBase() const;
	Window getBuddy() const;
	int getPosition(bool* error = 0) const;
	void getRange(int& lower, int& upper) const;
	bool getUnicodeFormat() const;
	bool setAccel(int count, const UDACCEL accel[]);
	int setBase(int base);
	Window setBuddy(HWND buddy);
	int setPosition(int pos);
	void setRange(int lower, int upper);
	bool setUnicodeFormat(bool unicode = true);
};
typedef UpDownCtrl SpinCtrl;

}}} // namespace manah.win32.controls

#include "common-controls.inl"

#endif // !MANAH_COMMON_CONTROLS_HPP
