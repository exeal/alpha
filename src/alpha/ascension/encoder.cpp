/**
 * @file encoder.cpp
 * @author exeal
 * @date 2004-2006
 */

#include "StdAfx.h"
#include "encoder.hpp"
#include <algorithm>
using namespace ascension;
using namespace ascension::encoding;
using namespace std;


// Encoder //////////////////////////////////////////////////////////////////

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
 * Detects the encoding of the string buffer.
 * @param first the beginning of the sequence
 * @param last the end of the sequence
 * @param mib the MIBenum value of the encoding
 * @return the MIBenum of the detected encoding
 */
MIBenum Encoder::detectEncoding(const uchar* first, const uchar* last, MIBenum mib) {
	if(first == 0 || last == 0)
		throw NullPointerException("first or last");
	else if(first > last)
		throw invalid_argument("first > last");

#ifndef ASCENSION_NO_EXTENDED_ENCODINGS
	if(mib == extended::MIB_AUTO_DETECTION_SYSTEM_LANGUAGE || mib == extended::MIB_AUTO_DETECTION_USER_LANGUAGE) {
#ifdef _WIN32
		const ::LANGID langID = (mib == extended::MIB_AUTO_DETECTION_SYSTEM_LANGUAGE) ? ::GetSystemDefaultLangID() : ::GetUserDefaultLangID();
		switch(PRIMARYLANGID(langID)) {
		case LANG_ARMENIAN:		mib = extended::MIB_ARMENIAN_AUTO_DETECTION;	break;
		case LANG_JAPANESE:		mib = extended::MIB_JAPANESE_AUTO_DETECTION;	break;
//		case LANG_KOREAN:		mib = extended::MIB_KOREAN_AUTO_DETECTION;		break;
		case LANG_VIETNAMESE:	mib = extended::MIB_VIETNAMESE_AUTO_DETECTION;	break;
		default:				mib = fundamental::MIB_UNICODE_AUTO_DETECTION;	break;
		}
#else
#endif
#endif /* !ASCENSION_NO_EXTENDED_ENCODINGS */
	}

	EncodingDetectors::const_iterator detector(encodingDetectors_.find(mib));
	if(detector == encodingDetectors_.end())
		detector = encodingDetectors_.find(fundamental::MIB_UNICODE_AUTO_DETECTION);
	assert(detector != encodingDetectors_.end());

	MIBenum result;
	size_t score = (*detector->second)(first, last, result);
	return (score != 0) ? result : fundamental::MIB_UNICODE_UTF8;
}

/**
 * Returns the encoder which has the given MIBenum value.
 * @param mib the MIBenum value
 * @return the encoder or @c null if not registered
 */
Encoder* Encoder::forMIB(MIBenum mib) throw() {
	Encoders::iterator i(encoders_.find(mib));
	return (i != encoders_.end()) ? i->second : 0;
}

/**
 * Returns the encoder which matches the given name.
 * @param name the name
 * @return the encoder or @c null if not registered
 */
Encoder* Encoder::forName(const string& name) throw() {
	for(Encoders::iterator i(encoders_.begin()), e(encoders_.end()); i != e; ++i) {
		const string otherName = i->second->getName();
		if(matchEncodingNames(name.begin(), name.end(), otherName.begin(), otherName.end()))
			return i->second;
	}
	return 0;
}

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
	toNext = to;
	fromNext = from;
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
 * Registers the new auto encoding detector.
 * @param mib the MIBenum value of the detector
 * @param detector the function produces an encoding detector
 * @throw NullPointerException @a detector is @c null
 * @throw std#invalid_argument @a mib is already registered
 */
void Encoder::registerDetector(MIBenum mib, EncodingDetector detector) {
	if(detector == 0)
		throw NullPointerException("detector");
	else if(encodingDetectors_.find(mib) != encodingDetectors_.end())
		throw invalid_argument("min is already registered.");
	encodingDetectors_.insert(make_pair(mib, detector));
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
		throw invalid_argument("the encoder is already registered.");
	encoders_.insert(make_pair(encoder->getMIBenum(), encoder.release()));
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
	toNext = to;
	fromNext = from;
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


#ifdef _WIN32

// WindowsEncoder ///////////////////////////////////////////////////////////

namespace {
	/// Encoder uses Windows NLS.
	class WindowsEncoder : public Encoder {
	public:
		WindowsEncoder(::UINT codePage);
	private:
		Result		doFromUnicode(uchar* to, uchar* toEnd, uchar*& toNext,
						const Char* from, const Char* fromEnd, const Char*& fromNext, Policy policy) const;
		Result		doToUnicode(Char* to, Char* toEnd, Char*& toNext,
						const uchar* from, const uchar* fromEnd, const uchar*& fromNext, Policy policy) const;
		std::string	getAliases() const throw();
		std::size_t	getMaximumNativeLength() const throw();
		MIBenum		getMIBenum() const throw();
		std::string	getName() const throw();
	private:
		const ::UINT codePage_;
	};

	struct WindowsEncoderInstaller {
		WindowsEncoderInstaller() throw() {
			::EnumSystemCodePagesW(procedure, CP_INSTALLED);
		}
		static BOOL CALLBACK procedure(::LPWSTR name) {
			const ::UINT cp = wcstoul(name, 0, 10);
			if(toBoolean(::IsValidCodePage(cp)))
				Encoder::registerEncoder(auto_ptr<Encoder>(new WindowsEncoder(cp)));
			return TRUE;
		}
	} unused;
} // namespace @0

/**
 * Constructor.
 * @param codePage the code page
 * @throw std#invalid_argument @a codePage is not supported by the system
 */
WindowsEncoder::WindowsEncoder(::UINT cp) : codePage_(cp) {
	if(!toBoolean(::IsValidCodePage(cp)))
		throw invalid_argument("Specified code page is not supported.");
}

/// @see Encoder#doFromUnicode
Encoder::Result WindowsEncoder::doFromUnicode(uchar* to, uchar* toEnd, uchar*& toNext,
		const Char* from, const Char* fromEnd, const Char*& fromNext, Policy policy) const {
	if(from == fromEnd) {
		// no conversion
		fromNext = from;
		toNext = to;
		return COMPLETED;
	} else if(const int writtenBytes = ::WideCharToMultiByte(codePage_,
			WC_SEPCHARS | (policy == REPLACE_UNMAPPABLE_CHARACTER ? WC_DEFAULTCHAR : 0),
			from, static_cast<int>(fromEnd - from), reinterpret_cast<char*>(to), static_cast<int>(toEnd - to), 0, 0)) {
		// succeeded (fromNext is not modified)
		toNext = to + writtenBytes;
		return COMPLETED;
	} else
		return (::GetLastError() == ERROR_INSUFFICIENT_BUFFER) ? INSUFFICIENT_BUFFER : UNMAPPABLE_CHARACTER;
}

/// @see Encoder#doToUnicode
Encoder::Result WindowsEncoder::doToUnicode(Char* to, Char* toEnd, Char*& toNext,
		const uchar* from, const uchar* fromEnd, const uchar*& fromNext, Policy policy) const {
	if(from == fromEnd) {
		// no conversion
		fromNext = from;
		toNext = to;
		return COMPLETED;
	} else if(const int writtenChars = ::MultiByteToWideChar(codePage_,
			policy == REPLACE_UNMAPPABLE_CHARACTER ? 0 : MB_ERR_INVALID_CHARS,
			reinterpret_cast<const char*>(from), static_cast<int>(fromEnd - from), to, static_cast<int>(toEnd - to))) {
		// succeeded (fromNext is not modified)
		toNext = to + writtenChars;
		return COMPLETED;
	} else
		return (::GetLastError() == ERROR_INSUFFICIENT_BUFFER) ? INSUFFICIENT_BUFFER : UNMAPPABLE_CHARACTER;
}

/// @see Encoder#getMaximumNativeLength
size_t WindowsEncoder::getMaximumNativeLength() const throw() {
	::CPINFO cpi;
	return toBoolean(::GetCPInfo(codePage_, &cpi)) ? static_cast<uchar>(cpi.MaxCharSize) : 0;
}

/// @see Encoder#getName
string WindowsEncoder::getName() const throw() {
	::CPINFOEXA cpi;
	::GetCPInfoExA(codePage_, 0, &cpi);
	return cpi.CodePageName;
}

#endif /* _WIN32 */
