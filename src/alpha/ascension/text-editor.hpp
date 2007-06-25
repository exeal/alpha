/**
 * @file text-editor.hpp
 * @author exeal
 * @date 2006-2007
 */

#ifndef ASCENSION_TEXT_EDITOR_HPP
#define ASCENSION_TEXT_EDITOR_HPP
#include "session.hpp"
#include "viewer.hpp"
#include "searcher.hpp"

namespace ascension {

	namespace texteditor {

		class TextEditor {};

		/**
		 * Abstract class for the editor commands.
		 * @see ascension#commands
		 */
		class EditorCommand {
		public:
			/// Constructor.
			EditorCommand(viewers::TextViewer& view) throw() : view_(&view) {}
			/// Destructor.
			virtual ~EditorCommand() throw() {}
			/// Executes the command and returns the command-specific result value.
			virtual ulong execute() = 0;
			/// Changes the command target.
			void retarget(viewers::TextViewer& view) throw() {view_ = &view;}
		protected:
			/// Returns the command target.
			viewers::TextViewer& getTarget() const throw() {return *view_;}
		private:
			viewers::TextViewer* view_;
		};

		/// @internal
		namespace internal {
			template<typename Parameter> class EditorCommandBase : public EditorCommand {
			public:
				EditorCommandBase(viewers::TextViewer& view, Parameter param) : EditorCommand(view), param_(param) {}
				virtual ~EditorCommandBase() {}
			protected:
				Parameter param_;
			};
		} // namespace internal

		/// Implementations of the standard commands.
		namespace commands {
			/// Bookmark operations.
			class BookmarkCommand : public EditorCommand {
			public:
				enum Type {
					CLEAR_ALL,			///< Removes all bookmarks.
					TOGGLE_CURRENT_LINE	///< Toggles the current bookmark.
				};
				BookmarkCommand(viewers::TextViewer& view, Type type) throw() : EditorCommand(view), type_(type) {}
				ulong execute();
			private:
				Type type_;
			};
			/// Clears the selection or cancels incremental search.
			class CancelCommand : public EditorCommand {
			public:
				explicit CancelCommand(viewers::TextViewer& view) throw() : EditorCommand(view) {}
				ulong execute();
			};
			/// Moves the caret or extends the selection.
			class CaretMovementCommand : public EditorCommand {
			public:
				enum Type {
					NEXT_CHARACTER,			///< Moves or extends to the next character.
					PREVIOUS_CHARACTER,		///< Moves or extends to the previous character.
					LEFT_CHARACTER,			///< Moves or extends to the left character.
					RIGHT_CHARACTER,		///< Moves or extends to the right character.
					NEXT_WORD,				///< Moves or extends to the start of the next word.
					PREVIOUS_WORD,			///< Moves or extends to the start of the previous word.
					LEFT_WORD,				///< Moves or extends to the start of the left word.
					RIGHT_WORD,				///< Moves or extends to the start of the right word.
					NEXT_WORDEND,			///< Moves or extends to the end of the next word.
					PREVIOUS_WORDEND,		///< Moves or extends to the end of the previous word.
					LEFT_WORDEND,			///< Moves or extends to the end of the left word.
					RIGHT_WORDEND,			///< Moves or extends to the end of the right word.
					NEXT_LINE,				///< Moves or extends to the next logical line.
					PREVIOUS_LINE,			///< Moves or extends to the previous logical line.
					VISUAL_NEXT_LINE,		///< Moves or extends to the next visual line.
					VISUAL_PREVIOUS_LINE,	///< Moves or extends to the previous visual line.
					NEXT_PAGE,				///< Moves or extends to the next page.
					PREVIOUS_PAGE,			///< Moves or extends to the previous page.
					START_OF_LINE,			///< Moves or extends to the start of the line.
					END_OF_LINE,			///< Moves or extends to the end of the line.
					FIRST_CHAR_OF_LINE,		///< Moves or extends to the first character in the line.
					LAST_CHAR_OF_LINE,		///< Moves or extends to the last character in the line.
					START_OR_FIRST_OF_LINE,	///< Moves or extends to the start of the line or the first character in the line by context.
					END_OR_LAST_OF_LINE,	///< Moves or extends to the end of the line or the last character in the line by context.
					START_OF_DOCUMENT,		///< Moves or extends to the start of the document.
					END_OF_DOCUMENT,		///< Moves or extends to the end of the document.
					NEXT_BOOKMARK,			///< Moves or extends to the next bookmark.
					PREVIOUS_BOOKMARK,		///< Moves or extends to the previous bookmark.
					MATCH_BRACKET,			///< Moves or extends to the match bracket.
				};
				CaretMovementCommand(viewers::TextViewer& view, Type type, bool extend = false, length_t offset = 1) throw()
						: EditorCommand(view), type_(type), extend_(extend), offset_(offset) {}
				ulong execute();
			private:
				Type		type_;
				bool		extend_;
				length_t	offset_;
			};
			/// Exchanges a character and a text represents a code value.
			class CharacterCodePointConversionCommand : public internal::EditorCommandBase<bool> {
			public:
				CharacterCodePointConversionCommand(viewers::TextViewer& view, bool charToCp) throw() :
					internal::EditorCommandBase<bool>(view, charToCp) {}
				ulong execute();
			};
			/// Inputs a character.
			class CharacterInputCommand : public internal::EditorCommandBase<CodePoint> {
			public:
				CharacterInputCommand(viewers::TextViewer& view, CodePoint cp) throw() : internal::EditorCommandBase<CodePoint>(view, cp) {}
				ulong execute();
			};
			/// Inputs a character is at same position in the next/previous line.
			class CharacterInputFromNextLineCommand : internal::EditorCommandBase<bool> {
			public:
				CharacterInputFromNextLineCommand(viewers::TextViewer& view, bool fromNextLine) throw() :
					internal::EditorCommandBase<bool>(view, fromNextLine) {}
				ulong execute();
			};
			/// Operates the clipboard.
			class ClipboardCommand : public EditorCommand {
			public:
				enum Type {
					COPY,	///< Copy the selection into the clipboard.
					CUT,	///< Cuts the selection and copys.
					PASTE	///< Pastes the content of the clipboard at the caret.
				};
				ClipboardCommand(viewers::TextViewer& view, Type type, bool performClipboardRing) throw()
					: EditorCommand(view), type_(type), performClipboardRing_(performClipboardRing) {}
				ulong execute();
			private:
				Type type_;
				bool performClipboardRing_;
			};
			/// Shows completion proposals.
			class CompletionProposalPopupCommand : public EditorCommand {
			public:
				explicit CompletionProposalPopupCommand(viewers::TextViewer& view) throw() : EditorCommand(view) {}
				ulong execute();
			};
			/// Erases the text in the document.
			class DeletionCommand : public EditorCommand {
			public:
				enum Type {
					NEXT_CHARACTER,		///< Erases the next character. Or resets the pattern in incremental search.
					PREVIOUS_CHARACTER,	///< Erases the previous character. Or undos the last operation in incremental search.
					NEXT_WORD,			///< Erases to the start of the next word.
					PREVIOUS_WORD,		///< Erases to the start of the previous word.
					WHOLE_LINE			///< Erases the whole line.
				};
				DeletionCommand(viewers::TextViewer& view, Type type) throw() : EditorCommand(view), type_(type) {}
				ulong execute();
			private:
				Type type_;
			};
			/// Searches all.
			class FindAllCommand : public EditorCommand {
			public:
				enum Type {
					BOOKMARK,	///< Sets bookmarks at the all matched lines.
					REPLACE		///< Replaces the all matched texts.
				};
				FindAllCommand(viewers::TextViewer& view, Type type, bool onlySelection) throw()
					: EditorCommand(view), type_(type), onlySelection_(onlySelection) {}
				ulong execute();
			private:
				Type type_;
				bool onlySelection_;
			};
			/// Searches the next or the previous.
			class FindNextCommand : public EditorCommand {
			public:
				FindNextCommand(viewers::TextViewer& view, bool replace, Direction direction) throw() :
					EditorCommand(view), replace_(replace), direction_(direction) {}
				ulong execute();
			private:
				bool replace_;
				Direction direction_;
			};
			/// Begins the incremental search.
			class IncrementalSearchCommand : public EditorCommand {
			public:
				IncrementalSearchCommand(viewers::TextViewer& view,
					Direction direction, searcher::IIncrementalSearchCallback* callback = 0) throw()
					: EditorCommand(view), direction_(direction), callback_(callback) {}
				ulong execute();
			private:
				Direction direction_;
				searcher::IIncrementalSearchCallback* callback_;
			};
			/// Makes/Deletes indents.
			class IndentationCommand : public EditorCommand {
			public:
				IndentationCommand(viewers::TextViewer& view, bool indent, bool tabIndent, ushort level) throw()
					: EditorCommand(view), indent_(indent), tabIndent_(tabIndent), level_(level) {}
				ulong execute();
			private:
				bool indent_;
				bool tabIndent_;
				ushort level_;
			};
			/// Toggles the input status.
			class InputStatusToggleCommand : public EditorCommand {
			public:
				enum Type {
					IME_STATUS,		///< About IME.
					OVERTYPE_MODE,	///< About insertion/overtype mode.
					SOFT_KEYBOARD	///< About soft keyboard.
				};
				InputStatusToggleCommand(viewers::TextViewer& view, Type type) throw() : EditorCommand(view), type_(type) {}
				ulong execute();
			private:
				Type type_;
			};
			/// Inserts a newline.
			class NewlineCommand : public internal::EditorCommandBase<bool> {
			public:
				NewlineCommand(viewers::TextViewer& view, bool previousLine) throw() : internal::EditorCommandBase<bool>(view, previousLine) {}
				ulong execute();
			};			/// Reconverts by using IME.
			class ReconversionCommand : public EditorCommand {
			public:
				explicit ReconversionCommand(viewers::TextViewer& view) throw() : EditorCommand(view) {}
				ulong execute();
			};
			/// Extends the selection and begins rectangular selection.
			class RowSelectionExtensionCommand : public EditorCommand {
			public:
				enum Type {
					NEXT_CHARACTER,			///< Extends to the next character.
					PREVIOUS_CHARACTER,		///< Extends to the previous character.
					LEFT_CHARACTER,			///< Extends to the left character.
					RIGHT_CHARACTER,		///< Extends to the right character.
					NEXT_WORD,				///< Extends to the start of the next word.
					PREVIOUS_WORD,			///< Extends to the start of the previous word.
					LEFT_WORD,				///< Extends to the start of the left word.
					RIGHT_WORD,				///< Extends to the start of the right word.
					NEXT_WORDEND,			///< Extends to the end of the next word.
					PREVIOUS_WORDEND,		///< Extends to the end of the previous word.
					LEFT_WORDEND,			///< Extends to the end of the left word.
					RIGHT_WORDEND,			///< Extends to the end of the right word.
					NEXT_LINE,				///< Extends to the next logical line.
					PREVIOUS_LINE,			///< Extends to the previous logical line.
					VISUAL_NEXT_LINE,		///< Extends to the next visual line.
					VISUAL_PREVIOUS_LINE,	///< Extends to the previous visual line.
					START_OF_LINE,			///< Extends to the start of the line.
					END_OF_LINE,			///< Extends to the end of the line.
					FIRST_CHAR_OF_LINE,		///< Extends to the first character in the line.
					LAST_CHAR_OF_LINE,		///< Extends to the last character in the line.
					START_OR_FIRST_OF_LINE,	///< Extends to the start of the line or first character in the line by context.
					END_OR_LAST_OF_LINE,	///< Extends to the end of the line or last character in the line by context.
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
			class UndoCommand : public internal::EditorCommandBase<bool> {
			public:
				UndoCommand(viewers::TextViewer& view, bool undo) throw() : internal::EditorCommandBase<bool>(view, undo) {}
				ulong execute();
			};
		} // namespace commands

		/// Standard input sequence checkers.
		namespace isc {
			/// I.S.C. for Ainu.
			class AinuInputSequenceChecker : virtual public InputSequenceChecker {
			public:
				bool	check(HKL keyboardLayout, const Char* first, const Char* last, CodePoint cp) const;
			};
			/// I.S.C. for Thai.
			class ThaiInputSequenceChecker : virtual public InputSequenceChecker, private manah::Unassignable {
			public:
				enum Mode {PASS_THROUGH, BASIC_MODE, STRICT_MODE};
				ThaiInputSequenceChecker(Mode mode = BASIC_MODE) throw() : mode_(mode) {}
				bool	check(HKL keyboardLayout, const Char* first, const Char* last, CodePoint cp) const;
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
				bool	check(HKL keyboardLayout, const Char* first, const Char* last, CodePoint cp) const;
			};
		} // namespace isc

	} // namespace texteditor

} // namespace ascension

#endif /* !ASCENSION_TEXT_EDITOR_HPP */
