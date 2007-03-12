// window.hpp
// (c) 2002-2007

#ifndef MANAH_WINDOW_HPP
#define MANAH_WINDOW_HPP

#include "menu.hpp"
#include "../dc.hpp"
#include "../../memory.hpp"	// manah.AutoBuffer
#include <shellapi.h>		// DragAcceptFiles
#include <ole2.h>			// D&D
#include <string>

#ifndef WS_EX_LAYERED
#define WS_EX_LAYERED	0x00080000
#define LWA_COLORKEY	0x00000001
#define LWA_ALPHA		0x00000002
/*typedef struct _BLENDFUNCTION {
	BYTE	BlendOp;
	BYTE	BlendFlags;
	BYTE	SourceConstantAlpha;
	BYTE	AlphaFormat;
} BLENDFUNCTION, *PBLENDFUNCTION;*/
#endif /* !WS_EX_LAYERED */

#ifndef AW_SLIDE
#define AW_HOR_POSITIVE	0x00000001
#define AW_HOR_NEGATIVE	0x00000002
#define AW_VER_POSITIVE	0x00000004
#define AW_VER_NEGATIVE	0x00000008
#define AW_CENTER		0x00000010
#define AW_HIDE			0x00010000
#define AW_ACTIVATE		0x00020000
#define AW_SLIDE		0x00040000
#define AW_BLEND		0x00080000
#endif /* !AW_SLIDE */

/*
namespace {
#if(_MSC_VER < 1300)
	template<class T, class U> struct SameTypes {
		template<class V> struct X {enum {result = false};};
		template<> struct X<U> {enum {result = true};};
		enum {result = X<T>::result};
	};
#else
	template<class T1, class T2> struct SameTypes {enum {result = false};};
	template<class T> struct SameTypes<T, T> {enum {result = true};};
#endif // _MSC_VER < 1300

	template<bool> struct TimerIDTypeSelector {typedef UINT type;};
	template<> struct TimerIDTypeSelector<true> {typedef UINT_PTR type;};
} // namespace `anonymous'

// WIN32 API Timer ID type
typedef TimerIDTypeSelector<
	SameTypes<TIMERPROC, void(*)(HWND, UINT, UINT_PTR, DWORD)>::result
>::type timerid_t;
*/

// Window class definition //////////////////////////////////////////////////

namespace manah {
namespace win32 {
namespace ui {

struct DefaultWindowRect : public ::tagRECT {
	DefaultWindowRect() {left = top = right = bottom = CW_USEDEFAULT;}
};

template<class Window> class Subclassable;

class Window : public Handle<HWND, ::DestroyWindow> {
public:
	// constructors
	explicit Window(HWND handle = 0);
	Window(const Window& rhs);
	virtual ~Window();
	// operator
	bool operator==(const Window& rhs) const {return get() == rhs.get();}
	// constructions
	void	close();
	bool	create(const TCHAR* className, HWND parentOrHInstance,
				const ::RECT& rect = DefaultWindowRect(), const TCHAR* windowName = 0,
				DWORD style = 0UL, DWORD exStyle = 0UL, HMENU menu = 0, void* param = 0);
	bool	destroy();
	// styles
	DWORD	getExStyle() const;
	DWORD	getStyle() const;
	bool	modifyStyle(DWORD removeStyles, DWORD addStyles);
	bool	modifyStyleEx(DWORD removeStyles, DWORD addStyles);
	// window class
	DWORD		getClassLong(int index) const;
	int			getClassName(TCHAR* className, int maxLength) const;
	LONG		getWindowLong(int index) const;
	DWORD		setClassLong(int index, DWORD newLong);
	LONG		setWindowLong(int index, LONG newLong);
#ifdef _WIN64
	ULONG_PTR	getClassLongPtr(int index) const;
	LONG_PTR	getWindowLongPtr(int index) const;
	ULONG_PTR	setClassLongPtr(int index, ULONG_PTR newLong);
	LONG_PTR	setWindowLongPtr(int index, LONG_PTR newLong);
#endif /* _WIN64 */
	// state
	bool							enable(bool enable = true);
	static std::auto_ptr<Window>	getActive();
	static std::auto_ptr<Window>	getCapture();
	static std::auto_ptr<Window>	getDesktop();
	static std::auto_ptr<Window>	getFocus();
	static std::auto_ptr<Window>	getForeground();
	HICON							getIcon(bool bigIcon = true) const;
	bool							hasFocus() const;
	bool							isWindow() const;
	bool							isEnabled() const;
	bool							isUnicode() const;
	static bool						releaseCapture();
	std::auto_ptr<Window>			setActive();
	std::auto_ptr<Window>			setCapture();
	std::auto_ptr<Window>			setFocus();
	bool							setForeground();
	HICON							setIcon(HICON icon, bool bigIcon  = true);
	// size and position
	UINT	arrangeIconicWindows();
	void	bringToTop();
	void	getClientRect(::RECT& rect) const;
	bool	getPlacement(::WINDOWPLACEMENT& placement) const;
	void	getRect(::RECT& rect) const;
	int		getRegion(HRGN region) const;
	bool	isIconic() const;
	bool	isZoomed() const;
	void	move(int x, int y, int width, int height, bool repaint = true);
	void	move(const ::RECT& rect, bool repaint = true);
	bool	setPlacement(const ::WINDOWPLACEMENT& placement);
	bool	setPosition(HWND windowInsertAfter, int x, int y, int cx, int cy, UINT flags);
	bool	setPosition(HWND windowInsertAfter, const ::RECT& rect, UINT flags);
	int		setRegion(const HRGN region, bool redraw = true);
	// window access
	void							center(HWND alternateWindow = 0);
	std::auto_ptr<Window>			childFromPoint(const ::POINT& pt) const;
	std::auto_ptr<Window>			childFromPointEx(const ::POINT& pt, UINT flags) const;
	static std::auto_ptr<Window>	find(const TCHAR* className, const TCHAR* windowName);
	int								getDlgCtrlID() const;
	std::auto_ptr<Window>			getLastActivePopup() const;
	std::auto_ptr<Window>			getNext(UINT flag = GW_HWNDNEXT) const;
	std::auto_ptr<Window>			getOwner() const;
	std::auto_ptr<Window>			getParent() const;
	std::auto_ptr<Window>			getTop() const;
	std::auto_ptr<Window>			getWindow(UINT command) const;
	bool							isChild(HWND window) const;
	int								setDlgCtrlID(int id);
	std::auto_ptr<Window>			setParent(HWND newParent);
	static std::auto_ptr<Window>	fromPoint(const ::POINT& pt);
	// update and paint
	gdi::PaintDC	beginPaint(::PAINTSTRUCT& paint);
	bool			enableScrollBar(int barFlags, UINT arrowFlags = ESB_ENABLE_BOTH);
	void			endPaint(const ::PAINTSTRUCT& paint);
	gdi::ClientDC	getDC();
	gdi::ClientDC	getDCEx(HRGN clipRegion, DWORD flags);
	bool			getUpdateRect(::RECT& rect, bool erase = false);
	int				getUpdateRegion(HRGN region, bool erase = false);
	gdi::WindowDC	getWindowDC();
	bool			lockUpdate();
	void			invalidate(bool erase = true);
	void			invalidateRect(const ::RECT* rect, bool erase = true);
	void			invalidateRegion(HRGN region, bool erase = true);
	bool			isVisible() const;
	void			print(HDC dc, DWORD flags) const;
	void			printClient(HDC dc, DWORD flags) const;
	bool			redraw(::RECT* rectUpdate = 0, HRGN clipRegion = 0, UINT flags = RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
	int				releaseDC(HDC dc);
	void			setRedraw(bool redraw = true);
	void			showOwnedPopups(bool show = true);
	bool			show(UINT command);
	void			unlockUpdate();
	void			update();
	void			validateRect(const ::RECT* rect);
	void			validateRegion(HRGN region);
	// point mapping
	void	clientToScreen(::POINT& pt) const;
	void	clientToScreen(::RECT& rect) const;
	void	mapWindowPoints(HWND destWindow, ::RECT& rect) const;
	void	mapWindowPoints(HWND destWindow, ::POINT points[], UINT count) const;
	void	screenToClient(::POINT& pt) const;
	void	screenToClient(::RECT& rect) const;
	// window text
	int							getText(TCHAR* text, int maxCount) const;
	std::basic_string<TCHAR>	getText() const;
	int							getTextLength() const;
	void						setText(const TCHAR* text);
	// font
	HFONT	getFont() const;
	void	setFont(HFONT font, bool redraw = true);
	// properties
	int		enumerateProperties(::PROPENUMPROC enumFunction);
	int		enumeratePropertiesEx(::PROPENUMPROCEX enumFunction, LPARAM param);
	HANDLE	getProperty(const TCHAR* identifier) const;
	HANDLE	getProperty(::ATOM identifier) const;
	HANDLE	removeProperty(const TCHAR* identifier);
	HANDLE	removeProperty(::ATOM identifier);
	bool	setProperty(const TCHAR* identifier, HANDLE data);
	bool	setProperty(::ATOM identifier, HANDLE data);
	// help
	DWORD	getContextHelpID() const;
	bool	setContextHelpID(DWORD contextHelpID);
	bool	winHelp(const TCHAR* help, UINT command = HELP_CONTEXT, DWORD data = 0);
	// scroll
	bool	getScrollInformation(int bar, ::SCROLLINFO& scrollInfo, UINT mask = SIF_ALL) const;
	int		getScrollLimit(int bar) const;
	int		getScrollPosition(int bar) const;
	void	getScrollRange(int bar, int& minPos, int& maxPos) const;
	int		getScrollTrackPosition(int bar) const;
	void	scroll(int xAmount, int yAmount, ::RECT* rect = 0, ::RECT* clipRect = 0);
	int		scrollEx(int dx, int dy, ::RECT* scrollRect, ::RECT* clipRect, HRGN updateRegion, ::RECT* updateRect, UINT flags);
	bool	setScrollInformation(int bar, const ::SCROLLINFO& scrollInfo, bool bedraw = true);
	int		setScrollPosition(int bar, int pos, bool redraw = true);
	void	setScrollRange(int bar, int minPos, int maxPos, bool eedraw = true);
	void	showScrollBar(int bar, bool show = true);
	// clipboard viewer
	bool					changeClipboardChain(HWND newNext);
	std::auto_ptr<Window>	setClipboardViewer();
	// D&D
	void	dragAcceptFiles(bool accept = true);
	HRESULT	registerDragDrop(::IDropTarget& dropTarget);
	HRESULT	revokeDragDrop();
	// caret
	bool		createCaret(HBITMAP bitmap, int width, int height);
	bool		createSolidCaret(int width, int height);
	bool		createGrayCaret(int width, int height);
	::POINT		getCaretPosition();
	void		hideCaret();
	static void	setCaretPosition(const ::POINT& pt);
	void		showCaret();
	// cursor
	::POINT	getCursorPosition() const;
	bool	setCursorPosition(const POINT& pt);
	// menu
	void				drawMenuBar();
	std::auto_ptr<Menu>	getMenu() const;
	std::auto_ptr<Menu>	getSystemMenu(bool revert) const;
	bool				hiliteMenuItem(HMENU menu, UINT item, UINT flags);
	bool				setMenu(HMENU menu);
	// hotkey
	DWORD	getHotKey() const;
	int		setHotKey(WORD virtualKeyCode, WORD modifiers);
	// timer
	bool		killTimer(UINT_PTR eventID);
	UINT_PTR	setTimer(UINT_PTR eventID, UINT elapse,
					void (CALLBACK* procedure)(HWND, UINT, UINT_PTR, DWORD) = 0);
	// alert
	bool	flash(bool invert);
	int		messageBox(const TCHAR* text, const TCHAR* caption = 0, UINT type = MB_OK);
	// window message
	virtual LRESULT	defWindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT			sendMessage(UINT message, WPARAM wParam = 0, LPARAM lParam = 0L);
	bool			sendNotifyMessage(UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT			postMessage(UINT message, WPARAM wParam = 0, LPARAM lParam = 0L);
	// process and thread
	DWORD	getProcessID() const;
	DWORD	getThreadID() const;

protected:
	// Do not override this directly. Use MANAH_DECLEAR_WINDOW_MESSAGE_MAP familiy instead.
	virtual bool processWindowMessage(UINT message, WPARAM wParam, LPARAM lParam, LRESULT& result) {return false;}
	// Call the implementation of the base class if override this.
	virtual LRESULT preTranslateWindowMessage(UINT message, WPARAM wParam, LPARAM lParam, bool& handled) {return 1;}
	LRESULT fireProcessWindowMessage(UINT message, WPARAM wParam, LPARAM lParam) {
		LRESULT result = 1;
		if(!processWindowMessage(message, wParam, lParam, result))
			result = ::CallWindowProc(::DefWindowProc, get(), message, wParam, lParam);
		return result;
	}

	// �f�o�b�O
protected:
#ifdef _DEBUG
#	define assertValidAsWindow() assert(isWindow())
#else
#	define assertValidAsWindow()
#endif /* _DEBUG */

private:
	static std::auto_ptr<Window> createAttached(HWND handle) {std::auto_ptr<Window> p(new Window); p->attach(handle); return p;}
	friend class Subclassable;
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
		// WM_CONTEXTMENU -> void onContextMenu(HWND window, const ::POINT& position)
		DEFINE_DISPATCH(WM_CONTEXTMENU) {const ::POINT p = {LOWORD(lp), HIWORD(lp)}; w.onContextMenu(reinterpret_cast<HWND>(wp), p); return 1;}
		// WM_CREATE -> LRESULT onCreate(const ::CREATESTRUCT&)
		DEFINE_DISPATCH(WM_CREATE) {return w.onCreate(*reinterpret_cast<const ::CREATESTRUCT*>(lp));}
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
		// WM_GETMINMAXINFO -> void onGetMinMaxInfo(::MINMAXINFO&)
		DEFINE_DISPATCH(WM_GETMINMAXINFO) {handled = true; w.onGetMinMaxInfo(*reinterpret_cast<::MINMAXINFO*>(lp)); return 0;}
		// WM_GETTEXTLENGTH -> int onGetText(int maximumLength, TCHAR* text)
		DEFINE_DISPATCH(WM_GETTEXT) {handled = true; return w.onGetText(static_cast<int>(wp), reinterpret_cast<TCHAR*>(lp));}
		// WM_GETTEXTLENGTH -> int onGetTextLength(void)
		DEFINE_DISPATCH(WM_GETTEXTLENGTH) {handled = true; return w.onGetTextLength();}
		// WM_HSCROLL -> void onHScroll(UINT sbCode, UINT position, HWND scrollBar)
		DEFINE_DISPATCH(WM_HSCROLL) {w.onHScroll(LOWORD(wp), HIWORD(wp), reinterpret_cast<HWND>(lp)); return 1;}
		// WM_IME_COMPOSITION -> void onIMEComposition(WPARAM wParam, LPARAM lParam)
		DEFINE_DISPATCH(WM_IME_COMPOSITION) {w.onIMEComposition(wp, lp); return 0;}
		// WM_IME_ENDCOMPOSITION -> void onIMEEndComposition(void)
		DEFINE_DISPATCH(WM_IME_ENDCOMPOSITION) {w.onIMEEndComposition(); return 0;}
		// WM_IME_NOTIFY -> LRESULT onIMENotify(WPARAM command, LPARAM lParam, bool& handled)
		DEFINE_DISPATCH(WM_IME_NOTIFY) {return w.onIMENotify(wp, lp, handled);}
		// WM_IME_REQUEST -> LRESULT onIMERequest(WPARAM command, LPARAM lParam, bool& handled)
		DEFINE_DISPATCH(WM_IME_REQUEST) {return w.onIMERequest(wp, lp, handled);}
		// WM_IME_STARTCOMPOSITION -> void onIMEStartComposition(void)
		DEFINE_DISPATCH(WM_IME_STARTCOMPOSITION) {handled = true; w.onIMEStartComposition(); return 0;}
		// WM_KEYDOWN -> void onKeyDown(UINT vkey, UINT flags, bool& handled)
		DEFINE_DISPATCH(WM_KEYDOWN) {w.onKeyDown(static_cast<UINT>(wp), static_cast<UINT>(lp), handled); return 1;}
		// WM_KEYUP -> void onKeyUp(UINT vkey, UINT flags, bool& handled)
		DEFINE_DISPATCH(WM_KEYUP) {w.onKeyUp(static_cast<UINT>(wp), static_cast<UINT>(lp), handled); return 1;}
		// WM_KILLFOCUS -> void onKillFocus(HWND newWindow)
		DEFINE_DISPATCH(WM_KILLFOCUS) {w.onKillFocus(reinterpret_cast<HWND>(wp)); return 1;}
		// WM_LBUTTONDBLCLK -> void onLButtonDblClk(UINT flags, const ::POINT& position)
		DEFINE_DISPATCH(WM_LBUTTONDBLCLK) {const ::POINT p = {LOWORD(lp), HIWORD(lp)}; w.onLButtonDblClk(static_cast<UINT>(wp), p); return 1;}
		// WM_LBUTTONDOWN -> void onLButtonDblClk(UINT flags, const ::POINT& position)
		DEFINE_DISPATCH(WM_LBUTTONDOWN) {const ::POINT p = {LOWORD(lp), HIWORD(lp)}; w.onLButtonDown(static_cast<UINT>(wp), p); return 1;}
		// WM_LBUTTONUP -> void onLButtonUp(UINT flags, const ::POINT& position)
		DEFINE_DISPATCH(WM_LBUTTONUP) {const ::POINT p = {LOWORD(lp), HIWORD(lp)}; w.onLButtonUp(static_cast<UINT>(wp), p); return 1;}
		// WM_MBUTTONDBLCLK -> void onMButtonDblClk(UINT flags, const ::POINT& position)
		DEFINE_DISPATCH(WM_MBUTTONDBLCLK) {const ::POINT p = {LOWORD(lp), HIWORD(lp)}; w.onMButtonDblClk(static_cast<UINT>(wp), p); return 1;}
		// WM_MBUTTONDOWN -> void onMButtonDblClk(UINT flags, const ::POINT& position)
		DEFINE_DISPATCH(WM_MBUTTONDOWN) {const ::POINT p = {LOWORD(lp), HIWORD(lp)}; w.onMButtonDown(static_cast<UINT>(wp), p); return 1;}
		// WM_MBUTTONUP -> void onMButtonUp(UINT flags, const ::POINT& position)
		DEFINE_DISPATCH(WM_MBUTTONUP) {const ::POINT p = {LOWORD(lp), HIWORD(lp)}; w.onMButtonUp(static_cast<UINT>(wp), p); return 1;}
		// WM_MOUSEACTIVATE -> int onMouseActivate(HWND desktop, UINT hitTest, UINT message)
		DEFINE_DISPATCH(WM_MOUSEACTIVATE) {return w.onMouseActivate(reinterpret_cast<HWND>(wp), LOWORD(lp), HIWORD(lp));}
		// WM_MOUSEMOVE -> void onMouseMove(UINT flags, const ::POINT& position)
		DEFINE_DISPATCH(WM_MOUSEMOVE) {const ::POINT p = {LOWORD(lp), HIWORD(lp)}; w.onMouseMove(static_cast<UINT>(wp), p); return 1;}
#ifdef WM_MOUSEWHEEL
		// WM_MOUSEWHEEL -> void onMouseWheel(UINT flags, short delta, const ::POINT& position)
		DEFINE_DISPATCH(WM_MOUSEWHEEL) {const ::POINT p = {LOWORD(lp), HIWORD(lp)}; w.onMouseWheel(GET_KEYSTATE_WPARAM(wp), GET_WHEEL_DELTA_WPARAM(wp), p); return 1;}
#endif /* WM_MOUSEWHEEL */
		// WM_MOVE -> void onMove(int x, int y)
		DEFINE_DISPATCH(WM_MOVE) {w.onMove(LOWORD(lp), HIWORD(lp)); return 1;}
		// WM_MOVING -> void onMoving(const ::RECT& rect)
		DEFINE_DISPATCH(WM_MOVING) {w.onMoving(*reinterpret_cast<const ::RECT*>(lp)); return 1;}
		// WM_NOTIFY -> bool onNotify(int id, ::NMHDR& nmhdr)
		DEFINE_DISPATCH(WM_NOTIFY) {handled = w.onNotify(static_cast<int>(wp), *reinterpret_cast<::NMHDR*>(lp)); return 1;}
//		// WM_PAINT -> bool onPaint(void)
//		DEFINE_DISPATCH(WM_PAINT) {return w.onPaint();}
		// WM_RBUTTONDBLCLK -> void onRButtonDblClk(UINT flags, const ::POINT& position)
		DEFINE_DISPATCH(WM_RBUTTONDBLCLK) {const ::POINT p = {LOWORD(lp), HIWORD(lp)}; w.onRButtonDblClk(static_cast<UINT>(wp), p); return 1;}
		// WM_RBUTTONDOWN -> void onRButtonDblClk(UINT flags, const ::POINT& position)
		DEFINE_DISPATCH(WM_RBUTTONDOWN) {const ::POINT p = {LOWORD(lp), HIWORD(lp)}; w.onRButtonDown(static_cast<UINT>(wp), p); return 1;}
		// WM_RBUTTONUP -> void onRButtonUp(UINT flags, const ::POINT& position)
		DEFINE_DISPATCH(WM_RBUTTONUP) {const ::POINT p = {LOWORD(lp), HIWORD(lp)}; w.onRButtonUp(static_cast<UINT>(wp), p); return 1;}
		// WM_SETCURSOR -> bool onSetCursor(HWND window, UINT hitTest, UINT message)
		DEFINE_DISPATCH(WM_SETCURSOR) {return handled = w.onSetCursor(reinterpret_cast<HWND>(wp), LOWORD(lp), HIWORD(lp));}
		// WM_SETFOCUS -> void onSetFocus(HWND oldWindow)
		DEFINE_DISPATCH(WM_SETFOCUS) {w.onSetFocus(reinterpret_cast<HWND>(wp)); return 1;}
		// WM_SETFONT -> void onSetFont(HFONT font, bool redraw)
		DEFINE_DISPATCH(WM_SETFONT) {handled = true; w.onSetFont(reinterpret_cast<HFONT>(wp), toBoolean(LOWORD(lp))); return 0;}
		// WM_SETTEXT -> bool onSetText(const TCHAR* text)
		DEFINE_DISPATCH(WM_SETTEXT) {handled = w.onSetText(reinterpret_cast<const TCHAR*>(lp)); return 0;}
		// WM_SETTINGCHANGE -> void onSettingChange(UINT flags, const TCHAR* sectionName)
		DEFINE_DISPATCH(WM_SETTINGCHANGE) {w.onSettingChange(static_cast<UINT>(wp), reinterpret_cast<const TCHAR*>(lp)); return 1;}
		// WM_SIZE -> void onSize(UINT type, int cx, int cy)
		DEFINE_DISPATCH(WM_SIZE) {w.onSize(static_cast<UINT>(wp), LOWORD(lp), HIWORD(lp)); return 1;}
		// WM_SIZING -> void onSizing(UINT side, const ::RECT& rect)
		DEFINE_DISPATCH(WM_SIZING) {w.onSizing(static_cast<UINT>(wp), *reinterpret_cast<::RECT*>(lp)); return 1;}
		// WM_STYLECHANGED -> void onStyleChanged(int type, const ::STYLESTRUCT& style)
		DEFINE_DISPATCH(WM_STYLECHANGED) {w.onStyleChanged(static_cast<int>(wp), *reinterpret_cast<const ::STYLESTRUCT*>(lp)); return 1;}
		// WM_STYLECHANGING -> void onStyleChanging(int type, ::STYLESTRUCT& style)
		DEFINE_DISPATCH(WM_STYLECHANGING) {w.onStyleChanging(static_cast<int>(wp), *reinterpret_cast<::STYLESTRUCT*>(lp)); return 1;}
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
#endif /* WM_THEMECHANGED */
		// WM_TIMER -> void onTimer(UINT_PTR eventID, ::TIMERPROC)
		DEFINE_DISPATCH(WM_TIMER) {w.onTimer(static_cast<UINT_PTR>(wp), reinterpret_cast<::TIMERPROC>(lp)); return 1;}
#ifdef WM_UNICHAR
		// WM_UNICHAR -> void onUniChar(UINT code, UINT flags)
		DEFINE_DISPATCH(WM_UNICHAR) {w.onUniChar(static_cast<UINT>(wp), static_cast<UINT>(lp)); return 1;}
#endif /* WM_UNICHAR */
		// WM_VSCROLL -> void onVScroll(UINT sbCode, UINT position, HWND scrollBar)
		DEFINE_DISPATCH(WM_VSCROLL) {w.onVScroll(LOWORD(wp), HIWORD(wp), reinterpret_cast<HWND>(lp)); return 1;}
#ifdef WM_XBUTTONDBLCLK
		// WM_XBUTTONDBLCLK -> bool onXButtonDblClk(WORD xButton, WORD keyState, const ::POINT& position)
		DEFINE_DISPATCH(WM_XBUTTONDBLCLK) {const ::POINT p = {LOWORD(lp), HIWORD(lp)}; return w.onXButtonDblClk(GET_XBUTTON_WPARAM(wp), GET_KEYSTATE_WPARAM(wp), p);}
		// WM_XBUTTONDOWN -> bool onXButtonDblClk(WORD xButton, WORD keyState, const ::POINT& position)
		DEFINE_DISPATCH(WM_XBUTTONDOWN) {const ::POINT p = {LOWORD(lp), HIWORD(lp)}; return w.onXButtonDown(GET_XBUTTON_WPARAM(wp), GET_KEYSTATE_WPARAM(wp), p);}
		// WM_XBUTTONUP -> bool onXButtonUp(WORD xButton, WORD keyState, const ::POINT& position)
		DEFINE_DISPATCH(WM_XBUTTONUP) {const ::POINT p = {LOWORD(lp), HIWORD(lp)}; return w.onXButtonUp(GET_XBUTTON_WPARAM(wp), GET_KEYSTATE_WPARAM(wp), p);}
#endif /* WM_XBUTTONDBLCLK */
#undef DEFINE_DISPATCH
	};
}

#define MANAH_DECLEAR_WINDOW_MESSAGE_MAP(WindowClass)							\
protected:																		\
	virtual bool processWindowMessage(UINT message, WPARAM, LPARAM, LRESULT&);	\
	friend struct manah::win32::ui::internal::MessageDispatcher<WindowClass>

#define MANAH_BEGIN_WINDOW_MESSAGE_MAP(WindowClass, BaseClass)												\
	bool WindowClass::processWindowMessage(UINT message, WPARAM wParam, LPARAM lParam, LRESULT& result) {	\
		typedef WindowClass Klass; typedef BaseClass BaseKlass; bool handled = false;						\
		switch(message) {

#define MANAH_WINDOW_MESSAGE_ENTRY(msg)														\
		case msg: result = manah::win32::ui::internal::MessageDispatcher<Klass>::dispatch(	\
			*this, manah::win32::ui::internal::Msg2Type<msg>(), wParam, lParam, handled);	\
			if(handled) return true; break;

#define MANAH_END_WINDOW_MESSAGE_MAP()												\
		}																			\
		return BaseKlass::processWindowMessage(message, wParam, lParam, result);	\
	}


template<class ConcreteWindow = Window> class Subclassable : public ConcreteWindow, public Noncopyable {
public:
	explicit Subclassable(HWND handle = 0) : ConcreteWindow(handle), originalProcedure_(0) {}
	virtual ~Subclassable() {if(isWindow()) unsubclass();}
public:
//	virtual bool	attach(HWND handle);
//	bool			attach(HWND handle, bool subclass);
	virtual LRESULT	defWindowProc(UINT message, WPARAM wParam, LPARAM lParam);
//	virtual HWND	detach();
	bool			isSubclassed() const {return originalProcedure_ != 0;}
	bool			subclass();
	bool			unsubclass();
protected:
	virtual bool	processWindowMessage(UINT message, WPARAM wParam, LPARAM lParam, LRESULT& result);
	static LRESULT CALLBACK	windowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
private:
	using ConcreteWindow::fireProcessWindowMessage;
	::WNDPROC originalProcedure_;
};

typedef Subclassable<Window> SubclassableWindow;


template<class ConcreteWindow = Window> class Layered : public ConcreteWindow {
public:
	Layered() : user32Dll_(0), animateWindowProc_(0),
			setLayeredWindowAttributesProc_(0), updateLayeredWindowProc_(0) {}
	explicit Layered(HWND hWnd) : ConcreteWindow(handle), user32Dll_(0), animateWindowProc_(0),
			setLayeredWindowAttributesProc_(0), updateLayeredWindowProc_(0) {}
	virtual ~Layered() {if(user32Dll_ != 0) ::FreeLibrary(user32Dll_);}
public:
	bool	animate(DWORD time, DWORD flags, bool catchError = true);
	bool	setLayeredAttributes(COLORREF keyColor, ::BYTE alpha, DWORD flags);
	bool	updateLayered(HDC destDC, ::POINT* destPt, ::SIZE* size, HDC srcDC, ::POINT* srcPt,
				COLORREF keyColor, ::BLENDFUNCTION* blendFunction, DWORD flags);
private:
	typedef BOOL(WINAPI *AW)(HWND, DWORD, DWORD);
	typedef BOOL(WINAPI *SLWA)(HWND, COLORREF, ::BYTE, DWORD);
	typedef BOOL(WINAPI *ULW)(HWND, HDC, ::POINT*, ::SIZE*, HDC, ::POINT*, COLORREF, ::BLENDFUNCTION*, DWORD);
private:
	HMODULE user32Dll_;	// loaded dynamically to use layered window APIs
	AW animateWindowProc_;
	SLWA setLayeredWindowAttributesProc_;
	ULW updateLayeredWindowProc_;
};

typedef Layered<Window> LayeredWindow;


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
#define DEFINE_CLASS_NAME(name)	public: static const TCHAR* getClassName() {return name;}
template<class Control, class DefaultStyles = DefaultControlStyles>
class StandardControl : public SubclassableWindow {
public:
	explicit StandardControl(HWND handle = 0) : SubclassableWindow(handle) {}
	virtual ~StandardControl() {}
public:
	virtual bool create(HWND parent, const ::RECT& rect = DefaultWindowRect(),
			const TCHAR* windowName = 0, INT_PTR id = 0, DWORD style = 0, DWORD exStyle = 0) {
		return Window::create(Control::getClassName(), parent, rect, windowName,
			DefaultStyles::getStyle(style), DefaultStyles::getExStyle(exStyle), reinterpret_cast<HMENU>(id));
	}
	enum {DefaultStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS};
protected:
	// implementation helpers
	template<typename ReturnType>
	ReturnType sendMessageR(UINT message, WPARAM wParam = 0, LPARAM lParam = 0) {
#pragma warning(disable : 4800)	// non-boolean to boolean conversion
		return static_cast<ReturnType>(sendMessage(message, wParam, lParam));
#pragma warning(default : 4800)
	}
	template<typename ReturnType>
	ReturnType sendMessageC(UINT message, WPARAM wParam = 0, LPARAM lParam = 0) const {
		return const_cast<StandardControl*>(this)->sendMessageR<ReturnType>(message, wParam, lParam);
	}
};


// Control must define one static method:
// void GetClass(GET_CLASS_PARAM_LIST)
#define GET_CLASS_PARAM_LIST	const TCHAR*& name,									\
	HINSTANCE& instance, UINT& style, manah::win32::BrushHandleOrColor& bgColor,	\
	manah::win32::CursorHandleOrID& cursor,	HICON& icon, HICON& smallIcon, int& clsExtraBytes, int& wndExtraBytes
#define DEFINE_WINDOW_CLASS()	public: static void getClass(GET_CLASS_PARAM_LIST)
template<class Control, class BaseWindow = Window>
class CustomControl : public BaseWindow {
public:
	explicit CustomControl(HWND handle = 0) : BaseWindow(handle) {}
	CustomControl(const CustomControl<Control, BaseWindow>& rhs) : BaseWindow(rhs) {}
	virtual ~CustomControl();
	bool	create(HWND parent, const ::RECT& rect = DefaultWindowRect(),
				const TCHAR* windowName = 0, DWORD style = 0UL, DWORD exStyle = 0UL);
protected:
	static LRESULT CALLBACK	windowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
	virtual void	onPaint(gdi::PaintDC& dc) = 0;	// WM_PAINT
private:
	using BaseWindow::fireProcessWindowMessage;
};


// Window ///////////////////////////////////////////////////////////////////

inline Window::Window(HWND handle /* = 0 */) : Handle<HWND, ::DestroyWindow>(handle) {}

inline Window::Window(const Window& rhs) : Handle<HWND, ::DestroyWindow>(rhs) {}

inline Window::~Window() {}

inline UINT Window::arrangeIconicWindows() {return ::ArrangeIconicWindows(get());}

inline gdi::PaintDC Window::beginPaint(::PAINTSTRUCT& paint) {
	assertValidAsWindow(); ::BeginPaint(get(), &paint); return gdi::PaintDC(get(), paint);}

inline void Window::bringToTop() {assertValidAsWindow(); ::BringWindowToTop(get());}

inline void Window::center(HWND alternateWindow /* = 0 */) {
	assertValidAsWindow();

	::RECT winRect, altRect;
	if(alternateWindow == 0){
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

inline bool Window::changeClipboardChain(HWND newNext) {assertValidAsWindow(); return toBoolean(::ChangeClipboardChain(get(), newNext));}

inline std::auto_ptr<Window> Window::childFromPoint(const ::POINT& pt) const {
	assertValidAsWindow(); return createAttached(::ChildWindowFromPoint(get(), pt));}

inline std::auto_ptr<Window> Window::childFromPointEx(const ::POINT& pt, UINT flags) const {
	assertValidAsWindow(); return createAttached(::ChildWindowFromPointEx(get(), pt, flags));}

inline void Window::clientToScreen(::RECT& rect) const {
	assertValidAsWindow();
	::POINT point[2];
	point[0].x = rect.left;
	point[0].y = rect.top;
	point[1].x = rect.right;
	point[1].y = rect.bottom;
	::ClientToScreen(get(), &point[0]);
	::ClientToScreen(get(), &point[1]);
	::SetRect(&rect, point[0].x, point[0].y, point[1].x, point[1].y);
}

inline void Window::clientToScreen(::POINT& pt) const {assertValidAsWindow(); ::ClientToScreen(get(), &pt);}

inline void Window::close() {assertValidAsWindow(); ::CloseWindow(get());}

inline bool Window::create(const TCHAR* className, HWND parentOrHInstance,
		const ::RECT& rect /* = DefaultWindowRect() */, const TCHAR* windowName /* = 0 */,
		DWORD style /* = 0UL */, DWORD exStyle /* = 0UL */, HMENU menu /* = 0 */, void* param /* = 0 */) {
	if(isWindow())
		return false;

	HWND parent = toBoolean(::IsWindow(parentOrHInstance)) ? parentOrHInstance : 0;
	HINSTANCE instance = (parent != 0) ?
#ifdef _WIN64
					reinterpret_cast<HINSTANCE>(::GetWindowLongPtr(parent, GWLP_HINSTANCE))
#else
					reinterpret_cast<HINSTANCE>(static_cast<HANDLE_PTR>(::GetWindowLong(parent, GWL_HINSTANCE)))
#endif /* _WIN64 */
					: reinterpret_cast<HINSTANCE>(parentOrHInstance);

	if(HWND handle = ::CreateWindowEx(exStyle, className, windowName, style,
			rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, parent, menu, instance, param)) {
		reset(handle);
		return true;
	}
	return false;
}

inline bool Window::createCaret(HBITMAP bitmap, int width, int height) {assertValidAsWindow(); return toBoolean(::CreateCaret(get(), bitmap, width, height));}

inline bool Window::createGrayCaret(int width, int height) {return createCaret(reinterpret_cast<HBITMAP>(1), width, height);}

inline bool Window::createSolidCaret(int width, int height) {return createCaret(0, width, height);}

inline LRESULT Window::defWindowProc(UINT message, WPARAM wParam, LPARAM lParam) {assertValidAsWindow(); return ::DefWindowProc(get(), message, wParam, lParam);}

inline bool Window::destroy() {if(toBoolean(::DestroyWindow(get()))) {release(); return true;} return false;}

inline void Window::dragAcceptFiles(bool accept /* = true */) {assertValidAsWindow(); ::DragAcceptFiles(get(), accept);}

inline void Window::drawMenuBar() {assertValidAsWindow(); ::DrawMenuBar(get());}

inline bool Window::enableScrollBar(int barFlags, UINT arrowFlags /* = ESB_ENABLE_BOTH */) {
	assertValidAsWindow(); return toBoolean(::EnableScrollBar(get(), barFlags, arrowFlags));}

inline bool Window::enable(bool enable /* = true */) {assertValidAsWindow(); return toBoolean(::EnableWindow(get(), enable));}

inline void Window::endPaint(const ::PAINTSTRUCT& paint) {assertValidAsWindow(); ::EndPaint(get(), &paint);}

inline int Window::enumerateProperties(::PROPENUMPROC enumFunction) {assertValidAsWindow(); return ::EnumProps(get(), enumFunction);}

inline int Window::enumeratePropertiesEx(::PROPENUMPROCEX enumFunction, LPARAM param) {assertValidAsWindow(); return ::EnumPropsEx(get(), enumFunction, param);}

inline std::auto_ptr<Window> Window::find(const TCHAR* className, const TCHAR* windowName) {return createAttached(::FindWindow(className, windowName));}

inline bool Window::flash(bool invert) {assertValidAsWindow(); return toBoolean(::FlashWindow(get(), invert));}

inline std::auto_ptr<Window> Window::getActive() {return createAttached(::GetActiveWindow());}

inline std::auto_ptr<Window> Window::getCapture() {return createAttached(::GetCapture());}

inline ::POINT Window::getCaretPosition() {::POINT pt; ::GetCaretPos(&pt); return pt;}

inline DWORD Window::getClassLong(int index) const {assertValidAsWindow(); return ::GetClassLong(get(), index);}

#ifdef _WIN64
inline ULONG_PTR Window::getClassLongPtr(int index) const {assertValidAsWindow(); return ::GetClassLongPtr(get(), index);}
#endif /* _WIN64 */

inline int Window::getClassName(TCHAR* className, int maxLength) const {assertValidAsWindow(); return ::GetClassName(get(), className, maxLength);}

inline void Window::getClientRect(::RECT& rect) const {assertValidAsWindow(); ::GetClientRect(get(), &rect);}

inline ::POINT Window::getCursorPosition() const {::POINT pt; ::GetCursorPos(&pt); screenToClient(pt); return pt;}

inline gdi::ClientDC Window::getDC() {assertValidAsWindow(); return gdi::ClientDC(get());}

inline gdi::ClientDC Window::getDCEx(HRGN clipRegion, DWORD flags) {assertValidAsWindow(); return gdi::ClientDC(get(), clipRegion, flags);}

inline std::auto_ptr<Window> Window::getDesktop() {return createAttached(::GetDesktopWindow());}

inline int Window::getDlgCtrlID() const {assertValidAsWindow(); return ::GetDlgCtrlID(get());}

inline DWORD Window::getExStyle() const {assertValidAsWindow(); return static_cast<DWORD>(getWindowLong(GWL_EXSTYLE));}

inline std::auto_ptr<Window> Window::getFocus() {return createAttached(::GetFocus());}

inline HFONT Window::getFont() const {return reinterpret_cast<HFONT>(::SendMessage(get(), WM_GETFONT, 0, 0L));}

inline std::auto_ptr<Window> Window::getForeground() {return createAttached(::GetForegroundWindow());}

inline DWORD Window::getHotKey() const {return static_cast<DWORD>(::SendMessage(get(), WM_GETHOTKEY, 0, 0L));}

inline HICON Window::getIcon(bool bigIcon /* = true */) const {
	return reinterpret_cast<HICON>(::SendMessage(get(), WM_GETICON, bigIcon ? ICON_BIG : ICON_SMALL, 0L));}

inline std::auto_ptr<Window> Window::getLastActivePopup() const {assertValidAsWindow(); return createAttached(::GetLastActivePopup(get()));}

inline std::auto_ptr<Menu> Window::getMenu() const {assertValidAsWindow(); std::auto_ptr<Menu> temp(new Menu); temp->attach(::GetMenu(get())); return temp;}

inline std::auto_ptr<Window> Window::getNext(UINT flag /* = GW_HWNDNEXT */) const {assertValidAsWindow(); return createAttached(::GetNextWindow(get(), flag));}

inline std::auto_ptr<Window> Window::getOwner() const {assertValidAsWindow(); return createAttached(::GetWindow(get(), GW_OWNER));}

inline std::auto_ptr<Window> Window::getParent() const {assertValidAsWindow(); return createAttached(::GetParent(get()));}

inline HANDLE Window::getProperty(const TCHAR* identifier) const {assertValidAsWindow(); return ::GetProp(get(), identifier);}

inline HANDLE Window::getProperty(::ATOM identifier) const {return getProperty(reinterpret_cast<const TCHAR*>(identifier));}

inline bool Window::getScrollInformation(int bar, ::SCROLLINFO& scrollInfo, UINT mask /* = SIF_ALL */) const {
	assertValidAsWindow();
	scrollInfo.cbSize = sizeof(::SCROLLINFO);
	scrollInfo.fMask = mask;
	return toBoolean(::GetScrollInfo(get(), bar, &scrollInfo));
}

inline int Window::getScrollLimit(int bar) const {
	assertValidAsWindow();
	int dummy, limit;
	getScrollRange(bar, dummy, limit);
	return limit;
}

inline int Window::getScrollPosition(int bar) const {assertValidAsWindow(); return ::GetScrollPos(get(), bar);}

inline void Window::getScrollRange(int bar, int& minPos, int& maxPos) const {
	assertValidAsWindow(); ::GetScrollRange(get(), bar, &minPos, &maxPos);}

inline int Window::getScrollTrackPosition(int bar) const {
	assertValidAsWindow();
	::SCROLLINFO si;
	return (getScrollInformation(bar, si, SIF_TRACKPOS)) ? si.nTrackPos : -1;
}

inline DWORD Window::getStyle() const {return static_cast<DWORD>(getWindowLong(GWL_STYLE));}

inline std::auto_ptr<Menu> Window::getSystemMenu(bool revert) const {
	assertValidAsWindow(); std::auto_ptr<Menu> temp(new Menu); temp->attach(::GetSystemMenu(get(), revert)); return temp;}

inline std::auto_ptr<Window> Window::getTop() const {assertValidAsWindow(); return createAttached(::GetTopWindow(get()));}

inline bool Window::getUpdateRect(::RECT& rect, bool erase /* = false */) {
	assertValidAsWindow(); return toBoolean(::GetUpdateRect(get(), &rect, erase));}

inline int Window::getUpdateRegion(HRGN region, bool erase /* = false */) {assertValidAsWindow(); return ::GetUpdateRgn(get(), region, erase);}

inline std::auto_ptr<Window> Window::getWindow(UINT command) const {assertValidAsWindow(); return createAttached(::GetWindow(get(), command));}

inline DWORD Window::getContextHelpID() const {assertValidAsWindow(); return ::GetWindowContextHelpId(get());}

inline gdi::WindowDC Window::getWindowDC() {assertValidAsWindow(); return gdi::WindowDC(get());}

inline LONG Window::getWindowLong(int index) const {assertValidAsWindow(); return ::GetWindowLong(get(), index);}

#ifdef _WIN64
inline LONG_PTR Window::getWindowLongPtr(int index) const {assertValidAsWindow(); return ::GetWindowLongPtr(get(), index);}
#endif /* _WIN64 */

inline bool Window::getPlacement(::WINDOWPLACEMENT& placement) const {assertValidAsWindow(); return toBoolean(::GetWindowPlacement(get(), &placement));}

inline DWORD Window::getProcessID() const {assertValidAsWindow(); DWORD id; ::GetWindowThreadProcessId(get(), &id); return id;}

inline void Window::getRect(::RECT& rect) const {assertValidAsWindow(); ::GetWindowRect(get(), &rect);}

inline int Window::getRegion(HRGN region) const {assertValidAsWindow(); return ::GetWindowRgn(get(), region);}

inline int Window::getText(TCHAR* text, int maxLength) const {assertValidAsWindow(); return ::GetWindowText(get(), text, maxLength);}

inline std::basic_string<TCHAR> Window::getText() const {
	const int len = getTextLength();
	AutoBuffer<TCHAR> buffer(new TCHAR[len + 1]);
	getText(buffer.get(), len + 1);
	return std::basic_string<TCHAR>(buffer.get());
}

inline int Window::getTextLength() const {assertValidAsWindow(); return ::GetWindowTextLength(get());}

inline DWORD Window::getThreadID() const {assertValidAsWindow(); return ::GetWindowThreadProcessId(get(), 0);}

inline bool Window::hasFocus() const {return ::GetFocus() == get();}

inline void Window::hideCaret() {assertValidAsWindow(); ::HideCaret(get());}

inline bool Window::hiliteMenuItem(HMENU menu, UINT item, UINT flags) {
	assertValidAsWindow(); return toBoolean(::HiliteMenuItem(get(), menu, item, flags));}

inline void Window::invalidate(bool erase /* = true */) {assertValidAsWindow(); ::InvalidateRect(get(), 0, erase);}

inline void Window::invalidateRect(const ::RECT* rect, bool erase /* = true */) {
	assertValidAsWindow(); ::InvalidateRect(get(), rect, erase);}

inline void Window::invalidateRegion(HRGN region, bool erase /* = true */) {assertValidAsWindow(); ::InvalidateRgn(get(), region, erase);}

inline bool Window::isChild(HWND window) const {assertValidAsWindow(); return toBoolean(::IsChild(get(), window));}

inline bool Window::isIconic() const {assertValidAsWindow(); return toBoolean(::IsIconic(get()));}

inline bool Window::isWindow() const {return toBoolean(::IsWindow(get()));}

inline bool Window::isEnabled() const {assertValidAsWindow(); return toBoolean(::IsWindowEnabled(get()));}

inline bool Window::isUnicode() const {assertValidAsWindow(); return toBoolean(::IsWindowUnicode(get()));}

inline bool Window::isVisible() const {return toBoolean(::IsWindowVisible(get()));}

inline bool Window::isZoomed() const {assertValidAsWindow(); return toBoolean(::IsZoomed(get()));}

inline bool Window::killTimer(UINT eventID) {assertValidAsWindow(); return toBoolean(::KillTimer(get(), eventID));}

inline bool Window::lockUpdate() {assertValidAsWindow(); return toBoolean(::LockWindowUpdate(get()));}

inline void Window::mapWindowPoints(HWND destWindow, ::RECT& rect) const {
	assertValidAsWindow();
	::POINT point[2];
	point[0].x = rect.left;
	point[0].y = rect.top;
	point[1].x = rect.right;
	point[1].y = rect.bottom;
	::MapWindowPoints(get(), destWindow, point, 2);
	::SetRect(&rect, point[0].x, point[0].y, point[1].x, point[1].y);
}

inline void Window::mapWindowPoints(HWND destWindow, ::POINT points[], UINT count) const {
	assertValidAsWindow(); ::MapWindowPoints(get(), destWindow, points, count);}

inline int Window::messageBox(const TCHAR* text, const TCHAR* caption /* = 0 */, UINT type /* = MB_OK */) {
	assertValidAsWindow(); return ::MessageBox(get(), text, caption, type);}

inline bool Window::modifyStyle(DWORD removeStyles, DWORD addStyles) {
	assertValidAsWindow();
	DWORD style;
	style = getStyle();
	style &= ~removeStyles;
	style |= addStyles;
	if(setWindowLong(GWL_STYLE, style) == 0)
		return false;
	return true;
}

inline bool Window::modifyStyleEx(DWORD removeStyles, DWORD addStyles) {
	assertValidAsWindow();
	DWORD exStyle = getExStyle();
	exStyle &= ~removeStyles;
	exStyle |= addStyles;
	if(setWindowLong(GWL_EXSTYLE, exStyle) == 0)
		return false;
	return true;
}

inline void Window::move(int x, int y, int width, int height, bool repaint /* = true */) {
	assertValidAsWindow(); ::MoveWindow(get(), x, y, width, height, repaint);}

inline void Window::move(const ::RECT& rect, bool repaint /* = true */) {
	assertValidAsWindow(); ::MoveWindow(get(), rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, repaint);}

inline void Window::print(HDC dc, DWORD flags) const {::SendMessage(get(), WM_PRINT, reinterpret_cast<WPARAM>(dc), flags);}

inline void Window::printClient(HDC dc, DWORD flags) const {::SendMessage(get(), WM_PRINTCLIENT, reinterpret_cast<WPARAM>(dc), flags);}

inline LRESULT Window::postMessage(UINT message, WPARAM wParam /* = 0 */, LPARAM lParam /* = 0L */) {
	assertValidAsWindow(); return ::PostMessage(get(), message, wParam, lParam);}

inline bool Window::redraw(::RECT* updateRect /* = 0 */, HRGN clipRegion/* = 0 */,
		UINT flags /* = RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE */) {
	assertValidAsWindow(); return toBoolean(::RedrawWindow(get(), updateRect, clipRegion, flags));}

inline HRESULT Window::registerDragDrop(IDropTarget& dropTarget) {
	assertValidAsWindow(); return ::RegisterDragDrop(get(), &dropTarget);}

inline bool Window::releaseCapture() {return toBoolean(::ReleaseCapture());}

inline int Window::releaseDC(HDC dc) {assertValidAsWindow(); return ::ReleaseDC(get(), dc);}

inline HANDLE Window::removeProperty(const TCHAR* identifier) {assertValidAsWindow(); return ::RemoveProp(get(), identifier);}

inline HANDLE Window::removeProperty(::ATOM identifier) {return removeProperty(reinterpret_cast<const TCHAR*>(identifier));}

inline HRESULT Window::revokeDragDrop() {assertValidAsWindow(); return ::RevokeDragDrop(get());}

inline void Window::screenToClient(::RECT& rect) const {
	assertValidAsWindow();
	::POINT point[2];
	point[0].x = rect.left;
	point[0].y = rect.top;
	point[1].x = rect.right;
	point[1].y = rect.bottom;
	::ScreenToClient(get(), &point[0]);
	::ScreenToClient(get(), &point[1]);
	::SetRect(&rect, point[0].x, point[0].y, point[1].x, point[1].y);
}

inline void Window::scroll(int xAmount, int yAmount, ::RECT* rect /* = 0 */, LPRECT clipRect /* = 0 */) {
	assertValidAsWindow(); ::ScrollWindow(get(), xAmount, yAmount, rect, clipRect);}

inline int Window::scrollEx(
		int dx, int dy, RECT* scrollRect, RECT* clipRect, HRGN updateRegion, RECT* updateRect, UINT flags) {
	assertValidAsWindow(); return ::ScrollWindowEx(get(), dx, dy, scrollRect, clipRect, updateRegion, updateRect, flags);}

inline LRESULT Window::sendMessage(UINT message, WPARAM wParam /* = 0 */, LPARAM lParam /* = 0L */) {
	assertValidAsWindow(); return ::SendMessage(get(), message, wParam, lParam);}

inline bool Window::sendNotifyMessage(UINT message, WPARAM wParam, LPARAM lParam) {
	assertValidAsWindow(); return toBoolean(::SendNotifyMessage(get(), message, wParam, lParam));}

inline void Window::screenToClient(::POINT& pt) const {assertValidAsWindow(); ::ScreenToClient(get(), &pt);}

inline std::auto_ptr<Window> Window::setActive() {assertValidAsWindow(); return createAttached(::SetActiveWindow(get()));}

inline std::auto_ptr<Window> Window::setCapture() {assertValidAsWindow(); return createAttached(::SetCapture(get()));}

inline void Window::setCaretPosition(const POINT& pt) {::SetCaretPos(pt.x, pt.y);}

inline DWORD Window::setClassLong(int index, DWORD newLong) {assertValidAsWindow(); return ::SetClassLong(get(), index, newLong);}

#ifdef _WIN64
inline ULONG_PTR Window::setClassLongPtr(int index, ULONG_PTR newLong) {
	assertValidAsWindow(); return ::SetClassLongPtr(get(), index, newLong);}
#endif /* _WIN64 */

inline std::auto_ptr<Window> Window::setClipboardViewer() {assertValidAsWindow(); return createAttached(::SetClipboardViewer(get()));}

inline bool Window::setCursorPosition(const ::POINT& pt) {POINT p(pt); clientToScreen(p); return toBoolean(::SetCursorPos(p.x, p.y));}

inline int Window::setDlgCtrlID(int id) {assertValidAsWindow(); return static_cast<int>(setWindowLong(GWL_ID, id));}

inline std::auto_ptr<Window> Window::setFocus() {assertValidAsWindow(); return createAttached(::SetFocus(get()));}

inline void Window::setFont(HFONT font, bool redraw /* = true */) {
	sendMessage(WM_SETFONT, reinterpret_cast<WPARAM>(font), MAKELPARAM(redraw, 0));}

inline bool Window::setForeground() {assertValidAsWindow(); return toBoolean(::SetForegroundWindow(get()));}

inline int Window::setHotKey(WORD virtualKeyCode, WORD modifiers) {
	assertValidAsWindow(); return static_cast<int>(sendMessage(WM_SETHOTKEY, MAKEWPARAM(virtualKeyCode, modifiers)));}

inline HICON Window::setIcon(HICON icon, bool bigIcon /* = true */) {
	return reinterpret_cast<HICON>(sendMessage(WM_SETICON, bigIcon ? ICON_BIG : ICON_SMALL));}

inline bool Window::setMenu(HMENU menu) {assertValidAsWindow(); return toBoolean(::SetMenu(get(), menu));}

inline std::auto_ptr<Window> Window::setParent(HWND newParent) {assertValidAsWindow(); return createAttached(::SetParent(get(), newParent));}

inline bool Window::setProperty(const TCHAR* identifier, HANDLE data) {
	assertValidAsWindow(); return toBoolean(::SetProp(get(), identifier, data));}

inline bool Window::setProperty(::ATOM identifier, HANDLE data) {return setProperty(reinterpret_cast<const TCHAR*>(identifier), data);}

inline void Window::setRedraw(bool redraw /* = true */) {sendMessage(WM_SETREDRAW, redraw);}

inline bool Window::setScrollInformation(int bar, const ::SCROLLINFO& scrollInfo, bool redraw /* = true */) {
	assertValidAsWindow(); return toBoolean(::SetScrollInfo(get(), bar, &scrollInfo, redraw));}

inline int Window::setScrollPosition(int bar, int pos, bool redraw /* = true */) {
	assertValidAsWindow(); return ::SetScrollPos(get(), bar, pos, redraw);}

inline void Window::setScrollRange(int bar, int minPos, int maxPos, bool redraw /* = true */) {
	assertValidAsWindow(); ::SetScrollRange(get(), bar, minPos, maxPos, redraw);}

inline UINT_PTR Window::setTimer(
		UINT_PTR eventID, UINT elapse, void (CALLBACK* procedure)(HWND, UINT, UINT_PTR, DWORD) /* = 0 */) {
	assertValidAsWindow(); return static_cast<UINT_PTR>(::SetTimer(get(), eventID, elapse, procedure));}

inline bool Window::setContextHelpID(DWORD contextHelpID) {assertValidAsWindow(); return toBoolean(::SetWindowContextHelpId(get(), contextHelpID));}

inline LONG Window::setWindowLong(int index, LONG newLong) {assertValidAsWindow(); return ::SetWindowLong(get(), index, newLong);}

#ifdef _WIN64
inline LONG_PTR Window::setWindowLongPtr(int index, LONG_PTR newLong) {assertValidAsWindow(); return ::SetWindowLongPtr(get(), index, newLong);}
#endif /* _WIN64 */

inline bool Window::setPlacement(const ::WINDOWPLACEMENT& placement) {assertValidAsWindow(); return toBoolean(::SetWindowPlacement(get(), &placement));}

inline bool Window::setPosition(HWND windowInsertAfter, int x, int y, int cx, int cy, UINT flags) {
	assertValidAsWindow(); return toBoolean(::SetWindowPos(get(), windowInsertAfter, x, y, cx, cy, flags));}

inline bool Window::setPosition(HWND windowInsertAfter, const ::RECT& rect, UINT flags) {
	return setPosition(windowInsertAfter, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, flags);}

inline int Window::setRegion(const HRGN region, bool redraw /* = true */) {
	assertValidAsWindow(); return ::SetWindowRgn(get(), region, redraw);}

inline void Window::setText(const TCHAR* text) {assertValidAsWindow(); ::SetWindowText(get(), text);}

inline void Window::showCaret() {assertValidAsWindow(); ::ShowCaret(get());}

inline void Window::showOwnedPopups(bool show /* = true */) {assertValidAsWindow(); ::ShowOwnedPopups(get(), show);}

inline void Window::showScrollBar(int bar, bool show /* = true */) {assertValidAsWindow(); ::ShowScrollBar(get(), bar, show);}

inline bool Window::show(UINT command) {assertValidAsWindow(); return toBoolean(::ShowWindow(get(), command));}

inline void Window::unlockUpdate() {assertValidAsWindow(); ::LockWindowUpdate(0);}

inline void Window::update() {assertValidAsWindow(); ::UpdateWindow(get());}

inline void Window::validateRect(const ::RECT* rect) {assertValidAsWindow(); ::ValidateRect(get(), rect);}

inline void Window::validateRegion(HRGN region) {assertValidAsWindow(); ::ValidateRgn(get(), region);}

inline std::auto_ptr<Window> Window::fromPoint(const ::POINT& pt) {return createAttached(::WindowFromPoint(pt));}

inline bool Window::winHelp(const TCHAR* help, UINT command /* = HELP_CONTEXT */, DWORD data /* = 0 */) {
	return toBoolean(::WinHelp(get(), help, command, data));}


// Subclassable /////////////////////////////////////////////////////////////
/*
template<class ConcreteWindow> inline bool Subclassable<ConcreteWindow>::attach(HWND handle) {return ConcreteWindow::attach(handle);}

template<class ConcreteWindow> inline bool Subclassable<ConcreteWindow>::attach(HWND handle, bool subclass) {
	if(subclass)	return attach(window) ? subclassWindow() : false;
	else			return attach(window);
}
*/
template<class ConcreteWindow> inline LRESULT Subclassable<ConcreteWindow>::defWindowProc(UINT message, WPARAM wParam, LPARAM lParam) {
	assertValidAsWindow();
	return (originalProcedure_ != 0) ?
		::CallWindowProc(originalProcedure_, get(), message, wParam, lParam)
		: ::DefWindowProc(get(), message, wParam, lParam);
}
/*
template<class ConcreteWindow> inline HWND Subclassable<ConcreteWindow>::detach() {
	if(isAttached())
		unsubclassWindow();
	return ConcreteWindow::detach();
}
*/
template<class ConcreteWindow>
inline bool Subclassable<ConcreteWindow>::processWindowMessage(UINT message, WPARAM wParam, LPARAM lParam, LRESULT& result) {
	if(message == WM_NCDESTROY) {
		unsubclass();
		return false;
	}
	return ConcreteWindow::processWindowMessage(message, wParam, lParam, result);
}

template<class ConcreteWindow> inline bool Subclassable<ConcreteWindow>::subclass() {
	assertValidAsWindow();
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
#endif /* _WIN64 */
		return true;
	} else
		return false;
}

template<class ConcreteWindow> inline bool Subclassable<ConcreteWindow>::unsubclass() {
	assertValidAsWindow();
	if(!isSubclassed())
		return false;
#ifdef _WIN64
	setWindowLongPtr(GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(originalProcedure_));
	setWindowLongPtr(GWLP_USERDATA, 0);
#else
	setWindowLong(GWL_WNDPROC, static_cast<long>(reinterpret_cast<LONG_PTR>(originalProcedure_)));
	setWindowLong(GWL_USERDATA, 0);
#endif
	originalProcedure_ = 0;
	return true;
}

template<class ConcreteWindow>
inline LRESULT CALLBACK Subclassable<ConcreteWindow>::windowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
#ifdef _WIN64
	if(Subclassable<ConcreteWindow>* instance =
		reinterpret_cast<Subclassable<Window>*>(::GetWindowLongPtr(window, GWLP_USERDATA)))
#else
	if(Subclassable<ConcreteWindow>* instance =
		reinterpret_cast<Subclassable<Window>*>(static_cast<LONG_PTR>(::GetWindowLong(window, GWL_USERDATA))))
#endif /* _WIN64 */
		return instance->fireProcessWindowMessage(message, wParam, lParam);
	return false;
}


// Layered //////////////////////////////////////////////////////////////////

template<class ConcreteWindow>
inline bool Layered<ConcreteWindow>::animate(DWORD time, DWORD flags, bool catchError /* = true */) {
	assertValidAsWindow();
	if(user32Dll_ == 0)
		user32Dll_ = ::LoadLibrary(_T("User32.dll"));
	if(user32Dll_ != 0 && animateWindowProc_ == 0)
		animateWindowProc_ = reinterpret_cast<AW>(::GetProcAddress(user32Dll_, "AnimateWindow"));
	if((animateWindowProc_ != 0) ? toBoolean((*animateWindowProc_)(get(), time, flags)) : false)
		return true;
	else if(catchError) {
		if(toBoolean(flags & AW_HIDE))
			return show(SW_HIDE);
		else
			return show(toBoolean(flags & AW_ACTIVATE) ? SW_SHOW : SW_SHOWNA);
	}
	return false;
}

template<class ConcreteWindow>
inline bool Layered<ConcreteWindow>::setLayeredAttributes(COLORREF keyColor, ::BYTE alpha, DWORD flags) {
	assertValidAsWindow();
	if(user32Dll_ == 0)
		user32Dll_ = ::LoadLibrary(_T("User32.dll"));
	if(user32Dll_ != 0 && setLayeredWindowAttributesProc_ == 0)
		setLayeredWindowAttributesProc_ =
			reinterpret_cast<SLWA>(::GetProcAddress(user32Dll_, "SetLayeredWindowAttributes"));
	return (setLayeredWindowAttributesProc_ != 0) ?
		toBoolean((*setLayeredWindowAttributesProc_)(get(), keyColor, alpha, flags)) : false;
}

template<class ConcreteWindow>
inline bool Layered<ConcreteWindow>::updateLayered(HDC destDC, ::POINT* destPt,
		::SIZE* size, HDC srcDC, ::POINT* srcPt, COLORREF keyColor, ::BLENDFUNCTION* blendFunction, DWORD flags) {
	assertValidAsWindow();
	if(user32Dll_ == 0)
		user32Dll_ = ::LoadLibrary(_T("User32.dll"));
	if(user32Dll_ != 0 && updateLayeredWindowProc_ == 0)
		updateLayeredWindowProc_ =
			reinterpret_cast<ULW>(::GetProcAddress(user32Dll_, "UpdateLayeredWindow"));
	return (updateLayeredWindowProc_ != 0) ?
		toBoolean((*updateLayeredWindowProc_)(get(), destDC, destPt, size, srcDC, srcPt, keyColor, blendFunction, flags)) : false;
}


// CustomControl ////////////////////////////////////////////////////////////

template<class Control, class BaseWindow> inline CustomControl<Control, BaseWindow>::~CustomControl() {
	// prevent to be called as this by windowProcedure
	if(isWindow())
		::SetWindowLongPtr(get(), GWLP_USERDATA, 0);
}

template<class Control, class BaseWindow>
inline bool CustomControl<Control, BaseWindow>::create(HWND parent, const ::RECT& rect /* = DefaultWindowRect() */,
		const TCHAR* windowName /* = 0 */, DWORD style /* = 0UL */, DWORD exStyle /* = 0UL */) {
	BrushHandleOrColor bgColor;
	CursorHandleOrID cursor;
	AutoZeroCB<::WNDCLASSEX> wc, dummy;

	wc.hInstance = ::GetModuleHandle(0);	// default value
	wc.lpfnWndProc = CustomControl<Control, BaseWindow>::windowProcedure;
	Control::getClass(wc.lpszClassName, wc.hInstance, wc.style,
		bgColor, cursor, wc.hIcon, wc.hIconSm, wc.cbClsExtra, wc.cbWndExtra);
	wc.hbrBackground = bgColor.brush;
	wc.hCursor = cursor.cursor;
	if(::GetClassInfoEx(wc.hInstance, wc.lpszClassName, &dummy) == 0)
		::RegisterClassEx(&wc);
	return Window::create(wc.lpszClassName, parent, rect, windowName, style, exStyle, 0, this);
}

template<class Control, class BaseWindow>
inline LRESULT CALLBACK CustomControl<Control, BaseWindow>::windowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
	typedef CustomControl<Control, BaseWindow> C;
	if(message == WM_CREATE) {
		C* const p = reinterpret_cast<C*>(reinterpret_cast<::CREATESTRUCT*>(lParam)->lpCreateParams);
		assert(p != 0);
		p->reset(window);	// ... the handle will be reset by BaseWindow::create (no problem)
#ifdef _WIN64
		p->setWindowLongPtr(GWLP_USERDATA, reinterpret_cast<LONG_PTR>(p));
#else
		p->setWindowLong(GWL_USERDATA, static_cast<long>(reinterpret_cast<LONG_PTR>(p)));
#endif /* _WIN64 */

		return p->fireProcessWindowMessage(message, wParam, lParam);
	} else {
		C* const p = reinterpret_cast<C*>(
#ifdef _WIN64
			::GetWindowLongPtr(window, GWLP_USERDATA));
#else
			static_cast<LONG_PTR>(::GetWindowLong(window, GWL_USERDATA)));
#endif /* _WIN64 */
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

#endif /* !MANAH_WINDOW_HPP */
