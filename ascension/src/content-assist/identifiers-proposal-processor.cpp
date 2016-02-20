/**
 * @file identifiers-proposal-processor.cpp
 * @author exeal
 * @date 2003-2006 was CompletionWindow.cpp
 * @date 2006-2012 was content-assist.cpp
 * @date 2012-03-12 separated from content-assist.cpp
 * @date 2014-2015
 */

#include <ascension/content-assist/default-completion-proposal.hpp>
#include <ascension/content-assist/identifiers-proposal-processor.hpp>
#include <ascension/corelib/text/case-folder.hpp>
#include <ascension/corelib/text/identifier-syntax.hpp>
#include <ascension/graphics/font/text-layout.hpp>
#include <ascension/kernel/document.hpp>
#include <ascension/kernel/document-character-iterator.hpp>
#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/text-viewer.hpp>
#include <ascension/viewer/text-viewer-utility.hpp>
#include <boost/foreach.hpp>


namespace ascension {
	namespace contentassist {
		// IdentifiersProposalProcessor ///////////////////////////////////////////////////////////////////////////////

		namespace {
			class DisplayStringComparer {
			public:
				explicit DisplayStringComparer(const ContentAssistProcessor& processor) BOOST_NOEXCEPT : processor_(processor) {
				}
				bool operator()(std::shared_ptr<const CompletionProposal> lhs, const String& rhs) const {
					return processor_.compareDisplayStrings(lhs->displayString(), rhs);
				}
				bool operator()(const String& lhs, std::shared_ptr<const CompletionProposal> rhs) const {
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
		IdentifiersProposalProcessor::IdentifiersProposalProcessor(kernel::ContentType contentType,
				const text::IdentifierSyntax& syntax) BOOST_NOEXCEPT : contentType_(contentType), syntax_(syntax) {
		}

		/// Destructor.
		IdentifiersProposalProcessor::~IdentifiersProposalProcessor() BOOST_NOEXCEPT {
		}

		/// @see ContentAssistProcessor#activeCompletionProposal
		std::shared_ptr<const CompletionProposal> IdentifiersProposalProcessor::activeCompletionProposal(
				const viewer::TextViewer& textViewer, const kernel::Region& replacementRegion,
				std::shared_ptr<const CompletionProposal> currentProposals[], std::size_t numberOfCurrentProposals) const BOOST_NOEXCEPT {
			// select the partially matched proposal
			String precedingIdentifier(textViewer.document().line(kernel::line(replacementRegion.first)).substr(
				kernel::offsetInLine(replacementRegion.beginning()), kernel::offsetInLine(replacementRegion.end()) - kernel::offsetInLine(replacementRegion.beginning())));
			if(precedingIdentifier.empty())
				return std::shared_ptr<CompletionProposal>();
			std::shared_ptr<const CompletionProposal> activeProposal(*std::lower_bound(currentProposals,
				currentProposals + numberOfCurrentProposals, precedingIdentifier, DisplayStringComparer(*this)));
			if(activeProposal == currentProposals[numberOfCurrentProposals]
					|| text::CaseFolder::compare(activeProposal->displayString().substr(0, precedingIdentifier.length()), precedingIdentifier) != 0)
				return std::shared_ptr<CompletionProposal>();
			return activeProposal;
		}

		/// @see ContentAssistProcessor#compareDisplayStrings
		bool IdentifiersProposalProcessor::compareDisplayStrings(const String& s1, const String& s2) const BOOST_NOEXCEPT {
			return text::CaseFolder::compare(s1, s2) < 0;
		}

		/// @see ContentAssistProcessor#computCompletionProposals
		void IdentifiersProposalProcessor::computeCompletionProposals(const viewer::Caret& caret,
				bool& incremental, kernel::Region& replacementRegion, std::set<std::shared_ptr<const CompletionProposal>>& proposals) const {
			replacementRegion.second = caret;

			// find the preceding identifier
			static const Index MAXIMUM_IDENTIFIER_LENGTH = 100;
			if(!incremental || kernel::locations::isBeginningOfLine(caret))
				replacementRegion.first = caret;
			else if(viewer::utils::getNearestIdentifier(caret.document(), caret, &replacementRegion.first.offsetInLine, nullptr))
				replacementRegion.first.line = kernel::line(caret);
			else
				replacementRegion.first = caret;

			// collect identifiers in the document
			static const Index MAXIMUM_BACKTRACKING_LINES = 500;
			const kernel::Document& document = caret.document();
			kernel::DocumentCharacterIterator i(document, kernel::Region(kernel::Position(
				(kernel::line(caret) > MAXIMUM_BACKTRACKING_LINES) ?
					kernel::line(caret) - MAXIMUM_BACKTRACKING_LINES : 0, 0), replacementRegion.first));
			kernel::DocumentPartition currentPartition;
			std::set<String> identifiers;
			bool followingNIDs = false;
			document.partitioner().partition(i.tell(), currentPartition);
			while(i.hasNext()) {
				if(currentPartition.contentType != contentType_)
					i.seek(currentPartition.region.end());
				if(i.tell() >= currentPartition.region.end()) {
					if(kernel::offsetInLine(i.tell()) == i.line().length())
						++i;
					document.partitioner().partition(i.tell(), currentPartition);
					continue;
				}
				if(!followingNIDs) {
					const Char* const bol = i.line().data();
					const Char* const s = bol + kernel::offsetInLine(i.tell());
					const Char* e = syntax_.eatIdentifier(s, bol + i.line().length());
					if(e > s) {
						identifiers.insert(String(s, e));	// automatically merged
						i.seek(kernel::Position(kernel::line(i.tell()), e - bol));
					} else {
						if(syntax_.isIdentifierContinueCharacter(*i))
							followingNIDs = true;
						++i;
					}
				} else {
					if(!syntax_.isIdentifierContinueCharacter(*i))
						followingNIDs = false;
					++i;
				}
			}
			BOOST_FOREACH(const String& identifier, identifiers)
				proposals.insert(std::make_shared<DefaultCompletionProposal>(identifier));
		}

		/// Returns the identifier syntax the processor uses or @c null.
		const text::IdentifierSyntax& IdentifiersProposalProcessor::identifierSyntax() const BOOST_NOEXCEPT {
			return syntax_;
		}

		/// @see ContentAssistProcessor#isIncrementalCompletionAutoTerminationCharacter
		bool IdentifiersProposalProcessor::isIncrementalCompletionAutoTerminationCharacter(CodePoint c) const BOOST_NOEXCEPT {
			return !syntax_.isIdentifierContinueCharacter(c);
		}

		/// @see ContentAssistProcessor#recomputIncrementalCompletionProposals
		void IdentifiersProposalProcessor::recomputeIncrementalCompletionProposals(const viewer::TextViewer&,
				const kernel::Region&, std::shared_ptr<const CompletionProposal>[], std::size_t, std::set<std::shared_ptr<const CompletionProposal>>&) const {
			// do nothing
		}
	}
}
