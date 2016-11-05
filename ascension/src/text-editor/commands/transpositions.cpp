/**
 * @file transpositions.cpp
 * @author exeal
 * @date 2006-2011 was text-editor.cpp
 * @date 2011-05-06
 * @date 2011-2014, 2016
 * @date 2016-11-04 Separated from command.cpp.
 */

#include <ascension/kernel/document.hpp>
#include <ascension/kernel/document-input.hpp>
#include <ascension/text-editor/commands/transpositions.hpp>
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
				throwIfTargetIsReadOnly();
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
		}
	}
}
