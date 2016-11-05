/**
 * @file command.cpp
 * @author exeal
 * @date 2006-2011 was text-editor.cpp
 * @date 2011-05-06
 * @date 2011-2014, 2016
 */

#include <ascension/kernel/document.hpp>
#include <ascension/text-editor/command.hpp>
#include <ascension/text-editor/session.hpp>
#include <ascension/viewer/text-viewer.hpp>
#include <ascension/viewer/text-viewer-utility.hpp>

namespace ascension {
	namespace texteditor {
		/**
		 * Protected constructor.
		 * @param viewer The target text viewer
		 */
		Command::Command(viewer::TextViewer& viewer) BOOST_NOEXCEPT : viewer_(&viewer), numericPrefix_(1) {
		}

		/// Destructor.
		Command::~Command() BOOST_NOEXCEPT {
		}

		bool Command::abortModes() {
			viewer::utils::closeCompletionProposalsPopup(target());
			return abortIncrementalSearch(*viewer::document(target()));
		}

		void Command::throwIfTargetHasNoWindow() const {
			assert(true/*target().isWindow()*/);
		}

		void Command::throwIfTargetIsReadOnly() const {
			if(viewer::document(target())->isReadOnly())
				throw kernel::ReadOnlyDocumentException();
		}
	}
}
