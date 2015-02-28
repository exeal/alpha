/**
 * @file indicator-margin.hpp
 * Defines @c IndicatorMargin class.
 * @author exeal
 * @date 2015-01-17 Created.
 */

#ifndef ASCENSION_INDICATOR_MARGIN_HPP
#define ASCENSION_INDICATOR_MARGIN_HPP
#include <ascension/viewer/source/abstract-ruler.hpp>

namespace ascension {
	namespace viewer {
		namespace source {
			/// An indicator margin.
			class IndicatorMargin : public AbstractRuler {
			public:
				virtual ~IndicatorMargin() BOOST_NOEXCEPT;

				/// @name Styles
				/// @{
				void setMinimumWidth(graphics::Scalar minimumWidth);
				/// @}

			private:
				// Ruler
				void paint(graphics::PaintContext& context);
				graphics::Scalar width() const BOOST_NOEXCEPT;
			private:
				graphics::Scalar minimumWidth_;
			};
		}
	}
}

#endif // !ASCENSION_INDICATOR_MARGIN_HPP
