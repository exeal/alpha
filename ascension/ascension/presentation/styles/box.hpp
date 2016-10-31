/**
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
#include <ascension/presentation/styles/percentage.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/rational.hpp>
#include <boost/variant.hpp>

namespace ascension {
	namespace presentation {
		namespace styles {
			/// @defgroup css_box_3 CSS basic box model
			/// @see CSS basic box model, Editor's Draft 12 October 2013 (http://dev.w3.org/csswg/css-box/)
			/// @{

			/// Enumerated values for @c PaddingSide.
			ASCENSION_SCOPED_ENUM_DECLARE_BEGIN(PaddingEnums)
				AUTO
			ASCENSION_SCOPED_ENUM_DECLARE_END(PaddingEnums)

			/**
			 * [Copied from CSS3] Sets the thickness of the padding area. The value may not be negative.
			 * @c std#tuple&lt;&gt; means 'auto' keyword.
			 * @see CSS basic box model, 7. The padding properties
			 *      (http://dev.w3.org/csswg/css-box-3/#the-padding-properties)
			 */
			typedef StyleProperty<
				Multiple<
					boost::variant<Length, Percentage, PaddingEnums>,
					Length
				>,
				Inherited<false>,
				boost::variant<Length, Percentage, std::tuple<>>
			> PaddingSide;

			template<>			
			inline SpecifiedValue<PaddingSide>::type uncompute<PaddingSide>(const ComputedValue<PaddingSide>::type& computedValue) {
				if(const Length* const length = boost::get<Length>(&computedValue))
					return *length;
				else if(const Percentage* const percentage = boost::get<Percentage>(&computedValue))
					return *percentage;
				else if(const std::tuple<>* const autoValue = boost::get<std::tuple<>>(&computedValue))
					return PaddingEnums(PaddingEnums::AUTO);
				else
					throw UnknownValueException("computedValue");
			}

			/// Enumerated values for @c MarginSide.
			ASCENSION_SCOPED_ENUM_DECLARE_BEGIN(MarginEnums)
				/// Makes the margin depend on the available space, as defined in “Calculating widths, heights and
				/// margins” and in ...
				FILL,
				/// On the A edge and C edge, the used value of ‘auto’ is 0. On the B edge and D edge, the used value
				/// depends on the available space, as defined in “Calculating widths, heights and margins.”
				AUTO
			ASCENSION_SCOPED_ENUM_DECLARE_END(MarginEnums)

			/**
			 * [Copied from CSS3] These properties set the thickness of the margin area. The value may be negative.
			 * @c std#tuple&lt;&gt; means 'auto' keyword.
			 * @see CSS basic box model, 8. Margins (http://dev.w3.org/csswg/css-box-3/#margins)
			 */
			typedef StyleProperty<
				Multiple<
					boost::variant<Length, Percentage, MarginEnums>,
					Length
				>,
				Inherited<false>,
				boost::variant<Length, Percentage, std::tuple<>>
			> MarginSide;

			template<>			
			inline SpecifiedValue<MarginSide>::type uncompute<MarginSide>(const ComputedValue<MarginSide>::type& computedValue) {
				if(const Length* const length = boost::get<Length>(&computedValue))
					return *length;
				else if(const Percentage* const percentage = boost::get<Percentage>(&computedValue))
					return *percentage;
				else if(const std::tuple<>* const autoValue = boost::get<std::tuple<>>(&computedValue))
					return MarginEnums(MarginEnums::AUTO);
				else
					throw UnknownValueException("computedValue");
			}

			/**
			 * [Copied from CSS3] These properties specify the width and height of the content area or border area
			 * (depending on ‘box-sizing’) of certain boxes.
			 * @see CSS basic box model, 9. The width and height properties (http://dev.w3.org/csswg/css-box-3/#width)
			 */
			typedef StyleProperty<
				MultipleWithInitialIndex<
					boost::variant<Length, Percentage, std::tuple<>>,
					boost::mpl::int_<2>
				>,
				Inherited<false>
			> Measure;
			/// @}
		}
	}
}

#endif // !ASCENSION_STYLES_BOX_HPP
