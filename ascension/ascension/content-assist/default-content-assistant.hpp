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
#include <ascension/graphics/font/text-viewport.hpp>
#include <ascension/presentation/writing-mode.hpp>
#if defined(ASCENSION_WINDOW_SYSTEM_GTK)
#	include <gtkmm/liststore.h>
#	include <gtkmm/scrolledwindow.h>
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
			DefaultContentAssistant() BOOST_NOEXCEPT;
			// attributes
			std::uint32_t autoActivationDelay() const BOOST_NOEXCEPT;
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
			ContentAssistant::CompletionProposalsUI* completionProposalsUI() const BOOST_NOEXCEPT;
			std::shared_ptr<const ContentAssistProcessor>
				contentAssistProcessor(kernel::ContentType contentType) const BOOST_NOEXCEPT;
			void install(viewers::TextViewer& viewer);
			void uninstall();
			void viewerBoundsChanged() BOOST_NOEXCEPT;
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
			void viewportBoundsInViewChanged(const graphics::Rectangle& oldBounds) BOOST_NOEXCEPT;
			void viewportScrollPositionChanged(
				const presentation::AbstractTwoAxes<graphics::font::TextViewport::SignedScrollOffset>& offsets,
				const graphics::font::VisualLine& oldLine,
				graphics::font::TextViewport::ScrollOffset oldInlineProgressionOffset) BOOST_NOEXCEPT;
			// ContentAssistant.CompletionProposalsUI
			void close();
			bool complete();
			bool hasSelection() const BOOST_NOEXCEPT;
			void nextPage(int pages);
			void nextProposal(int proposals);
		private:
			viewers::TextViewer* textViewer_;
			std::map<kernel::ContentType, std::shared_ptr<ContentAssistProcessor>> processors_;
			std::uint32_t autoActivationDelay_;
			Timer timer_;
			struct CompletionSession {
				std::shared_ptr<const ContentAssistProcessor> processor;
				bool incremental;
				kernel::Region replacementRegion;
				std::unique_ptr<std::shared_ptr<const CompletionProposal>[]> proposals;
				std::size_t numberOfProposals;
				CompletionSession() BOOST_NOEXCEPT : numberOfProposals(0) {}
			};
			std::unique_ptr<CompletionSession> completionSession_;
			class CompletionProposalsPopup : public
#if defined(ASCENSION_WINDOW_SYSTEM_GTK)
				Gtk::ScrolledWindow
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
				void resetContent(std::shared_ptr<const CompletionProposal> proposals[], size_t numberOfProposals);
				std::shared_ptr<const CompletionProposal> selectedProposal() const;
				void selectProposal(std::shared_ptr<const CompletionProposal> selection);
				void setWritingMode(const presentation::WritingMode& writingMode);
//				bool start(const std::set<const CompletionProposal*>& proposals);
			private:
#if defined(ASCENSION_WINDOW_SYSTEM_GTK)
				Gtk::TreeView view_;
				Glib::RefPtr<Gtk::ListStore> model_;
				struct ColumnRecord : Gtk::TreeModel::ColumnRecord {
					ColumnRecord();
					Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf>> icon;
					Gtk::TreeModelColumn<Glib::ustring> displayString;
					Gtk::TreeModelColumn<std::shared_ptr<const CompletionProposal>> proposalObject;
				} columns_;
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
				std::vector<std::shared_ptr<const CompletionProposal>> proposals_;
			};
			std::unique_ptr<CompletionProposalsPopup> proposalsPopup_;
		};
	}
} // namespace ascension.contentassist

#endif // ASCENSION_DEFAULT_CONTENT_ASSISTANT_HPP
