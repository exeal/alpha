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
 * @param replacementRegion the region to be replaced
 * @param icon the icon to display for the proposal
 * @param autoInsertable set true to enable auto insertion for the proposal
 */
CompletionProposal::CompletionProposal(const String& replacementString,
		const Region& replacementRegion, HICON icon /* = 0 */, bool autoInsertable /* = true */) :
		displayString_(replacementString), replacementString_(replacementString), icon_(icon),
		replacementRegion_(replacementRegion), autoInsertable_(autoInsertable) {
}

/**
 * Constructor.
 * @param replacementString the actual string to be inserted into the document
 * @param replacementRegion the region to be replaced
 * @param displayString the string to display for the proposal
 * @param icon the icon to display for the proposal
 * @param autoInsertable set true to enable auto insertion for the proposal
 */
CompletionProposal::CompletionProposal(const String& replacementString,
		const Region& replacementRegion, const String& displayString, HICON icon /* = 0 */, bool autoInsertable /* = true */) :
		displayString_(displayString), replacementString_(replacementString), icon_(icon),
		replacementRegion_(replacementRegion), autoInsertable_(autoInsertable) {
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
void CompletionProposal::replace(Document& document) {
	document.beginSequentialEdit();
	document.erase(replacementRegion_);
	document.insert(replacementRegion_.getTop(), replacementString_);
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
		const TextViewer& viewer, const Position& position, set<ICompletionProposal*>& proposals) const {
	// TODO: not implemented.
}


// ContentAssistant.CompletionProposalPopup /////////////////////////////////

/// A completion window.
class ContentAssistant::CompletionProposalPopup :
	public manah::win32::ui::ListBox, virtual public IContentAssistant::ICompletionProposalsUI {
public:
	// constructors
	explicit CompletionProposalPopup(viewers::TextViewer& viewer);
	virtual ~CompletionProposalPopup();
	// construction
	bool	create();
	// attributes
	text::Region	getContextRegion() const;
	void			setFont(const HFONT font);
	// operations
	bool	start(const std::set<ICompletionProposal*>& proposals);
	bool	updateListCursel();

private:
	void	disposeProposals();
	void	updateDefaultFont();
	// IContentAssistant.ICompletionProposalsUI
	void	close();
	bool	complete();
	void	nextPage(int pages);
	void	nextProposal(int proposals);
private:
	void	onDestroy();
	void	onKeyDown(UINT vkey, UINT, bool& handled);
	void	onKillFocus(HWND);
	void	onLButtonDblClk(UINT, const ::POINT&);
	void	onSettingChange(UINT, const WCHAR*);
	void	onShowWindow(bool, UINT);

private:
	TextViewer& viewer_;
	set<ICompletionProposal*> proposals_;
	HFONT defaultFont_;
	bool running_;
	Position contextStart_;		// 補完開始位置の前方の単語先頭
	VisualPoint* contextEnd_;	// 補完開始位置の後方の単語終端
	MANAH_DECLEAR_WINDOW_MESSAGE_MAP(CompletionProposalPopup);
};

MANAH_BEGIN_WINDOW_MESSAGE_MAP(ContentAssistant::CompletionProposalPopup, manah::win32::ui::ListBox)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_DESTROY)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_KEYDOWN)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_KILLFOCUS)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_LBUTTONDBLCLK)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_SETTINGCHANGE)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_SHOWWINDOW)
MANAH_END_WINDOW_MESSAGE_MAP()

namespace {
	const length_t COMPLETION_MAX_TRACKBACK_CCH = 100;
}

/**
 * Constructor.
 * @param viewer the target text viewer
 */
ContentAssistant::CompletionProposalPopup::CompletionProposalPopup(TextViewer& viewer) :
		viewer_(viewer), defaultFont_(0), running_(false), contextEnd_(new VisualPoint(viewer)) {
}

/// Destructor.
ContentAssistant::CompletionProposalPopup::~CompletionProposalPopup() {
	disposeProposals();
	delete contextEnd_;
}

/// @see ICompletionProposalsUI#close
void ContentAssistant::CompletionProposalPopup::close() {
	if(running_) {
		running_ = false;
		contextEnd_->adaptToDocument(false);
		show(SW_HIDE);
		viewer_.setFocus();
	}
}

/// @see ICompletionProposalsUI#complete
bool ContentAssistant::CompletionProposalPopup::complete() {
	const int sel = getCurSel();
	if(sel != LB_ERR) {
		if(ICompletionProposal* p = static_cast<ICompletionProposal*>(getItemDataPtr(sel))) {
			Document& document = viewer_.getDocument();
			document.beginSequentialEdit();
			p->replace(document);
			document.endSequentialEdit();
			close();
			return true;
		}
	}
	close();
	return false;
}

/**
 * Creates the list window.
 * @return succeeded or not
 */
bool ContentAssistant::CompletionProposalPopup::create() {
	using namespace manah::win32::ui;

	if(ListBox::create(viewer_.getHandle(), DefaultWindowRect(), 0, 0,
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

inline void ContentAssistant::CompletionProposalPopup::disposeProposals() {
	for(set<ICompletionProposal*>::iterator i(proposals_.begin()), e(proposals_.end()); i != e; ++i)
		delete *i;
}

/**
 * Returns the region for the completion. if the caret goes outside of this, the completion will be aborted
 * @throw std#logic_error the completion is not running
 */
inline Region ContentAssistant::CompletionProposalPopup::getContextRegion() const {
	if(!running_)
		throw logic_error("Completion is not running.");
	return Region(contextStart_, contextEnd_->getPosition());
}

/// @see ICompletionProposalsUI#nextPage
void ContentAssistant::CompletionProposalPopup::nextPage(int pages) {
	while(pages > 0) {sendMessage(WM_KEYDOWN, VK_NEXT, 0UL); --pages;}
	while(pages < 0) {sendMessage(WM_KEYDOWN, VK_PRIOR, 0UL); ++pages;}
}

/// @see ICompletionProposalsUI#nextProposal
void ContentAssistant::CompletionProposalPopup::nextProposal(int proposals) {
	while(proposals > 0) {sendMessage(WM_KEYDOWN, VK_DOWN, 0UL); --proposals;}
	while(proposals < 0) {sendMessage(WM_KEYDOWN, VK_UP, 0UL); ++proposals;}
}

/// @see WM_DESTROY
void ContentAssistant::CompletionProposalPopup::onDestroy() {
	::DeleteObject(defaultFont_);
}

/// @see WM_LBUTTONDBLCLK
void ContentAssistant::CompletionProposalPopup::onLButtonDblClk(UINT, const ::POINT&) {
	complete();
}

/// @see WM_KEYDOWN
void ContentAssistant::CompletionProposalPopup::onKeyDown(UINT vkey, UINT, bool& handled) {
	if(vkey == VK_RETURN && getCurSel() != LB_ERR) {
		complete();
		handled = true;
	} else if(vkey == VK_ESCAPE) {
		close();
		handled = true;
	}
}

/// @see WM_KILLFOCUS
void ContentAssistant::CompletionProposalPopup::onKillFocus(HWND) {
	close();
}

/// @see WM_SETTINGCHANGE
void ContentAssistant::CompletionProposalPopup::onSettingChange(UINT, const WCHAR*) {
	updateDefaultFont();
}

/// @see WM_SHOWWINDOW
void ContentAssistant::CompletionProposalPopup::onShowWindow(bool showing, UINT) {
	if(!showing)
		resetContent();
}

/**
 * Sets the new font.
 * @param font the font to be set. if set to @c null, default font will be selected
 */
void ContentAssistant::CompletionProposalPopup::setFont(const HFONT font) {
	assertValidAsWindow();
	ListBox::setFont((font != 0) ? font : defaultFont_);
}

/**
 * Starts the completion session and shows the popup.
 * @param proposals the completion proposals
 * @return succeeded or not
 */
bool ContentAssistant::CompletionProposalPopup::start(const set<ICompletionProposal*>& proposals) {
	assertValidAsWindow();

	Caret& caret = viewer_.getCaret();
	const unicode::IdentifierSyntax& syntax = viewer_.getDocument().getContentTypeInformation().getIdentifierSyntax(caret.getContentType());

	resetContent();
	const bool rtl = viewer_.getConfiguration().orientation == RIGHT_TO_LEFT;
	modifyStyleEx(rtl ? 0: WS_EX_LAYOUTRTL, rtl ? WS_EX_LAYOUTRTL : 0);
	for(set<ICompletionProposal*>::const_iterator proposal(proposals.begin()), e(proposals.end()); proposal != e; ++proposal) {
		// TODO: display icons.
		const String s((*proposal)->getDisplayString());
		if(!s.empty()) {
			const int index = addString(s.c_str());
			if(index != LB_ERR && index != LB_ERRSPACE)
				setItemDataPtr(index, *proposal);
		}
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

	int cx = 200, cy = getItemHeight(0) * min(static_cast<int>(proposals.size()), 10);
	::RECT viewerRect;
	viewer_.getClientRect(viewerRect);
	const ::POINT pt = viewer_.getClientXYForCharacter(caret, false, LineLayout::LEADING);
	int x = !rtl ? pt.x : (pt.x - cx - 1);
	if(x + cx > viewerRect.right) {
		if()
	}
	int y = pt.y + viewer_.getTextRenderer().getLineHeight();
	if(y + cy > viewerRect.bottom) {
		if(pt.y - 1 - viewerRect.top < viewerRect.bottom - y)
			cy = viewerRect.bottom - y;
		else {
			cy = min<int>(cy, pt.y - viewerRect.top);
			y = pt.y - cy - 1;
		}
	}
	setPosition(0, x, y, cx, cy, SWP_NOZORDER | SWP_SHOWWINDOW);
	return true;
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

/// @see viewers#ICharacterInputListener#characterInputted
void ContentAssistant::characterInputted(const Caret& self, CodePoint c) {
	if(textViewer_ != 0) {
		if(proposalPopup_ != 0 && proposalPopup_->isVisible()) {
			// TODO: exit the session if the character does not consist an identifier.
		} else if(const IContentAssistProcessor* const p = getContentAssistProcessor(self.getContentType())) {
			// activate automatically
			const String s(p->getCompletionProposalAutoActivationCharacters());
			bool activate;
			if(c < 0x10000)
				activate = s.find(static_cast<Char>(c & 0xFFFF)) != String::npos;
			else {
				const StringCharacterIterator e(s, s.end());
				activate = find(StringCharacterIterator(s), e, c) != e;
			}
			if(activate) {
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

/// Returns the automatic activation delay in milliseconds.
ulong ContentAssistant::getAutoActivationDelay() const throw() {
	return autoActivationDelay_;
}

/// @see IContentAssistant#getCompletionProposalsUI
IContentAssistant::ICompletionProposalsUI* ContentAssistant::getCompletionProposalsUI() const throw() {
	return (proposalPopup_ != 0 && proposalPopup_->isVisible()) ? proposalPopup_ : 0;
}

/// @see IContentAssistant#getContentAssistProcessor
const IContentAssistProcessor* ContentAssistant::getContentAssistProcessor(ContentType contentType) const throw() {
	map<ContentType, IContentAssistProcessor*>::const_iterator i(processors_.find(contentType));
	return (i != processors_.end()) ? i->second : 0;
}

/// @see IContentAssistant#install
void ContentAssistant::install(TextViewer& viewer) {
	(textViewer_ = &viewer)->getCaret().addCharacterInputListener(*this);
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
	if(textViewer_ == 0)
		return;
	const Caret& caret = textViewer_->getCaret();
	if(caret.isSelectionEmpty()) {
		if(const IContentAssistProcessor* const processor = getContentAssistProcessor(caret.getContentType())) {
			set<ICompletionProposal*> proposals;
			processor->computeCompletionProposals(*textViewer_, caret.getPosition(), proposals);
			if(!proposals.empty()) {
				if(proposals.size() == 1 && (*proposals.begin())->isAutoInsertable()) {
					(*proposals.begin())->replace(textViewer_->getDocument());
					delete *proposals.begin();
				} else {
					if(proposalPopup_ == 0) {
						proposalPopup_ = new CompletionProposalPopup(*textViewer_);
						proposalPopup_->create();
					}
					proposalPopup_->start(proposals);
					completionListeners_.notify(ICompletionListener::completionSessionStarted);
				}
				return;	// succeeded
			}
		}
	}
	textViewer_->beep();
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
