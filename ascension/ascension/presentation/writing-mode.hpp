/**
 * @file writing-mode.hpp
 * @author exeal
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2011 was presentation.hpp
 * @date 2011-05-04 text-style.hpp separated from presentation.hpp
 * @date 2011-07-24 separated from text-style.hpp
 * @date 2011-2012
 */

#ifndef ASCENSION_WRITING_MODE_HPP
#define ASCENSION_WRITING_MODE_HPP
#include <ascension/directions.hpp>
#include <ascension/corelib/basic-exceptions.hpp>	// UnknownValueException, std.logic_error
#include <ascension/graphics/geometry.hpp>			// PhysicalFourSides
#include <boost/operators.hpp>
#include <boost/optional.hpp>

namespace ascension {

	namespace graphics {
		namespace font {
			class TextRenderer;
		}
	}

	namespace presentation {

		/**
		 * Orientation of the text layout.
		 * @see TextLineStyle#readingDirection, defaultReadingDirection,
		 * @see "CSS Writing Modes Module Level 3, 2.1. Specifying Directionality: the ‘direction’
		 *      property" (http://www.w3.org/TR/css3-writing-modes/#direction)
		 * @see SVG 1.1 (Second Edition), 10.7.4 Relationship with bidirectionality
		 *      (http://www.w3.org/TR/SVG/text.html#DirectionProperty)
		 * @see XSL 1.1, 7.29.1 "direction" (http://www.w3.org/TR/xsl/#direction)
		 */
		enum ReadingDirection {
			LEFT_TO_RIGHT,	///< Left-to-right directionality.
			RIGHT_TO_LEFT	///< Right-to-left directionality.
		};

		/**
		 * Negation operation for @c ReadingDirection.
		 * @throw UnknownValueException @a value is invalid
		 */
		inline ReadingDirection operator!(ReadingDirection value) {
			switch(value) {
				case LEFT_TO_RIGHT:
					return RIGHT_TO_LEFT;
				case RIGHT_TO_LEFT:
					return LEFT_TO_RIGHT;
				default:
					throw UnknownValueException("value");
			}
		}

		class Presentation;
		ReadingDirection defaultReadingDirection(const Presentation& presentation);

		/**
		 * Defines block flow directions.
		 * @see "CSS Writing Modes Module Level 3, 3.1. Block Flow Direction: the ‘writing-mode’
		 *      property" (http://www.w3.org/TR/css3-writing-modes/#writing-mode)
		 * @see "SVG 1.1 (Second Edition), 10.7.2 Setting the inline-progression-direction"
		 *      (http://www.w3.org/TR/SVG/text.html#WritingModeProperty)
		 * @see "XSL 1.1, 7.29.7 "writing-mode"" (http://www.w3.org/TR/xsl/#writing-mode)
		 */
		enum BlockFlowDirection {
			HORIZONTAL_TB,	///< Top-to-bottom block flow. The writing mode is horizontal.
			VERTICAL_RL,	///< Right-to-left block flow. The writing mode is vertical.
			VERTICAL_LR		///< Left-to-right block flow. The writing mode is vertical.
		};

		/// Returns @c true if @a dir is horizontal direction.
		inline bool isHorizontal(BlockFlowDirection dir) {
			switch(dir) {
				case HORIZONTAL_TB:
					return true;
				case VERTICAL_RL:
				case VERTICAL_LR:
					return false;
				default:
					throw UnknownValueException("dir");
			}
		}

		/// Returns @c true if @a dir is vertical direction.
		inline bool isVertical(BlockFlowDirection dir) {return !isHorizontal(dir);}

		/**
		 * Defines the orientation of characters within a line.
		 * @see resolveTextOrientation
		 * @see "CSS Writing Modes Module Level 3, 5.1. Orienting Text: the ‘text-orientation’
		 *      property" (http://www.w3.org/TR/css3-writing-modes/#text-orientation)
		 * @see "SVG 1.1 (Second Edition), 10.7.3 Glyph orientation within a text run"
		 *      (http://www.w3.org/TR/SVG/text.html#GlyphOrientation)
		 */
		enum TextOrientation {
			MIXED_RIGHT, UPRIGHT, SIDEWAYS_RIGHT, SIDEWAYS_LEFT, SIDEWAYS, USE_GLYPH_ORIENTATION
		};

		/**
		 * @c WritingMode.
		 * @see "CSS Writing Modes Module Level 3" (http://www.w3.org/TR/css3-writing-modes/)
		 * @see "SVG 1.1 (Second Edition), 10.7 Text layout"
		 *      (http://www.w3.org/TR/SVG/text.html#TextLayout)
		 * @see "XSL 1.1, 7.29 Writing-mode-related Properties"
		 *      (http://www.w3.org/TR/xsl/#writing-mode-related)
		 */
		struct WritingMode : private boost::equality_comparable<WritingMode> {		
			ReadingDirection inlineFlowDirection;	///< The inline flow direction.
			BlockFlowDirection blockFlowDirection;	///< The block flow direction.
			TextOrientation textOrientation;		///< The text orientation.

			/**
			 * Constructor initializes the data members with the given values.
			 * @param inlineFlowDirection
			 * @param blockFlowDirection
			 * @param textOrientation
			 */
			explicit WritingMode(
				ReadingDirection inlineFlowDirection = LEFT_TO_RIGHT/*ASCENSION_DEFAULT_TEXT_READING_DIRECTION*/,
				BlockFlowDirection blockFlowDirection = HORIZONTAL_TB,
				TextOrientation textOrientation = MIXED_RIGHT) :
				inlineFlowDirection(inlineFlowDirection), blockFlowDirection(blockFlowDirection),
				textOrientation(textOrientation) /*noexcept*/ {}
			/// Equality operator.
			inline bool operator==(const WritingMode& other) const /*noexcept*/ {
				return inlineFlowDirection == other.inlineFlowDirection
					&& blockFlowDirection == other.blockFlowDirection
					&& textOrientation == other.textOrientation;
			}
		};

		/**
		 * Resolve ambiguous value of @c WritingMode#textOrientation.
		 * @param writingMode The writing mode
		 * @return The resolved @c TextOrientation value
		 */
		inline TextOrientation resolveTextOrientation(const WritingMode& writingMode) BOOST_NOEXCEPT {
			switch(writingMode.textOrientation) {
				case SIDEWAYS:
					if(writingMode.blockFlowDirection == VERTICAL_RL)
						return SIDEWAYS_RIGHT;
					else if(writingMode.blockFlowDirection == VERTICAL_LR)
						return SIDEWAYS_LEFT;
					else
						return SIDEWAYS;
				case USE_GLYPH_ORIENTATION:
					return MIXED_RIGHT;
				default:
					return writingMode.textOrientation;
			}
		}

		/// @name Free Functions Privide Mappings Between Relative-Flow vs. Physical Directions
		/// @{
		/**
		 * Maps flow-relative direction into corresponding physical direction.
		 * @param writingMode The writing mode
		 * @param direction The flow-relative direction to map
		 * @return Physical direction
		 * @throw UnknownValueException @a writingMode or @a direction is invalid
		 */
		inline graphics::PhysicalDirection mapFlowRelativeToPhysical(const WritingMode& writingMode, FlowRelativeDirection direction) {
			switch(writingMode.blockFlowDirection) {
				case HORIZONTAL_TB:
					switch(direction) {
						case BEFORE:
							return graphics::TOP;
						case AFTER:
							return graphics::BOTTOM;
						case START:
							return (writingMode.inlineFlowDirection == LEFT_TO_RIGHT) ? graphics::LEFT : graphics::RIGHT;
						case END:
							return (writingMode.inlineFlowDirection == LEFT_TO_RIGHT) ? graphics::RIGHT : graphics::LEFT;
					}
					break;
				case VERTICAL_RL:
				case VERTICAL_LR:
					switch(direction) {
						case BEFORE:
							return (writingMode.blockFlowDirection == VERTICAL_RL) ? graphics::RIGHT : graphics::LEFT;
						case AFTER:
							return (writingMode.blockFlowDirection == VERTICAL_RL) ? graphics::LEFT : graphics::RIGHT;
						case START:
						case END: {
							bool ttb = resolveTextOrientation(writingMode) == SIDEWAYS_LEFT;
							ttb = (writingMode.inlineFlowDirection == LEFT_TO_RIGHT) ? !ttb : ttb;
							return (direction == START) ? (ttb ? graphics::TOP : graphics::BOTTOM) : (ttb ? graphics::BOTTOM : graphics::TOP);
						}
					}
				default:
					throw UnknownValueException("writingMode.blockFlowDirection");
			}
			throw UnknownValueException("direction");
		}

		/**
		 * Maps physical direction into corresponding flow-relative direction.
		 * @param writingMode The writing mode
		 * @param direction The physical direction to map
		 * @return Flow-relative direction
		 * @throw UnknownValueException @a writingMode or @a direction is invalid
		 */
		inline FlowRelativeDirection mapPhysicalToFlowRelative(const WritingMode& writingMode, graphics::PhysicalDirection direction) {
			switch(writingMode.blockFlowDirection) {
				case HORIZONTAL_TB:
					switch(direction) {
						case graphics::TOP:
							return BEFORE;
						case graphics::RIGHT:
							return (writingMode.inlineFlowDirection == LEFT_TO_RIGHT) ? START : END;
						case graphics::BOTTOM:
							return AFTER;
						case graphics::LEFT:
							return (writingMode.inlineFlowDirection == LEFT_TO_RIGHT) ? END : START;
					}
					break;
				case VERTICAL_RL:
				case VERTICAL_LR:
					switch(direction) {
						case graphics::TOP:
						case graphics::BOTTOM: {
							bool ttb = resolveTextOrientation(writingMode) == SIDEWAYS_LEFT;
							ttb = (writingMode.inlineFlowDirection == LEFT_TO_RIGHT) ? !ttb : ttb;
							return (direction == graphics::TOP) ? (ttb ? START : END) : (ttb ? END : START);
						}
						case graphics::RIGHT:
							return (writingMode.blockFlowDirection == VERTICAL_RL) ? BEFORE : AFTER;
						case graphics::LEFT:
							return (writingMode.blockFlowDirection == VERTICAL_RL) ? AFTER : BEFORE;
					}
				default:
					throw UnknownValueException("writingMode.blockFlowDirection");
			}
			throw UnknownValueException("direction");
		}
		/// @}

		/// @name Free Functions Privide Mappings Between Relative-Flow vs. Physical Points
		/// @{
		/**
		 * Converts an abstract point into a physical point.
		 * @tparam To Type of coordinates of return value
		 * @tparam From Type of coordinates of @a from
		 * @param writingMode The writing mode
		 * @param from The abstract point to convert
		 * @param origin The origin of the physical space
		 * @return A converted physical point
		 */
		template<typename To, typename From>
		inline graphics::PhysicalTwoAxes<To> mapAbstractToPhysical(
				const WritingMode& writingMode, const AbstractTwoAxes<From>& from,
				const graphics::PhysicalTwoAxes<To>& origin = graphics::PhysicalTwoAxes<To>(graphics::_x = 0, graphics::_y = 0)) {
			switch(writingMode.blockFlowDirection) {
				case HORIZONTAL_TB:
					return graphics::PhysicalTwoAxes<To>((
						graphics::_x = origin.x() + (writingMode.inlineFlowDirection == LEFT_TO_RIGHT) ? +from.ipd() : -from.ipd(),
						graphics::_y = origin.y() + from.bpd()
					));
				case VERTICAL_RL:
				case VERTICAL_LR: {
					bool ttb = writingMode.inlineFlowDirection == LEFT_TO_RIGHT;
					ttb = (resolveTextOrientation(writingMode) != SIDEWAYS_LEFT) ? ttb : !ttb;
					return graphics::PhysicalTwoAxes<To>((
						graphics::_x = origin.x() + (writingMode.blockFlowDirection == VERTICAL_RL) ? -from.bpd() : +from.bpd(),
						graphics::_y = origin.y() + ttb ? +from.ipd() : -from.ipd()
					));
				}
				default:
					throw UnknownValueException("writingMode.blockFlowDirection");
			}
		}

		/**
		 * Converts a physical point into an abstract point.
		 * @tparam To Type of coordinates of return value
		 * @tparam From Type of coordinates of @a from
		 * @param writingMode The writing mode
		 * @param from The physical point to convert
		 * @param origin The origin of the abstract space
		 * @return A converted abstract point
		 */
		template<typename To, typename From>
		inline AbstractTwoAxes<To> mapPhysicalToAbstract(
				const WritingMode& writingMode, const graphics::PhysicalTwoAxes<From>& from,
				const AbstractTwoAxes<To>& origin = AbstractTwoAxes<To>(_ipd = 0, _bpd = 0)) {
			switch(writingMode.blockFlowDirection) {
				case HORIZONTAL_TB:
					return AbstractTwoAxes<To>((
						_ipd = origin.ipd() + (writingMode.inlineFlowDirection == LEFT_TO_RIGHT) ? +from.x() : -from.x(),
						_bpd = origin.bpd() + from.y(),
					));
				case VERTICAL_RL:
				case VERTICAL_LR: {
					bool ttb = writingMode.inlineFlowDirection == LEFT_TO_RIGHT;
					ttb = (resolveTextOrientation(writingMode) != SIDEWAYS_LEFT) ? ttb : !ttb;
					return AbstractTwoAxes<To>((
						_ipd = origin.ipd() + ttb ? +from.y() : -from.y()
						_bpd = origin.bpd() + (writingMode.blockFlowDirection == VERTICAL_RL) ? -from.x() : +from.x()
					));
				}
				default:
					throw UnknownValueException("writingMode.blockFlowDirection");
			}
		}
		/// @}

		/// @name Free Functions Privide Mappings Between Relative-Flow vs. Physical Rectangles
		/// @{
		/**
		 * Converts a flow-relative four sides into a physical four sides.
		 * @tparam To Type of coordinates of return value
		 * @tparam From Type of coordinates of @a from
		 * @param writingMode The writing mode
		 * @param from The flow-relative four sides to convert
		 * @param origin The origin of the physical space
		 * @return A converted physical four sides
		 */
		template<typename To, typename From>
		inline graphics::PhysicalFourSides<To> mapFlowRelativeToPhysical(
				const WritingMode& writingMode, const FlowRelativeFourSides<From>& from,
				const graphics::PhysicalTwoAxes<To>& origin = graphics::PhysicalTwoAxes<To>(graphics::_x = 0, graphics::_y = 0)) {
			AbstractTwoAxes<From> sources[2] = {
				AbstractTwoAxes<From>((_ipd = from.start(), _bpd = from.before())),
				AbstractTwoAxes<From>((_ipd = from.end(), _bpd = from.after()))
			};
			graphics::PhysicalTwoAxes<To> destinations[2] = {
				mapPhysicalToAbstract(writingMode, sources[0], origin),
				mapPhysicalToAbstract(writingMode, sources[1], origin)
			};
			return graphics::PhysicalFourSides<To>((
				graphics::_top = std::min(destinations[0].y(), destinations[1].y()),
				graphics::_right = std::max(destinations[0].x(), destinations[1].x()),
				graphics::_bottom = std::max(destinations[0].y(), destinations[1].y()),
				graphics::_left = std::min(destinations[0].x(), destinations[1].x())
			));
		}

		/**
		 * Converts a physical four sides into a flow-relative four sides.
		 * @tparam To Type of coordinates of return value
		 * @tparam From Type of coordinates of @a from
		 * @param writingMode The writing mode
		 * @param from The physical four sides to convert
		 * @param origin The origin of the flow-relative space
		 * @return A converted flow-relative four sides
		 */
		template<typename To, typename From>
		inline FlowRelativeFourSides<To> mapPhysicalToFlowRelative(
				const WritingMode& writingMode, const graphics::PhysicalFourSides<From>& from,
				const AbstractTwoAxes<To>& origin = AbstractTwoAxes<To>(_ipd = 0, _bpd = 0)) {
			PhysicalTwoAxes<From> sources[2] = {
				PhysicalTwoAxes<From>((graphics::_x = from.left(), graphics::_y = from.top())),
				PhysicalTwoAxes<From>((graphics::_x = from.right(), graphics::_y = from.bottom()))
			};
			AbstractTwoAxes<To> destinations[2] = {
				mapAbstractToPhysical(writingMode, sources[0], origin),
				mapAbstractToPhysical(writingMode, sources[1], origin)
			};
			return FlowRelativeFourSides<To>((
				_before = std::min(destinations[0].y(), destinations[1].y()),
				_after = std::max(destinations[0].y(), destinations[1].y()),
				_start = std::min(destinations[0].x(), destinations[1].x()),
				_end = std::max(destinations[0].x(), destinations[1].x())
			));
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
		/// @}

	}
} // namespace ascension.presentation

#endif // !ASCENSION_WRITING_MODE_HPP
