/**
 * @file goto-line-dialog.cpp
 * @author exeal
 * @date 2003-2006
 */

#include "stdafx.h"
#include "goto-line-dialog.hpp"
#include "command.hpp"
#include "resource/messages.h"

using alpha::ui::GotoLineDialog;
using alpha::Alpha;
using namespace ascension;
using namespace std;


/// @see Dialog#onInitDialog
void GotoLineDialog::onInitDialog(HWND, bool&) {
	Alpha& app = Alpha::instance();
	const Buffer& buffer = app.bufferList().active();
	const length_t lineOffset = app.getBufferList().activeView().verticalRulerConfiguration().lineNumbers.startValue;
	const wstring s = app.loadMessage(MSG_DIALOG__LINE_NUMBER_RANGE, MARGS
						% static_cast<ulong>(buffer.accessibleRegion().first.line + lineOffset)
						% static_cast<ulong>(buffer.accessibleRegion().second.line + lineOffset));

	setItemText(IDC_STATIC_1, s.c_str());
	lineNumberSpin_.setRange(
		static_cast<int>(buffer.accessibleRegion().first.line + lineOffset),
		static_cast<int>(buffer.accessibleRegion().second.line + lineOffset));
	lineNumberSpin_.setPosition(static_cast<int>(app.bufferList().activeView().caret().lineNumber() + lineOffset));
	lineNumberSpin_.invalidateRect(0);

	checkButton(IDC_CHK_SAVESELECTION,
		app.readIntegerProfile(L"Search", L"GotoLineDialog.extendSelection", 0) == 1 ? BST_CHECKED : BST_UNCHECKED);
}

/// @see Dialog#onOK()
void GotoLineDialog::onOK(bool& continueDialog) {
	// 一時マクロ定義中は実行できない
	Alpha& app = Alpha::instance();
	if(app.commandManager().temporaryMacro().state() == command::TemporaryMacro::DEFINING) {
		app.messageBox(MSG_ERROR__PROHIBITED_FOR_MACRO_DEFINING, MB_ICONEXCLAMATION);
		continueDialog = true;
		return;
	}

	EditorView& activeView = app.bufferList().activeView();
	length_t line = lineNumberSpin_.getPosition();
	line -= activeView.verticalRulerConfiguration().lineNumbers.startValue;

	// 移動する
	if(isButtonChecked(IDC_CHK_SAVESELECTION) == BST_CHECKED) {
		activeView.caret().extendSelection(kernel::Position(line, 0));
		app.writeIntegerProfile(L"Search", L"GotoLineDialog.extendSelection", 1);
	} else {
		activeView.caret().moveTo(kernel::Position(line, 0));
		app.writeIntegerProfile(L"Search", L"GotoLineDialog.extendSelection", 0);
	}
}
