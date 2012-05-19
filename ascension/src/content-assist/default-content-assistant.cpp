/**
 * @file default-content-assistant.cpp
 * @author exeal
 * @date 2003-2006 was CompletionWindow.cpp
 * @date 2006-2012 was content-assist.cpp
 * @date 2012-03-12 renamed from content-assist.cpp
 */

#include <ascension/viewer/caret.hpp>
#include <ascension/content-assist/default-content-assistant.hpp>
#include <ascension/viewer/viewer.hpp>	// TextViewer

using namespace ascension;
using namespace ascension::contentassist;
using namespace ascension::kernel;
using namespace ascension::presentation;
using namespace ascension::text;
using namespace ascension::viewers;
using namespace std;


// DefaultContentAssistant ////////////////////////////////////////////////////////////////////////

namespace {
	class DisplayStringComparer {
	public:
		explicit DisplayStringComparer(const ContentAssistProcessor& processor) /*throw()*/ : processor_(processor) {
		}
		bool operator()(const CompletionProposal* lhs, const CompletionProposal* rhs) const {
			return processor_.compareDisplayStrings(lhs->displayString(), rhs->displayString());
		}
	private:
		const ContentAssistProcessor& processor_;
	};
}

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
					erase(document, locations::nextCharacter(caret, Direction::BACKWARD, locations::UTF32_CODE_UNIT), caret);
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
		// these connections were maken by startPopup() method
		textViewer_->removeViewportListener(*this);
		textViewer_->textRenderer().viewport()->removeListener(*this);
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
				completionSession_->proposals.get() + newProposals.size(), DisplayStringComparer(*completionSession_->processor));
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
				// proposal is only one which is auto insertable => insert it without popup
				(*proposals.begin())->replace(textViewer_->document(), completionSession_->replacementRegion);
				completionSession_.reset();
				delete *proposals.begin();
			} else {
				assert(completionSession_->proposals.get() == 0);
				completionSession_->proposals.reset(new CompletionProposal*[completionSession_->numberOfProposals = proposals.size()]);
				copy(proposals.begin(), proposals.end(), completionSession_->proposals.get());
				sort(completionSession_->proposals.get(),
					completionSession_->proposals.get() + completionSession_->numberOfProposals, DisplayStringComparer(*completionSession_->processor));
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
	assert(completionSession_.get() != 0);
	if(proposalsPopup_.get() == nullptr)
		proposalsPopup_.reset(new CompletionProposalsPopup(*textViewer_, *this));

	// determine the horizontal orientation of the window
	proposalsPopup_->setReadingDirection(textViewer_->textRenderer().writingMode().inlineFlowDirection);
	proposalsPopup_->resetContent(completionSession_->proposals.get(), completionSession_->numberOfProposals);

	updatePopupBounds();
	// these connections are revoke by close() method
	textViewer_->addViewportListener(*this);
	textViewer_->textRenderer().viewport()->addListener(*this);
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

/// @see ContentAssistant#viewerBoundsChanged
void DefaultContentAssistant::viewerBoundsChanged() /*throw()*/ {
	try {
		updatePopupBounds();
	} catch(...) {
	}
}

/// @see graphics.font.TextViewportListener.viewportBoundsInViewChanged
void DefaultContentAssistant::viewportBoundsInViewChanged(const graphics::NativeRectangle&) /*throw()*/ {
	viewerBoundsChanged();
}

/// @see graphics.font.TextViewportListener.viewportScrollPositionChanged
void DefaultContentAssistant::viewportScrollPositionChanged(
		const AbstractTwoAxes<graphics::font::TextViewport::SignedScrollOffset>&,
		const graphics::font::VisualLine&, graphics::font::TextViewport::ScrollOffset) /*throw()*/ {
	viewerBoundsChanged();
}
