/**
 * @file identifiers-proposal-processor.hpp
 * @author exeal
 * @date 2003-2006 was CompletionWindow.h
 * @date 2006-2012 was content-assist.hpp
 * @date 2012-03-12 separated from content-assist.hpp
 */

#ifndef ASCENSION_IDENTIFIERS_PROPOSAL_PROCESSOR_HPP
#define ASCENSION_IDENTIFIERS_PROPOSAL_PROCESSOR_HPP

#include <ascension/content-assist/content-assist.hpp>

namespace ascension {
	namespace contentassist {
		/**
		 * An abstract implementation of @c ContentAssistProcessor builds completion proposals by
		 * collecting identifiers in the document.
		 */
		class IdentifiersProposalProcessor : public ContentAssistProcessor {
		protected:
			// constructors
			IdentifiersProposalProcessor(
				kernel::ContentType contentType, const text::IdentifierSyntax& syntax) BOOST_NOEXCEPT;
			virtual ~IdentifiersProposalProcessor() BOOST_NOEXCEPT;
			// attributes
			kernel::ContentType contentType() const BOOST_NOEXCEPT;
			const text::IdentifierSyntax& identifierSyntax() const BOOST_NOEXCEPT;
			// ContentAssistProcessor
			virtual std::shared_ptr<const CompletionProposal> activeCompletionProposal(
				const viewer::TextViewer& textViewer, const kernel::Region& replacementRegion,
				std::shared_ptr<const CompletionProposal> proposals[], std::size_t numberOfProposals) const BOOST_NOEXCEPT;
			virtual bool compareDisplayStrings(const String& s1, const String& s2) const BOOST_NOEXCEPT;
			virtual void computeCompletionProposals(const viewer::Caret& caret, bool& incremental,
				kernel::Region& replacementRegion, std::set<std::shared_ptr<const CompletionProposal>>& proposals) const;
			virtual bool isIncrementalCompletionAutoTerminationCharacter(CodePoint c) const BOOST_NOEXCEPT;
			virtual void recomputeIncrementalCompletionProposals(const viewer::TextViewer& textViewer,
				const kernel::Region& replacementRegion, std::shared_ptr<const CompletionProposal> currentProposals[],
				std::size_t numberOfCurrentProposals, std::set<std::shared_ptr<const CompletionProposal>>& newProposals) const;
		private:
			const kernel::ContentType contentType_;
			const text::IdentifierSyntax& syntax_;
		};
	}
} // namespace ascension.contentassist

#endif // ASCENSION_IDENTIFIERS_PROPOSAL_PROCESSOR_HPP
