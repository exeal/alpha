/**
 * @file line-layout-vector.cpp
 * @author exeal
 * @date 2003-2006 was LineLayout.cpp
 * @date 2006-2011 was rendering.cpp
 * @date 2010-11-20 separated from ascension/layout.cpp
 * @date 2011-??-?? separated from rendering.cpp
 * @date 2011-2012
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
const TextLayout& LineLayoutVector::operator[](Index line) const {
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
			self.fireVisualLinesModified(makeRange(i->first, i->first + 1),
				1, i->second->numberOfLines(), documentChangePhase_ == CHANGING);
			delete i->second;
		}
		const TextLayout* const layout = layoutGenerator_->generate(line).release();
		self.layouts_.push_front(make_pair(line, layout));
		self.fireVisualLinesModified(makeRange(line, line + 1), layout->numberOfLines(), 1, documentChangePhase_ == CHANGING);
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
	const Index lines = document_.numberOfLines();
	if(lines > 1)
		listener.visualLinesInserted(makeRange<Index>(1, lines));
}

/**
 * Clears the layout caches of the specified lines. This method calls @c #layoutModified.
 * @param lines The range of lines. @a lines.end() is exclusive and will not be cleared
 * @param repair Set @c true to recreate layouts for the lines. If @c true, this method calls
 *               @c #layoutModified. Otherwise calls @c #layoutDeleted
 * @throw std#invalid_argument @a first and/or @a last are invalid
 */
void LineLayoutVector::clearCaches(const Range<Index>& lines, bool repair) {
	if(false /*|| lines.end() > viewer_.document().numberOfLines()*/)
		throw invalid_argument("either line number is invalid.");
	if(documentChangePhase_ == ABOUT_TO_CHANGE) {
		pendingCacheClearance_ = makeRange(
			!pendingCacheClearance_ ?
				lines.beginning() : min(lines.beginning(), pendingCacheClearance_->beginning()),
			!pendingCacheClearance_ ?
				lines.end() : max(lines.end(), pendingCacheClearance_->end()));
		return;
	}
	if(isEmpty(lines))
		return;

//	const size_t originalSize = layouts_.size();
	Index oldSublines = 0, cachedLines = 0;
	if(repair) {
		Index newSublines = 0, actualFirst = lines.end(), actualLast = lines.beginning();
		for(Iterator i(layouts_.begin()); i != layouts_.end(); ++i) {
			if(includes(lines, i->first)) {
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
		if(actualFirst == lines.end())	// no lines cleared
			return;
		++actualLast;
		fireVisualLinesModified(makeRange(actualFirst, actualLast), newSublines += actualLast - actualFirst - cachedLines,
			oldSublines += actualLast - actualFirst - cachedLines, documentChangePhase_ == CHANGING);
	} else {
		for(Iterator i(layouts_.begin()); i != layouts_.end(); ) {
			if(includes(lines, i->first)) {
				oldSublines += i->second->numberOfLines();
				delete i->second;
				i = layouts_.erase(i);
				++cachedLines;
			} else
				++i;
		}
		fireVisualLinesDeleted(lines, oldSublines += length(lines) - cachedLines);
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
		clearCaches(makeRange(region.first.line + 1, region.second.line + 1), false);
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
		fireVisualLinesInserted(makeRange(region.first.line + 1, region.second.line + 1));
	}
	const Index firstLine = min(change.erasedRegion().first.line, change.insertedRegion().first.line);
	if(!pendingCacheClearance_ || !includes(*pendingCacheClearance_, firstLine))
		invalidate(firstLine);
	documentChangePhase_ = NONE;
	if(pendingCacheClearance_) {
		clearCaches(*pendingCacheClearance_, autoRepair_);
		pendingCacheClearance_ = boost::none;
	}
}

/// @see kernel#IDocumentPartitioningListener#documentPartitioningChanged
void LineLayoutVector::documentPartitioningChanged(const k::Region& changedRegion) {
	invalidate(makeRange(changedRegion.beginning().line, changedRegion.end().line + 1));
}

void LineLayoutVector::fireVisualLinesDeleted(const Range<Index>& lines, Index sublines) {
	numberOfVisualLines_ -= sublines;
	const bool widthChanged = includes(lines, *longestLine_);
	if(widthChanged)
		updateLongestLine(static_cast<Index>(-1), 0);
	listeners_.notify<const Range<Index>&, Index>(
		&VisualLinesListener::visualLinesDeleted, lines, sublines, widthChanged);
}

void LineLayoutVector::fireVisualLinesInserted(const Range<Index>& lines) /*throw()*/ {
	numberOfVisualLines_ += length(lines);
	listeners_.notify<const Range<Index>&>(&VisualLinesListener::visualLinesInserted, lines);
}

void LineLayoutVector::fireVisualLinesModified(const Range<Index>& lines,
		Index newSublines, Index oldSublines, bool documentChanged) /*throw()*/ {
	numberOfVisualLines_ += newSublines;
	numberOfVisualLines_ -= oldSublines;

	// update the longest line
	bool longestLineChanged = false;
	if(includes(lines, *longestLine_)) {
		updateLongestLine(static_cast<Index>(-1), 0);
		longestLineChanged = true;
	} else {
		Index newLongestLine = *longestLine_;
		Scalar newMaximumIpd = maximumMeasure();
		for(list<LineLayout>::const_iterator i(layouts_.begin()), e(layouts_.end()); i != e; ++i) {
			if(i->second->measure() > newMaximumIpd) {
				newLongestLine = i->first;
				newMaximumIpd = i->second->measure();
			}
		}
		if(longestLineChanged = (newLongestLine != longestLine_))
			updateLongestLine(newLongestLine, newMaximumIpd);
	}

	listeners_.notify<const Range<Index>&, SignedIndex>(
		&VisualLinesListener::visualLinesModified, lines,
		static_cast<SignedIndex>(newSublines) - static_cast<SignedIndex>(oldSublines),
		documentChanged, longestLineChanged);
}

/// @internal Only called by constructor.
void LineLayoutVector::initialize() /*throw()*/ {
	pendingCacheClearance_ = boost::none;
	if(bufferSize_ == 0)
		throw invalid_argument("size of the buffer can't be zero.");
	document_.addPrenotifiedListener(*this);
	document_.addPartitioningListener(*this);
}

/// Invalidates all layouts.
void LineLayoutVector::invalidate() /*throw()*/ {
	clearCaches(makeRange<Index>(0, document().numberOfLines()), autoRepair_);
}

/**
 * Invalidates the layouts of the specified lines.
 * @param lines The range of the lines. @a lines.end() is exclusive and will not be cleared
 */
void LineLayoutVector::invalidate(const Range<Index>& lines) {
	clearCaches(lines, autoRepair_);
}

void LineLayoutVector::invalidate(const vector<Index>& lines) {
	vector<Index> cachedLines;
	for(list<LineLayout>::const_iterator i(layouts_.begin()), e(layouts_.end()); i != e; ++i)
		cachedLines.push_back(i->first);
	sort(cachedLines.begin(), cachedLines.end());

	typedef vector<Index>::const_iterator Iterator;
	const vector<Index>::const_iterator e(cachedLines.end());
	for(pair<Iterator, Iterator> p(mismatch<Iterator, Iterator>(cachedLines.begin(), e, lines.begin())); ; ) {
		p.first = find(p.first, e, *p.second);
		const pair<Iterator, Iterator> next(mismatch(p.first, e, p.second));
		if(next.second == lines.end()) {
			clearCaches(makeRange(*p.second, lines.back() + 1), autoRepair_);
			break;
		}
		clearCaches(makeRange(*p.second, *next.second), autoRepair_);
		p = next;
	}
}

/**
 * Resets the cached layout of the specified line and repairs if necessary.
 * @param line The line to invalidate layout
 */
inline void LineLayoutVector::invalidate(Index line) {
	for(Iterator i(layouts_.begin()), e(layouts_.end()); i != e; ++i) {
		if(i->first == line) {
			const Index oldSublines = i->second->numberOfLines();
			delete i->second;
			if(autoRepair_) {
				i->second = layoutGenerator_->generate(line).release();
				fireVisualLinesModified(makeRange(line, line + 1),
					i->second->numberOfLines(), oldSublines, documentChangePhase_ == CHANGING);
			} else {
				layouts_.erase(i);
				fireVisualLinesModified(makeRange(line, line + 1),
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
Index LineLayoutVector::mapLogicalLineToVisualLine(Index line) const {
	if(line >= document().numberOfLines())
		throw kernel::BadPositionException(kernel::Position(line, 0));
//	else if(!wrapLongLines())
//		return line;
	Index result = 0, cachedLines = 0;
	for(list<LineLayout>::const_iterator i(layouts_.begin()), e(layouts_.end()); i != e; ++i) {
		if(i->first < line) {
			result += i->second->numberOfLines();
			++cachedLines;
		}
	}
	return result + line - cachedLines;
}

/**
 * Returns the visual line number and the offset in the visual line of the specified logical
 * position.
 * @param position The logical coordinates of the position to be mapped
 * @param[out] offsetInVisualLine The offset of @a position in visual line. Can be @c null if not
 *                                needed
 * @return The visual line of @a position
 * @throw kernel#BadPositionException @a position is outside of the document
 * @see #mapLogicalLineToVisualLine
 */
Index LineLayoutVector::mapLogicalPositionToVisualPosition(const k::Position& position, Index* offsetInVisualLine) const {
//	if(!wrapsLongLines()) {
//		if(offsetInVisualLine != 0)
//			*offsetInVisualLine = position.offsetInLine;
//		return position.line;
//	}
	const TextLayout& layout = at(position.line);
	const Index line = layout.lineAt(position.offsetInLine);
	if(offsetInVisualLine != 0)
		*offsetInVisualLine = position.offsetInLine - layout.lineOffset(line);
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
Index LineLayoutVector::mapVisualLineToLogicalLine(Index line, Index* subline) const {
	if(!getTextViewer().getConfiguration().lineWrap.wraps()) {
		if(subline != 0)
			*subline = 0;
		return line;
	}
	Index c = getCacheFirstLine();
	for(Index i = getCacheFirstLine(); ; ++i) {
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
 * Returns the logical line number and the offset in the logical line of the specified visual
 * position.
 * @param position The visual coordinates of the position to be mapped
 * @return The logical coordinates of @a position
 * @throw kernel#BadPositionException @a position is outside of the document
 * @see #mapLogicalPositionToVisualPosition, #mapVisualLineToLogicalLine
 */
k::Position LineLayoutVector::mapVisualPositionToLogicalPosition(const k::Position& position) const {
	if(!getTextViewer().getConfiguration().lineWrap.wraps())
		return position;
	k::Position result;
	Index subline;
	result.line = mapVisualLineToLogicalLine(position.line, &subline);
	result.offsetInLine = getLineLayout(result.line).getSublineOffset(subline) + position.offsetInLine;
	return result;
}
#endif // 0

/**
 * Offsets visual line. The line whose layout not calculated is treat as single visual line.
 * @param[in,out] line The visual line
 * @param[in] offset The offset
 * @return @c false if absolute value of @a offset is too large so that the results were snapped to
 *         the beginning or the end of the document
 */
bool LineLayoutVector::offsetVisualLine(VisualLine& line, SignedIndex offset) const /*throw()*/ {
	bool overflowedOrUnderflowed = false;
	if(offset > 0) {
		if(line.subline + offset < numberOfSublinesOfLine(line.line))
			line.subline += offset;
		else {
			const Index lines = document().numberOfLines();
			offset -= static_cast<SignedIndex>(numberOfSublinesOfLine(line.line) - line.subline) - 1;
			while(offset > 0 && line.line < lines - 1)
				offset -= static_cast<SignedIndex>(numberOfSublinesOfLine(++line.line));
			line.subline = numberOfSublinesOfLine(line.line) - 1;
			if(offset < 0)
				line.subline += offset;
			overflowedOrUnderflowed = offset > 0;
		}
	} else if(offset < 0) {
		if(static_cast<Index>(-offset) <= line.subline)
			line.subline += offset;
		else {
			offset += static_cast<SignedIndex>(line.subline);
			while(offset < 0 && line.line > 0)
				offset += static_cast<SignedIndex>(numberOfSublinesOfLine(--line.line));
			line.subline = (offset > 0) ? offset : 0;
			overflowedOrUnderflowed = offset > 0;
		}
	}
	return !overflowedOrUnderflowed;
}

/// @see presentation#IPresentationStylistListener
void LineLayoutVector::presentationStylistChanged() {
	invalidate();
}

/**
 * Updates the longest line.
 * @param line The new longest line. set -1 to recalculate
 * @param measure The measure (inline-progression-dimension) of the longest line. If @a line is -1,
 *                this value is ignored
 */
void LineLayoutVector::updateLongestLine(Index line, Scalar measure) /*throw()*/ {
	if(line != -1) {
		longestLine_ = line;
		maximumMeasure_ = measure;
	} else {
		longestLine_ = boost::none;
		maximumMeasure_ = 0;
		for(list<LineLayout>::const_iterator i(layouts_.begin()), e(layouts_.end()); i != e; ++i) {
			if(i->second->measure() > maximumMeasure_) {
				longestLine_ = i->first;
				maximumMeasure_ = i->second->measure();
			}
		}
	}
}
