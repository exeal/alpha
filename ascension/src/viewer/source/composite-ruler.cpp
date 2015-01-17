/**
 * @file composite-ruler.cpp
 * Implements @c CompositeRuler class.
 * @author exeal
 * @date 2015-01-12 Created.
 */

#include <ascension/viewer/source/composite-ruler.hpp>
#include <boost/foreach.hpp>
#include <boost/range/numeric.hpp>

namespace ascension {
	namespace viewers {
		namespace source {
			/// Default constructor.
			CompositeRuler::CompositeRuler() BOOST_NOEXCEPT : textViewer_(nullptr) {
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
				if(textViewer_ != nullptr)
					rulerColumn->install(*textViewer_);
				columns_.insert(std::begin(columns_) + position, std::move(rulerColumn));
			}

			/// @see Ruler#install
			void CompositeRuler::install(const TextViewer& viewer) {
				if(textViewer_ == nullptr) {
					// install ruler columns
					for(auto i(std::begin(columns_)), e(std::end(columns_)); i != e; ++i) {
						try {
							(*i)->install(viewer);
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
					textViewer_ = &viewer;
				}
			}

			/// @see Ruler#paint
			void CompositeRuler::paint(graphics::PaintContext& context) {
				// TODO: Not implemented.
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
				if(textViewer_ != nullptr)
					(*column)->uninstall(*textViewer_);
				columns_.erase(column);
			}

			/// @see Ruler#uninstall
			void CompositeRuler::uninstall(const TextViewer& viewer) {
				if(textViewer_ == &viewer) {
					// uninstall ruler columns
					BOOST_FOREACH(auto& column, columns_) {
						try {
							column->uninstall(viewer);
						} catch(...) {
							// ignore exceptions
						}
					}
					textViewer_ = nullptr;
				}
			}

			/// @see Ruler#width
			graphics::Scalar CompositeRuler::width() const BOOST_NOEXCEPT {
				graphics::Scalar total = 0;
				if(textViewer_ != nullptr)
					total = boost::accumulate(columns_, total, [](graphics::Scalar lhs, const std::unique_ptr<Ruler>& rhs) {
						return lhs += rhs->width();
					});
				return total;
			}
		}
	}
}
