/**
 * @file encoder.hpp
 * @author exeal
 * @date 2004-2014
 */

#ifndef ASCENSION_ENCODER_HPP
#define ASCENSION_ENCODER_HPP

#include <ascension/config.hpp>	// ASCENSION_NO_*_ENCODINGS
#include <ascension/corelib/basic-types.hpp>
#include <ascension/corelib/string-piece.hpp>
#include <array>
#include <memory>	// std.unique_ptr
#include <locale>	// std.locale, std.codecvt
#include <vector>

namespace ascension {

	/// Members of the namespace @c encoding provide conversions between text encodings.
	namespace encoding {

		/**
		 * "The MIBenum value is a unique value for use in MIBs to identify coded character sets"
		 * (http://www.iana.org/assignments/character-sets).
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

		template<typename T> inline Byte mask7Bit(T c) {return static_cast<Byte>(c & 0x7fu);}
		template<typename T> inline std::uint8_t mask8Bit(T c) {return static_cast<std::uint8_t>(c & 0xffu);}
		template<typename T> inline std::uint16_t mask16Bit(T c) {return static_cast<std::uint16_t>(c & 0xffffu);}
		template<typename T> inline Char maskUCS2(T c) {return static_cast<Char>(c & 0xffffu);}

		/**
		 * Compares the given two encoding (charset) names based on
		 * <a href="http://www.unicode.org/reports/tr22/">UTS #22: CharMapML</a> 1.4 Charset Alias
		 * Matching.
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

		MIBenum convertCCSIDtoMIB(unsigned int ccsid) BOOST_NOEXCEPT;
		unsigned int convertMIBtoCCSID(MIBenum mib) BOOST_NOEXCEPT;
#ifdef BOOST_OS_WINDOWS
		unsigned int convertMIBtoWinCP(MIBenum mib) BOOST_NOEXCEPT;
		MIBenum convertWinCPtoMIB(unsigned int codePage) BOOST_NOEXCEPT;
#endif // BOOST_OS_WINDOWS
		String getEncodingDisplayName(MIBenum mib);
		std::string encodingNameFromUnicode(const String& source);

		/// Thrown if the specified encoding is not supported.
		class UnsupportedEncodingException : public std::invalid_argument {
		public:
			explicit UnsupportedEncodingException(const std::string& message);
		};

		/**
		 * An interface which describes the properties of a encoding. @c Encoder#properties method
		 * returns this object.
		 */
		class EncodingProperties {
		public:
			/**
			 * Returns the aliases of the encoding. Default implementation returns an empty.
			 * @return a string contains aliases separated by vertical bar ('|')
			 */
			virtual std::string aliases() const BOOST_NOEXCEPT {return "";}
			/**
			 * Returns the human-readable name of the encoding. Default implementation calls
			 * @c #name method.
			 * @param locale The locale used to localize the name
			 * @see #name
			 */
			virtual std::string displayName(const std::locale& locale) const BOOST_NOEXCEPT {return name();}
			/// Returns the number of bytes represents a UCS character.
			virtual std::size_t maximumNativeBytes() const BOOST_NOEXCEPT = 0;
			/// Returns the number of UCS characters represents a native character. Default
			/// implementation returns 1.
			virtual std::size_t maximumUCSLength() const BOOST_NOEXCEPT {return 1;}
			/// Returns the MIBenum value of the encoding.
			virtual MIBenum mibEnum() const BOOST_NOEXCEPT = 0;
			/**
			 * Returns the name of the encoding. If the encoding is registered as a character set
			 * in <a href="http://www.iana.org/assignments/character-sets">IANA character-sets
			 * encoding file</a>, should return the preferred mime name.
			 * @see #displayName
			 */
			virtual std::string name() const BOOST_NOEXCEPT = 0;
			/**
			 * Returns an native character which indicates that the given Unicode character can't
			 * map. If @c #policy returns @c REPLACE_UNMAPPABLE_CHARACTER, the encoder should use
			 * this character. Default implementation returns 0x1A.
			 */
			virtual Byte substitutionCharacter() const BOOST_NOEXCEPT {return 0x1a;}
		};

		class EncoderFactory;

		class Encoder : private boost::noncopyable {
		public:
			/// Result of conversion.
			enum Result {
				/**
				 * The conversion fully succeeded. If @a fromNext parameter of the conversion
				 * method is less @a fromEnd, more input is required.
				 */
				COMPLETED,
				/**
				 * The conversion partially succeeded because the destination buffer was not large
				 * enough.
				 */
				INSUFFICIENT_BUFFER,
				/**
				 * The conversion partially succeeded because encounted an unmappable character.
				 * @c fromNext parameter of the conversion method should addresses the unmappable
				 * character. If either @c REPLACE_UNMAPPABLE_CHARACTER or
				 * @c IGNORE_UNMAPPABLE_CHARACTER is set, this value never be returned.
				 * @see #SubstitutionPolicy
				 */
				UNMAPPABLE_CHARACTER,
				/**
				 * The conversion partially succeeded because detected malformed input.
				 * @c fromNext parameter of the conversion method should addresses the unmappable
				 * character. @c Encoder#fromUnicode should not return this value.
				 */
				MALFORMED_INPUT
			};

			/**
			 * Specifies how to handle unmappable bytes/characters.
			 * @see #substitutionPolicy, #setSubstitutionPolicy
			 */
			enum SubstitutionPolicy {
				/// Aborts with @c UNMAPPABLE_CHARACTER return value.
				DONT_SUBSTITUTE,
				/// Replaces unmappable bytes/characters with replacement characters/bytes.
				REPLACE_UNMAPPABLE_CHARACTERS,
				/// Skips (ignores) unmappable bytes/characters.
				IGNORE_UNMAPPABLE_CHARACTERS
			};

			/// Miscellaneous conversion flag.
			enum Flag {
				/**
				 * Indicates that @a from parameter of the conversion method addresses the
				 * beginning of the entire input sequence and @a to parameter addresses the
				 * beginning of the entire output sequence.
				 */
				BEGINNING_OF_BUFFER = 0x01,
				/**
				 * Indicates that @a fromEnd parameter of the conversion method addresses the end
				 * of the entire input sequence.
				 */
				END_OF_BUFFER = 0x02,
				/**
				 * Indicates that incoming or outgoing buffer contains a Unicode byte order mark
				 * (BOM). If you set this flag without @c FROM_IS_NOT_BOB when encoding, the
				 * encoder writes BOM into the beginning of the output byte sequence. And the
				 * decoder sets this flag if the input byte sequence contained BOM and the other
				 * flag @c FROM_IS_NOT_BOB was not set.
				 */
				UNICODE_BYTE_ORDER_MARK = 0x04
			};

			static const char ALIASES_SEPARATOR;

		public:
			virtual ~Encoder() BOOST_NOEXCEPT;

			/// @name Attributes
			/// @{
			int flags() const BOOST_NOEXCEPT;
			virtual const EncodingProperties& properties() const BOOST_NOEXCEPT = 0;
			virtual Encoder& resetDecodingState() BOOST_NOEXCEPT;
			virtual Encoder& resetEncodingState() BOOST_NOEXCEPT;
			Encoder& setFlags(const int newFlags);
			Encoder& setSubstitutionPolicy(SubstitutionPolicy newPolicy);
			SubstitutionPolicy substitutionPolicy() const BOOST_NOEXCEPT;
			/// @}

			/// @name Conversion
			/// @{
			bool canEncode(CodePoint c);
			bool canEncode(const StringPiece& s);
			Result fromUnicode(Byte* to, Byte* toEnd, Byte*& toNext, const Char* from, const Char* fromEnd, const Char*& fromNext);
			std::string fromUnicode(const String& from);
			Result toUnicode(Char* to, Char* toEnd, Char*& toNext, const Byte* from, const Byte* fromEnd, const Byte*& fromNext);
			String toUnicode(const std::string& from);
			/// @}

			/// @name Factory
			/// @{
			template<typename OutputIterator> static void availableEncodings(OutputIterator out);
			static Encoder& defaultInstance() BOOST_NOEXCEPT;
			static std::unique_ptr<Encoder> forCCSID(int ccsid) BOOST_NOEXCEPT;
			static std::unique_ptr<Encoder> forCPGID(int cpgid) BOOST_NOEXCEPT;
			static std::unique_ptr<Encoder> forID(std::size_t id) BOOST_NOEXCEPT;
			static std::unique_ptr<Encoder> forMIB(MIBenum mib) BOOST_NOEXCEPT;
			static std::unique_ptr<Encoder> forName(const std::string& name) BOOST_NOEXCEPT;
			static std::unique_ptr<Encoder> forWindowsCodePage(unsigned int codePage) BOOST_NOEXCEPT;
			static bool supports(MIBenum mib) BOOST_NOEXCEPT;
			static bool supports(const std::string& name) BOOST_NOEXCEPT;
			static void registerFactory(std::shared_ptr<const EncoderFactory> newFactory);
			/// @}

		protected:
			Encoder() BOOST_NOEXCEPT;
		private:
			/**
			 * Converts the given string from UTF-16 into the native encoding.
			 * @param[out] to The beginning of the destination buffer
			 * @param[out] toEnd The end of the destination buffer
			 * @param[out] toNext Points the first unaltered character in the destination buffer after the conversion
			 * @param[in] from The beginning of the buffer to be converted
			 * @param[in] fromEnd The end of the buffer to be converted
			 * @param[in] fromNext Points to the first unconverted character after the conversion
			 * @return The result of the conversion
			 */
			virtual Result doFromUnicode(Byte* to, Byte* toEnd, Byte*& toNext,
				const Char* from, const Char* fromEnd, const Char*& fromNext) = 0;
			/**
			 * Converts the given string from the native encoding into UTF-16.
			 * @param[out] to The beginning of the destination buffer
			 * @param[out] toEnd The end of the destination buffer
			 * @param[out] toNext Points the first unaltered character in the destination buffer
			 *             after the conversion
			 * @param[in] from The beginning of the buffer to be converted
			 * @param[in] fromEnd The end of the buffer to be converted
			 * @param[in] fromNext Points to the first unconverted character after the conversion
			 * @return The result of the conversion
			 */
			virtual Result doToUnicode(Char* to, Char* toEnd, Char*& toNext,
				const Byte* from, const Byte* fromEnd, const Byte*& fromNext) = 0;
		private:
			static std::shared_ptr<const EncoderFactory> find(MIBenum mib) BOOST_NOEXCEPT;
			static std::shared_ptr<const EncoderFactory> find(const std::string& name) BOOST_NOEXCEPT;
			static std::vector<std::shared_ptr<const EncoderFactory>>& registry();
			SubstitutionPolicy substitutionPolicy_;
			int flags_;	// see Flag enums
		};

		/// A factory class creates @c Encoder instances.
		class EncoderFactory : public EncodingProperties {
		public:
			/// Destructor.
			virtual ~EncoderFactory() BOOST_NOEXCEPT {}
		protected:
			/// Returns the @c Encoder instance.
			virtual std::unique_ptr<Encoder> create() const BOOST_NOEXCEPT = 0;
			friend class Encoder;
		};

		class EncodingDetector : private boost::noncopyable {
		public:
			// constructors
			virtual ~EncodingDetector() BOOST_NOEXCEPT;
			// attributes
			/// Returns the name of the encoding detector.
			std::string name() const BOOST_NOEXCEPT {return name_;}
			// detection
			std::pair<MIBenum, std::string> detect(const Byte* first, const Byte* last, std::ptrdiff_t* convertibleBytes) const;
			// factory
			static std::shared_ptr<const EncodingDetector> forName(const std::string& name) BOOST_NOEXCEPT;
#ifdef BOOST_OS_WINDOWS
			static std::shared_ptr<const EncodingDetector> forWindowsCodePage(unsigned int codePage) BOOST_NOEXCEPT;
#endif // BOOST_OS_WINDOWS
			template<typename OutputIterator> static void availableNames(OutputIterator out);
			static void registerDetector(std::shared_ptr<const EncodingDetector> newDetector);
		protected:
			explicit EncodingDetector(const std::string& name);
		private:
			/**
			 * Detects the encoding of the given character sequence.
			 * @param first The beginning of the sequence
			 * @param last The end of the sequence
			 * @param[out] convertibleBytes The number of bytes (from @a first) absolutely
			 *             detected. The value can't exceed the result of (@a last - @a first).
			 *             May be @c null
			 * @return The MIBenum value and the name of the detected encoding
			 */
			virtual std::pair<MIBenum, std::string> doDetect(
				const Byte* first, const Byte* last, std::ptrdiff_t* convertibleBytes) const BOOST_NOEXCEPT = 0;
		private:
			static std::vector<std::shared_ptr<const EncodingDetector>>& registry();
			const std::string name_;
		};


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
				EncoderFactoryBase(const std::string& name,
					MIBenum mib, const std::string& displayName = "",
					std::size_t maximumNativeBytes = 1, std::size_t maximumUCSLength = 1,
					const std::string& aliases = "", Byte substitutionCharacter = 0x1a);
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
					SingleByteEncoderFactory(const std::string& name, MIBenum mib,
						const std::string& displayName, const std::string& aliases, Byte substitutionCharacter);
					virtual ~SingleByteEncoderFactory() BOOST_NOEXCEPT;
				private:
					std::unique_ptr<Encoder> create() const BOOST_NOEXCEPT;
				};
			} // namespace sbcs

			namespace dbcs {
				ASCENSION_STATIC_ASSERT(sizeof(std::uint16_t) == 2);

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

		/**
		 * Returns informations for all available encodings.
		 * @tparam OutputIterator The type of @a out
		 * @param[out] out The output iterator to receive pairs consist of the enumeration
		 *             identifier and the encoding information. The expected type of the pair is
		 *             @c std#pair&lt;std::size_t, const EncodingProperties*&gt;. The enumeration
		 *             identifier can be used with @c #forID method.
		 */
		template<typename OutputIterator>
		inline void Encoder::availableEncodings(OutputIterator out) {
			for(std::size_t i = 0, c = registry().size(); i < c; ++i, ++out)
				*out = std::make_pair<std::size_t, const EncodingProperties*>(i, registry()[i]);
		}

		/// Returns the miscellaneous flags.
		inline int Encoder::flags() const BOOST_NOEXCEPT {return flags_;}

		/// Returns the substitution policy.
		inline Encoder::SubstitutionPolicy Encoder::substitutionPolicy() const BOOST_NOEXCEPT {
			return substitutionPolicy_;
		}

		/**
		 * Returns names for all available encoding detectors.
		 * @tparam OutputIterator The type of @a out
		 * @param[out] out The output iterator to receive names
		 */
		template<typename OutputIterator>
		inline void EncodingDetector::availableNames(OutputIterator out) {
			for(std::vector<std::shared_ptr<const EncodingDetector>>::const_iterator i(std::begin(registry())), e(std::end(registry())); i != e; ++i, ++out)
				*out = (*i)->name();
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

		/**
		 * Returns the byte corresponds to the given character @c c or @c UNMAPPABLE_BYTE if
		 * umappable.
		 */
		inline Byte implementation::sbcs::BidirectionalMap::toByte(Char c) const BOOST_NOEXCEPT {
			return unicodeToByte_[c >> 8][mask8Bit(c)];
		}

		/**
		 * Returns the character corresponds to the given byte @a c or @c REPLACEMENT_CHARACTER if
		 * umappable.
		 */
		inline Char implementation::sbcs::BidirectionalMap::toCharacter(Byte c) const BOOST_NOEXCEPT {
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
		inline implementation::sbcs::SingleByteEncoderFactory<MappingTable>::SingleByteEncoderFactory(const std::string& name, MIBenum mib,
			const std::string& displayName, const std::string& aliases, Byte substitutionCharacter) : EncoderFactoryBase(name, mib, displayName, 1, 1, aliases, substitutionCharacter) {}

		/// Destructor.
		template<typename MappingTable>
		inline implementation::sbcs::SingleByteEncoderFactory<MappingTable>::~SingleByteEncoderFactory() BOOST_NOEXCEPT {
		}

		/// @see EncoderFactory#create
		template<typename MappingTable> std::unique_ptr<Encoder>
		implementation::sbcs::SingleByteEncoderFactory<MappingTable>::create() const BOOST_NOEXCEPT {
			return detail::createSingleByteEncoder(MappingTable::VALUES, *this);
		}
	}
} // namespace ascension.encoding

#endif // !ASCENSION_ENCODER_HPP
