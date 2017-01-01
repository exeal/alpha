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
					MouseButtonInput(const graphics::Point& location, MouseButton button, const MouseButtons& buttons,
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
		
	template<typename Model, typename Native> inline Model fromNative(const Native& object) {
		return fromNative<Model>(object);
	}

	template<typename Native, typename Model> inline Native toNative(const Model& object) {
		return toNative(object, static_cast<const Native*>(nullptr));
	}

#if ASCENSION_SUPPORTS_WINDOW_SYSTEM(WIN32)
	namespace win32 {
		inline viewer::widgetapi::event::MouseButtonInput makeMouseButtonInput(viewer::widgetapi::event::MouseButton button, WPARAM wp, LPARAM lp) {
			return viewer::widgetapi::event::MouseButtonInput(makeMouseLocation<graphics::Point>(lp), button,
				ascension::fromNative<viewer::widgetapi::event::MouseButtons>(wp), ascension::fromNative<viewer::widgetapi::event::KeyboardModifiers>(wp));
		}
	}
#endif // ASCENSION_SUPPORTS_WINDOW_SYSTEM(WIN32)
}

#endif // !ASCENSION_MOUSE_BUTTON_INPUT_HPP
