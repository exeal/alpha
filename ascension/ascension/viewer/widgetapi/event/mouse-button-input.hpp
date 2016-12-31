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

#if ASCENSION_SUPPORTS_WINDOW_SYSTEM(WIN32)
	namespace win32 {
		inline viewer::widgetapi::event::MouseButtonInput makeMouseButtonInput(viewer::widgetapi::event::LocatedUserInput::MouseButton button, WPARAM wp, LPARAM lp) {
			return viewer::widgetapi::event::MouseButtonInput(makeMouseLocation<graphics::Point>(lp), button, makeModifiers(wp));
		}

		template<typename Point>
		inline viewer::widgetapi::event::MouseButtonInput makeMouseButtonInput(DWORD keyState, const Point& location) {
			viewer::widgetapi::UserInput::MouseButton buttons = 0;
			if(boole(keyState & MK_LBUTTON))
				buttons |= viewer::widgetapi::UserInput::BUTTON1_DOWN;
			if(boole(keyState & MK_MBUTTON))
				buttons |= viewer::widgetapi::UserInput::BUTTON2_DOWN;
			if(boole(keyState & MK_RBUTTON))
				buttons |= viewer::widgetapi::UserInput::BUTTON3_DOWN;
			if(boole(keyState & MK_XBUTTON1))
				buttons |= viewer::widgetapi::UserInput::BUTTON4_DOWN;
			if(boole(keyState & MK_XBUTTON2))
				buttons |= viewer::widgetapi::UserInput::BUTTON5_DOWN;
			viewer::widgetapi::UserInput::ModifierKey modifiers = 0;
			if(boole(keyState & MK_CONTROL))
				modifiers |= viewer::widgetapi::UserInput::CONTROL_DOWN;
			if(boole(keyState & MK_SHIFT))
				modifiers |= viewer::widgetapi::UserInput::SHIFT_DOWN;
			if(boole(keyState & MK_ALT))
				modifiers |= viewer::widgetapi::UserInput::ALT_DOWN;
			return viewer::widgetapi::MouseButtonInput(
				boost::geometry::make<graphics::Point>(
					static_cast<typename boost::geometry::coordinate_type<graphics::Point>::type>(geometry::x(location)),
					static_cast<typename boost::geometry::coordinate_type<graphics::Point>::type>(geometry::y(location))),
				buttons, modifiers);
		}
	}
#endif // ASCENSION_SUPPORTS_WINDOW_SYSTEM(WIN32)
}

#endif // !ASCENSION_MOUSE_BUTTON_INPUT_HPP
