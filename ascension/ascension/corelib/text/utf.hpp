/**
 * @file utf.hpp
 * @author exeal
 * @date 2005-2010 was unicode.hpp
 * @date 2010 was character-iterator.hpp
 * @date 2010-11-06 separated from character-iterator.hpp to unicode-surrogates.hpp and
 *                  unicode-utf.hpp
 * @date 2011-08-19 split a part into utf8.hpp
 * @date 2011-08-20 joined unicode-surrogates.hpp and unicode-utf.hpp
 * @date 2011-08-27 joined utf8.hpp and utf16.hpp
 * @see utf-iterator.hpp, encoder.hpp
 */

#ifndef ASCENSION_UTF_HPP
#define ASCENSION_UTF_HPP

#include <ascension/corelib/text/character.hpp>	// CodePoint, ASCENSION_STATIC_ASSERT, surrogates.*
#include <cassert>								// assert

#if ASCENSION_UNICODE_VERSION > 0x0510
#error These class definitions and implementations are based on old version of Unicode.
#endif

namespace ascension {

	namespace detail {
		/*
			well-formed UTF-8 first byte distribution (based on Unicode 5.0 Table 3.7)
			value  1st-byte   code points       byte count
			----------------------------------------------
			10     00..7F     U+0000..007F      1
			21     C2..DF     U+0080..07FF      2
			32     E0         U+0800..0FFF      3
			33     E1..EC     U+1000..CFFF      3
			34     ED         U+D000..D7FF      3
			35     EE..EF     U+E000..FFFF      3
			46     F0         U+10000..3FFFF    4
			47     F1..F3     U+40000..FFFFF    4
			48     F4         U+100000..10FFFF  4
			09     otherwise  ill-formed        (0)
		 */
		const uint8_t UTF8_WELL_FORMED_FIRST_BYTES[] = {
			0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,	// 0x80
			0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,	// 0x90
			0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,	// 0xA0
			0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,	// 0xB0
			0x09, 0x09, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21,	// 0xC0
			0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21,	// 0xD0
			0x32, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x34, 0x35, 0x35,	// 0xE0
			0x46, 0x47, 0x47, 0x47, 0x48, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09	// 0xF0
		};

		template<bool check, typename OutputIterator>
		inline std::size_t encodeUTF8(CodePoint c, OutputIterator& out) {
			ASCENSION_STATIC_ASSERT(CodeUnitSizeOf<OutputIterator>::value == 1);
			if(c < 0x0080u) {	// 00000000 0xxxxxxx -> 0xxxxxxx
				*(out++) = static_cast<uint8_t>(c);
				return 1;
			} else if(c < 0x0800u) {	// 00000yyy yyxxxxxx -> 110yyyyy 10xxxxxx
				*(out++) = static_cast<uint8_t>((c >> 6) | 0xc0);
				*(out++) = static_cast<uint8_t>((c & 0x3f) | 0x80);
				return 2;
			} else if(c < 0x10000u) {	// zzzzyyyy yyxxxxxx -> 1110zzzz 10yyyyyy 10xxxxxx
				if(check && text::surrogates::isSurrogate(c))
					throw text::InvalidScalarValueException(c);
				*(out++) = static_cast<uint8_t>((c >> 12) | 0xe0);
				*(out++) = static_cast<uint8_t>(((c >> 6) & 0x3f) | 0x80);
				*(out++) = static_cast<uint8_t>((c & 0x3f) | 0x80);
				return 3;
			} else if(c < 0x110000u) {	// 000uuuuu zzzzyyyy yyxxxxxx <- 11110uuu 10uuzzzz 10yyyyyy 10xxxxxx
				*(out++) = static_cast<uint8_t>((c >> 18) | 0xf0);
				*(out++) = static_cast<uint8_t>(((c >> 12) & 0x3f) | 0x80);
				*(out++) = static_cast<uint8_t>(((c >> 6) & 0x3f) | 0x80);
				*(out++) = static_cast<uint8_t>((c & 0x3f) | 0x80);
				return 4;
			}
			throw text::InvalidCodePointException(c);
		}

		template<bool check, typename OutputIterator>
		inline std::size_t encodeUTF16(CodePoint c, OutputIterator& out) {
			ASCENSION_STATIC_ASSERT(text::CodeUnitSizeOf<OutputIterator>::value == 2);
			if(c < 0x00010000ul) {
				if(check && text::surrogates::isSurrogate(c))
					throw text::InvalidScalarValueException(c);
				*(out++) = static_cast<uint16_t>(c & 0xffffu);
				return 1;
			} else if(!check || c < 0x00110000ul) {
				*(out++) = text::surrogates::highSurrogate(c);
				*(out++) = text::surrogates::lowSurrogate(c);
				return 2;
			}
			throw text::InvalidScalarValueException(c);
		} 
	}

	namespace text {
		namespace utf {

			// UTF-8 trivials /////////////////////////////////////////////////////////////////////

			/**
			 * Returns @c true if the given code unit is UTF-8 valid byte (which can be any
			 * component of valid UTF-8 byte sequence).
			 * @param byte The code unit to test
			 * @return true if @a byte is valid byte
			 */
			inline bool isValidByte(uint8_t byte) {
				return byte < 0xc0 || (byte > 0xc1 && byte < 0xf5);
			}

			/**
			 * Returns @c true if the given code unit is UTF-8 single byte (which encodes a code
			 * point by itself).
			 * @param byte The code unit to test
			 * @return true if @a byte is single byte
			 */
			inline bool isSingleByte(uint8_t byte) {
				return (byte & 0x80) == 0;
			}

			/**
			 * Returns @c true if the given code unit is UTF-8 leading byte.
			 * @param byte The code unit to test
			 * @return true if @a byte is leading byte
			 */
			inline bool isLeadingByte(uint8_t byte) /*throw()*/ {
				return byte < 0x80 || ((detail::UTF8_WELL_FORMED_FIRST_BYTES[byte - 0x80] & 0xf0) != 0);
			}

			/**
			 * Returns @c true if the given code unit is UTF-8 trailing byte.
			 * @param byte The code unit to test
			 * @return true if @a byte is trailing byte
			 */
			inline bool isTrailingByte(uint8_t byte) {
				return (byte & 0xc0) == 0x80;
			}

			inline std::size_t length(uint8_t leadingByte) {
				if(isSingleByte(leadingByte))
					return 1;
				assert(leadingByte >= 0x80);
				return detail::UTF8_WELL_FORMED_FIRST_BYTES[leadingByte - 0x80] >> 4;
			}

			inline std::size_t numberOfTrailingBytes(uint8_t leadingByte) {
				return length(leadingByte) - 1;
			}

			// UTF-8 //////////////////////////////////////////////////////////////////////////////

			template<typename InputIterator>
			inline CodePoint decodeUnsafe(InputIterator i) {
				ASCENSION_STATIC_ASSERT(CodeUnitSizeOf<InputIterator>::value == 1);
				const uint8_t leadingByte = *i;
				switch(length(leadingByte)) {
				case 1:	// 00000000 0xxxxxxx <- 0xxxxxxx
					return leadingByte;
				case 2:	// 00000yyy yyxxxxxx <- 110yyyyy 10xxxxxx
					return ((leadingByte & 0x1f) << 6) | (*++i & 0x3f);
				case 3:	// zzzzyyyy yyxxxxxx <- 1110zzzz 10yyyyyy 10xxxxxx
					return ((leadingByte & 0x0f) << 12) | ((*++i & 0x3f) << 6) | (*++i & 0x3f);
				case 4:	// 000uuuuu zzzzyyyy yyxxxxxx <- 11110uuu 10uuzzzz 10yyyyyy 10xxxxxx
					return ((leadingByte & 0x07) << 18) | ((*++i & 0x3f) << 12) | ((*++i & 0x3f) << 6) | (*++i & 0x3f);
				case 0:
					return REPLACEMENT_CHARACTER;
				default:
					ASCENSION_ASSERT_NOT_REACHED();
				}
			}

			/**
			 * Writes a character into the specified output iterator as UTF-8 character sequence
			 * and advances the iterator to end of written bytes.
			 * @tparam OutputIterator The type for @a out
			 * @param c The code point of the character
			 * @param[out] out The output iterator
			 * @return The number of bytes written to @a out (1..4)
			 * @throw InvalidCodePointException @a c is invalid
			 */
			template<typename OutputIterator>
			inline std::size_t encode(CodePoint c, OutputIterator& out,
					typename std::enable_if<CodeUnitSizeOf<OutputIterator>::value == 1>::type* = 0) {
				return detail::encodeUTF8<false>(c, out);
			}

			/**
			 * Writes a character into the specified output iterator as UTF-8 character sequence
			 * and advances the iterator to end of written bytes.
			 * @tparam OutputIterator The type for @a out
			 * @param c The code point of the character
			 * @param[out] out The output iterator
			 * @return The number of bytes written to @a out (1..4)
			 * @throw InvalidCodePointException @a c is invalid
			 * @throw InvalidScalarValueException @a c is invalid
			 */
			template<typename OutputIterator>
			inline std::size_t checkedEncode(CodePoint c, OutputIterator& out,
					typename std::enable_if<CodeUnitSizeOf<OutputIterator>::value == 1>::type* = 0) {
				return detail::encodeUTF8<true>(c, out);
			}

			template<typename InputIterator>
			inline InputIterator nextUnsafe(InputIterator i,
					typename std::enable_if<CodeUnitSizeOf<InputIterator>::value == 1>::type* = 0) {
				const std::size_t n = length(*i);
				std::advance(i, (n != 0) ? n : 1);
				return i;
			}

			// UTF-16 /////////////////////////////////////////////////////////////////////////////

			/**
			 * Converts the first character in the given UTF-16 code unit sequence to the
			 * corresponding code point.
			 * @tparam InputIterator The input iterator represents a UTF-16 code unit sequence
			 * @param first The beginning of the code unit sequence
			 * @param last The end of the code unit sequence
			 * @return The code point
			 */
			template<typename InputIterator>
			inline CodePoint decodeFirst(InputIterator first, InputIterator last,
					typename std::enable_if<CodeUnitSizeOf<InputIterator>::value == 2>::type* = 0) {
				assert(first != last);
				const uint16_t high = *first;
				if(surrogates::isHighSurrogate(high))
					return (++first != last) ? surrogates::decode(high, *first) : high;
				return high;
			}

			/**
			 * Converts the first character in the given UTF-16 code unit sequence to the
			 * corresponding code point.
			 * @tparam InputIterator The input iterator represents a UTF-16 code unit sequence
			 * @param first The beginning of the code unit sequence
			 * @param last The end of the code unit sequence
			 * @return The code point
			 * @throw MalformedInputException&lt;uint16_t&gt; The input code unit sequence is
			 *                                                ill-formed UTF-16
			 */
			template<typename InputIterator>
			inline CodePoint checkedDecodeFirst(InputIterator first, InputIterator last,
					typename std::enable_if<CodeUnitSizeOf<InputIterator>::value == 2>::type* = 0) {
				assert(first != last);
				const uint16_t high = *first;
				if(surrogates::isHighSurrogate(high)) {
					if(++first == last)
						throw MalformedInputException<uint16_t>(high);
					const uint16_t low = *first;
					if(!surrogates::isLowSurrogate(low))
						throw MalformedInputException<uint16_t>(low);
					return surrogates::decode(high, low);
				} else if(surrogates::isLowSurrogate(high))
					throw MalformedInputException<uint16_t>(high);
				return high;
			}

			/**
			 * Converts the last character in the given UTF-16 code unit sequence to the
			 * corresponding code point.
			 * @tparam BidirectionalIterator The bidirectional iterator represents a UTF-16
			 *                               code unit sequence
			 * @param first The beginning of the code unit sequence
			 * @param last The end of the code unit sequence
			 * @return The code point
			 */
			template<typename BidirectionalIterator>
			inline CodePoint decodeLast(
					BidirectionalIterator first, BidirectionalIterator last,
					typename std::enable_if<CodeUnitSizeOf<BidirectionalIterator>::value == 2>::type* = 0) {
				assert(first != last);
				const uint16_t low = *--last;
				if(surrogates::isLowSurrogate(low))
					return (--last != first) ? surrogates::decode(*last, low) : low;
				return low;
			}

			/**
			 * Converts the last character in the given UTF-16 code unit sequence to the
			 * corresponding code point.
			 * @tparam BidirectionalIterator The bidirectional iterator represents a UTF-16
			 *                               code unit sequence
			 * @param first The beginning of the code unit sequence
			 * @param last The end of the code unit sequence
			 * @return The code point
			 */
			template<typename BidirectionalIterator>
			inline CodePoint checkedDecodeLast(
					BidirectionalIterator first, BidirectionalIterator last,
					typename std::enable_if<CodeUnitSizeOf<BidirectionalIterator>::value == 2>::type* = 0) {
				assert(first != last);
				const uint16_t low = *--last;
				if(surrogates::isLowSurrogate(low)) {
					if(last == first)
						throw MalformedInputException<uint16_t>(low);
					const uint16_t high = *--last;
					if(!surrogates::isHighSurrogate(high))
						throw MalformedInputException<uint16_t>(high);
					return surrogates::decode(high, low);
				} else if(surrogates::isHighSurrogate(low))
					throw MalformedInputException<uint16_t>(low);
				return low;
			}

			/**
			 * Writes a character into the given output iterator as UTF-16 code unit sequence and
			 * advances the iterator to end of written bytes.
			 * This function does not check if @a c is valid at all.
			 * @tparam OutputIterator The output iterator represents a UTF-16 code unit sequence
			 * @param c The code point to encode
			 * @param[out] out The output iterator
			 * @return The number of code unit written into @a out (either 1 or 2)
			 */
			template<typename OutputIterator>
			inline std::size_t encode(CodePoint c, OutputIterator& out,
					typename std::enable_if<CodeUnitSizeOf<OutputIterator>::value == 2>::type* = 0) {
				return detail::encodeUTF16<false>(c, out);
			}

			/**
			 * Writes a character into the given output iterator as UTF-16 code unit sequence and
			 * advances the iterator to end of written bytes.
			 * @tparam OutputIterator The output iterator represents a UTF-16 code unit sequence
			 * @param c The code point to encode
			 * @param[out] out The output iterator
			 * @return The number of code unit written into @a out (either 1 or 2)
			 * @throw InvalidScalarValueException @a c is not valid scalar value can be expressed
			 *                                    in UTF-16 code unit sequence
			 */
			template<typename OutputIterator>
			inline std::size_t checkedEncode(CodePoint c, OutputIterator& out,
					typename std::enable_if<CodeUnitSizeOf<OutputIterator>::value == 2>::type* = 0) {
				return detail::encodeUTF16<true>(c, out);
			}

			/**
			 * Searches the next high-surrogate in the given character sequence.
			 * @tparam InputIterator The input iterator represents a UTF-16 character sequence
			 * @param start The start position to search
			 * @param last The end of the character sequence
			 * @return The next high-surrogate
			 */
			template<typename InputIterator>
			inline InputIterator next(InputIterator start, InputIterator last,
					typename std::enable_if<CodeUnitSizeOf<InputIterator>::value == 2>::type* = 0) {
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
					BidirectionalIterator first, BidirectionalIterator start,
					typename std::enable_if<CodeUnitSizeOf<BidirectionalIterator>::value == 2>::type* = 0) {
				assert(first != start);
				return (!surrogates::isLowSurrogate(*--start)
					|| (start == first) || surrogates::isHighSurrogate(*--start)) ? start : ++start;
			}

			// UTF-32 /////////////////////////////////////////////////////////////////////////////

			/**
			 * Converts the first character in the given UTF-32 code unit sequence to the
			 * corresponding code point.
			 * @tparam InputIterator The input iterator represents a UTF-32 code unit sequence
			 * @param first The beginning of the code unit sequence
			 * @param last The end of the code unit sequence
			 * @return The code point
			 */
			template<typename InputIterator>
			inline CodePoint decodeFirst(InputIterator first, InputIterator last,
					typename std::enable_if<CodeUnitSizeOf<InputIterator>::value == 4>::type* = 0) {
				assert(first != last);
				return static_cast<CodePoint>(*first);
			}

			/**
			 * Converts the first character in the given UTF-32 code unit sequence to the
			 * corresponding code point.
			 * @tparam InputIterator The input iterator represents a UTF-32 code unit sequence
			 * @param first The beginning of the code unit sequence
			 * @param last The end of the code unit sequence
			 * @return The code point
			 * @throw MalformedInputException&lt;uint16_t&gt; The input code unit sequence is
			 *                                                ill-formed UTF-32
			 */
			template<typename InputIterator>
			inline CodePoint checkedDecodeFirst(InputIterator first, InputIterator last,
					typename std::enable_if<CodeUnitSizeOf<InputIterator>::value == 4>::type* = 0) {
				assert(first != last);
				if(!isScalarValueException(*first))
					throw MalformedInputException<uint32_t>(*first);
				return decodeFirst32(first, last);
			}

			/**
			 * Converts the last character in the given UTF-32 code unit sequence to the
			 * corresponding code point.
			 * @tparam BidirectionalIterator The bidirectional iterator represents a UTF-32
			 *                               code unit sequence
			 * @param first The beginning of the code unit sequence
			 * @param last The end of the code unit sequence
			 * @return The code point
			 */
			template<typename BidirectionalIterator>
			inline CodePoint decodeLast(BidirectionalIterator first, BidirectionalIterator last,
					typename std::enable_if<CodeUnitSizeOf<BidirectionalIterator>::value == 4>::type* = 0) {
				assert(first != last);
				return static_cast<CodePoint>(*--last);
			}

			/**
			 * Converts the last character in the given UTF-32 code unit sequence to the
			 * corresponding code point.
			 * @tparam BidirectionalIterator The bidirectional iterator represents a UTF-32
			 *                               code unit sequence
			 * @param first The beginning of the code unit sequence
			 * @param last The end of the code unit sequence
			 * @return The code point
			 */
			template<typename BidirectionalIterator>
			inline CodePoint checkedDecodeLast(BidirectionalIterator first, BidirectionalIterator last,
					typename std::enable_if<CodeUnitSizeOf<BidirectionalIterator>::value == 4>::type* = 0) {
				assert(first != last);
				const uint32_t c = *--last;
				if(!isScalarValue(c))
					throw MalformedInputException<uint32_t>(c);
				return static_cast<CodePoint>(c);
			}

			/**
			 * Writes a character into the given output iterator as UTF-32 code unit sequence and
			 * advances the iterator to end of written bytes.
			 * This function does not check if @c is valid at all.
			 * @tparam OutputIterator The output iterator represents a UTF-32 code unit sequence
			 * @param c The code point to encode
			 * @param[out] out The output iterator
			 * @return 1
			 */
			template<typename OutputIterator>
			inline std::size_t encode(CodePoint c, OutputIterator& out,
					typename std::enable_if<CodeUnitSizeOf<OutputIterator>::value == 4>::type* = 0) {
				*out = static_cast<uint32_t>(c);
				return ++out, 1;
			}

			/**
			 * Writes a character into the given output iterator as UTF-32 code unit sequence and
			 * advances the iterator to end of written bytes.
			 * @tparam OutputIterator The output iterator represents a UTF-32 code unit sequence
			 * @param c The code point to encode
			 * @param[out] out The output iterator
			 * @return 1
			 * @throw InvalidScalarValueException @a c is not valid scalar value can be expressed
			 *                                    in UTF-32 code unit sequence
			 */
			template<typename OutputIterator>
			inline std::size_t checkedEncode(CodePoint c, OutputIterator& out,
					typename std::enable_if<CodeUnitSizeOf<OutputIterator>::value == 4>::type* = 0) {
				if(!isScalarValue(c))
					throw InvalidScalarValueException(c);
				return encode(c, out);
			}

		}

	}
}

#endif // !ASCENSION_UTF_HPP
