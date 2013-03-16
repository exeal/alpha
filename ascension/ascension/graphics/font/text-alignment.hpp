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

			// from CSS Line Layout Module Level 3 ////////////////////////////////////////////////

			/**
			 * [Copied from CSS3] This property enumerates which aspects of the elements in a line
			 * box contribute to the height height of that line box.
			 * @see CSS Line Layout Module Level 3, 3.4.2 Line Stacking: the ‘line-box-contain’
			 *      property (http://dev.w3.org/csswg/css3-linebox/#LineStacking)
			 * @see XSL 1.1, 7.16.6 "line-stacking-strategy"
			 *      (http://www.w3.org/TR/xsl/#line-stacking-strategy)
			 */
			ASCENSION_SCOPED_ENUMS_BEGIN(LineBoxContain)
				// TODO: 'NONE' should be 0.
				// TODO: Values other than 'NONE' can be combined by bitwise-OR.
				BLOCK, INLINE, FONT, GLYPHS, REPLACED, INLINE_BOX, NONE
			ASCENSION_SCOPED_ENUMS_END;

			/**
			 * [Copied from CSS3] The ‘dominant-baseline’ property is used to determine or
			 * re-determine a scaled-baseline-table.
			 * @see CSS Line Layout Module Level 3, 4.4 Dominant baseline: the ‘dominant-baseline’
			 *      property (http://dev.w3.org/csswg/css3-linebox/#dominant-baseline-prop)
			 * @see CSS3 module: line, 4.4. Dominant baseline: the 'dominant-baseline' property
			 *      (http://www.w3.org/TR/css3-linebox/#dominant-baseline-prop)
			 * @see SVG 1.1 (Second Edition), 10.9.2 Baseline alignment properties
			 *      (http://www.w3.org/TR/SVG/text.html#DominantBaselineProperty)
			 * @see XSL 1.1, 7.14.5 "dominant-basline"
			 *      (http://www.w3.org/TR/xsl/#dominant-baseline)
			 */
			ASCENSION_SCOPED_ENUMS_BEGIN(DominantBaseline)
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
			ASCENSION_SCOPED_ENUMS_END;

			/**
			 * [Copied from CSS3] This property specifies how an inline-level element is aligned
			 * with respect to its parent. That is, to which of the parent's baselines the
			 * alignment point of this element is aligned. Unlike the ‘dominant-baseline’ property
			 * the ‘alignment-baseline’ property has no effect on its children dominant-baselines.
			 * @see CSS Line Layout Module Level 3, 4.5 Aligning the alignment point of an element:
			 *      the ‘alignment-baseline’ property
			 *      (http://dev.w3.org/csswg/css3-linebox/#alignment-baseline-prop)
			 * @see CSS3 module: line, 4.5. Aligning the alignment point of an element: the
			 *      'alignment-baseline' property
			 *      (http://www.w3.org/TR/css3-linebox/#alignment-baseline-prop)
			 * @see SVG 1.1 (Second Edition), 10.9.2 Baseline alignment properties
			 *      (http://www.w3.org/TR/SVG/text.html#AlignmentBaselineProperty)
			 * @see XSL 1.1, 7.14.2 "alignment-baseline"
			 *      (http://www.w3.org/TR/xsl/#alignment-baseline)
			 */
			ASCENSION_SCOPED_ENUMS_BEGIN(AlignmentBaseline)
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
			ASCENSION_SCOPED_ENUMS_END;

			// from CSS Text Level 3 //////////////////////////////////////////////////////////////

			/**
			 * @c TextAnchor describes an alignment of text relative to the given point.
			 * @see resolveTextAlignment, TextLineStyle#alignment,
			 *      TextLineStyle#lastSublineAlignment, defaultTextAnchor
			 * @see CSS Text Level 3, 7.1. Text Alignment: the ‘text-align’ property
			 *      (http://www.w3.org/TR/css3-text/#text-align)
			 * @see XSL 1.1, 7.16.9 "text-align"
			 *      (http://www.w3.org/TR/2006/REC-xsl11-20061205/#text-align)
			 */
			ASCENSION_SCOPED_ENUMS_BEGIN(TextAlignment)
				START,
				END,
				LEFT,
				RIGHT,
				CENTER,
				JUSTIFY,
				MATCH_PARENT,
				START_END
			ASCENSION_SCOPED_ENUMS_END;
	
			/**
			 * [Copied from SVG11] The ‘text-anchor’ property is used to align (start-, middle- or
			 * end-alignment) a string of text relative to a given point.
			 * @see SVG 1.1, 10.9.1 Text alignment properties
			 *      (http://www.w3.org/TR/SVG/text.html#TextAlignmentProperties)
			 */
			ASCENSION_SCOPED_ENUMS_BEGIN(TextAnchor)
				START = TextAlignment::START,
				MIDDLE = TextAlignment::CENTER,
				END = TextAlignment::END
			ASCENSION_SCOPED_ENUMS_END;

			// TODO: Other types defined in presentation/text-style.hpp should move to here???

		}
	}
}

#endif // !ASCENSION_TEXT_ALIGNMENT_HPP
