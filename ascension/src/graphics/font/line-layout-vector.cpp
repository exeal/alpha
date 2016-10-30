/**
 * @file line-layout-vector.cpp
 * @author exeal
 * @date 2003-2006 was LineLayout.cpp
 * @date 2006-2011 was rendering.cpp
 * @date 2010-11-20 separated from ascension/layout.cpp
 * @date 2011-??-?? separated from rendering.cpp
 * @date 2011-2014
 */

#include <ascension/corelib/numeric-range-algorithm/order.hpp>
#include <ascension/graphics/font/line-layout-vector.hpp>
#include <ascension/graphics/font/text-layout.hpp>
#include <ascension/graphics/font/visual-line.hpp>
#include <ascension/kernel/document.hpp>
#include <ascension/log.hpp>
#include <boost/foreach.hpp>
#include <boost/range/algorithm/sort.hpp>

namespace ascension {
	namespace graphics {
		namespace font {
			// LineLayoutVector ///////////////////////////////////////////////////////////////////////////////////////

			const UseCalculatedLayoutTag LineLayoutVector::USE_CALCULATED_LAYOUT;

			/// Destructor.
			LineLayoutVector::~LineLayoutVector() BOOST_NOEXCEPT {
			//	clearCaches(startLine_, startLine_ + bufferSize_, false);
				document_.removePrenotifiedListener(*this);
				document_.removePartitioningListener(*this);
			}

			/**
			 * Returns the layout of the specified line.
			 * @param line The line number. This method does not throw an exception even if this paramter is out of
			 *             bound
			 * @return The layout
			 * @see #at
			 */
			const TextLayout& LineLayoutVector::operator[](Index line) {
#if defined(ASCENSION_TRACE_LAYOUT_CACHES) && defined(_DEBUG)
				ASCENSION_LOG_TRIVIAL(debug) << "finding layout for line " << line;
#endif
				LineLayoutVector& self = *const_cast<LineLayoutVector*>(this);
				auto i(std::begin(self.layouts_));
				for(const auto e(end(self.layouts_)); i != e; ++i) {
					if(i->lineNumber == line)
						break;
				}

				if(i != std::end(layouts_)) {
#if defined(ASCENSION_TRACE_LAYOUT_CACHES) && defined(_DEBUG)
					ASCENSION_LOG_TRIVIAL(debug) << "... cache found\n";
#endif
					if(i->layout != layouts_.front().layout) {
						// bring to the top
						NumberedLayout temp(std::move(*i));
						self.layouts_.erase(i);
						self.layouts_.push_front(std::move(temp));
						i = std::begin(self.layouts_);
					}
					return *i->layout;
				} else {
#ifdef ASCENSION_TRACE_LAYOUT_CACHES
					dout << "... cache not found\n";
#endif
					if(boost::size(layouts_) == bufferSize_) {
						// delete the last
						NumberedLayout temp(std::move(self.layouts_.back()));
						self.layouts_.pop_back();
						self.fireVisualLinesModified(boost::irange(temp.lineNumber, temp.lineNumber + 1),
							1, temp.layout->numberOfLines(), documentChangePhase_ == CHANGING);
					}

					NumberedLayout newLayout;
					newLayout.lineNumber = line;
					newLayout.layout = layoutGenerator_->generate(line);
					self.layouts_.push_front(std::move(newLayout));
					self.fireVisualLinesModified(boost::irange(line, line + 1), layouts_.front().layout->numberOfLines(), 1, documentChangePhase_ == CHANGING);
					return *layouts_.front().layout;
				}
			}

			/**
			 * Registers the visual lines listener.
			 * @param listener The listener to be registered
			 * @throw std#invalid_argument @a listener is already registered
			 */
			void LineLayoutVector::addVisualLinesListener(VisualLinesListener& listener) {
				listeners_.add(listener);
				const Index lines = document_.numberOfLines();
				if(lines > 1)
					listener.visualLinesInserted(boost::irange(static_cast<Index>(1), lines));
			}

			/**
			 * Returns the layout of the specified line.
			 * @param line The line
			 * @return The layout
			 * @throw IndexOutOfBoundsException @a line is greater than the number of the lines
			 * @see #operator[], #at(Index)
			 */
			const TextLayout& LineLayoutVector::at(Index line, const UseCalculatedLayoutTag&) {
				if(line >= document().numberOfLines())
					throw IndexOutOfBoundsException("line");
				return (*this)[line];
			}

			/**
			 * Clears the layout caches of the specified lines.
			 * @param lines The range of lines. @a lines.end() is exclusive and will not be cleared
			 * @param repair Set @c true to recreate layouts for the lines. If @c true, this method calls
			 *               @c VisualLinesListener#visualLinesModified. Otherwise calls
			 *               @c VisualLinesListener#visualLinesDeleted
			 */
			void LineLayoutVector::clearCaches(const boost::integer_range<Index>& lines, bool repair) {
				const auto orderedLines(lines | adaptors::ordered());
				if(false /*|| *orderedLines.end() > document().numberOfLines()*/)
					throw IndexOutOfBoundsException("lines");
				if(documentChangePhase_ == ABOUT_TO_CHANGE) {
					pendingCacheClearance_ = boost::irange(
						!pendingCacheClearance_ ?
							*boost::const_begin(orderedLines) : std::min(*boost::const_begin(orderedLines), *boost::const_begin(boost::get(pendingCacheClearance_))),
						!pendingCacheClearance_ ?
							*boost::const_end(orderedLines) : std::max(*boost::const_end(orderedLines), *boost::const_end(boost::get(pendingCacheClearance_))));
					return;
				}
				if(boost::empty(orderedLines))
					return;

//				const std::size_t originalSize = layouts_.size();
				Index oldSublines = 0, cachedLines = 0;
				if(repair) {
					Index newSublines = 0, actualFirst = *boost::const_end(orderedLines), actualLast = *boost::const_begin(orderedLines);
					BOOST_FOREACH(NumberedLayout& layout, layouts_) {
						if(includes(orderedLines, layout.lineNumber)) {
							oldSublines += layout.layout->numberOfLines();
							layout.layout = layoutGenerator_->generate(layout.lineNumber);
							assert(layout.layout.get() != nullptr);	// TODO:
							newSublines += layout.layout->numberOfLines();
							++cachedLines;
							actualFirst = std::min(actualFirst, layout.lineNumber);
							actualLast = std::max(actualLast, layout.lineNumber);
						}
					}
					if(actualFirst == *boost::const_end(orderedLines))	// no lines cleared
						return;
					++actualLast;
					fireVisualLinesModified(boost::irange(actualFirst, actualLast), newSublines += actualLast - actualFirst - cachedLines,
						oldSublines += actualLast - actualFirst - cachedLines, documentChangePhase_ == CHANGING);
				} else {
					for(auto i(boost::const_begin(layouts_)); i != boost::const_end(layouts_); ) {
						if(includes(orderedLines, i->lineNumber)) {
							oldSublines += i->layout->numberOfLines();
							i = layouts_.erase(i);
							++cachedLines;
						} else
							++i;
					}
					fireVisualLinesDeleted(orderedLines, oldSublines += orderedLines.size() - cachedLines);
				}
			}

			/**
			 * Creates and returns an isolated layout for the specified line. This layout is not inserted into the
			 * vector and the instances of @c VisualLinesListener are not invoked.
			 * @param line The line number
			 * @return The layout
			 * @throw IndexOutOfBoundsException @a line is greater than the number of the lines
			 */
			std::unique_ptr<const TextLayout> LineLayoutVector::createIsolatedLayout(Index line) const {
				if(line > document().numberOfLines())
					throw IndexOutOfBoundsException("line");
				return layoutGenerator_->generate(line);
			}

			/// @see kernel#DocumentListener#documentAboutToBeChanged
			void LineLayoutVector::documentAboutToBeChanged(const kernel::Document&) {
				documentChangePhase_ = ABOUT_TO_CHANGE;
			}

			/// @see kernel#DocumentListener#documentChanged
			void LineLayoutVector::documentChanged(const kernel::Document&, const kernel::DocumentChange& change) {
				documentChangePhase_ = CHANGING;
				if(boost::size(change.erasedRegion().lines()) > 1) {	// erased region includes newline(s)
					const kernel::Region& region = change.erasedRegion();
					clearCaches(boost::irange(kernel::line(*boost::const_begin(region)) + 1, kernel::line(*boost::const_end(region)) + 1), false);
					BOOST_FOREACH(NumberedLayout& layout, layouts_) {
						if(layout.lineNumber > kernel::line(*boost::const_begin(region)))
							layout.lineNumber -= boost::size(region.lines()) - 1;	// $friendly-access
					}
				}
				if(boost::size(change.insertedRegion().lines()) > 1) {	// inserted text is multiline
					const kernel::Region& region = change.insertedRegion();
					BOOST_FOREACH(NumberedLayout& layout, layouts_) {
						if(layout.lineNumber > kernel::line(*boost::const_begin(region)))
							layout.lineNumber += boost::size(region.lines()) - 1;	// $friendly-access
					}
					fireVisualLinesInserted(boost::irange(kernel::line(*boost::const_begin(region)) + 1, kernel::line(*boost::const_end(region)) + 1));
				}
				const Index firstLine = std::min(kernel::line(*boost::const_begin(change.erasedRegion())), kernel::line(*boost::const_begin(change.insertedRegion())));
				if(!pendingCacheClearance_ || !includes(*pendingCacheClearance_, firstLine))
					invalidate(firstLine);
				documentChangePhase_ = NONE;
				if(pendingCacheClearance_) {
					clearCaches(*pendingCacheClearance_, autoRepair_);
					pendingCacheClearance_ = boost::none;
				}
			}

			/// @see kernel#DocumentPartitioningListener#documentPartitioningChanged
			void LineLayoutVector::documentPartitioningChanged(const kernel::Region& changedRegion) {
				invalidate(changedRegion.lines());
			}

			void LineLayoutVector::fireVisualLinesDeleted(const boost::integer_range<Index>& lines, Index sublines) {
				numberOfVisualLines_ -= sublines;
				const bool measureChanged = longestLine_ == boost::none || includes(lines, boost::get(longestLine_));
				if(measureChanged)
					updateLongestLine(boost::none, 0);
				listeners_.notify<const boost::integer_range<Index>&, Index>(
					&VisualLinesListener::visualLinesDeleted, lines, sublines, measureChanged);
			}

			void LineLayoutVector::fireVisualLinesInserted(const boost::integer_range<Index>& lines) {
				numberOfVisualLines_ += boost::size(lines);
				listeners_.notify<const boost::integer_range<Index>&>(&VisualLinesListener::visualLinesInserted, lines);
			}

			void LineLayoutVector::fireVisualLinesModified(const boost::integer_range<Index>& lines,
					Index newSublines, Index oldSublines, bool documentChanged) {
				numberOfVisualLines_ += newSublines;
				numberOfVisualLines_ -= oldSublines;

				// update the longest line
				bool longestLineChanged = false;
				if(longestLine_ == boost::none || includes(lines, boost::get(longestLine_))) {
					updateLongestLine(boost::none, 0);
					longestLineChanged = true;
				} else {
					Index newLongestLine = boost::get(longestLine_);
					Scalar newMaximumIpd = maximumMeasure();
					BOOST_FOREACH(const NumberedLayout& layout, layouts_) {
						if(layout.layout->measure() > newMaximumIpd) {
							newLongestLine = layout.lineNumber;
							newMaximumIpd = layout.layout->measure();
						}
					}
					if(longestLineChanged = (newLongestLine != longestLine_))
						updateLongestLine(newLongestLine, newMaximumIpd);
				}

				listeners_.notify<const boost::integer_range<Index>&, SignedIndex>(
					&VisualLinesListener::visualLinesModified, lines,
					static_cast<SignedIndex>(newSublines) - static_cast<SignedIndex>(oldSublines),
					documentChanged, longestLineChanged);
			}

			/// @internal Only called by constructor.
			void LineLayoutVector::initialize() {
				pendingCacheClearance_ = boost::none;
				if(bufferSize_ == 0)
					throw std::invalid_argument("size of the buffer can't be zero.");
				document_.addPrenotifiedListener(*this);
				document_.addPartitioningListener(*this);
			}

			/// Invalidates all layouts.
			void LineLayoutVector::invalidate() BOOST_NOEXCEPT {
				clearCaches(boost::irange(static_cast<Index>(0), document().numberOfLines()), autoRepair_);
			}

			/**
			 * Invalidates the layouts of the specified lines.
			 * @param lines The range of the lines. @a lines.end() is exclusive and will not be cleared
			 */
			void LineLayoutVector::invalidate(const boost::integer_range<Index>& lines) {
				clearCaches(lines, autoRepair_);
			}

			void LineLayoutVector::invalidate(const std::vector<Index>& lines) {
				std::vector<Index> cachedLines;
				BOOST_FOREACH(const NumberedLayout& layout, layouts_)
					cachedLines.push_back(layout.lineNumber);
				boost::sort(cachedLines);

				typedef std::vector<Index>::const_iterator Iterator;
				const Iterator e(std::end(cachedLines));
				for(std::pair<Iterator, Iterator> p(std::mismatch<Iterator, Iterator>(std::begin(cachedLines), e, std::begin(lines))); ; ) {
					p.first = std::find(p.first, e, *p.second);
					const std::pair<Iterator, Iterator> next(std::mismatch(p.first, e, p.second));
					if(next.second == std::end(lines)) {
						clearCaches(boost::irange(*p.second, lines.back() + 1), autoRepair_);
						break;
					}
					clearCaches(boost::irange(*p.second, *next.second), autoRepair_);
					p = next;
				}
			}

			/**
			 * Resets the cached layout of the specified line and repairs if necessary.
			 * @param line The line to invalidate layout
			 */
			inline void LineLayoutVector::invalidate(Index line) {
				for(auto i(std::begin(layouts_)), e(std::end(layouts_)); i != e; ++i) {
					if(i->lineNumber == line) {
						const Index oldSublines = i->layout->numberOfLines();
						if(autoRepair_) {
#if 0
							std::unique_ptr<const TextLayout> newLayout(layoutGenerator_->generate(line));
							std::swap(i->layout, newLayout);
#else
							i->layout.reset();
							i->layout = layoutGenerator_->generate(line);
#endif
							fireVisualLinesModified(boost::irange(line, line + 1),
								i->layout->numberOfLines(), oldSublines, documentChangePhase_ == CHANGING);
						} else {
							layouts_.erase(i);
							fireVisualLinesModified(boost::irange(line, line + 1),
								1, oldSublines, documentChangePhase_ == CHANGING);
						}
						break;
					}
				}
			}

			/**
			 * Returns the first visual line number of the specified logical line.
			 * @param line The logical line
			 * @return The first visual line of @a line
			 * @throw IndexOutOfBoundsException @a line is outside of the document
			 * @note This method treats an uncalculated line as a single visual line
			 * @see #mapLogicalPositionToVisualPosition
			 */
			Index LineLayoutVector::mapLogicalLineToVisualLine(Index line) const {
				if(line >= document().numberOfLines())
					throw IndexOutOfBoundsException("line");
//				else if(!wrapLongLines())
//					return line;
				Index result = 0, cachedLines = 0;
				BOOST_FOREACH(const NumberedLayout& layout, layouts_) {
					if(layout.lineNumber < line) {
						result += layout.layout->numberOfLines();
						++cachedLines;
					}
				}
				return result + line - cachedLines;
			}

			/**
			 * Returns the visual line number and the offset in the visual line of the specified logical position.
			 * @param position The logical coordinates of the position to be mapped
			 * @param[out] offsetInVisualLine The offset of @a position in visual line. Can be @c null if not needed
			 * @return The visual line of @a position
			 * @throw kernel#BadPositionException @a position is outside of the document
			 * @see #mapLogicalLineToVisualLine
			 */
			Index LineLayoutVector::mapLogicalPositionToVisualPosition(const kernel::Position& position, Index* offsetInVisualLine) const {
//				if(!wrapsLongLines()) {
//					if(offsetInVisualLine != 0)
//						*offsetInVisualLine = kernel::offsetInLine(position);
//					return kernel::line(position);
//				}
				try {
					const TextLayout* const layout = at(kernel::line(position));
					const Index line = (layout != nullptr) ? layout->lineAt(TextHit<>::leading(kernel::offsetInLine(position))) : 0;
					if(offsetInVisualLine != nullptr)
						*offsetInVisualLine = kernel::offsetInLine(position) - ((layout != nullptr) ? layout->lineOffset(line) : 0);
					return mapLogicalLineToVisualLine(kernel::line(position)) + line;
				} catch(const IndexOutOfBoundsException&) {
					throw kernel::BadPositionException(position);
				}
			}

			/**
			 * Returns the number of sublines of the specified line.
			 * If the layout of the line is not calculated, this method returns 1.
			 * @param line The line
			 * @return The number of the sublines
			 * @throw IndexOutOfBoundsException @a line is outside of the document
			 * @see #at(Index), TextLayout#numberOfLines
			 */
			Index LineLayoutVector::numberOfSublinesOfLine(Index line) const {
				if(line >= document().numberOfLines())
					throw IndexOutOfBoundsException("line");
				const TextLayout* const layout = at(line);
				return (layout != nullptr) ? layout->numberOfLines() : 1;
			}

			/**
			 * Returns the number of sublines of the specified line.
			 * @param line The line
			 * @return The number of the sublines
			 * @throw IndexOutOfBoundsException @a line is outside of the document
			 * @see #at(Index, const UseCalculatedLayoutTag&), TextLayout#numberOfLines
			 */
			Index LineLayoutVector::numberOfSublinesOfLine(Index line, const UseCalculatedLayoutTag&) {
				if(line >= document().numberOfLines())
					throw IndexOutOfBoundsException("line");
				return at(line, USE_CALCULATED_LAYOUT).numberOfLines();
			}

			/// @internal
			SignedIndex LineLayoutVector::offsetVisualLine(VisualLine& line, SignedIndex offset, bool useCalculatedLayout) const {
				LineLayoutVector& self = *const_cast<LineLayoutVector*>(this);
				SignedIndex walked = 0;
				if(offset > 0) {
					while(true) {
						const Index sublines = useCalculatedLayout ? self.numberOfSublinesOfLine(line.line, USE_CALCULATED_LAYOUT) : numberOfSublinesOfLine(line.line);
						if(line.subline + offset < sublines) {
							line.subline += offset;
							walked += offset;
							break;
						} else if(line.line == document().numberOfLines() - 1) {
							walked += sublines - 1 - line.subline;
							line.subline = sublines - 1;
							break;
						}
						offset -= sublines - line.subline;
						walked += sublines - line.subline;
						++line.line;
						line.subline = 0;
					}
				} else if(offset < 0) {
					while(true) {
						if(line.subline >= static_cast<Index>(-offset)) {
							line.subline += offset;
							walked += offset;
							break;
						} else if(line.line == 0) {
							walked -= line.subline;
							line.subline = 0;
							break;
						}
						offset += line.subline + 1;
						walked -= line.subline + 1;
						--line.line;
						line.subline = (useCalculatedLayout ? self.numberOfSublinesOfLine(line.line, USE_CALCULATED_LAYOUT) : numberOfSublinesOfLine(line.line)) - 1;
					}
				}
				return walked;
			}

			/// @see presentation#IPresentationStylistListener
			void LineLayoutVector::presentationStylistChanged() {
				// TODO: This method is referred by noone???
				invalidate();
			}

			/**
			 * Updates the longest line.
			 * @param line The new longest line. Set @c boost#none to recalculate
			 * @param measure The measure (inline-progression-dimension) of the longest line in user units. If @a line
			 *                is @c boost#none, this value is ignored
			 */
			void LineLayoutVector::updateLongestLine(boost::optional<Index> line, Scalar measure) BOOST_NOEXCEPT {
				if(longestLine_ = line)
					maximumMeasure_ = measure;
				else {
					maximumMeasure_ = 0;
					BOOST_FOREACH(const NumberedLayout& layout, layouts_) {
						if(layout.layout->measure() > maximumMeasure_) {
							longestLine_ = layout.lineNumber;
							maximumMeasure_ = layout.layout->measure();
						}
					}
				}
			}


			// LineLayoutVector.NumberedLayout ////////////////////////////////////////////////////////////////////////

			LineLayoutVector::NumberedLayout::NumberedLayout() BOOST_NOEXCEPT {
			}

			LineLayoutVector::NumberedLayout::NumberedLayout(NumberedLayout&& other) BOOST_NOEXCEPT
					: lineNumber(other.lineNumber), layout(std::move(other.layout)) {
			}

			LineLayoutVector::NumberedLayout::~NumberedLayout() BOOST_NOEXCEPT {
			}
		}
	}
}
