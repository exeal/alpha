/**
 * @file cyrillic.cpp
 * Implements Cyrillic encodings. This includes:
 * - ISO 8859-5:1999
 * - KOI8-R
 * - KOI8-U
 * - IBM808
 * - IBM848
 * - IBM849
 * - IBM872
 * - IBM1154
 * - IBM1158
 * - windows-1251
 * - MacCyrillic
 * - MacUkrainian
 * @author exeal
 * @date 2008-2009
 */

#include <ascension/encoder.hpp>
using namespace ascension::encoding;
using namespace ascension::encoding::implementation;
using namespace std;

namespace {
#ifndef ASCENSION_STANDARD_ENCODINGS
	sbcs::SingleByteEncoderFactory<
		sbcs::ISO8859CompatibleCharWire<
			CharLine<0x00A0, 0x0401, 0x0402, 0x0403, 0x0404, 0x0405, 0x0406, 0x0407, 0x0408, 0x0409, 0x040A, 0x040B, 0x040C, 0x00AD, 0x040E, 0x040F>,
			CharLine<0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417, 0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F>,
			CharLine<0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427, 0x0428, 0x0429, 0x042A, 0x042B, 0x042C, 0x042D, 0x042E, 0x042F>,
			CharLine<0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437, 0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E, 0x043F>,
			CharLine<0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447, 0x0448, 0x0449, 0x044A, 0x044B, 0x044C, 0x044D, 0x044E, 0x044F>,
			CharLine<0x2116, 0x0451, 0x0452, 0x0453, 0x0454, 0x0455, 0x0456, 0x0457, 0x0458, 0x0459, 0x045A, 0x045B, 0x045C, 0x00A7, 0x045E, 0x045F>
		>
	> ISO_8859_5("ISO-8859-5", standard::ISO_8859_8, "Cyrillic (ISO 8859-5)",
		"iso-ir-144|ISO_8859-5|cyrillic|csISOLatinCyrillic" "\0ibm-915|8859_5|cp915|915|windows-28595|ibm-915_P100-1995", 0x1A);
	sbcs::SingleByteEncoderFactory<
		sbcs::ASCIICompatibleCharWire<
			CharLine<0x2500, 0x2502, 0x250C, 0x2510, 0x2514, 0x2518, 0x251C, 0x2524, 0x252C, 0x2534, 0x253C, 0x2580, 0x2584, 0x2588, 0x258C, 0x2590>,
			CharLine<0x2591, 0x2592, 0x2593, 0x2320, 0x25A0, 0x2219, 0x221A, 0x2248, 0x2264, 0x2265, 0x00A0, 0x2321, 0x00B0, 0x00B2, 0x00B7, 0x00F7>,
			CharLine<0x2550, 0x2551, 0x2552, 0x0451, 0x2553, 0x2554, 0x2555, 0x2556, 0x2557, 0x2558, 0x2559, 0x255A, 0x255B, 0x255C, 0x255D, 0x255E>,
			CharLine<0x255F, 0x2560, 0x2561, 0x0401, 0x2562, 0x2563, 0x2564, 0x2565, 0x2566, 0x2567, 0x2568, 0x2569, 0x256A, 0x256B, 0x256C, 0x00A9>,
			CharLine<0x044E, 0x0430, 0x0431, 0x0446, 0x0434, 0x0435, 0x0444, 0x0433, 0x0445, 0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E>,
			CharLine<0x043F, 0x044F, 0x0440, 0x0441, 0x0442, 0x0443, 0x0436, 0x0432, 0x044C, 0x044B, 0x0437, 0x0448, 0x044D, 0x0449, 0x0447, 0x044A>,
			CharLine<0x042E, 0x0410, 0x0411, 0x0426, 0x0414, 0x0415, 0x0424, 0x0413, 0x0425, 0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E>,
			CharLine<0x041F, 0x042F, 0x0420, 0x0421, 0x0422, 0x0423, 0x0416, 0x0412, 0x042C, 0x042B, 0x0417, 0x0428, 0x042D, 0x0429, 0x0427, 0x042A>
		>
	> KOI8_R("KOI8-R", standard::KOI8_R, "Cyrillic (KOI8-R)", "csKOI8R" "\0ibm-878|koi8|windows-20866|cp878|ibm-878_P100-1996", 0x1A);
	sbcs::SingleByteEncoderFactory<
		sbcs::ASCIICompatibleCharWire<
			CharLine<0x2500, 0x2502, 0x250C, 0x2510, 0x2514, 0x2518, 0x251C, 0x2524, 0x252C, 0x2534, 0x253C, 0x2580, 0x2584, 0x2588, 0x258C, 0x2590>,
			CharLine<0x2591, 0x2592, 0x2593, 0x2320, 0x25A0, 0x2219, 0x221A, 0x2248, 0x2264, 0x2265, 0x00A0, 0x2321, 0x00B0, 0x00B2, 0x00B7, 0x00F7>,
			CharLine<0x2550, 0x2551, 0x2552, 0x0451, 0x0454, 0x2554, 0x0456, 0x0457, 0x2557, 0x2558, 0x2559, 0x255A, 0x255B, 0x0491, 0x045E, 0x255E>,
			CharLine<0x255F, 0x2560, 0x2561, 0x0401, 0x0404, 0x2563, 0x0406, 0x0407, 0x2566, 0x2567, 0x2568, 0x2569, 0x256A, 0x0490, 0x040E, 0x00A9>,
			CharLine<0x044E, 0x0430, 0x0431, 0x0446, 0x0434, 0x0435, 0x0444, 0x0433, 0x0445, 0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E>,
			CharLine<0x043F, 0x044F, 0x0440, 0x0441, 0x0442, 0x0443, 0x0436, 0x0432, 0x044C, 0x044B, 0x0437, 0x0448, 0x044D, 0x0449, 0x0447, 0x044A>,
			CharLine<0x042E, 0x0410, 0x0411, 0x0426, 0x0414, 0x0415, 0x0424, 0x0413, 0x0425, 0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E>,
			CharLine<0x041F, 0x042F, 0x0420, 0x0421, 0x0422, 0x0423, 0x0416, 0x0412, 0x042C, 0x042B, 0x0417, 0x0428, 0x042D, 0x0429, 0x0427, 0x042A>
		>
	> KOI8_U("KOI8-U", standard::KOI8_U, "Cyrillic/Ukrainian (KOI8-U)", "\0ibm-1168|windows-21866|ibm-1168_P100-2002", 0x1A);
#endif // !ASCENSION_STANDARD_ENCODINGS
#ifndef ASCENSION_PROPRIETARY_ENCODINGS
	sbcs::SingleByteEncoderFactory<
		sbcs::IBMPCCompatibleCharWire<
			CharLine<0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417, 0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F>,
			CharLine<0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427, 0x0428, 0x0429, 0x042A, 0x042B, 0x042C, 0x042D, 0x042E, 0x042F>,
			CharLine<0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437, 0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E, 0x043F>,
			CharLine<0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556, 0x2555, 0x2563, 0x2551, 0x2557, 0x255D, 0x255C, 0x255B, 0x2510>,
			CharLine<0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x255E, 0x255F, 0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2567>,
			CharLine<0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256B, 0x256A, 0x2518, 0x250C, 0x2588, 0x2584, 0x258C, 0x2590, 0x2580>,
			CharLine<0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447, 0x0448, 0x0449, 0x044A, 0x044B, 0x044C, 0x044D, 0x044E, 0x044F>,
			CharLine<0x0401, 0x0451, 0x0404, 0x0454, 0x0407, 0x0457, 0x040E, 0x045E, 0x00B0, 0x2219, 0x00B7, 0x221A, 0x2116, 0x20AC, 0x25A0, 0x00A0>
		>
	> IBM808("IBM808", MIB_OTHER, "Cyrillic/Russian (IBM808 (IBM866 + Euro))", "\0ibm-808|ibm-808_P100-1999", 0x7F);
	sbcs::SingleByteEncoderFactory<
		sbcs::IBMPCCompatibleCharWire<
			CharLine<0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417, 0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F>,
			CharLine<0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427, 0x0428, 0x0429, 0x042A, 0x042B, 0x042C, 0x042D, 0x042E, 0x042F>,
			CharLine<0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437, 0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E, 0x043F>,
			CharLine<0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556, 0x2555, 0x2563, 0x2551, 0x2557, 0x255D, 0x255C, 0x255B, 0x2510>,
			CharLine<0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x255E, 0x255F, 0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2567>,
			CharLine<0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256B, 0x256A, 0x2518, 0x250C, 0x2588, 0x2584, 0x258C, 0x2590, 0x2580>,
			CharLine<0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447, 0x0448, 0x0449, 0x044A, 0x044B, 0x044C, 0x044D, 0x044E, 0x044F>,
			CharLine<0x0401, 0x0451, 0x0490, 0x0491, 0x0404, 0x0454, 0x0406, 0x0456, 0x0407, 0x0457, 0x00F7, 0x00B1, 0x2116, 0x20AC, 0x25A0, 0x00A0>
		>
	> IBM848("IBM848", MIB_OTHER, "Cyrillic/Ukraine (IBM848 (IBM1125 + Euro))", "\0ibm-848|ibm-848_P100-1999", 0x7F);
	sbcs::SingleByteEncoderFactory<
		sbcs::IBMPCCompatibleCharWire<
			CharLine<0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417, 0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F>,
			CharLine<0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427, 0x0428, 0x0429, 0x042A, 0x042B, 0x042C, 0x042D, 0x042E, 0x042F>,
			CharLine<0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437, 0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E, 0x043F>,
			CharLine<0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556, 0x2555, 0x2563, 0x2551, 0x2557, 0x255D, 0x255C, 0x255B, 0x2510>,
			CharLine<0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x255E, 0x255F, 0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2567>,
			CharLine<0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256B, 0x256A, 0x2518, 0x250C, 0x2588, 0x2584, 0x258C, 0x2590, 0x2580>,
			CharLine<0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447, 0x0448, 0x0449, 0x044A, 0x044B, 0x044C, 0x044D, 0x044E, 0x044F>,
			CharLine<0x0401, 0x0451, 0x0404, 0x0454, 0x0407, 0x0457, 0x040E, 0x045E, 0x0406, 0x0456, 0x00B7, 0x20AC, 0x0490, 0x0491, 0x2219, 0x00A0>
		>
	> IBM849("IBM849", MIB_OTHER, "Cyrillic/Belarus (IBM849 (IBM1131 + Euro))", "\0ibm-849|ibm-849_P100-1999", 0x7F);
	sbcs::SingleByteEncoderFactory<
		sbcs::IBMPCCompatibleCharWire<
			CharLine<0x0452, 0x0402, 0x0453, 0x0403, 0x0451, 0x0401, 0x0454, 0x0404, 0x0455, 0x0405, 0x0456, 0x0406, 0x0457, 0x0407, 0x0458, 0x0408>,
			CharLine<0x0459, 0x0409, 0x045A, 0x040A, 0x045B, 0x040B, 0x045C, 0x040C, 0x045E, 0x040E, 0x045F, 0x040F, 0x044E, 0x042E, 0x044A, 0x042A>,
			CharLine<0x0430, 0x0410, 0x0431, 0x0411, 0x0446, 0x0426, 0x0434, 0x0414, 0x0435, 0x0415, 0x0444, 0x0424, 0x0433, 0x0413, 0x00AB, 0x00BB>,
			CharLine<0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x0445, 0x0425, 0x0438, 0x0418, 0x2563, 0x2551, 0x2557, 0x255D, 0x0439, 0x0419, 0x2510>,
			CharLine<0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x043A, 0x041A, 0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x20AC>,
			CharLine<0x043B, 0x041B, 0x043C, 0x041C, 0x043D, 0x041D, 0x043E, 0x041E, 0x043F, 0x2518, 0x250C, 0x2588, 0x2584, 0x041F, 0x044F, 0x2580>,
			CharLine<0x042F, 0x0440, 0x0420, 0x0441, 0x0421, 0x0442, 0x0422, 0x0443, 0x0423, 0x0436, 0x0416, 0x0432, 0x0412, 0x044C, 0x042C, 0x2116>,
			CharLine<0x00AD, 0x044B, 0x042B, 0x0437, 0x0417, 0x0448, 0x0428, 0x044D, 0x042D, 0x0449, 0x0429, 0x0447, 0x0427, 0x00A7, 0x25A0, 0x00A0>
		>
	> IBM872("IBM872", MIB_OTHER, "Cyrillic (IBM872 (IBM855 + Euro))", "\0ibm-872|ibm-872_P100-1999", 0x7F);
	sbcs::SingleByteEncoderFactory<
		CharWire<
			CharLine<0x0000, 0x0001, 0x0002, 0x0003, 0x009C, 0x0009, 0x0086, 0x007F, 0x0097, 0x008D, 0x008E, 0x000B, 0x000C, 0x000D, 0x000E, 0x000F>,
			CharLine<0x0010, 0x0011, 0x0012, 0x0013, 0x009D, 0x0085, 0x0008, 0x0087, 0x0018, 0x0019, 0x0092, 0x008F, 0x001C, 0x001D, 0x001E, 0x001F>,
			CharLine<0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x000A, 0x0017, 0x001B, 0x0088, 0x0089, 0x008A, 0x008B, 0x008C, 0x0005, 0x0006, 0x0007>,
			CharLine<0x0090, 0x0091, 0x0016, 0x0093, 0x0094, 0x0095, 0x0096, 0x0004, 0x0098, 0x0099, 0x009A, 0x009B, 0x0014, 0x0015, 0x009E, 0x001A>,
			CharLine<0x0020, 0x00A0, 0x0452, 0x0453, 0x0451, 0x0454, 0x0455, 0x0456, 0x0457, 0x0458, 0x005B, 0x002E, 0x003C, 0x0028, 0x002B, 0x0021>,
			CharLine<0x0026, 0x0459, 0x045A, 0x045B, 0x045C, 0x045E, 0x045F, 0x042A, 0x2116, 0x0402, 0x005D, 0x0024, 0x002A, 0x0029, 0x003B, 0x005E>,
			CharLine<0x002D, 0x002F, 0x0403, 0x0401, 0x0404, 0x0405, 0x0406, 0x0407, 0x0408, 0x0409, 0x007C, 0x002C, 0x0025, 0x005F, 0x003E, 0x003F>,
			CharLine<0x040A, 0x040B, 0x040C, 0x00AD, 0x040E, 0x040F, 0x044E, 0x0430, 0x0431, 0x0060, 0x003A, 0x0023, 0x0040, 0x0027, 0x003D, 0x0022>,
			CharLine<0x0446, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x0434, 0x0435, 0x0444, 0x0433, 0x0445, 0x0438>,
			CharLine<0x0439, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F, 0x0070, 0x0071, 0x0072, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E, 0x043F>,
			CharLine<0x044F, 0x007E, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007A, 0x0440, 0x0441, 0x0442, 0x0443, 0x0436, 0x0432>,
			CharLine<0x044C, 0x044B, 0x0437, 0x0448, 0x044D, 0x0449, 0x0447, 0x044A, 0x042E, 0x0410, 0x0411, 0x0426, 0x0414, 0x0415, 0x0424, 0x0413>,
			CharLine<0x007B, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x0425, 0x0418, 0x0419, 0x041A, 0x041B, 0x041C>,
			CharLine<0x007D, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F, 0x0050, 0x0051, 0x0052, 0x041D, 0x041E, 0x041F, 0x042F, 0x0420, 0x0421>,
			CharLine<0x005C, 0x20AC, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005A, 0x0422, 0x0423, 0x0416, 0x0412, 0x042C, 0x042B>,
			CharLine<0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x0417, 0x0428, 0x042D, 0x0429, 0x0427, 0x009F>
		>
	> IBM1154("IBM1154", MIB_OTHER, "Cyrillic/Multilingual (IBM1154)", "\0ibm-1154|ibm-1154_P100-1999", 0x3F);
	sbcs::SingleByteEncoderFactory<
		CharWire<
			CharLine<0x0000, 0x0001, 0x0002, 0x0003, 0x009C, 0x0009, 0x0086, 0x007F, 0x0097, 0x008D, 0x008E, 0x000B, 0x000C, 0x000D, 0x000E, 0x000F>,
			CharLine<0x0010, 0x0011, 0x0012, 0x0013, 0x009D, 0x0085, 0x0008, 0x0087, 0x0018, 0x0019, 0x0092, 0x008F, 0x001C, 0x001D, 0x001E, 0x001F>,
			CharLine<0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x000A, 0x0017, 0x001B, 0x0088, 0x0089, 0x008A, 0x008B, 0x008C, 0x0005, 0x0006, 0x0007>,
			CharLine<0x0090, 0x0091, 0x0016, 0x0093, 0x0094, 0x0095, 0x0096, 0x0004, 0x0098, 0x0099, 0x009A, 0x009B, 0x0014, 0x0015, 0x009E, 0x001A>,
			CharLine<0x0020, 0x00A0, 0x0452, 0x0491, 0x0451, 0x0454, 0x0455, 0x0456, 0x0457, 0x0458, 0x005B, 0x002E, 0x003C, 0x0028, 0x002B, 0x0021>,
			CharLine<0x0026, 0x0459, 0x045A, 0x045B, 0x045C, 0x045E, 0x045F, 0x042A, 0x2116, 0x0402, 0x005D, 0x0024, 0x002A, 0x0029, 0x003B, 0x005E>,
			CharLine<0x002D, 0x002F, 0x0490, 0x0401, 0x0404, 0x0405, 0x0406, 0x0407, 0x0408, 0x0409, 0x007C, 0x002C, 0x0025, 0x005F, 0x003E, 0x003F>,
			CharLine<0x040A, 0x040B, 0x040C, 0x00AD, 0x040E, 0x040F, 0x044E, 0x0430, 0x0431, 0x0060, 0x003A, 0x0023, 0x0040, 0x0027, 0x003D, 0x0022>,
			CharLine<0x0446, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x0434, 0x0435, 0x0444, 0x0433, 0x0445, 0x0438>,
			CharLine<0x0439, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F, 0x0070, 0x0071, 0x0072, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E, 0x043F>,
			CharLine<0x044F, 0x007E, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007A, 0x0440, 0x0441, 0x0442, 0x0443, 0x0436, 0x0432>,
			CharLine<0x044C, 0x044B, 0x0437, 0x0448, 0x044D, 0x0449, 0x0447, 0x044A, 0x042E, 0x0410, 0x0411, 0x0426, 0x0414, 0x0415, 0x0424, 0x0413>,
			CharLine<0x007B, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x0425, 0x0418, 0x0419, 0x041A, 0x041B, 0x041C>,
			CharLine<0x007D, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F, 0x0050, 0x0051, 0x0052, 0x041D, 0x041E, 0x041F, 0x042F, 0x0420, 0x0421>,
			CharLine<0x005C, 0x20AC, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005A, 0x0422, 0x0423, 0x0416, 0x0412, 0x042C, 0x042B>,
			CharLine<0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x0417, 0x0428, 0x042D, 0x0429, 0x0427, 0x009F>
		>
	> IBM1158("IBM1158", MIB_OTHER, "Cyrillic/Ukraine (EBCDIC (IBM1123 + Euro))", "\0ibm-1158|ibm-1158_P100-1999", 0x3F);
	sbcs::SingleByteEncoderFactory<
		sbcs::ASCIICompatibleCharWire<
			CharLine<0x0402, 0x0403, 0x201A, 0x0453, 0x201E, 0x2026, 0x2020, 0x2021, 0x20AC, 0x2030, 0x0409, 0x2039, 0x040A, 0x040C, 0x040B, 0x040F>,
			CharLine<0x0452, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, 0x0098, 0x2122, 0x0459, 0x203A, 0x045A, 0x045C, 0x045B, 0x045F>,
			CharLine<0x00A0, 0x040E, 0x045E, 0x0408, 0x00A4, 0x0490, 0x00A6, 0x00A7, 0x0401, 0x00A9, 0x0404, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x0407>,
			CharLine<0x00B0, 0x00B1, 0x0406, 0x0456, 0x0491, 0x00B5, 0x00B6, 0x00B7, 0x0451, 0x2116, 0x0454, 0x00BB, 0x0458, 0x0405, 0x0455, 0x0457>,
			CharLine<0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417, 0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F>,
			CharLine<0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427, 0x0428, 0x0429, 0x042A, 0x042B, 0x042C, 0x042D, 0x042E, 0x042F>,
			CharLine<0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437, 0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E, 0x043F>,
			CharLine<0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447, 0x0448, 0x0449, 0x044A, 0x044B, 0x044C, 0x044D, 0x044E, 0x044F>
		>
	> WINDOWS_1251("windows-1251", proprietary::WINDOWS_1251, "Cyrillic (windows-1251)", "\0ibm-5347|cp1251|ibm-5347_P100-1998", 0x3F);
#endif // !ASCENSION_PROPRIETARY_ENCODINGS
#ifndef ASCENSION_MINORITY_ENCODINGS
#endif // !ASCENSION_MINORITY_ENCODINGS

	struct Installer {
		Installer() {
#ifndef ASCENSION_STANDARD_ENCODINGS
			Encoder::registerFactory(ISO_8859_5);
			Encoder::registerFactory(KOI8_R);
			Encoder::registerFactory(KOI8_U);
#endif // !ASCENSION_STANDARD_ENCODINGS */
#ifndef ASCENSION_PROPRIETARY_ENCODINGS
			Encoder::registerFactory(IBM808);
			Encoder::registerFactory(IBM848);
			Encoder::registerFactory(IBM849);
			Encoder::registerFactory(IBM872);
			Encoder::registerFactory(IBM1154);
			Encoder::registerFactory(IBM1158);
			Encoder::registerFactory(WINDOWS_1251);
#endif // !ASCENSION_PROPRIETARY_ENCODINGS
#ifndef ASCENSION_MINORITY_ENCODINGS
#endif // !ASCENSION_MINORITY_ENCODINGS
		}
	} installer;
}