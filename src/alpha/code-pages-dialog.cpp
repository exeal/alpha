/**
 * @file code-pages-dialog.cpp
 * @author exeal
 * @date 2003-2007
 */

#include "stdafx.h"
#include "code-pages-dialog.hpp"
#include "application.hpp"
#include "resource/messages.h"
#include "../manah/win32/ui/standard-controls.hpp"
using alpha::ui::EncodingsDialog;
using namespace ascension::encoding;
using namespace std;


/**
 * Constructor.
 * @param mib the MIBenum value of the encoding initially selected
 * @param forReading set true to enumelate encodings for read files
 */
EncodingsDialog::EncodingsDialog(MIBenum mib, bool forReading) throw() : mib_(mib), forReading_(forReading) {
}

/// @see Dialog#onCommand
bool EncodingsDialog::onCommand(WORD id, WORD notifyCode, HWND control) {
	if(id == IDC_LIST_CODEPAGES && notifyCode == LBN_DBLCLK) {
		postMessage(WM_COMMAND, IDOK);
		return true;
	}
	return Dialog::onCommand(id, notifyCode, control);
}

/// @see Dialog#onInitDialog
void EncodingsDialog::onInitDialog(HWND focusWindow, bool&) {
	vector<MIBenum> mibs;
	Encoder::getAvailableMIBs(back_inserter(mibs));
	for(vector<MIBenum>::const_iterator mib(mibs.begin()), e(mibs.end()); mib != e; ++mib) {
//		const DWORD id = (*cp < 0x10000) ? (*cp + MSGID_ENCODING_START) : (*cp - 60000 + MSGID_EXTENDED_ENCODING_START);
//		const wstring name(Alpha::getInstance().loadMessage(id));
		const wstring name(getEncodingDisplayName(*mib));
		if(!name.empty())
			encodingList_.setItemData(encodingList_.addString(((mib_ == *mib) ? name + L" *" : name).c_str()), *mib);
	}
	if(forReading_) {
		mibs.clear();
		EncodingDetector::getAvailableIDs(back_inserter(mibs));
		for(vector<MIBenum>::const_iterator mib(mibs.begin()), e(mibs.end()); mib != e; ++mib) {
			const wstring name(getEncodingDisplayName(*mib));
			if(!name.empty())
				encodingList_.setItemData(encodingList_.addString(name.c_str()), *mib);
		}
	}
	for(int i = 0, c = encodingList_.getCount(); i < c; ++i) {
		if(mib_ == encodingList_.getItemData(i)) {
			encodingList_.setCurSel(i);
			break;
		}
	}
}

/// @see Dialog#onOK
void EncodingsDialog::onOK(bool&) {
	mib_ = static_cast<MIBenum>(encodingList_.getItemData(encodingList_.getCurSel()));
}
