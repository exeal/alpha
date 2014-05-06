/**
 * @file text-style.hpp
 * @author exeal
 * @see presentation.hpp, graphics/text-alignment.hpp, graphics/text-layout-styles.hpp
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2011 was presentation.hpp
 * @date 2011-05-04 separated from presentation.hpp
 * @date 2012-07-16 reunioned with text-line-style.hpp
 * @date 2014
 */

#ifndef ASCENSION_TEXT_STYLE_HPP
#define ASCENSION_TEXT_STYLE_HPP

#include <ascension/corelib/string-piece.hpp>
#include <ascension/corelib/future/scoped-enum-emulation.hpp>
#include <ascension/directions.hpp>
#include <ascension/graphics/color.hpp>	// graphics.Color
#include <ascension/graphics/paint.hpp>	// graphics.Paint
#include <ascension/graphics/font/font.hpp>	// graphics.font.FontProperties, ...
#include <ascension/graphics/font/text-alignment.hpp>
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
	/// Defines presentative stuffs.
	namespace presentation {
		/// @defgroup css3_color CSS Color Module Level 3
		/// @{
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
#ifndef BOOST_NO_TEMPLATE_ALIASES
		template<typename InheritedOrNot> using ColorProperty =
			StyleProperty<sp::Complex<boost::optional<graphics::Color>>, InheritedOrNot>;
#else
		template<typename InheritedOrNot>
		class ColorProperty : public StyleProperty<
			sp::Complex<
				boost::optional<graphics::Color>
			>, InheritedOrNot
		> {
		private:
			typedef StyleProperty<sp::Complex<boost::optional<graphics::Color>>, InheritedOrNot> Base;
		public:
			ColorProperty() : Base() {}
			ColorProperty(const value_type& value) : Base(value) {}
		};
#endif

		/**
		 * Computes the specified color properties with inheritance and defaulting.
		 * @tparam InheritedOrNotForCurrentColor The template parameter for @a current
		 * @tparam InheritedOrNotForParentColor The template parameter for @a parent
		 * @tparam InheritedOrNotForAncestorColor The template parameter for @a ancestor
		 * @param current The declared color property of the current element
		 * @param parent The declared color property of the parent element
		 * @param ancestor The declared color property of the ancestor element
		 * @return A computed color value
		 */
		template<typename InheritedOrNotForCurrent,
			typename InheritedOrNotForParent, typename InheritedOrNotForAncestor>
		inline graphics::Color computeColor(
				const ColorProperty<InheritedOrNotForCurrent>* current,
				const ColorProperty<InheritedOrNotForParent>* parent,
				const ColorProperty<InheritedOrNotForAncestor>& ancestor) {
			if(current != nullptr && !current->inherits() && current->get() != boost::none)
				return *current->get();
			else if(parent != nullptr && !parent->inherits() && parent->get() != boost::none)
				return *parent->get();
			else if(!ancestor.inherits() && ancestor.get() != boost::none)
				return *ancestor.get();
			else
				return boost::get_optional_value_or(graphics::SystemColors::get(
					graphics::SystemColors::WINDOW_TEXT), graphics::Color::OPAQUE_BLACK);
		}
		/// @}

		/// @defgroup css3_background CSS Backgrounds and Borders Module Level 3
		/// @{
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
			ColorProperty<sp::NotInherited> color;
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


		/**
		 * Computes the specified background properties with inheritance and defaulting.
		 * @param current The declared background property of the current element
		 * @param parent The declared background property of the parent element
		 * @param ancestor The declared background property of the ancestor element
		 * @return A computed background value as @c Paint
		 */
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
				new graphics::SolidColor(boost::get_optional_value_or(
					graphics::SystemColors::get(graphics::SystemColors::WINDOW), graphics::Color::OPAQUE_WHITE)));
		}

		/**
		 * @see CSS Backgrounds and Borders Module Level 3, 4. Borders
		 *      (http://www.w3.org/TR/css3-background/#borders)
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
				ColorProperty<sp::NotInherited> color;
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
		/// @}

		/// @defgroup css3_basic_box_model CSS basic box model
		/// @{

		/// Enumerated values for @c TextRunStyle#padding.
		ASCENSION_SCOPED_ENUMS_BEGIN(PaddingEnums)
			AUTO
		ASCENSION_SCOPED_ENUMS_END;

		/// Enumerated values for @c TextRunStyle#margin.
		ASCENSION_SCOPED_ENUMS_BEGIN(MarginEnums)
			FILL,
			AUTO
		ASCENSION_SCOPED_ENUMS_END;
		/// @}

		/// @defgroup css3_fonts CSS Fonts Module Level 3
		/// @{
		/**
		 * [Copied from CSS3] An <absolute-size> keyword refers to an entry in a table of font
		 * sizes computed and kept by the user agent.
		 * @see http://www.w3.org/TR/css3-fonts/#ltabsolute-sizegt
		 */
		ASCENSION_SCOPED_ENUMS_BEGIN(AbsoluteFontSize)
			XX_SMALL, X_SMALL, SMALL, MEDIUM, LARGE, X_LARGE, XX_LARGE
		ASCENSION_SCOPED_ENUMS_END

		/**
		 * [Copied from CSS3] A <relative-size> keyword is interpreted relative to the table of
		 * font sizes and the font size of the parent element.
		 * @see http://www.w3.org/TR/css3-fonts/#ltrelative-sizegt
		 */
		ASCENSION_SCOPED_ENUMS_BEGIN(RelativeFontSize)
			LARGER, SMALLER
		ASCENSION_SCOPED_ENUMS_END

		/// Enumerated values for @c TextRunStyle#fontSizeAdjust.
		ASCENSION_SCOPED_ENUMS_BEGIN(FontSizeAdjustEnums)
			NONE, AUTO
		ASCENSION_SCOPED_ENUMS_END;
		/// @}

		/// @defgroup css3_line_layout CSS Line Layout Module Level 3
		/// @{

		/// Enumerated values for @c TextRunStyle#textHeight.
		ASCENSION_SCOPED_ENUMS_BEGIN(TextHeightEnums)
			AUTO, FONT_SIZE, TEXT_SIZE, MAX_SIZE
		ASCENSION_SCOPED_ENUMS_END;

		/// Enumerated values for @c TextRunStyle#lineHeight.
		ASCENSION_SCOPED_ENUMS_BEGIN(LineHeightEnums)
			NORMAL, NONE
		ASCENSION_SCOPED_ENUMS_END;

		using graphics::font::LineBoxContain;

		using graphics::font::DominantBaseline;

		using graphics::font::AlignmentBaseline;

		/// Enumerated values for @c TextRunStyle#alignmentAdjust.
		ASCENSION_SCOPED_ENUMS_BEGIN(AlignmentAdjustEnums)
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
		ASCENSION_SCOPED_ENUMS_END;

		/// Enumerated values for @c TextRunStyle#baselineShift.
		ASCENSION_SCOPED_ENUMS_BEGIN(BaselineShiftEnums)
			BASELINE,
			SUB,
			SUPER
		ASCENSION_SCOPED_ENUMS_END;

		/// Enumerated values for @c InlineBoxAlignment.
		ASCENSION_SCOPED_ENUMS_BEGIN(InlineBoxAlignmentEnums)
			INITIAL, LAST
		ASCENSION_SCOPED_ENUMS_END;

		/**
		 * [Copied from CSS3] The ‘inline-box-align’ property determines which line of a multi-line
		 * inline block aligns with the previous and next inline elements within a line.
		 * @see CSS Line Layout Module Level 3, 4.9 Inline box alignment: the
		 *      ‘inline-box-align’ property
		 *      (http://dev.w3.org/csswg/css3-linebox/#inline-box-align-prop)
		 */
		typedef boost::variant<InlineBoxAlignmentEnums, Index> InlineBoxAlignment;
		/// @}

		/// @defgroup css3_text CSS Text Level 3
		/// @{
		/**
		 * [Copied from CSS3] This property transforms text for styling purposes.
		 * @see CSS Text Level 3, 2.1. Transforming Text: the ‘text-transform’ property
		 *      (http://www.w3.org/TR/css3-text/#text-transform)
		 * @see XSL 1.1, 7.17.6 "text-transform" (http://www.w3.org/TR/xsl/#text-transform)
		 */
		ASCENSION_SCOPED_ENUMS_BEGIN(TextTransform)
			NONE, CAPITALIZE, UPPERCASE, LOWERCASE, FULL_WIDTH, FULL_SIZE_KANA
		ASCENSION_SCOPED_ENUMS_END;

		/**
		 * [Copied from CSS3] This property specifies two things: whether and how white space
		 * inside the element is collapsed, and whether lines may wrap at unforced soft wrap
		 * opportunities.
		 * @see CSS Text Level 3, 3. White Space and Wrapping: the ‘white-space’ property
		 *      (http://www.w3.org/TR/css3-text/#white-space)
		 */
		ASCENSION_SCOPED_ENUMS_BEGIN(WhiteSpace)
			NORMAL		= 1 << 0 | 1 << 1 | 1 << 2,
			PRE			= 0 << 0 | 0 << 1 | 0 << 2,
			NOWRAP		= 1 << 0 | 1 << 1 | 0 << 2,
			PRE_WRAP	= 0 << 0 | 0 << 1 | 1 << 2,
			PRE_LINE	= 0 << 0 | 1 << 1 | 1 << 2
		ASCENSION_SCOPED_ENUMS_END;

		inline bool collapsesNewLines(WhiteSpace value) BOOST_NOEXCEPT {
			return (value & (1 << 0)) != 0;
		}

		inline bool collapsesSpacesAndTabs(WhiteSpace value) BOOST_NOEXCEPT {
			return (value & (1 << 1)) != 0;
		}

		inline bool wrapsText(WhiteSpace value) BOOST_NOEXCEPT {
			return (value & (1 << 2)) != 0;
		}

		/**
		 * [Copied from CSS3] This property specifies the strictness of line-breaking rules applied
		 * within an element: particularly how wrapping interacts with punctuation and symbols.
		 * @see CSS Text Level 3, 5.2. Breaking Rules for Punctuation: the ‘line-break’ property
		 *      (http://www.w3.org/TR/css3-text/#line-break)
		 */
		ASCENSION_SCOPED_ENUMS_BEGIN(LineBreak)
			AUTO,
			LOOSE,
			NORMAL,
#ifdef STRICT
#	define ASCENSION_DEFINED_STRICT STRICT
#	undef STRICT
#endif
			STRICT
#ifdef ASCENSION_DEFINED_STRICT
#	define STRICT ASCENSION_DEFINED_STRICT
#	undef ASCENSION_DEFINED_STRICT
#endif
		ASCENSION_SCOPED_ENUMS_END;

		/**
		 * [Copied from CSS3] This property specifies soft wrap opportunities between letters.
		 * @see CSS Text Level 3, 5.3. Breaking Rules for Letters: the ‘word-break’ property
		 *      (http://www.w3.org/TR/css3-text/#word-break)
		 */
		ASCENSION_SCOPED_ENUMS_BEGIN(WordBreak)
			NORMAL,
			KEEP_ALL,
			BREAK_ALL
		ASCENSION_SCOPED_ENUMS_END;

		/**
		 * [Copied from CSS3] This property controls whether hyphenation is allowed to create more
		 * soft wrap opportunities within a line of text.
		 * @see CSS Text Level 3, 6.1. Hyphenation Control: the ‘hyphens’ property
		 *      (http://www.w3.org/TR/css3-text/#hyphens)
		 * @see XSL 1.1, 7.10 Common Hyphenation Properties
		 *      (http://www.w3.org/TR/xsl/#common-hyphenation-properties)
		 */
		ASCENSION_SCOPED_ENUMS_BEGIN(Hyphens)
			NONE,
			MANUAL,
			AUTO
		ASCENSION_SCOPED_ENUMS_END;

		/**
		 * [Copied from CSS3] This property specifies whether the UA may arbitrarily break within a
		 * word to prevent overflow when an otherwise-unbreakable string is too long to fit within
		 * the line box. It only has an effect when ‘white-space’ allows wrapping.
		 * @see CSS Text Level 3 - 6.2. Overflow Wrapping: the ‘word-wrap’/‘overflow-wrap’ property
		 *      (http://www.w3.org/TR/css3-text/#overflow-wrap)
		 */
		ASCENSION_SCOPED_ENUMS_BEGIN(OverflowWrap)
			NORMAL,
			BREAK_WORD/*,
			HYPHENATE*/
		ASCENSION_SCOPED_ENUMS_END;

		using graphics::font::TextAlignment;

		using graphics::font::TextAnchor;

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
		ASCENSION_SCOPED_ENUMS_BEGIN(TextAlignmentLast)
			START = TextAlignment::START,
			CENTER = TextAlignment::CENTER,
			END = TextAlignment::END,
			LEFT = TextAlignment::LEFT,
			RIGHT = TextAlignment::RIGHT,
			JUSTIFY = TextAlignment::JUSTIFY,
			AUTO = TextAlignment::START_END + 1
		ASCENSION_SCOPED_ENUMS_END;

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
		ASCENSION_SCOPED_ENUMS_BEGIN(TextJustification)
			/// Specifies no justification.
			AUTO, NONE, INTER_WORD, INTER_IDEOGRAPH, INTER_CLUSTER, DISTRIBUTE, KASHIDA
		ASCENSION_SCOPED_ENUMS_END;

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
		 * @tparam LengthType The type of @c #length data member. Usually @c Length or @c Scalar
		 * @tparam BooleanType The type of @c #hanging and @c #eachLine data members
		 * @see CSS Text Level 3, 9.1. First Line Indentation: the ‘text-indent’ property
		 *      (http://www.w3.org/TR/css3-text/#text-indent)
		 * @see XSL 1.1, 7.16.11 "text-indent" (http://www.w3.org/TR/xsl/#text-indent)
		 */
		template<typename LengthType, typename BooleanType>
		struct TextIndent {
			/**
			 * [Copied from CSS3] Gives the amount of the indent as an absolute length. If this is
			 * in percentage, as a percentage of the containing block's logical width
			 */
			LengthType length;
			/// [Copied from CSS3] Inverts which lines are affected.
			BooleanType hanging;
			/**
			 * [Copied from CSS3] Indentation affects the first line of the block container as well
			 * as each line after a forced line break, but does not affect lines after a soft wrap
			 * break.
			 */
			BooleanType eachLine;
			/**
			 * Default constructor initializes @c length by calling default constructor of
			 * @c LengthType, and @c hanging and @c eachLine with false.
			 */
			TextIndent() : length(), hanging(false), eachLine(false) {}
		};

		/**
		 * [Copied from CSS3] This property determines whether a punctuation mark, if one is
		 * present, may be placed outside the line box (or in the indent) at the start or at the
		 * end of a line of text.
		 * @see CSS Text Level 3, 9.2. Hanging Punctuation: the ‘hanging-punctuation’ property
		 *      (http://www.w3.org/TR/css3-text/#hanging-punctuation)
		 */
		ASCENSION_SCOPED_ENUMS_BEGIN(HangingPunctuation)
			// TODO: Some values should be able to be combined by bitwise-OR.
			NONE, FIRST, FORCE_END, ALLOW_END, LAST
		ASCENSION_SCOPED_ENUMS_END;
		/// @}

		/// @defgroup css3_text_decor CSS Text Decoration Module Level 3
		/// @{
		/**
		 * [Copied from CSS3] Describes line decorations that are added to the content of an element.
		 * @see CSS Text Decoration Module Level 3, 2. Line Decoration: Underline, Overline, and
		 *      Strike-Through (http://dev.w3.org/csswg/css-text-decor-3/#line-decoration)
		 * @see SVG 1.1 (Second Edition), 10.12 Text decoration
		 *      (http://www.w3.org/TR/2011/REC-SVG11-20110816/text.html#TextDecorationProperties)
		 * @see XSL 1.1, 7.17.4 "text-decoration" (http://www.w3.org/TR/xsl/#text-decoration)
		 */
		struct TextDecoration {
			/**
			 * [Copied from CSS3] Specifies what line decorations, if any, are added to the element.
			 * @see CSS Text Decoration Module Level 3, 2.1. Text Decoration Lines: the
			 *      ‘text-decoration-line’ property
			 *      (http://dev.w3.org/csswg/css-text-decor-3/#text-decoration-line-property)
			 * @see SVG 1.1 (Second Edition), 10.12 Text decoration
			 *      (http://www.w3.org/TR/2011/REC-SVG11-20110816/text.html#TextDecorationProperties)
			 * @see XSL 1.1, 7.17.4 "text-decoration" (http://www.w3.org/TR/xsl/#text-decoration)
			 */
			ASCENSION_SCOPED_ENUMS_BEGIN(Line)
				NONE = 0,				///< Neither produces nor inhibits text decoration.
				UNDERLINE = 1 << 0,		///< Each line of text is underlined.
				OVERLINE = 1 << 1,		///< Each line of text has a line above it.
//				BASELINE = 1 << 2,
				LINE_THROUGH = 1 << 3	///< Each line of text has a line through the middle.
			ASCENSION_SCOPED_ENUMS_END;
			/**
			 * [Copied from CSS3] This property specifies the style of the line(s) drawn for text
			 * decoration specified on the element. 
			 * @see CSS Text Decoration Module Level 3, 2.3. Text Decoration Style: the
			 *      ‘text-decoration-style’ property
			 *      (http://dev.w3.org/csswg/css-text-decor-3/#text-decoration-style-property)
			 */
			ASCENSION_SCOPED_ENUMS_BEGIN(Style)
				SOLID = Border::SOLID,		///< Same meaning as for @c Border#Style.
				DOUBLE = Border::DOUBLE,	///< Same meaning as for @c Border#Style.
				DOTTED = Border::DOTTED,	///< Same meaning as for @c Border#Style.
				DAHSED = Border::DASHED,	///< Same meaning as for @c Border#Style.
				WAVY = Border::OUTSET + 1	///< A wavy line.
			ASCENSION_SCOPED_ENUMS_END;
			/**
			 * [Copied from CSS3] This property specifies what parts of the element's content any
			 * text decoration affecting the element must skip over. It controls all text
			 * decoration lines drawn by the element and also any text decoration lines drawn by
			 * its ancestors.
			 * @see CSS Text Decoration Module Level 3, 2.5. Text Decoration Line Continuity: the
			 *      ‘text-decoration-skip’ property
			 *      (http://dev.w3.org/csswg/css-text-decor-3/#text-decoration-skip-property)
			 */
			ASCENSION_SCOPED_ENUMS_BEGIN(Skip)
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
				EDGES = 1 << 4,
				/// Skip over the box's margin, border, and padding areas. Note that this only has
				/// an effect on decorations imposed by an ancestor.
				BOX_DECORATION = 1 << 5
			ASCENSION_SCOPED_ENUMS_END;
			/**
			 * [Copied from CSS3] This property sets the position of an underline specified on the
			 * same element: it does not affect underlines specified by ancestor elements.
			 * @see CSS Text Decoration Module Level 3, 2.6. Text Underline Position: the
			 *      ‘text-underline-position’ property
			 *      (http://dev.w3.org/csswg/css-text-decor-3/#text-underline-position-property)
			 */
			ASCENSION_SCOPED_ENUMS_BEGIN(UnderlinePosition)
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
				/// Combination of @c #BELOW and @c #LEFT.
				BELOW_LEFT,
				/// Combination of @c #BELOW and @c #RIGHT.
				BELOW_RIGHT,
				/// In vertical writing modes, the underline is aligned as for ‘below’ on the left
				/// edge of the text.
				LEFT,
				/// In vertical writing modes, the underline is aligned as for ‘below’ except it
				/// is aligned to the right edge of the text.
				RIGHT
			ASCENSION_SCOPED_ENUMS_END;

			StyleProperty<
				sp::Enumerated<Line, Line::NONE>,
				sp::NotInherited
			> lines;	///< 'text-decoration-line' property.
			/**
			 * [Copied from CSS3] This property specifies the color of text decoration (underlines
			 * overlines, and line-throughs) set on the element with ‘text-decoration-line’
			 * @see CSS Text Decoration Module Level 3, 2.2. Text Decoration Color: the
			 *      ‘text-decoration-color’ property
			 *      (http://dev.w3.org/csswg/css-text-decor-3/#text-decoration-color-property)
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

		/**
		 * [Copied from CSS3] East Asian documents traditionally use small symbols next to each
		 * glyph to emphasize a run of text.
		 * @see CSS Text Decoration Module Level 3, 3. Emphasis Marks
		 *      (http://dev.w3.org/csswg/css-text-decor-3/#emphasis-marks)
		 */
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
			 * @see CSS Text Decoration Module Level 3, 3.4. Emphasis Mark Position: the
			 *      ‘text-emphasis-position’ property
			 *      (http://dev.w3.org/csswg/css-text-decor-3/#text-emphasis-position-property)
			 */
			typedef unsigned char Position;
			static const Position
				ABOVE = 0,	///< Draw marks over the text in horizontal writing mode.
				BELOW = 1,	///< Draw marks under the text in horizontal writing mode.
				RIGHT = 0,	///< Draw marks to the right of the text in vertical writing mode.
				LEFT = 2;	///< Draw marks to the left of the text in vertical writing mode.

			/**
			 * [Copied from CSS3] This property applies emphasis marks to the element's text.
			 * @see	CSS Text Decoration Module Level 3, 3.1. Emphasis Mark Style: the
			 *      ‘text-emphasis-style’ property
			 *      (http://dev.w3.org/csswg/css-text-decor-3/#text-emphasis-style-property)
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
			 * @see CSS Text Decoration Module Level 3, 3.2. Emphasis Mark Color: the
			 *      ‘text-emphasis-color’ property
			 *      (http://dev.w3.org/csswg/css-text-decor-3/#text-decoration-color-property)
			 */
			ColorProperty<sp::Inherited> color;
			StyleProperty<
				sp::Enumerated<Position, ABOVE | RIGHT>,
				sp::Inherited
			> position;	///< 'text-emphasis-position' property.
		};

		/**
		 * @see CSS Text Decoration Module Level 3, 4. Text Shadows: the ‘text-shadow’ property
		 *      (http://dev.w3.org/csswg/css-text-decor-3/#text-shadow-property)
		 */
		struct TextShadow {};
		/// @}

		/**
		 * Visual style settings of a text run.
		 * @see TextLineStyle, TextToplevelStyle, StyledTextRun, StyledTextRunIterator
		 */
		struct TextRunStyle :
				public FastArenaObject<TextRunStyle>,
				public std::enable_shared_from_this<TextRunStyle> {
			/// @name Colors
			/// @{
#if 1
			/// Foreground color of the text content. See @c ColorProperty.
			ColorProperty<sp::Inherited> color;
#else
			/// Text paint style.
			std::shared_ptr<graphics::Paint> foreground;
#endif
			/// @}

			/// @name Backgrounds and Borders
			/// @{
			Background background;	///< The background properties. See @c Background.
			Border border;	///< Border of the text run. See the description of @c Border.
			/// @}

			/// @name Basic Box Model
			/// @{
			/**
			 * [Copied from CSS3] Sets the thickness of the padding area. The value may not be
			 * negative.
			 * @see CSS basic box model, 7. The padding properties
			 *      (http://dev.w3.org/csswg/css3-box/#the-padding-properties)
			 */
			FlowRelativeFourSides<
				StyleProperty<
					sp::Multiple<
						boost::variant<PaddingEnums, Length>,
						Length, 0
					>, sp::NotInherited
				>
			> padding;
			/**
			 * [Copied from CSS3] These properties set the thickness of the margin area. The value
			 * may be negative.
			 * @see CSS basic box model, 8. Margins
			 *      (http://dev.w3.org/csswg/css3-box/#margins)
			 */
			FlowRelativeFourSides<
				StyleProperty<
					sp::Multiple<
						boost::variant<MarginEnums, Length>,
						Length, 0
					>, sp::NotInherited
				>
			> margin;
			/// @}

			/// @name Fonts
			/// @{
			/**
			 * @see CSS Fonts Module Level 3, 3.1 Font family: the font-family property
			 *      (http://www.w3.org/TR/css3-fonts/#font-family-prop)
			 * @see SVG 1.1 (Second Edition), 10.10 Font selection properties
			 *      (http://www.w3.org/TR/SVG11/text.html#FontFamilyProperty)
			 * @see XSL 1.1, 7.9.2 "font-family" (http://www.w3.org/TR/xsl/#font-family)
			 */
			StyleProperty<
				sp::Complex<
					std::vector<graphics::font::FontFamily>
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
			/// 'font-size-adjust' property.
			StyleProperty<
				sp::Multiple<
					boost::variant<FontSizeAdjustEnums, graphics::Scalar>,
					FontSizeAdjustEnums, FontSizeAdjustEnums::NONE
				>, sp::Inherited
			> fontSizeAdjust;
//			StyleProperty<
//				sp::Complex<
//					std::map<graphics::font::TrueTypeFontTag, std::uint32_t>
//				>, sp::Inherited
//			> fontFeatureSettings;
//			StyleProperty<
//				sp::Complex<
//					boost::optional<String>
//				>, sp::Inherited
//			> fontLanguageOverride;
			/// @}

			/// @defgroup css3_linebox Line Layout
			/// @{
			/**
			 * [Copied from CSS3] The ‘text-height’ property determine the block-progression
			 * dimension of the text content area of an inline box (non-replaced elements).
			 * @see CSS Line Layout Module Level 3, 3.3 Block-progression dimensions: the
			 *      ‘text-height’ property (http://dev.w3.org/csswg/css3-linebox/#inline1)
			 */
			StyleProperty<
				sp::Multiple<
					boost::variant<TextHeightEnums, graphics::Scalar>,
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
					boost::variant<LineHeightEnums, graphics::Scalar, Length>,
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
			/// @}

			/// @addtogroup css3_text
			/// @{
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
			/// @}

			/// @addtogroup css3_text_decor
			/// @{
			/// Text decoration properties. See @c TextDecoration.
			TextDecoration textDecoration;
			/// Text emphasis properties. See @c TextEmphasis.
			TextEmphasis textEmphasis;
			/// Text shadow properties. See @c TextShadow.
			TextShadow textShadow;
			/// @}

//			RubyProperties rubyProperties;
//			Effects effects;
			/// Set @c false to disable shaping. Default is @c true.
			StyleProperty<
				sp::Enumerated<bool, true>,
				sp::NotInherited
			> shapingEnabled;

			TextRunStyle& resolveInheritance(const TextRunStyle& base, bool baseIsRoot);
		};
#if 0
		/**
		 * A @c StyledTextRun represents a text range with declared style. @c #beginning and
		 * @c #end return pointers to characters in the line text string.
		 * @note This class is not intended to be derived.
		 * @see StyledTextRunIterator, StyledTextRunEnumerator
		 */
		struct StyledTextRun : public StringPiece, public FastArenaObject<StyledTextRun> {
			/// The declared style in this text run.
			std::shared_ptr<const TextRunStyle> style;
			/// Default constructor.
			StyledTextRun() BOOST_NOEXCEPT {}
			/**
			 * Constructor.
			 * @param characterRange The range of the text run in the line
			 * @param style The declared style of the text run. Can be @c null
			 */
			StyledTextRun(const StringPiece& characterRange,
				std::shared_ptr<const TextRunStyle> style) BOOST_NOEXCEPT
				: StringPiece(characterRange), style_(style) {}
		};
#endif
		/**
		 * Abstract input iterator to obtain @c StyledTextRun objects.
		 * @see TextRunStyleDeclarator, graphics#font#ComputedStyledTextRunIterator
		 */
		class StyledTextRunIterator {
		public:
			/// Destructor.
			virtual ~StyledTextRunIterator() BOOST_NOEXCEPT {}
			/**
			 * Returns the range of the current text run addressed by this iterator.
			 * @return The range of the current text run this iterator addresses in character
			 *         offsets in the line. @c beginning() should be greater than or equal to
			 *         @c end for the previous text run. If @c end is greater than the length of
			 *         the line, the range is truncated. Otherwise if @c beginning() is greater
			 *         than @c end() of the previous text run, treated as if there is a text run
			 *         with the range [previous's @c end(), beginning()) and default style
			 * @throw NoSuchElementException This iterator is done
			 * @see #currentStyle
			 */
			virtual boost::integer_range<Index> currentRange() const = 0;
			/**
			 * Returns the declared style of the current text run addressed by this iterator.
			 * @return The style of the current text run this iterator addresses. If @c null, the
			 *         default text run is used
			 * @throw NoSuchElementException This iterator is done
		 	 * @see #currentRange
			 */
			virtual std::shared_ptr<const TextRunStyle> currentStyle() const = 0;
			/// Returns @c true if the iterator addresses the end of the range.
			virtual bool isDone() const BOOST_NOEXCEPT = 0;
			/**
			 * Moves the iterator to the next styled text run.
			 * @throw NoSuchElementException This iterator is done.
			 */
			virtual void next() = 0;
		};

		/**
		 * Specifies how numbers in text are displayed in different locales.
		 * @see TextLineStyle#numberSubstitution, RulerStyles#LineNumbers#numberSubstitution
		 */
		struct NumberSubstitution {
			/// Specifies how the locale for numbers in a text run is determined.
			enum LocaleSource {
				/// Number locale is derived from the text run.
				TEXT,
				/// Number locale is derived from the value of the current thread.
				USER,
				/// Number locale is derived from @c #localeOverride.
				OVERRIDE
			};

			/// The type of number substitution to perform on numbers in a text run.
			enum Method {
				/// The substitution method should be determined based on the number locale.
				AS_LOCALE,
				/// If the number locale is an Arabic or Farsi, specifies that the digits depend on
				/// the context. Either traditional or Latin digits are used depending on the
				/// nearest preceding strong character, or if there is none, the text direction of
				/// the paragraph.
				CONTEXT,
				/// Code points U+0030..0039 are always rendered as European digits, in which case,
				/// no number substitution is performed.
				EUROPEAN,
				/// Numbers are rendered using the national digits for the number locale, as
				/// specified by the locale.
				NATIVE_NATIONAL,
				/// Numbers are rendered using the traditional digits for the number locale. For
				/// most locales, this is the same as @c NATIVE_NATIONAL enumeration value.
				/// However, using @c NATIVE_NATIONAL can result in Latin digits for some Arabic
				/// locales, whereas using @c TRADITIONAL results in Arabic digits for all Arabic
				/// locales.
				TRADITIONAL
			};

			/**
			 * The locale to use when the value of @c #localeSource is @c LocaleSource#OVERRIDE. If
			 * @c #localeSource is not @c LocaleSource#OVERRIDE, this is ignored. The default value
			 * is an empty string.
			 * @see TextLineStyle#numberSubstitutionLocaleOverride
			 */
			std::string localeOverride;
			/**
			 * The source of the locale that is used to determine number substitution. The default
			 * value is @c LocaleSource#TEXT.
			 * @see TextLineStyle#numberSubstitutionLocaleSource
			 */
			LocaleSource localeSource;
			/**
			 * The substitution method that is used to determine number substitution. The default
			 * value is @c Method#AS_LOCALE.
			 * @see TextLineStyle#numberSubstitutionMethod
			 */
			Method method;

			/// Default constructor initializes the all data members with their default values.
			NumberSubstitution() BOOST_NOEXCEPT : localeSource(TEXT), method(AS_LOCALE) {}
		};

		/**
		 * Specifies the style of a text line. This object also gives the default text run style.
		 * @see TextRunStyle, TextToplevelStyle, TextLineStyleDirector
		 */
		struct TextLineStyle {
			/**
			 * The default text run style. The default value is @c null.
			 * @see defaultTextRunStyle
			 */
			std::shared_ptr<const TextRunStyle> defaultRunStyle;
			/// 'direction' property. See @c ReadingDirection.
			StyleProperty<
				sp::Enumerated<ReadingDirection, LEFT_TO_RIGHT>,
				sp::Inherited
			> direction;
//			StyleProperty<
//				sp::Enumerated<UnicodeBidi, UnicodeBidi::NORMAL>,
//				sp::NotInherited
//			> unicodeBidi;
			/// 'text-orientation' property. See @c TextOrientation.
			StyleProperty<
				sp::Enumerated<TextOrientation, MIXED_RIGHT>,
				sp::Inherited
			> textOrientation;
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
			/// ‘white-space’ property. See @c WhiteSpace.
			StyleProperty<
				sp::Enumerated<WhiteSpace, WhiteSpace::NORMAL>,
				sp::Inherited
			> whiteSpace;
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
				sp::Complex<
					TextIndent<Length, bool>
				>, sp::Inherited
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
			///
			StyleProperty<
				sp::Multiple<
					boost::variant<LineHeightEnums, graphics::Scalar, Length>,
					LineHeightEnums, LineHeightEnums::NORMAL
				>, sp::Inherited
			> lineHeight;
			/// ‘width’ property.
			StyleProperty<
				sp::Complex<
					boost::optional<Length>
				>, sp::NotInherited
			> measure;
			/// ‘NumberSubstitution.CultureOverride’ property. See @c NumberSubstitution.
			StyleProperty<
				sp::Complex<
					std::string
				>, sp::Inherited
			> numberSubstitutionLocaleOverride;
			/// ‘NumberSubstitution.CultureSource’ property. See @c NumberSubstitution.
			StyleProperty<
				sp::Enumerated<NumberSubstitution::LocaleSource, NumberSubstitution::TEXT>,
				sp::Inherited
			> numberSubstitutionLocaleSource;
			/// ‘NumberSubstitution.Substitution’ property. See @c NumberSubstitution.
			StyleProperty<
				sp::Enumerated<NumberSubstitution::Method, NumberSubstitution::AS_LOCALE>,
				sp::Inherited
			> numberSubstitutionMethod;
		};

		/**
		 * 
		 * The writing modes specified by this style may be overridden by
		 * @c graphics#font#TextRenderer#writingMode.
		 * @see TextRunStyle, TextLineStyle, Presentation#textToplevelStyle,
		 *      Presentation#setTextToplevelStyle
		 */
		struct TextToplevelStyle : public std::enable_shared_from_this<TextToplevelStyle> {
			/// 'writing-mode' property. See @c BlockFlowDirection.
			StyleProperty<
				sp::Enumerated<BlockFlowDirection, HORIZONTAL_TB>,
				sp::Inherited
			> writingMode;
			/**
			 * The default text line style. The default value is @c null.
			 * @see defaultTextLineStyle
			 */
			std::shared_ptr<const TextLineStyle> defaultLineStyle;
		};

		inline std::shared_ptr<const TextRunStyle> defaultTextRunStyle(const TextLineStyle&);
		inline std::shared_ptr<const TextLineStyle> defaultTextLineStyle(const TextToplevelStyle&);
				
		/**
		 * @see Presentation#computeTextLineStyle
		 */
		class GlobalTextStyleSwitch {
		public:
			/// Destructor.
			virtual ~GlobalTextStyleSwitch() BOOST_NOEXCEPT {}
		private:
			/**
			 * Returns the 'direction' style property which follows @c TextLineStyle#direction and
			 * overrides @c TextToplevelStyle#defaultLineStyle#direction.
			 * @return The declared value of 'direction' style property
			 */
			virtual decltype(TextLineStyle().direction) direction() const BOOST_NOEXCEPT = 0;
			/**
			 * Returns the 'text-align' style property which follows @c TextLineStyle#textAlignment
			 * and overrides @c TextToplevelStyle#defaultLineStyle#textAlignment.
			 * @return The declared value of 'text-align' style property
			 */
			virtual decltype(TextLineStyle().textAlignment) textAlignment() const BOOST_NOEXCEPT = 0;
			/**
			 * Returns 'text-orientation' style property which follows
			 * @c TextLineStyle#textOrientation and overrides
			 * @c TextToplevelStyle#defaultLineStyle#textOrientation.
			 * @return The declared value of 'text-orientation' style property
			 */
			virtual decltype(TextLineStyle().textOrientation) textOrientation() const BOOST_NOEXCEPT = 0;
			/**
			 * Returns 'white-space' style property which follows @c TextLineStyle#whiteSpace and
			 * overrides @c TextToplevelStyle#defaultLineStyle#whiteSpace.
			 * @return The declared value of 'white-space' style property
			 */
			virtual decltype(TextLineStyle().whiteSpace) whiteSpace() const BOOST_NOEXCEPT = 0;
			/**
			 * Returns the 'writing-mode' style property which follows
			 * @c TextToplevelStyle#writingMode.
			 * @return The declared value of 'writing-mode' style property
			 */
			virtual decltype(TextToplevelStyle().writingMode) writingMode() const BOOST_NOEXCEPT = 0;
			friend class Presentation;
		};

		/**
		 * @overload
		 * @tparam InheritedOrNotForCurrentColor The template parameter for @a current
		 * @tparam InheritedOrNotForParentColor The template parameter for @a parent
		 * @param current The declared color property of the current element
		 * @param parent The declared color property of the parent element
		 * @param ancestor The top-level style provides the color property
		 * @return A computed color value
		 * @ingroup css3_color
		 */
		template<typename InheritedOrNotForCurrentColor, typename InheritedOrNotForParentColor>
		inline graphics::Color computeColor(const ColorProperty<InheritedOrNotForCurrentColor>* current,
				const ColorProperty<InheritedOrNotForParentColor>* parent, const TextToplevelStyle& ancestor) {
			const ColorProperty<sp::Inherited>* ancestorColor = nullptr;
			if(ancestor.defaultLineStyle && ancestor.defaultLineStyle->defaultRunStyle)
				ancestorColor = &ancestor.defaultLineStyle->defaultRunStyle->color;
			return computeColor(current, parent, (ancestorColor != nullptr) ? *ancestorColor : ColorProperty<sp::Inherited>());
		}

		/**
		 * @overload
		 * @param current The declared color property of the current element
		 * @param parent The declared color property of the parent element
		 * @param ancestor The top-level style provides the background property
		 * @return A computed background value as @c Paint
		 * @ingroup css3_background
		 */
		inline std::unique_ptr<graphics::Paint> computeBackground(
				const Background* current, const Background* parent, const TextToplevelStyle& ancestor) {
			const Background* ancestorBackground = nullptr;
			if(ancestor.defaultLineStyle && ancestor.defaultLineStyle->defaultRunStyle)
				ancestorBackground = &ancestor.defaultLineStyle->defaultRunStyle->background;
			Background inheritedBackground;
			inheritedBackground.color.inherit();
			return computeBackground(current, parent, (ancestorBackground != nullptr) ? *ancestorBackground : inheritedBackground);
		}

		namespace detail {
			ASCENSION_SCOPED_ENUMS_BEGIN(PhysicalTextAnchor)
				LEFT = presentation::TextAlignment::LEFT,
				CENTER = presentation::TextAlignment::CENTER,
				RIGHT = presentation::TextAlignment::RIGHT
			ASCENSION_SCOPED_ENUMS_END;

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
	}
} // namespace ascension.presentation

#endif // !ASCENSION_TEXT_STYLE_HPP
