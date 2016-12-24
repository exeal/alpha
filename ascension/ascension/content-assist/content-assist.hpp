/**
 * @file content-assist.hpp
 * @author exeal
 * @date 2003-2006 was CompletionWindow.h
 * @date 2006-2013, 2015
 */

#ifndef ASCENSION_CONTENT_ASSIST_HPP
#define ASCENSION_CONTENT_ASSIST_HPP
#include <ascension/platforms.hpp>
#include <ascension/corelib/text/code-point.hpp>
#include <ascension/kernel/content-type.hpp>
#include <ascension/kernel/document-observers.hpp>
#include <memory>	// std.shared_ptr
#include <set>
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
#	include <gdkmm/pixbuf.h>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
#	include <ascension/win32/handle.hpp>
#endif

namespace ascension {
	namespace kernel {
		class Region;
	}

	namespace viewer {
		class Caret;
		class TextViewer;
	}

	/**
	 * Provides a content assist feature for a @c viewer#TextViewer. Content assist supports the
	 * user in writing by proposing completions at a given document position.
	 */
	namespace contentassist {
		/**
		 * A completion proposal contains a string and an icon to display itself in the proposal
		 * list, and insert the completion into the given document.
		 */
		class CompletionProposal {
		public:
			typedef
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				Glib::RefPtr<Gdk::Pixbuf>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				QIcon
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				win32::Handle<HICON>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(X)
#endif
				Icon;
		public:
			/// Destructor.
			virtual ~CompletionProposal() BOOST_NOEXCEPT {}
			/// Returns the string to provide a description of the proposal. May be empty.
			virtual String description() const BOOST_NOEXCEPT = 0;
			/// Returns the string to be display in the completion proposal list.
			virtual String displayString() const BOOST_NOEXCEPT = 0;
			/**
			 * Returns the icon to be display in the completion proposal list. The icon would be
			 * shown to the leading of the display string.
			 * @return The icon or @c null if no image is desired
			 */
			virtual const Icon& icon() const BOOST_NOEXCEPT = 0;
			/**
			 * Returns true if the proposal may be automatically inserted if the proposal is the
			 * only one. In this case, completion proposals will not displayed but the single
			 * proposal will be inserted if auto insertion is enabled.
			 */
			virtual bool isAutoInsertable() const BOOST_NOEXCEPT = 0;
			/**
			 * Inserts the proposed completion into the given document.
			 * @param document The document
			 * @param replacementRegion The region to be replaced by the proposal
			 */
			virtual void replace(kernel::Document& document,
				const kernel::Region& replacementRegion) const = 0;
			/// The proposal was selected.
			virtual void selected() const {}
			/// The proposal was unselected.
			virtual void unselected() const {}
		};

		/**
		 * A content assist processor proposes completions for a particular content type.
		 * @see DefaultContentAssistant#contentAssistProcessor,
		 *      DefaultContentAssistant#setContentAssistProcessor
		 */
		class ContentAssistProcessor {
		public:
			/// Destructor.
			virtual ~ContentAssistProcessor() BOOST_NOEXCEPT {}
			/**
			 * Returns the proposal initially selected in the list.
			 * @param textViewer The text viewer
			 * @param replacementRegion The region to be replaced by the completion
			 * @param proposals The completion proposals listed currently. this list is sorted
			 *                  alphabetically
			 * @param numberOfProposals The number of the current proposals
			 * @return The proposal or @c null if no proposal should be selected
			 */
			virtual std::shared_ptr<const CompletionProposal> activeCompletionProposal(
				const viewer::TextViewer& textViewer, const kernel::Region& replacementRegion,
				std::shared_ptr<const CompletionProposal> proposals[], std::size_t numberOfProposals) const BOOST_NOEXCEPT = 0;
			/**
			 * Compares the given two display strings.
			 * @param s1, s2 The display strings to compare
			 * @return true if @a s1 &lt; @a s2
			 */
			virtual bool compareDisplayStrings(const String& s1, const String& s2) const BOOST_NOEXCEPT = 0;
			/// The completion session was closed.
			virtual void completionSessionClosed() BOOST_NOEXCEPT {};
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
			virtual void computeCompletionProposals(const viewer::Caret& caret,
				bool& incremental, kernel::Region& replacementRegion,
				std::set<std::shared_ptr<const CompletionProposal>>& proposals) const = 0;
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
			virtual bool isIncrementalCompletionAutoTerminationCharacter(CodePoint c) const BOOST_NOEXCEPT = 0;
			/**
			 * Returns a list of the running incremental completion proposals.
			 * @param textViewer The text viewer
			 * @param replacementRegion The region to be replaced by the completion
			 * @param currentProposals The completion proposals listed currently. This list is
			 *                         sorted alphabetically
			 * @param numberOfCurrentProposals The number of the current proposals
			 * @param[out] newProposals The proposals should be newly. If empty, the current
			 *                          proposals will be kept
			 * @see #computeCompletionProposals
			 */
			virtual void recomputeIncrementalCompletionProposals(
				const viewer::TextViewer& textViewer, const kernel::Region& replacementRegion,
				std::shared_ptr<const CompletionProposal> currentProposals[], std::size_t numberOfCurrentProposals,
				std::set<std::shared_ptr<const CompletionProposal>>& newProposals) const = 0;
		};

		/**
		 * An content assistant provides support on interactive content completion.
		 * @see TextViewer#contentAssistant, TextViewer#setContentAssistant
		 */
		class ContentAssistant {
		public:
			/**
			 * Represents an user interface of a completion proposal list.
			 * @see ContentAssistant#completionProposalsUI
			 */
			class CompletionProposalsUI {
			public:
				/// Closes the list without completion.
				virtual void close() = 0;
				/// Completes and closes. Returns @c true if the completion was succeeded.
				virtual bool complete() = 0;
				/// Returns true if the list has a selection.
				virtual bool hasSelection() const BOOST_NOEXCEPT = 0;
				/// Selects the proposal in the next/previous page.
				virtual void nextPage(int pages) = 0;
				/// Selects the next/previous proposal.
				virtual void nextProposal(int proposals) = 0;
			protected:
				/// Destructor.
				virtual ~CompletionProposalsUI() BOOST_NOEXCEPT {}
			};
			/// Destructor.
			virtual ~ContentAssistant() BOOST_NOEXCEPT {}
			/// Returns the user interface of the completion proposal list or @c null.
			virtual CompletionProposalsUI* completionProposalsUI() const BOOST_NOEXCEPT = 0;
			/**
			 * Returns the content assist processor to be used for the specified content type.
			 * @param contentType The content type
			 * @return The content assist processor or @c null if none corresponds to
			 *         @a contentType
			 */
			virtual std::shared_ptr<const ContentAssistProcessor>
				contentAssistProcessor(const kernel::ContentType& contentType) const BOOST_NOEXCEPT = 0;
			/// Shows all possible completions on the current context.
			virtual void showPossibleCompletions() = 0;
		protected:
			/// Installs the content assistant on the specified text viewer.
			virtual void install(viewer::TextViewer& viewer) = 0;
			/// Uninstalls the content assistant from the text viewer.
			virtual void uninstall() = 0;
			/// The bounds of the text viewer was changed.
			virtual void viewerBoundsChanged() {}
			friend class viewer::TextViewer;
		};
/*
		class ContextInformation {};

		class ContextInformationPresenter {};

		class ContextInformationValidator {};
*/
}} // namespace ascension.contentassist

#endif // ASCENSION_CONTENT_ASSIST_HPP
