/**
 * @file user-input.hpp
 * Defines @c MouseWheelInput class.
 * @author exeal
 * @date 2010-11-10 Created (as user-input.hpp)
 * @date 2014-09-07 Separated from user-input.hpp
 */

#ifndef ASCENSION_MOUSE_WHEEL_INPUT_HPP
#define ASCENSION_MOUSE_WHEEL_INPUT_HPP

#include <ascension/viewer/widgetapi/event/located-user-input.hpp>

namespace ascension {
	namespace viewers {
		namespace widgetapi {
			namespace event {
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
			}
		}
	}
}

#endif // !ASCENSION_MOUSE_WHEEL_INPUT_HPP
