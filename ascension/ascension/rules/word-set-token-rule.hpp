/**
 * @file word-set-token-rule.hpp
 * @author exeal
 * @date 2004-2006 was Lexer.h
 * @date 2016-08-13 Separated from token-rules.hpp.
 */

#ifndef ASCENSION_WORD_SET_TOKEN_RULE_HPP
#define ASCENSION_WORD_SET_TOKEN_RULE_HPP
#include <ascension/rules/hash-table.hpp>
#include <ascension/rules/word-token-rule.hpp>

namespace ascension {
	namespace rules {
		class HashTable;

		/// A concrete rule detects the registered words.
		class WordSetTokenRule : protected WordTokenRule {
		public:
			WordSetTokenRule(Token::Identifier identifier,
				const String* first, const String* last, bool caseSensitive = true);
			WordSetTokenRule(Token::Identifier identifier,
				const StringPiece& words, Char separator, bool caseSensitive = true);
			bool parse(const StringPiece& text, const StringPiece& word,
				const text::IdentifierSyntax& identifierSyntax) const BOOST_NOEXCEPT override;

		private:
			std::unique_ptr<detail::HashTable> words_;
		};
	}
} // namespace ascension.rules

#endif // !ASCENSION_WORD_SET_TOKEN_RULE_HPP
