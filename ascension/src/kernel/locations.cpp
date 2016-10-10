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
				template<typename T>
				inline void throwIfOutsideOfDocument(const T& p) {
					if(positions::isOutsideOfDocumentRegion(document(p), position(p)))
						throw BadPositionException(position(p));
				}
			}

			/**
			 * Returns the beginning of the accessible region of the document.
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
			 * @throw BadPositionException @a p is outside of the document
			 */
			Position beginningOfLine(const PointProxy& p) {
				throwIfOutsideOfDocument(p);
				return positions::shrinkToAccessibleRegion(document(p), Position::bol(position(p)));
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
			 * @throw BadPositionException @a p is outside of the document
			 */
			Position endOfLine(const PointProxy& p) {
				throwIfOutsideOfDocument(p);
				const auto ln = line(position(p));
				return positions::shrinkToAccessibleRegion(document(p), Position(ln, document(p).lineLength(ln)));
			}

			/**
			 * Returns @c true if the given point is the beginning of the accessible region of the document.
			 * @param p The point to check
			 * @return true if @a is the beginning of the accessible region of the document
			 * @throw BadPositionException @a p is outside of the document
			 */
			bool isBeginningOfDocument(const PointProxy& p) {
				throwIfOutsideOfDocument(p);
				return position(p) == beginningOfDocument(p);
			}

			/**
			 * Returns @c true if the given point is the beginning of the line.
			 * @param p The point to check
			 * @return true if @a is the beginning of the line
			 * @throw BadPositionException @a p is outside of the document
			 */
			bool isBeginningOfLine(const PointProxy& p) {
				return position(p) == beginningOfLine(p);	// this may throw BadPositionException
			}

			/**
			 * Returns @c true if the given point is the end of the accessible region of the document.
			 * @param p The point to check
			 * @return true if @a is the end of the accessible region of the document
			 * @throw BadPositionException @a p is outside of the document
			 */
			bool isEndOfDocument(const PointProxy& p) {
				throwIfOutsideOfDocument(p);
				return position(p) == endOfDocument(p);
			}

			/**
			 * Returns @c true if the given point is the end of the line.
			 * @param p The point to check
			 * @return true if @a is the end of the line
			 * @throw BadPositionException @a p is outside of the document
			 */
			bool isEndOfLine(const PointProxy& p) {
				return position(p) == endOfLine(p);	// this may throw BadPositionException
			}

			/**
			 * Returns the beginning of the next bookmarked line.
			 * @param p The base point
			 * @param direction The direction
			 * @param marks The number of motions
			 * @return The beginning of the forward/backward bookmarked line
			 * @retval boost#none The found bookmark was outside of the accessible region of the document, or there is
			 *                    no bookmark in the document
			 * @throw BadPositionException @a p is outside of the document
			 */
			boost::optional<Position> nextBookmark(const PointProxy& p, Direction direction, Index marks /* = 1 */) {
				throwIfOutsideOfDocument(p);
				const auto temp(document(p).bookmarker().next(line(position(p)), direction, true, marks));
				if(temp != boost::none) {
					const auto bookmark = boost::get(temp);
					const auto accessibleRegion(document(p).accessibleRegion());
					if(bookmark == line(*boost::const_begin(accessibleRegion)))
						return boost::make_optional(*boost::const_begin(accessibleRegion));
					else if(bookmark > line(*boost::const_begin(accessibleRegion)) && bookmark <= line(*boost::const_end(accessibleRegion)))
						return boost::make_optional(Position::bol(bookmark));
				}
				return boost::none;
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
					if(direction == Direction::forward()) {
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
					if(direction == Direction::forward())
						while(offset-- > 0) ++i;	// TODO: Use std.advance instead.
					else
						while(offset-- > 0) --i;	// TODO: Use std.advance instead.
					return i.tell();
				} else if(characterUnit == locations::GRAPHEME_CLUSTER) {
					text::GraphemeBreakIterator<DocumentCharacterIterator> i(
						DocumentCharacterIterator(document(p), document(p).accessibleRegion(), position(p)));
					i.next((direction == Direction::forward()) ? offset : -static_cast<SignedIndex>(offset));
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
			 * @throw BadPositionException @a p is outside of the document
			 * @see viewer#locations#nextVisualLine
			 */
			Position nextLine(const PointProxy& p, Direction direction, Index lines /* = 1 */) {
				throwIfOutsideOfDocument(p);
				auto result(position(p));
				if(direction == Direction::forward()) {
					const auto eob(endOfDocument(p));
					result.line = (line(result) + lines < line(eob)) ? line(result) + lines : line(eob);
					if(line(result) == line(eob) && offsetInLine(result) > offsetInLine(eob))
						--result.line;
				} else {
					const auto bob(beginningOfDocument(p));
					result.line = (line(result) > line(bob) + lines) ? line(result) - lines : line(bob);
					if(line(result) == line(bob) && offsetInLine(result) < offsetInLine(bob))
						++result.line;
				}
				return result;
			}

			namespace {
				inline Position nextWord(const PointProxy& p, Direction direction, Index words, text::WordBreakIteratorBase::Component component) {
					text::WordBreakIterator<DocumentCharacterIterator> i(
						DocumentCharacterIterator(document(p), document(p).accessibleRegion(), position(p)),
						component, detail::identifierSyntax(p));	// this may throw BadPositionException
					if(direction == Direction::forward())
						i += words;
					else
						i -= words;
					return i.base().tell();
				}
			}

			/**
			 * Returns the beginning of the forward/backward N words.
			 * @param p The base point
			 * @param direction The direction
			 * @param words The number of words to traverse
			 * @return The destination
			 * @throw BadPositionException @a p is outside of the document
			 * @see viewer#locations#nextWordEnd, viewer#locations#nextWordInPhysicalDirection
			 */
			Position nextWord(const PointProxy& p, Direction direction, Index words /* = 1 */) {
				return nextWord(p, direction, words, text::WordBreakIteratorBase::START_OF_SEGMENT);
			}

			/**
			 * Returns the end of the forward/backward N words.
			 * @param p The base point
			 * @param direction The direction
			 * @param words The number of words to traverse
			 * @return The destination
			 * @throw BadPositionException @a p is outside of the document
			 * @see viewer#locations#nextWord, viewer#locations#nextWordEndInPhysicalDirection
			 */
			Position nextWordEnd(const PointProxy& p, Direction direction, Index words /* = 1 */) {
				return nextWord(p, direction, words, text::WordBreakIteratorBase::END_OF_SEGMENT);
			}
		}
	}
}
