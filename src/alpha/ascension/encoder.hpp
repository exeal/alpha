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
#include <locale>	// std.codecvt
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
				UTF_16LE = 1014;	///< UTF-16LE.
//				UTF_16 = 1015;		///< UTF-16.
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
				GB2312 = 2025,		///< GB2312.
				BIG5 = 2026,		///< Big5.
				KOI8_R = 2084;		///< KOI8-R.
		}
#endif /* !ASCENSION_NO_STANDARD_ENCODINGS */

#ifndef ASCENSION_NO_EXTENDED_ENCODINGS
		/// MIBenum values of the extended encodings.
		namespace extended {
			const MIBenum
				// Unicode
				UTF_5		= 3001,	///< UTF-5.
				UTF_7		= 1012,	///< UTF-7 (RFC2152).
				UTF_32BE	= 1018,	///< UTF-32BE.
				UTF_32LE	= 1019,	///< UTF-32LE.
				// Latin
				ISO_8859_13	= 109,	///< ISO-8859-13.
				ISO_8859_14	= 110,	///< ISO-8859-14:1998.
				ISO_8859_15	= 111,	///< ISO-8859-15.
				ISO_8859_16	= 112,	///< ISO-8859-16:2001.
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
				// Thai
				TIS620		= 2259,	///< TIS 620-2533:1990.
				ISO_8859_11	= 3060,	///< ISO-8859-11.
				// Lao
				MULE_LAO	= 3065,	///< MuleLao-1.
				CP1132		= 3066,	///< IBM1132.
				CP1133		= 3067,	///< IBM1133.
				// Irelandic
				IS434	= 3070,	///< Irelandic (I.S. 434:1999).
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
#ifndef ASCENSION_NO_EXTENDED_ENCODINGS
		const byte	UTF32LE_BOM[] = {0xFF, 0xFF, 0x00, 0x00};	///< BOM of UTF-16 little endian.
		const byte	UTF32BE_BOM[] = {0xFE, 0xFF, 0x00, 0x00};	///< BOM of UTF-16 big endian.
#endif /* !ASCENSION_NO_EXTENDED_ENCODINGS */

		template<typename T> inline byte mask7Bit(T c) {return static_cast<byte>(c) & 0x7FU;}
		template<typename T> inline byte mask8Bit(T c) {return static_cast<byte>(c) & 0xFFU;}
		template<typename T> inline ushort mask16Bit(T c) {return static_cast<ushort>(c) & 0xFFFFU;}
		template<typename T> inline Char maskUCS2(T c) {return static_cast<Char>(c) & 0xFFFFU;}

		/**
		 * Compares the given two encoding (charset) names based on
		 * <a href="http://www.unicode.org/reports/tr22/">UTS #22: CharMapML</a> 1.4 Charset Alias
		 * Matching.
		 */
		template<typename CharacterSequence>
		inline bool matchEncodingNames(CharacterSequence first1, CharacterSequence last1, CharacterSequence first2, CharacterSequence last2) {
			const std::locale& lc = std::locale::classic();
			bool precededByDigit[2] = {false, false};
			while(first1 != last1 && first2 != last2) {
				if(*first1 == '0' && !precededByDigit[0]) ++first1;
				else if(!std::isalnum(*first1, lc)) {++first1; precededByDigit[0] = false;}
				else if(*first2 == '0' && !precededByDigit[1]) ++first2;
				else if(!std::isalnum(*first2, lc)) {++first2; precededByDigit[1] = false;}
				else {
					if(std::tolower(*first1, lc) != std::tolower(*first2, lc))
						return false;
					precededByDigit[0] = std::isdigit(*(first1++), lc);
					precededByDigit[1] = std::isdigit(*(first2++), lc);
				}
			}
			return first1 == last1 && first2 == last2;
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

		class Encoder {
			MANAH_NONCOPYABLE_TAG(Encoder);
		public:
			/**
			 * A value represents intermediate states of a encoder. The sematics are defined by
			 * each specific encoder implementations, but should treat zero as the initial state.
			 */
			class State {
			public:
				/// Constructor initializes the value with zero.
				State() throw() : value_(0) {}
				/// Conversion operator.
				operator int() const throw() {return value_;}
				/// Assignment operator takes an integer.
				State& operator=(int newValue) throw() {value_ = newValue; return *this;}
			private:
				int value_;
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
			// concrete attributes
			Policy		policy() const throw();
			Encoder&	setPolicy(Policy newPolicy);
			// abstract attributes
			/**
			 * Returns the aliases of the encoding. Default implementation returns an empty.
			 * @return a string contains aliases separated by NUL
			 */
			virtual std::string aliases() const throw() {return "";}
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
			 */
			virtual std::string name() const throw() = 0;
			/**
			 * Returns an native character which indicates that the given Unicode character can't
			 * map. If @c #policy returns @c REPLACE_UNMAPPABLE_CHARACTER, the encoder should use
			 * this character. Default implementation returns 0x1A.
			 */
			virtual byte substitutionCharacter() const throw() {return 0x1A;}
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
			static Encoder*	forCCSID(int ccsid) throw();
			static Encoder*	forCPGID(int cpgid) throw();
			static Encoder*	forMIB(MIBenum mib) throw();
			static Encoder*	forName(const std::string& name) throw();
#ifdef ASCENSION_WINDOWS
			static Encoder*	forWindowsCodePage(::UINT codePage) throw();
#endif /* ASCENSION_WINDOWS */
			template<typename OutputIterator>
			static void		availableMIBs(OutputIterator out);
			template<typename OutputIterator>
			static void		availableNames(OutputIterator out);
			static Encoder&	getDefault() throw();
			static void		registerEncoder(std::auto_ptr<Encoder> newEncoder);
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
			static std::set<Encoder*>& registry();
			Policy policy_;
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
			static std::set<EncodingDetector*>& registry();
			const std::string name_;
		};


		/// Supports implementation of encoder classes.
		namespace implementation {
			/// @c EncoderBase is a base implementation of @c Encoder, which defines the methods
			/// describe the properties of the encoding.
			class EncoderBase : public Encoder {
			public:
				virtual ~EncoderBase() throw();
			protected:
				EncoderBase(const std::string& name, MIBenum mib,
					std::size_t maximumNativeBytes = 1, std::size_t maximumUCSLength = 1,
					const std::string& aliases = "", byte substitutionCharacter = 0x1A);
			protected:
				virtual std::string	aliases() const throw();
				virtual std::size_t	maximumNativeBytes() const throw();
				virtual std::size_t	maximumUCSLength() const throw();
				virtual MIBenum		mibEnum() const throw();
				virtual std::string	name() const throw();
				virtual byte		substitutionCharacter() const throw();
			private:
				const std::string name_, aliases_;
				const std::size_t maximumNativeBytes_, maximumUCSLength_;
				const MIBenum mib_;
				const byte substitutionCharacter_;
			};

			/// Base class of single byte charset encoders.
			class SingleByteEncoder : public EncoderBase {
			public:
				static const byte UNMAPPABLE_BYTE;
			public:
				SingleByteEncoder(const std::string& name, MIBenum mib, const std::string& aliases,
					byte substitutionCharacter, const Char native8ToUnicode[0x80], const Char native7ToUnicode[0x80] = 0);
				virtual ~SingleByteEncoder() throw();
			private:
				void	buildUnicodeToNativeTable();
				// Encoder
				Result	doFromUnicode(byte* to, byte* toEnd, byte*& toNext,
					const Char* from, const Char* fromEnd, const Char*& fromNext, State* state) const;
				Result	doToUnicode(Char* to, Char* toEnd, Char*& toNext,
					const byte* from, const byte* fromEnd, const byte*& fromNext, State* state) const;
			private:
				const Char* const native7ToUnicode_;
				const Char* const native8ToUnicode_;
				byte* unicodeToNative_[0x100];
				static const Char ASCII_TABLE[0x80];
				static const byte UNMAPPABLE_16x16_UNICODE_TABLE[0x100];
			};
		}


		// implementation macros

#define ASCENSION_INCREMENTAL_BYTE_SEQUENCE_C0														\
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,	\
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F

#define ASCENSION_INCREMENTAL_BYTE_SEQUENCE_C1														\
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,	\
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F

#define ASCENSION_INCREMENTAL_BYTE_SEQUENCE_7BIT													\
	ASCENSION_INCREMENTAL_BYTE_SEQUENCE_C0,															\
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,	\
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,	\
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,	\
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,	\
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,	\
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F

#define ASCENSION_INCREMENTAL_BYTE_SEQUENCE_8BIT													\
	ASCENSION_INCREMENTAL_BYTE_SEQUENCE_C1,															\
	0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,	\
	0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,	\
	0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,	\
	0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,	\
	0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,	\
	0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF


		/**
		 * Returns MIBs for all available encodings.
		 * @param[out] out the output iterator to receive MIBs
		 */
		template<typename OutputIterator> inline void Encoder::availableMIBs(OutputIterator out) {
			for(std::set<Encoder*>::const_iterator i(registry().begin()), e(registry().end()); i != e; ++i, ++out) {
				const MIBenum mib = (*i)->mibEnum();
				if(mib > MIB_UNKNOWN)
					*out = (*i)->mibEnum();
			}
		}

		/**
		 * Returns names for all available encodings.
		 * @param[out] out the output iterator to receive names
		 */
		template<typename OutputIterator> inline void Encoder::availableNames(OutputIterator out) {
			for(std::set<Encoder*>::const_iterator i(registry().begin()), e(registry().end()); i != e; ++i, ++out) *out = (*i)->name();}

		/// Returns the conversion policy.
		inline Encoder::Policy Encoder::policy() const throw() {return policy_;}

		/**
		 * Returns names for all available encoding detectors.
		 * @param[out] out the output iterator to receive names
		 */
		template<typename OutputIterator> inline void EncodingDetector::availableNames(OutputIterator out) {
			for(std::set<EncodingDetector*>::const_iterator i(registry().begin()), e(registry().end()); i != e; ++i, ++out) *out = (*i)->name();}
	}
} // namespace ascension.encoding

#endif /* !ASCENSION_ENCODER_HPP */
