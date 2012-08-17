/**
 * @file text-style.hpp
 * @author exeal
 * @see text-line-style.hpp
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2011 was presentation.hpp
 * @date 2011-05-04 separated from presentation.hpp
 * @date 2012-07-16 reunioned with text-line-style.hpp
 */

#ifndef ASCENSION_TEXT_STYLE_HPP
#define ASCENSION_TEXT_STYLE_HPP

#include <ascension/corelib/future/scoped-enum-emulation.hpp>
#include <ascension/graphics/color.hpp>	// graphics.Color
#include <ascension/graphics/font.hpp>	// graphics.font.FontProperties, ...
#include <ascension/graphics/paint.hpp>	// graphics.Paint
#include <ascension/presentation/length.hpp>
#include <ascension/presentation/style-property.hpp>
#include <ascension/presentation/writing-mode.hpp>
#include <map>
#include <memory>
#include <tuple>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/optional.hpp>
#include <boost/variant.hpp>

namespace ascension {
	namespace presentation {

		// from CSS Color Module Level 3 //////////////////////////////////////////////////////////

		/**
		 * Describes the foreground color of the text content. @c boost#none means 'currentColor'
		 * CSS 3.
		 * @tparam InheritedOrNot See @c StyleProperty class template
		 * @see CSS Color Module Level 3, 3.1. Foreground color: the ‘color’ property
		 *      (http://www.w3.org/TR/css3-color/#foreground)
		 * @see SVG 1.1 (Second Edition), 12.2 The ‘color’ property
		 *      (http://www.w3.org/TR/SVG11/color.html#ColorProperty)
		 * @see XSL 1.1, 7.18.1 "color" (http://www.w3.org/TR/xsl/#color)
		 */
		template<typename InheritedOrNot>
		class ColorProperty : public StyleProperty<
			sp::Complex<
				boost::optional<graphics::Color>
			>, InheritedOrNot
		> {};

		template<typename InheritedOrNot>
		inline graphics::Color computeColor(
				const ColorProperty<InheritedOrNot>* current,
				const ColorProperty<InheritedOrNot>* parent,
				const ColorProperty<InheritedOrNot>& ancestor) {
			if(current != nullptr && !current->inherits() && current->get() != boost::none)
				return *current->get();
			else if(parent != nullptr && !parent->inherits() && parent->get() != boost::none)
				return *parent->get();
			else if(!ancestor.inherits() && ancestor.get() != boost::none)
				return *ancestor.get();
			else
				return graphics::SystemColors::get(graphics::SystemColors::WINDOW_TEXT);
		}

		// from CSS Backgrounds and Borders Module Level 3 ////////////////////////////////////////

		/**
		 * @c null also means 'transparent'.
		 * @see CSS Backgrounds and Borders Module Level 3, 3.10. Backgrounds Shorthand: the
		 *      ‘background’ property (http://www.w3.org/TR/css3-background/#the-background)
		 * @see SVG 1.1 (Second Edition), 11.3 Fill Properties
		 *      (http://www.w3.org/TR/SVG11/painting.html#FillProperties)
		 * @see XSL 1.1, 7.31.1 "background" (http://www.w3.org/TR/xsl/#background)
		 */
		struct Background {
			/**
			 * [Copied from CSS3] This property sets the background color of an element. The color
			 * is drawn behind any background images.
			 * @see CSS Backgrounds and Borders Module Level 3, 3.2. Base Color: the
			 *      ‘background-color’ property
			 *      (http://www.w3.org/TR/css3-background/#the-background-color)
			 * @see XSL 1.1, 7.8.2 "background-color" (http://www.w3.org/TR/xsl/#background-color)
			 */
			StyleProperty<
				sp::Complex<
					boost::optional<graphics::Color>
				>, sp::NotInherited
			> color;
			/**
			 * @see CSS Backgrounds and Borders Module Level 3, 3.1. Layering Multiple Background
			 *      Images (http://www.w3.org/TR/css3-background/#layering)
			 */
			struct Layer {
				struct Image {} image;
				struct RepeatStyle {} repeat;
				enum Attachment {} attachment;
				struct Position {} position;
				enum Clip {} clip;
				enum Origin {} origin;
				struct Size {} size;
			};

			Background() : color(boost::make_optional(graphics::Color::TRANSPARENT_BLACK)) {}
		};

		inline std::unique_ptr<graphics::Paint> computeBackground(
				const Background* current, const Background* parent, const Background& ancestor) {
			// TODO: This code is not complete.
			if(current != nullptr && !current->color.inherits()
					&& current->color.get() != boost::none && current->color.get()->isFullyTransparent())
				return std::unique_ptr<graphics::Paint>(new graphics::SolidColor(*current->color.get()));
			else if(parent != nullptr && !parent->color.inherits()
					&& parent->color.get() != boost::none && parent->color.get()->isFullyTransparent())
				return std::unique_ptr<graphics::Paint>(new graphics::SolidColor(*parent->color.get()));
			else if(!ancestor.color.inherits()
					&& ancestor.color.get() != boost::none && ancestor.color.get()->isFullyTransparent())
				return std::unique_ptr<graphics::Paint>(new graphics::SolidColor(*ancestor.color.get()));
			return std::unique_ptr<graphics::Paint>(
				new graphics::SolidColor(graphics::SystemColors::get(graphics::SystemColors::WINDOW)));
		}

		/**
		 * @see "CSS Backgrounds and Borders Module Level 3"
		 *      (http://www.w3.org/TR/2011/CR-css3-background-20110215/)
		 */
		struct Border {
			/**
			 * @see 
			 */
			enum Style {
				NONE, HIDDEN, DOTTED, DASHED, SOLID,
				DOT_DASH, DOT_DOT_DASH,
				DOUBLE, GROOVE, RIDGE, INSET, OUTSET
			};
			static const Length THIN, MEDIUM, THICK;
			struct Side {
				/**
				 * [Copied from CSS3] This property sets the foreground color of the border
				 * specified by the border-style properties. @c boost#none means 'currentColor'.
				 * @see CSS Backgrounds and Borders Module Level 3, 4.1. Line Colors: the
				 *      ‘border-color’ property
				 *      (http://www.w3.org/TR/css3-background/#the-border-color)
				 */
				boost::optional<graphics::Color> color;
				/**
				 * [Copied from CSS3] This property sets the style of the border, unless there is a
				 * border image.
				 * @see CSS Backgrounds and Borders Module Level 3, 4.2. Line Patterns: the
				 *      ‘border-style’ properties
				 *      (http://www.w3.org/TR/css3-background/#the-border-style)
				 */
				StyleProperty<
					sp::Enumerated<Style, NONE>,
					sp::NotInherited
				> style;

				struct WidthTypeSpec : public detail::Type2Type<Length> {
					static const Length& initialValue() {return MEDIUM;}
				};
				/**
				 * [Copied from CSS3] This property sets the thickness of the border.
				 * @see CSS Backgrounds and Borders Module Level 3, 4.3. Line Thickness: the
				 *      ‘border-width’ properties
				 *      (http://www.w3.org/TR/css3-background/#the-border-width)
				 */
				StyleProperty<
					WidthTypeSpec,
					sp::NotInherited
				> width;
			};
			FlowRelativeFourSides<Side> sides;
		};

		// from CSS Fonts Module Level 3 //////////////////////////////////////////////////////////

		/**
		 * [Copied from CSS3] An <absolute-size> keyword refers to an entry in a table of font
		 * sizes computed and kept by the user agent.
		 * @see http://www.w3.org/TR/css3-fonts/#ltabsolute-sizegt
		 */
		ASCENSION_BEGIN_SCOPED_ENUM(AbsoluteFontSize)
			XX_SMALL, X_SMALL, SMALL, MEDIUM, LARGE, X_LARGE, XX_LARGE
		ASCENSION_END_SCOPED_ENUM;

		/**
		 * [Copied from CSS3] A <relative-size> keyword is interpreted relative to the table of
		 * font sizes and the font size of the parent element.
		 * @see http://www.w3.org/TR/css3-fonts/#ltrelative-sizegt
		 */
		ASCENSION_BEGIN_SCOPED_ENUM(RelativeFontSize)
			LARGER, SMALLER
		ASCENSION_END_SCOPED_ENUM;

		// from CSS Line Layout Module Level 3 ////////////////////////////////////////////////////

		/// Enumerated values for @c TextRunStyle#textHeight.
		ASCENSION_BEGIN_SCOPED_ENUM(TextHeightEnums)
			AUTO, FONT_SIZE, TEXT_SIZE, MAX_SIZE
		ASCENSION_END_SCOPED_ENUM;

		/// Enumerated values for @c TextRunStyle#lineHeight.
		ASCENSION_BEGIN_SCOPED_ENUM(LineHeightEnums)
			NORMAL, NONE
		ASCENSION_END_SCOPED_ENUM;

		/**
		 * [Copied from CSS3] This property enumerates which aspects of the elements in a line box
		 * contribute to the height height of that line box.
		 * @see CSS Line Layout Module Level 3, 3.4.2 Line Stacking: the ‘line-box-contain’
		 *      property (http://dev.w3.org/csswg/css3-linebox/#LineStacking)
		 * @see XSL 1.1, 7.16.6 "line-stacking-strategy"
		 *      (http://www.w3.org/TR/xsl/#line-stacking-strategy)
		 */
		ASCENSION_BEGIN_SCOPED_ENUM(LineBoxContain)
			// TODO: 'NONE' should be 0.
			// TODO: Values other than 'NONE' can be combined by bitwise-OR.
			BLOCK, INLINE, FONT, GLYPHS, REPLACED, INLINE_BOX, NONE
		ASCENSION_END_SCOPED_ENUM;
		/**
		 * [Copied from CSS3] The ‘dominant-baseline’ property is used to determine or re-determine
		 * a scaled-baseline-table.
		 * @see CSS Line Layout Module Level 3, 4.4 Dominant baseline: the ‘dominant-baseline’
		 *      property (http://dev.w3.org/csswg/css3-linebox/#dominant-baseline-prop)
		 * @see CSS3 module: line, 4.4. Dominant baseline: the 'dominant-baseline' property
		 *      (http://www.w3.org/TR/css3-linebox/#dominant-baseline-prop)
		 * @see SVG 1.1 (Second Edition), 10.9.2 Baseline alignment properties
		 *      (http://www.w3.org/TR/SVG/text.html#DominantBaselineProperty)
		 * @see XSL 1.1, 7.14.5 "dominant-basline" (http://www.w3.org/TR/xsl/#dominant-baseline)
		 */
		ASCENSION_BEGIN_SCOPED_ENUM(DominantBaseline)
			AUTO,
			USE_SCRIPT,
			NO_CHANGE,
			RESET_SIZE,
			ALPHABETIC,
			HANGING,
			IDEOGRAPHIC,
			MATHEMATICAL,
			CENTRAL,
			MIDDLE,
			TEXT_AFTER_EDGE,
			TEXT_DEFORE_EDGE,
		ASCENSION_END_SCOPED_ENUM;

		/**
		 * [Copied from CSS3] This property specifies how an inline-level element is aligned with
		 * respect to its parent. That is, to which of the parent's baselines the alignment point
		 * of this element is aligned. Unlike the ‘dominant-baseline’ property the
		 * ‘alignment-baseline’ property has no effect on its children dominant-baselines.
		 * @see CSS Line Layout Module Level 3, 4.5 Aligning the alignment point of an element: the
		 *      ‘alignment-baseline’ property
		 *      (http://dev.w3.org/csswg/css3-linebox/#alignment-baseline-prop)
		 * @see CSS3 module: line, 4.5. Aligning the alignment point of an element: the
		 *      'alignment-baseline' property
		 *      (http://www.w3.org/TR/css3-linebox/#alignment-baseline-prop)
		 * @see SVG 1.1 (Second Edition), 10.9.2 Baseline alignment properties
		 *      (http://www.w3.org/TR/SVG/text.html#AlignmentBaselineProperty)
		 * @see XSL 1.1, 7.14.2 "alignment-baseline" (http://www.w3.org/TR/xsl/#alignment-baseline)
		 */
		ASCENSION_BEGIN_SCOPED_ENUM(AlignmentBaseline)
			BASELINE,
			USE_SCRIPT,
			BEFORE_EDGE,
			TEXT_BEFORE_EDGE,
			AFTER_EDGE,
			TEXT_AFTER_EDGE,
			CENTRAL,
			MIDDLE,
			IDEOGRAPHIC,
			ALPHABETIC,
			HANGING,
			MATHEMATICAL
		ASCENSION_END_SCOPED_ENUM;

		/// Enumerated values for @c TextRunStyle#alignmentAdjust.
		ASCENSION_BEGIN_SCOPED_ENUM(AlignmentAdjustEnums)
			AUTO,
			BASELINE,
			BEFORE_EDGE,
			TEXT_BEFORE_EDGE,
			MIDDLE,
			CENTRAL,
			AFTER_EDGE,
			TEXT_AFTER_EDGE,
			IDEOGRAPHIC,
			ALPHABETIC,
			HANGING,
			MATHEMATICAL
		ASCENSION_END_SCOPED_ENUM;

		/// Enumerated values for @c TextRunStyle#baselineShift.
		ASCENSION_BEGIN_SCOPED_ENUM(BaselineShiftEnums)
			BASELINE,
			SUB,
			SUPER
		ASCENSION_END_SCOPED_ENUM;

		/// Enumerated values for @c InlineBoxAlignment.
		ASCENSION_BEGIN_SCOPED_ENUM(InlineBoxAlignmentEnums)
			INITIAL, LAST
		ASCENSION_END_SCOPED_ENUM;

		/**
		 * [Copied from CSS3] The ‘inline-box-align’ property determines which line of a multi-line
		 * inline block aligns with the previous and next inline elements within a line.
		 * @see CSS Line Layout Module Level 3, 4.9 Inline box alignment: the
		 *      ‘inline-box-align’ property
		 *      (http://dev.w3.org/csswg/css3-linebox/#inline-box-align-prop)
		 */
		typedef boost::variant<InlineBoxAlignmentEnums, Index> InlineBoxAlignment;

		// from CSS Text Level 3 //////////////////////////////////////////////////////////////////

		/**
		 * [Copied from CSS3] This property transforms text for styling purposes.
		 * @see CSS Text Level 3, 2.1. Transforming Text: the ‘text-transform’ property
		 *      (http://www.w3.org/TR/css3-text/#text-transform)
		 * @see XSL 1.1, 7.17.6 "text-transform" (http://www.w3.org/TR/xsl/#text-transform)
		 */
		ASCENSION_BEGIN_SCOPED_ENUM(TextTransform)
			NONE, CAPITALIZE, UPPERCASE, LOWERCASE, FULL_WIDTH, FULL_SIZE_KANA
		ASCENSION_END_SCOPED_ENUM;

//		enum TextSpaceCollapse;

		/**
		 * [Copied from CSS3] This property specifies the strictness of line-breaking rules applied
		 * within an element: particularly how line-breaking interacts with punctuation.
		 * @see CSS Text Level 3, 4.1. Line Breaking Strictness: the ‘line-break’ property
		 *      (http://www.w3.org/TR/css3-text/#line-break)
		 */
		ASCENSION_BEGIN_SCOPED_ENUM(LineBreak)
			AUTO,
			LOOSE,
			NORMAL,
			STRICT
		ASCENSION_END_SCOPED_ENUM;

		/**
		 * [Copied from CSS3] This property specifies line break opportunities within words.
		 * @see CSS Text Level 3, 4.2. Word Breaking Rules: the 'word-break' property
		 *      (http://www.w3.org/TR/css3-text/#word-break)
		 */
		ASCENSION_BEGIN_SCOPED_ENUM(WordBreak)
			NORMAL,
			KEEP_ALL,
			BREAK_ALL
		ASCENSION_END_SCOPED_ENUM;

		/**
		 * [Copied from CSS3] This property controls whether hyphenation is allowed to create more
		 * soft wrap opportunities within a line of text.
		 * @see CSS Text Level 3, 6.1. Hyphenation Control: the ‘hyphens’ property
		 *      (http://www.w3.org/TR/css3-text/#hyphens)
		 * @see XSL 1.1, 7.10 Common Hyphenation Properties
		 *      (http://www.w3.org/TR/xsl/#common-hyphenation-properties)
		 */
		ASCENSION_BEGIN_SCOPED_ENUM(Hyphens)
			NONE,
			MANUAL,
			AUTO
		ASCENSION_END_SCOPED_ENUM;

		/**
		 * [Copied from CSS3] This property specifies the mode for text wrapping.
		 * @see CSS Text Level 3, 6.1. Text Wrap Settings: 'text-wrap' property
		 *      (http://www.w3.org/TR/css3-text/#text-wrap)
		 */
		ASCENSION_BEGIN_SCOPED_ENUM(TextWrap)
			NORMAL,
			NONE,
			AVOID
		ASCENSION_END_SCOPED_ENUM;

		/**
		 * [Copied from CSS3] This property specifies whether the UA may break within a word to
		 * prevent overflow when an otherwise-unbreakable string is too long to fit within the line
		 * box. It only has an effect when ‘text-wrap’ is either ‘normal’ or ‘avoid’
		 * @see CSS Text Level 3 - 6.2. Emergency Wrapping: the 'overflow-wrap' property
		 *      (http://www.w3.org/TR/css3-text/#overflow-wrap)
		 */
		ASCENSION_BEGIN_SCOPED_ENUM(OverflowWrap)
			NORMAL,
			BREAK_WORD/*,
			HYPHENATE*/
		ASCENSION_END_SCOPED_ENUM;

		template<typename Measure>
		struct TextWrapping {
			TextWrap textWrap;
			OverflowWrap overflowWrap;
			Measure measure;
			/// Default constructor.
			TextWrapping() : textWrap(TextWrap::NORMAL), overflowWrap(OverflowWrap::NORMAL), measure(0) {}
		};

		/**
		 * @c TextAnchor describes an alignment of text relative to the given point.
		 * @see resolveTextAlignment, TextLineStyle#alignment, TextLineStyle#lastSublineAlignment,
		 *      defaultTextAnchor
		 * @see XSL 1.1, 7.16.9 "text-align"
		 *      (http://www.w3.org/TR/2006/REC-xsl11-20061205/#text-align)
		 * @see CSS Text Level 3, 7.1. Text Alignment: the 'text-align' property
		 *      (http://www.w3.org/TR/2010/WD-css3-text-20101005/#text-align)
		 */
		ASCENSION_BEGIN_SCOPED_ENUM(TextAlignment)
			START,
			END,
			LEFT,
			RIGHT,
			CENTER,
			JUSTIFY,
			MATCH_PARENT,
			START_END
		ASCENSION_END_SCOPED_ENUM;

		/**
		 * [Copied from SVG11] The ‘text-anchor’ property is used to align (start-, middle- or
		 * end-alignment) a string of text relative to a given point.
		 * @see SVG 1.1, 10.9.1 Text alignment properties
		 *      (http://www.w3.org/TR/SVG/text.html#TextAlignmentProperties)
		 */
		ASCENSION_BEGIN_SCOPED_ENUM(TextAnchor)
			START = TextAlignment::START,
			MIDDLE = TextAlignment::CENTER,
			END = TextAlignment::END
		ASCENSION_END_SCOPED_ENUM;

		/**
		 * [Copied from CSS3] This property describes how the last line of a block or a line right
		 * before a forced line break is aligned. If a line is also the first line of the block or
		 * the first line after a forced line break, then, unless ‘text-align’ assigns an explicit
		 * first line alignment (via ‘start end’ , ‘text-align-last’ takes precedence over
		 * ‘text-align’  If ‘auto’ is specified, content on the affected line is aligned per
		 * ‘text-align’ unless ‘text-align’ is set to ‘justify’  In this case, content is justified
		 * if ‘text-justify’ is ‘distribute’ and start-aligned otherwise. All other values have the
		 * same meanings as in ‘text-align’ 
		 * @see CSS Text Level 3, 7.2. Last Line Alignment: the ‘text-align-last’ property
		 *      (http://www.w3.org/TR/css3-text/#text-align-last)
		 * @see XSL 1.1, 7.16.10 "text-align-last" (http://www.w3.org/TR/xsl/#text-align-last)
		 */
		ASCENSION_BEGIN_SCOPED_ENUM(TextAlignmentLast)
			START = TextAlignment::START,
			CENTER = TextAlignment::CENTER,
			END = TextAlignment::END,
			LEFT = TextAlignment::LEFT,
			RIGHT = TextAlignment::RIGHT,
			JUSTIFY = TextAlignment::JUSTIFY,
			AUTO = TextAlignment::START_END + 1
		ASCENSION_END_SCOPED_ENUM;

		class Presentation;
		TextAnchor defaultTextAnchor(const Presentation& presentation);

		/**
		 * [Copied from CSS3] This property selects the justification method used when a line's
		 * alignment is set to ‘justify’ (see ‘text-align’ , primarily by controlling which
		 * scripts' characters are adjusted together or separately. The property applies to block
		 * containers, but the UA may (but is not required to) also support it on inline elements.
		 * @see CSS Text Level 3, 7.3. Justification Method: the ‘text-justify’ property
		 *      (http://www.w3.org/TR/css3-text/#text-justify)
		 */
		ASCENSION_BEGIN_SCOPED_ENUM(TextJustification)
			/// Specifies no justification.
			AUTO, NONE, INTER_WORD, INTER_IDEOGRAPH, INTER_CLUSTER, DISTRIBUTE, KASHIDA
		ASCENSION_END_SCOPED_ENUM;

		/**
		 * [Copied from CSS3] Represents optimum, minimum, and maximum spacing.
		 * @tparam T The type of @c #optimum, @c #minimum and @c #maximum data members
		 * @see CSS Text Level 3, 8. Spacing (http://www.w3.org/TR/css3-text/#spacing)
		 * @see XSL 1.1, 4.3 Spaces and Conditionality (http://www.w3.org/TR/xsl/#spacecond)
		 */
		template<typename T>
		struct SpacingLimit {
			T optimum, minimum, maximum;
#if 0
			// followings are defined in only XSL 1.1
			boost::optional<int> precedence;	// boost.none means 'force'
			enum Conditionality {DISCARD, RETAIN} conditionality;
#endif
			SpacingLimit() {}
			template<typename U>
			explicit SpacingLimit(const U& allValues)
					: optimum(allValues), minimum(allValues), maximum(allValues) {}
			template<typename OptimumAndMinimum, typename Maximum>
			SpacingLimit(const OptimumAndMinimum& optimumAndMinimum, const Maximum& maximum)
					: optimum(optimumAndMinimum), minimum(optimumAndMinimum), maximum(maximum) {}
			template<typename Optimum, typename Minimum, typename Maximum>
			SpacingLimit(const Optimum& optimum, const Minimum& minimum, const Maximum& maximum)
					: optimum(optimum), minimum(minimum), maximum(maximum) {}
			template<typename U>
			SpacingLimit& operator=(const U& allValues) {
				optimum = minimum = maximum = allValues;
				return *this;
			}
			template<typename OptimumAndMinimum, typename Maximum>
			SpacingLimit& operator=(const std::tuple<OptimumAndMinimum, Maximum>& other) {
				this->optimum = this->minimum = std::get<0>(other);
				this->maximum = std::get<1>(other);
				return *this;
			}
			template<typename Optimum, typename Minimum, typename Maximum>
			SpacingLimit& operator=(const std::tuple<Optimum, Minimum, Maximum>& other) {
				this->optimum = std::get<0>(other);
				this->minimum = std::get<1>(other);
				this->maximum = std::get<2>(other);
				return *this;
			}
		};

		/**
		 * [Copied from CSS3] This property specifies the indentation applied to lines of inline
		 * content in a block.
		 * @tparam LengthType The type of @c #length. Usually @c Length or @c Scalar
		 * @see CSS Text Level 3, 9.1. First Line Indentation: the ‘text-indent’ property
		 *      (http://www.w3.org/TR/css3-text/#text-indent)
		 * @see XSL 1.1, 7.16.11 "text-indent" (http://www.w3.org/TR/xsl/#text-indent)
		 */
		template<typename LengthType>
		struct TextIndent {
			/**
			 * [Copied from CSS3] Gives the amount of the indent as an absolute length. If this is
			 * in percentage, as a percentage of the containing block's logical width
			 */
			LengthType length;
			/// [Copied from CSS3] Inverts which lines are affected.
			bool hanging;
			/**
			 * [Copied from CSS3] Indentation affects the first line of the block container as well
			 * as each line after a forced line break, but does not affect lines after a soft wrap
			 * break.
			 */
			bool eachLine;
		};

		/**
		 * [Copied from CSS3] This property determines whether a punctuation mark, if one is
		 * present, may be placed outside the line box (or in the indent) at the start or at the
		 * end of a line of text.
		 * @see CSS Text Level 3, 9.2. Hanging Punctuation: the ‘hanging-punctuation’ property
		 *      (http://www.w3.org/TR/css3-text/#hanging-punctuation)
		 */
		ASCENSION_BEGIN_SCOPED_ENUM(HangingPunctuation)
			// TODO: Some values should be able to be combined by bitwise-OR.
			NONE, FIRST, FORCE_END, ALLOW_END, LAST
		ASCENSION_END_SCOPED_ENUM;

		/**
		 * @see CSS Text Level 3, 10.1. Line Decoration: Underline, Overline, and Strike-Through
		 *      (http://www.w3.org/TR/css3-text/#line-decoration)
		 * @see SVG 1.1 (Second Edition), 10.12 Text decoration
		 *      (http://www.w3.org/TR/2011/REC-SVG11-20110816/text.html#TextDecorationProperties)
		 * @see XSL 1.1, 7.17.4 "text-decoration" (http://www.w3.org/TR/xsl/#text-decoration)
		 */
		struct TextDecoration {
			/**
			 * [Copied from CSS3] Specifies what line decorations, if any, are added to the element.
			 * @see CSS Text Level 3, 10.1.1. Text Decoration Lines: the ‘text-decoration-line’
			 *      property (http://dev.w3.org/csswg/css3-text/#text-decoration-line)
			 * @see SVG 1.1 (Second Edition), 10.12 Text decoration
			 *      (http://www.w3.org/TR/2011/REC-SVG11-20110816/text.html#TextDecorationProperties)
			 * @see XSL 1.1, 7.17.4 "text-decoration" (http://www.w3.org/TR/xsl/#text-decoration)
			 */
			ASCENSION_BEGIN_SCOPED_ENUM(Line)
				NONE = 0,				///< Neither produces nor inhibits text decoration.
				UNDERLINE = 1 << 0,		///< Each line of text is underlined.
				OVERLINE = 1 << 1,		///< Each line of text has a line above it.
//				BASELINE = 1 << 2,
				LINE_THROUGH = 1 << 3	///< Each line of text has a line through the middle.
			ASCENSION_END_SCOPED_ENUM;
			/**
			 * This property specifies the style of the line(s) drawn for text decoration specified
			 * on the element. 
			 * @see CSS Text Level 3, 10.1.3. Text Decoration Style: the ‘text-decoration-style’
			 *      property (http://dev.w3.org/csswg/css3-text/#text-decoration-style)
			 */
			ASCENSION_BEGIN_SCOPED_ENUM(Style)
				SOLID = Border::SOLID,		///< Same meaning as for @c Border#Style.
				DOUBLE = Border::DOUBLE,	///< Same meaning as for @c Border#Style.
				DOTTED = Border::DOTTED,	///< Same meaning as for @c Border#Style.
				DAHSED = Border::DASHED,	///< Same meaning as for @c Border#Style.
				WAVY = Border::OUTSET + 1	///< A wavy line.
			ASCENSION_END_SCOPED_ENUM;
			/**
			 * [Copied from CSS3] This property specifies what parts of the element's content any
			 * text decoration affecting the element must skip over. It controls all text
			 * decoration lines drawn by the element and also any text decoration lines drawn by
			 * its ancestors.
			 * @see CSS Text Level 3, 10.1.5. Text Decoration Line Continuity: the
			 *      ‘text-decoration-skip’ property
			 *      (http://dev.w3.org/csswg/css3-text/#text-decoration-skip)
			 */
			ASCENSION_BEGIN_SCOPED_ENUM(Skip)
				/// Skip nothing: text-decoration is drawn for all text content and for inline
				/// replaced elements.
				NONE = 0,
				/// Skip this element if it is an atomic inline (such as an image or inline-block).
				OBJECTS = 1 << 0,
				/// Skip white space: this includes regular spaces (U+0020) and tabs (U+0009), as
				/// well as nbsp (U+00A0), ideographic space (U+3000), all fixed width spaces (such
				/// as U+2000–U+200A, U+202F and U+205F), and any adjacent letter-spacing or
				/// word-spacing.
				SPACES = 1 << 2,
				/// Skip over where glyphs are drawn: interrupt the decoration line to let text
				/// show through where the text decoration would otherwise cross over a glyph. The
				/// UA may also skip a small distance to either side of the glyph outline.
				INK = 1 << 3,
				/// The UA should place the start and end of the line inwards from the content edge
				/// of the decorating element so that, e.g. two underlined elements side-by-side do
				/// not appear to have a single underline. (This is important in Chinese, where
				/// underlining is a form of punctuation.)
				EDGES = 1<< 4
			ASCENSION_END_SCOPED_ENUM;
			/**
			 * [Copied from CSS3] This property sets the position of an underline specified on the
			 * same element: it does not affect underlines specified by ancestor elements.
			 * @see CSS Text Level 3, 10.1.6. Text Underline Position: the
			 *      ‘text-underline-position’ property
			 *      (http://dev.w3.org/csswg/css3-text/#text-underline-position)
			 */
			ASCENSION_BEGIN_SCOPED_ENUM(UnderlinePosition)
				/// The user agent may use any algorithm to determine the underline's position;
				/// however it must be placed at or below the alphabetic baseline.
				AUTO,
				/// The underline is positioned relative to the alphabetic baseline. In this case
				/// the underline is likely to cross some descenders.
				ALPHABETIC,
				/// In horizontal writing modes, the underline is positioned relative to the under
				/// edge of the element's content box. In this case the underline usually does not
				/// cross the descenders.
				BELOW,
				BELOW_LEFT,
				BELOW_RIGHT,
				/// In vertical writing modes, the underline is aligned as for ‘below’ on the left
				/// edge of the text.
				LEFT,
				/// In vertical writing modes, the underline is aligned as for ‘below’ except it
				/// is aligned to the right edge of the text.
				RIGHT
			ASCENSION_END_SCOPED_ENUM;

			StyleProperty<
				sp::Enumerated<Line, Line::NONE>,
				sp::NotInherited
			> lines;	///< 'text-decoration-line' property.
			/**
			 * [Copied from CSS3] This property specifies the color of text decoration (underlines
			 * overlines, and line-throughs) set on the element with ‘text-decoration-line’
			 * @see CSS Text Level 3, 10.1.2. Text Decoration Color: the ‘text-decoration-color’
			 *      property (http://dev.w3.org/csswg/css3-text/#text-decoration-color)
			 */
			ColorProperty<sp::NotInherited> color;
			StyleProperty<
				sp::Enumerated<Style, Style::SOLID>,
				sp::NotInherited
			> style;	///< 'text-decoration-style' property.
			StyleProperty<
				sp::Enumerated<Skip, Skip::NONE>,
				sp::Inherited
			> skip;	///< 'text-decoration-skip' property.
			StyleProperty<
				sp::Enumerated<UnderlinePosition, UnderlinePosition::AUTO>,
				sp::Inherited
			> underlinePosition;	///< 'text-underline-position' property.
		};

		struct TextEmphasis {
			/// Enumerated values for @c #style.
			enum StyleEnums {
				/// No emphasis marks.
				NONE,
				/// The shape is filled with solid color.
				FILLED,
				/// The shape is hollow.
				OPEN,
				/// Display small circles as marks. The filled dot is U+2022 ‘•’ and the open dot
				/// is U+25E6 ‘◦’
				DOT,
				/// Display large circles as marks. The filled circle is U+25CF ‘●’ and the open
				/// circle is U+25CB ‘○’
				CIRCLE,
				/// Display double circles as marks. The filled double-circle is U+25C9 ‘◉’ and
				/// the open double-circle is U+25CE ‘◎’
				DOUBLE_CIRCLE,
				/// Display triangles as marks. The filled triangle is U+25B2 ‘▲’ and the open
				/// triangle is U+25B3 ‘△’
				TRIANGLE,
				/// Display sesames as marks. The filled sesame is U+FE45 ‘﹅’, and the open sesame
				/// is U+FE46 ‘﹆’.
				SESAME
			};
			/**
			 * [Copied from CSS3] This property describes where emphasis marks are drawn at.
			 * @see CSS Text Level 3, 10.2.4. Emphasis Mark Position: the ‘text-emphasis-position’
			 *      property (http://dev.w3.org/csswg/css3-text/#text-emphasis-position)
			 */
			enum Position {
				/// Draw marks over the text in horizontal writing mode.
				ABOVE,
				/// Draw marks under the text in horizontal writing mode.
				BELOW,
				/// Draw marks to the right of the text in vertical writing mode.
				RIGHT,
				/// Draw marks to the left of the text in vertical writing mode.
				LEFT
			};

			/**
			 * [Copied from CSS3] This property applies emphasis marks to the element's text.
			 * @see CSS Text Level 3, 10.2.1. Emphasis Mark Style: the ‘text-emphasis-style’
			 *      property (http://dev.w3.org/csswg/css3-text/#text-emphasis-style)
			 */
			StyleProperty<
				sp::Multiple<
					boost::variant<StyleEnums, CodePoint>,
					StyleEnums, NONE
				>,
				sp::Inherited
			> style;
			/**
			 * [Copied from CSS3] This property specifies the foreground color of the emphasis marks.
			 * @see CSS Text Level 3, 10.2.2. Emphasis Mark Color: the ‘text-emphasis-color’
			 *      property (http://dev.w3.org/csswg/css3-text/#text-emphasis-color)
			 */
			ColorProperty<sp::Inherited> color;
			StyleProperty<
				sp::Enumerated<Position, ABOVE | RIGHT>,
				sp::Inherited
			> position;	///< 'text-emphasis-position' property.
		};

		struct TextShadow {};

		/**
		 * Visual style settings of a text run.
		 * @see TextLineStyle, TextToplevelStyle, StyledTextRun, StyledTextRunIterator
		 */
		struct TextRunStyle :
				public FastArenaObject<TextRunStyle>,
				public std::enable_shared_from_this<TextRunStyle> {
#if 1
			/// Foreground color of the text content. See @c ColorProperty.
			ColorProperty<sp::Inherited> color;
#else
			/// Text paint style.
			std::shared_ptr<graphics::Paint> foreground;
#endif
			/// The background properties. See @c Background.
			Background background;
			/// Border of the text run. See the description of @c Border.
			Border border;
			/**
			 * @see CSS Fonts Module Level 3, 3.1 Font family: the font-family property
			 *      (http://www.w3.org/TR/css3-fonts/#font-family-prop)
			 * @see SVG 1.1 (Second Edition), 10.10 Font selection properties
			 *      (http://www.w3.org/TR/SVG11/text.html#FontFamilyProperty)
			 * @see XSL 1.1, 7.9.2 "font-family" (http://www.w3.org/TR/xsl/#font-family)
			 */
			StyleProperty<
				sp::Complex<
					graphics::font::FontFamiliesSpecification
				>, sp::Inherited
			> fontFamily;
			/// 'font-weight' property. See @c FontWeight.
			StyleProperty<
				sp::Enumerated<graphics::font::FontWeight, graphics::font::FontWeight::NORMAL>,
				sp::Inherited
			> fontWeight;
			/// 'font-stretch' property. See @c FontStretch.
			StyleProperty<
				sp::Enumerated<graphics::font::FontStretch, graphics::font::FontStretch::NORMAL>,
				sp::Inherited
			> fontStretch;
			/// 'font-style' property. See @c FontStyle.
			StyleProperty<
				sp::Enumerated<graphics::font::FontStyle, graphics::font::FontStyle::NORMAL>,
				sp::Inherited
			> fontStyle;
			/**
			 * [Copied from CSS3] This property indicates the desired height of glyphs from the
			 * font. For scalable fonts, the font-size is a scale factor applied to the EM unit of
			 * the font.
			 * @see CSS Fonts Module Level 3, 3.5 Font size: the font-size property
			 *      (http://www.w3.org/TR/css3-fonts/#font-size-prop)
			 * @see SVG 1.1 (Second Edition), 10.10 Font selection properties
			 *      (http://www.w3.org/TR/SVG11/text.html#FontFamilyProperty)
			 * @see XSL 1.1, 7.9.4 "font-size" (http://www.w3.org/TR/xsl/#font-size)
			 */
			StyleProperty<
				sp::Multiple<
					boost::variant<AbsoluteFontSize, RelativeFontSize, Length>,
					AbsoluteFontSize, AbsoluteFontSize::MEDIUM
				>, sp::Inherited
			> fontSize;
			/// 'font-size-adjust' property. @c boost#none means 'none'.
			StyleProperty<
				sp::Complex<
					boost::optional<double>
				>, sp::Inherited
			> fontSizeAdjust;
//			StyleProperty<
//				sp::Complex<
//					std::map<graphics::font::TrueTypeFontTag, uint32_t>
//				>, sp::Inherited
//			> fontFeatureSettings;
//			StyleProperty<
//				sp::Complex<
//					boost::optional<String>
//				>, sp::Inherited
//			> fontLanguageOverride;
			/**
			 * [Copied from CSS3] The ‘text-height’ property determine the block-progression
			 * dimension of the text content area of an inline box (non-replaced elements).
			 * @see CSS Line Layout Module Level 3, 3.3 Block-progression dimensions: the
			 *      ‘text-height’ property (http://dev.w3.org/csswg/css3-linebox/#inline1)
			 */
			StyleProperty<
				sp::Multiple<
					boost::variant<TextHeightEnums, double>,
					TextHeightEnums, TextHeightEnums::AUTO
				>, sp::Inherited
			> textHeight;
			/**
			 * [Copied from CSS3] The ‘line-height’ property controls the amount of leading space
			 * which is added before and after the block-progression dimension of an inline box
			 * (not including replaced inline boxes, but including the root inline box) to
			 * determine the extended block-progression dimension of the inline box.
			 * @see CSS Line Layout Module Level 3, 3.4.1 Line height adjustment: the ‘line-height’
			 *      property (http://dev.w3.org/csswg/css3-linebox/#InlineBoxHeight)
			 * @see XSL 1.1, 7.16.4 "line-height" (http://www.w3.org/TR/xsl/#line-height)
			 */
			StyleProperty<
				sp::Multiple<
					boost::variant<LineHeightEnums, double, Length>,
					LineHeightEnums, LineHeightEnums::NORMAL
				>, sp::Inherited
			> lineHeight;
			/// The dominant baseline of the line. See @c DominantBaseline.
			StyleProperty<
				sp::Enumerated<DominantBaseline, DominantBaseline::AUTO>,
				sp::NotInherited
			> dominantBaseline;
			/// The alignment baseline. Default value is @c ALIGNMENT_BASELINE_AUTO.
			StyleProperty<
				sp::Enumerated<AlignmentBaseline, AlignmentBaseline::BASELINE>,
				sp::NotInherited
			> alignmentBaseline;
			/**
			 * [Copied from CSS3] The ‘alignment-adjust’ property allows more precise alignment of
			 * elements, such as graphics, that do not have a baseline-table or lack the desired
			 * baseline in their baseline-table. With the ‘alignment-adjust’ property, the position
			 * of the baseline identified by the ‘alignment-baseline’ can be explicitly determined.
			 * It also determines precisely the alignment point for each glyph within a textual
			 * element. The user agent should use heuristics to determine the position of a non
			 * existing baseline for a given element.
			 * @see CSS Line Layout Module Level 3, 4.6 Setting the alignment point: the
			 *      ‘alignment-adjust’ property
			 *      (http://dev.w3.org/csswg/css3-linebox/#alignment-adjust-prop)
			 * @see CSS3 module: line, 4.6. Setting the alignment point: the 'alignment-adjust'
			 *      property (http://www.w3.org/TR/css3-linebox/#alignment-adjust-prop)
			 * @see XSL 1.1, 7.14.1 "alignment-adjust" (http://www.w3.org/TR/xsl/#alignment-adjust)
			 */
			StyleProperty<
				sp::Multiple<
					boost::variant<AlignmentAdjustEnums, Length>,
					AlignmentAdjustEnums, AlignmentAdjustEnums::AUTO
				>, sp::NotInherited
			> alignmentAdjust;
			/**
			 * [Copied from CSS3] The ‘baseline-shift’ property allows repositioning of the
			 * dominant-baseline relative to the dominant-baseline. The shifted object might be a
			 * sub- or superscript. Within the shifted element, the whole baseline table is offset;
			 * not just a single baseline. For sub- and superscript, the amount of offset is
			 * determined from the nominal font of the parent.
			 * @see CSS Line Layout Module Level 3, 4.7 Repositioning the dominant baseline: the
			 *      ‘baseline-shift’ property
			 *      (http://dev.w3.org/csswg/css3-linebox/#baseline-shift-prop)
			 * @see CSS3 module: line, 4.7. Repositioning the dominant baseline: the
			 *     'baseline-shift' property
			 *      (http://www.w3.org/TR/css3-linebox/#baseline-shift-prop)
			 * @see SVG 1.1 (Second Edition), 10.9.2 Baseline alignment properties
			 *      (http://www.w3.org/TR/SVG/text.html#BaselineShiftProperty)
			 * @see XSL 1.1, 7.14.3 "baseline-shift" (http://www.w3.org/TR/xsl/#baseline-shift)
			 */
			StyleProperty<
				sp::Multiple<
					boost::variant<BaselineShiftEnums, Length>,
					BaselineShiftEnums, BaselineShiftEnums::BASELINE
				>, sp::NotInherited
			> baselineShift;
			StyleProperty<
				sp::Enumerated<TextTransform, TextTransform::NONE>,
				sp::Inherited
			> textTransform;
			StyleProperty<
				sp::Enumerated<Hyphens, Hyphens::MANUAL>,
				sp::Inherited
			> hyphens;
			/**
			 * [Copied from CSS3] This property specifies the minimum, maximum, and optimal spacing
			 * between “words”. Additional spacing is applied to each word-separator character left
			 * in the text after the white space processing rules have been applied, and should be
			 * applied half on each side of the character.
			 * @see CSS Text Level 3, 8.1. Word Spacing: the ‘word-spacing’ property
			 *      (http://www.w3.org/TR/css3-text/#word-spacing)
			 * @see SVG 1.1 (Second Edition), 10.11 Spacing properties
			 *      (http://www.w3.org/TR/SVG11/text.html#WordSpacingProperty)
			 * @see XSL 1.1, 7.17.8 "word-spacing" (http://www.w3.org/TR/xsl/#word-spacing)
			 */
			SpacingLimit<
				StyleProperty<
					sp::Complex<boost::optional<Length>>,
					sp::Inherited
				>
			> wordSpacing;
			/**
			 * [Copied from CSS3] This property specifies the minimum, maximum, and optimal spacing
			 * between characters. Letter-spacing is applied in addition to any word-spacing.
			 * ‘normal’ optimum letter-spacing is typically zero. Letter-spacing must not be
			 * applied at the beginning or at the end of a line. At element boundaries, the total
			 * letter spacing between two characters is given by and rendered within the innermost
			 * element that contains the boundary. For the purpose of letter-spacing, each
			 * consecutive run of atomic inlines (such as image and/or inline blocks) is treated as
			 * a single character.
			 * @see CSS Text Level 3, 8.2. Word Spacing: the ‘letter-spacing’ property
			 *      (http://www.w3.org/TR/css3-text/#letter-spacing)
			 * @see SVG 1.1 (Second Edition), 10.11 Spacing properties
			 *      (http://www.w3.org/TR/SVG11/text.html#LetterSpacingProperty)
			 * @see XSL 1.1, 7.17.2 "letter-spacing" (http://www.w3.org/TR/xsl/#letter-spacing)
			 */
			SpacingLimit<
				StyleProperty<
					sp::Complex<boost::optional<Length>>,
					sp::Inherited
				>
			> letterSpacing;
			/// Text decoration properties. See @c TextDecoration.
			TextDecoration textDecoration;
			/// Text emphasis properties. See @c TextEmphasis.
			TextEmphasis textEmphasis;
			/// Text shadow properties. See @c TextShadow.
			TextShadow textShadow;
//			RubyProperties rubyProperties;
//			Effects effects;
			/// Set @c false to disable shaping. Default is @c true.
			StyleProperty<
				sp::Enumerated<bool, true>,
				sp::NotInherited
			> shapingEnabled;

			TextRunStyle& resolveInheritance(const TextRunStyle& base, bool baseIsRoot);
		};

		/**
		 * Represents a styled text run, with the beginning position (offset) in the line and the
		 * style.
		 * @note This class does not provides the length of the text run.
		 * @note This class is not intended to be derived.
		 * @see StyledTextRunIterator, StyledTextRunEnumerator
		 */
		class StyledTextRun {
		public:
			/// Default constructor.
			StyledTextRun() /*throw()*/ {}
			/**
			 * Constructor.
			 * @param position The beginning position of the text style
			 * @param style The style of the text run
			 */
			StyledTextRun(Index position,
				std::shared_ptr<const TextRunStyle> style) /*throw()*/ : position_(position), style_(style) {}
			/// Returns the position in the line of the text range which the style applies.
			Index position() const /*throw()*/ {return position_;}
			/// Returns the style of the text run.
			std::shared_ptr<const TextRunStyle> style() const /*throw()*/ {return style_;}
		private:
			Index position_;
			std::shared_ptr<const TextRunStyle> style_;
		};

		/**
		 *
		 * @see StyledTextRunEnumerator
		 */
		class StyledTextRunIterator {
		public:
			/// Destructor.
			virtual ~StyledTextRunIterator() /*throw()*/ {}
			/// Returns the current styled text run or throws @c NoSuchElementException.
			virtual StyledTextRun current() const = 0;
			/// Returns @c false if the iterator addresses the end of the range.
			virtual bool hasNext() const = 0;
			/// Moves the iterator to the next styled run or throws @c NoSuchElementException.
			virtual void next() = 0;
		};

		/**
		 *
		 * @see StyledTextRunIterator
		 */
		class StyledTextRunEnumerator : public boost::iterator_facade<
			StyledTextRunEnumerator, std::pair<Range<Index>, std::shared_ptr<const TextRunStyle>>,
			std::input_iterator_tag, std::pair<Range<Index>, std::shared_ptr<const TextRunStyle>>,
			std::ptrdiff_t
		> {
		public:
			StyledTextRunEnumerator();
			StyledTextRunEnumerator(std::unique_ptr<StyledTextRunIterator> sourceIterator, Index end);
		private:
			friend class boost::iterator_core_access;
			const reference dereference() const;
			bool equal(const StyledTextRunEnumerator& other) const /*throw()*/;
			void increment();
		private:
			std::unique_ptr<StyledTextRunIterator> iterator_;
			boost::optional<StyledTextRun> current_, next_;
			const Index end_;
		};

		struct NumberSubstitution {
			/// Specifies how to apply number substitution on digits and related punctuation.
			enum Method {
				/// Uses the user setting.
				USER_SETTING,
				/**
				 * The substitution method should be determined based on the system setting for
				 * the locale given in the text.
				 */
				FROM_LOCALE,
				/**
				 * The number shapes depend on the context (the nearest preceding strong character,
				 * or the reading direction if there is none).
				 */
				CONTEXTUAL,
				/**
				 * No substitution is performed. Characters U+0030..0039 are always rendered as
				 * nominal numeral shapes (European numbers, not Arabic-Indic digits).
				 */
				NONE,
				/// Numbers are rendered using the national number shapes.
				NATIONAL,
				/// Numbers are rendered using the traditional shapes for the specified locale.
				TRADITIONAL
			};

			/// The substitution method.
			StyleProperty<
				sp::Enumerated<Method, USER_SETTING>,
				sp::Inherited
			> method;
			/// The name of the locale to be used.
			StyleProperty<
				sp::Complex<std::string>,
				sp::Inherited
			> localeName;
			/// Whether to ignore user override.
			StyleProperty<
				sp::Enumerated<bool, false>,
				sp::Inherited
			> ignoreUserOverride;
		};

		/**
		 * Specifies the style of a text line. This object also gives the default text run style.
		 * @see TextRunStyle, TextToplevelStyle, TextLineStyleDirector
		 */
		struct TextLineStyle {
			/// The default text run style. The default value is @c null.
			std::shared_ptr<const TextRunStyle> defaultRunStyle;
			/// 'line-box-contain' property. See @c LineBoxContain.
			StyleProperty<
				sp::Enumerated<LineBoxContain, LineBoxContain::BLOCK | LineBoxContain::INLINE | LineBoxContain::REPLACED>,
				sp::Inherited
			> lineBoxContain;
			/// ‘inline-box-align’ property. See @c InlineBoxAlignment.
			StyleProperty<
				sp::Multiple<
					InlineBoxAlignment,
					InlineBoxAlignmentEnums, InlineBoxAlignmentEnums::LAST
				>, sp::NotInherited
			> inlineBoxAlignment;
//			StyleProperty<
//				sp::Enumerated<TextSpaceCollapse, TextSpaceCollapse::COLLAPSE>,
//				sp::Inherited
//			> textSpaceCollapse;
			/**
			 * [Copied from CSS3] This property determines the measure of the tab character
			 * (U+0009) when rendered. Integers represent the measure in space characters (U+0020).
			 * @see CSS Text Level 3, 3.2. Tab Character Size: the ‘tab-size’ property
			 *      (http://www.w3.org/TR/css3-text/#tab-size)
			 */
			StyleProperty<
				sp::Multiple<
					boost::variant<unsigned int, Length>,
					unsigned int, 8
				>, sp::Inherited
			> tabSize;
			/// The line breaking strictness. See @c LineBreak.
			StyleProperty<
				sp::Enumerated<LineBreak, LineBreak::AUTO>,
				sp::Inherited
			> lineBreak;
			/// The word breaking rules. See @c WordBreak.
			StyleProperty<
				sp::Enumerated<WordBreak, WordBreak::NORMAL>,
				sp::Inherited
			> wordBreak;
			/// 'text-wrap' property. See @c TextWrap.
			StyleProperty<
				sp::Enumerated<TextWrap, TextWrap::NORMAL>,
				sp::Inherited
			> textWrap;
			/// 'overflow-wrap' property. See @c OverflowWrap.
			StyleProperty<
				sp::Enumerated<OverflowWrap, OverflowWrap::NORMAL>,
				sp::Inherited
			> overflowWrap;
			/// 'text-align' property. See @c TextAlignment.
			StyleProperty<
				sp::Enumerated<TextAlignment, TextAlignment::START>,
				sp::Inherited
			> textAlignment;
			/// 'text-align-last' property. See @c TextAlignmentLast.
			StyleProperty<
				sp::Enumerated<TextAlignmentLast, TextAlignmentLast::AUTO>,
				sp::Inherited
			> textAlignmentLast;
			/// 'text-justify' property. See @c TextJustification.
			StyleProperty<
				sp::Enumerated<TextJustification, TextJustification::AUTO>,
				sp::Inherited
			> textJustification;
			/// 'text-indent' property. See @c TextIndent.
			StyleProperty<
				sp::Complex<TextIndent<Length>>,
				sp::Inherited
			> textIndent;
			/// 'hanging-punctuation' property. See @c HangingPunctuation.
			StyleProperty<
				sp::Enumerated<HangingPunctuation, HangingPunctuation::NONE>,
				sp::Inherited
			> hangingPunctuation;
			/// 'dominant-baseline' property. See @c DominantBaseline.
			StyleProperty<
				sp::Enumerated<DominantBaseline, DominantBaseline::AUTO>,
				sp::NotInherited
			> dominantBaseline;
			/// The number substitution process. The default value is @c NumberSubstitution().
			StyleProperty<
				sp::Complex<NumberSubstitution>,
				sp::Inherited
			> numberSubstitution;
		};

		std::shared_ptr<const TextRunStyle> defaultTextRunStyle(
			const TextLineStyle& textLineStyle) /*noexcept*/;

		/**
		 * 
		 * The writing modes specified by this style may be overridden by
		 * @c graphics#font#TextRenderer#writingMode.
		 * @see TextRunStyle, TextLineStyle, Presentation#globalTextStyle,
		 *      Presentation#setGlobalTextStyle
		 */
		struct TextToplevelStyle : public std::enable_shared_from_this<TextToplevelStyle> {
			/// 'direction' property. See @c ReadingDirection.
			StyleProperty<
				sp::Enumerated<ReadingDirection, LEFT_TO_RIGHT>,
				sp::Inherited
			> direction;
//			StyleProperty<
//				sp::Enumerated<UnicodeBidi, UnicodeBidi::NORMAL>,
//				sp::NotInherited
//			> unicodeBidi;
			/// 'writing-mode' property. See @c BlockFlowDirection.
			StyleProperty<
				sp::Enumerated<BlockFlowDirection, HORIZONTAL_TB>,
				sp::Inherited
			> writingMode;
			/// 'text-orientation' property. See @c TextOrientation.
			StyleProperty<
				sp::Enumerated<TextOrientation, MIXED_RIGHT>,
				sp::Inherited
			> textOrientation;
			/// The default text line style. The default value is @c null.
			std::shared_ptr<const TextLineStyle> defaultLineStyle;
		};

		/**
		 * @tparam Parent @c std#shared_ptr&lt;const TextLineStyle&gt; or
		 *                @c RulerConfiguration#LineNumbers*
		 */
		template<typename Parent, typename InheritedOrNot>
		inline graphics::Color computeColor(const ColorProperty<InheritedOrNot>* current,
				const Parent parent, const TextToplevelStyle& ancestor) {
			const ColorProperty<InheritedOrNot>* parentColor = nullptr;
			if(const TextLineStyle* p = parent.get()) {
				if(p->defaultRunStyle)
					parentColor = &p->defaultRunStyle->color;
			}
			const ColorProperty<InheritedOrNot>* ancestorColor = nullptr;
			if(ancestor.defaultLineStyle && ancestor.defaultLineStyle->defaultRunStyle)
				ancestorColor = &ancestor.defaultLineStyle->defaultRunStyle->color;
			return computeColor(current, parentColor,
				(ancestorColor != nullptr) ? *ancestorColor : ColorProperty<InheritedOrNot>());
		}

		inline std::unique_ptr<graphics::Paint> computeBackground(const Background* current,
				std::shared_ptr<const TextLineStyle> parent, const TextToplevelStyle& ancestor) {
			const Background* parentBackground = nullptr;
			if(const TextLineStyle* p = parent.get()) {
				if(p->defaultRunStyle)
					parentBackground = &p->defaultRunStyle->background;
			}
			const Background* ancestorBackground = nullptr;
			if(ancestor.defaultLineStyle && ancestor.defaultLineStyle->defaultRunStyle)
				ancestorBackground = &ancestor.defaultLineStyle->defaultRunStyle->background;
			Background inheritedBackground;
			inheritedBackground.color.inherit();
			return computeBackground(current, parentBackground,
				(ancestorBackground != nullptr) ? *ancestorBackground : inheritedBackground);
		}

		std::shared_ptr<const TextLineStyle> defaultTextLineStyle(
			const TextToplevelStyle& textToplevelStyle) /*noexcept*/;
	}

	namespace detail {
		ASCENSION_BEGIN_SCOPED_ENUM(PhysicalTextAnchor)
			LEFT = presentation::TextAlignment::LEFT,
			CENTER = presentation::TextAlignment::CENTER,
			RIGHT = presentation::TextAlignment::RIGHT
		ASCENSION_END_SCOPED_ENUM;

		inline PhysicalTextAnchor computePhysicalTextAnchor(
				presentation::TextAnchor anchor, presentation::ReadingDirection readingDirection) {
			switch(anchor) {
				case presentation::TextAnchor::MIDDLE:
					return PhysicalTextAnchor::CENTER;
				case presentation::TextAnchor::START:
					return (readingDirection == presentation::LEFT_TO_RIGHT) ? PhysicalTextAnchor::LEFT : PhysicalTextAnchor::RIGHT;
				case presentation::TextAnchor::END:
					return (readingDirection == presentation::LEFT_TO_RIGHT) ? PhysicalTextAnchor::RIGHT : PhysicalTextAnchor::LEFT;
				default:
					ASCENSION_ASSERT_NOT_REACHED();
			}
		}
	}
} // namespace ascension.presentation

#endif // !ASCENSION_TEXT_STYLE_HPP
