/**
 * @file mouse-button-input.hpp
 * Defines @c MouseButtonInput class.
 * @author exeal
 * @date 2010-11-10 Created (as user-input.hpp)
 * @date 2014-09-07 Separated from user-input.hpp
 */

#ifndef ASCENSION_MOUSE_BUTTON_INPUT_HPP
#define ASCENSION_MOUSE_BUTTON_INPUT_HPP
#include <ascension/viewer/widgetapi/event/located-user-input.hpp>

namespace ascension {
	namespace viewer {
		namespace widgetapi {
			namespace event {
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
						const KeyboardModifiers& modifiers) : LocatedUserInput(location, buttons, modifiers), button_(button) {}
					/// Returns the button that caused the event.
					MouseButton button() const BOOST_NOEXCEPT {
						return button_;
					}

				private:
					const MouseButton button_;
				};
			}
		}
	}
}

#endif // !ASCENSION_MOUSE_BUTTON_INPUT_HPP
