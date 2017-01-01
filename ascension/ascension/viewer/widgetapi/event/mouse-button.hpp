/**
 * @file mouse-button.hpp
 * Defines @c MouseButton enumeration and @c MouseButtons class.
 * @author exeal
 * @date 2017-01-01 Created.
 */

#ifndef ASCENSION_MOUSE_BUTTON_HPP
#define ASCENSION_MOUSE_BUTTON_HPP
#include <ascension/corelib/combination.hpp>

namespace ascension {
	namespace viewer {
		namespace widgetapi {
			namespace event {
				/// Mouse button values which indicate positions of the bit in @c MouseButtons class.
				enum MouseButton {
					/// The Mouse Button1 (usually left button) is down. This corresponds to @c Gdk#BUTTON1_MASK in
					/// gtkmm, @c Qt#LeftButton in Qt and @c MK_LBUTTON in Win32.
					BUTTON1_DOWN,
					/// The Mouse Button2 (usually middle button) is down. This corresponds to @c Gdk#BUTTON2_MASK in
					/// gtkmm, @c Qt#RightButton in Qt and @c MK_RBUTTON in Win32.
					BUTTON2_DOWN,
					/// The Mouse Button3 (usually right button) is down. This corresponds to @c Gdk#BUTTON3_MASK in
					/// gtkmm, @c Qt#MiddleButton in Qt and @c MK_MBUTTON in Win32.
					BUTTON3_DOWN,
					/// The Mouse Button4 (usually X1 button) is down. This corresponds to @c Gdk#BUTTON4_MASK in
					/// gtkmm, @c Qt#XButton1 in Qt and @c MK_XBUTTON1 in Win32.
					BUTTON4_DOWN,
					/// The Mouse Button5 (usually X2 button) is down. This corresponds to @c Gdk#BUTTON5_MASK in
					/// gtkmm, @c Qt#XButton2 in Qt and @c MK_XBUTTON2 in Win32.
					BUTTON5_DOWN,
					NUMBER_OF_MOUSE_BUTTONS
				};

				/// Indicates the state of mouse buttons.
				typedef Combination<MouseButton, NUMBER_OF_MOUSE_BUTTONS> MouseButtons;

#if ASCENSION_SUPPORTS_WINDOW_SYSTEM(WIN32)
				template<typename Model>
				inline Model fromNative(WORD native, typename std::enable_if<std::is_same<Model, MouseButtons>::value>::type* = nullptr) {
					MouseButtons buttons;
					if((native & MK_LBUTTON) != 0)
						buttons.set(BUTTON1_DOWN);
					if((native & MK_RBUTTON) != 0)
						buttons.set(BUTTON2_DOWN);
					if((native & MK_MBUTTON) != 0)
						buttons.set(BUTTON3_DOWN);
					if((native & MK_XBUTTON1) != 0)
						buttons.set(BUTTON4_DOWN);
					if((native & MK_XBUTTON2) != 0)
						buttons.set(BUTTON5_DOWN);
					return buttons;
				}

				inline WORD toNative(const MouseButtons& buttons, WORD* = nullptr) {
					WORD native = 0;
					native |= buttons.test(BUTTON1_DOWN) ? MK_LBUTTON : 0;
					native |= buttons.test(BUTTON2_DOWN) ? MK_RBUTTON : 0;
					native |= buttons.test(BUTTON3_DOWN) ? MK_MBUTTON : 0;
					native |= buttons.test(BUTTON4_DOWN) ? MK_XBUTTON1 : 0;
					native |= buttons.test(BUTTON5_DOWN) ? MK_XBUTTON2 : 0;
					return native;
				}
#endif
			}
		}
	}
}

#endif // !ASCENSION_MOUSE_BUTTON_HPP
