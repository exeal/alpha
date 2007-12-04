/**
 * @file new-file-format-dialog.cpp
 * @author exeal
 * @date 2003-2007
 */

#include "stdafx.h"
#include "new-file-format-dialog.hpp"
#include "application.hpp"
#include "resource/messages.h"
#include "../manah/win32/ui/standard-controls.hpp"
using alpha::ui::NewFileFormatDialog;
using manah::win32::ui::ComboBox;
using namespace ascension::encoding;
using namespace ascension::kernel;
using namespace std;


/**
 * Constructor.
 * @param encoding the encoding initially selected
 * @param newline the newline initially selected
 */
NewFileFormatDialog::NewFileFormatDialog(MIBenum encoding, Newline newline) throw() : encoding_(encoding), newline_(newline) {
}

/// @see Dialog#onCommand
bool NewFileFormatDialog::onCommand(WORD id, WORD notifyCode, HWND control) {
	if(id != IDC_COMBO_ENCODING || notifyCode != CBN_SELCHANGE)
		return Dialog::onCommand(id, notifyCode, control);

	const MIBenum mib = static_cast<MIBenum>(encodingCombobox_.getItemData(encodingCombobox_.getCurSel()));

	if(mib == extended::UTF_5 || mib == extended::UTF_7 || mib == fundamental::UTF_8
			|| mib == fundamental::UTF_16LE || mib == fundamental::UTF_16BE
			|| mib == extended::UTF_32LE || mib == extended::UTF_32BE) {
		if(newlineCombobox_.getCount() != 6) {
			const int org = (newlineCombobox_.getCount() != 0) ? newlineCombobox_.getCurSel() : 0;
			newlineCombobox_.resetContent();
			newlineCombobox_.setItemData(newlineCombobox_.addString((newline_ == NLF_CR_LF) ? IDS_BREAK_CRLF L" *" : IDS_BREAK_CRLF), NLF_CRLF);
			newlineCombobox_.setItemData(newlineCombobox_.addString((newline_ == NLF_LINE_FEED) ? IDS_BREAK_LF L" *" : IDS_BREAK_LF), NLF_LF);
			newlineCombobox_.setItemData(newlineCombobox_.addString((newline_ == NLF_CARRIAGE_RETURN) ? IDS_BREAK_CR L" *" : IDS_BREAK_CR), NLF_CR);
			newlineCombobox_.setItemData(newlineCombobox_.addString((newline_ == NLF_NEXT_LINE) ? IDS_BREAK_NEL L" *" : IDS_BREAK_NEL), NLF_NEL);
			newlineCombobox_.setItemData(newlineCombobox_.addString((newline_ == NLF_LINE_SEPARATOR) ? IDS_BREAK_LS L" *" : IDS_BREAK_LS), NLF_LS);
			newlineCombobox_.setItemData(newlineCombobox_.addString((newline_ == NLF_PARAGRAPH_SEPARATOR) ? IDS_BREAK_PS L" *" : IDS_BREAK_PS), NLF_PS);
			newlineCombobox_.setCurSel(org);
		}
	} else {
		if(newlineCombobox_.getCount() != 3) {
			const int org = (newlineCombobox_.getCount() != 0) ? newlineCombobox_.getCurSel() : 0;
			newlineCombobox_.resetContent();
			newlineCombobox_.setItemData(newlineCombobox_.addString(
				(newline_ == NLF_CR_LF) ? IDS_BREAK_CRLF L" *" : IDS_BREAK_CRLF), NLF_CR_LF);
			newlineCombobox_.setItemData(newlineCombobox_.addString(
				(newline_ == NLF_LINE_FEED) ? IDS_BREAK_LF L" *" : IDS_BREAK_LF), NLF_LINE_FEED);
			newlineCombobox_.setItemData(newlineCombobox_.addString(
				(newline_ == NLF_CARRIAGE_RETURN) ? IDS_BREAK_CR L" *" : IDS_BREAK_CR), NLF_CARRIAGE_RETURN);
			newlineCombobox_.setCurSel((org < newlineCombobox_.getCount()) ? org : 0);
		}
	}
	return true;
}

/// @see Dialog#onInitDialog
void NewFileFormatDialog::onInitDialog(HWND focusWindow, bool&) {
	// [コードページ]
	vector<MIBenum> mibs;
	Encoder::availableMIBs(back_inserter(mibs));
	for(vector<MIBenum>::const_iterator mib(mibs.begin()), e(mibs.end()); mib != e; ++mib) {
//		const DWORD id = (*cp < 0x10000) ? (*cp + MSGID_ENCODING_START) : (*cp - 60000 + MSGID_EXTENDED_ENCODING_START);
//		const wstring name(Alpha::getInstance().loadMessage(id));
		const wstring name(getEncodingDisplayName(*mib));
		if(!name.empty()) {
			const int i = encodingCombobox_.addString((*mib == encoding_) ? (name + L" *").c_str() : name.c_str());
			encodingCombobox_.setItemData(i, *mib);
			if(*mib == encoding_)
				encodingCombobox_.setCurSel(i);
		}
	}

	// [改行コード]
	onCommand(IDC_COMBO_ENCODING, CBN_SELCHANGE, 0);
	for(int i = 0; i < 6; ++i) {
		if(newline_ == static_cast<Newline>(newlineCombobox_.getItemData(i))) {
			newlineCombobox_.setCurSel(i);
			break;
		}
	}

//	// [文書タイプ]
//	for(list<wstring>::const_iterator it = documentTypes_.begin(); it != documentTypes_.end(); ++it)
//		documentTypeCombobox_.addString(it->c_str());
//	documentTypeCombobox_.setCurSel(static_cast<int>(documentType_));
}

/// @see Dialog#onOK
void NewFileFormatDialog::onOK(bool&) {
	encoding_ = static_cast<MIBenum>(encodingCombobox_.getItemData(encodingCombobox_.getCurSel()));
	newline_ = static_cast<Newline>(newlineCombobox_.getItemData(newlineCombobox_.getCurSel()));
//	documentType_ = documentTypeCombobox_.getCurSel();
}
