/**
 * @file yanks.cpp
 * @author exeal
 * @date 2006-2011 was text-editor.cpp
 * @date 2011-05-06
 * @date 2011-2014, 2016
 * @date 2016-11-05 Separated from command.cpp.
 */

#include <ascension/text-editor/commands/yanks.hpp>
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
			 * @param useKillRing
			 */
			PasteCommand::PasteCommand(viewer::TextViewer& viewer, bool useKillRing) BOOST_NOEXCEPT : Command(viewer), usesKillRing_(useKillRing) {
			}

			/**
			 * @see Command#perform
			 * @return false the internal call of @c Caret#paste threw
			 */
			bool PasteCommand::perform() {
				throwIfTargetHasNoWindow();
				throwIfTargetIsReadOnly();
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
		}
	}
}
