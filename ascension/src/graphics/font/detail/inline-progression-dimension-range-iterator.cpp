/**
 * @file inline-progression-dimension-range-iterator.cpp
 * @internal Implements @c InlineProgressionDimensionRangeIterator class.
 * @author exeal
 * @date 2015-03-29 Separated from text-layout-uniscribe.cpp.
 */

#include <ascension/corelib/numeric-range-algorithm/intersection.hpp>
#include <ascension/corelib/numeric-range-algorithm/overlaps.hpp>
#include <ascension/graphics/font/actual-text-styles.hpp>
#include <ascension/graphics/font/detail/inline-progression-dimension-range-iterator.hpp>
#include <ascension/graphics/font/text-run.hpp>

namespace ascension {
	namespace graphics {
		namespace font {
			namespace detail {
				InlineProgressionDimensionRangeIterator::InlineProgressionDimensionRangeIterator(
						const boost::iterator_range<std::vector<std::unique_ptr<const TextRun>>::const_iterator>& textRunsOfLine,
						presentation::ReadingDirection layoutDirection, const StringPiece& effectiveCharacterRange,
						const Direction& scanningDirection, Scalar firstLineEdgeIpd) :
						effectiveCharacterRange_(effectiveCharacterRange), layoutDirection_(layoutDirection),
						currentRunAllocationStartEdge_(firstLineEdgeIpd) {
					const presentation::ReadingDirection scanningReadingDirection = computeScanningReadingDirection(layoutDirection, scanningDirection);
					currentRun_ = (scanningReadingDirection == presentation::LEFT_TO_RIGHT) ? boost::const_begin(textRunsOfLine) : boost::const_end(textRunsOfLine) - 1;
					lastRun_ = (scanningReadingDirection == presentation::LEFT_TO_RIGHT) ? boost::const_end(textRunsOfLine) : boost::begin(textRunsOfLine) - 1;
					next(true);
				}

				const std::vector<std::unique_ptr<const TextRun>> InlineProgressionDimensionRangeIterator::dummy_;

				InlineProgressionDimensionRangeIterator::value_type InlineProgressionDimensionRangeIterator::dereference() const {
					if(isDone())
						throw NoSuchElementException();
					const TextRun/*Impl*/& currentRun = static_cast<const TextRun/*Impl*/&>(**currentRun_);
					const presentation::FlowRelativeFourSides<Scalar>* const padding = currentRun.padding();
					const presentation::FlowRelativeFourSides<ActualBorderSide>* const border = currentRun.border();
					const presentation::FlowRelativeFourSides<Scalar>* const margin = currentRun.margin();
					const Scalar allocationStartOffset =
						(padding != nullptr) ? padding->start() : 0
						+ (margin != nullptr) ? margin->start() : 0
						+ (border != nullptr) ? border->start().actualWidth() : 0;
					const auto range(nrange(currentRun.characterRange()));
					const auto subrange(intersection(range, nrange(effectiveCharacterRange())));
					assert(subrange != boost::none);
					Scalar startInRun = currentRun.hitToLogicalPosition(TextHit<>::leading(boost::const_begin(boost::get(subrange)) - boost::const_begin(range)));
					Scalar endInRun = currentRun.hitToLogicalPosition(TextHit<>::trailing(boost::const_end(boost::get(subrange)) - boost::const_begin(range)));
					if(currentRun.direction() == presentation::RIGHT_TO_LEFT) {
						const Scalar runMeasure = boost::size(measure(currentRun));
						startInRun = runMeasure - startInRun;
						endInRun = runMeasure - endInRun;
					}
					startInRun += allocationStartOffset;
					endInRun += allocationStartOffset;
					assert(startInRun <= endInRun);
					const Scalar startOffset = (currentRun.direction() == layoutDirection_) ? startInRun : boost::size(allocationMeasure(currentRun)) - endInRun;
					const Scalar endOffset = (currentRun.direction() == layoutDirection_) ? endInRun : boost::size(allocationMeasure(currentRun)) - startInRun;
					assert(startOffset <= endOffset);
					return boost::irange(currentRunAllocationStartEdge_ + startOffset, currentRunAllocationStartEdge_ + endOffset);
				}

				void InlineProgressionDimensionRangeIterator::next(bool initializing) {
					if(isDone())
						throw NoSuchElementException();
					std::vector<std::unique_ptr<const TextRun>>::const_iterator nextRun(currentRun_);
					Scalar nextIpd = currentRunAllocationStartEdge_;
					const Direction sd = scanningDirection();
					const presentation::ReadingDirection srd = computeScanningReadingDirection(layoutDirection_, sd);
					while(nextRun != lastRun_) {
						if(sd == Direction::forward()) {
							if(initializing && overlaps(boost::make_iterator_range((*nextRun)->characterRange()), boost::make_iterator_range(effectiveCharacterRange())))
								break;
							nextIpd += boost::size(allocationMeasure(**nextRun));
						} else {
							nextIpd -= boost::size(allocationMeasure(**nextRun));
							if(initializing && overlaps(boost::make_iterator_range((*nextRun)->characterRange()), boost::make_iterator_range(effectiveCharacterRange())))
								break;
						}
						if(srd == presentation::LEFT_TO_RIGHT)
							++nextRun;
						else
							--nextRun;
						if(!initializing)
							break;
					}
					// commit
					currentRun_ = nextRun;
					currentRunAllocationStartEdge_ = nextIpd;
				}
			}
		}
	}
}
