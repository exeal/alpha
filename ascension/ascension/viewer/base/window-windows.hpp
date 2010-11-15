/**
 * @file window-windows.hpp
 * @date 2002-2010
 * @date 2010-10-27 renamed from "window.hpp"
 */

#ifndef ASCENSION_WINDOW_WINDOWS_HPP
#define ASCENSION_WINDOW_WINDOWS_HPP

#include <ascension/viewer/base/window.hpp>
#include <ascension/graphics/graphics-windows.hpp>	// win32.PaintContext
//#include "menu.hpp"
#include <ascension/corelib/memory.hpp>	// AutoBuffer
#include <shellapi.h>	// DragAcceptFiles
#include <ole2.h>		// D&D
#include <imm.h>
#include <windowsx.h>
#include <string>

// these macros are defined by winuser.h
#ifndef MK_XBUTTON1
#	define MK_XBUTTON1 0x0020
#	define MK_XBUTTON2 0x0040
#endif // !MK_XBUTTON1
#ifndef WM_XBUTTONDOWN
#	define XBUTTON1 0x0001
#	define XBUTTON2 0x0002
#	define WM_XBUTTONDOWN 0x020b
#	define WM_XBUTTONUP 0x020c
#	define WM_XBUTTONDBLCLK 0x020d
#	define GET_KEYSTATE_WPARAM(wp) (LOWORD(wp))
#	define GET_XBUTTON_WPARAM(wp) (HIWORD(wp))
#endif // !WM_XBUTTONDOWN
#ifndef WM_MOUSEHWHEEL
#	define WM_MOUSEHWHEEL 0x020e
#endif // !WM_MOUSEHWHEEL
#ifndef GET_KEYSTATE_LPARAM
#	define GET_KEYSTATE_LPARAM(lp) (LOWORD(lp))
#endif // !GET_KEYSTATE_LPARAM

namespace ascension {
	namespace win32 {

		inline int inputModifiersFromNative(WPARAM wp) {
			int result = 0;
			if(boole(wp & MK_LBUTTON))
				result |= viewers::base::UserInput::BUTTON1_DOWN;
			if(boole(wp & MK_RBUTTON))
				result |= viewers::base::UserInput::BUTTON3_DOWN;
			if(boole(wp & MK_SHIFT))
				result |= viewers::base::UserInput::SHIFT_DOWN;
			if(boole(wp & MK_CONTROL))
				result |= viewers::base::UserInput::CONTROL_DOWN;
			if(boole(wp & MK_MBUTTON))
				result |= viewers::base::UserInput::BUTTON2_DOWN;
			if(boole(wp & MK_XBUTTON1))
				result |= viewers::base::UserInput::BUTTON4_DOWN;
			if(boole(wp & MK_XBUTTON2))
				result |= viewers::base::UserInput::BUTTON5_DOWN;
			return result;
		}

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

///
class WindowBase : public viewers::base::Window {
public:
	static const DWORD defaultWidgetStyle = WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE;
public:
	graphics::Point<> clientToScreen(const graphics::Point<>& p) const {
		POINT temp(toNative(p));
		if(!boole(::ClientToScreen(handle().get(), &temp)))
			throw PlatformDependentError<>();
		return graphics::fromNative(temp);
	}
	const Handle<HWND>& handle() const /*throw()*/ {return handle_;}
	void initialize(const Handle<HWND>& parent,
			const graphics::Point<>& position = graphics::Point<>(CW_USEDEFAULT, CW_USEDEFAULT),
			const graphics::Dimension<>& size = graphics::Dimension<>(CW_USEDEFAULT, CW_USEDEFAULT),
			DWORD style = 0, DWORD extendedStyle = 0) {
		if(handle().get() != 0)
			throw IllegalStateException("this object already has a window handle.");
		AutoZeroSize<WNDCLASSEXW> klass;
		const std::basic_string<WCHAR> className(provideClassName());
		if(!boole(::GetClassInfoExW(klass.hInstance = ::GetModuleHandle(0), className.c_str(), &klass))) {
			ClassInformation ci;
			provideClassInformation(ci);
			klass.style = ci.style;
			klass.lpfnWndProc = &windowProcedure;
			klass.hIcon = ci.icon.get();
			klass.hCursor = ci.cursor.get();
			klass.hbrBackground = ci.background.get();
			klass.lpszClassName = className.c_str();
			klass.hIconSm = ci.smallIcon.get();
			if(::RegisterClassExW(&klass) == 0)
				throw PlatformDependentError<>();
		}

		HWND borrowed = ::CreateWindowExW(extendedStyle, className.c_str(), 0,
			style, position.x, position.y, size.cx, size.cy, parent.get(), 0, 0, this);
		if(borrowed == 0)
			throw PlatformDependentError<>();
		assert(borrowed == handle().get());
		const WindowBase* const self = reinterpret_cast<WindowBase*>(
#ifdef _WIN64
			::GetWindowLongPtrW(borrowed, GWLP_USERDATA));
#else
			static_cast<LONG_PTR>(::GetWindowLongW(borrowed, GWL_USERDATA)));
#endif // _WIN64
		assert(self == this);
	}
	graphics::Point<> screenToClient(const graphics::Point<>& p) const {
		POINT temp(toNative(p));
		if(!boole(::ScreenToClient(handle().get(), &temp)))
			throw PlatformDependentError<>();
		return graphics::fromNative(temp);
	}
protected:
	struct ClassInformation {
		UINT style;	// corresponds to WNDCLASSEXW.style
		/// Makes a brush handle parameter from either a brush handle or @c COLORREF value. 
		class Background {
		public:
			/// Constructor makes @c null @c HBRUSH value.
			Background() /*throw()*/ : brush_(0) {}
			/// Constructor takes a brush handle.
			Background(Handle<HBRUSH> handle) /*throw()*/ : brush_(handle.release()) {}
			/// Constructor takes a @c COLORREF value used to make the brush handle.
			Background(int systemColor) /*throw()*/
				: brush_(reinterpret_cast<HBRUSH>(static_cast<HANDLE_PTR>(systemColor + 1))) {}
			/// Returns the brush handle.
			HBRUSH get() const /*throw()*/ {return brush_;}
		private:
			HBRUSH brush_;
		} background;
		Handle<HICON> icon, smallIcon;
		/// Makes a cursor handle parameter from either a cursor handle or numeric identifier.
		class CursorHandleOrID {
		public:
			/// Constructor makes @c null @c HCURSOR value.
			CursorHandleOrID() /*throw()*/ : cursor_(0) {}
			/// Constructor takes a cursor handle.
			CursorHandleOrID(const Handle<HCURSOR>& handle) /*throw()*/ : cursor_(handle.get()) {}
			/// Constructor takes a numeric identifier for system cursor.
			CursorHandleOrID(const WCHAR* systemCursorID) : cursor_(::LoadCursorW(0, systemCursorID)) {}
			/// Returns the cursor handle.
			HCURSOR get() const /*throw()*/ {return cursor_;}
		private:
			HCURSOR cursor_;
		} cursor;
		ClassInformation() : style(0) {}
	};
protected:
	// old code:
	// Do not override this directly. Use ASCENSION_WIN32_DECLEAR_WINDOW_MESSAGE_MAP familiy instead.
	virtual LRESULT processWindowMessage(UINT /* message */, WPARAM wp, LPARAM lp, bool& handled);
	/**
	 * @note If you override this, call from your derived class.
	 */
	virtual LRESULT preTranslateWindowMessage(UINT message, WPARAM wp, LPARAM lp, bool& handled) {return TRUE;}
	virtual void provideClassInformation(ClassInformation& classInfomation) const {}
	virtual std::basic_string<WCHAR> provideClassName() const = 0;
private:
	LRESULT processKeyInput(bool released, WPARAM wp, LPARAM lp, bool& consumed) {
		int modifiers;
		if(::GetKeyState(VK_SHIFT) < 0)
			modifiers |= viewers::base::UserInput::SHIFT_DOWN;
		if(::GetKeyState(VK_CONTROL) < 0)
			modifiers |= viewers::base::UserInput::CONTROL_DOWN;
		if(::GetKeyState(VK_MENU) < 0)
			modifiers |= viewers::base::UserInput::ALT_DOWN;
		const viewers::base::KeyInput input(viewers::base::keyboardCodeFromWin32(wp),
			modifiers, static_cast<int>(lp & 0xffffu), HIWORD(lp));
		consumed = !released ? keyReleased(input) : keyPressed(input);
		return consumed ? 0 : 1;
	}
	void processMouseDoubleClicked(viewers::base::UserInput::MouseButton button, WPARAM wpForModifiers, LPARAM lp, bool& consumed) {
		consumed = mouseDoubleClicked(viewers::base::MouseButtonInput(
			graphics::Point<>(GET_X_LPARAM(lp), GET_Y_LPARAM(lp)),
			button, inputModifiersFromNative(wpForModifiers)));
	}
	void processMousePressed(viewers::base::UserInput::MouseButton button, WPARAM wpForModifiers, LPARAM lp, bool& consumed) {
		consumed = mousePressed(viewers::base::MouseButtonInput(
			graphics::Point<>(GET_X_LPARAM(lp), GET_Y_LPARAM(lp)),
			button, inputModifiersFromNative(wpForModifiers)));
	}
	void processMouseReleased(viewers::base::UserInput::MouseButton button, WPARAM wpForModifiers, LPARAM lp, bool& consumed) {
		consumed = mouseReleased(viewers::base::MouseButtonInput(
			graphics::Point<>(GET_X_LPARAM(lp), GET_Y_LPARAM(lp)),
			button, inputModifiersFromNative(wpForModifiers)));
	}
	LRESULT processMouseWheelChanged(bool horizontal, WPARAM wp, LPARAM lp, bool& consumed) {
		consumed = mouseWheelChanged(viewers::base::MouseWheelInput(
			screenToClient(graphics::Point<>(GET_X_LPARAM(lp), GET_Y_LPARAM(lp))),
			static_cast<int>(wp), graphics::Dimension<>(
				horizontal ? GET_WHEEL_DELTA_WPARAM(wp) : 0,
				horizontal ? 0 : GET_WHEEL_DELTA_WPARAM(wp))));
	}
	static LRESULT CALLBACK windowProcedure(HWND window, UINT message, WPARAM wp, LPARAM lp) {
		WindowBase* self;
		bool consumed = false;
		if(message == WM_NCCREATE) {
			self = reinterpret_cast<WindowBase*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);
			assert(self != 0);
#ifdef _WIN64
			::SetWindowLongPtr(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
#else
			::SetWindowLong(window, GWL_USERDATA, static_cast<long>(reinterpret_cast<LONG_PTR>(self)));
#endif // _WIN64
			self->handle_.reset(window, &::DestroyWindow);
		} else {
			self = reinterpret_cast<WindowBase*>(
#ifdef _WIN64
				::GetWindowLongPtrW(window, GWLP_USERDATA));
#else
				static_cast<LONG_PTR>(::GetWindowLongW(window, GWL_USERDATA)));
#endif // _WIN64
			if(self == 0)
				return TRUE;
			const LRESULT r = self->preTranslateWindowMessage(message, wp, lp, consumed);
			if(consumed)
				return r;
			else if(message == WM_PAINT) {
				Handle<HWND> temp(window);
				PaintContext context(temp);
				self->paint(context);
				return FALSE;
			}
		}

		LRESULT result = self->processWindowMessage(message, wp, lp, consumed);
		if(!consumed)
			result = ::CallWindowProcW(::DefWindowProcW, window, message, wp, lp);
		return result;
	}
private:
	Handle<HWND> handle_;
};

LRESULT WindowBase::processWindowMessage(UINT message, WPARAM wp, LPARAM lp, bool& consumed) {
	switch(message) {
		case WM_KEYDOWN:
		case WM_KEYUP:
			return processKeyInput(message == WM_KEYUP, wp, lp, consumed);
		case WM_KILLFOCUS:
			return (consumed = aboutToLoseFocus()) ? 0 : 1;
		case WM_LBUTTONDBLCLK:
			processMouseDoubleClicked(viewers::base::UserInput::BUTTON1_DOWN, wp, lp, consumed);
			return consumed ? 0 : 1;
		case WM_LBUTTONDOWN:
			processMousePressed(viewers::base::UserInput::BUTTON1_DOWN, wp, lp, consumed);
			return consumed ? 0 : 1;
		case WM_LBUTTONUP:
			processMouseReleased(viewers::base::UserInput::BUTTON1_DOWN, wp, lp, consumed);
			return consumed ? 0 : 1;
		case WM_MBUTTONDBLCLK:
			processMouseDoubleClicked(viewers::base::UserInput::BUTTON2_DOWN, wp, lp, consumed);
			return consumed ? 0 : 1;
		case WM_MBUTTONDOWN:
			processMousePressed(viewers::base::UserInput::BUTTON2_DOWN, wp, lp, consumed);
			return consumed ? 0 : 1;
		case WM_MBUTTONUP:
			processMouseReleased(viewers::base::UserInput::BUTTON2_DOWN, wp, lp, consumed);
			return consumed ? 0 : 1;
		case WM_MOUSEHWHEEL:
			return processMouseWheelChanged(true, wp, lp, consumed);
		case WM_MOUSEMOVE:
			consumed = mouseMoved(viewers::base::LocatedUserInput(
				graphics::Point<>(GET_X_LPARAM(lp), GET_Y_LPARAM(lp)), static_cast<int>(wp)));
			return consumed ? 0 : 1;
		case WM_MOUSEWHEEL:
			return processMouseWheelChanged(false, wp, lp, consumed);
		case WM_RBUTTONDBLCLK:
			processMouseDoubleClicked(viewers::base::UserInput::BUTTON3_DOWN, wp, lp, consumed);
			return consumed ? 0 : 1;
		case WM_RBUTTONDOWN:
			processMousePressed(viewers::base::UserInput::BUTTON3_DOWN, wp, lp, consumed);
			return consumed ? 0 : 1;
		case WM_RBUTTONUP:
			processMouseReleased(viewers::base::UserInput::BUTTON3_DOWN, wp, lp, consumed);
			return consumed ? 0 : 1;
		case WM_SETFOCUS:
			return (consumed = focusGained()) ? 0 : 1;
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
			return processKeyInput(message == WM_KEYUP, wp, lp, consumed);
		case WM_XBUTTONDBLCLK:
			processMouseDoubleClicked((GET_XBUTTON_WPARAM(wp) == XBUTTON1) ?
					viewers::base::UserInput::BUTTON4_DOWN : viewers::base::UserInput::BUTTON5_DOWN,
				GET_KEYSTATE_WPARAM(wp), lp, consumed);
			return consumed ? TRUE : FALSE;
		case WM_XBUTTONDOWN:
			processMousePressed((GET_XBUTTON_WPARAM(wp) == XBUTTON1) ?
					viewers::base::UserInput::BUTTON4_DOWN : viewers::base::UserInput::BUTTON5_DOWN,
				GET_KEYSTATE_WPARAM(wp), lp, consumed);
			return consumed ? TRUE : FALSE;
		case WM_XBUTTONUP:
			processMouseReleased(
				(GET_XBUTTON_WPARAM(wp) == XBUTTON1) ?
					viewers::base::UserInput::BUTTON4_DOWN : viewers::base::UserInput::BUTTON5_DOWN,
				GET_KEYSTATE_WPARAM(wp), lp, consumed);
			return consumed ? TRUE : FALSE;
	}
	return TRUE;
}

///
class Window : public WindowBase {
public:
	// Win32-specific methods
	bool isWindow() const /*throw()*/ {
		return boole(::IsWindow(handle().get()));
	}
	void scheduleRedraw(bool eraseBackground) {
		if(!boole(::InvalidateRect(handle().get(), 0, eraseBackground)))
			throw PlatformDependentError<>();
	}
	void scheduleRedraw(const graphics::Rect<>& rect, bool eraseBackground) {
		RECT temp = toNative(rect);
		if(!boole(::InvalidateRect(handle().get(), &temp, eraseBackground)))
			throw PlatformDependentError<>();
	}
	// Win32-specific scrolling methods
	void scrollInformation(int bar, SCROLLINFO& scrollInfo, UINT mask = SIF_ALL) const {
		scrollInfo.cbSize = sizeof(SCROLLINFO);
		scrollInfo.fMask = mask;
		if(!boole(::GetScrollInfo(handle().get(), bar, &scrollInfo)))
			throw PlatformDependentError<>();
	}
	int scrollPosition(int bar) const {return ::GetScrollPos(handle().get(), bar);}
	Range<int> scrollRange(int bar) const {
		int minPos, maxPos;
		if(!boole(::GetScrollRange(handle().get(), bar, &minPos, &maxPos)))
			throw PlatformDependentError<>();
		return makeRange(minPos, maxPos);
	}
	int scrollTrackPosition(int bar) const {
		SCROLLINFO si;
		scrollInformation(bar, si, SIF_TRACKPOS);
		return si.nTrackPos;
	}
	void setScrollInformation(int bar, const SCROLLINFO& scrollInfo, bool redraw = true) {
		if(!boole(::SetScrollInfo(handle().get(), bar, &scrollInfo, redraw)))
			throw PlatformDependentError<>();
	}
	int setScrollPosition(int bar, int pos, bool redraw = true) {
		return ::SetScrollPos(handle().get(), bar, pos, redraw);
	}
	void Window::setScrollRange(int bar, const Range<int>& range, bool redraw = true) {
		::SetScrollRange(handle().get(), bar, range.beginning(), range.end(), redraw);
	}
	// viewers.Window implementation
	graphics::Rect<> bounds(bool includeFrame) const {
		RECT temp;
		if(includeFrame) {
			if(!boole(::GetWindowRect(handle().get(), &temp)))
				throw PlatformDependentError<>();
		} else {
			if(!boole(::GetClientRect(handle().get(), &temp)))
				throw PlatformDependentError<>();
		}
		return graphics::fromNative(temp);
	}
	bool hasFocus() const /*throw()*/ {
		return ::GetFocus() == handle().get();
	}
	void hide() {
		if(!boole(::SetWindowPos(handle().get(), 0, 0, 0, 0, 0,
				SWP_HIDEWINDOW | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOREPOSITION | SWP_NOSIZE | SWP_NOZORDER)))
			throw PlatformDependentError<>();
	}
	bool isVisible() const /*throw()*/ {
		return boole(::IsWindowVisible(handle().get()));
	}
	void redrawScheduledRegion() {
		if(!boole(::UpdateWindow(handle().get())))
			throw PlatformDependentError<>();
	}
	void setBounds(const graphics::Rect<>& bounds) {
		if(!boole(::SetWindowPos(handle().get(), 0,
				bounds.origin().x, bounds.origin().y,
				bounds.size().cx, bounds.size().cy, SWP_NOACTIVATE | SWP_NOZORDER)))
			throw PlatformDependentError<>();
	}
	void show() {
		if(!boole(::ShowWindow(handle().get(), SW_SHOWNOACTIVATE)))
			throw PlatformDependentError<>();
	}
private:
};

namespace internal {
	template<UINT message> struct Msg2Type {};
	template<typename WindowClass> struct MessageDispatcher {
#define DEFINE_DISPATCH(msg) static LRESULT dispatch(WindowClass& w, const Msg2Type<msg>, WPARAM wp, LPARAM lp, bool& handled)
		// WM_ACTIVATE -> void onActivate(UINT state, HWND otherWindow, bool minimized)
		DEFINE_DISPATCH(WM_ACTIVATE) {w.onActivate(LOWORD(wp), reinterpret_cast<HWND>(lp), boole(HIWORD(wp))); return 1;}
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
		// WM_LBUTTONDBLCLK -> void onLButtonDblClk(UINT flags, const POINT& position, bool& handled)
		DEFINE_DISPATCH(WM_LBUTTONDBLCLK) {const POINT p = {LOWORD(lp), HIWORD(lp)}; w.onLButtonDblClk(static_cast<UINT>(wp), p, handled); return 1;}
		// WM_LBUTTONDOWN -> void onLButtonDblClk(UINT flags, const POINT& position, bool& handled)
		DEFINE_DISPATCH(WM_LBUTTONDOWN) {const POINT p = {LOWORD(lp), HIWORD(lp)}; w.onLButtonDown(static_cast<UINT>(wp), p, handled); return 1;}
		// WM_LBUTTONUP -> void onLButtonUp(UINT flags, const POINT& position, bool& handled)
		DEFINE_DISPATCH(WM_LBUTTONUP) {const POINT p = {LOWORD(lp), HIWORD(lp)}; w.onLButtonUp(static_cast<UINT>(wp), p, handled); return 1;}
		// WM_MBUTTONDBLCLK -> void onMButtonDblClk(UINT flags, const POINT& position, bool& handled)
		DEFINE_DISPATCH(WM_MBUTTONDBLCLK) {const POINT p = {LOWORD(lp), HIWORD(lp)}; w.onMButtonDblClk(static_cast<UINT>(wp), p, handled); return 1;}
		// WM_MBUTTONDOWN -> void onMButtonDblClk(UINT flags, const POINT& position, bool& handled)
		DEFINE_DISPATCH(WM_MBUTTONDOWN) {const POINT p = {LOWORD(lp), HIWORD(lp)}; w.onMButtonDown(static_cast<UINT>(wp), p, handled); return 1;}
		// WM_MBUTTONUP -> void onMButtonUp(UINT flags, const POINT& position, bool& handled)
		DEFINE_DISPATCH(WM_MBUTTONUP) {const POINT p = {LOWORD(lp), HIWORD(lp)}; w.onMButtonUp(static_cast<UINT>(wp), p, handled); return 1;}
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
		// WM_RBUTTONDBLCLK -> void onRButtonDblClk(UINT flags, const POINT& position, bool& handled)
		DEFINE_DISPATCH(WM_RBUTTONDBLCLK) {const POINT p = {LOWORD(lp), HIWORD(lp)}; w.onRButtonDblClk(static_cast<UINT>(wp), p, handled); return 1;}
		// WM_RBUTTONDOWN -> void onRButtonDblClk(UINT flags, const POINT& position, bool& handled)
		DEFINE_DISPATCH(WM_RBUTTONDOWN) {const POINT p = {LOWORD(lp), HIWORD(lp)}; w.onRButtonDown(static_cast<UINT>(wp), p, handled); return 1;}
		// WM_RBUTTONUP -> void onRButtonUp(UINT flags, const POINT& position, bool& handled)
		DEFINE_DISPATCH(WM_RBUTTONUP) {const POINT p = {LOWORD(lp), HIWORD(lp)}; w.onRButtonUp(static_cast<UINT>(wp), p, handled); return 1;}
		// WM_SETCURSOR -> bool onSetCursor(HWND window, UINT hitTest, UINT message)
		DEFINE_DISPATCH(WM_SETCURSOR) {return handled = w.onSetCursor(reinterpret_cast<HWND>(wp), LOWORD(lp), HIWORD(lp));}
		// WM_SETFOCUS -> void onSetFocus(HWND oldWindow)
		DEFINE_DISPATCH(WM_SETFOCUS) {w.onSetFocus(reinterpret_cast<HWND>(wp)); handled = true; return 0;}
		// WM_SETFONT -> void onSetFont(HFONT font, bool redraw)
		DEFINE_DISPATCH(WM_SETFONT) {handled = true; w.onSetFont(reinterpret_cast<HFONT>(wp), boole(LOWORD(lp))); return 0;}
		// WM_SETTEXT -> bool onSetText(const WCHAR* text)
		DEFINE_DISPATCH(WM_SETTEXT) {handled = w.onSetText(reinterpret_cast<const WCHAR*>(lp)); return 0;}
		// WM_SETTINGCHANGE -> void onSettingChange(UINT flags, const WCHAR* sectionName)
		DEFINE_DISPATCH(WM_SETTINGCHANGE) {w.onSettingChange(static_cast<UINT>(wp), reinterpret_cast<const WCHAR*>(lp)); return 1;}
		// WM_SHOWWINDOW -> void onShowWindow(bool showing, UINT status)
		DEFINE_DISPATCH(WM_SHOWWINDOW) {w.onShowWindow(boole(wp), static_cast<UINT>(lp)); return 1;}
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

#define ASCENSION_WIN32_DECLEAR_WINDOW_MESSAGE_MAP(WindowClass)					\
protected:																		\
	virtual LRESULT processWindowMessage(UINT message, WPARAM, LPARAM, bool&);	\
	friend struct ascension::win32::internal::MessageDispatcher<WindowClass>

#define ASCENSION_WIN32_BEGIN_WINDOW_MESSAGE_MAP(WindowClass, BaseClass)							\
	LRESULT WindowClass::processWindowMessage(UINT message, WPARAM wp, LPARAM lp, bool& handled) {	\
		typedef WindowClass Klass; LRESULT result;													\
		switch(message) {

#define ASCENSION_WIN32_WINDOW_MESSAGE_ENTRY(msg)											\
		case msg: result = ascension::win32::internal::MessageDispatcher<Klass>::dispatch(	\
			*this, ascension::win32::internal::Msg2Type<msg>(), wp, lp, handled);			\
			if(handled) return result; break;

#define ASCENSION_WIN32_END_WINDOW_MESSAGE_MAP()												\
		}																						\
		return ascension::win32::WindowBase::processWindowMessage(message, wp, lp, handled);	\
	}


// Control must be define getClassName, static method returns const TCHAR* and takes no parameters,
// using this macro.
#define DEFINE_CLASS_NAME(name)	public: static const WCHAR* getClassName() {return name;}
template<class Control, class DefaultStyles = DefaultControlStyles>
class StandardControl : public SubclassableWindow {
public:
	typedef StandardControl<Control, DefaultStyles> BaseObject;
public:
	StandardControl() : SubclassableWindow() {}
	explicit StandardControl(Managed<HWND>* handle) : SubclassableWindow(handle) {}
	explicit StandardControl(Borrowed<HWND>* handle) : SubclassableWindow(handle) {}
	virtual ~StandardControl() {}
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

	}
} // namespace ascension.win32

#endif // !ASCENSION_WINDOW_WINDOWS_HPP
