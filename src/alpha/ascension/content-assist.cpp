/**
 * @file content-assist.cpp
 * @author exeal
 * @date 2003-2006 (was CompletionWindow.cpp)
 * @date 2006-2007
 */

#include "content-assist.hpp"
#include "viewer.hpp"	// TextViewer
#include "../../manah/win32/ui/standard-controls.hpp"	// manah.windows.ui.ListBox
using namespace ascension;
using namespace ascension::contentassist;
using namespace ascension::kernel;
using namespace ascension::layout;
using namespace ascension::text;
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
	document.beginCompoundChange();
	document.erase(replacementRegion);
	document.insert(replacementRegion.beginning(), replacementString_);
	document.endCompoundChange();
}


// IdentifiersProposalProcessor /////////////////////////////////////////////

namespace {
	struct CompletionProposalDisplayStringComparer {
		bool operator()(const ICompletionProposal* lhs, const ICompletionProposal* rhs) {
			return CaseFolder::compare(lhs->getDisplayString(), rhs->getDisplayString()) < 0;
		}
		bool operator()(const ICompletionProposal* lhs, const String& rhs) {return CaseFolder::compare(lhs->getDisplayString(), rhs) < 0;}
		bool operator()(const String& lhs, const ICompletionProposal* rhs) {return CaseFolder::compare(lhs, rhs->getDisplayString()) < 0;}
	};
} // namespace @0

/**
 * Constructor.
 * @param contentType the content type
 * @param syntax the identifier syntax to detect identifiers
 */
IdentifiersProposalProcessor::IdentifiersProposalProcessor(ContentType contentType,
		const IdentifierSyntax& syntax) throw() : contentType_(contentType), syntax_(syntax) {
}

/// Destructor.
IdentifiersProposalProcessor::~IdentifiersProposalProcessor() throw() {
}

/// @see IContentAssistProcessor#computCompletionProposals
void IdentifiersProposalProcessor::computeCompletionProposals(const Caret& caret,
		bool& incremental, Region& replacementRegion, set<ICompletionProposal*>& proposals) const {
	replacementRegion.second = caret;

	// find the preceding identifier
	static const length_t MAXIMUM_IDENTIFIER_LENGTH = 100;
	if(!incremental || caret.isBeginningOfLine())
		replacementRegion.first = caret;
	else if(source::getNearestIdentifier(*caret.document(), caret, &replacementRegion.first.column, 0))
		replacementRegion.first.line = caret.lineNumber();
	else
		replacementRegion.first = caret;

	// collect identifiers in the document
	static const length_t MAXIMUM_BACKTRACKING_LINES = 500;
	const Document& document = *caret.document();
	DocumentCharacterIterator i(document, Region(Position(
		(caret.lineNumber() > MAXIMUM_BACKTRACKING_LINES) ?
			caret.lineNumber() - MAXIMUM_BACKTRACKING_LINES : 0, 0), replacementRegion.first));
	DocumentPartition currentPartition;
	set<String> identifiers;
	bool followingNIDs = false;
	document.partitioner().partition(i.tell(), currentPartition);
	while(i.hasNext()) {
		if(currentPartition.contentType != contentType_)
			i.seek(currentPartition.region.end());
		if(i.tell() >= currentPartition.region.end()) {
			if(i.tell().column == i.line().length())
				i.next();
			document.partitioner().partition(i.tell(), currentPartition);
			continue;
		}
		if(!followingNIDs) {
			const Char* const bol = i.line().data();
			const Char* const s = bol + i.tell().column;
			const Char* e = syntax_.eatIdentifier(s, bol + i.line().length());
			if(e > s) {
				identifiers.insert(String(s, e));	// automatically merged
				i.seek(Position(i.tell().line, e - bol));
			} else {
				if(syntax_.isIdentifierContinueCharacter(i.current()))
					followingNIDs = true;
				i.next();
			}
		} else {
			if(!syntax_.isIdentifierContinueCharacter(i.current()))
				followingNIDs = false;
			i.next();
		}
	}
	for(set<String>::const_iterator i(identifiers.begin()), e(identifiers.end()); i != e; ++i)
		proposals.insert(new CompletionProposal(*i));
}

/// @see IContentAssistProcessor#getActiveCompletionProposal
const ICompletionProposal* IdentifiersProposalProcessor::getActiveCompletionProposal(const TextViewer& textViewer,
		const Region& replacementRegion, ICompletionProposal* const currentProposals[], size_t numberOfCurrentProposals) const throw() {
	// select the partially matched proposal
	String precedingIdentifier(textViewer.document().line(replacementRegion.first.line).substr(
		replacementRegion.beginning().column, replacementRegion.end().column - replacementRegion.beginning().column));
	if(precedingIdentifier.empty())
		return 0;
	const ICompletionProposal* activeProposal = *lower_bound(currentProposals,
		currentProposals + numberOfCurrentProposals, precedingIdentifier, CompletionProposalDisplayStringComparer());
	if(activeProposal == currentProposals[numberOfCurrentProposals]
			|| CaseFolder::compare(activeProposal->getDisplayString().substr(0, precedingIdentifier.length()), precedingIdentifier) != 0)
		return 0;
	return activeProposal;
}

/// Returns the identifier syntax the processor uses or @c null.
const IdentifierSyntax& IdentifiersProposalProcessor::identifierSyntax() const throw() {
	return syntax_;
}

/// @see IContentAssistProcessor#isIncrementalCompletionAutoTerminationCharacter
bool IdentifiersProposalProcessor::isIncrementalCompletionAutoTerminationCharacter(CodePoint c) const throw() {
	return !syntax_.isIdentifierContinueCharacter(c);
}

/// @see IContentAssistProcessor#recomputIncrementalCompletionProposals
void IdentifiersProposalProcessor::recomputeIncrementalCompletionProposals(
		const TextViewer&, const Region&, ICompletionProposal* const[], size_t, set<ICompletionProposal*>&) const {
	// do nothing
}


// ContentAssistant.CompletionProposalPopup /////////////////////////////////

/// A completion window.
class ContentAssistant::CompletionProposalPopup : public manah::win32::ui::ListBox {
	MANAH_NONCOPYABLE_TAG(CompletionProposalPopup);
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
	void	onLButtonDblClk(UINT, const ::POINT&);
	void	onLButtonDown(UINT, const ::POINT&);
	void	onSetFocus(HWND);
	void	onSettingChange(UINT, const WCHAR*);

private:
	IContentAssistant::ICompletionProposalsUI& ui_;
	HFONT defaultFont_;
	MANAH_DECLEAR_WINDOW_MESSAGE_MAP(CompletionProposalPopup);
};

MANAH_BEGIN_WINDOW_MESSAGE_MAP(ContentAssistant::CompletionProposalPopup, manah::win32::ui::ListBox)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_DESTROY)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_LBUTTONDBLCLK)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_LBUTTONDOWN)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_SETFOCUS)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_SETTINGCHANGE)
MANAH_END_WINDOW_MESSAGE_MAP()

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
			WS_CHILD | WS_TABSTOP | WS_VSCROLL | LBS_HASSTRINGS | LBS_NOINTEGRALHEIGHT | LBS_NOTIFY,
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

/// @see WM_LBUTTONDOWN
void ContentAssistant::CompletionProposalPopup::onLButtonDown(UINT, const ::POINT& pt) {
	bool outside;
	const int index = itemFromPoint(pt, outside);
	setCurSel(outside ? -1 : index);
}

/// @see WM_SETFOCUS
void ContentAssistant::CompletionProposalPopup::onSetFocus(HWND) {
	getParent().setFocus();
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
	MANAH_AUTO_STRUCT_SIZE(::NONCLIENTMETRICSW, ncm);
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
 * @throw IllegalStateException the completion is not running
 */
bool ContentAssistant::CompletionProposalPopup::updateListCursel() {
	assertValidAsWindow();

	if(!running_)
		throw IllegalStateException("Completion is not running.");

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

namespace {
	void setupPopupContent(manah::win32::ui::ListBox& listbox, ICompletionProposal* const proposals[], size_t numberOfProposals) {
		listbox.resetContent();
		for(size_t i = 0; i < numberOfProposals; ++i) {
			// TODO: display icons.
			const String s(proposals[i]->getDisplayString());
			if(!s.empty()) {
				const int index = listbox.addString(s.c_str());
				if(index != LB_ERR && index != LB_ERRSPACE)
					listbox.setItemDataPtr(index, proposals[i]);
			}
		}
	}
	void selectProposal(manah::win32::ui::ListBox& listbox, const ICompletionProposal* selection) {
		listbox.setCurSel(-1);
		if(selection != 0) {
			for(int i = 0, c = listbox.getCount(); i < c; ++i) {
				if(listbox.getItemDataPtr(i) == selection) {
					listbox.setCurSel(i);
					return;
				}
			}
		}
	}
} // namespace @0

map<::UINT_PTR, ContentAssistant*> ContentAssistant::timerIDs_;

/// Constructor.
ContentAssistant::ContentAssistant() throw() : textViewer_(0), proposalPopup_(0), autoActivationDelay_(500) {
}

/// Destructor.
ContentAssistant::~ContentAssistant() throw() {
	for(map<ContentType, IContentAssistProcessor*>::iterator i(processors_.begin()), e(processors_.end()); i != e; ++i)
		delete i->second;
	delete proposalPopup_;
}

/// Returns the automatic activation delay in milliseconds.
ulong ContentAssistant::autoActivationDelay() const throw() {
	return autoActivationDelay_;
}

/// @see viewers#ICaretListener
void ContentAssistant::caretMoved(const Caret& self, const Region&) {
	if(completionSession_.get() != 0) {
		// non-incremental mode: close when the caret moved
		if(!completionSession_->incremental)
			close();
		// incremental mode: close if the caret gone out of the replacement region
		else if(self.position() < completionSession_->replacementRegion.beginning()
				|| self.position() > completionSession_->replacementRegion.end())
			close();
	}
}

/// @see viewers#ICharacterInputListener#characterInputted
void ContentAssistant::characterInputted(const Caret&, CodePoint c) {
	if(textViewer_ != 0) {
		if(completionSession_.get() != 0) {
			if(!completionSession_->incremental)
				close();
			else if(completionSession_->processor->isIncrementalCompletionAutoTerminationCharacter(c)) {
				textViewer_->document().beginCompoundChange();
				textViewer_->caret().erase(-1, EditPoint::UTF32_CODE_UNIT);
				textViewer_->document().endCompoundChange();
				complete();
			}
		} else {
			// activate automatically
			if(const IContentAssistProcessor* const cap = getContentAssistProcessor(textViewer_->caret().getContentType())) {
				if(cap->isCompletionProposalAutoActivationCharacter(c)) {
					if(autoActivationDelay_ == 0)
						showPossibleCompletions();
					else if(const ::UINT_PTR timerID = ::SetTimer(0, reinterpret_cast<::UINT_PTR>(this), autoActivationDelay_, timeElapsed))
						timerIDs_.insert(make_pair(timerID, this));
//					else
//						textViewer_->beep();
				}
			}
		}
	}
}

/// @see ICompletionProposalsUI#close
void ContentAssistant::close() {
	if(completionSession_.get() != 0) {
		textViewer_->removeViewportListener(*this);
		textViewer_->caret().removeListener(*this);
		if(completionSession_->incremental)
			textViewer_->document().removeListener(*this);
		completionSession_.reset();
		proposalPopup_->show(SW_HIDE);
		proposalPopup_->resetContent();
	}
}

/// @see ICompletionProposalsUI#complete
bool ContentAssistant::complete() {
	if(completionSession_.get() != 0) {
		const int sel = proposalPopup_->getCurSel();
		if(sel != LB_ERR) {
			if(ICompletionProposal* p = static_cast<ICompletionProposal*>(proposalPopup_->getItemDataPtr(sel))) {
				auto_ptr<CompletionSession> temp(completionSession_);	// force completionSession_ to null
				Document& document = textViewer_->document();
				if(!document.isReadOnly()) {
					document.beginCompoundChange();
					p->replace(document, temp->replacementRegion);
					document.endCompoundChange();
				}
				completionSession_ = temp;
				close();
				return true;
			}
		}
		close();
	}
	return false;
}

/// @see kernel#IDocumentListener#documentAboutToBeChanged
bool ContentAssistant::documentAboutToBeChanged(const Document&, const DocumentChange&) {
	return true;
}

/// @see kernel#IDocumentListener#documentChanged
void ContentAssistant::documentChanged(const Document&, const DocumentChange& change) {
	if(completionSession_.get() != 0) {
		// exit or update the replacement region
		if(!completionSession_->incremental || change.region().first.line != change.region().second.line)
			close();
		const Region& replacementRegion = completionSession_->replacementRegion;
		if(change.isDeletion() && !replacementRegion.encompasses(change.region()))
			close();
		completionSession_->replacementRegion.second = updatePosition(completionSession_->replacementRegion.second, change, FORWARD);
		if(!change.isDeletion() && !replacementRegion.encompasses(change.region()))
			close();

		// rebuild proposals
		set<ICompletionProposal*> newProposals;
		completionSession_->processor->recomputeIncrementalCompletionProposals(
			*textViewer_, completionSession_->replacementRegion,
			completionSession_->proposals.get(), completionSession_->numberOfProposals, newProposals);
		if(!newProposals.empty()) {
			for(size_t i = 0; i < completionSession_->numberOfProposals; ++i)
				delete completionSession_->proposals[i];
			completionSession_->proposals.reset();
			if(newProposals.size() == 1 && (*newProposals.begin())->isAutoInsertable()) {
				(*newProposals.begin())->replace(textViewer_->document(), completionSession_->replacementRegion);
				return close();
			}
			completionSession_->proposals.reset(new ICompletionProposal*[completionSession_->numberOfProposals = newProposals.size()]);
			copy(newProposals.begin(), newProposals.end(), completionSession_->proposals.get());
			sort(completionSession_->proposals.get(),
				completionSession_->proposals.get() + newProposals.size(), CompletionProposalDisplayStringComparer());
			setupPopupContent(*proposalPopup_, completionSession_->proposals.get(), completionSession_->numberOfProposals);
		}

		// select the most preferred
		selectProposal(*proposalPopup_, completionSession_->processor->getActiveCompletionProposal(
			*textViewer_, completionSession_->replacementRegion, completionSession_->proposals.get(), completionSession_->numberOfProposals));
	}
}

/// @see IContentAssistant#getCompletionProposalsUI
IContentAssistant::ICompletionProposalsUI* ContentAssistant::getCompletionProposalsUI() const throw() {
	return (completionSession_.get() != 0) ? const_cast<ContentAssistant*>(this) : 0;
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
	(textViewer_ = &viewer)->caret().addCharacterInputListener(*this);
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
	if(textViewer_ == 0 || completionSession_.get() != 0 || textViewer_->document().isReadOnly())
		return textViewer_->beep();
	const Caret& caret = textViewer_->caret();
	if(const IContentAssistProcessor* const cap = getContentAssistProcessor(caret.getContentType())) {
		set<ICompletionProposal*> proposals;
		completionSession_.reset(new CompletionSession);
		(completionSession_->processor = cap)->computeCompletionProposals(caret,
			completionSession_->incremental, completionSession_->replacementRegion, proposals);
		if(!proposals.empty()) {
			if(proposals.size() == 1 && (*proposals.begin())->isAutoInsertable()) {
				(*proposals.begin())->replace(textViewer_->document(), completionSession_->replacementRegion);
				completionSession_.reset();
				delete *proposals.begin();
			} else {
				assert(completionSession_->proposals.get() == 0);
				completionSession_->proposals.reset(new ICompletionProposal*[completionSession_->numberOfProposals = proposals.size()]);
				copy(proposals.begin(), proposals.end(), completionSession_->proposals.get());
				sort(completionSession_->proposals.get(),
					completionSession_->proposals.get() + completionSession_->numberOfProposals, CompletionProposalDisplayStringComparer());
				startPopup();
				selectProposal(*proposalPopup_, cap->getActiveCompletionProposal(
					*textViewer_, completionSession_->replacementRegion,
					completionSession_->proposals.get(), completionSession_->numberOfProposals));
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
	const bool rtl = textViewer_->configuration().orientation == RIGHT_TO_LEFT;
	proposalPopup_->modifyStyleEx(rtl ? 0: WS_EX_LAYOUTRTL, rtl ? WS_EX_LAYOUTRTL : 0);
	setupPopupContent(*proposalPopup_, completionSession_->proposals.get(), completionSession_->numberOfProposals);

	updatePopupPositions();
	textViewer_->addViewportListener(*this);
	textViewer_->caret().addListener(*this);
	if(completionSession_->incremental)
		textViewer_->document().addListener(*this);
}

void CALLBACK ContentAssistant::timeElapsed(HWND, UINT, ::UINT_PTR eventID, DWORD) {
	map<::UINT_PTR, ContentAssistant*>::iterator i(timerIDs_.find(eventID));
	if(i != timerIDs_.end()) {
		ContentAssistant& ca = *i->second;
		::KillTimer(0, eventID);
		timerIDs_.erase(i);
		ca.showPossibleCompletions();
	}
}

/// @see IContentAssistant#uninstall
void ContentAssistant::uninstall() {
	if(textViewer_ != 0) {
		textViewer_->caret().removeCharacterInputListener(*this);
		textViewer_ = 0;
	}
}

void ContentAssistant::updatePopupPositions() {
	if(proposalPopup_ != 0 && proposalPopup_->isWindow()) {
		::RECT viewerRect;
		textViewer_->getClientRect(viewerRect);
		int cx = (viewerRect.right - viewerRect.left) / 4;
		int cy = proposalPopup_->getItemHeight(0) * min(static_cast<int>(completionSession_->numberOfProposals), 10) + 6;
		const ::POINT pt = textViewer_->clientXYForCharacter(completionSession_->replacementRegion.beginning(), false, LineLayout::LEADING);
		const bool rtl = textViewer_->configuration().orientation == RIGHT_TO_LEFT;
		int x = !rtl ? (pt.x - 3) : (pt.x - cx - 1 + 3);
		if(x + cx > viewerRect.right) {
//			if()
		}
		int y = pt.y + textViewer_->textRenderer().lineHeight();
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
}

/// @see viewers#IViewportListener#viewportChanged
void ContentAssistant::viewportChanged(bool, bool) {
	updatePopupPositions();
}
