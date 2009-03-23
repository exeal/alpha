/**
 * @file point.cpp
 * @author exeal
 * @date 2003-2009
 */

#include <ascension/point.hpp>
#include <ascension/session.hpp>
#include <ascension/unicode-property.hpp>	// text.ucd.BinaryProperty
using namespace ascension;
using namespace ascension::kernel;
using namespace ascension::text;
using namespace std;


namespace {
	inline String regionText(const Document& document, const Position& first, const Position& second, Newline newline) {
		basic_ostringstream<Char> out;
		writeDocumentToStream(out, document, Region(first, second), newline);
		return out.str();
	}
} // namespace @0


// DocumentDisposedException ////////////////////////////////////////////////

/// Default constructor.
DocumentDisposedException::DocumentDisposedException() :
		IllegalStateException("The document the object connecting to has been already disposed.") {
}


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
 * @param listener the listener. can be @c null if not needed
 * @throw BadPositionException @a position is outside of the document
 */
Point::Point(Document& document, const Position& position /* = Position() */, IPointListener* listener /* = 0 */) :
		document_(&document), position_(position), adapting_(true), excludedFromRestriction_(false), gravity_(Direction::FORWARD), listener_(listener) {
	if(!document.region().includes(position))
		throw BadPositionException(position);
	static_cast<internal::IPointCollection<Point>&>(document).addNewPoint(*this);
}

/**
 * Copy-constructor.
 * @param rhs the source object
 * @throw DocumentDisposedException the document to which @a rhs belongs had been disposed
 */
Point::Point(const Point& rhs) :
		document_(rhs.document_), position_(rhs.position_), adapting_(rhs.adapting_),
		excludedFromRestriction_(rhs.excludedFromRestriction_), gravity_(rhs.gravity_), listener_(rhs.listener_) {
	if(document_ == 0)
		throw DocumentDisposedException();
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
	assert(!isDocumentDisposed());
	if(position_ != to) {
		const Position old(position_);
		position_ = to;
		normalize();
		if(listener_ != 0)
			listener_->pointMoved(*this, old);
	}
}

/// 
Point& Point::excludeFromRestriction(bool exclude) {
	if(isDocumentDisposed())
		throw DocumentDisposedException();
	if(excludedFromRestriction_ = exclude)
		normalize();
	return *this;
}

/**
 * Moves to the specified position.
 * Even if @a to is outside of the accessible region, this method will successes. In this case, the
 * destination position is the beginning or the end of the accessible region.
 * @param to the destination position
 * @throw BadPositionException @a to is outside of the document
 */
void Point::moveTo(const Position& to) {
	if(to > document().region().end())
		throw BadPositionException(to);
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
 * Sets the gravity.
 * @param gravity the new gravity value
 * @return this object
 */
Point& Point::setGravity(Direction gravity) /*throw()*/ {
	if(isDocumentDisposed())
		throw DocumentDisposedException();
	gravity_ = gravity;
	return *this;
}

/**
 * Called when the document was changed.
 * @param change the content of the document change
 */
void Point::update(const DocumentChange& change) {
	if(document_ == 0 || !adapting_)
		return;

//	normalize();
	const Position newPosition = positions::updatePosition(position_, change, gravity_);
	if(newPosition == position_)
		return;
	doMoveTo(newPosition);
}


// kernel.locations free functions //////////////////////////////////////////

/**
 * @namespace ascension#kernel#locations
 */

namespace {
	/// @internal Returns the @c IdentifierSyntax object corresponds to the given point.
	inline const IdentifierSyntax& identifierSyntax(const Point& p) {
		return p.document().contentTypeInformation().getIdentifierSyntax(p.contentType());
	}
} // namespace @0

/**
 * Returns the beginning of the previous bookmarked line.
 * @param p the base point
 * @return the beginning of the backward bookmarked line or @c Position#INVALID_POSITION if there
 *         is no bookmark in the document
 */
Position locations::backwardBookmark(const Point& p, length_t marks /* = 1 */) {
	const length_t line = p.document().bookmarker().next(p.normalized().line, Direction::BACKWARD, true, marks);
	return (line != INVALID_INDEX) ? Position(line, 0) : Position::INVALID_POSITION;
}

/**
 * Returns the position returned by N characters.
 * @param p the base point
 * @param unit defines what a character is
 * @param characters the number of the characters to return
 * @return the position of the previous character
 */
Position locations::backwardCharacter(const Point& p, locations::CharacterUnit unit, length_t characters /* = 1 */) {
	return nextCharacter(p.document(), p.position(), Direction::BACKWARD, unit, characters);
}

/**
 * Returns the position returned by N lines. If the destination position is outside of the
 * accessible region, returns the first line whose column is accessible, rather than the beginning
 * of the accessible region.
 * @param p the base point
 * @param lines the number of the lines to return
 * @return the position of the previous line
 */
Position locations::backwardLine(const Point& p, length_t lines /* = 1 */) {
	Position temp(p.normalized());
	const Position bob(p.document().accessibleRegion().first);
	length_t line = (temp.line > bob.line + lines) ? temp.line - lines : bob.line;
	if(line == bob.line && temp.column < bob.column)
		++line;
	return temp.line = line, temp;
}

/**
 * Returns the beginning of the backward N words.
 * @param p the base point
 * @param words the number of words to traverse
 * @return the destination
 */
Position locations::backwardWord(const Point& p, length_t words /* = 1 */) {
	WordBreakIterator<DocumentCharacterIterator> i(
		DocumentCharacterIterator(p.document(), p.document().accessibleRegion(), p.normalized()),
		AbstractWordBreakIterator::START_OF_SEGMENT, identifierSyntax(p));
	return (i -= words).base().tell();
}

/**
 * Returns the the end of the backward N words.
 * @param p the base point
 * @param words the number of words to traverse
 * @return the destination
 */
Position locations::backwardWordEnd(const Point& p, length_t words /* = 1 */) {
	WordBreakIterator<DocumentCharacterIterator> i(
		DocumentCharacterIterator(p.document(), p.document().accessibleRegion(), p.normalized()),
		AbstractWordBreakIterator::END_OF_SEGMENT, identifierSyntax(p));
	return (i -= words).base().tell();
}

/**
 * Returns the beginning of the document.
 * @param p the base point
 * @return the destination
 */
Position locations::beginningOfDocument(const Point& p) {
	return p.document().accessibleRegion().first;
}

/**
 * Returns the beginning of the current line.
 * @param p the base point
 * @return the destination
 */
Position locations::beginningOfLine(const Point& p) {
	return max(Position(p.normalized().line, 0), p.document().accessibleRegion().first);
}

/**
 * Returns the code point of the current character.
 * @param p the base point
 * @param useLineFeed true to return LF (U+000A) when the current position is end of the line. otherwise LS (U+2008)
 * @return the code point. if the current position is end of document, result is @c INVALID_CODE_POINT
 */
CodePoint locations::characterAt(const Point& p, bool useLineFeed /* = false */) {
	const String& line = p.document().line(p.line());
	if(p.column() == line.length())
		return (p.line() == p.document().numberOfLines() - 1) ? INVALID_CODE_POINT : (useLineFeed ? LINE_FEED : LINE_SEPARATOR);
	return surrogates::decodeFirst(line.begin() + p.column(), line.end());
}

/**
 * Returns the end of the document.
 * @param p the base point
 * @return the destination
 */
Position locations::endOfDocument(const Point& p) {
	return p.document().accessibleRegion().end();
}

/**
 * Returns the end of the current line.
 * @param p the base point
 * @return the destination
 */
Position locations::endOfLine(const Point& p) {
	const Position temp(p.normalized());
	return min(Position(temp.line, p.document().lineLength(temp.line)), p.document().accessibleRegion().second);
}

/**
 * Returns the beginning of the next bookmarked line.
 * @param p the base point
 * @return the beginning of the forward bookmarked line or @c Position#INVALID_POSITION if there
 *         is no bookmark in the document
 */
Position locations::forwardBookmark(const Point& p, length_t marks /* = 1 */) {
	const length_t line = p.document().bookmarker().next(p.normalized().line, Direction::FORWARD, true, marks);
	return (line != INVALID_INDEX) ? Position(line, 0) : Position::INVALID_POSITION;
}

/**
 * Returns the position advanced by N characters.
 * @param p the base point
 * @param unit defines what a character is
 * @param characters the number of the characters to advance
 * @return the position of the next character
 */
Position locations::forwardCharacter(const Point& p, locations::CharacterUnit unit, length_t characters /* = 1 */) {
	return nextCharacter(p.document(), p.position(), Direction::FORWARD, unit, characters);
}

/**
 * Returns the position advanced by N lines. If the destination position is outside of the
 * inaccessible region, returns the last line whose column is accessible, rather than the end of
 * the accessible region.
 * @param p the base point
 * @param lines the number of the lines to advance
 * @return the position of the next line
 */
Position locations::forwardLine(const Point& p, length_t lines /* = 1 */) {
	Position temp(p.normalized());
	const Position eob(p.document().accessibleRegion().second);
	length_t line = (temp.line + lines < eob.line) ? temp.line + lines : eob.line;
	if(line == eob.line && temp.column > eob.column)
		--line;
	return temp.line = line, temp;
}

/**
 * Returns the beginning of the forward N words.
 * @param p the base point
 * @param words the number of words to traverse
 * @return the destination
 */
Position locations::forwardWord(const Point& p, length_t words /* = 1 */) {
	WordBreakIterator<DocumentCharacterIterator> i(
		DocumentCharacterIterator(p.document(), p.document().accessibleRegion(), p.normalized()),
		AbstractWordBreakIterator::START_OF_SEGMENT, identifierSyntax(p));
	return (i += words).base().tell();
}

/**
 * Returns the end of the forward N words.
 * @param p the base point
 * @param words the number of words to traverse
 * @return the destination
 */
Position locations::forwardWordEnd(const Point& p, length_t words /* = 1 */) {
	WordBreakIterator<DocumentCharacterIterator> i(
		DocumentCharacterIterator(p.document(), p.document().accessibleRegion(), p.normalized()),
		AbstractWordBreakIterator::END_OF_SEGMENT, identifierSyntax(p));
	return (i += words).base().tell();
}

/// Returns true if the given point @a p is the beginning of the document.
bool locations::isBeginningOfDocument(const Point& p) {
	return p.position() == p.document().accessibleRegion().first;
}

/// Returns true if the given point @a p is the beginning of the line.
bool locations::isBeginningOfLine(const Point& p) {
	return p.column() == 0 || (p.document().isNarrowed() && p.position() == p.document().accessibleRegion().first);
}

/// Returns true if the given point @a p is the end of the document.
bool locations::isEndOfDocument(const Point& p) {
	return p.position() == p.document().accessibleRegion().second;
}

/// Returns true if the given point @a p is the end of the line.
bool locations::isEndOfLine(const Point& p) {
	return p.column() == p.document().lineLength(p.line()) || p.position() == p.document().accessibleRegion().second;
}

/**
 * Returns the position offset from the given point with the given character unit.
 * This function considers the accessible region of the document.
 * @param document the document
 * @param position the base position
 * @param direction the direction to offset
 * @param characterUnit the character unit
 * @param offset the amount to offset
 * @return the result position. this must be inside of the accessible region of the document
 * @throw BadPositionException @a position is outside of the document
 * @throw UnknownValueException @a characterUnit is invalid
 */
Position locations::nextCharacter(const Document& document, const Position& position,
		Direction direction, locations::CharacterUnit characterUnit, length_t offset /* = 1 */) {
	if(offset == 0)
		return position;
	else if(characterUnit == locations::UTF16_CODE_UNIT) {
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
	} else if(characterUnit == locations::UTF32_CODE_UNIT) {
		// TODO: there is more efficient implementation.
		DocumentCharacterIterator i(document, position);
		if(direction == Direction::FORWARD)
			while(offset-- > 0) i.next();
		else
			while(offset-- > 0) i.previous();
		return i.tell();
	} else if(characterUnit == locations::GRAPHEME_CLUSTER) {
		GraphemeBreakIterator<DocumentCharacterIterator> i(
			DocumentCharacterIterator(document, document.accessibleRegion(), position));
		i.next((direction == Direction::FORWARD) ? offset : -static_cast<signed_length_t>(offset));
		return i.base().tell();
	} else if(characterUnit == locations::GLYPH_CLUSTER) {
		// TODO: not implemented.
	}
	throw UnknownValueException("characterUnit");
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
