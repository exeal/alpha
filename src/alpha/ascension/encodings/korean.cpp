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
using namespace std;

namespace {
	const Char** const UCS_TO_UHC[256] = {
#include "windows-949.dat"
	};

	template<typename Factory>
	class InternalEncoder : public Encoder {
	public:
		explicit InternalEncoder(const Factory& factory) throw() : props_(factory), encodingState_(0), decodingState_(0) {}
	private:
		Result doFromUnicode(byte* to, byte* toEnd, byte*& toNext,
			const Char* from, const Char* fromEnd, const Char*& fromNext);
		Result doToUnicode(Char* to, Char* toEnd, Char*& toNext,
			const byte* from, const byte* fromEnd, const byte*& fromNext);
		const IEncodingProperties& properties() const throw() {return props_;}
		Encoder& resetDecodingState() throw() {decodingState_ = 0; return *this;}
		Encoder& resetEncodingState() throw() {encodingState_ = 0; return *this;}
	private:
		const IEncodingProperties& props_;
		byte encodingState_, decodingState_;
	};

	class UHC : public EncoderFactoryBase {
	public:
		UHC() throw() : EncoderFactoryBase("UHC", standard::UHC, "Korean (UHC)", 2, 1,
			"KS_C_5601-1987|iso-ir-149|KS_C_5601-1989|KSC_5601|korean|csKSC56011987"
			"\0ibm-1363|5601|cp1363|ksc|windows-949|ibm-1363_VSUB_VPUA|ms949|ibm-1363_P11B-1998|windows-949-2000", 0x3F) {}
	private:
		auto_ptr<Encoder> create() const throw() {return auto_ptr<Encoder>(new InternalEncoder<UHC>(*this));}
	} uhc;
/*
	class EUC_KR : public EncoderFactoryBase {
	public:
		EUC_KR() throw() : EncoderFactoryBase("EUC-KR", standard::EUC_KR, "Korean (EUC-KR)", 2, 1,
			"csEUCKR" "\0ibm-970KS_C_5601-1987|windows-51949|ibm-eucKR|KSC_5601|5601|cp970|970|ibm-970-VPUA|ibm-970_P110_P110-2006_U2") {}
	private:
		auto_ptr<Encoder> create() const throw() {return auto_ptr<Encoder>(new InternalEncoder<EUC_KR>(*this));}
	} euckr;
*/
	struct Installer {
		Installer() {
			Encoder::registerFactory(uhc);
//			Encoder::registerFactory(euckr);
		}
	} installer;
}

template<> Encoder::Result InternalEncoder<UHC>::doFromUnicode(
		byte* to, byte* toEnd, byte*& toNext, const Char* from, const Char* fromEnd, const Char*& fromNext) {
	for(; to < toEnd && from < fromEnd; ++from) {
		if(*from < 0x80)
			*(to++) = mask8Bit(*from);
		else {
			if(const Char** wire = UCS_TO_UHC[mask8Bit(*from >> 8)]) {
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
	fromNext = from;
	toNext = to;
	return (from == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
}