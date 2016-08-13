/**
 * @file region-token-rule.hpp
 * @author exeal
 * @date 2004-2006 was Lexer.h
 * @date 2016-08-13 Separated from token-rules.hpp.
 */

#ifndef ASCENSION_REGION_TOKEN_RULE_HPP
#define ASCENSION_REGION_TOKEN_RULE_HPP
#include <ascension/corelib/text/character.hpp>
#include <ascension/rules/token-rule.hpp>

namespace ascension {
	namespace rules {
		/***/
		class RegionTokenRule : public TokenRule {
		public:
			RegionTokenRule(Token::Identifier identifier,
				const String& startSequence, const String& endSequence,
				Char escapeCharacter = text::NONCHARACTER, bool caseSensitive = true);
			boost::optional<StringPiece::const_iterator> parse(
				const StringPiece& text, StringPiece::const_iterator start,
				const text::IdentifierSyntax& identifierSyntax) const BOOST_NOEXCEPT override;

		private:
			const String startSequence_, endSequence_;
			const Char escapeCharacter_;
			const bool caseSensitive_;
		};
	}
} // namespace ascension.rules

#endif // !ASCENSION_REGION_TOKEN_RULE_HPP
