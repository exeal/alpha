/**
 * @file indentations.cpp
 * @author exeal
 * @date 2006-2011 was text-editor.cpp
 * @date 2011-05-06
 * @date 2011-2014, 2016
 * @date 2016-11-04 Separated from command.cpp.
 */

#include <ascension/kernel/document.hpp>
#include <ascension/kernel/document-input.hpp>
#include <ascension/text-editor/commands/indentations.hpp>
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
				throwIfTargetIsReadOnly();
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
		}
	}
}
