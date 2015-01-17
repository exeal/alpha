/**
 * @file ruler-border-decorator.cpp
 * Implements @c RulerBorderDecorator class.
 * @author exeal
 * @date 2015-01-17 Created.
 */

#include <ascension/graphics/rendering-context.hpp>
#include <ascension/viewer/source/ruler-border-decorator.hpp>

namespace ascension {
	namespace viewers {
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
			}

			/// @see Ruler#width
			graphics::Scalar RulerBorderDecorator::width() const BOOST_NOEXCEPT {
				return decoratee().width() + borderEnd_.actualWidth();
			}
		}
	}
}
