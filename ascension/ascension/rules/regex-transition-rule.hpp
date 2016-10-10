/**
 * @file regex-transition-rule.hpp
 * @author exeal
 * @date 2004-2006 was Lexer.h
 * @date 2006-2014 was rules.hpp
 * @date 2014-11-16 Separated from rules.hpp
 * @date 2016-08-13 Separated from transition-rules.hpp.
 */

#ifndef ASCENSION_REGEX_TRANSITION_RULE_HPP
#define ASCENSION_REGEX_TRANSITION_RULE_HPP
#include <ascension/config.hpp>	// ASCENSION_NO_REGEX
#ifndef ASCENSION_NO_REGEX
#include <ascension/corelib/regex.hpp>
#include <ascension/rules/transition-rule.hpp>

namespace ascension {
	namespace rules {
		/// Implementation of @c TransitionRule uses regular expression match.
		class RegexTransitionRule : public TransitionRule {
		public:
			RegexTransitionRule(
				const kernel::ContentType& contentType, const kernel::ContentType& destination,
				std::unique_ptr<const regex::Pattern> pattern, TokenBias tokenBias);
			RegexTransitionRule(const RegexTransitionRule& other);
			std::unique_ptr<TransitionRule> clone() const override;
			boost::optional<TransitionToken> matches(
				const StringPiece& line, StringPiece::const_iterator at) const override;

		private:
			const std::unique_ptr<const regex::Pattern> pattern_;
			const TokenBias tokenBias_;
		};
	}
} // namespace ascension.rules

#endif // !ASCENSION_NO_REGEX
#endif // !ASCENSION_REGEX_TRANSITION_RULE_HPP
