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
#ifdef _WIN32
#include <windows.h>	// GetCPInfo, MultiByteToWideChar, WideCharToMultiByte, ...
#endif /* _WIN32 */


namespace ascension {

	/// Members of the namespace @c encoding provide conversions between text encodings.
	namespace encoding {

		/**
		 * "The MIBenum value is a unique value for use in MIBs to identify coded character sets"
		 * (http://www.iana.org/assignments/character-sets).
		 *
		 * Ascension defines own several encodings not registered by IANA. They have MIBenum
		 * values whose regions are 3000-3999. These values are subject to change in the future.
		 */
		typedef ushort MIBenum;

		const MIBenum MIB_UNICODE_AUTO_DETECTION = 3000;	///< Auto-detection for Unicode.
#ifndef ASCENSION_NO_EXTENDED_ENCODINGS
		const MIBenum
			MIB_UNIVERSAL_AUTO_DETECTION		= 3001,	///< Auto-detection for all languages (windows-50001).
			MIB_JAPANESE_AUTO_DETECTION			= 3002,	///< Auto-detection for Japanese (windows-50932).
			MIB_KOREAN_AUTO_DETECTION			= 3003,	///< Auto-detection for Korean (windows-50949).
			MIB_AUTO_DETECTION_SYSTEM_LANGUAGE	= 3004,	///< Auto-detection for the system language.
			MIB_AUTO_DETECTION_USER_LANGUAGE	= 3005,	///< Auto-detection for the user language.
			MIB_UNICODE_UTF5					= 3006,	///< Unicode (UTF-5).
			// Armenian
			MIB_ARMENIAN_AUTO_DETECTION	= 3020,	///< Auto-detection for Armenian.
			MIB_ARMENIAN_ARMSCII7		= 3021,	///< Armenian (ARMSCII-7).
			MIB_ARMENIAN_ARMSCII8		= 3022,	///< Armenian (ARMSCII-8).
			MIB_ARMENIAN_ARMSCII8A		= 3023,	///< Armenian (ARMSCII-8A).
			// Vietnamese
			MIB_VIETNAMESE_AUTO_DETECTION	= 3030,	///< Auto-detection for Vietnamese.
			MIB_VIETNAMESE_TCVN				= 3031,	///< Vietnamese (TCVN).
//			MIB_VIETNAMESE_VISCII			= 3032,	///< Vietnamese (VISCII).
			MIB_VIETNAMESE_VPS				= 3033,	///< Vietnamese (VPS).
			// Japanese
			MIB_JAPANESE_ISO2022JP		= 3040,	///< Japanese (ISO-2022-JP)
			MIB_JAPANESE_SHIFTJIS		= 3041,	///< Japanese (Shift JIS)
			MIB_JAPANESE_ISO2022JP1		= 3042,	///< Japanese (ISO-2022-JP-1)
			MIB_JAPANESE_ISO2022JP2		= 3043,	///< Japanese (ISO-2022-JP-2)
			MIB_JAPANESE_EUC			= 3044,	///< Japanese (EUC)
			MIB_JAPANESE_ISO2022JP2004				= 3045,	///< Japanese (ISO-2022-JP-2004)
			MIB_JAPANESE_ISO2022JP2004_STRICT		= 3046,	///< Japanese (ISO-2022-JP-2004-strict)
			MIB_JAPANESE_ISO2022JP2004_COMPATIBLE	= 3047,	///< Japanese (ISO-2022-JP-2004-compatible)
			MIB_JAPANESE_ISO2022JP3				= 3048,	///< Japanese (ISO-2022-JP-3)
			MIB_JAPANESE_ISO2022JP3_STRICT		= 3049,	///< Japanese (ISO-2022-JP-3-strict)
			MIB_JAPANESE_ISO2022JP3_COMPATIBLE	= 3050,	///< Japanese (ISO-2022-JP-3-compatible)
			MIB_JAPANESE_SHIFTJIS2004	= 3051,	///< Japanese (Shift_JIS-2004)
			MIB_JAPANESE_EUCJIS2004		= 3052,	///< Japanese (EUC-JIS-2004)
			// Lao
			MIB_LAO_MULE_LAO	= 3060,	///< Lao (MuleLao)
			MIB_LAO_CP1132		= 3061,	///< Lao (ibm-1132)
			MIB_LAO_CP1133		= 3062,	///< Lao (ibm-1133)
			// Irelandic
			MIB_IRISH_IS434	= 3070,	///< Irelandic (I.S. 434:1999).
			// Tamil
			MIB_TAMIL_TAB	= 3080,	///< Tamil (TAB).
			MIB_TAMIL_TAM	= 3081,	///< Tamil (TAM).
//			MIB_TAMIL_TSCII	= 3082,	///< Tamil (TSCII 1.7).
			// Hindi
			MIB_HINDI_MACINTOSH	= 3090,	///< Hindi (Macintosh, Devanagari).
			// Gujarati
			MIB_GUJARATI_MACINTOSH	= 3100,	///< Gujarati (Macintosh).
			// Panjabi
			MIB_PANJABI_MACINTOSH	= 3110,	///< Punjabi (Macintosh, Gurumkhi).
			// Cyrillic
/*			CPEX_CYRILLIC_MACINTOSH							= 10007,	///< Cyrillic (Macintosh)
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
*/			// ISO-2022 multilingual
			MIB_MULTILINGUAL_ISO2022_7BIT		= 3120,	///< Multilingual (ISO-2022, 7-bit).
			MIB_MULTILINGUAL_ISO2022_7BITSS2	= 3121,	///< Multilingual (ISO-2022, 7-bit, SS2).
			MIB_MULTILINGUAL_ISO2022_7BITSISO	= 3122,	///< Multilingual (ISO-2022, 7-bit, SI/SO).
			MIB_MULTILINGUAL_ISO2022_8BITSS2	= 3123,	///< Multilingual (ISO-2022, 8-bit, SS2).
			// miscellaneous
			MIB_MISCELLANEOUS_BINARY	= 3900,	///< Binary.
			MIB_MISCELLANEOUS_NEXTSTEP	= 3901,	///< NEXTSTEP.
			MIB_MISCELLANEOUS_ATARIST	= 3902;	///< Atari ST/TT.
#endif /* !ASCENSION_NO_EXTENDED_ENCODINGS */

		const uchar	UTF8_BOM[] = "\xEF\xBB\xBF";		///< BOM of UTF-8.
		const uchar	UTF16LE_BOM[] = "\xFF\xFE";			///< BOM of UTF-16 little endian.
		const uchar	UTF16BE_BOM[] = "\xFE\xFF";			///< BOM of UTF-16 big endian.
#ifndef ASCENSION_NO_EXTENDED_ENCODINGS
		const uchar	UTF32LE_BOM[] = "\xFF\xFF\x00\x00";	///< BOM of UTF-16 little endian.
		const uchar	UTF32BE_BOM[] = "\xFE\xFF\x00\x00";	///< BOM of UTF-16 big endian.
#endif /* !ASCENSION_NO_EXTENDED_ENCODINGS */


		// Encoder //////////////////////////////////////////////////////////////

		/// A replacement character used when unconvertable.
		const uchar NATIVE_DEFAULT_CHARACTER = '?';

		/// A shorthand for @c REPLACEMENT_CHARACTER.
		const Char RP__CH = REPLACEMENT_CHARACTER;
		const uchar N__A = 0x00;

		template<typename Ch> void setDefaultChar(Ch& ch);
		template<> inline void setDefaultChar(char& ch) {ch = NATIVE_DEFAULT_CHARACTER;}
		template<> inline void setDefaultChar(uchar& ch) {ch = NATIVE_DEFAULT_CHARACTER;}
		template<> inline void setDefaultChar(ushort& ch) {ch = NATIVE_DEFAULT_CHARACTER;}
		template<> inline void setDefaultChar(wchar_t& ch) {ch = REPLACEMENT_CHARACTER;}
		template<> inline void setDefaultChar(ulong& ch) {ch = REPLACEMENT_CHARACTER;}

		template<typename T> inline uchar mask7Bit(T c) {return static_cast<uchar>(c) & 0x7FU;}
		template<typename T> inline uchar mask8Bit(T c) {return static_cast<uchar>(c) & 0xFFU;}
		template<typename T> inline ushort mask16Bit(T c) {return static_cast<ushort>(c) & 0xFFFFU;}
		template<typename T> inline Char maskUCS2(T c) {return static_cast<Char>(c) & 0xFFFFU;}

		#define CONFIRM_ILLEGAL_CHAR(lhs)										\
			{																	\
				if(callback == 0 || callback->unconvertableCharacterFound()) {	\
					setDefaultChar(lhs);										\
					callback = 0;												\
				} else															\
					return 0;													\
			}

		#define MAP_TABLE(offset, table)	\
			else MAP_TABLE_START(offset, table)

		#define MAP_TABLE_START(offset, table)	\
			if(src[i] >= offset && src[i] < offset + countof(table)) dest[j] = table[src[i] - offset]

		#define MAP_TABLE_0(table)	\
			if(src[i] < countof(table)) dest[j] = table[src[i]]

		#define MAP_TABLE_SB(offset, table)	\
			else MAP_TABLE_SB_START(offset, table)

		#define MAP_TABLE_SB_START(offset, table)	\
			if(src[i] >= offset && src[i] < offset + countof(table)) dest[i] = table[src[i] - offset]

		#define MAP_TABLE_SB_0(table)	\
			if(src[i] < countof(table)) dest[i] = table[src[i]]

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

		/**
		 * An abstract encoder.
		 * @note This class will be rewritten in future.
		 */
		class Encoder : private manah::Noncopyable {
		public:
			enum Result {
				/// The conversion fully succeeded. @a fromNext parameter of the conversion method
				/// should equal @a fromEnd.
				COMPLETED,
				/// The conversion partially succeeded because the destination buffer is not large enough.
				INSUFFICIENT_BUFFER,
				/// The conversion partially succeeded because encounted an illegal character. If
				/// @c USE_DEFAULT_CHARACTER is set, this value never be returned.
				ILLEGAL_CHARACTER
			};
			enum Policy {
				NO_POLICY						= 0x0,
//				NO_UNICODE_BYTE_ORDER_SIGNATURE	= 0x1,
				USE_DEFAULT_CHARACTER			= 0x2	///< Uses 
			};
			typedef std::auto_ptr<Encoder>(*EncoderProducer)();
			typedef std::size_t(*EncodingDetector)(const uchar*, const uchar*, MIBenum&);
		public:
			virtual ~Encoder() throw();
			// attributes
			/**
			 * Returns the aliases of the encoding. Default implementation returns an empty.
			 * @return a string contains aliases separated by NUL
			 */
			virtual std::string getAliases() const throw() {return "";}
			/// Returns the number of bytes represents a UCS character.
			virtual std::size_t getMaximumNativeLength() const throw() = 0;
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
							const Char* from, const Char* fromEnd, const Char*& fromNext,
							const manah::Flags<Policy>& policy = NO_POLICY) const;
			std::string	fromUnicode(const String& from, const manah::Flags<Policy>& policy = NO_POLICY) const;
			Result		toUnicode(Char* to, Char* toEnd, Char*& toNext,
							const uchar* from, const uchar* fromEnd, const uchar*& fromNext,
							const manah::Flags<Policy>& policy = NO_POLICY) const;
			String		toUnicode(const std::string& from, const manah::Flags<Policy>& policy = NO_POLICY) const;
			// factory
			template<typename OutputIterator>
			static void		getAvailableMIBs(OutputIterator out);
			template<typename OutputIterator>
			static void		getAvailableNames(OutputIterator out);
			static Encoder*	forMIB(MIBenum mib) throw();
			static Encoder*	forName(const std::string& name) throw();
			static void		registerEncoder(std::auto_ptr<Encoder> encoder);
			// auto detection
			static MIBenum	detectEncoding(const uchar* first, const uchar* last, MIBenum mib);
			static void		registerDetector(MIBenum mib, EncodingDetector detector);
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
				const Char* from, const Char* fromEnd, const Char*& fromNext, const manah::Flags<Policy>& policy) const = 0;
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
				const uchar* from, const uchar* fromEnd, const uchar*& fromNext, const manah::Flags<Policy>& policy) const = 0;
		private:
			typedef std::map<MIBenum, Encoder*> Encoders;
			typedef std::map<MIBenum, EncodingDetector> EncodingDetectors;
			static Encoders encoders_;
			static EncodingDetectors encodingDetectors_;
		};

#define ASCENSION_INTERNAL_DEFINE_SIMPLE_ENCODER_PROLOGUE(className)													\
	class className : public Encoder {																					\
	public:																												\
		className() throw() {}																							\
	private:																											\
		Result doFromUnicode(uchar* to, uchar* toEnd, uchar*& toNext,													\
			const Char* from, const Char* fromEnd, const Char*& fromNext, const manah::Flags<Policy>& policy) const;	\
		Result doToUnicode(Char* to, Char* toEnd, Char*& toNext,														\
			const uchar* from, const uchar* fromEnd, const uchar*& fromNext, const manah::Flags<Policy>& policy) const;	\
		std::size_t getMaximumNativeLength() const throw();																\
		MIBenum getMIBenum() const throw();																				\
		std::string getName() const throw();
#define ASCENSION_INTERNAL_DEFINE_SIMPLE_ENCODER_EPILOGUE()																\
	}

		/// This macro defines a class has the name @a className extends @c Encoder.
#define ASCENSION_DEFINE_SIMPLE_ENCODER(className)					\
	ASCENSION_INTERNAL_DEFINE_SIMPLE_ENCODER_PROLOGUE(className)	\
	ASCENSION_INTERNAL_DEFINE_SIMPLE_ENCODER_EPILOGUE()

		/// This macro defines a class has the name @a className extends @c Encoder with aliases.
#define ASCENSION_DEFINE_SIMPLE_ENCODER_WITH_ALIASES(className)		\
	ASCENSION_INTERNAL_DEFINE_SIMPLE_ENCODER_PROLOGUE(className)	\
	std::string getAliases() const throw();							\
	ASCENSION_INTERNAL_DEFINE_SIMPLE_ENCODER_EPILOGUE()

#ifdef _WIN32
		/// Encoder uses Windows NLS.
		class WindowsEncoder : public Encoder {
		public:
			WindowsEncoder(::UINT codePage);
		private:
			Result		doFromUnicode(uchar* to, uchar* toEnd, uchar*& toNext,
							const Char* from, const Char* fromEnd, const Char*& fromNext, const manah::Flags<Policy>& policy) const;
			Result		doToUnicode(Char* to, Char* toEnd, Char*& toNext,
							const uchar* from, const uchar* fromEnd, const uchar*& fromNext, const manah::Flags<Policy>& policy) const;
			std::string	getAliases() const throw();
			std::size_t	getMaximumNativeLength() const throw();
			MIBenum		getMIBenum() const throw();
			std::string	getName() const throw();
		private:
			const ::UINT codePage_;
		};
#endif /* _WIN32 */


		/**
		 * Returns MIBs for all available encodings.
		 * @param[out] out the output iterator to receive MIBs
		 */
		template<typename OutputIterator> inline void Encoder::getAvailableMIBs(OutputIterator out) {
			for(EncoderProducers::const_iterator i(encoderProducers_.begin()), e(encoderProducers_.end()); i != e; ++i, ++out)
				*out = i->first;
			for(EncodingDetectors::const_iterator i(encodingDetectors_.begin()), e(encodingDetectors_.end()); i != e; ++i, ++out)
				*out = i->first;
		}

		/**
		 * Returns names for all available encodings.
		 * @param[out] out the output iterator to receive names
		 */
		template<typename OutputIterator> inline void Encoder::getAvailableNames(OutputIterator out) {
			for(EncoderProducers::const_iterator i(encoderProducers_.begin()), e(encoderProducers_.end()); i != e; ++i, ++out)
				*out = i->second.getName();
			for(EncodingDetectors::const_iterator i(encodingDetectors_.begin()), e(encodingDetectors_.end()); i != e; ++i, ++out)
				*out = i->second.getName();
		}

	}
} // namespace ascension.encoding

#endif /* !ASCENSION_ENCODER_HPP */
