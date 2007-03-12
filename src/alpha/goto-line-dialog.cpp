/**
 * @file goto-line-dialog.cpp
 * @author exeal
 * @date 2003-2006
 */

#include "stdafx.h"
#include "goto-line-dialog.hpp"
#include "command.hpp"

using alpha::ui::GotoLineDialog;
using alpha::Alpha;
using namespace ascension;
using namespace std;


/// @see Dialog#onInitDialog
void GotoLineDialog::onInitDialog(HWND, bool&) {
	Alpha& app = Alpha::getInstance();
	const Buffer& buffer = app.getBufferList().getActive();
	const length_t lineOffset = app.getBufferList().getActiveView().getVerticalRulerConfiguration().lineNumbers.startValue;
	const wstring s = app.loadString(MSG_DIALOG__LINE_NUMBER_RANGE, MARGS
						% static_cast<ulong>(buffer.getStartPosition().line + lineOffset)
						% static_cast<ulong>(buffer.getEndPosition().line + lineOffset));

	setItemText(IDC_STATIC_1, s.c_str());
	lineNumberSpin_.setRange(
		static_cast<int>(buffer.getStartPosition().line + lineOffset),
		static_cast<int>(buffer.getEndPosition().line + lineOffset));
	lineNumberSpin_.setPosition(static_cast<int>(app.getBufferList().getActiveView().getCaret().getLineNumber() + lineOffset));
	lineNumberSpin_.invalidateRect(0);

	checkRadioButton(IDC_RADIO_LOGICALLINE,
		IDC_RADIO_PHYSICALLINE, /*lineUnit_ ?*/ IDC_RADIO_LOGICALLINE /*: IDC_RADIO_PHYSICALLINE*/);
	checkButton(IDC_CHK_SAVESELECTION,
		app.readIntegerProfile(L"Search", L"GotoLineDialog.extendSelection", 0) == 1 ? BST_CHECKED : BST_UNCHECKED);
}

/// @see Dialog#onOK()
void GotoLineDialog::onOK(bool& continueDialog) {
	// 一時マクロ定義中は実行できない
	Alpha& app = Alpha::getInstance();
	if(app.getCommandManager().getTemporaryMacro().getState() == command::TemporaryMacro::DEFINING) {
		app.messageBox(MSG_ERROR__PROHIBITED_FOR_MACRO_DEFINING, MB_ICONEXCLAMATION);
		continueDialog = true;
		return;
	}

	EditorView& activeView = app.getBufferList().getActiveView();
	length_t line = lineNumberSpin_.getPosition();

	// 物理行から論理行に変換
	line -= activeView.getVerticalRulerConfiguration().lineNumbers.startValue;
	if(isButtonChecked(IDC_RADIO_PHYSICALLINE) == BST_CHECKED)
		line = activeView.getTextRenderer().mapVisualLineToLogicalLine(line, 0);

	// 移動する
	if(isButtonChecked(IDC_CHK_SAVESELECTION) == BST_CHECKED) {
		activeView.getCaret().extendSelection(text::Position(line, 0));
		app.writeIntegerProfile(L"Search", L"GotoLineDialog.extendSelection", 1);
	} else {
		activeView.getCaret().moveTo(text::Position(line, 0));
		app.writeIntegerProfile(L"Search", L"GotoLineDialog.extendSelection", 0);
	}
}
