/**
 * @file word-set-token-rule.cpp
 * @author exeal
 * @date 2004-2006 (was Lexer.cpp)
 * @date 2006-2014 was rules.cpp
 * @date 2014-01-13 separated from rules.cpp
 * @date 2016-08-13 Separated from token-rules.cpp.
 */

#include <ascension/rules/hash-table.hpp>
#include <ascension/rules/word-set-token-rule.hpp>
#include <boost/range/algorithm/find_if.hpp>

namespace ascension {
	namespace rules {		
		/**
		 * Creates a @c WordSetTokenRule instance.
		 * @param identifier The identifier of the token which will be returned by the rule
		 * @param first The start of the words
		 * @param last The end of the words
		 * @param caseSensitive Set @c false to enable caseless match
		 * @throw NullPointerException @a first and/or @a last are @c null
		 * @throw std#invalid_argument @a first &gt;= @a last
		 */
		WordSetTokenRule::WordSetTokenRule(Token::Identifier identifier,
				const String* first, const String* last, bool caseSensitive /* = true */) : WordTokenRule(identifier) {
			if(first == nullptr)
				throw NullPointerException("first");
			else if(last == nullptr)
				throw NullPointerException("last");
			else if(first >= last)
				throw std::invalid_argument("first >= last");
			words_.reset(new detail::HashTable(first, last, caseSensitive));
		}
		
		/**
		 * Creates a @c WordSetTokenRule instance.
		 * @param identifier The identifier of the token which will be returned by the rule
		 * @param words The string contains the words separated by @a separator
		 * @param separator The separator character in the string
		 * @param caseSensitive Set @c false to enable caseless match
		 * @throw NullPointerException @a first and/or @a last are @c null
		 * @throw text#InvalidScalarValueException @a separator is a surrogate
		 */
		WordSetTokenRule::WordSetTokenRule(Token::Identifier identifier,
				const StringPiece& words, Char separator, bool caseSensitive) : WordTokenRule(identifier) {
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
		bool WordSetTokenRule::parse(const StringPiece& text,
				const StringPiece& word, const text::IdentifierSyntax& identifierSyntax) const {
			assert(text.cbegin() < text.cend() && word.cbegin() < word.cend() && word >= text.cbegin() && word < text.cend());
			return words_->matches(word);
		}
	}
}
