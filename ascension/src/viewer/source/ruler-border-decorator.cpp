/**
 * @file ruler-border-decorator.cpp
 * Implements @c RulerBorderDecorator class.
 * @author exeal
 * @date 2015-01-17 Created.
 */

#include <ascension/graphics/rendering-context.hpp>
#include <ascension/viewer/source/ruler-allocation-width-sink.hpp>
#include <ascension/viewer/source/ruler-border-decorator.hpp>

namespace ascension {
	namespace viewer {
		namespace source {
			/**
			 * Creates @c RulerBorderDecorator object.
			 * @param decoratee The decoratee
			 * @param borderEnd The "Actual Value" of the 'border-end' style
			 * @throw UnknownValueException @a borderEnd is invalid
			 */
			RulerBorderDecorator::RulerBorderDecorator(std::unique_ptr<AbstractRuler> decoratee,
					const graphics::font::ActualBorderSide& borderEnd /* = graphics::font::ActualBorderSide() */) : RulerDecorator(std::move(decoratee)) {
				setBorderEnd(borderEnd);
			}

			/// @see RulerLocator#locateRuler
			graphics::Rectangle RulerBorderDecorator::locateRuler(const Ruler& ruler) const {
				if(&ruler != &decoratee())
					throw std::invalid_argument("ruler");
				if(const SourceViewer* sourceViewer = viewer()) {
					const graphics::Rectangle composite(locator_->locateRuler(*this));
					switch(boost::native_value(sourceViewer->rulerPhysicalAlignment)) {
						case graphics::PhysicalDirection::TOP:
							yrange.advance_end(-borderEnd_.actualWidth());
							break;
						case graphics::PhysicalDirection::RIGHT:
							xrange.advance_begin(+borderEnd_.actualWidth());
							break;
						case graphics::PhysicalDirection::BOTTOM:
							yrange.advance_begin(+borderEnd_.actualWidth());
							break;
						case graphics::PhysicalDirection::LEFT:
							xrange.advance_end(-borderEnd_.actualWidth());
							break;
					}
					return graphics::Rectangle(std::make_pair(xrange, yrange));
				}
				return boost::geometry::make_zero<graphics::Rectangle>();
			}

			/// @see Ruler#paint
			void RulerBorderDecorator::paint(graphics::PaintContext& context) {
				decoratee().paint(context);
				// TODO: Not implemented.
			}

			/**
			 * Sets 'border-end' style.
			 * @param borderEnd The "Actual Value" of the 'border-end' style
			 * @throw UnknownValueException @a borderEnd is invalid
			 */
			void RulerBorderDecorator::setBorderEnd(const graphics::font::ActualBorderSide& borderEnd) {
				const presentation::styles::BorderStyleEnums borderStyle = boost::fusion::at_key<presentation::styles::BorderStyle>(borderEnd);
				if(borderStyle < presentation::styles::BorderStyleEnums::NONE || borderStyle > presentation::styles::BorderStyleEnums::OUTSET)
					throw UnknownValueException("borderEnd");
				borderEnd_ = borderEnd;
				if(RulerAllocationWidthSink* sink = allocationWidthSink())
					sink->updateRulerAllocationWidth(*this);
			}

			/// @see Ruler#width
			graphics::Scalar RulerBorderDecorator::width() const BOOST_NOEXCEPT {
				return decoratee().width() + borderEnd_.actualWidth();
			}
		}
	}
}
