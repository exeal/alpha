/**
 * @file encoder.hpp
 * @author exeal
 * @date 2004-2007
 */

#ifndef ASCENSION_ENCODER_HPP
#define ASCENSION_ENCODER_HPP
#include "unicode.hpp"
#include <cassert>
#include <memory>	// std.auto_ptr
#include <locale>	// std.locale, std.codecvt
#include <vector>
#include <set>


namespace ascension {

	/// Members of the namespace @c encoding provide conversions between text encodings.
	namespace encoding {

		/// "The MIBenum value is a unique value for use in MIBs to identify coded character sets"
		/// (http://www.iana.org/assignments/character-sets).
		typedef ushort MIBenum;

		/// Indicates the encoding "is not registered by IANA."
		const MIBenum MIB_OTHER = 1;
		/// "Used as a default value."
		const MIBenum MIB_UNKNOWN = 2;

		/// MIBenum values of the fundamental encodings.
		namespace fundamental {
			const MIBenum
				US_ASCII = 3,		///< ANSI X3.4:1968.
				ISO_8859_1 = 4,		///< ISO-8859-1:1987.
				UTF_8 = 106,		///< UTF-8.
				UTF_16BE = 1013,	///< UTF-16BE.
				UTF_16LE = 1014,	///< UTF-16LE.
				UTF_16 = 1015;		///< UTF-16.
		}

#ifndef ASCENSION_NO_STANDARD_ENCODINGS
		/// MIBenum values of the standard encodings.
		namespace standard {
			const MIBenum
				ISO_8859_2 = 5,		///< ISO-8859-2:1987.
				ISO_8859_3 = 6,		///< ISO-8859-3:1988.
				ISO_8859_4 = 7,		///< ISO-8859-4:1988.
				ISO_8859_5 = 8,		///< ISO-8859-5:1988.
				ISO_8859_6 = 9,		///< ISO-8859-6:1987.
				ISO_8859_7 = 10,	///< ISO-8859-7:1987.
				ISO_8859_8 = 11,	///< ISO-8859-8:1988.
				ISO_8859_9 = 12,	///< ISO-8859-9:1989.
				ISO_8859_10 = 13,	///< ISO-8859-10:1992.
				SHIFT_JIS = 17,		///< Shift_JIS (JIS X 0208:1997).
				EUC_JP = 18,		///< EUC-JP (JIS X 0208:1997 and JIS X 0212:1990).
				ISO_2022_KR = 37,	///< ISO-2022-KR.
				EUC_KR = 38,		///< EUC-KR.
				ISO_2022_JP = 39,	///< ISO-2022-JP (RFC1468 and JIS X 0208:1997).
				ISO_2022_JP_2 = 40,	///< ISO-2022-JP-2 (RFC1554 and JIS X 0212:1990).
				ISO_8859_6_E = 81,	///< ISO-8859-6-E.
				ISO_8859_6_I = 82,	///< ISO-8859-6-I.
				ISO_8859_8_E = 84,	///< ISO-8859-8-E.
				ISO_8859_8_I = 85,	///< ISO-8859-8-I.
				ISO_8859_13 = 109,	///< ISO-8859-13.
				ISO_8859_14 = 110,	///< ISO-8859-14:1998.
				ISO_8859_15 = 111,	///< ISO-8859-15.
				ISO_8859_16 = 112,	///< ISO-8859-16:2001.
				UTF_7 = 1012,		///< UTF-7.
				UTF_32 = 1017,		///< UTF-32.
				UTF_32BE = 1018,	///< UTF-32BE.
				UTF_32LE = 1019,	///< UTF-32LE.
				GB2312 = 2025,		///< GB2312.
				BIG5 = 2026,		///< Big5.
				VISCII = 2082,		///< VISCII (VIetnamese Standard Code for Information Interchange; RFC1456).
				VIQR = 2083,		///< VIQR (VIetnamese Quoted-Readable; RFC1456).
				KOI8_R = 2084,		///< KOI8-R.
				KOI8_U = 2088,		///< KOI8-U.
				TIS_620 = 2259;		///< TIS 620-2533:1999.
		}
#endif /* !ASCENSION_NO_STANDARD_ENCODINGS */

#ifndef ASCENSION_NO_PROPRIETARY_ENCODINGS
		/// MIBenum values of the proprietary encodings registered by IANA.
		namespace proprietary {
			const MIBenum
				IBM864 = 2051,			///< IBM864 (ibm-864_X110-1999).
				WINDOWS_1250 = 2250,	///< windows-1250.
				WINDOWS_1251 = 2251,	///< windows-1251.
				WINDOWS_1252 = 2252,	///< windows-1252.
				WINDOWS_1253 = 2253,	///< windows-1253.
				WINDOWS_1254 = 2254,	///< windows-1254.
				WINDOWS_1255 = 2255,	///< windows-1255.
				WINDOWS_1256 = 2256,	///< windows-1256.
				WINDOWS_1257 = 2257,	///< windows-1257.
				WINDOWS_1258 = 2258;	///< windows-1258.
		}
#endif /* !ASCENSION_NO_PROPRIETARY_ENCODINGS */

#ifndef ASCENSION_NO_EXTENDED_ENCODINGS
		/// MIBenum values of the extended encodings.
		/// @deprecated
		namespace extended {
			const MIBenum
				// Unicode
				UTF_5		= 3001,	///< UTF-5.
				// Armenian
				ARMSCII7	= 3020,	///< ARMSCII-7.
				ARMSCII8	= 3021,	///< ARMSCII-8.
				ARMSCII8A	= 3022,	///< ARMSCII-8A.
				// Vietnamese
				VISCII	= 2082,	///< VISCII (VIetnamese Standard Code for Information Interchange; RFC1456).
				VIQR	= 2083,	///< VIQR (VIetnamese Quoted-Readable; RFC1456).
				TCVN	= 3030,	///< TCVN.
				VPS		= 3031,	///< VPS.
				// Japanese
				ISO_2022_JP_1				= 3040,	///< ISO-2022-JP-1 (RFC and JIS X 0208:1997).
				ISO_2022_JP_2004			= 3041,	///< ISO-2022-JP-2004 (JIS X 0213:2004).
				ISO_2022_JP_2004_STRICT		= 3042,	///< ISO-2022-JP-2004-strict (Emacs).
				ISO_2022_JP_2004_COMPATIBLE	= 3043,	///< ISO-2022-JP-2004-compatible (Emacs).
				ISO_2022_JP_3				= 3044,	///< ISO-2022-JP-3 (JIS X 0213:2000).
				ISO_2022_JP_3_STRICT		= 3045,	///< ISO-2022-JP-3-strict (Emacs).
				ISO_2022_JP_3_COMPATIBLE	= 3046,	///< ISO-2022-JP-3-compatible (Emacs).
				SHIFT_JIS_2004				= 3047,	///< Shift_JIS-2004 (JIS X 0213:2004).
				EUC_JIS_2004				= 3048,	///< EUC-JIS-2004 (JIS X 0213:2004).
				// Tamil
				TAB		= 3080,	///< Tamil (TAB).
				TAM		= 3081,	///< Tamil (TAM).
//				TSCII	= 3082,	///< Tamil (TSCII 1.7).
				// Hindi
				MIB_HINDI_MACINTOSH	= 3090,	///< Hindi (Macintosh, Devanagari).
				// Gujarati
				MIB_GUJARATI_MACINTOSH	= 3100,	///< Gujarati (Macintosh).
				// Panjabi
				MIB_PANJABI_MACINTOSH	= 3110,	///< Punjabi (Macintosh, Gurumkhi).
				// Cyrillic
/*				CPEX_CYRILLIC_MACINTOSH							= 10007,	///< Cyrillic (Macintosh)
				CPEX_CYRILLIC_KOI8R								= 20866,	///< Russian (KOI8-R)
				CPEX_CYRILLIC_RUSSIANSUPPORTFORDOS3				= 70120,	///< Russian (Russian Support for DOS Version 3)
				CPEX_CYRILLIC_RUSSIANSUPPORTFORDOS4ACADEMIC		= 70121,	///< Russian (Russian Support for DOS Version 4 Academic)
				CPEX_CYRILLIC_RUSSIANSUPPORTFORDOS3NONACADEMIC	= 70122,	///< Russian (Russian support for DOS Version 4 Non-Academic)
				CPEX_CYRILLIC_SOVIETKOI8BASIC					= 70123,	///< Russian (Soviet KOI-8 Basic)
				CPEX_CYRILLIC_SOVIETKOI8ALTERNATIVE				= 70124,	///< Russian (Soviet KOI-8 Alternative)
				CPEX_CYRILLIC_SOVIETKOI7						= 70125,	///< Russian (Soviet KOI-7)
				CPEX_CYRILLIC_ECMA								= 70126,	///< Cyrillic (ISO-IR-111, ECMA)
				CPEX_CYRILLIC_KOI8RU							= 70127,	///< Cyrillic (KOI8-RU)
				CPEX_CYRILLIC_KOI8UNIFIED						= 70128,	///< Cyrillic (KOI8 Unified)
*/				// ISO-2022 multilingual
				MIB_MULTILINGUAL_ISO2022_7BIT		= 3120,	///< Multilingual (ISO-2022, 7-bit).
				MIB_MULTILINGUAL_ISO2022_7BITSS2	= 3121,	///< Multilingual (ISO-2022, 7-bit, SS2).
				MIB_MULTILINGUAL_ISO2022_7BITSISO	= 3122,	///< Multilingual (ISO-2022, 7-bit, SI/SO).
				MIB_MULTILINGUAL_ISO2022_8BITSS2	= 3123,	///< Multilingual (ISO-2022, 8-bit, SS2).
				// miscellaneous
				NEXTSTEP	= 3901,	///< NEXTSTEP.
				ATARIST		= 3902;	///< Atari ST/TT.
		}
#endif /* !ASCENSION_NO_EXTENDED_ENCODINGS */

		// These data are not terminated by NUL.
		const byte	UTF8_BOM[] = {0xEF, 0xBB, 0xBF};	///< BOM of UTF-8.
		const byte	UTF16LE_BOM[] = {0xFF, 0xFE};		///< BOM of UTF-16 little endian.
		const byte	UTF16BE_BOM[] = {0xFE, 0xFF};		///< BOM of UTF-16 big endian.
#ifndef ASCENSION_NO_STANDARD_ENCODINGS
		const byte	UTF32LE_BOM[] = {0xFF, 0xFF, 0x00, 0x00};	///< BOM of UTF-32 little endian.
		const byte	UTF32BE_BOM[] = {0xFE, 0xFF, 0x00, 0x00};	///< BOM of UTF-32 big endian.
#endif /* !ASCENSION_NO_STANDARD_ENCODINGS */

		template<typename T> inline byte mask7Bit(T c) {return static_cast<byte>(c & 0x7FU);}
		template<typename T> inline byte mask8Bit(T c) {return static_cast<byte>(c & 0xFFU);}
		template<typename T> inline ushort mask16Bit(T c) {return static_cast<ushort>(c & 0xFFFFU);}
		template<typename T> inline Char maskUCS2(T c) {return static_cast<Char>(c & 0xFFFFU);}

		/**
		 * Compares the given two encoding (charset) names based on
		 * <a href="http://www.unicode.org/reports/tr22/">UTS #22: CharMapML</a> 1.4 Charset Alias
		 * Matching.
		 */
		template<typename CharacterSequence1, typename CharacterSequence2>
		inline int compareEncodingNames(
				CharacterSequence1 first1, CharacterSequence1 last1,
				CharacterSequence2 first2, CharacterSequence2 last2) {
			const std::locale& lc = std::locale::classic();
			bool precededByDigit[2] = {false, false};
			while(first1 != last1 && first2 != last2) {
				if(*first1 == '0' && !precededByDigit[0]) ++first1;
				else if(!std::isalnum(*first1, lc)) {++first1; precededByDigit[0] = false;}
				else if(*first2 == '0' && !precededByDigit[1]) ++first2;
				else if(!std::isalnum(*first2, lc)) {++first2; precededByDigit[1] = false;}
				else {
					if(const int d = std::tolower(*first1, lc) - std::tolower(*first2, lc)) return d;
					precededByDigit[0] = std::isdigit(*(first1++), lc);
					precededByDigit[1] = std::isdigit(*(first2++), lc);
				}
			}
			if(first1 != last1) return 1;
			else return (first2 == last2) ? 0 : -1;
		}

		MIBenum	convertCCSIDtoMIB(uint ccsid) throw();
		uint	convertMIBtoCCSID(MIBenum mib) throw();
#ifdef ASCENSION_WINDOWS
		uint	convertMIBtoWinCP(MIBenum mib) throw();
		MIBenum	convertWinCPtoMIB(uint codePage) throw();
#endif /* ASCENSION_WINDOWS */
		String	getEncodingDisplayName(MIBenum mib);

		/// Thrown if the specified encoding is not supported.
		class UnsupportedEncodingException : public std::invalid_argument {
		public:
			explicit UnsupportedEncodingException(MIBenum mib);
			MIBenum mibEnum() const throw();
		private:
			const MIBenum mib_;
		};

		class IEncodingProperties {
		public:
			/**
			 * Returns the aliases of the encoding. Default implementation returns an empty.
			 * @return a string contains aliases separated by vertical bar ('|')
			 */
			virtual std::string aliases() const throw() {return "";}
			/**
			 * Returns the human-readable name of the encoding. Default implementation calls
			 * @c #name method.
			 * @param locale the locale used to localize the name
			 * @see #name
			 */
			virtual std::string displayName(const std::locale& locale) const throw() {return name();}
			/// Returns the number of bytes represents a UCS character.
			virtual std::size_t maximumNativeBytes() const throw() = 0;
			/// Returns the number of UCS characters represents a native character. Default
			/// implementation returns 1.
			virtual std::size_t maximumUCSLength() const throw() {return 1;}
			/// Returns the MIBenum value of the encoding.
			virtual MIBenum mibEnum() const throw() = 0;
			/**
			 * Returns the name of the encoding. If the encoding is registered as a character set
			 * in <a href="http://www.iana.org/assignments/character-sets">IANA character-sets
			 * encoding file</a>, should return the preferred mime name.
			 * @see #displayName
			 */
			virtual std::string name() const throw() = 0;
			/**
			 * Returns an native character which indicates that the given Unicode character can't
			 * map. If @c #policy returns @c REPLACE_UNMAPPABLE_CHARACTER, the encoder should use
			 * this character. Default implementation returns 0x1A.
			 */
			virtual byte substitutionCharacter() const throw() {return 0x1A;}
		};

		class EncoderFactory;

		class Encoder {
			MANAH_NONCOPYABLE_TAG(Encoder);
		public:
			/**
			 * A value represents intermediate states of a encoder. The sematics depend on each
			 * specific encoder implementations. But the all members are filled with zero initially.
			 */
			struct State {
				/// Implementation dependent data.
				std::size_t states[4];
				/// Constructor initializes the value with zero.
				State() throw() {std::memset(states, 0, sizeof(std::size_t) * countof(states));}
			};

			/// Result of conversion.
			enum Result {
				/// The conversion fully succeeded. If @a fromNext parameter of the conversion
				/// method is less @a fromEnd, more input is required.
				COMPLETED,
				/// The conversion partially succeeded because the destination buffer was not large
				/// enough.
				INSUFFICIENT_BUFFER,
				/// The conversion partially succeeded because encounted an unmappable character.
				/// @c fromNext parameter of the conversion method should addresses the unmappable
				/// character. If either @c REPLACE_UNMAPPABLE_CHARACTER or
				/// @c IGNORE_UNMAPPABLE_CHARACTER is set, this value never be returned.
				UNMAPPABLE_CHARACTER,
				/// The conversion partially succeeded because detected malformed input.
				/// @c fromNext parameter of the conversion method should addresses the unmappable
				/// character. @c Encoder#fromUnicode should not return this value.
				MALFORMED_INPUT
			};

			/// Conversion policies.
			/// @see #policy, #setPolicy
			enum Policy {
				/// Nothing.
				NO_POLICY,
//				NO_UNICODE_BYTE_ORDER_SIGNATURE	= 0x8000,
				/// Replaces a unmappable character with default replacement character.
				REPLACE_UNMAPPABLE_CHARACTER,
				/// Skips (ignores) unmappable characters.
				IGNORE_UNMAPPABLE_CHARACTER
			};
		public:
			virtual ~Encoder() throw();
			// attributes
			Policy								policy() const throw();
			virtual const IEncodingProperties&	properties() const throw() = 0;
			Encoder&							setPolicy(Policy newPolicy);
			// operations
			bool		canEncode(CodePoint c) const;
			bool		canEncode(const Char* first, const Char* last) const;
			bool		canEncode(const String& s) const;
			Result		fromUnicode(byte* to, byte* toEnd, byte*& toNext,
							const Char* from, const Char* fromEnd, const Char*& fromNext, State* state = 0) const;
			std::string	fromUnicode(const String& from) const;
			Result		toUnicode(Char* to, Char* toEnd, Char*& toNext,
							const byte* from, const byte* fromEnd, const byte*& fromNext, State* state = 0) const;
			String		toUnicode(const std::string& from) const;
			// factory
			static std::auto_ptr<Encoder>	forCCSID(int ccsid) throw();
			static std::auto_ptr<Encoder>	forCPGID(int cpgid) throw();
			static std::auto_ptr<Encoder>	forID(std::size_t id) throw();
			static std::auto_ptr<Encoder>	forMIB(MIBenum mib) throw();
			static std::auto_ptr<Encoder>	forName(const std::string& name) throw();
			static std::auto_ptr<Encoder>	forWindowsCodePage(uint codePage) throw();
			static bool						supports(MIBenum mib) throw();
			static bool						supports(const std::string& name) throw();
			template<typename OutputIterator>
			static void		availableEncodings(OutputIterator out);
			static Encoder&	getDefault() throw();
			static void		registerFactory(EncoderFactory& newFactory);
		protected:
			Encoder() throw();
			/**
			 * Converts the given string from UTF-16 into the native encoding.
			 * @param[out] to the beginning of the destination buffer
			 * @param[out] toEnd the end of the destination buffer
			 * @param[out] toNext points the first unaltered character in the destination buffer after the conversion
			 * @param[in] from the beginning of the buffer to be converted
			 * @param[in] fromEnd the end of the buffer to be converted
			 * @param[in] fromNext points to the first unconverted character after the conversion
			 * @param[in,out] state the conversion state. may be @c null
			 * @return the result of the conversion
			 */
			virtual Result doFromUnicode(byte* to, byte* toEnd, byte*& toNext,
				const Char* from, const Char* fromEnd, const Char*& fromNext, State* state) const = 0;
			/**
			 * Converts the given string from the native encoding into UTF-16.
			 * @param[out] to the beginning of the destination buffer
			 * @param[out] toEnd the end of the destination buffer
			 * @param[out] toNext points the first unaltered character in the destination buffer after the conversion
			 * @param[in] from the beginning of the buffer to be converted
			 * @param[in] fromEnd the end of the buffer to be converted
			 * @param[in] fromNext points to the first unconverted character after the conversion
			 * @param[in,out] state the conversion state. may be @c null
			 * @return the result of the conversion
			 */
			virtual Result doToUnicode(Char* to, Char* toEnd, Char*& toNext,
				const byte* from, const byte* fromEnd, const byte*& fromNext, State* state) const = 0;
		private:
			static EncoderFactory* find(MIBenum mib) throw();
			static EncoderFactory* find(const std::string& name) throw();
			static std::vector<EncoderFactory*>& registry();
			Policy policy_;
		};

		/// A factory class creates @c Encoder instances.
		class EncoderFactory : public IEncodingProperties {
		public:
			/// Destructor.
			virtual ~EncoderFactory() throw() {}
		protected:
			/// Returns the @c Encoder instance.
			virtual std::auto_ptr<Encoder>	create() const throw() = 0;
			friend class Encoder;
		};

		class EncodingDetector {
			MANAH_NONCOPYABLE_TAG(EncodingDetector);
		public:
			// constructors
			virtual ~EncodingDetector() throw();
			// attributes
			/// Returns the name of the encoding detector.
			std::string name() const throw() {return name_;}
			// detection
			MIBenum	detect(const byte* first, const byte* last, std::ptrdiff_t* convertibleBytes) const;
			// factory
			static EncodingDetector*	forName(const std::string& name) throw();
#ifdef ASCENSION_WINDOWS
			static EncodingDetector*	forWindowsCodePage(::UINT codePage) throw();
#endif /* ASCENSION_WINDOWS */
			template<typename OutputIterator>
			static void		availableNames(OutputIterator out);
			static void		registerDetector(std::auto_ptr<EncodingDetector> newDetector);
		protected:
			explicit EncodingDetector(const std::string& name);
			/**
			 * Detects the encoding of the given character sequence.
			 * @param first the beginning of the sequence
			 * @param last the end of the sequence
			 * @param[out] convertibleBytes the number of bytes (from @a first) absolutely
			 * detected. the value can't exceed the result of (@a last - @a first). may be @c null
			 * @return the MIBenum value of the detected encoding
			 */
			virtual MIBenum doDetect(const byte* first, const byte* last, std::ptrdiff_t* convertibleBytes) const throw() = 0;
		private:
			static std::vector<EncodingDetector*>& registry();
			const std::string name_;
		};


		/// Supports implementation of encoder classes.
		namespace implementation {
			/// @c EncoderFactoryBase is a base implementation of @c EncoderFactory.
			class EncoderFactoryBase : public EncoderFactory {
			public:
				virtual ~EncoderFactoryBase() throw();
			protected:
				EncoderFactoryBase(const std::string& name,
					MIBenum mib, const std::string& displayName = "",
					std::size_t maximumNativeBytes = 1, std::size_t maximumUCSLength = 1,
					const std::string& aliases = "", byte substitutionCharacter = 0x1A);
			protected:
				// IEncodingProperties
				virtual std::string	aliases() const throw();
				virtual std::string	displayName(const std::locale& lc) const throw();
				virtual std::size_t	maximumNativeBytes() const throw();
				virtual std::size_t	maximumUCSLength() const throw();
				virtual MIBenum		mibEnum() const throw();
				virtual std::string	name() const throw();
				virtual byte		substitutionCharacter() const throw();
			private:
				const std::string name_, displayName_, aliases_;
				const std::size_t maximumNativeBytes_, maximumUCSLength_;
				const MIBenum mib_;
				const byte substitutionCharacter_;
			};

			namespace sbcs {
				/// A substitution byte value in used in Unicode to native mapping table.
				const byte UNMAPPABLE_BYTE = 0x00;

				/// A mapping table from bytes into characters.
				class ByteMap {
				public:
					explicit ByteMap(const Char** values) : values_(values) {}
					Char operator[](byte c) const throw() {return values_[c >> 4][c & 0xF];}
				private:
					const Char** values_;
				};

				/// Generates 16-character sequence.
				template<
					Char c0, Char c1, Char c2, Char c3, Char c4, Char c5, Char c6, Char c7,
					Char c8, Char c9, Char cA, Char cB, Char cC, Char cD, Char cE, Char cF>
				struct ByteLine {static const Char values[16];};

				/// Generates 16×16 character sequence.
				template<
					typename Line0, typename Line1, typename Line2, typename Line3,
					typename Line4, typename Line5, typename Line6, typename Line7,
					typename Line8, typename Line9, typename LineA, typename LineB,
					typename LineC, typename LineD, typename LineE, typename LineF>
				class ByteTable : public ByteMap {
				public:
					ByteTable() throw() : ByteMap(values_) {}
				private:
					static const Char* values_[16];
				};

				/// Generates ISO IR C0 character sequence 0x00 through 0x0F.
				typedef ByteLine<
					0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
					0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F> ISO_IR_C0_LINE0;
				/// Generates ISO IR C0 character sequence 0x10 through 0x1F.
				typedef ByteLine<
					0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
					0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F> ISO_IR_C0_LINE1;
				/// Generates ISO IR C1 character sequence 0x80 through 0x8F.
				typedef ByteLine<
					0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
					0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F> ISO_IR_C1_LINE8;
				/// Generates ISO IR C1 character sequence 0x90 through 0x9F.
				typedef ByteLine<
					0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
					0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F> ISO_IR_C1_LINE9;

				/// Generates 16×16 character table compatible with ISO 646.
				template<
					typename Line8, typename Line9, typename LineA, typename LineB,
					typename LineC, typename LineD, typename LineE, typename LineF>
				class ASCIICompatibleByteTable : public ByteTable<
					ISO_IR_C0_LINE0, ISO_IR_C0_LINE1,
					ByteLine<0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F>,
					ByteLine<0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F>,
					ByteLine<0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F>,
					ByteLine<0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F>,
					ByteLine<0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F>,
					ByteLine<0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F>,
					Line8, Line9, LineA, LineB, LineC, LineD, LineE, LineF> {};

				/// Generates 16×16 character table compatible with ISO-IR.
				template<
					typename Line2, typename Line3, typename Line4, typename Line5, typename Line6, typename Line7,
					typename LineA, typename LineB, typename LineC, typename LineD, typename LineE, typename LineF>
				class ISOIRByteTable : public ByteTable<
					ISO_IR_C0_LINE0, ISO_IR_C0_LINE1, Line2, Line3, Line4, Line5, Line6, Line7,
					ISO_IR_C1_LINE8, ISO_IR_C1_LINE9, LineA, LineB, LineC, LineD, LineE, LineF> {};

				/// Generates 16×16 character table compatible with ISO 646.
				template<typename LineA, typename LineB, typename LineC, typename LineD, typename LineE, typename LineF>
				class ISO646CompatibleByteTable : public ISOIRByteTable<
					ByteLine<0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F>,
					ByteLine<0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F>,
					ByteLine<0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F>,
					ByteLine<0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F>,
					ByteLine<0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F>,
					ByteLine<0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F>,
					LineA, LineB, LineC, LineD, LineE, LineF> {};

				/// Generates 16×16 character table compatible with IBM PC code page.
				template<
					typename Line8, typename Line9, typename LineA, typename LineB,
					typename LineC, typename LineD, typename LineE, typename LineF>
				class IBMPCCompatibleByteTable : public ByteTable<
					ByteLine<0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F>,
					ByteLine<0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1C, 0x1B, 0x7F, 0x1D, 0x1E, 0x1F>,
					ByteLine<0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F>,
					ByteLine<0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F>,
					ByteLine<0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F>,
					ByteLine<0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F>,
					ByteLine<0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F>,
					ByteLine<0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x1A>,
					Line8, Line9, LineA, LineB, LineC, LineD, LineE, LineF> {};

				/// Base class of single byte charset encoder factories.
				template<typename MappingTable>
				class SingleByteEncoderFactory : public EncoderFactoryBase {
				public:
					SingleByteEncoderFactory(const std::string& name, MIBenum mib,
						const std::string& displayName, const std::string& aliases, byte substitutionCharacter);
					virtual ~SingleByteEncoderFactory() throw();
				private:
					std::auto_ptr<Encoder>	create() const throw();
				};

				namespace internal {
					std::auto_ptr<Encoder> createSingleByteEncoder(const sbcs::ByteMap& table, const IEncodingProperties& properties) throw();
				}
			} // namespace sbcs

/*			/// Base class of 7-bit ISO-2022 encoders.
			class ISO2022Encoder : public EncoderBase {
			public:
				virtual ~ISO2022Encoder() throw();
			protected:
				static const byte SO = 0x0E;	///< Shift out.
				static const byte SI = 0x0F;	///< Shift in.
				static const byte ESC = 0x1B;	///< Escape.
				static const byte SS2 = 0x8E;	///< 8-bit single shift 2.
				static const byte SS3 = 0x8F;	///< 8-bit single shift 3.
			protected:
				ISO2022Encoder(const std::string& name, MIBenum mib, const std::string& aliases, byte substitutionCharacter);
				byte	currentState() const throw();
				void	designate(std::size_t gn, byte charset);
				void	shiftIn();
				void	shiftOut();
				void	ss2();
				void	ss3();
			private:
				byte gl_, gr_;
				byte g_[4];
				bool shiftOuted_;
				int currentState_;
			};
*/		}


		/**
		 * Returns informations for all available encodings.
		 * @param[out] out the output iterator to receive pairs consist of the enumeration
		 * identifier and the encoding information. the expected type of the pair is
		 * @c std#pair<std::size_t, const IEncodingProperties*>. the enumeration identifier can be
		 * used with @c #forID method.
		 */
		template<typename OutputIterator> inline void Encoder::availableEncodings(OutputIterator out) {
			for(std::size_t i = 0, c = registry().size(); i < c; ++i, ++out) *out = std::make_pair<std::size_t, const IEncodingProperties*>(i, registry()[i]);}

		/// Returns the conversion policy.
		inline Encoder::Policy Encoder::policy() const throw() {return policy_;}

		/**
		 * Returns names for all available encoding detectors.
		 * @param[out] out the output iterator to receive names
		 */
		template<typename OutputIterator> inline void EncodingDetector::availableNames(OutputIterator out) {
			for(std::vector<EncodingDetector*>::const_iterator i(registry().begin()), e(registry().end()); i != e; ++i, ++out) *out = (*i)->name();}

		template<
			Char c0, Char c1, Char c2, Char c3, Char c4, Char c5, Char c6, Char c7,
			Char c8, Char c9, Char cA, Char cB, Char cC, Char cD, Char cE, Char cF>
		const Char implementation::sbcs::ByteLine<c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, cA, cB, cC, cD, cE, cF>::values[16] = {
			c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, cA, cB, cC, cD, cE, cF};

		template<
			typename Line0, typename Line1, typename Line2, typename Line3, typename Line4, typename Line5, typename Line6, typename Line7,
			typename Line8, typename Line9, typename LineA, typename LineB, typename LineC, typename LineD, typename LineE, typename LineF>
		const Char* implementation::sbcs::ByteTable<
			Line0, Line1, Line2, Line3, Line4, Line5, Line6, Line7, Line8, Line9, LineA, LineB,LineC, LineD, LineE, LineF>::values_[16] = {
			Line0::values, Line1::values, Line2::values, Line3::values, Line4::values, Line5::values, Line6::values, Line7::values,
			Line8::values, Line9::values, LineA::values, LineB::values, LineC::values, LineD::values, LineE::values, LineF::values};

		/**
		 * Constructor.
		 * @param name the name returned by @c #name
		 * @param mib the MIBenum value returned by @c #mibEnum
		 * @param displayName the display name returned by @c #displayName
		 * @param aliases the encoding aliases returned by @c #aliases
		 * @param substitutionCharacter the native character returned by @c #substitutionCharacter
		 */
		template<typename MappingTable>
		inline implementation::sbcs::SingleByteEncoderFactory<MappingTable>::SingleByteEncoderFactory(const std::string& name, MIBenum mib,
			const std::string& displayName, const std::string& aliases, byte substitutionCharacter) : EncoderFactoryBase(name, mib, displayName, 1, 1, aliases, substitutionCharacter) {}

		/// Destructor.
		template<typename MappingTable> inline implementation::sbcs::SingleByteEncoderFactory<MappingTable>::~SingleByteEncoderFactory() throw() {}

		/// @see EncoderFactory#create
		template<typename MappingTable> std::auto_ptr<Encoder> implementation::sbcs::SingleByteEncoderFactory<MappingTable>::create() const throw() {
			MappingTable table; return implementation::sbcs::internal::createSingleByteEncoder(table, *this);}
	}
} // namespace ascension.encoding

#endif /* !ASCENSION_ENCODER_HPP */
