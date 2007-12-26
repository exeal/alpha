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
 * @param encoding the encoding initially selected
 * @param forReading set true to enumelate encodings for read files
 */
EncodingsDialog::EncodingsDialog(const string& encoding, bool forReading) throw() : result_(encoding), forReading_(forReading) {
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
	vector<string> names;
	Encoder::availableNames(back_inserter(names));
	const Encoder* asciiEncoder = Encoder::forMIB(fundamental::US_ASCII);
	assert(asciiEncoder != 0);

	for(vector<string>::const_iterator name(names.begin()), e(names.end()); name != e; ++name) {
		const wstring s(asciiEncoder->toUnicode(*name));
		if(!s.empty())
			encodingList_.setItemData(encodingList_.addString(s.c_str()),
				matchEncodingNames(s.begin(), s.end(), result_.begin(), result_.end()) ? 1 : 0);
	}
	if(forReading_) {
		names.clear();
		EncodingDetector::availableNames(back_inserter(names));
		for(vector<string>::const_iterator name(names.begin()), e(names.end()); name != e; ++name) {
			const wstring s(asciiEncoder->toUnicode(*name));
			if(!s.empty())
				encodingList_.setItemData(encodingList_.addString(s.c_str()),
					matchEncodingNames(s.begin(), s.end(), result_.begin(), result_.end()) ? 1 : 0);
		}
	}

	// highlight initially selected item
	for(int i = 0, c = encodingList_.getCount(); i < c; ++i) {
		if(encodingList_.getItemData(i) == 1) {
			encodingList_.setCurSel(i);
			break;
		}
	}
}

/// @see Dialog#onOK
void EncodingsDialog::onOK(bool&) {
	const int sel = encodingList_.getCurSel();
	if(const int len = encodingList_.getTextLen(sel)) {
		manah::AutoBuffer<wchar_t> name(new wchar_t[len + 1]);
		encodingList_.getText(sel, name.get());
		result_ = Encoder::forMIB(fundamental::US_ASCII)->fromUnicode(name.get());
	}
}
