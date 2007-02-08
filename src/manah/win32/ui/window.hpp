// window.hpp
// (c) 2002-2007

#ifndef MANAH_WINDOW_HPP
#define MANAH_WINDOW_HPP

#include "../dc.hpp"
#include <shellapi.h>	// DragAcceptFiles
#include <ole2.h>		// D&D
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
namespace windows {
namespace ui {

struct DefaultWindowRect : public ::tagRECT {
	DefaultWindowRect() {left = top = right = bottom = CW_USEDEFAULT;}
};

template<class Window> class Subclassable;

class Window : public HandleHolder<HWND> {
public:
	// �R���X�g���N�^
	explicit Window(HWND handle = 0);	// just attaches (does not delegate ownership)
	Window(const Window& rhs);
	virtual ~Window();
	// ���Z�q
	bool operator ==(const Window& rhs) const {return get() == rhs.get();}
	operator HWND() const {return get();}
	HWND getSafeHwnd() const throw() {return (this != 0) ? get() : 0;}
	// �\�z�Ɣj��
	virtual bool	attach(HWND handle);
	bool			create(const TCHAR* className, HWND parentOrHInstance,
						const RECT& rect = DefaultWindowRect(), const TCHAR* windowName = 0,
						DWORD style = 0UL, DWORD exStyle = 0UL, HMENU menu = 0, void* param = 0);
	bool			destroyWindow();
	// �X�^�C��
	DWORD	getExStyle() const;
	DWORD	getStyle() const;
	bool	modifyStyle(DWORD removeStyles, DWORD addStyles);
	bool	modifyStyleEx(DWORD removeStyles, DWORD addStyles);
	// �E�B���h�E�N���X
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
	// ���
	bool			enableWindow(bool enable = true);
	static Window	getActiveWindow();
	static Window	getCapture();
	static Window	getDesktopWindow();
	static Window	getFocus();
	static Window	getForegroundWindow();
	HICON			getIcon(bool bigIcon = true) const;
	bool			hasFocus() const;
	bool			isWindow() const;
	bool			isWindowEnabled() const;
	bool			isWindowUnicode() const;
	static bool		releaseCapture();
	Window			setActiveWindow();
	Window			setCapture();
	Window			setFocus();
	bool			setForegroundWindow();
	HICON			setIcon(HICON icon, bool bigIcon  = true);
	// �T�C�Y�ƈʒu
	UINT	arrangeIconicWindows();
	void	bringWindowToTop();
	void	getClientRect(RECT& rect) const;
	bool	getWindowPlacement(WINDOWPLACEMENT& placement) const;
	void	getWindowRect(RECT& rect) const;
	int		getWindowRgn(HRGN region) const;
	bool	isIconic() const;
	bool	isZoomed() const;
	void	moveWindow(int x, int y, int width, int height, bool repaint = true);
	void	moveWindow(const RECT& rect, bool repaint = true);
	bool	setWindowPlacement(const WINDOWPLACEMENT& placement);
	bool	setWindowPos(HWND windowInsertAfter, int x, int y, int cx, int cy, UINT flags);
	bool	setWindowPos(HWND windowInsertAfter, const RECT& rect, UINT flags);
	int		setWindowRgn(const HRGN region, bool redraw = true);
	// �E�B���h�E�A�N�Z�X
	void			centerWindow(HWND alternateWindow = 0);
	Window			childWindowFromPoint(const POINT& pt) const;
	Window			childWindowFromPointEx(const POINT& pt, UINT flags) const;
	static Window	findWindow(const TCHAR* className, const TCHAR* windowName);
	int				getDlgCtrlID() const;
	Window			getLastActivePopup() const;
	Window			getNext(UINT flag = GW_HWNDNEXT) const;
	Window			getOwner() const;
	Window			getParent() const;
	Window			getTopWindow() const;
	Window			getWindow(UINT command) const;
	bool			isChild(HWND window) const;
	int				setDlgCtrlID(int id);
	Window			setParent(HWND newParent);
	static Window	windowFromPoint(const POINT& pt);
	// �X�V�ƕ`��
	gdi::PaintDC	beginPaint(PAINTSTRUCT& paint);
	bool			enableScrollBar(int barFlags, UINT arrowFlags = ESB_ENABLE_BOTH);
	void			endPaint(const PAINTSTRUCT& paint);
	gdi::ClientDC	getDC();
	gdi::ClientDC	getDCEx(HRGN clipRegion, DWORD flags);
	bool			getUpdateRect(RECT& rect, bool erase = false);
	int				getUpdateRgn(HRGN region, bool erase = false);
	gdi::WindowDC	getWindowDC();
	bool			lockWindowUpdate();
	void			invalidate(bool erase = true);
	void			invalidateRect(const RECT* rect, bool erase = true);
	void			invalidateRgn(HRGN region, bool erase = true);
	bool			isWindowVisible() const;
	void			print(HDC dc, DWORD flags) const;
	void			printClient(HDC dc, DWORD flags) const;
	bool			redrawWindow(RECT* rectUpdate = 0, HRGN clipRegion = 0, UINT flags = RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
	int				releaseDC(HDC dc);
	void			setRedraw(bool redraw = true);
	void			showOwnedPopups(bool show = true);
	bool			showWindow(UINT command);
	void			unlockWindowUpdate();
	void			updateWindow();
	void			validateRect(const RECT* rect);
	void			validateRgn(HRGN region);
	// ���W�}�b�s���O
	void	clientToScreen(POINT& pt) const;
	void	clientToScreen(RECT& rect) const;
	void	mapWindowPoints(HWND destWindow, RECT& rect) const;
	void	mapWindowPoints(HWND destWindow, POINT points[], UINT count) const;
	void	screenToClient(POINT& pt) const;
	void	screenToClient(RECT& rect) const;
	// �E�B���h�E�e�L�X�g
	int							getWindowText(TCHAR* text, int maxCount) const;
	std::basic_string<TCHAR>	getWindowText() const;
	int							getWindowTextLength() const;
	void						setWindowText(const TCHAR* text);
	// �t�H���g
	HFONT	getFont() const;
	void	setFont(HFONT font, bool redraw = true);
	// �v���p�e�B
	int		enumProps(PROPENUMPROC enumFunction);
	int		enumPropsEx(PROPENUMPROCEX enumFunction, LPARAM param);
	HANDLE	getProp(const TCHAR* identifier) const;
	HANDLE	getProp(ATOM identifier) const;
	HANDLE	removeProp(const TCHAR* identifier);
	HANDLE	removeProp(ATOM identifier);
	bool	setProp(const TCHAR* identifier, HANDLE data);
	bool	setProp(ATOM identifier, HANDLE data);
	// �w���v
	DWORD	getWindowContextHelpID() const;
	bool	setWindowContextHelpID(DWORD contextHelpID);
	bool	winHelp(const TCHAR* help, UINT command = HELP_CONTEXT, DWORD data = 0);
	// �X�N���[��
	bool	getScrollInfo(int bar, SCROLLINFO& scrollInfo, UINT mask = SIF_ALL) const;
	int		getScrollLimit(int bar) const;
	int		getScrollPos(int bar) const;
	void	getScrollRange(int bar, int& minPos, int& maxPos) const;
	int		getScrollTrackPos(int bar) const;
	void	scrollWindow(int xAmount, int yAmount, RECT* rect = 0, RECT* clipRect = 0);
	int		scrollWindowEx(int dx, int dy, RECT* scrollRect, RECT* clipRect,
				HRGN updateRegion, RECT* updateRect, UINT flags);
	bool	setScrollInfo(int bar, const SCROLLINFO& scrollInfo, bool bedraw = true);
	int		setScrollPos(int bar, int pos, bool redraw = true);
	void	setScrollRange(int bar, int minPos, int maxPos, bool eedraw = true);
	void	showScrollBar(int bar, bool show = true);
	// �N���b�v�{�[�h�r���[�A
	bool	changeClipboardChain(HWND newNext);
	Window	setClipboardViewer();
	// �h���b�O�A���h�h���b�v
	void	dragAcceptFiles(bool accept = true);
	HRESULT	registerDragDrop(IDropTarget& dropTarget);
	HRESULT	revokeDragDrop();
	// �L�����b�g
	bool		createCaret(HBITMAP bitmap, int width, int height);
	bool		createSolidCaret(int width, int height);
	bool		createGrayCaret(int width, int height);
	POINT		getCaretPos();
	void		hideCaret();
	static void	setCaretPos(const POINT& pt);
	void		showCaret();
	// �J�[�\��
	POINT	getCursorPos() const;
	bool	setCursorPos(const POINT& pt);
	// ���j���[
	void	drawMenuBar();
	HMENU	getMenu() const;
	HMENU	getSystemMenu(bool revert) const;
	bool	hiliteMenuItem(HMENU menu, UINT item, UINT flags);
	bool	setMenu(HMENU menu);
	// �z�b�g�L�[
	DWORD	getHotKey() const;
	int		setHotKey(WORD virtualKeyCode, WORD modifiers);
	// �^�C�}
	bool		killTimer(UINT_PTR eventID);
	UINT_PTR	setTimer(UINT_PTR eventID, UINT elapse,
					void (CALLBACK* procedure)(HWND, UINT, UINT_PTR, DWORD) = 0);
	// �x��
	bool	flashWindow(bool invert);
	int		messageBox(const TCHAR* text, const TCHAR* caption = 0, UINT type = MB_OK);
	// �E�B���h�E���b�Z�[�W
	virtual LRESULT	defWindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT			sendMessage(UINT message, WPARAM wParam = 0, LPARAM lParam = 0L);
	bool			sendNotifyMessage(UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT			postMessage(UINT message, WPARAM wParam = 0, LPARAM lParam = 0L);
	// �v���Z�X�ƃX���b�h
	DWORD	getWindowProcessID() const;
	DWORD	getWindowThreadID() const;
	// �I�[�o�[���C�h�\�ȃ��b�Z�[�W�n���h��
protected:
	virtual void onActivate(UINT state, HWND previousWindow, bool minimized) {}				// WM_ACTIVATE
	virtual void onClose() {destroyWindow();}												// WM_CLOSE
	virtual bool onCommand(WORD id, WORD notifyCode, HWND controlWindow) {return false;}	// WM_COMMAND
	virtual bool onContextMenu(HWND window, const POINT& pt) {return false;}				// WM_CONTEXTMENU
	virtual int	 onCreate(const CREATESTRUCT& cs) {return 0;}								// WM_CREATE
	virtual void onDestroy() {}																// WM_DESTROY
	virtual void onDrawItem(UINT controlID, LPDRAWITEMSTRUCT drawItem) {}					// WM_DRAWITEM
	virtual void onKillFocus(HWND newWindow) {}												// WM_KILLFOCUS
	virtual void onLButtonDown(UINT flags, const POINT& pt) {}								// WM_LBUTTONDOWN
	virtual void onLButtonDblClk(UINT flags, const POINT& pt) {}							// WM_LBUTTONDBLCLK
	virtual void onLButtonUp(UINT flags, const POINT& pt) {}								// WM_LBUTTONUP
	virtual void onMeasureItem(UINT controlID, LPMEASUREITEMSTRUCT mis) {}					// WM_MEASUREITEM
	virtual void onMouseMove(UINT flags, const POINT& pt) {}								// WM_MOUSEMOVE
	virtual bool onNotify(int controlID, LPNMHDR nmhdr) {return false;}						// WM_NOTIFY
	virtual bool onSetCursor(HWND window, UINT hitTest, UINT message) {return false;}		// WM_SETCURSOR
	virtual void onSetFocus(HWND oldWindow) {}												// WM_SETFOCUS
	virtual void onShowWindow(bool show, UINT status) {}									// WM_SHOWWINDOW
	virtual void onSize(UINT type, int cx, int cy) {}										// WM_SIZE
	virtual void onSysColorChange() {}														// WM_SYSCOLORCHANGE
	virtual void onTimer(UINT eventID) {}													// WM_TIMER

protected:
	virtual LRESULT	dispatchEvent(UINT message, WPARAM wParam, LPARAM lParam);
	virtual bool	preTranslateMessage(UINT message, WPARAM wParam, LPARAM lParam);	// called by only DE

	// �f�o�b�O
protected:
#ifdef _DEBUG
#	define assertValidAsWindow() assert(isWindow())
#else
#	define assertValidAsWindow()
#endif /* _DEBUG */

	friend class Subclassable;
};


#ifdef SubclassWindow
#undef SubclassWindow
#endif /* SubclassWindow */

template<class ConcreteWindow = Window> class Subclassable : public ConcreteWindow, public Noncopyable {
public:
	explicit Subclassable(HWND handle = 0) : ConcreteWindow(handle), originalProcedure_(0) {}
	virtual ~Subclassable() {if(isWindow()) unsubclassWindow();}
public:
	virtual bool	attach(HWND handle);
	bool			attach(HWND handle, bool subclass);
	virtual LRESULT	defWindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	virtual HWND	detach();
	virtual LRESULT	dispatchEvent(UINT message, WPARAM wParam, LPARAM lParam);
	bool			isSubclassed() const {return originalProcedure_ != 0;}
	bool			subclassWindow();
	bool			unsubclassWindow();
protected:
	virtual void	onDestroy();
	static LRESULT CALLBACK	windowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
private:
	WNDPROC originalProcedure_;
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
	bool	animateWindow(DWORD time, DWORD flags, bool catchError = true);
	bool	setLayeredWindowAttributes(COLORREF keyColor, BYTE alpha, DWORD flags);
	bool	updateLayeredWindow(HDC destDC, POINT* destPt,
				SIZE* size, HDC srcDC, POINT* srcPt, COLORREF keyColor,
				BLENDFUNCTION* blendFunction, DWORD flags);
private:
	typedef BOOL(WINAPI *AW)(HWND, DWORD, DWORD);
	typedef BOOL(WINAPI *SLWA)(HWND, COLORREF, BYTE, DWORD);
	typedef BOOL(WINAPI *ULW)(HWND, HDC, POINT*, SIZE*, HDC, POINT*, COLORREF, BLENDFUNCTION*, DWORD);
private:
	HMODULE	user32Dll_;	// loaded dynamically to use layered window APIs
	AW		animateWindowProc_;
	SLWA	setLayeredWindowAttributesProc_;
	ULW		updateLayeredWindowProc_;
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
	virtual bool create(HWND parent, const RECT& rect = DefaultWindowRect(),
			const TCHAR* windowName = 0, INT_PTR id = 0, DWORD style = 0, DWORD exStyle = 0) {
		return Window::create(Control::getClassName(), parent, rect, windowName,
			DefaultStyles::getStyle(style), DefaultStyles::getExStyle(exStyle), reinterpret_cast<HMENU>(id));
	}
	enum {DefaultStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS};
protected:
	// implementation helpers
	template<class ReturnType>
	ReturnType sendMessageR(UINT message, WPARAM wParam = 0, LPARAM lParam = 0) {
#pragma warning(disable : 4800)	// non-boolean to boolean conversion
		return static_cast<ReturnType>(sendMessage(message, wParam, lParam));
#pragma warning(default : 4800)
	}
	template<class ReturnType>
	ReturnType sendMessageC(UINT message, WPARAM wParam = 0, LPARAM lParam = 0) const {
		return const_cast<StandardControl*>(this)->sendMessageR<ReturnType>(message, wParam, lParam);
	}
};


// Control must define one static method:
// void GetClass(GET_CLASS_PARAM_LIST)
#define GET_CLASS_PARAM_LIST	const TCHAR*& name,									\
	HINSTANCE& instance, UINT& style, manah::windows::BrushHandleOrColor& bgColor,	\
	manah::windows::CursorHandleOrID& cursor,	HICON& icon, HICON& smallIcon, int& clsExtraBytes, int& wndExtraBytes
#define DEFINE_WINDOW_CLASS()	public: static void getClass(GET_CLASS_PARAM_LIST)
template<class Control, class BaseWindow = Window> class CustomControl : public BaseWindow {
public:
	explicit CustomControl(HWND handle = 0) : BaseWindow(handle) {}
	CustomControl(const CustomControl<Control, BaseWindow>& rhs) : BaseWindow(rhs) {}
	virtual ~CustomControl();
	bool	create(HWND parent, const RECT& rect = DefaultWindowRect(),
				const TCHAR* windowName = 0, DWORD style = 0UL, DWORD exStyle = 0UL);
protected:
	static LRESULT CALLBACK	windowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
	virtual void	onPaint(gdi::PaintDC& dc) = 0;	// WM_PAINT
};


// write one of these in your class definition if override DispatchEvent.
#define OVERRIDE_DISPATCH_EVENT(Control)								\
	LRESULT	dispatchEvent(UINT message, WPARAM wParam, LPARAM lParam);	\
	friend class manah::windows::ui::CustomControl<Control>
#define OVERRIDE_DISPATCH_EVENT_(Control, BaseWindow)					\
	LRESULT	dispatchEvent(UINT message, WPARAM wParam, LPARAM lParam);	\
	friend class manah::windows::ui::CustomControl<Control, BaseWindow>


// Window ///////////////////////////////////////////////////////////////////

inline Window::Window(HWND handle /* = 0 */) : HandleHolder<HWND>(handle) {}

inline Window::Window(const Window& rhs) : HandleHolder<HWND>(rhs) {}

inline Window::~Window() {if(isWindow() && !isAttached()) destroyWindow();}

inline UINT Window::arrangeIconicWindows() {return ::ArrangeIconicWindows(get());}

inline bool Window::attach(HWND handle) {
	if(!toBoolean(::IsWindow(handle))) throw HandleHolder<HWND>::InvalidHandleException(); return HandleHolder<HWND>::attach(handle);}

inline gdi::PaintDC Window::beginPaint(PAINTSTRUCT& paint) {
	assertValidAsWindow();
	::BeginPaint(get(), &paint);
	return gdi::PaintDC(get(), paint);
}

inline void Window::bringWindowToTop() {assertValidAsWindow(); ::BringWindowToTop(get());}

inline void Window::centerWindow(HWND alternateWindow /* = 0 */) {
	assertValidAsWindow();

	RECT winRect, altRect;

	if(alternateWindow == 0){
		alternateWindow = getParent();
		if(alternateWindow == 0)
			alternateWindow = ::GetDesktopWindow();
	}
	assert(toBoolean(::IsWindow(alternateWindow)));

	getWindowRect(winRect);
	::GetWindowRect(alternateWindow, &altRect);
	setWindowPos(0, (altRect.right - altRect.left) / 2 - (winRect.right - winRect.left) / 2,
		(altRect.bottom - altRect.top) / 2 - (winRect.bottom - winRect.top) / 2,
		0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
}

inline bool Window::changeClipboardChain(HWND newNext) {
	assertValidAsWindow(); return toBoolean(::ChangeClipboardChain(get(), newNext));}

inline Window Window::childWindowFromPoint(const POINT& pt) const {
	assertValidAsWindow(); return Window(::ChildWindowFromPoint(get(), pt));}

inline Window Window::childWindowFromPointEx(const POINT& pt, UINT flags) const {
	assertValidAsWindow(); return Window(::ChildWindowFromPointEx(get(), pt, flags));}

inline void Window::clientToScreen(RECT& rect) const {
	assertValidAsWindow();

	POINT point[2];

	point[0].x = rect.left;
	point[0].y = rect.top;
	point[1].x = rect.right;
	point[1].y = rect.bottom;
	::ClientToScreen(get(), &point[0]);
	::ClientToScreen(get(), &point[1]);
	::SetRect(&rect, point[0].x, point[0].y, point[1].x, point[1].y);
}

inline void Window::clientToScreen(POINT& pt) const {
	assertValidAsWindow(); ::ClientToScreen(get(), &pt);}

inline bool Window::create(const TCHAR* className, HWND parentOrHInstance,
		const RECT& rect /* = DefaultWindowRect() */, const TCHAR* windowName /* = 0 */,
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

	if(HWND handle = ::CreateWindowEx(exStyle,
			className, windowName, style, rect.left, rect.top,
			rect.right - rect.left, rect.bottom - rect.top, parent, menu, instance, param)) {
		setHandle(handle);
		return true;
	}
	return false;
}

inline bool Window::createCaret(HBITMAP bitmap, int width, int height) {
	assertValidAsWindow(); return toBoolean(::CreateCaret(get(), bitmap, width, height));}

inline bool Window::createGrayCaret(int width, int height) {return createCaret(reinterpret_cast<HBITMAP>(1), width, height);}

inline bool Window::createSolidCaret(int width, int height) {return createCaret(0, width, height);}

inline LRESULT Window::defWindowProc(UINT message, WPARAM wParam, LPARAM lParam) {
	assertValidAsWindow(); return ::DefWindowProc(get(), message, wParam, lParam);}

inline bool Window::destroyWindow() {return isWindow() ? toBoolean(::DestroyWindow(get())) : false;}

inline LRESULT Window::dispatchEvent(UINT message, WPARAM wParam, LPARAM lParam) {
	if(preTranslateMessage(message, wParam, lParam))
		return false;

	POINT pt;

	switch(message) {
	case WM_ACTIVATE:
		onActivate(LOWORD(wParam), reinterpret_cast<HWND>(lParam), toBoolean(HIWORD(wParam)));
		break;
	case WM_CLOSE:
		onClose();
		return 0L;
	case WM_COMMAND:
		return onCommand(LOWORD(wParam), HIWORD(wParam), reinterpret_cast<HWND>(lParam));
	case WM_CONTEXTMENU:
		pt.x = LOWORD(lParam);
		pt.y = HIWORD(lParam);
		if(onContextMenu(reinterpret_cast<HWND>(wParam), pt))
			return 0L;
		break;
	case WM_CREATE:
		return onCreate(*reinterpret_cast<CREATESTRUCT*>(lParam));
	case WM_DESTROY:
		onDestroy();
		return 0L;
	case WM_DRAWITEM:
		onDrawItem(static_cast<UINT>(wParam), reinterpret_cast<LPDRAWITEMSTRUCT>(lParam));
		break;
	case WM_KILLFOCUS:
		onKillFocus(reinterpret_cast<HWND>(wParam));
		return 0L;
	case WM_LBUTTONDOWN:
		pt.x = LOWORD(lParam);
		pt.y = HIWORD(lParam);
		onLButtonDown(static_cast<UINT>(wParam), pt);
		break;
	case WM_LBUTTONDBLCLK:
		pt.x = LOWORD(lParam);
		pt.y = HIWORD(lParam);
		onLButtonDblClk(static_cast<UINT>(wParam), pt);
		break;
	case WM_LBUTTONUP:
		pt.x = LOWORD(lParam);
		pt.y = HIWORD(lParam);
		onLButtonUp(static_cast<UINT>(wParam), pt);
		break;
	case WM_MEASUREITEM:
		onMeasureItem(static_cast<UINT>(wParam), reinterpret_cast<LPMEASUREITEMSTRUCT>(lParam));
		break;
	case WM_MOUSEMOVE:
		pt.x = LOWORD(lParam);
		pt.y = HIWORD(lParam);
		onMouseMove(static_cast<UINT>(wParam), pt);
		break;
	case WM_NOTIFY:
		return onNotify(static_cast<UINT>(wParam), reinterpret_cast<LPNMHDR>(lParam));
	case WM_SETCURSOR:
		if(onSetCursor(reinterpret_cast<HWND>(wParam),
			static_cast<UINT>(LOWORD(lParam)), static_cast<UINT>(HIWORD(lParam))))
			return false;
		break;
	case WM_SETFOCUS:
		onSetFocus(reinterpret_cast<HWND>(wParam));
		return 0L;
	case WM_SHOWWINDOW:
		onShowWindow(toBoolean(wParam), static_cast<UINT>(lParam));
		return 0L;
	case WM_SIZE:
		onSize(static_cast<UINT>(wParam), LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_SYSCOLORCHANGE:
		onSysColorChange();
		break;
	case WM_TIMER:
		onTimer(static_cast<UINT>(wParam));
		break;
	default:
		break;
	}

	return defWindowProc(message, wParam, lParam);
}

inline void Window::dragAcceptFiles(bool accept /* = true */) {assertValidAsWindow(); ::DragAcceptFiles(get(), accept);}

inline void Window::drawMenuBar() {assertValidAsWindow(); ::DrawMenuBar(get());}

inline bool Window::enableScrollBar(int barFlags, UINT arrowFlags /* = ESB_ENABLE_BOTH */) {
	assertValidAsWindow(); return toBoolean(::EnableScrollBar(get(), barFlags, arrowFlags));}

inline bool Window::enableWindow(bool enable /* = true */) {assertValidAsWindow(); return toBoolean(::EnableWindow(get(), enable));}

inline void Window::endPaint(const PAINTSTRUCT& paint) {assertValidAsWindow(); ::EndPaint(get(), &paint);}

inline int Window::enumProps(PROPENUMPROC enumFunction) {assertValidAsWindow(); return ::EnumProps(get(), enumFunction);}

inline int Window::enumPropsEx(PROPENUMPROCEX enumFunction, LPARAM param) {assertValidAsWindow(); return ::EnumPropsEx(get(), enumFunction, param);}

inline Window Window::findWindow(const TCHAR* className, const TCHAR* windowName) {return Window(::FindWindow(className, windowName));}

inline bool Window::flashWindow(bool invert) {assertValidAsWindow(); return toBoolean(::FlashWindow(get(), invert));}

inline Window Window::getActiveWindow() {return Window(::GetActiveWindow());}

inline Window Window::getCapture() {return Window(::GetCapture());}

inline POINT Window::getCaretPos() {POINT pt; ::GetCaretPos(&pt); return pt;}

inline DWORD Window::getClassLong(int index) const {assertValidAsWindow(); return ::GetClassLong(get(), index);}

#ifdef _WIN64
inline ULONG_PTR Window::getClassLongPtr(int index) const {assertValidAsWindow(); return ::GetClassLongPtr(get(), index);}
#endif /* _WIN64 */

inline int Window::getClassName(TCHAR* className, int maxLength) const {
	assertValidAsWindow(); return ::GetClassName(get(), className, maxLength);}

inline void Window::getClientRect(RECT& rect) const {assertValidAsWindow(); ::GetClientRect(get(), &rect);}

inline POINT Window::getCursorPos() const {POINT pt; ::GetCursorPos(&pt); screenToClient(pt); return pt;}

inline gdi::ClientDC Window::getDC() {assertValidAsWindow(); return gdi::ClientDC(get());}

inline gdi::ClientDC Window::getDCEx(HRGN clipRegion, DWORD flags) {assertValidAsWindow(); return gdi::ClientDC(get(), clipRegion, flags);}

inline Window Window::getDesktopWindow() {return Window(::GetDesktopWindow());}

inline int Window::getDlgCtrlID() const {assertValidAsWindow(); return ::GetDlgCtrlID(get());}

inline DWORD Window::getExStyle() const {assertValidAsWindow(); return static_cast<DWORD>(getWindowLong(GWL_EXSTYLE));}

inline Window Window::getFocus() {return Window(::GetFocus());}

inline HFONT Window::getFont() const {return reinterpret_cast<HFONT>(::SendMessage(get(), WM_GETFONT, 0, 0L));}

inline Window Window::getForegroundWindow() {return Window(::GetForegroundWindow());}

inline DWORD Window::getHotKey() const {return static_cast<DWORD>(::SendMessage(get(), WM_GETHOTKEY, 0, 0L));}

inline HICON Window::getIcon(bool bigIcon /* = true */) const {
	return reinterpret_cast<HICON>(::SendMessage(get(), WM_GETICON, bigIcon ? ICON_BIG : ICON_SMALL, 0L));}

inline Window Window::getLastActivePopup() const {assertValidAsWindow(); return Window(::GetLastActivePopup(get()));}

inline HMENU Window::getMenu() const {assertValidAsWindow(); return ::GetMenu(get());}

inline Window Window::getNext(UINT flag /* = GW_HWNDNEXT */) const {assertValidAsWindow(); return Window(::GetNextWindow(get(), flag));}

inline Window Window::getOwner() const {assertValidAsWindow(); return Window(::GetWindow(get(), GW_OWNER));}

inline Window Window::getParent() const {assertValidAsWindow(); return Window(::GetParent(get()));}

inline HANDLE Window::getProp(const TCHAR* identifier) const {assertValidAsWindow(); return ::GetProp(get(), identifier);}

inline HANDLE Window::getProp(ATOM identifier) const {return getProp(reinterpret_cast<const TCHAR*>(identifier));}

inline bool Window::getScrollInfo(int bar, SCROLLINFO& scrollInfo, UINT mask /* = SIF_ALL */) const {
	assertValidAsWindow();
	scrollInfo.cbSize = sizeof(SCROLLINFO);
	scrollInfo.fMask = mask;
	return toBoolean(::GetScrollInfo(get(), bar, &scrollInfo));
}

inline int Window::getScrollLimit(int bar) const {
	assertValidAsWindow();
	int dummy, limit;
	getScrollRange(bar, dummy, limit);
	return limit;
}

inline int Window::getScrollPos(int bar) const {assertValidAsWindow(); return ::GetScrollPos(get(), bar);}

inline void Window::getScrollRange(int bar, int& minPos, int& maxPos) const {
	assertValidAsWindow(); ::GetScrollRange(get(), bar, &minPos, &maxPos);}

inline int Window::getScrollTrackPos(int bar) const {
	assertValidAsWindow();
	SCROLLINFO si;
	return (getScrollInfo(bar, si, SIF_TRACKPOS)) ? si.nTrackPos : -1;
}

inline DWORD Window::getStyle() const {return static_cast<DWORD>(getWindowLong(GWL_STYLE));}

inline HMENU Window::getSystemMenu(bool revert) const {assertValidAsWindow(); return ::GetSystemMenu(get(), revert);}

inline Window Window::getTopWindow() const {assertValidAsWindow(); return Window(::GetTopWindow(get()));}
inline bool Window::getUpdateRect(RECT& rect, bool erase /* = false */) {
	assertValidAsWindow(); return toBoolean(::GetUpdateRect(get(), &rect, erase));}

inline int Window::getUpdateRgn(HRGN region, bool erase /* = false */) {
	assertValidAsWindow(); return ::GetUpdateRgn(get(), region, erase);}

inline Window Window::getWindow(UINT command) const {assertValidAsWindow(); return Window(::GetWindow(get(), command));}

inline DWORD Window::getWindowContextHelpID() const {assertValidAsWindow(); return ::GetWindowContextHelpId(get());}

inline gdi::WindowDC Window::getWindowDC() {assertValidAsWindow(); return gdi::WindowDC(get());}

inline LONG Window::getWindowLong(int index) const {assertValidAsWindow(); return ::GetWindowLong(get(), index);}

#ifdef _WIN64
inline LONG_PTR Window::getWindowLongPtr(int index) const {assertValidAsWindow(); return ::GetWindowLongPtr(get(), index);}
#endif /* _WIN64 */

inline bool Window::getWindowPlacement(WINDOWPLACEMENT& placement) const {
	assertValidAsWindow(); return toBoolean(::GetWindowPlacement(get(), &placement));}

inline DWORD Window::getWindowProcessID() const {
	assertValidAsWindow();
	DWORD id;
	::GetWindowThreadProcessId(get(), &id);
	return id;
}

inline void Window::getWindowRect(RECT& rect) const {assertValidAsWindow(); ::GetWindowRect(get(), &rect);}

inline int Window::getWindowRgn(HRGN region) const {assertValidAsWindow(); return ::GetWindowRgn(get(), region);}

inline int Window::getWindowText(TCHAR* text, int maxLength) const {
	assertValidAsWindow(); return ::GetWindowText(get(), text, maxLength);}

inline std::basic_string<TCHAR> Window::getWindowText() const {
	const int len = getWindowTextLength();
	TCHAR* buffer = new TCHAR[len + 1];
	getWindowText(buffer, len + 1);
	std::basic_string<TCHAR> temp(buffer);
	delete[] buffer;
	return temp;
}

inline int Window::getWindowTextLength() const {assertValidAsWindow(); return ::GetWindowTextLength(get());}

inline DWORD Window::getWindowThreadID() const {assertValidAsWindow(); return ::GetWindowThreadProcessId(get(), 0);}

inline bool Window::hasFocus() const {return ::GetFocus() == get();}

inline void Window::hideCaret() {assertValidAsWindow(); ::HideCaret(get());}

inline bool Window::hiliteMenuItem(HMENU menu, UINT item, UINT flags) {
	assertValidAsWindow(); return toBoolean(::HiliteMenuItem(get(), menu, item, flags));}

inline void Window::invalidate(bool erase /* = true */) {assertValidAsWindow(); ::InvalidateRect(get(), 0, erase);}

inline void Window::invalidateRect(const RECT* rect, bool erase /* = true */) {
	assertValidAsWindow(); ::InvalidateRect(get(), rect, erase);}

inline void Window::invalidateRgn(HRGN region, bool erase /* = true */) {assertValidAsWindow(); ::InvalidateRgn(get(), region, erase);}

inline bool Window::isChild(HWND window) const {assertValidAsWindow(); return toBoolean(::IsChild(get(), window));}

inline bool Window::isIconic() const {assertValidAsWindow(); return toBoolean(::IsIconic(get()));}

inline bool Window::isWindow() const {return toBoolean(::IsWindow(get()));}

inline bool Window::isWindowEnabled() const {assertValidAsWindow(); return toBoolean(::IsWindowEnabled(get()));}

inline bool Window::isWindowUnicode() const {assertValidAsWindow(); return toBoolean(::IsWindowUnicode(get()));}

inline bool Window::isWindowVisible() const {return toBoolean(::IsWindowVisible(get()));}

inline bool Window::isZoomed() const {assertValidAsWindow(); return toBoolean(::IsZoomed(get()));}

inline bool Window::killTimer(UINT eventID) {assertValidAsWindow(); return toBoolean(::KillTimer(get(), eventID));}

inline bool Window::lockWindowUpdate() {assertValidAsWindow(); return toBoolean(::LockWindowUpdate(get()));}

inline void Window::mapWindowPoints(HWND destWindow, RECT& rect) const {
	assertValidAsWindow();
	POINT point[2];
	point[0].x = rect.left;
	point[0].y = rect.top;
	point[1].x = rect.right;
	point[1].y = rect.bottom;
	::MapWindowPoints(get(), destWindow, point, 2);
	::SetRect(&rect, point[0].x, point[0].y, point[1].x, point[1].y);
}

inline void Window::mapWindowPoints(HWND destWindow, POINT points[], UINT count) const {
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

inline void Window::moveWindow(int x, int y, int width, int height, bool repaint /* = true */) {
	assertValidAsWindow(); ::MoveWindow(get(), x, y, width, height, repaint);}

inline void Window::print(HDC dc, DWORD flags) const {::SendMessage(get(), WM_PRINT, reinterpret_cast<WPARAM>(dc), flags);}

inline void Window::printClient(HDC dc, DWORD flags) const {::SendMessage(get(), WM_PRINTCLIENT, reinterpret_cast<WPARAM>(dc), flags);}

inline void Window::moveWindow(const RECT& rect, bool repaint /* = true */) {
	assertValidAsWindow(); ::MoveWindow(get(), rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, repaint);
}

inline LRESULT Window::postMessage(UINT message, WPARAM wParam /* = 0 */, LPARAM lParam /* = 0L */) {
	assertValidAsWindow(); return ::PostMessage(get(), message, wParam, lParam);}

inline bool Window::preTranslateMessage(UINT message, WPARAM wParam, LPARAM lParam) {return false;}

inline bool Window::redrawWindow(LPRECT updateRect /* = 0 */, HRGN clipRegion/* = 0 */,
		UINT flags /* = RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE */) {
	assertValidAsWindow(); return toBoolean(::RedrawWindow(get(), updateRect, clipRegion, flags));}

inline HRESULT Window::registerDragDrop(IDropTarget& dropTarget) {
	assertValidAsWindow(); return ::RegisterDragDrop(get(), &dropTarget);}

inline bool Window::releaseCapture() {return toBoolean(::ReleaseCapture());}

inline int Window::releaseDC(HDC dc) {assertValidAsWindow(); return ::ReleaseDC(get(), dc);}

inline HANDLE Window::removeProp(const TCHAR* identifier) {assertValidAsWindow(); return ::RemoveProp(get(), identifier);}

inline HANDLE Window::removeProp(ATOM identifier) {return removeProp(reinterpret_cast<const TCHAR*>(identifier));}

inline HRESULT Window::revokeDragDrop() {assertValidAsWindow(); return ::RevokeDragDrop(get());}

inline void Window::screenToClient(RECT& rect) const {
	assertValidAsWindow();
	POINT point[2];
	point[0].x = rect.left;
	point[0].y = rect.top;
	point[1].x = rect.right;
	point[1].y = rect.bottom;
	::ScreenToClient(get(), &point[0]);
	::ScreenToClient(get(), &point[1]);
	::SetRect(&rect, point[0].x, point[0].y, point[1].x, point[1].y);
}

inline void Window::scrollWindow(int xAmount, int yAmount, LPRECT rect /* = 0 */, LPRECT clipRect /* = 0 */) {
	assertValidAsWindow(); ::ScrollWindow(get(), xAmount, yAmount, rect, clipRect);}

inline int Window::scrollWindowEx(
		int dx, int dy, LPRECT scrollRect, LPRECT clipRect, HRGN updateRegion, LPRECT updateRect, UINT flags) {
	assertValidAsWindow(); return ::ScrollWindowEx(get(), dx, dy, scrollRect, clipRect, updateRegion, updateRect, flags);}

inline LRESULT Window::sendMessage(UINT message, WPARAM wParam /* = 0 */, LPARAM lParam /* = 0L */) {
	assertValidAsWindow(); return ::SendMessage(get(), message, wParam, lParam);}

inline bool Window::sendNotifyMessage(UINT message, WPARAM wParam, LPARAM lParam) {
	assertValidAsWindow(); return toBoolean(::SendNotifyMessage(get(), message, wParam, lParam));}

inline void Window::screenToClient(POINT& pt) const {assertValidAsWindow(); ::ScreenToClient(get(), &pt);}

inline Window Window::setActiveWindow() {assertValidAsWindow(); return Window(::SetActiveWindow(get()));}

inline Window Window::setCapture() {assertValidAsWindow(); return Window(::SetCapture(get()));}

inline void Window::setCaretPos(const POINT& pt) {::SetCaretPos(pt.x, pt.y);}

inline DWORD Window::setClassLong(int index, DWORD newLong) {assertValidAsWindow(); return ::SetClassLong(get(), index, newLong);}

#ifdef _WIN64
inline ULONG_PTR Window::setClassLongPtr(int index, ULONG_PTR newLong) {
	assertValidAsWindow(); return ::SetClassLongPtr(get(), index, newLong);}
#endif /* _WIN64 */

inline Window Window::setClipboardViewer() {assertValidAsWindow(); return Window(::SetClipboardViewer(get()));}

inline bool Window::setCursorPos(const POINT& pt) {POINT p(pt); clientToScreen(p); return toBoolean(::SetCursorPos(p.x, p.y));}

inline int Window::setDlgCtrlID(int id) {assertValidAsWindow(); return static_cast<int>(setWindowLong(GWL_ID, id));}

inline Window Window::setFocus() {assertValidAsWindow(); return Window(::SetFocus(get()));}

inline void Window::setFont(HFONT font, bool redraw /* = true */) {
	sendMessage(WM_SETFONT, reinterpret_cast<WPARAM>(font), MAKELPARAM(redraw, 0));}

inline bool Window::setForegroundWindow() {assertValidAsWindow(); return toBoolean(::SetForegroundWindow(get()));}

inline int Window::setHotKey(WORD virtualKeyCode, WORD modifiers) {
	assertValidAsWindow(); return static_cast<int>(sendMessage(WM_SETHOTKEY, MAKEWPARAM(virtualKeyCode, modifiers)));}

inline HICON Window::setIcon(HICON icon, bool bigIcon /* = true */) {
	return reinterpret_cast<HICON>(sendMessage(WM_SETICON, bigIcon ? ICON_BIG : ICON_SMALL));}

inline bool Window::setMenu(HMENU menu) {assertValidAsWindow(); return toBoolean(::SetMenu(get(), menu));}

inline Window Window::setParent(HWND newParent) {assertValidAsWindow(); return Window(::SetParent(get(), newParent));}

inline bool Window::setProp(const TCHAR* identifier, HANDLE data) {
	assertValidAsWindow(); return toBoolean(::SetProp(get(), identifier, data));}

inline bool Window::setProp(ATOM identifier, HANDLE data) {return setProp(reinterpret_cast<const TCHAR*>(identifier), data);}

inline void Window::setRedraw(bool redraw /* = true */) {sendMessage(WM_SETREDRAW, redraw);}

inline bool Window::setScrollInfo(int bar, const SCROLLINFO& scrollInfo, bool redraw /* = true */) {
	assertValidAsWindow(); return toBoolean(::SetScrollInfo(get(), bar, &scrollInfo, redraw));}

inline int Window::setScrollPos(int bar, int pos, bool redraw /* = true */) {
	assertValidAsWindow(); return ::SetScrollPos(get(), bar, pos, redraw);}

inline void Window::setScrollRange(int bar, int minPos, int maxPos, bool redraw /* = true */) {
	assertValidAsWindow(); ::SetScrollRange(get(), bar, minPos, maxPos, redraw);}

inline UINT_PTR Window::setTimer(
		UINT_PTR eventID, UINT elapse, void (CALLBACK* procedure)(HWND, UINT, UINT_PTR, DWORD) /* = 0 */) {
	assertValidAsWindow(); return static_cast<UINT_PTR>(::SetTimer(get(), eventID, elapse, procedure));}

inline bool Window::setWindowContextHelpID(DWORD contextHelpID) {
	assertValidAsWindow(); return toBoolean(::SetWindowContextHelpId(get(), contextHelpID));}

inline LONG Window::setWindowLong(int index, LONG newLong) {
	assertValidAsWindow(); return ::SetWindowLong(get(), index, newLong);}

#ifdef _WIN64
inline LONG_PTR Window::setWindowLongPtr(int index, LONG_PTR newLong) {
	assertValidAsWindow(); return ::SetWindowLongPtr(get(), index, newLong);}
#endif /* _WIN64 */

inline bool Window::setWindowPlacement(const WINDOWPLACEMENT& placement) {
	assertValidAsWindow(); return toBoolean(::SetWindowPlacement(get(), &placement));
}

inline bool Window::setWindowPos(HWND windowInsertAfter, int x, int y, int cx, int cy, UINT flags) {
	assertValidAsWindow(); return toBoolean(::SetWindowPos(get(), windowInsertAfter, x, y, cx, cy, flags));}

inline bool Window::setWindowPos(HWND windowInsertAfter, const RECT& rect, UINT flags) {
	return setWindowPos(windowInsertAfter, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, flags);}

inline int Window::setWindowRgn(const HRGN region, bool redraw /* = true */) {
	assertValidAsWindow(); return ::SetWindowRgn(get(), region, redraw);}

inline void Window::setWindowText(const TCHAR* text) {assertValidAsWindow(); ::SetWindowText(get(), text);}

inline void Window::showCaret() {assertValidAsWindow(); ::ShowCaret(get());}

inline void Window::showOwnedPopups(bool show /* = true */) {assertValidAsWindow(); ::ShowOwnedPopups(get(), show);}

inline void Window::showScrollBar(int bar, bool show /* = true */) {assertValidAsWindow(); ::ShowScrollBar(get(), bar, show);}

inline bool Window::showWindow(UINT command) {assertValidAsWindow(); return toBoolean(::ShowWindow(get(), command));}

inline void Window::unlockWindowUpdate() {assertValidAsWindow(); ::LockWindowUpdate(0);}

inline void Window::updateWindow() {assertValidAsWindow(); ::UpdateWindow(get());}

inline void Window::validateRect(const RECT* rect) {assertValidAsWindow(); ::ValidateRect(get(), rect);}

inline void Window::validateRgn(HRGN region) {assertValidAsWindow(); ::ValidateRgn(get(), region);}

inline Window Window::windowFromPoint(const POINT& pt) {return Window(::WindowFromPoint(pt));}

inline bool Window::winHelp(const TCHAR* help, UINT command /* = HELP_CONTEXT */, DWORD data /* = 0 */) {
	return toBoolean(::WinHelp(get(), help, command, data));}


// Subclassable /////////////////////////////////////////////////////////////

template<class ConcreteWindow> inline bool Subclassable<ConcreteWindow>::attach(HWND handle) {return ConcreteWindow::attach(handle);}

template<class ConcreteWindow> inline bool Subclassable<ConcreteWindow>::attach(HWND handle, bool subclass) {
	if(subclass)	return attach(window) ? subclassWindow() : false;
	else			return attach(window);
}

template<class ConcreteWindow> inline LRESULT Subclassable<ConcreteWindow>::defWindowProc(UINT message, WPARAM wParam, LPARAM lParam) {
	assertValidAsWindow();
	return (originalProcedure_ != 0) ?
		::CallWindowProc(originalProcedure_, get(), message, wParam, lParam)
		: ::DefWindowProc(get(), message, wParam, lParam);
}

template<class ConcreteWindow> inline HWND Subclassable<ConcreteWindow>::detach() {
	if(isAttached())
		unsubclassWindow();
	return ConcreteWindow::detach();
}

template<class ConcreteWindow> inline LRESULT Subclassable<ConcreteWindow>::dispatchEvent(UINT message, WPARAM wParam, LPARAM lParam) {
	if(message == WM_NCDESTROY) {
		unsubclassWindow();
		return 0L;
	}
	return ConcreteWindow::dispatchEvent(message, wParam, lParam);
}

template<class ConcreteWindow> inline void Subclassable<ConcreteWindow>::onDestroy() {unsubclassWindow();}

template<class ConcreteWindow> inline bool Subclassable<ConcreteWindow>::subclassWindow() {
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

template<class ConcreteWindow> inline bool Subclassable<ConcreteWindow>::unsubclassWindow() {
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
		return instance->dispatchEvent(message, wParam, lParam);
	return false;
}


// Layered //////////////////////////////////////////////////////////////////

template<class ConcreteWindow>
inline bool Layered<ConcreteWindow>::animateWindow(DWORD time, DWORD flags, bool catchError /* = true */) {
	assertValidAsWindow();
	if(user32Dll_ == 0)
		user32Dll_ = ::LoadLibrary(_T("User32.dll"));
	if(user32Dll_ != 0 && animateWindowProc_ == 0)
		animateWindowProc_ = reinterpret_cast<AW>(::GetProcAddress(user32Dll_, "AnimateWindow"));
	if((animateWindowProc_ != 0) ? toBoolean((*animateWindowProc_)(get(), time, flags)) : false)
		return true;
	else if(catchError) {
		if(toBoolean(flags & AW_HIDE))
			return showWindow(SW_HIDE);
		else
			return showWindow(toBoolean(flags & AW_ACTIVATE) ? SW_SHOW : SW_SHOWNA);
	}
	return false;
}

template<class ConcreteWindow>
inline bool Layered<ConcreteWindow>::setLayeredWindowAttributes(COLORREF keyColor, BYTE alpha, DWORD flags) {
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
inline bool Layered<ConcreteWindow>::updateLayeredWindow(HDC destDC, POINT* destPt,
		SIZE* size, HDC srcDC, POINT* srcPt, COLORREF keyColor, BLENDFUNCTION* blendFunction, DWORD flags) {
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
inline bool CustomControl<Control, BaseWindow>::create(HWND parent, const RECT& rect /* = DefaultWindowRect() */,
		const TCHAR* windowName /* = 0 */, DWORD style /* = 0UL */, DWORD exStyle /* = 0UL */) {
	BrushHandleOrColor	bgColor;
	CursorHandleOrID	cursor;
	AutoZeroCB<WNDCLASSEX>	wc, dummy;

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
	typedef CustomControl<Control, BaseWindow>	SelfType;
	if(message == WM_CREATE) {
		SelfType* instance = reinterpret_cast<SelfType*>(reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams);
		assert(instance != 0);
		instance->setHandle(window);	// ... the handle will be reset by BaseWindow::create (no problem)
#ifdef _WIN64
		instance->setWindowLongPtr(GWLP_USERDATA, reinterpret_cast<LONG_PTR>(instance));
#else
		instance->setWindowLong(GWL_USERDATA, static_cast<long>(reinterpret_cast<LONG_PTR>(instance)));
#endif /* _WIN64 */

		return instance->dispatchEvent(message, wParam, lParam);
	} else {
#ifdef _WIN64
		SelfType* instance = reinterpret_cast<SelfType*>(::GetWindowLongPtr(window, GWLP_USERDATA));
#else
		SelfType* instance = reinterpret_cast<SelfType*>(static_cast<LONG_PTR>(::GetWindowLong(window, GWL_USERDATA)));
#endif /* _WIN64 */
		if(message == WM_PAINT && instance != 0) {
			instance->onPaint(gdi::PaintDC(*instance));
			return true;
		} else
			return (instance != 0) ?
				instance->dispatchEvent(message, wParam, lParam) : ::DefWindowProc(window, message, wParam, lParam);
	}
}

}}} // namespace manah::windows::ui

#endif /* !MANAH_WINDOW_HPP */
