/**
 * @file utf-16.hpp
 * @author exeal
 * @date 2005-2010 was unicode.hpp
 * @date 2010 was character-iterator.hpp
 * @date 2010-11-06 separated from character-iterator.hpp to unicode-surrogates.hpp and
 *                  unicode-utf.hpp
 * @date 2011-08-20 joined unicode-surrogates.hpp and unicode-utf.hpp
 */

#ifndef ASCENSION_UTF_16_HPP
#define ASCENSION_UTF_16_HPP

#include <ascension/corelib/text/character.hpp>	// CodePoint, ASCENSION_STATIC_ASSERT, surrogates.*
#include <cassert>								// assert

#if ASCENSION_UNICODE_VERSION > 0x0510
#error These class definitions and implementations are based on old version of Unicode.
#endif

namespace ascension {

	namespace detail {
		template<bool check, typename OutputIterator>
		inline std::size_t encodeUTF16(CodePoint c, OutputIterator& out) {
			ASCENSION_STATIC_ASSERT(text::CodeUnitSizeOf<OutputIterator>::value == 2);
			if(c < 0x00010000ul) {
				if(check && text::surrogates::isSurrogate(c))
					throw text::InvalidScalarValueException(c);
				*(out++) = static_cast<Char>(c & 0xffffu);
				return 1;
			} else if(!check || c < 0x00110000ul) {
				*(out++) = text::surrogates::highSurrogate(c);
				*(out++) = text::surrogates::lowSurrogate(c);
				return 2;
			}
			throw text::InvalidCodePointException(c);
		} 
	}

	namespace text {
		namespace utf16 {

			/**
			 * Converts the first surrogate pair in the given character sequence to the
			 * corresponding code point.
			 * @tparam InputIterator The input iterator represents a UTF-16 character sequence
			 * @param first The beginning of the character sequence
			 * @param last The end of the character sequence
			 * @return The code point
			 */
			template<typename InputIterator>
			inline CodePoint decodeFirst(InputIterator first, InputIterator last) /*throw()*/ {
				ASCENSION_STATIC_ASSERT(CodeUnitSizeOf<InputIterator>::value == 2);
				assert(first != last);
				const Char high = *first;
				return (++first != last) ? surrogates::checkedDecode(high, *first) : high;
			}

			/**
			 * Converts the last surrogate pair in the given character sequence to the
			 * corresponding code point.
			 * @tparam BidirectionalIterator The bidirectional iterator represents a UTF-16
			 *                               character sequence
			 * @param first The beginning of the character sequence
			 * @param last The end of the character sequence
			 * @return The code point
			 */
			template<typename BidirectionalIterator>
			inline CodePoint decodeLast(
					BidirectionalIterator first, BidirectionalIterator last) /*throw()*/ {
				ASCENSION_STATIC_ASSERT(CodeUnitSizeOf<BidirectionalIterator>::value == 2);
				assert(first != last);
				const Char low = *--last;
				return (last != first && surrogates::isLowSurrogate(low)
					&& surrogates::isHighSurrogate(*--last)) ? surrogates::decode(*last, low) : low;
			}

			/**
			 * Converts the specified code point to a corresponding surrogate pair.
			 * @tparam OutputIterator The output iterator represents a UTF-16 character sequence
			 * @param c The code point to encode
			 * @param[out] out The output iterator
			 * @return The number of code unit written into @a out (either 1 or 2)
			 * @throw InvalidCodePoint @a c is invalid
			 * @throw InvalidScalarValue @a c is invalid
			 */
			template<typename OutputIterator>
			inline std::size_t checkedEncode(CodePoint c, OutputIterator& out) {
				return detail::encodeUTF16<true>(c, out);
			}

			/**
			 * Converts the specified code point to a corresponding surrogate pair.
			 * This function does not check if @a c is valid at all.
			 * @tparam OutputIterator The output iterator represents a UTF-16 character sequence
			 * @param c The code point to encode
			 * @param[out] out The output iterator
			 * @return The number of code unit written into @a out (either 1 or 2)
			 */
			template<typename OutputIterator>
			inline std::size_t uncheckedEncode(CodePoint c, OutputIterator& out) {
				return detail::encodeUTF16<false>(c, out);
			}

			/**
			 * Searches the next high-surrogate in the given character sequence.
			 * @tparam InputIterator The input iterator represents a UTF-16 character sequence
			 * @param start The start position to search
			 * @param last The end of the character sequence
			 * @return The next high-surrogate
			 */
			template<typename InputIterator>
			inline InputIterator next(InputIterator start, InputIterator last) /*throw()*/ {
				ASCENSION_STATIC_ASSERT(CodeUnitSizeOf<InputIterator>::value == 2);
				assert(start != last);
				return (surrogates::isHighSurrogate(*(start++))
					&& (start != last) && surrogates::isLowSurrogate(*start)) ? ++start : start;
			}

			/**
			 * Searches the previous high-surrogate in the given character sequence.
			 * @tparam BidirectionalIterator The bidirectional iterator represents a UTF-16
			 *                               character sequence
			 * @param first The beginning of the character sequence
			 * @param start The start position to search
			 * @return The previous high-surrogate
			 */
			template<typename BidirectionalIterator>
			inline BidirectionalIterator previous(
					BidirectionalIterator first, BidirectionalIterator start) /*throw()*/ {
				ASCENSION_STATIC_ASSERT(CodeUnitSizeOf<BidirectionalIterator>::value == 2);
				assert(first != start);
				return (!surrogates::isLowSurrogate(*--start)
					|| (start == first) || surrogates::isHighSurrogate(*--start)) ? start : ++start;
			}

		} // namespace utf16
	}
} // namespace ascension.text

#endif // !ASCENSION_UTF_16_HPP
