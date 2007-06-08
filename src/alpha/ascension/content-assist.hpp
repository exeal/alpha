/**
 * @file content-assist.hpp
 * @author exeal
 * @date 2003-2006 (was CompletionWindow.h)
 * @date 2006-2007
 */

#ifndef ASCENSION_CONTENT_ASSIST_HPP
#define ASCENSION_CONTENT_ASSIST_HPP
#include "document.hpp"
#include "../../manah/win32/ui/standard-controls.hpp"	// manah.windows.ui.ListBox
#include <set>


namespace ascension {

	namespace viewers {class SourceViewer;}

	/**
	 * Provides a content assist feature for a @c viewers#SourceViewer. Content assist supports the
	 * user in writing by proposing completions at a given document position.
	 */
	namespace contentassist {

		/**
		 * A completion proposal contains a string and an icon to display itself in the proposal
		 * list, and insert the completion into the given document.
		 * @see CompletionProposal
		 */
		class ICompletionProposal {
		public:
			/// Returns the string to be display in the completion proposal list.
			virtual String getDisplayString() const throw() = 0;
			/**
			 * Returns the icon to be display in the completion proposal list. The icon would be
			 * shown to the leading of the display string.
			 * @return the icon or @c null if no image is desired
			 */
			virtual HICON getIcon() const throw() = 0;
			/**
			 * Returns true if the proposal may be automatically inserted if the proposal is the
			 * only one. In this case, completion proposals will not displayed but the single
			 * proposal will be inserted if auto insertion is enabled.
			 */
			virtual bool isAutoInsertable() const throw() = 0;
			/**
			 * Inserts the proposed completion into the given document.
			 * @param document the document
			 */
			virtual void replace(text::Document& document) = 0;
			/// The proposal was selected.
			virtual void selected() {}
			/// The proposal was unselected.
			virtual void unselected() {}
		};

		/// Default implementation of @c ICompletionalProposal.
		class CompletionProposal : virtual public ICompletionProposal {
		public:
			CompletionProposal(const String& replacementString,
				const text::Region& replacementRegion, HICON icon = 0, bool autoInsertable = true);
			CompletionProposal(const String& replacementString,
				const text::Region& replacementRegion, const String& displayString, HICON icon = 0, bool autoInsertable = true);
		public:
			String	getDisplayString() const throw();
			HICON	getIcon() const throw();
			bool	isAutoInsertable() const throw();
			void	replace(text::Document& document);
		private:
			const String displayString_, replacementString_;
			HICON icon_;
			const text::Region replacementRegion_;
			const bool autoInsertable_;
		};

		/**
		 * @see IContentAssistant#addCompletionListener, IContentAssistant#removeCompletionListener
		 */
		class ICompletionListener {
		public:
			/**
			 * Content assist was ended.
			 * @param true if the content assist was aborted. false if completed
			 */
			virtual void completionSessionEnded(bool aborted) = 0;
			/// Invoked content assist.
			virtual void completionSessionStarted() = 0;
			/**
			 * The selection in the proposal list was changed.
			 * @param proposal the newly selected proposal or @c null
			 */
			virtual void completionSelectionChanged(const ICompletionProposal* proposal) = 0;
		};

		/**
		 * A content assist processor proposes completions for a particular content type.
		 * @see ContentAssistant#getContentAssistProcessor, ContentAssistant#setContentAssistProcessor
		 */
		class IContentAssistProcessor {
		public:
			/**
			 * Returns a list of completion proposals.
			 * @param viewer the source viewer whose document is used to compute the proposals
			 * @param position the document position where the completion is active
			 * @param[out] proposals the result
			 */
			virtual void computeCompletionProposals(const viewers::SourceViewer& viewer,
				const text::Position& position, std::set<ICompletionProposal*>& proposals) const = 0;
			/**
			 * Returns the characters which when entered by the user should automatically activate
			 * the completion.
			 * @return the characters
			 */
			virtual String getCompletionProposalAutoActivationCharacters() const throw() = 0;
		};

		/**
		 * An content assistant provides support on interactive content completion.
		 * @see SourceViewer#getContentAssistant, SourceViewer#setContentAssistant
		 */
		class IContentAssistant {
		public:
			/// Adds the new completion listener.
			virtual void addCompletionListener(ICompletionListener& listener) = 0;
			/**
			 * Returns the content assist processor to be used for the specified content type.
			 * @param contentType the content type
			 * @return the content assist processor or @c null if none corresponds to @p contentType
			 */
			virtual const IContentAssistProcessor* getContentAssistProcessor(text::ContentType contentType) const throw() = 0;
			/// Removes the given completion listener.
			virtual void removeCompletionListener(ICompletionListener& listener) = 0;
			/// Shows all possible completions on the current context.
			virtual void showPossibleCompletions() = 0;
		protected:
			/// Installs the content assistant on the specified source viewer.
			virtual void install(viewers::SourceViewer& viewer) = 0;
			/// Uninstalls the content assistant from the text viewer.
			virtual void uninstall() = 0;
			friend class viewers::SourceViewer;
		};

		/**
		 * Default implementation of @c IContentAssistant.
		 * @note This class is not intended to be subclassed.
		 */
		class ContentAssistant : virtual public IContentAssistant {
		public:
			// constructors
			ContentAssistant() throw();
			~ContentAssistant() throw();
			// attributes
			void							enablePrefixCompletion(bool enable);
			const IContentAssistProcessor*	getContentAssistProcessor(text::ContentType contentType) const throw();
			void							setAutoActivationDelay(ulong milliseconds);
			void							setContentAssistProcessor(text::ContentType contentType,
												std::auto_ptr<IContentAssistProcessor> processor);
			// listeners
			void	addCompletionListener(ICompletionListener& listener);
			void	removeCompletionListener(ICompletionListener& listener);
			// operation
			void	showPossibleCompletions();
		private:
			void	install(viewers::SourceViewer& viewer);
			void	uninstall();
		private:
			viewers::SourceViewer* sourceViewer_;
			std::map<text::ContentType, IContentAssistProcessor*> processors_;
			ascension::internal::Listeners<ICompletionListener> completionListeners_;
			class CompletionProposalPopup;
			CompletionProposalPopup* proposalPopup_;
		};
/*
		class IContextInformation {};

		class IContextInformationPresenter {};

		class IContextInformationValidator {};
*/
}} // namespace ascension.contentassist

#endif /* ASCENSION_CONTENT_ASSIST_HPP */
