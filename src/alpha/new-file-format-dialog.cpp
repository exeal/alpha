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
using namespace ascension::encodings;
using namespace ascension::text;
using namespace std;


/**
 * Constructor.
 * @param encoding the encoding initially selected
 * @param newline the newline initially selected
 */
NewFileFormatDialog::NewFileFormatDialog(CodePage encoding, Newline newline) throw() : encoding_(encoding), newline_(newline) {
}

/// @see Dialog#onCommand
bool NewFileFormatDialog::onCommand(WORD id, WORD notifyCode, HWND control) {
	if(id != IDC_COMBO_ENCODING || notifyCode != CBN_SELCHANGE)
		return Dialog::onCommand(id, notifyCode, control);

	const CodePage cp = static_cast<CodePage>(codePageCombobox_.getItemData(codePageCombobox_.getCurSel()));

	if(cp == CPEX_UNICODE_UTF5 || cp == CP_UTF7 || cp == CP_UTF8
			|| cp == CPEX_UNICODE_UTF16LE || cp == CPEX_UNICODE_UTF16BE
			|| cp == CPEX_UNICODE_UTF32LE || cp == CPEX_UNICODE_UTF32BE) {
		if(newlineCombobox_.getCount() != 6) {
			const int org = (newlineCombobox_.getCount() != 0) ? newlineCombobox_.getCurSel() : 0;
			newlineCombobox_.resetContent();
			newlineCombobox_.setItemData(newlineCombobox_.addString((newline_ == NLF_CRLF) ? IDS_BREAK_CRLF L" *" : IDS_BREAK_CRLF), NLF_CRLF);
			newlineCombobox_.setItemData(newlineCombobox_.addString((newline_ == NLF_LF) ? IDS_BREAK_LF L" *" : IDS_BREAK_LF), NLF_LF);
			newlineCombobox_.setItemData(newlineCombobox_.addString((newline_ == NLF_CR) ? IDS_BREAK_CR L" *" : IDS_BREAK_CR), NLF_CR);
			newlineCombobox_.setItemData(newlineCombobox_.addString((newline_ == NLF_NEL) ? IDS_BREAK_NEL L" *" : IDS_BREAK_NEL), NLF_NEL);
			newlineCombobox_.setItemData(newlineCombobox_.addString((newline_ == NLF_LS) ? IDS_BREAK_LS L" *" : IDS_BREAK_LS), NLF_LS);
			newlineCombobox_.setItemData(newlineCombobox_.addString((newline_ == NLF_PS) ? IDS_BREAK_PS L" *" : IDS_BREAK_PS), NLF_PS);
			newlineCombobox_.setCurSel(org);
		}
	} else {
		if(newlineCombobox_.getCount() != 3) {
			const int org = (newlineCombobox_.getCount() != 0) ? newlineCombobox_.getCurSel() : 0;
			newlineCombobox_.resetContent();
			newlineCombobox_.setItemData(newlineCombobox_.addString(
				(newline_ == NLF_CRLF) ? IDS_BREAK_CRLF L" *" : IDS_BREAK_CRLF), NLF_CRLF);
			newlineCombobox_.setItemData(newlineCombobox_.addString(
				(newline_ == NLF_LF) ? IDS_BREAK_LF L" *" : IDS_BREAK_LF), NLF_LF);
			newlineCombobox_.setItemData(newlineCombobox_.addString(
				(newline_ == NLF_CR) ? IDS_BREAK_CR L" *" : IDS_BREAK_CR), NLF_CR);
			newlineCombobox_.setCurSel((org < newlineCombobox_.getCount()) ? org : 0);
		}
	}
	return true;
}

/// @see Dialog#onInitDialog
void NewFileFormatDialog::onInitDialog(HWND focusWindow, bool&) {
	set<CodePage> codePages;

	// [コードページ]
	EncoderFactory::getInstance().enumCodePages(codePages);
	for(set<CodePage>::const_iterator cp(codePages.begin()), e(codePages.end()); cp != e; ++cp) {
		if(EncoderFactory::getInstance().isCodePageForAutoDetection(*cp))
			continue;
		const DWORD id = (*cp < 0x10000) ? (*cp + MSGID_ENCODING_START) : (*cp - 60000 + MSGID_EXTENDED_ENCODING_START);
		const wstring name(Alpha::getInstance().loadMessage(id));
		if(!name.empty()) {
			const int i = codePageCombobox_.addString((*cp == encoding_) ? (name + L" *").c_str() : name.c_str());
			codePageCombobox_.setItemData(i, *cp);
			if(*cp == encoding_)
				codePageCombobox_.setCurSel(i);
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
	encoding_ = static_cast<CodePage>(codePageCombobox_.getItemData(codePageCombobox_.getCurSel()));
	newline_ = static_cast<Newline>(newlineCombobox_.getItemData(newlineCombobox_.getCurSel()));
//	documentType_ = documentTypeCombobox_.getCurSel();
}
