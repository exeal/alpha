/**
 * @file line-relative-direction.hpp
 * Defines @c LineRelativeDirection enumeration.
 * @date 2012-03-31 created
 * @date 2012-2014 was directions.hpp
 * @date 2015-01-09 Separated from directions.hpp
 * @date 2016-06-26 Renamed from line-relative-directions-dimensions.hpp
 * @see direction.hpp, flow-relative-direction.hpp, physical-direction.hpp, writing-mode.hpp
 */

#ifndef ASCENSION_LINE_RELATIVE_DIRECTION_HPP
#define ASCENSION_LINE_RELATIVE_DIRECTION_HPP
#include <ascension/corelib/basic-exceptions.hpp>	// UnknownValueException
#include <ascension/corelib/future/scoped-enum-emulation.hpp>
#include <type_traits>

namespace ascension {
	namespace graphics {
		namespace font {
			/// @defgroup line_relative_directions_dimensions Line-relative Directions and Dimensions
			/// @see CSS Writing Modes Module Level 3, 6.3 Line-relative Directions
			///      (http://www.w3.org/TR/css-writing-modes-3/#line-directions)
			/// @{
			/**
			 * Defines line-relative directions.
			 * @see PhysicalDirection, presentation#FlowRelativeDirection
			 */
			ASCENSION_SCOPED_ENUM_DECLARE_BEGIN(LineRelativeDirection)
				/// 'over' means nominally the side that corresponds to the ascender side or ÅgtopÅh side of a line box.
				OVER,
				/// 'under' means opposite of 'over': the line-relative ÅgbottomÅh or descender side.
				UNDER,
				/// 'line-left' means nominally the side from which LTR text would start.
				LINE_LEFT,
				/// 'line-right' means nominally the side from which RTL text would start.
				LINE_RIGHT,
				/// 'line-over' is an alias of 'over'.
				LINE_OVER = OVER,
				/// 'line-under' is an alias of 'under'.
				LINE_UNDER = UNDER
			ASCENSION_SCOPED_ENUM_DECLARE_END(LineRelativeDirection)

			/**
			 * Returns direction opposite @a direction.
			 * @throw UnknownValueException @a direction is invalid
			 */
			inline BOOST_SCOPED_ENUM_NATIVE(LineRelativeDirection) operator!(BOOST_SCOPED_ENUM_NATIVE(LineRelativeDirection) direction) {
				static const BOOST_SCOPED_ENUM_NATIVE(LineRelativeDirection) opposites[4] = {
					LineRelativeDirection::LINE_UNDER, LineRelativeDirection::LINE_OVER,
					LineRelativeDirection::LINE_RIGHT, LineRelativeDirection::LINE_LEFT
				};
				const std::size_t index = static_cast<std::size_t>(direction);
				if(index >= std::extent<decltype(opposites)>::value)
					throw UnknownValueException("direction");
				return opposites[index];
			}

#ifdef BOOST_NO_CXX11_SCOPED_ENUMS
			/// @overload
			inline LineRelativeDirection operator!(LineRelativeDirection direction) {
				return LineRelativeDirection(!boost::native_value(direction));
			}
#endif // BOOST_NO_CXX11_SCOPED_ENUMS
			/// @}
		}
	}
}

#endif // !ASCENSION_LINE_RELATIVE_DIRECTION_HPP
