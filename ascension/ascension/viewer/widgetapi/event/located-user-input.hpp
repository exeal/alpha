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
#include <ascension/viewer/widgetapi/event/mouse-button.hpp>
#include <ascension/viewer/widgetapi/event/user-input.hpp>

namespace ascension {
	namespace viewer {
		namespace widgetapi {
			namespace event {
				/// Abstract class represents a user input located at a specific position.
				class LocatedUserInput : public UserInput {
				public:
					/**
					 * Protected constructor.
					 * @param location The location in the widget-local coordinates
					 * @param buttons The button state when the event was generated
					 * @param modifiers The keyboard modifier flags
					 */
					LocatedUserInput(const graphics::Point& location, const MouseButtons& buttons,
						const KeyboardModifiers& modifiers) : UserInput(modifiers), location_(location), buttons_(buttons) {}
					/// Returns the button state when the event was generated.
					const MouseButtons& buttons() const BOOST_NOEXCEPT {
						return buttons_;
					}
					/// Returns the location in the widget-local coordinates.
					const graphics::Point& location() const BOOST_NOEXCEPT {
						return location_;
					}

				private:
					const graphics::Point location_;
					const MouseButtons buttons_;
				};
			}
		}
	}

#if ASCENSION_SUPPORTS_WINDOW_SYSTEM(WIN32)
	namespace win32 {
		template<typename Point>
		inline viewer::widgetapi::event::LocatedUserInput makeLocatedUserInput(DWORD keyState, const Point& location) {
			return viewer::widgetapi::event::LocatedUserInput(
				boost::geometry::make<graphics::Point>(
					static_cast<typename boost::geometry::coordinate_type<graphics::Point>::type>(boost::geometry::get<0>(location)),
					static_cast<typename boost::geometry::coordinate_type<graphics::Point>::type>(boost::geometry::get<1>(location))),
				ascension::fromNative<viewer::widgetapi::event::MouseButtons>(keyState), ascension::fromNative<viewer::widgetapi::event::KeyboardModifiers>(keyState));
		}
	}
#endif // ASCENSION_SUPPORTS_WINDOW_SYSTEM(WIN32)
}

#endif // !ASCENSION_LOCATED_USER_INPUT_HPP
