// Tamil.cpp
// (c) 2005 exeal

#include "StdAfx.h"
#include "Encoder.h"
using namespace Ascension::Encodings;
using namespace std;

BEGIN_ENCODER_DEFINITION()
//	DEFINE_ENCODER_CLASS(CPEX_TAMIL_TAB, Tamil_Tab, 2, 4)		// 2 <- tamil letter 'AU', 4 <- tamil letter 'SHRII'
//	DEFINE_ENCODER_CLASS(CPEX_TAMIL_TAM, Tamil_Tam, 2, 5)		// 2 <- tamil letter 'AU', 5 <- tamil letter 'KSS'
//	DEFINE_ENCODER_CLASS(CPEX_TAMIL_TSCII, Tamil_Tscii, 2, 5)	// 2 <- 3/2 <- some ligatures, 5 <- tamil letter 'KSS'
//	REGISTER_READONLY_CODEPAGE(CPEX_TAMIL_TAB);
//	REGISTER_READONLY_CODEPAGE(CPEX_TAMIL_TAM);
//	REGISTER_READONLY_CODEPAGE(CPEX_TAMIL_TSCII);
END_ENCODER_DEFINITION()

#define IN_RANGE(l, h)	(ch >= l && ch <= h)

namespace {
	const ulong	TAMtoUCS_41[] = {
		/* 0x41 */				0x0B950BBF, 0x0B990BBF, 0x0B9A0BBF, 0x0B9E0BBF, 0x0BA30BBF, 0x0BA40BBF, 0x0BA80BBF,
					0x0BAA0BBF, 0x0BAE0BBF, 0x0BAF0BBF, 0x0BB00BBF, 0x0BB20BBF, 0x0BB50BBF, 0x0BB40BBF, 0x0BB30BBF,
		/* 0x50 */	0x0BB10BBF, 0x0BA90BBF, 0x0BB80BBF, 0x0BB70BBF, 0x0B9C0BBF, 0x0BB90BBF, L"\x0B95\x0BCD\x0BB7\x0BBF", 0x0B950BC0,
					0x0B990BC0, 0x0B9A0BC0, 0x0B9E0BC0, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
		/* 0x60 */	0x60, 0x0BA30BC0, 0x0BA40BC0, 0x0BA80BC0, 0x0BAA0BC0, 0x0BAE0BC0, 0x0BAF0BC0, 0x0BB00BC0,
					0x0BB20BC0, 0x0BB50BC0, 0x0BB40BC0, 0x0BB30BC0, 0x0BB10BC0, 0x0BA90BC0, 0x0BB80BC0, 0x0BB70BC0,
		/* 0x70 */	0x0B9C0BC0, 0x0BB90BC0, L"\x0B95\x0BCD\x0BB7\x0BC0", 0x0BF3, 0x0BF4, 0x0BF5, L"\x0BB80BCD200C", L"\x0BB7\x0BCD\x200C",
					L"\x0B9C\x0BCD\x200C", L"\x0BB9\x0BCD\x200C", L"\x0B95\x0BCD\x0BB7\x0BCD\x200C", 0x7B, 0x7C, 0x7E, 0x7F,
		/* 0x80 */	RP__CH, RP__CH, L"\x0B95\x0BCD\x200C", L"\x0B99\x0BCD\x200C", L"\x0B9A\x0BCD\x200C", L"\x0B9E\x0BCD\x200C", L"\x0B9F\x0BCD\x200C", L"\x0BA3\x0BCD\x200C",
					L"\x0BA4\x0BCD\x200C", L"\x0BA8\x0BCD\x200C", L"\x0BAA\x0BCD\x200C", L"\x0BAE\x0BCD\x200C", L"\x0BAF\x0BCD\x200C", RP__CH, RP__CH, RP__CH,
		/* 0x90 */	RP__CH, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x0BF9, 0x0BF8,
					L"\x0BB0\x0BCD\x200C", L"\x0BB2\x0BCD\x200C", L"\x0BB5\x0BCD\x200C", L"\x0BB4\x0BCD\x200C", L"\x0BB3\x0BCD\x200C", RP__CH, RP__CH, L"\x0BB1\x0BCD\x200C",
		/* 0xA0 */	RP__CH, L"\x0BA9\x0BCD\x200C", 0x0BCD, 0x0BBE, 0x0BBF, RP__CH, 0x0BC0, 0x0BC1,
					0x0BC2, 0x00A9, 0x0BC6, 0x0BC7, 0x0BC8, 0x0BFA, 0x0B9F0BBF, 0x0B9F0BC0,
		/* 0xB0 */	0x0B950BC1, 0x0B990BC1, 0x0B9A0BC1, 0x0B9E0BC1, 0x0B9F0BC1, 0x0BA30BC1, 0x0BA40BC1, 0x2022,
					0x0BA80BC1, 0x0BAA0BC1, 0x0BAE0BC1, 0x0BAF0BC1, 0x0BB00BC1, 0x0BB20BC1, 0x0BB50BC1, 0x0BB40BC1,
		/* 0xC0 */	0x0BB30BC1, 0x0BB10BC1, 0x0BA90BC1, 0x0B950BC2, 0x0B990BC2, 0x0B9A0BC2, 0x0B9E0BC2, 0x0B9F0BC2,
					0x0BA30BC2, 0x0BA40BC2, RP__CH, 0x0BA80BC2, 0x0BAA0BC2, 0x0BAE0BC2, 0x0BAF0BC2, 0x0BB00BC2,
		/* 0xD0 */	0x0BF6, 0x0BF7, RP__CH, RP__CH, RP__CH, RP__CH, 0x0BB20BC2, 0x0BB50BC2,
					0x0BB40BC2, 0x0BB30BC2, 0x0BB10BC2, 0x0BA90BC2, 0x0B85, 0x0B86, 0x0B87, 0x0B88,
		/* 0xE0 */	0x0B89, 0x0B8A, 0x0B8E, 0x0B8F, 0x0B90, 0x0B92, 0x0B93, 0x0B83,
					0x0B95, 0x0B99, 0x0B9A, 0x0B9E, 0x0B9F, 0x0BA3, 0x0BA4, 0x0BA8,
		/* 0xF0 */	0x0BAA, 0x0BAE, 0x0BAF, 0x0BB0, 0x0BB2, 0x0BB5, 0x0BB4, 0x0BB3,
					0x0BB1, 0x0BA9, 0x0BB8, 0x0BB7, 0x0B9C, 0x0BB9, L"\x0B95\x0BCD\x0BB7", L"\x0BB8\x0BCD\x0BB0\x0BC0"
	};
} // namespace `anonymous'


// タミル語 (TAM) ///////////////////////////////////////////////////////////

size_t CEncoder_Tamil_Tam::ConvertFromUnicode(CFU_ARGLIST) {
	assert(false);
	return 0;
}

size_t CEncoder_Tamil_Tam::ConvertToUnicode(CTU_ARGLIST) {
	CTU_CHECKARGS();

	// 1バイトの分類 (* - used for multiple purposes)
	//	00..40 -- <ascii>
	//	41..56 -- ligated syllables (with vowel sign I)
	//	57..5A -- ligated syllables (with II)
	//	5B..60 -- <ascii>
	//	61..72 -- ligated syllables (with II)
	//	73..75 -- symbols
	//	76..7A -- ligated syllables (with sign virama and ZWNJ)
	//	7B..7F -- <ascii>
	//	80..81 -- <not defined>
	//	82..8C -- ligated syllables (with sign virama and ZWNJ)
	//	8D..90 -- <not defined>
	//	91..94 -- cp1252 symbols
	//	95     -- bullet
	//	96..97 -- symbols
	//	98..9C -- ligated syllables (with sign virama and ZWNJ)
	//	9D..9E -- <not defined>
	//	9F     -- ligated syllable RR = RRA + sign virama + ZWNJ
	//	A0     -- NBSP
	//	A1     -- ligated syllable NNN = NNNA + sign virama + ZWNJ
	//	A2..A4 -- isolated vowels (following) *
	//	A5     -- <not defined (mac bullet)>
	//	A6..A8 -- isolated vowels (following) *
	//	A9     -- copyright mark
	//	AA..AC -- isolated vowels (leading) *
	//	AD     -- number symbol
	//	AE     -- ligated syllable TTI = TTA + I
	//	AF     -- ligated syllable TTII = TTA + II
	//	B0..B6 -- (with vowel U)
	//	B7     -- bullet
	//	B8..C2 -- (with vowel U)
	//	C3..C9 -- (with vowel UU)
	//	CA     -- <not defined (mac NBSP)>
	//	CB..CF -- (with vowel UU)
	//	D0..D1 -- symbols
	//	D2..D5 -- <not defined (mac symbols)>
	//	D6..DB -- (with vowel UU)
	//	DC..E6 -- independent vowels
	//	E7     -- aytham
	//	E8..FE -- consonants *
	//	FF     -- special conjuct (SHRII)

	size_t	iSrc = 0, iDest = 0;
/*	while(iSrc < cchSrc && iDest < cchDest) {
		const uchar	ch = pszSrc[iSrc];
		if(IN_RANGE(0x41, 0x56) || ch == 0xAE) {	// consonant + I
			pwszDest[iDest++] = TAMtoUCS_41[pszSrc[iSrc++] - 0x41];
			pwszDest[iDest++] = 0x0BBF;
		} else if(IN_RANGE(0x57, 0x5A) || IN_RANGE(0x61, 0x72) || ch == 0xAF) {	// consonant + II
			pwszDest[iDest++] = TAMtoUCS_41[pszSrc[iSrc++] - 0x41];
			pwszDest[iDest++] = 0x0BC0;
		} else if(IN_RANGE(0x76, 0x7A) || IN_RANGE(0x82, 0x8C)
				|| IN_RANGE(0x98, 0x9C) || ch == 0x9F || ch == 0xA1) {	// consonant + virama
			pwszDest[iDest++] = TAMtoUCS_41[pszSrc[iSrc++] - 0x41];
			pwszDest[iDest++] = 0x0BCD;
			pwszDest[iDest++] = 0x200C;
		} else if(IN_RANGE(0xB0, 0xB6) || IN_RANGE(0xB8, 0xC2)) {	// consonant + U
			pwszDest[iDest++] = TAMtoUCS_41[pszSrc[iSrc++] - 0x41];
			pwszDest[iDest++] = 0x0BC1;
		} else if(IN_RANGE(0xC3, 0xC9) || IN_RANGE(0xCB, 0xCF)) {	// consonant + UU
			pwszDest[iDest++] = TAMtoUCS_41[pszSrc[iSrc++] - 0x41];
			pwszDest[iDest++] = 0x0BC2;
		} else if(IN_RANGE(0xAA, 0xAC)) {	// leading vowel?
			if(cchSrc - iSrc >= 3) {	// sandwiching vowel?
				if(ch == 0xAA && pszSrc[iSrc + 2] == 0xA3) {	// O?
					if(pszSrc[iSrc + 1] == 0x20 || (pszSrc[iSrc + 1] >= 0xE8 && pszSrc[iSrc + 1] <= 0xFE)) {
						pwszDest[iDest++] = TAMtoUCS_41[pszSrc[iSrc + 1] - 0x41];
						pwszDest[iDest++] = 0x0BCA;
						continue;
					}
				} else if(ch == 0xAB && pszSrc[iSrc + 2] == 0xA3) {	// OO?
					if(pszSrc[iSrc + 1] == 0x20 || (pszSrc[iSrc + 1] >= 0xE8 && pszSrc[iSrc + 1] <= 0xFE)) {
						pwszDest[iDest++] = TAMtoUCS_41[pszSrc[iSrc + 1] - 0x41];
						pwszDest[iDest++] = 0x0BCA;
						continue;
					}
				}
			}
		}
	}
*/	return iDest;
}

#undef IN_RANGE

/* [EOF] */