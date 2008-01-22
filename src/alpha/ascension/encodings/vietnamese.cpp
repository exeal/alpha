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
 * @date 2004-2008
 */

#include "../encoder.hpp"
using namespace ascension;
using namespace ascension::encoding;
using namespace ascension::encoding::implementation;
using namespace std;

namespace {
#ifndef ASCENSION_NO_STANDARD_ENCODINGS
	typedef CharWire<
		CharLine<0x0000, 0x0001, 0x1EB2, 0x0003, 0x0004, 0x1EB4, 0x1EAA, 0x0007, 0x0008, 0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E, 0x000F>,
		CharLine<0x0010, 0x0011, 0x0012, 0x0013, 0x1EF6, 0x0015, 0x0016, 0x0017, 0x0018, 0x1EF8, 0x001A, 0x001B, 0x001C, 0x001D, 0x1EF4, 0x001F>,
		CharLine<0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F>,
		CharLine<0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F>,
		CharLine<0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F>,
		CharLine<0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F>,
		CharLine<0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F>,
		CharLine<0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D, 0x007E, 0x007F>,
		CharLine<0x1EA0, 0x1EAE, 0x1EB0, 0x1EB6, 0x1EA4, 0x1EA6, 0x1EA8, 0x1EAC, 0x1EBC, 0x1EB8, 0x1EBE, 0x1EC0, 0x1EC2, 0x1EC4, 0x1EC6, 0x1ED0>,
		CharLine<0x1ED2, 0x1ED4, 0x1ED6, 0x1ED8, 0x1EE2, 0x1EDA, 0x1EDC, 0x1EDE, 0x1ECA, 0x1ECE, 0x1ECC, 0x1EC8, 0x1EE6, 0x0168, 0x1EE4, 0x1EF2>,
		CharLine<0x00D5, 0x1EAF, 0x1EB1, 0x1EB7, 0x1EA5, 0x1EA7, 0x1EA9, 0x1EAD, 0x1EBD, 0x1EB9, 0x1EBF, 0x1EC1, 0x1EC3, 0x1EC5, 0x1EC7, 0x1ED1>,
		CharLine<0x1ED3, 0x1ED5, 0x1ED7, 0x1EE0, 0x01A0, 0x1ED9, 0x1EDD, 0x1EDF, 0x1ECB, 0x1EF0, 0x1EE8, 0x1EEA, 0x1EEC, 0x01A1, 0x1EDB, 0x01AF>,
		CharLine<0x00C0, 0x00C1, 0x00C2, 0x00C3, 0x1EA2, 0x0102, 0x1EB3, 0x1EB5, 0x00C8, 0x00C9, 0x00CA, 0x1EBA, 0x00CC, 0x00CD, 0x0128, 0x1EF3>,
		CharLine<0x0110, 0x1EE9, 0x00D2, 0x00D3, 0x00D4, 0x1EA1, 0x1EF7, 0x1EEB, 0x1EED, 0x00D9, 0x00DA, 0x1EF9, 0x1EF5, 0x00DD, 0x1EE1, 0x01B0>,
		CharLine<0x00E0, 0x00E1, 0x00E2, 0x00E3, 0x1EA3, 0x0103, 0x1EEF, 0x1EAB, 0x00E8, 0x00E9, 0x00EA, 0x1EBB, 0x00EC, 0x00ED, 0x0129, 0x1EC9>,
		CharLine<0x0111, 0x1EF1, 0x00F2, 0x00F3, 0x00F4, 0x00F5, 0x1ECF, 0x1ECD, 0x1EE5, 0x00F9, 0x00FA, 0x0169, 0x1EE7, 0x00FD, 0x1EE3, 0x1EEE>
	> VISCII_BYTE_TABLE;
	sbcs::SingleByteEncoderFactory<VISCII_BYTE_TABLE> VISCII("VISCII", standard::VISCII, "Vietnamese (VISCII)", "csVISCII", 0x1A);
	sbcs::SingleByteEncoderFactory<
		CharWire<
			CharLine<0x0000, 0x00DA, 0x1EE4, 0x0003, 0x1EEA, 0x1EEC, 0x1EEE, 0x0007, 0x0008, 0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E, 0x000F>,
			CharLine<0x0010, 0x1EE8, 0x1EF0, 0x1EF2, 0x1EF6, 0x1EF8, 0x00DD, 0x1EF4, 0x0018, 0x0019, 0x001A, 0x001B, 0x001C, 0x001D, 0x001E, 0x001F>,
			CharLine<0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F>,
			CharLine<0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F>,
			CharLine<0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F>,
			CharLine<0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F>,
			CharLine<0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F>,
			CharLine<0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D, 0x007E, 0x007F>,
			CharLine<0x00C0, 0x1EA2, 0x00C3, 0x00C1, 0x1EA0, 0x1EB6, 0x1EAC, 0x00C8, 0x1EBA, 0x1EBC, 0x00C9, 0x1EB8, 0x1EC6, 0x00CC, 0x1EC8, 0x0128>,
			CharLine<0x00CD, 0x1ECA, 0x00D2, 0x1ECE, 0x00D5, 0x00D3, 0x1ECC, 0x1ED8, 0x1EDC, 0x1EDE, 0x1EE0, 0x1EDA, 0x1EE2, 0x00D9, 0x1EE6, 0x0168>,
			CharLine<0x00A0, 0x0102, 0x00C2, 0x00CA, 0x00D4, 0x01A0, 0x01AF, 0x0110, 0x0103, 0x00E2, 0x00EA, 0x00F4, 0x01A1, 0x01B0, 0x0111, 0x1EB0>,
			CharLine<0x0300, 0x0309, 0x0303, 0x0301, 0x0323, 0x00E0, 0x1EA3, 0x00E3, 0x00E1, 0x1EA1, 0x1EB2, 0x1EB1, 0x1EB3, 0x1EB5, 0x1EAF, 0x1EB4>,
			CharLine<0x1EAE, 0x1EA6, 0x1EA8, 0x1EAA, 0x1EA4, 0x1EC0, 0x1EB7, 0x1EA7, 0x1EA9, 0x1EAB, 0x1EA5, 0x1EAD, 0x00E8, 0x1EC2, 0x1EBB, 0x1EBD>,
			CharLine<0x00E9, 0x1EB9, 0x1EC1, 0x1EC3, 0x1EC5, 0x1EBF, 0x1EC7, 0x00EC, 0x1EC9, 0x1EC4, 0x1EBE, 0x1ED2, 0x0129, 0x00ED, 0x1ECB, 0x00F2>,
			CharLine<0x1ED4, 0x1ECF, 0x00F5, 0x00F3, 0x1ECD, 0x1ED3, 0x1ED5, 0x1ED7, 0x1ED1, 0x1ED9, 0x1EDD, 0x1EDF, 0x1EE1, 0x1EDB, 0x1EE3, 0x00F9>,
			CharLine<0x1ED6, 0x1EE7, 0x0169, 0x00FA, 0x1EE5, 0x1EEB, 0x1EED, 0x1EEF, 0x1EE9, 0x1EF1, 0x1EF3, 0x1EF7, 0x1EF9, 0x00FD, 0x1EF5, 0x1ED0>
		>
	> TCVN("TCVN", MIB_OTHER, "Vietnamese (TCVN)", "", 0x1A);
#endif /* !ASCENSION_NO_STANDARD_ENCODINGS */
#ifndef ASCENSION_NO_PROPRIETARY_ENCODINGS
	sbcs::SingleByteEncoderFactory<
		CharWire<
			CharLine<0x0000, 0x0001, 0x0002, 0x0003, 0x009C, 0x0009, 0x0086, 0x007F, 0x0097, 0x008D, 0x008E, 0x000B, 0x000C, 0x000D, 0x000E, 0x000F>,
			CharLine<0x0010, 0x0011, 0x0012, 0x0013, 0x009D, 0x0085, 0x0008, 0x0087, 0x0018, 0x0019, 0x0092, 0x008F, 0x001C, 0x001D, 0x001E, 0x001F>,
			CharLine<0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x000A, 0x0017, 0x001B, 0x0088, 0x0089, 0x008A, 0x008B, 0x008C, 0x0005, 0x0006, 0x0007>,
			CharLine<0x0090, 0x0091, 0x0016, 0x0093, 0x0094, 0x0095, 0x0096, 0x0004, 0x0098, 0x0099, 0x009A, 0x009B, 0x0014, 0x0015, 0x009E, 0x001A>,
			CharLine<0x0020, 0x00A0, 0x00E2, 0x00E4, 0x00E0, 0x00E1, 0x0103, 0x00E5, 0x00E7, 0x00F1, 0x005B, 0x002E, 0x003C, 0x0028, 0x002B, 0x0021>,
			CharLine<0x0026, 0x00E9, 0x00EA, 0x00EB, 0x00E8, 0x00ED, 0x00EE, 0x00EF, 0x0303, 0x00DF, 0x005D, 0x0024, 0x002A, 0x0029, 0x003B, 0x005E>,
			CharLine<0x002D, 0x002F, 0x00C2, 0x00C4, 0x00C0, 0x00C1, 0x0102, 0x00C5, 0x00C7, 0x00D1, 0x00A6, 0x002C, 0x0025, 0x005F, 0x003E, 0x003F>,
			CharLine<0x00F8, 0x00C9, 0x00CA, 0x00CB, 0x00C8, 0x00CD, 0x00CE, 0x00CF, 0x20AB, 0x0060, 0x003A, 0x0023, 0x0040, 0x0027, 0x003D, 0x0022>,
			CharLine<0x00D8, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x00AB, 0x00BB, 0x0111, 0x0309, 0x0300, 0x00B1>,
			CharLine<0x00B0, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F, 0x0070, 0x0071, 0x0072, 0x00AA, 0x00BA, 0x00E6, 0x0152, 0x00C6, 0x20AC>,
			CharLine<0x00B5, 0x007E, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007A, 0x00A1, 0x00BF, 0x0110, 0x0323, 0x0301, 0x00AE>,
			CharLine<0x00A2, 0x00A3, 0x00A5, 0x00B7, 0x00A9, 0x00A7, 0x00B6, 0x00BC, 0x00BD, 0x00BE, 0x00AC, 0x007C, 0x00AF, 0x0153, 0x0178, 0x00D7>,
			CharLine<0x007B, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x00AD, 0x00F4, 0x00F6, 0x01B0, 0x00F3, 0x01A1>,
			CharLine<0x007D, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F, 0x0050, 0x0051, 0x0052, 0x00B9, 0x00FB, 0x00FC, 0x00F9, 0x00FA, 0x00FF>,
			CharLine<0x005C, 0x00F7, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005A, 0x00B2, 0x00D4, 0x00D6, 0x01AF, 0x00D3, 0x01A0>,
			CharLine<0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x00B3, 0x00DB, 0x00DC, 0x00D9, 0x00DA, 0x009F>
		>
	> IBM1164("IBM1164", MIB_OTHER, "Vietnamese (EBCDIC Viet Nam (IBM1130 + Euro))", "\0ibm-1164_P100-1999", 0x3F);
	sbcs::SingleByteEncoderFactory<
		CharWire<
			CharLine<0x0000, 0x0001, 0x0002, 0x0003, 0x009C, 0x0009, 0x0086, 0x007F, 0x0097, 0x008D, 0x008E, 0x000B, 0x000C, 0x000D, 0x000E, 0x000F>,
			CharLine<0x0010, 0x0011, 0x0012, 0x0013, 0x009D, 0x0085, 0x0008, 0x0087, 0x0018, 0x0019, 0x0092, 0x008F, 0x001C, 0x001D, 0x001E, 0x001F>,
			CharLine<0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x000A, 0x0017, 0x001B, 0x0088, 0x0089, 0x008A, 0x008B, 0x008C, 0x0005, 0x0006, 0x0007>,
			CharLine<0x0090, 0x0091, 0x0016, 0x0093, 0x0094, 0x0095, 0x0096, 0x0004, 0x0098, 0x0099, 0x009A, 0x009B, 0x0014, 0x0015, 0x009E, 0x001A>,
			CharLine<0x0020, 0x00A0, 0x00E2, 0x00E4, 0x0163, 0x00E1, 0x0103, 0x010D, 0x00E7, 0x0107, 0x00DD, 0x002E, 0x003C, 0x0028, 0x002B, 0x007C>,
			CharLine<0x0026, 0x00E9, 0x0119, 0x00EB, 0x016F, 0x00ED, 0x00EE, 0x013E, 0x013A, 0x00DF, 0x0021, 0x0024, 0x002A, 0x0029, 0x003B, 0x005E>,
			CharLine<0x002D, 0x002F, 0x00C2, 0x00C4, 0x02DD, 0x00C1, 0x0102, 0x010C, 0x00C7, 0x0106, 0x00A8, 0x002C, 0x0025, 0x005F, 0x003E, 0x003F>,
			CharLine<0x02C7, 0x00C9, 0x0118, 0x00CB, 0x016E, 0x00CD, 0x00CE, 0x013D, 0x0139, 0x0060, 0x003A, 0x0023, 0x0040, 0x0027, 0x003D, 0x0022>,
			CharLine<0x02D8, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x015B, 0x0148, 0x0111, 0x00FD, 0x0159, 0x015F>,
			CharLine<0x00B0, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F, 0x0070, 0x0071, 0x0072, 0x0142, 0x0144, 0x0161, 0x00B8, 0x02DB, 0x20AC>,
			CharLine<0x0105, 0x007E, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007A, 0x015A, 0x0147, 0x0110, 0x005B, 0x0158, 0x015E>,
			CharLine<0x02D9, 0x0104, 0x017C, 0x0162, 0x017B, 0x00A7, 0x017E, 0x017A, 0x017D, 0x0179, 0x0141, 0x0143, 0x0160, 0x005D, 0x00B4, 0x00D7>,
			CharLine<0x007B, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x00AD, 0x00F4, 0x00F6, 0x0155, 0x00F3, 0x0151>,
			CharLine<0x007D, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F, 0x0050, 0x0051, 0x0052, 0x011A, 0x0171, 0x00FC, 0x0165, 0x00FA, 0x011B>,
			CharLine<0x005C, 0x00F7, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005A, 0x010F, 0x00D4, 0x00D6, 0x0154, 0x00D3, 0x0150>,
			CharLine<0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x010E, 0x0170, 0x00DC, 0x0164, 0x00DA, 0x009F>
		>
	> IBM1165("IBM1165", MIB_OTHER, "Vietnamese (EBCDIC)", "\0ibm-1165_P101-2000", 0x3F);
	sbcs::SingleByteEncoderFactory<
		sbcs::ASCIICompatibleCharWire<
			CharLine<0x20AC, 0x0081, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021, 0x02C6, 0x2030, 0x008A, 0x2039, 0x0152, 0x008D, 0x008E, 0x008F>,
			CharLine<0x0090, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, 0x02DC, 0x2122, 0x009A, 0x203A, 0x0153, 0x009D, 0x009E, 0x0178>,
			CharLine<0x00A0, 0x00A1, 0x00A2, 0x00A3, 0x00A4, 0x00A5, 0x00A6, 0x00A7, 0x00A8, 0x00A9, 0x00AA, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF>,
			CharLine<0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00B4, 0x00B5, 0x00B6, 0x00B7, 0x00B8, 0x00B9, 0x00BA, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x00BF>,
			CharLine<0x00C0, 0x00C1, 0x00C2, 0x0102, 0x00C4, 0x00C5, 0x00C6, 0x00C7, 0x00C8, 0x00C9, 0x00CA, 0x00CB, 0x0300, 0x00CD, 0x00CE, 0x00CF>,
			CharLine<0x0110, 0x00D1, 0x0309, 0x00D3, 0x00D4, 0x01A0, 0x00D6, 0x00D7, 0x00D8, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x01AF, 0x0303, 0x00DF>,
			CharLine<0x00E0, 0x00E1, 0x00E2, 0x0103, 0x00E4, 0x00E5, 0x00E6, 0x00E7, 0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x0301, 0x00ED, 0x00EE, 0x00EF>,
			CharLine<0x0111, 0x00F1, 0x0323, 0x00F3, 0x00F4, 0x01A1, 0x00F6, 0x00F7, 0x00F8, 0x00F9, 0x00FA, 0x00FB, 0x00FC, 0x01B0, 0x20AB, 0x00FF>
		>
	> WINDOWS_1258("windows-1258", proprietary::WINDOWS_1258, "Vietnamese (Windows)", "\0ibm-5354|cp1258|ibm-5354_P100-1998", 0x3F);
#endif /* !ASCENSION_NO_PROPRIETARY_ENCODINGS */
#ifndef ASCENSION_NO_MINORITY_ENCODINGS
	sbcs::SingleByteEncoderFactory<
		CharWire<
			CharLine<0x0000, 0x0001, 0x1EA0, 0x1EAC, 0x1EB6, 0x1EB8, 0x1EC6, 0x0007, 0x0008, 0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E, 0x000F>,
			CharLine<0x1ECA, 0x1ECC, 0x1ED8, 0x1EE2, 0x1EE4, 0x1EF0, 0x0016, 0x0017, 0x0018, 0x1EF4, 0x001A, 0x001B, 0x1EAA, 0x1EEE, 0x001E, 0x001F>,
			CharLine<0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F>,
			CharLine<0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F>,
			CharLine<0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F>,
			CharLine<0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F>,
			CharLine<0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F>,
			CharLine<0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D, 0x007E, 0x007F>,
			CharLine<0x00C0, 0x1EA2, 0x00C3, 0x1EA4, 0x1EA6, 0x1EA8, 0x1ECD, 0x1ED7, 0x0102, 0x1EBF, 0x1EC1, 0x1EC3, 0x1EC7, 0x1EAE, 0x1EB0, 0x1EB2>,
			CharLine<0x1EBE, 0x2018, 0x2019, 0x1EC0, 0x1EC2, 0x1EC4, 0x1ED0, 0x1ED2, 0x1ED4, 0x1ED6, 0x00FD, 0x1EF7, 0x1EF5, 0x1EDA, 0x1EDC, 0x1EDE>,
			CharLine<0x00A0, 0x1EAF, 0x1EB1, 0x1EB3, 0x1EB5, 0x1EB7, 0x1EE0, 0x1EDB, 0x00D9, 0x1EDD, 0x1EDF, 0x1EE1, 0x0168, 0x1EE8, 0x1EE3, 0x1EEA>,
			CharLine<0x1ED5, 0x1EEC, 0x1EF2, 0x1EF8, 0x00CD, 0x00CC, 0x1ED9, 0x1EC8, 0x0128, 0x00D3, 0x1EED, 0x1EEF, 0x00D2, 0x1ECE, 0x00D5, 0x1EF1>,
			CharLine<0x1EA7, 0x00C1, 0x00C2, 0x1EA5, 0x1EA9, 0x1EAB, 0x1EAD, 0x0111, 0x1EBB, 0x00C9, 0x00CA, 0x1EB9, 0x1EC9, 0x1EC5, 0x1ECB, 0x1EF9>,
			CharLine<0x01AF, 0x1EE6, 0x1ED3, 0x1ED1, 0x00D4, 0x1ECF, 0x01A1, 0x00C8, 0x1EEB, 0x1EE9, 0x00DA, 0x0169, 0x01B0, 0x00DD, 0x1EBA, 0x00DF>,
			CharLine<0x00E0, 0x00E1, 0x00E2, 0x00E3, 0x1EA3, 0x1EA1, 0x0103, 0x00E7, 0x00E8, 0x00E9, 0x00EA, 0x1EBD, 0x00EC, 0x00ED, 0x00EE, 0x0129>,
			CharLine<0x1EB4, 0x0110, 0x00F2, 0x00F3, 0x00F4, 0x00F5, 0x00F6, 0x01A0, 0x1EE5, 0x00F9, 0x00FA, 0x1EE7, 0x00FC, 0x1EF6, 0x1EBC, 0x1EF3>
		>
	> VPS("VPS", MIB_OTHER, "Vietnamese (VPS)", "", 0x1A);
	sbcs::SingleByteEncoderFactory<
		sbcs::ISO8859CompatibleCharWire<
			CharLine<0x00A0, 0x00A1, 0x00A2, 0x00A3, 0x20AC, 0x00A5, 0x00A6, 0x00A7, 0x0153, 0x00A9, 0x00AA, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF>,
			CharLine<0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x0178, 0x00B5, 0x00B6, 0x00B7, 0x0152, 0x00B9, 0x00BA, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x00BF>,
			CharLine<0x00C0, 0x00C1, 0x00C2, 0x0102, 0x00C4, 0x00C5, 0x00C6, 0x00C7, 0x00C8, 0x00C9, 0x00CA, 0x00CB, 0x0300, 0x00CD, 0x00CE, 0x00CF>,
			CharLine<0x0110, 0x00D1, 0x0309, 0x00D3, 0x00D4, 0x01A0, 0x00D6, 0x00D7, 0x00D8, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x01AF, 0x0303, 0x00DF>,
			CharLine<0x00E0, 0x00E1, 0x00E2, 0x0103, 0x00E4, 0x00E5, 0x00E6, 0x00E7, 0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x0301, 0x00ED, 0x00EE, 0x00EF>,
			CharLine<0x0111, 0x00F1, 0x0323, 0x00F3, 0x00F4, 0x01A1, 0x00F6, 0x00F7, 0x00F8, 0x00F9, 0x00FA, 0x00FB, 0x00FC, 0x01B0, 0x20AB, 0x00FF>
		>
	> IBM1163("IBM1163", MIB_OTHER, "Vietnamese (IBM1163)", "\0ibm-1163_P100-1999", 0x1A);
#endif /* !ASCENSION_NO_MINORITY_ENCODINGS */

#ifndef ASCENSION_NO_STANDARD_ENCODINGS
	class VIQREncoder : public Encoder {
	public:
		VIQREncoder() throw();
	private:
		Result doFromUnicode(byte* to, byte* toEnd, byte*& toNext, const Char* from, const Char* fromEnd, const Char*& fromNext);
		Result doToUnicode(Char* to, Char* toEnd, Char*& toNext, const byte* from, const byte* fromEnd, const byte*& fromNext);
		const IEncodingProperties& properties() const throw();
		Encoder& resetDecodingState() throw();
		Encoder& resetEncodingState() throw();
	private:
		enum {LITERAL_STATE, ENGLISH_STATE, VIETNAMESE_STATE} encodingState_, decodingState_;
		static const byte CLS = 0x01, COM = 0x5C;
		static auto_ptr<sbcs::BidirectionalMap> table_;
	};
	class VIQRFactory : public implementation::EncoderFactoryBase {
	public:
		VIQRFactory() throw() : implementation::EncoderFactoryBase("VIQR", standard::VIQR, "Vietnamese (VIQR)", 3, 1, "csVIQR", 0x1A) {}
	private:
		auto_ptr<Encoder> create() const throw() {return auto_ptr<Encoder>(new VIQREncoder);}
	} VIQR;
#endif /* !ASCENSION_NO_STANDARD_ENCODINGS */

	struct Installer {
		Installer() {
#ifndef ASCENSION_NO_STANDARD_ENCODINGS
			Encoder::registerFactory(VISCII);
			Encoder::registerFactory(VIQR);
			Encoder::registerFactory(TCVN);
#endif /* !ASCENSION_NO_STANDARD_ENCODINGS */
#ifndef ASCENSION_NO_PROPRIETARY_ENCODINGS
			Encoder::registerFactory(IBM1164);
			Encoder::registerFactory(IBM1165);
			Encoder::registerFactory(WINDOWS_1258);
#endif /* !ASCENSION_NO_PROPRIETARY_ENCODINGS */
#ifndef ASCENSION_NO_MINORITY_ENCODINGS
			Encoder::registerFactory(VPS);
			Encoder::registerFactory(IBM1163);
#endif /* !ASCENSION_NO_MINORITY_ENCODINGS */
		}
	} installer;
} // namespace @0

#ifndef ASCENSION_NO_STANDARD_ENCODINGS

auto_ptr<sbcs::BidirectionalMap> VIQREncoder::table_;

VIQREncoder::VIQREncoder() throw() : encodingState_(VIETNAMESE_STATE), decodingState_(VIETNAMESE_STATE) {
	if(table_.get() == 0)
		table_.reset(new sbcs::BidirectionalMap(VISCII_BYTE_TABLE::VALUES));
}

Encoder::Result VIQREncoder::doFromUnicode(byte* to, byte* toEnd,
		byte*& toNext, const Char* from, const Char* fromEnd, const Char*& fromNext) {
	static const byte VISCII_TO_VIQR[] =
		"\0\001A(?\003\004A(~A^~\007"		"\010\011\012\013\014\015\016\017"
		"\020\021\022\023Y?\024\025\026"	"\030Y~\031\032\033\034Y.\037"
		" !\"#$%&'"							"()*+,-./"
		"01234567"							"89:;<=>?"
		"@ABCDEFG"							"HIJKLMNO"
		"PQRSTUVW"							"XYZ[\\]^_"
		"`abcdefg"							"hijklmno"
		"pqrstuvw"							"xyz{|}~\x7F"
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
		byte viscii = table_->toByte(*from);
		if(viscii == sbcs::UNMAPPABLE_BYTE && *from != sbcs::UNMAPPABLE_BYTE) {
			if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTER)
				continue;
			else if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTER)
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
		Char*& toNext, const byte* from, const byte* fromEnd, const byte*& fromNext) {
	enum {NONE, BREVE, CIRCUMFLEX, HORN, ACUTE, GRAVE, HOOK_ABOVE, TILDE, DOT_BELOW, CAPITAL_D, SMALL_D, DIACRITICALS_COUNT};
	static const byte MNEMONIC_TABLE[0x80] = {
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
	static const byte BASE_CHARACTER_TABLE[0x80] = {
		0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,	// 0x00
		0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
		0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
		0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
		0x80, 0x00, 0x80, 0x80, 0x03, 0x04, 0x80, 0x80, 0x80, 0x06, 0x80, 0x80, 0x80, 0x80, 0x80, 0x07,	// 0x40
		0x80, 0x80, 0x80, 0x80, 0x80, 0x0A, 0x80, 0x80, 0x80, 0x0C, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,	// 0x50
		0x80, 0x0D, 0x80, 0x80, 0x10, 0x11, 0x80, 0x80, 0x80, 0x13, 0x80, 0x80, 0x80, 0x80, 0x80, 0x14,	// 0x60
		0x80, 0x80, 0x80, 0x80, 0x80, 0x17, 0x80, 0x80, 0x80, 0x19, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80	// 0x70
	};
	static const Char STATE_TABLE[][DIACRITICALS_COUNT] = {
	//	 <>      (       ^       +       '       `       ?       ~       .       D       d
		{0x0041,   0x01,   0x02, 0x0041, 0x00C1, 0x00C0, 0x1EA2, 0x00C3, 0x1EA0, 0x0041, 0x0041},	// 0x00 : A
		{0x0102, 0x0102, 0x0102, 0x0102, 0x1EAE, 0x1EB0, 0x1EB2, 0x1EB4, 0x1EB6, 0x0102, 0x0102},	// 0x01 : A(
		{0x00C2, 0x00C2, 0x00C2, 0x00C2, 0x1EA4, 0x1EA6, 0x1EA8, 0x1EAA, 0x1EAC, 0x00C2, 0x00C2},	// 0x02 : A^
		{0x0044, 0x0044, 0x0044, 0x0044, 0x0044, 0x0044, 0x0044, 0x0044, 0x0044, 0x0110, 0x0110},	// 0x03 : D
		{0x0045, 0x0045,   0x05, 0x0045, 0x00C9, 0x00C8, 0x1EBA, 0x1EBC, 0x1EB8, 0x0045, 0x0045},	// 0x04 : E
		{0x00CA, 0x00CA, 0x00CA, 0x00CA, 0x1EBE, 0x1EC0, 0x1EC2, 0x1EC4, 0x1EC6, 0x00CA, 0x00CA},	// 0x05 : E^
		{0x0049, 0x0049, 0x0049, 0x0049, 0x00CD, 0x00CC, 0x1EC8, 0x0128, 0x1ECA, 0x0049, 0x0049},	// 0x06 : I
		{0x004F, 0x004F,   0x08,   0x09, 0x00D3, 0x00D2, 0x1ECE, 0x00D5, 0x1ECC, 0x004F, 0x004F},	// 0x07 : O
		{0x00D4, 0x00D4, 0x00D4, 0x00D4, 0x1ED0, 0x1ED2, 0x1ED4, 0x1ED6, 0x1ED8, 0x00D4, 0x00D4},	// 0x08 : O^
		{0x01A0, 0x01A0, 0x01A0, 0x01A0, 0x1EDA, 0x1EDC, 0x1EDE, 0x1EE0, 0x1EE2, 0x01A0, 0x01A0},	// 0x09 : O+
		{0x0055, 0x0055, 0x0055,   0x0B, 0x00DA, 0x00D9, 0x1EE6, 0x0168, 0x1EE4, 0x0055, 0x0055},	// 0x0A : U
		{0x01AF, 0x01AF, 0x01AF, 0x01AF, 0x1EE8, 0x1EEA, 0x1EEC, 0x1EEE, 0x1EF0, 0x01AF, 0x01AF},	// 0x0B : U+
		{0x0059, 0x0059, 0x0059, 0x0059, 0x00DD, 0x1EF2, 0x1EF6, 0x1EF8, 0x1EF4, 0x0059, 0x0059},	// 0x0C : Y
		{0x0061,   0x0E,   0x0F, 0x0061, 0x00E1, 0x00E0, 0x1EA3, 0x00E3, 0x1EA1, 0x0061, 0x0061},	// 0x0D : a
		{0x0103, 0x0103, 0x0103, 0x0103, 0x1EAF, 0x1EB1, 0x1EB3, 0x1EB5, 0x1EB7, 0x0103, 0x0103},	// 0x0E : a(
		{0x00E2, 0x00E2, 0x00E2, 0x00E2, 0x1EA5, 0x1EA7, 0x1EA9, 0x1EAB, 0x1EAD, 0x00E2, 0x00E2},	// 0x0F : a^
		{0x0064, 0x0064, 0x0064, 0x0064, 0x0064, 0x0064, 0x0064, 0x0064, 0x0064, 0x0110, 0x0111},	// 0x10 : d
		{0x0065, 0x0065,   0x12, 0x0065, 0x00E9, 0x00E8, 0x1EBB, 0x1EBD, 0x1EB9, 0x0065, 0x0065},	// 0x11 : e
		{0x00EA, 0x00EA, 0x00EA, 0x00EA, 0x1EBF, 0x1EC1, 0x1EC3, 0x1EC5, 0x1EC7, 0x00EA, 0x00EA},	// 0x12 : e^
		{0x0069, 0x0069, 0x0069, 0x0069, 0x00ED, 0x00EC, 0x1EC9, 0x0129, 0x1ECB, 0x0069, 0x0069},	// 0x13 : i
		{0x006F, 0x006F,   0x15,   0x16, 0x00F3, 0x00F2, 0x1ECF, 0x00F5, 0x1ECD, 0x006F, 0x006F},	// 0x14 : o
		{0x00F4, 0x00F4, 0x00F4, 0x00F4, 0x1ED1, 0x1ED3, 0x1ED5, 0x1ED7, 0x1ED9, 0x00F4, 0x00F4},	// 0x15 : o^
		{0x01A1, 0x01A1, 0x01A1, 0x01A1, 0x1EDB, 0x1EDD, 0x1EDF, 0x1EE1, 0x1EE3, 0x01A1, 0x01A1},	// 0x16 : o+
		{0x0075, 0x0075, 0x0075,   0x18, 0x00FA, 0x00F9, 0x1EE7, 0x0169, 0x1EE5, 0x0075, 0x0075},	// 0x17 : u
		{0x01B0, 0x01B0, 0x01B0, 0x01B0, 0x1EE9, 0x1EEB, 0x1EED, 0x1EEF, 0x1EF1, 0x01B0, 0x01B0},	// 0x18 : u+
		{0x0079, 0x0079, 0x0079, 0x0079, 0x00FD, 0x1EF3, 0x1EF7, 0x1EF9, 0x1EF5, 0x0079, 0x0079}	// 0x19 : y
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
			byte mnemonic = BASE_CHARACTER_TABLE[*from];
			if(mnemonic != 0x80) {
				// ... got the base character
				if(from + 1 == fromEnd) {
					if(!flags().has(CONTINUOUS_INPUT)) {
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
					if(!flags().has(CONTINUOUS_INPUT)) {
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

const IEncodingProperties& VIQREncoder::properties() const throw() {
	return VIQR;
}

Encoder& VIQREncoder::resetDecodingState() throw() {
	decodingState_ = VIETNAMESE_STATE;
	return *this;
}

Encoder& VIQREncoder::resetEncodingState() throw() {
	encodingState_ = VIETNAMESE_STATE;
	return *this;
}

#endif /* !ASCENSION_NO_STANDARD_ENCODINGS */
