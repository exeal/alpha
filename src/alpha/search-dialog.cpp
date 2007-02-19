/**
 * @file search-dialog.cpp
 * @author exeal
 * @date 2003-2007
 */

#include "stdafx.h"
#include "search-dialog.hpp"
#include "application.hpp"
#include <algorithm>	// std::find
#include "../manah/win32/ui/standard-controls.hpp"
using alpha::Alpha;
using alpha::ui::SearchDialog;
using namespace ascension;
using namespace ascension::searcher;
using namespace std;


/**
 * コンストラクタ
 * @param app アプリケーション
 */
SearchDialog::SearchDialog(Alpha& app) : app_(app) {
}

/// アクティブな検索文字列を返す
wstring SearchDialog::getActivePattern() const throw() {
	if(const int len = patternCombobox_.getWindowTextLength()) {
		manah::AutoBuffer<wchar_t> s(new wchar_t[len + 1]);
		patternCombobox_.getWindowText(s.get(), len + 1);
		return wstring(s.get());
	}
	return L"";
}

/// アクティブな置換文字列を返す
wstring SearchDialog::getActiveReplacement() const throw() {
	if(const int len = replacementCombobox_.getWindowTextLength()) {
		manah::AutoBuffer<wchar_t> s(new wchar_t[len + 1]);
		replacementCombobox_.getWindowText(s.get(), len + 1);
		return wstring(s.get());
	}
	return L"";
}

/// @see Dialog#onActivate
void SearchDialog::onActivate(UINT state, HWND previousWindow, bool minimize) {
	if(state == WA_INACTIVE)
		setOptions();
	else
		updateOptions();
}

/// @see Dialog#OnCancel
void SearchDialog::onCancel() {
	onClose();
}

/// @see Dialog#OnClose
void SearchDialog::onClose() {
	showWindow(SW_HIDE);
//	Dialog::onClose();
}

/// @see Dialog#onCommand
bool SearchDialog::onCommand(WORD id, WORD notifyCode, HWND control) {
	bool enableCommandsAsOnlySelection = true;

	switch(id) {
	case CMD_SEARCH_FINDNEXT:		// [次を検索]
	case CMD_SEARCH_FINDPREV:		// [前を検索]
	case CMD_SEARCH_BOOKMARKALL:	// [すべてマーク]
	case CMD_SEARCH_REPLACEALL:		// [すべて置換]
	case CMD_SEARCH_REPLACEANDNEXT:	// [置換]
		getParent().sendMessage(WM_COMMAND, MAKEWPARAM(id, notifyCode), reinterpret_cast<LPARAM>(control));
		return true;
	case IDC_COMBO_FINDWHAT:	// [検索する文字列]
		if(notifyCode != CBN_EDITCHANGE && notifyCode != CBN_SELCHANGE)
			break;
		if(notifyCode == CBN_EDITCHANGE)
			enableCommandsAsOnlySelection = ::GetWindowTextLength(getDlgItem(IDC_COMBO_FINDWHAT)) != 0;
		::EnableWindow(getDlgItem(CMD_SEARCH_BOOKMARKALL), enableCommandsAsOnlySelection);
		::EnableWindow(getDlgItem(CMD_SEARCH_REPLACEALL),
			enableCommandsAsOnlySelection && !app_.getBufferList().getActive().isReadOnly());
		/* fall-through */
	case IDC_RADIO_WHOLEFILE:	// [ファイル全体]
	case IDC_RADIO_SELECTION:	// [選択範囲]
		if(isDlgButtonChecked(IDC_RADIO_SELECTION))
			enableCommandsAsOnlySelection = false;
		::EnableWindow(getDlgItem(CMD_SEARCH_FINDNEXT), enableCommandsAsOnlySelection);
		::EnableWindow(getDlgItem(CMD_SEARCH_FINDPREV), enableCommandsAsOnlySelection);
		::EnableWindow(getDlgItem(CMD_SEARCH_REPLACEANDNEXT),
			enableCommandsAsOnlySelection && !app_.getBufferList().getActive().isReadOnly());
		break;
	case IDC_BTN_BROWSE: {	// [拡張オプション]
//			RECT rect;
//			::GetWindowRect(getDlgItem(IDC_BTN_BROWSE), &rect);
//			optionMenu_.getSubMenu(0).trackPopupMenu(TPM_LEFTALIGN | TPM_TOPALIGN, rect.left, rect.bottom, *this);
		}
		break;
	}
	return Dialog::onCommand(id, notifyCode, control);
}

/// @see Dialog#onInitDialog
bool SearchDialog::onInitDialog(HWND focusWindow, LPARAM initParam) {
	Dialog::onInitDialog(focusWindow, initParam);

	// 半透明化
	modifyStyleEx(0, WS_EX_LAYERED);
	setLayeredWindowAttributes(0, 220, LWA_ALPHA);

	searchTypeCombobox_.addString(app_.loadString(MSG_DIALOG__LITERAL_SEARCH).c_str());
	if(TextSearcher::isRegexAvailable())
		searchTypeCombobox_.addString(app_.loadString(MSG_DIALOG__REGEXP_SEARCH).c_str());
	if(TextSearcher::isRegexAvailable())
		searchTypeCombobox_.addString(app_.loadString(MSG_DIALOG__MIGEMO_SEARCH).c_str());

	wholeMatchCombobox_.addString(app_.loadString(MSG_OTHER__NONE).c_str());
	wholeMatchCombobox_.addString(app_.loadString(MSG_DIALOG__WHOLE_GRAPHEME_MATCH).c_str());
	wholeMatchCombobox_.addString(app_.loadString(MSG_DIALOG__WHOLE_WORD_MATCH).c_str());
	checkRadioButton(IDC_RADIO_SELECTION, IDC_RADIO_WHOLEFILE, IDC_RADIO_WHOLEFILE);

	onCommand(IDC_COMBO_FINDWHAT, CBN_EDITCHANGE, getDlgItem(IDC_COMBO_FINDWHAT));

	return true;
}

/// GUI 上のオプションを検索オブジェクトに設定する
void SearchDialog::setOptions() {
	assertValidAsWindow();

	TextSearcher& searcher = app_.getBufferList().getEditorSession().getTextSearcher();
	SearchOptions options = searcher.getOptions();

	switch(searchTypeCombobox_.getCurSel()) {
	case 0:	options.type = LITERAL; break;
	case 1:	options.type = REGULAR_EXPRESSION; break;
	case 2:	options.type = MIGEMO; break;
	}
	options.caseSensitive = isDlgButtonChecked(IDC_CHK_IGNORECASE) != BST_CHECKED;
	options.canonicalEquivalents = isDlgButtonChecked(IDC_CHK_CANONICALEQUIVALENTS) == BST_CHECKED;
	switch(wholeMatchCombobox_.getCurSel()) {
	case 0:	options.wholeMatch = SearchOptions::NONE; break;
	case 1:	options.wholeMatch = SearchOptions::GRAPHEME_CLUSTER; break;
	case 2:	options.wholeMatch = SearchOptions::WORD; break;
	}
	searcher.setOptions(options);
	const wstring p = getActivePattern();
	if(!p.empty())
		searcher.setPattern(p);
	searcher.setReplacement(getActiveReplacement());
}

/// 検索オブジェクトの設定を GUI に反映する
void SearchDialog::updateOptions() {
	TextSearcher& s = app_.getBufferList().getEditorSession().getTextSearcher();
	const SearchOptions& options = s.getOptions();

	const String currentPattern = getActivePattern(), currentReplacement = getActiveReplacement();
	patternCombobox_.resetContent();
	for(size_t i = 0; i < s.getNumberOfStoredPatterns(); ++i)
		patternCombobox_.addString(s.getPattern(i).c_str());
	replacementCombobox_.resetContent();
	for(size_t i = 0; i < s.getNumberOfStoredReplacements(); ++i)
		replacementCombobox_.addString(s.getReplacement(i).c_str());
	patternCombobox_.setWindowText(currentPattern.c_str());
	replacementCombobox_.setWindowText(currentReplacement.c_str());

	switch(options.type) {
	case LITERAL:				searchTypeCombobox_.setCurSel(0); break;
	case REGULAR_EXPRESSION:	searchTypeCombobox_.setCurSel(1); break;
	case MIGEMO:				searchTypeCombobox_.setCurSel(2); break;
	}
	checkDlg2StateButton(IDC_CHK_IGNORECASE, !options.caseSensitive);
	checkDlg2StateButton(IDC_CHK_CANONICALEQUIVALENTS, options.canonicalEquivalents);
	switch(options.wholeMatch) {
	case SearchOptions::NONE:				wholeMatchCombobox_.setCurSel(0); break;
	case SearchOptions::GRAPHEME_CLUSTER:	wholeMatchCombobox_.setCurSel(1); break;
	case SearchOptions::WORD:				wholeMatchCombobox_.setCurSel(2); break;
	}

	const bool patternIsEmpty = patternCombobox_.getWindowTextLength() == 0;
	const bool hasSelection = !app_.getBufferList().getActiveView().getCaret().isSelectionEmpty();
	const bool readOnly = app_.getBufferList().getActiveView().getDocument().isReadOnly();
	const bool onlySelection = isDlgButtonChecked(IDC_RADIO_SELECTION) == BST_CHECKED;
	if(!hasSelection)
		checkRadioButton(IDC_RADIO_SELECTION, IDC_RADIO_WHOLEFILE, IDC_RADIO_WHOLEFILE);
	::EnableWindow(getDlgItem(CMD_SEARCH_FINDNEXT), !patternIsEmpty && !onlySelection);
	::EnableWindow(getDlgItem(CMD_SEARCH_FINDPREV), !patternIsEmpty && !onlySelection);
	::EnableWindow(getDlgItem(CMD_SEARCH_BOOKMARKALL), !patternIsEmpty);
	::EnableWindow(getDlgItem(CMD_SEARCH_REPLACEANDNEXT), !patternIsEmpty && !onlySelection && !readOnly);
	::EnableWindow(getDlgItem(CMD_SEARCH_REPLACEALL), !patternIsEmpty && !readOnly);
	::EnableWindow(getDlgItem(IDC_RADIO_SELECTION), hasSelection);
}
