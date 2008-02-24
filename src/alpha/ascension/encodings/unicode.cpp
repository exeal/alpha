/**
 * @file unicode.cpp
 * Implements Unicode encodings. This includes:
 * - UTF-8
 * - UTF-7
 * - UTF-16BE
 * - UTF-16LE
 * - UTF-16
 * - UTF-32
 * - UTF-32BE
 * - UTF-32LE
 * - UTF-5
 * @author exeal
 * @date 2003-2008
 */

#include "../encoder.hpp"
#include <algorithm>	// std.find_if
using namespace ascension;
using namespace ascension::encoding;
using namespace ascension::encoding::implementation;
using namespace ascension::text;
using namespace std;


// registry
namespace {
	template<typename Factory>
	class InternalEncoder : public Encoder {
	public:
		explicit InternalEncoder(const Factory& factory) throw() : props_(factory), encodingState_(0), decodingState_(0) {}
	private:
		Result doFromUnicode(byte* to, byte* toEnd, byte*& toNext,
			const Char* from, const Char* fromEnd, const Char*& fromNext);
		Result doToUnicode(Char* to, Char* toEnd, Char*& toNext,
			const byte* from, const byte* fromEnd, const byte*& fromNext);
		const IEncodingProperties& properties() const throw() {return props_;}
		Encoder& resetDecodingState() throw() {decodingState_ = 0; return *this;}
		Encoder& resetEncodingState() throw() {encodingState_ = 0; return *this;}
	private:
		const IEncodingProperties& props_;
		byte encodingState_, decodingState_;
	};

	class UTF_8 : public EncoderFactoryBase {
	public:
		UTF_8() : EncoderFactoryBase("UTF-8", fundamental::UTF_8, "Unicode (UTF-8)", 4) {}
	private:
		auto_ptr<Encoder> create() const throw() {return auto_ptr<Encoder>(new InternalEncoder<UTF_8>(*this));}
	} utf8;
	class UTF_16LE : public EncoderFactoryBase {
	public:
		UTF_16LE() : EncoderFactoryBase("UTF-16LE", fundamental::UTF_16LE, "Unicode (UTF-16LE)", 2) {}
	private:
		auto_ptr<Encoder> create() const throw() {return auto_ptr<Encoder>(new InternalEncoder<UTF_16LE>(*this));}
	} utf16le;
	class UTF_16BE : public EncoderFactoryBase {
	public:
		UTF_16BE() : EncoderFactoryBase("UTF-16BE", fundamental::UTF_16BE, "Unicode (UTF-16BE)", 2) {}
	private:
		auto_ptr<Encoder> create() const throw() {return auto_ptr<Encoder>(new InternalEncoder<UTF_16BE>(*this));}
	} utf16be;
#ifndef ASCENSION_NO_STANDARD_ENCODINGS
	class UTF_7 : public EncoderFactoryBase {
	public:
		UTF_7() : EncoderFactoryBase("UTF-7", standard::UTF_7, "Unicode (UTF-7)", 8) {}
	private:
		auto_ptr<Encoder> create() const throw() {return auto_ptr<Encoder>(new InternalEncoder<UTF_7>(*this));}
	} utf7;
	class UTF_32LE : public EncoderFactoryBase {
	public:
		UTF_32LE() : EncoderFactoryBase("UTF-32LE", standard::UTF_32LE, "Unicode (UTF-32LE)", 4) {}
	private:
		auto_ptr<Encoder> create() const throw() {return auto_ptr<Encoder>(new InternalEncoder<UTF_32LE>(*this));}
	} utf32le;
	class UTF_32BE : public EncoderFactoryBase {
	public:
		UTF_32BE() : EncoderFactoryBase("UTF-32BE", standard::UTF_32BE, "Unicode (UTF-32BE)", 4) {}
	private:
		auto_ptr<Encoder> create() const throw() {return auto_ptr<Encoder>(new InternalEncoder<UTF_32BE>(*this));}
	} utf32be;
#endif /* !ASCENSION_NO_STANDARD_ENCODINGS */
#ifndef ASCENSION_NO_MINORITY_ENCODINGS
	class UTF_5 : public EncoderFactoryBase {
	public:
		UTF_5() : EncoderFactoryBase("UTF-5", MIB_OTHER, "Unicode (UTF-5)", 6) {}
	private:
		auto_ptr<Encoder> create() const throw() {return auto_ptr<Encoder>(new InternalEncoder<UTF_5>(*this));}
	} utf5;
#endif /* !ASCENSION_NO_MINORITY_ENCODINGS */
	class UnicodeDetector : public EncodingDetector {
	public:
		UnicodeDetector() : EncodingDetector("UnicodeAutoDetect") {}
	private:
		pair<MIBenum, string> doDetect(const byte* first, const byte* last, ptrdiff_t* convertibleBytes) const throw();
	};

	struct EncoderInstaller {
		EncoderInstaller() throw() {
			Encoder::registerFactory(utf8);
			Encoder::registerFactory(utf16le);
			Encoder::registerFactory(utf16be);
#ifndef ASCENSION_NO_STANDARD_ENCODINGS
			Encoder::registerFactory(utf7);
			Encoder::registerFactory(utf32le);
			Encoder::registerFactory(utf32be);
#endif /* !ASCENSION_NO_STANDARD_ENCODINGS */
#ifndef ASCENSION_NO_MINORITY_ENCODINGS
			Encoder::registerFactory(utf5);
#endif /* !ASCENSION_NO_MINORITY_ENCODINGS */
			EncodingDetector::registerDetector(auto_ptr<EncodingDetector>(new UnicodeDetector));
		}
	} installer;
} // namespace @0


// UTF-8 ////////////////////////////////////////////////////////////////////

namespace {
	/*
		well-formed UTF-8 first byte distribution (based on Unicode 5.0 Table 3.7)
		value  1st-byte   code points       byte count
		----------------------------------------------
		10     00..7F     U+0000..007F      1
		21     C2..DF     U+0080..07FF      2
		32     E0         U+0800..0FFF      3
		33     E1..EC     U+1000..CFFF      3
		34     ED         U+D000..D7FF      3
		35     EE..EF     U+E000..FFFF      3
		46     F0         U+10000..3FFFF    4
		47     F1..F3     U+40000..FFFFF    4
		48     F4         U+100000..10FFFF  4
		09     otherwise  ill-formed        (0)
	 */
	const byte UTF8_WELL_FORMED_FIRST_BYTES[] = {
		0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,	// 0x80
		0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,	// 0x90
		0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,	// 0xA0
		0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,	// 0xB0
		0x09, 0x09, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21,	// 0xC0
		0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21,	// 0xD0
		0x32, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x34, 0x35, 0x35,	// 0xE0
		0x46, 0x47, 0x47, 0x47, 0x48, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09	// 0xF0
	};

	inline bool writeSurrogatePair(byte*& to, byte* toEnd, Char high, Char low) {
		if(to + 3 >= toEnd)
			return false;
		// 0000 0000  000w wwxx  xxxx yyyy  yyzz zzzz -> 1111 0www  10xx xxxx  10yy yyyy 10zz zzzz
		const CodePoint c = surrogates::decode(high, low);
		(*to++) = 0xF0 | mask8Bit((c & 0x001C0000U) >> 18);
		(*to++) = 0x80 | mask8Bit((c & 0x0003F000U) >> 12);
		(*to++) = 0x80 | mask8Bit((c & 0x00000FC0U) >> 6);
		(*to++) = 0x80 | mask8Bit((c & 0x0000003FU) >> 0);
		return true;
	}
} // namespace @0

template<> Encoder::Result InternalEncoder<UTF_8>::doFromUnicode(
		byte* to, byte* toEnd, byte*& toNext, const Char* from, const Char* fromEnd, const Char*& fromNext) {
	for(; to < toEnd && from < fromEnd; ++from) {
		if(*from < 0x0080U)	// 0000 0000  0zzz zzzz -> 0zzz zzzz
			(*to++) = mask8Bit(*from);
		else if(*from < 0x0800U) {	// 0000 0yyy  yyzz zzzz -> 110y yyyy  10zz zzzz
			if(to + 1 >= toEnd)
				break;
			(*to++) = 0xC0 | mask8Bit(*from >> 6);
			(*to++) = 0x80 | mask8Bit(*from & 0x003FU);
		} else if(surrogates::isHighSurrogate(*from)) {
			if(from + 1 == fromEnd) {
				toNext = to;
				fromNext = from;
				return COMPLETED;
			} else if(surrogates::isLowSurrogate(from[1])) {
				if(!writeSurrogatePair(to, toEnd, from[0], from[1]))
					break;
				++from;
			} else {
				toNext = to;
				fromNext = from;
				return MALFORMED_INPUT;
			}
		} else {	// xxxx yyyy  yyzz zzzz -> 1110 xxxx  10yy yyyy  10zz zzzz
			if(to + 2 >= toEnd)
				break;
			(*to++) = 0xE0 | mask8Bit((*from & 0xF000U) >> 12);
			(*to++) = 0x80 | mask8Bit((*from & 0x0FC0U) >> 6);
			(*to++) = 0x80 | mask8Bit((*from & 0x003FU) >> 0);
		}
	}
	fromNext = from;
	toNext = to;
	return (from == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
}

template<> Encoder::Result InternalEncoder<UTF_8>::doToUnicode(
		Char* to, Char* toEnd, Char*& toNext, const byte* from, const byte* fromEnd, const byte*& fromNext) {
	while(to < toEnd && from < fromEnd) {
		if(*from < 0x80)
			*(to++) = *(from++);
		else {
			const byte v = UTF8_WELL_FORMED_FIRST_BYTES[*from - 0x80];
			// check the source buffer length
			ptrdiff_t bytes = (v >> 4);
			if(fromEnd - from < bytes) {
				toNext = to;
				fromNext = from;
				return COMPLETED;
			}
			// check the second byte
			switch(v & 0x0F) {
			case 1: case 3: case 5: case 7:
				if(from[1] < 0x80 || from[1] > 0xBF) bytes = 0; break;
			case 2:	if(from[1] < 0xA0 || from[1] > 0xBF) bytes = 0; break;
			case 4: if(from[1] < 0x80 || from[1] > 0x9F) bytes = 0; break;
			case 6: if(from[1] < 0x90 || from[1] > 0xBF) bytes = 0; break;
			case 8: if(from[1] < 0x80 || from[1] > 0x8F) bytes = 0; break;
			}
			// check the third byte
			if(bytes >= 3 && (from[2] < 0x80 || from[2] > 0xBF)) bytes = 0;
			// check the forth byte
			if(bytes >= 4 && (from[3] < 0x80 || from[3] > 0xBF)) bytes = 0;

			if(bytes == 0) {
				toNext = to;
				fromNext = from;
				return MALFORMED_INPUT;
			}

			// decode
			CodePoint cp;
			assert(bytes >= 2 && bytes <= 4);
			switch(bytes) {
			case 2:	// 110y yyyy  10zz zzzz -> 0000 0yyy yyzz zzzz
				cp = ((from[0] & 0x1F) << 6) | ((from[1] & 0x3F) << 0); break;
			case 3:	// 1110 xxxx  10yy yyyy  10zz zzzz -> xxxx yyyy yyzz zzzz
				cp = ((from[0] & 0x0F) << 12) | ((from[1] & 0x3F) << 6) | ((from[2] & 0x3F) << 0); break;
			case 4:	// 1111 0www  10xx xxxx  10yy yyyy  10zz zzzz -> 0000 0000 000w wwxx xxxx yyyy yyzz zzzz
				cp = ((from[0] & 0x07) << 18) | ((from[1] & 0x3F) << 12) | ((from[2] & 0x3F) << 6) | ((from[3] & 0x3F) << 0); break;
			}

			if(to == toEnd - 1 && surrogates::isSupplemental(cp)) {
				fromNext = from;
				toNext = to;
				return INSUFFICIENT_BUFFER;
			}
			surrogates::encode(cp, to);
			to += surrogates::isSupplemental(cp) ? 2 : 1;
			from += bytes;
		}

	}
	fromNext = from;
	toNext = to;
	return (from == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
}


// UTF-16LE /////////////////////////////////////////////////////////////////

template<> Encoder::Result InternalEncoder<UTF_16LE>::doFromUnicode(
		byte* to, byte* toEnd, byte*& toNext, const Char* from, const Char* fromEnd, const Char*& fromNext) {
	for(; to < toEnd - 1 && from < fromEnd; ++from) {
		*(to++) = static_cast<byte>((*from & 0x00FFU) >> 0);
		*(to++) = static_cast<byte>((*from & 0xFF00U) >> 8);
	}
	fromNext = from;
	toNext = to;
	return (from == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
}

template<> Encoder::Result InternalEncoder<UTF_16LE>::doToUnicode(
		Char* to, Char* toEnd, Char*& toNext, const byte* from, const byte* fromEnd, const byte*& fromNext) {
	for(; to < toEnd && from < fromEnd - 1; from += 2)
		*(to++) = *from | maskUCS2(from[1] << 8);
	fromNext = from;
	toNext = to;
	if(from == fromEnd)
		return COMPLETED;
	else
		return (to >= toEnd - 1) ? INSUFFICIENT_BUFFER : UNMAPPABLE_CHARACTER;
}


// UTF-16BE /////////////////////////////////////////////////////////////////

template<> Encoder::Result InternalEncoder<UTF_16BE>::doFromUnicode(
		byte* to, byte* toEnd, byte*& toNext, const Char* from, const Char* fromEnd, const Char*& fromNext) {
	for(; to < toEnd - 1 && from < fromEnd; ++from) {
		*(to++) = static_cast<byte>((*from & 0xFF00U) >> 8);
		*(to++) = static_cast<byte>((*from & 0x00FFU) >> 0);
	}
	fromNext = from;
	toNext = to;
	return (from == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
}

template<> Encoder::Result InternalEncoder<UTF_16BE>::doToUnicode(
		Char* to, Char* toEnd, Char*& toNext, const byte* from, const byte* fromEnd, const byte*& fromNext) {
	for(; to < toEnd && from < fromEnd - 1; from += 2)
		*(to++) = maskUCS2(*from << 8) | from[1];
	fromNext = from;
	toNext = to;
	if(from == fromEnd)
		return COMPLETED;
	else
		return (to >= toEnd - 1) ? INSUFFICIENT_BUFFER : UNMAPPABLE_CHARACTER;
}


#ifndef ASCENSION_NO_STANDARD_ENCODINGS

// UTF-32LE /////////////////////////////////////////////////////////////////

template<> Encoder::Result InternalEncoder<UTF_32LE>::doFromUnicode(
		byte* to, byte* toEnd, byte*& toNext, const Char* from, const Char* fromEnd, const Char*& fromNext) {
	for(; to < toEnd - 3 && from < fromEnd; ++from) {
		const CodePoint c = surrogates::decodeFirst(from, fromEnd);
		if(!isScalarValue(c)) {
			toNext = to;
			fromNext = from;
			if(surrogates::isHighSurrogate(c) && from == fromEnd - 1)	// low surrogate may appear immediately
				return COMPLETED;
			return MALFORMED_INPUT;
		}
		*(to++) = mask8Bit((c & 0x000000FFU) >> 0);
		*(to++) = mask8Bit((c & 0x0000FF00U) >> 8);
		*(to++) = mask8Bit((c & 0x00FF0000U) >> 16);
		*(to++) = mask8Bit((c & 0xFF000000U) >> 24);
		if(surrogates::isSupplemental(c))
			++from;
	}
	fromNext = from;
	toNext = to;
	return (from == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
}

template<> Encoder::Result InternalEncoder<UTF_32LE>::doToUnicode(
		Char* to, Char* toEnd, Char*& toNext, const byte* from, const byte* fromEnd, const byte*& fromNext) {
	for(; to < toEnd && from < fromEnd - 3; from += 4) {
		const CodePoint c = from[0] + (from[1] << 8) + (from[2] << 16) + (from[3] << 24);
		if(isValidCodePoint(c)) {
			if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTER)
				*(to++) = REPLACEMENT_CHARACTER;
			else if(substitutionPolicy() != IGNORE_UNMAPPABLE_CHARACTER) {
				fromNext = from;
				toNext = to;
				return UNMAPPABLE_CHARACTER;
			}
		} else
			to += surrogates::encode(c, to);
	}
	fromNext = from;
	toNext = to;
	return (from == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
}


// UTF-32BE /////////////////////////////////////////////////////////////////

template<> Encoder::Result InternalEncoder<UTF_32BE>::doFromUnicode(
		byte* to, byte* toEnd, byte*& toNext, const Char* from, const Char* fromEnd, const Char*& fromNext) {
	for(; to < toEnd - 3 && from < fromEnd; ++from) {
		const CodePoint c = surrogates::decodeFirst(from, fromEnd);
		*(to++) = mask8Bit((c & 0xFF000000U) >> 24);
		*(to++) = mask8Bit((c & 0x00FF0000U) >> 16);
		*(to++) = mask8Bit((c & 0x0000FF00U) >> 8);
		*(to++) = mask8Bit((c & 0x000000FFU) >> 0);
		if(surrogates::isSupplemental(c))
			++from;
	}
	fromNext = from;
	toNext = to;
	return (from == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
}

template<> Encoder::Result InternalEncoder<UTF_32BE>::doToUnicode(
		Char* to, Char* toEnd, Char*& toNext, const byte* from, const byte* fromEnd, const byte*& fromNext) {
	for(; to < toEnd && from < fromEnd - 3; from += 4) {
		const CodePoint cp = from[3] + (from[2] << 8) + (from[1] << 16) + (from[0] << 24);
		if(isValidCodePoint(cp)) {
			if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTER)
				*(to++) = REPLACEMENT_CHARACTER;
			else if(substitutionPolicy() != IGNORE_UNMAPPABLE_CHARACTER) {
				fromNext = from;
				toNext = to;
				return UNMAPPABLE_CHARACTER;
			}
		} else
			to += surrogates::encode(cp, to);
	}
	fromNext = from;
	toNext = to;
	return (from == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
}


// UTF-7 ////////////////////////////////////////////////////////////////////

template<> Encoder::Result InternalEncoder<UTF_7>::doFromUnicode(
		byte* to, byte* toEnd, byte*& toNext, const Char* from, const Char* fromEnd, const Char*& fromNext) {
	static const byte SET_D[0x80] = {
		// 1 : in set D, 2 : '=', 3 : direct encodable but not set D, 0 : otherwise
		0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 0, 0, 3, 0, 0,	// 0x00
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	// 0x10
		3, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 2, 1, 1, 1, 1,	// 0x20
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1,	// 0x30
		0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,	// 0x40
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,	// 0x50
		0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,	// 0x60
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0	// 0x70
	};
	static const byte BASE64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	// encodingState_ == 1 if in BASE64. 0 otherwise
	for(; to < toEnd && from < fromEnd; ++from) {
		const byte klass = (*from < 0x80) ? SET_D[*from] : 0;
		if((klass & 1) == 1) {
			// encode directly (ascension puts '-' explicitly even if klass is 3)
			if(encodingState_ == 1) {
				*to = '-';
				encodingState_ = 0;
				if(++to == toEnd)
					break;	// the destination buffer is insufficient
			}
			*(to++) = mask8Bit(*from);
		} else if(klass == 2) {
			// '+' => '+-'
			if(to + 1 == toEnd)
				break;	// the destination buffer is insufficient
			*(to++) = '+';
			*(to++) = '-';
		} else {
			// modified BASE64 encode
			if(encodingState_ == 0) {
				*to = '+';
				encodingState_ = 1;
				if(++to == toEnd)
					break;	// the destination buffer is insufficient
			}
			// first, determine how many source characters can be encoded
			ptrdiff_t encodables = 1;
			if(from + 1 < fromEnd && (from[1] >= 0x80 || SET_D[from[1]] == 0)) {
				++encodables;
				if(from + 2 < fromEnd && (from[2] >= 0x80 || SET_D[from[2]] == 0))
					++encodables;
			}
			// check the size of the destination buffer
			switch(encodables) {
			case 3: if(to + 8 >= toEnd) encodables = 0; break;
			case 2: if(to + 6 >= toEnd) encodables = 0; break;
			case 1: if(to + 3 >= toEnd) encodables = 0; break;
			}
			if(encodables == 0)
				break;	// the destination buffer is insufficient

			// encode
			const Char utf16[3] = {from[0], (encodables > 1) ? from[1] : 0, (encodables > 2) ? from[2] : 0};
			*(to++) = BASE64[utf16[0] >> 10];
			*(to++) = BASE64[(utf16[0] >> 4) & 0x3F];
			*(to++) = BASE64[(utf16[0] << 2 | utf16[1] >> 14) & 0x3F];
			if(encodables >= 2) {
				*(to++) = BASE64[(utf16[1] >> 8) & 0x3F];
				*(to++) = BASE64[(utf16[1] >> 2) & 0x3F];
				*(to++) = BASE64[(utf16[1] << 4 | utf16[2] >> 12) & 0x3F];
				if(encodables >= 3) {
					*(to++) = BASE64[(utf16[2] >> 6) & 0x3F];
					*(to++) = BASE64[utf16[2] & 0x3F];
				}
			}
			from += encodables - 1;
		}
	}
	if(from == fromEnd && !flags().has(FROMEND_IS_NOT_EOB) && to != toEnd)
		*(to++) = '-';
	toNext = to;
	fromNext = from;
	return (from == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
}
		
template<> Encoder::Result InternalEncoder<UTF_7>::doToUnicode(
		Char* to, Char* toEnd, Char*& toNext, const byte* from, const byte* fromEnd, const byte*& fromNext) {
	static const byte SET_B[0x80] = {
		// 1 : in set B, 2 : '+', 3 : directly appearable in BASE64, 4 : '-', 0 : otherwise
		0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 0, 0, 3, 0, 0,	// 0x00
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	// 0x10
		3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 4, 0, 1,	// 0x20
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,	// 0x30
		0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,	// 0x40
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,	// 0x50
		0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,	// 0x60
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0	// 0x70
	};
	static const byte BASE64[0x80] = {
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	// <00>
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	// <10>
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x3E, 0xFF, 0xFF, 0xFF, 0x3F,	//  !"#$%&'()*+,-./
		0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	// 0123456789:;<=>?
		0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E,	// @ABCDEFGHIJKLMNO
		0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	// PQRSTUVWXYZ[\]^_
		0xFF, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,	// `abcdefghijklmno
		0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30,	0x31, 0x32, 0x33, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF	// pqrstuvwxyz{|}~
	};

	// decodingState_ == 1 if in BASE64. 0 otherwise
	while(to < toEnd && from < fromEnd) {
		if(*from >= 0x80 || SET_B[*from] == 0) {
			toNext = to;
			fromNext = from;
			return MALFORMED_INPUT;
		}
		const byte klass = SET_B[*from];
		if(klass == 2) {
			// '+'
			if(from + 1 == fromEnd) {	// the input is terminated by '+'...
				if(!flags().has(FROMEND_IS_NOT_EOB)) {
					toNext = to;
					fromNext = from;
					return COMPLETED;
				} else {
					decodingState_ = 1;
					++from;
					break;
				}
			} else if(from[1] == '-') {
				// '+-' => '+'
				*(to++) = L'+';
				from += 2;
			} else {
				decodingState_ = 1;	// introduce modified BASE64 sequence
				++from;
			}
		} else if(klass == 3) {
			(*to++) = (*from++);
			decodingState_ = 0;	// terminate modified BASE64 implicitly
		} else if(klass == 4) {
			// '-'
			if(decodingState_ == 1)
				decodingState_;
			else {
				toNext = to;
				fromNext = from;
				return MALFORMED_INPUT;	// '-' can't appear here
				// ...this can't handle '-' appeared at the exact beginning of the input buffer
			}
			++from;
		} else {
			// first, determine how many bytes can be decoded
			ptrdiff_t decodables = 1;
			for(const ptrdiff_t minimum = min<ptrdiff_t>(fromEnd - from, 8); decodables < minimum; ++decodables) {
				if(BASE64[from[decodables]] == 0xFF)
					break;
			}
			// check the size of the destination buffer
			switch(decodables) {
			case 8: if(to + 2 >= toEnd) decodables = 0; break;
			case 6: if(to + 1 >= toEnd) decodables = 0; break;
			case 3: break;
			default:
				toNext = to;
				fromNext = from;
				return MALFORMED_INPUT;	// invalid modified BASE64 sequence
			}
			if(decodables == 0)
				break;	// the destination buffer is insufficient

			// decode
			*(to++) = BASE64[from[0]] << 10 | BASE64[from[1]] << 4 | BASE64[from[2]] >> 2;
			if(decodables >= 6) {
				*(to++) = maskUCS2(BASE64[from[2]] << 14) | BASE64[from[3]] << 8 | BASE64[from[4]] << 2 | BASE64[from[5]] >> 4;
				if(decodables >= 8)
					*(to++) = BASE64[from[5]] << 12 | BASE64[from[6]] << 6 | BASE64[from[7]];
			}
			from += decodables;			
		}
	}
	toNext = to;
	fromNext = from;
	return (from == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
}

#endif /* !ASCENSION_NO_STANDARD_ENCODINGS */

#ifndef ASCENSION_NO_MINORITY_ENCODINGS

// UTF-5 ////////////////////////////////////////////////////////////////////

namespace {
	/**
	 * Transcodes the given UTF-5 sequence into a Unicode character.
	 * @param first the beginning of the sequence
	 * @param last the end of the sequence
	 * @param[out] the code point of the decoded character
	 * @return the end of the eaten subsequence
	 */
	inline const byte* decodeUTF5Character(const byte* first, const byte* last, CodePoint& cp) throw() {
		if(*first < 'G' || *first > 'V')
			return 0;
		cp = *first - 'G';
		for(++first; first < last; ++first) {
			if(*first >= '0' && *first <= '9') {
				cp <<= 4;
				cp |= *first - '0';
			} else if(*first >= 'A' && *first <= 'F'){
				cp <<= 4;
				cp |= *first - 'A' + 0x0A;
			} else
				break;
		}
		return first;
	}

	/**
	 * Transcodes the given Unicode character into UTF-5.
	 * @param from the beginning of the sequence
	 * @param fromEnd the end of the sequence
	 * @param[out] to beginning of the destination buffer
	 * @param[out] toEnd the end of the destination buffer
	 * @return the end of the eaten subsequence
	 */
	inline byte* encodeUTF5Character(const Char* from, const Char* fromEnd, byte* to) {
#define D2C(n) (mask8Bit(n) < 0x0A) ? (mask8Bit(n) + '0') : (mask8Bit(n) - 0x0A + 'A')

		const CodePoint cp = surrogates::decodeFirst(from, fromEnd);
		if(cp < 0x00000010U)
			*(to++) = mask8Bit((cp & 0x0000000FU) >> 0) + 'G';
		else if(cp < 0x00000100U) {
			*(to++) = mask8Bit((cp & 0x000000F0U) >> 4) + 'G';
			*(to++) = D2C((cp & 0x0000000FU) >> 0);
		} else if(cp < 0x00001000U) {
			*(to++) = mask8Bit((cp & 0x00000F00U) >> 8) + 'G';
			*(to++) = D2C((cp & 0x000000F0U) >> 4);
			*(to++) = D2C((cp & 0x0000000FU) >> 0);
		} else if(cp < 0x00010000U) {
			*(to++) = mask8Bit((cp & 0x0000F000U) >> 12) + 'G';
			*(to++) = D2C((cp & 0x00000F00U) >> 8);
			*(to++) = D2C((cp & 0x000000F0U) >> 4);
			*(to++) = D2C((cp & 0x0000000FU) >> 0);
		} else if(cp < 0x00100000U) {
			*(to++) = mask8Bit((cp & 0x000F0000U) >> 16) + 'G';
			*(to++) = D2C((cp & 0x0000F000U) >> 12);
			*(to++) = D2C((cp & 0x00000F00U) >> 8);
			*(to++) = D2C((cp & 0x000000F0U) >> 4);
			*(to++) = D2C((cp & 0x0000000FU) >> 0);
		} else if(cp < 0x01000000U) {
			*(to++) = mask8Bit((cp & 0x00F00000U) >> 20) + 'G';
			*(to++) = D2C((cp & 0x000F0000U) >> 16);
			*(to++) = D2C((cp & 0x0000F000U) >> 12);
			*(to++) = D2C((cp & 0x00000F00U) >> 8);
			*(to++) = D2C((cp & 0x000000F0U) >> 4);
			*(to++) = D2C((cp & 0x0000000FU) >> 0);
		} else if(cp < 0x10000000U) {
			*(to++) = mask8Bit((cp & 0x0F000000U) >> 24) + 'G';
			*(to++) = D2C((cp & 0x00F00000U) >> 20);
			*(to++) = D2C((cp & 0x000F0000U) >> 16);
			*(to++) = D2C((cp & 0x0000F000U) >> 12);
			*(to++) = D2C((cp & 0x00000F00U) >> 8);
			*(to++) = D2C((cp & 0x000000F0U) >> 4);
			*(to++) = D2C((cp & 0x0000000FU) >> 0);
		} else if(cp < 0x80000000U) {
			*(to++) = mask8Bit((cp & 0xF0000000U) >> 28) + 'G';
			*(to++) = D2C((cp & 0x0F000000U) >> 24);
			*(to++) = D2C((cp & 0x00F00000U) >> 20);
			*(to++) = D2C((cp & 0x000F0000U) >> 16);
			*(to++) = D2C((cp & 0x0000F000U) >> 12);
			*(to++) = D2C((cp & 0x00000F00U) >> 8);
			*(to++) = D2C((cp & 0x000000F0U) >> 4);
			*(to++) = D2C((cp & 0x0000000FU) >> 0);
		}
		return to;
#undef D2C
	}
} // namespace @0

template<> Encoder::Result InternalEncoder<UTF_5>::doFromUnicode(
		byte* to, byte* toEnd, byte*& toNext, const Char* from, const Char* fromEnd, const Char*& fromNext) {
	byte temp[8];
	byte* e;
	for(; to < toEnd && from < fromEnd; ++from) {
		e = encodeUTF5Character(from, fromEnd, temp);
		if(e == temp) {
			if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTER)
				*(to++) = properties().substitutionCharacter();
			else if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTER)
				continue;
			else {
				fromNext = from;
				toNext = to;
				return UNMAPPABLE_CHARACTER;
			}
		} else if(e - temp > toEnd - to) {
			fromNext = from;
			toNext = to;
			return INSUFFICIENT_BUFFER;
		} else {
			memcpy(to, temp, e - temp);
			to += e - temp;
			if(e - temp >= 5)
				++from;
		}
	}
	fromNext = from;
	toNext = to;
	return (from == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
}

template<> Encoder::Result InternalEncoder<UTF_5>::doToUnicode(
		Char* to, Char* toEnd, Char*& toNext, const byte* from, const byte* fromEnd, const byte*& fromNext) {
	const byte* e;
	CodePoint cp;
	while(to < toEnd && from < fromEnd) {
		e = decodeUTF5Character(from, fromEnd, cp);
		if(e == from) {
			fromNext = from;
			toNext = to;
			return MALFORMED_INPUT;
		} else if(!isValidCodePoint(cp)) {
			if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTER) {
				cp = REPLACEMENT_CHARACTER;
				if(e == from)
					e = from + 1;
			} else if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTER) {
				++from;
				continue;
			} else {
				fromNext = from;
				toNext = to;
				return UNMAPPABLE_CHARACTER;
			}
		}
		if(to == toEnd - 1 && surrogates::isSupplemental(cp)) {
			fromNext = from;
			toNext = to;
			return INSUFFICIENT_BUFFER;
		}
		from = e;
		surrogates::encode(cp, to);
		to += surrogates::isSupplemental(cp) ? 2 : 1;
	}
	fromNext = from;
	toNext = to;
	return (from == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
}

#endif /* !ASCENSION_NO_MINORITY_ENCODINGS */

namespace {
	inline const byte* maybeUTF8(const byte* first, const byte* last) throw() {
		while(first < last) {
			if(*first == 0xC0 || *first == 0xC1 || *first >= 0xF5)
				break;
			++first;
		}
		return first;
	}

	size_t UnicodeDetector(const byte* first, const byte* last, MIBenum& mib) {
		mib = MIB_UNKNOWN;
		if(last - first >= 3 && memcmp(first, UTF8_BOM, countof(UTF8_BOM)) == 0)
			mib = fundamental::UTF_8;
		else if(last - first >= 2) {
			if(memcmp(first, UTF16LE_BOM, countof(UTF16LE_BOM)) == 0)
				mib = fundamental::UTF_16LE;
			else if(memcmp(first, UTF16BE_BOM, countof(UTF16BE_BOM)) == 0)
				mib = fundamental::UTF_16BE;
#ifndef ASCENSION_NO_STANDARD_ENCODINGS
			if(last - first >= 4) {
				if(memcmp(first, UTF32LE_BOM, countof(UTF32LE_BOM)) == 0)
					mib = standard::UTF_32LE;
				else if(memcmp(first, UTF32BE_BOM, countof(UTF32BE_BOM)) == 0)
					mib = standard::UTF_32BE;
			}
#endif /* !ASCENSION_NO_STANDARD_ENCODINGS */
		}
		if(mib != MIB_UNKNOWN)
			return last - first;
		mib = fundamental::UTF_8;
		return maybeUTF8(first, last) - first;
	}
}

/// @see EncodingDetector#doDetect
pair<MIBenum, string> UnicodeDetector::doDetect(const byte* first, const byte* last, ptrdiff_t* convertibleBytes) const throw() {
	const IEncodingProperties* result = 0;
	// first, test Unicode byte order marks
	if(last - first >= 3 && memcmp(first, UTF8_BOM, countof(UTF8_BOM)) == 0)
		result = &utf8;
	else if(last - first >= 2) {
		if(memcmp(first, UTF16LE_BOM, countof(UTF16LE_BOM)) == 0)
			result = &utf16le;
		else if(memcmp(first, UTF16BE_BOM, countof(UTF16BE_BOM)) == 0)
			result = &utf16be;
#ifndef ASCENSION_NO_STANDARD_ENCODINGS
		if(last - first >= 4) {
			if(memcmp(first, UTF32LE_BOM, countof(UTF32LE_BOM)) == 0)
				result = &utf32le;
			else if(memcmp(first, UTF32BE_BOM, countof(UTF32BE_BOM)) == 0)
				result = &utf32be;
		}
#endif /* !ASCENSION_NO_STANDARD_ENCODINGS */
	}
	if(result != 0) {
		if(convertibleBytes != 0)
			*convertibleBytes = last - first;
	} else {
		// force into UTF-8
		result = &utf8;
		if(convertibleBytes != 0)
			*convertibleBytes = maybeUTF8(first, last) - first;
	}
	return make_pair(result->mibEnum(), result->name());
}
