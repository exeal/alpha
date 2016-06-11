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
#include <boost/parameter/name.hpp>
#include <boost/parameter/preprocessor.hpp>
#include <type_traits>

namespace ascension {
	namespace presentation {
#ifndef ASCENSION_DETAIL_DOXYGEN_IS_PREPROCESSING
		BOOST_PARAMETER_NAME(writingMode)
		BOOST_PARAMETER_NAME(from)
		BOOST_PARAMETER_NAME(to)
#endif	// !ASCENSION_DETAIL_DOXYGEN_IS_PREPROCESSING

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
		inline graphics::PhysicalDirection mapDirection(const WritingMode& writingMode, FlowRelativeDirection from) {
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
		inline graphics::PhysicalDirection mapDirection(const WritingMode& writingMode, graphics::font::LineRelativeDirection from) {
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

		namespace detail {
			namespace dispatch {
				inline FlowRelativeDirection mapDirection(const WritingMode& writingMode, graphics::PhysicalDirection from, const FlowRelativeDirection*) {
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

				inline graphics::font::LineRelativeDirection mapDirection(const WritingMode& writingMode, graphics::PhysicalDirection from, const graphics::font::LineRelativeDirection*) {
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
			}
		}

		/**
		 * Maps physical direction into corresponding abstract one.
		 * @tparam To The return type
		 * @param writingMode The writing mode
		 * @param from The physical direction to map
		 * @return The mapped abstract direction
		 * @throw UnknownValueException @a writingMode or @a from is invalid
		 * @see mapDimensions
		 */
		template<typename To>
		inline To mapDirection(const WritingMode& writingMode, graphics::PhysicalDirection from) {
			return detail::dispatch::mapDirection(writingMode, from, static_cast<To*>(nullptr));
		}
		/// @}


		namespace detail {
			namespace dispatch {
				template<typename T>
				inline void mapDimensions(const WritingMode& writingMode, const FlowRelativeTwoAxes<T>& from, graphics::PhysicalTwoAxes<T>& to, typename std::enable_if<std::is_arithmetic<T>::value>::type* = nullptr) {
					switch(writingMode.blockFlowDirection) {
						case HORIZONTAL_TB:
							to.x() = (writingMode.inlineFlowDirection == LEFT_TO_RIGHT) ? +from.ipd() : -from.ipd();
							to.y() = from.bpd();
							break;
						case VERTICAL_RL:
						case VERTICAL_LR: {
							bool ttb = writingMode.inlineFlowDirection == LEFT_TO_RIGHT;
							ttb = (resolveTextOrientation(writingMode) != SIDEWAYS_LEFT) ? ttb : !ttb;
							to.x() = (writingMode.blockFlowDirection == VERTICAL_RL) ? -from.bpd() : +from.bpd();
							to.y() = ttb ? +from.ipd() : -from.ipd();
							break;
						}
						default:
							throw UnknownValueException("writingMode.blockFlowDirection");
					}
				}

				template<typename T>
				inline void mapDimensions(const WritingMode& writingMode, const graphics::font::LineRelativePoint<T>& from, graphics::PhysicalTwoAxes<T>& to, typename std::enable_if<std::is_arithmetic<T>::value>::type* = nullptr) {
					switch(writingMode.blockFlowDirection) {
						case HORIZONTAL_TB:
							to.x() = from.u();
							to.y() = from.v();
							break;
						case VERTICAL_RL:
						case VERTICAL_LR: {
							const bool sidewaysLr = resolveTextOrientation(writingMode) == SIDEWAYS_LEFT;
							to.x() = !sidewaysLr ? -from.v() : +from.v();
							to.y() = !sidewaysLr ? +from.u() : -from.v();
							break;
						}
						default:
							throw UnknownValueException("writingMode.blockFlowDirection");
					}
				}

				template<typename T>
				inline void mapDimensions(const WritingMode& writingMode, const graphics::PhysicalTwoAxes<T>& from, FlowRelativeTwoAxes<T>& to, typename std::enable_if<std::is_arithmetic<T>::value>::type* = nullptr) {
					switch(writingMode.blockFlowDirection) {
						case HORIZONTAL_TB:
							to.ipd() = (writingMode.inlineFlowDirection == LEFT_TO_RIGHT) ? +from.x() : -from.x();
							to.bpd() = from.y();
							break;
						case VERTICAL_RL:
						case VERTICAL_LR: {
							bool ttb = writingMode.inlineFlowDirection == LEFT_TO_RIGHT;
							ttb = (resolveTextOrientation(writingMode) != SIDEWAYS_LEFT) ? ttb : !ttb;
							to.ipd() = ttb ? +from.y() : -from.y();
							to.bpd() = (writingMode.blockFlowDirection == VERTICAL_RL) ? -from.x() : +from.x();
							break;
						}
						default:
							throw UnknownValueException("writingMode.blockFlowDirection");
					}
				}

				template<typename T>
				inline void mapDimensions(const WritingMode& writingMode, const graphics::PhysicalTwoAxes<T>& from, graphics::font::LineRelativePoint<T>& to, typename std::enable_if<std::is_arithmetic<T>::value>::type* = nullptr) {
					switch(writingMode.blockFlowDirection) {
						case HORIZONTAL_TB:
							to.u() = from.x();
							to.v() = from.y();
							break;
						case VERTICAL_RL:
						case VERTICAL_LR: {
							const bool sidewaysLr = resolveTextOrientation(writingMode) == SIDEWAYS_LEFT;
							to.u() = !sidewaysLr ? +from.y() : -from.y();
							to.v() = !sidewaysLr ? -from.x() : +from.x();
							break;
						}
						default:
							throw UnknownValueException("writingMode.blockFlowDirection");
					}
				}

				template<typename T>
				inline void mapDimensions(const WritingMode& writingMode, const FlowRelativeFourSides<T>& from, graphics::PhysicalFourSides<T>& to) {
					const bool ltr = writingMode.blockFlowDirection == LEFT_TO_RIGHT;
					if(isHorizontal(writingMode.blockFlowDirection))
						to = graphics::PhysicalFourSides<T>(
							graphics::_top = from.blockStart(), graphics::_right = ltr ? from.inlineEnd() : from.inlineStart(),
							graphics::_bottom = from.blockEnd(), graphics::_left = ltr ? from.inlineStart() : from.inlineEnd());
					else {
						const bool verticalRl = writingMode.blockFlowDirection == VERTICAL_RL;
						bool ttb = ltr;
						if(resolveTextOrientation(writingMode) == SIDEWAYS_LEFT)
							ttb = !ttb;
						to = graphics::PhysicalFourSides<T>(
							graphics::_top = ttb ? from.inlineStart() : from.inlineEnd(), graphics::_right = verticalRl ? from.blockStart() : from.blockEnd(),
							graphics::_bottom = ttb ? from.inlineEnd() : from.inlineStart(), graphics::_left = verticalRl ? from.blockEnd() : from.blockStart());
					}
				}

				template<typename T>
				inline void mapDimensions(const WritingMode& writingMode, const graphics::font::LineRelativeFourSides<T>& from, graphics::PhysicalFourSides<T>& to) {
					using namespace graphics;
					if(isHorizontal(writingMode.blockFlowDirection))
						to = PhysicalFourSides<T>(_top = from.over(), _right = from.lineRight(), _bottom = from.under(), _left = from.lineLeft());
					else if(resolveTextOrientation(writingMode) != SIDEWAYS_LEFT)
						to = PhysicalFourSides<T>(_top = from.lineLeft(), _right = from.over(), _bottom = from.lineRight(), _left = from.under());
					else
						to = PhysicalFourSides<T>(_top = from.lineRight(), _right = from.under(), _bottom = from.lineLeft(), _left = from.over());
				}

				template<typename T>
				inline void mapDimensions(const WritingMode& writingMode, const graphics::PhysicalFourSides<T>& from, FlowRelativeFourSides<T>& to) {
					const bool ltr = writingMode.blockFlowDirection == LEFT_TO_RIGHT;
					if(isHorizontal(writingMode.blockFlowDirection))
						to = FlowRelativeFourSides<T>(
							_blockStart = from.top(), _blockEnd = from.bottom(),
							_inlineStart = ltr ? from.left() : from.right(), _inlineEnd = ltr ? from.right() : from.left());
					else {
						const bool verticalRl = writingMode.blockFlowDirection == VERTICAL_RL;
						bool ttb = ltr;
						if(resolveTextOrientation(writingMode) == SIDEWAYS_LEFT)
							ttb = !ttb;
						to = FlowRelativeFourSides<T>(
							_blockStart = verticalRl ? from.right() : from.left(), _blockEnd = verticalRl ? from.left() : from.right(),
							_inlineStart = ttb ? from.top() : from.bottom(), _inlineEnd = ttb ? from.bottom() : from.top());
					}
				}

				template<typename T>
				inline void mapDimensions(const WritingMode& writingMode, const graphics::PhysicalFourSides<T>& from, graphics::font::LineRelativeFourSides<T>& to) {
					using namespace graphics::font;
					if(isHorizontal(writingMode.blockFlowDirection))
						to = LineRelativeFourSides<T>(_over = from.top(), _under = from.bottom(), _lineLeft = from.left(), _lineRight = from.right());
					else if(resolveTextOrientation(writingMode) != SIDEWAYS_LEFT)
						to = LineRelativeFourSides<T>(_over = from.right(), _under = from.left(), _lineLeft = from.top(), _lineRight = from.bottom());
					else
						to = LineRelativeFourSides<T>(_over = from.left(), _under = from.right(), _lineLeft = from.bottom(), _lineRight = from.top());
				}
			}
		}

#ifndef ASCENSION_DETAIL_DOXYGEN_IS_PREPROCESSING
		BOOST_PARAMETER_FUNCTION(
			(void), mapDimensions, tag,
			(required
				(writingMode, *)
				(from, *)
				(out(to), *))) {
			typedef std::decay<decltype(from)>::type From;
			typedef std::decay<decltype(to)>::type To;
			detail::dispatch::mapDimensions(writingMode, from, to);
		}
#else
		/**
		 * Maps between abstract and physical dimensions.
		 * @param writingMode The writing mode
		 * @param from The source dimensions
		 * @param[out] to The destination dimensions
		 * @throw UnknownValueException @a writingMode or @a from is invalid
		 * @note Mappings about twoaxes are not described in "W3C CSS Writing Modes Level 3". Any mappings are
		 *       performed based on the neutral origin (0, 0) which is a both abstract and physical point.
		 */
		template<typename From, typename To>
		void mapDimensions(const WritingMode& writingMode, const From& from, To& to);
#endif	// !ASCENSION_DETAIL_DOXYGEN_IS_PREPROCESSING

		/// @defgroup abstract_physical_bounds Abstract and Physical Bounds Mappings
		/// @brief Free functions map between abstract and physical bounds.
		/// @{

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
