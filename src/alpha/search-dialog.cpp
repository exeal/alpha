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
using alpha::ui::SearchDlg;
using namespace ascension;
using namespace ascension::searcher;
using namespace std;
using manah::windows::ui::Menu;


/**
 * コンストラクタ
 * @param app アプリケーション
 */
SearchDlg::SearchDlg(Alpha& app) : app_(app), optionMenu_(app.loadMenu(IDR_MENU_SEARCHOPTION)) {
	updateHistoryOnNextActivation_[0] = updateHistoryOnNextActivation_[1] = false;
}

/**
 * 履歴に文字列を追加
 * @param text 追加する文字列
 * @param replace 置換文字列のとき真
 */
void SearchDlg::addToHistory(const String& text, bool replace) {
	list<String>& target = replace ? replaceWiths_ : findWhats_;
	list<String>::iterator it = find(target.begin(), target.end(), text);

	if(it != target.end())
		target.erase(it);
	target.push_front(text);

	if(isWindowVisible()) {
		updateHistory(replace);
		if(replace)
			replaceWithCombobox_.setCurSel(0);
		else
			findWhatCombobox_.setCurSel(0);
	} else
		updateHistoryOnNextActivation_[replace ? 1 : 0] = true;
}

/**
 * 履歴を削除
 * @param replace 置換文字列のとき真
 */
void SearchDlg::clearHistory(bool replace) {
	(replace ? replaceWiths_ : findWhats_).clear();
	if(isWindowVisible())
		updateHistory(replace);
	else
		updateHistoryOnNextActivation_[replace ? 1 : 0] = true;
}

/// 検索文字列を返す
String SearchDlg::getFindText() const {
	assertValidAsWindow();
	return findWhatCombobox_.getWindowText();
}

/// 履歴を取得
void SearchDlg::getHistory(list<String>& findWhats, list<String>& replaceWiths) const {
	findWhats = findWhats_;
	replaceWiths = replaceWiths_;
}

/// 置換文字列を返す
ascension::String SearchDlg::getReplaceText() const {
	assertValidAsWindow();
	return replaceWithCombobox_.getWindowText();
}

/// 履歴を設定
void SearchDlg::setHistory(const list<String>& findWhats, const list<String>& replaceWiths) {
	// 重複をチェックしながらコピー (sort -> unique_copy でもいいけど)
	findWhats_.clear();
	for(list<String>::const_iterator it = findWhats.begin(); it != findWhats.end(); ++it) {
		if(findWhats_.end() == find(findWhats_.begin(), findWhats_.end(), *it))
			findWhats_.push_back(*it);
	}
	replaceWiths_.clear();
	for(list<String>::const_iterator it = replaceWiths.begin(); it != replaceWiths.end(); ++it) {
		if(replaceWiths_.end() == find(replaceWiths_.begin(), replaceWiths_.end(), *it))
			replaceWiths_.push_back(*it);
	}

	if(isWindowVisible()) {
		updateHistory(false);
		updateHistory(true);
	} else
		updateHistoryOnNextActivation_[0] = updateHistoryOnNextActivation_[1] = true;
}

/**
 * 履歴リストをコンボボックスに反映
 * @param replace 置換履歴のとき真
 */
void SearchDlg::updateHistory(bool replace) {
	assertValidAsWindow();

	const list<String>& history = replace ? replaceWiths_ : findWhats_;
	manah::windows::ui::ComboBox& combobox = replace ? replaceWithCombobox_ : findWhatCombobox_;
	const int sel = combobox.getCurSel();
	wchar_t* const active = (sel == CB_ERR) ? new wchar_t[combobox.getWindowTextLength() + 1] : 0;

	if(sel == CB_ERR)
		combobox.getWindowText(active, combobox.getWindowTextLength() + 1);
	combobox.resetContent();
	for(list<String>::const_iterator it = history.begin(); it != history.end(); ++it)
		combobox.addString(it->c_str());
	if(sel == CB_ERR) {
		combobox.setWindowText(active);
		delete[] active;
	} else
		combobox.setCurSel(sel);
}

/// 検索条件の更新
void SearchDlg::updateOptions() {
	using namespace ascension::unicode;
	assertValidAsWindow();

	EditorView& activeView = app_.getBufferList().getActiveView();
	TextSearcher::Options options = app_.getBufferList().getEditorSession().getTextSearcher().getOptions();

//	if(isDlgButtonChecked(IDC_CHK_ONLYIDENTIFIERS) == BST_CHECKED)
//		options.generalOptions |= SO_ONLY_IDENTIFIERS;
	options.wholeWord = isDlgButtonChecked(IDC_CHK_WHOLEWORD) == BST_CHECKED;
	switch(searchTypeCombobox_.getCurSel()) {
	case 0:	options.type = TextSearcher::LITERAL;				break;
	case 1:	options.type = TextSearcher::REGULAR_EXPRESSION;	break;
	case 2:	options.type = TextSearcher::MIGEMO;				break;
	}
	switch(caseSensitivityCombobox_.getCurSel()) {
	case 0:	options.foldings.caseFolding = CASEFOLDING_NONE;			break;
	case 1:	options.foldings.caseFolding = CASEFOLDING_ASCII;			break;
	case 2:	options.foldings.caseFolding = CASEFOLDING_UNICODE_SIMPLE;	break;
	case 3:	options.foldings.caseFolding = CASEFOLDING_UNICODE_FULL;	break;
	}

#define CHECK_OPTION(id, option)	\
	options.foldings.others[foldingoptions::option] = toBoolean(menu.getMenuItemState<Menu::BY_COMMAND>(id) & MFS_CHECKED)
	{
		Menu menu = optionMenu_.getSubMenu(0).getSubMenu(0);
		CHECK_OPTION(IDC_CHK_IGNOREPUNCTUATIONS, SKIP_PUNCTUATIONS);
		CHECK_OPTION(IDC_CHK_IGNORESYMBOLS, SKIP_SYMBOLS);
		CHECK_OPTION(IDC_CHK_IGNOREWHITESPACES, SKIP_WHITESPACES);
		CHECK_OPTION(IDC_CHK_IGNOREDIACRITICS, SKIP_DIACRITICS);
		CHECK_OPTION(IDC_CHK_IGNOREVOWELS, SKIP_VOWELS);
		CHECK_OPTION(IDC_CHK_IGNOREKASHIDA, SKIP_KASHIDA);
		CHECK_OPTION(IDC_CHK_IGNORECONTROLS, SKIP_CONTROLS);
	}{
		Menu menu = optionMenu_.getSubMenu(0).getSubMenu(1);
		CHECK_OPTION(IDC_CHK_ACCENTREMOVAL, ACCENT_REMOVAL);
		CHECK_OPTION(IDC_CHK_CANONICALDUPLICATESFOLDING, CANONICAL_DUPLICATES);
		CHECK_OPTION(IDC_CHK_DASHESFOLDING, DASHES);
		CHECK_OPTION(IDC_CHK_GREEKLETTERFORMSFOLDING, GREEK_LETTERFORMS);
		CHECK_OPTION(IDC_CHK_HEBREWALTERNATESFOLDING, HEBREW_ALTERNATES);
		CHECK_OPTION(IDC_CHK_JAMOFOLDING, JAMO);
		CHECK_OPTION(IDC_CHK_MATHSYMBOLFOLDING, MATH_SYMBOL);
		CHECK_OPTION(IDC_CHK_NATIVEDIGITFOLDING, NATIVE_DIGIT);
		CHECK_OPTION(IDC_CHK_NOBREAKFOLDING, NOBREAK);
		CHECK_OPTION(IDC_CHK_OVERLINEFOLDING, OVERLINE);
		CHECK_OPTION(IDC_CHK_POSITIONALFORMSFOLDING, POSITIONAL_FORMS);
		CHECK_OPTION(IDC_CHK_SMALLFORMSFOLDING, SMALL_FORMS);
		CHECK_OPTION(IDC_CHK_SPACEFOLDING, SPACE);
		CHECK_OPTION(IDC_CHK_SPACINGACCENTS, SPACING_ACCENTS);
		CHECK_OPTION(IDC_CHK_SUBSCRIPTFOLDING, SUBSCRIPT);
		CHECK_OPTION(IDC_CHK_SYMBOLFOLDING, SYMBOL);
		CHECK_OPTION(IDC_CHK_UNDERLINEFOLDING, UNDERLINE);
		CHECK_OPTION(IDC_CHK_VERTICALFORMSFOLDING, VERTICAL_FORMS);
		CHECK_OPTION(IDC_CHK_DIACRITICREMOVAL, DIACRITIC_REMOVAL);
		CHECK_OPTION(IDC_CHK_HANRADICALFOLDING, HAN_RADICAL);
		CHECK_OPTION(IDC_CHK_KANAFOLDING, KANA);
		CHECK_OPTION(IDC_CHK_LETTERFORMSFOLDING, LETTER_FORMS);
		CHECK_OPTION(IDC_CHK_SIMPLIFIEDHANFOLDING, SIMPLIFIED_HAN);
		CHECK_OPTION(IDC_CHK_SUPERSCRIPTFOLDING, SUPERSCRIPT);
		CHECK_OPTION(IDC_CHK_SUZHOUNUMERALFOLDING, SUZHOU_NUMERAL);
		CHECK_OPTION(IDC_CHK_WIDTHFOLDING, WIDTH);
	}{
		Menu menu = optionMenu_.getSubMenu(0).getSubMenu(2);
		CHECK_OPTION(IDC_CHK_CIRCLEDSYMBOLSEXPANSION, EXPAND_CIRCLED_SYMBOLS);
		CHECK_OPTION(IDC_CHK_DOTTED, EXPAND_DOTTED);
		CHECK_OPTION(IDC_CHK_ELLIPSISEXPANSION, EXPAND_ELLIPSIS);
		CHECK_OPTION(IDC_CHK_FRACTIONEXPANSION, EXPAND_FRACTION);
		CHECK_OPTION(IDC_CHK_INTEGRALEXPANSION, EXPAND_INTEGRAL);
		CHECK_OPTION(IDC_CHK_LIGATUREEXPANSIONMISC, EXPAND_LIGATURE);
		CHECK_OPTION(IDC_CHK_PARENTHESIZED, EXPAND_PARENTHESIZED);
		CHECK_OPTION(IDC_CHK_PRIMESEXPANSION, EXPAND_PRIMES);
		CHECK_OPTION(IDC_CHK_ROMANNUMERALS, EXPAND_ROMAN_NUMERALS);
		CHECK_OPTION(IDC_CHK_SQUARED, EXPAND_SQUARED);
		CHECK_OPTION(IDC_CHK_SQUAREDUNMARKED, EXPAND_SQUARED_UNMARKED);
		CHECK_OPTION(IDC_CHK_DIGRAPHS, EXPAND_DIGRAPH);
		CHECK_OPTION(IDC_CHK_OTHERMULTIGRAPHS, EXPAND_OTHER_MULTIGRAPHS);
	}
#undef CHECK_OPTION
	app_.getBufferList().getEditorSession().getTextSearcher().setOptions(options);
}

/// @see Dialog#onActivate
void SearchDlg::onActivate(UINT state, HWND previousWindow, bool minimize) {
	if(state == WA_INACTIVE)
		updateOptions();
	else {
		const EditorView& activeView = app_.getBufferList().getActiveView();
		const bool hasFindText = findWhatCombobox_.getWindowTextLength() != 0;
		const bool hasSelection = !activeView.getCaret().isSelectionEmpty();
		const bool readOnly = activeView.getDocument().isReadOnly();
		const bool onlySelection = isDlgButtonChecked(IDC_RADIO_SELECTION) == BST_CHECKED;
		const TextSearcher::Options& options = app_.getBufferList().getEditorSession().getTextSearcher().getOptions();

		if(updateHistoryOnNextActivation_[0]) {
			updateHistory(false);
			updateHistoryOnNextActivation_[0] = false;
		}
		if(updateHistoryOnNextActivation_[1]) {
			updateHistory(true);
			updateHistoryOnNextActivation_[1] = false;
		}

		if(!hasSelection)
			checkRadioButton(IDC_RADIO_SELECTION, IDC_RADIO_WHOLEFILE, IDC_RADIO_WHOLEFILE);
		::EnableWindow(getDlgItem(CMD_SEARCH_FINDNEXT), hasFindText && !onlySelection);
		::EnableWindow(getDlgItem(CMD_SEARCH_FINDPREV), hasFindText && !onlySelection);
		::EnableWindow(getDlgItem(CMD_SEARCH_BOOKMARKALL), hasFindText);
		::EnableWindow(getDlgItem(CMD_SEARCH_REPLACEANDNEXT), hasFindText && !onlySelection && !readOnly);
		::EnableWindow(getDlgItem(CMD_SEARCH_REPLACEALL), hasFindText && !readOnly);
		::EnableWindow(getDlgItem(IDC_RADIO_SELECTION), hasSelection);
		checkDlg2StateButton(IDC_CHK_WHOLEWORD, options.wholeWord);

		if(options.type == TextSearcher::LITERAL)
			searchTypeCombobox_.setCurSel(0);
		else if(options.type == TextSearcher::REGULAR_EXPRESSION)
			searchTypeCombobox_.setCurSel(1);
		else if(options.type == TextSearcher::MIGEMO)
			searchTypeCombobox_.setCurSel(2);

		using namespace ascension::unicode;
		if(options.foldings.caseFolding == CASEFOLDING_ASCII)
			caseSensitivityCombobox_.setCurSel(1);
		else if(options.foldings.caseFolding == CASEFOLDING_UNICODE_SIMPLE)
			caseSensitivityCombobox_.setCurSel(2);
		else if(options.foldings.caseFolding == CASEFOLDING_UNICODE_FULL)
			caseSensitivityCombobox_.setCurSel(3);
		else
			caseSensitivityCombobox_.setCurSel(0);

#define CHECK_IGNORANCE_OPTION(id, option)	\
	ignoranceMenu.checkMenuItem<Menu::BY_COMMAND>(id, options.foldings.others[foldingoptions::option])
#define CHECK_FOLDING_OPTION(id, option)	\
	foldingMenu.checkMenuItem<Menu::BY_COMMAND>(id, options.foldings.others[foldingoptions::option])
#define CHECK_PROVISIONAL_FOLDING_OPTION(id, option)	\
	foldingMenu.checkMenuItem<Menu::BY_COMMAND>(id, options.foldings.others[foldingoptions::option])
#define CHECK_EXPANSION_OPTION(id, option)	\
	expansionMenu.checkMenuItem<Menu::BY_COMMAND>(id, options.foldings.others[foldingoptions::option])
		Menu ignoranceMenu = optionMenu_.getSubMenu(0).getSubMenu(0);
		Menu foldingMenu = optionMenu_.getSubMenu(0).getSubMenu(1);
		Menu expansionMenu = optionMenu_.getSubMenu(0).getSubMenu(2);
		CHECK_IGNORANCE_OPTION(IDC_CHK_IGNOREPUNCTUATIONS, SKIP_PUNCTUATIONS);
		CHECK_IGNORANCE_OPTION(IDC_CHK_IGNORESYMBOLS, SKIP_SYMBOLS);
		CHECK_IGNORANCE_OPTION(IDC_CHK_IGNOREWHITESPACES, SKIP_WHITESPACES);
		CHECK_IGNORANCE_OPTION(IDC_CHK_IGNOREDIACRITICS, SKIP_DIACRITICS);
		CHECK_IGNORANCE_OPTION(IDC_CHK_IGNOREVOWELS, SKIP_VOWELS);
		CHECK_IGNORANCE_OPTION(IDC_CHK_IGNOREKASHIDA, SKIP_KASHIDA);
		CHECK_IGNORANCE_OPTION(IDC_CHK_IGNORECONTROLS, SKIP_CONTROLS);
		CHECK_FOLDING_OPTION(IDC_CHK_ACCENTREMOVAL, ACCENT_REMOVAL);
		CHECK_FOLDING_OPTION(IDC_CHK_CANONICALDUPLICATESFOLDING, CANONICAL_DUPLICATES);
		CHECK_FOLDING_OPTION(IDC_CHK_DASHESFOLDING, DASHES);
		CHECK_FOLDING_OPTION(IDC_CHK_GREEKLETTERFORMSFOLDING, GREEK_LETTERFORMS);
		CHECK_FOLDING_OPTION(IDC_CHK_HEBREWALTERNATESFOLDING, HEBREW_ALTERNATES);
		CHECK_FOLDING_OPTION(IDC_CHK_JAMOFOLDING, JAMO);
		CHECK_FOLDING_OPTION(IDC_CHK_MATHSYMBOLFOLDING, MATH_SYMBOL);
		CHECK_FOLDING_OPTION(IDC_CHK_NATIVEDIGITFOLDING, NATIVE_DIGIT);
		CHECK_FOLDING_OPTION(IDC_CHK_NOBREAKFOLDING, NOBREAK);
		CHECK_FOLDING_OPTION(IDC_CHK_OVERLINEFOLDING, OVERLINE);
		CHECK_FOLDING_OPTION(IDC_CHK_POSITIONALFORMSFOLDING, POSITIONAL_FORMS);
		CHECK_FOLDING_OPTION(IDC_CHK_SMALLFORMSFOLDING, SMALL_FORMS);
		CHECK_FOLDING_OPTION(IDC_CHK_SPACEFOLDING, SPACE);
		CHECK_FOLDING_OPTION(IDC_CHK_SPACINGACCENTS, SPACING_ACCENTS);
		CHECK_FOLDING_OPTION(IDC_CHK_SUBSCRIPTFOLDING, SUBSCRIPT);
		CHECK_FOLDING_OPTION(IDC_CHK_SYMBOLFOLDING, SYMBOL);
		CHECK_FOLDING_OPTION(IDC_CHK_UNDERLINEFOLDING, UNDERLINE);
		CHECK_FOLDING_OPTION(IDC_CHK_VERTICALFORMSFOLDING, VERTICAL_FORMS);
		CHECK_PROVISIONAL_FOLDING_OPTION(IDC_CHK_DIACRITICREMOVAL, DIACRITIC_REMOVAL);
		CHECK_PROVISIONAL_FOLDING_OPTION(IDC_CHK_HANRADICALFOLDING, HAN_RADICAL);
		CHECK_PROVISIONAL_FOLDING_OPTION(IDC_CHK_KANAFOLDING, KANA);
		CHECK_PROVISIONAL_FOLDING_OPTION(IDC_CHK_LETTERFORMSFOLDING, LETTER_FORMS);
		CHECK_PROVISIONAL_FOLDING_OPTION(IDC_CHK_SIMPLIFIEDHANFOLDING, SIMPLIFIED_HAN);
		CHECK_PROVISIONAL_FOLDING_OPTION(IDC_CHK_SUPERSCRIPTFOLDING, SUPERSCRIPT);
		CHECK_PROVISIONAL_FOLDING_OPTION(IDC_CHK_SUZHOUNUMERALFOLDING, SUZHOU_NUMERAL);
		CHECK_PROVISIONAL_FOLDING_OPTION(IDC_CHK_WIDTHFOLDING, WIDTH);
		CHECK_EXPANSION_OPTION(IDC_CHK_CIRCLEDSYMBOLSEXPANSION, EXPAND_CIRCLED_SYMBOLS);
		CHECK_EXPANSION_OPTION(IDC_CHK_DOTTED, EXPAND_DOTTED);
		CHECK_EXPANSION_OPTION(IDC_CHK_ELLIPSISEXPANSION, EXPAND_ELLIPSIS);
		CHECK_EXPANSION_OPTION(IDC_CHK_FRACTIONEXPANSION, EXPAND_FRACTION);
		CHECK_EXPANSION_OPTION(IDC_CHK_INTEGRALEXPANSION, EXPAND_INTEGRAL);
		CHECK_EXPANSION_OPTION(IDC_CHK_LIGATUREEXPANSIONMISC, EXPAND_LIGATURE);
		CHECK_EXPANSION_OPTION(IDC_CHK_PARENTHESIZED, EXPAND_PARENTHESIZED);
		CHECK_EXPANSION_OPTION(IDC_CHK_PRIMESEXPANSION, EXPAND_PRIMES);
		CHECK_EXPANSION_OPTION(IDC_CHK_ROMANNUMERALS, EXPAND_ROMAN_NUMERALS);
		CHECK_EXPANSION_OPTION(IDC_CHK_SQUARED, EXPAND_SQUARED);
		CHECK_EXPANSION_OPTION(IDC_CHK_SQUAREDUNMARKED, EXPAND_SQUARED_UNMARKED);
		CHECK_EXPANSION_OPTION(IDC_CHK_DIGRAPHS, EXPAND_DIGRAPH);
		CHECK_EXPANSION_OPTION(IDC_CHK_OTHERMULTIGRAPHS, EXPAND_OTHER_MULTIGRAPHS);
#undef CHECK_IGNORANCE_OPTION
#undef CHECK_FOLDING_OPTION
#undef CHECK_PROVISIONAL_FOLDING_OPTION
#undef CHECK_EXPANSION_OPTION
	}
}

/// @see CDialog#OnCancel
void SearchDlg::onCancel() {
	onClose();
}

/// @see CDialog#OnClose
void SearchDlg::onClose() {
	showWindow(SW_HIDE);
//	Dialog::onClose();
}

/// @see Dialog#onCommand
bool SearchDlg::onCommand(WORD id, WORD notifyCode, HWND control) {
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
			RECT rect;
			::GetWindowRect(getDlgItem(IDC_BTN_BROWSE), &rect);
			optionMenu_.getSubMenu(0).trackPopupMenu(TPM_LEFTALIGN | TPM_TOPALIGN, rect.left, rect.bottom, *this);
		}
		break;
	default:
		if(id >= IDC_CHK_IGNOREPUNCTUATIONS && id <= IDC_CHK_IGNORECONTROLS) {
			Menu menu = optionMenu_.getSubMenu(0).getSubMenu(0);
			menu.checkMenuItem<Menu::BY_COMMAND>(id, !toBoolean(menu.getMenuItemState<Menu::BY_COMMAND>(id) & MFS_CHECKED));
		} else if(id >= IDC_CHK_ACCENTREMOVAL && id <= IDC_CHK_VERTICALFORMSFOLDING) {
			Menu menu = optionMenu_.getSubMenu(0).getSubMenu(1);
			menu.checkMenuItem<Menu::BY_COMMAND>(id, !toBoolean(menu.getMenuItemState<Menu::BY_COMMAND>(id) & MFS_CHECKED));
		} else if(id >= IDC_CHK_CIRCLEDSYMBOLSEXPANSION && id <= IDC_CHK_OTHERMULTIGRAPHS) {
			Menu menu = optionMenu_.getSubMenu(0).getSubMenu(2);
			menu.checkMenuItem<Menu::BY_COMMAND>(id, !toBoolean(menu.getMenuItemState<Menu::BY_COMMAND>(id) & MFS_CHECKED));
		} else if(id >= IDC_CHK_DIACRITICREMOVAL && id <= IDC_CHK_WIDTHFOLDING) {
			Menu menu = optionMenu_.getSubMenu(0).getSubMenu(1);
			menu.checkMenuItem<Menu::BY_COMMAND>(id, !toBoolean(menu.getMenuItemState<Menu::BY_COMMAND>(id) & MFS_CHECKED));
		}
		break;
	}
	return Dialog::onCommand(id, notifyCode, control);
}

/// @see Dialog#onInitDialog
bool SearchDlg::onInitDialog(HWND focusWindow, LPARAM initParam) {
	Dialog::onInitDialog(focusWindow, initParam);

	modifyStyleEx(0, WS_EX_LAYERED);
	setLayeredWindowAttributes(0, 220, LWA_ALPHA);

	searchTypeCombobox_.addString(app_.loadString(MSG_DIALOG__LITERAL_SEARCH).c_str());
	if(TextSearcher::isRegexAvailable())
		searchTypeCombobox_.addString(app_.loadString(MSG_DIALOG__REGEXP_SEARCH).c_str());
//	searchTypeCombobox_.addString(app_.loadString(MSG_DIALOG__WILDCARD_SEARCH).c_str());
	if(TextSearcher::isRegexAvailable())
		searchTypeCombobox_.addString(app_.loadString(MSG_DIALOG__MIGEMO_SEARCH).c_str());

	caseSensitivityCombobox_.addString(app_.loadString(MSG_DIALOG__MATCH_CASE).c_str());
	caseSensitivityCombobox_.addString(app_.loadString(MSG_DIALOG__IGNORE_ASCII_CASE).c_str());
	caseSensitivityCombobox_.addString(app_.loadString(MSG_DIALOG__CASE_FOLD_SIMPLE).c_str());
//	caseSensitivityCombobox_.addString(app_.loadString(MSG_DIALOG__CASE_FOLD_FULL).c_str());
	checkRadioButton(IDC_RADIO_SELECTION, IDC_RADIO_WHOLEFILE, IDC_RADIO_WHOLEFILE);

	onCommand(IDC_COMBO_FINDWHAT, CBN_EDITCHANGE, getDlgItem(IDC_COMBO_FINDWHAT));

	return true;
}
