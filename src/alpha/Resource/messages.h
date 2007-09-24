/*
 This file defines message strings.
 Translators are the following:
 - exeal (exeal@users.sourceforge.jp) -- Japanese
*/
// general messages
//
//  Values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//


//
// Define the severity codes
//


//
// MessageId: MSG_BUFFER__BUFFER_IS_DIRTY
//
// MessageText:
//
//  %1%n%nThe file is modified. Do you want to save?
//
#define MSG_BUFFER__BUFFER_IS_DIRTY      0x00000001L

//
// MessageId: MSG_BUFFER__CONFIRM_REOPEN
//
// MessageText:
//
//  Do you want to reopen the file?
//
#define MSG_BUFFER__CONFIRM_REOPEN       0x00000002L

//
// MessageId: MSG_BUFFER__CONFIRM_REOPEN_EVEN_IF_DIRTY
//
// MessageText:
//
//  The file is modified.
//  If you reopen, the modification will be lost. Do you want to do?
//
#define MSG_BUFFER__CONFIRM_REOPEN_EVEN_IF_DIRTY 0x00000003L

//
// MessageId: MSG_BUFFER__FAILED_TO_SEND_TO_MAILER
//
// MessageText:
//
//  Failed to send to a mailer.
//
#define MSG_BUFFER__FAILED_TO_SEND_TO_MAILER 0x00000004L

//
// MessageId: MSG_BUFFER__UNTITLED
//
// MessageText:
//
//  untitled
//
#define MSG_BUFFER__UNTITLED             0x00000005L

//
// MessageId: MSG_BUFFER__FILE_IS_MODIFIED_AND_REOPEN
//
// MessageText:
//
//  %1%n%nThe file was modified by other process. Do you want to reload?
//  If you reload, current editing contents will be disposed.
//
#define MSG_BUFFER__FILE_IS_MODIFIED_AND_REOPEN 0x00000006L

//
// MessageId: MSG_BUFFER__FILE_IS_MODIFIED_AND_SAVE
//
// MessageText:
//
//  %1%n%nThe file was modified by other process. Do you want to continue to save?
//
#define MSG_BUFFER__FILE_IS_MODIFIED_AND_SAVE 0x00000007L

//
// MessageId: MSG_BUFFER__FILE_IS_MODIFIED_AND_EDIT
//
// MessageText:
//
//  %1%n%nThe file was modified by other process. Do you want to continue to edit?
//
#define MSG_BUFFER__FILE_IS_MODIFIED_AND_EDIT 0x00000008L

//
// MessageId: MSG_BUFFER__SAVING_FILE_IS_OPENED
//
// MessageText:
//
//  %1%n%nThe file has been already opened. Cannot write to an editing file.
//
#define MSG_BUFFER__SAVING_FILE_IS_OPENED 0x00000009L

//
// MessageId: MSG_SEARCH__PATTERN_NOT_FOUND
//
// MessageText:
//
//  Specified string was not found.
//
#define MSG_SEARCH__PATTERN_NOT_FOUND    0x0000000AL

//
// MessageId: MSG_SEARCH__REPLACE_DONE
//
// MessageText:
//
//  Replaced %1 string(s).
//
#define MSG_SEARCH__REPLACE_DONE         0x0000000BL

//
// MessageId: MSG_SEARCH_REGEX_IS_INAVAILABLE
//
// MessageText:
//
//  Failed to load the regular expression engine. Cannot search.
//
#define MSG_SEARCH_REGEX_IS_INAVAILABLE  0x0000000CL

//
// MessageId: MSG_SEARCH__INVALID_REGEX_PATTERN
//
// MessageText:
//
//  An error occured during regular expression search.%n%nReason:%t%1%nPosition:%t%2
//
#define MSG_SEARCH__INVALID_REGEX_PATTERN 0x0000000DL

//
// MessageId: MSG_SEARCH__BAD_PATTERN_START
//
// MessageText:
//
//  Not an error
//
#define MSG_SEARCH__BAD_PATTERN_START    0x0000000EL

//
// MessageId: 0x0000000FL (No symbolic name defined)
//
// MessageText:
//
//  An invalid collating element was specified in a [[.name.]] block
//


//
// MessageId: 0x00000010L (No symbolic name defined)
//
// MessageText:
//
//  An invalid character class name was specified in a [[:name:]] block
//


//
// MessageId: 0x00000011L (No symbolic name defined)
//
// MessageText:
//
//  An invalid or trailing escape was encountered
//


//
// MessageId: 0x00000012L (No symbolic name defined)
//
// MessageText:
//
//  A back-reference to a non-existant marked sub-expression was encountered
//


//
// MessageId: 0x00000013L (No symbolic name defined)
//
// MessageText:
//
//  An invalid character set was encountered
//


//
// MessageId: 0x00000014L (No symbolic name defined)
//
// MessageText:
//
//  Mismatched '(' and ')'
//


//
// MessageId: 0x00000015L (No symbolic name defined)
//
// MessageText:
//
//  Mismatched '{' and '}'
//


//
// MessageId: 0x00000016L (No symbolic name defined)
//
// MessageText:
//
//  Invalid contents of a {...} block
//


//
// MessageId: 0x00000017L (No symbolic name defined)
//
// MessageText:
//
//  A character range was invalid
//


//
// MessageId: 0x00000018L (No symbolic name defined)
//
// MessageText:
//
//  Out of memory
//


//
// MessageId: 0x00000019L (No symbolic name defined)
//
// MessageText:
//
//  An attempt to repeat something that can not be repeated
//


//
// MessageId: 0x0000001AL (No symbolic name defined)
//
// MessageText:
//
//  The expression bacame too complex to handle
//


//
// MessageId: 0x0000001BL (No symbolic name defined)
//
// MessageText:
//
//  Out of program stack space
//


//
// MessageId: 0x0000001CL (No symbolic name defined)
//
// MessageText:
//
//  Unknown error
//


//
// MessageId: MSG_ERROR__EXE_NOT_REGISTERED
//
// MessageText:
//
//  "%1"%nNo program assigned to the document type.
//
#define MSG_ERROR__EXE_NOT_REGISTERED    0x0000001DL

//
// MessageId: MSG_ERROR__FAILED_TO_RUN_EXE
//
// MessageText:
//
//  Failed to run the program.
//  The path assigned to the document type may be incorrect.
//
#define MSG_ERROR__FAILED_TO_RUN_EXE     0x0000001EL

//
// MessageId: MSG_ERROR__UNSUPPORTED_OS_VERSION
//
// MessageText:
//
//  Alpha does not support your platform.
//
#define MSG_ERROR__UNSUPPORTED_OS_VERSION 0x0000001FL

//
// MessageId: MSG_ERROR__DUPLICATE_ABBREV
//
// MessageText:
//
//  There is already same abbreviation.
//
#define MSG_ERROR__DUPLICATE_ABBREV      0x00000020L

//
// MessageId: MSG_ERROR__REGEX_UNKNOWN_ERROR
//
// MessageText:
//
//  Error occured during regular expression pattern matching.
//  The pattern may be too complex.
//
#define MSG_ERROR__REGEX_UNKNOWN_ERROR   0x00000021L

//
// MessageId: MSG_ERROR__MIGEMO_UNKNOWN_ERROR
//
// MessageText:
//
//  Error occured in Migemo.
//
#define MSG_ERROR__MIGEMO_UNKNOWN_ERROR  0x00000022L

//
// MessageId: MSG_ERROR__PROHIBITED_FOR_MACRO_DEFINING
//
// MessageText:
//
//  This command cannot execute in keyboard macro definition.
//
#define MSG_ERROR__PROHIBITED_FOR_MACRO_DEFINING 0x00000023L

//
// MessageId: MSG_ERROR__FAILED_TO_LOAD_TEMP_MACRO
//
// MessageText:
//
//  Failed to load the keyboard macro file.%n%nFile: %1%nPosition: (%2, %3)%nReason: %4
//
#define MSG_ERROR__FAILED_TO_LOAD_TEMP_MACRO 0x00000024L

//
// MessageId: MSG_ERROR__FAILED_TO_LOAD_SOMETHING
//
// MessageText:
//
//  (Failed to load)
//
#define MSG_ERROR__FAILED_TO_LOAD_SOMETHING 0x00000025L

//
// MessageId: MSG_ERROR__OUT_OF_MEMORY
//
// MessageText:
//
//  Process failed for out of memory.
//
#define MSG_ERROR__OUT_OF_MEMORY         0x00000026L

//
// MessageId: MSG_STATUS__READ_ONLY_CAPTION
//
// MessageText:
//
//  [readonly]
//
#define MSG_STATUS__READ_ONLY_CAPTION    0x00000027L

//
// MessageId: MSG_STATUS__TEMP_MACRO_DEFINING
//
// MessageText:
//
//  Defining keyboard macro
//
#define MSG_STATUS__TEMP_MACRO_DEFINING  0x00000028L

//
// MessageId: MSG_STATUS__TEMP_MACRO_PAUSING
//
// MessageText:
//
//  Pausing keyboard macro definition
//
#define MSG_STATUS__TEMP_MACRO_PAUSING   0x00000029L

//
// MessageId: MSG_STATUS__NARROWING
//
// MessageText:
//
//  Narrowing
//
#define MSG_STATUS__NARROWING            0x0000002AL

//
// MessageId: MSG_STATUS__DEBUGGING
//
// MessageText:
//
//  Debuggnig
//
#define MSG_STATUS__DEBUGGING            0x0000002BL

//
// MessageId: MSG_STATUS__INSERT_MODE
//
// MessageText:
//
//  Insert
//
#define MSG_STATUS__INSERT_MODE          0x0000002CL

//
// MessageId: MSG_STATUS__OVERTYPE_MODE
//
// MessageText:
//
//  Overtype
//
#define MSG_STATUS__OVERTYPE_MODE        0x0000002DL

//
// MessageId: MSG_STATUS__CARET_POSITION
//
// MessageText:
//
//  Line: %1!lu! , Column: %2!lu! , Char: %3!lu!
//
#define MSG_STATUS__CARET_POSITION       0x0000002EL

//
// MessageId: MSG_STATUS__CAN_EXPAND_ABBREV
//
// MessageText:
//
//  '%1' can expand to '%2'.
//
#define MSG_STATUS__CAN_EXPAND_ABBREV    0x0000002FL

//
// MessageId: MSG_STATUS__LOADING_FILE
//
// MessageText:
//
//  Loading '%1'...
//
#define MSG_STATUS__LOADING_FILE         0x00000030L

//
// MessageId: MSG_STATUS__WAITING_FOR_2ND_KEY
//
// MessageText:
//
//  '%1' is pressed. Waiting for the second key combination.
//
#define MSG_STATUS__WAITING_FOR_2ND_KEY  0x00000031L

//
// MessageId: MSG_STATUS__INVALID_2STROKE_COMBINATION
//
// MessageText:
//
//  There are no command bound to the key combination '%1'.
//
#define MSG_STATUS__INVALID_2STROKE_COMBINATION 0x00000032L

//
// MessageId: MSG_STATUS__ISEARCH
//
// MessageText:
//
//  Incremental search : %1
//
#define MSG_STATUS__ISEARCH              0x00000033L

//
// MessageId: MSG_STATUS__ISEARCH_EMPTY_PATTERN
//
// MessageText:
//
//  Incremental search : (empty pattern)
//
#define MSG_STATUS__ISEARCH_EMPTY_PATTERN 0x00000034L

//
// MessageId: MSG_STATUS__ISEARCH_BAD_PATTERN
//
// MessageText:
//
//  Incremental Search : %1 (invalid pattern)
//
#define MSG_STATUS__ISEARCH_BAD_PATTERN  0x00000035L

//
// MessageId: MSG_STATUS__ISEARCH_NOT_FOUND
//
// MessageText:
//
//  Incremental search : %1 (not found)
//
#define MSG_STATUS__ISEARCH_NOT_FOUND    0x00000036L

//
// MessageId: MSG_STATUS__RISEARCH
//
// MessageText:
//
//  Reversal incremental search : %1
//
#define MSG_STATUS__RISEARCH             0x00000037L

//
// MessageId: MSG_STATUS__RISEARCH_NOT_FOUND
//
// MessageText:
//
//  Reversal incremental search : %1 (not found)
//
#define MSG_STATUS__RISEARCH_NOT_FOUND   0x00000038L

//
// MessageId: MSG_STATUS__RISEARCH_BAD_PATTERN
//
// MessageText:
//
//  Reversal Incremental Search : %1 (invalid pattern)
//
#define MSG_STATUS__RISEARCH_BAD_PATTERN 0x00000039L

//
// MessageId: MSG_STATUS__RISEARCH_EMPTY_PATTERN
//
// MessageText:
//
//  Reversal incremental search : (empty pattern)
//
#define MSG_STATUS__RISEARCH_EMPTY_PATTERN 0x0000003AL

//
// MessageId: MSG_STATUS__MATCH_BRACKET_OUT_OF_VIEW
//
// MessageText:
//
//  (Match brace) %1 : %2 
//
#define MSG_STATUS__MATCH_BRACKET_OUT_OF_VIEW 0x0000003BL

//
// MessageId: MSG_STATUS__INVOKABLE_LINK_POPUP
//
// MessageText:
//
//  %1%nCtrl plus click to open the link.
//
#define MSG_STATUS__INVOKABLE_LINK_POPUP 0x0000003CL

//
// MessageId: MSG_DIALOG__DEFAULT_OPENFILE_FILTER
//
// MessageText:
//
//  All Files:*.*
//
#define MSG_DIALOG__DEFAULT_OPENFILE_FILTER 0x0000003DL

//
// MessageId: MSG_DIALOG__WHOLE_GRAPHEME_MATCH
//
// MessageText:
//
//  Grapheme Cluster
//
#define MSG_DIALOG__WHOLE_GRAPHEME_MATCH 0x0000003EL

//
// MessageId: MSG_DIALOG__WHOLE_WORD_MATCH
//
// MessageText:
//
//  Word
//
#define MSG_DIALOG__WHOLE_WORD_MATCH     0x0000003FL

//
// MessageId: MSG_DIALOG__LITERAL_SEARCH
//
// MessageText:
//
//  Literal
//
#define MSG_DIALOG__LITERAL_SEARCH       0x00000040L

//
// MessageId: MSG_DIALOG__REGEX_SEARCH
//
// MessageText:
//
//  Regular expression (Boost.Regex 1.34.1)
//
#define MSG_DIALOG__REGEX_SEARCH         0x00000041L

//
// MessageId: MSG_DIALOG__MIGEMO_SEARCH
//
// MessageText:
//
//  Migemo (C/Migemo 1.2 expected)
//
#define MSG_DIALOG__MIGEMO_SEARCH        0x00000042L

//
// MessageId: MSG_DIALOG__BUFFERBAR_CAPTION
//
// MessageText:
//
//  Buffers
//
#define MSG_DIALOG__BUFFERBAR_CAPTION    0x00000043L

//
// MessageId: MSG_DIALOG__ABBREVIATION
//
// MessageText:
//
//  Abbreviation
//
#define MSG_DIALOG__ABBREVIATION         0x00000044L

//
// MessageId: MSG_DIALOG__EXPANDED_ABBREVIATION
//
// MessageText:
//
//  Expanded String
//
#define MSG_DIALOG__EXPANDED_ABBREVIATION 0x00000045L

//
// MessageId: MSG_DIALOG__BOOKMARKED_LINE
//
// MessageText:
//
//  Line
//
#define MSG_DIALOG__BOOKMARKED_LINE      0x00000046L

//
// MessageId: MSG_DIALOG__BOOKMARKED_POSITION
//
// MessageText:
//
//  Position
//
#define MSG_DIALOG__BOOKMARKED_POSITION  0x00000047L

//
// MessageId: MSG_DIALOG__LINE_NUMBER_RANGE
//
// MessageText:
//
//  Line &Number (%1-%2):
//
#define MSG_DIALOG__LINE_NUMBER_RANGE    0x00000048L

//
// MessageId: MSG_DIALOG__SELECT_ALL
//
// MessageText:
//
//  Select &All
//
#define MSG_DIALOG__SELECT_ALL           0x00000049L

//
// MessageId: MSG_DIALOG__UNSELECT_ALL
//
// MessageText:
//
//  &Cancel All Selections
//
#define MSG_DIALOG__UNSELECT_ALL         0x0000004AL

//
// MessageId: MSG_DIALOG__SELECT_DIRECTORY
//
// MessageText:
//
//  Select a directory.
//
#define MSG_DIALOG__SELECT_DIRECTORY     0x0000004BL

//
// MessageId: MSG_DIALOG__FILE_OPERATION
//
// MessageText:
//
//  File Operation
//
#define MSG_DIALOG__FILE_OPERATION       0x0000004CL

//
// MessageId: MSG_DIALOG__RENAME_FILE
//
// MessageText:
//
//  Rename
//
#define MSG_DIALOG__RENAME_FILE          0x0000004DL

//
// MessageId: MSG_DIALOG__COPY_FILE
//
// MessageText:
//
//  Copy
//
#define MSG_DIALOG__COPY_FILE            0x0000004EL

//
// MessageId: MSG_DIALOG__MOVE_FILE
//
// MessageText:
//
//  Move
//
#define MSG_DIALOG__MOVE_FILE            0x0000004FL

//
// MessageId: MSG_DIALOG__KEEP_NEWLINE
//
// MessageText:
//
//  No Change
//
#define MSG_DIALOG__KEEP_NEWLINE         0x00000050L

//
// MessageId: MSG_DIALOG__SAVE_FILE_FILTER
//
// MessageText:
//
//  All Files
//
#define MSG_DIALOG__SAVE_FILE_FILTER     0x00000051L

//
// MessageId: MSG_IO__FAILED_TO_DETECT_ENCODE
//
// MessageText:
//
//  %1%n%nFailed to detect code page of the file. Used system-default code page.
//
#define MSG_IO__FAILED_TO_DETECT_ENCODE  0x00000052L

//
// MessageId: MSG_IO__INVALID_ENCODING
//
// MessageText:
//
//  %1%n%nSpecified code page is invalid. Cannot open the file.
//
#define MSG_IO__INVALID_ENCODING         0x00000053L

//
// MessageId: MSG_IO__INVALID_NEWLINE
//
// MessageText:
//
//  %1%n%nSpecified newline is invalid. Cannot open the file.
//
#define MSG_IO__INVALID_NEWLINE          0x00000054L

//
// MessageId: MSG_IO__UNCONVERTABLE_UCS_CHAR
//
// MessageText:
//
//  %1%nEncoding: %2%n
//  The document contains unconvertable characters under specified encoding.
//  These characters will be written incorrectly.
//  Do you want to change code page?
//
#define MSG_IO__UNCONVERTABLE_UCS_CHAR   0x00000055L

//
// MessageId: MSG_IO__UNCONVERTABLE_NATIVE_CHAR
//
// MessageText:
//
//  %1%nEncoding: %2%n
//  The file contains unconvertable characters under specified encoding.
//  Saving may destroy contents of the file.
//  Do you want to change code page?
//
#define MSG_IO__UNCONVERTABLE_NATIVE_CHAR 0x00000056L

//
// MessageId: MSG_IO__HUGE_FILE
//
// MessageText:
//
//  %1%n%nThe file size is too large. Maximum file size Alpha can handle is 2GB.
//
#define MSG_IO__HUGE_FILE                0x00000057L

//
// MessageId: MSG_IO__FAILED_TO_WRITE_FOR_READONLY
//
// MessageText:
//
//  %1%n%nCannot overwrite the file. The file is read-only.
//
#define MSG_IO__FAILED_TO_WRITE_FOR_READONLY 0x00000058L

//
// MessageId: MSG_IO__FAILED_TO_RESOLVE_SHORTCUT
//
// MessageText:
//
//  %1%n%nFailed to resolve the shortcut. Cannot open the file.
//
#define MSG_IO__FAILED_TO_RESOLVE_SHORTCUT 0x00000059L

//
// MessageId: MSG_IO__CANNOT_OPEN_FILE
//
// MessageText:
//
//  %1%n%nCannot open the file.
//
#define MSG_IO__CANNOT_OPEN_FILE         0x0000005AL

//
// MessageId: MSG_IO__CANNOT_CREATE_TEMP_FILE
//
// MessageText:
//
//  %1%n%nFailed to save the file. Cannot create temporary file.
//
#define MSG_IO__CANNOT_CREATE_TEMP_FILE  0x0000005BL

//
// MessageId: MSG_IO__LOST_DISK_FILE
//
// MessageText:
//
//  %1%n%nFailed to save the file. The file is lost.
//
#define MSG_IO__LOST_DISK_FILE           0x0000005CL

//
// MessageId: MSG_SCRIPT__ERROR_DIALOG
//
// MessageText:
//
//  Script error occured.%n%nScript:%t%1%nLine:%t%2%nCharacter:%t%3%nError:%t%4%nSCODE:%t%5%nSource:%t%6
//
#define MSG_SCRIPT__ERROR_DIALOG         0x0000005DL

//
// MessageId: MSG_SCRIPT__UNSAFE_ACTIVEX_CAUTION
//
// MessageText:
//
//  %1%n%nThis script is about to create an unsafe object.%nDo you allow this?%n%nQueried object:%n  ProgID:%t%2%n  IID:%t%3
//
#define MSG_SCRIPT__UNSAFE_ACTIVEX_CAUTION 0x0000005EL

//
// MessageId: MSG_SCRIPT__INVALID_LANGUAGE_NAME
//
// MessageText:
//
//  The language "%1" bound to the macro is invalid or not installed. Cannot execute.
//
#define MSG_SCRIPT__INVALID_LANGUAGE_NAME 0x0000005FL

//
// MessageId: MSG_SCRIPT__FAILED_TO_OPEN_MACRO_SCRIPT
//
// MessageText:
//
//  %1%n%nFailed to load the macro file. Cannot execute.
//
#define MSG_SCRIPT__FAILED_TO_OPEN_MACRO_SCRIPT 0x00000060L

//
// MessageId: MSG_OTHER__EMPTY_MENU_CAPTION
//
// MessageText:
//
//  (nothing)
//
#define MSG_OTHER__EMPTY_MENU_CAPTION    0x00000061L

//
// MessageId: MSG_OTHER__UNKNOWN
//
// MessageText:
//
//  (unknown)
//
#define MSG_OTHER__UNKNOWN               0x00000062L

//
// MessageId: MSG_OTHER__NOT_OBTAINED
//
// MessageText:
//
//  (not obtained)
//
#define MSG_OTHER__NOT_OBTAINED          0x00000063L

//
// MessageId: MSG_OTHER__TEMPORARY_MACRO_QUERY
//
// MessageText:
//
//  Proceed the macro?\n\nNo: Skip this repetition and next\nCancel: Abort the macro
//
#define MSG_OTHER__TEMPORARY_MACRO_QUERY 0x00000064L

// command names and description.
//
// MessageId: CMD_FILE_TOP
//
// MessageText:
//
//  &File
//
#define CMD_FILE_TOP                     0x00010001L

//
// MessageId: CMD_FILE_NEW
//
// MessageText:
//
//  &New\nCreate a new document.
//
#define CMD_FILE_NEW                     0x00010002L

//
// MessageId: CMD_FILE_NEWWITHFORMAT
//
// MessageText:
//
//  New wi&th Format...\nCreate a new document with specified code page, line break, and document type.
//
#define CMD_FILE_NEWWITHFORMAT           0x00010003L

//
// MessageId: CMD_FILE_OPEN
//
// MessageText:
//
//  &Open...\nOpen an existing document.
//
#define CMD_FILE_OPEN                    0x00010004L

//
// MessageId: CMD_FILE_CLOSE
//
// MessageText:
//
//  &Close\nClose the document.
//
#define CMD_FILE_CLOSE                   0x00010005L

//
// MessageId: CMD_FILE_CLOSEALL
//
// MessageText:
//
//  Cl&ose All\nClose all documents.
//
#define CMD_FILE_CLOSEALL                0x00010006L

//
// MessageId: CMD_FILE_SAVE
//
// MessageText:
//
//  &Save\nSave the document.
//
#define CMD_FILE_SAVE                    0x00010007L

//
// MessageId: CMD_FILE_SAVEAS
//
// MessageText:
//
//  Save &As...\nSave the document under a different name.
//
#define CMD_FILE_SAVEAS                  0x00010008L

//
// MessageId: CMD_FILE_SAVEALL
//
// MessageText:
//
//  Sav&e All\nSave all documents.
//
#define CMD_FILE_SAVEALL                 0x00010009L

//
// MessageId: CMD_FILE_MRU
//
// MessageText:
//
//  Most Recent Used &Files
//
#define CMD_FILE_MRU                     0x0001000AL

//
// MessageId: CMD_FILE_OPERATE
//
// MessageText:
//
//  Op&erate
//
#define CMD_FILE_OPERATE                 0x0001000BL

//
// MessageId: CMD_FILE_REOPEN
//
// MessageText:
//
//  &Reopen\nReopen the document.
//
#define CMD_FILE_REOPEN                  0x0001000CL

//
// MessageId: CMD_FILE_REOPENWITHCODEPAGE
//
// MessageText:
//
//  Reopen with Different Code Pa&ge...\nReopen the document with a different code page.
//
#define CMD_FILE_REOPENWITHCODEPAGE      0x0001000DL

//
// MessageId: CMD_FILE_EXIT
//
// MessageText:
//
//  E&xit Alpha\nClose all documents and exit.
//
#define CMD_FILE_EXIT                    0x0001000EL

//
// MessageId: CMD_FILE_SENDMAIL
//
// MessageText:
//
//  Sen&d...\nSend the file.
//
#define CMD_FILE_SENDMAIL                0x0001000FL

//
// MessageId: CMD_FILE_CLOSEOTHERS
//
// MessageText:
//
//  Close Ot&hers\nClose all inactive documents.
//
#define CMD_FILE_CLOSEOTHERS             0x00010010L

//
// MessageId: CMD_FILE_PRINT
//
// MessageText:
//
//  &Print...\nPrint the document.
//
#define CMD_FILE_PRINT                   0x00010011L

//
// MessageId: CMD_FILE_PRINTSETUP
//
// MessageText:
//
//  Set&up Page...\nSetup page layout.
//
#define CMD_FILE_PRINTSETUP              0x00010012L

//
// MessageId: CMD_FILE_PRINTPREVIEW
//
// MessageText:
//
//  Pre&view Page...\nPreview the entire page.
//
#define CMD_FILE_PRINTPREVIEW            0x00010013L

//
// MessageId: CMD_EDIT_TOP
//
// MessageText:
//
//  &Edit
//
#define CMD_EDIT_TOP                     0x00010014L

//
// MessageId: CMD_EDIT_ADVANCED
//
// MessageText:
//
//  &Advanced
//
#define CMD_EDIT_ADVANCED                0x00010015L

//
// MessageId: CMD_EDIT_INSERTUNICODECTRLS
//
// MessageText:
//
//  &Insert Unicode Control Characters
//
#define CMD_EDIT_INSERTUNICODECTRLS      0x00010016L

//
// MessageId: CMD_EDIT_INSERTUNICODEWSS
//
// MessageText:
//
//  Insert Unicode &Whitespace Characters
//
#define CMD_EDIT_INSERTUNICODEWSS        0x00010017L

//
// MessageId: CMD_EDIT_DELETE
//
// MessageText:
//
//  &Delete\nDelete the selection.
//
#define CMD_EDIT_DELETE                  0x00010018L

//
// MessageId: CMD_EDIT_BACKSPACE
//
// MessageText:
//
//  Backspace\nDelete backward one character.
//
#define CMD_EDIT_BACKSPACE               0x00010019L

//
// MessageId: CMD_EDIT_DELETETONEXTWORD
//
// MessageText:
//
//  Delete Next Word\nDelete to next word.
//
#define CMD_EDIT_DELETETONEXTWORD        0x0001001AL

//
// MessageId: CMD_EDIT_DELETETOPREVWORD
//
// MessageText:
//
//  Delete Previous Word\nDelete to previous word.
//
#define CMD_EDIT_DELETETOPREVWORD        0x0001001BL

//
// MessageId: CMD_EDIT_DELETELINE
//
// MessageText:
//
//  Delete Line\nDelete current line.
//
#define CMD_EDIT_DELETELINE              0x0001001CL

//
// MessageId: CMD_EDIT_INSERTPREVLINE
//
// MessageText:
//
//  Insert Previous\nInsert new line previous.
//
#define CMD_EDIT_INSERTPREVLINE          0x0001001DL

//
// MessageId: CMD_EDIT_BREAK
//
// MessageText:
//
//  Break\nBreak the line.
//
#define CMD_EDIT_BREAK                   0x0001001EL

//
// MessageId: CMD_EDIT_UNDO
//
// MessageText:
//
//  &Undo\nUndo editing.
//
#define CMD_EDIT_UNDO                    0x0001001FL

//
// MessageId: CMD_EDIT_REDO
//
// MessageText:
//
//  &Redo\nRedo previously undone editing.
//
#define CMD_EDIT_REDO                    0x00010020L

//
// MessageId: CMD_EDIT_CUT
//
// MessageText:
//
//  Cu&t\nCut the selection and put it on the Clipboard.
//
#define CMD_EDIT_CUT                     0x00010021L

//
// MessageId: CMD_EDIT_COPY
//
// MessageText:
//
//  &Copy\nCopy the selection and put it on the Clipboard.
//
#define CMD_EDIT_COPY                    0x00010022L

//
// MessageId: CMD_EDIT_PASTE
//
// MessageText:
//
//  &Paste\nInsert Clipboard contents.
//
#define CMD_EDIT_PASTE                   0x00010023L

//
// MessageId: CMD_EDIT_INSERTTAB
//
// MessageText:
//
//  Insert Tab\nInsert a tab.
//
#define CMD_EDIT_INSERTTAB               0x00010024L

//
// MessageId: CMD_EDIT_DELETETAB
//
// MessageText:
//
//  Delete Tab\nDelete a tab.
//
#define CMD_EDIT_DELETETAB               0x00010025L

//
// MessageId: CMD_EDIT_TABIFY
//
// MessageText:
//
//  Tabify\nConvert whitespaces in the selection to tabs.
//
#define CMD_EDIT_TABIFY                  0x00010026L

//
// MessageId: CMD_EDIT_UNTABIFY
//
// MessageText:
//
//  Untabify\nConvert tabs in the selection to spaces.
//
#define CMD_EDIT_UNTABIFY                0x00010027L

//
// MessageId: CMD_EDIT_PASTEFROMCLIPBOARDRING
//
// MessageText:
//
//  Paste From C&lipboard Ring\nInsert Clipboard Ring contents.
//
#define CMD_EDIT_PASTEFROMCLIPBOARDRING  0x00010028L

//
// MessageId: CMD_EDIT_CHARFROMABOVELINE
//
// MessageText:
//
//  Input Above Character\nInput the character above caret.
//
#define CMD_EDIT_CHARFROMABOVELINE       0x00010029L

//
// MessageId: CMD_EDIT_CHARFROMBELOWLINE
//
// MessageText:
//
//  Input Below Character\nInput the character below caret.
//
#define CMD_EDIT_CHARFROMBELOWLINE       0x0001002AL

//
// MessageId: CMD_EDIT_TRANSPOSELINES
//
// MessageText:
//
//  Transpose Lines\nTranspose the current line and the previous line.
//
#define CMD_EDIT_TRANSPOSELINES          0x0001002BL

//
// MessageId: CMD_EDIT_TRANSPOSECHARS
//
// MessageText:
//
//  Transpose Characters\nTranspose the character before caret with the character after caret.
//
#define CMD_EDIT_TRANSPOSECHARS          0x0001002CL

//
// MessageId: CMD_EDIT_TRANSPOSEWORDS
//
// MessageText:
//
//  Transpose Words\nTranspose the word before caret with the word after caret.
//
#define CMD_EDIT_TRANSPOSEWORDS          0x0001002DL

//
// MessageId: CMD_EDIT_SHOWABBREVIATIONDLG
//
// MessageText:
//
//  Manage A&bbreviations...\nManage all abbreviations.
//
#define CMD_EDIT_SHOWABBREVIATIONDLG     0x0001002EL

//
// MessageId: CMD_EDIT_FIRSTCHAR
//
// MessageText:
//
//  First Character\nGo to the first non-whitespace character in the line.
//
#define CMD_EDIT_FIRSTCHAR               0x0001002FL

//
// MessageId: CMD_EDIT_LASTCHAR
//
// MessageText:
//
//  Last Character\nGo to the last non-whitespace character in the line.
//
#define CMD_EDIT_LASTCHAR                0x00010030L

//
// MessageId: CMD_EDIT_FIRSTCHAREXTEND
//
// MessageText:
//
//  Extend to First Character\nExtend the selection to the first non-whitespace character in the line.
//
#define CMD_EDIT_FIRSTCHAREXTEND         0x00010031L

//
// MessageId: CMD_EDIT_LASTCHAREXTEND
//
// MessageText:
//
//  Extend to Last Character\nExtend the selection to the last non-whitespace character in the line.
//
#define CMD_EDIT_LASTCHAREXTEND          0x00010032L

//
// MessageId: CMD_EDIT_FIRSTCHARORLINEHOME
//
// MessageText:
//
//  First Character or Start of Line\nGo to the first non-whitespace character in the line or the start of the line.
//
#define CMD_EDIT_FIRSTCHARORLINEHOME     0x00010033L

//
// MessageId: CMD_EDIT_LASTCHARORLINEEND
//
// MessageText:
//
//  Last Character or End of Line\nGo to the last non-whitespace character in the line or the end of the line.
//
#define CMD_EDIT_LASTCHARORLINEEND       0x00010034L

//
// MessageId: CMD_EDIT_CHARTOCODEPOINT
//
// MessageText:
//
//  Character to Code Point\nConvert a character to a corresponding code point.
//
#define CMD_EDIT_CHARTOCODEPOINT         0x00010035L

//
// MessageId: CMD_EDIT_CODEPOINTTOCHAR
//
// MessageText:
//
//  Code Point to Character\nConvert a code point to a corresponding character.
//
#define CMD_EDIT_CODEPOINTTOCHAR         0x00010036L

//
// MessageId: CMD_EDIT_RECOMPOSE
//
// MessageText:
//
//  Reconvert\nReconvert the selection.
//
#define CMD_EDIT_RECOMPOSE               0x00010037L

//
// MessageId: CMD_EDIT_TOGGLEOVERTYPEMODE
//
// MessageText:
//
//  Overtype Mode\nToggle insert/overtype mode.
//
#define CMD_EDIT_TOGGLEOVERTYPEMODE      0x00010038L

//
// MessageId: CMD_EDIT_OPENCANDIDATE
//
// MessageText:
//
//  I&nput Candidates\nShow the candidates window or complete the word.
//
#define CMD_EDIT_OPENCANDIDATE           0x00010039L

//
// MessageId: CMD_EDIT_HOME
//
// MessageText:
//
//  Start of Document\nGo to the start of the document.
//
#define CMD_EDIT_HOME                    0x0001003AL

//
// MessageId: CMD_EDIT_END
//
// MessageText:
//
//  End of Document\nGo to the end of the document.
//
#define CMD_EDIT_END                     0x0001003BL

//
// MessageId: CMD_EDIT_LINEHOME
//
// MessageText:
//
//  Start of Line\nGo to the start of the line.
//
#define CMD_EDIT_LINEHOME                0x0001003CL

//
// MessageId: CMD_EDIT_LINEEND
//
// MessageText:
//
//  End of Line\nGo to the end of the line.
//
#define CMD_EDIT_LINEEND                 0x0001003DL

//
// MessageId: CMD_EDIT_CHARNEXT
//
// MessageText:
//
//  Next Character\nGo to the next character.
//
#define CMD_EDIT_CHARNEXT                0x0001003EL

//
// MessageId: CMD_EDIT_SCROLLCOLUMNNEXT
//
// MessageText:
//
//  Scroll Right\nScroll the window right one column right.
//
#define CMD_EDIT_SCROLLCOLUMNNEXT        0x0001003FL

//
// MessageId: CMD_EDIT_SCROLLCOLUMNPREV
//
// MessageText:
//
//  Scroll Left\nScroll the window left one column left.
//
#define CMD_EDIT_SCROLLCOLUMNPREV        0x00010040L

//
// MessageId: CMD_EDIT_ENSURECARETCENTER
//
// MessageText:
//
//  Recenter\nEnsure the caret center.
//
#define CMD_EDIT_ENSURECARETCENTER       0x00010041L

//
// MessageId: CMD_EDIT_ENSURECARETVISIBLE
//
// MessageText:
//
//  Show Caret\nEnsure the caret visible.
//
#define CMD_EDIT_ENSURECARETVISIBLE      0x00010042L

//
// MessageId: CMD_EDIT_ROWCHARNEXT
//
// MessageText:
//
//  Extend Box to Next Character\nExtend the rectangle selection to the next character.
//
#define CMD_EDIT_ROWCHARNEXT             0x00010043L

//
// MessageId: CMD_EDIT_ROWCHARPREV
//
// MessageText:
//
//  Extend Box to Previous Character\nExtend the rectangle selection to the previous character.
//
#define CMD_EDIT_ROWCHARPREV             0x00010044L

//
// MessageId: CMD_EDIT_ROWLINEDOWN
//
// MessageText:
//
//  Extend Box to Down Line\nExtend the rectangle selection to the one down line.
//
#define CMD_EDIT_ROWLINEDOWN             0x00010045L

//
// MessageId: CMD_EDIT_ROWLINEUP
//
// MessageText:
//
//  Extend Box to Up Line\nExtend the rectangle selection to the one up line.
//
#define CMD_EDIT_ROWLINEUP               0x00010046L

//
// MessageId: CMD_EDIT_ROWLINEEND
//
// MessageText:
//
//  Extend Box to End of Line\nExtend the rectangle selection to the end of the line.
//
#define CMD_EDIT_ROWLINEEND              0x00010047L

//
// MessageId: CMD_EDIT_ROWLINEHOME
//
// MessageText:
//
//  Extend Box to Start of Line\nExtend the rectangle selection to the start of the line.
//
#define CMD_EDIT_ROWLINEHOME             0x00010048L

//
// MessageId: CMD_EDIT_ROWWORDNEXT
//
// MessageText:
//
//  Extend Box to Next Word\nExtend the rectangle selection to the next word.
//
#define CMD_EDIT_ROWWORDNEXT             0x00010049L

//
// MessageId: CMD_EDIT_ROWWORDPREV
//
// MessageText:
//
//  Extend Box to Previuos Word\nExtend the rectangle selection to the previous word.
//
#define CMD_EDIT_ROWWORDPREV             0x0001004AL

//
// MessageId: CMD_EDIT_ROWWORDENDNEXT
//
// MessageText:
//
//  Extend Box to Next Word End\nExtend the rectangle selection to the next word end.
//
#define CMD_EDIT_ROWWORDENDNEXT          0x0001004BL

//
// MessageId: CMD_EDIT_ROWWORDENDPREV
//
// MessageText:
//
//  Extend Box to Previous Word End\nExtend the rectangle selection to the previous word end.
//
#define CMD_EDIT_ROWWORDENDPREV          0x0001004CL

//
// MessageId: CMD_EDIT_CHARPREV
//
// MessageText:
//
//  Previous Character\nGo to the previous character.
//
#define CMD_EDIT_CHARPREV                0x0001004DL

//
// MessageId: CMD_EDIT_WORDENDNEXT
//
// MessageText:
//
//  Next Word End\nGo to the next word end.
//
#define CMD_EDIT_WORDENDNEXT             0x0001004EL

//
// MessageId: CMD_EDIT_WORDENDPREV
//
// MessageText:
//
//  Previous Word End\nGo to the previous word end.
//
#define CMD_EDIT_WORDENDPREV             0x0001004FL

//
// MessageId: CMD_EDIT_WORDNEXT
//
// MessageText:
//
//  Next Word\nGo to the next word.
//
#define CMD_EDIT_WORDNEXT                0x00010050L

//
// MessageId: CMD_EDIT_WORDPREV
//
// MessageText:
//
//  Previous Word\nGo to the previous word.
//
#define CMD_EDIT_WORDPREV                0x00010051L

//
// MessageId: CMD_EDIT_LINEDOWN
//
// MessageText:
//
//  Down Line\nGo to the one down line.
//
#define CMD_EDIT_LINEDOWN                0x00010052L

//
// MessageId: CMD_EDIT_LINEUP
//
// MessageText:
//
//  Up Line\nGo to the one up line.
//
#define CMD_EDIT_LINEUP                  0x00010053L

//
// MessageId: CMD_EDIT_PAGEDOWN
//
// MessageText:
//
//  Down Page\nGo to the one down page.
//
#define CMD_EDIT_PAGEDOWN                0x00010054L

//
// MessageId: CMD_EDIT_PAGEUP
//
// MessageText:
//
//  Up Page\nGo to the one up page.
//
#define CMD_EDIT_PAGEUP                  0x00010055L

//
// MessageId: CMD_EDIT_HOMEEXTEND
//
// MessageText:
//
//  Extend to Start of Document\nExtend the selection to the start of the document.
//
#define CMD_EDIT_HOMEEXTEND              0x00010056L

//
// MessageId: CMD_EDIT_ENDEXTEND
//
// MessageText:
//
//  Extend to End of Document\nExtend the selection to the end of the document.
//
#define CMD_EDIT_ENDEXTEND               0x00010057L

//
// MessageId: CMD_EDIT_LINEHOMEEXTEND
//
// MessageText:
//
//  Extend to Start of Line\nExtend the selection to the start of the line.
//
#define CMD_EDIT_LINEHOMEEXTEND          0x00010058L

//
// MessageId: CMD_EDIT_LINEENDEXTEND
//
// MessageText:
//
//  Extend to End of Line\nExtend the selection to the end of the line.
//
#define CMD_EDIT_LINEENDEXTEND           0x00010059L

//
// MessageId: CMD_EDIT_CHARNEXT
//
// MessageText:
//
//  Extend to Next Character\nExtend the selection to the next character.
//
#define CMD_EDIT_CHARNEXT                0x0001005AL

//
// MessageId: CMD_EDIT_CHARPREV
//
// MessageText:
//
//  Extend to Previous Character\nExtend the selection to the previous character.
//
#define CMD_EDIT_CHARPREV                0x0001005BL

//
// MessageId: CMD_EDIT_WORDENDNEXTEXTEND
//
// MessageText:
//
//  Extend to Next Word End\nExtend the selection to the next word end.
//
#define CMD_EDIT_WORDENDNEXTEXTEND       0x0001005CL

//
// MessageId: CMD_EDIT_WORDENDPREVEXTEND
//
// MessageText:
//
//  Extend to Previous Word End\nExtend the selection to the previous word end.
//
#define CMD_EDIT_WORDENDPREVEXTEND       0x0001005DL

//
// MessageId: CMD_EDIT_WORDNEXTEXTEND
//
// MessageText:
//
//  Extend to Next Word\nExtend the selection to the next word.
//
#define CMD_EDIT_WORDNEXTEXTEND          0x0001005EL

//
// MessageId: CMD_EDIT_WORDPREVEXTEND
//
// MessageText:
//
//  Extend to Previous Word\nExtend the selection to the previous word.
//
#define CMD_EDIT_WORDPREVEXTEND          0x0001005FL

//
// MessageId: CMD_EDIT_LINEDOWNEXTEND
//
// MessageText:
//
//  Extend to Next Line\nExtend the selection to the next line.
//
#define CMD_EDIT_LINEDOWNEXTEND          0x00010060L

//
// MessageId: CMD_EDIT_LINEUPEXTEND
//
// MessageText:
//
//  Extend to Previous Line\nExtend the selection to the previous line.
//
#define CMD_EDIT_LINEUPEXTEND            0x00010061L

//
// MessageId: CMD_EDIT_PAGEDOWNEXTEND
//
// MessageText:
//
//  Extend to Next Page\nExtend the selection to the next page.
//
#define CMD_EDIT_PAGEDOWNEXTEND          0x00010062L

//
// MessageId: CMD_EDIT_PAGEUPEXTEND
//
// MessageText:
//
//  Extend to Previous Page\nExtend the selection to the previous page.
//
#define CMD_EDIT_PAGEUPEXTEND            0x00010063L

//
// MessageId: CMD_EDIT_SELECTALL
//
// MessageText:
//
//  Select &All\nSelect the entire document.
//
#define CMD_EDIT_SELECTALL               0x00010064L

//
// MessageId: CMD_EDIT_SELECTCURRENTWORD
//
// MessageText:
//
//  Select Current Word\nSelect the current word.
//
#define CMD_EDIT_SELECTCURRENTWORD       0x00010065L

//
// MessageId: CMD_EDIT_CANCELSELECTION
//
// MessageText:
//
//  Cancel Selection\nCancel the selection.
//
#define CMD_EDIT_CANCELSELECTION         0x00010066L

//
// MessageId: CMD_EDIT_SCROLLHOME
//
// MessageText:
//
//  Scroll to Start\nScroll the window to the top.
//
#define CMD_EDIT_SCROLLHOME              0x00010067L

//
// MessageId: CMD_EDIT_SCROLLEND
//
// MessageText:
//
//  Scroll to End\nScroll the window to the bottom.
//
#define CMD_EDIT_SCROLLEND               0x00010068L

//
// MessageId: CMD_EDIT_SCROLLLINEDOWN
//
// MessageText:
//
//  Scroll Down\nScroll the window one line down.
//
#define CMD_EDIT_SCROLLLINEDOWN          0x00010069L

//
// MessageId: CMD_EDIT_SCROLLLINEUP
//
// MessageText:
//
//  Scroll Up\nScroll the window one line up.
//
#define CMD_EDIT_SCROLLLINEUP            0x0001006AL

//
// MessageId: CMD_EDIT_SCROLLPAGEDOWN
//
// MessageText:
//
//  Scroll Page Down\nScroll the window one page down.
//
#define CMD_EDIT_SCROLLPAGEDOWN          0x0001006BL

//
// MessageId: CMD_EDIT_SCROLLPAGEUP
//
// MessageText:
//
//  Scroll Page Up\nScroll the window one page up.
//
#define CMD_EDIT_SCROLLPAGEUP            0x0001006CL

//
// MessageId: CMD_SEARCH_TOP
//
// MessageText:
//
//  &Search
//
#define CMD_SEARCH_TOP                   0x0001006DL

//
// MessageId: CMD_SEARCH_BOOKMARKS
//
// MessageText:
//
//  &Bookmarks
//
#define CMD_SEARCH_BOOKMARKS             0x0001006EL

//
// MessageId: CMD_SEARCH_FIND
//
// MessageText:
//
//  &Find and Replace...\nShow [Search and Replace] dialog.
//
#define CMD_SEARCH_FIND                  0x0001006FL

//
// MessageId: CMD_SEARCH_FINDNEXT
//
// MessageText:
//
//  Find &Next\nSearch next match.
//
#define CMD_SEARCH_FINDNEXT              0x00010070L

//
// MessageId: CMD_SEARCH_FINDPREV
//
// MessageText:
//
//  Find &Previous\nSearch previous match.
//
#define CMD_SEARCH_FINDPREV              0x00010071L

//
// MessageId: CMD_SEARCH_REPLACEALLINTERACTIVE
//
// MessageText:
//
//  Replace and Next\nReplace the selection and search next.
//
#define CMD_SEARCH_REPLACEALLINTERACTIVE 0x00010072L

//
// MessageId: CMD_SEARCH_REPLACEALL
//
// MessageText:
//
//  Replace All\nReplace all matches.
//
#define CMD_SEARCH_REPLACEALL            0x00010073L

//
// MessageId: CMD_SEARCH_BOOKMARKALL
//
// MessageText:
//
//  Mark All\nSet bookmarks on all matched lines.
//
#define CMD_SEARCH_BOOKMARKALL           0x00010074L

//
// MessageId: CMD_SEARCH_REVOKEMARK
//
// MessageText:
//
//  Revo&ke Highlight\nRevoke match highlight.
//
#define CMD_SEARCH_REVOKEMARK            0x00010075L

//
// MessageId: CMD_SEARCH_GOTOLINE
//
// MessageText:
//
//  &Go to Line...\nGo to specified line.
//
#define CMD_SEARCH_GOTOLINE              0x00010076L

//
// MessageId: CMD_SEARCH_TOGGLEBOOKMARK
//
// MessageText:
//
//  T&oggle Bookmark\nSet or remove a bookmark on current line.
//
#define CMD_SEARCH_TOGGLEBOOKMARK        0x00010077L

//
// MessageId: CMD_SEARCH_NEXTBOOKMARK
//
// MessageText:
//
//  Nex&t Bookmark\nGo to next bookmark.
//
#define CMD_SEARCH_NEXTBOOKMARK          0x00010078L

//
// MessageId: CMD_SEARCH_PREVBOOKMARK
//
// MessageText:
//
//  Pre&vious Bookmark\nGo to previous bookmark.
//
#define CMD_SEARCH_PREVBOOKMARK          0x00010079L

//
// MessageId: CMD_SEARCH_CLEARBOOKMARKS
//
// MessageText:
//
//  &Clear All Bookmarks\nRemove all bookmarks in the document.
//
#define CMD_SEARCH_CLEARBOOKMARKS        0x0001007AL

//
// MessageId: CMD_SEARCH_MANAGEBOOKMARKS
//
// MessageText:
//
//  Manage &Bookmarks...\nManage all bookmarks.
//
#define CMD_SEARCH_MANAGEBOOKMARKS       0x0001007BL

//
// MessageId: CMD_SEARCH_GOTOMATCHBRACKET
//
// MessageText:
//
//  Go to &Match Brace\nGo to the match brace.
//
#define CMD_SEARCH_GOTOMATCHBRACKET      0x0001007CL

//
// MessageId: CMD_SEARCH_EXTENDTOMATCHBRACKET
//
// MessageText:
//
//  E&xtend Match Brace\nExtend the selection to the match brace.
//
#define CMD_SEARCH_EXTENDTOMATCHBRACKET  0x0001007DL

//
// MessageId: CMD_SEARCH_INCREMENTALSEARCH
//
// MessageText:
//
//  &Incremental Search\nStart incremental search.
//
#define CMD_SEARCH_INCREMENTALSEARCH     0x0001007EL

//
// MessageId: CMD_SEARCH_INCREMENTALSEARCHR
//
// MessageText:
//
//  &Reverse Incremental Search\nStart reverse incremental search.
//
#define CMD_SEARCH_INCREMENTALSEARCHR    0x0001007FL

//
// MessageId: CMD_SEARCH_INCREMENTALSEARCHRF
//
// MessageText:
//
//  R&egular Expression Incremental Search\nStart regular expression incremental search.
//
#define CMD_SEARCH_INCREMENTALSEARCHRF   0x00010080L

//
// MessageId: CMD_SEARCH_INCREMENTALSEARCHRR
//
// MessageText:
//
//  Reverse Regular Expression Incremental Search\nStart reverse regular expression incremental search.
//
#define CMD_SEARCH_INCREMENTALSEARCHRR   0x00010081L

//
// MessageId: CMD_SEARCH_INCREMENTALSEARCHMF
//
// MessageText:
//
//  &Migemo Incremental Search\nStart Migemo incremental search.
//
#define CMD_SEARCH_INCREMENTALSEARCHMF   0x00010082L

//
// MessageId: CMD_SEARCH_INCREMENTALSEARCHMR
//
// MessageText:
//
//  Reverse Migem&o Incremental Search\nStart reverse Migemo incremental search.
//
#define CMD_SEARCH_INCREMENTALSEARCHMR   0x00010083L

//
// MessageId: CMD_SEARCH_FINDFILES
//
// MessageText:
//
//  Find Files...\nFind files with the given pattern.
//
#define CMD_SEARCH_FINDFILES             0x00010084L

//
// MessageId: CMD_SEARCH_SEARCHMULTIPLEFILES
//
// MessageText:
//
//  Search in Files...\nSearch pattern in multiple files.
//
#define CMD_SEARCH_SEARCHMULTIPLEFILES   0x00010085L

//
// MessageId: CMD_SEARCH_REPLACEMULTIPLEFILES
//
// MessageText:
//
//  Replace in Files...\nReplace text in multiple files.
//
#define CMD_SEARCH_REPLACEMULTIPLEFILES  0x00010086L

//
// MessageId: CMD_WINDOW_TOP
//
// MessageText:
//
//  &Window
//
#define CMD_WINDOW_TOP                   0x00010087L

//
// MessageId: CMD_VIEW_TOP
//
// MessageText:
//
//  &View
//
#define CMD_VIEW_TOP                     0x00010088L

//
// MessageId: CMD_VIEW_BUFFERS
//
// MessageText:
//
//  Bu&ffers
//
#define CMD_VIEW_BUFFERS                 0x00010089L

//
// MessageId: CMD_VIEW_TOOLBAR
//
// MessageText:
//
//  &Toolbar\nShow or hide the toolbar.
//
#define CMD_VIEW_TOOLBAR                 0x0001008AL

//
// MessageId: CMD_VIEW_STATUSBAR
//
// MessageText:
//
//  &Status Bar\nShow or hide the status bar.
//
#define CMD_VIEW_STATUSBAR               0x0001008BL

//
// MessageId: CMD_VIEW_WRAPNO
//
// MessageText:
//
//  N&o Wrap\nDo not wrap lines.
//
#define CMD_VIEW_WRAPNO                  0x0001008CL

//
// MessageId: CMD_VIEW_WRAPBYSPECIFIEDWIDTH
//
// MessageText:
//
//  Wrap by Sp&ecified Width(&E)\nWrap lines at specified position.
//
#define CMD_VIEW_WRAPBYSPECIFIEDWIDTH    0x0001008DL

//
// MessageId: CMD_VIEW_WRAPBYWINDOWWIDTH
//
// MessageText:
//
//  Wrap by &Window\nWrap lines at the end of window.
//
#define CMD_VIEW_WRAPBYWINDOWWIDTH       0x0001008EL

//
// MessageId: CMD_VIEW_REFRESH
//
// MessageText:
//
//  &Refresh\nRefresh the window.
//
#define CMD_VIEW_REFRESH                 0x0001008FL

//
// MessageId: CMD_VIEW_NEXTBUFFER
//
// MessageText:
//
//  &Next Buffer\nShow next buffer.
//
#define CMD_VIEW_NEXTBUFFER              0x00010090L

//
// MessageId: CMD_VIEW_PREVBUFFER
//
// MessageText:
//
//  &Previous Buffer\nShow previous buffer.
//
#define CMD_VIEW_PREVBUFFER              0x00010091L

//
// MessageId: CMD_VIEW_BUFFERBAR
//
// MessageText:
//
//  &Buffer Bar\nShow or hide the buffer bar.
//
#define CMD_VIEW_BUFFERBAR               0x00010092L

//
// MessageId: CMD_WINDOW_SPLITNS
//
// MessageText:
//
//  Sp&lit Window\nSplit the window into up and bottom.
//
#define CMD_WINDOW_SPLITNS               0x00010093L

//
// MessageId: CMD_WINDOW_SPLITWE
//
// MessageText:
//
//  Split Window Si&de-by-Side\nSplit the window into side-by-side.
//
#define CMD_WINDOW_SPLITWE               0x00010094L

//
// MessageId: CMD_WINDOW_UNSPLITACTIVE
//
// MessageText:
//
//  &Close Window\nClose active window.
//
#define CMD_WINDOW_UNSPLITACTIVE         0x00010095L

//
// MessageId: CMD_WINDOW_UNSPLITOTHERS
//
// MessageText:
//
//  Close Ot&her Windows\nClose all inactive windows.
//
#define CMD_WINDOW_UNSPLITOTHERS         0x00010096L

//
// MessageId: CMD_WINDOW_NEXTPANE
//
// MessageText:
//
//  Ne&xt Pane\nActivate next window.
//
#define CMD_WINDOW_NEXTPANE              0x00010097L

//
// MessageId: CMD_WINDOW_PREVPANE
//
// MessageText:
//
//  Pre&vious Pane\nActivate previous window.
//
#define CMD_WINDOW_PREVPANE              0x00010098L

//
// MessageId: CMD_WINDOW_TOPMOSTALWAYS
//
// MessageText:
//
//  &Always in Foreground\nShow the application window foreground always.
//
#define CMD_WINDOW_TOPMOSTALWAYS         0x00010099L

//
// MessageId: CMD_MACRO_TOP
//
// MessageText:
//
//  &Macros
//
#define CMD_MACRO_TOP                    0x0001009AL

//
// MessageId: CMD_MACRO_SCRIPTS
//
// MessageText:
//
//  &Scripts
//
#define CMD_MACRO_SCRIPTS                0x0001009BL

//
// MessageId: CMD_MACRO_DEFINE
//
// MessageText:
//
//  Start/End &Definition\nStart or end the keyboard macro definition.
//
#define CMD_MACRO_DEFINE                 0x0001009CL

//
// MessageId: CMD_MACRO_EXECUTE
//
// MessageText:
//
//  E&xecute\nExecute the active keyboard macro.
//
#define CMD_MACRO_EXECUTE                0x0001009DL

//
// MessageId: CMD_MACRO_APPEND
//
// MessageText:
//
//  &Append\nAppend definition to the active keyboard macro.
//
#define CMD_MACRO_APPEND                 0x0001009EL

//
// MessageId: CMD_MACRO_PAUSERESTART
//
// MessageText:
//
//  &Pause/Restart\nPause or restart the keyboard macro definition.
//
#define CMD_MACRO_PAUSERESTART           0x0001009FL

//
// MessageId: CMD_MACRO_INSERTQUERY
//
// MessageText:
//
//  Insert &Query\nInsert an user prompt into the defining keyboard macro.
//
#define CMD_MACRO_INSERTQUERY            0x000100A0L

//
// MessageId: CMD_MACRO_ABORT
//
// MessageText:
//
//  A&bort\nAbort the keyboard macro definition.
//
#define CMD_MACRO_ABORT                  0x000100A1L

//
// MessageId: CMD_MACRO_SAVEAS
//
// MessageText:
//
//  Sa&ve As...\nSave the active keyboard macro with name.
//
#define CMD_MACRO_SAVEAS                 0x000100A2L

//
// MessageId: CMD_MACRO_LOAD
//
// MessageText:
//
//  &Load...\nLoad keyboard macro from a file.
//
#define CMD_MACRO_LOAD                   0x000100A3L

//
// MessageId: CMD_TOOL_TOP
//
// MessageText:
//
//  &Tool
//
#define CMD_TOOL_TOP                     0x000100A4L

//
// MessageId: CMD_TOOL_APPDOCTYPES
//
// MessageText:
//
//  &Apply Document Type
//
#define CMD_TOOL_APPDOCTYPES             0x000100A5L

//
// MessageId: CMD_TOOL_COMMONOPTION
//
// MessageText:
//
//  &Common Options...\nSet the options common to all document types.
//
#define CMD_TOOL_COMMONOPTION            0x000100A6L

//
// MessageId: CMD_TOOL_DOCTYPEOPTION
//
// MessageText:
//
//  Document T&ype Options...\nSet the options specific to the active document type.
//
#define CMD_TOOL_DOCTYPEOPTION           0x000100A7L

//
// MessageId: CMD_TOOL_FONT
//
// MessageText:
//
//  &Font...\nSet font settings.
//
#define CMD_TOOL_FONT                    0x000100A8L

//
// MessageId: CMD_TOOL_EXECUTE
//
// MessageText:
//
//  &Execute\nExecute document type specific program.
//
#define CMD_TOOL_EXECUTE                 0x000100A9L

//
// MessageId: CMD_TOOL_EXECUTECOMMAND
//
// MessageText:
//
//  E&xecute Command...\nExecute external command.
//
#define CMD_TOOL_EXECUTECOMMAND          0x000100AAL

//
// MessageId: CMD_HELP_TOP
//
// MessageText:
//
//  &Help
//
#define CMD_HELP_TOP                     0x000100ABL

//
// MessageId: CMD_HELP_ABOUT
//
// MessageText:
//
//  &About\nDisplay information and version of Alpha.
//
#define CMD_HELP_ABOUT                   0x000100ACL

// encoding names
//
// MessageId: 0x00020025L (No symbolic name defined)
//
// MessageText:
//
//  US-Canada (IBM EBCDIC)
//


//
// MessageId: 0x000201B5L (No symbolic name defined)
//
// MessageText:
//
//  United States (ibm-437)
//


//
// MessageId: 0x000201F4L (No symbolic name defined)
//
// MessageText:
//
//  International (IBM EBCDIC)
//


//
// MessageId: 0x000202C4L (No symbolic name defined)
//
// MessageText:
//
//  Arabic (ASMO)
//


//
// MessageId: 0x000202C5L (No symbolic name defined)
//
// MessageText:
//
//  Arabic (ASMO 449+, BCON V4)
//


//
// MessageId: 0x000202C6L (No symbolic name defined)
//
// MessageText:
//
//  Arabic (Transparent Arabic)
//


//
// MessageId: 0x000202D0L (No symbolic name defined)
//
// MessageText:
//
//  Arabic (Transparent ASMO)
//


//
// MessageId: 0x000202E1L (No symbolic name defined)
//
// MessageText:
//
//  Greek (437G, ibm-737)
//


//
// MessageId: 0x00020307L (No symbolic name defined)
//
// MessageText:
//
//  Baltic (ibm-775)
//


//
// MessageId: 0x00020352L (No symbolic name defined)
//
// MessageText:
//
//  Western European (ibm-850)
//


//
// MessageId: 0x00020354L (No symbolic name defined)
//
// MessageText:
//
//  Central European (ibm-852)
//


//
// MessageId: 0x00020357L (No symbolic name defined)
//
// MessageText:
//
//  Cyrillic (ibm-855)
//


//
// MessageId: 0x00020359L (No symbolic name defined)
//
// MessageText:
//
//  Turkish (ibm-857)
//


//
// MessageId: 0x0002035AL (No symbolic name defined)
//
// MessageText:
//
//  Multilingual Latin 1 + European (ibm-858)
//


//
// MessageId: 0x0002035CL (No symbolic name defined)
//
// MessageText:
//
//  Portuguese (ibm-860)
//


//
// MessageId: 0x0002035DL (No symbolic name defined)
//
// MessageText:
//
//  Icelandic (ibm-861)
//


//
// MessageId: 0x0002035EL (No symbolic name defined)
//
// MessageText:
//
//  Hebrew (ibm-862)
//


//
// MessageId: 0x0002035FL (No symbolic name defined)
//
// MessageText:
//
//  Canada-France (ibm-863)
//


//
// MessageId: 0x00020360L (No symbolic name defined)
//
// MessageText:
//
//  Arabic (ibm-864)
//


//
// MessageId: 0x00020361L (No symbolic name defined)
//
// MessageText:
//
//  Northern European (ibm-865)
//


//
// MessageId: 0x00020362L (No symbolic name defined)
//
// MessageText:
//
//  Russian (ibm-866)
//


//
// MessageId: 0x00020365L (No symbolic name defined)
//
// MessageText:
//
//  Modern Greek (ibm-869)
//


//
// MessageId: 0x00020366L (No symbolic name defined)
//
// MessageText:
//
//  Multilingual-RORCE (Latin-2, IBM EBCDIC)
//


//
// MessageId: 0x0002036AL (No symbolic name defined)
//
// MessageText:
//
//  Thai (ibm-874)
//


//
// MessageId: 0x0002036BL (No symbolic name defined)
//
// MessageText:
//
//  Modern Greek (IBM EBCDIC)
//


//
// MessageId: 0x000203A4L (No symbolic name defined)
//
// MessageText:
//
//  Japanese (Shift JIS, windows-932)
//


//
// MessageId: 0x000203A8L (No symbolic name defined)
//
// MessageText:
//
//  Simplified Chinese (GBK, windows-936)
//


//
// MessageId: 0x000203B5L (No symbolic name defined)
//
// MessageText:
//
//  Korean (windows-949)
//


//
// MessageId: 0x000203B6L (No symbolic name defined)
//
// MessageText:
//
//  Traditional Chinese (Big5, windows-950)
//


//
// MessageId: 0x00020402L (No symbolic name defined)
//
// MessageText:
//
//  Turkish (IBM EBCDIC)
//


//
// MessageId: 0x00020417L (No symbolic name defined)
//
// MessageText:
//
//  Latin-1/Open System (IBM EBCDIC)
//


//
// MessageId: 0x00020474L (No symbolic name defined)
//
// MessageText:
//
//  US-Canada (37 + Euro, IBM EBCDIC)
//


//
// MessageId: 0x00020475L (No symbolic name defined)
//
// MessageText:
//
//  Germany (20273 + Euro, IBM EBCDIC)
//


//
// MessageId: 0x00020476L (No symbolic name defined)
//
// MessageText:
//
//  Denmark-Norway (20277 + Euro, IBM EBCDIC)
//


//
// MessageId: 0x00020477L (No symbolic name defined)
//
// MessageText:
//
//  Finland-Sweden (20278 + Euro, IBM EBCDIC)
//


//
// MessageId: 0x00020478L (No symbolic name defined)
//
// MessageText:
//
//  Italy (20280 + Euro, IBM EBCDIC)
//


//
// MessageId: 0x00020479L (No symbolic name defined)
//
// MessageText:
//
//  Latin America-Spain (20284 + Euro, IBM EBCDIC)
//


//
// MessageId: 0x0002047AL (No symbolic name defined)
//
// MessageText:
//
//  UK (20285 + Euro, IBM EBCDIC)
//


//
// MessageId: 0x0002047BL (No symbolic name defined)
//
// MessageText:
//
//  France (20297 + Euro, IBM EBCDIC)
//


//
// MessageId: 0x0002047CL (No symbolic name defined)
//
// MessageText:
//
//  International (500 + Euro, IBM EBCDIC)
//


//
// MessageId: 0x0002047DL (No symbolic name defined)
//
// MessageText:
//
//  Icelandic (20871 + Euro, IBM EBCDIC)
//


//
// MessageId: 0x000204B0L (No symbolic name defined)
//
// MessageText:
//
//  Unicode (UTF-16)
//


//
// MessageId: 0x000204B1L (No symbolic name defined)
//
// MessageText:
//
//  Unicode (UTF-16 big endian)
//


//
// MessageId: 0x000204E2L (No symbolic name defined)
//
// MessageText:
//
//  Central European (windows-1250)
//


//
// MessageId: 0x000204E3L (No symbolic name defined)
//
// MessageText:
//
//  Cyrillic (windows-1251)
//


//
// MessageId: 0x000204E4L (No symbolic name defined)
//
// MessageText:
//
//  Western European (windows-1252)
//


//
// MessageId: 0x000204E5L (No symbolic name defined)
//
// MessageText:
//
//  Greek (windows-1253)
//


//
// MessageId: 0x000204E6L (No symbolic name defined)
//
// MessageText:
//
//  Turkish (windows-1254)
//


//
// MessageId: 0x000204E7L (No symbolic name defined)
//
// MessageText:
//
//  Hebrew (windows-1255)
//


//
// MessageId: 0x000204E8L (No symbolic name defined)
//
// MessageText:
//
//  Arabic (windows-1256)
//


//
// MessageId: 0x000204E9L (No symbolic name defined)
//
// MessageText:
//
//  Baltic (windows-1257)
//


//
// MessageId: 0x000204EAL (No symbolic name defined)
//
// MessageText:
//
//  Vietnamese (windows-1258)
//


//
// MessageId: 0x00020551L (No symbolic name defined)
//
// MessageText:
//
//  Korean (Johab)
//


//
// MessageId: 0x00022710L (No symbolic name defined)
//
// MessageText:
//
//  Roman (Macintosh)
//


//
// MessageId: 0x00022711L (No symbolic name defined)
//
// MessageText:
//
//  Japanese (Macintosh)
//


//
// MessageId: 0x00022712L (No symbolic name defined)
//
// MessageText:
//
//  Traditional Chinese (Macintosh)
//


//
// MessageId: 0x00022713L (No symbolic name defined)
//
// MessageText:
//
//  Korean (Macintosh)
//


//
// MessageId: 0x00022714L (No symbolic name defined)
//
// MessageText:
//
//  Arabic (Macintosh)
//


//
// MessageId: 0x00022715L (No symbolic name defined)
//
// MessageText:
//
//  Hebrew (Macintosh)
//


//
// MessageId: 0x00022716L (No symbolic name defined)
//
// MessageText:
//
//  Greek I (Macintosh)
//


//
// MessageId: 0x00022717L (No symbolic name defined)
//
// MessageText:
//
//  Cyrillic (Macintosh)
//


//
// MessageId: 0x00022718L (No symbolic name defined)
//
// MessageText:
//
//  Simplified Chinese (GB2312, Macintosh)
//


//
// MessageId: 0x0002271AL (No symbolic name defined)
//
// MessageText:
//
//  Rumanian (Macintosh)
//


//
// MessageId: 0x00022721L (No symbolic name defined)
//
// MessageText:
//
//  Ukrainian (Macintosh)
//


//
// MessageId: 0x00022725L (No symbolic name defined)
//
// MessageText:
//
//  Thai (Macintosh)
//


//
// MessageId: 0x0002272DL (No symbolic name defined)
//
// MessageText:
//
//  Central European (Macintosh)
//


//
// MessageId: 0x0002275FL (No symbolic name defined)
//
// MessageText:
//
//  Icelandic (Macintosh)
//


//
// MessageId: 0x00022761L (No symbolic name defined)
//
// MessageText:
//
//  Turkish (Macintosh)
//


//
// MessageId: 0x00022762L (No symbolic name defined)
//
// MessageText:
//
//  Croatian (Macintosh)
//


//
// MessageId: 0x00022EE0L (No symbolic name defined)
//
// MessageText:
//
//  Unicode (UTF-32)
//


//
// MessageId: 0x00022EE1L (No symbolic name defined)
//
// MessageText:
//
//  Unicode (UTF-32 big endian)
//


//
// MessageId: 0x00024E20L (No symbolic name defined)
//
// MessageText:
//
//  Taiwanese (CNS)
//


//
// MessageId: 0x00024E21L (No symbolic name defined)
//
// MessageText:
//
//  Taiwanese (TCA)
//


//
// MessageId: 0x00024E22L (No symbolic name defined)
//
// MessageText:
//
//  Taiwanese (Eten)
//


//
// MessageId: 0x00024E23L (No symbolic name defined)
//
// MessageText:
//
//  Taiwanese (IBM5550)
//


//
// MessageId: 0x00024E24L (No symbolic name defined)
//
// MessageText:
//
//  Taiwanese (TeleText)
//


//
// MessageId: 0x00024E25L (No symbolic name defined)
//
// MessageText:
//
//  Taiwanese (Wang)
//


//
// MessageId: 0x00024E89L (No symbolic name defined)
//
// MessageText:
//
//  IRV International Alphabets No.5 (IA5)
//


//
// MessageId: 0x00024E8AL (No symbolic name defined)
//
// MessageText:
//
//  German (IA5)
//


//
// MessageId: 0x00024E8BL (No symbolic name defined)
//
// MessageText:
//
//  Swedish (IA5)
//


//
// MessageId: 0x00024E8CL (No symbolic name defined)
//
// MessageText:
//
//  Norwegian (IA5)
//


//
// MessageId: 0x00024E9FL (No symbolic name defined)
//
// MessageText:
//
//  US-ASCII
//


//
// MessageId: 0x00024F25L (No symbolic name defined)
//
// MessageText:
//
//  T.61
//


//
// MessageId: 0x00024F2DL (No symbolic name defined)
//
// MessageText:
//
//  Non-Spacing Accent (ISO 6937)
//


//
// MessageId: 0x00024F31L (No symbolic name defined)
//
// MessageText:
//
//  Germany (IBM EBCDIC)
//


//
// MessageId: 0x00024F35L (No symbolic name defined)
//
// MessageText:
//
//  Denmark-Norway (IBM EBCDIC)
//


//
// MessageId: 0x00024F36L (No symbolic name defined)
//
// MessageText:
//
//  Finland-Sweden (IBM EBCDIC)
//


//
// MessageId: 0x00024F38L (No symbolic name defined)
//
// MessageText:
//
//  Italy (IBM EBCDIC)
//


//
// MessageId: 0x00024F3CL (No symbolic name defined)
//
// MessageText:
//
//  Latin American-Spain (IBM EBCDIC)
//


//
// MessageId: 0x00024F3DL (No symbolic name defined)
//
// MessageText:
//
//  UK (IBM EBCDIC)
//


//
// MessageId: 0x00024F42L (No symbolic name defined)
//
// MessageText:
//
//  Japanese (Katakana, IBM EBCDIC)
//


//
// MessageId: 0x00024F49L (No symbolic name defined)
//
// MessageText:
//
//  France (IBM EBCDIC)
//


//
// MessageId: 0x00024FC4L (No symbolic name defined)
//
// MessageText:
//
//  Arabic (IBM EBCDIC)
//


//
// MessageId: 0x00024FC7L (No symbolic name defined)
//
// MessageText:
//
//  Greek (IBM EBCDIC)
//


//
// MessageId: 0x00024FC8L (No symbolic name defined)
//
// MessageText:
//
//  Hebrew (IBM EBCDIC)
//


//
// MessageId: 0x00025161L (No symbolic name defined)
//
// MessageText:
//
//  Korean and Korean Extended (IBM EBCDIC)
//


//
// MessageId: 0x00025166L (No symbolic name defined)
//
// MessageText:
//
//  Thai (IBM EBCDIC)
//


//
// MessageId: 0x00025182L (No symbolic name defined)
//
// MessageText:
//
//  Russian (KOI8)
//


//
// MessageId: 0x00025187L (No symbolic name defined)
//
// MessageText:
//
//  Icelandic (IBM EBCDIC)
//


//
// MessageId: 0x00025190L (No symbolic name defined)
//
// MessageText:
//
//  Cyrillic (Russian, IBM EBCDIC)
//


//
// MessageId: 0x000251A9L (No symbolic name defined)
//
// MessageText:
//
//  Turkish (IBM EBCDIC)
//


//
// MessageId: 0x000251BCL (No symbolic name defined)
//
// MessageText:
//
//  Latin-1/Open System (1047 + Euro, IBM EBCDIC)
//


//
// MessageId: 0x000251C4L (No symbolic name defined)
//
// MessageText:
//
//  Japanese (JIS X 0208-1990 & 0212-1990, windows-20932)
//


//
// MessageId: 0x000251C8L (No symbolic name defined)
//
// MessageText:
//
//  Simplified Chinese (GB2312)
//


//
// MessageId: 0x00025221L (No symbolic name defined)
//
// MessageText:
//
//  Cyrillic (Serbian-Bulgarian, IBM EBCDIC)
//


//
// MessageId: 0x00025223L (No symbolic name defined)
//
// MessageText:
//
//  Ext Alpha Lowercase
//


//
// MessageId: 0x0002556AL (No symbolic name defined)
//
// MessageText:
//
//  Ukrainian (KOI8-U)
//


//
// MessageId: 0x00026FAFL (No symbolic name defined)
//
// MessageText:
//
//  Western European (ISO-8859-1)
//


//
// MessageId: 0x00026FB0L (No symbolic name defined)
//
// MessageText:
//
//  Central European (ISO-8859-2)
//


//
// MessageId: 0x00026FB1L (No symbolic name defined)
//
// MessageText:
//
//  Southern European (ISO-8859-3)
//


//
// MessageId: 0x00026FB2L (No symbolic name defined)
//
// MessageText:
//
//  Baltic (ISO-8859-4)
//


//
// MessageId: 0x00026FB3L (No symbolic name defined)
//
// MessageText:
//
//  Cyrillic (ISO-8859-5)
//


//
// MessageId: 0x00026FB4L (No symbolic name defined)
//
// MessageText:
//
//  Arabic (ISO-8859-6)
//


//
// MessageId: 0x00026FB5L (No symbolic name defined)
//
// MessageText:
//
//  Greek (ISO-8859-7)
//


//
// MessageId: 0x00026FB6L (No symbolic name defined)
//
// MessageText:
//
//  Hebrew (ISO-8859-8-Visual)
//


//
// MessageId: 0x00026FB7L (No symbolic name defined)
//
// MessageText:
//
//  Turkish (ISO-8859-9)
//


//
// MessageId: 0x00026FB8L (No symbolic name defined)
//
// MessageText:
//
//  Northern European (ISO-8859-10)
//


//
// MessageId: 0x00026FB9L (No symbolic name defined)
//
// MessageText:
//
//  Thai (ISO-8859-11)
//


//
// MessageId: 0x00026FBBL (No symbolic name defined)
//
// MessageText:
//
//  Baltic (ISO-8859-13)
//


//
// MessageId: 0x00026FBCL (No symbolic name defined)
//
// MessageText:
//
//  Keltic (ISO-8859-14)
//


//
// MessageId: 0x00026FBDL (No symbolic name defined)
//
// MessageText:
//
//  Western European (ISO-8859-15)
//


//
// MessageId: 0x00026FBEL (No symbolic name defined)
//
// MessageText:
//
//  Central European (ISO-8859-16)
//


//
// MessageId: 0x00027149L (No symbolic name defined)
//
// MessageText:
//
//  Europa 3
//


//
// MessageId: 0x000296C6L (No symbolic name defined)
//
// MessageText:
//
//  Hebrew (ISO-8859-8-Logical)
//


//
// MessageId: 0x0002C351L (No symbolic name defined)
//
// MessageText:
//
//  Auto-Select
//


//
// MessageId: 0x0002C42CL (No symbolic name defined)
//
// MessageText:
//
//  Japanese (ISO-2022-JP, windows-50220)
//


//
// MessageId: 0x0002C42DL (No symbolic name defined)
//
// MessageText:
//
//  Japanese (ISO-2022-JP-Allow 1 byte Kana, windows-50221)
//


//
// MessageId: 0x0002C42EL (No symbolic name defined)
//
// MessageText:
//
//  Japanese (ISO-2022-JP, JIS X 0201-1989, windows-50222)
//


//
// MessageId: 0x0002C431L (No symbolic name defined)
//
// MessageText:
//
//  Korean (ISO-2022-KR)
//


//
// MessageId: 0x0002C433L (No symbolic name defined)
//
// MessageText:
//
//  Simplified Chinese (ISO-2022-CN)
//


//
// MessageId: 0x0002C435L (No symbolic name defined)
//
// MessageText:
//
//  Traditional Chinese (ISO-2022-CN)
//


//
// MessageId: 0x0002C6F2L (No symbolic name defined)
//
// MessageText:
//
//  Japanese (Katakana Extended)
//


//
// MessageId: 0x0002C6F3L (No symbolic name defined)
//
// MessageText:
//
//  Japanese + US-Canada
//


//
// MessageId: 0x0002C6F4L (No symbolic name defined)
//
// MessageText:
//
//  Japanese (Auto-Select)
//


//
// MessageId: 0x0002C6F5L (No symbolic name defined)
//
// MessageText:
//
//  Korean + Korean Extended
//


//
// MessageId: 0x0002C6F7L (No symbolic name defined)
//
// MessageText:
//
//  Simplified Chinese + Simplified Chinese Extended
//


//
// MessageId: 0x0002C6F8L (No symbolic name defined)
//
// MessageText:
//
//  Simplified Chinese
//


//
// MessageId: 0x0002C6F9L (No symbolic name defined)
//
// MessageText:
//
//  Simplified Chinese + US-Canada
//


//
// MessageId: 0x0002C6FBL (No symbolic name defined)
//
// MessageText:
//
//  Japanese + Japanese Latin Extended
//


//
// MessageId: 0x0002C705L (No symbolic name defined)
//
// MessageText:
//
//  Korean (Auto-Select)
//


//
// MessageId: 0x0002CADCL (No symbolic name defined)
//
// MessageText:
//
//  Japanese (EUC, windows-51932)
//


//
// MessageId: 0x0002CAE0L (No symbolic name defined)
//
// MessageText:
//
//  Simplified Chinese (EUC, windows-51936)
//


//
// MessageId: 0x0002CAEDL (No symbolic name defined)
//
// MessageText:
//
//  Korean (EUC, windows-51949)
//


//
// MessageId: 0x0002CAEEL (No symbolic name defined)
//
// MessageText:
//
//  Traditional Chinese (EUC, windows-51950)
//


//
// MessageId: 0x0002CEC8L (No symbolic name defined)
//
// MessageText:
//
//  Simplified Chinese (HZ-GB2312)
//


//
// MessageId: 0x0002D698L (No symbolic name defined)
//
// MessageText:
//
//  Simplified Chinese (GB18030)
//


//
// MessageId: 0x0002DEAAL (No symbolic name defined)
//
// MessageText:
//
//  Hindi (ISCII, Devanagari)
//


//
// MessageId: 0x0002DEABL (No symbolic name defined)
//
// MessageText:
//
//  Bengali (ISCII)
//


//
// MessageId: 0x0002DEACL (No symbolic name defined)
//
// MessageText:
//
//  Tamil (ISCII)
//


//
// MessageId: 0x0002DEADL (No symbolic name defined)
//
// MessageText:
//
//  Telugu (ISCII)
//


//
// MessageId: 0x0002DEAEL (No symbolic name defined)
//
// MessageText:
//
//  Assamese (ISCII)
//


//
// MessageId: 0x0002DEAFL (No symbolic name defined)
//
// MessageText:
//
//  Oriya (ISCII)
//


//
// MessageId: 0x0002DEB0L (No symbolic name defined)
//
// MessageText:
//
//  Kannada (ISCII)
//


//
// MessageId: 0x0002DEB1L (No symbolic name defined)
//
// MessageText:
//
//  Malayalam (ISCII)
//


//
// MessageId: 0x0002DEB2L (No symbolic name defined)
//
// MessageText:
//
//  Gujarathi (ISCII)
//


//
// MessageId: 0x0002DEB3L (No symbolic name defined)
//
// MessageText:
//
//  Panjabi (ISCII, Gurmukhi)
//


//
// MessageId: 0x0002FDE8L (No symbolic name defined)
//
// MessageText:
//
//  Unicode (UTF-7)
//


//
// MessageId: 0x0002FDE9L (No symbolic name defined)
//
// MessageText:
//
//  Unicode (UTF-8)
//


// extended encoding names
//
// MessageId: 0x00032710L (No symbolic name defined)
//
// MessageText:
//
//  Auto-Select (System Language)
//


//
// MessageId: 0x00032711L (No symbolic name defined)
//
// MessageText:
//
//  Auto-Select (User Language)
//


//
// MessageId: 0x0003271AL (No symbolic name defined)
//
// MessageText:
//
//  Unicode (Auto-Select)
//


//
// MessageId: 0x0003271BL (No symbolic name defined)
//
// MessageText:
//
//  Unicode (UTF-5)
//


//
// MessageId: 0x00032724L (No symbolic name defined)
//
// MessageText:
//
//  Armenian (Auto-Select)
//


//
// MessageId: 0x00032725L (No symbolic name defined)
//
// MessageText:
//
//  Armenian (ARMSCII-7)
//


//
// MessageId: 0x00032726L (No symbolic name defined)
//
// MessageText:
//
//  Armenian (ARMSCII-8)
//


//
// MessageId: 0x00032727L (No symbolic name defined)
//
// MessageText:
//
//  Armenian (ARMSCII-8A)
//


//
// MessageId: 0x0003272EL (No symbolic name defined)
//
// MessageText:
//
//  Vietnamese (Auto-Select)
//


//
// MessageId: 0x0003272FL (No symbolic name defined)
//
// MessageText:
//
//  Vietnamese (TCVN)
//


//
// MessageId: 0x00032730L (No symbolic name defined)
//
// MessageText:
//
//  Vietnamese (VISCII)
//


//
// MessageId: 0x00032731L (No symbolic name defined)
//
// MessageText:
//
//  Vietnamese (VPS)
//


//
// MessageId: 0x00032738L (No symbolic name defined)
//
// MessageText:
//
//  Japanese (ISO-2022-JP)
//


//
// MessageId: 0x00032739L (No symbolic name defined)
//
// MessageText:
//
//  Japanese (Shift JIS)
//


//
// MessageId: 0x0003273AL (No symbolic name defined)
//
// MessageText:
//
//  Japanese (ISO-2022-JP-1)
//


//
// MessageId: 0x0003273BL (No symbolic name defined)
//
// MessageText:
//
//  Japanese (ISO-2022-JP-2)
//


//
// MessageId: 0x0003273CL (No symbolic name defined)
//
// MessageText:
//
//  Japanese (EUC)
//


//
// MessageId: 0x0003273DL (No symbolic name defined)
//
// MessageText:
//
//  Japanese (ISO-2022-JP-2004)
//


//
// MessageId: 0x0003273EL (No symbolic name defined)
//
// MessageText:
//
//  Japanese (ISO-2022-JP-2004-Strict)
//


//
// MessageId: 0x0003273FL (No symbolic name defined)
//
// MessageText:
//
//  Japanese (ISO-2022-JP-2004-Compatible)
//


//
// MessageId: 0x00032740L (No symbolic name defined)
//
// MessageText:
//
//  Japanese (ISO-2022-JP-3)
//


//
// MessageId: 0x00032741L (No symbolic name defined)
//
// MessageText:
//
//  Japanese (ISO-2022-JP-3-Strict)
//


//
// MessageId: 0x00032742L (No symbolic name defined)
//
// MessageText:
//
//  Japanese (ISO-2022-JP-3-Compatible)
//


//
// MessageId: 0x00032743L (No symbolic name defined)
//
// MessageText:
//
//  Japanese (Shift_JIS-2004)
//


//
// MessageId: 0x00032744L (No symbolic name defined)
//
// MessageText:
//
//  Japanese (EUC-JIS-2004)
//


//
// MessageId: 0x0003274CL (No symbolic name defined)
//
// MessageText:
//
//  Multilingual (ISO-2022, 7bits)
//


//
// MessageId: 0x0003274DL (No symbolic name defined)
//
// MessageText:
//
//  Multilingual (ISO-2022, 7bits, SS2)
//


//
// MessageId: 0x0003274EL (No symbolic name defined)
//
// MessageText:
//
//  Multilingual (ISO-2022, 7bits, SI/SO)
//


//
// MessageId: 0x0003274FL (No symbolic name defined)
//
// MessageText:
//
//  Multilingual (ISO-2022, 8bits, SS2)
//


//
// MessageId: 0x00032756L (No symbolic name defined)
//
// MessageText:
//
//  Binary
//


//
// MessageId: 0x00032757L (No symbolic name defined)
//
// MessageText:
//
//  NextSTEP
//


//
// MessageId: 0x00032758L (No symbolic name defined)
//
// MessageText:
//
//  Atari ST/TT
//


//
// MessageId: 0x00032760L (No symbolic name defined)
//
// MessageText:
//
//  Thai (TIS 620-2533:1990)
//


//
// MessageId: 0x0003276AL (No symbolic name defined)
//
// MessageText:
//
//  Lao (MuleLao-1)
//


//
// MessageId: 0x0003276BL (No symbolic name defined)
//
// MessageText:
//
//  Lao (ibm-1132)
//


//
// MessageId: 0x0003276CL (No symbolic name defined)
//
// MessageText:
//
//  Lao (ibm-1133)
//


//
// MessageId: 0x00032774L (No symbolic name defined)
//
// MessageText:
//
//  Irelandic (I.S. 434:1999)
//


//
// MessageId: 0x0003277EL (No symbolic name defined)
//
// MessageText:
//
//  Tamil (TAB)
//


//
// MessageId: 0x0003277FL (No symbolic name defined)
//
// MessageText:
//
//  Tamil (TAM)
//


//
// MessageId: 0x00032780L (No symbolic name defined)
//
// MessageText:
//
//  Tamil (TSCII 1.7)
//


//
// MessageId: 0x00032783L (No symbolic name defined)
//
// MessageText:
//
//  Hindi (Macintosh, Devanagari)
//


//
// MessageId: 0x00032784L (No symbolic name defined)
//
// MessageText:
//
//  Gujarathi (Macintosh)
//


//
// MessageId: 0x00032785L (No symbolic name defined)
//
// MessageText:
//
//  Panjabi (Macintosh, Gurmukhi)
//


