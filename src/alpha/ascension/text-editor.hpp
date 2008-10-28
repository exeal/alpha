/**
 * @file text-editor.hpp
 * @author exeal
 * @date 2006-2008
 */

#ifndef ASCENSION_TEXT_EDITOR_HPP
#define ASCENSION_TEXT_EDITOR_HPP
#include "session.hpp"
#include "viewer.hpp"
#include "searcher.hpp"

namespace ascension {

	namespace texteditor {

		/**
		 * Abstract class for the editor commands.
		 * @see ascension#texteditor#commands
		 */
		class Command {
		public:
			virtual ~Command() /*throw()*/;
			ulong operator()();
			Command& beepOnError(bool enable = true) /*throw()*/;
			bool beepsOnError() const /*throw()*/;
			long numericPrefix() const /*throw()*/;
			Command& retarget(viewers::TextViewer& viewer) /*throw()*/;
			Command& setNumericPrefix(long number) /*throw()*/;
		protected:
			explicit Command(viewers::TextViewer& viewer) /*throw()*/;
			/// Returns the command target.
			viewers::TextViewer& target() const /*throw()*/ {return *viewer_;}
		private:
			/// Called by @c #operator().
			virtual ulong perform() = 0;
		private:
			viewers::TextViewer* viewer_;
			long numericPrefix_;
			bool beepsOnError_;
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
			private:
				ulong perform();
				const kernel::Region region_;
			};
			/// Clears the selection, or aborts the active incremental search and exits the content assist.
			class CancelCommand : public Command {
			public:
				explicit CancelCommand(viewers::TextViewer& viewer) /*throw()*/;
			private:
				ulong perform();
			};
			/// Moves the caret or extends the selection.
			class CaretMovementCommand : public Command {
			public:
				CaretMovementCommand(viewers::TextViewer& viewer,
					kernel::Position(viewers::Caret::*procedure)(void) const, bool extendSelection = false);
				CaretMovementCommand(viewers::TextViewer& viewer,
					kernel::Position(viewers::Caret::*procedure)(length_t) const, bool extendSelection = false);
				CaretMovementCommand(viewers::TextViewer& viewer,
					viewers::VerticalDestinationProxy(viewers::Caret::*procedure)(length_t) const, bool extendSelection = false);
			private:
				ulong perform();
				kernel::Position(viewers::Caret::*const procedure0_)(void) const;
				kernel::Position(viewers::Caret::*const procedure1_)(length_t) const;
				viewers::VerticalDestinationProxy(viewers::Caret::*const procedureV1_)(length_t) const;
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
				ulong perform();
				const Direction direction_;
			};
			/// Converts a character into the text represents the code value of the character.
			class CharacterToCodePointConversionCommand : public Command {
			public:
				CharacterToCodePointConversionCommand(viewers::TextViewer& viewer) /*throw()*/;
			private:
				ulong perform();
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
				ulong perform();
				const CodePoint c_;
			};
			/// Inputs a character is at same position in the next/previous visual line.
			class CharacterInputFromNextLineCommand : public Command {
			public:
				CharacterInputFromNextLineCommand(viewers::TextViewer& viewer, bool fromPreviousLine) /*throw()*/;
			private:
				ulong perform();
				const bool fromPreviousLine_;
			};
			/// Converts a text represents a code value into the character has the code value.
			class CodePointToCharacterConversionCommand : public Command {
			public:
				CodePointToCharacterConversionCommand(viewers::TextViewer& view) /*throw()*/;
			private:
				ulong perform();
			};
			/**
			 * Shows completion proposals and aborts the active incremental search.
			 * @see contentassist#ContentAssistant#showPossibleCompletions
			 */
			class CompletionProposalPopupCommand : public Command {
			public:
				explicit CompletionProposalPopupCommand(viewers::TextViewer& view) /*throw()*/;
			private:
				ulong perform();
			};
			/// Selects the entire document.
			class EntireDocumentSelectionCreationCommand : public Command {
			public:
				explicit EntireDocumentSelectionCreationCommand(viewers::TextViewer& view) /*throw()*/;
			private:
				ulong perform();
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
				ulong perform();
				const Direction direction_;
			};
			/**
			 * Begins the incremental search. If the search is already running, jumps to the
			 * next/previous match.
			 */
			class IncrementalFindCommand : public Command {
			public:
				IncrementalFindCommand(viewers::TextViewer& view,
					Direction direction, searcher::IIncrementalSearchCallback* callback = 0) /*throw()*/;
			private:
				ulong perform();
				const Direction direction_;
				searcher::IIncrementalSearchCallback* const callback_;
			};
			/**
			 * Makes/Deletes indents of the selected non-blank lines.
			 * @see viewers#Caret#spaceIndent, viewers#Caret#tabIndent
			 */
			class IndentationCommand : public Command {
			public:
				IndentationCommand(viewers::TextViewer& view, bool increase) /*throw()*/;
			private:
				ulong perform();
				bool increases_;
			};
			/// Toggles the input method's open status.
			class InputMethodOpenStatusToggleCommand : public Command {
			public:
				explicit InputMethodOpenStatusToggleCommand(viewers::TextViewer& viewer) /*throw()*/;
			private:
				ulong perform();
			};
			/// Toggles Soft Keyboard mode of the input method.
			class InputMethodSoftKeyboardModeToggleCommand : public Command {
			public:
				explicit InputMethodSoftKeyboardModeToggleCommand(viewers::TextViewer& viewer) /*throw()*/;
			private:
				ulong perform();
			};
			/// Moves the caret or extends the selection to the match bracket.
			class MatchBracketCommand : public Command {
			public:
				MatchBracketCommand(viewers::TextViewer& viewer, bool extendSelection = false) /*throw()*/;
			private:
				ulong perform();
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
				ulong perform();
				const bool insertsPrevious_;
			};
			/// Toggles overtype mode of the caret.
			class OvertypeModeToggleCommand : public Command {
			public:
				explicit OvertypeModeToggleCommand(viewers::TextViewer& viewer) /*throw()*/;
			private:
				ulong perform();
			};
			/// Inserts the content of the kill ring or the clipboard at the caret position.
			class PasteCommand : public Command {
			public:
				PasteCommand(viewers::TextViewer& view, bool useKillRing) /*throw()*/;
				ulong perform();
			private:
				const bool usesKillRing_;
			};
			/// Reconverts by using the input method editor.
			class ReconversionCommand : public Command {
			public:
				explicit ReconversionCommand(viewers::TextViewer& view) /*throw()*/;
			private:
				ulong perform();
			};
			/// Replaces all matched texts.
			class ReplaceAllCommand : public Command {
			public:
				ReplaceAllCommand(viewers::TextViewer& viewer,
					bool onlySelection, searcher::IInteractiveReplacementCallback* callback) /*throw()*/;
			private:
				ulong perform();
				const bool onlySelection_;
				searcher::IInteractiveReplacementCallback* const callback_;
			};
			/// Extends the selection and begins rectangular selection.
			class RowSelectionExtensionCommand : public Command {
			public:
				RowSelectionExtensionCommand(viewers::TextViewer& viewer,
					kernel::Position(viewers::Caret::*procedure)(void) const);
				RowSelectionExtensionCommand(viewers::TextViewer& viewer,
					kernel::Position(viewers::Caret::*procedure)(length_t) const);
				RowSelectionExtensionCommand(viewers::TextViewer& viewer,
					viewers::VerticalDestinationProxy(viewers::Caret::*procedure)(length_t) const);
				ulong perform();
			private:
				kernel::Position(viewers::Caret::*procedure0_)(void) const;
				kernel::Position(viewers::Caret::*procedure1_)(length_t) const;
				viewers::VerticalDestinationProxy(viewers::Caret::*procedureV1_)(length_t) const;
			};
			/// Tabifies (exchanges tabs and spaces).
			class TabifyCommand : public Command {
			public:
				TabifyCommand(viewers::TextViewer& view, bool untabify) /*throw()*/;
			private:
				ulong perform();
				bool untabify_;
			};
			/// Inputs a text.
			class TextInputCommand : public Command {
			public:
				TextInputCommand(viewers::TextViewer& view, const String& text) /*throw()*/;
			private:
				ulong perform();
				String text_;
			};
			/// Transposes (swaps) the two text elements.
			class TranspositionCommand : public Command {
			public:
				TranspositionCommand(viewers::TextViewer& view, bool(kernel::EditPoint::*procedure)(void));
			private:
				ulong perform();
				bool(kernel::EditPoint::*const procedure_)(void);
			};
			/// Performs undo or redo.
			class UndoCommand : public Command {
			public:
				UndoCommand(viewers::TextViewer& view, bool redo) /*throw()*/;
			private:
				ulong perform();
				const bool redo_;
			};
			/// Deletes the forward/backward N word(s).
			class WordDeletionCommand : public Command {
			public:
				WordDeletionCommand(viewers::TextViewer& viewer, Direction direction) /*throw()*/;
			private:
				ulong perform();
				const Direction direction_;
			};
			/// Selects the current word.
			class WordSelectionCreationCommand : public Command {
			public:
				explicit WordSelectionCreationCommand(viewers::TextViewer& view) /*throw()*/;
			private:
				ulong perform();
			};
		} // namespace commands

		/// Standard input sequence checkers.
		namespace isc {
			/// I.S.C. for Ainu.
			class AinuInputSequenceChecker : public InputSequenceChecker {
			public:
				bool check(HKL keyboardLayout, const Char* first, const Char* last, CodePoint cp) const;
			};
			/// I.S.C. for Thai.
			class ThaiInputSequenceChecker : public InputSequenceChecker {
				MANAH_UNASSIGNABLE_TAG(ThaiInputSequenceChecker);
			public:
				enum Mode {PASS_THROUGH, BASIC_MODE, STRICT_MODE};
				ThaiInputSequenceChecker(Mode mode = BASIC_MODE) /*throw()*/ : mode_(mode) {}
				bool check(HKL keyboardLayout, const Char* first, const Char* last, CodePoint cp) const;
			private:
				enum CharacterClass {
					CTRL, NON, CONS,	// treat unassigned characters in Thai block as controls
					LV, FV1, FV2, FV3, BV1, BV2,
					BD, TONE, AD1, AD2, AD3,
					AV1, AV2, AV3,
					CHARCLASS_COUNT
				};
				const Mode mode_;
				static const CharacterClass charClasses_[];
				static const char checkMap_[];
				static CharacterClass getCharacterClass(CodePoint cp) /*throw()*/ {
					if(cp < 0x0020 || cp == 0x007F)			return CTRL;
					else if(cp >= 0x0E00 && cp < 0x0E60)	return charClasses_[cp - 0x0E00];
					else if(cp >= 0x0E60 && cp < 0x0E80)	return CTRL;
					else									return NON;
				}
				static bool doCheck(CharacterClass lead, CharacterClass follow, bool strict) /*throw()*/ {
					const char result = checkMap_[lead * CHARCLASS_COUNT + follow];
					if(result == 'A' || result == 'C' || result == 'X')
						return true;
					else if(result == 'R')
						return false;
					else /* if(result == 'S') */
						return !strict;
				}
			};
			/// I.S.C. for Vietnamese.
			class VietnameseInputSequenceChecker : public InputSequenceChecker {
			public:
				bool check(HKL keyboardLayout, const Char* first, const Char* last, CodePoint cp) const;
			};
		} // namespace isc


		/// Performs the command and returns the command-specific result value.
		inline ulong Command::operator()() {const ulong result = perform(); numericPrefix_ = 1; return result;}

		/// Sets beep-on-error mode.
		inline Command& Command::beepOnError(bool enable /* = true */) /*throw()*/ {beepsOnError_ = enable; return *this;}

		/// Returns true if the command beeps when error occured.
		inline bool Command::beepsOnError() const /*throw()*/ {return beepsOnError_;}

		/// Returns the numeric prefix for the next execution.
		inline long Command::numericPrefix() const /*throw()*/ {return numericPrefix_;}

		/// Changes the command target.
		inline Command& Command::retarget(viewers::TextViewer& viewer) /*throw()*/ {viewer_ = &viewer; return *this;}

		/// Sets the numeric prefix for the next execution.
		inline Command& Command::setNumericPrefix(long number) /*throw()*/ {numericPrefix_ = number; return *this;}

	} // namespace texteditor

} // namespace ascension

#endif // !ASCENSION_TEXT_EDITOR_HPP
