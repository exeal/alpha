/**
 * @file default-content-assistant.cpp
 * @author exeal
 * @date 2003-2006 was CompletionWindow.cpp
 * @date 2006-2012 was content-assist.cpp
 * @date 2012-03-12 renamed from content-assist.cpp
 */

#include <ascension/content-assist/default-content-assistant.hpp>
#include <ascension/graphics/font/font-metrics.hpp>
#include <ascension/graphics/rendering-context.hpp>
#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/viewer.hpp>	// TextViewer
#include <boost/range/algorithm/copy.hpp>

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
		explicit DisplayStringComparer(const ContentAssistProcessor& processor) BOOST_NOEXCEPT : processor_(processor) {
		}
		bool operator()(shared_ptr<const CompletionProposal>& lhs, shared_ptr<const CompletionProposal>& rhs) const {
			return processor_.compareDisplayStrings(lhs->displayString(), rhs->displayString());
		}
	private:
		const ContentAssistProcessor& processor_;
	};
}

/// Constructor.
DefaultContentAssistant::DefaultContentAssistant() BOOST_NOEXCEPT : textViewer_(nullptr), autoActivationDelay_(500) {
}

/// Returns the automatic activation delay in milliseconds.
uint32_t DefaultContentAssistant::autoActivationDelay() const BOOST_NOEXCEPT {
	return autoActivationDelay_;
}

/// @see viewers#CaretListener
void DefaultContentAssistant::caretMoved(const Caret& caret, const Region&) {
	if(completionSession_.get() != nullptr) {
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
	if(textViewer_ != nullptr) {
		if(completionSession_.get() != nullptr) {
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
			if(const shared_ptr<const ContentAssistProcessor> cap = contentAssistProcessor(contentType(textViewer_->caret()))) {
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
	if(completionSession_.get() != nullptr) {
		// these connections were maken by startPopup() method
		textViewer_->removeViewportListener(*this);
		textViewer_->textRenderer().viewport()->removeListener(*this);
		textViewer_->caret().removeListener(*this);
		if(completionSession_->incremental)
			textViewer_->document().removeListener(*this);
		completionSession_.reset();
		proposalsPopup_->end();	// TODO: Do i need to reset proposalsPopup_ ???
	}
}

/// @see CompletionProposalsUI#complete
bool DefaultContentAssistant::complete() {
	if(completionSession_.get() != nullptr) {
		if(const shared_ptr<const CompletionProposal> p = proposalsPopup_->selectedProposal()) {
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
	if(completionSession_.get() != nullptr) {
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
		set<shared_ptr<const CompletionProposal>> newProposals;
		completionSession_->processor->recomputeIncrementalCompletionProposals(
			*textViewer_, completionSession_->replacementRegion,
			completionSession_->proposals.get(), completionSession_->numberOfProposals, newProposals);
		if(!newProposals.empty()) {
			completionSession_->proposals.reset();
			if(newProposals.size() == 1 && (*begin(newProposals))->isAutoInsertable()) {
				(*begin(newProposals))->replace(textViewer_->document(), completionSession_->replacementRegion);
				return close();
			}
			completionSession_->proposals.reset(new shared_ptr<const CompletionProposal>[completionSession_->numberOfProposals = newProposals.size()]);
			boost::copy(newProposals, completionSession_->proposals.get());
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
ContentAssistant::CompletionProposalsUI* DefaultContentAssistant::completionProposalsUI() const BOOST_NOEXCEPT {
	return (completionSession_.get() != nullptr) ? const_cast<DefaultContentAssistant*>(this) : nullptr;
}

/// @see ContentAssistant#contentAssistProcessor
shared_ptr<const ContentAssistProcessor> DefaultContentAssistant::contentAssistProcessor(ContentType contentType) const BOOST_NOEXCEPT {
	map<ContentType, shared_ptr<ContentAssistProcessor>>::const_iterator i(processors_.find(contentType));
	return (i != end(processors_)) ? i->second : shared_ptr<const ContentAssistProcessor>();
}

/// @see CompletionProposalsUI#hasSelection
bool DefaultContentAssistant::hasSelection() const BOOST_NOEXCEPT {
	return completionSession_.get() != nullptr && proposalsPopup_.get() != nullptr && proposalsPopup_->selectedProposal() != nullptr;
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
	map<ContentType, shared_ptr<ContentAssistProcessor>>::iterator i(processors_.find(contentType));
	if(i != end(processors_))
		processors_.erase(i);
	if(processor.get() != nullptr)
		processors_.insert(make_pair(contentType, move(processor)));
}

/// @see ContentAssistant#showPossibleCompletions
void DefaultContentAssistant::showPossibleCompletions() {
	if(textViewer_ == nullptr || completionSession_.get() != nullptr || textViewer_->document().isReadOnly())
		return textViewer_->beep();
	const Caret& caret = textViewer_->caret();
	if(const shared_ptr<const ContentAssistProcessor> cap = contentAssistProcessor(contentType(caret))) {
		set<shared_ptr<const CompletionProposal>> proposals;
		completionSession_.reset(new CompletionSession);
		(completionSession_->processor = cap)->computeCompletionProposals(caret,
			completionSession_->incremental, completionSession_->replacementRegion, proposals);
		if(!proposals.empty()) {
			if(proposals.size() == 1 && (*begin(proposals))->isAutoInsertable()) {
				// proposal is only one which is auto insertable => insert it without popup
				(*begin(proposals))->replace(textViewer_->document(), completionSession_->replacementRegion);
				completionSession_.reset();
			} else {
				assert(completionSession_->proposals.get() == nullptr);
				completionSession_->proposals.reset(new shared_ptr<const CompletionProposal>[completionSession_->numberOfProposals = proposals.size()]);
				boost::copy(proposals, completionSession_->proposals.get());
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
	assert(completionSession_.get() != nullptr);
	if(proposalsPopup_.get() == nullptr)
		proposalsPopup_.reset(new CompletionProposalsPopup(*textViewer_, *this));

	// determine the horizontal orientation of the window
	proposalsPopup_->setWritingMode(textViewer_->textRenderer().layouts().at(line(textViewer_->caret())).writingMode());
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
	if(textViewer_ != nullptr) {
		textViewer_->caret().removeCharacterInputListener(*this);
		textViewer_ = nullptr;
	}
}

void DefaultContentAssistant::updatePopupBounds() {
	if(proposalsPopup_.get() == nullptr)
		return;

	using namespace ascension::graphics;
	if(const shared_ptr<const font::TextViewport> viewport = textViewer_->textRenderer().viewport()) {
		const presentation::WritingMode& writingMode = textViewer_->textRenderer().layouts().at(kernel::line(textViewer_->caret())).writingMode();
		graphics::Rectangle screenBounds(widgetapi::bounds(widgetapi::desktop(), false));
		screenBounds = widgetapi::mapFromGlobal(*textViewer_, screenBounds);
		Dimension size;
#if defined(ASCENSION_WINDOW_SYSTEM_GTK)
		Gtk::TreeView* view = static_cast<Gtk::TreeView*>(proposalsPopup_->get_child());
		assert(view != nullptr);
		Gtk::TreeModel::Path startPath, endPath;
		view->get_visible_range(startPath, endPath);
		Gdk::Rectangle cellArea;
		view->get_cell_area(startPath, *view->get_column(0/*1*/), cellArea);
		const Scalar itemHeight = static_cast<Scalar>(cellArea.get_height());
#elif defined(ASCENSION_WINDOW_SYSTEM_WIN32)
		const LRESULT listItemHeight = ::SendMessageW(proposalsPopup_->handle().get(), LB_GETITEMHEIGHT, 0, 0);
		if(listItemHeight == LB_ERR)
			throw makePlatformError();
		const Scalar itemHeight = listItemHeight;
#endif
		if(isHorizontal(writingMode.blockFlowDirection)) {
			geometry::dx(size) = geometry::dx(screenBounds) / 4;
			geometry::dy(size) = static_cast<Scalar>(itemHeight * min<size_t>(completionSession_->numberOfProposals, 10) + 6);
		} else {
			geometry::dx(size) = static_cast<Scalar>(itemHeight * min<size_t>(completionSession_->numberOfProposals, 10) + 6);
			geometry::dy(size) = geometry::dy(screenBounds) / 4;
		}
		Point origin(font::modelToView(*viewport, font::TextHit<kernel::Position>::leading(completionSession_->replacementRegion.beginning()), false));
		// TODO: This code does not support vertical writing mode.
		if(writingMode.blockFlowDirection == presentation::LEFT_TO_RIGHT)
			geometry::x(origin) = geometry::x(origin) - 3;
		else
			geometry::x(origin) = geometry::x(origin) - geometry::dx(size) - 1 + 3;
		proposalsPopup_->setWritingMode(writingMode);
		if(geometry::x(origin) + geometry::dx(size) > graphics::geometry::right(screenBounds)) {
//			if()
		}
		geometry::y(origin) = geometry::y(origin) + widgetapi::createRenderingContext(*textViewer_)->fontMetrics(textViewer_->textRenderer().defaultFont())->cellHeight();
		if(geometry::y(origin) + geometry::dy(size) > geometry::bottom(screenBounds)) {
			if(geometry::y(origin) - 1 - geometry::top(screenBounds) < geometry::bottom(screenBounds) - geometry::y(origin))
				geometry::dy(size) = geometry::bottom(screenBounds) - geometry::y(origin);
			else {
				geometry::dy(size) = min<Scalar>(geometry::dy(size), geometry::y(origin) - geometry::top(screenBounds));
				geometry::y(origin) = geometry::y(origin) - geometry::dy(size) - 1;
			}
		}
		widgetapi::setBounds(*proposalsPopup_, graphics::Rectangle(origin, size));
	}
}

/// @see ContentAssistant#viewerBoundsChanged
void DefaultContentAssistant::viewerBoundsChanged() BOOST_NOEXCEPT {
	try {
		updatePopupBounds();
	} catch(...) {
	}
}

/// @see graphics.font.TextViewportListener.viewportBoundsInViewChanged
void DefaultContentAssistant::viewportBoundsInViewChanged(const graphics::Rectangle&) BOOST_NOEXCEPT {
	viewerBoundsChanged();
}

/// @see graphics.font.TextViewportListener.viewportScrollPositionChanged
void DefaultContentAssistant::viewportScrollPositionChanged(
		const AbstractTwoAxes<graphics::font::TextViewport::SignedScrollOffset>&,
		const graphics::font::VisualLine&, graphics::font::TextViewport::ScrollOffset) BOOST_NOEXCEPT {
	viewerBoundsChanged();
}
