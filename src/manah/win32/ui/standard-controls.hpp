// standard-controls.hpp
// (c) 2002-2007 exeal

#ifndef MANAH_STANDARD_CONTROLS_HPP
#define MANAH_STANDARD_CONTROLS_HPP
#include "window.hpp"
#include <commctrl.h>

namespace manah {
namespace windows {
namespace ui {

class Button : public StandardControl<Button> {
	DEFINE_CLASS_NAME(_T("BUTTON"))
public:
	// コンストラクタ
	explicit Button(HWND handle = 0) : StandardControl<Button>(handle) {}
	// 属性
	HBITMAP	getBitmap() const;
	UINT	getButtonStyle() const;
	int		getCheck() const;
	HCURSOR	getCursor() const;
	HICON	getIcon() const;
	bool	getIdealSize(SIZE& size) const;
	UINT	getState() const;
	bool	getTextMargin(RECT& margin) const;
	HBITMAP	setBitmap(HBITMAP bitmap);
	void	setButtonStyle(UINT style, bool redraw = true);
	void	setCheck(int check);
	HCURSOR	setCursor(HCURSOR cursor);
	HICON	setIcon(HICON icon);
	void	setState(bool highlight);
	bool	setTextMargin(const RECT& margin);
#ifdef BCM_FIRST
	bool	getImageList(BUTTON_IMAGELIST& bi) const;
	bool	setImageList(const BUTTON_IMAGELIST& bi);
#endif /* BCM_FIRST */
	// 操作
	void	click();
};

class ComboBox : public StandardControl<ComboBox> {
	DEFINE_CLASS_NAME(_T("COMBOBOX"))
public:
	// コンストラクタ
	explicit ComboBox(HWND handle = 0) : StandardControl<ComboBox>(handle) {}
	// 作成
	int		initStorage(int itemCount, UINT bytes);
	// 属性
	int		getCount() const;
	int		getCurSel() const;
	void	getDroppedControlRect(RECT& rect) const;
	bool	getDroppedState() const;
	int		getDroppedWidth() const;
	DWORD	getEditSel() const;
	bool	getExtendedUI() const;
	UINT	getHorizontalExtent() const;
	DWORD	getItemData(int index) const;
	void*	getItemDataPtr(int index) const;
	int		getItemHeight(int index) const;
	int		getLBText(int index, TCHAR* text) const;
	int		getLBTextLen(int index) const;
	LCID	getLocale() const;
	int		getMinVisible() const;
	int		getTopIndex() const;
	int		setCurSel(int select);
	int		setDroppedWidth(UINT width);
	bool	setEditSel(int startChar, int endChar);
	int		setExtendedUI(bool extended = true);
	void	setHorizontalExtent(UINT extent);
	int		setItemData(int index, DWORD itemData);
	int		setItemDataPtr(int index, const void* itemData);
	int		setItemHeight(int index, UINT cyItemHeight);
	LCID	setLocale(LCID lcid);
	bool	setMinVisible(int minVisible);
	int		setTopIndex(int index);
#ifdef CB_GETCOMBOBOXINFO
	bool	getComboBoxInfo(COMBOBOXINFO& cbi) const;
#endif
	// 操作
	int		addString(const TCHAR* text);
	int		deleteString(UINT index);
	int		dir(UINT attributes, const TCHAR* fileSpec);
	int		findString(int startAfter, const TCHAR* text) const;
	int		findStringExact(int iStart, const TCHAR* text) const;
	int		insertString(int index, const TCHAR* text);
	bool	limitText(int maxChars);
	void	resetContent();
	int		selectString(int startAfter, const TCHAR* text);
	void	showDropDown(bool show = true);
	// クリップボード操作
	void	clear();
	void	copy();
	void	cut();
	void	paste();
};

class Edit : public StandardControl<Edit> {
	DEFINE_CLASS_NAME(_T("EDIT"))
public:
	// コンストラクタ
	explicit Edit(HWND handle = 0) : StandardControl<Edit>(handle) {}
	// 属性
	bool				canUndo() const;
	int					charFromPos(const POINT& pt) const;
	int					getFirstVisibleLine() const;
	HLOCAL				getBufferHandle() const;
	UINT				getLimitText() const;
	int					getLine(int nIndex, TCHAR* buffer) const;
	int					getLine(int nIndex, TCHAR* buffer, int maxLength) const;
	int					getLineCount() const;
	DWORD				getMargins() const;
	bool				getModify() const;
	void				getRect(RECT& rect) const;
	DWORD				getSel() const;
	void				getSel(int& startChar, int& endChar) const;
	int					getThumb() const;
	TCHAR				getPasswordChar() const;
	EDITWORDBREAKPROC	getWordBreakProc() const;
	int					lineFromChar(int index = -1) const;
	int					lineIndex(int index = -1) const;
	int					lineLength(int line = -1) const;
	POINT				posFromChar(UINT charPos) const;
	void				setHandle(HLOCAL buffer);
	void				setLimitText(UINT maxLength);
	void				setMargins(UINT left, UINT right);
	void				setModify(bool modified = true);
	void				setPasswordChar(TCHAR ch);
	void				setReadOnly(bool readOnly = true);
	void				setRect(const RECT& rect);
	void				setRectNP(const RECT& rect);
	void				setTabStops();
	bool				setTabStops(uint cxEachStop);
	bool				setTabStops(int count, uint tabStops[]);
	void				setWordBreakProc(EDITWORDBREAKPROC proc);
#ifdef EM_GETIMESTATUS
	DWORD				getImeStatus(DWORD type) const;
	DWORD				setImeStatus(DWORD type, DWORD data);
#endif /* EM_GETIMESTATUS */
	// 操作
	void	emptyUndoBuffer();
	bool	fmtLines(bool addEol);
	void	limitText(int maxLength = 0);
	void	lineScroll(int lines, int chars = 0);
	void	replaceSel(const TCHAR* newText, bool canUndo = false);
	bool	scrollCaret();
	void	setSel(DWORD selection, bool noScroll = false);
	void	setSel(int startChar, int endChar, bool noScroll = false);
#ifdef EM_GETCUEBANNER
	bool	getCueBanner(WCHAR* text, int maxLength) const;
	bool	hideBalloonTip();
	bool	setCueBanner(const WCHAR* text);
	bool	showBalloonTip(const EDITBALLOONTIP& ebt);
#endif /* EM_GETCUEBANNER */
};

class ListBox : public StandardControl<ListBox> {
	DEFINE_CLASS_NAME(_T("LISTBOX"))
public:
	// コンストラクタ
	explicit ListBox(HWND handle = 0) : StandardControl<ListBox>(handle) {}
	// 作成
	int		initStorage(int itemCount, UINT bytes);
	// 属性
	int		getAnchorIndex() const;
	int		getCaretIndex() const;
	int		getCount() const;
	int		getCurSel() const;
	int		getHorizontalExtent() const;
	DWORD	getItemData(int index) const;
	void*	getItemDataPtr(int index) const;
	int		getItemRect(int index, RECT& rect) const;
	int		getItemHeight(int index) const;
	LCID	getLocale() const;
	int		getSel(int index) const;
	int		getSelCount() const;
	int		getSelItems(int maxItems, INT* indices) const;
	int		getText(int index, TCHAR* buffer) const;
	int		getTextLen(int index) const;
	int		getTopIndex() const;
	UINT	itemFromPoint(const POINT& pt, bool& outSide) const;
	void	setAnchorIndex(int index);
	int		setCaretIndex(int index, bool scroll = true);
	void	setColumnWidth(int width);
	int		setCurSel(int select);
	void	setHorizontalExtent(int extent);
	int		setItemData(int index, DWORD itemData);
	int		setItemDataPtr(int index, void* data);
	int		setItemHeight(int index, UINT itemHeight);
	int		selItemRange(int firstItem, int lastItem, bool select = true);
	LCID	setLocale(LCID newLocale);
	int		setSel(int index, bool select = true);
	void	setTabStops();
	bool	setTabStops(int cxEachStop);
	bool	setTabStops(int count, INT* tabStops);
	int		setTopIndex(int index);
	// 操作
	int		addString(const TCHAR* text);
	int		deleteString(UINT index);
	int		dir(UINT attr, const TCHAR* wildCard);
	int		findString(int startAfter, const TCHAR* text) const;
	int		findStringExact(int startAfter, const TCHAR* text) const;
	int		insertString(int index, const TCHAR* text);
	void	resetContent();
	int		selectString(int startAfter, const TCHAR* text);
};

class ScrollBar : public StandardControl<ScrollBar> {
	DEFINE_CLASS_NAME(_T("SCROLLBAR"))
public:
	// コンストラクタ
	explicit ScrollBar(HWND handle = 0) : StandardControl<ScrollBar>(handle) {}
	// 属性
	bool	getScrollInfo(SCROLLINFO& scrollInfo) const;
	int		getScrollLimit() const;
	int		getScrollPos() const;
	void	getScrollRange(int* minPos, int* maxPos) const;
	int		setScrollInfo(const SCROLLINFO& scrollInfo, bool redraw = true);
	int		setScrollPos(int pos, bool redraw = true);
	void	setScrollRange(int min, int max, bool redraw = true);
#if(WINVER >= 0x0500)
	bool	getScrollBarInfo(SCROLLBARINFO& scrollInfo) const;
#endif /* WINVER >= 0x0500 */
	// 操作
	bool	enableScrollBar(UINT arrowFlags = ESB_ENABLE_BOTH);
	void	showScrollBar(bool show = true);
};

class Static : public StandardControl<Static> {
	DEFINE_CLASS_NAME(_T("STATIC"))
public:
	// コンストラクタ
	explicit Static(HWND handle = 0) : StandardControl<Static>(handle) {}
	// メソッド
	HBITMAP			getBitmap() const;
	HCURSOR			getCursor() const;
	HENHMETAFILE	getEnhMetaFile() const;
	HICON			getIcon() const;
	HBITMAP			setBitmap(HBITMAP bitmap);
	HCURSOR			setCursor(HCURSOR cursor);
	HENHMETAFILE	setEnhMetaFile(HENHMETAFILE metaFile);
	HICON			setIcon(HICON icon);
};

class DragListBox : public ListBox {
public:
	// コンストラクタ
	explicit DragListBox(HWND handle) : ListBox(handle) {}
	// メソッド
	void		drawInsert(int index);
	static UINT	getDragListMessage();
	int			lbItemFromPtr(const POINT& pt, bool autoScroll = true);
	bool		makeDragList();
};

}}} // namespace manah::windows::controls

#include "standard-controls.inl"

#endif /* MANAH_STANDARD_CONTROLS_HPP */
