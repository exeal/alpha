/**
 * @file text-style.hpp
 * @author exeal
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2011 was presentation.hpp
 * @date 2011-05-04 text-style.hpp separated from presentation.hpp
 * @date 2011-07-24 separated from text-style.hpp
 */

#ifndef ASCENSION_WRITING_MODE_HPP
#define ASCENSION_WRITING_MODE_HPP
#include <ascension/corelib/basic-exceptions.hpp>	// UnknownValueException, std.logic_error
#include <ascension/corelib/type-traits.hpp>		// detail.Select
#include <ascension/graphics/geometry.hpp>			// PhysicalFourSides

namespace ascension {

	namespace graphics {
		namespace font {
			class TextRenderer;
		}
	}

	namespace presentation {
		template<typename T>
		class Inheritable {
		public:
			typedef T value_type;
			typedef Inheritable<T> Type;
		public:
			Inheritable() /*throw()*/ : inherits_(true) {}
			Inheritable(value_type v) /*throw()*/ : value_(v), inherits_(false) {}
			operator value_type() const {return get();}
			value_type get() const {if(inherits()) throw std::logic_error(""); return value_;}
			value_type inherit() /*throw()*/ {inherits_ = true;}
			bool inherits() const /*throw()*/ {return inherits_;}
			Inheritable<value_type>& set(value_type v) /*throw()*/ {value_ = v; inherits_ = false; return *this;}
		private:
			value_type value_;
			bool inherits_;
		};

		template<bool condition, typename T>
		struct InheritableIf {
			typedef typename detail::Select<condition, Inheritable<T>, T>::Type Type;
		};

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

		template<typename T>
		struct AbstractFourSides {
			T before, after, start, end;
		};

		struct WritingModeBase {
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

			/// Returns @c true if @a dir is horizontal direction.
			static inline bool isHorizontal(BlockFlowDirection dir) {
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
			static inline bool isVertical(BlockFlowDirection dir) {return !isHorizontal(dir);}
		};

		/**
		 * @tparam inheritable All data members are inheritable when set to @c true
		 * @see WritingModeBase
		 * @see "CSS Writing Modes Module Level 3" (http://www.w3.org/TR/css3-writing-modes/)
		 * @see "SVG 1.1 (Second Edition), 10.7 Text layout"
		 *      (http://www.w3.org/TR/SVG/text.html#TextLayout)
		 * @see "XSL 1.1, 7.29 Writing-mode-related Properties"
		 *      (http://www.w3.org/TR/xsl/#writing-mode-related)
		 */
		template<bool inheritable>
		struct WritingMode : public WritingModeBase {
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
			explicit WritingMode(
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
			inline operator WritingMode<otherInheritable>() const {
				return WritingMode<otherInheritable>(inlineFlowDirection, blockFlowDirection, textOrientation);
			}
			/// Equality operator.
			template<bool otherInheritable>
			inline bool operator==(const WritingMode<otherInheritable>& other) const {
				return inlineFlowDirection == other.inlineFlowDirection
					&& blockFlowDirection == other.blockFlowDirection && textOrientation == other.textOrientation;
			}
			/// Inequality operator.
			template<bool otherInheritable>
			inline bool operator!=(const WritingMode<otherInheritable>& other) const {return !(*this == other);}
		};

		WritingMode<false> resolveWritingMode(
			const Presentation& presentation, const graphics::font::TextRenderer& textRenderer);

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
				const WritingMode<false>& writingMode,
				const AbstractFourSides<From>& from, graphics::PhysicalFourSides<To>& to) {
			const WritingModeBase::TextOrientation textOrientation(resolveTextOrientation(writingMode));
			switch(writingMode.blockFlowDirection) {
				case WritingModeBase::HORIZONTAL_TB:
					to.top = from.before;
					to.bottom = from.after;
					to.left = (writingMode.inlineFlowDirection == LEFT_TO_RIGHT) ? from.start : from.end;
					to.right = (writingMode.inlineFlowDirection == LEFT_TO_RIGHT) ? from.end : from.start;
					break;
				case WritingModeBase::VERTICAL_RL:
				case WritingModeBase::VERTICAL_LR:
					to.left = (writingMode.blockFlowDirection == WritingModeBase::VERTICAL_LR) ? from.before : from.after;
					to.right = (writingMode.blockFlowDirection == WritingModeBase::VERTICAL_RL) ? from.before : from.after;
					{
						bool ttb = textOrientation == WritingModeBase::SIDEWAYS_LEFT;
						ttb = (writingMode.inlineFlowDirection == LEFT_TO_RIGHT) ? !ttb : ttb;
						to.top = ttb ? from.start : from.end;
						to.bottom = ttb ? from.start : from.end;
					}
		 			break;			
				default:
					throw UnknownValueException("writingMode.blockFlowDirection");
			}
			return to;
		}

		template<typename Rectangle1, typename From, typename Rectangle2>
		inline Rectangle2& mapAbstractToPhysical(
				const WritingMode<false>& writingMode, const Rectangle1& viewport,
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
		inline AbstractFourSides<To>& mapPhysicalToAbstract(
				const WritingMode<false>& writingMode,
				const graphics::PhysicalFourSides<From>& from, AbstractFourSides<To>& to) {
			const WritingModeBase::TextOrientation textOrientation(resolveTextOrientation(writingMode));
			switch(writingMode.blockFlowDirection) {
				case WritingModeBase::HORIZONTAL_TB:
					to.before = from.top;
					to.after = from.bottom;
					to.start = (writingMode.inlineFlowDirection == LEFT_TO_RIGHT) ? from.left : from.right;
					to.end = (writingMode.inlineFlowDirection == RIGHT_TO_LRFT) ? from.left : from.right;
					break;
				case WritingModeBase::VERTICAL_RL:
				case WritingModeBase::VERTICAL_LR:
					to.before = (writingMode.blockFlowDirection == WritingModeBase::VERTICAL_LR) ? from.left : from.right;
					to.after = (writingMode.blockFlowDirection == WritingModeBase::VERTICAL_RL) ? from.left : from.right;
					{
						bool ttb = textOrientation == WritingModeBase::SIDEWAYS_LEFT;
						ttb = (writingMode.inlineFlowDirection == LEFT_TO_RIGHT) ? !ttb : ttb;
						to.start = ttb ? from.top : from.bottom;
						to.end = ttb ? from.top : from.bottom;
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
				const WritingMode<false>& writingMode, const Rectangle1& viewport,
				const Rectangle2& from, AbstractFourSides<To>& to) {
			using namespace graphics;
			const WritingModeBase::TextOrientation textOrientation(resolveTextOrientation(writingMode));
			switch(writingMode.blockFlowDirection) {
				case WritingModeBase::HORIZONTAL_TB:
					to.before = geometry::top(from) - geometry::top(viewport);
					to.after = geometry::bottom(from) - geometry::top(viewport);
					to.start = geometry::left(from) - geometry::left(viewport);
					to.end = geometry::right(from) - geometry::left(viewport);
					break;
				case WritingModeBase::VERTICAL_RL:
				case WritingModeBase::VERTICAL_LR:
					to.before = (writingMode.blockFlowDirection == WritingModeBase::VERTICAL_LR) ?
						(geometry::left(from) - geometry::left(viewport)) : (geometry::right(viewport) - geometry::right(from));
					to.after = (writingMode.blockFlowDirection == WritingModeBase::VERTICAL_LR) ?
						(geometry::right(from) - geometry::left(viewport)) : (geometry::right(viewport) - geometry::left(from));
					{
						bool ttb = textOrientation == WritingModeBase::SIDEWAYS_LEFT;
						ttb = (writingMode.inlineFlowDirection == LEFT_TO_RIGHT) ? !ttb : ttb;
						to.start = ttb ? (geometry::top(from) - geometry::top(viewport)) : (geometry::bottom(viewport) - geometry::bottom(from));
						to.end = ttb ? (geometry::bottom(from) - geometry::top(viewport)) : (geometry::bottom(viewport) - geometry::top(from));
					}
					break;
				default:
					throw UnknownValueException("writingMode.blockFlowDirection");
			}
			return to;
		}

		/***/
		inline WritingModeBase::TextOrientation resolveTextOrientation(const WritingMode<false>& writingMode) {
			switch(writingMode.textOrientation) {
				case WritingModeBase::SIDEWAYS:
					if(writingMode.blockFlowDirection == WritingModeBase::VERTICAL_RL)
						return WritingModeBase::SIDEWAYS_RIGHT;
					else if(writingMode.blockFlowDirection == WritingModeBase::VERTICAL_LR)
						return WritingModeBase::SIDEWAYS_LEFT;
					else
						return WritingModeBase::SIDEWAYS;
				case WritingModeBase::USE_GLYPH_ORIENTATION:
					return WritingModeBase::VERTICAL_RIGHT;
				default:
					return writingMode.textOrientation;
			}
		}

	}
} // namespace ascension.presentation

#endif // !ASCENSION_WRITING_MODE_HPP
