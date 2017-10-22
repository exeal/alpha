/**
 * @file deletions.cpp
 * @author exeal
 * @date 2006-2011 was text-editor.cpp
 * @date 2011-05-06
 * @date 2011-2014, 2016
 * @date 2016-11-05 Separated from command.cpp.
 */

#include <ascension/corelib/text/word-break-iterator.hpp>
#include <ascension/kernel/document.hpp>
#include <ascension/kernel/document-input.hpp>
#include <ascension/kernel/locations.hpp>
#include <ascension/kernel/searcher.hpp>
#include <ascension/text-editor/commands/deletions.hpp>
#include <ascension/text-editor/session.hpp>
#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/text-area.hpp>
#include <ascension/viewer/text-viewer.hpp>
#include <ascension/viewer/text-viewer-utility.hpp>

namespace ascension {
	namespace texteditor {
		namespace commands {
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
					throwIfTargetIsReadOnly();
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
								kernel::locations::nextCharacter(viewer::insertionPosition(caret->end()),
									Direction::forward(), kernel::locations::GRAPHEME_CLUSTER, viewer::isSelectionEmpty(*caret) ? n : (n - 1)));
						else
							region = kernel::Region(
								kernel::locations::nextCharacter(viewer::insertionPosition(caret->beginning()),
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
				throwIfTargetIsReadOnly();
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
		}
	}
}
