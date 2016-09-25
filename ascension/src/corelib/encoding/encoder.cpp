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
		 * <h3>Miscellaneous Flags</h3>
		 *
		 * An encodoer has flags represented by @c Flags. The initial value is @c BEGINNING_OF_BUFFER |
		 * @c END_OF_BUFFER.
		 *
		 * <h3>Making User-Defined @c Encoder Classes</h3>
		 *
		 * You can create and add your own @c Encoder class.
		 */

		/// Protected default constructor.
		Encoder::Encoder() BOOST_NOEXCEPT : substitutionPolicy_(DONT_SUBSTITUTE) {
			options_.set(BEGINNING_OF_BUFFER);
			options_.set(END_OF_BUFFER);
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
			return canEncode(StringPiece(buffer, text::utf::encode(c, p)));
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
			const std::size_t bytes = s.length() * properties().maximumNativeBytes();
			std::unique_ptr<Byte[]> temp(new Byte[bytes]);
			const Char* fromNext;
			Byte* toNext;
			resetEncodingState();
			return fromUnicode(temp.get(), temp.get() + bytes, toNext, s.cbegin(), s.cend(), fromNext) == COMPLETED;
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
		 * @param[out] to The beginning of the destination buffer
		 * @param[out] toEnd The end of the destination buffer
		 * @param[out] toNext Points the first unaltered character in the destination buffer after the conversion
		 * @param[in] from The beginning of the buffer to be converted
		 * @param[in] fromEnd The end of the buffer to be converted
		 * @param[in] fromNext Points to the first unconverted character after the conversion
		 * @return The result of the conversion
		 * @throw NullPointerException @a to, @a toEnd, @a from and/or @a fromEnd is @c null
		 * @throw std#invalid_argument @a to &gt; @a toEnd or @a from &gt; @a fromEnd
		 */
		Encoder::ConversionResult Encoder::fromUnicode(Byte* to, Byte* toEnd,
				Byte*& toNext, const Char* from, const Char* fromEnd, const Char*& fromNext) {
			if(to == nullptr || toEnd == nullptr || from == nullptr || fromEnd == nullptr)
				throw NullPointerException("");
			else if(to > toEnd)
				throw std::invalid_argument("to > toEnd");
			else if(from > fromEnd)
				throw std::invalid_argument("from > fromEnd");
			toNext = nullptr;
			fromNext = nullptr;
			return doFromUnicode(to, toEnd, toNext, from, fromEnd, fromNext);
		}

		/**
		 * Converts the given string from UTF-16 into the native encoding. This calls @c #resetEncodingState method.
		 * @param from The string to be converted
		 * @return The converted string or an empty if encountered unconvertible character
		 * @throw std#bad_alloc
		 */
		std::string Encoder::fromUnicode(const StringPiece& from) {
			std::size_t bytes = properties().maximumNativeBytes() * from.length();
			std::unique_ptr<Byte[]> temp(new Byte[bytes]);
			const Char* fromNext;
			Byte* toNext;
			ConversionResult result;
			resetEncodingState();
			while(true) {
				result = fromUnicode(temp.get(), temp.get() + bytes, toNext, from.data(), from.data() + from.length(), fromNext);
				if(result == COMPLETED)
					break;
				else if(result == INSUFFICIENT_BUFFER) {
					temp.reset(new(std::nothrow) Byte[bytes *= 2]);
					if(temp.get() == nullptr)
						throw std::bad_alloc();
				} else
					return std::string();
			}
			return std::string(temp.get(), toNext);
		}

		/**
		 * Resets the intermediate conversion state for @c #toUnicode to the default. Derived class
		 * implements stateful encoding should override this. Default implementation does nothing.
		 * @return The encoder
		 * @see #resetEncodingState
		 */
		Encoder& Encoder::resetDecodingState() BOOST_NOEXCEPT {
			return *this;
		}

		/**
		 * Resets the intermediate conversion state for @c #fromUnicode to the default. Derived class
		 * implements stateful encoding should override this. Default implementation does nothing.
		 * @return The encoder
		 * @see #resetDecodingState
		 */
		Encoder& Encoder::resetEncodingState() BOOST_NOEXCEPT {
			return *this;
		}

		/**
		 * Sets the new miscellaneous options.
		 * @param newOptions The options to set. See @c #OptionBits.
		 * @return This encoder
		 */
		Encoder& Encoder::setOptions(const std::bitset<NUMBER_OF_OPTIONS> newOptions) BOOST_NOEXCEPT {
			options_ = newOptions;
			return *this;
		}

		/**
		 * Sets the conversion policy.
		 * @param newPolicy The new policy
		 * @return This encoder
		 * @throw UnknownValueException<SubstitutionPolicy> @a newPolicy is invalid
		 */
		Encoder& Encoder::setSubstitutionPolicy(SubstitutionPolicy newPolicy) {
			if(newPolicy < DONT_SUBSTITUTE || newPolicy > IGNORE_UNMAPPABLE_CHARACTERS)
				throw UnknownValueException("newPolicy");
			substitutionPolicy_ = newPolicy;
			return *this;
		}

		/**
		 * Converts the given string from the native encoding into UTF-16.
		 * @param[out] to The beginning of the destination buffer
		 * @param[out] toEnd The end of the destination buffer
		 * @param[out] toNext Points the first unaltered character in the destination buffer after the conversion
		 * @param[in] from The beginning of the buffer to be converted
		 * @param[in] fromEnd The end of the buffer to be converted
		 * @param[in] fromNext Points to the first unconverted character after the conversion
		 * @return The result of the conversion
		 */
		Encoder::ConversionResult Encoder::toUnicode(Char* to, Char* toEnd,
				Char*& toNext, const Byte* from, const Byte* fromEnd, const Byte*& fromNext) {
			if(to == nullptr || toEnd == nullptr || from == nullptr || fromEnd == nullptr)
				throw NullPointerException("");
			else if(to > toEnd)
				throw std::invalid_argument("to > toEnd");
			else if(from > fromEnd)
				throw std::invalid_argument("from > fromEnd");
			toNext = nullptr;
			fromNext = nullptr;
			return doToUnicode(to, toEnd, toNext, from, fromEnd, fromNext);
		}

		/**
		 * Converts the given string from the native encoding into UTF-16.
		 * @param from The string to be converted
		 * @return The converted string or an empty if encountered unconvertible character
		 */
		String Encoder::toUnicode(const boost::string_ref& from) {
			std::size_t chars = properties().maximumUCSLength() * from.length();
			std::unique_ptr<Char[]> temp(new Char[chars]);
			const Byte* fromNext;
			Char* toNext;
			ConversionResult result;
			while(true) {
				result = toUnicode(temp.get(), temp.get() + chars, toNext,
					reinterpret_cast<const Byte*>(from.data()), reinterpret_cast<const Byte*>(from.data()) + from.length(), fromNext);
				if(result == COMPLETED)
					break;
				else if(result == INSUFFICIENT_BUFFER)
					temp.reset(new Char[chars *= 2]);
				else
					return String();
			}
			return String(temp.get(), toNext);
		}
	}
}
