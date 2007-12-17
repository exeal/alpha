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

namespace {
	const Char RP__CH = REPLACEMENT_CHARACTER;
	const Char IS434[0x80] = {
		ASCENSION_INCREMENTAL_BYTE_SEQUENCE_C1,
		0x00A0, RP__CH, RP__CH, 0x00A3, RP__CH, RP__CH, RP__CH, 0x00A7,	// 0xA0
		RP__CH, 0x00A9, RP__CH, RP__CH, RP__CH, RP__CH, 0x00AE, RP__CH,
		0x00B0, 0x00B1, RP__CH, RP__CH, RP__CH, RP__CH, 0x00B6, 0x00B7,	// 0xB0
		RP__CH, RP__CH, RP__CH, RP__CH, RP__CH, RP__CH, RP__CH, RP__CH,
		RP__CH, RP__CH, RP__CH, RP__CH, RP__CH, RP__CH, RP__CH, RP__CH,	// 0xC0
		RP__CH, RP__CH, RP__CH, RP__CH, RP__CH, RP__CH, RP__CH, RP__CH,
		RP__CH, RP__CH, RP__CH, RP__CH, RP__CH, RP__CH, RP__CH, RP__CH,	// 0xD0
		RP__CH, RP__CH, RP__CH, RP__CH, RP__CH, RP__CH, RP__CH, RP__CH,
		0x1680, 0x1681, 0x1682, 0x1683, 0x1684, 0x1685, 0x1686, 0x1687,	// 0xE0
		0x1688, 0x1689, 0x168A, 0x168B, 0x168C, 0x168D, 0x168E, 0x168F,
		0x1690, 0x1691, 0x1692, 0x1693, 0x1694, 0x1695, 0x1696, 0x1697,	// 0xF0
		0x1698, 0x1699, 0x169A, 0x169B, 0x169C, 0x169D, 0x169E, 0x169F
	};
	struct Installer {
		Installer() {
			Encoder::registerEncoder(std::auto_ptr<Encoder>(
				new implementation::SingleByteEncoder("I.S. 434", extended::IS434, "", IS434)));
		}
	} installer;
} // namespace @0

#endif /* !ASCENSION_NO_EXTENDED_ENCODINGS */
