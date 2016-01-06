/**
 * @file visual-point.cpp
 * @author exeal
 * @date 2003-2008 was point.cpp
 * @date 2008-2010 separated from point.cpp
 * @date 2011-10-02 separated from caret.cpp
 * @date 2011-2015
 */

#include <ascension/corelib/text/identifier-syntax.hpp>
#include <ascension/kernel/document.hpp>
#include <ascension/graphics/font/font-metrics.hpp>
#include <ascension/graphics/font/line-layout-vector.hpp>
#include <ascension/graphics/font/text-layout.hpp>
#include <ascension/graphics/font/text-viewport.hpp>
#include <ascension/graphics/rendering-context.hpp>
#include <ascension/viewer/text-area.hpp>
#include <ascension/viewer/text-viewer.hpp>
#include <ascension/viewer/visual-point.hpp>

namespace ascension {
	namespace viewer {
		namespace detail {
			// detail.VisualDestinationProxyMaker /////////////////////////////////////////////////////////////////////////

			class VisualDestinationProxyMaker {
			public:
				static viewer::VisualDestinationProxy make(const kernel::Position& p, bool crossVisualLines) {
					return viewer::VisualDestinationProxy(p, crossVisualLines);
				}
			};
		}

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
				TextArea& textArea = p.textArea();
				graphics::font::TextRenderer& renderer = textArea.textRenderer();
				const std::shared_ptr<graphics::font::TextViewport> viewport(renderer.viewport());
				const kernel::Position np(p.normalized());
				const graphics::font::TextLayout& layout = renderer.layouts().at(
					np.line, graphics::font::LineLayoutVector::USE_CALCULATED_LAYOUT);	// this call may change the layouts
				const float visibleLines = viewport->numberOfVisibleLines();
				presentation::FlowRelativeTwoAxes<graphics::font::TextViewport::ScrollOffset> to;	// scroll destination

				// scroll if the point is outside of 'before-edge'
				to.bpd() = std::min(p.visualLine().line, viewport->scrollPositions().bpd());
				// scroll if the point is outside of 'after-edge'
				if(p.visualLine().line > static_cast<graphics::font::TextViewport::ScrollOffset>(visibleLines))
					to.bpd() = std::max(p.visualLine().line - static_cast<graphics::font::TextViewport::ScrollOffset>(visibleLines) + 1, to.bpd());

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
				// TODO: Replace font.lineIndent with TextRenderer.lineStartEdge.
#	if 1
				const Index pointIpd = static_cast<Index>(
					(graphics::font::lineIndent(layout, viewport->contentMeasure()) + layout.hitToPoint(graphics::font::TextHit<>::leading(np.offsetInLine)).ipd()));
				to.ipd() = std::min(pointIpd, viewport->scrollPositions().ipd());
				to.ipd() = std::max(pointIpd - static_cast<Index>(graphics::font::pageSize<presentation::ReadingDirection>(*viewport)) + 1, to.ipd());
#	else
				const Index pointIpd = static_cast<Index>(
					(graphics::font::lineIndent(layout, viewport->contentMeasure()) + layout.hitToPoint(graphics::font::TextHit<>::leading(np.offsetInLine)).ipd())
					/ widgetapi::createRenderingContext(viewer)->fontMetrics(renderer.defaultFont())->averageCharacterWidth());
				to.ipd() = std::min(pointIpd, viewport->scrollPositions().ipd());
				to.ipd() = std::max(pointIpd - static_cast<Index>(viewport->numberOfVisibleCharactersInLine()) + 1, to.ipd());
#	endif
#endif // ASCENSION_ABANDONED_AT_VERSION_08
				viewport->scrollTo(presentation::makeFlowRelativeTwoAxes((
					presentation::_bpd = boost::make_optional(to.bpd()), presentation::_ipd = boost::make_optional(to.ipd()))));
			}
		}	// namespace utils


		// VisualPoint ////////////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * @class ascension::viewer::VisualPoint
		 * Extension of @c kernel#Point class for viewer and layout.
		 *
		 * A @c VisualPoint has the following three states:
		 * <dl>
		 *   <dt>State 1 : Constructed but not installed by @c TextViewer</dt>
		 *   <dd>
		 *     - The @c VisualPoint has been just constructed with the document, but not be installed by a
		 *       @c TextViewer.
		 *     - The all features only as @c kernel#Point are available. The other methods throw
		 *       @c VisualPoint#NotInstalledException exception.
		 *     - @c VisualPoint#isInstalled() returns @c false.
		 *     - @c VisualPoint#isTextViewerDisposed() throws @c VisualPoint#NotInstalledException exception.
		 *     - @c VisualPoint#isFullyAvailable() returns @c false.
		 *     - Constructed by the constructor which takes a document and a position.
		 *   </dd>
		 *   <dt>State 2 : Installed by @c TextViewer</dt>
		 *   <dd>
		 *     - The @c VisualPoint has been installed by the @c TextViewer.
		 *     - The all features are available.
		 *     - @c VisualPoint#isInstalled() returns @c true.
		 *     - @c VisualPoint#isTextViewerDisposed() returns @c false.
		 *     - @c VisualPoint#isFullyAvailable() returns @c true.
		 *     - Constructed by the constructor which takes a text viewer and a position.
		 *     - You can revert the @c VisualPoint to the state 1 by calling @c VisualPoint#uninstall method.
		 *   </dd>
		 *   <dt>State 3 : Installed @c TextViewer was disposed</dt>
		 *   <dd>
		 *     - The @c TextViewer which installed the @c VisualPoint has already disposed.
		 *     - The all features only as @c kernel#Point are available. The other methods throw
		 *       @c VisualPoint#TextViewerDisposedException exception.
		 *     - @c VisualPoint#isInstalled() returns @c true.
		 *     - @c VisualPoint#isTextViewerDisposed() returns @c true.
		 *     - @c VisualPoint#isFullyAvailable() returns @c false.
		 *     - You can revert the @c VisualPoint to the state 1 by calling @c VisualPoint#uninstall method.
		 *   </dd>
		 * </dl>
		 *
		 * @see kernel#Point, TextViewer
		 */

		/**
		 * Creates a @c VisualPoint object but does not install on @c TextViewer.
		 * @param document The document
		 * @param position The initial position of the point
		 * @throw kernel#BadPositionException The constructor of @c kernel#Point class threw this exception
		 * @post @c #isInstalled() returns @c false.
		 * @post @c #isTextViewerDisposed() throws @c #NotInstalledException exception.
		 * @post @c #isFullyAvailable() returns @c false.
		 */
		VisualPoint::VisualPoint(kernel::Document& document,
				const kernel::Position& position /* = kernel::Position::zero() */) : Point(document, position), crossingLines_(false) {
		}

		/**
		 * Creates a @c VisualPoint object and installs on the specified @c TextViewer.
		 * @param textArea The text area
		 * @param position The initial position of the point
		 * @throw kernel#BadPositionException The constructor of @c kernel#Point class threw this exception
		 * @post @c #isInstalled() returns @c true.
		 * @post @c #isTextViewerDisposed() returns @c false.
		 * @post @c #isFullyAvailable() returns @c true.
		 */
		VisualPoint::VisualPoint(TextArea& textArea, const kernel::Position& position /* = kernel::Position::zero() */)
				: Point(textArea.textViewer().document(), position), crossingLines_(false) {
			install(textArea);
		}

		/**
		 * Creates a @c VisualPoint object but does not install on @c TextViewer.
		 * @param other The point used to initialize kernel part of the new object
		 * @throw kernel#BadPositionException The constructor of @c kernel#Point class threw this exception
		 * @post @c #isInstalled() returns @c false.
		 * @post @c #isTextViewerDisposed() throws @c #NotInstalledException exception.
		 * @post @c #isFullyAvailable() returns @c false.
		 */
		VisualPoint::VisualPoint(const kernel::Point& other) : Point(other), crossingLines_(false) {
		}

		/**
		 * Copy-constructor.
		 * @param other The source object. If this has been installed, the new point is also installed
		 * @throw kernel#DocumentDisposedException The copy-constructor of @c kernel#Point threw this exception
		 * @throw TextViewerDisposedException @c other.isTextViewerDisposed() returned @c true
		 */
		VisualPoint::VisualPoint(const VisualPoint& other) : Point(other), crossingLines_(false) {
			if(other.isInstalled()) {
				if(other.isTextAreaDisposed())
					throw TextAreaDisposedException();
				install(const_cast<TextArea&>(other.textArea()));
				positionInVisualLine_ = other.positionInVisualLine_;
				crossingLines_ = false;
				lineNumberCaches_ = other.lineNumberCaches_;
			}
		}

		/// Destructor.
		VisualPoint::~VisualPoint() BOOST_NOEXCEPT {
			uninstall();
		}

		/// @internal Resets @c #lineNumberCaches_ data member.
		void VisualPoint::buildVisualLineCache() {
			assert(isFullyAvailable());
			if(lineNumberCaches_ == boost::none) {
				const kernel::Position p(normalized());	// may throw kernel.DocumentDisposedException
				const graphics::font::TextRenderer& renderer = textArea().textRenderer();
				Index line = renderer.layouts().mapLogicalLineToVisualLine(p.line), subline;
				if(const graphics::font::TextLayout* const layout = renderer.layouts().at(p.line))
					subline = layout->lineAt(p.offsetInLine);
				else
					subline = 0;
				line += subline;
				lineNumberCaches_ = graphics::font::VisualLine(line, subline);
			}
		}

		/**
		 * Installs this @c VisualPoint on the specified @c TextArea.
		 * @param textArea The @c TextArea by which this point is installed
		 * @throw kernel#DocumentDisposedException @c #isDocumentDisposed() returned @c true
		 * @throw std#invalid_argument @a textArea does not belong to the document of this point
		 * @note If @c #isInstalled() returned @c true, this method does nothing.
		 * @see #isInstalled, #uninstall
		 */
		void VisualPoint::install(TextArea& textArea) {
			if(!isInstalled()) {
				if(&textArea.textViewer().document() != &document())	// may throw kernel.DocumentDisposedException
					throw std::invalid_argument("The specified viewer does not belong to the document of this point.");

				textAreaProxy_ = textArea.referByPoint();
				assert(isFullyAvailable());
				positionInVisualLine_ = boost::none;
				crossingLines_ = false;
				lineNumberCaches_ = boost::none;
				textArea.textRenderer().layouts().addVisualLinesListener(*this);
			}
		}

		/// @see Point#moved
		void VisualPoint::moved(const kernel::Position& from) {
			assert(!isDocumentDisposed());
			const bool fullyAvailable = isFullyAvailable();
			if(fullyAvailable) {
				if(from.line == kernel::line(*this) && lineNumberCaches_ != boost::none) {
					const graphics::font::TextLayout* const layout = textArea().textRenderer().layouts().at(kernel::line(*this));
					lineNumberCaches_->line -= lineNumberCaches_->subline;
					lineNumberCaches_->subline = (layout != nullptr) ? layout->lineAt(kernel::offsetInLine(*this)) : 0;
					lineNumberCaches_->line += lineNumberCaches_->subline;
				} else
					lineNumberCaches_ = boost::none;
			}
			Point::moved(from);
			if(fullyAvailable && !crossingLines_)
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
				const Char* const eol = std::find_first_of(bol, last, std::begin(NEWLINE_CHARACTERS), std::end(ASCENSION_ENDOF));

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
		/// @internal @c Point#moveTo for @c VisualDestinationProxy.
		void VisualPoint::moveTo(const VisualDestinationProxy& to) {
			document();	// may throw kernel.DocumentDisposedException
			throwIfNotFullyAvailable();
			if(!to.crossesVisualLines()) {
				moveTo(to.position());
				return;
			}
			if(positionInVisualLine_ == boost::none)
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

		/**
		 * Returns the offset of the point in the visual line.
		 * @throw NotInstalledException
		 * @throw TextViewerDisposedException
		 */
		Index VisualPoint::offsetInVisualLine() const {
			throwIfNotFullyAvailable();
			if(positionInVisualLine_ == boost::none)
				const_cast<VisualPoint*>(this)->rememberPositionInVisualLine();
//			const TextViewer::Configuration& c = textViewer().configuration();
			const graphics::font::TextRenderer& renderer = textArea().textRenderer();
//			if(resolveTextAlignment(c.alignment, c.readingDirection) != ALIGN_RIGHT)
				return static_cast<Index>(boost::get(positionInVisualLine_)
					/ widgetapi::createRenderingContext(textArea().textViewer())->fontMetrics(renderer.defaultFont())->averageCharacterWidth());
//			else
//				return (renderer.width() - positionInVisualLine_) / renderer.averageCharacterWidth();
		}

		/// @internal Updates @c positionInVisualLine_ with the current position.
		inline void VisualPoint::rememberPositionInVisualLine() {
			// positionInVisualLine_ is distance from left/top-edge of content-area to the point
			assert(!crossingLines_);
			throwIfNotFullyAvailable();
			if(!isDocumentDisposed()) {
				graphics::font::TextRenderer& renderer = textArea().textRenderer();
				const graphics::font::TextLayout& layout =
					renderer.layouts().at(kernel::line(*this), graphics::font::LineLayoutVector::USE_CALCULATED_LAYOUT);
				positionInVisualLine_ =
					graphics::font::lineStartEdge(layout, renderer.viewport()->contentMeasure())
					+ layout.hitToPoint(graphics::font::TextHit<>::leading(kernel::offsetInLine(*this))).ipd();
			}
		}

		/**
		 * Uninstalls this @c VisualPoint.
		 * @note If @c #isInstalled() returned @c false, this method does nothing.
		 * @see #install, #isInstalled
		 */
		void VisualPoint::uninstall() BOOST_NOEXCEPT {
			if(isInstalled()) {
				if(!isTextAreaDisposed()) {
					try {
						textArea().textRenderer().layouts().removeVisualLinesListener(*this);
					} catch(...) {
						// ignore the error
					}
				}
				textAreaProxy_.reset();
			}
		}

		/// @see VisualLinesListener#visualLinesDeleted
		void VisualPoint::visualLinesDeleted(const boost::integer_range<Index>& lines, Index, bool) BOOST_NOEXCEPT {
			if(!adaptsToDocument() && includes(lines, kernel::line(*this)))
				lineNumberCaches_ = boost::none;
		}

		/// @see VisualLinesListener#visualLinesInserted
		void VisualPoint::visualLinesInserted(const boost::integer_range<Index>& lines) BOOST_NOEXCEPT {
			if(!adaptsToDocument() && includes(lines, kernel::line(*this)))
				lineNumberCaches_ = boost::none;
		}

		/// @see VisualLinesListener#visualLinesModified
		void VisualPoint::visualLinesModified(const boost::integer_range<Index>& lines, SignedIndex sublineDifference, bool, bool) BOOST_NOEXCEPT {
			if(isFullyAvailable() && lineNumberCaches_ != boost::none) {
				// adjust visualLine_ and visualSubine_ according to the visual lines modification
				if(*lines.end() <= kernel::line(*this))
					lineNumberCaches_->line += sublineDifference;
				else if(*lines.begin() == kernel::line(*this)) {
					if(const graphics::font::TextLayout* const layout = textArea().textRenderer().layouts().at(kernel::line(*this))) {
						lineNumberCaches_->line -= lineNumberCaches_->subline;
						lineNumberCaches_->subline =
							layout->lineAt(std::min(kernel::offsetInLine(*this), document().lineLength(kernel::line(*this))));
						lineNumberCaches_->line += lineNumberCaches_->subline;
					} else
						lineNumberCaches_ = boost::none;
				} else if(*lines.begin() < kernel::line(*this))
					lineNumberCaches_ = boost::none;
			}
		}

		/// Default constructor.
		VisualPoint::NotInstalledException::NotInstalledException() :
				IllegalStateException("The VisualPoint is not installed by a TextViewer.") {
		}

		/// Default constructor.
		VisualPoint::TextAreaDisposedException::TextAreaDisposedException() :
				IllegalStateException("The TextArea which had installed the VisualPoint has been disposed.") {
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
			 * Returns the beginning of the visual line. If the layout of the line where @a p is placed is not
			 * calculated, this function calls @c beginningOfLine(p).
			 * @param p The base position
			 * @return The destination
			 * @see beginningOfLine
			 */
			Position beginningOfVisualLine(const viewer::VisualPoint& p) {
				const Position np(p.normalized());
				if(const graphics::font::TextLayout* const layout = p.textArea().textRenderer().layouts().at(np.line))
					return Position(np.line, layout->lineOffset(layout->lineAt(np.offsetInLine)));
				return beginningOfLine(p);
			}

			/**
			 * Returns the beginning of the line or the first printable character in the line by context.
			 * @param p The base position
			 * @return The destination
			 */
			Position contextualBeginningOfLine(const viewer::VisualPoint& p) {
				return isFirstPrintableCharacterOfLine(p) ? beginningOfLine(p) : firstPrintableCharacterOfLine(p);
			}

			/**
			 * Moves to the beginning of the visual line or the first printable character in the visual line by
			 * context.
			 * @param p The base position
			 * @return The destination
			 */
			Position contextualBeginningOfVisualLine(const viewer::VisualPoint& p) {
				return isFirstPrintableCharacterOfLine(p) ?
					beginningOfVisualLine(p) : firstPrintableCharacterOfVisualLine(p);
			}

			/**
			 * Moves to the end of the line or the last printable character in the line by context.
			 * @param p The base position
			 * @return The destination
			 */
			Position contextualEndOfLine(const viewer::VisualPoint& p) {
				return isLastPrintableCharacterOfLine(p) ? endOfLine(p) : lastPrintableCharacterOfLine(p);
			}

			/**
			 * Moves to the end of the visual line or the last printable character in the visual line by
			 * context.
			 * @param p The base position
			 * @return The destination
			 */
			Position contextualEndOfVisualLine(const viewer::VisualPoint& p) {
				return isLastPrintableCharacterOfLine(p) ?
					endOfVisualLine(p) : lastPrintableCharacterOfVisualLine(p);
			}

			/**
			 * Returns the end of the visual line. If the layout of the line where @a p is placed is not calculated,
			 * this function calls @c endOfLine(p).
			 * @param p The base position
			 * @return The destination
			 * @see endOfLine
			 */
			Position locations::endOfVisualLine(const viewer::VisualPoint& p) {
				Position np(p.normalized());
				if(const graphics::font::TextLayout* const layout = p.textArea().textRenderer().layouts().at(np.line)) {
					const Index subline = layout->lineAt(np.offsetInLine);
					np.offsetInLine = (subline < layout->numberOfLines() - 1) ?
						layout->lineOffset(subline + 1) : p.document().lineLength(np.line);
					if(layout->lineAt(np.offsetInLine) != subline)
						np = nextCharacter(p.document(), np, Direction::BACKWARD, GRAPHEME_CLUSTER);
					return np;
				}
				return endOfLine(p);
			}

			/**
			 * Returns the first printable character in the line.
			 * @param p The base position
			 * @return The destination
			 * @see firstPrintableCharacterOfLine
			 */
			Position locations::firstPrintableCharacterOfLine(const viewer::VisualPoint& p) {
				Position np(p.normalized());
				const Char* const s = p.document().line(np.line).data();
				np.offsetInLine = detail::identifierSyntax(p).eatWhiteSpaces(s, s + p.document().lineLength(np.line), true) - s;
				return np;
			}

			/**
			 * Returns the first printable character in the visual line. If the layout of the line where @a p is placed
			 * is not calculated, this function calls @c firstPrintableCharacterOfLine(p).
			 * @param p The base position
			 * @return The destination
			 * @see firstPrintableCharacterOfLine
			 */
			Position firstPrintableCharacterOfVisualLine(const viewer::VisualPoint& p) {
				Position np(p.normalized());
				const String& s = p.document().line(np.line);
				if(const graphics::font::TextLayout* const layout = p.textArea().textRenderer().layouts().at(np.line)) {
					const Index subline = layout->lineAt(np.offsetInLine);
					np.offsetInLine = detail::identifierSyntax(p).eatWhiteSpaces(
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
			 * Returns @c true if the point is the beginning of the visual line. If the layout of the line where @a p is placed
			 * is not calculated, this function calls @c isBeginningOfLine(p).
			 * @param p The base position
			 * @see isBeginningOfLine
			 */
			bool isBeginningOfVisualLine(const viewer::VisualPoint& p) {
				if(isBeginningOfLine(p))	// this considers narrowing
					return true;
				const Position np(p.normalized());
				if(const graphics::font::TextLayout* const layout = p.textArea().textRenderer().layouts().at(np.line))
					return np.offsetInLine == layout->lineOffset(layout->lineAt(np.offsetInLine));
				return isBeginningOfLine(p);
			}

			/**
			 * Returns @c true if the point is end of the visual line. If the layout of the line where @a p is placed
			 * is not calculated, this function calls @c isEndOfLine(p).
			 * @param p The base position
			 * @see isEndOfLine
			 */
			bool isEndOfVisualLine(const viewer::VisualPoint& p) {
				if(isEndOfLine(p))	// this considers narrowing
					return true;
				const Position np(p.normalized());
				if(const graphics::font::TextLayout* const layout = p.textArea().textRenderer().layouts().at(np.line)) {
					const Index subline = layout->lineAt(np.offsetInLine);
					return np.offsetInLine == layout->lineOffset(subline) + layout->lineLength(subline);
				}
				return isEndOfLine(p);
			}

			/// Returns @c true if the given position is the first printable character in the line.
			bool isFirstPrintableCharacterOfLine(const viewer::VisualPoint& p) {
				const Position np(p.normalized()), bob(p.document().accessibleRegion().first);
				const Index offset = (bob.line == np.line) ? bob.offsetInLine : 0;
				const String& line = p.document().line(np.line);
				return line.data() + np.offsetInLine - offset
					== detail::identifierSyntax(p).eatWhiteSpaces(line.data() + offset, line.data() + line.length(), true);
			}

			/// Returns @c true if the given position is the first printable character in the visual line.
			bool isFirstPrintableCharacterOfVisualLine(const viewer::VisualPoint& p) {
				// TODO: not implemented.
				return false;
			}

			/// Returns @c true if the given position is the last printable character in the line.
			bool isLastPrintableCharacterOfLine(const viewer::VisualPoint& p) {
				const Position np(p.normalized()), eob(p.document().accessibleRegion().second);
				const String& line = p.document().line(np.line);
				const Index lineLength = (eob.line == np.line) ? eob.offsetInLine : line.length();
				return line.data() + lineLength - np.offsetInLine
					== detail::identifierSyntax(p).eatWhiteSpaces(line.data() + np.offsetInLine, line.data() + lineLength, true);
			}

			/// Returns @c true if the given position is the last printable character in the visual line.
			bool isLastPrintableCharacterOfVisualLine(const viewer::VisualPoint& p) {
				// TODO: not implemented.
				return false;
			}

			/**
			 * Returns the last printable character in the line.
			 * @param p The base position
			 * @return The destination
			 */
			Position lastPrintableCharacterOfLine(const viewer::VisualPoint& p) {
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
			Position lastPrintableCharacterOfVisualLine(const viewer::VisualPoint& p) {
				// TODO: not implemented.
				return p.normalized();
			}

#ifdef ASCENSION_ABANDONED_AT_VERSION_08
			namespace {
				inline ReadingDirection defaultUIReadingDirection(const viewer::VisualPoint& p) {
					return p.textViewer().textRenderer().defaultUIWritingMode().inlineFlowDirection;
				}
			}

			/**
			 * Returns the beginning of the word where advanced to the left by N words.
			 * @param p The base position
			 * @param words The number of words to adavance
			 * @return The destination, or @c boost#none if the writing mode is vertical
			 */
			boost::optional<Position> leftWord(const viewer::VisualPoint& p, Index words /* = 1 */) {
				return (defaultUIReadingDirection(p) == LEFT_TO_RIGHT) ? backwardWord(p, words) : forwardWord(p, words);
			}

			/**
			 * Returns the end of the word where advanced to the left by N words.
			 * @param p The base position
			 * @param words The number of words to adavance
			 * @return The destination, or @c boost#none if the writing mode is vertical
			 */
			boost::optional<Position> leftWordEnd(const viewer::VisualPoint& p, Index words /* = 1 */) {
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
					const viewer::VisualPoint& p, graphics::PhysicalDirection direction, Index marks /* = 1 */) {
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
			viewer::VisualDestinationProxy nextPage(const viewer::VisualPoint& p, Direction direction, Index pages /* = 1 */) {
				Index lines = 0;
				const std::shared_ptr<const graphics::font::TextViewport> viewport(p.textArea().textRenderer().viewport());
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
			viewer::VisualDestinationProxy nextVisualLine(const viewer::VisualPoint& p, Direction direction, Index lines /* = 1 */) {
				// ISSUE: LineLayoutVector.offsetVisualLine(VisualLine&, SignedIndex) does not use calculated layouts.
				Position np(p.normalized());
				const graphics::font::TextRenderer& renderer = p.textArea().textRenderer();
				const graphics::font::TextLayout* layout = renderer.layouts().at(np.line);
				Index subline = (layout != nullptr) ? layout->lineAt(np.offsetInLine) : 0;
				if(direction == Direction::FORWARD) {
					if(np.line == p.document().numberOfLines() - 1 && (layout == nullptr || subline == layout->numberOfLines() - 1))
						return viewer::detail::VisualDestinationProxyMaker::make(np, true);
				} else {
					if(np.line == 0 && subline == 0)
						return viewer::detail::VisualDestinationProxyMaker::make(np, true);
				}
				graphics::font::VisualLine visualLine(np.line, subline);
				renderer.layouts().offsetVisualLine(visualLine,
					(direction == Direction::FORWARD) ? static_cast<SignedIndex>(lines) : -static_cast<SignedIndex>(lines));
				if(!p.positionInVisualLine_)
					const_cast<viewer::VisualPoint&>(p).rememberPositionInVisualLine();
				np.line = visualLine.line;
				if(nullptr != (layout = renderer.layouts().at(visualLine.line))) {
					np.offsetInLine = layout->hitTestCharacter(
						presentation::FlowRelativeTwoAxes<graphics::Scalar>(
							presentation::_ipd = *p.positionInVisualLine_ - graphics::font::lineStartEdge(*layout, renderer.viewport()->contentMeasure()),
							presentation::_bpd = graphics::font::TextLayout::LineMetricsIterator(*layout, visualLine.subline).baselineOffset())).insertionIndex();
					if(layout->lineAt(np.offsetInLine) != visualLine.subline)
						np = nextCharacter(p.document(), np, Direction::BACKWARD, GRAPHEME_CLUSTER);
				}
				return viewer::detail::VisualDestinationProxyMaker::make(np, true);
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
					const viewer::VisualPoint& p, PhysicalDirection direction, Index words /* = 1 */) {
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
					const viewer::VisualPoint& p, PhysicalDirection direction, Index words /* = 1 */) {
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
			boost::optional<Position> rightWord(const viewer::VisualPoint& p, Index words /* = 1 */) {
				return (defaultUIReadingDirection(p) == LEFT_TO_RIGHT) ? forwardWord(p, words) : backwardWord(p, words);
			}

			/**
			 * Returns the end of the word where advanced to the right by N words.
			 * @param p The base position
			 * @param words The number of words to adavance
			 * @return The destination
			 */
			boost::optional<Position> rightWordEnd(const viewer::VisualPoint& p, Index words /* = 1 */) {
				return (defaultUIReadingDirection(p) == LEFT_TO_RIGHT) ? forwardWordEnd(p, words) : backwardWordEnd(p, words);
			}
#endif // ASCENSION_ABANDONED_AT_VERSION_08
		}
	}
}
