/**
 * @file encoder.cpp
 * @author exeal
 * @date 2004-2014
 */

#include <ascension/corelib/encoder.hpp>
#include <ascension/corelib/basic-exceptions.hpp>
#include <ascension/corelib/text/utf.hpp>	// text.isScalarValue, text.utf.encode
#include <algorithm>
#include <memory>							// std.unique_ptr
#include <boost/foreach.hpp>


namespace ascension {
	namespace encoding {
		/**
		 * Converts the given encoding name from Unicode into 7-bit US-ASCII can pass to other functions.
		 * @return The converted encoding name
		 * @throw std#bad_alloc Out of memory
		 * @throw UnsupportedEncodingException @a source can't convert
		 */
		std::string encodingNameFromUnicode(const String& source) {
			const std::unique_ptr<Encoder> encoder(Encoder::forMIB(fundamental::US_ASCII));
			const std::string temp(encoder->fromUnicode(source));
			if(temp.empty())
				throw UnsupportedEncodingException("invalid encoding name character");
			return temp;
		}


		// UnsupportedEncodingException ///////////////////////////////////////////////////////////////////////////////
		
		/**
		 * Constructor.
		 * @param message The message string
		 */
		UnsupportedEncodingException::UnsupportedEncodingException(const std::string& message) : std::invalid_argument(message) {
		}


		// Encoder ////////////////////////////////////////////////////////////////////////////////////////////////////
		
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
		 * @c #SubstitutionPolicy#DONT_SUBSTITUTE.
		 *
		 * If you use @c #SubstitutionPolicy#DONT_SUBSTITUTE, the encoder returns @c UNMAPPABLE_CHARACTER when
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

		/// Character separates the string returns @c #aliases.
		const char Encoder::ALIASES_SEPARATOR = '|';

		/// Protected default constructor.
		Encoder::Encoder() BOOST_NOEXCEPT : substitutionPolicy_(DONT_SUBSTITUTE), flags_(BEGINNING_OF_BUFFER | END_OF_BUFFER) {
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
//#ifdef ASCENSION_OS_WINDOWS
//			return convertWin32CPtoMIB(::GetACP());
//#else
			static std::unique_ptr<Encoder> instance(forMIB(fundamental::UTF_8));
			return *instance;
//#endif // ASCENSION_OS_WINDOWS
		}

		std::shared_ptr<const EncoderFactory> Encoder::find(MIBenum mib) BOOST_NOEXCEPT {
			if(mib > MIB_UNKNOWN) {
				BOOST_FOREACH(std::shared_ptr<const EncoderFactory> factory, registry()) {
					if(factory->mibEnum() == mib)
						return factory;
				}
			}
			return std::shared_ptr<const EncoderFactory>();
		}

		std::shared_ptr<const EncoderFactory> Encoder::find(const std::string& name) BOOST_NOEXCEPT {
			BOOST_FOREACH(std::shared_ptr<const EncoderFactory> factory, registry()) {
				// test canonical name
				const std::string canonicalName(factory->name());
				if(compareEncodingNames(std::begin(name), std::end(name), std::begin(canonicalName), std::end(canonicalName)) == 0)
					return factory;
				// test aliases
				const std::string aliases(factory->aliases());
				for(std::size_t i = 0; ; ++i) {
					std::size_t delimiter = aliases.find(ALIASES_SEPARATOR, i);
					if(delimiter == std::string::npos)
						delimiter = aliases.length();
					if(delimiter != i) {
						if(compareEncodingNames(std::begin(name), std::end(name), std::begin(aliases) + i, std::begin(aliases) + delimiter) == 0)
							return factory;
						else if(delimiter < aliases.length())
							++delimiter;
					}
					if(delimiter == aliases.length())
						break;
					i = delimiter;
				}
			}
			return std::shared_ptr<const EncoderFactory>();
		}

		/**
		 * Returns the encoder which has the given enumeration identifier.
		 * @param id The identifier obtained by @c #availableNames method
		 * @return The encoder or @c null if not registered
		 * @see #availableNames
		 */
		std::unique_ptr<Encoder> Encoder::forID(std::size_t id) BOOST_NOEXCEPT {
			return (id < registry().size()) ? registry()[id]->create() : std::unique_ptr<Encoder>();
		}

		/**
		 * Returns the encoder which has the given MIBenum value.
		 * @param mib The MIBenum value
		 * @return The encoder or @c null if not registered
		 */
		std::unique_ptr<Encoder> Encoder::forMIB(MIBenum mib) BOOST_NOEXCEPT {
			const std::shared_ptr<const EncoderFactory> factory(find(mib));
			return (factory.get() != nullptr) ? factory->create() : std::unique_ptr<Encoder>();
		}

		/**
		 * Returns the encoder which matches the given name.
		 * @param name The name
		 * @return The encoder or @c null if not registered
		 */
		std::unique_ptr<Encoder> Encoder::forName(const std::string& name) BOOST_NOEXCEPT {
			const std::shared_ptr<const EncoderFactory> factory(find(name));
			return (factory.get() != nullptr) ? factory->create() : std::unique_ptr<Encoder>();
		}

#ifdef ASCENSION_OS_WINDOWS
		/**
		 * Returns the encoder which has the given Win32 code page.
		 * @param codePage The code page
		 * @return The encoder or @c null if not registered
		 */
		std::unique_ptr<Encoder> Encoder::forWindowsCodePage(unsigned int codePage) BOOST_NOEXCEPT {
			// TODO: not implemented.
			return std::unique_ptr<Encoder>();
		}
#endif // ASCENSION_OS_WINDOWS

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
		Encoder::Result Encoder::fromUnicode(Byte* to, Byte* toEnd,
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
		std::string Encoder::fromUnicode(const String& from) {
			std::size_t bytes = properties().maximumNativeBytes() * from.length();
			std::unique_ptr<Byte[]> temp(new Byte[bytes]);
			const Char* fromNext;
			Byte* toNext;
			Result result;
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
		 * Registers the new encoder factory.
		 * @param newFactory The encoder factory
		 */
		void Encoder::registerFactory(std::shared_ptr<const EncoderFactory> newFactory) {
			registry().push_back(newFactory);
		}

		std::vector<std::shared_ptr<const EncoderFactory>>& Encoder::registry() {
			static std::vector<std::shared_ptr<const EncoderFactory>> singleton;
			return singleton;
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
		 * Sets the new miscellaneous flags.
		 * @param newFlags The flags to set. See @c #Flag.
		 * @return This encoder
		 * @throw The encoder
		 * @throw UnknownValueException<Flag> @a newFlags includes unknown value
		 */
		Encoder& Encoder::setFlags(int newFlags) {
			if((newFlags & ~(BEGINNING_OF_BUFFER | END_OF_BUFFER | UNICODE_BYTE_ORDER_MARK)) != 0)
				throw UnknownValueException("newFlags");
			flags_ = newFlags;
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

		/// Returns @c true if supports the encoding has the given MIBenum value.
		bool Encoder::supports(MIBenum mib) BOOST_NOEXCEPT {
			return find(mib) != nullptr;
		}

		/// Returns @c true if supports the encoding has to the given name or alias.
		bool Encoder::supports(const std::string& name) BOOST_NOEXCEPT {
			return find(name) != nullptr;
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
		Encoder::Result Encoder::toUnicode(Char* to, Char* toEnd,
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
		String Encoder::toUnicode(const std::string& from) {
			std::size_t chars = properties().maximumUCSLength() * from.length();
			std::unique_ptr<Char[]> temp(new Char[chars]);
			const Byte* fromNext;
			Char* toNext;
			Result result;
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


		// EncodingDetector ///////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * Constructor.
		 * @param name The name of the encoding detector
		 * @throw std#invalid_argument @a name is invalid
		 */
		EncodingDetector::EncodingDetector(const std::string& name) : name_(name) {
		}

		/// Destructor.
		EncodingDetector::~EncodingDetector() BOOST_NOEXCEPT {
		}

		/**
		 * Detects the encoding of the string buffer.
		 * @param detectorID The identifier of the encoding detector to use
		 * @param first The beginning of the sequence
		 * @param last The end of the sequence
		 * @param[out] convertibleBytes The number of bytes (from @a first) absolutely detected. The value can't exceed
		 *                              the result of (@a last - @a first). Can be @c null if not needed
		 * @return The MIBenum and the name of the detected encoding
		 * @throw NullPointerException @a first or @last is @c null
		 * @throw std#invalid_argument @c first is greater than @a last
		 */
		std::pair<MIBenum, std::string> EncodingDetector::detect(const Byte* first, const Byte* last, std::ptrdiff_t* convertibleBytes) const {
			if(first == nullptr || last == nullptr)
				throw NullPointerException("first or last");
			else if(first > last)
				throw std::invalid_argument("first > last");
			return doDetect(first, last, convertibleBytes);
		}

		/**
		 * Returns the encoding detector which matches the given name.
		 * @param name The name
		 * @return The encoding detector or @c null if not registered
		 */
		std::shared_ptr<const EncodingDetector> EncodingDetector::forName(const std::string& name) BOOST_NOEXCEPT {
			BOOST_FOREACH(const std::shared_ptr<const EncodingDetector>detector, registry()) {
				const std::string canonicalName(detector->name());
				if(compareEncodingNames(std::begin(name), std::end(name), std::begin(canonicalName), std::end(canonicalName)) == 0)
					return detector;
			}
			return nullptr;
		}

#ifdef ASCENSION_OS_WINDOWS
		/**
		 * Returns the encoding detector which has the given Windows code page.
		 * @param codePage The code page
		 * @return The encoding detector or @c null if not registered
		 */
		std::shared_ptr<const EncodingDetector> EncodingDetector::forWindowsCodePage(unsigned int codePage) BOOST_NOEXCEPT {
			switch(codePage) {
				case 50001:
					return forName("UniversalAutoDetect");
				case 50932:
					return forName("JISAutoDetect");
				case 50949:
					return forName("KSAutoDetect");
				default:
					return std::shared_ptr<const EncodingDetector>();
			}
		}
#endif // ASCENSION_OS_WINDOWS

		std::vector<std::shared_ptr<const EncodingDetector>>& EncodingDetector::registry() {
			static std::vector<std::shared_ptr<const EncodingDetector>> singleton;
			return singleton;
		}

		/**
		 * Registers the new encoding detector.
		 * @param newDetector The encoding detector
		 * @throw NullPointerException @a detector is @c null
		 */
		void EncodingDetector::registerDetector(std::shared_ptr<const EncodingDetector> newDetector) {
			if(newDetector.get() == nullptr)
				throw NullPointerException("newDetector");
			registry().push_back(newDetector);
		}


		// UniversalDetector //////////////////////////////////////////////////////////////////////////////////////////

		namespace {
			class UniversalDetector : public EncodingDetector {
			public:
				UniversalDetector() : EncodingDetector("UniversalAutoDetect") {}
			private:
				std::pair<MIBenum, std::string> doDetect(const Byte* first, const Byte* last, std::ptrdiff_t* convertibleBytes) const BOOST_NOEXCEPT;
			};
//			ASCENSION_DEFINE_ENCODING_DETECTOR(SystemLocaleBasedDetector, "SystemLocaleAutoDetect");
//			ASCENSION_DEFINE_ENCODING_DETECTOR(UserLocaleBasedDetector, "UserLocaleAutoDetect");
		} // namespace @0

		/// @see EncodingDetector#doDetect
		std::pair<MIBenum, std::string> UniversalDetector::doDetect(const Byte* first, const Byte* last, std::ptrdiff_t* convertibleBytes) const BOOST_NOEXCEPT {
			// try all detectors
			std::vector<std::string> names;
			availableNames(std::back_inserter(names));

			std::pair<MIBenum, std::string> result = std::make_pair(
				Encoder::defaultInstance().properties().mibEnum(), Encoder::defaultInstance().properties().name());
			std::ptrdiff_t bestScore = 0, score;
			BOOST_FOREACH(const std::string& name, names) {
				if(const std::shared_ptr<const EncodingDetector> detector = forName(name)) {
					if(detector.get() == this)
						continue;
					const std::pair<MIBenum, std::string> detectedEncoding(detector->detect(first, last, &score));
					if(score > bestScore) {
						result = detectedEncoding;
						if(score == last - first)
							break;
						bestScore = score;
					}
				}
			}

			if(convertibleBytes != 0)
				*convertibleBytes = bestScore;
			return result;
		}


		// US-ASCII and ISO-8859-1 ////////////////////////////////////////////////////////////////////////////////////

		namespace {
			class BasicLatinEncoderFactory : public implementation::EncoderFactoryBase {
			public:
				BasicLatinEncoderFactory(const std::string& name, MIBenum mib, const std::string& displayName,
					const std::string& aliases, std::uint32_t mask) : implementation::EncoderFactoryBase(name, mib, displayName, 1, 1, aliases), mask_(mask) {}
				virtual ~BasicLatinEncoderFactory() BOOST_NOEXCEPT {}
				std::unique_ptr<Encoder> create() const BOOST_NOEXCEPT {
					return std::unique_ptr<Encoder>(new InternalEncoder(mask_, *this));
				}
			private:
				class InternalEncoder : public Encoder {
				public:
					InternalEncoder(std::uint32_t mask, const EncodingProperties& properties) BOOST_NOEXCEPT : mask_(mask), props_(properties) {}
				private:
					Result doFromUnicode(Byte* to, Byte* toEnd, Byte*& toNext,
						const Char* from, const Char* fromEnd, const Char*& fromNext);
					Result doToUnicode(Char* to, Char* toEnd, Char*& toNext,
						const Byte* from, const Byte* fromEnd, const Byte*& fromNext);
					const EncodingProperties& properties() const BOOST_NOEXCEPT {return props_;}
				private:
					const std::uint32_t mask_;
					const EncodingProperties& props_;
				};
			private:
				const std::uint32_t mask_;
			};

			struct Installer {
				Installer() BOOST_NOEXCEPT {
					Encoder::registerFactory(std::make_shared<BasicLatinEncoderFactory>(
						"US-ASCII", fundamental::US_ASCII, "",
						"ANSI_X3.4-1968|iso-ir-6|ANSI_X3.4-1986|ISO_646.irv:1991|ASCII|ISO646-US|us|IBM367|cp367"
						"\0csASCII|iso_646.irv:1983|ascii7|646|windows-20127|ibm-367", 0x7f));
					Encoder::registerFactory(std::make_shared<BasicLatinEncoderFactory>(
						"ISO-8859-1", fundamental::ISO_8859_1, "Western (ISO 8859-1)",
						"iso-ir-100|ISO_8859-1|latin1|l1|IBM819|CP819|csISOLatin1" "\0ibm-819|8859_1|819", 0xff));
					EncodingDetector::registerDetector(std::make_shared<UniversalDetector>());
				}
			} unused;

			Encoder::Result BasicLatinEncoderFactory::InternalEncoder::doFromUnicode(
					Byte* to, Byte* toEnd, Byte*& toNext, const Char* from, const Char* fromEnd, const Char*& fromNext) {
				for(; to < toEnd && from < fromEnd; ++to, ++from) {
					if((*from & ~mask_) != 0) {
						if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTERS)
							--to;
						else if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTERS)
							*to = properties().substitutionCharacter();
						else {
							toNext = to;
							fromNext = from;
							return UNMAPPABLE_CHARACTER;
						}
					} else
						*to = mask8Bit(*from);
				}
				toNext = to;
				fromNext = from;
				return (fromNext == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
			}

			Encoder::Result BasicLatinEncoderFactory::InternalEncoder::doToUnicode(
					Char* to, Char* toEnd, Char*& toNext, const Byte* from, const Byte* fromEnd, const Byte*& fromNext) {
				for(; to < toEnd && from < fromEnd; ++to, ++from) {
					if((*from & ~mask_) != 0) {
						if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTERS)
							--to;
						else if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTERS)
							*to = text::REPLACEMENT_CHARACTER;
						else {
							toNext = to;
							fromNext = from;
							return UNMAPPABLE_CHARACTER;
						}
					} else
						*to = *from;
				}
				toNext = to;
				fromNext = from;
				return (fromNext == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
			}
		} // namespace @0

		namespace implementation {
			// implementation.EncoderFactoryBase //////////////////////////////////////////////////////////////////////

			/**
			 * Constructor.
			 * @param name The name returned by @c #name
			 * @param mib The MIBenum value returned by @c #mibEnum
			 * @param displayName The display name returned by @c #displayName
			 * @param maximumNativeBytes The value returned by @c #maximumNativeBytes
			 * @param maximumUCSLength The value returned by @c #maximumUCSLength
			 * @param aliases The encoding aliases returned by @c #aliases
			 */
			EncoderFactoryBase::EncoderFactoryBase(const std::string& name, MIBenum mib,
					const std::string& displayName /* = "" */,
					std::size_t maximumNativeBytes /* = 1 */, std::size_t maximumUCSLength /* = 1 */,
					const std::string& aliases /* = "" */, Byte substitutionCharacter /* = 0x1a */)
					: name_(name), displayName_(displayName.empty() ? name : displayName), aliases_(aliases),
					maximumNativeBytes_(maximumNativeBytes), maximumUCSLength_(maximumUCSLength),
					mib_(mib), substitutionCharacter_(substitutionCharacter) {
			}

			/// Destructor.
			EncoderFactoryBase::~EncoderFactoryBase() BOOST_NOEXCEPT {
			}

			/// @see EncodingProperties#aliases
			std::string EncoderFactoryBase::aliases() const BOOST_NOEXCEPT {
				return aliases_;
			}

			/// @see EncodingProperties#displayName
			std::string EncoderFactoryBase::displayName(const std::locale&) const BOOST_NOEXCEPT {
				return displayName_;
			}

			/// @see EncodingProperties#maximumNativeBytes
			std::size_t EncoderFactoryBase::maximumNativeBytes() const BOOST_NOEXCEPT {
				return maximumNativeBytes_;
			}

			/// @see EncodingProperties#maximumUCSLength
			std::size_t EncoderFactoryBase::maximumUCSLength() const BOOST_NOEXCEPT {
				return maximumUCSLength_;
			}

			/// @see EncodingProperties#mibEnum
			MIBenum EncoderFactoryBase::mibEnum() const BOOST_NOEXCEPT {
				return mib_;
			}

			/// @see EncodingProperties#name
			std::string EncoderFactoryBase::name() const BOOST_NOEXCEPT {
				return name_;
			}

			/// @see EncodingProperties#substitutionCharacter
			Byte EncoderFactoryBase::substitutionCharacter() const BOOST_NOEXCEPT {
				return substitutionCharacter_;
			}

			namespace sbcs {
				// implementation.sbcs.BidirectionalMap ///////////////////////////////////////////////////////////////

				const std::array<const Byte, 0x100> BidirectionalMap::UNMAPPABLE_16x16_UNICODE_TABLE = {
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				};

				/**
				 * Constructor.
				 * @param byteToCharacterWire The table defines byte-to-character mapping consists of 16Å~16-characters
				 */
				BidirectionalMap::BidirectionalMap(const Char** byteToCharacterWire) BOOST_NOEXCEPT : byteToUnicode_(byteToCharacterWire) {
					unicodeToByte_.fill(nullptr);
					buildUnicodeToByteTable();	// eager?
				}

				/// Destructor.
				BidirectionalMap::~BidirectionalMap() BOOST_NOEXCEPT {
					for(std::size_t i = 0; i < ASCENSION_COUNTOF(unicodeToByte_); ++i) {
						if(unicodeToByte_[i] != UNMAPPABLE_16x16_UNICODE_TABLE.data())
							delete[] unicodeToByte_[i];
					}
				}

				void sbcs::BidirectionalMap::buildUnicodeToByteTable() {
					assert(unicodeToByte_[0] == 0);
					unicodeToByte_.fill(const_cast<Byte*>(UNMAPPABLE_16x16_UNICODE_TABLE.data()));
					for(int i = 0x00; i < 0xff; ++i) {
						const Char ucs = wireAt(byteToUnicode_, static_cast<Byte>(i));
						Byte*& p = unicodeToByte_[ucs >> 8];
						if(p == UNMAPPABLE_16x16_UNICODE_TABLE.data()) {
							p = new Byte[0x100];
							std::fill_n(p, 0x100, UNMAPPABLE_BYTE);
						}
						p[mask8Bit(ucs)] = static_cast<Byte>(i);
					}
				}


				// implementation.sbcs.SingleByteEncoderFactory ///////////////////////////////////////////////////////

				namespace {
					class SingleByteEncoder : public Encoder {
					public:
						explicit SingleByteEncoder(const Char** byteToCharacterWire, const EncodingProperties& properties) BOOST_NOEXCEPT;
					private:
						// Encoder
						Result doFromUnicode(Byte* to, Byte* toEnd, Byte*& toNext,
							const Char* from, const Char* fromEnd, const Char*& fromNext);
						Result doToUnicode(Char* to, Char* toEnd, Char*& toNext,
							const Byte* from, const Byte* fromEnd, const Byte*& fromNext);
						const EncodingProperties& properties() const BOOST_NOEXCEPT {return props_;}
					private:
						const sbcs::BidirectionalMap table_;
						const EncodingProperties& props_;
					};

					SingleByteEncoder::SingleByteEncoder(const Char** byteToCharacterWire,
							const EncodingProperties& properties) BOOST_NOEXCEPT : table_(byteToCharacterWire), props_(properties) {
					}

					Encoder::Result SingleByteEncoder::doFromUnicode(Byte* to, Byte* toEnd,
							Byte*& toNext, const Char* from, const Char* fromEnd, const Char*& fromNext) {
						for(; to < toEnd && from < fromEnd; ++to, ++from) {
							*to = table_.toByte(*from);
							if(*to == sbcs::UNMAPPABLE_BYTE && *from != sbcs::UNMAPPABLE_BYTE) {
								if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTERS)
									--to;
								else if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTERS)
									*to = properties().substitutionCharacter();
								else {
									toNext = to;
									fromNext = from;
									return UNMAPPABLE_CHARACTER;
								}
							}
						}
						toNext = to;
						fromNext = from;
						return (fromNext == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
					}

					Encoder::Result SingleByteEncoder::doToUnicode(Char* to, Char* toEnd,
							Char*& toNext, const Byte* from, const Byte* fromEnd, const Byte*& fromNext) {
						for(; to < toEnd && from < fromEnd; ++to, ++from) {
							*to = table_.toCharacter(*from);
							if(*to == text::REPLACEMENT_CHARACTER) {
								if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTERS)
									--to;
								else if(substitutionPolicy() != REPLACE_UNMAPPABLE_CHARACTERS) {
									toNext = to;
									fromNext = from;
									return UNMAPPABLE_CHARACTER;
								}
							}
						}
						toNext = to;
						fromNext = from;
						return (fromNext == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
					}
				} // namespace @0
			}
		}
	}

	namespace detail {
		std::unique_ptr<encoding::Encoder> createSingleByteEncoder(
				const Char** byteToCharacterWire, const encoding::EncodingProperties& properties) BOOST_NOEXCEPT {
			return std::unique_ptr<encoding::Encoder>(new encoding::implementation::sbcs::SingleByteEncoder(byteToCharacterWire, properties));
		}
	}
}
