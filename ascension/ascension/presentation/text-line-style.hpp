/**
 * @file text-style.hpp
 * @author exeal
 * @note This does not define @c TextLineStyle structure.
 * @see text-style.hpp
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2011 was presentation.hpp
 * @date 2011-05-04 separated from presentation.hpp
 * @date 2012-2012 was text-style.hpp
 * @date 2012-05-21 separated from text-style.hpp
 */

#ifndef ASCENSION_TEXT_LINE_STYLE_HPP
#define ASCENSION_TEXT_LINE_STYLE_HPP

#include <ascension/presentation/inheritable.hpp>
#include <ascension/presentation/length.hpp>
#include <ascension/presentation/writing-mode.hpp>

namespace ascension {
	namespace presentation {
		/**
		 * Dominant baselines from XSL 1.1, 7.14.5 "dominant-baseline".
		 * @see "CSS3 module: line, 4.4. Dominant baseline: the 'dominant-baseline' property"
		 *      (http://www.w3.org/TR/css3-linebox/#dominant-baseline-prop)
		 * @see "SVG 1.1 (Second Edition), 10.9.2 Baseline alignment properties"
		 *      (http://www.w3.org/TR/SVG/text.html#DominantBaselineProperty)
		 */
		enum DominantBaseline {
			DOMINANT_BASELINE_AUTO,
			DOMINANT_BASELINE_USE_SCRIPT,
			DOMINANT_BASELINE_NO_CHANGE,
			DOMINANT_BASELINE_RESET_SIZE,
			DOMINANT_BASELINE_IDEOGRAPHIC,
			DOMINANT_BASELINE_ALPHABETIC,
			DOMINANT_BASELINE_HANGING,
			DOMINANT_BASELINE_MATHEMATICAL,
			DOMINANT_BASELINE_CENTRAL,
			DOMINANT_BASELINE_MIDDLE,
			DOMINANT_BASELINE_TEXT_AFTER_EDGE,
			DOMINANT_BASELINE_TEXT_DEFORE_EDGE,
		};

		/**
		 * Alignment baseline from XSL 1.1, 7.14.2 "alignment-baseline".
		 * @see "CSS3 module: line, 4.5. Aligning the alignment point of an element: the
		 *      'alignment-baseline' property"
		 *      (http://www.w3.org/TR/css3-linebox/#alignment-baseline-prop)
		 * @see "SVG 1.1 (Second Edition), 10.9.2 Baseline alignment properties"
		 *      (http://www.w3.org/TR/SVG/text.html#AlignmentBaselineProperty)
		 */
		enum AlignmentBaseline {
			ALIGNMENT_BASELINE_BASELINE,
			ALIGNMENT_BASELINE_USE_SCRIPT,
			ALIGNMENT_BASELINE_BEFORE_EDGE,
			ALIGNMENT_BASELINE_TEXT_BEFORE_EDGE,
			ALIGNMENT_BASELINE_AFTER_EDGE,
			ALIGNMENT_BASELINE_TEXT_AFTER_EDGE,
			ALIGNMENT_BASELINE_CENTRAL,
			ALIGNMENT_BASELINE_MIDDLE,
			ALIGNMENT_BASELINE_IDEOGRAPHIC,
			ALIGNMENT_BASELINE_ALPHABETIC,
			ALIGNMENT_BASELINE_HANGING,
			ALIGNMENT_BASELINE_MATHEMATICAL
		};
#if 0
		enum AlignmentAdjustEnums {
			ALIGNMENT_ADJUST_AUTO,
			ALIGNMENT_ADJUST_BASELINE,
			ALIGNMENT_ADJUST_BEFORE_EDGE,
			ALIGNMENT_ADJUST_TEXT_BEFORE_EDGE,
			ALIGNMENT_ADJUST_MIDDLE,
			ALIGNMENT_ADJUST_CENTRAL,
			ALIGNMENT_ADJUST_AFTER_EDGE,
			ALIGNMENT_ADJUST_TEXT_AFTER_EDGE,
			ALIGNMENT_ADJUST_IDEOGRAPHIC,
			ALIGNMENT_ADJUST_ALPHABETIC,
			ALIGNMENT_ADJUST_HANGING,
			ALIGNMENT_ADJUST_MATHEMATICAL
		};

		/**
		 * @see "SVG 1.1 (Second Edition), 10.9.2 Baseline alignment properties"
		 *      (http://www.w3.org/TR/SVG/text.html#BaselineShiftProperty)
		 */
		enum BaselineShiftEnums {
			BASELINE_SHIFT_BASELINE,
			BASELINE_SHIFT_SUB,
			BASELINE_SHIFT_SUPER
		};
#endif
		/**
		 * @see CSS Text Level 3 - 4.1. Line Breaking Strictness: the 'line-break' property
		 *      (http://www.w3.org/TR/css3-text/#line-break)
		 */
		enum LineBreak {
			LINE_BREAK_AUTO,
			LINE_BREAK_LOOSE,
			LINE_BREAK_NORMAL,
			LINE_BREAK_STRICT
		};

		/**
		 * @see CSS Text Level 3 - 4.2. Word Breaking Rules: the 'word-break' property
		 *      (http://www.w3.org/TR/css3-text/#word-break)
		 */
		enum WordBreak {
			WORD_BREAK_NORMAL,
			WORD_BREAK_KEEP_ALL,
			WORD_BREAK_BREAK_ALL
		};

		/**
		 * @see CSS Text Level 3 - 6.1. Text Wrap Settings: 'text-wrap' property
		 *      (http://www.w3.org/TR/css3-text/#text-wrap)
		 */
		enum TextWrap {
			TEXT_WRAP_NORMAL,
			TEXT_WRAP_NONE,
			TEXT_WRAP_AVOID
		};

		/**
		 * @see CSS Text Level 3 - 6.2. Emergency Wrapping: the 'overflow-wrap' property
		 *      (http://www.w3.org/TR/css3-text/#overflow-wrap)
		 */
		enum OverflowWrap {
			OVERFLOW_WRAP_NORMAL = 0,
			OVERFLOW_WRAP_BREAK_WORD = 1,
			OVERFLOW_WRAP_HYPHENATE = 2
		};

		template<typename Measure, bool inheritable>
		struct TextWrappingBase {
			typename InheritableIf<inheritable, TextWrap>::Type textWrap;
			typename InheritableIf<inheritable, OverflowWrap>::Type overflowWrap;
			Measure measure;
			/// Default constructor.
			TextWrappingBase() : textWrap(TEXT_WRAP_NONE), overflowWrap(OVERFLOW_WRAP_NORMAL), measure(0) {}
		};

		template<typename Measure>
		struct TextWrapping : public TextWrappingBase<Measure, false> {};
		template<typename Measure>
		struct Inheritable<TextWrapping<Measure>> : public TextWrappingBase<Measure, true> {};

		/**
		 * @c TextAnchor describes an alignment of text relative to the given point.
		 * @see resolveTextAlignment, TextLineStyle#alignment, TextLineStyle#lastSublineAlignment,
		 *      defaultTextAnchor
		 * @see XSL 1.1, 7.16.9 "text-align"
		 *      (http://www.w3.org/TR/2006/REC-xsl11-20061205/#text-align)
		 * @see "CSS Text Level 3, 7.1. Text Alignment: the 'text-align' property"
		 *      (http://www.w3.org/TR/2010/WD-css3-text-20101005/#text-align)
		 * @see "SVG 1.1, 10.9.1 Text alignment properties"
		 *      (http://www.w3.org/TR/SVG/text.html#TextAlignmentProperties)
		 */
		enum TextAnchor {
			/// The text is aligned to the start edge of the paragraph.
			TEXT_ANCHOR_START,
			/// The text is aligned to the middle (center) of the paragraph.
			TEXT_ANCHOR_MIDDLE,
			/// The text is aligned to the end edge of the paragraph.
			TEXT_ANCHOR_END
			/// Inherits the parent's setting.
			/// @note Some methods which take @c TextAnchor don't accept this value.
//			MATCH_PARENT_DIRECTION
		};

		class Presentation;
		TextAnchor defaultTextAnchor(const Presentation& presentation);

		/**
		 * @c TextJustification describes the justification method.
		 * @note This definition is under construction.
		 * @see "CSS Text Level 3, 7.3. Justification Method: the 'text-justify' property"
		 *      (http://www.w3.org/TR/2011/WD-css3-text-20110901/#text-justify)
		 */
		enum TextJustification {
			/// Specifies no justification.
			AUTO_JUSTIFICATION, NONE_JUSTIFICATION, INTER_WORD, INTER_IDEOGRAPH, INTER_CLUSTER, DISTRIBUTE, KASHIDA
		};

		/**
		 * @see XSL 1.1, 7.16.5 "line-height-shift-adjustment"
		 *      (http://www.w3.org/TR/xsl/#line-height-shift-adjustment)
		 */
		enum LineHeightShiftAdjustment {
			CONSIDER_SHIFTS, DISREGARD_SHIFTS
		};

		/**
		 * [Copied from XSL 1.1 documentation] Selects the strategy for positioning adjacent lines,
		 * relative to each other.
		 * @see XSL 1.1, 7.16.6 "line-stacking-strategy"
		 *      (http://www.w3.org/TR/xsl/#line-stacking-strategy)
		 */
		enum LineStackingStrategy {
			/// [Copied from XSL 1.1 documentation] Uses the per-inline-height-rectangle.
			LINE_HEIGHT,
			/// [Copied from XSL 1.1 documentation] Uses the nominal-requested-line-rectangle.
			FONT_HEIGHT,
			/// [Copied from XSL 1.1 documentation] Uses the maximal-line-rectangle.
			MAX_HEIGHT
		};
	}

	namespace detail {
		enum PhysicalTextAnchor {LEFT, MIDDLE, RIGHT};

		inline PhysicalTextAnchor computePhysicalTextAnchor(presentation::TextAnchor anchor, presentation::ReadingDirection readingDirection) {
			switch(anchor) {
				case presentation::TEXT_ANCHOR_MIDDLE:
					return MIDDLE;
				case presentation::TEXT_ANCHOR_START:
					return (readingDirection == presentation::LEFT_TO_RIGHT) ? LEFT : RIGHT;
				case presentation::TEXT_ANCHOR_END:
					return (readingDirection == presentation::LEFT_TO_RIGHT) ? RIGHT : LEFT;
				default:
					ASCENSION_ASSERT_NOT_REACHED();
			}
		}
	}
} // namespace ascension.presentation

#endif // !ASCENSION_TEXT_LINE_STYLE_HPP
