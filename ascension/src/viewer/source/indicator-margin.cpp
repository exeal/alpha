/**
 * @file indicator-margin.cpp
 * Implements @c IndicatorMargin class.
 * @author exeal
 * @date 2015-01-17 Created.
 */

#include <ascension/graphics/rendering-context.hpp>
#include <ascension/viewer/source/indicator-margin.hpp>

namespace ascension {
	namespace viewers {
		namespace source {
			/// Destructor does nothing.
			IndicatorMargin::~IndicatorMargin() BOOST_NOEXCEPT {
			}

			/// @see Ruler#paint
			void IndicatorMargin::paint(graphics::PaintContext& context) {
				// TODO: Not implemented.
			}

			/**
			 * Sets 'minimum-width' style.
			 * @param minimumWidth The "Actual Value" of the 'minimum-width' style.
			 * @throw std#underflow_error @a minimumWidth &lt; 0
			 */
			void IndicatorMargin::setMinimumWidth(graphics::Scalar minimumWidth) {
				if(std::is_signed<decltype(minimumWidth)>::value && minimumWidth < 0)
					throw std::underflow_error("minimumWidth");
				minimumWidth_ = minimumWidth;
			}

			/// @see Ruler#width
			graphics::Scalar IndicatorMargin::width() const BOOST_NOEXCEPT {
				return minimumWidth_;
			}
		}
	}
}
