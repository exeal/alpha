/**
 * @file transition-rules.cpp
 * @author exeal
 * @date 2004-2006 (was Lexer.cpp)
 * @date 2006-2014 was rules.cpp
 * @date 2014-01-13 separated from rules.cpp
 */

#include <ascension/corelib/text/case-folder.hpp>
#include <ascension/corelib/text/string-character-iterator.hpp>
#include <ascension/corelib/ustring.hpp>	// umemcmp
#include <ascension/rules/transition-rules.hpp>


namespace ascension {
	namespace rules {
		// TransitionRule /////////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * Protected constructor.
		 * @param contentType The content type of the transition source
		 * @param destination The content type of the transition destination
		 */
		TransitionRule::TransitionRule(kernel::ContentType contentType,
				kernel::ContentType destination) : contentType_(contentType), destination_(destination) BOOST_NOEXCEPT {
		}
		
		/// Destructor.
		TransitionRule::~TransitionRule() BOOST_NOEXCEPT {
		}


		// LiteralTransitionRule //////////////////////////////////////////////////////////////////////////////////////
		
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
		Index LiteralTransitionRule::matches(const String& line, Index offsetInLine) const {
			if(escapeCharacter_ != text::NONCHARACTER && offsetInLine > 0 && line[offsetInLine - 1] == escapeCharacter_)
				return 0;
			else if(pattern_.empty() && offsetInLine == line.length())	// matches EOL
				return 1;
			else if(line.length() - offsetInLine < pattern_.length())
				return 0;
			else if(caseSensitive_)
				return (umemcmp(pattern_.data(), line.data() + offsetInLine, pattern_.length()) == 0) ? pattern_.length() : 0;
			return (text::CaseFolder::compare(text::StringCharacterIterator(pattern_),
				text::StringCharacterIterator(line, std::begin(line) + offsetInLine)) == 0) ? pattern_.length() : 0;
		}


#ifndef ASCENSION_NO_REGEX

		// RegexTransitionRule ////////////////////////////////////////////////////////////////////////////////////////
		
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
		Index RegexTransitionRule::matches(const String& line, Index offsetInLine) const {
			try {
				typedef text::utf::CharacterDecodeIterator<String::const_iterator> I;
				std::unique_ptr<regex::Matcher<I>> matcher(pattern_->matcher(I(std::begin(line), std::end(line)), I(std::begin(line), std::end(line), std::end(line))));
				matcher->region(I(std::begin(line), std::end(line), std::begin(line) + offsetInLine), matcher->regionEnd());
				matcher->useAnchoringBounds(false).useTransparentBounds(true);
				return matcher->lookingAt() ? std::max(matcher->end().tell() - matcher->start().tell(), 1) : 0;
			} catch(const std::runtime_error&) {
				return 0;
			}
		}

#endif // !ASCENSION_NO_REGEX
	}
}
