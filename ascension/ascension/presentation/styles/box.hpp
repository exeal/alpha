﻿/**
 * @file box.hpp
 * @author exeal
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2011 was presentation.hpp
 * @date 2011-05-04 separated from presentation.hpp
 * @date 2012-07-16 reunioned with text-line-style.hpp
 * @date 2014-09-21 Separated from text-style.hpp
 */

#ifndef ASCENSION_STYLES_BOX_HPP
#define ASCENSION_STYLES_BOX_HPP

#include <ascension/corelib/future/scoped-enum-emulation.hpp>
#include <ascension/presentation/absolute-length.hpp>
#include <ascension/presentation/style-property.hpp>
#include <ascension/presentation/styles/length.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/rational.hpp>
#include <boost/variant.hpp>

namespace ascension {
	namespace presentation {
		namespace styles {
			/// @defgroup css3_basic_box_model CSS basic box model
			/// @see CSS basic box model, Editor's Draft 12 October 2013 (http://dev.w3.org/csswg/css-box/)
			/// @{

			/// Enumerated values for @c PaddingSide.
			ASCENSION_SCOPED_ENUMS_BEGIN(PaddingEnums)
				AUTO
			ASCENSION_SCOPED_ENUMS_END;

			/**
			 * [Copied from CSS3] Sets the thickness of the padding area. The value may not be negative.
			 * @c std#tuple&lt;&gt; means 'auto' keyword.
			 * @see CSS basic box model, 7. The padding properties
			 *      (http://dev.w3.org/csswg/css3-box/#the-padding-properties)
			 */
			typedef StyleProperty<
				Multiple<
					boost::variant<Length, Percentage, BOOST_SCOPED_ENUM_NATIVE(PaddingEnums)>,
					Length, 0
				>,
				Inherited<false>,
				boost::variant<Pixels, Percentage, std::tuple<>>
			> PaddingSide;

			/// Enumerated values for @c MarginSide.
			ASCENSION_SCOPED_ENUMS_BEGIN(MarginEnums)
				/// Makes the margin depend on the available space, as defined in “Calculating widths, heights and
				/// margins” and in ...
				FILL,
				/// On the A edge and C edge, the used value of ‘auto’ is 0. On the B edge and D edge, the used value
				/// depends on the available space, as defined in “Calculating widths, heights and margins.”
				AUTO
			ASCENSION_SCOPED_ENUMS_END;

			/**
			 * [Copied from CSS3] These properties set the thickness of the margin area. The value may be negative.
			 * @c std#tuple&lt;&gt; means 'auto' keyword.
			 * @see CSS basic box model, 8. Margins (http://dev.w3.org/csswg/css3-box/#margins)
			 */
			typedef StyleProperty<
				Multiple<
					boost::variant<Length, Percentage, MarginEnums>,
					Length, 0
				>,
				Inherited<false>,
				boost::variant<Pixels, Percentage, std::tuple<>>
			> MarginSide;

			/**
			 * [Copied from CSS3] These properties specify the width and height of the content area or border area
			 * (depending on ‘box-sizing’) of certain boxes.
			 * @see CSS basic box model, 9. The width and height properties (http://dev.w3.org/csswg/css-box/#width)
			 */
			typedef StyleProperty<
				MultipleWithInitialIndex<
					boost::variant<Length, Percentage, std::tuple<>>,
					boost::mpl::int_<2>
				>,
				Inherited<false>,
				boost::variant<Pixels, Percentage, std::tuple<>>
			> Measure;
			/// @}
		}
	}
}

#endif // !ASCENSION_STYLES_BOX_HPP