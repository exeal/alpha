/**
 * @file encoder.cpp
 * @author exeal
 * @date 2004-2011
 */

#include <ascension/corelib/encoder.hpp>
#include <ascension/corelib/basic-exceptions.hpp>
#include <algorithm>
using namespace ascension;
using namespace ascension::encoding;
using namespace ascension::encoding::implementation;
using namespace std;


// free function ////////////////////////////////////////////////////////////

/**
 * Converts the given encoding name from Unicode into 7-bit US-ASCII can pass to other functions.
 * @return The converted encoding name
 * @throw std#bad_alloc Out of memory
 * @throw UnsupportedEncodingException @a source can't convert
 */
string encoding::encodingNameFromUnicode(const String& source) {
	const auto_ptr<Encoder> encoder(Encoder::forMIB(fundamental::US_ASCII));
	const string temp(encoder->fromUnicode(source));
	if(temp.empty())
		throw UnsupportedEncodingException("invalid encoding name character");
	return temp;
}


// UnsupportedEncodingException /////////////////////////////////////////////

/**
 * Constructor.
 * @param message The message string
 */
UnsupportedEncodingException::UnsupportedEncodingException(const string& message) : invalid_argument(message) {
}


// Encoder //////////////////////////////////////////////////////////////////

/**
 * @class ascension::encoding::Encoder
 * @c Encoder class provides conversions between text encodings.
 *
 * Ascension uses Unicode to store and manipulate strings. However, different encodings are
 * preferred in many cases. Ascension provides a collection of @c Encoder classes to help with
 * converting non-Unicode formats to/from Unicode.
 *
 * You can convert a native encoded string into Unicode by using @c #toUnicode method.
 *
 * @code
 * const std::string native(...);
 * std::auto_ptr<Encoder> encoder(Encoder::forName("Shift_JIS"));
 * if(encoder->get() != 0) {
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
 * When an encoder encounters unmappable byte/character, its behavior depends on the substitution
 * policy. Available policies are enumerated by @c #SubstitutionPolicy. You can get the current
 * substitution policy by @c #substitutionPolicy, and can set by @c #setSubstitutionPolicy. Default
 * policy is @c #SubstitutionPolicy#DONT_SUBSTITUTE.
 *
 * If you use @c #SubstitutionPolicy#DONT_SUBSTITUTE, the encoder returns @c UNMAPPABLE_CHARACTER
 * when encountered an unmappable byte/character. In this case, @a fromNext parameter of the
 * conversion method will address the unmappable byte/character.
 *
 * Substitution policy can't handle malformed input. See the next section.
 *
 * <h3>Malformed Input</h3>
 *
 * When an encoder encounters illegal byte/character sequence (malformed input), returns
 * @c MALFORMED_INPUT. This behavior can't be changed by the caller. In this case, @a fromNext
 * parameter of the conversion method will address the unmappable byte/character.
 *
 * <h3>Intermediate Conversion State of Encoder</h3>
 *
 * An encoder may have the own conversion state to implement stateful encoding. For streaming
 * operation (ex. receiving data over a network), the conversion methods of @c Encoder accept a
 * part of the entire input. An encoder keeps its own conversion state between the several
 * invocations of the conversion methods. The following illustrates the case of ISO-2022-JP:
 *
 * @code
 * std::auto_ptr<Encoder> encoder = Encoder::forName("ISO-2022-JP");
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
Encoder::Encoder() /*throw()*/ : substitutionPolicy_(DONT_SUBSTITUTE), flags_(BEGINNING_OF_BUFFER | END_OF_BUFFER) {
}

/// Destructor.
Encoder::~Encoder() /*throw()*/ {
}

/**
 * Returns @c true if the given character can be fully encoded with this encoding. This calls
 * @c #resetEncodingState method.
 * @param c The code point of the character
 * @return Succeeded or not
 * @throw std#invalid_argument @a c is not a Unicode scalar value
 */
bool Encoder::canEncode(CodePoint c) {
	if(!text::isScalarValue(c))
		throw invalid_argument("the code point is not a scalar value.");
	Char temp[2];
	return canEncode(StringPiece(temp, temp + text::surrogates::encode(c, temp)));
}

/**
 * Returns @c true if the given string can be fully encoded with this encoding. This calls
 * @c #resetEncodingState method.
 * @param s The string
 * @return Succeeded or not
 */
bool Encoder::canEncode(const StringPiece& s) {
	if(s.beginning() == 0)
		throw NullPointerException("s.beginning()");
	else if(s.end() == 0)
		throw NullPointerException("s.end()");
	// TODO: Should be able to implement without heap/free store...
	const size_t bytes = s.length() * properties().maximumNativeBytes();
	AutoBuffer<byte> temp(new byte[bytes]);
	const Char* fromNext;
	byte* toNext;
	resetEncodingState();
	return fromUnicode(temp.get(), temp.get() + bytes, toNext, s.beginning(), s.end(), fromNext) == COMPLETED;
}

/// Returns the default encoder.
Encoder& Encoder::defaultInstance() /*throw()*/ {
//#ifdef ASCENSION_WINDOWS
//	return convertWin32CPtoMIB(::GetACP());
//#else
	static auto_ptr<Encoder> instance(forMIB(fundamental::UTF_8));
	return *instance;
//#endif /* ASCENSION_WINDOWS */
}

EncoderFactory* Encoder::find(MIBenum mib) /*throw()*/ {
	if(mib > MIB_UNKNOWN) {
		for(vector<EncoderFactory*>::iterator i(registry().begin()), e(registry().end()); i != e; ++i) {
			if((*i)->mibEnum() == mib)
				return *i;
		}
	}
	return 0;
}

EncoderFactory* Encoder::find(const string& name) /*throw()*/ {
	for(vector<EncoderFactory*>::iterator i(registry().begin()), e(registry().end()); i != e; ++i) {
		// test canonical name
		const string canonicalName((*i)->name());
		if(compareEncodingNames(name.begin(), name.end(), canonicalName.begin(), canonicalName.end()) == 0)
			return *i;
		// test aliases
		const string aliases((*i)->aliases());
		for(size_t j = 0; ; ++j) {
			size_t delimiter = aliases.find(ALIASES_SEPARATOR, j);
			if(delimiter == string::npos)
				delimiter = aliases.length();
			if(delimiter != j) {
				if(compareEncodingNames(name.begin(), name.end(), aliases.begin() + j, aliases.begin() + delimiter) == 0)
					return *i;
				else if(delimiter < aliases.length())
					++delimiter;
			}
			if(delimiter == aliases.length())
				break;
			j = delimiter;
		}
	}
	return 0;
}

/**
 * Returns the encoder which has the given enumeration identifier.
 * @param id The identifier obtained by @c #availableNames method
 * @return The encoder or @c null if not registered
 * @see #availableNames
 */
auto_ptr<Encoder> Encoder::forID(size_t id) /*throw()*/ {
	return (id < registry().size()) ? registry()[id]->create() : auto_ptr<Encoder>(0);
}

/**
 * Returns the encoder which has the given MIBenum value.
 * @param mib The MIBenum value
 * @return The encoder or @c null if not registered
 */
auto_ptr<Encoder> Encoder::forMIB(MIBenum mib) /*throw()*/ {
	EncoderFactory* const factory = find(mib);
	return (factory != 0) ? factory->create() : auto_ptr<Encoder>(0);
}

/**
 * Returns the encoder which matches the given name.
 * @param name The name
 * @return The encoder or @c null if not registered
 */
auto_ptr<Encoder> Encoder::forName(const string& name) /*throw()*/ {
	EncoderFactory* const factory = find(name);
	return (factory != 0) ? factory->create() : auto_ptr<Encoder>(0);
}

#ifdef ASCENSION_WINDOWS
/**
 * Returns the encoder which has the given Win32 code page.
 * @param codePage The code page
 * @return The encoder or @c null if not registered
 */
auto_ptr<Encoder> Encoder::forWindowsCodePage(uint codePage) /*throw()*/ {
	// TODO: not implemented.
	return auto_ptr<Encoder>(0);
}
#endif // ASCENSION_WINDOWS

/**
 * Converts the given string from UTF-16 into the native encoding.
 * @param[out] to The beginning of the destination buffer
 * @param[out] toEnd The end of the destination buffer
 * @param[out] toNext Points the first unaltered character in the destination buffer after the
 *                    conversion
 * @param[in] from The beginning of the buffer to be converted
 * @param[in] fromEnd The end of the buffer to be converted
 * @param[in] fromNext Points to the first unconverted character after the conversion
 * @return The result of the conversion
 * @throw NullPointerException @a to, @a toEnd, @a from and/or @a fromEnd is @c null
 * @throw std#invalid_argument @a to &gt; @a toEnd or @a from &gt; @a fromEnd
 */
Encoder::Result Encoder::fromUnicode(byte* to, byte* toEnd,
		byte*& toNext, const Char* from, const Char* fromEnd, const Char*& fromNext) {
	if(to == 0 || toEnd == 0 || from == 0 || fromEnd == 0)
		throw NullPointerException("");
	else if(to > toEnd)
		throw invalid_argument("to > toEnd");
	else if(from > fromEnd)
		throw invalid_argument("from > fromEnd");
	toNext = 0;
	fromNext = 0;
	return doFromUnicode(to, toEnd, toNext, from, fromEnd, fromNext);
}

/**
 * Converts the given string from UTF-16 into the native encoding. This calls
 * @c #resetEncodingState method.
 * @param from The string to be converted
 * @return The converted string or an empty if encountered unconvertible character
 * @throw std#bad_alloc
 */
string Encoder::fromUnicode(const String& from) {
	size_t bytes = properties().maximumNativeBytes() * from.length();
	AutoBuffer<byte> temp(new byte[bytes]);
	const Char* fromNext;
	byte* toNext;
	Result result;
	resetEncodingState();
	while(true) {
		result = fromUnicode(temp.get(), temp.get() + bytes, toNext, from.data(), from.data() + from.length(), fromNext);
		if(result == COMPLETED)
			break;
		else if(result == INSUFFICIENT_BUFFER) {
			temp.reset(new(nothrow) byte[bytes *= 2]);
			if(temp.get() == 0)
				throw bad_alloc();
		} else
			return "";
	}
	return string(temp.get(), toNext);
}

/**
 * Registers the new encoder factory.
 * @param newFactory The encoder factory
 */
void Encoder::registerFactory(EncoderFactory& newFactory) {
	registry().push_back(&newFactory);
}

vector<EncoderFactory*>& Encoder::registry() {
	static vector<EncoderFactory*> singleton;
	return singleton;
}

/**
 * Resets the intermediate conversion state for @c #toUnicode to the default. Derived class
 * implements stateful encoding should override this. Default implementation does nothing.
 * @return The encoder
 * @see #resetEncodingState
 */
Encoder& Encoder::resetDecodingState() /*throw()*/ {
	return *this;
}

/**
 * Resets the intermediate conversion state for @c #fromUnicode to the default. Derived class
 * implements stateful encoding should override this. Default implementation does nothing.
 * @return The encoder
 * @see #resetDecodingState
 */
Encoder& Encoder::resetEncodingState() /*throw()*/ {
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
bool Encoder::supports(MIBenum mib) /*throw()*/ {
	return find(mib) != 0;
}

/// Returns @c true if supports the encoding has to the given name or alias.
bool Encoder::supports(const string& name) /*throw()*/ {
	return find(name) != 0;
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
		Char*& toNext, const byte* from, const byte* fromEnd, const byte*& fromNext) {
	if(to == 0 || toEnd == 0 || from == 0 || fromEnd == 0)
		throw NullPointerException("");
	else if(to > toEnd)
		throw invalid_argument("to > toEnd");
	else if(from > fromEnd)
		throw invalid_argument("from > fromEnd");
	toNext = 0;
	fromNext = 0;
	return doToUnicode(to, toEnd, toNext, from, fromEnd, fromNext);
}

/**
 * Converts the given string from the native encoding into UTF-16.
 * @param from The string to be converted
 * @return The converted string or an empty if encountered unconvertible character
 */
String Encoder::toUnicode(const string& from) {
	size_t chars = properties().maximumUCSLength() * from.length();
	AutoBuffer<Char> temp(new Char[chars]);
	const byte* fromNext;
	Char* toNext;
	Result result;
	while(true) {
		result = toUnicode(temp.get(), temp.get() + chars, toNext,
			reinterpret_cast<const byte*>(from.data()), reinterpret_cast<const byte*>(from.data()) + from.length(), fromNext);
		if(result == COMPLETED)
			break;
		else if(result == INSUFFICIENT_BUFFER)
			temp.reset(new Char[chars *= 2]);
		else
			return String();
	}
	return String(temp.get(), toNext);
}


// EncodingDetector /////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param name The name of the encoding detector
 * @throw std#invalid_argument @a name is invalid
 */
EncodingDetector::EncodingDetector(const string& name) : name_(name) {
}

/// Destructor.
EncodingDetector::~EncodingDetector() /*throw()*/ {
}

/**
 * Detects the encoding of the string buffer.
 * @param detectorID The identifier of the encoding detector to use
 * @param first The beginning of the sequence
 * @param last The end of the sequence
 * @param[out] convertibleBytes The number of bytes (from @a first) absolutely detected. The value
 *             can't exceed the result of (@a last - @a first). Can be @c null if not needed
 * @return The MIBenum and the name of the detected encoding
 * @throw NullPointerException @a first or @last is @c null
 * @throw std#invalid_argument @c first is greater than @a last
 */
pair<MIBenum, string> EncodingDetector::detect(const byte* first, const byte* last, ptrdiff_t* convertibleBytes) const {
	if(first == 0 || last == 0)
		throw NullPointerException("first or last");
	else if(first > last)
		throw invalid_argument("first > last");
	return doDetect(first, last, convertibleBytes);
}

/**
 * Returns the encoding detector which matches the given name.
 * @param name The name
 * @return The encoding detector or @c null if not registered
 */
EncodingDetector* EncodingDetector::forName(const string& name) /*throw()*/ {
	for(vector<EncodingDetector*>::iterator i(registry().begin()), e(registry().end()); i != e; ++i) {
		const string canonicalName((*i)->name());
		if(compareEncodingNames(name.begin(), name.end(), canonicalName.begin(), canonicalName.end()) == 0)
			return *i;
	}
	return 0;
}

#ifdef ASCENSION_WINDOWS
/**
 * Returns the encoding detector which has the given Windows code page.
 * @param codePage The code page
 * @return The encoding detector or @c null if not registered
 */
EncodingDetector* EncodingDetector::forWindowsCodePage(UINT codePage) /*throw()*/ {
	switch(codePage) {
	case 50001:	return forName("UniversalAutoDetect");
	case 50932:	return forName("JISAutoDetect");
	case 50949:	return forName("KSAutoDetect");
	default:	return 0;
	}
}
#endif // ASCENSION_WINDOWS

vector<EncodingDetector*>& EncodingDetector::registry() {
	static struct Registry {
		~Registry() {
			for(vector<EncodingDetector*>::iterator i(registry.begin()), e(registry.end()); i != e; ++i)
				delete *i;
		}
		vector<EncodingDetector*> registry;
	} singleton;
	return singleton.registry;
}

/**
 * Registers the new encoding detector.
 * @param newDetector The encoding detector
 * @throw NullPointerException @a detector is @c null
 */
void EncodingDetector::registerDetector(auto_ptr<EncodingDetector> newDetector) {
	if(newDetector.get() == 0)
		throw NullPointerException("newDetector");
	registry().push_back(newDetector.release());
}


// UniversalDetector ////////////////////////////////////////////////////////

namespace {
	class UniversalDetector : public EncodingDetector {
	public:
		UniversalDetector() : EncodingDetector("UniversalAutoDetect") {}
	private:
		pair<MIBenum, string> doDetect(const byte* first, const byte* last, ptrdiff_t* convertibleBytes) const /*throw()*/;
	};
//	ASCENSION_DEFINE_ENCODING_DETECTOR(SystemLocaleBasedDetector, "SystemLocaleAutoDetect");
//	ASCENSION_DEFINE_ENCODING_DETECTOR(UserLocaleBasedDetector, "UserLocaleAutoDetect");
} // namespace @0

/// @see EncodingDetector#doDetect
pair<MIBenum, string> UniversalDetector::doDetect(const byte* first, const byte* last, ptrdiff_t* convertibleBytes) const /*throw()*/ {
	// try all detectors
	vector<string> names;
	availableNames(back_inserter(names));

	pair<MIBenum, string> result = make_pair(
		Encoder::defaultInstance().properties().mibEnum(), Encoder::defaultInstance().properties().name());
	ptrdiff_t bestScore = 0, score;
	for(vector<string>::const_iterator name(names.begin()), e(names.end()); name != e; ++name) {
		if(const EncodingDetector* detector = forName(*name)) {
			if(detector == this)
				continue;
			const pair<MIBenum, string> detectedEncoding(detector->detect(first, last, &score));
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


// US-ASCII and ISO-8859-1 //////////////////////////////////////////////////

namespace {
	class BasicLatinEncoderFactory : public EncoderFactoryBase {
	public:
		BasicLatinEncoderFactory(const string& name, MIBenum mib, const string& displayName,
			const string& aliases, ulong mask) : EncoderFactoryBase(name, mib, displayName, 1, 1, aliases), mask_(mask) {}
		virtual ~BasicLatinEncoderFactory() /*throw()*/ {}
		auto_ptr<Encoder> create() const /*throw()*/ {return auto_ptr<Encoder>(new InternalEncoder(mask_, *this));}
	private:
		class InternalEncoder : public Encoder {
		public:
			InternalEncoder(ulong mask, const EncodingProperties& properties) /*throw()*/ : mask_(mask), props_(properties) {}
		private:
			Result doFromUnicode(byte* to, byte* toEnd, byte*& toNext,
				const Char* from, const Char* fromEnd, const Char*& fromNext);
			Result doToUnicode(Char* to, Char* toEnd, Char*& toNext,
				const byte* from, const byte* fromEnd, const byte*& fromNext);
			const EncodingProperties& properties() const /*throw()*/ {return props_;}
		private:
			const ulong mask_;
			const EncodingProperties& props_;
		};
	private:
		const ulong mask_;
	};

	BasicLatinEncoderFactory US_ASCII("US-ASCII", fundamental::US_ASCII, "",
			"ANSI_X3.4-1968|iso-ir-6|ANSI_X3.4-1986|ISO_646.irv:1991|ASCII|ISO646-US|us|IBM367|cp367"
			"\0csASCII|iso_646.irv:1983|ascii7|646|windows-20127|ibm-367", 0x7f);
	BasicLatinEncoderFactory ISO_8859_1("ISO-8859-1", fundamental::ISO_8859_1, "Western (ISO 8859-1)",
			"iso-ir-100|ISO_8859-1|latin1|l1|IBM819|CP819|csISOLatin1" "\0ibm-819|8859_1|819", 0xff);

	struct Installer {
		Installer() /*throw()*/ {
			Encoder::registerFactory(US_ASCII);
			Encoder::registerFactory(ISO_8859_1);
			EncodingDetector::registerDetector(auto_ptr<EncodingDetector>(new UniversalDetector));
		}
	} unused;

	Encoder::Result BasicLatinEncoderFactory::InternalEncoder::doFromUnicode(
			byte* to, byte* toEnd, byte*& toNext, const Char* from, const Char* fromEnd, const Char*& fromNext) {
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
			Char* to, Char* toEnd, Char*& toNext, const byte* from, const byte* fromEnd, const byte*& fromNext) {
		for(; to < toEnd && from < fromEnd; ++to, ++from) {
			if((*from & ~mask_) != 0) {
				if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTERS)
					--to;
				else if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTERS)
					*to = REPLACEMENT_CHARACTER;
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


// implementation.EncoderFactoryBase ////////////////////////////////////////

/**
 * Constructor.
 * @param name The name returned by @c #name
 * @param mib The MIBenum value returned by @c #mibEnum
 * @param displayName The display name returned by @c #displayName
 * @param maximumNativeBytes The value returned by @c #maximumNativeBytes
 * @param maximumUCSLength The value returned by @c #maximumUCSLength
 * @param aliases The encoding aliases returned by @c #aliases
 */
EncoderFactoryBase::EncoderFactoryBase(const string& name, MIBenum mib,
		const string& displayName /* = "" */,
		size_t maximumNativeBytes /* = 1 */, size_t maximumUCSLength /* = 1 */,
		const string& aliases /* = "" */, byte substitutionCharacter /* = 0x1a */)
		: name_(name), displayName_(displayName.empty() ? name : displayName), aliases_(aliases),
		maximumNativeBytes_(maximumNativeBytes), maximumUCSLength_(maximumUCSLength),
		mib_(mib), substitutionCharacter_(substitutionCharacter) {
}

/// Destructor.
EncoderFactoryBase::~EncoderFactoryBase() /*throw()*/ {
}

/// @see EncodingProperties#aliases
string EncoderFactoryBase::aliases() const /*throw()*/ {
	return aliases_;
}

/// @see EncodingProperties#displayName
string EncoderFactoryBase::displayName(const locale&) const /*throw()*/ {
	return displayName_;
}

/// @see EncodingProperties#maximumNativeBytes
size_t EncoderFactoryBase::maximumNativeBytes() const /*throw()*/ {
	return maximumNativeBytes_;
}

/// @see EncodingProperties#maximumUCSLength
size_t EncoderFactoryBase::maximumUCSLength() const /*throw()*/ {
	return maximumUCSLength_;
}

/// @see EncodingProperties#mibEnum
MIBenum EncoderFactoryBase::mibEnum() const /*throw()*/ {
	return mib_;
}

/// @see EncodingProperties#name
string EncoderFactoryBase::name() const /*throw()*/ {
	return name_;
}

/// @see EncodingProperties#substitutionCharacter
byte EncoderFactoryBase::substitutionCharacter() const /*throw()*/ {
	return substitutionCharacter_;
}


// implementation.sbcs.BidirectionalMap /////////////////////////////////////

const byte sbcs::BidirectionalMap::UNMAPPABLE_16x16_UNICODE_TABLE[0x100] = {
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
 * @param byteToCharacterWire The table defines byte-to-character mapping consists of
 *                            16~16-characters
 */
sbcs::BidirectionalMap::BidirectionalMap(const Char** byteToCharacterWire) /*throw()*/ : byteToUnicode_(byteToCharacterWire) {
	fill_n(unicodeToByte_, ASCENSION_COUNTOF(unicodeToByte_), static_cast<byte*>(0));
	buildUnicodeToByteTable();	// eager?
}

/// Destructor.
sbcs::BidirectionalMap::~BidirectionalMap() /*throw()*/ {
	for(size_t i = 0; i < ASCENSION_COUNTOF(unicodeToByte_); ++i) {
		if(unicodeToByte_[i] != UNMAPPABLE_16x16_UNICODE_TABLE)
			delete[] unicodeToByte_[i];
	}
}

void sbcs::BidirectionalMap::buildUnicodeToByteTable() {
	assert(unicodeToByte_[0] == 0);
	fill_n(unicodeToByte_, ASCENSION_COUNTOF(unicodeToByte_), const_cast<byte*>(UNMAPPABLE_16x16_UNICODE_TABLE));
	for(int i = 0x00; i < 0xff; ++i) {
		const Char ucs = wireAt(byteToUnicode_, static_cast<byte>(i));
		byte*& p = unicodeToByte_[ucs >> 8];
		if(p == UNMAPPABLE_16x16_UNICODE_TABLE) {
			p = new byte[0x100];
			fill_n(p, 0x100, UNMAPPABLE_BYTE);
		}
		p[mask8Bit(ucs)] = static_cast<byte>(i);
	}
}


// implementation.sbcs.SingleByteEncoderFactory /////////////////////////////

namespace {
	class SingleByteEncoder : public Encoder {
	public:
		explicit SingleByteEncoder(const Char** byteToCharacterWire, const EncodingProperties& properties) /*throw()*/;
	private:
		// Encoder
		Result doFromUnicode(byte* to, byte* toEnd, byte*& toNext,
			const Char* from, const Char* fromEnd, const Char*& fromNext);
		Result doToUnicode(Char* to, Char* toEnd, Char*& toNext,
			const byte* from, const byte* fromEnd, const byte*& fromNext);
		const EncodingProperties& properties() const /*throw()*/ {return props_;}
	private:
		const sbcs::BidirectionalMap table_;
		const EncodingProperties& props_;
	};

	SingleByteEncoder::SingleByteEncoder(const Char** byteToCharacterWire,
			const EncodingProperties& properties) /*throw()*/ : table_(byteToCharacterWire), props_(properties) {
	}

	Encoder::Result SingleByteEncoder::doFromUnicode(byte* to, byte* toEnd,
			byte*& toNext, const Char* from, const Char* fromEnd, const Char*& fromNext) {
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
			Char*& toNext, const byte* from, const byte* fromEnd, const byte*& fromNext) {
		for(; to < toEnd && from < fromEnd; ++to, ++from) {
			*to = table_.toCharacter(*from);
			if(*to == REPLACEMENT_CHARACTER) {
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

auto_ptr<Encoder> detail::createSingleByteEncoder(
		const Char** byteToCharacterWire, const EncodingProperties& properties) /*throw()*/ {
	return auto_ptr<Encoder>(new SingleByteEncoder(byteToCharacterWire, properties));
}
