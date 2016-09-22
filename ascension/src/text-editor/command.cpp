/**
 * @file command.cpp
 * @author exeal
 * @date 2006-2011 was text-editor.cpp
 * @date 2011-05-06
 * @date 2011-2014, 2016
 */

#include <ascension/content-assist/content-assist.hpp>
#include <ascension/corelib/numeric-range-algorithm/encompasses.hpp>
#include <ascension/corelib/numeric-range-algorithm/intersection.hpp>
#include <ascension/corelib/text/word-break-iterator.hpp>
#include <ascension/graphics/font/text-layout.hpp>
#include <ascension/graphics/font/text-viewport.hpp>
#include <ascension/kernel/document.hpp>
#include <ascension/kernel/document-input.hpp>
#include <ascension/text-editor/command.hpp>
#include <ascension/text-editor/session.hpp>
#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/text-area.hpp>
#include <ascension/viewer/text-viewer.hpp>
#include <ascension/viewer/text-viewer-utility.hpp>
#include <ascension/viewer/visual-locations.hpp>
#include <ascension/win32/ui/wait-cursor.hpp>

namespace ascension {
	namespace texteditor {
		// Command ////////////////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * Protected constructor.
		 * @param viewer The target text viewer
		 */
		Command::Command(viewer::TextViewer& viewer) BOOST_NOEXCEPT : viewer_(&viewer), numericPrefix_(1) {
		}

		/// Destructor.
		Command::~Command() BOOST_NOEXCEPT {
		}

		namespace commands {
			namespace {
				inline bool abortModes(viewer::TextViewer& target) {
					viewer::utils::closeCompletionProposalsPopup(target);
					return abortIncrementalSearch(*viewer::document(target));
				}
			}

#define ASCENSION_ASSERT_IFISWINDOW() assert(true/*target().isWindow()*/)

// the command can't perform and throw if the document is read only
#define ASCENSION_CHECK_DOCUMENT_READ_ONLY()	\
	if(viewer::document(target())->isReadOnly()) return false

			/**
			 * Constructor.
			 * @param viewer The target text viewer
			 * @param region The region to operate on. If empty, the accessible region of the document. This
			 *               region will be shrunk to the accessible region when the command performed
			 */
			BookmarkMatchLinesCommand::BookmarkMatchLinesCommand(viewer::TextViewer& viewer,
					const kernel::Region& region /* = Region() */) BOOST_NOEXCEPT : Command(viewer), region_(region), numberOfMarkedLines_(0) {
			}

			/// Returns the number of the previously marked lines.
			Index BookmarkMatchLinesCommand::numberOfMarkedLines() const BOOST_NOEXCEPT {
				return numberOfMarkedLines_;
			}

			/**
			 * Implements @c Command#perform.
			 * @retval false The pattern to search was not set
			 * @throw ... Any exceptions specified @c TextSearcher#search other than @c BadPositionException and
			 *            @c IllegalStateException. If threw, the marking is interrupted
			 */
			bool BookmarkMatchLinesCommand::perform() {
				win32::WaitCursor wc;
				viewer::TextViewer& viewer = target();
				const auto document(viewer::document(viewer));
				const searcher::TextSearcher* s;
				if(const Session* const session = document->session())
					s = &session->textSearcher();
				else
					return true;	// TODO: prepares a default text searcher.
				if(!s->hasPattern())
					return false;

				numberOfMarkedLines_ = 0;
				kernel::Region scope;
				if(boost::empty(region_))
					scope = document->accessibleRegion();
				else {
					auto temp(intersection(region_, document->accessibleRegion()));
					if(temp == boost::none)
						return true;
					scope = boost::get(temp);
				}

				kernel::Bookmarker& bookmarker = document->bookmarker();
				kernel::Region matchedRegion;
				while(s->search(*document,
						std::max<kernel::Position>(*boost::const_begin(viewer.textArea()->caret()->selectedRegion()), *boost::const_begin(document->accessibleRegion())),
						scope, Direction::forward(), matchedRegion)) {
					bookmarker.mark(kernel::line(*boost::const_begin(matchedRegion)));
					scope = kernel::Region(kernel::Position::bol(kernel::line(*boost::const_begin(matchedRegion)) + 1), *boost::const_end(scope));
					++numberOfMarkedLines_;
				}
				return true;
			}

			/**
			 * Constructor.
			 * @param viewer The target text viewer
			 */
			CancelCommand::CancelCommand(viewer::TextViewer& viewer) BOOST_NOEXCEPT : Command(viewer) {
			}

			/**
			 * Implements @c Command#perform.
			 * @return true
			 */
			bool CancelCommand::perform() {
				ASCENSION_ASSERT_IFISWINDOW();
				abortModes(target());
				target().textArea()->caret()->clearSelection();
				return true;
			}

			/**
			 * Constructor.
			 * @param viewer The target text viewer
			 * @param procedure A function gives a motion
			 * @param direction The direction of motion
			 * @param extendSelection Set @c true to extend the selection
			 * @throw NullPointerException @a procedure is @c null
			 */
			template<typename ProcedureSignature>
			CaretMovementCommand<ProcedureSignature>::CaretMovementCommand(
					viewer::TextViewer& viewer, ProcedureSignature* procedure,
					Direction direction, bool extendSelection /* = false */)
					: Command(viewer), procedure_(procedure), direction_(direction), extends_(extendSelection) {
				if(procedure == nullptr)
					throw NullPointerException("procedure");
			}

			namespace {
				template<typename CaretMovementProcedure>
				inline bool selectCompletionProposal(viewer::TextViewer&, CaretMovementProcedure*, Direction, long) {
					return false;
				}
				inline bool selectCompletionProposal(viewer::TextViewer& target, kernel::Position(*procedure)(const kernel::locations::PointProxy&, Direction, Index), Direction direction, long n) {
					if(procedure == &kernel::locations::nextLine) {
						if(contentassist::ContentAssistant* const ca = target.contentAssistant()) {
							if(contentassist::ContentAssistant::CompletionProposalsUI* const cpui = ca->completionProposalsUI())
								return cpui->nextProposal((direction == Direction::forward()) ? +n : -n), true;
						}
					}
					return false;
				}
				inline bool selectCompletionProposal(viewer::TextViewer& target, viewer::VisualDestinationProxy(*procedure)(const viewer::VisualPoint&, Direction, Index), Direction direction, long n) {
					if(procedure == &viewer::locations::nextPage || procedure == &viewer::locations::nextVisualLine) {
						if(contentassist::ContentAssistant* const ca = target.contentAssistant()) {
							if(contentassist::ContentAssistant::CompletionProposalsUI* const cpui = ca->completionProposalsUI()) {
								if(procedure == &viewer::locations::nextPage)
									return cpui->nextPage((direction == Direction::forward()) ? +n : -n), true;
								else
									return cpui->nextProposal((direction == Direction::forward()) ? +n : -n), true;
							}
						}
					}
					return false;
				}

				template<typename ProcedureSignature>
				inline bool moveToBoundOfSelection(viewer::Caret&, ProcedureSignature*, Direction) {
					return false;
				}
				inline bool moveToBoundOfSelection(viewer::Caret& caret, kernel::Position(*procedure)(const kernel::locations::PointProxy&, Direction direction, kernel::locations::CharacterUnit, Index), Direction direction) {
					if(procedure == &kernel::locations::nextCharacter) {
						const auto selection(caret.selectedRegion());
						const kernel::Position destination((direction == Direction::forward()) ? *boost::const_end(selection) : *boost::const_begin(selection));
						return caret.moveTo(graphics::font::TextHit<kernel::Position>::leading(destination)), true;
					}
					return false;
				}

				template<typename ProcedureSignature>
				inline void scrollTextViewer(viewer::TextViewer&, ProcedureSignature, Direction, long) {
				}
				inline void scrollTextViewer(viewer::TextViewer& target, viewer::VisualDestinationProxy(*procedure)(const viewer::VisualPoint&, Direction, Index), Direction direction, long n) {
					// TODO: consider the numeric prefix.
					if(procedure == &viewer::locations::nextPage) {
						graphics::font::TextViewport::SignedScrollOffset offset = (direction == Direction::forward()) ? n : -n;
						if(offset != 0) {
							presentation::FlowRelativeTwoAxes<graphics::font::TextViewport::SignedScrollOffset> delta;
							delta.bpd() = offset;
							delta.ipd() = 0;
							target.textArea()->viewport()->scroll(delta);
						}
					}
				}

				template<typename PointType>
				inline void moveCaret(viewer::Caret& caret, kernel::Position(*procedure)(const PointType&), Index, bool extend) {
					const auto h(viewer::TextHit::leading((*procedure)(caret)));
					if(!extend)
						caret.moveTo(h);
					else
						caret.extendSelectionTo(h);
				}
				template<typename PointType>
				inline void moveCaret(viewer::Caret& caret, kernel::Position(*procedure)(const PointType&, Direction, Index), Direction direction, Index n, bool extend) {
					const auto h(viewer::TextHit::leading((*procedure)(caret, direction, n)));
					if(!extend)
						caret.moveTo(h);
					else
						caret.extendSelectionTo(h);
				}
				inline void moveCaret(viewer::Caret& caret, boost::optional<kernel::Position>(*procedure)(const kernel::locations::PointProxy&, Direction, Index), Direction direction, Index n, bool extend) {
					if(const boost::optional<kernel::Position> destination = (*procedure)(caret, direction, n)) {
						const auto h(viewer::TextHit::leading(boost::get(destination)));
						if(!extend)
							caret.moveTo(h);
						else
							caret.extendSelectionTo(h);
					}
				}
				template<typename PointType>
				inline void moveCaret(viewer::Caret& caret, kernel::Position(*procedure)(const PointType&, Direction, kernel::locations::CharacterUnit, Index), Direction direction, Index n, bool extend) {
					const auto h(viewer::TextHit::leading((*procedure)(caret, direction, kernel::locations::GRAPHEME_CLUSTER, n)));
					if(!extend)
						caret.moveTo(h);
					else
						caret.extendSelectionTo(h);
				}
				inline void moveCaret(viewer::Caret& caret, viewer::VisualDestinationProxy(*procedure)(const viewer::locations::PointProxy&, Direction, Index), Direction direction, Index n, bool extend) {
					if(!extend)
						caret.moveTo((*procedure)(caret, direction, n));
					else
						caret.extendSelectionTo((*procedure)(caret, direction, n));
				}
				inline void moveCaret(viewer::Caret& caret, viewer::VisualDestinationProxy(*procedure)(const viewer::VisualPoint&, Direction, Index), Direction direction, Index n, bool extend) {
					if(!extend)
						caret.moveTo((*procedure)(caret, direction, n));
					else
						caret.extendSelectionTo((*procedure)(caret, direction, n));
				}
			}

			// explicit instantiations
			template class CaretMovementCommand<kernel::Position(const kernel::locations::PointProxy&, Direction, Index)>;	// next(Line|Word|WordEnd)
			template class CaretMovementCommand<boost::optional<kernel::Position>(const kernel::locations::PointProxy&, Direction, Index)>;	// nextBookmark
			template class CaretMovementCommand<kernel::Position(const kernel::locations::PointProxy&, Direction, kernel::locations::CharacterUnit, Index)>;	// nextCharacter
			template class CaretMovementCommand<viewer::VisualDestinationProxy(const viewer::locations::PointProxy&, Direction, Index)>;	// nextPage
			template class CaretMovementCommand<viewer::VisualDestinationProxy(const viewer::VisualPoint&, Direction, Index)>;	// nextVisualLine

			/**
			 * Moves the caret or extends the selection.
			 * @return true if succeeded
			 */
			template<typename ProcedureSignature>
			bool CaretMovementCommand<ProcedureSignature>::perform() {
				const NumericPrefix n = numericPrefix();
				endIncrementalSearch(*viewer::document(target()));
				if(n == 0)
					return true;

				if(const auto textArea = target().textArea()) {
					if(const auto caret = textArea->caret()) {
						if(!extends_) {
							if(selectCompletionProposal(target(), procedure_, direction_, n))
								return true;
							caret->endRectangleSelection();
							if(!viewer::isSelectionEmpty(*caret)) {	// just clear the selection
								if(moveToBoundOfSelection(*caret, procedure_, direction_))
									return true;
							}
						}

						scrollTextViewer(target(), procedure_, direction_, n);
						moveCaret(*caret, procedure_, direction_, n, extends_);
						return true;
					}
				}
				return false;
			}

			/**
			 * Constructor.
			 * @param viewer The target text viewer
			 * @param procedure A function gives a motion
			 * @param extendSelection Set @c true to extend the selection
			 * @throw NullPointerException @a procedure is @c null
			 */
			template<typename ProcedureSignature>
			CaretMovementToDefinedPositionCommand<ProcedureSignature>::CaretMovementToDefinedPositionCommand(
					viewer::TextViewer& viewer, ProcedureSignature* procedure, bool extendSelection /* = false */)
					: Command(viewer), procedure_(procedure), extends_(extendSelection) {
				if(procedure == nullptr)
					throw NullPointerException("procedure");
			}

			namespace {
				inline viewer::TextHit makeNormalHit(const kernel::Position& p) BOOST_NOEXCEPT {
					return viewer::TextHit::leading(p);
				}
				inline viewer::TextHit makeNormalHit(const viewer::TextHit& h) BOOST_NOEXCEPT {
					return h;
				}
			}

			// explicit instantiations
			template class CaretMovementToDefinedPositionCommand<kernel::Position(const kernel::locations::PointProxy&)>;	// (beginning|end)Of(Document|Line)
			template class CaretMovementToDefinedPositionCommand<kernel::Position(const viewer::locations::PointProxy&)>;	// contextual(Beginning|End)OfLine, (beginning|end|contextualBeginning|contextualEnd)OfVisualLine, (first|last)PrintableCharacterOf(Visual)?Line

			/**
			 * Moves the caret or extends the selection.
			 * @return true if succeeded
			 */
			template<typename ProcedureSignature>
			bool CaretMovementToDefinedPositionCommand<ProcedureSignature>::perform() {
				endIncrementalSearch(*viewer::document(target()));
				if(const auto textArea = target().textArea()) {
					if(const auto caret = textArea->caret()) {
						const auto h(makeNormalHit((*procedure_)(*caret)));
						if(!extends_)
							caret->moveTo(h);
						else
							caret->extendSelectionTo(h);
						return true;
					}
				}
				return false;
			}

			/**
			 * Constructor.
			 * @param viewer The target text viewer
			 * @param direction The direcion to delete
			 */
			CharacterDeletionCommand::CharacterDeletionCommand(viewer::TextViewer& viewer,
					Direction direction) BOOST_NOEXCEPT : Command(viewer), direction_(direction) {
			}

			/**
			 * Implements @c Command#perform.
			 * @retval false If the incremental search was active, couldn't undo. Otherwise the document was read only
			 *               or the region to delete was inaccessible
			 */
			bool CharacterDeletionCommand::perform() {
				NumericPrefix n = numericPrefix();
				if(n == 0)
					return true;
				viewer::TextViewer& textViewer = target();
				if(/*caret.isAutoCompletionRunning() &&*/ direction_ == Direction::forward())
					viewer::utils::closeCompletionProposalsPopup(textViewer);

				const auto document(viewer::document(textViewer));
				searcher::IncrementalSearcher* isearch = nullptr;
				if(Session* const session = document->session())
					isearch = &session->incrementalSearcher();
				if(isearch != nullptr && isearch->isRunning()) {
					if(direction_ == Direction::forward())	// delete the entire pattern
						isearch->reset();
					else {	// delete the last N characters (undo)
						if(!isearch->canUndo())
							return false;
						else {
							isearch->undo();
							for(--n; n > 0 && isearch->canUndo(); --n)
								isearch->undo();
						}
					}
				} else {
					ASCENSION_CHECK_DOCUMENT_READ_ONLY();
					document->insertUndoBoundary();
					auto caret(textViewer.textArea()->caret());
					if(n == 1 && !viewer::isSelectionEmpty(*caret)) {	// delete only the selected content
						try {
							viewer::eraseSelection(*caret);
						} catch(const kernel::DocumentInput::ChangeRejectedException&) {
							return false;
						}
					} else {
						viewer::AutoFreeze af((!viewer::isSelectionEmpty(*caret) || n > 1) ? &textViewer : nullptr);
						kernel::Region region(caret->selectedRegion());
						if(direction_ == Direction::forward())
							region = kernel::Region(
								*boost::const_begin(region),
								kernel::locations::nextCharacter(caret->end(),
									Direction::forward(), kernel::locations::GRAPHEME_CLUSTER, viewer::isSelectionEmpty(*caret) ? n : (n - 1)));
						else
							region = kernel::Region(
								kernel::locations::nextCharacter(caret->beginning(),
									Direction::backward(), kernel::locations::UTF32_CODE_UNIT, viewer::isSelectionEmpty(*caret) ? n : (n - 1)),
								*boost::const_end(region));
						try {
							kernel::erase(*document, region);
						} catch(const kernel::DocumentInput::ChangeRejectedException&) {
							return false;
						}
					}
				}
				return true;
			}

			/**
			 * Constructor.
			 * @param viewer The target text viewer
			 * @param c The code point of the character to input
			 * @throw text#InvalidScalarValueException @a c is not valid Unicode scalar value
			 */
			CharacterInputCommand::CharacterInputCommand(viewer::TextViewer& viewer, CodePoint c) : Command(viewer), c_(c) {
				if(!text::isScalarValue(c))
					throw text::InvalidScalarValueException(c);
			}

			/**
			 * Implements @c Command#perform.
			 * @retval false Failed and the incremental search is not active
			 * @retval 0 Otherwise
			 * @see Caret#inputCharacter, TextViewer#onChar, TextViewer#onUniChar
			 */
			bool CharacterInputCommand::perform() {
				if(numericPrefix() == 1) {
#if 0
					if(Session* const session = target().document().session()) {
						if(session->incrementalSearcher().isRunning()) {
							viewer::utils::closeCompletionProposalsPopup(target());
							if(c_ == 0x0009u || std::iswcntrl(static_cast<std::wint_t>(c_)) == 0)
								session->incrementalSearcher().addCharacter(c_);
							return true;
						}
					}
#endif
					try {
						return target().textArea()->caret()->inputCharacter(c_);
					} catch(const kernel::DocumentCantChangeException&) {
						return false;
					}
				} else {
					ASCENSION_CHECK_DOCUMENT_READ_ONLY();
					if(numericPrefix() > 0) {	// ...
						String s;
						text::utf::encode(c_, back_inserter(s));
						return TextInputCommand(target(), s).setNumericPrefix(numericPrefix())();
					}
					return true;
				}
			}

			/**
			 * Constructor.
			 * @param viewer The target text viewer
			 * @param fromPreviousLine Set @c true to use a character on the previous visual line. Otherwise
			 *                         one on the next visual line is used
			 */
			CharacterInputFromNextLineCommand::CharacterInputFromNextLineCommand(
					viewer::TextViewer& viewer, bool fromPreviousLine) BOOST_NOEXCEPT : Command(viewer), fromPreviousLine_(fromPreviousLine) {
			}

			/**
			 * Implements @c Command#perform.
			 * @retval false The caret was the first/last line in the document and couldn't copy a character from the
			 *               previous/next line. Or the next/previous line was too short to locate the character to
			 *               copy. Or internal performance of @c CharacterInputCommand failed
			 */
			bool CharacterInputFromNextLineCommand::perform() {
				abortIncrementalSearch(*viewer::document(target()));
				ASCENSION_CHECK_DOCUMENT_READ_ONLY();

				// TODO: recognizes narrowing.

				const std::shared_ptr<const viewer::VisualPoint> caret(target().textArea()->caret());

				if((fromPreviousLine_ && kernel::line(*caret) == 0)
						|| (!fromPreviousLine_ && kernel::line(*caret) >= viewer::document(target())->numberOfLines() - 1))
					return false;
	
				const auto p(viewer::insertionPosition(*viewer::document(target()),
					viewer::locations::nextVisualLine(*caret, fromPreviousLine_ ? Direction::backward() : Direction::forward())));
				const String& lineString = viewer::document(target())->lineString(kernel::line(*caret) + (fromPreviousLine_ ? -1 : 1));
				if(kernel::offsetInLine(p) >= lineString.length())
					return false;
				setNumericPrefix(1);
				return CharacterInputCommand(target(), text::utf::decodeFirst(std::begin(lineString) + kernel::offsetInLine(p), std::end(lineString)))();
			}

			/**
			 * Constructor.
			 * @param viewer The target text viewer
			 */
			CharacterToCodePointConversionCommand::CharacterToCodePointConversionCommand(viewer::TextViewer& viewer) BOOST_NOEXCEPT : Command(viewer) {
			}

			/**
			 * Implements @c Command#perform.
			 * @retval false The end of the selection is the beginning of the line and couldn't find the string to
			 *               convert
			 */
			bool CharacterToCodePointConversionCommand::perform() {
				ASCENSION_CHECK_DOCUMENT_READ_ONLY();
				viewer::TextViewer& viewer = target();
				abortModes(viewer);
				const auto document(viewer::document(viewer));
				const auto& peos = viewer.textArea()->caret()->end();
				const auto eos(*boost::const_end(viewer.textArea()->caret()->selectedRegion()));
				if(kernel::locations::isBeginningOfLine(peos) || (document->isNarrowed() && eos == *boost::const_begin(document->accessibleRegion())))
					return false;

				const auto caret(viewer.textArea()->caret());
				const String& lineString = document->lineString(kernel::line(eos));
				const CodePoint c = text::utf::decodeLast(std::begin(lineString), std::begin(lineString) + kernel::offsetInLine(eos));
				std::array<Char, 7> buffer;
#if(_MSC_VER < 1400)
				if(std::swprintf(buffer.data(), L"%lX", c) < 0)
#else
				if(std::swprintf(buffer.data(), buffer.size(), L"%lX", c) < 0)
#endif // _MSC_VER < 1400
					return false;
				viewer::AutoFreeze af(&viewer);
				caret->select(viewer::_anchor = kernel::Position(kernel::line(eos), kernel::offsetInLine(eos) - ((c > 0xffff) ? 2 : 1)), viewer::_caret = peos.hit());
				try {
					caret->replaceSelection(buffer.data(), false);
				} catch(const kernel::DocumentInput::ChangeRejectedException&) {
					return false;
				}
				return true;
			}

			/**
			 * Constructor.
			 * @param viewer The target text viewer
			 */
			CodePointToCharacterConversionCommand::CodePointToCharacterConversionCommand(viewer::TextViewer& viewer) BOOST_NOEXCEPT : Command(viewer) {
			}

			/**
			 * Implements @c Command#perform.
			 * @retval false The end of the selection is the beginning of the line and couldn't find the string
			 *               to convert
			 */
			bool CodePointToCharacterConversionCommand::perform() {
				ASCENSION_CHECK_DOCUMENT_READ_ONLY();
				abortModes(target());
				const auto document(viewer::document(target()));
				const auto textArea(target().textArea());
				const auto caret(textArea->caret());
				const auto& peos = caret->end();
				const auto eos(*boost::const_end(caret->selectedRegion()));
				if(kernel::locations::isBeginningOfLine(peos) || (document->isNarrowed() && eos == *boost::const_begin(document->accessibleRegion())))
					return false;

				const String& lineString = document->lineString(kernel::line(eos));
				const Index offsetInLine = kernel::offsetInLine(eos);

				// accept /(?:[Uu]\+)?[0-9A-Fa-f]{1,6}/
				if(iswxdigit(lineString[offsetInLine - 1]) != 0) {
					Index i = offsetInLine - 1;
					while(i != 0) {
						if(offsetInLine - i == 7)
							return false;	// too long string
						else if(iswxdigit(lineString[i - 1]) == 0)
							break;
						--i;
					}

					const CodePoint c = wcstoul(lineString.substr(i, offsetInLine - i).c_str(), nullptr, 16);
					if(text::isValidCodePoint(c)) {
						String s;
						text::utf::encode(c, std::back_inserter(s));
						if(i >= 2 && lineString[i - 1] == L'+' && (lineString[i - 2] == L'U' || lineString[i - 2] == L'u'))
							i -= 2;
						viewer::AutoFreeze af(&target());
						caret->select(viewer::_anchor = kernel::Position(kernel::line(eos), i), viewer::_caret = peos.hit());
						try {
							caret->replaceSelection(s, false);
						} catch(const kernel::DocumentInput::ChangeRejectedException&) {
							return false;
						}
						return true;
					}
				}
				return false;	// invalid code point string and can't convert
			}

			/**
			 * Constructor.
			 * @param viewer The target text viewer
			 */
			CompletionProposalPopupCommand::CompletionProposalPopupCommand(viewer::TextViewer& viewer) BOOST_NOEXCEPT : Command(viewer) {
			}

			/**
			 * Implements @c Command#perform.
			 * @retval false The text viewer didn't have the content assistant
			 */
			bool CompletionProposalPopupCommand::perform() {
				ASCENSION_CHECK_DOCUMENT_READ_ONLY();
//				ASCENSION_CHECK_GUI_EDITABILITY();
				abortIncrementalSearch(*viewer::document(target()));
				if(contentassist::ContentAssistant* ca = target().contentAssistant()) {
					ca->showPossibleCompletions();
					return true;
				}
				return false;	// the viewer does not have a content assistant
			}

			/**
			 * Constructor.
			 * @param viewer The target text viewer
			 */
			EntireDocumentSelectionCreationCommand::EntireDocumentSelectionCreationCommand(viewer::TextViewer& viewer) BOOST_NOEXCEPT : Command(viewer) {
			}

			/**
			 * @see Command#perform
			 * @return @c true if succeeded
			 */
			bool EntireDocumentSelectionCreationCommand::perform() {
				if(const auto textArea = target().textArea()) {
					if(const auto caret = textArea->caret()) {
						endIncrementalSearch(*viewer::document(target()));
						caret->endRectangleSelection();
						caret->select(viewer::SelectedRegion(viewer::document(target())->accessibleRegion()));
						return true;
					}
				}
				return false;
			}

			/**
			 * Constructor.
			 * @param viewer The target text viewer
			 * @param direction The direction to search
			 */
			FindNextCommand::FindNextCommand(viewer::TextViewer& viewer, Direction direction) BOOST_NOEXCEPT : Command(viewer), direction_(direction) {
			}

			/**
			 * @see Command#perform
			 * @return false If no text matched
			 * @throw ... Any exceptions @c searcher#TextSearcher#search throws
			 */
			bool FindNextCommand::perform() {
				if(numericPrefix() == 0)
					return false;
				endIncrementalSearch(*viewer::document(target()));
				viewer::utils::closeCompletionProposalsPopup(target());

				win32::WaitCursor wc;	// TODO: code depends on Win32.
				const auto document(viewer::document(target()));
				const searcher::TextSearcher* s;
				if(const Session* const session = document->session())
					s = &session->textSearcher();
				else
					return false;	// TODO: prepares a default text searcher.

				if(const auto textArea = target().textArea()) {	// TODO: IllegalStateException should be thrown in this case.
					if(const auto caret = textArea->caret()) {
						const kernel::Region scope(document->accessibleRegion());
						kernel::Region matchedRegion(caret->selectedRegion());
						bool foundOnce = false;
						for(NumericPrefix n(numericPrefix()); n > 0; --n) {	// search N times
							if(!s->search(*document, (direction_ == Direction::forward()) ?
									std::max<kernel::Position>(*boost::const_end(matchedRegion), *boost::const_begin(scope))
									: std::min<kernel::Position>(*boost::const_begin(matchedRegion), *boost::const_end(scope)), scope, direction_, matchedRegion))
								break;
							foundOnce = true;
						}

						if(foundOnce) {
							caret->select(viewer::SelectedRegion(matchedRegion));
//							viewer.highlightMatchTexts();
						} else
/*							viewer.highlightMatchTexts(false)*/;
						return foundOnce;
					}
				}

				return false;
			}

			/**
			 * Constructor.
			 * @param viewer The target text viewer
			 * @param type The search type
			 * @param direction The direction to search
			 * @param callback The callback object for the incremental search. Can be @c null
			 */
			IncrementalFindCommand::IncrementalFindCommand(viewer::TextViewer& viewer, searcher::TextSearcher::Type type,
					Direction direction, searcher::IncrementalSearchCallback* callback /* = nullptr */) BOOST_NOEXCEPT
					: Command(viewer), type_(type), direction_(direction), callback_(callback) {
			}

			/**
			 * Implements Command#perform.
			 * @return false If no text matched
			 * @throw ... Any exceptions @c IncrementalSearcher#start and @c IncrementalSearcher#next throw
			 */
			bool IncrementalFindCommand::perform() {
				NumericPrefix n(numericPrefix());
				if(n == 0)
					return false;
				viewer::utils::closeCompletionProposalsPopup(target());
				if(Session* const session = viewer::document(target())->session()) {
					searcher::IncrementalSearcher& isearch = session->incrementalSearcher();
					if(!isearch.isRunning()) {	// begin the search if not running
						isearch.start(*viewer::document(target()),
							viewer::insertionPosition(*target().textArea()->caret()), session->textSearcher(), type_, direction_, callback_);
						--n;
					}
					for(; n > 0; --n) {	// jump N times
						if(!isearch.next(direction_))
							return false;	// it is not able to jump anymore in the active incremental search
					}
				}
				return true;
			}

			/**
			 * Constructor.
			 * @param viewer The target text viewer
			 * @param increase Set @c true to increase the indentation
			 */
			IndentationCommand::IndentationCommand(viewer::TextViewer& viewer, bool increase) BOOST_NOEXCEPT : Command(viewer), increases_(increase) {
			}

			/**
			 * @see Command#perform
			 * @retval false The document's input rejected the change
			 */
			bool IndentationCommand::perform() {
				const NumericPrefix n(numericPrefix());
				if(n == 0)
					return true;
				ASCENSION_CHECK_DOCUMENT_READ_ONLY();
//				ASCENSION_CHECK_GUI_EDITABILITY();
				endIncrementalSearch(*viewer::document(target()));
				viewer::utils::closeCompletionProposalsPopup(target());

				try {
					const auto caret(target().textArea()->caret());
					viewer::document(target())->insertUndoBoundary();
					viewer::AutoFreeze af(&target());
					const long tabs = n;
					indentByTabs(*caret, caret->isSelectionRectangle(), increases_ ? +tabs : -tabs);
					viewer::document(target())->insertUndoBoundary();
				} catch(const kernel::DocumentInput::ChangeRejectedException&) {
					return false;
				}

				return true;
			}

			/**
			 * Constructor.
			 * @param viewer the target text viewer
			 */
			InputMethodOpenStatusToggleCommand::InputMethodOpenStatusToggleCommand(viewer::TextViewer& viewer) BOOST_NOEXCEPT : Command(viewer) {
			}

			/**
			 * Implements @c Command#perform.
			 * @retval false The system didn't support the input method
			 */
			bool InputMethodOpenStatusToggleCommand::perform() {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				if(win32::Handle<HIMC>::Type imc = win32::inputMethod(target()))
					return win32::boole(::ImmSetOpenStatus(imc.get(), !win32::boole(::ImmGetOpenStatus(imc.get()))));
#else
				// TODO: Not implemented.
#endif
				return false;
			}

			/**
			 * Constructor.
			 * @param viewer The target text viewer
			 */
			InputMethodSoftKeyboardModeToggleCommand::InputMethodSoftKeyboardModeToggleCommand(viewer::TextViewer& viewer) BOOST_NOEXCEPT : Command(viewer) {
			}

			/**
			 * @see Command#perform
			 * @retval false The system didn't support the input method
			 */
			bool InputMethodSoftKeyboardModeToggleCommand::perform() {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				if(win32::Handle<HIMC>::Type imc = win32::inputMethod(target())) {
					DWORD conversionMode, sentenceMode;
					if(win32::boole(::ImmGetConversionStatus(imc.get(), &conversionMode, &sentenceMode))) {
						conversionMode = win32::boole(conversionMode & IME_CMODE_SOFTKBD) ?
							(conversionMode & ~IME_CMODE_SOFTKBD) : (conversionMode | IME_CMODE_SOFTKBD);
						return win32::boole(::ImmSetConversionStatus(imc.get(), conversionMode, sentenceMode));
					}
				}
#else
				// TODO: Not implemented.
#endif
				return false;
			}

			/**
			 * Constructor.
			 * @param viewer The target text viewer
			 * @param extendSelection Set @c true to extend the selection
			 */
			MatchBracketCommand::MatchBracketCommand(viewer::TextViewer& viewer, bool extendSelection) BOOST_NOEXCEPT : Command(viewer), extends_(extendSelection) {
			}

			/**
			 * Implements @c Command#perform.
			 * @retval false The match bracket was not found
			 */
			bool MatchBracketCommand::perform() {
				if(const auto textArea = target().textArea()) {
					if(const auto caret = textArea->caret()) {
						endIncrementalSearch(*viewer::document(target()));
						if(const boost::optional<std::pair<kernel::Position, kernel::Position>> matchBrackets = caret->matchBrackets()) {
							caret->endRectangleSelection();
							const auto another(std::get<0>(boost::get(matchBrackets)));
							if(!extends_)
								caret->moveTo(viewer::TextHit::leading(another));
							else {
								const auto ip(viewer::insertionPosition(*caret));
								if(another > ip)
									caret->select(viewer::_anchor = ip, viewer::_caret = viewer::TextHit::trailing(another));
								else {
									const auto& h = caret->hit();
									const auto anchor(h.isLeadingEdge() ? viewer::TextHit::trailing(h.characterIndex()) : viewer::TextHit::leading(h.characterIndex()));
									caret->select(
										viewer::_anchor = viewer::insertionPosition(*viewer::document(target()), anchor),
										viewer::_caret = viewer::TextHit::leading(another));
								}
							}
							return true;
						}
					}
				}
				return false;	// not found (or failed)
			}

			/**
			 * Constructor.
			 * @param viewer The target text viewer
			 * @param direction Set @c boost#none to break current line at the caret position. Otherwise, this command
			 *                  inserts newline(s) at the beginning of the next (@c Direction#forward()) or the
			 *                  previous (@c Direction#backward()) line. In this case, the command ends the active mode
			 *                  and inserts newline character(s)
			 */
			NewlineCommand::NewlineCommand(viewer::TextViewer& viewer, boost::optional<Direction> direction) BOOST_NOEXCEPT : Command(viewer), direction_(direction) {
			}

			/**
			 * Implements @c Command#perform.
			 * @retval false The document was read only or the change was rejected
			 */
			bool NewlineCommand::perform() {
				if(numericPrefix() <= 0)
					return true;

				if(contentassist::ContentAssistant* const ca = target().contentAssistant()) {
					if(contentassist::ContentAssistant::CompletionProposalsUI* cpui = ca->completionProposalsUI()) {
						if(cpui->complete())
							return true;
					}
				}

				if(endIncrementalSearch(*viewer::document(target())) && direction_ == boost::none)
					return true;

				ASCENSION_CHECK_DOCUMENT_READ_ONLY();
//				ASCENSION_CHECK_GUI_EDITABILITY(1);

				const auto caret(target().textArea()->caret());
				const auto oldSelection(caret->selectedRegion());
				const auto document(viewer::document(target()));
				viewer::AutoFreeze af(&target());

				if(direction_ != boost::none) {
					viewer::TextHit h(caret->hit());	// initial value is no matter...
					if(*direction_ == Direction::forward())
						h = viewer::locations::endOfVisualLine(*caret);
					else if(kernel::line(*caret) != kernel::line(*boost::const_begin(document->region()))) {
						const Index line = kernel::line(*caret) - 1;
						h = viewer::TextHit::leading(kernel::Position(line, document->lineLength(line)));
					} else
						h = viewer::TextHit::leading(*boost::const_begin(document->region()));
					if(!encompasses(document->accessibleRegion(), h.characterIndex()))
						return false;
					const bool autoShow = caret->isAutoShowEnabled();
					caret->enableAutoShow(false);
					caret->moveTo(h);
					caret->enableAutoShow(autoShow);
				}

				try {
					document->insertUndoBoundary();
					breakLine(*caret, false, numericPrefix());
				} catch(const kernel::DocumentInput::ChangeRejectedException&) {
					document->insertUndoBoundary();
					caret->select(oldSelection);
					return false;
				}
				document->insertUndoBoundary();
				caret->moveTo(caret->anchor().hit());
				return true;
			}

			/**
			 * Constructor.
			 * @param viewer The target text viewer
			 */
			OvertypeModeToggleCommand::OvertypeModeToggleCommand(viewer::TextViewer& viewer) BOOST_NOEXCEPT : Command(viewer) {
			}

			/**
			 * @see Command#perform
			 * @return @c true if succeeded
			 */
			bool OvertypeModeToggleCommand::perform() {
				if(const auto textArea = target().textArea()) {
					if(const auto caret = textArea->caret()) {
						caret->setOvertypeMode(!caret->isOvertypeMode());
						viewer::utils::closeCompletionProposalsPopup(target());
						return true;
					}
				}
				return false;
			}

			/**
			 * Constructor.
			 * @param viewer The target text viewer
			 * @param useKillRing
			 */
			PasteCommand::PasteCommand(viewer::TextViewer& viewer, bool useKillRing) BOOST_NOEXCEPT : Command(viewer), usesKillRing_(useKillRing) {
			}

			/**
			 * @see Command#perform
			 * @return false the internal call of @c Caret#paste threw
			 */
			bool PasteCommand::perform() {
				ASCENSION_ASSERT_IFISWINDOW();
				ASCENSION_CHECK_DOCUMENT_READ_ONLY();
				if(const auto textArea = target().textArea()) {
					if(const auto caret = textArea->caret()) {
						viewer::utils::closeCompletionProposalsPopup(target());
						try {
							caret->paste(usesKillRing_);
						} catch(...) {
							return false;
						}
						return true;
					}
				}
				return false;
			}

			/**
			 * Constructor.
			 * @param viewer The target text viewer
			 */
			ReconversionCommand::ReconversionCommand(viewer::TextViewer& viewer) BOOST_NOEXCEPT : Command(viewer) {
			}

			/**
			 * Implements @c Command#perform.
			 * @return false The selection was empty or rectangle. Or the system didn't support IME reconversion
			 * @throw std#bad_alloc Out of memory
			 * @see viewer#TextViewer#onIMERequest
			 */
			bool ReconversionCommand::perform() {
				endIncrementalSearch(*viewer::document(target()));
				ASCENSION_CHECK_DOCUMENT_READ_ONLY();
//	ASCENSION_CHECK_GUI_EDITABILITY();

				bool succeeded = false;
				if(const auto textArea = target().textArea()) {
					if(const auto caret = textArea->caret()) {
						if(!caret->isSelectionRectangle()) {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
							if(win32::Handle<HIMC>::Type imc = win32::inputMethod(target())) {
								if(!win32::boole(::ImmGetOpenStatus(imc.get())))	// without this, IME may ignore us?
									::ImmSetOpenStatus(imc.get(), true);

								// from NotePadView.pas of TNotePad (http://wantech.ikuto.com/)
								const bool multilineSelection = kernel::line(*caret) != kernel::line(caret->anchor());
								const String s(multilineSelection ? selectedString(*caret) : viewer::document(target()).line(kernel::line(*caret)));
								const DWORD bytes = static_cast<DWORD>(sizeof(RECONVERTSTRING) + sizeof(Char) * s.length());
								RECONVERTSTRING* const rcs = static_cast<RECONVERTSTRING*>(::operator new(bytes));
								rcs->dwSize = bytes;
								rcs->dwVersion = 0;
								rcs->dwStrLen = static_cast<DWORD>(s.length());
								rcs->dwStrOffset = sizeof(RECONVERTSTRING);
								rcs->dwCompStrLen = rcs->dwTargetStrLen =
									static_cast<DWORD>(multilineSelection ? s.length() : (kernel::offsetInLine(caret->end()) - kernel::offsetInLine(caret->beginning())));
								rcs->dwCompStrOffset = rcs->dwTargetStrOffset =
									multilineSelection ? 0 : static_cast<DWORD>(sizeof(Char) * kernel::offsetInLine(caret->beginning()));
								s.copy(reinterpret_cast<Char*>(reinterpret_cast<char*>(rcs) + rcs->dwStrOffset), s.length());

								if(viewer::isSelectionEmpty(*caret)) {
									// IME selects the composition target automatically if no selection
									if(win32::boole(::ImmSetCompositionStringW(imc.get(), SCS_QUERYRECONVERTSTRING, rcs, rcs->dwSize, nullptr, 0))) {
										caret->select(
											Position(kernel::line(*caret), rcs->dwCompStrOffset / sizeof(Char)),
											Position(kernel::line(*caret), rcs->dwCompStrOffset / sizeof(Char) + rcs->dwCompStrLen));
										if(win32::boole(::ImmSetCompositionStringW(imc.get(), SCS_SETRECONVERTSTRING, rcs, rcs->dwSize, nullptr, 0)))
											succeeded = true;
									}
								}
								::operator delete(rcs);
							}
#else
							// TODO: Not implemented.
#endif
						}
					}
				}

				viewer::utils::closeCompletionProposalsPopup(target());
				return succeeded;
			}

			/**
			 * Constructor.
			 * @param viewer The target text viewer
			 * @param onlySelection
			 * @param replacement The replacement string
			 * @param callback
			 */
			ReplaceAllCommand::ReplaceAllCommand(viewer::TextViewer& viewer, bool onlySelection,
					const String& replacement, searcher::InteractiveReplacementCallback* callback) BOOST_NOEXCEPT
					: Command(viewer), onlySelection_(onlySelection), replacement_(replacement), callback_(callback) {
			}

			/**
			 * Replaces all matched texts. This does not freeze the text viewer.
			 * @return The number of replced strings
			 * @throw ... Any exceptions @c searcher::TextSearcher::replaceAll throws other than
			 *            @c ReplacementInterruptedException&lt;IDocumentInput#ChangeRejectedException&gt;
			 */
			bool ReplaceAllCommand::perform() {
				abortModes(target());
				const auto caret(target().textArea()->caret());
				if(onlySelection_ && viewer::isSelectionEmpty(*caret))
					return false;

				win32::WaitCursor wc;
				const auto document(viewer::document(target()));
				searcher::TextSearcher* s;
				if(Session* const session = document->session())
					s = &session->textSearcher();
				else
					return false;	// TODO: prepares a default text searcher.

				kernel::Region scope(
					onlySelection_ ? std::max<kernel::Position>(
						*boost::const_begin(caret->selectedRegion()),
							*boost::const_begin(document->accessibleRegion()))
						: *boost::const_begin(document->accessibleRegion()),
					onlySelection_ ? std::min<kernel::Position>(
						*boost::const_end(caret->selectedRegion()),
							*boost::const_end(document->accessibleRegion()))
						: *boost::const_end(document->accessibleRegion()));

				// mark to restore the selection later
				kernel::Point anchorBeforeReplacement(*document, caret->selectedRegion().anchor());
				kernel::Point caretBeforeReplacement(*document, viewer::insertionPosition(*caret));

				viewer::AutoFreeze af(&target());
				try {
					numberOfLastReplacements_ = s->replaceAll(*document, scope, replacement_, callback_);
				} catch(const searcher::ReplacementInterruptedException<kernel::DocumentInput::ChangeRejectedException>& e) {
					numberOfLastReplacements_ = e.numberOfReplacements();
					throw;
				} catch(const searcher::ReplacementInterruptedException<std::bad_alloc>& e) {
					numberOfLastReplacements_ = e.numberOfReplacements();
					throw;
				}
				if(numberOfLastReplacements_ != 0)
					target().textArea()->caret()->select(
						viewer::_anchor = anchorBeforeReplacement.position(),
						viewer::_caret = viewer::TextHit::leading(caretBeforeReplacement.position()));
				return true;
			}

			/**
			 * Constructor.
			 * @param viewer The target text viewer
			 * @param procedure A function gives a motion
			 * @param direction The direction of motion
			 * @throw NullPointerException @a procedure is @c null
			 */
			template<typename ProcedureSignature>
			RowSelectionExtensionCommand<ProcedureSignature>::RowSelectionExtensionCommand(
					viewer::TextViewer& viewer, ProcedureSignature* procedure, Direction direction)
					: Command(viewer), procedure_(procedure), direction_(direction) {
				if(procedure == nullptr)
					throw NullPointerException("procedure");
			}

			/**
			 * @see Command#perform
			 * @return true
			 */
			template<typename ProcedureSignature>
			bool RowSelectionExtensionCommand<ProcedureSignature>::perform() {
				viewer::utils::closeCompletionProposalsPopup(target());
				endIncrementalSearch(*viewer::document(target()));
			
				const auto caret(target().textArea()->caret());
				if(viewer::isSelectionEmpty(*caret) && !caret->isSelectionRectangle())
					caret->beginRectangleSelection();
				return CaretMovementCommand<ProcedureSignature>(target(), procedure_, direction_, true).setNumericPrefix(numericPrefix())();
			}

			// explicit instantiations
			template class RowSelectionExtensionCommand<kernel::Position(const kernel::locations::PointProxy&, Direction, Index)>;	// next(Line|Word|WordEnd)
			template class RowSelectionExtensionCommand<boost::optional<kernel::Position>(const kernel::locations::PointProxy&, Direction, Index)>;	// nextBookmark
			template class RowSelectionExtensionCommand<kernel::Position(const kernel::locations::PointProxy&, Direction, kernel::locations::CharacterUnit, Index)>;	// nextCharacter
			template class RowSelectionExtensionCommand<viewer::VisualDestinationProxy(const viewer::locations::PointProxy&, Direction, Index)>;	// nextPage
			template class RowSelectionExtensionCommand<viewer::VisualDestinationProxy(const viewer::VisualPoint&, Direction, Index)>;	// nextVisualLine

			/**
			 * Constructor.
			 * @param viewer The target text viewer
			 * @param procedure A function gives a motion
			 * @throw NullPointerException @a procedure is @c null
			 * 
			 */
			template<typename ProcedureSignature>
			RowSelectionExtensionToDefinedPositionCommand<ProcedureSignature>::RowSelectionExtensionToDefinedPositionCommand(
					viewer::TextViewer& viewer, ProcedureSignature* procedure) : Command(viewer), procedure_(procedure) {
				if(procedure == nullptr)
					throw NullPointerException("procedure");
			}

			// explicit instantiations
			template class RowSelectionExtensionToDefinedPositionCommand<kernel::Position(const kernel::locations::PointProxy&)>;	// (beginning|end)Of(Document|Line)
			template class RowSelectionExtensionToDefinedPositionCommand<viewer::TextHit(const viewer::locations::PointProxy&)>;	// contextual(Beginning|End)OfLine, (beginning|end|contextualBeginning|contextualEnd)OfVisualLine, (first|last)PrintableCharacterOf(Visual)?Line

			/**
			 * Moves the caret or extends the selection.
			 * @return true
			 */
			template<typename ProcedureSignature>
			bool RowSelectionExtensionToDefinedPositionCommand<ProcedureSignature>::perform() {
				viewer::utils::closeCompletionProposalsPopup(target());
				endIncrementalSearch(*viewer::document(target()));

				const auto caret(target().textArea()->caret());
				if(viewer::isSelectionEmpty(*caret) && !caret->isSelectionRectangle())
					caret->beginRectangleSelection();
				return CaretMovementToDefinedPositionCommand<ProcedureSignature>(target(), procedure_, true).setNumericPrefix(numericPrefix())();
			}

			/**
			 * Constructor.
			 * @param viewer The target text viewer
			 * @param untabify Set @c true to untabify rather than tabify
			 */
			TabifyCommand::TabifyCommand(viewer::TextViewer& viewer, bool untabify) BOOST_NOEXCEPT : Command(viewer), untabify_(untabify) {
			}

			/**
			 * Implements @c Command#perform.
			 * @note Not implemented.
			 */
			bool TabifyCommand::perform() {
				ASCENSION_CHECK_DOCUMENT_READ_ONLY();
//				ASCENSION_CHECK_GUI_EDITABILITY(1);
				abortModes(target());
				// TODO: not implemented.
				return false;
			}

			/**
			 * Constructor.
			 * @param viewer The target text viewer
			 * @param text The text to input. This can be empty or ill-formed UTF-16 sequence
			 */
			TextInputCommand::TextInputCommand(viewer::TextViewer& viewer, const String& text) BOOST_NOEXCEPT : Command(viewer), text_(text) {
			}

			namespace {
				inline String multiplyString(const String& s, std::size_t n) {
					const std::size_t len = s.length();
					String temp;
					temp.reserve(n * len);
					for(std::size_t i = 0; i < n; ++i)
						temp.append(s.data(), len);
					return temp;
				}
			}

			/**
			 * Inserts a text. If the incremental search is active, appends a string to the end of the pattern.
			 * @retval false The change was rejected
			 * @throw ... Any exceptions @c searcher#IncrementalSearcher#addString and @c viewer#replaceSelection
			 *            throw other than @c kernel#IDocumentInput#ChangeRejectedException
			 */
			bool TextInputCommand::perform() {
				const NumericPrefix n(numericPrefix());
				if(n == 0)
					return true;
#if 0
				if(Session* const session = target().document().session()) {
					if(session->incrementalSearcher().isRunning()) {
						session->incrementalSearcher().addString((n > 1) ? multiplyString(text_, static_cast<std::size_t>(n)) : text_);
						return true;
					}
				}
#endif
				ASCENSION_CHECK_DOCUMENT_READ_ONLY();
//				ASCENSION_CHECK_GUI_EDITABILITY();
				if(n > 1) {
					try {
						target().textArea()->caret()->replaceSelection(multiplyString(text_, static_cast<std::size_t>(n)));
					} catch(const kernel::DocumentInput::ChangeRejectedException&) {
						return false;
					}
				} else {
					try {
						target().textArea()->caret()->replaceSelection(text_);
					} catch(const kernel::DocumentInput::ChangeRejectedException&) {
						return false;
					}
				}
				return true;
			}

			/**
			 * Constructor.
			 * @param viewer The target text viewer
			 * @param procedure Indicates what to transpose. This must be one of: @c EditPoint#transposeCharacters,
			 *                  @c EditPoint#transposeWords, @c EditPoint#transposeLines
			 * @throw std#invalid_argument
			 */
			TranspositionCommand::TranspositionCommand(viewer::TextViewer& viewer, bool(*procedure)(viewer::Caret&)) : Command(viewer), procedure_(procedure) {
				if(procedure_ != &viewer::transposeCharacters
						&& procedure_ != &viewer::transposeWords
						&& procedure_ != &viewer::transposeLines)
					throw std::invalid_argument("procedure");
			}

			/**
			 * Implements @c Command#perform by using a transposition method of @c viewer#Caret class. 
			 * @return false The internal transposition method call returned @c false
			 * @throw ... Any exceptions the transposition method returns other than
			 *            @c kernel#ReadOnlyDocumentException and @c kernel#DocumentCantChangeException
			 */
			bool TranspositionCommand::perform() {
				ASCENSION_CHECK_DOCUMENT_READ_ONLY();
//				ASCENSION_CHECK_GUI_EDITABILITY();
				endIncrementalSearch(*viewer::document(target()));
				viewer::utils::closeCompletionProposalsPopup(target());

				const auto caret(target().textArea()->caret());
				try {
					viewer::AutoFreeze af(&target());
					viewer::document(target())->insertUndoBoundary();
					const bool succeeded = (*procedure_)(*caret);
					viewer::document(target())->insertUndoBoundary();
					return succeeded;
				} catch(const kernel::DocumentInput::ChangeRejectedException&) {
					return false;
				}
			}

			/**
			 * Constructor.
			 * @param viewer The target text viewer
			 * @param redo Set @c true to perform redo, rather than undo
			 */
			UndoCommand::UndoCommand(viewer::TextViewer& viewer, bool redo) BOOST_NOEXCEPT : Command(viewer), redo_(redo), lastResult_(INDETERMINATE) {
			}

			/**
			 * Returns @c true if the last performance was done incompletely.
			 * @throw IllegalStateException The command has never performed
			 * @see Document#undo, Document#redo
			 */
			bool UndoCommand::isLastActionIncompleted() const {
				if(lastResult_ == INDETERMINATE)
					throw IllegalStateException("this command has never performed.");
				return lastResult_ == INCOMPLETED;
			}

			/**
			 * Undo or redo.
			 * @retval false The change was rejected
			 */
			bool UndoCommand::perform() {
//				ASCENSION_CHECK_GUI_EDITABILITY(1);
				if(numericPrefix() < 0)
					setNumericPrefix(0);	// currently, this is no-op

				win32::WaitCursor wc;
				const auto document(viewer::document(target()));
				bool (kernel::Document::*performance)(std::size_t) = !redo_ ? &kernel::Document::undo : &kernel::Document::redo;
				std::size_t (kernel::Document::*number)() const = !redo_ ? &kernel::Document::numberOfUndoableChanges : &kernel::Document::numberOfRedoableChanges;
				try {
					lastResult_ = ((*document).*performance)(std::min(static_cast<std::size_t>(numericPrefix()), ((*document).*number)())) ? COMPLETED : INCOMPLETED;
				} catch(kernel::DocumentCantChangeException&) {
					return false;
				}
				return true;
			}

			/**
			 * Constructor.
			 * @param viewer The target text viewer
			 * @param direction The direcion to delete
			 */
			WordDeletionCommand::WordDeletionCommand(viewer::TextViewer& viewer, Direction direction) BOOST_NOEXCEPT : Command(viewer), direction_(direction) {
			}

			/**
			 * Implements @c Command#perform.
			 * @retval false The document's input rejected the change
			 */
			bool WordDeletionCommand::perform() {
				long n = numericPrefix();
				if(n == 0)
					return true;
				ASCENSION_CHECK_DOCUMENT_READ_ONLY();
				const auto document(viewer::document(target()));
				abortIncrementalSearch(*document);

				const auto caret(target().textArea()->caret());
				if(/*caret.isAutoCompletionRunning() &&*/ direction_ == Direction::forward())
					viewer::utils::closeCompletionProposalsPopup(target());

				const kernel::Position from((direction_ == Direction::forward()) ?
					*boost::const_begin(caret->selectedRegion()) : *boost::const_end(caret->selectedRegion()));
				text::WordBreakIterator<kernel::DocumentCharacterIterator> to(
					kernel::DocumentCharacterIterator(*document,
						(direction_ == Direction::forward()) ? *boost::const_end(caret->selectedRegion()) : *boost::const_begin(caret->selectedRegion())),
					text::WordBreakIteratorBase::START_OF_SEGMENT,
						document->contentTypeInformation().getIdentifierSyntax(contentType(*caret)));
				for(kernel::Position p(to.base().tell()); n > 0; --n) {
					if(p == ((direction_ == Direction::forward()) ? ++to : --to).base().tell())
						break;
					p = to.base().tell();
				}
				if(to.base().tell() != from) {
					try {
						viewer::AutoFreeze af(&target());
						document->insertUndoBoundary();
						kernel::erase(*document, kernel::Region(from, to.base().tell()));
						caret->moveTo(viewer::TextHit::leading(std::min(from, to.base().tell())));
						document->insertUndoBoundary();
					} catch(const kernel::DocumentInput::ChangeRejectedException&) {
						return false;
					}
				}
				return true;
			}

			/**
			 * Constructor.
			 * @param viewer The target text viewer
			 */
			WordSelectionCreationCommand::WordSelectionCreationCommand(viewer::TextViewer& viewer) BOOST_NOEXCEPT : Command(viewer) {
			}

			/**
			 * Implements @c Command#perform.
			 * @return true
			 */
			bool WordSelectionCreationCommand::perform() {
				endIncrementalSearch(*viewer::document(target()));
				target().textArea()->caret()->endRectangleSelection();
				selectWord(*target().textArea()->caret());
				return 0;
			}

#undef ASCENSION_ASSERT_IFISWINDOW
#undef ASCENSION_CHECK_DOCUMENT_READ_ONLY
//#undef ASCENSION_CHECK_GUI_EDITABILITY
		}
	}
}
