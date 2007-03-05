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


/// コンストラクタ
GotoLineDialog::GotoLineDialog(Alpha& app) : app_(app) {
}

/// @see Dialog#onInitDialog
bool GotoLineDialog::onInitDialog(HWND focusWindow, LPARAM initParam) {
	Dialog::onInitDialog(focusWindow, initParam);

	const Buffer& buffer = app_.getBufferList().getActive();
	const length_t lineOffset = app_.getBufferList().getActiveView().getVerticalRulerConfiguration().lineNumbers.startValue;
	const wstring s = app_.loadString(MSG_DIALOG__LINE_NUMBER_RANGE, MARGS
								% static_cast<ulong>(buffer.getStartPosition().line + lineOffset)
								% static_cast<ulong>(buffer.getEndPosition().line + lineOffset));

	setItemText(IDC_STATIC_1, s.c_str());
	lineNumberSpin_.setRange(
		static_cast<int>(buffer.getStartPosition().line + lineOffset),
		static_cast<int>(buffer.getEndPosition().line + lineOffset));
	lineNumberSpin_.setPosition(static_cast<int>(app_.getBufferList().getActiveView().getCaret().getLineNumber() + lineOffset));
	lineNumberSpin_.invalidateRect(0);

	checkRadioButton(IDC_RADIO_LOGICALLINE,
		IDC_RADIO_PHYSICALLINE, /*lineUnit_ ?*/ IDC_RADIO_LOGICALLINE /*: IDC_RADIO_PHYSICALLINE*/);
	checkButton(IDC_CHK_SAVESELECTION,
		app_.readIntegerProfile(L"Search", L"GotoLineDialog.extendSelection", 0) == 1 ? BST_CHECKED : BST_UNCHECKED);

	return true;
}

/// @see Dialog#onOK()
void GotoLineDialog::onOK() {
	// 一時マクロ定義中は実行できない
	if(app_.getCommandManager().getTemporaryMacro().getState() == command::TemporaryMacro::DEFINING) {
		app_.messageBox(MSG_ERROR__PROHIBITED_FOR_MACRO_DEFINING, MB_ICONEXCLAMATION);
		return;
	}

	EditorView& activeView = app_.getBufferList().getActiveView();
	length_t line = lineNumberSpin_.getPosition();

	// 物理行から論理行に変換
	line -= activeView.getVerticalRulerConfiguration().lineNumbers.startValue;
	if(isButtonChecked(IDC_RADIO_PHYSICALLINE) == BST_CHECKED)
		line = activeView.getTextRenderer().mapVisualLineToLogicalLine(line, 0);

	// 移動する
	if(isButtonChecked(IDC_CHK_SAVESELECTION) == BST_CHECKED) {
		activeView.getCaret().extendSelection(text::Position(line, 0));
		app_.writeIntegerProfile(L"Search", L"GotoLineDialog.extendSelection", 1);
	} else {
		activeView.getCaret().moveTo(text::Position(line, 0));
		app_.writeIntegerProfile(L"Search", L"GotoLineDialog.extendSelection", 0);
	}
	Dialog::onOK();
}
