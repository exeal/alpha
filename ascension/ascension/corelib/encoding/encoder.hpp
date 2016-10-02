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
#include <boost/any.hpp>
#include <boost/core/noncopyable.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/optional.hpp>
#include <boost/range/iterator_range.hpp>

namespace ascension {
	namespace encoding {
		class EncodingProperties;

		class Encoder : private boost::noncopyable {
		public:
			/// Result of conversion.
			enum Result {
				/// The conversion fully succeeded.
				/// If @a fromNext parameter of the conversion method is less than `boost#const_end(from)`, more input
				/// is required.
				COMPLETED,
				/// The conversion partially succeeded because the destination buffer was not large enough.
				INSUFFICIENT_BUFFER,
				/// The conversion partially succeeded because encounted an unmappable character.
				/// @c fromNext parameter of the conversion method should addresses the unmappable character. If either
				/// @c REPLACE_UNMAPPABLE_CHARACTER or @c IGNORE_UNMAPPABLE_CHARACTER is set, this value never be
				/// returned.
				/// @see #SubstitutionPolicy
				UNMAPPABLE_CHARACTER,
				/// The conversion partially succeeded because detected malformed input.
				/// @c fromNext parameter of the conversion method should addresses the unmappable character.
				/// @c Encoder#fromUnicode should not return this value.
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

			/**
			 * Represents any of the implementation-defined conversion states.
			 * An empty @c State should be treated as the initial state of conversion.
			 */
			typedef boost::any State;

			/// An encoder received bad state value.
			class BadStateException : public std::invalid_argument {
			public:
				/// Default constructor.
				BadStateException() : std::invalid_argument("Bad conversion state.") {}
			};

		public:
			virtual ~Encoder() BOOST_NOEXCEPT;
			static Encoder& defaultInstance() BOOST_NOEXCEPT;

			/// @name Conversion
			/// @{
			bool canEncode(CodePoint c);
			bool canEncode(const StringPiece& s);
			Result fromUnicode(State& state,
				const boost::iterator_range<Byte*>& to, Byte*& toNext, const boost::iterator_range<const Char*>& from, const Char*& fromNext);
			std::string fromUnicode(const StringPiece& from);
			Result toUnicode(State& state,
				const boost::iterator_range<Char*>& to, Char*& toNext, const boost::iterator_range<const Byte*>& from, const Byte*& fromNext);
			String toUnicode(const boost::string_ref& from);
			/// @}

			/// @name Substitution Policy
			/// @{
			Encoder& setSubstitutionPolicy(SubstitutionPolicy newPolicy);
			SubstitutionPolicy substitutionPolicy() const BOOST_NOEXCEPT;
			/// @}

			/// @name Unicode Byte Order Mark
			/// @{
			bool isByteOrderMarkEncountered(const State& decodingState) const;
			Encoder& writeByteOrderMark(bool write = true) BOOST_NOEXCEPT;
			bool writesByteOrderMark() const BOOST_NOEXCEPT;
			/// @}


			/// @name Endianness
			/// @{
			boost::optional<bool> isBigEndian(const State& decodingState) const;
			bool isBigEndianDefault() const BOOST_NOEXCEPT;
			Encoder& setBigEndianAsDefault() BOOST_NOEXCEPT;
			Encoder& setLittleEndianAsDefault() BOOST_NOEXCEPT;
			/// @}

			/// @name Other Attribute
			/// @{
			virtual const EncodingProperties& properties() const BOOST_NOEXCEPT = 0;
			/// @}

		protected:
			Encoder() BOOST_NOEXCEPT;
			/**
			 * Implements @c #isBigEndian method. Default implementation returns @c boost#none.
			 * @param decodingState The decoding conversion state to test
			 * @return The result. See @c #isBigEndian
			 * @throw BadStateException @a state is invalid
			 * @throw boost#bad_any_cast @a state is invalid
			 */
			virtual boost::optional<bool> doIsBigEndian(const State& decodingState) const {
				boost::ignore_unused(decodingState);
				return boost::none;
			}
			/**
			 * Implements @c #isByteOrderMarkEncountered method. Default implementation returns @c false.
			 * @param state The state to test
			 * @return See @c #isByteOrderMarkEncountered
			 * @throw BadStateException @a state is invalid
			 * @throw boost#bad_any_cast @a state is invalid
			 */
			virtual bool doIsByteOrderMarkEncountered(const State& state) const {
				boost::ignore_unused(state);
				return false;
			}
		private:
			/**
			 * Converts the given string from UTF-16 into the native encoding.
			 * @param[in,out] state The conversion state
			 * @param[out] to The destination buffer
			 * @param[out] toNext Points the first unaltered character in the destination buffer after the conversion
			 * @param[in] from The buffer to be converted
			 * @param[in] fromNext Points to the first unconverted character after the conversion
			 * @return The result of the conversion
			 * @throw BadStateException @a state is invalid
			 * @throw boost#bad_any_cast @a state is invalid
			 */
			virtual Result doFromUnicode(State& state,
				const boost::iterator_range<Byte*>& to, Byte*& toNext, const boost::iterator_range<const Char*>& from, const Char*& fromNext) = 0;
			/**
			 * Converts the given string from the native encoding into UTF-16.
			 * @param[in,out] state The conversion state
			 * @param[out] to The destination buffer
			 * @param[out] toNext Points the first unaltered character in the destination buffer after the conversion
			 * @param[in] from The buffer to be converted
			 * @param[in] fromNext Points to the first unconverted character after the conversion
			 * @return The result of the conversion
			 * @throw BadStateException @a state is invalid
			 * @throw boost#bad_any_cast @a state is invalid
			 */
			virtual Result doToUnicode(State& state,
				const boost::iterator_range<Char*>& to, Char*& toNext, const boost::iterator_range<const Byte*>& from, const Byte*& fromNext) = 0;
		private:
			SubstitutionPolicy substitutionPolicy_;
			bool isBigEndianDefault_, writesByteOrderMark_;
		};

		/**
		 * Returns if the given decoding conversion state
		 * @param decodingState The decoding conversion state to test
		 * @retval true The incoming byte sequence is big endian
		 * @retval false The incoming byte sequence is little endian
		 * @retval boost#none The endian is unknown or this encoder has no endianness
		 * @throw BadStateException @a state is invalid
		 */
		inline boost::optional<bool> Encoder::isBigEndian(const State& decodingState) const {
			try {
				return doIsBigEndian(decodingState);
			} catch(const BadStateException&) {
				throw;
			} catch(const boost::bad_any_cast&) {
				throw BadStateException();
			}
		}

		/**
		 * Returns @c true if this encoder uses big endian as default. The default value is @c true.
		 * @see #setBigEndianAsDefault, #setLittleEndianAsDefault
		 */
		inline bool Encoder::isBigEndianDefault() const BOOST_NOEXCEPT {
			return isBigEndianDefault_;
		}

		/**
		 * Returns @c true if @a decodingState represents the state which has encountered a byte order mark in incoming
		 * sequence.
		 * @param decodingState The decoding conversion state to test
		 * @return true if @a state represents the state which has encountered a byte order mark in incoming sequence
		 * @throw BadStateException @a state is invalid
		 */
		inline bool Encoder::isByteOrderMarkEncountered(const State& decodingState) const {
			try {
				return doIsByteOrderMarkEncountered(decodingState);
			} catch(const BadStateException&) {
				throw;
			} catch(const boost::bad_any_cast&) {
				throw BadStateException();
			}
		}

		/**
		 * Returns the substitution policy. The default value is @c DONT_SUBSTITUTE.
		 * @return The substitution policy of this encoder
		 * @see #setSubstitutionPolicy
		 */
		inline Encoder::SubstitutionPolicy Encoder::substitutionPolicy() const BOOST_NOEXCEPT {
			return substitutionPolicy_;
		}

		/**
		 * Returns @c true if this encoder writes Unicode byte order mark into outgoing byte sequence if possible.
		 * The default value is @c false.
		 * @retval true This encoder writes Unicode byte order mark into outgoing byte sequence if possible
		 * @retval false This encoder never writes Unicode byte order mark into outgoing byte sequence
		 * @see #writeByteOrderMark
		 */
		inline bool Encoder::writesByteOrderMark() const BOOST_NOEXCEPT {
			return writesByteOrderMark_;
		}
	}
} // namespace ascension.encoding

#endif // !ASCENSION_ENCODER_HPP
