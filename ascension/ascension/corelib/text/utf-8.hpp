/**
 * @file utf-8.hpp
 * @author exeal
 * @date 2011-08-19 created
 */

#ifndef ASCENSION_UTF_8_HPP
#define ASCENSION_UTF_8_HPP

#include <ascension/corelib/standard-iterator-adapter.hpp>
#include <ascension/corelib/text/character.hpp>

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
	}

	namespace text {

		/**
		 * @c utf8 namespace provides low level procedures handle UTF-8 character sequence.
		 * @see utf16
		 */
		namespace utf8 {
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
			 * @return The number of bytes written to @a out or 0 if failed
			 */
			template<typename OutputIterator>
			inline length_t encode(CodePoint c, OutputIterator& out) {
				ASCENSION_STATIC_ASSERT(CodeUnitSizeOf<OutputIterator>::value == 1);
				if(c < 0x0080u) {	// 00000000 0xxxxxxx -> 0xxxxxxx
					*(out++) = static_cast<uint8_t>(c);
					return 1;
				} else if(c < 0x0800u) {	// 00000yyy yyxxxxxx -> 110yyyyy 10xxxxxx
					*(out++) = static_cast<uint8_t>((c >> 6) | 0xc0);
					*(out++) = static_cast<uint8_t>((c & 0x3f) | 0x80);
					return 2;
				} else if(c < 0x10000u) {	// zzzzyyyy yyxxxxxx -> 1110zzzz 10yyyyyy 10xxxxxx
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
				return 0;	// invalid code point
			}

			template<typename InputIterator>
			inline InputIterator nextUnsafe(InputIterator i) {
				ASCENSION_STATIC_ASSERT(CodeUnitSizeOf<InputIterator>::value == 1);
				const std::size_t n = length(*i);
				std::advance(i, (n != 0) ? n : 1);
				return i;
			}
		}

		/**
		 * Bidirectional iterator scans a UTF-8 character sequence as UTF-32.
		 * @par Scanned UTF-8 sequence is given by the template parameter @a BaseIterator.
		 * @par This supports four relation operators general bidirectional iterators don't have.
		 *      These are available if @a BaseIterator have these facilities.
		 * @tparam BaseIterator The base bidirectional iterator presents UTF-8 character sequence
		 * @tparam UChar32 The type of 32-bit code unit
		 * @see UTF8To32Iterator, UTF8To32IteratorUnsafe, UTF32To8Iterator, ToUTF32Sequence
		 */
		template<typename BaseIterator = const uint8_t*, typename UChar32 = CodePoint>
		class UnsafeUTF8BidirectionalIterator : public detail::IteratorAdapter<
			UnsafeUTF8BidirectionalIterator<BaseIterator, UChar32>,
			std::iterator<
				std::bidirectional_iterator_tag, UChar32,
				typename std::iterator_traits<BaseIterator>::difference_type,
				const UChar32*, const UChar32
			>
		> {
		public:
			/// Assignment operator.
			Derived& operator=(const FastUTF8To32Iterator<BaseIterator, UChar32>& other) {
				p_ = other.p_;
				cache_ = other.cache_;
				return derived();
			}
			/// Returns the current position.
			BaseIterator tell() const {return p_;}
		protected:
			/// Default constructor.
			UnsafeUTF8BidirectionalIterator() : cache_(INVALID_CODE_POINT) {}
			/// Copy-constructor.
			UnsafeUTF8BidirectionalIterator(const UnsafeUTF8BidirectionalIterator<BaseIterator, UChar32>& other) : p_(other.p_), cache_(other.cache_) {}
			/// Constructor takes a position to start iteration.
			UnsafeUTF8BidirectionalIterator(BaseIterator start) : p_(start), cache_(INVALID_CODE_POINT) {}
		private:
			// detail.IteratorAdapter
			friend class detail::IteratorCoreAccess;
			value_type current() const {
				if(cache_ == INVALID_CODE_POINT)
					cache_ = utf8::decodeUnsafe(p_);
				return cache_;
			}
			void next() {
				p_ = utf8::nextUnsafe(p_);
				cache_ = INVALID_CODE_POINT;
			}
			void previous() {
				p_ = utf8::previousUnsafe(p_);
				cache_ = INVALID_CODE_POINT;
			}
			bool equals(const Derived& other) const {return p_ == other.p_;}
			bool less(const Derived& other) const {return p_ < other.p_;}
		private:
			BaseIterator p_;
			UChar32 cache_;
		};
	}

}

#endif // !ASCENSION_UTF_8_HPP
