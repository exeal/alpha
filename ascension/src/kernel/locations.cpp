/**
 * @file locations.cpp
 * Implements @c ascension#kernel#locations namespace.
 * @author exeal
 * @see viewer/locations.cpp
 * @date 2003-2014, 2016 Was point.cpp.
 * @date 2016-05-22 Separated from point.cpp.
 */

#include <ascension/corelib/numeric-range-algorithm/clamp.hpp>
#include <ascension/corelib/text/character-property.hpp>	// text.ucd.BinaryProperty
#include <ascension/corelib/text/grapheme-break-iterator.hpp>
#include <ascension/corelib/text/word-break-iterator.hpp>
#include <ascension/kernel/bookmarker.hpp>
#include <ascension/kernel/document.hpp>
#include <ascension/kernel/document-character-iterator.hpp>
#include <ascension/kernel/locations.hpp>
#include <ascension/kernel/point-proxy.hpp>
#include <boost/core/ignore_unused.hpp>


namespace ascension {
	namespace kernel {
		namespace locations {
			namespace {
				template<typename T>
				inline void throwIfOutsideOfDocument(const T& p) {
					if(isOutsideOfDocumentRegion(p))
						throw BadPositionException(position(p));
				}
			}

			/**
			 * Returns absolute character offset of the specified position from the start of the document.
			 * @param document The document
			 * @param p The position
			 * @param fromAccessibleStart
			 * @throw BadPositionException @a p is outside of the document
			 * @throw DocumentAccessViolationException @a fromAccessibleStart is @c true and @a p is before the
			 *                                         accessible region of the document
			 */
			Index absoluteOffset(const PointProxy& p, bool fromAccessibleStart) {
				if(position(p) > *boost::const_end(document(p).region()))
					throw BadPositionException(position(p));
				else if(fromAccessibleStart && position(p) < *boost::const_begin(document(p).accessibleRegion()))
					throw DocumentAccessViolationException();
				Index offset = 0;
				const Position start(*boost::const_begin(fromAccessibleStart ? document(p).accessibleRegion() : document(p).region()));
				for(Index i = line(start); ; ++i) {
					if(i == line(p)) {
						offset += offsetInLine(p);
						break;
					} else {
						offset += document(p).lineLength(i) + 1;	// +1 is for a newline character
						if(i == line(start))
							offset -= offsetInLine(start);
					}
				}
				return offset;
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
				return shrinkToAccessibleRegion(PointProxy(document(p), Position::bol(position(p))));
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
				const auto ln = line(p);
				return shrinkToAccessibleRegion(PointProxy(document(p), Position(ln, document(p).lineLength(ln))));
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
			 * @return true if @a p is the end of the line
			 * @throw BadPositionException @a p is outside of the document
			 */
			bool isEndOfLine(const PointProxy& p) {
				return position(p) == endOfLine(p);	// this may throw BadPositionException
			}

			/**
			 * Returns @c true if the given position is outside of the accessible region of the document.
			 * @param p The point to check
			 * @return true if @a p is outside of the document
			 */
			bool isOutsideOfAccessibleRegion(const PointProxy& p) BOOST_NOEXCEPT {
				return shrinkToAccessibleRegion(p) != position(p);
			}

			/**
			 * Returns @c true if the given position is outside of the document.
			 * @param p The point to check
			 * @return true if @a p is outside of the document
			 */
			bool isOutsideOfDocumentRegion(const PointProxy& p) BOOST_NOEXCEPT {
				return shrinkToDocumentRegion(p) != position(p);
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
				const auto temp(document(p).bookmarker().next(line(p), direction, true, marks));
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
					// std.advance can't use because NoSuchElementException
					if(direction == Direction::forward())
						while(i.hasNext() && offset-- > 0) ++i;
					else
						while(i.hasPrevious() && offset-- > 0) --i;
					return i.tell();
				} else if(characterUnit == locations::GRAPHEME_CLUSTER) {
					text::GraphemeBreakIterator<DocumentCharacterIterator> i(DocumentCharacterIterator(document(p), document(p).accessibleRegion(), position(p)));
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
						component, kernel::detail::identifierSyntax(p));	// this may throw BadPositionException
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

			/**
			 * Shrinks the given position into the accessible region of the document.
			 * @param p The source position. This value can be outside of the document
			 * @return The result
			 */
			Position shrinkToAccessibleRegion(const PointProxy& p) BOOST_NOEXCEPT {
				if(!document(p).isNarrowed())
					return shrinkToDocumentRegion(p);
				return clamp(position(p), document(p).accessibleRegion());
			}

			/**
			 * Shrinks the given region into the accessible region of the document.
			 * @param document The document
			 * @param region The source region. This value can intersect with outside of the document
			 * @return The result. This may not be normalized
			 */
			Region shrinkToAccessibleRegion(const Document& document, const Region& region) BOOST_NOEXCEPT {
				return Region(
					shrinkToAccessibleRegion(PointProxy(document, *boost::const_begin(region))),
					shrinkToAccessibleRegion(PointProxy(document, *boost::const_end(region))));
			}

			/**
			 * Shrinks the given position into the document region.
			 * @param p The position
			 * @return The result
			 */
			Position shrinkToDocumentRegion(const PointProxy& p) BOOST_NOEXCEPT {
				return std::min(position(p), *boost::const_end(document(p).region()));
			}

			/**
			 * Shrinks the given region into the document region. The result may not be normalized.
			 * @param document The document
			 * @param region The region to shrink
			 * @return The result
			 */
			Region shrinkToDocumentRegion(const Document& document, const Region& region) BOOST_NOEXCEPT {
				return Region(
					shrinkToDocumentRegion(PointProxy(document, *boost::const_begin(region))),
					shrinkToDocumentRegion(PointProxy(document, *boost::const_end(region))));
			}

			/**
			 * Adapts the specified position to the document change.
			 *
			 * <h3>Insertion</h3>
			 * When "DEF" is inserted between "abc" and "ghi" ('|' is the position to update):
			 * <table>
			 *   <tr><th>Case</th><th>@a gravity</th><th>Before</th><th>After</th></tr>
			 *   <tr><td>(I-1)</td><td>Any</td><td><code>a b|c g h i</code></td><td><code>a b|c D E F g h i</code></td></tr>
			 *   <tr><td>(I-2a)</td><td>@c Direction#forward()</td><td><code>a b c|g h i</code></td><td><code>a b c D E F|g h i</code></td></tr>
			 *   <tr><td>(I-2b)</td><td>@c Direction#backward()</td><td><code>a b c|g h i</code></td><td><code>a b c|D E F g h i</code></td></tr>
			 *   <tr><td>(I-3)</td><td>Any</td><td><code>a b c g|h i</code></td><td><code>a b c D E F g|h i</code></td></tr>
			 * </table>
			 *
			 * <h3>Deletion</h3>
			 * When "DEF" is erased from "abcDEFghi" ('|' is the position to update):
			 * <table>
			 *   <tr><th>Case</th><th>@a gravity</th><th>Before</th><th>After</th></tr>
			 *   <tr><td>(D-1a)</td><td>Any</td><td><code>a b|c D E F g h i</code></td><td><code>a b|c g h i</code></td></tr>
			 *   <tr><td>(D-1b)</td><td>Any</td><td><code>a b c|D E F g h i</code></td><td><code>a b c|g h i</code></td></tr>
			 *   <tr><td>(D-2)</td><td>Any</td><td><code>a b c D|E F g h i</code></td><td><code>a b c|g h i</code></td></tr>
			 *   <tr><td>(D-3a)</td><td>Any</td><td><code>a b c D E F|g h i</code></td><td><code>a b c|g h i</code></td></tr>
			 *   <tr><td>(D-3b)</td><td>Any</td><td><code>a b c D E F g|h i</code></td><td><code>a b c g|h i</code></td></tr>
			 * </table>
			 *
			 * @param position The original position
			 * @param change The content of the document change
			 * @param gravity The gravity which determines the direction to which the position should move if a text
			 *                was inserted at the position. If @c Direction#backward() is specified, the position will
			 *                move to the start of the inserted text (no movement occur). Otherwise, move to the end of
			 *                the inserted text
			 * @return The result position
			 * @see viewer#locations#updateTextHit
			 */
			Position updatePosition(const Position& position, const DocumentChange& change, Direction gravity) BOOST_NOEXCEPT {
				return detail::updatePositionForInsertion(
					detail::updatePositionForDeletion(
						position, change.erasedRegion(), gravity), change.insertedRegion(), gravity);
			}

			namespace detail {
				/// @internal Implements @c updatePosition function.
				Position updatePositionForDeletion(const Position& position, const Region& region, Direction gravity) BOOST_NOEXCEPT {
					boost::ignore_unused(gravity);
					assert(*boost::const_begin(region) <= *boost::const_end(region));
					Position p(position);
					if(!boost::empty(region)) {
						const auto& b = *boost::const_begin(region), e = *boost::const_end(region);
						if(p > b) {	// !(D-1)
							if(p <= e)	// (D-2)
								p = b;
							else {	// (D-3)
								p.offsetInLine -= (line(position) == line(e) ? offsetInLine(e) : 0) - (line(position) == line(b) ? offsetInLine(b) : 0);
								p.line -= boost::size(region.lines()) - 1;
							}
						}
					}
					return p;
				}

				/// @internal Implements @c updatePosition function.
				Position updatePositionForInsertion(const Position& position, const Region& region, Direction gravity) BOOST_NOEXCEPT {
					assert(*boost::const_begin(region) <= *boost::const_end(region));
					Position p(position);
					if(!boost::empty(region)) {
						const auto& b = *boost::const_begin(region), e = *boost::const_end(region);
						if(gravity == Direction::forward())	// (I-2a)
							p = e;
						else if(p > b) {	// (I-3) & !(I-1) & !(I-2b)
							if(line(p) == line(b))
								p.offsetInLine += offsetInLine(e) - offsetInLine(b);
							p.line += boost::size(region.lines()) - 1;
						}
					}
					return p;
				}
			}
		}
	}
}
