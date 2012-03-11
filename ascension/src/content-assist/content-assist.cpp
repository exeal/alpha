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
void DefaultCompletionProposal::replace(Document& document, const Region& replacementRegion) const {
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

/// @see ContentAssistProcessor#isIncrementalCompletionAutoTerminationCharacter
bool IdentifiersProposalProcessor::isIncrementalCompletionAutoTerminationCharacter(CodePoint c) const /*throw()*/ {
	return !syntax_.isIdentifierContinueCharacter(c);
}

/// @see ContentAssistProcessor#recomputIncrementalCompletionProposals
void IdentifiersProposalProcessor::recomputeIncrementalCompletionProposals(
		const TextViewer&, const Region&, CompletionProposal* const[], size_t, set<CompletionProposal*>&) const {
	// do nothing
}


// DefaultContentAssistant ////////////////////////////////////////////////////////////////////////

/// Constructor.
DefaultContentAssistant::DefaultContentAssistant() /*throw()*/ : textViewer_(0), autoActivationDelay_(500) {
}

/// Destructor.
DefaultContentAssistant::~DefaultContentAssistant() /*throw()*/ {
	for(map<ContentType, ContentAssistProcessor*>::iterator i(processors_.begin()), e(processors_.end()); i != e; ++i)
		delete i->second;
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
		proposalsPopup_->end();
	}
}

/// @see CompletionProposalsUI#complete
bool DefaultContentAssistant::complete() {
	if(completionSession_.get() != 0) {
		if(const CompletionProposal* const p = proposalsPopup_->selectedProposal()) {
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
			proposalsPopup_->resetContent(completionSession_->proposals.get(), completionSession_->numberOfProposals);
		}

		// select the most preferred
		proposalsPopup_->selectProposal(completionSession_->processor->activeCompletionProposal(
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
	return completionSession_.get() != 0 && proposalsPopup_.get() != nullptr && proposalsPopup_->selectedProposal() != nullptr;
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
				proposalsPopup_->selectProposal(cap->activeCompletionProposal(
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
	if(proposalsPopup_.get() == nullptr)
		proposalsPopup_.reset(new CompletionProposalsPopup(*textViewer_, *this));

	// determine the horizontal orientation of the window
	const bool rtl = textViewer_->configuration().readingDirection == RIGHT_TO_LEFT;
	proposalPopup_->modifyStyleEx(rtl ? 0: WS_EX_LAYOUTRTL, rtl ? WS_EX_LAYOUTRTL : 0);
	proposalsPopup_->resetContent(completionSession_->proposals.get(), completionSession_->numberOfProposals);

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
	if(proposalsPopup_ != 0 && proposalsPopup_->isWindow()) {
		using namespace ascension::graphics;
		NativeRectangle viewerBounds(textViewer_->bounds(false));
		Scalar dx = geometry::dx(viewerBounds) / 4;
		Scalar dy = proposalsPopup_->getItemHeight(0) * min(static_cast<Scalar>(completionSession_->numberOfProposals), 10) + 6;
		const POINT pt = textViewer_->clientXYForCharacter(completionSession_->replacementRegion.beginning(), false, layout::LineLayout::LEADING);
		const bool rtl = textViewer_->configuration().readingDirection == RIGHT_TO_LEFT;
		Scalar x = !rtl ? (pt.x - 3) : (pt.x - dx - 1 + 3);
		if(x + dx > graphics::geometry::right(viewerBounds)) {
//			if()
		}
		int y = pt.y + textViewer_->textRenderer().textMetrics().cellHeight();
		if(y + dy > geometry::bottom(viewerBounds)) {
			if(pt.y - 1 - geometry::top(viewerBounds) < geometry::bottom(viewerBounds) - y)
				dy = geometry::bottom(viewerBounds) - y;
			else {
				dy = min<Scalar>(dy, pt.y - geometry::top(viewerBounds));
				y = pt.y - dy - 1;
			}
		}
		proposalsPopup_->setPosition(0, x, y, dx, dy, SWP_NOZORDER | SWP_SHOWWINDOW);
	}
}

/// @see viewers#IViewportListener#viewportChanged
void ContentAssistant::viewportChanged(bool, bool) {
	updatePopupPositions();
}
