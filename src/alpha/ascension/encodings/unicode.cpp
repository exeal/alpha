/**
 * @file unicode.cpp
 * This file defines seven UTF encoders. It includes: UTF-8, UTF-16 little endian, UTF-16 big
 * endian, UTF-32 little endian, UTF-32 big endian, UTF-7, and UTF-5. The last two will be
 * defined only if the configuration symbol @c ASCENSION_NO_EXTENDED_ENCODINGS is not defined.
 * @author exeal
 * @date 2003-2007
 */

#include "stdafx.h"
#include "../encoder.hpp"
using namespace ascension;
using namespace ascension::encodings;
using namespace ascension::unicode;
using namespace std;


BEGIN_ENCODER_DEFINITION()
	DEFINE_ENCODER_CLASS(CP_UTF8, Unicode_Utf8, 4, 1)			// max character length as in UTF-16 range
	DEFINE_ENCODER_CLASS(CPEX_UNICODE_UTF16LE, Unicode_Utf16LE, 2, 1)
	DEFINE_ENCODER_CLASS(CPEX_UNICODE_UTF16BE, Unicode_Utf16BE, 2, 1)
#ifndef ASCENSION_NO_EXTENDED_ENCODINGS
	DEFINE_ENCODER_CLASS(CPEX_UNICODE_UTF5, Unicode_Utf5, 6, 1)	// max character length as in UTF-16 range
	DEFINE_ENCODER_CLASS(CP_UTF7, Unicode_Utf7, 8, 1)
	DEFINE_ENCODER_CLASS(CPEX_UNICODE_UTF32LE, Unicode_Utf32LE, 4, 1)
	DEFINE_ENCODER_CLASS(CPEX_UNICODE_UTF32BE, Unicode_Utf32BE, 4, 1)
#endif /* !ASCENSION_NO_EXTENDED_ENCODINGS */
END_ENCODER_DEFINITION()
DEFINE_DETECTOR(CPEX_UNICODE_AUTODETECT, Unicode);


namespace {
	/// 引数を UTF-8 オクテットとみなし、それが何バイト文字かを返す
	inline size_t getByteLengthAsUTF8Char(const uchar* src, size_t len) throw() {
		if(len > 5) {	// 6 バイト文字
			// 1111 110x 10xx xxxx xxxx xxxx xxxx xxxx xxxx xxxx 10xx xxxx
			if((src[0] & 0xFE) == 0xFC && (src[1] & 0xC0) == 0x80 && (src[5] & 0xC0) == 0x80)
				return 6;
		}
		if(len > 4) {	// 5 バイト文字
			// 1111 10xx 10xx xxxx xxxx xxxx xxxx xxxx 10xx xxxx
			if((src[0] & 0xFC) == 0xF8 && (src[1] & 0xC0) == 0x80 && (src[4] & 0xC0) == 0x80)
				return 5;
		}
		if(len > 3) {	// 4 バイト文字
			// 1111 0xxx 10xx xxxx xxxx xxxx 10xx xxxx
			if((src[0] & 0xF8) == 0xF0 && (src[1] & 0xC0) == 0x80 && (src[3] & 0xC0) == 0x80)
				return 4;
		}
		if(len > 2) {	// 3 バイト文字
			// 1110 xxxx 10xx xxxx 10xx xxxx
			if((src[0] & 0xF0) == 0xE0 && (src[1] & 0xC0) == 0x80 && (src[2] & 0xC0) == 0x80)
				return 3;
		}
		if(len > 1) {	// 2 バイト文字
			// 110x xxxx 10xx xxxx
			if((src[0] & 0xE0) == 0xC0 && (src[1] & 0xC0) == 0x80)
				return 2;
		}
		// 1 バイト文字か不正
		// 0xxx xxxx
		if((src[0] & 0x80) == 0x00)
			return 1;

		return 0;
	}

	/**
	 *	文字列の先頭に UTF-16 の BOM があるか調べる
	 *	@retval 1	リトルエンディアン
	 *	@retval 2	ビッグエンディアン
	 *	@retval 0	無い
	 */
	inline size_t hasBOMOfUTF16(const uchar* src, size_t len) throw() {
		if(len < 2)
			return 0;
		if(memcmp(src, UTF16LE_BOM, 2) == 0)		return 1;
		else if(memcmp(src, UTF16BE_BOM, 2) == 0)	return 2;
		else										return 0;
	}

#ifndef ASCENSION_NO_EXTENDED_ENCODINGS
	/**
	 *	文字列の先頭に UTF-32 の BOM があるか調べる
	 *	@retval 1	リトルエンディアン
	 *	@retval 2	ビッグエンディアン
	 *	@retval 0	無い
	 */
	inline size_t hasBOMOfUTF32(const uchar* src, size_t len) throw() {
		if(len < 4)
			return 0;
		if(memcmp(src, UTF32LE_BOM, 4) == 0)		return 1;
		else if(memcmp(src, UTF32BE_BOM, 4) == 0)	return 2;
		else										return 0;
	}
#endif /* !ASCENSION_NO_EXTENDED_ENCODINGS */

	/// 文字列の先頭に UTF-8 の BOM があるかどうかを返す
	inline bool hasBOMOfUTF8(const uchar* src, size_t len) throw() {
		return len >= 3 && memcmp(src, UTF8_BOM, 3) == 0;
	}

	/// 引数が UTF-8 文字列として何文字有効かを返す
	inline size_t isUTF8String(const uchar* src, size_t len) throw() {
		size_t i = 0;
		if(hasBOMOfUTF8(src, len))
			return len;
		while(true) {
			if(i >= len)
				break;
			const size_t j = getByteLengthAsUTF8Char(src + i, len - i);
			if(j == 0)
				break;
			i += j;
		}
		return i;
	}

	void detectCodePage_Unicode(const uchar* src, size_t len, CodePage& result, size_t& convertableLength) {
		result = 0;
		convertableLength = len;
		if(hasBOMOfUTF8(src, len))
			result = CP_UTF8;
		else if(size_t n = hasBOMOfUTF16(src, len))
			result = (n == 1) ? CPEX_UNICODE_UTF16LE : CPEX_UNICODE_UTF16BE;
#ifndef ASCENSION_NO_EXTENDED_ENCODINGS
		else if(size_t n = hasBOMOfUTF32(src, len))
			result = (n == 1) ? CPEX_UNICODE_UTF32LE : CPEX_UNICODE_UTF32BE;
#endif /* !ASCENSION_NO_EXTENDED_ENCODINGS */
		if(result != 0)
			return;
		result = CP_UTF8;
		convertableLength = isUTF8String(src, len);
	}
}


// UTF-16 little endian /////////////////////////////////////////////////////

size_t Encoder_Unicode_Utf16LE::fromUnicode(CFU_ARGLIST) {
	CFU_CHECKARGS();

	size_t j = 0;
	for(size_t i = 0; i < srcLength && j + 1 < destLength; ++i) {
		dest[j++] = (src[i] & 0x00FF) >> 0;
		dest[j++] = (src[i] & 0xFF00) >> 8;
	}
	return j;
}

size_t Encoder_Unicode_Utf16LE::toUnicode(CTU_ARGLIST) {
	CTU_CHECKARGS();

	size_t j = 0;
	for(size_t i = 0; i + 1 < srcLength && j < destLength; i += 2)
		dest[j++] = src[i] | UTF16_MASK(src[i + 1] << 8);
	return j;
}


// UTF-16 big endian ////////////////////////////////////////////////////////

size_t Encoder_Unicode_Utf16BE::fromUnicode(CFU_ARGLIST) {
	CFU_CHECKARGS();

	size_t j = 0;
	for(size_t i = 0; i < srcLength && j + 1 < destLength; ++i) {
		dest[j++] = (src[i] & 0xFF00) >> 8;
		dest[j++] = (src[i] & 0x00FF) >> 0;
	}
	return j;
}

size_t Encoder_Unicode_Utf16BE::toUnicode(CTU_ARGLIST) {
	CTU_CHECKARGS();

	size_t j = 0;
	for(size_t i = 0; i + 1 < srcLength && j < destLength; i += 2)
		dest[j++] = UTF16_MASK(src[i] << 8) | src[i + 1];
	return j;
}


#ifndef ASCENSION_NO_EXTENDED_ENCODINGS

// UTF-32 little endian ////////////////////////////////////////////////////////////////////

size_t Encoder_Unicode_Utf32LE::fromUnicode(CFU_ARGLIST) {
	CFU_CHECKARGS();

	size_t j = 0;
	for(size_t i = 0; i < srcLength && j + 3 < destLength; ++i) {
		const CodePoint cp = surrogates::decodeFirst(src + i, src + srcLength);
		dest[j++] = BIT8_MASK((cp & 0x000000FF) >> 0);
		dest[j++] = BIT8_MASK((cp & 0x0000FF00) >> 8);
		dest[j++] = BIT8_MASK((cp & 0x00FF0000) >> 16);
		dest[j++] = BIT8_MASK((cp & 0xFF000000) >> 24);

		if(cp > 0xFFFF)
			++i;
	}
	return j;
}

size_t Encoder_Unicode_Utf32LE::toUnicode(CTU_ARGLIST) {
	CTU_CHECKARGS();

	size_t j = 0;
	for(size_t i = 0; i + 3 < srcLength && j < destLength; i += 4) {
		const CodePoint cp = src[i] + (src[i + 1] << 8) + (src[i + 2] << 16) + (src[i + 3] << 24);
		j += surrogates::encode(cp, dest + j);
	}
	return j;
}


// UTF-32 big endian ////////////////////////////////////////////////////////////////////

size_t Encoder_Unicode_Utf32BE::fromUnicode(CFU_ARGLIST) {
	CFU_CHECKARGS();

	size_t j = 0;
	for(size_t i = 0; i < srcLength && j + 3 < destLength; ++i) {
		const CodePoint cp = surrogates::decodeFirst(src + i, src + srcLength);

		dest[j++] = BIT8_MASK((cp & 0xFF000000) >> 24);
		dest[j++] = BIT8_MASK((cp & 0x00FF0000) >> 16);
		dest[j++] = BIT8_MASK((cp & 0x0000FF00) >> 8);
		dest[j++] = BIT8_MASK((cp & 0x000000FF) >> 0);

		if(cp > 0xFFFF)
			++i;
	}
	return j;
}

size_t Encoder_Unicode_Utf32BE::toUnicode(CTU_ARGLIST) {
	CTU_CHECKARGS();

	size_t j = 0;
	for(size_t i = 0; i + 3 < srcLength && j < destLength; i += 4) {
		const CodePoint cp = src[i + 3] + (src[i + 2] << 8) + (src[i + 1] << 16) + (src[i] << 24);
		j += surrogates::encode(cp, dest + j);
	}
	return j;
}


// UTF-5 ////////////////////////////////////////////////////////////////////

/**
 *	UTF-5 文字を UTF-16 に変換
 *	@param cp	変換後のコードポイント
 *	@param src	変換する文字へのポインタ
 *	@param len	変換する文字のバイト長さ
 *	@return		変換に使った文字のバイト長さ。0だと失敗
 */
inline size_t decodeUTF5CharToUnicode(CodePoint* cp, const uchar* src, size_t len) {
	assert(cp != 0 && src != 0);

	if(src[0] < 'G' || src[0] > 'V')
		return 0;

	size_t i = 1;
	*cp = src[0] - 'G';
	for(; i < len; ++i) {
		if(src[i] >= '0' && src[i] <= '9') {
			*cp <<= 4;
			*cp |= src[i] - '0';
		} else if(src[i] >= 'A' && src[i] <= 'F'){
			*cp <<= 4;
			*cp |= src[i] - 'A' + 0x0A;
		} else
			break;
	}
	return i;
}

/**
 *	UTF-16 文字を UTF-5 に変換
 *	@param dest	変換後の文字へのポインタ
 *	@param src	変換する文字へのポインタ
 *	@param len	変換する文字のバイト長さ
 *	@return		変換後の文字のバイト長さ。0だと失敗
 */
inline size_t encodeUnicodeCharToUTF5(uchar* dest, const wchar_t* src, size_t len) {
	assert(dest != 0 && src != 0);
#define D2C(n)	(BIT8_MASK(n) < 0x0A) ? (BIT8_MASK(n) + '0') : (BIT8_MASK(n) - 0x0A + 'A')

	const CodePoint cp = surrogates::decodeFirst(src, src + len);

	if(cp < 0x00000010) {
		dest[0] = BIT8_MASK((cp & 0x0000000F) >> 0) + 'G';
		return 1;
	} else if(cp < 0x00000100) {
		dest[0] = BIT8_MASK((cp & 0x000000F0) >> 4) + 'G';
		dest[1] = D2C((cp & 0x0000000F) >> 0);
		return 2;
	} else if(cp < 0x00001000) {
		dest[0] = BIT8_MASK((cp & 0x00000F00) >> 8) + 'G';
		dest[1] = D2C((cp & 0x000000F0) >> 4);
		dest[2] = D2C((cp & 0x0000000F) >> 0);
		return 3;
	} else if(cp < 0x00010000) {
		dest[0] = BIT8_MASK((cp & 0x0000F000) >> 12) + 'G';
		dest[1] = D2C((cp & 0x00000F00) >> 8);
		dest[2] = D2C((cp & 0x000000F0) >> 4);
		dest[3] = D2C((cp & 0x0000000F) >> 0);
		return 4;
	} else if(cp < 0x00100000) {
		dest[0] = BIT8_MASK((cp & 0x000F0000) >> 16) + 'G';
		dest[1] = D2C((cp & 0x0000F000) >> 12);
		dest[2] = D2C((cp & 0x00000F00) >> 8);
		dest[3] = D2C((cp & 0x000000F0) >> 4);
		dest[4] = D2C((cp & 0x0000000F) >> 0);
		return 5;
	} else if(cp < 0x01000000) {
		dest[0] = BIT8_MASK((cp & 0x00F00000) >> 20) + 'G';
		dest[1] = D2C((cp & 0x000F0000) >> 16);
		dest[2] = D2C((cp & 0x0000F000) >> 12);
		dest[3] = D2C((cp & 0x00000F00) >> 8);
		dest[4] = D2C((cp & 0x000000F0) >> 4);
		dest[5] = D2C((cp & 0x0000000F) >> 0);
		return 6;
	} else if(cp < 0x10000000) {
		dest[0] = BIT8_MASK((cp & 0x0F000000) >> 24) + 'G';
		dest[1] = D2C((cp & 0x00F00000) >> 20);
		dest[2] = D2C((cp & 0x000F0000) >> 16);
		dest[3] = D2C((cp & 0x0000F000) >> 12);
		dest[4] = D2C((cp & 0x00000F00) >> 8);
		dest[5] = D2C((cp & 0x000000F0) >> 4);
		dest[6] = D2C((cp & 0x0000000F) >> 0);
		return 7;
	} else if(cp < 0x80000000) {
		dest[0] = BIT8_MASK((cp & 0xF0000000) >> 28) + 'G';
		dest[1] = D2C((cp & 0x0F000000) >> 24);
		dest[2] = D2C((cp & 0x00F00000) >> 20);
		dest[3] = D2C((cp & 0x000F0000) >> 16);
		dest[4] = D2C((cp & 0x0000F000) >> 12);
		dest[5] = D2C((cp & 0x00000F00) >> 8);
		dest[6] = D2C((cp & 0x000000F0) >> 4);
		dest[7] = D2C((cp & 0x0000000F) >> 0);
		return 8;
	} else
		return 0;
#undef D2C
}

size_t Encoder_Unicode_Utf5::fromUnicode(CFU_ARGLIST) {
	CFU_CHECKARGS();

	size_t j = 0, converted = 0;
	for(size_t i = 0; i < srcLength && j < destLength; ++i) {
		converted = encodeUnicodeCharToUTF5(dest + j, src + i, srcLength - i);
		if(converted == 0)
			dest[j++] = BIT8_MASK(src[i]);
		else
			j += converted;
	}
	return j;
}

size_t Encoder_Unicode_Utf5::toUnicode(CTU_ARGLIST) {
	CTU_CHECKARGS();

	size_t j = 0, decoded;
	CodePoint cp;
	for(size_t i = 0; i < srcLength && j < destLength; ) {
		decoded = decodeUTF5CharToUnicode(&cp, src + i, srcLength- i);
		if(decoded == 0) {
			dest[j] = src[i];
			++j;
			decoded = 1;
		} else {
			surrogates::encode(cp, dest + j);
			j += (decoded >= 5) ? 2 : 1;
		}
		i += decoded;
	}
	return j;
}


// UTF-7 ////////////////////////////////////////////////////////////////////

/// UTF-7 の集合 B (修正 BASE64 エンコードに使う) の文字かを返す
inline bool isUTF7SetB(uchar ch) {
	return toBoolean(isalnum(ch)) || ch == '+' || ch == '/';
}

/// UTF-7 の集合 D (そのままコピーできる) の文字かを返す
inline bool isUTF7SetD(wchar_t ch) {
	if(ch > L'z')
		return false;
	return toBoolean(isalpha(BIT8_MASK(ch)))
			|| (ch >= L',' && ch <= L':')
			|| (ch >= L'\'' && ch <= L')')
			|| ch == L'\?' || ch == L'\t' || ch == L' ' || ch == L'\r' || ch == L'\n';
}

size_t Encoder_Unicode_Utf7::fromUnicode(CFU_ARGLIST) {
	CFU_CHECKARGS();

	static const uchar base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	size_t i = 0, j = 0;
	while(true /* i < srcLength */) {
		// 修正 BASE64 エンコードが必要な文字列長を求める
		size_t base64End = i;
		for(; base64End < srcLength; ++base64End) {
			if(isUTF7SetD(src[base64End]) || src[base64End] == L'+')
				break;
		}

		// 修正 BASE64 エンコード
		if(base64End != i) {
			dest[j++] = '+';
			while(i < base64End) {
				dest[j++] = base64[src[i] >> 10];
				dest[j++] = base64[(src[i] >> 4) & 0x3F];
				if(i + 1 >= base64End)
					dest[j++] = base64[(src[i] << 2)  & 0x3F];
				else {
					dest[j++] = base64[((src[i] << 2) | (src[i + 1] >> 14)) & 0x3F];
					dest[j++] = base64[(src[i + 1] >> 8) & 0x3F];
					dest[j++] = base64[(src[i + 1] >> 2) & 0x3F];
					if(i + 2 >= base64End)
						dest[j++] = base64[(src[i + 1] << 4) & 0x3F];
					else {
						dest[j++] = base64[((src[i + 1] << 4) | (src[i + 2]) >> 12) & 0x3F];
						dest[j++] = base64[(src[i + 2] >> 6) & 0x3F];
						dest[j++] = base64[(src[i + 2] >> 0) & 0x3F];
						++i;
					}
					++i;
				}
				++i;
			}
			dest[j++] = '-';
		}

		i = base64End;
		if(i < srcLength) {
			if(src[i] == L'+') {	// '+' -> '+-'
				dest[j++] = '+';
				dest[j++] = '-';
				++i;
			} else	// そのままコピー
				dest[j++] = BIT8_MASK(src[i++]);
		} else
			break;
	}
	return j;
}

size_t Encoder_Unicode_Utf7::toUnicode(CTU_ARGLIST) {
	CTU_CHECKARGS();

	static const uchar base64[] = {
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	//
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	//
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	//
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	//
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	//  !"#$%&'
		0xFF, 0xFF, 0xFF, 0x3E, 0xFF, 0xFF, 0xFF, 0x3F,	// ()*+,-./
		0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B,	// 01234567
		0x3C, 0x3D, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	// 89:;<=>?
		0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,	// @ABCDEFG
		0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E,	// HIJKLMNO
		0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,	// PQRSTUVW
		0x17, 0x18, 0x19, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	// XYZ[\]^_
		0xFF, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20,	// `abcdefg
		0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,	// hijklmno
		0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30,	// pqrstuvw
		0x31, 0x32, 0x33, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	// xyz{|}~
	};

	size_t i = 0, j = 0;
	bool inBase64 = false;

	while(i < srcLength) {
		if(!inBase64) {
			if(src[i] == '+') {
				if(i + 1 < srcLength && src[i + 1] == '-') {	// "+-" -> "+"
					dest[j++] = L'+';
					i += 2;
				} else {
					++i;
					inBase64 = true;
				}
			} else
				dest[j++] = src[i++];
		} else {
			// 修正 BASE64 エンコード部の長さを求める
			size_t base64End = i;
			while(base64End < srcLength && isUTF7SetB(src[base64End]))
				++base64End;

			// デコード
			while(i < base64End) {
				const size_t encodeChars = min<size_t>(base64End - i, 8);	// 1度にデコードできるバイト数
										dest[j + 0]  = base64[src[i + 0]] << 10;
				if(i + 1 < base64End)	dest[j + 0] |= base64[src[i + 1]] << 4;
				if(i + 2 < base64End)	dest[j + 0] |= base64[src[i + 2]] >> 2;
				if(i + 3 < base64End) {	dest[j + 1]  = base64[src[i + 2]] << 14;
										dest[j + 1] |= base64[src[i + 3]] << 8;}
				if(i + 4 < base64End)	dest[j + 1] |= base64[src[i + 4]] << 2;
				if(i + 5 < base64End)	dest[j + 1] |= base64[src[i + 5]] >> 4;
				if(i + 6 < base64End) {	dest[j + 2]  = base64[src[i + 5]] << 12;
										dest[j + 2] |= base64[src[i + 6]] << 6;}
				if(i + 7 < base64End)	dest[j + 2] |= base64[src[i + 7]] << 0;

				i += encodeChars;
				++j;
				if(encodeChars > 3)	++j;
				if(encodeChars > 6)	++j;
			}

			i = base64End;
			if(i < srcLength && src[i] == '-')
				++i;
			inBase64 = false;
		}
	}
	return j;
}

#endif /* !ASCENSION_NO_EXTENDED_ENCODINGS */


// UTF-8 ////////////////////////////////////////////////////////////////////

/**
 *	UTF-8 文字を UTF-16 に変換 (代替: @c MultiByteToWideChar)
 *	@param cp	[out] 変換後のコードポイント
 *	@param src	変換する文字へのポインタ
 *	@param len	変換する文字のバイト長さ
 *	@return		変換に使った文字のバイト長さ。0だと失敗
 */
inline size_t decodeUTF8CharToUnicode(CodePoint& cp, const uchar* src, size_t len) {
	assert(src != 0);

	const size_t bytes = getByteLengthAsUTF8Char(src, len);

	switch(bytes) {
/*	case 6:	// 6バイト文字
		// 1111 110u  10vv vvvv  ...  10zz zzzz -> 0uvv vvvv wwww wwxx xxxx yyyy yyzz zzzz
		cp = ((src[0] & 0x01) << 30) | ((src[1] & 0x3F) << 24) | ((src[2] & 0x3F) << 18)
				| ((src[3] & 0x3F) << 12) | ((src[4] & 0x3F) << 6) | ((src[5] & 0x3F) << 0);
		break;
	case 5:	// 5バイト文字
		// 1111 10vv  10ww wwww  ...  10zz zzzz -> 0000 00vv wwww wwxx xxxx yyyy yyzz zzzz
		cp = ((src[0] & 0x03) << 24) | ((src[1] & 0x3F) << 18)
				| ((src[2] & 0x3F) << 12) | ((src[3] & 0x3F) << 6) | ((src[4] & 0x3F) << 0);
		break;
*/	case 4:	// 4バイト文字
		// 1111 0www  10xx xxxx  10yy yyyy  10zz zzzz -> 0000 0000 000w wwxx xxxx yyyy yyzz zzzz
		cp = ((src[0] & 0x07) << 18) | ((src[1] & 0x3F) << 12) | ((src[2] & 0x3F) << 6) | ((src[3] & 0x3F) << 0);
		break;
	case 3:	// 3バイト文字
		// 1110 xxxx  10yy yyyy  10zz zzzz -> xxxx yyyy yyzz zzzz
		cp = ((src[0] & 0x0F) << 12) | ((src[1] & 0x3F) << 6) | ((src[2] & 0x3F) << 0);
		break;
	case 2:	// 2バイト文字
		// 110y yyyy  10zz zzzz -> 0000 0yyy yyzz zzzz
		cp = ((src[0] & 0x1F) << 6) | ((src[1] & 0x3F) << 0);
		break;
	case 1:	// 1バイト文字
		// 0zzz zzzz -> 0000 0000 0zzz zzzz
		cp = src[0];
		break;
	}

	return bytes;
}

/**
 *	UTF-16 文字を UTF-8 に変換 (代替: @c WideCharToMultiByte)
 *	@param dest	変換後の文字へのポインタ
 *	@param src	変換する文字へのポインタ
 *	@param len	変換する文字のバイト長さ
 *	@return		変換後の文字のバイト長さ。0だと失敗
 */
inline size_t encodeUnicodeCharToUTF8(uchar* dest, const wchar_t* src, size_t len) {
	assert(dest != 0 && len != 0);

	const CodePoint cp = surrogates::decodeFirst(src, src + len);

	if(cp <= 0x0000007F) {	// 1バイト文字
		// 0000 0000  0zzz zzzz -> 0zzz zzzz
		dest[0] = BIT8_MASK(cp);
		return 1;
	} else if(cp <= 0x000007FF) {	// 2バイト文字
		// 0000 0yyy  yyzz zzzz -> 110y yyyy  10zz zzzz
		dest[0] = 0xC0 | BIT8_MASK(cp >> 6);
		dest[1] = 0x80 | BIT8_MASK(cp & 0x003F);
		return 2;
	} else if(cp <= 0x0000FFFF) {	// 3バイト文字
		// xxxx yyyy  yyzz zzzz -> 1110 xxxx  10yy yyyy  10zz zzzz
		dest[0] = 0xE0 | BIT8_MASK((cp & 0xF000) >> 12);
		dest[1] = 0x80 | BIT8_MASK((cp & 0x0FC0) >> 6);
		dest[2] = 0x80 | BIT8_MASK((cp & 0x003F) >> 0);
		return 3;
	} else if(cp <= 0x0010FFFF) {	// 4バイト文字
		// 0000 0000  000w wwxx  xxxx yyyy  yyzz zzzz -> 1111 0www  10xx xxxx  10yy yyyy 10zz zzzz
		dest[0] = 0xF0 | BIT8_MASK((cp & 0x001C0000) >> 18);
		dest[1] = 0x80 | BIT8_MASK((cp & 0x0003F000) >> 12);
		dest[2] = 0x80 | BIT8_MASK((cp & 0x00000FC0) >> 6);
		dest[3] = 0x80 | BIT8_MASK((cp & 0x0000003F) >> 0);
		return 4;
/*	} else if(cp <= 0x03FFFFFF) {	// 5バイト文字
		// 0000 00vv  wwww wwxx  xxxx yyyy  yyzz zzzz -> 1111 10vv  10ww wwww  ...  10zz zzzz
		dest[0] = 0xF8 | BIT8_MASK((cp & 0x03000000) >> 24);
		dest[1] = 0x80 | BIT8_MASK((cp & 0x00FC0000) >> 18);
		dest[2] = 0x80 | BIT8_MASK((cp & 0x0003F000) >> 12);
		dest[3] = 0x80 | BIT8_MASK((cp & 0x00000FC0) >> 6);
		dest[4] = 0x80 | BIT8_MASK((cp & 0x0000003F) >> 0);
		return 5;
	} else if(cp <= 0x7FFFFFFF) {	// 6バイト文字
		// 0uvv vvvv  wwww wwxx  xxxx yyyy  yyzz zzzz -> 1111 110u  10vv vvvv  ...  10zz zzzz
		dest[0] = 0xFC | BIT8_MASK((cp & 0x40000000) >> 30);
		dest[1] = 0x80 | BIT8_MASK((cp & 0x3F000000) >> 24);
		dest[2] = 0x80 | BIT8_MASK((cp & 0x00FC0000) >> 18);
		dest[3] = 0x80 | BIT8_MASK((cp & 0x3F03F000) >> 12);
		dest[4] = 0x80 | BIT8_MASK((cp & 0x3F000FC0) >> 6);
		dest[5] = 0x80 | BIT8_MASK((cp & 0x3F00003F) >> 0);
		return 6;
*/	} else	// 不正
		return 0;
}

size_t Encoder_Unicode_Utf8::fromUnicode(CFU_ARGLIST) {
	CFU_CHECKARGS();

	size_t j = 0, converted;
	for(size_t i = 0; i < srcLength; ++i) {
		converted = encodeUnicodeCharToUTF8(dest + j, src + i, srcLength - i);
		if(converted == 0)
			dest[j++] = BIT8_MASK(src[i]);
		else {
			if(converted >= 4)
				++i;
			j += converted;
		}
	}
	return j;
}

size_t Encoder_Unicode_Utf8::toUnicode(CTU_ARGLIST) {
	CTU_CHECKARGS();

	size_t j = 0, decoded;
	CodePoint cp;
	for(size_t i = (srcLength > 2 && memcmp(src, UTF8_BOM, 3) == 0) ? 3 : 0; i < srcLength && j < destLength; i += decoded) {
		decoded = decodeUTF8CharToUnicode(cp, reinterpret_cast<const uchar*>(src + i), srcLength - i);
		if(decoded == 0) {
			dest[j] = src[i];
			++j;
			decoded = 1;
		} else {
			surrogates::encode(cp, dest + j);
			j += (decoded >= 4) ? 2 : 1;
		}
	}
	return j;
}
