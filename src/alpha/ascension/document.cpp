/**
 * @file document.cpp
 * @author exeal
 * @date 2003-2006 (was EditDoc.h)
 * @date 2006-2007
 */

#include "stdafx.h"
#include "document.hpp"
#include <shlwapi.h>	// PathXxxx
#include <shlobj.h>		// SHGetDesktopFolder, IShellFolder, ...
#include <MAPI.h>		// MAPISendMail
#include "../../manah/win32/ui/wait-cursor.hpp"
#include "../../manah/com/common.hpp"
#include <algorithm>
#include <limits>	// std.numeric_limits

using namespace ascension;
using namespace ascension::text;
using namespace ascension::encodings;
using namespace ascension::unicode;
using namespace manah::win32;
using namespace manah::win32::io;
using namespace manah::com;
using namespace std;


const Position Position::ZERO_POSITION(0, 0);
const Position Position::INVALID_POSITION(INVALID_INDEX, INVALID_INDEX);

namespace {
	class InsertOperation;
	class DeleteOperation;
}


/**
 * An abstract edit operation.
 * @see DeleteOperation, InsertOperation
 */
class text::internal::IOperation {
public:
	/// Destructor
	virtual ~IOperation() throw() {}
	/// Returns the operation is executable.
	virtual bool canExecute(Document& document) const = 0;
	/// Returns true if the operation can be appended to insertion @a postOperation.
	virtual bool isConcatenatable(InsertOperation& postOperation, const Document& document) const = 0;
	/// Returns true if the operation can be appended to deletion @a postOperation.
	virtual bool isConcatenatable(DeleteOperation& postOperation, const Document& document) const = 0;
	/// Executes the operation.
	virtual Position execute(Document& document) = 0;
};

namespace {
	/// An insertion operation.
	class InsertOperation : virtual public text::internal::IOperation, public manah::FastArenaObject<InsertOperation> {
	public:
		InsertOperation(const Position& pos, const String& text) : position_(pos), text_(text) {}
		bool canExecute(Document& document) const throw() {return !document.isNarrowed() || document.region().includes(position_);}
		bool isConcatenatable(InsertOperation&, const Document&) const throw() {return false;}
		bool isConcatenatable(DeleteOperation&, const Document&) const throw() {return false;}
		Position execute(Document& document) {return document.insert(position_, text_);}
	private:
		Position position_;
		String text_;
	};

	/// An deletion operation.
	class DeleteOperation : virtual public text::internal::IOperation, public manah::FastArenaObject<DeleteOperation> {
	public:
		DeleteOperation(const Region& region) throw() : region_(region) {}
		bool canExecute(Document& document) const throw() {return !document.isNarrowed() || document.region().encompasses(region_);}
		bool isConcatenatable(InsertOperation&, const Document&) const throw() {return false;}
		bool isConcatenatable(DeleteOperation& postOperation, const Document& document) const throw() {
			const Position& bottom = region_.end();
			if(bottom.column == 0 || bottom != postOperation.region_.beginning()) return false;
			else {const_cast<DeleteOperation*>(this)->region_.end() = postOperation.region_.end(); return true;}
		}
		Position execute(Document& document) {return document.erase(region_);}
	private:
		Region region_;
	};

	///
	class UnconvertableCharCallback : virtual public IUnconvertableCharCallback {
	public:
		UnconvertableCharCallback(IUnconvertableCharCallback* impl) : callback_(impl), calledOnce_(false) {
			assert(impl != 0);
		}
		bool unconvertableCharacterFound() {
			calledOnce_ = true;
			return callback_->unconvertableCharacterFound();
		}
		bool isCalledOnce() const throw() {
			return calledOnce_;
		}
	private:
		IUnconvertableCharCallback*	callback_;
		bool						calledOnce_;
	};

	/// Emulation of Win32 @c GetLongPathNameW (duplicated from NewApis.h)
	size_t getLongPathName(const WCHAR* shortName, WCHAR* longName, size_t bufferLength) {
		if(::GetFileAttributesW(shortName) == INVALID_FILE_ATTRIBUTES)
			return 0;
		WCHAR buffer[MAX_PATH];
		size_t c = ::GetFullPathNameW(shortName, MAX_PATH, buffer, 0);
		if(c == 0)
			return 0;
		else if(c >= MAX_PATH) {
			::SetLastError(ERROR_BUFFER_OVERFLOW);
			return 0;
		}
		ComPtr<IShellFolder> desktop;
		HRESULT hr = ::SHGetDesktopFolder(&desktop);
		if(FAILED(hr))
			return 0;
		::ITEMIDLIST* idList;
		ULONG eaten;
		if(FAILED(hr = desktop->ParseDisplayName(0, 0, buffer, &eaten, &idList, 0))) {
			::SetLastError((HRESULT_FACILITY(hr) == FACILITY_WIN32) ? HRESULT_CODE(hr) : ERROR_INVALID_DATA);
			return 0;
		}
		if((c = ::SHGetPathFromIDList(idList, buffer)) == 0 && buffer[0] != 0) {
			::SetLastError(ERROR_INVALID_DATA);
			return 0;
		}
		c = wcslen(buffer);
		if(c + 1 > bufferLength) {
			::SetLastError(ERROR_INSUFFICIENT_BUFFER);
			return ++c;
		}
		wcsncpy(longName, buffer, bufferLength);
		longName[bufferLength] = 0;
		ComPtr<IMalloc> m;
		if(SUCCEEDED(::SHGetMalloc(&m)))
			m->Free(idList);
		return c;
	}

	/**
	 * Makes the file name absolute if the file name is relative.
	 * @param fileName the file name
	 * @param[out] dest the destination
	 */
	void resolveRelativePathName(const WCHAR* fileName, WCHAR* dest) throw() {
		if(toBoolean(::PathIsRelativeW(fileName))) {
			WCHAR currDir[MAX_PATH];
			::GetCurrentDirectoryW(MAX_PATH, currDir);
			::PathCombineW(dest, currDir, fileName);
		} else
			wcscpy(dest, fileName);
	}

	// Document::length_ メンバ診断用
#ifdef _DEBUG
	length_t calculateDocumentLength(const Document& document) {
		length_t c = 0;
		const length_t lines = document.getNumberOfLines();
		for(length_t i = 0; i < lines; ++i)
			c += document.getLineLength(i);
		return c;
	}
#endif /* _DEBUG */
} // namespace @0


/// まとめてアンドゥ/リドゥ可能な一連の操作
class text::internal::OperationUnit : public manah::FastArenaObject<OperationUnit> {
public:
	virtual ~OperationUnit() throw();
	bool		execute(Document& document, Position& resultPosition);
	void		pop();
	void		push(InsertOperation& operation, const Document& document);
	void		push(DeleteOperation& operation, const Document& document);
	IOperation&	top() const;
private:
	stack<IOperation*> operations_;	// この単位に含まれる一連の操作
};


// OperationUnit ////////////////////////////////////////////////////////////

/// Destructor.
inline text::internal::OperationUnit::~OperationUnit() throw() {
	while(!operations_.empty()) {
		delete operations_.top();
		operations_.pop();
	}
}

/**
 * Executes the operation unit.
 *
 * 全操作を実行できたとき、このメソッドは true を返し、オブジェクトは無効になる。
 * ナローイングなどで実行できない操作があると、その操作までを実行し、
 * このオブジェクトは実行できなかった残りの操作を保持する。
 * 後でそれらの操作が実行可能になった後で再度メソッドを呼び出すと残りの操作が実行される
 * @param document 操作対象ドキュメント
 * @param[out] resultPosition 操作終了後にキャレットを置くべき位置
 * @return 全ての操作を実行できたとき true
 */
inline bool text::internal::OperationUnit::execute(Document& document, Position& resultPosition) {
	resultPosition = Position::INVALID_POSITION;
	while(!operations_.empty()) {	// 全ての操作を実行する
		if(!operations_.top()->canExecute(document))
			return false;
		resultPosition = operations_.top()->execute(document);
		delete operations_.top();
		operations_.pop();
	}
	return true;
}

/**
 * Pops the top operation.
 * @see #push, #top
 */
inline void text::internal::OperationUnit::pop() {
	operations_.pop();
}

/**
 * Pushes an operation.
 * @param operation the operation to be pushed
 * @param document the document
 */
inline void text::internal::OperationUnit::push(InsertOperation& operation, const Document& document) {
	operations_.push(&operation);
}

/// Pushes an operation (@a operation will be inavailble after this call).
inline void text::internal::OperationUnit::push(DeleteOperation& operation, const Document& document) {
	// if also the previous operation is deletion, extend the region to concatenate the operations
	if(!operations_.empty() && operations_.top()->isConcatenatable(operation, document)) {
		delete &operation;
		return;
	}
	operations_.push(&operation);
}

/**
 * Returns the top operation.
 * @see #pop, #push
 */
inline text::internal::IOperation& text::internal::OperationUnit::top() const {
	return *operations_.top();
}


// Document::UndoManager /////////////////////////////////////////////////////

/**
 * Constructor.
 * @param document the target document
 */
inline Document::UndoManager::UndoManager(Document& document) throw()
		: document_(document), compoundOperationStackingState_(NONE),
		virtualOperation_(false), virtualUnit_(0), lastUnit_(0), savedOperation_(0) {
}

/// Destructor.
inline Document::UndoManager::~UndoManager() throw() {
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
	savedOperation_ = 0;
	while(!undoStack_.empty()) {
		delete undoStack_.top();
		undoStack_.pop();
	}
	while(!redoStack_.empty()) {
		delete redoStack_.top();
		redoStack_.pop();
	}
}

/// The document was saved.
inline void Document::UndoManager::documentSaved() throw() {
	savedOperation_ = !undoStack_.empty() ? &undoStack_.top()->top() : 0;
}

/// Ends the compound operation.
inline void Document::UndoManager::endCompoundOperation() throw() {
	compoundOperationStackingState_ = NONE;
}

/// Returns the number of the redoable operations.
inline size_t Document::UndoManager::getRedoBufferLength() const throw() {
	return redoStack_.size();
}

/// Returns the number of the undoable operations.
inline size_t Document::UndoManager::getUndoBufferLength() const throw() {
	return undoStack_.size();
}

/// 前回保存時から操作が追加、削除されたかを返す
inline bool Document::UndoManager::isModifiedSinceLastSave() const throw() {
	if(undoStack_.empty())
		return savedOperation_ != 0;
	return savedOperation_ != &undoStack_.top()->top();
}

/// Returns true if the compound operation is running.
inline bool Document::UndoManager::isStackingCompoundOperation() const throw() {
	return compoundOperationStackingState_ != NONE;
}

/**
 * Pushes the operation into the undo stack.
 * @param operation the operation to be pushed
 */
template<class Operation> inline void Document::UndoManager::pushUndoBuffer(Operation& operation) {
	// make the redo stack empty
	if(!virtualOperation_) {
		while(!redoStack_.empty()) {
			delete redoStack_.top();
			redoStack_.pop();
		}
	}

	if(virtualOperation_) {	// 仮想操作時はスタックへの追加を遅延する
		if(virtualUnit_ == 0)	// 初回
			virtualUnit_ = new internal::OperationUnit();
		virtualUnit_->push(operation, document_);
	} else if(compoundOperationStackingState_ == WAIT_FOR_CONTINUATION && lastUnit_ != 0)	// 最後の操作単位に結合
		lastUnit_->push(operation, document_);
	else {
		internal::OperationUnit* newUnit = new internal::OperationUnit();
		newUnit->push(operation, document_);
		undoStack_.push(newUnit);
		lastUnit_ = newUnit;
		if(compoundOperationStackingState_ == WAIT_FOR_FIRST_PUSH)
			compoundOperationStackingState_ = WAIT_FOR_CONTINUATION;
	}
}

/// Redoes one operation.
inline bool Document::UndoManager::redo(Position& resultPosition) {
	if(redoStack_.empty())
		return false;

	internal::OperationUnit* unit = redoStack_.top();
	virtualOperation_ = true;			// 仮想操作開始
	const bool succeeded = unit->execute(document_, resultPosition);
	if(succeeded)
		redoStack_.pop();
	if(virtualUnit_ != 0)
		undoStack_.push(virtualUnit_);	// 仮想操作単位をアンドゥスタックへ移す
	virtualUnit_ = lastUnit_ = 0;
	virtualOperation_ = false;			// 仮想操作終了
	if(succeeded)
		delete unit;
	return succeeded;
}

/// Undoes one operation.
inline bool Document::UndoManager::undo(Position& resultPosition) {
	if(undoStack_.empty())
		return false;

	internal::OperationUnit* unit = undoStack_.top();
	virtualOperation_ = true;			// 仮想操作開始
	const bool succeeded = unit->execute(document_, resultPosition);
	if(succeeded)
		undoStack_.pop();
	if(virtualUnit_ != 0)
		redoStack_.push(virtualUnit_);	// 仮想操作単位をリドゥスタックへ移す
	virtualUnit_ = lastUnit_ = 0;
	virtualOperation_ = false;			// 仮想操作終了
	if(succeeded)
		delete unit;
	return succeeded;
}


// Point ////////////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param document the document to which the point attaches
 * @param position the initial position of the point
 * @throw BadPositionException @a position is outside of the document
 */
Point::Point(Document& document, const Position& position /* = Position() */) :
		document_(&document), position_(position), adapting_(true), excludedFromRestriction_(false), gravity_(FORWARD) {
	if(!document.region().includes(position))
		throw BadPositionException();
	static_cast<internal::IPointCollection<Point>&>(document).addNewPoint(*this);
}

/// Copy-constructor.
Point::Point(const Point& rhs) :
		document_(rhs.document_), position_(rhs.position_), adapting_(rhs.adapting_),
		excludedFromRestriction_(rhs.excludedFromRestriction_), gravity_(rhs.gravity_) {
	if(document_ == 0)
		throw DisposedDocumentException();
	static_cast<internal::IPointCollection<Point>*>(document_)->addNewPoint(*this);
}

/// Destructor.
Point::~Point() throw() {
	lifeCycleListeners_.notify(IPointLifeCycleListener::pointDestroyed);
	if(document_ != 0)
		static_cast<internal::IPointCollection<Point>*>(document_)->removePoint(*this);
}

/**
 * Registers the lifecycle listener.
 * @param listener the listener to be registered
 * @throw std#invalid_argument @a listener is already registered
 */
void Point::addLifeCycleListener(IPointLifeCycleListener& listener) {
	lifeCycleListeners_.add(listener);
}

/**
 * Moves to the specified position.
 * Derived classes can override this method to hook all movement of the point.
 * @param to the position
 */
void Point::doMoveTo(const Position& to) {
	verifyDocument();
	if(position_ != to) {
		position_ = to;
		normalize();
	}
}

/**
 * Moves to the specified position.
 * @param to the position
 */
void Point::moveTo(const Position& to) {
	verifyDocument();
	doMoveTo(to);
}

/**
 * Normalizes the position of the point.
 * This method does <strong>not</strong> inform to the listeners about any movement.
 */
void Point::normalize() const {
	verifyDocument();
	Position& position = const_cast<Point*>(this)->position_;
	position.line = min(position.line, document_->getNumberOfLines() - 1);
	position.column = min(position.column, document_->getLineLength(position.line));
	if(document_->isNarrowed() && excludedFromRestriction_) {
		const Region r(document_->accessibleRegion());
		position = max(position_, r.first);
		position = min(position_, r.second);
	}
}

/**
 * Removes the lifecycle listener
 * @param listener the listener to be removed
 * @throw std#invalid_argument @a listener is not registered
 */
void Point::removeLifeCycleListener(IPointLifeCycleListener& listener) {
	lifeCycleListeners_.remove(listener);
}

/**
 * Called when the document was changed.
 * @param change the content of the document change
 */
void Point::update(const DocumentChange& change) {
	if(document_ == 0 || !adapting_)
		return;

//	normalize();
	const Position newPosition = updatePosition(position_, change, gravity_);
	if(newPosition == position_)
		return;
	doMoveTo(newPosition);
}


// Bookmarker ///////////////////////////////////////////////////////////////

/**
 * Private constructor.
 * @param document the document
 */
Bookmarker::Bookmarker(Document& document) throw() : document_(document) {
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
	const length_t lines = document_.getNumberOfLines();
	bool clearedOnce = false;
	for(length_t i = 0; i < lines; ++i) {
		const Document::Line& line = document_.getLineInfo(i);
		if(line.bookmarked_) {
			line.bookmarked_ = false;
			clearedOnce = true;
		}
	}
	if(clearedOnce)
		listeners_.notify(IBookmarkListener::bookmarkCleared);
}

/**
 * Returns the line number of the next bookmarked line.
 * @param startLine the start line number to search. this line may be the result
 * @param direction direction to search
 * @return the next bookmarked line or @c INVALID_INDEX if not found
 * @throw BadPositionException @a line is outside of the document
 */
length_t Bookmarker::getNext(length_t startLine, Direction direction) const {
	const length_t lines = document_.getNumberOfLines();
	if(startLine >= lines)
		throw BadPositionException();
	else if(direction == FORWARD) {
		for(length_t line = startLine; line < lines; ++line) {
			if(document_.getLineInfo(line).bookmarked_)
				return line;
		}
	} else {
		for(length_t line = startLine + 1; line >= 0; --line) {
			if(document_.getLineInfo(line - 1).bookmarked_)
				return line - 1;
		}
	}
	return INVALID_INDEX;
}

/**
 * Returns true if the specified line is bookmarked.
 * @param line the line
 * @throw BadPositionException @a line is outside of the document
 */
bool Bookmarker::isMarked(length_t line) const {
	return document_.getLineInfo(line).bookmarked_;
}

/**
 * Sets or clears the bookmark of the specified line.
 * @param line the line
 * @param set true to set bookmark, false to clear
 * @throw BadPositionException @a line is outside of the document
 */
void Bookmarker::mark(length_t line, bool set) {
	const Document::Line& l = document_.getLineInfo(line);
	if(l.bookmarked_ != set) {
		l.bookmarked_ = set;
		listeners_.notify<length_t>(IBookmarkListener::bookmarkChanged, line);
	}
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
	const Document::Line& l = document_.getLineInfo(line);
	l.bookmarked_ = !l.bookmarked_;
	listeners_.notify<length_t>(IBookmarkListener::bookmarkChanged, line);
}


// DocumentPartitioner //////////////////////////////////////////////////////

/// Constructor.
DocumentPartitioner::DocumentPartitioner() throw() : document_(0) {
}

/// Destructor.
DocumentPartitioner::~DocumentPartitioner() throw() {
}


// Document //////////////////////////////////////////////////////////////////

/**
 * @class ascension::text::Document
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
 * value is initially zero and incremented by @c #insert, @c #insertFromStream, @c #erase, and
 * @c #resetContent methods. A revision number is never decremented or reset to zero. The current
 * revision number can be obtained by @c #getRevisionNumber.
 *
 * A document can be devides into a sequence of semantic segments called partition.
 * Document partitioners expressed by @c DocumentPartitioner class define these
 * partitioning. Each partitions have its content type and region (see @c DocumentPartition).
 * To set the new partitioner, use @c #setPartitioner method. The partitioner's ownership
 * will be transferred to the document.
 *
 * @see Viewer, IDocumentPartitioner, Point, EditPoint
 */

#define CHECK_FIRST_MODIFICATION()																\
	if(!isModified() && timeStampDirector_ != 0) {												\
		::FILETIME realTimeStamp;																\
		if(!verifyTimeStamp(true, realTimeStamp)) {												\
			if(!timeStampDirector_->queryAboutUnexpectedDocumentFileTimeStamp(					\
					*this, IUnexpectedFileTimeStampDirector::FIRST_MODIFICATION))				\
				return position;																\
			diskFile_.lastWriteTimes.internal = diskFile_.lastWriteTimes.user = realTimeStamp;	\
		}																						\
	}

CodePage Document::defaultCodePage_ = ::GetACP();
Newline Document::defaultNewline_ = ASCENSION_DEFAULT_NEWLINE;

/// Constructor.
Document::Document() : session_(0), partitioner_(0),
		contentTypeInformationProvider_(new DefaultContentTypeInformationProvider),
		readOnly_(false), modified_(false), codePage_(defaultCodePage_), newline_(defaultNewline_),
		length_(0), revisionNumber_(0), onceUndoBufferCleared_(false), recordingOperations_(true), virtualOperating_(false),
		changing_(false), accessibleArea_(0), timeStampDirector_(0) {
	bookmarker_.reset(new Bookmarker(*this));
	undoManager_.reset(new UndoManager(*this));
	resetContent();
}

/// Destructor.
Document::~Document() {
	if(diskFile_.isLocked())
		diskFile_.unlock();
	delete[] diskFile_.pathName;
	for(set<Point*>::iterator it = points_.begin(); it != points_.end(); ++it)
		(*it)->documentDisposed();
	if(accessibleArea_ != 0) {
		delete accessibleArea_->second;
		delete accessibleArea_;
	}
}

/**
 * Starts the sequential edit. Restarts if the sequential edit is already running.
 * @see #endSequentialEdit, #isSequentialEditing
 */
void Document::beginSequentialEdit() throw() {
	if(isSequentialEditing())
		endSequentialEdit();
	undoManager_->beginCompoundOperation();
	sequentialEditListeners_.notify<Document&>(ISequentialEditListener::documentSequentialEditStarted, *this);
}

/**
 * Checks the last modified date/time of the bound file and verifies if the other modified the
 * file. If the file is modified, the listener's
 * @c IUnexpectedFileTimeStampDerector#queryAboutUnexpectedDocumentFileTimeStamp will be called.
 * @return the value which the listener returned or true if the listener is not set
 */
bool Document::checkTimeStamp() {
	::FILETIME newTimeStamp;
	if(!verifyTimeStamp(false, newTimeStamp)) {
		::FILETIME original = diskFile_.lastWriteTimes.user;
		memset(&diskFile_.lastWriteTimes.user, 0, sizeof(::FILETIME));
		if(timeStampDirector_ == 0
				|| timeStampDirector_->queryAboutUnexpectedDocumentFileTimeStamp(*this, IUnexpectedFileTimeStampDirector::CLIENT_INVOCATION)) {
			diskFile_.lastWriteTimes.user = newTimeStamp;
			return true;
		}
		diskFile_.lastWriteTimes.user = original;
		return false;
	}
	return true;
}

/**
 * Ends the active sequential edit.
 * @see #beginSequentialEdit, #isSequentialEditing
 */
void Document::endSequentialEdit() throw() {
	undoManager_->endCompoundOperation();
	sequentialEditListeners_.notify<Document&>(ISequentialEditListener::documentSequentialEditStopped, *this);
}

/**
 * Deletes the specified region of the document.
 *
 * This method sets the modification flag and calls the listeners'
 * @c IDocumentListener#documentAboutToBeChanged and @c IDocumentListener#documentChanged.
 *
 * If the specified region intersects the inaccessible region, the union is not deleted.
 * @param region the region to be deleted
 * @return the position where the point which deletes the text will move to
 * @throw ReadOnlyDocumentException the document is read only
 */
Position Document::erase(const Region& region) {
	if(changing_ || isReadOnly())
		throw ReadOnlyDocumentException();
	else if(region.isEmpty())	// 空 -> 無視
		return region.beginning();
	else if(isNarrowed()) {
		const Region r(accessibleRegion());
		if(region.end() <= r.first)
			return region.end();
		else if(region.beginning() >= r.second)
			return region.beginning();
	} else if(!isModified() && timeStampDirector_ != 0) {
		::FILETIME realTimeStamp;
		if(!verifyTimeStamp(true, realTimeStamp)) {	// 他で上書きされとる
			if(!timeStampDirector_->queryAboutUnexpectedDocumentFileTimeStamp(*this, IUnexpectedFileTimeStampDirector::FIRST_MODIFICATION))
				return region.beginning();
			diskFile_.lastWriteTimes.internal = diskFile_.lastWriteTimes.user = realTimeStamp;
		}
	}

	ModificationGuard guard(*this);
	fireDocumentAboutToBeChanged();

	const Position begin = isNarrowed() ? max(region.beginning(), accessibleRegion().first) : region.beginning();
	const Position end = isNarrowed() ? min(region.end(), accessibleRegion().second) : region.end();
	StringBuffer deletedString;	// 削除した文字列 (複数行になるときは現在の改行文字が挿入される)

	if(begin.line == end.line) {	// 対象が1行以内
		const Line&	lineInfo = getLineInfo(end.line);
		String&	line = const_cast<String&>(getLine(end.line));

		++const_cast<Line&>(lineInfo).operationHistory_;
		deletedString.sputn(line.data() + begin.column, static_cast<streamsize>(end.column - begin.column));
		line.erase(begin.column, end.column - begin.column);
		length_ -= end.column - begin.column;
	} else {							// 対象が複数行
		Line* line = lines_[begin.line];
		auto_ptr<String> tail;
		deletedString.sputn(line->text_.data() + begin.column, static_cast<streamsize>(line->text_.length() - begin.column));
		length_ -= line->text_.length() - begin.column;
		line->text_.erase(begin.column);

		// 削除する部分を保存しつつ削除
		Line& firstLine = *lines_[begin.line];
		Newline lastNewline;
		deletedString.sputn(getNewlineString(lines_[begin.line]->newline_),
			static_cast<streamsize>(getNewlineStringLength(lines_[begin.line]->newline_)));
		for(length_t i = begin.line + 1; ; ++i) {
			line = lines_[i];
			deletedString.sputn(line->text_.data(), static_cast<streamsize>((i != end.line) ? line->text_.length() : end.column));
			length_ -= line->text_.length();
			if(i != end.line)
				deletedString.sputn(getNewlineString(line->newline_), static_cast<streamsize>(getNewlineStringLength(line->newline_)));
			else {	// 削除終了行
				tail.reset(new String(line->text_.substr(end.column)));
				lastNewline = line->newline_;
				break;
			}
		}
		lines_.erase(begin.line + 1, end.line - begin.line);

		// 削除した部分の前後を繋ぐ
		firstLine.newline_ = lastNewline;
		++firstLine.operationHistory_;
		if(!tail->empty()) {
			firstLine.text_ += *tail;
			length_ += tail->length();
		}
	}

	if(recordingOperations_)
		undoManager_->pushUndoBuffer(*(new InsertOperation(begin, deletedString.str())));

	// notify the change
	++revisionNumber_;
	fireDocumentChanged(DocumentChange(true, Region(begin, end)));
	setModified();

	return begin;
}

/// Invokes the listeners' @c IDocumentListener#documentAboutToBeChanged.
inline void Document::fireDocumentAboutToBeChanged() throw() {
	if(partitioner_.get() != 0)
		partitioner_->documentAboutToBeChanged();
	prenotifiedListeners_.notify<const Document&>(IDocumentListener::documentAboutToBeChanged, *this);
	listeners_.notify<const Document&>(IDocumentListener::documentAboutToBeChanged, *this);
}

/// Invokes the listeners' @c IDocumentListener#documentChanged.
inline void Document::fireDocumentChanged(const DocumentChange& c, bool updateAllPoints /* = true */) throw() {
	if(partitioner_.get() != 0)
		partitioner_->documentChanged(c);
	if(updateAllPoints)
		updatePoints(c);
	prenotifiedListeners_.notify<const Document&, const DocumentChange&>(IDocumentListener::documentChanged, *this, c);
	listeners_.notify<const Document&, const DocumentChange&>(IDocumentListener::documentChanged, *this, c);
}

/**
 * Returns the offset of the line.
 * @param line the line
 * @param nlr the line representation policy for character counting
 * @throw BadPostionException @a line is outside of the document
 */
length_t Document::getLineOffset(length_t line, NewlineRepresentation nlr) const {
	if(line >= lines_.getSize())
		throw BadPositionException();

	length_t offset = 0;
	for(length_t i = 0; i < line; ++i) {
		const Line& ln = *lines_[i];
		offset += ln.text_.length();
		switch(nlr) {
		case NLR_LINE_FEED:
		case NLR_LINE_SEPARATOR:	offset += 1; break;
		case NLR_CRLF:				offset += 2; break;
		case NLR_PHYSICAL_DATA:		offset += getNewlineStringLength(ln.newline_); break;
		case NLR_DOCUMENT_DEFAULT:	offset += getNewlineStringLength(getNewline()); break;
		case NLR_SKIP:				break;
		}
	}
	return offset;
}

/**
 * Inserts the text into the specified position. For detail, see two-parameter version of this method.
 *
 * This method sets the modification flag and calls the listeners'
 * @c IDocumentListener#documentAboutToBeChanged and @c IDocumentListener#documentChanged.

 * @param position the position
 * @param first the start of the text
 * @param last the end of the text
 * @return the result position
 * @throw ReadOnlyDocumentException the document is read only
 * @throw NullPointerException either @a first or @a last is @c null
 * @throw std#invalid_argument either @a first is greater than @a last
 */
Position Document::insert(const Position& position, const Char* first, const Char* last) {
	if(changing_ || isReadOnly())
		throw ReadOnlyDocumentException();
	else if(first == 0 || last == 0)
		throw NullPointerException("first and/or last are null.");
	else if(first > last)
		throw invalid_argument("first > last");
	else if(isNarrowed() && !accessibleRegion().includes(position))	// ignore the insertion position is out of the accessible region
		return position;
	else if(first == last)	// ignore if the input is empty
		return position;
	else
		CHECK_FIRST_MODIFICATION()

	ModificationGuard guard(*this);
	fireDocumentAboutToBeChanged();

	Position resultPosition(position.line, 0);
	const Char* breakPoint = find_first_of(first, last, LINE_BREAK_CHARACTERS, endof(LINE_BREAK_CHARACTERS));

	if(breakPoint == last) {	// single-line
		Line& line = const_cast<Line&>(getLineInfo(position.line));
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
			if(binary_search(LINE_BREAK_CHARACTERS, endof(LINE_BREAK_CHARACTERS), *lastBreak))
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
		breakPoint += (firstLine.newline_ != NLF_CRLF) ? 1 : 2;
		++firstLine.operationHistory_;
		++line;
		++resultPosition.line;

		// 改行ごとに行に区切っていく
		while(true) {
			if(breakPoint <= lastBreak) {
				const Char* const nextBreak =
					find_first_of(breakPoint, last, LINE_BREAK_CHARACTERS, endof(LINE_BREAK_CHARACTERS));
				assert(nextBreak != last);
				const Newline newline = eatNewline(nextBreak, last);

				length_ += nextBreak - breakPoint;
				lines_.insert(line, new Line(String(breakPoint, nextBreak), newline, true));
				++line;
				++resultPosition.line;
				breakPoint = nextBreak + ((newline != NLF_CRLF) ? 1 : 2);
			} else {	// 最終行
				length_ += last - breakPoint + firstLineRest.length();
				lines_.insert(line, new Line(String(breakPoint, last) + firstLineRest, firstNewline, true));
				break;
			}
		}
	}

	if(recordingOperations_)
		undoManager_->pushUndoBuffer(*(new DeleteOperation(Region(position, resultPosition))));

	// notify the change
	++revisionNumber_;
	fireDocumentChanged(DocumentChange(false, Region(position, resultPosition)));
	setModified();
//	assert(length_ == calculateDocumentLength(*this));	// length_ メンバの診断

	return resultPosition;
}

/**
 * Inserts the text into the specified position from the input stream.
 * @param position the position
 * @param in the input stream
 * @return the result position
 * @throw ReadOnlyDocumentException the document is read only
 */
Position Document::insertFromStream(const Position& position, InputStream& in) {
	if(isReadOnly())
		throw ReadOnlyDocumentException();
	else if(isNarrowed() && !accessibleRegion().includes(position))	// アクセス可能範囲外 -> 無視
		return position;
	else if(in.eof())
		return position;
	else
		CHECK_FIRST_MODIFICATION()

	// TODO: not implemented.
	return position;
}

/**
 * Returns true if the document is sequential editing.
 * @see #beginSequentialEdit, #endSequentialEdit
 */
bool Document::isSequentialEditing() const throw() {
	return undoManager_->isStackingCompoundOperation();
}

/**
 * Reads the document from the specified file.
 * @param fileName the file name. this method doesn't resolves the short cut
 * @param lockMode the lock mode
 * @param codePage the code page
 * @param callback the callback. can be @c null
 * @return succeeded or not. see @c #FileIOResult
 * @throw std#invalid_argument the bit combination of @a fileOpenMode is invalid
 */
Document::FileIOResult Document::load(const basic_string<WCHAR>& fileName,
		const FileLockMode& lockMode, CodePage codePage, IFileIOListener* callback /* = 0 */) {
//	Timer tm(L"load");	// 2.86s / 1MB
	const DWORD attributes = ::GetFileAttributesW(fileName.c_str());
	if(attributes == INVALID_FILE_ATTRIBUTES)	// file not found
		return FIR_STANDARD_WIN32_ERROR;

	auto_ptr<File<true> > newFile(new File<true>);
	EncoderFactory& encoderFactory = EncoderFactory::getInstance();
	if(!encoderFactory.isValidCodePage(codePage))
		return FIR_INVALID_CODE_PAGE;
	ui::WaitCursor wc;

	// open the file
	DWORD shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
	if(lockMode.type != FileLockMode::LOCK_TYPE_NONE && !lockMode.onlyAsEditing)
		shareMode = (lockMode.type == FileLockMode::LOCK_TYPE_SHARED) ? FILE_SHARE_READ : 0;
	if(!newFile->open(fileName.c_str(), GENERIC_READ, shareMode, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN))
		return FIR_STANDARD_WIN32_ERROR;	// ERROR_SHARING_VIOLATION, ...

	// abort if the file is too large
	if(newFile->getLength() > numeric_limits<ulong>::max()) {
		newFile->close();
		return FIR_HUGE_FILE;
	}

	// bind to the new file
	resetContent();
	diskFile_.lockingFile.reset(newFile.release());
	diskFile_.lockMode = lockMode;

#define CLOSE_AND_ABORT(errorCode)	\
	fileView.reset(0);				\
	if(fileMapper.get() != 0)		\
		fileMapper->close();		\
	diskFile_.lockingFile->close();	\
	return errorCode

	const ulong fileSize = static_cast<ulong>(diskFile_.lockingFile->getLength() & 0xFFFFFFFFUL);
	const uchar* nativeBuffer;
	auto_ptr<MemoryMappedFile<const uchar, true> > fileMapper(
		(fileSize != 0) ? new MemoryMappedFile<const uchar, true>(*diskFile_.lockingFile, PAGE_READONLY) : 0);
	auto_ptr<MemoryMappedFile<const uchar, true>::View> fileView;
	if(fileSize != 0) {
		if(fileMapper->get() == 0) {
			CLOSE_AND_ABORT(FIR_STANDARD_WIN32_ERROR);
		}
		fileView = fileMapper->mapView(FILE_MAP_READ);
		if(fileView.get() == 0) {
			CLOSE_AND_ABORT(FIR_STANDARD_WIN32_ERROR);
		}
		nativeBuffer = fileView->getData();
	}

	// convert the whole buffer
	Position last(0, 0);	// 次に文字列を追加する位置
	if(fileSize != 0) {
		codePage = encoderFactory.detectCodePage(nativeBuffer, min(fileSize, 4UL * 1024), codePage);

		auto_ptr<Encoder> encoder = encoderFactory.createEncoder(codePage);
		assert(encoder.get() != 0);
		size_t destLength = encoder->getMaxUCSCharLength() * fileSize;
		const uchar* bom;
		size_t bomLength;

		switch(codePage) {
		case CP_UTF8:				bom = encodings::UTF8_BOM; bomLength = 3; break;
		case CPEX_UNICODE_UTF16LE:	bom = encodings::UTF16LE_BOM; bomLength = 2; break;
		case CPEX_UNICODE_UTF16BE:	bom = encodings::UTF16BE_BOM; bomLength = 2; break;
		case CPEX_UNICODE_UTF32LE:	bom = encodings::UTF32LE_BOM; bomLength = 4; break;
		case CPEX_UNICODE_UTF32BE:	bom = encodings::UTF32BE_BOM; bomLength = 4; break;
		default:					bom = 0; bomLength = 0; break;
		}

		Char* const ucsBuffer =
			static_cast<Char*>(::HeapAlloc(::GetProcessHeap(), HEAP_NO_SERIALIZE, (destLength + 1) * sizeof(Char)));
		if(ucsBuffer == 0) {
			CLOSE_AND_ABORT(FIR_OUT_OF_MEMORY);
		}

		if(bomLength != 0 && fileSize >= bomLength && memcmp(nativeBuffer, bom, bomLength) == 0)
			destLength = encoder->toUnicode(ucsBuffer, destLength, nativeBuffer + bomLength, fileSize - bomLength, callback);
		else
			destLength = encoder->toUnicode(ucsBuffer, destLength, nativeBuffer, fileSize, callback);

		if(destLength == 0) {	// encounted an unconvertable character and the client aborted
			::HeapFree(::GetProcessHeap(), HEAP_NO_SERIALIZE, ucsBuffer);
			CLOSE_AND_ABORT(FIR_CALLER_ABORTED);
		}

#undef CLOSE_AND_ABORT

		ucsBuffer[destLength] = 0;
		setFilePathName(canonicalizePathName(fileName.c_str()).c_str());
		fireDocumentAboutToBeChanged();

		// break the buffer into lines by newlines
		length_t nextBreak, lastBreak = 0;
		Newline newline;
		lines_.clear();
		newline_ = NLF_AUTO;
		while(true) {
			for(size_t i = lastBreak; ; ++i) {	// search the next new line
				if(i == destLength) {
					nextBreak = -1;
					break;
				} else if(binary_search(LINE_BREAK_CHARACTERS, endof(LINE_BREAK_CHARACTERS) - 1, ucsBuffer[i])) {
					nextBreak = i;
					break;
				}
			}
			if(nextBreak != -1) {
				// 改行文字の判定
				switch(ucsBuffer[nextBreak]) {
				case LINE_FEED:	newline = NLF_LF;	break;
				case CARRIAGE_RETURN:
					newline = (nextBreak + 1 < destLength && ucsBuffer[nextBreak + 1] == LINE_FEED) ? NLF_CRLF : NLF_CR;
					break;
				case NEXT_LINE:	newline = NLF_NEL;	break;
				case LINE_SEPARATOR:		newline = NLF_LS;	break;
				case PARAGRAPH_SEPARATOR:	newline = NLF_PS;	break;
				}
				lines_.insert(lines_.getSize(), new Line(String(ucsBuffer + lastBreak, nextBreak - lastBreak), newline));
				length_ += nextBreak - lastBreak;
				lastBreak = nextBreak + ((newline != NLF_CRLF) ? 1 : 2);
				if(newline_ == NLF_AUTO)	// 最初に現れた改行文字を既定の改行文字とする
					setNewline(newline);
			} else {	// the last line
				lines_.insert(lines_.getSize(), new Line(String(ucsBuffer + lastBreak, destLength - lastBreak)));
				length_ += destLength - lastBreak;
				break;
			}
		}
		::HeapFree(::GetProcessHeap(), HEAP_NO_SERIALIZE, ucsBuffer);
	} else {	// an empty file
		codePage = ::GetACP();
		setFilePathName(canonicalizePathName(fileName.c_str()).c_str());
	}

	if(newline_ == NLF_AUTO)
		setNewline(Document::defaultNewline_);
	diskFile_.lockingFile->getFileTime(0, 0, &diskFile_.lastWriteTimes.internal);
	diskFile_.lastWriteTimes.user = diskFile_.lastWriteTimes.internal;
	if(diskFile_.lockMode.type == FileLockMode::LOCK_TYPE_NONE)
		diskFile_.unlock();

	setCodePage(codePage);
	clearUndoBuffer();
	setModified(false);
	onceUndoBufferCleared_ = false;
	fireDocumentChanged(DocumentChange(false, region()), false);
	if(toBoolean(attributes & FILE_ATTRIBUTE_READONLY))
		setReadOnly();

	return FIR_OK;
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
	stateListeners_.notify<Document&>(IDocumentStateListener::documentAccessibleRegionChanged, *this);
}

/**
 * Sets whether the document records or not the operations for undo/redo.
 *
 * The default is true. If change the setting, recorded contents will be disposed.
 * @param record set true to record
 * @see #isRecordingOperation, #undo, #redo
 */
void Document::recordOperations(bool record) {
	if(!(recordingOperations_ = record))
		clearUndoBuffer();
}

/**
 * Performs the redo.
 * @return false if the redo was not completely performed
 * @throw ReadOnlyDocumentException the document is read only
 * @see #undo
 */
bool Document::redo() {
	if(isReadOnly())
		throw ReadOnlyDocumentException();
	else if(getUndoHistoryLength(true) == 0)
		return false;

	beginSequentialEdit();
	sequentialEditListeners_.notify<Document&>(ISequentialEditListener::documentUndoSequenceStarted, *this);
	Position resultPosition;
	const bool succeeded = undoManager_->redo(resultPosition);
	sequentialEditListeners_.notify<Document&, const Position&>(ISequentialEditListener::documentUndoSequenceStopped, *this, resultPosition);
	endSequentialEdit();

	if(!undoManager_->isModifiedSinceLastSave())
		setModified(false);
	return succeeded;
}

/**
 * Renames (moves) the bound file.
 * @param newName the new name
 * @return true if succeeded
 */
bool Document::renameFile(const WCHAR* newName) {
	// TODO: not implemented.
	return false;
}

/**
 * Resets and initializes the content of the document.
 * @note This method does not reset the revision number to zero.
 */
void Document::resetContent() {
	diskFile_.unlock();
	widen();
	for(set<Point*>::iterator it = points_.begin(); it != points_.end(); ++it)
		(*it)->moveTo(0, 0);

	if(length_ != 0) {
		const Region entire(region());
		assert(!lines_.isEmpty());
		fireDocumentAboutToBeChanged();
		lines_.clear();
		lines_.insert(lines_.begin(), new Line);
		length_ = 0;
		++revisionNumber_;
		fireDocumentChanged(DocumentChange(true, entire), false);
	} else if(lines_.isEmpty())
		lines_.insert(lines_.begin(), new Line);

	setReadOnly(false);
	setCodePage(Document::defaultCodePage_);
	setNewline(Document::defaultNewline_);
	setFilePathName(0);
	setModified(false);
	clearUndoBuffer();
	onceUndoBufferCleared_ = false;
	diskFile_.DiskFile::DiskFile();
}

/**
 * Writes the content of the document to the specified file.
 * @param fileName the file name
 * @param params the options. set @a newline member of this object to @c NLF_AUTO not to unify the line breaks.
 * set @a codePage member of this object to @c CPEX_AUTODETECT to use the current code page
 * @param callback the callback or @c null
 * @return the result. @c FIR_OK if succeeded
 */
Document::FileIOResult Document::save(const basic_string<WCHAR>& fileName, const SaveParameters& params, IFileIOListener* callback /* = 0 */) {
	// 保存先のファイルが読み取り専用か調べておく
	const DWORD originalAttrs = ::GetFileAttributesW(fileName.c_str());	// この値は後でもう 1 回使う
	if(originalAttrs != INVALID_FILE_ATTRIBUTES && toBoolean(originalAttrs & FILE_ATTRIBUTE_READONLY))
		return FIR_READ_ONLY_MODE;

	// 現在開いているファイルに上書きする場合は、外部で変更されていないか調べる
	if(timeStampDirector_ != 0) {
		::FILETIME realTimeStamp;
		if(!verifyTimeStamp(true, realTimeStamp)) {	// 他で上書きされとる
			if(!timeStampDirector_->queryAboutUnexpectedDocumentFileTimeStamp(*this, IUnexpectedFileTimeStampDirector::OVERWRITE_FILE))
				return FIR_OK;
		}
	}

	const wstring realName = canonicalizePathName(fileName.c_str());
	EncoderFactory& encoderFactory = EncoderFactory::getInstance();
	if(!encoderFactory.isValidCodePage(params.codePage)					// コードページがインストールされているか
			|| encoderFactory.isCodePageForReadOnly(params.codePage))	// 読み取り専用のコードページか
		return FIR_INVALID_CODE_PAGE;
	const CodePage cp = encoderFactory.isCodePageForAutoDetection(params.codePage) ? codePage_ : params.codePage;

	// Unicode コードページでしか使えない改行コード
	if((params.newline == NLF_NEL || params.newline == NLF_LS || params.newline == NLF_PS)
			&& cp != CPEX_UNICODE_UTF5 && cp != CP_UTF7 && cp != CP_UTF8
			&& cp != CPEX_UNICODE_UTF16LE && cp != CPEX_UNICODE_UTF16BE
			&& cp != CPEX_UNICODE_UTF32LE && cp != CPEX_UNICODE_UTF32BE)
		return FIR_INVALID_NEWLINE;

	ui::WaitCursor wc;
	FileIOResult result = FIR_OK;

	// query progression callback
	IFileIOProgressListener* progressEvent = (callback != 0) ? callback->queryProgressCallback() : 0;
	const length_t intervalLineCount = (progressEvent != 0) ? progressEvent->queryIntervalLineCount() : 0;

	// 1 行ずつ変換してバッファに書き込み、後で一度にファイルに書き込む
	auto_ptr<Encoder> encoder(encoderFactory.createEncoder(cp));
	UnconvertableCharCallback* internalCallback = (callback != 0) ? new UnconvertableCharCallback(callback) : 0;
	const length_t lineCount = getNumberOfLines();	// 総行数
	const size_t nativeBufferBytes = (length(NLR_CRLF)) * encoder->getMaxNativeCharLength() + 4;
	uchar* const nativeBuffer = static_cast<uchar*>(::HeapAlloc(::GetProcessHeap(), HEAP_NO_SERIALIZE, nativeBufferBytes));
	size_t offset = 0;	// offset in bytes

	if(nativeBuffer == 0) {
		delete internalCallback;
		return FIR_OUT_OF_MEMORY;
	}

	// BOM
	if(params.options.has(SaveParameters::WRITE_UNICODE_BOM)) {
		size_t signatureSize = 0;
		switch(cp) {
		case CP_UTF8:
			signatureSize = countof(encodings::UTF8_BOM) - 1;
			memcpy(nativeBuffer, encodings::UTF8_BOM, signatureSize);
			break;
		case CPEX_UNICODE_UTF16LE:
			signatureSize = countof(encodings::UTF16LE_BOM) - 1;
			memcpy(nativeBuffer, encodings::UTF16LE_BOM, signatureSize);
			break;
		case CPEX_UNICODE_UTF16BE:
			signatureSize = countof(encodings::UTF16BE_BOM) - 1;
			memcpy(nativeBuffer, encodings::UTF16BE_BOM, signatureSize);
			break;
		case CPEX_UNICODE_UTF32LE:
			signatureSize = countof(encodings::UTF32LE_BOM) - 1;
			memcpy(nativeBuffer, encodings::UTF32LE_BOM, signatureSize);
			break;
		case CPEX_UNICODE_UTF32BE:
			signatureSize = countof(encodings::UTF32BE_BOM) - 1;
			memcpy(nativeBuffer, encodings::UTF32BE_BOM, signatureSize);
			break;
		}
		offset += sizeof(uchar) * signatureSize;
	}

	for(length_t i = 0; i < lineCount; ++i) {
		const Line& line = *lines_[i];
		if(i == lineCount - 1) {	// the last line
			if(line.text_.empty())
				break;
		}
		size_t convertedBytes;

		if(!line.text_.empty()) {
			convertedBytes = encoder->fromUnicode(nativeBuffer + offset, nativeBufferBytes - offset,
							line.text_.data(), line.text_.length(),
							(internalCallback != 0 && !internalCallback->isCalledOnce()) ? internalCallback : 0);
			if(convertedBytes == 0)
				goto CLIENT_ABORTED;
			offset += convertedBytes;
		}
		if(i != lineCount - 1) {
			const String nlf(getNewlineString((params.newline != NLF_AUTO) ? params.newline : line.newline_));
			convertedBytes = encoder->fromUnicode(nativeBuffer + offset,
				nativeBufferBytes - offset, nlf.data(), nlf.length(),
				(internalCallback != 0 && !internalCallback->isCalledOnce()) ? internalCallback : 0);
			if(convertedBytes == 0)
				goto CLIENT_ABORTED;
			offset += convertedBytes;
		}
		continue;
CLIENT_ABORTED:
		::HeapFree(::GetProcessHeap(), HEAP_NO_SERIALIZE, nativeBuffer);
		delete internalCallback;
		return FIR_CALLER_ABORTED;
	}

	diskFile_.unlock();

	bool byCopying = params.options.has(SaveParameters::BY_COPYING);
	if(byCopying) {
		if(toBoolean(::PathFileExistsW(fileName.c_str())))
			byCopying = false;
		else {
			// don't copy if the destination is removal
			const int driveNumber = ::PathGetDriveNumberW(fileName.c_str());
			if(driveNumber != -1) {
				WCHAR drive[4];
				::PathBuildRootW(drive, driveNumber);
				switch(::GetDriveTypeW(drive)) {
				case DRIVE_CDROM:	case DRIVE_FIXED:
				case DRIVE_RAMDISK:	case DRIVE_REMOVABLE:
					byCopying = false;
					break;
				}
			}
		}
	}

	// prepare temporary file
	WCHAR tempName[MAX_PATH + 1];
	{
		WCHAR path[MAX_PATH + 1];
		const WCHAR* const name = ::PathFindFileNameW(fileName.c_str());
		wcsncpy(path, fileName.c_str(), name - fileName.c_str() - 1);
		path[name - fileName.c_str() - 1] = 0;
		::GetTempFileNameW(path, name, 0, tempName);
	}

	// TODO: support backup on writing.
/*	// デバッグバージョンは常にバックアップを作る (上書きの場合のみ)
	if(
#ifdef _DEBUG
	true ||
#endif
	(options.has(SDO_CREATE_BACKUP) && toBoolean(::PathFileExistsW(fileName.c_str())))) {
		WCHAR backupPath[MAX_PATH + 1];
		SHFILEOPSTRUCTW	shfos = {
			0, FO_DELETE, backupPath, 0, FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT, false, 0
		};
		wcscpy(backupPath, filePath.c_str());
		wcscat(backupPath, L".bak");
		backupPath[wcslen(backupPath) + 1] = 0;
		::CopyFileW(filePath.c_str(), backupPath, false);
		::SHFileOperationW(&shfos);	// ごみ箱に持っていく
	}
*/

#define FREE_TEMPORARY_BUFFER()											\
	::HeapFree(::GetProcessHeap(), HEAP_NO_SERIALIZE, nativeBuffer);	\
	delete internalCallback

	bool deletedOldFile = false;
	byCopying = false;	// TODO: support "by-copying" file writing.
	if(byCopying) {
/*		if(!toBoolean(::CopyFileW(getPathName(), tempName, true)))
			return FIR_STANDARD_WIN32_ERROR;
		File<true> file(filePath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL);
		if(::GetLastError() == ERROR_ACCESS_DENIED) {
			if(toBoolean(originalAttrs & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM))) {
				if(!toBoolean(::SetFileAttributesW(filePath.c_str(), originalAttrs & ~(FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM))))
					return FIR_STANDARD_WIN32_ERROR;
				file.open(filePath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL);
				resolvedAccess = true;
			}
		}
		bool resolvedAccess = false;
		FREE_TEMPORARY_BUFFER();
*/	} else {
		// 保存用のファイルを作成 -> 元のファイルを削除 (上書きの場合) -> 名前を変更
		File<true> tempFile(tempName, GENERIC_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_ARCHIVE);
		if(!tempFile.isOpened()) {
			FREE_TEMPORARY_BUFFER();
			return FIR_CANNOT_CREATE_TEMPORARY_FILE;
		}
		tempFile.write(nativeBuffer, static_cast<DWORD>(offset));
		tempFile.close();
		FREE_TEMPORARY_BUFFER();
		diskFile_.unlock();
		const DWORD attrs = ::GetFileAttributesW(realName.c_str());
		if(attrs != INVALID_FILE_ATTRIBUTES) {
			::SetFileAttributesW(tempName, attrs);
			// 上書きの場合は古いファイルを消す
			if(!toBoolean(::DeleteFileW(realName.c_str())) && ::GetLastError() != ERROR_FILE_NOT_FOUND) {
				const DWORD e = ::GetLastError();
				::DeleteFileW(tempName);
				// 自分が開いていたファイルだった場合はロックし直す
				if(diskFile_.lockMode.type != FileLockMode::LOCK_TYPE_NONE
						&& isBoundToFile() && (!diskFile_.lockMode.onlyAsEditing || isModified())
						&& comparePathNames(realName.c_str(), getFilePathName()))
					diskFile_.lock(getFilePathName());
				::SetLastError(e);
				return FIR_STANDARD_WIN32_ERROR;
			}
			deletedOldFile = true;
		}
		// rename the file
		const bool moved = toBoolean(::MoveFileW(tempName, realName.c_str()));
		if(!moved && deletedOldFile)	// worst case (lost)
			return FIR_LOST_DISK_FILE;
		// relock the file
		if(!deletedOldFile && diskFile_.lockMode.type != FileLockMode::LOCK_TYPE_NONE && !diskFile_.lockMode.onlyAsEditing)
			diskFile_.lock(realName.c_str());
		if(!moved) {
			const DWORD e = ::GetLastError();
			::DeleteFileW(tempName);
			::SetLastError(e);
			return FIR_STANDARD_WIN32_ERROR;
		}
	}

#undef FREE_TEMPORARY_BUFFER

	if(params.newline != NLF_AUTO) {
		setNewline(params.newline);	// determine the newlines
		for(length_t i = 0; i < lines_.getSize(); ++i) {
			Line& line = *lines_[i];
			line.operationHistory_ = 0;		// erase the operation history
			line.newline_ = params.newline;	// overwrite the newline
		}
	} else {
		for(length_t i = 0; i < lines_.getSize(); ++i)
			lines_[i]->operationHistory_ = 0;
	}
	undoManager_->documentSaved();
	setModified(false);
	setReadOnly(false);
	setCodePage(cp);
	setFilePathName(realName.c_str());

	// update the internal time stamp
	::WIN32_FIND_DATAW wfd;
	HANDLE find = ::FindFirstFileW(getFilePathName(), &wfd);
	if(find != INVALID_HANDLE_VALUE) {
		diskFile_.lastWriteTimes.internal = diskFile_.lastWriteTimes.user = wfd.ftLastWriteTime;
		::FindClose(find);
	} else {
		memset(&diskFile_.lastWriteTimes.internal, 0, sizeof(::FILETIME));
		memset(&diskFile_.lastWriteTimes.user, 0, sizeof(::FILETIME));
	}

	return result;
}

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

	AutoZero<::MapiMessage> message;
	ulong error;

	message.flFlags = MAPI_RECEIPT_REQUESTED;

	if(asAttachment) {	// 添付ファイルにするとき
		AutoZero<::MapiFileDesc> fileDesc;
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

/**
 * Sets the default encoding and newline.
 * @param cp the code page of the encoding
 * @param newline the newline
 * @throw std#invalid_argument @a cp and/or @a newline are invalid
 */
void Document::setDefaultCode(CodePage cp, Newline newline) {
	cp = translateSpecialCodePage(cp);
	if(!EncoderFactory::getInstance().isValidCodePage(cp)
			|| EncoderFactory::getInstance().isCodePageForAutoDetection(cp))
		throw invalid_argument("Specified code page is not available.");
	switch(newline) {
	case NLF_LF:	case NLF_CR:	case NLF_CRLF:
	case NLF_NEL:	case NLF_LS:	case NLF_PS:
		break;
	default:
		throw invalid_argument("Specified newline is invalid.");
	}
	defaultCodePage_ = cp;
	defaultNewline_ = newline;
}

/**
 * Renames (or move) the bound file.
 * @param pathName the fill name
 */
void Document::setFilePathName(const WCHAR* pathName) {
	if(pathName != 0) {
		if(diskFile_.pathName != 0 && wcscmp(pathName, diskFile_.pathName) == 0)
			return;
		delete[] diskFile_.pathName;
		diskFile_.pathName = new TCHAR[wcslen(pathName) + 1];
		wcscpy(diskFile_.pathName, pathName);
	} else if(diskFile_.pathName == 0)
		return;
	else {
		delete[] diskFile_.pathName;
		diskFile_.pathName = 0;
	}
	stateListeners_.notify<Document&>(IDocumentStateListener::documentFileNameChanged, *this);
}

/**
 * Sets the modification flag of the document.
 * Derived classes can hook changes of the flag by overriding this method.
 * @param modified set true to make the document modfied
 * @see #isModified, IDocumentStateListener#documentModificationSignChanged
 */
void Document::setModified(bool modified /* = true */) throw() {
	const bool was = modified_;
	if(modified != modified_) {
		modified_ = modified;
		stateListeners_.notify<Document&>(IDocumentStateListener::documentModificationSignChanged, *this);
	}
	if(modified_ != was && diskFile_.lockMode.onlyAsEditing && isBoundToFile()) {
		if(was)
			diskFile_.unlock();
		else
			diskFile_.lock(getFilePathName());
	}
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
 * Translates the special Win32 code page to concrete one.
 * @param cp the code page to be translated
 * @return the concrete code page
 */
CodePage Document::translateSpecialCodePage(CodePage cp) {
	if(cp == CP_ACP)
		return ::GetACP();
	else if(cp == CP_OEMCP)
		return ::GetOEMCP();
	else if(cp == CP_MACCP) {
		wchar_t	wsz[7];
		::GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_IDEFAULTMACCODEPAGE, wsz, 6);
		return (wcscmp(wsz, L"2") != 0) ? wcstoul(wsz, 0, 10) : 0;
	} else if(cp == CP_THREAD_ACP) {
		wchar_t	wsz[7];
		::GetLocaleInfoW(::GetThreadLocale(), LOCALE_IDEFAULTANSICODEPAGE, wsz, 6);
		return (wcscmp(wsz, L"3") != 0) ? wcstoul(wsz, 0, 10) : 0;
	}
	return cp;
}

/**
 * @brief アンドゥの実行
 *
 * 操作対象がアクセス不能であればリドゥは行われない
 * @return アンドゥできなかった場合は false を返す
 * @throw ReadOnlyDocumentException the document is read only
 * @see #redo
 */
bool Document::undo() {
	if(isReadOnly())
		throw ReadOnlyDocumentException();
	else if(getUndoHistoryLength(false) == 0)
		return false;

	beginSequentialEdit();
	sequentialEditListeners_.notify<Document&>(ISequentialEditListener::documentUndoSequenceStarted, *this);
	Position resultPosition;
	const bool succeeded = undoManager_->undo(resultPosition);
	sequentialEditListeners_.notify<Document&, const Position&>(ISequentialEditListener::documentUndoSequenceStopped, *this, resultPosition);
	endSequentialEdit();

	if(!undoManager_->isModifiedSinceLastSave())
		setModified(false);
	return succeeded;
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
 * Returns last modified time.
 * @param internal set true for @c diskFile_.lastWriteTimes.internal, false for @c user
 * @param[out] newTimeStamp ファイルの実際の更新日時
 * @return 自分で書き込んだ日時と一致しなければ false
 */
bool Document::verifyTimeStamp(bool internal, ::FILETIME& newTimeStamp) throw() {
	const ::FILETIME& about = internal ? diskFile_.lastWriteTimes.internal : diskFile_.lastWriteTimes.user;
	if(!isBoundToFile()	// ファイルに束縛されていない場合や排他制御を行っている場合は無意味
			|| (about.dwLowDateTime == 0 && about.dwHighDateTime == 0)
			|| diskFile_.lockMode.type != FileLockMode::LOCK_TYPE_NONE)
		return true;

	::WIN32_FIND_DATAW wfd;
	HANDLE find = ::FindFirstFileW(getFilePathName(), &wfd);
	if(find == INVALID_HANDLE_VALUE)
		return true;	// ファイルは既に存在しない?
	::FindClose(find);
	newTimeStamp = wfd.ftLastWriteTime;
	return ::CompareFileTime(&about, &newTimeStamp) != -1;
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
		stateListeners_.notify<Document&>(IDocumentStateListener::documentAccessibleRegionChanged, *this);
	}
}

/**
 * Writes the specified region to the specified file.
 * @param fileName the file name
 * @param region the region to be written
 * @param params the options. set @a newline member of this object to @c NLF_AUTO not to unify the
 * newlines. set @a codePage member of this object to @c CPEX_AUTODETECT to use the current code page
 * @param append true to append to the file
 * @param callback the callback or @c null
 * @return the result. @c FIR_OK if succeeded
 */
Document::FileIOResult Document::writeRegion(const basic_string<WCHAR>& fileName,
		const Region& region, const SaveParameters& params, bool append, IFileIOListener* callback /* = 0 */) {
	// TODO: not implemented.
	return FIR_OK;
}

/**
 * Writes the content of the document to the specified output stream.
 * @param out the output stream
 * @param region the region to be written (this region is not restricted with narrowing)
 * @param lbr the line break to be used
 */
void Document::writeToStream(OutputStream& out, const Region& region, NewlineRepresentation nlr /* = NLR_PHYSICAL_DATA */) const {
	const Position& start = region.beginning();
	const Position end = min(region.end(), this->region().second);
	if(start.line == end.line) {	// shortcut for single-line
		out << getLine(end.line).substr(start.column, end.column - start.column);
		return;
	}
	Char eol[3] = L"";
	switch(nlr) {
	case NLR_LINE_FEED:			wcscpy(eol, L"\n"); break;
	case NLR_CRLF:				wcscpy(eol, L"\r\n"); break;
	case NLR_LINE_SEPARATOR:	wcscpy(eol, L"\x2028"); break;
	case NLR_DOCUMENT_DEFAULT:	wcscpy(eol, getNewlineString(getNewline())); break;
	}
	const streamsize eolSize = static_cast<streamsize>(wcslen(eol));
	for(length_t i = start.line; ; ++i) {
		const Line& line = *lines_[i];
		const length_t first = (i == start.line) ? start.column : 0;
		const length_t last = (i == end.line) ? end.column : line.text_.length();
		out.write(line.text_.data() + first, static_cast<streamsize>(last - first));
		if(i == end.line)
			return;
		if(nlr == NLR_PHYSICAL_DATA)
			out << getNewlineString(line.newline_);
		else if(nlr != NLR_SKIP)
			out.write(eol, eolSize);
	}
}


// Document.DiskFile /////////////////////////////////////////////////////////

/// Returns the file is locked.
inline bool Document::DiskFile::isLocked() const throw() {
	return lockingFile.get() != 0 && lockingFile->isOpened();
}

/**
 * Locks the specified file.
 * @param fileName the name of the file
 * @return succeeded or not
 */
inline bool Document::DiskFile::lock(const WCHAR* fileName) throw() {
	if(isLocked())
		unlock();
	if(lockingFile.get() == 0)
		lockingFile.reset(new File<true>(fileName, GENERIC_READ,
			lockMode.type == FileLockMode::LOCK_TYPE_SHARED ? FILE_SHARE_READ : 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL));
	return lockingFile->isOpened();
}

/**
 * Unlocks the file.
 * @return succeeded or not
 */
inline bool Document::DiskFile::unlock() throw() {
	if(!isLocked() || lockingFile->close()) {
		lockingFile.reset(0);
		return true;
	}
	return false;
}


// DocumentCharacterIterator ////////////////////////////////////////////////

const CharacterIterator::ConcreteTypeTag DocumentCharacterIterator::CONCRETE_TYPE_TAG_;

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
		region_(document.region()), line_(&document.getLine(position.line)), p_(position) {
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
		line_(&document.getLine(region.beginning().line)), p_(region.beginning()) {
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
		CharacterIterator(CONCRETE_TYPE_TAG_), document_(&document), region_(region), line_(&document.getLine(position.line)), p_(position) {
	region_.normalize();
	if(!document.region().encompasses(region_))
		throw BadRegionException();
	else if(!region_.includes(p_))
		throw BadPositionException();
}

/// Copy-constructor.
DocumentCharacterIterator::DocumentCharacterIterator(const DocumentCharacterIterator& rhs) throw() :
		unicode::CharacterIterator(rhs), document_(rhs.document_), region_(rhs.region_), line_(rhs.line_), p_(rhs.p_) {
}

/// @see unicode#CharacterIterator#current
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

/// @see unicode#CharacterIterator#doAssign
void DocumentCharacterIterator::doAssign(const CharacterIterator& rhs) {
	CharacterIterator::operator=(rhs);
	const DocumentCharacterIterator& r = static_cast<const DocumentCharacterIterator&>(rhs);
	document_ = r.document_;
	line_ = r.line_;
	p_ = r.p_;
	region_ = r.region_;
}

/// @see unicode#CharacterIterator#doClone
auto_ptr<CharacterIterator> DocumentCharacterIterator::doClone() const {
	return auto_ptr<CharacterIterator>(new DocumentCharacterIterator(*this));
}

/// @see unicode#CharacterIterator#doFirst
void DocumentCharacterIterator::doFirst() {
	seek(region_.first);
}

/// @see unicode#CharacterIterator#doLast
void DocumentCharacterIterator::doLast() {
	seek(region_.second);
}

/// @see unicode#CharacterIterator#doEquals
bool DocumentCharacterIterator::doEquals(const CharacterIterator& rhs) const {
	return p_ == static_cast<const DocumentCharacterIterator&>(rhs).p_;
}

/// @see unicode#CharacterIterator#doLess
bool DocumentCharacterIterator::doLess(const CharacterIterator& rhs) const {
	return p_ < static_cast<const DocumentCharacterIterator&>(rhs).p_;
}

/// @see unicode#CharacterIterator#doNext
void DocumentCharacterIterator::doNext() {
	if(!hasNext())
//		throw out_of_range("the iterator is at the last.");
		return;
	else if(p_.column == line_->length()) {
		line_ = &document_->getLine(++p_.line);
		p_.column = 0;
	} else if(++p_.column < line_->length()
			&& surrogates::isLowSurrogate((*line_)[p_.column]) && surrogates::isHighSurrogate((*line_)[p_.column - 1]))
		++p_.column;
}

/// @see unicode#CharacterIterator#doPrevious
void DocumentCharacterIterator::doPrevious() {
	if(!hasPrevious())
//		throw out_of_range("the iterator is at the first.");
		return;
	else if(p_.column == 0)
		p_.column = (line_ = &document_->getLine(--p_.line))->length();
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
 * @throw std#invalid_argument @a mode is invalid
 */
DocumentBuffer::DocumentBuffer(Document& document, const Position& initialPosition /* = Position::ZERO_POSITION */,
		NewlineRepresentation nlr /* = NLR_PHYSICAL_DATA */, ios_base::openmode streamMode /* = ios_base::in | ios_base::out */) :
		document_(document), nlr_(nlr), mode_(streamMode), current_(initialPosition) {
	if((mode_ & ~(ios_base::in | ios_base::out)) != 0)
		throw invalid_argument("the given mode is invalid.");
	setp(buffer_, endof(buffer_) - 1);
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
	setp(buffer_, endof(buffer_) - 1);
	if(buffer_ < p)
		current_ = document_.insert(current_, buffer_, p);
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


// NullPartitioner //////////////////////////////////////////////////////////

/// Constructor.
NullPartitioner::NullPartitioner() throw() : p_(DEFAULT_CONTENT_TYPE, Region(Position::ZERO_POSITION, Position::INVALID_POSITION)) {
}

/// @see DocumentPartitioner#documentAboutToBeChanged
void NullPartitioner::documentAboutToBeChanged() {
}

/// @see DocumentPartitioner#documentChanged
void NullPartitioner::documentChanged(const text::DocumentChange& change) {
	p_.region.second.line = INVALID_INDEX;
}

/// @see DocumentPartitioner#doGetPartition
void NullPartitioner::doGetPartition(const Position&, DocumentPartition& partition) const throw() {
	if(p_.region.second.line == INVALID_INDEX)
		const_cast<NullPartitioner*>(this)->p_.region.second = getDocument()->region().second;
	partition = p_;
}

/// @see DocumentPartitioner#doInstall
void NullPartitioner::doInstall() throw() {
	p_.region.second.line = INVALID_INDEX;
}


// free functions ///////////////////////////////////////////////////////////

/**
 * Makes the specified path name real. If the path is UNC, the case of @a pathName will not be
 * fixed. All slashes will be replaced by backslashes. Even if the path name is not exist, this
 * method will not fail.
 * @param pathName the absolute path name
 * @return the result real path name
 * @see comparePathNames
 */
wstring text::canonicalizePathName(const wchar_t* pathName) throw() {
	wchar_t path[MAX_PATH];
	resolveRelativePathName(pathName, path);

	// from Ftruename implementation in xyzzy

	wstring result;
	result.reserve(MAX_PATH);
	const wchar_t* p = path;
	if(((p[0] >= L'A' && p[0] <= L'Z') || (p[0] >= L'a' && p[0] <= L'z'))
			&& p[1] == L':' && (p[2] == L'\\' || p[2] == L'/')) {	// drive letter
		result.append(path, 3);
		result[0] = towupper(path[0]);	// unify with uppercase letters...
		p += 3;
	} else if((p[0] == L'\\' || p[0] == L'/') && (p[1] == L'\\' || p[1] == L'/')) {	// UNC?
		if((p = wcspbrk(p + 2, L"\\/")) == 0)	// server name
			return false;
		if((p = wcspbrk(p + 1, L"\\/")) == 0)	// shared name
			return false;
		result.append(path, ++p - path);
	} else	// not absolute name
		return pathName;

	::WIN32_FIND_DATAW wfd;
	while(true) {
		if(wchar_t* next = wcspbrk(p, L"\\/")) {
			const wchar_t c = *next;
			*next = 0;
			HANDLE h = ::FindFirstFileW(path, &wfd);
			if(h != INVALID_HANDLE_VALUE) {
				::FindClose(h);
				result += wfd.cFileName;
			} else
				result += p;
			*next = c;
			result += L'\\';
			p = next + 1;
		} else {
			HANDLE h = ::FindFirstFileW(path, &wfd);
			if(h != INVALID_HANDLE_VALUE) {
				::FindClose(h);
				result += wfd.cFileName;
			} else
				result += p;
			break;
		}
	}
	return result;
}

/**
 * Returns true if the specified two file path names are equivalent.
 * @param s1 the first path name
 * @param s2 the second path name
 * @return true if @a s1 and @a s2 are equivalent
 * @throw NullPointerException either file name is @c null
 * @see canonicalizePathName
 */
bool text::comparePathNames(const wchar_t* s1, const wchar_t* s2) {
	if(s1 == 0 || s2 == 0)
		throw NullPointerException("either file name is null.");

//	// 最も簡単な方法
//	if(toBoolean(::PathMatchSpecW(s1, s2)))
//		return true;

	// from pathname_equal and same_file_p implementation in xyzzy

	// ファイル名の文字列比較
	const int c1 = static_cast<int>(wcslen(s1)) + 1, c2 = static_cast<int>(wcslen(s2)) + 1;
	const int fc1 = ::LCMapStringW(LOCALE_NEUTRAL, LCMAP_LOWERCASE, s1, c1, 0, 0);
	const int fc2 = ::LCMapStringW(LOCALE_NEUTRAL, LCMAP_LOWERCASE, s2, c2, 0, 0);
	if(fc1 != 0 && fc2 != 0 && fc1 == fc2) {
		manah::AutoBuffer<WCHAR> fs1(new WCHAR[fc1]), fs2(new WCHAR[fc2]);
		::LCMapStringW(LOCALE_NEUTRAL, LCMAP_LOWERCASE, s1, c1, fs1.get(), fc1);
		::LCMapStringW(LOCALE_NEUTRAL, LCMAP_LOWERCASE, s2, c2, fs2.get(), fc2);
		if(wmemcmp(fs1.get(), fs2.get(), fc1) == 0)
			return toBoolean(::PathFileExists(s1));
	}

	// ボリューム情報を使う
	File<true> f1(s1, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS);
	if(!f1.isOpened())
		f1.open(s1, 0, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS);
	if(!f1.isOpened())
		return false;
	File<true> f2(s2, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS);
	if(!f2.isOpened())
		f2.open(s2, 0, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS);
	if(!f2.isOpened())
		return false;
	::BY_HANDLE_FILE_INFORMATION fi1;
	if(toBoolean(::GetFileInformationByHandle(f1.get(), &fi1))) {
		::BY_HANDLE_FILE_INFORMATION fi2;
		if(toBoolean(::GetFileInformationByHandle(f2.get(), &fi2)))
			return fi1.dwVolumeSerialNumber == fi2.dwVolumeSerialNumber
				&& fi1.nFileIndexHigh == fi2.nFileIndexHigh
				&& fi1.nFileIndexLow == fi2.nFileIndexLow;
	}
	return false;
}

/**
 * Returns absolute character offset of the specified position from the start of the document.
 * @param document the document
 * @param at the position
 * @param fromAccessibleStart
 * @throw BadPositionException @a at is outside of the document
 */
length_t text::getAbsoluteOffset(const Document& document, const Position& at, bool fromAccessibleStart) {
	if(at > document.region().second)
		throw BadPositionException();
	length_t offset = 0;
	const Position start((fromAccessibleStart ? document.accessibleRegion() : document.region()).first);
	for(length_t line = start.line; ; ++line) {
		if(line == at.line) {
			offset += at.column;
			break;
		} else {
			offset += document.getLineLength(line) + 1;	// +1 is for a newline character
			if(line == start.line)
				offset -= start.column;
		}
	}
	return offset;
}

/**
 * Returns the number of lines in the specified text string.
 * @param first the start of the text string
 * @param last the end of the text string
 * @return the number of lines
 */
length_t text::getNumberOfLines(const Char* first, const Char* last) throw() {
	if(first == last)
		return 0;
	length_t lines = 1;
	while(true) {
		first = find_first_of(first, last, LINE_BREAK_CHARACTERS, endof(LINE_BREAK_CHARACTERS));
		if(first == last)
			break;
		++lines;
		first += (*first == CARRIAGE_RETURN && first < last - 1 && first[1] == LINE_FEED) ? 2 : 1;
	}
	return lines;
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
Position text::updatePosition(const Position& position, const DocumentChange& change, Direction gravity) throw() {
	Position newPosition(position);
	if(!change.isDeletion()) {	// 挿入操作の場合
		if(position < change.getRegion().first)	// 現在位置より後方
			return newPosition;
		else if(position == change.getRegion().first && gravity == BACKWARD) // 現在位置 + backward gravity
			return newPosition;
		else if(position.line > change.getRegion().first.line)	// 現在行より前
			newPosition.line += change.getRegion().second.line - change.getRegion().first.line;
		else {	// 現在行と同じ行
			newPosition.line += change.getRegion().second.line - change.getRegion().first.line;
			newPosition.column += change.getRegion().second.column - change.getRegion().first.column;
		}
	} else {	// 削除操作の場合
		if(position < change.getRegion().second) {	// 終端が現在位置より後方
			if(position <= change.getRegion().first)
				return newPosition;
			else	// 範囲内
				newPosition = change.getRegion().first;
		} else if(position.line > change.getRegion().second.line)	// 現在行より前
			newPosition.line -= change.getRegion().second.line - change.getRegion().first.line;
		else {	// 終了点が現在行と同じ行
			if(position.line == change.getRegion().first.line)	// 範囲が1行
				newPosition.column -= change.getRegion().second.column - change.getRegion().first.column;
			else {	// 範囲が複数行
				newPosition.line -= change.getRegion().second.line - change.getRegion().first.line;
				newPosition.column -= change.getRegion().second.column - change.getRegion().first.column;
			}
		}
	}
	return newPosition;
}
