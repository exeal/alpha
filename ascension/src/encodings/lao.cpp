/**
 * @file lao.cpp
 * Implements Lao encodings. This includes:
 * - IBM1132
 * - IBM1133
 * - MuleLao-1
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
#ifndef ASCENSION_NO_PROPRIETARY_ENCODINGS
							EncoderRegistry::instance().registerFactory(std::make_shared<
								SingleByteEncoderFactory<
									CharWire<
										CharLine<0x0000, 0x0001, 0x0002, 0x0003, 0x009c, 0x0009, 0x0086, 0x007f, 0x0097, 0x008d, 0x008e, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f>,
										CharLine<0x0010, 0x0011, 0x0012, 0x0013, 0x009d, 0x0085, 0x0008, 0x0087, 0x0018, 0x0019, 0x0092, 0x008f, 0x001c, 0x001d, 0x001e, 0x001f>,
										CharLine<0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x000a, 0x0017, 0x001b, 0x0088, 0x0089, 0x008a, 0x008b, 0x008c, 0x0005, 0x0006, 0x0007>,
										CharLine<0x0090, 0x0091, 0x0016, 0x0093, 0x0094, 0x0095, 0x0096, 0x0004, 0x0098, 0x0099, 0x009a, 0x009b, 0x0014, 0x0015, 0x009e, 0x001a>,
										CharLine<0x0020, 0x00a0, 0x0e81, 0x0e82, 0x0e84, 0x0e87, 0x0e88, 0x0eaa, 0x0e8a, 0x005b, 0x00a2, 0x002e, 0x003c, 0x0028, 0x002b, 0x007c>,
										CharLine<0x0026, 0xfffd, 0x0e8d, 0x0e94, 0x0e95, 0x0e96, 0x0e97, 0x0e99, 0x0e9a, 0x005d, 0x0021, 0x0024, 0x002a, 0x0029, 0x003b, 0x00ac>,
										CharLine<0x002d, 0x002f, 0x0e9b, 0x0e9c, 0x0e9d, 0x0e9e, 0x0e9f, 0x0ea1, 0x0ea2, 0x005e, 0x00a6, 0x002c, 0x0025, 0x005f, 0x003e, 0x003f>,
										CharLine<0x20ad, 0xfffd, 0x0ea3, 0x0ea5, 0x0ea7, 0x0eab, 0x0ead, 0x0eae, 0xfffd, 0x0060, 0x003a, 0x0023, 0x0040, 0x0027, 0x003d, 0x0022>,
										CharLine<0xfffd, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0xfffd, 0xfffd, 0x0eaf, 0x0eb0, 0x0eb2, 0x0eb3>,
										CharLine<0xfffd, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f, 0x0070, 0x0071, 0x0072, 0x0eb4, 0x0eb5, 0x0eb6, 0x0eb7, 0x0eb8, 0x0eb9>,
										CharLine<0xfffd, 0x007e, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007a, 0x0ebc, 0x0eb1, 0x0ebb, 0x0ebd, 0xfffd, 0xfffd>,
										CharLine<0x0ed0, 0x0ed1, 0x0ed2, 0x0ed3, 0x0ed4, 0x0ed5, 0x0ed6, 0x0ed7, 0x0ed8, 0x0ed9, 0xfffd, 0x0ec0, 0x0ec1, 0x0ec2, 0x0ec3, 0x0ec4>,
										CharLine<0x007b, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0xfffd, 0x0ec8, 0x0ec9, 0x0eca, 0x0ecb, 0x0ecc>,
										CharLine<0x007d, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f, 0x0050, 0x0051, 0x0052, 0x0ecd, 0x0ec6, 0xfffd, 0x0edc, 0x0edd, 0xfffd>,
										CharLine<0x005c, 0xfffd, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005a, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd>,
										CharLine<0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0x009f>
									>
								>
							>("IBM1132", MIB_OTHER, "Lao (EBCDIC)", "\0ibm-1132|ibm-1132_P100-1998", 0x3f));
							
							EncoderRegistry::instance().registerFactory(std::make_shared<
								SingleByteEncoderFactory<
									ISO8859CompatibleCharWire<
										CharLine<0xfffd, 0x0e81, 0x0e82, 0x0e84, 0x0e87, 0x0e88, 0x0eaa, 0x0e8a, 0x0e8d, 0x0e94, 0x0e95, 0x0e96, 0x0e97, 0x0e99, 0x0e9a, 0x0e9b>,
										CharLine<0x0e9c, 0x0e9d, 0x0e9e, 0x0e9f, 0x0ea1, 0x0ea2, 0x0ea3, 0x0ea5, 0x0ea7, 0x0eab, 0x0ead, 0x0eae, 0xfffd, 0xfffd, 0xfffd, 0x0eaf>,
										CharLine<0x0eb0, 0x0eb2, 0x0eb3, 0x0eb4, 0x0eb5, 0x0eb6, 0x0eb7, 0x0eb8, 0x0eb9, 0x0ebc, 0x0eb1, 0x0ebb, 0x0ebd, 0xfffd, 0xfffd, 0xfffd>,
										CharLine<0x0ec0, 0x0ec1, 0x0ec2, 0x0ec3, 0x0ec4, 0x0ec8, 0x0ec9, 0x0eca, 0x0ecb, 0x0ecc, 0x0ecd, 0x0ec6, 0xfffd, 0x0edc, 0x0edd, 0xfffd>,
										CharLine<0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd>,
										CharLine<0x0ed0, 0x0ed1, 0x0ed2, 0x0ed3, 0x0ed4, 0x0ed5, 0x0ed6, 0x0ed7, 0x0ed8, 0x0ed9, 0xfffd, 0xfffd, 0x00a2, 0x00ac, 0x00a6, 0x00a0>
									>
								>
							>("IBM1133", MIB_OTHER, "Lao (IBM1133)", "\0ibm-1133|ibm-1133_P100-1997", 0x1a));
#endif // !ASCENSION_NO_PROPRIETARY_ENCODINGS

#ifndef ASCENSION_NO_MINORITY_ENCODINGS
							EncoderRegistry::instance().registerFactory(std::make_shared<
								SingleByteEncoderFactory<
									ISO8859CompatibleCharWire<
										CharLine<0x00a0, 0x0e81, 0x0e82, 0xfffd, 0x0e84, 0xfffd, 0xfffd, 0x0e87, 0x0e88, 0xfffd, 0x0e8a, 0xfffd, 0xfffd, 0x0e8d, 0xfffd, 0xfffd>,
										CharLine<0xfffd, 0xfffd, 0xfffd, 0xfffd, 0x0e94, 0x0e95, 0x0e96, 0x0e97, 0xfffd, 0x0e99, 0x0e9a, 0x0e9b, 0x0e9c, 0x0e9d, 0x0e9e, 0x0e9f>,
										CharLine<0xfffd, 0x0ea1, 0x0ea2, 0x0ea3, 0xfffd, 0x0ea5, 0xfffd, 0x0ea7, 0xfffd, 0xfffd, 0x0eaa, 0x0eab, 0xfffd, 0x0ead, 0x0eae, 0x0eaf>,
										CharLine<0x0eb0, 0x0eb1, 0x0eb2, 0x0eb3, 0x0eb4, 0x0eb5, 0x0eb6, 0x0eb7, 0x0eb8, 0x0eb9, 0xfffd, 0x0ebb, 0x0ebc, 0x0ebd, 0xfffd, 0xfffd>,
										CharLine<0x0ec0, 0x0ec1, 0x0ec2, 0x0ec3, 0x0ec4, 0xfffd, 0x0ec6, 0xfffd, 0x0ec8, 0x0ec9, 0x0eca, 0x0ecb, 0x0ecc, 0x0ecd, 0xfffd, 0xfffd>,
										CharLine<0x0ed0, 0x0ed1, 0x0ed2, 0x0ed3, 0x0ed4, 0x0ed5, 0x0ed6, 0x0ed7, 0x0ed8, 0x0ed9, 0xfffd, 0xfffd, 0x0edc, 0x0edd, 0xfffd, 0xfffd>
									>
								>
							>("MuleLao-1", MIB_OTHER, "Lao (MuleLao-1)", "", 0x1a));
#endif // !ASCENSION_NO_MINORITY_ENCODINGS
						}
					} installer;
				}
			}
		}
	}
}
