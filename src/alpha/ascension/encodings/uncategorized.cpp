/**
 * @file uncategorized.cpp
 * @author exeal
 * @date 2004-2007
 */

#ifndef ASCENSION_NO_EXTENDED_ENCODINGS
#include "../encoder.hpp"

using namespace ascension;
using namespace ascension::encoding;
using namespace std;

namespace {
	// This table is from Atari ST/TT Character Encoding (http://www.kostis.net/charsets/atarist.htm)
	// and Unicode.org (http://www.unicode.org/Public/MAPPINGS/VENDORS/MISC/ATARIST.TXT).
	const Char ATARIST[0x80] = {
		0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7,
		0x00EA, 0x00EB, 0x00E8, 0x00EF, 0x00EE, 0x00EC, 0x00C4, 0x00C5,
		0x00C9, 0x00E6, 0x00C6, 0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9,
		0x00FF, 0x00D6, 0x00DC, 0x00A2, 0x00A3, 0x00A5, 0x00DF, 0x0192,
		0x00E1, 0x00ED, 0x00F3, 0x00FA, 0x00F1, 0x00D1, 0x00AA, 0x00BA,
		0x00BF, 0x2310, 0x00AC, 0x00BD, 0x00BC, 0x00A1, 0x00AB, 0x00BB,
		0x00E3, 0x00F5, 0x00D8, 0x00F8, 0x0153, 0x0152, 0x00C0, 0x00C3,
		0x00D5, 0x00A8, 0x00B4, 0x2020, 0x00B6, 0x00A9, 0x00AE, 0x2122,
		0x0133, 0x0132, 0x05D0, 0x05D1, 0x05D2, 0x05D3, 0x05D4, 0x05D5,
		0x05D6, 0x05D7, 0x05D8, 0x05D9, 0x05DB, 0x05DC, 0x05DE, 0x05E0,
		0x05E1, 0x05E2, 0x05E4, 0x05E6, 0x05E7, 0x05E8, 0x05E9, 0x05EA,
		0x05DF, 0x05DA, 0x05DD, 0x05E3, 0x05E5, 0x00A7, 0x2227, 0x221E,
		0x03B1, 0x03B2, 0x0393, 0x03C0, 0x03A3, 0x03C3, 0x00B5, 0x03C4,
		0x03A6, 0x0398, 0x03A9, 0x03B4, 0x222E, 0x03C6, 0x2208, 0x2229,
		0x2261, 0x00B1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00F7, 0x2248,
		0x00B0, 0x2219, 0x00B7, 0x221A, 0x207F, 0x00B2, 0x00B3, 0x00AF
	};
	const Char NEXTSTEP[0x80] = {};
	struct Installer {
		Installer() {
			using implementation::SingleByteEncoder;
//			Encoder::registerEncoder(auto_ptr<Encoder>(new SingleByteEncoder("Nextstep", extended::NEXTSTEP, "", NEXTSTEP)));
			Encoder::registerEncoder(auto_ptr<Encoder>(new SingleByteEncoder("Atarist ST/TT", extended::ATARIST, "", ATARIST)));
		}
	} installer;
} // namespace @0

#endif /* !ASCENSION_NO_EXTENDED_ENCODINGS */
