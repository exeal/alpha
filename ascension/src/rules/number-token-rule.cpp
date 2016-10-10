/**
 * @file number-token-rule.cpp
 * @author exeal
 * @date 2004-2006 (was Lexer.cpp)
 * @date 2006-2014 was rules.cpp
 * @date 2014-01-13 separated from rules.cpp
 * @date 2016-08-13 Separated from token-rules.cpp.
 */

#include <ascension/corelib/text/identifier-syntax.hpp>
#include <ascension/rules/number-token-rule.hpp>
#include <boost/numeric/interval.hpp>

namespace ascension {
	namespace rules {
		/**
		 * Creates a @c NumberTokenRule.
		 * @param identifier The identifier of the token which will be returned by the rule
		 */
		NumberTokenRule::NumberTokenRule(Token::Identifier identifier) BOOST_NOEXCEPT : TokenRule(identifier) {
		}
		
		/// @see TokenRule#matches
		boost::optional<Index> NumberTokenRule::matches(const StringPiece& lineString,
				StringPiece::const_iterator at, const text::IdentifierSyntax& identifierSyntax) const BOOST_NOEXCEPT {
			assert(lineString.cbegin() < lineString.cend() && at >= lineString.cbegin() && at < lineString.cend());

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
			if(at > lineString.cbegin()	// see below
					&& (boost::numeric::in(at[-1], DIGITS) || boost::numeric::in(at[-1], CAPITAL_LETTERS) || boost::numeric::in(at[-1], SMALL_LETTERS)))
				return boost::none;
			StringPiece::const_iterator e;
			if(std::distance(at, lineString.cend()) > 2 && at[0] == '0' && (at[1] == 'x' || at[1] == 'X')) {	// HexIntegerLiteral?
				for(e = std::next(at, 2); e < lineString.cend(); ++e) {
					if(boost::numeric::in(*e, DIGITS) || boost::numeric::in(*e, CAPITAL_LETTERS) || boost::numeric::in(*e, SMALL_LETTERS))
						continue;
					break;
				}
				if(e == std::next(at, 2))
					return boost::none;
			} else {	// DecimalLiteral?
				static const auto isNotDigit = [](Char c) {
					return !boost::numeric::in(c, boost::numeric::interval<Char>('0', '9'));
				};
				bool foundDecimalIntegerLiteral = false, foundDot = false;
				if(boost::numeric::in(at[0], DIGITS)) {	// DecimalIntegerLiteral ::= /0|[1-9][0-9]*/
					e = std::next(at);
					foundDecimalIntegerLiteral = true;
					if(at[0] != '0')
						e = std::find_if(e, lineString.cend(), isNotDigit);
				} else
					e = at;
				if(e < lineString.cend() && *e == '.') {	// . DecimalDigits ::= /\.[0-9]+/
					foundDot = true;
					e = std::find_if(++e, lineString.cend(), isNotDigit);
					if(e[-1] == '.')
						return boost::none;
				}
				if(!foundDecimalIntegerLiteral && !foundDot)
					return boost::none;
				if(e < lineString.cend() && (*e == 'e' || *e == 'E')) {	// ExponentPart ::= /[e|E][\+\-]?[0-9]+/
					if(++e == lineString.cend())
						return boost::none;
					if(*e == '+' || *e == '-') {
						if(++e == lineString.cend())
							return boost::none;
					}
					if(!boost::numeric::in(*e, DIGITS))
						return boost::none;
					e = std::find_if(++e, lineString.cend(), isNotDigit);
				}
			}
		
			// e points the end of the found token
			assert(e > at);
			// "The source character immediately following a NumericLiteral must not be an IdentifierStart or DecimalDigit."
			if(e < lineString.cend() && (boost::numeric::in(*e, DIGITS) || identifierSyntax.isIdentifierStartCharacter(text::utf::decodeFirst(e, lineString.cend()))))
				return boost::none;

			return boost::make_optional<Index>(std::distance(at, e));
		}
	}
}
