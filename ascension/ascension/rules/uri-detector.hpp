/**
 * @file uri-detector.hpp
 * @author exeal
 * @date 2004-2006 was Lexer.h
 * @date 2006-2014 was rules.hpp
 * @date 2014-11-16 Separated from rules.hpp
 */

#ifndef ASCENSION_URI_DETECTOR_HPP
#define ASCENSION_URI_DETECTOR_HPP
#include <ascension/corelib/string-piece.hpp>
#include <boost/core/noncopyable.hpp>
#include <memory>
#include <set>

namespace ascension {
	namespace rules {
		namespace detail {
			class HashTable;
		}

		/**
		 * A @c URIDetector detects and searches URI. This class conforms to the syntaxes of
		 * <a href="http://www.ietf.org/rfc/rfc3986.txt">RFC 3986</a> and
		 * <a href="http://www.ietf.org/rfc/rfc3987.txt">RFC 3987</a>.
		 */
		class URIDetector : private boost::noncopyable {
		public:
			/// @name Parsing
			/// @{
			StringPiece::const_iterator detect(const StringPiece& text) const;
			StringPiece search(const StringPiece& text) const;
			/// @}

			/// @name Attribute
			/// @{
			URIDetector& setValidSchemes(const std::set<String>& schemes, bool caseSensitive = false);
			URIDetector& setValidSchemes(const StringPiece& schemes, Char separator, bool caseSensitive = false);
			/// @}

			/// @name Default Instances
			/// @{
			static const URIDetector& defaultGenericInstance() BOOST_NOEXCEPT;
			static const URIDetector& defaultIANAURIInstance() BOOST_NOEXCEPT;
			/// @}

		private:
			std::unique_ptr<detail::HashTable> validSchemes_;
		};
	}
} // namespace ascension.rules

#endif // !ASCENSION_URI_DETECTOR_HPP
