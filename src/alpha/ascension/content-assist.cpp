/**
 * @file content-assist.cpp
 * @author exeal
 * @date 2003-2006 (was CompletionWindow.cpp)
 * @date 2006-2007
 */

#include "stdafx.h"
#include "content-assist.hpp"
#include "viewer.hpp"	// SourceViewer

using namespace ascension;
using namespace ascension::contentassist;
using namespace ascension::text;
using namespace ascension::viewers;
using namespace std;

namespace {
	const length_t COMPLETION_MAX_TRACKBACK_CCH = 100;
}


/**
 * Constructor.
 * @param viewer the target source viewer
 */
CompletionWindow::CompletionWindow(SourceViewer& viewer) :
		viewer_(viewer), defaultFont_(0), running_(false), contextEnd_(new VisualPoint(viewer)) {
}

/// Destructor.
CompletionWindow::~CompletionWindow() {
	delete contextEnd_;
}

/// Aborts the running completion.
void CompletionWindow::abort() {
	if(isRunning()) {
		running_ = false;
		contextEnd_->adaptToDocument(false);
		showWindow(SW_HIDE);
	}
}

/// Completes (or aborts if no candidates completely match).
void CompletionWindow::complete() {
	const int sel = getCurSel();
	if(sel != LB_ERR) {
		Caret& caret = viewer_.getCaret();
		Document& document = viewer_.getDocument();
		const length_t len = getTextLen(sel);
		Char* text = new Char[len + 1];
		caret.clearSelection();

		String precWord;
		viewer_.getCaret().getPrecedingIdentifier(COMPLETION_MAX_TRACKBACK_CCH, precWord);
		getText(sel, text);
		viewer_.freeze(true);
		document.beginSequentialEdit();
		document.deleteText(Position(caret.getLineNumber(), caret.getColumnNumber() - precWord.length()), caret);
		caret.moveTo(document.insertText(caret, text, text + len));
		document.endSequentialEdit();
		viewer_.unfreeze(true);
		delete[] text;
	}
	abort();
}

/**
 * Creates the list window.
 * @return succeeded or not
 */
bool CompletionWindow::create() {
	using namespace manah::windows::ui;

	if(ListBox::create(viewer_, DefaultWindowRect(), 0, 0,
			WS_CHILD | WS_TABSTOP | WS_VSCROLL | LBS_NOINTEGRALHEIGHT | LBS_NOTIFY | LBS_SORT,
			WS_EX_DLGMODALFRAME | WS_EX_NOPARENTNOTIFY | WS_EX_TOOLWINDOW)
			&& subclassWindow()) {
		updateDefaultFont();
		setWindowPos(HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

#if 0
		// 影を付けてみたりする...
#ifndef CS_DROPSHADOW
		const ULONG_PTR CS_DROPSHADOW = 0x00020000UL;
#endif /* !CS_DROPSHADOW */
		const ULONG_PTR styleBits = ::GetClassLongPtrW(getHandle(), GCL_STYLE);
		::SetClassLongPtrW(getHandle(), GCL_STYLE, styleBits | CS_DROPSHADOW);
#endif

		return true;
	}
	return false;
}

/// @see ListBox#dispatchEvent
LRESULT CompletionWindow::dispatchEvent(UINT message, WPARAM wParam, LPARAM lParam) {
	switch(message) {
	case WM_DESTROY:
		::DeleteObject(defaultFont_);
		break;
	case WM_KILLFOCUS:
		abort();
		break;
	case WM_LBUTTONDBLCLK:	// ダブルクリックでも補完する
		complete();
		return true;
	case WM_LBUTTONDOWN: {	// 何で私が実装しなきゃならんのだ
			::POINT		pt = {LOWORD(lParam), HIWORD(lParam)};
			bool		outside;
			const int	sel = itemFromPoint(pt, outside);
			if(!outside)
				setCurSel(sel);
		}
		return true;
	case WM_SETTINGCHANGE:
		updateDefaultFont();
		break;
	case WM_SHOWWINDOW:
		if(!wParam)
			resetContent();
		break;
	}

	return ListBox::dispatchEvent(message, wParam, lParam);
}

/**
 * Returns the region for the completion. if the caret goes outside of this, the completion will be aborted
 * @throw std#logic_error the completion is not running
 */
Region CompletionWindow::getContextRegion() const {
	if(!isRunning())
		throw logic_error("Completion is not running.");
	return Region(contextStart_, contextEnd_->getPosition());
}

/**
 * Sets the new font.
 * @param font the font to be set. if set to @c null, default font will be selected
 */
void CompletionWindow::setFont(const HFONT font) {
	assertValidAsWindow();
	ListBox::setFont((font != 0) ? font : defaultFont_);
}

/**
 * Starts the completion.
 *
 * This method does not display and/or places the window.
 * @param candidateWords the candidate list
 * @return succeeded or not
 */
bool CompletionWindow::start(const set<String>& candidateWords) {
	assertValidAsWindow();

	Caret& caret = viewer_.getCaret();
	const unicode::CharacterDetector& ctypes = viewer_.getDocument().getContentTypeInformation().getCharacterDetector(caret.getContentType());

//	const bool rightToLeft = toBoolean(view_.getStyleEx() & WS_EX_RTLREADING);
//	const bool rightToLeft = view_.isTextDirectionRightToLeft();

	resetContent();
//	modifyStyleEx(rightToLeft ? 0: WS_EX_LAYOUTRTL, rightToLeft ? WS_EX_LAYOUTRTL : 0);
	for(set<String>::const_iterator it = candidateWords.begin(); it != candidateWords.end(); ++it) {
		if(!it->empty())
			addString(it->c_str());
	}

	const String& line = viewer_.getDocument().getLine(caret.getLineNumber());
	caret.clearSelection();
	contextStart_.line = caret.getLineNumber();
	String precWord;
	caret.getPrecedingIdentifier(COMPLETION_MAX_TRACKBACK_CCH, precWord);
	contextStart_.column = caret.getColumnNumber() - precWord.length();
	contextEnd_->moveTo(caret);
	if(ctypes.isIdentifierCharacter(contextEnd_->getCodePoint()))
		contextEnd_->wordEndNext();
	contextEnd_->adaptToDocument(true);
	running_ = true;
	return true;
}

/// Updates the default font with system parameter.
void CompletionWindow::updateDefaultFont() {
	manah::windows::AutoZeroCB<::NONCLIENTMETRICSW> ncm;
	::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);
	HFONT newFont = ::CreateFontIndirectW(&ncm.lfStatusFont);
	if(defaultFont_ != 0 && isWindow() && getFont() == defaultFont_) {
		HFONT oldFont = getFont();
		ListBox::setFont(newFont);
		::DeleteObject(oldFont);
	} else
		ListBox::setFont(newFont);
	defaultFont_ = newFont;
}

/**
 * Updates the list's cursel based on the viewer's context.
 * @return true if only one candidate matched
 * @throw std#logic_error the completion is not running
 */
bool CompletionWindow::updateListCursel() {
	assertValidAsWindow();

	if(!isRunning())
		throw logic_error("Completion is not running.");

	String precWord;
	viewer_.getCaret().getPrecedingIdentifier(COMPLETION_MAX_TRACKBACK_CCH, precWord);

	if(!precWord.empty()) {
		const int found = findString(-1, precWord.c_str());
		setCurSel((found != LB_ERR) ? found : -1);
		if(found != LB_ERR) {
			if(found != 0)	// そのままだと初回だけ選択項目が不可視になるみたい
				setCurSel(found - 1);
			setCurSel(found);
			if(found != getCount() - 1) {
				const size_t comparisonLength = min<size_t>(precWord.length(), getTextLen(found + 1));
				manah::AutoBuffer<wchar_t> prevWord = unicode::CaseFolder::foldSimple(precWord.data(), precWord.data() + comparisonLength);
				wchar_t* nextCand = new wchar_t[getTextLen(found + 1) + 1];

				getText(found + 1, nextCand);
				manah::AutoBuffer<wchar_t> nextCandFolded = unicode::CaseFolder::foldSimple(nextCand, nextCand + comparisonLength);
				const bool unique = wmemcmp(prevWord.get(), nextCandFolded.get(), comparisonLength) != 0;

				delete[] nextCand;
				return unique;
			}
		}
	} else
		setCurSel(-1);
	return false;
}
