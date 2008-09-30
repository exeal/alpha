/**
 * @file point.cpp
 * @author exeal
 * @date 2003-2008
 */

#include "point.hpp"
#include "session.hpp"
#include "unicode-property.hpp"	// text.ucd.BinaryProperty
using namespace ascension;
using namespace ascension::kernel;
using namespace ascension::text;
using namespace std;


// Point ////////////////////////////////////////////////////////////////////

/**
 * @class ascension::kernel::Point
 *
 * A point represents a document position and adapts to the document change.
 *
 * When the document change occured, @c Point moves automatically as follows:
 *
 * - If text was inserted or deleted before the point, the point will move accordingly.
 * - If text was inserted or deleted after the point, the point will not move.
 * - If region includes the point was deleted, the point will move to the start (= end) of
 *   the region.
 * - If text was inserted at the point, the point will or will not move according to the
 *   gravity.
 *
 * For details of gravity, see the description of @c updatePosition function.
 *
 * When the document was reset (by @c Document#resetContent), the all points move to the
 * start of the document.
 *
 * Almost all methods of this or derived classes will throw @c DisposedDocumentException if
 * the document is already disposed. Call @c #isDocumentDisposed to check if the document
 * is exist or not.
 *
 * @see Position, Document, EditPoint, viewers#VisualPoint, viewers#Caret
 */

/**
 * Constructor.
 * @param document the document to which the point attaches
 * @param position the initial position of the point
 * @throw BadPositionException @a position is outside of the document
 */
Point::Point(Document& document, const Position& position /* = Position() */) :
		document_(&document), position_(position), adapting_(true), excludedFromRestriction_(false), gravity_(Direction::FORWARD) {
	if(!document.region().includes(position))
		throw BadPositionException();
	static_cast<internal::IPointCollection<Point>&>(document).addNewPoint(*this);
}

/**
 * Copy-constructor.
 * @param rhs the source object
 * @throw DisposedDocumentException the document to which @a rhs belongs had been disposed
 */
Point::Point(const Point& rhs) :
		document_(rhs.document_), position_(rhs.position_), adapting_(rhs.adapting_),
		excludedFromRestriction_(rhs.excludedFromRestriction_), gravity_(rhs.gravity_) {
	if(document_ == 0)
		throw DisposedDocumentException();
	static_cast<internal::IPointCollection<Point>*>(document_)->addNewPoint(*this);
}

/// Destructor.
Point::~Point() /*throw()*/ {
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
 * <p>Derived classes can override this method to hook all movement of the point. In this case, the
 * derived class should call from its @c doMoveTo method.</p>
 * <p>If @a to is outside of the document, the destination will be the beginning or the end of the
 * document. Otherwise if @a is outside of the accessible region and @c #isExcludedFromRestriction
 * returns true, the destination will be the beginning or the end of the accessible region. Unlike
 * @c #moveTo public interface, this does not throw about bad position.</p>
 * @param to the destination position
 * @throw DisposedDocumentException the document to which the point belongs is already disposed
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
 * Even if @a to is outside of the accessible region, this method will successes. In this case, the
 * destination position is the beginning or the end of the accessible region.
 * @param to the destination position
 * @throw BadPositionException @a to is outside of the document
 */
void Point::moveTo(const Position& to) {
	verifyDocument();
	if(to > document_->region().end())
		throw BadPositionException();
	doMoveTo(to);
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

/**
 * @class ascension::kernel::EditPoint
 * Extension of @c Point. Editable and movable in the document.
 *
 * @c Viewer のクライアントは選択範囲やキャレットを位置情報としてテキストを編集できるが、
 * このクラスにより任意の場所の編集が可能となる。クライアントは点を編集箇所に移動させ、
 * @c Viewer の操作と似た方法で編集を行う
 *
 * 編集点は他の編集操作でその位置が変更される。親クラスの @c Point を見よ
 *
 * 文字単位、単語単位で編集点の移動を行うメソッドのうち、名前が @c Left 及び @c Right
 * で終わっているものは、論理順ではなく、視覚上の方向を指定する。
 * 具体的にはビューのテキスト方向が左から右であれば @c xxxxLeft は @c xxxxPrev に、方向が右から左であれば
 * @c xxxxLeft は @c xxxxNext にマップされる。これら視覚上の方向をベースにしたメソッドは、
 * キーボードなどのユーザインターフェイスから移動を行うことを考えて提供されている
 *
 * EditPoint hides @c Point#excludeFromRestriction and can't enter the inaccessible region of the
 * document. @c #isExcludedFromRestriction always returns @c true.
 *
 * EditPoint throws @c ReadOnlyDocumentException when tried to change the read-only document.
 *
 * EditPoint <strong>never</strong> uses compound change of the document and freeze of the
 * viewer. Client is responsible for the usage of these features.
 *
 * @see Point, Document, IPointListener, DisposedDocumentException
 */

/**
 * Constructor.
 * @param document the document
 * @param position the initial position of the point
 * @param listener the listener. can be @c null if not needed
 * @throw BadPositionException @a position is outside of the document
 */
EditPoint::EditPoint(Document& document, const Position& position /* = Position() */, IPointListener* listener /* = 0 */)
		: Point(document, position), listener_(listener), characterUnit_(GRAPHEME_CLUSTER) {
	excludeFromRestriction(true);
}

/**
 * Copy-constructor.
 * @param rhs the source object
 * @throw DisposedDocumentException the document to which @a rhs belongs had been disposed
 */
EditPoint::EditPoint(const EditPoint& rhs) : Point(rhs), listener_(rhs.listener_), characterUnit_(rhs.characterUnit_) {
}

/// Destructor.
EditPoint::~EditPoint() /*throw()*/ {
}

/**
 * Returns the beginning of the previous bookmarked line.
 * @return the beginning of the backward bookmarked line or @c Position#INVALID_POSITION if there
 * is no bookmark in the document
 */
Position EditPoint::backwardBookmark(length_t marks /* = 1 */) const {
	const length_t line = document()->bookmarker().next(normalized().line, Direction::BACKWARD, marks);
	return (line != INVALID_INDEX) ? Position(line, 0) : Position::INVALID_POSITION;
}

/**
 * Returns the position returned by N characters.
 * @param characters the number of the characters to return. "what a character" is depends on the
 * character unit the point uses
 * @return the position of the previous character
 */
Position EditPoint::backwardCharacter(length_t characters /* = 1 */) const {
	return offsetCharacterPosition(*document(), normalized(), Direction::BACKWARD, characterUnit(), characters);
}

/**
 * Returns the position returned by N lines. If the destination position is outside of the
 * accessible region, returns the first line whose column is accessible, rather than the beginning
 * of the accessible region.
 * @param lines the number of the lines to return
 */
Position EditPoint::backwardLine(length_t lines /* = 1 */) const {
	Position p(normalized());
	const Position bob(document()->accessibleRegion().first);
	length_t line = (p.line > bob.line + lines) ? p.line - lines : bob.line;
	if(line == bob.line && p.column < bob.column)
		++line;
	return p.line = line, p;
}

/**
 * Returns the beginning of the backward N words.
 * @param words the number of words to traverse
 * @return the destination
 */
Position EditPoint::backwardWord(length_t words /* = 1 */) const {
	WordBreakIterator<DocumentCharacterIterator> i(DocumentCharacterIterator(*document(),
		document()->accessibleRegion(), normalized()), AbstractWordBreakIterator::START_OF_SEGMENT, identifierSyntax());
	return (i -= words).base().tell();
}

/**
 * Returns the the end of the backward N words.
 * @param words the number of words to traverse
 * @return the destination
 */
Position EditPoint::backwardWordEnd(length_t words /* = 1 */) const {
	WordBreakIterator<DocumentCharacterIterator> i(DocumentCharacterIterator(*document(),
		document()->accessibleRegion(), normalized()), AbstractWordBreakIterator::END_OF_SEGMENT, identifierSyntax());
	return (i -= words).base().tell();
}

/// Returns the beginning of the document.
Position EditPoint::beginningOfDocument() const {
	verifyDocument();
	return document()->accessibleRegion().first;
}

/// Returns the beginning of the current line.
Position EditPoint::beginningOfLine() const {
	return max(Position(normalized().line, 0), document()->accessibleRegion().first);
}

/**
 * Returns the code point of the current character.
 * @param useLineFeed true to return LF (U+000A) when the current position is end of the line. otherwise LS (U+2008)
 * @return the code point. if the current position is end of document, result is @c INVALID_CODE_POINT
 */
CodePoint EditPoint::character(bool useLineFeed /* = false */) const {
	verifyDocument();
	const String& line = document()->line(lineNumber());
	if(columnNumber() == line.length())
		return (lineNumber() == document()->numberOfLines() - 1) ? INVALID_CODE_POINT : (useLineFeed ? LINE_FEED : LINE_SEPARATOR);
	return surrogates::decodeFirst(line.begin() + columnNumber(), line.end());
}

/**
 * Deletes the current character and inserts the specified text.
 * @param first the start of the text
 * @param last the end of the text
 * @param keepNewline set false to overwrite a newline characer
 * @throw ReadOnlyDocumentException the document is read only
 * @throw NullPointerException @a first and/or @a last are @c null
 * @throw std#invalid_argument @a first &gt; @a last
 */
bool EditPoint::destructiveInsert(const Char* first, const Char* last, bool keepNewline /* = true */) {
	verifyDocument();
	if(document()->isReadOnly())
		throw ReadOnlyDocumentException();
	else if(first == 0)
		throw NullPointerException("first");
	else if(last == 0)
		throw NullPointerException("last");
	else if(first > last)
		throw invalid_argument("first > last");

	const bool adapts = adaptsToDocument();
	adaptToDocument(false);

	Position e((keepNewline && isEndOfLine()) ? position() : forwardCharacter());
	const bool interrupted = (e != position()) ? !document()->erase(Region(position(), e)) : false;
	if(!interrupted) {
		document()->insert(*this, first, last, &e);
		moveTo(e);
	}
	adaptToDocument(adapts);
	return !interrupted;
}

/**
 * Overrides @c Point#doMoveTo.
 * Derived class overrides this method should call from its @c doMoveTo method.
 */
void EditPoint::doMoveTo(const Position& to) {
	verifyDocument();
	if(to != position()) {
		const Position oldPosition(position());
		Point::doMoveTo(to);
		if(listener_ != 0)
			listener_->pointMoved(*this, oldPosition);
	}
}

/// Returns the end of the document.
Position EditPoint::endOfDocument() const {
	verifyDocument();
	return document()->accessibleRegion().end();
}

/// Returns the end of the current line.
Position EditPoint::endOfLine() const {
	const Position p(normalized());
	return min(Position(p.line, document()->lineLength(p.line)), document()->accessibleRegion().second);
}

/**
 * Erases the forward/backward character(s).
 * @param length the number of the forward characters to erase. if negative, the backward
 * characters is erased
 * @param cu indicates what a character is. if this is @c DEFAULT_UNIT, the current unit is used
 * @return false if the change was interrupted
 * @throw ReadOnlyDocumentException the document is read only
 * @throw UnknownValueException @a cu is invalid
 */
bool EditPoint::erase(signed_length_t length /* = 1 */, EditPoint::CharacterUnit cu /* = DEFAULT_UNIT */) {
	verifyDocument();
	if(document()->isReadOnly())
		throw ReadOnlyDocumentException();
	else if(length == 0)
		return true;	// does nothing
	return document()->erase(position(), offsetCharacterPosition(
		(length > 0) ? Direction::FORWARD : Direction::BACKWARD, (length > 0) ? length : -length, cu));
}

/**
 * Returns the beginning of the forward N bookmarked lines. If there is no bookmark in the
 * document, returns the current position.
 */
Position EditPoint::forwardBookmark(length_t marks /* = 1 */) const {
	const Position p(normalized());
	const length_t line = document()->bookmarker().next(p.line, Direction::FORWARD, marks);
	return (line != INVALID_INDEX) ? Position(line, 0) : p;
}

/**
 * Returns the position advanced by N characters.
 * @param characters the number of the characters to advance. "what a character is" depends on the
 * character unit the point uses
 */
Position EditPoint::forwardCharacter(length_t characters /* = 1 */) const {
	return offsetCharacterPosition(*document(), normalized(), Direction::FORWARD, characterUnit(), characters);
}

/**
 * Returns the position advanced by N lines. If the destination position is outside of the
 * inaccessible region, returns the last line whose column is accessible, rather than to the end of
 * the accessible region.
 * @param lines the number of the lines to advance
 */
Position EditPoint::forwardLine(length_t lines /* = 1 */) const {
	Position p(normalized());
	const Position eob(document()->accessibleRegion().second);
	length_t line = (p.line + lines < eob.line) ? p.line + lines : eob.line;
	if(line == eob.line && p.column > eob.column)
		--line;
	return p.line = line, p;
}

/**
 * Returns the beginning of the forward N words.
 * @param words the number of words to traverse
 */
Position EditPoint::forwardWord(length_t words /* = 1 */) const {
	WordBreakIterator<DocumentCharacterIterator> i(DocumentCharacterIterator(*document(),
		document()->accessibleRegion(), normalized()), AbstractWordBreakIterator::START_OF_SEGMENT, identifierSyntax());
	return (i += words).base().tell();
}

/**
 * Returns the end of the forward N words.
 * @param words the number of words to traverse
 */
Position EditPoint::forwardWordEnd(length_t words /* = 1 */) const {
	WordBreakIterator<DocumentCharacterIterator> i(DocumentCharacterIterator(*document(),
		document()->accessibleRegion(), normalized()), AbstractWordBreakIterator::END_OF_SEGMENT, identifierSyntax());
	return (i += words).base().tell();
}

String EditPoint::getText(signed_length_t length, Newline newline /* = NLF_RAW_VALUE */) const {
	return getText(offsetCharacterPosition(
		(length >= 0) ? Direction::FORWARD : Direction::BACKWARD, (length >= 0) ? length : -length), newline);
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
 * @return false if the change was interrupted
 * @throw ReadOnlyDocumentException the document is read only
 * @throw NullPointerException @a first and/or @a last are @c null
 * @throw std#invalid_argument @a first &gt; @a last
 * @see viewers#VisualPoint#insertRectangle
 */
bool EditPoint::insert(const Char* first, const Char* last) {
	if(first == 0)
		throw NullPointerException("first");
	else if(last == 0)
		throw NullPointerException("last");
	else if(first > last)
		throw invalid_argument("first > last");
	verifyDocument();
	if(document()->isReadOnly())
		throw ReadOnlyDocumentException();
	if(first == last)
		return true;
	const bool adapts = adaptsToDocument();
	adaptToDocument(false);
	Position p;
	const bool interrupted = !document()->insert(*this, first, last, &p);
	if(!interrupted)
		moveTo(p);
	adaptToDocument(adapts);
	return !interrupted;
}

/// Returns true if the point is the beginning of the document.
bool EditPoint::isBeginningOfDocument() const {
	verifyDocument();
	normalize();
	return position() == document()->accessibleRegion().first;
}

/// Returns true if the point is the beginning of the line.
bool EditPoint::isBeginningOfLine() const {
	verifyDocument();
	normalize();
	return columnNumber() == 0 || (document()->isNarrowed() && position() == document()->accessibleRegion().first);
}

/// Returns true if the point is the end of the document.
bool EditPoint::isEndOfDocument() const {
	verifyDocument();
	normalize();
	return position() == document()->accessibleRegion().second;
}

/// Returns true if the point is the end of the line.
bool EditPoint::isEndOfLine() const {
	verifyDocument();
	normalize();
	return columnNumber() == document()->lineLength(lineNumber()) || position() == document()->accessibleRegion().second;
}

#if 0
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
#endif

/**
 * Inserts the newline(s).
 * @param newlines how many times to insert a newline
 * @return false if the change was interrupted
 * @throw ReadOnlyDocumentException the document is read only
 * @note This method is hidden by @c VisualPoint#newLine (C++ rule).
 */
bool EditPoint::newLine(size_t newlines /* = 1 */) {
	verifyDocument();
	if(document()->isReadOnly())
		throw ReadOnlyDocumentException();
	else if(newlines == 0)
		return true;
	const IDocumentInput* const di = document()->input();
	String s(getNewlineString((di != 0) ? di->newline() : ASCENSION_DEFAULT_NEWLINE));
	if(newlines > 1) {
		basic_stringbuf<Char> b;
		for(size_t i = 0; i < newlines; ++i)
			b.sputn(s.data(), static_cast<streamsize>(s.length()));
		s = b.str();
	}
	return insert(s);
}

/**
 * Offsets the given position with the given character unit and returns.
 * @param document the document
 * @param position the origin
 * @param direction the direction to offset
 * @param cu the character unit
 * @param offset the amount to offset
 * @return the result position. this must be inside of the accessible region of the document
 * @throw UnknownValueException @a cu is invalid
 */
Position EditPoint::offsetCharacterPosition(const Document& document,
		const Position& position, Direction direction, EditPoint::CharacterUnit cu, length_t offset /* = 1 */) {
	if(offset == 0)
		return position;
	else if(cu == UTF16_CODE_UNIT) {
		if(direction == Direction::FORWARD) {
			const Position e(document.accessibleRegion().second);
			if(position >= e)
				return e;
			for(Position p(position); ; offset -= document.lineLength(p.line++) + 1, p.column = 0) {
				if(p.line == e.line)
					return min(Position(p.line, p.column + offset), e);
				else if(p.column + offset <= document.lineLength(p.line))
					return p.column += offset, p;
			}
		} else {
			const Position e(document.accessibleRegion().first);
			if(position <= e)
				return e;
			for(Position p(position); ; offset -= document.lineLength(p.line) + 1, p.column = document.lineLength(--p.line)) {
				if(p.line == e.line)
					return (p.column <= e.column + offset) ? e : (p.column -= offset, p);
				else if(p.column >= offset)
					return p.column -= offset, p;
			}
		}
	} else if(cu == UTF32_CODE_UNIT) {
		// TODO: there is more efficient implementation.
		DocumentCharacterIterator i(document, position);
		if(direction == Direction::FORWARD)
			while(offset-- > 0) i.next();
		else
			while(offset-- > 0) i.previous();
		return i.tell();
	} else if(cu == GRAPHEME_CLUSTER) {
		GraphemeBreakIterator<DocumentCharacterIterator> i(
			DocumentCharacterIterator(document, document.accessibleRegion(), position));
		i.next((direction == Direction::FORWARD) ? offset : -static_cast<signed_length_t>(offset));
		return i.base().tell();
	} else if(cu == GLYPH_CLUSTER) {
		// TODO: not implemented.
	}
	throw UnknownValueException("cu");
}

/**
 * Sets the new character unit.
 * @param unit the new character unit the point uses
 * @return this object
 * @throw UnknownValueException @a unit is invalid
 */
EditPoint& EditPoint::setCharacterUnit(EditPoint::CharacterUnit unit) {
	if(unit >= DEFAULT_UNIT)
		throw UnknownValueException("unit");
	characterUnit_ = unit;
	return *this;
}

/**
 * Transposes the two grapheme clusters on either side of the point.
 * If the point is not start of a cluster, this method fails.
 * If the transposing target is not in the current line, this method fails.
 * @return false if there is not a character to transpose or the change was interrupted
 * @throw ReadOnlyDocumentException the document is read only
 */
bool EditPoint::transposeCharacters() {
	verifyDocument();
	if(document()->isReadOnly())
		throw ReadOnlyDocumentException();

	// As transposing characters in string "ab":
	//
	//  a b -- transposing clusters 'a' and 'b'. result is "ba"
	// ^ ^ ^
	// | | next-cluster (named pos[2])
	// | middle-cluster (named pos[1]; usually current-position)
	// previous-cluster (named pos[0])

	Position pos[3];
	const Region region(document()->accessibleRegion());

	if(text::ucd::BinaryProperty::is<text::ucd::BinaryProperty::GRAPHEME_EXTEND>(character()))	// not the start of a grapheme
		return false;
	else if(!region.includes(position()))	// inaccessible
		return false;

	if(columnNumber() == 0 || position() == region.first) {
		GraphemeBreakIterator<DocumentCharacterIterator> i(DocumentCharacterIterator(*document(), pos[0] = position()));
		pos[1] = (++i).base().tell();
		if(pos[1].line != pos[0].line || pos[1] == pos[0] || !region.includes(pos[1]))
			return false;
		pos[2] = (++i).base().tell();
		if(pos[2].line != pos[1].line || pos[2] == pos[1] || !region.includes(pos[2]))
			return false;
	} else if(columnNumber() == document()->lineLength(lineNumber()) || position() == region.second) {
		GraphemeBreakIterator<DocumentCharacterIterator> i(DocumentCharacterIterator(*document(), pos[2] = position()));
		pos[1] = (--i).base().tell();
		if(pos[1].line != pos[2].line || pos[1] == pos[2] || !region.includes(pos[1]))
			return false;
		pos[0] = (--i).base().tell();
		if(pos[0].line != pos[1].line || pos[0] == pos[1] || !region.includes(pos[0]))
			return false;
	} else {
		GraphemeBreakIterator<DocumentCharacterIterator> i(DocumentCharacterIterator(*document(), pos[1] = position()));
		pos[2] = (++i).base().tell();
		if(pos[2].line != pos[1].line || pos[2] == pos[1] || !region.includes(pos[2]))
			return false;
		i.base().seek(pos[1]);
		pos[0] = (--i).base().tell();
		if(pos[0].line != pos[1].line || pos[0] == pos[1] || !region.includes(pos[0]))
			return false;
	}

	moveTo(lineNumber(), pos[1].column);
	String s(getText(pos[2]));
	moveTo(lineNumber(), pos[0].column);
	s += getText(pos[1]);
	if(!document()->erase(position(), pos[2]))	// TODO: guarentee the complete rollback.
		return false;
	return insert(s);
}

/**
 * Transposes the current line and the previous line.
 * If the current line is the first line, transposes with the next line.
 * The line breaks will not be exchanged.
 * @return false if there is not a character to transpose or the change was interrupted
 * @throw ReadOnlyDocumentException the document is read only
 */
bool EditPoint::transposeLines() {
	verifyDocument();
	if(document()->isReadOnly())
		throw ReadOnlyDocumentException();

	const Region region(document()->accessibleRegion());
	if(region.first.line == region.second.line)	// there is just one line
		return false;

	if(lineNumber() == region.first.line)
		moveTo(lineNumber() + 1, columnNumber());

	const String str1 = (lineNumber() - 1 == region.first.line) ?
		document()->line(lineNumber() - 1).substr(region.first.column) : document()->line(lineNumber() - 1);
	const String str2 = (lineNumber() == region.second.line) ?
		document()->line(lineNumber()).substr(0, region.second.column) : document()->line(lineNumber());

	// make the two lines empty
	if(!str2.empty()) {
		beginningOfLine();
		if(!erase(static_cast<signed_length_t>(str2.length()), UTF16_CODE_UNIT))
			return false;
	}
	if(!str1.empty()) {
		moveTo(lineNumber() - 1, (lineNumber() == region.first.line) ? region.first.column : 0);
		if(!erase(static_cast<signed_length_t>(str1.length()), UTF16_CODE_UNIT))
			return false;
		moveTo(lineNumber() + 1, columnNumber());
	}

	// insert into the two lines
	// TODO: guarentee the complete rollback.
	bool succeeded = true;
	if(!str1.empty()) {
		beginningOfLine();
		succeeded = insert(str1);
	}
	moveTo(lineNumber() - 1, columnNumber());
	if(!str2.empty()) {
		moveTo(lineNumber(), (lineNumber() == region.first.line) ? region.first.column : 0);
		succeeded = insert(str2);
	}
	moveTo(Position(lineNumber() + 2, 0));

	return true;
}

/**
 * Transposes the two words on either side of the point.
 * @return false if there is not a character to transpose or the change was interrupted
 * @throw ReadOnlyDocumentException the document is read only
 */
bool EditPoint::transposeWords() {
	verifyDocument();
	if(document()->isReadOnly())
		throw ReadOnlyDocumentException();

	// As transposing words in string "(\w+)[^\w*](\w+)":
	//
	//  abc += xyz -- transposing words "abc" and "xyz". result is "xyz+=abc"
	// ^   ^  ^   ^
	// |   |  |   2nd-word-end (named pos[3])
	// |   |  2nd-word-start (named pos[2])
	// |   1st-word-end (named pos[1])
	// 1st-word-start (named pos[0])

	const Region region(document()->accessibleRegion());
	WordBreakIterator<DocumentCharacterIterator> i(DocumentCharacterIterator(*document(),
		document()->accessibleRegion(), *this), AbstractWordBreakIterator::START_OF_ALPHANUMERICS, identifierSyntax());
	Position pos[4];

	// find the backward word (1st-word-*)...
	pos[0] = (--i).base().tell();
	i.setComponent(AbstractWordBreakIterator::END_OF_ALPHANUMERICS);
	pos[1] = (++i).base().tell();
	if(pos[1] == pos[0])	// the word is empty
		return false;

	// ...and then backward one (2nd-word-*)
	i.base().seek(*this);
	i.setComponent(AbstractWordBreakIterator::START_OF_ALPHANUMERICS);
	pos[2] = (++i).base().tell();
	if(pos[2] == position())
		return false;
	pos[3] = (++i).base().tell();
	if(pos[2] == pos[3])	// the word is empty
		return false;

	// replace
	moveTo(pos[2]);
	String s(getText(pos[3]));
	moveTo(pos[1]);
	s += getText(pos[2]);
	moveTo(pos[0]);
	s += getText(pos[1]);
	if(!document()->erase(position(), pos[3]))
		return false;
	return insert(s);	// TODO: guarentee the complete rollback.
}
