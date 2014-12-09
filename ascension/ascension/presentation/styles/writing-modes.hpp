/**
 * @file writing-modes.hpp
 * @author exeal
 * @see writing-mode.hpp
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2011 was presentation.hpp
 * @date 2011-05-04 separated from presentation.hpp
 * @date 2012-07-16 reunioned with text-line-style.hpp
 * @date 2014-09-24 Separated from text-style.hpp
 */

#ifndef ASCENSION_STYLES_WRITING_MODES_HPP
#define ASCENSION_STYLES_WRITING_MODES_HPP

#include <ascension/presentation/style-property.hpp>
#include <ascension/presentation/writing-mode.hpp>

namespace ascension {
	namespace presentation {
		namespace styles {
			/// @defgroup css_writing_modes_3 CSS Writing Modes Level 3
			/// @see CSS Writing Modes Level 3 - W3C Candidate Recommendation, 20 March 2014
			///      (http://www.w3.org/TR/css-writing-modes-3/)
			/// @{

			/// @see ReadingDirection
			typedef StyleProperty<
				Enumerated<ReadingDirection, LEFT_TO_RIGHT>,
				Inherited<true>
			> Direction;

//			typedef StyleProperty<
//				Enumerated<UnicodeBidiEnums, UnicodeBidiEnums::NORMAL>,
//				Inherited<false>
//			> UnicodeBidi;

			/// @see BlockFlowDirection
			typedef StyleProperty<
				Enumerated<BlockFlowDirection, HORIZONTAL_TB>,
				Inherited<true>
			> WritingMode;

			/// @see TextOrientation
			typedef StyleProperty<
				Enumerated<TextOrientation, MIXED>,
				Inherited<true>
			> TextOrientation;

			/// @}
		}
	}
}

#endif	// !ASCENSION_STYLES_WRITING_MODES_HPP
