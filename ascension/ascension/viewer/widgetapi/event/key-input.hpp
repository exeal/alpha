/**
 * @file key-input.hpp
 * Defines @c KeyInput class.
 * @author exeal
 * @date 2010-11-10 Created (as user-input.hpp)
 * @date 2014-09-07 Separated from user-input.hpp
 */

#ifndef ASCENSION_KEY_INPUT_HPP
#define ASCENSION_KEY_INPUT_HPP

#include <ascension/viewer/widgetapi/event/user-input.hpp>

namespace ascension {
	namespace viewers {
		namespace widgetapi {
			namespace event {
				/// An event which indicates that keystroke occurred in a widget.
				class KeyInput : public UserInput {
				public:
					/**
					 * Keyboard codes.
					 * @note Corresponds to @c GDK_KEY_* in gdk/gdkkeysyms.h in GDK.
					 * @note Corresponds to @c Qt#Key in Qt.
					 * @note Cooresponds to @c VK_* in WinUser.h in Win32.
					 */
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
					typedef guint Code;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
					typedef int Code;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
					typedef WORD Code;
#elif 0
					typedef std::uint32_t Code;
#endif
				public:
					/**
					 * Creates a @c KeyInput object with given keycode and modifiers.
					 * @param keyboardCode The integer code for an actual key
					 * @param modifiers The keyboard modifier flags during event
					 */
					KeyInput(Code keyboardCode, KeyboardModifier modifiers) : UserInput(modifiers), keyboardCode_(keyboardCode) {}
					/// Returns the integer key code associated with the key in this event.
					Code keyboardCode() const BOOST_NOEXCEPT {
						return keyboardCode_;
					}

				private:
					const Code keyboardCode_;
				};
			}
		}
	}
}

#endif // !ASCENSION_KEY_INPUT_HPP
