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
#include <boost/foreach.hpp>
#include <boost/range/algorithm/sort.hpp>
#ifdef _DEBUG
#	include <boost/log/trivial.hpp>
#endif

namespace ascension {
	namespace graphics {
		namespace font {
			// LineLayoutVector ///////////////////////////////////////////////////////////////////////////////////////

			const LineLayoutVector::UseCalculatedLayoutTag LineLayoutVector::USE_CALCULATED_LAYOUT;

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
				BOOST_LOG_TRIVIAL(debug) << "finding layout for line " << line;
#endif
				LineLayoutVector& self = *const_cast<LineLayoutVector*>(this);
				auto i(std::begin(self.layouts_));
				for(const auto e(end(self.layouts_)); i != e; ++i) {
					if(i->lineNumber == line)
						break;
				}

				if(i != end(layouts_)) {
#if defined(ASCENSION_TRACE_LAYOUT_CACHES) && defined(_DEBUG)
					BOOST_LOG_TRIVIAL(debug) << "... cache found\n";
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
					if(layouts_.size() == bufferSize_) {
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
			 * Clears the layout caches of the specified lines. This method calls @c #layoutModified.
			 * @param lines The range of lines. @a lines.end() is exclusive and will not be cleared
			 * @param repair Set @c true to recreate layouts for the lines. If @c true, this method calls
			 *               @c #layoutModified. Otherwise calls @c #layoutDeleted
			 */
			void LineLayoutVector::clearCaches(const boost::integer_range<Index>& lines, bool repair) {
				const boost::integer_range<Index> orderedLines(lines | adaptors::ordered());
				if(false /*|| *orderedLines.end() > document().numberOfLines()*/)
					throw IndexOutOfBoundsException("lines");
				if(documentChangePhase_ == ABOUT_TO_CHANGE) {
					pendingCacheClearance_ = boost::irange(
						!pendingCacheClearance_ ?
							*boost::begin(orderedLines) : std::min(*boost::begin(orderedLines), *boost::begin(boost::get(pendingCacheClearance_))),
						!pendingCacheClearance_ ?
							*orderedLines.end() : std::max(*boost::end(orderedLines), *boost::end(boost::get(pendingCacheClearance_))));
					return;
				}
				if(orderedLines.empty())
					return;

//				const std::size_t originalSize = layouts_.size();
				Index oldSublines = 0, cachedLines = 0;
				if(repair) {
					Index newSublines = 0, actualFirst = *boost::end(orderedLines), actualLast = *boost::begin(orderedLines);
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
					if(actualFirst == *boost::end(orderedLines))	// no lines cleared
						return;
					++actualLast;
					fireVisualLinesModified(boost::irange(actualFirst, actualLast), newSublines += actualLast - actualFirst - cachedLines,
						oldSublines += actualLast - actualFirst - cachedLines, documentChangePhase_ == CHANGING);
				} else {
					for(auto i(std::begin(layouts_)); i != std::end(layouts_); ) {
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

			/// @see kernel#DocumentListener#documentAboutToBeChanged
			void LineLayoutVector::documentAboutToBeChanged(const kernel::Document&) {
				documentChangePhase_ = ABOUT_TO_CHANGE;
			}

			/// @see kernel#DocumentListener#documentChanged
			void LineLayoutVector::documentChanged(const kernel::Document&, const kernel::DocumentChange& change) {
				documentChangePhase_ = CHANGING;
				assert(change.erasedRegion().isNormalized() && change.insertedRegion().isNormalized());
				if(change.erasedRegion().first.line != change.erasedRegion().second.line) {	// erased region includes newline(s)
					const kernel::Region& region = change.erasedRegion();
					clearCaches(boost::irange(region.first.line + 1, region.second.line + 1), false);
					BOOST_FOREACH(NumberedLayout& layout, layouts_) {
						if(layout.lineNumber > region.first.line)
							layout.lineNumber -= region.second.line - region.first.line;	// $friendly-access
					}
				}
				if(change.insertedRegion().first.line != change.insertedRegion().second.line) {	// inserted text is multiline
					const kernel::Region& region = change.insertedRegion();
					BOOST_FOREACH(NumberedLayout& layout, layouts_) {
						if(layout.lineNumber > region.first.line)
							layout.lineNumber += region.second.line - region.first.line;	// $friendly-access
					}
					fireVisualLinesInserted(boost::irange(region.first.line + 1, region.second.line + 1));
				}
				const Index firstLine = std::min(change.erasedRegion().first.line, change.insertedRegion().first.line);
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
				invalidate(boost::irange(changedRegion.beginning().line, changedRegion.end().line + 1));
			}

			void LineLayoutVector::fireVisualLinesDeleted(const boost::integer_range<Index>& lines, Index sublines) {
				numberOfVisualLines_ -= sublines;
				const bool measureChanged = longestLine_ == boost::none || includes(lines, boost::get(longestLine_));
				if(measureChanged)
					updateLongestLine(boost::none, 0);
				listeners_.notify<const boost::integer_range<Index>&, Index>(
					&VisualLinesListener::visualLinesDeleted, lines, sublines, measureChanged);
			}

			void LineLayoutVector::fireVisualLinesInserted(const boost::integer_range<Index>& lines) BOOST_NOEXCEPT {
				numberOfVisualLines_ += lines.size();
				listeners_.notify<const boost::integer_range<Index>&>(&VisualLinesListener::visualLinesInserted, lines);
			}

			void LineLayoutVector::fireVisualLinesModified(const boost::integer_range<Index>& lines,
					Index newSublines, Index oldSublines, bool documentChanged) BOOST_NOEXCEPT {
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
			void LineLayoutVector::initialize() BOOST_NOEXCEPT {
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
							i->layout = layoutGenerator_->generate(line);
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
//						*offsetInVisualLine = position.offsetInLine;
//					return position.line;
//				}
				try {
					const TextLayout* const layout = at(position.line);
					const Index line = (layout != nullptr) ? layout->lineAt(position.offsetInLine) : 0;
					if(offsetInVisualLine != nullptr)
						*offsetInVisualLine = position.offsetInLine - ((layout != nullptr) ? layout->lineOffset(line) : 0);
					return mapLogicalLineToVisualLine(position.line) + line;
				} catch(const IndexOutOfBoundsException&) {
					throw kernel::BadPositionException(position);
				}
			}

#if 0
			/**
			 * Returns the logical line number and the visual subline number of the specified visual line.
			 * @param line The visual line
			 * @param[out] subline The visual subline of @a line. can be @c null if not needed
			 * @return The logical line
			 * @throw kernel#BadPositionException @a line is outside of the document
			 * @see #mapLogicalLineToVisualLine, #mapVisualPositionToLogicalPosition
			 */
			Index LineLayoutVector::mapVisualLineToLogicalLine(Index line, Index* subline) const {
				if(!getTextViewer().getConfiguration().lineWrap.wraps()) {
					if(subline != nullptr)
						*subline = 0;
					return line;
				}
				Index c = getCacheFirstLine();
				for(Index i = getCacheFirstLine(); ; ++i) {
					if(c + getNumberOfSublinesOfLine(i) > line) {
						if(subline != nullptr)
							*subline = line - c;
						return i;
					}
					c += getNumberOfSublinesOfLine(i);
				}
				assert(false);
				return getCacheLastLine();	// ここには来ない
			}

			/**
			 * Returns the logical line number and the offset in the logical line of the specified visual
			 * position.
			 * @param position The visual coordinates of the position to be mapped
			 * @return The logical coordinates of @a position
			 * @throw kernel#BadPositionException @a position is outside of the document
			 * @see #mapLogicalPositionToVisualPosition, #mapVisualLineToLogicalLine
			 */
			kernel::Position LineLayoutVector::mapVisualPositionToLogicalPosition(const kernel::Position& position) const {
				if(!getTextViewer().getConfiguration().lineWrap.wraps())
					return position;
				kernel::Position result;
				Index subline;
				result.line = mapVisualLineToLogicalLine(position.line, &subline);
				result.offsetInLine = getLineLayout(result.line).getSublineOffset(subline) + position.offsetInLine;
				return result;
			}
#endif // 0
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
		}
	}
}
