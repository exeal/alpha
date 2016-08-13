/**
 * @file uri-token-rule.cpp
 * @author exeal
 * @date 2004-2006 (was Lexer.cpp)
 * @date 2006-2014 was rules.cpp
 * @date 2014-01-13 separated from rules.cpp
 */

#include <ascension/rules/uri-detector.hpp>
#include <ascension/rules/uri-token-rule.hpp>

namespace ascension {
	namespace rules {
		/**
		 * Creates a @c URITokenRule.
		 * @param identifier The identifier of the token which will be returned by the rule
		 * @param uriDetector The URI detector. Can't be @c null
		 * @throw NullPointerException @a uriDetector is @c null
		 */
		URITokenRule::URITokenRule(Token::Identifier identifier, std::shared_ptr<const URIDetector> uriDetector) BOOST_NOEXCEPT
				: TokenRule(identifier), uriDetector_(uriDetector) {
			if(uriDetector.get() == nullptr)
				throw NullPointerException("uriDetector");
		}
		
		/// @see Rule#parse
		boost::optional<StringPiece::const_iterator> URITokenRule::parse(const StringPiece& text,
				StringPiece::const_iterator start, const text::IdentifierSyntax& identifierSyntax) const BOOST_NOEXCEPT {
			assert(text.cbegin() < text.cend() && start >= text.cbegin() && start < text.cend());

			const StringPiece::const_iterator e(uriDetector_->detect(text.substr(start - text.cbegin())));
			return (e != start) ? boost::make_optional(e) : boost::none;
		}
	}
}
