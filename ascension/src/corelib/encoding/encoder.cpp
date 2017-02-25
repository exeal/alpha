/**
 * @file encoder.cpp
 * @author exeal
 * @date 2004-2014
 */

#include <ascension/corelib/encoding/encoder.hpp>
#include <ascension/corelib/encoding/encoder-factory.hpp>
#include <ascension/corelib/basic-exceptions.hpp>
#include <ascension/corelib/text/utf.hpp>	// text.isScalarValue, text.utf.encode
#include <memory>							// std.unique_ptr
#include <boost/foreach.hpp>


namespace ascension {
	namespace encoding {		
		/**
		 * @class ascension::encoding::Encoder
		 * @c Encoder class provides conversions between text encodings.
		 *
		 * Ascension uses Unicode to store and manipulate strings. However, different encodings are preferred in many
		 * cases. Ascension provides a collection of @c Encoder classes to help with converting non-Unicode formats
		 * to/from Unicode.
		 *
		 * You can convert a native encoded string into Unicode by using @c #toUnicode method.
		 *
		 * @code
		 * const std::string native(...);
		 * std::unique_ptr<Encoder> encoder(Encoder::forName("Shift_JIS"));
		 * if(encoder->get() != nullptr) {
		 *   const String unicode(encoder->toUnicode(native));
		 *   ...
		 * }
		 * @endcode
		 *
		 * Converting a string from Unicode into native encoded can be done by @c #fromUnicode method.
		 *
		 * @note This class is not compatible with C++ standard @c std#codecvt template class.
		 *
		 * <h3>Substitution Policy</h3>
		 *
		 * When an encoder encounters unmappable byte/character, its behavior depends on the substitution policy.
		 * Available policies are enumerated by @c #SubstitutionPolicy. You can get the current substitution policy by
		 * @c #substitutionPolicy, and can set by @c #setSubstitutionPolicy. Default policy is
		 * @c SubstitutionPolicy#DONT_SUBSTITUTE.
		 *
		 * If you use @c SubstitutionPolicy#DONT_SUBSTITUTE, the encoder returns @c UNMAPPABLE_CHARACTER when
		 * encountered an unmappable byte/character. In this case, @a fromNext parameter of the conversion method will
		 * address the unmappable byte/character.
		 *
		 * Substitution policy can't handle malformed input. See the next section.
		 *
		 * <h3>Malformed Input</h3>
		 *
		 * When an encoder encounters illegal byte/character sequence (malformed input), returns @c MALFORMED_INPUT.
		 * This behavior can't be changed by the caller. In this case, @a fromNext parameter of the conversion method
		 * will address the unmappable byte/character.
		 *
		 * Note: an encoder considers that `boost#const_end(from)` of the conversion method addresses the end of the
		 * entire input sequence.
		 *
		 * <h3>Intermediate Conversion State of Encoder</h3>
		 *
		 * An encoder may have the own conversion state to implement stateful encoding. For streaming operation (ex.
		 * receiving data over a network), the conversion methods of @c Encoder accept a part of the entire input. An
		 * encoder keeps its own conversion state between the several invocations of the conversion methods. The
		 * following illustrates the case of ISO-2022-JP:
		 *
		 * @code
		 * std::unique_ptr<Encoder> encoder = Encoder::forName("ISO-2022-JP");
		 *
		 * // give an escape sequence to switch to JIS X 0208:1983
		 * encoder->toUnicode("\x1B$B"); 
		 *
		 * // this generates two kanji from 4 bytes
		 * const String s = encoder->toUnicode("4A;z");
		 * @endcode
		 *
		 * Conversion states can be cleared by @c #resetDecodingState and @c #resetEncodingState.
		 *
		 * <h3>Making User-Defined @c Encoder Classes</h3>
		 *
		 * You can create and add your own @c Encoder class.
		 */

		/// Protected default constructor.
		Encoder::Encoder() BOOST_NOEXCEPT : substitutionPolicy_(DONT_SUBSTITUTE), isBigEndianDefault_(true), writesByteOrderMark_(false) {
		}
		
		/// Destructor.
		Encoder::~Encoder() BOOST_NOEXCEPT {
		}

		/**
		 * Returns @c true if the given character can be fully encoded with this encoding.
		 * This calls @c #resetEncodingState method.
		 * @param c The code point of the character
		 * @return Succeeded or not
		 * @throw text#InvalidScalarValueException @a c is not a Unicode scalar value
		 */
		bool Encoder::canEncode(CodePoint c) {
			if(!text::isScalarValue(c))
				throw text::InvalidScalarValueException(c);
			Char buffer[2];
			Char* p = buffer;
			return canEncode(String(p, text::utf::encode(c, p)));
		}

		/**
		 * Returns @c true if the given string can be fully encoded with this encoding.
		 * This calls @c #resetEncodingState method.
		 * @param s The string
		 * @return Succeeded or not
		 * @throw NullPointerException @a s is @c null
		 * @throw std#invalid_argument @a s is empty
		 */
		bool Encoder::canEncode(const StringPiece& s) {
			if(s.cbegin() == nullptr)
				throw NullPointerException("s");
			else if(s.empty())
				throw std::invalid_argument("s");
			// TODO: Should be able to implement without heap/free store...
			static const std::size_t BUFFER_SIZE = 128;
			Byte buffer[BUFFER_SIZE];
			std::unique_ptr<Byte[]> allocatedBuffer;
			const std::size_t nbytes = s.length() * properties().maximumNativeBytes();
			boost::iterator_range<Byte*> to;
			if(nbytes > BUFFER_SIZE) {
				allocatedBuffer.reset(new Byte[nbytes]);
				to = boost::make_iterator_range_n(allocatedBuffer.get(), nbytes);
			} else
				to = boost::make_iterator_range_n(buffer, nbytes);
			const Char* fromNext;
			Byte* toNext;
			State state;
			return fromUnicode(state, to, toNext, boost::make_iterator_range(s.cbegin(), s.cend()), fromNext) == COMPLETED;
		}

		/// Returns the default encoder.
		Encoder& Encoder::defaultInstance() BOOST_NOEXCEPT {
//#if BOOST_OS_WINDOWS
//			return convertWin32CPtoMIB(::GetACP());
//#else
			static std::unique_ptr<Encoder> instance(EncoderRegistry::instance().forMIB(fundamental::UTF_8));
			return *instance;
//#endif // BOOST_OS_WINDOWS
		}

		/**
		 * Converts the given string from UTF-16 into the native encoding.
		 * @param[in,out] The conversion state
		 * @param[out] to The destination buffer
		 * @param[out] toNext Points the first unaltered character in the destination buffer after the conversion
		 * @param[in] from The buffer to be converted
		 * @param[in] fromNext Points to the first unconverted character after the conversion
		 * @return The result of the conversion
		 * @throw BadStateException @a state is invalid
		 * @throw NullPointerException @a to and/or @a from is @c null
		 * @throw std#invalid_argument @a to and/or @a from is not ordered
		 */
		Encoder::Result Encoder::fromUnicode(State& state,
				const boost::iterator_range<Byte*>& to, Byte*& toNext, const boost::iterator_range<const Char*>& from, const Char*& fromNext) {
			if(boost::const_begin(to) == nullptr || boost::const_end(to) == nullptr)
				throw NullPointerException("to");
			if(boost::const_begin(from) == nullptr || boost::const_end(from) == nullptr)
				throw NullPointerException("from");
			else if(boost::const_begin(to) > boost::const_end(to))
				throw std::invalid_argument("to is not ordered");
			else if(boost::const_begin(from) > boost::const_end(from))
				throw std::invalid_argument("from is not ordered");
			try {
				return doFromUnicode(state, to, toNext = nullptr, from, fromNext = nullptr);
			} catch(const boost::bad_any_cast&) {
				throw BadStateException();
			}
		}

		/**
		 * Converts the given string from UTF-16 into the native encoding. This calls @c #resetEncodingState method.
		 * @param from The string to be converted
		 * @return The converted string or an empty if encountered unconvertible character
		 * @throw std#bad_alloc
		 */
		std::string Encoder::fromUnicode(const StringPiece& from) {
			std::size_t nbytes = properties().maximumNativeBytes() * from.length();
			std::unique_ptr<Byte[]> temp(new Byte[nbytes]);
			const Char* fromNext;
			Byte* toNext;
			while(true) {
				State state;
				const auto result = fromUnicode(state, boost::make_iterator_range(temp.get(), temp.get() + nbytes), toNext, boost::make_iterator_range(from), fromNext);
				if(result == COMPLETED)
					break;
				else if(result == INSUFFICIENT_BUFFER) {
					temp.reset(new(std::nothrow) Byte[nbytes *= 2]);
					if(temp.get() == nullptr)
						throw std::bad_alloc();
				} else
					return std::string();
			}
			return std::string(temp.get(), toNext);
		}

		/**
		 * Sets the default endianness to big endian.
		 * @return This encoder
		 * @see #isBigEndianDefault, #setLittleEndianAsDefault
		 */
		Encoder& Encoder::setBigEndianAsDefault() BOOST_NOEXCEPT {
			return (isBigEndianDefault_ = true), *this;
		}

		/**
		 * Sets the default endianness to little endian.
		 * @return This encoder
		 * @see #isBigEndianDefault, #setBigEndianAsDefault
		 */
		Encoder& Encoder::setLittleEndianAsDefault() BOOST_NOEXCEPT {
			return (isBigEndianDefault_ = false), *this;
		}

		/**
		 * Sets the conversion policy.
		 * @param newPolicy The new policy
		 * @return This encoder
		 * @throw UnknownValueException&lt;SubstitutionPolicy&gt; @a newPolicy is invalid
		 * @see #substitutionPolicy
		 */
		Encoder& Encoder::setSubstitutionPolicy(SubstitutionPolicy newPolicy) {
			if(newPolicy < DONT_SUBSTITUTE || newPolicy > IGNORE_UNMAPPABLE_CHARACTERS)
				throw UnknownValueException("newPolicy");
			return (substitutionPolicy_ = newPolicy), *this;
		}

		/**
		 * Converts the given string from the native encoding into UTF-16.
		 * @param[in,out] state The conversion state
		 * @param[out] to The destination buffer
		 * @param[out] toNext Points the first unaltered character in the destination buffer after the conversion
		 * @param[in] from The buffer to be converted
		 * @param[in] fromNext Points to the first unconverted character after the conversion
		 * @return The result of the conversion
		 * @throw BadStateException @a state is invalid
		 * @throw NullPointerException @a to and/or @a from is @c null
		 * @throw std#invalid_argument @a to and/or @a from is not ordered
		 */
		Encoder::Result Encoder::toUnicode(State& state,
				const boost::iterator_range<Char*>& to, Char*& toNext, const boost::iterator_range<const Byte*>& from, const Byte*& fromNext) {
			if(boost::const_begin(to) == nullptr || boost::const_end(to) == nullptr)
				throw NullPointerException("to");
			if(boost::const_begin(from) == nullptr || boost::const_end(from) == nullptr)
				throw NullPointerException("from");
			else if(boost::const_begin(to) > boost::const_end(to))
				throw std::invalid_argument("to is not ordered");
			else if(boost::const_begin(from) > boost::const_end(from))
				throw std::invalid_argument("from is not ordered");
			try {
				return doToUnicode(state, to, toNext = nullptr, from, fromNext = nullptr);
			} catch(const boost::bad_any_cast&) {
				throw BadStateException();
			}
		}

		/**
		 * Converts the given string from the native encoding into UTF-16.
		 * @param from The string to be converted
		 * @return The converted string or an empty if encountered unconvertible character
		 */
		String Encoder::toUnicode(const boost::string_ref& from) {
			std::size_t nchars = properties().maximumUCSLength() * from.length();
			std::unique_ptr<Char[]> temp(new Char[nchars]);
			const Byte* fromNext;
			Char* toNext;
			while(true) {
				State state;
				const auto result = toUnicode(state, boost::make_iterator_range(temp.get(), temp.get() + nchars), toNext,
					boost::make_iterator_range(reinterpret_cast<const Byte*>(from.data()), reinterpret_cast<const Byte*>(from.data()) + from.length()), fromNext);
				if(result == COMPLETED)
					break;
				else if(result == INSUFFICIENT_BUFFER)
					temp.reset(new Char[nchars *= 2]);
				else
					return String();
			}
			return String(temp.get(), toNext);
		}

		/**
		 * Sets the byte order mark flag.
		 * @param write Set @c true to write the byte order mark into outgoing byte sequence
		 * @return This encoder
		 * @see #writesByteOrderMark
		 */
		Encoder& Encoder::writeByteOrderMark(bool write /* = true */) BOOST_NOEXCEPT {
			return (writesByteOrderMark_ = write), *this;
		}
	}
}
