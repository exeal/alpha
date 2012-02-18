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
#include <ascension/corelib/basic-exceptions.hpp>	// UnknownValueException, std.logic_error
#include <ascension/corelib/type-traits.hpp>		// detail.Select
#include <ascension/graphics/geometry.hpp>			// PhysicalFourSides
#include <ascension/presentation/inheritable.hpp>	// Inheritable
#include <array>
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
		 * @see "CSS Writing Modes Module Level 3, 2.1. Specifying Directionality: the ÅedirectionÅf
		 *      property" (http://www.w3.org/TR/css3-writing-modes/#direction)
		 * @see SVG 1.1 (Second Edition), 10.7.4 Relationship with bidirectionality
		 *      (http://www.w3.org/TR/SVG/text.html#DirectionProperty)
		 * @see XSL 1.1, 7.29.1 "direction" (http://www.w3.org/TR/xsl/#direction)
		 */
		enum ReadingDirection {
			LEFT_TO_RIGHT,	///< Left-to-right directionality.
			RIGHT_TO_LEFT	///< Right-to-left directionality.
		};

		class Presentation;
		ReadingDirection defaultReadingDirection(const Presentation& presentation);

		/**
		 * @see graphics#PhysicalFourSides
		 */
		template<typename T>
		class AbstractFourSides : public std::array<T, 4> {
		public:
			reference before() {return (*this)[0];}
			const_reference before() const {return (*this)[0];}
			reference after() {return (*this)[1];}
			const_reference after() const {return (*this)[1];}
			reference start() {return (*this)[2];}
			const_reference start() const {return (*this)[2];}
			reference end() {return (*this)[3];}
			const_reference end() const {return (*this)[3];}
		};

		/**
		 * Defines block flow directions.
		 * @see "CSS Writing Modes Module Level 3, 3.1. Block Flow Direction: the Åewriting-modeÅf
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
		 * @see "CSS Writing Modes Module Level 3, 5.1. Orienting Text: the Åetext-orientationÅf
		 *      property" (http://www.w3.org/TR/css3-writing-modes/#text-orientation)
		 * @see "SVG 1.1 (Second Edition), 10.7.3 Glyph orientation within a text run"
		 *      (http://www.w3.org/TR/SVG/text.html#GlyphOrientation)
		 */
		enum TextOrientation {
			VERTICAL_RIGHT, UPRIGHT, SIDEWAYS_RIGHT, SIDEWAYS_LEFT, SIDEWAYS, USE_GLYPH_ORIENTATION
		};

		/**
		 * Base type of @c WritingMode.
		 * @tparam inheritable All data members are inheritable when set to @c true
		 * @see "CSS Writing Modes Module Level 3" (http://www.w3.org/TR/css3-writing-modes/)
		 * @see "SVG 1.1 (Second Edition), 10.7 Text layout"
		 *      (http://www.w3.org/TR/SVG/text.html#TextLayout)
		 * @see "XSL 1.1, 7.29 Writing-mode-related Properties"
		 *      (http://www.w3.org/TR/xsl/#writing-mode-related)
		 */
		template<bool inheritable>
		struct WritingModeBase : private boost::equality_comparable<WritingMode<inheritable>> {
			/// The inline flow direction.
			typename InheritableIf<inheritable, ReadingDirection>::Type inlineFlowDirection;
			/// The block flow direction.
			typename InheritableIf<inheritable, BlockFlowDirection>::Type blockFlowDirection;
			/// The text orientation.
			typename InheritableIf<inheritable, TextOrientation>::Type textOrientation;

			/**
			 * Constructor initializes the data members with the given values.
			 * @param inlineFlowDirection
			 * @param blockFlowDirection
			 * @param textOrientation
			 */
			explicit WritingModeBase(
				typename InheritableIf<inheritable, ReadingDirection>::Type
					inlineFlowDirection = LEFT_TO_RIGHT/*ASCENSION_DEFAULT_TEXT_READING_DIRECTION*/,
				typename InheritableIf<inheritable, BlockFlowDirection>::Type
					blockFlowDirection = HORIZONTAL_TB,
				typename InheritableIf<inheritable, TextOrientation>::Type
					textOrientation = VERTICAL_RIGHT) :
				inlineFlowDirection(inlineFlowDirection), blockFlowDirection(blockFlowDirection),
				textOrientation(textOrientation) /*throw()*/ {}
			/// Implicit conversion operator.
			template<bool otherInheritable>
			inline operator WritingModeBase<otherInheritable>() const {
				return WritingModeBase<otherInheritable>(inlineFlowDirection, blockFlowDirection, textOrientation);
			}
			/// Equality operator.
			template<bool otherInheritable>
			inline bool operator==(const WritingModeBase<otherInheritable>& other) const {
				return inlineFlowDirection == other.inlineFlowDirection
					&& blockFlowDirection == other.blockFlowDirection && textOrientation == other.textOrientation;
			}
		};

		struct WritingMode : public WritingModeBase<false> {
			explicit WritingMode(
				ReadingDirection inlineFlowDirection
					= LEFT_TO_RIGHT/*ASCENSION_DEFAULT_TEXT_READING_DIRECTION*/,
				BlockFlowDirection blockFlowDirection = HORIZONTAL_TB,
				TextOrientation textOrientation = VERTICAL_RIGHT) :
				WritingModeBase(inlineFlowDirection, blockFlowDirection, textOrientation) /*throw()*/ {}
		};

		template<> class Inheritable<WritingMode> : public WritingModeBase<true> {};

		/**
		 * Performs abstract-to-physical mappings according to the given writing mode.
		 * @tparam From The type for @a from
		 * @tparam To The type for @a to
		 * @param writingMode The writing mode
		 * @param from The abstract value to map
		 * @param[out] to The result physical value
		 * @see mapPhysicalToAbstract
		 */
		template<typename From, typename To>
		inline graphics::PhysicalFourSides<To>& mapAbstractToPhysical(
				const WritingMode& writingMode,
				const AbstractFourSides<From>& from, graphics::PhysicalFourSides<To>& to) {
			const TextOrientation textOrientation(resolveTextOrientation(writingMode));
			switch(writingMode.blockFlowDirection) {
				case HORIZONTAL_TB:
					to.top() = from.before();
					to.bottom() = from.after();
					to.left() = (writingMode.inlineFlowDirection == LEFT_TO_RIGHT) ? from.start() : from.end();
					to.right() = (writingMode.inlineFlowDirection == LEFT_TO_RIGHT) ? from.end() : from.start();
					break;
				case VERTICAL_RL:
				case VERTICAL_LR:
					to.left() = (writingMode.blockFlowDirection == VERTICAL_LR) ? from.before() : from.after();
					to.right() = (writingMode.blockFlowDirection == VERTICAL_RL) ? from.before() : from.after();
					{
						bool ttb = textOrientation == SIDEWAYS_LEFT;
						ttb = (writingMode.inlineFlowDirection == LEFT_TO_RIGHT) ? !ttb : ttb;
						to.top() = ttb ? from.start() : from.end();
						to.bottom() = ttb ? from.start() : from.end();
					}
		 			break;			
				default:
					throw UnknownValueException("writingMode.blockFlowDirection");
			}
			return to;
		}

		template<typename Rectangle1, typename From, typename Rectangle2>
		inline Rectangle2& mapAbstractToPhysical(
				const WritingMode& writingMode, const Rectangle1& viewport,
				const AbstractFourSides<From>& from, Rectangle2& to);

		/**
		 * Performs abstract-to-physical mappings according to the given writing mode.
		 * @tparam From The type for @a from
		 * @tparam To The type for @a to
		 * @param writingMode The writing mode
		 * @param from The physical value to map
		 * @param[out] to The result abstract value
		 * @return @a to
		 * @see #mapAbstractToPhysical
		 */
		template<typename From, typename To>
		inline AbstractFourSides<To>& mapPhysicalToAbstract(const WritingMode& writingMode,
				const graphics::PhysicalFourSides<From>& from, AbstractFourSides<To>& to) {
			const TextOrientation textOrientation(resolveTextOrientation(writingMode));
			switch(writingMode.blockFlowDirection) {
				case HORIZONTAL_TB:
					to.before() = from.top();
					to.after() = from.bottom();
					to.start() = (writingMode.inlineFlowDirection == LEFT_TO_RIGHT) ? from.left() : from.right();
					to.end() = (writingMode.inlineFlowDirection == RIGHT_TO_LEFT) ? from.left() : from.right();
					break;
				case VERTICAL_RL:
				case VERTICAL_LR:
					to.before() = (writingMode.blockFlowDirection == VERTICAL_LR) ? from.left() : from.right();
					to.after() = (writingMode.blockFlowDirection == VERTICAL_RL) ? from.left() : from.right();
					{
						bool ttb = textOrientation == SIDEWAYS_LEFT;
						ttb = (writingMode.inlineFlowDirection == LEFT_TO_RIGHT) ? !ttb : ttb;
						to.start() = ttb ? from.top() : from.bottom();
						to.end() = ttb ? from.top() : from.bottom();
					}
		 			break;			
				default:
					throw UnknownValueException("writingMode.blockFlowDirection");
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
		 * @param[out] to The result abstract value
		 * @return @a to
		 */
		template<typename Rectangle1, typename Rectangle2, typename To>
		inline AbstractFourSides<To>& mapPhysicalToAbstract(
				const WritingMode& writingMode, const Rectangle1& viewport,
				const Rectangle2& from, AbstractFourSides<To>& to) {
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

		/***/
		inline TextOrientation resolveTextOrientation(const WritingMode& writingMode) {
			switch(writingMode.textOrientation) {
				case SIDEWAYS:
					if(writingMode.blockFlowDirection == VERTICAL_RL)
						return SIDEWAYS_RIGHT;
					else if(writingMode.blockFlowDirection == VERTICAL_LR)
						return SIDEWAYS_LEFT;
					else
						return SIDEWAYS;
				case USE_GLYPH_ORIENTATION:
					return VERTICAL_RIGHT;
				default:
					return writingMode.textOrientation;
			}
		}

	}
} // namespace ascension.presentation

#endif // !ASCENSION_WRITING_MODE_HPP
