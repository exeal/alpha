/**
 * @file encoder.cpp
 * @author exeal
 * @date 2004-2006
 */

#include "encoder.hpp"
#include <algorithm>
#include <vector>
#ifdef _WIN32
#include <windows.h>	// GetCPInfoExW, ...
#ifndef interface
#define interface struct
#endif
#include <mlang.h>
#include "../../manah/com/common.hpp"
#endif /* _WIN32 */
using namespace ascension;
using namespace ascension::encoding;
using namespace std;


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
 * @throw invalid_argument @a c is not a Unicode scalar value
 */
bool Encoder::canEncode(CodePoint c) const {
	if(!unicode::isScalarValue(c))
		throw invalid_argument("the code point is not a scalar value.");
	Char temp[2];
	return canEncode(temp, temp + unicode::surrogates::encode(c, temp));
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
	manah::AutoBuffer<uchar> temp(new uchar[bytes]);
	Char* fromNext;
	uchar* toNext;
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
	Encoders::iterator i(registry().find(mib));
	return (i != registry().end()) ? i->second.get() : 0;
}

/**
 * Returns the encoder which matches the given name.
 * @param name the name
 * @return the encoder or @c null if not registered
 */
Encoder* Encoder::forName(const string& name) throw() {
	for(Encoders::iterator i(registry().begin()), e(registry().end()); i != e; ++i) {
		// test canonical name
		const string canonicalName = i->second->name();
		if(matchEncodingNames(name.begin(), name.end(), canonicalName.begin(), canonicalName.end()))
			return i->second.get();
		// test aliases
		const string aliases = i->second->aliases();
		for(size_t j = 0; ; ++j) {
			size_t nul = aliases.find('\0', j);
			if(nul == string::npos)
				nul = aliases.length();
			if(nul != j) {
				if(matchEncodingNames(name.begin(), name.end(), aliases.begin() + j, aliases.begin() + nul))
					return i->second.get();
				++nul;
			}
			if(nul == aliases.length())
				break;
			j = nul;
		}
	}
	return 0;
}

#ifdef _WIN32
/**
 * Returns the encoder which has the given Win32 code page.
 * @param codePage the code page
 * @return the encoder or @c null if not registered
 */
Encoder* Encoder::forWindowsCodePage(::UINT codePage) throw() {
	// TODO: not implemented.
	return 0;
}
#endif /* _WIN32 */

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
Encoder::Result Encoder::fromUnicode(uchar* to, uchar* toEnd, uchar*& toNext,
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
	manah::AutoBuffer<uchar> temp(new uchar[bytes]);
	const Char* fromNext;
	uchar* toNext;
	Result result;
	while(true) {
		result = fromUnicode(temp.get(), temp.get() + bytes, toNext, from.data(), from.data() + from.length(), fromNext);
		if(result == COMPLETED)
			break;
		else if(result == INSUFFICIENT_BUFFER)
			temp.reset(new uchar[bytes *= 2]);
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

Encoder::Encoders& Encoder::registry() throw() {
	static Encoders singleton;
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
		const uchar* from, const uchar* fromEnd, const uchar*& fromNext, State* state /* = 0*/) const {
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
	const uchar* fromNext;
	Char* toNext;
	Result result;
	while(true) {
		result = toUnicode(temp.get(), temp.get() + chars, toNext,
			reinterpret_cast<const uchar*>(from.data()), reinterpret_cast<const uchar*>(from.data()) + from.length(), fromNext);
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
MIBenum EncodingDetector::detect(const uchar* first, const uchar* last, ptrdiff_t* convertibleBytes) const {
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
	EncodingDetectors::iterator i(registry().find(id));
	return (i != registry().end()) ? i->second.get() : 0;
}

/**
 * Returns the encoding detector which matches the given name.
 * @param name the name
 * @return the encoding detector or @c null if not registered
 */
EncodingDetector* EncodingDetector::forName(const string& name) throw() {
	for(EncodingDetectors::iterator i(registry().begin()), e(registry().end()); i != e; ++i) {
		const string canonicalName = i->second->name();
		if(matchEncodingNames(name.begin(), name.end(), canonicalName.begin(), canonicalName.end()))
			return i->second.get();
	}
	return 0;
}

#ifdef _WIN32
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
#endif /* !_WIN32 */

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
	registry().insert(make_pair(id, ASCENSION_SHARED_POINTER<EncodingDetector>(newDetector.release())));
}

EncodingDetector::EncodingDetectors& EncodingDetector::registry() throw() {
	static EncodingDetectors singleton;
	return singleton;
}

/// Returns true if the encoding detector which has the specified identifier.
bool EncodingDetector::supports(MIBenum detectorID) throw() {
	return detectorID == UNIVERSAL_DETECTOR || detectorID == SYSTEM_LOCALE_DETECTOR
		|| detectorID == USER_LOCALE_DETECTOR || registry().find(detectorID) != registry().end();
}


// USASCIIEncoder ///////////////////////////////////////////////////////////

namespace {
	ASCENSION_BEGIN_SBCS_ENCODER_CLASS(USASCIIEncoder, fundamental::US_ASCII, "US-ASCII")
		ASCENSION_ENCODER_ALIASES(
			"iso-ir-6\0"
			"ANSI_X3.4-1986\0"
			"ISO_646.irv:1991\0"
			"ASCII\0"
			"ISO646-US\0"
			"us\0"
			"IBM367\0"
			"cp367\0"
			"csASCII"
		)
	ASCENSION_END_ENCODER_CLASS()
	template<Char maximum, uchar(*mask)(Char)> inline bool doFromUnicode8Bit(uchar& to, Char from) {
		if(from > maximum)
			return false;
		to = mask(from);
		return true;
	}
} // namespace @0

/// @see SBCSEncoder#doFromUnicode
inline bool USASCIIEncoder::doFromUnicode(uchar& to, Char from) const {
	return doFromUnicode8Bit<0x7F, mask7Bit>(to, from);
}

/// @see SBCSEncoder#doToUnicode
inline bool USASCIIEncoder::doToUnicode(Char& to, uchar from) const {
	return to = from, true;
}


// ISO88591Encoder //////////////////////////////////////////////////////////

namespace {
	ASCENSION_BEGIN_SBCS_ENCODER_CLASS(ISO88591Encoder, fundamental::ISO_8859_1, "ISO-8859-1")
		ASCENSION_ENCODER_ALIASES(
			"iso-ir-100\0"
			"ISO_8859-1\0"
			"latin1\0"
			"l1\0"
			"IBM819\0"
			"CP819\0"
			"csISOLatin1"
		)
	ASCENSION_END_ENCODER_CLASS()
} // namespace @0

/// @see SBCSEncoder#doFromUnicode
inline bool ISO88591Encoder::doFromUnicode(uchar& to, Char from) const {
	return doFromUnicode8Bit<0xFF, mask8Bit>(to, from);
}

/// @see SBCSEncoder#doToUnicode
inline bool ISO88591Encoder::doToUnicode(Char& to, uchar from) const {
	return to = from, true;
}


// UniversalDetector ////////////////////////////////////////////////////////

namespace {
	ASCENSION_DEFINE_ENCODING_DETECTOR(UniversalDetector, EncodingDetector::UNIVERSAL_DETECTOR, "UniversalAutoDetect");
	ASCENSION_DEFINE_ENCODING_DETECTOR(SystemLocaleBasedDetector, EncodingDetector::SYSTEM_LOCALE_DETECTOR, "SystemLocaleAutoDetect");
	ASCENSION_DEFINE_ENCODING_DETECTOR(UserLocaleBasedDetector, EncodingDetector::USER_LOCALE_DETECTOR, "UserLocaleAutoDetect");
} // namespace @0

/// @see EncodingDetector#doDetect
MIBenum UniversalDetector::doDetect(const uchar* first, const uchar* last, ptrdiff_t* convertibleBytes) const throw() {
	// try all detectors
	vector<MIBenum> mibs;
	availableIDs(back_inserter(mibs));

	MIBenum result;
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


namespace {
	struct Installer {
		Installer() throw() {
			Encoder::registerEncoder(auto_ptr<Encoder>(new USASCIIEncoder));
			Encoder::registerEncoder(auto_ptr<Encoder>(new ISO88591Encoder));
		}
	} unused;
} // namespace @0


#include "encodings/win32cp.cpp"
