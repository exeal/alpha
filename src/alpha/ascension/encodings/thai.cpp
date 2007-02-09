/**
 * @file thai.cpp
 * @author exeal
 * @date 2004-2007
 */

#include "stdafx.h"
#ifndef ASCENSION_NO_EXTENDED_ENCODINGS
#include "../encoder.hpp"
using namespace ascension;
using namespace ascension::encodings;
using namespace std;


BEGIN_ENCODER_DEFINITION()
	DEFINE_ENCODER_CLASS(CPEX_THAI_TIS620, Thai_Tis620, 1, 1)
	DEFINE_ENCODER_CLASS(CPEX_ISO8859_11, Iso8859_11, 1, 1)
END_ENCODER_DEFINITION()

namespace {
	template<bool iso8859>
	inline wchar_t thaiNativeCharToUcs(uchar ch) {
		if(ch < 0x80)						return ch;
		else if(ch == 0xA0)					return iso8859 ? 0x00A0 : REPLACEMENT_CHARACTER;
		else if(ch >= 0xA1 && ch <= 0xDA)	return ch + 0x0D60;
		else if(ch >= 0xDF && ch <= 0xFB)	return ch + 0x0D60;
		else								return REPLACEMENT_CHARACTER;
	}
	template<bool iso8859>
	inline uchar ucsToThaiNativeChar(wchar_t ch) {
		assert(ch != 0);
		if(ch < 0x0080)							return BIT8_MASK(ch);
		else if(ch == 0x00A0)					return iso8859 ? 0xA0 : N__A;
		else if(ch >= 0x0E01 && ch <= 0x0E3A)	return BIT8_MASK(ch - 0x0D60);
		else if(ch >= 0x0E3F && ch <= 0x0E5B)	return BIT8_MASK(ch - 0x0D60);
		else									return N__A;
	}
	template<bool iso8859>
	inline size_t unicodeToThaiNative(uchar* dest, size_t destLength,
			const wchar_t* src, size_t srcLength, IUnconvertableCharCallback* callback) {
		const size_t len = min(srcLength, destLength);
		for(size_t i = 0; i < len; ++i) {
			dest[i] = ucsToThaiNativeChar<iso8859>(src[i]);
			if(dest[i] == N__A)
				CONFIRM_ILLEGAL_CHAR(dest[i]);
		}
		return len;
	}
	template<bool iso8859>
	inline size_t thaiNativeToUnicode(wchar_t* dest, size_t destLength,
			const uchar* src, size_t srcLength, IUnconvertableCharCallback* callback) {
		const size_t len = min(srcLength, destLength);
		for(size_t i = 0; i < len; ++i) {
			dest[i] = thaiNativeCharToUcs<false>(src[i]);
			if(dest[i] == REPLACEMENT_CHARACTER)
				CONFIRM_ILLEGAL_CHAR(dest[i]);
		}
		return len;
	}
}


// タイ語 (TIS 620-2533:1990) ///////////////////////////////////////////////

size_t Encoder_Thai_Tis620::fromUnicode(CFU_ARGLIST) {
	CFU_CHECKARGS();
	return unicodeToThaiNative<false>(reinterpret_cast<uchar*>(dest), destLength, src, srcLength, callback);
}

size_t Encoder_Thai_Tis620::toUnicode(CTU_ARGLIST) {
	CTU_CHECKARGS();
	return thaiNativeToUnicode<false>(dest, destLength, src, srcLength, callback);
}


// タイ語 (ISO-8859-11) /////////////////////////////////////////////////////

size_t Encoder_Iso8859_11::fromUnicode(CFU_ARGLIST) {
	CFU_CHECKARGS();
	return unicodeToThaiNative<true>(reinterpret_cast<uchar*>(dest), destLength, src, srcLength, callback);
}

size_t Encoder_Iso8859_11::toUnicode(CTU_ARGLIST) {
	CTU_CHECKARGS();
	return thaiNativeToUnicode<true>(dest, destLength, src, srcLength, callback);
}

#endif /* !ASCENSION_NO_EXTENDED_ENCODINGS */
