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
#include <ascension/corelib/text/code-point.hpp>
#include <cassert>

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
			inline CodePoint decodeUTF8(InputIterator first, InputIterator last, bool checkMalformedInput,
					typename std::enable_if<CodeUnitSizeOf<InputIterator>::value == 1>::type* = nullptr) {
				assert(first != last);
				InputIterator p(first);	// for throw
				std::uint8_t bytes[4] = {static_cast<std::uint8_t>(*first)};
				auto nbytes(utf::CodeUnitTraits<1>::lengthForLeading(bytes[0]));
				if(nbytes == boost::none)
					throw MalformedInputException<InputIterator>(p, 1);
				for(std::size_t i = 1; i < boost::get(nbytes); ++i) {
					if(++first == last) {
						nbytes = i;
						break;
					}
					bytes[i] = *first;
				}
				try {
					return decodeUTF8(bytes, boost::get(nbytes), checkMalformedInput);
				} catch(const MalformedInputException<const std::uint8_t*>& e) {
					std::advance(p, e.position() - bytes);
					throw MalformedInputException<InputIterator>(p, e.maximalSubpartLength());
				}
			}
		}	// namespace detail

		namespace utf {
			/**
			 * Base type of @c CodeUnitTraitsBase class template.
			 * @tparam codeUnitSize The number of bytes of a code unit
			 */
			template<std::size_t codeUnitSize>
			struct CodeUnitType {
				typedef char value_type;	///< Represents a code unit.
			};
			template<> struct CodeUnitType<1> {typedef std::uint8_t value_type;};
			template<> struct CodeUnitType<2> {typedef Char value_type;};
			template<> struct CodeUnitType<4> {typedef CodePoint value_type;};

			/**
			 * Base type of @c CodeUnitTraits class template.
			 * @tparam codeUnitSize The number of bytes of a code unit
			 * @tparam Derived
			 */
			template<std::size_t codeUnitSize, typename Derived>
			struct CodeUnitTraitsBase : CodeUnitType<codeUnitSize> {
				/**
				 * @copybrief CodeUnitTraits#checkedDecodeFirst
				 * @tparam BidirectionalReadableRange The range represents a UTF code unit sequence
				 * @param range The code unit sequence
				 * @return The code point
				 * @throw MalformedInputException&lt;InputIterator&gt; The input is ill-formed
				 */
				template<typename BidirectionalReadableRange>
				static CodePoint checkedDecodeFirst(const BidirectionalReadableRange& range) {
					return Derived::checkedDecodeFirst(boost::const_begin(range), boost::const_end(range));
				}
				/**
				 * @copybrief CodeUnitTraits#checkedDecodeLast
				 * @tparam BidirectionalReadableRange The range represents a UTF code unit sequence
				 * @param range The code unit sequence
				 * @return The code point
				 * @throw MalformedInputException&lt;BidirectionalIterator&gt; The input is ill-formed
				 */
				template<typename BidirectionalReadableRange>
				static CodePoint checkedDecodeLast(const BidirectionalReadableRange& range) {
					return Derived::checkedDecodeLast(boost::const_begin(range), boost::const_end(range));
				}
				/**
				 * @copybrief CodeUnitTraits#decodeFirst
				 * @tparam BidirectionalReadableRange The range represents a UTF code unit sequence
				 * @param range The code unit sequence
				 * @return The code point, or @c REPLACEMENT_CHARACTER if the input is ill-formed (only UTF-8)
				 */
				template<typename BidirectionalReadableRange>
				static CodePoint decodeFirst(const BidirectionalReadableRange& range) {
					return Derived::decodeFirst(boost::const_begin(range), boost::const_end(range));
				}
				/**
				 * @copybrief CodeUnitTraits#decodeLast
				 * @tparam BidirectionalReadableRange The range represents a UTF code unit sequence
				 * @param range The code unit sequence
				 * @return The code point
				 */
				template<typename BidirectionalReadableRange>
				static CodePoint decodeLast(const BidirectionalReadableRange& range) {
					return Derived::decodeLast(boost::const_begin(range), boost::const_end(range));
				}
				/**
				 * Returns the number of code units in a character which is trailed by the given code unit value.
				 * @param c The code unit
				 * @return The number of code units, or @c boost#none if @a codeUnit is invalid
				 * @see CodeUnitTraits#lengthForLeading
				 */
				static boost::optional<std::size_t> trailingLengthForLeading(value_type codeUnit) {
					const auto temp(Derived::lengthForLeading(codeUnit));
					if(temp == boost::none)
						return boost::none;
					return boost::get(temp) - 1;
				}
			};

			/**
			 * @tparam codeUnitSize The number of bytes of a code unit
			 */
			template<std::size_t codeUnitSize>
			struct CodeUnitTraits : CodeUnitTraitsBase<codeUnitSize, CodeUnitTraits<codeUnitSize>> {
				/// @name Code Unit Types
				/// @{
				/**
				 * Returns @c true if the given code unit is leading, which can be the start of a code unit.
				 * @param codeUnit The code unit to test
				 * @return true if @a codeUnit is leading
				 */
				static BOOST_CONSTEXPR bool isLeading(value_type codeUnit) BOOST_NOEXCEPT;
				/**
				 * Returns @c true if the given code unit is single, which encodes a code point by itself.
				 * @param codeUnit The code unit to test
				 * @return true if @a codeUnit is single
				 */
				static BOOST_CONSTEXPR bool isSingle(value_type codeUnit) BOOST_NOEXCEPT;
				/**
				 * Returns @c true if the given code unit is valid, which can be any component of valid code unit
				 * sequence.
				 * @param codeUnit The code unit to test
				 * @return true if @a codeUnit is valid
				 */
				static BOOST_CONSTEXPR bool isValid(value_type codeUnit) BOOST_NOEXCEPT;
				/**
				 * Returns @c true if the given code unit may be trailing, which can follow the leading code unit.
				 * @param codeUnit The code unit to test
				 * @return true if @a codeUnit may be trailing
				 */
				static BOOST_CONSTEXPR bool maybeTrailing(value_type codeUnit) BOOST_NOEXCEPT;
				/// @}

				/// @name Code Unit Length
				/// @{
				/**
				 * Returns the number of code units in a character.
				 * @param c The code point of character
				 * @return The number of code units
				 * @throw InvalidScalarValueException @a c is invalid as a scalar value
				 */
				static boost::optional<std::size_t> length(CodePoint c);
				/**
				* Returns the number of code units in a character which is started by the given code unit value.
				* @param c The code unit
				* @return The number of code units, or @c boost#none if @a codeUnit is invalid
				*/
				static boost::optional<std::size_t> lengthForLeading(value_type codeUnit) BOOST_NOEXCEPT;;
				/// @}

				/// @name UTF-x Decoding Methods
				/// @{
				/**
				 * Converts the first character in the given UTF code unit sequence to the corresponding code point.
				 * @tparam InputIterator The input iterator represents a UTF code unit sequence
				 * @param first The beginning of the code unit sequence
				 * @param last The end of the code unit sequence
				 * @return The code point
				 * @throw MalformedInputException&lt;InputIterator&gt; The input is ill-formed
				 */
				template<typename InputIterator> static CodePoint checkedDecodeFirst(InputIterator& first, InputIterator last);
				/**
				 * Converts the last character in the given UTF code unit sequence to the corresponding code point.
				 * @tparam BidirectionalIterator The bidirectional iterator represents a UTF code unit sequence
				 * @param first The beginning of the code unit sequence
				 * @param last The end of the code unit sequence
				 * @return The code point
				 * @throw MalformedInputException&lt;BidirectionalIterator&gt; The input is ill-formed
				 */
				template<typename BidirectionalIterator> static CodePoint checkedDecodeLast(BidirectionalIterator& first, BidirectionalIterator last);
				/**
				 * Converts the first character in the given UTF code unit sequence to the corresponding code point.
				 * @tparam InputIterator The input iterator represents a UTF code unit sequence
				 * @param first The beginning of the code unit sequence
				 * @param last The end of the code unit sequence
				 * @return The code point, or @c REPLACEMENT_CHARACTER if the input is ill-formed (only UTF-8)
				 */
				template<typename InputIterator> static CodePoint decodeFirst(InputIterator& first, InputIterator last);
				/**
				 * Converts the last character in the given UTF code unit sequence to the corresponding code point.
				 * @tparam BidirectionalIterator The bidirectional iterator represents a UTF code unit sequence
				 * @param first The beginning of the code unit sequence
				 * @param last The end of the code unit sequence
				 * @return The code point
				 */
				template<typename BidirectionalIterator> static CodePoint decodeLast(BidirectionalIterator& first, BidirectionalIterator last);
				/// @}

				/// @name UTF-x Encoding Methods
				/// @{
				/**
				 * Writes a character into the specified output iterator as UTF character sequence.
				 * @tparam OutputIterator The type of @a out, which represents a UTF code unit sequence
				 * @param c The code point of the character
				 * @param out The output iterator
				 * @return The advanced output iterator which addresses the end of the written code units
				 * @throw InvalidCodePointException @a c is invalid (only UTF-8)
				 * @throw InvalidScalarValueException @a c is not valid scalar value can be expressed in this code unit
				 */
				template<typename OutputIterator> static OutputIterator checkedEncode(CodePoint c, OutputIterator out);
				/**
				 * Writes a character into the specified output iterator as UTF character sequence.
				 * @tparam OutputIterator The type of @a out, which represents a UTF code unit sequence
				 * @param c The code point of the character
				 * @param out The output iterator
				 * @return The advanced output iterator which addresses the end of the written code units
				 * @throw InvalidCodePointException @a c is invalid (only UTF-8)
				 * @note UTF-16/32: This function does not check if @a c is valid at all.
				 */
				template<typename OutputIterator> static OutputIterator encode(CodePoint c, OutputIterator out);
				/// @}
			};

			template<> struct CodeUnitTraits<1> : CodeUnitTraitsBase<1, CodeUnitTraits<1>> {
				template<typename InputIterator>
				static CodePoint checkedDecodeFirst(InputIterator first, InputIterator last) {
					return detail::decodeUTF8(first, last, true);
				}

				template<typename OutputIterator>
				static OutputIterator checkedEncode(CodePoint c, OutputIterator out) {
					return encode<true>(c, out);
				}

				template<typename InputIterator>
				static CodePoint decodeFirst(InputIterator& first, InputIterator last) {
					try {
						return detail::decodeUTF8(first, last, false);
					} catch(const MalformedInputException<InputIterator>& e) {
						assert(e.maximalSubpartLength() == 1);
						return REPLACEMENT_CHARACTER;
					}
				}

				template<typename OutputIterator>
				static OutputIterator encode(CodePoint c, OutputIterator out) {
					return encode<false>(c, out);
				}

				static BOOST_CONSTEXPR bool isLeading(value_type codeUnit) BOOST_NOEXCEPT {
					return (detail::UTF8_CODE_UNIT_VALUES[codeUnit] & 0xf0) != 0;
				}

				static BOOST_CONSTEXPR bool isSingle(value_type codeUnit) BOOST_NOEXCEPT {
//					return (codeUnit & 0x80) == 0;
					return detail::UTF8_CODE_UNIT_VALUES[codeUnit] == 0x10;
				}

				static BOOST_CONSTEXPR bool isValid(value_type codeUnit) BOOST_NOEXCEPT {
//					return codeUnit < 0xc0 || (codeUnit > 0xc1 && codeUnit < 0xf5);
					return detail::UTF8_CODE_UNIT_VALUES[codeUnit] != 0x00;
				}

				static std::size_t length(CodePoint c) {
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

				static boost::optional<std::size_t> lengthForLeading(value_type leading) BOOST_NOEXCEPT {
					const auto temp = detail::UTF8_CODE_UNIT_VALUES[leading] >> 4;
					return (temp != 0) ? boost::make_optional<std::size_t>(temp) : boost::none;
				}

				static BOOST_CONSTEXPR bool maybeTrailing(value_type codeUnit) BOOST_NOEXCEPT {
//					return (codeUnit & 0xc0) == 0x80;
					return (detail::UTF8_CODE_UNIT_VALUES[codeUnit] & 0x0f) == 0x01;
				}

			private:
				template<bool check, typename OutputIterator>
				static OutputIterator encode(CodePoint c, OutputIterator out) {
					if(c < 0x0080u)	// 00000000 0xxxxxxx -> 0xxxxxxx
						*(out++) = static_cast<std::uint8_t>(c);
					else if(c < 0x0800u) {	// 00000yyy yyxxxxxx -> 110yyyyy 10xxxxxx
						*(out++) = static_cast<std::uint8_t>((c >> 6) | 0xc0);
						*(out++) = static_cast<std::uint8_t>((c & 0x3f) | 0x80);
					} else if(c < 0x10000u) {	// zzzzyyyy yyxxxxxx -> 1110zzzz 10yyyyyy 10xxxxxx
						if(check && surrogates::isSurrogate(c))
							throw InvalidScalarValueException(c);
						*(out++) = static_cast<std::uint8_t>((c >> 12) | 0xe0);
						*(out++) = static_cast<std::uint8_t>(((c >> 6) & 0x3f) | 0x80);
						*(out++) = static_cast<std::uint8_t>((c & 0x3f) | 0x80);
					} else if(c < 0x110000u) {	// 000uuuuu zzzzyyyy yyxxxxxx <- 11110uuu 10uuzzzz 10yyyyyy 10xxxxxx
						*(out++) = static_cast<std::uint8_t>((c >> 18) | 0xf0);
						*(out++) = static_cast<std::uint8_t>(((c >> 12) & 0x3f) | 0x80);
						*(out++) = static_cast<std::uint8_t>(((c >> 6) & 0x3f) | 0x80);
						*(out++) = static_cast<std::uint8_t>((c & 0x3f) | 0x80);
					} else
						throw InvalidCodePointException(c);
					return out;
				}
			};

			template<> struct CodeUnitTraits<2> : CodeUnitTraitsBase<2, CodeUnitTraits<2>> {
				template<typename InputIterator>
				static CodePoint checkedDecodeFirst(InputIterator first, InputIterator last) {
					assert(first != last);
					const std::uint16_t high = *first;
					if(surrogates::isHighSurrogate(high)) {
						if(++first == last)
							throw MalformedInputException<InputIterator>(first, 1);
						const std::uint16_t low = *first;
						if(!surrogates::isLowSurrogate(low))
							throw MalformedInputException<InputIterator>(first, 1);
						return surrogates::decode(high, low);
					}
					else if(surrogates::isLowSurrogate(high))
						throw MalformedInputException<InputIterator>(first, 1);
					return high;
				}

				template<typename BidirectionalIterator>
				static CodePoint checkedDecodeLast(BidirectionalIterator first, BidirectionalIterator last) {
					assert(first != last);
					const std::uint16_t low = *--last;
					if(surrogates::isLowSurrogate(low)) {
						if(last == first)
							throw MalformedInputException<BidirectionalIterator>(last, 1);
						const std::uint16_t high = *--last;
						if(!surrogates::isHighSurrogate(high))
							throw MalformedInputException<BidirectionalIterator>(last, 1);
						return surrogates::decode(high, low);
					}
					else if(surrogates::isHighSurrogate(low))
						throw MalformedInputException<BidirectionalIterator>(last, 1);
					return low;
				}

				template<typename OutputIterator>
				static OutputIterator checkedEncode(CodePoint c, OutputIterator out) {
					return encode<true>(c, out);
				}

				template<typename InputIterator>
				static CodePoint decodeFirst(InputIterator first, InputIterator last) {
					assert(first != last);
					const auto high = *first;
					if(surrogates::isHighSurrogate(high))
						return (++first != last) ? surrogates::decode(high, *first) : high;
					return high;
				}

				template<typename BidirectionalIterator>
				static CodePoint decodeLast(BidirectionalIterator first, BidirectionalIterator last) {
					assert(first != last);
					const auto low = *--last;
					if(surrogates::isLowSurrogate(low))
						return (--last != first) ? surrogates::decode(*last, low) : low;
					return low;
				}

				template<typename OutputIterator>
				static OutputIterator encode(CodePoint c, OutputIterator out) {
					return encode<false>(c, out);
				}

				static BOOST_CONSTEXPR bool isLeading(value_type codeUnit) BOOST_NOEXCEPT {
//					return isSingle(codeUnit) || surrogates::isHighSurrogate(codeUnit);
					return isValid(codeUnit) && !surrogates::isLowSurrogate(codeUnit);
				}

				static BOOST_CONSTEXPR bool isSingle(value_type codeUnit) BOOST_NOEXCEPT {
					return isValid(codeUnit) && !surrogates::isSurrogate(codeUnit);
				}

				static BOOST_CONSTEXPR bool isValid(value_type) BOOST_NOEXCEPT {
					return true;
				}

				static std::size_t length(CodePoint c) {
					if(!isScalarValue(c))
						throw InvalidScalarValueException(c);
					return (c < 0x10000u) ? 1 : 2;
				}

				static boost::optional<std::size_t> lengthForLeading(value_type codeUnit) BOOST_NOEXCEPT {
					if(!isValid(codeUnit) || surrogates::isLowSurrogate(codeUnit))
						return boost::none;
					return surrogates::isHighSurrogate(codeUnit) ? 2 : 1;
				}

				static BOOST_CONSTEXPR bool maybeTrailing(value_type codeUnit) BOOST_NOEXCEPT {
					return surrogates::isLowSurrogate(codeUnit);
				}

			private:
				template<bool check, typename OutputIterator>
				static OutputIterator encode(CodePoint c, OutputIterator out) {
					if(c < 0x00010000ul) {
						if(check && surrogates::isSurrogate(c))
							throw InvalidScalarValueException(c);
						*(out++) = static_cast<std::uint16_t>(c & 0xffffu);
					} else if(!check || c < 0x00110000ul) {
						*(out++) = surrogates::highSurrogate(c);
						*(out++) = surrogates::lowSurrogate(c);
					} else
						throw InvalidScalarValueException(c);
					return out;
				}
			};

			template<> struct CodeUnitTraits<4> : CodeUnitTraitsBase<4, CodeUnitTraits<4>> {
				template<typename InputIterator>
				static CodePoint checkedDecodeFirst(InputIterator first, InputIterator last) {
					assert(first != last);
					if(!isScalarValueException(*first))
						throw MalformedInputException<InputIterator>(first, 1);
					return decodeFirst32(first, last);
				}

				template<typename OutputIterator>
				static OutputIterator checkedEncode(CodePoint c, OutputIterator out) {
					if(!isScalarValue(c))
						throw InvalidScalarValueException(c);
					return encode(c, out);
				}

				template<typename BidirectionalIterator>
				static CodePoint checkedDecodeLast(BidirectionalIterator first, BidirectionalIterator last) {
					assert(first != last);
					const std::uint32_t c = *--last;
					if(!isScalarValue(c))
						throw MalformedInputException<BidirectionalIterator>(last, 1);
					return static_cast<CodePoint>(c);
				}

				template<typename InputIterator>
				static CodePoint decodeFirst(InputIterator first, InputIterator last) {
					assert(first != last);
					return static_cast<CodePoint>(*first);
				}

				template<typename BidirectionalIterator>
				static CodePoint decodeLast(BidirectionalIterator first, BidirectionalIterator last) {
					assert(first != last);
					return static_cast<CodePoint>(*--last);
				}

				template<typename OutputIterator>
				static OutputIterator encode(CodePoint c, OutputIterator out) {
					*out = static_cast<std::uint32_t>(c);
					return ++out;
				}

				static BOOST_CONSTEXPR bool isLeading(value_type codeUnit) BOOST_NOEXCEPT {
					return isScalarValue(codeUnit);
				}

				static BOOST_CONSTEXPR bool isSingle(value_type codeUnit) BOOST_NOEXCEPT {
					return isScalarValue(codeUnit);
				}

				static BOOST_CONSTEXPR bool isValid(value_type codeUnit) BOOST_NOEXCEPT {
					return isScalarValue(codeUnit);
				}

				static std::size_t length(CodePoint c) {
					if(!isScalarValue(c))
						throw InvalidScalarValueException(c);
					return 1;
				}

				static boost::optional<std::size_t> lengthForLeading(value_type codeUnit) BOOST_NOEXCEPT {
					return isLeading(codeUnit) ? boost::make_optional<std::size_t>(1) : boost::none;
				}

				static BOOST_CONSTEXPR bool maybeTrailing(value_type) BOOST_NOEXCEPT {
					return false;
				}
			};

			/**
			 * @defgroup utf_decoding_functions UTF-x Decoding Functions
			 * @{
			 */
			/// @copydoc CodeUnitTraits#decodeFirst
			template<typename InputIterator>
			inline CodePoint decodeFirst(InputIterator first, InputIterator last) {
				return CodeUnitTraits<CodeUnitSizeOf<InputIterator>::value>::decodeFirst(first, last);
			}

			/// @copydoc CodeUnitTraitsBase#decodeFirst
			template<typename BidirectionalReadableRange>
			inline CodePoint decodeFirst(const BidirectionalReadableRange& range) {
				return decodeFirst(boost::const_begin(range), boost::const_end(range));
			}

			/// @copydoc CodeUnitTraits#checkedDecodeFirst
			template<typename InputIterator>
			inline CodePoint checkedDecodeFirst(InputIterator first, InputIterator last) {
				return CodeUnitTraits<CodeUnitSizeOf<InputIterator>::value>::checkedDecodeFirst(first, last);
			}

			/// @copydoc CodeUnitTraitsBase#checkedDecodeFirst
			template<typename BidirectionalReadableRange>
			inline CodePoint checkedDecodeFirst(const BidirectionalReadableRange& range) {
				return checkedDecodeFirst(boost::const_begin(range), boost::const_end(range));
			}

			/// @copydoc CodeUnitTraits#decodeLast
			template<typename BidirectionalIterator>
			inline CodePoint decodeLast(BidirectionalIterator first, BidirectionalIterator last) {
				return CodeUnitTraits<CodeUnitSizeOf<BidirectionalIterator>::value>::decodeLast(first, last);
			}

			/// @copydoc CodeUnitTraitsBase#decodeLast
			template<typename BidirectionalReadableRange>
			inline CodePoint decodeLast(const BidirectionalReadableRange& range) {
				return decodeLast(boost::const_begin(range), boost::const_end(range));
			}

			/// @copydoc CodeUnitTraits#checkedDecodeLast
			template<typename BidirectionalIterator>
			inline CodePoint checkedDecodeLast(BidirectionalIterator first, BidirectionalIterator last) {
				return CodeUnitTraits<CodeUnitSizeOf<BidirectionalIterator>::value>::checkedDecodeLast(first, last);
			}

			/// @copydoc CodeUnitTraitsBase#checkedDecodeLast
			template<typename BidirectionalReadableRange>
			inline CodePoint checkedDecodeLast(const BidirectionalReadableRange& range) {
				return checkedDecodeLast(boost::const_begin(range), boost::const_end(range));
			}
			/// @}

			/**
			 * @defgroup utf_encoding_functions UTF-x Encoding Functions
			 * @{
			 */
			/// @copydoc CodeUnitTraits#encode
			template<typename OutputIterator>
			inline OutputIterator encode(CodePoint c, OutputIterator out) {
				return CodeUnitTraits<CodeUnitSizeOf<OutputIterator>::value>::encode(c, out);
			}

			/// @copydoc CodeUnitTraits#checkedEncode
			template<typename OutputIterator>
			inline OutputIterator checkedEncode(CodePoint c, OutputIterator out) {
				return CodeUnitTraits<CodeUnitSizeOf<OutputIterator>::value>::checkedEncode(c, out);
			}
			/// @}
		}
	}
}

#endif // !ASCENSION_UTF_HPP
