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

#ifndef ASCENSION_NO_EXTENDED_ENCODINGS
#include "../encoder.hpp"
using namespace ascension;
using namespace ascension::encoding;
using namespace std;

// registry
namespace {
	ASCENSION_DEFINE_SBCS_ENCODER(IS434Encoder, extended::IS434, "I.S. 434")
	struct Installer {
		Installer() {
			Encoder::registerEncoder(auto_ptr<Encoder>(new IS434Encoder));
		}
	} installer;
} // namespace @0


// I.S. 434:1999 ////////////////////////////////////////////////////////////

/// @see SBCSEncoder#doFromUnicode
inline bool IS434Encoder::doFromUnicode(uchar& to, Char from) const {
	if(from >= 0x1680 && from < 0x16A0)
		to = mask8Bit(from - 0x15A0);
	else if(from < 0xA0)
		to = mask8Bit(from);
	else {
		switch(from) {
		case 0x00A0:	case 0x00A3:	case 0x00A7:	case 0x00A9:	case 0x00AE:
		case 0x00B0:	case 0x00B1:	case 0x00B6:	case 0x00B7:
			to = mask8Bit(from);
			break;
		default:
			return false;
		}
	}
	return true;
}

/// @see SBCSEncoder#doToUnicode
inline bool IS434Encoder::doToUnicode(Char& to, uchar from) const {
	if(from >= 0xE0)
		to = from + 0x15A0;
	else if(from < 0xA0)
		to = from;
	else {
		switch(from) {
		case 0xA0:	case 0xA3:	case 0xA7:	case 0xA9:	case 0xAE:
		case 0xB0:	case 0xB1:	case 0xB6:	case 0xB7:
			to = from;
			break;
		default:
			return false;
		}
	}
	return true;
}

#endif /* !ASCENSION_NO_EXTENDED_ENCODINGS */
