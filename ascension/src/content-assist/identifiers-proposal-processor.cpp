/**
 * @file identifiers-proposal-processor.cpp
 * @author exeal
 * @date 2003-2006 was CompletionWindow.cpp
 * @date 2006-2012 was content-assist.cpp
 * @date 2012-03-12 separated from content-assist.cpp
 */

#include <ascension/content-assist/default-completion-proposal.hpp>
#include <ascension/content-assist/identifiers-proposal-processor.hpp>
#include <ascension/corelib/text/case-folder.hpp>
#include <ascension/kernel/document-character-iterator.hpp>
#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/viewer.hpp>

using namespace ascension;
using namespace ascension::contentassist;
using namespace ascension::kernel;
using namespace ascension::text;
using namespace ascension::viewers;
using namespace std;


// IdentifiersProposalProcessor ///////////////////////////////////////////////////////////////////

namespace {
	class DisplayStringComparer {
	public:
		explicit DisplayStringComparer(const ContentAssistProcessor& processor) BOOST_NOEXCEPT : processor_(processor) {
		}
		bool operator()(shared_ptr<const CompletionProposal> lhs, const String& rhs) const {
			return processor_.compareDisplayStrings(lhs->displayString(), rhs);
		}
		bool operator()(const String& lhs, shared_ptr<const CompletionProposal> rhs) const {
			return processor_.compareDisplayStrings(lhs, rhs->displayString());
		}
	private:
		const ContentAssistProcessor& processor_;
	};
} // namespace @0

/**
 * Constructor.
 * @param contentType The content type
 * @param syntax The identifier syntax to detect identifiers
 */
IdentifiersProposalProcessor::IdentifiersProposalProcessor(ContentType contentType,
		const IdentifierSyntax& syntax) BOOST_NOEXCEPT : contentType_(contentType), syntax_(syntax) {
}

/// Destructor.
IdentifiersProposalProcessor::~IdentifiersProposalProcessor() BOOST_NOEXCEPT {
}

/// @see ContentAssistProcessor#activeCompletionProposal
shared_ptr<const CompletionProposal> IdentifiersProposalProcessor::activeCompletionProposal(
		const TextViewer& textViewer, const Region& replacementRegion,
		shared_ptr<const CompletionProposal> currentProposals[], size_t numberOfCurrentProposals) const BOOST_NOEXCEPT {
	// select the partially matched proposal
	String precedingIdentifier(textViewer.document().line(replacementRegion.first.line).substr(
		replacementRegion.beginning().offsetInLine, replacementRegion.end().offsetInLine - replacementRegion.beginning().offsetInLine));
	if(precedingIdentifier.empty())
		return shared_ptr<CompletionProposal>();
	shared_ptr<const CompletionProposal> activeProposal(*lower_bound(currentProposals,
		currentProposals + numberOfCurrentProposals, precedingIdentifier, DisplayStringComparer(*this)));
	if(activeProposal == currentProposals[numberOfCurrentProposals]
			|| CaseFolder::compare(activeProposal->displayString().substr(0, precedingIdentifier.length()), precedingIdentifier) != 0)
		return shared_ptr<CompletionProposal>();
	return activeProposal;
}

/// @see ContentAssistProcessor#compareDisplayStrings
bool IdentifiersProposalProcessor::compareDisplayStrings(const String& s1, const String& s2) const BOOST_NOEXCEPT {
	return CaseFolder::compare(s1, s2) < 0;
}

/// @see ContentAssistProcessor#computCompletionProposals
void IdentifiersProposalProcessor::computeCompletionProposals(const Caret& caret,
		bool& incremental, Region& replacementRegion, set<shared_ptr<const CompletionProposal>>& proposals) const {
	replacementRegion.second = caret;

	// find the preceding identifier
	static const Index MAXIMUM_IDENTIFIER_LENGTH = 100;
	if(!incremental || locations::isBeginningOfLine(caret))
		replacementRegion.first = caret;
	else if(source::getNearestIdentifier(caret.document(), caret, &replacementRegion.first.offsetInLine, nullptr))
		replacementRegion.first.line = line(caret);
	else
		replacementRegion.first = caret;

	// collect identifiers in the document
	static const Index MAXIMUM_BACKTRACKING_LINES = 500;
	const Document& document = caret.document();
	DocumentCharacterIterator i(document, Region(Position(
		(line(caret) > MAXIMUM_BACKTRACKING_LINES) ?
			line(caret) - MAXIMUM_BACKTRACKING_LINES : 0, 0), replacementRegion.first));
	DocumentPartition currentPartition;
	set<String> identifiers;
	bool followingNIDs = false;
	document.partitioner().partition(i.tell(), currentPartition);
	while(i.hasNext()) {
		if(currentPartition.contentType != contentType_)
			i.seek(currentPartition.region.end());
		if(i.tell() >= currentPartition.region.end()) {
			if(i.tell().offsetInLine == i.line().length())
				i.next();
			document.partitioner().partition(i.tell(), currentPartition);
			continue;
		}
		if(!followingNIDs) {
			const Char* const bol = i.line().data();
			const Char* const s = bol + i.tell().offsetInLine;
			const Char* e = syntax_.eatIdentifier(s, bol + i.line().length());
			if(e > s) {
				identifiers.insert(String(s, e));	// automatically merged
				i.seek(Position(i.tell().line, e - bol));
			} else {
				if(syntax_.isIdentifierContinueCharacter(i.current()))
					followingNIDs = true;
				i.next();
			}
		} else {
			if(!syntax_.isIdentifierContinueCharacter(i.current()))
				followingNIDs = false;
			i.next();
		}
	}
	for(set<String>::const_iterator i(identifiers.begin()), e(identifiers.end()); i != e; ++i)
		proposals.insert(shared_ptr<CompletionProposal>(new DefaultCompletionProposal(*i)));
}

/// Returns the identifier syntax the processor uses or @c null.
const IdentifierSyntax& IdentifiersProposalProcessor::identifierSyntax() const BOOST_NOEXCEPT {
	return syntax_;
}

/// @see ContentAssistProcessor#isIncrementalCompletionAutoTerminationCharacter
bool IdentifiersProposalProcessor::isIncrementalCompletionAutoTerminationCharacter(CodePoint c) const BOOST_NOEXCEPT {
	return !syntax_.isIdentifierContinueCharacter(c);
}

/// @see ContentAssistProcessor#recomputIncrementalCompletionProposals
void IdentifiersProposalProcessor::recomputeIncrementalCompletionProposals(
		const TextViewer&, const Region&, shared_ptr<const CompletionProposal>[], size_t, set<shared_ptr<const CompletionProposal>>&) const {
	// do nothing
}
