/**
 * @file source-viewer.cpp
 * Implements @c SourceViewer class.
 * @author exeal
 * @date 2015-03-01 Created.
 */

#include <ascension/corelib/numeric-range-algorithm/clamp.hpp>
#include <ascension/corelib/numeric-range-algorithm/order.hpp>
#include <ascension/graphics/font/text-layout.hpp>
#include <ascension/graphics/geometry/rectangle-corners.hpp>
#include <ascension/graphics/geometry/rectangle-range.hpp>
#include <ascension/graphics/geometry/algorithms/make.hpp>
#include <ascension/graphics/geometry/algorithms/within.hpp>
#include <ascension/presentation/presentation.hpp>
#include <ascension/presentation/writing-mode-mappings.hpp>
#include <ascension/viewer/mouse-input-strategy.hpp>
#include <ascension/viewer/source/composite-ruler.hpp>
#include <ascension/viewer/source/source-viewer.hpp>
#include <ascension/viewer/text-area.hpp>
#include <boost/geometry/arithmetic/arithmetic.hpp>	// boost.geometry.subtract_point

namespace ascension {
	namespace viewer {
		namespace source {
			/**
			 * Creates a @c SourceViewer instance.
			 * @param document The document
			 * @throw NullPointerException @a document is @c null
			 */
			SourceViewer::SourceViewer(std::shared_ptr<kernel::Document> document) : TextViewer(document) {
				setRulerAlignment(graphics::font::TextAlignment::START);
			}

			/// @see TextViewer#hitTest
			const TextViewerComponent* SourceViewer::hitTest(const graphics::Point& location) const BOOST_NOEXCEPT {
				if(ruler().get() != nullptr) {
					const graphics::Rectangle rulerRectangle(locateComponent(*ruler()));
					if(graphics::geometry::within(location, rulerRectangle)) {
						if(!rulerIsComposite_)
							return ruler().get();
						graphics::Point p(location);
						boost::geometry::subtract_point(p, graphics::geometry::topLeft(rulerRectangle));
						return static_cast<CompositeRuler*>(ruler().get())->hitTest(p);
					}
				}
				return TextViewer::hitTest(location);
			}

			/// @see TextViewer#keyPressed
			void SourceViewer::keyPressed(widgetapi::event::KeyInput& input) {
				if(ruler_.get() != nullptr) {
					if(const auto mouseInputStrategy = ruler_->mouseInputStrategy().lock())
						mouseInputStrategy->interruptMouseReaction(true);
				}
				return TextViewer::keyPressed(input);
			}

			/// @see TextViewer#keyReleased
			void SourceViewer::keyReleased(widgetapi::event::KeyInput& input) {
				if(input.hasModifier(widgetapi::event::ALT_DOWN) && ruler_.get() != nullptr) {
					if(const auto mouseInputStrategy = ruler_->mouseInputStrategy().lock())
						mouseInputStrategy->interruptMouseReaction(true);
				}
				return TextViewer::keyReleased(input);
			}

			/// @see TextViewerComponent#Locator#locateComponent
			graphics::Rectangle SourceViewer::locateComponent(const TextViewerComponent& component) const {
				if(ruler_.get() == nullptr)
					return TextViewer::locateComponent(component);

				const bool locateRuler = &component == ruler_.get();
				if(!locateRuler && &component != textArea().get())
					throw std::invalid_argument("component");

				const graphics::Scalar rulerWidth = std::max<graphics::Scalar>(ruler_->width(), 0);
				const graphics::Rectangle window(widgetapi::bounds(*this, false));
				auto xrange(graphics::geometry::range<0>(window) | adaptors::ordered());
				auto yrange(graphics::geometry::range<1>(window) | adaptors::ordered());
				switch(boost::native_value(rulerPhysicalAlignment())) {
					case graphics::PhysicalDirection::TOP:
						if(locateRuler)
							*boost::end(yrange) = clamp(*boost::const_begin(yrange) + rulerWidth, yrange);
						else
							yrange.advance_begin(+std::min(rulerWidth, boost::size(yrange)));
						break;
					case graphics::PhysicalDirection::RIGHT:
						if(locateRuler)
							*boost::begin(xrange) = clamp(*boost::const_end(xrange) - rulerWidth, xrange);
						else
							xrange.advance_end(-std::min(rulerWidth, boost::size(xrange)));
						break;
					case graphics::PhysicalDirection::BOTTOM:
						if(locateRuler)
							*boost::begin(yrange) = clamp(*boost::const_end(yrange) - rulerWidth, yrange);
						else
							yrange.advance_end(-std::min(rulerWidth, boost::size(yrange)));
						break;
					case graphics::PhysicalDirection::LEFT:
						if(locateRuler)
							*boost::end(xrange) = clamp(*boost::const_begin(xrange) + rulerWidth, xrange);
						else
							xrange.advance_begin(+std::min(rulerWidth, boost::size(xrange)));
						break;
					default:
						ASCENSION_ASSERT_NOT_REACHED();
				}

				return graphics::geometry::make<graphics::Rectangle>(std::make_pair(xrange, yrange));
			}

			/// @see TextViewer#paint
			void SourceViewer::paint(graphics::PaintContext& context) {
				TextViewer::paint(context);
				if(ruler_.get() != nullptr)
					ruler_->paint(context);
			}

			/// @see TextViewer#resized
			void SourceViewer::resized(const graphics::Dimension& newSize) {
				if(ruler_.get() != nullptr)
					ruler_->relocated();
				return TextViewer::resized(newSize);
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
							physicalAlignment = presentation::mapDirection(writingMode, presentation::FlowRelativeDirection::INLINE_START);
							break;
						case graphics::font::TextAlignment::END:
							physicalAlignment = presentation::mapDirection(writingMode, presentation::FlowRelativeDirection::INLINE_END);
							break;
						case graphics::font::TextAlignment::LEFT:
							physicalAlignment = presentation::mapDirection(writingMode, graphics::font::LineRelativeDirection::LINE_LEFT);
							break;
						case graphics::font::TextAlignment::RIGHT:
							physicalAlignment = presentation::mapDirection(writingMode, graphics::font::LineRelativeDirection::LINE_RIGHT);
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
				presentation::WritingMode writingMode;
				if(const auto ta = textArea()) {
					if(const auto renderer = ta->textRenderer())
						writingMode = renderer->writingModes();
				}
				rulerPhysicalAlignment_ = calculateRulerPhysicalAlignment(alignment, writingMode);
				rulerAbstractAlignment_ = alignment;
				updateTextAreaAllocationRectangle();
			}

			/// @see RulerAllocationWidthSink#updateRulerAllocationWidth
			void SourceViewer::updateRulerAllocationWidth(const Ruler&) {
				updateTextAreaAllocationRectangle();
			}
		}
	}
}
