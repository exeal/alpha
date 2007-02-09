/**
 * @file encoder.hpp
 * @author exeal
 * @date 2004-2007
 */

#ifndef ASCENSION_ENCODER_HPP
#define ASCENSION_ENCODER_HPP

#include "unicode-utils.hpp"
#include <cassert>
#include <set>
#include <map>
#include <memory>	// std::auto_ptr
#include <windows.h>


namespace ascension {

	/// About encodings.
	namespace encodings {

	/// Windows code page.
	typedef uint CodePage;

	//	Windows コードページに無い、或いは自分で実装する文字コード
	const CodePage
		CPEX_UNICODE_UTF16LE		= 1200,		///< UTF-16
		CPEX_UNICODE_UTF16BE		= 1201,		///< UTF-16 big endian
		CPEX_UNICODE_UTF32LE		= 12000,	///< UTF-32
		CPEX_UNICODE_UTF32BE		= 12001,	///< UTF-32 big endian
#ifndef ASCENSION_NO_EXTENDED_ENCODINGS
		CPEX_AUTODETECT				= 50001,	///< Auto-detect
		CPEX_JAPANESE_AUTODETECT	= 50932,	///< Japanese (Auto-detect)
		CPEX_KOREAN_AUTODETECT		= 50949,	///< Korean (Auto-detect)
		CPEX_AUTODETECT_SYSTEMLANG	= 70000,	///< Auto-detect (System language)
		CPEX_AUTODETECT_USERLANG	= 70001,	///< Auto-detect (User language)
#endif /* !ASCENSION_NO_EXTENDED_ENCODINGS */
		CPEX_UNICODE_AUTODETECT		= 70010,	///< Unicode (Auto-detect)
#ifndef ASCENSION_NO_EXTENDED_ENCODINGS
		CPEX_UNICODE_UTF5			= 70011,	///< UTF-5
		CPEX_ARMENIAN_AUTODETECT	= 70020,	///< Armenian (Auto-detect)
		CPEX_ARMENIAN_ARMSCII7		= 70021,	///< Armenian (ARMSCII-7)
		CPEX_ARMENIAN_ARMSCII8		= 70022,	///< Armenian (ARMSCII-8)
		CPEX_ARMENIAN_ARMSCII8A		= 70023,	///< Armenian (ARMSCII-8A)
		CPEX_VIETNAMESE_AUTODETECT	= 70030,	///< Vietnamese (Auto-detect)
		CPEX_VIETNAMESE_TCVN		= 70031,	///< Vietnamese (TCVN)
		CPEX_VIETNAMESE_VISCII		= 70032,	///< Vietnamese (VISCII)
		CPEX_VIETNAMESE_VPS			= 70033,	///< Vietnamese (VPS)
		CPEX_JAPANESE_ISO2022JP		= 70040,	///< Japanese (ISO-2022-JP)
		CPEX_JAPANESE_SHIFTJIS		= 70041,	///< Japanese (Shift JIS)
		CPEX_JAPANESE_ISO2022JP1	= 70042,	///< Japanese (ISO-2022-JP-1)
		CPEX_JAPANESE_ISO2022JP2	= 70043,	///< Japanese (ISO-2022-JP-2)
		CPEX_JAPANESE_EUC			= 70044,	///< Japanese (EUC)
		CPEX_JAPANESE_ISO2022JP2004				= 70045,	///< Japanese (ISO-2022-JP-2004)
		CPEX_JAPANESE_ISO2022JP2004_STRICT		= 70046,	///< Japanese (ISO-2022-JP-2004-strict)
		CPEX_JAPANESE_ISO2022JP2004_COMPATIBLE	= 70047,	///< Japanese (ISO-2022-JP-2004-compatible)
		CPEX_JAPANESE_ISO2022JP3			= 70048,	///< Japanese (ISO-2022-JP-3)
		CPEX_JAPANESE_ISO2022JP3_STRICT		= 70049,	///< Japanese (ISO-2022-JP-3-strict)
		CPEX_JAPANESE_ISO2022JP3_COMPATIBLE	= 70050,	///< Japanese (ISO-2022-JP-3-compatible)
		CPEX_JAPANESE_SHIFTJIS2004	= 70051,	///< Japanese (Shift_JIS-2004)
		CPEX_JAPANESE_EUCJIS2004	= 70052,	///< Japanese (EUC-JIS-2004)
		CPEX_MULTILINGUAL_ISO2022_7BIT		= 70060,	///< Multilingual (ISO-2022, 7-bit)
		CPEX_MULTILINGUAL_ISO2022_7BITSS2	= 70061,	///< Multilingual (ISO-2022, 7-bit, SS2)
		CPEX_MULTILINGUAL_ISO2022_7BITSISO	= 70062,	///< Multilingual (ISO-2022, 7-bit, SI/SO)
		CPEX_MULTILINGUAL_ISO2022_8BITSS2	= 70063,	///< Multilingual (ISO-2022, 8-bit, SS2)
		CPEX_UNCATEGORIZED_BINARY	= 70070,	///< Binary
		CPEX_UNCATEGORIZED_NEXTSTEP	= 70071,	///< NEXTSTEP
		CPEX_UNCATEGORIZED_ATARIST	= 70072,	///< Atari ST/TT
		CPEX_THAI_TIS620	= 70080,	///< Thai (TIS 620-2533:1990)
		CPEX_LAO_MULELAO	= 70090,	///< Lao (MuleLao)
		CPEX_LAO_CP1132		= 70091,	///< Lao (ibm-1132)
		CPEX_LAO_CP1133		= 70092,	///< Lao (ibm-1133)
		CPEX_IRISH_IS434	= 70100,	///< Irelandic (I.S. 434:1999)
		CPEX_TAMIL_TAB		= 70110,	///< Tamil (TAB)
		CPEX_TAMIL_TAM		= 70111,	///< Tamil (TAM)
		CPEX_TAMIL_TSCII	= 70112,	///< Tamil (TSCII 1.7)
		CPEX_HINDI_MACINTOSH	= 70115,	///< Hindi (Macintosh, Devanagari)
		CPEX_GUJARATI_MACINTOSH	= 70116,	///< Gujarati (Macintosh)
		CPEX_PANJABI_MACINTOSH	= 70117,	///< Punjabi (Macintosh, グルムキー文字)
		CPEX_CYRILLIC_MACINTOSH							= 10007,	///< Cyrillic (Macintosh)
		CPEX_CYRILLIC_KOI8R								= 20866,	///< Russian (KOI8-R)
		CPEX_CYRILLIC_RUSSIANSUPPORTFORDOS3				= 70120,	///< Russian (DOS 3 ロシア語サポート)
		CPEX_CYRILLIC_RUSSIANSUPPORTFORDOS4ACADEMIC		= 70121,	///< Russian (DOS 4 アカデミックロシア語サポート)
		CPEX_CYRILLIC_RUSSIANSUPPORTFORDOS3NONACADEMIC	= 70122,	///< Russian (DOS 4 非アカデミックロシア語サポート)
		CPEX_CYRILLIC_SOVIETKOI8BASIC					= 70123,	///< Russian (ソビエト KOI-8 基本)
		CPEX_CYRILLIC_SOVIETKOI8ALTERNATIVE				= 70124,	///< Russian (ソビエト KOI-8 代替)
		CPEX_CYRILLIC_SOVIETKOI7						= 70125,	///< Russian (ソビエト KOI-7)
		CPEX_CYRILLIC_ECMA								= 70126,	///< Cyrillic (ISO-IR-111, ECMA)
		CPEX_CYRILLIC_KOI8RU							= 70127,	///< Cyrillic (KOI8-RU)
		CPEX_CYRILLIC_KOI8UNIFIED						= 70128,	///< Cyrillic (KOI8 統合)
		CPEX_ISO8859_1	= 28591,	///< 西ヨーロッパ (ISO-8859-1)
		CPEX_ISO8859_2	= 28592,	///< 中央ヨーロッパ (ISO-8859-2)
		CPEX_ISO8859_3	= 28593,	///< 南ヨーロッパ (ISO-8859-3)
		CPEX_ISO8859_4	= 28594,	///< Baltic (ISO-8859-4)
		CPEX_ISO8859_5	= 28595,	///< Cyrillic (ISO-8859-5)
		CPEX_ISO8859_6	= 28596,	///< Arabic (ISO-8859-6)
		CPEX_ISO8859_7	= 28597,	///< Greek (ISO-8859-7)
		CPEX_ISO8859_8	= 28598,	///< Hebrew (ISO-8859-8)
		CPEX_ISO8859_9	= 28599,	///< Turkish (ISO-8859-9)
		CPEX_ISO8859_10	= 28600,	///< 北欧 (ISO-8859-10)
		CPEX_ISO8859_11	= 28601,	///< Thai (ISO-8859-11)
		CPEX_ISO8859_13	= 28603,	///< Baltic (ISO-8859-13)
		CPEX_ISO8859_14	= 28604,	///< Keltic (ISO-8859-14)
		CPEX_ISO8859_15	= 28605,	///< 西ヨーロッパ (ISO-8859-15)
		CPEX_ISO8859_16	= 28606;	///< 中央ヨーロッパ (ISO-8859-16)
#endif /* !ASCENSION_NO_EXTENDED_ENCODINGS */

	// バイトオーダーマーク
	const uchar	UTF16LE_BOM[] = "\xFF\xFE";			///< BOM of UTF-16 little endian
	const uchar	UTF16BE_BOM[] = "\xFE\xFF";			///< BOM of UTF-16 big endian
#ifndef ASCENSION_NO_EXTENDED_ENCODINGS
	const uchar	UTF32LE_BOM[] = "\xFF\xFF\x00\x00";	///< BOM of UTF-16 little endian
	const uchar	UTF32BE_BOM[] = "\xFE\xFF\x00\x00";	///< BOM of UTF-16 big endian
#endif /* !ASCENSION_NO_EXTENDED_ENCODINGS */
	const uchar	UTF8_BOM[] = "\xEF\xBB\xBF";		///< BOM of UTF-8


	// Encoder //////////////////////////////////////////////////////////////

	// 変換できない場合の既定の文字 (ユーザの言語から取得したほうがいいかもしれないが)
	const uchar NATIVE_DEFAULT_CHARACTER = '?';

	// 文字セットにマップされない文字
	const wchar_t RP__CH = REPLACEMENT_CHARACTER;
	const uchar N__A = 0x00;

	template<class Ch> void setDefaultChar(Ch& ch);
	template<> inline void setDefaultChar(char& ch) {ch = NATIVE_DEFAULT_CHARACTER;}
	template<> inline void setDefaultChar(uchar& ch) {ch = NATIVE_DEFAULT_CHARACTER;}
	template<> inline void setDefaultChar(ushort& ch) {ch = NATIVE_DEFAULT_CHARACTER;}
	template<> inline void setDefaultChar(wchar_t& ch) {ch = REPLACEMENT_CHARACTER;}
	template<> inline void setDefaultChar(ulong& ch) {ch = REPLACEMENT_CHARACTER;}

	#define BIT7_MASK(c)	static_cast<uchar>((c) & 0x7F)
	#define BIT8_MASK(c)	static_cast<uchar>((c) & 0xFF)
	#define BIT16_MASK(c)	static_cast<ushort>((c) & 0xFFFF)
	#define UTF16_MASK(c)	static_cast<wchar_t>((c) & 0xFFFF)

	#define CONFIRM_ILLEGAL_CHAR(lhs)										\
		{																	\
			if(callback == 0 || callback->unconvertableCharacterFound()) {	\
				setDefaultChar(lhs);										\
				callback = 0;												\
			} else															\
				return 0;													\
		}

	#define CFU_ARGLIST											\
		uchar* dest, std::size_t destLength,					\
		const wchar_t* src, std::size_t srcLength /* = -1 */,	\
		IUnconvertableCharCallback* callback /* = 0 */

	#define CTU_ARGLIST										\
		wchar_t* dest, std::size_t destLength,				\
		const uchar* src, std::size_t srcLength /* = -1 */,	\
		IUnconvertableCharCallback* callback /* = 0 */

	#define CFU_CHECKARGS()				\
		assert(dest != 0 && src != 0);	\
		if(srcLength == -1)				\
			srcLength = wcslen(src)

	#define CTU_CHECKARGS()				\
		assert(dest != 0 && src != 0);	\
		if(srcLength == -1)				\
			srcLength = strlen(reinterpret_cast<const char*>(src))

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

	/// 変換できない文字の処理するコールバック
	class IUnconvertableCharCallback {
	public:
		/// Destructor.
		virtual ~IUnconvertableCharCallback() throw() {}
		/**
		 * ファイル読み込み時に Unicode に変換できない、
		 * または保存時にネイティブコードに変換できない文字が見つかったときに呼び出される。
		 * 戻り値によりその文字をどう扱うを決める。
		 * このメソッドは1度の処理で1度しか呼び出されない
		 * @retval true 変換できない文字を既定の文字に変換して処理を続行する
		 * @retval false 読み込み/保存を直ちに中止する (変換メソッドは 0 を返す)
		 */
		virtual bool unconvertableCharacterFound() = 0;
	};

	/**
	 * An abstract encoder.
	 * @note This class will be rewritten in future.
	 */
	class Encoder : public manah::Noncopyable {
		// コンストラクタ
	protected:
		Encoder() {}
	public:
		virtual ~Encoder() {}

		// メソッド
	public:
		/**
		 * UTF-16 から変換
		 * @param[out] dest 変換先
		 * @param[in] destLength 変換先の長さ
		 * @param[in] src 変換元
		 * @param[in] srcLength 変換元の文字列長
		 * @param[in] callback 変換できない文字を処理するためのコールバック。@c null でもよい
		 * @return 変換後の文字数
		 */
		virtual std::size_t fromUnicode(uchar* dest, std::size_t destLength,
			const wchar_t* src, std::size_t srcLength = -1, IUnconvertableCharCallback* callback = 0) = 0;
		/**
		 * UTF-16 に変換
		 * @param[out] dest 変換先
		 * @param[in] destLength 変換先の文字列長
		 * @param[in] src 変換元
		 * @param[in] srcLength 変換元の文字列長
		 * @param[in] callBack 変換できない文字を処理するためのコールバック。@c null でもよい
		 * @return 変換後の文字数
		 */
		virtual std::size_t toUnicode(wchar_t* dest, std::size_t destLength,
			const uchar* src, std::size_t srcLength = -1, IUnconvertableCharCallback* callBack = 0) = 0;
		/// UCS 1 文字をネイティブ文字に変換するのに必要な最大バイト長を返す
		virtual uchar getMaxNativeCharLength() const = 0;
		/// ネイティブ文字 1 バイトを UCS 文字に変換するのに必要な最大長さを返す (UTF-16 単位)
		virtual uchar getMaxUCSCharLength() const = 0;
	};


	/// Factory of encoders.
	class EncoderFactory {
	public:
		// データ型
		typedef std::auto_ptr<Encoder>(*EncoderProducer)();
		typedef void(*CodePageDetector)(const uchar*, std::size_t, CodePage&, std::size_t&);
		// メソッド
		std::auto_ptr<Encoder>	createEncoder(CodePage cp);
		CodePage				detectCodePage(const uchar* src, std::size_t length, CodePage cp);
		void					enumCodePages(std::set<CodePage>& codePages) const;
		static EncoderFactory&	getInstance();
		CodePageDetector		getUnicodeDetector() const;
		bool					isCodePageForAutoDetection(CodePage cp) const;
		bool					isCodePageForReadOnly(CodePage cp) const;
		bool					isValidCodePage(CodePage cp) const;

		bool	registerCodePageForReadOnly(CodePage cp);
		bool	registerDetector(CodePage cp, CodePageDetector factoryMethod);
		bool	registerEncoder(CodePage cp, EncoderProducer factoryMethod);

	private:
		typedef std::map<CodePage, EncoderProducer> EncoderMap;
		typedef std::map<CodePage, CodePageDetector> DetectorMap;
		EncoderMap registeredEncoders_;
		DetectorMap registeredDetectors_;
		std::set<CodePage> codePagesForReadOnly_;
	};


	#define BEGIN_ENCODER_DEFINITION()	namespace {

	#define END_ENCODER_DEFINITION()	}

	#define DEFINE_ENCODER_CLASS_(cp, name)							\
		class Encoder_##name : public Encoder {						\
		private:													\
			Encoder_##name();										\
		public:														\
			std::size_t	fromUnicode(CFU_ARGLIST);					\
			std::size_t	toUnicode(CTU_ARGLIST);						\
			uchar		getMaxNativeCharLength() const;				\
			uchar		getMaxUCSCharLength() const;				\
			static std::auto_ptr<Encoder> create() {				\
				return std::auto_ptr<Encoder>(new Encoder_##name);}	\
		};															\
		const bool res##name =										\
			EncoderFactory::getInstance().registerEncoder(cp, &Encoder_##name::create);

	#define DEFINE_ENCODER_CLASS(cp, name, cch, ccp)						\
		DEFINE_ENCODER_CLASS_(cp, name)										\
		Encoder_##name::Encoder_##name() {}									\
		uchar Encoder_##name::getMaxNativeCharLength() const {return cch;}	\
		uchar Encoder_##name::getMaxUCSCharLength() const {return ccp;}

	#define DEFINE_DETECTOR(cp, name)														\
		namespace {																			\
			void detectCodePage_##name(const uchar* buffer,									\
				std::size_t length, CodePage& result, std::size_t& convertableLength);		\
			const bool res##name =															\
				EncoderFactory::getInstance().registerDetector(cp, &detectCodePage_##name);	\
		}

	#define REGISTER_READONLY_CODEPAGE(cp)	\
		const bool res##cp = EncoderFactory::getInstance().registerCodePageForReadOnly(cp)


	// Windows 変換テーブルをそのまま使用するエンコーダ
	class WindowsEncoder : public Encoder {
	private:
		WindowsEncoder(CodePage cp) : codePage_(cp) {
			if(!toBoolean(::IsValidCodePage(cp)))
				throw std::invalid_argument("Specified code page is not supported.");
		}
	public:
		std::size_t fromUnicode(CFU_ARGLIST) {
			if(const int result = ::WideCharToMultiByte(codePage_, 0,
					src, static_cast<int>(srcLength), reinterpret_cast<char*>(dest), static_cast<int>(destLength), 0, 0))
				return result;
			return (callback == 0 || callback->unconvertableCharacterFound()) ?
				::WideCharToMultiByte(codePage_, WC_DEFAULTCHAR,
					src, static_cast<int>(srcLength), reinterpret_cast<char*>(dest), static_cast<int>(destLength), 0, 0) : 0;
		}
		std::size_t toUnicode(CTU_ARGLIST) {
			if(const int result = ::MultiByteToWideChar(codePage_, MB_ERR_INVALID_CHARS,
					reinterpret_cast<const char*>(src), static_cast<int>(srcLength), dest, static_cast<int>(destLength)))
				return result;
			return (callback == 0 || callback->unconvertableCharacterFound()) ?
				::MultiByteToWideChar(codePage_, 0,
					reinterpret_cast<const char*>(src), static_cast<int>(srcLength), dest, static_cast<int>(destLength)) : 0;
		}
		uchar getMaxNativeCharLength() const {CPINFO cpi; return toBoolean(::GetCPInfo(codePage_, &cpi)) ? cpi.MaxCharSize : 0;}
		uchar getMaxUCSCharLength() const {return 1;}
		friend class EncoderFactory;
	private:
		const CodePage codePage_;
	};


	/// Returns the singleton instance.
	inline EncoderFactory& EncoderFactory::getInstance() {static EncoderFactory instance; return instance;}

	/// Returns the Unicode auto detector or @c null if not registered.
	inline EncoderFactory::CodePageDetector EncoderFactory::getUnicodeDetector() const {
		DetectorMap::const_iterator	it = registeredDetectors_.find(CPEX_UNICODE_AUTODETECT);
		return (it != registeredDetectors_.end()) ? it->second : 0;
	}

	/// Returns true if the specified code page is for auto-detection.
	inline bool EncoderFactory::isCodePageForAutoDetection(CodePage cp) const {return registeredDetectors_.find(cp) != registeredDetectors_.end();}

	/// Returns true if the specified code page supports only native-to-UCS conversion.
	inline bool EncoderFactory::isCodePageForReadOnly(CodePage cp) const {return codePagesForReadOnly_.find(cp) != codePagesForReadOnly_.end();}

	/**
	 * Returns the specified code page is valid.
	 * @param cp the code page
	 * @return true if @a cp is valid
	 */
	inline bool EncoderFactory::isValidCodePage(CodePage cp) const {
		return toBoolean(::IsValidCodePage(cp))
			|| isCodePageForAutoDetection(cp)
			|| registeredEncoders_.find(cp) != registeredEncoders_.end();
	}

	/**
	 * Registers the new code page supports only native-to-UCS conversion.
	 * @param cp the code page
	 * @return succeeded or not
	 */
	inline bool EncoderFactory::registerCodePageForReadOnly(CodePage cp) {
		return codePagesForReadOnly_.insert(cp).second;	// VC extended return
	}

	/**
	 * Registers the new auto encoding detector.
	 * @param cp the code page
	 * @param factoryMethod the function produces auto detectors
	 * @return succeeded or not
	 */
	inline bool EncoderFactory::registerDetector(CodePage cp, CodePageDetector factoryMethod) {
		assert(factoryMethod != 0);
		return registeredDetectors_.insert(std::make_pair(cp, factoryMethod)).second;	// VC extended return
	}

	/**
	 * Registers the new encoder.
	 * @param cp the code page
	 * @param factoryMethod the function produces encoders
	 * @return succeeded or not
	 */
	inline bool EncoderFactory::registerEncoder(CodePage cp, EncoderProducer factoryMethod) {
		assert(factoryMethod != 0);
		return registeredEncoders_.insert(std::make_pair(cp, factoryMethod)).second;	// VC extended return
	}

	}
} // namespace ascension::encodings

#endif /* ASCENSION_ENCODER_HPP */
