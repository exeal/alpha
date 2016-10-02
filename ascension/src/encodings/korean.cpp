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

					inline bool eob(const Encoder& encoder) BOOST_NOEXCEPT {
#if 0
						return encoder.options().test(Encoder::END_OF_BUFFER);
#else
						boost::ignore_unused(encoder);
						return true;
#endif
					}

					template<typename Factory>
					class InternalEncoder : public Encoder {
					public:
						explicit InternalEncoder(const Factory& factory) BOOST_NOEXCEPT : properties_(factory) {}
					private:
						Result doFromUnicode(State& state,
							const boost::iterator_range<Byte*>& to, Byte*& toNext,
							const boost::iterator_range<const Char*>& from, const Char*& fromNext) override;
						Result doToUnicode(State& state,
							const boost::iterator_range<Char*>& to, Char*& toNext,
							const boost::iterator_range<const Byte*>& from, const Byte*& fromNext) override;
						const EncodingProperties& properties() const override BOOST_NOEXCEPT {
							return properties_;
						}
					private:
						const EncodingProperties& properties_;
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

					template<> Encoder::Result InternalEncoder<Uhc>::doFromUnicode(State& state,
							const boost::iterator_range<Byte*>& to, Byte*& toNext, const boost::iterator_range<const Char*>& from, const Char*& fromNext) {
						toNext = boost::begin(to);
						fromNext = boost::const_begin(from);
						for(; toNext < boost::end(to) && fromNext < boost::const_end(from); ++fromNext) {
							if(*fromNext < 0x80)
								*(toNext++) = mask8Bit(*fromNext);
							else {	// double byte character
								if(const Char** const wire = UCS_TO_UHC[mask8Bit(*fromNext >> 8)]) {
									if(const std::uint16_t dbcs = wireAt(wire, mask8Bit(*fromNext))) {
										if(toNext + 1 >= boost::end(to))
											break;	// the destnation buffer is insufficient
										*(toNext++) = mask8Bit(dbcs >> 8);
										*(toNext++) = mask8Bit(dbcs >> 0);
										continue;
									}
								}
								if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTERS)
									*(toNext++) = properties().substitutionCharacter();
								else if(substitutionPolicy() != IGNORE_UNMAPPABLE_CHARACTERS)
									return UNMAPPABLE_CHARACTER;
							}
						}
						return (fromNext == boost::const_end(from)) ? COMPLETED : INSUFFICIENT_BUFFER;
					}
		
					template<> Encoder::Result InternalEncoder<Uhc>::doToUnicode(State& state,
							const boost::iterator_range<Char*>& to, Char*& toNext, const boost::iterator_range<const Byte*>& from, const Byte*& fromNext) {
						toNext = boost::begin(to);
						fromNext = boost::const_begin(from);
						while(toNext < boost::end(to) && fromNext < boost::const_end(from)) {
							if(*fromNext < 0x80)
								*(toNext++) = *(fromNext++);
							else if(fromNext + 1 >= boost::const_end(from))	// remaining lead byte
								return eob(*this) ? MALFORMED_INPUT : COMPLETED;
							else {	// double byte character
								if(const std::uint16_t** const wire = UHC_TO_UCS[mask8Bit(*fromNext)]) {
									const Char ucs = wireAt(wire, mask8Bit(fromNext[1]));
									if(ucs != text::REPLACEMENT_CHARACTER) {
										*(toNext++) = ucs;
										fromNext += 2;
										continue;
									}
								}
								if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTERS) {
									*(toNext++) = text::REPLACEMENT_CHARACTER;
									fromNext += 2;
								} else if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTERS)
									fromNext += 2;
								else
									return UNMAPPABLE_CHARACTER;
							}
						}
						return (fromNext == boost::const_end(from)) ? COMPLETED : INSUFFICIENT_BUFFER;
					}


					// EUC-KR /////////////////////////////////////////////////////////////////////////////////////////

					template<> Encoder::Result InternalEncoder<EucKr>::doFromUnicode(State& state,
							const boost::iterator_range<Byte*>& to, Byte*& toNext, const boost::iterator_range<const Char*>& from, const Char*& fromNext) {
						toNext = boost::begin(to);
						fromNext = boost::const_begin(from);
						for(; toNext < boost::end(to) && fromNext < boost::const_end(from); ++fromNext) {
							if(*fromNext < 0x80)
								*(toNext++) = mask8Bit(*fromNext);
							else {	// double byte character
								if(const Char** const wire = UCS_TO_UHC[mask8Bit(*fromNext >> 8)]) {
									if(const std::uint16_t dbcs = wireAt(wire, mask8Bit(*fromNext))) {
										const Byte lead = mask8Bit(dbcs >> 8), trail = mask8Bit(dbcs);
										if(lead - 0xa1u < 0x5e && trail - 0xa1 < 0x5e) {
//									if(lead >= 0xa1 && lead <= 0xfe && trail >= 0xa1 && trail <= 0xfe) {
											if(toNext + 1 >= boost::end(to))
												break;	// the destnation buffer is insufficient
											*(toNext++) = lead;
											*(toNext++) = trail;
											continue;
										}
									}
								}
								if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTERS)
									*(toNext++) = properties().substitutionCharacter();
								else if(substitutionPolicy() != IGNORE_UNMAPPABLE_CHARACTERS)
									return UNMAPPABLE_CHARACTER;
							}
						}
						return (fromNext == boost::const_end(from)) ? COMPLETED : INSUFFICIENT_BUFFER;
					}
		
					template<> Encoder::Result InternalEncoder<EucKr>::doToUnicode(State& state,
							const boost::iterator_range<Char*>& to, Char*& toNext, const boost::iterator_range<const Byte*>& from, const Byte*& fromNext) {
						toNext = boost::begin(to);
						fromNext = boost::const_begin(from);
						while(toNext < boost::end(to) && fromNext < boost::const_end(from)) {
							if(*fromNext < 0x80)
								*(toNext++) = *(fromNext++);
							else if(fromNext + 1 >= boost::const_end(from))	// remaining lead byte
								return eob(*this) ? MALFORMED_INPUT : COMPLETED;
							else {	// double byte character
								if(fromNext[0] - 0xa1u > 0x5du || fromNext[1] - 0xa1u > 0x5du)
//								if(!(fromNext[0] >= 0xa1 && fromNext[0] <= 0xfe) || !(fromNext[1] >= 0xa1 && fromNext[1] <= 0xfe))
									return MALFORMED_INPUT;
								else if(const std::uint16_t** const wire = UHC_TO_UCS[mask8Bit(*fromNext)]) {
									const Char ucs = wireAt(wire, mask8Bit(fromNext[1]));
									if(ucs != text::REPLACEMENT_CHARACTER) {
										*(toNext++) = ucs;
										fromNext += 2;
										continue;
									}
								}
								if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTERS) {
									*(toNext++) = text::REPLACEMENT_CHARACTER;
									fromNext += 2;
								} else if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTERS)
									fromNext += 2;
								else
									return UNMAPPABLE_CHARACTER;
							}
						}
						return (fromNext == boost::const_end(from)) ? COMPLETED : INSUFFICIENT_BUFFER;
					}


					// ISO-2022-KR ////////////////////////////////////////////////////////////////////////////////////

					enum ConversionState {
						ASCII,		// encountered/written escape sequence and ASCII
						KS_C_5601	// KS C 5601 (KS X 1001)
					};

					template<> Encoder::Result InternalEncoder<Iso2022Kr>::doFromUnicode(State& state,
							const boost::iterator_range<Byte*>& to, Byte*& toNext, const boost::iterator_range<const Char*>& from, const Char*& fromNext) {
						if(state.empty()) {
							// write an escape sequence
							if(std::next(toNext, 3) >= boost::end(to))
								return INSUFFICIENT_BUFFER;
							std::memcpy(toNext, "\x1b$)C", 4);
							state = ASCII;
							std::advance(toNext, 4);
						} else if(boost::any_cast<ConversionState>(&state) == nullptr)
							throw BadStateException();

						ConversionState& conversionState = boost::any_cast<ConversionState&>(state);
						toNext = boost::begin(to);
						fromNext = boost::const_begin(from);
						for(; toNext < boost::end(to) && fromNext < boost::const_end(from); ++fromNext) {
							if(*fromNext < 0x80) {
								if(conversionState == KS_C_5601) {
									// introduce ASCII character set
									*(toNext++) = SI;
									conversionState = ASCII;
									if(toNext == boost::end(to))
										break;
								}
								*(toNext++) = mask8Bit(*fromNext);
							} else {	// double byte character
								if(conversionState == ASCII) {
									// introduce KS C 5601 (KS X 1001) character set
									*(toNext++) = SO;
									conversionState = KS_C_5601;
									if(toNext == boost::end(to))
										break;
								}
								if(const Char** const wire = UCS_TO_UHC[mask8Bit(*fromNext >> 8)]) {
									if(const std::uint16_t dbcs = wireAt(wire, mask8Bit(*fromNext))) {
										const Byte lead = mask8Bit(dbcs >> 8), trail = mask8Bit(dbcs);
										if(lead - 0xa1u < 0x5e && trail - 0xa1 < 0x5e) {
//										if(lead >= 0xa1 && lead <= 0xfe && trail >= 0xa1 && trail <= 0xfe) {
											if(toNext + 1 >= boost::end(to))
												break;	// the destnation buffer is insufficient
											*(toNext++) = mask7Bit(lead);
											*(toNext++) = mask7Bit(trail);
											continue;
										}
									}
								}
								if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTERS)
									*(toNext++) = properties().substitutionCharacter();
								else if(substitutionPolicy() != IGNORE_UNMAPPABLE_CHARACTERS)
									return UNMAPPABLE_CHARACTER;
							}
						}
						return (fromNext == boost::const_end(from)) ? COMPLETED : INSUFFICIENT_BUFFER;
					}

					template<> Encoder::Result InternalEncoder<Iso2022Kr>::doToUnicode(State& state,
							const boost::iterator_range<Char*>& to, Char*& toNext, const boost::iterator_range<const Byte*>& from, const Byte*& fromNext) {
						if(state.empty())
							state = ASCII;
						else if(boost::any_cast<ConversionState>(&state) == nullptr)
							throw BadStateException();

						ConversionState& conversionState = boost::any_cast<ConversionState&>(state);
						toNext = boost::begin(to);
						fromNext = boost::const_begin(from);
						while(toNext < boost::end(to) && fromNext < boost::const_end(from)) {
							if((*fromNext & 0x80) != 0)	// reject 8-bit character
								return MALFORMED_INPUT;
							else if(*fromNext == SI) {
								// introduce ASCII character set
								conversionState = ASCII;
								++fromNext;
							} else if(*fromNext == SO) {
								// introduce KS C 5601 (KS X 1001) character set
								conversionState = KS_C_5601;
								++fromNext;
							} else if(*fromNext == ESC) {
								if(fromNext + 3 >= boost::const_end(from) || std::memcmp(fromNext + 1, "$)C", 3) != 0)
									// invalid escape sequence
									return MALFORMED_INPUT;
								std::advance(fromNext, 4);
							} else if(conversionState == ASCII)
								*(toNext++) = *(fromNext++);
							else if(fromNext + 1 >= boost::const_end(from))	// remaining lead byte
								return eob(*this) ? MALFORMED_INPUT : COMPLETED;
							else {	// double byte character
								if(fromNext[0] - 0x21u > 0x5du || fromNext[1] - 0x21u > 0x5du)
	//							if(!(fromNext[0] >= 0x21 && fromNext[0] <= 0x7e) || !(fromNext[1] >= 0x21 && fromNext[1] <= 0x7e))
									return MALFORMED_INPUT;
								else if(const std::uint16_t** const wire = UHC_TO_UCS[mask8Bit(*fromNext + 0x80)]) {
									const Char ucs = wireAt(wire, mask8Bit(fromNext[1] + 0x80));
									if(ucs != text::REPLACEMENT_CHARACTER) {
										*(toNext++) = ucs;
										fromNext += 2;
										continue;
									}
								}
								if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTERS) {
									*(toNext++) = text::REPLACEMENT_CHARACTER;
									fromNext += 2;
								} else if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTERS)
									fromNext += 2;
								else
									return UNMAPPABLE_CHARACTER;
							}
						}
						return (fromNext == boost::const_end(from)) ? COMPLETED : INSUFFICIENT_BUFFER;
					}
				}
			}
		}
	}
}
