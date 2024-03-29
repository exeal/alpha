/**
 * @file default-content-assistant.cpp
 * @author exeal
 * @date 2003-2006 was CompletionWindow.cpp
 * @date 2006-2012 was content-assist.cpp
 * @date 2012-03-12 renamed from content-assist.cpp
 * @date 2014-2016
 */

#include <ascension/corelib/numeric-range-algorithm/encompasses.hpp>
#include <ascension/content-assist/default-content-assistant.hpp>
#include <ascension/kernel/document.hpp>
#include <ascension/graphics/font/font-metrics.hpp>
#include <ascension/graphics/font/line-layout-vector.hpp>
#include <ascension/graphics/font/text-layout.hpp>
#include <ascension/graphics/font/text-viewport.hpp>
#include <ascension/graphics/geometry/point-xy.hpp>
#include <ascension/graphics/geometry/rectangle-odxdy.hpp>
#include <ascension/graphics/geometry/rectangle-sides.hpp>
#include <ascension/graphics/geometry/algorithms/make.hpp>
#include <ascension/graphics/rendering-context.hpp>
#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/text-area.hpp>
#include <ascension/viewer/text-viewer.hpp>
#include <ascension/viewer/text-viewer-model-conversion.hpp>
#include <boost/range/algorithm/copy.hpp>


namespace ascension {
	namespace contentassist {
		// DefaultContentAssistant ////////////////////////////////////////////////////////////////////////////////////

		namespace {
			class DisplayStringComparer {
			public:
				explicit DisplayStringComparer(const ContentAssistProcessor& processor) BOOST_NOEXCEPT : processor_(processor) {
				}
				bool operator()(std::shared_ptr<const CompletionProposal>& lhs, std::shared_ptr<const CompletionProposal>& rhs) const {
					return processor_.compareDisplayStrings(lhs->displayString(), rhs->displayString());
				}
			private:
				const ContentAssistProcessor& processor_;
			};
		}

		/// Constructor.
		DefaultContentAssistant::DefaultContentAssistant() BOOST_NOEXCEPT : textViewer_(nullptr), autoActivationDelay_(500) {
		}

		/// Returns the automatic activation delay.
		boost::chrono::milliseconds DefaultContentAssistant::autoActivationDelay() const BOOST_NOEXCEPT {
			return autoActivationDelay_;
		}

		/// @see viewer#CaretListener
		void DefaultContentAssistant::caretMoved(const viewer::Caret& caret, const kernel::Region&) {
			if(completionSession_.get() != nullptr) {
				// non-incremental mode: close when the caret moved
				if(!completionSession_->incremental)
					close();
				// incremental mode: close if the caret gone out of the replacement region
				else if(!encompasses(completionSession_->replacementRegion, viewer::insertionPosition(caret)))
					close();
			}
		}

		/// @see viewer#CharacterInputListener#characterInput
		void DefaultContentAssistant::characterInput(const viewer::Caret&, CodePoint c) {
			if(textViewer_ != nullptr) {
				if(completionSession_.get() != nullptr) {
					if(!completionSession_->incremental)
						close();
					else if(completionSession_->processor->isIncrementalCompletionAutoTerminationCharacter(c)) {
						auto caret(textViewer_->textArea()->caret());
						try {
							viewer::document(*textViewer_)->insertUndoBoundary();
							kernel::erase(*viewer::document(*textViewer_),
								kernel::Region(
									kernel::locations::nextCharacter(*caret, Direction::backward(), kernel::locations::UTF32_CODE_UNIT),
									viewer::insertionPosition(*caret)));
							viewer::document(*textViewer_)->insertUndoBoundary();
							complete();
						} catch(...) {
						}
					}
				} else {
					// activate automatically
					if(const std::shared_ptr<const ContentAssistProcessor> cap
							= contentAssistProcessor(contentType(*textViewer_->textArea()->caret()))) {
						if(cap->isCompletionProposalAutoActivationCharacter(c)) {
							if(autoActivationDelay_ == boost::chrono::milliseconds::zero())
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
				// these connections were made by startPopup() method
				textAreaContentRectangleChangedConnection_.disconnect();
				viewportScrolledConnection_.disconnect();
				caretMotionConnection_.disconnect();
				if(completionSession_->incremental)
					viewer::document(*textViewer_)->removeListener(*this);
				completionSession_.reset();
				proposalsPopup_->end();	// TODO: Do i need to reset proposalsPopup_ ???
			}
		}

		/// @see CompletionProposalsUI#complete
		bool DefaultContentAssistant::complete() {
			if(completionSession_.get() != nullptr) {
				if(const std::shared_ptr<const CompletionProposal> p = proposalsPopup_->selectedProposal()) {
					std::unique_ptr<CompletionSession> temp(move(completionSession_));	// force completionSession_ to null
					const auto document(viewer::document(*textViewer_));
					if(!document->isReadOnly()) {
						document->insertUndoBoundary();
						p->replace(*document, temp->replacementRegion);
						document->insertUndoBoundary();
					}
					completionSession_ = std::move(temp);
					close();
					return true;
				}
				close();
			}
			return false;
		}

		/// @see kernel#DocumentListener#documentAboutToBeChanged
		void DefaultContentAssistant::documentAboutToBeChanged(const kernel::Document&, const kernel::DocumentChange&) {
			// do nothing
		}

		/// @see kernel#DocumentListener#documentChanged
		void DefaultContentAssistant::documentChanged(const kernel::Document&, const kernel::DocumentChange& change) {
			if(completionSession_.get() != nullptr) {
				// exit or update the replacement region
				if(!completionSession_->incremental
						|| boost::size(change.erasedRegion().lines()) == 1
						|| boost::size(change.insertedRegion().lines()) == 1)
					close();
				const kernel::Region& replacementRegion = completionSession_->replacementRegion;
				if(!boost::empty(change.erasedRegion()) && !encompasses(replacementRegion, change.erasedRegion()))
					close();
				completionSession_->replacementRegion = kernel::Region(
					*boost::const_begin(completionSession_->replacementRegion),
					kernel::locations::updatePosition(*boost::const_end(completionSession_->replacementRegion), change, Direction::forward()));
				if(!boost::empty(change.insertedRegion()) && !encompasses(replacementRegion, change.insertedRegion()))
					close();

				// rebuild proposals
				std::set<std::shared_ptr<const CompletionProposal>> newProposals;
				completionSession_->processor->recomputeIncrementalCompletionProposals(
					*textViewer_, completionSession_->replacementRegion,
					completionSession_->proposals.get(), completionSession_->numberOfProposals, newProposals);
				if(!newProposals.empty()) {
					completionSession_->proposals.reset();
					if(newProposals.size() == 1 && (*std::begin(newProposals))->isAutoInsertable()) {
						(*std::begin(newProposals))->replace(*viewer::document(*textViewer_), completionSession_->replacementRegion);
						return close();
					}
					completionSession_->proposals.reset(new std::shared_ptr<const CompletionProposal>[completionSession_->numberOfProposals = newProposals.size()]);
					boost::copy(newProposals, completionSession_->proposals.get());
					std::sort(completionSession_->proposals.get(),
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
		std::shared_ptr<const ContentAssistProcessor> DefaultContentAssistant::contentAssistProcessor(const kernel::ContentType& contentType) const BOOST_NOEXCEPT {
			std::map<kernel::ContentType, std::shared_ptr<ContentAssistProcessor>>::const_iterator i(processors_.find(contentType));
			return (i != std::end(processors_)) ? i->second : std::shared_ptr<const ContentAssistProcessor>();
		}

		/// @see CompletionProposalsUI#hasSelection
		bool DefaultContentAssistant::hasSelection() const BOOST_NOEXCEPT {
			return completionSession_.get() != nullptr && proposalsPopup_.get() != nullptr && proposalsPopup_->selectedProposal() != nullptr;
		}

		/// @see ContentAssistant#install
		void DefaultContentAssistant::install(viewer::TextViewer& viewer) {
			textViewer_ = &viewer;
			caretCharacterInputConnection_ = textViewer_->textArea()->caret()->characterInputSignal().connect(
				std::bind(&DefaultContentAssistant::characterInput, this, std::placeholders::_1, std::placeholders::_2));
		}

		/**
		 * Sets the delay between a character input and the session activation.
		 * @param newValue The delay amount. If set to zero, the proposals will popup immediately
		 */
		void DefaultContentAssistant::setAutoActivationDelay(boost::chrono::milliseconds newValue) {
			autoActivationDelay_ = newValue;
		}

		/**
		 * Registers the given content assist processor for the specified content type. If there is already a processor
		 * registered for the content type, the old processor is unregistered.
		 * @param contentType The content type
		 * @param processor The new content assist processor to register or @c null to unregister
		 */
		void DefaultContentAssistant::setContentAssistProcessor(const kernel::ContentType& contentType, std::unique_ptr<ContentAssistProcessor> processor) {
			std::map<kernel::ContentType, std::shared_ptr<ContentAssistProcessor>>::iterator i(processors_.find(contentType));
			if(i != std::end(processors_))
				processors_.erase(i);
			if(processor.get() != nullptr)
				processors_.insert(std::make_pair(contentType, std::move(processor)));
		}

		/// @see ContentAssistant#showPossibleCompletions
		void DefaultContentAssistant::showPossibleCompletions() {
			if(textViewer_ == nullptr || completionSession_.get() != nullptr || viewer::document(*textViewer_)->isReadOnly())
				return textViewer_->beep();
			const auto caret(textViewer_->textArea()->caret());
			if(const std::shared_ptr<const ContentAssistProcessor> cap = contentAssistProcessor(kernel::contentType(*caret))) {
				std::set<std::shared_ptr<const CompletionProposal>> proposals;
				completionSession_.reset(new CompletionSession);
				(completionSession_->processor = cap)->computeCompletionProposals(*caret,
					completionSession_->incremental, completionSession_->replacementRegion, proposals);
				if(!proposals.empty()) {
					if(proposals.size() == 1 && (*std::begin(proposals))->isAutoInsertable()) {
						// proposal is only one which is auto insertable => insert it without popup
						(*std::begin(proposals))->replace(*viewer::document(*textViewer_), completionSession_->replacementRegion);
						completionSession_.reset();
					} else {
						assert(completionSession_->proposals.get() == nullptr);
						completionSession_->proposals.reset(new std::shared_ptr<const CompletionProposal>[completionSession_->numberOfProposals = proposals.size()]);
						boost::copy(proposals, completionSession_->proposals.get());
						std::sort(completionSession_->proposals.get(),
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
			proposalsPopup_->setWritingMode(
				graphics::font::writingMode(
					textViewer_->textArea()->textRenderer()->layouts().at(
						kernel::line(*textViewer_->textArea()->caret()),
						graphics::font::LineLayoutVector::USE_CALCULATED_LAYOUT)));
			proposalsPopup_->resetContent(completionSession_->proposals.get(), completionSession_->numberOfProposals);
			updatePopupBounds();

			// these connections are revoke by close() method
			textAreaContentRectangleChangedConnection_ = textViewer_->textArea()->contentRectangleChangedSignal().connect([this](const viewer::TextArea&) {
				this->viewerBoundsChanged();
			});
			viewportScrolledConnection_ = textViewer_->textArea()->viewport()->scrolledSignal().connect(
				[this](const presentation::FlowRelativeTwoAxes<graphics::font::TextViewport::ScrollOffset>&, const graphics::font::VisualLine&) {
					this->viewerBoundsChanged();
				}
			);
			caretMotionConnection_ = textViewer_->textArea()->caret()->motionSignal().connect(
				std::bind(&DefaultContentAssistant::caretMoved, this, std::placeholders::_1, std::placeholders::_2));
			if(completionSession_->incremental)
				viewer::document(*textViewer_)->addListener(*this);
		}

		/// @see HasTimer#timeElapsed
		void DefaultContentAssistant::timeElapsed(Timer<>&) {
			timer_.stop();
			showPossibleCompletions();
		}

		/// @see ContentAssistant#uninstall
		void DefaultContentAssistant::uninstall() {
			if(textViewer_ != nullptr) {
				caretCharacterInputConnection_.disconnect();
				textViewer_ = nullptr;
			}
		}

		void DefaultContentAssistant::updatePopupBounds() {
			if(proposalsPopup_.get() == nullptr)
				return;

			auto textRenderer(textViewer_->textArea()->textRenderer());
			const presentation::WritingMode writingMode(
				graphics::font::writingMode(
					textRenderer->layouts().at(
						kernel::line(*textViewer_->textArea()->caret()), graphics::font::LineLayoutVector::USE_CALCULATED_LAYOUT)));

#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
			const Glib::RefPtr<const Gdk::Screen> screen(textViewer_->get_screen());
			auto screenBounds(
				graphics::geometry::make<graphics::Rectangle>(
					boost::geometry::make_zero<graphics::Point>(),
					graphics::Dimension(
						graphics::geometry::_dx = static_cast<graphics::Scalar>(screen->get_width()),
						graphics::geometry::_dy = static_cast<graphics::Scalar>(screen->get_height()))));
#else
			auto screenBounds(viewer::widgetapi::bounds(viewer::widgetapi::desktop(), false));
#endif
			screenBounds = viewer::widgetapi::mapFromGlobal(*textViewer_, screenBounds);

			graphics::Dimension size;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
			Gtk::TreeView* view = static_cast<Gtk::TreeView*>(proposalsPopup_->get_child());
			assert(view != nullptr);
			Gtk::TreeModel::Path startPath, endPath;
			view->get_visible_range(startPath, endPath);
			Gdk::Rectangle cellArea;
			view->get_cell_area(startPath, *view->get_column(0/*1*/), cellArea);
			const graphics::Scalar itemHeight = static_cast<graphics::Scalar>(cellArea.get_height());
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			const LRESULT listItemHeight = ::SendMessageW(proposalsPopup_->handle().get(), LB_GETITEMHEIGHT, 0, 0);
			if(listItemHeight == LB_ERR)
				throw makePlatformError();
			const auto itemHeight = static_cast<graphics::Scalar>(listItemHeight);
#endif
			if(isHorizontal(writingMode.blockFlowDirection)) {
				graphics::geometry::dx(size) = graphics::geometry::dx(screenBounds) / 4;
				graphics::geometry::dy(size) = static_cast<graphics::Scalar>(itemHeight * std::min<std::size_t>(completionSession_->numberOfProposals, 10) + 6);
			} else {
				graphics::geometry::dx(size) = static_cast<graphics::Scalar>(itemHeight * std::min<std::size_t>(completionSession_->numberOfProposals, 10) + 6);
				graphics::geometry::dy(size) = graphics::geometry::dy(screenBounds) / 4;
			}
			graphics::Point origin(
				viewer::modelToView(
					*textViewer_, graphics::font::TextHit<kernel::Position>::leading(*boost::const_begin(completionSession_->replacementRegion))));
			// TODO: This code does not support vertical writing mode.
			if(writingMode.blockFlowDirection == presentation::LEFT_TO_RIGHT)
				graphics::geometry::x(origin) = graphics::geometry::x(origin) - 3;
			else
				graphics::geometry::x(origin) = graphics::geometry::x(origin) - graphics::geometry::dx(size) - 1 + 3;
			proposalsPopup_->setWritingMode(writingMode);
			if(graphics::geometry::x(origin) + graphics::geometry::dx(size) > graphics::geometry::right(screenBounds)) {
//				if()
			}
			graphics::geometry::y(origin) = graphics::geometry::y(origin) +
				viewer::widgetapi::createRenderingContext(*textViewer_)->fontMetrics(textRenderer->defaultFont())->cellHeight();
			if(graphics::geometry::y(origin) + graphics::geometry::dy(size) > graphics::geometry::bottom(screenBounds)) {
				if(graphics::geometry::y(origin) - 1 - graphics::geometry::top(screenBounds) < graphics::geometry::bottom(screenBounds) - graphics::geometry::y(origin))
					graphics::geometry::dy(size) = graphics::geometry::bottom(screenBounds) - graphics::geometry::y(origin);
				else {
					graphics::geometry::dy(size) = std::min<graphics::Scalar>(
						graphics::geometry::dy(size), graphics::geometry::y(origin) - graphics::geometry::top(screenBounds));
					graphics::geometry::y(origin) = graphics::geometry::y(origin) - graphics::geometry::dy(size) - 1;
				}
			}
			viewer::widgetapi::setBounds(*proposalsPopup_, graphics::geometry::make<graphics::Rectangle>(origin, size));
		}

		/// @see ContentAssistant#viewerBoundsChanged
		void DefaultContentAssistant::viewerBoundsChanged() BOOST_NOEXCEPT {
			try {
				updatePopupBounds();
			} catch(...) {
			}
		}
	}

	namespace viewer {
		namespace utils {
			/// Closes the opened completion proposals popup immediately.
			void closeCompletionProposalsPopup(TextViewer& viewer) BOOST_NOEXCEPT {
				if(const auto ca = viewer.contentAssistant()) {
					if(contentassist::ContentAssistant::CompletionProposalsUI* cpui = ca->completionProposalsUI())
						cpui->close();
				}
			}
		}
	}
}
