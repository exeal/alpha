/**
 * @file visual-point.cpp
 * @author exeal
 * @date 2003-2008 was point.cpp
 * @date 2008-2010 separated from point.cpp
 * @date 2011-10-02 separated from caret.cpp
 * @date 2011-2014
 */

#include <ascension/graphics/font/font-metrics.hpp>
#include <ascension/graphics/rendering-context.hpp>
#include <ascension/viewer/viewer.hpp>
#include <ascension/viewer/visual-point.hpp>
#include <ascension/corelib/text/identifier-syntax.hpp>

namespace ascension {
	namespace detail {
		// detail.VisualDestinationProxyMaker /////////////////////////////////////////////////////////////////////////

		class VisualDestinationProxyMaker {
		public:
			static viewers::VisualDestinationProxy make(const kernel::Position& p, bool crossVisualLines) {
				return viewers::VisualDestinationProxy(p, crossVisualLines);
			}
		};
	}

	namespace viewers {
		namespace utils {
			/**
			 * Centers the current visual line addressed by the given visual point in the text viewer by vertical scrolling
			 * the window.
			 * @param p The visual point
			 * @throw DocumentDisposedException 
			 * @throw TextViewerDisposedException 
			 */
			void recenter(VisualPoint& p) {
				// TODO: not implemented.
			}

			/**
			 * Scrolls the text viewer until the given point is visible in the window.
			 * @param p The visual point. This position will be normalized before the process
			 * @throw DocumentDisposedException 
			 * @throw TextViewerDisposedException 
			 */
			void show(VisualPoint& p) {
				TextViewer& viewer = p.textViewer();
				const graphics::font::TextRenderer& renderer = viewer.textRenderer();
				const std::shared_ptr<graphics::font::TextViewport> viewport(viewer.textRenderer().viewport());
				const kernel::Position np(p.normalized());
				const float visibleLines = viewport->numberOfVisibleLines();
				presentation::AbstractTwoAxes<boost::optional<graphics::font::TextViewport::ScrollOffset>> to;	// scroll destination
				to.bpd() = viewport->firstVisibleLineInVisualNumber();
				to.ipd() = viewport->inlineProgressionOffset();

				// scroll if the point is outside of 'before-edge' or 'after-edge'
				to.bpd() = std::min(p.visualLine(), *to.bpd());
				to.bpd() = std::max(p.visualLine() - static_cast<graphics::font::TextViewport::ScrollOffset>(viewport->numberOfVisibleLines()) + 1, *to.bpd());

				// scroll if the point is outside of 'start-edge' or 'end-edge'
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
//				if(!viewer.configuration().lineWrap.wrapsAtWindowEdge()) {
					const graphics::font::Font::Metrics& fontMetrics = renderer.defaultFont()->metrics();
					const Index visibleColumns = viewport->numberOfVisibleCharactersInLine();
					const font::TextLayout& lineLayout = renderer.layouts().at(np.line);
					const graphics::Scalar x = geometry::x(lineLayout.location(np.offsetInLine, font::TextLayout::LEADING)) + font::lineIndent(lineLayout, viewport->contentMeasure(), 0);
					const graphics::Scalar scrollOffset = viewer.horizontalScrollBar().position() * viewer.scrollRate(true) * fontMetrics.averageCharacterWidth();
					if(x <= scrollOffset)	// point is beyond left side of the viewport
						geometry::x(to) = x / fontMetrics.averageCharacterWidth() - visibleColumns / 4;
					else if(x >= static_cast<graphics::Scalar>((viewer.horizontalScrollBar().position()	// point is beyond right side of the viewport
							* viewer.scrollRate(true) + visibleColumns) * fontMetrics.averageCharacterWidth()))
						geometry::x(to) = x / fontMetrics.averageCharacterWidth() - visibleColumns * 3 / 4;
					if(geometry::x(to) < -1)
						geometry::x(to) = 0;
//				}
#else
				const graphics::font::TextLayout& layout = renderer.layouts().at(np.line);
				// TODO: Replace font.lineIndent with TextRenderer.lineStartEdge.
				const Index pointIpd = static_cast<Index>(
					(graphics::font::lineIndent(layout, viewport->contentMeasure()) + layout.hitToPoint(graphics::font::TextHit<>::leading(np.offsetInLine)).ipd())
					/ widgetapi::createRenderingContext(viewer)->fontMetrics(renderer.defaultFont())->averageCharacterWidth());
				to.ipd() = std::min(pointIpd, *to.ipd());
				to.ipd() = std::max(pointIpd - static_cast<Index>(viewport->numberOfVisibleCharactersInLine()) + 1, *to.ipd());
#endif // ASCENSION_ABANDONED_AT_VERSION_08
				viewport->scrollTo(to);
			}
		}	// namespace utils


		// TextViewerDisposedException ////////////////////////////////////////////////////////////////////////////////

		TextViewerDisposedException::TextViewerDisposedException() :
				logic_error("The text viewer the object connecting to has been disposed.") {
		}


		// VisualPoint ////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef ASCENSION_ABANDONED_AT_VERSION_08
		/**
		 * Constructor.
		 * @param viewer The viewer
		 * @param listener The listener. can be @c null
		 * @throw BadPositionException @a position is outside of the document
		 */
		VisualPoint::VisualPoint(TextViewer& viewer, PointListener* listener /* = nullptr */) :
				Point(viewer.document(), listener), viewer_(&viewer), crossingLines_(false) {
			static_cast<detail::PointCollection<VisualPoint>&>(viewer).addNewPoint(*this);
			viewer_->textRenderer().layouts().addVisualLinesListener(*this);
		}
#endif // ASCENSION_ABANDONED_AT_VERSION_08

		/**
		 * Constructor.
		 * @param viewer The viewer
		 * @param position The initial position of the point
		 * @param listener The listener. can be @c null
		 * @throw BadPositionException @a position is outside of the document
		 */
		VisualPoint::VisualPoint(TextViewer& viewer, const kernel::Position& position, kernel::PointListener* listener /* = nullptr */) :
				Point(viewer.document(), position, listener), viewer_(&viewer), crossingLines_(false) {
			static_cast<detail::PointCollection<VisualPoint>&>(viewer).addNewPoint(*this);
			viewer_->textRenderer().layouts().addVisualLinesListener(*this);
		}

			/**
			 * Copy-constructor.
			 * @param other The source object
			 * @throw DocumentDisposedException The document to which @a other belongs had been disposed
			 * @throw TextViewerDisposedException The text viewer to which @a other belongs had been disposed
			 */
			VisualPoint::VisualPoint(const VisualPoint& other) : Point(other), viewer_(other.viewer_),
					positionInVisualLine_(other.positionInVisualLine_), crossingLines_(false), lineNumberCaches_(other.lineNumberCaches_) {
				if(viewer_ == nullptr)
					throw TextViewerDisposedException();
				static_cast<detail::PointCollection<VisualPoint>*>(viewer_)->addNewPoint(*this);
				viewer_->textRenderer().layouts().addVisualLinesListener(*this);
			}

			/// Destructor.
			VisualPoint::~VisualPoint() BOOST_NOEXCEPT {
				if(viewer_ != nullptr) {
					static_cast<detail::PointCollection<VisualPoint>*>(viewer_)->removePoint(*this);
					viewer_->textRenderer().layouts().removeVisualLinesListener(*this);
				}
			}

			/// @see Point#aboutToMove
			void VisualPoint::aboutToMove(kernel::Position& to) {
				if(isTextViewerDisposed())
					throw TextViewerDisposedException();
				Point::aboutToMove(to);
			}

			/// @see Point#moved
			void VisualPoint::moved(const kernel::Position& from) {
				if(isTextViewerDisposed())
					return;
				if(from.line == line(*this) && lineNumberCaches_) {
					const graphics::font::TextLayout* const layout = viewer_->textRenderer().layouts().at(kernel::line(*this));
					lineNumberCaches_->visualLine -= lineNumberCaches_->visualSubline;
					lineNumberCaches_->visualSubline = (layout != nullptr) ? layout->lineAt(offsetInLine(*this)) : 0;
					lineNumberCaches_->visualLine += lineNumberCaches_->visualSubline;
				} else
					lineNumberCaches_ = boost::none;
				Point::moved(from);
				if(!crossingLines_)
					positionInVisualLine_ = boost::none;
			}
#if 0
		/**
		 * Inserts the spcified text as a rectangle at the current position. This method has two
		 * restrictions as the follows:
		 * - If the text viewer is line wrap mode, this method inserts text as linear not rectangle.
		 * - If the destination line is bidirectional, the insertion may be performed incorrectly.
		 * @param first The start of the text
		 * @param last The end of the text
		 * @throw NullPointerException @a first and/or @a last are @c null
		 * @throw std#invalid_argument @a first &gt; @a last
		 * @throw ... Any exceptions @c Document#insert throws
		 * @see kernel#EditPoint#insert
		 */
		void VisualPoint::insertRectangle(const Char* first, const Char* last) {
			verifyViewer();

			// HACK: 
			if(textViewer().configuration().lineWrap.wraps())
				return insert(first, last);

			if(first == nullptr)
				throw NullPointerException("first");
			else if(last == nullptr)
				throw NullPointerException("last");
			else if(first > last)
				throw invalid_argument("first > last");
			else if(first == last)
				return;

			Document& doc = *document();
			DocumentLocker lock(doc);

			const Index numberOfLines = doc.numberOfLines();
			Index line = lineNumber();
			const TextRenderer& renderer = textViewer().textRenderer();
			const int x = renderer.lineLayout(line).location(columnNumber()).x + renderer.lineIndent(line, 0);
			const DocumentInput* const documentInput = doc.input();
			const String newline(getNewlineString((documentInput != nullptr) ? documentInput->newline() : ASCENSION_DEFAULT_NEWLINE));
			for(const Char* bol = first; ; ++line) {
				// find the next EOL
				const Char* const eol = find_first_of(bol, last, NEWLINE_CHARACTERS, ASCENSION_ENDOF(NEWLINE_CHARACTERS));

				// insert text if the source line is not empty
				if(eol > bol) {
					const LineLayout& layout = renderer.lineLayout(line);
					const Index offsetInLine = layout.offset(x - renderer.lineIndent(line), 0);
					String s(layout.fillToX(x));
					s.append(bol, eol);
					if(line >= numberOfLines - 1)
						s.append(newline);
					doc.insert(Position(line, offsetInLine), s);	// this never throw
				}

				if(eol == last)
					break;
				bol = eol + ((eol[0] == CARRIAGE_RETURN && eol < last - 1 && eol[1] == LINE_FEED) ? 2 : 1);
			}
		}
#endif
		/// @internal @c Point#moveTo for @c BlockProgressionDestinationProxy.
		void VisualPoint::moveTo(const VisualDestinationProxy& to) {
			if(!to.crossesVisualLines()) {
				moveTo(to.position());
				return;
			}
			if(!positionInVisualLine_)
				rememberPositionInVisualLine();
			crossingLines_ = true;
			try {
				moveTo(to.position());
			} catch(...) {
				crossingLines_ = false;
				throw;
			}
			crossingLines_ = false;
		}

		/// Returns the offset of the point in the visual line.
		Index VisualPoint::offsetInVisualLine() const {
			if(!positionInVisualLine_)
				const_cast<VisualPoint*>(this)->rememberPositionInVisualLine();
			const TextViewer::Configuration& c = textViewer().configuration();
			const graphics::font::TextRenderer& renderer = textViewer().textRenderer();
//	if(resolveTextAlignment(c.alignment, c.readingDirection) != ALIGN_RIGHT)
				return static_cast<Index>(*positionInVisualLine_
					/ widgetapi::createRenderingContext(textViewer())->fontMetrics(renderer.defaultFont())->averageCharacterWidth());
//	else
//		return (renderer.width() - positionInVisualLine_) / renderer.averageCharacterWidth();
		}

		/// Updates @c positionInVisualLine_ with the current position.
		inline void VisualPoint::rememberPositionInVisualLine() {
			// positionInVisualLine_ is distance from left/top-edge of content-area to the point
			assert(!crossingLines_);
			if(isTextViewerDisposed())
				throw TextViewerDisposedException();
			if(!isDocumentDisposed()) {
				const graphics::font::TextRenderer& renderer = textViewer().textRenderer();
				const graphics::font::TextLayout& layout = renderer.layouts().at(line(*this));
				positionInVisualLine_ =
					graphics::font::lineStartEdge(layout, renderer.viewport()->contentMeasure())
					+ layout.hitToPoint(graphics::font::TextHit<>::leading(offsetInLine(*this))).ipd();
			}
		}

		/// Returns the visual line number.
		Index VisualPoint::visualLine() const {
			if(lineNumberCaches_) {
				VisualPoint& self = const_cast<VisualPoint&>(*this);
				const kernel::Position p(normalized());
				self.lineNumberCaches_->visualLine = textViewer().textRenderer().layouts().mapLogicalLineToVisualLine(p.line);
				self.lineNumberCaches_->visualSubline = textViewer().textRenderer().layouts().at(p.line).lineAt(p.offsetInLine);
				self.lineNumberCaches_->visualLine += lineNumberCaches_->visualSubline;
			}
			return lineNumberCaches_->visualLine;
		}

		/// @see VisualLinesListener#visualLinesDeleted
		void VisualPoint::visualLinesDeleted(const boost::integer_range<Index>& lines, Index, bool) BOOST_NOEXCEPT {
			if(!adaptsToDocument() && includes(lines, line(*this)))
				lineNumberCaches_ = boost::none;
		}

		/// @see VisualLinesListener#visualLinesInserted
		void VisualPoint::visualLinesInserted(const boost::integer_range<Index>& lines) BOOST_NOEXCEPT {
			if(!adaptsToDocument() && includes(lines, line(*this)))
				lineNumberCaches_ = boost::none;
		}

		/// @see VisualLinesListener#visualLinesModified
		void VisualPoint::visualLinesModified(const boost::integer_range<Index>& lines, SignedIndex sublineDifference, bool, bool) BOOST_NOEXCEPT {
			if(lineNumberCaches_) {
				// adjust visualLine_ and visualSubine_ according to the visual lines modification
				if(*lines.end() <= line(*this))
					lineNumberCaches_->visualLine += sublineDifference;
				else if(*lines.begin() == line(*this)) {
					lineNumberCaches_->visualLine -= lineNumberCaches_->visualSubline;
					lineNumberCaches_->visualSubline =
						textViewer().textRenderer().layouts().at(line(*this)).lineAt(std::min(offsetInLine(*this), document().lineLength(line(*this))));
					lineNumberCaches_->visualLine += lineNumberCaches_->visualSubline;
				} else if(*lines.begin() < line(*this))
					lineNumberCaches_ = boost::none;
			}
		}
	}

	namespace kernel {
		namespace locations {
			// kernel.locations free functions ////////////////////////////////////////////////////////////////////////

#ifdef ASCENSION_ABANDONED_AT_VERSION_08
			/**
			 * Returns the position returned by N pages.
			 * @param p The base position
			 * @param pages The number of pages to return
			 * @return The destination
			 */
			viewers::VisualDestinationProxy backwardPage(const VisualPoint& p, Index pages /* = 1 */) {
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
			viewers::VisualDestinationProxy backwardVisualLine(const VisualPoint& p, Index lines /* = 1 */) {
				Position np(p.normalized());
				const graphics::font::TextRenderer& renderer = p.textViewer().textRenderer();
				Index subline = renderer.layouts().at(np.line).lineAt(np.offsetInLine);
				if(np.line == 0 && subline == 0)
					return detail::VisualDestinationProxyMaker::make(np, true);
				graphics::font::VisualLine visualLine(np.line, subline);
				renderer.layouts().offsetVisualLine(visualLine, -static_cast<SignedIndex>(lines));
				const graphics::font::TextLayout& layout = renderer.layouts().at(visualLine.line);
				if(!p.positionInVisualLine_)
					const_cast<VisualPoint&>(p).rememberPositionInVisualLine();
				const graphics::Scalar ipd = *p.positionInVisualLine_ - graphics::font::lineStartEdge(layout, renderer.viewport()->contentMeasure());
				const graphics::Scalar bpd = layout.baseline(visualLine.subline);
				np.offsetInLine = layout.offset(
					isHorizontal(layout.writingMode().blockFlowDirection) ?
						geometry::make<NativePoint>(ipd, bpd) : geometry::make<NativePoint>(bpd, ipd)).second;
				if(layout.lineAt(np.offsetInLine) != visualLine.subline)
					np = nextCharacter(p.document(), np, Direction::BACKWARD, GRAPHEME_CLUSTER);
				return detail::VisualDestinationProxyMaker::make(np, true);
			}
#endif // ASCENSION_ABANDONED_AT_VERSION_08

			/**
			 * Returns the beginning of the visual line.
			 * @param p The base position
			 * @return The destination
			 * @see EditPoint#beginningOfLine
			 */
			Position beginningOfVisualLine(const viewers::VisualPoint& p) {
				const Position np(p.normalized());
				const graphics::font::TextLayout& layout = p.textViewer().textRenderer().layouts().at(np.line);
				return Position(np.line, layout.lineOffset(layout.lineAt(np.offsetInLine)));
			}

			/**
			 * Returns the beginning of the line or the first printable character in the line by context.
			 * @param p The base position
			 * @return The destination
			 */
			Position contextualBeginningOfLine(const viewers::VisualPoint& p) {
				return isFirstPrintableCharacterOfLine(p) ? beginningOfLine(p) : firstPrintableCharacterOfLine(p);
			}

			/**
			 * Moves to the beginning of the visual line or the first printable character in the visual line by
			 * context.
			 * @param p The base position
			 * @return The destination
			 */
			Position contextualBeginningOfVisualLine(const viewers::VisualPoint& p) {
				return isFirstPrintableCharacterOfLine(p) ?
					beginningOfVisualLine(p) : firstPrintableCharacterOfVisualLine(p);
			}

			/**
			 * Moves to the end of the line or the last printable character in the line by context.
			 * @param p The base position
			 * @return The destination
			 */
			Position contextualEndOfLine(const viewers::VisualPoint& p) {
				return isLastPrintableCharacterOfLine(p) ? endOfLine(p) : lastPrintableCharacterOfLine(p);
			}

			/**
			 * Moves to the end of the visual line or the last printable character in the visual line by
			 * context.
			 * @param p The base position
			 * @return The destination
			 */
			Position contextualEndOfVisualLine(const viewers::VisualPoint& p) {
				return isLastPrintableCharacterOfLine(p) ?
					endOfVisualLine(p) : lastPrintableCharacterOfVisualLine(p);
			}

			/**
			 * Returns the end of the visual line.
			 * @param p The base position
			 * @return The destination
			 * @see EditPoint#endOfLine
			 */
			Position locations::endOfVisualLine(const viewers::VisualPoint& p) {
				Position np(p.normalized());
				const graphics::font::TextLayout& layout = p.textViewer().textRenderer().layouts().at(np.line);
				const Index subline = layout.lineAt(np.offsetInLine);
				np.offsetInLine = (subline < layout.numberOfLines() - 1) ?
					layout.lineOffset(subline + 1) : p.document().lineLength(np.line);
				if(layout.lineAt(np.offsetInLine) != subline)
					np = nextCharacter(p.document(), np, Direction::BACKWARD, GRAPHEME_CLUSTER);
				return np;
			}

			/**
			 * Returns the first printable character in the line.
			 * @param p The base position
			 * @return The destination
			 */
			Position locations::firstPrintableCharacterOfLine(const viewers::VisualPoint& p) {
				Position np(p.normalized());
				const Char* const s = p.document().line(np.line).data();
				np.offsetInLine = detail::identifierSyntax(p).eatWhiteSpaces(s, s + p.document().lineLength(np.line), true) - s;
				return np;
			}

			/**
			 * Returns the first printable character in the visual line.
			 * @param p The base position
			 * @return The destination
			 */
			Position firstPrintableCharacterOfVisualLine(const viewers::VisualPoint& p) {
				Position np(p.normalized());
				const String& s = p.document().line(np.line);
				const graphics::font::TextLayout& layout = p.textViewer().textRenderer().layouts().at(np.line);
				const Index subline = layout.lineAt(np.offsetInLine);
				np.offsetInLine = detail::identifierSyntax(p).eatWhiteSpaces(
					s.begin() + layout.lineOffset(subline),
					s.begin() + ((subline < layout.numberOfLines() - 1) ?
						layout.lineOffset(subline + 1) : s.length()), true) - s.begin();
				return np;
			}

#ifdef ASCENSION_ABANDONED_AT_VERSION_08
			/**
			 * Returns the position advanced by N pages.
			 * @param p The base position
			 * @param pages The number of pages to advance
			 * @return The destination
			 */
			viewers::VisualDestinationProxy forwardPage(const viewers::VisualPoint& p, Index pages /* = 1 */) {
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
			viewers::VisualDestinationProxy forwardVisualLine(const viewers::VisualPoint& p, Index lines /* = 1 */) {
				Position np(p.normalized());
				const graphics::font::TextRenderer& renderer = p.textViewer().textRenderer();
				const graphics::font::TextLayout* layout = &renderer.layouts().at(np.line);
				Index subline = layout->lineAt(np.offsetInLine);
				if(np.line == p.document().numberOfLines() - 1 && subline == layout->numberOfLines() - 1)
					return detail::VisualDestinationProxyMaker::make(np, true);
				graphics::font::VisualLine visualLine(np.line, subline);
				renderer.layouts().offsetVisualLine(visualLine, static_cast<SignedIndex>(lines));
				layout = &renderer.layouts().at(visualLine.line);
				if(!p.positionInVisualLine_)
					const_cast<VisualPoint&>(p).rememberPositionInVisualLine();
				const graphics::Scalar ipd = *p.positionInVisualLine_ - graphics::font::lineStartEdge(*layout, renderer.viewport()->contentMeasure());
				const graphics::Scalar bpd = layout->baseline(visualLine.subline);
				np.offsetInLine = layout->offset(
					isHorizontal(layout->writingMode().blockFlowDirection) ?
						geometry::make<NativePoint>(ipd, bpd) : geometry::make<NativePoint>(bpd, ipd)).second;
				if(layout->lineAt(np.offsetInLine) != visualLine.subline)
					np = nextCharacter(p.document(), np, Direction::BACKWARD, GRAPHEME_CLUSTER);

				return detail::VisualDestinationProxyMaker::make(np, true);
			}
#endif // ASCENSION_ABANDONED_AT_VERSION_08

			/**
			 * Returns @c true if the point is the beginning of the visual line.
			 * @param p The base position
			 * @see EditPoint#isBeginningOfLine
			 */
			bool isBeginningOfVisualLine(const viewers::VisualPoint& p) {
				if(isBeginningOfLine(p))	// this considers narrowing
					return true;
				const Position np(p.normalized());
				const graphics::font::TextLayout& layout = p.textViewer().textRenderer().layouts().at(np.line);
				return np.offsetInLine == layout.lineOffset(layout.lineAt(np.offsetInLine));
			}

			/**
			 * Returns @c true if the point is end of the visual line.
			 * @param p The base position
			 * @see kernel#EditPoint#isEndOfLine
			 */
			bool isEndOfVisualLine(const viewers::VisualPoint& p) {
				if(isEndOfLine(p))	// this considers narrowing
					return true;
				const Position np(p.normalized());
				const graphics::font::TextLayout& layout = p.textViewer().textRenderer().layouts().at(np.line);
				const Index subline = layout.lineAt(np.offsetInLine);
				return np.offsetInLine == layout.lineOffset(subline) + layout.lineLength(subline);
			}

			/// Returns @c true if the given position is the first printable character in the line.
			bool isFirstPrintableCharacterOfLine(const viewers::VisualPoint& p) {
				const Position np(p.normalized()), bob(p.document().accessibleRegion().first);
				const Index offset = (bob.line == np.line) ? bob.offsetInLine : 0;
				const String& line = p.document().line(np.line);
				return line.data() + np.offsetInLine - offset
					== detail::identifierSyntax(p).eatWhiteSpaces(line.data() + offset, line.data() + line.length(), true);
			}

			/// Returns @c true if the given position is the first printable character in the visual line.
			bool isFirstPrintableCharacterOfVisualLine(const viewers::VisualPoint& p) {
				// TODO: not implemented.
				return false;
			}

			/// Returns @c true if the given position is the last printable character in the line.
			bool isLastPrintableCharacterOfLine(const viewers::VisualPoint& p) {
				const Position np(p.normalized()), eob(p.document().accessibleRegion().second);
				const String& line = p.document().line(np.line);
				const Index lineLength = (eob.line == np.line) ? eob.offsetInLine : line.length();
				return line.data() + lineLength - np.offsetInLine
					== detail::identifierSyntax(p).eatWhiteSpaces(line.data() + np.offsetInLine, line.data() + lineLength, true);
			}

			/// Returns @c true if the given position is the last printable character in the visual line.
			bool isLastPrintableCharacterOfVisualLine(const viewers::VisualPoint& p) {
				// TODO: not implemented.
				return false;
			}

			/**
			 * Returns the last printable character in the line.
			 * @param p The base position
			 * @return The destination
			 */
			Position lastPrintableCharacterOfLine(const viewers::VisualPoint& p) {
				Position np(p.normalized());
				const String& s(p.document().line(np.line));
				const text::IdentifierSyntax& syntax = detail::identifierSyntax(p);
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
			Position lastPrintableCharacterOfVisualLine(const viewers::VisualPoint& p) {
				// TODO: not implemented.
				return p.normalized();
			}

#ifdef ASCENSION_ABANDONED_AT_VERSION_08
			namespace {
				inline ReadingDirection defaultUIReadingDirection(const viewers::VisualPoint& p) {
					return p.textViewer().textRenderer().defaultUIWritingMode().inlineFlowDirection;
				}
			}

			/**
			 * Returns the beginning of the word where advanced to the left by N words.
			 * @param p The base position
			 * @param words The number of words to adavance
			 * @return The destination, or @c boost#none if the writing mode is vertical
			 */
			boost::optional<Position> leftWord(const viewers::VisualPoint& p, Index words /* = 1 */) {
				return (defaultUIReadingDirection(p) == LEFT_TO_RIGHT) ? backwardWord(p, words) : forwardWord(p, words);
			}

			/**
			 * Returns the end of the word where advanced to the left by N words.
			 * @param p The base position
			 * @param words The number of words to adavance
			 * @return The destination, or @c boost#none if the writing mode is vertical
			 */
			boost::optional<Position> leftWordEnd(const viewers::VisualPoint& p, Index words /* = 1 */) {
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
			boost::optional<Position> nextBookmarkInPhysicalDirection(
					const viewers::VisualPoint& p, graphics::PhysicalDirection direction, Index marks /* = 1 */) {
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
			viewers::VisualDestinationProxy nextCharacterInPhysicalDirection(
					const VisualPoint& p, PhysicalDirection direction, CharacterUnit unit, Index characters /* = 1 */) {
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

			/**
			 * Returns the position advanced/returned by N pages.
			 * @param p The base position
			 * @param direction The direction
			 * @param pages The number of pages to advance/return
			 * @return The destination
			 */
			viewers::VisualDestinationProxy nextPage(const viewers::VisualPoint& p, Direction direction, Index pages /* = 1 */) {
				Index lines = 0;
				const std::shared_ptr<const graphics::font::TextViewport> viewport(p.textViewer().textRenderer().viewport());
				// TODO: calculate exact number of visual lines.
				lines = static_cast<Index>(viewport->numberOfVisibleLines() * pages);

				return nextVisualLine(p, direction, lines);
			}

			/**
			 * Returns the position advanced/returned by N visual lines.
			 * @param p The base position
			 * @param lines The number of the visual lines to advance/return
			 * @return The destination
			 * @see #nextLine
			 */
			viewers::VisualDestinationProxy nextVisualLine(const viewers::VisualPoint& p, Direction direction, Index lines /* = 1 */) {
				Position np(p.normalized());
				const graphics::font::TextRenderer& renderer = p.textViewer().textRenderer();
				Index subline;
				if(direction == Direction::FORWARD) {
					const graphics::font::TextLayout& layout = renderer.layouts().at(np.line);
					subline = layout.lineAt(np.offsetInLine);
					if(np.line == p.document().numberOfLines() - 1 && subline == layout.numberOfLines() - 1)
						return detail::VisualDestinationProxyMaker::make(np, true);
				} else {
					subline = renderer.layouts().at(np.line).lineAt(np.offsetInLine);
					if(np.line == 0 && subline == 0)
						return detail::VisualDestinationProxyMaker::make(np, true);
				}
				graphics::font::VisualLine visualLine(np.line, subline);
				renderer.layouts().offsetVisualLine(visualLine,
					(direction == Direction::FORWARD) ? static_cast<SignedIndex>(lines) : -static_cast<SignedIndex>(lines));
				const graphics::font::TextLayout& layout = renderer.layouts().at(visualLine.line);
				if(!p.positionInVisualLine_)
					const_cast<viewers::VisualPoint&>(p).rememberPositionInVisualLine();
				np.offsetInLine = layout.hitTestCharacter(
					presentation::AbstractTwoAxes<graphics::Scalar>(
						presentation::_ipd = *p.positionInVisualLine_ - graphics::font::lineStartEdge(layout, renderer.viewport()->contentMeasure()),
						presentation::_bpd = layout.lineMetrics(visualLine.subline).baselineOffset())).insertionIndex();
				if(layout.lineAt(np.offsetInLine) != visualLine.subline)
					np = nextCharacter(p.document(), np, Direction::BACKWARD, GRAPHEME_CLUSTER);
				return detail::VisualDestinationProxyMaker::make(np, true);
			}

#ifdef ASCENSION_ABANDONED_AT_VERSION_08
			/**
			 * Returns the beginning of the word where advanced to the left/right/top/bottom by N words.
			 * @param p The base position
			 * @param words The number of words to adavance
			 * @return The destination, or @c boost#none if @a direction is block-progression
			 * @see #nextWord, #nextWordEndInPhysicalDirection
			 */
			boost::optional<Position> nextWordInPhysicalDirection(
					const viewers::VisualPoint& p, PhysicalDirection direction, Index words /* = 1 */) {
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
			boost::optional<Position> nextWordEndInPhysicalDirection(
					const viewers::VisualPoint& p, PhysicalDirection direction, Index words /* = 1 */) {
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
			boost::optional<Position> rightWord(const viewers::VisualPoint& p, Index words /* = 1 */) {
				return (defaultUIReadingDirection(p) == LEFT_TO_RIGHT) ? forwardWord(p, words) : backwardWord(p, words);
			}

			/**
			 * Returns the end of the word where advanced to the right by N words.
			 * @param p The base position
			 * @param words The number of words to adavance
			 * @return The destination
			 */
			boost::optional<Position> rightWordEnd(const viewers::VisualPoint& p, Index words /* = 1 */) {
				return (defaultUIReadingDirection(p) == LEFT_TO_RIGHT) ? forwardWordEnd(p, words) : backwardWordEnd(p, words);
			}
#endif // ASCENSION_ABANDONED_AT_VERSION_08
		}
	}
}
