/**
 * @file token-rules.hpp
 * @author exeal
 * @date 2004-2006 was Lexer.h
 * @date 2006-2014
 */

#ifndef ASCENSION_TOKEN_RULES_HPP
#define ASCENSION_TOKEN_RULES_HPP
#include <ascension/corelib/string-piece.hpp>
#include <ascension/rules/token.hpp>

namespace ascension {
	namespace text {
		class IdentifierSyntax;
	}

	/// Provides a framework for rule based text scanning and document partitioning.
	namespace rules {
		/// Base class of the two abstract rule classes.
		class TokenRuleBase {
		public:
			/// Destructor.
			virtual ~TokenRuleBase() BOOST_NOEXCEPT {}
			/// Returns the identifier of the token.
			Token::Identifier tokenID() const BOOST_NOEXCEPT {return identifier_;}

		protected:
			explicit TokenRuleBase(Token::Identifier tokenID);
		private:
			const Token::Identifier identifier_;
		};

		/**
		 * Base class of non-word token rule classes.
		 * @see LexicalTokenScanner, RegionTokenRule, NumberTokenRule, RegexTokenRule
		 */
		class TokenRule : public TokenRuleBase {
		public:
			/// Destructor.
			virtual ~TokenRule() BOOST_NOEXCEPT {}
			/**
			 * Returns a token starts at the specified position in the line.
			 * @param lineString The text string of the line
			 * @param at The start position of the token in @a lineString
			 * @param identifierSyntax The identifier syntax
			 * @return The length of the found token, or @c boost::none if not found
			 * @note @a lineString is neither @c null nor empty
			 */
			virtual boost::optional<Index> matches(
				const StringPiece& lineString, StringPiece::const_iterator at,
				const text::IdentifierSyntax& identifierSyntax) const BOOST_NOEXCEPT = 0;

		protected:
			explicit TokenRule(Token::Identifier tokenID);
		};
	}
} // namespace ascension.rules

#endif // !ASCENSION_TOKEN_RULES_HPP
