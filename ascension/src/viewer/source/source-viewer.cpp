/**
 * @file source-viewer.cpp
 * Implements @c SourceViewer class.
 * @author exeal
 * @date 2015-03-01 Created.
 */

#include <ascension/viewer/source/source-viewer.hpp>

namespace ascension {
	namespace viewer {
		namespace source {
			/// @see TextViewer#doTextAreaAllocationRectangle
			graphics::Rectangle SourceViewer::doTextAreaAllocationRectangle() const BOOST_NOEXCEPT {
				if(ruler_.get() == nullptr)
					return TextViewer::doTextAreaAllocationRectangle();
				using graphics::PhysicalDirection;
				const graphics::Rectangle window(widgetapi::bounds(*this, false));
				graphics::PhysicalFourSides<graphics::Scalar> result(window);
				if(rulerPainter_.get() != nullptr) {
					switch(boost::native_value(rulerPainter_->alignment())) {
						case PhysicalDirection::LEFT:
							result.left() += rulerPainter_->allocationWidth();
							break;
						case PhysicalDirection::TOP:
							result.top() += rulerPainter_->allocationWidth();
							break;
						case PhysicalDirection::RIGHT:
							result.right() -= rulerPainter_->allocationWidth();
							break;
						case PhysicalDirection::BOTTOM:
							result.bottom() -= rulerPainter_->allocationWidth();
							break;
						default:
							ASCENSION_ASSERT_NOT_REACHED();
					}
				}
				return graphics::geometry::make<graphics::Rectangle>(result);
			}

			/// @see TextViewer#paint
			void SourceViewer::paint(graphics::PaintContext& context) {
				TextViewer::paint(context);
				if(ruler_.get() != nullptr)
					ruler_->paint(context);
			}

			/// @see TextViewer#resized
			void SourceViewer::resized(const graphics::Dimension& newSize) {
				rulerPainter_->update();
				if(rulerPainter_->alignment() != graphics::PhysicalDirection::LEFT && rulerPainter_->alignment() != graphics::PhysicalDirection::TOP) {
//					recreateCaret();
//					redrawVerticalRuler();
					widgetapi::scheduleRedraw(*this, false);	// hmm...
				}
			}

			/// @see RulerAllocationWidthSink#rulerAllocationWidthChanged
			void SourceViewer::rulerAllocationWidthChanged(const Ruler&) {
				updateTextAreaAllocationRectangle();
			}

			namespace {
				/**
				 * @internal Computes the snap alignment of the ruler of the @c SourceViewer.
				 * @param abstractAlignment The defined abstract alignment
				 * @param writingMode The writing mode
				 * @return The snap alignment of the ruler in the @c SourceViewer
				 */
				graphics::PhysicalDirection calculateRulerPhysicalAlignment(
						graphics::font::TextAlignment abstractAlignment, const presentation::WritingMode& writingMode) {
					graphics::PhysicalDirection physicalAlignment;
					switch(boost::native_value(abstractAlignment)) {
						case graphics::font::TextAlignment::START:
							physicalAlignment = presentation::mapFlowRelativeToPhysical(writingMode, presentation::FlowRelativeDirection::INLINE_START);
							break;
						case graphics::font::TextAlignment::END:
							physicalAlignment = presentation::mapFlowRelativeToPhysical(writingMode, presentation::FlowRelativeDirection::INLINE_END);
							break;
						case graphics::font::TextAlignment::LEFT:
							physicalAlignment = presentation::mapLineRelativeToPhysical(writingMode, graphics::font::LineRelativeDirection::LINE_LEFT);
							break;
						case graphics::font::TextAlignment::RIGHT:
							physicalAlignment = presentation::mapLineRelativeToPhysical(writingMode, graphics::font::LineRelativeDirection::LINE_RIGHT);
							break;
						default:
							throw UnknownValueException("abstractAlignment");
					}

					assert(presentation::isHorizontal(writingMode.blockFlowDirection)
						|| physicalAlignment == graphics::PhysicalDirection::TOP || physicalAlignment == graphics::PhysicalDirection::BOTTOM);
					assert(presentation::isVertical(writingMode.blockFlowDirection)
						|| physicalAlignment == graphics::PhysicalDirection::LEFT || physicalAlignment == graphics::PhysicalDirection::RIGHT);

					return physicalAlignment;
				}
			}

			/**
			 * Sets the alignment (anchor) of the ruler.
			 * @param alignment The abstract alignment. Must be either @c graphics#font#TextAlignment#START,
			 * @c graphics#font#TextAlignment#END, @c graphics#font#TextAlignment#LEFT or
			 * @c graphics#font#TextAlignment#RIGHT. In vertical layout, @c graphics#font#TextAlignment#LEFT and
			 * @c graphics#font#TextAlignment#RIGHT are treated as top and bottom respectively.
			 */
			void SourceViewer::setRulerAlignment(graphics::font::TextAlignment alignment) {
				const graphics::PhysicalDirection newPhysicalAlignment =
					calculateRulerPhysicalAlignment(alignment, presentation().computeWritingMode());
				updateTextAreaAllocationRectangle();
			}

			/// @see TextViewer#unfrozen
			void SourceViewer::unfrozen(const boost::integer_range<Index>& linesToRedraw) {
				rulerPainter_->update();
			}
		}
	}
}
