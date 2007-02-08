/**
 * @file uncategorized.cpp
 * @author exeal
 * @date 2004-2007
 */

#include "stdafx.h"
#ifndef ASCENSION_NO_EXTENDED_ENCODINGS
#include "../encoder.hpp"
using namespace ascension::encodings;
using namespace std;


BEGIN_ENCODER_DEFINITION()
	DEFINE_ENCODER_CLASS(CPEX_UNCATEGORIZED_BINARY, Uncategorized_Binary, 1, 1)
	DEFINE_ENCODER_CLASS(CPEX_UNCATEGORIZED_NEXTSTEP, Uncategorized_Nextstep, 1, 1)
	DEFINE_ENCODER_CLASS(CPEX_UNCATEGORIZED_ATARIST, Uncategorized_Atarist, 1, 1)
END_ENCODER_DEFINITION()

// 以下の変換テーブルは Atari ST/TT Character Encoding (http://www.kostis.net/charsets/atarist.htm)
// 及び Unicode.org のファイル (http://www.unicode.org/Public/MAPPINGS/VENDORS/MISC/ATARIST.TXT) によった
namespace {
	const wchar_t ATARISTtoUCS_80[] = {
		/* 0x80 */	0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7,
					0x00EA, 0x00EB, 0x00E8, 0x00EF, 0x00EE, 0x00EC, 0x00C4, 0x00C5,
		/* 0x90 */	0x00C9, 0x00E6, 0x00C6, 0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9,
					0x00FF, 0x00D6, 0x00DC, 0x00A2, 0x00A3, 0x00A5, 0x00DF, 0x0192,
		/* 0xA0 */	0x00E1, 0x00ED, 0x00F3, 0x00FA, 0x00F1, 0x00D1, 0x00AA, 0x00BA,
					0x00BF, 0x2310, 0x00AC, 0x00BD, 0x00BC, 0x00A1, 0x00AB, 0x00BB,
		/* 0xB0 */	0x00E3, 0x00F5, 0x00D8, 0x00F8, 0x0153, 0x0152, 0x00C0, 0x00C3,
					0x00D5, 0x00A8, 0x00B4, 0x2020, 0x00B6, 0x00A9, 0x00AE, 0x2122,
		/* 0xC0 */	0x0133, 0x0132, 0x05D0, 0x05D1, 0x05D2, 0x05D3, 0x05D4, 0x05D5,
					0x05D6, 0x05D7, 0x05D8, 0x05D9, 0x05DB, 0x05DC, 0x05DE, 0x05E0,
		/* 0xD0 */	0x05E1, 0x05E2, 0x05E4, 0x05E6, 0x05E7, 0x05E8, 0x05E9, 0x05EA,
					0x05DF, 0x05DA, 0x05DD, 0x05E3, 0x05E5, 0x00A7, 0x2227, 0x221E,
		/* 0xE0 */	0x03B1, 0x03B2, 0x0393, 0x03C0, 0x03A3, 0x03C3, 0x00B5, 0x03C4,
					0x03A6, 0x0398, 0x03A9, 0x03B4, 0x222E, 0x03C6, 0x2208, 0x2229,
		/* 0xF0 */	0x2261, 0x00B1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00F7, 0x2248,
					0x00B0, 0x2219, 0x00B7, 0x221A, 0x207F, 0x00B2, 0x00B3, 0x00AF
	};
	const uchar UCStoATARIST_00A0[] = {
		/* U+00A0 */	N__A, 0xAD, 0x9B, 0x9C, N__A, 0x9D, N__A, 0xDD,
						0xB9, 0xBD, 0xA6, 0xAE, 0xAA, N__A, 0xBE, 0xFF,
		/* U+00B0 */	0xF8, 0xF1, 0xFD, 0xFE, 0xBA, 0xE6, 0xBC, 0xFA,
						N__A, N__A, 0xA7, 0xAF, 0xAC, 0xAB, N__A, 0xA8,
		/* U+00C0 */	0xB6, N__A, N__A, 0xB7, 0x8E, 0x8F, 0x92, 0x80,
						N__A, 0x90, N__A, N__A, N__A, N__A, N__A, N__A,
		/* U+00D0 */	N__A, 0xA5, N__A, N__A, N__A, 0xB8, 0x99, N__A,
						0xB2, N__A, N__A, N__A, 0x9A, N__A, N__A, 0x9E,
		/* U+00E0 */	0x85, 0xA0, 0x83, 0xB0, 0x84, 0x86, 0x91, 0x87,
						0x8A, 0x82, 0x88, 0x89, 0x8D, 0xA1, 0x8C, 0x8B,
		/* U+00F0 */	N__A, 0xA4, 0x95, 0xA2, 0x93, 0xB1, 0x94, 0xF6,
						0xB3, 0x97, 0xA3, 0x96, 0x81, N__A, N__A, 0x98
	};
	const uchar	UCStoATARIST_0132[] = {
		/* U+0130 */				0xC1, 0xC0
	};
	const uchar	UCStoATARIST_0152[] = {
		/* U+0150 */				0xB5, 0xB4
	};
	const uchar	UCStoATARIST_0192[] = {
		/* U+0190 */				0x9F
	};
	const uchar	UCStoATARIST_0393[] = {
		/* U+0390 */					  0xE2, N__A, N__A, N__A, N__A,
						0xE9, N__A, N__A, N__A, N__A, N__A, N__A, N__A,
		/* U+03A0 */	N__A, N__A, N__A, 0xE4, N__A, N__A, 0xE8, N__A,
						N__A, 0xEA, N__A, N__A, N__A, N__A, N__A, N__A,
		/* U+03B0 */	N__A, 0xE0, 0xE1, N__A, 0xEB, N__A, N__A, N__A,
						N__A, N__A, N__A, N__A, N__A, N__A, N__A, N__A,
		/* U+03C0 */	0xE3, N__A, N__A, 0xE5, 0xE7, N__A, 0xED
	};
	const uchar	UCStoATARIST_05D0[] = {
		/* U+05D0 */	0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9,
						0xCA, 0xCB, 0xD9, 0xCC, 0xCD, 0xDA, 0xCE, 0xD8,
		/* U+05E0 */	0xCF, 0xD0, 0xD1, 0xDB, 0xD2, 0xDC, 0xD3, 0xD4,
						0xD5, 0xD6, 0xD7
	};
	const uchar	UCStoATARIST_2020[] = {
		/* U+2020 */	0xBB
	};
	const uchar	UCStoATARIST_207F[] = {
		/* U+2078 */											  0xFC
	};
	const uchar	UCStoATARIST_2122[] = {
		/* U+2120 */				0xBF
	};
	const uchar	UCStoATARIST_2208[] = {
						0xEE, N__A, N__A, N__A, N__A, N__A, N__A, N__A,
		/* U+2210 */	N__A, N__A, N__A, N__A, N__A, N__A, N__A, N__A,
						N__A, N__A, 0xF9, 0xFB, N__A, N__A, 0xDF, N__A,
		/* U+2220 */	N__A, N__A, N__A, N__A, N__A, N__A, N__A, 0xDE,
						N__A, 0xEF, N__A, N__A, N__A, N__A, 0xEC
	};
	const uchar	UCStoATARIST_2248[] = {
						0xF7
	};
	const uchar	UCStoATARIST_2261[] = {
		/* U+2260 */		  0xF0, N__A, N__A, 0xF3, 0xF2
	};
	const uchar	UCStoATARIST_2310[] = {
		/* U+2310 */	0xA9, N__A, N__A, N__A, N__A, N__A, N__A, N__A,
						N__A, N__A, N__A, N__A, N__A, N__A, N__A, N__A,
		/* U+2120 */	0xF4, 0xF5
	};
}


// バイナリ /////////////////////////////////////////////////////////////////

size_t Encoder_Uncategorized_Binary::fromUnicode(CFU_ARGLIST) {
	CFU_CHECKARGS();
	const size_t len = min(srcLength, destLength);
	for(size_t i = 0; i < len; ++i) {
		if(src[i] > 0x00FF)
			CONFIRM_ILLEGAL_CHAR(dest[i]);
		dest[i] = BIT8_MASK(src[i]);
	}
	return len;
}

size_t Encoder_Uncategorized_Binary::toUnicode(CTU_ARGLIST) {
	CTU_CHECKARGS();
	const size_t len = min(srcLength, destLength);
	for(size_t i = 0; i < len; ++i)
		dest[i] = src[i];
	return len;
}


// NEXTSTEP /////////////////////////////////////////////////////////////////

size_t Encoder_Uncategorized_Nextstep::fromUnicode(CFU_ARGLIST) {
	CFU_CHECKARGS();
	return 0;
}

size_t Encoder_Uncategorized_Nextstep::toUnicode(CTU_ARGLIST) {
	CTU_CHECKARGS();
	return 0;
}


// Atari ST/TT //////////////////////////////////////////////////////////////

size_t Encoder_Uncategorized_Atarist::fromUnicode(CFU_ARGLIST) {
	CFU_CHECKARGS();
	const size_t len = min(srcLength, destLength);
	for(size_t i = 0; i < len; ++i) {
		if(src[i] < 0x80)
			dest[i] = BIT8_MASK(src[i]);
		else {
			MAP_TABLE_SB_START(0x00A0, UCStoATARIST_00A0);
			MAP_TABLE_SB(0x0132, UCStoATARIST_0132);
			MAP_TABLE_SB(0x0152, UCStoATARIST_0152);
			MAP_TABLE_SB(0x0192, UCStoATARIST_0192);
			MAP_TABLE_SB(0x0393, UCStoATARIST_0393);
			MAP_TABLE_SB(0x05D0, UCStoATARIST_05D0);
			MAP_TABLE_SB(0x2020, UCStoATARIST_2020);
			MAP_TABLE_SB(0x207F, UCStoATARIST_207F);
			MAP_TABLE_SB(0x2122, UCStoATARIST_2122);
			MAP_TABLE_SB(0x2208, UCStoATARIST_2208);
			MAP_TABLE_SB(0x2248, UCStoATARIST_2248);
			MAP_TABLE_SB(0x2261, UCStoATARIST_2261);
			MAP_TABLE_SB(0x2310, UCStoATARIST_2310);
			else
				dest[i] = N__A;
			if(dest[i] == N__A)
				CONFIRM_ILLEGAL_CHAR(dest[i]);
		}
	}
	return len;
}

size_t Encoder_Uncategorized_Atarist::toUnicode(CTU_ARGLIST) {
	CTU_CHECKARGS();
	const size_t len = min(srcLength, destLength);
	for(size_t i = 0; i < len; ++i)
		dest[i] = (src[i] < 0x80) ? src[i] : ATARISTtoUCS_80[src[i]];
	return len;
}

#endif /* !ASCENSION_NO_EXTENDED_ENCODINGS */
