/**
 * @file token-rules.cpp
 * @author exeal
 * @date 2004-2006 (was Lexer.cpp)
 * @date 2006-2014 was rules.cpp
 * @date 2014-01-13 separated from rules.cpp
 */

#include <ascension/corelib/ustring.hpp>	// umemchr, umemcmp, ustrchr
#include <ascension/rules/hash-table.hpp>
#include <ascension/rules/token-rules.hpp>
#include <boost/numeric/interval.hpp>
#include <boost/range/algorithm/find_if.hpp>


namespace ascension {
	namespace rules {
		namespace {
			inline bool includes(const StringPiece& s, StringPiece::const_iterator i) {
				return i >= s.cbegin() && i < s.cend();
			}
			inline bool includes(const StringPiece& outer, const StringPiece& inner) {
				return inner.cbegin() >= outer.cbegin() && inner.cend() <= outer.cend();
			}
		}

		// Token //////////////////////////////////////////////////////////////////////////////////////////////////////

		const Token::Identifier Token::UNCALCULATED = static_cast<Token::Identifier>(-1);


		// RuleBase ///////////////////////////////////////////////////////////////////////////////////////////////////
		
		/**
		 * Protected constructor.
		 * @param tokenID The identifier of the token which will be returned by the rule. Can't be
		 *                @c Token#UNCALCULATED which is for internal use
		 * @throw std#invalid_argument @a tokenID is invalid
		 */
		RuleBase::RuleBase(Token::Identifier tokenID) : identifier_(tokenID) {
			if(tokenID == Token::UNCALCULATED)
				throw std::invalid_argument("tokenID");
		}


		// RegionRule /////////////////////////////////////////////////////////////////////////////////////////////////
		
		/**
		 * Constructor.
		 * @param identifier The identifier of the token which will be returned by the rule
		 * @param startSequence The pattern's start sequence
		 * @param endSequence The pattern's end sequence. if empty, token will end at end of line
		 * @param escapeCharacter The character which a character will be ignored
		 * @param caseSensitive Set @c false to enable caseless match
		 * @throw std#invalid_argument @a startSequence is empty
		 */
		RegionRule::RegionRule(Token::Identifier identifier, const String& startSequence, const String& endSequence,
				Char escapeCharacter /* = NONCHARACTER */, bool caseSensitive /* = true */) : Rule(identifier),
				startSequence_(startSequence), endSequence_(endSequence), escapeCharacter_(escapeCharacter), caseSensitive_(caseSensitive) {
			if(startSequence.empty())
				throw std::invalid_argument("The start sequence is empty.");
		}
		
		/// @see Rule#parse
		boost::optional<StringPiece::const_iterator> RegionRule::parse(const StringPiece& text,
				StringPiece::const_iterator start, const text::IdentifierSyntax& identifierSyntax) const BOOST_NOEXCEPT {
			assert(text.cbegin() < text.cend() && includes(text, start));

			// match the start sequence
			if(start[0] != startSequence_[0]
					|| static_cast<std::size_t>(text.cend() - start) < startSequence_.length() + endSequence_.length()
					|| (startSequence_.length() > 1 && umemcmp(start + 1, startSequence_.data() + 1, startSequence_.length() - 1) != 0))
				return boost::none;
			StringPiece::const_iterator end(text.cend());
			if(!endSequence_.empty()) {
				// search the end sequence
				for(StringPiece::const_iterator p(start + startSequence_.length()); p <= text.cend() - endSequence_.length(); ++p) {
					if(escapeCharacter_ != text::NONCHARACTER && *p == escapeCharacter_)
						++p;
					else if(*p == endSequence_[0] && umemcmp(p + 1, endSequence_.data() + 1, endSequence_.length() - 1) == 0) {
						end = p + endSequence_.length();
						break;
					}
				}
			}
			return end;
		}
		
		
		// NumberRule /////////////////////////////////////////////////////////////////////////////////////////////////
		
		/**
		 * Constructor.
		 * @param identifier The identifier of the token which will be returned by the rule
		 */
		NumberRule::NumberRule(Token::Identifier identifier) BOOST_NOEXCEPT : Rule(identifier) {
		}
		
		/// @see Rule#parse
		boost::optional<StringPiece::const_iterator> NumberRule::parse(const StringPiece& text,
				StringPiece::const_iterator start, const text::IdentifierSyntax& identifierSyntax) const BOOST_NOEXCEPT {
			assert(text.cbegin() < text.cend() && includes(text, start));

			/*
				This is based on ECMAScript 3 "7.8.3 Numeric Literals" and performs the following regular
				expression match:
					/(0|[1-9][0-9]*)(\.[0-9]+)?([e|E][\+\-]?[0-9]+)?/ for DecimalLiteral (case 1)
					/\.[0-9]+([e|E][\+\-]?[0-9]+)?/ for DecimalLiteral (case 2)
					/0[x|X][0-9A-Fa-f]+/ for HexIntegerLiteral
				Octal integer literals are not supported. See "B.1.1 Numeric Literals" in the same specification.
			*/
			// ISSUE: This implementation accepts some illegal format like as "0.1.2".
			static const boost::numeric::interval<Char> DIGITS('0', '9'), CAPITAL_LETTERS('A', 'F'), SMALL_LETTERS('a', 'f');
			if(start > text.cbegin()	// see below
					&& (boost::numeric::in(start[-1], DIGITS) || boost::numeric::in(start[-1], CAPITAL_LETTERS) || boost::numeric::in(start[-1], SMALL_LETTERS)))
				return boost::none;
			StringPiece::const_iterator e;
			if(text.cend() - start > 2 && start[0] == '0' && (start[1] == 'x' || start[1] == 'X')) {	// HexIntegerLiteral?
				for(e = start + 2; e < text.cend(); ++e) {
					if(boost::numeric::in(*e, DIGITS) || boost::numeric::in(*e, CAPITAL_LETTERS) || boost::numeric::in(*e, SMALL_LETTERS))
						continue;
					break;
				}
				if(e == start + 2)
					return boost::none;
			} else {	// DecimalLiteral?
				static const auto isNotDigit = [](Char c) {
					return !boost::numeric::in(c, boost::numeric::interval<Char>('0', '9'));
				};
				bool foundDecimalIntegerLiteral = false, foundDot = false;
				if(boost::numeric::in(start[0], DIGITS)) {	// DecimalIntegerLiteral ::= /0|[1-9][0-9]*/
					e = start + 1;
					foundDecimalIntegerLiteral = true;
					if(start[0] != '0')
						e = std::find_if(e, text.cend(), isNotDigit);
				} else
					e = start;
				if(e < text.cend() && *e == '.') {	// . DecimalDigits ::= /\.[0-9]+/
					foundDot = true;
					e = std::find_if(++e, text.cend(), isNotDigit);
					if(e[-1] == '.')
						return boost::none;
				}
				if(!foundDecimalIntegerLiteral && !foundDot)
					return boost::none;
				if(e < text.cend() && (*e == 'e' || *e == 'E')) {	// ExponentPart ::= /[e|E][\+\-]?[0-9]+/
					if(++e == text.cend())
						return boost::none;
					if(*e == '+' || *e == '-') {
						if(++e == text.cend())
							return boost::none;
					}
					e = std::find_if(++e, text.cend(), isNotDigit);
				}
			}
		
			// e points the end of the found token
			assert(e > start);
			// "The source character immediately following a NumericLiteral must not be an IdentifierStart or DecimalDigit."
			if(e < text.cend() && (boost::numeric::in(*e, DIGITS) || identifierSyntax.isIdentifierStartCharacter(text::utf::decodeFirst(e, text.cend()))))
				return boost::none;

			return e;
		}


		// URIRule ////////////////////////////////////////////////////////////////////////////////////////////////////
		
		/**
		 * Constructor.
		 * @param identifier The identifier of the token which will be returned by the rule
		 * @param uriDetector The URI detector. Can't be @c null
		 * @throw NullPointerException @a uriDetector is @c null
		 */
		URIRule::URIRule(Token::Identifier identifier, std::shared_ptr<const URIDetector> uriDetector) BOOST_NOEXCEPT : Rule(identifier), uriDetector_(uriDetector) {
			if(uriDetector.get() == nullptr)
				throw NullPointerException("uriDetector");
		}
		
		/// @see Rule#parse
		boost::optional<StringPiece::const_iterator> URIRule::parse(const StringPiece& text,
				StringPiece::const_iterator start, const text::IdentifierSyntax& identifierSyntax) const BOOST_NOEXCEPT {
			assert(text.cbegin() < text.cend() && includes(text, start));

			const StringPiece::const_iterator e(uriDetector_->detect(text.substr(start - text.cbegin())));
			return (e != start) ? boost::make_optional(e) : boost::none;
		}


		// WordSetRule ////////////////////////////////////////////////////////////////////////////////////////////////
		
		/**
		 * Constructor.
		 * @param identifier The identifier of the token which will be returned by the rule
		 * @param first The start of the words
		 * @param last The end of the words
		 * @param caseSensitive Set @c false to enable caseless match
		 * @throw NullPointerException @a first and/or @a last are @c null
		 * @throw std#invalid_argument @a first &gt;= @a last
		 */
		WordSetRule::WordSetRule(Token::Identifier identifier, const String* first, const String* last, bool caseSensitive /* = true */) : WordRule(identifier) {
			if(first == nullptr)
				throw NullPointerException("first");
			else if(last == nullptr)
				throw NullPointerException("last");
			else if(first >= last)
				throw std::invalid_argument("first >= last");
			words_.reset(new detail::HashTable(first, last, caseSensitive));
		}
		
		/**
		 * Constructor.
		 * @param identifier The identifier of the token which will be returned by the rule
		 * @param words The string contains the words separated by @a separator
		 * @param separator The separator character in the string
		 * @param caseSensitive Set @c false to enable caseless match
		 * @throw NullPointerException @a first and/or @a last are @c null
		 * @throw text#InvalidScalarValueException @a separator is a surrogate
		 */
		WordSetRule::WordSetRule(Token::Identifier identifier, const StringPiece& words, Char separator, bool caseSensitive) : WordRule(identifier) {
			if(words.cbegin() == nullptr)
				throw NullPointerException("words");
			else if(text::surrogates::isSurrogate(separator))
				throw text::InvalidScalarValueException(separator);
			std::list<String> wordList;
			const Char* p = boost::find_if(words, std::bind(std::not_equal_to<Char>(), separator, std::placeholders::_1));
			for(const Char* next; ; p = ++next) {
				next = std::find(p, words.cend(), separator);
				if(next == p)
					continue;
				wordList.push_back(String(p, next));
				if(next == words.cend())
					break;
			}
			if(wordList.empty())
				throw std::invalid_argument("The input string includes no words.");
			words_.reset(new detail::HashTable(std::begin(wordList), std::end(wordList), caseSensitive));
		}
		
		/// @see Rule#parse
		bool WordSetRule::parse(const StringPiece& text,
				const StringPiece& word, const text::IdentifierSyntax& identifierSyntax) const {
			assert(text.cbegin() < text.cend() && word.cbegin() < word.cend() && includes(text, word));
			return words_->matches(word);
		}


#ifndef ASCENSION_NO_REGEX

		// RegexRule //////////////////////////////////////////////////////////////////////////////////////////////////
		
		/**
		 * Constructor.
		 * @param identifier The identifier of the token which will be returned by the rule
		 * @param pattern The compiled regular expression
		 * @throw regex#PatternSyntaxException The specified pattern is invalid
		 */
		RegexRule::RegexRule(Token::Identifier identifier, std::unique_ptr<const regex::Pattern> pattern) : Rule(identifier), pattern_(std::move(pattern)) {
		}
		
		/// @see Rule#parse
		boost::optional<StringPiece::const_iterator> RegexRule::parse(const StringPiece& text,
				StringPiece::const_iterator start, const text::IdentifierSyntax& identifierSyntax) const {
			assert(text.cbegin() < text.cend() && includes(text, start));

			const auto b(text::utf::makeCharacterDecodeIterator(text.cbegin(), text.cend()));
			const auto e(text::utf::makeCharacterDecodeIterator(text.cbegin(), text.cend(), text.cend()));
			std::unique_ptr<regex::Matcher<text::utf::CharacterDecodeIterator<StringPiece::const_iterator>>> matcher(pattern_->matcher(b, e));
			return matcher->lookingAt() ? boost::make_optional(matcher->end().tell()) : boost::none;
		}

#endif // !ASCENSION_NO_REGEX
	}
}
