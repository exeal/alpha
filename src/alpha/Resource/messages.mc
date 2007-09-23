;/*
; This file defines message strings.
; Translators are the following:
; - exeal (exeal@users.sourceforge.jp) -- Japanese
;*/

FacilityNames=(
  Command=0x1
  Encoding=0x2
  ExtendedEncoding=0x3
)
LanguageNames=(English=0x0409:MSG00409)
LanguageNames=(Japanese=0x0411:MSG00411)


;// general messages

SymbolicName=MSG_BUFFER__BUFFER_IS_DIRTY
Language=English
%1%n%nThe file is modified. Do you want to save?
.
Language=Japanese
%1%n%nこのファイルは変更されています。保存しますか?
.

SymbolicName=MSG_BUFFER__CONFIRM_REOPEN
Language=English
Do you want to reopen the file?
.
Language=Japanese
現在のファイルを開き直しますか?
.

SymbolicName=MSG_BUFFER__CONFIRM_REOPEN_EVEN_IF_DIRTY
Language=English
The file is modified.
If you reopen, the modification will be lost. Do you want to do?
.
Language=Japanese
現在のファイルは変更されています。
ファイルを開き直すと変更が失われますが、よろしいですか?
.

SymbolicName=MSG_BUFFER__FAILED_TO_SEND_TO_MAILER
Language=English
Failed to send to a mailer.
.
Language=Japanese
メーラへの送信に失敗しました。
.

SymbolicName=MSG_BUFFER__UNTITLED
Language=English
untitled
.
Language=Japanese
無題
.

SymbolicName=MSG_BUFFER__FILE_IS_MODIFIED_AND_REOPEN
Language=English
%1%n%nThe file was modified by other process. Do you want to reload?
If you reload, current editing contents will be disposed.
.
Language=Japanese
%1%n%nこのファイルは他のプロセスで変更されています。ファイルを読み込み直しますか?
読み込み直すと現在の編集内容は破棄されます。
.

SymbolicName=MSG_BUFFER__FILE_IS_MODIFIED_AND_SAVE
Language=English
%1%n%nThe file was modified by other process. Do you want to continue to save?
.
Language=Japanese
%1%n%nこのファイルは他のプロセスで変更されています。ファイルの保存を続行しますか?
.

SymbolicName=MSG_BUFFER__FILE_IS_MODIFIED_AND_EDIT
Language=English
%1%n%nThe file was modified by other process. Do you want to continue to edit?
.
Language=Japanese
%1%n%nこのファイルは他のプロセスで変更されています。編集を続行しますか?
.

SymbolicName=MSG_BUFFER__SAVING_FILE_IS_OPENED
Language=English
%1%n%nThe file has been already opened. Cannot write to an editing file.
.
Language=Japanese
%1%n%nこのファイルは既に開いています。編集中のファイルに上書きすることはできません。
.

SymbolicName=MSG_SEARCH__PATTERN_NOT_FOUND
Language=English
Specified string was not found.
.
Language=Japanese
指定された文字列は見つかりませんでした。
.

SymbolicName=MSG_SEARCH__REPLACE_DONE
Language=English
Replaced %1 string(s).
.
Language=Japanese
%1 個の文字列を置換しました。
.

SymbolicName=MSG_SEARCH_REGEX_IS_INAVAILABLE
Language=English
Failed to load the regular expression engine. Cannot search.
.
Language=Japanese
正規表現エンジンのロードに失敗しました。検索を行うことはできません。
.

SymbolicName=MSG_SEARCH__INVALID_REGEX_PATTERN
Language=English
An error occured during regular expression search.%n%nReason:%t%1%nPosition:%t%2
.
Language=Japanese
正規表現検索中にエラーが発生しました。%n%n理由:%t%1%n位置:%t%2
.

SymbolicName=MSG_SEARCH__BAD_PATTERN_START
Language=English
Not an error
.
Language=Japanese
エラーではありません
.

Language=English
An invalid collating element was specified in a [[.name.]] block
.
Language=Japanese
不正な照合要素です
.

Language=English
An invalid character class name was specified in a [[:name:]] block
.
Language=Japanese
不正な文字セットです
.

Language=English
An invalid or trailing escape was encountered
.
Language=Japanese
不正なエスケープです
.

Language=English
A back-reference to a non-existant marked sub-expression was encountered
.
Language=Japanese
存在しない部分式への後方参照が見つかりました
.

Language=English
An invalid character set was encountered
.
Language=Japanese
不正な文字クラスです
.

Language=English
Mismatched '(' and ')'
.
Language=Japanese
対応する丸括弧 (')') が見つかりません
.

Language=English
Mismatched '{' and '}'
.
Language=Japanese
対応する中括弧 ('}') が見つかりません
.

Language=English
Invalid contents of a {...} block
.
Language=Japanese
中括弧の内容 ("{...}") が不正です
.

Language=English
A character range was invalid
.
Language=Japanese
不正な文字範囲です
.

Language=English
Out of memory
.
Language=Japanese
メモリ不足のため、検索を行うことができません
.

Language=English
An attempt to repeat something that can not be repeated
.
Language=Japanese
不正な繰り返しです
.

Language=English
The expression bacame too complex to handle
.
Language=Japanese
パターンが複雑なため、検索を行うことができません
.

Language=English
Out of program stack space
.
Language=Japanese
スタック空間が枯渇しています。検索を行うことはできません
.

Language=English
Unknown error
.
Language=Japanese
不明
.

SymbolicName=MSG_ERROR__EXE_NOT_REGISTERED
Language=English
"%1"%nNo program assigned to the document type.
.
Language=Japanese
"%1"%nこの文書タイプには実行に使用するプログラムが設定されていません。
.

SymbolicName=MSG_ERROR__FAILED_TO_RUN_EXE
Language=English
Failed to run the program.
The path assigned to the document type may be incorrect.
.
Language=Japanese
プログラムの実行に失敗しました。
文書タイプに設定されているプログラムのパスが正しくない可能性があります。
.

SymbolicName=MSG_ERROR__UNSUPPORTED_OS_VERSION
Language=English
Alpha does not support your platform.
.
Language=Japanese
ご使用のバージョンの OS では Alpha を実行することはできません。
.

SymbolicName=MSG_ERROR__DUPLICATE_ABBREV
Language=English
There is already same abbreviation.
.
Language=Japanese
既に同じ短縮語句が存在します。
.

SymbolicName=MSG_ERROR__REGEX_UNKNOWN_ERROR
Language=English
Error occured during regular expression pattern matching.
The pattern may be too complex.
.
Language=Japanese
正規表現パターンマッチ中にエラーが発生しました。
パターンが複雑すぎる可能性があります。
.

SymbolicName=MSG_ERROR__MIGEMO_UNKNOWN_ERROR
Language=English
Error occured in Migemo.
.
Language=Japanese
Migemo の内部でエラーが発生しました。
.

SymbolicName=MSG_ERROR__PROHIBITED_FOR_MACRO_DEFINING
Language=English
This command cannot execute in keyboard macro definition.
.
Language=Japanese
この操作はキーボードマクロ記録中には実行できません。
.

SymbolicName=MSG_ERROR__FAILED_TO_LOAD_TEMP_MACRO
Language=English
Failed to load the keyboard macro file.%n%nFile: %1%nPosition: (%2, %3)%nReason: %4
.
Language=Japanese
キーボードマクロファイルの読み込みに失敗しました。%n%nファイル: %1%n位置: (%2, %3)%n理由: %4
.

SymbolicName=MSG_ERROR__FAILED_TO_LOAD_SOMETHING
Language=English
(Failed to load)
.
Language=Japanese
(読み込みに失敗しました)
.

SymbolicName=MSG_ERROR__OUT_OF_MEMORY
Language=English
Process failed for out of memory.
.
Language=Japanese
メモリ不足のため、処理に失敗しました。
.

SymbolicName=MSG_STATUS__READ_ONLY_CAPTION
Language=English
[readonly]
.
Language=Japanese
[読み取り専用]
.

SymbolicName=MSG_STATUS__TEMP_MACRO_DEFINING
Language=English
Defining keyboard macro
.
Language=Japanese
キーボードマクロ記録中
.

SymbolicName=MSG_STATUS__TEMP_MACRO_PAUSING
Language=English
Pausing keyboard macro definition
.
Language=Japanese
キーボードマクロ記録中断中
.

SymbolicName=MSG_STATUS__NARROWING
Language=English
Narrowing
.
Language=Japanese
ナローイング
.

SymbolicName=MSG_STATUS__DEBUGGING
Language=English
Debuggnig
.
Language=Japanese
デバッグ中
.

SymbolicName=MSG_STATUS__INSERT_MODE
Language=English
Insert
.
Language=Japanese
挿入
.

SymbolicName=MSG_STATUS__OVERTYPE_MODE
Language=English
Overtype
.
Language=Japanese
上書き
.

SymbolicName=MSG_STATUS__CARET_POSITION
Language=English
Line: %1!lu! , Column: %2!lu! , Char: %3!lu!
.
Language=Japanese
%lu 行 、 %lu 列 、 %lu 文字
.

SymbolicName=MSG_STATUS__CAN_EXPAND_ABBREV
Language=English
'%1' can expand to '%2'.
.
Language=Japanese
'%1' は '%2' に展開可能です。
.

SymbolicName=MSG_STATUS__LOADING_FILE
Language=English
Loading '%1'...
.
Language=Japanese
'%1' を読み込み中...
.

SymbolicName=MSG_STATUS__WAITING_FOR_2ND_KEY
Language=English
'%1' is pressed. Waiting for the second key combination.
.
Language=Japanese
'%1' が押されました。2番目のキー組み合わせを待っています。
.

SymbolicName=MSG_STATUS__INVALID_2STROKE_COMBINATION
Language=English
There are no command bound to the key combination '%1'.
.
Language=Japanese
キーストローク '%1' に割り当てられているコマンドはありません。
.

SymbolicName=MSG_STATUS__ISEARCH
Language=English
Incremental search : %1
.
Language=Japanese
インクリメンタル検索 : %1
.

SymbolicName=MSG_STATUS__ISEARCH_EMPTY_PATTERN
Language=English
Incremental search : (empty pattern)
.
Language=Japanese
インクリメンタル検索 : (検索文字列が入力されていません)
.

SymbolicName=MSG_STATUS__ISEARCH_BAD_PATTERN
Language=English
Incremental Search : %1 (invalid pattern)
.
Language=Japanese
インクリメンタル検索 : %1 (パターンが正しくありません)
.

SymbolicName=MSG_STATUS__ISEARCH_NOT_FOUND
Language=English
Incremental search : %1 (not found)
.
Language=Japanese
インクリメンタル検索 : %1 (見つかりません)
.

SymbolicName=MSG_STATUS__RISEARCH
Language=English
Reversal incremental search : %1
.
Language=Japanese
逆方向インクリメンタル検索 : %1
.

SymbolicName=MSG_STATUS__RISEARCH_NOT_FOUND
Language=English
Reversal incremental search : %1 (not found)
.
Language=Japanese
逆方向インクリメンタル検索 : %1 (見つかりません)
.

SymbolicName=MSG_STATUS__RISEARCH_BAD_PATTERN
Language=English
Reversal Incremental Search : %1 (invalid pattern)
.
Language=Japanese
逆方向インクリメンタル検索 : %1 (パターンが正しくありません)
.

SymbolicName=MSG_STATUS__RISEARCH_EMPTY_PATTERN
Language=English
Reversal incremental search : (empty pattern)
.
Language=Japanese
逆方向インクリメンタル検索 : (検索文字列が入力されていません)
.

SymbolicName=MSG_STATUS__MATCH_BRACKET_OUT_OF_VIEW
Language=English
(Match brace) %1 : %2 
.
Language=Japanese
(対括弧) %1 : %2 
.

SymbolicName=MSG_STATUS__INVOKABLE_LINK_POPUP
Language=English
%1%nCtrl plus click to open the link.
.
Language=Japanese
%1%nCtrl キーを押しながらクリックするとリンクを起動します。
.

SymbolicName=MSG_DIALOG__DEFAULT_OPENFILE_FILTER
Language=English
All Files:*.*
.
Language=Japanese
すべてのファイル:*.*
.

SymbolicName=MSG_DIALOG__WHOLE_GRAPHEME_MATCH
Language=English
Grapheme Cluster
.
Language=Japanese
書記素
.

SymbolicName=MSG_DIALOG__WHOLE_WORD_MATCH
Language=English
Word
.
Language=Japanese
単語
.

SymbolicName=MSG_DIALOG__LITERAL_SEARCH
Language=English
Literal
.
Language=Japanese
リテラル
.

SymbolicName=MSG_DIALOG__REGEX_SEARCH
Language=English
Regular expression (Boost.Regex 1.34.1)
.
Language=Japanese
正規表現 (Boost.Regex 1.34.1)
.

SymbolicName=MSG_DIALOG__MIGEMO_SEARCH
Language=English
Migemo (C/Migemo 1.2 expected)
.
Language=Japanese
Migemo (C/Migemo 1.2 expected)
.

SymbolicName=MSG_DIALOG__BUFFERBAR_CAPTION
Language=English
Buffers
.
Language=Japanese
バッファ
.

SymbolicName=MSG_DIALOG__ABBREVIATION
Language=English
Abbreviation
.
Language=Japanese
短縮語
.

SymbolicName=MSG_DIALOG__EXPANDED_ABBREVIATION
Language=English
Expanded String
.
Language=Japanese
展開後の文字列
.

SymbolicName=MSG_DIALOG__BOOKMARKED_LINE
Language=English
Line
.
Language=Japanese
行
.

SymbolicName=MSG_DIALOG__BOOKMARKED_POSITION
Language=English
Position
.
Language=Japanese
位置
.

SymbolicName=MSG_DIALOG__LINE_NUMBER_RANGE
Language=English
Line &Number (%1-%2):
.
Language=Japanese
行番号 (%1-%2)(&N):
.

SymbolicName=MSG_DIALOG__SELECT_ALL
Language=English
Select &All
.
Language=Japanese
すべて選択(&A)
.

SymbolicName=MSG_DIALOG__UNSELECT_ALL
Language=English
&Cancel All Selections
.
Language=Japanese
選択をすべて解除(&C)
.

SymbolicName=MSG_DIALOG__SELECT_DIRECTORY
Language=English
Select a directory.
.
Language=Japanese
ディレクトリを選択してください。
.

SymbolicName=MSG_DIALOG__FILE_OPERATION
Language=English
File Operation
.
Language=Japanese
ファイル操作
.

SymbolicName=MSG_DIALOG__RENAME_FILE
Language=English
Rename
.
Language=Japanese
名前の変更
.

SymbolicName=MSG_DIALOG__COPY_FILE
Language=English
Copy
.
Language=Japanese
コピー
.

SymbolicName=MSG_DIALOG__MOVE_FILE
Language=English
Move
.
Language=Japanese
移動
.

SymbolicName=MSG_DIALOG__KEEP_NEWLINE
Language=English
No Change
.
Language=Japanese
統一しない
.

SymbolicName=MSG_DIALOG__SAVE_FILE_FILTER
Language=English
All Files
.
Language=Japanese
すべてのファイル
.

SymbolicName=MSG_IO__FAILED_TO_DETECT_ENCODE
Language=English
%1%n%nFailed to detect code page of the file. Used system-default code page.
.
Language=Japanese
%1%n%nコードページを判別できませんでした。システム既定のコードページを使用します。
.

SymbolicName=MSG_IO__INVALID_ENCODING
Language=English
%1%n%nSpecified code page is invalid. Cannot open the file.
.
Language=Japanese
%1%n%n指定されたコードページは正しくありません。ファイルを開くことはできません。
.

SymbolicName=MSG_IO__INVALID_NEWLINE
Language=English
%1%n%nSpecified newline is invalid. Cannot open the file.
.
Language=Japanese
%1%n%n指定された改行コードは正しくありません。ファイルを開くことはできません。
.

SymbolicName=MSG_IO__UNCONVERTABLE_UCS_CHAR
Language=English
%1%nEncoding: %2%n
The document contains unconvertable characters under specified encoding.
These characters will be written incorrectly.
Do you want to change code page?
.
Language=Japanese
%1%nエンコード%2%n
この文書は指定したエンコードで変換できない文字を含んでいます。
このまま続行するとそれらの文字は正しく保存されません。
コードページを変更しますか?
.

SymbolicName=MSG_IO__UNCONVERTABLE_NATIVE_CHAR
Language=English
%1%nEncoding: %2%n
The file contains unconvertable characters under specified encoding.
Saving may destroy contents of the file.
Do you want to change code page?
.
Language=Japanese
%1%nエンコード: %2%n
このファイルは指定したエンコードで Unicode に変換できない文字を含んでいます。
編集を続行して保存するとファイルの内容が破壊される可能性があります。
コードページを変更しますか?
.

SymbolicName=MSG_IO__HUGE_FILE
Language=English
%1%n%nThe file size is too large. Maximum file size Alpha can handle is 2GB.
.
Language=Japanese
%1%n%nファイルサイズが大きすぎます。Alpha が扱えるファイルサイズの上限値は 2GB です。
.

SymbolicName=MSG_IO__FAILED_TO_WRITE_FOR_READONLY
Language=English
%1%n%nCannot overwrite the file. The file is read-only.
.
Language=Japanese
%1%n%nファイルを上書き保存できません。このバッファは読み取り専用です。
.

SymbolicName=MSG_IO__FAILED_TO_RESOLVE_SHORTCUT
Language=English
%1%n%nFailed to resolve the shortcut. Cannot open the file.
.
Language=Japanese
%1%n%nショートを解決できませんでした。ファイルを開くことはできません。
.

SymbolicName=MSG_IO__CANNOT_OPEN_FILE
Language=English
%1%n%nCannot open the file.
.
Language=Japanese
%1%n%nファイルを開くことができません。
.

SymbolicName=MSG_IO__CANNOT_CREATE_TEMP_FILE
Language=English
%1%n%nFailed to save the file. Cannot create temporary file.
.
Language=Japanese
%1%n%nファイルの保存に失敗しました。一時ファイルを作成できませんでした。
.

SymbolicName=MSG_IO__LOST_DISK_FILE
Language=English
%1%n%nFailed to save the file. The file is lost.
.
Language=Japanese
%1%n%nファイルの保存に失敗しました。ファイルが失われました。
.

SymbolicName=MSG_SCRIPT__ERROR_DIALOG
Language=English
Script error occured.%n%nScript:%t%1%nLine:%t%2%nCharacter:%t%3%nError:%t%4%nSCODE:%t%5%nSource:%t%6
.
Language=Japanese
スクリプトエラーが発生しました。%n%nスクリプト:%t%1%n行:%t%2%n文字:%t%3%nエラー:%t%4%nSCODE:%t%5%nエラー元:%t%6
.

SymbolicName=MSG_SCRIPT__UNSAFE_ACTIVEX_CAUTION
Language=English
%1%n%nThis script is about to create an unsafe object.%nDo you allow this?%n%nQueried object:%n  ProgID:%t%2%n  IID:%t%3
.
Language=Japanese
%1%n%nこのスクリプトは安全でない可能性のあるオブジェクトを作成しようとしています。%nオブジェクトの作成を許可しますか?%n%n要求されたオブジェクト:%n  ProgID:%t%2%n  IID:%t%3
.

SymbolicName=MSG_SCRIPT__INVALID_LANGUAGE_NAME
Language=English
The language "%1" bound to the macro is invalid or not installed. Cannot execute.
.
Language=Japanese
マクロに指定されている言語 "%1" は正しくないか、インストールされていません。マクロを実行することはできません。
.

SymbolicName=MSG_SCRIPT__FAILED_TO_OPEN_MACRO_SCRIPT
Language=English
%1%n%nFailed to load the macro file. Cannot execute.
.
Language=Japanese
%1%n%nマクロスクリプトが読み込めませんでした。マクロを実行することはできません。
.

SymbolicName=MSG_OTHER__EMPTY_MENU_CAPTION
Language=English
(nothing)
.
Language=Japanese
(なし)
.

SymbolicName=MSG_OTHER__UNKNOWN
Language=English
(unknown)
.
Language=Japanese
(不明)
.

SymbolicName=MSG_OTHER__NOT_OBTAINED
Language=English
(not obtained)
.
Language=Japanese
(取得できませんでした)
.

SymbolicName=MSG_OTHER__TEMPORARY_MACRO_QUERY
Language=English
Proceed the macro?\n\nNo: Skip this repetition and next\nCancel: Abort the macro
.
Language=Japanese
このマクロを続行しますか?\n\nいいえ: 現在の繰り返しをスキップし、次の繰り返しを開始します\nキャンセル: マクロの実行を中止します
.


;// command names and descriptions

Facility=Command
SymbolName=CMD_FILE_TOP
Language=English
&File
.
Language=Japanese
ファイル(&F)
.

Facility=Command
SymbolicName=CMD_FILE_NEW
Language=English
&New\nCreate a new document.
.
Language=Japanese
新規(&N)\n文書を新しく作成します。
.

Facility=Command
SymbolicName=CMD_FILE_NEWWITHFORMAT
Language=English
New wi&th Format...\nCreate a new document with specified code page, line break, and document type.
.
Language=Japanese
書式を指定して新規(&T)...\nコードページ、改行コード、文書タイプを指定して文書を新しく作成します。
.

Facility=Command
SymbolicName=CMD_FILE_OPEN
Language=English
&Open...\nOpen an existing document.
.
Language=Japanese
開く(&O)...\n既存のファイルを開きます。
.

Facility=Command
SymbolicName=CMD_FILE_CLOSE
Language=English
&Close\nClose the document.
.
Language=Japanese
閉じる(&C)\n現在の文書を閉じます。
.

Facility=Command
SymbolicName=CMD_FILE_CLOSEALL
Language=English
Cl&ose All\nClose all documents.
.
Language=Japanese
すべて閉じる(&Q)\nすべての文書を閉じます。
.

Facility=Command
SymbolicName=CMD_FILE_SAVE
Language=English
&Save\nSave the document.
.
Language=Japanese
上書き保存(&S)\n現在の文書を保存します。
.

Facility=Command
SymbolicName=CMD_FILE_SAVEAS
Language=English
Save &As...\nSave the document under a different name.
.
Language=Japanese
名前を付けて保存(&A)...\n現在の文書を新しい名前で保存します。
.

Facility=Command
SymbolicName=CMD_FILE_SAVEALL
Language=English
Sav&e All\nSave all documents.
.
Language=Japanese
すべて保存(&E)\nすべての文書を保存します。
.

Facility=Command
SymbolicName=CMD_FILE_MRU
Language=English

.
Language=Japanese
最近使ったファイル(&F)
.
Most Recent Used &Files
Facility=Command
SymbolicName=CMD_FILE_OPERATE
Language=Japanese
ファイル操作(&E)
.

Facility=Command
SymbolicName=CMD_FILE_REOPEN
Language=English
&Reopen\nReopen the document.
.
Language=Japanese
開き直す(&R)\n現在の文書を開き直します。
.

Facility=Command
SymbolicName=CMD_FILE_REOPENWITHCODEPAGE
Language=English
Reopen with Different Code Pa&ge...\nReopen the document with a different code page.
.
Language=Japanese
コードページを変更して開きな直す(&G)\n現在の文書をコードページを指定して開き直します。
.

Facility=Command
SymbolicName=CMD_FILE_EXIT
Language=English
E&xit Alpha\nClose all documents and exit.
.
Language=Japanese
Alpha の終了(&X)\n文書の保存を確認し、Alpha を終了します。
.

Facility=Command
SymbolicName=CMD_FILE_SENDMAIL
Language=English
Sen&d...\nSend the file.
.
Language=Japanese
送信(&D)...\n電子メールでこの文書を送信します。
.

Facility=Command
SymbolicName=CMD_FILE_CLOSEOTHERS
Language=English
Close Ot&hers\nClose all inactive documents.
.
Language=Japanese
他を閉じる(&H)\n現在の文書以外をすべて閉じます。
.

Facility=Command
SymbolicName=CMD_FILE_PRINT
Language=English

.
Language=Japanese
印刷(&P)...\n現在の文書を印刷します。
.
&Print...\nPrint the document.
Facility=Command
SymbolicName=CMD_FILE_PRINTSETUP
Language=English
Set&up Page...\nSetup page layout.
.
Language=Japanese
ページ設定(&U)...\nページ レイアウトの設定を変更します。
.

Facility=Command
SymbolicName=CMD_FILE_PRINTPREVIEW
Language=English
Pre&view Page...\nPreview the entire page.
.
Language=Japanese
印刷プレビュー(&V)\nページ全体を表示します。
.

Facility=Command
SymbolicName=CMD_EDIT_TOP
Language=English
&Edit
.
Language=Japanese
編集(&E)
.

Facility=Command
SymbolicName=CMD_EDIT_ADVANCED
Language=English
&Advanced
.
Language=Japanese
高度な操作(&V)
.

Facility=Command
SymbolicName=CMD_EDIT_INSERTUNICODECTRLS
Language=English
&Insert Unicode Control Characters
.
Language=Japanese
Unicode 制御文字の挿入(&I)
.

Facility=Command
SymbolicName=CMD_EDIT_INSERTUNICODEWSS
Language=English
Insert Unicode &Whitespace Characters
.
Language=Japanese
Unicode 空白文字の挿入(&W)
.

Facility=Command
SymbolicName=CMD_EDIT_DELETE
Language=English
&Delete\nDelete the selection.
.
Language=Japanese
削除(&D)\n選択範囲を削除します。
.

Facility=Command
SymbolicName=CMD_EDIT_BACKSPACE
Language=English
Backspace\nDelete backward one character.
.
Language=Japanese
前の 1 文字を削除\n直前の 1 文字を削除します。
.

Facility=Command
SymbolicName=CMD_EDIT_DELETETONEXTWORD
Language=English
Delete Next Word\nDelete to next word.
.
Language=Japanese
次の単語まで削除\n次の単語の先頭までを削除します。
.

Facility=Command
SymbolicName=CMD_EDIT_DELETETOPREVWORD
Language=English
Delete Previous Word\nDelete to previous word.
.
Language=Japanese
前の単語まで削除\n前の単語の先頭までを削除します。
.

Facility=Command
SymbolicName=CMD_EDIT_DELETELINE
Language=English
Delete Line\nDelete current line.
.
Language=Japanese
現在行を削除\n現在行を削除します。
.

Facility=Command
SymbolicName=CMD_EDIT_INSERTPREVLINE
Language=English
Insert Previous\nInsert new line previous.
.
Language=Japanese
上に 1 行挿入\n1 つ上の行に改行を挿入します。
.

Facility=Command
SymbolicName=CMD_EDIT_BREAK
Language=English
Break\nBreak the line.
.
Language=Japanese
改行\n改行文字を挿入します。
.

Facility=Command
SymbolicName=CMD_EDIT_UNDO
Language=English
&Undo\nUndo editing.
.
Language=Japanese
元に戻す(&U)\n直前に行った動作を取り消します。
.

Facility=Command
SymbolicName=CMD_EDIT_REDO
Language=English
&Redo\nRedo previously undone editing.
.
Language=Japanese
やり直し(&R)\n取り消された動作をやり直します。
.

Facility=Command
SymbolicName=CMD_EDIT_CUT
Language=English
Cu&t\nCut the selection and put it on the Clipboard.
.
Language=Japanese
切り取り(&T)\n選択範囲を切り取ってクリップボードに移動します。
.

Facility=Command
SymbolicName=CMD_EDIT_COPY
Language=English
&Copy\nCopy the selection and put it on the Clipboard.
.
Language=Japanese
コピー(&C)\n選択範囲をクリップボードにコピーします。
.

Facility=Command
SymbolicName=CMD_EDIT_PASTE
Language=English
&Paste\nInsert Clipboard contents.
.
Language=Japanese
貼り付け(&P)\nクリップボードの内容を挿入します。
.

Facility=Command
SymbolicName=CMD_EDIT_INSERTTAB
Language=English
Insert Tab\nInsert a tab.
.
Language=Japanese
タブを挿入\nタブを挿入します。
.

Facility=Command
SymbolicName=CMD_EDIT_DELETETAB
Language=English
Delete Tab\nDelete a tab.
.
Language=Japanese
タブを削除\nタブを削除します。
.

Facility=Command
SymbolicName=CMD_EDIT_TABIFY
Language=English
Tabify\nConvert whitespaces in the selection to tabs.
.
Language=Japanese
空白をタブに変換\n選択範囲の空白類文字をタブに変換します。
.

Facility=Command
SymbolicName=CMD_EDIT_UNTABIFY
Language=English
Untabify\nConvert tabs in the selection to spaces.
.
Language=Japanese
タブを空白に変換\n選択範囲のタブを半角空白に変換します。
.

Facility=Command
SymbolicName=CMD_EDIT_PASTEFROMCLIPBOARDRING
Language=English
Paste From C&lipboard Ring\nInsert Clipboard Ring contents.
.
Language=Japanese
クリップボードリングから貼り付け(&L)\nクリップボードリングの内容を貼り付けます。
.

Facility=Command
SymbolicName=CMD_EDIT_CHARFROMABOVELINE
Language=English
Input Above Character\nInput the character above caret.
.
Language=Japanese
1 行上の同じ位置の文字を挿入\n現在行の 1 行上の同じ位置にある文字を挿入します。
.

Facility=Command
SymbolicName=CMD_EDIT_CHARFROMBELOWLINE
Language=English
Input Below Character\nInput the character below caret.
.
Language=Japanese
1 行下の同じ位置の文字を挿入\n現在行の 1 行下の同じ位置にある文字を挿入します。
.

Facility=Command
SymbolicName=CMD_EDIT_TRANSPOSELINES
Language=English
Transpose Lines\nTranspose the current line and the previous line.
.
Language=Japanese
行の入れ替え\n前後の行を入れ替えます。
.

Facility=Command
SymbolicName=CMD_EDIT_TRANSPOSECHARS
Language=English
Transpose Characters\nTranspose the character before caret with the character after caret.
.
Language=Japanese
文字の入れ替え\n前後の文字を入れ替えます。
.

Facility=Command
SymbolicName=CMD_EDIT_TRANSPOSEWORDS
Language=English
Transpose Words\nTranspose the word before caret with the word after caret.
.
Language=Japanese
単語の入れ替え\n前後の単語を入れ替えます。
.

Facility=Command
SymbolicName=CMD_EDIT_SHOWABBREVIATIONDLG
Language=English
Manage A&bbreviations...\nManage all abbreviations.
.
Language=Japanese
短縮語句の管理(&B)...\n短縮語句の管理を行います。
.

Facility=Command
SymbolicName=CMD_EDIT_FIRSTCHAR
Language=English
First Character\nGo to the first non-whitespace character in the line.
.
Language=Japanese
最初の非空白類文字に移動\n現在行で最初に現れる非空白類文字に移動します。
.

Facility=Command
SymbolicName=CMD_EDIT_LASTCHAR
Language=English
Last Character\nGo to the last non-whitespace character in the line.
.
Language=Japanese
最後の非空白類文字に移動\n現在行で最初に現れる非空白類文字に移動します。
.

Facility=Command
SymbolicName=CMD_EDIT_FIRSTCHAREXTEND
Language=English
Extend to First Character\nExtend the selection to the first non-whitespace character in the line.
.
Language=Japanese
最初の非空白類文字まで選択を拡張\n現在行で最初に現れる非空白類文字まで選択を拡張します。
.

Facility=Command
SymbolicName=CMD_EDIT_LASTCHAREXTEND
Language=English
Extend to Last Character\nExtend the selection to the last non-whitespace character in the line.
.
Language=Japanese
最後の非空白類文字まで選択を拡張\n現在行で最後に現れる非空白類文字まで選択を拡張します。
.

Facility=Command
SymbolicName=CMD_EDIT_FIRSTCHARORLINEHOME
Language=English
First Character or Start of Line\nGo to the first non-whitespace character in the line or the start of the line.
.
Language=Japanese
行頭か最初の非空白類文字に移動\n行頭か現在行で最初に現れる非空白類文字に移動します。
.

Facility=Command
SymbolicName=CMD_EDIT_LASTCHARORLINEEND
Language=English
Last Character or End of Line\nGo to the last non-whitespace character in the line or the end of the line.
.
Language=Japanese
行末か最後の非空白類文字に移動\n行末か現在行で最後に現れる非空白類文字に移動します。
.

Facility=Command
SymbolicName=CMD_EDIT_CHARTOCODEPOINT
Language=English
Character to Code Point\nConvert a character to a corresponding code point.
.
Language=Japanese
文字をコードポイントに変換\nキャレット直前の文字をコードポイントに変換します。
.

Facility=Command
SymbolicName=CMD_EDIT_CODEPOINTTOCHAR
Language=English
Code Point to Character\nConvert a code point to a corresponding character.
.
Language=Japanese
コードポイントを文字に変換\nキャレット直前のコードポイントを文字に変換します。
.

Facility=Command
SymbolicName=CMD_EDIT_RECOMPOSE
Language=English
Reconvert\nReconvert the selection.
.
Language=Japanese
再変換\n選択範囲を未確定文字列として編集しなおします。
.

Facility=Command
SymbolicName=CMD_EDIT_TOGGLEOVERTYPEMODE
Language=English
Overtype Mode\nToggle insert/overtype mode.
.
Language=Japanese
挿入/上書きモードの切り替え\n入力方式の挿入モード、上書きモードを切り替えます。
.

Facility=Command
SymbolicName=CMD_EDIT_OPENCANDIDATE
Language=English
I&nput Candidates\nShow the candidates window or complete the word.
.
Language=Japanese
入力候補(&N)\n入力候補ウィンドウを表示、または単語を補完します。
.

Facility=Command
SymbolicName=CMD_EDIT_HOME
Language=English
Start of Document\nGo to the start of the document.
.
Language=Japanese
文書の先頭に移動\n文書の先頭に移動します。
.

Facility=Command
SymbolicName=CMD_EDIT_END
Language=English
End of Document\nGo to the end of the document.
.
Language=Japanese
文書の終端に移動\n文書の終端に移動します。
.

Facility=Command
SymbolicName=CMD_EDIT_LINEHOME
Language=English
Start of Line\nGo to the start of the line.
.
Language=Japanese
行頭に移動\n現在行の先頭に移動します。
.

Facility=Command
SymbolicName=CMD_EDIT_LINEEND
Language=English
End of Line\nGo to the end of the line.
.
Language=Japanese
行末に移動\n現在行の終端に移動します。
.

Facility=Command
SymbolicName=CMD_EDIT_CHARNEXT
Language=English
Next Character\nGo to the next character.
.
Language=Japanese
次の文字に移動\n次の文字に移動します。
.

Facility=Command
SymbolicName=CMD_EDIT_SCROLLCOLUMNNEXT
Language=English
Scroll Right\nScroll the window right one column right.
.
Language=Japanese
1 列右へスクロール\n1 列右へスクロールします。
.

Facility=Command
SymbolicName=CMD_EDIT_SCROLLCOLUMNPREV
Language=English
Scroll Left\nScroll the window left one column left.
.
Language=Japanese
1 列左へスクロール\n1 列左へスクロールします。
.

Facility=Command
SymbolicName=CMD_EDIT_ENSURECARETCENTER
Language=English
Recenter\nEnsure the caret center.
.
Language=Japanese
キャレットが中央になるまでスクロール\nキャレットがウィンドウの中央に表示されるようにスクロールします。
.

Facility=Command
SymbolicName=CMD_EDIT_ENSURECARETVISIBLE
Language=English
Show Caret\nEnsure the caret visible.
.
Language=Japanese
キャレットが可視になるまでスクロール\nキャレットがウィンドウ内に表示されるようにスクロールします。
.

Facility=Command
SymbolicName=CMD_EDIT_ROWCHARNEXT
Language=English
Extend Box to Next Character\nExtend the rectangle selection to the next character.
.
Language=Japanese
次の文字まで矩形選択\n次の文字まで選択を拡張し、矩形選択を開始します。
.

Facility=Command
SymbolicName=CMD_EDIT_ROWCHARPREV
Language=English
Extend Box to Previous Character\nExtend the rectangle selection to the previous character.
.
Language=Japanese
前の文字まで矩形選択\n前の文字まで選択を拡張し、矩形選択を開始します。
.

Facility=Command
SymbolicName=CMD_EDIT_ROWLINEDOWN
Language=English
Extend Box to Down Line\nExtend the rectangle selection to the one down line.
.
Language=Japanese
次の行まで矩形選択\n次の行まで選択を拡張し、矩形選択を開始します。
.

Facility=Command
SymbolicName=CMD_EDIT_ROWLINEUP
Language=English
Extend Box to Up Line\nExtend the rectangle selection to the one up line.
.
Language=Japanese
前の行まで矩形選択\n前の行まで選択を拡張し、矩形選択を開始します。
.

Facility=Command
SymbolicName=CMD_EDIT_ROWLINEEND
Language=English
Extend Box to End of Line\nExtend the rectangle selection to the end of the line.
.
Language=Japanese
行末まで矩形選択\n行末まで選択を拡張し、矩形選択を開始します。
.

Facility=Command
SymbolicName=CMD_EDIT_ROWLINEHOME
Language=English
Extend Box to Start of Line\nExtend the rectangle selection to the start of the line.
.
Language=Japanese
行頭まで矩形選択\n行頭まで選択を拡張し、矩形選択を開始します。
.

Facility=Command
SymbolicName=CMD_EDIT_ROWWORDNEXT
Language=English
Extend Box to Next Word\nExtend the rectangle selection to the next word.
.
Language=Japanese
次の単語の先頭まで矩形選択\n次の単語の先頭まで選択を拡張し、矩形選択を開始します。
.

Facility=Command
SymbolicName=CMD_EDIT_ROWWORDPREV
Language=English
Extend Box to Previuos Word\nExtend the rectangle selection to the previous word.
.
Language=Japanese
前の単語の先頭まで矩形選択\n前の単語の先頭まで選択を拡張し、矩形選択を開始します。
.

Facility=Command
SymbolicName=CMD_EDIT_ROWWORDENDNEXT
Language=English
Extend Box to Next Word End\nExtend the rectangle selection to the next word end.
.
Language=Japanese
次の単語の終端まで矩形選択\n次の単語の終端まで選択を拡張し、矩形選択を開始します。
.

Facility=Command
SymbolicName=CMD_EDIT_ROWWORDENDPREV
Language=English
Extend Box to Previous Word End\nExtend the rectangle selection to the previous word end.
.
Language=Japanese
前の単語の終端まで矩形選択\n前の単語の終端まで選択を拡張し、矩形選択を開始します。
.

Facility=Command
SymbolicName=CMD_EDIT_CHARPREV
Language=English
Previous Character\nGo to the previous character.
.
Language=Japanese
前の文字に移動\n前の文字に移動します。
.

Facility=Command
SymbolicName=CMD_EDIT_WORDENDNEXT
Language=English
Next Word End\nGo to the next word end.
.
Language=Japanese
次の単語の終端に移動\n次の単語の終端に移動します。
.

Facility=Command
SymbolicName=CMD_EDIT_WORDENDPREV
Language=English
Previous Word End\nGo to the previous word end.
.
Language=Japanese
前の単語の終端に移動\n前の単語の終端に移動します。
.

Facility=Command
SymbolicName=CMD_EDIT_WORDNEXT
Language=English
Next Word\nGo to the next word.
.
Language=Japanese
次の単語の先頭に移動\n次の単語の先頭に移動します。
.

Facility=Command
SymbolicName=CMD_EDIT_WORDPREV
Language=English
Previous Word\nGo to the previous word.
.
Language=Japanese
前の単語の先頭に移動\n前の単語の先頭に移動します。
.

Facility=Command
SymbolicName=CMD_EDIT_LINEDOWN
Language=English
Down Line\nGo to the one down line.
.
Language=Japanese
次の行に移動\n次の行に移動します。
.

Facility=Command
SymbolicName=CMD_EDIT_LINEUP
Language=English
Up Line\nGo to the one up line.
.
Language=Japanese
前の行に移動\n前の行に移動します。
.

Facility=Command
SymbolicName=CMD_EDIT_PAGEDOWN
Language=English
Down Page\nGo to the one down page.
.
Language=Japanese
次のページに移動\n次のページに移動します。
.

Facility=Command
SymbolicName=CMD_EDIT_PAGEUP
Language=English
Up Page\nGo to the one up page.
.
Language=Japanese
前のページに移動\n前のページに移動します。
.

Facility=Command
SymbolicName=CMD_EDIT_HOMEEXTEND
Language=English
Extend to Start of Document\nExtend the selection to the start of the document.
.
Language=Japanese
文書の先頭まで選択を拡張\n文書の先頭まで選択を拡張します。
.

Facility=Command
SymbolicName=CMD_EDIT_ENDEXTEND
Language=English
Extend to End of Document\nExtend the selection to the end of the document.
.
Language=Japanese
文書の終端まで選択を拡張\n文書の終端まで選択を拡張します。
.

Facility=Command
SymbolicName=CMD_EDIT_LINEHOMEEXTEND
Language=English
Extend to Start of Line\nExtend the selection to the start of the line.
.
Language=Japanese
行頭まで選択を拡張\n現在行の先頭まで選択を拡張します。
.

Facility=Command
SymbolicName=CMD_EDIT_LINEENDEXTEND
Language=English
Extend to End of Line\nExtend the selection to the end of the line.
.
Language=Japanese
行末まで選択を拡張\n現在行の終端まで選択を拡張します。
.

Facility=Command
SymbolicName=CMD_EDIT_CHARNEXT
Language=English
Extend to Next Character\nExtend the selection to the next character.
.
Language=Japanese
次の文字まで選択を拡張\n次の文字まで選択を拡張します。
.

Facility=Command
SymbolicName=CMD_EDIT_CHARPREV
Language=English
Extend to Previous Character\nExtend the selection to the previous character.
.
Language=Japanese
前の文字まで選択を拡張\n前の文字まで選択を拡張します。
.

Facility=Command
SymbolicName=CMD_EDIT_WORDENDNEXTEXTEND
Language=English
Extend to Next Word End\nExtend the selection to the next word end.
.
Language=Japanese
次の単語の終端まで選択を拡張\n次の単語の終端まで選択を拡張します。
.

Facility=Command
SymbolicName=CMD_EDIT_WORDENDPREVEXTEND
Language=English
Extend to Previous Word End\nExtend the selection to the previous word end.
.
Language=Japanese
前の単語の終端まで選択を拡張\n前の単語の終端まで選択を拡張します。
.

Facility=Command
SymbolicName=CMD_EDIT_WORDNEXTEXTEND
Language=English
Extend to Next Word\nExtend the selection to the next word.
.
Language=Japanese
次の単語の先頭まで選択を拡張\n次の単語の先頭まで選択を拡張します。
.

Facility=Command
SymbolicName=CMD_EDIT_WORDPREVEXTEND
Language=English
Extend to Previous Word\nExtend the selection to the previous word.
.
Language=Japanese
前の単語の先頭まで選択を拡張\n前の単語の先頭まで選択を拡張します。
.

Facility=Command
SymbolicName=CMD_EDIT_LINEDOWNEXTEND
Language=English
Extend to Next Line\nExtend the selection to the next line.
.
Language=Japanese
次の行まで選択を拡張\n次の行まで選択を拡張します。
.

Facility=Command
SymbolicName=CMD_EDIT_LINEUPEXTEND
Language=English
Extend to Previous Line\nExtend the selection to the previous line.
.
Language=Japanese
次の行まで選択を拡張\n次の行まで選択を拡張します。
.

Facility=Command
SymbolicName=CMD_EDIT_PAGEDOWNEXTEND
Language=English
Extend to Next Page\nExtend the selection to the next page.
.
Language=Japanese
次のページまで選択を拡張\n次のページまで選択を拡張します。
.

Facility=Command
SymbolicName=CMD_EDIT_PAGEUPEXTEND
Language=English
Extend to Previous Page\nExtend the selection to the previous page.
.
Language=Japanese
次のページまで選択を拡張\n次のページまで選択を拡張します。
.

Facility=Command
SymbolicName=CMD_EDIT_SELECTALL
Language=English
Select &All\nSelect the entire document.
.
Language=Japanese
すべて選択(&A)\n文書全体を選択します。
.

Facility=Command
SymbolicName=CMD_EDIT_SELECTCURRENTWORD
Language=English
Select Current Word\nSelect the current word.
.
Language=Japanese
現在の単語を選択\n現在位置の単語を選択します。
.

Facility=Command
SymbolicName=CMD_EDIT_CANCELSELECTION
Language=English
Cancel Selection\nCancel the selection.
.
Language=Japanese
選択を解除\n現在の選択を解除します。
.

Facility=Command
SymbolicName=CMD_EDIT_SCROLLHOME
Language=English
Scroll to Start\nScroll the window to the top.
.
Language=Japanese
先頭行へスクロール\n先頭行へスクロールします。
.

Facility=Command
SymbolicName=CMD_EDIT_SCROLLEND
Language=English
Scroll to End\nScroll the window to the bottom.
.
Language=Japanese
最終行へスクロール\n最終行へスクロールします。
.

Facility=Command
SymbolicName=CMD_EDIT_SCROLLLINEDOWN
Language=English
Scroll Down\nScroll the window one line down.
.
Language=Japanese
1 行下へスクロール\n1 行下へスクロールします。
.

Facility=Command
SymbolicName=CMD_EDIT_SCROLLLINEUP
Language=English
Scroll Up\nScroll the window one line up.
.
Language=Japanese
1 行上へスクロール\n1 行上へスクロールします。
.

Facility=Command
SymbolicName=CMD_EDIT_SCROLLPAGEDOWN
Language=English
Scroll Page Down\nScroll the window one page down.
.
Language=Japanese
1 ページ下へスクロール\n1 ページ下へスクロールします。
.

Facility=Command
SymbolicName=CMD_EDIT_SCROLLPAGEUP
Language=English
Scroll Page Up\nScroll the window one page up.
.
Language=Japanese
1 ページ上へスクロール\n1 ページ上へスクロールします。
.

Facility=Command
SymbolicName=CMD_SEARCH_TOP
Language=English
&Search
.
Language=Japanese
検索(&S)
.

Facility=Command
SymbolicName=CMD_SEARCH_BOOKMARKS
Language=English
&Bookmarks
.
Language=Japanese
ブックーマック(&B)
.

Facility=Command
SymbolicName=CMD_SEARCH_FIND
Language=English
&Find and Replace...\nShow [Search and Replace] dialog.
.
Language=Japanese
検索と置換(&F)...\n[検索と置換] ダイアログを表示します。
.

Facility=Command
SymbolicName=CMD_SEARCH_FINDNEXT
Language=English
Find &Next\nSearch next match.
.
Language=Japanese
次を検索(&N)\n検索条件に適合する次の文字列を選択します。
.

Facility=Command
SymbolicName=CMD_SEARCH_FINDPREV
Language=English
Find &Previous\nSearch previous match.
.
Language=Japanese
前を検索(&P)\n検索条件に適合する前の文字列を選択します。
.

Facility=Command
SymbolicName=CMD_SEARCH_REPLACEALLINTERACTIVE
Language=English
Replace and Next\nReplace the selection and search next.
.
Language=Japanese
置換して次に\n選択文字列を置換し次を検索します。
.

Facility=Command
SymbolicName=CMD_SEARCH_REPLACEALL
Language=English
Replace All\nReplace all matches.
.
Language=Japanese
すべて置換\n検索条件に適合するすべての文字列を置換します。
.

Facility=Command
SymbolicName=CMD_SEARCH_BOOKMARKALL
Language=English
Mark All\nSet bookmarks on all matched lines.
.
Language=Japanese
すべてマーク\n検索条件に適合する文字列が存在するすべての行にブックマークを設定します。
.

Facility=Command
SymbolicName=CMD_SEARCH_REVOKEMARK
Language=English
Revo&ke Highlight\nRevoke match highlight.
.
Language=Japanese
検索マークの解除(&K)\n検索条件に適合する文字列の強調表示を解除します。
.

Facility=Command
SymbolicName=CMD_SEARCH_GOTOLINE
Language=English
&Go to Line...\nGo to specified line.
.
Language=Japanese
指定行へ移動(&G)...\n指定した行へ移動します。
.

Facility=Command
SymbolicName=CMD_SEARCH_TOGGLEBOOKMARK
Language=English
T&oggle Bookmark\nSet or remove a bookmark on current line.
.
Language=Japanese
ブックマークの設定/解除(&O)\n現在行にブックマークを追加/削除します。
.

Facility=Command
SymbolicName=CMD_SEARCH_NEXTBOOKMARK
Language=English
Nex&t Bookmark\nGo to next bookmark.
.
Language=Japanese
次のブックマーク(&T)\n次のブックマークへ移動します。
.

Facility=Command
SymbolicName=CMD_SEARCH_PREVBOOKMARK
Language=English
Pre&vious Bookmark\nGo to previous bookmark.
.
Language=Japanese
前のブックマーク(&V)\n前のブックマークへ移動します。
.

Facility=Command
SymbolicName=CMD_SEARCH_CLEARBOOKMARKS
Language=English
&Clear All Bookmarks\nRemove all bookmarks in the document.
.
Language=Japanese
ブックマークのクリア(&C)\n現在の文書の全てのブックマークを解除します。
.

Facility=Command
SymbolicName=CMD_SEARCH_MANAGEBOOKMARKS
Language=English
Manage &Bookmarks...\nManage all bookmarks.
.
Language=Japanese
ブックマークの管理(&B)...\nブックマークの管理を行います。
.

Facility=Command
SymbolicName=CMD_SEARCH_GOTOMATCHBRACKET
Language=English
Go to &Match Brace\nGo to the match brace.
.
Language=Japanese
対括弧に移動(&M)\n現在位置の括弧に対応する括弧に移動します。
.

Facility=Command
SymbolicName=CMD_SEARCH_EXTENDTOMATCHBRACKET
Language=English
E&xtend Match Brace\nExtend the selection to the match brace.
.
Language=Japanese
対括弧まで選択を拡張(&X)\n現在位置の括弧に対応する括弧まで選択を拡張します。
.

Facility=Command
SymbolicName=CMD_SEARCH_INCREMENTALSEARCH
Language=English
&Incremental Search\nStart incremental search.
.
Language=Japanese
インクリメンタル検索(&I)\nインクリメンタル検索を行います。
.

Facility=Command
SymbolicName=CMD_SEARCH_INCREMENTALSEARCHR
Language=English
&Reverse Incremental Search\nStart reverse incremental search.
.
Language=Japanese
逆方向インクリメンタル検索(&R)\n逆方向インクリメンタル検索を行います。
.

Facility=Command
SymbolicName=CMD_SEARCH_INCREMENTALSEARCHRF
Language=English
R&egular Expression Incremental Search\nStart regular expression incremental search.
.
Language=Japanese
正規表現インクリメンタル検索(&E)\n正規表現インクリメンタル検索を行います。
.

Facility=Command
SymbolicName=CMD_SEARCH_INCREMENTALSEARCHRR
Language=English
Reverse Regular Expression Incremental Search\nStart reverse regular expression incremental search.
.
Language=Japanese
逆方向正規表現インクリメンタル検索(&U)\n逆方向正規表現インクリメンタル検索を行います。
.

Facility=Command
SymbolicName=CMD_SEARCH_INCREMENTALSEARCHMF
Language=English
&Migemo Incremental Search\nStart Migemo incremental search.
.
Language=Japanese
Migemo インクリメンタル検索(&M)\nMigemo インクリメンタル検索を行います。
.

Facility=Command
SymbolicName=CMD_SEARCH_INCREMENTALSEARCHMR
Language=English
Reverse Migem&o Incremental Search\nStart reverse Migemo incremental search.
.
Language=Japanese
逆方向 Migemo インクリメンタル検索(&O)\n逆方向 Migemo インクリメンタル検索を行います。
.

Facility=Command
SymbolicName=CMD_SEARCH_FINDFILES
Language=English
Find Files...\nFind files with the given pattern.
.
Language=Japanese
ファイルの検索...\nファイルを検索します。
.

Facility=Command
SymbolicName=CMD_SEARCH_SEARCHMULTIPLEFILES
Language=English
Search in Files...\nSearch pattern in multiple files.
.
Language=Japanese
ファイルから検索...\n複数のファイルから検索を行います。
.

Facility=Command
SymbolicName=CMD_SEARCH_REPLACEMULTIPLEFILES
Language=English
Replace in Files...\nReplace text in multiple files.
.
Language=Japanese
ファイルから置換...\n複数のファイルで置換を行います。
.

Facility=Command
SymbolicName=CMD_WINDOW_TOP
Language=English
&Window
.
Language=Japanese
ウィンドウ(&W)
.

Facility=Command
SymbolicName=CMD_VIEW_TOP
Language=English
&View
.
Language=Japanese
表示(&V)
.

Facility=Command
SymbolicName=CMD_VIEW_BUFFERS
Language=English
Bu&ffers
.
Language=Japanese
バッファ(&F)
.

Facility=Command
SymbolicName=CMD_VIEW_TOOLBAR
Language=English
&Toolbar\nShow or hide the toolbar.
.
Language=Japanese
ツールバー(&T)\nツールバーの表示/非表示を切り替えます。
.

Facility=Command
SymbolicName=CMD_VIEW_STATUSBAR
Language=English
&Status Bar\nShow or hide the status bar.
.
Language=Japanese
ステータスバー(&S)\nステータスバーの表示/非表示を切り替えます。
.

Facility=Command
SymbolicName=CMD_VIEW_WRAPNO
Language=English
N&o Wrap\nDo not wrap lines.
.
Language=Japanese
折り返さない(&O)\n行の折り返し表示を行いません。
.

Facility=Command
SymbolicName=CMD_VIEW_WRAPBYSPECIFIEDWIDTH
Language=English
Wrap by Sp&ecified Width(&E)\nWrap lines at specified position.
.
Language=Japanese
指定幅で折り返す(&E)\n行を指定した幅で折り返して表示します。
.

Facility=Command
SymbolicName=CMD_VIEW_WRAPBYWINDOWWIDTH
Language=English
Wrap by &Window\nWrap lines at the end of window.
.
Language=Japanese
ウィンドウ端で折り返す(&W)\n行をウィンドウ幅で折り返して表示します。
.

Facility=Command
SymbolicName=CMD_VIEW_REFRESH
Language=English
&Refresh\nRefresh the window.
.
Language=Japanese
再描画(&R)\n画面を再描画します。
.

Facility=Command
SymbolicName=CMD_VIEW_NEXTBUFFER
Language=English
&Next Buffer\nShow next buffer.
.
Language=Japanese
次のバッファ(&N)\n次のバッファを表示します。
.

Facility=Command
SymbolicName=CMD_VIEW_PREVBUFFER
Language=English
&Previous Buffer\nShow previous buffer.
.
Language=Japanese
前のバッファ(&P)\n前のバッファを表示します。
.

Facility=Command
SymbolicName=CMD_VIEW_BUFFERBAR
Language=English
&Buffer Bar\nShow or hide the buffer bar.
.
Language=Japanese
バッファバー(&B)\nバッファバーの表示/非表示を切り替えます。
.

Facility=Command
SymbolicName=CMD_WINDOW_SPLITNS
Language=English
Sp&lit Window\nSplit the window into up and bottom.
.
Language=Japanese
ウィンドウの上下分割(&L)\nエディタウィンドウを上下に分割します。
.

Facility=Command
SymbolicName=CMD_WINDOW_SPLITWE
Language=English
Split Window Si&de-by-Side\nSplit the window into side-by-side.
.
Language=Japanese
ウィンドウの左右分割(&D)\nエディタウィンドウを左右に分割します。
.

Facility=Command
SymbolicName=CMD_WINDOW_UNSPLITACTIVE
Language=English
&Close Window\nClose active window.
.
Language=Japanese
現在のウィンドウを閉じる(&C)\nアクティブなエディタウィンドウを閉じ、分割を解除します。
.

Facility=Command
SymbolicName=CMD_WINDOW_UNSPLITOTHERS
Language=English
Close Ot&her Windows\nClose all inactive windows.
.
Language=Japanese
他のウィンドウを閉じる(&H)\nアクティブなエディタウィンドウを残し、すべてのウィンドウを閉じます。
.

Facility=Command
SymbolicName=CMD_WINDOW_NEXTPANE
Language=English
Ne&xt Pane\nActivate next window.
.
Language=Japanese
次のウィンドウ(&X)\n次のエディタウィンドウをアクティブにします。
.

Facility=Command
SymbolicName=CMD_WINDOW_PREVPANE
Language=English
Pre&vious Pane\nActivate previous window.
.
Language=Japanese
前のウィンドウ(&V)\n前のエディタウィンドウをアクティブにします。
.

Facility=Command
SymbolicName=CMD_WINDOW_TOPMOSTALWAYS
Language=English
&Always in Foreground\nShow the application window foreground always.
.
Language=Japanese
常に最前面に表示(&A)\nウィンドウを常に最前面に表示します。
.

Facility=Command
SymbolicName=CMD_MACRO_TOP
Language=English
&Macros
.
Language=Japanese
マクロ(&M)
.

Facility=Command
SymbolicName=CMD_MACRO_SCRIPTS
Language=English
&Scripts
.
Language=Japanese
スクリプト(&S)
.

Facility=Command
SymbolicName=CMD_MACRO_DEFINE
Language=English
Start/End &Definition\nStart or end the keyboard macro definition.
.
Language=Japanese
記録の開始/終了(&D)\nキーボードマクロの記録を開始/終了します。
.

Facility=Command
SymbolicName=CMD_MACRO_EXECUTE
Language=English
E&xecute\nExecute the active keyboard macro.
.
Language=Japanese
実行(&X)\nアクティブなキーボードマクロを実行します。
.

Facility=Command
SymbolicName=CMD_MACRO_APPEND
Language=English
&Append\nAppend definition to the active keyboard macro.
.
Language=Japanese
追加記録の開始(&A)\nアクティブなキーボードマクロに追加記録を開始します。
.

Facility=Command
SymbolicName=CMD_MACRO_PAUSERESTART
Language=English
&Pause/Restart\nPause or restart the keyboard macro definition.
.
Language=Japanese
記録の一時停止/再開(&P)\nキーボードマクロの記録を一時停止/再開します。
.

Facility=Command
SymbolicName=CMD_MACRO_INSERTQUERY
Language=English
Insert &Query\nInsert an user prompt into the defining keyboard macro.
.
Language=Japanese
プロンプトの挿入(&Q)\n記録中のキーボードマクロにユーザプロンプトを挿入します。
.

Facility=Command
SymbolicName=CMD_MACRO_ABORT
Language=English
A&bort\nAbort the keyboard macro definition.
.
Language=Japanese
記録の中止(&B)\nキーボードマクロの記録を中止します。
.

Facility=Command
SymbolicName=CMD_MACRO_SAVEAS
Language=English
Sa&ve As...\nSave the active keyboard macro with name.
.
Language=Japanese
名前を付けて保存(&V)...\nアクティブなキーボードマクロに名前を付けて保存します。
.

Facility=Command
SymbolicName=CMD_MACRO_LOAD
Language=English
&Load...\nLoad keyboard macro from a file.
.
Language=Japanese
読み込み(&L)...\nキーボードマクロをファイルから読み込みます。
.

Facility=Command
SymbolicName=CMD_TOOL_TOP
Language=English
&Tool
.
Language=Japanese
ツール(&T)
.

Facility=Command
SymbolicName=CMD_TOOL_APPDOCTYPES
Language=English
&Apply Document Type
.
Language=Japanese
適用文書タイプ(&A)
.

Facility=Command
SymbolicName=CMD_TOOL_COMMONOPTION
Language=English
&Common Options...\nSet the options common to all document types.
.
Language=Japanese
共通設定(&C)...\nすべての文書タイプに共通のオプションを設定します。
.

Facility=Command
SymbolicName=CMD_TOOL_DOCTYPEOPTION
Language=English
Document T&ype Options...\nSet the options specific to the active document type.
.
Language=Japanese
文書タイプ別設定(&Y)...\n文書タイプ別のオプションを設定します。
.

Facility=Command
SymbolicName=CMD_TOOL_FONT
Language=English
&Font...\nSet font settings.
.
Language=Japanese
フォント設定(&F)...\nフォントの設定を行います。
.

Facility=Command
SymbolicName=CMD_TOOL_EXECUTE
Language=English
&Execute\nExecute document type specific program.
.
Language=Japanese
実行(&E)\n現在のファイル名を引数にして文書タイプ指定のプログラムを実行します。
.

Facility=Command
SymbolicName=CMD_TOOL_EXECUTECOMMAND
Language=English
E&xecute Command...\nExecute external command.
.
Language=Japanese
コマンドの実行(&X)...\n外部コマンドを実行します。
.

Facility=Command
SymbolicName=CMD_HELP_TOP
Language=English
&Help
.
Language=Japanese
ヘルプ(&H)
.

Facility=Command
SymbolicName=CMD_HELP_ABOUT
Language=English
&About\nDisplay information and version of Alpha.
.
Language=Japanese
バージョン情報(&A)\nAlpha の情報、バージョンを表示します。
.


;// encoding names

MessageId=37
Facility=Encoding
Language=English
US-Canada (IBM EBCDIC)
.
Language=Japanese
米国/カナダ (IBM EBCDIC)
.

MessageId=437
Facility=Encoding
Language=English
United States (ibm-437)
.
Language=Japanese
米国 (ibm-437)
.

MessageId=500
Facility=Encoding
Language=English
International (IBM EBCDIC)
.
Language=Japanese
インターナショナル (IBM EBCDIC)
.

MessageId=708
Facility=Encoding
Language=English
Arabic (ASMO)
.
Language=Japanese
アラビア語 (ASMO)
.

MessageId=709
Facility=Encoding
Language=English
Arabic (ASMO 449+, BCON V4)
.
Language=Japanese
アラビア語 (ASMO 449+, BCON V4)
.

MessageId=710
Facility=Encoding
Language=English
Arabic (Transparent Arabic)
.
Language=Japanese
アラビア語 (Transparent Arabic)
.

MessageId=720
Facility=Encoding
Language=English
Arabic (Transparent ASMO)
.
Language=Japanese
アラビア語 (Transparent ASMO)
.

MessageId=737
Facility=Encoding
Language=English
Greek (437G, ibm-737)
.
Language=Japanese
ギリシャ語 (437G, ibm-737)
.

MessageId=775
Facility=Encoding
Language=English
Baltic (ibm-775)
.
Language=Japanese
バルト言語 (ibm-775)
.

MessageId=850
Facility=Encoding
Language=English
Western European (ibm-850)
.
Language=Japanese
西ヨーロッパ (ibm-850)
.

MessageId=852
Facility=Encoding
Language=English
Central European (ibm-852)
.
Language=Japanese
中央ヨーロッパ (ibm-852)
.

MessageId=855
Facility=Encoding
Language=English
Cyrillic (ibm-855)
.
Language=Japanese
キリル言語 (ibm-855)
.

MessageId=857
Facility=Encoding
Language=English
Turkish (ibm-857)
.
Language=Japanese
トルコ語 (ibm-857)
.

MessageId=858
Facility=Encoding
Language=English
Multilingual Latin 1 + European (ibm-858)
.
Language=Japanese
多言語ラテン I + ヨーロッパ言語 (ibm-858)
.

MessageId=860
Facility=Encoding
Language=English
Portuguese (ibm-860)
.
Language=Japanese
ポルトガル語 (ibm-860)
.

MessageId=861
Facility=Encoding
Language=English
Icelandic (ibm-861)
.
Language=Japanese
アイスランド語 (ibm-861)
.

MessageId=862
Facility=Encoding
Language=English
Hebrew (ibm-862)
.
Language=Japanese
ヘブライ語 (ibm-862)
.

MessageId=863
Facility=Encoding
Language=English
Canada-France (ibm-863)
.
Language=Japanese
カナダ/フランス語 (ibm-863)
.

MessageId=864
Facility=Encoding
Language=English
Arabic (ibm-864)
.
Language=Japanese
アラビア語 (ibm-864)
.

MessageId=865
Facility=Encoding
Language=English
Northern European (ibm-865)
.
Language=Japanese
北欧 (ibm-865)
.

MessageId=866
Facility=Encoding
Language=English
Russian (ibm-866)
.
Language=Japanese
ロシア語 (ibm-866)
.

MessageId=869
Facility=Encoding
Language=English
Modern Greek (ibm-869)
.
Language=Japanese
現代ギリシャ語 (ibm-869)
.

MessageId=870
Facility=Encoding
Language=English
Multilingual-RORCE (Latin-2, IBM EBCDIC)
.
Language=Japanese
マルチリンガル/ROECE (ラテン-2, IBM EBCDIC)
.

MessageId=874
Facility=Encoding
Language=English
Thai (ibm-874)
.
Language=Japanese
タイ語 (ibm-874)
.

MessageId=875
Facility=Encoding
Language=English
Modern Greek (IBM EBCDIC)
.
Language=Japanese
現代ギリシャ語 (IBM EBCDIC)
.

MessageId=932
Facility=Encoding
Language=English
Japanese (Shift JIS, windows-932)
.
Language=Japanese
日本語 (シフト JIS, windows-932)
.

MessageId=936
Facility=Encoding
Language=English
Simplified Chinese (GBK, windows-936)
.
Language=Japanese
簡体字中国語 (GBK, windows-936)
.

MessageId=949
Facility=Encoding
Language=English
Korean (windows-949)
.
Language=Japanese
韓国語 (windows-949)
.

MessageId=950
Facility=Encoding
Language=English
Traditional Chinese (Big5, windows-950)
.
Language=Japanese
繁体字中国語 (Big5, windows-950)
.

MessageId=1026
Facility=Encoding
Language=English
Turkish (IBM EBCDIC)
.
Language=Japanese
トルコ語 (IBM EBCDIC)
.

MessageId=1047
Facility=Encoding
Language=English
Latin-1/Open System (IBM EBCDIC)
.
Language=Japanese
ラテン-1/Open System (IBM EBCDIC)
.

MessageId=1140
Facility=Encoding
Language=English
US-Canada (37 + Euro, IBM EBCDIC)
.
Language=Japanese
米国/カナダ (37 + ユーロ, IBM EBCDIC)
.

MessageId=1141
Facility=Encoding
Language=English
Germany (20273 + Euro, IBM EBCDIC)
.
Language=Japanese
ドイツ (20273 + ユーロ, IBM EBCDIC)
.

MessageId=1142
Facility=Encoding
Language=English
Denmark-Norway (20277 + Euro, IBM EBCDIC)
.
Language=Japanese
デンマーク/ノルウェー (20277 + ユーロ, IBM EBCDIC)
.

MessageId=1143
Facility=Encoding
Language=English
Finland-Sweden (20278 + Euro, IBM EBCDIC)
.
Language=Japanese
フィンランド/スウェーデン (20278 + ユーロ, IBM EBCDIC)
.

MessageId=1144
Facility=Encoding
Language=English
Italy (20280 + Euro, IBM EBCDIC)
.
Language=Japanese
イタリア (20280 + ユーロ, IBM EBCDIC)
.

MessageId=1145
Facility=Encoding
Language=English
Latin America-Spain (20284 + Euro, IBM EBCDIC)
.
Language=Japanese
ラテン アメリカ言語/スペイン (20284 + ユーロ, IBM EBCDIC)
.

MessageId=1146
Facility=Encoding
Language=English
UK (20285 + Euro, IBM EBCDIC)
.
Language=Japanese
英国 (20285 + ユーロ, IBM EBCDIC)
.

MessageId=1147
Facility=Encoding
Language=English
France (20297 + Euro, IBM EBCDIC)
.
Language=Japanese
フランス (20297 + ユーロ, IBM EBCDIC)
.

MessageId=1148
Facility=Encoding
Language=English
International (500 + Euro, IBM EBCDIC)
.
Language=Japanese
インターナショナル (500 + ユーロ, IBM EBCDIC)
.

MessageId=1149
Facility=Encoding
Language=English
Icelandic (20871 + Euro, IBM EBCDIC)
.
Language=Japanese
アイスランド語 (20871 + ユーロ, IBM EBCDIC)
.

MessageId=1200
Facility=Encoding
Language=English
Unicode (UTF-16)
.
Language=Japanese
Unicode (UTF-16)
.

MessageId=1201
Facility=Encoding
Language=English
Unicode (UTF-16 big endian)
.
Language=Japanese
Unicode (UTF-16 big endian)
.

MessageId=1250
Facility=Encoding
Language=English
Central European (windows-1250)
.
Language=Japanese
中央ヨーロッパ (windows-1250)
.

MessageId=1251
Facility=Encoding
Language=English
Cyrillic (windows-1251)
.
Language=Japanese
キリル言語 (windows-1251)
.

MessageId=1252
Facility=Encoding
Language=English
Western European (windows-1252)
.
Language=Japanese
西ヨーロッパ (windows-1252)
.

MessageId=1253
Facility=Encoding
Language=English
Greek (windows-1253)
.
Language=Japanese
ギリシャ語 (windows-1253)
.

MessageId=1254
Facility=Encoding
Language=English
Turkish (windows-1254)
.
Language=Japanese
トルコ語 (windows-1254)
.

MessageId=1255
Facility=Encoding
Language=English
Hebrew (windows-1255)
.
Language=Japanese
ヘブライ語 (windows-1255)
.

MessageId=1256
Facility=Encoding
Language=English
Arabic (windows-1256)
.
Language=Japanese
アラビア語 (windows-1256)
.

MessageId=1257
Facility=Encoding
Language=English
Baltic (windows-1257)
.
Language=Japanese
バルト言語 (windows-1257)
.

MessageId=1258
Facility=Encoding
Language=English
Vietnamese (windows-1258)
.
Language=Japanese
ベトナム語 (windows-1258)
.

MessageId=1361
Facility=Encoding
Language=English
Korean (Johab)
.
Language=Japanese
韓国語 (Johab)
.

MessageId=10000
Facility=Encoding
Language=English
Roman (Macintosh)
.
Language=Japanese
ローマン (Macintosh)
.

MessageId=10001
Facility=Encoding
Language=English
Japanese (Macintosh)
.
Language=Japanese
日本語 (Macintosh)
.

MessageId=10002
Facility=Encoding
Language=English
Traditional Chinese (Macintosh)
.
Language=Japanese
繁体字中国語 (Big5, Macintosh)
.

MessageId=10003
Facility=Encoding
Language=English
Korean (Macintosh)
.
Language=Japanese
韓国語 (Macintosh)
.

MessageId=10004
Facility=Encoding
Language=English
Arabic (Macintosh)
.
Language=Japanese
アラビア語 (Macintosh)
.

MessageId=10005
Facility=Encoding
Language=English
Hebrew (Macintosh)
.
Language=Japanese
ヘブライ語 (Macintosh)
.

MessageId=10006
Facility=Encoding
Language=English
Greek I (Macintosh)
.
Language=Japanese
ギリシャ語 I (Macintosh)
.

MessageId=10007
Facility=Encoding
Language=English
Cyrillic (Macintosh)
.
Language=Japanese
キリル言語 (Macintosh)
.

MessageId=10008
Facility=Encoding
Language=English
Simplified Chinese (GB2312, Macintosh)
.
Language=Japanese
簡体字中国語 (GB2312, Macintosh)
.

MessageId=10010
Facility=Encoding
Language=English
Rumanian (Macintosh)
.
Language=Japanese
ルーマニア語 (Macintosh)
.

MessageId=10017
Facility=Encoding
Language=English
Ukrainian (Macintosh)
.
Language=Japanese
ウクライナ語 (Macintosh)
.

MessageId=10021
Facility=Encoding
Language=English
Thai (Macintosh)
.
Language=Japanese
タイ語 (Macintosh)
.

MessageId=10029
Facility=Encoding
Language=English
Central European (Macintosh)
.
Language=Japanese
中央ヨーロッパ (Macintosh)
.

MessageId=10079
Facility=Encoding
Language=English
Icelandic (Macintosh)
.
Language=Japanese
アイスランド語 (Macintosh)
.

MessageId=10081
Facility=Encoding
Language=English
Turkish (Macintosh)
.
Language=Japanese
トルコ語 (Macintosh)
.

MessageId=10082
Facility=Encoding
Language=English
Croatian (Macintosh)
.
Language=Japanese
クロアチア語 (Macintosh)
.

MessageId=12000
Facility=Encoding
Language=English
Unicode (UTF-32)
.
Language=Japanese
Unicode (UTF-32)
.

MessageId=12001
Facility=Encoding
Language=English
Unicode (UTF-32 big endian)
.
Language=Japanese
Unicode (UTF-32 big endian)
.

MessageId=20000
Facility=Encoding
Language=English
Taiwanese (CNS)
.
Language=Japanese
台湾 (CNS)
.


MessageId=20001
Facility=Encoding
Language=English
Taiwanese (TCA)
.
Language=Japanese
台湾 (TCA)
.

MessageId=20002
Facility=Encoding
Language=English
Taiwanese (Eten)
.
Language=Japanese
台湾 (Eten)
.

MessageId=20003
Facility=Encoding
Language=English
Taiwanese (IBM5550)
.
Language=Japanese
台湾 (IBM5550)
.

MessageId=20004
Facility=Encoding
Language=English
Taiwanese (TeleText)
.
Language=Japanese
台湾 (TeleText)
.

MessageId=20005
Facility=Encoding
Language=English
Taiwanese (Wang)
.
Language=Japanese
台湾 (Wang)
.

MessageId=20105
Facility=Encoding
Language=English
IRV International Alphabets No.5 (IA5)
.
Language=Japanese
IRV インターナショナル アルファベット No.5 (IA5)
.

MessageId=20106
Facility=Encoding
Language=English
German (IA5)
.
Language=Japanese
ドイツ語 (IA5)
.

MessageId=20107
Facility=Encoding
Language=English
Swedish (IA5)
.
Language=Japanese
スウェーデン語 (IA5)
.

MessageId=20108
Facility=Encoding
Language=English
Norwegian (IA5)
.
Language=Japanese
ノルウェー語 (IA5)
.

MessageId=20127
Facility=Encoding
Language=English
US-ASCII
.
Language=Japanese
US-ASCII
.

MessageId=20261
Facility=Encoding
Language=English
T.61
.
Language=Japanese
T.61
.

MessageId=20269
Facility=Encoding
Language=English
Non-Spacing Accent (ISO 6937)
.
Language=Japanese
Non-Spacing Accent (ISO 6937)
.

MessageId=20273
Facility=Encoding
Language=English
Germany (IBM EBCDIC)
.
Language=Japanese
ドイツ (IBM EBCDIC)
.

MessageId=20277
Facility=Encoding
Language=English
Denmark-Norway (IBM EBCDIC)
.
Language=Japanese
デンマーク/ノルウェー (IBM EBCDIC)
.

MessageId=20278
Facility=Encoding
Language=English
Finland-Sweden (IBM EBCDIC)
.
Language=Japanese
フィンランド/スウェーデン (IBM EBCDIC)
.

MessageId=20280
Facility=Encoding
Language=English
Italy (IBM EBCDIC)
.
Language=Japanese
イタリア (IBM EBCDIC)
.

MessageId=20284
Facility=Encoding
Language=English
Latin American-Spain (IBM EBCDIC)
.
Language=Japanese
ラテン アメリカ言語/スペイン (IBM EBCDIC)
.

MessageId=20285
Facility=Encoding
Language=English
UK (IBM EBCDIC)
.
Language=Japanese
英国 (IBM EBCDIC)
.

MessageId=20290
Facility=Encoding
Language=English
Japanese (Katakana, IBM EBCDIC)
.
Language=Japanese
日本語 (カタカナ拡張, IBM EBCDIC)
.

MessageId=20297
Facility=Encoding
Language=English
France (IBM EBCDIC)
.
Language=Japanese
フランス (IBM EBCDIC)
.

MessageId=20420
Facility=Encoding
Language=English
Arabic (IBM EBCDIC)
.
Language=Japanese
アラビア語 (IBM EBCDIC)
.

MessageId=20423
Facility=Encoding
Language=English
Greek (IBM EBCDIC)
.
Language=Japanese
ギリシャ語 (IBM EBCDIC)
.

MessageId=20424
Facility=Encoding
Language=English
Hebrew (IBM EBCDIC)
.
Language=Japanese
ヘブライ語 (IBM EBCDIC)
.

MessageId=20833
Facility=Encoding
Language=English
Korean and Korean Extended (IBM EBCDIC)
.
Language=Japanese
韓国語拡張 (IBM EBCDIC)
.

MessageId=20838
Facility=Encoding
Language=English
Thai (IBM EBCDIC)
.
Language=Japanese
タイ語 (IBM EBCDIC)
.

MessageId=20866
Facility=Encoding
Language=English
Russian (KOI8)
.
Language=Japanese
ロシア語 (KOI8)
.

MessageId=20871
Facility=Encoding
Language=English
Icelandic (IBM EBCDIC)
.
Language=Japanese
アイスランド語 (IBM EBCDIC)
.

MessageId=20880
Facility=Encoding
Language=English
Cyrillic (Russian, IBM EBCDIC)
.
Language=Japanese
キリル文字 (ロシア語, IBM EBCDIC)
.

MessageId=20905
Facility=Encoding
Language=English
Turkish (IBM EBCDIC)
.
Language=Japanese
トルコ語 (IBM EBCDIC)
.

MessageId=20924
Facility=Encoding
Language=English
Latin-1/Open System (1047 + Euro, IBM EBCDIC)
.
Language=Japanese
ラテン-1/Open System (1047 + ユーロ, IBM EBCDIC)
.

MessageId=20932
Facility=Encoding
Language=English
Japanese (JIS X 0208-1990 & 0212-1990, windows-20932)
.
Language=Japanese
日本語 (JIS X 0208-1990 & 0212-1990, windows-20932)
.

MessageId=20936
Facility=Encoding
Language=English
Simplified Chinese (GB2312)
.
Language=Japanese
簡体字中国語 (GB2312)
.

MessageId=21025
Facility=Encoding
Language=English
Cyrillic (Serbian-Bulgarian, IBM EBCDIC)
.
Language=Japanese
キリル文字 (セルビア語、ブルガリア語, IBM EBCDIC)
.

MessageId=21027
Facility=Encoding
Language=English
Ext Alpha Lowercase
.
Language=Japanese
Ext Alpha Lowercase
.

MessageId=21866
Facility=Encoding
Language=English
Ukrainian (KOI8-U)
.
Language=Japanese
ウクライナ語 (KOI8-U)
.

MessageId=28591
Facility=Encoding
Language=English
Western European (ISO-8859-1)
.
Language=Japanese
西ヨーロッパ (ISO-8859-1)
.

MessageId=28592
Facility=Encoding
Language=English
Central European (ISO-8859-2)
.
Language=Japanese
中央ヨーロッパ (ISO-8859-2)
.

MessageId=28593
Facility=Encoding
Language=English
Southern European (ISO-8859-3)
.
Language=Japanese
南ヨーロッパ (ISO-8859-3)
.

MessageId=28594
Facility=Encoding
Language=English
Baltic (ISO-8859-4)
.
Language=Japanese
バルト言語 (ISO-8859-4)
.

MessageId=28595
Facility=Encoding
Language=English
Cyrillic (ISO-8859-5)
.
Language=Japanese
キリル言語 (ISO-8859-5)
.

MessageId=28596
Facility=Encoding
Language=English
Arabic (ISO-8859-6)
.
Language=Japanese
アラビア語 (ISO-8859-6)
.

MessageId=28597
Facility=Encoding
Language=English
Greek (ISO-8859-7)
.
Language=Japanese
ギリシャ語 (ISO-8859-7)
.

MessageId=28598
Facility=Encoding
Language=English
Hebrew (ISO-8859-8-Visual)
.
Language=Japanese
ヘブライ語 (ISO-8859-8, 視覚順)
.

MessageId=28599
Facility=Encoding
Language=English
Turkish (ISO-8859-9)
.
Language=Japanese
トルコ語 (ISO-8859-9)
.

MessageId=28600
Facility=Encoding
Language=English
Northern European (ISO-8859-10)
.
Language=Japanese
北欧 (ISO-8859-10)
.

MessageId=28601
Facility=Encoding
Language=English
Thai (ISO-8859-11)
.
Language=Japanese
タイ語 (ISO-8859-11)
.

MessageId=28603
Facility=Encoding
Language=English
Baltic (ISO-8859-13)
.
Language=Japanese
バルト言語 (ISO-8859-13)
.

MessageId=28604
Facility=Encoding
Language=English
Keltic (ISO-8859-14)
.
Language=Japanese
ケルト語 (ISO-8859-14)
.

MessageId=28605
Facility=Encoding
Language=English
Western European (ISO-8859-15)
.
Language=Japanese
西ヨーロッパ (ISO-8859-15)
.

MessageId=28606
Facility=Encoding
Language=English
Central European (ISO-8859-16)
.
Language=Japanese
中央ヨーロッパ (ISO-8859-16)
.

MessageId=29001
Facility=Encoding
Language=English
Europa 3
.
Language=Japanese
ヨーロッパ 3
.

MessageId=38598
Facility=Encoding
Language=English
Hebrew (ISO-8859-8-Logical)
.
Language=Japanese
ヘブライ語 (ISO-8859-8, 論理順)
.

MessageId=50001
Facility=Encoding
Language=English
Auto-Select
.
Language=Japanese
自動選択
.

MessageId=50220
Facility=Encoding
Language=English
Japanese (ISO-2022-JP, windows-50220)
.
Language=Japanese
日本語 (ISO-2022-JP, 半角カタカナなし, windows-50220)
.

MessageId=50221
Facility=Encoding
Language=English
Japanese (ISO-2022-JP-Allow 1 byte Kana, windows-50221)
.
Language=Japanese
日本語 (ISO-2022-JP, 半角カタカナ, windows-50221)
.

MessageId=50222
Facility=Encoding
Language=English
Japanese (ISO-2022-JP, JIS X 0201-1989, windows-50222)
.
Language=Japanese
日本語 (ISO-2022-JP, JIS X 0201-1989, windows-50222)
.

MessageId=50225
Facility=Encoding
Language=English
Korean (ISO-2022-KR)
.
Language=Japanese
韓国語 (ISO-2022-KR)
.

MessageId=50227
Facility=Encoding
Language=English
Simplified Chinese (ISO-2022-CN)
.
Language=Japanese
簡体字中国語 (ISO-2022-CN)
.

MessageId=50229
Facility=Encoding
Language=English
Traditional Chinese (ISO-2022-CN)
.
Language=Japanese
繁体字中国語 (ISO-2022-CN)
.

MessageId=50930
Facility=Encoding
Language=English
Japanese (Katakana Extended)
.
Language=Japanese
日本語 (カタカナ拡張)
.

MessageId=50931
Facility=Encoding
Language=English
Japanese + US-Canada
.
Language=Japanese
日本語 + 米国/カナダ
.

MessageId=50932
Facility=Encoding
Language=English
Japanese (Auto-Select)
.
Language=Japanese
日本語 (自動選択)
.

MessageId=50933
Facility=Encoding
Language=English
Korean + Korean Extended
.
Language=Japanese
韓国語 + 韓国語拡張
.

MessageId=50935
Facility=Encoding
Language=English
Simplified Chinese + Simplified Chinese Extended
.
Language=Japanese
簡体字中国語 + 簡体字中国語拡張
.

MessageId=50936
Facility=Encoding
Language=English
Simplified Chinese
.
Language=Japanese
簡体字中国語
.

MessageId=50937
Facility=Encoding
Language=English
Simplified Chinese + US-Canada
.
Language=Japanese
繁体字中国語 + 米国/カナダ
.

MessageId=50939
Facility=Encoding
Language=English
Japanese + Japanese Latin Extended
.
Language=Japanese
日本語 + 日本語ラテン拡張
.

MessageId=50949
Facility=Encoding
Language=English
Korean (Auto-Select)
.
Language=Japanese
韓国語 (自動選択)
.

MessageId=51932
Facility=Encoding
Language=English
Japanese (EUC, windows-51932)
.
Language=Japanese
日本語 (EUC, windows-51932)
.

MessageId=51936
Facility=Encoding
Language=English
Simplified Chinese (EUC, windows-51936)
.
Language=Japanese
簡体字中国語 (EUC, windows-51936)
.

MessageId=51949
Facility=Encoding
Language=English
Korean (EUC, windows-51949)
.
Language=Japanese
韓国語 (EUC, windows-51949)
.

MessageId=51950
Facility=Encoding
Language=English
Traditional Chinese (EUC, windows-51950)
.
Language=Japanese
繁体字中国語 (EUC, windows-51950)
.

MessageId=52936
Facility=Encoding
Language=English
Simplified Chinese (HZ-GB2312)
.
Language=Japanese
簡体字中国語 (HZ-GB2312)
.

MessageId=54936
Facility=Encoding
Language=English
Simplified Chinese (GB18030)
.
Language=Japanese
簡体字中国語 (GB18030)
.

MessageId=57002
Facility=Encoding
Language=English
Hindi (ISCII, Devanagari)
.
Language=Japanese
ヒンディー語 (ISCII, デバナガリ文字)
.

MessageId=57003
Facility=Encoding
Language=English
Bengali (ISCII)
.
Language=Japanese
ベンガル語 (ISCII)
.

MessageId=57004
Facility=Encoding
Language=English
Tamil (ISCII)
.
Language=Japanese
タミル語 (ISCII)
.

MessageId=57005
Facility=Encoding
Language=English
Telugu (ISCII)
.
Language=Japanese
テルグ語 (ISCII)
.

MessageId=57006
Facility=Encoding
Language=English
Assamese (ISCII)
.
Language=Japanese
アッサム語 (ISCII)
.

MessageId=57007
Facility=Encoding
Language=English
Oriya (ISCII)
.
Language=Japanese
オリヤー語 (ISCII)
.

MessageId=57008
Facility=Encoding
Language=English
Kannada (ISCII)
.
Language=Japanese
カンナダ語 (ISCII)
.

MessageId=57009
Facility=Encoding
Language=English
Malayalam (ISCII)
.
Language=Japanese
マラヤラム語 (ISCII)
.

MessageId=57010
Facility=Encoding
Language=English
Gujarathi (ISCII)
.
Language=Japanese
グジャラート語 (ISCII)
.

MessageId=57011
Facility=Encoding
Language=English
Panjabi (ISCII, Gurmukhi)
.
Language=Japanese
パンジャブ語 (ISCII, グルムキー文字)
.

MessageId=65000
Facility=Encoding
Language=English
Unicode (UTF-7)
.
Language=Japanese
Unicode (UTF-7)
.

MessageId=65001
Facility=Encoding
Language=English
Unicode (UTF-8)
.
Language=Japanese
Unicode (UTF-8)
.


;// extended encoding names

MessageId=10000
Facility=ExtendedEncoding
Language=English
Auto-Select (System Language)
.
Language=Japanese
自動選択 (システムの言語)
.

MessageId=10001
Facility=ExtendedEncoding
Language=English
Auto-Select (User Language)
.
Language=Japanese
自動選択 (ユーザの言語)
.

MessageId=10010
Facility=ExtendedEncoding
Language=English
Unicode (Auto-Select)
.
Language=Japanese
Unicode (自動選択)
.

MessageId=10011
Facility=ExtendedEncoding
Language=English
Unicode (UTF-5)
.
Language=Japanese
Unicode (UTF-5)
.

MessageId=10020
Facility=ExtendedEncoding
Language=English
Armenian (Auto-Select)
.
Language=Japanese
アルメニア語 (自動選択)
.

MessageId=10021
Facility=ExtendedEncoding
Language=English
Armenian (ARMSCII-7)
.
Language=Japanese
アルメニア語 (ARMSCII-7)
.

MessageId=10022
Facility=ExtendedEncoding
Language=English
Armenian (ARMSCII-8)
.
Language=Japanese
アルメニア語 (ARMSCII-8)
.

MessageId=10023
Facility=ExtendedEncoding
Language=English
Armenian (ARMSCII-8A)
.
Language=Japanese
アルメニア語 (ARMSCII-8A)
.

MessageId=10030
Facility=ExtendedEncoding
Language=English
Vietnamese (Auto-Select)
.
Language=Japanese
ベトナム語 (自動選択)
.

MessageId=10031
Facility=ExtendedEncoding
Language=English
Vietnamese (TCVN)
.
Language=Japanese
ベトナム語 (TCVN)
.

MessageId=10032
Facility=ExtendedEncoding
Language=English
Vietnamese (VISCII)
.
Language=Japanese
ベトナム語 (VISCII)
.

MessageId=10033
Facility=ExtendedEncoding
Language=English
Vietnamese (VPS)
.
Language=Japanese
ベトナム語 (VPS)
.

MessageId=10040
Facility=ExtendedEncoding
Language=English
Japanese (ISO-2022-JP)
.
Language=Japanese
日本語 (ISO-2022-JP)
.

MessageId=10041
Facility=ExtendedEncoding
Language=English
Japanese (Shift JIS)
.
Language=Japanese
日本語 (シフト JIS)
.

MessageId=10042
Facility=ExtendedEncoding
Language=English
Japanese (ISO-2022-JP-1)
.
Language=Japanese
日本語 (ISO-2022-JP-1)
.

MessageId=10043
Facility=ExtendedEncoding
Language=English
Japanese (ISO-2022-JP-2)
.
Language=Japanese
日本語 (ISO-2022-JP-2)
.

MessageId=10044
Facility=ExtendedEncoding
Language=English
Japanese (EUC)
.
Language=Japanese
日本語 (EUC)
.

MessageId=10045
Facility=ExtendedEncoding
Language=English
Japanese (ISO-2022-JP-2004)
.
Language=Japanese
日本語 (ISO-2022-JP-2004)
.

MessageId=10046
Facility=ExtendedEncoding
Language=English
Japanese (ISO-2022-JP-2004-Strict)
.
Language=Japanese
日本語 (ISO-2022-JP-2004-Strict)
.

MessageId=10047
Facility=ExtendedEncoding
Language=English
Japanese (ISO-2022-JP-2004-Compatible)
.
Language=Japanese
日本語 (ISO-2022-JP-2004-Compatible)
.

MessageId=10048
Facility=ExtendedEncoding
Language=English
Japanese (ISO-2022-JP-3)
.
Language=Japanese
日本語 (ISO-2022-JP-3)
.

MessageId=10049
Facility=ExtendedEncoding
Language=English
Japanese (ISO-2022-JP-3-Strict)
.
Language=Japanese
日本語 (ISO-2022-JP-3-Strict)
.

MessageId=10050
Facility=ExtendedEncoding
Language=English
Japanese (ISO-2022-JP-3-Compatible)
.
Language=Japanese
日本語 (ISO-2022-JP-3-Compatible)
.

MessageId=10051
Facility=ExtendedEncoding
Language=English
Japanese (Shift_JIS-2004)
.
Language=Japanese
日本語 (Shift_JIS-2004)
.

MessageId=10052
Facility=ExtendedEncoding
Language=English
Japanese (EUC-JIS-2004)
.
Language=Japanese
日本語 (EUC-JIS-2004)
.

MessageId=10060
Facility=ExtendedEncoding
Language=English
Multilingual (ISO-2022, 7bits)
.
Language=Japanese
マルチリンガル (ISO-2022, 7ビット)
.

MessageId=10061
Facility=ExtendedEncoding
Language=English
Multilingual (ISO-2022, 7bits, SS2)
.
Language=Japanese
マルチリンガル (ISO-2022, 7ビット, SS2)
.

MessageId=10062
Facility=ExtendedEncoding
Language=English
Multilingual (ISO-2022, 7bits, SI/SO)
.
Language=Japanese
マルチリンガル (ISO-2022, 7ビット, SI/SO)
.

MessageId=10063
Facility=ExtendedEncoding
Language=English
Multilingual (ISO-2022, 8bits, SS2)
.
Language=Japanese
マルチリンガル (ISO-2022, 8ビット, SS2)
.

MessageId=10070
Facility=ExtendedEncoding
Language=English
Binary
.
Language=Japanese
バイナリ
.

MessageId=10071
Facility=ExtendedEncoding
Language=English
NextSTEP
.
Language=Japanese
NextSTEP
.

MessageId=10072
Facility=ExtendedEncoding
Language=English
Atari ST/TT
.
Language=Japanese
Atari ST/TT
.

MessageId=10080
Facility=ExtendedEncoding
Language=English
Thai (TIS 620-2533:1990)
.
Language=Japanese
タイ語 (TIS 620-2533:1990)
.

MessageId=10090
Facility=ExtendedEncoding
Language=English
Lao (MuleLao-1)
.
Language=Japanese
ラオ語 (MuleLao-1)
.

MessageId=10091
Facility=ExtendedEncoding
Language=English
Lao (ibm-1132)
.
Language=Japanese
ラオ語 (ibm-1132)
.

MessageId=10092
Facility=ExtendedEncoding
Language=English
Lao (ibm-1133)
.
Language=Japanese
ラオ語 (ibm-1133)
.

MessageId=10100
Facility=ExtendedEncoding
Language=English
Irelandic (I.S. 434:1999)
.
Language=Japanese
アイルランド語 (I.S. 434:1999)
.

MessageId=10110
Facility=ExtendedEncoding
Language=English
Tamil (TAB)
.
Language=Japanese
タミル語 (TAB)
.

MessageId=10111
Facility=ExtendedEncoding
Language=English
Tamil (TAM)
.
Language=Japanese
タミル語 (TAM)
.

MessageId=10112
Facility=ExtendedEncoding
Language=English
Tamil (TSCII 1.7)
.
Language=Japanese
タミル語 (TSCII 1.7)
.

MessageId=10115
Facility=ExtendedEncoding
Language=English
Hindi (Macintosh, Devanagari)
.
Language=Japanese
ヒンディー語 (Macintosh, デバナガリ文字)
.

MessageId=10116
Facility=ExtendedEncoding
Language=English
Gujarathi (Macintosh)
.
Language=Japanese
グジャラート語 (Macintosh)
.

MessageId=10117
Facility=ExtendedEncoding
Language=English
Panjabi (Macintosh, Gurmukhi)
.
Language=Japanese
パンジャブ語 (Macintosh, グルムキー文字)
.
