/**
 * @file locations.cpp
 * Implements @c ascension#kernel#locations namespace.
 * @author exeal
 * @see viewer/locations.cpp
 * @date 2003-2014, 2016 Was point.cpp.
 * @date 2016-05-22 Separated from point.cpp.
 */

#include <ascension/corelib/numeric-range-algorithm/encompasses.hpp>
#include <ascension/corelib/text/character-property.hpp>	// text.ucd.BinaryProperty
#include <ascension/corelib/text/grapheme-break-iterator.hpp>
#include <ascension/corelib/text/word-break-iterator.hpp>
#include <ascension/kernel/bookmarker.hpp>
#include <ascension/kernel/document.hpp>
#include <ascension/kernel/document-character-iterator.hpp>
#include <ascension/kernel/locations.hpp>


namespace ascension {
	namespace kernel {
		namespace locations {
			namespace {
				template<typename T>
				inline const Document& document(const T& p) BOOST_NOEXCEPT {
					return std::get<0>(p);
				}
				template<typename T>
				inline const Position& position(const T& p) BOOST_NOEXCEPT {
					return std::get<1>(p);
				}
				template<typename T>
				inline Position shrinkToAccessibleRegion(const T& p) BOOST_NOEXCEPT {
					return positions::shrinkToAccessibleRegion(document(p), position(p));
				}
			}

#ifdef ASCENSION_ABANDONED_AT_VERSION_08
			/**
			 * Returns the beginning of the previous bookmarked line.
			 * @param p The base point
			 * @return The beginning of the backward bookmarked line or @c boost#none if there is no bookmark in the
			 *         document
			 */
			boost::optional<Position> backwardBookmark(const PointProxy& p, Index marks /* = 1 */) {
				const auto temp(document(p).bookmarker().next(line(shrinkToAccessibleRegion(p)), Direction::BACKWARD, true, marks));
				return (temp != boost::none) ? boost::make_optional(Position::bol(boost::get(temp))) : boost::none;
			}

			/**
			 * Returns the position returned by N characters.
			 * @param p The base point
			 * @param unit Defines what a character is
			 * @param characters The number of the characters to return
			 * @return The position of the previous character
			 */
			Position backwardCharacter(const PointProxy& p, locations::CharacterUnit unit, Index characters /* = 1 */) {
				return nextCharacter(document(p), position(p), Direction::BACKWARD, unit, characters);
			}

			/**
			 * Returns the position returned by N lines. If the destination position is outside of the accessible
			 * region, returns the first line whose offset is accessible, rather than the beginning of the accessible
			 * region.
			 * @param p The base point
			 * @param lines The number of the lines to return
			 * @return The position of the previous line
			 */
			Position backwardLine(const PointProxy& p, Index lines /* = 1 */) {
				Position temp(shrinkToAccessibleRegion(p));
				const Position bob(document(p).accessibleRegion().first);
				Index destination = (line(temp) > line(bob) + lines) ? temp.line - lines : line(bob);
				if(destination == line(bob) && offsetInLine(temp) < offsetInLine(bob))
					++destination;
				return temp.line = destination, temp;
			}

			/**
			 * Returns the beginning of the backward N words.
			 * @param p The base point
			 * @param words The number of words to traverse
			 * @return The destination
			 */
			Position backwardWord(const PointProxy& p, Index words /* = 1 */) {
				WordBreakIterator<DocumentCharacterIterator> i(
					DocumentCharacterIterator(document(p), document(p).accessibleRegion(), shrinkToAccessibleRegion(p)),
					AbstractWordBreakIterator::START_OF_SEGMENT, identifierSyntax(p));
				return (i -= words).base().tell();
			}

			/**
			 * Returns the the end of the backward N words.
			 * @param p The base point
			 * @param words The number of words to traverse
			 * @return The destination
			 */
			Position backwardWordEnd(const PointProxy& p, Index words /* = 1 */) {
				WordBreakIterator<DocumentCharacterIterator> i(
					DocumentCharacterIterator(document(p), document(p).accessibleRegion(), shrinkToAccessibleRegion(p)),
					AbstractWordBreakIterator::END_OF_SEGMENT, identifierSyntax(p));
				return (i -= words).base().tell();
			}
#endif // ASCENSION_ABANDONED_AT_VERSION_08

			/**
			 * Returns the beginning of the document.
			 * @param p The base point
			 * @return The destination
			 */
			Position beginningOfDocument(const PointProxy& p) {
				return *boost::const_begin(document(p).accessibleRegion());
			}

			/**
			 * Returns the beginning of the current line.
			 * @param p The base point
			 * @return The destination
			 */
			Position beginningOfLine(const PointProxy& p) {
				return Position::bol(shrinkToAccessibleRegion(p));
			}

			/**
			 * Returns the code point of the current character.
			 * @param p The base point
			 * @param useLineFeed Set @c true to return LF (U+000A) when the current position is the end of the line.
			 *                    Otherwise LS (U+2008)
			 * @return The code point of the character, or @c INVALID_CODE_POINT if @a p is the end of the document
			 */
			CodePoint characterAt(const PointProxy& p, bool useLineFeed /* = false */) {
				const String& lineString = document(p).lineString(line(position(p)));
				if(offsetInLine(position(p)) == lineString.length())
					return (line(position(p)) == document(p).numberOfLines() - 1) ? text::INVALID_CODE_POINT : (useLineFeed ? text::LINE_FEED : text::LINE_SEPARATOR);
				return text::utf::decodeFirst(std::begin(lineString) + offsetInLine(position(p)), std::end(lineString));
			}

			/**
			 * Returns the end of the document.
			 * @param p The base point
			 * @return The destination
			 */
			Position endOfDocument(const PointProxy& p) {
				return *boost::const_end(document(p).accessibleRegion());
			}

			/**
			 * Returns the end of the current line.
			 * @param p The base point
			 * @return The destination
			 */
			Position endOfLine(const PointProxy& p) {
				const Position temp(shrinkToAccessibleRegion(p));
				return std::min(Position(line(temp), document(p).lineLength(line(temp))), *boost::const_end(document(p).accessibleRegion()));
			}

#ifdef ASCENSION_ABANDONED_AT_VERSION_08
			/**
			 * Returns the beginning of the next bookmarked line.
			 * @param p The base point
			 * @return The beginning of the forward bookmarked line or @c boost#none if there is no bookmark in the
			 *         document
			 */
			boost::optional<Position> forwardBookmark(const PointProxy& p, Index marks /* = 1 */) {
				const auto temp(document(p).bookmarker().next(line(shrinkToAccessibleRegion(p)), Direction::FORWARD, true, marks));
				return (temp != boost::none) ? boost::make_optional(Position::bol(boost::get(temp))) : boost::none;
			}

			/**
			 * Returns the position advanced by N characters.
			 * @param p The base point
			 * @param unit Defines what a character is
			 * @param characters The number of the characters to advance
			 * @return The position of the next character
			 */
			Position forwardCharacter(const PointProxy& p, CharacterUnit unit, Index characters /* = 1 */) {
				return nextCharacter(document(p), position(p), Direction::FORWARD, unit, characters);
			}

			/**
			 * Returns the position advanced by N lines. If the destination position is outside of the inaccessible
			 * region, returns the last line whose offset is accessible, rather than the end of the accessible region.
			 * @param p The base point
			 * @param lines The number of the lines to advance
			 * @return The position of the next line
			 */
			Position forwardLine(const PointProxy& p, Index lines /* = 1 */) {
				Position temp(shrinkToAccessibleRegion(p));
				const Position eob(*boost::const_end(document(p).accessibleRegion()));
				Index destination = (line(temp) + lines < line(eob)) ? line(temp) + lines : line(eob);
				if(destination == line(eob) && offsetInLine(temp) > offsetInLine(eob))
					--destination;
				return temp.line = destination, temp;
			}

			/**
			 * Returns the beginning of the forward N words.
			 * @param p The base point
			 * @param words The number of words to traverse
			 * @return The destination
			 */
			Position forwardWord(const PointProxy& p, Index words /* = 1 */) {
				text::WordBreakIterator<DocumentCharacterIterator> i(
					DocumentCharacterIterator(document(p), document(p).accessibleRegion(), shrinkToAccessibleRegion(p)),
					text::AbstractWordBreakIterator::START_OF_SEGMENT, identifierSyntax(p));
				return (i += words).base().tell();
			}

			/**
			 * Returns the end of the forward N words.
			 * @param p The base point
			 * @param words The number of words to traverse
			 * @return The destination
			 */
			Position forwardWordEnd(const PointProxy& p, Index words /* = 1 */) {
				text::WordBreakIterator<DocumentCharacterIterator> i(
					DocumentCharacterIterator(document(p), document(p).accessibleRegion(), shrinkToAccessibleRegion(p)),
					text::AbstractWordBreakIterator::END_OF_SEGMENT, identifierSyntax(p));
				return (i += words).base().tell();
			}
#endif // ASCENSION_ABANDONED_AT_VERSION_08

			/// Returns @c true if the given point @a p is the beginning of the document.
			bool isBeginningOfDocument(const PointProxy& p) {
				return position(p) == *boost::const_begin(document(p).accessibleRegion());
			}

			/// Returns @c true if the given point @a p is the beginning of the line.
			bool isBeginningOfLine(const PointProxy& p) {
				return offsetInLine(position(p)) == 0
					|| (document(p).isNarrowed() && position(p) == *boost::const_begin(document(p).accessibleRegion()));
			}

			/// Returns @c true if the given point @a p is the end of the document.
			bool isEndOfDocument(const PointProxy& p) {
				return position(p) == *boost::const_end(document(p).accessibleRegion());
			}

			/// Returns @c true if the given point @a p is the end of the line.
			bool isEndOfLine(const PointProxy& p) {
				return offsetInLine(position(p)) == document(p).lineLength(line(position(p)))
					|| position(p) == *boost::const_end(document(p).accessibleRegion());
			}

			/**
			 * Returns the beginning of the next bookmarked line.
			 * @param p The base point
			 * @param direction The direction
			 * @param marks The number of motions
			 * @return The beginning of the forward/backward bookmarked line, or @c boost#none if there is no bookmark
			 *         in the document
			 */
			boost::optional<Position> nextBookmark(const PointProxy& p, Direction direction, Index marks /* = 1 */) {
				const auto temp(document(p).bookmarker().next(line(shrinkToAccessibleRegion(p)), direction, true, marks));
				return (temp != boost::none) ? boost::make_optional(Position::bol(boost::get(temp))) : boost::none;
			}

			/**
			 * Returns the position offset from the given point with the given character unit.
			 * This function considers the accessible region of the document.
			 * @param p The base point
			 * @param direction The direction to offset
			 * @param characterUnit The character unit
			 * @param offset The amount to offset
			 * @return The result position. This must be inside of the accessible region of the document
			 * @throw BadPositionException @a position is outside of the document
			 * @throw UnknownValueException @a characterUnit is invalid
			 */
			Position nextCharacter(const PointProxy& p, Direction direction, locations::CharacterUnit characterUnit, Index offset /* = 1 */) {
				if(offset == 0)
					return position(p);
				else if(characterUnit == UTF16_CODE_UNIT) {
					if(direction == Direction::FORWARD) {
						const Position e(*boost::const_end(document(p).accessibleRegion()));
						if(position(p) >= e)
							return e;
						for(Position q(position(p)); ; offset -= document(p).lineLength(q.line++) + 1, q.offsetInLine = 0) {
							if(line(q) == line(e))
								return std::min(Position(line(q), offsetInLine(q) + offset), e);
							else if(offsetInLine(q) + offset <= document(p).lineLength(line(q)))
								return q.offsetInLine += offset, q;
						}
					} else {
						const Position e(*boost::const_begin(document(p).accessibleRegion()));
						if(position(p) <= e)
							return e;
						for(Position q(position(p)); ; offset -= document(p).lineLength(line(q)) + 1, q.offsetInLine = document(p).lineLength(--q.line)) {
							if(line(q) == line(e))
								return (offsetInLine(q) <= offsetInLine(e) + offset) ? e : (q.offsetInLine -= offset, q);
							else if(offsetInLine(q) >= offset)
								return q.offsetInLine -= offset, q;
						}
					}
				} else if(characterUnit == UTF32_CODE_UNIT) {
					// TODO: there is more efficient implementation.
					DocumentCharacterIterator i(document(p), position(p));
					if(direction == Direction::FORWARD)
						while(offset-- > 0) ++i;	// TODO: Use std.advance instead.
					else
						while(offset-- > 0) --i;	// TODO: Use std.advance instead.
					return i.tell();
				} else if(characterUnit == locations::GRAPHEME_CLUSTER) {
					text::GraphemeBreakIterator<DocumentCharacterIterator> i(
						DocumentCharacterIterator(document(p), document(p).accessibleRegion(), position(p)));
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
			 * @see viewer#locations#nextVisualLine
			 */
			Position nextLine(const PointProxy& p, Direction direction, Index lines /* = 1 */) {
				Position result(shrinkToAccessibleRegion(p));
				if(direction == Direction::FORWARD) {
					const Position eob(*boost::const_end(document(p).accessibleRegion()));
					result.line = (line(result) + lines < line(eob)) ? line(result) + lines : line(eob);
					if(line(result) == line(eob) && offsetInLine(result) > offsetInLine(eob))
						--result.line;
				} else {
					const Position bob(*boost::const_begin(document(p).accessibleRegion()));
					result.line = (line(result) > line(bob) + lines) ? line(result) - lines : line(bob);
					if(line(result) == line(bob) && offsetInLine(result) < offsetInLine(bob))
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
			 * @see viewer#locations#nextWordEnd, viewer#locations#nextWordInPhysicalDirection
			 */
			Position nextWord(const PointProxy& p, Direction direction, Index words /* = 1 */) {
				text::WordBreakIterator<DocumentCharacterIterator> i(
					DocumentCharacterIterator(document(p), document(p).accessibleRegion(), shrinkToAccessibleRegion(p)),
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
			 * @param direction The direction
			 * @param words The number of words to traverse
			 * @return The destination
			 * @see viewer#locations#nextWord, viewer#locations#nextWordEndInPhysicalDirection
			 */
			Position nextWordEnd(const PointProxy& p, Direction direction, Index words /* = 1 */) {
				text::WordBreakIterator<DocumentCharacterIterator> i(
					DocumentCharacterIterator(document(p), document(p).accessibleRegion(), shrinkToAccessibleRegion(p)),
					text::WordBreakIteratorBase::END_OF_SEGMENT, detail::identifierSyntax(p));
				if(direction == Direction::FORWARD)
					i += words;
				else
					i -= words;
				return i.base().tell();
			}
		}
	}
}
