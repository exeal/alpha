/**
 * @file content-assist.hpp
 * @author exeal
 * @date 2003-2006 (was CompletionWindow.h)
 * @date 2006-2007
 */

#ifndef ASCENSION_CONTENT_ASSIST_HPP
#define ASCENSION_CONTENT_ASSIST_HPP
#include "point.hpp"
#include <set>


namespace ascension {

//	namespace viewers {class TextViewer;}

	/**
	 * Provides a content assist feature for a @c viewers#TextViewer. Content assist supports the
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
			/// Destructor.
			virtual ~ICompletionProposal() throw() {}
			/// Returns the string to provide a description of the proposal. May be empty.
			virtual String getDescription() const throw() = 0;
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
			 * @param replacementRegion the region to be replaced by the proposal
			 */
			virtual void replace(kernel::Document& document, const kernel::Region& replacementRegion) = 0;
			/// The proposal was selected.
			virtual void selected() {}
			/// The proposal was unselected.
			virtual void unselected() {}
		};

		/// Default implementation of @c ICompletionalProposal.
		class CompletionProposal : virtual public ICompletionProposal {
			MANAH_UNASSIGNABLE_TAG(CompletionProposal);
		public:
			explicit CompletionProposal(const String& replacementString,
				const String& description = L"", HICON icon = 0, bool autoInsertable = true);
			CompletionProposal(const String& replacementString, const String& displayString,
				const String& description = L"", HICON icon = 0, bool autoInsertable = true);
		public:
			String	getDescription() const throw();
			String	getDisplayString() const throw();
			HICON	getIcon() const throw();
			bool	isAutoInsertable() const throw();
			void	replace(kernel::Document& document, const kernel::Region& replacementRegion);
		private:
			const String displayString_, replacementString_, descriptionString_;
			::HICON icon_;
			const bool autoInsertable_;
		};

		/**
		 * A content assist processor proposes completions for a particular content type.
		 * @see ContentAssistant#getContentAssistProcessor, ContentAssistant#setContentAssistProcessor
		 */
		class IContentAssistProcessor {
		public:
			/// Destructor.
			virtual ~IContentAssistProcessor() throw() {}
			/// The completion session was closed.
			virtual void completionSessionClosed() throw() {};
			/**
			 * Returns a list of completion proposals.
			 * @param caret the caret whose document is used to compute the proposals and has
			 * position where the completion is active
			 * @param[out] incremental true if the content assistant should start an incremental
			 * completion. false, otherwise
			 * @param[out] replacementRegion the region to be replaced by the completion
			 * @param[out] proposals the result. if empty, the completion does not activate
			 * @see #recomputeIncrementalCompletionProposals
			 */
			virtual void computeCompletionProposals(const viewers::Caret& caret,
				bool& incremental, kernel::Region& replacementRegion, std::set<ICompletionProposal*>& proposals) const = 0;
			/**
			 * Returns the proposal initially selected in the list.
			 * @param textViewer the text viewer
			 * @param replacementRegion the region to be replaced by the completion
			 * @param proposals the completion proposals listed currently. this list is sorted
			 * alphabetically
			 * @param numberOfProposals the number of the current proposals
			 * @return the proposal or @c null if no proposal should be selected
			 */
			virtual const ICompletionProposal* getActiveCompletionProposal(
				const viewers::TextViewer& textViewer, const kernel::Region& replacementRegion,
				ICompletionProposal* const proposals[], std::size_t numberOfProposals) const throw() = 0;
			/**
			 * Returns true if the given character automatically activates the completion when the
			 * user entered.
			 * @param c the code point of the character
			 * @return true if @a c automatically activates the completion
			 */
			virtual bool isCompletionProposalAutoActivationCharacter(CodePoint c) const throw() = 0;
			/**
			 * Returns true if the given character automatically terminates (completes) the active
			 * incremental completion session.
			 * @param c the code point of the character
			 * @return true if @a c automatically terminates the incremental completion
			 */
			virtual bool isIncrementalCompletionAutoTerminationCharacter(CodePoint c) const throw() = 0;
			/**
			 * Returns a list of the running incremental completion proposals.
			 * @param textViewer the text viewer
			 * @param replacementRegion the region to be replaced by the completion
			 * @param currentProposals the completion proposals listed currently. this list is
			 * sorted alphabetically
			 * @param numberOfCurrentProposals the number of the current proposals
			 * @param[out] newProposals the proposals should be newly. if empty, the current
			 * proposals will be kept
			 * @see #computeCompletionProposals
			 */
			virtual void recomputeIncrementalCompletionProposals(const viewers::TextViewer& textViewer,
				const kernel::Region& replacementRegion, ICompletionProposal* const currentProposals[],
				std::size_t numberOfCurrentProposals, std::set<ICompletionProposal*>& newProposals) const = 0;
		};

		/**
		 * An abstract implementation of @c IContentAssistProcessor builds completion proposals by
		 * collecting identifiers in the document.
		 */
		class IdentifiersProposalProcessor : virtual public IContentAssistProcessor {
			MANAH_UNASSIGNABLE_TAG(IdentifiersProposalProcessor);
		protected:
			// constructors
			IdentifiersProposalProcessor(kernel::ContentType contentType, const unicode::IdentifierSyntax& syntax) throw();
			virtual ~IdentifiersProposalProcessor() throw();
			// attributes
			kernel::ContentType contentType() const throw();
			const unicode::IdentifierSyntax& identifierSyntax() const throw();
			// IContentAssistProcessor
			virtual void computeCompletionProposals(const viewers::Caret& caret, bool& incremental,
				kernel::Region& replacementRegion, std::set<ICompletionProposal*>& proposals) const;
			virtual const ICompletionProposal* getActiveCompletionProposal(
				const viewers::TextViewer& textViewer, const kernel::Region& replacementRegion,
				ICompletionProposal* const proposals[], std::size_t numberOfProposals) const throw();
			virtual bool isIncrementalCompletionAutoTerminationCharacter(CodePoint c) const throw();
			virtual void recomputeIncrementalCompletionProposals(const viewers::TextViewer& textViewer,
				const kernel::Region& replacementRegion, ICompletionProposal* const currentProposals[],
				std::size_t numberOfCurrentProposals, std::set<ICompletionProposal*>& newProposals) const;
		private:
			const kernel::ContentType contentType_;
			const unicode::IdentifierSyntax& syntax_;
		};

		/**
		 * An content assistant provides support on interactive content completion.
		 * @see TextViewer#getContentAssistant, TextViewer#setContentAssistant
		 */
		class IContentAssistant {
		public:
			/**
			 * Represents an user interface of a completion proposal list.
			 * @see IContentAssistant#getCompletionProposalsUI
			 */
			class ICompletionProposalsUI {
			public:
				/// Closes the list without completion.
				virtual void close() = 0;
				/// Completes and closes. Returns true if the completion was succeeded.
				virtual bool complete() = 0;
				/// Returns true if the list has a selection.
				virtual bool hasSelection() const throw() = 0;
				/// Selects the proposal in the next/previous page.
				virtual void nextPage(int pages) = 0;
				/// Selects the next/previous proposal.
				virtual void nextProposal(int proposals) = 0;
			protected:
				/// Destructor.
				virtual ~ICompletionProposalsUI() throw() {}
			};
			/// Destructor.
			virtual ~IContentAssistant() throw() {}
			/// Returns the user interface of the completion proposal list or @c null.
			virtual ICompletionProposalsUI* getCompletionProposalsUI() const throw() = 0;
			/**
			 * Returns the content assist processor to be used for the specified content type.
			 * @param contentType the content type
			 * @return the content assist processor or @c null if none corresponds to @a contentType
			 */
			virtual const IContentAssistProcessor* getContentAssistProcessor(kernel::ContentType contentType) const throw() = 0;
			/// Shows all possible completions on the current context.
			virtual void showPossibleCompletions() = 0;
		protected:
			/// Installs the content assistant on the specified text viewer.
			virtual void install(viewers::TextViewer& viewer) = 0;
			/// Uninstalls the content assistant from the text viewer.
			virtual void uninstall() = 0;
			friend class viewers::TextViewer;
		};

		/**
		 * Default implementation of @c IContentAssistant.
		 * @note This class is not intended to be subclassed.
		 */
		class ContentAssistant :
			virtual public IContentAssistant,
			virtual public kernel::IDocumentListener,
			virtual public viewers::ICaretListener,
			virtual public viewers::ICharacterInputListener,
			virtual public viewers::IViewportListener,
			virtual private IContentAssistant::ICompletionProposalsUI {
		public:
			// constructors
			ContentAssistant() throw();
			~ContentAssistant() throw();
			// attributes
			ulong	autoActivationDelay() const throw();
			void	enablePrefixCompletion(bool enable);
			void	setAutoActivationDelay(ulong milliseconds);
			void	setContentAssistProcessor(kernel::ContentType contentType, std::auto_ptr<IContentAssistProcessor> processor);
			// operation
			void	showPossibleCompletions();
		private:
			void					startPopup();
			static void CALLBACK	timeElapsed(HWND, UINT, ::UINT_PTR eventID, DWORD);
			void					updatePopupPositions();
			// IContentAssistant
			ICompletionProposalsUI*			getCompletionProposalsUI() const throw();
			const IContentAssistProcessor*	getContentAssistProcessor(kernel::ContentType contentType) const throw();
			void							install(viewers::TextViewer& viewer);
			void							uninstall();
			// kernel.IDocumentListener
			bool	documentAboutToBeChanged(const kernel::Document& document, const kernel::DocumentChange& change);
			void	documentChanged(const kernel::Document& document, const kernel::DocumentChange& change);
			// viewers.ICaretListener
			void	caretMoved(const viewers::Caret& self, const kernel::Region& oldRegion);
			// viewers.ICharacterInputListener
			void	characterInputted(const viewers::Caret& self, CodePoint c);
			// viewers.IViewportListener
			void	viewportChanged(bool horizontal, bool vertical);
			// IContentAssistant.ICompletionProposalsUI
			void	close();
			bool	complete();
			bool	hasSelection() const throw();
			void	nextPage(int pages);
			void	nextProposal(int proposals);
		private:
			viewers::TextViewer* textViewer_;
			std::map<kernel::ContentType, IContentAssistProcessor*> processors_;
			class CompletionProposalPopup;
			CompletionProposalPopup* proposalPopup_;
			ulong autoActivationDelay_;
			static std::map<::UINT_PTR, ContentAssistant*> timerIDs_;
			struct CompletionSession {
				const IContentAssistProcessor* processor;
				bool incremental;
				kernel::Region replacementRegion;
				manah::AutoBuffer<ICompletionProposal*> proposals;
				std::size_t numberOfProposals;
				CompletionSession() throw() : processor(0), numberOfProposals(0) {}
				~CompletionSession() throw() {for(std::size_t i = 0; i < numberOfProposals; ++i) delete proposals[i];}
			};
			std::auto_ptr<CompletionSession> completionSession_;
		};
/*
		class IContextInformation {};

		class IContextInformationPresenter {};

		class IContextInformationValidator {};
*/
}} // namespace ascension.contentassist

#endif /* ASCENSION_CONTENT_ASSIST_HPP */
