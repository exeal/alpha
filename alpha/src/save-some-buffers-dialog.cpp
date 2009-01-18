/**
 * @file save-some-buffers-dialog.cpp
 * @author exeal
 * @date 2003-2009
 */

#include "application.hpp"
#include "save-some-buffers-dialog.hpp"
#include "../resource/messages.h"
using alpha::Alpha;
using alpha::ui::SaveSomeBuffersDialog;
using namespace std;


/// @see Dialog#onCommand
bool SaveSomeBuffersDialog::onCommand(WORD id, WORD notifyCode, HWND control) {
	if(id == IDC_LINK_SELECTALL)
		bufferListbox_.selItemRange(0, bufferListbox_.getCount() - 1);
	else if(id == IDC_LINK_CLEARALL)
		bufferListbox_.selItemRange(0, bufferListbox_.getCount() - 1, false);
	return Dialog::onCommand(id, notifyCode, control);
}

/// @see Dialog#onInitDialog
void SaveSomeBuffersDialog::onInitDialog(HWND, bool&) {
	HICON icon = Alpha::loadStandardIcon(MAKEINTRESOURCEW(32515));	// IDI_WARNING
	sendItemMessage(IDC_STATIC_1, STM_SETICON, reinterpret_cast<WPARAM>(icon), 0);
	::DestroyIcon(icon);
	selectAllLink_.create(get(), ::GetModuleHandle(0), IDC_LINK_SELECTALL);
	selectAllLink_.move(106, 200, 0, 0);
	selectAllLink_.setText(Alpha::instance().loadMessage(MSG_DIALOG__SELECT_ALL).c_str());
	clearAllLink_.create(get(), ::GetModuleHandle(0), IDC_LINK_CLEARALL);
	clearAllLink_.move(186, 200, 0, 0);
	clearAllLink_.setText(Alpha::instance().loadMessage(MSG_DIALOG__UNSELECT_ALL).c_str());

	for(vector<DirtyFile>::const_iterator it = files_.begin(); it != files_.end(); ++it)
		bufferListbox_.addString(it->fileName.c_str());
	bufferListbox_.selItemRange(0, bufferListbox_.getCount() - 1);
	::MessageBeep(MB_ICONEXCLAMATION);
}

/// @see Dialog#onOK
void SaveSomeBuffersDialog::onOK(bool&) {
	const int c = bufferListbox_.getSelCount();
	int* sels = new int[c];

	bufferListbox_.getSelItems(c, sels);
	for(vector<DirtyFile>::iterator it = files_.begin(); it != files_.end(); ++it)
		it->save = false;
	for(int i = 0; i < c; ++i)
		files_[i].save = true;
	delete[] sels;
}
