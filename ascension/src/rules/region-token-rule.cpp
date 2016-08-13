/**
 * @file region-token-rule.cpp
 * @author exeal
 * @date 2004-2006 (was Lexer.cpp)
 * @date 2006-2014 was rules.cpp
 * @date 2014-01-13 separated from rules.cpp
 * @date 2016-08-13 Separated from token-rules.cpp.
 */

#include <ascension/rules/region-token-rule.hpp>

namespace ascension {
	namespace rules {
		/**
		 * Creates a @c RegionTokenRule instance.
		 * @param identifier The identifier of the token which will be returned by the rule
		 * @param startSequence The pattern's start sequence
		 * @param endSequence The pattern's end sequence. if empty, token will end at end of line
		 * @param escapeCharacter The character which a character will be ignored
		 * @param caseSensitive Set @c false to enable caseless match
		 * @throw std#invalid_argument @a startSequence is empty
		 */
		RegionTokenRule::RegionTokenRule(Token::Identifier identifier, const String& startSequence, const String& endSequence,
				Char escapeCharacter /* = NONCHARACTER */, bool caseSensitive /* = true */) : TokenRule(identifier),
				startSequence_(startSequence), endSequence_(endSequence), escapeCharacter_(escapeCharacter), caseSensitive_(caseSensitive) {
			if(startSequence.empty())
				throw std::invalid_argument("The start sequence is empty.");
		}
		
		/// @see Rule#parse
		boost::optional<StringPiece::const_iterator> RegionTokenRule::parse(const StringPiece& text,
				StringPiece::const_iterator start, const text::IdentifierSyntax& identifierSyntax) const BOOST_NOEXCEPT {
			assert(text.cbegin() < text.cend() && start >= text.cbegin() && start < text.cend());

			// match the start sequence
			if(start[0] != startSequence_[0]
					|| static_cast<std::size_t>(text.cend() - start) < startSequence_.length() + endSequence_.length()
					|| (startSequence_.length() > 1 && !std::equal(start + 1, start + startSequence_.length(), startSequence_.cbegin() + 1)))
				return boost::none;
			StringPiece::const_iterator end(text.cend());
			if(!endSequence_.empty()) {
				// search the end sequence
				for(StringPiece::const_iterator p(start + startSequence_.length()); p <= text.cend() - endSequence_.length(); ++p) {
					if(escapeCharacter_ != text::NONCHARACTER && *p == escapeCharacter_)
						++p;
					else if(*p == endSequence_[0] && std::equal(p + 1, p + endSequence_.length(), endSequence_.cbegin() + 1)) {
						end = p + endSequence_.length();
						break;
					}
				}
			}
			return end;
		}
	}
}
