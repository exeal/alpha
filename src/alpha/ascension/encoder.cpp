/**
 * @file encoder.cpp
 * @author exeal
 * @date 2004-2006
 */

#include "encoder.hpp"
#include <algorithm>
#include <vector>
#ifdef ASCENSION_WINDOWS
#	include <windows.h>	// GetCPInfoExW, ...
#	ifndef interface
#		define interface struct
#	endif
#include <mlang.h>
#include "../../manah/com/common.hpp"
#endif /* ASCENSION_WINDOWS */
using namespace ascension;
using namespace ascension::encoding;
using namespace ascension::encoding::implementation;
using namespace std;


namespace {
	template<typename Element> struct Registry {
		~Registry() {
			for(map<MIBenum, Element*>::iterator i(registry.begin()), e(registry.end()); i != e; ++i)
				delete i->second;
		}
		map<MIBenum, Element*> registry;
	};
} // namespace @0


/// Returns the human-readable name of the encoding.
String encoding::getEncodingDisplayName(MIBenum mib) {/*
#ifdef ASCENSION_WINDOWS
	if(const uint cp = convertMIBtoWinCP(mib)) {
		manah::com::ComPtr<::IMultiLanguage> mlang;
		HRESULT hr = mlang.createInstance(::CLSID_CMultiLanguage, ::IID_IMultiLanguage, CLSCTX_INPROC);
		if(SUCCEEDED(hr)) {
			::MIMECPINFO mcpi;
			if(SUCCEEDED(hr = mlang->GetCodePageInfo(cp, &mcpi)))
				return mcpi.wszDescription;
		}
	}
#endif /* ASCENSION_WINDOWS */
	if(const Encoder* encoder = Encoder::forMIB(mib)) {
		const string name(encoder->name());
		String s(name.length(), L'x');
		copy(name.begin(), name.end(), s.begin());
		return s;
	} else if(const EncodingDetector* detector = EncodingDetector::forID(mib)) {
		const string name(detector->name());
		String s(name.length(), L'x');
		copy(name.begin(), name.end(), s.begin());
		return s;
	}
	return L"";
}


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
 * In addition, the encodings supported by the system are available if the target is Win32.
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
	const size_t bytes = (last - first) * maximumNativeBytes();
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

/**
 * Returns the encoder which has the given MIBenum value.
 * @param mib the MIBenum value
 * @return the encoder or @c null if not registered
 */
Encoder* Encoder::forMIB(MIBenum mib) throw() {
	map<MIBenum, Encoder*>::iterator i(registry().find(mib));
	return (i != registry().end()) ? i->second : 0;
}

/**
 * Returns the encoder which matches the given name.
 * @param name the name
 * @return the encoder or @c null if not registered
 */
Encoder* Encoder::forName(const string& name) throw() {
	for(map<MIBenum, Encoder*>::iterator i(registry().begin()), e(registry().end()); i != e; ++i) {
		// test canonical name
		const string canonicalName = i->second->name();
		if(matchEncodingNames(name.begin(), name.end(), canonicalName.begin(), canonicalName.end()))
			return i->second;
		// test aliases
		const string aliases = i->second->aliases();
		for(size_t j = 0; ; ++j) {
			size_t nul = aliases.find('\0', j);
			if(nul == string::npos)
				nul = aliases.length();
			if(nul != j) {
				if(matchEncodingNames(name.begin(), name.end(), aliases.begin() + j, aliases.begin() + nul))
					return i->second;
				++nul;
			}
			if(nul == aliases.length())
				break;
			j = nul;
		}
	}
	return 0;
}

#ifdef ASCENSION_WINDOWS
/**
 * Returns the encoder which has the given Win32 code page.
 * @param codePage the code page
 * @return the encoder or @c null if not registered
 */
Encoder* Encoder::forWindowsCodePage(::UINT codePage) throw() {
	// TODO: not implemented.
	return 0;
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
	size_t bytes = maximumNativeBytes() * from.length();
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

/// Returns the MIBenum value of the default encoding.
MIBenum Encoder::getDefault() throw() {
//#ifdef _WIN32
//	return convertWin32CPtoMIB(::GetACP());
//#else
	return fundamental::UTF_8;
//#endif /* _WIN32 */
}

/**
 * Registers the new encoder.
 * @param producer the function produces an encoder
 * @throw NullPointerException @a producer is @c null
 * @throw std#invalid_argument @a mib is already registered
 */
void Encoder::registerEncoder(std::auto_ptr<Encoder> encoder) {
	if(encoder.get() == 0)
		throw NullPointerException("encoder");
	else if(registry().find(encoder->mibEnum()) != registry().end())
//		throw invalid_argument("the encoder is already registered.");
		return;
	registry().insert(make_pair(encoder->mibEnum(), encoder.release()));
}

map<MIBenum, Encoder*>& Encoder::registry() {
	static Registry<Encoder> singleton;
	return singleton.registry;
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

/// Returns true if supports the specified encoding.
bool Encoder::supports(MIBenum mib) throw() {
	return registry().find(mib) != registry().end();
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
	size_t chars = maximumUCSLength() * from.length();
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
 * @param id the identifier of the encoding detector
 * @param name the name of the encoding detector
 * @throw std#invalid_argument @a id is invalid
 */
EncodingDetector::EncodingDetector(MIBenum id, const string& name) : id_(id), name_(name) {
	if(id < MINIMUM_ID || id > MAXIMUM_ID)
		throw invalid_argument("id");
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
 * Returns the encoding detector which has the given identifier.
 * @param id the identifier
 * @return the encoding detectir or @c null if not registered
 */
EncodingDetector* EncodingDetector::forID(MIBenum id) throw() {
	map<MIBenum, EncodingDetector*>::iterator i(registry().find(id));
	return (i != registry().end()) ? i->second : 0;
}

/**
 * Returns the encoding detector which matches the given name.
 * @param name the name
 * @return the encoding detector or @c null if not registered
 */
EncodingDetector* EncodingDetector::forName(const string& name) throw() {
	for(map<MIBenum, EncodingDetector*>::iterator i(registry().begin()), e(registry().end()); i != e; ++i) {
		const string canonicalName = i->second->name();
		if(matchEncodingNames(name.begin(), name.end(), canonicalName.begin(), canonicalName.end()))
			return i->second;
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
	case 50001:	return forID(UNIVERSAL_DETECTOR);
	case 50932:	return forID(JIS_DETECTOR);
	case 50949:	return forID(KS_DETECTOR);
	default:	return 0;
	}
}
#endif /* ASCENSION_WINDOWS */

map<MIBenum, EncodingDetector*>& EncodingDetector::registry() {
	static Registry<EncodingDetector> singleton;
	return singleton.registry;
}

/**
 * Registers the new encoding detector.
 * @param newDetector the encoding detector
 * @throw NullPointerException @a detector is @c null
 * @throw std#invalid_argument @a detectorID is already registered
 */
void EncodingDetector::registerDetector(auto_ptr<EncodingDetector> newDetector) {
	if(newDetector.get() == 0)
		throw NullPointerException("newDetector");
	const MIBenum id(newDetector->id());
	if(registry().find(id) != registry().end())
//		throw invalid_argument("the same identifier is already registered.");
		return;
	registry().insert(make_pair(id, newDetector.release()));
}

/// Returns true if the encoding detector which has the specified identifier.
bool EncodingDetector::supports(MIBenum detectorID) throw() {
	return detectorID == UNIVERSAL_DETECTOR || detectorID == SYSTEM_LOCALE_DETECTOR
		|| detectorID == USER_LOCALE_DETECTOR || registry().find(detectorID) != registry().end();
}


// UniversalDetector ////////////////////////////////////////////////////////

namespace {
	class UniversalDetector : public EncodingDetector {
	public:
		UniversalDetector() : EncodingDetector(EncodingDetector::UNIVERSAL_DETECTOR, "UniversalAutoDetect") {}
	private:
		MIBenum	doDetect(const byte* first, const byte* last, ptrdiff_t* convertibleBytes) const throw();
	};
//	ASCENSION_DEFINE_ENCODING_DETECTOR(SystemLocaleBasedDetector, EncodingDetector::SYSTEM_LOCALE_DETECTOR, "SystemLocaleAutoDetect");
//	ASCENSION_DEFINE_ENCODING_DETECTOR(UserLocaleBasedDetector, EncodingDetector::USER_LOCALE_DETECTOR, "UserLocaleAutoDetect");
} // namespace @0

/// @see EncodingDetector#doDetect
MIBenum UniversalDetector::doDetect(const byte* first, const byte* last, ptrdiff_t* convertibleBytes) const throw() {
	// try all detectors
	vector<MIBenum> mibs;
	availableIDs(back_inserter(mibs));

	MIBenum result = Encoder::getDefault();
	ptrdiff_t bestScore = 0, score;
	for(vector<MIBenum>::const_iterator mib(mibs.begin()), e(mibs.end()); mib != e; ++mib) {
		if(const EncodingDetector* detector = forID(*mib)) {
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
	class BasicLatinEncoder : public EncoderBase {
	public:
		virtual ~BasicLatinEncoder() throw() {}
	protected:
		BasicLatinEncoder(const string& name, MIBenum mib,
			const string& aliases, ulong mask) : EncoderBase(name, mib, 1, 1, aliases), mask_(mask) {}
	private:
		Result doFromUnicode(byte* to, byte* toEnd, byte*& toNext,
			const Char* from, const Char* fromEnd, const Char*& fromNext, State* state) const;
		Result doToUnicode(Char* to, Char* toEnd, Char*& toNext,
			const byte* from, const byte* fromEnd, const byte*& fromNext, State* state) const;
	private:
		const ulong mask_;
	};

	class US_ASCII : public BasicLatinEncoder {
	public:
		US_ASCII() : BasicLatinEncoder("US-ASCII", fundamental::US_ASCII,
			"iso-ir-6\0"
			"ANSI_X3.4-1986\0"
			"ISO_646.irv:1991\0"
			"ASCII\0"
			"ISO646-US\0"
			"us\0"
			"IBM367\0"
			"cp367\0"
			"csASCII", 0x7F) {}
	};

	class ISO_8859_1 : public BasicLatinEncoder {
	public:
		ISO_8859_1() : BasicLatinEncoder("ISO-8859-1", fundamental::ISO_8859_1,
			"iso-ir-100\0"
			"ISO_8859-1\0"
			"latin1\0"
			"l1\0"
			"IBM819\0"
			"CP819\0"
			"csISOLatin1", 0xFF) {}
	};

	struct Installer {
		Installer() throw() {
			Encoder::registerEncoder(auto_ptr<Encoder>(new US_ASCII));
			Encoder::registerEncoder(auto_ptr<Encoder>(new ISO_8859_1));
		}
	} unused;

	Encoder::Result BasicLatinEncoder::doFromUnicode(
			byte* to, byte* toEnd, byte*& toNext, const Char* from, const Char* fromEnd, const Char*& fromNext, State*) const {
		for(; to < toEnd && from < fromEnd; ++to, ++from) {
			if((*from ^ mask_) != 0) {
				if(policy() == IGNORE_UNMAPPABLE_CHARACTER)
					--to;
				else if(policy() == REPLACE_UNMAPPABLE_CHARACTER)
					*to = NATIVE_REPLACEMENT_CHARACTER;
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

	Encoder::Result BasicLatinEncoder::doToUnicode(
			Char* to, Char* toEnd, Char*& toNext, const byte* from, const byte* fromEnd, const byte*& fromNext, State*) const {
		for(; to < toEnd && from < fromEnd; ++to, ++from) {
			if((*from ^ mask_) != 0) {
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


// implementation.EncoderBase ///////////////////////////////////////////////

/**
 * Constructor.
 * @param name the name returned by @c #name
 * @param mib the MIBenum value returned by @c #mibEnum
 * @param maximumNativeBytes the value returned by @c #maximumNativeBytes
 * @param maximumUCSLength the value returned by @c #maximumUCSLength
 * @param aliases the encoding aliases returned by @c #aliases
 */
EncoderBase::EncoderBase(const string& name, MIBenum mib,
		size_t maximumNativeBytes /* = 1 */, size_t maximumUCSLength /* = 1 */, const string& aliases /* = "" */)
		: name_(name), aliases_(aliases), maximumNativeBytes_(maximumNativeBytes), maximumUCSLength_(maximumUCSLength), mib_(mib) {
}

/// Destructor.
EncoderBase::~EncoderBase() throw() {
}

/// @see Encoder#aliases
string EncoderBase::aliases() const throw() {
	return aliases_;
}

/// @see Encoder#maximumNativeBytes
size_t EncoderBase::maximumNativeBytes() const throw() {
	return maximumNativeBytes_;
}

/// @see Encoder#maximumUCSLength
size_t EncoderBase::maximumUCSLength() const throw() {
	return maximumUCSLength_;
}

/// @see Encoder#mibEnum
MIBenum EncoderBase::mibEnum() const throw() {
	return mib_;
}

/// @see Encoder#name
string EncoderBase::name() const throw() {
	return name_;
}


// implementation.SingleByteEncoder /////////////////////////////////////////

const Char SingleByteEncoder::ASCII_TABLE[0x80] = {
	ASCENSION_INCREMENTAL_BYTE_SEQUENCE_C0,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F
};

const byte SingleByteEncoder::UNMAPPABLE_16x16_UNICODE_TABLE[0x100] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

/**
 * Constructor.
 * @param name the name returned by @c #name
 * @param mib the MIBenum value returned by @c #mibEnum
 * @param aliases the encoding aliases returned by @c #aliases
 * @param native8ToUnicode the table maps a native byte 0x80 through 0xFF into a UCS2
 * @param native7ToUnicode the table maps a native byte 0x00 through 0x7F into a UCS2
 */
SingleByteEncoder::SingleByteEncoder(const string& name, MIBenum mib, const string& aliases,
		const Char native8ToUnicode[0x80], const Char native7ToUnicode[0x80] /* = 0 */) : EncoderBase(name, mib, 1, 1, aliases),
		native7ToUnicode_((native7ToUnicode != 0) ? native7ToUnicode : ASCII_TABLE), native8ToUnicode_(native8ToUnicode) {
	unicodeToNative_[0] = 0;
}

/// Destructor.
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
		const Char ucs = (i < 0x80) ? native7ToUnicode_[i] : native8ToUnicode_[i - 0x80];
		byte*& p = unicodeToNative_[ucs >> 8];
		if(p == 0) {
			p = new byte[0x100];
			fill_n(p, 0x100, UNMAPPABLE_NATIVE_CHARACTER);
		}
		p[mask8Bit(ucs)] = static_cast<byte>(i);
	}
}

/// @see Encoder#doFromUnicode
Encoder::Result SingleByteEncoder::doFromUnicode(byte* to, byte* toEnd, byte*& toNext,
		const Char* from, const Char* fromEnd, const Char*& fromNext, State*) const {
	if(unicodeToNative_[0] == 0)
		const_cast<SingleByteEncoder*>(this)->buildUnicodeToNativeTable();
	for(; to < toEnd && from < fromEnd; ++to, ++from) {
		*to = unicodeToNative_[*from >> 8][mask8Bit(*from)];
		if(*to == UNMAPPABLE_NATIVE_CHARACTER && *from != UNMAPPABLE_NATIVE_CHARACTER) {
			if(policy() == IGNORE_UNMAPPABLE_CHARACTER)
				--to;
			else if(policy() == REPLACE_UNMAPPABLE_CHARACTER)
				*to = NATIVE_REPLACEMENT_CHARACTER;
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

/// @see Encoder#doToUnicode
Encoder::Result SingleByteEncoder::doToUnicode(Char* to, Char* toEnd, Char*& toNext,
		const byte* from, const byte* fromEnd, const byte*& fromNext, State*) const {
	for(; to < toEnd && from < fromEnd; ++to, ++from) {
		*to = ((*from & 0x80) == 0) ? native7ToUnicode_[*from] : native8ToUnicode_[*from - 0x80];
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


//#include "encodings/win32cp.cpp"
