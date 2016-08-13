/**
 * @file regex-token-rule.hpp
 * @author exeal
 * @date 2004-2006 was Lexer.h
 * @date 2016-08-13 Separated from token-rules.hpp.
 */

#ifndef ASCENSION_REGEX_TOKEN_RULE_HPP
#define ASCENSION_REGEX_TOKEN_RULE_HPP
#include <ascension/config.hpp>	// ASCENSION_NO_REGEX
#ifndef ASCENSION_NO_REGEX
#include <ascension/rules/token-rule.hpp>

namespace ascension {
	namespace regex {
		class Pattern;
	}

	namespace rules {
		/// A concrete rule detects tokens using regular expression match.
		class RegexTokenRule : public TokenRule {
		public:
			RegexTokenRule(Token::Identifier identifier, std::unique_ptr<const regex::Pattern> pattern);
			boost::optional<StringPiece::const_iterator> parse(
				const StringPiece& text, StringPiece::const_iterator start,
				const text::IdentifierSyntax& identifierSyntax) const BOOST_NOEXCEPT override;

		private:
			std::unique_ptr<const regex::Pattern> pattern_;
		};
	}
} // namespace ascension.rules

#endif // !ASCENSION_NO_REGEX
#endif // !ASCENSION_REGEX_TOKEN_RULE_HPP
