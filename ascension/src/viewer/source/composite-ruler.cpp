/**
 * @file composite-ruler.cpp
 * Implements @c CompositeRuler class.
 * @author exeal
 * @date 2015-01-12 Created.
 */

#include <ascension/viewer/source/composite-ruler.hpp>
#include <ascension/viewer/source/source-viewer.hpp>
#include <boost/foreach.hpp>
#include <boost/geometry/algorithms/make.hpp>
#include <boost/optional.hpp>
#include <boost/range/numeric.hpp>

namespace ascension {
	namespace viewer {
		namespace source {
			/// Default constructor.
			CompositeRuler::CompositeRuler() BOOST_NOEXCEPT : viewer_(nullptr), allocationWidthSink_(nullptr) {
			}

			/**
			 * Inserts the given column at the specified slot to this composite ruler.
			 * @param position The position
			 * @param rulerColumn The decorator to be inserted
			 * @throw IndexOutOfBoundsException @a position is invalid
			 * @throw NullPointerException @a rulerColumn is @c null
			 */
			void CompositeRuler::addDecorator(std::size_t position, std::unique_ptr<Ruler> rulerColumn) {
				if(rulerColumn.get() == nullptr)
					throw NullPointerException("rulerColumn");
				if(position > columns_.size())
					throw IndexOutOfBoundsException("position");
				if(viewer_ != nullptr)
					rulerColumn->install(*viewer_, *this, *allocationWidthSink_);
				columns_.insert(std::begin(columns_) + position, std::move(rulerColumn));
			}

			/// @see Ruler#install
			void CompositeRuler::install(SourceViewer& viewer, const Locator& locator, RulerAllocationWidthSink& allocationWidthSink) {
				if(viewer_ == nullptr) {
					// install ruler columns
					for(auto i(std::begin(columns_)), e(std::end(columns_)); i != e; ++i) {
						try {
							(*i)->install(viewer, *this, allocationWidthSink);
						} catch(...) {
							// rollback installation
							for(auto j(std::begin(columns_)); j != i; ++j) {
								try {
									(*j)->uninstall(viewer);
								} catch(...) {
								}
							}
						}
					}
					viewer_ = &viewer;
					allocationWidthSink_ = &allocationWidthSink;
					locator_ = &locator;
				}
			}

			/// @see TextViewerComponent#Locator#locateComponent
			graphics::Rectangle CompositeRuler::locateComponent(const TextViewerComponent& component) const {
				if(viewer_ != nullptr) {
					graphics::Scalar start = 0;
					boost::optional<NumericRange<graphics::Scalar>> range;
					BOOST_FOREACH(const std::unique_ptr<Ruler>& column, columns_) {
						if(column.get() == &component) {
							range = nrange(start, start + column->width());
							break;
						}
						start += column->width();
					}
					if(range == boost::none)
						throw std::invalid_argument("ruler");
				
					const graphics::Rectangle composite(locator_->locateComponent(*this));
					const graphics::PhysicalDirection physicalAlignment = viewer_->rulerPhysicalAlignment();
					graphics::Scalar offset = 0;
					switch(boost::native_value(physicalAlignment)) {
						case graphics::PhysicalDirection::RIGHT:
							offset = graphics::geometry::dx(composite) - range->size();
							break;
						case graphics::PhysicalDirection::BOTTOM:
							offset = graphics::geometry::dy(composite) - range->size();
							break;
					}
					range->advance_begin(offset).advance_end(offset);

					switch(boost::native_value(physicalAlignment)) {
						case graphics::PhysicalDirection::TOP:
						case graphics::PhysicalDirection::BOTTOM:
							return graphics::Rectangle(std::make_pair(graphics::geometry::range<0>(composite), boost::get(range)));
						case graphics::PhysicalDirection::LEFT:
						case graphics::PhysicalDirection::RIGHT:
							return graphics::Rectangle(std::make_pair(boost::get(range), graphics::geometry::range<1>(composite)));
					}
				}
				return boost::geometry::make_zero<graphics::Rectangle>();
			}

			/// @see Ruler#paint
			void CompositeRuler::paint(graphics::PaintContext& context) {
				BOOST_FOREACH(auto& column, columns_)
					column->paint(context);
			}

			/**
			 * Removed the decorator in the specified slot from this composite ruler.
			 * @param position The position
			 * @throw IndexOutOfBoundsException @a position is invalid
			 */
			void CompositeRuler::removeDecorator(std::size_t position) {
				if(position >= columns_.size())
					throw IndexOutOfBoundsException("position");
				const auto column(std::begin(columns_) + position);
				if(viewer_ != nullptr)
					(*column)->uninstall(*viewer_);
				columns_.erase(column);
			}

			/// @see Ruler#uninstall
			void CompositeRuler::uninstall(SourceViewer& viewer) {
				if(viewer_ == &viewer) {
					// uninstall ruler columns
					BOOST_FOREACH(auto& column, columns_) {
						try {
							column->uninstall(viewer);
						} catch(...) {
							// ignore exceptions
						}
					}
					viewer_ = nullptr;
					allocationWidthSink_ = nullptr;
					locator_ = nullptr;
				}
			}

			/// @see Ruler#width
			graphics::Scalar CompositeRuler::width() const BOOST_NOEXCEPT {
				graphics::Scalar total = 0;
				if(viewer_ != nullptr)
					total = boost::accumulate(columns_, total, [](graphics::Scalar lhs, const std::unique_ptr<Ruler>& rhs) {
						return lhs += rhs->width();
					});
				return total;
			}
		}
	}
}
