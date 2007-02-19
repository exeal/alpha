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
			/// Constructor
			EditorCommand(viewers::TextViewer& view) throw() : view_(&view) {}
			/// Destructor
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

		namespace internal {
			template<class T> class EditorCommandBase : public EditorCommand {
			public:
				EditorCommandBase(viewers::TextViewer& view, T param) : EditorCommand(view), param_(param) {}
				virtual ~EditorCommandBase() {}
			protected:
				T param_;
			};
		} // namespace internal

		/// Implementations of the standard commands.
		namespace commands {
			/// ブックマーク操作
			class BookmarkCommand : public EditorCommand {
			public:
				enum Type {
					CLEAR_ALL,			///< 全て削除
					TOGGLE_CURRENT_LINE	///< 現在行のブックマークのトグル
				};
				BookmarkCommand(viewers::TextViewer& view, Type type) throw() : EditorCommand(view), type_(type) {}
				ulong execute();
			private:
				Type type_;
			};
			/// 選択の解除、インクリメンタル検索の中止
			class CancelCommand : public EditorCommand {
			public:
				explicit CancelCommand(viewers::TextViewer& view) throw() : EditorCommand(view) {}
				ulong execute();
			};
			/// キャレットの移動
			class CaretMovementCommand : public EditorCommand {
			public:
				enum Type {
					NEXT_CHARACTER,		///< 次の文字
					PREVIOUS_CHARACTER,	///< 前の文字
					LEFT_CHARACTER,		///< 左の文字
					RIGHT_CHARACTER,	///< 右の文字
					NEXT_WORD,		///< 次の単語の先頭
					PREVIOUS_WORD,	///< 前の単語の先頭
					LEFT_WORD,		///< 左の単語の先頭
					RIGHT_WORD,		///< 右の単語の先頭
					NEXT_WORDEND,		///< 次の単語の終端
					PREVIOUS_WORDEND,	///< 前の単語の終端
					LEFT_WORDEND,		///< 左の単語の終端
					RIGHT_WORDEND,		///< 右の単語の終端
					NEXT_LINE,		///< 次の行
					PREVIOUS_LINE,	///< 前の行
					VISUAL_NEXT_LINE,		///< 次の表示行
					VISUAL_PREVIOUS_LINE,	///< 前の表示行
					NEXT_PAGE,		///< 次のページ
					PREVIOUS_PAGE,	///< 前のページ
					START_OF_LINE,			///< 行頭
					END_OF_LINE,			///< 行末
					FIRST_CHAR_OF_LINE,		///< 行の最初の文字
					LAST_CHAR_OF_LINE,		///< 行の最後の文字
					START_OR_FIRST_OF_LINE,	///< 現在位置により、行頭か行の最初の文字
					END_OR_LAST_OF_LINE,	///< 現在位置により、行末か行の最後の文字
					START_OF_DOCUMENT,	///< ドキュメントの先頭
					END_OF_DOCUMENT,	///< ドキュメントの終端
					NEXT_BOOKMARK,		///< 次のブックマーク (移動量は常に1)
					PREVIOUS_BOOKMARK,	///< 前のブックマーク (移動量は常に1)
					MATCH_BRACKET,		///< 対括弧
				};
				CaretMovementCommand(viewers::TextViewer& view, Type type, bool extend = false, length_t offset = 1) throw()
						: EditorCommand(view), type_(type), extend_(extend), offset_(offset) {}
				ulong execute();
			private:
				Type		type_;
				bool		extend_;
				length_t	offset_;
			};
			/// 文字とコードポイントの変換
			class CharacterCodePointConversionCommand : public internal::EditorCommandBase<bool> {
			public:
				CharacterCodePointConversionCommand(viewers::TextViewer& view, bool charToCp) throw() :
					internal::EditorCommandBase<bool>(view, charToCp) {}
				ulong execute();
			};
			/// 1文字の入力
			class CharacterInputCommand : public internal::EditorCommandBase<CodePoint> {
			public:
				CharacterInputCommand(viewers::TextViewer& view, CodePoint cp) throw() : internal::EditorCommandBase<CodePoint>(view, cp) {}
				ulong execute();
			};
			/// 隣接行の同じ位置の文字を入力
			class CharacterInputFromNextLineCommand : internal::EditorCommandBase<bool> {
			public:
				CharacterInputFromNextLineCommand(viewers::TextViewer& view, bool fromNextLine) throw() :
					internal::EditorCommandBase<bool>(view, fromNextLine) {}
				ulong execute();
			};
			/// クリップボード関連のコマンド
			class ClipboardCommand : public EditorCommand {
			public:
				enum Type {
					COPY,	///< コピー
					CUT,	///< 切り取り
					PASTE	///< 貼り付け
				};
				ClipboardCommand(viewers::TextViewer& view, Type type, bool performClipboardRing) throw()
					: EditorCommand(view), type_(type), performClipboardRing_(performClipboardRing) {}
				ulong execute();
			private:
				Type type_;
				bool performClipboardRing_;
			};
			/// テキストの削除
			class DeletionCommand : public EditorCommand {
			public:
				enum Type {
					NEXT_CHARACTER,		///< 次の1文字。インクリメンタル検索中の場合、検索開始時の状態にリセット
					PREVIOUS_CHARACTER,	///< 前の1文字。インクリメンタル検索中の場合、最後の変更を元に戻す
					NEXT_WORD,			///< 次の単語の先頭まで
					PREVIOUS_WORD,		///< 前の単語の先頭まで
					WHOLE_LINE			///< 行全体
				};
				DeletionCommand(viewers::TextViewer& view, Type type) throw() : EditorCommand(view), type_(type) {}
				ulong execute();
			private:
				Type type_;
			};
			/// 全検索
			class FindAllCommand : public EditorCommand {
			public:
				enum Type {
					BOOKMARK,	///< ブックマークを設定
					REPLACE		///< 置換
				};
				FindAllCommand(viewers::TextViewer& view, Type type, bool onlySelection) throw()
					: EditorCommand(view), type_(type), onlySelection_(onlySelection) {}
				ulong execute();
			private:
				Type type_;
				bool onlySelection_;
			};
			/// 次を検索
			class FindNextCommand : public EditorCommand {
			public:
				FindNextCommand(viewers::TextViewer& view, bool replace, Direction direction) throw() :
					EditorCommand(view), replace_(replace), direction_(direction) {}
				ulong execute();
			private:
				bool replace_;
				Direction direction_;
			};
			/// インクリメンタル検索
			class IncrementalSearchCommand : public EditorCommand {
			public:
				IncrementalSearchCommand(viewers::TextViewer& view, searcher::SearchType type,
					Direction direction, searcher::IIncrementalSearchListener* listener = 0) throw()
					: EditorCommand(view), type_(type), direction_(direction), listener_(listener) {}
				ulong execute();
			private:
				searcher::SearchType type_;
				Direction direction_;
				searcher::IIncrementalSearchListener* listener_;
			};
			/// インデント
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
			/// 入力状態のトグル
			class InputStatusToggleCommand : public EditorCommand {
			public:
				enum Type {
					IME_STATUS,		///< IME
					OVERTYPE_MODE,	///< 挿入/上書きモード
					SOFT_KEYBOARD	///< ソフトキーボード
				};
				InputStatusToggleCommand(viewers::TextViewer& view, Type type) throw() : EditorCommand(view), type_(type) {}
				ulong execute();
			private:
				Type type_;
			};
			/// 改行
			class LineBreakCommand : public internal::EditorCommandBase<bool> {
			public:
				LineBreakCommand(viewers::TextViewer& view, bool previousLine) throw() : internal::EditorCommandBase<bool>(view, previousLine) {}
				ulong execute();
			};
			/// 補完ウィンドウを開く
			class OpenCompletionWindowCommand : public EditorCommand {
			public:
				explicit OpenCompletionWindowCommand(viewers::TextViewer& view) throw() : EditorCommand(view) {}
				ulong execute();
			};
			/// 再変換
			class ReconversionCommand : public EditorCommand {
			public:
				explicit ReconversionCommand(viewers::TextViewer& view) throw() : EditorCommand(view) {}
				ulong execute();
			};
			/// 選択を拡張し、矩形選択を開始
			class RowSelectionExtensionCommand : public EditorCommand {
			public:
				enum Type {
					NEXT_CHARACTER,		///< 次の文字
					PREVIOUS_CHARACTER,	///< 前の文字
					LEFT_CHARACTER,		///< 左の文字
					RIGHT_CHARACTER,	///< 右の文字
					NEXT_WORD,		///< 次の単語の先頭
					PREVIOUS_WORD,	///< 前の単語の先頭
					LEFT_WORD,		///< 左の単語の先頭
					RIGHT_WORD,		///< 右の単語の先頭
					NEXT_WORDEND,		///< 次の単語の終端
					PREVIOUS_WORDEND,	///< 前の単語の終端
					LEFT_WORDEND,		///< 左の単語の終端
					RIGHT_WORDEND,		///< 右の単語の終端
					NEXT_LINE,		///< 次の行
					PREVIOUS_LINE,	///< 前の行
					START_OF_LINE,			///< 行頭
					END_OF_LINE,			///< 行末
					FIRST_CHAR_OF_LINE,		///< 行の最初の文字
					LAST_CHAR_OF_LINE,		///< 行の最後の文字
					START_OR_FIRST_OF_LINE,	///< 現在位置により、行頭か行の最初の文字
					END_OR_LAST_OF_LINE,	///< 現在位置により、行末か行の最後の文字
				};
				RowSelectionExtensionCommand(viewers::TextViewer& view, Type type) throw() : EditorCommand(view), type_(type) {}
				ulong execute();
			private:
				Type type_;
			};
			/// 選択の作成
			class SelectionCreationCommand : public EditorCommand {
			public:
				enum Type {
					ALL,			///< 全テキスト
					CURRENT_WORD	///< 現在の単語
				};
				SelectionCreationCommand(viewers::TextViewer& view, Type type) throw() : EditorCommand(view), type_(type) {}
				ulong execute();
			private:
				Type type_;
			};
			/// タブと空白の変換
			class TabifyCommand : public internal::EditorCommandBase<bool> {
			public:
				TabifyCommand(viewers::TextViewer& view, bool tabify) throw() : internal::EditorCommandBase<bool>(view, tabify) {}
				ulong execute();
			};
			/// テキストの入力
			class TextInputCommand : public internal::EditorCommandBase<String> {
			public:
				TextInputCommand(viewers::TextViewer& view, const String& text) throw() : internal::EditorCommandBase<String>(view, text) {}
				ulong execute();
			};
			/// テキストの入れ替え
			class TranspositionCommand : public EditorCommand {
			public:
				enum Type {
					CHARACTERS,	///< 文字
					LINES,		///< 行
					WORDS,		///< 単語
	//				SENTENCES,	///< 文
	//				PARAGRAPHS	///< 段落
				};
				TranspositionCommand(viewers::TextViewer& view, Type type) throw() : EditorCommand(view), type_(type) {}
				ulong execute();
			private:
				Type type_;
			};
			/// 元に戻す/やり直し
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
			class ThaiInputSequenceChecker : virtual public InputSequenceChecker {
			public:
				enum Mode {PASS_THROUGH, BASIC_MODE, STRICT_MODE};
				ThaiInputSequenceChecker(Mode mode = BASIC_MODE) throw() : mode_(mode) {}
				bool	check(HKL keyboardLayout, const Char* first, const Char* last, CodePoint cp) const;
			private:
				enum CharacterClass {
					CTRL, NON, CONS,	// タイ文字ブロックの未定義文字は制御文字として扱う
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
