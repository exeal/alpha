/**
 * @file located-user-input.hpp
 * Defines @c LocatedUserInput class.
 * @author exeal
 * @date 2010-11-10 Created (as user-input.hpp)
 * @date 2014-09-07 Separated from user-input.hpp
 */

#ifndef ASCENSION_LOCATED_USER_INPUT_HPP
#define ASCENSION_LOCATED_USER_INPUT_HPP
#include <ascension/graphics/geometry/point.hpp>
#include <ascension/viewer/widgetapi/event/user-input.hpp>

namespace ascension {
	namespace viewer {
		namespace widgetapi {
			namespace event {
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

					/// @var #NO_BUTTON
					/// Indicates no mouse buttons.

					/// @var #BUTTON1_DOWN
					/// The Mouse Button1 (usually left button) is down.

					/// @var #BUTTON2_DOWN
					/// The Mouse Button2 (usually middle button) is down.

					/// @var #BUTTON3_DOWN
					/// The Mouse Button3 (usually right button) is down.

					/// @var #BUTTON4_DOWN
					/// The Mouse Button4 (usually X1 button) is down.

					/// @var #BUTTON5_DOWN
					/// The Mouse Button5 (usually X2 button) is down.

				public:
					/**
					 * Protected constructor.
					 * @param location The location in the widget-local coordinates
					 * @param buttons The button state when the event was generated
					 * @param modifiers The keyboard modifier flags
					 */
					LocatedUserInput(const graphics::Point& location, MouseButton buttons,
						const KeyboardModifiers& modifiers) : UserInput(modifiers), location_(location), buttons_(buttons) {}
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
			}
		}
	}
}

#endif // !ASCENSION_LOCATED_USER_INPUT_HPP
