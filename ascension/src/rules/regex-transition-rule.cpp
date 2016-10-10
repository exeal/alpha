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
		 * @param tokenBias The @c TokenBias flag
		 * @throw regex#PatternSyntaxException @a pattern is invalid
		 * @throw UnknownValueException @a tokenBias is invalid
		 */
		RegexTransitionRule::RegexTransitionRule(
				const kernel::ContentType& contentType, const kernel::ContentType& destination,
				std::unique_ptr<const regex::Pattern> pattern, TokenBias tokenBias) :
				TransitionRule(contentType, destination), pattern_(std::move(pattern)), tokenBias_(tokenBias) {
			if(tokenBias != NEW_PARTITION_BEGINS_AT_BEGINNING_OF_TOKEN && tokenBias != NEW_PARTITION_BEGINS_AT_END_OF_TOKEN)
				throw UnknownValueException("tokenBias");
		}
		
		/// Copy-constructor.
		RegexTransitionRule::RegexTransitionRule(const RegexTransitionRule& other) :
				TransitionRule(other), pattern_(new regex::Pattern(*other.pattern_.get())), tokenBias_(other.tokenBias_) {
		}
		
		/// @see TransitionRule#clone
		std::unique_ptr<TransitionRule> RegexTransitionRule::clone() const {
			return std::unique_ptr<TransitionRule>(new RegexTransitionRule(*this));
		}
		
		/// @see TransitionRule#matches
		boost::optional<TransitionRule::TransitionToken> RegexTransitionRule::matches(const StringPiece& lineString, StringPiece::const_iterator at) const {
			try {
				typedef text::utf::CharacterDecodeIterator<StringPiece::const_iterator> I;
				std::unique_ptr<regex::Matcher<I>> matcher(pattern_->matcher(
					I(std::begin(lineString), std::end(lineString)), I(std::begin(lineString), std::end(lineString), std::end(lineString))));
				matcher->region(I(std::begin(lineString), std::end(lineString), at), matcher->regionEnd());
				matcher->useAnchoringBounds(false).useTransparentBounds(true);
				if(matcher->lookingAt()) {
					TransitionToken token;
					token.length = std::max<Index>(std::distance(matcher->start().tell(), matcher->end().tell()), 1);
					token.bias = tokenBias_;
					return token;
				}
			} catch(const std::runtime_error&) {
			}
			return boost::none;
		}
	}
}

#endif // !ASCENSION_NO_REGEX
