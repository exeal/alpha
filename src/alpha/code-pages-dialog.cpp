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
using alpha::ui::CodePagesDialog;
using namespace ascension::encodings;
using namespace std;


/**
 * Constructor.
 * @param codePage the encoding initially selected
 * @param forReading set true to enumelate encodings for read files
 */
CodePagesDialog::CodePagesDialog(CodePage codePage, bool forReading) throw() : codePage_(codePage), forReading_(forReading) {
}

/// @see Dialog#onCommand
bool CodePagesDialog::onCommand(WORD id, WORD notifyCode, HWND control) {
	if(id == IDC_LIST_CODEPAGES && notifyCode == LBN_DBLCLK) {
		postMessage(WM_COMMAND, IDOK);
		return true;
	}
	return Dialog::onCommand(id, notifyCode, control);
}

/// @see Dialog#onInitDialog
void CodePagesDialog::onInitDialog(HWND focusWindow, bool&) {
	const EncoderFactory& encoders = EncoderFactory::getInstance();
	set<CodePage> codePages;

	encoders.enumCodePages(codePages);
	for(set<CodePage>::const_iterator cp(codePages.begin()), e(codePages.end()); cp != e; ++cp) {
		if(!forReading_ && (encoders.isCodePageForAutoDetection(*cp) || encoders.isCodePageForReadOnly(*cp)))
			continue;
		else {
			const DWORD id = (*cp < 0x10000) ? (*cp + MSGID_ENCODING_START) : (*cp - 60000 + MSGID_EXTENDED_ENCODING_START);
			const wstring name(Alpha::getInstance().loadMessage(id));
			if(!name.empty())
				codepageList_.setItemData(codepageList_.addString(((codePage_ == *cp) ? name + L" *" : name).c_str()), *cp);
		}
	}
	const int c = codepageList_.getCount();
	for(int i = 0; i < c; ++i) {
		if(codePage_ == codepageList_.getItemData(i)) {
			codepageList_.setCurSel(i);
			break;
		}
	}
}

/// @see Dialog#onOK
void CodePagesDialog::onOK(bool&) {
	codePage_ = static_cast<CodePage>(codepageList_.getItemData(codepageList_.getCurSel()));
}
