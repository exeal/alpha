/**
 * @file character.hpp
 * @author exeal
 * @date 2005-2011 was unicode.hpp
 * @date 2011-04-27 renamed from unicode.hpp
 */

#ifndef ASCENSION_CHARACTER_HPP
#define ASCENSION_CHARACTER_HPP

#include <ascension/corelib/basic-types.hpp>

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

		/**
		 * Returns the size of a code unit of the specified code unit sequence in bytes.
		 * @tparam CodeUnitSequence The type represents a code unit sequence
		 */
		template<typename CodeUnitSequence> struct CodeUnitSizeOf {
			/// Byte size of the code unit.
			static const std::size_t value =
				sizeof(typename std::iterator_traits<CodeUnitSequence>::value_type);
		};
		template<typename T> struct CodeUnitSizeOf<std::back_insert_iterator<T> > {
			static const std::size_t value = sizeof(T::value_type);
		};
		template<typename T> struct CodeUnitSizeOf<std::front_insert_iterator<T> > {
			static const std::size_t value = sizeof(T::value_type);
		};
		template<typename T, typename U> struct CodeUnitSizeOf<std::ostream_iterator<T, U> > {
			static const std::size_t value = sizeof(T);
		};

		/**
		 * @c surrogates namespace collects low level procedures handle UTF-16 surrogate pair.
		 * @see UTF16To32Iterator, UTF32To16Iterator, utf-16.hpp
		 */
		namespace surrogates {
			/**
			 * Returns @c true if the specified code point is supplemental (out of BMP).
			 * @param c The code point
			 * @return true if @a c is supplemental
			 */
			inline bool isSupplemental(CodePoint c) /*throw()*/ {
				return (c & 0xffff0000ul) != 0;
			}

			/**
			 * Returns @c true if the specified code unit is high (leading)-surrogate.
			 * @param c The code point
			 * @return true if @a c is high-surrogate
			 */
			inline bool isHighSurrogate(CodePoint c) /*throw()*/ {
				return (c & 0xfffffc00ul) == 0xd800u;
			}

			/**
			 * Returns @c true if the specified code unit is low (trailing)-surrogate.
			 * @param c The code point
			 * @return true if @a c is low-surrogate
			 */
			inline bool isLowSurrogate(CodePoint c) /*throw()*/ {
				return (c & 0xfffffc00ul) == 0xdc00u;
			}

			/**
			 * Returns @c true if the specified code unit is surrogate.
			 * @param c The code point
			 * @return true if @a c is surrogate
			 */
			inline bool isSurrogate(CodePoint c) /*throw()*/ {
				return (c & 0xfffff800ul) == 0xd800u;
			}

			/**
			 * Returns high (leading)-surrogate for the specified code point.
			 * @note If @a c is in BMP, the behavior is undefined.
			 * @param c The code point
			 * @return The high-surrogate code unit for @a c
			 */
			inline Char highSurrogate(CodePoint c) /*throw()*/ {
				return static_cast<Char>((c >> 10) & 0xffffu) + 0xd7c0u;
			}

			/**
			 * Returns low (trailing)-surrogate for the specified code point.
			 * @note If @a c is in BMP, the behavior is undefined.
			 * @param c The code point
			 * @return The low-surrogate code unit for @a c
			 */
			inline Char lowSurrogate(CodePoint c) /*throw()*/ {
				return static_cast<Char>(c & 0x03ffu) | 0xdc00u;
			}

			/**
			 * Converts the specified surrogate pair to a corresponding code point.
			 * @param high The high-surrogate
			 * @param low The low-surrogate
			 * @return The code point or the value of @a high if the pair is not valid
			 */
			inline CodePoint decode(Char high, Char low) /*throw()*/ {
				return (isHighSurrogate(high) && isLowSurrogate(low)) ?
					0x10000ul + (high - 0xd800u) * 0x0400u + low - 0xdc00u : high;
			}

			/**
			 * Searches an isolated surrogate character in the given UTF-16 code unit sequence.
			 * @note About UTF-32 code unit sequence, use <code>std#find_if(,,
			 *       std#ptr_fun(isSurrogate))</code> instead.
			 * @tparam InputIterator The input iterator represents a UTF-16 character sequence
			 * @param first The beginning of the character sequence
			 * @param last The end of the sequence
			 * @return The isolated surrogate or @a last if not found
			 */
			template<typename InputIterator>
			inline InputIterator searchIsolatedSurrogate(
					InputIterator first, InputIterator last) /*throw()*/ {
				ASCENSION_STATIC_ASSERT(CodeUnitSizeOf<InputIterator>::value == 2);
				while(first != last) {
					if(isLowSurrogate(*first))
						break;
					else if(isHighSurrogate(*first)) {
						const InputIterator high(first);
						if(++first == last || !isLowSurrogate(*first))
							return high;
					}
					++first;
				}
				return first;
			}
		} // namespace surrogates

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
