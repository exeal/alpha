/**
 * @file content-assist.hpp
 * @author exeal
 * @date 2003-2006 (was CompletionWindow.h)
 * @date 2006-2007
 */

#ifndef ASCENSION_CONTENT_ASSIST_HPP
#define ASCENSION_CONTENT_ASSIST_HPP
#include "document.hpp"
#include "../../manah/win32/ui/standard-controls.hpp"	// manah::windows::ui::ListBox
#include <set>


namespace ascension {

	namespace viewers {
		class TextViewer;
		class SourceViewer;
		class VisualPoint;
	}

	/**
	 * Provides a content assist feature for a @c viewers#TextViewer. Content assist supports the
	 * user in writing by proposing completions at a given document position.
	 */
	namespace contentassist {

		class ICompletionListener {};

		class ICompletionProposal {
		public:
			virtual String getDisplayString() const throw() = 0;
			virtual HICON getIcon() const throw() = 0;
			virtual text::Region getSelection(const text::Document& document) const throw() = 0;
			virtual void replace(text::Document& document) = 0;
		};

		/**
		 * A content assist processor proposes completions for a particular content type.
		 * @see ContentAssistant#getContentAssistProcessor, ContentAssistant#setContentAssistProcessor
		 */
		class IContentAssistProcessor {
		public:
			/**
			 * Returns a list of completion proposals.
			 * @param viewer the viewer whose document is used to compute the proposals
			 * @param position the document position where the completion is active
			 * @param[out] proposals the result
			 */
			virtual void computeCompletionProposals(const viewers::TextViewer& viewer,
				const text::Position& position, std::vector<ICompletionProposal*>& proposals) const = 0;
			/**
			 * Returns the characters which when entered by the user should automatically activate
			 * the completion.
			 * @return the characters
			 */
			virtual String getCompletionProposalAutoActivationCharacters() const throw() = 0;
		};

		/// An content assistant provides support on interactive content completion.
		class ContentAssistant {
		public:
			// attributes
			void							enableAutoInsert(bool enable);
			void							enablePrefixCompletion(bool enable);
			const IContentAssistProcessor*	getContentAssistProcessor(text::ContentType contentType) const throw();
			void							setAutoActivationDelay(ulong milliseconds);
			void							setContentAssistProcessor(text::ContentType contentType, IContentAssistProcessor* processor);
			// listeners
			void	addCompletionListener(ICompletionListener& listener);
			void	removeCompletionListener(ICompletionListener& listener);
			// operation
			void	showPossibleCompletions();
		private:
			std::map<text::ContentType, IContentAssistProcessor*> processors_;
			ascension::internal::Listeners<ICompletionListener> completionListeners_;
		};
/*
		class IContextInformation {};

		class IContextInformationPresenter {};

		class IContextInformationValidator {};
*/
		/// A completion window.
		class CompletionWindow : public manah::win32::ui::ListBox {
		public:
			// constructors
			explicit CompletionWindow(viewers::SourceViewer& viewer);
			virtual ~CompletionWindow();
			// construction
			bool	create();
			// attributes
			text::Region	getContextRegion() const;
			bool			isRunning() const throw();
			void			setFont(const HFONT font);
			// operations
			void	abort();
			void	complete();
			bool	start(const std::set<String>& candidateWords);
			bool	updateListCursel();

		protected:
			virtual LRESULT	preTranslateWindowMessage(UINT message, WPARAM wParam, LPARAM lParam, bool& handled);
		private:
			void	updateDefaultFont();

		private:
			viewers::SourceViewer& viewer_;
			HFONT defaultFont_;
			bool running_;
			text::Position contextStart_;		// 補完開始位置の前方の単語先頭
			viewers::VisualPoint* contextEnd_;	// 補完開始位置の後方の単語終端
		};


		/// Returns if the completion is running.
		inline bool CompletionWindow::isRunning() const throw() {return running_;}

}} // namespace ascension.contentassist

#endif /* ASCENSION_CONTENT_ASSIST_HPP */
