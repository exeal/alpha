/**
 * @file code-point.hpp
 * Defines @c CodePoint data type and related free functions.
 * @author exeal
 * @date 2005-2011 was unicode.hpp
 * @date 2011-04-27 renamed from unicode.hpp
 * @date 2016-08-15 Separated from character.hpp.
 */

#ifndef ASCENSION_CODE_POINT_HPP
#define ASCENSION_CODE_POINT_HPP
#include <ascension/corelib/text/character.hpp>
#include <ascension/corelib/text/code-unit-size-of.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/iterator.hpp>
#include <ios>	// std.ios_base.failure
#include <stdexcept>

#if ASCENSION_UNICODE_VERSION > 0x0510
#	error These class definitions and implementations are based on old version of Unicode.
#endif

namespace ascension {
	namespace text {
		/// Unicode code point.
		typedef std::uint32_t CodePoint;
		static_assert(sizeof(CodePoint) == 4, "");
		/// Invalid code point value.
		const CodePoint INVALID_CODE_POINT = 0xfffffffful;

		/**
		 * The Unicode decoding failed for malformed input.
		 * @tparam InputIterator The type of the return value of @c #position method
		 * @see encoding#Encoder#MALFORMED_INPUT, kernel#fileio#UnmappableCharacterException,
		 *      REPLACEMENT_CHARACTER
		 */
		template<typename InputIterator>
		class MalformedInputException : public std::ios_base::failure {
			// TODO: std.ios_base.failure is derived from std.system_error since C++11.
		public:
			/**
			 * Constructor.
			 * @param position The position where the malformed input was found
			 * @param maximalSubpartLength "Maximal subpart of an ill-formed subsequence" in Unicode Standard 6.0, D39b
			 * @throw std#length_error @a maximalSubpartLength is zero
			 */
			explicit MalformedInputException(
					InputIterator position = InputIterator(), std::size_t maximalSubpartLength = 1) :
					std::ios_base::failure("Detected malformed input in decoding."),
					position_(position), maximalSubpartLength_(maximalSubpartLength) {
				if(maximalSubpartLength == 0)
					throw std::length_error("maximalSubpartLength");
			}
			/// Returns the length of the maximal subpart.
			std::size_t maximalSubpartLength() const BOOST_NOEXCEPT {return maximalSubpartLength_;}
			/// Returns the position where the malformed input was found.
			const InputIterator& position() const BOOST_NOEXCEPT {return position_;}
		private:
			const InputIterator position_;
			const std::size_t maximalSubpartLength_;	// see Unicode 6.0, D39b
		};

		/**
		 * @c surrogates namespace collects low level procedures handle UTF-16 surrogate pair.
		 * @see CharacterDecodeIterator, CharacterEncodeIterator, CharacterOutputIterator
		 */
		namespace surrogates {
			/**
			 * Returns @c true if the specified code point is supplemental (out of BMP).
			 * @param c The code point
			 * @return true if @a c is supplemental
			 */
			inline BOOST_CONSTEXPR bool isSupplemental(CodePoint c) BOOST_NOEXCEPT {
				return (c & 0xffff0000ul) != 0;
			}

			/**
			 * Returns @c true if the specified code unit is high (leading)-surrogate.
			 * @param c The code point
			 * @return true if @a c is high-surrogate
			 */
			inline BOOST_CONSTEXPR bool isHighSurrogate(CodePoint c) BOOST_NOEXCEPT {
				return (c & 0xfffffc00ul) == 0xd800u;
			}

			/**
			 * Returns @c true if the specified code unit is low (trailing)-surrogate.
			 * @param c The code point
			 * @return true if @a c is low-surrogate
			 */
			inline BOOST_CONSTEXPR bool isLowSurrogate(CodePoint c) BOOST_NOEXCEPT {
				return (c & 0xfffffc00ul) == 0xdc00u;
			}

			/**
			 * Returns @c true if the specified code unit is surrogate.
			 * @param c The code point
			 * @return true if @a c is surrogate
			 */
			inline BOOST_CONSTEXPR bool isSurrogate(CodePoint c) BOOST_NOEXCEPT {
				return (c & 0xfffff800ul) == 0xd800u;
			}

			/**
			 * Returns high (leading)-surrogate for the specified code point.
			 * @note If @a c is in BMP, the behavior is undefined.
			 * @param c The code point
			 * @return The high-surrogate code unit for @a c
			 */
			inline BOOST_CONSTEXPR Char highSurrogate(CodePoint c) BOOST_NOEXCEPT {
				return static_cast<Char>((c >> 10) & 0xffffu) + 0xd7c0u;
			}

			/**
			 * Returns low (trailing)-surrogate for the specified code point.
			 * @note If @a c is in BMP, the behavior is undefined.
			 * @param c The code point
			 * @return The low-surrogate code unit for @a c
			 */
			inline BOOST_CONSTEXPR Char lowSurrogate(CodePoint c) BOOST_NOEXCEPT {
				return static_cast<Char>(c & 0x03ffu) | 0xdc00u;
			}

			/**
			 * Converts the specified surrogate pair to a corresponding code point.
			 * This function does not check the input code units.
			 * @tparam Surrogate The type of @a high and @a low
			 * @param high A UTF-16 code unit for the high-surrogate
			 * @param low A UTF-16 code unit for the low-surrogate
			 * @return The code point
			 * @see #checkedDecode
			 */
			template<typename Surrogate>
			inline BOOST_CONSTEXPR CodePoint decode(Surrogate high, Surrogate low) BOOST_NOEXCEPT {
				static_assert(sizeof(Surrogate) == 2, "Surrogate should be 16-bit.");
				return 0x10000ul + (high - 0xd800u) * 0x0400u + low - 0xdc00u;
			}

			/**
			 * Converts the specified surrogate pair to a corresponding code point.
			 * @tparam Surrogate The type of @a high and @a low
			 * @param high A UTF-16 code unit for the high-surrogate
			 * @param low A UTF-16 code unit for the low-surrogate
			 * @return The code point
			 * @throw MalformedInputException&lt;std#uint16_t&gt; @a high and/or @a low are invalid
			 * @see #decode
			 */
			template<typename Surrogate>
			inline CodePoint checkedDecode(Surrogate high, Surrogate low) {
				static_assert(sizeof(Surrogate) == 2, "Surrogate should be 16-bit.");
				if(!text::surrogates::isHighSurrogate(high))
					throw MalformedInputException<Surrogate>(high);
				if(!text::surrogates::isLowSurrogate(low))
					throw MalformedInputException<Surrogate>(low);
				return decode(high, low);
			}

			/**
			 * Searches an isolated surrogate character in the given UTF-16 code unit sequence.
			 * @note About UTF-32 code unit sequence, use <code>std#find_if(..., isSurrogate)</code> instead.
			 * @tparam InputIterator The input iterator represents a UTF-16 character sequence
			 * @param first The beginning of the character sequence
			 * @param last The end of the sequence
			 * @return The isolated surrogate or @a last if not found
			 */
			template<typename InputIterator>
			inline InputIterator searchIsolatedSurrogate(InputIterator first, InputIterator last) {
				static_assert(CodeUnitSizeOf<InputIterator>::value == 2, "InputIterator should be a 16-bit character sequence.");
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

			/**
			 * @overload
			 * @tparam SinglePassReadableRange UTF-16 character sequence
			 * @param characterSequence The character sequence
			 * @return The isolated surrogate or @c boost#end(characterSequence) if not found
			 */
			template<typename SinglePassReadableRange>
			inline typename boost::range_iterator<const SinglePassReadableRange>::type
					searchIsolatedSurrogate(const SinglePassReadableRange& characterSequence) {
				return searchIsolatedSurrogate(boost::const_begin(characterSequence), boost::const_end(characterSequence));
			}
		} // namespace surrogates

		/**
		 * Returns @c true if the specified code point is in Unicode codespace (0..10FFFF).
		 * @see InvalidCodePointException
		 */
		inline bool isValidCodePoint(CodePoint c) BOOST_NOEXCEPT {return c <= 0x10fffful;}

		/**
		 * Returns @c true if the specified code point is Unicode scalar value.
		 * @see InvalidScalarValueException
		 */
		inline bool isScalarValue(CodePoint c) BOOST_NOEXCEPT {
			return isValidCodePoint(c) && !surrogates::isSurrogate(c);
		}

		/**
		 * Faced an invalid code point.
		 * @see isValidCodePoint
		 */
		class InvalidCodePointException : public std::out_of_range {
		public:
			/**
			 * Constructor.
			 * @param c The code point
			 */
			explicit InvalidCodePointException(CodePoint c) : std::out_of_range("Found an invalid code point."), c_(c) {}
			/// Returns the code point.
			CodePoint codePoint() const BOOST_NOEXCEPT {return c_;}
		private:
			const CodePoint c_;
		};

		/**
		 * Faced an invalid scalar value.
		 * @see isScalarValue
		 */
		class InvalidScalarValueException : public std::out_of_range {
		public:
			/**
			 * Constructor.
			 * @param c The code point
			 */
			explicit InvalidScalarValueException(CodePoint c) : std::out_of_range("Found an invalid code point."), c_(c) {}
			/// Returns the code point.
			CodePoint codePoint() const BOOST_NOEXCEPT {return c_;}
		private:
			const CodePoint c_;
		};

		/// Case sensitivities for caseless-match.
		enum CaseSensitivity {	// TODO: Why is this definition here?
			CASE_SENSITIVE,							///< Case-sensitive.
			CASE_INSENSITIVE,						///< Case-insensitive.
			CASE_INSENSITIVE_EXCLUDING_TURKISH_I	///< Case-insensitive and excludes Turkish I.
		};

		/// Types of decomposition mapping.
		enum Decomposition {	// TODO: Why is this definition here?
			NO_DECOMPOSITION,			///< No decomposition.
			CANONICAL_DECOMPOSITION,	///< Canonical decomposition mapping.
			FULL_DECOMPOSITION			///< Canonical and compatibility mapping.
		};
	}

	using text::CodePoint;
} // namespace ascension.text

#endif // !ASCENSION_CODE_POINT_HPP
