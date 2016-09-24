/**
 * @file encoder.hpp
 * @author exeal
 * @date 2004-2014
 */

#ifndef ASCENSION_ENCODER_HPP
#define ASCENSION_ENCODER_HPP
#include <ascension/corelib/basic-types.hpp>
#include <ascension/corelib/string-piece.hpp>
#include <ascension/corelib/text/code-point.hpp>
#include <boost/core/noncopyable.hpp>
#include <bitset>

namespace ascension {
	namespace encoding {
		class EncodingProperties;

		class Encoder : private boost::noncopyable {
		public:
			/// Result of conversion.
			enum ConversionResult {
				/**
				 * The conversion fully succeeded. If @a fromNext parameter of the conversion
				 * method is less @a fromEnd, more input is required.
				 */
				COMPLETED,
				/**
				 * The conversion partially succeeded because the destination buffer was not large
				 * enough.
				 */
				INSUFFICIENT_BUFFER,
				/**
				 * The conversion partially succeeded because encounted an unmappable character.
				 * @c fromNext parameter of the conversion method should addresses the unmappable
				 * character. If either @c REPLACE_UNMAPPABLE_CHARACTER or
				 * @c IGNORE_UNMAPPABLE_CHARACTER is set, this value never be returned.
				 * @see #SubstitutionPolicy
				 */
				UNMAPPABLE_CHARACTER,
				/**
				 * The conversion partially succeeded because detected malformed input.
				 * @c fromNext parameter of the conversion method should addresses the unmappable
				 * character. @c Encoder#fromUnicode should not return this value.
				 */
				MALFORMED_INPUT
			};

			/**
			 * Specifies how to handle unmappable bytes/characters.
			 * @see #substitutionPolicy, #setSubstitutionPolicy
			 */
			enum SubstitutionPolicy {
				/// Aborts with @c UNMAPPABLE_CHARACTER return value.
				DONT_SUBSTITUTE,
				/// Replaces unmappable bytes/characters with replacement characters/bytes.
				REPLACE_UNMAPPABLE_CHARACTERS,
				/// Skips (ignores) unmappable bytes/characters.
				IGNORE_UNMAPPABLE_CHARACTERS
			};

			/// Miscellaneous conversion options.
			enum OptionBits {
				/**
				 * Indicates that @a from parameter of the conversion method addresses the
				 * beginning of the entire input sequence and @a to parameter addresses the
				 * beginning of the entire output sequence.
				 */
				BEGINNING_OF_BUFFER,
				/**
				 * Indicates that @a fromEnd parameter of the conversion method addresses the end
				 * of the entire input sequence.
				 */
				END_OF_BUFFER,
				/**
				 * Indicates that incoming or outgoing buffer contains a Unicode byte order mark
				 * (BOM). If you set this flag without @c FROM_IS_NOT_BOB when encoding, the
				 * encoder writes BOM into the beginning of the output byte sequence. And the
				 * decoder sets this flag if the input byte sequence contained BOM and the other
				 * flag @c FROM_IS_NOT_BOB was not set.
				 */
				UNICODE_BYTE_ORDER_MARK,
				NUMBER_OF_OPTIONS
			};

		public:
			virtual ~Encoder() BOOST_NOEXCEPT;
			static Encoder& defaultInstance() BOOST_NOEXCEPT;

			/// @name Attributes
			/// @{
			const std::bitset<NUMBER_OF_OPTIONS>& options() const BOOST_NOEXCEPT;
			virtual const EncodingProperties& properties() const BOOST_NOEXCEPT = 0;
			virtual Encoder& resetDecodingState() BOOST_NOEXCEPT;
			virtual Encoder& resetEncodingState() BOOST_NOEXCEPT;
			Encoder& setOptions(const std::bitset<NUMBER_OF_OPTIONS> newOptions) BOOST_NOEXCEPT;
			Encoder& setSubstitutionPolicy(SubstitutionPolicy newPolicy);
			SubstitutionPolicy substitutionPolicy() const BOOST_NOEXCEPT;
			/// @}

			/// @name Conversion
			/// @{
			bool canEncode(CodePoint c);
			bool canEncode(const StringPiece& s);
			ConversionResult fromUnicode(Byte* to, Byte* toEnd, Byte*& toNext, const Char* from, const Char* fromEnd, const Char*& fromNext);
			std::string fromUnicode(const StringPiece& from);
			ConversionResult toUnicode(Char* to, Char* toEnd, Char*& toNext, const Byte* from, const Byte* fromEnd, const Byte*& fromNext);
			String toUnicode(const boost::string_ref& from);
			/// @}

		protected:
			Encoder() BOOST_NOEXCEPT;
		private:
			/**
			 * Converts the given string from UTF-16 into the native encoding.
			 * @param[out] to The beginning of the destination buffer
			 * @param[out] toEnd The end of the destination buffer
			 * @param[out] toNext Points the first unaltered character in the destination buffer after the conversion
			 * @param[in] from The beginning of the buffer to be converted
			 * @param[in] fromEnd The end of the buffer to be converted
			 * @param[in] fromNext Points to the first unconverted character after the conversion
			 * @return The result of the conversion
			 */
			virtual ConversionResult doFromUnicode(Byte* to, Byte* toEnd, Byte*& toNext,
				const Char* from, const Char* fromEnd, const Char*& fromNext) = 0;
			/**
			 * Converts the given string from the native encoding into UTF-16.
			 * @param[out] to The beginning of the destination buffer
			 * @param[out] toEnd The end of the destination buffer
			 * @param[out] toNext Points the first unaltered character in the destination buffer
			 *             after the conversion
			 * @param[in] from The beginning of the buffer to be converted
			 * @param[in] fromEnd The end of the buffer to be converted
			 * @param[in] fromNext Points to the first unconverted character after the conversion
			 * @return The result of the conversion
			 */
			virtual ConversionResult doToUnicode(Char* to, Char* toEnd, Char*& toNext,
				const Byte* from, const Byte* fromEnd, const Byte*& fromNext) = 0;
		private:
			SubstitutionPolicy substitutionPolicy_;
			std::bitset<NUMBER_OF_OPTIONS> options_;	// see OptionBits enums
		};

		/// Returns the miscellaneous options.
		inline const std::bitset<Encoder::NUMBER_OF_OPTIONS>& Encoder::options() const BOOST_NOEXCEPT {
			return options_;
		}

		/// Returns the substitution policy.
		inline Encoder::SubstitutionPolicy Encoder::substitutionPolicy() const BOOST_NOEXCEPT {
			return substitutionPolicy_;
		}
	}
} // namespace ascension.encoding

#endif // !ASCENSION_ENCODER_HPP
