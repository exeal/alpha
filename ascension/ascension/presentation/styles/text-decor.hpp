﻿/**
 * @file text-decor.hpp
 * @author exeal
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2011 was presentation.hpp
 * @date 2011-05-04 separated from presentation.hpp
 * @date 2012-07-16 reunioned with text-line-style.hpp
 * @date 2014-09-21 Separated from text-style.hpp
 */

#ifndef ASCENSION_STYLES_TEXT_DECOR_HPP
#define ASCENSION_STYLES_TEXT_DECOR_HPP
#include <ascension/corelib/future/scoped-enum-emulation.hpp>
#include <ascension/corelib/text/code-point.hpp>
#include <ascension/presentation/style-property.hpp>
#include <ascension/presentation/styles/background.hpp>	// Border
#include <ascension/presentation/styles/color.hpp>
#include <boost/operators.hpp>
#include <boost/variant.hpp>

namespace ascension {
	namespace presentation {
		namespace styles {
			/**
			 * @defgroup css_text_decor_3 CSS Text Decoration Module Level 3
			 * @see CSS Text Decoration Module Level 3 - W3C Candidate Recommendation 1 August 2013
			 *      (http://www.w3.org/TR/css-text-decor-3/)
			 * @{
			 */
			/// Enumerated values for @c TextDecorationLine.
			ASCENSION_SCOPED_ENUM_DECLARE_BEGIN(TextDecorationLineEnums)
				NONE = 0,				///< Neither produces nor inhibits text decoration.
				UNDERLINE = 1 << 0,		///< Each line of text is underlined.
				OVERLINE = 1 << 1,		///< Each line of text has a line above it.
//				BASELINE = 1 << 2,
				LINE_THROUGH = 1 << 3	///< Each line of text has a line through the middle.
			ASCENSION_SCOPED_ENUM_DECLARE_END(TextDecorationLineEnums)

			/**
			 * [Copied from CSS3] Specifies what line decorations, if any, are added to the element.
			 * @see CSS Text Decoration Module Level 3, 2.1. Text Decoration Lines: the ‘text-decoration-line’ property
			 *      (http://dev.w3.org/csswg/css-text-decor-3/#text-decoration-line-property)
			 * @see SVG 1.1 (Second Edition), 10.12 Text decoration
			 *      (http://www.w3.org/TR/2011/REC-SVG11-20110816/text.html#TextDecorationProperties)
			 * @see XSL 1.1, 7.17.4 "text-decoration" (http://www.w3.org/TR/xsl/#text-decoration)
			 */
			typedef StyleProperty<
				Enumerated<BOOST_SCOPED_ENUM_NATIVE(TextDecorationLineEnums), TextDecorationLineEnums::NONE>,
				Inherited<false>
			> TextDecorationLine;

			/// A tag type for @c TextDecorationColor.
			struct TextDecorationColorSpec : BasicColorSpec<Inherited<false>> {};

			/**
			 * [Copied from CSS3] This property specifies the color of text decoration (underlines, overlines, and
			 * line-throughs) set on the element with ‘text-decoration-line’
			 * @see CSS Text Decoration Module Level 3, 2.2. Text Decoration Color: the ‘text-decoration-color’
			 *      property (http://www.w3.org/TR/css-text-decor-3/#text-decoration-color-property)
			 */
			typedef TypedColor<TextDecorationColorSpec> TextDecorationColor;

			/// Enumerated values for @c TextDecorationStyle.
			ASCENSION_SCOPED_ENUM_DECLARE_BEGIN(TextDecorationStyleEnums)
				SOLID = BorderStyleEnums::SOLID,	///< Same meaning as for @c Border#Style.
				DOUBLE = BorderStyleEnums::DOUBLE,	///< Same meaning as for @c Border#Style.
				DOTTED = BorderStyleEnums::DOTTED,	///< Same meaning as for @c Border#Style.
				DAHSED = BorderStyleEnums::DASHED,	///< Same meaning as for @c Border#Style.
				WAVY = static_cast<int>(BorderStyleEnums::OUTSET) + 1	///< A wavy line.
			ASCENSION_SCOPED_ENUM_DECLARE_END(TextDecorationStyleEnums)

			/**
			 * [Copied from CSS3] This property specifies the style of the line(s) drawn for text decoration specified
			 * on the element. 
			 * @see CSS Text Decoration Module Level 3, 2.3. Text Decoration Style: the ‘text-decoration-style’
			 *      property (http://www.w3.org/TR/css-text-decor-3/#text-decoration-style-property)
			 */
			typedef StyleProperty<
				Enumerated<BOOST_SCOPED_ENUM_NATIVE(TextDecorationStyleEnums), TextDecorationStyleEnums::SOLID>,
				Inherited<false>
			> TextDecorationStyle;

			/// Enumerated values for @c TextDecorationSkip. The documentation of the members are copied from CSS 3.
			ASCENSION_SCOPED_ENUM_DECLARE_BEGIN(TextDecorationSkipEnums)
				/// Skip nothing: text-decoration is drawn for all text content and for inline replaced elements.
				NONE = 0,
				/// Skip this element if it is an atomic inline (such as an image or inline-block).
				OBJECTS = 1 << 0,
				/// Skip white space: this includes regular spaces (U+0020) and tabs (U+0009), as well as nbsp
				/// (U+00A0), ideographic space (U+3000), all fixed width spaces (such as U+2000–U+200A, U+202F and
				/// U+205F), and any adjacent letter-spacing or word-spacing.
				SPACES = 1 << 2,
				/// Skip over where glyphs are drawn: interrupt the decoration line to let text show through where the
				/// text decoration would otherwise cross over a glyph. The UA may also skip a small distance to either
				/// side of the glyph outline.
				INK = 1 << 3,
				/// The UA should place the start and end of the line inwards from the content edge of the decorating
				/// element so that, e.g. two underlined elements side-by-side do not appear to have a single
				/// underline. (This is important in Chinese, where underlining is a form of punctuation.)
				EDGES = 1 << 4,
				/// Skip over the box's margin, border, and padding areas. Note that this only has an effect on
				/// decorations imposed by an ancestor.
				BOX_DECORATION = 1 << 5
			ASCENSION_SCOPED_ENUM_DECLARE_END(TextDecorationSkipEnums)

			/**
			 * [Copied from CSS3] This property specifies what parts of the element's content any text decoration
			 * affecting the element must skip over. It controls all text decoration lines drawn by the element and
			 * also any text decoration lines drawn by its ancestors.
			 * @see CSS Text Decoration Module Level 3, 2.5. Text Decoration Line Continuity: the
			 *      ‘text-decoration-skip’ property
			 *      (http://dev.w3.org/csswg/css-text-decor-3/#text-decoration-skip-property)
			 */
			typedef StyleProperty<
				Enumerated<BOOST_SCOPED_ENUM_NATIVE(TextDecorationSkipEnums), TextDecorationSkipEnums::OBJECTS>,
				Inherited<true>
			> TextDecorationSkip;

			/// Enumerated values for @c TextUnderlinePosition. The documentation of the members are copied from CSS 3.
			ASCENSION_SCOPED_ENUM_DECLARE_BEGIN(TextUnderlinePositionEnums)
				/// The user agent may use any algorithm to determine the underline's position; however it must be
				/// placed at or under the alphabetic baseline.
				AUTO = 0,
//				/// The underline is positioned relative to the alphabetic baseline. In this case the underline is
//				/// likely to cross some descenders.
//				ALPHABETIC,
				/// The underline is positioned under the element's text content. In this case the underline usually
				/// does not cross the descenders. (This is sometimes called "accounting" underline.)
				UNDER,
				/// In vertical writing modes, the underline is aligned as for ‘under’ except it is always aligned to
				/// the left edge of the text. If this causes the underline to be drawn on the "over" side of the text,
				/// then an overline also switches sides and is drawn on the "under" side.
				UNDER_LEFT,
				/// In vertical writing modes, the underline is aligned as for ‘under’ except it is always aligned to
				/// the right edge of the text. If this causes the underline to be drawn on the "over" side of the
				/// text, then an overline also switches sides and is drawn on the "under" side.
				UNDER_RIGHT
			ASCENSION_SCOPED_ENUM_DECLARE_END(TextUnderlinePositionEnums)

			/**
			 * [Copied from CSS3] This property sets the position of an underline specified on the same element. (It
			 * does not affect underlines specified by ancestor elements.)
			 * @see CSS Text Decoration Module Level 3, 2.6. Text Underline Position: the
			 *      ‘text-underline-position’ property
			 *      (http://www.w3.org/TR/css-text-decor-3/#text-underline-position-property)
			 */
			typedef StyleProperty<
				Enumerated<BOOST_SCOPED_ENUM_NATIVE(TextUnderlinePositionEnums), TextUnderlinePositionEnums::AUTO>,
				Inherited<true>
			> TextUnderlinePosition;

			/**
			 * Enumerated values for @c TextEmphasisStyle. The documentation of the members are copied from CSS 3 and
			 * modified. Prefix @c FILLED_ means the shape is filled with solid color, @c OPEN_ means the shape is
			 * hollow.
			 * @note This does not define a value for 'none' as enumerated one. Use @c boost#none for "no emphasis
			 *       mark" meaning.
			 */
			ASCENSION_SCOPED_ENUM_DECLARE_BEGIN(TextEmphasisStyleEnums)
//				/// No emphasis marks.
//				NONE = 0,
				/// Display small circles as marks. The filled dot is U+2022 ‘•’.
				FILLED_DOT = 0x2022u,
				/// Display small circles as marks. The open dot is U+25E6 ‘◦’.
				OPEN_DOT = 0x25e6u,
				/// Display large circles as marks. The filled circle is U+25CF ‘●’.
				FILLED_CIRCLE = 0x25cfu,
				/// Display large circles as marks. The open circle is U+25CB ‘○’.
				OPEN_CIRCLE = 0x25cbu,
				/// Display double circles as marks. The filled double-circle is U+25C9 ‘◉’.
				FILLED_DOUBLE_CIRCLE = 0x25c9u,
				/// Display double circles as marks. The open double-circle is U+25CE ‘◎’.
				OPEN_DOUBLE_CIRCLE = 0x25ceu,
				/// Display triangles as marks. The filled triangle is U+25B2 ‘▲’.
				FILLED_TRIANGLE = 0x25b2u,
				/// Display triangles as marks. The open triangle is U+25B3 ‘△’.
				OPEN_TRIANGLE = 0x25b3u,
				/// Display sesames as marks. The filled sesame is U+FE45 ‘﹅’.
				FILLED_SESAME = 0xfe45u,
				/// Display sesames as marks. The open sesame is U+FE46 ‘﹆’.
				OPEN_SESAME = 0xfe46u
			ASCENSION_SCOPED_ENUM_DECLARE_END(TextEmphasisStyleEnums)

			/**
			 * [Copied from CSS3] This property applies emphasis marks to the element's text.
			 * @c std#tuple&lt;&gt; means 'none' keyword.
			 * @see	CSS Text Decoration Module Level 3, 3.1. Emphasis Mark Style: the
			 *      ‘text-emphasis-style’ property
			 *      (http://www.w3.org/TR/css-text-decor-3/#text-emphasis-style-property)
			 */
			typedef StyleProperty<
				MultipleWithInitialIndex<
					boost::variant<std::tuple<>, BOOST_SCOPED_ENUM_NATIVE(TextEmphasisStyleEnums), CodePoint>,
					boost::mpl::int_<0>
				>,
				Inherited<true>,
				boost::optional<CodePoint>
			> TextEmphasisStyle;

			template<>
			inline typename SpecifiedValue<TextEmphasisStyle>::type uncompute<TextEmphasisStyle>(const typename ComputedValue<TextEmphasisStyle>::type& computedValue) {
				if(computedValue == boost::none)
					return std::make_tuple();
				else
					return boost::get(computedValue);
			}

			/// A tag type for @c TextEmphasisColor.
			struct TextEmphasisColorSpec : BasicColorSpec<Inherited<true>> {};

			/**
			 * [Copied from CSS3] This property specifies the foreground color of the emphasis marks.
			 * @see CSS Text Decoration Module Level 3, 3.2. Emphasis Mark Color: the
			 *      ‘text-emphasis-color’ property
			 *      (http://www.w3.org/TR/css-text-decor-3/#text-decoration-color-property)
			 */
			typedef TypedColor<TextEmphasisColorSpec> TextEmphasisColor;

			/// Enumerated values for @c TextEmphasisPosition.
			ASCENSION_SCOPED_ENUM_DECLARE_BEGIN(TextEmphasisPositionEnums)
				OVER = 0,	///< Draw marks over the text in horizontal writing mode.
				UNDER = 1,	///< Draw marks under the text in horizontal writing mode.
				RIGHT = 0,	///< Draw marks to the right of the text in vertical writing mode.
				LEFT = 2	///< Draw marks to the left of the text in vertical writing mode.
			ASCENSION_SCOPED_ENUM_DECLARE_END(TextEmphasisPositionEnums)

			/**
			 * [Copied from CSS3] This property describes where emphasis marks are drawn at.
			 * @see CSS Text Decoration Module Level 3, 3.4. Emphasis Mark Position: the
			 *      ‘text-emphasis-position’ property
			 *      (http://www.w3.org/TR/css-text-decor-3/#text-emphasis-position-property)
			 */
			typedef StyleProperty<
				Enumerated<
					BOOST_SCOPED_ENUM_NATIVE(TextEmphasisPositionEnums),
					static_cast<BOOST_SCOPED_ENUM_NATIVE(TextEmphasisPositionEnums)>(static_cast<int>(TextEmphasisPositionEnums::OVER) | static_cast<int>(TextEmphasisPositionEnums::RIGHT))
				>,
				Inherited<true>
			> TextEmphasisPosition;
#if 0
			/**
			 * @see CSS Text Decoration Module Level 3, 4. Text Shadows: the ‘text-shadow’ property
			 *      (http://www.w3.org/TR/css-text-decor-3/#text-shadow-property)
			 */
			struct TextShadow {};
#endif
			/// @}
		}
	}
}

#endif // !ASCENSION_STYLES_TEXT_DECOR_HPP
