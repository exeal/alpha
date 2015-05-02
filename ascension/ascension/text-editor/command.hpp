/**
 * @file command.hpp
 * @author exeal
 * @date 2006-2011 was text-editor.hpp
 * @date 2011-05-06
 * @date 2011-2013, 2015
 */

#ifndef ASCENSION_COMMAND_HPP
#define ASCENSION_COMMAND_HPP
#include <ascension/kernel/searcher.hpp>

namespace ascension {

	namespace viewer {
		class BlockProgressionDestinationProxy;
		class TextViewer;
	}

	namespace texteditor {
		/**
		 * Abstract class for the editor commands.
		 * @see ascension#texteditor#commands
		 */
		class Command {
		public:
			typedef unsigned long NumericPrefix;	///< Type of numeric prefixes.
		public:
			virtual ~Command() throw();
			bool operator()();
			NumericPrefix numericPrefix() const BOOST_NOEXCEPT;
			Command& retarget(viewer::TextViewer& viewer) BOOST_NOEXCEPT;
			Command& setNumericPrefix(NumericPrefix number) BOOST_NOEXCEPT;
		protected:
			explicit Command(viewer::TextViewer& viewer) BOOST_NOEXCEPT;
			/// Returns the text viewer which is the target of this command.
			viewer::TextViewer& target() const BOOST_NOEXCEPT {return *viewer_;}
		private:
			/// Called by @c #operator(). For semantics, see @c #operator().
			virtual bool perform() = 0;
		private:
			viewer::TextViewer* viewer_;
			NumericPrefix numericPrefix_;
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
				explicit BookmarkMatchLinesCommand(viewer::TextViewer& viewer,
					const kernel::Region& region = kernel::Region()) BOOST_NOEXCEPT;
				Index numberOfMarkedLines() const BOOST_NOEXCEPT;
			private:
				bool perform();
				const kernel::Region region_;
				Index numberOfMarkedLines_;
			};
			/// Clears the selection, or aborts the active incremental search and exits the content assist.
			class CancelCommand : public Command {
			public:
				explicit CancelCommand(viewer::TextViewer& viewer) BOOST_NOEXCEPT;
			private:
				bool perform();
			};
			/**
			 * Moves the caret or extends the selection.
			 * @c kernel#locations#CharacterUnit#GRAPHEME_CLUSTER is always used as character unit.
			 * @tparam ProcedureSignature Type of the function gives a motion
			 * @see CaretMovementToDefinedPositionCommand, viewer#Caret, kernel#locations
			 */
			template<typename ProcedureSignature>
			class CaretMovementCommand : public Command {
			public:
				CaretMovementCommand(viewer::TextViewer& viewer,
					ProcedureSignature* procedure, Direction direction, bool extendSelection = false);
			private:
				bool perform();
				ProcedureSignature* const procedure_;
				const Direction direction_;
				const bool extends_;
			};
			/**
			 * Moves the caret or extends the selection to a defined position.
			 * @tparam ProcedureSignature Type of the function gives a motion
			 * @see CaretMovementCommand, viewer#Caret, kernel#locations
			 */
			template<typename ProcedureSignature>
			class CaretMovementToDefinedPositionCommand : public Command {
			public:
				CaretMovementToDefinedPositionCommand(viewer::TextViewer& viewer,
					ProcedureSignature* procedure, bool extendSelection = false);
			private:
				bool perform();
				ProcedureSignature* const procedure_;
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
				CharacterDeletionCommand(viewer::TextViewer& viewer, Direction direction) BOOST_NOEXCEPT;
			private:
				bool perform();
				const Direction direction_;
			};
			/// Converts a character into the text represents the code value of the character.
			class CharacterToCodePointConversionCommand : public Command {
			public:
				CharacterToCodePointConversionCommand(viewer::TextViewer& viewer) BOOST_NOEXCEPT;
			private:
				bool perform();
			};
			/**
			 * Inputs a character on the caret position, or appends to the end of the active
			 * incremental search pattern.
			 * @see viewer#Caret#inputCharacter
			 */
			class CharacterInputCommand : public Command {
			public:
				CharacterInputCommand(viewer::TextViewer& viewer, CodePoint c);
			private:
				bool perform();
				const CodePoint c_;
			};
			/// Inputs a character is at same position in the next/previous visual line.
			class CharacterInputFromNextLineCommand : public Command {
			public:
				CharacterInputFromNextLineCommand(viewer::TextViewer& viewer, bool fromPreviousLine) BOOST_NOEXCEPT;
			private:
				bool perform();
				const bool fromPreviousLine_;
			};
			/// Converts a text represents a code value into the character has the code value.
			class CodePointToCharacterConversionCommand : public Command {
			public:
				CodePointToCharacterConversionCommand(viewer::TextViewer& view) BOOST_NOEXCEPT;
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
			/// Selects the entire document.
			class EntireDocumentSelectionCreationCommand : public Command {
			public:
				explicit EntireDocumentSelectionCreationCommand(viewer::TextViewer& view) BOOST_NOEXCEPT;
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
				FindNextCommand(viewer::TextViewer& viewer, Direction direction) BOOST_NOEXCEPT;
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
				IncrementalFindCommand(viewer::TextViewer& view, searcher::TextSearcher::Type type,
					Direction direction, searcher::IncrementalSearchCallback* callback = nullptr) BOOST_NOEXCEPT;
			private:
				bool perform();
				searcher::TextSearcher::Type type_;
				const Direction direction_;
				searcher::IncrementalSearchCallback* const callback_;
			};
			/**
			 * Makes/Deletes indents of the selected non-blank lines.
			 * @see viewer#Caret#spaceIndent, viewer#Caret#tabIndent
			 */
			class IndentationCommand : public Command {
			public:
				IndentationCommand(viewer::TextViewer& view, bool increase) BOOST_NOEXCEPT;
			private:
				bool perform();
				bool increases_;
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
			/// Moves the caret or extends the selection to the match bracket.
			class MatchBracketCommand : public Command {
			public:
				MatchBracketCommand(viewer::TextViewer& viewer, bool extendSelection = false) BOOST_NOEXCEPT;
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
				NewlineCommand(viewer::TextViewer& view, boost::optional<Direction> direction = boost::none) BOOST_NOEXCEPT;
			private:
				bool perform();
				const boost::optional<Direction> direction_;
			};
			/// Toggles overtype mode of the caret.
			class OvertypeModeToggleCommand : public Command {
			public:
				explicit OvertypeModeToggleCommand(viewer::TextViewer& viewer) BOOST_NOEXCEPT;
			private:
				bool perform();
			};
			/// Inserts the content of the kill ring or the clipboard at the caret position.
			class PasteCommand : public Command {
			public:
				PasteCommand(viewer::TextViewer& view, bool useKillRing) BOOST_NOEXCEPT;
				bool perform();
			private:
				const bool usesKillRing_;
			};
			/// Reconverts by using the input method editor.
			class ReconversionCommand : public Command {
			public:
				explicit ReconversionCommand(viewer::TextViewer& view) BOOST_NOEXCEPT;
			private:
				bool perform();
			};
			/// Replaces all matched texts.
			class ReplaceAllCommand : public Command {
			public:
				ReplaceAllCommand(viewer::TextViewer& viewer, bool onlySelection,
					const String& replacement, searcher::InteractiveReplacementCallback* callback) BOOST_NOEXCEPT;
				std::size_t numberOfLastReplacements() const BOOST_NOEXCEPT;
			private:
				bool perform();
				const bool onlySelection_;
				const String replacement_;
				searcher::InteractiveReplacementCallback* const callback_;
				std::size_t numberOfLastReplacements_;
			};
			/**
			 * Extends the selection and begins rectangular selection.
			 * @tparam ProcedureSignature Type of the function gives a motion
			 * @see RowSelectionExtensionToDefinedPositionCommand, viewer#Caret, kernel#locations
			 */
			template<typename ProcedureSignature>
			class RowSelectionExtensionCommand : public Command {
			public:
				RowSelectionExtensionCommand(viewer::TextViewer& viewer,
					ProcedureSignature* procedure, Direction direction);
			private:
				bool perform();
				ProcedureSignature* const procedure_;
				const Direction direction_;
			};
			/**
			 * Extends the selection to a defined position and begins rectangular selection.
			 * @tparam ProcedureSignature Type of the function gives a motion
			 * @see RowSelectionExtensionCommand, viewer#Caret, kernel#locations
			 */
			template<typename ProcedureSignature>
			class RowSelectionExtensionToDefinedPositionCommand : public Command {
			public:
				RowSelectionExtensionToDefinedPositionCommand(
					viewer::TextViewer& viewer, ProcedureSignature* procedure);
			private:
				bool perform();
				ProcedureSignature* const procedure_;
			};
			/// Tabifies (exchanges tabs and spaces).
			class TabifyCommand : public Command {
			public:
				TabifyCommand(viewer::TextViewer& view, bool untabify) BOOST_NOEXCEPT;
			private:
				bool perform();
				bool untabify_;
			};
			/// Inputs a text.
			class TextInputCommand : public Command {
			public:
				TextInputCommand(viewer::TextViewer& view, const String& text) BOOST_NOEXCEPT;
			private:
				bool perform();
				String text_;
			};
			/// Transposes (swaps) the two text elements.
			class TranspositionCommand : public Command {
			public:
				TranspositionCommand(viewer::TextViewer& view, bool(*procedure)(viewer::Caret&));
			private:
				bool perform();
				bool(*procedure_)(viewer::Caret&);
			};
			/// Performs undo or redo.
			class UndoCommand : public Command {
			public:
				UndoCommand(viewer::TextViewer& view, bool redo) BOOST_NOEXCEPT;
				bool isLastActionIncompleted() const;
			private:
				bool perform();
				const bool redo_;
				enum {COMPLETED, INCOMPLETED, INDETERMINATE} lastResult_;
			};
			/// Deletes the forward/backward N word(s).
			class WordDeletionCommand : public Command {
			public:
				WordDeletionCommand(viewer::TextViewer& viewer, Direction direction) BOOST_NOEXCEPT;
			private:
				bool perform();
				const Direction direction_;
			};
			/// Selects the current word.
			class WordSelectionCreationCommand : public Command {
			public:
				explicit WordSelectionCreationCommand(viewer::TextViewer& view) BOOST_NOEXCEPT;
			private:
				bool perform();
			};

			template<typename ProcedureSignature>
			inline CaretMovementCommand<ProcedureSignature> makeCaretMovementCommand(
					viewer::TextViewer& viewer, ProcedureSignature* procedure,
					Direction direction, bool extendSelection = false) {
				return CaretMovementCommand<ProcedureSignature>(
					viewer, procedure, direction, extendSelection);
			}
			template<typename ProcedureSignature>
			inline CaretMovementToDefinedPositionCommand<ProcedureSignature> makeCaretMovementCommand(
					viewer::TextViewer& viewer, ProcedureSignature* procedure,
					bool extendSelection = false) {
				return CaretMovementToDefinedPositionCommand<ProcedureSignature>(
					viewer, procedure, extendSelection);
			}
			template<typename ProcedureSignature>
			inline RowSelectionExtensionCommand<ProcedureSignature> makeRowSelectionExtensionCommand(
					viewer::TextViewer& viewer, ProcedureSignature* procedure, Direction direction) {
				return RowSelectionExtensionCommand<ProcedureSignature>(viewer, procedure, direction);
			}
			template<typename ProcedureSignature>
			inline RowSelectionExtensionToDefinedPositionCommand<ProcedureSignature> makeRowSelectionExtensionCommand(
					viewer::TextViewer& viewer, ProcedureSignature* procedure) {
				return RowSelectionExtensionToDefinedPositionCommand<ProcedureSignature>(viewer, procedure);
			}
		} // namespace commands


		/**
		 * Performs the command. The command can return the command-specific result value. If the
		 * command didn't throw an exception, this resets the numeric prefix to 1.
		 * @retval true The command succeeded
		 * @retval false An ignorable or easily recoverable error occurred. Or the command tried to
		 *               change the read-only document or the document's input rejected the change
		 * @throw ... A fatal error occurred. The type of exception(s) is defined by the derived
		 *            class. See the documentation of @c #perform methods of the implementation
		 * @see kernel#DocumentCantChangeException, kernel#ReadOnlyDocumentException
		 */
		inline bool Command::operator()() {
			const bool result = perform();
			setNumericPrefix(1);
			return result;
		}

		/// Returns the numeric prefix for the next execution.
		inline Command::NumericPrefix Command::numericPrefix() const BOOST_NOEXCEPT {
			return numericPrefix_;
		}

		/**
		 * Changes the command target.
		 * @param viewer The text viewer as the new target to set
		 * @return This command
		 */
		inline Command& Command::retarget(viewer::TextViewer& viewer) BOOST_NOEXCEPT {
			viewer_ = &viewer;
			return *this;
		}

		/**
		 * Sets the numeric prefix for the next performance.
		 * @param number The new numeric prefix
		 * @return This command
		 */
		inline Command& Command::setNumericPrefix(NumericPrefix number) BOOST_NOEXCEPT {
			numericPrefix_ = number;
			return *this;
		}

	} // namespace texteditor

} // namespace ascension

#endif // !ASCENSION_COMMAND_HPP
