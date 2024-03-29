;/*
; This file defines message strings.
; Translators are the following:
; - exeal (exeal@users.sourceforge.jp) -- Japanese
;*/

FacilityNames=(
  Command=0x1
)
LanguageNames=(English=0x0409:MSG00409)
LanguageNames=(Japanese=0x0411:MSG00411)


;// general messages

MessageID=
SymbolicName=MSG_BUFFER__BUFFER_IS_DIRTY
Language=English
%1%n%nThe file is modified. Do you want to save?%0%0
.
Language=Japanese
%1%n%nこのファイルは変更されています。保存しますか?%0%0
.

MessageID=
SymbolicName=MSG_BUFFER__CONFIRM_REOPEN
Language=English
Do you want to reopen the file?%0%0
.
Language=Japanese
現在のファイルを開き直しますか?%0%0
.

MessageID=
SymbolicName=MSG_BUFFER__CONFIRM_REOPEN_EVEN_IF_DIRTY
Language=English
The file is modified.%nIf you reopen, the modification will be lost. Do you want to do?%0%0
.
Language=Japanese
現在のファイルは変更されています。%nファイルを開き直すと変更が失われますが、よろしいですか?%0%0
.

MessageID=
SymbolicName=MSG_BUFFER__FAILED_TO_SEND_TO_MAILER
Language=English
Failed to send to a mailer.%0%0
.
Language=Japanese
メーラへの送信に失敗しました。%0%0
.

MessageID=
SymbolicName=MSG_BUFFER__UNTITLED
Language=English
untitled%0%0
.
Language=Japanese
無題%0%0
.

MessageID=
SymbolicName=MSG_BUFFER__FILE_IS_MODIFIED_AND_REOPEN
Language=English
%1%n%nThe file was modified by other process. Do you want to reload?%nIf you reload, current editing contents will be disposed.%0%0
.
Language=Japanese
%1%n%nこのファイルは他のプロセスで変更されています。ファイルを読み込み直しますか?%n読み込み直すと現在の編集内容は破棄されます。%0%0
.

MessageID=
SymbolicName=MSG_BUFFER__FILE_IS_MODIFIED_AND_SAVE
Language=English
%1%n%nThe file was modified by other process. Do you want to continue to save?%0%0
.
Language=Japanese
%1%n%nこのファイルは他のプロセスで変更されています。ファイルの保存を続行しますか?%0%0
.

MessageID=
SymbolicName=MSG_BUFFER__FILE_IS_MODIFIED_AND_EDIT
Language=English
%1%n%nThe file was modified by other process. Do you want to continue to edit?%0%0
.
Language=Japanese
%1%n%nこのファイルは他のプロセスで変更されています。編集を続行しますか?%0%0
.

MessageID=
SymbolicName=MSG_BUFFER__SAVING_FILE_IS_OPENED
Language=English
%1%n%nThe file has been already opened. Cannot write to an editing file.%0%0
.
Language=Japanese
%1%n%nこのファイルは既に開いています。編集中のファイルに上書きすることはできません。%0%0
.

MessageID=
SymbolicName=MSG_SEARCH__PATTERN_NOT_FOUND
Language=English
Specified string was not found.%0%0
.
Language=Japanese
指定された文字列は見つかりませんでした。%0%0
.

MessageID=
SymbolicName=MSG_SEARCH__REPLACE_DONE
Language=English
Replaced %1 string(s).%0%0
.
Language=Japanese
%1 個の文字列を置換しました。%0%0
.

MessageID=
SymbolicName=MSG_SEARCH_REGEX_IS_INAVAILABLE
Language=English
Failed to load the regular expression engine. Cannot search.%0%0
.
Language=Japanese
正規表現エンジンのロードに失敗しました。検索を行うことはできません。%0%0
.

MessageID=
SymbolicName=MSG_SEARCH__INVALID_REGEX_PATTERN
Language=English
An error occured during regular expression search.%n%nReason:%t%1%nPosition:%t%2%0%0
.
Language=Japanese
正規表現検索中にエラーが発生しました。%n%n理由:%t%1%n位置:%t%2%0%0
.

MessageID=
SymbolicName=MSG_SEARCH__BAD_PATTERN_START
Language=English
Not an error%0%0
.
Language=Japanese
エラーではありません%0%0
.

MessageID=
Language=English
An invalid collating element was specified in a [[.name.]] block%0%0
.
Language=Japanese
不正な照合要素です%0%0
.

MessageID=
Language=English
An invalid character class name was specified in a [[:name:]] block%0%0
.
Language=Japanese
不正な文字セットです%0%0
.

MessageID=
Language=English
An invalid or trailing escape was encountered%0%0
.
Language=Japanese
不正なエスケープです%0%0
.

MessageID=
Language=English
A back-reference to a non-existant marked sub-expression was encountered%0%0
.
Language=Japanese
存在しない部分式への後方参照が見つかりました%0%0
.

MessageID=
Language=English
An invalid character set was encountered%0%0
.
Language=Japanese
不正な文字クラスです%0%0
.

MessageID=
Language=English
Mismatched '(' and ')'%0%0
.
Language=Japanese
対応する丸括弧 (')') が見つかりません%0%0
.

MessageID=
Language=English
Mismatched '{' and '}'%0%0
.
Language=Japanese
対応する中括弧 ('}') が見つかりません%0%0
.

MessageID=
Language=English
Invalid contents of a {...} block%0%0
.
Language=Japanese
中括弧の内容 ("{...}") が不正です%0%0
.

MessageID=
Language=English
A character range was invalid%0%0
.
Language=Japanese
不正な文字範囲です%0%0
.

MessageID=
Language=English
Out of memory%0%0
.
Language=Japanese
メモリ不足のため、検索を行うことができません%0%0
.

MessageID=
Language=English
An attempt to repeat something that can not be repeated%0%0
.
Language=Japanese
不正な繰り返しです%0%0
.

MessageID=
Language=English
The expression bacame too complex to handle%0%0
.
Language=Japanese
パターンが複雑なため、検索を行うことができません%0%0
.

MessageID=
Language=English
Out of program stack space%0%0
.
Language=Japanese
スタック空間が枯渇しています。検索を行うことはできません%0%0
.

MessageID=
Language=English
Unknown error%0%0
.
Language=Japanese
不明%0%0
.

MessageID=
SymbolicName=MSG_ERROR__EXE_NOT_REGISTERED
Language=English
"%1"%nNo program assigned to the document type.%0%0
.
Language=Japanese
"%1"%nこの文書タイプには実行に使用するプログラムが設定されていません。%0%0
.

MessageID=
SymbolicName=MSG_ERROR__FAILED_TO_RUN_EXE
Language=English
Failed to run the program.%nThe path assigned to the document type may be incorrect.%0%0
.
Language=Japanese
プログラムの実行に失敗しました。%n文書タイプに設定されているプログラムのパスが正しくない可能性があります。%0%0
.

MessageID=
SymbolicName=MSG_ERROR__UNSUPPORTED_OS_VERSION
Language=English
Alpha does not support your platform.%0%0
.
Language=Japanese
ご使用のバージョンの OS では Alpha を実行することはできません。%0%0
.

MessageID=
SymbolicName=MSG_ERROR__DUPLICATE_ABBREV
Language=English
There is already same abbreviation.%0%0
.
Language=Japanese
既に同じ短縮語句が存在します。%0
.

MessageID=
SymbolicName=MSG_ERROR__REGEX_UNKNOWN_ERROR
Language=English
Error occured during regular expression pattern matching.%nThe pattern may be too complex.%0
.
Language=Japanese
正規表現パターンマッチ中にエラーが発生しました。%nパターンが複雑すぎる可能性があります。%0
.

MessageID=
SymbolicName=MSG_ERROR__MIGEMO_UNKNOWN_ERROR
Language=English
Error occured in Migemo.%0
.
Language=Japanese
Migemo の内部でエラーが発生しました。%0
.

MessageID=
SymbolicName=MSG_ERROR__PROHIBITED_FOR_MACRO_DEFINING
Language=English
This command cannot execute in keyboard macro definition.%0
.
Language=Japanese
この操作はキーボードマクロ記録中には実行できません。%0
.

MessageID=
SymbolicName=MSG_ERROR__FAILED_TO_LOAD_TEMP_MACRO
Language=English
Failed to load the keyboard macro file.%n%nFile: %1%nPosition: (%2!lu!, %3!lu!)%nReason: %4%0
.
Language=Japanese
キーボードマクロファイルの読み込みに失敗しました。%n%nファイル: %1%n位置: (%2!lu!, %3!lu!)%n理由: %4%0
.

MessageID=
SymbolicName=MSG_ERROR__FAILED_TO_LOAD_SOMETHING
Language=English
(Failed to load)%0
.
Language=Japanese
(読み込みに失敗しました)%0
.

MessageID=
SymbolicName=MSG_ERROR__OUT_OF_MEMORY
Language=English
Process failed for out of memory.%0
.
Language=Japanese
メモリ不足のため、処理に失敗しました。%0
.

MessageID=
SymbolicName=MSG_STATUS__READ_ONLY_CAPTION
Language=English
[readonly]%0
.
Language=Japanese
[読み取り専用]%0
.

MessageID=
SymbolicName=MSG_STATUS__TEMP_MACRO_DEFINING
Language=English
Defining keyboard macro%0
.
Language=Japanese
キーボードマクロ記録中%0
.

MessageID=
SymbolicName=MSG_STATUS__TEMP_MACRO_PAUSING
Language=English
Pausing keyboard macro definition%0
.
Language=Japanese
キーボードマクロ記録中断中%0
.

MessageID=
SymbolicName=MSG_STATUS__NARROWING
Language=English
Narrowing%0
.
Language=Japanese
ナローイング%0
.

MessageID=
SymbolicName=MSG_STATUS__DEBUGGING
Language=English
Debuggnig%0
.
Language=Japanese
デバッグ中%0
.

MessageID=
SymbolicName=MSG_STATUS__INSERT_MODE
Language=English
Insert%0
.
Language=Japanese
挿入%0
.

MessageID=
SymbolicName=MSG_STATUS__OVERTYPE_MODE
Language=English
Overtype%0
.
Language=Japanese
上書き%0
.

MessageID=
SymbolicName=MSG_STATUS__CARET_POSITION
Language=English
Line: %1!lu! , Column: %2!lu! , Char: %3!lu!%0
.
Language=Japanese
%1!lu! 行 、 %2!lu! 列 、 %3!lu! 文字%0
.

MessageID=
SymbolicName=MSG_STATUS__CAN_EXPAND_ABBREV
Language=English
'%1' can expand to '%2'.%0
.
Language=Japanese
'%1' は '%2' に展開可能です。%0
.

MessageID=
SymbolicName=MSG_STATUS__LOADING_FILE
Language=English
Loading '%1'...%0
.
Language=Japanese
'%1' を読み込み中...%0
.

MessageID=
SymbolicName=MSG_STATUS__WAITING_FOR_NEXT_KEY_STROKE
Language=English
'%1' is pressed. Waiting for the next key stroke.%0
.
Language=Japanese
'%1' が押されました。次のキーストロークを待っています。%0
.

MessageID=
SymbolicName=MSG_STATUS__INVALID_KEY_SEQUENCE
Language=English
There is no command bound to the key sequence '%1'.%0
.
Language=Japanese
キーシーケンス '%1' に割り当てられているコマンドはありません。%0
.

MessageID=
SymbolicName=MSG_STATUS__ISEARCH
Language=English
Incremental search : %1%0
.
Language=Japanese
インクリメンタル検索 : %1%0
.

MessageID=
SymbolicName=MSG_STATUS__ISEARCH_EMPTY_PATTERN
Language=English
Incremental search : (empty pattern)%0
.
Language=Japanese
インクリメンタル検索 : (検索文字列が入力されていません)%0
.

MessageID=
SymbolicName=MSG_STATUS__ISEARCH_BAD_PATTERN
Language=English
Incremental Search : %1 (invalid pattern)%0
.
Language=Japanese
インクリメンタル検索 : %1 (パターンが正しくありません)%0
.

MessageID=
SymbolicName=MSG_STATUS__ISEARCH_NOT_FOUND
Language=English
Incremental search : %1 (not found)%0
.
Language=Japanese
インクリメンタル検索 : %1 (見つかりません)%0
.

MessageID=
SymbolicName=MSG_STATUS__RISEARCH
Language=English
Reversal incremental search : %1%0
.
Language=Japanese
逆方向インクリメンタル検索 : %1%0
.

MessageID=
SymbolicName=MSG_STATUS__RISEARCH_NOT_FOUND
Language=English
Reversal incremental search : %1 (not found)%0
.
Language=Japanese
逆方向インクリメンタル検索 : %1 (見つかりません)%0
.

MessageID=
SymbolicName=MSG_STATUS__RISEARCH_BAD_PATTERN
Language=English
Reversal Incremental Search : %1 (invalid pattern)%0
.
Language=Japanese
逆方向インクリメンタル検索 : %1 (パターンが正しくありません)%0
.

MessageID=
SymbolicName=MSG_STATUS__RISEARCH_EMPTY_PATTERN
Language=English
Reversal incremental search : (empty pattern)%0
.
Language=Japanese
逆方向インクリメンタル検索 : (検索文字列が入力されていません)%0
.

MessageID=
SymbolicName=MSG_STATUS__MATCH_BRACKET_OUT_OF_VIEW
Language=English
(Match brace) %1 : %2 %0
.
Language=Japanese
(対括弧) %1 : %2 %0
.

MessageID=
SymbolicName=MSG_STATUS__INVOKABLE_LINK_POPUP
Language=English
%1%nCtrl plus click to open the link.%0
.
Language=Japanese
%1%nCtrl キーを押しながらクリックするとリンクを起動します。%0
.

MessageID=
SymbolicName=MSG_DIALOG__DEFAULT_OPENFILE_FILTER
Language=English
All Files:*.*%0
.
Language=Japanese
すべてのファイル:*.*%0
.

MessageID=
SymbolicName=MSG_DIALOG__WHOLE_GRAPHEME_MATCH
Language=English
Grapheme Cluster%0
.
Language=Japanese
書記素%0
.

MessageID=
SymbolicName=MSG_DIALOG__WHOLE_WORD_MATCH
Language=English
Word%0
.
Language=Japanese
単語%0
.

MessageID=
SymbolicName=MSG_DIALOG__LITERAL_SEARCH
Language=English
Literal%0
.
Language=Japanese
リテラル%0
.

MessageID=
SymbolicName=MSG_DIALOG__REGEX_SEARCH
Language=English
Regular expression (Boost.Regex 1.34.1)%0
.
Language=Japanese
正規表現 (Boost.Regex 1.34.1)%0
.

MessageID=
SymbolicName=MSG_DIALOG__MIGEMO_SEARCH
Language=English
Migemo (C/Migemo 1.2 expected)%0
.
Language=Japanese
Migemo (C/Migemo 1.2 expected)%0
.

MessageID=
SymbolicName=MSG_DIALOG__BUFFERBAR_CAPTION
Language=English
Buffers%0
.
Language=Japanese
バッファ%0
.

MessageID=
SymbolicName=MSG_DIALOG__ABBREVIATION
Language=English
Abbreviation%0
.
Language=Japanese
短縮語%0
.

MessageID=
SymbolicName=MSG_DIALOG__EXPANDED_ABBREVIATION
Language=English
Expanded String%0
.
Language=Japanese
展開後の文字列%0
.

MessageID=
SymbolicName=MSG_DIALOG__BOOKMARKED_LINE
Language=English
Line%0
.
Language=Japanese
行%0
.

MessageID=
SymbolicName=MSG_DIALOG__BOOKMARKED_POSITION
Language=English
Position%0
.
Language=Japanese
位置%0
.

MessageID=
SymbolicName=MSG_DIALOG__LINE_NUMBER_RANGE
Language=English
Line &Number (%1-%2):%0
.
Language=Japanese
行番号 (%1-%2)(&N):%0
.

MessageID=
SymbolicName=MSG_DIALOG__SELECT_ALL
Language=English
Select &All%0
.
Language=Japanese
すべて選択(&A)%0
.

MessageID=
SymbolicName=MSG_DIALOG__UNSELECT_ALL
Language=English
&Cancel All Selections%0
.
Language=Japanese
選択をすべて解除(&C)%0
.

MessageID=
SymbolicName=MSG_DIALOG__SELECT_DIRECTORY
Language=English
Select a directory.%0
.
Language=Japanese
ディレクトリを選択してください。%0
.

MessageID=
SymbolicName=MSG_DIALOG__FILE_OPERATION
Language=English
File Operation%0
.
Language=Japanese
ファイル操作%0
.

MessageID=
SymbolicName=MSG_DIALOG__RENAME_FILE
Language=English
Rename%0
.
Language=Japanese
名前の変更%0
.

MessageID=
SymbolicName=MSG_DIALOG__COPY_FILE
Language=English
Copy%0
.
Language=Japanese
コピー%0
.

MessageID=
SymbolicName=MSG_DIALOG__MOVE_FILE
Language=English
Move%0
.
Language=Japanese
移動%0
.

MessageID=
SymbolicName=MSG_DIALOG__KEEP_NEWLINE
Language=English
No Change%0
.
Language=Japanese
統一しない%0
.

MessageID=
SymbolicName=MSG_DIALOG__SAVE_FILE_FILTER
Language=English
All Files%0
.
Language=Japanese
すべてのファイル%0
.

MessageID=
SymbolicName=MSG_IO__FAILED_TO_DETECT_ENCODE
Language=English
%1%n%nFailed to detect encoding of the file. Used default encoding.%0
.
Language=Japanese
%1%n%nエンコードを判別できませんでした。既定のエンコードを使用します。%0
.

MessageID=
SymbolicName=MSG_IO__INVALID_ENCODING
Language=English
%1%n%nSpecified encoding is invalid. Cannot open the file.%0
.
Language=Japanese
%1%n%n指定されたエンコードは正しくありません。ファイルを開くことはできません。%0
.

MessageID=
SymbolicName=MSG_IO__INVALID_NEWLINE
Language=English
%1%n%nSpecified newline is invalid. Cannot open the file.%0
.
Language=Japanese
%1%n%n指定された改行コードは正しくありません。ファイルを開くことはできません。%0
.

MessageID=
SymbolicName=MSG_IO__UNCONVERTABLE_UCS_CHAR
Language=English
%1%nEncoding: %2%n%nThe document contains unconvertable characters under specified encoding.%nThese characters will be written incorrectly.%nDo you want to change encoding?%0
.
Language=Japanese
%1%nエンコード: %2%n
この文書は指定したエンコードで変換できない文字を含んでいます。%nこのまま続行するとそれらの文字は正しく保存されません。%nエンコードを変更しますか?%0
.

MessageID=
SymbolicName=MSG_IO__UNCONVERTABLE_NATIVE_CHAR
Language=English
%1%nEncoding: %2%n%nThe file contains unconvertable characters under specified encoding.%nSaving may destroy contents of the file.%nDo you want to change encoding?%0
.
Language=Japanese
%1%nエンコード: %2%n%nこのファイルは指定したエンコードで Unicode に変換できない文字を含んでいます。%n編集を続行して保存するとファイルの内容が破壊される可能性があります。%nエンコードを変更しますか?%0
.

MessageID=
SymbolicName=MSG_IO__MALFORMED_INPUT_FILE
Language=English
%1%nEncoding: %2%n%nThe file contains malformed byte sequence as the specified encoding. Try the other encoding.%0
.
Language=Japanese
%1%nエンコード: %2%n%nこのファイルは指定したエンコードとして不正なシーケンスを含んでいます。エンコードを変更してください。%0
.

MessageID=
SymbolicName=MSG_IO__HUGE_FILE
Language=English
%1%n%nThe file size is too large. Maximum file size Alpha can handle is 2GB.%0
.
Language=Japanese
%1%n%nファイルサイズが大きすぎます。Alpha が扱えるファイルサイズの上限値は 2GB です。%0
.

MessageID=
SymbolicName=MSG_IO__FAILED_TO_WRITE_FOR_READONLY
Language=English
%1%n%nCannot overwrite the file. The file is read-only.%0
.
Language=Japanese
%1%n%nファイルを上書き保存できません。このバッファは読み取り専用です。%0
.

MessageID=
SymbolicName=MSG_IO__FAILED_TO_RESOLVE_SHORTCUT
Language=English
%1%n%nFailed to resolve the shortcut. Cannot open the file.%0
.
Language=Japanese
%1%n%nショートを解決できませんでした。ファイルを開くことはできません。%0
.

MessageID=
SymbolicName=MSG_IO__CANNOT_OPEN_FILE
Language=English
%1%n%nCannot open the file.%0
.
Language=Japanese
%1%n%nファイルを開くことができません。%0
.

MessageID=
SymbolicName=MSG_IO__CANNOT_CREATE_TEMP_FILE
Language=English
%1%n%nFailed to save the file. Cannot create temporary file.%0
.
Language=Japanese
%1%n%nファイルの保存に失敗しました。一時ファイルを作成できませんでした。%0
.

MessageID=
SymbolicName=MSG_IO__LOST_DISK_FILE
Language=English
%1%n%nFailed to save the file. The file is lost.%0
.
Language=Japanese
%1%n%nファイルの保存に失敗しました。ファイルが失われました。%0
.

MessageID=
SymbolicName=MSG_IO__UNSUPPORTED_ENCODING
Language=English
The encoding is not supported.%0
.
Language=Japanese
指定したエンコードはサポートされていません。%0
.

MessageID=
SymbolicName=MSG_SCRIPT__ERROR_DIALOG
Language=English
Script error occured.%n%nScript:%t%1%nLine:%t%2%nCharacter:%t%3%nError:%t%4%nSCODE:%t%5%nSource:%t%6%0
.
Language=Japanese
スクリプトエラーが発生しました。%n%nスクリプト:%t%1%n行:%t%2%n文字:%t%3%nエラー:%t%4%nSCODE:%t%5%nエラー元:%t%6%0
.

MessageID=
SymbolicName=MSG_SCRIPT__UNSAFE_ACTIVEX_CAUTION
Language=English
%1%n%nThis script is about to create an unsafe object.%nDo you allow this?%n%nQueried object:%n  ProgID:%t%2%n  IID:%t%3%0
.
Language=Japanese
%1%n%nこのスクリプトは安全でない可能性のあるオブジェクトを作成しようとしています。%nオブジェクトの作成を許可しますか?%n%n要求されたオブジェクト:%n  ProgID:%t%2%n  IID:%t%3%0
.

MessageID=
SymbolicName=MSG_SCRIPT__INVALID_LANGUAGE_NAME
Language=English
The language "%1" bound to the macro is invalid or not installed. Cannot execute.%0
.
Language=Japanese
マクロに指定されている言語 "%1" は正しくないか、インストールされていません。マクロを実行することはできません。%0
.

MessageID=
SymbolicName=MSG_SCRIPT__FAILED_TO_OPEN_MACRO_SCRIPT
Language=English
%1%n%nFailed to load the macro file. Cannot execute.%0
.
Language=Japanese
%1%n%nマクロスクリプトが読み込めませんでした。マクロを実行することはできません。%0
.

MessageID=
SymbolicName=MSG_OTHER__EMPTY_MENU_CAPTION
Language=English
(nothing)%0
.
Language=Japanese
(なし)%0
.

MessageID=
SymbolicName=MSG_OTHER__UNKNOWN
Language=English
(unknown)%0
.
Language=Japanese
(不明)%0
.

MessageID=
SymbolicName=MSG_OTHER__NOT_OBTAINED
Language=English
(not obtained)%0
.
Language=Japanese
(取得できませんでした)%0
.

MessageID=
SymbolicName=MSG_OTHER__TEMPORARY_MACRO_QUERY
Language=English
Proceed the macro?%n%nNo: Skip this repetition and next%nCancel: Abort the macro%0
.
Language=Japanese
このマクロを続行しますか?%n%nいいえ: 現在の繰り返しをスキップし、次の繰り返しを開始します%nキャンセル: マクロの実行を中止します%0
.

MessageID=
SymbolicName=MSG_OTHER__NONE
Language=English
None%0
.
Language=Japanese
なし%0
.


;// command names and description.

MessageID=0
Facility=Command
SymbolicName=CMD_SPECIAL_START
Language=English
.
Language=Japanese
.

MessageID=
Facility=Command
SymbolicName=CMD_FILE_TOP
Language=English
&File%0
.
Language=Japanese
ファイル(&F)%0
.

MessageID=
Facility=Command
SymbolicName=CMD_FILE_NEW
Language=English
&New%nCreate a new document.%0
.
Language=Japanese
新規(&N)%n文書を新しく作成します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_FILE_NEWWITHFORMAT
Language=English
New wi&th Format...%nCreate a new document with specified encoding, line break, and document type.%0
.
Language=Japanese
書式を指定して新規(&T)...%nエンコード、改行コード、文書タイプを指定して文書を新しく作成します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_FILE_OPEN
Language=English
&Open...%nOpen an existing document.%0
.
Language=Japanese
開く(&O)...%n既存のファイルを開きます。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_FILE_CLOSE
Language=English
&Close%nClose the document.%0
.
Language=Japanese
閉じる(&C)%n現在の文書を閉じます。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_FILE_CLOSEALL
Language=English
Cl&ose All%nClose all documents.%0
.
Language=Japanese
すべて閉じる(&Q)%nすべての文書を閉じます。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_FILE_SAVE
Language=English
&Save%nSave the document.%0
.
Language=Japanese
上書き保存(&S)%n現在の文書を保存します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_FILE_SAVEAS
Language=English
Save &As...%nSave the document under a different name.%0
.
Language=Japanese
名前を付けて保存(&A)...%n現在の文書を新しい名前で保存します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_FILE_SAVEALL
Language=English
Sav&e All%nSave all documents.%0
.
Language=Japanese
すべて保存(&E)%nすべての文書を保存します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_FILE_MRU
Language=English
Most Recent Used &Files%0
.
Language=Japanese
最近使ったファイル(&F)%0
.

MessageID=
Facility=Command
SymbolicName=CMD_FILE_OPERATE
Language=English
Op&erate%0
.
Language=Japanese
ファイル操作(&E)%0
.

MessageID=
Facility=Command
SymbolicName=CMD_FILE_REOPEN
Language=English
&Reopen%nReopen the document.%0
.
Language=Japanese
開き直す(&R)%n現在の文書を開き直します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_FILE_REOPENWITHCODEPAGE
Language=English
Reopen with Different Encodin&g...%nReopen the document with a different encoding.%0
.
Language=Japanese
エンコードを変更して開き直す(&G)%n現在の文書をエンコードを指定して開き直します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_FILE_EXIT
Language=English
E&xit Alpha%nClose all documents and exit.%0
.
Language=Japanese
Alpha の終了(&X)%n文書の保存を確認し、Alpha を終了します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_FILE_SENDMAIL
Language=English
Sen&d...%nSend the file.%0
.
Language=Japanese
送信(&D)...%n電子メールでこの文書を送信します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_FILE_CLOSEOTHERS
Language=English
Close Ot&hers%nClose all inactive documents.%0
.
Language=Japanese
他を閉じる(&H)%n現在の文書以外をすべて閉じます。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_FILE_PRINT
Language=English
&Print...%nPrint the document.%0
.
Language=Japanese
印刷(&P)...%n現在の文書を印刷します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_FILE_PRINTSETUP
Language=English
Set&up Page...%nSetup page layout.%0
.
Language=Japanese
ページ設定(&U)...%nページ レイアウトの設定を変更します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_FILE_PRINTPREVIEW
Language=English
Pre&view Page...%nPreview the entire page.%0
.
Language=Japanese
印刷プレビュー(&V)%nページ全体を表示します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_TOP
Language=English
&Edit%0
.
Language=Japanese
編集(&E)%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_ADVANCED
Language=English
&Advanced%0
.
Language=Japanese
高度な操作(&V)%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_INSERTUNICODECTRLS
Language=English
&Insert Unicode Control Characters%0
.
Language=Japanese
Unicode 制御文字の挿入(&I)%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_INSERTUNICODEWSS
Language=English
Insert Unicode &Whitespace Characters%0
.
Language=Japanese
Unicode 空白文字の挿入(&W)%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_DELETE
Language=English
&Delete%nDelete the selection.%0
.
Language=Japanese
削除(&D)%n選択範囲を削除します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_BACKSPACE
Language=English
Backspace%nDelete backward one character.%0
.
Language=Japanese
前の 1 文字を削除%n直前の 1 文字を削除します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_DELETETONEXTWORD
Language=English
Delete Next Word%nDelete to next word.%0
.
Language=Japanese
次の単語まで削除%n次の単語の先頭までを削除します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_DELETETOPREVWORD
Language=English
Delete Previous Word%nDelete to previous word.%0
.
Language=Japanese
前の単語まで削除%n前の単語の先頭までを削除します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_DELETELINE
Language=English
Delete Line%nDelete current line.%0
.
Language=Japanese
現在行を削除%n現在行を削除します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_INSERTPREVLINE
Language=English
Insert Previous%nInsert new line previous.%0
.
Language=Japanese
上に 1 行挿入%n1 つ上の行に改行を挿入します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_BREAK
Language=English
Break%nBreak the line.%0
.
Language=Japanese
改行%n改行文字を挿入します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_UNDO
Language=English
&Undo%nUndo editing.%0
.
Language=Japanese
元に戻す(&U)%n直前に行った動作を取り消します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_REDO
Language=English
&Redo%nRedo previously undone editing.%0
.
Language=Japanese
やり直し(&R)%n取り消された動作をやり直します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_CUT
Language=English
Cu&t%nCut the selection and put it on the Clipboard.%0
.
Language=Japanese
切り取り(&T)%n選択範囲を切り取ってクリップボードに移動します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_COPY
Language=English
&Copy%nCopy the selection and put it on the Clipboard.%0
.
Language=Japanese
コピー(&C)%n選択範囲をクリップボードにコピーします。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_PASTE
Language=English
&Paste%nInsert Clipboard contents.%0
.
Language=Japanese
貼り付け(&P)%nクリップボードの内容を挿入します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_INSERTTAB
Language=English
Insert Tab%nInsert a tab.%0
.
Language=Japanese
タブを挿入%nタブを挿入します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_DELETETAB
Language=English
Delete Tab%nDelete a tab.%0
.
Language=Japanese
タブを削除%nタブを削除します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_TABIFY
Language=English
Tabify%nConvert whitespaces in the selection to tabs.%0
.
Language=Japanese
空白をタブに変換%n選択範囲の空白類文字をタブに変換します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_UNTABIFY
Language=English
Untabify%nConvert tabs in the selection to spaces.%0
.
Language=Japanese
タブを空白に変換%n選択範囲のタブを半角空白に変換します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_PASTEFROMCLIPBOARDRING
Language=English
Paste From C&lipboard Ring%nInsert Clipboard Ring contents.%0
.
Language=Japanese
クリップボードリングから貼り付け(&L)%nクリップボードリングの内容を貼り付けます。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_CHARFROMABOVELINE
Language=English
Input Above Character%nInput the character above caret.%0
.
Language=Japanese
1 行上の同じ位置の文字を挿入%n現在行の 1 行上の同じ位置にある文字を挿入します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_CHARFROMBELOWLINE
Language=English
Input Below Character%nInput the character below caret.%0
.
Language=Japanese
1 行下の同じ位置の文字を挿入%n現在行の 1 行下の同じ位置にある文字を挿入します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_TRANSPOSELINES
Language=English
Transpose Lines%nTranspose the current line and the previous line.%0
.
Language=Japanese
行の入れ替え%n前後の行を入れ替えます。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_TRANSPOSECHARS
Language=English
Transpose Characters%nTranspose the character before caret with the character after caret.%0
.
Language=Japanese
文字の入れ替え%n前後の文字を入れ替えます。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_TRANSPOSEWORDS
Language=English
Transpose Words%nTranspose the word before caret with the word after caret.%0
.
Language=Japanese
単語の入れ替え%n前後の単語を入れ替えます。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_SHOWABBREVIATIONDLG
Language=English
Manage A&bbreviations...%nManage all abbreviations.%0
.
Language=Japanese
短縮語句の管理(&B)...%n短縮語句の管理を行います。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_FIRSTCHAR
Language=English
First Character%nGo to the first non-whitespace character in the line.%0
.
Language=Japanese
最初の非空白類文字に移動%n現在行で最初に現れる非空白類文字に移動します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_LASTCHAR
Language=English
Last Character%nGo to the last non-whitespace character in the line.%0
.
Language=Japanese
最後の非空白類文字に移動%n現在行で最初に現れる非空白類文字に移動します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_FIRSTCHAREXTEND
Language=English
Extend to First Character%nExtend the selection to the first non-whitespace character in the line.%0
.
Language=Japanese
最初の非空白類文字まで選択を拡張%n現在行で最初に現れる非空白類文字まで選択を拡張します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_LASTCHAREXTEND
Language=English
Extend to Last Character%nExtend the selection to the last non-whitespace character in the line.%0
.
Language=Japanese
最後の非空白類文字まで選択を拡張%n現在行で最後に現れる非空白類文字まで選択を拡張します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_FIRSTCHARORLINEHOME
Language=English
First Character or Start of Line%nGo to the first non-whitespace character in the line or the start of the line.%0
.
Language=Japanese
行頭か最初の非空白類文字に移動%n行頭か現在行で最初に現れる非空白類文字に移動します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_LASTCHARORLINEEND
Language=English
Last Character or End of Line%nGo to the last non-whitespace character in the line or the end of the line.%0
.
Language=Japanese
行末か最後の非空白類文字に移動%n行末か現在行で最後に現れる非空白類文字に移動します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_FIRSTCHARORLINEHOMEEXTEND
Language=English
Extend to First Character or Start of Line%nExtend the selection to the first non-whitespace character in the line or the start of the line.%0
.
Language=Japanese
行頭か最初の非空白類文字まで選択を拡張%n行頭か現在行で最初に現れる非空白類文字まで選択を拡張します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_LASTCHARORLINEENDEXTEND
Language=English
Extend to Last Character or End of Line%nExtend the selection to the last non-whitespace character in the line or the end of the line.%0
.
Language=Japanese
行末か最後の非空白類文字まで選択を拡張%n行末か現在行で最後に現れる非空白類文字まで選択を拡張します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_CHARTOCODEPOINT
Language=English
Character to Code Point%nConvert a character to a corresponding code point.%0
.
Language=Japanese
文字をコードポイントに変換%nキャレット直前の文字をコードポイントに変換します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_CODEPOINTTOCHAR
Language=English
Code Point to Character%nConvert a code point to a corresponding character.%0
.
Language=Japanese
コードポイントを文字に変換%nキャレット直前のコードポイントを文字に変換します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_NARROWTOSELECTION
Language=English
&Narrow%nProtect the out of the selection from modification.%0
.
Language=Japanese
選択範囲外の保護(&N)%n選択範囲外のテキストを変更できないように保護します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_WIDEN
Language=English
&Widen%nRevoke the protection.%0
.
Language=Japanese
テキスト保護の解除(&W)%n変更不能テキストの保護を解除します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_RECOMPOSE
Language=English
Reconvert%nReconvert the selection.%0
.
Language=Japanese
再変換%n選択範囲を未確定文字列として編集しなおします。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_TOGGLEOVERTYPEMODE
Language=English
Overtype Mode%nToggle insert/overtype mode.%0
.
Language=Japanese
挿入/上書きモードの切り替え%n入力方式の挿入モード、上書きモードを切り替えます。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_OPENCANDIDATEWINDOW
Language=English
I&nput Candidates%nShow the candidates window or complete the word.%0
.
Language=Japanese
入力候補(&N)%n入力候補ウィンドウを表示、または単語を補完します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_HOME
Language=English
Start of Document%nGo to the start of the document.%0
.
Language=Japanese
文書の先頭に移動%n文書の先頭に移動します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_END
Language=English
End of Document%nGo to the end of the document.%0
.
Language=Japanese
文書の終端に移動%n文書の終端に移動します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_LINEHOME
Language=English
Start of Line%nGo to the start of the line.%0
.
Language=Japanese
行頭に移動%n現在行の先頭に移動します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_LINEEND
Language=English
End of Line%nGo to the end of the line.%0
.
Language=Japanese
行末に移動%n現在行の終端に移動します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_CHARNEXT
Language=English
Next Character%nGo to the next character.%0
.
Language=Japanese
次の文字に移動%n次の文字に移動します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_SCROLLCOLUMNNEXT
Language=English
Scroll Right%nScroll the window right one column right.%0
.
Language=Japanese
1 列右へスクロール%n1 列右へスクロールします。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_SCROLLCOLUMNPREV
Language=English
Scroll Left%nScroll the window left one column left.%0
.
Language=Japanese
1 列左へスクロール%n1 列左へスクロールします。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_ENSURECARETCENTER
Language=English
Recenter%nEnsure the caret center.%0
.
Language=Japanese
キャレットが中央になるまでスクロール%nキャレットがウィンドウの中央に表示されるようにスクロールします。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_ENSURECARETVISIBLE
Language=English
Show Caret%nEnsure the caret visible.%0
.
Language=Japanese
キャレットが可視になるまでスクロール%nキャレットがウィンドウ内に表示されるようにスクロールします。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_ROWCHARNEXT
Language=English
Extend Box to Next Character%nExtend the rectangle selection to the next character.%0
.
Language=Japanese
次の文字まで矩形選択%n次の文字まで選択を拡張し、矩形選択を開始します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_ROWCHARPREV
Language=English
Extend Box to Previous Character%nExtend the rectangle selection to the previous character.%0
.
Language=Japanese
前の文字まで矩形選択%n前の文字まで選択を拡張し、矩形選択を開始します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_ROWLINEDOWN
Language=English
Extend Box to Down Line%nExtend the rectangle selection to the one down line.%0
.
Language=Japanese
次の行まで矩形選択%n次の行まで選択を拡張し、矩形選択を開始します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_ROWLINEUP
Language=English
Extend Box to Up Line%nExtend the rectangle selection to the one up line.%0
.
Language=Japanese
前の行まで矩形選択%n前の行まで選択を拡張し、矩形選択を開始します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_ROWLINEEND
Language=English
Extend Box to End of Line%nExtend the rectangle selection to the end of the line.%0
.
Language=Japanese
行末まで矩形選択%n行末まで選択を拡張し、矩形選択を開始します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_ROWLINEHOME
Language=English
Extend Box to Start of Line%nExtend the rectangle selection to the start of the line.%0
.
Language=Japanese
行頭まで矩形選択%n行頭まで選択を拡張し、矩形選択を開始します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_ROWWORDNEXT
Language=English
Extend Box to Next Word%nExtend the rectangle selection to the next word.%0
.
Language=Japanese
次の単語の先頭まで矩形選択%n次の単語の先頭まで選択を拡張し、矩形選択を開始します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_ROWWORDPREV
Language=English
Extend Box to Previuos Word%nExtend the rectangle selection to the previous word.%0
.
Language=Japanese
前の単語の先頭まで矩形選択%n前の単語の先頭まで選択を拡張し、矩形選択を開始します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_ROWWORDENDNEXT
Language=English
Extend Box to Next Word End%nExtend the rectangle selection to the next word end.%0
.
Language=Japanese
次の単語の終端まで矩形選択%n次の単語の終端まで選択を拡張し、矩形選択を開始します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_ROWWORDENDPREV
Language=English
Extend Box to Previous Word End%nExtend the rectangle selection to the previous word end.%0
.
Language=Japanese
前の単語の終端まで矩形選択%n前の単語の終端まで選択を拡張し、矩形選択を開始します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_CHARPREV
Language=English
Previous Character%nGo to the previous character.%0
.
Language=Japanese
前の文字に移動%n前の文字に移動します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_WORDENDNEXT
Language=English
Next Word End%nGo to the next word end.%0
.
Language=Japanese
次の単語の終端に移動%n次の単語の終端に移動します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_WORDENDPREV
Language=English
Previous Word End%nGo to the previous word end.%0
.
Language=Japanese
前の単語の終端に移動%n前の単語の終端に移動します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_WORDNEXT
Language=English
Next Word%nGo to the next word.%0
.
Language=Japanese
次の単語の先頭に移動%n次の単語の先頭に移動します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_WORDPREV
Language=English
Previous Word%nGo to the previous word.%0
.
Language=Japanese
前の単語の先頭に移動%n前の単語の先頭に移動します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_LINEDOWN
Language=English
Down Line%nGo to the one down line.%0
.
Language=Japanese
次の行に移動%n次の行に移動します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_LINEUP
Language=English
Up Line%nGo to the one up line.%0
.
Language=Japanese
前の行に移動%n前の行に移動します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_PAGEDOWN
Language=English
Down Page%nGo to the one down page.%0
.
Language=Japanese
次のページに移動%n次のページに移動します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_PAGEUP
Language=English
Up Page%nGo to the one up page.%0
.
Language=Japanese
前のページに移動%n前のページに移動します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_HOMEEXTEND
Language=English
Extend to Start of Document%nExtend the selection to the start of the document.%0
.
Language=Japanese
文書の先頭まで選択を拡張%n文書の先頭まで選択を拡張します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_ENDEXTEND
Language=English
Extend to End of Document%nExtend the selection to the end of the document.%0
.
Language=Japanese
文書の終端まで選択を拡張%n文書の終端まで選択を拡張します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_LINEHOMEEXTEND
Language=English
Extend to Start of Line%nExtend the selection to the start of the line.%0
.
Language=Japanese
行頭まで選択を拡張%n現在行の先頭まで選択を拡張します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_LINEENDEXTEND
Language=English
Extend to End of Line%nExtend the selection to the end of the line.%0
.
Language=Japanese
行末まで選択を拡張%n現在行の終端まで選択を拡張します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_CHARNEXTEXTEND
Language=English
Extend to Next Character%nExtend the selection to the next character.%0
.
Language=Japanese
次の文字まで選択を拡張%n次の文字まで選択を拡張します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_CHARPREVEXTEND
Language=English
Extend to Previous Character%nExtend the selection to the previous character.%0
.
Language=Japanese
前の文字まで選択を拡張%n前の文字まで選択を拡張します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_WORDENDNEXTEXTEND
Language=English
Extend to Next Word End%nExtend the selection to the next word end.%0
.
Language=Japanese
次の単語の終端まで選択を拡張%n次の単語の終端まで選択を拡張します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_WORDENDPREVEXTEND
Language=English
Extend to Previous Word End%nExtend the selection to the previous word end.%0
.
Language=Japanese
前の単語の終端まで選択を拡張%n前の単語の終端まで選択を拡張します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_WORDNEXTEXTEND
Language=English
Extend to Next Word%nExtend the selection to the next word.%0
.
Language=Japanese
次の単語の先頭まで選択を拡張%n次の単語の先頭まで選択を拡張します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_WORDPREVEXTEND
Language=English
Extend to Previous Word%nExtend the selection to the previous word.%0
.
Language=Japanese
前の単語の先頭まで選択を拡張%n前の単語の先頭まで選択を拡張します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_LINEDOWNEXTEND
Language=English
Extend to Next Line%nExtend the selection to the next line.%0
.
Language=Japanese
次の行まで選択を拡張%n次の行まで選択を拡張します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_LINEUPEXTEND
Language=English
Extend to Previous Line%nExtend the selection to the previous line.%0
.
Language=Japanese
次の行まで選択を拡張%n次の行まで選択を拡張します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_PAGEDOWNEXTEND
Language=English
Extend to Next Page%nExtend the selection to the next page.%0
.
Language=Japanese
次のページまで選択を拡張%n次のページまで選択を拡張します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_PAGEUPEXTEND
Language=English
Extend to Previous Page%nExtend the selection to the previous page.%0
.
Language=Japanese
次のページまで選択を拡張%n次のページまで選択を拡張します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_SELECTALL
Language=English
Select &All%nSelect the entire document.%0
.
Language=Japanese
すべて選択(&A)%n文書全体を選択します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_SELECTCURRENTWORD
Language=English
Select Current Word%nSelect the current word.%0
.
Language=Japanese
現在の単語を選択%n現在位置の単語を選択します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_CANCELSELECTION
Language=English
Cancel Selection%nCancel the selection.%0
.
Language=Japanese
選択を解除%n現在の選択を解除します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_SCROLLHOME
Language=English
Scroll to Start%nScroll the window to the top.%0
.
Language=Japanese
先頭行へスクロール%n先頭行へスクロールします。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_SCROLLEND
Language=English
Scroll to End%nScroll the window to the bottom.%0
.
Language=Japanese
最終行へスクロール%n最終行へスクロールします。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_SCROLLLINEDOWN
Language=English
Scroll Down%nScroll the window one line down.%0
.
Language=Japanese
1 行下へスクロール%n1 行下へスクロールします。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_SCROLLLINEUP
Language=English
Scroll Up%nScroll the window one line up.%0
.
Language=Japanese
1 行上へスクロール%n1 行上へスクロールします。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_SCROLLPAGEDOWN
Language=English
Scroll Page Down%nScroll the window one page down.%0
.
Language=Japanese
1 ページ下へスクロール%n1 ページ下へスクロールします。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_EDIT_SCROLLPAGEUP
Language=English
Scroll Page Up%nScroll the window one page up.%0
.
Language=Japanese
1 ページ上へスクロール%n1 ページ上へスクロールします。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_SEARCH_TOP
Language=English
&Search%0
.
Language=Japanese
検索(&S)%0
.

MessageID=
Facility=Command
SymbolicName=CMD_SEARCH_BOOKMARKS
Language=English
&Bookmarks%0
.
Language=Japanese
ブックーマック(&B)%0
.

MessageID=
Facility=Command
SymbolicName=CMD_SEARCH_FIND
Language=English
&Find and Replace...%nShow [Search and Replace] dialog.%0
.
Language=Japanese
検索と置換(&F)...%n[検索と置換] ダイアログを表示します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_SEARCH_FINDNEXT
Language=English
Find &Next%nSearch next match.%0
.
Language=Japanese
次を検索(&N)%n検索条件に適合する次の文字列を選択します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_SEARCH_FINDPREV
Language=English
Find &Previous%nSearch previous match.%0
.
Language=Japanese
前を検索(&P)%n検索条件に適合する前の文字列を選択します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_SEARCH_REPLACEALLINTERACTIVE
Language=English
Replace and Next%nReplace the selection and search next.%0
.
Language=Japanese
置換して次に%n選択文字列を置換し次を検索します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_SEARCH_REPLACEALL
Language=English
Replace All%nReplace all matches.%0
.
Language=Japanese
すべて置換%n検索条件に適合するすべての文字列を置換します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_SEARCH_BOOKMARKALL
Language=English
Mark All%nSet bookmarks on all matched lines.%0
.
Language=Japanese
すべてマーク%n検索条件に適合する文字列が存在するすべての行にブックマークを設定します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_SEARCH_REVOKEMARK
Language=English
Revo&ke Highlight%nRevoke match highlight.%0
.
Language=Japanese
検索マークの解除(&K)%n検索条件に適合する文字列の強調表示を解除します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_SEARCH_GOTOLINE
Language=English
&Go to Line...%nGo to specified line.%0
.
Language=Japanese
指定行へ移動(&G)...%n指定した行へ移動します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_SEARCH_TOGGLEBOOKMARK
Language=English
T&oggle Bookmark%nSet or remove a bookmark on current line.%0
.
Language=Japanese
ブックマークの設定/解除(&O)%n現在行にブックマークを追加/削除します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_SEARCH_NEXTBOOKMARK
Language=English
Nex&t Bookmark%nGo to next bookmark.%0
.
Language=Japanese
次のブックマーク(&T)%n次のブックマークへ移動します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_SEARCH_PREVBOOKMARK
Language=English
Pre&vious Bookmark%nGo to previous bookmark.%0
.
Language=Japanese
前のブックマーク(&V)%n前のブックマークへ移動します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_SEARCH_CLEARBOOKMARKS
Language=English
&Clear All Bookmarks%nRemove all bookmarks in the document.%0
.
Language=Japanese
ブックマークのクリア(&C)%n現在の文書の全てのブックマークを解除します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_SEARCH_MANAGEBOOKMARKS
Language=English
Manage &Bookmarks...%nManage all bookmarks.%0
.
Language=Japanese
ブックマークの管理(&B)...%nブックマークの管理を行います。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_SEARCH_GOTOMATCHBRACKET
Language=English
Go to &Match Brace%nGo to the match brace.%0
.
Language=Japanese
対括弧に移動(&M)%n現在位置の括弧に対応する括弧に移動します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_SEARCH_EXTENDTOMATCHBRACKET
Language=English
E&xtend Match Brace%nExtend the selection to the match brace.%0
.
Language=Japanese
対括弧まで選択を拡張(&X)%n現在位置の括弧に対応する括弧まで選択を拡張します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_SEARCH_INCREMENTALSEARCH
Language=English
&Incremental Search%nStart incremental search.%0
.
Language=Japanese
インクリメンタル検索(&I)%nインクリメンタル検索を行います。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_SEARCH_INCREMENTALSEARCHR
Language=English
&Reverse Incremental Search%nStart reverse incremental search.%0
.
Language=Japanese
逆方向インクリメンタル検索(&R)%n逆方向インクリメンタル検索を行います。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_SEARCH_INCREMENTALSEARCHRF
Language=English
R&egular Expression Incremental Search%nStart regular expression incremental search.%0
.
Language=Japanese
正規表現インクリメンタル検索(&E)%n正規表現インクリメンタル検索を行います。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_SEARCH_INCREMENTALSEARCHRR
Language=English
Reverse Regular Expression Incremental Search%nStart reverse regular expression incremental search.%0
.
Language=Japanese
逆方向正規表現インクリメンタル検索(&U)%n逆方向正規表現インクリメンタル検索を行います。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_SEARCH_INCREMENTALSEARCHMF
Language=English
&Migemo Incremental Search%nStart Migemo incremental search.%0
.
Language=Japanese
Migemo インクリメンタル検索(&M)%nMigemo インクリメンタル検索を行います。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_SEARCH_INCREMENTALSEARCHMR
Language=English
Reverse Migem&o Incremental Search%nStart reverse Migemo incremental search.%0
.
Language=Japanese
逆方向 Migemo インクリメンタル検索(&O)%n逆方向 Migemo インクリメンタル検索を行います。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_SEARCH_FINDFILES
Language=English
Find Files...%nFind files with the given pattern.%0
.
Language=Japanese
ファイルの検索...%nファイルを検索します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_SEARCH_SEARCHMULTIPLEFILES
Language=English
Search in Files...%nSearch pattern in multiple files.%0
.
Language=Japanese
ファイルから検索...%n複数のファイルから検索を行います。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_SEARCH_REPLACEMULTIPLEFILES
Language=English
Replace in Files...%nReplace text in multiple files.%0
.
Language=Japanese
ファイルから置換...%n複数のファイルで置換を行います。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_WINDOW_TOP
Language=English
&Window%0
.
Language=Japanese
ウィンドウ(&W)%0
.

MessageID=
Facility=Command
SymbolicName=CMD_VIEW_TOP
Language=English
&View%0
.
Language=Japanese
表示(&V)%0
.

MessageID=
Facility=Command
SymbolicName=CMD_VIEW_BUFFERS
Language=English
Bu&ffers%0
.
Language=Japanese
バッファ(&F)%0
.

MessageID=
Facility=Command
SymbolicName=CMD_VIEW_TOOLBAR
Language=English
&Toolbar%nShow or hide the toolbar.%0
.
Language=Japanese
ツールバー(&T)%nツールバーの表示/非表示を切り替えます。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_VIEW_STATUSBAR
Language=English
&Status Bar%nShow or hide the status bar.%0
.
Language=Japanese
ステータスバー(&S)%nステータスバーの表示/非表示を切り替えます。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_VIEW_WRAPNO
Language=English
N&o Wrap%nDo not wrap lines.%0
.
Language=Japanese
折り返さない(&O)%n行の折り返し表示を行いません。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_VIEW_WRAPBYSPECIFIEDWIDTH
Language=English
Wrap by Sp&ecified Width(&E)%nWrap lines at specified position.%0
.
Language=Japanese
指定幅で折り返す(&E)%n行を指定した幅で折り返して表示します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_VIEW_WRAPBYWINDOWWIDTH
Language=English
Wrap by &Window%nWrap lines at the end of window.%0
.
Language=Japanese
ウィンドウ端で折り返す(&W)%n行をウィンドウ幅で折り返して表示します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_VIEW_REFRESH
Language=English
&Refresh%nRefresh the window.%0
.
Language=Japanese
再描画(&R)%n画面を再描画します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_VIEW_NEXTBUFFER
Language=English
&Next Buffer%nShow next buffer.%0
.
Language=Japanese
次のバッファ(&N)%n次のバッファを表示します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_VIEW_PREVBUFFER
Language=English
&Previous Buffer%nShow previous buffer.%0
.
Language=Japanese
前のバッファ(&P)%n前のバッファを表示します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_VIEW_BUFFERBAR
Language=English
&Buffer Bar%nShow or hide the buffer bar.%0
.
Language=Japanese
バッファバー(&B)%nバッファバーの表示/非表示を切り替えます。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_WINDOW_SPLITNS
Language=English
Sp&lit Window%nSplit the window into up and bottom.%0
.
Language=Japanese
ウィンドウの上下分割(&L)%nエディタウィンドウを上下に分割します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_WINDOW_SPLITWE
Language=English
Split Window Si&de-by-Side%nSplit the window into side-by-side.%0
.
Language=Japanese
ウィンドウの左右分割(&D)%nエディタウィンドウを左右に分割します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_WINDOW_UNSPLITACTIVE
Language=English
&Close Window%nClose active window.%0
.
Language=Japanese
現在のウィンドウを閉じる(&C)%nアクティブなエディタウィンドウを閉じ、分割を解除します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_WINDOW_UNSPLITOTHERS
Language=English
Close Ot&her Windows%nClose all inactive windows.%0
.
Language=Japanese
他のウィンドウを閉じる(&H)%nアクティブなエディタウィンドウを残し、すべてのウィンドウを閉じます。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_WINDOW_NEXTPANE
Language=English
Ne&xt Pane%nActivate next window.%0
.
Language=Japanese
次のウィンドウ(&X)%n次のエディタウィンドウをアクティブにします。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_WINDOW_PREVPANE
Language=English
Pre&vious Pane%nActivate previous window.%0
.
Language=Japanese
前のウィンドウ(&V)%n前のエディタウィンドウをアクティブにします。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_WINDOW_TOPMOSTALWAYS
Language=English
&Always in Foreground%nShow the application window foreground always.%0
.
Language=Japanese
常に最前面に表示(&A)%nウィンドウを常に最前面に表示します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_MACRO_TOP
Language=English
&Macros%0
.
Language=Japanese
マクロ(&M)%0
.

MessageID=
Facility=Command
SymbolicName=CMD_MACRO_SCRIPTS
Language=English
&Scripts%0
.
Language=Japanese
スクリプト(&S)%0
.

MessageID=
Facility=Command
SymbolicName=CMD_MACRO_DEFINE
Language=English
Start/End &Definition%nStart or end the keyboard macro definition.%0
.
Language=Japanese
記録の開始/終了(&D)%nキーボードマクロの記録を開始/終了します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_MACRO_EXECUTE
Language=English
E&xecute%nExecute the active keyboard macro.%0
.
Language=Japanese
実行(&X)%nアクティブなキーボードマクロを実行します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_MACRO_APPEND
Language=English
&Append%nAppend definition to the active keyboard macro.%0
.
Language=Japanese
追加記録の開始(&A)%nアクティブなキーボードマクロに追加記録を開始します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_MACRO_PAUSERESTART
Language=English
&Pause/Restart%nPause or restart the keyboard macro definition.%0
.
Language=Japanese
記録の一時停止/再開(&P)%nキーボードマクロの記録を一時停止/再開します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_MACRO_INSERTQUERY
Language=English
Insert &Query%nInsert an user prompt into the defining keyboard macro.%0
.
Language=Japanese
プロンプトの挿入(&Q)%n記録中のキーボードマクロにユーザプロンプトを挿入します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_MACRO_ABORT
Language=English
A&bort%nAbort the keyboard macro definition.%0
.
Language=Japanese
記録の中止(&B)%nキーボードマクロの記録を中止します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_MACRO_SAVEAS
Language=English
Sa&ve As...%nSave the active keyboard macro with name.%0
.
Language=Japanese
名前を付けて保存(&V)...%nアクティブなキーボードマクロに名前を付けて保存します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_MACRO_LOAD
Language=English
&Load...%nLoad keyboard macro from a file.%0
.
Language=Japanese
読み込み(&L)...%nキーボードマクロをファイルから読み込みます。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_TOOL_TOP
Language=English
&Tool%0
.
Language=Japanese
ツール(&T)%0
.

MessageID=
Facility=Command
SymbolicName=CMD_TOOL_APPDOCTYPES
Language=English
&Apply Document Type%0
.
Language=Japanese
適用文書タイプ(&A)%0
.

MessageID=
Facility=Command
SymbolicName=CMD_TOOL_COMMONOPTION
Language=English
&Common Options...%nSet the options common to all document types.%0
.
Language=Japanese
共通設定(&C)...%nすべての文書タイプに共通のオプションを設定します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_TOOL_DOCTYPEOPTION
Language=English
Document T&ype Options...%nSet the options specific to the active document type.%0
.
Language=Japanese
文書タイプ別設定(&Y)...%n文書タイプ別のオプションを設定します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_TOOL_FONT
Language=English
&Font...%nSet font settings.%0
.
Language=Japanese
フォント設定(&F)...%nフォントの設定を行います。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_TOOL_EXECUTE
Language=English
&Execute%nExecute document type specific program.%0
.
Language=Japanese
実行(&E)%n現在のファイル名を引数にして文書タイプ指定のプログラムを実行します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_TOOL_EXECUTECOMMAND
Language=English
E&xecute Command...%nExecute external command.%0
.
Language=Japanese
コマンドの実行(&X)...%n外部コマンドを実行します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_HELP_TOP
Language=English
&Help%0
.
Language=Japanese
ヘルプ(&H)%0
.

MessageID=
Facility=Command
SymbolicName=CMD_HELP_ABOUT
Language=English
&About%nDisplay information and version of Alpha.%0
.
Language=Japanese
バージョン情報(&A)%nAlpha の情報、バージョンを表示します。%0
.

MessageID=
Facility=Command
SymbolicName=CMD_SPECIAL_WAITINGFORNEXTKEYCOMBINATION
Language=English
.
Language=Japanese
.

MessageID=
Facility=Command
SymbolicName=CMD_SPECIAL_ILLEGALKEYSTROKES
Language=English
.
Language=Japanese
.

MessageID=
Facility=Command
SymbolicName=CMD_SPECIAL_MRUSTART
Language=English
.
Language=Japanese
.

MessageID=+1000
Facility=Command
SymbolicName=CMD_SPECIAL_MRUEND
Language=English
.
Language=Japanese
.

MessageID=
Facility=Command
SymbolicName=CMD_SPECIAL_BUFFERSSTART
Language=English
.
Language=Japanese
.

MessageID=+1000
Facility=Command
SymbolicName=CMD_SPECIAL_BUFFERSEND
Language=English
.
Language=Japanese
.

MessageID=
Facility=Command
SymbolicName=CMD_SPECIAL_END
Language=English
.
Language=Japanese
.
