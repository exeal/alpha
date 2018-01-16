/**
 * @file visual-locations.cpp
 * Implements @c viewer#locations namespace.
 * @author exeal
 * @see kernel/locations.cpp
 * @date 2003-2008 Was point.cpp
 * @date 2008-2010 Separated from point.cpp.
 * @date 2011-10-02 Separated from caret.cpp.
 * @date 2016-05-22 Separated from visual-point.cpp.
 */

#include <ascension/corelib/text/identifier-syntax.hpp>
#include <ascension/graphics/font/line-layout-vector.hpp>
#include <ascension/graphics/font/text-layout.hpp>
#include <ascension/graphics/font/text-viewport.hpp>
#include <ascension/kernel/document.hpp>
#include <ascension/kernel/document-character-iterator.hpp>
#include <ascension/kernel/locations.hpp>
#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/text-area.hpp>
#include <ascension/viewer/visual-locations.hpp>

namespace ascension {
	namespace viewer {
		namespace locations {
			namespace {
				inline const kernel::Document& document(const PointProxy& p) BOOST_NOEXCEPT {
					return p.textArea.caret()->document();
				}
				inline kernel::Position position(const PointProxy& p) BOOST_NOEXCEPT {
					return insertionPosition(document(p), p.hit);
				}
				inline kernel::Position normalPosition(const PointProxy& p) BOOST_NOEXCEPT {
					return kernel::locations::shrinkToAccessibleRegion(kernel::locations::PointProxy(document(p), position(p)));
				}
				inline TextHit normalHit(const PointProxy& p) BOOST_NOEXCEPT {
					const auto np(kernel::locations::shrinkToAccessibleRegion(kernel::locations::PointProxy(document(p), p.hit.characterIndex())));
					if(np != p.hit.characterIndex() || kernel::locations::isEndOfLine(kernel::locations::PointProxy(document(p), np)))
						return TextHit::leading(np);
					return p.hit;
				}
				inline kernel::locations::PointProxy kernelProxy(const PointProxy& p) {
					return kernel::locations::PointProxy(document(p), position(p));
				}
				inline graphics::font::TextHit<> inlineHit(const TextHit& hit) BOOST_NOEXCEPT {
					return graphics::font::transformTextHit(hit, [](const kernel::Position& p) {
						return kernel::offsetInLine(p);
					});
				}
			}

#ifdef ASCENSION_ABANDONED_AT_VERSION_08
			/**
			 * Returns the position returned by N pages.
			 * @param p The base position
			 * @param pages The number of pages to return
			 * @return The destination
			 */
			viewer::VisualDestinationProxy backwardPage(const VisualPoint& p, Index pages /* = 1 */) {
				Index lines = 0;
				const std::shared_ptr<const graphics::font::TextViewport> viewport(p.textViewer().textRenderer().viewport());
				// TODO: calculate exact number of visual lines.
				lines = static_cast<Index>(viewport->numberOfVisibleLines() * pages);
			
				return backwardVisualLine(p, lines);
			}

			/**
			 * Returns the position returned by N visual lines.
			 * @param p The base position
			 * @param lines The number of the visual lines to return
			 * @return The destination
			 */
			viewer::VisualDestinationProxy backwardVisualLine(const VisualPoint& p, Index lines /* = 1 */) {
				kernel::Position np(p.normalized());
				const graphics::font::TextRenderer& renderer = p.textViewer().textRenderer();
				Index subline = renderer.layouts().at(kernel::line(np)).lineAt(kernel::offsetInLine(np));
				if(kernel::line(np) == 0 && subline == 0)
					return detail::VisualDestinationProxyMaker::make(np, true);
				graphics::font::VisualLine visualLine(kernel::line(np), subline);
				renderer.layouts().offsetVisualLine(visualLine, -static_cast<SignedIndex>(lines));
				const graphics::font::TextLayout& layout = renderer.layouts().at(visualLine.line);
				if(!p.positionInVisualLine_)
					const_cast<VisualPoint&>(p).rememberPositionInVisualLine();
				const graphics::Scalar ipd = *p.positionInVisualLine_ - graphics::font::lineStartEdge(layout, renderer.viewport()->contentMeasure());
				const graphics::Scalar bpd = layout.baseline(visualLine.subline);
				np.offsetInLine = layout.offset(
					isHorizontal(layout.writingMode().blockFlowDirection) ?
						geometry::make<NativePoint>(ipd, bpd) : geometry::make<NativePoint>(bpd, ipd)).second;
				if(layout.lineAt(kernel::offsetInLine(np)) != visualLine.subline)
					np = nextCharacter(p.document(), np, Direction::backward(), GRAPHEME_CLUSTER);
				return detail::VisualDestinationProxyMaker::make(np, true);
			}
#endif // ASCENSION_ABANDONED_AT_VERSION_08

			/**
			 * Returns the beginning of the visual line. If the layout of the line where @a p is placed is not
			 * calculated, this function calls @c beginningOfLine(p).
			 * @param p The base position
			 * @return The destination
			 * @see beginningOfLine
			 */
			TextHit beginningOfVisualLine(const PointProxy& p) {
				const auto h(normalHit(p));
				if(const graphics::font::TextLayout* const layout = p.textArea.textRenderer()->layouts().at(kernel::line(h.characterIndex())))
					return TextHit::leading(kernel::Position(kernel::line(h.characterIndex()), layout->lineOffset(layout->lineAt(inlineHit(h)))));
				return TextHit::leading(kernel::locations::beginningOfLine(kernelProxy(p)));
			}

			/**
			 * Returns the beginning of the line or the first printable character in the line by context.
			 * @param p The base position
			 * @return The destination
			 */
			TextHit contextualBeginningOfLine(const PointProxy& p) {
				return isFirstPrintableCharacterOfLine(p) ? TextHit::leading(kernel::locations::beginningOfLine(kernelProxy(p))) : firstPrintableCharacterOfLine(p);
			}

			/**
			 * Moves to the beginning of the visual line or the first printable character in the visual line by
			 * context.
			 * @param p The base position
			 * @return The destination
			 */
			TextHit contextualBeginningOfVisualLine(const PointProxy& p) {
				return isFirstPrintableCharacterOfLine(p) ? beginningOfVisualLine(p) : firstPrintableCharacterOfVisualLine(p);
			}

			/**
			 * Moves to the end of the line or the last printable character in the line by context.
			 * @param p The base position
			 * @return The destination
			 */
			TextHit contextualEndOfLine(const PointProxy& p) {
				return isLastPrintableCharacterOfLine(p) ? otherHit(document(p), TextHit::leading(kernel::locations::endOfLine(kernelProxy(p)))) : lastPrintableCharacterOfLine(p);
			}

			/**
			 * Moves to the end of the visual line or the last printable character in the visual line by
			 * context.
			 * @param p The base position
			 * @return The destination
			 */
			TextHit contextualEndOfVisualLine(const PointProxy& p) {
				return isLastPrintableCharacterOfLine(p) ? endOfVisualLine(p) : lastPrintableCharacterOfVisualLine(p);
			}

			/**
			 * Returns the end of the visual line. If the layout of the line where @a p is placed is not calculated,
			 * this function calls @c endOfLine(p).
			 * @param p The base position
			 * @return The destination
			 * @see endOfLine
			 */
			TextHit endOfVisualLine(const PointProxy& p) {
				auto h(normalHit(p));
				const auto line = kernel::line(h.characterIndex());
				if(const graphics::font::TextLayout* const layout = p.textArea.textRenderer()->layouts().at(line)) {
					const Index subline = layout->lineAt(inlineHit(h));
					if(subline < layout->numberOfLines() - 1)
						return otherHit(document(p), TextHit::leading(kernel::Position(line, layout->lineOffset(subline + 1))));
				}
				return otherHit(document(p), TextHit::leading(kernel::locations::endOfLine(kernelProxy(p))));
			}

			/**
			 * Returns the first printable character in the line.
			 * @param p The base position
			 * @return The destination
			 * @see firstPrintableCharacterOfLine
			 */
			TextHit firstPrintableCharacterOfLine(const PointProxy& p) {
				kernel::Position np(normalPosition(p));
				const Char* const s = document(p).lineString(kernel::line(np)).data();
				np.offsetInLine = kernel::detail::identifierSyntax(kernelProxy(p)).eatWhiteSpaces(s, s + document(p).lineLength(kernel::line(np)), true) - s;
				return TextHit::leading(np);
			}

			/**
			 * Returns the first printable character in the visual line. If the layout of the line where @a p is placed
			 * is not calculated, this function calls @c firstPrintableCharacterOfLine(p).
			 * @param p The base position
			 * @return The destination
			 * @see firstPrintableCharacterOfLine
			 */
			TextHit firstPrintableCharacterOfVisualLine(const PointProxy& p) {
				kernel::Position np(normalPosition(p));
				const String& s = document(p).lineString(kernel::line(np));
				if(const graphics::font::TextLayout* const layout = p.textArea.textRenderer()->layouts().at(kernel::line(np))) {
					const Index subline = layout->lineAt(graphics::font::makeLeadingTextHit(kernel::offsetInLine(np)));
					np.offsetInLine = kernel::detail::identifierSyntax(kernelProxy(p)).eatWhiteSpaces(
						s.begin() + layout->lineOffset(subline),
						s.begin() + ((subline < layout->numberOfLines() - 1) ?
							layout->lineOffset(subline + 1) : s.length()), true) - s.begin();
					return TextHit::leading(np);
				}
				return firstPrintableCharacterOfLine(p);
			}

#ifdef ASCENSION_ABANDONED_AT_VERSION_08
			/**
			 * Returns the position advanced by N pages.
			 * @param p The base position
			 * @param pages The number of pages to advance
			 * @return The destination
			 */
			viewer::VisualDestinationProxy forwardPage(const viewer::VisualPoint& p, Index pages /* = 1 */) {
				Index lines = 0;
				const std::shared_ptr<const graphics::font::TextViewport> viewport(p.textViewer().textRenderer().viewport());
				// TODO: calculate exact number of visual lines.
				lines = static_cast<Index>(viewport->numberOfVisibleLines() * pages);

				return forwardVisualLine(p, lines);
			}

			/**
			 * Returns the position advanced by N visual lines.
			 * @param p The base position
			 * @param lines The number of the visual lines to advance
			 * @return The destination
			 */
			viewer::VisualDestinationProxy forwardVisualLine(const viewer::VisualPoint& p, Index lines /* = 1 */) {
				kernel::Position np(p.normalized());
				const graphics::font::TextRenderer& renderer = p.textViewer().textRenderer();
				const graphics::font::TextLayout* layout = &renderer.layouts().at(kernel::line(np));
				Index subline = layout->lineAt(kernel::offsetInLine(np));
				if(kernel::line(np) == p.document().numberOfLines() - 1 && subline == layout->numberOfLines() - 1)
					return detail::VisualDestinationProxyMaker::make(np, true);
				graphics::font::VisualLine visualLine(kernel::line(np), subline);
				renderer.layouts().offsetVisualLine(visualLine, static_cast<SignedIndex>(lines));
				layout = &renderer.layouts().at(visualLine.line);
				if(!p.positionInVisualLine_)
					const_cast<VisualPoint&>(p).rememberPositionInVisualLine();
				const graphics::Scalar ipd = *p.positionInVisualLine_ - graphics::font::lineStartEdge(*layout, renderer.viewport()->contentMeasure());
				const graphics::Scalar bpd = layout->baseline(visualLine.subline);
				np.offsetInLine = layout->offset(
					isHorizontal(layout->writingMode().blockFlowDirection) ?
						geometry::make<NativePoint>(ipd, bpd) : geometry::make<NativePoint>(bpd, ipd)).second;
				if(layout->lineAt(kernel::offsetInLine(np)) != visualLine.subline)
					np = nextCharacter(document(p), np, Direction::backward(), GRAPHEME_CLUSTER);

				return detail::VisualDestinationProxyMaker::make(np, true);
			}
#endif // ASCENSION_ABANDONED_AT_VERSION_08

			/**
			 * Returns @c true if the point is the beginning of the visual line. If the layout of the line where @a p is placed
			 * is not calculated, this function calls @c isBeginningOfLine(p).
			 * @param p The base position
			 * @see isBeginningOfLine
			 */
			bool isBeginningOfVisualLine(const PointProxy& p) {
				if(kernel::locations::isBeginningOfLine(kernelProxy(p)))	// this considers narrowing
					return true;
				const kernel::Position np(normalPosition(p));
				if(const graphics::font::TextLayout* const layout = p.textArea.textRenderer()->layouts().at(kernel::line(np)))
					return kernel::offsetInLine(np) == layout->lineOffset(layout->lineAt(graphics::font::makeLeadingTextHit(kernel::offsetInLine(np))));
				return kernel::locations::isBeginningOfLine(kernelProxy(p));
			}

			/**
			 * Returns @c true if the point is end of the visual line. If the layout of the line where @a p is placed
			 * is not calculated, this function calls @c isEndOfLine(p).
			 * @param p The base position
			 * @see isEndOfLine
			 */
			bool isEndOfVisualLine(const PointProxy& p) {
				if(kernel::locations::isEndOfLine(kernelProxy(p)))	// this considers narrowing
					return true;
				const kernel::Position np(normalPosition(p));
				if(const graphics::font::TextLayout* const layout = p.textArea.textRenderer()->layouts().at(kernel::line(np))) {
					const Index subline = layout->lineAt(graphics::font::makeLeadingTextHit(kernel::offsetInLine(np)));
					return kernel::offsetInLine(np) == layout->lineOffset(subline) + layout->lineLength(subline);
				}
				return kernel::locations::isEndOfLine(kernelProxy(p));
			}

			/// Returns @c true if the given position is the first printable character in the line.
			bool isFirstPrintableCharacterOfLine(const PointProxy& p) {
				const kernel::Position np(normalPosition(p)), bob(*boost::const_begin(document(p).accessibleRegion()));
				const Index offset = (kernel::line(bob) == kernel::line(np)) ? kernel::offsetInLine(bob) : 0;
				const String& line = document(p).lineString(kernel::line(np));
				return line.data() + kernel::offsetInLine(np) - offset
					== kernel::detail::identifierSyntax(kernelProxy(p)).eatWhiteSpaces(line.data() + offset, line.data() + line.length(), true);
			}

			/// Returns @c true if the given position is the first printable character in the visual line.
			bool isFirstPrintableCharacterOfVisualLine(const PointProxy& p) {
				// TODO: not implemented.
				return false;
			}

			/// Returns @c true if the given position is the last printable character in the line.
			bool isLastPrintableCharacterOfLine(const PointProxy& p) {
				const kernel::Position np(normalPosition(p)), eob(*boost::const_end(document(p).accessibleRegion()));
				const String& line = document(p).lineString(kernel::line(np));
				const Index lineLength = (kernel::line(eob) == kernel::line(np)) ? kernel::offsetInLine(eob) : line.length();
				return line.data() + lineLength - kernel::offsetInLine(np)
					== kernel::detail::identifierSyntax(kernelProxy(p)).eatWhiteSpaces(line.data() + kernel::offsetInLine(np), line.data() + lineLength, true);
			}

			/// Returns @c true if the given position is the last printable character in the visual line.
			bool isLastPrintableCharacterOfVisualLine(const PointProxy& p) {
				// TODO: not implemented.
				return false;
			}

			/**
			 * Returns the last printable character in the line.
			 * @param p The base position
			 * @return The destination
			 */
			TextHit lastPrintableCharacterOfLine(const PointProxy& p) {
				kernel::Position np(normalPosition(p));
				const String& s(document(p).lineString(kernel::line(np)));
				const text::IdentifierSyntax& syntax = kernel::detail::identifierSyntax(kernelProxy(p));
				for(Index spaceLength = 0; spaceLength < s.length(); ++spaceLength) {
					if(syntax.isWhiteSpace(s[s.length() - spaceLength - 1], true))
						return np.offsetInLine = s.length() - spaceLength, otherHit(document(p), TextHit::leading(np));
				}
				return np.offsetInLine = s.length(), otherHit(document(p), TextHit::leading(np));
			}

			/**
			 * Moves to the last printable character in the visual line.
			 * @param p The base position
			 * @return The destination
			 */
			TextHit lastPrintableCharacterOfVisualLine(const PointProxy& p) {
				// TODO: not implemented.
				return TextHit::leading(normalPosition(p));
			}

#ifdef ASCENSION_ABANDONED_AT_VERSION_08
			namespace {
				inline ReadingDirection defaultUIReadingDirection(const PointProxy& p) {
					return p.textViewer().textRenderer().defaultUIWritingMode().inlineFlowDirection;
				}
			}

			/**
			 * Returns the beginning of the word where advanced to the left by N words.
			 * @param p The base position
			 * @param words The number of words to adavance
			 * @return The destination, or @c boost#none if the writing mode is vertical
			 */
			boost::optional<TextHit> leftWord(const PointProxy& p, Index words /* = 1 */) {
				return (defaultUIReadingDirection(p) == LEFT_TO_RIGHT) ? backwardWord(p, words) : forwardWord(p, words);
			}

			/**
			 * Returns the end of the word where advanced to the left by N words.
			 * @param p The base position
			 * @param words The number of words to adavance
			 * @return The destination, or @c boost#none if the writing mode is vertical
			 */
			boost::optional<TextHit> leftWordEnd(const PointProxy& p, Index words /* = 1 */) {
				return (defaultUIReadingDirection(p) == LEFT_TO_RIGHT) ? backwardWordEnd(p, words) : forwardWordEnd(p, words);
			}

			/**
			 * Returns the beginning of the next bookmarked line.
			 * @param p The base point
			 * @param direction The direction
			 * @return The beginning of the forward/backward bookmarked line, or @c boost#none if there is no
			 *         bookmark in the document or @a direction is inline-progression
			 * @see #nextBookmark
			 */
			boost::optional<TextHit> nextBookmarkInPhysicalDirection(
					const PointProxy& p, graphics::PhysicalDirection direction, Index marks /* = 1 */) {
				switch(mapPhysicalToFlowRelative(p.textViewer().textRenderer().defaultUIWritingMode(), direction)) {
					case BEFORE:
						return nextBookmark(p, Direction::backward(), marks);
					case AFTER:
						return nextBookmark(p, Direction::forward(), marks);
					case START:
					case END:
						return boost::none;
					default:
						ASCENSION_ASSERT_NOT_REACHED();
				}
			}

			/**
			 * Returns the position advanced to the left by N characters.
			 * @param p The base position
			 * @param direction The physical direction
			 * @param unit Defines what a character is
			 * @param characters The number of characters to adavance
			 * @return The destination
			 * @see #nextCharacter
			 */
			viewer::VisualDestinationProxy nextCharacterInPhysicalDirection(
					const PointProxy& p, PhysicalDirection direction, CharacterUnit unit, Index characters /* = 1 */) {
				switch(mapPhysicalToFlowRelative(p.textViewer().textRenderer().defaultUIWritingMode(), direction)) {
					case BEFORE:
						return nextVisualLine(p, Direction::backward(), characters);
					case AFTER:
						return nextVisualLine(p, Direction::forward(), characters);
					case START:
						return detail::VisualDestinationProxyMaker::make(nextCharacter(p, Direction::backward(), unit, characters), false);
					case END:
						return detail::VisualDestinationProxyMaker::make(nextCharacter(p, Direction::forward(), unit, characters), false);
					default:
						ASCENSION_ASSERT_NOT_REACHED();
				}
			}
#endif // ASCENSION_ABANDONED_AT_VERSION_08

#ifdef ASCENSION_ABANDONED_AT_VERSION_08
			/**
			 * Returns the beginning of the word where advanced to the left/right/top/bottom by N words.
			 * @param p The base position
			 * @param words The number of words to adavance
			 * @return The destination, or @c boost#none if @a direction is block-progression
			 * @see #nextWord, #nextWordEndInPhysicalDirection
			 */
			boost::optional<TextHit> nextWordInPhysicalDirection(
					const PointProxy& p, PhysicalDirection direction, Index words /* = 1 */) {
				switch(mapPhysicalToFlowRelative(p.textViewer().textRenderer().defaultUIWritingMode(), direction)) {
					case BEFORE:
					case AFTER:
						return boost::none;
					case START:
						return nextWord(p, Direction::backward(), words);
					case END:
						return nextWord(p, Direction::forward(), words);
					default:
						ASCENSION_ASSERT_NOT_REACHED();
				}
			}

			/**
			 * Returns the end of the word where advanced to the left/right/top/bottom by N words.
			 * @param p The base position
			 * @param words The number of words to adavance
			 * @return The destination, or @c boost#none if @a direction is block-progression
			 * @see #nextWordEnd, #nextWordInPhysicalDirection
			 */
			boost::optional<TextHit> nextWordEndInPhysicalDirection(
					const PointProxy& p, PhysicalDirection direction, Index words /* = 1 */) {
				switch(mapPhysicalToFlowRelative(p.textViewer().textRenderer().defaultUIWritingMode(), direction)) {
					case BEFORE:
					case AFTER:
						return boost::none;
					case START:
						return nextWordEnd(p, Direction::backward(), words);
					case END:
						return nextWordEnd(p, Direction::forward(), words);
					default:
						ASCENSION_ASSERT_NOT_REACHED();
				}
			}

			/**
			 * Returns the beginning of the word where advanced to the right by N words.
			 * @param p The base position
			 * @param words The number of words to adavance
			 * @return The destination
			 */
			boost::optional<TextHit> rightWord(const PointProxy& p, Index words /* = 1 */) {
				return (defaultUIReadingDirection(p) == LEFT_TO_RIGHT) ? forwardWord(p, words) : backwardWord(p, words);
			}

			/**
			 * Returns the end of the word where advanced to the right by N words.
			 * @param p The base position
			 * @param words The number of words to adavance
			 * @return The destination
			 */
			boost::optional<TextHit> rightWordEnd(const PointProxy& p, Index words /* = 1 */) {
				return (defaultUIReadingDirection(p) == LEFT_TO_RIGHT) ? forwardWordEnd(p, words) : backwardWordEnd(p, words);
			}
#endif // ASCENSION_ABANDONED_AT_VERSION_08

			/**
			 * Adapts the specified @c TextHit to the document change.
			 *
			 * <h3>Insertion</h3>
			 * When "DEF" is inserted between "abc" and "ghi" (underlined is the character-index and '|' is the
			 * insertion-index):
			 * <table>
			 *   <tr><th>Case</th><th>@a gravity</th><th>Before</th><th>After</th></tr>
			 *   <tr>
			 *     <td>(I-1)</td><td>Any</td>
			 *     <td><code>a b<span style="text-decoration:underline">|c</span> g h i</code></td>
			 *     <td><code>a b<span style="text-decoration:underline">|c</span> D E F g h i</code></td>
			 *   </tr>
			 *   <tr>
			 *     <td>(I-2a)</td><td>@c Direction#forward()</td>
			 *     <td><code>a b <span style="text-decoration:underline">c|</span>g h i</code></td>
			 *     <td><code>a b c D E <span style="text-decoration:underline">F|</span>g h i</code></td>
			 *   </tr>
			 *   <tr>
			 *     <td>(I-2b)</td><td>@c Direction#backward()</td>
			 *     <td><code>a b <span style="text-decoration:underline">c|</span>g h i</code></td>
			 *     <td><code>a b <span style="text-decoration:underline">c|</span>D E F g h i</code></td>
			 *   </tr>
			 *   <tr>
			 *     <td>(I-3a)</td><td>@c Direction#forward()</td>
			 *     <td><code>a b c<span style="text-decoration:underline">|g</span> h i</code></td>
			 *     <td><code>a b c D E F<span style="text-decoration:underline">|g</span> h i</code></td>
			 *   </tr>
			 *   <tr>
			 *     <td>(I-3b)</td><td>@c Direction#backward()</td>
			 *     <td><code>a b c<span style="text-decoration:underline">|g</span> h i</code></td>
			 *     <td><code>a b c<span style="text-decoration:underline">|D</span> E F g h i</code></td>
			 *   </tr>
			 *   <tr>
			 *     <td>(I-4)</td><td>Any</td>
			 *     <td><code>a b c <span style="text-decoration:underline">g|</span>h i</code></td>
			 *     <td><code>a b c D E F <span style="text-decoration:underline">g|</span>h i</code></td>
			 *  </tr>
			 * </table>
			 *
			 * <h3>Deletion</h3>
			 * When "DEF" is erased from "abcDEFghi" (underlined is the character-index and '|' is the
			 * insertion-index):
			 * <table>
			 *   <tr><th>Case</th><th>@a gravity</th><th>Before</th><th>After</th></tr>
			 *   <tr>
			 *     <td>(D-1)</td><td>Any</td>
			 *     <td><code>a b <span style="text-decoration:underline">c|</span>D E F g h i</code></td>
			 *     <td><code>a b <span style="text-decoration:underline">c|</span>g h i</code></td>
			 *   </tr>
			 *   <tr>
			 *     <td>(D-2a)</td><td>Any</td>
			 *     <td><code>a b c<span style="text-decoration:underline">|D</span> E F g h i</code></td>
			 *     <td><code>a b c<span style="text-decoration:underline">|g</span> h i</code></td>
			 *   </tr>
			 *   <tr>
			 *     <td>(D-2b)</td><td>Any</td>
			 *     <td><code>a b c <span style="text-decoration:underline">D|</span>E F g h i</code></td>
			 *     <td><code>a b c<span style="text-decoration:underline">|g</span> h i</code></td>
			 *   </tr>
			 *   <tr>
			 *     <td>(D-2c)</td><td>Any</td>
			 *     <td><code>a b c D<span style="text-decoration:underline">|E</span> F g h i</code></td>
			 *     <td><code>a b c<span style="text-decoration:underline">|g</span> h i</code></td>
			 *   </tr>
			 *   <tr>
			 *     <td>(D-3)</td><td>Any</td>
			 *     <td><code>a b c D E <span style="text-decoration:underline">F|</span>g h i</code></td>
			 *     <td><code>a b <span style="text-decoration:underline">c|</span>g h i</code></td>
			 *   </tr>
			 *   <tr>
			 *     <td>(D-4)</td><td>Any</td>
			 *     <td><code>a b c D E F<span style="text-decoration:underline">|g</span> h i</code></td>
			 *     <td><code>a b c<span style="text-decoration:underline">|g</span> h i</code></td>
			 *   </tr>
			 * </table>
			 *
			 * @param hit The original text hit
			 * @param document The document
			 * @param change The content of the document change
			 * @param gravity See @c kernel#locations#updatePosition method and tables above
			 * @return The result text hit
			 * @throw ... Any exception @c kernel#DocumentCharacterIterator throws
			 * @note This function creates and uses @c kernel#DocumentCharacterIterator instance. After @a change is
			 *       processed by @a document, some exception may occur.
			 * @see kernel#locations#updatePosition
			 */
			TextHit updateTextHit(const TextHit& hit, const kernel::Document& document, const kernel::DocumentChange& change, Direction gravity) {
				TextHit h(hit);
				if(!boost::empty(change.erasedRegion())) {	// deletion
					assert(*boost::const_begin(change.erasedRegion()) <= *boost::const_end(change.erasedRegion()));
					const auto& b = *boost::const_begin(change.erasedRegion()), e = *boost::const_end(change.erasedRegion());
					if(h.characterIndex() >= b && h.characterIndex() < e) {	// (D-2) or (D-3)
						if(insertionPosition(document, h) < e)	// (D-2)
							h = TextHit::leading(b);
						else {	// (D-3)
							kernel::DocumentCharacterIterator i(document, b);
							h = TextHit::trailing((--i).tell());
						}
					} else { // (D-1) or (D-4)
						const auto p(kernel::locations::detail::updatePositionForDeletion(h.characterIndex(), change.erasedRegion(), gravity));
						h = h.isLeadingEdge() ? TextHit::leading(p) : TextHit::trailing(p);
					}
				}
				if(!boost::empty(change.insertedRegion())) {	// insertion
					assert(*boost::const_begin(change.insertedRegion()) <= *boost::const_end(change.insertedRegion()));
					const auto& b = *boost::const_begin(change.insertedRegion()), e = *boost::const_end(change.insertedRegion());
					const auto ip(insertionPosition(document, h));
					if(ip == b) {	// (I-2) or (I-3)
						h = TextHit::leading((gravity == Direction::forward()) ? e : b);
						if(!h.isLeadingEdge())	// (I-2)
							h = otherHit(document, h);
					} else {	// (I-1) or (I-4)
						const auto p(kernel::locations::detail::updatePositionForInsertion(h.characterIndex(), change.insertedRegion(), gravity));
						h = h.isLeadingEdge() ? TextHit::leading(p) : TextHit::trailing(p);
					}
				}
				return h;
			}
		}
	}
}
