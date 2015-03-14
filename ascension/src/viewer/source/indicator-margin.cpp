/**
 * @file indicator-margin.cpp
 * Implements @c IndicatorMargin class.
 * @author exeal
 * @date 2015-01-17 Created.
 */

#include <ascension/graphics/paint.hpp>
#include <ascension/graphics/rendering-context.hpp>
#include <ascension/viewer/source/indicator-margin.hpp>
#include <ascension/viewer/source/ruler-allocation-width-sink.hpp>
#include <ascension/viewer/source/ruler-locator.hpp>

namespace ascension {
	namespace viewer {
		namespace source {
			/// Destructor does nothing.
			IndicatorMargin::~IndicatorMargin() BOOST_NOEXCEPT {
			}

			/// @see Ruler#paint
			void IndicatorMargin::paint(graphics::PaintContext& context) {
				if(const RulerLocator* const rulerLocator = locator()) {
					context.setFillStyle(std::make_shared<graphics::SolidColor>(backgroundColor()));
					context.fillRectangle(rulerLocator->locateRuler(*this));
				}
			}

			/**
			 * Sets 'minimum-width' style.
			 * @param minimumWidth The "Actual Value" of the 'minimum-width' style.
			 * @throw std#underflow_error @a minimumWidth &lt; 0
			 */
			void IndicatorMargin::setMinimumWidth(graphics::Scalar minimumWidth) {
				if(std::is_signed<decltype(minimumWidth)>::value && minimumWidth < 0)
					throw std::underflow_error("minimumWidth");
				if(minimumWidth != minimumWidth_) {
					minimumWidth_ = minimumWidth;
					if(RulerAllocationWidthSink* const sink = allocationWidthSink())
						sink->updateRulerAllocationWidth(*this);
				}
			}

			/// @see Ruler#width
			graphics::Scalar IndicatorMargin::width() const BOOST_NOEXCEPT {
				return minimumWidth_;
			}
		}
	}
}
