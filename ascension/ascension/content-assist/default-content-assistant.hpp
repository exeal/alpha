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
#include <ascension/corelib/timer.hpp>
#include <ascension/presentation/writing-mode.hpp>
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
#	include <gtkmm/liststore.h>
#	include <gtkmm/scrolledwindow.h>
#	include <gtkmm/treeview.h>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#	include <???>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
#	include <QListView>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
#	include <ascension/win32/window.hpp>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(X)
#	include <???>
#endif
#include <map>
#include <memory>	// std.unique_ptr
#include <boost/signals2.hpp>

namespace ascension {
	namespace contentassist {
		/**
		 * Default implementation of @c ContentAssistant.
		 * @note This class is not intended to be subclassed.
		 */
		class DefaultContentAssistant : public ContentAssistant,
			public HasTimer<>, public kernel::DocumentListener, private ContentAssistant::CompletionProposalsUI {
		public:
			// constructors
			DefaultContentAssistant() BOOST_NOEXCEPT;
			// attributes
			boost::chrono::milliseconds autoActivationDelay() const BOOST_NOEXCEPT;
			void enablePrefixCompletion(bool enable);
			void setAutoActivationDelay(boost::chrono::milliseconds newValue);
			void setContentAssistProcessor(kernel::ContentType contentType, std::unique_ptr<ContentAssistProcessor> processor);
			// operation
			void showPossibleCompletions();

		private:
			void startPopup();
			void updatePopupBounds();
			// HasTimer
			void timeElapsed(Timer<>& timer) override;
			// ContentAssistant
			ContentAssistant::CompletionProposalsUI* completionProposalsUI() const BOOST_NOEXCEPT override;
			std::shared_ptr<const ContentAssistProcessor>
				contentAssistProcessor(kernel::ContentType contentType) const BOOST_NOEXCEPT override;
			void install(viewer::TextViewer& viewer) override;
			void uninstall() override;
			void viewerBoundsChanged() BOOST_NOEXCEPT override;
			// kernel.DocumentListener
			void documentAboutToBeChanged(const kernel::Document& document) override;
			void documentChanged(const kernel::Document& document, const kernel::DocumentChange& change) override;
			// viewer.Caret.MotionSignal
			void caretMoved(const viewer::Caret& caret, const kernel::Region& regionBeforeMotion);
			// viewer.Caret.CharacterInputSignal
			void characterInput(const viewer::Caret& caret, CodePoint c);
			// ContentAssistant.CompletionProposalsUI
			void close() override;
			bool complete() override;
			bool hasSelection() const BOOST_NOEXCEPT override;
			void nextPage(int pages) override;
			void nextProposal(int proposals) override;
		private:
			viewer::TextViewer* textViewer_;
			std::map<kernel::ContentType, std::shared_ptr<ContentAssistProcessor>> processors_;
			boost::chrono::milliseconds autoActivationDelay_;
			Timer<> timer_;
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
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				Gtk::ScrolledWindow
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
				???
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				QListView
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				win32::SubclassedWindow
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(X)
				???
#endif
			{
			public:
				CompletionProposalsPopup(viewer::TextViewer& parent, CompletionProposalsUI& ui);
				void end();
				void resetContent(std::shared_ptr<const CompletionProposal> proposals[], size_t numberOfProposals);
				std::shared_ptr<const CompletionProposal> selectedProposal() const;
				void selectProposal(std::shared_ptr<const CompletionProposal> selection);
				void setWritingMode(const presentation::WritingMode& writingMode);
//				bool start(const std::set<const CompletionProposal*>& proposals);
			private:
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				Gtk::TreeView view_;
				Glib::RefPtr<Gtk::ListStore> model_;
				struct ColumnRecord : Gtk::TreeModel::ColumnRecord {
					ColumnRecord();
					Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf>> icon;
					Gtk::TreeModelColumn<Glib::ustring> displayString;
					Gtk::TreeModelColumn<std::shared_ptr<const CompletionProposal>> proposalObject;
				} columns_;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				LRESULT processMessage(UINT message, WPARAM wp, LPARAM lp, bool& consumed);
				void setFont(const HFONT newFont);
				void updateDefaultFont();
				HFONT defaultFont_;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(X)
#endif
			private:
				CompletionProposalsUI& ui_;
				std::vector<std::shared_ptr<const CompletionProposal>> proposals_;
			};
			std::unique_ptr<CompletionProposalsPopup> proposalsPopup_;
			boost::signals2::connection textAreaContentRectangleChangedConnection_,
				caretMotionConnection_, caretCharacterInputConnection_, viewportScrolledConnection_;
		};
	}
} // namespace ascension.contentassist

#endif // ASCENSION_DEFAULT_CONTENT_ASSISTANT_HPP
