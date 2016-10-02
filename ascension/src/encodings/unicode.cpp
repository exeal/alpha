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
 * @note To write byte order mark with UTF-8, UTF-16 or UTF-32 in encoding context, use @c Encoder#writeByteOrderMark
 *       method.
 * @note UTF-16BE, UTF-16LE, UTF32-BE and UTF-32LE does not write byte order mark event if
 *       @c Encoder#writesByteOrderMark method returned @c true.
 * @note To check if the encoder read a byte order mark, use @c Encoder#isBigEndian method.
 * @note In this implementation, UTF-7 and UTF-5 never use byte order marks.
 * @author exeal
 * @date 2003-2012, 2014
 */

#include <ascension/corelib/encoding/encoder.hpp>
#include <ascension/corelib/encoding/encoder-implementation.hpp>
#include <ascension/corelib/encoding/encoding-detector.hpp>
#include <ascension/corelib/text/utf.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/range/algorithm/equal.hpp>
#include <array>
#include <cassert>
#include <type_traits>	// std.is_same

namespace ascension {
	namespace encoding {
		namespace implementation {
			// registry
			namespace {
				template<typename Factory>
				class InternalEncoder : public Encoder {
				public:
					explicit InternalEncoder(const Factory& factory) BOOST_NOEXCEPT : properties_(factory) {}
				private:
					Result doFromUnicode(State& state,
						const boost::iterator_range<Byte*>& to, Byte*& toNext,
						const boost::iterator_range<const Char*>& from, const Char*& fromNext) override;
					boost::optional<bool> doIsBigEndian(const State& decodingState) const override {
						return Encoder::doIsBigEndian(decodingState);
					}
					bool doIsByteOrderMarkEncountered(const State& decodingState) const override {
						return Encoder::doIsByteOrderMarkEncountered(decodingState);
					}
					Result doToUnicode(State& state,
						const boost::iterator_range<Char*>& to, Char*& toNext,
						const boost::iterator_range<const Byte*>& from, const Byte*& fromNext) override;
					const EncodingProperties& properties() const BOOST_NOEXCEPT override {
						return properties_;
					}
				private:
					const EncodingProperties& properties_;
				};

				class Utf8 : public EncoderFactoryImpl {
				public:
					enum EncodingState {SKIPPED_OUTGOING_BYTE_ORDER_MARK, WROTE_BYTE_ORDER_MARK};
					enum DecodingState {SKIPPED_INCOMING_BYTE_ORDER_MARK, READ_BYTE_ORDER_MARK};
					static const std::array<Byte, 3> BYTE_ORDER_MARK;
					Utf8() : EncoderFactoryImpl("UTF-8", fundamental::UTF_8, "Unicode (UTF-8)", 4) {}
				private:
					std::unique_ptr<Encoder> create() const BOOST_NOEXCEPT override {
						return std::unique_ptr<Encoder>(new InternalEncoder<Utf8>(*this));
					}
				};
				const std::array<Byte, 3> Utf8::BYTE_ORDER_MARK = {{0xef, 0xbb, 0xbf}};

				class Utf16 : public EncoderFactoryImpl {
				public:
					enum EncodingState {SKIPPED_OUTGOING_BYTE_ORDER_MARK, WROTE_BIG_ENDIAN_BYTE_ORDER_MARK, WROTE_LITTLE_ENDIAN_BYTE_ORDER_MARK};
					enum DecodingState {SKIPPED_INCOMING_BYTE_ORDER_MARK, READ_BIG_ENDIAN_BYTE_ORDER_MARK, READ_LITTLE_ENDIAN_BYTE_ORDER_MARK};
					static const std::size_t BYTE_ORDER_MARK_SIZE = 2;
					static const std::array<Byte, BYTE_ORDER_MARK_SIZE> BIG_ENDIAN_BYTE_ORDER_MARK, LITTLE_ENDIAN_BYTE_ORDER_MARK;
					static Encoder::Result fromUnicode(bool bigEndian,
						const boost::iterator_range<Byte*>& to, Byte*& toNext, const boost::iterator_range<const Char*>& from, const Char*& fromNext);
					static Encoder::Result toUnicode(const Encoder& encoder, bool bigEndian,
						const boost::iterator_range<Char*>& to, Char*& toNext, const boost::iterator_range<const Byte*>& from, const Byte*& fromNext);
					Utf16() : EncoderFactoryImpl("UTF-16", fundamental::UTF_16, "Unicode (UTF-16)", 2) {}
				private:
					std::unique_ptr<Encoder> create() const BOOST_NOEXCEPT override {
						return std::unique_ptr<Encoder>(new InternalEncoder<Utf16>(*this));
					}
				};
				const std::array<Byte, Utf16::BYTE_ORDER_MARK_SIZE> Utf16::BIG_ENDIAN_BYTE_ORDER_MARK = {{0xfe, 0xff}};
				const std::array<Byte, Utf16::BYTE_ORDER_MARK_SIZE> Utf16::LITTLE_ENDIAN_BYTE_ORDER_MARK = {{0xff, 0xfe}};

				class Utf16BigEndian : public EncoderFactoryImpl {
				public:
					Utf16BigEndian() : EncoderFactoryImpl("UTF-16BE", fundamental::UTF_16BE, "Unicode (UTF-16BE)", 2) {}
				private:
					std::unique_ptr<Encoder> create() const BOOST_NOEXCEPT override {
						return std::unique_ptr<Encoder>(new InternalEncoder<Utf16BigEndian>(*this));
					}
				};

				class Utf16LittleEndian : public EncoderFactoryImpl {
				public:
					Utf16LittleEndian() : EncoderFactoryImpl("UTF-16LE", fundamental::UTF_16LE, "Unicode (UTF-16LE)", 2) {}
				private:
					std::unique_ptr<Encoder> create() const BOOST_NOEXCEPT override {
						return std::unique_ptr<Encoder>(new InternalEncoder<Utf16LittleEndian>(*this));
					}
				};

#ifndef ASCENSION_NO_STANDARD_ENCODINGS
				class Utf7 : public EncoderFactoryImpl {
				public:
					enum ConversionState {BASE64};
					Utf7() : EncoderFactoryImpl("UTF-7", standard::UTF_7, "Unicode (UTF-7)", 8) {}
					static bool isBase64(const Encoder::State& state);
				private:
					std::unique_ptr<Encoder> create() const BOOST_NOEXCEPT override {
						return std::unique_ptr<Encoder>(new InternalEncoder<Utf7>(*this));
					}
				};

				class Utf32 : public EncoderFactoryImpl {
				public:
					enum EncodingState {SKIPPED_OUTGOING_BYTE_ORDER_MARK, WROTE_BIG_ENDIAN_BYTE_ORDER_MARK, WROTE_LITTLE_ENDIAN_BYTE_ORDER_MARK};
					enum DecodingState {SKIPPED_INCOMING_BYTE_ORDER_MARK, READ_BIG_ENDIAN_BYTE_ORDER_MARK, READ_LITTLE_ENDIAN_BYTE_ORDER_MARK};
					static const std::size_t BYTE_ORDER_MARK_SIZE = 4;
					static const std::array<Byte, BYTE_ORDER_MARK_SIZE> BIG_ENDIAN_BYTE_ORDER_MARK, LITTLE_ENDIAN_BYTE_ORDER_MARK;
					static Encoder::Result fromUnicode(bool bigEndian,
						const boost::iterator_range<Byte*>& to, Byte*& toNext, const boost::iterator_range<const Char*>& from, const Char*& fromNext);
					static Encoder::Result toUnicode(const Encoder& encoder, bool bigEndian,
						const boost::iterator_range<Char*>& to, Char*& toNext, const boost::iterator_range<const Byte*>& from, const Byte*& fromNext);
					Utf32() : EncoderFactoryImpl("UTF-32", standard::UTF_32, "Unicode (UTF-32)", 4) {}
				private:
					std::unique_ptr<Encoder> create() const BOOST_NOEXCEPT override {
						return std::unique_ptr<Encoder>(new InternalEncoder<Utf32>(*this));
					}
				};
				const std::array<Byte, Utf32::BYTE_ORDER_MARK_SIZE> Utf32::BIG_ENDIAN_BYTE_ORDER_MARK = {{0xff, 0xff, 0x00, 0x00}};
				const std::array<Byte, Utf32::BYTE_ORDER_MARK_SIZE> Utf32::LITTLE_ENDIAN_BYTE_ORDER_MARK = {{0xfe, 0xff, 0x00, 0x00}};

				class Utf32BigEndian : public EncoderFactoryImpl {
				public:
					Utf32BigEndian() : EncoderFactoryImpl("UTF-32BE", standard::UTF_32BE, "Unicode (UTF-32BE)", 4) {}
				private:
					std::unique_ptr<Encoder> create() const BOOST_NOEXCEPT override {
						return std::unique_ptr<Encoder>(new InternalEncoder<Utf32BigEndian>(*this));
					}
				};

				class Utf32LittleEndian : public EncoderFactoryImpl {
				public:
					Utf32LittleEndian() : EncoderFactoryImpl("UTF-32LE", standard::UTF_32LE, "Unicode (UTF-32LE)", 4) {}
				private:
					std::unique_ptr<Encoder> create() const BOOST_NOEXCEPT override {
						return std::unique_ptr<Encoder>(new InternalEncoder<Utf32LittleEndian>(*this));
					}
				};
#endif // !ASCENSION_NO_STANDARD_ENCODINGS

#ifndef ASCENSION_NO_MINORITY_ENCODINGS
				class Utf5 : public EncoderFactoryImpl {
				public:
					Utf5() : EncoderFactoryImpl("UTF-5", MIB_OTHER, "Unicode (UTF-5)", 6) {}
				private:
					std::unique_ptr<Encoder> create() const BOOST_NOEXCEPT override {
						return std::unique_ptr<Encoder>(new InternalEncoder<Utf5>(*this));
					}
				};
#endif // !ASCENSION_NO_MINORITY_ENCODINGS

				class UnicodeDetector : public EncodingDetector {
				public:
					UnicodeDetector() : EncodingDetector("UnicodeAutoDetect") {}
				private:
					std::tuple<MIBenum, std::string, std::size_t> doDetect(const boost::iterator_range<const Byte*>& bytes) const BOOST_NOEXCEPT override;
				};

				struct Installer {
					Installer() BOOST_NOEXCEPT : UTF_8(std::make_shared<Utf8>()),
							UTF_16BE(std::make_shared<Utf16BigEndian>()), UTF_16LE(std::make_shared<Utf16LittleEndian>()), UTF_16(std::make_shared<Utf16>())
#ifndef ASCENSION_NO_STANDARD_ENCODINGS
							, UTF_7(std::make_shared<Utf7>()),
							UTF_32BE(std::make_shared<Utf32BigEndian>()), UTF_32LE(std::make_shared<Utf32LittleEndian>()), UTF_32(std::make_shared<Utf32>())
#endif // !ASCENSION_NO_STANDARD_ENCODINGS
#ifndef ASCENSION_NO_MINORITY_ENCODINGS
							, UTF_5(std::make_shared<Utf5>())
#endif // !ASCENSION_NO_MINORITY_ENCODINGS
					{
						EncoderRegistry::instance().registerFactory(UTF_8);
						EncoderRegistry::instance().registerFactory(UTF_16BE);
						EncoderRegistry::instance().registerFactory(UTF_16LE);
						EncoderRegistry::instance().registerFactory(UTF_16);
#ifndef ASCENSION_NO_STANDARD_ENCODINGS
						EncoderRegistry::instance().registerFactory(UTF_7);
						EncoderRegistry::instance().registerFactory(UTF_32BE);
						EncoderRegistry::instance().registerFactory(UTF_32LE);
						EncoderRegistry::instance().registerFactory(UTF_32);
#endif // !ASCENSION_NO_STANDARD_ENCODINGS
#ifndef ASCENSION_NO_MINORITY_ENCODINGS
						EncoderRegistry::instance().registerFactory(UTF_5);
#endif // !ASCENSION_NO_MINORITY_ENCODINGS
						EncodingDetector::registerDetector(std::make_shared<UnicodeDetector>());
					}

					const std::shared_ptr<Utf8> UTF_8;
					const std::shared_ptr<Utf16BigEndian> UTF_16BE;
					const std::shared_ptr<Utf16LittleEndian> UTF_16LE;
					const std::shared_ptr<Utf16> UTF_16;
#ifndef ASCENSION_NO_STANDARD_ENCODINGS
					const std::shared_ptr<Utf7> UTF_7;
					const std::shared_ptr<Utf32BigEndian> UTF_32BE;
					const std::shared_ptr<Utf32LittleEndian> UTF_32LE;
					const std::shared_ptr<Utf32> UTF_32;
#endif // !ASCENSION_NO_STANDARD_ENCODINGS
#ifndef ASCENSION_NO_MINORITY_ENCODINGS
					const std::shared_ptr<Utf5> UTF_5;
#endif // !ASCENSION_NO_MINORITY_ENCODINGS
				} installer;

				inline bool eob(const Encoder& encoder) BOOST_NOEXCEPT {
#if 0
					return encoder.options().test(Encoder::END_OF_BUFFER);
#else
					boost::ignore_unused(encoder);
					return true;
#endif
				}

				template<typename Factory>
				inline Encoder::Result _fromUnicode(const Encoder& encoder, Encoder::State& state,
						const boost::iterator_range<Byte*>& to, Byte*& toNext, const boost::iterator_range<const Char*>& from, const Char*& fromNext) {
					auto _to(to);

					// handle byte order mark
					if(state.empty()) {
						if(encoder.writesByteOrderMark()) {
							if(boost::size(to) < Factory::BYTE_ORDER_MARK_SIZE)
								return Encoder::INSUFFICIENT_BUFFER;
							if(encoder.isBigEndianDefault()) {
								boost::copy(Factory::BIG_ENDIAN_BYTE_ORDER_MARK, toNext);
								state = Factory::WROTE_BIG_ENDIAN_BYTE_ORDER_MARK;
							} else {
								boost::copy(Factory::LITTLE_ENDIAN_BYTE_ORDER_MARK, toNext);
								state = Factory::WROTE_LITTLE_ENDIAN_BYTE_ORDER_MARK;
							}
							_to.advance_begin(Factory::BYTE_ORDER_MARK_SIZE);
						} else
							state = Factory::SKIPPED_OUTGOING_BYTE_ORDER_MARK;
					}

					bool bigEndian;
					assert(!state.empty());
					switch(boost::any_cast<typename Factory::EncodingState>(state)) {
						case Factory::SKIPPED_OUTGOING_BYTE_ORDER_MARK:
							bigEndian = encoder.isBigEndianDefault();
							break;
						case Factory::WROTE_BIG_ENDIAN_BYTE_ORDER_MARK:
							bigEndian = true;
							break;
						case Factory::WROTE_LITTLE_ENDIAN_BYTE_ORDER_MARK:
							bigEndian = false;
							break;
						default:
							throw Encoder::BadStateException();
					}
					return Factory::fromUnicode(bigEndian, _to, toNext, from, fromNext);
				}

				template<typename Factory>
				inline Encoder::Result _toUnicode(const Encoder& encoder, Encoder::State& state,
						const boost::iterator_range<Char*>& to, Char*& toNext, const boost::iterator_range<const Byte*>& from, const Byte*& fromNext) {
					auto _from(from);

					// handle byte order mark
					if(state.empty()) {
						if(boost::size(from) >= Factory::BYTE_ORDER_MARK_SIZE) {
							const auto initialBytes(boost::make_iterator_range_n(boost::const_begin(from), Factory::BYTE_ORDER_MARK_SIZE));
							if(boost::equal(initialBytes, Factory::BIG_ENDIAN_BYTE_ORDER_MARK))
								state = Factory::READ_BIG_ENDIAN_BYTE_ORDER_MARK;
							else if(boost::equal(initialBytes, Factory::LITTLE_ENDIAN_BYTE_ORDER_MARK))
								state = Factory::READ_LITTLE_ENDIAN_BYTE_ORDER_MARK;
							if(!state.empty())
								_from.advance_begin(Factory::BYTE_ORDER_MARK_SIZE);
						}
					}

					bool bigEndian;
					if(state.empty()) {
						bigEndian = encoder.isBigEndianDefault();
						state = Factory::SKIPPED_INCOMING_BYTE_ORDER_MARK;
					} else {
						switch(boost::any_cast<typename Factory::DecodingState>(state)) {
							case Factory::SKIPPED_INCOMING_BYTE_ORDER_MARK:
								bigEndian = encoder.isBigEndianDefault();
								break;
							case Factory::READ_BIG_ENDIAN_BYTE_ORDER_MARK:
								bigEndian = true;
								break;
							case Factory::READ_LITTLE_ENDIAN_BYTE_ORDER_MARK:
								bigEndian = false;
								break;
							default:
								throw Encoder::BadStateException();
						}
					}
					return Factory::toUnicode(encoder, bigEndian, to, toNext, _from, fromNext);
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

				inline Byte* writeSurrogatePair(const boost::iterator_range<Byte*>& to, Char high, Char low) {
					if(boost::size(to) < 4)
						return nullptr;
					// 0000 0000  000w wwxx  xxxx yyyy  yyzz zzzz -> 1111 0www  10xx xxxx  10yy yyyy 10zz zzzz
					const CodePoint c = text::surrogates::checkedDecode(high, low);
					auto p = boost::begin(to);
					(*p++) = 0xf0 | mask8Bit((c & 0x001c0000ul) >> 18);
					(*p++) = 0x80 | mask8Bit((c & 0x0003f000ul) >> 12);
					(*p++) = 0x80 | mask8Bit((c & 0x00000fc0ul) >> 6);
					(*p++) = 0x80 | mask8Bit((c & 0x0000003ful) >> 0);
					return p;
				}

				template<> Encoder::Result InternalEncoder<Utf8>::doFromUnicode(State& state,
						const boost::iterator_range<Byte*>& to, Byte*& toNext, const boost::iterator_range<const Char*>& from, const Char*& fromNext) {
					toNext = boost::begin(to);
					fromNext = boost::const_begin(from);

					// handle byte order mark
					if(state.empty()) {
						if(writesByteOrderMark()) {
							if(boost::size(to) < boost::size(Utf8::BYTE_ORDER_MARK))
								return INSUFFICIENT_BUFFER;
							boost::copy(Utf8::BYTE_ORDER_MARK, toNext);
							state = Utf8::WROTE_BYTE_ORDER_MARK;
							std::advance(toNext, boost::size(Utf8::BYTE_ORDER_MARK));
						} else
							state = Utf8::SKIPPED_OUTGOING_BYTE_ORDER_MARK;
					}

					for(; toNext < boost::end(to) && fromNext < boost::const_end(from); ++fromNext) {
						if(*fromNext < 0x0080u)	// 0000 0000  0zzz zzzz -> 0zzz zzzz
							(*toNext++) = mask8Bit(*fromNext);
						else if(*fromNext < 0x0800u) {	// 0000 0yyy  yyzz zzzz -> 110y yyyy  10zz zzzz
							if(toNext + 1 >= boost::end(to))
								break;
							(*toNext++) = 0xc0 | mask8Bit(*fromNext >> 6);
							(*toNext++) = 0x80 | mask8Bit(*fromNext & 0x003fu);
						} else if(text::surrogates::isHighSurrogate(*fromNext)) {
							if(fromNext + 1 == boost::const_end(from))
								return COMPLETED;
							else if(text::surrogates::isLowSurrogate(fromNext[1])) {
								const auto temp = writeSurrogatePair(boost::make_iterator_range(toNext, boost::end(to)), fromNext[0], fromNext[1]);
								if(temp == nullptr)
									break;
								toNext = temp;
								++fromNext;
							} else
								return MALFORMED_INPUT;
						} else {	// xxxx yyyy  yyzz zzzz -> 1110 xxxx  10yy yyyy  10zz zzzz
							if(toNext + 2 >= boost::end(to))
								break;
							(*toNext++) = 0xe0 | mask8Bit((*fromNext & 0xf000u) >> 12);
							(*toNext++) = 0x80 | mask8Bit((*fromNext & 0x0fc0u) >> 6);
							(*toNext++) = 0x80 | mask8Bit((*fromNext & 0x003fu) >> 0);
						}
					}
					return (fromNext == boost::const_end(from)) ? COMPLETED : INSUFFICIENT_BUFFER;
				}

				template<> boost::optional<bool> InternalEncoder<Utf8>::doIsBigEndian(const State& decodingState) const {
					if(decodingState.empty())
						return boost::none;
					switch(boost::any_cast<Utf8::DecodingState>(decodingState)) {
						case Utf8::SKIPPED_INCOMING_BYTE_ORDER_MARK:
						case Utf8::READ_BYTE_ORDER_MARK:
							return boost::none;
						default:
							throw BadStateException();
					}
				}

				template<> bool InternalEncoder<Utf8>::doIsByteOrderMarkEncountered(const State& decodingState) const {
					if(decodingState.empty())
						return false;
					switch(boost::any_cast<Utf8::DecodingState>(decodingState)) {
						case Utf8::SKIPPED_INCOMING_BYTE_ORDER_MARK:
							return false;
						case Utf8::READ_BYTE_ORDER_MARK:
							return true;
						default:
							throw BadStateException();
					}
				}

				template<> Encoder::Result InternalEncoder<Utf8>::doToUnicode(State& state,
						const boost::iterator_range<Char*>& to, Char*& toNext, const boost::iterator_range<const Byte*>& from, const Byte*& fromNext) {
					toNext = boost::begin(to);
					fromNext = boost::const_begin(from);

					// handle byte order mark
					if(state.empty()) {
						if(boost::size(from) >= boost::size(Utf8::BYTE_ORDER_MARK)
								&& boost::equal(boost::make_iterator_range_n(fromNext, boost::size(Utf8::BYTE_ORDER_MARK)), Utf8::BYTE_ORDER_MARK)) {
							state = Utf8::READ_BYTE_ORDER_MARK;
							std::advance(fromNext, boost::size(Utf8::BYTE_ORDER_MARK));
						} else
							state = Utf8::SKIPPED_INCOMING_BYTE_ORDER_MARK;
					}

					while(toNext < boost::end(to) && fromNext < boost::const_end(from)) {
						if(*fromNext < 0x80)
							*(toNext++) = *(fromNext++);
						else {
							const Byte v = UTF8_WELL_FORMED_FIRST_BYTES[*fromNext - 0x80];
							// check the source buffer length
							std::ptrdiff_t bytes = (v >> 4);
							if(std::distance(fromNext, boost::const_end(from)) < bytes)
								return COMPLETED;
							// check the second byte
							switch(v & 0x0f) {
								case 1:
								case 3:
								case 5:
								case 7:
									if(fromNext[1] < 0x80 || from[1] > 0xbf)
										bytes = 0;
									break;
								case 2:
									if(fromNext[1] < 0xa0 || from[1] > 0xbf)
										bytes = 0;
									break;
								case 4:
									if(fromNext[1] < 0x80 || from[1] > 0x9f)
										bytes = 0;
									break;
								case 6:
									if(fromNext[1] < 0x90 || from[1] > 0xbf)
										bytes = 0;
									break;
								case 8:
									if(fromNext[1] < 0x80 || from[1] > 0x8f)
										bytes = 0;
									break;
							}
							// check the third byte
							if(bytes >= 3 && (fromNext[2] < 0x80 || fromNext[2] > 0xbf))
								bytes = 0;
							// check the forth byte
							if(bytes >= 4 && (fromNext[3] < 0x80 || fromNext[3] > 0xbf))
								bytes = 0;

							if(bytes == 0)
								return MALFORMED_INPUT;

							// decode
							CodePoint cp;
							assert(bytes >= 2 && bytes <= 4);
							switch(bytes) {
								case 2:	// 110y yyyy  10zz zzzz -> 0000 0yyy yyzz zzzz
									cp = ((fromNext[0] & 0x1f) << 6) | ((fromNext[1] & 0x3f) << 0);
									break;
								case 3:	// 1110 xxxx  10yy yyyy  10zz zzzz -> xxxx yyyy yyzz zzzz
									cp = ((fromNext[0] & 0x0f) << 12) | ((fromNext[1] & 0x3f) << 6) | ((fromNext[2] & 0x3f) << 0);
									break;
								case 4:	// 1111 0www  10xx xxxx  10yy yyyy  10zz zzzz -> 0000 0000 000w wwxx xxxx yyyy yyzz zzzz
									cp = ((fromNext[0] & 0x07) << 18) | ((fromNext[1] & 0x3f) << 12)
										| ((fromNext[2] & 0x3f) << 6) | ((fromNext[3] & 0x3f) << 0);
									break;
							}

							if(std::distance(toNext, boost::end(to)) == 1 && text::surrogates::isSupplemental(cp))
								return INSUFFICIENT_BUFFER;
							text::utf::encode(cp, toNext);
							std::advance(toNext, text::surrogates::isSupplemental(cp) ? 2 : 1);
							std::advance(fromNext, bytes);
						}

					}
					return (fromNext == boost::const_end(from)) ? COMPLETED : INSUFFICIENT_BUFFER;
				}


				// UTF-16 /////////////////////////////////////////////////////////////////////////////////////////////

				Encoder::Result Utf16::fromUnicode(bool bigEndian,
						const boost::iterator_range<Byte*>& to, Byte*& toNext, const boost::iterator_range<const Char*>& from, const Char*& fromNext) {
					static const std::array<Char, 2> BIG_ENDIAN_MASKS = {{0xff00u, 0x00ffu}}, LITTLE_ENDIAN_MASKS = {{0x00ffu, 0xff00u}};
					static const std::array<int, 2> BIG_ENDIAN_SHIFTS = {{8, 0}}, LITTLE_ENDIAN_SHIFTS = {{0, 8}};
					const auto& masks = bigEndian ? BIG_ENDIAN_MASKS : LITTLE_ENDIAN_MASKS;
					const auto& shifts = bigEndian ? BIG_ENDIAN_SHIFTS : LITTLE_ENDIAN_SHIFTS;
					toNext = boost::begin(to);
					fromNext = boost::const_begin(from);
					for(; std::distance(toNext, boost::end(to)) > 1 && fromNext < boost::const_end(from); ++fromNext) {
						*(toNext++) = static_cast<Byte>((*fromNext & std::get<0>(masks)) >> std::get<0>(shifts));
						*(toNext++) = static_cast<Byte>((*fromNext & std::get<1>(masks)) >> std::get<1>(shifts));
					}
					return (fromNext == boost::const_end(from)) ? Encoder::COMPLETED : Encoder::INSUFFICIENT_BUFFER;
				}

				Encoder::Result Utf16::toUnicode(const Encoder&, bool bigEndian,
						const boost::iterator_range<Char*>& to, Char*& toNext, const boost::iterator_range<const Byte*>& from, const Byte*& fromNext) {
					static const std::array<int, 2> BIG_ENDIAN_SHIFTS = {{0, 8}}, LITTLE_ENDIAN_SHIFTS = {{8, 0}};
					const auto& shifts = bigEndian ? BIG_ENDIAN_SHIFTS : LITTLE_ENDIAN_SHIFTS;
					toNext = boost::begin(to);
					fromNext = boost::const_begin(from);
					for(; toNext < boost::end(to) && std::distance(fromNext, boost::const_end(from)) > 1; std::advance(fromNext, 2))
						*(toNext++) = maskUCS2(fromNext[0] << std::get<0>(shifts)) | maskUCS2(fromNext[1] << std::get<1>(shifts));
					if(fromNext == boost::const_end(from))
						return Encoder::COMPLETED;
					else
						return (std::distance(toNext, boost::end(to)) <= 1) ? Encoder::INSUFFICIENT_BUFFER : Encoder::UNMAPPABLE_CHARACTER;
				}
				
				template<> Encoder::Result InternalEncoder<Utf16>::doFromUnicode(State& state,
						const boost::iterator_range<Byte*>& to, Byte*& toNext, const boost::iterator_range<const Char*>& from, const Char*& fromNext) {
					return _fromUnicode<Utf16>(*this, state, to, toNext, from, fromNext);
				}

				template<> boost::optional<bool> InternalEncoder<Utf16>::doIsBigEndian(const State& decodingState) const {
					if(decodingState.empty())
						return boost::none;
					switch(boost::any_cast<Utf16::DecodingState>(decodingState)) {
						case Utf16::SKIPPED_INCOMING_BYTE_ORDER_MARK:
							return boost::none;
						case Utf16::READ_BIG_ENDIAN_BYTE_ORDER_MARK:
							return true;
						case Utf16::READ_LITTLE_ENDIAN_BYTE_ORDER_MARK:
							return false;
						default:
							throw BadStateException();
					}
				}

				template<> bool InternalEncoder<Utf16>::doIsByteOrderMarkEncountered(const State& decodingState) const {
					if(decodingState.empty())
						return false;
					switch(boost::any_cast<Utf16::DecodingState>(decodingState)) {
						case Utf16::SKIPPED_INCOMING_BYTE_ORDER_MARK:
							return false;
						case Utf16::READ_BIG_ENDIAN_BYTE_ORDER_MARK:
						case Utf16::READ_LITTLE_ENDIAN_BYTE_ORDER_MARK:
							return true;
						default:
							throw BadStateException();
					}
				}

				template<> Encoder::Result InternalEncoder<Utf16>::doToUnicode(State& state,
						const boost::iterator_range<Char*>& to, Char*& toNext, const boost::iterator_range<const Byte*>& from, const Byte*& fromNext) {
					return _toUnicode<Utf16>(*this, state, to, toNext, from, fromNext);
				}


				// UTF-16BE ///////////////////////////////////////////////////////////////////////////////////////////

				template<> Encoder::Result InternalEncoder<Utf16BigEndian>::doFromUnicode(State&,
						const boost::iterator_range<Byte*>& to, Byte*& toNext, const boost::iterator_range<const Char*>& from, const Char*& fromNext) {
					return Utf16::fromUnicode(true, to, toNext, from, fromNext);
				}

				template<> Encoder::Result InternalEncoder<Utf16BigEndian>::doToUnicode(State&,
						const boost::iterator_range<Char*>& to, Char*& toNext, const boost::iterator_range<const Byte*>& from, const Byte*& fromNext) {
					return Utf16::toUnicode(*this, true, to, toNext, from, fromNext);
				}


				// UTF-16LE ///////////////////////////////////////////////////////////////////////////////////////////

				template<> Encoder::Result InternalEncoder<Utf16LittleEndian>::doFromUnicode(State&,
						const boost::iterator_range<Byte*>& to, Byte*& toNext, const boost::iterator_range<const Char*>& from, const Char*& fromNext) {
					return Utf16::fromUnicode(false, to, toNext, from, fromNext);
				}

				template<> Encoder::Result InternalEncoder<Utf16LittleEndian>::doToUnicode(State&,
						const boost::iterator_range<Char*>& to, Char*& toNext, const boost::iterator_range<const Byte*>& from, const Byte*& fromNext) {
					return Utf16::toUnicode(*this, false, to, toNext, from, fromNext);
				}


#ifndef ASCENSION_NO_STANDARD_ENCODINGS

				// UTF-32 /////////////////////////////////////////////////////////////////////////////////////////////

				Encoder::Result Utf32::fromUnicode(bool bigEndian,
						const boost::iterator_range<Byte*>& to, Byte*& toNext, const boost::iterator_range<const Char*>& from, const Char*& fromNext) {
					static const std::array<CodePoint, 4> BIG_ENDIAN_MASKS = {{0xff000000ul, 0x00ff0000ul, 0x0000ff00ul, 0x000000fful}};
					static const std::array<CodePoint, 4> LITTLE_ENDIAN_MASKS = {{0x000000fful, 0x0000ff00ul, 0x00ff0000ul, 0xff000000ul}};
					static const std::array<int, 4> BIG_ENDIAN_SHIFTS = {{24, 16, 8, 0}};
					static const std::array<int, 4> LITTLE_ENDIAN_SHIFTS = {{0, 8, 16, 24}};
					const auto& masks = bigEndian ? BIG_ENDIAN_MASKS : LITTLE_ENDIAN_MASKS;
					const auto& shifts = bigEndian ? BIG_ENDIAN_SHIFTS : LITTLE_ENDIAN_SHIFTS;
					toNext = boost::begin(to);
					fromNext = boost::const_begin(from);
					for(; std::distance(toNext, boost::end(to)) > 3 && fromNext < boost::const_end(from); ++fromNext) {
						const CodePoint c = text::utf::decodeFirst(fromNext, boost::const_end(from));
						if(!text::isScalarValue(c)) {
							if(text::surrogates::isHighSurrogate(c) && std::next(fromNext, 1) == boost::const_end(from))	// low surrogate may appear immediately
								return Encoder::COMPLETED;
							return Encoder::MALFORMED_INPUT;
						}
						*(toNext++) = mask8Bit((c & std::get<0>(masks)) >> std::get<0>(shifts));
						*(toNext++) = mask8Bit((c & std::get<1>(masks)) >> std::get<1>(shifts));
						*(toNext++) = mask8Bit((c & std::get<2>(masks)) >> std::get<2>(shifts));
						*(toNext++) = mask8Bit((c & std::get<3>(masks)) >> std::get<3>(shifts));
						if(text::surrogates::isSupplemental(c))
							++fromNext;
					}
					return (fromNext == boost::const_end(from)) ? Encoder::COMPLETED : Encoder::INSUFFICIENT_BUFFER;
				}

				Encoder::Result Utf32::toUnicode(const Encoder& encoder, bool bigEndian,
						const boost::iterator_range<Char*>& to, Char*& toNext, const boost::iterator_range<const Byte*>& from, const Byte*& fromNext) {
					static const std::array<int, 4> BIG_ENDIAN_SHIFTS = {{24, 16, 8, 0}};
					static const std::array<int, 4> LITTLE_ENDIAN_SHIFTS = {{0, 8, 16, 24}};
					const auto& shifts = bigEndian ? BIG_ENDIAN_SHIFTS : LITTLE_ENDIAN_SHIFTS;
					toNext = boost::begin(to);
					fromNext = boost::const_begin(from);
					for(; toNext < boost::end(to) && std::distance(fromNext, boost::const_end(from)) > 3; std::advance(fromNext, 4)) {
						const CodePoint c =
							(fromNext[0] << std::get<0>(shifts)) + (fromNext[1] << std::get<1>(shifts))
							+ (fromNext[2] << std::get<2>(shifts)) + (fromNext[3] << std::get<3>(shifts));
						if(text::isValidCodePoint(c)) {
							if(encoder.substitutionPolicy() == Encoder::REPLACE_UNMAPPABLE_CHARACTERS)
								*(toNext++) = text::REPLACEMENT_CHARACTER;
							else if(encoder.substitutionPolicy() != Encoder::IGNORE_UNMAPPABLE_CHARACTERS)
								return Encoder::UNMAPPABLE_CHARACTER;
						} else
							toNext += text::utf::encode(c, toNext);
					}
					return (fromNext == boost::const_end(from)) ? Encoder::COMPLETED : Encoder::INSUFFICIENT_BUFFER;
				}
				
				template<> Encoder::Result InternalEncoder<Utf32>::doFromUnicode(State& state,
						const boost::iterator_range<Byte*>& to, Byte*& toNext, const boost::iterator_range<const Char*>& from, const Char*& fromNext) {
					return _fromUnicode<Utf32>(*this, state, to, toNext, from, fromNext);
				}

				template<> boost::optional<bool> InternalEncoder<Utf32>::doIsBigEndian(const State& decodingState) const {
					if(decodingState.empty())
						return boost::none;
					switch(boost::any_cast<Utf32::DecodingState>(decodingState)) {
						case Utf32::SKIPPED_INCOMING_BYTE_ORDER_MARK:
							return boost::none;
						case Utf32::READ_BIG_ENDIAN_BYTE_ORDER_MARK:
							return true;
						case Utf32::READ_LITTLE_ENDIAN_BYTE_ORDER_MARK:
							return false;
						default:
							throw BadStateException();
					}
				}

				template<> bool InternalEncoder<Utf32>::doIsByteOrderMarkEncountered(const State& decodingState) const {
					if(decodingState.empty())
						return false;
					switch(boost::any_cast<Utf16::DecodingState>(decodingState)) {
						case Utf32::SKIPPED_INCOMING_BYTE_ORDER_MARK:
							return false;
						case Utf32::READ_BIG_ENDIAN_BYTE_ORDER_MARK:
						case Utf32::READ_LITTLE_ENDIAN_BYTE_ORDER_MARK:
							return true;
						default:
							throw BadStateException();
					}
				}

				template<> Encoder::Result InternalEncoder<Utf32>::doToUnicode(State& state,
						const boost::iterator_range<Char*>& to, Char*& toNext, const boost::iterator_range<const Byte*>& from, const Byte*& fromNext) {
					return _toUnicode<Utf32>(*this, state, to, toNext, from, fromNext);
				}


				// UTF-32BE ///////////////////////////////////////////////////////////////////////////////////////////

				template<> Encoder::Result InternalEncoder<Utf32BigEndian>::doFromUnicode(State&,
						const boost::iterator_range<Byte*>& to, Byte*& toNext, const boost::iterator_range<const Char*>& from, const Char*& fromNext) {
					return Utf32::fromUnicode(true, to, toNext, from, fromNext);
				}

				template<> Encoder::Result InternalEncoder<Utf32BigEndian>::doToUnicode(State&,
						const boost::iterator_range<Char*>& to, Char*& toNext, const boost::iterator_range<const Byte*>& from, const Byte*& fromNext) {
					return Utf32::toUnicode(*this, true, to, toNext, from, fromNext);
				}


				// UTF-32LE ///////////////////////////////////////////////////////////////////////////////////////////

				template<> Encoder::Result InternalEncoder<Utf32LittleEndian>::doFromUnicode(State&,
						const boost::iterator_range<Byte*>& to, Byte*& toNext, const boost::iterator_range<const Char*>& from, const Char*& fromNext) {
					return Utf32::fromUnicode(false, to, toNext, from, fromNext);
				}

				template<> Encoder::Result InternalEncoder<Utf32LittleEndian>::doToUnicode(State&,
						const boost::iterator_range<Char*>& to, Char*& toNext, const boost::iterator_range<const Byte*>& from, const Byte*& fromNext) {
					return Utf32::toUnicode(*this, false, to, toNext, from, fromNext);
				}


				// UTF-7 //////////////////////////////////////////////////////////////////////////////////////////////

				bool Utf7::isBase64(const Encoder::State& state) {
					if(state.empty())
						return false;
					else if(const auto* const st = boost::any_cast<ConversionState>(&state)) {
						if(*st == BASE64)
							return true;
					}
					throw Encoder::BadStateException();
				}

				template<> Encoder::Result InternalEncoder<Utf7>::doFromUnicode(State& state,
						const boost::iterator_range<Byte*>& to, Byte*& toNext, const boost::iterator_range<const Char*>& from, const Char*& fromNext) {
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

					bool base64 = Utf7::isBase64(state);
					toNext = boost::begin(to);
					fromNext = boost::const_begin(from);
					for(; toNext < boost::end(to) && fromNext < boost::const_end(from); ++fromNext) {
						const Byte klass = (*fromNext < 0x80) ? SET_D[*fromNext] : 0;
						if((klass & 1) == 1) {
							// encode directly (ascension puts '-' explicitly even if klass is 3)
							if(base64) {
								*toNext = '-';
								base64 = false;
								if(++toNext == boost::end(to))
									break;	// the destination buffer is insufficient
							}
							*(toNext++) = mask8Bit(*fromNext);
						} else if(klass == 2) {
							// '+' => '+-'
							if(std::distance(toNext, boost::end(to)) == 1)
								break;	// the destination buffer is insufficient
							*(toNext++) = '+';
							*(toNext++) = '-';
						} else {
							// modified BASE64 encode
							if(!base64) {
								*toNext = '+';
								base64 = true;
								if(++toNext == boost::end(to))
									break;	// the destination buffer is insufficient
							}
							// first, determine how many source characters can be encoded
							std::ptrdiff_t encodables = 1;
							if(std::distance(fromNext, boost::const_end(from)) > 1 && (fromNext[1] >= 0x80 || SET_D[fromNext[1]] == 0)) {
								++encodables;
								if(std::distance(fromNext, boost::const_end(from)) > 2 && (fromNext[2] >= 0x80 || SET_D[fromNext[2]] == 0))
									++encodables;
							}
							// check the size of the destination buffer
							switch(encodables) {
								case 3:
									if(std::distance(toNext, boost::end(to)) <= 8)
										encodables = 0;
									break;
								case 2:
									if(std::distance(toNext, boost::end(to)) <= 6)
										encodables = 0;
									break;
								case 1:
									if(std::distance(toNext, boost::end(to)) <= 3)
										encodables = 0;
									break;
							}
							if(encodables == 0)
								break;	// the destination buffer is insufficient

							// encode
							const Char utf16[3] = {fromNext[0], (encodables > 1) ? fromNext[1] : '\0', (encodables > 2) ? fromNext[2] : '\0'};
							*(toNext++) = BASE64[utf16[0] >> 10];
							*(toNext++) = BASE64[(utf16[0] >> 4) & 0x3f];
							*(toNext++) = BASE64[(utf16[0] << 2 | utf16[1] >> 14) & 0x3f];
							if(encodables >= 2) {
								*(toNext++) = BASE64[(utf16[1] >> 8) & 0x3f];
								*(toNext++) = BASE64[(utf16[1] >> 2) & 0x3f];
								*(toNext++) = BASE64[(utf16[1] << 4 | utf16[2] >> 12) & 0x3f];
								if(encodables >= 3) {
									*(toNext++) = BASE64[(utf16[2] >> 6) & 0x3f];
									*(toNext++) = BASE64[utf16[2] & 0x3f];
								}
							}
							std::advance(fromNext, encodables - 1);
						}
					}
					if(fromNext == boost::const_end(from) && eob(*this) && toNext != boost::end(to))
						*(toNext++) = '-';
					if(base64)
						state = Utf7::BASE64;
					else
						state = boost::any();
					return (fromNext == boost::const_end(from)) ? COMPLETED : INSUFFICIENT_BUFFER;
				}
		
				template<> Encoder::Result InternalEncoder<Utf7>::doToUnicode(State& state,
						const boost::iterator_range<Char*>& to, Char*& toNext, const boost::iterator_range<const Byte*>& from, const Byte*& fromNext) {
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

					bool base64 = Utf7::isBase64(state);
					toNext = boost::begin(to);
					fromNext = boost::const_begin(from);
					while(toNext < boost::end(to) && fromNext < boost::const_end(from)) {
						if(*fromNext >= 0x80 || SET_B[*fromNext] == 0)
							return MALFORMED_INPUT;
						const Byte klass = SET_B[*fromNext];
						if(klass == 2) {
							// '+'
							if(std::distance(fromNext, boost::const_end(from)) == 1) {	// the input is terminated by '+'...
								if(eob(*this))
									return COMPLETED;
								else {
									base64 = true;
									++fromNext;
									break;
								}
							} else if(fromNext[1] == '-') {
								// '+-' => '+'
								*(toNext++) = L'+';
								fromNext += 2;
							} else {
								base64 = true;	// introduce modified BASE64 sequence
								++fromNext;
							}
						} else if(klass == 3) {
							(*toNext++) = (*fromNext++);
							base64 = false;	// terminate modified BASE64 implicitly
						} else if(klass == 4) {
							// '-'
							if(base64)
								base64 = false;
							else
								return MALFORMED_INPUT;	// '-' can't appear here
								// ...this can't handle '-' appeared at the exact beginning of the input buffer
							++fromNext;
						} else {
							// first, determine how many bytes can be decoded
							std::ptrdiff_t decodables = 1;
							for(const std::ptrdiff_t minimum = std::min<std::ptrdiff_t>(std::distance(fromNext, boost::const_end(from)), 8); decodables < minimum; ++decodables) {
								if(BASE64[fromNext[decodables]] == 0xff)
									break;
							}
							// check the size of the destination buffer
							switch(decodables) {
								case 8: if(std::distance(toNext, boost::end(to)) <= 2) decodables = 0; break;
								case 6: if(std::distance(toNext, boost::end(to)) <= 1) decodables = 0; break;
								case 3: break;
								default:
									return MALFORMED_INPUT;	// invalid modified BASE64 sequence
							}
							if(decodables == 0)
								break;	// the destination buffer is insufficient

							// decode
							*(toNext++) = BASE64[fromNext[0]] << 10 | BASE64[fromNext[1]] << 4 | BASE64[fromNext[2]] >> 2;
							if(decodables >= 6) {
								*(toNext++) = maskUCS2(BASE64[fromNext[2]] << 14) | BASE64[fromNext[3]] << 8 | BASE64[fromNext[4]] << 2 | BASE64[fromNext[5]] >> 4;
								if(decodables >= 8)
									*(toNext++) = BASE64[fromNext[5]] << 12 | BASE64[fromNext[6]] << 6 | BASE64[fromNext[7]];
							}
							std::advance(fromNext, decodables);			
						}
					}
					return (fromNext == boost::const_end(from)) ? COMPLETED : INSUFFICIENT_BUFFER;
				}

#endif // !ASCENSION_NO_STANDARD_ENCODINGS

#ifndef ASCENSION_NO_MINORITY_ENCODINGS

				// UTF-5 //////////////////////////////////////////////////////////////////////////////////////////////

				/**
				 * Transcodes the given UTF-5 sequence into a Unicode character.
				 * @param s The UTF-5 byte sequence
				 * @param[out] cp Code point of the decoded character
				 * @return The end of the eaten subsequence, or @c null
				 */
				inline const Byte* decodeUtf5Character(const boost::iterator_range<const Byte*>& s, CodePoint& cp) BOOST_NOEXCEPT {
					auto p = boost::const_begin(s);
					if(*p < 'G' || *p > 'V')
						return nullptr;
					cp = *p - 'G';
					for(++p; p < boost::const_end(s); ++p) {
						if(*p >= '0' && *p <= '9') {
							cp <<= 4;
							cp |= *p - '0';
						} else if(*p >= 'A' && *p <= 'F'){
							cp <<= 4;
							cp |= *p - 'A' + 0x0a;
						} else
							break;
					}
					return p;
				}

				/**
				 * Transcodes the given Unicode character into UTF-5.
				 * @param s The Unicode character sequence
				 * @param[out] to The beginning of the destination buffer
				 * @return The end of the eaten subsequence
				 */
				inline Byte* encodeUtf5Character(const boost::iterator_range<const Char*>& s, Byte* to) {
#define D2C(n) (mask8Bit(n) < 0x0a) ? (mask8Bit(n) + '0') : (mask8Bit(n) - 0x0a + 'A')

					const CodePoint cp = text::utf::decodeFirst(s);
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

				template<> Encoder::Result InternalEncoder<Utf5>::doFromUnicode(State&,
						const boost::iterator_range<Byte*>& to, Byte*& toNext, const boost::iterator_range<const Char*>& from, const Char*& fromNext) {
					Byte temp[8];
					Byte* e;
					toNext = boost::begin(to);
					fromNext = boost::const_begin(from);
					for(; toNext < boost::end(to) && fromNext < boost::const_end(from); ++fromNext) {
						e = encodeUtf5Character(boost::make_iterator_range(fromNext, boost::const_end(from)), temp);
						if(e == temp) {
							if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTERS)
								*(toNext++) = properties().substitutionCharacter();
							else if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTERS)
								continue;
							else
								return UNMAPPABLE_CHARACTER;
						} else if(std::distance(temp, e) > std::distance(toNext, boost::end(to)))
							return INSUFFICIENT_BUFFER;
						else {
							boost::copy(boost::make_iterator_range(temp, e), toNext);
							std::advance(toNext, e - temp);
							if(std::distance(temp, e) >= 5)
								++fromNext;
						}
					}
					return (fromNext == boost::const_end(from)) ? COMPLETED : INSUFFICIENT_BUFFER;
				}

				template<> Encoder::Result InternalEncoder<Utf5>::doToUnicode(State&,
						const boost::iterator_range<Char*>& to, Char*& toNext, const boost::iterator_range<const Byte*>& from, const Byte*& fromNext) {
					const Byte* e;
					CodePoint cp;
					toNext = boost::begin(to);
					fromNext = boost::const_begin(from);
					while(toNext < boost::end(to) && fromNext < boost::const_end(from)) {
						e = decodeUtf5Character(boost::make_iterator_range(fromNext, boost::const_end(from)), cp);
						if(e == fromNext)
							return MALFORMED_INPUT;
						else if(!text::isValidCodePoint(cp)) {
							if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTERS) {
								cp = text::REPLACEMENT_CHARACTER;
								if(e == fromNext)
									e = std::next(fromNext, 1);
							} else if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTERS) {
								++fromNext;
								continue;
							} else
								return UNMAPPABLE_CHARACTER;
						}
						if(std::next(toNext, 1) == boost::end(to) && text::surrogates::isSupplemental(cp))
							return INSUFFICIENT_BUFFER;
						fromNext = e;
						text::utf::encode(cp, toNext);
						std::advance(toNext, text::surrogates::isSupplemental(cp) ? 2 : 1);
					}
					return (fromNext == boost::const_end(from)) ? COMPLETED : INSUFFICIENT_BUFFER;
				}
#endif // !ASCENSION_NO_MINORITY_ENCODINGS

				inline std::size_t maybeUtf8(const boost::iterator_range<const Byte*>& bytes) BOOST_NOEXCEPT {
					auto p(boost::const_begin(bytes));
					while(p < boost::const_end(bytes)) {
						if(*p == 0xc0 || *p == 0xc1 || *p >= 0xf5)
							break;
						++p;
					}
					return p - boost::const_begin(bytes);
				}

				/// @see EncodingDetector#doDetect
				std::tuple<MIBenum, std::string, std::size_t> UnicodeDetector::doDetect(const boost::iterator_range<const Byte*>& bytes) const BOOST_NOEXCEPT {
					std::shared_ptr<const EncodingProperties> result;
					// first, test Unicode byte order marks
					if(boost::size(bytes) >= boost::size(Utf8::BYTE_ORDER_MARK)
							&& boost::equal(boost::make_iterator_range_n(boost::const_begin(bytes), boost::size(Utf8::BYTE_ORDER_MARK)), Utf8::BYTE_ORDER_MARK))
						result = installer.UTF_8;
					else if(boost::size(bytes) >= Utf16::BYTE_ORDER_MARK_SIZE) {
						const auto initialBytes(boost::make_iterator_range_n(boost::const_begin(bytes), Utf16::BYTE_ORDER_MARK_SIZE));
						static_assert(std::is_same<decltype(Utf16::BIG_ENDIAN_BYTE_ORDER_MARK), decltype(Utf16::LITTLE_ENDIAN_BYTE_ORDER_MARK)>::value, "");
						if(boost::equal(initialBytes, Utf16::BIG_ENDIAN_BYTE_ORDER_MARK))
							result = installer.UTF_16BE;
						else if(boost::equal(initialBytes, Utf16::LITTLE_ENDIAN_BYTE_ORDER_MARK))
							result = installer.UTF_16LE;
#ifndef ASCENSION_NO_STANDARD_ENCODINGS
					} else if(boost::size(bytes) >= Utf32::BYTE_ORDER_MARK_SIZE) {
						const auto initialBytes(boost::make_iterator_range_n(boost::const_begin(bytes), Utf32::BYTE_ORDER_MARK_SIZE));
						static_assert(std::is_same<decltype(Utf32::BIG_ENDIAN_BYTE_ORDER_MARK), decltype(Utf32::LITTLE_ENDIAN_BYTE_ORDER_MARK)>::value, "");
						if(boost::equal(initialBytes, Utf32::BIG_ENDIAN_BYTE_ORDER_MARK))
							result = installer.UTF_32BE;
						else if(boost::equal(initialBytes, Utf32::LITTLE_ENDIAN_BYTE_ORDER_MARK))
							result = installer.UTF_32LE;
#endif // !ASCENSION_NO_STANDARD_ENCODINGS
					}

					std::size_t score;
					if(result != nullptr)
						score = boost::size(bytes);
					else {
						// force into UTF-8
						result = installer.UTF_8;
						score = maybeUtf8(bytes);
					}
					return std::make_tuple(result->mibEnum(), result->name(), score);
				}
			}
		}
	}
}
