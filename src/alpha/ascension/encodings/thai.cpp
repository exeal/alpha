/**
 * @file thai.cpp
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
	ASCENSION_DEFINE_SBCS_ENCODER(TIS620Encoder, extended::TIS620, "TIS-620")
	ASCENSION_DEFINE_SBCS_ENCODER(ISO885911Encoder, extended::ISO_8859_11, "ISO-8859-11")

	struct Installer {
		Installer() {
			Encoder::registerEncoder(auto_ptr<Encoder>(new TIS620Encoder));
			Encoder::registerEncoder(auto_ptr<Encoder>(new ISO885911Encoder));
		}
	} installer;
} // namespace @0

namespace {
	template<bool iso8859> inline bool fromThaiNative(Char& to, uchar from) {
		if(from < 0x80)
			to = from;
		else if(from == 0xA0) {
			if(!iso8859)
				return false;
			to = 0x00A0;
		} else if(from >= 0xA1 && from <= 0xDA)
			to = from + 0x0D60;
		else if(from >= 0xDF && from <= 0xFB)
			to = from + 0x0D60;
		else
			return false;
		return true;
	}
	template<bool iso8859> inline bool toThaiNative(uchar& to, Char from) {
		assert(from != 0);
		if(from < 0x0080)
			to = mask8Bit(from);
		else if(from == 0x00A0) {
			if(!iso8859)
				return false;
			to = 0xA0;
		} else if(from >= 0x0E01 && from <= 0x0E3A)
			to = mask8Bit(from - 0x0D60);
		else if(from >= 0x0E3F && from <= 0x0E5B)
			to = mask8Bit(from - 0x0D60);
		else
			return false;
		return true;
	}
} // namespace @0


// TIS620Encoder ////////////////////////////////////////////////////////////

/// @see SBCSEncoder#doFromUnicode
inline bool TIS620Encoder::doFromUnicode(uchar& to, Char from) const {
	return toThaiNative<false>(to, from);
}

/// @see Encoder#doToUnicode
inline bool TIS620Encoder::doToUnicode(Char& to, uchar from) const {
	return fromThaiNative<false>(to, from);
}


// ISO885911Encoder /////////////////////////////////////////////////////////

/// @see Encoder#doFromUnicode
inline bool ISO885911Encoder::doFromUnicode(uchar& to, Char from) const {
	return toThaiNative<true>(to, from);
}

/// @see Encoder#doToUnicode
inline bool ISO885911Encoder::doToUnicode(Char& to, uchar from) const {
	return fromThaiNative<true>(to, from);
}

#endif /* !ASCENSION_NO_EXTENDED_ENCODINGS */
