/**
 * @file baseline-iterator.cpp
 * @author exeal
 * @date 2003-2006 was LineLayout.cpp
 * @date 2006-2012, 2014
 * @date 2010-11-20 separated from ascension/layout.cpp
 * @date 2011-11-12 renamed from rendering.cpp
 * @date 2012-02-18 separated from text-renderer.hpp
 * @date 2015-09-27 Separated from baseline-iterator.cpp
 */

#include <ascension/graphics/font/baseline-iterator.hpp>
#include <ascension/graphics/font/line-layout-vector.hpp>
#include <ascension/graphics/font/text-layout.hpp>
#include <ascension/graphics/font/text-renderer.hpp>
#include <ascension/graphics/font/text-viewport.hpp>
#include <ascension/graphics/geometry/point-xy.hpp>
#include <ascension/graphics/geometry/rectangle-sides.hpp>
#include <ascension/graphics/geometry/algorithms/make.hpp>
#include <ascension/presentation/presentation.hpp>

namespace ascension {
	namespace graphics {
		namespace font {
			/**
			 * Default constructor creates an invalid instance.
			 * This instance equals to one which addresses the end of lines.
			 */
			BaselineIterator::BaselineIterator() BOOST_NOEXCEPT : viewport_(nullptr), tracksOutOfViewport_(false) {
				assert(isDefaultConstructed());
			}

			/**
			 * Constructor. Iterator will address the first visible visual line in the given viewport.
			 * @param viewport The text viewport
			 * @param trackOutOfViewport
			 */
			BaselineIterator::BaselineIterator(const TextViewport& viewport, bool trackOutOfViewport)
					: viewport_(&viewport), tracksOutOfViewport_(trackOutOfViewport) {
				initializeWithFirstVisibleLine();
			}

			/**
			 * Constructor.
			 * @param viewport The text viewport
			 * @param line The visual line this iterator addresses
			 * @param trackOutOfViewport
			 */
			BaselineIterator::BaselineIterator(const TextViewport& viewport, const VisualLine& line, bool trackOutOfViewport)
					: viewport_(&viewport), tracksOutOfViewport_(trackOutOfViewport) {
				initializeWithFirstVisibleLine();
				internalAdvance(&line, boost::none);
			}

			/**
			 * Constructor.
			 * @param viewport The text viewport
			 * @param position The position gives a visual line
			 * @param trackOutOfViewport
			 * @throw IllegalStateException @c viewport.textRenderer().lock() returned @c null
			 */
			BaselineIterator::BaselineIterator(const TextViewport& viewport, const TextHit<kernel::Position>& position, bool trackOutOfViewport)
					: viewport_(&viewport), tracksOutOfViewport_(trackOutOfViewport) {
				initializeWithFirstVisibleLine();
				VisualLine line(kernel::line(position.characterIndex()), 0);
				if(line.line < this->viewport().firstVisibleLine().line)
					internalAdvance(&line, boost::none);	// should go beyond before-edge
				else {
					const Index offset = kernel::offsetInLine(position.characterIndex());
					const TextHit<> hit(position.isLeadingEdge() ? TextHit<>::leading(offset) : TextHit<>::trailing(offset));
					const auto textRenderer(this->viewport().textRenderer().lock());
					if(textRenderer.get() == nullptr)
						throw IllegalStateException("viewport.textRenderer().lock() returned null.");
					if(line.line == this->viewport().firstVisibleLine().line) {
						line.subline = textRenderer->layouts().at(line.line)->lineAt(hit);
						internalAdvance(&line, boost::none);
					} else {
						internalAdvance(&line, boost::none);
						if(this->line() != boost::none)
							std::advance(*this, textRenderer->layouts().at(line.line)->lineAt(hit));
					}
				}
			}

			/// @see boost#iterator_facade#advance
			void BaselineIterator::advance(BaselineIterator::difference_type n) {
				return internalAdvance(nullptr, n);
			}

			/// @see boost#iterator_facade#decrement
			void BaselineIterator::decrement() {
				return advance(-1);
			}

			/// @see boost#iterator_facade#dereference
			const BaselineIterator::reference BaselineIterator::dereference() const {
				if(viewport_ == nullptr)
					throw NoSuchElementException();
				return distanceFromViewportBeforeEdge_;
			}

			inline void BaselineIterator::end() BOOST_NOEXCEPT {
				geometry::x(positionInViewport_) = geometry::y(positionInViewport_) = 1;
				assert(isEnd());
			}

			/// @see boost#iterator_facade#equal
			bool BaselineIterator::equal(const BaselineIterator& other) const BOOST_NOEXCEPT {
				if(isDefaultConstructed())
					return other.isDefaultConstructed() || other.isEnd();
				else if(isEnd())
					return other.isDefaultConstructed() || (other.isEnd() && viewport_ == other.viewport_);
				else
					return viewport_ == other.viewport_ && line_ == other.line_;
			}

			/// @see boost#iterator_facade#increment
			void BaselineIterator::increment() {
				return advance(+1);
			}

			namespace {
				template<typename T>
				inline Point calculatePositionInViewport(presentation::BlockFlowDirection blockFlowDirection, const T& bounds, Scalar distanceFromViewportBeforeEdge) {
					switch(blockFlowDirection) {
						case presentation::HORIZONTAL_TB:
							return geometry::make<Point>((geometry::_x = static_cast<Scalar>(0), geometry::_y = distanceFromViewportBeforeEdge));
							break;
						case presentation::VERTICAL_RL:
							return geometry::make<Point>((geometry::_x = geometry::dx(bounds) - distanceFromViewportBeforeEdge, geometry::_y = static_cast<Scalar>(0)));
							break;
						case presentation::VERTICAL_LR:
							return geometry::make<Point>((geometry::_x = distanceFromViewportBeforeEdge, geometry::_y = static_cast<Scalar>(0)));
							break;
						default:
							ASCENSION_ASSERT_NOT_REACHED();
					}
				}
			}

			/// @internal Moves this iterator to the first visible line in the viewport.
			void BaselineIterator::initializeWithFirstVisibleLine() {
				const auto textRenderer(viewport().textRenderer().lock());
				if(textRenderer.get() == nullptr)
					throw IllegalStateException("viewport.textRenderer().lock() returned null.");

				const VisualLine firstVisibleLine(viewport().firstVisibleLine());
				const TextLayout* const layout = textRenderer->layouts().at(firstVisibleLine.line);
				assert(layout != nullptr);
				const auto lineMetrics(layout->lineMetrics(firstVisibleLine.subline));
				const Scalar baseline = lineMetrics.baselineOffset() - *boost::const_begin(lineMetrics.extentWithHalfLeadings());
				Point axis;
				const Rectangle bounds(geometry::make<Rectangle>(boost::geometry::make_zero<Point>(), viewport().size()));
				switch(textRenderer->blockFlowDirection()) {
					case presentation::HORIZONTAL_TB:
						axis = geometry::make<Point>((geometry::_x = 0.0f, geometry::_y = geometry::top(bounds) + baseline));
						break;
					case presentation::VERTICAL_RL:
						axis = geometry::make<Point>((geometry::_x = geometry::right(bounds) - baseline, geometry::_y = 0.0f));
						break;
					case presentation::VERTICAL_LR:
						axis = geometry::make<Point>((geometry::_x = geometry::left(bounds) + baseline, geometry::_y = 0.0f));
						break;
					default:
						ASCENSION_ASSERT_NOT_REACHED();
				}

				// commit
				line_ = firstVisibleLine;
				distanceFromViewportBeforeEdge_ = baseline;
				extent_ = lineMetrics.extent() + baseline;
				extentWithHalfLeadings_ = lineMetrics.extentWithHalfLeadings() + baseline;
				positionInViewport_ = axis;
			}

			/// @internal Implements constructor and @c #advance method.
			void BaselineIterator::internalAdvance(const VisualLine* to, const boost::optional<difference_type>& delta) {
				assert(viewport_ != nullptr);
				const auto textRenderer(viewport().textRenderer().lock());
				if(textRenderer.get() == nullptr)
					throw IllegalStateException("viewport.textRenderer().lock() returned null.");

				bool forward;
				if(to != nullptr) {
					assert(delta == boost::none);
					if(to->line >= textRenderer->layouts().document().numberOfLines())
						throw IndexOutOfBoundsException("*to");
					if(*to == line())
						return;
					forward = *to > line();
				} else {
					assert(delta != boost::none);
					if(boost::get(delta) == 0)
						return;
					forward = boost::get(delta) > 0;
				}
				if(isDefaultConstructed())
					throw NullPointerException("this");
				if(isEnd())
					throw IllegalStateException("Iterator is end.");
				if(!tracksOutOfViewport()) {
					if(forward && **this == std::numeric_limits<value_type>::max()) {
//						line_ = VisualLine(destination, 0);
						return;	// already outside of after-edge of the viewport
					} else if(!forward && **this == std::numeric_limits<value_type>::min()) {
//						line_ = VisualLine(destination, 0);
						return;	// already outside of before-edge of the viewport
					}
				}

				// calculate extent of the viewport (if needed)
				const presentation::BlockFlowDirection blockFlowDirection(textRenderer->blockFlowDirection());
				const NumericRange<Scalar> viewportExtent(viewportContentExtent(viewport()));

				class LayoutPointer {
				public:
					explicit LayoutPointer(std::shared_ptr<const TextRenderer> textRenderer) : textRenderer_(textRenderer) {}
					const TextLayout* get(Index line) const {
						auto layout = textRenderer_->layouts().at(line);
						if(layout == nullptr) {
							if(emptyLayout_ == nullptr)
								const_cast<LayoutPointer*>(this)->emptyLayout_ = textRenderer_->createEmptyLineLayout();
							layout = emptyLayout_.get();
						}
						return layout;
					}
				private:
					std::shared_ptr<const TextRenderer> textRenderer_;
					const TextLayout* layout_;
					std::unique_ptr<const TextLayout> emptyLayout_;
				};

				auto newLine(boost::get(line()));
				auto newBaseline = **this;
				difference_type n = 0;
				LayoutPointer layouts(textRenderer);
				const TextLayout* layout = /*tracksOutOfViewport() ? &layouts[newLine.line] :*/ layouts.get(newLine.line);
				auto lineMetrics(layout->lineMetrics(newLine.subline));
				bool negativeVertical = detail::isNegativeVertical(*layout);
				if(forward) {
					bool ended = false;
					while((to != nullptr && newLine < *to) || (delta != boost::none && n < boost::get(delta))) {
						if(ended)
							throw std::overflow_error("");	// TODO: Is this suitable?
						newBaseline += *boost::const_end(lineMetrics.extentWithHalfLeadings()) - lineMetrics.baselineOffset();
						if(!tracksOutOfViewport() && newBaseline >= *boost::const_end(viewportExtent)) {
							newBaseline = std::numeric_limits<decltype(newBaseline)>::max();	// over after-edge of the viewport
							break;
						}

						// move to forward visual line
						if(newLine.subline == layout->numberOfLines() - 1) {
							if(newLine.line < textRenderer->layouts().document().numberOfLines() - 1) {
								layout = layouts.get(++newLine.line);
								lineMetrics = layout->lineMetrics(newLine.subline = 0);
								negativeVertical = detail::isNegativeVertical(*layout);
							} else
								ended = true;
						} else {
							++newLine.subline;
							++lineMetrics;
						}
						++n;

						if(!ended)
							newBaseline += lineMetrics.baselineOffset() - *boost::const_begin(lineMetrics.extentWithHalfLeadings());
					}
					if(ended)
						return end();
				} else {	// backward
					while((to != nullptr && newLine > *to) || (delta != boost::none && n > boost::get(delta))) {
						if(newLine.subline == 0 && newLine.line == 0)
							throw std::underflow_error("");	// TODO: Is this suitable?
						newBaseline -= lineMetrics.baselineOffset() - *boost::const_begin(lineMetrics.extentWithHalfLeadings());
						if(!tracksOutOfViewport() && newBaseline < *boost::const_begin(viewportExtent)) {
							newBaseline = std::numeric_limits<decltype(newBaseline)>::min();	// over before-edge of the viewport
							break;
						}

						// move to backward visual line
						if(newLine.subline == 0) {
							layout = layouts.get(--newLine.line);
							lineMetrics = layout->lineMetrics(newLine.subline = layout->numberOfLines() - 1);
							negativeVertical = detail::isNegativeVertical(*layout);
						} else {
							--newLine.subline;
							--lineMetrics;
						}
						--n;

						newBaseline -= *boost::const_end(lineMetrics.extentWithHalfLeadings()) - lineMetrics.baselineOffset();
					}
				}

				// commit
				positionInViewport_ = calculatePositionInViewport(blockFlowDirection, viewport().size(), newBaseline);
				std::swap(line_, newLine);
				extent_ = lineMetrics.extent() + newBaseline;
				extentWithHalfLeadings_ = lineMetrics.extentWithHalfLeadings() + newBaseline;
				std::swap(distanceFromViewportBeforeEdge_, newBaseline);
			}
		}
	}
}
