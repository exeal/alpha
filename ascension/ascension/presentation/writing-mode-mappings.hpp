/**
 * @file writing-mode-mappings.hpp
 * Provides free functions map between abstract and physical directions, axes and bounds.
 * @author exeal
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2011 was presentation.hpp
 * @date 2011-05-04 text-style.hpp separated from presentation.hpp
 * @date 2011-07-24 separated from text-style.hpp
 * @date 2013-03-17 separated from writing-mode.hpp
 * @see directions.hpp, writing-mode.hpp
 * @see CSS Writing Modes Module Level 3, 6.4 Abstract-to-Physical Mappings
 *      (http://www.w3.org/TR/css3-writing-modes/#logical-to-physical)
 */

#ifndef ASCENSION_WRITING_MODE_MAPPINGS_HPP
#define ASCENSION_WRITING_MODE_MAPPINGS_HPP
#include <ascension/graphics/physical-directions-dimensions.hpp>
#include <ascension/graphics/font/line-relative-directions-dimensions.hpp>
#include <ascension/presentation/flow-relative-directions-dimensions.hpp>
#include <ascension/presentation/writing-mode.hpp>	// WritingMode, ...
#include <type_traits>

namespace ascension {
	namespace presentation {
		static_assert(
			static_cast<int>(graphics::font::LineRelativeDirection::OVER) == 0
			&& static_cast<int>(graphics::font::LineRelativeDirection::UNDER) == 1
			&& static_cast<int>(graphics::font::LineRelativeDirection::LINE_LEFT) == 2
			&& static_cast<int>(graphics::font::LineRelativeDirection::LINE_RIGHT) == 3,
			"");
		static_assert(
			static_cast<int>(FlowRelativeDirection::BEFORE) == 0
			&& static_cast<int>(FlowRelativeDirection::AFTER) == 1
			&& static_cast<int>(FlowRelativeDirection::START) == 2
			&& static_cast<int>(FlowRelativeDirection::END) == 3,
			"");
		static_assert(
			static_cast<int>(graphics::PhysicalDirection::TOP) == 0
			&& static_cast<int>(graphics::PhysicalDirection::RIGHT) == 1
			&& static_cast<int>(graphics::PhysicalDirection::BOTTOM) == 2
			&& static_cast<int>(graphics::PhysicalDirection::LEFT) == 3,
			"");

		/// @defgroup abstract_physical_directions Abstract and Physical Directions Mappings
		/// @brief Free functions to map between abstract and physical directions.
		/// @{
		/**
		 * Maps flow-relative direction into corresponding physical one.
		 * @param writingMode The writing mode
		 * @param from The flow-relative direction to map
		 * @return The mapped physical direction
		 * @throw UnknownValueException @a writingMode or @a from is invalid
		 * @see mapPhysicalToFlowRelative
		 */
		inline graphics::PhysicalDirection mapFlowRelativeToPhysical(const WritingMode& writingMode, FlowRelativeDirection from) {
			using graphics::PhysicalDirection;
			static const PhysicalDirection
				horizontalMappings[]  = {PhysicalDirection::TOP, PhysicalDirection::BOTTOM, PhysicalDirection::LEFT, PhysicalDirection::RIGHT},
				verticalMappings[] = {PhysicalDirection::RIGHT, PhysicalDirection::LEFT, PhysicalDirection::TOP, PhysicalDirection::BOTTOM};
			if(boost::native_value(from) < 0 || boost::underlying_cast<std::size_t>(from) >= std::extent<decltype(horizontalMappings)>::value)
				throw UnknownValueException("from");
			if(writingMode.inlineFlowDirection == RIGHT_TO_LEFT && (from == FlowRelativeDirection::START || from == FlowRelativeDirection::END))
				from = !from;
			if(isHorizontal(writingMode.blockFlowDirection))	// this may throw UnknownValueException
				return horizontalMappings[boost::underlying_cast<std::size_t>(from)];
			else {
				if(from == FlowRelativeDirection::BEFORE || from == FlowRelativeDirection::AFTER) {
					if(writingMode.blockFlowDirection == VERTICAL_LR)
						from = !from;
				} else if(resolveTextOrientation(writingMode) == SIDEWAYS_LEFT)
					from = !from;
				return verticalMappings[boost::underlying_cast<std::size_t>(from)];
			}
		}

		/**
		 * Maps abstract direction/axes/bounds into corresponding physical one.
		 * @param writingMode The writing mode
		 * @param from The abstract direction/axes/bounds to map
		 * @return The mapped physical direction/axes/bounds
		 * @throw UnknownValueException @a writingMode or @a from is invalid
		 * @see mapPhysicalToLineRelative
		 */
		inline graphics::PhysicalDirection mapLineRelativeToPhysical(const WritingMode& writingMode, graphics::font::LineRelativeDirection from) {
			using graphics::PhysicalDirection;
			static const PhysicalDirection
				horizontalMappings[] = {PhysicalDirection::TOP, PhysicalDirection::BOTTOM, PhysicalDirection::LEFT, PhysicalDirection::RIGHT},
				verticalMappings[] = {PhysicalDirection::RIGHT, PhysicalDirection::LEFT, PhysicalDirection::TOP, PhysicalDirection::BOTTOM};
			if(boost::native_value(from) < 0 || boost::underlying_cast<std::size_t>(from) >= std::extent<decltype(horizontalMappings)>::value)
				throw UnknownValueException("from");
			if(isHorizontal(writingMode.blockFlowDirection))	// this may throw UnknownValueException
				return horizontalMappings[boost::underlying_cast<std::size_t>(from)];
			else
				return verticalMappings[boost::underlying_cast<std::size_t>((resolveTextOrientation(writingMode) != SIDEWAYS_LEFT) ? from : !from)];
		}

		/**
		 * Maps physical direction into corresponding flow-relative one.
		 * @param writingMode The writing mode
		 * @param from The physical direction to map
		 * @return The mapped flow-relative direction
		 * @throw UnknownValueException @a writingMode or @a from is invalid
		 * @see mapFlowRelativeToPhysical
		 */
		inline FlowRelativeDirection mapPhysicalToFlowRelative(const WritingMode& writingMode, graphics::PhysicalDirection from) {
			static const FlowRelativeDirection
				horizontalMappings[] = {FlowRelativeDirection::BEFORE, FlowRelativeDirection::END, FlowRelativeDirection::AFTER, FlowRelativeDirection::START},
				verticalMappings[] = {FlowRelativeDirection::START, FlowRelativeDirection::BEFORE, FlowRelativeDirection::END, FlowRelativeDirection::AFTER};
			if(boost::native_value(from) < 0 || boost::underlying_cast<std::size_t>(from) >= std::extent<decltype(horizontalMappings)>::value)
				throw UnknownValueException("from");
			if(isHorizontal(writingMode.blockFlowDirection)) {	// this may throw UnknownValueException
				if(writingMode.inlineFlowDirection == RIGHT_TO_LEFT && (from == graphics::PhysicalDirection::LEFT || from == graphics::PhysicalDirection::RIGHT))
					from = !from;
				return horizontalMappings[boost::underlying_cast<std::size_t>(from)];
			} else {
				if(from == graphics::PhysicalDirection::LEFT || from == graphics::PhysicalDirection::RIGHT) {
					if(writingMode.blockFlowDirection == VERTICAL_LR)
						from = !from;
				} else {
					if(writingMode.inlineFlowDirection == RIGHT_TO_LEFT)
						from = !from;
					if(resolveTextOrientation(writingMode) == SIDEWAYS_LEFT)
						from = !from;
				}
				return verticalMappings[boost::underlying_cast<std::size_t>(from)];
			}
		}

		/**
		 * Maps physical direction into corresponding line-relative one.
		 * @param writingMode The writing mode
		 * @param from The physical direction to map
		 * @return The mapped line-relative direction
		 * @throw UnknownValueException @a writingMode or @a from is invalid
		 * @see mapLineRelativeToPhysical
		 */
		inline graphics::font::LineRelativeDirection mapPhysicalToLineRelative(const WritingMode& writingMode, graphics::PhysicalDirection from) {
			using graphics::font::LineRelativeDirection;
			static const LineRelativeDirection
				horizontalMappings[] = {LineRelativeDirection::OVER, LineRelativeDirection::LINE_RIGHT, LineRelativeDirection::UNDER, LineRelativeDirection::LINE_LEFT},
				verticalMappings[] = {LineRelativeDirection::LINE_LEFT, LineRelativeDirection::OVER, LineRelativeDirection::LINE_RIGHT, LineRelativeDirection::UNDER};
			if(boost::native_value(from) < 0 || boost::underlying_cast<std::size_t>(from) >= std::extent<decltype(horizontalMappings)>::value)
				throw UnknownValueException("from");
			if(isHorizontal(writingMode.blockFlowDirection))	// this may throw UnknownValueException
				return horizontalMappings[boost::underlying_cast<std::size_t>(from)];
			else
				return verticalMappings[boost::underlying_cast<std::size_t>((resolveTextOrientation(writingMode) != SIDEWAYS_LEFT) ? from : !from)];
		}
		/// @}

		/// @defgroup abstract_physical_axes Abstract and Physical Axes Mappings
		/// @brief Free functions map between abstract and physical axes.
		/// @note These mappings are not described in "W3C CSS Writing Modes Level 3". Any mappings are performed based
		///       on the neutral origin (0, 0) which is a both abstract and physical point.
		/// @{
		/**
		 * Maps flow-relative axes into corresponding physical one.
		 * @tparam T An arithmetic type
		 * @param writingMode The writing mode
		 * @param from The flow-relative axes to map
		 * @return The mapped physical axes
		 * @throw UnknownValueException @a writingMode or @a from is invalid
		 * @see mapPhysicalToFlowRelative
		 */
		template<typename T>
		inline graphics::PhysicalTwoAxes<T> mapFlowRelativeToPhysical(const WritingMode& writingMode,
				const FlowRelativeTwoAxes<T>& from, typename std::enable_if<std::is_arithmetic<T>::value>::type* = nullptr) {
			switch(writingMode.blockFlowDirection) {
				case HORIZONTAL_TB:
					return graphics::PhysicalTwoAxes<T>(
						graphics::_x = (writingMode.inlineFlowDirection == LEFT_TO_RIGHT) ? +from.ipd() : -from.ipd(),
						graphics::_y = from.bpd()
					);
				case VERTICAL_RL:
				case VERTICAL_LR: {
					bool ttb = writingMode.inlineFlowDirection == LEFT_TO_RIGHT;
					ttb = (resolveTextOrientation(writingMode) != SIDEWAYS_LEFT) ? ttb : !ttb;
					return graphics::PhysicalTwoAxes<T>(
						graphics::_x = (writingMode.blockFlowDirection == VERTICAL_RL) ? -from.bpd() : +from.bpd(),
						graphics::_y = ttb ? +from.ipd() : -from.ipd()
					);
				}
				default:
					throw UnknownValueException("writingMode.blockFlowDirection");
			}
		}

		/**
		 * Maps physical axes into corresponding flow-relative one.
		 * @tparam T An arithmetic type
		 * @param writingMode The writing mode
		 * @param from The physical axes to map
		 * @return The mapped flow-relative axes
		 * @throw UnknownValueException @a writingMode or @a from is invalid
		 * @see mapFlowRelativeToPhysical
		 */
		template<typename T>
		inline FlowRelativeTwoAxes<T> mapPhysicalToFlowRelative(const WritingMode& writingMode,
				const graphics::PhysicalTwoAxes<T>& from, typename std::enable_if<std::is_arithmetic<T>::value>::type* = nullptr) {
			switch(writingMode.blockFlowDirection) {
				case HORIZONTAL_TB:
					return FlowRelativeTwoAxes<T>(
						_ipd = (writingMode.inlineFlowDirection == LEFT_TO_RIGHT) ? +from.x() : -from.x(),
						_bpd = from.y()
					);
				case VERTICAL_RL:
				case VERTICAL_LR: {
					bool ttb = writingMode.inlineFlowDirection == LEFT_TO_RIGHT;
					ttb = (resolveTextOrientation(writingMode) != SIDEWAYS_LEFT) ? ttb : !ttb;
					return FlowRelativeTwoAxes<T>(
						_ipd = ttb ? +from.y() : -from.y(),
						_bpd = (writingMode.blockFlowDirection == VERTICAL_RL) ? -from.x() : +from.x()
					);
				}
				default:
					throw UnknownValueException("writingMode.blockFlowDirection");
			}
		}
		/// @}

		/// @defgroup abstract_physical_bounds Abstract and Physical Bounds Mappings
		/// @brief Free functions map between abstract and physical bounds.
		/// @{ 
		/**
		 * Maps flow-relative bounds into corresponding physical one.
		 * @param writingMode The writing mode
		 * @param from The flow-relative bounds to map
		 * @return The mapped physical bounds
		 * @throw UnknownValueException @a writingMode or @a from is invalid
		 * @see mapPhysicalToFlowRelative
		 */
		template<typename T>
		inline graphics::PhysicalFourSides<T> mapFlowRelativeToPhysical(const WritingMode& writingMode, const FlowRelativeFourSides<T>& from) {
			const bool ltr = writingMode.blockFlowDirection == LEFT_TO_RIGHT;
			if(isHorizontal(writingMode.blockFlowDirection))
				return graphics::PhysicalFourSides<T>(
					graphics::_top = from.blockStart(),
					graphics::_right = ltr ? from.inlineStart() : from.inlineEnd(),
					graphics::_bottom = from.blockEnd(),
					graphics::_left = ltr ? from.inlineEnd() : from.inlineStart()
				);
			else {
				const bool verticalRl = writingMode.blockFlowDirection == VERTICAL_RL;
				bool ttb = ltr;
				if(resolveTextOrientation(writingMode) == SIDEWAYS_LEFT)
					ttb = !ttb;
				return graphics::PhysicalFourSides<T>(
					graphics::_top = ttb ? from.inlineStart() : from.inlineEnd(),
					graphics::_right = verticalRl ? from.blockStart() : from.blockEnd(),
					graphics::_bottom = ttb ? from.inlineEnd() : from.inlineStart(),
					graphics::_left = verticalRl ? from.blockEnd() : from.blockStart()
				);
			}
		}

		/**
		 * Maps line-relative bounds into corresponding physical one.
		 * @param writingMode The writing mode
		 * @param from The line-relative bounds to map
		 * @return The mapped physical bounds
		 * @throw UnknownValueException @a writingMode or @a from is invalid
		 * @see mapPhysicalToFlowRelative
		 */
		template<typename T>
		inline graphics::PhysicalFourSides<T> mapLineRelativeToPhysical(const WritingMode& writingMode, const graphics::font::LineRelativeFourSides<T>& from) {
			using namespace graphics;
			if(isHorizontal(writingMode.blockFlowDirection))
				return PhysicalFourSides<T>(_top = from.over(), _right = from.lineRight(), _bottom = from.under(), _left = from.lineLeft());
			else if(resolveTextOrientation(writingMode) != SIDEWAYS_LEFT)
				return PhysicalFourSides<T>(_top = from.lineLeft(), _right = from.over(), _bottom = from.lineRight(), _left = from.under());
			else
				return PhysicalFourSides<T>(_top = from.lineRight(), _right = from.under(), _bottom = from.lineLeft(), _left = from.over());
		}

		/**
		 * Maps physical bounds into corresponding flow-relative one.
		 * @param writingMode The writing mode
		 * @param from The physical bounds to map
		 * @return The mapped flow-relative bounds
		 * @throw UnknownValueException @a writingMode or @a from is invalid
		 * @see mapFlowRelativeToPhysical
		 */
		template<typename T>
		inline FlowRelativeFourSides<T> mapPhysicalToFlowRelative(const WritingMode& writingMode, const graphics::PhysicalFourSides<T>& from) {
			const bool ltr = writingMode.blockFlowDirection == LEFT_TO_RIGHT;
			if(isHorizontal(writingMode.blockFlowDirection))
				return FlowRelativeFourSides<T>(
					_blockStart = from.top(),
					_blockEnd = from.bottom(),
					_inlineStart = ltr ? from.left() : from.right(),
					_inlineEnd = ltr ? from.right() : from.left()
				);
			else {
				const bool verticalRl = writingMode.blockFlowDirection == VERTICAL_RL;
				bool ttb = ltr;
				if(resolveTextOrientation(writingMode) == SIDEWAYS_LEFT)
					ttb = !ttb;
				return FlowRelativeFourSides<T>(
					_blockStart = verticalRl ? from.right() : from.left(),
					_blockEnd = verticalRl ? from.left() : from.right(),
					_inlineStart = ttb ? from.top() : from.bottom(),
					_inlineEnd = ttb ? from.bottom() : from.top()
				);
			}
		}

		/**
		 * Maps physical bounds into corresponding line-relative one.
		 * @param writingMode The writing mode
		 * @param from The physical bounds to map
		 * @return The mapped line-relative bounds
		 * @throw UnknownValueException @a writingMode or @a from is invalid
		 * @see mapLineRelativeToPhysical
		 */
		template<typename T>
		inline graphics::font::LineRelativeFourSides<T> mapPhysicalToLineRelative(const WritingMode& writingMode, const graphics::PhysicalFourSides<T>& from) {
			using namespace graphics::font;
			if(isHorizontal(writingMode.blockFlowDirection))
				return LineRelativeFourSides<T>(_over = from.top(), _under = from.bottom(), _lineLeft = from.left(), _lineRight = from.right());
			else if(resolveTextOrientation(writingMode) != SIDEWAYS_LEFT)
				return LineRelativeFourSides<T>(_over = from.right(), _under = from.left(), _lineLeft = from.top(), _lineRight = from.bottom());
			else
				return LineRelativeFourSides<T>(_over = from.left(), _under = from.right(), _lineLeft = from.bottom(), _lineRight = from.top());
		}

		/**
		 * 
		 * @tparam Rectangle1 The type for @a viewport
		 * @tparam Rectangle2 The type for @a from
		 * @tparam To The type for @a to
		 * @param writingMode The writing mode
		 * @param viewport The base physical rectangle
		 * @param from The physical rectangle to map
		 * @param[out] to The result flow-relative value
		 * @return @a to
		 */
		template<typename Rectangle1, typename Rectangle2, typename To>
		inline FlowRelativeFourSides<To>& mapPhysicalToFlowRelative(
				const WritingMode& writingMode, const Rectangle1& viewport,
				const Rectangle2& from, FlowRelativeFourSides<To>& to) {
			using namespace graphics;
			const TextOrientation textOrientation(resolveTextOrientation(writingMode));
			switch(writingMode.blockFlowDirection) {
				case HORIZONTAL_TB:
					to.before() = geometry::top(from) - geometry::top(viewport);
					to.after() = geometry::bottom(from) - geometry::top(viewport);
					to.start() = geometry::left(from) - geometry::left(viewport);
					to.end() = geometry::right(from) - geometry::left(viewport);
					break;
				case VERTICAL_RL:
				case VERTICAL_LR:
					to.before() = (writingMode.blockFlowDirection == VERTICAL_LR) ?
						(geometry::left(from) - geometry::left(viewport)) : (geometry::right(viewport) - geometry::right(from));
					to.after() = (writingMode.blockFlowDirection == VERTICAL_LR) ?
						(geometry::right(from) - geometry::left(viewport)) : (geometry::right(viewport) - geometry::left(from));
					{
						bool ttb = textOrientation == SIDEWAYS_LEFT;
						ttb = (writingMode.inlineFlowDirection == LEFT_TO_RIGHT) ? !ttb : ttb;
						to.start() = ttb ? (geometry::top(from) - geometry::top(viewport)) : (geometry::bottom(viewport) - geometry::bottom(from));
						to.end() = ttb ? (geometry::bottom(from) - geometry::top(viewport)) : (geometry::bottom(viewport) - geometry::top(from));
					}
					break;
				default:
					throw UnknownValueException("writingMode.blockFlowDirection");
			}
			return to;
		}
	}
} // namespace ascension.presentation

#endif // !ASCENSION_WRITING_MODE_MAPPINGS_HPP
