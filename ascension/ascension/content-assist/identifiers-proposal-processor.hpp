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
			ASCENSION_UNASSIGNABLE_TAG(IdentifiersProposalProcessor);
		protected:
			// constructors
			IdentifiersProposalProcessor(
				kernel::ContentType contentType, const text::IdentifierSyntax& syntax) /*throw()*/;
			virtual ~IdentifiersProposalProcessor() /*throw()*/;
			// attributes
			kernel::ContentType contentType() const /*throw()*/;
			const text::IdentifierSyntax& identifierSyntax() const /*throw()*/;
			// ContentAssistProcessor
			virtual const CompletionProposal* activeCompletionProposal(
				const viewers::TextViewer& textViewer, const kernel::Region& replacementRegion,
				CompletionProposal* const proposals[], std::size_t numberOfProposals) const /*throw()*/;
			virtual void computeCompletionProposals(const viewers::Caret& caret, bool& incremental,
				kernel::Region& replacementRegion, std::set<CompletionProposal*>& proposals) const;
			virtual bool isIncrementalCompletionAutoTerminationCharacter(CodePoint c) const /*throw()*/;
			virtual void recomputeIncrementalCompletionProposals(const viewers::TextViewer& textViewer,
				const kernel::Region& replacementRegion, CompletionProposal* const currentProposals[],
				std::size_t numberOfCurrentProposals, std::set<CompletionProposal*>& newProposals) const;
		private:
			const kernel::ContentType contentType_;
			const text::IdentifierSyntax& syntax_;
		};
	}
} // namespace ascension.contentassist

#endif // ASCENSION_IDENTIFIERS_PROPOSAL_PROCESSOR_HPP
