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
 * EditPoint <strong>never</strong> uses sequential edit of the document and freeze of the
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
EditPoint::~EditPoint() throw() {
}

/**
 * Moves to the beginning of the previous bookmarked line. If there is no bookmark in the document,
 * this method does nothing.
 * @return false if the bookmark is not found
 */
void EditPoint::backwardBookmark(length_t marks /* = 1 */) {
	verifyDocument();
	const length_t line = document()->bookmarker().next(lineNumber(), BACKWARD, marks);
	if(line != INVALID_INDEX)
		moveTo(Position(line, 0));
}

/**
 * Moves to the previous (backward) character.
 * @param characters the number of the characters to move. what is a character depends on the
 * character unit the point uses
 */
void EditPoint::backwardCharacter(length_t characters /* = 1 */) {
	verifyDocument();
	normalize();
	moveTo(offsetCharacterPosition(*document(), *this, BACKWARD, characterUnit(), characters));
}

/**
 * Moves to the previous line. If the destination position is outside of the inaccessible region,
 * moves to the last line whose column is accessible, rather than to the beginning of the
 * accessible region.
 * @param lines the number of the lines to move
 */
void EditPoint::backwardLine(length_t lines /* = 1 */) {
	verifyDocument();
	normalize();
	const length_t newLine = (lineNumber() > lines) ?
		max(lineNumber() - lines,
			(isExcludedFromRestriction() ? document()->accessibleRegion() : document()->region()).first.line) : 0;
	if(newLine != lineNumber())
		moveTo(Position(newLine, columnNumber()));
}

/**
 * Moves to the beginning of the previous word.
 * @param offset the number of words
 */
void EditPoint::backwardWord(length_t offset /* = 1 */) {
	verifyDocument();
	normalize();
	WordBreakIterator<DocumentCharacterIterator> i(
		DocumentCharacterIterator(*document(), *this), AbstractWordBreakIterator::START_OF_SEGMENT, identifierSyntax());
	i -= offset;
	moveTo(i.base().tell());
}

/**
 * Moves to the end of the previous word.
 * @param offset the number of words
 */
void EditPoint::backwardWordEnd(length_t offset /* = 1 */) {
	verifyDocument();
	normalize();
	WordBreakIterator<DocumentCharacterIterator> i(
		DocumentCharacterIterator(*document(), *this), AbstractWordBreakIterator::END_OF_SEGMENT, identifierSyntax());
	i -= offset;
	moveTo(i.base().tell());
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
 */
void EditPoint::destructiveInsert(const Char* first, const Char* last) {
	verifyDocument();
	if(document()->isReadOnly())
		return;
	Position next(offsetCharacterPosition(*document(), *this, FORWARD, characterUnit()));
	const bool adapts = adaptsToDocument();
	adaptToDocument(false);
	if(document()->erase(Region(*this, next))) {	// TODO: guarantee complete rollback.
		if(document()->insert(*this, first, last, &next))
			moveTo(next);
	}
	adaptToDocument(adapts);
}

/**
 * Overrides @c Point#doMoveTo.
 * Derived class overrides this method should call from its @c doMoveTo method.
 */
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
	moveTo(isExcludedFromRestriction() ? document()->accessibleRegion().end() : document()->region().end());
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
	erase(offsetCharacterPosition(*document(), *this, (length > 0) ? FORWARD : BACKWARD, cu, (length > 0) ? length : -length));
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
 * Moves to the beginning of the next bookmarked line. If there is no bookmark in the document,
 * this method does nothing.
 */
void EditPoint::forwardBookmark(length_t marks /* = 1 */) {
	verifyDocument();
	const length_t line = document()->bookmarker().next(lineNumber(), FORWARD, marks);
	if(line != INVALID_INDEX)
		moveTo(Position(line, 0));
}

/**
 * Moves to the next (forward) character.
 * @param characters the number of the characters to move. what is a character depends on the
 * character unit the point uses
 */
void EditPoint::forwardCharacter(length_t characters /* = 1 */) {
	verifyDocument();
	normalize();
	moveTo(offsetCharacterPosition(*document(), *this, FORWARD, characterUnit(), characters));
}

/**
 * Moves to the next line. If the destination position is outside of the inaccessible region, moves
 * to the last line whose column is accessible, rather than to the end of the accessible region.
 * @param lines the number of the lines to move
 */
void EditPoint::forwardLine(length_t lines /* = 1 */) {
	verifyDocument();
	normalize();
	const length_t newLine = min(lineNumber() + lines,
		(isExcludedFromRestriction() ? document()->accessibleRegion() : document()->region()).second.line);
	if(newLine != lineNumber())
		moveTo(Position(newLine, columnNumber()));
}

/**
 * Moves to the beginning of the next word.
 * @param offset the number of words
 */
void EditPoint::forwardWord(length_t offset /* = 1 */) {
	verifyDocument();
	normalize();
	WordBreakIterator<DocumentCharacterIterator> i(
		DocumentCharacterIterator(*document(), *this), AbstractWordBreakIterator::START_OF_SEGMENT, identifierSyntax());
	i += offset;
	moveTo(i.base().tell());
}

/**
 * Moves to the end of the next word.
 * @param offset the number of words
 */
void EditPoint::forwardWordEnd(length_t offset /* = 1 */) {
	verifyDocument();
	normalize();
	WordBreakIterator<DocumentCharacterIterator> i(
		DocumentCharacterIterator(*document(), *this), AbstractWordBreakIterator::END_OF_SEGMENT, identifierSyntax());
	i += offset;
	moveTo(i.base().tell());
}

String EditPoint::getText(signed_length_t length, Newline newline /* = NLF_RAW_VALUE */) const {
	return getText(offsetCharacterPosition(*document(), *this,
		(length >= 0) ? FORWARD : BACKWARD, characterUnit(), (length >= 0) ? length : -length), newline);
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
 * @throw NullPointerException @a first or @a last is @c null
 * @throw std#invalid_argument @a first &gt; @a last
 * @see viewers#VisualPoint#insertRectangle
 */
void EditPoint::insert(const Char* first, const Char* last) {
	if(first == 0)
		throw NullPointerException("first");
	else if(last == 0)
		throw NullPointerException("last");
	else if(first > last)
		throw invalid_argument("first > last");
	verifyDocument();
	if(document()->isReadOnly() || first == last)
		return;
	const bool adapts = adaptsToDocument();
	adaptToDocument(false);
	Position p;
	if(document()->insert(*this, first, last, &p))
		moveTo(p);
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
 * Inserts the newline(s).
 * @param newlines how many times to insert a newline
 * @note This method is hidden by @c VisualPoint#newLine (C++ rule).
 */
void EditPoint::newLine(size_t newlines /* = 1 */) {
	verifyDocument();
	if(document()->isReadOnly() || newlines == 0)
		return;
	const IDocumentInput* const di = document()->input();
	String s(getNewlineString((di != 0) ? di->newline() : ASCENSION_DEFAULT_NEWLINE));
	if(newlines > 1) {
		basic_stringbuf<Char> b;
		for(size_t i = 0; i < newlines; ++i)
			b.sputn(s.data(), static_cast<streamsize>(s.length()));
		s = b.str();
	}
	insert(s);
}

/**
 * Offsets the given position with the given character unit and returns.
 * This method does not consider the inaccessible region of the document.
 * @param document the document
 * @param position the origin
 * @param direction the direction to offset
 * @param cu the character unit
 * @param offset the amount to offset
 * @return the result position
 * @throw UnknownValueException @a cu is invalid
 */
Position EditPoint::offsetCharacterPosition(const Document& document,
		const Position& position, Direction direction, EditPoint::CharacterUnit cu, length_t offset /* = 1 */) {
	if(offset == 0)
		return position;
	else if(cu == UTF16_CODE_UNIT) {
		UTF32To16Iterator<DocumentCharacterIterator> i(DocumentCharacterIterator(document, position));
		if(direction == FORWARD)
			while(offset-- > 0) ++i;
		else
			while(offset-- > 0) --i;
		return i.tell().tell();
	} else if(cu == UTF32_CODE_UNIT) {
		DocumentCharacterIterator i(document, position);
		if(direction == FORWARD)
			while(offset-- > 0) i.next();
		else
			while(offset-- > 0) i.previous();
		return i.tell();
	} else if(cu == GRAPHEME_CLUSTER) {
		GraphemeBreakIterator<DocumentCharacterIterator> i(DocumentCharacterIterator(document, position));
		i.next((direction == FORWARD) ? offset : -static_cast<signed_length_t>(offset));
		return i.base().tell();
	} else if(cu == GLYPH_CLUSTER) {
		// TODO: not implemented.
	}
	throw UnknownValueException("cu");
}

/**
 * Transposes the two grapheme clusters on either side of the point.
 * If the point is not start of a cluster, this method fails.
 * If the transposing target is not in the current line, this method fails.
 * @return false if failed
 */
bool EditPoint::transposeCharacters() {
	verifyDocument();
	if(document()->isReadOnly())
		return false;

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
	String s = getText(pos[2]);
	moveTo(lineNumber(), pos[0].column);
	s += getText(pos[1]);
	erase(pos[2]);
	insert(s);

	return true;

#undef IS_RESTRICTION
}

/**
 * Transposes the current line and the previous line.
 * If the current line is the first line, transposes with the next line.
 * The line breaks will not be exchanged.
 * @return false if failed
 */
bool EditPoint::transposeLines() {
	verifyDocument();

	if(document()->isReadOnly())
		return false;

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
		erase(static_cast<signed_length_t>(str2.length()), UTF16_CODE_UNIT);
	}
	if(!str1.empty()) {
		moveTo(lineNumber() - 1, (lineNumber() == region.first.line) ? region.first.column : 0);
		erase(static_cast<signed_length_t>(str1.length()), UTF16_CODE_UNIT);
		moveTo(lineNumber() + 1, columnNumber());
	}

	// insert into the two lines
	if(!str1.empty()) {
		beginningOfLine();
		insert(str1);
	}
	moveTo(lineNumber() - 1, columnNumber());
	if(!str2.empty()) {
		moveTo(lineNumber(), (lineNumber() == region.first.line) ? region.first.column : 0);
		insert(str2);
	}
	moveTo(Position(lineNumber() + 2, 0));

	return true;
}

/**
 * Transposes the two words on either side of the point.
 * @return false if failed
 */
bool EditPoint::transposeWords() {
	verifyDocument();

	if(document()->isReadOnly())
		return false;

	// As transposing words in string "(\w+)[^\w*](\w+)":
	//
	//  abc += xyz -- transposing words "abc" and "xyz". result is "xyz+=abc"
	// ^   ^  ^   ^
	// |   |  |   2nd-word-end (named pos[3])
	// |   |  2nd-word-start (named pos[2])
	// |   1st-word-end (named pos[1])
	// 1st-word-start (named pos[0])

	const Region region(document()->accessibleRegion());
	WordBreakIterator<DocumentCharacterIterator> i(
		DocumentCharacterIterator(*document(), *this), AbstractWordBreakIterator::START_OF_ALPHANUMERICS, identifierSyntax());
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
	String str = getText(pos[3]);
	moveTo(pos[1]);
	str += getText(pos[2]);
	moveTo(pos[0]);
	str += getText(pos[1]);
	erase(pos[3]);
	insert(str);

	return true;
}
