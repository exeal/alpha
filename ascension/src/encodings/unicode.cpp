/**
 * @file unicode.cpp
 * Implements Unicode encodings. This includes:
 * - UTF-8
 * - UTF-7
 * - UTF-16BE
 * - UTF-16LE
 * - UTF-16
 * - UTF-32
 * - UTF-32BE
 * - UTF-32LE
 * - UTF-5
 * @author exeal
 * @date 2003-2012, 2014
 */

#include <ascension/corelib/encoder.hpp>
#include <ascension/corelib/text/utf.hpp>
#include <algorithm>	// std.find_if
#include <array>
#include <cassert>
#include <cstring>		// std.memcmp, std.memcpy
#include <type_traits>	// std.is_same

namespace ascension {
	namespace encoding {
		namespace implementation {
			// registry
			namespace {
				template<typename Factory>
				class InternalEncoder : public Encoder {
				public:
					explicit InternalEncoder(const Factory& factory) BOOST_NOEXCEPT : properties_(factory), encodingState_(0), decodingState_(0) {}
				private:
					Result doFromUnicode(Byte* to, Byte* toEnd, Byte*& toNext,
						const Char* from, const Char* fromEnd, const Char*& fromNext);
					Result doToUnicode(Char* to, Char* toEnd, Char*& toNext,
						const Byte* from, const Byte* fromEnd, const Byte*& fromNext);
					const EncodingProperties& properties() const BOOST_NOEXCEPT {
						return properties_;
					}
					Encoder& resetDecodingState() BOOST_NOEXCEPT {
						return (decodingState_ = 0), *this;
					}
					Encoder& resetEncodingState() BOOST_NOEXCEPT {
						return (encodingState_ = 0), *this;
					}
				private:
					const EncodingProperties& properties_;
					Byte encodingState_, decodingState_;
				};

				class Utf8 : public EncoderFactoryBase {
				public:
					Utf8() : EncoderFactoryBase("UTF-8", fundamental::UTF_8, "Unicode (UTF-8)", 4) {}
				private:
					std::unique_ptr<Encoder> create() const BOOST_NOEXCEPT {
						return std::unique_ptr<Encoder>(new InternalEncoder<Utf8>(*this));
					}
				};

				class Utf16LittleEndian : public EncoderFactoryBase {
				public:
					Utf16LittleEndian() : EncoderFactoryBase("UTF-16LE", fundamental::UTF_16LE, "Unicode (UTF-16LE)", 2) {}
				private:
					std::unique_ptr<Encoder> create() const BOOST_NOEXCEPT {
						return std::unique_ptr<Encoder>(new InternalEncoder<Utf16LittleEndian>(*this));
					}
				};

				class Utf16BigEndian : public EncoderFactoryBase {
				public:
					Utf16BigEndian() : EncoderFactoryBase("UTF-16BE", fundamental::UTF_16BE, "Unicode (UTF-16BE)", 2) {}
				private:
					std::unique_ptr<Encoder> create() const BOOST_NOEXCEPT {
						return std::unique_ptr<Encoder>(new InternalEncoder<Utf16BigEndian>(*this));
					}
				};

#ifndef ASCENSION_NO_STANDARD_ENCODINGS
				class Utf7 : public EncoderFactoryBase {
				public:
					Utf7() : EncoderFactoryBase("UTF-7", standard::UTF_7, "Unicode (UTF-7)", 8) {}
				private:
					std::unique_ptr<Encoder> create() const BOOST_NOEXCEPT {
						return std::unique_ptr<Encoder>(new InternalEncoder<Utf7>(*this));
					}
				};

				class Utf32LittleEndian : public EncoderFactoryBase {
				public:
					Utf32LittleEndian() : EncoderFactoryBase("UTF-32LE", standard::UTF_32LE, "Unicode (UTF-32LE)", 4) {}
				private:
					std::unique_ptr<Encoder> create() const BOOST_NOEXCEPT {
						return std::unique_ptr<Encoder>(new InternalEncoder<Utf32LittleEndian>(*this));
					}
				};

				class Utf32BigEndian : public EncoderFactoryBase {
				public:
					Utf32BigEndian() : EncoderFactoryBase("UTF-32BE", standard::UTF_32BE, "Unicode (UTF-32BE)", 4) {}
				private:
					std::unique_ptr<Encoder> create() const BOOST_NOEXCEPT {
						return std::unique_ptr<Encoder>(new InternalEncoder<Utf32BigEndian>(*this));
					}
				};
#endif // !ASCENSION_NO_STANDARD_ENCODINGS

#ifndef ASCENSION_NO_MINORITY_ENCODINGS
				class Utf5 : public EncoderFactoryBase {
				public:
					Utf5() : EncoderFactoryBase("UTF-5", MIB_OTHER, "Unicode (UTF-5)", 6) {}
				private:
					std::unique_ptr<Encoder> create() const BOOST_NOEXCEPT {
						return std::unique_ptr<Encoder>(new InternalEncoder<Utf5>(*this));
					}
				};
#endif // !ASCENSION_NO_MINORITY_ENCODINGS

				class UnicodeDetector : public EncodingDetector {
				public:
					UnicodeDetector() : EncodingDetector("UnicodeAutoDetect") {}
				private:
					std::pair<MIBenum, std::string> doDetect(const Byte* first, const Byte* last, std::ptrdiff_t* convertibleBytes) const BOOST_NOEXCEPT;
				};

				struct Installer {
					Installer() BOOST_NOEXCEPT : UTF_8(std::make_shared<Utf8>()),
							UTF_16LE(std::make_shared<Utf16LittleEndian>()), UTF_16BE(std::make_shared<Utf16BigEndian>())
#ifndef ASCENSION_NO_STANDARD_ENCODINGS
							, UTF_7(std::make_shared<Utf7>()), UTF_32LE(std::make_shared<Utf32LittleEndian>()), UTF_32BE(std::make_shared<Utf32BigEndian>())
#endif // !ASCENSION_NO_STANDARD_ENCODINGS
#ifndef ASCENSION_NO_MINORITY_ENCODINGS
							, UTF_5(std::make_shared<Utf5>())
#endif // !ASCENSION_NO_MINORITY_ENCODINGS
					{
						Encoder::registerFactory(UTF_8);
						Encoder::registerFactory(UTF_16LE);
						Encoder::registerFactory(UTF_16BE);
#ifndef ASCENSION_NO_STANDARD_ENCODINGS
						Encoder::registerFactory(UTF_7);
						Encoder::registerFactory(UTF_32LE);
						Encoder::registerFactory(UTF_32BE);
#endif // !ASCENSION_NO_STANDARD_ENCODINGS
#ifndef ASCENSION_NO_MINORITY_ENCODINGS
						Encoder::registerFactory(UTF_5);
#endif // !ASCENSION_NO_MINORITY_ENCODINGS
						EncodingDetector::registerDetector(std::make_shared<UnicodeDetector>());
					}

					const std::shared_ptr<Utf8> UTF_8;
					const std::shared_ptr<Utf16LittleEndian> UTF_16LE;
					const std::shared_ptr<Utf16BigEndian> UTF_16BE;
#ifndef ASCENSION_NO_STANDARD_ENCODINGS
					const std::shared_ptr<Utf7> UTF_7;
					const std::shared_ptr<Utf32LittleEndian> UTF_32LE;
					const std::shared_ptr<Utf32BigEndian> UTF_32BE;
#endif // !ASCENSION_NO_STANDARD_ENCODINGS
#ifndef ASCENSION_NO_MINORITY_ENCODINGS
					const std::shared_ptr<Utf5> UTF_5;
#endif // !ASCENSION_NO_MINORITY_ENCODINGS
				} installer;

				const std::array<Byte, 3> UTF8_BOM = {0xef, 0xbb, 0xbf};
				const std::array<Byte, 2> UTF16LE_BOM = {0xff, 0xfe};
				const std::array<Byte, 2> UTF16BE_BOM = {0xfe, 0xff};
#ifndef ASCENSION_NO_STANDARD_ENCODINGS
				const std::array<Byte, 4> UTF32LE_BOM = {0xff, 0xff, 0x00, 0x00};
				const std::array<Byte, 4> UTF32BE_BOM = {0xfe, 0xff, 0x00, 0x00};
#endif // !ASCENSION_NO_STANDARD_ENCODINGS

#define ASCENSION_ENCODE_BOM(encoding)														\
	if((flags() & BEGINNING_OF_BUFFER) != 0 && (flags() & UNICODE_BYTE_ORDER_MARK) != 0) {	\
		if(to + encoding##_BOM.size() >= toEnd)												\
			return INSUFFICIENT_BUFFER;														\
		std::memcpy(to, encoding##_BOM.data(), encoding##_BOM.size());						\
		to += encoding##_BOM.size();														\
	}

#define ASCENSION_DECODE_BOM(encoding)															\
	if((flags() & BEGINNING_OF_BUFFER) != 0) {													\
		if(fromEnd - from >= static_cast<std::ptrdiff_t>(encoding##_BOM.size())					\
				&& std::equal(std::begin(encoding##_BOM), std::end(encoding##_BOM), from)) {	\
			setFlags(flags() | UNICODE_BYTE_ORDER_MARK);										\
			from += encoding##_BOM.size();														\
		} else																					\
			setFlags(flags() & ~UNICODE_BYTE_ORDER_MARK);										\
	}


				// UTF-8 //////////////////////////////////////////////////////////////////////////////////////////////

				/*
					well-formed UTF-8 first byte distribution (based on Unicode 5.0 Table 3.7)
					value  1st-byte   code points       byte count
					----------------------------------------------
					10     00..7F     U+0000..007F      1
					21     C2..DF     U+0080..07FF      2
					32     E0         U+0800..0FFF      3
					33     E1..EC     U+1000..CFFF      3
					34     ED         U+D000..D7FF      3
					35     EE..EF     U+E000..FFFF      3
					46     F0         U+10000..3FFFF    4
					47     F1..F3     U+40000..FFFFF    4
					48     F4         U+100000..10FFFF  4
					09     otherwise  ill-formed        (0)
				 */
				const Byte UTF8_WELL_FORMED_FIRST_BYTES[] = {
					0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,	// 0x80
					0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,	// 0x90
					0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,	// 0xA0
					0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,	// 0xB0
					0x09, 0x09, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21,	// 0xC0
					0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21,	// 0xD0
					0x32, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x34, 0x35, 0x35,	// 0xE0
					0x46, 0x47, 0x47, 0x47, 0x48, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09	// 0xF0
				};

				inline bool writeSurrogatePair(Byte*& to, Byte* toEnd, Char high, Char low) {
					if(to + 3 >= toEnd)
						return false;
					// 0000 0000  000w wwxx  xxxx yyyy  yyzz zzzz -> 1111 0www  10xx xxxx  10yy yyyy 10zz zzzz
					const CodePoint c = text::surrogates::checkedDecode(high, low);
					(*to++) = 0xf0 | mask8Bit((c & 0x001c0000ul) >> 18);
					(*to++) = 0x80 | mask8Bit((c & 0x0003f000ul) >> 12);
					(*to++) = 0x80 | mask8Bit((c & 0x00000fc0ul) >> 6);
					(*to++) = 0x80 | mask8Bit((c & 0x0000003ful) >> 0);
					return true;
				}

				template<> Encoder::Result InternalEncoder<Utf8>::doFromUnicode(
						Byte* to, Byte* toEnd, Byte*& toNext, const Char* from, const Char* fromEnd, const Char*& fromNext) {
					ASCENSION_ENCODE_BOM(UTF8)
					for(; to < toEnd && from < fromEnd; ++from) {
						if(*from < 0x0080u)	// 0000 0000  0zzz zzzz -> 0zzz zzzz
							(*to++) = mask8Bit(*from);
						else if(*from < 0x0800u) {	// 0000 0yyy  yyzz zzzz -> 110y yyyy  10zz zzzz
							if(to + 1 >= toEnd)
								break;
							(*to++) = 0xc0 | mask8Bit(*from >> 6);
							(*to++) = 0x80 | mask8Bit(*from & 0x003fu);
						} else if(text::surrogates::isHighSurrogate(*from)) {
							if(from + 1 == fromEnd) {
								toNext = to;
								fromNext = from;
								return COMPLETED;
							} else if(text::surrogates::isLowSurrogate(from[1])) {
								if(!writeSurrogatePair(to, toEnd, from[0], from[1]))
									break;
								++from;
							} else {
								toNext = to;
								fromNext = from;
								return MALFORMED_INPUT;
							}
						} else {	// xxxx yyyy  yyzz zzzz -> 1110 xxxx  10yy yyyy  10zz zzzz
							if(to + 2 >= toEnd)
								break;
							(*to++) = 0xe0 | mask8Bit((*from & 0xf000u) >> 12);
							(*to++) = 0x80 | mask8Bit((*from & 0x0fc0u) >> 6);
							(*to++) = 0x80 | mask8Bit((*from & 0x003fu) >> 0);
						}
					}
					fromNext = from;
					toNext = to;
					return (from == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
				}

				template<> Encoder::Result InternalEncoder<Utf8>::doToUnicode(
						Char* to, Char* toEnd, Char*& toNext, const Byte* from, const Byte* fromEnd, const Byte*& fromNext) {
					ASCENSION_DECODE_BOM(UTF8)
					while(to < toEnd && from < fromEnd) {
						if(*from < 0x80)
							*(to++) = *(from++);
						else {
							const Byte v = UTF8_WELL_FORMED_FIRST_BYTES[*from - 0x80];
							// check the source buffer length
							std::ptrdiff_t bytes = (v >> 4);
							if(fromEnd - from < bytes) {
								toNext = to;
								fromNext = from;
								return COMPLETED;
							}
							// check the second byte
							switch(v & 0x0f) {
								case 1:
								case 3:
								case 5:
								case 7:
									if(from[1] < 0x80 || from[1] > 0xbf)
										bytes = 0;
									break;
								case 2:
									if(from[1] < 0xa0 || from[1] > 0xbf)
										bytes = 0;
									break;
								case 4:
									if(from[1] < 0x80 || from[1] > 0x9f)
										bytes = 0;
									break;
								case 6:
									if(from[1] < 0x90 || from[1] > 0xbf)
										bytes = 0;
									break;
								case 8:
									if(from[1] < 0x80 || from[1] > 0x8f)
										bytes = 0;
									break;
							}
							// check the third byte
							if(bytes >= 3 && (from[2] < 0x80 || from[2] > 0xbf))
								bytes = 0;
							// check the forth byte
							if(bytes >= 4 && (from[3] < 0x80 || from[3] > 0xbf))
								bytes = 0;

							if(bytes == 0) {
								toNext = to;
								fromNext = from;
								return MALFORMED_INPUT;
							}

							// decode
							CodePoint cp;
							assert(bytes >= 2 && bytes <= 4);
							switch(bytes) {
								case 2:	// 110y yyyy  10zz zzzz -> 0000 0yyy yyzz zzzz
									cp = ((from[0] & 0x1f) << 6) | ((from[1] & 0x3f) << 0);
									break;
								case 3:	// 1110 xxxx  10yy yyyy  10zz zzzz -> xxxx yyyy yyzz zzzz
									cp = ((from[0] & 0x0f) << 12) | ((from[1] & 0x3f) << 6) | ((from[2] & 0x3f) << 0);
									break;
								case 4:	// 1111 0www  10xx xxxx  10yy yyyy  10zz zzzz -> 0000 0000 000w wwxx xxxx yyyy yyzz zzzz
									cp = ((from[0] & 0x07) << 18) | ((from[1] & 0x3f) << 12)
										| ((from[2] & 0x3f) << 6) | ((from[3] & 0x3f) << 0);
									break;
							}

							if(to == toEnd - 1 && text::surrogates::isSupplemental(cp)) {
								fromNext = from;
								toNext = to;
								return INSUFFICIENT_BUFFER;
							}
							text::utf::encode(cp, to);
							to += text::surrogates::isSupplemental(cp) ? 2 : 1;
							from += bytes;
						}

					}
					fromNext = from;
					toNext = to;
					return (from == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
				}


				// UTF-16LE ///////////////////////////////////////////////////////////////////////////////////////////

				template<> Encoder::Result InternalEncoder<Utf16LittleEndian>::doFromUnicode(
						Byte* to, Byte* toEnd, Byte*& toNext, const Char* from, const Char* fromEnd, const Char*& fromNext) {
					ASCENSION_ENCODE_BOM(UTF16LE)
					for(; to < toEnd - 1 && from < fromEnd; ++from) {
						*(to++) = static_cast<Byte>((*from & 0x00ffu) >> 0);
						*(to++) = static_cast<Byte>((*from & 0xff00u) >> 8);
					}
					fromNext = from;
					toNext = to;
					return (from == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
				}

				template<> Encoder::Result InternalEncoder<Utf16LittleEndian>::doToUnicode(
						Char* to, Char* toEnd, Char*& toNext, const Byte* from, const Byte* fromEnd, const Byte*& fromNext) {
					ASCENSION_DECODE_BOM(UTF16LE)
					for(; to < toEnd && from < fromEnd - 1; from += 2)
						*(to++) = *from | maskUCS2(from[1] << 8);
					fromNext = from;
					toNext = to;
					if(from == fromEnd)
						return COMPLETED;
					else
						return (to >= toEnd - 1) ? INSUFFICIENT_BUFFER : UNMAPPABLE_CHARACTER;
				}


				// UTF-16BE ///////////////////////////////////////////////////////////////////////////////////////////

				template<> Encoder::Result InternalEncoder<Utf16BigEndian>::doFromUnicode(
						Byte* to, Byte* toEnd, Byte*& toNext, const Char* from, const Char* fromEnd, const Char*& fromNext) {
					ASCENSION_ENCODE_BOM(UTF16BE)
					for(; to < toEnd - 1 && from < fromEnd; ++from) {
						*(to++) = static_cast<Byte>((*from & 0xff00u) >> 8);
						*(to++) = static_cast<Byte>((*from & 0x00ffu) >> 0);
					}
					fromNext = from;
					toNext = to;
					return (from == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
				}

				template<> Encoder::Result InternalEncoder<Utf16BigEndian>::doToUnicode(
						Char* to, Char* toEnd, Char*& toNext, const Byte* from, const Byte* fromEnd, const Byte*& fromNext) {
					ASCENSION_DECODE_BOM(UTF16BE)
					for(; to < toEnd && from < fromEnd - 1; from += 2)
						*(to++) = maskUCS2(*from << 8) | from[1];
					fromNext = from;
					toNext = to;
					if(from == fromEnd)
						return COMPLETED;
					else
						return (to >= toEnd - 1) ? INSUFFICIENT_BUFFER : UNMAPPABLE_CHARACTER;
				}


#ifndef ASCENSION_NO_STANDARD_ENCODINGS

				// UTF-32LE ///////////////////////////////////////////////////////////////////////////////////////////

				template<> Encoder::Result InternalEncoder<Utf32LittleEndian>::doFromUnicode(
						Byte* to, Byte* toEnd, Byte*& toNext, const Char* from, const Char* fromEnd, const Char*& fromNext) {
					ASCENSION_ENCODE_BOM(UTF32LE)
					for(; to < toEnd - 3 && from < fromEnd; ++from) {
						const CodePoint c = text::utf::decodeFirst(from, fromEnd);
						if(!text::isScalarValue(c)) {
							toNext = to;
							fromNext = from;
							if(text::surrogates::isHighSurrogate(c) && from == fromEnd - 1)	// low surrogate may appear immediately
								return COMPLETED;
							return MALFORMED_INPUT;
						}
						*(to++) = mask8Bit((c & 0x000000fful) >> 0);
						*(to++) = mask8Bit((c & 0x0000ff00ul) >> 8);
						*(to++) = mask8Bit((c & 0x00ff0000ul) >> 16);
						*(to++) = mask8Bit((c & 0xff000000ul) >> 24);
						if(text::surrogates::isSupplemental(c))
							++from;
					}
					fromNext = from;
					toNext = to;
					return (from == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
				}

				template<> Encoder::Result InternalEncoder<Utf32LittleEndian>::doToUnicode(
						Char* to, Char* toEnd, Char*& toNext, const Byte* from, const Byte* fromEnd, const Byte*& fromNext) {
					ASCENSION_DECODE_BOM(UTF32LE)
					for(; to < toEnd && from < fromEnd - 3; from += 4) {
						const CodePoint c = from[0] + (from[1] << 8) + (from[2] << 16) + (from[3] << 24);
						if(text::isValidCodePoint(c)) {
							if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTERS)
								*(to++) = text::REPLACEMENT_CHARACTER;
							else if(substitutionPolicy() != IGNORE_UNMAPPABLE_CHARACTERS) {
								fromNext = from;
								toNext = to;
								return UNMAPPABLE_CHARACTER;
							}
						} else
							to += text::utf::encode(c, to);
					}
					fromNext = from;
					toNext = to;
					return (from == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
				}


				// UTF-32BE ///////////////////////////////////////////////////////////////////////////////////////////

				template<> Encoder::Result InternalEncoder<Utf32BigEndian>::doFromUnicode(
						Byte* to, Byte* toEnd, Byte*& toNext, const Char* from, const Char* fromEnd, const Char*& fromNext) {
					ASCENSION_ENCODE_BOM(UTF32BE)
					for(; to < toEnd - 3 && from < fromEnd; ++from) {
						const CodePoint c = text::utf::decodeFirst(from, fromEnd);
						*(to++) = mask8Bit((c & 0xff000000ul) >> 24);
						*(to++) = mask8Bit((c & 0x00ff0000ul) >> 16);
						*(to++) = mask8Bit((c & 0x0000ff00ul) >> 8);
						*(to++) = mask8Bit((c & 0x000000fful) >> 0);
						if(text::surrogates::isSupplemental(c))
							++from;
					}
					fromNext = from;
					toNext = to;
					return (from == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
				}

				template<> Encoder::Result InternalEncoder<Utf32BigEndian>::doToUnicode(
						Char* to, Char* toEnd, Char*& toNext, const Byte* from, const Byte* fromEnd, const Byte*& fromNext) {
					ASCENSION_DECODE_BOM(UTF32BE)
					for(; to < toEnd && from < fromEnd - 3; from += 4) {
						const CodePoint cp = from[3] + (from[2] << 8) + (from[1] << 16) + (from[0] << 24);
						if(text::isValidCodePoint(cp)) {
							if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTERS)
								*(to++) = text::REPLACEMENT_CHARACTER;
							else if(substitutionPolicy() != IGNORE_UNMAPPABLE_CHARACTERS) {
								fromNext = from;
								toNext = to;
								return UNMAPPABLE_CHARACTER;
							}
						} else
							to += text::utf::encode(cp, to);
					}
					fromNext = from;
					toNext = to;
					return (from == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
				}


				// UTF-7 //////////////////////////////////////////////////////////////////////////////////////////////

				template<> Encoder::Result InternalEncoder<Utf7>::doFromUnicode(
						Byte* to, Byte* toEnd, Byte*& toNext, const Char* from, const Char* fromEnd, const Char*& fromNext) {
					static const Byte SET_D[0x80] = {
						// 1 : in set D, 2 : '=', 3 : direct encodable but not set D, 0 : otherwise
						0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 0, 0, 3, 0, 0,	// 0x00
						0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	// 0x10
						3, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 2, 1, 1, 1, 1,	// 0x20
						1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1,	// 0x30
						0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,	// 0x40
						1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,	// 0x50
						0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,	// 0x60
						1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0	// 0x70
					};
					static const Byte BASE64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

					// encodingState_ == 1 if in BASE64. 0 otherwise
					for(; to < toEnd && from < fromEnd; ++from) {
						const Byte klass = (*from < 0x80) ? SET_D[*from] : 0;
						if((klass & 1) == 1) {
							// encode directly (ascension puts '-' explicitly even if klass is 3)
							if(encodingState_ == 1) {
								*to = '-';
								encodingState_ = 0;
								if(++to == toEnd)
									break;	// the destination buffer is insufficient
							}
							*(to++) = mask8Bit(*from);
						} else if(klass == 2) {
							// '+' => '+-'
							if(to + 1 == toEnd)
								break;	// the destination buffer is insufficient
							*(to++) = '+';
							*(to++) = '-';
						} else {
							// modified BASE64 encode
							if(encodingState_ == 0) {
								*to = '+';
								encodingState_ = 1;
								if(++to == toEnd)
									break;	// the destination buffer is insufficient
							}
							// first, determine how many source characters can be encoded
							std::ptrdiff_t encodables = 1;
							if(from + 1 < fromEnd && (from[1] >= 0x80 || SET_D[from[1]] == 0)) {
								++encodables;
								if(from + 2 < fromEnd && (from[2] >= 0x80 || SET_D[from[2]] == 0))
									++encodables;
							}
							// check the size of the destination buffer
							switch(encodables) {
								case 3:
									if(to + 8 >= toEnd)
										encodables = 0;
									break;
								case 2:
									if(to + 6 >= toEnd)
										encodables = 0;
									break;
								case 1:
									if(to + 3 >= toEnd)
										encodables = 0;
									break;
							}
							if(encodables == 0)
								break;	// the destination buffer is insufficient

							// encode
							const Char utf16[3] = {from[0], (encodables > 1) ? from[1] : 0, (encodables > 2) ? from[2] : 0};
							*(to++) = BASE64[utf16[0] >> 10];
							*(to++) = BASE64[(utf16[0] >> 4) & 0x3f];
							*(to++) = BASE64[(utf16[0] << 2 | utf16[1] >> 14) & 0x3f];
							if(encodables >= 2) {
								*(to++) = BASE64[(utf16[1] >> 8) & 0x3f];
								*(to++) = BASE64[(utf16[1] >> 2) & 0x3f];
								*(to++) = BASE64[(utf16[1] << 4 | utf16[2] >> 12) & 0x3f];
								if(encodables >= 3) {
									*(to++) = BASE64[(utf16[2] >> 6) & 0x3f];
									*(to++) = BASE64[utf16[2] & 0x3f];
								}
							}
							from += encodables - 1;
						}
					}
					if(from == fromEnd && (flags() & END_OF_BUFFER) != 0 && to != toEnd)
						*(to++) = '-';
					toNext = to;
					fromNext = from;
					return (from == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
				}
		
				template<> Encoder::Result InternalEncoder<Utf7>::doToUnicode(
						Char* to, Char* toEnd, Char*& toNext, const Byte* from, const Byte* fromEnd, const Byte*& fromNext) {
					static const Byte SET_B[0x80] = {
						// 1 : in set B, 2 : '+', 3 : directly appearable in BASE64, 4 : '-', 0 : otherwise
						0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 0, 0, 3, 0, 0,	// 0x00
						0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	// 0x10
						3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 4, 0, 1,	// 0x20
						1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,	// 0x30
						0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,	// 0x40
						1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,	// 0x50
						0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,	// 0x60
						1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0	// 0x70
					};
					static const Byte BASE64[0x80] = {
						0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,	// <00>
						0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,	// <10>
						0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x3e, 0xff, 0xff, 0xff, 0x3f,	//  !"#$%&'()*+,-./
						0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,	// 0123456789:;<=>?
						0xff, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,	// @ABCDEFGHIJKLMNO
						0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0xff, 0xff, 0xff, 0xff, 0xff,	// PQRSTUVWXYZ[\]^_
						0xff, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,	// `abcdefghijklmno
						0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30,	0x31, 0x32, 0x33, 0xff, 0xff, 0xff, 0xff, 0xff	// pqrstuvwxyz{|}~
					};

					// decodingState_ == 1 if in BASE64. 0 otherwise
					while(to < toEnd && from < fromEnd) {
						if(*from >= 0x80 || SET_B[*from] == 0) {
							toNext = to;
							fromNext = from;
							return MALFORMED_INPUT;
						}
						const Byte klass = SET_B[*from];
						if(klass == 2) {
							// '+'
							if(from + 1 == fromEnd) {	// the input is terminated by '+'...
								if((flags() & END_OF_BUFFER) != 0) {
									toNext = to;
									fromNext = from;
									return COMPLETED;
								} else {
									decodingState_ = 1;
									++from;
									break;
								}
							} else if(from[1] == '-') {
								// '+-' => '+'
								*(to++) = L'+';
								from += 2;
							} else {
								decodingState_ = 1;	// introduce modified BASE64 sequence
								++from;
							}
						} else if(klass == 3) {
							(*to++) = (*from++);
							decodingState_ = 0;	// terminate modified BASE64 implicitly
						} else if(klass == 4) {
							// '-'
							if(decodingState_ == 1)
								decodingState_ = 0;
							else {
								toNext = to;
								fromNext = from;
								return MALFORMED_INPUT;	// '-' can't appear here
								// ...this can't handle '-' appeared at the exact beginning of the input buffer
							}
							++from;
						} else {
							// first, determine how many bytes can be decoded
							std::ptrdiff_t decodables = 1;
							for(const std::ptrdiff_t minimum = std::min<std::ptrdiff_t>(fromEnd - from, 8); decodables < minimum; ++decodables) {
								if(BASE64[from[decodables]] == 0xff)
									break;
							}
							// check the size of the destination buffer
							switch(decodables) {
							case 8: if(to + 2 >= toEnd) decodables = 0; break;
							case 6: if(to + 1 >= toEnd) decodables = 0; break;
							case 3: break;
							default:
								toNext = to;
								fromNext = from;
								return MALFORMED_INPUT;	// invalid modified BASE64 sequence
							}
							if(decodables == 0)
								break;	// the destination buffer is insufficient

							// decode
							*(to++) = BASE64[from[0]] << 10 | BASE64[from[1]] << 4 | BASE64[from[2]] >> 2;
							if(decodables >= 6) {
								*(to++) = maskUCS2(BASE64[from[2]] << 14) | BASE64[from[3]] << 8 | BASE64[from[4]] << 2 | BASE64[from[5]] >> 4;
								if(decodables >= 8)
									*(to++) = BASE64[from[5]] << 12 | BASE64[from[6]] << 6 | BASE64[from[7]];
							}
							from += decodables;			
						}
					}
					toNext = to;
					fromNext = from;
					return (from == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
				}

#endif // !ASCENSION_NO_STANDARD_ENCODINGS

#ifndef ASCENSION_NO_MINORITY_ENCODINGS

				// UTF-5 //////////////////////////////////////////////////////////////////////////////////////////////

				/**
				 * Transcodes the given UTF-5 sequence into a Unicode character.
				 * @param first the beginning of the sequence
				 * @param last the end of the sequence
				 * @param[out] cp code point of the decoded character
				 * @return the end of the eaten subsequence
				 */
				inline const Byte* decodeUtf5Character(const Byte* first, const Byte* last, CodePoint& cp) BOOST_NOEXCEPT {
					if(*first < 'G' || *first > 'V')
						return nullptr;
					cp = *first - 'G';
					for(++first; first < last; ++first) {
						if(*first >= '0' && *first <= '9') {
							cp <<= 4;
							cp |= *first - '0';
						} else if(*first >= 'A' && *first <= 'F'){
							cp <<= 4;
							cp |= *first - 'A' + 0x0a;
						} else
							break;
					}
					return first;
				}

				/**
				 * Transcodes the given Unicode character into UTF-5.
				 * @param from the beginning of the sequence
				 * @param fromEnd the end of the sequence
				 * @param[out] to beginning of the destination buffer
				 * @return the end of the eaten subsequence
				 */
				inline Byte* encodeUtf5Character(const Char* from, const Char* fromEnd, Byte* to) {
#define D2C(n) (mask8Bit(n) < 0x0a) ? (mask8Bit(n) + '0') : (mask8Bit(n) - 0x0a + 'A')

					const CodePoint cp = text::utf::decodeFirst(from, fromEnd);
					if(cp < 0x00000010ul)
						*(to++) = mask8Bit((cp & 0x0000000ful) >> 0) + 'G';
					else if(cp < 0x00000100ul) {
						*(to++) = mask8Bit((cp & 0x000000f0ul) >> 4) + 'G';
						*(to++) = D2C((cp & 0x0000000ful) >> 0);
					} else if(cp < 0x00001000ul) {
						*(to++) = mask8Bit((cp & 0x00000f00ul) >> 8) + 'G';
						*(to++) = D2C((cp & 0x000000f0ul) >> 4);
						*(to++) = D2C((cp & 0x0000000ful) >> 0);
					} else if(cp < 0x00010000ul) {
						*(to++) = mask8Bit((cp & 0x0000f000ul) >> 12) + 'G';
						*(to++) = D2C((cp & 0x00000f00ul) >> 8);
						*(to++) = D2C((cp & 0x000000f0ul) >> 4);
						*(to++) = D2C((cp & 0x0000000ful) >> 0);
					} else if(cp < 0x00100000ul) {
						*(to++) = mask8Bit((cp & 0x000f0000ul) >> 16) + 'G';
						*(to++) = D2C((cp & 0x0000f000ul) >> 12);
						*(to++) = D2C((cp & 0x00000f00ul) >> 8);
						*(to++) = D2C((cp & 0x000000f0ul) >> 4);
						*(to++) = D2C((cp & 0x0000000ful) >> 0);
					} else if(cp < 0x01000000ul) {
						*(to++) = mask8Bit((cp & 0x00f00000ul) >> 20) + 'G';
						*(to++) = D2C((cp & 0x000f0000ul) >> 16);
						*(to++) = D2C((cp & 0x0000f000ul) >> 12);
						*(to++) = D2C((cp & 0x00000f00ul) >> 8);
						*(to++) = D2C((cp & 0x000000f0ul) >> 4);
						*(to++) = D2C((cp & 0x0000000ful) >> 0);
					} else if(cp < 0x10000000ul) {
						*(to++) = mask8Bit((cp & 0x0f000000ul) >> 24) + 'G';
						*(to++) = D2C((cp & 0x00f00000ul) >> 20);
						*(to++) = D2C((cp & 0x000f0000ul) >> 16);
						*(to++) = D2C((cp & 0x0000f000ul) >> 12);
						*(to++) = D2C((cp & 0x00000f00ul) >> 8);
						*(to++) = D2C((cp & 0x000000f0ul) >> 4);
						*(to++) = D2C((cp & 0x0000000ful) >> 0);
					} else if(cp < 0x80000000ul) {
						*(to++) = mask8Bit((cp & 0xf0000000ul) >> 28) + 'G';
						*(to++) = D2C((cp & 0x0f000000ul) >> 24);
						*(to++) = D2C((cp & 0x00f00000ul) >> 20);
						*(to++) = D2C((cp & 0x000f0000ul) >> 16);
						*(to++) = D2C((cp & 0x0000f000ul) >> 12);
						*(to++) = D2C((cp & 0x00000f00ul) >> 8);
						*(to++) = D2C((cp & 0x000000f0ul) >> 4);
						*(to++) = D2C((cp & 0x0000000ful) >> 0);
					}
					return to;
#undef D2C
				}

				template<> Encoder::Result InternalEncoder<Utf5>::doFromUnicode(
						Byte* to, Byte* toEnd, Byte*& toNext, const Char* from, const Char* fromEnd, const Char*& fromNext) {
					Byte temp[8];
					Byte* e;
					for(; to < toEnd && from < fromEnd; ++from) {
						e = encodeUtf5Character(from, fromEnd, temp);
						if(e == temp) {
							if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTERS)
								*(to++) = properties().substitutionCharacter();
							else if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTERS)
								continue;
							else {
								fromNext = from;
								toNext = to;
								return UNMAPPABLE_CHARACTER;
							}
						} else if(e - temp > toEnd - to) {
							fromNext = from;
							toNext = to;
							return INSUFFICIENT_BUFFER;
						} else {
							std::memcpy(to, temp, e - temp);
							to += e - temp;
							if(e - temp >= 5)
								++from;
						}
					}
					fromNext = from;
					toNext = to;
					return (from == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
				}

				template<> Encoder::Result InternalEncoder<Utf5>::doToUnicode(
						Char* to, Char* toEnd, Char*& toNext, const Byte* from, const Byte* fromEnd, const Byte*& fromNext) {
					const Byte* e;
					CodePoint cp;
					while(to < toEnd && from < fromEnd) {
						e = decodeUtf5Character(from, fromEnd, cp);
						if(e == from) {
							fromNext = from;
							toNext = to;
							return MALFORMED_INPUT;
						} else if(!text::isValidCodePoint(cp)) {
							if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTERS) {
								cp = text::REPLACEMENT_CHARACTER;
								if(e == from)
									e = from + 1;
							} else if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTERS) {
								++from;
								continue;
							} else {
								fromNext = from;
								toNext = to;
								return UNMAPPABLE_CHARACTER;
							}
						}
						if(to == toEnd - 1 && text::surrogates::isSupplemental(cp)) {
							fromNext = from;
							toNext = to;
							return INSUFFICIENT_BUFFER;
						}
						from = e;
						text::utf::encode(cp, to);
						to += text::surrogates::isSupplemental(cp) ? 2 : 1;
					}
					fromNext = from;
					toNext = to;
					return (from == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
				}
#endif // !ASCENSION_NO_MINORITY_ENCODINGS

				inline const Byte* maybeUtf8(const Byte* first, const Byte* last) BOOST_NOEXCEPT {
					while(first < last) {
						if(*first == 0xc0 || *first == 0xc1 || *first >= 0xf5)
							break;
						++first;
					}
					return first;
				}

				std::size_t UnicodeDetector(const Byte* first, const Byte* last, MIBenum& mib) {
					mib = MIB_UNKNOWN;
					if(last - first >= static_cast<std::ptrdiff_t>(UTF8_BOM.size()) && std::equal(std::begin(UTF8_BOM), std::end(UTF8_BOM), first))
						mib = fundamental::UTF_8;
					else if(last - first >= static_cast<std::ptrdiff_t>(UTF16LE_BOM.size())) {
						static_assert(std::is_same<decltype(UTF16LE_BOM), decltype(UTF16BE_BOM)>::value, "");
						if(std::equal(std::begin(UTF16LE_BOM), std::end(UTF16LE_BOM), first))
							mib = fundamental::UTF_16LE;
						else if(std::equal(std::begin(UTF16BE_BOM), std::end(UTF16BE_BOM), first))
							mib = fundamental::UTF_16BE;
#ifndef ASCENSION_NO_STANDARD_ENCODINGS
						if(last - first >= static_cast<std::ptrdiff_t>(UTF32LE_BOM.size())) {
							static_assert(std::is_same<decltype(UTF32LE_BOM), decltype(UTF32BE_BOM)>::value, "");
							if(std::equal(std::begin(UTF32LE_BOM), std::end(UTF32LE_BOM), first))
								mib = standard::UTF_32LE;
							else if(std::equal(std::begin(UTF32BE_BOM), std::end(UTF32BE_BOM), first))
								mib = standard::UTF_32BE;
						}
#endif // !ASCENSION_NO_STANDARD_ENCODINGS
					}
					if(mib != MIB_UNKNOWN)
						return last - first;
					mib = fundamental::UTF_8;
					return maybeUtf8(first, last) - first;
				}

				/// @see EncodingDetector#doDetect
				std::pair<MIBenum, std::string> UnicodeDetector::doDetect(const Byte* first, const Byte* last, std::ptrdiff_t* convertibleBytes) const BOOST_NOEXCEPT {
					std::shared_ptr<const EncodingProperties> result;
					// first, test Unicode byte order marks
					if(last - first >= static_cast<std::ptrdiff_t>(UTF8_BOM.size()) && std::equal(std::begin(UTF8_BOM), std::end(UTF8_BOM), first))
						result = installer.UTF_8;
					else if(last - first >= static_cast<std::ptrdiff_t>(UTF16LE_BOM.size())) {
						static_assert(std::is_same<decltype(UTF16LE_BOM), decltype(UTF16BE_BOM)>::value, "");
						if(std::equal(std::begin(UTF16LE_BOM), std::end(UTF16LE_BOM), first))
							result = installer.UTF_16LE;
						else if(std::equal(std::begin(UTF16BE_BOM), std::end(UTF16BE_BOM), first))
							result = installer.UTF_16BE;
#ifndef ASCENSION_NO_STANDARD_ENCODINGS
						if(last - first >= static_cast<std::ptrdiff_t>(UTF32LE_BOM.size())) {
							static_assert(std::is_same<decltype(UTF32LE_BOM), decltype(UTF32BE_BOM)>::value, "");
							if(std::equal(std::begin(UTF32LE_BOM), std::end(UTF32LE_BOM), first))
								result = installer.UTF_32LE;
							else if(std::equal(std::begin(UTF32BE_BOM), std::end(UTF32BE_BOM), first))
								result = installer.UTF_32BE;
						}
#endif // !ASCENSION_NO_STANDARD_ENCODINGS
					}
					if(result != nullptr) {
						if(convertibleBytes != 0)
							*convertibleBytes = last - first;
					} else {
						// force into UTF-8
						result = installer.UTF_8;
						if(convertibleBytes != 0)
							*convertibleBytes = maybeUtf8(first, last) - first;
					}
					return std::make_pair(result->mibEnum(), result->name());
				}
			}
		}
	}
}

#undef ASCENSION_ENCODE_BOM
#undef ASCENSION_DECODE_BOM
