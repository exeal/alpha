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
using manah::AutoBuffer;

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
		show(SW_HIDE);
	}
}

/// Completes (or aborts if no candidates completely match).
void CompletionWindow::complete() {
	const int sel = getCurSel();
	if(sel != LB_ERR) {
		Caret& caret = viewer_.getCaret();
		Document& document = viewer_.getDocument();
		caret.clearSelection();

		const String precedingID = viewer_.getCaret().getPrecedingIdentifier(COMPLETION_MAX_TRACKBACK_CCH);
		const length_t len = getTextLen(sel);
		AutoBuffer<Char> text(new Char[len + 1]);
		getText(sel, text.get());
		viewer_.freeze(true);
		document.beginSequentialEdit();
		document.deleteText(Position(caret.getLineNumber(), caret.getColumnNumber() - precedingID.length()), caret);
		caret.moveTo(document.insertText(caret, text.get(), text.get() + len));
		document.endSequentialEdit();
		viewer_.unfreeze(true);
	}
	abort();
}

/**
 * Creates the list window.
 * @return succeeded or not
 */
bool CompletionWindow::create() {
	using namespace manah::win32::ui;

	if(ListBox::create(viewer_.get(), DefaultWindowRect(), 0, 0,
			WS_CHILD | WS_TABSTOP | WS_VSCROLL | LBS_NOINTEGRALHEIGHT | LBS_NOTIFY | LBS_SORT,
			WS_EX_DLGMODALFRAME | WS_EX_NOPARENTNOTIFY | WS_EX_TOOLWINDOW)
			&& subclass()) {
		updateDefaultFont();
		setPosition(HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

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

/**
 * Returns the region for the completion. if the caret goes outside of this, the completion will be aborted
 * @throw std#logic_error the completion is not running
 */
Region CompletionWindow::getContextRegion() const {
	if(!isRunning())
		throw logic_error("Completion is not running.");
	return Region(contextStart_, contextEnd_->getPosition());
}

/// @see ListBox#preTranslateWindowMessage
LRESULT CompletionWindow::preTranslateWindowMessage(UINT message, WPARAM wParam, LPARAM lParam, bool& handled) {
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
			::POINT pt = {LOWORD(lParam), HIWORD(lParam)};
			bool outside;
			const int sel = itemFromPoint(pt, outside);
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

	return ListBox::preTranslateWindowMessage(message, wParam, lParam, handled);
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
	const unicode::IdentifierSyntax& syntax = viewer_.getDocument().getContentTypeInformation().getIdentifierSyntax(caret.getContentType());

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
	const String precedingID = caret.getPrecedingIdentifier(COMPLETION_MAX_TRACKBACK_CCH);
	contextStart_.column = caret.getColumnNumber() - precedingID.length();
	contextEnd_->moveTo(caret);
	if(syntax.isIdentifierContinueCharacter(contextEnd_->getCodePoint()))
		contextEnd_->wordEndNext();
	contextEnd_->adaptToDocument(true);
	running_ = true;
	return true;
}

/// Updates the default font with system parameter.
void CompletionWindow::updateDefaultFont() {
	manah::win32::AutoZeroCB<::NONCLIENTMETRICSW> ncm;
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

	const String precedingID = viewer_.getCaret().getPrecedingIdentifier(COMPLETION_MAX_TRACKBACK_CCH);
	if(!precedingID.empty()) {
		const int found = findString(-1, precedingID.c_str());
		setCurSel((found != LB_ERR) ? found : -1);
		if(found != LB_ERR) {
			if(found != 0)	// そのままだと初回だけ選択項目が不可視になるみたい
				setCurSel(found - 1);
			setCurSel(found);
			if(found != getCount() - 1) {
				const int nextLength = getTextLen(found + 1);
				AutoBuffer<wchar_t> next(new wchar_t[nextLength + 1]);
				getText(found + 1, next.get());
				return unicode::CaseFolder::compare(
					unicode::StringCharacterIterator(precedingID),
					unicode::StringCharacterIterator(next.get(), next.get() + nextLength)) == 0;
			}
		}
	} else
		setCurSel(-1);
	return false;
}
