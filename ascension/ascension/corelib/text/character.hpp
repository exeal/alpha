/**
 * @file character.hpp
 * @author exeal
 * @date 2005-2011 was unicode.hpp
 * @date 2011-04-27 renamed from unicode.hpp
 * @date 2012-2014
 */

#ifndef ASCENSION_CHARACTER_HPP
#define ASCENSION_CHARACTER_HPP
#include <ascension/platforms.hpp>
#include <array>
#ifdef BOOST_NO_CXX11_CHAR16_T
#	include <cstdint>
#endif
#ifdef ASCENSION_TEST
#	include <iomanip>
#endif // !ASCENSION_TEST
#include <string>

#if ASCENSION_UNICODE_VERSION > 0x0510
#	error These class definitions and implementations are based on old version of Unicode.
#endif

namespace ascension {
	/**
	 * Provides stuffs implement some of the Unicode standard. This includes:
	 * - @c Normalizer class implements <a href="http://www.unicode.org/reports/tr15/">UAX #15:
	 *   Unicode Normalization Forms</a>.
	 * - @c BreakIterator class implements <a href="http://www.unicode.org/reports/tr14/">UAX #14:
	 *   Line Breaking Properties</a> and <a href="http://www.unicode.org/reports/tr29/">UAX #29:
	 *   Text Boundary</a>.
	 * - @c IdentifierSyntax class implements <a href="http://www.unicode.org/reports/tr31/">UAX
	 *   #31: Identifier and Pattern Syntax</a>.
	 * - @c Collator class implements <a href="http://www.unicode.org/reports/tr10/">UTS #10:
	 * Unicode Collation Algorithm</a>.
	 * - @c surrogates namespace provides functions to handle UTF-16 surrogate pairs.
	 * - Unicode properties.
	 * @see ASCENSION_UNICODE_VERSION
	 */
	namespace text {
		/// UTF-16 character type.
#ifdef BOOST_NO_CXX11_CHAR16_T
		typedef std::conditional<
			sizeof(wchar_t) == 2,
			wchar_t,
			std::conditional<
				sizeof(std::uint16_t) == 2,
				std::uint16_t,	// may not be 16-bit
				unsigned short	// may not be 16-bit
			>::type
		>::type Char;
#else
		typedef char16_t Char;
#endif
		static_assert(sizeof(Char) == 2, "Failed to define text.Char type.");

		/// UTF-16 string.
		typedef std::basic_string<Char> String;

		/// Code point of LINE FEED (U+000A).
		const Char LINE_FEED = 0x000au;
		/// Code point of CARRIAGE RETURN (U+000D).
		const Char CARRIAGE_RETURN = 0x000du;
		/// Code point of NEXT LINE (U+0085).
		const Char NEXT_LINE = 0x0085u;
		/// Code point of SUBSTITUTE (U+001A).
		const Char C0_SUBSTITUTE = 0x001au;
		/// Code point of ZERO WIDTH NON-JOINER (U+200C).
		const Char ZERO_WIDTH_NON_JOINER = 0x200cu;
		/// Code point of ZERO WIDTH JOINER (U+200D).
		const Char ZERO_WIDTH_JOINER = 0x200du;
		/// Code point of LINE SEPARATOR (U+2028).
		const Char LINE_SEPARATOR = 0x2028u;
		/// Code point of PARAGRAPH SEPARATOR (U+2029).
		const Char PARAGRAPH_SEPARATOR = 0x2029u;
		/// Code point of REPLACEMENT CHARACTER (U+FFFD).
		const Char REPLACEMENT_CHARACTER = 0xfffdu;
		/// Code point of non-character (U+FFFF).
		const Char NONCHARACTER = 0xffffu;
		/// Set of newline characters.
		/// @see kernel#Newline
		const std::array<Char, 5> NEWLINE_CHARACTERS = {{
			LINE_FEED, CARRIAGE_RETURN, NEXT_LINE, LINE_SEPARATOR, PARAGRAPH_SEPARATOR
		}};
	}

	using text::Char;
	using text::String;
} // namespace ascension.text

#ifndef BOOST_NO_CXX11_CHAR16_T
namespace boost {
	std::size_t hash_value(char16_t c);
}
#endif

#ifdef ASCENSION_TEST
namespace std {
	template<typename CharType, typename CharTraits>
	inline basic_ostream<CharType, CharTraits>& operator<<(basic_ostream<CharType, CharTraits>& out, const ascension::String& value) {
		out << std::setfill(out.widen('0'));
		for(auto i(value.cbegin()), e(value.cend()); i != e; ++i) {
			if(*i < 0x80)
				out << *i;
			else
				out << std::setw(4) << static_cast<std::uint16_t>(*i);
		}
		return out;
	}
}
#endif // ASCENSION_TEST

#endif // !ASCENSION_CHARACTER_HPP
