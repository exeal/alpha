/**
 * @file code-pages-dialog.cpp
 * @author exeal
 * @date 2003-2007
 */

#include "stdafx.h"
#include "code-pages-dialog.hpp"
#include "application.hpp"
#include "../manah/win32/ui/standard-controls.hpp"
using alpha::ui::CodePagesDialog;
using namespace ascension::encodings;
using namespace std;


/**
 * コンストラクタ
 * @param codePage 最初に選択されているコードページ
 * @param forReading ファイルを読み込むのに使うコードページを列挙する場合は true
 */
CodePagesDialog::CodePagesDialog(CodePage codePage, bool forReading) throw() : codePage_(codePage), forReading_(forReading) {
}

/// @see Dialog#onCommand
bool CodePagesDialog::onCommand(WORD id, WORD notifyCode, HWND control) {
	if(id == IDC_LIST_CODEPAGES && notifyCode == LBN_DBLCLK) {
		onOK();
		return true;
	}
	return Dialog::onCommand(id, notifyCode, control);
}

/// @see Dialog#onInitDialog
bool CodePagesDialog::onInitDialog(HWND focusWindow, LPARAM initParam) {
	Dialog::onInitDialog(focusWindow, initParam);

	const EncoderFactory& encoders = EncoderFactory::getInstance();
	set<CodePage> codePages;

	encoders.enumCodePages(codePages);
	for(set<CodePage>::const_iterator it = codePages.begin(); it != codePages.end(); ++it) {
		if(!forReading_ && (encoders.isCodePageForAutoDetection(*it) || encoders.isCodePageForReadOnly(*it)))
			continue;
		else if(const wstring* name = Alpha::getInstance().getCodePageName(*it))
			codepageList_.setItemData(codepageList_.addString(((codePage_ == *it) ? *name + L" *" : *name).c_str()), *it);
	}
	const int c = codepageList_.getCount();
	for(int i = 0; i < c; ++i) {
		if(codePage_ == codepageList_.getItemData(i)) {
			codepageList_.setCurSel(i);
			break;
		}
	}

	return true;
}

/// @see Dialog#onOK
void CodePagesDialog::onOK() {
	codePage_ = static_cast<CodePage>(codepageList_.getItemData(codepageList_.getCurSel()));
	Dialog::onOK();
}
