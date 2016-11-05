/**
 * @file rollbacks.cpp
 * @author exeal
 * @date 2006-2011 was text-editor.cpp
 * @date 2011-05-06
 * @date 2011-2014, 2016
 * @date 2016-11-05 Separated from command.cpp.
 */

#include <ascension/kernel/document.hpp>
#include <ascension/text-editor/commands/rollbacks.hpp>
#include <ascension/viewer/text-viewer.hpp>
#include <ascension/win32/ui/wait-cursor.hpp>

namespace ascension {
	namespace texteditor {
		namespace commands {
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
		}
	}
}
