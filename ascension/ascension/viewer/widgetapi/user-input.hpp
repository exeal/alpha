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

			public:
				/**
				 * Returns @c true if the user input is the specified keyboard modifier down.
				 * @param modifier The modifiers to test
				 * @return @c true if @a input has @a mask
				 */
				bool hasModifier(KeyboardModifier mask) const BOOST_NOEXCEPT {
					return (modifiers() & mask) != 0;
				}	
				/**
				 * Returns @c true if the user input has other than the specified keyboard modifiers.
				 * @param mask The modifiers to test
				 * @return @c true if @a input has modifiers other than @a mask
				 */
				bool hasModifierOtherThan(KeyboardModifier mask) const BOOST_NOEXCEPT {
					return (modifiers() & ~mask) != 0;
				}
				/// Returns the keyboard modifier flags.
				KeyboardModifier modifiers() const BOOST_NOEXCEPT {
					return modifiers_;
				}
				/// Returns the time stamp.
				const std::time_t& timeStamp() const BOOST_NOEXCEPT {
					return timeStamp_;
				}

			protected:
				/**
				 * Protected constructor.
				 * @param modifiers The keyboard modifier flags
				 */
				explicit UserInput(KeyboardModifier modifiers) : modifiers_(modifiers) {
					std::time(&timeStamp_);
				}

			private:
				const KeyboardModifier modifiers_;
				std::time_t timeStamp_;
			};

			/// Abstract class represents a user input located at a specific position.
			class LocatedUserInput : public UserInput {
			public:
				/**
				 * Indicates the state of mouse buttons.
				 * @note Corresponds to @c GdkModifierType in GDK.
				 * @note Cooresponds to @c Qt#MouseButton in Qt.
				 * @note Corresponds to @c MK_* in Win32.
				 */
				typedef std::uint32_t MouseButton;

				/// @var NO_BUTTON Indicates no mouse buttons.

				/// @var BUTTON1_DOWN The Mouse Button1 (usually left button) is down.

				/// @var BUTTON2_DOWN The Mouse Button2 (usually middle button) is down.

				/// @var BUTTON3_DOWN The Mouse Button3 (usually right button) is down.

				/// @var BUTTON4_DOWN The Mouse Button4 (usually X1 button) is down.

				/// @var BUTTON5_DOWN The Mouse Button5 (usually X2 button) is down.

				static const MouseButton
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
					NO_BUTTON = 0,
					BUTTON1_DOWN = Gdk::BUTTON1_MASK,
					BUTTON2_DOWN = Gdk::BUTTON2_MASK,
					BUTTON3_DOWN = Gdk::BUTTON3_MASK,
					BUTTON4_DOWN = Gdk::BUTTON4_MASK,
					BUTTON5_DOWN = Gdk::BUTTON5_MASK
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
					NO_BUTTON = Qt::NoButton,
					BUTTON1_DOWN = Qt::LeftButton,
					BUTTON2_DOWN = Qt::RightButton,
					BUTTON3_DOWN = Qt::MiddleButton,
					BUTTON4_DOWN = Qt::ExtraButton1,
					BUTTON5_DOWN = Qt::ExtraButton2
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
					NO_BUTTON = 0,
					BUTTON1_DOWN = MK_LBUTTON,
					BUTTON2_DOWN = MK_RBUTTON,
					BUTTON3_DOWN = MK_MBUTTON,
					BUTTON4_DOWN = MK_XBUTTON1,
					BUTTON5_DOWN = MK_XBUTTON2
#endif
				;

			public:
				/**
				 * Protected constructor.
				 * @param location The location in the widget-local coordinates
				 * @param buttons The button state when the event was generated
				 * @param modifiers The keyboard modifier flags
				 */
				LocatedUserInput(const graphics::Point& location, MouseButton buttons,
					KeyboardModifier modifiers) : UserInput(modifiers), location_(location), buttons_(buttons) {}
				/// Returns the button state when the event was generated.
				MouseButton buttons() const BOOST_NOEXCEPT {
					return buttons_;
				}
				/// Returns the location in the widget-local coordinates.
				const graphics::Point& location() const BOOST_NOEXCEPT {
					return location_;
				}

			private:
				const graphics::Point location_;
				const MouseButton buttons_;
			};

			/// A @c MouseButtonInput represents a mouse button event.
			class MouseButtonInput : public LocatedUserInput {
			public:
				/**
				 * Constructor.
				 * @param location The location in the widget-local coordinates
				 * @param button The button that caused the event
				 * @param buttons The button state when the event was generated
				 * @param modifiers The keyboard modifier flags
				 */
				MouseButtonInput(const graphics::Point& location, MouseButton button, MouseButton buttons,
					KeyboardModifier modifiers) : LocatedUserInput(location, buttons, modifiers), button_(button) {}
				/// Returns the button that caused the event.
				MouseButton button() const BOOST_NOEXCEPT {
					return button_;
				}
			private:
				const MouseButton button_;
			};

			/// A @c MouseWheelEvent represents a mouse wheel event.
			class MouseWheelInput : public LocatedUserInput {
			public:
				/// Return type of @c #scrollType method.
				enum ScrollType {
					WHEEL_UNIT_SCROLL,	///< Represents scrolling by units.
					WHEEL_BLOCK_SCROLL	///< Represents scrolling by a block.
				};
			public:
				/**
				 * Creates @c MouseWheelInput object whose type is @c #WHEEL_UNIT_SCROLL.
				 * @param location The mouse location in widget-local coordinates
				 * @param buttons The button state when the event was generated
				 * @param modifiers The keyboard modifier flags
				 * @param scrollAmount The number of units to be scrolled
				 * @param wheelRotation The number of notches by which the mouse wheel was rotated
				 */
				MouseWheelInput(
					const graphics::Point& location, MouseButton buttons, KeyboardModifier modifiers,
					const graphics::geometry::BasicDimension<unsigned int>& scrollAmount,
					const graphics::geometry::BasicDimension<double>& wheelRotation)
					: LocatedUserInput(location, buttons, modifiers), scrollAmount_(scrollAmount), wheelRotation_(wheelRotation) {}
				/**
				 * Creates @c MouseWheelInput object whose type is @c #WHEEL_BLOCK_SCROLL.
				 * @param location The mouse location in widget-local coordinates
				 * @param buttons The button state when the event was generated
				 * @param modifiers The keyboard modifier flags
				 * @param wheelRotation The number of notches by which the mouse wheel was rotated
				 */
				MouseWheelInput(
					const graphics::Point& location, MouseButton buttons, KeyboardModifier modifiers,
					const graphics::geometry::BasicDimension<double>& wheelRotation)
					: LocatedUserInput(location, buttons, modifiers), wheelRotation_(wheelRotation) {}
				/// Returns the number of units that should be scrolled per click of mouse wheel rotation.
				/// Only valid if @c #scrollType returns @c #WHEEL_UNIT_SCROLL.
				const boost::optional<graphics::geometry::BasicDimension<unsigned int>>& scrollAmount() const BOOST_NOEXCEPT {
					return scrollAmount_;
				}
				/// Returns the type of scrolling that should take place in response to this event.
				ScrollType scrollType() const BOOST_NOEXCEPT {
					return (scrollAmount() != boost::none) ? WHEEL_UNIT_SCROLL : WHEEL_BLOCK_SCROLL;
				}
				/// Returns the number of units to scroll when scroll type is @c #WHEEL_UNIT_SCROLL.
				boost::optional<graphics::geometry::BasicDimension<double>> unitsToScroll() const BOOST_NOEXCEPT {
					const auto& amount = scrollAmount();
					if(amount == boost::none)
						return boost::none;
					return graphics::geometry::BasicDimension<double>(
						graphics::geometry::_dx = graphics::geometry::dx(*amount) * graphics::geometry::dx(wheelRotation()),
						graphics::geometry::_dy = graphics::geometry::dy(*amount) * graphics::geometry::dx(wheelRotation()));
				}
				/// Returns the number of notches the mouse wheel was rotated.
				const graphics::geometry::BasicDimension<double>& wheelRotation() const BOOST_NOEXCEPT {
					return wheelRotation_;
				}

			private:
				const boost::optional<graphics::geometry::BasicDimension<unsigned int>> scrollAmount_;
				const graphics::geometry::BasicDimension<double> wheelRotation_;
			};

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

#endif // !ASCENSION_USER_INPUT_HPP
