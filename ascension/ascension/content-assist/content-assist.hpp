/**
 * @file content-assist.hpp
 * @author exeal
 * @date 2003-2006 was CompletionWindow.h
 * @date 2006-2012
 */

#ifndef ASCENSION_CONTENT_ASSIST_HPP
#define ASCENSION_CONTENT_ASSIST_HPP

#include <ascension/platforms.hpp>
#include <ascension/corelib/timer.hpp>
#include <ascension/kernel/document-observers.hpp>
#include <ascension/kernel/partition.hpp>	// kernel.ContentType
#include <ascension/viewer/caret-observers.hpp>
#include <ascension/viewer/viewer-observers.hpp>
#include <memory>	// std.unique_ptr
#include <set>

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
			typedef
#if defined(ASCENSION_WINDOW_SYSTEM_GTK)
				GLib::RefPtr<Gdk::Pixbuf>
#elif defined(ASCENSION_WINDOW_SYSTEM_QT)
				QIcon
#elif defined(ASCENSION_WINDOW_SYSTEM_QUARTZ)
#elif defined(ASCENSION_WINDOW_SYSTEM_WIN32)
				std::shared_ptr<std::remove_pointer<HICON>::type>
#elif defined(ASCENSION_WINDOW_SYSTEM_X)
#endif
				Icon;
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
			virtual Icon icon() const /*throw()*/ = 0;
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
			virtual void replace(kernel::Document& document,
				const kernel::Region& replacementRegion) const = 0;
			/// The proposal was selected.
			virtual void selected() {}
			/// The proposal was unselected.
			virtual void unselected() {}
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
		 * An content assistant provides support on interactive content completion.
		 * @see TextViewer#getContentAssistant, TextViewer#setContentAssistant
		 */
		class ContentAssistant : public HasTimer {
		public:
			/**
			 * Represents an user interface of a completion proposal list.
			 * @see ContentAssistant#completionProposalsUI
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
/*
		class ContextInformation {};

		class ContextInformationPresenter {};

		class ContextInformationValidator {};
*/
}} // namespace ascension.contentassist

#endif // ASCENSION_CONTENT_ASSIST_HPP
