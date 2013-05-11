/**
 * @file undo.cpp
 * @author exeal
 * @date 2009 separated from document.cpp
 * @date 2010-2013
 */

#include <ascension/kernel/document.hpp>
#include <ascension/kernel/point.hpp>
#include <ascension/corelib/utility.hpp>	// detail.ValueSaver
#include <boost/foreach.hpp>
#include <stack>
#include <vector>
using namespace ascension;
using namespace ascension::kernel;
using namespace ascension::text;
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
			void reset() BOOST_NOEXCEPT {
				completed = false;
				numberOfRevisions = 0;
//				endOfChange = Position();
			}
		};
	public:
		// destructor
		virtual ~UndoableChange() BOOST_NOEXCEPT {}
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
		virtual ~AtomicChange() BOOST_NOEXCEPT {}
		virtual const TypeTag& type() const BOOST_NOEXCEPT = 0;
	};

	// an atomic insertion change
	class InsertionChange : public AtomicChange, public FastArenaObject<InsertionChange> {
	public:
		InsertionChange(const Position& position, const String& text) : position_(position), text_(text) {}
		bool appendChange(AtomicChange&, const Document&) BOOST_NOEXCEPT {return false;}
		bool canPerform(const Document& document) const BOOST_NOEXCEPT {return !document.isNarrowed() || document.region().includes(position_);}
		void perform(Document& document, Result& result);
	private:
		const TypeTag& type() const BOOST_NOEXCEPT {return type_;}
	private:
		const Position position_;
		const String text_;
		static const TypeTag type_;
	};

	// an atomic deletion change
	class DeletionChange : public AtomicChange, public FastArenaObject<DeletionChange> {
	public:
		explicit DeletionChange(const Region& region) BOOST_NOEXCEPT : region_(region), revisions_(1) {}
		bool appendChange(AtomicChange& postChange, const Document&) BOOST_NOEXCEPT;
		bool canPerform(const Document& document) const BOOST_NOEXCEPT {return !document.isNarrowed() || document.region().encompasses(region_);}
		void perform(Document& document, Result& result);
	private:
		const TypeTag& type() const BOOST_NOEXCEPT {return type_;}
	private:
		Region region_;
		size_t revisions_;
		static const TypeTag type_;
	};

	// an atomic replacement change
	class ReplacementChange : public AtomicChange, public FastArenaObject<ReplacementChange> {
	public:
		explicit ReplacementChange(const Region& region, const String& text) : region_(region), text_(text) {}
		bool appendChange(AtomicChange&, const Document&) BOOST_NOEXCEPT {return false;}
		bool canPerform(const Document& document) const BOOST_NOEXCEPT {return !document.isNarrowed() || document.region().encompasses(region_);}
		void perform(Document& document, Result& result);
	private:
		const TypeTag& type() const BOOST_NOEXCEPT {return type_;}
	private:
		const Region region_;
		const String text_;
		static const TypeTag type_;
	};

	// a compound change
	class CompoundChange : public UndoableChange {
	public:
		~CompoundChange() BOOST_NOEXCEPT;
		bool appendChange(AtomicChange& postChange, const Document& document);
		bool canPerform(const Document& document) const BOOST_NOEXCEPT {return !changes_.empty() && changes_.back()->canPerform(document);}
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
		if(bottom.offsetInLine == 0 || bottom != static_cast<DeletionChange&>(postChange).region_.beginning())
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

	CompoundChange::~CompoundChange() BOOST_NOEXCEPT {
		BOOST_FOREACH(AtomicChange* change, changes_)
			delete change;
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
		vector<AtomicChange*>::iterator i(end(changes_)), e(begin(changes_));
		for(--i; ; --i) {
			(*i)->perform(document, delta);
			result.numberOfRevisions += delta.numberOfRevisions;
			if(!delta.completed) {
				if(i != --end(changes_))
					// partially completed
					changes_.erase(++i, end(changes_));
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
	explicit UndoManager(Document& document) BOOST_NOEXCEPT;
	virtual ~UndoManager() BOOST_NOEXCEPT {clear();}
	// attributes
	size_t numberOfRedoableChanges() const BOOST_NOEXCEPT {return redoableChanges_.size() + ((pendingAtomicChange_.get() != nullptr) ? 1 : 0);}
	size_t numberOfUndoableChanges() const BOOST_NOEXCEPT {return undoableChanges_.size() + ((pendingAtomicChange_.get() != nullptr) ? 1 : 0);}
	bool isStackingCompoundOperation() const BOOST_NOEXCEPT {return compoundChangeDepth_ > 0;}
	// rollbacks
	void redo(UndoableChange::Result& result);
	void undo(UndoableChange::Result& result);
	// recordings
	void addUndoableChange(AtomicChange& c);
	void beginCompoundChange() BOOST_NOEXCEPT {++compoundChangeDepth_;}
	void clear() BOOST_NOEXCEPT;
	void endCompoundChange() BOOST_NOEXCEPT;
	void insertBoundary() BOOST_NOEXCEPT;
private:
	void commitPendingChange(bool beginCompound);
	Document& document_;
	stack<unique_ptr<UndoableChange>> undoableChanges_, redoableChanges_;
	unique_ptr<AtomicChange> pendingAtomicChange_;
	size_t compoundChangeDepth_;
	bool rollbacking_;
	unique_ptr<CompoundChange> rollbackingChange_;
	CompoundChange* currentCompoundChange_;
};

// constructor takes the target document
Document::UndoManager::UndoManager(Document& document) BOOST_NOEXCEPT : document_(document),
		compoundChangeDepth_(0), rollbacking_(false), rollbackingChange_(nullptr), currentCompoundChange_(nullptr) {
}

// pushes the operation into the undo stack
void Document::UndoManager::addUndoableChange(AtomicChange& c) {
	if(!rollbacking_) {
		if(currentCompoundChange_ != nullptr)
			currentCompoundChange_->appendChange(c, document_);	// CompoundChange.appendChange always returns true
		else if(pendingAtomicChange_.get() != nullptr) {
			if(!pendingAtomicChange_->appendChange(c, document_)) {
				commitPendingChange(true);
				currentCompoundChange_->appendChange(c, document_);	// CompoundChange.appendChange always returns true
			}
		} else
			pendingAtomicChange_.reset(&c);

		// make the redo stack empty
		while(!redoableChanges_.empty())
			redoableChanges_.pop();
	} else {
		// delay pushing to the stack when rollbacking
		if(rollbackingChange_.get() == nullptr)
			rollbackingChange_.reset(new CompoundChange());
		rollbackingChange_->appendChange(c, document_);	// CompoundChange.appendChange always returns true
	}
}

// clears the stacks
inline void Document::UndoManager::clear() BOOST_NOEXCEPT {
	while(!undoableChanges_.empty())
		undoableChanges_.pop();
	while(!redoableChanges_.empty())
		redoableChanges_.pop();
	pendingAtomicChange_.reset();
	compoundChangeDepth_ = 0;
	currentCompoundChange_ = nullptr;
}

// commits the pending undoable change
inline void Document::UndoManager::commitPendingChange(bool beginCompound) {
	if(pendingAtomicChange_.get() != nullptr) {
		if(beginCompound) {
			unique_ptr<CompoundChange> newCompound(new CompoundChange());
			newCompound->appendChange(*pendingAtomicChange_.get(), document_);
			undoableChanges_.push(move(newCompound));
			pendingAtomicChange_.release();
			currentCompoundChange_ = static_cast<CompoundChange*>(undoableChanges_.top().get());	// safe down cast
		} else {
			if(currentCompoundChange_ == nullptr
					|| !currentCompoundChange_->appendChange(*pendingAtomicChange_.get(), document_)) {
				undoableChanges_.push(move(pendingAtomicChange_));
				currentCompoundChange_ = nullptr;
			}
			pendingAtomicChange_.reset();
		}
	}
}

// ends the compound change
inline void Document::UndoManager::endCompoundChange() BOOST_NOEXCEPT {
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
	rollbacking_ = true;
	redoableChanges_.top()->perform(document_, result);
	if(result.completed)
		redoableChanges_.pop();
	if(rollbackingChange_ != nullptr)
		undoableChanges_.push(move(rollbackingChange_));	// move the rollbcked change(s) into the undo stack
	rollbackingChange_.reset();
	currentCompoundChange_ = nullptr;
	rollbacking_ = false;
	compoundChangeDepth_ = 0;
}

// undoes one change
void Document::UndoManager::undo(UndoableChange::Result& result) {
	commitPendingChange(false);
	if(undoableChanges_.empty()) {
		result.reset();
		return;
	}
	rollbacking_ = true;
	undoableChanges_.top()->perform(document_, result);
	if(result.completed)
		undoableChanges_.pop();
	if(rollbackingChange_ != nullptr)
		redoableChanges_.push(move(rollbackingChange_));	// move the rollbacked change(s) into the redo stack
	rollbackingChange_.reset();
	currentCompoundChange_ = nullptr;
	rollbacking_ = false;
	compoundChangeDepth_ = 0;
}


// Document ///////////////////////////////////////////////////////////////////////////////////////

/// Constructor.
Document::Document() : session_(nullptr), partitioner_(),
		contentTypeInformationProvider_(new DefaultContentTypeInformationProvider),
		readOnly_(false), length_(0), revisionNumber_(0), lastUnmodifiedRevisionNumber_(0),
		onceUndoBufferCleared_(false), recordingChanges_(true), changing_(false), rollbacking_(false)/*, locker_(nullptr)*/ {
	bookmarker_.reset(new Bookmarker(*this));
	undoManager_.reset(new UndoManager(*this));
	resetContent();
}

/// Destructor.
Document::~Document() {
	BOOST_FOREACH(Point* p, points_)
		p->documentDisposed();
	accessibleRegion_.reset();
	bookmarker_.reset();	// Bookmarker.~Bookmarker() calls Document...
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
void Document::clearUndoBuffer() BOOST_NOEXCEPT {
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
bool Document::isCompoundChanging() const BOOST_NOEXCEPT {
	return undoManager_->isStackingCompoundOperation();
}

/// Returns the number of undoable changes.
size_t Document::numberOfUndoableChanges() const BOOST_NOEXCEPT {
	return undoManager_->numberOfUndoableChanges();
}

/// Returns the number of redoable changes.
size_t Document::numberOfRedoableChanges() const BOOST_NOEXCEPT {
	return undoManager_->numberOfRedoableChanges();
}

/**
 * Sets whether the document records or not the changes for undo/redo. Recording in a newly created
 * document is enabled to start with.
 * @param record If set to @c true, this method enables the recording and subsequent changes can be
 * undone. If set to @c false, discards the undo/redo information and disables the recording
 * @see #isRecordingChanges, #undo, #redo
 */
void Document::recordChanges(bool record) BOOST_NOEXCEPT {
	if(!(recordingChanges_ = record))
		clearUndoBuffer();
}

namespace {
	class FirstChangeHolder {
	public:
		FirstChangeHolder(const Document& document, weak_ptr<DocumentInput> input,
				void(DocumentInput::*post)(const Document&)) : document_(document), input_(input), post_(post) {
			assert(post_ != nullptr);
		}
		~FirstChangeHolder() {
			if(shared_ptr<DocumentInput> input = input_.lock())
				(input.get()->*post_)(document_);
		}
	private:
		const Document& document_;
		weak_ptr<DocumentInput> input_;
		void(DocumentInput::*post_)(const Document&);
	};
}

#define ASCENSION_PREPARE_FIRST_CHANGE(skip)								\
	const bool firstChange = !(skip) && !isModified() && !input_.expired();	\
	if(firstChange) {														\
		if(const shared_ptr<DocumentInput> p = input_.lock()) {				\
			if(!p->isChangeable(*this))										\
				throw DocumentInput::ChangeRejectedException();				\
		}																	\
	}																		\
	FirstChangeHolder fch(*this, firstChange ? input_ : weak_ptr<DocumentInput>(), &DocumentInput::postFirstDocumentChange)

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
		modificationSignChangedSignal_(*this);
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
 * @param text The text string for replacement. If this is @c null or empty, no text is inserted
 * @param[out] eos The position of the end of the inserted text. Can be @c null if not needed
 * @throw ReadOnlyDocumentException The document is read only
 * @throw DocumentAccessViolationException @a region intersects the inaccesible region
 * @throw IllegalStateException The method was called in @c{IDocumentListener}s' notification
 * @throw IDocumentInput#ChangeRejectedException The input of the document rejected this change
 * @throw std#bad_alloc The internal memory allocation failed
 */
void Document::replace(const Region& region, const StringPiece& text, Position* eos /* = nullptr */) {
	if(changing_)
		throw IllegalStateException("called in IDocumentListeners' notification.");
	else if(isReadOnly())
		throw ReadOnlyDocumentException();
	else if(region.end().line >= numberOfLines()
			|| region.first.offsetInLine > lineLength(region.first.line)
			|| region.second.offsetInLine > lineLength(region.second.line))
		throw BadRegionException(region);
	else if(isNarrowed() && !accessibleRegion().encompasses(region))
		throw DocumentAccessViolationException();
	else if(region.isEmpty() && (text.begin() == nullptr || text.empty()))
		return;	// nothing to do
	ASCENSION_PREPARE_FIRST_CHANGE(rollbacking_);

	// preprocess. these can't throw
	detail::ValueSaver<bool> writeLock(changing_);
	changing_ = true;
	fireDocumentAboutToBeChanged();

	// change the content
	const Position& beginning = region.beginning();
	const Position& end = region.end();
	const Char* nextNewline = (text.begin() != nullptr && !text.empty()) ?
		text.begin() + text.find_first_of(StringPiece(NEWLINE_CHARACTERS)) : nullptr;
	basic_stringbuf<Char> erasedString;
	Index erasedStringLength = 0, insertedStringLength = 0;
	Position endOfInsertedString;
	try {
		// simple cases: both erased region and inserted string are single line
		if(beginning.line == end.line && (text.begin() == nullptr || text.empty())) {	// erase in single line
			Line& line = *lines_[beginning.line];
			erasedString.sputn(line.text().data() + beginning.offsetInLine, static_cast<streamsize>(end.offsetInLine - beginning.offsetInLine));
			line.text_.erase(beginning.offsetInLine, end.offsetInLine - beginning.offsetInLine);
			erasedStringLength += end.offsetInLine - beginning.offsetInLine;
			endOfInsertedString = beginning;
		} else if(region.isEmpty() && nextNewline == text.end()) {	// insert single line
			lines_[beginning.line]->text_.insert(beginning.offsetInLine, text.begin(), text.length());
			insertedStringLength += text.length();
			endOfInsertedString.line = beginning.line;
			endOfInsertedString.offsetInLine = beginning.offsetInLine + text.length();
		} else if(beginning.line == end.line && nextNewline == text.end()) {	// replace in single line
			Line& line = *lines_[beginning.line];
			erasedString.sputn(line.text().data() + beginning.offsetInLine, static_cast<streamsize>(end.offsetInLine - beginning.offsetInLine));
			line.text_.replace(beginning.offsetInLine, end.offsetInLine - beginning.offsetInLine, text.begin(), text.length());
			erasedStringLength += end.offsetInLine - beginning.offsetInLine;
			insertedStringLength += text.length();
			endOfInsertedString.line = beginning.line;
			endOfInsertedString.offsetInLine = beginning.offsetInLine + text.length();
		}
		// complex case: erased region and/or inserted string are/is multi-line
		else {
			// 1. save undo information
			if(!region.isEmpty()) {
				for(Position p(beginning); ; ++p.line, p.offsetInLine = 0) {
					const Line& line = *lines_[p.line];
					const bool last = p.line == end.line;
					const Index e = !last ? line.text().length() : end.offsetInLine;
					if(recordingChanges_) {
						erasedString.sputn(line.text().data() + p.offsetInLine, static_cast<streamsize>(e - p.offsetInLine));
						if(!last) {
							const String eol(line.newline().asString());
							erasedString.sputn(eol.data(), static_cast<streamsize>(eol.length()));
						}
					}
//					erasedStringLength += e - p.offsetInLine;
					if(last)
						break;
				}
			}
			// 2. allocate strings (lines except first) to insert newly. only when inserted string was multiline
			vector<Line*> allocatedLines;
			const Char* const firstNewline = nextNewline;
			if(text.begin() != nullptr && nextNewline != text.end()) {
				try {
					const Char* p = nextNewline + eatNewline(nextNewline, text.end())->asString().length();
					while(true) {
						nextNewline = find_first_of(p, text.end(), NEWLINE_CHARACTERS, ASCENSION_ENDOF(NEWLINE_CHARACTERS));
						unique_ptr<Line> temp(new Line(revisionNumber_ + 1, String(p, nextNewline), *eatNewline(nextNewline, text.end())));
						allocatedLines.push_back(temp.get());
						temp.release();
						insertedStringLength += allocatedLines.back()->text().length();
						if(nextNewline == text.end())
							break;
						p = nextNewline + allocatedLines.back()->newline().asString().length();
					}
					// merge last line
					Line& lastAllocatedLine = *allocatedLines.back();
					endOfInsertedString.line = beginning.line + allocatedLines.size();
					endOfInsertedString.offsetInLine = lastAllocatedLine.text().length();
					const Line& lastLine = *lines_[end.line];
					lastAllocatedLine.text_.append(lastLine.text(), end.offsetInLine, lastLine.text().length() - end.offsetInLine);
					lastAllocatedLine.newline_ = lastLine.newline();
				} catch(...) {
					BOOST_FOREACH(Line* line, allocatedLines)
						delete line;
					throw;
				}
			} else
				endOfInsertedString = beginning;
			try {
				// 3. insert allocated strings
				if(!allocatedLines.empty())
					lines_.insert(begin(lines_) + end.line + 1, begin(allocatedLines), std::end(allocatedLines));
				// 4. replace first line
				Line& firstLine = *lines_[beginning.line];
				const Index erasedLength = firstLine.text().length() - beginning.offsetInLine;
				const Index insertedLength = firstNewline - text.begin();
				try {
					if(!allocatedLines.empty())
						firstLine.text_.replace(beginning.offsetInLine, erasedLength, text.begin(), insertedLength);
					else {
						// join the first line, inserted string and the last line
						String temp(text.begin(), insertedLength);
						const Line& lastLine = *lines_[end.line];
						temp.append(lastLine.text(), end.offsetInLine, lastLine.text().length() - end.offsetInLine);
						firstLine.text_.replace(beginning.offsetInLine, erasedLength, temp);
						endOfInsertedString.offsetInLine += insertedLength;
					}
				} catch(...) {
					const detail::GapVector<Line*>::const_iterator b(begin(lines_) + end.line + 1);
					const detail::GapVector<Line*>::const_iterator e(b + allocatedLines.size());
					for_each(b, e, default_delete<Line>());
					lines_.erase(b, e);
					throw;
				}
				firstLine.newline_ = (firstNewline != nullptr) ?
					*eatNewline(firstNewline, text.end()) : lines_[end.line]->newline();
				erasedStringLength += erasedLength;
				insertedStringLength += insertedLength;
			} catch(...) {
				BOOST_FOREACH(Line* line, allocatedLines)
					delete line;
				throw;
			}
			// 5. remove lines to erase
			if(!region.isEmpty()) {
				const detail::GapVector<Line*>::const_iterator b(begin(lines_) + beginning.line + 1);
				const detail::GapVector<Line*>::const_iterator e(begin(lines_) + end.line + 1);
				for_each(b, e, default_delete<Line>());
				lines_.erase(b, e);
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
		else if(text.begin() == nullptr || text.empty())
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
		modificationSignChangedSignal_(*this);

	if(eos != nullptr)
		*eos = endOfInsertedString;
}

/**
 * @see fileio#insertFileContents
 */
void Document::replace(const Region& region, basic_istream<Char>& in, Position* endOfInsertedString /* = nullptr */) {
	// TODO: this implementation is provisional and not exception-safe.
	Position e;
	Char buffer[0x8000];
	for(Region r(region); in; r.first = r.second = e) {
		in.read(buffer, ASCENSION_COUNTOF(buffer));
		if(in.gcount() == 0)
			break;
		replace(r, StringPiece(buffer, static_cast<StringPiece::size_type>(in.gcount())), &e);
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
		modificationSignChangedSignal_(*this);
	return result.completed;
}