/**
 * @file default-completion-proposal.cpp
 * @author exeal
 * @date 2003-2006 was CompletionWindow.cpp
 * @date 2006-2012 was content-assist.cpp
 * @date 2012-03-12 separated from content-assist.cpp
 * @date 2014
 */

#include <ascension/content-assist/default-completion-proposal.hpp>
#include <ascension/kernel/document.hpp>


namespace ascension {
	namespace contentassist {
		// DefaultCompletionProposal //////////////////////////////////////////////////////////////////////////////////

		/**
		 * Constructor.
		 * @param replacementString The actual string to be inserted into the document
		 * @param description The description of the proposal
		 * @param icon The icon to display for the proposal
		 * @param autoInsertable Set @c true to enable auto insertion for the proposal
		 */
		DefaultCompletionProposal::DefaultCompletionProposal(
				const String& replacementString, const String& description /* = String() */,
				Icon&& icon /* = Icon() */, bool autoInsertable /* = true */) :
				displayString_(replacementString), replacementString_(replacementString), icon_(std::move(icon)),
				descriptionString_(description), autoInsertable_(autoInsertable) {
		}

		/**
		 * Constructor.
		 * @param replacementString The actual string to be inserted into the document
		 * @param displayString The string to display for the proposal
		 * @param description The description of the proposal
		 * @param icon The icon to display for the proposal
		 * @param autoInsertable Set @c true to enable auto insertion for the proposal
		 */
		DefaultCompletionProposal::DefaultCompletionProposal(const String& replacementString,
				const String& displayString, const String& description /* = String() */,
				Icon&& icon /* = Icon() */, bool autoInsertable /* = true */) :
				displayString_(displayString), replacementString_(replacementString), icon_(std::move(icon)),
				descriptionString_(description), autoInsertable_(autoInsertable) {
		}

		/// @see CompletionProposal#description
		String DefaultCompletionProposal::description() const BOOST_NOEXCEPT {
			return descriptionString_;
		}

		/// @see CompletionProposal#displayString
		String DefaultCompletionProposal::displayString() const BOOST_NOEXCEPT {
			return displayString_;
		}

		/// @see CompletionProposal#icon
		const CompletionProposal::Icon& DefaultCompletionProposal::icon() const BOOST_NOEXCEPT {
			return icon_;
		}

		/// @see CompletionProposal#isAutoInsertable
		bool DefaultCompletionProposal::isAutoInsertable() const BOOST_NOEXCEPT {
			return autoInsertable_;
		}

		/**
		 * Implements @c CompletionProposal#replace.
		 * This method may throw any exceptions @c kernel#Document#replace throws other than
		 * @c kernel#ReadOnlyDocumentException.
		 */
		void DefaultCompletionProposal::replace(kernel::Document& document, const kernel::Region& replacementRegion) const {
			if(!document.isReadOnly()) {
				document.insertUndoBoundary();
				document.replace(replacementRegion, replacementString_);
				document.insertUndoBoundary();
			}
		}
	}
}
