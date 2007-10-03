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
//using namespace ascension::unicode;
using namespace std;


// Encoder //////////////////////////////////////////////////////////////////

/// Protected default constructor.
Encoder::Encoder() throw() {
}

/// Destructor.
Encoder::~Encoder() throw() {
}

/**
 * Returns true if the given character @c c can be fully encoded with this encoding.
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

	if(mib == MIB_AUTO_DETECTION_SYSTEM_LANGUAGE || mib == MIB_AUTO_DETECTION_USER_LANGUAGE) {
#ifdef _WIN32
		const ::LANGID langID = (mib == MIB_AUTO_DETECTION_SYSTEM_LANGUAGE) ? ::GetSystemDefaultLangID() : ::GetUserDefaultLangID();
		switch(PRIMARYLANGID(langID)) {
		case LANG_ARMENIAN:		mib = MIB_ARMENIAN_AUTO_DETECTION;		break;
		case LANG_JAPANESE:		mib = MIB_JAPANESE_AUTO_DETECTION;		break;
//		case LANG_KOREAN:		mib = MIB_KOREAN_AUTO_DETECTION;		break;
		case LANG_VIETNAMESE:	mib = MIB_VIETNAMESE_AUTO_DETECTION;	break;
		default:				mib = MIB_UNICODE_AUTO_DETECTION;		break;
		}
#else
#endif
	}

	EncodingDetectors::const_iterator detector(encodingDetectors_.find(mib));
	if(detector == encodingDetectors_.end())
		detector = encodingDetectors_.find(MIB_UNICODE_AUTO_DETECTION);
	assert(detector != encodingDetectors_.end());

	MIBenum result;
	size_t score = (*detector->second)(first, last, result);
	return (score != 0) ? result : forName("UTF-8")->getMIBenum();;
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
		const Char* from, const Char* fromEnd, const Char*& fromNext, const manah::Flags<Policy>& policy /* = NO_POLICY */) const {
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
string Encoder::fromUnicode(const String& from, const manah::Flags<Policy>& policy /* = NO_POLICY */) const {
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
		else if(result == ILLEGAL_CHARACTER)
			return "";
	}
	return string(temp.get(), toNext);
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
		const uchar* from, const uchar* fromEnd, const uchar*& fromNext, const manah::Flags<Policy>& policy /* = NO_POLICY */) const {
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
String Encoder::toUnicode(const string& from, const manah::Flags<Policy>& policy /* = NO_POLICY */) const {
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
		else if(result == ILLEGAL_CHARACTER)
			return L"";
	}
	return String(temp.get(), toNext);
}
