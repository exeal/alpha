/**
 * @file searches.cpp
 * @author exeal
 * @date 2006-2011 was text-editor.cpp
 * @date 2011-05-06
 * @date 2011-2014, 2016
 * @date 2016-11-04 Separated from command.cpp.
 */

#include <ascension/corelib/numeric-range-algorithm/intersection.hpp>
#include <ascension/kernel/document.hpp>
#include <ascension/kernel/document-input.hpp>
#include <ascension/text-editor/commands/searches.hpp>
#include <ascension/text-editor/session.hpp>
#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/text-area.hpp>
#include <ascension/viewer/text-viewer.hpp>
#include <ascension/viewer/text-viewer-utility.hpp>
#include <ascension/win32/ui/wait-cursor.hpp>

namespace ascension {
	namespace texteditor {
		namespace commands {
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
				abortModes();
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
		}
	}
}
