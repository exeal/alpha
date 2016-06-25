/**
 * @file flow-relative-direction.hpp
 * Defines flow-relative directional and dimensional terms.
 * @date 2012-03-31 created
 * @date 2012-2014 was directions.hpp
 * @date 2015-01-09 Separated from directions.hpp
 * @date 2016-06-20 Renamed from flow-relative-direction.hpp
 * @see direction.hpp, line-relative-direction.hpp, physical-direction.hpp, writing-mode.hpp
 */

#ifndef ASCENSION_FLOW_RELATIVE_DIRECTION_HPP
#define ASCENSION_FLOW_RELATIVE_DIRECTION_HPP
#include <ascension/corelib/basic-exceptions.hpp>	// UnknownValueException
#include <ascension/corelib/future/scoped-enum-emulation.hpp>
#include <type_traits>	// std.extent

namespace ascension {
	namespace presentation {
		/// @defgroup flow_relative_directions_dimensions Flow-relative Directions and Dimensions
		/// @see CSS Writing Modes Module Level 3, 6.1 Abstract Dimensions
		///      (http://www.w3.org/TR/css-writing-modes-3/#abstract-box)
		/// @see CSS Writing Modes Module Level 3, 6.2 Flow-relative Directions
		///      (http://www.w3.org/TR/css-writing-modes-3/#logical-directions)
		/// @{
		/**
		 * Defines the 'flow-relative directions' which are defined relative to the flow of content on the page.
		 * @see graphics#PhysicalDirection, graphics#font#LineRelativeDirection
		 */
		ASCENSION_SCOPED_ENUM_DECLARE_BEGIN(FlowRelativeDirection)
			/// 'block-start' means the side that comes earlier in the block progression.
			BLOCK_START,
			/// 'block-end' means the side opposite 'block-start'.
			BLOCK_END,
			/// 'inline-start' means the side from which text of the inline base direction would start.
			INLINE_START,
			/// 'inline-end' means the side opposite 'inline-start'.
			INLINE_END
#ifndef ASCENSION_NO_XSL_FLOW_RELATIVE_DIRECTIONS
			,
			/// 'before' -- Nominally the side that comes earlier in the block progression.
			BEFORE = BLOCK_START,
			/// 'after' -- The side opposite 'before'.
			AFTER = BLOCK_END,
			/// 'start' -- Nominally the side from which text of its inline base direction will start.
			START = INLINE_START,
			/// 'end' -- The side opposite 'start'.
			END = INLINE_END
#endif // !ASCENSION_NO_XSL_FLOW_RELATIVE_DIRECTIONS
		ASCENSION_SCOPED_ENUM_DECLARE_END(FlowRelativeDirection)

		/**
		 * Returns direction opposite @a direction.
		 * @throw UnknownValueException @a direction is invalid
		 */
		inline FlowRelativeDirection operator!(FlowRelativeDirection direction) {
			static const FlowRelativeDirection opposites[4] = {
				FlowRelativeDirection::BLOCK_END, FlowRelativeDirection::BLOCK_START,
				FlowRelativeDirection::INLINE_END, FlowRelativeDirection::INLINE_START
			};
			const std::size_t index = boost::underlying_cast<std::size_t>(direction);
			if(index >= std::extent<decltype(opposites)>::value)
				throw UnknownValueException("direction");
			return opposites[index];
		}
		/// @}
	}
}

#endif // !ASCENSION_FLOW_RELATIVE_DIRECTION_HPP
