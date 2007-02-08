/**
 * @file irish.cpp
 * @author exeal
 * @date 2005-2007
 *
 * Implementation of I.S. 434:1999 (for Irelandic). For details of this encoding,
 * see http://www.evertype.com/standards/iso10646/pdf/is434.pdf.
 *
 * In this implementation, native 0xFD..0xFF characters are mapped to UCS U+169D..169F.
 * And also non-Ogham characters shown in the above document are mapped.
 */

#include "stdafx.h"
#ifndef ASCENSION_NO_EXTENDED_ENCODINGS
#include "../encoder.hpp"
using namespace ascension::encodings;
using namespace std;


BEGIN_ENCODER_DEFINITION()
	DEFINE_ENCODER_CLASS(CPEX_IRISH_IS434, Irish_Is434, 1, 1)
END_ENCODER_DEFINITION()


// ƒAƒCƒ‹ƒ‰ƒ“ƒhŒê (I.S. 434:1999) ///////////////////////////////////////////

size_t Encoder_Irish_Is434::fromUnicode(CFU_ARGLIST) {
	CFU_CHECKARGS();

	const size_t len = min(srcLength, destLength);
	for(size_t i = 0; i < len; ++i) {
		if(src[i] >= 0x1680 && src[i] < 0x16A0)	dest[i] = BIT8_MASK(src[i] - 0x15A0);
		else if(src[i] < 0xA0)					dest[i] = BIT8_MASK(src[i]);
		else {
			switch(src[i]) {
			case 0x00A0:	case 0x00A3:	case 0x00A7:	case 0x00A9:	case 0x00AE:
			case 0x00B0:	case 0x00B1:	case 0x00B6:	case 0x00B7:
				dest[i] = BIT8_MASK(src[i]);
				break;
			default:
				CONFIRM_ILLEGAL_CHAR(dest[i]);
			}
		}
	}
	return len;
}

size_t Encoder_Irish_Is434::toUnicode(CTU_ARGLIST) {
	CTU_CHECKARGS();

	const size_t len = min(srcLength, destLength);
	for(size_t i = 0; i < len; ++i) {
		if(src[i] >= 0xE0)			dest[i] = src[i] + 0x15A0;
		else if(src[i] < 0xA0)		dest[i] = src[i];
		else {
			switch(src[i]) {
			case 0xA0:	case 0xA3:	case 0xA7:	case 0xA9:	case 0xAE:
			case 0xB0:	case 0xB1:	case 0xB6:	case 0xB7:
				dest[i] = src[i];
				break;
			default:
				CONFIRM_ILLEGAL_CHAR(dest[i]);
			}
		}
	}
	return len;
}

#endif /* !ASCENSION_NO_EXTENDED_ENCODINGS */
