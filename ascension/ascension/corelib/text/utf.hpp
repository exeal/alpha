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
 * @date 2014
 * @see utf-iterator.hpp, encoder.hpp
 */

#ifndef ASCENSION_UTF_HPP
#define ASCENSION_UTF_HPP

#include <ascension/corelib/basic-exceptions.hpp>
#include <ascension/corelib/text/character.hpp>	// CodePoint, ASCENSION_STATIC_ASSERT, surrogates.*
#include <cassert>								// assert

#if ASCENSION_UNICODE_VERSION > 0x0510
#error These class definitions and implementations are based on old version of Unicode.
#endif

namespace ascension {
	namespace text {
		namespace detail {
			/*
				UTF-8 code unit value distribution (based on Unicode 6.0 Table 3.7)
	
				Code unit  As leading byte:                 As trailing byte:  Value
				(hex)      y/n  code points         length  y/n
				--------------------------------------------------------------------
				00..7F     yes  U+0000..U+007F      1       no                 0x10
				80..BF     no                               maybe              0x01
				C0..C1     no                               no                 0x00
				C2..DF     yes  U+0080..U+07FF      2       no                 0x20
				E0         yes  U+0800..U+0FFF      3       no                 0x30
				E1..EC     yes  U+1000..U+CFFF      3       no                 0x30
				ED         yes  U+D000..U+D7FF      3       no                 0x30
				EE..EF     yes  U+E000..U+FFFF      3       no                 0x30
				F0         yes  U+10000..U+3FFFF    4       no                 0x40
		 		F1..F3     yes  U+40000..U+FFFFF    4       no                 0x40
				F4         yes  U+100000..U+10FFFF  4       no                 0x40
				F5..FF     no                               no                 0x00
			 */
			const std::uint8_t UTF8_CODE_UNIT_VALUES[] = {
				0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,	// 0x00
				0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,	// 0x10
				0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,	// 0x20
				0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,	// 0x30
				0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,	// 0x40
				0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,	// 0x50
				0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,	// 0x60
				0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,	// 0x70
				0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,	// 0x80
				0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,	// 0x90
				0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,	// 0xA0
				0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,	// 0xB0
				0x00, 0x00, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,	// 0xC0
				0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,	// 0xD0
				0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30,	// 0xE0
				0x40, 0x40, 0x40, 0x40, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00	// 0xF0
			};
	
			inline CodePoint decodeUTF8(const std::uint8_t bytes[], std::size_t nbytes, bool checkMalformedInput) {
				// this function never checks bytes[0] value
				switch(nbytes) {
				case 1:	// 00000000 0xxxxxxx <- 0xxxxxxx
					return bytes[0];
				case 2:	// 00000yyy yyxxxxxx <- 110yyyyy 10xxxxxx
					if(checkMalformedInput && (bytes[1] & 0xc0) != 0x80)	// <C2..DF 80..BF>
						throw MalformedInputException<const std::uint8_t*>(bytes + 1, 1);
					return ((bytes[0] & 0x1f) << 6) | (bytes[1] & 0x3f);
				case 3:	// zzzzyyyy yyxxxxxx <- 1110zzzz 10yyyyyy 10xxxxxx
					if(checkMalformedInput) {
						if((bytes[0] == 0xe0 && (bytes[1] & 0xe0) != 0xa0)	// <E0 A0..BF XX>
								|| (bytes[0] == 0xed && (bytes[1] & 0xe0) != 0x80)	// <ED 80..9F XX>
								|| ((bytes[1] & 0xc0) != 0x80))	// <XX 80..BF XX>
							throw MalformedInputException<const std::uint8_t*>(bytes + 1, 1);
						if((bytes[2] & 0xc0) != 0x80)	// <XX XX 80..BF>
							throw MalformedInputException<const std::uint8_t*>(bytes + 2, 2);
					}
					return ((bytes[0] & 0x0f) << 12) | ((bytes[1] & 0x3f) << 6) | (bytes[2] & 0x3f);
				case 4:	// 000uuuuu zzzzyyyy yyxxxxxx <- 11110uuu 10uuzzzz 10yyyyyy 10xxxxxx
					if(checkMalformedInput) {
						if((bytes[0] == 0xf0 && (bytes[1] < 0x90 || bytes[1] > 0xbf))	// <F0 90..BF XX XX>
								|| (bytes[0] == 0xf4 && (bytes[1] & 0xf0) != 0x80)	// <F4 80..8F XX XX>
								|| ((bytes[1] & 0xc0) != 0x80))	// <F1..F3 80..BF XX XX>
							throw MalformedInputException<const std::uint8_t*>(bytes + 1, 1);
						if((bytes[2] & 0xc0) != 0x80)	// <XX XX 80..BF XX>
							throw MalformedInputException<const std::uint8_t*>(bytes + 2, 2);
						if((bytes[3] & 0xc0) != 0x80)	// <XX XX XX 80..BF>
							throw MalformedInputException<const std::uint8_t*>(bytes + 3, 3);
					}
					return ((bytes[0] & 0x07) << 18) | ((bytes[1] & 0x3f) << 12) | ((bytes[2] & 0x3f) << 6) | (bytes[3] & 0x3f);
				case 0:	// bad leading byte
					throw MalformedInputException<const std::uint8_t*>(bytes, 1);
				default:
					ASCENSION_ASSERT_NOT_REACHED();
				}
			}
	
			template<typename InputIterator>
			inline CodePoint decodeUTF8(InputIterator first, InputIterator last, bool checkMalformedInput) {
				ASCENSION_STATIC_ASSERT(CodeUnitSizeOf<InputIterator>::value == 1);
				assert(first != last);
				InputIterator p(first);	// for throw
				std::uint8_t bytes[4] = {*first};
				std::size_t nbytes = utf::length(bytes[0]);
				for(std::size_t i = 1; i < nbytes; ++i) {
					if(++first == last) {
						nbytes = i;
						break;
					}
					bytes[i] = *first;
				}
				try {
					return decodeUTF8(bytes, nbytes, checkMalformedInput);
				} catch(const MalformedInputException<const std::uint8_t*>& e) {
					std::advance(p, e.position() - bytes);
					throw MalformedInputException<InputIterator>(p, e.maximalSubpartLength());
				}
			}
	
			template<bool check, typename OutputIterator>
			inline std::size_t encodeUTF8(CodePoint c, OutputIterator& out) {
				ASCENSION_STATIC_ASSERT(CodeUnitSizeOf<OutputIterator>::value == 1);
				if(c < 0x0080u) {	// 00000000 0xxxxxxx -> 0xxxxxxx
					*(out++) = static_cast<std::uint8_t>(c);
					return 1;
				} else if(c < 0x0800u) {	// 00000yyy yyxxxxxx -> 110yyyyy 10xxxxxx
					*(out++) = static_cast<std::uint8_t>((c >> 6) | 0xc0);
					*(out++) = static_cast<std::uint8_t>((c & 0x3f) | 0x80);
					return 2;
				} else if(c < 0x10000u) {	// zzzzyyyy yyxxxxxx -> 1110zzzz 10yyyyyy 10xxxxxx
					if(check && surrogates::isSurrogate(c))
						throw InvalidScalarValueException(c);
					*(out++) = static_cast<std::uint8_t>((c >> 12) | 0xe0);
					*(out++) = static_cast<std::uint8_t>(((c >> 6) & 0x3f) | 0x80);
					*(out++) = static_cast<std::uint8_t>((c & 0x3f) | 0x80);
					return 3;
				} else if(c < 0x110000u) {	// 000uuuuu zzzzyyyy yyxxxxxx <- 11110uuu 10uuzzzz 10yyyyyy 10xxxxxx
					*(out++) = static_cast<std::uint8_t>((c >> 18) | 0xf0);
					*(out++) = static_cast<std::uint8_t>(((c >> 12) & 0x3f) | 0x80);
					*(out++) = static_cast<std::uint8_t>(((c >> 6) & 0x3f) | 0x80);
					*(out++) = static_cast<std::uint8_t>((c & 0x3f) | 0x80);
					return 4;
				}
				throw InvalidCodePointException(c);
			}
	
			template<bool check, typename OutputIterator>
			inline std::size_t encodeUTF16(CodePoint c, OutputIterator& out) {
				ASCENSION_STATIC_ASSERT(CodeUnitSizeOf<OutputIterator>::value == 2);
				if(c < 0x00010000ul) {
					if(check && surrogates::isSurrogate(c))
						throw InvalidScalarValueException(c);
					*(out++) = static_cast<std::uint16_t>(c & 0xffffu);
					return 1;
				} else if(!check || c < 0x00110000ul) {
					*(out++) = surrogates::highSurrogate(c);
					*(out++) = surrogates::lowSurrogate(c);
					return 2;
				}
				throw InvalidScalarValueException(c);
			} 
		}	// namespace detail

		namespace utf {
			/// @defgroup utf_common_trivials UTF Common Trivial Functions
			/// @{
			/**
			 * Returns the number of bytes in 
			 * @tparam codeUnitSize The size of the code unit. Either 1, 2 or 4
			 * @param c The code point of character
			 * @return The number of bytes
			 * @throw InvalidScalarValueException @a c is invalid
			 */
			template<std::size_t codeUnitSize>
			std::size_t numberOfEncodedBytes(CodePoint c);

			template<> inline std::size_t numberOfEncodedBytes<1>(CodePoint c) {
				if(!isScalarValue(c))
					throw InvalidScalarValueException(c);
				else if(c < 0x0080u)
					return 1;
				else if(c < 0x0800u)
					return 2;
				else if(c < 0x10000u)
					return 3;
				else
					return 4;
			}
			template<> inline std::size_t numberOfEncodedBytes<2>(CodePoint c) {
				if(!isScalarValue(c))
					throw InvalidScalarValueException(c);
				else
					return (c < 0x10000u) ? 1 : 2;
			}
			template<> inline std::size_t numberOfEncodedBytes<4>(CodePoint c) {
				if(!isScalarValue(c))
					throw InvalidScalarValueException(c);
				else
					return 1;
			}
			/// @}

			/// @defgroup utf8_trivials UTF-8 Trivial Functions
			/// @{
			/**
			 * Returns @c true if the given code unit is UTF-8 valid byte (which can be any
			 * component of valid UTF-8 byte sequence).
			 * @param byte The code unit to test
			 * @return true if @a byte is valid byte
			 */
			inline bool isValidByte(std::uint8_t byte) {
//				return byte < 0xc0 || (byte > 0xc1 && byte < 0xf5);
				return detail::UTF8_CODE_UNIT_VALUES[byte] != 0x00;
			}

			/**
			 * Returns @c true if the given code unit is UTF-8 single byte (which encodes a code
			 * point by itself).
			 * @param byte The code unit to test
			 * @return true if @a byte is single byte
			 */
			inline bool isSingleByte(std::uint8_t byte) {
//				return (byte & 0x80) == 0;
				return detail::UTF8_CODE_UNIT_VALUES[byte] == 0x10;
			}

			/**
			 * Returns @c true if the given code unit is UTF-8 leading byte.
			 * @param byte The code unit to test
			 * @return true if @a byte is leading byte
			 */
			inline bool isLeadingByte(std::uint8_t byte) BOOST_NOEXCEPT {
				return (detail::UTF8_CODE_UNIT_VALUES[byte] & 0xf0) != 0;
			}

			/**
			 * Returns @c true if the given code unit may be UTF-8 trailing byte.
			 * @param byte The code unit to test
			 * @return true if @a byte may be trailing byte
			 */
			inline bool maybeTrailingByte(std::uint8_t byte) {
//				return (byte & 0xc0) == 0x80;
				return (detail::UTF8_CODE_UNIT_VALUES[byte] & 0x0f) == 0x01;
			}

			inline std::size_t length(std::uint8_t leadingByte) {
				return detail::UTF8_CODE_UNIT_VALUES[leadingByte] >> 4;
			}

			inline std::size_t numberOfTrailingBytes(std::uint8_t leadingByte) {
				return length(leadingByte) - 1;
			}
			/// @}

			/// @defgroup utf8_encode_decode UTF-8 Encoding and Decoding
			/// @{
			/**
			 * Converts the first character in the given UTF-8 code unit sequence to the
			 * corresponding code point.
			 * @tparam InputIterator The input iterator represents a UTF-8 code unit sequence
			 * @param first The beginning of the code unit sequence
			 * @param last The end of the code unit sequence
			 * @return The code point, or @c REPLACEMENT_CHARACTER if the input was ill-formed
			 */
			template<typename InputIterator>
			inline CodePoint decodeFirst(InputIterator first, InputIterator last,
					typename std::enable_if<CodeUnitSizeOf<InputIterator>::value == 1>::type* = nullptr) {
				try {
					return detail::decodeUTF8(first, last, false);
				} catch(const MalformedInputException<InputIterator>& e) {
					assert(e.maximalSubpartLength() == 1);
					return REPLACEMENT_CHARACTER;
				}
			}

			/**
			 * Converts the first character in the given UTF-8 code unit sequence to the
			 * corresponding code point.
			 * @tparam InputIterator The input iterator represents a UTF-8 code unit sequence
			 * @param first The beginning of the code unit sequence
			 * @param last The end of the code unit sequence
			 * @return The code point
			 * @throw MalformedInputException&lt;InputIterator&gt; The input was ill-formed
			 */
			template<typename InputIterator>
			inline CodePoint checkedDecodeFirst(InputIterator first, InputIterator last,
					typename std::enable_if<CodeUnitSizeOf<InputIterator>::value == 1>::type* = nullptr) {
				return detail::decodeUTF8(first, last, true);
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
					typename std::enable_if<CodeUnitSizeOf<OutputIterator>::value == 1>::type* = nullptr) {
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
					typename std::enable_if<CodeUnitSizeOf<OutputIterator>::value == 1>::type* = nullptr) {
				return detail::encodeUTF8<true>(c, out);
			}
			/// @}

			/// @defgroup utf16_encode_decode UTF-16 Encoding and Decoding
			/// @{
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
					typename std::enable_if<CodeUnitSizeOf<InputIterator>::value == 2>::type* = nullptr) {
				assert(first != last);
				const std::uint16_t high = *first;
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
			 * @throw MalformedInputException&lt;InputIterator&gt; The input code unit sequence is
			 *                                                     ill-formed UTF-16
			 */
			template<typename InputIterator>
			inline CodePoint checkedDecodeFirst(InputIterator first, InputIterator last,
					typename std::enable_if<CodeUnitSizeOf<InputIterator>::value == 2>::type* = nullptr) {
				assert(first != last);
				const std::uint16_t high = *first;
				if(surrogates::isHighSurrogate(high)) {
					if(++first == last)
						throw MalformedInputException<InputIterator>(first, 1);
					const std::uint16_t low = *first;
					if(!surrogates::isLowSurrogate(low))
						throw MalformedInputException<InputIterator>(first, 1);
					return surrogates::decode(high, low);
				} else if(surrogates::isLowSurrogate(high))
					throw MalformedInputException<InputIterator>(first, 1);
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
					typename std::enable_if<CodeUnitSizeOf<BidirectionalIterator>::value == 2>::type* = nullptr) {
				assert(first != last);
				const std::uint16_t low = *--last;
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
			 * @throw MalformedInputException&lt;BidirectionalIterator&gt; The input code unit
			 *                                                             quence is ill-formed
			 *                                                             UTF-16
			 */
			template<typename BidirectionalIterator>
			inline CodePoint checkedDecodeLast(BidirectionalIterator first, BidirectionalIterator last,
					typename std::enable_if<CodeUnitSizeOf<BidirectionalIterator>::value == 2>::type* = nullptr) {
				assert(first != last);
				const std::uint16_t low = *--last;
				if(surrogates::isLowSurrogate(low)) {
					if(last == first)
						throw MalformedInputException<BidirectionalIterator>(last, 1);
					const std::uint16_t high = *--last;
					if(!surrogates::isHighSurrogate(high))
						throw MalformedInputException<BidirectionalIterator>(last, 1);
					return surrogates::decode(high, low);
				} else if(surrogates::isHighSurrogate(low))
					throw MalformedInputException<BidirectionalIterator>(last, 1);
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
					typename std::enable_if<CodeUnitSizeOf<OutputIterator>::value == 2>::type* = nullptr) {
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
					typename std::enable_if<CodeUnitSizeOf<OutputIterator>::value == 2>::type* = nullptr) {
				return detail::encodeUTF16<true>(c, out);
			}
			/// @}

			/// @defgroup utf32_encode_decode UTF-32 Encoding and Decoding
			/// @{
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
					typename std::enable_if<CodeUnitSizeOf<InputIterator>::value == 4>::type* = nullptr) {
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
			 * @throw MalformedInputException&lt;InputIterator&gt; The input code unit sequence is
			 *                                                     ill-formed UTF-32
			 */
			template<typename InputIterator>
			inline CodePoint checkedDecodeFirst(InputIterator first, InputIterator last,
					typename std::enable_if<CodeUnitSizeOf<InputIterator>::value == 4>::type* = nullptr) {
				assert(first != last);
				if(!isScalarValueException(*first))
					throw MalformedInputException<InputIterator>(first, 1);
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
					typename std::enable_if<CodeUnitSizeOf<BidirectionalIterator>::value == 4>::type* = nullptr) {
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
			 * @throw MalformedInputException&lt;BidirectionalIterator&gt; The input code unit
			 *                                                             sequence is ill-formed
			 *                                                             UTF-32
			 * @return The code point
			 */
			template<typename BidirectionalIterator>
			inline CodePoint checkedDecodeLast(BidirectionalIterator first, BidirectionalIterator last,
					typename std::enable_if<CodeUnitSizeOf<BidirectionalIterator>::value == 4>::type* = nullptr) {
				assert(first != last);
				const std::uint32_t c = *--last;
				if(!isScalarValue(c))
					throw MalformedInputException<BidirectionalIterator>(last, 1);
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
					typename std::enable_if<CodeUnitSizeOf<OutputIterator>::value == 4>::type* = nullptr) {
				*out = static_cast<std::uint32_t>(c);
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
					typename std::enable_if<CodeUnitSizeOf<OutputIterator>::value == 4>::type* = nullptr) {
				if(!isScalarValue(c))
					throw InvalidScalarValueException(c);
				return encode(c, out);
			}
			/// @}
		}
	}
}

#endif // !ASCENSION_UTF_HPP
