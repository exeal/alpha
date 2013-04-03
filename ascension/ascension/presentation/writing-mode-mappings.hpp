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
#include <ascension/directions.hpp>
#include <ascension/graphics/geometry.hpp>			// graphics.PhysicalFourSides
#include <ascension/presentation/writing-mode.hpp>	// WritingMode, ...

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

		/// @name Free Functions Map Between Abstract and Physical Directions
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
			if(static_cast<int>(from) < 0 || static_cast<std::size_t>(from) >= std::extent<decltype(horizontalMappings)>::value)
				throw UnknownValueException("from");
			if(writingMode.inlineFlowDirection == RIGHT_TO_LEFT && (from == FlowRelativeDirection::START || from == FlowRelativeDirection::END))
				from = !from;
			if(isHorizontal(writingMode.blockFlowDirection))	// this may throw UnknownValueException
				return horizontalMappings[from];
			else {
				if(from == FlowRelativeDirection::BEFORE || from == FlowRelativeDirection::AFTER) {
					if(writingMode.blockFlowDirection == VERTICAL_LR)
						from = !from;
				} else if(resolveTextOrientation(writingMode) == SIDEWAYS_LEFT)
					from = !from;
				return verticalMappings[from];
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
			if(static_cast<int>(from) < 0 || static_cast<std::size_t>(from) >= std::extent<decltype(horizontalMappings)>::value)
				throw UnknownValueException("from");
			if(isHorizontal(writingMode.blockFlowDirection))	// this may throw UnknownValueException
				return horizontalMappings[from];
			else
				return verticalMappings[(resolveTextOrientation(writingMode) != SIDEWAYS_LEFT) ? from : !from];
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
			if(static_cast<int>(from) < 0 || static_cast<std::size_t>(from) >= std::extent<decltype(horizontalMappings)>::value)
				throw UnknownValueException("from");
			if(isHorizontal(writingMode.blockFlowDirection)) {	// this may throw UnknownValueException
				if(writingMode.inlineFlowDirection == RIGHT_TO_LEFT && (from == graphics::PhysicalDirection::LEFT || from == graphics::PhysicalDirection::RIGHT))
					from = !from;
				return horizontalMappings[from];
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
				return verticalMappings[from];
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
			if(static_cast<int>(from) < 0 || static_cast<std::size_t>(from) >= std::extent<decltype(horizontalMappings)>::value)
				throw UnknownValueException("from");
			if(isHorizontal(writingMode.blockFlowDirection))	// this may throw UnknownValueException
				return horizontalMappings[from];
			else
				return verticalMappings[(resolveTextOrientation(writingMode) != SIDEWAYS_LEFT) ? from : !from];
		}
		/// @}

		/// @name Free Functions Map Between Abstract and Physical Axes
		/// @{
		/**
		 * Maps abstract axes into corresponding physical one.
		 * @param writingMode The writing mode
		 * @param from The abstract axes to map
		 * @return The mapped physical axes
		 * @throw UnknownValueException @a writingMode or @a from is invalid
		 * @see mapPhysicalToAbstract
		 */
		template<typename T>
		inline graphics::PhysicalTwoAxes<T> mapAbstractToPhysical(const WritingMode& writingMode, const AbstractTwoAxes<T>& from) {
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
		 * Maps physical axes into corresponding abstract one.
		 * @param writingMode The writing mode
		 * @param from The physical axes to map
		 * @return The mapped avstract axes
		 * @throw UnknownValueException @a writingMode or @a from is invalid
		 * @see mapAbstractToPhysical
		 */
		template<typename T>
		inline AbstractTwoAxes<T> mapPhysicalToAbstract(const WritingMode& writingMode, const graphics::PhysicalTwoAxes<T>& from) {
			switch(writingMode.blockFlowDirection) {
				case HORIZONTAL_TB:
					return AbstractTwoAxes<T>(
						_ipd = (writingMode.inlineFlowDirection == LEFT_TO_RIGHT) ? +from.x() : -from.x(),
						_bpd = from.y()
					);
				case VERTICAL_RL:
				case VERTICAL_LR: {
					bool ttb = writingMode.inlineFlowDirection == LEFT_TO_RIGHT;
					ttb = (resolveTextOrientation(writingMode) != SIDEWAYS_LEFT) ? ttb : !ttb;
					return AbstractTwoAxes<T>(
						_ipd = ttb ? +from.y() : -from.y(),
						_bpd = (writingMode.blockFlowDirection == VERTICAL_RL) ? -from.x() : +from.x()
					);
				}
				default:
					throw UnknownValueException("writingMode.blockFlowDirection");
			}
		}
		/// @}

		/// @name Free Functions Map Between Abstract and Physical Bounds
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
			const AbstractTwoAxes<T> sources[2] = {
				AbstractTwoAxes<T>(_ipd = from.start(), _bpd = from.before()),
				AbstractTwoAxes<T>(_ipd = from.end(), _bpd = from.after())
			};
			const graphics::PhysicalTwoAxes<T> destinations[2] = {
				mapAbstractToPhysical(writingMode, sources[0]), mapAbstractToPhysical(writingMode, sources[1])
			};
			return graphics::PhysicalFourSides<T>(
				graphics::_top = std::min(destinations[0].y(), destinations[1].y()),
				graphics::_right = std::max(destinations[0].x(), destinations[1].x()),
				graphics::_bottom = std::max(destinations[0].y(), destinations[1].y()),
				graphics::_left = std::min(destinations[0].x(), destinations[1].x())
			);
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
			const PhysicalTwoAxes<T> sources[2] = {
				PhysicalTwoAxes<T>(graphics::_x = from.left(), graphics::_y = from.top()),
				PhysicalTwoAxes<T>(graphics::_x = from.right(), graphics::_y = from.bottom())
			};
			const AbstractTwoAxes<T> destinations[2] = {
				mapPhysicalToAbstract(writingMode, sources[0]), mapPhysicalToAbstract(writingMode, sources[1])
			};
			return FlowRelativeFourSides<T>(
				_before = std::min(destinations[0].bpd(), destinations[1].bpd()),
				_after = std::max(destinations[0].bpd(), destinations[1].bpd()),
				_start = std::min(destinations[0].ipd(), destinations[1].ipd()),
				_end = std::max(destinations[0].ipd(), destinations[1].ipd())
			);
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
