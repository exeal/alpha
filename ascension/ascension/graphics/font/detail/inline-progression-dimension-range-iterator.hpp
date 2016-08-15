/**
 * @file inline-progression-dimension-range-iterator.hpp
 * @internal Defines @c InlineProgressionDimensionRangeIterator class.
 * @author exeal
 * @date 2015-03-29 Separated from text-layout-uniscribe.cpp.
 */

#ifndef ASCENSION_INLINE_PROGRESSION_DIMENSION_RANGE_ITERATOR_HPP
#define ASCENSION_INLINE_PROGRESSION_DIMENSION_RANGE_ITERATOR_HPP
#include <ascension/corelib/numeric-range.hpp>
#include <ascension/corelib/string-piece.hpp>
#include <ascension/direction.hpp>
#include <ascension/graphics/geometry/common.hpp>
#include <ascension/presentation/writing-mode.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <memory>
#include <utility>
#include <vector>

namespace ascension {
	namespace graphics {
		namespace font {
			class TextRun;

			namespace detail {
				class InlineProgressionDimensionRangeIterator :
					public boost::iterators::iterator_facade<InlineProgressionDimensionRangeIterator,
						NumericRange<Scalar>, boost::iterators::forward_traversal_tag, NumericRange<Scalar>, std::ptrdiff_t
					> {
				public:
					InlineProgressionDimensionRangeIterator() BOOST_NOEXCEPT
						: currentRun_(std::begin(dummy_)), lastRun_(std::begin(dummy_)) {}
					InlineProgressionDimensionRangeIterator(
						const boost::iterator_range<std::vector<std::unique_ptr<const TextRun>>::const_iterator>& textRunsOfLine,
						presentation::ReadingDirection layoutDirection, const StringPiece& effectiveCharacterRange,
						const Direction& scanningDirection, Scalar firstLineEdgeIpd);
					value_type dereference() const;
					const StringPiece& effectiveCharacterRange() const BOOST_NOEXCEPT {
						return effectiveCharacterRange_;
					}
					bool equal(const InlineProgressionDimensionRangeIterator& other) const BOOST_NOEXCEPT {
						return isDone() && other.isDone();
					}
					void increment() {
						return next(false);
					}
					Direction scanningDirection() const BOOST_NOEXCEPT {
						int temp = (currentRun_ <= lastRun_) ? 0 : 1;
						temp += (layoutDirection_ == presentation::LEFT_TO_RIGHT) ? 0 : 1;
						return (temp % 2 == 0) ? Direction::forward() : Direction::backward();
					}
				private:
					static presentation::ReadingDirection computeScanningReadingDirection(
							presentation::ReadingDirection layoutDirection, const Direction& scanningDirection) {
						presentation::ReadingDirection computed = layoutDirection;
						if(scanningDirection == Direction::backward())
							computed = !computed;
						return computed;
					}
					void next(bool initializing);
					bool isDone() const BOOST_NOEXCEPT {return currentRun_ == lastRun_;}
				private:
					static const std::vector<std::unique_ptr<const TextRun>> dummy_;
					friend class boost::iterators::iterator_core_access;
					/*const*/ presentation::ReadingDirection layoutDirection_;
					/*const*/ StringPiece effectiveCharacterRange_;
					std::vector<std::unique_ptr<const TextRun>>::const_iterator currentRun_;
					/*const*/ std::vector<std::unique_ptr<const TextRun>>::const_iterator lastRun_;
					Scalar currentRunAllocationStartEdge_;	// 'start' means for 'layoutDirection_'
				};
			}
		}
	}
}

#endif // !ASCENSION_INLINE_PROGRESSION_DIMENSION_RANGE_ITERATOR_HPP
