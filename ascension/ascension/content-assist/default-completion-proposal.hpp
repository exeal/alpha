/**
 * @file default-completion-proposal.hpp
 * @author exeal
 * @date 2003-2006 was CompletionWindow.h
 * @date 2006-2012 was content-assist.hpp
 * @date 2012-03-11 separated from content-assist.hpp
 */

#ifndef ASCENSION_DEFAULT_COMPLETION_PROPOSAL_HPP
#define ASCENSION_DEFAULT_COMPLETION_PROPOSAL_HPP

#include <ascension/content-assist/content-assist.hpp>

namespace ascension {
	namespace contentassist {
		/// Default implementation of @c CompletionalProposal.
		class DefaultCompletionProposal : public CompletionProposal {
			ASCENSION_UNASSIGNABLE_TAG(DefaultCompletionProposal);
		public:
			explicit DefaultCompletionProposal(
				const String& replacementString, const String& description = String(),
				CompletionProposal::Icon&& icon = Icon(), bool autoInsertable = true);
			DefaultCompletionProposal(const String& replacementString,
				const String& displayString, const String& description = String(),
				CompletionProposal::Icon&& icon = Icon(), bool autoInsertable = true);
		public:
			String description() const /*throw()*/;
			String displayString() const /*throw()*/;
			const Icon& icon() const /*throw()*/;
			bool isAutoInsertable() const /*throw()*/;
			void replace(kernel::Document& document,
				const kernel::Region& replacementRegion) const;
		private:
			const String displayString_, replacementString_, descriptionString_;
			const Icon icon_;
			const bool autoInsertable_;
		};
	}
} // namespace ascension.contentassist

#endif // ASCENSION_DEFAULT_COMPLETION_PROPOSAL_HPP
