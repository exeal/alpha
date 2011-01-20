/**
 * @file undo.cpp
 * @author exeal
 * @date 2009 separated from document.cpp
 * @date 2010-2011
 */

#include <ascension/kernel/document.hpp>
#include <ascension/kernel/point.hpp>
#include <stack>
#include <vector>
using namespace ascension;
using namespace ascension::kernel;
using namespace std;


// local entities /////////////////////////////////////////////////////////////////////////////////

namespace {
	class AtomicChange;

	// an abstract edit operation
	class UndoableChange {
	public:
		// result of IUndoableChange.perform
		struct Result {
			bool completed;				// true if the change was *completely* performed
			size_t numberOfRevisions;	// the number of the performed changes
			Position endOfChange;		// the end position of the change
			void reset() /*throw()*/ {completed = false; numberOfRevisions = 0; endOfChange = Position();}
		};
	public:
		// destructor
		virtual ~UndoableChange() /*throw()*/ {}
		// appends the "postChange" to this change and returns true, or returns false
		virtual bool appendChange(AtomicChange& postChange, const Document& document) = 0;
		// returns true if the change can perform
		virtual bool canPerform(const Document& document) const = 0;
		// performs the change. this method may fail
		virtual void perform(Document& document, Result& result) = 0;
	};

	// base interface of InsertionChange and DeletionChange
	class AtomicChange : public UndoableChange {
	public:
		struct TypeTag {};	// ugly dynamic type system for performance reason
		virtual ~AtomicChange() /*throw()*/ {}
		virtual const TypeTag& type() const /*throw()*/ = 0;
	};

	// an atomic insertion change
	class InsertionChange : public AtomicChange, public FastArenaObject<InsertionChange> {
	public:
		InsertionChange(const Position& position, const String& text) : position_(position), text_(text) {}
		bool appendChange(AtomicChange&, const Document&) /*throw()*/ {return false;}
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
	class DeletionChange : public AtomicChange, public FastArenaObject<DeletionChange> {
	public:
		explicit DeletionChange(const Region& region) /*throw()*/ : region_(region), revisions_(1) {}
		bool appendChange(AtomicChange& postChange, const Document&) /*throw()*/;
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
	class ReplacementChange : public AtomicChange, public FastArenaObject<ReplacementChange> {
	public:
		explicit ReplacementChange(const Region& region, const String& text) : region_(region), text_(text) {}
		bool appendChange(AtomicChange&, const Document&) /*throw()*/ {return false;}
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
	class CompoundChange : public UndoableChange {
	public:
		~CompoundChange() /*throw()*/;
		bool appendChange(AtomicChange& postChange, const Document& document);
		bool canPerform(const Document& document) const /*throw()*/ {return !changes_.empty() && changes_.back()->canPerform(document);}
		void perform(Document& document, Result& result);
	private:
		vector<AtomicChange*> changes_;
	};

	// static members initialization (inherent parens? see "Exceptional C++ Style" item 29)
	const AtomicChange::TypeTag InsertionChange::type_((AtomicChange::TypeTag()));
	const AtomicChange::TypeTag DeletionChange::type_((AtomicChange::TypeTag()));
	const AtomicChange::TypeTag ReplacementChange::type_((AtomicChange::TypeTag()));

	// implements IUndoableChange.perform
	inline void InsertionChange::perform(Document& document, Result& result) {
		try {
			insert(document, position_, text_, &result.endOfChange);
		} catch(DocumentAccessViolationException&) {
			result.reset();	// the position was inaccessible
		}	// std.bad_alloc is ignored...
		result.completed = true;
		result.numberOfRevisions = 1;
	}

	// implements IUndoableChange.appendChange
	inline bool DeletionChange::appendChange(AtomicChange& postChange, const Document&) {
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

	// implements IUndoableChange.perform
	inline void DeletionChange::perform(Document& document, Result& result) {
		try {
			erase(document, region_);
		} catch(DocumentAccessViolationException&) {
			result.reset();	// the region was inaccessible
		}	// std.bad_alloc is ignored...
		result.completed = true;
		result.numberOfRevisions = revisions_;
		result.endOfChange = region_.first;
	}

	// implements IUndoableChange.perform
	inline void ReplacementChange::perform(Document& document, Result& result) {
		try {
			document.replace(region_, text_, &result.endOfChange);
		} catch(DocumentAccessViolationException&) {
			result.reset();	// the region was inaccessible
		}	// std.bad_alloc is ignored...
		result.completed = true;
		result.numberOfRevisions = 1;
	}

	CompoundChange::~CompoundChange() /*throw()*/ {
		for(vector<AtomicChange*>::iterator i(changes_.begin()), e(changes_.end()); i != e; ++i)
			delete *i;
	}

	// implements IUndoableChange.appendChange
	inline bool CompoundChange::appendChange(AtomicChange& postChange, const Document& document) {
		if(changes_.empty() || !changes_.back()->appendChange(postChange, document))
			changes_.push_back(&postChange);
		return true;
	}

	// implements IUndoableChange.perform
	void CompoundChange::perform(Document& document, Result& result) {
		assert(!changes_.empty());
		result.reset();
		Result delta;
		vector<AtomicChange*>::iterator i(changes_.end()), e(changes_.begin());
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


// Document.UndoManager ///////////////////////////////////////////////////////////////////////////

// manages undo/redo of the document.
class Document::UndoManager {
	ASCENSION_NONCOPYABLE_TAG(UndoManager);
public:
	// constructors
	explicit UndoManager(Document& document) /*throw()*/;
	virtual ~UndoManager() /*throw()*/ {clear();}
	// attributes
	size_t numberOfRedoableChanges() const /*throw()*/ {return redoableChanges_.size() + ((pendingAtomicChange_.get() != 0) ? 1 : 0);}
	size_t numberOfUndoableChanges() const /*throw()*/ {return undoableChanges_.size() + ((pendingAtomicChange_.get() != 0) ? 1 : 0);}
	bool isStackingCompoundOperation() const /*throw()*/ {return compoundChangeDepth_ > 0;}
	// rollbacks
	void redo(UndoableChange::Result& result);
	void undo(UndoableChange::Result& result);
	// recordings
	void addUndoableChange(AtomicChange& c);
	void beginCompoundChange() /*throw()*/ {++compoundChangeDepth_;}
	void clear() /*throw()*/;
	void endCompoundChange() /*throw()*/;
	void insertBoundary() /*throw()*/;
private:
	void commitPendingChange(bool beginCompound);
	Document& document_;
	stack<UndoableChange*> undoableChanges_, redoableChanges_;
	auto_ptr<AtomicChange> pendingAtomicChange_;
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
void Document::UndoManager::addUndoableChange(AtomicChange& c) {
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
void Document::UndoManager::redo(UndoableChange::Result& result) {
	commitPendingChange(false);
	if(redoableChanges_.empty()) {
		result.reset();
		return;
	}
	UndoableChange* c = redoableChanges_.top();
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
void Document::UndoManager::undo(UndoableChange::Result& result) {
	commitPendingChange(false);
	if(undoableChanges_.empty()) {
		result.reset();
		return;
	}
	UndoableChange* c = undoableChanges_.top();
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


// Document ///////////////////////////////////////////////////////////////////////////////////////

/// Constructor.
Document::Document() : session_(0), partitioner_(0),
		contentTypeInformationProvider_(new DefaultContentTypeInformationProvider),
		readOnly_(false), length_(0), revisionNumber_(0), lastUnmodifiedRevisionNumber_(0),
		onceUndoBufferCleared_(false), recordingChanges_(true), changing_(false), rollbacking_(false), /*locker_(0),*/ accessibleArea_(0) {
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
 * @throw ReadOnlyDocumentException The document is read only
 * @see #endCompoundChange, #isCompoundChanging
 */
void Document::beginCompoundChange() {
	if(isReadOnly())
		throw ReadOnlyDocumentException();
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
 * @throw IllegalStateException There is no compound change in this document
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
 * @throw ReadOnlyDocumentException The document is read only
 * @throw IllegalStateException The method was called in @c{IDocumentListener}s' notification
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
 * @param record If set to @c true, this method enables the recording and subsequent changes can be
 * undone. If set to @c false, discards the undo/redo information and disables the recording
 * @see #isRecordingChanges, #undo, #redo
 */
void Document::recordChanges(bool record) /*throw()*/ {
	if(!(recordingChanges_ = record))
		clearUndoBuffer();
}

namespace {
	class FirstChangeHolder {
	public:
		FirstChangeHolder(const Document& document, DocumentInput* input,
			void(DocumentInput::*post)(const Document&)) : document_(document), input_(input), post_(post) {assert(post_ != 0);}
		~FirstChangeHolder() {if(input_ != 0) (input_->*post_)(document_);}
	private:
		const Document& document_;
		DocumentInput* input_;
		void(DocumentInput::*post_)(const Document&);
	};
}

#define ASCENSION_PREPARE_FIRST_CHANGE(skip)									\
	const bool firstChange = !(skip) && !isModified() && (input_.get() != 0);	\
	if(firstChange) {															\
		if(!input_->isChangeable(*this))										\
			throw DocumentInput::ChangeRejectedException();						\
	}																			\
	FirstChangeHolder fch(*this, firstChange ? input_.get() : 0, &DocumentInput::postFirstDocumentChange)

/**
 * Performs the redo. Does nothing if the target region is inaccessible.
 * @param n The repeat count
 * @return @c false if the redo was not completely performed
 * @throw ReadOnlyDocumentException The document is read only
 * @throw IDocumentInput#ChangeRejectedException The change was rejected
 * @std#invalid_argument @a n &gt; #numberOfRedoableChanges()
 * @see #undo
 */
bool Document::redo(size_t n /* = 1 */) {
	if(n == 0)
		return true;
	else if(isReadOnly())
		throw ReadOnlyDocumentException();
	else if(n > numberOfRedoableChanges())
		throw invalid_argument("n");
	ASCENSION_PREPARE_FIRST_CHANGE(false);

	const bool modified = isModified();
	UndoableChange::Result result;
	result.completed = true;
	rollbackListeners_.notify<const Document&>(&DocumentRollbackListener::documentUndoSequenceStarted, *this);

	for(; n > 0 && result.completed; --n) {
		beginCompoundChange();
		rollbacking_ = true;
		undoManager_->redo(result);	// this shouldn't throw
		rollbacking_ = false;
		endCompoundChange();
	}
	assert(n == 0 || !result.completed);

	rollbackListeners_.notify<const Document&, const Position&>(
		&DocumentRollbackListener::documentUndoSequenceStopped, *this, result.endOfChange);
	if(isModified() != modified)
		stateListeners_.notify<const Document&>(&DocumentStateListener::documentModificationSignChanged, *this);
	return result.completed;
}

#if 0
/**
 * Removes the compound change listener.
 * @param listener The listener to be removed
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
 * @param region The region to erase. If this is empty, no text is erased
 * @param text The start of the text. If this is @c null, no text is inserted
 * @param[out] eos The position of the end of the inserted text. Can be @c null if not needed
 * @throw ReadOnlyDocumentException The document is read only
 * @throw DocumentAccessViolationException @a region intersects the inaccesible region
 * @throw NullPointerException Either @c text.end() returned @c null but @c text.beginning()
 *                             returned not @c null
 * @throw std#invalid_argument Either @a first is greater than @a last
 * @throw IllegalStateException The method was called in @c{IDocumentListener}s' notification
 * @throw IDocumentInput#ChangeRejectedException The input of the document rejected this change
 * @throw std#bad_alloc The internal memory allocation failed
 */
void Document::replace(const Region& region, const StringPiece& text, Position* eos /* = 0 */) {
	if(text.beginning() != 0 && text.end() == 0)
		throw NullPointerException("text.end()");
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
	else if(region.isEmpty() && (text.beginning() == 0 || text.isEmpty()))
		return;	// nothing to do
	ASCENSION_PREPARE_FIRST_CHANGE(rollbacking_);

	// preprocess. these can't throw
	detail::ValueSaver<bool> writeLock(changing_);
	changing_ = true;
	fireDocumentAboutToBeChanged();

	// change the content
	const Position& beginning = region.beginning();
	const Position& end = region.end();
	const Char* nextNewline = (text.beginning() != 0 && !text.isEmpty()) ?
		find_first_of(text.beginning(), text.end(), NEWLINE_CHARACTERS, ASCENSION_ENDOF(NEWLINE_CHARACTERS)) : 0;
	basic_stringbuf<Char> erasedString;
	length_t erasedStringLength = 0, insertedStringLength = 0;
	Position endOfInsertedString;
	try {
		// simple cases: both erased region and inserted string are single line
		if(beginning.line == end.line && (text.beginning() == 0 || text.isEmpty())) {	// erase in single line
			Line& line = *lines_[beginning.line];
			erasedString.sputn(line.text().data() + beginning.column, static_cast<streamsize>(end.column - beginning.column));
			line.text_.erase(beginning.column, end.column - beginning.column);
			erasedStringLength += end.column - beginning.column;
			endOfInsertedString = beginning;
		} else if(region.isEmpty() && nextNewline == text.end()) {	// insert single line
			lines_[beginning.line]->text_.insert(beginning.column, text.beginning(), text.length());
			insertedStringLength += text.length();
			endOfInsertedString.line = beginning.line;
			endOfInsertedString.column = beginning.column + text.length();
		} else if(beginning.line == end.line && nextNewline == text.end()) {	// replace in single line
			Line& line = *lines_[beginning.line];
			erasedString.sputn(line.text().data() + beginning.column, static_cast<streamsize>(end.column - beginning.column));
			line.text_.replace(beginning.column, end.column - beginning.column, text.beginning(), text.length());
			erasedStringLength += end.column - beginning.column;
			insertedStringLength += text.length();
			endOfInsertedString.line = beginning.line;
			endOfInsertedString.column = beginning.column + text.length();
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
							erasedString.sputn(newlineString(line.newline()),
								static_cast<streamsize>(newlineStringLength(line.newline())));
					}
//					erasedStringLength += e - p.column;
					if(last)
						break;
				}
			}
			// 2. allocate strings (lines except first) to insert newly. only when inserted string was multiline
			vector<Line*> allocatedLines;
			const Char* const firstNewline = nextNewline;
			if(text.beginning() != 0 && nextNewline != text.end()) {
				try {
					const Char* p = nextNewline + newlineStringLength(eatNewline(nextNewline, text.end()));
					while(true) {
						nextNewline = find_first_of(p, text.end(), NEWLINE_CHARACTERS, ASCENSION_ENDOF(NEWLINE_CHARACTERS));
						auto_ptr<Line> temp(new Line(revisionNumber_ + 1, String(p, nextNewline), eatNewline(nextNewline, text.end())));
						allocatedLines.push_back(temp.get());
						temp.release();
						insertedStringLength += allocatedLines.back()->text().length();
						if(nextNewline == text.end())
							break;
						p = nextNewline + newlineStringLength(allocatedLines.back()->newline());
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
				const length_t insertedLength = firstNewline - text.beginning();
				try {
					if(!allocatedLines.empty())
						firstLine.text_.replace(beginning.column, erasedLength, text.beginning(), insertedLength);
					else {
						// join the first line, inserted string and the last line
						String temp(text.beginning(), insertedLength);
						const Line& lastLine = *lines_[end.line];
						temp.append(lastLine.text(), end.column, lastLine.text().length() - end.column);
						firstLine.text_.replace(beginning.column, erasedLength, temp);
						endOfInsertedString.column += insertedLength;
					}
				} catch(...) {
					for(size_t i = end.line + 1, c = i + allocatedLines.size(); i < c; ++i)
						delete lines_[i];
					lines_.erase(end.line + 1, allocatedLines.size());
					throw;
				}
				firstLine.newline_ = (firstNewline != 0) ?
					eatNewline(firstNewline, text.end()) : lines_[end.line]->newline();
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
		else if(text.beginning() == 0 || text.isEmpty())
			undoManager_->addUndoableChange(*(new InsertionChange(beginning, erasedString.str())));
		else
			undoManager_->addUndoableChange(*(new ReplacementChange(Region(beginning, endOfInsertedString), erasedString.str())));
	}
	const bool modified = isModified();
	++revisionNumber_;
	length_ += insertedStringLength;
	length_ -= erasedStringLength;

	const DocumentChange change(region, Region(beginning, endOfInsertedString));
	fireDocumentChanged(change);
	if(!rollbacking_ && !modified)
		stateListeners_.notify<const Document&>(&DocumentStateListener::documentModificationSignChanged, *this);

	if(eos != 0)
		*eos = endOfInsertedString;
}

/**
 * @see fileio#insertFileContents
 */
void Document::replace(const Region& region, basic_istream<Char>& in, Position* endOfInsertedString /* = 0 */) {
	// TODO: this implementation is provisional and not exception-safe.
	Position e;
	Char buffer[0x8000];
	for(Region r(region); in; r.first = r.second = e) {
		in.read(buffer, ASCENSION_COUNTOF(buffer));
		if(in.gcount() == 0)
			break;
		replace(r, StringPiece(buffer, buffer + in.gcount()), &e);
	}
	if(endOfInsertedString != 0)
		*endOfInsertedString = e;
}

/**
 * Performs the undo. Does nothing if the target region is inaccessible.
 * @param n The repeat count
 * @return @c false if the undo was not completely performed
 * @throw ReadOnlyDocumentException The document is read only
 * @throw IDocumentInput#ChangeRejectedException The change was rejected
 * @std#invalid_argument @a n &gt; #numberOfUndoableChanges()
 * @see #redo
 */
bool Document::undo(size_t n /* = 1 */) {
	if(n == 0)
		return true;
	else if(isReadOnly())
		throw ReadOnlyDocumentException();
	else if(n > numberOfUndoableChanges())
		throw invalid_argument("n");
	ASCENSION_PREPARE_FIRST_CHANGE(false);

	const bool modified = isModified();
	const size_t oldRevisionNumber = revisionNumber_;
	UndoableChange::Result result;
	result.completed = true;
	rollbackListeners_.notify<const Document&>(&DocumentRollbackListener::documentUndoSequenceStarted, *this);

	for(; n > 0 && result.completed; --n) {
		beginCompoundChange();
		rollbacking_ = true;
		undoManager_->undo(result);	// this shouldn't throw
		rollbacking_ = false;
		endCompoundChange();
		revisionNumber_ = oldRevisionNumber - result.numberOfRevisions;
	}
	assert(n == 0 || !result.completed);

	rollbackListeners_.notify<const Document&, const Position&>(
		&DocumentRollbackListener::documentUndoSequenceStopped, *this, result.endOfChange);
	if(isModified() != modified)
		stateListeners_.notify<const Document&>(&DocumentStateListener::documentModificationSignChanged, *this);
	return result.completed;
}