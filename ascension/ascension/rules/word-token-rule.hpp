/**
 * @file word-token-rule.hpp
 * @author exeal
 * @date 2004-2006 was Lexer.h
 * @date 2016-08-13 Separated from token-rules.hpp.
 */

#ifndef ASCENSION_WORD_TOKEN_RULE_HPP
#define ASCENSION_WORD_TOKEN_RULE_HPP
#include <ascension/rules/token-rule.hpp>

namespace ascension {
	namespace rules {
		/**
		 * Base class of word rule classes.
		 * @see LexicalTokenScanner, WordSetRule
		 */
		class WordTokenRule : public TokenRuleBase {
		public:
			/// Destructor.
			virtual ~WordTokenRule() BOOST_NOEXCEPT {}
			/**
			 * Returns a token starts at the specified position in the line.
			 * @param lineString The text string of the line
			 * @param word The range representing the word to check if is a token
			 * @param identifierSyntax The identifier syntax
			 * @return @c true if @a word is a token
			 * @note @a lineString is neither @c null nor empty
			 */
			virtual bool matches(const StringPiece& lineString, const StringPiece& word,
				const text::IdentifierSyntax& identifierSyntax) const BOOST_NOEXCEPT = 0;

		protected:
			explicit WordTokenRule(Token::Identifier tokenID) : TokenRuleBase(tokenID) {}
		};
	}
} // namespace ascension.rules

#endif // !ASCENSION_WORD_TOKEN_RULE_HPP
