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
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
#	include <gdkmm/types.h>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#	error not implemented.
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
#	include <QInputEvent>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
#	include <ascension/win32/windows.hpp>
#endif

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
				/**
				 * Indicates the state of modifier keys.
				 * @note Corresponds to @c GdkModifierType in GDK.
				 * @note Cooresponds to @c Qt#Modifier and @c Qt#KeyboardModifier in Qt.
				 */
				typedef std::uint32_t KeyboardModifier;

				/// @var SHIFT_DOWN The Shift key is down.

				/// @var CONTROL_DOWN The Ctrl (Control) key is down.

				/// @var ALT_DOWN The Alt key is down.

				/// @var ALT_GRAPH_DOWN The AltGraph key is down.

				/// @var COMMAND_DOWN The Command key is down. Only for Mac OS X.

				static const KeyboardModifier
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
					SHIFT_DOWN = Gdk::SHIFT_MASK,
					CONTROL_DOWN = Gdk::CONTROL_MASK,
					ALT_DOWN = Gdk::MOD1_MASK,
					META_DOWN = Gdk::META_MASK
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
					SHIFT_DOWN = Qt::ShiftModifier,
					CONTROL_DOWN = Qt::ControlModifier,
					ALT_DOWN = Qt::AltModifier,
					META_DOWN = Qt::MetaModifier
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
					SHIFT_DOWN = MOD_SHIFT,
					CONTROL_DOWN = MOD_CONTROL,
					ALT_DOWN = MOD_ALT,
					META_DOWN = MOD_WIN
#endif
					;

				/**
				 * Indicates the state of mouse buttons.
				 * @note Corresponds to @c GdkModifierType in GDK.
				 * @note Cooresponds to @c Qt#MouseButton in Qt.
				 * @note Defined here because these values also can be used as modifiers.
				 */
				typedef std::uint32_t MouseButton;

				/// @var BUTTON1_DOWN The Mouse Button1 (usually left button) is down.

				/// @var BUTTON2_DOWN The Mouse Button2 (usually middle button) is down.

				/// @var BUTTON3_DOWN The Mouse Button3 (usually right button) is down.

				/// @var BUTTON4_DOWN The Mouse Button4 (usually X1 button) is down.

				/// @var BUTTON5_DOWN The Mouse Button5 (usually X2 button) is down.

				static const MouseButton
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
					BUTTON1_DOWN = Gdk::BUTTON1_MASK,
					BUTTON2_DOWN = Gdk::BUTTON2_MASK,
					BUTTON3_DOWN = Gdk::BUTTON3_MASK,
					BUTTON4_DOWN = Gdk::BUTTON4_MASK,
					BUTTON5_DOWN = Gdk::BUTTON5_MASK
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
					BUTTON1_DOWN = Qt::LeftButton,
					BUTTON2_DOWN = Qt::RightButton,
					BUTTON3_DOWN = Qt::MiddleButton,
					BUTTON4_DOWN = Qt::ExtraButton1,
					BUTTON5_DOWN = Qt::ExtraButton2
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
					BUTTON1_DOWN = MK_LBUTTON,
					BUTTON2_DOWN = MK_RBUTTON,
					BUTTON3_DOWN = MK_MBUTTON,
					BUTTON4_DOWN = MK_XBUTTON1,
					BUTTON5_DOWN = MK_XBUTTON2
#endif
					;

				/// Indicates the state of modifier keys and mouse buttons.
				typedef std::uint32_t Modifiers;

			public:
				/**
				 * Returns @c true if the user input is the specified modifier down.
				 * @param modifier The modifiers to test
				 * @return true if @a input has @a mask
				 */
				bool hasModifier(Modifiers mask) const BOOST_NOEXCEPT {
					return (modifiers() & mask) != 0;
				}	
				/**
				 * Returns @c true if the user input has other than the specified modifiers.
				 * @param mask The modifiers to test
				 * @return true if @a input has modifiers other than @a mask
				 */
				bool hasModifierOtherThan(Modifiers mask) const BOOST_NOEXCEPT {
					return (modifiers() & ~mask) != 0;
				}
				Modifiers modifiers() const BOOST_NOEXCEPT {return modifiers_;}
				const std::time_t& timeStamp() const BOOST_NOEXCEPT {return timeStamp_;}
			protected:
				/**
				 * Protected constructor.
				 * @param modifiers The modifier flags
				 */
				explicit UserInput(Modifiers modifiers) : modifiers_(modifiers) {std::time(&timeStamp_);}
			private:
				const Modifiers modifiers_;
				std::time_t timeStamp_;
			};

			/// Abstract class represents a user input located at a specific position in the screen.
			class LocatedUserInput : public UserInput {
			public:
				/**
				 * Protected constructor.
				 * @param location The location
				 * @param modifiers The modifier flags
				 */
				LocatedUserInput(const graphics::Point& location, Modifiers modifiers) : UserInput(modifiers), location_(location) {
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
					Modifiers modifiers) : LocatedUserInput(location, modifiers), button_(button) {}
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
				MouseWheelInput(const graphics::Point& location, Modifiers modifiers,
					graphics::Dimension& rotation) : LocatedUserInput(location, modifiers), rotation_(rotation) {}
				/// Returns the mouse wheel rotation.
				const graphics::Dimension& rotation() const BOOST_NOEXCEPT {return rotation_;}
			private:
				const graphics::Dimension rotation_;
			};

			class KeyInput : public UserInput {
			public:
				/// Keyboard codes.
				/// @note Corresponds to @c Qt#Key in Qt.
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				typedef guint Code;	// GDK_KEY_* in gdk/gdkkeysyms.h
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				typedef int Code;	// Qt.Key
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				typedef WORD Code;	// VK_* in WinUser.h
#elif 0
				typedef std::uint32_t Code;
#endif
			public:
				KeyInput(Code keyboardCode, Modifiers modifiers, int repeatCount, int messageFlags)
					: UserInput(modifiers), keyboardCode_(keyboardCode), repeatCount_(repeatCount), messageFlags_(messageFlags) {}
				Code keyboardCode() const BOOST_NOEXCEPT {return keyboardCode_;}
			private:
				const Code keyboardCode_;
				const int repeatCount_, messageFlags_;
			};

		}
	}
}

#endif // !ASCENSION_USER_INPUT_HPP
