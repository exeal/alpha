/**
 * @file region-token-rule.hpp
 * @author exeal
 * @date 2004-2006 was Lexer.h
 * @date 2016-08-13 Separated from token-rules.hpp.
 */

#ifndef ASCENSION_REGION_TOKEN_RULE_HPP
#define ASCENSION_REGION_TOKEN_RULE_HPP
#include <ascension/rules/token-rule.hpp>

namespace ascension {
	namespace rules {
		/// A concrete @c TokenRule detects regional tokens which start and end with the specified character sequences.
		class RegionTokenRule : public TokenRule {
		public:
			RegionTokenRule(Token::Identifier identifier,
				const StringPiece& startSequence, const StringPiece& endSequence,
				boost::optional<Char> escapeCharacter = boost::none, bool caseSensitive = true);
			boost::optional<Index> matches(
				const StringPiece& lineString, StringPiece::const_iterator at,
				const text::IdentifierSyntax& identifierSyntax) const BOOST_NOEXCEPT override;

		private:
			const String startSequence_, endSequence_;
			const boost::optional<Char> escapeCharacter_;
			const bool caseSensitive_;
		};
	}
} // namespace ascension.rules

#endif // !ASCENSION_REGION_TOKEN_RULE_HPP
