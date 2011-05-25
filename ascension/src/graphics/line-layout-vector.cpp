/**
 * @file line-layout-vector.cpp
 * @author exeal
 * @date 2003-2006 was LineLayout.cpp
 * @date 2006-2011 was rendering.cpp
 * @date 2010-11-20 separated from ascension/layout.cpp
 * @date separated from rendering.cpp
 */

#include <ascension/graphics/line-layout-vector.hpp>

using namespace ascension;
using namespace ascension::graphics;
using namespace ascension::graphics::font;
using namespace std;
namespace k = ascension::kernel;


// LineLayoutVector ///////////////////////////////////////////////////////////////////////////////

/// Destructor.
LineLayoutVector::~LineLayoutVector() /*throw()*/ {
//	clearCaches(startLine_, startLine_ + bufferSize_, false);
	for(Iterator i(layouts_.begin()), e(layouts_.end()); i != e; ++i)
		delete i->second;
	document_.removePrenotifiedListener(*this);
	document_.removePartitioningListener(*this);
}

/**
 * Returns the layout of the specified line.
 * @param line The line number. This method does not throw an exception even if this paramter is
 *             out of bound
 * @return The layout
 * @see #at, atIfCached
 */
const TextLayout& LineLayoutVector::operator[](length_t line) const {
#ifdef ASCENSION_TRACE_LAYOUT_CACHES
	manah::win32::DumpContext dout;
	dout << "finding layout for line " << line;
#endif
	LineLayoutVector& self = *const_cast<LineLayoutVector*>(this);
	Iterator i(self.layouts_.begin());
	for(const Iterator e(self.layouts_.end()); i != e; ++i) {
		if(i->first == line)
			break;
	}

	if(i != layouts_.end()) {
#ifdef ASCENSION_TRACE_LAYOUT_CACHES
		dout << "... cache found\n";
#endif
		if(i->second != layouts_.front().second) {
			// bring to the top
			const LineLayout temp(*i);
			self.layouts_.erase(i);
			self.layouts_.push_front(temp);
			i = self.layouts_.begin();
		}
		return *i->second;
	} else {
#ifdef ASCENSION_TRACE_LAYOUT_CACHES
		dout << "... cache not found\n";
#endif
		if(layouts_.size() == bufferSize_) {
			// delete the last
			self.layouts_.pop_back();
			self.fireVisualLinesModified(i->first, i->first + 1,
				1, i->second->numberOfLines(), documentChangePhase_ == CHANGING);
			delete i->second;
		}
		const TextLayout* const layout = layoutGenerator_->generate(line).release();
		self.layouts_.push_front(make_pair(line, layout));
		self.fireVisualLinesModified(line, line + 1, layout->numberOfLines(), 1, documentChangePhase_ == CHANGING);
		return *layout;
	}
}

/**
 * Registers the visual lines listener.
 * @param listener The listener to be registered
 * @throw std#invalid_argument @a listener is already registered
 */
void LineLayoutVector::addVisualLinesListener(VisualLinesListener& listener) {
	listeners_.add(listener);
	const length_t lines = document_.numberOfLines();
	if(lines > 1)
		listener.visualLinesInserted(1, lines);
}

/**
 * Clears the layout caches of the specified lines. This method calls @c #layoutModified.
 * @param first The start of lines
 * @param last The end of lines (exclusive. this line will not be cleared)
 * @param repair Set @c true to recreate layouts for the lines. If @c true, this method calls
 *               @c #layoutModified. Otherwise calls @c #layoutDeleted
 * @throw std#invalid_argument @a first and/or @a last are invalid
 */
void LineLayoutVector::clearCaches(length_t first, length_t last, bool repair) {
	if(first > last /*|| last > viewer_.getDocument().getNumberOfLines()*/)
		throw invalid_argument("either line number is invalid.");
	if(documentChangePhase_ == ABOUT_TO_CHANGE) {
		pendingCacheClearance_.first = (pendingCacheClearance_.first == INVALID_INDEX) ? first : min(first, pendingCacheClearance_.first);
		pendingCacheClearance_.last = (pendingCacheClearance_.last == INVALID_INDEX) ? last : max(last, pendingCacheClearance_.last);
		return;
	}
	if(first == last)
		return;

//	const size_t originalSize = layouts_.size();
	length_t oldSublines = 0, cachedLines = 0;
	if(repair) {
		length_t newSublines = 0, actualFirst = last, actualLast = first;
		for(Iterator i(layouts_.begin()); i != layouts_.end(); ++i) {
			if(i->first >= first && i->first < last) {
				oldSublines += i->second->numberOfLines();
				delete i->second;
				auto_ptr<const TextLayout> newLayout(layoutGenerator_->generate(i->first));
				assert(newLayout.get() != 0);	// TODO:
				i->second = newLayout.release();
				newSublines += i->second->numberOfLines();
				++cachedLines;
				actualFirst = min(actualFirst, i->first);
				actualLast = max(actualLast, i->first);
			}
		}
		if(actualFirst == last)	// no lines cleared
			return;
		++actualLast;
		fireVisualLinesModified(actualFirst, actualLast, newSublines += actualLast - actualFirst - cachedLines,
			oldSublines += actualLast - actualFirst - cachedLines, documentChangePhase_ == CHANGING);
	} else {
		for(Iterator i(layouts_.begin()); i != layouts_.end(); ) {
			if(i->first >= first && i->first < last) {
				oldSublines += i->second->numberOfLines();
				delete i->second;
				i = layouts_.erase(i);
				++cachedLines;
			} else
				++i;
		}
		fireVisualLinesDeleted(first, last, oldSublines += last - first - cachedLines);
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
		const k::Region& region = change.erasedRegion();
		clearCaches(region.first.line + 1, region.second.line + 1, false);
		for(Iterator i(layouts_.begin()), e(layouts_.end()); i != e; ++i) {
			if(i->first > region.first.line)
				i->first -= region.second.line - region.first.line;	// $friendly-access
		}
	}
	if(change.insertedRegion().first.line != change.insertedRegion().second.line) {	// inserted text is multiline
		const k::Region& region = change.insertedRegion();
		for(Iterator i(layouts_.begin()), e(layouts_.end()); i != e; ++i) {
			if(i->first > region.first.line)
				i->first += region.second.line - region.first.line;	// $friendly-access
		}
		fireVisualLinesInserted(region.first.line + 1, region.second.line + 1);
	}
	const length_t firstLine = min(change.erasedRegion().first.line, change.insertedRegion().first.line);
	if(pendingCacheClearance_.first == INVALID_INDEX
			|| firstLine < pendingCacheClearance_.first || firstLine >= pendingCacheClearance_.last)
		invalidate(firstLine);
	documentChangePhase_ = NONE;
	if(pendingCacheClearance_.first != INVALID_INDEX) {
		clearCaches(pendingCacheClearance_.first, pendingCacheClearance_.last, autoRepair_);
		pendingCacheClearance_.first = pendingCacheClearance_.last = INVALID_INDEX;
	}
}

/// @see kernel#IDocumentPartitioningListener#documentPartitioningChanged
void LineLayoutVector::documentPartitioningChanged(const k::Region& changedRegion) {
	invalidate(changedRegion.beginning().line, changedRegion.end().line + 1);
}

void LineLayoutVector::fireVisualLinesDeleted(length_t first, length_t last, length_t sublines) {
	numberOfVisualLines_ -= sublines;
	const bool widthChanged = longestLine_ >= first && longestLine_ < last;
	if(widthChanged)
		updateLongestLine(static_cast<length_t>(-1), 0);
	listeners_.notify<length_t, length_t, length_t>(
		&VisualLinesListener::visualLinesDeleted, first, last, sublines, widthChanged);
}

void LineLayoutVector::fireVisualLinesInserted(length_t first, length_t last) /*throw()*/ {
	numberOfVisualLines_ += last - first;
	listeners_.notify<length_t, length_t>(&VisualLinesListener::visualLinesInserted, first, last);
}

void LineLayoutVector::fireVisualLinesModified(length_t first, length_t last,
		length_t newSublines, length_t oldSublines, bool documentChanged) /*throw()*/ {
	numberOfVisualLines_ += newSublines;
	numberOfVisualLines_ -= oldSublines;

	// update the longest line
	bool longestLineChanged = false;
	if(longestLine_ >= first && longestLine_ < last) {
		updateLongestLine(static_cast<length_t>(-1), 0);
		longestLineChanged = true;
	} else {
		length_t newLongestLine = longestLine_;
		Scalar newMaximumIpd = maximumInlineProgressionDimension();
		for(list<LineLayout>::const_iterator i(layouts_.begin()), e(layouts_.end()); i != e; ++i) {
			if(i->second->maximumInlineProgressionDimension() > newMaximumIpd) {
				newLongestLine = i->first;
				newMaximumIpd = i->second->maximumInlineProgressionDimension();
			}
		}
		if(longestLineChanged = (newLongestLine != longestLine_))
			updateLongestLine(newLongestLine, newMaximumIpd);
	}

	listeners_.notify<length_t, length_t, signed_length_t>(
		&VisualLinesListener::visualLinesModified, first, last,
		static_cast<signed_length_t>(newSublines) - static_cast<signed_length_t>(oldSublines), documentChanged, longestLineChanged);
}

/// @internal Only called by constructor.
void LineLayoutVector::initialize() /*throw()*/ {
	pendingCacheClearance_.first = pendingCacheClearance_.last = INVALID_INDEX;
	if(bufferSize_ == 0)
		throw invalid_argument("size of the buffer can't be zero.");
	document_.addPrenotifiedListener(*this);
	document_.addPartitioningListener(*this);
}

/// Invalidates all layouts.
void LineLayoutVector::invalidate() /*throw()*/ {
	clearCaches(0, document().numberOfLines(), autoRepair_);
}

/**
 * Invalidates the layouts of the specified lines.
 * @param first The start of the lines
 * @param last The end of the lines (exclusive. this line will not be cleared)
 * @throw std#invalid_argument @a first &gt;= @a last
 */
void LineLayoutVector::invalidate(length_t first, length_t last) {
	if(first >= last)
		throw invalid_argument("Any line number is invalid.");
	clearCaches(first, last, autoRepair_);
}

void LineLayoutVector::invalidate(const vector<length_t>& lines) {
	vector<length_t> cachedLines;
	for(list<LineLayout>::const_iterator i(layouts_.begin()), e(layouts_.end()); i != e; ++i)
		cachedLines.push_back(i->first);
	sort(cachedLines.begin(), cachedLines.end());

	typedef vector<length_t>::const_iterator Iterator;
	const vector<length_t>::const_iterator e(cachedLines.end());
	for(pair<Iterator, Iterator> p(mismatch<Iterator, Iterator>(cachedLines.begin(), e, lines.begin())); ; ) {
		p.first = find(p.first, e, *p.second);
		const pair<Iterator, Iterator> next(mismatch(p.first, e, p.second));
		if(next.second == lines.end()) {
			clearCaches(*p.second, lines.back() + 1, autoRepair_);
			break;
		}
		clearCaches(*p.second, *next.second, autoRepair_);
		p = next;
	}
}

/**
 * Resets the cached layout of the specified line and repairs if necessary.
 * @param line The line to invalidate layout
 */
inline void LineLayoutVector::invalidate(length_t line) {
	for(Iterator i(layouts_.begin()), e(layouts_.end()); i != e; ++i) {
		if(i->first == line) {
			const length_t oldSublines = i->second->numberOfLines();
			delete i->second;
			if(autoRepair_) {
				i->second = layoutGenerator_->generate(line).release();
				fireVisualLinesModified(line, line + 1,
					i->second->numberOfLines(), oldSublines, documentChangePhase_ == CHANGING);
			} else {
				layouts_.erase(i);
				fireVisualLinesModified(line, line + 1,
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
 * @throw kernel#BadPositionException @a line is outside of the document
 * @see #mapLogicalPositionToVisualPosition
 */
length_t LineLayoutVector::mapLogicalLineToVisualLine(length_t line) const {
	if(line >= document().numberOfLines())
		throw kernel::BadPositionException(kernel::Position(line, 0));
//	else if(!wrapLongLines())
//		return line;
	length_t result = 0, cachedLines = 0;
	for(list<LineLayout>::const_iterator i(layouts_.begin()), e(layouts_.end()); i != e; ++i) {
		if(i->first < line) {
			result += i->second->numberOfLines();
			++cachedLines;
		}
	}
	return result + line - cachedLines;
}

/**
 * Returns the visual line number and the visual column number of the specified logical position.
 * @param position The logical coordinates of the position to be mapped
 * @param[out] column The visual column of @a position. Can be @c null if not needed
 * @return The visual line of @a position
 * @throw kernel#BadPositionException @a position is outside of the document
 * @see #mapLogicalLineToVisualLine
 */
length_t LineLayoutVector::mapLogicalPositionToVisualPosition(const k::Position& position, length_t* column) const {
//	if(!wrapsLongLines()) {
//		if(column != 0)
//			*column = position.column;
//		return position.line;
//	}
	const TextLayout& layout = at(position.line);
	const length_t line = layout.lineAt(position.column);
	if(column != 0)
		*column = position.column - layout.lineOffset(line);
	return mapLogicalLineToVisualLine(position.line) + line;
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
length_t LineLayoutVector::mapVisualLineToLogicalLine(length_t line, length_t* subline) const {
	if(!getTextViewer().getConfiguration().lineWrap.wraps()) {
		if(subline != 0)
			*subline = 0;
		return line;
	}
	length_t c = getCacheFirstLine();
	for(length_t i = getCacheFirstLine(); ; ++i) {
		if(c + getNumberOfSublinesOfLine(i) > line) {
			if(subline != 0)
				*subline = line - c;
			return i;
		}
		c += getNumberOfSublinesOfLine(i);
	}
	assert(false);
	return getCacheLastLine();	// ここには来ない
}

/**
 * Returns the logical line number and the logical column number of the specified visual position.
 * @param position The visual coordinates of the position to be mapped
 * @return The logical coordinates of @a position
 * @throw kernel#BadPositionException @a position is outside of the document
 * @see #mapLogicalPositionToVisualPosition, #mapVisualLineToLogicalLine
 */
k::Position LineLayoutVector::mapVisualPositionToLogicalPosition(const k::Position& position) const {
	if(!getTextViewer().getConfiguration().lineWrap.wraps())
		return position;
	k::Position result;
	length_t subline;
	result.line = mapVisualLineToLogicalLine(position.line, &subline);
	result.column = getLineLayout(result.line).getSublineOffset(subline) + position.column;
	return result;
}
#endif // 0

/**
 * Offsets visual line.
 * @param[in,out] line The logical line
 * @param[in,out] subline The visual subline
 * @param[in] offset The offset
 * @param[out] overflowedOrUnderflowed @c true if absolute value of @a offset is too large so that
 *                                     the results were snapped to the beginning or the end of the
 *                                     document. Optional
 */
void LineLayoutVector::offsetVisualLine(length_t& line, length_t& subline,
		signed_length_t offset, bool* overflowedOrUnderflowed) const /*throw()*/ {
	if(offset > 0) {
		if(subline + offset < numberOfSublinesOfLine(line))
			subline += offset;
		else {
			const length_t lines = document().numberOfLines();
			offset -= static_cast<signed_length_t>(numberOfSublinesOfLine(line) - subline) - 1;
			while(offset > 0 && line < lines - 1)
				offset -= static_cast<signed_length_t>(numberOfSublinesOfLine(++line));
			subline = numberOfSublinesOfLine(line) - 1;
			if(offset < 0)
				subline += offset;
			if(overflowedOrUnderflowed != 0)
				*overflowedOrUnderflowed = offset > 0;
		}
	} else if(offset < 0) {
		if(static_cast<length_t>(-offset) <= subline)
			subline += offset;
		else {
			offset += static_cast<signed_length_t>(subline);
			while(offset < 0 && line > 0)
				offset += static_cast<signed_length_t>(numberOfSublinesOfLine(--line));
			subline = (offset > 0) ? offset : 0;
			if(overflowedOrUnderflowed != 0)
				*overflowedOrUnderflowed = offset > 0;
		}
	}
}

/// @see presentation#IPresentationStylistListener
void LineLayoutVector::presentationStylistChanged() {
	invalidate();
}

/**
 * Updates the longest line.
 * @param line The new longest line. set -1 to recalculate
 * @param ipd The inline progression dimension of the longest line. If @a line is -1, this value is
 *            ignored
 */
void LineLayoutVector::updateLongestLine(length_t line, Scalar ipd) /*throw()*/ {
	if(line != -1) {
		longestLine_ = line;
		maximumIpd_ = ipd;
	} else {
		longestLine_ = static_cast<length_t>(-1);
		maximumIpd_ = 0;
		for(list<LineLayout>::const_iterator i(layouts_.begin()), e(layouts_.end()); i != e; ++i) {
			if(i->second->maximumInlineProgressionDimension() > maximumIpd_) {
				longestLine_ = i->first;
				maximumIpd_ = i->second->maximumInlineProgressionDimension();
			}
		}
	}
}
