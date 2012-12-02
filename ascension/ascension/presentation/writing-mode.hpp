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

		/// @name Free Functions Privide Mappings Relative-Flow vs. Physical Direction/Dimension/Axis
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

		/**
		 * Performs abstract-to-physical mappings according to the given writing mode.
		 * @tparam From The type for @a from
		 * @tparam To The type for @a to
		 * @param writingMode The writing mode
		 * @param from The flow-relative value to map
		 * @param[out] to The result physical value
		 * @see mapPhysicalToFlowRelative
		 */
		template<typename From, typename To>
		inline graphics::PhysicalFourSides<To>& mapFlowRelativeToPhysical(
				const WritingMode& writingMode,
				const FlowRelativeFourSides<From>& from, graphics::PhysicalFourSides<To>& to) {
			for(std::size_t i = 0; i != from.size(); ++i) {
				const FlowRelativeDirection direction = static_cast<FlowRelativeDirection>(i);
				to[mapFlowRelativeToPhysical(writingMode, direction)] = from[direction];
			}
			return to;
		}

		template<typename Rectangle1, typename From, typename Rectangle2>
		inline Rectangle2& mapFlowRelativeToPhysical(
				const WritingMode& writingMode, const Rectangle1& viewport,
				const FlowRelativeFourSides<From>& from, Rectangle2& to);

		/**
		 * Performs abstract-to-physical mappings according to the given writing mode.
		 * @tparam From The type for @a from
		 * @tparam To The type for @a to
		 * @param writingMode The writing mode
		 * @param from The physical value to map
		 * @param[out] to The result flow-relative value
		 * @return @a to
		 * @see #mapFlowRelativeToPhysical
		 */
		template<typename From, typename To>
		inline FlowRelativeFourSides<To>& mapPhysicalToFlowRelative(const WritingMode& writingMode,
				const graphics::PhysicalFourSides<From>& from, FlowRelativeFourSides<To>& to) {
			for(std::size_t i = 0; i != from.size(); ++i) {
				const graphics::PhysicalDirection direction = static_cast<graphics::PhysicalDirection>(i);
				to[mapPhysicalToFlowRelative(writingMode, direction)] = from[direction];
			}
			return to;
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
