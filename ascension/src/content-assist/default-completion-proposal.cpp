/**
 * @file default-completion-proposal.cpp
 * @author exeal
 * @date 2003-2006 was CompletionWindow.cpp
 * @date 2006-2012 was content-assist.cpp
 * @date 2012-03-12 separated from content-assist.cpp
 */

#include <ascension/content-assist/default-completion-proposal.hpp>
#include <ascension/kernel/document.hpp>

using namespace ascension;
using namespace ascension::contentassist;
using namespace ascension::kernel;
using namespace std;


// DefaultCompletionProposal //////////////////////////////////////////////////////////////////////

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
		displayString_(replacementString), replacementString_(replacementString), icon_(icon),
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
		displayString_(displayString), replacementString_(replacementString), icon_(icon),
		descriptionString_(description), autoInsertable_(autoInsertable) {
}

/// @see CompletionProposal#description
String DefaultCompletionProposal::description() const /*throw()*/ {
	return descriptionString_;
}

/// @see CompletionProposal#displayString
String DefaultCompletionProposal::displayString() const /*throw()*/ {
	return displayString_;
}

/// @see CompletionProposal#icon
CompletionProposal::Icon DefaultCompletionProposal::icon() const /*throw()*/ {
	return icon_;
}

/// @see CompletionProposal#isAutoInsertable
bool DefaultCompletionProposal::isAutoInsertable() const /*throw()*/ {
	return autoInsertable_;
}

/**
 * Implements @c CompletionProposal#replace.
 * This method may throw any exceptions @c kernel#Document#replace throws other than
 * @c kernel#ReadOnlyDocumentException.
 */
void DefaultCompletionProposal::replace(Document& document, const Region& replacementRegion) const {
	if(!document.isReadOnly()) {
		document.insertUndoBoundary();
		document.replace(replacementRegion, replacementString_);
		document.insertUndoBoundary();
	}
}
