/**
 * @file search-dialog.cpp
 * @author exeal
 * @date 2003-2007
 */

#include "stdafx.h"
#include "search-dialog.hpp"
#include "application.hpp"
#include "resource/messages.h"
#include "ascension/text-editor.hpp"
#include <algorithm>	// std.find
#include "../manah/win32/ui/standard-controls.hpp"
using alpha::Alpha;
using alpha::ui::SearchDialog;
using alpha::ui::InteractiveReplacementCallback;
using namespace ascension;
using namespace ascension::searcher;
using namespace std;


// SearchDialog /////////////////////////////////////////////////////////////

/// Implements "bookmark all" command.
void SearchDialog::bookmarkAll() {
	texteditor::commands::BookmarkAllCommand command(
		Alpha::getInstance().getBufferList().getActiveView(), toBoolean(isButtonChecked(IDC_RADIO_SELECTION)));
	setOptions();
	try {
		command.execute();
	} catch(regex::PatternSyntaxException& e) {
		showRegexErrorMessage(&e);
	} catch(runtime_error&) {
		showRegexErrorMessage(0);
	}
}

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
	case CMD_SEARCH_FINDNEXT - CMD_SPECIAL_START:		// [次を検索]
	case CMD_SEARCH_FINDPREV - CMD_SPECIAL_START:		// [前を検索]
	case CMD_SEARCH_BOOKMARKALL - CMD_SPECIAL_START:	// [すべてマーク]
	case CMD_SEARCH_REPLACEALL - CMD_SPECIAL_START:		// [すべて置換]
	case CMD_SEARCH_REPLACEALLINTERACTIVE - CMD_SPECIAL_START:	// [置換]
		getParent().sendMessage(WM_COMMAND, MAKEWPARAM(id, notifyCode), reinterpret_cast<LPARAM>(control));
		return true;
	case IDC_COMBO_FINDWHAT:	// [検索する文字列]
		if(notifyCode != CBN_EDITCHANGE && notifyCode != CBN_SELCHANGE)
			break;
		if(notifyCode == CBN_EDITCHANGE)
			enableCommandsAsOnlySelection = ::GetWindowTextLength(getItem(IDC_COMBO_FINDWHAT)) != 0;
		::EnableWindow(getItem(CMD_SEARCH_BOOKMARKALL), enableCommandsAsOnlySelection);
		::EnableWindow(getItem(CMD_SEARCH_REPLACEALL),
			enableCommandsAsOnlySelection && !Alpha::getInstance().getBufferList().getActive().isReadOnly());
		/* fall-through */
	case IDC_RADIO_WHOLEFILE:	// [ファイル全体]
	case IDC_RADIO_SELECTION:	// [選択範囲]
		if(isButtonChecked(IDC_RADIO_SELECTION))
			enableCommandsAsOnlySelection = false;
		::EnableWindow(getItem(CMD_SEARCH_FINDNEXT), enableCommandsAsOnlySelection);
		::EnableWindow(getItem(CMD_SEARCH_FINDPREV), enableCommandsAsOnlySelection);
		::EnableWindow(getItem(CMD_SEARCH_REPLACEALLINTERACTIVE),
			enableCommandsAsOnlySelection && !Alpha::getInstance().getBufferList().getActive().isReadOnly());
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
void SearchDialog::onInitDialog(HWND, bool&) {
	// 半透明化
	modifyStyleEx(0, WS_EX_LAYERED);
	setLayeredAttributes(0, 220, LWA_ALPHA);

	Alpha& app = Alpha::getInstance();
	searchTypeCombobox_.addString(app.loadMessage(MSG_DIALOG__LITERAL_SEARCH).c_str());
	if(TextSearcher::isRegexAvailable())
		searchTypeCombobox_.addString(app.loadMessage(MSG_DIALOG__REGEX_SEARCH).c_str());
	if(TextSearcher::isRegexAvailable())
		searchTypeCombobox_.addString(app.loadMessage(MSG_DIALOG__MIGEMO_SEARCH).c_str());

	wholeMatchCombobox_.addString(app.loadMessage(MSG_OTHER__NONE).c_str());
	wholeMatchCombobox_.addString(app.loadMessage(MSG_DIALOG__WHOLE_GRAPHEME_MATCH).c_str());
	wholeMatchCombobox_.addString(app.loadMessage(MSG_DIALOG__WHOLE_WORD_MATCH).c_str());
	checkRadioButton(IDC_RADIO_SELECTION, IDC_RADIO_WHOLEFILE, IDC_RADIO_WHOLEFILE);

	collationWeightCombobox_.addString(L"15..IDENTICAL");
	collationWeightCombobox_.setCurSel(0);

	onCommand(IDC_COMBO_FINDWHAT, CBN_EDITCHANGE, getItem(IDC_COMBO_FINDWHAT));
}

/// Implements "replace all" command.
void SearchDialog::replaceAll(bool interactive) {
	static InteractiveReplacementCallback callback;
	const bool wasVisible = isVisible();
	viewers::TextViewer& textViewer = Alpha::getInstance().getBufferList().getActiveView();
	callback.setTextViewer(textViewer);
	texteditor::commands::ReplaceAllCommand command(textViewer,
		toBoolean(isButtonChecked(IDC_RADIO_SELECTION)), interactive ? &callback : 0);
	ulong c = -1;

	setOptions();
	if(isWindow())
		show(SW_HIDE);
	if(!interactive) {
		textViewer.getDocument().beginSequentialEdit();
		textViewer.freeze();
	}
	try {
		c = command.execute();
	} catch(const regex::PatternSyntaxException& e) {
		showRegexErrorMessage(&e);
	} catch(runtime_error&) {
		showRegexErrorMessage(0);
	}
	if(!interactive) {
		textViewer.unfreeze();
		textViewer.getDocument().endSequentialEdit();
		if(c == 0)
			Alpha::getInstance().messageBox(MSG_SEARCH__PATTERN_NOT_FOUND, MB_ICONINFORMATION);
		else if(c != -1)
			Alpha::getInstance().messageBox(MSG_SEARCH__REPLACE_DONE, MB_ICONINFORMATION, MARGS % c);
	}
	if(wasVisible && isButtonChecked(IDC_CHK_AUTOCLOSE) != BST_CHECKED) {
		show(SW_SHOW);
		::SetFocus(getItem(IDC_COMBO_FINDWHAT));
	}
}

/// Implements "search next" command.
bool SearchDialog::searchNext(Direction direction) {
	texteditor::commands::FindNextCommand command(Alpha::getInstance().getBufferList().getActiveView(), direction);
	setOptions();
	bool found = false;
	try {
		if(command.execute() == 0)
			found = true;
		else
			Alpha::getInstance().messageBox(MSG_SEARCH__PATTERN_NOT_FOUND, MB_ICONINFORMATION);
	} catch(const regex::PatternSyntaxException& e) {
		showRegexErrorMessage(&e);
	} catch(runtime_error&) {
		Alpha::getInstance().messageBox(MSG_ERROR__REGEX_UNKNOWN_ERROR, MB_ICONEXCLAMATION);
	}
	if(isVisible()) {
		if(isButtonChecked(IDC_CHK_AUTOCLOSE) == BST_CHECKED)	// "Close automatically"
			Alpha::getInstance().getMainWindow().sendMessage(WM_COMMAND, CMD_SEARCH_FIND);
		else
			::SetFocus(getItem(IDC_COMBO_FINDWHAT));
	}
	return found;
}

/// GUI 上のオプションを検索オブジェクトに設定する
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

/// Shows a message box indicating regular expression search error.
void SearchDialog::showRegexErrorMessage(const regex::PatternSyntaxException* e) {
	Alpha& app = Alpha::getInstance();
	if(e == 0)
		app.messageBox(MSG_ERROR__REGEX_UNKNOWN_ERROR, MB_ICONEXCLAMATION);
	else
		app.messageBox(MSG_SEARCH__INVALID_REGEX_PATTERN, MB_ICONEXCLAMATION,
			MARGS % app.loadMessage(MSG_SEARCH__BAD_PATTERN_START + e->getCode()) % static_cast<long>(e->getIndex()));
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

/// 検索オブジェクトの設定を GUI に反映する
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
	::EnableWindow(getItem(CMD_SEARCH_REPLACEALLINTERACTIVE), !patternIsEmpty && !onlySelection && !readOnly);
	::EnableWindow(getItem(CMD_SEARCH_REPLACEALL), !patternIsEmpty && !readOnly);
	::EnableWindow(getItem(IDC_RADIO_SELECTION), hasSelection);
}


// InteractiveReplacementCallback ///////////////////////////////////////////

/// Default constructor.
InteractiveReplacementCallback::InteractiveReplacementCallback() :
		menu_(Alpha::getInstance().loadMenu(IDR_MENU_REPLACEALLACTION)), textViewer_(0) {
	if(menu_ == 0)
		throw runtime_error("popup menu can't load.");
}

/// Destructor.
InteractiveReplacementCallback::~InteractiveReplacementCallback() throw() {
	::DestroyMenu(menu_);
}

/// @see InteractiveReplacementCallback#queryReplacementAction
searcher::IInteractiveReplacementCallback::Action InteractiveReplacementCallback::queryReplacementAction(const text::Region& matchedRegion, bool canUndo) {
	textViewer_->getCaret().select(matchedRegion);

	::POINT p = textViewer_->getClientXYForCharacter(matchedRegion.beginning(), false);
	::UINT popupFlags = TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_LEFTBUTTON | TPM_NOANIMATION | TPM_VERTICAL;
	if(p.y == -32768)
		p.y = 0;
	else if(p.y == 32767) {
		::RECT clientRect;
		textViewer_->getClientRect(clientRect);
		p.y = clientRect.bottom;
	} else
		p.y += textViewer_->getTextRenderer().getLineHeight();
	textViewer_->clientToScreen(p);

//	::TPMPARAMS tpmp;
//	tpmp.cbSize = sizeof(::TPMPARAMS);
//	tpmp.rcExclude = ;
	Action action = EXIT;
	textViewer_->unfreeze();
	textViewer_->getDocument().endSequentialEdit();
	switch(static_cast<::UINT>(::TrackPopupMenuEx(
			::GetSubMenu(menu_, 0), popupFlags, p.x, p.y, textViewer_->getHandle(), 0/*&tpmp*/))) {
	case IDYES:			action = REPLACE; break;
	case IDNO:			action = SKIP; break;
	case CMD_EDIT_UNDO:	action = UNDO; break;
	case IDOK:			action = REPLACE_ALL; break;
	case IDCLOSE:		action = REPLACE_AND_EXIT; break;
	case IDCANCEL:
	case 0:				action = EXIT; break;
	}
	if(action == REPLACE || action == REPLACE_ALL || action == REPLACE_AND_EXIT) {
		textViewer_->getDocument().beginSequentialEdit();
		textViewer_->freeze();
	}
	return action;
}

/// @see InteractiveReplacementCallback#replacementEnded
void InteractiveReplacementCallback::replacementEnded(size_t numberOfMatches, size_t numberOfReplacements) {
	textViewer_->unfreeze();
	textViewer_->getDocument().endSequentialEdit();
	if(numberOfMatches == 0)
		Alpha::getInstance().messageBox(MSG_SEARCH__PATTERN_NOT_FOUND, MB_ICONINFORMATION);
	else
		Alpha::getInstance().messageBox(MSG_SEARCH__REPLACE_DONE, MB_ICONINFORMATION, MARGS % numberOfReplacements);
}

/// @see InteractiveReplacementCallback#replacementStarted
void InteractiveReplacementCallback::replacementStarted(const text::Document& document, const text::Region& scope) {
	textViewer_->getDocument().endSequentialEdit();
}

/**
 * Sets the new text viewer.
 * @param textViewer the text viewer to search
 */
void InteractiveReplacementCallback::setTextViewer(viewers::TextViewer& textViewer) {
	textViewer_ = &textViewer;
}
