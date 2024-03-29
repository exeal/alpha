/**
 * @file indic.cpp
 * @author exeal
 * @date 2004-2006
 */

#include "StdAfx.h"
#include "encoder.hpp"
using namespace ascension::encodings;
using namespace std;

BEGIN_ENCODER_DEFINITION()
//	DEFINE_ENCODER_CLASS(57002, Hindi_Iscii, 4, 1)
//	DEFINE_ENCODER_CLASS(57003, Bengali_Iscii, 4)
//	DEFINE_ENCODER_CLASS(57004, Tamil_Iscii, 4)
//	DEFINE_ENCODER_CLASS(57005, Telugu_Iscii, 4)
//	DEFINE_ENCODER_CLASS(57006, Assamese_Iscii, 4)
//	DEFINE_ENCODER_CLASS(57007, Oriya_Iscii, 4)
//	DEFINE_ENCODER_CLASS(57008, Kannada_Iscii, 4)
//	DEFINE_ENCODER_CLASS(57009, Malayalam_Iscii, 4)
//	DEFINE_ENCODER_CLASS(57010, Gujarati_Iscii, 4)
//	DEFINE_ENCODER_CLASS(57011, Panjabi_Iscii, 4)
	DEFINE_ENCODER_CLASS(CPEX_HINDI_MACINTOSH, Hindi_Macintosh, 1, 1)
	DEFINE_ENCODER_CLASS(CPEX_GUJARATI_MACINTOSH, Gujarati_Macintosh, 1, 1)
	DEFINE_ENCODER_CLASS(CPEX_PANJABI_MACINTOSH, Gurmukhi_Macintosh, 1, 1)
END_ENCODER_DEFINITION()

namespace {
	const wchar_t MACINDICtoUCS_80[] = {
	/* 0x80 */	0x00D7, 0x2212, 0x2013, 0x2014, 0x2018, 0x2019, 0x2026, 0x2022,
				0x00A9, 0x00AE, 0x2122,
	};
	const uchar UCStoMACINDIC_00A9[] = {
						  0x88, 0x00, 0x00, 0x00, 0x00, 0x89,
	};
	const uchar UCStoMACINDIC_0964[] = {
											0xEA, 0x90,
	};
	const uchar UCStoMACINDIC_200E[] = {
														0xD9, 0x00,
	/* U+2010 */	0x00, 0x00, 0x00, 0x00, 0x82, 0x83, 0x00, 0x00,
					0x84, 0x85, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* U+2020 */	0x00, 0x00, 0x87, 0x00, 0x00, 0x00, 0x00, 0x86,
	};
	const wchar_t MACDEVANAGARItoUCS_90[] = {
	/* 0x90 */	0x0965, 0x0970, RP__CH, RP__CH, RP__CH, RP__CH, RP__CH, RP__CH,
				RP__CH, RP__CH, RP__CH, RP__CH, RP__CH, RP__CH, RP__CH, RP__CH,
	/* 0xA0 */	RP__CH, 0x0901, 0x0902, 0x0903, 0x0905, 0x0906, 0x0907, 0x0908,
				0x0909, 0x090A, 0x090B, 0x090E, 0x090F, 0x0910, 0x090D, 0x0912,
	/* 0xB0 */	0x0913, 0x0914, 0x0911, 0x0915, 0x0916, 0x0917, 0x0918, 0x0919,
				0x091A, 0x091B, 0x091C, 0x091D, 0x091E, 0x091F, 0x0920, 0x0921,
	/* 0xC0 */	0x0922, 0x0923, 0x0924, 0x0925, 0x0926, 0x0927, 0x0928, 0x0929,
				0x092A, 0x092B, 0x092C, 0x092D, 0x092E, 0x092F, 0x095F, 0x0930,
	/* 0xD0 */	0x0931, 0x0932, 0x0933, 0x0934, 0x0935, 0x0936, 0x0937, 0x0938,
				0x0939, 0x200E, 0x093E, 0x093F, 0x0940, 0x0941, 0x0942, 0x0943,
	/* 0xE0 */	0x0946, 0x0947, 0x0948, 0x0945, 0x094A, 0x094B, 0x094C, 0x0949,
				0x094D, 0x093C, 0x0964, RP__CH, RP__CH, RP__CH, RP__CH, RP__CH,
	/* 0xF0 */	RP__CH, 0x0966, 0x0967, 0x0968, 0x0969, 0x096A, 0x096B, 0x096C,
				0x096D, 0x096E, 0x096F,
	};
	const uchar UCStoMACDEVANAGARI_0901[] = {
	/* U+0900 */		  0xA1, 0xA2, 0xA3, 0x00, 0xA4, 0xA5, 0xA6,
					0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAE, 0xAB, 0xAC,
	/* U+0910 */	0xAD, 0xB2, 0xAF, 0xB0, 0xB1, 0xB3, 0xB4, 0xB5,
					0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD,
	/* U+0920 */	0xBE, 0xBF, 0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5,
					0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD,
	/* U+0930 */	0xCF, 0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6,
					0xD7, 0xD8, 0x00, 0x00, 0xE9, 0x00, 0xDA, 0xDB,
	/* U+0940 */	0xDC, 0xDD, 0xDE, 0xDF, 0x00, 0xE3, 0xE0, 0xE1,
					0xE2, 0xE7, 0xE4, 0xE5, 0xE6, 0xE8, 0x00, 0x00,
	/* U+0950 */	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xCE,
	};
	const uchar UCStoMACDEVANAGARI_0966[] = {
	/* U+0960 */										0xF1, 0xF2,
					0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA,
	/* U+0970 */	0x91,
	};
	const wchar_t MACGUJARATItoUCS_90[] = {
	/* 0x90 */	0x0965, RP__CH, RP__CH, RP__CH, RP__CH, RP__CH, RP__CH, RP__CH,
				RP__CH, RP__CH, RP__CH, RP__CH, RP__CH, RP__CH, RP__CH, RP__CH,
	/* 0xA0 */	RP__CH, 0x0A81, 0x0A82, 0x0A83, 0x0A85, 0x0A86, 0x0A87, 0x0A88,
				0x0A89, 0x0A8A, 0x0A8B, RP__CH, 0x0A8F, 0x0A90, 0x0A8D, RP__CH,
	/* 0xB0 */	0x0A93, 0x0A94, 0x0A91, 0x0A95, 0x0A96, 0x0A97, 0x0A98, 0x0A99,
				0x0A9A, 0x0A9B, 0x0A9C, 0x0A9D, 0x0A9E, 0x0A9F, 0x0AA0, 0x0AA1,
	/* 0xC0 */	0x0AA2, 0x0AA3, 0x0AA4, 0x0AA5, 0x0AA6, 0x0AA7, 0x0AA8, RP__CH,
				0x0AAA, 0x0AAB, 0x0AAC, 0x0AAD, 0x0AAE, 0x0AAF, RP__CH, 0x0AB0,
	/* 0xD0 */	RP__CH, 0x0AB2, 0x0AB3, RP__CH, 0x0AB5, 0x0AB6, 0x0AB7, 0x0AB8,
				0x0AB9, 0x200E, 0x0ABE, 0x0ABF, 0x0AC0, 0x0AC1, 0x0AC2, 0x0AC3,
	/* 0xE0 */	RP__CH, 0x0AC7, 0x0AC8, 0x0AC5, RP__CH, 0x0ACB, 0x0ACC, 0x0AC9,
				0x0ACD, 0x0ABC, 0x0964, RP__CH, RP__CH, RP__CH, RP__CH, RP__CH,
	/* 0xF0 */	RP__CH, 0x0AE6, 0x0AE7, 0x0AE8, 0x0AE9, 0x0AEA, 0x0AEB, 0x0AEC,
				0x0AED, 0x0AEE, 0x0AEF,
	};
	const uchar UCStoMACGUJARATI_0A81[] = {
	/* U+0A80 */		  0xA1, 0xA2, 0xA3, 0x00, 0xA4, 0xA5, 0xA6,
					0xA7, 0xA8, 0xA9, 0xAA, 0x00, 0xAE, 0x00, 0xAC,
	/* U+0A90 */	0xAD, 0xB2, 0x00, 0xB0, 0xB1, 0xB3, 0xB4, 0xB5,
					0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD,
	/* U+0AA0 */	0xBE, 0xBF, 0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5,
					0xC6, 0x00, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD,
	/* U+0AB0 */	0xCF, 0x00, 0xD1, 0xD2, 0x00, 0xD4, 0xD5, 0xD6,
					0xD7, 0xD8, 0x00, 0x00, 0xE9, 0x00, 0xDA, 0xDB,
	/* U+0AC0 */	0xDC, 0xDD, 0xDE, 0xDF, 0x00, 0xE3, 0x00, 0xE1,
					0xE2, 0xE7, 0x00, 0xE5, 0xE6, 0xE8,
	};
	const uchar UCStoMACGUJARATI_0AE6[] = {
														0xF1, 0xF2,
					0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA,
	};
	const wchar_t MACGURMUKHItoUCS_90[] = {
	/* 0x90 */	0x0A71, 0x0A5C, 0x0A73, 0x0A72, 0x0A74, RP__CH, RP__CH, RP__CH,
				RP__CH, RP__CH, RP__CH, RP__CH, RP__CH, RP__CH, RP__CH, RP__CH,
	/* 0xA0 */	RP__CH, RP__CH, 0x0A02, RP__CH, 0x0A05, 0x0A06, 0x0A07, 0x0A08,
				0x0A09, 0x0A0A, RP__CH, RP__CH, 0x0A0F, 0x0A10, RP__CH, RP__CH,
	/* 0xB0 */	0x0A13, 0x0A14, RP__CH, 0x0A15, 0x0A16, 0x0A17, 0x0A18, 0x0A19,
				0x0A1A, 0x0A1B, 0x0A1C, 0x0A1D, 0x0A1E, 0x0A1F, 0x0A20, 0x0A21,
	/* 0xC0 */	0x0A22, 0x0A23, 0x0A24, 0x0A25, 0x0A26, 0x0A27, 0x0A28, RP__CH,
				0x0A2A, 0x0A2B, 0x0A2C, 0x0A2D, 0x0A2E, 0x0A2F, RP__CH, 0x0A30,
	/* 0xD0 */	RP__CH, 0x0A32, RP__CH, RP__CH, 0x0A35, 0xF860, RP__CH, 0x0A38,
				0x0A39, 0x200E, 0x0A3E, 0x0A3F, 0x0A40, 0x0A41, 0x0A42, RP__CH,
	/* 0xE0 */	RP__CH, 0x0A47, 0x0A48, RP__CH, RP__CH, 0x0A4B, 0x0A4C, RP__CH,
				0x0A4D, 0x0A3C, 0x0964, RP__CH, RP__CH, RP__CH, RP__CH, RP__CH,
	/* 0xF0 */	RP__CH, 0x0A66, 0x0A67, 0x0A68, 0x0A69, 0x0A6A, 0x0A6B, 0x0A6C,
				0x0A6D, 0x0A6E, 0x0A6F,
	};
	const uchar UCStoMACGURMUKHI_0A02[] = {
	/* U+0A00 */				0xA2, 0x00, 0x00, 0xA4, 0xA5, 0xA6,
					0xA7, 0xA8, 0xA9, 0x00, 0x00, 0x00, 0x00, 0xAC,
	/* U+0A10 */	0xAD, 0x00, 0x00, 0xB0, 0xB1, 0xB3, 0xB4, 0xB5,
					0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD,
	/* U+0A20 */	0xBE, 0xBF, 0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5,
					0xC6, 0x00, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD,
	/* U+0A30 */	0xCF, 0x00, 0xD1, 0x00, 0x00, 0xD4, 0x00, 0x00,
					0xD7, 0xD8, 0x00, 0x00, 0xE9, 0x00, 0xDA, 0xDB,
	/* U+0A40 */	0xDC, 0xDD, 0xDE, 0x00, 0x00, 0x00, 0x00, 0xE1,
					0xE2, 0x00, 0x00, 0xE5, 0xE6, 0xE8, 0x00, 0x00,
	/* U+0A50 */	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0x00, 0x00, 0x00, 0x91, 0x00, 0x00, 0x00, 0x00,
	/* U+0A60 */	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF1, 0xF2,
					0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA,
	/* U+0A70 */	0x00, 0x90, 0x93, 0x92, 0x94,
	};
	const uchar UCStoMACGURMUKHI_F860[] = {
	/* U+F860 */	0xD5,
	};
}

#define IMPLEMENT_MACINDIC_U2A(scriptTable1, ucsBegin1, scriptTable2, ucsBegin2)	\
	CFU_CHECKARGS();																\
	const size_t len = min(srcLength, destLength);									\
	for(size_t i = 0; i < len; ++i) {												\
		if(src[i] < 0x007F) {dest[i] = BIT8_MASK(src[i]); continue;}				\
		MAP_TABLE_SB(ucsBegin1, scriptTable1);										\
		MAP_TABLE_SB(ucsBegin2, scriptTable2);										\
		MAP_TABLE_SB(0x00A9, UCStoMACINDIC_00A9);									\
		else if(src[i] == 0x00D7) {dest[i] = 0x80; continue;}						\
		MAP_TABLE_SB(0x0964, UCStoMACINDIC_0964);									\
		MAP_TABLE_SB(0x200E, UCStoMACINDIC_200E);									\
		else if(src[i] == 0x2122) {dest[i] = 0x8A; continue;}						\
		else if(src[i] == 0x2212) {dest[i] = 0x81; continue;}						\
		else dest[i] = N__A;														\
		if(dest[i] == N__A)															\
			CONFIRM_ILLEGAL_CHAR(dest[i]);											\
	}																				\
	return len

#define IMPLEMENT_MACINDIC_A2U(scriptTable)				\
	CTU_CHECKARGS();									\
	const size_t len = min(srcLength, destLength);		\
	for(size_t i = 0; i < len; ++i) {					\
		if(src[i] < 0x7F) {dest[i] = src[i]; continue;}	\
		MAP_TABLE_SB(0x90, scriptTable);				\
		MAP_TABLE_SB(0x80, MACINDICtoUCS_80);			\
		else dest[i] = REPLACEMENT_CHARACTER;			\
		if(dest[i] == REPLACEMENT_CHARACTER)			\
			CONFIRM_ILLEGAL_CHAR(dest[i]);				\
	}													\
	return len


// ヒンディー語 (Macintosh, デバナガリ文字) /////////////////////////////

size_t Encoder_Hindi_Macintosh::fromUnicode(CFU_ARGLIST) {
	IMPLEMENT_MACINDIC_U2A(UCStoMACDEVANAGARI_0901, 0x0901, UCStoMACDEVANAGARI_0966, 0x0966);
}

size_t Encoder_Hindi_Macintosh::toUnicode(CTU_ARGLIST) {
	IMPLEMENT_MACINDIC_A2U(MACDEVANAGARItoUCS_90);
}


// グジャラート語 (Macintosh) /////////////////////////////////////////////

size_t Encoder_Gujarati_Macintosh::fromUnicode(CFU_ARGLIST) {
	IMPLEMENT_MACINDIC_U2A(UCStoMACGUJARATI_0A81, 0x0A81, UCStoMACGUJARATI_0AE6, 0x0AE6);
}

size_t Encoder_Gujarati_Macintosh::toUnicode(CTU_ARGLIST) {
	IMPLEMENT_MACINDIC_A2U(MACGUJARATItoUCS_90);
}


// グジャラート語 (Macintosh) /////////////////////////////////////////////

size_t Encoder_Gurmukhi_Macintosh::fromUnicode(CFU_ARGLIST) {
	IMPLEMENT_MACINDIC_U2A(UCStoMACGURMUKHI_0A02, 0x0A02, UCStoMACGURMUKHI_F860, 0xF860);
}

size_t Encoder_Gurmukhi_Macintosh::toUnicode(CTU_ARGLIST) {
	IMPLEMENT_MACINDIC_A2U(MACGURMUKHItoUCS_90);
}


#undef IMPLEMENT_MACINDIC_U2A
#undef IMPLEMENT_MACINDIC_A2U
