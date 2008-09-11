/**
 * @file document.cpp
 * @author exeal
 * @date 2003-2006 (was EditDoc.h)
 * @date 2006-2008
 */

#include "document.hpp"
#include "point.hpp"
//#include <shlwapi.h>	// PathXxxx
//#include <shlobj.h>	// SHGetDesktopFolder, IShellFolder, ...
//#include <MAPI.h>		// MAPISendMail
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
			assert((newline & NLF_SPECIAL_VALUE_MASK) != 0);
		}
		return newline;
	}
} // namespace @0


const Position Position::ZERO_POSITION(0, 0);
const Position Position::INVALID_POSITION(INVALID_INDEX, INVALID_INDEX);


// kernel free functions ////////////////////////////////////////////////////

/**
 * Returns absolute character offset of the specified position from the start of the document.
 * @param document the document
 * @param at the position
 * @param fromAccessibleStart
 * @throw BadPositionException @a at is outside of the document
 */
length_t kernel::getAbsoluteOffset(const Document& document, const Position& at, bool fromAccessibleStart) {
	if(at > document.region().second)
		throw BadPositionException();
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
Position kernel::updatePosition(const Position& position, const DocumentChange& change, Direction gravity) throw() {
	Position newPosition(position);
	if(!change.isDeletion()) {	// insertion
		if(position < change.region().first)	// behind the current position
			return newPosition;
		else if(position == change.region().first && gravity == BACKWARD) // the current position + backward gravity
			return newPosition;
		else if(position.line > change.region().first.line)	// in front of the current line
			newPosition.line += change.region().second.line - change.region().first.line;
		else {	// in the current line
			newPosition.line += change.region().second.line - change.region().first.line;
			newPosition.column += change.region().second.column - change.region().first.column;
		}
	} else {	// deletion
		if(position < change.region().second) {	// the end is behind the current line
			if(position <= change.region().first)
				return newPosition;
			else	// in the region
				newPosition = change.region().first;
		} else if(position.line > change.region().second.line)	// in front of the current line
			newPosition.line -= change.region().second.line - change.region().first.line;
		else {	// the end is the current line
			if(position.line == change.region().first.line)	// the region is single-line
				newPosition.column -= change.region().second.column - change.region().first.column;
			else {	// the region is multiline
				newPosition.line -= change.region().second.line - change.region().first.line;
				newPosition.column -= change.region().second.column - change.region().first.column;
			}
		}
	}
	return newPosition;
}

/**
 * Writes the content of the document to the specified output stream.
 * <p>This method does not write Unicode byte order mark.</p>
 * <p>This method explicitly flushes te output stream.</p>
 * @param out the output stream
 * @param document the document
 * @param region the region to be written (this region is not restricted with narrowing)
 * @param newline the newline representation
 * @return @a out
 * @see getNewlineString, readDocumentFromStream
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
		const String eol(isLiteralNewline(newline) ? getNewlineString(newline) : L"");
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
				out.write(getNewlineString(line.newline()), static_cast<streamsize>(getNewlineStringLength(line.newline())));
			else
				out.write(eol.data(), static_cast<streamsize>(eol.length()));
		}
	}
	return out.flush();
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

/// Default constructor.
DisposedDocumentException::DisposedDocumentException() :
	runtime_error("The document the object connecting to has been already disposed.") {
}

/// Default constructor.
ReadOnlyDocumentException::ReadOnlyDocumentException() :
	IllegalStateException("The document is readonly. Any edit process is denied.") {
}

/// Default constructor.
DocumentAccessViolationException::DocumentAccessViolationException() :
	invalid_argument("The specified position or region is inaccessible.") {
}

/// Default constructor.
BadPositionException::BadPositionException() :
	invalid_argument("The position is outside of the document.") {
}


// Bookmarker ///////////////////////////////////////////////////////////////

/**
 * Private constructor.
 * @param document the document
 */
Bookmarker::Bookmarker(Document& document) throw() : document_(document) {
	document.addListener(*this);
}

/// Destructor.
Bookmarker::~Bookmarker() throw() {
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

/// Deletes all bookmarks.
void Bookmarker::clear() throw() {
	if(!markedLines_.empty()) {
		markedLines_.clear();
		listeners_.notify(&IBookmarkListener::bookmarkCleared);
	}
}

/// @see IDocumentListener#documentAboutToBeChanged
bool Bookmarker::documentAboutToBeChanged(const Document& document, const DocumentChange& change) {
	return true;
}

/// @see IDocumentListener#documentChanged
void Bookmarker::documentChanged(const Document& document, const DocumentChange& change) {
}

inline GapBuffer<length_t>::Iterator Bookmarker::find(length_t line) const throw() {
	assert(line < document_.numberOfLines());
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
		throw BadPositionException();
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
		throw BadPositionException();
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
 * @param from the start line number to search. this line may be the result
 * @param direction direction to search
 * @param marks
 * @return the next bookmarked line or @c INVALID_INDEX if not found
 * @throw BadPositionException @a line is outside of the document
 */
length_t Bookmarker::next(length_t from, Direction direction, size_t marks /* = 1 */) const {
	if(from >= document_.numberOfLines())
		throw BadPositionException();
	else if(marks == 0 || markedLines_.empty())
		return INVALID_INDEX;
	else if(marks > markedLines_.size()) {
		marks = marks % markedLines_.size();
		if(marks == 0)
			marks = markedLines_.size();
	}
	return INVALID_INDEX;
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
		throw BadPositionException();
	const GapBuffer<length_t>::Iterator i(find(line));
	if(i == markedLines_.end() || *i != line)
		markedLines_.insert(i, line);
	else
		markedLines_.erase(i);
	listeners_.notify<length_t>(&IBookmarkListener::bookmarkChanged, line);
}


// DocumentPartitioner //////////////////////////////////////////////////////

/// Constructor.
DocumentPartitioner::DocumentPartitioner() throw() : document_(0) {
}

/// Destructor.
DocumentPartitioner::~DocumentPartitioner() throw() {
}


// Document.UndoManager /////////////////////////////////////////////////////

// about document undo/redo
namespace {
	class InsertOperation;
	class DeleteOperation;

	/**
	 * An abstract edit operation.
	 * @see DeleteOperation, InsertOperation
	 */
	class IOperation {
	public:
		/// Destructor
		virtual ~IOperation() {}
		/// Returns the operation is executable.
		virtual bool canExecute(Document& document) const = 0;
		/// Returns true if the operation can be appended to insertion @a postOperation.
		virtual bool isConcatenatable(InsertOperation& postOperation, const Document& document) const = 0;
		/// Returns true if the operation can be appended to deletion @a postOperation.
		virtual bool isConcatenatable(DeleteOperation& postOperation, const Document& document) const = 0;
		/// Executes the operation.
		virtual Position execute(Document& document) = 0;
	};

	/// An insertion operation.
	class InsertOperation : virtual public IOperation, public manah::FastArenaObject<InsertOperation> {
	public:
		InsertOperation(const Position& pos, const String& text) : position_(pos), text_(text) {}
		bool canExecute(Document& document) const throw() {return !document.isNarrowed() || document.region().includes(position_);}
		bool isConcatenatable(InsertOperation&, const Document&) const throw() {return false;}
		bool isConcatenatable(DeleteOperation&, const Document&) const throw() {return false;}
		Position execute(Document& document) {Position p; return document.insert(position_, text_, &p) ? p : Position::INVALID_POSITION;}
	private:
		Position position_;
		String text_;
	};

	/// An deletion operation.
	class DeleteOperation : virtual public IOperation, public manah::FastArenaObject<DeleteOperation> {
	public:
		DeleteOperation(const Region& region) throw() : region_(region) {}
		bool canExecute(Document& document) const throw() {return !document.isNarrowed() || document.region().encompasses(region_);}
		bool isConcatenatable(InsertOperation&, const Document&) const throw() {return false;}
		bool isConcatenatable(DeleteOperation& postOperation, const Document&) const throw() {
			const Position& bottom = region_.end();
			if(bottom.column == 0 || bottom != postOperation.region_.beginning()) return false;
			else {const_cast<DeleteOperation*>(this)->region_.end() = postOperation.region_.end(); return true;}
		}
		Position execute(Document& document) {return document.erase(region_) ? region_.beginning() : Position::INVALID_POSITION;}
	private:
		Region region_;
	};

	/// A compound operation.
	class CompoundOperation : public manah::FastArenaObject<CompoundOperation> {
	public:
		~CompoundOperation() throw();
		pair<bool, size_t> execute(Document& document, Position& resultPosition);
		void pop() {operations_.pop(); numbersOfOperations_.pop();}
		void push(InsertOperation& operation, const Document&) {operations_.push(&operation); numbersOfOperations_.push(1);}
		void push(DeleteOperation& operation, const Document& document);
		IOperation& top() const {return *operations_.top();}
	private:
		stack<IOperation*> operations_;
		stack<size_t> numbersOfOperations_;
	};
} // namespace @0

CompoundOperation::~CompoundOperation() throw() {
	while(!operations_.empty()) {
		delete operations_.top();
		operations_.pop();
	}
}

inline pair<bool, size_t> CompoundOperation::execute(Document& document, Position& resultPosition) {
	size_t c = 0;
	resultPosition = Position::INVALID_POSITION;
	while(!operations_.empty()) {
		if(!operations_.top()->canExecute(document))
			break;
		resultPosition = operations_.top()->execute(document);
		delete operations_.top();
		c += numbersOfOperations_.top() + 1;
		pop();
	}
	return make_pair(operations_.empty(), c);
}

inline void CompoundOperation::push(DeleteOperation& operation, const Document& document) {
	// if also the previous operation is deletion, extend the region to concatenate the operations
	if(!operations_.empty() && operations_.top()->isConcatenatable(operation, document)) {
		delete &operation;
		++numbersOfOperations_.top();
	} else {
		operations_.push(&operation);
		numbersOfOperations_.push(1);
	}
}

/// Manages undo/redo of the document.
class Document::UndoManager {
	MANAH_NONCOPYABLE_TAG(UndoManager);
public:
	// constructors
	UndoManager(Document& document) throw();
	virtual ~UndoManager() throw();
	// attributes
	size_t numberOfRedoableCompoundOperations() const throw();
	size_t numberOfUndoableCompoundOperations() const throw();
	bool isStackingCompoundOperation() const throw();
	// operations
	void beginCompoundOperation() throw();
	void clear() throw();
	void endCompoundOperation() throw();
	template<typename Operation> void pushUndoableOperation(Operation& operation);
	pair<bool, size_t> redo(Position& resultPosition);
	pair<bool, size_t> undo(Position& resultPosition);
private:
	Document& document_;
	stack<CompoundOperation*> undoStack_, redoStack_;
	enum {
		NONE, WAIT_FOR_FIRST_PUSH, WAIT_FOR_CONTINUATION
	} compoundOperationStackingState_;
	bool virtualOperation_;
	CompoundOperation* virtualUnit_;
	CompoundOperation* lastUnit_;
	IOperation* savedOperation_;
};

/**
 * Constructor.
 * @param document the target document
 */
Document::UndoManager::UndoManager(Document& document) throw()
		: document_(document), compoundOperationStackingState_(NONE),
		virtualOperation_(false), virtualUnit_(0), lastUnit_(0), savedOperation_(0) {
}

/// Destructor.
Document::UndoManager::~UndoManager() throw() {
	clear();
}

/// Starts the compound operation.
inline void Document::UndoManager::beginCompoundOperation() throw() {
	assert(compoundOperationStackingState_ == NONE);
	compoundOperationStackingState_ = WAIT_FOR_FIRST_PUSH;
}

/// Clears the stacks.
inline void Document::UndoManager::clear() throw() {
	compoundOperationStackingState_ = NONE;
	lastUnit_ = 0;
	while(!undoStack_.empty()) {
		delete undoStack_.top();
		undoStack_.pop();
	}
	while(!redoStack_.empty()) {
		delete redoStack_.top();
		redoStack_.pop();
	}
}

/// Ends the compound operation.
inline void Document::UndoManager::endCompoundOperation() throw() {
	compoundOperationStackingState_ = NONE;
}

/// Returns true if the compound operation is running.
inline bool Document::UndoManager::isStackingCompoundOperation() const throw() {
	return compoundOperationStackingState_ != NONE;
}

/// Returns the number of the redoable operations.
inline size_t Document::UndoManager::numberOfRedoableCompoundOperations() const throw() {
	return redoStack_.size();
}

/// Returns the number of the undoable operations.
inline size_t Document::UndoManager::numberOfUndoableCompoundOperations() const throw() {
	return undoStack_.size();
}

/**
 * Pushes the operation into the undo stack.
 * @param operation the operation to be pushed
 */
template<typename Operation> void Document::UndoManager::pushUndoableOperation(Operation& operation) {
	// make the redo stack empty
	if(!virtualOperation_) {
		while(!redoStack_.empty()) {
			delete redoStack_.top();
			redoStack_.pop();
		}
	}

	if(virtualOperation_) {	// 仮想操作時はスタックへの追加を遅延する
		if(virtualUnit_ == 0)	// 初回
			virtualUnit_ = new CompoundOperation();
		virtualUnit_->push(operation, document_);
	} else if(compoundOperationStackingState_ == WAIT_FOR_CONTINUATION && lastUnit_ != 0)	// 最後の操作単位に結合
		lastUnit_->push(operation, document_);
	else {
		CompoundOperation* newUnit = new CompoundOperation();
		newUnit->push(operation, document_);
		undoStack_.push(newUnit);
		lastUnit_ = newUnit;
		if(compoundOperationStackingState_ == WAIT_FOR_FIRST_PUSH)
			compoundOperationStackingState_ = WAIT_FOR_CONTINUATION;
	}
}

/// Redoes one operation.
pair<bool, size_t> Document::UndoManager::redo(Position& resultPosition) {
	if(redoStack_.empty())
		return make_pair(false, 0);

	CompoundOperation* unit = redoStack_.top();
	virtualOperation_ = true;			// 仮想操作開始
	const pair<bool, size_t> status(unit->execute(document_, resultPosition));
	if(status.first)
		redoStack_.pop();
	if(virtualUnit_ != 0)
		undoStack_.push(virtualUnit_);	// 仮想操作単位をアンドゥスタックへ移す
	virtualUnit_ = lastUnit_ = 0;
	virtualOperation_ = false;			// 仮想操作終了
	if(status.first)
		delete unit;
	return status;
}

/// Undoes one operation.
pair<bool, size_t> Document::UndoManager::undo(Position& resultPosition) {
	if(undoStack_.empty())
		return make_pair(false, 0);

	CompoundOperation* unit = undoStack_.top();
	virtualOperation_ = true;			// 仮想操作開始
	const pair<bool, size_t> status = unit->execute(document_, resultPosition);
	if(status.first)
		undoStack_.pop();
	if(virtualUnit_ != 0)
		redoStack_.push(virtualUnit_);	// 仮想操作単位をリドゥスタックへ移す
	virtualUnit_ = lastUnit_ = 0;
	virtualOperation_ = false;			// 仮想操作終了
	if(status.first)
		delete unit;
	return status;
}


// Document //////////////////////////////////////////////////////////////////

/**
 * @class ascension::kernel::Document
 * A document manages a text content and supports text manipulations.
 *
 * All text content is represented in UTF-16. To treat this as UTF-32, use
 * @c DocumentCharacterIterator.
 *
 * A document manages also its operation history, encoding, and line-breaks
 * and writes to or reads the content from files or streams.
 *
 * @c #insert inserts a text string into any position. @c #erase deletes any text region.
 * Other classes also provide text manipulation for the document.
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

/// Constructor.
Document::Document() : session_(0), partitioner_(0),
		contentTypeInformationProvider_(new DefaultContentTypeInformationProvider),
		readOnly_(false), length_(0), revisionNumber_(0), lastUnmodifiedRevisionNumber_(0),
		onceUndoBufferCleared_(false), recordingOperations_(true), changing_(false), accessibleArea_(0) {
	bookmarker_.reset(new Bookmarker(*this));
	undoManager_ = new UndoManager(*this);
	resetContent();
}

/// Destructor.
Document::~Document() {
	for(set<Point*>::iterator i(points_.begin()), e(points_.end()); i != e; ++i)
		(*i)->documentDisposed();
	if(accessibleArea_ != 0) {
		delete accessibleArea_->second;
		delete accessibleArea_;
	}
	for(map<const DocumentPropertyKey*, String*>::iterator i(properties_.begin()), e(properties_.end()); i != e; ++i)
		delete i->second;
	delete undoManager_;
	bookmarker_.reset(0);	// Bookmarker.~Bookmarker() calls Document...
}

/**
 * Returns the accessible region of the document. The returned region is normalized.
 * @see #region, DocumentAccessViolationException
 */
Region Document::accessibleRegion() const throw() {
	return (accessibleArea_ != 0) ? Region(accessibleArea_->first, *accessibleArea_->second) : region();
}

/**
 * Registers the compound change listener.
 * @param listener the listener to be registered
 * @throw std#invalid_argument @a listener is already registered
 */
void Document::addCompoundChangeListener(ICompoundChangeListener& listener) {
	compoundChangeListeners_.add(listener);
}

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

/**
 * Starts the sequential edit. Restarts if the sequential edit is already running.
 * @see #endSequentialEdit, #isSequentialEditing
 */
void Document::beginCompoundChange() throw() {
	if(isCompoundChanging())
		endCompoundChange();
	undoManager_->beginCompoundOperation();
	compoundChangeListeners_.notify<const Document&>(&ICompoundChangeListener::documentCompoundChangeStarted, *this);
}

/// Clears the undo/redo stacks and deletes the history.
void Document::clearUndoBuffer() {
	undoManager_->clear();
	onceUndoBufferCleared_ = true;
}

/// @c #resetContent invokes this method finally. Default implementation does nothing.
void Document::doResetContent() {
}

/**
 * Ends the active compound change.
 * @see #beginCompoundChange, #isCompoundChanging
 */
void Document::endCompoundChange() throw() {
	undoManager_->endCompoundOperation();
	compoundChangeListeners_.notify<const Document&>(&ICompoundChangeListener::documentCompoundChangeStopped, *this);
}

/**
 * Deletes the specified region of the document.
 * This method sets the modification flag and calls the listeners'
 * @c IDocumentListener#documentAboutToBeChanged and @c IDocumentListener#documentChanged.
 * @param region the region to be deleted. if empty, this method does nothing
 * @return the position where the point which deletes the text will move to
 * @throw ReadOnlyDocumentException the document is read only
 * @throw DocumentAccessViolationException @a region intersects the inaccesible region
 */
bool Document::erase(const Region& region) {
	if(changing_ || isReadOnly())
		throw ReadOnlyDocumentException();
	else if(region.isEmpty())	// empty -> ignore
		return true;
	else if(isNarrowed() && !accessibleRegion().encompasses(region))
		throw DocumentAccessViolationException();

	ModificationGuard guard(*this);
	if(!fireDocumentAboutToBeChanged(DocumentChange(true, region)))
		return false;
	return eraseText(region), true;
}

void Document::eraseText(const Region& region) {
	assert(!isNarrowed() || accessibleRegion().encompasses(region));
	const Position& beginning = region.beginning();
	const Position& end = region.end();
	basic_stringbuf<Char> deletedString;

	if(beginning.line == end.line) {	// region is single line
		const Line&	lineInfo = getLineInformation(end.line);
		String&	line = const_cast<String&>(this->line(end.line));

		++const_cast<Line&>(lineInfo).operationHistory_;
		deletedString.sputn(line.data() + beginning.column, static_cast<streamsize>(end.column - beginning.column));
		line.erase(beginning.column, end.column - beginning.column);
		length_ -= end.column - beginning.column;
	} else {						// region is multiline
		Line* line = lines_[beginning.line];
		auto_ptr<String> tail;
		deletedString.sputn(line->text_.data() + beginning.column, static_cast<streamsize>(line->text_.length() - beginning.column));
		length_ -= line->text_.length() - beginning.column;
		line->text_.erase(beginning.column);

		Line& firstLine = *lines_[beginning.line];
		Newline lastNewline;
		deletedString.sputn(getNewlineString(lines_[beginning.line]->newline_),
			static_cast<streamsize>(getNewlineStringLength(lines_[beginning.line]->newline_)));
		for(length_t i = beginning.line + 1; ; ++i) {
			line = lines_[i];
			deletedString.sputn(line->text_.data(), static_cast<streamsize>((i != end.line) ? line->text_.length() : end.column));
			length_ -= line->text_.length();
			if(i != end.line)
				deletedString.sputn(getNewlineString(line->newline_), static_cast<streamsize>(getNewlineStringLength(line->newline_)));
			else {	// last
				tail.reset(new String(line->text_.substr(end.column)));
				lastNewline = line->newline_;
				break;
			}
		}
		lines_.erase(beginning.line + 1, end.line - beginning.line);

		// concatinate before and after erased part
		firstLine.newline_ = lastNewline;
		++firstLine.operationHistory_;
		if(!tail->empty()) {
			firstLine.text_ += *tail;
			length_ += tail->length();
		}
	}

	if(recordingOperations_)
		undoManager_->pushUndoableOperation(*(new InsertOperation(beginning, deletedString.str())));

	// notify the change
	++revisionNumber_;
	fireDocumentChanged(DocumentChange(true, Region(beginning, end)));
	stateListeners_.notify<const Document&>(&IDocumentStateListener::documentModificationSignChanged, *this);

//	return beginning;
}

bool Document::fireDocumentAboutToBeChanged(const DocumentChange& c) throw() {
	if(partitioner_.get() != 0)
		partitioner_->documentAboutToBeChanged();
	for(list<IDocumentListener*>::iterator i(prenotifiedListeners_.begin()), e(prenotifiedListeners_.end()); i != e; ++i) {
		if(!(*i)->documentAboutToBeChanged(*this, c))
			return false;
	}
	for(list<IDocumentListener*>::iterator i(listeners_.begin()), e(listeners_.end()); i != e; ++i) {
		if(!(*i)->documentAboutToBeChanged(*this, c))
			return false;
	}
	return true;
}

void Document::fireDocumentChanged(const DocumentChange& c, bool updateAllPoints /* = true */) throw() {
	if(partitioner_.get() != 0)
		partitioner_->documentChanged(c);
	if(updateAllPoints)
		updatePoints(c);
	for(list<IDocumentListener*>::iterator i(prenotifiedListeners_.begin()), e(prenotifiedListeners_.end()); i != e; ++i)
		(*i)->documentChanged(*this, c);
	for(list<IDocumentListener*>::iterator i(listeners_.begin()), e(listeners_.end()); i != e; ++i)
		(*i)->documentChanged(*this, c);
}

#define ASCENSION_DOCUMENT_INSERT_PROLOGUE()								\
	if(changing_ || isReadOnly())											\
		throw ReadOnlyDocumentException();									\
	else if(at.line >= numberOfLines() || at.column > lineLength(at.line))	\
		throw BadPositionException();										\
	else if(isNarrowed() && !accessibleRegion().includes(at))				\
		throw DocumentAccessViolationException();							\
	ModificationGuard guard(*this);											\
	if(!fireDocumentAboutToBeChanged(DocumentChange(false, Region(at))))	\
		return false;														\
	Position resultPosition(at)

#define ASCENSION_DOCUMENT_INSERT_EPILOGUE()																	\
	if(recordingOperations_)																					\
		undoManager_->pushUndoableOperation(*(new DeleteOperation(Region(at, resultPosition))));				\
	/* notify the change */																						\
	++revisionNumber_;																							\
	fireDocumentChanged(DocumentChange(false, Region(at, resultPosition)));										\
	stateListeners_.notify<const Document&>(&IDocumentStateListener::documentModificationSignChanged, *this);	\
/*	assert(length_ == calculateDocumentLength(*this));	/* diagnose length_  */									\
	if(eos != 0)																								\
		*eos = resultPosition;																					\
	return true

/**
 * Inserts the text into the specified position.
 * This method sets the modification flag and calls the listeners'
 * @c IDocumentListener#documentAboutToBeChanged and @c IDocumentListener#documentChanged.
 * @param at the position
 * @param first the start of the text
 * @param last the end of the text
 * @return the result position
 * @throw BadPositionException @a at is outside of the document
 * @throw ReadOnlyDocumentException the document is read only
 * @throw DocumentAccessViolationException @a at is inside of the inaccessible region
 * @throw NullPointerException either @a first or @a last is @c null
 * @throw std#invalid_argument either @a first is greater than @a last
 */
bool Document::insert(const Position& at, const Char* first, const Char* last, Position* eos /* = 0 */) {
	if(first == 0)
		throw NullPointerException("first");
	else if(last == 0)
		throw NullPointerException("last");
	else if(first > last)
		throw invalid_argument("first > last");
	ASCENSION_DOCUMENT_INSERT_PROLOGUE();
	resultPosition = insertText(at, first, last);
	ASCENSION_DOCUMENT_INSERT_EPILOGUE();
}

/**
 * Inserts the text provided by the given stream into the specified position. For details, see the
 * documentation of @c #insert(const Position&, const Char*, const Char*)
 * @param at the position
 * @param in the input stream
 * @return the position to where the caret will move
 * @throw BadPositionException @a at is outside of the document
 * @throw ReadOnlyDocumentException the document is read only
 * @throw DocumentAccessViolationException @a at is inside of the inaccessible region
 * @see writeDocumentToStream
 */
bool Document::insert(const Position& at, basic_istream<Char>& in, Position* eos /* = 0 */) {
	ASCENSION_DOCUMENT_INSERT_PROLOGUE();
	Char buffer[8192];
	while(in) {
		in.read(buffer, MANAH_COUNTOF(buffer));
		if(in.gcount() == 0)
			break;
		resultPosition = insertText(resultPosition, buffer, buffer + in.gcount());
	}
	ASCENSION_DOCUMENT_INSERT_EPILOGUE();
}

// called by insert(const Position&, const Char*, const Char*) and insert(const Position&, basic_istream<Char>&).
Position Document::insertText(const Position& position, const Char* first, const Char* last) {
	Position resultPosition(position.line, 0);
	const Char* breakPoint = find_first_of(first, last, NEWLINE_CHARACTERS, MANAH_ENDOF(NEWLINE_CHARACTERS));

	if(breakPoint == last) {	// single-line
		Line& line = const_cast<Line&>(getLineInformation(position.line));
		line.text_.insert(position.column, first, static_cast<String::size_type>(last - first));
		length_ += static_cast<length_t>(last - first);
		++line.operationHistory_;
		resultPosition.column = position.column + (last - first);
	} else {	// multiline
		length_t line = position.line;
		Line& firstLine = *lines_[line];
		const Char* lastBreak;
		const Newline firstNewline = firstLine.newline_;	// 先頭行の改行文字 (挿入後、一番後ろに付けられる)

		// 最後の改行位置を探し、resultPosition の文字位置も決定する
		for(lastBreak = last - 1; ; --lastBreak) {
			if(binary_search(NEWLINE_CHARACTERS, MANAH_ENDOF(NEWLINE_CHARACTERS), *lastBreak))
				break;
		}
		resultPosition.column = static_cast<length_t>((last - first) - (lastBreak - first) - 1);
		if(*lastBreak == LINE_FEED && lastBreak != breakPoint && *(lastBreak - 1) == CARRIAGE_RETURN)
			--lastBreak;

		// 先頭行の置換
		const String firstLineRest = firstLine.text_.substr(position.column, firstLine.text_.length() - position.column);
		length_ += breakPoint - first - firstLineRest.length();
		firstLine.text_.replace(position.column, firstLineRest.length(), first, static_cast<String::size_type>(breakPoint - first));
		firstLine.newline_ = eatNewline(breakPoint, last);
		assert(firstLine.newline_ != NLF_RAW_VALUE);
		breakPoint += (firstLine.newline_ != NLF_CR_LF) ? 1 : 2;
		++firstLine.operationHistory_;
		++line;
		++resultPosition.line;

		// 改行ごとに行に区切っていく
		while(true) {
			if(breakPoint <= lastBreak) {
				const Char* const nextBreak =
					find_first_of(breakPoint, last, NEWLINE_CHARACTERS, MANAH_ENDOF(NEWLINE_CHARACTERS));
				assert(nextBreak != last);
				const Newline newline = eatNewline(nextBreak, last);

				length_ += nextBreak - breakPoint;
				lines_.insert(line, new Line(String(breakPoint, nextBreak), newline, true));
				++line;
				++resultPosition.line;
				breakPoint = nextBreak + ((newline != NLF_CR_LF) ? 1 : 2);
			} else {	// 最終行
				length_ += last - breakPoint + firstLineRest.length();
				lines_.insert(line, new Line(String(breakPoint, last) + firstLineRest, firstNewline, true));
				break;
			}
		}
	}

	return resultPosition;
}

/**
 * Returns true if the document is compound changing.
 * @see #beginSequentialEdit, #endSequentialEdit
 */
bool Document::isCompoundChanging() const throw() {
	return undoManager_->isStackingCompoundOperation();
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
			len += getNewlineStringLength(lines_[i]->newline_);
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
		throw BadPositionException();
	newline = resolveNewline(*this, newline);

	length_t offset = 0, eolLength = isLiteralNewline(newline) ? getNewlineStringLength(newline) : 0;
	if(eolLength == 0 && newline != NLF_RAW_VALUE)
		throw UnknownValueException("newline");
	for(length_t i = 0; i < line; ++i) {
		const Line& ln = *lines_[i];
		offset += ln.text_.length();
		if(newline == NLF_RAW_VALUE)
			offset += getNewlineStringLength(ln.newline_);
		else
			offset += eolLength;
	}
	return offset;
}

/**
 * Marks the document modified. There is not a method like @c markModified.
 * @see #isModified, IDocumentStateListener#documentModificationSignChanged
 */
void Document::markUnmodified() throw() {
	if(isModified()) {
		lastUnmodifiedRevisionNumber_ = revisionNumber();
		stateListeners_.notify<const Document&>(&IDocumentStateListener::documentModificationSignChanged, *this);
	}
}

/**
 * Narrows the accessible area to the specified region.
 * @param region the region
 * @see #isNarrowed, #widen
 */
void Document::narrow(const Region& region) {
	if(accessibleArea_ == 0)
		accessibleArea_ = new pair<Position, Point*>;
	accessibleArea_->first = region.beginning();
	accessibleArea_->second = new Point(*this);
	accessibleArea_->second->moveTo(region.end());
	for(set<Point*>::iterator i = points_.begin(); i != points_.end(); ++i) {
		if((*i)->isExcludedFromRestriction())
			(*i)->normalize();
	}
	stateListeners_.notify<const Document&>(&IDocumentStateListener::documentAccessibleRegionChanged, *this);
}

/// Returns the number of undoable edits.
size_t Document::numberOfUndoableEdits() const throw() {
	return undoManager_->numberOfUndoableCompoundOperations();
}

/// Returns the number of redoable edits.
size_t Document::numberOfRedoableEdits() const throw() {
	return undoManager_->numberOfRedoableCompoundOperations();
}

/**
 * Sets whether the document records or not the operations for undo/redo.
 *
 * The default is true. If change the setting, recorded contents will be disposed.
 * @param record set true to record
 * @see #isRecordingOperations, #undo, #redo
 */
void Document::recordOperations(bool record) {
	if(!(recordingOperations_ = record))
		clearUndoBuffer();
}

/**
 * Performs the redo. Does nothing if the target region is inaccessible.
 * @return false if the redo was not completely performed
 * @throw ReadOnlyDocumentException the document is read only
 * @see #undo
 */
bool Document::redo() {
	if(isReadOnly())
		throw ReadOnlyDocumentException();
	else if(numberOfRedoableEdits() == 0)
		return false;

	beginCompoundChange();
	rollbackListeners_.notify<const Document&>(&IDocumentRollbackListener::documentUndoSequenceStarted, *this);
	Position resultPosition;
	const bool succeeded = undoManager_->redo(resultPosition).first;
	rollbackListeners_.notify<const Document&, const Position&>(
		&IDocumentRollbackListener::documentUndoSequenceStopped, *this, resultPosition);
	endCompoundChange();
	return succeeded;
}

/**
 * Removes the compound change listener.
 * @param listener the listener to be removed
 * @throw std#invalid_argument @a listener is not registered
 */
void Document::removeCompoundChangeListener(ICompoundChangeListener& listener) {
	compoundChangeListeners_.remove(listener);
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
 * @see #doResetContent
 */
void Document::resetContent() {
	if(lines_.empty())	// called by constructor
		lines_.insert(lines_.begin(), new Line);
	else {
		widen();
		for(set<Point*>::iterator i(points_.begin()), e(points_.end()); i != e; ++i)
			(*i)->moveTo(0, 0);

		const DocumentChange ca(true, region());
		fireDocumentAboutToBeChanged(ca);
		if(length_ != 0) {
			assert(!lines_.empty());
			lines_.clear();
			lines_.insert(lines_.begin(), new Line);
			length_ = 0;
			++revisionNumber_;
		}
		fireDocumentChanged(ca, false);
	}

	setReadOnly(false);
	markUnmodified();
	clearUndoBuffer();
	onceUndoBufferCleared_ = false;
	doResetContent();
}

#if 0
/**
 * Sends the document to the user's mailer.
 * @param asAttachment true to send as attachment. in this case, the modification  is not 
 * 現在の変更は反映されない。本文として送信する場合は現在のドキュメントが使用される
 * @param showDialog true to show the user selection dialog
 * @return true if succeeded
 * @deprecated 0.8
 */
bool Document::sendFile(bool asAttachment, bool showDialog /* = true */) {
	if(asAttachment && getFilePathName() == 0)
		return false;

	ui::WaitCursor wc;
	HMODULE dll = ::LoadLibraryW(L"MAPI32.DLL");
	if(dll == 0)
		return false;

	MAPISENDMAIL* MAPISendMailPtr = reinterpret_cast<MAPISENDMAIL*>(::GetProcAddress(dll, "MAPISendMail"));
	if(MAPISendMailPtr == 0) {
		::FreeLibrary(dll);
		return false;
	}

	MANAH_AUTO_STRUCT(::MapiMessage, message);
	ulong error;

	message.flFlags = MAPI_RECEIPT_REQUESTED;

	if(asAttachment) {	// 添付ファイルにするとき
		MANAH_AUTO_STRUCT(::MapiFileDesc, fileDesc);
		const int cb = ::WideCharToMultiByte(CP_ACP, 0,
							getFilePathName(), static_cast<int>(wcslen(getFileName())), 0, 0, 0, 0);
		char* const filePath = new char[cb + 1];

		::WideCharToMultiByte(CP_ACP, 0, getFilePathName(), static_cast<int>(wcslen(getFileName())), filePath, cb, 0, 0);
		filePath[cb] = 0;
		message.nFileCount = 1;
		message.lpFiles = &fileDesc;

		fileDesc.lpszPathName = filePath;
		fileDesc.nPosition = static_cast<ulong>(-1);
		error = MAPISendMailPtr(0, 0, &message, MAPI_LOGON_UI | (showDialog ? MAPI_DIALOG : 0), 0);
		delete[] filePath;
	} else {	// 本文として送信するとき
		wchar_t* const content = new wchar_t[length_ + 1];
		for(length_t i = 0, offset = 0; ; ++i) {
			const Line& line = *lines_[i];
			wmemcpy(content + offset, line.text_.data(), line.text_.length());
			offset += line.text_.length();
			if(i != lines_.getSize() - 1) {
				wcscpy(content + offset, getNewlineString(line.newline_));
				offset += getNewlineStringLength(line.newline_);
			} else
				break;
		}
		content[length_] = 0;

		// ユーザ既定のマルチバイト文字列に変換
		const int contentSize = ::WideCharToMultiByte(CP_ACP, 0, content, static_cast<int>(length_), 0, 0, 0, 0);
		char* const	nativeContent = new char[contentSize + 1];
		::WideCharToMultiByte(CP_ACP, 0, content, static_cast<int>(length_), nativeContent, contentSize, 0, 0);
		nativeContent[contentSize] = 0;
		message.lpszNoteText = nativeContent;
		delete[] content;
		error = MAPISendMailPtr(0, 0, &message, MAPI_LOGON_UI | (showDialog ? MAPI_DIALOG : 0), 0);
		delete[] nativeContent;
	}

	::FreeLibrary(dll);
	return error == SUCCESS_SUCCESS || error == MAPI_USER_ABORT || error == MAPI_E_LOGIN_FAILURE;
}
#endif /* 0 */

/**
 * Sets the new document input.
 * @param newInput the new document input. can be @c null
 * @param delegateOwnership set true to transfer the ownership into the callee
 */
void Document::setInput(IDocumentInput* newInput, bool delegateOwnership) throw() {
	input_.reset(newInput, delegateOwnership);
}

/**
 * Sets the new document partitioner.
 * @param newPartitioner the new partitioner. the ownership will be transferred to the callee
 */
void Document::setPartitioner(auto_ptr<DocumentPartitioner> newPartitioner) throw() {
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
	std::map<const DocumentPropertyKey*, String*>::iterator i(properties_.find(&key));
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
void Document::setReadOnly(bool readOnly /* = true */) {
	if(readOnly != isReadOnly()) {
		readOnly_ = readOnly;
		stateListeners_.notify<const Document&>(&IDocumentStateListener::documentReadOnlySignChanged, *this);
	}
}

#if 0
/**
 * Translates the special Win32 code page to concrete one.
 * @param cp the code page to be translated
 * @return the concrete code page
 * @deprecated 0.8
 */
::UINT Document::translateSpecialCodePage(::UINT codePage) {
	if(codePage == CP_ACP)
		return ::GetACP();
	else if(codePage == CP_OEMCP)
		return ::GetOEMCP();
	else if(codePage == CP_MACCP) {
		wchar_t	wsz[7];
		::GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_IDEFAULTMACCODEPAGE, wsz, 6);
		return (wcscmp(wsz, L"2") != 0) ? wcstoul(wsz, 0, 10) : 0;
	} else if(codePage == CP_THREAD_ACP) {
		wchar_t	wsz[7];
		::GetLocaleInfoW(::GetThreadLocale(), LOCALE_IDEFAULTANSICODEPAGE, wsz, 6);
		return (wcscmp(wsz, L"3") != 0) ? wcstoul(wsz, 0, 10) : 0;
	}
	return codePage;
}
#endif /* 0 */

/**
 * Performs the undo. Does nothing if the target region is inaccessible.
 * @return false if the undo was not completely performed
 * @throw ReadOnlyDocumentException the document is read only
 * @see #redo
 */
bool Document::undo() {
	if(isReadOnly())
		throw ReadOnlyDocumentException();
	else if(numberOfUndoableEdits() == 0)
		return false;

	beginCompoundChange();
	rollbackListeners_.notify<const Document&>(&IDocumentRollbackListener::documentUndoSequenceStarted, *this);
	Position resultPosition;
	const pair<bool, size_t> status(undoManager_->undo(resultPosition));
	rollbackListeners_.notify<const Document&, const Position&>(
		&IDocumentRollbackListener::documentUndoSequenceStopped, *this, resultPosition);
	endCompoundChange();

	revisionNumber_ -= status.second;
	if(!isModified())
		stateListeners_.notify<const Document&>(&IDocumentStateListener::documentModificationSignChanged, *this);
	return status.first;
}

/**
 * Informs the document change to the adapting points.
 * @param change the document change
 */
inline void Document::updatePoints(const DocumentChange& change) throw() {
	for(set<Point*>::iterator i = points_.begin(); i != points_.end(); ++i) {
		if((*i)->adaptsToDocument())
			(*i)->update(change);
	}
}

/**
 * Revokes the narrowing.
 * @see #isNarrowed, #narrow
 */
void Document::widen() {
	if(accessibleArea_ != 0) {
		delete accessibleArea_->second;
		delete accessibleArea_;
		accessibleArea_ = 0;
		stateListeners_.notify<const Document&>(&IDocumentStateListener::documentAccessibleRegionChanged, *this);
	}
}


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
DocumentCharacterIterator::DocumentCharacterIterator() throw() : CharacterIterator(CONCRETE_TYPE_TAG_), document_(0), line_(0) {
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
		throw BadPositionException();
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
		throw BadRegionException();
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
		throw BadRegionException();
	else if(!region_.includes(p_))
		throw BadPositionException();
}

/// Copy-constructor.
DocumentCharacterIterator::DocumentCharacterIterator(const DocumentCharacterIterator& rhs) throw() :
		text::CharacterIterator(rhs), document_(rhs.document_), region_(rhs.region_), line_(rhs.line_), p_(rhs.p_) {
}

/// @see text#CharacterIterator#current
CodePoint DocumentCharacterIterator::current() const throw() {
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


// DocumentBuffer ///////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param document the document
 * @param initialPosition the initial position of streams
 * @param nlr the newline representation
 * @param streamMode the streaming mode. this can be @c std#ios_base#in and @c std#ios_base#out
 * @throw UnknownValueException @a streamMode is invalid
 */
DocumentBuffer::DocumentBuffer(Document& document, const Position& initialPosition /* = Position::ZERO_POSITION */,
		Newline newline /* = NLF_RAW_VALUE */, ios_base::openmode streamMode /* = ios_base::in | ios_base::out */) :
		document_(document), newline_(newline), mode_(streamMode), current_(initialPosition) {
	if((mode_ & ~(ios_base::in | ios_base::out)) != 0)
		throw UnknownValueException("streamMode");
	setp(buffer_, MANAH_ENDOF(buffer_) - 1);
}

/// Destructor.
DocumentBuffer::~DocumentBuffer() throw() {
	sync();
}

/// Returns the current position in the document.
const Position& DocumentBuffer::tell() const throw() {
	return current_;
}

/// @see std#basic_streambuf#overflow
DocumentBuffer::int_type DocumentBuffer::overflow(int_type c) {
	if((mode_ & ios_base::out) == 0)
		return traits_type::eof();
	char_type* p = pptr();
	if(!traits_type::eq_int_type(c, traits_type::eof()))
		*p++ = traits_type::to_char_type(c);
	setp(buffer_, MANAH_ENDOF(buffer_) - 1);
	if(buffer_ < p)
		document_.insert(current_, buffer_, p, &current_);
	return traits_type::not_eof(c);
}

/// @see std#basic_streambuf#sync
int DocumentBuffer::sync() {
	if((mode_ & ios_base::out) != 0)
		return traits_type::eq_int_type(overflow(traits_type::eof()), traits_type::eof()) ? -1 : 0;
	else
		return 0;
}

/// @see std#basic_streambuf#uflow
DocumentBuffer::int_type DocumentBuffer::uflow() {
	if(gptr() != egptr()) {
		const int_type temp = traits_type::to_int_type(*gptr());
		gbump(1);
		return temp;
	} else
		return traits_type::eof();
}

/// @see std#basic_streambuf#underflow
DocumentBuffer::int_type DocumentBuffer::underflow() {
	return (gptr() != egptr()) ? traits_type::to_int_type(*gptr()) : traits_type::eof();
}


// document stream classes //////////////////////////////////////////////////

/// Constructor.
DocumentInputStream::DocumentInputStream(Document& document, const Position& initialPosition /* = Position::ZERO_POSITION */,
		Newline newline /* = NLF_RAW_VALUE */) : basic_istream<Char>(&buffer_), buffer_(document, initialPosition, newline) {
}

/// Constructor.
DocumentOutputStream::DocumentOutputStream(Document& document, const Position& initialPosition /* = Position::ZERO_POSITION */,
		Newline newline /* = NLF_RAW_VALUE */) : basic_ostream<Char>(&buffer_), buffer_(document, initialPosition, newline) {
}

/// Constructor.
DocumentStream::DocumentStream(Document& document, const Position& initialPosition /* = Position::ZERO_POSITION */,
		Newline newline /* = NLF_RAW_VALUE */) : basic_iostream<Char>(&buffer_), buffer_(document, initialPosition, newline) {
}


// NullPartitioner //////////////////////////////////////////////////////////

/// Constructor.
NullPartitioner::NullPartitioner() throw() : p_(DEFAULT_CONTENT_TYPE, Region(Position::ZERO_POSITION, Position::INVALID_POSITION)) {
}

/// @see DocumentPartitioner#documentAboutToBeChanged
void NullPartitioner::documentAboutToBeChanged() throw() {
}

/// @see DocumentPartitioner#documentChanged
void NullPartitioner::documentChanged(const DocumentChange&) throw() {
	p_.region.second.line = INVALID_INDEX;
}

/// @see DocumentPartitioner#doGetPartition
void NullPartitioner::doGetPartition(const Position&, DocumentPartition& partition) const throw() {
	if(p_.region.second.line == INVALID_INDEX)
		const_cast<NullPartitioner*>(this)->p_.region.second = document()->region().second;
	partition = p_;
}

/// @see DocumentPartitioner#doInstall
void NullPartitioner::doInstall() throw() {
	p_.region.second.line = INVALID_INDEX;
}
