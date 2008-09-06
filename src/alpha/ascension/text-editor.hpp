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
			virtual ~Command() throw();
			Command& beepOnError(bool enable = true) throw();
			bool beepsOnError() const throw();
			ulong execute();
			long numericPrefix() const throw();
			Command& retarget(viewers::TextViewer& viewer) throw();
			Command& setNumericPrefix(long number) throw();
		protected:
			explicit Command(viewers::TextViewer& viewer) throw();
			/// Called by @c #execute public method.
			virtual ulong doExecute() = 0;
			/// Returns the command target.
			viewers::TextViewer& target() const throw() {return *viewer_;}
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
				BookmarkMatchLinesCommand(viewers::TextViewer& viewer, bool onlySelection) throw();
			private:
				ulong doExecute();
				const bool onlySelection_;
			};
			/// Clears the selection, or aborts the active incremental search and exits the content assist.
			class CancelCommand : public Command {
			public:
				explicit CancelCommand(viewers::TextViewer& viewer) throw();
			private:
				ulong doExecute();
			};
			/// Moves the caret or extends the selection.
			class CaretMovementCommand : public Command {
			public:
				/// Types of movement.
				enum Type {
					FORWARD_CHARACTER,							///< Moves or extends to the next (forward) character.
					BACKWARD_CHARACTER,							///< Moves or extends to the previous (backward) character.
					LEFT_CHARACTER,								///< Moves or extends to the left character.
					RIGHT_CHARACTER,							///< Moves or extends to the right character.
					NEXT_WORD,									///< Moves or extends to the start of the next word.
					PREVIOUS_WORD,								///< Moves or extends to the start of the previous word.
					LEFT_WORD,									///< Moves or extends to the start of the left word.
					RIGHT_WORD,									///< Moves or extends to the start of the right word.
					NEXT_WORDEND,								///< Moves or extends to the end of the next word.
					PREVIOUS_WORDEND,							///< Moves or extends to the end of the previous word.
					LEFT_WORDEND,								///< Moves or extends to the end of the left word.
					RIGHT_WORDEND,								///< Moves or extends to the end of the right word.
					NEXT_LINE,									///< Moves or extends to the next logical line.
					PREVIOUS_LINE,								///< Moves or extends to the previous logical line.
					NEXT_VISUAL_LINE,							///< Moves or extends to the next visual line.
					PREVIOUS_VISUAL_LINE,						///< Moves or extends to the previous visual line.
					NEXT_PAGE,									///< Moves or extends to the next page.
					PREVIOUS_PAGE,								///< Moves or extends to the previous page.
					BEGINNING_OF_LINE,							///< Moves or extends to the beginning of the logical line.
					END_OF_LINE,								///< Moves or extends to the end of the logical line.
					FIRST_PRINTABLE_CHARACTER_OF_LINE,			///< Moves or extends to the first printable character in the logical line.
					LAST_PRINTABLE_CHARACTER_OF_LINE,			///< Moves or extends to the last printable character in the logical line.
					CONTEXTUAL_BEGINNING_OF_LINE,				///< Moves or extends to the beginning of the line or the first printable character in the logical line by context.
					CONTEXTUAL_END_OF_LINE,						///< Moves or extends to the end of the line or the last printable character in the logical line by context.
					BEGINNING_OF_VISUAL_LINE,					///< Moves or extends to the start of the visual line.
					END_OF_VISUAL_LINE,							///< Moves or extends to the end of the visual line.
					FIRST_PRINTABLE_CHARACTER_OF_VISUAL_LINE,	///< Moves or extends to the first printable character in the visual line.
					LAST_PRINTABLE_CHARACTER_OF_VISUAL_LINE,	///< Moves or extends to the last printable character in the visual line.
					CONTEXTUAL_BEGINNING_OF_VISUAL_LINE,		///< Moves or extends to the beginning of the line or the first printable character in the visual line by context.
					CONTEXTUAL_END_OF_VISUAL_LINE,				///< Moves or extends to the end of the line or the last printable character in the visual line by context.
					BEGINNING_OF_DOCUMENT,						///< Moves or extends to the beginning of the document.
					END_OF_DOCUMENT,							///< Moves or extends to the end of the document.
					NEXT_BOOKMARK,								///< Moves or extends to the next bookmark.
					PREVIOUS_BOOKMARK,							///< Moves or extends to the previous bookmark.
					MATCH_BRACKET,								///< Moves or extends to the match bracket.
				};
				CaretMovementCommand(viewers::TextViewer& view, Type type, bool extend = false, length_t offset = 1) throw()
						: EditorCommand(view), type_(type), extend_(extend), offset_(offset) {}
				ulong execute();
			private:
				Type type_;
				bool extend_;
				length_t offset_;
			};
			/**
			 * Deletes the forward/backward N character(s). If the incremental search is active,
			 * deletes the entire pattern (direction is @c FORWARD) or the last N character(s)
			 * (direction is @c BACKWARD).
			 * @see WordDeletionCommand
			 */
			class CharacterDeletionCommand : public Command {
			public:
				CharacterDeletionCommand(viewers::TextViewer& viewer, Direction direction) throw();
			private:
				ulong doExecute();
				const Direction direction_;
			};
			/// Converts a character into the text represents the code value of the character.
			class CharacterToCodePointConversionCommand : public Command {
			public:
				CharacterToCodePointConversionCommand(viewers::TextViewer& viewer) throw();
			private:
				ulong doExecute();
			};
			/**
			 * Inputs a character on the caret position, or appends to the end of the active
			 * incremental search pattern.
			 * @see viewers#Caret#inputCharacter
			 */
			class CharacterInputCommand : public Command {
			public:
				CharacterInputCommand(viewers::TextViewer& viewer, CodePoint c) throw();
			private:
				ulong doExecute();
				const CodePoint c_;
			};
			/// Inputs a character is at same position in the next/previous visual line.
			class CharacterInputFromNextLineCommand : public Command {
			public:
				CharacterInputFromNextLineCommand(viewers::TextViewer& viewer, bool fromPreviousLine) throw();
			private:
				ulong doExecute();
				const bool fromPreviousLine_;
			};
			/// Converts a text represents a code value into the character has the code value.
			class CodePointToCharacterConversionCommand : public Command {
			public:
				CodePointToCharacterConversionCommand(viewers::TextViewer& view) throw();
			private:
				ulong doExecute();
			};
			/**
			 * Shows completion proposals and aborts the active incremental search.
			 * @see contentassist#ContentAssistant#showPossibleCompletions
			 */
			class CompletionProposalPopupCommand : public Command {
			public:
				explicit CompletionProposalPopupCommand(viewers::TextViewer& view) throw();
			private:
				ulong doExecute();
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
				FindNextCommand(viewers::TextViewer& viewer, Direction direction) throw();
			private:
				ulong doExecute();
				const Direction direction_;
			};
			/**
			 * Begins the incremental search. If the search is already running, jumps to the
			 * next/previous match.
			 */
			class IncrementalFindCommand : public Command {
			public:
				IncrementalFindCommand(viewers::TextViewer& view,
					Direction direction, searcher::IIncrementalSearchCallback* callback = 0) throw();
			private:
				ulong doExecute();
				const Direction direction_;
				searcher::IIncrementalSearchCallback* const callback_;
			};
			/**
			 * Makes/Deletes indents of the selected non-blank lines.
			 * @see viewers#Caret#spaceIndent, viewers#Caret#tabIndent
			 */
			class IndentationCommand : public Command {
			public:
				IndentationCommand(viewers::TextViewer& view, bool increase) throw();
			private:
				ulong doExecute();
				bool increases_;
			};
			/// Toggles the input method's open status.
			class InputMethodOpenStatusToggleCommand : public Command {
			public:
				explicit InputMethodOpenStatusToggleCommand(viewers::TextViewer& viewer) throw();
			private:
				ulong doExecute();
			};
			/// Toggles Soft Keyboard mode of the input method.
			class InputMethodSoftKeyboardModeToggleCommand : public Command {
			public:
				explicit InputMethodSoftKeyboardModeToggleCommand(viewers::TextViewer& viewer) throw();
			private:
				ulong doExecute();
			};
			/**
			 * Inserts a newline, or exits a mode.
			 * If the incremental search is running, exits the search. If the content assist is
			 * active, completes or aborts and breaks the line if no candidate matches exactly.
			 */
			class NewlineCommand : public Command {
			public:
				NewlineCommand(viewers::TextViewer& view, bool insertPrevious) throw();
			private:
				ulong doExecute();
				const bool insertsPrevious_;
			};
			/// Toggles overtype mode of the caret.
			class OvertypeModeToggleCommand : public Command {
			public:
				explicit OvertypeModeToggleCommand(viewers::TextViewer& viewer) throw();
			private:
				ulong doExecute();
			};
			/// Inserts the content of the kill ring or the clipboard at the caret position.
			class PasteCommand : public Command {
			public:
				PasteCommand(viewers::TextViewer& view, bool useKillRing) throw();
				ulong execute();
			private:
				const bool usesKillRing_;
			};
			/// Reconverts by using the input method editor.
			class ReconversionCommand : public Command {
			public:
				explicit ReconversionCommand(viewers::TextViewer& view) throw();
			private:
				ulong doExecute();
			};
			/// Replaces all matched texts.
			class ReplaceAllCommand : public Command {
			public:
				ReplaceAllCommand(viewers::TextViewer& viewer,
					bool onlySelection, searcher::IInteractiveReplacementCallback* callback) throw();
			private:
				ulong doExecute();
				const bool onlySelection_;
				searcher::IInteractiveReplacementCallback* const callback_;
			};
			/// Extends the selection and begins rectangular selection.
			class RowSelectionExtensionCommand : public Command {
			public:
				enum Type {
					FORWARD_CHARACTER,							///< Extends to the next (forward) character.
					BACKWARD_CHARACTER,							///< Extends to the previous (backward) character.
					LEFT_CHARACTER,								///< Extends to the left character.
					RIGHT_CHARACTER,							///< Extends to the right character.
					NEXT_WORD,									///< Extends to the start of the next word.
					PREVIOUS_WORD,								///< Extends to the start of the previous word.
					LEFT_WORD,									///< Extends to the start of the left word.
					RIGHT_WORD,									///< Extends to the start of the right word.
					NEXT_WORDEND,								///< Extends to the end of the next word.
					PREVIOUS_WORDEND,							///< Extends to the end of the previous word.
					LEFT_WORDEND,								///< Extends to the end of the left word.
					RIGHT_WORDEND,								///< Extends to the end of the right word.
					NEXT_LINE,									///< Extends to the next logical line.
					PREVIOUS_LINE,								///< Extends to the previous logical line.
					NEXT_VISUAL_LINE,							///< Extends to the next visual line.
					PREVIOUS_VISUAL_LINE,						///< Extends to the previous visual line.
					BEGINNING_OF_LINE,							///< Extends to the beginning of the line.
					END_OF_LINE,								///< Extends to the end of the line.
					FIRST_PRINTABLE_CHARACTER_OF_LINE,			///< Extends to the first printable character in the line.
					LAST_PRINTABLE_CHARACTER_OF_LINE,			///< Extends to the last printable character in the line.
					CONTEXTUAL_BEGINNING_OF_LINE,				///< Extends to the beginning of the line or first printable character in the line by context.
					CONTEXTUAL_END_OF_LINE,						///< Extends to the end of the line or last printable character in the line by context.
					BEGINNING_OF_VISUAL_LINE,					///< Extends to the beginning of the visual line.
					END_OF_VISUAL_LINE,							///< Extends to the end of the visual line.
					FIRST_PRINTABLE_CHARACTER_OF_VISUAL_LINE,	///< Extends to the first printable character in the visual line.
					LAST_PRINTABLE_CHARACTER_OF_VISUAL_LINE,	///< Extends to the last printable character in the visual line.
					CONTEXTUAL_BEGINNING_OF_VISUAL_LINE,		///< Extends to the beginning of the visual line or first printable character in the visual line by context.
					CONTEXTUAL_END_OF_VISUAL_LINE,				///< Extends to the end of the visual line or last printable character in the visual line by context.
				};
				RowSelectionExtensionCommand(viewers::TextViewer& view, Type type) throw() : EditorCommand(view), type_(type) {}
				ulong execute();
			private:
				Type type_;
			};
			/// Creates the selection.
			class SelectionCreationCommand : public EditorCommand {
			public:
				enum Type {
					ALL,			///< Selects the all of the document.
					CURRENT_WORD	///< Selects the current word.
				};
				SelectionCreationCommand(viewers::TextViewer& view, Type type) throw() : EditorCommand(view), type_(type) {}
				ulong execute();
			private:
				Type type_;
			};
			/// Tabifies (exchanges tabs and spaces).
			class TabifyCommand : public internal::EditorCommandBase<bool> {
			public:
				TabifyCommand(viewers::TextViewer& view, bool tabify) throw() : internal::EditorCommandBase<bool>(view, tabify) {}
				ulong execute();
			};
			/// Inputs a text.
			class TextInputCommand : public internal::EditorCommandBase<String> {
			public:
				TextInputCommand(viewers::TextViewer& view, const String& text) throw() : internal::EditorCommandBase<String>(view, text) {}
				ulong execute();
			};
			/// Transposes (swaps) the two text elements.
			class TranspositionCommand : public EditorCommand {
			public:
				enum Type {
					CHARACTERS,	///< Transposes the characters.
					LINES,		///< Transposes the lines.
					WORDS,		///< Transposes the words.
//					SENTENCES,	///< Transposes the sentences.
//					PARAGRAPHS	///< Transposes the paragraphs.
				};
				TranspositionCommand(viewers::TextViewer& view, Type type) throw() : EditorCommand(view), type_(type) {}
				ulong execute();
			private:
				Type type_;
			};
			/// Performs undo or redo.
			class UndoCommand : public Command {
			public:
				UndoCommand(viewers::TextViewer& view, bool redo) throw();
			private:
				ulong doExecute();
				const bool redo_;
			};
			/// Deletes the forward/backward N word(s).
			class WordDeletionCommand : public Command {
			public:
				WordDeletionCommand(viewers::TextViewer& viewer, Direction direction) throw();
			private:
				ulong doExecute();
				const Direction direction_;
			};
		} // namespace commands

		/// Standard input sequence checkers.
		namespace isc {
			/// I.S.C. for Ainu.
			class AinuInputSequenceChecker : virtual public InputSequenceChecker {
			public:
				bool check(HKL keyboardLayout, const Char* first, const Char* last, CodePoint cp) const;
			};
			/// I.S.C. for Thai.
			class ThaiInputSequenceChecker : virtual public InputSequenceChecker {
				MANAH_UNASSIGNABLE_TAG(ThaiInputSequenceChecker);
			public:
				enum Mode {PASS_THROUGH, BASIC_MODE, STRICT_MODE};
				ThaiInputSequenceChecker(Mode mode = BASIC_MODE) throw() : mode_(mode) {}
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
				static CharacterClass getCharacterClass(CodePoint cp) throw() {
					if(cp < 0x0020 || cp == 0x007F)			return CTRL;
					else if(cp >= 0x0E00 && cp < 0x0E60)	return charClasses_[cp - 0x0E00];
					else if(cp >= 0x0E60 && cp < 0x0E80)	return CTRL;
					else									return NON;
				}
				static bool doCheck(CharacterClass lead, CharacterClass follow, bool strict) throw() {
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
			class VietnameseInputSequenceChecker : virtual public InputSequenceChecker {
			public:
				bool check(HKL keyboardLayout, const Char* first, const Char* last, CodePoint cp) const;
			};
		} // namespace isc

	} // namespace texteditor

} // namespace ascension

#endif // !ASCENSION_TEXT_EDITOR_HPP
