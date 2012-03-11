/**
 * @file message-dispatcher.hpp
 * @author exeal
 * @date 2011-03-26 separated from window-windows.hpp
 * @date 2011-03-27 separated from widget-windows.hpp
 * @date 2012-03-07 renamed from ascension/viewer/base/message-dispatcher-windows.hpp
 */

#ifndef ASCENSION_WIN32_MESSAGE_DISPATCHER_HPP
#define ASCENSION_WIN32_MESSAGE_DISPATCHER_HPP
#include <ascension/corelib/type-traits.hpp>	// ASCENSION_DEFINED_HAS_METHOD
#include <ascension/graphics/geometry.hpp>
#include <ascension/win32/windows.hpp>
#include <type_traits>	// std.enable_if

// these macros are defined by winuser.h
#ifndef WM_UNICHAR
#	define WM_UNICHAR 109
#endif // !WM_UNICHAR
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
#ifndef WM_THEMECHANGED
#	define WM_THEMECHANGED 0x031a
#endif // !WM_THEMECHANGED
#ifndef GET_KEYSTATE_LPARAM
#	define GET_KEYSTATE_LPARAM(lp) (LOWORD(lp))
#endif // !GET_KEYSTATE_LPARAM

#define ASCENSION_DISPATCH_MESSAGE(messageID)	\
	case messageID:								\
		return dispatch(static_cast<Derived&>(widget), MessageTag<messageID>(), wp, lp, consumed)

#define ASCENSION_IMPLEMENT_MESSAGE_DISPATCH(messageID, methodName)	\
	template<typename T>											\
	static LRESULT dispatch(T& widget, MessageTag<messageID>,		\
		WPARAM wp, LPARAM lp, bool& consumed, typename std::enable_if<detail::Has_##methodName<T>::value>::type* = nullptr)

namespace ascension {

	namespace graphics {
		class PaintContext;
	}

	namespace detail {
		ASCENSION_DEFINE_HAS_METHOD(focusGained, bool(T::*)());
		ASCENSION_DEFINE_HAS_METHOD(aboutToLoseFocus, bool(T::*)());
//		ASCENSION_DEFINE_HAS_METHOD(paint, bool(T::*)(graphics::PaintContext&));
		ASCENSION_DEFINE_HAS_METHOD(keyPressed, bool(T::*)(const viewers::base::KeyInput&));
		ASCENSION_DEFINE_HAS_METHOD(keyReleased, bool(T::*)(const viewers::base::KeyInput&));
		ASCENSION_DEFINE_HAS_METHOD(mouseMoved, bool(T::*)(const viewers::base::LocatedUserInput&));
		ASCENSION_DEFINE_HAS_METHOD(mousePressed, bool(T::*)(const viewers::base::MouseButtonInput&));
		ASCENSION_DEFINE_HAS_METHOD(mouseReleased, bool(T::*)(const viewers::base::MouseButtonInput&));
		ASCENSION_DEFINE_HAS_METHOD(mouseDoubleClicked, bool(T::*)(const viewers::base::MouseButtonInput&));
		ASCENSION_DEFINE_HAS_METHOD(mouseWheelChanged, bool(T::*)(const viewers::base::MouseWheelInput&));
		
		ASCENSION_DEFINE_HAS_METHOD(onSetCursor, bool(T::*)(const win32::Handle<HWND>&, UINT, UINT));
		ASCENSION_DEFINE_HAS_METHOD(onNcCreate, bool(T::*)(CREATESTRUCT&));
		ASCENSION_DEFINE_HAS_METHOD(onDestroy, bool(T::*)());
		ASCENSION_DEFINE_HAS_METHOD(onSize, bool(T::*)(UINT, const graphics::Dimension<>&));
		ASCENSION_DEFINE_HAS_METHOD(onEraseBkgnd, bool(T::*)(graphics::PaintContext&));
		ASCENSION_DEFINE_HAS_METHOD(onSysColorChange, void(T::*)());
		ASCENSION_DEFINE_HAS_METHOD(onGetFont, const win32::Handle<HFONT>&(T::*)());
		ASCENSION_DEFINE_HAS_METHOD(onNotify, bool(T::*)(int, const NMHDR&));
		ASCENSION_DEFINE_HAS_METHOD(onContextMenu, bool(T::*)(const win32::Handle<HWND>&, const graphics::Point<>&));
		ASCENSION_DEFINE_HAS_METHOD(onStyleChanging, bool(T::*)(int, STYLESTRUCT&));
		ASCENSION_DEFINE_HAS_METHOD(onStyleChanged, bool(T::*)(int, const STYLESTRUCT&));
		ASCENSION_DEFINE_HAS_METHOD(onChar, bool(T::*)(UINT, UINT));
		ASCENSION_DEFINE_HAS_METHOD(onSysChar, bool(T::*)(UINT, UINT));
		ASCENSION_DEFINE_HAS_METHOD(onUniChar, bool(T::*)(UINT, UINT));
		ASCENSION_DEFINE_HAS_METHOD(onIMEStartComposition, void(T::*)());
		ASCENSION_DEFINE_HAS_METHOD(onIMEEndComposition, void(T::*)());
		ASCENSION_DEFINE_HAS_METHOD(onIMEComposition, void(T::*)(WPARAM, LPARAM, bool&));
		ASCENSION_DEFINE_HAS_METHOD(onCommand, bool(T::*)(WORD, WORD, const win32::Handle<HWND>&));
		ASCENSION_DEFINE_HAS_METHOD(onTimer, bool(T::*)(UINT_PTR, TIMERPROC));
		ASCENSION_DEFINE_HAS_METHOD(onHScroll, bool(T::*)(UINT, UINT, const win32::Handle<HWND>&));
		ASCENSION_DEFINE_HAS_METHOD(onVScroll, bool(T::*)(UINT, UINT, const win32::Handle<HWND>&));
		ASCENSION_DEFINE_HAS_METHOD(onCaptureChanged, bool(T::*)(const win32::Handle<HWND>&));
		ASCENSION_DEFINE_HAS_METHOD(onIMERequest, LRESULT(T::*)(WPARAM, LPARAM, bool&));
		ASCENSION_DEFINE_HAS_METHOD(onIMENotify, LRESULT(T::*)(WPARAM, LPARAM, bool&));
		ASCENSION_DEFINE_HAS_METHOD(onThemeChanged, bool(T::*)());
	}

	namespace win32 {
/*
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
*/
		template<UINT message> struct MessageTag {
			static const UINT value = message;
		};

		class WidgetBase;

		template<typename Derived>
		class MessageDispatcher {
		public:
			static LRESULT processMessage(WidgetBase& widget, UINT message, WPARAM wp, LPARAM lp, bool& consumed) {
				switch(message.message) {
					ASCENSION_DISPATCH_MESSAGE(WM_SETFOCUS);
					ASCENSION_DISPATCH_MESSAGE(WM_KILLFOCUS);
//					ASCENSION_DISPATCH_MESSAGE(WM_PAINT);
					ASCENSION_DISPATCH_MESSAGE(WM_SETCURSOR);
					ASCENSION_DISPATCH_MESSAGE(WM_NCCREATE);
					ASCENSION_DISPATCH_MESSAGE(WM_KEYDOWN);
					ASCENSION_DISPATCH_MESSAGE(WM_KEYUP);
					ASCENSION_DISPATCH_MESSAGE(WM_SYSKEYDOWN);
					ASCENSION_DISPATCH_MESSAGE(WM_SYSKEYUP);
					ASCENSION_DISPATCH_MESSAGE(WM_MOUSEMOVE);
					ASCENSION_DISPATCH_MESSAGE(WM_LBUTTONDOWN);
					ASCENSION_DISPATCH_MESSAGE(WM_LBUTTONUP);
					ASCENSION_DISPATCH_MESSAGE(WM_LBUTTONDBLCLK);
					ASCENSION_DISPATCH_MESSAGE(WM_RBUTTONDOWN);
					ASCENSION_DISPATCH_MESSAGE(WM_RBUTTONUP);
					ASCENSION_DISPATCH_MESSAGE(WM_RBUTTONDBLCLK);
					ASCENSION_DISPATCH_MESSAGE(WM_MBUTTONDOWN);
					ASCENSION_DISPATCH_MESSAGE(WM_MBUTTONUP);
					ASCENSION_DISPATCH_MESSAGE(WM_MBUTTONDBLCLK);
					ASCENSION_DISPATCH_MESSAGE(WM_MOUSEWHEEL);
					ASCENSION_DISPATCH_MESSAGE(WM_XBUTTONDOWN);
					ASCENSION_DISPATCH_MESSAGE(WM_XBUTTONUP);
					ASCENSION_DISPATCH_MESSAGE(WM_XBUTTONDBLCLK);
					ASCENSION_DISPATCH_MESSAGE(WM_MOUSEHWHEEL);
					
					ASCENSION_DISPATCH_MESSAGE(WM_DESTROY);
					ASCENSION_DISPATCH_MESSAGE(WM_SIZE);
					ASCENSION_DISPATCH_MESSAGE(WM_ERASEBKGND);
					ASCENSION_DISPATCH_MESSAGE(WM_SYSCOLORCHANGE);
					ASCENSION_DISPATCH_MESSAGE(WM_GETFONT);
					ASCENSION_DISPATCH_MESSAGE(WM_NOTIFY);
					ASCENSION_DISPATCH_MESSAGE(WM_CONTEXTMENU);
					ASCENSION_DISPATCH_MESSAGE(WM_STYLECHANGING);
					ASCENSION_DISPATCH_MESSAGE(WM_STYLECHANGED);
					ASCENSION_DISPATCH_MESSAGE(WM_CHAR);
					ASCENSION_DISPATCH_MESSAGE(WM_SYSCHAR);
					ASCENSION_DISPATCH_MESSAGE(WM_UNICHAR);
					ASCENSION_DISPATCH_MESSAGE(WM_IME_STARTCOMPOSITION);
					ASCENSION_DISPATCH_MESSAGE(WM_IME_ENDCOMPOSITION);
					ASCENSION_DISPATCH_MESSAGE(WM_IME_COMPOSITION);
					ASCENSION_DISPATCH_MESSAGE(WM_COMMAND);
					ASCENSION_DISPATCH_MESSAGE(WM_TIMER);
					ASCENSION_DISPATCH_MESSAGE(WM_HSCROLL);
					ASCENSION_DISPATCH_MESSAGE(WM_VSCROLL);
					ASCENSION_DISPATCH_MESSAGE(WM_CAPTURECHANGED);
					ASCENSION_DISPATCH_MESSAGE(WM_IME_NOTIFY);
					ASCENSION_DISPATCH_MESSAGE(WM_IME_REQUEST);
					ASCENSION_DISPATCH_MESSAGE(WM_THEMECHANGED);
				}
			}
		private:
			static int generateKeyModifiers() {
				int modifiers;
				if(::GetKeyState(VK_SHIFT) < 0)
					modifiers |= viewers::base::UserInput::SHIFT_DOWN;
				if(::GetKeyState(VK_CONTROL) < 0)
					modifiers |= viewers::base::UserInput::CONTROL_DOWN;
				if(::GetKeyState(VK_MENU) < 0)
					modifiers |= viewers::base::UserInput::ALT_DOWN;
			}
			static LRESULT callKeyPressed(Derived& widget, WPARAM wp, LPARAM lp, bool& consumed) {
				const viewers::base::KeyInput input(viewers::base::keyboardCodeFromWin32(wp),
					generateKeyModifiers(), static_cast<int>(lp & 0xffffu), HIWORD(lp));
				return (consumed = widget.keyPressed(input)) ? 0 : 1;
			}
			static LRESULT callKeyReleased(Derived& widget, WPARAM wp, LPARAM lp, bool& consumed) {
				const viewers::base::KeyInput input(viewers::base::keyboardCodeFromWin32(wp),
					generateKeyModifiers(), static_cast<int>(lp & 0xffffu), HIWORD(lp));
				return (consumed = widget.keyReleased(input)) ? 0 : 1;
			}
			static void processMouseDoubleClicked(Derived& widget,
					viewers::base::UserInput::MouseButton button, WPARAM wpForModifiers, LPARAM lp, bool& consumed) {
				consumed = widget.mouseDoubleClicked(viewers::base::MouseButtonInput(
					graphics::Point<>(GET_X_LPARAM(lp), GET_Y_LPARAM(lp)),
					button, inputModifiersFromNative(wpForModifiers)));
			}
			static void processMousePressed(Derived& widget,
					viewers::base::UserInput::MouseButton button, WPARAM wpForModifiers, LPARAM lp, bool& consumed) {
				consumed = widget.mousePressed(viewers::base::MouseButtonInput(
					graphics::Point<>(GET_X_LPARAM(lp), GET_Y_LPARAM(lp)),
					button, inputModifiersFromNative(wpForModifiers)));
			}
			static void processMouseReleased(Derived& widget,
					viewers::base::UserInput::MouseButton button, WPARAM wpForModifiers, LPARAM lp, bool& consumed) {
				consumed = widget.mouseReleased(viewers::base::MouseButtonInput(
					graphics::Point<>(GET_X_LPARAM(lp), GET_Y_LPARAM(lp)),
					button, inputModifiersFromNative(wpForModifiers)));
			}
			static void processMouseWheelChanged(bool horizontal, WPARAM wp, LPARAM lp, bool& consumed) {
				consumed = mouseWheelChanged(viewers::base::MouseWheelInput(
					screenToClient(graphics::Point<>(GET_X_LPARAM(lp), GET_Y_LPARAM(lp))),
					static_cast<int>(wp), graphics::Dimension<>(
						horizontal ? GET_WHEEL_DELTA_WPARAM(wp) : 0,
						horizontal ? 0 : GET_WHEEL_DELTA_WPARAM(wp))));
			}
			
			ASCENSION_IMPLEMENT_MESSAGE_DISPATCH(WM_KILLFOCUS, aboutToLoseFocus) {
				return (consumed = widget.aboutToLoseFocus()) ? 0 : 1;
			}
			ASCENSION_IMPLEMENT_MESSAGE_DISPATCH(WM_SETFOCUS, focusGained) {
				return (consumed = widget.focusGained()) ? 0 : 1;
			}
			ASCENSION_IMPLEMENT_MESSAGE_DISPATCH(WM_KEYDOWN, keyPressed) {
				return callKeyPressed(widget, wp, lp, consumed);
			}
			ASCENSION_IMPLEMENT_MESSAGE_DISPATCH(WM_KEYUP, keyReleased) {
				return callKeyReleased(widget, wp, lp, consumed);
			}
			ASCENSION_IMPLEMENT_MESSAGE_DISPATCH(WM_SYSKEYDOWN, keyPressed) {
				return callKeyPressed(widget, wp, lp, consumed);
			}
			ASCENSION_IMPLEMENT_MESSAGE_DISPATCH(WM_SYSKEYUP, keyReleased) {
				return callKeyReleased(widget, wp, lp, consumed);
			}
			ASCENSION_IMPLEMENT_MESSAGE_DISPATCH(WM_MOUSEMOVE, mouseMoved) {
				consumed = widget.mouseMoved(viewers::base::LocatedUserInput(
					graphics::Point<>(GET_X_LPARAM(lp), GET_Y_LPARAM(lp)), static_cast<int>(wp)));
				return consumed ? 0 : 1;
			}
			ASCENSION_IMPLEMENT_MESSAGE_DISPATCH(WM_LBUTTONDOWN, mousePressed) {
				processMousePressed(viewers::base::UserInput::BUTTON1_DOWN, wp, lp, consumed);
				return consumed ? 0 : 1;
			}
			ASCENSION_IMPLEMENT_MESSAGE_DISPATCH(WM_LBUTTONUP, mouseReleased) {
				processMouseReleased(viewers::base::UserInput::BUTTON1_DOWN, wp, lp, consumed);
				return consumed ? 0 : 1;
			}
			ASCENSION_IMPLEMENT_MESSAGE_DISPATCH(WM_LBUTTONDBLCLK, mousePressed) {
				processMouseDoubleClicked(viewers::base::UserInput::BUTTON1_DOWN, wp, lp, consumed);
				return consumed ? 0 : 1;
			}
			ASCENSION_IMPLEMENT_MESSAGE_DISPATCH(WM_RBUTTONDOWN, mousePressed) {
				processMousePressed(viewers::base::UserInput::BUTTON3_DOWN, wp, lp, consumed);
				return consumed ? 0 : 1;
			}
			ASCENSION_IMPLEMENT_MESSAGE_DISPATCH(WM_RBUTTONUP, mouseReleased) {
				processMouseReleased(viewers::base::UserInput::BUTTON3_DOWN, wp, lp, consumed);
				return consumed ? 0 : 1;
			}
			ASCENSION_IMPLEMENT_MESSAGE_DISPATCH(WM_RBUTTONDBLCLK, mouseDoubleClicked) {
				processMouseDoubleClicked(viewers::base::UserInput::BUTTON3_DOWN, wp, lp, consumed);
				return consumed ? 0 : 1;
			}
			ASCENSION_IMPLEMENT_MESSAGE_DISPATCH(WM_MBUTTONDOWN, mousePressed) {
				processMousePressed(viewers::base::UserInput::BUTTON2_DOWN, wp, lp, consumed);
				return consumed ? 0 : 1;
			}
			ASCENSION_IMPLEMENT_MESSAGE_DISPATCH(WM_MBUTTONUP, mouseReleased) {
				processMouseReleased(viewers::base::UserInput::BUTTON2_DOWN, wp, lp, consumed);
				return consumed ? 0 : 1;
			}
			ASCENSION_IMPLEMENT_MESSAGE_DISPATCH(WM_MBUTTONDBLCLK, mouseDoubleClicked) {
				processMouseDoubleClicked(viewers::base::UserInput::BUTTON2_DOWN, wp, lp, consumed);
				return consumed ? 0 : 1;
			}
			ASCENSION_IMPLEMENT_MESSAGE_DISPATCH(WM_MOUSEWHEEL, mouseWheelChanged) {
				processMouseWheelChanged(false, wp, lp, consumed);
				return consumed ? 0 : 1;
			}
			ASCENSION_IMPLEMENT_MESSAGE_DISPATCH(WM_XBUTTONDOWN, mousePressed) {
				processMousePressed((GET_XBUTTON_WPARAM(wp) == XBUTTON1) ?
						viewers::base::UserInput::BUTTON4_DOWN : viewers::base::UserInput::BUTTON5_DOWN,
					GET_KEYSTATE_WPARAM(wp), lp, consumed);
				return consumed ? TRUE : FALSE;
			}
			ASCENSION_IMPLEMENT_MESSAGE_DISPATCH(WM_XBUTTONUP, mouseReleased) {
				processMouseReleased(
					(GET_XBUTTON_WPARAM(wp) == XBUTTON1) ?
						viewers::base::UserInput::BUTTON4_DOWN : viewers::base::UserInput::BUTTON5_DOWN,
					GET_KEYSTATE_WPARAM(wp), lp, consumed);
				return consumed ? TRUE : FALSE;
			}
			ASCENSION_IMPLEMENT_MESSAGE_DISPATCH(WM_XBUTTONDBLCLK, mouseDoubleClicked) {
				processMouseDoubleClicked((GET_XBUTTON_WPARAM(wp) == XBUTTON1) ?
						viewers::base::UserInput::BUTTON4_DOWN : viewers::base::UserInput::BUTTON5_DOWN,
					GET_KEYSTATE_WPARAM(wp), lp, consumed);
				return consumed ? TRUE : FALSE;
			}
			
			ASCENSION_IMPLEMENT_MESSAGE_DISPATCH(WM_SETCURSOR, onSetCursor) {
				return (consumed = widget.onSetCursor(Handle<HWND>(reinterpret_cast<HWND>(wp)), LOWORD(lp), HIWORD(lp))) ? TRUE : FALSE;
			}
			ASCENSION_IMPLEMENT_MESSAGE_DISPATCH(WM_NCCREATE, onNcCreate) {
				return (consumed = true), widget.onNcCreate();
			}
			ASCENSION_IMPLEMENT_MESSAGE_DISPATCH(WM_DESTROY, onDestroy) {
				return (consumed = widget.onDestroy()) ? 0 : 1;
			}
			ASCENSION_IMPLEMENT_MESSAGE_DISPATCH(WM_SIZE, onSize) {
				return (consumed = widget.onSize(static_cast<UINT>(wp), LOWORD(lp), HIWORD(lp))) ? 0 : 1;
			}
			ASCENSION_IMPLEMENT_MESSAGE_DISPATCH(WM_ERASEBKGND, onEraseBkgnd) {
				return (consumed = widget.onEraseBkgnd()) ? TRUE : FALSE;
			}
			ASCENSION_IMPLEMENT_MESSAGE_DISPATCH(WM_SYSCOLORCHANGE, onSysColorChange) {
				return (consumed = true), widget.onSysColorChange();
			}
			ASCENSION_IMPLEMENT_MESSAGE_DISPATCH(WM_GETFONT, onGetFont) {
				const Handle<HFONT>& borrowed = widget.onGetFont();
				return (consumed = true), borrowed.get();
			}
			ASCENSION_IMPLEMENT_MESSAGE_DISPATCH(WM_NOTIFY, onNotify) {
				return (consumed = widget.onNotify(static_cast<int>(wp), *reinterpret_cast<NMHDR*>(lp))), 0;
			}
			ASCENSION_IMPLEMENT_MESSAGE_DISPATCH(WM_CONTEXTMENU, onContextMenu) {
				const POINT temp = {LOWORD(lp), HIWORD(lp)};
				return (consumed = widget.onContextMenu(Handle<HWND>(reinterpret_cast<HWND>(wp)), graphics::fromNative(temp))), 1;
			}
			ASCENSION_IMPLEMENT_MESSAGE_DISPATCH(WM_STYLECHANGING, onStyleChanging) {
				return (consumed = widget.onStyleChanging(static_cast<int>(wp), *reinterpret_cast<STYLESTRUCT*>(lp))) ? 0 : 1;
			}
			ASCENSION_IMPLEMENT_MESSAGE_DISPATCH(WM_STYLECHANGED, onStyleChanged) {
				return (consumed = widget.onStyleChanged(static_cast<int>(wp), *reinterpret_cast<STYLESTRUCT*>(lp))) ? 0 : 1;
			}
			ASCENSION_IMPLEMENT_MESSAGE_DISPATCH(WM_IME_STARTCOMPOSITION, onIMEStartComposition) {
				return widget.onIMEStartComposition(), 1;
			}
			ASCENSION_IMPLEMENT_MESSAGE_DISPATCH(WM_IME_ENDCOMPOSITION, onIMEEndComposition) {
				return widget.onIMEEndComposition(), 1;
			}
			ASCENSION_IMPLEMENT_MESSAGE_DISPATCH(WM_IME_COMPOSITION, onIMEComposition) {
				return widget.onIMEComposition(wp, lp, consumed), 1;
			}
			ASCENSION_IMPLEMENT_MESSAGE_DISPATCH(WM_CHAR, onChar) {
				return (consumed = widget.onChar(static_cast<UINT>(wp), static_cast<UINT>(lp))) ? 0 : 1;
			}
			ASCENSION_IMPLEMENT_MESSAGE_DISPATCH(WM_SYSCHAR, onSysChar) {
				return (consumed = widget.onSysChar(static_cast<UINT>(wp), static_cast<UINT>(lp))) ? 0 : 1;
			}
			ASCENSION_IMPLEMENT_MESSAGE_DISPATCH(WM_UNICHAR, onUniChar) {
				return (consumed = widget.onUniChar(static_cast<UINT>(wp), static_cast<UINT>(lp))) ? 0 : 1;
			}
			ASCENSION_IMPLEMENT_MESSAGE_DISPATCH(WM_COMMAND, onCommand) {
				return (consumed = widget.onCommand(LOWORD(wp), HIWORD(wp), Handle<HWND>(reinterpret_cast<HWND>(lp)))) ? 0 : 1;
			}
			ASCENSION_IMPLEMENT_MESSAGE_DISPATCH(WM_TIMER, onTimer) {
				return (consumed = widget.onTimer(static_cast<UINT_PTR>(wp), reinterpret_cast<TIMERPROC>(lp))) ? 0 : 1;
			}
			ASCENSION_IMPLEMENT_MESSAGE_DISPATCH(WM_HSCROLL, onHScroll) {
				return (consumed = widget.onHScroll(LOWORD(wp), HIWORD(wp), Handle<HWND>(reinterpret_cast<HWND>(lp)))) ? 0 : 1;
			}
			ASCENSION_IMPLEMENT_MESSAGE_DISPATCH(WM_VSCROLL, onVScroll) {
				return (consumed = widget.onVScroll(LOWORD(wp), HIWORD(wp), Handle<HWND>(reinterpret_cast<HWND>(lp)))) ? 0 : 1;
			}
			ASCENSION_IMPLEMENT_MESSAGE_DISPATCH(WM_CAPTURECHANGED, onCaptureChanged) {
				return (consumed = widget.onCaptureChanged(Handle<HWND>(reinterpret_cast<HWND>(lp)))) ? 0 : 1;
			}
			ASCENSION_IMPLEMENT_MESSAGE_DISPATCH(WM_IME_REQUEST, onIMERequest) {
				return widget.onIMERequest(wp, lp, consumed);
			}
			ASCENSION_IMPLEMENT_MESSAGE_DISPATCH(WM_IME_NOTIFY, onIMENotify) {
				return widget.onIMENotify(wp, lp, consumed);
			}
			ASCENSION_IMPLEMENT_MESSAGE_DISPATCH(WM_THEMECHANGED, onThemeChanged) {
				return (consumed = widget.onThemeChanged()) ? 0 : 1;
			}

			template<typename T> static LRESULT dispatch(T& widget, ...) {return 0;}
			
		};

	}
}

#undef ASCENSION_DISPATCH_MESSAGE
#undef ASCENSION_IMPLEMENT_MESSAGE_DISPATCH

#endif // !ASCENSION_WIN32_MESSAGE_DISPATCHER_HPP
