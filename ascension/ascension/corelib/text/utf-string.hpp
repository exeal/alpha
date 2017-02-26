/**
 * @file utf-string.hpp
 * Defines @c ascension#text#utf#fromString and @c ascension#text#utf#toString free functions.
 * @author exeal
 * @date 2017-02-26 Created.
 */

#ifndef ASCENSION_UTF_STRING_HPP
#define ASCENSION_UTF_STRING_HPP
#include <ascension/corelib/text/utf-iterator.hpp>

namespace ascension {
	namespace text {
		namespace utf {
			template<typename UtfString>
			inline UtfString fromString(const StringPiece& s, typename std::enable_if<CodeUnitSizeOf<typename boost::range_iterator<UtfString>::type>::value != sizeof(Char)>::type* = nullptr) {
				const std::basic_string<CodePoint> temp(makeCharacterDecodeIterator(s.cbegin(), s.cend()), makeCharacterDecodeIterator(s.cbegin(), s.cend(), s.cend()));
				typedef CodeUnitTraits<CodeUnitSizeOf<UtfString>::value>::value_type CodeUnit;
				return UtfString(makeCharacterEncodeIterator<CodeUnit>(temp.cbegin()), makeCharacterEncodeIterator<CodeUnit>(temp.cend()));
			}

			template<typename UtfString>
			inline UtfString fromString(const StringPiece& s, typename std::enable_if<CodeUnitSizeOf<typename boost::range_iterator<UtfString>::type>::value == sizeof(Char)>::type* = nullptr) {
				return UtfString(s.cbegin(), s.cend());
			}

			template<typename UtfString>
			inline String toString(const UtfString& s, typename std::enable_if<CodeUnitSizeOf<typename boost::range_iterator<UtfString>::type>::value != sizeof(Char)>::type* = nullptr) {
				const std::basic_string<CodePoint> temp(makeCharacterDecodeIterator(s.cbegin(), s.cend()), makeCharacterDecodeIterator(s.cbegin(), s.cend(), s.cend()));
				return String(makeCharacterEncodeIterator<Char>(temp.cbegin()), makeCharacterEncodeIterator<Char>(temp.cend()));
			}

			template<typename UtfString>
			inline String toString(const UtfString& s, typename std::enable_if<CodeUnitSizeOf<typename boost::range_iterator<UtfString>::type>::value == sizeof(Char)>::type* = nullptr) {
				return String(s.cbegin(), s.cend());
			}
		}
	}
}

#endif // !ASCENSION_UTF_STRING_HPP
