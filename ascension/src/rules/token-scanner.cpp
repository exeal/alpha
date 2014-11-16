/**
 * @file token-scanner.cpp
 * @author exeal
 * @date 2004-2006 (was Lexer.cpp)
 * @date 2006-2014 was rules.cpp
 * @date 2014-01-13 separated from rules.cpp
 * @date 2014-11-16 Separated from scanner.cpp
 */

#include <ascension/corelib/text/identifier-syntax.hpp>
#include <ascension/rules/token.hpp>
#include <ascension/rules/token-scanner.hpp>

namespace ascension {
	namespace rules {		
		/// @see TokenScanner#hasNext
		bool NullTokenScanner::hasNext() const BOOST_NOEXCEPT {
			return false;
		}
		
		/// @see TokenScanner#identifierSyntax
		const text::IdentifierSyntax& NullTokenScanner::identifierSyntax() const BOOST_NOEXCEPT {
			return text::IdentifierSyntax::defaultInstance();
		}
		
		/// @see TokenScanner#nextToken
		std::unique_ptr<Token> NullTokenScanner::nextToken() {
			return std::unique_ptr<Token>();
		}
		
		/// @see TokenScanner#parse
		void NullTokenScanner::parse(const kernel::Document&, const kernel::Region&) {
		}
		
		/// @see TokenScanner#position
		kernel::Position NullTokenScanner::position() const {
			if(!position_)
				throw BadScannerStateException();
			return *position_;
		}
	}
}
