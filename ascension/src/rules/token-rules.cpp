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
#include <ascension/rules/token-scanner.hpp>
#include <boost/foreach.hpp>
#include <boost/numeric/interval.hpp>
#include <boost/range/algorithm/find_if.hpp>


namespace ascension {
	namespace rules {
		// Token //////////////////////////////////////////////////////////////////////////////////////////////////////

		const Token::Identifier Token::UNCALCULATED = static_cast<Token::Identifier>(-1);


		// Rule ///////////////////////////////////////////////////////////////////////////////////////////////////////
		
		/**
		 * Protected constructor.
		 * @param tokenID The identifier of the token which will be returned by the rule. Can't be
		 *                @c Token#UNCALCULATED which is for internal use
		 * @throw std#invalid_argument @a tokenID is invalid
		 */
		Rule::Rule(Token::Identifier tokenID) : id_(tokenID) {
			if(tokenID == Token::UNCALCULATED)
				throw std::invalid_argument("tokenID");
		}


		// RegionRule /////////////////////////////////////////////////////////////////////////////////////////////////
		
		/**
		 * Constructor.
		 * @param id The identifier of the token which will be returned by the rule
		 * @param startSequence The pattern's start sequence
		 * @param endSequence The pattern's end sequence. if empty, token will end at end of line
		 * @param escapeCharacter The character which a character will be ignored
		 * @param caseSensitive Set @c false to enable caseless match
		 * @throw std#invalid_argument @a startSequence is empty
		 */
		RegionRule::RegionRule(Token::Identifier id, const String& startSequence, const String& endSequence,
				Char escapeCharacter /* = NONCHARACTER */, bool caseSensitive /* = true */) : Rule(id),
				startSequence_(startSequence), endSequence_(endSequence), escapeCharacter_(escapeCharacter), caseSensitive_(caseSensitive) {
			if(startSequence.empty())
				throw std::invalid_argument("The start sequence is empty.");
		}
		
		/// @see Rule#parse
		std::unique_ptr<Token> RegionRule::parse(const TokenScanner& scanner, const StringPiece& text) const BOOST_NOEXCEPT {
			// match the start sequence
			if(text[0] != startSequence_[0]
					|| static_cast<std::size_t>(text.length()) < startSequence_.length() + endSequence_.length()
					|| (startSequence_.length() > 1 && umemcmp(text.cbegin() + 1, startSequence_.data() + 1, startSequence_.length() - 1) != 0))
				return std::unique_ptr<Token>();
			StringPiece::const_iterator end(text.cend());
			if(!endSequence_.empty()) {
				// search the end sequence
				for(StringPiece::const_iterator p(text.cbegin() + startSequence_.length()); p <= text.cend() - endSequence_.length(); ++p) {
					if(escapeCharacter_ != text::NONCHARACTER && *p == escapeCharacter_)
						++p;
					else if(*p == endSequence_[0] && umemcmp(p + 1, endSequence_.data() + 1, endSequence_.length() - 1) == 0) {
						end = p + endSequence_.length();
						break;
					}
				}
			}
			std::unique_ptr<Token> result(new Token);
			result->id = tokenID();
			result->region.first.line = result->region.second.line = scanner.position().line;
			result->region.first.offsetInLine = scanner.position().line;
			result->region.second.offsetInLine = result->region.first.offsetInLine + (end - text.cbegin());
			return result;
		}
		
		
		// NumberRule /////////////////////////////////////////////////////////////////////////////////////////////////
		
		/**
		 * Constructor.
		 * @param id The identifier of the token which will be returned by the rule
		 */
		NumberRule::NumberRule(Token::Identifier id) BOOST_NOEXCEPT : Rule(id) {
		}
		
		/// @see Rule#parse
		std::unique_ptr<Token> NumberRule::parse(const TokenScanner& scanner, const StringPiece& text) const BOOST_NOEXCEPT {
			assert(text.cbegin() < text.cend());
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
			if(scanner.position().offsetInLine > 0	// see below
					&& (boost::numeric::in(text[-1], DIGITS) || boost::numeric::in(text[-1], CAPITAL_LETTERS) || boost::numeric::in(text[-1], SMALL_LETTERS)))
				return std::unique_ptr<Token>();
			StringPiece::const_iterator e;
			if(text.length() > 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {	// HexIntegerLiteral?
				for(e = text.cbegin() + 2; e < text.cend(); ++e) {
					if(boost::numeric::in(*e, DIGITS) || boost::numeric::in(*e, CAPITAL_LETTERS) || boost::numeric::in(*e, SMALL_LETTERS))
						continue;
					break;
				}
				if(e == text.cbegin() + 2)
					return std::unique_ptr<Token>();
			} else {	// DecimalLiteral?
				static const auto isNotDigit = [](Char c) {
					return !boost::numeric::in(c, boost::numeric::interval<Char>('0', '9'));
				};
				bool foundDecimalIntegerLiteral = false, foundDot = false;
				if(boost::numeric::in(text[0], DIGITS)) {	// DecimalIntegerLiteral ::= /0|[1-9][0-9]*/
					e = text.cbegin() + 1;
					foundDecimalIntegerLiteral = true;
					if(text[0] != '0')
						e = std::find_if(e, text.cend(), isNotDigit);
				} else
					e = text.cbegin();
				if(e < text.cend() && *e == '.') {	// . DecimalDigits ::= /\.[0-9]+/
					foundDot = true;
					e = std::find_if(++e, text.cend(), isNotDigit);
					if(e[-1] == '.')
						return std::unique_ptr<Token>();
				}
				if(!foundDecimalIntegerLiteral && !foundDot)
					return std::unique_ptr<Token>();
				if(e < text.cend() && (*e == 'e' || *e == 'E')) {	// ExponentPart ::= /[e|E][\+\-]?[0-9]+/
					if(++e == text.cend())
						return std::unique_ptr<Token>();
					if(*e == '+' || *e == '-') {
						if(++e == text.cend())
							return std::unique_ptr<Token>();
					}
					e = std::find_if(++e, text.cend(), isNotDigit);
				}
			}
		
			// e points the end of the found token
			assert(e > text.cbegin());
			// "The source character immediately following a NumericLiteral must not be an IdentifierStart or DecimalDigit."
			if(e < text.cend() && (boost::numeric::in(*e, DIGITS) || scanner.identifierSyntax().isIdentifierStartCharacter(text::utf::decodeFirst(e, text.cend()))))
				return std::unique_ptr<Token>();
		
			std::unique_ptr<Token> temp(new Token);
			temp->id = tokenID();
			temp->region.first.line = temp->region.second.line = scanner.position().line;
			temp->region.first.offsetInLine = scanner.position().offsetInLine;
			temp->region.second.offsetInLine = temp->region.first.offsetInLine + e - text.cbegin();
			return temp;
		}


		// URIRule ////////////////////////////////////////////////////////////////////////////////////////////////////
		
		/**
		 * Constructor.
		 * @param id The identifier of the token which will be returned by the rule
		 * @param uriDetector The URI detector. Can't be @c null
		 * @throw NullPointerException @a uriDetector is @c null
		 */
		URIRule::URIRule(Token::Identifier id, std::shared_ptr<const URIDetector> uriDetector) BOOST_NOEXCEPT : Rule(id), uriDetector_(uriDetector) {
			if(uriDetector.get() == nullptr)
				throw NullPointerException("uriDetector");
		}
		
		/// @see Rule#parse
		std::unique_ptr<Token> URIRule::parse(const TokenScanner& scanner, const StringPiece& text) const BOOST_NOEXCEPT {
			assert(text.cbegin() < text.cend());
			const StringPiece::const_iterator e(uriDetector_->detect(text));
			if(e == text.cbegin())
				return std::unique_ptr<Token>();
			std::unique_ptr<Token> temp(new Token);
			temp->id = tokenID();
			temp->region.first.line = temp->region.second.line = scanner.position().line;
			temp->region.first.offsetInLine = scanner.position().offsetInLine;
			temp->region.second.offsetInLine = temp->region.first.offsetInLine + e - text.cbegin();
			return temp;
		}


		// WordRule ///////////////////////////////////////////////////////////////////////////////////////////////////
		
		/**
		 * Constructor.
		 * @param id The identifier of the token which will be returned by the rule
		 * @param first The start of the words
		 * @param last The end of the words
		 * @param caseSensitive Set @c false to enable caseless match
		 * @throw NullPointerException @a first and/or @a last are @c null
		 * @throw std#invalid_argument @a first &gt;= @a last
		 */
		WordRule::WordRule(Token::Identifier id, const String* first, const String* last, bool caseSensitive /* = true */) : Rule(id) {
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
		 * @param id The identifier of the token which will be returned by the rule
		 * @param words The string contains the words separated by @a separator
		 * @param separator The separator character in the string
		 * @param caseSensitive Set @c false to enable caseless match
		 * @throw NullPointerException @a first and/or @a last are @c null
		 * @throw text#InvalidScalarValueException @a separator is a surrogate
		 */
		WordRule::WordRule(Token::Identifier id, const StringPiece& words, Char separator, bool caseSensitive) : Rule(id) {
			if(words.cbegin() == nullptr)
				throw NullPointerException("words");
			else if(text::surrogates::isSurrogate(separator))
				throw text::InvalidScalarValueException(separator);
			std::list<String> wordList;
			const Char* p = boost::find_if(words, std::bind(std::not_equal_to<Char>(), separator, std::placeholders::_1));
			for(const Char* next; ; p = ++next) {
				next = std::find(p, words.end(), separator);
				if(next == p)
					continue;
				wordList.push_back(String(p, next));
				if(next == words.end())
					break;
			}
			if(wordList.empty())
				throw std::invalid_argument("The input string includes no words.");
			words_.reset(new detail::HashTable(std::begin(wordList), std::end(wordList), caseSensitive));
		}
		
		/// @see Rule#parse
		std::unique_ptr<Token> WordRule::parse(const TokenScanner& scanner, const StringPiece& text) const {
			if(!words_->matches(text))
				return std::unique_ptr<Token>();
			std::unique_ptr<Token> result(new Token);
			result->id = tokenID();
			result->region.first.line = result->region.second.line = scanner.position().line;
			result->region.first.offsetInLine = scanner.position().offsetInLine;
			result->region.second.offsetInLine = result->region.first.offsetInLine + text.length();
			return result;
		}


#ifndef ASCENSION_NO_REGEX

		// RegexRule //////////////////////////////////////////////////////////////////////////////////////////////////
		
		/**
		 * Constructor.
		 * @param id The identifier of the token which will be returned by the rule
		 * @param pattern The compiled regular expression
		 * @throw regex#PatternSyntaxException The specified pattern is invalid
		 */
		RegexRule::RegexRule(Token::Identifier id, std::unique_ptr<const regex::Pattern> pattern) : Rule(id), pattern_(std::move(pattern)) {
		}
		
		/// @see Rule#parse
		std::unique_ptr<Token> RegexRule::parse(const TokenScanner& scanner, const StringPiece& text) const {
			const text::utf::CharacterDecodeIterator<StringPiece::const_iterator> b(text.cbegin(), text.cend()), e(text.cbegin(), text.cend(), text.cend());
			std::unique_ptr<regex::Matcher<text::utf::CharacterDecodeIterator<StringPiece::const_iterator>>> matcher(pattern_->matcher(b, e));
			if(!matcher->lookingAt())
				return std::unique_ptr<Token>();
			std::unique_ptr<Token> token(new Token);
			token->id = tokenID();
			token->region.first.line = token->region.second.line = scanner.position().line;
			token->region.first.offsetInLine = scanner.position().offsetInLine;
			token->region.second.offsetInLine = token->region.first.offsetInLine + (matcher->end().tell() - matcher->start().tell());
			return token;
		}

#endif // !ASCENSION_NO_REGEX
	}
}
