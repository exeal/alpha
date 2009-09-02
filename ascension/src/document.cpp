/**
 * @file document.cpp
 * @author exeal
 * @date 2003-2006 (was EditDoc.h)
 * @date 2006-2009
 */

#include <ascension/document.hpp>
#include <ascension/point.hpp>
#include <stack>
#include <algorithm>
#include <limits>	// std.numeric_limits

using namespace ascension;
using namespace ascension::kernel;
using namespace ascension::text;
using namespace std;
using manah::GapBuffer;


namespace {
	inline Newline resolveNewline(const Document& document, Newline newline) {
		if(newline == NLF_DOCUMENT_INPUT) {
			// fallback
			newline = (document.input() != 0) ? document.input()->newline() : ASCENSION_DEFAULT_NEWLINE;
			assert(isLiteralNewline(newline));
		}
		return newline;
	}
} // namespace @0

const Direction Direction::FORWARD(true);
const Direction Direction::BACKWARD(false);

const Position Position::ZERO_POSITION(0, 0);
const Position Position::INVALID_POSITION(INVALID_INDEX, INVALID_INDEX);


// kernel free functions ////////////////////////////////////////////////////

/**
 * Writes the content of the document to the specified output stream.
 * <p>This method does not write Unicode byte order mark.</p>
 * <p>This method explicitly flushes the output stream.</p>
 * @param out the output stream
 * @param document the document
 * @param region the region to be written (this region is not restricted with narrowing)
 * @param newline the newline representation
 * @return @a out
 * @throw UnknownValueException @a newline is invalid
 * @throw ... any exceptions out.operator bool, out.write and out.flush throw
 * @see getNewlineString, Document#insert
 */
basic_ostream<Char>& kernel::writeDocumentToStream(basic_ostream<Char>& out,
		const Document& document, const Region& region, Newline newline /* = NLF_RAW_VALUE */) {
	const Position& beginning = region.beginning();
	const Position end = min(region.end(), document.region().second);
	if(beginning.line == end.line) {	// shortcut for single-line
		if(out) {
			// TODO: this cast may be danger.
			out.write(document.line(end.line).data() + beginning.column, static_cast<streamsize>(end.column - beginning.column));
		}
	} else {
		newline = resolveNewline(document, newline);
		const String eol(isLiteralNewline(newline) ? newlineString(newline) : L"");
		if(eol.empty() && newline != NLF_RAW_VALUE)
			throw UnknownValueException("newline");
		for(length_t i = beginning.line; out; ++i) {
			const Document::Line& line = document.getLineInformation(i);
			const length_t first = (i == beginning.line) ? beginning.column : 0;
			const length_t last = (i == end.line) ? end.column : line.text().length();
			out.write(line.text().data() + first, static_cast<streamsize>(last - first));
			if(i == end.line)
				break;
			if(newline == NLF_RAW_VALUE)
				out.write(newlineString(line.newline()), static_cast<streamsize>(newlineStringLength(line.newline())));
			else
				out.write(eol.data(), static_cast<streamsize>(eol.length()));
		}
	}
	return out.flush();
}


// kernel.positions free functions //////////////////////////////////////////

/**
 * Returns absolute character offset of the specified position from the start of the document.
 * @param document the document
 * @param at the position
 * @param fromAccessibleStart
 * @throw BadPositionException @a at is outside of the document
 */
length_t positions::absoluteOffset(const Document& document, const Position& at, bool fromAccessibleStart) {
	if(at > document.region().second)
		throw BadPositionException(at);
	length_t offset = 0;
	const Position start((fromAccessibleStart ? document.accessibleRegion() : document.region()).first);
	for(length_t line = start.line; ; ++line) {
		if(line == at.line) {
			offset += at.column;
			break;
		} else {
			offset += document.lineLength(line) + 1;	// +1 is for a newline character
			if(line == start.line)
				offset -= start.column;
		}
	}
	return offset;
}

/**
 * Adapts the specified position to the document change.
 * @param position the original position
 * @param change the content of the document change
 * @param gravity the gravity which determines the direction to which the position should move if
 * a text was inserted at the position. if @c FORWARD is specified, the position will move to the
 * start of the inserted text (no movement occur). Otherwise, move to the end of the inserted text
 * @return the result position
 */
Position positions::updatePosition(const Position& position, const DocumentChange& change, Direction gravity) /*throw()*/ {
	Position newPosition(position);
	if(!change.erasedRegion().isEmpty()) {	// deletion
		if(position < change.erasedRegion().second) {	// the end is behind the current line
			if(position <= change.erasedRegion().first)
				return newPosition;
			else	// in the region
				newPosition = change.erasedRegion().first;
		} else if(position.line > change.erasedRegion().second.line)	// in front of the current line
			newPosition.line -= change.erasedRegion().second.line - change.erasedRegion().first.line;
		else {	// the end is the current line
			if(position.line == change.erasedRegion().first.line)	// the region is single-line
				newPosition.column -= change.erasedRegion().second.column - change.erasedRegion().first.column;
			else {	// the region is multiline
				newPosition.line -= change.erasedRegion().second.line - change.erasedRegion().first.line;
				newPosition.column -= change.erasedRegion().second.column - change.erasedRegion().first.column;
			}
		}
	}
	if(!change.insertedRegion().isEmpty()) {	// insertion
		if(position < change.insertedRegion().first)	// behind the current position
			return newPosition;
		else if(position == change.insertedRegion().first && gravity == Direction::BACKWARD) // the current position + backward gravity
			return newPosition;
		else if(position.line > change.insertedRegion().first.line)	// in front of the current line
			newPosition.line += change.insertedRegion().second.line - change.insertedRegion().first.line;
		else {	// in the current line
			newPosition.line += change.insertedRegion().second.line - change.insertedRegion().first.line;
			newPosition.column += change.insertedRegion().second.column - change.insertedRegion().first.column;
		}
	}
	return newPosition;
}


namespace {
#ifdef _DEBUG
	// for Document.length_ diagnostic
	length_t calculateDocumentLength(const Document& document) {
		length_t c = 0;
		const length_t lines = document.numberOfLines();
		for(length_t i = 0; i < lines; ++i)
			c += document.lineLength(i);
		return c;
	}
#endif /* _DEBUG */
} // namespace @0


// exception classes ////////////////////////////////////////////////////////

/// Protected default constructor.
DocumentCantChangeException::DocumentCantChangeException() {
}

/// Destructor.
DocumentCantChangeException::~DocumentCantChangeException() {
}

/// Default constructor.
ReadOnlyDocumentException::ReadOnlyDocumentException() :
	IllegalStateException("The document is readonly. Any edit process is denied.") {
}

/// Default constructor.
DocumentAccessViolationException::DocumentAccessViolationException() :
	invalid_argument("The specified position or region is inaccessible.") {
}

/**
 * Constructor.
 * @param requested the requested position in the document
 */
BadPositionException::BadPositionException(const Position& requested) : invalid_argument(
		static_cast<ostringstream&>(ostringstream() << "the position " << requested
		<< " is outside of the document.").str()), requestedPosition_(requested) {
}

/**
 * Constructor.
 * @param requested the requested position in the document
 * @param message the message @c #what returns
 */
BadPositionException::BadPositionException(const Position& requested,
		const string& message) : invalid_argument(message), requestedPosition_(requested) {
}

/// Returns the requested position in the document.
const Position& BadPositionException::requestedPosition() const /*throw()*/ {
	return requestedPosition_;
}

/**
 * Constructor.
 * @param requested the requested region in the document
 * @param message the exception message
 */
BadRegionException::BadRegionException(const Region& requested) : invalid_argument(
		static_cast<ostringstream&>(ostringstream() << "the region " << requested
		<< ") intersects with the outside of the document.").str()), requestedRegion_(requested) {
}

/**
 * Constructor.
 * @param requested the requested region in the document
 * @param message the exception message
 */
BadRegionException::BadRegionException(const Region& requested,
		const string& message) : invalid_argument(message), requestedRegion_(requested) {
}

/// Returns the requested region in the document.
const Region& BadRegionException::requestedRegion() const /*throw()*/ {
	return requestedRegion_;
}

/// Constructor.
IDocumentInput::ChangeRejectedException::ChangeRejectedException() {
}


// DocumentChange ///////////////////////////////////////////////////////////

/**
 * Private constructor.
 * @param erasedRegion the erased region in the change
 * @param insertedRegion the inserted region in the change
 */
DocumentChange::DocumentChange(const Region& erasedRegion, const Region& insertedRegion)
		/*throw()*/ : erasedRegion_(erasedRegion), insertedRegion_(insertedRegion) {
	const_cast<Region&>(erasedRegion_).normalize();
	const_cast<Region&>(erasedRegion_).normalize();
}

/// Private destructor.
DocumentChange::~DocumentChange() /*throw()*/ {
}


// Bookmarker ///////////////////////////////////////////////////////////////

/**
 * Private constructor.
 * @param document the document
 */
Bookmarker::Bookmarker(Document& document) /*throw()*/ : document_(document) {
	document.addListener(*this);
}

/// Destructor.
Bookmarker::~Bookmarker() /*throw()*/ {
	document_.removeListener(*this);
}

/**
 * Registers the listener.
 * @param listener the listener to be registered
 * @throw std#invalid_argument @a listener is already registered
 */
void Bookmarker::addListener(IBookmarkListener& listener) {
	listeners_.add(listener);
}

/**
 * Returns a bidirectional iterator to addresses the first marked line.
 * @see #end, #next
 */
Bookmarker::Iterator Bookmarker::begin() const {
	return Iterator(markedLines_.begin());
}

/// Deletes all bookmarks.
void Bookmarker::clear() /*throw()*/ {
	if(!markedLines_.empty()) {
		markedLines_.clear();
		listeners_.notify(&IBookmarkListener::bookmarkCleared);
	}
}

/// @see IDocumentListener#documentAboutToBeChanged
void Bookmarker::documentAboutToBeChanged(const Document&) {
	// do nothing
}

/// @see IDocumentListener#documentChanged
void Bookmarker::documentChanged(const Document& document, const DocumentChange& change) {
	// update markedLines_ based on the change
	if(&document_ != &document || markedLines_.empty())
		return;
	if(change.erasedRegion().first.line != change.erasedRegion().second.line) {
		// remove the marks on the deleted lines
		const length_t lines = change.erasedRegion().second.line - change.erasedRegion().first.line;
		const GapBuffer<length_t>::Iterator e(markedLines_.end());
		GapBuffer<length_t>::Iterator top(find(change.erasedRegion().first.line));
		if(top != e) {
			if(*top == change.erasedRegion().first.line)
				++top;
			GapBuffer<length_t>::Iterator bottom(find(change.erasedRegion().second.line));
			if(bottom != e && *bottom == change.erasedRegion().second.line)
				++bottom;
			// slide the following lines before removing
			if(bottom != e) {
				for(GapBuffer<length_t>::Iterator i(bottom); i != e; ++i)
					*i -= lines;
			}
			markedLines_.erase(top, bottom);	// GapBuffer<>.erase does not return an iterator
		}
	}
	if(change.insertedRegion().first.line != change.insertedRegion().second.line) {
		const length_t lines = change.insertedRegion().second.line - change.insertedRegion().first.line;
		GapBuffer<length_t>::Iterator i(find(change.insertedRegion().first.line));
		if(i != markedLines_.end()) {
			if(*i == change.insertedRegion().first.line && change.insertedRegion().first.column != 0)
				++i;
			for(const GapBuffer<length_t>::Iterator e(markedLines_.end()); i != e; ++i)
				*i += lines;
		}
	}
}

/**
 * Returns a bidirectional iterator to addresses just beyond the last marked line.
 * @see #begin, #next
 */
Bookmarker::Iterator Bookmarker::end() const {
	return Iterator(markedLines_.end());
}

inline GapBuffer<length_t>::Iterator Bookmarker::find(length_t line) const /*throw()*/ {
	// TODO: can write faster implementation (and design) by internal.searchBound().
	Bookmarker& self = const_cast<Bookmarker&>(*this);
	return lower_bound(self.markedLines_.begin(), self.markedLines_.end(), line);
}

/**
 * Returns true if the specified line is bookmarked.
 * @param line the line
 * @throw BadPositionException @a line is outside of the document
 */
bool Bookmarker::isMarked(length_t line) const {
	if(line >= document_.numberOfLines())
		throw BadPositionException(Position(line, 0));
	const GapBuffer<length_t>::ConstIterator i(find(line));
	return i != markedLines_.end() && *i == line;
}

/**
 * Sets or clears the bookmark of the specified line.
 * @param line the line
 * @param set true to set bookmark, false to clear
 * @throw BadPositionException @a line is outside of the document
 */
void Bookmarker::mark(length_t line, bool set) {
	if(line >= document_.numberOfLines())
		throw BadPositionException(Position(line, 0));
	const GapBuffer<length_t>::Iterator i(find(line));
	if(i != markedLines_.end() && *i == line) {
		if(!set) {
			markedLines_.erase(i);
			listeners_.notify<length_t>(&IBookmarkListener::bookmarkChanged, line);
		}
	} else {
		if(set) {
			markedLines_.insert(i, line);
			listeners_.notify<length_t>(&IBookmarkListener::bookmarkChanged, line);
		}
	}
}

/**
 * Returns the line number of the next/previous bookmarked line.
 * @param from the start line number to search
 * @param direction direction to search
 * @param wrapAround set true to enable "wrapping around". if set, this method starts again from
 * the end (in forward case) or beginning (in backward case) of the document when the reached the
 * end or beginning of the document
 * @param marks 
 * @return the next bookmarked line or @c INVALID_INDEX if not found
 * @throw BadPositionException @a line is outside of the document
 * @see #begin, #end
 */
length_t Bookmarker::next(length_t from, Direction direction, bool wrapAround /* = true */, size_t marks /* = 1 */) const {
	// this code is tested by 'test/document-test.cpp'
	if(from >= document_.numberOfLines())
		throw BadPositionException(Position(from));
	else if(marks == 0 || markedLines_.empty())
		return INVALID_INDEX;
	else if(marks > markedLines_.size()) {
		if(!wrapAround)
			return INVALID_INDEX;
		marks = marks % markedLines_.size();
		if(marks == 0)
			marks = markedLines_.size();
	}

	size_t i = static_cast<GapBuffer<length_t>::ConstIterator>(find(from)) - markedLines_.begin();
	if(direction == Direction::FORWARD) {
		if(i == markedLines_.size()) {
			if(!wrapAround)
				return INVALID_INDEX;
			i = 0;
			--marks;
		} else if(markedLines_[i] != from)
			--marks;
		if((i += marks) >= markedLines_.size()) {
			if(wrapAround)
				i -= markedLines_.size();
			else
				return INVALID_INDEX;
		}
	} else {
		if(i < marks) {
			if(wrapAround)
				i += markedLines_.size();
			else
				return INVALID_INDEX;
		}
		i -= marks;
	}
	return markedLines_[i];
}

/// Returns the number of the lines bookmarked.
size_t Bookmarker::numberOfMarks() const /*throw()*/ {
	return markedLines_.size();
}

/**
 * Removes the listener.
 * @param listener the listener to be removed
 * @throw std#invalid_argument @a listener is not registered
 */
void Bookmarker::removeListener(IBookmarkListener& listener) {
	listeners_.remove(listener);
}

/**
 * Toggles the bookmark of the spcified line.
 * @param line the line
 * @throw BadPositionException @a line is outside of the document
 */
void Bookmarker::toggle(length_t line) {
	if(line >= document_.numberOfLines())
		throw BadPositionException(Position(line, 0));
	const GapBuffer<length_t>::Iterator i(find(line));
	if(i == markedLines_.end() || *i != line)
		markedLines_.insert(i, line);
	else
		markedLines_.erase(i);
	listeners_.notify<length_t>(&IBookmarkListener::bookmarkChanged, line);
}


// DocumentPartitioner //////////////////////////////////////////////////////

/// Constructor.
DocumentPartitioner::DocumentPartitioner() /*throw()*/ : document_(0) {
}

/// Destructor.
DocumentPartitioner::~DocumentPartitioner() /*throw()*/ {
}


// Document //////////////////////////////////////////////////////////////////

/**
 * @class ascension::kernel::Document
 * A document manages a text content and supports text manipulations.
 *
 * All text content is represented in UTF-16. To treat this as UTF-32, use
 * @c DocumentCharacterIterator.
 *
 * A document manages also its operation history, encoding, and newlines and writes to or reads the
 * content from files or streams.
 *
 * @c #insert inserts a text string into any position. @c #erase deletes any text region.
 * Other classes also provide text manipulation for the document.
 *
 * @c #insert and @c #erase methods throw @c DocumentCantChangeException when the change was
 * rejected. This occurs if the the document was not marked modified and the document input's
 * @c IDocumentInput#isChangeable returned @c false.
 *
 * A document manages a revision number indicates how many times the document was changed. This
 * value is initially zero. @c #insert, @c #erase, @c #redo, and @c #resetContent methods increment
 * and @c #undo method decrement the revision number. The current revision number can be obtained
 * by @c #revisionNumber. It is guarenteed that the contents of a document correspond to same
 * revision number are equivalent.
 *
 * A document can be devides into a sequence of semantic segments called partition.
 * Document partitioners expressed by @c DocumentPartitioner class define these
 * partitioning. Each partitions have its content type and region (see @c DocumentPartition).
 * To set the new partitioner, use @c #setPartitioner method. The partitioner's ownership
 * will be transferred to the document.
 *
 * @see IDocumentPartitioner, Point, EditPoint
 */

const DocumentPropertyKey Document::TITLE_PROPERTY;

/**
 * Returns the accessible region of the document. The returned region is normalized.
 * @see #region, DocumentAccessViolationException
 */
Region Document::accessibleRegion() const /*throw()*/ {
	return (accessibleArea_ != 0) ? Region(accessibleArea_->first, *accessibleArea_->second) : region();
}

#if 0
/**
 * Registers the compound change listener.
 * @param listener the listener to be registered
 * @throw std#invalid_argument @a listener is already registered
 */
void Document::addCompoundChangeListener(ICompoundChangeListener& listener) {
	compoundChangeListeners_.add(listener);
}
#endif

/**
 * Registers the document listener with the document. After registration @a listener is notified
 * about each modification of this document.
 * @param listener the listener to be registered
 * @throw std#invalid_argument @a listener is already registered
 */
void Document::addListener(IDocumentListener& listener) {
	if(find(listeners_.begin(), listeners_.end(), &listener) != listeners_.end())
		throw invalid_argument("the listener already has been registered.");
	listeners_.push_back(&listener);
}

/**
 * Registers the document partitioning listener with the document.
 * @param listener the listener to be registered
 * @throw std#invalid_argument @a listener is already registered
 */
void Document::addPartitioningListener(IDocumentPartitioningListener& listener) {
	partitioningListeners_.add(listener);
}

/**
 * Registers the document listener as one which is notified before those document listeners
 * registered with @c #addListener are notified.
 * @internal This method is not for public use.
 * @param listener the listener to be registered
 * @throw std#invalid_argument @a listener is already registered
 */
void Document::addPrenotifiedListener(IDocumentListener& listener) {
	if(find(prenotifiedListeners_.begin(), prenotifiedListeners_.end(), &listener) != prenotifiedListeners_.end())
		throw invalid_argument("the listener already has been registered.");
	prenotifiedListeners_.push_back(&listener);
}

/**
 * Registers the rollback listener.
 * @param listener the listener to be registered
 * @throw std#invalid_argument @a listener is already registered
 */
void Document::addRollbackListener(IDocumentRollbackListener& listener) {
	rollbackListeners_.add(listener);
}

/**
 * Registers the state listener.
 * @param listener the listener to be registered
 * @throw std#invalid_argument @a listener is already registered
 */
void Document::addStateListener(IDocumentStateListener& listener) {
	stateListeners_.add(listener);
}

/// @c #resetContent invokes this method finally. Default implementation does nothing.
void Document::doResetContent() {
}

void Document::fireDocumentAboutToBeChanged() /*throw()*/ {
	if(partitioner_.get() != 0)
		partitioner_->documentAboutToBeChanged();
	for(list<IDocumentListener*>::iterator i(prenotifiedListeners_.begin()), e(prenotifiedListeners_.end()); i != e; ++i)
		(*i)->documentAboutToBeChanged(*this);
	for(list<IDocumentListener*>::iterator i(listeners_.begin()), e(listeners_.end()); i != e; ++i)
		(*i)->documentAboutToBeChanged(*this);
}

void Document::fireDocumentChanged(const DocumentChange& c, bool updateAllPoints /* = true */) /*throw()*/ {
	if(partitioner_.get() != 0)
		partitioner_->documentChanged(c);
	if(updateAllPoints)
		updatePoints(c);
	for(list<IDocumentListener*>::iterator i(prenotifiedListeners_.begin()), e(prenotifiedListeners_.end()); i != e; ++i)
		(*i)->documentChanged(*this, c);
	for(list<IDocumentListener*>::iterator i(listeners_.begin()), e(listeners_.end()); i != e; ++i)
		(*i)->documentChanged(*this, c);
}

/**
 * Returns the number of characters (UTF-16 code units) in the document.
 * @param newline the method to count newlines
 * @return the number of characters
 * @throw UnknownValueException @a newline is invalid
 */
length_t Document::length(Newline newline /* = NLF_RAW_VALUE */) const {
	newline = resolveNewline(*this, newline);
	if(isLiteralNewline(newline))
		return length_ + (numberOfLines() - 1) * ((newline != NLF_CR_LF) ? 1 : 2);
	else if(newline == NLF_RAW_VALUE) {
		length_t len = length_;
		const length_t lines = numberOfLines();
		assert(lines > 0);
		for(length_t i = 0; i < lines - 1; ++i)
			len += newlineStringLength(lines_[i]->newline_);
		return len;
	} else
		throw UnknownValueException("newline");
}

/**
 * Returns the offset of the line.
 * @param line the line
 * @param newline the line representation policy for character counting
 * @throw BadPostionException @a line is outside of the document
 * @throw UnknownValueException @a newline is invalid
 */
length_t Document::lineOffset(length_t line, Newline newline) const {
	if(line >= numberOfLines())
		throw BadPositionException(Position(line, 0));
	newline = resolveNewline(*this, newline);

	length_t offset = 0, eolLength = isLiteralNewline(newline) ? newlineStringLength(newline) : 0;
	if(eolLength == 0 && newline != NLF_RAW_VALUE)
		throw UnknownValueException("newline");
	for(length_t i = 0; i < line; ++i) {
		const Line& ln = *lines_[i];
		offset += ln.text_.length();
		if(newline == NLF_RAW_VALUE)
			offset += newlineStringLength(ln.newline_);
		else
			offset += eolLength;
	}
	return offset;
}
#if 0
/**
 * Locks the document.
 * @param locker the object locks the document. this object is used with later @c #unlock call
 * @retval true succeeded to lock
 * @retval false failed to lock because the other object already had locked, or the document's
 *         input rejected the lock (only when the document was not marked as modified)
 * @throw ReadOnlyDocumentException the document is read only
 * @throw NullPointerException @a locker is @c null
 * @see #unlock
 */
bool Document::lock(const void* locker) {
	// TODO: should support exclusive operation.
	if(isReadOnly())
		throw ReadOnlyDocumentException();
	else if(locker == 0)
		throw NullPointerException("locker");
	else if(locker_ != 0 || (isModified() && input_.get() != 0 && !input_->isChangeable()))
		return false;
	locker_ = locker;
	return true;
}
#endif
/**
 * Marks the document modified. There is not a method like @c markModified.
 * @see #isModified, IDocumentStateListener#documentModificationSignChanged
 */
void Document::markUnmodified() /*throw()*/ {
	if(isModified()) {
		lastUnmodifiedRevisionNumber_ = revisionNumber();
		stateListeners_.notify<const Document&>(&IDocumentStateListener::documentModificationSignChanged, *this);
	}
}

/**
 * Narrows the accessible area to the specified region.
 * If the document is already narrowed, the accessible region will just change to @a region. In
 * this case, @a region can be wider than the current accessible region.
 * @param region the region
 * @throw BadRegionException @a region intersects with the outside of the document
 * @see #isNarrowed, #widen
 */
void Document::narrowToRegion(const Region& region) {
	if(region.end() > this->region().end())
		throw BadRegionException(region);
	else if(region == accessibleRegion())
		return;
	if(accessibleArea_ == 0) {
		auto_ptr<pair<Position, Point*> > temp(new pair<Position, Point*>);
		temp->second = new Point(*this);
		accessibleArea_ = temp.release();
	}
	accessibleArea_->first = region.beginning();
	accessibleArea_->second->moveTo(region.end());
	for(set<Point*>::iterator i = points_.begin(); i != points_.end(); ++i) {
		if((*i)->isExcludedFromRestriction())
			(*i)->normalize();
	}
	stateListeners_.notify<const Document&>(&IDocumentStateListener::documentAccessibleRegionChanged, *this);
}

/**
 * Removes the document listener from the document.
 * @param listener the listener to be removed
 * @throw std#invalid_argument @a listener is not registered
 */
void Document::removeListener(IDocumentListener& listener) {
	const list<IDocumentListener*>::iterator i(find(listeners_.begin(), listeners_.end(), &listener));
	if(i == listeners_.end())
		throw invalid_argument("the listener is not registered.");
	listeners_.erase(i);
}

/**
 * Removes the document partitioning listener from the document.
 * @param listener the listener to be removed
 * @throw std#invalid_argument @a listener is not registered
 */
void Document::removePartitioningListener(IDocumentPartitioningListener& listener) {
	partitioningListeners_.remove(listener);
}

/**
 * Removes the pre-notified document listener from the document.
 * @internal This method is not for public use.
 * @param listener the listener to be removed
 * @throw std#invalid_argument @a listener is not registered
 */
void Document::removePrenotifiedListener(IDocumentListener& listener) {
	const list<IDocumentListener*>::iterator i(find(prenotifiedListeners_.begin(), prenotifiedListeners_.end(), &listener));
	if(i == prenotifiedListeners_.end())
		throw invalid_argument("the listener is not registered.");
	prenotifiedListeners_.erase(i);
}

/**
 * Removes the rollback listener.
 * @param listener the listener to be removed
 * @throw std#invalid_argument @a listener is not registered
 */
void Document::removeRollbackListener(IDocumentRollbackListener& listener) {
	rollbackListeners_.remove(listener);
}

/**
 * Removes the state listener.
 * @param listener the listener to be removed
 * @throw std#invalid_argument @a listener is not registered
 */
void Document::removeStateListener(IDocumentStateListener& listener) {
	stateListeners_.remove(listener);
}

/**
 * Resets and initializes the content of the document. Does the following:
 * - Clears the text buffer, invokes the two methods of @c IDocumentListener and increments the
 *   revision number even if the document was empty.
 * - Moves the all point to the beginning of the document.
 * - Clears the undo/redo buffers.
 * - Resets the modification flag to false.
 * - Resets the read-only flag to false.
 * - Revokes the narrowing.
 * - Removes the all bookmarks.
 * @note This method does not call @c IDocumentInput#isChangeable for rejection.
 * @see #doResetContent
 */
void Document::resetContent() {
	if(lines_.empty())	// called by constructor
		lines_.insert(lines_.begin(), new Line(0));
	else {
		widen();
		for(set<Point*>::iterator i(points_.begin()), e(points_.end()); i != e; ++i)
			(*i)->moveTo(0, 0);
		bookmarker_->clear();

		fireDocumentAboutToBeChanged();
		if(length_ != 0) {
			assert(!lines_.empty());
			for(size_t i = 0, c = lines_.size(); i < c; ++i)
				delete lines_[i];
			lines_.clear();
			lines_.insert(lines_.begin(), new Line(revisionNumber_ + 1));
			length_ = 0;
			++revisionNumber_;
		}
		const DocumentChange ca(region(), Region(region().beginning()));
		fireDocumentChanged(ca, false);
	}

	setReadOnly(false);
	markUnmodified();
	clearUndoBuffer();
	onceUndoBufferCleared_ = false;
	doResetContent();
}

/**
 * Sets the new document input.
 * @param newInput the new document input. can be @c null
 * @param delegateOwnership set true to transfer the ownership into the callee
 */
void Document::setInput(IDocumentInput* newInput, bool delegateOwnership) /*throw()*/ {
	input_.reset(newInput, delegateOwnership);
}

/**
 * Sets the new document partitioner.
 * @param newPartitioner the new partitioner. the ownership will be transferred to the callee
 */
void Document::setPartitioner(auto_ptr<DocumentPartitioner> newPartitioner) /*throw()*/ {
	partitioner_ = newPartitioner;
	if(partitioner_.get() != 0)
		partitioner_->install(*this);
	partitioningChanged(region());
}

/**
 * Associates the given property with the document.
 * @param key the key of the property
 * @param property the property value
 * @see #property
 */
void Document::setProperty(const DocumentPropertyKey& key, const String& property) {
	map<const DocumentPropertyKey*, String*>::iterator i(properties_.find(&key));
	if(i == properties_.end())
		properties_.insert(make_pair(&key, new String(property)));
	else
		i->second->assign(property);
	stateListeners_.notify<const Document&, const DocumentPropertyKey&>(&IDocumentStateListener::documentPropertyChanged, *this, key);
}

/**
 * Makes the document read only or not.
 * @see ReadOnlyDocumentException, #isReadOnly
 */
void Document::setReadOnly(bool readOnly /* = true */) /*throw()*/ {
	if(readOnly != isReadOnly()) {
		readOnly_ = readOnly;
		stateListeners_.notify<const Document&>(&IDocumentStateListener::documentReadOnlySignChanged, *this);
	}
}
#if 0
/**
 * Unlocks the document.
 * @param locker the object used with previous @c #lock call
 * @throw IllegalStateException the document is not locked
 * @throw NullPointerException @a locker is @c null
 * @throw std#invalid_argument @a locker is not the object used with @c #lock call
 * @see #lock
 */
void Document::unlock(const void* locker) {
	// TODO: support exclusive operation.
	if(locker_ == 0)
		throw IllegalStateException("the document's input is not locked.");
	else if(locker == 0)
		throw NullPointerException("locker");
	else if(locker != locker_)
		throw invalid_argument("locker");
	locker_ = 0;
}
#endif
/**
 * Informs the document change to the adapting points.
 * @param change the document change
 */
inline void Document::updatePoints(const DocumentChange& change) /*throw()*/ {
	for(set<Point*>::iterator i = points_.begin(); i != points_.end(); ++i) {
		if((*i)->adaptsToDocument())
			(*i)->update(change);
	}
}

/**
 * Revokes the narrowing.
 * @see #isNarrowed, #narrow
 */
void Document::widen() /*throw()*/ {
	if(accessibleArea_ != 0) {
		delete accessibleArea_->second;
		delete accessibleArea_;
		accessibleArea_ = 0;
		stateListeners_.notify<const Document&>(&IDocumentStateListener::documentAccessibleRegionChanged, *this);
	}
}


// Document.Line ////////////////////////////////////////////////////////////

Document::Line::Line(size_t revisionNumber) /*throw()*/ : newline_(ASCENSION_DEFAULT_NEWLINE), revisionNumber_(revisionNumber) {
}

Document::Line::Line(size_t revisionNumber, const String& text,
		Newline newline /* = ASCENSION_DEFAULT_NEWLINE */) : text_(text), newline_(newline), revisionNumber_(revisionNumber) {
}


// CompoundChangeSaver //////////////////////////////////////////////////////

/**
 * @class ascension#kernel#CompoundChangeSaver
 *
 * Calls automatically @c Document#beginCompoundChange and @c Document#endCompoundChange.
 *
 * @code
 * extern Document* target;
 * CompoundChangeSaver saver(target);
 * target-&gt;mayThrow();
 * // target-&gt;endCompoundChange() will be called automatically
 * @endcode
 *
 * @note This class is not intended to be subclassed.
 */

/**
 * Constructor calls @c Document#beginCompoundChange.
 * @param document the document this object manages. if this is @c null, the object does nothing
 * @throw ... any exceptions @c Document#beginCompoundChange throws
 */
CompoundChangeSaver::CompoundChangeSaver(Document* document) : document_(document) {
	if(document_ != 0)
		document_->beginCompoundChange();
}

/**
 * Destructor calls @c Document#endCompoundChange.
 * @throw ... any exceptions @c Document#endCompoundChange throws
 */
CompoundChangeSaver::~CompoundChangeSaver() {
	if(document_ != 0)
		document_->endCompoundChange();
}

#if 0
// DocumentLocker ///////////////////////////////////////////////////////////

/**
 * @class ascension::kernel::DocumentLocker
 *
 * Calls @c Document#lock and @c Document#unlock automatically.
 *
 * @note This class is not intended to be subclassed.
 */

/**
 * Constructor calls @c Document#lock.
 * @param document the document to lock
 * @throw ReadOnlyDocumentException the document is read only
 * @throw DocumentCantChangeException failed to lock the document
 */
DocumentLocker::DocumentLocker(Document& document) : document_((document.locker() == 0) ? &document : 0) {
	if(document_ != 0 && !document_->lock(this))	// manage lock if there is not an active lock
		throw DocumentCantChangeException();
}

/// Destructor calls @c Document#unlock.
DocumentLocker::~DocumentLocker() /*throw()*/ {
	if(document_ != 0)
		document_->unlock(this);
}
#endif

// DocumentCharacterIterator ////////////////////////////////////////////////

/**
 * @class ascension::text::DocumentCharacterIterator
 * Bidirectional iterator scans characters in the specified document.
 *
 * @c #current implementation of this class returns a character at which the iterator addresses. A
 * returned character is as a UTF-32 code unit (not UTF-16). In the following cases, returns a
 * special value depending on the context:
 *
 * - @c CharacterIterator#DONE at the end of the region of the iterator
 * - @c LINE_SEPARATOR at the end of the line
 * - a raw code unit value at any unpaired surrogate
 *
 * This class can't detect any change of the document. When the document changed, the existing
 * iterators may be invalid.
 *
 * @note This class is not intended to be subclassed.
 */

const CharacterIterator::ConcreteTypeTag DocumentCharacterIterator::CONCRETE_TYPE_TAG_ = CharacterIterator::ConcreteTypeTag();

/// Default constructor.
DocumentCharacterIterator::DocumentCharacterIterator() /*throw()*/ : CharacterIterator(CONCRETE_TYPE_TAG_), document_(0), line_(0) {
}

/**
 * Constructor. The iteration region is the accessible area of the document.
 * @param document the document to iterate
 * @param position the position at which the iteration starts
 * @throw BadPositionException @a position is outside of the accessible area of the document
 */
DocumentCharacterIterator::DocumentCharacterIterator(const Document& document, const Position& position) :
		CharacterIterator(CONCRETE_TYPE_TAG_), document_(&document),
		region_(document.region()), line_(&document.line(position.line)), p_(position) {
	if(!region_.includes(p_))
		throw BadPositionException(p_);
}

/**
 * Constructor. The iteration is started at @a region.beginning().
 * @param document the document to iterate
 * @param region the region to iterate
 * @throw BadRegionException @a region intersects outside of the document
 */
DocumentCharacterIterator::DocumentCharacterIterator(const Document& document, const Region& region) :
		CharacterIterator(CONCRETE_TYPE_TAG_), document_(&document), region_(region),
		line_(&document.line(region.beginning().line)), p_(region.beginning()) {
	region_.normalize();
	if(!document.region().encompasses(region_))
		throw BadRegionException(region_);
}

/**
 * Constructor.
 * @param document the document to iterate
 * @param region the region to iterate
 * @param position the position at which the iteration starts
 * @throw BadRegionException @a region intersects outside of the document
 * @throw BadPositionException @a position is outside of @a region
 */
DocumentCharacterIterator::DocumentCharacterIterator(const Document& document, const Region& region, const Position& position) :
		CharacterIterator(CONCRETE_TYPE_TAG_), document_(&document), region_(region), line_(&document.line(position.line)), p_(position) {
	region_.normalize();
	if(!document.region().encompasses(region_))
		throw BadRegionException(region_);
	else if(!region_.includes(p_))
		throw BadPositionException(p_);
}

/// Copy-constructor.
DocumentCharacterIterator::DocumentCharacterIterator(const DocumentCharacterIterator& rhs) /*throw()*/ :
		text::CharacterIterator(rhs), document_(rhs.document_), region_(rhs.region_), line_(rhs.line_), p_(rhs.p_) {
}

/// @see text#CharacterIterator#current
CodePoint DocumentCharacterIterator::current() const /*throw()*/ {
	if(p_ == region_.second)
		return DONE;
	else if(p_.column == line_->length())
		return LINE_SEPARATOR;
	else
		return (surrogates::isHighSurrogate((*line_)[p_.column])
			&& p_.column + 1 < line_->length() && surrogates::isLowSurrogate((*line_)[p_.column + 1])) ?
			surrogates::decode((*line_)[p_.column], (*line_)[p_.column + 1]) : (*line_)[p_.column];
}

/// @see text#CharacterIterator#doAssign
void DocumentCharacterIterator::doAssign(const CharacterIterator& rhs) {
	CharacterIterator::operator=(rhs);
	const DocumentCharacterIterator& r = static_cast<const DocumentCharacterIterator&>(rhs);
	document_ = r.document_;
	line_ = r.line_;
	p_ = r.p_;
	region_ = r.region_;
}

/// @see text#CharacterIterator#doClone
auto_ptr<CharacterIterator> DocumentCharacterIterator::doClone() const {
	return auto_ptr<CharacterIterator>(new DocumentCharacterIterator(*this));
}

/// @see text#CharacterIterator#doFirst
void DocumentCharacterIterator::doFirst() {
	seek(region_.first);
}

/// @see text#CharacterIterator#doLast
void DocumentCharacterIterator::doLast() {
	seek(region_.second);
}

/// @see text#CharacterIterator#doEquals
bool DocumentCharacterIterator::doEquals(const CharacterIterator& rhs) const {
	return p_ == static_cast<const DocumentCharacterIterator&>(rhs).p_;
}

/// @see text#CharacterIterator#doLess
bool DocumentCharacterIterator::doLess(const CharacterIterator& rhs) const {
	return p_ < static_cast<const DocumentCharacterIterator&>(rhs).p_;
}

/// @see text#CharacterIterator#doNext
void DocumentCharacterIterator::doNext() {
	if(!hasNext())
//		throw out_of_range("the iterator is at the last.");
		return;
	else if(p_.column == line_->length()) {
		line_ = &document_->line(++p_.line);
		p_.column = 0;
	} else if(++p_.column < line_->length()
			&& surrogates::isLowSurrogate((*line_)[p_.column]) && surrogates::isHighSurrogate((*line_)[p_.column - 1]))
		++p_.column;
}

/// @see text#CharacterIterator#doPrevious
void DocumentCharacterIterator::doPrevious() {
	if(!hasPrevious())
//		throw out_of_range("the iterator is at the first.");
		return;
	else if(p_.column == 0)
		p_.column = (line_ = &document_->line(--p_.line))->length();
	else if(--p_.column > 0 && surrogates::isLowSurrogate((*line_)[p_.column]) && surrogates::isHighSurrogate((*line_)[p_.column - 1]))
		--p_.column;
}

/**
 * Sets the region of the iterator. The current position will adjusted.
 * @param newRegion the new region to set
 * @throw BadRegionException @a newRegion intersects outside of the document
 */
void DocumentCharacterIterator::setRegion(const Region& newRegion) {
	const Position e(document_->region().second);
	if(newRegion.first > e || newRegion.second > e)
		throw BadRegionException(newRegion);
	if(!(region_ = newRegion).includes(p_))
		seek(p_);
}


// NullPartitioner //////////////////////////////////////////////////////////

/// Constructor.
NullPartitioner::NullPartitioner() /*throw()*/ : p_(DEFAULT_CONTENT_TYPE, Region(Position::ZERO_POSITION, Position::INVALID_POSITION)) {
}

/// @see DocumentPartitioner#documentAboutToBeChanged
void NullPartitioner::documentAboutToBeChanged() /*throw()*/ {
}

/// @see DocumentPartitioner#documentChanged
void NullPartitioner::documentChanged(const DocumentChange&) /*throw()*/ {
	p_.region.second.line = INVALID_INDEX;
}

/// @see DocumentPartitioner#doGetPartition
void NullPartitioner::doGetPartition(const Position&, DocumentPartition& partition) const /*throw()*/ {
	if(p_.region.second.line == INVALID_INDEX)
		const_cast<NullPartitioner*>(this)->p_.region.second = document()->region().second;
	partition = p_;
}

/// @see DocumentPartitioner#doInstall
void NullPartitioner::doInstall() /*throw()*/ {
	p_.region.second.line = INVALID_INDEX;
}
