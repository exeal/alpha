﻿/**
 * @file inline.hpp
 * @author exeal
 * @see presentation.hpp, graphics/text-alignment.hpp, graphics/text-layout-styles.hpp
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2011 was presentation.hpp
 * @date 2011-05-04 separated from presentation.hpp
 * @date 2012-07-16 reunioned with text-line-style.hpp
 * @date 2014-09-23 Separated from text-style.hpp
 */

#ifndef ASCENSION_STYLES_INLINE_HPP
#define ASCENSION_STYLES_INLINE_HPP

#include <ascension/corelib/future/scoped-enum-emulation.hpp>
#include <ascension/graphics/font/text-alignment.hpp>
#include <ascension/presentation/length.hpp>
#include <ascension/presentation/style-property.hpp>
#include <boost/variant.hpp>

namespace ascension {
	namespace presentation {
		namespace styles {
			/// @defgroup css3_inline CSS Inline Layout Module Level 3
			/// @see CSS Inline Layout Module Level 3 - Editor's Draft, 5 September 2014
			/// @{

			/// Enumerated values for @c TextHeight. The documentation of the members are copied from CSS 3.
			ASCENSION_SCOPED_ENUMS_BEGIN(TextHeightEnums)
				/// The block-progression dimension is based either on the em square determined by the computed element
				/// font-size property value or the cell-height (ascender + descender) related to the computed element
				/// font-size as chosen by the user agent.
				AUTO,
				/// The block-progression dimension is based on the em square as determined by the computed element
				/// font-size.
				FONT_SIZE,
				/// The block-progression dimension is based on the cell-height (ascender + descender) related to the
				/// computed element font-size.
				TEXT_SIZE,
				/// The block-progression dimension is based on the maximum extents toward the over-edge and under-edge
				/// of the box obtained by considering all children elements located on the same line, ruby annotations
				/// (elements with 'display:ruby-text') and baseline shifted elements.
				MAX_SIZE
			ASCENSION_SCOPED_ENUMS_END;

			/**
			 * [Copied from CSS3] The text-height property determine the block-progression dimension of the text
			 * content area of an inline box (non-replaced elements).
			 * @see CSS Inline Layout Module Level 3, 1.3 Block-progression dimensions: the text-height property
			 *      (http://dev.w3.org/csswg/css-inline/#inline1)
			 */
			typedef StyleProperty<
				Multiple<
					boost::variant<TextHeightEnums, graphics::Scalar>,
					TextHeightEnums, TextHeightEnums::AUTO
				>, Inherited<true>
			> TextHeight;

			/// Enumerated values for @c LineHeight. The documentation of the members are copied from CSS 3.
			ASCENSION_SCOPED_ENUMS_BEGIN(LineHeightEnums)
				/// Tells user agents to set the computed value to a "reasonable" value based on the font size of the
				/// element. The value has the same meaning as &lt;number&gt;. We recommend a computed value for normal
				/// between 1.0 to 1.2. The user agent may allow the &lt;number&gt; to vary depending on the metrics of
				/// the font(s) being used.
				NORMAL,
				/// For inline-level elements, the element does not influence the extended block-progression dimension
				/// of the line. The computed value is the specified value (none). For block-level elements, equivalent
				/// to normal (and the computed value is normal).
				NONE
			ASCENSION_SCOPED_ENUMS_END;

			/**
			 * [Copied from CSS3] The line-height property controls the amount of leading space which is added over and
			 * under an inline box (including the root inline box) to determine the extended block-progression
			 * dimension of the inline box.
			 * @see CSS Inline Layout Module Level 3, 1.4.1 Line height adjustment: the line-height property
			 *      (http://dev.w3.org/csswg/css-inline/#InlineBoxHeight)
			 * @see XSL 1.1, 7.16.4 "line-height" (http://www.w3.org/TR/xsl/#line-height)
			 */
			typedef StyleProperty<
				Multiple<
					boost::variant<LineHeightEnums, graphics::Scalar, Length>,
					LineHeightEnums, LineHeightEnums::NORMAL
				>,
				Inherited<true>,
				boost::optional<graphics::Scalar>
			> LineHeight;

			/// @see graphics#font#LineBoxContain
			typedef StyleProperty<
				Enumerated<
					graphics::font::LineBoxContain,
					graphics::font::LineBoxContain::BLOCK | graphics::font::LineBoxContain::INLINE | graphics::font::LineBoxContain::REPLACED
				>, Inherited<true>
			> LineBoxContain;

			/// @see graphics#font#DominantBaseline
			typedef StyleProperty<
				Enumerated<graphics::font::DominantBaseline, graphics::font::DominantBaseline::AUTO>,
				Inherited<false>
			> DominantBaseline;

			/// @see graphics#font#AlignmentBaseline
			typedef StyleProperty<
				Enumerated<graphics::font::AlignmentBaseline, graphics::font::AlignmentBaseline::BASELINE>,
				Inherited<false>
			> AlignmentBaseline;

			/// Enumerated values for @c AlignmentAdjust. The documentation of the members are copied from CSS 3.
			ASCENSION_SCOPED_ENUMS_BEGIN(AlignmentAdjustEnums)
				// TODO: Describe the values.
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

			/**
			 * [Copied from CSS3] The ‘alignment-adjust’ property allows more precise alignment of elements, such as
			 * graphics, that do not have a baseline-table or lack the desired baseline in their baseline-table.
			 * @see CSS Inline Layout Module Level 3, 2.6 Setting the alignment point: the ‘alignment-adjust’ property
			 *      (http://dev.w3.org/csswg/css-inline/#alignment-adjust-prop)
			 * @see XSL 1.1, 7.14.1 "alignment-adjust" (http://www.w3.org/TR/xsl/#alignment-adjust)
			 */
			typedef StyleProperty<
				Multiple<
					boost::variant<AlignmentAdjustEnums, Length>,
					AlignmentAdjustEnums, AlignmentAdjustEnums::AUTO
				>, Inherited<false>
			> AlignmentAdjust;

			/// Enumerated values for @c BaselineShift. The documentation of the members are copied from CSS 3.
			ASCENSION_SCOPED_ENUMS_BEGIN(BaselineShiftEnums)
				// TODO: Describe the values.
				BASELINE,
				SUB,
				SUPER
			ASCENSION_SCOPED_ENUMS_END;

			/**
			 * [Copied from CSS3] The ‘baseline-shift’ property allows repositioning of the dominant-baseline relative
			 * to the dominant-baseline.
			 * @see CSS Inline Layout Module Level 3, 2.7 Repositioning the dominant baseline: the ‘baseline-shift’
			 *      property (http://dev.w3.org/csswg/css-inline/#baseline-shift-prop)
			 * @see SVG 1.1 (Second Edition), 10.9.2 Baseline alignment properties
			 *      (http://www.w3.org/TR/SVG/text.html#BaselineShiftProperty)
			 * @see XSL 1.1, 7.14.3 "baseline-shift" (http://www.w3.org/TR/xsl/#baseline-shift)
			 */
			typedef StyleProperty<
				Multiple<
					boost::variant<BaselineShiftEnums, Length>,
					BaselineShiftEnums, BaselineShiftEnums::BASELINE
				>, Inherited<false>
			> BaselineShift;

			/// Enumerated values for @c InlineBoxAlignment. The documentation of the members are copied from CSS 3.
			ASCENSION_SCOPED_ENUMS_BEGIN(InlineBoxAlignmentEnums)
				/// Use the initial line of the inline block element for alignment purpose.
				INITIAL,
				/// Use the last line of the inline block element for alignment purpose.
				LAST
			ASCENSION_SCOPED_ENUMS_END;

			/**
			 * [Copied from CSS3] The ‘inline-box-align’ property determines which line of a multi-line inline block
			 * aligns with the previous and next inline elements within a line.
			 * @see CSS Inline Layout Module Level 3, 2.9 Inline box alignment: the ‘inline-box-align’ property
			 *      (http://dev.w3.org/csswg/css-inline/#inline-box-align-prop)
			 */
			typedef StyleProperty<
				Multiple<
					boost::variant<InlineBoxAlignmentEnums, Index>,
					InlineBoxAlignmentEnums, InlineBoxAlignmentEnums::LAST
				>, Inherited<false>
			> InlineBoxAlignment;
			/// @}
		}
	}
}

#endif // !ASCENSION_STYLES_INLINE_HPP
