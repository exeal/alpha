/**
 * @file character.hpp
 * @author exeal
 * @date 2005-2011 was unicode.hpp
 * @date 2011-04-27 renamed from unicode.hpp
 * @date 2012-2014
 */

#ifndef ASCENSION_CHARACTER_HPP
#define ASCENSION_CHARACTER_HPP

#include <ascension/corelib/basic-types.hpp>
#include <ascension/corelib/future/type-traits.hpp>	// std.integral_constant
#include <boost/optional.hpp>
#include <boost/range/iterator.hpp>
#include <array>
#include <ios>	// std.ios_base.failure

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
		const std::array<Char, 5> NEWLINE_CHARACTERS = {
			LINE_FEED, CARRIAGE_RETURN, NEXT_LINE, LINE_SEPARATOR, PARAGRAPH_SEPARATOR
		};

		namespace detail {
			template<typename Iterator> class IteratorValue {
				typedef typename std::iterator_traits<Iterator>::value_type T1;
				template<typename T> struct T2 : boost::mpl::identity<T> {};
				template<typename T> struct T2<boost::optional<T>> : boost::mpl::identity<typename boost::optional<T>::value_type> {};
			public:
				typedef typename T2<T1>::type type;
			};
		}

		/**
		 * Returns the size of a code unit of the specified code unit sequence in bytes.
		 * @tparam CodeUnitSequence The type represents a code unit sequence
		 */
		template<typename CodeUnitSequence> struct CodeUnitSizeOf
			: std::integral_constant<std::size_t, sizeof(detail::IteratorValue<CodeUnitSequence>::type)> {};
		template<typename T> struct CodeUnitSizeOf<std::back_insert_iterator<T>>
			: std::integral_constant<std::size_t, sizeof(T::value_type)> {};
		template<typename T> struct CodeUnitSizeOf<std::front_insert_iterator<T>>
			: std::integral_constant<std::size_t, sizeof(T::value_type)> {};
		template<typename T, typename U> struct CodeUnitSizeOf<std::ostream_iterator<T, U>>
			: std::integral_constant<std::size_t, sizeof(T)> {};

		/**
		 * The Unicode decoding failed for malformed input.
		 * @tparam InputIterator The type of the return value of @c #position method
		 * @see encoding#Encoder#MALFORMED_INPUT, kernel#fileio#UnmappableCharacterException,
		 *      REPLACEMENT_CHARACTER
		 */
		template<typename InputIterator>
		class MalformedInputException : public std::ios_base::failure {
			// TODO: std.ios_base.failure is derived from std.system_error after C++0x.
		public:
			/**
			 * Constructor.
			 * @param position The position where the malformed input was found
			 * @param maximalSubpartLength "Maximal subpart of an ill-formed subsequence" in
			 *                             Unicode Standard 6.0, D39b
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
			inline bool isSupplemental(CodePoint c) BOOST_NOEXCEPT {
				return (c & 0xffff0000ul) != 0;
			}

			/**
			 * Returns @c true if the specified code unit is high (leading)-surrogate.
			 * @param c The code point
			 * @return true if @a c is high-surrogate
			 */
			inline bool isHighSurrogate(CodePoint c) BOOST_NOEXCEPT {
				return (c & 0xfffffc00ul) == 0xd800u;
			}

			/**
			 * Returns @c true if the specified code unit is low (trailing)-surrogate.
			 * @param c The code point
			 * @return true if @a c is low-surrogate
			 */
			inline bool isLowSurrogate(CodePoint c) BOOST_NOEXCEPT {
				return (c & 0xfffffc00ul) == 0xdc00u;
			}

			/**
			 * Returns @c true if the specified code unit is surrogate.
			 * @param c The code point
			 * @return true if @a c is surrogate
			 */
			inline bool isSurrogate(CodePoint c) BOOST_NOEXCEPT {
				return (c & 0xfffff800ul) == 0xd800u;
			}

			/**
			 * Returns high (leading)-surrogate for the specified code point.
			 * @note If @a c is in BMP, the behavior is undefined.
			 * @param c The code point
			 * @return The high-surrogate code unit for @a c
			 */
			inline Char highSurrogate(CodePoint c) BOOST_NOEXCEPT {
				return static_cast<Char>((c >> 10) & 0xffffu) + 0xd7c0u;
			}

			/**
			 * Returns low (trailing)-surrogate for the specified code point.
			 * @note If @a c is in BMP, the behavior is undefined.
			 * @param c The code point
			 * @return The low-surrogate code unit for @a c
			 */
			inline Char lowSurrogate(CodePoint c) BOOST_NOEXCEPT {
				return static_cast<Char>(c & 0x03ffu) | 0xdc00u;
			}

			/**
			 * Converts the specified surrogate pair to a corresponding code point.
			 * This function does not check the input code units.
			 * @param high A UTF-16 code unit for the high-surrogate
			 * @param low A UTF-16 code unit for the low-surrogate
			 * @return The code point
			 * @see #checkedDecode
			 */
			inline CodePoint decode(std::uint16_t high, std::uint16_t low) BOOST_NOEXCEPT {
				return 0x10000ul + (high - 0xd800u) * 0x0400u + low - 0xdc00u;
			}

			/**
			 * Converts the specified surrogate pair to a corresponding code point.
			 * @param high A UTF-16 code unit for the high-surrogate
			 * @param low A UTF-16 code unit for the low-surrogate
			 * @return The code point
			 * @throw MalformedInputException&lt;std#uint16_t&gt; @a high and/or @a low are invalid
			 * @see #decode
			 */
			inline CodePoint checkedDecode(std::uint16_t high, std::uint16_t low) {
				if(!text::surrogates::isHighSurrogate(high))
					throw MalformedInputException<std::uint16_t>(high);
				if(!text::surrogates::isLowSurrogate(low))
					throw MalformedInputException<std::uint16_t>(low);
				return decode(high, low);
			}

			/**
			 * Searches an isolated surrogate character in the given UTF-16 code unit sequence.
			 * @note About UTF-32 code unit sequence, use <code>std#find_if(...,
			 *       isSurrogate)</code> instead.
			 * @tparam InputIterator The input iterator represents a UTF-16 character sequence
			 * @param first The beginning of the character sequence
			 * @param last The end of the sequence
			 * @return The isolated surrogate or @a last if not found
			 */
			template<typename InputIterator>
			inline InputIterator searchIsolatedSurrogate(InputIterator first, InputIterator last) {
				static_assert(CodeUnitSizeOf<InputIterator>::value == 2,
					"InputIterator should be a 16-bit character sequence.");
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
				return searchIsolatedSurrogate(
					boost::const_begin(characterSequence), boost::const_end(characterSequence));
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
			explicit InvalidCodePointException(CodePoint c)
				: std::out_of_range("Found an invalid code point."), c_(c) {}
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
			explicit InvalidScalarValueException(CodePoint c)
				: std::out_of_range("Found an invalid code point."), c_(c) {}
			/// Returns the code point.
			CodePoint codePoint() const BOOST_NOEXCEPT {return c_;}
		private:
			const CodePoint c_;
		};

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
