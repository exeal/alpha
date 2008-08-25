/**
 * @file point.cpp
 * @author exeal
 * @date 2003-2008
 */

#include "point.hpp"
#include "session.hpp"
using namespace ascension;
using namespace ascension::kernel;
using namespace ascension::text;
using namespace std;


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
	lifeCycleListeners_.notify(&IPointLifeCycleListener::pointDestroyed);
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
	position.line = min(position.line, document_->numberOfLines() - 1);
	position.column = min(position.column, document_->lineLength(position.line));
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


// EditPoint ////////////////////////////////////////////////////////////////

namespace {
} // namespace @0

/**
 * Constructor.
 * @param document the document
 * @param position the initial position of the point
 * @param listener the listener. can be @c null if not needed
 * @throw BadPositionException @a position is outside of the document
 */
EditPoint::EditPoint(Document& document, const Position& position /* = Position() */, IPointListener* listener /* = 0 */)
	: Point(document, position), listener_(listener), characterUnit_(GRAPHEME_CLUSTER) {
}

/// Copy-constructor.
EditPoint::EditPoint(const EditPoint& rhs) throw() : Point(rhs), listener_(rhs.listener_), characterUnit_(rhs.characterUnit_) {
}

/// Destructor.
EditPoint::~EditPoint() throw() {
}

/**
 * Moves to the previous (backward) character.
 * @param offset the offset of the movement
 */
void EditPoint::backwardCharacter(length_t offset /* = 1 */) {
	verifyDocument();
	normalize();
	moveTo(getBackwardCharacterPosition(*document(), *this, characterUnit(), offset));
}

/// Moves to the beginning of the document.
void EditPoint::beginningOfDocument() {
	moveTo(Position::ZERO_POSITION);
}

/// Moves to the beginning of the line.
void EditPoint::beginningOfLine() {
	moveTo(Position(min(lineNumber(), document()->numberOfLines() - 1), 0));
}

/**
 * Deletes the current character and inserts the specified text.
 * @param first the start of the text
 * @param last the end of the text
 */
void EditPoint::destructiveInsert(const Char* first, const Char* last) {
	verifyDocument();
	if(document()->isReadOnly())
		return;
	EditPoint p = EditPoint(*this);
	p.adaptToDocument(false);
	p.forwardCharacter();
	if(p != *this) {
		const bool adapts = adaptsToDocument();
		adaptToDocument(false);
		document()->erase(Region(*this, p));
		moveTo(document()->insert(*this, first, last));
		adaptToDocument(adapts);
	}
}

/// @see Point#doMoveTo
void EditPoint::doMoveTo(const Position& to) {
	verifyDocument();
	if(to != position()) {
		const Position oldPosition = position();
		Point::doMoveTo(to);
		if(listener_ != 0)
			listener_->pointMoved(*this, oldPosition);
	}
}

/// Moves to the end of the document.
void EditPoint::endOfDocument() {
	const length_t lines = document()->numberOfLines();
	moveTo(Position(lines - 1, document()->lineLength(lines - 1)));
}

/// Moves to the end of the line.
void EditPoint::endOfLine() {
	moveTo(Position(min(lineNumber(), document()->numberOfLines() - 1), document()->lineLength(lineNumber())));
}

/**
 * 指定位置までのテキストを削除
 * @param length もう1つの位置までの文字数 (負でもよい)
 * @param cu 文字数の計算方法。@c DEFAULT_UNIT を指定すると現在の設定が使用される
 */
void EditPoint::erase(signed_length_t length /* = 1 */, EditPoint::CharacterUnit cu /* = DEFAULT_UNIT */) {
	verifyDocument();
	if(document()->isReadOnly() || length == 0)
		return;
	if(cu == DEFAULT_UNIT)
		cu = characterUnit();
	erase((length > 0) ?
		getForwardCharacterPosition(*document(), *this, characterUnit(), length)
		: getBackwardCharacterPosition(*document(), *this, cu, -length));
}

/**
 * Erase the region between the point and the other specified point.
 * @param other the other point
 */
void EditPoint::erase(const Position& other) {
	verifyDocument();
	if(document()->isReadOnly() || other == position())
		return;
	document()->erase(Region(*this, other));
}

/**
 * Moves to the next (forward) character.
 * @param offset the offset of the movement
 */
void EditPoint::forwardCharacter(length_t offset /* = 1 */) {
	verifyDocument();
	normalize();
	moveTo(getForwardCharacterPosition(*document(), *this, characterUnit(), offset));
}

/**
 * Returns the position of the previous character.
 * @param document the document
 * @param position the origin
 * @param cu the character unit
 * @param offset the amount of the movement
 * @return the result position
 * @throw UnknownValueException @a cu is invalid
 */
Position EditPoint::getBackwardCharacterPosition(const Document& document,
		const Position& position, EditPoint::CharacterUnit cu, length_t offset /* = 1 */) {
	assert(cu != DEFAULT_UNIT);
	if(offset == 0)
		return position;
	else if(cu == UTF16_CODE_UNIT) {
		UTF32To16Iterator<DocumentCharacterIterator> i(DocumentCharacterIterator(document, position));
		while(offset-- > 0)
			--i;
		return i.tell().tell();
	} else if(cu == UTF32_CODE_UNIT) {
		DocumentCharacterIterator i(document, position);
		while(offset-- > 0)
			i.previous();
		return i.tell();
	} else if(cu == GRAPHEME_CLUSTER) {
		GraphemeBreakIterator<DocumentCharacterIterator> i(DocumentCharacterIterator(document, position));
		i.next(-static_cast<signed_length_t>(offset));
		return i.base().tell();
	} else if(cu == GLYPH_CLUSTER) {
		// TODO: not implemented.
	}
	throw UnknownValueException("cu");
}

/**
 * Returns the code point of the current character.
 * @param useLineFeed true to return LF (U+000A) when the current position is end of the line. otherwise LS (U+2008)
 * @return the code point. if the current position is end of document, result is @c INVALID_CODE_POINT
 */
CodePoint EditPoint::getCodePoint(bool useLineFeed /* = false */) const {
	verifyDocument();
	const String& line = document()->line(lineNumber());
	if(columnNumber() == line.length())
		return (lineNumber() == document()->numberOfLines() - 1) ? INVALID_CODE_POINT : (useLineFeed ? LINE_FEED : LINE_SEPARATOR);
	return surrogates::decodeFirst(line.begin() + columnNumber(), line.end());
}

/**
 * Returns the position of the next character.
 * @param document the document
 * @param position the origin
 * @param cu the character unit
 * @param offset the amount of the movement
 * @return the result position
 * @throw UnknownValueException @a cu is invalid
 */
Position EditPoint::getForwardCharacterPosition(const Document& document,
		const Position& position, EditPoint::CharacterUnit cu, length_t offset /* = 1 */) {
	if(offset == 0)
		return position;
	else if(cu == UTF16_CODE_UNIT) {
		UTF32To16Iterator<DocumentCharacterIterator> i(DocumentCharacterIterator(document, position));
		while(offset-- > 0)
			++i;
		return i.tell().tell();
	} else if(cu == UTF32_CODE_UNIT) {
		DocumentCharacterIterator i(document, position);
		while(offset-- > 0)
			i.next();
		return i.tell();
	} else if(cu == GRAPHEME_CLUSTER) {
		GraphemeBreakIterator<DocumentCharacterIterator> i(DocumentCharacterIterator(document, position));
		i.next(offset);
		return i.base().tell();
	} else if(cu == GLYPH_CLUSTER) {
		// TODO: not implemented.
	}
	throw UnknownValueException("cu");
}

String EditPoint::getText(signed_length_t length, Newline newline /* = NLF_RAW_VALUE */) const {
	return getText((length >= 0) ?
		getForwardCharacterPosition(*document(), *this, characterUnit(), length)
		: getBackwardCharacterPosition(*document(), *this, characterUnit(), length), newline);
}

String EditPoint::getText(const Position& other, Newline newline /* = NLF_RAW_VALUE */) const {
	basic_ostringstream<Char> s;
	writeDocumentToStream(s, *document(), Region(*this, other), newline);
	return s.str();
}

/**
 * Inserts the spcified text at the current position.
 * @param first the start of the text
 * @param last the end of the text
 * @see viewers#VisualPoint#insertRectangle
 */
void EditPoint::insert(const Char* first, const Char* last) {
	verifyDocument();
	if(document()->isReadOnly() || first == last)
		return;
	const bool adapts = adaptsToDocument();
	adaptToDocument(false);
	moveTo(document()->insert(*this, first, last));
	adaptToDocument(adapts);
}

/// Returns true if the point is the beginning of the document.
bool EditPoint::isBeginningOfDocument() const {
	verifyDocument();
	normalize();
	return isExcludedFromRestriction() ? position() == document()->region().first : position() == Position::ZERO_POSITION;
}

/// Returns true if the point is the beginning of the line.
bool EditPoint::isBeginningOfLine() const {
	verifyDocument();
	normalize();
	if(isExcludedFromRestriction()) {
		const Position start(document()->region().first);
		return (start.line == lineNumber()) ? start.column == columnNumber() : columnNumber() == 0;
	} else
		return columnNumber() == 0;
}

/// Returns true if the point is the end of the document.
bool EditPoint::isEndOfDocument() const {
	verifyDocument();
	normalize();
	return isExcludedFromRestriction() ? position() == document()->region().second
		: lineNumber() == document()->numberOfLines() - 1 && columnNumber() == document()->lineLength(lineNumber());
}

/// Returns true if the point is the end of the line.
bool EditPoint::isEndOfLine() const {
	verifyDocument();
	normalize();
	if(isExcludedFromRestriction()) {
		const Position end = document()->region().second;
		return (end.line == lineNumber()) ?
			columnNumber() == end.column : columnNumber() == document()->lineLength(lineNumber());
	} else
		return columnNumber() == document()->lineLength(lineNumber());
}

/**
 * Moves to the specified offset.
 * @param offset the offset from the start of the document.
 * @deprecated 0.8
 */
void EditPoint::moveToAbsoluteCharacterOffset(length_t offset) {
	verifyDocument();

	length_t readCount = 0;
	const Region region(document()->region());

	if(document()->lineLength(region.first.line) + 1 - region.first.column >= offset) {
		moveTo(Position(region.first.line, region.first.column + offset));
		return;
	}
	readCount += document()->lineLength(region.first.line) + 1 - region.first.column;
	for(length_t line = region.first.line + 1; line <= region.second.line; ++line) {
		const length_t lineLength = document()->lineLength(line) + 1;	// +1 is for a newline
		if(readCount + lineLength >= offset) {
			moveTo(Position(line, readCount + lineLength - offset));
			return;
		}
		readCount += lineLength;
	}
	moveTo(Position(region.second.line, document()->lineLength(region.second.line)));
}

/**
 * Breaks the line.
 * @note This method is hidden by @c VisualPoint#newLine (C++ rule).
 */
void EditPoint::newLine() {
	verifyDocument();
	if(document()->isReadOnly())
		return;
	const IDocumentInput* const di = document()->input();
	insert(getNewlineString((di != 0) ? di->newline() : ASCENSION_DEFAULT_NEWLINE));
}

/**
 * Moves to the beginning of the next bookmarked line.
 * @return false if the bookmark is not found
 */
bool EditPoint::nextBookmark() {
	verifyDocument();

	const Bookmarker& bookmarker = document()->bookmarker();
	length_t line;
	const length_t endLine = document()->region().second.line;

	// search...
	for(line = lineNumber() + 1; line <= endLine; ++line) {
		if(bookmarker.isMarked(line)) {
			moveTo(Position(line, 0));
			return true;
		}
	}

	// wrap around if not found
	for(line = document()->region().first.line; line < lineNumber(); ++line) {
		if(bookmarker.isMarked(line)) {
			moveTo(Position(line, 0));
			return true;
		}
	}

	return false;
}

/**
 * Moves to the next line.
 * @param offset the offset of the movement
 */
void EditPoint::nextLine(length_t offset /* = 1 */) {
	verifyDocument();
	normalize();
	const length_t newLine = min(lineNumber() + offset,
		(isExcludedFromRestriction() ? document()->accessibleRegion() : document()->region()).second.line);
	if(newLine != lineNumber())
		moveTo(Position(newLine, columnNumber()));
}

/**
 * Moves to the beginning of the previous bookmarked line.
 * @return false if the bookmark is not found
 */
bool EditPoint::previousBookmark() {
	verifyDocument();

	const Bookmarker& bookmarker = document()->bookmarker();
	length_t line = lineNumber();
	const length_t startLine = document()->region().first.line;

	// search...
	while(line-- != startLine) {
		if(bookmarker.isMarked(line)) {
			moveTo(Position(line, 0));
			return true;
		}
	}

	// wrap around if not found
	for(line = document()->region().second.line; line > lineNumber(); --line) {
		if(bookmarker.isMarked(line)) {
			moveTo(Position(line, 0));
			return true;
		}
	}

	return false;
}

/**
 * Moves to the previous line.
 * @param offset the offset of the movement
 */
void EditPoint::previousLine(length_t offset /* = 1 */) {
	verifyDocument();
	normalize();
	const length_t newLine = (lineNumber() > offset) ?
		max(lineNumber() - offset,
			(isExcludedFromRestriction() ? document()->accessibleRegion() : document()->region()).first.line) : 0;
	if(newLine != lineNumber())
		moveTo(Position(newLine, columnNumber()));
}
