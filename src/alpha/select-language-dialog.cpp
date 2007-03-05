/**
 * @file select-language-dialog.cpp
 * @author exeal
 * @date 2003-2007
 */

#include "stdafx.h"
#include "select-language-dialog.hpp"
#include "../manah/com/common.hpp"	// ComPtr
#include <comcat.h>					// ICatInformation
using alpha::ui::SelectLanguageDialog;
using namespace std;


/**
 * コンストラクタ
 * @param scriptName 処理中のスクリプトファイル
 */
SelectLanguageDialog::SelectLanguageDialog(const basic_string<WCHAR>& scriptName) : scriptName_(scriptName) {
}

/// @see Dialog#onCommand
bool SelectLanguageDialog::onCommand(WORD id, WORD notifyCode, HWND control) {
	if(id == IDC_LIST_SCRIPTENGINES && notifyCode == LBN_DBLCLK) {
		onOK();
		return true;
	}
	return Dialog::onCommand(id, notifyCode, control);
}

/// @see Dialog#onInitDialog
bool SelectLanguageDialog::onInitDialog(HWND focusWindow, LPARAM initParam) {
	Dialog::onInitDialog(focusWindow, initParam);

	using manah::com::ComPtr;

	ComPtr<ICatInformation> catInfo;
	static const CATID CATID_ActiveScript = {
		0xf0b7a1a1, 0x9847, 0x11cf, {0x8f, 0x20, 0x00, 0x80, 0x5f, 0x2c, 0xd0, 0x64}};
	static const CATID CATID_ActiveScriptParse = {
		0xf0b7a1a2, 0x9847, 0x11cf, {0x8f, 0x20, 0x00, 0x80, 0x5f, 0x2c, 0xd0, 0x64}};

	// コンポーネントカテゴリから利用可能なスクリプト言語を列挙する
	if(SUCCEEDED(catInfo.createInstance(CLSID_StdComponentCategoriesMgr))) {
		ComPtr<IEnumCLSID> clsidEnumerator;
		CATID impls[1] = {CATID_ActiveScript};
		CATID reqs[1] = {CATID_ActiveScriptParse};

		if(SUCCEEDED(catInfo->EnumClassesOfCategories(1, impls, 1, reqs, &clsidEnumerator))) {
			CLSID clsid;
			OLECHAR* languageName;
			for(clsidEnumerator->Reset(); clsidEnumerator->Next(1, &clsid, 0) == S_OK; ) {
				if(SUCCEEDED(::ProgIDFromCLSID(clsid, &languageName))) {
					languageListbox_.addString(languageName);
					::CoTaskMemFree(languageName);
				}
			}
		}
	}

	if(languageListbox_.getCount() == 0)
		::EnableWindow(getItem(IDOK), false);
	else
		languageListbox_.setCurSel(0);

	return true;
}

/// @see Dialog#onOK
void SelectLanguageDialog::onOK() {
	wchar_t* selection = 0;
	int sel = languageListbox_.getCurSel();
	size_t len = languageListbox_.getTextLen(sel);

	selection = new wchar_t[len + 1];
	languageListbox_.getText(sel, selection);
	selectedLanguage_.assign(selection);
	delete[] selection;

	Dialog::onOK();
}
