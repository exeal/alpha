/**
 * @file encoder.cpp
 * @author exeal
 * @date 2004-2006
 */

#include "encoder.hpp"
#include <algorithm>
using namespace ascension;
using namespace ascension::encoding;
using namespace ascension::encoding::implementation;
using namespace std;


// UnsupportedEncodingException /////////////////////////////////////////////

/**
 * Constructor.
 * @param mib the MIBenum vaule of the encoding
 */
UnsupportedEncodingException::UnsupportedEncodingException(MIBenum mib) : invalid_argument("unsupported encoding."), mib_(mib) {
}

/// Returns the MIBenum value of the encoding
MIBenum UnsupportedEncodingException::mibEnum() const throw() {
	return mib_;
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
 * Ascension provides @c Encoder classes implement the encodings in three groups:
 * <dl>
 *   <dt>@c fundamental</dt>
 *   <dd>The most fundamental encodings including US-ASCII, ISO-8859-1, UTF-8 and UTF-16.</dd>
 *   <dt>@c standard</dt>
 *   <dd>Major encodings including: most of ISO-8859-x, KOI-8, most of Windows-125x, ... These are
 *     not available if @c ASCENSION_NO_STANDARD_ENCODINGS symbol is defined.</dd>
 *   <dt>@c extended</dt>
 *   <dd>Other minorities. Not available if @c ASCENSION_NO_EXTENDED_ENCODINGS symbol is defined.</dd>
 * </dl>
 *
 * <del>In addition, the encodings supported by the system are available if the target is Win32.</del>
 *
 * @note This class is not compatible with C++ standard @c std#codecvt template class.
 *
 * <h3>Making user-defined @c Encoder classes</h3>
 *
 * You can create and add your own @c Encoder class.
 *
 * <h3>Important protocol of @c #fromUnicode and @c #toUnicode</h3>
 */

/// Protected default constructor.
Encoder::Encoder() throw() : policy_(NO_POLICY) {
}

/// Destructor.
Encoder::~Encoder() throw() {
}

/**
 * Returns true if the given character can be fully encoded with this encoding.
 * @param c the code point of the character
 * @return succeeded or not
 * @throw std#invalid_argument @a c is not a Unicode scalar value
 */
bool Encoder::canEncode(CodePoint c) const {
	if(!text::isScalarValue(c))
		throw invalid_argument("the code point is not a scalar value.");
	Char temp[2];
	return canEncode(temp, temp + text::surrogates::encode(c, temp));
}

/**
 * Returns true if the given string can be fully encoded with this encoding.
 * @param first the beginning of the string
 * @param last the end of the string
 * @return succeeded or not
 */
bool Encoder::canEncode(const Char* first, const Char* last) const {
	if(first == 0)
		throw NullPointerException("first");
	else if(last == 0)
		throw NullPointerException("last");
	else if(first > last)
		throw invalid_argument("first > last");
	// TODO: Should be able to implement without heap/free store...
	const size_t bytes = (last - first) * properties().maximumNativeBytes();
	manah::AutoBuffer<byte> temp(new byte[bytes]);
	const Char* fromNext;
	byte* toNext;
	return fromUnicode(temp.get(), temp.get() + bytes, toNext, first, last, fromNext) == COMPLETED;
}


/**
 * Returns true if the given string can be fully encoded with this encoding.
 * @param s the string
 * @return succeeded or not
 */
bool Encoder::canEncode(const String& s) const {
	return canEncode(s.data(), s.data() + s.length());
}

EncoderFactory* Encoder::find(MIBenum mib) throw() {
	if(mib > MIB_UNKNOWN) {
		for(vector<EncoderFactory*>::iterator i(registry().begin()), e(registry().end()); i != e; ++i) {
			if((*i)->mibEnum() == mib)
				return *i;
		}
	}
	return 0;
}

EncoderFactory* Encoder::find(const string& name) throw() {
	for(vector<EncoderFactory*>::iterator i(registry().begin()), e(registry().end()); i != e; ++i) {
		// test canonical name
		const string canonicalName((*i)->name());
		if(matchEncodingNames(name.begin(), name.end(), canonicalName.begin(), canonicalName.end()))
			return *i;
		// test aliases
		const string aliases((*i)->aliases());
		for(size_t j = 0; ; ++j) {
			size_t delimiter = aliases.find('|', j);
			if(delimiter == string::npos)
				delimiter = aliases.length();
			if(delimiter != j) {
				if(matchEncodingNames(name.begin(), name.end(), aliases.begin() + j, aliases.begin() + delimiter))
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
 * @param id the identifier obtained by @c #availableNames method
 * @return the encoder or @c null if not registered
 * @see #availableNames
 */
auto_ptr<Encoder> Encoder::forID(size_t id) throw() {
	return (id < registry().size()) ? registry()[id]->create() : auto_ptr<Encoder>(0);
}

/**
 * Returns the encoder which has the given MIBenum value.
 * @param mib the MIBenum value
 * @return the encoder or @c null if not registered
 */
auto_ptr<Encoder> Encoder::forMIB(MIBenum mib) throw() {
	EncoderFactory* const factory = find(mib);
	return (factory != 0) ? factory->create() : auto_ptr<Encoder>(0);
}

/**
 * Returns the encoder which matches the given name.
 * @param name the name
 * @return the encoder or @c null if not registered
 */
auto_ptr<Encoder> Encoder::forName(const string& name) throw() {
	EncoderFactory* const factory = find(name);
	return (factory != 0) ? factory->create() : auto_ptr<Encoder>(0);
}

#ifdef ASCENSION_WINDOWS
/**
 * Returns the encoder which has the given Win32 code page.
 * @param codePage the code page
 * @return the encoder or @c null if not registered
 */
auto_ptr<Encoder> Encoder::forWindowsCodePage(uint codePage) throw() {
	// TODO: not implemented.
	return auto_ptr<Encoder>(0);
}
#endif /* ASCENSION_WINDOWS */

/**
 * Converts the given string from UTF-16 into the native encoding.
 * @param[out] to the beginning of the destination buffer
 * @param[out] toEnd the end of the destination buffer
 * @param[out] toNext points the first unaltered character in the destination buffer after the conversion
 * @param[in] from the beginning of the buffer to be converted
 * @param[in] fromEnd the end of the buffer to be converted
 * @param[in] fromNext points to the first unconverted character after the conversion
 * @param[in,out] state the conversion state. can be @c null
 * @return the result of the conversion
 */
Encoder::Result Encoder::fromUnicode(byte* to, byte* toEnd, byte*& toNext,
		const Char* from, const Char* fromEnd, const Char*& fromNext, State* state /* = 0*/) const {
	if(to == 0 || toEnd == 0 || from == 0 || fromEnd == 0)
		throw NullPointerException("");
	else if(to > toEnd)
		throw invalid_argument("to > toEnd");
	else if(from > fromEnd)
		throw invalid_argument("from > fromEnd");
	toNext = 0;
	fromNext = 0;
	return doFromUnicode(to, toEnd, toNext, from, fromEnd, fromNext, state);
}

/**
 * Converts the given string from UTF-16 into the native encoding.
 * @param from the string to be converted
 * @return the converted string or an empty if encountered unconvertible character
 */
string Encoder::fromUnicode(const String& from) const {
	size_t bytes = properties().maximumNativeBytes() * from.length();
	manah::AutoBuffer<byte> temp(new byte[bytes]);
	const Char* fromNext;
	byte* toNext;
	Result result;
	while(true) {
		result = fromUnicode(temp.get(), temp.get() + bytes, toNext, from.data(), from.data() + from.length(), fromNext);
		if(result == COMPLETED)
			break;
		else if(result == INSUFFICIENT_BUFFER)
			temp.reset(new byte[bytes *= 2]);
		else
			return "";
	}
	return string(temp.get(), toNext);
}

/// Returns the default encoder.
Encoder& Encoder::getDefault() throw() {
//#ifdef ASCENSION_WINDOWS
//	return convertWin32CPtoMIB(::GetACP());
//#else
	static auto_ptr<Encoder> instance(forMIB(fundamental::UTF_8));
	return *instance;
//#endif /* ASCENSION_WINDOWS */
}

/**
 * Registers the new encoder factory.
 * @param newFactory the encoder factory
 */
void Encoder::registerFactory(EncoderFactory& newFactory) {
	registry().push_back(&newFactory);
}

vector<EncoderFactory*>& Encoder::registry() {
	static vector<EncoderFactory*> singleton;
	return singleton;
}

/**
 * Sets the conversion policy.
 * @param newPolicy the new policy
 * @return this encoder
 * @throw std#invalid_argument @a newPolicy is invalid
 */
Encoder& Encoder::setPolicy(Policy newPolicy) {
	if(newPolicy < NO_POLICY || newPolicy > IGNORE_UNMAPPABLE_CHARACTER)
		throw invalid_argument("the given policy is not supported.");
	policy_ = newPolicy;
	return *this;
}

/// Returns true if supports the encoding has the given MIBenum value.
bool Encoder::supports(MIBenum mib) throw() {
	return find(mib) != 0;
}

/// Returns true if supports the encoding has to the given name or alias.
bool Encoder::supports(const string& name) throw() {
	return find(name) != 0;
}

/**
 * Converts the given string from the native encoding into UTF-16.
 * @param[out] to the beginning of the destination buffer
 * @param[out] toEnd the end of the destination buffer
 * @param[out] toNext points the first unaltered character in the destination buffer after the conversion
 * @param[in] from the beginning of the buffer to be converted
 * @param[in] fromEnd the end of the buffer to be converted
 * @param[in] fromNext points to the first unconverted character after the conversion
 * @param[in,out] state the conversion state. can be @c null
 * @return the result of the conversion
 */
Encoder::Result Encoder::toUnicode(Char* to, Char* toEnd, Char*& toNext,
		const byte* from, const byte* fromEnd, const byte*& fromNext, State* state /* = 0*/) const {
	if(to == 0 || toEnd == 0 || from == 0 || fromEnd == 0)
		throw NullPointerException("");
	else if(to > toEnd)
		throw invalid_argument("to > toEnd");
	else if(from > fromEnd)
		throw invalid_argument("from > fromEnd");
	toNext = 0;
	fromNext = 0;
	return doToUnicode(to, toEnd, toNext, from, fromEnd, fromNext, state);
}

/**
 * Converts the given string from the native encoding into UTF-16.
 * @param from the string to be converted
 * @return the converted string or an empty if encountered unconvertible character
 */
String Encoder::toUnicode(const string& from) const {
	size_t chars = properties().maximumUCSLength() * from.length();
	manah::AutoBuffer<Char> temp(new Char[chars]);
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
			return L"";
	}
	return String(temp.get(), toNext);
}


// EncodingDetector /////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param name the name of the encoding detector
 * @throw std#invalid_argument @a id is invalid
 */
EncodingDetector::EncodingDetector(const string& name) : name_(name) {
}

/// Destructor.
EncodingDetector::~EncodingDetector() throw() {
}

/**
 * Detects the encoding of the string buffer.
 * @param detectorID the identifier of the encoding detector to use
 * @param first the beginning of the sequence
 * @param last the end of the sequence
 * @param[out] convertibleBytes the number of bytes (from @a first) absolutely detected. the value
 * can't exceed the result of (@a last - @a first). can be @c null if not needed
 * @return the MIBenum of the detected encoding
 * @throw NullPointerException @a first or @last is @c null
 * @throw std#invalid_argument @c first is greater than @a last
 */
MIBenum EncodingDetector::detect(const byte* first, const byte* last, ptrdiff_t* convertibleBytes) const {
	if(first == 0 || last == 0)
		throw NullPointerException("first or last");
	else if(first > last)
		throw invalid_argument("first > last");
	return doDetect(first, last, convertibleBytes);
}

/**
 * Returns the encoding detector which matches the given name.
 * @param name the name
 * @return the encoding detector or @c null if not registered
 */
EncodingDetector* EncodingDetector::forName(const string& name) throw() {
	for(vector<EncodingDetector*>::iterator i(registry().begin()), e(registry().end()); i != e; ++i) {
		const string canonicalName((*i)->name());
		if(matchEncodingNames(name.begin(), name.end(), canonicalName.begin(), canonicalName.end()))
			return *i;
	}
	return 0;
}

#ifdef ASCENSION_WINDOWS
/**
 * Returns the encoding detector which has the given Windows code page.
 * @param codePage the code page
 * @return the encoding detector or @c null if not registered
 */
EncodingDetector* EncodingDetector::forWindowsCodePage(::UINT codePage) throw() {
	switch(codePage) {
	case 50001:	return forName("UniversalAutoDetect");
	case 50932:	return forName("JISAutoDetect");
	case 50949:	return forName("KSAutoDetect");
	default:	return 0;
	}
}
#endif /* ASCENSION_WINDOWS */

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
 * @param newDetector the encoding detector
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
		MIBenum	doDetect(const byte* first, const byte* last, ptrdiff_t* convertibleBytes) const throw();
	};
//	ASCENSION_DEFINE_ENCODING_DETECTOR(SystemLocaleBasedDetector, "SystemLocaleAutoDetect");
//	ASCENSION_DEFINE_ENCODING_DETECTOR(UserLocaleBasedDetector, "UserLocaleAutoDetect");
} // namespace @0

/// @see EncodingDetector#doDetect
MIBenum UniversalDetector::doDetect(const byte* first, const byte* last, ptrdiff_t* convertibleBytes) const throw() {
	// try all detectors
	vector<string> names;
	availableNames(back_inserter(names));

	MIBenum result = Encoder::getDefault().properties().mibEnum();
	ptrdiff_t bestScore = 0, score;
	for(vector<string>::const_iterator name(names.begin()), e(names.end()); name != e; ++name) {
		if(const EncodingDetector* detector = forName(*name)) {
			const MIBenum detectedEncoding = detector->detect(first, last, &score);
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
		virtual ~BasicLatinEncoderFactory() throw() {}
		auto_ptr<Encoder> create() const throw() {return auto_ptr<Encoder>(new InternalEncoder(mask_, *this));}
	private:
		class InternalEncoder : public Encoder {
		public:
			InternalEncoder(ulong mask, const IEncodingProperties& properties) throw() : mask_(mask), props_(properties) {}
		private:
			Result doFromUnicode(byte* to, byte* toEnd, byte*& toNext,
				const Char* from, const Char* fromEnd, const Char*& fromNext, State* state) const;
			Result doToUnicode(Char* to, Char* toEnd, Char*& toNext,
				const byte* from, const byte* fromEnd, const byte*& fromNext, State* state) const;
			const IEncodingProperties&	properties() const throw() {return props_;}
		private:
			const ulong mask_;
			const IEncodingProperties& props_;
		};
	private:
		const ulong mask_;
	};

	BasicLatinEncoderFactory US_ASCII("US-ASCII", fundamental::US_ASCII, "",
			"ANSI_X3.4-1968|iso-ir-6|ANSI_X3.4-1986|ISO_646.irv:1991|ASCII|ISO646-US|us|IBM367|cp367"
			"\0csASCII|iso_646.irv:1983|ascii7|646|windows-20127|ibm-367", 0x7F);
	BasicLatinEncoderFactory ISO_8859_1("ISO-8859-1", fundamental::ISO_8859_1, "Western (ISO 8859-1)",
			"iso-ir-100|ISO_8859-1|latin1|l1|IBM819|CP819|csISOLatin1" "\0ibm-819|8859_1|819", 0xFF);

	struct Installer {
		Installer() throw() {
			Encoder::registerFactory(US_ASCII);
			Encoder::registerFactory(ISO_8859_1);
		}
	} unused;

	Encoder::Result BasicLatinEncoderFactory::InternalEncoder::doFromUnicode(
			byte* to, byte* toEnd, byte*& toNext, const Char* from, const Char* fromEnd, const Char*& fromNext, State*) const {
		for(; to < toEnd && from < fromEnd; ++to, ++from) {
			if((*from & ~mask_) != 0) {
				if(policy() == IGNORE_UNMAPPABLE_CHARACTER)
					--to;
				else if(policy() == REPLACE_UNMAPPABLE_CHARACTER)
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
			Char* to, Char* toEnd, Char*& toNext, const byte* from, const byte* fromEnd, const byte*& fromNext, State*) const {
		for(; to < toEnd && from < fromEnd; ++to, ++from) {
			if((*from & ~mask_) != 0) {
				if(policy() == IGNORE_UNMAPPABLE_CHARACTER)
					--to;
				else if(policy() == REPLACE_UNMAPPABLE_CHARACTER)
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
 * @param name the name returned by @c #name
 * @param mib the MIBenum value returned by @c #mibEnum
 * @param displayName the display name returned by @c #displayName
 * @param maximumNativeBytes the value returned by @c #maximumNativeBytes
 * @param maximumUCSLength the value returned by @c #maximumUCSLength
 * @param aliases the encoding aliases returned by @c #aliases
 */
EncoderFactoryBase::EncoderFactoryBase(const string& name, MIBenum mib,
		const string& displayName /* = "" */,
		size_t maximumNativeBytes /* = 1 */, size_t maximumUCSLength /* = 1 */,
		const string& aliases /* = "" */, byte substitutionCharacter /* = 0x1A */)
		: name_(name), displayName_(displayName.empty() ? name : displayName), aliases_(aliases),
		maximumNativeBytes_(maximumNativeBytes), maximumUCSLength_(maximumUCSLength),
		mib_(mib), substitutionCharacter_(substitutionCharacter) {
}

/// Destructor.
EncoderFactoryBase::~EncoderFactoryBase() throw() {
}

/// @see IEncodingProperties#aliases
string EncoderFactoryBase::aliases() const throw() {
	return aliases_;
}

/// @see IEncodingProperties#displayName
string EncoderFactoryBase::displayName(const locale&) const throw() {
	return displayName_;
}

/// @see IEncodingProperties#maximumNativeBytes
size_t EncoderFactoryBase::maximumNativeBytes() const throw() {
	return maximumNativeBytes_;
}

/// @see IEncodingProperties#maximumUCSLength
size_t EncoderFactoryBase::maximumUCSLength() const throw() {
	return maximumUCSLength_;
}

/// @see IEncodingProperties#mibEnum
MIBenum EncoderFactoryBase::mibEnum() const throw() {
	return mib_;
}

/// @see IEncodingProperties#name
string EncoderFactoryBase::name() const throw() {
	return name_;
}

/// @see IEncodingProperties#substitutionCharacter
byte EncoderFactoryBase::substitutionCharacter() const throw() {
	return substitutionCharacter_;
}


// implementation.sbcs.SingleByteEncoderFactory /////////////////////////////

namespace {
	class SingleByteEncoder : public Encoder {
	public:
		explicit SingleByteEncoder(const sbcs::ByteMap& table, const IEncodingProperties& properties) throw();
		~SingleByteEncoder() throw();
	private:
		void	buildUnicodeToNativeTable();
		// Encoder
		Result	doFromUnicode(byte* to, byte* toEnd, byte*& toNext,
			const Char* from, const Char* fromEnd, const Char*& fromNext, State* state) const;
		Result	doToUnicode(Char* to, Char* toEnd, Char*& toNext,
			const byte* from, const byte* fromEnd, const byte*& fromNext, State* state) const;
		const IEncodingProperties&	properties() const throw() {return props_;}
	private:
		const sbcs::ByteMap& nativeToUnicode_;
		const IEncodingProperties& props_;
		byte* unicodeToNative_[0x100];
		static const byte UNMAPPABLE_16x16_UNICODE_TABLE[0x100];
	};

	const byte SingleByteEncoder::UNMAPPABLE_16x16_UNICODE_TABLE[0x100] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	};

	SingleByteEncoder::SingleByteEncoder(const sbcs::ByteMap& table, const IEncodingProperties& properties) throw() : nativeToUnicode_(table), props_(properties) {
		fill_n(unicodeToNative_, countof(unicodeToNative_), static_cast<byte*>(0));
	}
	
	SingleByteEncoder::~SingleByteEncoder() throw() {
		for(size_t i = 0; i < countof(unicodeToNative_); ++i) {
			if(unicodeToNative_[i] != UNMAPPABLE_16x16_UNICODE_TABLE)
				delete[] unicodeToNative_[i];
		}
	}

	void SingleByteEncoder::buildUnicodeToNativeTable() {
		assert(unicodeToNative_[0] == 0);
		fill_n(unicodeToNative_, countof(unicodeToNative_), const_cast<byte*>(UNMAPPABLE_16x16_UNICODE_TABLE));
		unicodeToNative_[0] = new byte[0x100];
		for(int i = 0x00; i < 0xFF; ++i) {
			const Char ucs = nativeToUnicode_[i];
			byte*& p = unicodeToNative_[ucs >> 8];
			if(p == 0) {
				p = new byte[0x100];
				fill_n(p, 0x100, sbcs::UNMAPPABLE_BYTE);
			}
			p[mask8Bit(ucs)] = static_cast<byte>(i);
		}
	}

	Encoder::Result SingleByteEncoder::doFromUnicode(byte* to, byte* toEnd, byte*& toNext,
			const Char* from, const Char* fromEnd, const Char*& fromNext, State*) const {
		if(unicodeToNative_[0] == 0)
			const_cast<SingleByteEncoder*>(this)->buildUnicodeToNativeTable();
		for(; to < toEnd && from < fromEnd; ++to, ++from) {
			*to = unicodeToNative_[*from >> 8][mask8Bit(*from)];
			if(*to == sbcs::UNMAPPABLE_BYTE && *from != sbcs::UNMAPPABLE_BYTE) {
				if(policy() == IGNORE_UNMAPPABLE_CHARACTER)
					--to;
				else if(policy() == REPLACE_UNMAPPABLE_CHARACTER)
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

	Encoder::Result SingleByteEncoder::doToUnicode(Char* to, Char* toEnd, Char*& toNext,
			const byte* from, const byte* fromEnd, const byte*& fromNext, State*) const {
		for(; to < toEnd && from < fromEnd; ++to, ++from) {
			*to = nativeToUnicode_[*from];
			if(*to == REPLACEMENT_CHARACTER) {
				if(policy() == IGNORE_UNMAPPABLE_CHARACTER)
					--to;
				else if(policy() != REPLACE_UNMAPPABLE_CHARACTER) {
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

auto_ptr<Encoder> sbcs::internal::createSingleByteEncoder(const sbcs::ByteMap& table, const IEncodingProperties& properties) throw() {
	return auto_ptr<Encoder>(new SingleByteEncoder(table, properties));
}
