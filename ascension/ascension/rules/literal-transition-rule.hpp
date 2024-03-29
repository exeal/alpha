/**
 * @file literal-transition-rule.hpp
 * @author exeal
 * @date 2004-2006 was Lexer.h
 * @date 2006-2014 was rules.hpp
 * @date 2014-11-16 Separated from rules.hpp
 * @date 2016-08-13 Separated from transition-rule.hpp.
 */

#ifndef ASCENSION_LITERAL_TRANSITION_RULE_HPP
#define ASCENSION_LITERAL_TRANSITION_RULE_HPP
#include <ascension/rules/transition-rule.hpp>

namespace ascension {
	namespace rules {
		/// Implementation of @c TransitionRule uses literal string match.
		class LiteralTransitionRule : public TransitionRule {
		public:
			LiteralTransitionRule(const kernel::ContentType& contentType, const kernel::ContentType& destination,
				const String& pattern, TokenBias tokenBias, Char escapeCharacter = text::NONCHARACTER, bool caseSensitive = true);
			std::unique_ptr<TransitionRule> clone() const override;
			boost::optional<TransitionToken> matches(
				const StringPiece& lineString, StringPiece::const_iterator at) const override;

		private:
			const String pattern_;
			const TokenBias tokenBias_;
			const Char escapeCharacter_;
			const bool caseSensitive_;
		};
	}
} // namespace ascension.rules

#endif // !ASCENSION_LITERAL_TRANSITION_RULE_HPP
