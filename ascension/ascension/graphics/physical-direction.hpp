/**
 * @file physical-direction.hpp
 * Defines @c PhysicalDirection enumeration.
 * @date 2012-03-31 created
 * @date 2012-2014 was directions.hpp
 * @date 2015-01-09 Separated from directions.hpp
 * @date 2016-06-26 Renamed from physical-directions-dimensions.hpp
 * @see direction.hpp, flow-relative-direction.hpp, line-relative-direction.hpp, writing-mode.hpp
 */

#ifndef ASCENSION_PHYSICAL_DIRECTION_HPP
#define ASCENSION_PHYSICAL_DIRECTION_HPP
#include <ascension/corelib/basic-exceptions.hpp>	// UnknownValueException
#include <ascension/corelib/future/scoped-enum-emulation.hpp>
#include <type_traits>

namespace ascension {
	namespace graphics {
		/// @defgroup physical_directions_dimensions Physical Directions and Dimensions
		/// @see CSS Writing Modes Module Level 3, 6 Abstract Box Terminology
		///      (http://www.w3.org/TR/css-writing-modes-3/#abstract-box)
		/// @{
		/**
		 * Defines physical directions.
		 * @see font#LineRelativeDirection, presentation#FlowRelativeDirection
		 */
		ASCENSION_SCOPED_ENUM_DECLARE_BEGIN(PhysicalDirection)
			TOP,	///< Physical top.
			RIGHT,	///< Physical right.
			BOTTOM,	///< Physical bottom.
			LEFT	///< Physical left.
		ASCENSION_SCOPED_ENUM_DECLARE_END(PhysicalDirection)

		/**
		 * Returns direction opposite @a direction.
		 * @throw UnknownValueException @a direction is invalid
		 */
		inline BOOST_SCOPED_ENUM_NATIVE(PhysicalDirection) operator!(PhysicalDirection direction) {
			static const PhysicalDirection opposites[4] = {
				PhysicalDirection::BOTTOM, PhysicalDirection::LEFT,
				PhysicalDirection::TOP, PhysicalDirection::RIGHT
			};
			const std::size_t index = boost::underlying_cast<std::size_t>(direction);
			if(index >= std::extent<decltype(opposites)>::value)
				throw UnknownValueException("direction");
			return boost::native_value(opposites[index]);
		}
		/// @}
	}
}

#endif // !ASCENSION_PHYSICAL_DIRECTION_HPP
