/**
 * @file armenian.cpp
 * Implements Armenian encodings. This includes:
 * - ARMSCII-7
 * - ARMSCII-8
 * - ARMSCII-8A
 * This implementation is based on the report of Hovik Melikyan (http://www.freenet.am/armscii/).
 * @author exeal
 * @date 2004-2011
 */

#ifndef ASCENSION_NO_STANDARD_ENCODINGS
#include <ascension/corelib/encoder.hpp>
#include <ascension/corelib/text/character.hpp>	// text.REPLACEMENT_CHARACTER
#include <algorithm>	// std.binary_search
#ifndef ASCENSION_NO_MINORITY_ENCODINGS
#include <bitset>
#endif // !ASCENSION_NO_MINORITY_ENCODINGS
#include <cassert>

using namespace ascension;
using namespace ascension::encoding;
using namespace ascension::encoding::implementation;
using namespace std;

// registry
namespace {
	template<int n> class ARMSCII : public EncoderFactoryBase {
	public:
		ARMSCII() /*throw()*/;
		unique_ptr<Encoder> create() const /*throw()*/ {
			return unique_ptr<Encoder>(new InternalEncoder(*this));
		}
	private:
		class InternalEncoder : public Encoder {
		public:
			explicit InternalEncoder(const EncodingProperties& properties) /*throw()*/ : props_(properties) {
			}
		private:
			Result doFromUnicode(Byte* to, Byte* toEnd, Byte*& toNext,
				const Char* from, const Char* fromEnd, const Char*& fromNext);
			Result doToUnicode(Char* to, Char* toEnd, Char*& toNext,
				const Byte* from, const Byte* fromEnd, const Byte*& fromNext);
			const EncodingProperties& properties() const /*throw()*/ {return props_;}
		private:
			const EncodingProperties& props_;
		};
	};

	class ArmenianDetector : public EncodingDetector {
	public:
		ArmenianDetector() : EncodingDetector("ARMSCIIAutoDetect") {}
	private:
		pair<MIBenum, string> doDetect(const Byte* first, const Byte* last, ptrdiff_t* convertibleBytes) const /*throw()*/;
	};

	const Char RP_CH_ = text::REPLACEMENT_CHARACTER;
	const Byte N_A_ = 0x1a;
	const Char ARMSCII78toUCS_20[] = {
		0x0020, RP_CH_, 0x00a7, 0x0589, 0x0029, 0x0028, 0x00bb, 0x00ab,	// 0x20
		0x2014, 0x002e, 0x055d, 0x002c, 0x002d, 0x058a, 0x2026, 0x055c,
		0x055b, 0x055e, 0x0531, 0x0561, 0x0532, 0x0562, 0x0533, 0x0563,	// 0x30
		0x0534, 0x0564, 0x0535, 0x0565, 0x0536, 0x0566, 0x0537, 0x0567,
		0x0538, 0x0568, 0x0539, 0x0569, 0x053a, 0x056a, 0x053b, 0x056b,	// 0x40
		0x053c, 0x056c, 0x053d, 0x056d, 0x053e, 0x056e, 0x053f, 0x056f,
		0x053f, 0x056f, 0x0540, 0x0570, 0x0541, 0x0571, 0x0542, 0x0572,	// 0x50
		0x0544, 0x0574, 0x0545, 0x0575, 0x0546, 0x0576, 0x0547, 0x0577,
		0x0548, 0x0578, 0x0549, 0x0579, 0x054a, 0x057a, 0x054b, 0x057b,	// 0x60
		0x054c, 0x057c, 0x054d, 0x057d, 0x054e, 0x057e, 0x054f, 0x057f,
		0x0550, 0x0580, 0x0551, 0x0581, 0x0552, 0x0582, 0x0553, 0x0583,	// 0x70
		0x0554, 0x0584, 0x0555, 0x0585, 0x0556, 0x0586, 0x055a, 0x007f
	};
#ifndef ASCENSION_NO_MINORITY_ENCODINGS
	const Byte UCStoARMSCII7_0028[] = {
		0x25, 0x24, N_A_, N_A_, 0x2b, 0x2c, 0x29, N_A_
	};
#endif // !ASCENSION_NO_MINORITY_ENCODINGS
	const Byte UCStoARMSCII8_0028[] = {
		0xa5, 0xa4, 0x2a, 0x2b, 0xab, 0xac, 0xa9, 0x2f
	};
	const Byte UCStoARMSCII78_00A0[] = {
		0x20, N_A_, N_A_, N_A_, N_A_, N_A_, N_A_, 0x22,	// U+00A0
		N_A_, N_A_, N_A_, 0x27, N_A_, N_A_, N_A_, N_A_,
		N_A_, N_A_, N_A_, N_A_, N_A_, N_A_, N_A_, N_A_,	// U+00B0
		N_A_, N_A_, N_A_, 0x26
	};
	const Byte UCStoARMSCII78_0530[] = {
		N_A_, 0x32, 0x34, 0x36, 0x38, 0x3a, 0x3c, 0x3e,	// U+0530
		0x40, 0x42, 0x44, 0x46, 0x48, 0x4a, 0x4c, 0x4e,
		0x50, 0x52, 0x54, 0x56, 0x58, 0x5a, 0x5c, 0x5e,	// U+0540
		0x60, 0x62, 0x64, 0x66, 0x68, 0x6a, 0x6c, 0x6e,
		0x70, 0x72, 0x74, 0x76, 0x78, 0x7a, 0x7c, N_A_,	// U+0550
		N_A_, N_A_, 0x7e, 0x30, 0x2f, 0x2a, 0x31, N_A_,
		N_A_, 0x33, 0x35, 0x37, 0x39, 0x3b, 0x3d, 0x3f,	// U+0560
		0x41, 0x43, 0x45, 0x47, 0x49, 0x4b, 0x4d, 0x4f,
		0x51, 0x53, 0x55, 0x57, 0x59, 0x5b, 0x5d, 0x5f,	// U+0570
		0x61, 0x63, 0x65, 0x67, 0x69, 0x6b, 0x6d, 0x6f,
		0x71, 0x73, 0x75, 0x77, 0x79, 0x7b, 0x7d, N_A_,	// U+0580
		N_A_, 0x23, 0x2D							   			
	};
	const Byte UCStoARMSCII78_2010[] = {
		N_A_, N_A_, N_A_, N_A_, 0x28, N_A_, N_A_, N_A_,	// U+2010
		N_A_, N_A_, N_A_, N_A_, N_A_, N_A_, N_A_, N_A_,
		N_A_, N_A_, N_A_, N_A_, N_A_, N_A_, 0x2e		// U+2020
	};
#ifndef ASCENSION_NO_MINORITY_ENCODINGS
	const Char ARMSCII8AtoUCS_20[] = {
		0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x055b,	// 0x20
		0x0028, 0x0029, 0x0030, 0x0031, 0x002c, 0x2014, 0x002e, 0x0032,
		0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,	// 0x30
		0x0038, 0x0039, 0x0589, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,
		0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,	// 0x40
		0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f,
		0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,	// 0x50
		0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x002d,
		0x055d, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,	// 0x60
		0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f,
		0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,	// 0x70
		0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x055c, 0x007f,
		0x0531, 0x0561, 0x0532, 0x0562, 0x0533, 0x0563, 0x0534, 0x0564,	// 0x80
		0x0535, 0x0565, 0x0536, 0x0566, 0x0537, 0x0567, 0x0538, 0x0568,
		0x0539, 0x0569, 0x053a, 0x056a, 0x053b, 0x056b, 0x053c, 0x056c,	// 0x90
		0x053d, 0x056d, 0x053e, 0x056e, 0x053f, 0x056f, 0x0540, 0x0570,
		0x0541, 0x0571, 0x0542, 0x0572, 0x0543, 0x0573, 0x0544, 0x0574,	// 0xa0
		0x0545, 0x0575, 0x0546, 0x0576, 0x0547, 0x0577, 0x00ab, 0x00bb
	};
	const Char ARMSCII8AtoUCS_D8[] = {
		RP_CH_, RP_CH_, RP_CH_, RP_CH_, RP_CH_, 0x058a, 0x2026, 0x055e,
		0x0548, 0x0578, 0x0549, 0x0579, 0x054a, 0x057a, 0x054b, 0x057b,	// 0xe0
		0x054c, 0x057c, 0x054d, 0x057d, 0x054e, 0x057e, 0x054f, 0x057f,
		0x0550, 0x0580, 0x0551, 0x0581, 0x0552, 0x0582, 0x0553, 0x0583,	// 0xf0
		0x0554, 0x0584, 0x0555, 0x0585, 0x0556, 0x0586, 0x055a, RP_CH_
	};

	const Byte UCStoARMSCII8A_00A8[] = {
		N_A_, N_A_, 0xae, N_A_, N_A_, N_A_, N_A_, N_A_,
		N_A_, N_A_, N_A_, N_A_, N_A_, N_A_, N_A_, N_A_,	// U+00B0
		N_A_, N_A_, N_A_, 0xaf
	};
	const Byte UCStoARMSCII8A_0530[] = {
		N_A_, 0x80, 0x82, 0x84, 0x86, 0x88, 0x8a, 0x8c,	// U+0530
		0x8e, 0x90, 0x92, 0x94, 0x96, 0x98, 0x9a, 0x9c,
		0x9e, 0xa0, 0xa2, 0xa4, 0xa6, 0xa8, 0xaa, 0xac,	// U+0540
		0xe0, 0xe2, 0xe4, 0xe6, 0xe8, 0xea, 0xec, 0xee,
		0xf0, 0xf2, 0xf4, 0xf6, 0xf8, 0xfa, 0xfc, N_A_,	// U+0550
		N_A_, N_A_, 0xfe, 0x27, 0x7e, 0x60, 0xdf, N_A_,
		N_A_, 0x81, 0x83, 0x85, 0x87, 0x89, 0x8b, 0x8d,	// U+0560
		0x8f, 0x91, 0x93, 0x95, 0x97, 0x99, 0x9b, 0x9d,
		0x9f, 0xa1, 0xa3, 0xa5, 0xa7, 0xa9, 0xab, 0xad,	// U+0570
		0xe1, 0xe3, 0xe5, 0xe7, 0xe9, 0xeb, 0xed, 0xef,
		0xf1, 0xf3, 0xf5, 0xf7, 0xf9, 0xfb, 0xfd, 0x3a,	// U+0580
		N_A_, N_A_, 0xdd
	};
	const Byte UCStoARMSCII8A_2010[] = {
		N_A_, N_A_, N_A_, N_A_, 0x2d, N_A_, N_A_, N_A_,	// U+2010
		N_A_, N_A_, N_A_, N_A_, N_A_, N_A_, N_A_, N_A_,
		N_A_, N_A_, N_A_, N_A_, N_A_, N_A_, 0xde		// U+2020
	};
#endif // !ASCENSION_NO_MINORITY_ENCODINGS

	inline const Char* decomposeArmenianLigature(Char c) {
		static const Char ECH_YIWN[] = {0x0565u, 0x0582u, 0};
		static const Char MEN_NOW[] = {0x0574u, 0x0576u, 0};
		static const Char MEN_ECH[] = {0x0574u, 0x0565u, 0};
		static const Char MEN_INI[] = {0x0574u, 0x056bu, 0};
		static const Char VEW_NOW[] = {0x057eu, 0x0576u, 0};
		static const Char MEN_XEH[] = {0x0574u, 0x056du, 0};
		switch(c) {
		case 0x0587u:	return ECH_YIWN;
		case 0xfb13u:	return MEN_NOW;
		case 0xfb14u:	return MEN_ECH;
		case 0xfb15u:	return MEN_INI;
		case 0xfb16u:	return VEW_NOW;
		case 0xfb17u:	return MEN_XEH;
		default:		return 0;
		}
	}


	// ARMSCII-8 //////////////////////////////////////////////////////////////////////////////////

	template<> ARMSCII<8>::ARMSCII() /*throw()*/ : EncoderFactoryBase("ARMSCII-8", MIB_OTHER, "Armenian (ARMSCII-8)", 1, 2, "", 0x1a) {
	}

	template<> Encoder::Result ARMSCII<8>::InternalEncoder::doFromUnicode(
			Byte* to, Byte* toEnd, Byte*& toNext, const Char* from, const Char* fromEnd, const Char*& fromNext) {
		for(; to < toEnd && from < fromEnd; ++to, ++from) {
			if(*from < 0x0028) {
				*to = mask8Bit(*from);
				continue;
			} else if(*from < 0x0028 + ASCENSION_COUNTOF(UCStoARMSCII8_0028))
				*to = UCStoARMSCII8_0028[*from - 0x0028];
			else if(*from < 0x00a0 + ASCENSION_COUNTOF(UCStoARMSCII78_00A0))
				*to = UCStoARMSCII78_00A0[*from - 0x00a0];
			else if(*from < 0x0530 + ASCENSION_COUNTOF(UCStoARMSCII78_0530))
				*to = UCStoARMSCII78_0530[*from - 0x0530];
			else if(*from < 0x2010 + ASCENSION_COUNTOF(UCStoARMSCII78_2010))
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
				if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTERS) {
					--to;
					continue;
				} else if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTERS) {
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
			Char* to, Char* toEnd, Char*& toNext, const Byte* from, const Byte* fromEnd, const Byte*& fromNext) {
		for(; to < toEnd && from < fromEnd; ++to, ++from) {
			if(*from < 0xa1)
				*to = *from;
			else if(ARMSCII78toUCS_20[*from - 0x20 - 0x80] != text::REPLACEMENT_CHARACTER)
				*to = ARMSCII78toUCS_20[*from - 0x20 - 0x80];
			else if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTERS)
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

	// ARMSCII-7 //////////////////////////////////////////////////////////////////////////////////

	template<> ARMSCII<7>::ARMSCII() /*throw()*/ : EncoderFactoryBase("ARMSCII-7", MIB_OTHER, "Armenian (ARMSCII-7)", 1, 2, "", 0x1a) {
	}

	template<> Encoder::Result ARMSCII<7>::InternalEncoder::doFromUnicode(
			Byte* to, Byte* toEnd, Byte*& toNext, const Char* from, const Char* fromEnd, const Char*& fromNext) {
		for(; to < toEnd && from < fromEnd; ++to, ++from) {
			if(*from < 0x0028) {
				*to = mask8Bit(*from);
				continue;
			} else if(*from < 0x0028 + ASCENSION_COUNTOF(UCStoARMSCII7_0028))
				*to = UCStoARMSCII7_0028[*from - 0x0028];
			else if(*from < 0x00a0 + ASCENSION_COUNTOF(UCStoARMSCII78_00A0))
				*to = UCStoARMSCII78_00A0[*from - 0x00a0];
			else if(*from < 0x0530 + ASCENSION_COUNTOF(UCStoARMSCII78_0530))
				*to = UCStoARMSCII78_0530[*from - 0x0530];
			else if(*from < 0x2010 + ASCENSION_COUNTOF(UCStoARMSCII78_2010))
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
				if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTERS)
					--to;
				else if(substitutionPolicy() != REPLACE_UNMAPPABLE_CHARACTERS) {
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
			Char* to, Char* toEnd, Char*& toNext, const Byte* from, const Byte* fromEnd, const Byte*& fromNext) {
		for(; to < toEnd && from < fromEnd; ++to, ++from) {
			if(*from < 0x20)
				*to = *from;
			else if(*from < 0x20 + ASCENSION_COUNTOF(ARMSCII78toUCS_20)
					&& ARMSCII78toUCS_20[*from - 0x20] != text::REPLACEMENT_CHARACTER)
				*to = ARMSCII78toUCS_20[*from - 0x20];
			else if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTERS)
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


	// ARMSCII-8A /////////////////////////////////////////////////////////////////////////////////

	template<> ARMSCII<0x8a>::ARMSCII() /*throw()*/ : EncoderFactoryBase("ARMSCII-8A", MIB_OTHER, "Armenian (ARMSCII-8A)", 1, 2, "", 0x1a) {
	}

	template<> Encoder::Result ARMSCII<0x8a>::InternalEncoder::doFromUnicode(
			Byte* to, Byte* toEnd, Byte*& toNext, const Char* from, const Char* fromEnd, const Char*& fromNext) {
		for(; to < toEnd && from < fromEnd; ++to, ++from) {
			if(*from < 0x80) {
				static const Char invChars[] = {0x0027, 0x003a, 0x005f, 0x0060, 0x007e};
				*to = binary_search(invChars, ASCENSION_ENDOF(invChars), *from) ? mask8Bit(*from) : props_.substitutionCharacter();
			} else if(*from < 0x00a8)
				*to = props_.substitutionCharacter();
			else if(*from < 0x00a8 + ASCENSION_COUNTOF(UCStoARMSCII8A_00A8))
				*to = UCStoARMSCII8A_00A8[*from - 0x00a8];
			else if(*from < 0x0530 + ASCENSION_COUNTOF(UCStoARMSCII8A_0530))
				*to = UCStoARMSCII8A_0530[*from - 0x0530];
			else if(*from < 0x2010 + ASCENSION_COUNTOF(UCStoARMSCII8A_2010))
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
				if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTERS)
					--to;
				else if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTERS) {
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

	template<> Encoder::Result ARMSCII<0x8a>::InternalEncoder::doToUnicode(
			Char* to, Char* toEnd, Char*& toNext, const Byte* from, const Byte* fromEnd, const Byte*& fromNext) {
		for(; to < toEnd && from < fromEnd; ++to, ++from) {
			if(*from < 0x20)
				*to = *from;
			else if(*from < 0x20 + ASCENSION_COUNTOF(ARMSCII8AtoUCS_20))
				*to = ARMSCII8AtoUCS_20[*from - 0x20];
			else
				*to = ARMSCII8AtoUCS_D8[*from - 0xd8];
			if(*to == text::REPLACEMENT_CHARACTER) {
				if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTERS)
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

#endif // !ASCENSION_NO_MINORITY_ENCODINGS


	ARMSCII<8> ARMSCII_8;
#ifndef ASCENSION_NO_MINORITY_ENCODINGS
	ARMSCII<7> ARMSCII_7;
	ARMSCII<0x8a> ARMSCII_8A;
#endif // !ASCENSION_NO_MINORITY_ENCODINGS

	struct Installer {
		Installer() {
			Encoder::registerFactory(ARMSCII_8);
#ifndef ASCENSION_NO_MINORITY_ENCODINGS
			Encoder::registerFactory(ARMSCII_7);
			Encoder::registerFactory(ARMSCII_8A);
#endif // !ASCENSION_NO_MINORITY_ENCODINGS
			EncodingDetector::registerDetector(unique_ptr<EncodingDetector>(new ArmenianDetector));
		}
	} installer;


	// ArmenianDetector ///////////////////////////////////////////////////////////////////////////

	/// @see EncodingDetector#doDetect
	pair<MIBenum, string> ArmenianDetector::doDetect(const Byte* first, const Byte* last, ptrdiff_t* convertibleBytes) const /*throw()*/ {
		// first, check if Unicode
		if(const EncodingDetector* unicodeDetector = forName("UnicodeAutoDetect")) {
			ptrdiff_t temp;
			pair<MIBenum, string> result(unicodeDetector->detect(first, last, &temp));
			if(temp == last - first) {
				if(convertibleBytes != 0)
					*convertibleBytes = temp;
				return result;
			}
		}

		const EncodingProperties* props = 0;
#ifdef ASCENSION_NO_MINORITY_ENCODINGS
		props = &ARMSCII_8;
		for(; first < last; ++first) {
			if(*first >= 0x80 && *from < 0xa0)
				break;
		}
#else
		bitset<3> candidates;	// 0:-7, 1:-8, 2:-8A
		candidates.set();
		for(; first < last; ++first) {
			const Byte c = *first;
			if(c >= 0x80)				candidates.reset(0);	// ARMSCII-7 consists of only 7-bits
			if(c >= 0x80 && c < 0xa0)	candidates.reset(1);	// 8-bit controls (but ARMSCII-8 may contain these)
			if(c >= 0xb0 && c < 0xdc)	candidates.reset(2);
			if(candidates.none())
				break;
		}
		if(candidates.none() || candidates.test(1))
			props = &ARMSCII_8;
		else if(candidates.test(2))
			props = &ARMSCII_8A;
		else
			props = &ARMSCII_7;
#endif // ASCENSION_NO_MINORITY_ENCODINGS

		return make_pair(props->mibEnum(), props->name());
	}

} // namespace @0

#endif // !ASCENSION_NO_STANDARD_ENCODINGS

