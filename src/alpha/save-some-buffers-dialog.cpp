/**
 * @file save-some-buffers-dialog.cpp
 * @author exeal
 * @date 2003-2006
 */

#include "stdafx.h"
#include "application.hpp"
#include "save-some-buffers-dialog.hpp"
using alpha::Alpha;
using alpha::ui::SaveSomeBuffersDlg;
using namespace std;


/// @see Dialog#onCommand
bool SaveSomeBuffersDlg::onCommand(WORD id, WORD notifyCode, HWND control) {
	if(id == IDC_LINK_SELECTALL)
		bufferListbox_.selItemRange(0, bufferListbox_.getCount() - 1);
	else if(id == IDC_LINK_CLEARALL)
		bufferListbox_.selItemRange(0, bufferListbox_.getCount() - 1, false);
	return Dialog::onCommand(id, notifyCode, control);
}

/// @see Dialog#onInitDialog
bool SaveSomeBuffersDlg::onInitDialog(HWND focusWindow, LPARAM initParam) {
	Dialog::onInitDialog(focusWindow, initParam);

	HICON icon = Alpha::loadStandardIcon(IDI_WARNING);
	sendDlgItemMessage(IDC_STATIC_1, STM_SETICON, reinterpret_cast<WPARAM>(icon), 0);
	::DestroyIcon(icon);
	selectAllLink_.create(*this, ::GetModuleHandle(0), IDC_LINK_SELECTALL);
	selectAllLink_.moveWindow(106, 200, 0, 0);
	selectAllLink_.setWindowText(Alpha::getInstance().loadString(MSG_DIALOG__SELECT_ALL).c_str());
	clearAllLink_.create(*this, ::GetModuleHandle(0), IDC_LINK_CLEARALL);
	clearAllLink_.moveWindow(186, 200, 0, 0);
	clearAllLink_.setWindowText(Alpha::getInstance().loadString(MSG_DIALOG__UNSELECT_ALL).c_str());

	for(vector<DirtyFile>::const_iterator it = files_.begin(); it != files_.end(); ++it)
		bufferListbox_.addString(it->fileName.c_str());
	bufferListbox_.selItemRange(0, bufferListbox_.getCount() - 1);
	::MessageBeep(MB_ICONEXCLAMATION);

	return true;
}

/// @see Dialog#onOK
void SaveSomeBuffersDlg::onOK() {
	const int c = bufferListbox_.getSelCount();
	int* sels = new int[c];

	bufferListbox_.getSelItems(c, sels);
	for(vector<DirtyFile>::iterator it = files_.begin(); it != files_.end(); ++it)
		it->save = false;
	for(int i = 0; i < c; ++i)
		files_[i].save = true;
	delete[] sels;

	Dialog::onOK();
}
