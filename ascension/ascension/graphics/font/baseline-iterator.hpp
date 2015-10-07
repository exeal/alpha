/**
 * @file baseline-iterator.hpp
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2012
 * @date 2010-11-20 separated from ascension/layout.hpp
 * @date 2011-11-12 renamed from rendering.hpp
 * @date 2012-02-18 separated from text-renderer.hpp
 * @date 2015-09-27 Separated from text-viewport.hpp
 */

#ifndef ASCENSION_BASELINE_ITERATOR_HPP
#define ASCENSION_BASELINE_ITERATOR_HPP
#include <ascension/corelib/numeric-range.hpp>
#include <ascension/graphics/geometry/point.hpp>
#include <ascension/graphics/font/visual-line.hpp>
#include <ascension/kernel/position.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/optional.hpp>

namespace ascension {
	namespace graphics {
		namespace font {
			template<typename Position> class TextHit;
			class TextViewport;

			/**
			 * A @c BaselineIterator iterates the baselines of the visual lines in the specified @c TextViewport.
			 * @c #operator* returns distance from the before-edge of the @c TextViewport to the baseline of the
			 * current visual line.
			 * @note Once reached the end, this iterator can't move anymore.
			 */
			class BaselineIterator : public boost::iterators::iterator_facade<
				BaselineIterator,								// derived type
				Scalar,											// value_type
				boost::iterators::random_access_traversal_tag,	// iterator_category
				Scalar,											// reference
				std::ptrdiff_t									// difference_type
			> {
			public:
				BaselineIterator() BOOST_NOEXCEPT;
				explicit BaselineIterator(const TextViewport& viewport, bool trackOutOfViewport);
				BaselineIterator(const TextViewport& viewport, const VisualLine& line, bool trackOutOfViewport);
				BaselineIterator(const TextViewport& viewport, const TextHit<kernel::Position>& position, bool trackOutOfViewport);

				/// @name Line Number
				/// @{
				boost::optional<VisualLine> line() const;
				const VisualLine& snappedLine() const;
				/// @}

				/// @name Location and Geometry
				/// @{
				const NumericRange<Scalar>& extent() const;
				const NumericRange<Scalar>& extentWithHalfLeadings() const;
				const Point& positionInViewport() const;
				/// @}

				/// @name Other Attributes
				/// @{
				const TextViewport& viewport() const;
				bool tracksOutOfViewport() const BOOST_NOEXCEPT;
				/// @}

			private:
				void end() BOOST_NOEXCEPT;
				void internalAdvance(const VisualLine* to, const boost::optional<difference_type>& delta);
				void initializeWithFirstVisibleLine();
				bool isDefaultConstructed() const BOOST_NOEXCEPT {
					return viewport_ == nullptr;
				}
				bool isEnd() const BOOST_NOEXCEPT {
					return boost::geometry::get<0>(positionInViewport_) != 0 && boost::geometry::get<1>(positionInViewport_) != 0;
				}
				void verifyDerefereceable() const {
					if(isDefaultConstructed() || isEnd())
						throw NoSuchElementException();
				}
				// boost.iterator_facade
				void advance(difference_type n);
//				difference_type distance_to(const BaseIterator& other) const;
				void decrement();
				const reference dereference() const;
				bool equal(const BaselineIterator& other) const BOOST_NOEXCEPT;
				void increment();
				friend class boost::iterators::iterator_core_access;
			private:
				const TextViewport* viewport_;	// this is not a reference, for operator=
				bool tracksOutOfViewport_;	// this is not const, for operator=
				VisualLine line_;
				Scalar distanceFromViewportBeforeEdge_;
				NumericRange<Scalar> extent_, extentWithHalfLeadings_;
				Point positionInViewport_;
			};


			// inline implementation //////////////////////////////////////////////////////////////////////////////////

			/**
			 * Returns the extent of the current line in block-progression-dimension.
			 * @return The extent range in viewport-local coordinates
			 * @throw NoSuchElementException The iterator is invalid or addresses the end
			 * @see #extentWithHalfLeadings, TextLayout#LineMetricsIterator#extent
			 */
			inline const NumericRange<Scalar>& BaselineIterator::extent() const {
				verifyDerefereceable();
				return extent_;
			}

			/**
			 * Returns the extent of the current line in block-progression-dimension with leading.
			 * @return The extent range in viewport-local coordinates
			 * @throw NoSuchElementException The iterator is invalid or addresses the end
			 * @see #extent, TextLayout#LineMetricsIterator#extentWithHalfLeadings
			 */
			inline const NumericRange<Scalar>& BaselineIterator::extentWithHalfLeadings() const {
				verifyDerefereceable();
				return extentWithHalfLeadings_;
			}

			/**
			 * Returns the line the iterator addresses, or @c boost#none if out of the viewport.
			 * @return The visual line this iterator addresses
			 * @retval boost#none The iterator is out of the viewport
			 * @throw NoSuchElementException The iterator is invalid or addresses the end
			 * @see #snappedLine
			 */
			inline boost::optional<VisualLine> BaselineIterator::line() const {
				verifyDerefereceable();
				typedef std::iterator_traits<BaselineIterator>::value_type ValueType;
				if(**this == std::numeric_limits<ValueType>::min() || **this == std::numeric_limits<ValueType>::max())
					return boost::none;
				return line_;
			}

			/**
			 * Returns the baseline position of the line the iterator addresses.
			 * @return The point in view-local coordinates. If the writing mode is horizontal, x-coordinate of the
			 *         point is zero, otherwise y-coordinate is zero
			 * @throw NoSuchElementException The iterator is invalid or addresses the end
			 */
			inline const Point& BaselineIterator::positionInViewport() const {
				verifyDerefereceable();
				return positionInViewport_;
			}

			/**
			 * Returns the viewport.
			 * @throw NullPointerException This iterator is invalid
			 */
			inline const TextViewport& BaselineIterator::viewport() const {
				if(isDefaultConstructed())
					throw NullPointerException("this");
				return *viewport_;
			}

			/**
			 * Returns the line the iterator addresses. Unlike @c #line method, this returns a line snapped within the
			 * viewport.
			 * @throw NoSuchElementException The iterator is invalid or addresses the end
			 * @see #line
			 */
			inline const VisualLine& BaselineIterator::snappedLine() const {
				verifyDerefereceable();
				return line_;
			}

			/// Returns @c true if
			inline bool BaselineIterator::tracksOutOfViewport() const BOOST_NOEXCEPT {
				return tracksOutOfViewport_;
			}
		}
	}
} // namespace ascension.graphics.font

#endif // !ASCENSION_TEXT_VIEWPORT_HPP
