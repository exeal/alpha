/**
 * @file regex-token-rule.cpp
 * @author exeal
 * @date 2004-2006 (was Lexer.cpp)
 * @date 2006-2014 was rules.cpp
 * @date 2014-01-13 separated from rules.cpp
 * @date 2016-08-13 Separated from token-rules.cpp.
 */

#include <ascension/rules/regex-token-rule.hpp>
#ifndef ASCENSION_NO_REGEX
#include <ascension/corelib/regex.hpp>

namespace ascension {
	namespace rules {
		/**
		 * Creates a @c RegexTokenRule instance.
		 * @param identifier The identifier of the token which will be returned by the rule
		 * @param pattern The compiled regular expression
		 * @throw regex#PatternSyntaxException The specified pattern is invalid
		 */
		RegexTokenRule::RegexTokenRule(Token::Identifier identifier, std::unique_ptr<const regex::Pattern> pattern)
				: TokenRule(identifier), pattern_(std::move(pattern)) {
		}
		
		/// @see TokenRule#matches
		boost::optional<Index> RegexTokenRule::matches(const StringPiece& lineString,
				StringPiece::const_iterator at, const text::IdentifierSyntax& identifierSyntax) const BOOST_NOEXCEPT {
			assert(lineString.cbegin() < lineString.cend() && at >= lineString.cbegin() && at < lineString.cend());

			const auto b(text::utf::makeCharacterDecodeIterator(lineString.cbegin(), lineString.cend()));
			const auto e(text::utf::makeCharacterDecodeIterator(lineString.cbegin(), lineString.cend(), lineString.cend()));
			std::unique_ptr<regex::Matcher<text::utf::CharacterDecodeIterator<StringPiece::const_iterator>>> matcher(pattern_->matcher(b, e));
			return matcher->lookingAt() ? boost::make_optional<Index>(std::distance(at, matcher->end().tell())) : boost::none;
		}
	}
}

#endif // !ASCENSION_NO_REGEX
