/**
 * @file character.hpp
 * @author exeal
 * @date 2005-2011 was unicode.hpp
 * @date 2011-04-27 renamed from unicode.hpp
 */

#ifndef ASCENSION_CHARACTER_HPP
#define ASCENSION_CHARACTER_HPP

#include <ascension/corelib/basic-types.hpp>
#include <ascension/corelib/text/unicode-surrogates.hpp>

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
		/// Invalid code point value.
		const CodePoint INVALID_CODE_POINT = 0xfffffffful;
		/// Set of newline characters.
		/// @see kernel#Newline
		const Char NEWLINE_CHARACTERS[] = {
			LINE_FEED, CARRIAGE_RETURN, NEXT_LINE, LINE_SEPARATOR, PARAGRAPH_SEPARATOR};

		/// Returns @c true if the specified code point is in Unicode codespace (0..10FFFF).
		inline bool isValidCodePoint(CodePoint c) /*throw()*/ {return c <= 0x10fffful;}

		/// Returns @c true if the specified code point is Unicode scalar value.
		inline bool isScalarValue(CodePoint c) /*throw()*/ {
			return isValidCodePoint(c) && !surrogates::isSurrogate(c);
		}

		/// Case sensitivities for caseless-match.
		enum CaseSensitivity {
			CASE_SENSITIVE,							///< Case-sensitive.
			CASE_INSENSITIVE,						///< Case-insensitive.
			CASE_INSENSITIVE_EXCLUDING_TURKISH_I	///< Case-insensitive and excludes Turkish I.
		};

		/// Types of decomposition mapping.
		enum Decomposition {
			NO_DECOMPOSITION,			///< No decomposition.
			CANONICAL_DECOMPOSITION,	///< Canonical decomposition mapping.
			FULL_DECOMPOSITION			///< Canonical and compatibility mapping.
		};

	}
} // namespace ascension.text

#endif // !ASCENSION_CHARACTER_HPP
