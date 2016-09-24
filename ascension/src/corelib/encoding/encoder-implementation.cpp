/**
 * @file encoder-implementation.cpp
 * Implements @c ascension#encoding#implementation namespace.
 * @author exeal
 * @date 2004-2014 Was encoder.cpp.
 * @date 2016-09-24 Separated from encoder.cpp.
 */

#include <ascension/corelib/encoding/encoder.hpp>
#include <ascension/corelib/encoding/encoder-implementation.hpp>
#include <algorithm>	// std.fill_n


namespace ascension {
	namespace encoding {
		namespace implementation {
			/**
			 * Constructor.
			 * @param name The name returned by @c #name
			 * @param mib The MIBenum value returned by @c #mibEnum
			 * @param displayName The display name returned by @c #displayName
			 * @param maximumNativeBytes The value returned by @c #maximumNativeBytes
			 * @param maximumUCSLength The value returned by @c #maximumUCSLength
			 * @param aliases The encoding aliases returned by @c #aliases
			 * @param substitutionCharacter The substitution character
			 */
			EncoderFactoryImpl::EncoderFactoryImpl(const std::string& name, MIBenum mib,
					const std::string& displayName /* = std::string() */,
					std::size_t maximumNativeBytes /* = 1 */, std::size_t maximumUCSLength /* = 1 */,
					const std::string& aliases /* = std::string() */, Byte substitutionCharacter /* = 0x1a */)
					: name_(name), displayName_(displayName.empty() ? name : displayName), aliases_(aliases),
					maximumNativeBytes_(maximumNativeBytes), maximumUCSLength_(maximumUCSLength),
					mib_(mib), substitutionCharacter_(substitutionCharacter) {
			}

			/// Destructor.
			EncoderFactoryImpl::~EncoderFactoryImpl() BOOST_NOEXCEPT {
			}

			/// @see EncodingProperties#aliases
			std::string EncoderFactoryImpl::aliases() const BOOST_NOEXCEPT {
				return aliases_;
			}

			/// @see EncodingProperties#displayName
			std::string EncoderFactoryImpl::displayName(const std::locale&) const BOOST_NOEXCEPT {
				return displayName_;
			}

			/// @see EncodingProperties#maximumNativeBytes
			std::size_t EncoderFactoryImpl::maximumNativeBytes() const BOOST_NOEXCEPT {
				return maximumNativeBytes_;
			}

			/// @see EncodingProperties#maximumUCSLength
			std::size_t EncoderFactoryImpl::maximumUCSLength() const BOOST_NOEXCEPT {
				return maximumUCSLength_;
			}

			/// @see EncodingProperties#mibEnum
			MIBenum EncoderFactoryImpl::mibEnum() const BOOST_NOEXCEPT {
				return mib_;
			}

			/// @see EncodingProperties#name
			std::string EncoderFactoryImpl::name() const BOOST_NOEXCEPT {
				return name_;
			}

			/// @see EncodingProperties#substitutionCharacter
			Byte EncoderFactoryImpl::substitutionCharacter() const BOOST_NOEXCEPT {
				return substitutionCharacter_;
			}

			namespace sbcs {
				const std::array<const Byte, 0x100> BidirectionalMap::UNMAPPABLE_16x16_UNICODE_TABLE = {{
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				}};

				/**
				 * Constructor.
				 * @param byteToCharacterWire The table defines byte-to-character mapping consists of 16~16-characters
				 */
				BidirectionalMap::BidirectionalMap(const Char** byteToCharacterWire) BOOST_NOEXCEPT : byteToUnicode_(byteToCharacterWire) {
					unicodeToByte_.fill(nullptr);
					buildUnicodeToByteTable();	// eager?
				}

				/// Destructor.
				BidirectionalMap::~BidirectionalMap() BOOST_NOEXCEPT {
					for(std::size_t i = 0; i < std::tuple_size<decltype(unicodeToByte_)>::value; ++i) {
						if(unicodeToByte_[i] != UNMAPPABLE_16x16_UNICODE_TABLE.data())
							delete[] unicodeToByte_[i];
					}
				}

				void sbcs::BidirectionalMap::buildUnicodeToByteTable() {
					assert(unicodeToByte_[0] == 0);
					unicodeToByte_.fill(const_cast<Byte*>(UNMAPPABLE_16x16_UNICODE_TABLE.data()));
					for(int i = 0x00; i < 0xff; ++i) {
						const Char ucs = wireAt(byteToUnicode_, static_cast<Byte>(i));
						Byte*& p = unicodeToByte_[ucs >> 8];
						if(p == UNMAPPABLE_16x16_UNICODE_TABLE.data()) {
							p = new Byte[0x100];
							std::fill_n(p, 0x100, UNMAPPABLE_BYTE);
						}
						p[mask8Bit(ucs)] = static_cast<Byte>(i);
					}
				}

				namespace {
					class SingleByteEncoder : public Encoder {
					public:
						explicit SingleByteEncoder(const Char** byteToCharacterWire, const EncodingProperties& properties) BOOST_NOEXCEPT;
					private:
						// Encoder
						ConversionResult doFromUnicode(Byte* to, Byte* toEnd, Byte*& toNext,
							const Char* from, const Char* fromEnd, const Char*& fromNext) override;
						ConversionResult doToUnicode(Char* to, Char* toEnd, Char*& toNext,
							const Byte* from, const Byte* fromEnd, const Byte*& fromNext) override;
						const EncodingProperties& properties() const override BOOST_NOEXCEPT {return props_;}
					private:
						const sbcs::BidirectionalMap table_;
						const EncodingProperties& props_;
					};

					SingleByteEncoder::SingleByteEncoder(const Char** byteToCharacterWire,
							const EncodingProperties& properties) BOOST_NOEXCEPT : table_(byteToCharacterWire), props_(properties) {
					}

					Encoder::ConversionResult SingleByteEncoder::doFromUnicode(Byte* to, Byte* toEnd,
							Byte*& toNext, const Char* from, const Char* fromEnd, const Char*& fromNext) {
						for(; to < toEnd && from < fromEnd; ++to, ++from) {
							*to = table_.toByte(*from);
							if(*to == sbcs::UNMAPPABLE_BYTE && *from != sbcs::UNMAPPABLE_BYTE) {
								if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTERS)
									--to;
								else if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTERS)
									*to = properties().substitutionCharacter();
								else {
									toNext = to;
									fromNext = from;
									return UNMAPPABLE_CHARACTER;
								}
							}
						}
						toNext = to;
						fromNext = from;
						return (fromNext == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
					}

					Encoder::ConversionResult SingleByteEncoder::doToUnicode(Char* to, Char* toEnd,
							Char*& toNext, const Byte* from, const Byte* fromEnd, const Byte*& fromNext) {
						for(; to < toEnd && from < fromEnd; ++to, ++from) {
							*to = table_.toCharacter(*from);
							if(*to == text::REPLACEMENT_CHARACTER) {
								if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTERS)
									--to;
								else if(substitutionPolicy() != REPLACE_UNMAPPABLE_CHARACTERS) {
									toNext = to;
									fromNext = from;
									return UNMAPPABLE_CHARACTER;
								}
							}
						}
						toNext = to;
						fromNext = from;
						return (fromNext == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
					}
				} // namespace @0
			}
		}

		namespace detail {
			std::unique_ptr<encoding::Encoder> createSingleByteEncoder(
					const Char** byteToCharacterWire, const encoding::EncodingProperties& properties) BOOST_NOEXCEPT {
				return std::unique_ptr<encoding::Encoder>(new encoding::implementation::sbcs::SingleByteEncoder(byteToCharacterWire, properties));
			}
		}
	}
}
