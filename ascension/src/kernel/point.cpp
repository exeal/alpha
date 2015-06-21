/**
 * @file point.cpp
 * @author exeal
 * @date 2003-2014
 */

#include <ascension/kernel/document.hpp>
#include <ascension/kernel/document-character-iterator.hpp>
#include <ascension/kernel/point.hpp>
#include <ascension/corelib/text/break-iterator.hpp>
#include <ascension/corelib/text/character-property.hpp>	// text.ucd.BinaryProperty


namespace ascension {
	namespace kernel {
		// DocumentDisposedException //////////////////////////////////////////////////////////////////////////////////

		/// Default constructor.
		DocumentDisposedException::DocumentDisposedException() :
				IllegalStateException("The document the object connecting to has been already disposed.") {
		}


		// Point //////////////////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * @class ascension::kernel::Point
		 *
		 * A point represents a document position and adapts to the document change.
		 *
		 * When the document change occurred, @c Point moves automatically as follows:
		 *
		 * - If text was inserted or deleted before the point, the point will move accordingly.
		 * - If text was inserted or deleted after the point, the point will not move.
		 * - If region includes the point was deleted, the point will move to the beginning (= end) of the region.
		 * - If text was inserted at the point, the point will or will not move according to the gravity.
		 *
		 * When the document was reset (by @c Document#resetContent), the all points move to the beginning of the
		 * document.
		 *
		 * Almost all methods of this or derived classes will throw @c DocumentDisposedException if the document is
		 * already disposed. Call @c #isDocumentDisposed to check if the document is exist or not.
		 *
		 * @c Point is unaffected by narrowing and can moves outside of the accessible region.
		 *
		 * @see Position, Document, locations, viewer#VisualPoint, viewer#Caret
		 */

		/**
		 * @typedef ascension::kernel::Point::DestructionSignal
		 * The signal which gets emitted when the point was destructed.
		 * @param point A pointer to the destructed point. Don't access the point by this pointer
		 * @see Point#destructionSignal, MotionSignal
		 */

		/**
		 * @typedef ascension::kernel::Point::MotionSignal
		 * The signal which gets emitted when the point was moved.
		 * @param self The point
		 * @param oldPosition The position from which the point moved
		 * @see Point#motionSignal, DestructionSignal
		 */

		/**
		 * Constructor.
		 * @param document The document to which the point attaches
		 * @param position The initial position of the point
		 * @throw BadPositionException @a position is outside of the document
		 */
		Point::Point(Document& document, const Position& position) :
				document_(&document), position_(position), adapting_(true), gravity_(Direction::FORWARD) {
			if(!document.region().includes(position))
				throw BadPositionException(position);
			static_cast<detail::PointCollection<Point>&>(document).addNewPoint(*this);
		}

		/**
		 * Copy-constructor.
		 * @param other The source object
		 * @throw DocumentDisposedException The document to which @a other belongs had been disposed
		 */
		Point::Point(const Point& other) : document_(other.document_), position_(other.position_),
				adapting_(other.adapting_), gravity_(other.gravity_) {
			if(document_ == nullptr)
				throw DocumentDisposedException();
			static_cast<detail::PointCollection<Point>*>(document_)->addNewPoint(*this);
		}

		/// Destructor.
		Point::~Point() BOOST_NOEXCEPT {
			destructionSignal_(this);
			if(document_ != nullptr)
				static_cast<detail::PointCollection<Point>*>(document_)->removePoint(*this);
		}

		/**
		 * This overridable method is called by @c #moveTo to check and adjust the desitination position.
		 * If you override this, consider the followings:
		 *
		 * - To change the destination, modify the value of @a parameter.
		 * - Call @c #aboutToMove method of the super class with the same parameter.
		 * - Throw any exceptions to interrupt the movement.
		 *
		 * Note that @c #moveToNowhere method does not call this method.
		 *
		 * @c Point#aboutToMove does nothing.
		 * @param to The destination position. implementation can modify this value
		 * @throw DocumentDisposedException the document to which the point belongs is already disposed
		 * @see #moved, #moveTo, #moveToNowhere
		 */
		void Point::aboutToMove(Position& to) {
		}

		/// Returns the @c DestructionSignal signal connector.
		SignalConnector<Point::DestructionSignal> Point::destructionSignal() BOOST_NOEXCEPT {
			return makeSignalConnector(destructionSignal_);
		}

		/// Returns the @c MotionSignal signal connector.
		SignalConnector<Point::MotionSignal> Point::motionSignal() BOOST_NOEXCEPT {
			return makeSignalConnector(motionSignal_);
		}

		/**
		 * This overridable method is called by @c #moveTo to notify the movement was finished.
		 * If you override this, call @c #moved method of the super class with the same parameter. And don't throw any
		 . exceptions. Note that this method is not called if @c #aboutToMove threw an exception.
		 *
		 * @c Point's implementation does nothing.
		 * @param from The position before the point moved. This value may equal to the current position
		 * @see #aboutToMove, moveTo
		 */
		void Point::moved(const Position& from) BOOST_NOEXCEPT {
		}

		/**
		 * Moves to the specified position.
		 * While this method fails when @a to was outside of the document, whether it depends on the derived class when
		 * @a to was outside of the accessible region. @c Point succeeds in the latter case. For other classes, see the
		 * documentations of the classes.
		 * @param to The destination position
		 * @throw BadPositionException @a to is outside of the document
		 * @throw ... Any exceptions @c #aboutToMove implementation of sub-classe throws
		 * @return This point
		 */
		Point& Point::moveTo(const Position& to) {
			if(isDocumentDisposed())
				throw DocumentDisposedException();
			else if(to > document().region().end())
				throw BadPositionException(to);
			Position destination(to);
			aboutToMove(destination);
			destination = positions::shrinkToDocumentRegion(document(), destination);
			const Position from(position());
			position_ = destination;
			moved(from);
			if(destination != from)
				motionSignal_(*this, from);
			return *this;
		}

		/// Returns the normalized position of the point.
		Position Point::normalized() const {
			return positions::shrinkToDocumentRegion(document(), position());
		}

		/**
		 * Sets the gravity.
		 * @param gravity The new gravity value
		 * @return This object
		 */
		Point& Point::setGravity(Direction gravity) BOOST_NOEXCEPT {
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
			if(document_ == nullptr || !adaptsToDocument())
				return;
//			normalize();
			const Position newPosition(positions::updatePosition(position(), change, gravity()));
			if(newPosition != position())
				moveTo(newPosition);	// TODO: this may throw...
		}


		// free functions /////////////////////////////////////////////////////////////////////////////////////////////

		/// Returns the content type of the document partition contains the point.
		ContentType contentType(const Point& p) {
			return p.document().partitioner().contentType(p);
		}

		/**
		 * @namespace ascension::kernel::locations
		 *
		 * Provides several functions related to locations in document.
		 *
		 * Functions this namespace defines are categorized into the following three:
		 *
		 * - Functions take a position and return other position (ex. @c nextCharacter). These functions take a
		 *   @c Position, @c Point or @c VisualPoint as the base position.
		 * - Functions check if the given position is specific location (ex. isBeginningOfLine). These functions take a
		 *   @c Point or @c VisualPoint as the first parameter.
		 * - @c characterAt.
		 *
		 * Some of the above functions return @c BlockProgressionDestinationProxy objects and these can be passed to
		 * @c VisualPoint#moveTo and @c Caret#extendSelectionTo methods.
		 *
		 * All functions are unaffected by accessible region of the document.
		 */
		namespace locations {

#ifdef ASCENSION_ABANDONED_AT_VERSION_08
			/**
			 * Returns the beginning of the previous bookmarked line.
			 * @param p The base point
			 * @return The beginning of the backward bookmarked line or @c boost#none if there is no bookmark in the
			 *         document
			 */
			boost::optional<Position> backwardBookmark(const Point& p, Index marks /* = 1 */) {
				const boost::optional<Index> line(p.document().bookmarker().next(p.normalized().line, Direction::BACKWARD, true, marks));
				return (line != boost::none) ? boost::make_optional(Position(*line, 0)) : boost::none;
			}

			/**
			 * Returns the position returned by N characters.
			 * @param p The base point
			 * @param unit Defines what a character is
			 * @param characters The number of the characters to return
			 * @return The position of the previous character
			 */
			Position backwardCharacter(const Point& p, locations::CharacterUnit unit, Index characters /* = 1 */) {
				return nextCharacter(p.document(), p.position(), Direction::BACKWARD, unit, characters);
			}

			/**
			 * Returns the position returned by N lines. If the destination position is outside of the accessible
			 * region, returns the first line whose offset is accessible, rather than the beginning of the accessible
			 * region.
			 * @param p The base point
			 * @param lines The number of the lines to return
			 * @return The position of the previous line
			 */
			Position backwardLine(const Point& p, Index lines /* = 1 */) {
				Position temp(p.normalized());
				const Position bob(p.document().accessibleRegion().first);
				Index line = (temp.line > bob.line + lines) ? temp.line - lines : bob.line;
				if(line == bob.line && temp.offsetInLine < bob.offsetInLine)
					++line;
				return temp.line = line, temp;
			}

			/**
			 * Returns the beginning of the backward N words.
			 * @param p The base point
			 * @param words The number of words to traverse
			 * @return The destination
			 */
			Position backwardWord(const Point& p, Index words /* = 1 */) {
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
			Position backwardWordEnd(const Point& p, Index words /* = 1 */) {
				WordBreakIterator<DocumentCharacterIterator> i(
					DocumentCharacterIterator(p.document(), p.document().accessibleRegion(), p.normalized()),
					AbstractWordBreakIterator::END_OF_SEGMENT, identifierSyntax(p));
				return (i -= words).base().tell();
			}
#endif // ASCENSION_ABANDONED_AT_VERSION_08

			/**
			 * Returns the beginning of the document.
			 * @param p The base point
			 * @return The destination
			 */
			Position beginningOfDocument(const Point& p) {
				return p.document().accessibleRegion().first;
			}

			/**
			 * Returns the beginning of the current line.
			 * @param p The base point
			 * @return The destination
			 */
			Position beginningOfLine(const Point& p) {
				return std::max(Position(p.normalized().line, 0), p.document().accessibleRegion().first);
			}

			/**
			 * Returns the code point of the current character.
			 * @param p The base point
			 * @param useLineFeed Set @c true to return LF (U+000A) when the current position is the end of the line.
			 *                    Otherwise LS (U+2008)
			 * @return The code point of the character, or @c INVALID_CODE_POINT if @a p is the end of the document
			 */
			CodePoint characterAt(const Point& p, bool useLineFeed /* = false */) {
				const String& lineString = p.document().line(line(p));
				if(offsetInLine(p) == lineString.length())
					return (line(p) == p.document().numberOfLines() - 1) ? text::INVALID_CODE_POINT : (useLineFeed ? text::LINE_FEED : text::LINE_SEPARATOR);
				return text::utf::decodeFirst(std::begin(lineString) + offsetInLine(p), std::end(lineString));
			}

			/**
			 * Returns the end of the document.
			 * @param p The base point
			 * @return The destination
			 */
			Position endOfDocument(const Point& p) {
				return p.document().accessibleRegion().end();
			}

			/**
			 * Returns the end of the current line.
			 * @param p The base point
			 * @return The destination
			 */
			Position endOfLine(const Point& p) {
				const Position temp(p.normalized());
				return std::min(Position(temp.line, p.document().lineLength(temp.line)), p.document().accessibleRegion().second);
			}

#ifdef ASCENSION_ABANDONED_AT_VERSION_08
			/**
			 * Returns the beginning of the next bookmarked line.
			 * @param p The base point
			 * @return The beginning of the forward bookmarked line or @c boost#none if there is no bookmark in the
			 *         document
			 */
			boost::optional<Position> forwardBookmark(const Point& p, Index marks /* = 1 */) {
				const boost::optional<Index> line(p.document().bookmarker().next(p.normalized().line, Direction::FORWARD, true, marks));
				return (line != boost::none) ? boost::make_optional(Position(*line, 0)) : boost::none;
			}

			/**
			 * Returns the position advanced by N characters.
			 * @param p The base point
			 * @param unit Defines what a character is
			 * @param characters The number of the characters to advance
			 * @return The position of the next character
			 */
			Position forwardCharacter(const Point& p, CharacterUnit unit, Index characters /* = 1 */) {
				return nextCharacter(p.document(), p.position(), Direction::FORWARD, unit, characters);
			}

			/**
			 * Returns the position advanced by N lines. If the destination position is outside of the inaccessible
			 * region, returns the last line whose offset is accessible, rather than the end of the accessible region.
			 * @param p The base point
			 * @param lines The number of the lines to advance
			 * @return The position of the next line
			 */
			Position forwardLine(const Point& p, Index lines /* = 1 */) {
				Position temp(p.normalized());
				const Position eob(p.document().accessibleRegion().second);
				Index line = (temp.line + lines < eob.line) ? temp.line + lines : eob.line;
				if(line == eob.line && temp.offsetInLine > eob.offsetInLine)
					--line;
				return temp.line = line, temp;
			}

			/**
			 * Returns the beginning of the forward N words.
			 * @param p The base point
			 * @param words The number of words to traverse
			 * @return The destination
			 */
			Position forwardWord(const Point& p, Index words /* = 1 */) {
				text::WordBreakIterator<DocumentCharacterIterator> i(
					DocumentCharacterIterator(p.document(), p.document().accessibleRegion(), p.normalized()),
					text::AbstractWordBreakIterator::START_OF_SEGMENT, identifierSyntax(p));
				return (i += words).base().tell();
			}

			/**
			 * Returns the end of the forward N words.
			 * @param p The base point
			 * @param words The number of words to traverse
			 * @return The destination
			 */
			Position forwardWordEnd(const Point& p, Index words /* = 1 */) {
				text::WordBreakIterator<DocumentCharacterIterator> i(
					DocumentCharacterIterator(p.document(), p.document().accessibleRegion(), p.normalized()),
					text::AbstractWordBreakIterator::END_OF_SEGMENT, identifierSyntax(p));
				return (i += words).base().tell();
			}
#endif // ASCENSION_ABANDONED_AT_VERSION_08

			/// Returns @c true if the given point @a p is the beginning of the document.
			bool isBeginningOfDocument(const Point& p) {
				return p.position() == p.document().accessibleRegion().first;
			}

			/// Returns @c true if the given point @a p is the beginning of the line.
			bool isBeginningOfLine(const Point& p) {
				return offsetInLine(p) == 0
					|| (p.document().isNarrowed() && p.position() == p.document().accessibleRegion().first);
			}

			/// Returns @c true if the given point @a p is the end of the document.
			bool isEndOfDocument(const Point& p) {
				return p.position() == p.document().accessibleRegion().second;
			}

			/// Returns @c true if the given point @a p is the end of the line.
			bool isEndOfLine(const Point& p) {
				return offsetInLine(p) == p.document().lineLength(line(p))
					|| p.position() == p.document().accessibleRegion().second;
			}

			/**
			 * Returns the beginning of the next bookmarked line.
			 * @param p The base point
			 * @param direction The direction
			 * @return The beginning of the forward/backward bookmarked line, or @c boost#none if there is no bookmark
			 *         in the document
			 * @see #nextBookmarkInPhysicalDirection
			 */
			boost::optional<Position> nextBookmark(const Point& p, Direction direction, Index marks /* = 1 */) {
				const boost::optional<Index> line(p.document().bookmarker().next(p.normalized().line, direction, true, marks));
				return (line != boost::none) ? boost::make_optional(Position(*line, 0)) : boost::none;
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
			 * @see #nextCharacterInPhysicalDirection
			 */
			Position nextCharacter(const Document& document, const Position& position,
					Direction direction, locations::CharacterUnit characterUnit, Index offset /* = 1 */) {
				if(offset == 0)
					return position;
				else if(characterUnit == UTF16_CODE_UNIT) {
					if(direction == Direction::FORWARD) {
						const Position e(document.accessibleRegion().second);
						if(position >= e)
							return e;
						for(Position p(position); ; offset -= document.lineLength(p.line++) + 1, p.offsetInLine = 0) {
							if(p.line == e.line)
								return std::min(Position(p.line, p.offsetInLine + offset), e);
							else if(p.offsetInLine + offset <= document.lineLength(p.line))
								return p.offsetInLine += offset, p;
						}
					} else {
						const Position e(document.accessibleRegion().first);
						if(position <= e)
							return e;
						for(Position p(position); ; offset -= document.lineLength(p.line) + 1, p.offsetInLine = document.lineLength(--p.line)) {
							if(p.line == e.line)
								return (p.offsetInLine <= e.offsetInLine + offset) ? e : (p.offsetInLine -= offset, p);
							else if(p.offsetInLine >= offset)
								return p.offsetInLine -= offset, p;
						}
					}
				} else if(characterUnit == UTF32_CODE_UNIT) {
					// TODO: there is more efficient implementation.
					DocumentCharacterIterator i(document, position);
					if(direction == Direction::FORWARD)
						while(offset-- > 0) ++i;	// TODO: Use std.advance instead.
					else
						while(offset-- > 0) --i;	// TODO: Use std.advance instead.
					return i.tell();
				} else if(characterUnit == locations::GRAPHEME_CLUSTER) {
					text::GraphemeBreakIterator<DocumentCharacterIterator> i(
						DocumentCharacterIterator(document, document.accessibleRegion(), position));
					i.next((direction == Direction::FORWARD) ? offset : -static_cast<SignedIndex>(offset));
					return i.base().tell();
				} else if(characterUnit == locations::GLYPH_CLUSTER) {
					// TODO: not implemented.
				}
				throw UnknownValueException("characterUnit");
			}

			/**
			 * Returns the position advanced/returned by N lines. If the destination position is outside of the
			 * inaccessible region, returns the last/first line whose offset is accessible, rather than the
			 * end/beginning of the accessible region.
			 * @param p The base point
			 * @param direction The direction
			 * @param lines The number of the lines to advance/return
			 * @return The position of the next/previous line
			 * @see #nextVisualLine
			 */
			Position nextLine(const Point& p, Direction direction, Index lines /* = 1 */) {
				Position result(p.normalized());
				if(direction == Direction::FORWARD) {
					const Position eob(p.document().accessibleRegion().end());
					result.line = (result.line + lines < eob.line) ? result.line + lines : eob.line;
					if(result.line == eob.line && result.offsetInLine > eob.offsetInLine)
						--result.line;
				} else {
					const Position bob(p.document().accessibleRegion().beginning());
					result.line = (result.line > bob.line + lines) ? result.line - lines : bob.line;
					if(result.line == bob.line && result.offsetInLine < bob.offsetInLine)
						++result.line;
				}
				return result;
			}

			/**
			 * Returns the beginning of the forward/backward N words.
			 * @param p The base point
			 * @param direction The direction
			 * @param words The number of words to traverse
			 * @return The destination
			 * @see #nextWordEnd, #nextWordInPhysicalDirection
			 */
			Position nextWord(const Point& p, Direction direction, Index words /* = 1 */) {
				text::WordBreakIterator<DocumentCharacterIterator> i(
					DocumentCharacterIterator(p.document(), p.document().accessibleRegion(), p.normalized()),
					text::WordBreakIteratorBase::START_OF_SEGMENT, detail::identifierSyntax(p));
				if(direction == Direction::FORWARD)
					i += words;
				else
					i -= words;
				return i.base().tell();
			}

			/**
			 * Returns the end of the forward/backward N words.
			 * @param p The base point
			 * @param words The number of words to traverse
			 * @return The destination
			 * @see #nextWord, #nextWordEndInPhysicalDirection
			 */
			Position nextWordEnd(const Point& p, Direction direction, Index words /* = 1 */) {
				text::WordBreakIterator<DocumentCharacterIterator> i(
					DocumentCharacterIterator(p.document(), p.document().accessibleRegion(), p.normalized()),
					text::WordBreakIteratorBase::END_OF_SEGMENT, detail::identifierSyntax(p));
				if(direction == Direction::FORWARD)
					i += words;
				else
					i -= words;
				return i.base().tell();
			}
		}
#if 0
		/**
		 * Moves to the specified offset.
		 * @param offset The offset from the start of the document
		 * @deprecated 0.8
		 */
		void EditPoint::moveToAbsoluteCharacterOffset(Index offset) {
			verifyDocument();
		
			Index readCount = 0;
			const Region region(document()->region());
		
			if(document()->lineLength(region.first.line) + 1 - region.first.offsetInLine >= offset) {
				moveTo(Position(region.first.line, region.first.offsetInLine + offset));
				return;
			}
			readCount += document()->lineLength(region.first.line) + 1 - region.first.offsetInLine;
			for(Index line = region.first.line + 1; line <= region.second.line; ++line) {
				const Index lineLength = document()->lineLength(line) + 1;	// +1 is for a newline
				if(readCount + lineLength >= offset) {
					moveTo(Position(line, readCount + lineLength - offset));
					return;
				}
				readCount += lineLength;
			}
			moveTo(Position(region.second.line, document()->lineLength(region.second.line)));
		}
#endif
	}
}
