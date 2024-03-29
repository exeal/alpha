/**
 * @file irish.cpp
 * Implements Irish encodings. This includes:
 * - I.S. 434:1999 (for old Irelandic). For details of this encoding, see
 *   http://www.evertype.com/standards/iso10646/pdf/is434.pdf. In this implementation, native
 *   0xFD..0xFF characters are mapped to UCS U+169D..169F. And also non-Ogham characters shown in
 *   the above document are mapped.
 * @author exeal
 * @date 2005-2010, 2014
 */

#ifndef ASCENSION_NO_MINORITY_ENCODINGS
#include <ascension/corelib/encoding/encoder.hpp>
#include <ascension/corelib/encoding/encoder-implementation.hpp>

namespace ascension {
	namespace encoding {
		namespace implementation {
			namespace sbcs {
				namespace {
					struct Installer {
						Installer() {
							EncoderRegistry::instance().registerFactory(std::make_shared<
								SingleByteEncoderFactory<
									ISO8859CompatibleCharWire<
										CharLine<0x00a0, 0xfffd, 0xfffd, 0x00a3, 0xfffd, 0xfffd, 0xfffd, 0x00a7, 0xfffd, 0x00a9, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0x00ae, 0xfffd>,
										CharLine<0x00b0, 0x00b1, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0x00b6, 0x00b7, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd>,
										CharLine<0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd>,
										CharLine<0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd>,
										CharLine<0x1680, 0x1681, 0x1682, 0x1683, 0x1684, 0x1685, 0x1686, 0x1687, 0x1688, 0x1689, 0x168a, 0x168b, 0x168c, 0x168d, 0x168e, 0x168f>,
										CharLine<0x1690, 0x1691, 0x1692, 0x1693, 0x1694, 0x1695, 0x1696, 0x1697, 0x1698, 0x1699, 0x169a, 0x169b, 0x169c, 0x169d, 0x169e, 0x169f>
									>
								>
							>("I.S. 434", MIB_OTHER, "Irish (I.S. 434)", "", 0x1a));
						}
					} installer;
				}
			}
		}
	}
}

#endif // !ASCENSION_NO_MINORITY_ENCODINGS
