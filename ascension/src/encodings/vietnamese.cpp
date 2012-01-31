/**
 * @file vietnamese.cpp
 * Implements Vietnamese encodings. This include:
 * - VISCII
 * - VIQR
 * - TCVN
 * - VPS
 * - IBM1163
 * - IBM1164
 * - IBM1165
 * - windows-1258
 * @author exeal
 * @date 2004-2011
 */

#include <ascension/corelib/encoder.hpp>
#include <cassert>
#include <cstring>	// std.memcpy
using namespace ascension;
using namespace ascension::encoding;
using namespace ascension::encoding::implementation;
using namespace std;

namespace {
#ifndef ASCENSION_NO_STANDARD_ENCODINGS
	typedef CharWire<
		CharLine<0x0000, 0x0001, 0x1eb2, 0x0003, 0x0004, 0x1eb4, 0x1eaa, 0x0007, 0x0008, 0x0009, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f>,
		CharLine<0x0010, 0x0011, 0x0012, 0x0013, 0x1ef6, 0x0015, 0x0016, 0x0017, 0x0018, 0x1ef8, 0x001a, 0x001b, 0x001c, 0x001d, 0x1ef4, 0x001f>,
		CharLine<0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f>,
		CharLine<0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f>,
		CharLine<0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f>,
		CharLine<0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f>,
		CharLine<0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f>,
		CharLine<0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x007f>,
		CharLine<0x1ea0, 0x1eae, 0x1eb0, 0x1eb6, 0x1ea4, 0x1ea6, 0x1ea8, 0x1eac, 0x1ebc, 0x1eb8, 0x1ebe, 0x1ec0, 0x1ec2, 0x1ec4, 0x1ec6, 0x1ed0>,
		CharLine<0x1ed2, 0x1ed4, 0x1ed6, 0x1ed8, 0x1ee2, 0x1eda, 0x1edc, 0x1ede, 0x1eca, 0x1ece, 0x1ecc, 0x1ec8, 0x1ee6, 0x0168, 0x1ee4, 0x1ef2>,
		CharLine<0x00d5, 0x1eaf, 0x1eb1, 0x1eb7, 0x1ea5, 0x1ea7, 0x1ea9, 0x1ead, 0x1ebd, 0x1eb9, 0x1ebf, 0x1ec1, 0x1ec3, 0x1ec5, 0x1ec7, 0x1ed1>,
		CharLine<0x1ed3, 0x1ed5, 0x1ed7, 0x1ee0, 0x01a0, 0x1ed9, 0x1edd, 0x1edf, 0x1ecb, 0x1ef0, 0x1ee8, 0x1eea, 0x1eec, 0x01a1, 0x1edb, 0x01af>,
		CharLine<0x00c0, 0x00c1, 0x00c2, 0x00c3, 0x1ea2, 0x0102, 0x1eb3, 0x1eb5, 0x00c8, 0x00c9, 0x00ca, 0x1eba, 0x00cc, 0x00cd, 0x0128, 0x1ef3>,
		CharLine<0x0110, 0x1ee9, 0x00d2, 0x00d3, 0x00d4, 0x1ea1, 0x1ef7, 0x1eeb, 0x1eed, 0x00d9, 0x00da, 0x1ef9, 0x1ef5, 0x00dd, 0x1ee1, 0x01b0>,
		CharLine<0x00e0, 0x00e1, 0x00e2, 0x00e3, 0x1ea3, 0x0103, 0x1eef, 0x1eab, 0x00e8, 0x00e9, 0x00ea, 0x1ebb, 0x00ec, 0x00ed, 0x0129, 0x1ec9>,
		CharLine<0x0111, 0x1ef1, 0x00f2, 0x00f3, 0x00f4, 0x00f5, 0x1ecf, 0x1ecd, 0x1ee5, 0x00f9, 0x00fa, 0x0169, 0x1ee7, 0x00fd, 0x1ee3, 0x1eee>
	> VISCII_BYTE_TABLE;
	sbcs::SingleByteEncoderFactory<VISCII_BYTE_TABLE> VISCII("VISCII", standard::VISCII, "Vietnamese (VISCII)", "csVISCII", 0x1a);
	sbcs::SingleByteEncoderFactory<
		CharWire<
			CharLine<0x0000, 0x00da, 0x1ee4, 0x0003, 0x1eea, 0x1eec, 0x1eee, 0x0007, 0x0008, 0x0009, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f>,
			CharLine<0x0010, 0x1ee8, 0x1ef0, 0x1ef2, 0x1ef6, 0x1ef8, 0x00dd, 0x1ef4, 0x0018, 0x0019, 0x001a, 0x001b, 0x001c, 0x001d, 0x001e, 0x001f>,
			CharLine<0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f>,
			CharLine<0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f>,
			CharLine<0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f>,
			CharLine<0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f>,
			CharLine<0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f>,
			CharLine<0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x007f>,
			CharLine<0x00c0, 0x1ea2, 0x00c3, 0x00c1, 0x1ea0, 0x1eb6, 0x1eac, 0x00c8, 0x1eba, 0x1ebc, 0x00c9, 0x1eb8, 0x1ec6, 0x00cc, 0x1ec8, 0x0128>,
			CharLine<0x00cd, 0x1eca, 0x00d2, 0x1ece, 0x00d5, 0x00d3, 0x1ecc, 0x1ed8, 0x1edc, 0x1ede, 0x1ee0, 0x1eda, 0x1ee2, 0x00d9, 0x1ee6, 0x0168>,
			CharLine<0x00a0, 0x0102, 0x00c2, 0x00ca, 0x00d4, 0x01a0, 0x01af, 0x0110, 0x0103, 0x00e2, 0x00ea, 0x00f4, 0x01a1, 0x01b0, 0x0111, 0x1eb0>,
			CharLine<0x0300, 0x0309, 0x0303, 0x0301, 0x0323, 0x00e0, 0x1ea3, 0x00e3, 0x00e1, 0x1ea1, 0x1eb2, 0x1eb1, 0x1eb3, 0x1eb5, 0x1eaf, 0x1eb4>,
			CharLine<0x1eae, 0x1ea6, 0x1ea8, 0x1eaa, 0x1ea4, 0x1ec0, 0x1eb7, 0x1ea7, 0x1ea9, 0x1eab, 0x1ea5, 0x1ead, 0x00e8, 0x1ec2, 0x1ebb, 0x1ebd>,
			CharLine<0x00e9, 0x1eb9, 0x1ec1, 0x1ec3, 0x1ec5, 0x1ebf, 0x1ec7, 0x00ec, 0x1ec9, 0x1ec4, 0x1ebe, 0x1ed2, 0x0129, 0x00ed, 0x1ecb, 0x00f2>,
			CharLine<0x1ed4, 0x1ecf, 0x00f5, 0x00f3, 0x1ecd, 0x1ed3, 0x1ed5, 0x1ed7, 0x1ed1, 0x1ed9, 0x1edd, 0x1edf, 0x1ee1, 0x1edb, 0x1ee3, 0x00f9>,
			CharLine<0x1ed6, 0x1ee7, 0x0169, 0x00fa, 0x1ee5, 0x1eeb, 0x1eed, 0x1eef, 0x1ee9, 0x1ef1, 0x1ef3, 0x1ef7, 0x1ef9, 0x00fd, 0x1ef5, 0x1ed0>
		>
	> TCVN("TCVN", MIB_OTHER, "Vietnamese (TCVN)", "", 0x1a);
#endif // !ASCENSION_NO_STANDARD_ENCODINGS
#ifndef ASCENSION_NO_PROPRIETARY_ENCODINGS
	sbcs::SingleByteEncoderFactory<
		CharWire<
			CharLine<0x0000, 0x0001, 0x0002, 0x0003, 0x009c, 0x0009, 0x0086, 0x007f, 0x0097, 0x008d, 0x008e, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f>,
			CharLine<0x0010, 0x0011, 0x0012, 0x0013, 0x009d, 0x0085, 0x0008, 0x0087, 0x0018, 0x0019, 0x0092, 0x008f, 0x001c, 0x001d, 0x001e, 0x001f>,
			CharLine<0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x000a, 0x0017, 0x001b, 0x0088, 0x0089, 0x008a, 0x008b, 0x008c, 0x0005, 0x0006, 0x0007>,
			CharLine<0x0090, 0x0091, 0x0016, 0x0093, 0x0094, 0x0095, 0x0096, 0x0004, 0x0098, 0x0099, 0x009a, 0x009b, 0x0014, 0x0015, 0x009e, 0x001a>,
			CharLine<0x0020, 0x00a0, 0x00e2, 0x00e4, 0x00e0, 0x00e1, 0x0103, 0x00e5, 0x00e7, 0x00f1, 0x005b, 0x002e, 0x003c, 0x0028, 0x002b, 0x0021>,
			CharLine<0x0026, 0x00e9, 0x00ea, 0x00eb, 0x00e8, 0x00ed, 0x00ee, 0x00ef, 0x0303, 0x00df, 0x005d, 0x0024, 0x002a, 0x0029, 0x003b, 0x005e>,
			CharLine<0x002d, 0x002f, 0x00c2, 0x00c4, 0x00c0, 0x00c1, 0x0102, 0x00c5, 0x00c7, 0x00d1, 0x00a6, 0x002c, 0x0025, 0x005f, 0x003e, 0x003f>,
			CharLine<0x00f8, 0x00c9, 0x00ca, 0x00cb, 0x00c8, 0x00cd, 0x00ce, 0x00cf, 0x20ab, 0x0060, 0x003a, 0x0023, 0x0040, 0x0027, 0x003d, 0x0022>,
			CharLine<0x00d8, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x00ab, 0x00bb, 0x0111, 0x0309, 0x0300, 0x00b1>,
			CharLine<0x00b0, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f, 0x0070, 0x0071, 0x0072, 0x00aa, 0x00ba, 0x00e6, 0x0152, 0x00c6, 0x20ac>,
			CharLine<0x00b5, 0x007e, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007a, 0x00a1, 0x00bf, 0x0110, 0x0323, 0x0301, 0x00ae>,
			CharLine<0x00a2, 0x00a3, 0x00a5, 0x00b7, 0x00a9, 0x00a7, 0x00b6, 0x00bc, 0x00bd, 0x00be, 0x00ac, 0x007c, 0x00af, 0x0153, 0x0178, 0x00d7>,
			CharLine<0x007b, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x00ad, 0x00f4, 0x00f6, 0x01b0, 0x00f3, 0x01a1>,
			CharLine<0x007d, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f, 0x0050, 0x0051, 0x0052, 0x00b9, 0x00fb, 0x00fc, 0x00f9, 0x00fa, 0x00ff>,
			CharLine<0x005c, 0x00f7, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005a, 0x00b2, 0x00d4, 0x00d6, 0x01af, 0x00d3, 0x01a0>,
			CharLine<0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x00b3, 0x00db, 0x00dc, 0x00d9, 0x00da, 0x009f>
		>
	> IBM1164("IBM1164", MIB_OTHER, "Vietnamese (EBCDIC Viet Nam (IBM1130 + Euro))", "\0ibm-1164_P100-1999", 0x3f);
	sbcs::SingleByteEncoderFactory<
		CharWire<
			CharLine<0x0000, 0x0001, 0x0002, 0x0003, 0x009c, 0x0009, 0x0086, 0x007f, 0x0097, 0x008d, 0x008e, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f>,
			CharLine<0x0010, 0x0011, 0x0012, 0x0013, 0x009d, 0x0085, 0x0008, 0x0087, 0x0018, 0x0019, 0x0092, 0x008f, 0x001c, 0x001d, 0x001e, 0x001f>,
			CharLine<0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x000a, 0x0017, 0x001b, 0x0088, 0x0089, 0x008a, 0x008b, 0x008c, 0x0005, 0x0006, 0x0007>,
			CharLine<0x0090, 0x0091, 0x0016, 0x0093, 0x0094, 0x0095, 0x0096, 0x0004, 0x0098, 0x0099, 0x009a, 0x009b, 0x0014, 0x0015, 0x009e, 0x001a>,
			CharLine<0x0020, 0x00a0, 0x00e2, 0x00e4, 0x0163, 0x00e1, 0x0103, 0x010d, 0x00e7, 0x0107, 0x00dd, 0x002e, 0x003c, 0x0028, 0x002b, 0x007c>,
			CharLine<0x0026, 0x00e9, 0x0119, 0x00eb, 0x016f, 0x00ed, 0x00ee, 0x013e, 0x013a, 0x00df, 0x0021, 0x0024, 0x002a, 0x0029, 0x003b, 0x005e>,
			CharLine<0x002d, 0x002f, 0x00c2, 0x00c4, 0x02dd, 0x00c1, 0x0102, 0x010c, 0x00c7, 0x0106, 0x00a8, 0x002c, 0x0025, 0x005f, 0x003e, 0x003f>,
			CharLine<0x02c7, 0x00c9, 0x0118, 0x00cb, 0x016e, 0x00cd, 0x00ce, 0x013d, 0x0139, 0x0060, 0x003a, 0x0023, 0x0040, 0x0027, 0x003d, 0x0022>,
			CharLine<0x02d8, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x015b, 0x0148, 0x0111, 0x00fd, 0x0159, 0x015f>,
			CharLine<0x00b0, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f, 0x0070, 0x0071, 0x0072, 0x0142, 0x0144, 0x0161, 0x00b8, 0x02db, 0x20ac>,
			CharLine<0x0105, 0x007e, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007a, 0x015a, 0x0147, 0x0110, 0x005b, 0x0158, 0x015e>,
			CharLine<0x02d9, 0x0104, 0x017c, 0x0162, 0x017b, 0x00a7, 0x017e, 0x017a, 0x017d, 0x0179, 0x0141, 0x0143, 0x0160, 0x005d, 0x00b4, 0x00d7>,
			CharLine<0x007b, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x00ad, 0x00f4, 0x00f6, 0x0155, 0x00f3, 0x0151>,
			CharLine<0x007d, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f, 0x0050, 0x0051, 0x0052, 0x011a, 0x0171, 0x00fc, 0x0165, 0x00fa, 0x011b>,
			CharLine<0x005c, 0x00f7, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005a, 0x010f, 0x00d4, 0x00d6, 0x0154, 0x00d3, 0x0150>,
			CharLine<0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x010e, 0x0170, 0x00dc, 0x0164, 0x00da, 0x009f>
		>
	> IBM1165("IBM1165", MIB_OTHER, "Vietnamese (EBCDIC)", "\0ibm-1165_P101-2000", 0x3f);
	sbcs::SingleByteEncoderFactory<
		sbcs::ASCIICompatibleCharWire<
			CharLine<0x20ac, 0x0081, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021, 0x02c6, 0x2030, 0x008a, 0x2039, 0x0152, 0x008d, 0x008e, 0x008f>,
			CharLine<0x0090, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014, 0x02dc, 0x2122, 0x009a, 0x203a, 0x0153, 0x009d, 0x009e, 0x0178>,
			CharLine<0x00a0, 0x00a1, 0x00a2, 0x00a3, 0x00a4, 0x00a5, 0x00a6, 0x00a7, 0x00a8, 0x00a9, 0x00aa, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x00af>,
			CharLine<0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x00b4, 0x00b5, 0x00b6, 0x00b7, 0x00b8, 0x00b9, 0x00ba, 0x00bb, 0x00bc, 0x00bd, 0x00be, 0x00bf>,
			CharLine<0x00c0, 0x00c1, 0x00c2, 0x0102, 0x00c4, 0x00c5, 0x00c6, 0x00c7, 0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x0300, 0x00cd, 0x00ce, 0x00cf>,
			CharLine<0x0110, 0x00d1, 0x0309, 0x00d3, 0x00d4, 0x01a0, 0x00d6, 0x00d7, 0x00d8, 0x00d9, 0x00da, 0x00db, 0x00dc, 0x01af, 0x0303, 0x00df>,
			CharLine<0x00e0, 0x00e1, 0x00e2, 0x0103, 0x00e4, 0x00e5, 0x00e6, 0x00e7, 0x00e8, 0x00e9, 0x00ea, 0x00eb, 0x0301, 0x00ed, 0x00ee, 0x00ef>,
			CharLine<0x0111, 0x00f1, 0x0323, 0x00f3, 0x00f4, 0x01a1, 0x00f6, 0x00f7, 0x00f8, 0x00f9, 0x00fa, 0x00fb, 0x00fc, 0x01b0, 0x20ab, 0x00ff>
		>
	> WINDOWS_1258("windows-1258", proprietary::WINDOWS_1258, "Vietnamese (Windows)", "\0ibm-5354|cp1258|ibm-5354_P100-1998", 0x3f);
#endif // !ASCENSION_NO_PROPRIETARY_ENCODINGS
#ifndef ASCENSION_NO_MINORITY_ENCODINGS
	sbcs::SingleByteEncoderFactory<
		CharWire<
			CharLine<0x0000, 0x0001, 0x1ea0, 0x1eac, 0x1eb6, 0x1eb8, 0x1ec6, 0x0007, 0x0008, 0x0009, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f>,
			CharLine<0x1eca, 0x1ecc, 0x1ed8, 0x1ee2, 0x1ee4, 0x1ef0, 0x0016, 0x0017, 0x0018, 0x1ef4, 0x001a, 0x001b, 0x1eaa, 0x1eee, 0x001e, 0x001f>,
			CharLine<0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f>,
			CharLine<0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f>,
			CharLine<0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f>,
			CharLine<0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f>,
			CharLine<0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f>,
			CharLine<0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x007f>,
			CharLine<0x00c0, 0x1ea2, 0x00c3, 0x1ea4, 0x1ea6, 0x1ea8, 0x1ecd, 0x1ed7, 0x0102, 0x1ebf, 0x1ec1, 0x1ec3, 0x1ec7, 0x1eae, 0x1eb0, 0x1eb2>,
			CharLine<0x1ebe, 0x2018, 0x2019, 0x1ec0, 0x1ec2, 0x1ec4, 0x1ed0, 0x1ed2, 0x1ed4, 0x1ed6, 0x00fd, 0x1ef7, 0x1ef5, 0x1eda, 0x1edc, 0x1ede>,
			CharLine<0x00a0, 0x1eaf, 0x1eb1, 0x1eb3, 0x1eb5, 0x1eb7, 0x1ee0, 0x1edb, 0x00d9, 0x1edd, 0x1edf, 0x1ee1, 0x0168, 0x1ee8, 0x1ee3, 0x1eea>,
			CharLine<0x1ed5, 0x1eec, 0x1ef2, 0x1ef8, 0x00cd, 0x00cc, 0x1ed9, 0x1ec8, 0x0128, 0x00d3, 0x1eed, 0x1eef, 0x00d2, 0x1ece, 0x00d5, 0x1ef1>,
			CharLine<0x1ea7, 0x00c1, 0x00c2, 0x1ea5, 0x1ea9, 0x1eab, 0x1ead, 0x0111, 0x1ebb, 0x00c9, 0x00ca, 0x1eb9, 0x1ec9, 0x1ec5, 0x1ecb, 0x1ef9>,
			CharLine<0x01af, 0x1ee6, 0x1ed3, 0x1ed1, 0x00d4, 0x1ecf, 0x01a1, 0x00c8, 0x1eeb, 0x1ee9, 0x00da, 0x0169, 0x01b0, 0x00dd, 0x1eba, 0x00df>,
			CharLine<0x00e0, 0x00e1, 0x00e2, 0x00e3, 0x1ea3, 0x1ea1, 0x0103, 0x00e7, 0x00e8, 0x00e9, 0x00ea, 0x1ebd, 0x00ec, 0x00ed, 0x00ee, 0x0129>,
			CharLine<0x1eb4, 0x0110, 0x00f2, 0x00f3, 0x00f4, 0x00f5, 0x00f6, 0x01a0, 0x1ee5, 0x00f9, 0x00fa, 0x1ee7, 0x00fc, 0x1ef6, 0x1ebc, 0x1ef3>
		>
	> VPS("VPS", MIB_OTHER, "Vietnamese (VPS)", "", 0x1a);
	sbcs::SingleByteEncoderFactory<
		sbcs::ISO8859CompatibleCharWire<
			CharLine<0x00a0, 0x00a1, 0x00a2, 0x00a3, 0x20ac, 0x00a5, 0x00a6, 0x00a7, 0x0153, 0x00a9, 0x00aa, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x00af>,
			CharLine<0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x0178, 0x00b5, 0x00b6, 0x00b7, 0x0152, 0x00b9, 0x00ba, 0x00bb, 0x00bc, 0x00bd, 0x00be, 0x00bf>,
			CharLine<0x00c0, 0x00c1, 0x00c2, 0x0102, 0x00c4, 0x00c5, 0x00c6, 0x00c7, 0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x0300, 0x00cd, 0x00ce, 0x00cf>,
			CharLine<0x0110, 0x00d1, 0x0309, 0x00d3, 0x00d4, 0x01a0, 0x00d6, 0x00d7, 0x00d8, 0x00d9, 0x00da, 0x00db, 0x00dc, 0x01af, 0x0303, 0x00df>,
			CharLine<0x00e0, 0x00e1, 0x00e2, 0x0103, 0x00e4, 0x00e5, 0x00e6, 0x00e7, 0x00e8, 0x00e9, 0x00ea, 0x00eb, 0x0301, 0x00ed, 0x00ee, 0x00ef>,
			CharLine<0x0111, 0x00f1, 0x0323, 0x00f3, 0x00f4, 0x01a1, 0x00f6, 0x00f7, 0x00f8, 0x00f9, 0x00fa, 0x00fb, 0x00fc, 0x01b0, 0x20ab, 0x00ff>
		>
	> IBM1163("IBM1163", MIB_OTHER, "Vietnamese (IBM1163)", "\0ibm-1163_P100-1999", 0x1a);
#endif // !ASCENSION_NO_MINORITY_ENCODINGS

#ifndef ASCENSION_NO_STANDARD_ENCODINGS
	class VIQREncoder : public Encoder {
	public:
		VIQREncoder() /*throw()*/;
	private:
		Result doFromUnicode(Byte* to, Byte* toEnd, Byte*& toNext, const Char* from, const Char* fromEnd, const Char*& fromNext);
		Result doToUnicode(Char* to, Char* toEnd, Char*& toNext, const Byte* from, const Byte* fromEnd, const Byte*& fromNext);
		const EncodingProperties& properties() const /*throw()*/;
		Encoder& resetDecodingState() /*throw()*/;
		Encoder& resetEncodingState() /*throw()*/;
	private:
		enum {LITERAL_STATE, ENGLISH_STATE, VIETNAMESE_STATE} encodingState_, decodingState_;
		static const Byte CLS = 0x01, COM = 0x5c;
		static unique_ptr<sbcs::BidirectionalMap> table_;
	};
	class VIQRFactory : public implementation::EncoderFactoryBase {
	public:
		VIQRFactory() /*throw()*/ : implementation::EncoderFactoryBase("VIQR", standard::VIQR, "Vietnamese (VIQR)", 3, 1, "csVIQR", 0x1a) {}
	private:
		unique_ptr<Encoder> create() const /*throw()*/ {return unique_ptr<Encoder>(new VIQREncoder);}
	} VIQR;
#endif // !ASCENSION_NO_STANDARD_ENCODINGS

	struct Installer {
		Installer() {
#ifndef ASCENSION_NO_STANDARD_ENCODINGS
			Encoder::registerFactory(VISCII);
			Encoder::registerFactory(VIQR);
			Encoder::registerFactory(TCVN);
#endif // !ASCENSION_NO_STANDARD_ENCODINGS
#ifndef ASCENSION_NO_PROPRIETARY_ENCODINGS
			Encoder::registerFactory(IBM1164);
			Encoder::registerFactory(IBM1165);
			Encoder::registerFactory(WINDOWS_1258);
#endif // !ASCENSION_NO_PROPRIETARY_ENCODINGS
#ifndef ASCENSION_NO_MINORITY_ENCODINGS
			Encoder::registerFactory(VPS);
			Encoder::registerFactory(IBM1163);
#endif // !ASCENSION_NO_MINORITY_ENCODINGS
		}
	} installer;
} // namespace @0

#ifndef ASCENSION_NO_STANDARD_ENCODINGS

unique_ptr<sbcs::BidirectionalMap> VIQREncoder::table_;

VIQREncoder::VIQREncoder() /*throw()*/ : encodingState_(VIETNAMESE_STATE), decodingState_(VIETNAMESE_STATE) {
	if(table_.get() == nullptr)
		table_.reset(new sbcs::BidirectionalMap(VISCII_BYTE_TABLE::VALUES));
}

Encoder::Result VIQREncoder::doFromUnicode(Byte* to, Byte* toEnd,
		Byte*& toNext, const Char* from, const Char* fromEnd, const Char*& fromNext) {
	static const Byte VISCII_TO_VIQR[] =
		"\0\001A(?\003\004A(~A^~\007"		"\010\011\012\013\014\015\016\017"
		"\020\021\022\023Y?\024\025\026"	"\030Y~\031\032\033\034Y.\037"
		" !\"#$%&'"							"()*+,-./"
		"01234567"							"89:;<=>?"
		"@ABCDEFG"							"HIJKLMNO"
		"PQRSTUVW"							"XYZ[\\]^_"
		"`abcdefg"							"hijklmno"
		"pqrstuvw"							"xyz{|}~\x7f"
		"A.A('A(`A(.A^'A^`A^?A^."			"E~E.E^'E^`E^?E^~E^.O^'"
		"O^`O^?O^~O^.O+.O+'O+`O+?"			"I.O?O.I?U?U~U.Y`"
		"O~a('a(`a(.a^'a^`a^?a^."			"e~e.e^'e^`e^?e^~e^.o^'"
		"o^`o^?o^~O+~O+o^.o+`o+?"			"i.U+.U+'U+`U+?o+o+'U+"
		"A`A'A^A~A?A(a(?a(~"				"E`E'E^E?I`I'I~y`"
		"DDu+'O`O'O^a.y?u+`"				"u+?U`U'y~y.Y'o+~u+"
		"a`a'a^a~a?a(u+~a^~"				"e`e'e^e?i`i'i~i?"
		"ddu+.o`o'o^o~o?o."					"u.u`u'u~u?y'o+.U+~";
	static const ptrdiff_t VISCII_TO_VIQR_INDICES[16 * 16 + 1] = {
		0, 1, 2, 5, 6, 7, 10, 13,				14, 15, 16, 17, 18, 19, 20, 21,
		22, 23, 24, 25, 27, 28, 29, 30,			31, 33, 34, 35, 36, 37, 38, 40,
		41, 42, 43, 44, 45, 46, 47, 48,			49, 50, 51, 52, 53, 54, 55, 56,
		57, 58, 59, 60, 61, 62, 63, 64,			65, 66, 67, 68, 69, 70, 71, 72,
		73, 74, 75, 76, 77, 78,	79, 80,			81, 82, 83, 84, 85, 86, 87, 88,
		89, 90, 91, 92, 93, 94, 95, 96,			97, 98, 99, 100, 101, 102, 103, 104,
		105, 106, 107, 108, 109, 110, 111, 112,	113, 114, 115, 116, 117, 118, 119, 120,
		121, 122, 123, 124, 125, 126, 127, 128,	129, 130, 131, 132, 133, 134, 135, 136,
		137, 139, 142, 145, 148, 151, 154, 157,	160, 162, 164, 167, 170, 173, 176, 179,
		182, 185, 188, 191, 194, 197, 200, 203,	206, 208, 210, 212, 214, 216, 218, 220,
		222, 224, 227, 230, 233, 236, 239, 242, 245, 247, 249, 252, 255, 258, 261, 265,
		267, 270, 273, 276, 279, 281, 284, 287,	290, 292, 295, 298, 301, 304, 306, 309,
		311, 313, 315, 317, 319, 321, 323, 326,	329, 331, 333, 335, 337, 339, 341, 343,
		345, 347, 350, 352, 354, 356, 358, 360,	363, 366, 368, 370, 372, 374, 376, 379,
		381, 383, 385, 387, 389, 391, 393, 396,	399, 401, 403, 405, 407, 409, 411, 413,
		415, 417, 420, 422, 424, 426, 428, 430,	432, 434, 436, 438, 440, 442, 444, 447, 450
	};

	if(encodingState_ != VIETNAMESE_STATE) {
		// switch to Vietnamese state
		if(to > toEnd - 2) {
			toNext = to;
			fromNext = from;
			return INSUFFICIENT_BUFFER;
		}
		*to++ = COM;
		*to++ = 'V';
		encodingState_ = VIETNAMESE_STATE;
	}
	for(; to < toEnd && from < fromEnd; ++from) {
		Byte viscii = table_->toByte(*from);
		if(viscii == sbcs::UNMAPPABLE_BYTE && *from != sbcs::UNMAPPABLE_BYTE) {
			if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTERS)
				continue;
			else if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTERS)
				viscii = properties().substitutionCharacter();
			else {
				toNext = to;
				fromNext = from;
				return UNMAPPABLE_CHARACTER;
			}
		}
		const ptrdiff_t length = VISCII_TO_VIQR_INDICES[viscii + 1] - VISCII_TO_VIQR_INDICES[viscii];
		if(length > toEnd - to)
			break;
		memcpy(to, VISCII_TO_VIQR + VISCII_TO_VIQR_INDICES[viscii], length);
		to += length;
	}
	toNext = to;
	fromNext = from;
	return (fromNext == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
}

Encoder::Result VIQREncoder::doToUnicode(Char* to, Char* toEnd,
		Char*& toNext, const Byte* from, const Byte* fromEnd, const Byte*& fromNext) {
	enum {NONE, BREVE, CIRCUMFLEX, HORN, ACUTE, GRAVE, HOOK_ABOVE, TILDE, DOT_BELOW, CAPITAL_D, SMALL_D, DIACRITICALS_COUNT};
	static const Byte MNEMONIC_TABLE[0x80] = {
		NONE,	NONE,	NONE,	NONE,	NONE,		NONE,	NONE,		NONE,	// 0x00
		NONE,	NONE,	NONE,	NONE,	NONE,		NONE,	NONE,		NONE,
		NONE,	NONE,	NONE,	NONE,	NONE,		NONE,	NONE,		NONE,	// 0x10
		NONE,	NONE,	NONE,	NONE,	NONE,		NONE,	NONE,		NONE,
		NONE,	NONE,	NONE,	NONE,	NONE,		NONE,	NONE,		ACUTE,	// 0x20
		BREVE,	NONE,	NONE,	HORN,	NONE,		NONE,	DOT_BELOW,	NONE,
		NONE,	NONE,	NONE,	NONE,	NONE,		NONE,	NONE,		NONE,	// 0x30
		NONE,	NONE,	NONE,	NONE,	NONE,		NONE,	NONE,		HOOK_ABOVE,
		NONE,	NONE,	NONE,	NONE,	CAPITAL_D,	NONE,	NONE,		NONE,	// 0x40
		NONE,	NONE,	NONE,	NONE,	NONE,		NONE,	NONE,		NONE,
		NONE,	NONE,	NONE,	NONE,	NONE,		NONE,	NONE,		NONE,	// 0x50
		NONE,	NONE,	NONE,	NONE,	NONE,		NONE,	CIRCUMFLEX,	NONE,
		GRAVE,	NONE,	NONE,	NONE,	SMALL_D,	NONE,	NONE,		NONE,	// 0x60
		NONE,	NONE,	NONE,	NONE,	NONE,		NONE,	NONE,		NONE,
		NONE,	NONE,	NONE,	NONE,	NONE,		NONE,	NONE,		NONE,	// 0x70
		NONE,	NONE,	NONE,	NONE,	NONE,		NONE,	TILDE,		NONE
	};
	static const Byte BASE_CHARACTER_TABLE[0x80] = {
		0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,	// 0x00
		0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
		0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
		0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
		0x80, 0x00, 0x80, 0x80, 0x03, 0x04, 0x80, 0x80, 0x80, 0x06, 0x80, 0x80, 0x80, 0x80, 0x80, 0x07,	// 0x40
		0x80, 0x80, 0x80, 0x80, 0x80, 0x0a, 0x80, 0x80, 0x80, 0x0c, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,	// 0x50
		0x80, 0x0d, 0x80, 0x80, 0x10, 0x11, 0x80, 0x80, 0x80, 0x13, 0x80, 0x80, 0x80, 0x80, 0x80, 0x14,	// 0x60
		0x80, 0x80, 0x80, 0x80, 0x80, 0x17, 0x80, 0x80, 0x80, 0x19, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80	// 0x70
	};
	static const Char STATE_TABLE[][DIACRITICALS_COUNT] = {
	//	 <>      (       ^       +       '       `       ?       ~       .       D       d
		{0x0041,   0x01,   0x02, 0x0041, 0x00c1, 0x00c0, 0x1ea2, 0x00c3, 0x1ea0, 0x0041, 0x0041},	// 0x00 : A
		{0x0102, 0x0102, 0x0102, 0x0102, 0x1eae, 0x1eb0, 0x1eb2, 0x1eb4, 0x1eb6, 0x0102, 0x0102},	// 0x01 : A(
		{0x00c2, 0x00c2, 0x00c2, 0x00c2, 0x1ea4, 0x1ea6, 0x1ea8, 0x1eaa, 0x1eac, 0x00c2, 0x00c2},	// 0x02 : A^
		{0x0044, 0x0044, 0x0044, 0x0044, 0x0044, 0x0044, 0x0044, 0x0044, 0x0044, 0x0110, 0x0110},	// 0x03 : D
		{0x0045, 0x0045,   0x05, 0x0045, 0x00c9, 0x00c8, 0x1eba, 0x1ebc, 0x1eb8, 0x0045, 0x0045},	// 0x04 : E
		{0x00ca, 0x00ca, 0x00ca, 0x00ca, 0x1ebe, 0x1ec0, 0x1ec2, 0x1ec4, 0x1ec6, 0x00ca, 0x00ca},	// 0x05 : E^
		{0x0049, 0x0049, 0x0049, 0x0049, 0x00cd, 0x00cc, 0x1ec8, 0x0128, 0x1eca, 0x0049, 0x0049},	// 0x06 : I
		{0x004f, 0x004f,   0x08,   0x09, 0x00d3, 0x00d2, 0x1ece, 0x00d5, 0x1ecc, 0x004f, 0x004f},	// 0x07 : O
		{0x00d4, 0x00d4, 0x00d4, 0x00d4, 0x1ed0, 0x1ed2, 0x1ed4, 0x1ed6, 0x1ed8, 0x00d4, 0x00d4},	// 0x08 : O^
		{0x01a0, 0x01a0, 0x01a0, 0x01a0, 0x1eda, 0x1edc, 0x1ede, 0x1ee0, 0x1ee2, 0x01a0, 0x01a0},	// 0x09 : O+
		{0x0055, 0x0055, 0x0055,   0x0b, 0x00da, 0x00d9, 0x1ee6, 0x0168, 0x1ee4, 0x0055, 0x0055},	// 0x0A : U
		{0x01af, 0x01af, 0x01af, 0x01af, 0x1ee8, 0x1eea, 0x1eec, 0x1eee, 0x1ef0, 0x01af, 0x01af},	// 0x0B : U+
		{0x0059, 0x0059, 0x0059, 0x0059, 0x00dd, 0x1ef2, 0x1ef6, 0x1ef8, 0x1ef4, 0x0059, 0x0059},	// 0x0C : Y
		{0x0061,   0x0e,   0x0f, 0x0061, 0x00e1, 0x00e0, 0x1ea3, 0x00e3, 0x1ea1, 0x0061, 0x0061},	// 0x0D : a
		{0x0103, 0x0103, 0x0103, 0x0103, 0x1eaf, 0x1eb1, 0x1eb3, 0x1eb5, 0x1eb7, 0x0103, 0x0103},	// 0x0E : a(
		{0x00e2, 0x00e2, 0x00e2, 0x00e2, 0x1ea5, 0x1ea7, 0x1ea9, 0x1eab, 0x1ead, 0x00e2, 0x00e2},	// 0x0F : a^
		{0x0064, 0x0064, 0x0064, 0x0064, 0x0064, 0x0064, 0x0064, 0x0064, 0x0064, 0x0110, 0x0111},	// 0x10 : d
		{0x0065, 0x0065,   0x12, 0x0065, 0x00e9, 0x00e8, 0x1ebb, 0x1ebd, 0x1eb9, 0x0065, 0x0065},	// 0x11 : e
		{0x00ea, 0x00ea, 0x00ea, 0x00ea, 0x1ebf, 0x1ec1, 0x1ec3, 0x1ec5, 0x1ec7, 0x00ea, 0x00ea},	// 0x12 : e^
		{0x0069, 0x0069, 0x0069, 0x0069, 0x00ed, 0x00ec, 0x1ec9, 0x0129, 0x1ecb, 0x0069, 0x0069},	// 0x13 : i
		{0x006f, 0x006f,   0x15,   0x16, 0x00f3, 0x00f2, 0x1ecf, 0x00f5, 0x1ecd, 0x006f, 0x006f},	// 0x14 : o
		{0x00f4, 0x00f4, 0x00f4, 0x00f4, 0x1ed1, 0x1ed3, 0x1ed5, 0x1ed7, 0x1ed9, 0x00f4, 0x00f4},	// 0x15 : o^
		{0x01a1, 0x01a1, 0x01a1, 0x01a1, 0x1edb, 0x1edd, 0x1edf, 0x1ee1, 0x1ee3, 0x01a1, 0x01a1},	// 0x16 : o+
		{0x0075, 0x0075, 0x0075,   0x18, 0x00fa, 0x00f9, 0x1ee7, 0x0169, 0x1ee5, 0x0075, 0x0075},	// 0x17 : u
		{0x01b0, 0x01b0, 0x01b0, 0x01b0, 0x1ee9, 0x1eeb, 0x1eed, 0x1eef, 0x1ef1, 0x01b0, 0x01b0},	// 0x18 : u+
		{0x0079, 0x0079, 0x0079, 0x0079, 0x00fd, 0x1ef3, 0x1ef7, 0x1ef9, 0x1ef5, 0x0079, 0x0079}	// 0x19 : y
	};

	bool escaped = false;
	for(; to < toEnd && from < fromEnd; ++from) {
		if((*from & 0x80) != 0) {
			toNext = to;
			fromNext = from;
			return UNMAPPABLE_CHARACTER;
		} else if(*from == COM) {
			if(escaped)
				*to++ = L'\\';
			escaped = !escaped;
			continue;
		}
		if(escaped) {
			// try to switch the state
			switch(*from) {
			case 'L': case 'l': decodingState_ = LITERAL_STATE; escaped = false; continue;
			case 'M': case 'm': decodingState_ = ENGLISH_STATE; escaped = false; continue;
			case 'V': case 'v': decodingState_ = VIETNAMESE_STATE; escaped = false; continue;
			}
		}
		if(decodingState_ == VIETNAMESE_STATE || (decodingState_ == ENGLISH_STATE && escaped)) {
			escaped = false;
			Byte mnemonic = BASE_CHARACTER_TABLE[*from];
			if(mnemonic != 0x80) {
				// ... got the base character
				if(from + 1 == fromEnd) {
					if((flags() & END_OF_BUFFER) != 0) {
						*to++ = *from++;
						break;
					}
					toNext = to;
					fromNext = from - (escaped ? 1 : 0);
					return COMPLETED;	// more input is required
				}
				const Char* const state1 = STATE_TABLE[mnemonic];
				mnemonic = MNEMONIC_TABLE[from[1]];
				const Char state2 = state1[mnemonic];
				if(state2 >= 0x20) {
					*to++ = state2;
					if(state2 != state1[NONE])
						++from;
					continue;
				}
				if(from + 2 == fromEnd) {
					if((flags() & END_OF_BUFFER) != 0) {
						*to++ = STATE_TABLE[state2][NONE];
						break;
					}
					toNext = to;
					fromNext = from - (escaped ? 1 : 0);
					return COMPLETED;	// more input is required
				}
				const Char* const state3 = STATE_TABLE[state2];
				mnemonic = MNEMONIC_TABLE[from[2]];
				const Char state4 = state3[mnemonic];
				assert(state4 >= 0x20);
				*to++ = state4;
				from += (state4 != state3[NONE]) ? 2 : 1;
				continue;
			}
		}
		*to++ = *from;
	}
	toNext = to;
	fromNext = from;
	return (fromNext == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
}

const EncodingProperties& VIQREncoder::properties() const /*throw()*/ {
	return VIQR;
}

Encoder& VIQREncoder::resetDecodingState() /*throw()*/ {
	decodingState_ = VIETNAMESE_STATE;
	return *this;
}

Encoder& VIQREncoder::resetEncodingState() /*throw()*/ {
	encodingState_ = VIETNAMESE_STATE;
	return *this;
}

#endif // !ASCENSION_NO_STANDARD_ENCODINGS
