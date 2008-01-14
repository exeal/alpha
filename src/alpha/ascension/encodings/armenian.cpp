/**
 * @file armenian.cpp
 * Implements Armenian encodings. This includes:
 * - ARMSCII-7
 * - ARMSCII-8
 * - ARMSCII-8A
 * This implementation is based on the report of Hovik Melikyan (http://www.freenet.am/armscii/).
 * @author exeal
 * @date 2004-2008
 */

#ifndef ASCENSION_NO_STANDARD_ENCODINGS
#include "../encoder.hpp"
#include <algorithm>	// std.binary_search
using namespace ascension;
using namespace ascension::encoding;
using namespace ascension::encoding::implementation;
using namespace std;

// registry
namespace {
	template<int n> class ARMSCII : public EncoderFactoryBase {
	public:
		ARMSCII() throw();
		auto_ptr<Encoder> create() const throw() {return auto_ptr<Encoder>(new InternalEncoder(*this));}
	private:
		class InternalEncoder : public Encoder {
		public:
			explicit InternalEncoder(const IEncodingProperties& properties) throw() : props_(properties) {}
		private:
			Result doFromUnicode(byte* to, byte* toEnd, byte*& toNext,
				const Char* from, const Char* fromEnd, const Char*& fromNext);
			Result doToUnicode(Char* to, Char* toEnd, Char*& toNext,
				const byte* from, const byte* fromEnd, const byte*& fromNext);
			const IEncodingProperties& properties() const throw() {return props_;}
		private:
			const IEncodingProperties& props_;
		};
	};
	ARMSCII<8> ARMSCII_8;
#ifndef ASCENSION_NO_MINORITY_ENCODINGS
	ARMSCII<7> ARMSCII_7;
	ARMSCII<0x8A> ARMSCII_8A;
#endif /* !ASCENSION_NO_MINORITY_ENCODINGS */

	class ArmenianDetector : public EncodingDetector {
	public:
		ArmenianDetector() : EncodingDetector("ARMSCIIAutoDetect") {}
	private:
		MIBenum	doDetect(const byte* first, const byte* last, ptrdiff_t* convertibleBytes) const throw();
	};

	struct Installer {
		Installer() {
			Encoder::registerFactory(ARMSCII_8);
#ifndef ASCENSION_NO_MINORITY_ENCODINGS
			Encoder::registerFactory(ARMSCII_7);
			Encoder::registerFactory(ARMSCII_8A);
#endif /* !ASCENSION_NO_MINORITY_ENCODINGS */
			EncodingDetector::registerDetector(auto_ptr<EncodingDetector>(new ArmenianDetector));
		}
	} installer;
} // namespace @0

namespace {
	const Char RP__CH = REPLACEMENT_CHARACTER;
	const byte N__A = 0x1A;
	const Char ARMSCII78toUCS_20[] = {
	/* 0x20 */	0x0020, RP__CH, 0x00A7, 0x0589, 0x0029, 0x0028, 0x00BB, 0x00AB,
				0x2014, 0x002E, 0x055D, 0x002C, 0x002D, 0x058A, 0x2026, 0x055C,
	/* 0x30 */	0x055B, 0x055E, 0x0531, 0x0561, 0x0532, 0x0562, 0x0533, 0x0563,
				0x0534, 0x0564, 0x0535, 0x0565, 0x0536, 0x0566, 0x0537, 0x0567,
	/* 0x40 */	0x0538, 0x0568, 0x0539, 0x0569, 0x053A, 0x056A, 0x053B, 0x056B,
				0x053C, 0x056C, 0x053D, 0x056D, 0x053E, 0x056E, 0x053F, 0x056F,
	/* 0x50 */	0x053F, 0x056F, 0x0540, 0x0570, 0x0541, 0x0571, 0x0542, 0x0572,
				0x0544, 0x0574, 0x0545, 0x0575, 0x0546, 0x0576, 0x0547, 0x0577,
	/* 0x60 */	0x0548, 0x0578, 0x0549, 0x0579, 0x054A, 0x057A, 0x054B, 0x057B,
				0x054C, 0x057C, 0x054D, 0x057D, 0x054E, 0x057E, 0x054F, 0x057F,
	/* 0x70 */	0x0550, 0x0580, 0x0551, 0x0581, 0x0552, 0x0582, 0x0553, 0x0583,
				0x0554, 0x0584, 0x0555, 0x0585, 0x0556, 0x0586, 0x055A, 0x007F
	};
#ifndef ASCENSION_NO_MINORITY_ENCODINGS
	const byte UCStoARMSCII7_0028[] = {
					0x25, 0x24, N__A, N__A, 0x2B, 0x2C, 0x29, N__A
	};
#endif /* !ASCENSION_NO_MINORITY_ENCODINGS */
	const byte UCStoARMSCII8_0028[] = {
					0xA5, 0xA4, 0x2A, 0x2B, 0xAB, 0xAC, 0xA9, 0x2F
	};
	const byte UCStoARMSCII78_00A0[] = {
	/* U+00A0 */	0x20, N__A, N__A, N__A, N__A, N__A, N__A, 0x22,
					N__A, N__A, N__A, 0x27, N__A, N__A, N__A, N__A,
	/* U+00B0 */	N__A, N__A, N__A, N__A, N__A, N__A, N__A, N__A,
					N__A, N__A, N__A, 0x26
	};
	const byte UCStoARMSCII78_0530[] = {
	/* U+0530 */	N__A, 0x32, 0x34, 0x36, 0x38, 0x3A, 0x3C, 0x3E,
					0x40, 0x42, 0x44, 0x46, 0x48, 0x4A, 0x4C, 0x4E,
	/* U+0540 */	0x50, 0x52, 0x54, 0x56, 0x58, 0x5A, 0x5C, 0x5E,
					0x60, 0x62, 0x64, 0x66, 0x68, 0x6A, 0x6C, 0x6E,
	/* U+0550 */	0x70, 0x72, 0x74, 0x76, 0x78, 0x7A, 0x7C, N__A,
					N__A, N__A, 0x7E, 0x30, 0x2F, 0x2A, 0x31, N__A,
	/* U+0560 */	N__A, 0x33, 0x35, 0x37, 0x39, 0x3B, 0x3D, 0x3F,
					0x41, 0x43, 0x45, 0x47, 0x49, 0x4B, 0x4D, 0x4F,
	/* U+0570 */	0x51, 0x53, 0x55, 0x57, 0x59, 0x5B, 0x5D, 0x5F,
					0x61, 0x63, 0x65, 0x67, 0x69, 0x6B, 0x6D, 0x6F,
	/* U+0580 */	0x71, 0x73, 0x75, 0x77, 0x79, 0x7B, 0x7D, N__A,
					N__A, 0x23, 0x2D
	};
	const byte UCStoARMSCII78_2010[] = {
	/* U+2010 */	N__A, N__A, N__A, N__A, 0x28, N__A, N__A, N__A,
					N__A, N__A, N__A, N__A, N__A, N__A, N__A, N__A,
	/* U+2020 */	N__A, N__A, N__A, N__A, N__A, N__A, 0x2E
	};
#ifndef ASCENSION_NO_MINORITY_ENCODINGS
	const Char ARMSCII8AtoUCS_20[] = {
	/* 0x20 */	0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x055B,
				0x0028, 0x0029, 0x0030, 0x0031, 0x002C, 0x2014, 0x002E, 0x0032,
	/* 0x30 */	0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
				0x0038, 0x0039, 0x0589, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F,
	/* 0x40 */	0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
				0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,
	/* 0x50 */	0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
				0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x002D,
	/* 0x60 */	0x055D, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
				0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F,
	/* 0x70 */	0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
				0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D, 0x055C, 0x007F,
	/* 0x80 */	0x0531, 0x0561, 0x0532, 0x0562, 0x0533, 0x0563, 0x0534, 0x0564,
				0x0535, 0x0565, 0x0536, 0x0566, 0x0537, 0x0567, 0x0538, 0x0568,
	/* 0x90 */	0x0539, 0x0569, 0x053A, 0x056A, 0x053B, 0x056B, 0x053C, 0x056C,
				0x053D, 0x056D, 0x053E, 0x056E, 0x053F, 0x056F, 0x0540, 0x0570,
	/* 0xA0 */	0x0541, 0x0571, 0x0542, 0x0572, 0x0543, 0x0573, 0x0544, 0x0574,
				0x0545, 0x0575, 0x0546, 0x0576, 0x0547, 0x0577, 0x00AB, 0x00BB
	};
	const Char ARMSCII8AtoUCS_D8[] = {
				RP__CH, RP__CH, RP__CH, RP__CH, RP__CH, 0x058A, 0x2026, 0x055E,
	/* 0xE0 */	0x0548, 0x0578, 0x0549, 0x0579, 0x054A, 0x057A, 0x054B, 0x057B,
				0x054C, 0x057C, 0x054D, 0x057D, 0x054E, 0x057E, 0x054F, 0x057F,
	/* 0xF0 */	0x0550, 0x0580, 0x0551, 0x0581, 0x0552, 0x0582, 0x0553, 0x0583,
				0x0554, 0x0584, 0x0555, 0x0585, 0x0556, 0x0586, 0x055A, RP__CH
	};

	const byte UCStoARMSCII8A_00A8[] = {
					N__A, N__A, 0xAE, N__A, N__A, N__A, N__A, N__A,
	/* U+00B0 */	N__A, N__A, N__A, N__A, N__A, N__A, N__A, N__A,
					N__A, N__A, N__A, 0xAF
	};
	const byte UCStoARMSCII8A_0530[] = {
	/* U+0530 */	N__A, 0x80, 0x82, 0x84, 0x86, 0x88, 0x8A, 0x8C,
					0x8E, 0x90, 0x92, 0x94, 0x96, 0x98, 0x9A, 0x9C,
	/* U+0540 */	0x9E, 0xA0, 0xA2, 0xA4, 0xA6, 0xA8, 0xAA, 0xAC,
					0xE0, 0xE2, 0xE4, 0xE6, 0xE8, 0xEA, 0xEC, 0xEE,
	/* U+0550 */	0xF0, 0xF2, 0xF4, 0xF6, 0xF8, 0xFA, 0xFC, N__A,
					N__A, N__A, 0xFE, 0x27, 0x7E, 0x60, 0xDF, N__A,
	/* U+0560 */	N__A, 0x81, 0x83, 0x85, 0x87, 0x89, 0x8B, 0x8D,
					0x8F, 0x91, 0x93, 0x95, 0x97, 0x99, 0x9B, 0x9D,
	/* U+0570 */	0x9F, 0xA1, 0xA3, 0xA5, 0xA7, 0xA9, 0xAB, 0xAD,
					0xE1, 0xE3, 0xE5, 0xE7, 0xE9, 0xEB, 0xED, 0xEF,
	/* U+0580 */	0xF1, 0xF3, 0xF5, 0xF7, 0xF9, 0xFB, 0xFD, 0x3A,
					N__A, N__A, 0xDD
	};
	const byte UCStoARMSCII8A_2010[] = {
	/* U+2010 */	N__A, N__A, N__A, N__A, 0x2D, N__A, N__A, N__A,
					N__A, N__A, N__A, N__A, N__A, N__A, N__A, N__A,
	/* U+2020 */	N__A, N__A, N__A, N__A, N__A, N__A, 0xDE
	};
#endif /* !ASCENSION_NO_MINORITY_ENCODINGS */

	inline const Char* decomposeArmenianLigature(Char c) {
		switch(c) {
		case 0x0587:	return L"\x0565\x0582";	// Ech Yiwn
		case 0xFB13:	return L"\x0574\x0576";	// Men Now
		case 0xFB14:	return L"\x0574\x0565";	// Men Ech
		case 0xFB15:	return L"\x0574\x056B";	// Men Ini
		case 0xFB16:	return L"\x057E\x0576";	// Vew Now
		case 0xFB17:	return L"\x0574\x056D";	// Men Xeh
		default:		return 0;
		}
	}
} // namespace @0


// ARMSCII-8 ////////////////////////////////////////////////////////////////

template<> ARMSCII<8>::ARMSCII() throw() : EncoderFactoryBase("ARMSCII-8", MIB_OTHER, "Armenian (ARMSCII-8)", 1, 2, "", 0x1A) {
}

template<> Encoder::Result ARMSCII<8>::InternalEncoder::doFromUnicode(
		byte* to, byte* toEnd, byte*& toNext, const Char* from, const Char* fromEnd, const Char*& fromNext) {
	for(; to < toEnd && from < fromEnd; ++to, ++from) {
		if(*from < 0x0028) {
			*to = mask8Bit(*from);
			continue;
		} else if(*from < 0x0028 + countof(UCStoARMSCII8_0028))
			*to = UCStoARMSCII8_0028[*from - 0x0028];
		else if(*from < 0x00A0 + countof(UCStoARMSCII78_00A0))
			*to = UCStoARMSCII78_00A0[*from - 0x00A0];
		else if(*from < 0x0530 + countof(UCStoARMSCII78_0530))
			*to = UCStoARMSCII78_0530[*from - 0x0530];
		else if(*from < 0x2010 + countof(UCStoARMSCII78_2010))
			*to = UCStoARMSCII78_2010[*from - 0x2010];
		else if(const Char* decomposed = decomposeArmenianLigature(*from)) {
			if(to + 1 < toEnd) {
				toNext = to;
				fromNext = from;
				return INSUFFICIENT_BUFFER;
			}
			*to = UCStoARMSCII78_0530[decomposed[0] - 0x0530] + 0x80;
			*++to = UCStoARMSCII78_0530[decomposed[1] - 0x0530] + 0x80;
			assert(to[-1] != 0x80 && to[0] != 0x80);
			continue;
		} else
			*to = props_.substitutionCharacter();

		if(*to == props_.substitutionCharacter()) {
			if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTER) {
				--to;
				continue;
			} else if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTER) {
				toNext = to;
				fromNext = from;
				return UNMAPPABLE_CHARACTER;
			}
		}
		*to += 0x80;
	}
	toNext = to;
	fromNext = from;
	return (fromNext == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
}

template<> Encoder::Result ARMSCII<8>::InternalEncoder::doToUnicode(
		Char* to, Char* toEnd, Char*& toNext, const byte* from, const byte* fromEnd, const byte*& fromNext) {
	for(; to < toEnd && from < fromEnd; ++to, ++from) {
		if(*from < 0xA1)
			*to = *from;
		else if(ARMSCII78toUCS_20[*from - 0x20 - 0x80] != REPLACEMENT_CHARACTER)
			*to = ARMSCII78toUCS_20[*from - 0x20 - 0x80];
		else if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTER)
			--to;
		else if(substitutionPolicy() == DONT_SUBSTITUTE) {
			toNext = to;
			fromNext = from;
			return UNMAPPABLE_CHARACTER;
		}
	}
	toNext = to;
	fromNext = from;
	return (fromNext == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
}


#ifndef ASCENSION_NO_MINORITY_ENCODINGS

// ARMSCII-7 ////////////////////////////////////////////////////////////////

template<> ARMSCII<7>::ARMSCII() throw() : EncoderFactoryBase("ARMSCII-7", MIB_OTHER, "Armenian (ARMSCII-7)", 1, 2, "", 0x1A) {
}

template<> Encoder::Result ARMSCII<7>::InternalEncoder::doFromUnicode(
		byte* to, byte* toEnd, byte*& toNext, const Char* from, const Char* fromEnd, const Char*& fromNext) {
	for(; to < toEnd && from < fromEnd; ++to, ++from) {
		if(*from < 0x0028) {
			*to = mask8Bit(*from);
			continue;
		} else if(*from < 0x0028 + countof(UCStoARMSCII7_0028))
			*to = UCStoARMSCII7_0028[*from - 0x0028];
		else if(*from < 0x00A0 + countof(UCStoARMSCII78_00A0))
			*to = UCStoARMSCII78_00A0[*from - 0x00A0];
		else if(*from < 0x0530 + countof(UCStoARMSCII78_0530))
			*to = UCStoARMSCII78_0530[*from - 0x0530];
		else if(*from < 0x2010 + countof(UCStoARMSCII78_2010))
			*to = UCStoARMSCII78_2010[*from - 0x2010];
		else if(const Char* const decomposed = decomposeArmenianLigature(*from)) {
			if(to + 1 < toEnd) {
				toNext = to;
				fromNext = from;
				return INSUFFICIENT_BUFFER;
			}
			*to = UCStoARMSCII78_0530[decomposed[0] - 0x0530];
			*++to = UCStoARMSCII78_0530[decomposed[1] - 0x0530];
			assert(to[-1] != props_.substitutionCharacter() && to[0] != props_.substitutionCharacter());
			continue;
		} else
			*to = props_.substitutionCharacter();

		if(*to == props_.substitutionCharacter()) {
			if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTER)
				--to;
			else if(substitutionPolicy() != REPLACE_UNMAPPABLE_CHARACTER) {
				toNext = to;
				fromNext = from;
				return UNMAPPABLE_CHARACTER;
			}
		}
	}
	toNext = to;
	fromNext = from;
	return (fromNext == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
}

template<> Encoder::Result ARMSCII<7>::InternalEncoder::doToUnicode(
		Char* to, Char* toEnd, Char*& toNext, const byte* from, const byte* fromEnd, const byte*& fromNext) {
	for(; to < toEnd && from < fromEnd; ++to, ++from) {
		if(*from < 0x20)
			*to = *from;
		else if(*from < 0x20 + countof(ARMSCII78toUCS_20) && ARMSCII78toUCS_20[*from - 0x20] != REPLACEMENT_CHARACTER)
			*to = ARMSCII78toUCS_20[*from - 0x20];
		else if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTER)
			--to;
		else if(substitutionPolicy() == DONT_SUBSTITUTE) {
			toNext = to;
			fromNext = from;
			return UNMAPPABLE_CHARACTER;
		}
	}
	toNext = to;
	fromNext = from;
	return (fromNext == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
}


// ARMSCII-8A ///////////////////////////////////////////////////////////////

template<> ARMSCII<0x8A>::ARMSCII() throw() : EncoderFactoryBase("ARMSCII-8A", MIB_OTHER, "Armenian (ARMSCII-8A)", 1, 2, "", 0x1A) {
}

template<> Encoder::Result ARMSCII<0x8A>::InternalEncoder::doFromUnicode(
		byte* to, byte* toEnd, byte*& toNext, const Char* from, const Char* fromEnd, const Char*& fromNext) {
	for(; to < toEnd && from < fromEnd; ++to, ++from) {
		if(*from < 0x80) {
			static const Char invChars[] = {0x0027, 0x003A, 0x005F, 0x0060, 0x007E};
			*to = binary_search(invChars, invChars + countof(invChars), *from) ? mask8Bit(*from) : props_.substitutionCharacter();
		} else if(*from < 0x00A8)
			*to = props_.substitutionCharacter();
		else if(*from < 0x00A8 + countof(UCStoARMSCII8A_00A8))
			*to = UCStoARMSCII8A_00A8[*from - 0x00A8];
		else if(*from < 0x0530 + countof(UCStoARMSCII8A_0530))
			*to = UCStoARMSCII8A_0530[*from - 0x0530];
		else if(*from < 0x2010 + countof(UCStoARMSCII8A_2010))
			*to = UCStoARMSCII8A_2010[*from - 0x2010];
		else if(const Char* const decomposed = decomposeArmenianLigature(*from)) {
			if(to + 1 < toEnd) {
				toNext = to;
				fromNext = from;
				return INSUFFICIENT_BUFFER;
			}
			*to = UCStoARMSCII8A_0530[decomposed[0] - 0x0530] + 0x80;
			*++to = UCStoARMSCII8A_0530[decomposed[1] - 0x0530] + 0x80;
			assert(to[-1] != 0x80 && to[0] != 0x80);
			continue;
		} else
			*to = props_.substitutionCharacter();

		if(*to == props_.substitutionCharacter()) {
			if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTER)
				--to;
			else if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTER) {
				toNext = to;
				fromNext = from;
				return UNMAPPABLE_CHARACTER;
			}
		}
	}
	toNext = to;
	fromNext = from;
	return (fromNext == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
}

template<> Encoder::Result ARMSCII<0x8A>::InternalEncoder::doToUnicode(
		Char* to, Char* toEnd, Char*& toNext, const byte* from, const byte* fromEnd, const byte*& fromNext) {
	for(; to < toEnd && from < fromEnd; ++to, ++from) {
		if(*from < 0x20)
			*to = *from;
		else if(*from < 0x20 + countof(ARMSCII8AtoUCS_20))
			*to = ARMSCII8AtoUCS_20[*from - 0x20];
		else
			*to = ARMSCII8AtoUCS_D8[*from - 0xD8];
		if(*to == REPLACEMENT_CHARACTER) {
			if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTER)
				--to;
			else if(substitutionPolicy() == DONT_SUBSTITUTE) {
				toNext = to;
				fromNext = from;
				return UNMAPPABLE_CHARACTER;
			}
		}
	}
	toNext = to;
	fromNext = from;
	return (fromNext == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
}

#endif /* !ASCENSION_NO_MINORITY_ENCODINGS */


// ArmenianDetector /////////////////////////////////////////////////////////

/// @see EncodingDetector#doDetect
MIBenum ArmenianDetector::doDetect(const byte* first, const byte* last, ptrdiff_t* convertibleBytes) const throw() {
	// first, check if Unicode
	if(const EncodingDetector* unicodeDetector = forName("UnicodeAutoDetect")) {
		ptrdiff_t temp;
		MIBenum result = unicodeDetector->detect(first, last, &temp);
		if(temp == last - first) {
			if(convertibleBytes != 0)
				*convertibleBytes = temp;
			return result;
		}
	}

	if(convertibleBytes != 0)
		*convertibleBytes = last - first;

#ifndef ASCENSION_NO_MINORITY_ENCODINGS
	bool b[3] = {true, true, true};	// 0:-7, 1:-8, 2:-8A
	for(; first < last; ++first) {
		const byte c = *first;
		if(c >= 0x80)				b[0] = false;	// ARMSCII-7 consists of only 7-bits
		if(c >= 0x80 && c < 0xA0)	b[1] = false;	// 8-bit controls (but ARMSCII-8 may contain these)
		if(c >= 0xB0 && c < 0xDC)	b[2] = false;

		if(!b[0] && !b[2])
			return extended::ARMSCII8;	// ARMSCII-8
	}
	if(!b[0] && !b[1])
		return extended::ARMSCII8A;
	else if(!b[2] && !b[1])
		return extended::ARMSCII7;
#endif /* !ASCENSION_NO_MINORITY_ENCODINGS */
	return extended::ARMSCII8;	// most preferred encoding
}

#endif /* !ASCENSION_NO_STANDARD_ENCODINGS */
