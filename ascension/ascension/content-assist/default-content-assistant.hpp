/**
 * @file default-content-assistant.hpp
 * @author exeal
 * @date 2003-2006 was CompletionWindow.h
 * @date 2006-2012 was content-assist.hpp
 * @date 2012-03-12 separated from content-assist.hpp
 */

#ifndef ASCENSION_DEFAULT_CONTENT_ASSISTANT_HPP
#define ASCENSION_DEFAULT_CONTENT_ASSISTANT_HPP

#include <ascension/platforms.hpp>
#include <ascension/content-assist/content-assist.hpp>
#include <ascension/graphics/text-viewport.hpp>
#include <ascension/presentation/writing-mode.hpp>
#if defined(ASCENSION_WINDOW_SYSTEM_GTK)
#	include <gtkmm/treeview.h>
#elif defined(ASCENSION_WINDOW_SYSTEM_QUARTZ)
#	include <???>
#elif defined(ASCENSION_WINDOW_SYSTEM_QT)
#	include <QListView>
#elif defined(ASCENSION_WINDOW_SYSTEM_WIN32)
#	include <ascension/win32/window.hpp>
#elif defined(ASCENSION_WINDOW_SYSTEM_X)
#	include <???>
#endif
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
			public viewers::ViewportListener, public graphics::font::TextViewportListener,
			private ContentAssistant::CompletionProposalsUI {
		public:
			// constructors
			DefaultContentAssistant() /*noexcept*/;
			~DefaultContentAssistant() /*noexcept*/;
			// attributes
			std::uint32_t autoActivationDelay() const /*noexcept*/;
			void enablePrefixCompletion(bool enable);
			void setAutoActivationDelay(std::uint32_t milliseconds);
			void setContentAssistProcessor(kernel::ContentType contentType, std::unique_ptr<ContentAssistProcessor> processor);
			// operation
			void showPossibleCompletions();
		private:
			void startPopup();
			void updatePopupBounds();
			// HasTimer
			void timeElapsed(Timer& timer);
			// ContentAssistant
			ContentAssistant::CompletionProposalsUI* completionProposalsUI() const /*noexcept*/;
			const ContentAssistProcessor* contentAssistProcessor(kernel::ContentType contentType) const /*noexcept*/;
			void install(viewers::TextViewer& viewer);
			void uninstall();
			void viewerBoundsChanged() /*noexcept*/;
			// kernel.DocumentListener
			void documentAboutToBeChanged(const kernel::Document& document);
			void documentChanged(const kernel::Document& document, const kernel::DocumentChange& change);
			// viewers.CaretListener
			void caretMoved(const viewers::Caret& caret, const kernel::Region& oldRegion);
			// viewers.CharacterInputListener
			void characterInput(const viewers::Caret& caret, CodePoint c);
			// viewers.ViewportListener
			void viewportChanged(bool horizontal, bool vertical);
			// graphics.font.TextViewportListener
			void viewportBoundsInViewChanged(const graphics::NativeRectangle& oldBounds) /*noexcept*/;
			void viewportScrollPositionChanged(
				const presentation::AbstractTwoAxes<graphics::font::TextViewport::SignedScrollOffset>& offsets,
				const graphics::font::VisualLine& oldLine,
				graphics::font::TextViewport::ScrollOffset oldInlineProgressionOffset) /*noexcept*/;
			// ContentAssistant.CompletionProposalsUI
			void close();
			bool complete();
			bool hasSelection() const /*noexcept*/;
			void nextPage(int pages);
			void nextProposal(int proposals);
		private:
			viewers::TextViewer* textViewer_;
			std::map<kernel::ContentType, ContentAssistProcessor*> processors_;
			std::uint32_t autoActivationDelay_;
			Timer timer_;
			struct CompletionSession {
				const ContentAssistProcessor* processor;
				bool incremental;
				kernel::Region replacementRegion;
				std::unique_ptr<CompletionProposal*[]> proposals;
				std::size_t numberOfProposals;
				CompletionSession() /*noexcept*/ : processor(nullptr), numberOfProposals(0) {}
				~CompletionSession() /*noexcept*/ {
					for(std::size_t i = 0; i < numberOfProposals; ++i)
						delete proposals[i];
				}
			};
			std::unique_ptr<CompletionSession> completionSession_;
			class CompletionProposalsPopup : public
#if defined(ASCENSION_WINDOW_SYSTEM_GTK)
				Gtk::TreeView
#elif defined(ASCENSION_WINDOW_SYSTEM_QUARTZ)
				???
#elif defined(ASCENSION_WINDOW_SYSTEM_QT)
				QListView
#elif defined(ASCENSION_WINDOW_SYSTEM_WIN32)
				win32::SubclassedWindow
#elif defined(ASCENSION_WINDOW_SYSTEM_X)
				???
#endif
			{
			public:
				CompletionProposalsPopup(viewers::TextViewer& parent, CompletionProposalsUI& ui);
				void end();
				void resetContent(CompletionProposal* const proposals[], size_t numberOfProposals);
				const CompletionProposal* selectedProposal() const;
				void selectProposal(const CompletionProposal* selection);
				void setReadingDirection(presentation::ReadingDirection direction);
				bool start(const std::set<CompletionProposal*>& proposals);
			private:
#if defined(ASCENSION_WINDOW_SYSTEM_GTK)
#elif defined(ASCENSION_WINDOW_SYSTEM_QUARTZ)
#elif defined(ASCENSION_WINDOW_SYSTEM_QT)
#elif defined(ASCENSION_WINDOW_SYSTEM_WIN32)
				LRESULT processMessage(UINT message, WPARAM wp, LPARAM lp, bool& consumed);
				void setFont(const HFONT newFont);
				void updateDefaultFont();
				HFONT defaultFont_;
#elif defined(ASCENSION_WINDOW_SYSTEM_X)
#endif
			private:
				CompletionProposalsUI& ui_;
			};
			std::unique_ptr<CompletionProposalsPopup> proposalsPopup_;
		};
	}
} // namespace ascension.contentassist

#endif // ASCENSION_DEFAULT_CONTENT_ASSISTANT_HPP
