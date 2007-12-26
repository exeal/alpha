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
NewFileFormatDialog::NewFileFormatDialog(const string& encoding, Newline newline) throw() : encoding_(encoding), newline_(newline) {
}

/// @see Dialog#onCommand
bool NewFileFormatDialog::onCommand(WORD id, WORD notifyCode, HWND control) {
	if(id != IDC_COMBO_ENCODING || notifyCode != CBN_SELCHANGE)
		return Dialog::onCommand(id, notifyCode, control);

	MIBenum mib = MIB_UNKNOWN;
	if(const Encoder* const e = Encoder::forName(selectedEncoding()))
		mib = e->mibEnum();

	if(mib == extended::UTF_5 || mib == extended::UTF_7 || mib == fundamental::UTF_8
			|| mib == fundamental::UTF_16LE || mib == fundamental::UTF_16BE
			|| mib == extended::UTF_32LE || mib == extended::UTF_32BE) {
		if(newlineCombobox_.getCount() != 6) {
			const int org = (newlineCombobox_.getCount() != 0) ? newlineCombobox_.getCurSel() : 0;
			newlineCombobox_.resetContent();
			newlineCombobox_.setItemData(newlineCombobox_.addString(IDS_BREAK_CRLF), NLF_CR_LF);
			newlineCombobox_.setItemData(newlineCombobox_.addString(IDS_BREAK_LF), NLF_LINE_FEED);
			newlineCombobox_.setItemData(newlineCombobox_.addString(IDS_BREAK_CR), NLF_CARRIAGE_RETURN);
			newlineCombobox_.setItemData(newlineCombobox_.addString(IDS_BREAK_NEL), NLF_NEXT_LINE);
			newlineCombobox_.setItemData(newlineCombobox_.addString(IDS_BREAK_LS), NLF_LINE_SEPARATOR);
			newlineCombobox_.setItemData(newlineCombobox_.addString(IDS_BREAK_PS), NLF_PARAGRAPH_SEPARATOR);
			newlineCombobox_.setCurSel(org);
		}
	} else {
		if(newlineCombobox_.getCount() != 3) {
			const int org = (newlineCombobox_.getCount() != 0) ? newlineCombobox_.getCurSel() : 0;
			newlineCombobox_.resetContent();
			newlineCombobox_.setItemData(newlineCombobox_.addString(IDS_BREAK_CRLF), NLF_CR_LF);
			newlineCombobox_.setItemData(newlineCombobox_.addString(IDS_BREAK_LF), NLF_LINE_FEED);
			newlineCombobox_.setItemData(newlineCombobox_.addString(IDS_BREAK_CR), NLF_CARRIAGE_RETURN);
			newlineCombobox_.setCurSel((org < newlineCombobox_.getCount()) ? org : 0);
		}
	}
	return true;
}

/// @see Dialog#onInitDialog
void NewFileFormatDialog::onInitDialog(HWND focusWindow, bool&) {
	// [Encoding]
	vector<string> names;
	Encoder::availableNames(back_inserter(names));
	const Encoder* asciiEncoder = Encoder::forMIB(fundamental::US_ASCII);
	for(vector<string>::const_iterator name(names.begin()), e(names.end()); name != e; ++name) {
		const wstring s(asciiEncoder->toUnicode(*name));
		if(!s.empty()) {
			const int i = encodingCombobox_.addString(s.c_str());
			if(i >= 0 && matchEncodingNames(s.begin(), s.end(), encoding_.begin(), encoding_.end()))
				encodingCombobox_.setCurSel(i);
		}
	}

	// [Newline]
	onCommand(IDC_COMBO_ENCODING, CBN_SELCHANGE, 0);
	for(int i = 0; i < 6; ++i) {
		if(newline_ == static_cast<Newline>(newlineCombobox_.getItemData(i))) {
			newlineCombobox_.setCurSel(i);
			break;
		}
	}

//	// [ï∂èëÉ^ÉCÉv]
//	for(list<wstring>::const_iterator it = documentTypes_.begin(); it != documentTypes_.end(); ++it)
//		documentTypeCombobox_.addString(it->c_str());
//	documentTypeCombobox_.setCurSel(static_cast<int>(documentType_));
}

/// @see Dialog#onOK
void NewFileFormatDialog::onOK(bool&) {
	encoding_ = selectedEncoding();
	newline_ = static_cast<Newline>(newlineCombobox_.getItemData(newlineCombobox_.getCurSel()));
//	documentType_ = documentTypeCombobox_.getCurSel();
}

inline string NewFileFormatDialog::selectedEncoding() const throw() {
	return Encoder::forMIB(fundamental::US_ASCII)->fromUnicode(encodingCombobox_.getText());
}
