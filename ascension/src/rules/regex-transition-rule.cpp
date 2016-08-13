/**
 * @file regex-transition-rule.cpp
 * @author exeal
 * @date 2004-2006 (was Lexer.cpp)
 * @date 2006-2014 was rules.cpp
 * @date 2014-01-13 separated from rules.cpp
 * @date 2016-08-13 Separated from transition-rules.cpp.
 */

#include <ascension/rules/regex-transition-rule.hpp>
#ifndef ASCENSION_NO_REGEX

#include <ascension/corelib/text/utf-iterator.hpp>

namespace ascension {
	namespace rules {
		/**
		 * Constructor.
		 * @param contentType The content type of the transition source
		 * @param destination The content type of the transition destination
		 * @param pattern The compiled regular expression to introduce the transition
		 * @throw regex#PatternSyntaxException @a pattern is invalid
		 */
		RegexTransitionRule::RegexTransitionRule(kernel::ContentType contentType, kernel::ContentType destination,
				std::unique_ptr<const regex::Pattern> pattern) : TransitionRule(contentType, destination), pattern_(std::move(pattern)) {
		}
		
		/// Copy-constructor.
		RegexTransitionRule::RegexTransitionRule(const RegexTransitionRule& other) :
				TransitionRule(other), pattern_(new regex::Pattern(*other.pattern_.get())) {
		}
		
		/// @see TransitionRule#clone
		std::unique_ptr<TransitionRule> RegexTransitionRule::clone() const {
			return std::unique_ptr<TransitionRule>(new RegexTransitionRule(*this));
		}
		
		/// @see TransitionRule#matches
		Index RegexTransitionRule::matches(const StringPiece& line, Index offsetInLine) const {
			try {
				typedef text::utf::CharacterDecodeIterator<StringPiece::const_iterator> I;
				std::unique_ptr<regex::Matcher<I>> matcher(pattern_->matcher(I(std::begin(line), std::end(line)), I(std::begin(line), std::end(line), std::end(line))));
				matcher->region(I(std::begin(line), std::end(line), std::begin(line) + offsetInLine), matcher->regionEnd());
				matcher->useAnchoringBounds(false).useTransparentBounds(true);
				return matcher->lookingAt() ? std::max(matcher->end().tell() - matcher->start().tell(), 1) : 0;
			} catch(const std::runtime_error&) {
				return 0;
			}
		}
	}
}

#endif // !ASCENSION_NO_REGEX
