/**
 * @file encoder-implementation.hpp
 * Defines @c ascension#encoding#implementation namespace.
 * @author exeal
 * @date 2004-2014 Was encoder.hpp.
 * @date 2016-09-22 Separated from encoder.hpp.
 */

#ifndef ASCENSION_ENCODER_IMPLEMENTATION_HPP
#define ASCENSION_ENCODER_IMPLEMENTATION_HPP
#include <ascension/config.hpp>	// ASCENSION_NO_*_ENCODINGS
#include <ascension/corelib/basic-types.hpp>
#include <ascension/corelib/string-piece.hpp>
#include <ascension/corelib/text/code-point.hpp>
#include <boost/core/ignore_unused.hpp>
#include <array>
#include <locale>	// std.locale, std.codecvt
#include <memory>	// std.unique_ptr
#include <vector>

namespace ascension {
	namespace encoding {
		template<typename T> inline Byte mask7Bit(T c) {return static_cast<Byte>(c & 0x7fu);}
		template<typename T> inline std::uint8_t mask8Bit(T c) {return static_cast<std::uint8_t>(c & 0xffu);}
		template<typename T> inline std::uint16_t mask16Bit(T c) {return static_cast<std::uint16_t>(c & 0xffffu);}
		template<typename T> inline Char maskUCS2(T c) {return static_cast<Char>(c & 0xffffu);}

		/// Supports implementation of encoder classes.
		namespace implementation {
			// control codes
			const Byte SI = 0x0f;		// SI (Shift in).
			const Byte SO = 0x0e;		// SO (Shift out).
			const Byte ESC = 0x1b;		// Escape.
			const Byte SS2_8BIT = 0x8e;	// SS2 (Single shift two).
			const Byte SS3_8BIT = 0x8f;	// SS3 (Single shift three).

			/// @c EncoderFactoryBase is a base implementation of @c EncoderFactory.
			class EncoderFactoryBase : public EncoderFactory {
			public:
				virtual ~EncoderFactoryBase() BOOST_NOEXCEPT;
			protected:
				EncoderFactoryBase(const boost::string_ref& name,
					MIBenum mib, const boost::string_ref& displayName = "",
					std::size_t maximumNativeBytes = 1, std::size_t maximumUCSLength = 1,
					const boost::string_ref& aliases = "", Byte substitutionCharacter = 0x1a);
			protected:
				// EncodingProperties
				virtual std::string aliases() const BOOST_NOEXCEPT;
				virtual std::string displayName(const std::locale& lc) const BOOST_NOEXCEPT;
				virtual std::size_t maximumNativeBytes() const BOOST_NOEXCEPT;
				virtual std::size_t maximumUCSLength() const BOOST_NOEXCEPT;
				virtual MIBenum mibEnum() const BOOST_NOEXCEPT;
				virtual std::string name() const BOOST_NOEXCEPT;
				virtual Byte substitutionCharacter() const BOOST_NOEXCEPT;
			private:
				const std::string name_, displayName_, aliases_;
				const std::size_t maximumNativeBytes_, maximumUCSLength_;
				const MIBenum mib_;
				const Byte substitutionCharacter_;
			};

			/// Generates 16-code sequence.
			template<typename Code,
				Code c0, Code c1, Code c2, Code c3, Code c4, Code c5, Code c6, Code c7,
				Code c8, Code c9, Code cA, Code cB, Code cC, Code cD, Code cE, Code cF>
			struct CodeLine {static const Code VALUES[16];};

			/// Generates 16-character sequence.
			template<
				Char c0, Char c1, Char c2, Char c3, Char c4, Char c5, Char c6, Char c7,
				Char c8, Char c9, Char cA, Char cB, Char cC, Char cD, Char cE, Char cF>
			struct CharLine : public CodeLine<Char, c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, cA, cB, cC, cD, cE, cF> {};

			/// Generates an incremental character sequence from @a start.
			template<Char start, Char step = +1> struct SequentialCharLine : public CharLine<
				start + step * 0, start + step * 1, start + step * 2, start + step * 3,
				start + step * 4, start + step * 5, start + step * 6, start + step * 7,
				start + step * 8, start + step * 8, start + step * 10, start + step * 11,
				start + step * 12, start + step * 13, start + step * 14, start + step * 15> {};

			/// Generates an all NUL character sequence.
			struct EmptyCharLine : public SequentialCharLine<0xfffdu, 0> {};

			/// Generates 16×16-code sequence.
			template<typename Code,
				typename Line0, typename Line1, typename Line2, typename Line3,
				typename Line4, typename Line5, typename Line6, typename Line7,
				typename Line8, typename Line9, typename LineA, typename LineB,
				typename LineC, typename LineD, typename LineE, typename LineF>
			struct CodeWire {static const Code* VALUES[16];};

			/// Returns a code corresponds to a byte in the 16×16 wire.
			/// @see CodeWire
			template<typename Code> inline Code wireAt(const Code** wire, Byte c) BOOST_NOEXCEPT {return wire[c >> 4][c & 0xf];}

			/// Generates 16×16-character sequence.
			template<
				typename Line0, typename Line1, typename Line2, typename Line3,
				typename Line4, typename Line5, typename Line6, typename Line7,
				typename Line8, typename Line9, typename LineA, typename LineB,
				typename LineC, typename LineD, typename LineE, typename LineF>
			class CharWire : public CodeWire<Char, Line0, Line1, Line2, Line3,
				Line4, Line5, Line6, Line7, Line8, Line9, LineA, LineB, LineC, LineD, LineE, LineF> {};

			namespace sbcs {
				/// A substitution byte value in used in Unicode to native mapping table.
				const Byte UNMAPPABLE_BYTE = 0x00;

				/// Provides bidirectional mapping between a byte and a character.
				/// @see CharMap
				class BidirectionalMap {
				public:
					BidirectionalMap(const Char** byteToCharacterWire) BOOST_NOEXCEPT;
					~BidirectionalMap() BOOST_NOEXCEPT;
					Byte toByte(Char c) const BOOST_NOEXCEPT;
					Char toCharacter(Byte c) const BOOST_NOEXCEPT;
				private:
					void buildUnicodeToByteTable();
				private:
					const Char** const byteToUnicode_;
					std::array<Byte*, 0x100> unicodeToByte_;
					static const std::array<const Byte, 0x100> UNMAPPABLE_16x16_UNICODE_TABLE;
				};

				/// Generates ISO IR C0 character sequence 0x00 through 0x0F.
				typedef SequentialCharLine<0x0000u> ISO_IR_C0_LINE0;
				/// Generates ISO IR C0 character sequence 0x10 through 0x1F.
				typedef SequentialCharLine<0x0010u> ISO_IR_C0_LINE1;
				/// Generates ISO IR C1 character sequence 0x80 through 0x8F.
				typedef SequentialCharLine<0x0080u> ISO_IR_C1_LINE8;
				/// Generates ISO IR C1 character sequence 0x90 through 0x9F.
				typedef SequentialCharLine<0x0090u> ISO_IR_C1_LINE9;

				/// Generates 16×16 character table compatible with ISO 646.
				template<
					typename Line8, typename Line9, typename LineA, typename LineB,
					typename LineC, typename LineD, typename LineE, typename LineF>
				class ASCIICompatibleCharWire : public CharWire<
					ISO_IR_C0_LINE0, ISO_IR_C0_LINE1,
					SequentialCharLine<0x0020u>, SequentialCharLine<0x0030u>, SequentialCharLine<0x0040u>,
					SequentialCharLine<0x0050u>, SequentialCharLine<0x0060u>, SequentialCharLine<0x0070u>,
					Line8, Line9, LineA, LineB, LineC, LineD, LineE, LineF> {};

				/// Generates 16×16 character table compatible with ISO-IR.
				template<
					typename Line2, typename Line3, typename Line4, typename Line5, typename Line6, typename Line7,
					typename LineA, typename LineB, typename LineC, typename LineD, typename LineE, typename LineF>
				class ISOIRCharWire : public CharWire<
					ISO_IR_C0_LINE0, ISO_IR_C0_LINE1, Line2, Line3, Line4, Line5, Line6, Line7,
					ISO_IR_C1_LINE8, ISO_IR_C1_LINE9, LineA, LineB, LineC, LineD, LineE, LineF> {};

				/// Generates 16×16 character table compatible with ISO 8859.
				template<typename LineA, typename LineB, typename LineC, typename LineD, typename LineE, typename LineF>
				class ISO8859CompatibleCharWire : public ISOIRCharWire<
					SequentialCharLine<0x0020u>, SequentialCharLine<0x0030u>, SequentialCharLine<0x0040u>,
					SequentialCharLine<0x0050u>, SequentialCharLine<0x0060u>, SequentialCharLine<0x0070u>,
					LineA, LineB, LineC, LineD, LineE, LineF> {};

				/// Generates 16×16 character table compatible with IBM PC code page.
				template<
					typename Line8, typename Line9, typename LineA, typename LineB,
					typename LineC, typename LineD, typename LineE, typename LineF>
				class IBMPCCompatibleCharWire : public CharWire<
					SequentialCharLine<0x0000u>,
					CharLine<0x0010u, 0x0011u, 0x0012u, 0x0013u, 0x0014u, 0x0015u,
						0x0016u, 0x0017u, 0x0018u, 0x0019u, 0x001cu, 0x001bu, 0x007fu, 0x001du, 0x001eu, 0x001fu>,
					SequentialCharLine<0x0020u>, SequentialCharLine<0x0030u>, SequentialCharLine<0x0040u>,
					SequentialCharLine<0x0050u>, SequentialCharLine<0x0060u>,
					CharLine<0x0070u, 0x0071u, 0x0072u, 0x0073u, 0x0074u, 0x0075u,
						0x0076u, 0x0077u, 0x0078u, 0x0079u, 0x007au, 0x007bu, 0x007cu, 0x007du, 0x007eu, 0x001au>,
					Line8, Line9, LineA, LineB, LineC, LineD, LineE, LineF> {};

				/// Base class of single byte charset encoder factories.
				template<typename MappingTable>
				class SingleByteEncoderFactory : public EncoderFactoryBase {
				public:
					SingleByteEncoderFactory(const boost::string_ref& name, MIBenum mib,
						const boost::string_ref& displayName, const boost::string_ref& aliases, Byte substitutionCharacter);
					virtual ~SingleByteEncoderFactory() BOOST_NOEXCEPT;
				private:
					std::unique_ptr<Encoder> create() const BOOST_NOEXCEPT;
				};

				/**
				 * Returns the byte corresponds to the given character @c c or @c UNMAPPABLE_BYTE if umappable.
				 * @param c The character to map
				 * @return The corresponding byte or @c UNMAPPABLE_BYTE
				 */
				inline Byte BidirectionalMap::toByte(Char c) const BOOST_NOEXCEPT {
					return unicodeToByte_[c >> 8][mask8Bit(c)];
				}

				/**
				 * Returns the character corresponds to the given byte @a c or @c REPLACEMENT_CHARACTER if umappable.
				 * @param c The byte to map
				 * @return The corresponding character or @c REPLACEMENT_CHARACTER
				 */
				inline Char BidirectionalMap::toCharacter(Byte c) const BOOST_NOEXCEPT {
					return wireAt(byteToUnicode_, c);
				}

				/**
				 * Constructor.
				 * @param name The name returned by @c #name
				 * @param mib The MIBenum value returned by @c #mibEnum
				 * @param displayName The display name returned by @c #displayName
				 * @param aliases The encoding aliases returned by @c #aliases
				 * @param substitutionCharacter The native character returned by @c #substitutionCharacter
				 */
				template<typename MappingTable>
				inline SingleByteEncoderFactory<MappingTable>::SingleByteEncoderFactory(const boost::string_ref& name, MIBenum mib,
					const boost::string_ref& displayName, const boost::string_ref& aliases, Byte substitutionCharacter) : EncoderFactoryBase(name, mib, displayName, 1, 1, aliases, substitutionCharacter) {}

				/// Destructor.
				template<typename MappingTable>
				inline SingleByteEncoderFactory<MappingTable>::~SingleByteEncoderFactory() BOOST_NOEXCEPT {
				}

				/// @see EncoderFactory#create
				template<typename MappingTable> std::unique_ptr<Encoder>
				SingleByteEncoderFactory<MappingTable>::create() const BOOST_NOEXCEPT {
					return detail::createSingleByteEncoder(MappingTable::VALUES, *this);
				}
			} // namespace sbcs

			namespace dbcs {
				static_assert(sizeof(std::uint16_t) == 2, "");

				/// Generates 16-DBCS character sequence.
				template<
					std::uint16_t c0, std::uint16_t c1, std::uint16_t c2, std::uint16_t c3,
					std::uint16_t c4, std::uint16_t c5, std::uint16_t c6, std::uint16_t c7,
					std::uint16_t c8, std::uint16_t c9, std::uint16_t cA, std::uint16_t cB,
					std::uint16_t cC, std::uint16_t cD, std::uint16_t cE, std::uint16_t cF>
				struct DBCSLine : public CodeLine<std::uint16_t, c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, cA, cB, cC, cD, cE, cF> {};

				/// Generates an all NUL value sequence.
				struct EmptyDBCSLine : public DBCSLine<0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0> {};

				/// Generates 16×16-DBCS character sequence.
				template<
					typename Line0, typename Line1, typename Line2, typename Line3,
					typename Line4, typename Line5, typename Line6, typename Line7,
					typename Line8, typename Line9, typename LineA, typename LineB,
					typename LineC, typename LineD, typename LineE, typename LineF>
				class DBCSWire : public CodeWire<std::uint16_t, Line0, Line1, Line2, Line3,
					Line4, Line5, Line6, Line7, Line8, Line9, LineA, LineB, LineC, LineD, LineE, LineF> {};
			} // namespace dbcs
		}

		namespace detail {
			std::unique_ptr<Encoder> createSingleByteEncoder(
				const Char** byteToCharacterWire, const EncodingProperties& properties) BOOST_NOEXCEPT;
		}

		template<typename Code,
			Code c0, Code c1, Code c2, Code c3, Code c4, Code c5, Code c6, Code c7,
			Code c8, Code c9, Code cA, Code cB, Code cC, Code cD, Code cE, Code cF>
		const Code implementation::CodeLine<Code,
			c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, cA, cB, cC, cD, cE, cF>::VALUES[16] = {
			c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, cA, cB, cC, cD, cE, cF};

		template<typename Code,
			typename Line0, typename Line1, typename Line2, typename Line3, typename Line4, typename Line5, typename Line6, typename Line7,
			typename Line8, typename Line9, typename LineA, typename LineB, typename LineC, typename LineD, typename LineE, typename LineF>
		const Code* implementation::CodeWire<Code,
			Line0, Line1, Line2, Line3, Line4, Line5, Line6, Line7, Line8, Line9, LineA, LineB,LineC, LineD, LineE, LineF>::VALUES[16] = {
			Line0::VALUES, Line1::VALUES, Line2::VALUES, Line3::VALUES, Line4::VALUES, Line5::VALUES, Line6::VALUES, Line7::VALUES,
			Line8::VALUES, Line9::VALUES, LineA::VALUES, LineB::VALUES, LineC::VALUES, LineD::VALUES, LineE::VALUES, LineF::VALUES};
	}
} // namespace ascension.encoding

#endif // !ASCENSION_ENCODER_IMPLEMENTATION_HPP
