/**
 * @file unicode-surrogates.hpp
 * @author exeal
 * @date 2005-2010 (was unicode.hpp)
 * @date 2010 (was character-iterator.hpp)
 * @date 2010-11-06 separated from character-iterator.hpp
 * @date 2011
 * @see unicode.hpp
 */

#ifndef ASCENSION_UNICODE_SURROGATES_HPP
#define ASCENSION_UNICODE_SURROGATES_HPP

#include <ascension/corelib/basic-types.hpp>	// CodePoint, ASCENSION_STATIC_ASSERT
#include <cassert>								// assert
#include <stdexcept>							// std.invalid_argument

#if ASCENSION_UNICODE_VERSION > 0x0510
#error These class definitions and implementations are based on old version of Unicode.
#endif

namespace ascension {
	namespace text {

		/**
		 * Returns the size of a code unit of the specified code unit sequence in bytes.
		 * @tparam CodeUnitSequence the type represents a code unit sequence
		 * @see ToUTF32Sequence
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
		 * @see UTF16To32Iterator, UTF32To16Iterator
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
				return (++first != last) ? decode(high, *first) : high;
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
				return (last != first && isLowSurrogate(low)
					&& isHighSurrogate(*--last)) ? decode(*last, low) : low;
			}

			/**
			 * Converts the specified code point to a corresponding surrogate pair.
			 * @tparam OutputIterator The output iterator represents a UTF-16 character sequence
			 * @param c The code point
			 * @param[out] dest The surrogate pair
			 * @retval 0 @a c is a surrogate. in this case, @a *dest will be @a c
			 * @retval 1 @a c is in BMP
			 * @retval 2 @a c is out of BMP
			 * @throw std#invalid_argument @a c can't be expressed by UTF-16
			 */
			template<typename OutputIterator>
			inline length_t encode(CodePoint c, OutputIterator dest) {
				ASCENSION_STATIC_ASSERT(CodeUnitSizeOf<OutputIterator>::value == 2);
				if(c < 0x00010000ul) {
					*dest = static_cast<Char>(c & 0xffffu);
					return !isSurrogate(c) ? 1 : 0;
				} else if(c <= 0x0010fffful) {
					*dest = highSurrogate(c);
					*++dest = lowSurrogate(c);
					return 2;
				}
				throw std::invalid_argument("the specified code point is not valid.");
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
				return (isHighSurrogate(*(start++))
					&& (start != last) && isLowSurrogate(*start)) ? ++start : start;
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
				return (!isLowSurrogate(*--start)
					|| (start == first) || isHighSurrogate(*--start)) ? start : ++start;
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
	}
} // namespace ascension.text

#endif // !ASCENSION_UNICODE_SURROGATES_HPP
