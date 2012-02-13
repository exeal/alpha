/**
 * @file command.hpp
 * @author exeal
 * @date 2006-2011 was text-editor.hpp
 * @date 2011-05-06
 * @date 2011-2012
 */

#ifndef ASCENSION_COMMAND_HPP
#define ASCENSION_COMMAND_HPP

#include <ascension/text-editor/session.hpp>
#include <ascension/kernel/searcher.hpp>
#include <ascension/viewer/viewer.hpp>

namespace ascension {

	namespace viewers {
		class BlockProgressionDestinationProxy;
	}

	namespace texteditor {

		/**
		 * Abstract class for the editor commands.
		 * @see ascension#texteditor#commands
		 */
		class Command {
		public:
			virtual ~Command() throw();
			bool operator()();
			long numericPrefix() const /*throw()*/;
			Command& retarget(viewers::TextViewer& viewer) /*throw()*/;
			Command& setNumericPrefix(long number) /*throw()*/;
		protected:
			explicit Command(viewers::TextViewer& viewer) /*throw()*/;
			/// Returns the text viewer which is the target of this command.
			viewers::TextViewer& target() const /*throw()*/ {return *viewer_;}
		private:
			/// Called by @c #operator(). For semantics, see @c #operator().
			virtual bool perform() = 0;
		private:
			viewers::TextViewer* viewer_;
			long numericPrefix_;
		};

		/**
		 * Implementations of the standard commands. These classes extends @c Command class.
		 *
		 * These commands are very common for text editors, but somewhat complex to implement. Use
		 * these classes, rather than write your own code implements same feature.
		 */
		namespace commands {
			/// Searches and bookmarks all matched lines.
			class BookmarkMatchLinesCommand : public Command {
			public:
				explicit BookmarkMatchLinesCommand(viewers::TextViewer& viewer,
					const kernel::Region& region = kernel::Region()) /*throw()*/;
				Index numberOfMarkedLines() const /*throw()*/;
			private:
				bool perform();
				const kernel::Region region_;
				Index numberOfMarkedLines_;
			};
			/// Clears the selection, or aborts the active incremental search and exits the content assist.
			class CancelCommand : public Command {
			public:
				explicit CancelCommand(viewers::TextViewer& viewer) /*throw()*/;
			private:
				bool perform();
			};
			/**
			 * Moves the caret or extends the selection. @c CharacterUnit#GRAPHEME_CLUSTER is
			 * always used as character unit.
			 * @see viewers#Caret
			 */
			class CaretMovementCommand : public Command {
			public:
				CaretMovementCommand(viewers::TextViewer& viewer,
					kernel::Position(*procedure)(const kernel::Point&), bool extendSelection = false);
				CaretMovementCommand(viewers::TextViewer& viewer,
					kernel::Position(*procedure)(const kernel::Point&, Index), bool extendSelection = false);
				CaretMovementCommand(viewers::TextViewer& viewer,
					kernel::Position(*procedure)(const kernel::Point&, kernel::locations::CharacterUnit, Index), bool extendSelection = false);
				CaretMovementCommand(viewers::TextViewer& viewer,
					kernel::Position(*procedure)(const viewers::VisualPoint&), bool extendSelection = false);
				CaretMovementCommand(viewers::TextViewer& viewer,
					kernel::Position(*procedure)(const viewers::VisualPoint&, Index), bool extendSelection = false);
				CaretMovementCommand(viewers::TextViewer& viewer,
					kernel::Position(*procedure)(const viewers::VisualPoint&, kernel::locations::CharacterUnit, Index), bool extendSelection = false);
				CaretMovementCommand(viewers::TextViewer& viewer,
					viewers::BlockProgressionDestinationProxy(*procedure)(const viewers::VisualPoint&, Index), bool extendSelection = false);
			private:
				bool perform();
				kernel::Position(*procedureP_)(const kernel::Point&);
				kernel::Position(*procedurePL_)(const kernel::Point&, Index);
				kernel::Position(*procedurePCL_)(const kernel::Point&, kernel::locations::CharacterUnit, Index);
				kernel::Position(*procedureV_)(const viewers::VisualPoint&);
				kernel::Position(*procedureVL_)(const viewers::VisualPoint&, Index);
				kernel::Position(*procedureVCL_)(const viewers::VisualPoint&, kernel::locations::CharacterUnit, Index);
				viewers::BlockProgressionDestinationProxy(*procedureVLV_)(const viewers::VisualPoint&, Index);
				const bool extends_;
			};
			/**
			 * Deletes the forward/backward N character(s). If the incremental search is active,
			 * deletes the entire pattern (direction is @c FORWARD) or the last N character(s)
			 * (direction is @c BACKWARD).
			 * @see WordDeletionCommand
			 */
			class CharacterDeletionCommand : public Command {
			public:
				CharacterDeletionCommand(viewers::TextViewer& viewer, Direction direction) /*throw()*/;
			private:
				bool perform();
				const Direction direction_;
			};
			/// Converts a character into the text represents the code value of the character.
			class CharacterToCodePointConversionCommand : public Command {
			public:
				CharacterToCodePointConversionCommand(viewers::TextViewer& viewer) /*throw()*/;
			private:
				bool perform();
			};
			/**
			 * Inputs a character on the caret position, or appends to the end of the active
			 * incremental search pattern.
			 * @see viewers#Caret#inputCharacter
			 */
			class CharacterInputCommand : public Command {
			public:
				CharacterInputCommand(viewers::TextViewer& viewer, CodePoint c);
			private:
				bool perform();
				const CodePoint c_;
			};
			/// Inputs a character is at same position in the next/previous visual line.
			class CharacterInputFromNextLineCommand : public Command {
			public:
				CharacterInputFromNextLineCommand(viewers::TextViewer& viewer, bool fromPreviousLine) /*throw()*/;
			private:
				bool perform();
				const bool fromPreviousLine_;
			};
			/// Converts a text represents a code value into the character has the code value.
			class CodePointToCharacterConversionCommand : public Command {
			public:
				CodePointToCharacterConversionCommand(viewers::TextViewer& view) /*throw()*/;
			private:
				bool perform();
			};
			/**
			 * Shows completion proposals and aborts the active incremental search.
			 * @see contentassist#ContentAssistant#showPossibleCompletions
			 */
			class CompletionProposalPopupCommand : public Command {
			public:
				explicit CompletionProposalPopupCommand(viewers::TextViewer& view) /*throw()*/;
			private:
				bool perform();
			};
			/// Selects the entire document.
			class EntireDocumentSelectionCreationCommand : public Command {
			public:
				explicit EntireDocumentSelectionCreationCommand(viewers::TextViewer& view) /*throw()*/;
			private:
				bool perform();
			};
			/**
			 * Searches the next/previous or the previous match and selects matched region. The
			 * search performs using the current search conditions.
			 *
			 * To find the next/previous for the incremental search, use
			 * @c IncrementalSearchCommand instead.
			 */
			class FindNextCommand : public Command {
			public:
				FindNextCommand(viewers::TextViewer& viewer, Direction direction) /*throw()*/;
			private:
				bool perform();
				const Direction direction_;
			};
			/**
			 * Begins the incremental search. If the search is already running, jumps to the
			 * next/previous match.
			 */
			class IncrementalFindCommand : public Command {
			public:
				IncrementalFindCommand(viewers::TextViewer& view, searcher::TextSearcher::Type type,
					Direction direction, searcher::IncrementalSearchCallback* callback = nullptr) /*throw()*/;
			private:
				bool perform();
				searcher::TextSearcher::Type type_;
				const Direction direction_;
				searcher::IncrementalSearchCallback* const callback_;
			};
			/**
			 * Makes/Deletes indents of the selected non-blank lines.
			 * @see viewers#Caret#spaceIndent, viewers#Caret#tabIndent
			 */
			class IndentationCommand : public Command {
			public:
				IndentationCommand(viewers::TextViewer& view, bool increase) /*throw()*/;
			private:
				bool perform();
				bool increases_;
			};
			/// Toggles the input method's open status.
			class InputMethodOpenStatusToggleCommand : public Command {
			public:
				explicit InputMethodOpenStatusToggleCommand(viewers::TextViewer& viewer) /*throw()*/;
			private:
				bool perform();
			};
			/// Toggles Soft Keyboard mode of the input method.
			class InputMethodSoftKeyboardModeToggleCommand : public Command {
			public:
				explicit InputMethodSoftKeyboardModeToggleCommand(viewers::TextViewer& viewer) /*throw()*/;
			private:
				bool perform();
			};
			/// Moves the caret or extends the selection to the match bracket.
			class MatchBracketCommand : public Command {
			public:
				MatchBracketCommand(viewers::TextViewer& viewer, bool extendSelection = false) /*throw()*/;
			private:
				bool perform();
				const bool extends_;
			};
			/**
			 * Inserts a newline, or exits a mode.
			 * If the incremental search is running, exits the search. If the content assist is
			 * active, completes or aborts and breaks the line if no candidate matches exactly.
			 */
			class NewlineCommand : public Command {
			public:
				NewlineCommand(viewers::TextViewer& view, bool insertPrevious) /*throw()*/;
			private:
				bool perform();
				const bool insertsPrevious_;
			};
			/// Toggles overtype mode of the caret.
			class OvertypeModeToggleCommand : public Command {
			public:
				explicit OvertypeModeToggleCommand(viewers::TextViewer& viewer) /*throw()*/;
			private:
				bool perform();
			};
			/// Inserts the content of the kill ring or the clipboard at the caret position.
			class PasteCommand : public Command {
			public:
				PasteCommand(viewers::TextViewer& view, bool useKillRing) /*throw()*/;
				bool perform();
			private:
				const bool usesKillRing_;
			};
			/// Reconverts by using the input method editor.
			class ReconversionCommand : public Command {
			public:
				explicit ReconversionCommand(viewers::TextViewer& view) /*throw()*/;
			private:
				bool perform();
			};
			/// Replaces all matched texts.
			class ReplaceAllCommand : public Command {
			public:
				ReplaceAllCommand(viewers::TextViewer& viewer, bool onlySelection,
					const String& replacement, searcher::InteractiveReplacementCallback* callback) /*throw()*/;
				std::size_t numberOfLastReplacements() const /*throw()*/;
			private:
				bool perform();
				const bool onlySelection_;
				const String replacement_;
				searcher::InteractiveReplacementCallback* const callback_;
				std::size_t numberOfLastReplacements_;
			};
			/// Extends the selection and begins rectangular selection.
			class RowSelectionExtensionCommand : public Command {
			public:
				RowSelectionExtensionCommand(viewers::TextViewer& viewer,
					kernel::Position(*procedure)(const kernel::Point&));
				RowSelectionExtensionCommand(viewers::TextViewer& viewer,
					kernel::Position(*procedure)(const kernel::Point&, Index));
				RowSelectionExtensionCommand(viewers::TextViewer& viewer,
					kernel::Position(*procedure)(const kernel::Point&, kernel::locations::CharacterUnit, Index));
				RowSelectionExtensionCommand(viewers::TextViewer& viewer,
					kernel::Position(*procedure)(const viewers::VisualPoint&));
				RowSelectionExtensionCommand(viewers::TextViewer& viewer,
					kernel::Position(*procedure)(const viewers::VisualPoint&, Index));
				RowSelectionExtensionCommand(viewers::TextViewer& viewer,
					kernel::Position(*procedure)(const viewers::VisualPoint&, kernel::locations::CharacterUnit, Index));
				RowSelectionExtensionCommand(viewers::TextViewer& viewer,
					viewers::BlockProgressionDestinationProxy(*procedure)(const viewers::VisualPoint&, Index));
				bool perform();
			private:
				kernel::Position(*procedureP_)(const kernel::Point&);
				kernel::Position(*procedurePL_)(const kernel::Point&, Index);
				kernel::Position(*procedurePCL_)(const kernel::Point&, kernel::locations::CharacterUnit, Index);
				kernel::Position(*procedureV_)(const viewers::VisualPoint&);
				kernel::Position(*procedureVL_)(const viewers::VisualPoint&, Index);
				kernel::Position(*procedureVCL_)(const viewers::VisualPoint&, kernel::locations::CharacterUnit, Index);
				viewers::BlockProgressionDestinationProxy(*procedureVLV_)(const viewers::VisualPoint&, Index);
			};
			/// Tabifies (exchanges tabs and spaces).
			class TabifyCommand : public Command {
			public:
				TabifyCommand(viewers::TextViewer& view, bool untabify) /*throw()*/;
			private:
				bool perform();
				bool untabify_;
			};
			/// Inputs a text.
			class TextInputCommand : public Command {
			public:
				TextInputCommand(viewers::TextViewer& view, const String& text) /*throw()*/;
			private:
				bool perform();
				String text_;
			};
			/// Transposes (swaps) the two text elements.
			class TranspositionCommand : public Command {
			public:
				TranspositionCommand(viewers::TextViewer& view, bool(*procedure)(viewers::Caret&));
			private:
				bool perform();
				bool(*procedure_)(viewers::Caret&);
			};
			/// Performs undo or redo.
			class UndoCommand : public Command {
			public:
				UndoCommand(viewers::TextViewer& view, bool redo) /*throw()*/;
				bool isLastActionIncompleted() const;
			private:
				bool perform();
				const bool redo_;
				enum {COMPLETED, INCOMPLETED, INDETERMINATE} lastResult_;
			};
			/// Deletes the forward/backward N word(s).
			class WordDeletionCommand : public Command {
			public:
				WordDeletionCommand(viewers::TextViewer& viewer, Direction direction) /*throw()*/;
			private:
				bool perform();
				const Direction direction_;
			};
			/// Selects the current word.
			class WordSelectionCreationCommand : public Command {
			public:
				explicit WordSelectionCreationCommand(viewers::TextViewer& view) /*throw()*/;
			private:
				bool perform();
			};
		} // namespace commands


		/**
		 * Performs the command. The command can return the command-specific result value.
		 * @retval true the command succeeded
		 * @retval false ignorable or easily recoverable error occurred. or the command tried to
		 *               change the read-only document or the document's input rejected the change
		 * @throw ... a fatal error occurred. the type of exception(s) is defined by the derived
		 *            class. see the documentation of @c #perform methods of the implementation
		 * @see kernel#DocumentCantChangeException, kernel#ReadOnlyDocumentException
		 */
		inline bool Command::operator()() {const bool result = perform(); numericPrefix_ = 1; return result;}

		/// Returns the numeric prefix for the next execution.
		inline long Command::numericPrefix() const /*throw()*/ {return numericPrefix_;}

		/// Changes the command target.
		inline Command& Command::retarget(viewers::TextViewer& viewer) /*throw()*/ {viewer_ = &viewer; return *this;}

		/// Sets the numeric prefix for the next execution.
		inline Command& Command::setNumericPrefix(long number) /*throw()*/ {numericPrefix_ = number; return *this;}

	} // namespace texteditor

} // namespace ascension

#endif // !ASCENSION_COMMAND_HPP
