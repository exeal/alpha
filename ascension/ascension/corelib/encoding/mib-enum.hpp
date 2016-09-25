/**
 * @file mib-enum.hpp
 * Defines @c MIBenum type and individual values, and related free functions.
 * @author exeal
 * @date 2004-2014 Was encoder.hpp.
 * @date 2016-09-22 Separated from encoder.hpp.
 */

#ifndef ASCENSION_MIB_ENUM_HPP
#define ASCENSION_MIB_ENUM_HPP
#include <ascension/config.hpp>	// ASCENSION_NO_*_ENCODINGS
#include <ascension/corelib/string-piece.hpp>
#include <cstdint>
#include <locale>	// std.locale, std.codecvt

namespace ascension {
	/// Members of the namespace @c encoding provide conversions between text encodings.
	namespace encoding {
		/**
		 * "The MIBenum value is a unique value for use in MIBs to identify coded character sets"
		 * (http://www.iana.org/assignments/character-sets).
		 * @see http://www.iana.org/assignments/ianacharset-mib/ianacharset-mib
		 */
		typedef std::uint16_t MIBenum;

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
				UHC = 36,			///< UHC (windows-949).
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
#endif // !ASCENSION_NO_STANDARD_ENCODINGS

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
#endif // !ASCENSION_NO_PROPRIETARY_ENCODINGS

#ifndef ASCENSION_NO_EXTENDED_ENCODINGS
		/**
		 * MIBenum values of the extended encodings.
		 * @deprecated 0.7
		 */
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
#endif // !ASCENSION_NO_EXTENDED_ENCODINGS

		/// Thrown if the specified encoding is not supported.
		class UnsupportedEncodingException : public std::invalid_argument {
		public:	
			/**
			 * Constructor.
			 * @param message The message string
			 */
			explicit UnsupportedEncodingException(const std::string& message) : std::invalid_argument(message) {}
		};

		/// @defgroup mibenum_value_conversions MIBenum Value Conversions
		/// Free functions convert between MIBenum value and other enumerated one.
		/// @{
		MIBenum convertCCSIDtoMIB(unsigned int ccsid) BOOST_NOEXCEPT;
		unsigned int convertMIBtoCCSID(MIBenum mib) BOOST_NOEXCEPT;
#if BOOST_OS_WINDOWS
		unsigned int convertMIBtoWinCP(MIBenum mib) BOOST_NOEXCEPT;
		MIBenum convertWinCPtoMIB(unsigned int codePage) BOOST_NOEXCEPT;
#endif // BOOST_OS_WINDOWS
		/// @}

		/// @defgroup encoding_names Encoding Names
		/// @{
		/**
		 * Compares the given two encoding (charset) names based on <a href="http://www.unicode.org/reports/tr22/">UTS
		 * #22: CharMapML</a> 1.4 Charset Alias Matching.
		 * @tparam CharacterSequence1 The type of @a first1 and @a last1
		 * @tparam CharacterSequence2 The type of @a first2 and @a last2
		 * @param first1 The beginning of the first character sequence
		 * @param last1 The end of the first character sequence
		 * @param first2 The beginning of the second character sequence
		 * @param last2 The end of the second character sequence
		 * @retval -1 The first encoding name is less than the second
		 * @retval 0 The two encoding names matched
		 * @retval 1 The first encoding name is greater than the second
		 */
		template<typename CharacterSequence1, typename CharacterSequence2>
		inline int compareEncodingNames(
				CharacterSequence1 first1, CharacterSequence1 last1,
				CharacterSequence2 first2, CharacterSequence2 last2) {
			const std::locale& lc = std::locale::classic();
			bool precededByDigit[2] = {false, false};
			while(first1 != last1 && first2 != last2) {
				if(*first1 == '0' && !precededByDigit[0])
					++first1;
				else if(!std::isalnum(*first1, lc)) {
					++first1;
					precededByDigit[0] = false;
				} else if(*first2 == '0' && !precededByDigit[1])
					++first2;
				else if(!std::isalnum(*first2, lc)) {
					++first2;
					precededByDigit[1] = false;
				} else {
					if(const int d = std::tolower(*first1, lc) - std::tolower(*first2, lc))
						return d;
					precededByDigit[0] = std::isdigit(*(first1++), lc);
					precededByDigit[1] = std::isdigit(*(first2++), lc);
				}
			}
			if(first1 != last1)
				return 1;
			else
				return (first2 == last2) ? 0 : -1;
		}
		/**
		 * @overload
		 * @tparam SinglePassReadableRange1 The type of @a name1
		 * @tparam SinglePassReadableRange2 The type of @a name2
		 * @param name1 (See the description of iterator version)
		 * @param name2 (See the description of iterator version)
		 * @return (See the description of iterator version)
		 */
		template<typename SinglePassReadableRange1, typename SinglePassReadableRange2>
		inline int compareEncodingNames(const SinglePassReadableRange1& name1, const SinglePassReadableRange2& name2) {
			return compareEncodingNames(boost::const_begin(name1), boost::const_end(name1), boost::const_begin(name2), boost::const_end(name2));
		}
		std::string encodingNameFromUnicode(const StringPiece& source);
		String getEncodingDisplayName(MIBenum mib);
		/// @}
	}
} // namespace ascension.encoding

#endif // !ASCENSION_MIB_ENUM_HPP
