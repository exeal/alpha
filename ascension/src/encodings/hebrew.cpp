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
 * - MacHebrew (not implemented)
 * @author exeal
 * @date 2007-2010, 2014
 */

#include <ascension/corelib/encoding/encoder.hpp>
#include <ascension/corelib/encoding/encoder-implementation.hpp>

namespace ascension {
	namespace encoding {
		namespace implementation {
			namespace sbcs {
				namespace {
					struct Installer {
						Installer() {
#ifndef ASCENSION_NO_STANDARD_ENCODINGS
							EncoderRegistry::instance().registerFactory(std::make_shared<
								SingleByteEncoderFactory<
									ISO8859CompatibleCharWire<
										CharLine<0x00a0, 0xfffd, 0x00a2, 0x00a3, 0x00a4, 0x00a5, 0x00a6, 0x00a7, 0x00a8, 0x00a9, 0x00d7, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x00af>,
										CharLine<0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x00b4, 0x00b5, 0x00b6, 0x00b7, 0x00b8, 0x00b9, 0x00f7, 0x00bb, 0x00bc, 0x00bd, 0x00be, 0xfffd>,
										CharLine<0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd>,
										CharLine<0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0x2017>,
										CharLine<0x05d0, 0x05d1, 0x05d2, 0x05d3, 0x05d4, 0x05d5, 0x05d6, 0x05d7, 0x05d8, 0x05d9, 0x05da, 0x05db, 0x05dc, 0x05dd, 0x05de, 0x05df>,
										CharLine<0x05e0, 0x05e1, 0x05e2, 0x05e3, 0x05e4, 0x05e5, 0x05e6, 0x05e7, 0x05e8, 0x05e9, 0x05ea, 0xfffd, 0xfffd, 0x200e, 0x200f, 0xfffd>
									>
								>
							>("ISO-8859-8", standard::ISO_8859_8, "Hebrew (ISO 8859-8)",
								"iso-ir-138|ISO_8859-8|hebrew|csISOLatinHebrew" "\0ibm-5012|ISO-8859-8-I|ISO-8859-8-E|8859_8|windows-28598|ibm-5012_P100-1999", 0x1a));
#endif // !ASCENSION_NO_STANDARD_ENCODINGS

#ifndef ASCENSION_NO_PROPRIETARY_ENCODINGS
							EncoderRegistry::instance().registerFactory(std::make_shared<
								SingleByteEncoderFactory<
									IBMPCCompatibleCharWire<
										CharLine<0x05d0, 0x05d1, 0x05d2, 0x05d3, 0x05d4, 0x05d5, 0x05d6, 0x05d7, 0x05d8, 0x05d9, 0x05da, 0x05db, 0x05dc, 0x05dd, 0x05de, 0x05df>,
										CharLine<0x05e0, 0x05e1, 0x05e2, 0x05e3, 0x05e4, 0x05e5, 0x05e6, 0x05e7, 0x05e8, 0x05e9, 0x05ea, 0x00a2, 0x00a3, 0x00a5, 0xfffd, 0x20aa>,
										CharLine<0x200e, 0x200f, 0x202a, 0x202b, 0x202d, 0x202e, 0x202c, 0xfffd, 0xfffd, 0x2310, 0x00ac, 0x00bd, 0x00bc, 0x20ac, 0x00ab, 0x00bb>,
										CharLine<0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556, 0x2555, 0x2563, 0x2551, 0x2557, 0x255d, 0x255c, 0x255b, 0x2510>,
										CharLine<0x2514, 0x2534, 0x252c, 0x251c, 0x2500, 0x253c, 0x255e, 0x255f, 0x255a, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256c, 0x2567>,
										CharLine<0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256b, 0x256a, 0x2518, 0x250c, 0x2588, 0x2584, 0x258c, 0x2590, 0x2580>,
										CharLine<0x03b1, 0x00df, 0x0393, 0x03c0, 0x03a3, 0x03c3, 0x03bc, 0x03c4, 0x03a6, 0x0398, 0x03a9, 0x03b4, 0x221e, 0x03c6, 0x03b5, 0x2229>,
										CharLine<0x2261, 0x00b1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00f7, 0x2248, 0x00b0, 0x2219, 0x00b7, 0x221a, 0x207f, 0x00b2, 0x25a0, 0x00a0>
									>
								>
							>("IBM867", MIB_OTHER, "Hebrew (IBM867)", "\0ibm-867|ibm-867_P100-1998", 0x7f));

							EncoderRegistry::instance().registerFactory(std::make_shared<
								SingleByteEncoderFactory<
									CharWire<
										CharLine<0x0000, 0x0001, 0x0002, 0x0003, 0x009c, 0x0009, 0x0086, 0x007f, 0x0097, 0x008d, 0x008e, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f>,
										CharLine<0x0010, 0x0011, 0x0012, 0x0013, 0x009d, 0x0085, 0x0008, 0x0087, 0x0018, 0x0019, 0x0092, 0x008f, 0x001c, 0x001d, 0x001e, 0x001f>,
										CharLine<0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x000a, 0x0017, 0x001b, 0x0088, 0x0089, 0x008a, 0x008b, 0x008c, 0x0005, 0x0006, 0x0007>,
										CharLine<0x0090, 0x0091, 0x0016, 0x0093, 0x0094, 0x0095, 0x0096, 0x0004, 0x0098, 0x0099, 0x009a, 0x009b, 0x0014, 0x0015, 0x009e, 0x001a>,
										CharLine<0x0020, 0x05d0, 0x05d1, 0x05d2, 0x05d3, 0x05d4, 0x05d5, 0x05d6, 0x05d7, 0x05d8, 0x00a2, 0x002e, 0x003c, 0x0028, 0x002b, 0x007c>,
										CharLine<0x0026, 0x05d9, 0x05da, 0x05db, 0x05dc, 0x05dd, 0x05de, 0x05df, 0x05e0, 0x05e1, 0x0021, 0x0024, 0x002a, 0x0029, 0x003b, 0x00ac>,
										CharLine<0x002d, 0x002f, 0x05e2, 0x05e3, 0x05e4, 0x05e5, 0x05e6, 0x05e7, 0x05e8, 0x05e9, 0x00a6, 0x002c, 0x0025, 0x005f, 0x003e, 0x003f>,
										CharLine<0xfffd, 0x05ea, 0xfffd, 0xfffd, 0x00a0, 0xfffd, 0xfffd, 0xfffd, 0x2017, 0x0060, 0x003a, 0x0023, 0x0040, 0x0027, 0x003d, 0x0022>,
										CharLine<0xfffd, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x00ab, 0x00bb, 0xfffd, 0xfffd, 0xfffd, 0x00b1>,
										CharLine<0x00b0, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f, 0x0070, 0x0071, 0x0072, 0xfffd, 0xfffd, 0x20ac, 0x00b8, 0x20aa, 0x00a4>,
										CharLine<0x00b5, 0x007e, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007a, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0x00ae>,
										CharLine<0x005e, 0x00a3, 0x00a5, 0x2022, 0x00a9, 0x00a7, 0x00b6, 0x00bc, 0x00bd, 0x00be, 0x005b, 0x005d, 0x203e, 0x00a8, 0x00b4, 0x00d7>,
										CharLine<0x007b, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x00ad, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd>,
										CharLine<0x007d, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f, 0x0050, 0x0051, 0x0052, 0x00b9, 0x202d, 0x202e, 0x202c, 0xfffd, 0xfffd>,
										CharLine<0x005c, 0x00f7, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005a, 0x00b2, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd>,
										CharLine<0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x00b3, 0x202a, 0x202b, 0x200e, 0x200f, 0x009f>
									>
								>
							>("IBM12712", MIB_OTHER, "Hebrew (IBM12712)", "\0ibm-12712|ibm-12712_P100-1998", 0x3f));

							EncoderRegistry::instance().registerFactory(std::make_shared<
								SingleByteEncoderFactory<
									ASCIICompatibleCharWire<
										CharLine<0x20ac, 0x0081, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021, 0x02c6, 0x2030, 0x008a, 0x2039, 0x008c, 0x008d, 0x008e, 0x008f>,
										CharLine<0x0090, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014, 0x02dc, 0x2122, 0x009a, 0x203a, 0x009c, 0x009d, 0x009e, 0x009f>,
										CharLine<0x00a0, 0x00a1, 0x00a2, 0x00a3, 0x20aa, 0x00a5, 0x00a6, 0x00a7, 0x00a8, 0x00a9, 0x00d7, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x00af>,
										CharLine<0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x00b4, 0x00b5, 0x00b6, 0x00b7, 0x00b8, 0x00b9, 0x00f7, 0x00bb, 0x00bc, 0x00bd, 0x00be, 0x00bf>,
										CharLine<0x05b0, 0x05b1, 0x05b2, 0x05b3, 0x05b4, 0x05b5, 0x05b6, 0x05b7, 0x05b8, 0x05b9, 0x05ba, 0x05bb, 0x05bc, 0x05bd, 0x05be, 0x05bf>,
										CharLine<0x05c0, 0x05c1, 0x05c2, 0x05c3, 0x05f0, 0x05f1, 0x05f2, 0x05f3, 0x05f4, 0xf88d, 0xf88e, 0xf88f, 0xf890, 0xf891, 0xf892, 0xf893>,
										CharLine<0x05d0, 0x05d1, 0x05d2, 0x05d3, 0x05d4, 0x05d5, 0x05d6, 0x05d7, 0x05d8, 0x05d9, 0x05da, 0x05db, 0x05dc, 0x05dd, 0x05de, 0x05df>,
										CharLine<0x05e0, 0x05e1, 0x05e2, 0x05e3, 0x05e4, 0x05e5, 0x05e6, 0x05e7, 0x05e8, 0x05e9, 0x05ea, 0xf894, 0xf895, 0x200e, 0x200f, 0xf896>
									>
								>
							>("windows-1255", proprietary::WINDOWS_1255, "Hebrew (windows-1255)", "\0ibm-9447|cp1255|ibm-9447_P100-2002", 0x3f));
#endif // !ASCENSION_NO_PROPRIETARY_ENCODINGS

#ifndef ASCENSION_NO_MINORITY_ENCODINGS
							EncoderRegistry::instance().registerFactory(std::make_shared<
								SingleByteEncoderFactory<
									CharWire<
										CharLine<0x0000, 0x0001, 0x0002, 0x0003, 0x009c, 0x0009, 0x0086, 0x007f, 0x0097, 0x008d, 0x008e, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f>,
										CharLine<0x0010, 0x0011, 0x0012, 0x0013, 0x009d, 0x0085, 0x0008, 0x0087, 0x0018, 0x0019, 0x0092, 0x008f, 0x001c, 0x001d, 0x001e, 0x001f>,
										CharLine<0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x000a, 0x0017, 0x001b, 0x0088, 0x0089, 0x008a, 0x008b, 0x008c, 0x0005, 0x0006, 0x0007>,
										CharLine<0x0090, 0x0091, 0x0016, 0x0093, 0x0094, 0x0095, 0x0096, 0x0004, 0x0098, 0x0099, 0x009a, 0x009b, 0x0014, 0x0015, 0x009e, 0x001a>,
										CharLine<0x0020, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0x0024, 0x002e, 0x003c, 0x0028, 0x002b, 0x007c>,
										CharLine<0x05d0, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0x0021, 0x00a2, 0x002a, 0x0029, 0x003b, 0x00ac>,
										CharLine<0x002d, 0x002f, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0x002c, 0x0025, 0x005f, 0x003e, 0x003f>,
										CharLine<0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0x003a, 0x0023, 0x0040, 0x0027, 0x003d, 0x0022>,
										CharLine<0xfffd, 0x05d1, 0x05d2, 0x05d3, 0x05d4, 0x05d5, 0x05d6, 0x05d7, 0x05d8, 0x05d9, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd>,
										CharLine<0xfffd, 0x05da, 0x05db, 0x05dc, 0x05dd, 0x05de, 0x05df, 0x05e0, 0x05e1, 0x05e2, 0xfffd, 0xfffd, 0x20ac, 0xfffd, 0x20aa, 0xfffd>,
										CharLine<0xfffd, 0xfffd, 0x05e3, 0x05e4, 0x05e5, 0x05e6, 0x05e7, 0x05e8, 0x05e9, 0x05ea, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd>,
										CharLine<0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd>,
										CharLine<0xfffd, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd>,
										CharLine<0xfffd, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f, 0x0050, 0x0051, 0x0052, 0xfffd, 0x202d, 0x202e, 0x202c, 0xfffd, 0xfffd>,
										CharLine<0xfffd, 0xfffd, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005a, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd>,
										CharLine<0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0xfffd, 0x202a, 0x202b, 0x200e, 0x200f, 0x009f>
									>
								>
							>("IBM4899", MIB_OTHER, "Hebrew (IBM4899)", "\0ibm-4899|ibm-4899_P100-1998", 0x3f));

							EncoderRegistry::instance().registerFactory(std::make_shared<
								SingleByteEncoderFactory<
									IBMPCCompatibleCharWire<
										CharLine<0x05d0, 0x05d1, 0x05d2, 0x05d3, 0x05d4, 0x05d5, 0x05d6, 0x05d7, 0x05d8, 0x05d9, 0x05da, 0x05db, 0x05dc, 0x05dd, 0x05de, 0x05df>,
										CharLine<0x05e0, 0x05e1, 0x05e2, 0x05e3, 0x05e4, 0x05e5, 0x05e6, 0x05e7, 0x05e8, 0x05e9, 0x05ea, 0xfffd, 0x00a3, 0xfffd, 0x00d7, 0xfffd>,
										CharLine<0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0x00ae, 0x00ac, 0x00bd, 0x00bc, 0xfffd, 0x00ab, 0x00bb>,
										CharLine<0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0x00a9, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0x00a2, 0x00a5, 0xfffd>,
										CharLine<0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0x00a4>,
										CharLine<0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0x00a6, 0xfffd, 0xfffd>,
										CharLine<0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0x00b5, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0x203e, 0x00b4>,
										CharLine<0x00ad, 0x00b1, 0x2017, 0x00be, 0x00b6, 0x00a7, 0x00f7, 0x00b8, 0x00b0, 0x00a8, 0x2022, 0x00b9, 0x00b3, 0x00b2, 0xfffd, 0x00a0>
									>
								>
							>("IBM4952", MIB_OTHER, "Hebrew (IBM4952)", "\0ibm-4952|ibm-4952_P100-1995", 0x7f));

							EncoderRegistry::instance().registerFactory(std::make_shared<
								SingleByteEncoderFactory<
									IBMPCCompatibleCharWire<
										CharLine<0x05d0, 0x05d1, 0x05d2, 0x05d3, 0x05d4, 0x05d5, 0x05d6, 0x05d7, 0x05d8, 0x05d9, 0x05da, 0x05db, 0x05dc, 0x05dd, 0x05de, 0x05df>,
										CharLine<0x05e0, 0x05e1, 0x05e2, 0x05e3, 0x05e4, 0x05e5, 0x05e6, 0x05e7, 0x05e8, 0x05e9, 0x05ea, 0xfffd, 0x00a3, 0xfffd, 0x00d7, 0x20aa>,
										CharLine<0x200e, 0x200f, 0x202a, 0x202b, 0x202d, 0x202e, 0x202c, 0xfffd, 0xfffd, 0x00ae, 0x00ac, 0x00bd, 0x00bc, 0x20ac, 0x00ab, 0x00bb>,
										CharLine<0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0xfffd, 0xfffd, 0xfffd, 0x00a9, 0x2563, 0x2551, 0x2557, 0x255d, 0x00a2, 0x00a5, 0x2510>,
										CharLine<0x2514, 0x2534, 0x252c, 0x251c, 0x2500, 0x253c, 0xfffd, 0xfffd, 0x255a, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256c, 0x00a4>,
										CharLine<0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0x2518, 0x250c, 0x2588, 0x2584, 0x00a6, 0xfffd, 0x2580>,
										CharLine<0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0x00b5, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0x203e, 0x00b4>,
										CharLine<0x00ad, 0x00b1, 0x2017, 0x00be, 0x00b6, 0x00a7, 0x00f7, 0x00b8, 0x00b0, 0x00a8, 0x2022, 0x00b9, 0x00b3, 0x00b2, 0x25a0, 0x00a0>
									>
								>
							>("IBM9048", MIB_OTHER, "Hebrew (IBM9048)", "\0ibm-9048|ibm-9048_P100-1998", 0x7f));
#endif // !ASCENSION_NO_MINORITY_ENCODINGS
						}
					} installer;
				}
			}
		}
	}
}
