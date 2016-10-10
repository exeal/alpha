/**
 * @file literal-transition-rules.cpp
 * @author exeal
 * @date 2004-2006 (was Lexer.cpp)
 * @date 2006-2014 was rules.cpp
 * @date 2014-01-13 separated from rules.cpp
 * @date 2016-08-13 Separated from transition-rules.cpp.
 */

#include <ascension/corelib/text/case-folder.hpp>
#include <ascension/corelib/text/string-character-iterator.hpp>
#include <ascension/rules/literal-transition-rule.hpp>

namespace ascension {
	namespace rules {
		/**
		 * Constructor.
		 * @param contentType The content type of the transition source
		 * @param destination The content type of the transition destination
		 * @param pattern The pattern string to introduce the transition. If empty string is specified, the transition
		 *                will be occurred at the end of line
		 * @param tokenBias The @c TokenBias flag
		 * @param escapeCharacter The character which a character will be ignored. If @c NONCHARACTER is specified, the
		 *                        escape character will be not set. This is always case-sensitive
		 * @param caseSensitive Set @c false to enable caseless match
		 */
		LiteralTransitionRule::LiteralTransitionRule(const kernel::ContentType& contentType, const kernel::ContentType& destination,
				const String& pattern, TokenBias tokenBias, Char escapeCharacter /* = NONCHARACTER */, bool caseSensitive /* = true */) :
				TransitionRule(contentType, destination), pattern_(pattern), tokenBias_(tokenBias), escapeCharacter_(escapeCharacter), caseSensitive_(caseSensitive) {
		}
		
		/// @see TransitionRule#clone
		std::unique_ptr<TransitionRule> LiteralTransitionRule::clone() const {
			return std::unique_ptr<TransitionRule>(new LiteralTransitionRule(*this));
		}
		
		/// @see TransitionRule#matches
		boost::optional<TransitionRule::TransitionToken> LiteralTransitionRule::matches(const StringPiece& lineString, StringPiece::const_iterator at) const {
			if(escapeCharacter_ != text::NONCHARACTER && std::distance(lineString.cbegin(), at) > 0 && at[-1] == escapeCharacter_)
				return boost::none;
			else if(std::next(at, pattern_.length()) > lineString.cend())
				return boost::none;
			TransitionToken token;
			token.bias = tokenBias_;
			if(pattern_.empty() && at == lineString.cend())	// matches EOL
				token.length = 1;
			else if(caseSensitive_)
				token.length = std::equal(pattern_.cbegin(), pattern_.cend(), at) ? pattern_.length() : 0;
			token.length = (text::CaseFolder::compare(text::StringCharacterIterator(pattern_),
				text::StringCharacterIterator(lineString.cbegin(), at)) == 0) ? pattern_.length() : 0;
			return boost::make_optional(token);
		}
	}
}
