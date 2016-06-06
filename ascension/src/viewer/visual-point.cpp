/**
 * @file visual-point.cpp
 * @author exeal
 * @date 2003-2008 was point.cpp
 * @date 2008-2010 separated from point.cpp
 * @date 2011-10-02 separated from caret.cpp
 * @date 2011-2015
 */

#include <ascension/corelib/text/break-iterator.hpp>
#include <ascension/corelib/text/identifier-syntax.hpp>
#include <ascension/kernel/document.hpp>
#include <ascension/kernel/document-character-iterator.hpp>
#include <ascension/graphics/font/font-metrics.hpp>
#include <ascension/graphics/font/line-layout-vector.hpp>
#include <ascension/graphics/font/text-layout.hpp>
#include <ascension/graphics/font/text-viewport.hpp>
#include <ascension/graphics/rendering-context.hpp>
#include <ascension/viewer/text-area.hpp>
#include <ascension/viewer/text-viewer.hpp>
#include <ascension/viewer/text-viewer-model-conversion.hpp>
#include <ascension/viewer/visual-point.hpp>
#ifndef ASCENSION_PIXELFUL_SCROLL_IN_BPD
#	include <boost/math/special_functions/trunc.hpp>
#endif

namespace ascension {
	namespace viewer {
		namespace detail {
			// detail.VisualDestinationProxyMaker /////////////////////////////////////////////////////////////////////////

			class VisualDestinationProxyMaker {
			public:
				static viewer::VisualDestinationProxy make(const TextHit& p, bool crossVisualLines) {
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
				const kernel::Position np(insertionPosition(p));
				const graphics::font::TextLayout& layout = renderer.layouts().at(
					kernel::line(np), graphics::font::LineLayoutVector::USE_CALCULATED_LAYOUT);	// this call may change the layouts
				const float visibleLines = viewport->numberOfVisibleLines();
				presentation::FlowRelativeTwoAxes<graphics::font::TextViewport::ScrollOffset> to;	// scroll destination

#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
#	error Not implemented.
#else
				to.bpd() = viewport->scrollPositions().bpd();
				// scroll if the point is outside of 'before-edge'
				if(to.bpd() > p.visualLine().line)
					to.bpd() = p.visualLine().line;
				// scroll if the point is outside of 'after-edge'
				else if(to.bpd() + static_cast<graphics::font::TextViewport::ScrollOffset>(visibleLines) - 1 < p.visualLine().line)
					to.bpd() = p.visualLine().line + 1 - static_cast<graphics::font::TextViewport::ScrollOffset>(boost::math::trunc(visibleLines));
#endif

				// scroll if the point is outside of 'start-edge' or 'end-edge'
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
//				if(!viewer.configuration().lineWrap.wrapsAtWindowEdge()) {
					const graphics::font::Font::Metrics& fontMetrics = renderer.defaultFont()->metrics();
					const Index visibleColumns = viewport->numberOfVisibleCharactersInLine();
					const font::TextLayout& lineLayout = renderer.layouts().at(kernel::line(np));
					const graphics::Scalar x = geometry::x(lineLayout.location(kernel::offsetInLine(np), font::TextLayout::LEADING)) + font::lineIndent(lineLayout, viewport->contentMeasure(), 0);
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
					(graphics::font::lineIndent(layout, viewport->contentMeasure()) + layout.hitToPoint(graphics::font::makeLeadingTextHit(kernel::offsetInLine(np))).ipd()));
				to.ipd() = std::min(pointIpd, viewport->scrollPositions().ipd());
				to.ipd() = std::max(pointIpd - static_cast<Index>(graphics::font::pageSize<presentation::ReadingDirection>(*viewport)) + 1, to.ipd());
#	else
				const Index pointIpd = static_cast<Index>(
					(graphics::font::lineIndent(layout, viewport->contentMeasure()) + layout.hitToPoint(graphics::font::makeLeadingTextHit(kernel::offsetInLine(np))).ipd())
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
		 * <h3>Three States of @c VisualPoint</h3>
		 *
		 * A @c VisualPoint has the following three states:
		 * <dl>
		 *   <dt>State 1 : Constructed but not installed by @c TextArea</dt>
		 *   <dd>
		 *     - The @c VisualPoint has been just constructed with the document, but not be installed by a
		 *       @c TextArea.
		 *     - The all features only as @c kernel#Point are available. The other methods throw
		 *       @c VisualPoint#NotInstalledException exception.
		 *     - @c VisualPoint#isInstalled() returns @c false.
		 *     - @c VisualPoint#isTextAreaDisposed() throws @c VisualPoint#NotInstalledException exception.
		 *     - @c VisualPoint#isFullyAvailable() returns @c false.
		 *     - Constructed by the constructor which takes a document and a position.
		 *   </dd>
		 *   <dt>State 2 : Installed by @c TextArea</dt>
		 *   <dd>
		 *     - The @c VisualPoint has been installed by the @c TextArea.
		 *     - The all features are available.
		 *     - @c VisualPoint#isInstalled() returns @c true.
		 *     - @c VisualPoint#isTextAreaDisposed() returns @c false.
		 *     - @c VisualPoint#isFullyAvailable() returns @c true.
		 *     - Constructed by the constructor which takes a text viewer and a position.
		 *     - You can revert the @c VisualPoint to the state 1 by calling @c VisualPoint#uninstall method.
		 *   </dd>
		 *   <dt>State 3 : Installed @c TextArea was disposed</dt>
		 *   <dd>
		 *     - The @c TextArea which installed the @c VisualPoint has already disposed.
		 *     - The all features only as @c kernel#Point are available. The other methods throw
		 *       @c VisualPoint#TextViewerDisposedException exception.
		 *     - @c VisualPoint#isInstalled() returns @c true.
		 *     - @c VisualPoint#isTextAreaDisposed() returns @c true.
		 *     - @c VisualPoint#isFullyAvailable() returns @c false.
		 *     - You can revert the @c VisualPoint to the state 1 by calling @c VisualPoint#uninstall method.
		 *   </dd>
		 * </dl>
		 *
		 * <h3>@c TextHit, rather than @c kernel#Position</h3>
		 *
		 * Unlike @c kernel#Point, a @c VisualPoint uses @c TextHit rather than @c kernel#Position.
		 *
		 * - @c VisualPoint hides @c kernel#Point#position method.
		 *
		 * @see kernel#Point, TextViewer
		 */

		kernel::Position insertionPosition(const kernel::Document& document, const TextHit& hit) {
			if(hit.isLeadingEdge())
				return hit.characterIndex();
			text::GraphemeBreakIterator<kernel::DocumentCharacterIterator> i(kernel::DocumentCharacterIterator(document, hit.characterIndex()));
			return (++i).base().tell();
		}

		TextHit otherHit(const kernel::Document& document, const TextHit& hit) {
			text::GraphemeBreakIterator<kernel::DocumentCharacterIterator> i(kernel::DocumentCharacterIterator(document, hit.characterIndex()));
			if(hit.isLeadingEdge())
				return TextHit::trailing((--i).base().tell());
			else
				return TextHit::leading((++i).base().tell());
		}

		/**
		 * Creates a @c VisualPoint object but does not install on @c TextArea.
		 * @param document The document
		 * @param position The initial position of the point. @c kernel#Point is initialized by @c position.insertionIndex()
		 * @throw kernel#BadPositionException The constructor of @c kernel#Point class threw this exception
		 * @post @c #isInstalled() returns @c false.
		 * @post @c #isTextAreaDisposed() throws @c #NotInstalledException exception.
		 * @post @c #isFullyAvailable() returns @c false.
		 */
		VisualPoint::VisualPoint(kernel::Document& document,
				const TextHit& position /* = TextHit::leading(kernel::Position::zero()) */)
				: AbstractPoint(document), hit_(position), crossingLines_(false) {
		}

		/**
		 * Creates a @c VisualPoint object and installs on the specified @c TextArea.
		 * @param textArea The text area
		 * @param position The initial position of the point
		 * @throw kernel#BadPositionException The constructor of @c kernel#Point class threw this exception
		 * @post @c #isInstalled() returns @c true.
		 * @post @c #isTextAreaDisposed() returns @c false.
		 * @post @c #isFullyAvailable() returns @c true.
		 */
		VisualPoint::VisualPoint(TextArea& textArea,
				const TextHit& position /* = TextHit::leading(kernel::Position::zero()) */)
				: AbstractPoint(textArea.textViewer().document()), hit_(position), crossingLines_(false) {
			install(textArea);
		}

		/**
		 * Creates a @c VisualPoint object but does not install on @c TextArea.
		 * @param other The point used to initialize kernel part of the new object
		 * @throw kernel#BadPositionException The constructor of @c kernel#Point class threw this exception
		 * @post @c #isInstalled() returns @c false.
		 * @post @c #isTextAreaDisposed() throws @c #NotInstalledException exception.
		 * @post @c #isFullyAvailable() returns @c false.
		 */
		VisualPoint::VisualPoint(const graphics::font::TextHit<kernel::Point>& other) :
				AbstractPoint(const_cast<kernel::Document&>(other.characterIndex().document())),
				hit_(other.isLeadingEdge() ?
					TextHit::leading(other.characterIndex().position()) : TextHit::trailing(other.characterIndex().position())),
				crossingLines_(false) {
		}

		/**
		 * Copy-constructor.
		 * @param other The source object. If this has been installed, the new point is also installed
		 * @throw kernel#DocumentDisposedException The copy-constructor of @c kernel#Point threw this exception
		 * @throw TextAreaDisposedException @c other.isTextAreaDisposed() returned @c true
		 */
		VisualPoint::VisualPoint(const VisualPoint& other) : AbstractPoint(other), hit_(other.hit_), crossingLines_(false) {
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

		/**
		 * See @c kernel#Point#aboutToMove. @c VisualPoint#aboutToMove does nothing.
		 * @param to The destination position
		 * @throw DocumentDisposedException The document to which the point belongs is already disposed
		 * @see #moved, #moveTo, kernel#Point#aboutToMove
		 */
		void VisualPoint::aboutToMove(TextHit& to) {
		}

		/// @internal Resets @c #lineNumberCaches_ data member.
		void VisualPoint::buildVisualLineCaches() {
			assert(isFullyAvailable());
			if(lineNumberCaches_ == boost::none) {
				const auto p(hit().characterIndex());	// may throw kernel.DocumentDisposedException
				const graphics::font::TextRenderer& renderer = textArea().textRenderer();
				Index line = renderer.layouts().mapLogicalLineToVisualLine(kernel::line(p)), subline;
				if(const graphics::font::TextLayout* const layout = renderer.layouts().at(kernel::line(p))) {
					const auto offset = kernel::offsetInLine(p);
					subline = layout->lineAt(hit().isLeadingEdge() ? graphics::font::makeLeadingTextHit(offset) : graphics::font::makeTrailingTextHit(offset));
				} else
					subline = 0;
				line += subline;
				lineNumberCaches_ = graphics::font::VisualLine(line, subline);
			}
		}

		/// @see AbstractPoint#contentReset
		void VisualPoint::contentReset() {
			assert(!isDocumentDisposed());
			assert(adaptsToDocument());
			moveTo(TextHit::leading(kernel::Position::zero()));
		}

		/// @see AbstractPoint#documentChanged
		void VisualPoint::documentChanged(const kernel::DocumentChange& change) {
			assert(!isDocumentDisposed());
			assert(adaptsToDocument());
//			normalize();
			const auto ip(insertionPosition(*this));
			const auto newPosition(kernel::positions::updatePosition(ip, change, gravity()));
			const TextHit newHit((hit().isLeadingEdge() || includes(change.erasedRegion(), ip)) ? TextHit::leading(newPosition) : TextHit::trailing(newPosition));
			if(newHit != hit())
				moveTo(newHit);	// TODO: this may throw...
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

		/**
		 * See @c kernel#Point#moved.
		 * @see #aboutToMove, #moveTo
		 */
		void VisualPoint::moved(const TextHit& from) {
			assert(!isDocumentDisposed());
			const bool fullyAvailable = isFullyAvailable();
			if(fullyAvailable) {
				if(kernel::line(from.characterIndex()) == kernel::line(hit().characterIndex()) && lineNumberCaches_ != boost::none)
					updateLineNumberCaches();
				else
					lineNumberCaches_ = boost::none;
			}
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
		namespace {
			inline bool isOutsideOfDocumentRegion(const kernel::Document& document, const TextHit& hit) BOOST_NOEXCEPT {
				return kernel::positions::isOutsideOfDocumentRegion(document, insertionPosition(document, hit));
			}
			inline TextHit shrinkToDocumentRegion(const kernel::Document& document, const TextHit& hit) BOOST_NOEXCEPT {
				if(kernel::line(hit.characterIndex()) >= document.numberOfLines())
					return TextHit::leading(kernel::locations::endOfDocument(std::make_pair(std::ref(document), hit.characterIndex())));
				const Index line = kernel::line(hit.characterIndex());
				if(kernel::offsetInLine(hit.characterIndex()) < document.lineLength(line))
					return hit;
				else
					return TextHit::leading(kernel::locations::endOfLine(std::make_pair(std::ref(document), hit.characterIndex())));
			}
		}

		/**
		 * Moves to the specified position. See @c kernel#Point#moveTo.
		 * @param to The destination position
		 * @param trailing If a @c VisualPoint is the end of the visual line, @c modelToView free functions return the
		 *                 beginning of the next visual line. Set @c true to return the end of the visual line
		 * @return This @c VisualPoint
		 * @throw kernel#BadPositionException @a to is outside of the document
		 * @throw ... Any exceptions @c #aboutToMove implementation of sub-classe throws
		 * @see kernel#Point#moveTo
		 */
		VisualPoint& VisualPoint::moveTo(const TextHit& to) {
			if(isDocumentDisposed())
				throw kernel::DocumentDisposedException();
			else if(isOutsideOfDocumentRegion(document(), to))
				throw kernel::BadPositionException();
			TextHit destination(to);
			aboutToMove(destination);
			destination = shrinkToDocumentRegion(document(), destination);
			const TextHit from(hit());
			hit_ = destination;
			moved(from);
			if(destination != from)
				motionSignal_(*this, from);
			return *this;
		}

		/// @internal @c Point#moveTo for @c VisualDestinationProxy.
		void VisualPoint::moveTo(const VisualDestinationProxy& to) {
			document();	// may throw kernel.DocumentDisposedException
			throwIfNotFullyAvailable();
			if(!to.crossesVisualLines()) {
				moveTo(static_cast<const TextHit&>(to));
				return;
			}
			if(positionInVisualLine_ == boost::none)
				rememberPositionInVisualLine();
			crossingLines_ = true;
			try {
				moveTo(static_cast<const TextHit&>(to));
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
				const auto p(hit().characterIndex());
				const graphics::font::TextLayout& layout =
					renderer.layouts().at(kernel::line(p), graphics::font::LineLayoutVector::USE_CALCULATED_LAYOUT);
				const auto offset = kernel::offsetInLine(p);
				const auto h(hit().isLeadingEdge() ? graphics::font::makeTextHitAfterOffset(offset) : graphics::font::makeTextHitBeforeOffset(offset));
				positionInVisualLine_ =
					graphics::font::lineStartEdge(layout, renderer.viewport()->contentMeasure())
					+ layout.hitToPoint(h).ipd();
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

		/// @internal
		void VisualPoint::updateLineNumberCaches() {
			if(lineNumberCaches_ != boost::none) {
				const graphics::font::TextLayout* const layout = textArea().textRenderer().layouts().at(kernel::line(hit().characterIndex()));
				graphics::font::VisualLine newLineNumber;
				assert(boost::get(lineNumberCaches_).line >= boost::get(lineNumberCaches_).subline);
				newLineNumber.line = boost::get(lineNumberCaches_).line - boost::get(lineNumberCaches_).subline;
				if(layout != nullptr) {
					const auto p(hit().characterIndex());
					const auto offset = kernel::offsetInLine(p);
					auto h(hit().isLeadingEdge() ? graphics::font::makeLeadingTextHit(offset) : graphics::font::makeTrailingTextHit(offset));
					h = std::min(h, graphics::font::makeLeadingTextHit(document().lineLength(kernel::line(p))));
					newLineNumber.subline = layout->lineAt(h);
				} else
					newLineNumber.subline = 0;
				newLineNumber.line += newLineNumber.subline;
				lineNumberCaches_ = newLineNumber;
			}
		}

		/// @see VisualLinesListener#visualLinesDeleted
		void VisualPoint::visualLinesDeleted(const boost::integer_range<Index>& lines, Index, bool) BOOST_NOEXCEPT {
			if(!adaptsToDocument() && includes(lines, kernel::line(hit().characterIndex())))
				lineNumberCaches_ = boost::none;
		}

		/// @see VisualLinesListener#visualLinesInserted
		void VisualPoint::visualLinesInserted(const boost::integer_range<Index>& lines) BOOST_NOEXCEPT {
			if(!adaptsToDocument() && includes(lines, kernel::line(hit().characterIndex())))
				lineNumberCaches_ = boost::none;
		}

		/// @see VisualLinesListener#visualLinesModified
		void VisualPoint::visualLinesModified(const boost::integer_range<Index>& lines, SignedIndex sublineDifference, bool, bool) BOOST_NOEXCEPT {
			if(isFullyAvailable() && lineNumberCaches_ != boost::none) {
				// adjust visualLine_ and visualSubine_ according to the visual lines modification
				const Index line = kernel::line(hit().characterIndex());
				if(*boost::const_end(lines) <= line)
					lineNumberCaches_->line += sublineDifference;
				else if(*boost::const_begin(lines) == line)
					updateLineNumberCaches();
				else if(*boost::const_begin(lines) < line)
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

		graphics::Point modelToView(const TextViewer& textViewer, const VisualPoint& p/*, bool fullSearchBpd*/) {
			return modelToView(textViewer, p.hit());
		}

		namespace locations {
			/**
			 * Returns the position advanced/returned by N pages.
			 * @param p The base position
			 * @param direction The direction
			 * @param pages The number of pages to advance/return
			 * @return The destination
			 */
			VisualDestinationProxy nextPage(const VisualPoint& p, Direction direction, Index pages /* = 1 */) {
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
			VisualDestinationProxy nextVisualLine(const VisualPoint& p, Direction direction, Index lines /* = 1 */) {
				// ISSUE: LineLayoutVector.offsetVisualLine(VisualLine&, SignedIndex) does not use calculated layouts.
				const auto hit(p.hit());
				auto line(kernel::line(hit.characterIndex()));
				auto inlineHit(hit.isLeadingEdge() ?
					graphics::font::makeLeadingTextHit(kernel::offsetInLine(hit.characterIndex()))
					: graphics::font::makeTrailingTextHit(kernel::offsetInLine(hit.characterIndex())));
				const graphics::font::TextRenderer& renderer = p.textArea().textRenderer();
				const graphics::font::TextLayout* layout = renderer.layouts().at(line);
				Index subline = (layout != nullptr) ? layout->lineAt(inlineHit) : 0;
				if(direction == Direction::FORWARD) {
					if(line == p.document().numberOfLines() - 1 && (layout == nullptr || subline == layout->numberOfLines() - 1))
						return detail::VisualDestinationProxyMaker::make(hit, true);
				} else {
					if(line == 0 && subline == 0)
						return viewer::detail::VisualDestinationProxyMaker::make(hit, true);
				}
				graphics::font::VisualLine visualLine(line, subline);
				renderer.layouts().offsetVisualLine(visualLine,
					(direction == Direction::FORWARD) ? static_cast<SignedIndex>(lines) : -static_cast<SignedIndex>(lines));
				if(p.positionInVisualLine_ == boost::none)
					const_cast<viewer::VisualPoint&>(p).rememberPositionInVisualLine();
				line = visualLine.line;
				if(nullptr != (layout = renderer.layouts().at(visualLine.line))) {
					inlineHit = layout->hitTestCharacter(
						presentation::FlowRelativeTwoAxes<graphics::Scalar>(
							presentation::_ipd = *p.positionInVisualLine_ - graphics::font::lineStartEdge(*layout, renderer.viewport()->contentMeasure()),
							presentation::_bpd = graphics::font::TextLayout::LineMetricsIterator(*layout, visualLine.subline).baselineOffset()));
//					if(layout->lineAt(inlineHit) != visualLine.subline)
//						np = nextCharacter(p.document(), np, Direction::BACKWARD, GRAPHEME_CLUSTER);
				}

				const kernel::Position temp(line, inlineHit.characterIndex());
				return viewer::detail::VisualDestinationProxyMaker::make(
					inlineHit.isLeadingEdge() ? viewer::TextHit::leading(temp) : viewer::TextHit::trailing(temp), true);
			}
		}
	}
}
