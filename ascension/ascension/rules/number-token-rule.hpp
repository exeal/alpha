/**
 * @file number-token-rule.hpp
 * @author exeal
 * @date 2004-2006 was Lexer.h
 * @date 2016-08-13 Separated from token-rules.hpp.
 */

#ifndef ASCENSION_NUMBER_TOKEN_RULE_HPP
#define ASCENSION_NUMBER_TOKEN_RULE_HPP
#include <ascension/rules/token-rule.hpp>

namespace ascension {
	namespace rules {
		/// A concrete rule detects numeric tokens.
		class NumberTokenRule : public TokenRule {
		public:
			explicit NumberTokenRule(Token::Identifier identifier) BOOST_NOEXCEPT;
			boost::optional<Index> matches(
				const StringPiece& lineString, StringPiece::const_iterator at,
				const text::IdentifierSyntax& identifierSyntax) const BOOST_NOEXCEPT override;
		};
	}
} // namespace ascension.rules

#endif // !ASCENSION_NUMBER_TOKEN_RULE_HPP
