/**
 * @file text-alignment.hpp
 * @author exeal
 * @see text-layout-styles.hpp, presentation/text-style.hpp
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2011 was presentation/presentation.hpp
 * @date 2011-05-04 separated from presentation/presentation.hpp
 * @date 2012-07-16 reunioned with presentation/text-line-style.hpp
 * @date 2012-08-17 separated from presentation/text-style.hpp
 */

#ifndef ASCENSION_TEXT_ALIGNMENT_HPP
#define ASCENSION_TEXT_ALIGNMENT_HPP
#include <ascension/corelib/future/scoped-enum-emulation.hpp>

namespace ascension {
	namespace graphics {
		namespace font {
			/// @addtogroup css_inline_3
			/// @{
			/**
			 * [Copied from CSS3] This property enumerates which aspects of the elements in a line box contribute to
			 * the height height of that line box.
			 * @see CSS Inline Layout Module Level 3, 1.4.2 Line Stacking: the line-box-contain property
			 *      (http://dev.w3.org/csswg/css-inline/#LineStacking)
			 * @see XSL 1.1, 7.16.6 "line-stacking-strategy" (http://www.w3.org/TR/xsl/#line-stacking-strategy)
			 */
			ASCENSION_SCOPED_ENUM_DECLARE_BEGIN(LineBoxContain)
				// TODO: 'NONE' should be 0.
				// TODO: Values other than 'NONE' can be combined by bitwise-OR.
				/// The extended block progression dimension of the root inline box must fit within the line box.
				BLOCK,
				/// The extended block progression dimension of all non-replaced inline boxes whose line-height is not
				/// none in the line box must fit within the line box.
				INLINE,
				/// The block progression dimension of all non-replaced inline boxes in the line whose line-height is
				/// not none that directly (i.e., within the box but not within one of its descendants) contain
				/// non-removed text must fit within the line box, where non-removed text is any characters not removed
				/// based on the values of the white-space.
				FONT,
				/// The block progression dimension of all the glyph bounding boxes of glyphs in the line box must fit
				/// within the line box.
				GLYPHS,
				/// The margin box of all replaced elements within the line must fit within the line box.
				REPLACED,
				/// The margin-box of all non-replaced inline elements in the line whose line-height is not none must
				/// fit within the line box.
				INLINE_BOX,
				/// 
				NONE
			ASCENSION_SCOPED_ENUM_DECLARE_END(LineBoxContain)

			/**
			 * [Copied from CSS3] The dominant-baseline property is used to determine or re-determine a
			 * scaled-baseline-table.
			 * @see CSS Inline Layout Module Level 3, 2.4 Dominant baseline: the ‘dominant-baseline’ property
			 *      (http://dev.w3.org/csswg/css-inline/#dominant-baseline-prop)
			 * @see SVG 1.1 (Second Edition), 10.9.2 Baseline alignment properties
			 *      (http://www.w3.org/TR/SVG/text.html#DominantBaselineProperty)
			 * @see XSL 1.1, 7.14.5 "dominant-basline" (http://www.w3.org/TR/xsl/#dominant-baseline)
			 */
			ASCENSION_SCOPED_ENUM_DECLARE_BEGIN(DominantBaseline)
				/// If this property occurs on a block or inline-block element, then the user agent behavior depends on
				/// the value of the ‘text-script’ property.
				AUTO,
				/// The dominant baseline-identifier is set using the computed value of the ‘text-script’ property.
				USE_SCRIPT,
				/// The dominant baseline-identifier, the baseline-table and the baseline-table font-size remain the
				/// same as that of the parent.
				NO_CHANGE,
				/// The dominant baseline-identifier and the baseline table remain the same, but the baseline-table
				/// ‘font-size’ is changed to the value of the font-size property on this element.
				RESET_SIZE,
				/// The dominant baseline-identifier is set to the ‘alphabetic’ baseline, the derived baseline-table is
				/// constructed using the ‘alphabetic’ baseline-table in the nominal font, and the baseline-table
				/// ‘font-size’ is changed to the value of the font-size property on this element. (The alphabetic
				/// baseline is the standard baseline for Roman scripts.)
				ALPHABETIC,
				/// The dominant baseline-identifier is set to the ‘hanging’ baseline, the derived baseline-table is
				/// constructed using the ‘hanging’ baseline-table in the nominal font, and the baseline-table
				/// ‘font-size’ is changed to the value of the font-size property on this element.
				HANGING,
				/// The dominant baseline-identifier is set to the ‘ideographic’ baseline, the derived baseline-table
				/// is constructed using the ‘ideographic’ baseline-table in the nominal font, and the baseline-table
				/// ‘font-size’ is changed to the value of the font-size property on this element.
				IDEOGRAPHIC,
				/// The dominant baseline-identifier is set to the ‘mathematical’ baseline, the derived baseline-table
				/// is constructed using the ‘mathematical’ baseline-table in the nominal font, and the baseline-table
				/// ‘font-size’ is changed to the value of the font-size property on this element.
				MATHEMATICAL,
				/// The dominant baseline-identifier is set to be ‘central’.
				CENTRAL,
				/// The dominant baseline-identifier is set to be ‘middle’.
				MIDDLE,
				/// The dominant baseline-identifier is set to be ‘text-under-edge’.
				TEXT_UNDER_EDGE,
				/// The dominant baseline-identifier is set to be ‘text-over-edge’.
				TEXT_OVER_EDGE,
			ASCENSION_SCOPED_ENUM_DECLARE_END(DominantBaseline)

			/**
			 * [Copied from CSS3] This property specifies how an inline-level element is aligned with respect to its
			 * parent. That is, to which of the parent’s baselines the alignment point of this element is aligned.
			 * @see CSS Inline Layout Module Level 3, 2.5 Aligning the alignment point of an element: the
			 *      ‘alignment-baseline’ property (http://dev.w3.org/csswg/css-inline/#alignment-baseline-prop)
			 * @see SVG 1.1 (Second Edition), 10.9.2 Baseline alignment properties
			 *      (http://www.w3.org/TR/SVG/text.html#AlignmentBaselineProperty)
			 * @see XSL 1.1, 7.14.2 "alignment-baseline" (http://www.w3.org/TR/xsl/#alignment-baseline)
			 */
			ASCENSION_SCOPED_ENUM_DECLARE_BEGIN(AlignmentBaseline)
				/// The alignment-point of the element being aligned is aligned with the dominant baseline of the
				/// parent.
				BASELINE,
				/// If the element ‘text-script’ property value is ‘auto’, the alignment point of each glyph is aligned
				/// with the parent baseline-identifier of the script to which the glyph belongs.
				USE_SCRIPT,
				/// The alignment point of the box is aligned with the over-edge baseline of the line box.
				OVER_EDGE,
				/// The alignment-point of the element being aligned is aligned with the ‘text-over-edge’ baseline of
				/// the parent.
				TEXT_OVER_EDGE,
				/// The alignment point of the box is aligned with the under-edge baseline of the line box.
				UNDER_EDGE,
				/// The alignment-point of the element being aligned is aligned with the ‘text-under-edge’ baseline of
				/// the parent.
				TEXT_UNDER_EDGE,
				/// The alignment point of the box is aligned with the ‘central’ baseline of the parent.
				CENTRAL,
				/// The alignment point of the box is aligned with the ‘middle’ baseline of the parent.
				MIDDLE,
				/// The alignment-point of the element being aligned is aligned with the ‘ideographic’ baseline of the
				/// parent.
				IDEOGRAPHIC,
				/// The alignment-point of the element being aligned is aligned with the alphabetic baseline of the
				/// parent.
				ALPHABETIC,
				/// The alignment-point of the element being aligned is aligned with the hanging baseline of the
				/// parent.
				HANGING,
				/// The alignment-point of the element being aligned is aligned with the mathematical baseline of the
				/// parent.
				MATHEMATICAL
			ASCENSION_SCOPED_ENUM_DECLARE_END(AlignmentBaseline)
			/// @}

			/// @addtogroup css_text_3
			/// @{
			/**
			 * [Copied from CSS3] This property describes how the inline-level content of a block is aligned along the
			 * inline axis if the content does not completely fill the line box.
			 * @see resolveTextAlignment, TextLineStyle#alignment, TextLineStyle#lastSublineAlignment, defaultTextAnchor
			 * @see CSS Text Module Level 3, 7.1. Text Alignment: the ‘text-align’ property
			 *      (http://www.w3.org/TR/css-text-3/#text-align-property)
			 * @see XSL 1.1, 7.16.9 "text-align"
			 *      (http://www.w3.org/TR/2006/REC-xsl11-20061205/#text-align)
			 */
			ASCENSION_SCOPED_ENUM_DECLARE_BEGIN(TextAlignment)
				/// Inline-level content is aligned to the start edge of the line box.
				START,
				/// Inline-level content is aligned to the end edge of the line box.
				END,
				/// Inline-level content is aligned to the line left edge of the line box. (In vertical writing modes,
				/// this will be either the physical top or bottom, depending on ‘text-orientation’.)
				LEFT,
				/// Inline-level content is aligned to the line right edge of the line box. (In vertical writing modes,
				/// this will be either the physical top or bottom, depending on ‘text-orientation’.)
				RIGHT,
				/// Inline-level content is centered within the line box.
				CENTER,
				/// Text is justified according to the method specified by the ‘text-justify’ property, in order to
				/// exactly fill the line box.
				JUSTIFY,
				/// This value behaves the same as ‘inherit’ (computes to its parent's computed value) except that an
				/// inherited ‘start’ or ‘end’ keyword is interpreted against its parent's ‘direction’ value and
				/// results in a computed value of either ‘left’ or ‘right’.
				MATCH_PARENT,
				/// Specifies ‘start’ alignment of the first line and any line immediately after a forced line break;
				/// and ‘end’ alignment of any remaining lines.
				START_END
			ASCENSION_SCOPED_ENUM_DECLARE_END(TextAlignment)

			/**
			 * [Copied from CSS3] This property selects the justification method used when a line's alignment is set to
			 * ‘justify’ (see ‘text-align’). The property applies to block containers, but the UA may (but is not
			 * required to) also support it on inline elements.
			 * @see CSS Text Module Level 3, 7.3. Justification Method: the ‘text-justify’ property
			 *      (http://www.w3.org/TR/css-text-3/#text-justify-property)
			 * @note The name of this type is @c TextJustification, not @c TextJustify.
			 */
			ASCENSION_SCOPED_ENUM_DECLARE_BEGIN(TextJustification)
				/// The UA determines the justification algorithm to follow, based on a balance between performance and
				/// adequate presentation quality.
				AUTO,
				/// Justification is disabled: there are no expansion opportunities within the text.
				NONE,
				/// Justification adjusts spacing at word separators only (effectively varying the used ‘word-spacing’
				/// on the line). This behavior is typical for languages that separate words using spaces, like English
				/// or Korean.
				INTER_WORD,
				/// Justification adjusts spacing between each pair of adjacent characters (effectively varying the
				/// used ‘letter-spacing’ on the line). This value is sometimes used in e.g. Japanese.
				DISTRIBUTE/*,
				INTER_IDEOGRAPH,
				INTER_CLUSTER,
				KASHIDA*/
			ASCENSION_SCOPED_ENUM_DECLARE_END(TextJustification)
			/// @}
	
			/**
			 * [Copied from SVG11] The ‘text-anchor’ property is used to align (start-, middle- or end-alignment) a
			 * string of text relative to a given point.
			 * @see SVG 1.1, 10.9.1 Text alignment properties
			 *      (http://www.w3.org/TR/SVG/text.html#TextAlignmentProperties)
			 */
			ASCENSION_SCOPED_ENUM_DECLARE_BEGIN(TextAnchor)
				START = TextAlignment::START,
				MIDDLE = TextAlignment::CENTER,
				END = TextAlignment::END
			ASCENSION_SCOPED_ENUM_DECLARE_END(TextAnchor)

			namespace detail {
				ASCENSION_SCOPED_ENUM_DECLARE_BEGIN(PhysicalTextAnchor)
					LEFT = TextAlignment::LEFT,
					CENTER = TextAlignment::CENTER,
					RIGHT = TextAlignment::RIGHT
				ASCENSION_SCOPED_ENUM_DECLARE_END(PhysicalTextAnchor)
#if 0
				inline PhysicalTextAnchor computePhysicalTextAnchor(
						presentation::styles::TextAnchor anchor, presentation::ReadingDirection readingDirection) {
					switch(boost::native_value(anchor)) {
						case presentation::styles::TextAnchor::MIDDLE:
							return PhysicalTextAnchor::CENTER;
						case presentation::styles::TextAnchor::START:
							return (readingDirection == presentation::LEFT_TO_RIGHT) ? PhysicalTextAnchor::LEFT : PhysicalTextAnchor::RIGHT;
						case presentation::styles::TextAnchor::END:
							return (readingDirection == presentation::LEFT_TO_RIGHT) ? PhysicalTextAnchor::RIGHT : PhysicalTextAnchor::LEFT;
						default:
							ASCENSION_ASSERT_NOT_REACHED();
					}
				}
#endif
			}
		}
	}
}

#endif // !ASCENSION_TEXT_ALIGNMENT_HPP
