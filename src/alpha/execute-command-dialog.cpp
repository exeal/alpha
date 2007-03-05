/**
 * @file execute-command-dialog.cpp
 * @author exeal
 * @date 2003-2005
 */

#include "StdAfx.h"
#include "execute-command-dialog.hpp"
#include "application.hpp"
using alpha::Alpha;
using alpha::ui::ExecuteCommandDlg;
using namespace manah;
using namespace std;


// このクラスは将来削除する

const uint MAX_HISTORY_LENGTH = 16;


/// コンストラクタ
ExecuteCommandDlg::ExecuteCommandDlg(Alpha& app, HFONT ioFont) : app_(app), ioFont_(ioFont), executing_(false), interrupted_(false) {
}

/// 入力キューに追加
void ExecuteCommandDlg::appendInput(const char* first, const char* last) {
	inputQueue_.push(string(first, last));
}

/// 出力コントロールに文字列を書き出す
void ExecuteCommandDlg::appendOutput(const char* first, const char* last) {
	ostringstream output;
	const char* p = first;
	while(p < last) {
		p = std::find(first, last, '\n');
		if(p == last) {
			output.write(first, static_cast<streamsize>(p - first));
			break;
		} else if(p == first)
			output << '\r\n';
		else if(p[-1] == '\r')
			output.write(first, static_cast<streamsize>(p - first + 1));
		else {
			output.write(first, static_cast<streamsize>(p - first));
			output << '\r\n';
		}
		first = p + 1;
	}

	const string s = output.str();
	const int len = ::MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, s.c_str(), static_cast<int>(s.length() + 1), 0, 0);
	const wchar_t* buffer = new wchar_t[len];
	::MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, s.c_str(), static_cast<int>(s.length() + 1), const_cast<wchar_t*>(buffer), len);
	outputTextbox_.setSel(-1, -1);
	outputTextbox_.replaceSel(buffer);
	delete[] buffer;
}

/// コマンドを実行する
bool ExecuteCommandDlg::execute(const wstring& commandLine, bool consoleProgram) {
	SECURITY_ATTRIBUTES	sa;
	HANDLE stdoutReadPipe = 0, stdoutWritePipe = 0;
	HANDLE stdinReadPipe, stdinWritePipe;

	memset(&sa, 0, sizeof(SECURITY_ATTRIBUTES));
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = true;
	if(!toBoolean(::CreatePipe(&stdoutReadPipe, &stdoutWritePipe, &sa, 0))
			|| !toBoolean(::CreatePipe(&stdinReadPipe, &stdinWritePipe, &sa, 0))) {
		::CloseHandle(stdoutReadPipe);
		::CloseHandle(stdoutWritePipe);
		return false;
	}

	STARTUPINFO	si;
	memset(&si, 0, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	if(consoleProgram) {
		si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
		si.hStdError = stdoutWritePipe;
		si.hStdInput = stdinReadPipe;
		si.hStdOutput = stdoutWritePipe;
		si.wShowWindow = SW_HIDE;
	}

	bool startupFailed = true;
	win32::AutoZero<::PROCESS_INFORMATION> pi;

	if(!consoleProgram) {	// GUI プログラム
		if(toBoolean(::CreateProcessW(0,
				const_cast<wchar_t*>(commandLine.c_str()), 0, 0, true, CREATE_DEFAULT_ERROR_MODE, 0, 0, &si, &pi)))
			startupFailed = false;
	} else if(toBoolean(::CreateProcessW(0,	// コンソールプログラム
			const_cast<wchar_t*>((L"cmd /C " + commandLine).c_str()), 0, 0, true,
			CREATE_DEFAULT_ERROR_MODE | CREATE_NEW_CONSOLE, 0, 0, &si, &pi))) {
		bool processEnded = false;
		DWORD storedBytes, readBytes;

		startupFailed = false;
		::WaitForInputIdle(pi.hProcess, INFINITE);
		while(true) {
			rescueUser();
			if(interrupted_) {	// ユーザが止めた?
				::TerminateProcess(pi.hProcess, 0);
				break;
			}
			if(::WaitForSingleObject(pi.hProcess, 0) == WAIT_OBJECT_0)	// プロセスの生死確認
				processEnded = true;
			if(toBoolean(::PeekNamedPipe(stdoutReadPipe, 0, 0, 0, &storedBytes, 0)) && storedBytes > 0) {
				switchControls(EXECUTING);
				char* const buffer = new char[storedBytes];
				if(toBoolean(::ReadFile(stdoutReadPipe, buffer, storedBytes, &readBytes, 0)))
					appendOutput(buffer, buffer + readBytes);
				delete[] buffer;
			} else if(storedBytes == 0) {	// 出力が空であれば入力の機会を与える
				if(processEnded)
					break;
				if(!inputTextbox_.isEnabled()) {
					switchControls(INPUT_IDLE);
					inputTextbox_.setFocus();
					inputTextbox_.setSel(0, -1);
				}
				while(!inputQueue_.empty()) {	// 入力キューの内容を書き出す
					DWORD writtenBytes;
					::WriteFile(stdinWritePipe, inputQueue_.front().data(),
						static_cast<DWORD>(inputQueue_.front().length()), &writtenBytes, 0);
					::WriteFile(stdinWritePipe, "\n", 1, &writtenBytes, 0);
					inputQueue_.pop();
				}
				::Sleep(0);
			}
		}
	}

	::CloseHandle(stdoutReadPipe);
	::CloseHandle(stdoutWritePipe);
	::CloseHandle(stdinReadPipe);
	::CloseHandle(stdinWritePipe);
	::CloseHandle(pi.hProcess);
	::CloseHandle(pi.hThread);
	return !startupFailed;
}

/// GUI のフリーズを防ぐ
inline bool ExecuteCommandDlg::rescueUser() {
	MSG message;
	if(toBoolean(::PeekMessage(&message, 0, 0, 0, PM_REMOVE))) {
		if(message.message == WM_QUIT)
			return false;
		else if(!isDialogMessage(message)) {
			::TranslateMessage(&message);
			::DispatchMessage(&message);
		}
	}
	return true;
}

/// いくつかのコントロールの有効/無効を切り替える
void ExecuteCommandDlg::switchControls(ControlState state) {
	commandCombobox_.enable(state == WAIT_FOR_NEW_COMMAND);
	inputTextbox_.enable(state == INPUT_IDLE);
	inputTextbox_.setReadOnly(state != INPUT_IDLE);
	::EnableWindow(getItem(IDC_BTN_SENDEOS), state == INPUT_IDLE);
	::EnableWindow(getItem(IDOK), state != EXECUTING);
	::EnableWindow(getItem(IDC_CHK_GETCONSOLE), state == WAIT_FOR_NEW_COMMAND);
	::EnableWindow(getItem(IDC_CHK_USEUNICODEFORINPUT), state == WAIT_FOR_NEW_COMMAND);
}

/// @see Dialog::onCancel
void ExecuteCommandDlg::onCancel() {
	if(executing_)	// [中止] ボタン
		interrupted_ = true;
	else
		Dialog::onCancel();
}

/// @see Dialog::onClose
void ExecuteCommandDlg::onClose() {
	wchar_t keyName[35];

	interrupted_ = true;

	// 履歴と設定を保存
	for(int i = 0; i < commandCombobox_.getCount(); ++i) {
		wchar_t* const buffer = new wchar_t[commandCombobox_.getLBTextLen(i) + 1];
		commandCombobox_.getLBText(i, buffer);
		swprintf(keyName, L"CommandExecutionDialog.command(%u)", i);
		app_.writeStringProfile(L"Tool", keyName, buffer);
		delete[] buffer;
	}
	app_.writeIntegerProfile(L"Tool", L"CommandExecutionDialog.consoleProgram",
		(isButtonChecked(IDC_CHK_GETCONSOLE) == BST_CHECKED) ? 1 : 0);
	app_.writeIntegerProfile(L"Tool", L"CommandExecutionDialog.unicodeInput",
		(isButtonChecked(IDC_CHK_USEUNICODEFORINPUT) == BST_CHECKED) ? 1 : 0);

	Dialog::onClose();
}

/// @see Dialog::onCommand
bool ExecuteCommandDlg::onCommand(WORD id, WORD notifyCode, HWND control) {
	switch(id) {
	case IDC_COMBO_COMMAND:	// [コマンド]
		if(notifyCode == CBN_EDITCHANGE)
			::EnableWindow(getItem(IDOK), commandCombobox_.getTextLength() != 0);
		break;
	case IDC_BTN_CLEAR:	// [クリア]
		outputTextbox_.setSel(0, -1);
		outputTextbox_.replaceSel(L"");
		break;
	case IDC_BTN_SENDEOS:	// [^Z]
		inputTextbox_.setSel(-1, -1);
		inputTextbox_.replaceSel(L"\x001A");
		break;
	case IDC_CHK_GETCONSOLE:	// [コンソールプログラム]
		::EnableWindow(getItem(IDC_CHK_USEUNICODEFORINPUT), isButtonChecked(IDC_CHK_GETCONSOLE) == BST_CHECKED);
		break;
	}

	return Dialog::onCommand(id, notifyCode, control);
}

/// @see Dialog::onInitDialog
bool ExecuteCommandDlg::onInitDialog(HWND focusWindow, LPARAM initParam) {
	Dialog::onInitDialog(focusWindow, initParam);

	if(ioFont_) {
		commandCombobox_.setFont(ioFont_);
		inputTextbox_.setFont(ioFont_);
		outputTextbox_.setFont(ioFont_);
	}

	// 履歴と設定を読み込む
	wchar_t keyName[35];
	for(uint i = 0; i < MAX_HISTORY_LENGTH; ++i) {
		swprintf(keyName, L"CommandExecutionDialog.command(%u)", i);
		const wstring command = app_.readStringProfile(L"Tool", keyName);
		if(command.empty())
			commandCombobox_.addString(command.c_str());
		else
			break;
	}
	check2StateButton(IDC_CHK_GETCONSOLE,
		app_.readIntegerProfile(L"Tool", L"CommandExecutionDialog.consoleProgram", 0) != 0);
	check2StateButton(IDC_CHK_USEUNICODEFORINPUT,
		app_.readIntegerProfile(L"Tool", L"CommandExecutionDialog.unicodeInput", 0) != 0);
	onCommand(IDC_COMBO_COMMAND, CBN_EDITCHANGE, 0);
	onCommand(IDC_CHK_GETCONSOLE, 0, 0);
	return true;
}

/// @see Dialog::onOK
void ExecuteCommandDlg::onOK() {
	if(!executing_) {	// [実行]
		const int len = commandCombobox_.getTextLength();
		wchar_t* rawCmdLine = new wchar_t[len + 1];
		const basic_string<WCHAR> filePath = (app_.getBufferList().getActive().getFilePathName() != 0) ?
			basic_string<WCHAR>(L"\"") + app_.getBufferList().getActive().getFilePathName() + L"\"" : L"";

		commandCombobox_.getText(rawCmdLine, len + 1);

		// "$F" をファイルパスに変換する
		wstring cmdLine = rawCmdLine;
		wstring::size_type last = 0;
		while(true) {
			last = cmdLine.find(L"$F", last);
			if(last == wstring::npos)
				break;
			cmdLine.replace(last, 2, filePath);
			last += filePath.length() + 2;
		}
		delete[] rawCmdLine;

		// [コンソールを取り込む] 場合はウィンドウを用意する
		if(isButtonChecked(IDC_CHK_GETCONSOLE) == BST_CHECKED) {
			HWND splitStatic = getItem(IDC_STATIC_PROMPT);
			RECT rect;

			switchControls(EXECUTING);
			getRect(rect);
			rect.bottom = rect.top + 460;
			move(rect, true);
			::MoveWindow(splitStatic, 10, 122, 492, 2, false);
			inputTextbox_.move(10, 130, 466, 18, false);
			outputTextbox_.move(10, 150, 492, 270, false);
			::MoveWindow(getItem(IDC_BTN_SENDEOS), 480, 130, 22, 18, false);
			::ShowWindow(splitStatic, SW_SHOW);
			inputTextbox_.show(SW_SHOW);
			outputTextbox_.show(SW_SHOW);
			::ShowWindow(getItem(IDC_BTN_SENDEOS), SW_SHOW);

			executing_ = true;
			execute(cmdLine, true);
			executing_ = interrupted_ = false;

			switchControls(WAIT_FOR_NEW_COMMAND);
			commandCombobox_.setFocus();
			commandCombobox_.setEditSel(0, -1);
		} else
			execute(cmdLine, false);
	} else if(const int inputLength = inputTextbox_.getTextLength()) {
		wchar_t* const input = new wchar_t[inputLength + 1];

		inputTextbox_.getText(input, inputLength + 1);
		if(isButtonChecked(IDC_CHK_USEUNICODEFORINPUT) == BST_CHECKED)	// [標準入力に Unicode を使用する]
			appendInput(reinterpret_cast<char*>(input), reinterpret_cast<char*>(input + inputLength));
		else {
			const int cb = ::WideCharToMultiByte(CP_ACP, 0, input, inputLength, 0, 0, 0, 0);
			char* const	buffer = new char[cb];
			::WideCharToMultiByte(CP_ACP, 0, input, inputLength, buffer, cb, 0, 0);
			appendInput(buffer, buffer + cb);
			delete[] buffer;
		}
		delete[] input;
	}

//	Dialog::onOK();
}
