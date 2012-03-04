/**
 * @file content-assist.cpp
 * @author exeal
 * @date 2003-2006 (was CompletionWindow.cpp)
 * @date 2006-2012
 */

#include <ascension/corelib/text/case-folder.hpp>
#include <ascension/kernel/document-character-iterator.hpp>
#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/content-assist.hpp>
#include <ascension/viewer/viewer.hpp>				// TextViewer
#if defined(ASCENSION_WINDOW_SYSTEM_GTK)
#	include <gtkmm/gtkmm.h>
#elif defined(ASCENSION_WINDOW_SYSTEM_QT)
#	include <QListWidget>
#elif defined(ASCENSION_WINDOW_SYSTEM_QUARTZ)
#elif defined(ASCENSION_WINDOW_SYSTEM_WIN32)
#	include <ascension/win32/windows.hpp>
#elif defined(ASCENSION_WINDOW_SYSTEM_X)
#endif

using namespace ascension;
using namespace ascension::contentassist;
using namespace ascension::kernel;
using namespace ascension::presentation;
using namespace ascension::text;
using namespace ascension::viewers;
using namespace std;


// DefaultCompletionProposal //////////////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param replacementString The actual string to be inserted into the document
 * @param description The description of the proposal
 * @param icon The icon to display for the proposal
 * @param autoInsertable Set @c true to enable auto insertion for the proposal
 */
DefaultCompletionProposal::DefaultCompletionProposal(
		const String& replacementString, const String& description /* = String() */,
		Icon&& icon /* = Icon() */, bool autoInsertable /* = true */) :
		displayString_(replacementString), replacementString_(replacementString), icon_(icon),
		descriptionString_(description), autoInsertable_(autoInsertable) {
}

/**
 * Constructor.
 * @param replacementString The actual string to be inserted into the document
 * @param displayString The string to display for the proposal
 * @param description The description of the proposal
 * @param icon The icon to display for the proposal
 * @param autoInsertable Set @c true to enable auto insertion for the proposal
 */
DefaultCompletionProposal::DefaultCompletionProposal(const String& replacementString,
		const String& displayString, const String& description /* = String() */,
		Icon&& icon /* = Icon() */, bool autoInsertable /* = true */) :
		displayString_(displayString), replacementString_(replacementString), icon_(icon),
		descriptionString_(description), autoInsertable_(autoInsertable) {
}

/// @see CompletionProposal#description
String DefaultCompletionProposal::description() const /*throw()*/ {
	return descriptionString_;
}

/// @see CompletionProposal#displayString
String DefaultCompletionProposal::displayString() const /*throw()*/ {
	return displayString_;
}

/// @see CompletionProposal#icon
CompletionProposal::Icon DefaultCompletionProposal::icon() const /*throw()*/ {
	return icon_;
}

/// @see CompletionProposal#isAutoInsertable
bool DefaultCompletionProposal::isAutoInsertable() const /*throw()*/ {
	return autoInsertable_;
}

/**
 * Implements @c CompletionProposal#replace.
 * This method may throw any exceptions @c kernel#Document#replace throws other than
 * @c kernel#ReadOnlyDocumentException.
 */
void DefaultCompletionProposal::replace(Document& document, const Region& replacementRegion) {
	if(!document.isReadOnly()) {
		document.insertUndoBoundary();
		document.replace(replacementRegion, replacementString_);
		document.insertUndoBoundary();
	}
}


// IdentifiersProposalProcessor ///////////////////////////////////////////////////////////////////

namespace {
	struct CompletionProposalDisplayStringComparer {
		bool operator()(const CompletionProposal* lhs, const CompletionProposal* rhs) const {
			return CaseFolder::compare(lhs->displayString(), rhs->displayString()) < 0;
		}
		bool operator()(const CompletionProposal* lhs, const String& rhs) const {
			return CaseFolder::compare(lhs->displayString(), rhs) < 0;
		}
		bool operator()(const String& lhs, const CompletionProposal* rhs) const {
			return CaseFolder::compare(lhs, rhs->displayString()) < 0;
		}
	};
} // namespace @0

/**
 * Constructor.
 * @param contentType The content type
 * @param syntax The identifier syntax to detect identifiers
 */
IdentifiersProposalProcessor::IdentifiersProposalProcessor(ContentType contentType,
		const IdentifierSyntax& syntax) /*throw()*/ : contentType_(contentType), syntax_(syntax) {
}

/// Destructor.
IdentifiersProposalProcessor::~IdentifiersProposalProcessor() /*throw()*/ {
}

/// @see ContentAssistProcessor#activeCompletionProposal
const CompletionProposal* IdentifiersProposalProcessor::activeCompletionProposal(
		const TextViewer& textViewer, const Region& replacementRegion,
		CompletionProposal* const currentProposals[], size_t numberOfCurrentProposals) const /*throw()*/ {
	// select the partially matched proposal
	String precedingIdentifier(textViewer.document().line(replacementRegion.first.line).substr(
		replacementRegion.beginning().offsetInLine, replacementRegion.end().offsetInLine - replacementRegion.beginning().offsetInLine));
	if(precedingIdentifier.empty())
		return 0;
	const CompletionProposal* activeProposal = *lower_bound(currentProposals,
		currentProposals + numberOfCurrentProposals, precedingIdentifier, CompletionProposalDisplayStringComparer());
	if(activeProposal == currentProposals[numberOfCurrentProposals]
			|| CaseFolder::compare(activeProposal->displayString().substr(0, precedingIdentifier.length()), precedingIdentifier) != 0)
		return 0;
	return activeProposal;
}

/// @see ContentAssistProcessor#computCompletionProposals
void IdentifiersProposalProcessor::computeCompletionProposals(const Caret& caret,
		bool& incremental, Region& replacementRegion, set<CompletionProposal*>& proposals) const {
	replacementRegion.second = caret;

	// find the preceding identifier
	static const Index MAXIMUM_IDENTIFIER_LENGTH = 100;
	if(!incremental || locations::isBeginningOfLine(caret))
		replacementRegion.first = caret;
	else if(source::getNearestIdentifier(caret.document(), caret, &replacementRegion.first.offsetInLine, 0))
		replacementRegion.first.line = line(caret);
	else
		replacementRegion.first = caret;

	// collect identifiers in the document
	static const Index MAXIMUM_BACKTRACKING_LINES = 500;
	const Document& document = caret.document();
	DocumentCharacterIterator i(document, Region(Position(
		(line(caret) > MAXIMUM_BACKTRACKING_LINES) ?
			line(caret) - MAXIMUM_BACKTRACKING_LINES : 0, 0), replacementRegion.first));
	DocumentPartition currentPartition;
	set<String> identifiers;
	bool followingNIDs = false;
	document.partitioner().partition(i.tell(), currentPartition);
	while(i.hasNext()) {
		if(currentPartition.contentType != contentType_)
			i.seek(currentPartition.region.end());
		if(i.tell() >= currentPartition.region.end()) {
			if(i.tell().offsetInLine == i.line().length())
				i.next();
			document.partitioner().partition(i.tell(), currentPartition);
			continue;
		}
		if(!followingNIDs) {
			const Char* const bol = i.line().data();
			const Char* const s = bol + i.tell().offsetInLine;
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
		proposals.insert(new DefaultCompletionProposal(*i));
}

/// Returns the identifier syntax the processor uses or @c null.
const IdentifierSyntax& IdentifiersProposalProcessor::identifierSyntax() const /*throw()*/ {
	return syntax_;
}

/// @see IContentAssistProcessor#isIncrementalCompletionAutoTerminationCharacter
bool IdentifiersProposalProcessor::isIncrementalCompletionAutoTerminationCharacter(CodePoint c) const /*throw()*/ {
	return !syntax_.isIdentifierContinueCharacter(c);
}

/// @see IContentAssistProcessor#recomputIncrementalCompletionProposals
void IdentifiersProposalProcessor::recomputeIncrementalCompletionProposals(
		const TextViewer&, const Region&, CompletionProposal* const[], size_t, set<CompletionProposal*>&) const {
	// do nothing
}


// DefaultContentAssistant.CompletionProposalPopup ////////////////////////////////////////////////

/// A completion window.
class DefaultContentAssistant::CompletionProposalPopup : public manah::win32::ui::ListBox {
	MANAH_NONCOPYABLE_TAG(CompletionProposalPopup);
public:
	// constructor
	CompletionProposalPopup(ContentAssistant::CompletionProposalsUI& ui) /*throw()*/;
	// construction
	bool create(HWND parent);
	// attributes
	void setFont(const HFONT font);
	// operations
	bool start(const std::set<CompletionProposal*>& proposals);
	bool updateListCursel();

private:
	void updateDefaultFont();
private:
	void onDestroy();
	void onLButtonDblClk(UINT, const POINT&, bool&);
	void onLButtonDown(UINT, const POINT&, bool&);
	void onSetFocus(HWND);
	void onSettingChange(UINT, const WCHAR*);

private:
	ContentAssistant::CompletionProposalsUI& ui_;
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
 * @param ui The user interface
 */
ContentAssistant::CompletionProposalPopup::CompletionProposalPopup(ContentAssistant::ICompletionProposalsUI& ui) : ui_(ui), defaultFont_(0) {
}

/**
 * Creates the list window.
 * @param parent The parent window
 * @return Succeeded or not
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
		const ULONG_PTR CS_DROPSHADOW = 0x00020000ul;
#endif // !CS_DROPSHADOW
		const ULONG_PTR styleBits = ::GetClassLongPtrW(getHandle(), GCL_STYLE);
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
void ContentAssistant::CompletionProposalPopup::onLButtonDblClk(UINT, const POINT&, bool&) {
	ui_.complete();
}

/// @see WM_LBUTTONDOWN
void ContentAssistant::CompletionProposalPopup::onLButtonDown(UINT, const POINT& pt, bool&) {
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
 * @param font The font to be set. If set to @c null, default font will be selected
 */
void ContentAssistant::CompletionProposalPopup::setFont(const HFONT font) {
	ListBox::setFont((font != 0) ? font : defaultFont_);
}

/// Updates the default font with system parameter.
void ContentAssistant::CompletionProposalPopup::updateDefaultFont() {
	manah::win32::AutoZeroSize<NONCLIENTMETRICSW> ncm;
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
 * @throw IllegalStateException The completion is not running
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

// DefaultContentAssistant ////////////////////////////////////////////////////////////////////////

namespace {
	void setupPopupContent(manah::win32::ui::ListBox& listbox, CompletionProposal* const proposals[], size_t numberOfProposals) {
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
	void selectProposal(manah::win32::ui::ListBox& listbox, const CompletionProposal* selection) {
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

/// Constructor.
DefaultContentAssistant::DefaultContentAssistant() /*throw()*/ : textViewer_(0), proposalPopup_(0), autoActivationDelay_(500) {
}

/// Destructor.
DefaultContentAssistant::~DefaultContentAssistant() /*throw()*/ {
	for(map<ContentType, ContentAssistProcessor*>::iterator i(processors_.begin()), e(processors_.end()); i != e; ++i)
		delete i->second;
	delete proposalPopup_;
}

/// Returns the automatic activation delay in milliseconds.
uint32_t DefaultContentAssistant::autoActivationDelay() const /*throw()*/ {
	return autoActivationDelay_;
}

/// @see viewers#CaretListener
void DefaultContentAssistant::caretMoved(const Caret& caret, const Region&) {
	if(completionSession_.get() != 0) {
		// non-incremental mode: close when the caret moved
		if(!completionSession_->incremental)
			close();
		// incremental mode: close if the caret gone out of the replacement region
		else if(caret.position() < completionSession_->replacementRegion.beginning()
				|| caret.position() > completionSession_->replacementRegion.end())
			close();
	}
}

/// @see viewers#CharacterInputListener#characterInput
void DefaultContentAssistant::characterInput(const Caret&, CodePoint c) {
	if(textViewer_ != 0) {
		if(completionSession_.get() != 0) {
			if(!completionSession_->incremental)
				close();
			else if(completionSession_->processor->isIncrementalCompletionAutoTerminationCharacter(c)) {
				Document& document = textViewer_->document();
				Caret& caret = textViewer_->caret();
				try {
					document.insertUndoBoundary();
					erase(document, locations::backwardCharacter(caret, locations::UTF32_CODE_UNIT), caret);
					document.insertUndoBoundary();
					complete();
				} catch(...) {
				}
			}
		} else {
			// activate automatically
			if(const ContentAssistProcessor* const cap = contentAssistProcessor(contentType(textViewer_->caret()))) {
				if(cap->isCompletionProposalAutoActivationCharacter(c)) {
					if(autoActivationDelay_ == 0)
						showPossibleCompletions();
					else
						timer_.start(autoActivationDelay_, *this);
				}
			}
		}
	}
}

/// @see CompletionProposalsUI#close
void DefaultContentAssistant::close() {
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

/// @see CompletionProposalsUI#complete
bool DefaultContentAssistant::complete() {
	if(completionSession_.get() != 0) {
		const int sel = proposalPopup_->getCurSel();
		if(sel != LB_ERR) {
			if(CompletionProposal* p = static_cast<CompletionProposal*>(proposalPopup_->getItemDataPtr(sel))) {
				unique_ptr<CompletionSession> temp(move(completionSession_));	// force completionSession_ to null
				Document& document = textViewer_->document();
				if(!document.isReadOnly()) {
					document.insertUndoBoundary();
					p->replace(document, temp->replacementRegion);
					document.insertUndoBoundary();
				}
				completionSession_ = move(temp);
				close();
				return true;
			}
		}
		close();
	}
	return false;
}

/// @see kernel#DocumentListener#documentAboutToBeChanged
void DefaultContentAssistant::documentAboutToBeChanged(const Document&) {
	// do nothing
}

/// @see kernel#DocumentListener#documentChanged
void DefaultContentAssistant::documentChanged(const Document&, const DocumentChange& change) {
	if(completionSession_.get() != 0) {
		// exit or update the replacement region
		if(!completionSession_->incremental
				|| change.erasedRegion().first.line != change.erasedRegion().second.line
				|| change.insertedRegion().first.line != change.insertedRegion().second.line)
			close();
		const Region& replacementRegion = completionSession_->replacementRegion;
		if(!change.erasedRegion().isEmpty() && !replacementRegion.encompasses(change.erasedRegion()))
			close();
		completionSession_->replacementRegion.second =
			positions::updatePosition(completionSession_->replacementRegion.second, change, Direction::FORWARD);
		if(!change.insertedRegion().isEmpty() && !replacementRegion.encompasses(change.insertedRegion()))
			close();

		// rebuild proposals
		set<CompletionProposal*> newProposals;
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
			completionSession_->proposals.reset(new CompletionProposal*[completionSession_->numberOfProposals = newProposals.size()]);
			copy(newProposals.begin(), newProposals.end(), completionSession_->proposals.get());
			sort(completionSession_->proposals.get(),
				completionSession_->proposals.get() + newProposals.size(), CompletionProposalDisplayStringComparer());
			setupPopupContent(*proposalPopup_, completionSession_->proposals.get(), completionSession_->numberOfProposals);
		}

		// select the most preferred
		selectProposal(*proposalPopup_, completionSession_->processor->activeCompletionProposal(
			*textViewer_, completionSession_->replacementRegion, completionSession_->proposals.get(), completionSession_->numberOfProposals));
	}
}

/// @see ContentAssistant#completionProposalsUI
ContentAssistant::CompletionProposalsUI* DefaultContentAssistant::completionProposalsUI() const /*throw()*/ {
	return (completionSession_.get() != 0) ? const_cast<DefaultContentAssistant*>(this) : 0;
}

/// @see ContentAssistant#contentAssistProcessor
const ContentAssistProcessor* DefaultContentAssistant::contentAssistProcessor(ContentType contentType) const /*throw()*/ {
	map<ContentType, ContentAssistProcessor*>::const_iterator i(processors_.find(contentType));
	return (i != processors_.end()) ? i->second : 0;
}

/// @see CompletionProposalsUI#hasSelection
bool DefaultContentAssistant::hasSelection() const /*throw()*/ {
	return completionSession_.get() != 0 && proposalPopup_ != 0 && proposalPopup_->getCurSel() != LB_ERR;
}

/// @see ContentAssistant#install
void DefaultContentAssistant::install(TextViewer& viewer) {
	(textViewer_ = &viewer)->caret().addCharacterInputListener(*this);
}

/// @see CompletionProposalsUI#nextPage
void DefaultContentAssistant::nextPage(int pages) {
	while(pages > 0) {
		proposalPopup_->sendMessage(WM_KEYDOWN, VK_NEXT);
		--pages;
	}
	while(pages < 0) {
		proposalPopup_->sendMessage(WM_KEYDOWN, VK_PRIOR);
		++pages;
	}
}

/// @see CompletionProposalsUI#nextProposal
void DefaultContentAssistant::nextProposal(int proposals) {
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
 * @param milliseconds The delay amount as milliseconds. if set to zero, the proposals will popup
 * immediately
 */
void DefaultContentAssistant::setAutoActivationDelay(uint32_t milliseconds) {
	autoActivationDelay_ = milliseconds;
}

/**
 * Registers the given content assist processor for the specified content type. If there is already
 * a processor registered for the content type, the old processor is unregistered.
 * @param contentType The content type
 * @param processor The new content assist processor to register or @c null to unregister
 */
void DefaultContentAssistant::setContentAssistProcessor(ContentType contentType, unique_ptr<ContentAssistProcessor> processor) {
	map<ContentType, ContentAssistProcessor*>::iterator i(processors_.find(contentType));
	if(i != processors_.end()) {
		delete i->second;
		processors_.erase(i);
	}
	if(processor.get() != 0)
		processors_.insert(make_pair(contentType, processor.release()));
}

/// @see ContentAssistant#showPossibleCompletions
void DefaultContentAssistant::showPossibleCompletions() {
	if(textViewer_ == 0 || completionSession_.get() != 0 || textViewer_->document().isReadOnly())
		return textViewer_->beep();
	const Caret& caret = textViewer_->caret();
	if(const ContentAssistProcessor* const cap = contentAssistProcessor(contentType(caret))) {
		set<CompletionProposal*> proposals;
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
				completionSession_->proposals.reset(new CompletionProposal*[completionSession_->numberOfProposals = proposals.size()]);
				copy(proposals.begin(), proposals.end(), completionSession_->proposals.get());
				sort(completionSession_->proposals.get(),
					completionSession_->proposals.get() + completionSession_->numberOfProposals, CompletionProposalDisplayStringComparer());
				startPopup();
				selectProposal(*proposalPopup_, cap->activeCompletionProposal(
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
void DefaultContentAssistant::startPopup() {
	if(proposalPopup_ == 0) {
		proposalPopup_ = new CompletionProposalPopup(*this);
		proposalPopup_->create(textViewer_->use());
	} else
		proposalPopup_->resetContent();

	// determine the horizontal orientation of the window
	const bool rtl = textViewer_->configuration().readingDirection == RIGHT_TO_LEFT;
	proposalPopup_->modifyStyleEx(rtl ? 0: WS_EX_LAYOUTRTL, rtl ? WS_EX_LAYOUTRTL : 0);
	setupPopupContent(*proposalPopup_, completionSession_->proposals.get(), completionSession_->numberOfProposals);

	updatePopupPositions();
	textViewer_->addViewportListener(*this);
	textViewer_->caret().addListener(*this);
	if(completionSession_->incremental)
		textViewer_->document().addListener(*this);
}

/// @see HasTimer#timeElapsed
void DefaultContentAssistant::timeElapsed(Timer&) {
	timer_.stop();
	showPossibleCompletions();
}

/// @see ContentAssistant#uninstall
void DefaultContentAssistant::uninstall() {
	if(textViewer_ != 0) {
		textViewer_->caret().removeCharacterInputListener(*this);
		textViewer_ = 0;
	}
}

void DefaultContentAssistant::updatePopupPositions() {
	if(proposalPopup_ != 0 && proposalPopup_->isWindow()) {
		RECT viewerRect;
		textViewer_->getClientRect(viewerRect);
		int cx = (viewerRect.right - viewerRect.left) / 4;
		int cy = proposalPopup_->getItemHeight(0) * min(static_cast<int>(completionSession_->numberOfProposals), 10) + 6;
		const POINT pt = textViewer_->clientXYForCharacter(completionSession_->replacementRegion.beginning(), false, layout::LineLayout::LEADING);
		const bool rtl = textViewer_->configuration().readingDirection == RIGHT_TO_LEFT;
		int x = !rtl ? (pt.x - 3) : (pt.x - cx - 1 + 3);
		if(x + cx > viewerRect.right) {
//			if()
		}
		int y = pt.y + textViewer_->textRenderer().textMetrics().cellHeight();
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
