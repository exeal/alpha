/**
 * @file uri-token-rule.cpp
 * @author exeal
 * @date 2004-2006 (was Lexer.cpp)
 * @date 2006-2014 was rules.cpp
 * @date 2014-01-13 separated from rules.cpp
 */

#include <ascension/corelib/basic-exceptions.hpp>	// NullPointerException
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
		URITokenRule::URITokenRule(Token::Identifier identifier, std::shared_ptr<const URIDetector> uriDetector)
				: TokenRule(identifier), uriDetector_(uriDetector) {
			if(uriDetector.get() == nullptr)
				throw NullPointerException("uriDetector");
		}
		
		/// @see TokenRule#parse
		boost::optional<Index> URITokenRule::matches(const StringPiece& lineString,
				StringPiece::const_iterator at, const text::IdentifierSyntax&) const BOOST_NOEXCEPT {
			assert(lineString.cbegin() < lineString.cend() && at >= lineString.cbegin() && at < lineString.cend());

			const auto e(uriDetector_->detect(lineString.substr(std::distance(lineString.cbegin(), at))));
			return (e != at) ? boost::make_optional<Index>(std::distance(at, e)) : boost::none;
		}
	}
}
