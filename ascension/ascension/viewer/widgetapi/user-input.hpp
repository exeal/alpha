/**
 * @file user-input.hpp
 * Defines the classes represent user input events.
 * @author exeal
 * @date 2010-11-10 created
 */

#ifndef ASCENSION_USER_INPUT_HPP
#define ASCENSION_USER_INPUT_HPP

#include <ascension/graphics/geometry.hpp>	// graphics.Point
#include <ctime>

namespace ascension {
	namespace viewers {
		namespace widgetapi {

			class Event {
			public:
				Event() BOOST_NOEXCEPT : consumed_(false) {}
				void consume() BOOST_NOEXCEPT {consumed_ = true;}
				void ignore() BOOST_NOEXCEPT {consumed_ = false;}
				bool isConsumed() const BOOST_NOEXCEPT {return consumed_;}
			private:
				bool consumed_;
			};

			/// Abstract class represents a user input.
			class UserInput : public Event {
			public:
				typedef std::uint16_t ModifierKey;
				static const ModifierKey
					/// The Shift key is down.
					SHIFT_DOWN		= 1 << 0,
					/// The Ctrl (Control) key is down.
					CONTROL_DOWN	= 1 << 1,
					/// The Alt key is down.
					ALT_DOWN		= 1 << 2,
					/// The AltGraph key is down.
					ALT_GRAPH_DOWN	= 1 << 3,
					/// The Command key is down. Only for Mac OS X.
					COMMAND_DOWN	= 1 << 4;
				/**
				 * @note Defined here because these values also can be used as modifiers.
				 */
				typedef std::uint16_t MouseButton;
				static const MouseButton
					/// The Mouse Button1 (usually left button) is down.
					BUTTON1_DOWN	= 1 << 5,
					/// The Mouse Button2 (usually middle button) is down.
					BUTTON2_DOWN	= 1 << 6,
					/// The Mouse Button3 (usually right button) is down.
					BUTTON3_DOWN	= 1 << 7,
					/// The Mouse Button4 (usually X1 button) is down.
					BUTTON4_DOWN	= 1 << 8,
					/// The Mouse Button5 (usually X2 button) is down.
					BUTTON5_DOWN	= 1 << 9;
			public:
				ModifierKey modifiers() const BOOST_NOEXCEPT {return modifiers_;}
				const std::time_t& timeStamp() const BOOST_NOEXCEPT {return timeStamp_;}
			protected:
				/**
				 * Protected constructor.
				 * @param modifiers The modifier flags
				 */
				explicit UserInput(ModifierKey modifiers) : modifiers_(modifiers) {std::time(&timeStamp_);}
			private:
				const ModifierKey modifiers_;
				std::time_t timeStamp_;
			};

			/**
			 * Returns @c true if the given user input is the specified modifier down.
			 * @tparam modifier The modifier key to test
			 * @param input The user input
			 * @return true if @a input has @a modifier
			 */
			template<UserInput::ModifierKey modifier>
			inline bool hasModifier(const UserInput& input) BOOST_NOEXCEPT {
				return (input.modifiers() & modifier) != 0;
			}

			/// Abstract class represents a user input located at a specific position in the screen.
			class LocatedUserInput : public UserInput {
			public:
				/**
				 * Protected constructor.
				 * @param location The location
				 * @param modifiers The modifier flags
				 */
				LocatedUserInput(const graphics::Point& location, ModifierKey modifiers) : UserInput(modifiers), location_(location) {
				}
				/// Returns the location.
				const graphics::Point& location() const BOOST_NOEXCEPT {return location_;}
			private:
				const graphics::Point location_;
			};

			/// A @c MouseButtonInput represents a mouse button event.
			class MouseButtonInput : public LocatedUserInput {
			public:
				/**
				 * Constructor.
				 * @param location
				 * @param button
				 * @param modifiers
				 */
				MouseButtonInput(const graphics::Point& location, MouseButton button,
					ModifierKey modifiers) : LocatedUserInput(location, modifiers), button_(button) {}
				/// Returns the mouse button.
				MouseButton button() const BOOST_NOEXCEPT {return button_;}
			private:
				const MouseButton button_;
			};

			/// A @c MouseWheelEvent represents a mouse wheel event.
			class MouseWheelInput : public LocatedUserInput {
			public:
				/**
				 * Constructor.
				 * @param location
				 * @param modifiers
				 * @param rotation
				 */
				MouseWheelInput(const graphics::Point& location, ModifierKey modifiers,
					graphics::Dimension& rotation) : LocatedUserInput(location, modifiers), rotation_(rotation) {}
				/// Returns the mouse wheel rotation.
				const graphics::Dimension& rotation() const BOOST_NOEXCEPT {return rotation_;}
			private:
				const graphics::Dimension rotation_;
			};

			class KeyInput : public UserInput {
			public:
				/// Keyboard codes.
#if defined(ASCENSION_WINDOW_SYSTEM_GTK)
				typedef guint Code;	// GDK_KEY_* in gdk/gdkkeysyms.h
#elif defined(ASCENSION_WINDOW_SYSTEM_WIN32)
				typedef WORD Code;	// VK_* in WinUser.h
#elif 0
				typedef std::uint32_t Code;
#endif
			public:
				KeyInput(Code keyboardCode, ModifierKey modifiers, int repeatCount, int messageFlags)
					: UserInput(modifiers), keyboardCode_(keyboardCode), repeatCount_(repeatCount), messageFlags_(messageFlags) {}
				Code keyboardCode() const BOOST_NOEXCEPT {return keyboardCode_;}
			private:
				const Code keyboardCode_;
				const int repeatCount_, messageFlags_;
			};

#ifdef ASCENSION_ABANDONED_AT_VERSION_08
			namespace keyboardcodes {
				static const KeyInput::Code
#if defined(ASCENSION_WINDOW_SYSTEM_WIN32)
					BACK_SPACE = VK_BACK, TAB = VK_TAB, CLEAR = VK_CLEAR, ENTER_OR_RETURN = VK_RETURN,
					SHIFT = VK_SHIFT, CONTROL = VK_CONTROL, ALT_OR_MENU = VK_MENU, PAUSE = VK_PAUSE, CAPITAL = VK_CAPITAL,
					KANA = VK_KANA, HANGUL = VK_HANGUL, JUNJA = VK_JUNJA, FINAL = VK_FINAL, HANJA = VK_HANJA, KANJI = VK_KANJI,
					ESCAPE = VK_ESCAPE, CONVERT = VK_CONVERT, NON_CONVERT = VK_NONCONVERT, ACCEPT = VK_ACCEPT, MODE_CHANGE = VK_MODECHANGE,
					SPACE = VK_SPACE, PRIOR_OR_PAGE_UP = VK_PRIOR, NEXT_OR_PAGE_DOWN = VK_NEXT,
					END = VK_END, HOME = VK_HOME, LEFT = VK_LEFT, UP = VK_UP, RIGHT = VK_RIGHT, DOWN = VK_DOWN,
					SELECT = VK_SELECT, PRINT = VK_PRINT, EXECUTE = VK_EXECUTE,
					SNAPSHOT = VK_SNAPSHOT, INSERT = VK_INSERT, DEL_OR_DELETE = VK_DELETE, HELP = VK_HELP,
					LEFT_WINDOWS = VK_LWIN, COMMAND = LEFT_WINDOWS, RIGHT_WINDOWS = VK_RWIN, APPLICATIONS = VK_APPS, SLEEP = VK_SLEEP,
					NUMBER_PAD_0 = VK_NUMPAD0, NUMBER_PAD_1 = VK_NUMPAD1, NUMBER_PAD_2 = VK_NUMPAD2, NUMBER_PAD_3 = VK_NUMPAD3,
					NUMBER_PAD_4 = VK_NUMPAD4, NUMBER_PAD_5 = VK_NUMPAD5, NUMBER_PAD_6 = VK_NUMPAD6, NUMBER_PAD_7 = VK_NUMPAD7,
					NUMBER_PAD_8 = VK_NUMPAD8, NUMBER_PAD_9 = VK_NUMPAD9,
					MULTIPLAY = VK_MULTIPLY, ADD = VK_ADD, SEPARATOR = VK_SEPARATOR, SUBTRACT = VK_SUBTRACT,
					DECIMAL = VK_DECIMAL, DIVIDE = VK_DIVIDE,
					F1 = VK_F1, F2 = VK_F2, F3 = VK_F3, F4 = VK_F4, F5 = VK_F5, F6 = VK_F6, F7 = VK_F7, F8 = VK_F8,
					F9 = VK_F9, F10 = VK_F10, F11 = VK_F11, F12 = VK_F12, F13 = VK_F13, F14 = VK_F14, F15 = VK_F15, F16 = VK_F16,
					F17 = VK_F17, F18 = VK_F18, F19 = VK_F19, F20 = VK_F20, F21 = VK_F21, F22 = VK_F22, F23 = VK_F23, F24 = VK_F24,
					NUMBER_LOCK = VK_NUMLOCK, SCROLL_LOCK = VK_SCROLL, LEFT_SHIFT = VK_LSHIFT, RIGHT_SHIFT = VK_RSHIFT,
					LEFT_CONTROL = VK_LCONTROL, RIGHT_CONTROL = VK_RCONTROL, LEFT_ALT_OR_MENU = VK_LMENU, RIGHT_ALT_OR_MENU = VK_RMENU;
#else
#endif
			}
#endif

		}
	}
}


#endif // !ASCENSION_USER_INPUT_HPP
