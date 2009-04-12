/**
 * @file undo.cpp
 * @author exeal
 * @date 2009 separated from document.cpp
 */

#include <ascension/document.hpp>
#include <ascension/point.hpp>
#include <stack>
#include <vector>
using namespace ascension;
using namespace ascension::kernel;
using namespace std;


// local entities ///////////////////////////////////////////////////////////

namespace {
	class IAtomicChange;

	// an abstract edit operation
	class IUndoableChange {
	public:
		// result of IUndoableChange.perform
		struct Result {
			bool completed;				// true if the change was *completely* performed
			size_t numberOfRevisions;	// the number of the performed changes
			Position endOfChange;		// the end position of the change
			void reset() /*throw()*/ {completed = false; numberOfRevisions = 0; endOfChange = Position::INVALID_POSITION;}
		};
	public:
		// destructor
		virtual ~IUndoableChange() /*throw()*/ {}
		// appends the "postChange" to this change and returns true, or returns false
		virtual bool appendChange(IAtomicChange& postChange, const Document& document) = 0;
		// returns true if the change can perform
		virtual bool canPerform(const Document& document) const = 0;
		// performs the change. this method may fail
		virtual void perform(Document& document, Result& result) = 0;
	};

	// base interface of InsertionChange and DeletionChange
	class IAtomicChange : public IUndoableChange {
	public:
		struct TypeTag {};	// ugly dynamic type system for performance reason
		virtual ~IAtomicChange() /*throw()*/ {}
		virtual const TypeTag& type() const /*throw()*/ = 0;
	};

	// an atomic insertion change
	class InsertionChange : public IAtomicChange, public manah::FastArenaObject<InsertionChange> {
	public:
		InsertionChange(const Position& position, const String& text) : position_(position), text_(text) {}
		bool appendChange(IAtomicChange&, const Document&) /*throw()*/ {return false;}
		bool canPerform(const Document& document) const /*throw()*/ {return !document.isNarrowed() || document.region().includes(position_);}
		void perform(Document& document, Result& result);
	private:
		const TypeTag& type() const /*throw()*/ {return type_;}
	private:
		const Position position_;
		const String text_;
		static const TypeTag type_;
	};

	// an atomic deletion change
	class DeletionChange : public IAtomicChange, public manah::FastArenaObject<DeletionChange> {
	public:
		explicit DeletionChange(const Region& region) /*throw()*/ : region_(region), revisions_(1) {}
		bool appendChange(IAtomicChange& postChange, const Document&) /*throw()*/;
		bool canPerform(const Document& document) const /*throw()*/ {return !document.isNarrowed() || document.region().encompasses(region_);}
		void perform(Document& document, Result& result);
	private:
		const TypeTag& type() const /*throw()*/ {return type_;}
	private:
		Region region_;
		size_t revisions_;
		static const TypeTag type_;
	};

	// an atomic replacement change
	class ReplacementChange : public IAtomicChange, public manah::FastArenaObject<ReplacementChange> {
	public:
		explicit ReplacementChange(const Region& region, const String& text) : region_(region), text_(text) {}
		bool appendChange(IAtomicChange&, const Document&) /*throw()*/ {return false;}
		bool canPerform(const Document& document) const /*throw()*/ {return !document.isNarrowed() || document.region().encompasses(region_);}
		void perform(Document& document, Result& result);
	private:
		const TypeTag& type() const /*throw()*/ {return type_;}
	private:
		const Region region_;
		const String text_;
		static const TypeTag type_;
	};

	// a compound change
	class CompoundChange : public IUndoableChange {
	public:
		~CompoundChange() /*throw()*/;
		bool appendChange(IAtomicChange& postChange, const Document& document);
		bool canPerform(const Document& document) const /*throw()*/ {return !changes_.empty() && changes_.back()->canPerform(document);}
		void perform(Document& document, Result& result);
	private:
		vector<IAtomicChange*> changes_;
	};

	// static members initialization (inherent parens? see "Exceptional C++ Style" item 29)
	const IAtomicChange::TypeTag InsertionChange::type_((IAtomicChange::TypeTag()));
	const IAtomicChange::TypeTag DeletionChange::type_((IAtomicChange::TypeTag()));
	const IAtomicChange::TypeTag ReplacementChange::type_((IAtomicChange::TypeTag()));

	inline void InsertionChange::perform(Document& document, Result& result) {
//		try {
			insert(document, position_, text_, &result.endOfChange);
//		} catch(...) {
//			result.reset();
//		}
		result.completed = true;
		result.numberOfRevisions = 1;
	}

	inline bool DeletionChange::appendChange(IAtomicChange& postChange, const Document&) {
		if(&postChange.type() != &type_)
			return false;
		const Position& bottom = region_.end();
		if(bottom.column == 0 || bottom != static_cast<DeletionChange&>(postChange).region_.beginning())
			return false;
		else {
			region_.end() = static_cast<DeletionChange&>(postChange).region_.end();
			delete &postChange;
			++revisions_;
			return true;
		}
	}

	inline void DeletionChange::perform(Document& document, Result& result) {
//		try {
			erase(document, region_);
//		} catch(...) {
//			result.reset();
//		}
		result.completed = true;
		result.numberOfRevisions = revisions_;
		result.endOfChange = region_.first;
	}

	inline void ReplacementChange::perform(Document& document, Result& result) {
		replace(document, region_, text_, &result.endOfChange);
		result.completed = true;
		result.numberOfRevisions = 1;
	}

	CompoundChange::~CompoundChange() /*throw()*/ {
		for(vector<IAtomicChange*>::iterator i(changes_.begin()), e(changes_.end()); i != e; ++i)
			delete *i;
	}

	inline bool CompoundChange::appendChange(IAtomicChange& postChange, const Document& document) {
		if(changes_.empty() || !changes_.back()->appendChange(postChange, document))
			changes_.push_back(&postChange);
		return true;
	}

	void CompoundChange::perform(Document& document, Result& result) {
		assert(!changes_.empty());
		result.reset();
		Result delta;
		vector<IAtomicChange*>::iterator i(changes_.end()), e(changes_.begin());
		for(--i; ; --i) {
			(*i)->perform(document, delta);
			result.numberOfRevisions += delta.numberOfRevisions;
			if(!delta.completed) {
				if(i != --changes_.end())
					// partially completed
					changes_.erase(++i, changes_.end());
				result.endOfChange = delta.endOfChange;
				break;
			} else if(i == e) {	// completed
				result.completed = true;
				result.endOfChange = delta.endOfChange;
				break;
			}
		}
	}
} // namespace @0


// Document.UndoManager /////////////////////////////////////////////////////

// manages undo/redo of the document.
class Document::UndoManager {
	MANAH_NONCOPYABLE_TAG(UndoManager);
public:
	// constructors
	explicit UndoManager(Document& document) /*throw()*/;
	virtual ~UndoManager() /*throw()*/ {clear();}
	// attributes
	size_t numberOfRedoableChanges() const /*throw()*/ {return redoableChanges_.size() + ((pendingAtomicChange_.get() != 0) ? 1 : 0);}
	size_t numberOfUndoableChanges() const /*throw()*/ {return undoableChanges_.size() + ((pendingAtomicChange_.get() != 0) ? 1 : 0);}
	bool isStackingCompoundOperation() const /*throw()*/ {return compoundChangeDepth_ > 0;}
	// rollbacks
	void redo(IUndoableChange::Result& result);
	void undo(IUndoableChange::Result& result);
	// recordings
	void addUndoableChange(IAtomicChange& c);
	void beginCompoundChange() /*throw()*/ {++compoundChangeDepth_;}
	void clear() /*throw()*/;
	void endCompoundChange() /*throw()*/;
	void insertBoundary() /*throw()*/;
private:
	void commitPendingChange(bool beginCompound);
	Document& document_;
	stack<IUndoableChange*> undoableChanges_, redoableChanges_;
	auto_ptr<IAtomicChange> pendingAtomicChange_;
	size_t compoundChangeDepth_;
	bool rollbacking_;
	CompoundChange* rollbackingChange_;
	CompoundChange* currentCompoundChange_;
};

// constructor takes the target document
Document::UndoManager::UndoManager(Document& document) /*throw()*/ : document_(document),
		compoundChangeDepth_(0), rollbacking_(false), rollbackingChange_(0), currentCompoundChange_(0) {
}

// pushes the operation into the undo stack
void Document::UndoManager::addUndoableChange(IAtomicChange& c) {
	if(!rollbacking_) {
		if(currentCompoundChange_ != 0)
			currentCompoundChange_->appendChange(c, document_);	// CompoundChange.appendChange always returns true
		else if(pendingAtomicChange_.get() != 0) {
			if(!pendingAtomicChange_->appendChange(c, document_)) {
				commitPendingChange(true);
				currentCompoundChange_->appendChange(c, document_);	// CompoundChange.appendChange always returns true
			}
		} else
			pendingAtomicChange_.reset(&c);

		// make the redo stack empty
		while(!redoableChanges_.empty()) {
			delete redoableChanges_.top();
			redoableChanges_.pop();
		}
	} else {
		// delay pushing to the stack when rollbacking
		if(rollbackingChange_ == 0)
			rollbackingChange_ = new CompoundChange();
		rollbackingChange_->appendChange(c, document_);	// CompoundChange.appendChange always returns true
	}
}

// clears the stacks
inline void Document::UndoManager::clear() /*throw()*/ {
	while(!undoableChanges_.empty()) {
		delete undoableChanges_.top();
		undoableChanges_.pop();
	}
	while(!redoableChanges_.empty()) {
		delete redoableChanges_.top();
		redoableChanges_.pop();
	}
	pendingAtomicChange_.reset();
	compoundChangeDepth_ = 0;
	currentCompoundChange_ = 0;
}

// commits the pending undoable change
inline void Document::UndoManager::commitPendingChange(bool beginCompound) {
	if(pendingAtomicChange_.get() != 0) {
		if(beginCompound) {
			auto_ptr<CompoundChange> newCompound(new CompoundChange());
			newCompound->appendChange(*pendingAtomicChange_.get(), document_);
			undoableChanges_.push(newCompound.release());
			pendingAtomicChange_.release();
			currentCompoundChange_ = static_cast<CompoundChange*>(undoableChanges_.top());	// safe down cast
		} else {
			if(currentCompoundChange_ == 0 || !currentCompoundChange_->appendChange(*pendingAtomicChange_.get(), document_)) {
				undoableChanges_.push(pendingAtomicChange_.get());
				currentCompoundChange_ = 0;
			}
			pendingAtomicChange_.release();
		}
	}
}

// ends the compound change
inline void Document::UndoManager::endCompoundChange() /*throw()*/ {
	if(compoundChangeDepth_ == 0)
// this does not throw IllegalStateException even if the internal counter is zero, because undo()
// and redo() reset the counter to zero.
//		throw IllegalStateException("there is no compound change in this document.");
		return;
	--compoundChangeDepth_;
}

// stops the current compound chaining
inline void Document::UndoManager::insertBoundary() {
	if(compoundChangeDepth_ == 0)
		commitPendingChange(false);
}

// redoes one change
void Document::UndoManager::redo(IUndoableChange::Result& result) {
	commitPendingChange(false);
	if(redoableChanges_.empty()) {
		result.reset();
		return;
	}
	IUndoableChange* c = redoableChanges_.top();
	rollbacking_ = true;
	c->perform(document_, result);
	if(result.completed)
		redoableChanges_.pop();
	if(rollbackingChange_ != 0)
		undoableChanges_.push(rollbackingChange_);	// move the rollbcked change(s) into the undo stack
	rollbackingChange_ = currentCompoundChange_ = 0;
	rollbacking_ = false;
	if(result.completed)
		delete c;
	compoundChangeDepth_ = 0;
}

// undoes one change
void Document::UndoManager::undo(IUndoableChange::Result& result) {
	commitPendingChange(false);
	if(undoableChanges_.empty()) {
		result.reset();
		return;
	}
	IUndoableChange* c = undoableChanges_.top();
	rollbacking_ = true;
	c->perform(document_, result);
	if(result.completed)
		undoableChanges_.pop();
	if(rollbackingChange_ != 0)
		redoableChanges_.push(rollbackingChange_);	// move the rollbacked change(s) into the redo stack
	rollbackingChange_ = currentCompoundChange_ = 0;
	rollbacking_ = false;
	if(result.completed)
		delete c;
	compoundChangeDepth_ = 0;
}


// Document /////////////////////////////////////////////////////////////////

/// Constructor.
Document::Document() : session_(0), partitioner_(0),
		contentTypeInformationProvider_(new DefaultContentTypeInformationProvider),
		readOnly_(false), length_(0), revisionNumber_(0), lastUnmodifiedRevisionNumber_(0),
		onceUndoBufferCleared_(false), recordingChanges_(true), changing_(false), /*locker_(0),*/ accessibleArea_(0) {
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
	for(size_t i = 0, c = lines_.size(); i < c; ++i)
		delete lines_[i];
}

/**
 * Starts the compound change.
 * @throw ReadOnlyDocumentException the document is read only
 * @see #endCompoundChange, #isCompoundChanging
 */
void Document::beginCompoundChange() {
//	const bool init = !undoManager_->isStackingCompoundOperation();
	undoManager_->beginCompoundChange();
//	if(init)
//		compoundChangeListeners_.notify<const Document&>(&ICompoundChangeListener::documentCompoundChangeStarted, *this);
}

/// Clears the undo/redo stacks and deletes the history.
void Document::clearUndoBuffer() /*throw()*/ {
	undoManager_->clear();
	onceUndoBufferCleared_ = true;
}

/**
 * Ends the active compound change.
 * @throw IllegalStateException there is no compound change in this document
 * @see #beginCompoundChange, #isCompoundChanging
 */
void Document::endCompoundChange() {
	undoManager_->endCompoundChange();	// this may throw IllegalStateException (this is doubt now. see UndoManager)
//	if(!undoManager_->isStackingCompoundOperation())
//		compoundChangeListeners_.notify<const Document&>(&ICompoundChangeListener::documentCompoundChangeStopped, *this);
}

/**
 * Marks a boundary between units of undo.
 * An undo call will stop at this point. However, see the documentation of @c Document. This method
 * does not throw @c DocumentCantChangeException.
 * @throw ReadOnlyDocumentException the document is read only
 * @throw IllegalStateException the method was called in @c{IDocumentListener}s' notification
 */
void Document::insertUndoBoundary() {
	if(changing_)
		throw IllegalStateException("called in IDocumentListeners' notification.");
	else if(isReadOnly())
		throw ReadOnlyDocumentException();
	undoManager_->insertBoundary();
}

/**
 * Returns true if the document is compound changing.
 * @see #beginCompoundChange, #endCompoundChange
 */
bool Document::isCompoundChanging() const /*throw()*/ {
	return undoManager_->isStackingCompoundOperation();
}

/// Returns the number of undoable changes.
size_t Document::numberOfUndoableChanges() const /*throw()*/ {
	return undoManager_->numberOfUndoableChanges();
}

/// Returns the number of redoable changes.
size_t Document::numberOfRedoableChanges() const /*throw()*/ {
	return undoManager_->numberOfRedoableChanges();
}

/**
 * Sets whether the document records or not the changes for undo/redo. Recording in a newly created
 * document is enabled to start with.
 * @param record if set to true, this method enables the recording and subsequent changes can be
 * undone. if set to false, discards the undo/redo information and disables the recording
 * @see #isRecordingChanges, #undo, #redo
 */
void Document::recordChanges(bool record) /*throw()*/ {
	if(!(recordingChanges_ = record))
		clearUndoBuffer();
}

/**
 * Performs the redo. Does nothing if the target region is inaccessible.
 * @param n the repeat count
 * @return false if the redo was not completely performed
 * @throw ReadOnlyDocumentException the document is read only
 * @std#invalid_argument @a n &gt; #numberOfRedoableChanges()
 * @see #undo
 */
bool Document::redo(size_t n /* = 1 */) {
	if(isReadOnly())
		throw ReadOnlyDocumentException();
	else if(n > numberOfRedoableChanges())
		throw invalid_argument("n");

	IUndoableChange::Result result;
	result.completed = true;
	rollbackListeners_.notify<const Document&>(&IDocumentRollbackListener::documentUndoSequenceStarted, *this);

	for(; n > 0 && result.completed; --n) {
		beginCompoundChange();
		undoManager_->redo(result);
		endCompoundChange();
	}
	assert(n == 0 || !result.completed);

	rollbackListeners_.notify<const Document&, const Position&>(
		&IDocumentRollbackListener::documentUndoSequenceStopped, *this, result.endOfChange);
	return result.completed;
}

#if 0
/**
 * Removes the compound change listener.
 * @param listener the listener to be removed
 * @throw std#invalid_argument @a listener is not registered
 */
void Document::removeCompoundChangeListener(ICompoundChangeListener& listener) {
	compoundChangeListeners_.remove(listener);
}
#endif

/**
 * Substitutes the given text for the specified region in the document.
 * This method sets the modification flag and calls the listeners'
 * @c IDocumentListener#documentAboutToBeChanged and @c IDocumentListener#documentChanged.
 * @param region the region to erase. if this is empty, no text is erased
 * @param first the start of the text. if this is @c null, no text is inserted
 * @param last the end of the text. if @a first is @c null, this is ignored
 * @param[out] eos the position of the end of the inserted text. can be @c null if not needed
 * @throw ReadOnlyDocumentException the document is read only
 * @throw DocumentAccessViolationException @a region intersects the inaccesible region
 * @throw NullPointerException either @a last is @c null
 * @throw std#invalid_argument either @a first is greater than @a last
 * @throw IllegalStateException the method was called in @c{IDocumentListener}s' notification
 * @throw IDocumentInput#ChangeRejectedException the input of the document rejected this change
 * @throw std#bad_alloc the internal memory allocation failed
 */
void Document::replace(const Region& region, const Char* first, const Char* last, Position* eos /* = 0 */) {
	if(first != 0) {
		if(last == 0)
			throw NullPointerException("last");
		else if(first > last)
			throw invalid_argument("first > last");
	}
	if(changing_)
		throw IllegalStateException("called in IDocumentListeners' notification.");
	else if(isReadOnly())
		throw ReadOnlyDocumentException();
	else if(region.end().line >= numberOfLines()
			|| region.first.column > lineLength(region.first.line)
			|| region.second.column > lineLength(region.second.line))
		throw BadRegionException(region);
	else if(isNarrowed() && !accessibleRegion().encompasses(region))
		throw DocumentAccessViolationException();
	else if(!isModified() && input_.get() != 0 && !input_->isChangeable())
		throw IDocumentInput::ChangeRejectedException();
	else if(region.isEmpty() && (first == 0 || first == last))
		return;	// nothing to do

	// preprocess. these can't throw
	ascension::internal::ValueSaver<bool> writeLock(changing_);
	changing_ = true;
	fireDocumentAboutToBeChanged();

	// change the content
	const Position& beginning = region.beginning(), end = region.end();
	const Char* nextNewline = (first != 0 && first != last) ?
		find_first_of(first, last, NEWLINE_CHARACTERS, MANAH_ENDOF(NEWLINE_CHARACTERS)) : 0;
	basic_stringbuf<Char> erasedString;
	length_t erasedStringLength = 0, insertedStringLength = 0;
	Position endOfInsertedString;
	try {
		// simple cases: both erased region and inserted string are single line
		if(beginning.line == end.line && (first == 0 || first == last)) {	// erase in single line
			Line& line = *lines_[beginning.line];
			erasedString.sputn(line.text().data() + beginning.column, static_cast<streamsize>(end.column - beginning.column));
			line.text_.erase(beginning.column, end.column - beginning.column);
			erasedStringLength += end.column - beginning.column;
		} else if(region.isEmpty() && nextNewline == last) {	// insert single line
			lines_[region.first.line]->text_.insert(
				region.first.column, first, static_cast<String::size_type>(last - first));
			insertedStringLength += static_cast<length_t>(last - first);
			endOfInsertedString.line = region.first.line;
			endOfInsertedString.column = region.first.column + (last - first);
		} else if(beginning.line == end.line && nextNewline == last) {	// replace in single line
			lines_[region.first.line]->text_.replace(
				beginning.column, end.column - beginning.column, first, static_cast<String::size_type>(last - first));
			erasedStringLength += end.column - beginning.column;
			insertedStringLength += static_cast<length_t>(last - first);
			endOfInsertedString.line = region.first.line;
			endOfInsertedString.column = region.first.column + (last - first) - (end.column - beginning.column);
		}
		// complex case: erased region and/or inserted string are/is multi-line
		else {
			// 1. save undo information
			if(!region.isEmpty()) {
				for(Position p(beginning); ; ++p.line, p.column = 0) {
					const Line& line = *lines_[p.line];
					const bool last = p.line == end.line;
					const length_t e = !last ? line.text().length() : end.column;
					if(recordingChanges_) {
						erasedString.sputn(line.text().data() + p.column, static_cast<streamsize>(e - p.column));
						if(!last)
							erasedString.sputn(getNewlineString(line.newline()),
								static_cast<streamsize>(getNewlineStringLength(line.newline())));
					}
//					erasedStringLength += e - p.column;
					if(last)
						break;
				}
			}
			// 2. allocate strings (lines except first) to insert newly. only when inserted string was multiline
			vector<Line*> allocatedLines;
			const Char* const firstNewline = nextNewline;
			if(first != 0 && nextNewline != last) {
				try {
					const Char* p = nextNewline + getNewlineStringLength(eatNewline(nextNewline, last));
					while(true) {
						nextNewline = find_first_of(p, last, NEWLINE_CHARACTERS, MANAH_ENDOF(NEWLINE_CHARACTERS));
						auto_ptr<Line> temp(new Line(revisionNumber_ + 1, String(p, nextNewline), eatNewline(nextNewline, last)));
						allocatedLines.push_back(temp.get());
						temp.release();
						insertedStringLength += allocatedLines.back()->text().length();
						if(nextNewline == last)
							break;
						p = nextNewline + getNewlineStringLength(allocatedLines.back()->newline());
					}
					// merge last line
					Line& lastAllocatedLine = *allocatedLines.back();
					endOfInsertedString.line = beginning.line + allocatedLines.size();
					endOfInsertedString.column = lastAllocatedLine.text().length();
					const Line& lastLine = *lines_[end.line];
					lastAllocatedLine.text_.append(lastLine.text(), end.column, lastLine.text().length() - end.column);
					lastAllocatedLine.newline_ = lastLine.newline();
				} catch(...) {
					for(vector<Line*>::iterator i(allocatedLines.begin()), e(allocatedLines.end()); i != e; ++i)
						delete *i;
					throw;
				}
			} else
				endOfInsertedString = beginning;
			try {
				// 3. insert allocated strings
				if(!allocatedLines.empty())
					lines_.insert(end.line + 1, allocatedLines.begin(), allocatedLines.end());
				// 4. replace first line
				Line& firstLine = *lines_[beginning.line];
				const length_t erasedLength = firstLine.text().length() - beginning.column;
				const length_t insertedLength = firstNewline - first;
				try {
					if(!allocatedLines.empty())
						firstLine.text_.replace(beginning.column, erasedLength, first, insertedLength);
					else {
						// join the first and the last line
						const Line& lastLine = *lines_[end.line];
						firstLine.text_.replace(beginning.column, erasedLength,
							lastLine.text().data() + end.column, lastLine.text().length() - end.column);
					}
				} catch(...) {
					for(size_t i = end.line + 1, c = i + allocatedLines.size(); i < c; ++i)
						delete lines_[i];
					lines_.erase(end.line + 1, allocatedLines.size());
					throw;
				}
				if(firstNewline != 0)
					firstLine.newline_ = eatNewline(firstNewline, last);
				erasedStringLength += erasedLength;
				insertedStringLength += insertedLength;
			} catch(...) {
				for(vector<Line*>::iterator i(allocatedLines.begin()), e(allocatedLines.end()); i != e; ++i)
					delete *i;
				throw;
			}
			// 5. remove lines to erase
			if(!region.isEmpty()) {
				for(size_t i = beginning.line + 1; i <= end.line; ++i)
					delete lines_[i];
				lines_.erase(beginning.line + 1, end.line - beginning.line);
			}
		}
	} catch(...) {
		// fire event even if change failed
		const Region empty(beginning);
		fireDocumentChanged(DocumentChange(empty, empty));
		throw;
	}

	if(isRecordingChanges()) {
		if(region.isEmpty())
			undoManager_->addUndoableChange(*(new DeletionChange(Region(beginning, endOfInsertedString))));
		else if(first == 0 || first == last)
			undoManager_->addUndoableChange(*(new InsertionChange(beginning, erasedString.str())));
		else
			undoManager_->addUndoableChange(*(new ReplacementChange(region, erasedString.str())));
	}
	++revisionNumber_;
	length_ += insertedStringLength;
	length_ -= erasedStringLength;

	const DocumentChange change(region, Region(beginning, endOfInsertedString));
	fireDocumentChanged(change);
	stateListeners_.notify<const Document&>(&IDocumentStateListener::documentModificationSignChanged, *this);

	if(eos != 0)
		*eos = endOfInsertedString;
}

/***/
void Document::replace(const Region& region, basic_istream<Char>& in, Position* endOfInsertedString /* = 0 */) {
	// TODO: this implementation is provisional and not exception-safe.
	Position e;
	Char buffer[0x8000];
	for(Region r(region); in; r.first = r.second = e) {
		in.read(buffer, MANAH_COUNTOF(buffer));
		if(in.gcount() == 0)
			break;
		replace(r, buffer, buffer + in.gcount(), &e);
	}
	if(endOfInsertedString != 0)
		*endOfInsertedString = e;
}

/**
 * Performs the undo. Does nothing if the target region is inaccessible.
 * @param n the repeat count
 * @return false if the undo was not completely performed
 * @throw ReadOnlyDocumentException the document is read only
 * @std#invalid_argument @a n &gt; #numberOfUndoableChanges()
 * @see #redo
 */
bool Document::undo(size_t n /* = 1 */) {
	if(isReadOnly())
		throw ReadOnlyDocumentException();
	else if(n > numberOfUndoableChanges())
		throw invalid_argument("n");

	const size_t oldRevisionNumber = revisionNumber_;
	IUndoableChange::Result result;
	result.completed = true;
	rollbackListeners_.notify<const Document&>(&IDocumentRollbackListener::documentUndoSequenceStarted, *this);

	for(; n > 0 && result.completed; --n) {
		beginCompoundChange();
		undoManager_->undo(result);
		endCompoundChange();
		revisionNumber_ = oldRevisionNumber - result.numberOfRevisions;
	}
	assert(n == 0 || !result.completed);

	rollbackListeners_.notify<const Document&, const Position&>(
		&IDocumentRollbackListener::documentUndoSequenceStopped, *this, result.endOfChange);
	if(!isModified())
		stateListeners_.notify<const Document&>(&IDocumentStateListener::documentModificationSignChanged, *this);
	return result.completed;
}