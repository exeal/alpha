/**
 * @file korean.cpp
 * This file implements the following encodings for Korean:
 * - EUC-KR
 * - UHC
 * - JOHAB
 * - ISO-2022-KR
 * @author exeal
 * @date 2007-2012, 2014
 */

#include <ascension/corelib/encoding/encoder.hpp>
#include <ascension/corelib/encoding/encoder-implementation.hpp>
#include <ascension/corelib/text/character.hpp>	// text.REPLACEMENT_CHARACTER
#include <cstring>								// std.memcmp, std.memcpy

namespace ascension {
	namespace encoding {
		namespace implementation {
			namespace dbcs {
				namespace {
					const Char** const UCS_TO_UHC[256] = {
#include "generated/ucs-to-windows-949.dat"
					};
					const std::uint16_t** const UHC_TO_UCS[256] = {
#include "generated/windows-949-to-ucs.dat"
					};

					template<typename Factory>
					class InternalEncoder : public Encoder {
					public:
						explicit InternalEncoder(const Factory& factory) BOOST_NOEXCEPT : properties_(factory), encodingState_(0), decodingState_(0) {}
					private:
						ConversionResult doFromUnicode(Byte* to, Byte* toEnd, Byte*& toNext,
							const Char* from, const Char* fromEnd, const Char*& fromNext) override;
						ConversionResult doToUnicode(Char* to, Char* toEnd, Char*& toNext,
							const Byte* from, const Byte* fromEnd, const Byte*& fromNext) override;
						const EncodingProperties& properties() const override BOOST_NOEXCEPT {return properties_;}
						Encoder& resetDecodingState() override BOOST_NOEXCEPT {
							return (decodingState_ = 0), *this;
						}
						Encoder& resetEncodingState() override BOOST_NOEXCEPT {
							return (encodingState_ = 0), *this;
						}
					private:
						const EncodingProperties& properties_;
						Byte encodingState_, decodingState_;
					};

					class Uhc : public EncoderFactoryImpl {
					public:
						Uhc() BOOST_NOEXCEPT : EncoderFactoryImpl("UHC", standard::UHC, "Korean (UHC)", 2, 1,
							"KS_C_5601-1987|iso-ir-149|KS_C_5601-1989|KSC_5601|korean|csKSC56011987"
							"\0ibm-1363|5601|cp1363|ksc|windows-949|ibm-1363_VSUB_VPUA|ms949|ibm-1363_P11B-1998|windows-949-2000", 0x3f) {}
					private:
						std::unique_ptr<Encoder> create() const override BOOST_NOEXCEPT {
							return std::unique_ptr<Encoder>(new InternalEncoder<Uhc>(*this));
						}
					};

					class EucKr : public EncoderFactoryImpl {
					public:
						EucKr() BOOST_NOEXCEPT : EncoderFactoryImpl("EUC-KR", standard::EUC_KR, "Korean (EUC-KR)", 2, 1,
							"csEUCKR" "\0ibm-970KS_C_5601-1987|windows-51949|ibm-eucKR|KSC_5601|5601|cp970|970|ibm-970-VPUA|ibm-970_P110_P110-2006_U2") {}
					private:
						std::unique_ptr<Encoder> create() const override BOOST_NOEXCEPT {
							return std::unique_ptr<Encoder>(new InternalEncoder<EucKr>(*this));
						}
					};

					class Iso2022Kr : public EncoderFactoryImpl {
					public:
						Iso2022Kr() BOOST_NOEXCEPT : EncoderFactoryImpl("ISO-2022-KR", standard::ISO_2022_KR, "Korean (ISO-2022-KR)", 7, 1, "csISO2022KR") {}
					private:
						std::unique_ptr<Encoder> create() const override BOOST_NOEXCEPT {
							return std::unique_ptr<Encoder>(new InternalEncoder<Iso2022Kr>(*this));
						}
					} iso2022kr;

					struct Installer {
						Installer() {
							EncoderRegistry::instance().registerFactory(std::make_shared<const Uhc>());
							EncoderRegistry::instance().registerFactory(std::make_shared<const EucKr>());
							EncoderRegistry::instance().registerFactory(std::make_shared<const Iso2022Kr>());
						}
					} installer;


					// UHC ////////////////////////////////////////////////////////////////////////////////////////////

					template<> Encoder::ConversionResult InternalEncoder<Uhc>::doFromUnicode(
							Byte* to, Byte* toEnd, Byte*& toNext, const Char* from, const Char* fromEnd, const Char*& fromNext) {
						for(; to < toEnd && from < fromEnd; ++from) {
							if(*from < 0x80)
								*(to++) = mask8Bit(*from);
							else {	// double byte character
								if(const Char** const wire = UCS_TO_UHC[mask8Bit(*from >> 8)]) {
									if(const std::uint16_t dbcs = wireAt(wire, mask8Bit(*from))) {
										if(to + 1 >= toEnd)
											break;	// the destnation buffer is insufficient
										*(to++) = mask8Bit(dbcs >> 8);
										*(to++) = mask8Bit(dbcs >> 0);
										continue;
									}
								}
								if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTERS)
									*(to++) = properties().substitutionCharacter();
								else if(substitutionPolicy() != IGNORE_UNMAPPABLE_CHARACTERS) {
									toNext = to;
									fromNext = from;
									return UNMAPPABLE_CHARACTER;
								}
							}
						}
						fromNext = from;
						toNext = to;
						return (from == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
					}
		
					template<> Encoder::ConversionResult InternalEncoder<Uhc>::doToUnicode(
							Char* to, Char* toEnd, Char*& toNext, const Byte* from, const Byte* fromEnd, const Byte*& fromNext) {
						while(to < toEnd && from < fromEnd) {
							if(*from < 0x80)
								*(to++) = *(from++);
							else if(from + 1 >= fromEnd) {	// remaining lead byte
								toNext = to;
								fromNext = from;
								return options().test(END_OF_BUFFER) ? MALFORMED_INPUT : COMPLETED;
							} else {	// double byte character
								if(const std::uint16_t** const wire = UHC_TO_UCS[mask8Bit(*from)]) {
									const Char ucs = wireAt(wire, mask8Bit(from[1]));
									if(ucs != text::REPLACEMENT_CHARACTER) {
										*(to++) = ucs;
										from += 2;
										continue;
									}
								}
								if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTERS) {
									*(to++) = text::REPLACEMENT_CHARACTER;
									from += 2;
								} else if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTERS)
									from += 2;
								else {
									toNext = to;
									fromNext = from;
									return UNMAPPABLE_CHARACTER;
								}
							}
						}
						fromNext = from;
						toNext = to;
						return (from == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
					}


					// EUC-KR /////////////////////////////////////////////////////////////////////////////////////////

					template<> Encoder::ConversionResult InternalEncoder<EucKr>::doFromUnicode(
							Byte* to, Byte* toEnd, Byte*& toNext, const Char* from, const Char* fromEnd, const Char*& fromNext) {
						for(; to < toEnd && from < fromEnd; ++from) {
							if(*from < 0x80)
								*(to++) = mask8Bit(*from);
							else {	// double byte character
								if(const Char** const wire = UCS_TO_UHC[mask8Bit(*from >> 8)]) {
									if(const std::uint16_t dbcs = wireAt(wire, mask8Bit(*from))) {
										const Byte lead = mask8Bit(dbcs >> 8), trail = mask8Bit(dbcs);
										if(lead - 0xa1u < 0x5e && trail - 0xa1 < 0x5e) {
//									if(lead >= 0xa1 && lead <= 0xfe && trail >= 0xa1 && trail <= 0xfe) {
											if(to + 1 >= toEnd)
												break;	// the destnation buffer is insufficient
											*(to++) = lead;
											*(to++) = trail;
											continue;
										}
									}
								}
								if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTERS)
									*(to++) = properties().substitutionCharacter();
								else if(substitutionPolicy() != IGNORE_UNMAPPABLE_CHARACTERS) {
									toNext = to;
									fromNext = from;
									return UNMAPPABLE_CHARACTER;
								}
							}
						}
						fromNext = from;
						toNext = to;
						return (from == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
					}
		
					template<> Encoder::ConversionResult InternalEncoder<EucKr>::doToUnicode(
							Char* to, Char* toEnd, Char*& toNext, const Byte* from, const Byte* fromEnd, const Byte*& fromNext) {
						while(to < toEnd && from < fromEnd) {
							if(*from < 0x80)
								*(to++) = *(from++);
							else if(from + 1 >= fromEnd) {	// remaining lead byte
								toNext = to;
								fromNext = from;
								return options().test(END_OF_BUFFER) ? MALFORMED_INPUT : COMPLETED;
							} else {	// double byte character
								if(from[0] - 0xa1u > 0x5du || from[1] - 0xa1u > 0x5du) {
//							if(!(from[0] >= 0xa1 && from[0] <= 0xfe) || !(from[1] >= 0xa1 && from[1] <= 0xfe)) {
									toNext = to;
									fromNext = from;
									return MALFORMED_INPUT;
								} else if(const std::uint16_t** const wire = UHC_TO_UCS[mask8Bit(*from)]) {
									const Char ucs = wireAt(wire, mask8Bit(from[1]));
									if(ucs != text::REPLACEMENT_CHARACTER) {
										*(to++) = ucs;
										from += 2;
										continue;
									}
								}
								if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTERS) {
									*(to++) = text::REPLACEMENT_CHARACTER;
									from += 2;
								} else if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTERS)
									from += 2;
								else {
									toNext = to;
									fromNext = from;
									return UNMAPPABLE_CHARACTER;
								}
							}
						}
						fromNext = from;
						toNext = to;
						return (from == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
					}


					// ISO-2022-KR ////////////////////////////////////////////////////////////////////////////////////

					// state definition
					// 0 : initial, 1 : encountered/written escape sequence and ASCII, 2 : KS C 5601 (KS X 1001)

					template<> Encoder::ConversionResult InternalEncoder<Iso2022Kr>::doFromUnicode(
							Byte* to, Byte* toEnd, Byte*& toNext, const Char* from, const Char* fromEnd, const Char*& fromNext) {
						if(encodingState_ == 0) {
							// write an escape sequence
							if(to + 3 >= toEnd) {
								toNext = to;
								fromNext = from;
								return INSUFFICIENT_BUFFER;
							}
							std::memcpy(to, "\x1b$)C", 4);
							++encodingState_;
							to += 4;
						}
						for(; to < toEnd && from < fromEnd; ++from) {
							if(*from < 0x80) {
								if(encodingState_ == 2) {
									// introduce ASCII character set
									*(to++) = SI;
									--encodingState_;
									if(to == toEnd)
										break;
								}
								*(to++) = mask8Bit(*from);
							} else {	// double byte character
								if(encodingState_ == 1) {
									// introduce KS C 5601 (KS X 1001) character set
									*(to++) = SO;
									++encodingState_;
									if(to == toEnd)
										break;
								}
								if(const Char** const wire = UCS_TO_UHC[mask8Bit(*from >> 8)]) {
									if(const std::uint16_t dbcs = wireAt(wire, mask8Bit(*from))) {
										const Byte lead = mask8Bit(dbcs >> 8), trail = mask8Bit(dbcs);
										if(lead - 0xa1u < 0x5e && trail - 0xa1 < 0x5e) {
//									if(lead >= 0xa1 && lead <= 0xfe && trail >= 0xa1 && trail <= 0xfe) {
											if(to + 1 >= toEnd)
												break;	// the destnation buffer is insufficient
											*(to++) = mask7Bit(lead);
											*(to++) = mask7Bit(trail);
											continue;
										}
									}
								}
								if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTERS)
									*(to++) = properties().substitutionCharacter();
								else if(substitutionPolicy() != IGNORE_UNMAPPABLE_CHARACTERS) {
									toNext = to;
									fromNext = from;
									return UNMAPPABLE_CHARACTER;
								}
							}
						}
						fromNext = from;
						toNext = to;
						return (from == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
					}

					template<> Encoder::ConversionResult InternalEncoder<Iso2022Kr>::doToUnicode(
							Char* to, Char* toEnd, Char*& toNext, const Byte* from, const Byte* fromEnd, const Byte*& fromNext) {
						if(decodingState_ == 0)
							++decodingState_;
						while(to < toEnd && from < fromEnd) {
							if((*from & 0x80) != 0) {
								// reject 8-bit character
								toNext = to;
								fromNext = from;
								return MALFORMED_INPUT;
							} else if(*from == SI) {
								// introduce ASCII character set
								decodingState_ = 2;
								++from;
							} else if(*from == SO) {
								// introduce KS C 5601 (KS X 1001) character set
								decodingState_ = 3;
								++from;
							} else if(*from == ESC) {
								if(from + 3 >= fromEnd || std::memcmp(from + 1, "$)C", 3) != 0) {
									// invalid escape sequence
									toNext = to;
									fromNext = from;
									return MALFORMED_INPUT;
								}
								from += 4;
							} else if(decodingState_ == 1)
								*(to++) = *(from++);
							else if(from + 1 >= fromEnd) {	// remaining lead byte
								toNext = to;
								fromNext = from;
								return options().test(END_OF_BUFFER) ? MALFORMED_INPUT : COMPLETED;
							} else {	// double byte character
								if(from[0] - 0x21u > 0x5du || from[1] - 0x21u > 0x5du) {
//							if(!(from[0] >= 0x21 && from[0] <= 0x7e) || !(from[1] >= 0x21 && from[1] <= 0x7e)) {
									toNext = to;
									fromNext = from;
									return MALFORMED_INPUT;
								} else if(const std::uint16_t** const wire = UHC_TO_UCS[mask8Bit(*from + 0x80)]) {
									const Char ucs = wireAt(wire, mask8Bit(from[1] + 0x80));
									if(ucs != text::REPLACEMENT_CHARACTER) {
										*(to++) = ucs;
										from += 2;
										continue;
									}
								}
								if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTERS) {
									*(to++) = text::REPLACEMENT_CHARACTER;
									from += 2;
								} else if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTERS)
									from += 2;
								else {
									toNext = to;
									fromNext = from;
									return UNMAPPABLE_CHARACTER;
								}
							}
						}
						fromNext = from;
						toNext = to;
						return (from == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
					}
				}
			}
		}
	}
}
