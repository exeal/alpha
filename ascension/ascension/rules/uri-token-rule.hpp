/**
 * @file uri-token-rule.hpp
 * @author exeal
 * @date 2004-2006 was Lexer.h
 * @date 2016-08-13 Separated from token-rules.hpp.
 */

#ifndef ASCENSION_URI_TOKEN_RULE_HPP
#define ASCENSION_URI_TOKEN_RULE_HPP
#include <ascension/rules/token-rule.hpp>

namespace ascension {
	namespace rules {
		class URIDetector;

		/// A concrete rule detects URI strings.
		class URITokenRule : public TokenRule {
		public:
			URITokenRule(Token::Identifier identifier, std::shared_ptr<const URIDetector> uriDetector);
			boost::optional<Index> matches(
				const StringPiece& lineString, StringPiece::const_iterator at,
				const text::IdentifierSyntax& identifierSyntax) const BOOST_NOEXCEPT override;
		private:
			std::shared_ptr<const URIDetector> uriDetector_;
		};
	}
} // namespace ascension.rules

#endif // !ASCENSION_URI_TOKEN_RULE_HPP
