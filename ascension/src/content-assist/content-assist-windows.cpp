/**
 * @file content-assist-windows.cpp
 * Implements @c DefaultContentAssistant and @c DefaultContentAssistant#CompletionProposalsPopup
 * classes on Win32 window system.
 * @author exeal
 * @date 2012-03-05 separated from content-assist.cpp
 */

#include <ascension/platforms.hpp>
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)

#include <ascension/content-assist/default-content-assistant.hpp>
#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/text-viewer.hpp>

using namespace ascension;
using namespace ascension::contentassist;
using namespace ascension::viewer;
using namespace std;


// DefaultContentAssistant.CompletionProposalsPopup ///////////////////////////////////////////////

/**
 * Constructor.
 * @param parent The parent window
 * @param ui The user interface
 */
DefaultContentAssistant::CompletionProposalsPopup::CompletionProposalsPopup(
		TextViewer& parent, ContentAssistant::CompletionProposalsUI& ui)
		: win32::SubclassedWindow(parent.handle(), L"LISTBOX"), ui_(ui) {
	::SetWindowLongPtrW(handle().get(), GWL_STYLE, WS_CHILD | WS_TABSTOP | WS_VSCROLL | LBS_HASSTRINGS | LBS_NOINTEGRALHEIGHT | LBS_NOTIFY);
	::SetWindowLongPtrW(handle().get(), GWL_EXSTYLE, WS_EX_DLGMODALFRAME | WS_EX_NOPARENTNOTIFY | WS_EX_TOOLWINDOW);
	updateDefaultFont();
	::SetWindowPos(handle().get(), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
#if 0
	// drop shadow
#ifndef CS_DROPSHADOW
	const ULONG_PTR CS_DROPSHADOW = 0x00020000ul;
#endif // !CS_DROPSHADOW
	const ULONG_PTR styleBits = ::GetClassLongPtrW(handle().get(), GCL_STYLE);
	::SetClassLongPtrW(handle().get(), GCL_STYLE, styleBits | CS_DROPSHADOW);
#endif
}

void DefaultContentAssistant::CompletionProposalsPopup::end() {
	widgetapi::hide(*this);
	::SendMessageW(handle().get(), LB_RESETCONTENT, 0, 0);
}

#if 0
/**
 * Updates the list's cursel based on the viewer's context.
 * @return true if only one candidate matched
 * @throw IllegalStateException The completion is not running
 */
bool DefaultContentAssistant::CompletionProposalsPopup::updateListCursel() {
	if(!running_)
		throw IllegalStateException("Completion is not running.");

	const String precedingID = viewer_.getCaret().getPrecedingIdentifier(COMPLETION_MAX_TRACKBACK_CCH);
	if(!precedingID.empty()) {
		const int found = findString(-1, precedingID.c_str());
		setCurSel((found != LB_ERR) ? found : -1);
		if(found != LB_ERR) {
			if(found != 0)	// ÇªÇÃÇ‹Ç‹ÇæÇ∆èââÒÇæÇØëIëçÄñ⁄Ç™ïsâ¬éãÇ…Ç»ÇÈÇ›ÇΩÇ¢
				setCurSel(found - 1);
			setCurSel(found);
			if(found != getCount() - 1) {
				const int nextLength = getTextLen(found + 1);
				unique_ptr<wchar_t[]> next(new wchar_t[nextLength + 1]);
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
#endif

LRESULT DefaultContentAssistant::CompletionProposalsPopup::processMessage(UINT message, WPARAM wp, LPARAM lp, bool& consumed) {
	switch(message) {
		case WM_DESTROY:
			::DeleteObject(defaultFont_);
			break;
		case WM_LBUTTONDBLCLK:
			ui_.complete();
			break;
		case WM_LBUTTONDOWN: {
			const LRESULT item = ::SendMessageW(handle().get(), LB_ITEMFROMPOINT, 0, lp);
			::SendMessageW(handle().get(), LB_SETCURSEL, (HIWORD(item) == 0) ? LOWORD(item) : -1, 0);
			break;
		}
		case WM_SETFOCUS:
			::SetFocus(::GetParent(handle().get()));
			consumed = true;
			return 0;
		case WM_SETTINGCHANGE:
		case WM_THEMECHANGED:
			updateDefaultFont();
			break;
	}
	return SubclassedWindow::processMessage(message, wp, lp, consumed);
}

void DefaultContentAssistant::CompletionProposalsPopup::resetContent(shared_ptr<const CompletionProposal> proposals[], size_t numberOfProposals) {
	decltype(proposals_) newProposals;
//	copy(proposals, proposals + numberOfProposals, back_inserter(newProposals));
	newProposals.reserve(numberOfProposals);

	::SendMessageA(handle().get(), LB_RESETCONTENT, 0, 0);
	for(size_t i = 0; i < numberOfProposals; ++i) {
		// TODO: display icons.
		const String s(proposals[i]->displayString());
		if(!s.empty()) {
			const int index = static_cast<int>(::SendMessageW(handle().get(), LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(s.c_str())));
			if(index != LB_ERR && index != LB_ERRSPACE) {
				newProposals.push_back(proposals[i]);
				::SendMessageW(handle().get(), LB_SETITEMDATA, index, reinterpret_cast<LPARAM>(&newProposals.back()));
			}
		}
	}

	swap(proposals_, newProposals);
}

shared_ptr<const CompletionProposal> DefaultContentAssistant::CompletionProposalsPopup::selectedProposal() const {
	const int sel = static_cast<int>(::SendMessageW(handle().get(), LB_GETCURSEL, 0, 0));
	if(sel != LB_ERR) {
		if(const LRESULT temp = ::SendMessageW(handle().get(), LB_GETITEMDATA, sel, 0))
			return *reinterpret_cast<const shared_ptr<const CompletionProposal>*>(temp);
	}
	return shared_ptr<const CompletionProposal>();
}

void DefaultContentAssistant::CompletionProposalsPopup::selectProposal(shared_ptr<const CompletionProposal> selection) {
	::SendMessageW(handle().get(), LB_SETCURSEL, -1, 0);
	if(selection.get() != nullptr) {
		for(int i = 0, c = static_cast<int>(::SendMessageW(handle().get(), LB_GETCOUNT, 0, 0)); i < c; ++i) {
			if(const LRESULT temp = ::SendMessageW(handle().get(), LB_GETITEMDATA, i, 0)) {
				if(reinterpret_cast<const shared_ptr<const CompletionProposal>*>(temp)->get() == selection.get()) {
					::SendMessageW(handle().get(), LB_SETCURSEL, i, 0);
					return;
				}
			}
		}
	}
}

/**
 * Sets the new font.
 * @param font The font to be set. If set to @c null, default font will be selected
 */
void DefaultContentAssistant::CompletionProposalsPopup::setFont(const HFONT font) {
	::SendMessageW(handle().get(), WM_SETFONT, reinterpret_cast<WPARAM>((font != nullptr) ? font : defaultFont_), 0);
}

void DefaultContentAssistant::CompletionProposalsPopup::setWritingMode(const presentation::WritingMode& writingMode) {
	LONG_PTR style = ::GetWindowLongPtrW(handle().get(), GWL_EXSTYLE);
	if(writingMode.inlineFlowDirection == presentation::LEFT_TO_RIGHT) {
		style &= ~(WS_EX_LAYOUTRTL | WS_EX_RTLREADING);
		style |= WS_EX_LTRREADING;
	} else {
		style &= ~WS_EX_LTRREADING;
		style |= WS_EX_LAYOUTRTL | WS_EX_RTLREADING;
	}
	::SetWindowLongPtrW(handle().get(), GWL_EXSTYLE, style);
	// TODO: Change the orientation.
}

void DefaultContentAssistant::CompletionProposalsPopup::updateDefaultFont() {
	win32::AutoZeroSize<NONCLIENTMETRICSW> ncm;
	::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);
	HFONT newFont = ::CreateFontIndirectW(&ncm.lfStatusFont);
	if(defaultFont_ != nullptr && reinterpret_cast<HFONT>(::SendMessageW(handle().get(), WM_GETFONT, 0, 0)) == defaultFont_) {
		HFONT oldFont = reinterpret_cast<HFONT>(::SendMessageW(handle().get(), WM_GETFONT, 0, 0));
		::SendMessageW(handle().get(), WM_SETFONT, reinterpret_cast<WPARAM>(newFont), 0);
		::DeleteObject(oldFont);
	} else
		::SendMessageW(handle().get(), WM_SETFONT, reinterpret_cast<WPARAM>(newFont), 0);
	defaultFont_ = newFont;
}


// DefaultContentAssistant ////////////////////////////////////////////////////////////////////////

/// @see CompletionProposalsUI#nextPage
void DefaultContentAssistant::nextPage(int pages) {
	while(pages > 0) {
		::SendMessageW(proposalsPopup_->handle().get(), WM_KEYDOWN, VK_NEXT, 0);
		--pages;
	}
	while(pages < 0) {
		::SendMessageW(proposalsPopup_->handle().get(), WM_KEYDOWN, VK_PRIOR, 0);
		++pages;
	}
}

/// @see CompletionProposalsUI#nextProposal
void DefaultContentAssistant::nextProposal(int proposals) {
	while(proposals > 0) {
		::SendMessageW(proposalsPopup_->handle().get(), WM_KEYDOWN, VK_DOWN, 0);
		--proposals;
	}
	while(proposals < 0) {
		::SendMessageW(proposalsPopup_->handle().get(), WM_KEYDOWN, VK_UP, 0);
		++proposals;
	}
}

#endif // ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
