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


// ContentAssistant.CompletionProposalPopup /////////////////////////////////

/// A completion window.
class ContentAssistant::CompletionProposalPopup : public manah::win32::ui::ListBox {
public:
	// constructors
	explicit CompletionProposalPopup(viewers::SourceViewer& viewer);
	virtual ~CompletionProposalPopup();
	// construction
	bool	create();
	// attributes
	text::Region	getContextRegion() const;
	bool			isRunning() const throw();
	void			setFont(const HFONT font);
	// operations
	void	abort();
	void	complete();
	bool	start(const std::set<ICompletionProposal*>& proposals);
	bool	updateListCursel();

protected:
	virtual LRESULT	preTranslateWindowMessage(UINT message, WPARAM wParam, LPARAM lParam, bool& handled);
private:
	void	disposeProposals();
	void	updateDefaultFont();

private:
	SourceViewer& viewer_;
	set<ICompletionProposal*> proposals_;
	HFONT defaultFont_;
	bool running_;
	Position contextStart_;		// 補完開始位置の前方の単語先頭
	VisualPoint* contextEnd_;	// 補完開始位置の後方の単語終端
};

namespace {
	const length_t COMPLETION_MAX_TRACKBACK_CCH = 100;
}

/**
 * Constructor.
 * @param viewer the target source viewer
 */
ContentAssistant::CompletionProposalPopup::CompletionProposalPopup(SourceViewer& viewer) :
		viewer_(viewer), defaultFont_(0), running_(false), contextEnd_(new VisualPoint(viewer)) {
}

/// Destructor.
ContentAssistant::CompletionProposalPopup::~CompletionProposalPopup() {
	disposeProposals();
	delete contextEnd_;
}

/// Aborts the running completion.
void ContentAssistant::CompletionProposalPopup::abort() {
	if(isRunning()) {
		running_ = false;
		contextEnd_->adaptToDocument(false);
		show(SW_HIDE);
	}
}

/// Completes (or aborts if no candidates completely match).
void ContentAssistant::CompletionProposalPopup::complete() {
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
		document.erase(Position(caret.getLineNumber(), caret.getColumnNumber() - precedingID.length()), caret);
		caret.moveTo(document.insert(caret, text.get(), text.get() + len));
		document.endSequentialEdit();
		viewer_.unfreeze(true);
	}
	abort();
}

/**
 * Creates the list window.
 * @return succeeded or not
 */
bool ContentAssistant::CompletionProposalPopup::create() {
	using namespace manah::win32::ui;

	if(ListBox::create(viewer_.getHandle(), DefaultWindowRect(), 0, 0,
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

inline void ContentAssistant::CompletionProposalPopup::disposeProposals() {
	for(set<ICompletionProposal*>::iterator i(proposals_.begin()), e(proposals_.end()); i != e; ++i)
		delete *i;
}

/**
 * Returns the region for the completion. if the caret goes outside of this, the completion will be aborted
 * @throw std#logic_error the completion is not running
 */
inline Region ContentAssistant::CompletionProposalPopup::getContextRegion() const {
	if(!isRunning())
		throw logic_error("Completion is not running.");
	return Region(contextStart_, contextEnd_->getPosition());
}

/// @see ListBox#preTranslateWindowMessage
LRESULT ContentAssistant::CompletionProposalPopup::preTranslateWindowMessage(UINT message, WPARAM wParam, LPARAM lParam, bool& handled) {
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
void ContentAssistant::CompletionProposalPopup::setFont(const HFONT font) {
	assertValidAsWindow();
	ListBox::setFont((font != 0) ? font : defaultFont_);
}

/**
 * Starts the completion. This method does not display and/or places the window.
 * @param proposals the completion proposals
 * @return succeeded or not
 */
bool ContentAssistant::CompletionProposalPopup::start(const set<ICompletionProposal*>& proposals) {
	assertValidAsWindow();

	Caret& caret = viewer_.getCaret();
	const unicode::IdentifierSyntax& syntax = viewer_.getDocument().getContentTypeInformation().getIdentifierSyntax(caret.getContentType());

//	const bool rightToLeft = toBoolean(view_.getStyleEx() & WS_EX_RTLREADING);
//	const bool rightToLeft = view_.isTextDirectionRightToLeft();

	resetContent();
//	modifyStyleEx(rightToLeft ? 0: WS_EX_LAYOUTRTL, rightToLeft ? WS_EX_LAYOUTRTL : 0);
	for(set<ICompletionProposal*>::const_iterator i(proposals.begin()), e(proposals.end()); i != e; ++i) {
		// TODO: display icons.
		const String s((*i)->getDisplayString());
		if(!s.empty())
			addString(s.c_str());
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


// ContentAssistant /////////////////////////////////////////////////////////

/// Constructor.
ContentAssistant::ContentAssistant() throw() : sourceViewer_(0), proposalPopup_(0) {
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

/// @see IContentAssistant#getContentAssistProcessor
const IContentAssistProcessor* ContentAssistant::getContentAssistProcessor(ContentType contentType) const throw() {
	map<ContentType, IContentAssistProcessor*>::const_iterator i(processors_.find(contentType));
	return (i != processors_.end()) ? i->second : 0;
}

/// @see IContentAssistant#install
void ContentAssistant::install(SourceViewer& viewer) {
	sourceViewer_ = &viewer;
}

/// @see IContentAssistant#removeCompletionListener
void ContentAssistant::removeCompletionListener(ICompletionListener& listener) {
	completionListeners_.remove(listener);
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
	assert(sourceViewer_ != 0);
	const Caret& caret = sourceViewer_->getCaret();
	if(caret.isSelectionEmpty()) {
		if(const IContentAssistProcessor* const processor = getContentAssistProcessor(caret.getContentType())) {
			set<ICompletionProposal*> proposals;
			processor->computeCompletionProposals(*sourceViewer_, caret.getPosition(), proposals);
			if(!proposals.empty()) {
				if(proposals.size() == 1 && (*proposals.begin())->isAutoInsertable()) {
					(*proposals.begin())->replace(sourceViewer_->getDocument());
					delete *proposals.begin();
				} else {
					if(proposalPopup_ == 0) {
						proposalPopup_ = new CompletionProposalPopup(*sourceViewer_);
						proposalPopup_->create();
					}
					proposalPopup_->start(proposals);
					completionListeners_.notify(ICompletionListener::completionSessionStarted);
				}
				return;	// succeeded
			}
		}
	}
	sourceViewer_->beep();
}

/// @see IContentAssistant#uninstall
void ContentAssistant::uninstall() {
	sourceViewer_ = 0;
}
