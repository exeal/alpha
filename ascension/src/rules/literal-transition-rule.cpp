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
		 * @param escapeCharacter The character which a character will be ignored. If @c NONCHARACTER is specified, the
		 *                        escape character will be not set. This is always case-sensitive
		 * @param caseSensitive Set @c false to enable caseless match
		 */
		LiteralTransitionRule::LiteralTransitionRule(kernel::ContentType contentType, kernel::ContentType destination,
				const String& pattern, Char escapeCharacter /* = NONCHARACTER */, bool caseSensitive /* = true */) :
				TransitionRule(contentType, destination), pattern_(pattern), escapeCharacter_(escapeCharacter), caseSensitive_(caseSensitive) {
		}
		
		/// @see TransitionRule#clone
		std::unique_ptr<TransitionRule> LiteralTransitionRule::clone() const {
			return std::unique_ptr<TransitionRule>(new LiteralTransitionRule(*this));
		}
		
		/// @see TransitionRule#matches
		Index LiteralTransitionRule::matches(const StringPiece& line, Index offsetInLine) const {
			if(escapeCharacter_ != text::NONCHARACTER && offsetInLine > 0 && line[offsetInLine - 1] == escapeCharacter_)
				return 0;
			else if(pattern_.empty() && offsetInLine == line.length())	// matches EOL
				return 1;
			else if(line.length() - offsetInLine < pattern_.length())
				return 0;
			else if(caseSensitive_)
				return std::equal(pattern_.cbegin(), pattern_.cend(), line.data() + offsetInLine) ? pattern_.length() : 0;
			return (text::CaseFolder::compare(text::StringCharacterIterator(pattern_),
				text::StringCharacterIterator(line, std::begin(line) + offsetInLine)) == 0) ? pattern_.length() : 0;
		}
	}
}
