/**
 * @file window.hpp
 * @date 2002-2009
 */

#ifndef MANAH_WINDOW_HPP
#define MANAH_WINDOW_HPP

#include "menu.hpp"
#include "../dc.hpp"
#include "../../memory.hpp"	// manah.AutoBuffer
#include <shellapi.h>		// DragAcceptFiles
#include <ole2.h>			// D&D
#include <imm.h>
#include <string>

// these macros are defined by winuser.h
#ifndef GET_KEYSTATE_LPARAM
#	define GET_KEYSTATE_LPARAM(lp) (LOWORD(lp))
#endif // !GET_KEYSTATE_LPARAM
#ifndef GET_KEYSTATE_WPARAM
#	define GET_KEYSTATE_WPARAM(wp) (LOWORD(wp))
#endif // !GET_KEYSTATE_WPARAM
#ifndef GET_XBUTTON_WPARAM
#	define GET_XBUTTON_WPARAM(wp) (HIWORD(wp))
#endif // !GET_XBUTTON_WPARAM


// Window ///////////////////////////////////////////////////////////////////

namespace manah {
namespace win32 {
namespace ui {

		/// Makes a menu handle parameter from either a menu handle or numeric identifier.
		class MenuHandleOrControlID {
		public:
			/// Constructor takes a menu handle.
			MenuHandleOrControlID(HMENU handle) /*throw()*/ : handle_(handle) {}
			/// Constructor takes a numeric identifier.
			MenuHandleOrControlID(UINT_PTR id) /*throw()*/ : handle_(reinterpret_cast<HMENU>(id)) {}
			/// Returns the menu handle.
			HMENU get() const /*throw()*/ {return handle_;}
		private:
			HMENU handle_;
		};

		/// Makes a brush handle parameter from either a brush handle or @c COLORREF value. 
		class BrushHandleOrColor {
		public:
			/// Constructor makes @c null @c HBRUSH value.
			BrushHandleOrColor() /*throw()*/ : brush_(0) {}
			/// Constructor takes a brush handle.
			BrushHandleOrColor(HBRUSH handle) /*throw()*/ : brush_(handle) {}
			/// Constructor takes a @c COLORREF value used to make the brush handle.
			BrushHandleOrColor(COLORREF color) /*throw()*/ : brush_(reinterpret_cast<HBRUSH>(static_cast<HANDLE_PTR>(color + 1))) {}
			/// Returns the brush handle.
			HBRUSH get() const /*throw()*/ {return brush_;}
		private:
			HBRUSH brush_;
		};

		/// Makes a cursor handle parameter from either a cursor handle or numeric identifier.
		class CursorHandleOrID {
		public:
			/// Constructor makes @c null @c HCURSOR value.
			CursorHandleOrID() /*throw()*/ : cursor_(0) {}
			/// Constructor takes a cursor handle.
			CursorHandleOrID(HCURSOR handle) /*throw()*/ : cursor_(handle) {}
			/// Constructor takes a numeric identifier for system cursor.
			CursorHandleOrID(const WCHAR* systemCursorID) : cursor_(::LoadCursorW(0, systemCursorID)) {}
			/// Returns the cursor handle.
			HCURSOR get() const /*throw()*/ {return cursor_;}
		private:
			HCURSOR cursor_;
		};

		/// Makes a rectangle whose all member are @c CW_USEDEFAULT values.
		struct DefaultWindowRect : public tagRECT {
			DefaultWindowRect() {left = top = right = bottom = CW_USEDEFAULT;}
		};

class Window : public Handle<HWND, ::DestroyWindow> {
public:
	// constructors
	explicit Window(HWND handle = 0);
	virtual ~Window() throw();
	// constructions
	void close();
	bool create(const WCHAR* className, HWND parentOrHInstance,
		const RECT& rect = DefaultWindowRect(), const WCHAR* windowName = 0,
		DWORD style = 0UL, DWORD exStyle = 0UL, HMENU menu = 0, void* param = 0);
	bool destroy();
	// styles
	DWORD getExStyle() const;
	DWORD getStyle() const;
	bool modifyStyle(DWORD removeStyles, DWORD addStyles);
	bool modifyStyleEx(DWORD removeStyles, DWORD addStyles);
	// window class
	DWORD getClassLong(int index) const;
	int getClassName(WCHAR* className, int maxLength) const;
	LONG getWindowLong(int index) const;
	DWORD setClassLong(int index, DWORD newLong);
	LONG setWindowLong(int index, LONG newLong);
#ifdef _WIN64
	ULONG_PTR getClassLongPtr(int index) const;
	LONG_PTR getWindowLongPtr(int index) const;
	ULONG_PTR setClassLongPtr(int index, ULONG_PTR newLong);
	LONG_PTR setWindowLongPtr(int index, LONG_PTR newLong);
#endif // _WIN64
	// state
	bool enable(bool enable = true);
	static Borrowed<Window> getActive();
	static Borrowed<Window> getCapture();
	static Borrowed<Window> getDesktop();
	static Borrowed<Window> getFocus();
	static Borrowed<Window> getForeground();
	HICON getIcon(bool bigIcon = true) const;
	bool hasFocus() const;
	bool isWindow() const;
	bool isEnabled() const;
	bool isUnicode() const;
	static bool releaseCapture();
	Borrowed<Window> setActive();
	Borrowed<Window> setCapture();
	Borrowed<Window> setFocus();
	bool setForeground();
	HICON setIcon(HICON icon, bool bigIcon  = true);
	// size and position
	UINT arrangeIconicWindows();
	void bringToTop();
	void getClientRect(RECT& rect) const;
	bool getPlacement(WINDOWPLACEMENT& placement) const;
	void getRect(RECT& rect) const;
	int getRegion(HRGN region) const;
	bool isIconic() const;
	bool isZoomed() const;
	void move(int x, int y, int width, int height, bool repaint = true);
	void move(const RECT& rect, bool repaint = true);
	bool setPlacement(const WINDOWPLACEMENT& placement);
	bool setPosition(HWND windowInsertAfter, int x, int y, int cx, int cy, UINT flags);
	bool setPosition(HWND windowInsertAfter, const RECT& rect, UINT flags);
	int setRegion(const HRGN region, bool redraw = true);
	// window access
	void center(HWND alternateWindow = 0);
	Borrowed<Window> childFromPoint(const POINT& pt) const;
	Borrowed<Window> childFromPointEx(const POINT& pt, UINT flags) const;
	static Borrowed<Window> find(const WCHAR* className, const WCHAR* windowName);
	int getDlgCtrlID() const;
	Borrowed<Window> getLastActivePopup() const;
	Borrowed<Window> getNext(UINT flag = GW_HWNDNEXT) const;
	Borrowed<Window> getOwner() const;
	Borrowed<Window> getParent() const;
	Borrowed<Window> getTop() const;
	Borrowed<Window> getWindow(UINT command) const;
	bool isChild(HWND window) const;
	int setDlgCtrlID(int id);
	Borrowed<Window> setParent(HWND newParent);
	static Borrowed<Window> fromPoint(const POINT& pt);
	// update and paint
#if(WINVER >= 0x0500)
	bool animate(DWORD time, DWORD flags, bool catchError = true);
#endif
	gdi::PaintDC beginPaint(PAINTSTRUCT& paint);
	bool enableScrollBar(int barFlags, UINT arrowFlags = ESB_ENABLE_BOTH);
	void endPaint(const PAINTSTRUCT& paint);
	gdi::ClientDC getDC();
	gdi::ClientDC getDCEx(HRGN clipRegion, DWORD flags);
	bool getUpdateRect(RECT& rect, bool erase = false);
	int getUpdateRegion(HRGN region, bool erase = false);
	gdi::WindowDC getWindowDC();
	bool lockUpdate();
	void invalidate(bool erase = true);
	void invalidateRect(const RECT* rect, bool erase = true);
	void invalidateRegion(HRGN region, bool erase = true);
	bool isVisible() const;
	void print(HDC dc, DWORD flags) const;
	void printClient(HDC dc, DWORD flags) const;
	bool redraw(RECT* rectUpdate = 0, HRGN clipRegion = 0, UINT flags = RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
	int releaseDC(HDC dc);
	void setRedraw(bool redraw = true);
	void showOwnedPopups(bool show = true);
	bool show(UINT command);
	void unlockUpdate();
	void update();
	void validateRect(const RECT* rect);
	void validateRegion(HRGN region);
	// point mapping
	void clientToScreen(POINT& pt) const;
	void clientToScreen(RECT& rect) const;
	void mapWindowPoints(HWND destWindow, RECT& rect) const;
	void mapWindowPoints(HWND destWindow, POINT points[], UINT count) const;
	void screenToClient(POINT& pt) const;
	void screenToClient(RECT& rect) const;
	// window text
	int getText(WCHAR* text, int maxCount) const;
	std::wstring getText() const;
	int getTextLength() const;
	void setText(const WCHAR* text);
	// font
	HFONT getFont() const;
	void setFont(HFONT font, bool redraw = true);
	// properties
	int enumerateProperties(PROPENUMPROCW enumFunction);
	int enumeratePropertiesEx(PROPENUMPROCEXW enumFunction, LPARAM param);
	HANDLE getProperty(const WCHAR* identifier) const;
	HANDLE getProperty(ATOM identifier) const;
	HANDLE removeProperty(const WCHAR* identifier);
	HANDLE removeProperty(ATOM identifier);
	bool setProperty(const WCHAR* identifier, HANDLE data);
	bool setProperty(ATOM identifier, HANDLE data);
	// help
	DWORD getContextHelpID() const;
	bool setContextHelpID(DWORD contextHelpID);
	bool winHelp(const WCHAR* help, UINT command = HELP_CONTEXT, DWORD data = 0);
	// scroll
	bool getScrollInformation(int bar, SCROLLINFO& scrollInfo, UINT mask = SIF_ALL) const;
	int getScrollLimit(int bar) const;
	int getScrollPosition(int bar) const;
	void getScrollRange(int bar, int& minPos, int& maxPos) const;
	int getScrollTrackPosition(int bar) const;
	void scroll(int xAmount, int yAmount, RECT* rect = 0, RECT* clipRect = 0);
	int scrollEx(int dx, int dy, RECT* scrollRect, RECT* clipRect, HRGN updateRegion, RECT* updateRect, UINT flags);
	bool setScrollInformation(int bar, const SCROLLINFO& scrollInfo, bool bedraw = true);
	int setScrollPosition(int bar, int pos, bool redraw = true);
	void setScrollRange(int bar, int minPos, int maxPos, bool eedraw = true);
	void showScrollBar(int bar, bool show = true);
	// clipboard viewer
	bool changeClipboardChain(HWND newNext);
	Borrowed<Window> setClipboardViewer();
	// D&D
	void dragAcceptFiles(bool accept = true);
	HRESULT registerDragDrop(IDropTarget& dropTarget);
	HRESULT revokeDragDrop();
	// caret
	bool createCaret(HBITMAP bitmap, int width, int height);
	bool createSolidCaret(int width, int height);
	bool createGrayCaret(int width, int height);
	POINT getCaretPosition();
	void hideCaret();
	static void setCaretPosition(const POINT& pt);
	void showCaret();
	// cursor
	POINT getCursorPosition() const;
	bool setCursorPosition(const POINT& pt);
	// menu
	void drawMenuBar();
	Borrowed<Menu> getMenu() const;
	Borrowed<Menu> getSystemMenu(bool revert) const;
	bool hiliteMenuItem(HMENU menu, UINT item, UINT flags);
	bool setMenu(HMENU menu);
	// hotkey
	DWORD getHotKey() const;
	int setHotKey(WORD virtualKeyCode, WORD modifiers);
	// timer
	bool killTimer(UINT_PTR eventID);
	UINT_PTR setTimer(UINT_PTR eventID, UINT elapse,
		void (CALLBACK* procedure)(HWND, UINT, UINT_PTR, DWORD) = 0);
	// alert
	bool flash(bool invert);
	int messageBox(const WCHAR* text, const WCHAR* caption = 0, UINT type = MB_OK);
	// window message
	virtual LRESULT defWindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT sendMessage(UINT message, WPARAM wParam = 0, LPARAM lParam = 0L);
	bool sendNotifyMessage(UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT postMessage(UINT message, WPARAM wParam = 0, LPARAM lParam = 0L);
	// process and thread
	DWORD getProcessID() const;
	DWORD getThreadID() const;
	// layered window
#if(_WIN32_WINNT >= 0x0500)
	bool getLayeredAttributes(COLORREF* keyColor, BYTE* alpha, DWORD* flags) const;
	bool setLayeredAttributes(COLORREF keyColor, BYTE alpha, DWORD flags);
	bool updateLayered(HDC destDC, POINT* destPt, SIZE* size, HDC srcDC, POINT* srcPt,
		COLORREF keyColor, BLENDFUNCTION* blendFunction, DWORD flags);
#endif

protected:
	// Do not override this directly. Use MANAH_DECLEAR_WINDOW_MESSAGE_MAP familiy instead.
	virtual LRESULT processWindowMessage(UINT /* message */, WPARAM /* wParam */, LPARAM /* lParam */, bool& /* handled */) {return 1;}
	// Call the implementation of the base class if override this.
	virtual LRESULT preTranslateWindowMessage(UINT /* message */, WPARAM /* wParam */, LPARAM /* lParam */, bool& /* handled */) {return 1;}
	LRESULT fireProcessWindowMessage(UINT message, WPARAM wParam, LPARAM lParam) {
		bool handled = false;
		LRESULT result = processWindowMessage(message, wParam, lParam, handled);
		if(!handled)
			result = ::CallWindowProcW(::DefWindowProcW, get(), message, wParam, lParam);
		return result;
	}
protected:
	bool check() const /*throw()*/ {return isWindow();}
};

namespace internal {
	template<UINT message> struct Msg2Type {};
	template<typename WindowClass> struct MessageDispatcher {
#define DEFINE_DISPATCH(msg) static LRESULT dispatch(WindowClass& w, const Msg2Type<msg>, WPARAM wp, LPARAM lp, bool& handled)
		// WM_ACTIVATE -> void onActivate(UINT state, HWND otherWindow, bool minimized)
		DEFINE_DISPATCH(WM_ACTIVATE) {w.onActivate(LOWORD(wp), reinterpret_cast<HWND>(lp), toBoolean(HIWORD(wp))); return 1;}
		// WM_CAPTURECHANGED -> void onCapturChanged(HWND newWindow)
		DEFINE_DISPATCH(WM_CAPTURECHANGED) {w.onCaptureChanged(reinterpret_cast<HWND>(lp)); return 1;}
		// WM_CHAR -> void onChar(UINT code, UINT flags)
		DEFINE_DISPATCH(WM_CHAR) {w.onChar(static_cast<UINT>(wp), static_cast<UINT>(lp)); return 1;}
		// WM_CLOSE -> void onClose(void)
		DEFINE_DISPATCH(WM_CLOSE) {w.onClose(); return 1;}
		// WM_COMMAND -> bool onCommand(WORD id, WORD notifyCode, HWND control)
		DEFINE_DISPATCH(WM_COMMAND) {handled = w.onCommand(LOWORD(wp), HIWORD(wp), reinterpret_cast<HWND>(lp)); return 1;}
		// WM_CONTEXTMENU -> void onContextMenu(HWND window, const POINT& position)
		DEFINE_DISPATCH(WM_CONTEXTMENU) {const POINT p = {LOWORD(lp), HIWORD(lp)}; w.onContextMenu(reinterpret_cast<HWND>(wp), p); return 1;}
		// WM_CREATE -> LRESULT onCreate(const CREATESTRUCT&)
		DEFINE_DISPATCH(WM_CREATE) {return w.onCreate(*reinterpret_cast<const CREATESTRUCTW*>(lp));}
		// WM_DEADCHAR -> void onDeadChar(UINT code, UINT flags)
		DEFINE_DISPATCH(WM_DEADCHAR) {w.onDeadChar(static_cast<UINT>(wp), static_cast<UINT>(lp)); return 1;}
		// WM_DESTROY -> void onDestroy(void)
		DEFINE_DISPATCH(WM_DESTROY) {w.onDestroy(); return 1;}
		// WM_ENTERSIZEMOVE -> void onEnterSizeMove(void)
		DEFINE_DISPATCH(WM_ENTERSIZEMOVE) {w.onEnterSizeMove(); return 1;}
		// WM_ERASEBKGND -> bool onEraseBkgnd(HDC dc)
		DEFINE_DISPATCH(WM_ERASEBKGND) {return w.onEraseBkgnd(reinterpret_cast<HDC>(wp));}
		// WM_EXITSIZEMOVE -> void onExitSizeMove(void)
		DEFINE_DISPATCH(WM_EXITSIZEMOVE) {w.onExitSizeMove(); return 1;}
		// WM_FONTCHANGE -> void onFontChange(void)
		DEFINE_DISPATCH(WM_FONTCHANGE) {w.onFontChange(); return 1;}
		// WM_GETDLGCODE -> UINT onGetDlgCode(void)
		DEFINE_DISPATCH(WM_GETDLGCODE) {handled = true; return w.onGetDlgCode();}
		// WM_GETFONT -> HFONT onGetFont(void)
		DEFINE_DISPATCH(WM_GETFONT) {handled = true; return reinterpret_cast<LRESULT>(w.onGetFont());}
		// WM_GETMINMAXINFO -> void onGetMinMaxInfo(MINMAXINFO&)
		DEFINE_DISPATCH(WM_GETMINMAXINFO) {handled = true; w.onGetMinMaxInfo(*reinterpret_cast<MINMAXINFO*>(lp)); return 0;}
		// WM_GETTEXTLENGTH -> int onGetText(int maximumLength, TCHAR* text)
		DEFINE_DISPATCH(WM_GETTEXT) {handled = true; return w.onGetText(static_cast<int>(wp), reinterpret_cast<WCHAR*>(lp));}
		// WM_GETTEXTLENGTH -> int onGetTextLength(void)
		DEFINE_DISPATCH(WM_GETTEXTLENGTH) {handled = true; return w.onGetTextLength();}
		// WM_HSCROLL -> void onHScroll(UINT sbCode, UINT position, HWND scrollBar)
		DEFINE_DISPATCH(WM_HSCROLL) {w.onHScroll(LOWORD(wp), HIWORD(wp), reinterpret_cast<HWND>(lp)); return 1;}
		// WM_IME_COMPOSITION -> void onIMEComposition(WPARAM wParam, LPARAM lParam, bool& handled)
		DEFINE_DISPATCH(WM_IME_COMPOSITION) {w.onIMEComposition(wp, lp, handled); return 0;}
		// WM_IME_ENDCOMPOSITION -> void onIMEEndComposition(void)
		DEFINE_DISPATCH(WM_IME_ENDCOMPOSITION) {w.onIMEEndComposition(); return 0;}
		// WM_IME_NOTIFY -> LRESULT onIMENotify(WPARAM command, LPARAM lParam, bool& handled)
		DEFINE_DISPATCH(WM_IME_NOTIFY) {return w.onIMENotify(wp, lp, handled);}
		// WM_IME_REQUEST -> LRESULT onIMERequest(WPARAM command, LPARAM lParam, bool& handled)
		DEFINE_DISPATCH(WM_IME_REQUEST) {return w.onIMERequest(wp, lp, handled);}
		// WM_IME_STARTCOMPOSITION -> void onIMEStartComposition(void)
		DEFINE_DISPATCH(WM_IME_STARTCOMPOSITION) {w.onIMEStartComposition(); return 0;}
		// WM_KEYDOWN -> void onKeyDown(UINT vkey, UINT flags, bool& handled)
		DEFINE_DISPATCH(WM_KEYDOWN) {w.onKeyDown(static_cast<UINT>(wp), static_cast<UINT>(lp), handled); return !handled;}
		// WM_KEYUP -> void onKeyUp(UINT vkey, UINT flags, bool& handled)
		DEFINE_DISPATCH(WM_KEYUP) {w.onKeyUp(static_cast<UINT>(wp), static_cast<UINT>(lp), handled); return !handled;}
		// WM_KILLFOCUS -> void onKillFocus(HWND newWindow)
		DEFINE_DISPATCH(WM_KILLFOCUS) {w.onKillFocus(reinterpret_cast<HWND>(wp)); return 1;}
		// WM_LBUTTONDBLCLK -> void onLButtonDblClk(UINT flags, const POINT& position)
		DEFINE_DISPATCH(WM_LBUTTONDBLCLK) {const POINT p = {LOWORD(lp), HIWORD(lp)}; w.onLButtonDblClk(static_cast<UINT>(wp), p); return 1;}
		// WM_LBUTTONDOWN -> void onLButtonDblClk(UINT flags, const POINT& position)
		DEFINE_DISPATCH(WM_LBUTTONDOWN) {const POINT p = {LOWORD(lp), HIWORD(lp)}; w.onLButtonDown(static_cast<UINT>(wp), p); return 1;}
		// WM_LBUTTONUP -> void onLButtonUp(UINT flags, const POINT& position)
		DEFINE_DISPATCH(WM_LBUTTONUP) {const POINT p = {LOWORD(lp), HIWORD(lp)}; w.onLButtonUp(static_cast<UINT>(wp), p); return 1;}
		// WM_MBUTTONDBLCLK -> void onMButtonDblClk(UINT flags, const POINT& position)
		DEFINE_DISPATCH(WM_MBUTTONDBLCLK) {const POINT p = {LOWORD(lp), HIWORD(lp)}; w.onMButtonDblClk(static_cast<UINT>(wp), p); return 1;}
		// WM_MBUTTONDOWN -> void onMButtonDblClk(UINT flags, const POINT& position)
		DEFINE_DISPATCH(WM_MBUTTONDOWN) {const POINT p = {LOWORD(lp), HIWORD(lp)}; w.onMButtonDown(static_cast<UINT>(wp), p); return 1;}
		// WM_MBUTTONUP -> void onMButtonUp(UINT flags, const POINT& position)
		DEFINE_DISPATCH(WM_MBUTTONUP) {const POINT p = {LOWORD(lp), HIWORD(lp)}; w.onMButtonUp(static_cast<UINT>(wp), p); return 1;}
		// WM_MOUSEACTIVATE -> int onMouseActivate(HWND desktop, UINT hitTest, UINT message)
		DEFINE_DISPATCH(WM_MOUSEACTIVATE) {return w.onMouseActivate(reinterpret_cast<HWND>(wp), LOWORD(lp), HIWORD(lp));}
		// WM_MOUSEMOVE -> void onMouseMove(UINT flags, const POINT& position)
		DEFINE_DISPATCH(WM_MOUSEMOVE) {const POINT p = {LOWORD(lp), HIWORD(lp)}; w.onMouseMove(static_cast<UINT>(wp), p); return 1;}
#ifdef WM_MOUSEWHEEL
		// WM_MOUSEWHEEL -> void onMouseWheel(UINT flags, short delta, const POINT& position)
		DEFINE_DISPATCH(WM_MOUSEWHEEL) {const POINT p = {LOWORD(lp), HIWORD(lp)}; w.onMouseWheel(GET_KEYSTATE_WPARAM(wp), GET_WHEEL_DELTA_WPARAM(wp), p); return 1;}
#endif // WM_MOUSEWHEEL
		// WM_MOVE -> void onMove(int x, int y)
		DEFINE_DISPATCH(WM_MOVE) {w.onMove(LOWORD(lp), HIWORD(lp)); return 1;}
		// WM_MOVING -> void onMoving(const RECT& rect)
		DEFINE_DISPATCH(WM_MOVING) {w.onMoving(*reinterpret_cast<const RECT*>(lp)); return 1;}
		// WM_NCCREATE -> bool onNcCreate(CREATESTRUCTW&)
		DEFINE_DISPATCH(WM_NCCREATE) {handled = true; return w.onNcCreate(*reinterpret_cast<CREATESTRUCTW*>(lp));}
		// WM_NOTIFY -> bool onNotify(int id, NMHDR& nmhdr)
		DEFINE_DISPATCH(WM_NOTIFY) {handled = w.onNotify(static_cast<int>(wp), *reinterpret_cast<NMHDR*>(lp)); return 1;}
//		// WM_PAINT -> bool onPaint(void)
//		DEFINE_DISPATCH(WM_PAINT) {return w.onPaint();}
		// WM_RBUTTONDBLCLK -> void onRButtonDblClk(UINT flags, const POINT& position)
		DEFINE_DISPATCH(WM_RBUTTONDBLCLK) {const POINT p = {LOWORD(lp), HIWORD(lp)}; w.onRButtonDblClk(static_cast<UINT>(wp), p); return 1;}
		// WM_RBUTTONDOWN -> void onRButtonDblClk(UINT flags, const POINT& position)
		DEFINE_DISPATCH(WM_RBUTTONDOWN) {const POINT p = {LOWORD(lp), HIWORD(lp)}; w.onRButtonDown(static_cast<UINT>(wp), p); return 1;}
		// WM_RBUTTONUP -> void onRButtonUp(UINT flags, const POINT& position)
		DEFINE_DISPATCH(WM_RBUTTONUP) {const POINT p = {LOWORD(lp), HIWORD(lp)}; w.onRButtonUp(static_cast<UINT>(wp), p); return 1;}
		// WM_SETCURSOR -> bool onSetCursor(HWND window, UINT hitTest, UINT message)
		DEFINE_DISPATCH(WM_SETCURSOR) {return handled = w.onSetCursor(reinterpret_cast<HWND>(wp), LOWORD(lp), HIWORD(lp));}
		// WM_SETFOCUS -> void onSetFocus(HWND oldWindow)
		DEFINE_DISPATCH(WM_SETFOCUS) {w.onSetFocus(reinterpret_cast<HWND>(wp)); handled = true; return 0;}
		// WM_SETFONT -> void onSetFont(HFONT font, bool redraw)
		DEFINE_DISPATCH(WM_SETFONT) {handled = true; w.onSetFont(reinterpret_cast<HFONT>(wp), toBoolean(LOWORD(lp))); return 0;}
		// WM_SETTEXT -> bool onSetText(const WCHAR* text)
		DEFINE_DISPATCH(WM_SETTEXT) {handled = w.onSetText(reinterpret_cast<const WCHAR*>(lp)); return 0;}
		// WM_SETTINGCHANGE -> void onSettingChange(UINT flags, const WCHAR* sectionName)
		DEFINE_DISPATCH(WM_SETTINGCHANGE) {w.onSettingChange(static_cast<UINT>(wp), reinterpret_cast<const WCHAR*>(lp)); return 1;}
		// WM_SHOWWINDOW -> void onShowWindow(bool showing, UINT status)
		DEFINE_DISPATCH(WM_SHOWWINDOW) {w.onShowWindow(toBoolean(wp), static_cast<UINT>(lp)); return 1;}
		// WM_SIZE -> void onSize(UINT type, int cx, int cy)
		DEFINE_DISPATCH(WM_SIZE) {w.onSize(static_cast<UINT>(wp), LOWORD(lp), HIWORD(lp)); return 1;}
		// WM_SIZING -> void onSizing(UINT side, const RECT& rect)
		DEFINE_DISPATCH(WM_SIZING) {w.onSizing(static_cast<UINT>(wp), *reinterpret_cast<RECT*>(lp)); return 1;}
		// WM_STYLECHANGED -> void onStyleChanged(int type, const STYLESTRUCT& style)
		DEFINE_DISPATCH(WM_STYLECHANGED) {w.onStyleChanged(static_cast<int>(wp), *reinterpret_cast<const STYLESTRUCT*>(lp)); return 1;}
		// WM_STYLECHANGING -> void onStyleChanging(int type, STYLESTRUCT& style)
		DEFINE_DISPATCH(WM_STYLECHANGING) {w.onStyleChanging(static_cast<int>(wp), *reinterpret_cast<STYLESTRUCT*>(lp)); return 1;}
		// WM_SYSCHAR -> void onSysChar(UINT code, UINT flags)
		DEFINE_DISPATCH(WM_SYSCHAR) {w.onSysChar(static_cast<UINT>(wp), static_cast<UINT>(lp)); return 1;}
		// WM_SYSCOLORCHANGE -> void onSysColorChange(void)
		DEFINE_DISPATCH(WM_SYSCOLORCHANGE) {w.onSysColorChange(); return 1;}
		// WM_SYSDEADCHAR -> void onSysDeadChar(UINT code, UINT flags)
		DEFINE_DISPATCH(WM_SYSDEADCHAR) {w.onSysDeadChar(static_cast<UINT>(wp), static_cast<UINT>(lp)); return 1;}
		// WM_SYSKEYDOWN -> bool onSysKeyDown(UINT vkey, UINT flags)
		DEFINE_DISPATCH(WM_SYSKEYDOWN) {return w.onSysKeyDown(static_cast<UINT>(wp), static_cast<UINT>(lp));}
		// WM_SYSKEYUP -> bool onSysKeyUp(UINT vkey, UINT flags)
		DEFINE_DISPATCH(WM_SYSKEYUP) {return w.onSysKeyUp(static_cast<UINT>(wp), static_cast<UINT>(lp));}
#ifdef WM_THEMECHANGED
		// WM_THEMECHANGED -> void onThemeChanged(void)
		DEFINE_DISPATCH(WM_THEMECHANGED) {w.onThemeChanged(); return 1;}
#endif // WM_THEMECHANGED
		// WM_TIMER -> void onTimer(UINT_PTR eventID, TIMERPROC)
		DEFINE_DISPATCH(WM_TIMER) {w.onTimer(static_cast<UINT_PTR>(wp), reinterpret_cast<TIMERPROC>(lp)); return 1;}
#ifdef WM_UNICHAR
		// WM_UNICHAR -> void onUniChar(UINT code, UINT flags)
		DEFINE_DISPATCH(WM_UNICHAR) {w.onUniChar(static_cast<UINT>(wp), static_cast<UINT>(lp)); return 1;}
#endif // WM_UNICHAR
		// WM_VSCROLL -> void onVScroll(UINT sbCode, UINT position, HWND scrollBar)
		DEFINE_DISPATCH(WM_VSCROLL) {w.onVScroll(LOWORD(wp), HIWORD(wp), reinterpret_cast<HWND>(lp)); return 1;}
#ifdef WM_XBUTTONDBLCLK
		// WM_XBUTTONDBLCLK -> bool onXButtonDblClk(WORD xButton, WORD keyState, const POINT& position)
		DEFINE_DISPATCH(WM_XBUTTONDBLCLK) {const POINT p = {LOWORD(lp), HIWORD(lp)}; return w.onXButtonDblClk(GET_XBUTTON_WPARAM(wp), GET_KEYSTATE_WPARAM(wp), p);}
		// WM_XBUTTONDOWN -> bool onXButtonDblClk(WORD xButton, WORD keyState, const POINT& position)
		DEFINE_DISPATCH(WM_XBUTTONDOWN) {const POINT p = {LOWORD(lp), HIWORD(lp)}; return w.onXButtonDown(GET_XBUTTON_WPARAM(wp), GET_KEYSTATE_WPARAM(wp), p);}
		// WM_XBUTTONUP -> bool onXButtonUp(WORD xButton, WORD keyState, const POINT& position)
		DEFINE_DISPATCH(WM_XBUTTONUP) {const POINT p = {LOWORD(lp), HIWORD(lp)}; return w.onXButtonUp(GET_XBUTTON_WPARAM(wp), GET_KEYSTATE_WPARAM(wp), p);}
#endif // WM_XBUTTONDBLCLK
#undef DEFINE_DISPATCH
	};
}

#define MANAH_DECLEAR_WINDOW_MESSAGE_MAP(WindowClass)							\
protected:																		\
	virtual LRESULT processWindowMessage(UINT message, WPARAM, LPARAM, bool&);	\
	friend struct manah::win32::ui::internal::MessageDispatcher<WindowClass>

#define MANAH_BEGIN_WINDOW_MESSAGE_MAP(WindowClass, BaseClass)												\
	LRESULT WindowClass::processWindowMessage(UINT message, WPARAM wParam, LPARAM lParam, bool& handled) {	\
		typedef WindowClass Klass; typedef BaseClass BaseKlass; LRESULT result;								\
		switch(message) {

#define MANAH_WINDOW_MESSAGE_ENTRY(msg)														\
		case msg: result = manah::win32::ui::internal::MessageDispatcher<Klass>::dispatch(	\
			*this, manah::win32::ui::internal::Msg2Type<msg>(), wParam, lParam, handled);	\
			if(handled) return result; break;

#define MANAH_END_WINDOW_MESSAGE_MAP()												\
		}																			\
		return BaseKlass::processWindowMessage(message, wParam, lParam, handled);	\
	}


class SubclassableWindow : public Window {
	MANAH_NONCOPYABLE_TAG(SubclassableWindow);
public:
	explicit SubclassableWindow(HWND handle = 0) : Window(handle), originalProcedure_(0) {}
	virtual ~SubclassableWindow() {if(isWindow()) unsubclass();}
public:
	virtual LRESULT	defWindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	bool isSubclassed() const {return originalProcedure_ != 0;}
	bool subclass();
	bool unsubclass();
protected:
	virtual LRESULT processWindowMessage(UINT message, WPARAM wParam, LPARAM lParam, bool& handled);
	static LRESULT CALLBACK windowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
private:
	using Window::fireProcessWindowMessage;
	WNDPROC originalProcedure_;
};


// Window styling policies from ATL::CWinTraits and ATL::CWinTraitsOR
template<DWORD style_, DWORD exStyle_ = 0> struct DefaultWindowStyles {
	static DWORD getStyle(DWORD style) {return (style != 0) ? style : style_;}
	static DWORD getExStyle(DWORD exStyle) {return (exStyle != 0) ? exStyle : exStyle_;}
};

typedef DefaultWindowStyles<0>	NullWindowStyle;

template<DWORD style_, DWORD exStyle_ = 0, class RhsStyles_ = NullWindowStyle> struct AdditiveWindowStyles {
	static DWORD getStyle(DWORD style) {return style | style_ | RhsStyles_::getStyle(style);}
	static DWORD getExStyle(DWORD exStyle) {return exStyle | exStyle_ | RhsStyles_::getExStyle(exStyle);}
};

typedef DefaultWindowStyles<WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE>	DefaultControlStyles;


// Control must be define getClassName, static method returns const TCHAR* and takes no parameters,
// using this macro.
#define DEFINE_CLASS_NAME(name)	public: static const WCHAR* getClassName() {return name;}
template<class Control, class DefaultStyles = DefaultControlStyles>
class StandardControl : public SubclassableWindow {
public:
	explicit StandardControl(HWND handle = 0) : SubclassableWindow(handle) {}
	virtual ~StandardControl() {}
public:
	virtual bool create(HWND parent, const RECT& rect = DefaultWindowRect(),
			const WCHAR* windowName = 0, INT_PTR id = 0, DWORD style = 0, DWORD exStyle = 0) {
		return Window::create(Control::getClassName(), parent, rect, windowName,
			DefaultStyles::getStyle(style), DefaultStyles::getExStyle(exStyle), reinterpret_cast<HMENU>(id));
	}
	enum {DefaultStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS};
protected:
	// implementation helpers
	template<typename ReturnType>
	ReturnType sendMessageR(UINT message, WPARAM wParam = 0, LPARAM lParam = 0) {
#ifdef _MSC_VER
#	pragma warning(disable : 4800)	// non-boolean to boolean conversion
#endif // _MSC_VER
		return static_cast<ReturnType>(sendMessage(message, wParam, lParam));
#ifdef _MSC_VER
#	pragma warning(default : 4800)
#endif // _MSC_VER
	}
	template<typename ReturnType>
	ReturnType sendMessageC(UINT message, WPARAM wParam = 0, LPARAM lParam = 0) const {
		return const_cast<StandardControl*>(this)->sendMessageR<ReturnType>(message, wParam, lParam);
	}
};

// Control must define one static method:
// void getClass(GET_CLASS_PARAM_LIST)
#define GET_CLASS_PARAM_LIST	const WCHAR*& name,										\
	HINSTANCE& instance, UINT& style, manah::win32::ui::BrushHandleOrColor& bgColor,	\
	manah::win32::ui::CursorHandleOrID& cursor,	HICON& icon, HICON& smallIcon, int& clsExtraBytes, int& wndExtraBytes
#define DEFINE_WINDOW_CLASS()	public: static void getClass(GET_CLASS_PARAM_LIST)
template<class Control>
class CustomControl : public Window {
	MANAH_UNASSIGNABLE_TAG(CustomControl);
public:
	explicit CustomControl(HWND handle = 0) : Window(handle) {}
	virtual ~CustomControl();
	bool create(HWND parent, const RECT& rect = DefaultWindowRect(),
		const WCHAR* windowName = 0, DWORD style = 0UL, DWORD exStyle = 0UL);
protected:
	CustomControl(const CustomControl<Control>& rhs) : Window() {}
	static LRESULT CALLBACK	windowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
	virtual void onPaint(gdi::PaintDC& dc) = 0;	// WM_PAINT
private:
	using Window::fireProcessWindowMessage;
};


// Window ///////////////////////////////////////////////////////////////////

inline Window::Window(HWND handle /* = 0 */) : Handle<HWND, ::DestroyWindow>(handle) {}

inline Window::~Window() throw() {}

#if(WINVER >= 0x0500)
inline bool Window::animate(DWORD time, DWORD flags, bool catchError /* = true */) {
	if(toBoolean(::AnimateWindow(use(), time, flags)))
		return true;
	else if(catchError) {
		if(toBoolean(flags & AW_HIDE))
			return show(SW_HIDE);
		else
			return show(toBoolean(flags & AW_ACTIVATE) ? SW_SHOW : SW_SHOWNA);
	}
	return false;
}
#endif

inline UINT Window::arrangeIconicWindows() {return ::ArrangeIconicWindows(use());}

inline gdi::PaintDC Window::beginPaint(PAINTSTRUCT& paint) {::BeginPaint(use(), &paint); return gdi::PaintDC(get(), paint);}

inline void Window::bringToTop() {::BringWindowToTop(use());}

inline void Window::center(HWND alternateWindow /* = 0 */) {
	RECT winRect, altRect;
	if(alternateWindow == 0) {
		alternateWindow = getParent()->get();
		if(alternateWindow == 0)
			alternateWindow = ::GetDesktopWindow();
	}
	assert(toBoolean(::IsWindow(alternateWindow)));

	getRect(winRect);
	::GetWindowRect(alternateWindow, &altRect);
	setPosition(0, (altRect.right - altRect.left) / 2 - (winRect.right - winRect.left) / 2,
		(altRect.bottom - altRect.top) / 2 - (winRect.bottom - winRect.top) / 2, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
}

inline bool Window::changeClipboardChain(HWND newNext) {return toBoolean(::ChangeClipboardChain(use(), newNext));}

inline Borrowed<Window> Window::childFromPoint(const POINT& pt) const {return Borrowed<Window>(::ChildWindowFromPoint(use(), pt));}

inline Borrowed<Window> Window::childFromPointEx(const POINT& pt, UINT flags) const {return Borrowed<Window>(::ChildWindowFromPointEx(use(), pt, flags));}

inline void Window::clientToScreen(RECT& rect) const {
	POINT point[2];
	point[0].x = rect.left;
	point[0].y = rect.top;
	point[1].x = rect.right;
	point[1].y = rect.bottom;
	::ClientToScreen(use(), &point[0]);
	::ClientToScreen(get(), &point[1]);
	::SetRect(&rect, point[0].x, point[0].y, point[1].x, point[1].y);
}

inline void Window::clientToScreen(POINT& pt) const {::ClientToScreen(use(), &pt);}

inline void Window::close() {::CloseWindow(use());}

inline bool Window::create(const WCHAR* className, HWND parentOrHInstance,
		const RECT& rect /* = DefaultWindowRect() */, const WCHAR* windowName /* = 0 */,
		DWORD style /* = 0UL */, DWORD exStyle /* = 0UL */, HMENU menu /* = 0 */, void* param /* = 0 */) {
	if(isWindow())
		return false;

	HWND parent = toBoolean(::IsWindow(parentOrHInstance)) ? parentOrHInstance : 0;
	HINSTANCE instance = (parent != 0) ?
#ifdef _WIN64
					reinterpret_cast<HINSTANCE>(::GetWindowLongPtrW(parent, GWLP_HINSTANCE))
#else
					reinterpret_cast<HINSTANCE>(static_cast<HANDLE_PTR>(::GetWindowLongW(parent, GWL_HINSTANCE)))
#endif // _WIN64
					: reinterpret_cast<HINSTANCE>(parentOrHInstance);

	if(HWND handle = ::CreateWindowExW(exStyle, className, windowName, style,
			rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, parent, menu, instance, param)) {
		reset(handle);
		return true;
	}
	return false;
}

inline bool Window::createCaret(HBITMAP bitmap, int width, int height) {return toBoolean(::CreateCaret(use(), bitmap, width, height));}

inline bool Window::createGrayCaret(int width, int height) {return createCaret(reinterpret_cast<HBITMAP>(1), width, height);}

inline bool Window::createSolidCaret(int width, int height) {return createCaret(0, width, height);}

inline LRESULT Window::defWindowProc(UINT message, WPARAM wParam, LPARAM lParam) {return ::DefWindowProcW(use(), message, wParam, lParam);}

inline bool Window::destroy() {if(toBoolean(::DestroyWindow(get()))) {release(); return true;} return false;}

inline void Window::dragAcceptFiles(bool accept /* = true */) {::DragAcceptFiles(use(), accept);}

inline void Window::drawMenuBar() {::DrawMenuBar(use());}

inline bool Window::enableScrollBar(int barFlags, UINT arrowFlags /* = ESB_ENABLE_BOTH */) {
	return toBoolean(::EnableScrollBar(use(), barFlags, arrowFlags));}

inline bool Window::enable(bool enable /* = true */) {return toBoolean(::EnableWindow(use(), enable));}

inline void Window::endPaint(const PAINTSTRUCT& paint) {::EndPaint(use(), &paint);}

inline int Window::enumerateProperties(PROPENUMPROCW enumFunction) {return ::EnumPropsW(use(), enumFunction);}

inline int Window::enumeratePropertiesEx(PROPENUMPROCEXW enumFunction, LPARAM param) {return ::EnumPropsExW(use(), enumFunction, param);}

inline Borrowed<Window> Window::find(const WCHAR* className, const WCHAR* windowName) {return Borrowed<Window>(::FindWindowW(className, windowName));}

inline bool Window::flash(bool invert) {return toBoolean(::FlashWindow(use(), invert));}

inline Borrowed<Window> Window::getActive() {return Borrowed<Window>(::GetActiveWindow());}

inline Borrowed<Window> Window::getCapture() {return Borrowed<Window>(::GetCapture());}

inline POINT Window::getCaretPosition() {POINT pt; ::GetCaretPos(&pt); return pt;}

inline DWORD Window::getClassLong(int index) const {return ::GetClassLongW(use(), index);}

#ifdef _WIN64
inline ULONG_PTR Window::getClassLongPtr(int index) const {return ::GetClassLongPtrW(use(), index);}
#endif // _WIN64

inline int Window::getClassName(WCHAR* className, int maxLength) const {return ::GetClassNameW(use(), className, maxLength);}

inline void Window::getClientRect(RECT& rect) const {::GetClientRect(use(), &rect);}

inline POINT Window::getCursorPosition() const {POINT pt; ::GetCursorPos(&pt); screenToClient(pt); return pt;}

inline gdi::ClientDC Window::getDC() {return gdi::ClientDC(use());}

inline gdi::ClientDC Window::getDCEx(HRGN clipRegion, DWORD flags) {return gdi::ClientDC(use(), clipRegion, flags);}

inline Borrowed<Window> Window::getDesktop() {return Borrowed<Window>(::GetDesktopWindow());}

inline int Window::getDlgCtrlID() const {return ::GetDlgCtrlID(use());}

inline DWORD Window::getExStyle() const {return static_cast<DWORD>(getWindowLong(GWL_EXSTYLE));}

inline Borrowed<Window> Window::getFocus() {return Borrowed<Window>(::GetFocus());}

inline HFONT Window::getFont() const {return reinterpret_cast<HFONT>(::SendMessageW(use(), WM_GETFONT, 0, 0L));}

inline Borrowed<Window> Window::getForeground() {return Borrowed<Window>(::GetForegroundWindow());}

inline DWORD Window::getHotKey() const {return static_cast<DWORD>(::SendMessageW(use(), WM_GETHOTKEY, 0, 0L));}

inline HICON Window::getIcon(bool bigIcon /* = true */) const {
	return reinterpret_cast<HICON>(::SendMessageW(use(), WM_GETICON, bigIcon ? ICON_BIG : ICON_SMALL, 0L));}

inline Borrowed<Window> Window::getLastActivePopup() const {return Borrowed<Window>(::GetLastActivePopup(use()));}

#if(_WIN32_WINNT >= 0x0501)
inline bool Window::getLayeredAttributes(COLORREF* keyColor, BYTE* alpha, DWORD* flags) const {
	return toBoolean(::GetLayeredWindowAttributes(use(), keyColor, alpha, flags));}
#endif

inline Borrowed<Menu> Window::getMenu() const {return Borrowed<Menu>(::GetMenu(use()));}

inline Borrowed<Window> Window::getNext(UINT flag /* = GW_HWNDNEXT */) const {return Borrowed<Window>(::GetNextWindow(use(), flag));}

inline Borrowed<Window> Window::getOwner() const {return Borrowed<Window>(::GetWindow(use(), GW_OWNER));}

inline Borrowed<Window> Window::getParent() const {return Borrowed<Window>(::GetParent(use()));}

inline HANDLE Window::getProperty(const WCHAR* identifier) const {return ::GetPropW(use(), identifier);}

inline HANDLE Window::getProperty(ATOM identifier) const {return getProperty(reinterpret_cast<const WCHAR*>(identifier));}

inline bool Window::getScrollInformation(int bar, SCROLLINFO& scrollInfo, UINT mask /* = SIF_ALL */) const {
	scrollInfo.cbSize = sizeof(SCROLLINFO);
	scrollInfo.fMask = mask;
	return toBoolean(::GetScrollInfo(use(), bar, &scrollInfo));
}

inline int Window::getScrollLimit(int bar) const {
	int dummy, limit;
	getScrollRange(bar, dummy, limit);
	return limit;
}

inline int Window::getScrollPosition(int bar) const {return ::GetScrollPos(use(), bar);}

inline void Window::getScrollRange(int bar, int& minPos, int& maxPos) const {::GetScrollRange(use(), bar, &minPos, &maxPos);}

inline int Window::getScrollTrackPosition(int bar) const {SCROLLINFO si; return (getScrollInformation(bar, si, SIF_TRACKPOS)) ? si.nTrackPos : -1;}

inline DWORD Window::getStyle() const {return static_cast<DWORD>(getWindowLong(GWL_STYLE));}

inline Borrowed<Menu> Window::getSystemMenu(bool revert) const {return Borrowed<Menu>(::GetSystemMenu(use(), revert));}

inline Borrowed<Window> Window::getTop() const {return Borrowed<Window>(::GetTopWindow(use()));}

inline bool Window::getUpdateRect(RECT& rect, bool erase /* = false */) {return toBoolean(::GetUpdateRect(use(), &rect, erase));}

inline int Window::getUpdateRegion(HRGN region, bool erase /* = false */) {return ::GetUpdateRgn(use(), region, erase);}

inline Borrowed<Window> Window::getWindow(UINT command) const {return Borrowed<Window>(::GetWindow(use(), command));}

inline DWORD Window::getContextHelpID() const {return ::GetWindowContextHelpId(use());}

inline gdi::WindowDC Window::getWindowDC() {return gdi::WindowDC(use());}

inline LONG Window::getWindowLong(int index) const {return ::GetWindowLongW(use(), index);}

#ifdef _WIN64
inline LONG_PTR Window::getWindowLongPtr(int index) const {return ::GetWindowLongPtrW(use(), index);}
#endif // _WIN64

inline bool Window::getPlacement(WINDOWPLACEMENT& placement) const {return toBoolean(::GetWindowPlacement(use(), &placement));}

inline DWORD Window::getProcessID() const {DWORD id; ::GetWindowThreadProcessId(use(), &id); return id;}

inline void Window::getRect(RECT& rect) const {::GetWindowRect(use(), &rect);}

inline int Window::getRegion(HRGN region) const {return ::GetWindowRgn(use(), region);}

inline int Window::getText(WCHAR* text, int maxLength) const {return ::GetWindowTextW(use(), text, maxLength);}

inline std::wstring Window::getText() const {
	const int len = getTextLength();
	AutoBuffer<WCHAR> buffer(new WCHAR[len + 1]);
	getText(buffer.get(), len + 1);
	return std::wstring(buffer.get());
}

inline int Window::getTextLength() const {return ::GetWindowTextLengthW(use());}

inline DWORD Window::getThreadID() const {return ::GetWindowThreadProcessId(use(), 0);}

inline bool Window::hasFocus() const {return ::GetFocus() == use();}

inline void Window::hideCaret() {::HideCaret(use());}

inline bool Window::hiliteMenuItem(HMENU menu, UINT item, UINT flags) {return toBoolean(::HiliteMenuItem(use(), menu, item, flags));}

inline void Window::invalidate(bool erase /* = true */) {::InvalidateRect(use(), 0, erase);}

inline void Window::invalidateRect(const RECT* rect, bool erase /* = true */) {::InvalidateRect(use(), rect, erase);}

inline void Window::invalidateRegion(HRGN region, bool erase /* = true */) {::InvalidateRgn(use(), region, erase);}

inline bool Window::isChild(HWND window) const {return toBoolean(::IsChild(use(), window));}

inline bool Window::isIconic() const {return toBoolean(::IsIconic(use()));}

inline bool Window::isWindow() const /*throw()*/ {return toBoolean(::IsWindow(get()));}

inline bool Window::isEnabled() const {return toBoolean(::IsWindowEnabled(use()));}

inline bool Window::isUnicode() const {return toBoolean(::IsWindowUnicode(use()));}

inline bool Window::isVisible() const {return toBoolean(::IsWindowVisible(use()));}

inline bool Window::isZoomed() const {return toBoolean(::IsZoomed(use()));}

inline bool Window::killTimer(UINT eventID) {return toBoolean(::KillTimer(use(), eventID));}

inline bool Window::lockUpdate() {return toBoolean(::LockWindowUpdate(use()));}

inline void Window::mapWindowPoints(HWND destWindow, RECT& rect) const {
	POINT point[2];
	point[0].x = rect.left;
	point[0].y = rect.top;
	point[1].x = rect.right;
	point[1].y = rect.bottom;
	::MapWindowPoints(use(), destWindow, point, 2);
	::SetRect(&rect, point[0].x, point[0].y, point[1].x, point[1].y);
}

inline void Window::mapWindowPoints(HWND destWindow, POINT points[], UINT count) const {::MapWindowPoints(use(), destWindow, points, count);}

inline int Window::messageBox(const WCHAR* text, const WCHAR* caption /* = 0 */, UINT type /* = MB_OK */) {
	return ::MessageBoxW(use(), text, caption, type);}

inline bool Window::modifyStyle(DWORD removeStyles, DWORD addStyles) {
	DWORD style;
	style = getStyle();
	style &= ~removeStyles;
	style |= addStyles;
	if(setWindowLong(GWL_STYLE, style) == 0)
		return false;
	return true;
}

inline bool Window::modifyStyleEx(DWORD removeStyles, DWORD addStyles) {
	DWORD exStyle = getExStyle();
	exStyle &= ~removeStyles;
	exStyle |= addStyles;
	if(setWindowLong(GWL_EXSTYLE, exStyle) == 0)
		return false;
	return true;
}

inline void Window::move(int x, int y, int width, int height, bool repaint /* = true */) {
	::MoveWindow(use(), x, y, width, height, repaint);}

inline void Window::move(const RECT& rect, bool repaint /* = true */) {
	::MoveWindow(use(), rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, repaint);}

inline void Window::print(HDC dc, DWORD flags) const {::SendMessageW(use(), WM_PRINT, reinterpret_cast<WPARAM>(dc), flags);}

inline void Window::printClient(HDC dc, DWORD flags) const {::SendMessageW(use(), WM_PRINTCLIENT, reinterpret_cast<WPARAM>(dc), flags);}

inline LRESULT Window::postMessage(UINT message, WPARAM wParam /* = 0 */, LPARAM lParam /* = 0L */) {
	return ::PostMessageW(use(), message, wParam, lParam);}

inline bool Window::redraw(RECT* updateRect /* = 0 */, HRGN clipRegion/* = 0 */,
		UINT flags /* = RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE */) {
	return toBoolean(::RedrawWindow(use(), updateRect, clipRegion, flags));}

inline HRESULT Window::registerDragDrop(IDropTarget& dropTarget) {return ::RegisterDragDrop(use(), &dropTarget);}

inline bool Window::releaseCapture() {return toBoolean(::ReleaseCapture());}

inline int Window::releaseDC(HDC dc) {return ::ReleaseDC(use(), dc);}

inline HANDLE Window::removeProperty(const WCHAR* identifier) {return ::RemovePropW(use(), identifier);}

inline HANDLE Window::removeProperty(ATOM identifier) {return removeProperty(reinterpret_cast<const WCHAR*>(identifier));}

inline HRESULT Window::revokeDragDrop() {return ::RevokeDragDrop(use());}

inline void Window::screenToClient(RECT& rect) const {
	POINT point[2];
	point[0].x = rect.left;
	point[0].y = rect.top;
	point[1].x = rect.right;
	point[1].y = rect.bottom;
	::ScreenToClient(use(), &point[0]);
	::ScreenToClient(get(), &point[1]);
	::SetRect(&rect, point[0].x, point[0].y, point[1].x, point[1].y);
}

inline void Window::scroll(int xAmount, int yAmount, RECT* rect /* = 0 */, LPRECT clipRect /* = 0 */) {
	::ScrollWindow(use(), xAmount, yAmount, rect, clipRect);}

inline int Window::scrollEx(
		int dx, int dy, RECT* scrollRect, RECT* clipRect, HRGN updateRegion, RECT* updateRect, UINT flags) {
	return ::ScrollWindowEx(use(), dx, dy, scrollRect, clipRect, updateRegion, updateRect, flags);}

inline LRESULT Window::sendMessage(UINT message, WPARAM wParam /* = 0 */, LPARAM lParam /* = 0L */) {
	return ::SendMessageW(use(), message, wParam, lParam);}

inline bool Window::sendNotifyMessage(UINT message, WPARAM wParam, LPARAM lParam) {
	return toBoolean(::SendNotifyMessageW(use(), message, wParam, lParam));}

inline void Window::screenToClient(POINT& pt) const {::ScreenToClient(use(), &pt);}

inline Borrowed<Window> Window::setActive() {return Borrowed<Window>(::SetActiveWindow(use()));}

inline Borrowed<Window> Window::setCapture() {return Borrowed<Window>(::SetCapture(use()));}

inline void Window::setCaretPosition(const POINT& pt) {::SetCaretPos(pt.x, pt.y);}

inline DWORD Window::setClassLong(int index, DWORD newLong) {return ::SetClassLongW(use(), index, newLong);}

#ifdef _WIN64
inline ULONG_PTR Window::setClassLongPtr(int index, ULONG_PTR newLong) {return ::SetClassLongPtrW(use(), index, newLong);}
#endif // _WIN64

inline Borrowed<Window> Window::setClipboardViewer() {return Borrowed<Window>(::SetClipboardViewer(use()));}

inline bool Window::setCursorPosition(const POINT& pt) {POINT p(pt); clientToScreen(p); return toBoolean(::SetCursorPos(p.x, p.y));}

inline int Window::setDlgCtrlID(int id) {return static_cast<int>(setWindowLong(GWL_ID, id));}

inline Borrowed<Window> Window::setFocus() {return Borrowed<Window>(::SetFocus(use()));}

inline void Window::setFont(HFONT font, bool redraw /* = true */) {
	sendMessage(WM_SETFONT, reinterpret_cast<WPARAM>(font), MAKELPARAM(redraw, 0));}

inline bool Window::setForeground() {return toBoolean(::SetForegroundWindow(use()));}

inline int Window::setHotKey(WORD virtualKeyCode, WORD modifiers) {
	return static_cast<int>(sendMessage(WM_SETHOTKEY, MAKEWPARAM(virtualKeyCode, modifiers)));}

inline HICON Window::setIcon(HICON icon, bool bigIcon /* = true */) {
	return reinterpret_cast<HICON>(sendMessage(WM_SETICON, bigIcon ? ICON_BIG : ICON_SMALL, reinterpret_cast<LPARAM>(icon)));}

#if(_WIN32_WINNT >= 0x0500)
inline bool Window::setLayeredAttributes(COLORREF keyColor, BYTE alpha, DWORD flags) {
	return toBoolean(::SetLayeredWindowAttributes(use(), keyColor, alpha, flags));}
#endif

inline bool Window::setMenu(HMENU menu) {return toBoolean(::SetMenu(use(), menu));}

inline Borrowed<Window> Window::setParent(HWND newParent) {return Borrowed<Window>(::SetParent(use(), newParent));}

inline bool Window::setProperty(const WCHAR* identifier, HANDLE data) {return toBoolean(::SetPropW(use(), identifier, data));}

inline bool Window::setProperty(ATOM identifier, HANDLE data) {return setProperty(reinterpret_cast<const WCHAR*>(identifier), data);}

inline void Window::setRedraw(bool redraw /* = true */) {sendMessage(WM_SETREDRAW, redraw);}

inline bool Window::setScrollInformation(int bar, const SCROLLINFO& scrollInfo, bool redraw /* = true */) {
	return toBoolean(::SetScrollInfo(use(), bar, &scrollInfo, redraw));}

inline int Window::setScrollPosition(int bar, int pos, bool redraw /* = true */) {
	return ::SetScrollPos(use(), bar, pos, redraw);}

inline void Window::setScrollRange(int bar, int minPos, int maxPos, bool redraw /* = true */) {
	::SetScrollRange(use(), bar, minPos, maxPos, redraw);}

inline UINT_PTR Window::setTimer(
		UINT_PTR eventID, UINT elapse, void (CALLBACK* procedure)(HWND, UINT, UINT_PTR, DWORD) /* = 0 */) {
	return static_cast<UINT_PTR>(::SetTimer(use(), eventID, elapse, procedure));}

inline bool Window::setContextHelpID(DWORD contextHelpID) {return toBoolean(::SetWindowContextHelpId(use(), contextHelpID));}

inline LONG Window::setWindowLong(int index, LONG newLong) {return ::SetWindowLongW(use(), index, newLong);}

#ifdef _WIN64
inline LONG_PTR Window::setWindowLongPtr(int index, LONG_PTR newLong) {return ::SetWindowLongPtrW(use(), index, newLong);}
#endif // _WIN64

inline bool Window::setPlacement(const WINDOWPLACEMENT& placement) {return toBoolean(::SetWindowPlacement(use(), &placement));}

inline bool Window::setPosition(HWND windowInsertAfter, int x, int y, int cx, int cy, UINT flags) {
	return toBoolean(::SetWindowPos(use(), windowInsertAfter, x, y, cx, cy, flags));}

inline bool Window::setPosition(HWND windowInsertAfter, const RECT& rect, UINT flags) {
	return setPosition(windowInsertAfter, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, flags);}

inline int Window::setRegion(const HRGN region, bool redraw /* = true */) {return ::SetWindowRgn(use(), region, redraw);}

inline void Window::setText(const WCHAR* text) {::SetWindowTextW(use(), text);}

inline void Window::showCaret() {::ShowCaret(use());}

inline void Window::showOwnedPopups(bool show /* = true */) {::ShowOwnedPopups(use(), show);}

inline void Window::showScrollBar(int bar, bool show /* = true */) {::ShowScrollBar(use(), bar, show);}

inline bool Window::show(UINT command) {return toBoolean(::ShowWindow(use(), command));}

inline void Window::unlockUpdate() {::LockWindowUpdate(0);}

inline void Window::update() {::UpdateWindow(use());}

#if(_WIN32_WINNT >= 0x0500)
inline bool Window::updateLayered(HDC destDC, POINT* destPt,
		SIZE* size, HDC srcDC, POINT* srcPt, COLORREF keyColor, BLENDFUNCTION* blendFunction, DWORD flags) {
	return toBoolean(::UpdateLayeredWindow(use(), destDC, destPt, size, srcDC, srcPt, keyColor, blendFunction, flags));}
#endif

inline void Window::validateRect(const RECT* rect) {::ValidateRect(use(), rect);}

inline void Window::validateRegion(HRGN region) {::ValidateRgn(use(), region);}

inline Borrowed<Window> Window::fromPoint(const POINT& pt) {return Borrowed<Window>(::WindowFromPoint(pt));}

inline bool Window::winHelp(const WCHAR* help, UINT command /* = HELP_CONTEXT */, DWORD data /* = 0 */) {
	return toBoolean(::WinHelpW(use(), help, command, data));}

#undef RETURN_ATTACHED


// SubclassableWindow ///////////////////////////////////////////////////////

inline LRESULT SubclassableWindow::defWindowProc(UINT message, WPARAM wParam, LPARAM lParam) {
	return (originalProcedure_ != 0) ?
		::CallWindowProcW(originalProcedure_, get(), message, wParam, lParam)
		: ::DefWindowProcW(get(), message, wParam, lParam);
}

inline LRESULT SubclassableWindow::processWindowMessage(UINT message, WPARAM wParam, LPARAM lParam, bool& handled) {
	if(message == WM_NCDESTROY)
		unsubclass();
	handled = true;
	return ::CallWindowProcW(originalProcedure_, get(), message, wParam, lParam);
}

inline bool SubclassableWindow::subclass() {
	if(isSubclassed())
		return false;
#ifdef _WIN64
	else if(originalProcedure_ = reinterpret_cast<WNDPROC>(getWindowLongPtr(GWLP_WNDPROC))) {
		setWindowLongPtr(GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(windowProcedure));
		setWindowLongPtr(GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
#else
	else if(originalProcedure_ = reinterpret_cast<WNDPROC>(static_cast<LONG_PTR>(getWindowLong(GWLP_WNDPROC)))) {
		setWindowLong(GWLP_WNDPROC, static_cast<long>(reinterpret_cast<LONG_PTR>(windowProcedure)));
		setWindowLong(GWLP_USERDATA, static_cast<long>(reinterpret_cast<LONG_PTR>(this)));
#endif // _WIN64
		return true;
	} else
		return false;
}

inline bool SubclassableWindow::unsubclass() {
	if(!isSubclassed())
		return false;
#ifdef _WIN64
	setWindowLongPtr(GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(originalProcedure_));
	setWindowLongPtr(GWLP_USERDATA, 0);
#else
	setWindowLong(GWL_WNDPROC, static_cast<long>(reinterpret_cast<LONG_PTR>(originalProcedure_)));
	setWindowLong(GWL_USERDATA, 0);
#endif // _WIN64
	originalProcedure_ = 0;
	return true;
}

inline LRESULT CALLBACK SubclassableWindow::windowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
	if(SubclassableWindow* instance = reinterpret_cast<SubclassableWindow*>(
#ifdef _WIN64
			::GetWindowLongPtrW(window, GWLP_USERDATA
#else
			static_cast<LONG_PTR>(::GetWindowLongW(window, GWL_USERDATA)
#endif // _WIN64
			)))
		return instance->fireProcessWindowMessage(message, wParam, lParam);
	return false;
}


// CustomControl ////////////////////////////////////////////////////////////

template<class Control> inline CustomControl<Control>::~CustomControl() {
	// prevent to be called as this by windowProcedure
	if(isWindow())
		::SetWindowLongPtrW(get(), GWLP_USERDATA, 0);
}

template<class Control>
inline bool CustomControl<Control>::create(HWND parent, const RECT& rect /* = DefaultWindowRect() */,
		const WCHAR* windowName /* = 0 */, DWORD style /* = 0UL */, DWORD exStyle /* = 0UL */) {
	BrushHandleOrColor bgColor;
	CursorHandleOrID cursor;
	AutoZeroSize<WNDCLASSEXW> wc;
	AutoZeroSize<WNDCLASSEXW> dummy;

	wc.hInstance = ::GetModuleHandleW(0);	// default value
	wc.lpfnWndProc = CustomControl<Control>::windowProcedure;
	Control::getClass(wc.lpszClassName, wc.hInstance, wc.style,
		bgColor, cursor, wc.hIcon, wc.hIconSm, wc.cbClsExtra, wc.cbWndExtra);
	wc.hbrBackground = bgColor.get();
	wc.hCursor = cursor.get();
	if(::GetClassInfoExW(wc.hInstance, wc.lpszClassName, &dummy) == 0)
		::RegisterClassExW(&wc);
	return Window::create(wc.lpszClassName, parent, rect, windowName, style, exStyle, 0, this);
}

template<class Control>
inline LRESULT CALLBACK CustomControl<Control>::windowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
	typedef CustomControl<Control> C;
	if(message == WM_NCCREATE) {
		C* const p = reinterpret_cast<C*>(reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams);
		assert(p != 0);
		p->reset(window);	// ... the handle will be reset by Window.create (no problem)
#ifdef _WIN64
		p->setWindowLongPtr(GWLP_USERDATA, reinterpret_cast<LONG_PTR>(p));
#else
		p->setWindowLong(GWL_USERDATA, static_cast<long>(reinterpret_cast<LONG_PTR>(p)));
#endif // _WIN64

		return p->fireProcessWindowMessage(message, wParam, lParam);
	} else {
		C* const p = reinterpret_cast<C*>(
#ifdef _WIN64
			::GetWindowLongPtrW(window, GWLP_USERDATA));
#else
			static_cast<LONG_PTR>(::GetWindowLongW(window, GWL_USERDATA)));
#endif // _WIN64
		if(p == 0)
			return 1;
		bool handled = false;
		const LRESULT r = p->preTranslateWindowMessage(message, wParam, lParam, handled);
		if(handled)
			return r;
		else if(message == WM_PAINT) {
			p->onPaint(gdi::PaintDC(p->get()));
			return 0;
		}
		return p->fireProcessWindowMessage(message, wParam, lParam);
	}
}

}}} // namespace manah.win32.ui

#endif // !MANAH_WINDOW_HPP
