/**
 * @file point.cpp
 * @author exeal
 * @date 2003-2011
 */

#include <ascension/kernel/point.hpp>
#include <ascension/kernel/document-character-iterator.hpp>
#include <ascension/corelib/text/break-iterator.hpp>
#include <ascension/corelib/text/character-property.hpp>	// text.ucd.BinaryProperty
using namespace ascension;
using namespace ascension::kernel;
using namespace ascension::text;
using namespace std;


// DocumentDisposedException //////////////////////////////////////////////////////////////////////

/// Default constructor.
DocumentDisposedException::DocumentDisposedException() :
		IllegalStateException("The document the object connecting to has been already disposed.") {
}


// Point //////////////////////////////////////////////////////////////////////////////////////////

/**
 * @class ascension::kernel::Point
 *
 * A point represents a document position and adapts to the document change.
 *
 * When the document change occurred, @c Point moves automatically as follows:
 *
 * - If text was inserted or deleted before the point, the point will move accordingly.
 * - If text was inserted or deleted after the point, the point will not move.
 * - If region includes the point was deleted, the point will move to the beginning (= end) of
 *   the region.
 * - If text was inserted at the point, the point will or will not move according to the
 *   gravity.
 *
 * When the document was reset (by @c Document#resetContent), the all points move to the
 * beginning of the document.
 *
 * Almost all methods of this or derived classes will throw @c DocumentDisposedException if
 * the document is already disposed. Call @c #isDocumentDisposed to check if the document
 * is exist or not.
 *
 * @c Point is unaffected by narrowing and can moves outside of the accessible region.
 *
 * @see Position, Document, locations, viewers#VisualPoint, viewers#Caret
 */

/**
 * Constructor.
 * @param document The document to which the point attaches
 * @param position The initial position of the point
 * @param listener The listener. Can be @c null if not needed
 * @throw BadPositionException @a position is outside of the document
 */
Point::Point(Document& document, const Position& position /* = Position() */, PointListener* listener /* = 0 */) :
		document_(&document), position_(position), adapting_(true), gravity_(Direction::FORWARD), listener_(listener) {
	if(position != Position() && !document.region().includes(position))
		throw BadPositionException(position);
	static_cast<detail::PointCollection<Point>&>(document).addNewPoint(*this);
}

/**
 * Copy-constructor.
 * @param other The source object
 * @throw DocumentDisposedException The document to which @a other belongs had been disposed
 */
Point::Point(const Point& other) : document_(other.document_), position_(other.position_),
		adapting_(other.adapting_), gravity_(other.gravity_), listener_(other.listener_) {
	if(document_ == 0)
		throw DocumentDisposedException();
	static_cast<detail::PointCollection<Point>*>(document_)->addNewPoint(*this);
}

/// Destructor.
Point::~Point() /*throw()*/ {
	lifeCycleListeners_.notify(&PointLifeCycleListener::pointDestroyed);
	if(document_ != 0)
		static_cast<detail::PointCollection<Point>*>(document_)->removePoint(*this);
}

/**
 * Registers the lifecycle listener.
 * @param listener The listener to be registered
 * @throw std#invalid_argument @a listener is already registered
 */
void Point::addLifeCycleListener(PointLifeCycleListener& listener) {
	lifeCycleListeners_.add(listener);
}

/**
 * This overridable method is called by @c #moveTo to check and adjust the desitination position.
 * If you override this, consider the followings:
 *
 * - To change the destination, modify the value of @a parameter.
 * - Call @c #aboutToMove method of the super class with the same parameter.
 * - Throw any exceptions to interrupt the movement.
 *
 * @c Point#aboutToMove does nothing.
 * @param to The destination position. implementation can modify this value
 * @throw DocumentDisposedException the document to which the point belongs is already disposed
 * @see #moved, moveTo
 */
void Point::aboutToMove(Position& to) {
}

/**
 * This overridable method is called by @c #moveTo to notify the movement was finished.
 * If you override this, call @c #moved method of the super class with the same parameter. And
 * don't throw any exceptions. Note that this method is not called if @c #aboutToMove threw an
 * exception.
 * @c Point's implementation does nothing.
 * @param from The position before the point moved. This value may equal to the current position
 * @see #aboutToMove, moveTo
 */
void Point::moved(const Position& from) /*throw()*/ {
}

/**
 * Moves to the specified position.
 * While this method fails when @a to was outside of the document, whether it depends on the
 * derived class when @a to was outside of the accessible region. @c Point succeeds in the latter
 * case. For other classes, see the documentations of the classes.
 * @param to The destination position
 * @throw BadPositionException @a to is outside of the document
 * @throw ... Any exceptions @c #aboutToMove implementation of sub-classe throws
 */
void Point::moveTo(const Position& to) {
	if(isDocumentDisposed())
		throw DocumentDisposedException();
	else if(to != Position() && to > document().region().end())
		throw BadPositionException(to);
//	if(to != position()) {
		Position destination(to);
		aboutToMove(destination);
		destination = positions::shrinkToDocumentRegion(document(), destination);
//		if(destination != position()) {
			const Position from(position());
			position_ = destination;
			moved(from);
			if(listener_ != 0 && destination != from)
				listener_->pointMoved(*this, from);
//		}
//	}
}

/**
 * Removes the lifecycle listener
 * @param listener The listener to be removed
 * @throw std#invalid_argument @a listener is not registered
 */
void Point::removeLifeCycleListener(PointLifeCycleListener& listener) {
	lifeCycleListeners_.remove(listener);
}

/**
 * Sets the gravity.
 * @param gravity The new gravity value
 * @return This object
 */
Point& Point::setGravity(Direction gravity) /*throw()*/ {
	if(isDocumentDisposed())
		throw DocumentDisposedException();
	gravity_ = gravity;
	return *this;
}

/**
 * Called when the document was changed.
 * @param change The content of the document change
 */
void Point::update(const DocumentChange& change) {
	if(document_ == 0 || !adaptsToDocument())
		return;

//	normalize();
	const Position newPosition(positions::updatePosition(position(), change, gravity()));
	if(newPosition != position())
		moveTo(newPosition);	// TODO: this may throw...
}


// kernel.locations free functions ////////////////////////////////////////////////////////////////

/**
 * @namespace ascension::kernel::locations
 *
 * Provides several functions related to locations in document.
 *
 * Functions this namespace defines are categorized into the following three:
 *
 * - Functions take a position and return other position (ex. @c forwardCharacter). These functions
 *   take a @c Point or @c VisualPoint as the first parameter excepting @c nextCharacter.
 * - Functions check if the given position is specific location (ex. isBeginningOfLine). These
 *   functions take a @c Point or @c VisualPoint as the first parameter.
 * - @c characterAt.
 *
 * Some of the above functions return @c VerticalDestinationProxy objects and these can be passed
 * to @c VisualPoint#moveTo method.
 *
 * All functions are unaffected by accessible region of the document.
 */

namespace {
	/// @internal Returns the @c IdentifierSyntax object corresponds to the given point.
	inline const IdentifierSyntax& identifierSyntax(const Point& p) {
		return p.document().contentTypeInformation().getIdentifierSyntax(p.contentType());
	}
} // namespace @0

/**
 * Returns the beginning of the previous bookmarked line.
 * @param p The base point
 * @return The beginning of the backward bookmarked line or @c Position#INVALID_POSITION if there
 *         is no bookmark in the document
 */
Position locations::backwardBookmark(const Point& p, length_t marks /* = 1 */) {
	const length_t line = p.document().bookmarker().next(p.normalized().line, Direction::BACKWARD, true, marks);
	return (line != INVALID_INDEX) ? Position(line, 0) : Position();
}

/**
 * Returns the position returned by N characters.
 * @param p The base point
 * @param unit Defines what a character is
 * @param characters The number of the characters to return
 * @return The position of the previous character
 */
Position locations::backwardCharacter(const Point& p, locations::CharacterUnit unit, length_t characters /* = 1 */) {
	return nextCharacter(p.document(), p.position(), Direction::BACKWARD, unit, characters);
}

/**
 * Returns the position returned by N lines. If the destination position is outside of the
 * accessible region, returns the first line whose column is accessible, rather than the beginning
 * of the accessible region.
 * @param p The base point
 * @param lines The number of the lines to return
 * @return The position of the previous line
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
 * @param p The base point
 * @param words The number of words to traverse
 * @return The destination
 */
Position locations::backwardWord(const Point& p, length_t words /* = 1 */) {
	WordBreakIterator<DocumentCharacterIterator> i(
		DocumentCharacterIterator(p.document(), p.document().accessibleRegion(), p.normalized()),
		AbstractWordBreakIterator::START_OF_SEGMENT, identifierSyntax(p));
	return (i -= words).base().tell();
}

/**
 * Returns the the end of the backward N words.
 * @param p The base point
 * @param words The number of words to traverse
 * @return The destination
 */
Position locations::backwardWordEnd(const Point& p, length_t words /* = 1 */) {
	WordBreakIterator<DocumentCharacterIterator> i(
		DocumentCharacterIterator(p.document(), p.document().accessibleRegion(), p.normalized()),
		AbstractWordBreakIterator::END_OF_SEGMENT, identifierSyntax(p));
	return (i -= words).base().tell();
}

/**
 * Returns the beginning of the document.
 * @param p The base point
 * @return The destination
 */
Position locations::beginningOfDocument(const Point& p) {
	return p.document().accessibleRegion().first;
}

/**
 * Returns the beginning of the current line.
 * @param p The base point
 * @return The destination
 */
Position locations::beginningOfLine(const Point& p) {
	return max(Position(p.normalized().line, 0), p.document().accessibleRegion().first);
}

/**
 * Returns the code point of the current character.
 * @param p The base point
 * @param useLineFeed Set @c true to return LF (U+000A) when the current position is the end of the
 *                    line. Otherwise LS (U+2008)
 * @return The code point of the character, or @c INVALID_CODE_POINT if @a p is the end of the
 *         document
 */
CodePoint locations::characterAt(const Point& p, bool useLineFeed /* = false */) {
	const String& line = p.document().line(p.line());
	if(p.column() == line.length())
		return (p.line() == p.document().numberOfLines() - 1) ? INVALID_CODE_POINT : (useLineFeed ? LINE_FEED : LINE_SEPARATOR);
	return utf16::decodeFirst(line.begin() + p.column(), line.end());
}

/**
 * Returns the end of the document.
 * @param p The base point
 * @return The destination
 */
Position locations::endOfDocument(const Point& p) {
	return p.document().accessibleRegion().end();
}

/**
 * Returns the end of the current line.
 * @param p The base point
 * @return The destination
 */
Position locations::endOfLine(const Point& p) {
	const Position temp(p.normalized());
	return min(Position(temp.line, p.document().lineLength(temp.line)), p.document().accessibleRegion().second);
}

/**
 * Returns the beginning of the next bookmarked line.
 * @param p The base point
 * @return The beginning of the forward bookmarked line or @c Position#INVALID_POSITION if there
 *         is no bookmark in the document
 */
Position locations::forwardBookmark(const Point& p, length_t marks /* = 1 */) {
	const length_t line = p.document().bookmarker().next(p.normalized().line, Direction::FORWARD, true, marks);
	return (line != INVALID_INDEX) ? Position(line, 0) : Position();
}

/**
 * Returns the position advanced by N characters.
 * @param p The base point
 * @param unit Defines what a character is
 * @param characters The number of the characters to advance
 * @return The position of the next character
 */
Position locations::forwardCharacter(const Point& p, locations::CharacterUnit unit, length_t characters /* = 1 */) {
	return nextCharacter(p.document(), p.position(), Direction::FORWARD, unit, characters);
}

/**
 * Returns the position advanced by N lines. If the destination position is outside of the
 * inaccessible region, returns the last line whose column is accessible, rather than the end of
 * the accessible region.
 * @param p The base point
 * @param lines The number of the lines to advance
 * @return The position of the next line
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
 * @param p The base point
 * @param words The number of words to traverse
 * @return The destination
 */
Position locations::forwardWord(const Point& p, length_t words /* = 1 */) {
	WordBreakIterator<DocumentCharacterIterator> i(
		DocumentCharacterIterator(p.document(), p.document().accessibleRegion(), p.normalized()),
		AbstractWordBreakIterator::START_OF_SEGMENT, identifierSyntax(p));
	return (i += words).base().tell();
}

/**
 * Returns the end of the forward N words.
 * @param p The base point
 * @param words The number of words to traverse
 * @return The destination
 */
Position locations::forwardWordEnd(const Point& p, length_t words /* = 1 */) {
	WordBreakIterator<DocumentCharacterIterator> i(
		DocumentCharacterIterator(p.document(), p.document().accessibleRegion(), p.normalized()),
		AbstractWordBreakIterator::END_OF_SEGMENT, identifierSyntax(p));
	return (i += words).base().tell();
}

/// Returns @c true if the given point @a p is the beginning of the document.
bool locations::isBeginningOfDocument(const Point& p) {
	return p.position() == p.document().accessibleRegion().first;
}

/// Returns @c true if the given point @a p is the beginning of the line.
bool locations::isBeginningOfLine(const Point& p) {
	return p.column() == 0 || (p.document().isNarrowed() && p.position() == p.document().accessibleRegion().first);
}

/// Returns @c true if the given point @a p is the end of the document.
bool locations::isEndOfDocument(const Point& p) {
	return p.position() == p.document().accessibleRegion().second;
}

/// Returns @c true if the given point @a p is the end of the line.
bool locations::isEndOfLine(const Point& p) {
	return p.column() == p.document().lineLength(p.line()) || p.position() == p.document().accessibleRegion().second;
}

/**
 * Returns the position offset from the given point with the given character unit.
 * This function considers the accessible region of the document.
 * @param document The document
 * @param position The base position
 * @param direction The direction to offset
 * @param characterUnit The character unit
 * @param offset The amount to offset
 * @return The result position. This must be inside of the accessible region of the document
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
 * @param offset The offset from the start of the document
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
