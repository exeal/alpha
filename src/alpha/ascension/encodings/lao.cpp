/**
 * @file lao.cpp
 * Defines encoders for MuleLao-1, CP1132 and CP1133.
 * @author exeal
 * @date 2004-2007
 */

#ifndef ASCENSION_NO_EXTENDED_ENCODINGS
#include "../encoder.hpp"
using namespace ascension;
using namespace ascension::encoding;
using namespace std;

// registry
namespace {
	ASCENSION_DEFINE_SBCS_ENCODER(MuleLao1Encoder, extended::MIB_LAO_MULE_LAO, "MuleLao-1")
//	ASCENSION_DEFINE_SBCS_ENCODER(CP1132Encoder, extended::MIB_LAO_CP1132, "CP1132")
	ASCENSION_DEFINE_SBCS_ENCODER(CP1133Encoder, extended::MIB_LAO_CP1133, "CP1133")

	struct Installer {
		Installer() {
			Encoder::registerEncoder(auto_ptr<Encoder>(new MuleLao1Encoder));
//			Encoder::registerEncoder(auto_ptr<Encoder>(new CP1132Encoder));
			Encoder::registerEncoder(auto_ptr<Encoder>(new CP1133Encoder));
		}
	} installer;
} // namespace @0

namespace {
	const Char RP__CH = REPLACEMENT_CHARACTER;
	const uchar N__A = UNMAPPABLE_NATIVE_CHARACTER;
	const Char MULELAOtoUCS[] = {
	/* 0xA0 */	0x00A0, 0x0E81, 0x0E82, RP__CH, 0x0E84, RP__CH, RP__CH, 0x0E87,
				0x0E88, RP__CH, 0x0E8A, RP__CH, RP__CH, 0x0E8D, RP__CH, RP__CH,
	/* 0xB0 */	RP__CH, RP__CH, RP__CH, RP__CH, 0x0E94, 0x0E95, 0x0E96, 0x0E97,
				RP__CH, 0x0E99, 0x0E9A, 0x0E9B, 0x0E9C, 0x0E9D, 0x0E9E, 0x0E9F,
	/* 0xC0 */	RP__CH, 0x0EA1, 0x0EA2, 0x0EA3, RP__CH, 0x0EA5, RP__CH, 0x0EA7,
				RP__CH, RP__CH, 0x0EAA, 0x0EAB, RP__CH, 0x0EAD, 0x0EAE, 0x0EAF,
	/* 0xD0 */	0x0EB0, 0x0EB1, 0x0EB2, 0x0EB3, 0x0EB4, 0x0EB5, 0x0EB6, 0x0EB7,
				0x0EB8, 0x0EB9, RP__CH, 0x0EBB, 0x0EBC, 0x0EBD, RP__CH, RP__CH,
	/* 0xE0 */	0x0EC0, 0x0EC1, 0x0EC2, 0x0EC3, 0x0EC4, RP__CH, 0x0EC6, RP__CH,
				0x0EC8, 0x0EC9, 0x0ECA, 0x0ECB, 0x0ECC, 0x0ECD, RP__CH, RP__CH,
	/* 0xF0 */	0x0ED0, 0x0ED1, 0x0ED2, 0x0ED3, 0x0ED4, 0x0ED5, 0x0ED6, 0x0ED7,
				0x0ED8, 0x0ED9, RP__CH, RP__CH, 0x0EDC, 0x0EDD, RP__CH, RP__CH,
	};
	const uchar UCStoMULELAO[] = {
	/* U+0E80 */	N__A, 0xA1, 0xA2, N__A, 0xA4, N__A, N__A, 0xA7,
					0xA8, N__A, 0xAA, N__A, N__A, 0xAD, N__A, N__A,
	/* U+0E90 */	N__A, N__A, N__A, N__A, 0xB4, 0xB5, 0xB6, 0xB7,
					N__A, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
	/* U+0EA0 */	N__A, 0xC1, 0xC2, 0xC3, N__A, 0xC5, N__A, 0xC7,
					N__A, N__A, 0xCA, 0xCB, N__A, 0xCD, 0xCE, 0xCF,
	/* U+0EB0 */	0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7,
					0xD8, 0xD9, N__A, 0xDB, 0xDC, 0xDD, N__A, N__A,
	/* U+0EC0 */	0xE0, 0xE1, 0xE2, 0xE3, 0xE4, N__A, 0xE6, N__A,
					0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, N__A, N__A,
	/* U+0ED0 */	0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
					0xF8, 0xF9, N__A, N__A, 0xFC, 0xFD
	};
	const Char CP1133toUCS[] = {
	/* 0xA0 */	RP__CH, 0x0E81, 0x0E82, 0x0E84, 0x0E87, 0x0E88, 0x0EAA, 0x0E8A,
				0x0E8D, 0x0E94, 0x0E95, 0x0E96, 0x0E97, 0x0E99, 0x0E9A, 0x0E9B,
	/* 0xB0 */	0x0E9C, 0x0E9D, 0x0E9E, 0x0E9F, 0x0EA1, 0x0EA2, 0x0EA3, 0x0EA5,
				0x0EA7, 0x0EAB, 0x0EAD, 0x0EAE, RP__CH, RP__CH, RP__CH, 0x0EAF,
	/* 0xC0 */	0x0EB0, 0x0EB2, 0x0EB3, 0x0EB4, 0x0EB5, 0x0EB6, 0x0EB7, 0x0EB8,
				0x0EB9, 0x0EBC, 0x0EB1, 0x0EBB, 0x0EBD, RP__CH, RP__CH, RP__CH,
	/* 0xD0 */	0x0EC0, 0x0EC1, 0x0EC2, 0x0EC3, 0x0EC4, 0x0EC8, 0x0EC9, 0x0ECA,
				0x0ECB, 0x0ECC, 0x0ECD, 0x0EC6, RP__CH, 0x0EDC, 0x0EDD, 0x20AD,
	/* 0xE0 */	RP__CH, RP__CH, RP__CH, RP__CH, RP__CH, RP__CH, RP__CH, RP__CH,
				RP__CH, RP__CH, RP__CH, RP__CH, RP__CH, RP__CH, RP__CH, RP__CH,
	/* 0xF0 */  0x0ED0, 0x0ED1, 0x0ED2, 0x0ED3, 0x0ED4, 0x0ED5, 0x0ED6, 0x0ED7,
				0x0ED8, 0x0ED9, RP__CH, RP__CH, 0x00A2, 0x00AC, 0x00A6, 0x00A0,
	};
	const uchar UCStoCP1133_00A0[] = {
	/* U+00A0 */	0xFF, N__A, 0xFC, N__A, N__A, N__A, 0xFE, N__A,
					N__A, N__A, N__A, N__A, 0xFD
	};
	const uchar UCStoCP1133_0E80[] = {
	/* U+0E80 */	N__A, 0xA1, 0xA2, N__A, 0xA3, N__A, N__A, 0xA4,
					0xA5, N__A, 0xA7, N__A, N__A, 0xA8, N__A, N__A,
	/* U+0E90 */	N__A, N__A, N__A, N__A, 0xA9, 0xAA, 0xAB, 0xAC,
					N__A, 0xAD, 0xAE, 0xAF, 0xB0, 0xB1, 0xB2, 0xB3,
	/* U+0EA0 */	N__A, 0xB4, 0xB5, 0xB6, N__A, 0xB7, N__A, 0xB8,
					N__A, N__A, 0xA6, 0xB9, N__A, 0xBA, 0xBB, 0xBF,
	/* U+0EB0 */	0xC0, 0xCA, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6,
					0xC7, 0xC8, N__A, 0xCB, 0xC9, 0xCC, N__A, N__A,
	/* U+0EC0 */	0xD0, 0xD1, 0xD2, 0xD3, 0xD4, N__A, 0xDB, N__A,
					0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, N__A, N__A,
	/* U+0ED0 */	0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
					0xF8, 0xF9, N__A, N__A, 0xDD, 0xDE
	};
}


// MuleLao1Encoder //////////////////////////////////////////////////////////

/// @see SBCSEncoder#doFromUnicode
inline bool MuleLao1Encoder::doFromUnicode(uchar& to, Char from) const {
	if(from <= 0x00A0)
		to = mask8Bit(from);
	else {
		to = (from >= 0x0E80 && from < 0x0E80 + countof(UCStoMULELAO)) ? UCStoMULELAO[from - 0x0E80] : UNMAPPABLE_NATIVE_CHARACTER;
		if(to == UNMAPPABLE_NATIVE_CHARACTER)
			return false;
	}
	return true;
}

/// @see SBCSEncoder#doToUnicode
inline bool MuleLao1Encoder::doToUnicode(Char& to, uchar from) const {
	if(from < 0xA0)
		to = from;
	else {
		to = MULELAOtoUCS[from - 0xA0];
		if(to == REPLACEMENT_CHARACTER)
			return false;
	}
	return true;
}


// CP1133Encoder ////////////////////////////////////////////////////////////

/// @see SBCSEncoder#doFromUnicode
inline bool CP1133Encoder::doFromUnicode(uchar& to, Char from) const {
	if(from < 0x00A0)
		to = mask8Bit(from);
	else {
		if(from < 0x00A0 + countof(UCStoCP1133_00A0))
			to = UCStoCP1133_00A0[from - 0x00A0];
		else if(from < 0x0E80 + countof(UCStoCP1133_0E80))
			to = UCStoCP1133_0E80[from - 0x0E80];
		else if(from == 0x20AD)
			to = static_cast<uchar>(0xDF);
		else
			return false;
		if(to == UNMAPPABLE_NATIVE_CHARACTER)
			return false;
	}
	return true;
}

/// @see SBCSEncoder#doToUnicode
inline bool CP1133Encoder::doToUnicode(Char& to, uchar from) const {
	if(from < 0xA0)
		to = from;
	else {
		to = CP1133toUCS[from - 0xA0];
		if(to == REPLACEMENT_CHARACTER)
			return false;
	}
	return true;
}

#endif /* !ASCENSION_NO_EXTENDED_ENCODINGS */
