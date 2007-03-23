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


/// Returns the active pattern string.
wstring SearchDialog::getActivePattern() const throw() {
	if(const int len = patternCombobox_.getTextLength()) {
		manah::AutoBuffer<wchar_t> s(new wchar_t[len + 1]);
		patternCombobox_.getText(s.get(), len + 1);
		return wstring(s.get());
	}
	return L"";
}

/// Returns the active replacement string.
wstring SearchDialog::getActiveReplacement() const throw() {
	if(const int len = replacementCombobox_.getTextLength()) {
		manah::AutoBuffer<wchar_t> s(new wchar_t[len + 1]);
		replacementCombobox_.getText(s.get(), len + 1);
		return wstring(s.get());
	}
	return L"";
}

/// @see Dialog#onCancel
void SearchDialog::onCancel(bool& continueDialog) {
	show(SW_HIDE);
	continueDialog = true;
}

/// @see Dialog#onClose
void SearchDialog::onClose(bool& continueDialog) {
	show(SW_HIDE);
	continueDialog = true;
}

/// @see Dialog#onCommand
bool SearchDialog::onCommand(WORD id, WORD notifyCode, HWND control) {
	bool enableCommandsAsOnlySelection = true;

	switch(id) {
	case CMD_SEARCH_FINDNEXT:		// [��������]
	case CMD_SEARCH_FINDPREV:		// [�O������]
	case CMD_SEARCH_BOOKMARKALL:	// [���ׂă}�[�N]
	case CMD_SEARCH_REPLACEALL:		// [���ׂĒu��]
	case CMD_SEARCH_REPLACEANDNEXT:	// [�u��]
		getParent().sendMessage(WM_COMMAND, MAKEWPARAM(id, notifyCode), reinterpret_cast<LPARAM>(control));
		return true;
	case IDC_COMBO_FINDWHAT:	// [�������镶����]
		if(notifyCode != CBN_EDITCHANGE && notifyCode != CBN_SELCHANGE)
			break;
		if(notifyCode == CBN_EDITCHANGE)
			enableCommandsAsOnlySelection = ::GetWindowTextLength(getItem(IDC_COMBO_FINDWHAT)) != 0;
		::EnableWindow(getItem(CMD_SEARCH_BOOKMARKALL), enableCommandsAsOnlySelection);
		::EnableWindow(getItem(CMD_SEARCH_REPLACEALL),
			enableCommandsAsOnlySelection && !Alpha::getInstance().getBufferList().getActive().isReadOnly());
		/* fall-through */
	case IDC_RADIO_WHOLEFILE:	// [�t�@�C���S��]
	case IDC_RADIO_SELECTION:	// [�I��͈�]
		if(isButtonChecked(IDC_RADIO_SELECTION))
			enableCommandsAsOnlySelection = false;
		::EnableWindow(getItem(CMD_SEARCH_FINDNEXT), enableCommandsAsOnlySelection);
		::EnableWindow(getItem(CMD_SEARCH_FINDPREV), enableCommandsAsOnlySelection);
		::EnableWindow(getItem(CMD_SEARCH_REPLACEANDNEXT),
			enableCommandsAsOnlySelection && !Alpha::getInstance().getBufferList().getActive().isReadOnly());
		break;
	case IDC_BTN_BROWSE: {	// [�g���I�v�V����]
//			RECT rect;
//			::GetWindowRect(getDlgItem(IDC_BTN_BROWSE), &rect);
//			optionMenu_.getSubMenu(0).trackPopupMenu(TPM_LEFTALIGN | TPM_TOPALIGN, rect.left, rect.bottom, *this);
		}
		break;
	}
	return Dialog::onCommand(id, notifyCode, control);
}

/// @see Dialog#onInitDialog
void SearchDialog::onInitDialog(HWND, bool&) {
	// ��������
	modifyStyleEx(0, WS_EX_LAYERED);
	setLayeredAttributes(0, 220, LWA_ALPHA);

	Alpha& app = Alpha::getInstance();
	searchTypeCombobox_.addString(app.loadString(MSG_DIALOG__LITERAL_SEARCH).c_str());
	if(TextSearcher::isRegexAvailable())
		searchTypeCombobox_.addString(app.loadString(MSG_DIALOG__REGEXP_SEARCH).c_str());
	if(TextSearcher::isRegexAvailable())
		searchTypeCombobox_.addString(app.loadString(MSG_DIALOG__MIGEMO_SEARCH).c_str());

	wholeMatchCombobox_.addString(app.loadString(MSG_OTHER__NONE).c_str());
	wholeMatchCombobox_.addString(app.loadString(MSG_DIALOG__WHOLE_GRAPHEME_MATCH).c_str());
	wholeMatchCombobox_.addString(app.loadString(MSG_DIALOG__WHOLE_WORD_MATCH).c_str());
	checkRadioButton(IDC_RADIO_SELECTION, IDC_RADIO_WHOLEFILE, IDC_RADIO_WHOLEFILE);

	onCommand(IDC_COMBO_FINDWHAT, CBN_EDITCHANGE, getItem(IDC_COMBO_FINDWHAT));
}

/// GUI ��̃I�v�V�����������I�u�W�F�N�g�ɐݒ肷��
void SearchDialog::setOptions() {
	assertValidAsWindow();

	TextSearcher& searcher = Alpha::getInstance().getBufferList().getEditorSession().getTextSearcher();
	SearchOptions options = searcher.getOptions();

	switch(searchTypeCombobox_.getCurSel()) {
	case 0:	options.type = LITERAL; break;
	case 1:	options.type = REGULAR_EXPRESSION; break;
	case 2:	options.type = MIGEMO; break;
	}
	options.caseSensitive = isButtonChecked(IDC_CHK_IGNORECASE) != BST_CHECKED;
	options.canonicalEquivalents = isButtonChecked(IDC_CHK_CANONICALEQUIVALENTS) == BST_CHECKED;
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

/// @see Dialog#processWindowMessage
INT_PTR SearchDialog::processWindowMessage(UINT message, WPARAM wParam, LPARAM lParam) {
	if(message == WM_ACTIVATE) {
		if(LOWORD(wParam) == WA_INACTIVE)
			setOptions();
		else
			updateOptions();
	}
	return Dialog::processWindowMessage(message, wParam, lParam);
}

/// �����I�u�W�F�N�g�̐ݒ�� GUI �ɔ��f����
void SearchDialog::updateOptions() {
	const BufferList& buffers = Alpha::getInstance().getBufferList();
	const TextSearcher& s = buffers.getEditorSession().getTextSearcher();
	const SearchOptions& options = s.getOptions();

	const String currentPattern = getActivePattern(), currentReplacement = getActiveReplacement();
	patternCombobox_.resetContent();
	for(size_t i = 0; i < s.getNumberOfStoredPatterns(); ++i)
		patternCombobox_.addString(s.getPattern(i).c_str());
	replacementCombobox_.resetContent();
	for(size_t i = 0; i < s.getNumberOfStoredReplacements(); ++i)
		replacementCombobox_.addString(s.getReplacement(i).c_str());
	patternCombobox_.setText(currentPattern.c_str());
	replacementCombobox_.setText(currentReplacement.c_str());

	switch(options.type) {
	case LITERAL:				searchTypeCombobox_.setCurSel(0); break;
	case REGULAR_EXPRESSION:	searchTypeCombobox_.setCurSel(1); break;
	case MIGEMO:				searchTypeCombobox_.setCurSel(2); break;
	}
	check2StateButton(IDC_CHK_IGNORECASE, !options.caseSensitive);
	check2StateButton(IDC_CHK_CANONICALEQUIVALENTS, options.canonicalEquivalents);
	switch(options.wholeMatch) {
	case SearchOptions::NONE:				wholeMatchCombobox_.setCurSel(0); break;
	case SearchOptions::GRAPHEME_CLUSTER:	wholeMatchCombobox_.setCurSel(1); break;
	case SearchOptions::WORD:				wholeMatchCombobox_.setCurSel(2); break;
	}

	const bool patternIsEmpty = patternCombobox_.getTextLength() == 0;
	const bool hasSelection = !buffers.getActiveView().getCaret().isSelectionEmpty();
	const bool readOnly = buffers.getActiveView().getDocument().isReadOnly();
	const bool onlySelection = isButtonChecked(IDC_RADIO_SELECTION) == BST_CHECKED;
	if(!hasSelection)
		checkRadioButton(IDC_RADIO_SELECTION, IDC_RADIO_WHOLEFILE, IDC_RADIO_WHOLEFILE);
	::EnableWindow(getItem(CMD_SEARCH_FINDNEXT), !patternIsEmpty && !onlySelection);
	::EnableWindow(getItem(CMD_SEARCH_FINDPREV), !patternIsEmpty && !onlySelection);
	::EnableWindow(getItem(CMD_SEARCH_BOOKMARKALL), !patternIsEmpty);
	::EnableWindow(getItem(CMD_SEARCH_REPLACEANDNEXT), !patternIsEmpty && !onlySelection && !readOnly);
	::EnableWindow(getItem(CMD_SEARCH_REPLACEALL), !patternIsEmpty && !readOnly);
	::EnableWindow(getItem(IDC_RADIO_SELECTION), hasSelection);
}
