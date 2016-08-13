/**
 * @file token-rules.cpp
 * @author exeal
 * @date 2004-2006 (was Lexer.cpp)
 * @date 2006-2014 was rules.cpp
 * @date 2014-01-13 separated from rules.cpp
 */

#include <ascension/rules/token-rule.hpp>

namespace ascension {
	namespace rules {
		const Token::Identifier Token::UNCALCULATED = static_cast<Token::Identifier>(-1);
		
		/**
		 * Protected constructor.
		 * @param tokenID The identifier of the token which will be returned by the rule. Can't be
		 *                @c Token#UNCALCULATED which is for internal use
		 * @throw std#invalid_argument @a tokenID is invalid
		 */
		TokenRuleBase::TokenRuleBase(Token::Identifier tokenID) : identifier_(tokenID) {
			if(tokenID == Token::UNCALCULATED)
				throw std::invalid_argument("tokenID");
		}

		/**
		 * Creates a @c TokenRule with the given token identifier.
		 * @param tokenID The token identifier to pass to @c TokenRuleBase
		 * @throw std#invalid_argument @a tokenID is invalid
		 */
		TokenRule::TokenRule(Token::Identifier tokenID) : TokenRuleBase(tokenID) {
		}
	}
}
