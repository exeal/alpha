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
	return doFromUnicode(temp.get(), temp.get() + bytes, toNext, first, last, fromNext, NO_UNICODE_BYTE_ORDER_SIGNATURE);
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
MIBenum EncoderFactory::detectCodePage(const uchar* first, const uchar* last, MIBenum mib) {
	if(first == 0 || last == 0)
		throw NullPointerException("first or last");
	else if(first > last)
		throw invalid_argument("first > last");
	EncodingDetector::const_iterator detector(encodingDetectors_.find(mib));
	if(detector == encodingDetector_.end())
		throw invalid_argument("the given MIBenum is not registered as an encoding detection encoding.");

	CodePage result;
	size_t score;

	// 透過的に言語を選択する場合
	if(cp == CPEX_AUTODETECT_SYSTEMLANG || cp == CPEX_AUTODETECT_USERLANG) {
		const LANGID langID = (cp == CPEX_AUTODETECT_SYSTEMLANG) ? ::GetSystemDefaultLangID() : ::GetUserDefaultLangID();
		switch(PRIMARYLANGID(langID)) {
		case LANG_ARMENIAN:	cp = CPEX_ARMENIAN_AUTODETECT;	break;
		case LANG_JAPANESE:	cp = CPEX_JAPANESE_AUTODETECT;	break;
//		case LANG_KOREAN:	cp = CPEX_KOREAN_AUTODETECT;	break;
		default:			cp = CPEX_UNICODE_AUTODETECT;	break;
		}
	}

	DetectorMap::iterator it = registeredDetectors_.find(cp);

	assert(it != registeredDetectors_.end());
	it->second(src, length, result, score);
	return (score != 0) ? result : ::GetACP();
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

bool Encoder::fromUnicode(uchar* to, uchar* toEnd, uchar*& toNext,
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
