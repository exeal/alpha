/**
 * @file content-assist.hpp
 * @author exeal
 * @date 2003-2006 (was CompletionWindow.h)
 * @date 2006-2012
 */

#ifndef ASCENSION_CONTENT_ASSIST_HPP
#define ASCENSION_CONTENT_ASSIST_HPP

#include <ascension/corelib/timer.hpp>
#include <ascension/kernel/document-observers.hpp>
#include <ascension/kernel/partition.hpp>	// kernel.ContentType
#include <ascension/viewer/caret-observers.hpp>
#include <ascension/viewer/viewer-observers.hpp>
#include <ascension/win32/windows.hpp>	// win32.Handle
#include <map>
#include <memory>	// std.unique_ptr
#include <set>

// TODO: make code cross-platform.

namespace ascension {

	namespace viewers {
		class TextViewer;
	}

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
		class CompletionProposal {
		public:
			/// Destructor.
			virtual ~CompletionProposal() /*throw()*/ {}
			/// Returns the string to provide a description of the proposal. May be empty.
			virtual String description() const /*throw()*/ = 0;
			/// Returns the string to be display in the completion proposal list.
			virtual String displayString() const /*throw()*/ = 0;
			/**
			 * Returns the icon to be display in the completion proposal list. The icon would be
			 * shown to the leading of the display string.
			 * @return The icon or @c null if no image is desired
			 */
			virtual const win32::Handle<HICON>& icon() const /*throw()*/ = 0;
			/**
			 * Returns true if the proposal may be automatically inserted if the proposal is the
			 * only one. In this case, completion proposals will not displayed but the single
			 * proposal will be inserted if auto insertion is enabled.
			 */
			virtual bool isAutoInsertable() const /*throw()*/ = 0;
			/**
			 * Inserts the proposed completion into the given document.
			 * @param document The document
			 * @param replacementRegion The region to be replaced by the proposal
			 */
			virtual void replace(kernel::Document& document, const kernel::Region& replacementRegion) = 0;
			/// The proposal was selected.
			virtual void selected() {}
			/// The proposal was unselected.
			virtual void unselected() {}
		};

		/// Default implementation of @c ICompletionalProposal.
		class DefaultCompletionProposal : public CompletionProposal {
			ASCENSION_UNASSIGNABLE_TAG(DefaultCompletionProposal);
		public:
			explicit DefaultCompletionProposal(
				const String& replacementString, const String& description = String(),
				win32::Handle<HICON> icon = win32::Handle<HICON>(), bool autoInsertable = true);
			DefaultCompletionProposal(const String& replacementString,
				const String& displayString, const String& description = String(),
				win32::Handle<HICON> icon = win32::Handle<HICON>(), bool autoInsertable = true);
		public:
			String description() const /*throw()*/;
			String displayString() const /*throw()*/;
			const win32::Handle<HICON>& icon() const /*throw()*/;
			bool isAutoInsertable() const /*throw()*/;
			void replace(kernel::Document& document, const kernel::Region& replacementRegion);
		private:
			const String displayString_, replacementString_, descriptionString_;
			const win32::Handle<HICON> icon_;
			const bool autoInsertable_;
		};

		/**
		 * A content assist processor proposes completions for a particular content type.
		 * @see ContentAssistant#getContentAssistProcessor, ContentAssistant#setContentAssistProcessor
		 */
		class ContentAssistProcessor {
		public:
			/// Destructor.
			virtual ~ContentAssistProcessor() /*throw()*/ {}
			/// The completion session was closed.
			virtual void completionSessionClosed() /*throw()*/ {};
			/**
			 * Returns a list of completion proposals.
			 * @param caret The caret whose document is used to compute the proposals and has
			 *              position where the completion is active
			 * @param[out] incremental @c true if the content assistant should start an incremental
			 *                         completion. false, otherwise
			 * @param[out] replacementRegion The region to be replaced by the completion
			 * @param[out] proposals The result. If empty, the completion does not activate
			 * @see #recomputeIncrementalCompletionProposals
			 */
			virtual void computeCompletionProposals(const viewers::Caret& caret,
				bool& incremental, kernel::Region& replacementRegion, std::set<CompletionProposal*>& proposals) const = 0;
			/**
			 * Returns the proposal initially selected in the list.
			 * @param textViewer The text viewer
			 * @param replacementRegion The region to be replaced by the completion
			 * @param proposals The completion proposals listed currently. this list is sorted
			 *                  alphabetically
			 * @param numberOfProposals The number of the current proposals
			 * @return The proposal or @c null if no proposal should be selected
			 */
			virtual const CompletionProposal* activeCompletionProposal(
				const viewers::TextViewer& textViewer, const kernel::Region& replacementRegion,
				CompletionProposal* const proposals[], std::size_t numberOfProposals) const /*throw()*/ = 0;
			/**
			 * Returns @c true if the given character automatically activates the completion when
			 * the user entered.
			 * @param c The code point of the character
			 * @return true if @a c automatically activates the completion
			 */
			virtual bool isCompletionProposalAutoActivationCharacter(CodePoint c) const = 0;
			/**
			 * Returns @c true if the given character automatically terminates (completes) the
			 * active incremental completion session.
			 * @param c The code point of the character
			 * @return true if @a c automatically terminates the incremental completion
			 */
			virtual bool isIncrementalCompletionAutoTerminationCharacter(CodePoint c) const /*throw()*/ = 0;
			/**
			 * Returns a list of the running incremental completion proposals.
			 * @param textViewer The text viewer
			 * @param replacementRegion The region to be replaced by the completion
			 * @param currentProposals The completion proposals listed currently. this list is
			 *                         sorted alphabetically
			 * @param numberOfCurrentProposals The number of the current proposals
			 * @param[out] newProposals The proposals should be newly. if empty, the current
			 *                          proposals will be kept
			 * @see #computeCompletionProposals
			 */
			virtual void recomputeIncrementalCompletionProposals(const viewers::TextViewer& textViewer,
				const kernel::Region& replacementRegion, CompletionProposal* const currentProposals[],
				std::size_t numberOfCurrentProposals, std::set<CompletionProposal*>& newProposals) const = 0;
		};

		/**
		 * An abstract implementation of @c IContentAssistProcessor builds completion proposals by
		 * collecting identifiers in the document.
		 */
		class IdentifiersProposalProcessor : public ContentAssistProcessor {
			ASCENSION_UNASSIGNABLE_TAG(IdentifiersProposalProcessor);
		protected:
			// constructors
			IdentifiersProposalProcessor(kernel::ContentType contentType, const text::IdentifierSyntax& syntax) /*throw()*/;
			virtual ~IdentifiersProposalProcessor() /*throw()*/;
			// attributes
			kernel::ContentType contentType() const /*throw()*/;
			const text::IdentifierSyntax& identifierSyntax() const /*throw()*/;
			// IContentAssistProcessor
			virtual void computeCompletionProposals(const viewers::Caret& caret, bool& incremental,
				kernel::Region& replacementRegion, std::set<CompletionProposal*>& proposals) const;
			virtual const CompletionProposal* getActiveCompletionProposal(
				const viewers::TextViewer& textViewer, const kernel::Region& replacementRegion,
				CompletionProposal* const proposals[], std::size_t numberOfProposals) const /*throw()*/;
			virtual bool isIncrementalCompletionAutoTerminationCharacter(CodePoint c) const /*throw()*/;
			virtual void recomputeIncrementalCompletionProposals(const viewers::TextViewer& textViewer,
				const kernel::Region& replacementRegion, CompletionProposal* const currentProposals[],
				std::size_t numberOfCurrentProposals, std::set<CompletionProposal*>& newProposals) const;
		private:
			const kernel::ContentType contentType_;
			const text::IdentifierSyntax& syntax_;
		};

		/**
		 * An content assistant provides support on interactive content completion.
		 * @see TextViewer#getContentAssistant, TextViewer#setContentAssistant
		 */
		class ContentAssistant : public HasTimer {
		public:
			/**
			 * Represents an user interface of a completion proposal list.
			 * @see IContentAssistant#getCompletionProposalsUI
			 */
			class CompletionProposalsUI {
			public:
				/// Closes the list without completion.
				virtual void close() = 0;
				/// Completes and closes. Returns true if the completion was succeeded.
				virtual bool complete() = 0;
				/// Returns true if the list has a selection.
				virtual bool hasSelection() const /*throw()*/ = 0;
				/// Selects the proposal in the next/previous page.
				virtual void nextPage(int pages) = 0;
				/// Selects the next/previous proposal.
				virtual void nextProposal(int proposals) = 0;
			protected:
				/// Destructor.
				virtual ~CompletionProposalsUI() /*throw()*/ {}
			};
			/// Destructor.
			virtual ~ContentAssistant() /*throw()*/ {}
			/// Returns the user interface of the completion proposal list or @c null.
			virtual CompletionProposalsUI* completionProposalsUI() const /*throw()*/ = 0;
			/**
			 * Returns the content assist processor to be used for the specified content type.
			 * @param contentType The content type
			 * @return The content assist processor or @c null if none corresponds to
			 *         @a contentType
			 */
			virtual const ContentAssistProcessor* contentAssistProcessor(kernel::ContentType contentType) const /*throw()*/ = 0;
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
			void setAutoActivationDelay(unsigned long milliseconds);
			void setContentAssistProcessor(kernel::ContentType contentType, std::unique_ptr<ContentAssistProcessor> processor);
			// operation
			void showPossibleCompletions();
		private:
			void startPopup();
			static void CALLBACK timeElapsed(HWND, UINT, UINT_PTR eventID, DWORD);
			void updatePopupPositions();
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
			class CompletionProposalPopup;
			CompletionProposalPopup* proposalPopup_;
			uint32_t autoActivationDelay_;
			static std::map<UINT_PTR, ContentAssistant*> timerIDs_;
			struct CompletionSession {
				const ContentAssistProcessor* processor;
				bool incremental;
				kernel::Region replacementRegion;
				std::unique_ptr<CompletionProposal*[]> proposals;
				std::size_t numberOfProposals;
				CompletionSession() /*throw()*/ : processor(nullptr), numberOfProposals(0) {}
				~CompletionSession() /*throw()*/ {for(std::size_t i = 0; i < numberOfProposals; ++i) delete proposals[i];}
			};
			std::unique_ptr<CompletionSession> completionSession_;
		};
/*
		class ContextInformation {};

		class ContextInformationPresenter {};

		class ContextInformationValidator {};
*/
}} // namespace ascension.contentassist

#endif // ASCENSION_CONTENT_ASSIST_HPP
