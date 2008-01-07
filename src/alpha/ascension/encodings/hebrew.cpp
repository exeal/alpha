/**
 * @file hebrew.cpp
 * Implements Hebrew encodings. This includes:
 * - ISO 8859-8:1999
 * - IBM867
 * - IBM4899
 * - IBM4952
 * - IBM9048
 * - IBM12712
 * - windows-1255
 * - <del>MacHebrew</del>
 * @author exeal
 * @date 2007-12-29 .. 2008-01-02
 */

#include "../encoder.hpp"
using namespace ascension::encoding;
using namespace ascension::encoding::implementation::sbcs;
using namespace std;

namespace {
#ifndef ASCENSION_NO_STANDARD_ENCODINGS
	SingleByteEncoderFactory<
		ISO646CompatibleByteTable<
			ByteLine<0x00A0, 0xFFFD, 0x00A2, 0x00A3, 0x00A4, 0x00A5, 0x00A6, 0x00A7, 0x00A8, 0x00A9, 0x00D7, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF>,
			ByteLine<0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00B4, 0x00B5, 0x00B6, 0x00B7, 0x00B8, 0x00B9, 0x00F7, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0xFFFD>,
			ByteLine<0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD>,
			ByteLine<0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0x2017>,
			ByteLine<0x05D0, 0x05D1, 0x05D2, 0x05D3, 0x05D4, 0x05D5, 0x05D6, 0x05D7, 0x05D8, 0x05D9, 0x05DA, 0x05DB, 0x05DC, 0x05DD, 0x05DE, 0x05DF>,
			ByteLine<0x05E0, 0x05E1, 0x05E2, 0x05E3, 0x05E4, 0x05E5, 0x05E6, 0x05E7, 0x05E8, 0x05E9, 0x05EA, 0xFFFD, 0xFFFD, 0x200E, 0x200F, 0xFFFD>
		>
	> ISO_8859_8("ISO-8859-8", standard::ISO_8859_8, "Hebrew (ISO 8859-8)",
		"iso-ir-138|ISO_8859-8|hebrew|csISOLatinHebrew" "\0ibm-5012|ISO-8859-8-I|ISO-8859-8-E|8859_8|windows-28598|ibm-5012_P100-1999", 0x1A);
#endif /* !ASCENSION_NO_STANDARD_ENCODINGS */
#ifndef ASCENSION_NO_PROPRIETARY_ENCODINGS
	SingleByteEncoderFactory<
		IBMPCCompatibleByteTable<
			ByteLine<0x05D0, 0x05D1, 0x05D2, 0x05D3, 0x05D4, 0x05D5, 0x05D6, 0x05D7, 0x05D8, 0x05D9, 0x05DA, 0x05DB, 0x05DC, 0x05DD, 0x05DE, 0x05DF>,
			ByteLine<0x05E0, 0x05E1, 0x05E2, 0x05E3, 0x05E4, 0x05E5, 0x05E6, 0x05E7, 0x05E8, 0x05E9, 0x05EA, 0x00A2, 0x00A3, 0x00A5, 0xFFFD, 0x20AA>,
			ByteLine<0x200E, 0x200F, 0x202A, 0x202B, 0x202D, 0x202E, 0x202C, 0xFFFD, 0xFFFD, 0x2310, 0x00AC, 0x00BD, 0x00BC, 0x20AC, 0x00AB, 0x00BB>,
			ByteLine<0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556, 0x2555, 0x2563, 0x2551, 0x2557, 0x255D, 0x255C, 0x255B, 0x2510>,
			ByteLine<0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x255E, 0x255F, 0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2567>,
			ByteLine<0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256B, 0x256A, 0x2518, 0x250C, 0x2588, 0x2584, 0x258C, 0x2590, 0x2580>,
			ByteLine<0x03B1, 0x00DF, 0x0393, 0x03C0, 0x03A3, 0x03C3, 0x03BC, 0x03C4, 0x03A6, 0x0398, 0x03A9, 0x03B4, 0x221E, 0x03C6, 0x03B5, 0x2229>,
			ByteLine<0x2261, 0x00B1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00F7, 0x2248, 0x00B0, 0x2219, 0x00B7, 0x221A, 0x207F, 0x00B2, 0x25A0, 0x00A0>
		>
	> IBM867("IBM867", MIB_OTHER, "Hebrew (IBM867)", "\0ibm-867|ibm-867_P100-1998", 0x7F);
	SingleByteEncoderFactory<
		ByteTable<
			ByteLine<0x0000, 0x0001, 0x0002, 0x0003, 0x009C, 0x0009, 0x0086, 0x007F, 0x0097, 0x008D, 0x008E, 0x000B, 0x000C, 0x000D, 0x000E, 0x000F>,
			ByteLine<0x0010, 0x0011, 0x0012, 0x0013, 0x009D, 0x0085, 0x0008, 0x0087, 0x0018, 0x0019, 0x0092, 0x008F, 0x001C, 0x001D, 0x001E, 0x001F>,
			ByteLine<0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x000A, 0x0017, 0x001B, 0x0088, 0x0089, 0x008A, 0x008B, 0x008C, 0x0005, 0x0006, 0x0007>,
			ByteLine<0x0090, 0x0091, 0x0016, 0x0093, 0x0094, 0x0095, 0x0096, 0x0004, 0x0098, 0x0099, 0x009A, 0x009B, 0x0014, 0x0015, 0x009E, 0x001A>,
			ByteLine<0x0020, 0x05D0, 0x05D1, 0x05D2, 0x05D3, 0x05D4, 0x05D5, 0x05D6, 0x05D7, 0x05D8, 0x00A2, 0x002E, 0x003C, 0x0028, 0x002B, 0x007C>,
			ByteLine<0x0026, 0x05D9, 0x05DA, 0x05DB, 0x05DC, 0x05DD, 0x05DE, 0x05DF, 0x05E0, 0x05E1, 0x0021, 0x0024, 0x002A, 0x0029, 0x003B, 0x00AC>,
			ByteLine<0x002D, 0x002F, 0x05E2, 0x05E3, 0x05E4, 0x05E5, 0x05E6, 0x05E7, 0x05E8, 0x05E9, 0x00A6, 0x002C, 0x0025, 0x005F, 0x003E, 0x003F>,
			ByteLine<0xFFFD, 0x05EA, 0xFFFD, 0xFFFD, 0x00A0, 0xFFFD, 0xFFFD, 0xFFFD, 0x2017, 0x0060, 0x003A, 0x0023, 0x0040, 0x0027, 0x003D, 0x0022>,
			ByteLine<0xFFFD, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x00AB, 0x00BB, 0xFFFD, 0xFFFD, 0xFFFD, 0x00B1>,
			ByteLine<0x00B0, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F, 0x0070, 0x0071, 0x0072, 0xFFFD, 0xFFFD, 0x20AC, 0x00B8, 0x20AA, 0x00A4>,
			ByteLine<0x00B5, 0x007E, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007A, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0x00AE>,
			ByteLine<0x005E, 0x00A3, 0x00A5, 0x2022, 0x00A9, 0x00A7, 0x00B6, 0x00BC, 0x00BD, 0x00BE, 0x005B, 0x005D, 0x203E, 0x00A8, 0x00B4, 0x00D7>,
			ByteLine<0x007B, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x00AD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD>,
			ByteLine<0x007D, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F, 0x0050, 0x0051, 0x0052, 0x00B9, 0x202D, 0x202E, 0x202C, 0xFFFD, 0xFFFD>,
			ByteLine<0x005C, 0x00F7, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005A, 0x00B2, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD>,
			ByteLine<0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x00B3, 0x202A, 0x202B, 0x200E, 0x200F, 0x009F>
		>
	> IBM12712("IBM12712", MIB_OTHER, "Hebrew (IBM12712)", "\0ibm-12712|ibm-12712_P100-1998", 0x3F);
	SingleByteEncoderFactory<
		ASCIICompatibleByteTable<
			ByteLine<0x20AC, 0x0081, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021, 0x02C6, 0x2030, 0x008A, 0x2039, 0x008C, 0x008D, 0x008E, 0x008F>,
			ByteLine<0x0090, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, 0x02DC, 0x2122, 0x009A, 0x203A, 0x009C, 0x009D, 0x009E, 0x009F>,
			ByteLine<0x00A0, 0x00A1, 0x00A2, 0x00A3, 0x20AA, 0x00A5, 0x00A6, 0x00A7, 0x00A8, 0x00A9, 0x00D7, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF>,
			ByteLine<0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00B4, 0x00B5, 0x00B6, 0x00B7, 0x00B8, 0x00B9, 0x00F7, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x00BF>,
			ByteLine<0x05B0, 0x05B1, 0x05B2, 0x05B3, 0x05B4, 0x05B5, 0x05B6, 0x05B7, 0x05B8, 0x05B9, 0x05BA, 0x05BB, 0x05BC, 0x05BD, 0x05BE, 0x05BF>,
			ByteLine<0x05C0, 0x05C1, 0x05C2, 0x05C3, 0x05F0, 0x05F1, 0x05F2, 0x05F3, 0x05F4, 0xF88D, 0xF88E, 0xF88F, 0xF890, 0xF891, 0xF892, 0xF893>,
			ByteLine<0x05D0, 0x05D1, 0x05D2, 0x05D3, 0x05D4, 0x05D5, 0x05D6, 0x05D7, 0x05D8, 0x05D9, 0x05DA, 0x05DB, 0x05DC, 0x05DD, 0x05DE, 0x05DF>,
			ByteLine<0x05E0, 0x05E1, 0x05E2, 0x05E3, 0x05E4, 0x05E5, 0x05E6, 0x05E7, 0x05E8, 0x05E9, 0x05EA, 0xF894, 0xF895, 0x200E, 0x200F, 0xF896>
		>
	> WINDOWS_1255("windows-1255", proprietary::WINDOWS_1255, "Hebrew (windows-1255)", "\0ibm-9447|cp1255|ibm-9447_P100-2002", 0x3F);
#endif /* !ASCENSION_NO_PROPRIETARY_ENCODINGS */
#ifndef ASCENSION_NO_MINORITY_ENCODINGS
	SingleByteEncoderFactory<
		ByteTable<
			ByteLine<0x0000, 0x0001, 0x0002, 0x0003, 0x009C, 0x0009, 0x0086, 0x007F, 0x0097, 0x008D, 0x008E, 0x000B, 0x000C, 0x000D, 0x000E, 0x000F>,
			ByteLine<0x0010, 0x0011, 0x0012, 0x0013, 0x009D, 0x0085, 0x0008, 0x0087, 0x0018, 0x0019, 0x0092, 0x008F, 0x001C, 0x001D, 0x001E, 0x001F>,
			ByteLine<0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x000A, 0x0017, 0x001B, 0x0088, 0x0089, 0x008A, 0x008B, 0x008C, 0x0005, 0x0006, 0x0007>,
			ByteLine<0x0090, 0x0091, 0x0016, 0x0093, 0x0094, 0x0095, 0x0096, 0x0004, 0x0098, 0x0099, 0x009A, 0x009B, 0x0014, 0x0015, 0x009E, 0x001A>,
			ByteLine<0x0020, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0x0024, 0x002E, 0x003C, 0x0028, 0x002B, 0x007C>,
			ByteLine<0x05D0, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0x0021, 0x00A2, 0x002A, 0x0029, 0x003B, 0x00AC>,
			ByteLine<0x002D, 0x002F, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0x002C, 0x0025, 0x005F, 0x003E, 0x003F>,
			ByteLine<0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0x003A, 0x0023, 0x0040, 0x0027, 0x003D, 0x0022>,
			ByteLine<0xFFFD, 0x05D1, 0x05D2, 0x05D3, 0x05D4, 0x05D5, 0x05D6, 0x05D7, 0x05D8, 0x05D9, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD>,
			ByteLine<0xFFFD, 0x05DA, 0x05DB, 0x05DC, 0x05DD, 0x05DE, 0x05DF, 0x05E0, 0x05E1, 0x05E2, 0xFFFD, 0xFFFD, 0x20AC, 0xFFFD, 0x20AA, 0xFFFD>,
			ByteLine<0xFFFD, 0xFFFD, 0x05E3, 0x05E4, 0x05E5, 0x05E6, 0x05E7, 0x05E8, 0x05E9, 0x05EA, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD>,
			ByteLine<0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD>,
			ByteLine<0xFFFD, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD>,
			ByteLine<0xFFFD, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F, 0x0050, 0x0051, 0x0052, 0xFFFD, 0x202D, 0x202E, 0x202C, 0xFFFD, 0xFFFD>,
			ByteLine<0xFFFD, 0xFFFD, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005A, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD>,
			ByteLine<0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0xFFFD, 0x202A, 0x202B, 0x200E, 0x200F, 0x009F>
		>
	> IBM4899("IBM4899", MIB_OTHER, "Hebrew (IBM4899)", "\0ibm-4899|ibm-4899_P100-1998", 0x3F);
	SingleByteEncoderFactory<
		IBMPCCompatibleByteTable<
			ByteLine<0x05D0, 0x05D1, 0x05D2, 0x05D3, 0x05D4, 0x05D5, 0x05D6, 0x05D7, 0x05D8, 0x05D9, 0x05DA, 0x05DB, 0x05DC, 0x05DD, 0x05DE, 0x05DF>,
			ByteLine<0x05E0, 0x05E1, 0x05E2, 0x05E3, 0x05E4, 0x05E5, 0x05E6, 0x05E7, 0x05E8, 0x05E9, 0x05EA, 0xFFFD, 0x00A3, 0xFFFD, 0x00D7, 0xFFFD>,
			ByteLine<0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0x00AE, 0x00AC, 0x00BD, 0x00BC, 0xFFFD, 0x00AB, 0x00BB>,
			ByteLine<0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0x00A9, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0x00A2, 0x00A5, 0xFFFD>,
			ByteLine<0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0x00A4>,
			ByteLine<0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0x00A6, 0xFFFD, 0xFFFD>,
			ByteLine<0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0x00B5, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0x203E, 0x00B4>,
			ByteLine<0x00AD, 0x00B1, 0x2017, 0x00BE, 0x00B6, 0x00A7, 0x00F7, 0x00B8, 0x00B0, 0x00A8, 0x2022, 0x00B9, 0x00B3, 0x00B2, 0xFFFD, 0x00A0>
		>
	> IBM4952("IBM4952", MIB_OTHER, "Hebrew (IBM4952)", "\0ibm-4952|ibm-4952_P100-1995", 0x7F);
	SingleByteEncoderFactory<
		IBMPCCompatibleByteTable<
			ByteLine<0x05D0, 0x05D1, 0x05D2, 0x05D3, 0x05D4, 0x05D5, 0x05D6, 0x05D7, 0x05D8, 0x05D9, 0x05DA, 0x05DB, 0x05DC, 0x05DD, 0x05DE, 0x05DF>,
			ByteLine<0x05E0, 0x05E1, 0x05E2, 0x05E3, 0x05E4, 0x05E5, 0x05E6, 0x05E7, 0x05E8, 0x05E9, 0x05EA, 0xFFFD, 0x00A3, 0xFFFD, 0x00D7, 0x20AA>,
			ByteLine<0x200E, 0x200F, 0x202A, 0x202B, 0x202D, 0x202E, 0x202C, 0xFFFD, 0xFFFD, 0x00AE, 0x00AC, 0x00BD, 0x00BC, 0x20AC, 0x00AB, 0x00BB>,
			ByteLine<0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0xFFFD, 0xFFFD, 0xFFFD, 0x00A9, 0x2563, 0x2551, 0x2557, 0x255D, 0x00A2, 0x00A5, 0x2510>,
			ByteLine<0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0xFFFD, 0xFFFD, 0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x00A4>,
			ByteLine<0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0x2518, 0x250C, 0x2588, 0x2584, 0x00A6, 0xFFFD, 0x2580>,
			ByteLine<0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0x00B5, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0x203E, 0x00B4>,
			ByteLine<0x00AD, 0x00B1, 0x2017, 0x00BE, 0x00B6, 0x00A7, 0x00F7, 0x00B8, 0x00B0, 0x00A8, 0x2022, 0x00B9, 0x00B3, 0x00B2, 0x25A0, 0x00A0>
		>
	> IBM9048("IBM9048", MIB_OTHER, "Hebrew (IBM9048)", "\0ibm-9048|ibm-9048_P100-1998", 0x7F);
#endif /* !ASCENSION_NO_MINORITY_ENCODINGS */

	struct Installer {
		Installer() {
#ifndef ASCENSION_NO_STANDARD_ENCODINGS
			Encoder::registerFactory(ISO_8859_8);
#endif /* !ASCENSION_NO_STANDARD_ENCODINGS */
#ifndef ASCENSION_NO_PROPRIETARY_ENCODINGS
			Encoder::registerFactory(IBM867);
			Encoder::registerFactory(IBM12712);
			Encoder::registerFactory(WINDOWS_1255);
#endif /* !ASCENSION_NO_PROPRIETARY_ENCODINGS */
#ifndef ASCENSION_NO_MINORITY_ENCODINGS
			Encoder::registerFactory(IBM4899);
			Encoder::registerFactory(IBM4952);
			Encoder::registerFactory(IBM9048);
#endif /* !ASCENSION_NO_MINORITY_ENCODINGS */
		}
	} installer;
}
