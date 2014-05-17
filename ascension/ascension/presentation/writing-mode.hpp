/**
 * @file writing-mode.hpp
 * @author exeal
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2011 was presentation.hpp
 * @date 2011-05-04 text-style.hpp separated from presentation.hpp
 * @date 2011-07-24 separated from text-style.hpp
 * @date 2011-2012
 * @see directions.hpp, writing-mode-mappings.hpp
 */

#ifndef ASCENSION_WRITING_MODE_HPP
#define ASCENSION_WRITING_MODE_HPP
#include <ascension/corelib/basic-exceptions.hpp>	// UnknownValueException
#include <boost/functional/hash.hpp>
#include <boost/operators.hpp>
#include <functional>	// std.hash

namespace ascension {
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
		 * @param value The reading direction
		 * @return The opposite value of @a value
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

		/**
		 * Returns @c true if @a dir is horizontal direction.
		 * @throw UnknownValueException @a direction is invalid
		 */
		inline bool isHorizontal(BlockFlowDirection direction) {
			switch(direction) {
				case HORIZONTAL_TB:
					return true;
				case VERTICAL_RL:
				case VERTICAL_LR:
					return false;
				default:
					throw UnknownValueException("direction");
			}
		}

		/**
		 * Returns @c true if @a direction is vertical direction.
		 * @throw UnknownValueException @a direction is invalid
		 */
		inline bool isVertical(BlockFlowDirection direction) {return !isHorizontal(direction);}

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
			 * @param inlineFlowDirection The initial value for 'direction'
			 * @param blockFlowDirection The initial value for 'writing-mode'
			 * @param textOrientation The initial value for 'text-orientation'
			 */
			explicit WritingMode(
				ReadingDirection inlineFlowDirection = LEFT_TO_RIGHT/*ASCENSION_DEFAULT_TEXT_READING_DIRECTION*/,
				BlockFlowDirection blockFlowDirection = HORIZONTAL_TB,
				TextOrientation textOrientation = MIXED_RIGHT) BOOST_NOEXCEPT :
				inlineFlowDirection(inlineFlowDirection), blockFlowDirection(blockFlowDirection),
				textOrientation(textOrientation) {}
			/// Equality operator.
			inline bool operator==(const WritingMode& other) const BOOST_NOEXCEPT {
				return inlineFlowDirection == other.inlineFlowDirection
					&& blockFlowDirection == other.blockFlowDirection
					&& textOrientation == other.textOrientation;
			}
		};

		/// Specialization of @c boost#hash_value function template for @c WritingMode.
		inline std::size_t hash_value(const WritingMode& object) BOOST_NOEXCEPT {
			std::size_t seed = 0;
			boost::hash_combine(seed, object.inlineFlowDirection);
			boost::hash_combine(seed, object.blockFlowDirection);
			boost::hash_combine(seed, object.textOrientation);
			return seed;
		}

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
	}
} // namespace ascension.presentation

namespace std {
	/// Specialization of @c std#hash class template for @c WritingMode.
	template<>
	class hash<ascension::presentation::WritingMode> : public std::function<std::hash<void*>::result_type(const ascension::presentation::WritingMode&)> {
	public:
		result_type operator()(const argument_type& key) const BOOST_NOEXCEPT {
			return boost::hash<argument_type>()(key);
		}
	};
}

#endif // !ASCENSION_WRITING_MODE_HPP
