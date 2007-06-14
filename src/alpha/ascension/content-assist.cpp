/**
 * @file content-assist.cpp
 * @author exeal
 * @date 2003-2006 (was CompletionWindow.cpp)
 * @date 2006-2007
 */

#include "stdafx.h"
#include "content-assist.hpp"
#include "viewer.hpp"	// SourceViewer
#include "../../manah/win32/ui/standard-controls.hpp"	// manah.windows.ui.ListBox
using namespace ascension;
using namespace ascension::contentassist;
using namespace ascension::text;
using namespace ascension::unicode;
using namespace ascension::viewers;
using namespace std;
using manah::AutoBuffer;


// CompletionProposal ///////////////////////////////////////////////////////

/**
 * Constructor.
 * @param replacementString the actual string to be inserted into the document
 * @param description the description of the proposal
 * @param icon the icon to display for the proposal
 * @param autoInsertable set true to enable auto insertion for the proposal
 */
CompletionProposal::CompletionProposal(const String& replacementString,
		const String& description /* = L"" */, HICON icon /* = 0 */, bool autoInsertable /* = true */) :
		displayString_(replacementString), replacementString_(replacementString), icon_(icon),
		descriptionString_(description), autoInsertable_(autoInsertable) {
}

/**
 * Constructor.
 * @param replacementString the actual string to be inserted into the document
 * @param displayString the string to display for the proposal
 * @param description the description of the proposal
 * @param icon the icon to display for the proposal
 * @param autoInsertable set true to enable auto insertion for the proposal
 */
CompletionProposal::CompletionProposal(const String& replacementString,
		const String& displayString, const String& description /* = L"" */, HICON icon /* = 0 */, bool autoInsertable /* = true */) :
		displayString_(displayString), replacementString_(replacementString), icon_(icon),
		descriptionString_(description), autoInsertable_(autoInsertable) {
}

/// @see ICompletionProposal#getDescription
String CompletionProposal::getDescription() const throw() {
	return descriptionString_;
}

/// @see ICompletionProposal#getDisplayString
String CompletionProposal::getDisplayString() const throw() {
	return displayString_;
}

/// @see ICompletionProposal#getIcon
HICON CompletionProposal::getIcon() const throw() {
	return icon_;
}

/// @see ICompletionProposal#isAutoInsertable
bool CompletionProposal::isAutoInsertable() const throw() {
	return autoInsertable_;
}

/// @see ICompletionProposal#replace
void CompletionProposal::replace(Document& document, const Region& replacementRegion) {
	document.beginSequentialEdit();
	document.erase(replacementRegion);
	document.insert(replacementRegion.getTop(), replacementString_);
	document.endSequentialEdit();
}


// IdentifiersProposalProcessor /////////////////////////////////////////////

/**
 * Constructor.
 * @param syntax the identifier syntax to detect identifiers
 */
IdentifiersProposalProcessor::IdentifiersProposalProcessor(const IdentifierSyntax& syntax) throw() : syntax_(syntax) {
}

/// Destructor.
IdentifiersProposalProcessor::~IdentifiersProposalProcessor() throw() {
}

/// @see IContentAssistProcessor#computCompletionProposals
void IdentifiersProposalProcessor::computeCompletionProposals(
		const Caret& caret, bool& incremental, Region& replacementRegion, set<ICompletionProposal*>& proposals) const {
	// TODO: not implemented.
}

/// @see IContentAssistProcessor#recomputIncrementalCompletionProposals
void IdentifiersProposalProcessor::recomputeIncrementalCompletionProposals(
		const Region& replacementRegion, set<ICompletionProposal*>& proposals) const {
	// TODO: not implemented.
}


// ContentAssistant.CompletionProposalPopup /////////////////////////////////

/// A completion window.
class ContentAssistant::CompletionProposalPopup : public manah::win32::ui::ListBox {
public:
	// constructor
	CompletionProposalPopup(IContentAssistant::ICompletionProposalsUI& ui) throw();
	// construction
	bool	create(HWND parent);
	// attributes
	void			setFont(const HFONT font);
	// operations
	bool	start(const std::set<ICompletionProposal*>& proposals);
	bool	updateListCursel();

private:
	void	updateDefaultFont();
private:
	void	onDestroy();
	void	onKillFocus(HWND);
	void	onLButtonDblClk(UINT, const ::POINT&);
	void	onSettingChange(UINT, const WCHAR*);

private:
	IContentAssistant::ICompletionProposalsUI& ui_;
	HFONT defaultFont_;
	MANAH_DECLEAR_WINDOW_MESSAGE_MAP(CompletionProposalPopup);
};

MANAH_BEGIN_WINDOW_MESSAGE_MAP(ContentAssistant::CompletionProposalPopup, manah::win32::ui::ListBox)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_DESTROY)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_KILLFOCUS)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_LBUTTONDBLCLK)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_SETTINGCHANGE)
MANAH_END_WINDOW_MESSAGE_MAP()

namespace {
	const length_t COMPLETION_MAX_TRACKBACK_CCH = 100;
}

/**
 * Constructor.
 * @param ui the user interface
 */
ContentAssistant::CompletionProposalPopup::CompletionProposalPopup(IContentAssistant::ICompletionProposalsUI& ui) : ui_(ui), defaultFont_(0) {
}

/**
 * Creates the list window.
 * @param parent the parent window
 * @return succeeded or not
 */
bool ContentAssistant::CompletionProposalPopup::create(HWND parent) {
	using namespace manah::win32::ui;

	if(ListBox::create(parent, DefaultWindowRect(), 0, 0,
			WS_CHILD | WS_TABSTOP | WS_VSCROLL | LBS_HASSTRINGS | LBS_NOINTEGRALHEIGHT | LBS_NOTIFY | LBS_SORT,
			WS_EX_DLGMODALFRAME | WS_EX_NOPARENTNOTIFY | WS_EX_TOOLWINDOW)
			&& subclass()) {
		updateDefaultFont();
		setPosition(HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

#if 0
		// 影を付けてみたりする...
#ifndef CS_DROPSHADOW
		const ::ULONG_PTR CS_DROPSHADOW = 0x00020000UL;
#endif /* !CS_DROPSHADOW */
		const ::ULONG_PTR styleBits = ::GetClassLongPtrW(getHandle(), GCL_STYLE);
		::SetClassLongPtrW(getHandle(), GCL_STYLE, styleBits | CS_DROPSHADOW);
#endif

		return true;
	}
	return false;
}

/// @see WM_DESTROY
void ContentAssistant::CompletionProposalPopup::onDestroy() {
	::DeleteObject(defaultFont_);
}

/// @see WM_LBUTTONDBLCLK
void ContentAssistant::CompletionProposalPopup::onLButtonDblClk(UINT, const ::POINT&) {
	ui_.complete();
}

/// @see WM_KILLFOCUS
void ContentAssistant::CompletionProposalPopup::onKillFocus(HWND) {
	ui_.close();
}

/// @see WM_SETTINGCHANGE
void ContentAssistant::CompletionProposalPopup::onSettingChange(UINT, const WCHAR*) {
	updateDefaultFont();
}

/**
 * Sets the new font.
 * @param font the font to be set. if set to @c null, default font will be selected
 */
void ContentAssistant::CompletionProposalPopup::setFont(const HFONT font) {
	assertValidAsWindow();
	ListBox::setFont((font != 0) ? font : defaultFont_);
}

/// Updates the default font with system parameter.
void ContentAssistant::CompletionProposalPopup::updateDefaultFont() {
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

#if 0
/**
 * Updates the list's cursel based on the viewer's context.
 * @return true if only one candidate matched
 * @throw std#logic_error the completion is not running
 */
bool ContentAssistant::CompletionProposalPopup::updateListCursel() {
	assertValidAsWindow();

	if(!running_)
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
#endif

// ContentAssistant /////////////////////////////////////////////////////////

map<ContentAssistant*, ::UINT_PTR> ContentAssistant::timerIDs_;

/// Constructor.
ContentAssistant::ContentAssistant() throw() : textViewer_(0), proposalPopup_(0), autoActivationDelay_(500) {
}

/// Destructor.
ContentAssistant::~ContentAssistant() throw() {
	for(map<ContentType, IContentAssistProcessor*>::iterator i(processors_.begin()), e(processors_.end()); i != e; ++i)
		delete i->second;
	delete proposalPopup_;
}

/// @see IContentAssistant#addCompletionListener
void ContentAssistant::addCompletionListener(ICompletionListener& listener) {
	completionListeners_.add(listener);
}

/// @see viewers#ICaretListener
void ContentAssistant::caretMoved(const Caret&self, const Region& oldRegion) {
	// TODO: not implemented.
}

/// @see viewers#ICharacterInputListener#characterInputted
void ContentAssistant::characterInputted(const Caret& self, CodePoint c) {
	if(textViewer_ != 0) {
		if(proposalPopup_ != 0 && proposalPopup_->isVisible()) {
			// TODO: exit the session if the character does not consist an identifier.
		} else if(const IContentAssistProcessor* const p = getContentAssistProcessor(self.getContentType())) {
			// activate automatically
			if(p->isCompletionProposalAutoActivationCharacter(c)) {
				if(autoActivationDelay_ == 0)
					showPossibleCompletions();
				else if(const ::UINT_PTR timerID = ::SetTimer(0, reinterpret_cast<::UINT_PTR>(this), autoActivationDelay_, timeElapsed))
					timerIDs_.insert(make_pair(this, timerID));
				else
					textViewer_->beep();
			}
		}
	}
}

/// @see ICompletionProposalsUI#close
void ContentAssistant::close() {
	if(completionSession_.get() != 0) {
		completionSession_.reset();
		textViewer_->getDocument().removeListener(*this);
		proposalPopup_->show(SW_HIDE);
		proposalPopup_->resetContent();
		textViewer_->setFocus();
	}
}

/// @see ICompletionProposalsUI#complete
bool ContentAssistant::complete() {
	if(completionSession_.get() != 0) {
		const int sel = proposalPopup_->getCurSel();
		if(sel != LB_ERR) {
			if(ICompletionProposal* p = static_cast<ICompletionProposal*>(proposalPopup_->getItemDataPtr(sel))) {
				auto_ptr<CompletionSession> temp(completionSession_);	// force completionSession_ to null
				Document& document = textViewer_->getDocument();
				document.beginSequentialEdit();
				p->replace(document, temp->replacementRegion);
				document.endSequentialEdit();
				completionSession_ = temp;
				close();
				return true;
			}
		}
		close();
	}
	return false;
}

/// @see text#IDocumentListener#documentAboutToBeChanged
void ContentAssistant::documentAboutToBeChanged(const Document&) {
}

/// @see text#IDocumentListener#documentChanged
void ContentAssistant::documentChanged(const Document& document, const DocumentChange& change) {
	if(completionSession_.get() != 0) {
		if(!completionSession_->incremental || change.getRegion().first.line != change.getRegion().second.line)
			close();
		// TODO: not implemented.
	}
}

/// Returns the automatic activation delay in milliseconds.
ulong ContentAssistant::getAutoActivationDelay() const throw() {
	return autoActivationDelay_;
}

/// @see IContentAssistant#getCompletionProposalsUI
IContentAssistant::ICompletionProposalsUI* ContentAssistant::getCompletionProposalsUI() const throw() {
	return const_cast<ContentAssistant*>(this);
}

/// @see IContentAssistant#getContentAssistProcessor
const IContentAssistProcessor* ContentAssistant::getContentAssistProcessor(ContentType contentType) const throw() {
	map<ContentType, IContentAssistProcessor*>::const_iterator i(processors_.find(contentType));
	return (i != processors_.end()) ? i->second : 0;
}

/// @see ICompletionProposalsUI#hasSelection
bool ContentAssistant::hasSelection() const throw() {
	return completionSession_.get() != 0 && proposalPopup_ != 0 && proposalPopup_->getCurSel() != LB_ERR;
}

/// @see IContentAssistant#install
void ContentAssistant::install(TextViewer& viewer) {
	(textViewer_ = &viewer)->getCaret().addCharacterInputListener(*this);
}

/// @see ICompletionProposalsUI#nextPage
void ContentAssistant::nextPage(int pages) {
	while(pages > 0) {
		proposalPopup_->sendMessage(WM_KEYDOWN, VK_NEXT);
		--pages;
	}
	while(pages < 0) {
		proposalPopup_->sendMessage(WM_KEYDOWN, VK_PRIOR);
		++pages;
	}
}

/// @see ICompletionProposalsUI#nextProposal
void ContentAssistant::nextProposal(int proposals) {
	while(proposals > 0) {
		proposalPopup_->sendMessage(WM_KEYDOWN, VK_DOWN);
		--proposals;
	}
	while(proposals < 0) {
		proposalPopup_->sendMessage(WM_KEYDOWN, VK_UP);
		++proposals;
	}
}

/// @see IContentAssistant#removeCompletionListener
void ContentAssistant::removeCompletionListener(ICompletionListener& listener) {
	completionListeners_.remove(listener);
}

/**
 * Sets the delay between a character input and the session activation.
 * @param milliseconds the delay amount as milliseconds. if set to zero, the proposals will popup
 * immediately
 */
void ContentAssistant::setAutoActivationDelay(ulong milliseconds) {
	autoActivationDelay_ = milliseconds;
}

/**
 * Registers the given content assist processor for the specified content type. If there is already
 * a processor registered for the content type, the old processor is unregistered.
 * @param contentType the content type
 * @param processor the new content assist processor to register or @c null to unregister
 */
void ContentAssistant::setContentAssistProcessor(ContentType contentType, auto_ptr<IContentAssistProcessor> processor) {
	map<ContentType, IContentAssistProcessor*>::iterator i(processors_.find(contentType));
	if(i != processors_.end()) {
		delete i->second;
		processors_.erase(i);
	}
	if(processor.get() != 0)
		processors_.insert(make_pair(contentType, processor.release()));
}

/// @see IContentAssistant#showPossibleCompletions
void ContentAssistant::showPossibleCompletions() {
	if(textViewer_ == 0 || completionSession_.get() != 0)
		return;
	const Caret& caret = textViewer_->getCaret();
	if(const IContentAssistProcessor* const processor = getContentAssistProcessor(caret.getContentType())) {
		completionSession_.reset(new CompletionSession);
		processor->computeCompletionProposals(caret,
			completionSession_->incremental, completionSession_->replacementRegion, completionSession_->proposals);
		if(!completionSession_->proposals.empty()) {
			if(completionSession_->proposals.size() == 1 && (*completionSession_->proposals.begin())->isAutoInsertable()) {
				(*completionSession_->proposals.begin())->replace(textViewer_->getDocument(), completionSession_->replacementRegion);
				completionSession_.reset();
			} else {
				startPopup();
				completionListeners_.notify(ICompletionListener::completionSessionStarted);
			}
			return;	// succeeded
		}
		completionSession_.reset();	// can't start
	}
	textViewer_->beep();
}

/// Resets the contents of the completion proposal popup based on @c completionSession-&gt;proposals.
void ContentAssistant::startPopup() {
	if(proposalPopup_ == 0) {
		proposalPopup_ = new CompletionProposalPopup(*this);
		proposalPopup_->create(textViewer_->getHandle());
	} else
		proposalPopup_->resetContent();

	// determine the horizontal orientation of the window
	const bool rtl = textViewer_->getConfiguration().orientation == RIGHT_TO_LEFT;
	proposalPopup_->modifyStyleEx(rtl ? 0: WS_EX_LAYOUTRTL, rtl ? WS_EX_LAYOUTRTL : 0);
	for(set<ICompletionProposal*>::const_iterator
			p(completionSession_->proposals.begin()), e(completionSession_->proposals.end()); p!= e; ++p) {
		// TODO: display icons.
		const String s((*p)->getDisplayString());
		if(!s.empty()) {
			const int index = proposalPopup_->addString(s.c_str());
			if(index != LB_ERR && index != LB_ERRSPACE)
				proposalPopup_->setItemDataPtr(index, *p);
		}
	}

	// placement
	Caret& caret = textViewer_->getCaret();
	int cx = 200, cy = proposalPopup_->getItemHeight(0) * min(static_cast<int>(completionSession_->proposals.size()), 10);
	::RECT viewerRect;
	textViewer_->getClientRect(viewerRect);
	const ::POINT pt = textViewer_->getClientXYForCharacter(caret, false, LineLayout::LEADING);
	int x = !rtl ? pt.x : (pt.x - cx - 1);
	if(x + cx > viewerRect.right) {
//		if()
	}
	int y = pt.y + textViewer_->getTextRenderer().getLineHeight();
	if(y + cy > viewerRect.bottom) {
		if(pt.y - 1 - viewerRect.top < viewerRect.bottom - y)
			cy = viewerRect.bottom - y;
		else {
			cy = min<int>(cy, pt.y - viewerRect.top);
			y = pt.y - cy - 1;
		}
	}
	proposalPopup_->setPosition(0, x, y, cx, cy, SWP_NOZORDER | SWP_SHOWWINDOW);
}

void CALLBACK ContentAssistant::timeElapsed(HWND, UINT, ::UINT_PTR eventID, DWORD) {
	if(ContentAssistant* ca = reinterpret_cast<ContentAssistant*>(eventID)) {
		map<ContentAssistant*, ::UINT_PTR>::iterator i(timerIDs_.find(ca));
		if(i != timerIDs_.end()) {
			::KillTimer(0, i->second);
			timerIDs_.erase(i);
			ca->showPossibleCompletions();
		}
	}
}

/// @see IContentAssistant#uninstall
void ContentAssistant::uninstall() {
	if(textViewer_ != 0) {
		textViewer_->getCaret().removeCharacterInputListener(*this);
		textViewer_ = 0;
	}
}
