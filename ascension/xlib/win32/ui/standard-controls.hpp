// standard-controls.hpp
// (c) 2002-2009 exeal

#ifndef MANAH_STANDARD_CONTROLS_HPP
#define MANAH_STANDARD_CONTROLS_HPP
#include "window.hpp"
#include <commctrl.h>

namespace manah {
namespace win32 {
namespace ui {

class Button : public StandardControl<Button> {
	MANAH_NONCOPYABLE_TAG(Button);
	DEFINE_CLASS_NAME(L"BUTTON")
public:
	MANAH_WIN32_OBJECT_CONSTRUCTORS(Button)
	// attributes
	HBITMAP getBitmap() const;
	UINT getButtonStyle() const;
	int getCheck() const;
	HCURSOR getCursor() const;
	HICON getIcon() const;
	bool getIdealSize(SIZE& size) const;
	UINT getState() const;
	bool getTextMargin(RECT& margin) const;
	HBITMAP setBitmap(HBITMAP bitmap);
	void setButtonStyle(UINT style, bool redraw = true);
	void setCheck(int check);
	HCURSOR setCursor(HCURSOR cursor);
	HICON setIcon(HICON icon);
	void setState(bool highlight);
	bool setTextMargin(const RECT& margin);
#ifdef BCM_FIRST
	bool getImageList(BUTTON_IMAGELIST& bi) const;
	bool setImageList(const BUTTON_IMAGELIST& bi);
#endif // BCM_FIRST
	// operation
	void click();
};

class ComboBox : public StandardControl<ComboBox> {
	MANAH_NONCOPYABLE_TAG(ComboBox);
	DEFINE_CLASS_NAME(L"COMBOBOX")
public:
	MANAH_WIN32_OBJECT_CONSTRUCTORS(ComboBox)
	// initialize
	int initStorage(int itemCount, UINT bytes);
	// attributes
	int getCount() const;
	int getCurSel() const;
	void getDroppedControlRect(RECT& rect) const;
	bool getDroppedState() const;
	int getDroppedWidth() const;
	DWORD getEditSel() const;
	bool getExtendedUI() const;
	UINT getHorizontalExtent() const;
	DWORD getItemData(int index) const;
	void* getItemDataPtr(int index) const;
	int getItemHeight(int index) const;
	int getLBText(int index, WCHAR* text) const;
	int getLBTextLen(int index) const;
	LCID getLocale() const;
	int getMinVisible() const;
	int getTopIndex() const;
	int setCurSel(int select);
	int setDroppedWidth(UINT width);
	bool setEditSel(int startChar, int endChar);
	int setExtendedUI(bool extended = true);
	void setHorizontalExtent(UINT extent);
	int setItemData(int index, DWORD itemData);
	int setItemDataPtr(int index, const void* itemData);
	int setItemHeight(int index, UINT cyItemHeight);
	LCID setLocale(LCID lcid);
	bool setMinVisible(int minVisible);
	int setTopIndex(int index);
#ifdef CB_GETCOMBOBOXINFO
	bool getComboBoxInformation(COMBOBOXINFO& cbi) const;
#endif
	// operations
	int addString(const WCHAR* text);
	int deleteString(UINT index);
	int dir(UINT attributes, const WCHAR* fileSpec);
	int findString(int startAfter, const WCHAR* text) const;
	int findStringExact(int iStart, const WCHAR* text) const;
	int insertString(int index, const WCHAR* text);
	bool limitText(int maxChars);
	void resetContent();
	int selectString(int startAfter, const WCHAR* text);
	void showDropDown(bool show = true);
	// clipboard
	void clear();
	void copy();
	void cut();
	void paste();
};

class Edit : public StandardControl<Edit> {
	MANAH_NONCOPYABLE_TAG(Edit);
	DEFINE_CLASS_NAME(L"EDIT")
public:
	MANAH_WIN32_OBJECT_CONSTRUCTORS(Edit)
	// attributes
	bool canUndo() const;
	int charFromPos(const POINT& pt) const;
	void getEditRect(RECT& rect) const;
	int getFirstVisibleLine() const;
	HLOCAL getBufferHandle() const;
	UINT getLimitText() const;
	int getLine(int nIndex, WCHAR* buffer) const;
	int getLine(int nIndex, WCHAR* buffer, int maxLength) const;
	int getLineCount() const;
	DWORD getMargins() const;
	bool getModify() const;
	DWORD getSel() const;
	void getSel(int& startChar, int& endChar) const;
	int getThumb() const;
	WCHAR getPasswordChar() const;
	EDITWORDBREAKPROCW getWordBreakProc() const;
	int lineFromChar(int index = -1) const;
	int lineIndex(int index = -1) const;
	int lineLength(int line = -1) const;
	POINT posFromChar(UINT charPos) const;
	void setHandle(HLOCAL buffer);
	void setLimitText(UINT maxLength);
	void setMargins(UINT left, UINT right);
	void setModify(bool modified = true);
	void setPasswordChar(WCHAR ch);
	void setReadOnly(bool readOnly = true);
	void setRect(const RECT& rect);
	void setRectNP(const RECT& rect);
	void setTabStops();
	bool setTabStops(uint cxEachStop);
	bool setTabStops(int count, uint tabStops[]);
	void setWordBreakProc(EDITWORDBREAKPROCW proc);
#ifdef EM_GETIMESTATUS
	DWORD getImeStatus(DWORD type) const;
	DWORD setImeStatus(DWORD type, DWORD data);
#endif // EM_GETIMESTATUS
	// operations
	void emptyUndoBuffer();
	bool fmtLines(bool addEol);
	void limitText(int maxLength = 0);
	void lineScroll(int lines, int chars = 0);
	void replaceSel(const WCHAR* newText, bool canUndo = false);
	bool scrollCaret();
	void setSel(DWORD selection, bool noScroll = false);
	void setSel(int startChar, int endChar, bool noScroll = false);
#ifdef EM_GETCUEBANNER
	bool getCueBanner(WCHAR* text, int maxLength) const;
	bool hideBalloonTip();
	bool setCueBanner(const WCHAR* text);
	bool showBalloonTip(const EDITBALLOONTIP& ebt);
#endif // EM_GETCUEBANNER
};

class ListBox : public StandardControl<ListBox> {
	MANAH_NONCOPYABLE_TAG(ListBox);
	DEFINE_CLASS_NAME(L"LISTBOX")
public:
	MANAH_WIN32_OBJECT_CONSTRUCTORS(ListBox)
	// initialize
	int initStorage(int itemCount, UINT bytes);
	// attributes
	int getAnchorIndex() const;
	int getCaretIndex() const;
	int getCount() const;
	int getCurSel() const;
	int getHorizontalExtent() const;
	DWORD getItemData(int index) const;
	void* getItemDataPtr(int index) const;
	int getItemRect(int index, RECT& rect) const;
	int getItemHeight(int index) const;
	LCID getLocale() const;
	int getSel(int index) const;
	int getSelCount() const;
	int getSelItems(int maxItems, INT* indices) const;
	int getText(int index, WCHAR* buffer) const;
	int getTextLen(int index) const;
	int getTopIndex() const;
	UINT itemFromPoint(const POINT& pt, bool& outSide) const;
	void setAnchorIndex(int index);
	int setCaretIndex(int index, bool scroll = true);
	void setColumnWidth(int width);
	int setCurSel(int select);
	void setHorizontalExtent(int extent);
	int setItemData(int index, DWORD itemData);
	int setItemDataPtr(int index, void* data);
	int setItemHeight(int index, UINT itemHeight);
	int selItemRange(int firstItem, int lastItem, bool select = true);
	LCID setLocale(LCID newLocale);
	int setSel(int index, bool select = true);
	void setTabStops();
	bool setTabStops(int cxEachStop);
	bool setTabStops(int count, INT* tabStops);
	int setTopIndex(int index);
	// operations
	int addString(const WCHAR* text);
	int deleteString(UINT index);
	int dir(UINT attr, const WCHAR* wildCard);
	int findString(int startAfter, const WCHAR* text) const;
	int findStringExact(int startAfter, const WCHAR* text) const;
	int insertString(int index, const WCHAR* text);
	void resetContent();
	int selectString(int startAfter, const WCHAR* text);
};

class ScrollBar : public StandardControl<ScrollBar> {
	MANAH_NONCOPYABLE_TAG(ScrollBar);
	DEFINE_CLASS_NAME(L"SCROLLBAR")
public:
	MANAH_WIN32_OBJECT_CONSTRUCTORS(ScrollBar)
	// attributes
	bool getScrollInformation(SCROLLINFO& scrollInfo) const;
	int getScrollLimit() const;
	int getScrollPosition() const;
	void getScrollRange(int* minPos, int* maxPos) const;
	int setScrollInformation(const SCROLLINFO& scrollInfo, bool redraw = true);
	int setScrollPosition(int pos, bool redraw = true);
	void setScrollRange(int min, int max, bool redraw = true);
#if(WINVER >= 0x0500)
	bool getScrollBarInformation(SCROLLBARINFO& scrollInfo) const;
#endif // WINVER >= 0x0500
	// operations
	bool enableScrollBar(UINT arrowFlags = ESB_ENABLE_BOTH);
	void showScrollBar(bool show = true);
};

class Static : public StandardControl<Static> {
	MANAH_NONCOPYABLE_TAG(Static);
	DEFINE_CLASS_NAME(L"STATIC")
public:
	MANAH_WIN32_OBJECT_CONSTRUCTORS(Static)
	// methods
	HBITMAP getBitmap() const;
	HCURSOR getCursor() const;
	HENHMETAFILE getEnhMetaFile() const;
	HICON getIcon() const;
	HBITMAP setBitmap(HBITMAP bitmap);
	HCURSOR setCursor(HCURSOR cursor);
	HENHMETAFILE setEnhMetaFile(HENHMETAFILE metaFile);
	HICON setIcon(HICON icon);
};

class DragListBox : public ListBox {
	MANAH_NONCOPYABLE_TAG(DragListBox);
public:
	DragListBox() : ListBox() {}
	template<typename T> explicit DragListBox(T* handle) : ListBox(handle) {}
	// methods
	void drawInsert(int index);
	static UINT getDragListMessage();
	int lbItemFromPtr(const POINT& pt, bool autoScroll = true);
	bool makeDragList();
};

}}} // namespace manah.win32.ui

#include "standard-controls.inl"

#endif // !MANAH_STANDARD_CONTROLS_HPP
