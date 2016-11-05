/**
 * @file modals.hpp
 * @author exeal
 * @date 2006-2011 was text-editor.hpp
 * @date 2011-05-06
 * @date 2011-2013, 2015
 * @date 2016-11-04 Separated from command.hpp.
 */

#ifndef ASCENSION_MODALS_HPP
#define ASCENSION_MODALS_HPP
#include <ascension/text-editor/command.hpp>

namespace ascension {
	namespace texteditor {
		namespace commands {
			/// Clears the selection, or aborts the active incremental search and exits the content assist.
			class CancelCommand : public Command {
			public:
				explicit CancelCommand(viewer::TextViewer& viewer) BOOST_NOEXCEPT;
			private:
				bool perform();
			};

			/**
			 * Shows completion proposals and aborts the active incremental search.
			 * @see contentassist#ContentAssistant#showPossibleCompletions
			 */
			class CompletionProposalPopupCommand : public Command {
			public:
				explicit CompletionProposalPopupCommand(viewer::TextViewer& view) BOOST_NOEXCEPT;
			private:
				bool perform();
			};

			/// Toggles the input method's open status.
			class InputMethodOpenStatusToggleCommand : public Command {
			public:
				explicit InputMethodOpenStatusToggleCommand(viewer::TextViewer& viewer) BOOST_NOEXCEPT;
			private:
				bool perform();
			};

			/// Toggles Soft Keyboard mode of the input method.
			class InputMethodSoftKeyboardModeToggleCommand : public Command {
			public:
				explicit InputMethodSoftKeyboardModeToggleCommand(viewer::TextViewer& viewer) BOOST_NOEXCEPT;
			private:
				bool perform();
			};

			/// Toggles overtype mode of the caret.
			class OvertypeModeToggleCommand : public Command {
			public:
				explicit OvertypeModeToggleCommand(viewer::TextViewer& viewer) BOOST_NOEXCEPT;
			private:
				bool perform();
			};
		}
	}
}

#endif // !ASCENSION_MODALS_HPP
