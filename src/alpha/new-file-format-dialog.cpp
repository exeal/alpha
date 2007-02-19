/**
 * @file new-file-format-dialog.cpp
 * @author exeal
 * @date 2003-2007
 */

#include "stdafx.h"
#include "new-file-format-dialog.hpp"
#include "application.hpp"
#include "../manah/win32/ui/standard-controls.hpp"
using alpha::ui::NewFileFormatDialog;
using manah::windows::ui::ComboBox;
using namespace ascension::encodings;
using namespace ascension::text;
using namespace std;


/**
 * コンストラクタ
 * @param encoding エンコーディングの初期値
 * @param lineBreak 改行の初期値
 */
NewFileFormatDialog::NewFileFormatDialog(CodePage encoding, LineBreak lineBreak) throw() : encoding_(encoding), lineBreak_(lineBreak) {
}

/// @see Dialog#onCommand
bool NewFileFormatDialog::onCommand(WORD id, WORD notifyCode, HWND control) {
	if(id != IDC_COMBO_CHARCODE || notifyCode != CBN_SELCHANGE)
		return Dialog::onCommand(id, notifyCode, control);

	const CodePage cp = static_cast<CodePage>(codePageCombobox_.getItemData(codePageCombobox_.getCurSel()));

	if(cp == CPEX_UNICODE_UTF5 || cp == CP_UTF7 || cp == CP_UTF8
			|| cp == CPEX_UNICODE_UTF16LE || cp == CPEX_UNICODE_UTF16BE
			|| cp == CPEX_UNICODE_UTF32LE || cp == CPEX_UNICODE_UTF32BE) {
		if(lineBreakCombobox_.getCount() != 6) {
			const int org = (lineBreakCombobox_.getCount() != 0) ? lineBreakCombobox_.getCurSel() : 0;

			lineBreakCombobox_.resetContent();
			lineBreakCombobox_.setItemData(lineBreakCombobox_.addString((lineBreak_ == LB_CRLF) ? IDS_BREAK_CRLF L" *" : IDS_BREAK_CRLF), LB_CRLF);
			lineBreakCombobox_.setItemData(lineBreakCombobox_.addString((lineBreak_ == LB_LF) ? IDS_BREAK_LF L" *" : IDS_BREAK_LF), LB_LF);
			lineBreakCombobox_.setItemData(lineBreakCombobox_.addString((lineBreak_ == LB_CR) ? IDS_BREAK_CR L" *" : IDS_BREAK_CR), LB_CR);
			lineBreakCombobox_.setItemData(lineBreakCombobox_.addString((lineBreak_ == LB_NEL) ? IDS_BREAK_NEL L" *" : IDS_BREAK_NEL), LB_NEL);
			lineBreakCombobox_.setItemData(lineBreakCombobox_.addString((lineBreak_ == LB_LS) ? IDS_BREAK_LS L" *" : IDS_BREAK_LS), LB_LS);
			lineBreakCombobox_.setItemData(lineBreakCombobox_.addString((lineBreak_ == LB_PS) ? IDS_BREAK_PS L" *" : IDS_BREAK_PS), LB_PS);
			lineBreakCombobox_.setCurSel(org);
		}
	} else {
		if(lineBreakCombobox_.getCount() != 3) {
			const int org = (lineBreakCombobox_.getCount() != 0) ? lineBreakCombobox_.getCurSel() : 0;

			lineBreakCombobox_.resetContent();
			lineBreakCombobox_.setItemData(lineBreakCombobox_.addString(
				(lineBreak_ == LB_CRLF) ? IDS_BREAK_CRLF L" *" : IDS_BREAK_CRLF), LB_CRLF);
			lineBreakCombobox_.setItemData(lineBreakCombobox_.addString(
				(lineBreak_ == LB_LF) ? IDS_BREAK_LF L" *" : IDS_BREAK_LF), LB_LF);
			lineBreakCombobox_.setItemData(lineBreakCombobox_.addString(
				(lineBreak_ == LB_CR) ? IDS_BREAK_CR L" *" : IDS_BREAK_CR), LB_CR);
			lineBreakCombobox_.setCurSel((org < lineBreakCombobox_.getCount()) ? org : 0);
		}
	}
	return true;
}

/// @see Dialog#onInitDialog
bool NewFileFormatDialog::onInitDialog(HWND focusWindow, LPARAM initParam) {
	Dialog::onInitDialog(focusWindow, initParam);

	set<CodePage> codePages;

	// [コードページ]
	EncoderFactory::getInstance().enumCodePages(codePages);
	for(set<CodePage>::const_iterator it = codePages.begin(); it != codePages.end(); ++it) {
		if(EncoderFactory::getInstance().isCodePageForAutoDetection(*it))
			continue;
		if(const wstring* name = Alpha::getInstance().getCodePageName(*it)) {
			const int i = codePageCombobox_.addString((*it == encoding_) ? (*name + L" *").c_str() : name->c_str());
			codePageCombobox_.setItemData(i, *it);
			if(*it == encoding_)
				codePageCombobox_.setCurSel(i);
		}
	}

	// [改行コード]
	onCommand(IDC_COMBO_CHARCODE, CBN_SELCHANGE, 0);
	for(int i = 0; i < 6; ++i) {
		if(lineBreak_ == static_cast<LineBreak>(lineBreakCombobox_.getItemData(i))) {
			lineBreakCombobox_.setCurSel(i);
			break;
		}
	}

//	// [文書タイプ]
//	for(list<wstring>::const_iterator it = documentTypes_.begin(); it != documentTypes_.end(); ++it)
//		documentTypeCombobox_.addString(it->c_str());
//	documentTypeCombobox_.setCurSel(static_cast<int>(documentType_));

	return true;
}

/// @see Dialog#onOK
void NewFileFormatDialog::onOK() {
	encoding_ = static_cast<CodePage>(codePageCombobox_.getItemData(codePageCombobox_.getCurSel()));
	lineBreak_ = static_cast<ascension::text::LineBreak>(lineBreakCombobox_.getItemData(lineBreakCombobox_.getCurSel()));
//	documentType_ = documentTypeCombobox_.getCurSel();

	Dialog::onOK();
}
