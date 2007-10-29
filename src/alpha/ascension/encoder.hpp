/**
 * @file encoder.hpp
 * @author exeal
 * @date 2004-2007
 */

#ifndef ASCENSION_ENCODER_HPP
#define ASCENSION_ENCODER_HPP
#include "unicode.hpp"
#include <cassert>
#include <set>
#include <map>
#include <memory>	// std.auto_ptr


namespace ascension {

	/// Members of the namespace @c encoding provide conversions between text encodings.
	namespace encoding {

		/**
		 * "The MIBenum value is a unique value for use in MIBs to identify coded character sets"
		 * (http://www.iana.org/assignments/character-sets).
		 *
		 * Ascension defines own several encodings not registered by IANA. They have MIBenum
		 * values whose regions are 3000-3999. These values are subject to change in the future.
		 * @see MIB_MINIMUM, MIB_MAXIMUM, AutoDetector#ID
		 */
		typedef ushort MIBenum;

		/// Minimum value of @c MIBenum.
		/// Ascension never define an encoding has MIBenum less than this.
		const MIBenum MIB_MINIMUM = 0;
		/// Maximum value of @c MIBenum.
		/// Ascension never define an encoding has MIBenum greater than this.
		const MIBenum MIB_MAXIMUM = 3999;

		/// MIBenum values of the fundamental encodings.
		namespace fundamental {
			const MIBenum
				MIB_US_ASCII = 3,			///< ANSI X3.4:1968.
				MIB_ISO_8859_1 = 4,			///< ISO-8859-1:1987.
				MIB_UNICODE_UTF8 = 106,		///< UTF-8.
				MIB_UNICODE_UTF16BE = 1013,	///< UTF-16BE.
				MIB_UNICODE_UTF16LE = 1014;	///< UTF-16LE.
//				MIB_UNICODE_UTF16 = 1015;	///< UTF-16.
		}

#ifndef ASCENSION_NO_STANDARD_ENCODINGS
		/// MIBenum values of the standard encodings.
		namespace standard {
			const MIBenum
				MIB_ISO_8859_2 = 5,		///< ISO-8859-2:1987.
				MIB_ISO_8859_3 = 6,		///< ISO-8859-3:1988.
				MIB_ISO_8859_4 = 7,		///< ISO-8859-4:1988.
				MIB_ISO_8859_5 = 8,		///< ISO-8859-5:1988.
				MIB_ISO_8859_6 = 9,		///< ISO-8859-6:1987.
				MIB_ISO_8859_7 = 10,	///< ISO-8859-7:1987.
				MIB_ISO_8859_8 = 11,	///< ISO-8859-8:1988.
				MIB_ISO_8859_9 = 12,	///< ISO-8859-9:1989.
				MIB_ISO_8859_10 = 13,	///< ISO-8859-10:1992.
				MIB_SHIFT_JIS = 17,		///< Shift_JIS (JIS X 0208:1997).
				MIB_EUC_JP = 18,		///< EUC-JP (JIS X 0208:1997 and JIS X 0212:1990).
				MIB_ISO_2022_KR = 37,	///< ISO-2022-KR.
				MIB_EUC_KR = 38,		///< EUC-KR.
				MIB_ISO_2022_JP = 39,	///< ISO-2022-JP (RFC1468 and JIS X 0208:1997).
				MIB_ISO_2022_JP_2 = 40,	///< ISO-2022-JP-2 (RFC1554 and JIS X 0212:1990).
				MIB_ISO_8859_6_E = 81,	///< ISO-8859-6-E.
				MIB_ISO_8859_6_I = 82,	///< ISO-8859-6-I.
				MIB_ISO_8859_8_E = 84,	///< ISO-8859-8-E.
				MIB_ISO_8859_8_I = 85,	///< ISO-8859-8-I.
				MIB_GB2312 = 2025,		///< GB2312.
				MIB_BIG5 = 2026,		///< Big5.
				MIB_KOI8_R = 2084;		///< KOI8-R.
		}
#endif /* !ASCENSION_NO_STANDARD_ENCODINGS */

#ifndef ASCENSION_NO_EXTENDED_ENCODINGS
		/// MIBenum values of the extended encodings.
		namespace extended {
			const MIBenum
				// Unicode
				MIB_UNICODE_UTF5	= 3001,	///< UTF-5.
				MIB_UNICODE_UTF7	= 1012,	///< UTF-7 (RFC2152).
				MIB_UNICODE_UTF32BE	= 1018,	///< UTF-32BE.
				MIB_UNICODE_UTF32LE	= 1019,	///< UTF-32LE.
				// Armenian
				MIB_ARMENIAN_ARMSCII7	= 3020,	///< ARMSCII-7.
				MIB_ARMENIAN_ARMSCII8	= 3021,	///< ARMSCII-8.
				MIB_ARMENIAN_ARMSCII8A	= 3022,	///< ARMSCII-8A.
				// Vietnamese
				MIB_VIETNAMESE_VISCII	= 2082,	///< VISCII (VIetnamese Standard Code for Information Interchange; RFC1456).
				MIB_VIETNAMESE_VIQR		= 2083,	///< VIQR (VIetnamese Quoted-Readable; RFC1456).
				MIB_VIETNAMESE_TCVN		= 3030,	///< TCVN.
				MIB_VIETNAMESE_VPS		= 3031,	///< VPS.
				// Japanese
				MIB_JAPANESE_ISO_2022_JP_1					= 3040,	///< ISO-2022-JP-1 (RFC and JIS X 0208:1997).
				MIB_JAPANESE_ISO_2022_JP_2004				= 3041,	///< ISO-2022-JP-2004 (JIS X 0213:2004).
				MIB_JAPANESE_ISO_2022_JP_2004_STRICT		= 3042,	///< ISO-2022-JP-2004-strict (Emacs).
				MIB_JAPANESE_ISO_2022_JP_2004_COMPATIBLE	= 3043,	///< ISO-2022-JP-2004-compatible (Emacs).
				MIB_JAPANESE_ISO_2022_JP_3					= 3044,	///< ISO-2022-JP-3 (JIS X 0213:2000).
				MIB_JAPANESE_ISO_2022_JP_3_STRICT			= 3045,	///< ISO-2022-JP-3-strict (Emacs).
				MIB_JAPANESE_ISO_2022_JP_3_COMPATIBLE		= 3046,	///< ISO-2022-JP-3-compatible (Emacs).
				MIB_JAPANESE_SHIFT_JIS_2004					= 3047,	///< Shift_JIS-2004 (JIS X 0213:2004).
				MIB_JAPANESE_EUC_JIS_2004					= 3048,	///< EUC-JIS-2004 (JIS X 0213:2004).
				// Thai
				MIB_THAI_TIS620			= 2259,	///< TIS 620-2533:1990.
				MIB_THAI_ISO_8859_11	= 3060,	///< ISO-8859-11.
				// Lao
				MIB_LAO_MULE_LAO	= 3065,	///< MuleLao-1.
				MIB_LAO_CP1132		= 3066,	///< IBM1132.
				MIB_LAO_CP1133		= 3067,	///< IBM1133.
				// Irelandic
				MIB_IRISH_IS434	= 3070,	///< Irelandic (I.S. 434:1999).
				// Tamil
				MIB_TAMIL_TAB	= 3080,	///< Tamil (TAB).
				MIB_TAMIL_TAM	= 3081,	///< Tamil (TAM).
//				MIB_TAMIL_TSCII	= 3082,	///< Tamil (TSCII 1.7).
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
				MIB_MISCELLANEOUS_BINARY	= 3900,	///< Binary.
				MIB_MISCELLANEOUS_NEXTSTEP	= 3901,	///< NEXTSTEP.
				MIB_MISCELLANEOUS_ATARIST	= 3902;	///< Atari ST/TT.
		}
#endif /* !ASCENSION_NO_EXTENDED_ENCODINGS */

		// These data are not terminated by NUL.
		const uchar	UTF8_BOM[] = {0xEF, 0xBB, 0xBF};	///< BOM of UTF-8.
		const uchar	UTF16LE_BOM[] = {0xFF, 0xFE};		///< BOM of UTF-16 little endian.
		const uchar	UTF16BE_BOM[] = {0xFE, 0xFF};		///< BOM of UTF-16 big endian.
#ifndef ASCENSION_NO_EXTENDED_ENCODINGS
		const uchar	UTF32LE_BOM[] = {0xFF, 0xFF, 0x00, 0x00};	///< BOM of UTF-16 little endian.
		const uchar	UTF32BE_BOM[] = {0xFE, 0xFF, 0x00, 0x00};	///< BOM of UTF-16 big endian.
#endif /* !ASCENSION_NO_EXTENDED_ENCODINGS */

		/// A replacement character used when unconvertable.
		const uchar NATIVE_REPLACEMENT_CHARACTER = '?';
		/// A code value for an unmappable native character.
		const uchar UNMAPPABLE_NATIVE_CHARACTER = 0x00;

		template<typename T> inline uchar mask7Bit(T c) {return static_cast<uchar>(c) & 0x7FU;}
		template<typename T> inline uchar mask8Bit(T c) {return static_cast<uchar>(c) & 0xFFU;}
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
#ifdef _WIN32
		uint	convertMIBtoWinCP(MIBenum mib) throw();
		MIBenum	convertWinCPtoMIB(uint codePage) throw();
#endif /* _WIN32 */
		String	getEncodingDisplayName(MIBenum mib);

		class Encoder {
			MANAH_NONCOPYABLE_TAG(Encoder);
		public:
			enum Result {
				/// The conversion fully succeeded. @a fromNext parameter of the conversion method
				/// should equal @a fromEnd.
				COMPLETED,
				/// The conversion partially succeeded because the destination buffer was not large enough.
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
			/**
			 * Returns the aliases of the encoding. Default implementation returns an empty.
			 * @return a string contains aliases separated by NUL
			 */
			virtual std::string getAliases() const throw() {return "";}
			/// Returns the number of bytes represents a UCS character.
			virtual std::size_t getMaximumNativeBytes() const throw() = 0;
			/// Returns the number of UCS characters represents a native character. Default
			/// implementation returns 1.
			virtual std::size_t getMaximumUCSLength() const throw() {return 1;}
			/// Returns the MIBenum value of the encoding.
			virtual MIBenum getMIBenum() const throw() = 0;
			/**
			 * Returns the name of the encoding. If the encoding is registered as a character set
			 * in <a href="http://www.iana.org/assignments/character-sets">IANA character-sets
			 * encoding file</a>, should return the preferred mime name.
			 */
			virtual std::string getName() const throw() = 0;
			// operations
			bool		canEncode(CodePoint c) const;
			bool		canEncode(const Char* first, const Char* last) const;
			bool		canEncode(const String& s) const;
			Result		fromUnicode(uchar* to, uchar* toEnd, uchar*& toNext,
							const Char* from, const Char* fromEnd, const Char*& fromNext, Policy policy = NO_POLICY) const;
			std::string	fromUnicode(const String& from, Policy policy = NO_POLICY) const;
			Result		toUnicode(Char* to, Char* toEnd, Char*& toNext,
							const uchar* from, const uchar* fromEnd, const uchar*& fromNext, Policy policy = NO_POLICY) const;
			String		toUnicode(const std::string& from, Policy policy = NO_POLICY) const;
			// factory
			static Encoder*	forCCSID(int ccsid) throw();
			static Encoder*	forCPGID(int cpgid) throw();
			static Encoder*	forMIB(MIBenum mib) throw();
			static Encoder*	forName(const std::string& name) throw();
#ifdef _WIN32
			static Encoder*	forWindowsCodePage(::UINT codePage) throw();
#endif /* _WIN32 */
			template<typename OutputIterator>
			static void		getAvailableMIBs(OutputIterator out);
			template<typename OutputIterator>
			static void		getAvailableNames(OutputIterator out);
			static MIBenum	getDefault() throw();
			static void		registerEncoder(std::auto_ptr<Encoder> newEncoder);
			static bool		supports(MIBenum mib) throw();
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
			 * @param[in] policy the conversion policy
			 * @return the result of the conversion
			 */
			virtual Result doFromUnicode(uchar* to, uchar* toEnd, uchar*& toNext,
				const Char* from, const Char* fromEnd, const Char*& fromNext, Policy policy) const = 0;
			/**
			 * Converts the given string from the native encoding into UTF-16.
			 * @param[out] to the beginning of the destination buffer
			 * @param[out] toEnd the end of the destination buffer
			 * @param[out] toNext points the first unaltered character in the destination buffer after the conversion
			 * @param[in] from the beginning of the buffer to be converted
			 * @param[in] fromEnd the end of the buffer to be converted
			 * @param[in] fromNext points to the first unconverted character after the conversion
			 * @param[in] policy the conversion policy
			 * @return the result of the conversion
			 */
			virtual Result doToUnicode(Char* to, Char* toEnd, Char*& toNext,
				const uchar* from, const uchar* fromEnd, const uchar*& fromNext, Policy policy) const = 0;
		private:
			typedef std::map<MIBenum, ASCENSION_SHARED_POINTER<Encoder> > Encoders;
			static Encoders& registry() throw();
		};

		class EncodingDetector {
			MANAH_NONCOPYABLE_TAG(EncodingDetector);
		public:
			/// Minimum value of identifier of encoding detector.
			/// Ascension never define an encoding detector has ID less than this.
			static const MIBenum MINIMUM_ID = 4000;
			/// Maximum value of identifier of encoding detector.
			/// Ascension never define an encoding detector has ID greater than this.
			static const MIBenum MAXIMUM_ID = 4999;
			/// Identifier of the Unicode detector.
			static const MIBenum UNICODE_DETECTOR = MINIMUM_ID;
			/// Identifier of the universal detector.
			static const MIBenum UNIVERSAL_DETECTOR = UNICODE_DETECTOR + 1;
			/// Identifier of the detector for the system locale.
			static const MIBenum SYSTEM_LOCALE_DETECTOR = UNICODE_DETECTOR + 2;
			/// Identifier of the detector for the user locale.
			static const MIBenum USER_LOCALE_DETECTOR = UNICODE_DETECTOR + 3;
#ifndef ASCENSION_NO_STANDARD_ENCODINGS
			/// Identifier of the detector for JIS X 0208 encodings.
			static const MIBenum JIS_DETECTOR = UNICODE_DETECTOR + 4;
			/// Identifier of the detector for KS C 5601 encodings.
			static const MIBenum KS_DETECTOR = UNICODE_DETECTOR + 5;
#endif /* !ASCENSION_NO_STANDARD_ENCODINGS */
#ifndef ASCENSION_NO_EXTENDED_ENCODINGS
			/// Identifier of the detector for ARMSCII encodings.
			static const MIBenum ARMSCII_DETECTOR = UNICODE_DETECTOR + 6;
#endif /* !ASCENSION_NO_EXTENDED_ENCODINGS */
		public:
			// constructors
			virtual ~EncodingDetector() throw();
			// attributes
			/// Returns the identifier of the encoding detector.
			MIBenum getID() const throw() {return id_;}
			/// Returns the name of the encoding detector.
			std::string getName() const throw() {return name_;}
			// detection
			MIBenum	detect(const uchar* first, const uchar* last, std::ptrdiff_t* convertibleBytes) const;
			// factory
			static EncodingDetector*	forID(MIBenum id) throw();
			static EncodingDetector*	forName(const std::string& name) throw();
#ifdef _WIN32
			static EncodingDetector*	forWindowsCodePage(::UINT codePage) throw();
#endif /* _WIN32 */
			template<typename OutputIterator>
			static void		getAvailableIDs(OutputIterator out);
			template<typename OutputIterator>
			static void		getAvailableNames(OutputIterator out);
			static void		registerDetector(std::auto_ptr<EncodingDetector> newDetector);
			static bool		supports(MIBenum detectorID) throw();
		protected:
			EncodingDetector(MIBenum id, const std::string& name);
			/**
			 * Detects the encoding of the given character sequence.
			 * @param first the beginning of the sequence
			 * @param last the end of the sequence
			 * @param[out] convertibleBytes the number of bytes (from @a first) absolutely
			 * detected. the value can't exceed the result of (@a last - @a first). may be @c null
			 * @return the MIBenum value of the detected encoding
			 */
			virtual MIBenum doDetect(const uchar* first, const uchar* last, std::ptrdiff_t* convertibleBytes) const throw() = 0;
		private:
			typedef std::map<MIBenum, ASCENSION_SHARED_POINTER<EncodingDetector> > EncodingDetectors;
			static EncodingDetectors& registry() throw();
			const MIBenum id_;
			const std::string name_;
		};


		/// Supports implementation of encoder classes.
		namespace implementation {
			///
			template<typename Concrete> class SBCSEncoder : public Encoder {
			private:
				Result doFromUnicode(uchar* to, uchar* toEnd, uchar*& toNext,
						const Char* from, const Char* fromEnd, const Char*& fromNext, Policy policy) const {
					for(; to < toEnd && from < fromEnd; ++to, ++from) {
						if(!static_cast<const Concrete*>(this)->doFromUnicode(*to, *from)) {
							if(policy == REPLACE_UNMAPPABLE_CHARACTER) *to = NATIVE_REPLACEMENT_CHARACTER;
							else if(policy == IGNORE_UNMAPPABLE_CHARACTER) --to;
							else {toNext = to; fromNext = from; return UNMAPPABLE_CHARACTER;}}}
					toNext = to; fromNext = from; return (from == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
				}
				Result doToUnicode(Char* to, Char* toEnd, Char*& toNext,
						const uchar* from, const uchar* fromEnd, const uchar*& fromNext, Policy policy) const {
					for(; to < toEnd && from < fromEnd; ++to, ++from) {
						if(!static_cast<const Concrete*>(this)->doToUnicode(*to, *from)) {
							if(policy == REPLACE_UNMAPPABLE_CHARACTER) *to = REPLACEMENT_CHARACTER;
							else if(policy == IGNORE_UNMAPPABLE_CHARACTER) --to;
							else {toNext = to; fromNext = from; return UNMAPPABLE_CHARACTER;}}}
					toNext = to; fromNext = from; return (from == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
				}
				std::size_t getMaximumNativeBytes() const throw() {return 1;}
			};
		}


		// implementation macros

		/// Begins the definition of a class has the name @a className extends @c Encoder.
#define ASCENSION_BEGIN_ENCODER_CLASS(className, mib, name)											\
	class className : public ascension::encoding::Encoder {											\
	private:																						\
		Result doFromUnicode(uchar* to, uchar* toEnd, uchar*& toNext,								\
			const Char* from, const Char* fromEnd, const Char*& fromNext, Policy policy) const;		\
		Result doToUnicode(Char* to, Char* toEnd, Char*& toNext,									\
			const uchar* from, const uchar* fromEnd, const uchar*& fromNext, Policy policy) const;	\
		MIBenum getMIBenum() const throw() {return mib;}											\
		std::string getName() const throw() {return name;}
		/// Enters the definition of @c Encoder#getAliases method.
#define ASCENSION_ENCODER_ALIASES(aliases)	\
		std::string getAliases() const throw() {static const char s[] = aliases; return std::string(s, countof(s) - 1);}
		/// Enters the definition of @c Encoder#getMaximumNativeBytes method.
#define ASCENSION_ENCODER_MAXIMUM_NATIVE_BYTES(bytes)	\
		std::size_t getMaximumNativeBytes() const throw() {return bytes;}
		/// Enters the definition of @c Encoder#getMaximumUCSLength method.
#define ASCENSION_ENCODER_MAXIMUM_UCS_LENGTH(length)	\
		std::size_t getMaximumUCSLength() const throw() {return length;}
		/// Ends the class definition opened by @c ASCENSION_BEGIN_ENCODER_CLASS or
		/// @c ASCENSION_BEGIN_SBCS_ENCODER_CLASS.
#define ASCENSION_END_ENCODER_CLASS()	\
	};
		/// Begins the definition of an encoder class for SBCS encoding.
		/// @see ASCENSION_BEGIN_ENCODER_CLASS
#define ASCENSION_BEGIN_SBCS_ENCODER_CLASS(className, mib, name)							\
	class className : public ascension::encoding::implementation::SBCSEncoder<className> {	\
	public:																				\
		bool doFromUnicode(uchar& to, Char from) const;										\
		bool doToUnicode(Char& to, uchar from) const;										\
		MIBenum getMIBenum() const throw() {return mib;}									\
		std::string getName() const throw() {return name;}
		/// Defines a SBCS encoder class.
#define ASCENSION_DEFINE_SBCS_ENCODER(className, mib, name)		\
	ASCENSION_BEGIN_SBCS_ENCODER_CLASS(className, mib, name)	\
	ASCENSION_END_ENCODER_CLASS()

		/// This macro defines a class has the name @a name and the ID @a mib extends @c EncodingDetector.
#define ASCENSION_DEFINE_ENCODING_DETECTOR(className, mib, name)							\
		class className : public EncodingDetector {											\
		public:																				\
			className() : EncodingDetector(mib, name) {}									\
		private:																			\
			MIBenum doDetect(const uchar*, const uchar*, std::ptrdiff_t*) const throw();	\
		}

		/**
		 * Returns MIBs for all available encodings.
		 * @param[out] out the output iterator to receive MIBs
		 */
		template<typename OutputIterator> inline void Encoder::getAvailableMIBs(OutputIterator out) {
			for(Encoders::const_iterator i(registry().begin()), e(registry().end()); i != e; ++i, ++out) *out = i->first;}

		/**
		 * Returns names for all available encodings.
		 * @param[out] out the output iterator to receive names
		 */
		template<typename OutputIterator> inline void Encoder::getAvailableNames(OutputIterator out) {
			for(Encoders::const_iterator i(registry().begin()), e(registry().end()); i != e; ++i, ++out) *out = i->second->getName();}

		/**
		 * Returns identifiers for all available encoding detectors.
		 * @param[out] out the output iterator to receive identifiers
		 */
		template<typename OutputIterator> inline void EncodingDetector::getAvailableIDs(OutputIterator out) {
			for(EncodingDetectors::const_iterator i(registry().begin()), e(registry().end()); i != e; ++i, ++out) *out = i->first;}

		/**
		 * Returns names for all available encoding detectors.
		 * @param[out] out the output iterator to receive names
		 */
		template<typename OutputIterator> inline void EncodingDetector::getAvailableNames(OutputIterator out) {
			for(EncodingDetectors::const_iterator i(registry().begin()), e(registry().end()); i != e; ++i, ++out) *out = i->second->getName();}

	}
} // namespace ascension.encoding

#endif /* !ASCENSION_ENCODER_HPP */
