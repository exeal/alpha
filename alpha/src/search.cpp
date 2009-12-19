/**
 * @file search.cpp
 * @author exeal
 * @date 2003-2007 (was search-dialog.hpp)
 * @date 2008-2009
 */

#include "search.hpp"
#include "application.hpp"
#include "editor-window.hpp"
#include "resource.h"
#include "../resource/messages.h"
#include <ascension/text-editor.hpp>
#include <algorithm>	// std.find
#include <manah/win32/ui/standard-controls.hpp>
using alpha::ui::SearchDialog;
using alpha::ui::InteractiveReplacementCallback;
using namespace alpha;
using namespace std;
namespace a = ascension;
namespace k = ascension::kernel;
namespace re = ascension::regex;
namespace s = ascension::searcher;
namespace v = ascension::viewers;
namespace py = boost::python;


// free functions ///////////////////////////////////////////////////////////

namespace {
	// shows a message box indicating regular expression search error
	void showRegexErrorMessage(const re::PatternSyntaxException* e) {
		Alpha& app = Alpha::instance();
		if(e == 0)
			app.messageBox(MSG_ERROR__REGEX_UNKNOWN_ERROR, MB_ICONEXCLAMATION);
		else
			app.messageBox(MSG_SEARCH__INVALID_REGEX_PATTERN, MB_ICONEXCLAMATION,
				MARGS % app.loadMessage(MSG_SEARCH__BAD_PATTERN_START + e->getCode()) % static_cast<long>(e->getIndex()));
	}

	bool search(const wstring& pattern, a::Direction direction, bool noerror, py::ssize_t n) {
		return Alpha::instance().searchDialog().search(pattern, direction, noerror, static_cast<long>(n));
	}
}

size_t alpha::bookmarkMatchLines(const k::Region& region, bool interactive) {
	try {
		return a::texteditor::commands::BookmarkMatchLinesCommand(EditorWindows::instance().activePane().visibleView(), region)();
	} catch(re::PatternSyntaxException& e) {
		if(interactive)
			showRegexErrorMessage(&e);
	} catch(runtime_error&) {
		if(interactive)
			showRegexErrorMessage(0);
	}
	return 0;
}


// SearchDialog /////////////////////////////////////////////////////////////

/// Default constructor.
SearchDialog::SearchDialog() :
		initializesPatternFromEditor_(manah::toBoolean(Alpha::instance().readIntegerProfile(L"Find", L"initializeFromEditor", 1))) {
}


/// Returns the active pattern string.
wstring SearchDialog::activePattern() const throw() {
	if(const int len = patternCombobox_.getTextLength()) {
		manah::AutoBuffer<wchar_t> s(new wchar_t[len + 1]);
		patternCombobox_.getText(s.get(), len + 1);
		return wstring(s.get());
	}
	return L"";
}

/// Returns the active replacement string.
wstring SearchDialog::activeReplacement() const throw() {
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
	case IDC_BTN_FINDNEXT:		// "Find Forward"
		this->search(activePattern(), a::Direction::FORWARD, false);
		return true;
	case IDC_BTN_FINDPREVIOUS:	// "Find Backward"
		this->search(activePattern(), a::Direction::BACKWARD, false);
		return true;
	case IDC_BTN_MARKALL:		// "Mark All"
		bookmarkMatchLines(manah::toBoolean(isButtonChecked(IDC_RADIO_SELECTION)) ?
			EditorWindows::instance().activePane().visibleView().caret().selectedRegion() : k::Region(), true);
		return true;
	case IDC_BTN_REPLACE:		// "Replace"
		replaceAll(true);
		return true;
	case IDC_BTN_REPLACEALL:	// "Replace All"
		replaceAll(false);
		return true;
	case IDC_COMBO_FINDWHAT:	// "Find What"
		if(notifyCode != CBN_EDITCHANGE && notifyCode != CBN_SELCHANGE)
			break;
		if(notifyCode == CBN_EDITCHANGE)
			enableCommandsAsOnlySelection = ::GetWindowTextLength(getItem(IDC_COMBO_FINDWHAT)) != 0;
		::EnableWindow(getItem(IDC_BTN_MARKALL), enableCommandsAsOnlySelection);
		::EnableWindow(getItem(IDC_BTN_REPLACEALL),
			enableCommandsAsOnlySelection && !EditorWindows::instance().activeBuffer().isReadOnly());
		/* fall-through */
	case IDC_RADIO_WHOLEFILE:	// "Whole File"
	case IDC_RADIO_SELECTION:	// "Only Selection"
		if(isButtonChecked(IDC_RADIO_SELECTION))
			enableCommandsAsOnlySelection = false;
		::EnableWindow(getItem(IDC_BTN_FINDNEXT), enableCommandsAsOnlySelection);
		::EnableWindow(getItem(IDC_BTN_FINDPREVIOUS), enableCommandsAsOnlySelection);
		::EnableWindow(getItem(IDC_BTN_REPLACE),
			enableCommandsAsOnlySelection && !EditorWindows::instance().activeBuffer().isReadOnly());
		break;
	case IDC_COMBO_WHOLEMATCH:
		if(notifyCode == CBN_SELCHANGE) {
			s::TextSearcher::WholeMatch f;
			switch(wholeMatchCombobox_.getCurSel()) {
			case 0:
				f = s::TextSearcher::MATCH_UTF32_CODE_UNIT;
				break;
			case 1:
				f = s::TextSearcher::MATCH_GRAPHEME_CLUSTER;
				break;
			case 2:
				f = s::TextSearcher::MATCH_WORD;
				break;
			}
			BufferList::instance().editorSession().textSearcher().setWholeMatch(f);
		}
		break;
	case IDC_BTN_BROWSE: {	// "Extended Options"
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
	// make transparent
	modifyStyleEx(0, WS_EX_LAYERED);
	setLayeredAttributes(0, 220, LWA_ALPHA);

	Alpha& app = Alpha::instance();
	searchTypeCombobox_.addString(app.loadMessage(MSG_DIALOG__LITERAL_SEARCH).c_str());
	if(s::TextSearcher::isRegexAvailable())
		searchTypeCombobox_.addString(app.loadMessage(MSG_DIALOG__REGEX_SEARCH).c_str());
	if(s::TextSearcher::isRegexAvailable())
		searchTypeCombobox_.addString(app.loadMessage(MSG_DIALOG__MIGEMO_SEARCH).c_str());

	wholeMatchCombobox_.addString(app.loadMessage(MSG_OTHER__NONE).c_str());
	wholeMatchCombobox_.addString(app.loadMessage(MSG_DIALOG__WHOLE_GRAPHEME_MATCH).c_str());
	wholeMatchCombobox_.addString(app.loadMessage(MSG_DIALOG__WHOLE_WORD_MATCH).c_str());
	checkRadioButton(IDC_RADIO_SELECTION, IDC_RADIO_WHOLEFILE, IDC_RADIO_WHOLEFILE);

	collationWeightCombobox_.addString(L"15..IDENTICAL");
	collationWeightCombobox_.setCurSel(0);

	updateConditions();
	onCommand(IDC_COMBO_FINDWHAT, CBN_EDITCHANGE, getItem(IDC_COMBO_FINDWHAT));
}

/// @see Dialog#processWindowMessage
INT_PTR SearchDialog::processWindowMessage(UINT message, WPARAM wParam, LPARAM lParam) {
	switch(message) {
	case WM_SHOWWINDOW:
		if(manah::toBoolean(wParam) && lParam == 0 && initializesPatternFromEditor_) {	// ??? C4244@MSVC9
			v::Caret& caret = EditorWindows::instance().activePane().visibleView().caret();
			if(isSelectionEmpty(caret)) {
//				String s;
//				// TODO: obtain the word nearest from the caret position.
//				caret.getNearestWordFromCaret(0, 0, &s);
//				searchDialog_->setItemText(IDC_COMBO_FINDWHAT, s.c_str());
			} else if(caret.anchor().line() == caret.line())
				setItemText(IDC_COMBO_FINDWHAT, selectedString(caret).c_str());
		}
		break;
	}
	return Dialog::processWindowMessage(message, wParam, lParam);
}

void SearchDialog::rebuildPattern() {
	const a::String pattern(activePattern());
	s::TextSearcher::Type type;
	switch(searchTypeCombobox_.getCurSel()) {
	case 0:	type = s::TextSearcher::LITERAL; break;
	case 1:	type = s::TextSearcher::REGULAR_EXPRESSION; break;
	case 2:	type = s::TextSearcher::MIGEMO; break;
	}
	const int collationWeight = a::text::Collator::IDENTICAL;
	const bool caseSensitive = isButtonChecked(IDC_CHK_IGNORECASE) != BST_CHECKED;
	const bool canonicalEquivalents = isButtonChecked(IDC_CHK_CANONICALEQUIVALENTS) == BST_CHECKED;

	s::TextSearcher& searcher = BufferList::instance().editorSession().textSearcher();
	if(!searcher.hasPattern()
			|| pattern != searcher.pattern()
			|| type != searcher.type()
			|| caseSensitive != searcher.isCaseSensitive()
			|| canonicalEquivalents != searcher.usesCanonicalEquivalents()
			|| collationWeight != searcher.collationWeight()) {
		switch(type) {
		case s::TextSearcher::LITERAL:
			searcher.setPattern(auto_ptr<s::LiteralPattern>(new s::LiteralPattern(pattern, caseSensitive)));
			break;
		case s::TextSearcher::REGULAR_EXPRESSION:
			searcher.setPattern(re::Pattern::compile(pattern, re::Pattern::MULTILINE
				| (!caseSensitive ? re::Pattern::CASE_INSENSITIVE : 0) | (canonicalEquivalents ? re::Pattern::CANON_EQ : 0)));
			break;
		case s::TextSearcher::MIGEMO:
			searcher.setPattern(re::MigemoPattern::compile(pattern.data(), pattern.data() + pattern.length(), caseSensitive));
		}
	}
}

bool SearchDialog::repeatSearch(a::Direction direction, bool noerror /* = true */, long n /* = 1 */) {
	::PyErr_SetString(PyExc_NotImplementedError, "");
	py::throw_error_already_set();
	return false;
}

/**
 * Implements "replace all" command.
 * @param interactive set @c true to perform interactive replacements
 */
void SearchDialog::replaceAll(bool interactive) {
	static InteractiveReplacementCallback callback;
	const bool wasVisible = isVisible();
	v::TextViewer& textViewer = EditorWindows::instance().activePane().visibleView();
	callback.setTextViewer(textViewer);
	a::ulong c = -1;

//	rebuildPattern();
	if(isWindow())
		show(SW_HIDE);
	if(!interactive) {
		textViewer.document().beginCompoundChange();
		textViewer.freeze();
	}
	try {
		c = a::texteditor::commands::ReplaceAllCommand(textViewer,
			manah::toBoolean(isButtonChecked(IDC_RADIO_SELECTION)), activeReplacement(), interactive ? &callback : 0)();
	} catch(const re::PatternSyntaxException& e) {
		showRegexErrorMessage(&e);
	} catch(runtime_error&) {
		showRegexErrorMessage(0);
	}
	if(!interactive) {
		textViewer.unfreeze();
		textViewer.document().endCompoundChange();
		if(c == 0)
			Alpha::instance().messageBox(MSG_SEARCH__PATTERN_NOT_FOUND, MB_ICONINFORMATION);
		else if(c != -1)
			Alpha::instance().messageBox(MSG_SEARCH__REPLACE_DONE, MB_ICONINFORMATION, MARGS % c);
	}
	if(wasVisible && isButtonChecked(IDC_CHK_AUTOCLOSE) != BST_CHECKED) {
		show(SW_SHOW);
		::SetFocus(getItem(IDC_COMBO_FINDWHAT));
	}
}

bool SearchDialog::search(const a::String& pattern, a::Direction direction, bool noerror /* = true */, long n /* = 1 */) {
	patternCombobox_.setText(pattern.c_str());
	rebuildPattern();

	bool found;
	try {
		found = a::texteditor::commands::FindNextCommand(
			EditorWindows::instance().activePane().visibleView(), direction).setNumericPrefix(n)();
	} catch(const a::IllegalStateException& e) {
		::PyErr_SetString(PyExc_RuntimeError, e.what());
	} catch(const re::PatternSyntaxException& e) {
		if(!noerror)
			showRegexErrorMessage(&e);
		return false;
	} catch(runtime_error&) {
		if(!noerror)
			Alpha::instance().messageBox(MSG_ERROR__REGEX_UNKNOWN_ERROR, MB_ICONEXCLAMATION);
		return false;
	}
	if(!found & !noerror)
		Alpha::instance().messageBox(MSG_SEARCH__PATTERN_NOT_FOUND, MB_ICONINFORMATION);
	return found;
}

/// Updates GUI according to the current search options.
void SearchDialog::updateConditions() {
	const BufferList& buffers = BufferList::instance();
	const s::TextSearcher& searcher = buffers.editorSession().textSearcher();

	const a::String currentPattern(activePattern()), currentReplacement(activeReplacement());
	patternCombobox_.resetContent();
	for(size_t i = 0; i < searcher.numberOfStoredPatterns(); ++i)
		patternCombobox_.addString(searcher.pattern(i).c_str());
	replacementCombobox_.resetContent();
	for(size_t i = 0; i < searcher.numberOfStoredReplacements(); ++i)
		replacementCombobox_.addString(searcher.replacement(i).c_str());
	patternCombobox_.setText(currentPattern.c_str());
	replacementCombobox_.setText(currentReplacement.c_str());

	switch(searcher.type()) {
	case s::TextSearcher::LITERAL:				searchTypeCombobox_.setCurSel(0); break;
	case s::TextSearcher::REGULAR_EXPRESSION:	searchTypeCombobox_.setCurSel(1); break;
	case s::TextSearcher::MIGEMO:				searchTypeCombobox_.setCurSel(2); break;
	}
	check2StateButton(IDC_CHK_IGNORECASE, !searcher.isCaseSensitive());
	check2StateButton(IDC_CHK_CANONICALEQUIVALENTS, searcher.usesCanonicalEquivalents());
	switch(searcher.wholeMatch()) {
	case s::TextSearcher::MATCH_UTF32_CODE_UNIT:	wholeMatchCombobox_.setCurSel(0); break;
	case s::TextSearcher::MATCH_GRAPHEME_CLUSTER:	wholeMatchCombobox_.setCurSel(1); break;
	case s::TextSearcher::MATCH_WORD:				wholeMatchCombobox_.setCurSel(2); break;
	}

	const bool patternIsEmpty = patternCombobox_.getTextLength() == 0;
	const bool hasSelection = !isSelectionEmpty(EditorWindows::instance().activePane().visibleView().caret());
	const bool readOnly = EditorWindows::instance().activeBuffer().isReadOnly();
	const bool onlySelection = isButtonChecked(IDC_RADIO_SELECTION) == BST_CHECKED;
	if(!hasSelection)
		checkRadioButton(IDC_RADIO_SELECTION, IDC_RADIO_WHOLEFILE, IDC_RADIO_WHOLEFILE);
	::EnableWindow(getItem(IDC_BTN_FINDNEXT), !patternIsEmpty && !onlySelection);
	::EnableWindow(getItem(IDC_BTN_FINDPREVIOUS), !patternIsEmpty && !onlySelection);
	::EnableWindow(getItem(IDC_BTN_MARKALL), !patternIsEmpty);
	::EnableWindow(getItem(IDC_BTN_REPLACE), !patternIsEmpty && !onlySelection && !readOnly);
	::EnableWindow(getItem(IDC_BTN_REPLACEALL), !patternIsEmpty && !readOnly);
	::EnableWindow(getItem(IDC_RADIO_SELECTION), hasSelection);
}


// InteractiveReplacementCallback ///////////////////////////////////////////

/// Default constructor.
InteractiveReplacementCallback::InteractiveReplacementCallback() :
		menu_(Alpha::instance().loadMenu(IDR_MENU_REPLACEALLACTION)), textViewer_(0) {
	if(menu_ == 0)
		throw runtime_error("popup menu can't load.");
}

/// Destructor.
InteractiveReplacementCallback::~InteractiveReplacementCallback() throw() {
	::DestroyMenu(menu_);
}

/// @see InteractiveReplacementCallback#queryReplacementAction
s::IInteractiveReplacementCallback::Action
InteractiveReplacementCallback::queryReplacementAction(const k::Region& matchedRegion, bool canUndo) {
	textViewer_->caret().select(matchedRegion);

	POINT p = textViewer_->clientXYForCharacter(matchedRegion.beginning(), false);
	UINT popupFlags = TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_LEFTBUTTON | TPM_NOANIMATION | TPM_VERTICAL;
	if(p.y == -32768)
		p.y = 0;
	else if(p.y == 32767) {
		RECT clientRect;
		textViewer_->getClientRect(clientRect);
		p.y = clientRect.bottom;
	} else
		p.y += textViewer_->textRenderer().lineHeight();
	textViewer_->clientToScreen(p);

//	TPMPARAMS tpmp;
//	tpmp.cbSize = sizeof(TPMPARAMS);
//	tpmp.rcExclude = ;
	Action action = EXIT;
	textViewer_->unfreeze();
//	textViewer_->document().endCompoundChange();
	switch(static_cast<UINT>(::TrackPopupMenuEx(
			::GetSubMenu(menu_, 0), popupFlags, p.x, p.y, textViewer_->get(), 0/*&tpmp*/))) {
	case IDYES:		action = REPLACE; break;
	case IDNO:		action = SKIP; break;
	case IDRETRY:	action = UNDO; break;
	case IDOK:		action = REPLACE_ALL; break;
	case IDCLOSE:	action = REPLACE_AND_EXIT; break;
	case IDCANCEL:
	case 0:			action = EXIT; break;
	}
	if(action == REPLACE || action == REPLACE_ALL || action == REPLACE_AND_EXIT) {
//		textViewer_->document().beginCompoundChange();
		textViewer_->freeze();
	}
	return action;
}

/// @see InteractiveReplacementCallback#replacementEnded
void InteractiveReplacementCallback::replacementEnded(size_t numberOfMatches, size_t numberOfReplacements) {
	textViewer_->unfreeze();
//	textViewer_->document().endCompoundChange();
	if(numberOfMatches == 0)
		Alpha::instance().messageBox(MSG_SEARCH__PATTERN_NOT_FOUND, MB_ICONINFORMATION);
	else
		Alpha::instance().messageBox(MSG_SEARCH__REPLACE_DONE, MB_ICONINFORMATION, MARGS % numberOfReplacements);
}

/// @see InteractiveReplacementCallback#replacementStarted
void InteractiveReplacementCallback::replacementStarted(const k::Document& document, const k::Region& scope) {
//	textViewer_->document().endCompoundChange();
}

/**
 * Sets the new text viewer.
 * @param textViewer the text viewer to search
 */
void InteractiveReplacementCallback::setTextViewer(v::TextViewer& textViewer) {
	textViewer_ = &textViewer;
}


namespace {
	struct SearchDialogProxy {
		static bool isVisible() {
			return Alpha::instance().searchDialog().isVisible();
		}
		static void show(py::object hide) {
			ui::SearchDialog& dialog = Alpha::instance().searchDialog();
			if((hide == py::object() && !dialog.isVisible()) || ::PyObject_IsTrue(hide.ptr())) {
				if(!dialog.isVisible())
					dialog.show(SW_SHOW);
				else
					dialog.setActive();
				::SetFocus(dialog.getItem(IDC_COMBO_FINDWHAT));
			} else
				dialog.show(SW_HIDE);
		}
	};

	void isearch(a::Direction direction) {
		EditorWindows::instance().activePane().visibleView().beginIncrementalSearch(
			BufferList::instance().editorSession().textSearcher().type(), direction);}
	void queryReplace() {Alpha::instance().searchDialog().replaceAll(true);}
	void replaceString() {Alpha::instance().searchDialog().replaceAll(false);}
} // namespace @0

ALPHA_EXPOSE_PROLOGUE(ambient::Interpreter::LOWEST_INSTALLATION_ORDER)
	ambient::Interpreter& interpreter = ambient::Interpreter::instance();
	{
		py::scope scope(interpreter.module("intrinsics"));
		py::def("incremental_search", &isearch, py::arg("direction"));
		py::def("query_replace", &queryReplace);
//		py::def("repeat_search", &repeatSearch, (py::arg("direction"), py::arg("noerror") = true, py::arg("n") = 1));
		py::def("replace_string", &replaceString);
		py::def("search", &::search, (py::arg("pattern"), py::arg("direction"), py::arg("noerror") = true, py::arg("n") = 1));
	}
	{
		py::scope scope(interpreter.module("ui"));
		py::class_<SearchDialogProxy, boost::noncopyable>("SearchDialog", py::no_init)
			.def("is_visible", &SearchDialogProxy::isVisible).staticmethod("is_visible")
			.def("show", &SearchDialogProxy::show, py::arg("hide") = py::object()).staticmethod("show");
	}
ALPHA_EXPOSE_EPILOGUE()
