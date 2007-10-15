/**
 * @file encoder.cpp
 * @author exeal
 * @date 2004-2006
 */

#include "encoder.hpp"
#include "encodings/win32cp.cpp"
#include <algorithm>
using namespace ascension;
using namespace ascension::encoding;
using namespace std;


/// Returns the human-readable name of the encoding.
String encoding::getEncodingDisplayName(MIBenum mib) {
#ifdef _WIN32
	if(const uint cp = convertMIBtoWinCP(mib)) {
		manah::com::ComPtr<::IMultiLanguage> mlang;
		HRESULT hr = mlang.createInstance(CLSID_CMultiLanguage, 0, CLSCTX_INPROC, IID_IMultiLanguage);
		if(SUCCEEDED(hr)) {
			::MIMECPINFO mcpi;
			if(SUCCEEDED(hr = mlang->GetCodePageInfo(cp, &mcpi)))
				return mcpi.wszDescription;
		}
	}
#endif /* _WIN32 */
	if(const Encoder* encoder = Encoder::forMIB(mib)) {
		const string name(encoder->getName());
		String s(name.length(), L'x');
		copy(name.begin(), name.end(), s.begin());
		return s;
	} else if(const EncodingDetector* detector = EncodingDetector::forID(mib)) {
		const string name(detector->getName());
		String s(name.length(), L'x');
		copy(name.begin(), name.end(), s.begin());
		return s;
	}
	return L"";
}


// Encoder //////////////////////////////////////////////////////////////////

/**
 * @class ascension::Encoder
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

map<MIBenum, ASCENSION_SHARED_POINTER<Encoder> > Encoder::encoders_;

/// Protected default constructor.
Encoder::Encoder() throw() {
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
	const size_t bytes = (last - first) * getMaximumNativeLength();
	manah::AutoBuffer<uchar> temp(new uchar[bytes]);
	const Char* fromNext;
	uchar* toNext;
	return fromUnicode(temp.get(), temp.get() + bytes, toNext, first, last, fromNext, NO_POLICY) == COMPLETED;
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
	Encoders::iterator i(encoders_.find(mib));
	return (i != encoders_.end()) ? i->second.get() : 0;
}

/**
 * Returns the encoder which matches the given name.
 * @param name the name
 * @return the encoder or @c null if not registered
 */
Encoder* Encoder::forName(const string& name) throw() {
	for(Encoders::iterator i(encoders_.begin()), e(encoders_.end()); i != e; ++i) {
		// test canonical name
		const string canonicalName = i->second->getName();
		if(matchEncodingNames(name.begin(), name.end(), canonicalName.begin(), canonicalName.end()))
			return i->second.get();
		// test aliases
		const string aliases = i->second->getAliases();
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
 * @param[in] policy the conversion policy
 * @return the result of the conversion
 */
Encoder::Result Encoder::fromUnicode(uchar* to, uchar* toEnd, uchar*& toNext,
		const Char* from, const Char* fromEnd, const Char*& fromNext, Policy policy /* = NO_POLICY */) const {
	if(to == 0 || toEnd == 0 || from == 0 || fromEnd == 0)
		throw NullPointerException("");
	else if(to > toEnd)
		throw invalid_argument("to > toEnd");
	else if(from > fromEnd)
		throw invalid_argument("from > fromEnd");
	toNext = 0;
	fromNext = 0;
	return doFromUnicode(to, toEnd, toNext, from, fromEnd, fromNext, policy);
}

/**
 * Converts the given string from UTF-16 into the native encoding.
 * @param from the string to be converted
 * @param policy the conversion policy
 * @return the converted string or an empty if encountered unconvertable character
 */
string Encoder::fromUnicode(const String& from, Policy policy /* = NO_POLICY */) const {
	size_t bytes = getMaximumNativeLength() * from.length();
	manah::AutoBuffer<uchar> temp(new uchar[bytes]);
	const Char* fromNext;
	uchar* toNext;
	Result result;
	while(true) {
		result = fromUnicode(temp.get(), temp.get() + bytes, toNext, from.data(), from.data() + from.length(), fromNext, policy);
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
	return fundamental::MIB_UNICODE_UTF8;
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
	else if(encoders_.find(encoder->getMIBenum()) != encoders_.end())
//		throw invalid_argument("the encoder is already registered.");
		return;
//	encoders_.insert(make_pair(encoder->getMIBenum(), encoder.release()));
}

/// Returns true if supports the specified encoding.
bool Encoder::supports(MIBenum mib) throw() {
	return encoders_.find(mib) != encoders_.end();
}

/**
 * Converts the given string from the native encoding into UTF-16.
 * @param[out] to the beginning of the destination buffer
 * @param[out] toEnd the end of the destination buffer
 * @param[out] toNext points the first unaltered character in the destination buffer after the conversion
 * @param[in] from the beginning of the buffer to be converted
 * @param[in] fromEnd the end of the buffer to be converted
 * @param[in] fromNext points to the first unconverted character after the conversion
 * @param[in] policy the conversion policy
 * @return the result of the conversion
 */
Encoder::Result Encoder::toUnicode(Char* to, Char* toEnd, Char*& toNext,
		const uchar* from, const uchar* fromEnd, const uchar*& fromNext, Policy policy /* = NO_POLICY */) const {
	if(to == 0 || toEnd == 0 || from == 0 || fromEnd == 0)
		throw NullPointerException("");
	else if(to > toEnd)
		throw invalid_argument("to > toEnd");
	else if(from > fromEnd)
		throw invalid_argument("from > fromEnd");
	toNext = 0;
	fromNext = 0;
	return doToUnicode(to, toEnd, toNext, from, fromEnd, fromNext, policy);
}

/**
 * Converts the given string from the native encoding into UTF-16.
 * @param from the string to be converted
 * @param policy the conversion policy
 * @return the converted string or an empty if encountered unconvertable character
 */
String Encoder::toUnicode(const string& from, Policy policy /* = NO_POLICY */) const {
	size_t chars = getMaximumUCSLength() * from.length();
	manah::AutoBuffer<Char> temp(new Char[chars]);
	const uchar* fromNext;
	Char* toNext;
	Result result;
	while(true) {
		result = toUnicode(temp.get(), temp.get() + chars, toNext,
			reinterpret_cast<const uchar*>(from.data()), reinterpret_cast<const uchar*>(from.data()) + from.length(), fromNext, policy);
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

map<MIBenum, ASCENSION_SHARED_POINTER<EncodingDetector> > EncodingDetector::encodingDetectors_;

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
 * @return the MIBenum of the detected encoding
 */
MIBenum EncodingDetector::detect(MIBenum detectorID, const uchar* first, const uchar* last) {
	if(first == 0 || last == 0)
		throw NullPointerException("first or last");
	else if(first > last)
		throw invalid_argument("first > last");

#if 0
#ifndef ASCENSION_NO_EXTENDED_ENCODINGS
	if(detectorID == SYSTEM_LOCALE_DETECTOR || detectorID == USER_LOCALE_DETECTOR) {
#ifdef _WIN32
		const ::LANGID langID = (detectorID == SYSTEM_LOCALE_DETECTOR) ? ::GetSystemDefaultLangID() : ::GetUserDefaultLangID();
		switch(PRIMARYLANGID(langID)) {
		case LANG_ARMENIAN:		mib = extended::MIB_ARMENIAN_AUTO_DETECTION;	break;
		case LANG_JAPANESE:		mib = extended::MIB_JAPANESE_AUTO_DETECTION;	break;
//		case LANG_KOREAN:		mib = extended::MIB_KOREAN_AUTO_DETECTION;		break;
		case LANG_VIETNAMESE:	mib = extended::MIB_VIETNAMESE_AUTO_DETECTION;	break;
		default:				mib = fundamental::MIB_UNICODE_AUTO_DETECTION;	break;
		}
#else
#endif
	}
#endif /* !ASCENSION_NO_EXTENDED_ENCODINGS */
#endif

	MIBenum result = fundamental::MIB_UNICODE_UTF8;
	if(detectorID == UNIVERSAL_DETECTOR) {
		// try all detectors
		MIBenum detected;
		ptrdiff_t bestScore = 0;
		for(EncodingDetectors::const_iterator i(encodingDetectors_.begin()), e(encodingDetectors_.end()); i != e; ++i) {
			const ptrdiff_t score = i->second->doDetect(first, last, detected);
			if(score > bestScore) {
				result = detected;
				if(score == last - first)
					break;
				bestScore = score;
			}
		}
	} else {
		EncodingDetectors::const_iterator detector(encodingDetectors_.find(detectorID));
		if(detector == encodingDetectors_.end())
			throw invalid_argument("unsupported encoding detector identifier.");
		detector->second->doDetect(first, last, result);
	}
	return result;
}

/**
 * Returns the encoding detector which has the given identifier.
 * @param id the identifier
 * @return the encoding detectir or @c null if not registered
 */
EncodingDetector* EncodingDetector::forID(MIBenum id) throw() {
	EncodingDetectors::iterator i(encodingDetectors_.find(id));
	return (i != encodingDetectors_.end()) ? i->second.get() : 0;
}

/**
 * Returns the encoding detector which matches the given name.
 * @param name the name
 * @return the encoding detector or @c null if not registered
 */
EncodingDetector* EncodingDetector::forName(const string& name) throw() {
	for(EncodingDetectors::iterator i(encodingDetectors_.begin()), e(encodingDetectors_.end()); i != e; ++i) {
		const string canonicalName = i->second->getName();
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
	// TODO: not implemented.
//	for(EncodingDetectors::iterator i(encodingDetectors_.begin()), e(encodingDetectors_.end()); i != e; ++i) {
//		if(convertMIBenumToWindowsCodePage(i->second->getID()) == codePage)
//			return i->second.get();
//	}
	return 0;
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
	const MIBenum id(newDetector->getID());
	if(encodingDetectors_.find(id) != encodingDetectors_.end())
		throw invalid_argument("the same identifier is already registered.");
	encodingDetectors_.insert(make_pair(id, ASCENSION_SHARED_POINTER<EncodingDetector>(newDetector.release())));
}

/// Returns true if the encoding detector which has the specified identifier.
bool EncodingDetector::supports(MIBenum detectorID) throw() {
	return detectorID == UNIVERSAL_DETECTOR || detectorID == SYSTEM_LOCALE_DETECTOR
		|| detectorID == USER_LOCALE_DETECTOR || encodingDetectors_.find(detectorID) != encodingDetectors_.end();
}


// USASCIIEncoder ///////////////////////////////////////////////////////////

namespace {
	ASCENSION_DEFINE_SIMPLE_ENCODER_WITH_ALIASES(USASCIIEncoder);
	template<Char maximum, uchar(*mask)(Char)>
	inline Encoder::Result doFromUnicode8Bit(uchar* to, uchar* toEnd, uchar*& toNext,
			const Char* from, const Char* fromEnd, const Char*& fromNext, Encoder::Policy policy) {
		for(; to < toEnd && from < fromEnd; ++from) {
			if(*from > maximum) {
				if(policy == Encoder::REPLACE_UNMAPPABLE_CHARACTER)
					*(to++) = NATIVE_DEFAULT_CHARACTER;
				else if(policy != Encoder::IGNORE_UNMAPPABLE_CHARACTER) {
					toNext = to;
					fromNext = from;
					return Encoder::UNMAPPABLE_CHARACTER;
				} else
					*(to++) = mask(*from);
			}
		}
		toNext = to;
		fromNext = from;
		return (fromNext == fromEnd) ? Encoder::COMPLETED : Encoder::INSUFFICIENT_BUFFER;
	}
	inline Encoder::Result doToUnicode8Bit(Char* to, Char* toEnd, Char*& toNext,
			const uchar* from, const uchar* fromEnd, const uchar*& fromNext, Encoder::Policy policy) {
		for(; to < toEnd && from < fromEnd; ++to, ++from)
			*to = *from;
		toNext = to;
		fromNext = from;
		return (fromNext == fromEnd) ? Encoder::COMPLETED : Encoder::INSUFFICIENT_BUFFER;
	}
} // namespace @0

/// @see Encoder#doFromUnicode
Encoder::Result USASCIIEncoder::doFromUnicode(uchar* to, uchar* toEnd, uchar*& toNext,
		const Char* from, const Char* fromEnd, const Char*& fromNext, Policy policy) const {
	return doFromUnicode8Bit<0x7F, mask7Bit>(to, toEnd, toNext, from, fromEnd, fromNext, policy);
}

/// @see Encoder#doToUnicode
Encoder::Result USASCIIEncoder::doToUnicode(Char* to, Char* toEnd, Char*& toNext,
		const uchar* from, const uchar* fromEnd, const uchar*& fromNext, Policy policy) const {
	return doToUnicode8Bit(to, toEnd, toNext, from, fromEnd, fromNext, policy);
}

/// @see Encoder#getAliases
string USASCIIEncoder::getAliases() const throw() {
	static const char aliases[] =
		"iso-ir-6\0"
		"ANSI_X3.4-1986\0"
		"ISO_646.irv:1991\0"
		"ASCII\0"
		"ISO646-US\0"
		"us\0"
		"IBM367\0"
		"cp367\0"
		"csASCII";
	return string(aliases, countof(aliases) - 1);
}

/// @see Encoder#getMaximumNativeLength
size_t USASCIIEncoder::getMaximumNativeLength() const throw() {
	return 1;
}

/// @see Encoder#getMIBenum
MIBenum USASCIIEncoder::getMIBenum() const throw() {
	return fundamental::MIB_US_ASCII;
}

/// @see Encoder#getName
string USASCIIEncoder::getName() const throw() {
	return "US-ASCII";
}


// ISO88591Encoder //////////////////////////////////////////////////////////

namespace {
	ASCENSION_DEFINE_SIMPLE_ENCODER_WITH_ALIASES(ISO88591Encoder);
} // namespace @0

/// @see Encoder#doFromUnicode
Encoder::Result ISO88591Encoder::doFromUnicode(uchar* to, uchar* toEnd, uchar*& toNext,
		const Char* from, const Char* fromEnd, const Char*& fromNext, Policy policy) const {
	return doFromUnicode8Bit<0xFF, mask8Bit>(to, toEnd, toNext, from, fromEnd, fromNext, policy);
}

/// @see Encoder#doToUnicode
Encoder::Result ISO88591Encoder::doToUnicode(Char* to, Char* toEnd, Char*& toNext,
		const uchar* from, const uchar* fromEnd, const uchar*& fromNext, Policy policy) const {
	return doToUnicode8Bit(to, toEnd, toNext, from, fromEnd, fromNext, policy);
}

/// @see Encoder#getAliases
string ISO88591Encoder::getAliases() const throw() {
	static const char aliases[] =
		"iso-ir-100\0"
		"ISO_8859-1\0"
		"latin1\0"
		"l1\0"
		"IBM819\0"
		"CP819\0"
		"csISOLatin1";
	return string(aliases, countof(aliases) - 1);
}

/// @see Encoder#getMaximumNativeLength
size_t ISO88591Encoder::getMaximumNativeLength() const throw() {
	return 1;
}

/// @see Encoder#getMIBenum
MIBenum ISO88591Encoder::getMIBenum() const throw() {
	return fundamental::MIB_ISO_8859_1;
}

/// @see Encoder#getName
string ISO88591Encoder::getName() const throw() {
	return "ISO-8859-1";
}


namespace {
	struct Installer {
		Installer() throw() {
			Encoder::registerEncoder(auto_ptr<Encoder>(new USASCIIEncoder));
			Encoder::registerEncoder(auto_ptr<Encoder>(new ISO88591Encoder));
#ifdef _WIN32
			::EnumSystemCodePagesW(procedure, CP_INSTALLED);
#endif /* _WIN32 */
		}
#ifdef _WIN32
		static BOOL CALLBACK procedure(::LPWSTR name) {
			const ::UINT cp = wcstoul(name, 0, 10);
			if(toBoolean(::IsValidCodePage(cp))) {
				::CPINFOEXW cpi;
				if(toBoolean(::GetCPInfoExW(cp, 0, &cpi))) {
					if(const MIBenum mib = convertWinCPtoMIB(cp)) {
						if(!Encoder::supports(mib))
							Encoder::registerEncoder(auto_ptr<Encoder>(new WindowsNLSEncoder(cp, mib)));
					}
				}
			}
			return TRUE;
		}
#endif /* _WIN32 */
	} unused;
} // namespace @0
