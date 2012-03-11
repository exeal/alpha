/**
 * @file default-content-assistant.hpp
 * @author exeal
 * @date 2003-2006 was CompletionWindow.h
 * @date 2006-2012 was content-assist.hpp
 * @date 2012-03-12 separated from content-assist.hpp
 */

#ifndef ASCENSION_DEFAULT_CONTENT_ASSISTANT_HPP
#define ASCENSION_DEFAULT_CONTENT_ASSISTANT_HPP

#include <ascension/content-assist/content-assist.hpp>
#include <map>
#include <memory>	// std.unique_ptr

namespace ascension {
	namespace contentassist {
		/**
		 * Default implementation of @c ContentAssistant.
		 * @note This class is not intended to be subclassed.
		 */
		class DefaultContentAssistant : public ContentAssistant, public kernel::DocumentListener,
			public viewers::CaretListener, public viewers::CharacterInputListener,
			public viewers::ViewportListener, private ContentAssistant::CompletionProposalsUI {
		public:
			// constructors
			DefaultContentAssistant() /*throw()*/;
			~DefaultContentAssistant() /*throw()*/;
			// attributes
			uint32_t autoActivationDelay() const /*throw()*/;
			void enablePrefixCompletion(bool enable);
			void setAutoActivationDelay(uint32_t milliseconds);
			void setContentAssistProcessor(kernel::ContentType contentType, std::unique_ptr<ContentAssistProcessor> processor);
			// operation
			void showPossibleCompletions();
		private:
			void startPopup();
			void updatePopupPositions();
			// HasTimer
			void timeElapsed(Timer& timer);
			// ContentAssistant
			ContentAssistant::CompletionProposalsUI* completionProposalsUI() const /*throw()*/;
			const ContentAssistProcessor* contentAssistProcessor(kernel::ContentType contentType) const /*throw()*/;
			void install(viewers::TextViewer& viewer);
			void uninstall();
			// kernel.DocumentListener
			void documentAboutToBeChanged(const kernel::Document& document);
			void documentChanged(const kernel::Document& document, const kernel::DocumentChange& change);
			// viewers.CaretListener
			void caretMoved(const viewers::Caret& caret, const kernel::Region& oldRegion);
			// viewers.CharacterInputListener
			void characterInput(const viewers::Caret& caret, CodePoint c);
			// viewers.ViewportListener
			void viewportChanged(bool horizontal, bool vertical);
			// ContentAssistant.CompletionProposalsUI
			void close();
			bool complete();
			bool hasSelection() const /*throw()*/;
			void nextPage(int pages);
			void nextProposal(int proposals);
		private:
			viewers::TextViewer* textViewer_;
			std::map<kernel::ContentType, ContentAssistProcessor*> processors_;
			uint32_t autoActivationDelay_;
			Timer timer_;
			struct CompletionSession {
				const ContentAssistProcessor* processor;
				bool incremental;
				kernel::Region replacementRegion;
				std::unique_ptr<CompletionProposal*[]> proposals;
				std::size_t numberOfProposals;
				CompletionSession() /*throw()*/ : processor(nullptr), numberOfProposals(0) {}
				~CompletionSession() /*throw()*/ {
					for(std::size_t i = 0; i < numberOfProposals; ++i)
						delete proposals[i];
				}
			};
			std::unique_ptr<CompletionSession> completionSession_;
			class CompletionProposalsPopup {
			public:
				CompletionProposalsPopup(viewers::TextViewer& parent, CompletionProposalsUI& ui);
				void end();
				void resetContent(CompletionProposal* const proposals[], size_t numberOfProposals);
				const CompletionProposal* selectedProposal() const;
				void selectProposal(const CompletionProposal* selection);
				bool start(const std::set<CompletionProposal*>& proposals);
			private:
				CompletionProposalsUI& ui_;
				class Impl;
				std::unique_ptr<Impl> impl_;
			};
			std::unique_ptr<CompletionProposalsPopup> proposalsPopup_;
		};
	}
} // namespace ascension.contentassist

#endif // ASCENSION_DEFAULT_CONTENT_ASSISTANT_HPP
