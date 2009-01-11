/**
 * @file arabic.cpp
 * Implements Arabic encodings. This includes:
 * - ISO 8859-6:1999
 * - IBM425
 * - IBM864
 * - IBM1046
 * - IBM9056
 * - IBM9238
 * - IBM16804
 * - windows-1256
 * - <del>MacArabic</del>
 * @author exeal
 * @date 2007-2009
 */

#include <ascension/encoder.hpp>
using namespace ascension::encoding;
using namespace ascension::encoding::implementation;
using namespace std;

namespace {
#ifndef ASCENSION_NO_STANDARD_ENCODINGS
	sbcs::SingleByteEncoderFactory<
		sbcs::ISO8859CompatibleCharWire<
			CharLine<0x00A0, 0xFFFD, 0xFFFD, 0xFFFD, 0x00A4, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0x060C, 0x00AD, 0xFFFD, 0xFFFD>,
			CharLine<0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0x061B, 0xFFFD, 0xFFFD, 0xFFFD, 0x061F>,
			CharLine<0xFFFD, 0x0621, 0x0622, 0x0623, 0x0624, 0x0625, 0x0626, 0x0627, 0x0628, 0x0629, 0x062A, 0x062B, 0x062C, 0x062D, 0x062E, 0x062F>,
			CharLine<0x0630, 0x0631, 0x0632, 0x0633, 0x0634, 0x0635, 0x0636, 0x0637, 0x0638, 0x0639, 0x063A, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD>,
			CharLine<0x0640, 0x0641, 0x0642, 0x0643, 0x0644, 0x0645, 0x0646, 0x0647, 0x0648, 0x0649, 0x064A, 0x064B, 0x064C, 0x064D, 0x064E, 0x064F>,
			CharLine<0x0650, 0x0651, 0x0652, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD>
		>
	> ISO_8859_6("ISO-8859-6", standard::ISO_8859_6, "Arabic (ISO 8859-6)",
		"iso-ir-127|ISO_8859-6|ECMA-114|ASMO-708|arabic|csISOLatinArabic"
		"\0ibm-1089|8859_6|cp1089|1089|windows-28596|ISO-8859-6-I|ISO-8859-6-E|ibm-1089_P100-1995", 0x1A);
#endif // !ASCENSION_NO_STANDARD_ENCODINGS
#ifndef ASCENSION_NO_PROPRIETARY_ENCODINGS
	sbcs::SingleByteEncoderFactory<
		sbcs::IBMPCCompatibleCharWire<
			CharLine<0x00B0, 0x00B7, 0x2219, 0x221A, 0x2592, 0x2500, 0x2502, 0x253C, 0x2524, 0x252C, 0x251C, 0x2534, 0x2510, 0x250C, 0x2514, 0x2518>,
			CharLine<0x03B2, 0x221E, 0x03C6, 0x00B1, 0x00BD, 0x00BC, 0x2248, 0x00AB, 0x00BB, 0xFEF7, 0xFEF8, 0xFFFD, 0xFFFD, 0xFEFB, 0xFEFC, 0x200B>,
			CharLine<0x00A0, 0x00AD, 0xFE82, 0x00A3, 0x00A4, 0xFE84, 0xFFFD, 0xFFFD, 0xFE8E, 0xFE8F, 0xFE95, 0xFE99, 0x060C, 0xFE9D, 0xFEA1, 0xFEA5>,
			CharLine<0x0660, 0x0661, 0x0662, 0x0663, 0x0664, 0x0665, 0x0666, 0x0667, 0x0668, 0x0669, 0xFED1, 0x061B, 0xFEB1, 0xFEB5, 0xFEB9, 0x061F>,
			CharLine<0x00A2, 0xFE80, 0xFE81, 0xFE83, 0xFE85, 0xFECA, 0xFE8B, 0xFE8D, 0xFE91, 0xFE93, 0xFE97, 0xFE9B, 0xFE9F, 0xFEA3, 0xFEA7, 0xFEA9>,
			CharLine<0xFEAB, 0xFEAD, 0xFEAF, 0xFEB3, 0xFEB7, 0xFEBB, 0xFEBF, 0xFEC3, 0xFEC7, 0xFECB, 0xFECF, 0x00A6, 0x00AC, 0x00F7, 0x00D7, 0xFEC9>,
			CharLine<0x0640, 0xFED3, 0xFED7, 0xFEDB, 0xFEDF, 0xFEE3, 0xFEE7, 0xFEEB, 0xFEED, 0xFEEF, 0xFEF3, 0xFEBD, 0xFECC, 0xFECE, 0xFECD, 0xFEE1>,
			CharLine<0xFE7D, 0xFE7C, 0xFEE5, 0xFEE9, 0xFEEC, 0xFEF0, 0xFEF2, 0xFED0, 0xFED5, 0xFEF5, 0xFEF6, 0xFEDD, 0xFED9, 0xFEF1, 0x25A0, 0xFFFD>
		>
	> IBM864("IBM864", proprietary::IBM864, "Arabic (IBM864)", "cp864|csIBM864\0ibm-864|ibm-864_X110-1999", 0x7F);
	sbcs::SingleByteEncoderFactory<
		CharWire<
			CharLine<0x0000, 0x0001, 0x0002, 0x0003, 0x009C, 0x0009, 0x0086, 0x007F, 0x0097, 0x008D, 0x008E, 0x000B, 0x000C, 0x000D, 0x000E, 0x000F>,
			CharLine<0x0010, 0x0011, 0x0012, 0x0013, 0x009D, 0x0085, 0x0008, 0x0087, 0x0018, 0x0019, 0x0092, 0x008F, 0x001C, 0x001D, 0x001E, 0x001F>,
			CharLine<0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x000A, 0x0017, 0x001B, 0x0088, 0x0089, 0x008A, 0x008B, 0x008C, 0x0005, 0x0006, 0x0007>,
			CharLine<0x0090, 0x0091, 0x0016, 0x0093, 0x0094, 0x0095, 0x0096, 0x0004, 0x0098, 0x0099, 0x009A, 0x009B, 0x0014, 0x0015, 0x009E, 0x001A>,
			CharLine<0x0020, 0x00A0, 0x0651, 0xFE7D, 0x0640, 0x200B, 0x0621, 0x0622, 0xFE82, 0x0623, 0x00A2, 0x002E, 0x003C, 0x0028, 0x002B, 0x007C>,
			CharLine<0x0026, 0xFE84, 0x0624, 0xFFFD, 0xFFFD, 0x0626, 0x0627, 0xFE8E, 0x0628, 0xFE91, 0x0021, 0x0024, 0x002A, 0x0029, 0x003B, 0x00AC>,
			CharLine<0x002D, 0x002F, 0x0629, 0x062A, 0xFE97, 0x062B, 0xFE9B, 0x062C, 0xFE9F, 0x062D, 0x00A6, 0x002C, 0x0025, 0x005F, 0x003E, 0x003F>,
			CharLine<0xFEA3, 0x062E, 0xFEA7, 0x062F, 0x0630, 0x0631, 0x0632, 0x0633, 0xFEB3, 0x060C, 0x003A, 0x0023, 0x0040, 0x0027, 0x003D, 0x0022>,
			CharLine<0x0634, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0xFEB7, 0x0635, 0xFEBB, 0x0636, 0xFEBF, 0x0637>,
			CharLine<0x0638, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F, 0x0070, 0x0071, 0x0072, 0x0639, 0xFECA, 0xFECB, 0xFECC, 0x063A, 0xFECE>,
			CharLine<0xFECF, 0x00F7, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007A, 0xFED0, 0x0641, 0xFED3, 0x0642, 0xFED7, 0x0643>,
			CharLine<0xFEDB, 0x0644, 0xFEF5, 0xFEF6, 0xFEF7, 0xFEF8, 0xFFFD, 0xFFFD, 0xFEFB, 0xFEFC, 0xFEDF, 0x0645, 0xFEE3, 0x0646, 0xFEE7, 0x0647>,
			CharLine<0x061B, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x00AD, 0xFEEB, 0xFFFD, 0xFEEC, 0xFFFD, 0x0648>,
			CharLine<0x061F, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F, 0x0050, 0x0051, 0x0052, 0x0649, 0xFEF0, 0x064A, 0xFEF2, 0xFEF3, 0x0660>,
			CharLine<0x00D7, 0x2007, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005A, 0x0661, 0x0662, 0xFFFD, 0x0663, 0x0664, 0x0665>,
			CharLine<0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x20AC, 0x0666, 0x0667, 0x0668, 0x0669, 0x009F>
		>
	> IBM16804("IBM16804", MIB_OTHER, "Arabic (IBM16804 (IBM420 + Euro))", "ibm-16804|ibm-16804_X110-1999", 0x3F);
	sbcs::SingleByteEncoderFactory<
		sbcs::ASCIICompatibleCharWire<
			CharLine<0x20AC, 0x067E, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021, 0x02C6, 0x2030, 0x0679, 0x2039, 0x0152, 0x0686, 0x0698, 0x0688>,
			CharLine<0x06AF, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, 0x06A9, 0x2122, 0x0691, 0x203A, 0x0153, 0x200C, 0x200D, 0x06BA>,
			CharLine<0x00A0, 0x060C, 0x00A2, 0x00A3, 0x00A4, 0x00A5, 0x00A6, 0x00A7, 0x00A8, 0x00A9, 0x06BE, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF>,
			CharLine<0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00B4, 0x00B5, 0x00B6, 0x00B7, 0x00B8, 0x00B9, 0x061B, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x061F>,
			CharLine<0x06C1, 0x0621, 0x0622, 0x0623, 0x0624, 0x0625, 0x0626, 0x0627, 0x0628, 0x0629, 0x062A, 0x062B, 0x062C, 0x062D, 0x062E, 0x062F>,
			CharLine<0x0630, 0x0631, 0x0632, 0x0633, 0x0634, 0x0635, 0x0636, 0x00D7, 0x0637, 0x0638, 0x0639, 0x063A, 0x0640, 0x0641, 0x0642, 0x0643>,
			CharLine<0x00E0, 0x0644, 0x00E2, 0x0645, 0x0646, 0x0647, 0x0648, 0x00E7, 0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x0649, 0x064A, 0x00EE, 0x00EF>,
			CharLine<0x064B, 0x064C, 0x064D, 0x064E, 0x00F4, 0x064F, 0x0650, 0x00F7, 0x0651, 0x00F9, 0x0652, 0x00FB, 0x00FC, 0x200E, 0x200F, 0x06D2>
		>
	> WINDOWS_1256("windows-1256", proprietary::WINDOWS_1256, "Arabic (windows-1256)", "\0cp1256|ibm-9448|ibm-9448_X100-2005", 0x3F);
#endif // !ASCENSION_NO_PROPRIETARY_ENCODINGS
#ifndef ASCENSION_NO_MINORITY_ENCODINGS
	sbcs::SingleByteEncoderFactory<
		CharWire<
			CharLine<0x0000, 0x0001, 0x0002, 0x0003, 0x009C, 0x0009, 0x0086, 0x007F, 0x0097, 0x008D, 0x008E, 0x000B, 0x000C, 0x000D, 0x000E, 0x000F>,
			CharLine<0x0010, 0x0011, 0x0012, 0x0013, 0x009D, 0x0085, 0x0008, 0x0087, 0x0018, 0x0019, 0x0092, 0x008F, 0x001C, 0x001D, 0x001E, 0x001F>,
			CharLine<0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x000A, 0x0017, 0x001B, 0x0088, 0x0089, 0x008A, 0x008B, 0x008C, 0x0005, 0x0006, 0x0007>,
			CharLine<0x0090, 0x0091, 0x0016, 0x0093, 0x0094, 0x0095, 0x0096, 0x0004, 0x0098, 0x0099, 0x009A, 0x009B, 0x0014, 0x0015, 0x009E, 0x001A>,
			CharLine<0x0020, 0x00A0, 0x00E2, 0x060C, 0x00E0, 0x061B, 0x0640, 0x061F, 0x00E7, 0x0621, 0x0622, 0x002E, 0x003C, 0x0028, 0x002B, 0x007C>,
			CharLine<0x0026, 0x00E9, 0x00EA, 0x00EB, 0x00E8, 0x0623, 0x00EE, 0x00EF, 0x0624, 0x0625, 0x0021, 0x0024, 0x002A, 0x0029, 0x003B, 0x005E>,
			CharLine<0x002D, 0x002F, 0x00C2, 0x0626, 0x00C0, 0x0627, 0x0628, 0x0629, 0x00C7, 0x062A, 0x062B, 0x002C, 0x0025, 0x005F, 0x003E, 0x003F>,
			CharLine<0x062C, 0x00C9, 0x00CA, 0x00CB, 0x00C8, 0x062D, 0x00CE, 0x00CF, 0x062E, 0x0060, 0x003A, 0x0023, 0x0040, 0x0027, 0x003D, 0x0022>,
			CharLine<0x062F, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x00AB, 0x00BB, 0x0630, 0x0631, 0x0632, 0x0633>,
			CharLine<0x0634, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F, 0x0070, 0x0071, 0x0072, 0x0635, 0x0636, 0x00E6, 0x0637, 0x00C6, 0x20AC>,
			CharLine<0x00B5, 0x007E, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007A, 0x0638, 0x0639, 0x063A, 0x005B, 0x0641, 0x0642>,
			CharLine<0x0643, 0x0644, 0x0645, 0x0646, 0x00A9, 0x00A7, 0x0647, 0x0152, 0x0153, 0x0178, 0x0648, 0x0649, 0x064A, 0x005D, 0x064B, 0x00D7>,
			CharLine<0x007B, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x00AD, 0x00F4, 0x064C, 0x064D, 0x064E, 0x064F>,
			CharLine<0x007D, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F, 0x0050, 0x0051, 0x0052, 0x0650, 0x00FB, 0x00FC, 0x00F9, 0x0651, 0x00FF>,
			CharLine<0x005C, 0x00F7, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005A, 0x0652, 0x00D4, 0x200C, 0x200D, 0x200E, 0x200F>,
			CharLine<0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0xFFFD, 0x00DB, 0x00DC, 0x00D9, 0x00A4, 0x009F>
		>
	> IBM425("IBM425", MIB_OTHER, "Arabic (IBM425)", "ibm-425|ibm-425_P101-2000", 0x3F);
	sbcs::SingleByteEncoderFactory<
		sbcs::ASCIICompatibleCharWire<
			CharLine<0xFE88, 0x00D7, 0x00F7, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFE71, 0x0088, 0x25A0, 0x2502, 0x2500, 0x2510, 0x250C, 0x2514, 0x2518>,
			CharLine<0xFE79, 0xFE7B, 0xFE7D, 0xFE7F, 0xFE77, 0xFE8A, 0xFEF0, 0xFEF3, 0xFEF2, 0xFECE, 0xFECF, 0xFED0, 0xFEF6, 0xFEF8, 0xFEFA, 0xFEFC>,
			CharLine<0x00A0, 0xFFFD, 0xFFFD, 0xFFFD, 0x00A4, 0xFFFD, 0xFE8B, 0xFE91, 0xFE97, 0xFE9B, 0xFE9F, 0xFEA3, 0x060C, 0x00AD, 0xFEA7, 0xFEB3>,
			CharLine<0x0660, 0x0661, 0x0662, 0x0663, 0x0664, 0x0665, 0x0666, 0x0667, 0x0668, 0x0669, 0xFEB7, 0x061B, 0xFEBB, 0xFEBF, 0xFECA, 0x061F>,
			CharLine<0xFECB, 0x0621, 0x0622, 0x0623, 0x0624, 0x0625, 0x0626, 0x0627, 0x0628, 0x0629, 0x062A, 0x062B, 0x062C, 0x062D, 0x062E, 0x062F>,
			CharLine<0x0630, 0x0631, 0x0632, 0x0633, 0x0634, 0x0635, 0x0636, 0x0637, 0x0638, 0x0639, 0x063A, 0xFECC, 0xFE82, 0xFE84, 0xFE8E, 0xFED3>,
			CharLine<0x0640, 0x0641, 0x0642, 0x0643, 0x0644, 0x0645, 0x0646, 0x0647, 0x0648, 0x0649, 0x064A, 0x064B, 0x064C, 0x064D, 0x064E, 0x064F>,
			CharLine<0x0650, 0x0651, 0x0652, 0xFED7, 0xFEDB, 0xFEDF, 0x200B, 0xFEF5, 0xFEF7, 0xFEF9, 0xFEFB, 0xFEE3, 0xFEE7, 0xFEEC, 0xFEE9, 0xFFFD>
		>
	> IBM1046("IBM1046", MIB_OTHER, "Arabic (IBM1046)", "ibm-1046|ibm-1046_X110-1999", 0x1A);
	sbcs::SingleByteEncoderFactory<
		sbcs::IBMPCCompatibleCharWire<
			CharLine<0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD>,
			CharLine<0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD>,
			CharLine<0x00A0, 0x00AD, 0xFFFD, 0x00A3, 0x00A4, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0x060C, 0xFFFD, 0xFFFD, 0xFFFD>,
			CharLine<0x0660, 0x0661, 0x0662, 0x0663, 0x0664, 0x0665, 0x0666, 0x0667, 0x0668, 0x0669, 0xFFFD, 0x061B, 0xFFFD, 0xFFFD, 0xFFFD, 0x061F>,
			CharLine<0x00A2, 0x0621, 0x0622, 0x0623, 0x0624, 0xFFFD, 0x0626, 0x0627, 0x0628, 0x0629, 0x062A, 0x062B, 0x062C, 0x062D, 0x062E, 0x062F>,
			CharLine<0x0630, 0x0631, 0x0632, 0x0633, 0x0634, 0x0635, 0x0636, 0x0637, 0x0638, 0x0639, 0x063A, 0x00A6, 0x00AC, 0x00F7, 0x00D7, 0xFFFD>,
			CharLine<0x0640, 0x0641, 0x0642, 0x0643, 0x0644, 0x0645, 0x0646, 0x0647, 0x0648, 0x0649, 0x064A, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD>,
			CharLine<0xFFFD, 0x0651, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD>
		>
	> IBM9056("IBM9056", MIB_OTHER, "Arabic (IBM9056)", "ibm-9056|ibm-9056_P100-1995", 0x7F);
	sbcs::SingleByteEncoderFactory<
		sbcs::ASCIICompatibleCharWire<
			CharLine<0xFE88, 0x00D7, 0x00F7, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFE71, 0x0088, 0x25A0, 0x2502, 0x2500, 0x2510, 0x250C, 0x2514, 0x2518>,
			CharLine<0xFE79, 0xFE7B, 0xFE7D, 0xFE7F, 0xFE77, 0xFE8A, 0xFEF0, 0xFEF3, 0xFEF2, 0xFECE, 0xFECF, 0xFED0, 0xFEF6, 0xFEF8, 0xFEFA, 0xFEFC>,
			CharLine<0x00A0, 0xFFFD, 0xFFFD, 0xFFFD, 0x00A4, 0xFFFD, 0xFE8B, 0xFE91, 0xFE97, 0xFE9B, 0xFE9F, 0xFEA3, 0x060C, 0x00AD, 0xFEA7, 0xFEB3>,
			CharLine<0x0660, 0x0661, 0x0662, 0x0663, 0x0664, 0x0665, 0x0666, 0x0667, 0x0668, 0x0669, 0xFEB7, 0x061B, 0xFEBB, 0xFEBF, 0xFECA, 0x061F>,
			CharLine<0xFECB, 0x0621, 0x0622, 0x0623, 0x0624, 0x0625, 0x0626, 0x0627, 0x0628, 0x0629, 0x062A, 0x062B, 0x062C, 0x062D, 0x062E, 0x062F>,
			CharLine<0x0630, 0x0631, 0x0632, 0x0633, 0x0634, 0x0635, 0x0636, 0x0637, 0x0638, 0x0639, 0x063A, 0xFECC, 0xFE82, 0xFE84, 0xFE8E, 0xFED3>,
			CharLine<0x0640, 0x0641, 0x0642, 0x0643, 0x0644, 0x0645, 0x0646, 0x0647, 0x0648, 0x0649, 0x064A, 0x064B, 0x064C, 0x064D, 0x064E, 0x064F>,
			CharLine<0x0650, 0x0651, 0x0652, 0xFED7, 0xFEDB, 0xFEDF, 0x200B, 0xFEF5, 0xFEF7, 0xFEF9, 0xFEFB, 0xFEE3, 0xFEE7, 0xFEEC, 0xFEE9, 0x20AC>
		>
	> IBM9238("IBM9238", MIB_OTHER, "Arabic (IBM9238)", "ibm-9238|ibm-9238_X110-1999", 0x1A);
#endif // !ASCENSION_NO_MINORITY_ENCODINGS

	struct Installer {
		Installer() {
#ifndef ASCENSION_NO_STANDARD_ENCODINGS
			Encoder::registerFactory(ISO_8859_6);
#endif // !ASCENSION_NO_STANDARD_ENCODINGS
#ifndef ASCENSION_NO_PROPRIETARY_ENCODINGS
			Encoder::registerFactory(IBM864);
			Encoder::registerFactory(IBM16804);
			Encoder::registerFactory(WINDOWS_1256);
#endif // !ASCENSION_NO_PROPRIETARY_ENCODINGS
#ifndef ASCENSION_NO_MINORITY_ENCODINGS
			Encoder::registerFactory(IBM425);
			Encoder::registerFactory(IBM1046);
			Encoder::registerFactory(IBM9056);
			Encoder::registerFactory(IBM9238);
#endif // !ASCENSION_NO_MINORITY_ENCODINGS
		}
	} installer;
}
