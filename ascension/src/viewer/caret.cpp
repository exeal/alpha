/**
 * @file caret.cpp
 * @author exeal
 * @date 2003-2008 was point.cpp
 * @date 2008-2010 separated from point.cpp
 * @date 2010-2016
 */

#include <ascension/corelib/numeric-range-algorithm/encompasses.hpp>
#include <ascension/corelib/utility.hpp>	// detail.ValueSaver<>
#include <ascension/corelib/text/identifier-syntax.hpp>
#include <ascension/corelib/text/utf.hpp>
#include <ascension/graphics/image.hpp>
#include <ascension/graphics/rendering-context.hpp>
#include <ascension/graphics/font/line-layout-vector.hpp>
#include <ascension/graphics/font/text-viewport.hpp>
#include <ascension/kernel/document.hpp>
#include <ascension/kernel/document-character-iterator.hpp>
#include <ascension/text-editor/commands/inputs.hpp>
#include <ascension/text-editor/input-sequence-checker.hpp>
#include <ascension/text-editor/session.hpp>
#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/standard-caret-painter.hpp>
#include <ascension/viewer/text-area.hpp>
#include <ascension/viewer/text-viewer.hpp>
#include <ascension/viewer/text-viewer-model-conversion.hpp>
#include <ascension/viewer/text-viewer-utility.hpp>
#include <ascension/viewer/virtual-box.hpp>
#include <ascension/viewer/visual-locations.hpp>
#include <boost/range/algorithm/binary_search.hpp>

namespace ascension {
	namespace viewer {
		/// @internal Default constructor.
		Caret::Shape::Shape() BOOST_NOEXCEPT : alignmentPoint() {
			// oh, this does nothing
		}

		/// @internal Default constructor.
		Caret::Context::Context() BOOST_NOEXCEPT : yanking(false), typing(false), inputMethodCompositionActivated(false), inputMethodComposingCharacter(false) {
		}

		// TODO: rewrite this documentation.

		/**
		 * @class ascension::viewer::Caret
		 *
		 * @c Caret is an extension of @c VisualPoint. A caret has a selection on the text viewer. And
		 * supports line selection, word selection, rectangle (box) selection, tracking match brackets, and
		 * clipboard enhancement.
		 *
		 * A caret has one another point called "anchor" (or "mark"). The selection is a region between the
		 * caret and the anchor. Anchor is @c VisualPoint but client can't operate this directly.
		 *
		 * Usually, the anchor will move adapting to the caret automatically. If you want to move the
		 * anchor isolately, create the selection by using @c #select method or call @c #extendSelectionTo
		 * method.
		 *
		 * When the caret moves, the text viewer will scroll automatically to show the caret. See the
		 * description of @c #enableAutoShow and @c #isAutoShowEnabled.
		 *
		 * @c Caret throws @c ReadOnlyDocumentException when tried to change the read-only document.
		 *
		 * このクラスの編集用のメソッドは @c EditPoint 、@c VisualPoint の編集用メソッドと異なり、
		 * 積極的に連続編集とビューの凍結を使用する
		 *
		 * 対括弧の検索はプログラムを編集しているときに役立つ機能で、キャレット位置に括弧があれば対応する括弧を検索する。
		 * 括弧のペアを強調表示するのは、現時点ではビューの責任である
		 *
		 * To enter rectangle selection mode, call @c #beginRectangleSelection method. To exit, call
		 * @c #endRectangleSelection method. You can get the information of the current rectangle selection
		 * by using @c #boxForRectangleSelection method.
		 *
		 * This class does not accept @c IPointListener. Use @c ICaretListener interface instead.
		 *
		 * @note This class is not intended to subclass.
		 */

		/**
		 * @typedef ascension::viewer::Caret::CharacterInputSignal
		 * The signal which gets emitted when a character was input by the caret.
		 * @param caret The caret
		 * @param c The code point of the input character
		 * @see #characterInputSignal, #inputCharacter
		 */

		/**
		 * @typedef ascension::viewer::Caret::InputModeChangedSignal
		 * The signal which gets emitted when any input mode of the caret had been changed.
		 * @param caret The caret
		 * @see #inputModeChangedSignal, #isOvertypeMode, #setOvertypeMode
		 */

		/**
		 * @typedef ascension::viewer::Caret::MatchBracketsChangedSignal
		 * The signal which gets emitted when the matched brackets were changed.
		 * @param caret The caret
		 * @param previouslyMatchedBrackets The pair of the brackets previously matched. May be @c boost#none
		 * @param outsideOfView The brackets newly matched are outside of the view
		 * @see #matchBrackets, #matchBracketsChangedSignal
		 */

		/**
		 * @typedef ascension::viewer::Caret::MotionSignal
		 * The signal which gets emitted when the caret was moved.
		 * @param caret The caret
		 * @param regionBeforeMotion The region which the caret had before. The @c first member is the anchor and
		 *                           @c second member is the caret
		 * @see #motionSignal
		 */

		/**
		 * @typedef ascension::viewer::Caret::SelectionShapeChangedSignal
		 * The signal which gets emitted when the shape (linear or rectangle) of the selection is changed.
		 * @param caret The caret
		 * @see #beginRectangleSelection, #endRectangleSelection, #isSelectionRectangle, #selectionShapeChangedSignal
		 */

		/**
		 * Creates a @c Caret object but does not install on @c TextViewer.
		 * @param document The document
		 * @param position The initial position of the caret
		 * @throw kernel#BadPositionException The constructor of @c kernel#Point class threw this exception
		 */
		Caret::Caret(kernel::Document& document, const TextHit& position /* = TextHit::leading(kernel::Position::zero()) */) : VisualPoint(document, position),
#if BOOST_OS_WINDOWS
				clipboardLocale_(::GetUserDefaultLCID()),
#endif // BOOST_OS_WINDOWS
				overtypeMode_(false), autoShow_(true), matchBracketsTrackingMode_(DONT_TRACK) {
			document.addListener(*this);
		}

		/**
		 * Creates a @c Caret object but does not install on @c TextViewer.
		 * @param other The point used to initialize kernel part of the new object
		 * @throw kernel#BadPositionException The constructor of @c kernel#Point class threw this exception
		 */
		Caret::Caret(const graphics::font::TextHit<kernel::Point>& other) : VisualPoint(other),
#if BOOST_OS_WINDOWS
				clipboardLocale_(::GetUserDefaultLCID()),
#endif // BOOST_OS_WINDOWS
				overtypeMode_(false), autoShow_(true), matchBracketsTrackingMode_(DONT_TRACK) {
			document().addListener(*this);
		}

		/**
		 * Creates a @c Caret object but does not install on @c TextViewer.
		 * @param other The point used to initialize kernel part of the new object
		 * @throw kernel#BadPositionException The constructor of @c kernel#Point class threw this exception
		 */
		Caret::Caret(const VisualPoint& other) : VisualPoint(other),
#if BOOST_OS_WINDOWS
				clipboardLocale_(::GetUserDefaultLCID()),
#endif // BOOST_OS_WINDOWS
				overtypeMode_(false), autoShow_(true), matchBracketsTrackingMode_(DONT_TRACK) {
			document().addListener(*this);
		}

		/**
		 * Creates a @c Caret object and installs on the specified @c TextArea.
		 * @param textArea The text area
		 * @param position The initial position of the point
		 * @throw kernel#BadPositionException The constructor of @c kernel#Point class threw this exception
		 */
		Caret::Caret(TextArea& textArea, const TextHit& position /* = TextHit::leading(kernel::Position::zero()) */) : VisualPoint(textArea, position),
#if BOOST_OS_WINDOWS
				clipboardLocale_(::GetUserDefaultLCID()),
#endif // BOOST_OS_WINDOWS
				overtypeMode_(false), autoShow_(true), matchBracketsTrackingMode_(DONT_TRACK) {
			document().addListener(*this);
			install(textArea);
		}

		/// Destructor.
		Caret::~Caret() BOOST_NOEXCEPT {
			if(isInstalled())
				uninstall();
			if(!isDocumentDisposed())
				document().removeListener(*this);
		}

		/// @see VisualPoint#aboutToMove
		void Caret::aboutToMove(TextHit& to) {
			const auto ip(insertionPosition(document(), to));
			if(kernel::locations::isOutsideOfDocumentRegion(kernel::locations::PointProxy(document(), ip)))
				throw kernel::BadPositionException(ip, "Caret tried to move outside of document.");
			VisualPoint::aboutToMove(to);
		}

		/**
		 * Starts rectangular selection.
		 * @see #endRectangleSelection, #isSelectionRectangle
		 */
		void Caret::beginRectangleSelection() {
			if(context_.selectedRectangle.get() == nullptr) {
				context_.selectedRectangle.reset(new VirtualBox(textArea(), selectedRegion()));
				selectionShapeChangedSignal_(*this);
			}
		}

		/**
		 * Returns @c true if a paste operation can be performed.
		 * @note Even when this method returned @c true, the following @c #paste call can fail.
		 * @param useKillRing Set @c true to get the content from the kill-ring of the session, not from the system
		 *                    clipboard
		 * @return true if the clipboard data is pastable
		 */
		bool Caret::canPaste(bool useKillRing) const BOOST_NOEXCEPT {
			if(!useKillRing) {
				try {
					return canPastePlatformData();
				} catch(...) {
					return false;
				}
			} else if(const texteditor::Session* const session = document().session())
				return session->killRing().numberOfKills() != 0;
			return false;
		}

		/// 対括弧の追跡を更新する
		void Caret::checkMatchBrackets() {
//			bool matched;
			boost::optional<std::pair<kernel::Position, kernel::Position>> oldPair(context_.matchBrackets);
			// TODO: implement matching brackets checking
/*			if(!isSelectionEmpty() || matchBracketsTrackingMode_ == DONT_TRACK)
				matched = false;
			else if(matched = getViewer().searchMatchBracket(getPosition(), matchBrackets_.first, true))
				matchBrackets_.second = getPosition();
			else if(matchBracketsTrackingMode_ == TRACK_FOR_SURROUND_CHARACTERS && !isStartOfLine()) {	// 1文字前も調べる
				const String& line = document()->getLine(lineNumber());
				GraphemeBreakIterator i(line.data(), line.data() + line.length(), line.data() + columnNumber());
				if(matched = getViewer().searchMatchBracket(Position(lineNumber(), (--i).tell() - line.data()), matchBrackets_.first, true))
					matchBrackets_.second = Position(lineNumber(), i.tell() - line.data());
			}
			if(!matched)
				matchBrackets_.first = matchBrackets_.second = Position::INVALID_POSITION;
*/			// TODO: check if the pair is out of view.
			if(context_.matchBrackets != oldPair)
				matchBracketsChangedSignal_(*this, oldPair, false);
		}

		/// Returns the @c CharacterInputSignal signal connector.
		SignalConnector<Caret::CharacterInputSignal> Caret::characterInputSignal() BOOST_NOEXCEPT {
			return makeSignalConnector(characterInputSignal_);
		}

		/// Clears the selection. The anchor will move to the caret.
		void Caret::clearSelection() {
			endRectangleSelection();
			moveTo(hit());
		}

		/// @see detail#InputMethodEventHandler#commitString
		void Caret::commitString(widgetapi::event::InputMethodEvent& event) BOOST_NOEXCEPT {
			const auto commitString(event.commitString());
			assert(commitString != boost::none);
			if(!context_.inputMethodComposingCharacter)
				texteditor::commands::TextInputCommand(textArea().textViewer(), boost::get(commitString))();
			else {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				auto& d = document();
				try {
					d.insertUndoBoundary();
					d.replace(
						kernel::Region(
							insertionPosition(*this),
							static_cast<kernel::DocumentCharacterIterator&>(++kernel::DocumentCharacterIterator(d, insertionPosition(*this))).tell()),
						String(1, static_cast<Char>(static_cast<const MSG*>(event.native())->wParam)));
					d.insertUndoBoundary();
				} catch(const kernel::DocumentCantChangeException&) {
				}
#endif
				context_.inputMethodComposingCharacter = false;
			}
		}

		/// @see kernel#DocumentListener#documentAboutToBeChanged
		void Caret::documentAboutToBeChanged(const kernel::Document&, const kernel::DocumentChange&) {
			// does nothing
		}

		/// @see VisualPoint#documentAboutToBeChanged
		void Caret::documentAboutToBeChanged(const kernel::DocumentChange& change) {
			assert(context_.anchorDestination == boost::none);
			if(anchor_ != boost::none)
				context_.anchorDestination = locations::updateTextHit(boost::get(anchor_), document(), change, gravity());
			VisualPoint::documentAboutToBeChanged(change);
		}

		/// @see VisualPoint#documentChanged
		void Caret::documentChanged(const kernel::DocumentChange& change) {
			VisualPoint::documentChanged(change);
		}

		/// @see kernel#DocumentListener#documentChanged
		void Caret::documentChanged(const kernel::Document&, const kernel::DocumentChange&) {
			context_.yanking = false;
			updateVisualAttributes();
		}

		/**
		 * Ends the rectangular selection.
		 * @see #beginRectangleSelection, #isSelectionRectangle
		 */
		void Caret::endRectangleSelection() {
			if(isTextAreaDisposed())
				throw TextAreaDisposedException();
			if(context_.selectedRectangle.get() != nullptr) {
				context_.selectedRectangle.reset();
				selectionShapeChangedSignal_(*this);
			}
		}
#if 0
		/**
		 * Deletes the selected text. This method ends the rectangle selection mode.
		 * @throw DocumentAccessViolationException The selected region intersects the inaccesible region
		 * @throw ... Any exceptions @c Document#erase throws
		 */
		void Caret::eraseSelection() {
			verifyViewer();
			Document& doc = *document();
			if(isSelectionEmpty())
				return;
			else if(!isSelectionRectangle()) {	// the selection is linear
				const Position to(min<Position>(*anchor_, *this));
				doc.erase(*anchor_, *this);	// this can throw all exceptions this method throws
				moveTo(to);
			} else {	// the selection is rectangle
				DocumentLocker lock(*document());
				const Position resultPosition(beginning());
				const bool adapts = adaptsToDocument();
				adaptToDocument(false);
				const Index firstLine = beginning().lineNumber(), lastLine = end().lineNumber();
				pair<Index, Index> rangeInLine;
				bool interrupted = false;

				// rectangle deletion can't delete newline characters
				{
					AutoFreeze af(&textViewer());
					if(textViewer().configuration().lineWrap.wraps()) {	// ...and the lines are wrapped
						// hmmm..., this is heavy work
						vector<Point*> points;
						vector<Index> sizes;
						points.reserve((lastLine - firstLine) * 2);
						sizes.reserve((lastLine - firstLine) * 2);
						const TextRenderer& renderer = textViewer().textRenderer();
						for(Index line = kernel::line(resultPosition); line <= lastLine; ++line) {
							const LineLayout& layout = renderer.lineLayout(line);
							for(Index subline = 0; subline < layout.numberOfSublines(); ++subline) {
								box_->overlappedSubline(line, subline, rangeInLine.first, rangeInLine.second);
								points.push_back(new Point(doc, Position(line, rangeInLine.first)));
								sizes.push_back(rangeInLine.second - rangeInLine.first);
							}
						}
						const std::size_t sublines = points.size();
						for(std::size_t i = 0; i < sublines; ++i) {
							if(!interrupted) {
								try {
									doc.erase(Position(points[i]->lineNumber(), points[i]->columnNumber()),
										Position(points[i]->lineNumber(), points[i]->columnNumber() + sizes[i]));
								} catch(...) {
									if(i == 0) {
										while(i < sublines)
											delete points[i++];
										adaptToDocument(adapts);
										throw;
									}
								}
							}
							delete points[i];
						}
					} else {
						for(Index line = kernel::line(resultPosition); line <= lastLine; ++line) {
							box_->overlappedSubline(line, 0, rangeInLine.first, rangeInLine.second);
							try {
								doc.erase(Position(line, rangeInLine.first), Position(line, rangeInLine.second));
							} catch(...) {
								if(line == kernel::line(resultPosition)) {
									adaptToDocument(adapts);
									throw;
								}
							}
						}
					}
				}
				adaptToDocument(adapts);
				endRectangleSelection();
				moveTo(resultPosition);
			}
		}
#endif
		/**
		 * Moves to the specified position without the anchor adapting.
		 * @param to The destination position
		 */
		void Caret::extendSelectionTo(const TextHit& to) {
			context_.anchorDestination = anchor();
			try {
				moveTo(to);
			} catch(...) {
				context_.anchorDestination = boost::none;
				throw;
			}
			context_.anchorDestination = boost::none;
		}

		/**
		 * Moves to the specified position without the anchor adapting.
		 * @param to The destination position
		 */
		void Caret::extendSelectionTo(const VisualDestinationProxy& to) {
			context_.anchorDestination = anchor();
			try {
				moveTo(to);
			} catch(...) {
				context_.anchorDestination = boost::none;
				throw;
			}
			context_.anchorDestination = boost::none;
		}

		/// @internal Invokes @c MotionSignal.
		inline void Caret::fireCaretMoved(const SelectedRegion& regionBeforeMotion) {
#ifdef ASCENSION_USE_SYSTEM_CARET
			if(!isTextViewerDisposed() && !textViewer().isFrozen() && (widgetapi::hasFocus(textViewer()) /*|| widgetapi::hasFocus(*completionWindow_)*/))
				updateLocation();
#endif
			motionSignal_(*this, regionBeforeMotion);
		}

		/**
		 * Hides the caret.
		 * @see #show, #shows
		 */
		void Caret::hide() BOOST_NOEXCEPT {
			if(painter_.get() != nullptr)
				painter_->hide();
		}

		namespace {
			/**
			 * @internal Deletes the forward one character and inserts the specified text.
			 * This function emulates keyboard overtyping input.
			 * @param caret The caret
			 * @param text The text to insert
			 * @param keepNewline Set @c false to overwrite a newline characer
			 * @throw NullPointerException @a text is @c null
			 * @throw DocumentDisposedException
			 * @throw TextViewerDisposedException
			 * @throw ... Any exceptions @c Document#replace throws
			 */
			void destructiveInsert(Caret& caret, const StringPiece& text, bool keepNewline = true) {
				if(text.cbegin() == nullptr || text.cend() == nullptr)
					throw NullPointerException("text");
				const boost::optional<kernel::AbstractPoint::AdaptationLevel> adaptationLevel(caret.adaptationLevel());
				caret.setAdaptationLevel(boost::none);
				const auto p(insertionPosition(caret));
				kernel::Position e((keepNewline && kernel::locations::isEndOfLine(caret)) ?
					p : kernel::locations::nextCharacter(caret, Direction::forward(), kernel::locations::GRAPHEME_CLUSTER));
				if(e != p) {
					try {
						e = caret.document().replace(kernel::Region(p, e), text);
					} catch(...) {
						caret.setAdaptationLevel(adaptationLevel);
						throw;
					}
					caret.moveTo(TextHit::leading(e));
				}
				caret.setAdaptationLevel(adaptationLevel);
			}
		} // namespace @0

		/**
		 * Inputs the specified character at current position.
		 * <p>If the selection is not empty, replaces the selected region. Otherwise if in overtype mode, replaces a
		 * character at current position (but this does not erase newline character).</p>
		 * <p>This method may insert undo boundaries for compound typing.</p>
		 * @param character The code point of the character to input
		 * @param validateSequence Set @c true to perform input sequence check using the active ISC. See
		 *                         @c texteditor#InputSequenceCheckers
		 * @param blockControls Set @c true to refuse any ASCII control characters except HT (U+0009), RS (U+001E) and
		 *                      US (U+001F)
		 * @retval true Succeeded
		 * @retval false The input was rejected by the input sequence validation (when @a validateSequence was @c true)
		 * @return false @c character was control character and blocked (when @a blockControls was @c true)
		 * @throw ... Any exceptions @c Document#insertUndoBoundary and @c Caret#replaceSelection throw
		 * @see #isOvertypeMode, #setOvertypeMode, texteditor#commands#TextInputCommand
		 */
		bool Caret::inputCharacter(CodePoint character, bool validateSequence /* = true */, bool blockControls /* = true */) {
			// check blockable control character
			static const std::array<CodePoint, 3> SAFE_CONTROLS = {0x0009u, 0x001eu, 0x001fu};
			if(blockControls && character <= 0x00ffu
					&& (iscntrl(static_cast<int>(character)) != 0)
					&& !boost::binary_search(SAFE_CONTROLS, character))
				return false;

			// check the input sequence
			kernel::Document& doc = document();
			if(validateSequence) {
				if(const texteditor::Session* const session = doc.session()) {
					if(const std::shared_ptr<const texteditor::InputSequenceCheckers> checker = session->inputSequenceCheckers()) {
						const auto ip(insertionPosition(document(), beginning()));
						const Char* const lineString = doc.lineString(kernel::line(ip)).data();
						if(!checker->check(StringPiece(lineString, kernel::offsetInLine(ip)), character)) {
							eraseSelection(*this);
							return false;	// invalid sequence
						}
					}
				}
			}

			Char buffer[2];
			{
				Char* out = buffer;
				text::utf::checkedEncode(character, out);
			}
			if(!isSelectionEmpty(*this)) {	// just replace if the selection is not empty
				doc.insertUndoBoundary();
				replaceSelection(StringPiece(buffer, (character < 0x10000u) ? 1 : 2));
				doc.insertUndoBoundary();
			} else if(overtypeMode_) {
				prechangeDocument();
				doc.insertUndoBoundary();
				destructiveInsert(*this, StringPiece(buffer, (character < 0x10000u) ? 1 : 2));
				doc.insertUndoBoundary();
			} else {
				const bool alpha = kernel::detail::identifierSyntax(*this).isIdentifierContinueCharacter(character);
				if(context_.lastTypedPosition != boost::none && (!alpha || boost::get(context_.lastTypedPosition) != insertionPosition(*this))) {
					// end sequential typing
					doc.insertUndoBoundary();
					context_.lastTypedPosition = boost::none;
				}
				if(alpha && !context_.lastTypedPosition)
					// (re)start sequential typing
					doc.insertUndoBoundary();

				ascension::detail::ValueSaver<bool> lock(context_.typing);
				context_.typing = true;
				replaceSelection(StringPiece(buffer, (character < 0x10000u) ? 1 : 2));	// this may throw
				if(alpha)
					context_.lastTypedPosition = insertionPosition(*this);
			}

			characterInputSignal_(*this, character);
			return true;
		}

		/// Returns the @c InputModeChangedSignal signal connector.
		SignalConnector<Caret::InputModeChangedSignal> Caret::inputModeChangedSignal() BOOST_NOEXCEPT {
			return makeSignalConnector(inputModeChangedSignal_);
		}

		/// @see VisualPoint#install
		void Caret::install(TextArea& textArea) {
			const bool installed = isInstalled();
			VisualPoint::install(textArea);
			setPainter(std::unique_ptr<CaretPainter>());
			show();
		}

		/// Returns the @c MatchBracketsChangedSignal signal connector.
		SignalConnector<Caret::MatchBracketsChangedSignal> Caret::matchBracketsChangedSignal() BOOST_NOEXCEPT {
			return makeSignalConnector(matchBracketsChangedSignal_);
		}

		/// Returns the @c MotionSignal signal connector.
		SignalConnector<Caret::MotionSignal> Caret::motionSignal() BOOST_NOEXCEPT {
			return makeSignalConnector(motionSignal_);
		}

		/// @see VisualPoint#moved
		void Caret::moved(const TextHit& from) BOOST_NOEXCEPT {
			const SelectedRegion selectionBeforeMotion(_document = document(), _anchor = insertionPosition(document(), anchor()), _caret = from);
			VisualPoint::moved(from);
			anchor_ = context_.anchorDestination;
			context_.anchorDestination = boost::none;
			if(anchor_ != boost::none && boost::get(anchor_) == hit())
				anchor_ = boost::none;
			fireCaretMoved(selectionBeforeMotion);
			if(!document().isChanging())
				updateVisualAttributes();
		}

		/**
		 * Paints the figure of the caret.
		 * @param context The painting context
		 */
		void Caret::paint(graphics::PaintContext& context) const {
			if(painter_.get() != nullptr) {
				const auto ip(insertionPosition(*this));
				if(const graphics::font::TextLayout* const layout = textArea().textRenderer()->layouts().at(kernel::line(ip)))
					painter_->paintIfShows(context, *layout,
						modelToView(textArea().textViewer(), graphics::font::TextHit<kernel::Position>::leading(kernel::Position::bol(ip))));
			}
		}

		/// @internal Should be called before change the document.
		inline void Caret::prechangeDocument() {
			if(context_.lastTypedPosition && !context_.typing) {
				document().insertUndoBoundary();
				context_.lastTypedPosition = boost::none;
			}
		}

		/// @see detail#InputMethodEvent#preeditEnded
		void Caret::preeditEnded() BOOST_NOEXCEPT {
			context_.inputMethodCompositionActivated = false;
			updateVisualAttributes();
		}

		/// @see detail#InputMethodEventHandler#preeditStarted
		void Caret::preeditStarted() BOOST_NOEXCEPT {
			context_.inputMethodCompositionActivated = true;
			adjustInputMethodCompositionWindow();
			utils::closeCompletionProposalsPopup(textArea().textViewer());
		}

		// @see detail#InputMethodQueryEvent#querySurroundingText
		std::pair<const StringPiece, StringPiece::const_iterator> Caret::querySurroundingText() const BOOST_NOEXCEPT {
			const StringPiece lineString(document().lineString(kernel::line(hit().characterIndex())));
			StringPiece::const_iterator position(lineString.cbegin());
			if(boost::size(selectedRegion().lines()) == 1)
				position += kernel::offsetInLine(insertionPosition(document(), beginning()));
			return std::make_pair(lineString, position);
		}

		/**
		 * Replaces the selected region with the specified text.
		 * If the selection is empty, inserts the text at current position.
		 * @param text The text to insert
		 * @param rectangleInsertion Set @c true to insert text as rectangle
		 * @throw NullPointerException @a text is @c null
		 * @throw ... Any exceptions @c Document#replace throws
		 */
		void Caret::replaceSelection(const StringPiece& text, bool rectangleInsertion /* = false */) {
			kernel::Position e;
			prechangeDocument();
			if(!isSelectionRectangle() && !rectangleInsertion)
				e = document().replace(selectedRegion(), text);
			else {
				// TODO: not implemented.
				return;
			}
			moveTo(TextHit::leading(e));
		}
#if 0
		/**
		 * Recreates and shows the caret. If the text viewer does not have focus, nothing heppen.
		 * @see #updateLocation
		 */
		void Caret::resetVisualization() {
			TextViewer& viewer = textViewer();
			if(!widgetapi::hasFocus(viewer))
				return;

			std::unique_ptr<graphics::Image> image;
			graphics::geometry::BasicPoint<std::uint32_t> alignmentPoint;
			bool invisible = false;

			if(context_.inputMethodComposingCharacter) {
				const bool horizontal = isHorizontal(viewer.textRenderer().computedBlockFlowDirection());
				if(const boost::optional<graphics::Rectangle> bounds = currentCharacterLogicalBounds(*this)) {
					image.reset(
						new graphics::Image(graphics::geometry::BasicDimension<std::uint32_t>(
							static_cast<std::uint32_t>(graphics::geometry::dx(*bounds)),
							static_cast<std::uint32_t>(graphics::geometry::dy(*bounds))),
							graphics::Image::RGB16));
					alignmentPoint = graphics::geometry::negate(graphics::geometry::topLeft(*bounds));
				} else
					invisible = true;
			} else if(context_.inputMethodCompositionActivated)
				invisible = true;
			else if(shaper_.get() != nullptr)
				shaper_->shape(image, alignmentPoint);
			else {
				DefaultCaretShaper s;
				CaretShapeUpdater u(*this);
				static_cast<CaretShaper&>(s).install(u);
				static_cast<CaretShaper&>(s).shape(image, alignmentPoint);
				static_cast<CaretShaper&>(s).uninstall();
			}
			if(invisible) {
				image.reset(new graphics::Image(boost::geometry::make_zero<graphics::geometry::BasicDimension<std::uint32_t>>(), graphics::Image::RGB16));
				boost::geometry::assign_zero(alignmentPoint);
			}
			assert(image.get() != nullptr);

#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			::DestroyCaret();
			::CreateCaret(viewer.handle().get(), image->asNativeObject().get(), 0, 0);
			::ShowCaret(viewer.handle().get());
#else
			ASCENSION_CANT_DETECT_PLATFORM();
#endif

			shapeCache_.image = std::move(image);
			shapeCache_.alignmentPoint = alignmentPoint;
			updateLocation();
		}
#endif
		/**
		 * Selects the specified region. The active selection mode will be cleared.
		 * @param region The region to select
		 * @throw kernel#BadRegionException @a region intersects with outside of the document
		 */
		void Caret::select(const SelectedRegion& region) {
			if(isTextAreaDisposed())
				throw TextAreaDisposedException();
			else if(!encompasses(document().region(), static_cast<const kernel::Region&>(region)))
				throw kernel::BadRegionException(region);
			context_.yanking = false;
			if(region.anchor() != insertionPosition(document(), anchor()) || region.caret() != hit()) {
				context_.anchorDestination = TextHit::leading(region.anchor());
				VisualPoint::moveTo(region.caret());	// TODO: this may throw...
			}
		}

		/**
		 * @fn ascension::viewer::Caret::select
		 * Selects the specified region.
		 * @param anchor The named parameter describes the anchor of the new selection
		 * @param caret The named parameter describes the caret of the new selection
		 */

		/// @internal
		void Caret::_select(const kernel::Position& a, const TextHit& c) {
			select(SelectedRegion(_document = document(), _anchor = a, _caret = c));
		}

		/// @internal
		SelectedRegion Caret::selection() const BOOST_NOEXCEPT {
			return SelectedRegion(_document = document(), _anchor = insertionPosition(document(), anchor()), _caret = hit());
		}

		/// Returns the @c SelectionShapeChangedSignal signal connector.
		SignalConnector<Caret::SelectionShapeChangedSignal> Caret::selectionShapeChangedSignal() BOOST_NOEXCEPT {
			return makeSignalConnector(selectionShapeChangedSignal_);
		}

		/**
		 * Sets character input mode.
		 * @param overtype Set @c true to set to overtype mode, @c false to set to insert mode
		 * @return This caret
		 * @see #inputCharacter, #isOvertypeMode
		 */
		Caret& Caret::setOvertypeMode(bool overtype) BOOST_NOEXCEPT {
			if(overtype != overtypeMode_) {
				overtypeMode_ = overtype;
				inputModeChangedSignal_(*this, OVERTYPE_MODE);
			}
			return *this;
		}

		/**
		 * Sets the caret painter.
		 * @param newPainter The new caret painter to set. If this parameter is @c null, the @c Caret instance uses a
		 *                   default-constructed @c StandardCaretPainter object instead
		 */
		void Caret::setPainter(std::unique_ptr<CaretPainter> newPainter) {
			if(newPainter == painter_ && newPainter.get() != nullptr)
				return;
			if(painter_.get() != nullptr)
				painter_->uninstall(*this);	// TODO: Support multiple carets.
			if(newPainter.get() == nullptr)
				newPainter.reset(new StandardCaretPainter());
			painter_ = std::move(newPainter);
			painter_->install(*this);	// TODO: Support multiple carets.
		}

		/**
		 * Shows (and may begin blinking) the hidden caret.
		 * @see #hide, #shows
		 */
		void Caret::show() BOOST_NOEXCEPT {
			if(painter_.get() != nullptr)
				painter_->show();
		}

		/// @see VisualPoint#uninstall
		void Caret::uninstall() BOOST_NOEXCEPT {
			try {
				painter_.reset();
				shapeCache_.image.reset();
				context_.Context::Context();
			} catch(...) {
				// ignore the error
			}
			VisualPoint::uninstall();
		}
#if 0
		/**
		 * Moves the caret to valid position with current position, scroll context, and the fonts.
		 * @see #resetVisualization
		 */
		void Caret::updateLocation() {
			const TextViewer& viewer = textViewer();
			if(!widgetapi::hasFocus(viewer) || viewer.isFrozen())
				return;

			const std::shared_ptr<const graphics::font::TextViewport> viewport(textViewer().textRenderer().viewport());
//			graphics::font::TextViewportNotificationLocker lock(viewport.get());
			graphics::Point p(modelToView(textViewer(), TextHit::leading(*this)));
			const graphics::Rectangle contentRectangle(viewer.textAreaContentRectangle());
			assert(graphics::geometry::isNormalized(contentRectangle));

			boost::geometry::model::d2::point_xy<int> newLocation(static_cast<int>(graphics::geometry::x(p)), static_cast<int>(graphics::geometry::y(p)));
			if(!graphics::geometry::within(p, contentRectangle)) {
				// "hide" the caret
				const int linePitch = static_cast<int>(widgetapi::createRenderingContext(viewer)->fontMetrics(viewer.textRenderer().defaultFont())->linePitch());
				if(isHorizontal(textViewer().textRenderer().computedBlockFlowDirection()))
					boost::geometry::assign_values(newLocation, static_cast<int>(graphics::geometry::x(p)), -linePitch);
				else
					boost::geometry::assign_values(newLocation, -linePitch, static_cast<int>(graphics::geometry::y(p)));
			} else
				boost::geometry::assign_values(newLocation,
					static_cast<int>(graphics::geometry::x(p)) - graphics::geometry::x(shapeCache_.alignmentPoint),
					static_cast<int>(graphics::geometry::y(p)) - graphics::geometry::y(shapeCache_.alignmentPoint));
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			::SetCaretPos(boost::geometry::get<0>(newLocation), boost::geometry::get<1>(newLocation));
#else
			ASCENSION_CANT_DETECT_PLATFORM();
#endif
			adjustInputMethodCompositionWindow();
		}
#endif
		inline void Caret::updateVisualAttributes() {
			if(isSelectionRectangle())
				context_.selectedRectangle->update(selectedRegion());
			if(autoShow_)
				utils::show(*this);
			checkMatchBrackets();
		}
	}
}
