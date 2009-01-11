// Multilingual.cpp
// (c) 2004-2005 exeal

#include "StdAfx.h"
#include "Encoder.h"
#include <memory>	// std::auto_ptr

using namespace Ascension::Encodings;
using namespace std;

//DEFINE_ENCODER_CLASS(CPEX_MULTILINGUAL_ISO2022_7BIT, Multilingual_Iso2022_7bit, 8);
//DEFINE_ENCODER_CLASS(CPEX_MULTILINGUAL_ISO2022_7BITSS2, Multilingual_Iso2022_7bitss2, 0);
//DEFINE_ENCODER_CLASS(CPEX_MULTILINGUAL_ISO2022_7BITSISO, Multilingual_Iso2022_7bitsiso, 0);
//DEFINE_ENCODER_CLASS(CPEX_MULTILINGUAL_ISO2022_8BITSS2, Multilingual_Iso2022_8bitss2, 0);

REGISTER_READONLY_CODEPAGE(CPEX_MULTILINGUAL_ISO2022_7BIT);
REGISTER_READONLY_CODEPAGE(CPEX_MULTILINGUAL_ISO2022_7BITSS2);
REGISTER_READONLY_CODEPAGE(CPEX_MULTILINGUAL_ISO2022_7BITSISO);
REGISTER_READONLY_CODEPAGE(CPEX_MULTILINGUAL_ISO2022_8BITSS2);

// "ESC , F" �܂��� "ESC $ , F" ��1�o�C�g96�����W���� G0 �Ɏw�����邱�Ƃ�F�߂邩
// (Mule �̃G�~�����[�V�����BECMA-35 �ł� "," �� "reserved for future standardisation" �ƂȂ��Ă���)
#define ALLOW_DESIGNATION_96_CHARSET_TO_G0

namespace {
	// ���䕶��
	const uchar	ESC = 0x1B;
	const uchar	SO = 0x1E;
	const uchar	SI = 0x1F;
	const uchar	SS2 = 0x8E;
	const uchar	SS3 = 0x8F;

	// ISO-2022 �G���R�[�_���T�|�[�g���镶���Z�b�g
	enum Iso2022Charset {
		ascii,
		iso8859_1, iso8859_2, iso8859_3, iso8859_4, iso8859_5, iso8859_6, iso8859_7, iso8859_8,
		iso8859_9, iso8859_10, /*iso8859_11,*/ /*iso8859_13,*/ iso8859_14, iso8859_15, /*iso8859_16,*/
		asmo449,
		tis620, muleLao,
		tcvn, 
		jisx0201_Kana, jisx0201_Roman, jisx0208, jisx0212, jisx0213p1, jisx0213p2,
		gb2312, big5_1, big5_2, ksc5601,
	};
} // namespace `anonymous'

/**
 *	@file	Multilingual.cpp
 *
 *	ISO-2022 �x�[�X�̃}���`�����K���G���R�[�_�̎���
 *
 *	�����̃R�[�h�y�[�W�͕����̕����W����؂�ւ��ė��p���邽�߂̊g�������������ł���
 *	ISO/IEC 2022 (ECMA-35�AJIS X0202) ��\������B���̃G���R�[�h�̕ώ�� Emacs Mule �Ŏg�p����Ă���A
 *	Unicode �ł͂Ȃ�����1�̑�����o�b�t�@����������B���{��� ISO-2022-JP-X �͂��̃T�u�Z�b�g�ł���
 *
 *	ISO-2022 ���J�o�[���镶���W���͔��ɑ����̂� Ascension �ł͂��̈ꕔ��������������B
 *	�T�|�[�g���镶���Z�b�g�� Iso2022Charset �̒�`���Q��
 *
 *	UCS �̕����͑��̕����̕����W���ɑ����Ă��邱�Ƃ�����̂ŁAUCS ���� ISO-2022
 *	�ւ̕ϊ��͌����ɂ͕s�\�����AAscension �͂��̞B�������������邽�߂̃I�v�V�������������
 *
 *	ISO-2022 �ł� Emacs Mule �Ƃ̌݊����̂��߂ɁA
 *	���ԕ��� ',' ���g����1�o�C�g96�����W���� G0 �ʂɎw�����邱�Ƃ�F�߂Ă���
 */

size_t ConvertIso2022ToUnicode(CodePage cp, wchar_t* pwszDest, size_t cchDest,
		const uchar* pszSrc, size_t cchSrc, IUnconvertableCharCallback* pCallback) {
	size_t	iSrc = 0, iDest = 0;
	Iso2022Charset	g[4] = {ascii, ascii, ascii, ascii};
	Iso2022Charset*	pGL = &g[0];
	Iso2022Charset*	pGR = &g[1];

	auto_ptr<CEncoder>	pIso8859Encoder[10];

	while(iSrc < cchSrc && iDest < cchDest) {
		if(pszSrc[iSrc] == ESC && iSrc + 2 < cchSrc) {
			bool			bMultiByte = false;
			bool			b96Charset = false;
			Iso2022Charset*	pDesignatedPlane = 0;

			// ���ԕ�������w������ʂ����߂�
			switch(pszSrc[iSrc + 1]) {
			case '(':				pDesignatedPlane = &g[0];	break;
#ifdef ALLOW_DESIGNATION_96_CHARSET_TO_G0
			case ',':				pDesignatedPlane = &g[0];	break;
#endif /* ALLOW_DESIGNATION_96_CHARSET_TO_G0 */
			case ')':	case '-':	pDesignatedPlane = &g[1];	break;
			case '*':	case '.':	pDesignatedPlane = &g[2];	break;
			case '+':	case '/':	pDesignatedPlane = &g[3];	break;
			case '$':
				if(iSrc + 3 >= cchSrc)
					break;
				bMultiByte = true;
				switch(pszSrc[iSrc + 2]) {
				case '(':				pDesignatedPlane = &g[0];	break;
#ifdef ALLOW_DESIGNATION_96_CHARSET_TO_G0
				case ',':				pDesignatedPlane = &g[0];	break;
#endif /* ALLOW_DESIGNATION_96_CHARSET_TO_G0 */
				case ')':	case '-':	pDesignatedPlane = &g[1];	break;
				case '*':	case '.':	pDesignatedPlane = &g[2];	break;
				case '+':	case '/':	pDesignatedPlane = &g[3];	break;
				}
				break;
			}

			if(pDesignatedPlane != 0) {
				b96Charset = pszSrc[iSrc + (bMultiByte ? 2 : 1)] > '+';

				// �I�[��������w�����镶���W�������߂�
				bool		bDesignated = true;
				const uchar	chTerm = pszSrc[iSrc + (bMultiByte ? 3 : 2)];
				if(!bMultiByte && !b96Charset) {		// 1 �o�C�g 94 �}�`�����W��
					switch(chTerm) {
					case '1':	*pDesignatedPlane = muleLao;		break;
					case 'B':	*pDesignatedPlane = ascii;			break;
					case 'I':	*pDesignatedPlane = jisx0201_Kana;	break;
					case 'J':	*pDesignatedPlane = jisx0201_Roman;	break;
					case 'k':	*pDesignatedPlane = asmo449;		break;
					default:	bDesignated = false;				break;
					}
				} else if(!bMultiByte && b96Charset) {	// 1 �o�C�g 96 �}�`�����W��
					switch(chTerm) {
					case 'A':	*pDesignatedPlane = iso8859_1;		break;
					case 'B':	*pDesignatedPlane = iso8859_2;		break;
					case 'C':	*pDesignatedPlane = iso8859_3;		break;
					case 'D':	*pDesignatedPlane = iso8859_4;		break;
					case 'F':	*pDesignatedPlane = iso8859_7;		break;
					case 'G':	*pDesignatedPlane = iso8859_6;		break;
					case 'H':	*pDesignatedPlane = iso8859_8;		break;
					case 'L':	*pDesignatedPlane = iso8859_5;		break;
					case 'M':	*pDesignatedPlane = iso8859_9;		break;
					case 'T':	*pDesignatedPlane = tis620;			break;
					case 'V':	*pDesignatedPlane = iso8859_10;		break;
					case 'Z':	*pDesignatedPlane = tcvn;			break;
					case '_':	*pDesignatedPlane = iso8859_15;		break;
					case 'b':	*pDesignatedPlane = iso8859_14;		break;
					default:	bDesignated = false;				break;
					}
				} else if(bMultiByte && !b96Charset) {	// ���o�C�g 94^2 �}�`�����W��
					switch(chTerm) {
					case '0':	*pDesignatedPlane = big5_1;		break;
					case '1':	*pDesignatedPlane = big5_2;		break;
					case 'A':	*pDesignatedPlane = gb2312;		break;
					case 'B':	*pDesignatedPlane = jisx0208;	break;
					case 'C':	*pDesignatedPlane = ksc5601;	break;
					case 'D':	*pDesignatedPlane = jisx0212;	break;
					case 'P':	*pDesignatedPlane = jisx0213p2;	break;
					case 'Q':	*pDesignatedPlane = jisx0213p1;	break;
					default:	bDesignated = false;			break;
					}
				} else {								// ���o�C�g 96^2 �}�`�����W��
					bDesignated = false;
				}
				if(bDesignated) {
					iSrc += (bMultiByte ? 4 : 3);
					continue;
				}
			}
		}

		const uchar	ch = pszSrc[iSrc];
		if(ch == SO) {	// SO
			pGL = &g[1]; ++iSrc; continue;
		} else if(ch == SI) {	// SI
			pGL = &g[0]; ++iSrc; continue;
		} else if(ch == SS2) {	// SS2
			pGL = pGR = &g[2]; ++iSrc; continue;
		} else if(ch == ESC && cchSrc - iSrc > 1 && pszSrc[iSrc + 1] == SS2 - 0x40) {	// SS2
			pGL = pGR = &g[2]; iSrc += 2; continue;
		} else if(ch == SS3) {	// SS3
			pGL = pGR = &g[3]; ++iSrc; continue;
		} else if(ch == ESC && cchSrc - iSrc > 1 && pszSrc[iSrc + 1] == SS3 - 0x40) {	// SS3
			pGL = pGR = &g[3]; iSrc += 2; continue;
		}

#define IMPLEMENT_ISO8859_TO_UTF16(n)											\
	if(pIso8859Encoder[n - 1].get() == 0)										\
		pIso8859Encoder[n - 1].reset(											\
			CEncoderFactory::GetInstance().CreateEncoder(CPEX_ISO8859_##n));	\
	const uchar		chAnsi = ch | 0x80;											\
	const size_t	cchConverted = pIso8859Encoder[n - 1]->ConvertToUnicode(	\
						pwszDest + iDest, cchDest - iDest,						\
						reinterpret_cast<const char*>(&chAnsi), 1, pCallback);	\
	if(cchConverted == 0)														\
		return 0;																\
	++iSrc;																		\
	iDest += cchConverted;

		const Iso2022Charset	charset = (ch < 0x80) ? *pGL : *pGR;
		if(charset == ascii) {	// ASCII
			uchar	ascii = ch;
			if(ascii >= 0x80)
				CONFIRM_ILLEGAL_CHAR(ascii);
			pwszDest[iDest++] = ascii;
			++iSrc;
		} else if(charset == iso8859_1) {	// ISO-8859-1
			IMPLEMENT_ISO8859_TO_UTF16(1);
		} else if(charset == iso8859_2) {	// ISO-8859-2
			IMPLEMENT_ISO8859_TO_UTF16(2);
		} else if(charset == iso8859_3) {	// ISO-8859-3
			IMPLEMENT_ISO8859_TO_UTF16(3);
		} else if(charset == iso8859_4) {	// ISO-8859-4
			IMPLEMENT_ISO8859_TO_UTF16(4);
		} else if(charset == iso8859_5) {	// ISO-8859-5
			IMPLEMENT_ISO8859_TO_UTF16(5);
		} else if(charset == iso8859_6) {	// ISO-8859-6
			IMPLEMENT_ISO8859_TO_UTF16(6);
		} else if(charset == iso8859_7) {	// ISO-8859-7
			IMPLEMENT_ISO8859_TO_UTF16(7);
		} else if(charset == iso8859_8) {	// ISO-8859-8
			IMPLEMENT_ISO8859_TO_UTF16(8);
		} else if(charset == iso8859_9) {	// ISO-8859-9
			IMPLEMENT_ISO8859_TO_UTF16(9);
		} else if(charset == iso8859_10) {	// ISO-8859-10
			IMPLEMENT_ISO8859_TO_UTF16(10);
		} else if(charset == gb2312) {	// GB2312
			wchar_t		ucs;	// for error
			char		sz[2] = {pszSrc[iSrc] | 0x80, pszSrc[iSrc + 1] | 0x80};
			const int	cch = ::MultiByteToWideChar(936, MB_PRECOMPOSED, sz, 2, pwszDest + iDest, 2);
			if(cch == 0) {
				CONFIRM_ILLEGAL_CHAR(ucs);
				pwszDest[iDest++] = ucs;
			} else
				iDest += cch;
			iSrc += 2;
		} else
			pwszDest[iDest++] = pszSrc[iSrc++];
	}
	return iDest;
}

// �}���`�����K�� (ISO-2022, 7�r�b�g) ///////////////////////////////////////////////////////

size_t Encoder_Multilingual_Iso2022_7bit::fromUnicode(CFU_ARGLIST) {
	CFU_CHECKARGS();
	return 0;
}

size_t Encoder_Multilingual_Iso2022_7bit::toUnicode(CTU_ARGLIST) {
	CTU_CHECKARGS();
	return convertIso2022ToUnicode(CPEX_MULTILINGUAL_ISO2022_7BIT, dest, destLength, srcLength, srcLength, callback);
}


/* [EOF] */