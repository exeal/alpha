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
		 * @param endSequence The pattern's end sequence. If empty, token will end at end of line
		 * @param escapeCharacter The character which a character will be ignored
		 * @param caseSensitive Set @c false to enable caseless match
		 * @throw std#invalid_argument @a startSequence is empty
		 */
		RegionTokenRule::RegionTokenRule(Token::Identifier identifier,
				const StringPiece& startSequence, const StringPiece& endSequence,
				boost::optional<Char> escapeCharacter /* = boost::none */, bool caseSensitive /* = true */) :
				TokenRule(identifier),
				startSequence_(startSequence.to_string()), endSequence_(endSequence.to_string()),
				escapeCharacter_(escapeCharacter), caseSensitive_(caseSensitive) {
			if(startSequence.empty())
				throw std::invalid_argument("The start sequence is empty.");
		}
		
		/// @see TokenRule#matches
		boost::optional<Index> RegionTokenRule::matches(const StringPiece& lineString,
				StringPiece::const_iterator at, const text::IdentifierSyntax&) const BOOST_NOEXCEPT {
			assert(lineString.cbegin() < lineString.cend() && at >= lineString.cbegin() && at < lineString.cend());
			const auto eol(lineString.cend());

			// match the start sequence
			if(at[0] != startSequence_[0]
					|| (escapeCharacter_ != boost::none && at > lineString.cbegin() && at[-1] == boost::get(escapeCharacter_))
					|| static_cast<std::size_t>(std::distance(at, eol)) < startSequence_.length() + endSequence_.length()
					|| (startSequence_.length() > 1 && !std::equal(std::next(at), std::next(at, startSequence_.length()), std::next(startSequence_.cbegin()))))
				return boost::none;

			if(endSequence_.empty())
				return boost::make_optional<Index>(std::distance(at, eol));

			// search the end sequence
			for(auto p(std::next(at, startSequence_.length())); p <= std::prev(eol, endSequence_.length()); ++p) {
				if(escapeCharacter_ != boost::none && *p == boost::get(escapeCharacter_))
					++p;
				else if(*p == endSequence_[0]) {
					if(endSequence_.length() > 1 && !std::equal(std::next(endSequence_.cbegin()), endSequence_.cend(), std::next(p)))
						continue;
					return boost::make_optional<Index>(std::distance(at, std::next(p, endSequence_.length())));
				}
			}
			return boost::none;
		}
	}
}
