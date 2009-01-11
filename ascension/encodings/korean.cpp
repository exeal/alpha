/**
 * @file korean.cpp
 * This file implements the following encodings for Korean:
 * - EUC-KR
 * - UHC
 * - JOHAB
 * - ISO-2022-KR
 * @author exeal
 * @date 2007-2008
 */

#include "../encoder.hpp"
using namespace ascension;
using namespace ascension::encoding;
using namespace ascension::encoding::implementation;
using namespace ascension::encoding::implementation::dbcs;
using namespace std;

namespace {
	const Char** const UCS_TO_UHC[256] = {
#include "data/ucs-to-windows-949.dat"
	};
	const ushort** const UHC_TO_UCS[256] = {
#include "data/windows-949-to-ucs.dat"
	};

	template<typename Factory>
	class InternalEncoder : public Encoder {
	public:
		explicit InternalEncoder(const Factory& factory) ASC_NOFAIL : props_(factory), encodingState_(0), decodingState_(0) {}
	private:
		Result doFromUnicode(byte* to, byte* toEnd, byte*& toNext,
			const Char* from, const Char* fromEnd, const Char*& fromNext);
		Result doToUnicode(Char* to, Char* toEnd, Char*& toNext,
			const byte* from, const byte* fromEnd, const byte*& fromNext);
		const IEncodingProperties& properties() const ASC_NOFAIL {return props_;}
		Encoder& resetDecodingState() ASC_NOFAIL {decodingState_ = 0; return *this;}
		Encoder& resetEncodingState() ASC_NOFAIL {encodingState_ = 0; return *this;}
	private:
		const IEncodingProperties& props_;
		byte encodingState_, decodingState_;
	};

	class UHC : public EncoderFactoryBase {
	public:
		UHC() ASC_NOFAIL : EncoderFactoryBase("UHC", standard::UHC, "Korean (UHC)", 2, 1,
			"KS_C_5601-1987|iso-ir-149|KS_C_5601-1989|KSC_5601|korean|csKSC56011987"
			"\0ibm-1363|5601|cp1363|ksc|windows-949|ibm-1363_VSUB_VPUA|ms949|ibm-1363_P11B-1998|windows-949-2000", 0x3F) {}
	private:
		auto_ptr<Encoder> create() const ASC_NOFAIL {return auto_ptr<Encoder>(new InternalEncoder<UHC>(*this));}
	} uhc;

	class EUC_KR : public EncoderFactoryBase {
	public:
		EUC_KR() ASC_NOFAIL : EncoderFactoryBase("EUC-KR", standard::EUC_KR, "Korean (EUC-KR)", 2, 1,
			"csEUCKR" "\0ibm-970KS_C_5601-1987|windows-51949|ibm-eucKR|KSC_5601|5601|cp970|970|ibm-970-VPUA|ibm-970_P110_P110-2006_U2") {}
	private:
		auto_ptr<Encoder> create() const ASC_NOFAIL {return auto_ptr<Encoder>(new InternalEncoder<EUC_KR>(*this));}
	} euckr;

	class ISO_2022_KR : public EncoderFactoryBase {
	public:
		ISO_2022_KR() ASC_NOFAIL : EncoderFactoryBase("ISO-2022-KR", standard::ISO_2022_KR, "Korean (ISO-2022-KR)", 7, 1, "csISO2022KR") {}
	private:
		auto_ptr<Encoder> create() const ASC_NOFAIL {return auto_ptr<Encoder>(new InternalEncoder<ISO_2022_KR>(*this));}
	} iso2022kr;

	struct Installer {
		Installer() {
			Encoder::registerFactory(uhc);
			Encoder::registerFactory(euckr);
			Encoder::registerFactory(iso2022kr);
		}
	} installer;
}


// UHC //////////////////////////////////////////////////////////////////////

template<> Encoder::Result InternalEncoder<UHC>::doFromUnicode(
		byte* to, byte* toEnd, byte*& toNext, const Char* from, const Char* fromEnd, const Char*& fromNext) {
	for(; to < toEnd && from < fromEnd; ++from) {
		if(*from < 0x80)
			*(to++) = mask8Bit(*from);
		else {	// double byte character
			if(const Char** const wire = UCS_TO_UHC[mask8Bit(*from >> 8)]) {
				if(const ushort dbcs = wireAt(wire, mask8Bit(*from))) {
					if(to + 1 >= toEnd)
						break;	// the destnation buffer is insufficient
					*(to++) = mask8Bit(dbcs >> 8);
					*(to++) = mask8Bit(dbcs >> 0);
					continue;
				}
			}
			if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTER)
				*(to++) = properties().substitutionCharacter();
			else if(substitutionPolicy() != IGNORE_UNMAPPABLE_CHARACTER) {
				toNext = to;
				fromNext = from;
				return UNMAPPABLE_CHARACTER;
			}
		}
	}
	fromNext = from;
	toNext = to;
	return (from == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
}
		
template<> Encoder::Result InternalEncoder<UHC>::doToUnicode(
		Char* to, Char* toEnd, Char*& toNext, const byte* from, const byte* fromEnd, const byte*& fromNext) {
	while(to < toEnd && from < fromEnd) {
		if(*from < 0x80)
			*(to++) = *(from++);
		else if(from + 1 >= fromEnd) {	// remaining lead byte
			toNext = to;
			fromNext = from;
			return flags().has(END_OF_BUFFER) ? MALFORMED_INPUT : COMPLETED;
		} else {	// double byte character
			if(const ushort** const wire = UHC_TO_UCS[mask8Bit(*from)]) {
				const Char ucs = wireAt(wire, mask8Bit(from[1]));
				if(ucs != REPLACEMENT_CHARACTER) {
					*(to++) = ucs;
					from += 2;
					continue;
				}
			}
			if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTER) {
				*(to++) = REPLACEMENT_CHARACTER;
				from += 2;
			} else if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTER)
				from += 2;
			else {
				toNext = to;
				fromNext = from;
				return UNMAPPABLE_CHARACTER;
			}
		}
	}
	fromNext = from;
	toNext = to;
	return (from == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
}


// EUC-KR ///////////////////////////////////////////////////////////////////

template<> Encoder::Result InternalEncoder<EUC_KR>::doFromUnicode(
		byte* to, byte* toEnd, byte*& toNext, const Char* from, const Char* fromEnd, const Char*& fromNext) {
	for(; to < toEnd && from < fromEnd; ++from) {
		if(*from < 0x80)
			*(to++) = mask8Bit(*from);
		else {	// double byte character
			if(const Char** const wire = UCS_TO_UHC[mask8Bit(*from >> 8)]) {
				if(const ushort dbcs = wireAt(wire, mask8Bit(*from))) {
					const byte lead = mask8Bit(dbcs >> 8), trail = mask8Bit(dbcs);
					if(lead - 0xA1U < 0x5E && trail - 0xA1 < 0x5E) {
//					if(lead >= 0xA1 && lead <= 0xFE && trail >= 0xA1 && trail <= 0xFE) {
						if(to + 1 >= toEnd)
							break;	// the destnation buffer is insufficient
						*(to++) = lead;
						*(to++) = trail;
						continue;
					}
				}
			}
			if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTER)
				*(to++) = properties().substitutionCharacter();
			else if(substitutionPolicy() != IGNORE_UNMAPPABLE_CHARACTER) {
				toNext = to;
				fromNext = from;
				return UNMAPPABLE_CHARACTER;
			}
		}
	}
	fromNext = from;
	toNext = to;
	return (from == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
}
		
template<> Encoder::Result InternalEncoder<EUC_KR>::doToUnicode(
		Char* to, Char* toEnd, Char*& toNext, const byte* from, const byte* fromEnd, const byte*& fromNext) {
	while(to < toEnd && from < fromEnd) {
		if(*from < 0x80)
			*(to++) = *(from++);
		else if(from + 1 >= fromEnd) {	// remaining lead byte
			toNext = to;
			fromNext = from;
			return flags().has(END_OF_BUFFER) ? MALFORMED_INPUT : COMPLETED;
		} else {	// double byte character
			if(from[0] - 0xA1U > 0x5DU || from[1] - 0xA1U > 0x5DU) {
//			if(!(from[0] >= 0xA1 && from[0] <= 0xFE) || !(from[1] >= 0xA1 && from[1] <= 0xFE)) {
				toNext = to;
				fromNext = from;
				return MALFORMED_INPUT;
			} else if(const ushort** const wire = UHC_TO_UCS[mask8Bit(*from)]) {
				const Char ucs = wireAt(wire, mask8Bit(from[1]));
				if(ucs != REPLACEMENT_CHARACTER) {
					*(to++) = ucs;
					from += 2;
					continue;
				}
			}
			if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTER) {
				*(to++) = REPLACEMENT_CHARACTER;
				from += 2;
			} else if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTER)
				from += 2;
			else {
				toNext = to;
				fromNext = from;
				return UNMAPPABLE_CHARACTER;
			}
		}
	}
	fromNext = from;
	toNext = to;
	return (from == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
}


// ISO-2022-KR //////////////////////////////////////////////////////////////

// state definition
// 0 : initial, 1 : encountered/written escape sequence and ASCII, 2 : KS C 5601 (KS X 1001)

template<> Encoder::Result InternalEncoder<ISO_2022_KR>::doFromUnicode(
		byte* to, byte* toEnd, byte*& toNext, const Char* from, const Char* fromEnd, const Char*& fromNext) {
	if(encodingState_ == 0) {
		// write an escape sequence
		if(to + 3 >= toEnd) {
			toNext = to;
			fromNext = from;
			return INSUFFICIENT_BUFFER;
		}
		memcpy(to, "\x1B$)C", 4);
		++encodingState_;
		to += 4;
	}
	for(; to < toEnd && from < fromEnd; ++from) {
		if(*from < 0x80) {
			if(encodingState_ == 2) {
				// introduce ASCII character set
				*(to++) = SI;
				--encodingState_;
				if(to == toEnd)
					break;
			}
			*(to++) = mask8Bit(*from);
		} else {	// double byte character
			if(encodingState_ == 1) {
				// introduce KS C 5601 (KS X 1001) character set
				*(to++) = SO;
				++encodingState_;
				if(to == toEnd)
					break;
			}
			if(const Char** const wire = UCS_TO_UHC[mask8Bit(*from >> 8)]) {
				if(const ushort dbcs = wireAt(wire, mask8Bit(*from))) {
					const byte lead = mask8Bit(dbcs >> 8), trail = mask8Bit(dbcs);
					if(lead - 0xA1U < 0x5E && trail - 0xA1 < 0x5E) {
//					if(lead >= 0xA1 && lead <= 0xFE && trail >= 0xA1 && trail <= 0xFE) {
						if(to + 1 >= toEnd)
							break;	// the destnation buffer is insufficient
						*(to++) = mask7Bit(lead);
						*(to++) = mask7Bit(trail);
						continue;
					}
				}
			}
			if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTER)
				*(to++) = properties().substitutionCharacter();
			else if(substitutionPolicy() != IGNORE_UNMAPPABLE_CHARACTER) {
				toNext = to;
				fromNext = from;
				return UNMAPPABLE_CHARACTER;
			}
		}
	}
	fromNext = from;
	toNext = to;
	return (from == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
}

template<> Encoder::Result InternalEncoder<ISO_2022_KR>::doToUnicode(
		Char* to, Char* toEnd, Char*& toNext, const byte* from, const byte* fromEnd, const byte*& fromNext) {
	if(decodingState_ == 0)
		++decodingState_;
	while(to < toEnd && from < fromEnd) {
		if((*from & 0x80) != 0) {
			// reject 8-bit character
			toNext = to;
			fromNext = from;
			return MALFORMED_INPUT;
		} else if(*from == SI) {
			// introduce ASCII character set
			decodingState_ = 2;
			++from;
		} else if(*from == SO) {
			// introduce KS C 5601 (KS X 1001) character set
			decodingState_ = 3;
			++from;
		} else if(*from == ESC) {
			if(from + 3 >= fromEnd || memcmp(from + 1, "$)C", 3) != 0) {
				// invalid escape sequence
				toNext = to;
				fromNext = from;
				return MALFORMED_INPUT;
			}
			from += 4;
		} else if(decodingState_ == 1)
			*(to++) = *(from++);
		else if(from + 1 >= fromEnd) {	// remaining lead byte
			toNext = to;
			fromNext = from;
			return flags().has(END_OF_BUFFER) ? MALFORMED_INPUT : COMPLETED;
		} else {	// double byte character
			if(from[0] - 0x21U > 0x5DU || from[1] - 0x21U > 0x5DU) {
//			if(!(from[0] >= 0x21 && from[0] <= 0x7E) || !(from[1] >= 0x21 && from[1] <= 0x7E)) {
				toNext = to;
				fromNext = from;
				return MALFORMED_INPUT;
			} else if(const ushort** const wire = UHC_TO_UCS[mask8Bit(*from + 0x80)]) {
				const Char ucs = wireAt(wire, mask8Bit(from[1] + 0x80));
				if(ucs != REPLACEMENT_CHARACTER) {
					*(to++) = ucs;
					from += 2;
					continue;
				}
			}
			if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTER) {
				*(to++) = REPLACEMENT_CHARACTER;
				from += 2;
			} else if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTER)
				from += 2;
			else {
				toNext = to;
				fromNext = from;
				return UNMAPPABLE_CHARACTER;
			}
		}
	}
	fromNext = from;
	toNext = to;
	return (from == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
}