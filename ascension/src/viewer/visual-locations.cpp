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
#include <ascension/kernel/locations.hpp>
#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/text-area.hpp>
#include <ascension/viewer/visual-locations.hpp>

namespace ascension {
	namespace viewer {
		namespace locations {
			namespace {
				inline const kernel::Document& document(const PointProxy& p) BOOST_NOEXCEPT {
					return std::get<0>(p).caret().document();
				}

				inline const kernel::Position& position(const PointProxy& p) BOOST_NOEXCEPT {
					return std::get<1>(p);
				}

				inline kernel::Position normalPosition(const PointProxy& p) BOOST_NOEXCEPT {
					return kernel::positions::shrinkToAccessibleRegion(document(p), position(p));
				}

				inline const TextArea& textArea(const PointProxy& p) BOOST_NOEXCEPT {
					return std::get<0>(p);
				}

				inline std::pair<const kernel::Document&, kernel::Position> kernelProxy(const PointProxy& p) {
					return std::make_pair(std::ref(document(p)), position(p));
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
					np = nextCharacter(p.document(), np, Direction::BACKWARD, GRAPHEME_CLUSTER);
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
			kernel::Position beginningOfVisualLine(const PointProxy& p) {
				const auto np(normalPosition(p));
				if(const graphics::font::TextLayout* const layout = textArea(p).textRenderer().layouts().at(kernel::line(np)))
					return kernel::Position(kernel::line(np), layout->lineOffset(layout->lineAt(graphics::font::makeLeadingTextHit(kernel::offsetInLine(np)))));
				return kernel::locations::beginningOfLine(kernelProxy(p));
			}

			/**
			 * Returns the beginning of the line or the first printable character in the line by context.
			 * @param p The base position
			 * @return The destination
			 */
			kernel::Position contextualBeginningOfLine(const PointProxy& p) {
				return isFirstPrintableCharacterOfLine(p) ? kernel::locations::beginningOfLine(kernelProxy(p)) : firstPrintableCharacterOfLine(p);
			}

			/**
			 * Moves to the beginning of the visual line or the first printable character in the visual line by
			 * context.
			 * @param p The base position
			 * @return The destination
			 */
			kernel::Position contextualBeginningOfVisualLine(const PointProxy& p) {
				return isFirstPrintableCharacterOfLine(p) ?
					beginningOfVisualLine(p) : firstPrintableCharacterOfVisualLine(p);
			}

			/**
			 * Moves to the end of the line or the last printable character in the line by context.
			 * @param p The base position
			 * @return The destination
			 */
			kernel::Position contextualEndOfLine(const PointProxy& p) {
				return isLastPrintableCharacterOfLine(p) ? kernel::locations::endOfLine(kernelProxy(p)) : lastPrintableCharacterOfLine(p);
			}

			/**
			 * Moves to the end of the visual line or the last printable character in the visual line by
			 * context.
			 * @param p The base position
			 * @return The destination
			 */
			kernel::Position contextualEndOfVisualLine(const PointProxy& p) {
				return isLastPrintableCharacterOfLine(p) ? endOfVisualLine(p) : lastPrintableCharacterOfVisualLine(p);
			}

			/**
			 * Returns the end of the visual line. If the layout of the line where @a p is placed is not calculated,
			 * this function calls @c endOfLine(p).
			 * @param p The base position
			 * @return The destination
			 * @see endOfLine
			 */
			kernel::Position endOfVisualLine(const PointProxy& p) {
				kernel::Position np(normalPosition(p));
				if(const graphics::font::TextLayout* const layout = textArea(p).textRenderer().layouts().at(kernel::line(np))) {
					const Index subline = layout->lineAt(graphics::font::makeLeadingTextHit(kernel::offsetInLine(np)));
					np.offsetInLine = (subline < layout->numberOfLines() - 1) ?
						layout->lineOffset(subline + 1) : document(p).lineLength(kernel::line(np));
					if(layout->lineAt(graphics::font::makeLeadingTextHit(kernel::offsetInLine(np))) != subline)
						np = kernel::locations::nextCharacter(std::make_pair(std::ref(document(p)), np), Direction::BACKWARD, kernel::locations::GRAPHEME_CLUSTER);
					return np;
				}
				return kernel::locations::endOfLine(kernelProxy(p));
			}

			/**
			 * Returns the first printable character in the line.
			 * @param p The base position
			 * @return The destination
			 * @see firstPrintableCharacterOfLine
			 */
			kernel::Position firstPrintableCharacterOfLine(const PointProxy& p) {
				kernel::Position np(normalPosition(p));
				const Char* const s = document(p).lineString(kernel::line(np)).data();
				np.offsetInLine = kernel::detail::identifierSyntax(kernelProxy(p)).eatWhiteSpaces(s, s + document(p).lineLength(kernel::line(np)), true) - s;
				return np;
			}

			/**
			 * Returns the first printable character in the visual line. If the layout of the line where @a p is placed
			 * is not calculated, this function calls @c firstPrintableCharacterOfLine(p).
			 * @param p The base position
			 * @return The destination
			 * @see firstPrintableCharacterOfLine
			 */
			kernel::Position firstPrintableCharacterOfVisualLine(const PointProxy& p) {
				kernel::Position np(normalPosition(p));
				const String& s = document(p).lineString(kernel::line(np));
				if(const graphics::font::TextLayout* const layout = textArea(p).textRenderer().layouts().at(kernel::line(np))) {
					const Index subline = layout->lineAt(graphics::font::makeLeadingTextHit(kernel::offsetInLine(np)));
					np.offsetInLine = kernel::detail::identifierSyntax(kernelProxy(p)).eatWhiteSpaces(
						s.begin() + layout->lineOffset(subline),
						s.begin() + ((subline < layout->numberOfLines() - 1) ?
							layout->lineOffset(subline + 1) : s.length()), true) - s.begin();
					return np;
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
					np = nextCharacter(document(p), np, Direction::BACKWARD, GRAPHEME_CLUSTER);

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
				if(const graphics::font::TextLayout* const layout = textArea(p).textRenderer().layouts().at(kernel::line(np)))
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
				if(const graphics::font::TextLayout* const layout = textArea(p).textRenderer().layouts().at(kernel::line(np))) {
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
			kernel::Position lastPrintableCharacterOfLine(const PointProxy& p) {
				kernel::Position np(normalPosition(p));
				const String& s(document(p).lineString(kernel::line(np)));
				const text::IdentifierSyntax& syntax = kernel::detail::identifierSyntax(kernelProxy(p));
				for(Index spaceLength = 0; spaceLength < s.length(); ++spaceLength) {
					if(syntax.isWhiteSpace(s[s.length() - spaceLength - 1], true))
						return np.offsetInLine = s.length() - spaceLength, np;
				}
				return np.offsetInLine = s.length(), np;
			}

			/**
			 * Moves to the last printable character in the visual line.
			 * @param p The base position
			 * @return The destination
			 */
			kernel::Position lastPrintableCharacterOfVisualLine(const PointProxy& p) {
				// TODO: not implemented.
				return normalPosition(p);
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
			boost::optional<kernel::Position> leftWord(const PointProxy& p, Index words /* = 1 */) {
				return (defaultUIReadingDirection(p) == LEFT_TO_RIGHT) ? backwardWord(p, words) : forwardWord(p, words);
			}

			/**
			 * Returns the end of the word where advanced to the left by N words.
			 * @param p The base position
			 * @param words The number of words to adavance
			 * @return The destination, or @c boost#none if the writing mode is vertical
			 */
			boost::optional<kernel::Position> leftWordEnd(const PointProxy& p, Index words /* = 1 */) {
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
			boost::optional<kernel::Position> nextBookmarkInPhysicalDirection(
					const PointProxy& p, graphics::PhysicalDirection direction, Index marks /* = 1 */) {
				switch(mapPhysicalToFlowRelative(p.textViewer().textRenderer().defaultUIWritingMode(), direction)) {
					case BEFORE:
						return nextBookmark(p, Direction::BACKWARD, marks);
					case AFTER:
						return nextBookmark(p, Direction::FORWARD, marks);
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
						return nextVisualLine(p, Direction::BACKWARD, characters);
					case AFTER:
						return nextVisualLine(p, Direction::FORWARD, characters);
					case START:
						return detail::VisualDestinationProxyMaker::make(nextCharacter(p, Direction::BACKWARD, unit, characters), false);
					case END:
						return detail::VisualDestinationProxyMaker::make(nextCharacter(p, Direction::FORWARD, unit, characters), false);
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
			boost::optional<kernel::Position> nextWordInPhysicalDirection(
					const PointProxy& p, PhysicalDirection direction, Index words /* = 1 */) {
				switch(mapPhysicalToFlowRelative(p.textViewer().textRenderer().defaultUIWritingMode(), direction)) {
					case BEFORE:
					case AFTER:
						return boost::none;
					case START:
						return nextWord(p, Direction::BACKWARD, words);
					case END:
						return nextWord(p, Direction::FORWARD, words);
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
			boost::optional<kernel::Position> nextWordEndInPhysicalDirection(
					const PointProxy& p, PhysicalDirection direction, Index words /* = 1 */) {
				switch(mapPhysicalToFlowRelative(p.textViewer().textRenderer().defaultUIWritingMode(), direction)) {
					case BEFORE:
					case AFTER:
						return boost::none;
					case START:
						return nextWordEnd(p, Direction::BACKWARD, words);
					case END:
						return nextWordEnd(p, Direction::FORWARD, words);
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
			boost::optional<kernel::Position> rightWord(const PointProxy& p, Index words /* = 1 */) {
				return (defaultUIReadingDirection(p) == LEFT_TO_RIGHT) ? forwardWord(p, words) : backwardWord(p, words);
			}

			/**
			 * Returns the end of the word where advanced to the right by N words.
			 * @param p The base position
			 * @param words The number of words to adavance
			 * @return The destination
			 */
			boost::optional<kernel::Position> rightWordEnd(const PointProxy& p, Index words /* = 1 */) {
				return (defaultUIReadingDirection(p) == LEFT_TO_RIGHT) ? forwardWordEnd(p, words) : backwardWordEnd(p, words);
			}
#endif // ASCENSION_ABANDONED_AT_VERSION_08
		}
	}
}
