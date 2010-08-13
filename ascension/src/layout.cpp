/**
 * @file layout.cpp
 * @author exeal
 * @date 2003-2006 (was LineLayout.cpp)
 * @date 2006-2010
 */

#include <ascension/layout.hpp>
#include <ascension/viewer.hpp>
#include <limits>	// std.numeric_limits
#include <boost/tr1/unordered_map.hpp>
#include <usp10.h>
using namespace ascension;
using namespace ascension::kernel;
using namespace ascension::layout;
using namespace ascension::presentation;
using namespace ascension::text;
using namespace ascension::text::ucd;
using namespace manah::win32;
using namespace manah::win32::gdi;
using namespace std;

#pragma comment(lib, "usp10.lib")

//#define TRACE_LAYOUT_CACHES
extern bool DIAGNOSE_INHERENT_DRAWING;

namespace {
	// SystemColors caches the system colors.
	class SystemColors {
	public:
		SystemColors() /*throw()*/ {update();}
		COLORREF get(int index) const {assert(index >= 0 && index < MANAH_COUNTOF(c_)); return c_[index];}
		COLORREF serve(const Color& color, int index) const {return (color != Color()) ? color.asCOLORREF() : get(index);}
		void update() /*throw()*/ {for(int i = 0; i < MANAH_COUNTOF(c_); ++i) c_[i] = ::GetSysColor(i);}
	private:
		COLORREF c_[128];
	} systemColors;

	const class ScriptProperties {
	public:
		ScriptProperties() /*throw()*/ : p_(0), c_(0) {::ScriptGetProperties(&p_, &c_);}
		const SCRIPT_PROPERTIES& get(int script) const {if(script >= c_) throw out_of_range("script"); return *p_[script];}
		int numberOfOfScripts() const /*throw()*/ {return c_;}
	private:
		const SCRIPT_PROPERTIES** p_;
		int c_;
	} scriptProperties;

	class UserSettings {
	public:
		UserSettings() /*throw()*/ {update();}
		LANGID defaultLanguage() const /*throw()*/ {return languageID_;}
		const SCRIPT_DIGITSUBSTITUTE& digitSubstitution(bool ignoreUserOverride) const /*throw()*/ {
			return ignoreUserOverride ? digitSubstitutionNoUserOverride_ : digitSubstitution_;
		}
		void update() /*throw()*/ {
			languageID_ = ::GetUserDefaultLangID();
			::ScriptRecordDigitSubstitution(LOCALE_USER_DEFAULT, &digitSubstitution_);
			::ScriptRecordDigitSubstitution(LOCALE_USER_DEFAULT | LOCALE_NOUSEROVERRIDE, &digitSubstitutionNoUserOverride_);
		}
	private:
		LANGID languageID_;
		SCRIPT_DIGITSUBSTITUTE digitSubstitution_, digitSubstitutionNoUserOverride_;
	} userSettings;

	int CALLBACK checkFontInstalled(ENUMLOGFONTEXW*, NEWTEXTMETRICEXW*, DWORD, LPARAM param) {
		*reinterpret_cast<bool*>(param) = true;
		return 0;
	}

	inline int estimateNumberOfGlyphs(length_t length) {return static_cast<int>(length) * 3 / 2 + 16;}
	String fallback(int script);
	inline bool isC0orC1Control(CodePoint c) /*throw()*/ {return c < 0x20 || c == 0x7f || (c >= 0x80 && c < 0xa0);}
	ReadingDirection lineTerminatorOrientation(const LineStyle& style, tr1::shared_ptr<const LineStyle> defaultStyle) /*throw()*/;
	int pixels(const DC& dc, const Length& length, bool vertical, const IFontMetrics& fontMetrics);
	HRESULT resolveNumberSubstitution(const NumberSubstitution* configuration, SCRIPT_CONTROL& sc, SCRIPT_STATE& ss);
	template<typename T> inline int round(T value) {return static_cast<int>(floor(value + 0.5));}
	bool uniscribeSupportsVSS() /*throw()*/;
	LANGID userCJKLanguage() /*throw()*/;

#ifdef ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
	typedef tr1::unordered_map<uint32_t, uint16_t> IdeographicVariationSequences;
	void generateIVSMappings(const void* cmapData, size_t length, IdeographicVariationSequences& ivs);
#endif // !ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND

	// New Uniscribe features (usp10.dll 1.6) dynamic loading
#if(!defined(UNISCRIBE_OPENTYPE) || UNISCRIBE_OPENTYPE < 0x0100)
	typedef ULONG OPENTYPE_TAG;
	static const OPENTYPE_TAG SCRIPT_TAG_UNKNOWN = 0x00000000;
	struct OPENTYPE_FEATURE_RECORD {
		OPENTYPE_TAG tagFeature;
		LONG lParameter;
	};
	struct SCRIPT_CHARPROP {
		WORD fCanGlyphAlone : 1;
		WORD reserved : 15;
	};
	struct SCRIPT_GLYPHPROP {
		SCRIPT_VISATTR sva;
		WORD reserved;
	};
	struct TEXTRANGE_PROPERTIES {
		OPENTYPE_FEATURE_RECORD* potfRecords;
		int cotfRecords;
	};
#endif // not usp10-1.6
	ASCENSION_DEFINE_SHARED_LIB_ENTRIES(Uniscribe16, 4);
	ASCENSION_SHARED_LIB_ENTRY(Uniscribe16, 0, "ScriptPlaceOpenType",
		HRESULT(WINAPI* signature)(
			HDC, SCRIPT_CACHE*, SCRIPT_ANALYSIS*, OPENTYPE_TAG, OPENTYPE_TAG, int*,
			TEXTRANGE_PROPERTIES**, int, const WCHAR*, WORD*, SCRIPT_CHARPROP*, int, const WORD*,
			const SCRIPT_GLYPHPROP*, int, int*, GOFFSET*, ABC*));
	ASCENSION_SHARED_LIB_ENTRY(Uniscribe16, 1, "ScriptShapeOpenType",
		HRESULT(WINAPI* signature)(
			HDC, SCRIPT_CACHE*, SCRIPT_ANALYSIS*, OPENTYPE_TAG, OPENTYPE_TAG, int*,
			TEXTRANGE_PROPERTIES**, int, const WCHAR*, int, int, WORD*, SCRIPT_CHARPROP*, WORD*,
			SCRIPT_GLYPHPROP*, int*));
	ASCENSION_SHARED_LIB_ENTRY(Uniscribe16, 2, "ScriptItemizeOpenType",
		HRESULT(WINAPI* signature)(
			const WCHAR*, int, int, const SCRIPT_CONTROL*, const SCRIPT_STATE*, SCRIPT_ITEM*,
			OPENTYPE_TAG*, int*));
//#ifdef ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
	ASCENSION_SHARED_LIB_ENTRY(Uniscribe16, 3, "ScriptSubstituteSingleGlyph",
		HRESULT(WINAPI *signature)(
			HDC, SCRIPT_CACHE*, SCRIPT_ANALYSIS*, OPENTYPE_TAG, OPENTYPE_TAG, OPENTYPE_TAG, LONG,
			WORD, WORD*));
//#endif // ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
	auto_ptr<ascension::internal::SharedLibrary<Uniscribe16> > uspLib(
		new ascension::internal::SharedLibrary<Uniscribe16>("usp10.dll"));
} // namespace @0

// file-local free functions ////////////////////////////////////////////////

namespace {

	void dumpRuns(const LineLayout& layout) {
#ifdef _DEBUG
		ostringstream s;
		layout.dumpRuns(s);
		::OutputDebugStringA(s.str().c_str());
#endif // _DEBUG
	}

	String fallback(int script) {
		if(script <= Script::FIRST_VALUE || script == Script::INHERITED
				|| script == Script::KATAKANA_OR_HIRAGANA || script >= Script::LAST_VALUE)
			throw UnknownValueException("script");

		static map<int, const String> associations;
		static const WCHAR MS_P_GOTHIC[] = L"\xff2d\xff33 \xff30\x30b4\x30b7\x30c3\x30af";	// "ＭＳ Ｐゴシック"
		if(associations.empty()) {
			associations.insert(make_pair(Script::ARABIC, L"Microsoft Sans Serif"));
			associations.insert(make_pair(Script::CYRILLIC, L"Microsoft Sans Serif"));
			associations.insert(make_pair(Script::GREEK, L"Microsoft Sans Serif"));
			associations.insert(make_pair(Script::HANGUL, L"Gulim"));
			associations.insert(make_pair(Script::HEBREW, L"Microsoft Sans Serif"));
//			associations.insert(make_pair(Script::HIRAGANA, MS_P_GOTHIC));
//			associations.insert(make_pair(Script::KATAKANA, MS_P_GOTHIC));
			associations.insert(make_pair(Script::LATIN, L"Tahoma"));
			associations.insert(make_pair(Script::THAI, L"Tahoma"));
			// Windows 2000
			associations.insert(make_pair(Script::ARMENIAN, L"Sylfaen"));
			associations.insert(make_pair(Script::DEVANAGARI, L"Mangal"));
			associations.insert(make_pair(Script::GEORGIAN, L"Sylfaen"));	// partial support?
			associations.insert(make_pair(Script::TAMIL, L"Latha"));
			// Windows XP
			associations.insert(make_pair(Script::GUJARATI, L"Shruti"));
			associations.insert(make_pair(Script::GURMUKHI, L"Raavi"));
			associations.insert(make_pair(Script::KANNADA, L"Tunga"));
			associations.insert(make_pair(Script::SYRIAC, L"Estrangelo Edessa"));
			associations.insert(make_pair(Script::TELUGU, L"Gautami"));
			associations.insert(make_pair(Script::THAANA, L"MV Boli"));
			// Windows XP SP2
			associations.insert(make_pair(Script::BENGALI, L"Vrinda"));
			associations.insert(make_pair(Script::MALAYALAM, L"Kartika"));
			// Windows Vista
			associations.insert(make_pair(Script::CANADIAN_ABORIGINAL, L"Euphemia"));
			associations.insert(make_pair(Script::CHEROKEE, L"Plantagenet Cherokee"));
			associations.insert(make_pair(Script::ETHIOPIC, L"Nyala"));
			associations.insert(make_pair(Script::KHMER, L"DaunPenh"));	// or "MoolBoran"
			associations.insert(make_pair(Script::LAO, L"DokChampa"));
			associations.insert(make_pair(Script::MONGOLIAN, L"Mongolian Baiti"));
			associations.insert(make_pair(Script::ORIYA, L"Kalinga"));
			associations.insert(make_pair(Script::SINHALA, L"Iskoola Pota"));
			associations.insert(make_pair(Script::TIBETAN, L"Microsoft Himalaya"));
			associations.insert(make_pair(Script::YI, L"Microsoft Yi Baiti"));
			// CJK
			const LANGID uiLang = userCJKLanguage();
			switch(PRIMARYLANGID(uiLang)) {	// yes, this is not enough...
			case LANG_CHINESE:
				associations.insert(make_pair(Script::HAN, (SUBLANGID(uiLang) == SUBLANG_CHINESE_TRADITIONAL
					&& SUBLANGID(uiLang) == SUBLANG_CHINESE_HONGKONG) ? L"PMingLiu" : L"SimSun")); break;
			case LANG_JAPANESE:
				associations.insert(make_pair(Script::HAN, MS_P_GOTHIC)); break;
			case LANG_KOREAN:
				associations.insert(make_pair(Script::HAN, L"Gulim")); break;
			default:
				{
					ScreenDC dc;
					bool installed = false;
					LOGFONTW lf;
					memset(&lf, 0, sizeof(LOGFONTW));
#define ASCENSION_SELECT_INSTALLED_FONT(charset, fontname)			\
	lf.lfCharSet = charset;											\
	wcscpy(lf.lfFaceName, fontname);								\
	::EnumFontFamiliesExW(dc.get(), &lf,							\
		reinterpret_cast<FONTENUMPROCW>(checkFontInstalled),		\
		reinterpret_cast<LPARAM>(&installed), 0);					\
	if(installed) {													\
		associations.insert(make_pair(Script::HAN, lf.lfFaceName));	\
		break;														\
	}
					ASCENSION_SELECT_INSTALLED_FONT(GB2312_CHARSET, L"SimSun")
					ASCENSION_SELECT_INSTALLED_FONT(SHIFTJIS_CHARSET, MS_P_GOTHIC)
					ASCENSION_SELECT_INSTALLED_FONT(HANGUL_CHARSET, L"Gulim")
					ASCENSION_SELECT_INSTALLED_FONT(CHINESEBIG5_CHARSET, L"PMingLiu")
#undef ASCENSION_SELECT_INSTALLED_FONT
				}
				break;
			}
			if(associations.find(Script::HAN) != associations.end()) {
				associations.insert(make_pair(Script::HIRAGANA, associations[Script::HAN]));
				associations.insert(make_pair(Script::KATAKANA, associations[Script::HAN]));
			}
		}

		map<int, const String>::const_iterator i(associations.find(script));
		return (i != associations.end()) ? i->second : String();
	}

#ifdef ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
	template<size_t bytes> uint32_t readBytes(const uint8_t*& p);
	template<> inline uint32_t readBytes<1>(const uint8_t*& p) {
		return *(p++);
	}
	template<> inline uint32_t readBytes<2>(const uint8_t*& p) {
		p += 2;
		return (p[-2] << 8) | (p[-1] << 0);
	}
	template<> inline uint32_t readBytes<3>(const uint8_t*& p) {
		p += 3;
		return (p[-3] << 16) | (p[-2] << 8) | (p[-1] << 0);
	}
	template<> inline uint32_t readBytes<4>(const uint8_t*& p) {
		p += 4;
		return (p[-4] << 24) | (p[-3] << 16) | (p[-2] << 8) | (p[-1] << 0);
	}
	void generateIVSMappings(const void* cmapData, size_t length, IdeographicVariationSequences& ivs) {
//	if(length > ) {
			const uint8_t* p = static_cast<const uint8_t*>(cmapData);
			const uint32_t numberOfSubtables = readBytes<2>(p += 2);
			const uint8_t* uvsSubtable = 0;
			for(uint16_t i = 0; i < numberOfSubtables; ++i) {
				const uint32_t platformID = readBytes<2>(p);
				const uint32_t encodingID = readBytes<2>(p);
				const uint32_t offset = readBytes<4>(p);
				const uint8_t* temp = static_cast<const uint8_t*>(cmapData) + offset;
				const uint32_t format = readBytes<2>(temp);
				if(format == 14 && platformID == 0 && encodingID == 5) {
					uvsSubtable = temp - 2;
					break;
				}
			}
			if(uvsSubtable != 0) {
				p = uvsSubtable + 6;
				const uint32_t numberOfRecords = readBytes<4>(p);
				for(uint32_t i = 0; i < numberOfRecords; ++i) {
					const uint32_t varSelector = readBytes<3>(p);
					const uint32_t nonDefaultUVSOffset = readBytes<4>(p += 4);
					if(nonDefaultUVSOffset != 0) {
						const uint8_t* q = uvsSubtable/*static_cast<const uint8_t*>(cmapData)*/ + nonDefaultUVSOffset;
						const uint32_t numberOfMappings = readBytes<4>(q);
						for(uint32_t j = 0; j < numberOfMappings; ++j) {
							const uint32_t unicodeValue = readBytes<3>(q);
							const uint32_t glyphID = readBytes<2>(q);
							ivs.insert(make_pair(((varSelector - 0x0e0100u) << 24) | unicodeValue, static_cast<uint16_t>(glyphID)));
						}
					}
				}
			}
//		}
	}
#endif // ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND

	inline ReadingDirection lineTerminatorOrientation(const LineStyle& style, tr1::shared_ptr<const LineStyle> defaultStyle) /*throw()*/ {
		const TextAlignment alignment = (style.alignment != INHERIT_TEXT_ALIGNMENT) ?
			style.alignment : ((defaultStyle.get() != 0 && defaultStyle->alignment != INHERIT_TEXT_ALIGNMENT) ?
				defaultStyle->alignment : ASCENSION_DEFAULT_TEXT_ALIGNMENT);
		const ReadingDirection readingDirection = (style.readingDirection != INHERIT_READING_DIRECTION) ?
			style.readingDirection : ((defaultStyle.get() != 0 && defaultStyle->readingDirection != INHERIT_READING_DIRECTION) ?
				defaultStyle->readingDirection : ASCENSION_DEFAULT_TEXT_READING_DIRECTION);
		switch(resolveTextAlignment(alignment, readingDirection)) {
			case ALIGN_LEFT:	return LEFT_TO_RIGHT;
			case ALIGN_RIGHT:	return RIGHT_TO_LEFT;
			case ALIGN_CENTER:
			default:			return readingDirection;
		}
	}

	int pixels(const DC& dc, const Length& length, bool vertical, const IFontMetrics& fontMetrics) {
		if(equals(length.value, 0.0))
			return 0;
		switch(length.unit) {
			case Length::EM_HEIGHT:
				return static_cast<int>(static_cast<double>(fontMetrics.emHeight()) * length.value);
			case Length::X_HEIGHT:
				return static_cast<int>(static_cast<double>(fontMetrics.xHeight()) * length.value);
			case Length::PIXELS:
				return round(length.value);
			case Length::INCHES:	case Length::CENTIMETERS:	case Length::MILLIMETERS:
			case Length::POINTS:	case Length::PICAS:			case Length::DIPS: {
				const double dpi = dc.getDeviceCaps(vertical ? LOGPIXELSY : LOGPIXELSX);
				const double inches = length.value * dpi;
				switch(length.unit) {
					case Length::INCHES:
						return round(inches);
					case Length::CENTIMETERS:
						return round(inches / 2.54);
					case Length::MILLIMETERS:
						return round(inches / 25.4);
					case Length::POINTS:
						return round(inches / 72.0);
					case Length::PICAS:
						return round(inches / 6.0);
					case Length::DIPS:
						return round(inches / 96.0);
				}
			}
			default:
				throw UnknownValueException("length.unit");
		}
	}

	HRESULT resolveNumberSubstitution(const NumberSubstitution* configuration, SCRIPT_CONTROL& sc, SCRIPT_STATE& ss) {
		if(configuration == 0 || configuration->method == NumberSubstitution::USER_SETTING)
			return ::ScriptApplyDigitSubstitution(&userSettings.digitSubstitution(
				(configuration != 0) ? configuration->ignoreUserOverride : false), &sc, &ss);

		NumberSubstitution::Method method;
		if(configuration->method == NumberSubstitution::FROM_LOCALE) {
			DWORD n;
			if(::GetLocaleInfoW(
					LOCALE_USER_DEFAULT | (configuration->ignoreUserOverride ? LOCALE_NOUSEROVERRIDE : 0),
					LOCALE_IDIGITSUBSTITUTION | LOCALE_RETURN_NUMBER, reinterpret_cast<LPWSTR>(&n), 2) == 0)
				return HRESULT_FROM_WIN32(::GetLastError());
			switch(n) {
				case 0:
					method = NumberSubstitution::CONTEXTUAL;
					break;
				case 1:
					method = NumberSubstitution::NONE;
					break;
				case 2:
					method = NumberSubstitution::NATIONAL;
					break;
				default:
					return S_FALSE;	// hmm...
			}
		} else
			method = configuration->method;

		// modify SCRIPT_CONTROL and SCRIPT_STATE (without SCRIPT_DIGITSUBSTITUTE)
		sc.uDefaultLanguage = PRIMARYLANGID(userSettings.defaultLanguage());
		switch(method) {
			case NumberSubstitution::CONTEXTUAL:
				sc.fContextDigits = ss.fDigitSubstitute = 1;
				ss.fArabicNumContext = 0;
				break;
			case NumberSubstitution::NONE:
				ss.fDigitSubstitute = 0;
				break;
			case NumberSubstitution::NATIONAL:
				ss.fDigitSubstitute = 1;
				sc.fContextDigits = ss.fArabicNumContext = 0;
				break;
			case NumberSubstitution::TRADITIONAL:
				ss.fDigitSubstitute = ss.fArabicNumContext = 1;
				sc.fContextDigits = 0;
				break;
			default:
				throw invalid_argument("configuration.method");
		}
		return S_OK;
	}

	inline bool uniscribeSupportsIVS() /*throw()*/ {
		static bool checked = false, supports = false;
		if(!checked) {
			static const WCHAR text[] = L"\x82a6\xdb40\xdd00";	// <芦, U+E0100>
			SCRIPT_ITEM items[4];
			int numberOfItems;
			if(SUCCEEDED(::ScriptItemize(text, MANAH_COUNTOF(text) - 1,
					MANAH_COUNTOF(items), 0, 0, items, &numberOfItems)) && numberOfItems == 1)
				supports = true;
			checked = true;
		}
		return supports;
	}

	LANGID userCJKLanguage() /*throw()*/ {
		// this code is preliminary...
		static const WORD CJK_LANGUAGES[] = {LANG_CHINESE, LANG_JAPANESE, LANG_KOREAN};	// sorted by numeric values
		LANGID result = getUserDefaultUILanguage();
		if(find(CJK_LANGUAGES, MANAH_ENDOF(CJK_LANGUAGES), PRIMARYLANGID(result)) != MANAH_ENDOF(CJK_LANGUAGES))
			return result;
		result = ::GetUserDefaultLangID();
		if(find(CJK_LANGUAGES, MANAH_ENDOF(CJK_LANGUAGES), PRIMARYLANGID(result)) != MANAH_ENDOF(CJK_LANGUAGES))
			return result;
		result = ::GetSystemDefaultLangID();
		if(find(CJK_LANGUAGES, MANAH_ENDOF(CJK_LANGUAGES), PRIMARYLANGID(result)) != MANAH_ENDOF(CJK_LANGUAGES))
			return result;
		switch(::GetACP()) {
		case 932:	return MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT);
		case 936:	return MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED);
		case 949:	return MAKELANGID(LANG_KOREAN, SUBLANG_KOREAN);
		case 950:	return MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL);
		}
		return result;
	}
} // namespace @0

void ascension::updateSystemSettings() /*throw()*/ {
	systemColors.update();
	userSettings.update();
}


// layout.* free functions //////////////////////////////////////////////////

/**
 * Returns metrics of underline and/or strikethrough for the currently selected font.
 * @param dc the device context
 * @param[out] baselineOffset the baseline position relative to the top in pixels
 * @param[out] underlineOffset the underline position relative to the baseline in pixels
 * @param[out] underlineThickness the thickness of underline in pixels
 * @param[out] strikethroughOffset the linethrough position relative to the baseline in pixels
 * @param[out] strikethroughThickness the thickness of linethrough in pixels
 * @return succeeded or not
 */
bool layout::getDecorationLineMetrics(HDC dc, int* baselineOffset,
		int* underlineOffset, int* underlineThickness, int* strikethroughOffset, int* strikethroughThickness) /*throw()*/ {
	OUTLINETEXTMETRICW* otm = 0;
	TEXTMETRICW tm;
	if(const UINT c = ::GetOutlineTextMetricsW(dc, 0, 0)) {
		otm = static_cast<OUTLINETEXTMETRICW*>(::operator new(c));
		if(!manah::toBoolean(::GetOutlineTextMetricsW(dc, c, otm)))
			return false;
	} else if(!manah::toBoolean(::GetTextMetricsW(dc, &tm)))
		return false;
	const int baseline = (otm != 0) ? otm->otmTextMetrics.tmAscent : tm.tmAscent;
	if(baselineOffset != 0)
		*baselineOffset = baseline;
	if(underlineOffset != 0)
		*underlineOffset = (otm != 0) ? otm->otmsUnderscorePosition : baseline;
	if(underlineThickness != 0)
		*underlineThickness = (otm != 0) ? otm->otmsUnderscoreSize : 1;
	if(strikethroughOffset != 0)
		*strikethroughOffset = (otm != 0) ? otm->otmsStrikeoutPosition : (baseline / 3);
	if(strikethroughThickness != 0)
		*strikethroughThickness = (otm != 0) ? otm->otmsStrikeoutSize : 1;
	::operator delete(otm);
	return true;
}

/// Returns @c true if complex scripts are supported.
bool layout::supportsComplexScripts() /*throw()*/ {
	return true;
}

/// Returns @c true if OpenType features are supported.
bool layout::supportsOpenTypeFeatures() /*throw()*/ {
	return uspLib->get<0>() != 0;
}


// SystemFont and SystemFonts ///////////////////////////////////////////////

namespace {
	class SystemFont : public AbstractFont, public IFontMetrics {
	public:
		explicit SystemFont(HFONT handle);
#ifdef ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
		pair<bool, uint16_t> ivsGlyph(CodePoint baseCharacter, CodePoint variationSelector) const;
#endif //ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
		// AbstractFont
		Object<HGDIOBJ, &::DeleteObject, HFONT> handle() const /*throw()*/ {return Object<HGDIOBJ, &::DeleteObject, HFONT>(borrowed(handle_.get()));}
		const IFontMetrics& metrics() const /*throw()*/ {return *this;}
		// IFontMetrics
		int ascent() const /*throw()*/ {return ascent_;}
		int averageCharacterWidth() const /*throw()*/ {return averageCharacterWidth_;}
		int descent() const /*throw()*/ {return descent_;}
		int externalLeading() const /*throw()*/ {return externalLeading_;}
		String familyName() const /*throw()*/ {return familyName_;}
		int internalLeading() const /*throw()*/ {return internalLeading_;}
		int xHeight() const /*throw()*/ {return xHeight_;}
	private:
		Object<HGDIOBJ, &::DeleteObject, HFONT> handle_;
		int ascent_, averageCharacterWidth_, descent_, externalLeading_, internalLeading_, xHeight_;
		String familyName_;
#ifdef ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
		auto_ptr<IdeographicVariationSequences> ivsMappings_;
#endif //ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
	};
	class SystemFonts : public IFontCollection {
		tr1::shared_ptr<const AbstractFont> get(const String& familyName, const FontProperties& properties, double sizeAdjust) const;
	private:
		tr1::shared_ptr<const AbstractFont> cache(const String& familyName, const FontProperties& properties, double sizeAdjust);
		struct Hasher : tr1::hash<String> {
			size_t operator()(const pair<String, FontProperties>& value) const {
				return tr1::hash<String>::operator()(value.first) + value.second.weight + value.second.stretch + value.second.style;}
		};
	private:
		typedef tr1::unordered_map<pair<String, FontProperties>, tr1::shared_ptr<AbstractFont>, Hasher> Registry;
		Registry registry_;
	};
} // namespace @0

SystemFont::SystemFont(HFONT handle) : handle_(managed(handle)) {
	ScreenDC dc;
	HFONT oldFont = dc.selectObject(handle_.use());
	::SetGraphicsMode(dc.get(), GM_ADVANCED);
//	const double xdpi = dc.getDeviceCaps(LOGPIXELSX);
//	const double ydpi = dc.getDeviceCaps(LOGPIXELSY);

	// generic font metrics
	TEXTMETRICW tm;
	if(!manah::toBoolean(dc.getTextMetrics(tm)))
		throw runtime_error("GetTextMetricsW failed.");
	ascent_ = tm.tmAscent/* * 96.0 / ydpi*/;
	descent_ = tm.tmDescent/* * 96.0 / ydpi*/;
	internalLeading_ = tm.tmInternalLeading/* * 96.0 / ydpi*/;
	externalLeading_ = tm.tmExternalLeading/* * 96.0 / ydpi*/;
	averageCharacterWidth_ = max<int>(((tm.tmAveCharWidth > 0) ? tm.tmAveCharWidth : ::MulDiv(tm.tmHeight, 56, 100)), 1)/* * 96.0 / xdpi*/;

	// x-height
	GLYPHMETRICS gm;
	const MAT2 temp = {{0, 1}, {0, 0}, {0, 0}, {0, 1}};
	xHeight_ = (dc.getGlyphOutline(L'x', GGO_METRICS, &gm, 0, 0, 0) != GDI_ERROR
		&& gm.gmptGlyphOrigin.y > 0) ? gm.gmptGlyphOrigin.y : round(static_cast<double>(ascent_) * 0.56);
	dc.selectObject(oldFont);

	// family name
	LOGFONTW lf;
	familyName_ = (::GetObjectW(handle_.get(), sizeof(LOGFONTW), &lf) > 0) ? lf.lfFaceName : L"";
}

#ifdef ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
pair<bool, uint16_t> SystemFont::ivsGlyph(CodePoint baseCharacter, CodePoint variationSelector) const {
	if(!isValidCodePoint(baseCharacter))
		throw invalid_argument("baseCharacter");
	else if(!isValidCodePoint(variationSelector))
		throw invalid_argument("variationSelector");
	else if(variationSelector < 0x0e0100ul || variationSelector > 0x0e01eful)
		return make_pair(false, 0);
	if(ivsMappings_.get() == 0) {
		const_cast<SystemFont*>(this)->ivsMappings_.reset(new IdeographicVariationSequences);
		ScreenDC dc;
		HFONT oldFont = dc.selectObject(handle_.get());
		static const uint32_t CMAP_TAG = makeTrueTypeTag("cmap");
		const DWORD bytes = dc.getFontData(CMAP_TAG, 0, 0, 0);
		if(bytes != GDI_ERROR) {
			manah::AutoBuffer<uint8_t> data(new uint8_t[bytes]);
			if(GDI_ERROR != dc.getFontData(CMAP_TAG, 0, data.get(), bytes))
				generateIVSMappings(data.get(), bytes, *const_cast<SystemFont*>(this)->ivsMappings_);
		}
		dc.selectObject(oldFont);
	}
	IdeographicVariationSequences::const_iterator i(ivsMappings_->find(((variationSelector - 0x0e0100ul) << 24) | baseCharacter));
	return (i != ivsMappings_->end()) ? make_pair(true, i->second) : make_pair(false, 0);
}
#endif //ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND

tr1::shared_ptr<const AbstractFont> SystemFonts::get(const String& familyName, const FontProperties& properties, double sizeAdjust) const {
	Registry::const_iterator i(registry_.find(make_pair(familyName, properties)));
	if(i != registry_.end())
		return i->second;
	return const_cast<SystemFonts*>(this)->cache(familyName, properties, sizeAdjust);
}

tr1::shared_ptr<const AbstractFont> SystemFonts::cache(const String& familyName, const FontProperties& properties, double sizeAdjust) {
	if(familyName.length() >= LF_FACESIZE)
		throw length_error("");

	LOGFONTW lf;
	memset(&lf, 0, sizeof(LOGFONTW));
	lf.lfHeight = -round(properties.size);
	lf.lfWeight = properties.weight;
	lf.lfItalic = (properties.style == FontProperties::ITALIC) || (properties.style == FontProperties::OBLIQUE);
	wcscpy(lf.lfFaceName, familyName.c_str());
	Object<HANDLE, &::DeleteObject, HFONT> font(managed(::CreateFontIndirectW(&lf)));
#ifdef _DEBUG
	if(::GetObjectW(font.get(), sizeof(LOGFONTW), &lf) > 0) {
		::OutputDebugStringW(L"[SystemFonts.cache] Created font '");
		::OutputDebugStringW(lf.lfFaceName);
		::OutputDebugStringW(L"' for request '");
		::OutputDebugStringW(familyName.c_str());
		::OutputDebugStringW(L"'.\n");
	}
#endif

	// handle RunStyle.fontSizeAdjust
	if(!equals(sizeAdjust, 0.0) && sizeAdjust > 0.0) {
		ScreenDC dc;
		HFONT oldFont = dc.selectObject(font.get());
		TEXTMETRICW tm;
		if(dc.getTextMetrics(tm)) {
			GLYPHMETRICS gm;
			const MAT2 temp = {{0, 1}, {0, 0}, {0, 0}, {0, 1}};
			const int xHeight =
				(dc.getGlyphOutline(L'x', GGO_METRICS, &gm, 0, 0, 0) != GDI_ERROR && gm.gmptGlyphOrigin.y > 0) ?
					gm.gmptGlyphOrigin.y : round(static_cast<double>(tm.tmAscent) * 0.56);
			const double aspect = static_cast<double>(xHeight) / static_cast<double>(tm.tmHeight - tm.tmInternalLeading);
			FontProperties adjustedProperties(properties);
			adjustedProperties.size = max(properties.size * (sizeAdjust / aspect), 1.0);
			dc.selectObject(oldFont);
			return cache(familyName, adjustedProperties, 0.0);
		}
		dc.selectObject(oldFont);
	}

	// handle 'font-stretch'
	if(properties.stretch != FontProperties::NORMAL_STRETCH) {
		// TODO: this implementation is too simple...
		if(::GetObject(font.get(), sizeof(LOGFONTW), &lf) > 0) {
			static const int WIDTH_RATIOS[] = {1000, 1000, 1000, 500, 625, 750, 875, 1125, 1250, 1500, 2000, 1000};
			lf.lfWidth = ::MulDiv(lf.lfWidth, WIDTH_RATIOS[properties.stretch], 1000);
			Object<HGDIOBJ, &::DeleteObject, HFONT> temp(managed(::CreateFontIndirectW(&lf)));
			if(temp.get() != 0)
				font = temp;
		}
	}

	tr1::shared_ptr<AbstractFont> newFont(new SystemFont(font.release()));
	registry_.insert(make_pair(make_pair(familyName, properties), newFont));
	return newFont;
}

/// Returns the object implements @c IFontCollection interface.
const IFontCollection& layout::systemFonts() {
	static SystemFonts instance;
	return instance;
}


// LineLayout.Run ///////////////////////////////////////////////////////////

class LineLayout::Run {
	MANAH_UNASSIGNABLE_TAG(Run);
public:
	Run(length_t column, length_t length, const SCRIPT_ANALYSIS& script, tr1::shared_ptr<const RunStyle> style) /*throw()*/;
	virtual ~Run() /*throw()*/;
	uchar bidiEmbeddingLevel() const /*throw()*/;
	auto_ptr<Run> breakAt(DC& dc, length_t at, const String& lineString, const ILayoutInformationProvider& lip);	// 'at' is from the beginning of the line
	length_t column() const /*throw()*/ {return glyphs_->column + characterRange_.beginning();}
	HRESULT draw(DC& dc, int x, int y, bool restoreFont, const RECT* clipRect = 0, UINT options = 0) const;
	bool expandTabCharacters(const String& lineString, int x, int tabWidth, int maximumWidth);
	HRESULT hitTest(int x, int& cp, int& trailing) const;
	HRESULT justify(int width);
	length_t length() const /*throw()*/ {return characterRange_.length();}
	HRESULT logicalAttributes(const String& lineString, SCRIPT_LOGATTR attributes[]) const;
	HRESULT logicalWidths(int widths[]) const;
	int numberOfGlyphs() const /*throw()*/;
	ReadingDirection readingDirection() const /*throw()*/;
	bool overhangs() const /*throw()*/;
	HRESULT place(DC& dc, const String& lineString, const ILayoutInformationProvider& lip);
	tr1::shared_ptr<const RunStyle> requestedStyle() const /*throw()*/ {return style_;}
	void shape(DC& dc, const String& lineString, const ILayoutInformationProvider& lip, Run* previousRun);
	auto_ptr<Run> splitIfTooLong(const String& lineString);
	int totalWidth() const /*throw()*/;
	int x(size_t offset, bool trailing) const;
protected:
	struct GlyphData {
		const length_t column;	// first character (distance from the beginning of the line) for this glyph arrays
		SCRIPT_CACHE cache;
		// only 'clusters' is character-base. others are glyph-base
		manah::AutoBuffer<WORD> indices, clusters;
		manah::AutoBuffer<SCRIPT_VISATTR> visualAttributes;
		manah::AutoBuffer<int> advances, justifiedAdvances;
		manah::AutoBuffer<GOFFSET> offsets;
		explicit GlyphData(length_t column) /*throw()*/ : column(column), cache(0) {}
		~GlyphData() /*throw()*/ {::ScriptFreeCache(&cache);}
	};
protected:
	Run(Run& leading, length_t characterBoundary, int glyphBoundary) /*throw()*/;
	const int* advances() const /*throw()*/ {return glyphs_->advances.get() + glyphRange_.beginning();}
	const WORD* clusters() const /*throw()*/ {return glyphs_->clusters.get() + characterRange_.beginning();}
	const WORD* glyphs() const /*throw()*/ {return glyphs_->indices.get() + glyphRange_.beginning();}
	const GOFFSET* glyphOffsets() const /*throw()*/ {return glyphs_->offsets.get() + glyphRange_.beginning();}
	const int* justifiedAdvances() const /*throw()*/ {return glyphs_->justifiedAdvances.get() + glyphRange_.beginning();}
	const SCRIPT_VISATTR* visualAttributes() const /*throw()*/ {return glyphs_->visualAttributes.get() + glyphRange_.beginning();}
private:
	HRESULT buildGlyphs(const DC& dc, const Char* text) /*throw()*/;
	pair<int, HRESULT> countMissingGlyphs(const DC& dc) const /*throw()*/;
	HRESULT generateGlyphs(const DC& dc, const Char* text, int* numberOfMissingGlyphs);
	void generateDefaultGlyphs(const DC& dc);
private:
	Range<length_t> characterRange_;	// character range in 'glyphs_'
	SCRIPT_ANALYSIS analysis_;
	tr1::shared_ptr<GlyphData> glyphs_;	// glyph range in 'glyphs_'
	Range<int> glyphRange_;
	tr1::shared_ptr<const RunStyle> style_;
	tr1::shared_ptr<const AbstractFont> font_;
	ABC width_;
};

LineLayout::Run::Run(length_t column, length_t length, const SCRIPT_ANALYSIS& script, tr1::shared_ptr<const RunStyle> style) /*throw()*/
		: characterRange_(0, length), analysis_(script), glyphs_(new GlyphData(column)), style_(style) {
}

LineLayout::Run::Run(Run& leading, length_t characterBoundary, int glyphBoundary) /*throw()*/ :
		characterRange_(characterBoundary, leading.characterRange_.end()), analysis_(leading.analysis_),
		glyphs_(leading.glyphs_), glyphRange_(
			(leading.readingDirection() == LEFT_TO_RIGHT) ? glyphBoundary : leading.glyphRange_.beginning(),
			(leading.readingDirection() == LEFT_TO_RIGHT) ? leading.glyphRange_.end() : glyphBoundary),
		font_(leading.font_) {
	if(leading.glyphs_.get() == 0)
		throw invalid_argument("leading");
	if(characterBoundary >= leading.length())
		throw invalid_argument("firstCharacter");
	if(glyphBoundary >= leading.numberOfGlyphs())
		throw invalid_argument("firstGlyph");

	// modify 'leading's ranges
	const bool ltr = leading.readingDirection() == LEFT_TO_RIGHT;
	leading.characterRange_ = Range<length_t>(leading.characterRange_.beginning(), characterBoundary);
	if(ltr)
		leading.glyphRange_ = Range<int>(leading.glyphRange_.beginning(), glyphBoundary);
	else
		leading.glyphRange_ = Range<int>(glyphBoundary, leading.glyphRange_.end());

	// modify clusters
	Run& target = ltr ? *this : leading;
	WORD* const clusters = glyphs_->clusters.get();
	transform(target.clusters(), target.clusters() + target.length(), clusters + target.characterRange_.beginning(),
		bind2nd(minus<WORD>(), clusters[ltr ? target.characterRange_.beginning() : (target.characterRange_.end() - 1)]));
}

LineLayout::Run::~Run() /*throw()*/ {
//	if(cache_ != 0)
//		::ScriptFreeCache(&cache_);
}

inline uchar LineLayout::Run::bidiEmbeddingLevel() const /*throw()*/ {
	return static_cast<uchar>(analysis_.s.uBidiLevel);
}

auto_ptr<LineLayout::Run> LineLayout::Run::breakAt(DC& dc, length_t at, const String& lineString, const ILayoutInformationProvider& lip) {
	assert(at > column() && at < column() + length());
	assert(glyphs_->clusters[at - column()] != glyphs_->clusters[at - column() - 1]);

	const bool ltr = readingDirection() == LEFT_TO_RIGHT;
	const length_t newLength = at - column();
	const int newNumberOfGlyphs = ltr ? glyphs_->clusters[newLength] : (numberOfGlyphs() - glyphs_->clusters[newLength - 1]);
	assert(ltr == (analysis_.fRTL == 0));

	// create the new following run
	auto_ptr<Run> following(new Run(*this, newLength, ltr ? newNumberOfGlyphs : (numberOfGlyphs() - newNumberOfGlyphs)));

	// update placements
	place(dc, lineString, lip);
	following->place(dc, lineString, lip);

	return following;
}

/**
 * Builds glyphs into the run structure.
 * @param dc the device context
 * @param text the text to generate glyphs
 * @retval S_OK succeeded
 * @retval USP_E_SCRIPT_NOT_IN_FONT the font does not support the required script
 * @retval E_INVALIDARG other Uniscribe error. usually, too long run was specified
 * @retval HRESULT other Uniscribe error
 * @throw std#bad_alloc failed to allocate buffer for glyph indices or visual attributes array
 */
HRESULT LineLayout::Run::buildGlyphs(const DC& dc, const Char* text) /*throw()*/ {
	assert(glyphs_->advances.get() == 0);

#ifdef _DEBUG
	if(HFONT currentFont = dc.getCurrentFont()) {
		LOGFONTW lf;
		if(::GetObjectW(currentFont, sizeof(LOGFONTW), &lf) > 0) {
			DumpContext dout;
			dout << L"[LineLayout.Run.buildGlyphs] Selected font is '" << lf.lfFaceName << L"'.\n";
		}
	}
#endif

	manah::AutoBuffer<WORD> indices, clusters;
	manah::AutoBuffer<SCRIPT_VISATTR> visualAttributes;
	clusters.reset(new WORD[length()]);
	int numberOfGlyphs = estimateNumberOfGlyphs(length());
	HRESULT hr;
	while(true) {
		indices.reset(new WORD[numberOfGlyphs]);
		visualAttributes.reset(new SCRIPT_VISATTR[numberOfGlyphs]);
		hr = ::ScriptShape(dc.get(), &glyphs_->cache, text, static_cast<int>(length()),
			numberOfGlyphs, const_cast<SCRIPT_ANALYSIS*>(&analysis_),
			indices.get(), clusters.get(), visualAttributes.get(), &numberOfGlyphs);
		if(hr != E_OUTOFMEMORY)
			break;
		// repeat until a large enough buffer is provided
		numberOfGlyphs *= 2;
	}

	if(analysis_.fNoGlyphIndex != 0)
		return GDI_ERROR;	// the caller should try other fonts or disable shaping

	// commit
	if(SUCCEEDED(hr)) {
		swap(glyphs_->indices, indices);
		swap(glyphs_->clusters, clusters);
		swap(glyphs_->visualAttributes, visualAttributes);
		glyphRange_ = Range<int>(0, numberOfGlyphs);
	}
	return hr;
}

/**
 * Returns the number of missing glyphs in this run.
 * @param dc the device context
 * @param run the run
 * @return the number of missing glyphs
 */
inline pair<int, HRESULT> LineLayout::Run::countMissingGlyphs(const DC& dc) const /*throw()*/ {
	SCRIPT_FONTPROPERTIES fp;
	fp.cBytes = sizeof(SCRIPT_FONTPROPERTIES);
	const HRESULT hr = ::ScriptGetFontProperties(dc.get(), &glyphs_->cache, &fp);
	if(FAILED(hr))
		return make_pair(0, hr);	// can't handle
	// following is not offical way, but from Mozilla (gfxWindowsFonts.cpp)
	int result = 0;
	for(int i = glyphRange_.beginning(); i < glyphRange_.end(); ++i) {
		const WORD glyph = glyphs_->indices[i];
		if(glyph == fp.wgDefault || (glyph == fp.wgInvalid && glyph != fp.wgBlank))
			++result;
		else if(glyphs_->visualAttributes[i].fZeroWidth == 1 && scriptProperties.get(analysis_.eScript).fComplex == 0)
			++result;
	}
	return make_pair(result, S_OK);
}

/**
 * @param dc the device context
 * @param x
 * @param y
 * @param restoreFont set @c true to 
 * @param clipRect
 * @param options
 * @return
 */
inline HRESULT LineLayout::Run::draw(DC& dc, int x, int y, bool restoreFont, const RECT* clipRect /* = 0 */, UINT options /* = 0 */) const {
	HFONT oldFont = dc.selectObject(font_->handle().get());
	const HRESULT hr = ::ScriptTextOut(dc.use(), &glyphs_->cache, x, y, 0, clipRect,
		&analysis_, 0, 0, glyphs(), numberOfGlyphs(), advances(), justifiedAdvances(), glyphOffsets());
	if(restoreFont)
		dc.selectObject(oldFont);
	return hr;
}

/**
 * Expands tab characters in this run and modifies the width.
 * @param lineString the whole line string this run belongs to
 * @param x the position in writing direction this run begins, in pixels
 * @param tabWidth the maximum expanded tab width, in pixels
 * @param maximumWidth the maximum width this run can take place, in pixels
 * @return @c true if expanded tab characters
 * @throw std#invalid_argument @a maximumWidth &lt;= 0
 */
inline bool LineLayout::Run::expandTabCharacters(const String& lineString, int x, int tabWidth, int maximumWidth) {
	if(maximumWidth <= 0)
		throw invalid_argument("maximumWidth");
	if(lineString[column()] != L'\t')
		return false;
	assert(length() == 1);
	width_.abcB = min(tabWidth - x % tabWidth, maximumWidth);
	width_.abcA = width_.abcC = 0;
	assert(glyphs_.unique());
	glyphs_->advances[0] = totalWidth();
	glyphs_->justifiedAdvances.reset();
	return true;
}

/**
 * @param dc the device context
 * @param lineString the text to generate glyphs
 * @param run the run
 * @param[out] numberOfMissingGlyphs the number of the missing glyphs. can be @c null
 * @retval S_FALSE there are missing glyphs
 * @retval otherwise see @c buildGlyphs function
 */
inline HRESULT LineLayout::Run::generateGlyphs(const DC& dc, const Char* text, int* numberOfMissingGlyphs) {
	HRESULT hr = buildGlyphs(dc, text);
	if(SUCCEEDED(hr) && numberOfMissingGlyphs != 0 && 0 != (*numberOfMissingGlyphs = countMissingGlyphs(dc).first))
		hr = S_FALSE;
	else if(hr == USP_E_SCRIPT_NOT_IN_FONT && numberOfMissingGlyphs != 0)
		*numberOfMissingGlyphs = numberOfGlyphs();
	return hr;
}

/// Fills the glyph array with default index, instead of using @c ScriptShape.
inline void LineLayout::Run::generateDefaultGlyphs(const DC& dc) {
	assert(glyphs_->advances.get() == 0);

	SCRIPT_FONTPROPERTIES fp;
	fp.cBytes = sizeof(SCRIPT_FONTPROPERTIES);
	if(FAILED(::ScriptGetFontProperties(dc.get(), &glyphs_->cache, &fp)))
		fp.wgDefault = 0;	// hmm...

	manah::AutoBuffer<WORD> indices, clusters;
	manah::AutoBuffer<SCRIPT_VISATTR> visualAttributes;
	const int numberOfGlyphs = static_cast<int>(length());
	indices.reset(new WORD[numberOfGlyphs]);
	clusters.reset(new WORD[length()]);
	visualAttributes.reset(new SCRIPT_VISATTR[numberOfGlyphs]);
	fill_n(indices.get(), numberOfGlyphs, fp.wgDefault);
	const bool ltr = analysis_.fRTL == 0 || analysis_.fLogicalOrder == 1;
	for(size_t i = 0, c = length(); i < c; ++i)
		clusters[i] = static_cast<WORD>(ltr ? i : (c - i));
	const SCRIPT_VISATTR va = {SCRIPT_JUSTIFY_NONE, 1, 0, 0, 0, 0};
	fill_n(visualAttributes.get(), numberOfGlyphs, va);

	// commit
	swap(glyphs_->indices, indices);
	swap(glyphs_->clusters, clusters);
	swap(glyphs_->visualAttributes, visualAttributes);
	glyphRange_ = Range<int>(0, numberOfGlyphs);
}

inline HRESULT LineLayout::Run::hitTest(int x, int& cp, int& trailing) const {
	return ::ScriptXtoCP(x, static_cast<int>(length()), numberOfGlyphs(), clusters(),
		visualAttributes(), (justifiedAdvances() == 0) ? advances() : justifiedAdvances(), &analysis_, &cp, &trailing);
}

inline HRESULT LineLayout::Run::justify(int width) {
	assert(glyphs_.get() != 0 && advances() != 0);
	HRESULT hr = S_OK;
	if(width != totalWidth()) {
		glyphs_->justifiedAdvances.reset(new int[numberOfGlyphs()]);
		hr = ::ScriptJustify(visualAttributes(), advances(), numberOfGlyphs(),
			width - totalWidth(), 2, glyphs_->justifiedAdvances.get() + glyphRange_.beginning());
		width_.abcB += width - totalWidth();
	}
	return hr;
}

inline HRESULT LineLayout::Run::logicalAttributes(const String& lineString, SCRIPT_LOGATTR attributes[]) const {
	if(attributes == 0)
		throw NullPointerException("attributes");
	return ::ScriptBreak(lineString.data() + column(), static_cast<int>(length()), &analysis_, attributes);
}

inline HRESULT LineLayout::Run::logicalWidths(int widths[]) const {
	if(widths == 0)
		throw NullPointerException("widths");
	return ::ScriptGetLogicalWidths(&analysis_, static_cast<int>(length()),
		numberOfGlyphs(), advances(), clusters(), visualAttributes(), widths);
}

inline int LineLayout::Run::numberOfGlyphs() const /*throw()*/ {
	return glyphRange_.length();
}

inline bool LineLayout::Run::overhangs() const /*throw()*/ {
	return width_.abcA < 0 || width_.abcC < 0;
}

HRESULT LineLayout::Run::place(DC& dc, const String& lineString, const ILayoutInformationProvider& lip) {
	assert(glyphs_.get() != 0 && glyphs() != 0);
	// TODO: rewrite this method to be able to call multiple times.

	manah::AutoBuffer<int> advances(new int[numberOfGlyphs()]);
	manah::AutoBuffer<GOFFSET> offsets(new GOFFSET[numberOfGlyphs()]);
	ABC width(width_);
	HRESULT hr = ::ScriptPlace(0, &glyphs_->cache, glyphs(), numberOfGlyphs(),
		visualAttributes(), &analysis_, advances.get(), offsets.get(), &width);
	if(hr == E_PENDING) {
		dc.selectObject(font_->handle().get());
		hr = ::ScriptPlace(dc.get(), &glyphs_->cache, glyphs(), numberOfGlyphs(),
			visualAttributes(), &analysis_, advances.get(), offsets.get(), &width);
	}
	if(FAILED(hr))
		return hr;

	// query widths of C0 and C1 controls in this run
	manah::AutoBuffer<WORD> glyphIndices;
	if(ISpecialCharacterRenderer* scr = lip.specialCharacterRenderer()) {
		ISpecialCharacterRenderer::LayoutContext context(dc);
		context.readingDirection = readingDirection();
		dc.selectObject(font_->handle().get());
		SCRIPT_FONTPROPERTIES fp;
		fp.cBytes = 0;
		for(length_t i = column(), e = column() + length(); i < e; ++i) {
			if(isC0orC1Control(lineString[i])) {
				if(const int w = scr->getControlCharacterWidth(context, lineString[i])) {
					// substitute the glyph
					width.abcB += w - glyphs_->advances[i - column()];
					glyphs_->advances[i] = w;
					if(fp.cBytes == 0) {
						fp.cBytes = sizeof(SCRIPT_FONTPROPERTIES);
						const HRESULT hr2 = ::ScriptGetFontProperties(dc.get(), &glyphs_->cache, &fp);
						if(FAILED(hr2))
							fp.wgBlank = 0;	// hmm...
					}
					if(glyphIndices.get() == 0) {
						glyphIndices.reset(new WORD[numberOfGlyphs()]);
						memcpy(glyphIndices.get(), glyphs(), sizeof(WORD) * numberOfGlyphs());
					}
					glyphIndices[i] = fp.wgBlank;
				}
			}
		}
	}

	// handle letter spacing
	if(style_.get() != 0 && style_->letterSpacing.unit != Length::INHERIT) {
		const int letterSpacing = pixels(dc, style_->letterSpacing, false, font_->metrics());
		const bool rtl = readingDirection() == RIGHT_TO_LEFT;
		const SCRIPT_VISATTR* const va = visualAttributes();
		for(size_t i = glyphRange_.beginning(), e = numberOfGlyphs(); i < e; ++i) {
			if((!rtl && (i + 1 == e || va[i + 1].fClusterStart != 0))
					|| (rtl && (i == 0 || va[i - 1].fClusterStart != 0))) {
				advances[i] += letterSpacing;
				width.abcB += letterSpacing;
				if(rtl)
					offsets[i].du += letterSpacing;
			}
		}
	}

	// commit
	if(glyphs_.unique()) {
		swap(advances, glyphs_->advances);
		swap(offsets, glyphs_->offsets);
		if(glyphIndices.get() != 0)
			swap(glyphIndices, glyphs_->indices);
	} else {
		memcpy(glyphs_->advances.get() + glyphRange_.beginning(), advances.get(), sizeof(int) * numberOfGlyphs());
		memcpy(glyphs_->offsets.get() + glyphRange_.beginning(), offsets.get(), sizeof(GOFFSET) * numberOfGlyphs());
		if(glyphIndices.get() != 0)
			memcpy(glyphs_->indices.get() + glyphRange_.beginning(), glyphIndices.get(), sizeof(WORD) * numberOfGlyphs());
	}
	swap(width, width_);
	return hr;
}

inline ReadingDirection LineLayout::Run::readingDirection() const /*throw()*/ {
	return ((analysis_.s.uBidiLevel & 0x01) == 0x00) ? LEFT_TO_RIGHT : RIGHT_TO_LEFT;
}

// shaping stuffs
namespace {
	/**
	 * Returns a Unicode script corresponds to Win32 language identifier for digit substitution.
	 * @param id the language identifier
	 * @return the script or @c NOT_PROPERTY
	 */
	inline int convertWin32LangIDtoUnicodeScript(LANGID id) /*throw()*/ {
		switch(id) {
		case LANG_ARABIC:		return Script::ARABIC;
		case LANG_ASSAMESE:		return Script::BENGALI;
		case LANG_BENGALI:		return Script::BENGALI;
		case 0x5c:				return Script::CHEROKEE;
		case LANG_DIVEHI:		return Script::THAANA;
		case 0x5e:				return Script::ETHIOPIC;
		case LANG_FARSI:		return Script::ARABIC;	// Persian
		case LANG_GUJARATI:		return Script::GUJARATI;
		case LANG_HINDI:		return Script::DEVANAGARI;
		case LANG_KANNADA:		return Script::KANNADA;
		case 0x53:				return Script::KHMER;
		case 0x54:				return Script::LAO;
		case LANG_MALAYALAM:	return Script::MALAYALAM;
		case 0x55:				return Script::MYANMAR;
		case LANG_ORIYA:		return Script::ORIYA;
		case LANG_PUNJABI:		return Script::GURMUKHI;
		case 0x5b:				return Script::SINHALA;
		case LANG_SYRIAC:		return Script::SYRIAC;
		case LANG_TAMIL:		return Script::TAMIL;
		case 0x51:				return Script::TIBETAN;
		case LANG_TELUGU:		return Script::TELUGU;
		case LANG_THAI:			return Script::THAI;
		case LANG_URDU:			return Script::ARABIC;
		}
		return NOT_PROPERTY;
	}

#ifdef ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_LEGACY_WORKAROUND
	const OPENTYPE_TAG HANI_TAG = makeTrueTypeTag("hani");
	const OPENTYPE_TAG JP78_TAG = makeTrueTypeTag("jp78");
	const OPENTYPE_TAG JP90_TAG = makeTrueTypeTag("jp90");
	const OPENTYPE_TAG JP04_TAG = makeTrueTypeTag("jp04");
	struct IVStoOTFT {
		ulong ivs;	// (base character) << 8 | (variation selector number)
		OPENTYPE_TAG featureTag;
	};
	const IVStoOTFT IVS_TO_OTFT[] = {
#include "generated/ivs-otft.ipp"
	};
	struct GetIVS {
		ulong operator()(size_t index) /*throw()*/ {return IVS_TO_OTFT[index].ivs;}
	};
#endif // ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_LEGACY_WORKAROUND
} // namespace @0

void LineLayout::Run::shape(DC& dc, const String& lineString, const ILayoutInformationProvider& lip, Run* previousRun) {
	if(glyphs_->clusters.get() != 0)
		throw IllegalStateException("");
	if(requestedStyle().get() != 0) {
		if(!requestedStyle()->shapingEnabled)
			analysis_.eScript = SCRIPT_UNDEFINED;
	} else {
		tr1::shared_ptr<const RunStyle> defaultStyle(lip.presentation().defaultTextRunStyle());
		if(defaultStyle.get() != 0 && !defaultStyle->shapingEnabled)
			analysis_.eScript = SCRIPT_UNDEFINED;
	}

	HRESULT hr;
	const WORD originalScript = analysis_.eScript;
	HFONT oldFont;

	// compute font properties
	String computedFontFamily((requestedStyle().get() != 0) ? requestedStyle()->fontFamily : String(L"\x5c0f\x585a\x660e\x671d Pr6N R"));
	FontProperties computedFontProperties((requestedStyle().get() != 0) ? requestedStyle()->fontProperties : FontProperties());
	double computedFontSizeAdjust = (requestedStyle().get() != 0) ? requestedStyle()->fontSizeAdjust : -1.0;
	tr1::shared_ptr<const RunStyle> defaultStyle(lip.presentation().defaultTextRunStyle());
	if(computedFontFamily.empty()) {
		if(defaultStyle.get() != 0)
			computedFontFamily = lip.presentation().defaultTextRunStyle()->fontFamily;
		if(computedFontFamily.empty())
			computedFontFamily = lip.textMetrics().familyName();
	}
	if(computedFontProperties.weight == FontProperties::INHERIT_WEIGHT)
		computedFontProperties.weight = (defaultStyle.get() != 0) ? defaultStyle->fontProperties.weight : FontProperties::NORMAL_WEIGHT;
	if(computedFontProperties.stretch == FontProperties::INHERIT_STRETCH)
		computedFontProperties.stretch = (defaultStyle.get() != 0) ? defaultStyle->fontProperties.stretch : FontProperties::NORMAL_STRETCH;
	if(computedFontProperties.style == FontProperties::INHERIT_STYLE)
		computedFontProperties.style = (defaultStyle.get() != 0) ? defaultStyle->fontProperties.style : FontProperties::NORMAL_STYLE;
	if(computedFontProperties.size == 0.0f) {
		if(defaultStyle.get() != 0)
			computedFontProperties.size = defaultStyle->fontProperties.size;
		if(computedFontProperties.size == 0.0f)
			computedFontProperties.size = lip.textMetrics().emHeight();
	}
	if(computedFontSizeAdjust < 0.0)
		computedFontSizeAdjust = (defaultStyle.get() != 0) ? defaultStyle->fontSizeAdjust : 0.0;

	if(analysis_.s.fDisplayZWG != 0 && scriptProperties.get(analysis_.eScript).fControl != 0) {
		// bidirectional format controls
		FontProperties fp;
		fp.size = computedFontProperties.size;
		tr1::shared_ptr<const AbstractFont> font(lip.fontCollection().get(L"Arial", fp, computedFontSizeAdjust));
		oldFont = dc.selectObject((font_ = font)->handle().get());
		if(USP_E_SCRIPT_NOT_IN_FONT == (hr = buildGlyphs(dc, lineString.data()))) {
			analysis_.eScript = SCRIPT_UNDEFINED;
			hr = buildGlyphs(dc, lineString.data());
		}
		if(FAILED(hr))
			generateDefaultGlyphs(dc);
		for(int i = 0; i < numberOfGlyphs(); ++i)
			glyphs_->visualAttributes[i].fZeroWidth = 1;
		dc.selectObject(oldFont);
	}

	else {
		// buildGlyphs() returns glyphs for the run. however it can contain missing glyphs.
		// we try candidate fonts in following order:
		//
		// 1. the primary font
		// 2. the national font for digit substitution
		// 3. the linked fonts
		// 4. the fallback font
		//
		// with storing the fonts failed to shape ("failedFonts"). and then retry the failed
		// fonts returned USP_E_SCRIPT_NOT_IN_FONT with shaping)

		int script = NOT_PROPERTY;	// script of the run for fallback
		typedef vector<pair<tr1::shared_ptr<const AbstractFont>, int> > FailedFonts;
		FailedFonts failedFonts;	// failed fonts (font handle vs. # of missings)
		int numberOfMissingGlyphs;

		const Char* textString = lineString.data() + column();

#define ASCENSION_MAKE_TEXT_STRING_SAFE()												\
	assert(safeString.get() == 0);														\
	safeString.reset(new Char[length()]);												\
	wmemcpy(safeString.get(), lineString.data() + column(), length());					\
	replace_if(safeString.get(),														\
		safeString.get() + length(), surrogates::isSurrogate, REPLACEMENT_CHARACTER);	\
	textString = safeString.get();

#define ASCENSION_CHECK_FAILED_FONTS()										\
	bool skip = false;														\
	for(FailedFonts::const_iterator											\
			i(failedFonts.begin()), e(failedFonts.end()); i != e; ++i) {	\
		if(i->first == font_) {												\
			skip = true;													\
			break;															\
		}																	\
	}

		// ScriptShape may crash if the shaping is disabled (see Mozilla bug 341500).
		// Following technique is also from Mozilla (gfxWindowsFonts.cpp).
		manah::AutoBuffer<Char> safeString;
		if(analysis_.eScript == SCRIPT_UNDEFINED
				&& find_if(textString, textString + length(), surrogates::isSurrogate) != textString + length()) {
			ASCENSION_MAKE_TEXT_STRING_SAFE();
		}

		// 1. the primary font
		oldFont = dc.selectObject((font_ = lip.fontCollection().get(computedFontFamily, computedFontProperties))->handle().get());
		hr = generateGlyphs(dc, textString, &numberOfMissingGlyphs);
		if(hr != S_OK) {
			::ScriptFreeCache(&glyphs_->cache);
			failedFonts.push_back(make_pair(font_, (hr == S_FALSE) ? numberOfMissingGlyphs : numeric_limits<int>::max()));
		}

		// 2. the national font for digit substitution
		if(hr == USP_E_SCRIPT_NOT_IN_FONT && analysis_.eScript != SCRIPT_UNDEFINED && analysis_.s.fDigitSubstitute != 0) {
			script = convertWin32LangIDtoUnicodeScript(scriptProperties.get(analysis_.eScript).langid);
			if(script != NOT_PROPERTY) {
				const basic_string<WCHAR> fallbackFontFamily(fallback(script));
				if(!fallbackFontFamily.empty()) {
					font_ = lip.fontCollection().get(fallbackFontFamily, computedFontProperties, computedFontSizeAdjust);
					ASCENSION_CHECK_FAILED_FONTS()
					if(!skip) {
						dc.selectObject(font_->handle().get());
						hr = generateGlyphs(dc, textString, &numberOfMissingGlyphs);
						if(hr != S_OK) {
							::ScriptFreeCache(&glyphs_->cache);
							failedFonts.push_back(make_pair(font_, numberOfMissingGlyphs));	// not material what hr is...
						}
					}
				}
			}
		}
/*
		// 3. the linked fonts
		if(hr != S_OK) {
			for(size_t i = 0; i < lip.getFontSelector().numberOfLinkedFonts(); ++i) {
				font_ = lip.getFontSelector().linkedFont(i, run->style.bold, run->style.italic);
				ASCENSION_CHECK_FAILED_FONTS()
				if(!skip) {
					dc.selectObject(font_);
					if(S_OK == (hr = generateGlyphs(dc, textString, &numberOfMissingGlyphs)))
						break;
					::ScriptFreeCache(&cache_);
					failedFonts.push_back(make_pair(font_, (hr == S_FALSE) ? numberOfMissingGlyphs : numeric_limits<int>::max()));
				}
			}
		}
*/
		// 4. the fallback font
		if(hr != S_OK) {
			for(StringCharacterIterator i(textString, textString + length()); i.hasNext(); i.next()) {
				script = Script::of(i.current());
				if(script != Script::UNKNOWN && script != Script::COMMON && script != Script::INHERITED)
					break;
			}
			if(script != Script::UNKNOWN && script != Script::COMMON && script != Script::INHERITED) {
				const basic_string<WCHAR> fallbackFontFamily(fallback(script));
				if(!fallbackFontFamily.empty())
					font_ = lip.fontCollection().get(fallbackFontFamily, computedFontProperties, computedFontSizeAdjust);
			} else {
				font_.reset();
				// ambiguous CJK?
				if(script == Script::COMMON && scriptProperties.get(analysis_.eScript).fAmbiguousCharSet != 0) {
					// TODO: this code check only the first character in the run?
					switch(Block::of(surrogates::decodeFirst(textString, textString + length()))) {
					case Block::CJK_SYMBOLS_AND_PUNCTUATION:
					case Block::ENCLOSED_CJK_LETTERS_AND_MONTHS:
					case Block::CJK_COMPATIBILITY:
					case Block::VERTICAL_FORMS:	// as of GB 18030
					case Block::CJK_COMPATIBILITY_FORMS:
					case Block::SMALL_FORM_VARIANTS:	// as of CNS-11643
					case Block::HALFWIDTH_AND_FULLWIDTH_FORMS: {
						const basic_string<WCHAR> fallbackFontFamily(fallback(Script::HAN));
						if(!fallbackFontFamily.empty())
							font_ = lip.fontCollection().get(fallbackFontFamily, computedFontProperties, computedFontSizeAdjust);
						break;
						}
					}
				}
				if(font_ == 0 && previousRun != 0) {
					// use the previous run setting (but this will copy the style of the font...)
					analysis_.eScript = previousRun->analysis_.eScript;
					font_ = previousRun->font_;
				}
			}
			if(font_ != 0) {
				ASCENSION_CHECK_FAILED_FONTS()
				if(!skip) {
					dc.selectObject(font_->handle().get());
					hr = generateGlyphs(dc, textString, &numberOfMissingGlyphs);
					if(hr != S_OK) {
						::ScriptFreeCache(&glyphs_->cache);
						failedFonts.push_back(make_pair(font_, (hr == S_FALSE) ? numberOfMissingGlyphs : numeric_limits<int>::max()));
					}
				}
			}
		}

		// retry without shaping
		if(hr != S_OK) {
			if(analysis_.eScript != SCRIPT_UNDEFINED) {
				const WORD oldScript = analysis_.eScript;
				analysis_.eScript = SCRIPT_UNDEFINED;	// disable shaping
				if(find_if(textString, textString + length(), surrogates::isSurrogate) != textString + length()) {
					ASCENSION_MAKE_TEXT_STRING_SAFE();
				}
				for(FailedFonts::iterator i(failedFonts.begin()), e(failedFonts.end()); i != e; ++i) {
					if(i->second == numeric_limits<int>::max()) {
						font_.reset();
						dc.selectObject((font_ = i->first)->handle().get());
						hr = generateGlyphs(dc, textString, &numberOfMissingGlyphs);
						if(hr == S_OK)
							break;	// found the best
						::ScriptFreeCache(&glyphs_->cache);
						if(hr == S_FALSE)
							i->second = -numberOfMissingGlyphs;
					}
				}
				analysis_.eScript = oldScript;
			}
			if(hr != S_OK) {
				// select the font which generated the least missing glyphs
				assert(!failedFonts.empty());
				FailedFonts::const_iterator bestFont(failedFonts.begin());
				for(FailedFonts::const_iterator i(bestFont + 1), e(failedFonts.end()); i != e; ++i) {
					if(i->second < bestFont->second)
						bestFont = i;
				}
				dc.selectObject((font_ = bestFont->first)->handle().get());
				if(bestFont->second < 0)
					analysis_.eScript = SCRIPT_UNDEFINED;
				hr = generateGlyphs(dc, textString, 0);
				if(FAILED(hr)) {
					::ScriptFreeCache(&glyphs_->cache);
					generateDefaultGlyphs(dc);	// worst case...
				}
			}
		}

#undef ASCENSION_MAKE_TEXT_STRING_SAFE
#undef ASCENSION_CHECK_FAILED_FONTS
	}

#if defined(ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND) || defined(ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_LEGACY_WORKAROUND)
	if(!uniscribeSupportsIVS()
#ifdef ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_LEGACY_WORKAROUND
			&& supportsOpenTypeFeatures()
#endif // ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_LEGACY_WORKAROUND
			) {
		if(previousRun != 0 && length() > 1) {
			const CodePoint variationSelector = surrogates::decodeFirst(lineString.begin() + column(), lineString.begin() + column() + 2);
			if(variationSelector >= 0xe0100ul && variationSelector <= 0xe01eful) {
				const CodePoint baseCharacter = surrogates::decodeLast(
					lineString.data() + previousRun->column(), lineString.data() + previousRun->column() + previousRun->length());
				const WORD originalGlyph = previousRun->glyphs_->indices[0];

				pair<bool, uint16_t> result;
#ifdef ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
				result = static_cast<const SystemFont*>(font_.get())->ivsGlyph(baseCharacter, variationSelector);
#elif ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_LEGACY_WORKAROUND
				result.first = false;
				const size_t i = ascension::internal::searchBound(
					0u, MANAH_COUNTOF(IVS_TO_OTFT), (baseCharacter << 8) | (vs - 0xe0100ul), GetIVS());
				if(i != MANAH_COUNTOF(IVS_TO_OTFT) && IVS_TO_OTFT[i].ivs == ((baseCharacter << 8) | (vs - 0xe0100ul))) {
					// found valid IVS -> apply OpenType feature tag to obtain the variant
					// note that this glyph alternation is not effective unless the script in run->analysis is 'hani'
					if(SUCCEEDED(hr = uspLib->get<0>()(dc.get(), &previousRun->glyphs_->cache, &previousRun->analysis_,
							HANI_TAG, 0, IVS_TO_OTFT[i].featureTag, 1, originalGlyph, &result.second)))
						result.first = true;
				}
#endif // ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND

				if(result.first) {
					previousRun->glyphs_->indices[0] = result.second;
					// last character in the previous run and first character in this run are an IVS
					// => so, remove glyphs correspond to the first character
					WORD blankGlyph;
					if(S_OK != (hr = ::ScriptGetCMap(dc.get(), &glyphs_->cache, L"\x0020", 1, 0, &blankGlyph))) {
						SCRIPT_FONTPROPERTIES fp;
						fp.cBytes = sizeof(SCRIPT_FONTPROPERTIES);
						if(FAILED(::ScriptGetFontProperties(dc.get(), &glyphs_->cache, &fp)))
							fp.wgBlank = 0;	// hmm...
						blankGlyph = fp.wgBlank;
					}
					glyphs_->indices[glyphs_->clusters[0]] = glyphs_->indices[glyphs_->clusters[1]] = blankGlyph;
					SCRIPT_VISATTR* const va = glyphs_->visualAttributes.get();
					va[glyphs_->clusters[0]].uJustification = va[glyphs_->clusters[1]].uJustification = SCRIPT_JUSTIFY_BLANK;
					va[glyphs_->clusters[0]].fZeroWidth = va[glyphs_->clusters[1]].fZeroWidth = 1;
				}
			}
		}
	}
#endif // ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_*_WORKAROUND
}

auto_ptr<LineLayout::Run> LineLayout::Run::splitIfTooLong(const String& lineString) {
	if(estimateNumberOfGlyphs(length()) <= 65535)
		return auto_ptr<Run>();

	// split this run, because the length would cause ScriptShape to fail (see also Mozilla bug 366643).
	static const length_t MAXIMUM_RUN_LENGTH = 43680;	// estimateNumberOfGlyphs(43680) == 65536
	length_t opportunity = 0;
	manah::AutoBuffer<SCRIPT_LOGATTR> la(new SCRIPT_LOGATTR[length()]);
	const HRESULT hr = logicalAttributes(lineString, la.get());
	if(SUCCEEDED(hr)) {
		for(length_t i = MAXIMUM_RUN_LENGTH; i > 0; --i) {
			if(la[i].fCharStop != 0) {
				if(legacyctype::isspace(lineString[i]) || legacyctype::isspace(lineString[i - 1])) {
					opportunity = i;
					break;
				}
				opportunity = max(i, opportunity);
			}
		}
	}
	if(opportunity == 0) {
		opportunity = MAXIMUM_RUN_LENGTH;
		if(surrogates::isLowSurrogate(lineString[opportunity]) && surrogates::isHighSurrogate(lineString[opportunity - 1]))
			--opportunity;
	}

	auto_ptr<Run> following(new Run(opportunity, length() - opportunity, analysis_, style_));
	characterRange_ = Range<length_t>(0, opportunity);
	return following;
}

inline int LineLayout::Run::totalWidth() const /*throw()*/ {
	return width_.abcA + width_.abcB + width_.abcC;
}

inline int LineLayout::Run::x(size_t offset, bool trailing) const {
	int result;
	const HRESULT hr = ::ScriptCPtoX(static_cast<int>(offset), trailing,
		static_cast<int>(length()), numberOfGlyphs(), clusters(), visualAttributes(),
		((justifiedAdvances() == 0) ? advances() : justifiedAdvances()), &analysis_, &result);
	if(FAILED(hr))
		throw hr;
	// TODO: handle letter-spacing correctly.
//	if(visualAttributes()[offset].fClusterStart == 0) {
//	}
	return result;
}


// LineLayout ///////////////////////////////////////////////////////////////

// helpers for LineLayout.draw
namespace {
	const size_t MAXIMUM_RUN_LENGTH = 1024;
	inline HPEN createPen(COLORREF color, int width, int style) {
		LOGBRUSH brush;
		brush.lbColor = color;
		brush.lbStyle = BS_SOLID;
		switch(style) {
		case 1:	// solid
			return (width == 1) ? ::CreatePen(PS_SOLID, 1, color) : ::ExtCreatePen(PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_FLAT, width, &brush, 0, 0);
		case 2:	// dashed
			return ::ExtCreatePen(PS_GEOMETRIC | PS_DASH | PS_ENDCAP_FLAT, width, &brush, 0, 0);
		case 3:	// dotted
			return ::ExtCreatePen(PS_GEOMETRIC | PS_DOT | PS_ENDCAP_FLAT, width, &brush, 0, 0);
		}
		throw UnknownValueException("style");
	}
	inline void drawDecorationLines(DC& dc, const RunStyle& style, COLORREF foregroundColor, int x, int y, int width, int height) {
		if(style.decorations.underline.style != Decorations::NONE || style.decorations.strikethrough.style != Decorations::NONE) {
			int baselineOffset, underlineOffset, underlineThickness, linethroughOffset, linethroughThickness;
			if(getDecorationLineMetrics(dc.get(), &baselineOffset, &underlineOffset, &underlineThickness, &linethroughOffset, &linethroughThickness)) {
				// draw underline
				if(style.decorations.underline.style != Decorations::NONE) {
					HPEN oldPen = dc.selectObject(createPen((style.decorations.underline.color != Color()) ?
						style.decorations.underline.color.asCOLORREF() : foregroundColor, underlineThickness, style.decorations.underline.style));
					const int underlineY = y + baselineOffset - underlineOffset + underlineThickness / 2;
					dc.moveTo(x, underlineY);
					dc.lineTo(x + width, underlineY);
					::DeleteObject(dc.selectObject(oldPen));
				}
				// draw strikethrough line
				if(style.decorations.strikethrough.style != Decorations::NONE) {
					HPEN oldPen = dc.selectObject(createPen((style.decorations.strikethrough.color != Color()) ?
						style.decorations.strikethrough.color.asCOLORREF() : foregroundColor, linethroughThickness, 1));
					const int strikeoutY = y + baselineOffset - linethroughOffset + linethroughThickness / 2;
					dc.moveTo(x, strikeoutY);
					dc.lineTo(x + width, strikeoutY);
					::DeleteObject(dc.selectObject(oldPen));
				}
			}
		}
	}
	inline void drawBorder(DC& dc, const Border& style, const IFontMetrics& fontMetrics,
			const Color& currentColor, int start, int before, int end, int after) {
		// TODO: rewrite later.
		const Border::Part* const styles[] = {&style.before, &style.after, &style.start, &style.end};
		const POINT points[4][2] = {
			{{start, before}, {end, before}},
			{{start, after}, {end, after}},
			{{start, before}, {start, after}},
			{{end, before}, {end, after}}
		};
		for(size_t i = 0; i < MANAH_COUNTOF(styles); ++i) {
			if(styles[i]->style != Border::NONE && styles[i]->style != Border::HIDDEN) {
				const int width = pixels(dc,  styles[i]->width, true, fontMetrics);
				if(width != 0) {
					HPEN oldPen = dc.selectObject(createPen(
						((styles[i]->color != Color()) ? styles[i]->color : currentColor).asCOLORREF(), width, styles[i]->style));
					dc.moveTo(points[i][0].x, points[i][0].y);
					dc.lineTo(points[i][1].x, points[i][1].y);
					dc.selectObject(oldPen);
				}
			}
		}
	}
} // namespace @0

/**
 * @class ascension::layout::LineLayout
 * @c LineLayout represents a layout of styled line text. Provides support for drawing, cursor
 * navigation, hit testing, text wrapping, etc.
 *
 * <del>A long run will be split into smaller runs automatically because Uniscribe rejects too long
 * text (especially @c ScriptShape and @c ScriptTextOut). For this reason, a combining character
 * will be rendered incorrectly if it is presented at the boundary. The maximum length of a run is
 * 1024.
 *
 * In present, this class supports only text layout horizontal against the output device.
 *
 * @note This class is not intended to derive.
 * @see LineLayoutBuffer#lineLayout, LineLayoutBuffer#lineLayoutIfCached
 */

/**
 * Constructor.
 * @param dc the device context
 * @param layoutInformation the layout information
 * @param line the line
 * @throw kernel#BadPositionException @a line is invalid
 */
LineLayout::LineLayout(DC& dc, const ILayoutInformationProvider& layoutInformation,
		length_t line) : lip_(layoutInformation), lineNumber_(line), style_(layoutInformation.presentation().lineStyle(line)),
		runs_(0), numberOfRuns_(0), sublineOffsets_(0), sublineFirstRuns_(0), numberOfSublines_(0), longestSublineWidth_(-1), wrapWidth_(-1) {
	assert(style_.get() != 0);
	// calculate the wrapping width
	if(layoutInformation.layoutSettings().lineWrap.wraps()) {
		wrapWidth_ = layoutInformation.width();
		if(ISpecialCharacterRenderer* scr = layoutInformation.specialCharacterRenderer()) {
			ISpecialCharacterRenderer::LayoutContext context(dc);
			context.readingDirection = readingDirection();
			wrapWidth_ -= scr->getLineWrappingMarkWidth(context);
		}
	}
	// construct the layout
	if(!text().empty()) {
		itemize(line);
		shape(dc);
		if(numberOfRuns_ == 0 || wrapWidth_ == -1) {
			numberOfSublines_ = 1;
			sublineFirstRuns_ = new size_t[1];
			sublineFirstRuns_[0] = 0;
			reorder();
			expandTabsWithoutWrapping();
		} else {
			wrap(dc);
			reorder();
			if(style_->alignment == JUSTIFY)
				justify();
		}
	} else {	// an empty line
		numberOfRuns_ = 0;
		numberOfSublines_ = 1;
		longestSublineWidth_ = 0;
	}
}

/// Destructor.
LineLayout::~LineLayout() /*throw()*/ {
	dispose();
}

/**
 * Returns the computed text alignment of the line. The returned value may be
 * @c presentation#ALIGN_START or @c presentation#ALIGN_END.
 * @see #readingDirection, presentation#resolveTextAlignment
 */
TextAlignment LineLayout::alignment() const /*throw()*/ {
	if(style_.get() != 0 && style_->readingDirection != INHERIT_TEXT_ALIGNMENT)
		style_->readingDirection;
	tr1::shared_ptr<const LineStyle> defaultStyle(lip_.presentation().defaultLineStyle());
	return (defaultStyle.get() != 0
		&& defaultStyle->alignment != INHERIT_TEXT_ALIGNMENT) ? defaultStyle->alignment : ASCENSION_DEFAULT_TEXT_ALIGNMENT;
}

/**
 * Returns the bidirectional embedding level at specified position.
 * @param column the column
 * @return the embedding level
 * @throw kernel#BadPositionException @a column is greater than the length of the line
 */
ascension::byte LineLayout::bidiEmbeddingLevel(length_t column) const {
	if(numberOfRuns_ == 0) {
		if(column != 0)
			throw kernel::BadPositionException(kernel::Position(lineNumber_, column));
		// use the default level
		return (readingDirection() == RIGHT_TO_LEFT) ? 1 : 0;
	}
	const size_t i = findRunForPosition(column);
	if(i == numberOfRuns_)
		throw kernel::BadPositionException(kernel::Position(lineNumber_, column));
	return runs_[i]->bidiEmbeddingLevel();
}

/**
 * Returns the black box bounds of the characters in the specified range. The black box bounds is
 * an area consisting of the union of the bounding boxes of the all of the characters in the range.
 * The result region can be disjoint.
 * @param first the start of the range
 * @param last the end of the range
 * @return the Win32 GDI region object encompasses the black box bounds. the coordinates are based
 *         on the left-top of the first visual subline in the layout
 * @throw kernel#BadPositionException @a first or @a last is greater than the length of the line
 * @throw std#invalid_argument @a first is greater than @a last
 * @see #bounds(void), #bounds(length_t, length_t), #sublineBounds, #sublineIndent
 */
Rgn LineLayout::blackBoxBounds(length_t first, length_t last) const {
	if(first > last)
		throw invalid_argument("first is greater than last.");
	else if(last > text().length())
		throw kernel::BadPositionException(kernel::Position(lineNumber_, last));

	// handle empty line
	if(numberOfRuns_ == 0)
		return Rgn::createRect(0, 0, 0, linePitch());

	const length_t firstSubline = subline(first), lastSubline = subline(last);
	vector<RECT> rectangles;
	RECT rectangle;
	rectangle.top = 0;
	rectangle.bottom = rectangle.top + linePitch();
	for(length_t subline = firstSubline; subline <= lastSubline;
			++subline, rectangle.top = rectangle.bottom, rectangle.bottom += linePitch()) {
		const size_t endOfRuns = (subline + 1 < numberOfSublines_) ? sublineFirstRuns_[subline + 1] : numberOfRuns_;
		int cx = sublineIndent(subline);
		if(first <= sublineOffset(subline) && last >= sublineOffset(subline) + sublineLength(subline)) {
			// whole visual subline is encompassed by the range
			rectangle.left = cx;
			rectangle.right = rectangle.left + sublineWidth(subline);
			rectangles.push_back(rectangle);
			continue;
		}
		for(size_t i = sublineFirstRuns_[subline]; i < endOfRuns; ++i) {
			const Run& run = *runs_[i];
			if(first <= run.column() + run.length() && last >= run.column()) {
				rectangle.left = cx + ((first > run.column()) ?
					run.x(first - run.column(), false) : ((run.readingDirection() == LEFT_TO_RIGHT) ? 0 : run.totalWidth()));
				rectangle.right = cx + ((last < run.column() + run.length()) ?
					run.x(last - run.column(), false) : ((run.readingDirection() == LEFT_TO_RIGHT) ? run.totalWidth() : 0));
				if(rectangle.left != rectangle.right) {
					if(rectangle.left > rectangle.right)
						swap(rectangle.left, rectangle.right);
					rectangles.push_back(rectangle);
				}
			}
			cx += run.totalWidth();
		}
	}

	// create the result region
	manah::AutoBuffer<POINT> vertices(new POINT[rectangles.size() * 4]);
	manah::AutoBuffer<int> numbersOfVertices(new int[rectangles.size()]);
	for(size_t i = 0, c = rectangles.size(); i < c; ++i) {
		vertices[i * 4 + 0].x = vertices[i * 4 + 3].x = rectangles[i].left;
		vertices[i * 4 + 0].y = vertices[i * 4 + 1].y = rectangles[i].top;
		vertices[i * 4 + 1].x = vertices[i * 4 + 2].x = rectangles[i].right;
		vertices[i * 4 + 2].y = vertices[i * 4 + 3].y = rectangles[i].bottom;
	}
	fill_n(numbersOfVertices.get(), rectangles.size(), 4);
	return Rgn::createPolyPolygon(vertices.get(), numbersOfVertices.get(), static_cast<int>(rectangles.size()), WINDING);
}

/**
 * Returns the smallest rectangle emcompasses the whole text of the line. It might not coincide
 * exactly the ascent, descent or overhangs of the text.
 * @return the size of the bounds
 * @see #blackBoxBounds, #bounds(length_t, length_t), #sublineBounds
 */
SIZE LineLayout::bounds() const /*throw()*/ {
	SIZE s;
	s.cx = longestSublineWidth();
	s.cy = static_cast<long>(linePitch() * numberOfSublines_);
	return s;
}

/**
 * Returns the smallest rectangle emcompasses all characters in the range. It might not coincide
 * exactly the ascent, descent or overhangs of the specified region of the text.
 * @param first the start of the range
 * @param last the end of the range
 * @return the rectangle whose @c left value is the indentation of the bounds and @c top value is
 *         the distance from the top of the whole line
 * @throw kernel#BadPositionException @a first or @a last is greater than the length of the line
 * @throw std#invalid_argument @a first is greater than @a last
 * @see #blackBoxBounds, #bounds(void), #sublineBounds, #sublineIndent
 */
RECT LineLayout::bounds(length_t first, length_t last) const {
	if(first > last)
		throw invalid_argument("first is greater than last.");
	else if(last > text().length())
		throw kernel::BadPositionException(kernel::Position(lineNumber_, last));
	RECT bounds;	// the result
	int cx;

	// handle empty line
	if(numberOfRuns_ == 0) {
		bounds.left = bounds.top = bounds.right = 0;
		bounds.bottom = bounds.top + linePitch();
		return bounds;
	}

	// determine the top and the bottom (it's so easy)
	const length_t firstSubline = subline(first), lastSubline = subline(last);
	bounds.top = static_cast<LONG>(linePitch() * firstSubline);
	bounds.bottom = static_cast<LONG>(linePitch() * (lastSubline + 1));

	// find side bounds between 'firstSubline' and 'lastSubline'
	bounds.left = numeric_limits<LONG>::max();
	bounds.right = numeric_limits<LONG>::min();
	for(length_t subline = firstSubline + 1; subline < lastSubline; ++subline) {
		const LONG indent = sublineIndent(subline);
		bounds.left = min(indent, bounds.left);
		bounds.right = max(indent + sublineWidth(subline), bounds.right);
	}

	// find side bounds in 'firstSubline' and 'lastSubline'
	const length_t firstAndLast[2] = {firstSubline, lastSubline};
	for(size_t i = 0; i < MANAH_COUNTOF(firstAndLast); ++i) {
		const length_t subline = firstAndLast[i];
		const size_t endOfRuns = (subline + 1 < numberOfSublines_) ? sublineFirstRuns_[subline + 1] : numberOfRuns_;
		// find left bound
		cx = sublineIndent(subline);
		for(size_t j = sublineFirstRuns_[subline]; j < endOfRuns; ++j) {
			if(cx >= bounds.left)
				break;
			const Run& run = *runs_[j];
			if(first <= run.column() + run.length() && last >= run.column()) {
				const int x = run.x(((run.readingDirection() == LEFT_TO_RIGHT) ?
					max(first, run.column()) : min(last, run.column() + run.length())) - run.column(), false);
				bounds.left = min<LONG>(cx + x, bounds.left);
				break;
			}
			cx += run.totalWidth();
		}
		// find right bound
		cx = sublineIndent(firstSubline) + sublineWidth(lastSubline);
		for(size_t j = endOfRuns - 1; ; --j) {
			if(cx <= bounds.right)
				break;
			const Run& run = *runs_[j];
			if(first <= run.column() + run.length() && last >= run.column()) {
				const int x = run.x(((run.readingDirection() == LEFT_TO_RIGHT) ?
					min(last, run.column() + run.length()) : max(first, run.column())) - run.column(), false);
				bounds.right = max<LONG>(cx - run.totalWidth() + x, bounds.right);
				break;
			}
			if(j == sublineFirstRuns_[subline])
				break;
			cx -= run.totalWidth();
		}
	}

	return bounds;
}

/// Disposes the layout.
inline void LineLayout::dispose() /*throw()*/ {
	for(size_t i = 0; i < numberOfRuns_; ++i)
		delete runs_[i];
	delete[] runs_;
	runs_ = 0;
	numberOfRuns_ = 0;
	delete[] sublineOffsets_;
	delete[] sublineFirstRuns_;
	sublineFirstRuns_ = 0;
	numberOfSublines_ = 0;
}

/**
 * Draws the layout to the output device.
 * @param dc the device context
 * @param x the x-coordinate of the position to draw
 * @param y the y-coordinate of the position to draw
 * @param paintRect the region to draw
 * @param clipRect the clipping region
 * @param selection defines the region and the color of the selection
 */
void LineLayout::draw(DC& dc, int x, int y, const RECT& paintRect, const RECT& clipRect, const Selection* selection) const /*throw()*/ {
	const int dy = linePitch();

	// empty line
	if(isDisposed()) {
		RECT r;
		r.left = max(paintRect.left, clipRect.left);
		r.top = max(clipRect.top, max<long>(paintRect.top, y));
		r.right = min(paintRect.right, clipRect.right);
		r.bottom = min(clipRect.bottom, min<long>(paintRect.bottom, y + dy));
		const Colors lineColor = lip_.presentation().getLineColor(lineNumber_);
		dc.fillSolidRect(r, systemColors.serve(lineColor.background, COLOR_WINDOW));
		return;
	}

	// skip to the subline needs to draw
	length_t subline = (y + dy >= paintRect.top) ? 0 : (paintRect.top - (y + dy)) / dy;
	if(subline >= numberOfSublines_)
		return;	// this logical line does not need to draw
	y += static_cast<int>(dy * subline);

	for(; subline < numberOfSublines_; ++subline) {
		draw(subline, dc, x, y, paintRect, clipRect, selection);
		if((y += dy) >= paintRect.bottom)	// to next subline
			break;
	}
}

/**
 * Draws the specified subline layout to the output device.
 * @param subline the visual subline
 * @param dc the device context
 * @param x the x-coordinate of the position to draw
 * @param y the y-coordinate of the position to draw
 * @param paintRect the region to draw
 * @param clipRect the clipping region
 * @param selection defines the region and the color of the selection. can be @c null
 * @throw IndexOutOfBoundsException @a subline is invalid
 */
void LineLayout::draw(length_t subline, DC& dc,
		int x, int y, const RECT& paintRect, const RECT& clipRect, const Selection* selection) const {
	if(subline >= numberOfSublines_)
		throw IndexOutOfBoundsException("subline");

#ifdef _DEBUG
	if(DIAGNOSE_INHERENT_DRAWING)
		manah::win32::DumpContext() << L"@LineLayout.draw draws line " << lineNumber_ << L" (" << subline << L")\n";
#endif // _DEBUG

	// the following topic describes how to draw a selected text using masking by clipping
	// Catch 22 : Design and Implementation of a Win32 Text Editor
	// Part 10 - Transparent Text and Selection Highlighting (http://www.catch22.net/tuts/editor10.asp)

	const int dy = linePitch();
	const int lineHeight = lip_.textMetrics().cellHeight();
	const Colors lineColor = lip_.presentation().getLineColor(lineNumber_);
	const COLORREF marginColor = systemColors.serve(lineColor.background, COLOR_WINDOW);
	ISpecialCharacterRenderer::DrawingContext context(dc);
	ISpecialCharacterRenderer* specialCharacterRenderer = lip_.specialCharacterRenderer();

	if(specialCharacterRenderer != 0) {
		context.rect.top = y;
		context.rect.bottom = y + lineHeight;
	}

	const int savedCookie = dc.save();
	dc.setTextAlign(TA_BASELINE | TA_LEFT | TA_NOUPDATECP);
	if(isDisposed()) {	// empty line
		RECT r;
		r.left = max(paintRect.left, clipRect.left);
		r.top = max(clipRect.top, max<long>(paintRect.top, y));
		r.right = min(paintRect.right, clipRect.right);
		r.bottom = min(clipRect.bottom, min<long>(paintRect.bottom, y + dy));
		dc.fillSolidRect(r, marginColor);
	} else {
		const String& line = text();
		HRESULT hr;
		Range<length_t> selectedRange;
		if(selection != 0) {
			if(!selectedRangeOnVisualLine(selection->caret(), lineNumber_, subline, selectedRange))
				selection = 0;
		}

		// paint between sublines
		Rgn clipRegion(Rgn::createRect(clipRect.left, max<long>(y, clipRect.top), clipRect.right, min<long>(y + dy, clipRect.bottom)));
//		dc.selectClipRgn(clipRegion.getHandle());
		if(dy - lineHeight > 0)
			dc.fillSolidRect(paintRect.left, y + lineHeight, paintRect.right - paintRect.left, dy - lineHeight, marginColor);

		x += sublineIndent(subline);

		// 1. paint background of the runs
		// 2. determine the first and the last runs need to draw
		// 3. mask selected region
		tr1::shared_ptr<const RunStyle> defaultStyle(lip_.presentation().defaultTextRunStyle());
		const COLORREF defaultForeground = systemColors.serve((defaultStyle.get() != 0) ? defaultStyle->foreground : Color(), COLOR_WINDOWTEXT);
		const COLORREF defaultBackground = systemColors.serve((defaultStyle.get() != 0) ? defaultStyle->background : Color(), COLOR_WINDOW);
		size_t firstRun = sublineFirstRuns_[subline];
		size_t lastRun = (subline < numberOfSublines_ - 1) ? sublineFirstRuns_[subline + 1] : numberOfRuns_;
		// paint the left margin
		if(x > paintRect.left)
			dc.fillSolidRect(paintRect.left, y, x - paintRect.left, lineHeight, marginColor);
		// paint background of the runs
		int startX = x;
		for(size_t i = firstRun; i < lastRun; ++i) {
			Run& run = *runs_[i];
			if(x + run.totalWidth() < paintRect.left) {	// this run does not need to draw
				++firstRun;
				startX = x + run.totalWidth();
			} else {
				COLORREF background;
				if(lineColor.background != Color())
					background = marginColor;
				else if(run.requestedStyle().get() != 0 && run.requestedStyle()->background != Color())
					background = run.requestedStyle()->background.asCOLORREF();
				else
					background = defaultBackground;
				if(selection == 0 || run.column() >= selectedRange.end() || run.column() + run.length() <= selectedRange.beginning())
					// no selection in this run
					dc.fillSolidRect(x, y, run.totalWidth(), lineHeight, background);
				else if(selection != 0 && run.column() >= selectedRange.beginning() && run.column() + run.length() <= selectedRange.end()) {
					// this run is selected entirely
					dc.fillSolidRect(x, y, run.totalWidth(), lineHeight, selection->color().background.asCOLORREF());
					dc.excludeClipRect(x, y, x + run.totalWidth(), y + lineHeight);
				} else {
					// selected partially
					int left = run.x(max(selectedRange.beginning(), run.column()) - run.column(), false);
					int right = run.x(min(selectedRange.end(), run.column() + run.length()) - 1 - run.column(), true);
					if(left > right)
						swap(left, right);
					left += x;
					right += x;
					if(left > x/* && left > paintRect.left*/)
						dc.fillSolidRect(x, y, left - x, lineHeight, background);
					if(right > left) {
						dc.fillSolidRect(left, y, right - left, lineHeight, selection->color().background.asCOLORREF());
						dc.excludeClipRect(left, y, right, y + lineHeight);
					}
					if(right < x + run.totalWidth())
						dc.fillSolidRect(right, y, run.totalWidth() - (left - x), lineHeight, background);
				}
			}
			x += run.totalWidth();
			if(x >= paintRect.right) {
				lastRun = i + 1;
				break;
			}
		}
		// paint the right margin
		if(x < paintRect.right)
			dc.fillSolidRect(x, y, paintRect.right - x, dy, marginColor);

		// draw outside of the selection
		RECT runRect;
		runRect.top = y;
		runRect.bottom = y + dy;
		runRect.left = x = startX;
		dc.setBkMode(TRANSPARENT);
		for(size_t i = firstRun; i < lastRun; ++i) {
			Run& run = *runs_[i];
			COLORREF foreground;
			if(lineColor.foreground != Color())
				foreground = lineColor.foreground.asCOLORREF();
			else if(run.requestedStyle().get() != 0 && run.requestedStyle()->foreground != Color())
				foreground = run.requestedStyle()->foreground.asCOLORREF();
			else
				foreground = defaultForeground;
			if(line[run.column()] != L'\t') {
				if(selection == 0 || run.overhangs()
						|| !(run.column() >= selectedRange.beginning() && run.column() + run.length() <= selectedRange.end())) {
					dc.setTextColor(foreground);
					runRect.left = x;
					runRect.right = runRect.left + run.totalWidth();
					hr = run.draw(dc, x, y + lip_.textMetrics().ascent(), false, &runRect);
				}
			}
			// decoration (underline and border)
			if(run.requestedStyle().get() != 0)
				drawDecorationLines(dc, *run.requestedStyle(), foreground, x, y, run.totalWidth(), dy);
			x += run.totalWidth();
			runRect.left = x;
		}

		// draw selected text segment (also underline and border)
		if(selection != 0) {
			x = startX;
			clipRegion.setRect(clipRect);
			dc.selectClipRgn(clipRegion.get(), RGN_XOR);
			for(size_t i = firstRun; i < lastRun; ++i) {
				Run& run = *runs_[i];
				// text
				if(selection != 0 && line[run.column()] != L'\t'
						&& (run.overhangs() || (run.column() < selectedRange.end() && run.column() + run.length() > selectedRange.beginning()))) {
					dc.setTextColor(selection->color().foreground.asCOLORREF());
					runRect.left = x;
					runRect.right = runRect.left + run.totalWidth();
					hr = run.draw(dc, x, y + lip_.textMetrics().ascent(), false, &runRect);
				}
				// decoration (underline and border)
				if(run.requestedStyle().get() != 0)
					drawDecorationLines(dc, *run.requestedStyle(), selection->color().foreground.asCOLORREF(), x, y, run.totalWidth(), dy);
				x += run.totalWidth();
			}
		}

		// special character substitution
		if(specialCharacterRenderer != 0) {
			// white spaces and C0/C1 control characters
			dc.selectClipRgn(clipRegion.get());
			x = startX;
			for(size_t i = firstRun; i < lastRun; ++i) {
				Run& run = *runs_[i];
				context.readingDirection = run.readingDirection();
				for(length_t j = run.column(); j < run.column() + run.length(); ++j) {
					if(BinaryProperty::is(line[j], BinaryProperty::WHITE_SPACE)) {	// IdentifierSyntax.isWhiteSpace() is preferred?
						context.rect.left = x + run.x(j - run.column(), false);
						context.rect.right = x + run.x(j - run.column(), true);
						if(context.rect.left > context.rect.right)
							swap(context.rect.left, context.rect.right);
						specialCharacterRenderer->drawWhiteSpaceCharacter(context, line[j]);
					} else if(isC0orC1Control(line[j])) {
						context.rect.left = x + run.x(j - run.column(), false);
						context.rect.right = x + run.x(j - run.column(), true);
						if(context.rect.left > context.rect.right)
							swap(context.rect.left, context.rect.right);
						specialCharacterRenderer->drawControlCharacter(context, line[j]);
					}
				}
				x += run.totalWidth();
			}
		}
		if(subline == numberOfSublines_ - 1
				&& resolveTextAlignment(alignment(), readingDirection()) == ALIGN_RIGHT)
			x = startX;
	} // end of nonempty line case
	
	// line terminator and line wrapping mark
	const Document& document = lip_.presentation().document();
	if(specialCharacterRenderer != 0) {
		context.readingDirection = lineTerminatorOrientation(style(), lip_.presentation().defaultLineStyle());
		if(subline < numberOfSublines_ - 1) {	// line wrapping mark
			const int markWidth = specialCharacterRenderer->getLineWrappingMarkWidth(context);
			if(context.readingDirection == LEFT_TO_RIGHT) {
				context.rect.right = lip_.width();
				context.rect.left = context.rect.right - markWidth;
			} else {
				context.rect.left = 0;
				context.rect.right = markWidth;
			}
			specialCharacterRenderer->drawLineWrappingMark(context);
		} else if(lineNumber_ < document.numberOfLines() - 1) {	// line teminator
			const kernel::Newline nlf = document.getLineInformation(lineNumber_).newline();
			const int nlfWidth = specialCharacterRenderer->getLineTerminatorWidth(context, nlf);
			if(context.readingDirection == LEFT_TO_RIGHT) {
				context.rect.left = x;
				context.rect.right = x + nlfWidth;
			} else {
				context.rect.left = x - nlfWidth;
				context.rect.right = x;
			}
			if(selection != 0) {
				const Position eol(lineNumber_, document.lineLength(lineNumber_));
				if(!selection->caret().isSelectionRectangle()
						&& selection->caret().beginning().position() <= eol
						&& selection->caret().end().position() > eol)
					dc.fillSolidRect(x - (context.readingDirection == RIGHT_TO_LEFT ? nlfWidth : 0),
						y, nlfWidth, dy, selection->color().background.asCOLORREF());
			}
			dc.setBkMode(TRANSPARENT);
			specialCharacterRenderer->drawLineTerminator(context, nlf);
		}
	}

	dc.restore(savedCookie);
}

#ifdef _DEBUG
/**
 * Dumps the all runs to the specified output stream.
 * @param out the output stream
 */
void LineLayout::dumpRuns(ostream& out) const {
	for(size_t i = 0; i < numberOfRuns_; ++i) {
		const Run& run = *runs_[i];
		out << static_cast<uint>(i)
			<< ":column=" << static_cast<uint>(run.column())
			<< ",length=" << static_cast<uint>(run.length()) << endl;
	}
}
#endif // _DEBUG

/// Expands the all tabs and resolves each width.
inline void LineLayout::expandTabsWithoutWrapping() /*throw()*/ {
	const String& lineString = text();
	const int fullTabWidth = lip_.textMetrics().averageCharacterWidth() * lip_.layoutSettings().tabWidth;
	int x = 0;

	if(lineTerminatorOrientation(style(), lip_.presentation().defaultLineStyle()) == LEFT_TO_RIGHT) {	// expand from the left most
		for(size_t i = 0; i < numberOfRuns_; ++i) {
			Run& run = *runs_[i];
			run.expandTabCharacters(lineString, x, fullTabWidth, numeric_limits<int>::max());
			x += run.totalWidth();
		}
	} else {	// expand from the right most
		for(size_t i = numberOfRuns_; i > 0; --i) {
			Run& run = *runs_[i - 1];
			run.expandTabCharacters(lineString, x, fullTabWidth, numeric_limits<int>::max());
			x += run.totalWidth();
		}
	}
	longestSublineWidth_ = x;
}

/**
 * Returns the space string added to the end of the specified line to reach the specified virtual
 * point. If the end of the line is over @a virtualX, the result is an empty string.
 * @param x the x-coordinate of the virtual point
 * @return the space string consists of only white spaces (U+0020) and horizontal tabs (U+0009)
 * @throw kernel#BadPositionException @a line is outside of the document
 * @deprecated 0.8
 * @note This does not support line wrapping and bidirectional context.
 */
String LineLayout::fillToX(int x) const {
#if 0
	int cx = longestSublineWidth();
	if(cx >= x)
		return L"";

	size_t numberOfTabs = 0;
	while(true) {
		const int next = nextTabStopBasedLeftEdge(cx, true);
		if(next > x)
			break;
		++numberOfTabs;
		cx = next;
	}

	if(cx == x)
		return String(numberOfTabs, L'\t');

	auto_ptr<DC> dc(lip_.getFontSelector().deviceContext());
	HFONT oldFont = dc->selectObject(lip_.getFontSelector().font(Script::COMMON));
	int spaceWidth;
	dc->getCharWidth(L' ', L' ', &spaceWidth);
	dc->selectObject(oldFont);
	size_t numberOfSpaces = 0;
	while(true) {
		if(cx + spaceWidth > x)
			break;
		++numberOfSpaces;
		cx += spaceWidth;
	}

	String result(numberOfTabs + numberOfSpaces, L' ');
	result.replace(0, numberOfTabs, numberOfTabs, L'\t');
	return result;
#else
	return String();
#endif
}

/**
 * Returns the index of run containing the specified column.
 * @param column the column
 * @return the index of the run
 */
inline size_t LineLayout::findRunForPosition(length_t column) const /*throw()*/ {
	assert(numberOfRuns_ > 0);
	if(column == text().length())
		return numberOfRuns_ - 1;
	const length_t sl = subline(column);
	const size_t lastRun = (sl + 1 < numberOfSublines_) ? sublineFirstRuns_[sl + 1] : numberOfRuns_;
	for(size_t i = sublineFirstRuns_[sl]; i < lastRun; ++i) {
		if(runs_[i]->column() <= column && runs_[i]->column() + runs_[i]->length() > column)
			return i;
	}
	assert(false);
	return lastRun - 1;	// never reachable...
}
#if 0
/// Returns an iterator addresses the first styled segment.
LineLayout::StyledSegmentIterator LineLayout::firstStyledSegment() const /*throw()*/ {
	const Run* temp = *runs_;
	return StyledSegmentIterator(temp);
}
#endif
/// Returns if the line contains right-to-left run.
bool LineLayout::isBidirectional() const /*throw()*/ {
	if(readingDirection() == RIGHT_TO_LEFT)
		return true;
	for(size_t i = 0; i < numberOfRuns_; ++i) {
		if(runs_[i]->readingDirection() == RIGHT_TO_LEFT)
			return true;
	}
	return false;
}

/**
 * Itemizes the text into shapable runs.
 * @param lineNumber the line number of the line
 */
inline void LineLayout::itemize(length_t lineNumber) /*throw()*/ {
	const String& line = text();
	assert(!line.empty());

	HRESULT hr;
	const LayoutSettings& c = lip_.layoutSettings();
	const Presentation& presentation = lip_.presentation();

	// configure
	AutoZero<SCRIPT_CONTROL> control;
	AutoZero<SCRIPT_STATE> initialState;
	initialState.uBidiLevel = (readingDirection() == RIGHT_TO_LEFT) ? 1 : 0;
//	initialState.fOverrideDirection = 1;
	initialState.fInhibitSymSwap = c.inhibitsSymmetricSwapping;
	initialState.fDisplayZWG = c.displaysShapingControls;
	resolveNumberSubstitution(
		(style_.get() != 0) ? &style_->numberSubstitution : 0, control, initialState);	// ignore result...

	// itemize
	// note that ScriptItemize can cause a buffer overflow (see Mozilla bug 366643)
	static SCRIPT_ITEM fastItems[128];
	int expectedNumberOfRuns = max(static_cast<int>(line.length()) / 8, 2);
	SCRIPT_ITEM* items;
	int numberOfItems;
	if(expectedNumberOfRuns <= MANAH_COUNTOF(fastItems)) {
		hr = ::ScriptItemize(line.data(), static_cast<int>(line.length()),
			expectedNumberOfRuns = MANAH_COUNTOF(fastItems) - 1, &control, &initialState, items = fastItems, &numberOfItems);
		if(hr == E_OUTOFMEMORY)
			expectedNumberOfRuns *= 2;
	} else
		hr = E_OUTOFMEMORY;
	if(hr == E_OUTOFMEMORY) {
		while(true) {
			items = new SCRIPT_ITEM[expectedNumberOfRuns];
			hr = ::ScriptItemize(line.data(), static_cast<int>(line.length()),
				expectedNumberOfRuns - 1, &control, &initialState, items, &numberOfItems);
			if(hr != E_OUTOFMEMORY)	// expectedNumberOfRuns was enough...
				break;
			delete[] items;
			expectedNumberOfRuns *= 2;
		}
	}
	if(c.disablesDeprecatedFormatCharacters) {
		for(int i = 0; i < numberOfItems; ++i) {
			items[i].a.s.fInhibitSymSwap = initialState.fInhibitSymSwap;
			items[i].a.s.fDigitSubstitute = initialState.fDigitSubstitute;
		}
	}

	// text run style
	merge(items, numberOfItems, presentation.textRunStyles(lineNumber));

	if(items != fastItems)
		delete[] items;
}

/// Justifies the wrapped visual lines.
inline void LineLayout::justify() /*throw()*/ {
	assert(wrapWidth_ != -1);
	for(length_t subline = 0; subline < numberOfSublines_; ++subline) {
		const int lineWidth = sublineWidth(subline);
		const size_t last = (subline + 1 < numberOfSublines_) ? sublineFirstRuns_[subline + 1] : numberOfRuns_;
		for(size_t i = sublineFirstRuns_[subline]; i < last; ++i) {
			Run& run = *runs_[i];
			const int newRunWidth = ::MulDiv(run.totalWidth(), wrapWidth_, lineWidth);	// TODO: there is more precise way.
			run.justify(newRunWidth);
		}
	}
}
#if 0
/// Returns an iterator addresses the last styled segment.
LineLayout::StyledSegmentIterator LineLayout::lastStyledSegment() const /*throw()*/ {
	const Run* temp = runs_[numberOfRuns_];
	return StyledSegmentIterator(temp);
}
#endif
/// Returns the line pitch in pixels.
inline int LineLayout::linePitch() const /*throw()*/ {
	return lip_.textMetrics().cellHeight() + max(lip_.layoutSettings().lineSpacing, lip_.textMetrics().lineGap());
}

// implements public location methods
void LineLayout::locations(length_t column, POINT* leading, POINT* trailing) const {
	assert(leading != 0 || trailing != 0);
	if(column > text().length())
		throw kernel::BadPositionException(kernel::Position(lineNumber_, column));
	else if(isDisposed()) {
		if(leading != 0)
			leading->x = leading->y = 0;
		if(trailing != 0)
			trailing->x = trailing->y = 0;
		return;
	}
	const length_t sl = subline(column);
	const length_t firstRun = sublineFirstRuns_[sl];
	const length_t lastRun = (sl + 1 < numberOfSublines_) ? sublineFirstRuns_[sl + 1] : numberOfRuns_;
	// about x
	if(readingDirection() == LEFT_TO_RIGHT) {	// LTR
		int x = sublineIndent(sl);
		for(size_t i = firstRun; i < lastRun; ++i) {
			const Run& run = *runs_[i];
			if(column >= run.column() && column <= run.column() + run.length()) {
				if(leading != 0)
					leading->x = x + run.x(column - run.column(), false);
				if(trailing != 0)
					trailing->x = x + run.x(column - run.column(), true);
				break;
			}
			x += run.totalWidth();
		}
	} else {	// RTL
		int x = sublineIndent(sl) + sublineWidth(sl);
		for(size_t i = lastRun - 1; ; --i) {
			const Run& run = *runs_[i];
			x -= run.totalWidth();
			if(column >= run.column() && column <= run.column() + run.length()) {
				if(leading != 0)
					leading->x = x + run.x(column - run.column(), false);
				if(trailing)
					trailing->x = x + run.x(column - run.column(), true);
				break;
			}
			if(i == firstRun)
				break;
		}
	}
	// about y
	if(leading != 0)
		leading->y = static_cast<long>(sl * linePitch());
	if(trailing != 0)
		trailing->y = static_cast<long>(sl * linePitch());
}

/// Returns the width of the longest subline.
int LineLayout::longestSublineWidth() const /*throw()*/ {
	if(longestSublineWidth_ == -1) {
		int width = 0;
		for(length_t subline = 0; subline < numberOfSublines_; ++subline)
			width = max<long>(sublineWidth(subline), width);
		const_cast<LineLayout*>(this)->longestSublineWidth_ = width;
	}
	return longestSublineWidth_;
}

/**
 * Merges the given item runs and the given style runs.
 * @param items the items itemized by @c #itemize()
 * @param numberOfItems the length of the array @a items
 * @param styles the iterator returns the styled runs in the line. can be @c null
 * @see presentation#Presentation#getLineStyle
 */
inline void LineLayout::merge(const SCRIPT_ITEM items[], size_t numberOfItems, auto_ptr<IStyledRunIterator> styles) /*throw()*/ {
	// TODO: 'styles' value seems to be not needed to pass as a parameter.
	assert(runs_ == 0 && items != 0 && numberOfItems > 0);

#define ASCENSION_SPLIT_LAST_RUN()										\
	while(runs.back()->length() > MAXIMUM_RUN_LENGTH) {					\
		Run& back = *runs.back();										\
		Run* piece = new SimpleRun(back.style);							\
		length_t pieceLength = MAXIMUM_RUN_LENGTH;						\
		if(surrogates::isLowSurrogate(line[back.column + pieceLength]))	\
			--pieceLength;												\
		piece->analysis = back.analysis;								\
		piece->column = back.column + pieceLength;						\
		piece->setLength(back.length() - pieceLength);					\
		back.setLength(pieceLength);									\
		runs.push_back(piece);											\
	}

	const String& lineString = text();
	vector<Run*> runs;
	runs.reserve(static_cast<size_t>(numberOfItems * ((styles.get() != 0) ? 1.2 : 1)));	// hmm...

	const SCRIPT_ITEM* scriptRun = items;
	pair<const SCRIPT_ITEM*, length_t> nextScriptRun;
	nextScriptRun.first = (numberOfItems > 1) ? (items + 1) : 0;
	nextScriptRun.second = (nextScriptRun.first != 0) ? nextScriptRun.first->iCharPos : lineString.length();
	const StyledRun* styleRun = (styles.get() != 0 && !styles->isDone()) ? &styles->current() : 0;
	if(styleRun != 0)
		styles->next();
	pair<const StyledRun*, length_t> nextStyleRun;
	nextStyleRun.first = (styles.get() != 0 && !styles->isDone()) ? &styles->current() : 0;
	nextStyleRun.second = (nextStyleRun.first != 0) ? nextStyleRun.first->column : lineString.length();
	do {
		const length_t previousRunEnd = max<length_t>(scriptRun->iCharPos, (styleRun != 0) ? styleRun->column : 0);
		assert((runs.empty() && previousRunEnd == 0) || (previousRunEnd == runs.back()->column() + runs.back()->length()));
		length_t newRunEnd;
		bool forwardScriptRun = false, forwardStyleRun = false, breakScriptRun = false;

		if(nextScriptRun.second == nextStyleRun.second) {
			newRunEnd = nextScriptRun.second;
			forwardScriptRun = forwardStyleRun = true;
		} else if(nextScriptRun.second < nextStyleRun.second) {
			newRunEnd = nextScriptRun.second;
			forwardScriptRun = true;
		} else {	// nextScriptRun.second > nextStyleRun.second
			newRunEnd = nextStyleRun.second;
			forwardStyleRun = true;
			breakScriptRun = true;
		}

		if(breakScriptRun) {
			if(breakScriptRun =
					!legacyctype::isspace(lineString[previousRunEnd - 1]) && !legacyctype::isspace(lineString[previousRunEnd]))
				const_cast<SCRIPT_ITEM*>(scriptRun)->a.fLinkAfter = 1;
		}
		runs.push_back(
			new Run(previousRunEnd, newRunEnd - previousRunEnd,
				scriptRun->a, (styleRun != 0) ? styleRun->style : tr1::shared_ptr<const RunStyle>()));
		if(breakScriptRun) {
			const_cast<SCRIPT_ITEM*>(scriptRun)->a.fLinkBefore = 1;
			const_cast<SCRIPT_ITEM*>(scriptRun)->a.fLinkAfter = 0;
		}
		if(forwardScriptRun) {
			scriptRun = nextScriptRun.first;
			if(nextScriptRun.first != 0) {
				if(++nextScriptRun.first == items + numberOfItems)
					nextScriptRun.first = 0;
				nextScriptRun.second = (nextScriptRun.first != 0) ? nextScriptRun.first->iCharPos : lineString.length();
			}
		}
		if(forwardStyleRun && styles.get() != 0) {
			styleRun = nextStyleRun.first;
			styles->next();
			nextStyleRun.first = styles->isDone() ? 0 : &styles->current();
			nextStyleRun.second = (nextStyleRun.first != 0) ? nextStyleRun.first->column : lineString.length();
		}

		while(true) {
			auto_ptr<Run> piece(runs.back()->splitIfTooLong(lineString));
			if(piece.get() == 0)
				break;
			runs.push_back(piece.release());
		}
	} while(scriptRun != 0 || styleRun != 0);

	runs_ = new Run*[numberOfRuns_ = runs.size()];
	copy(runs.begin(), runs.end(), runs_);

#undef ASCENSION_SPLIT_LAST_RUN
}

/// Reorders the runs in visual order.
inline void LineLayout::reorder() /*throw()*/ {
	if(numberOfRuns_ == 0)
		return;
	Run** temp = new Run*[numberOfRuns_];
	memcpy(temp, runs_, sizeof(Run*) * numberOfRuns_);
	for(length_t subline = 0; subline < numberOfSublines_; ++subline) {
		const size_t numberOfRunsInSubline = ((subline < numberOfSublines_ - 1) ?
			sublineFirstRuns_[subline + 1] : numberOfRuns_) - sublineFirstRuns_[subline];
		BYTE* const levels = new BYTE[numberOfRunsInSubline];
		for(size_t i = 0; i < numberOfRunsInSubline; ++i)
			levels[i] = static_cast<BYTE>(runs_[i + sublineFirstRuns_[subline]]->bidiEmbeddingLevel() & 0x1f);
		int* const log2vis = new int[numberOfRunsInSubline];
		const HRESULT hr = ::ScriptLayout(static_cast<int>(numberOfRunsInSubline), levels, 0, log2vis);
		assert(SUCCEEDED(hr));
		delete[] levels;
		for(size_t i = sublineFirstRuns_[subline]; i < sublineFirstRuns_[subline] + numberOfRunsInSubline; ++i)
			runs_[sublineFirstRuns_[subline] + log2vis[i - sublineFirstRuns_[subline]]] = temp[i];
		delete[] log2vis;
	}
	delete[] temp;
}

/**
 * Returns the next tab stop position.
 * @param x the distance from leading edge of the line (can not be negative)
 * @param direction the direction
 * @return the distance from leading edge of the line to the next tab position
 */
inline int LineLayout::nextTabStop(int x, Direction direction) const /*throw()*/ {
	assert(x >= 0);
	const int tabWidth = lip_.textMetrics().averageCharacterWidth() * lip_.layoutSettings().tabWidth;
	return (direction == Direction::FORWARD) ? x + tabWidth - x % tabWidth : x - x % tabWidth;
}

/**
 * Returns the next tab stop.
 * @param x the distance from the left edge of the line to base position (can not be negative)
 * @param right @c true to find the next right position
 * @return the tab stop position in pixel
 */
int LineLayout::nextTabStopBasedLeftEdge(int x, bool right) const /*throw()*/ {
	assert(x >= 0);
	const LayoutSettings& c = lip_.layoutSettings();
	const int tabWidth = lip_.textMetrics().averageCharacterWidth() * c.tabWidth;
	if(lineTerminatorOrientation(style(), lip_.presentation().defaultLineStyle()) == LEFT_TO_RIGHT)
		return nextTabStop(x, right ? Direction::FORWARD : Direction::BACKWARD);
	else
		return right ? x + (x - longestSublineWidth()) % tabWidth : x - (tabWidth - (x - longestSublineWidth()) % tabWidth);
}

/**
 * Returns the character column (offset) for the specified point.
 * @param x the x coordinate of the point. distance from the left edge of the first subline
 * @param y the y coordinate of the point. distance from the top edge of the first subline
 * @param[out] outside @c true if the specified point is outside of the layout. optional
 * @return a pair of the character offsets. the first element addresses the character whose black
 *         box (bounding box) encompasses the specified point. the second element addresses the
 *         character whose leading point is the closest to the specified point in the line
 * @see #location
 */
pair<length_t, length_t> LineLayout::offset(int x, int y, bool* outside /* = 0 */) const /*throw()*/ {
	if(text().empty())
		return make_pair(0, 0);

	// determine the subline
	length_t subline = 0;
	for(; subline < numberOfSublines_ - 1; ++subline) {
		if(static_cast<int>(linePitch() * subline) >= y)
			break;
	}

	pair<length_t, length_t> result;

	// determine the column
	assert(numberOfRuns_ > 0);
	const size_t lastRun = (subline + 1 < numberOfSublines_) ? sublineFirstRuns_[subline + 1] : numberOfRuns_;
	int cx = sublineIndent(subline);
	if(x <= cx) {	// on the left margin
		if(outside != 0)
			*outside = true;
		const Run& firstRun = *runs_[sublineFirstRuns_[subline]];
		result.first = result.second = firstRun.column()
			+ ((firstRun.readingDirection() == LEFT_TO_RIGHT) ? 0 : firstRun.length());	// TODO: used SCRIPT_ANALYSIS.fRTL past...
		return result;
	}
	for(size_t i = sublineFirstRuns_[subline]; i < lastRun; ++i) {
		const Run& run = *runs_[i];
		if(x >= cx && x <= cx + run.totalWidth()) {
			int cp, trailing;
			run.hitTest(x - cx, cp, trailing);	// TODO: check the returned value.
			if(outside != 0)
				*outside = false;
			result.first = run.column() + static_cast<length_t>(cp);
			result.second = result.first + static_cast<length_t>(trailing);
			return result;
		}
		cx += run.totalWidth();
	}
	// on the right margin
	if(outside != 0)
		*outside = true;
	result.first = result.second = runs_[lastRun - 1]->column()
		+ ((runs_[lastRun - 1]->readingDirection() == LEFT_TO_RIGHT) ? runs_[lastRun - 1]->length() : 0);	// used SCRIPT_ANALYSIS.fRTL past...
	return result;
}

/**
 * Returns the computed reading direction of the line.
 * @see #alignment
 */
ReadingDirection LineLayout::readingDirection() const /*throw()*/ {
	ReadingDirection result = INHERIT_READING_DIRECTION;
	// try the requested line style
	if(style_.get() != 0)
		result = style_->readingDirection;
	// try the default line style
	if(result == INHERIT_READING_DIRECTION) {
		tr1::shared_ptr<const LineStyle> defaultLineStyle(lip_.presentation().defaultLineStyle());
		if(defaultLineStyle.get() != 0)
			result = defaultLineStyle->readingDirection;
	}
	// try the default UI style
	if(result == INHERIT_READING_DIRECTION)
		result = lip_.defaultUIReadingDirection();
	// use user default
	if(result == INHERIT_READING_DIRECTION)
		result = ASCENSION_DEFAULT_TEXT_READING_DIRECTION;
	assert(result == LEFT_TO_RIGHT || result == RIGHT_TO_LEFT);
	return result;
}
#if 0
/**
 * Returns the styled text run containing the specified column.
 * @param column the column
 * @return the styled segment
 * @throw kernel#BadPositionException @a column is greater than the length of the line
 */
StyledRun LineLayout::styledTextRun(length_t column) const {
	if(column > text().length())
		throw kernel::BadPositionException(kernel::Position(lineNumber_, column));
	const Run& run = *runs_[findRunForPosition(column)];
	return StyledRun(run.column(), run.requestedStyle());
}
#endif
/**
 * Returns the smallest rectangle emcompasses the specified visual line. It might not coincide
 * exactly the ascent, descent or overhangs of the specified subline.
 * @param subline the wrapped line
 * @return the rectangle whose @c left value is the indentation of the subline and @c top value is
 *         the distance from the top of the whole line
 * @throw IndexOutOfBoundsException @a subline is greater than the number of the wrapped lines
 * @see #sublineIndent
 */
RECT LineLayout::sublineBounds(length_t subline) const {
	if(subline >= numberOfSublines_)
		throw IndexOutOfBoundsException("subline");
	RECT rc;
	rc.left = sublineIndent(subline);
	rc.top = linePitch() * static_cast<long>(subline);
	rc.right = rc.left + sublineWidth(subline);
	rc.bottom = rc.top + linePitch();
	return rc;
}

/**
 * Returns the indentation of the specified subline. An indent is a horizontal distance from the
 * leftmost of the first subline to the leftmost of the given subline. If the subline is longer
 * than the first subline, the result is negative. The first subline's indent is always zero.
 * @param subline the visual line
 * @return the indentation in pixel
 * @throw IndexOutOfBoundsException @a subline is invalid
 */
int LineLayout::sublineIndent(length_t subline) const {
	if(subline == 0)
		return 0;
	const TextAlignment resolvedAlignment = resolveTextAlignment(alignment(), readingDirection());
	if(resolvedAlignment == ALIGN_LEFT || resolvedAlignment == JUSTIFY)	// TODO: recognize the last line if justified.
		return 0;
	switch(resolvedAlignment) {
	case ALIGN_LEFT:
	default:
		return 0;
	case ALIGN_RIGHT:
		return sublineWidth(0) - sublineWidth(subline);
	case ALIGN_CENTER:
		return (sublineWidth(0) - sublineWidth(subline)) / 2;
	}
}

/**
 * Returns the width of the specified wrapped line.
 * @param subline the visual line
 * @return the width
 * @throw IndexOutOfBoundsException @a subline is greater than the number of visual lines
 */
int LineLayout::sublineWidth(length_t subline) const {
	if(subline >= numberOfSublines_)
		throw IndexOutOfBoundsException("subline");
	else if(isDisposed())
		return 0;
	else if(numberOfSublines_ == 1 && longestSublineWidth_ != -1)
		return longestSublineWidth_;
	else {
		const size_t lastRun = (subline + 1 < numberOfSublines_) ? sublineFirstRuns_[subline + 1] : numberOfRuns_;
		int cx = 0;
		for(size_t i = sublineFirstRuns_[subline]; i < lastRun; ++i)
			cx += runs_[i]->totalWidth();
		return cx;
	}
}

/// Returns the text of the line.
inline const String& LineLayout::text() const /*throw()*/ {
	return lip_.presentation().document().line(lineNumber_);
}

/// Generates the glyphs for the text.
void LineLayout::shape(DC& dc) /*throw()*/ {
	// for each runs...
	const String& lineString = text();
	for(size_t i = 0; i < numberOfRuns_; ++i) {
		Run& run = *runs_[i];
		run.shape(dc, lineString, lip_, i > 0 ? runs_[i - 1] : 0);
		run.place(dc, lineString, lip_);
	}
}

/// Locates the wrap points and resolves tab expansions.
void LineLayout::wrap(DC& dc) /*throw()*/ {
	assert(numberOfRuns_ != 0 && lip_.layoutSettings().lineWrap.wraps());
	assert(numberOfSublines_ == 0 && sublineOffsets_ == 0 && sublineFirstRuns_ == 0);

	const String& lineString = text();
	vector<length_t> sublineFirstRuns;
	sublineFirstRuns.push_back(0);
	const int cookie = dc.save();
	int x1 = 0;	// addresses the beginning of the run. see x2
	const int fullTabWidth = lip_.textMetrics().averageCharacterWidth() * lip_.layoutSettings().tabWidth;
	manah::AutoBuffer<int> logicalWidths;
	manah::AutoBuffer<SCRIPT_LOGATTR> logicalAttributes;
	length_t longestRunLength = 0;	// for efficient allocation
	vector<Run*> newRuns;
	newRuns.reserve(numberOfRuns_ * 3 / 2);
	// for each runs... (at this time, runs_ is in logical order)
	for(size_t i = 0; i < numberOfRuns_; ++i) {
		Run* run = runs_[i];

		// if the run is a tab, expand and calculate actual width
		if(run->expandTabCharacters(lineString, (x1 < wrapWidth_) ? x1 : 0, fullTabWidth, wrapWidth_ - (x1 < wrapWidth_) ? x1 : 0)) {
			if(x1 < wrapWidth_) {
				x1 += run->totalWidth();
				newRuns.push_back(run);
			} else {
				x1 = run->totalWidth();
				newRuns.push_back(run);
				sublineFirstRuns.push_back(newRuns.size());
			}
			continue;
		}

		// obtain logical widths and attributes for all characters in this run to determine line break positions
		if(run->length() > longestRunLength) {
			longestRunLength = run->length();
			longestRunLength += 16 - longestRunLength % 16;
			logicalWidths.reset(new int[longestRunLength]);
			logicalAttributes.reset(new SCRIPT_LOGATTR[longestRunLength]);
		}
		HRESULT hr = run->logicalWidths(logicalWidths.get());
		hr = run->logicalAttributes(lineString, logicalAttributes.get());
		const length_t originalRunPosition = run->column();
		int widthInThisRun = 0;
		length_t lastBreakable = run->column(), lastGlyphEnd = run->column();
		int lastBreakableX = x1, lastGlyphEndX = x1;
		// for each characters in the run...
		for(length_t j = run->column(); j < run->column() + run->length(); ) {	// j is position in the LOGICAL line
			const int x2 = x1 + widthInThisRun;
			// remember this opportunity
			if(logicalAttributes[j - originalRunPosition].fCharStop != 0) {
				lastGlyphEnd = j;
				lastGlyphEndX = x2;
				if(logicalAttributes[j - originalRunPosition].fSoftBreak != 0
						|| logicalAttributes[j - originalRunPosition].fWhiteSpace != 0) {
					lastBreakable = j;
					lastBreakableX = x2;
				}
			}
			// break if the width of the visual line overs the wrap width
			if(x2 + logicalWidths[j - originalRunPosition] > wrapWidth_) {
				// the opportunity is the start of this run
				if(lastBreakable == run->column()) {
					// break at the last glyph boundary if no opportunities
					if(sublineFirstRuns.empty() || sublineFirstRuns.back() == newRuns.size()) {
						if(lastGlyphEnd == run->column()) {	// break here if no glyph boundaries
							lastBreakable = j;
							lastBreakableX = x2;
						} else {
							lastBreakable = lastGlyphEnd;
							lastBreakableX = lastGlyphEndX;
						}
					}
				}

				// case 1: break at the start of the run
				if(lastBreakable == run->column()) {
					assert(sublineFirstRuns.empty() || newRuns.size() != sublineFirstRuns.back());
					sublineFirstRuns.push_back(newRuns.size());
//dout << L"broke the line at " << lastBreakable << L" where the run start.\n";
				}
				// case 2: break at the end of the run
				else if(lastBreakable == run->column() + run->length()) {
					if(lastBreakable < lineString.length()) {
						assert(sublineFirstRuns.empty() || newRuns.size() != sublineFirstRuns.back());
						sublineFirstRuns.push_back(newRuns.size() + 1);
//dout << L"broke the line at " << lastBreakable << L" where the run end.\n";
					}
					break;
				}
				// case 3: break at the middle of the run -> split the run (run -> newRun + run)
				else {
					auto_ptr<Run> followingRun(run->breakAt(dc, lastBreakable, lineString, lip_));
					newRuns.push_back(run);
					assert(sublineFirstRuns.empty() || newRuns.size() != sublineFirstRuns.back());
					sublineFirstRuns.push_back(newRuns.size());
//dout << L"broke the line at " << lastBreakable << L" where the run meddle.\n";
					run = followingRun.release();	// continue the process about this run
				}
				widthInThisRun = x1 + widthInThisRun - lastBreakableX;
				lastBreakableX -= x1;
				lastGlyphEndX -= x1;
				x1 = 0;
				j = max(lastBreakable, j);
			} else
				widthInThisRun += logicalWidths[j++ - originalRunPosition];
		}
		newRuns.push_back(run);
		x1 += widthInThisRun;
	}
//dout << L"...broke the all lines.\n";
	dc.restore(cookie);
	if(newRuns.empty())
		newRuns.push_back(0);
	delete[] runs_;
	runs_ = new Run*[numberOfRuns_ = newRuns.size()];
	copy(newRuns.begin(), newRuns.end(), runs_);
	sublineFirstRuns_ = new length_t[numberOfSublines_ = sublineFirstRuns.size()];
	copy(sublineFirstRuns.begin(), sublineFirstRuns.end(), sublineFirstRuns_);
	sublineOffsets_ = new length_t[numberOfSublines_];
	for(size_t i = 0; i < numberOfSublines_; ++i)
		sublineOffsets_[i] = runs_[sublineFirstRuns_[i]]->column();
}


// LineLayout.Selection /////////////////////////////////////////////////////

namespace {
	inline Colors fallbackSelectionColors(const Colors& source, bool focused) {
		return Colors(
			(source.foreground != Color()) ? source.foreground :
				Color::fromCOLORREF(::GetSysColor(focused ? COLOR_HIGHLIGHTTEXT : COLOR_INACTIVECAPTIONTEXT)),
			(source.background != Color()) ? source.background :
				Color::fromCOLORREF(::GetSysColor(focused ? COLOR_HIGHLIGHT : COLOR_INACTIVECAPTION)));
	}
}

/**
 * Constructor.
 */
LineLayout::Selection::Selection(const viewers::Caret& caret) /*throw()*/ :
		caret_(caret), color_(fallbackSelectionColors(Colors(), caret.textViewer().hasFocus())) {
}

/// Constructor.
LineLayout::Selection::Selection(const viewers::Caret& caret, const Colors& color) /*throw()*/ :
		caret_(caret), color_(fallbackSelectionColors(color, caret.textViewer().hasFocus())) {
}

#if 0
// LineLayout.StyledSegmentIterator /////////////////////////////////////////

/**
 * Private constructor.
 * @param start
 */
LineLayout::StyledSegmentIterator::StyledSegmentIterator(const Run*& start) /*throw()*/ : p_(&start) {
}

/// Copy-constructor.
LineLayout::StyledSegmentIterator::StyledSegmentIterator(const StyledSegmentIterator& rhs) /*throw()*/ : p_(rhs.p_) {
}

/// Returns the current segment.
StyledRun LineLayout::StyledSegmentIterator::current() const /*throw()*/ {
	const Run& run = **p_;
	return StyledRun(run.column, run.style);
}
#endif

// LineLayoutBuffer /////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param document the document
 * @param bufferSize the maximum number of lines cached
 * @param autoRepair set @c true to repair disposed layout automatically if the line number of its
 *                   line was not changed
 * @throw std#invalid_argument @a bufferSize is zero
 */
LineLayoutBuffer::LineLayoutBuffer(Document& document, length_t bufferSize, bool autoRepair) :
		document_(document), bufferSize_(bufferSize), autoRepair_(autoRepair), documentChangePhase_(NONE),
		longestLineWidth_(0), longestLine_(INVALID_INDEX), numberOfVisualLines_(document.numberOfLines()) {
	pendingCacheClearance_.first = pendingCacheClearance_.last = INVALID_INDEX;
	if(bufferSize == 0)
		throw invalid_argument("size of the buffer can't be zero.");
	document_.addPrenotifiedListener(*this);
}

/// Destructor.
LineLayoutBuffer::~LineLayoutBuffer() /*throw()*/ {
//	clearCaches(startLine_, startLine_ + bufferSize_, false);
	for(list<LineLayout*>::iterator i(layouts_.begin()), e(layouts_.end()); i != e; ++i)
		delete *i;
	document_.removePrenotifiedListener(*this);
}

/**
 * Registers the visual lines listener.
 * @param listener the listener to be registered
 * @throw std#invalid_argument @a listener is already registered
 */
void LineLayoutBuffer::addVisualLinesListener(IVisualLinesListener& listener) {
	listeners_.add(listener);
	const length_t lines = document_.numberOfLines();
	if(lines > 1)
		listener.visualLinesInserted(1, lines);
}

/**
 * Clears the layout caches of the specified lines. This method calls @c #layoutModified.
 * @param first the start of lines
 * @param last the end of lines (exclusive. this line will not be cleared)
 * @param repair set @c true to recreate layouts for the lines. if @c true, this method calls
 * @c #layoutModified. otherwise calls @c #layoutDeleted
 * @throw std#invalid_argument @a first and/or @a last are invalid
 */
void LineLayoutBuffer::clearCaches(length_t first, length_t last, bool repair) {
	if(first > last /*|| last > viewer_.getDocument().getNumberOfLines()*/)
		throw invalid_argument("either line number is invalid.");
	if(documentChangePhase_ == ABOUT_CHANGE) {
		pendingCacheClearance_.first = (pendingCacheClearance_.first == INVALID_INDEX) ? first : min(first, pendingCacheClearance_.first);
		pendingCacheClearance_.last = (pendingCacheClearance_.last == INVALID_INDEX) ? last : max(last, pendingCacheClearance_.last);
		return;
	}
	if(first == last)
		return;

//	const size_t originalSize = layouts_.size();
	length_t oldSublines = 0, cachedLines = 0;
	if(repair) {
		DC dc;
		length_t newSublines = 0, actualFirst = last, actualLast = first;
		for(list<LineLayout*>::iterator i(layouts_.begin()); i != layouts_.end(); ++i) {
			LineLayout*& layout = *i;
			const length_t lineNumber = layout->lineNumber();
			if(lineNumber >= first && lineNumber < last) {
				oldSublines += layout->numberOfSublines();
				delete layout;
				if(dc.get() == 0)
					dc = deviceContext();
				layout = new LineLayout(dc, *lip_, lineNumber);
				newSublines += layout->numberOfSublines();
				++cachedLines;
				actualFirst = min(actualFirst, lineNumber);
				actualLast = max(actualLast, lineNumber);
			}
		}
		if(actualFirst == last)	// no lines cleared
			return;
		++actualLast;
		fireVisualLinesModified(actualFirst, actualLast, newSublines += actualLast - actualFirst - cachedLines,
			oldSublines += actualLast - actualFirst - cachedLines, documentChangePhase_ == CHANGING);
	} else {
		for(list<LineLayout*>::iterator i(layouts_.begin()); i != layouts_.end(); ) {
			if((*i)->lineNumber() >= first && (*i)->lineNumber() < last) {
				oldSublines += (*i)->numberOfSublines();
				delete *i;
				i = layouts_.erase(i);
				++cachedLines;
			} else
				++i;
		}
		fireVisualLinesDeleted(first, last, oldSublines += last - first - cachedLines);
	}
}

/// @see kernel#IDocumentListener#documentAboutToBeChanged
void LineLayoutBuffer::documentAboutToBeChanged(const kernel::Document&) {
	documentChangePhase_ = ABOUT_CHANGE;
}

/// @see kernel#IDocumentListener#documentChanged
void LineLayoutBuffer::documentChanged(const kernel::Document&, const kernel::DocumentChange& change) {
	documentChangePhase_ = CHANGING;
	assert(change.erasedRegion().isNormalized() && change.insertedRegion().isNormalized());
	if(change.erasedRegion().first.line != change.erasedRegion().second.line) {	// erased region includes newline(s)
		const Region& region = change.erasedRegion();
		clearCaches(region.first.line + 1, region.second.line + 1, false);
		for(list<LineLayout*>::iterator i(layouts_.begin()), e(layouts_.end()); i != e; ++i) {
			if((*i)->lineNumber() > region.first.line)
				(*i)->lineNumber_ -= region.second.line - region.first.line;	// $friendly-access
		}
	}
	if(change.insertedRegion().first.line != change.insertedRegion().second.line) {	// inserted text is multiline
		const Region& region = change.insertedRegion();
		for(list<LineLayout*>::iterator i(layouts_.begin()), e(layouts_.end()); i != e; ++i) {
			if((*i)->lineNumber() > region.first.line)
				(*i)->lineNumber_ += region.second.line - region.first.line;	// $friendly-access
		}
		fireVisualLinesInserted(region.first.line + 1, region.second.line + 1);
	}
	const length_t firstLine = min(change.erasedRegion().first.line, change.insertedRegion().first.line);
	if(pendingCacheClearance_.first == INVALID_INDEX
			|| firstLine < pendingCacheClearance_.first || firstLine >= pendingCacheClearance_.last)
		invalidate(firstLine);
	documentChangePhase_ = NONE;
	if(pendingCacheClearance_.first != INVALID_INDEX) {
		clearCaches(pendingCacheClearance_.first, pendingCacheClearance_.last, autoRepair_);
		pendingCacheClearance_.first = pendingCacheClearance_.last = INVALID_INDEX;
	}
}

void LineLayoutBuffer::fireVisualLinesDeleted(length_t first, length_t last, length_t sublines) {
	numberOfVisualLines_ -= sublines;
	const bool widthChanged = longestLine_ >= first && longestLine_ < last;
	if(widthChanged)
		updateLongestLine(static_cast<length_t>(-1), 0);
	listeners_.notify<length_t, length_t, length_t>(&IVisualLinesListener::visualLinesDeleted, first, last, sublines, widthChanged);
}

void LineLayoutBuffer::fireVisualLinesInserted(length_t first, length_t last) /*throw()*/ {
	numberOfVisualLines_ += last - first;
	listeners_.notify<length_t, length_t>(&IVisualLinesListener::visualLinesInserted, first, last);
}

void LineLayoutBuffer::fireVisualLinesModified(length_t first, length_t last,
		length_t newSublines, length_t oldSublines, bool documentChanged) /*throw()*/ {
	numberOfVisualLines_ += newSublines;
	numberOfVisualLines_ -= oldSublines;

	// update the longest line
	bool longestLineChanged = false;
	if(longestLine_ >= first && longestLine_ < last) {
		updateLongestLine(static_cast<length_t>(-1), 0);
		longestLineChanged = true;
	} else {
		length_t newLongestLine = longestLine_;
		int newLongestLineWidth = longestLineWidth_;
		for(Iterator i(firstCachedLine()), e(lastCachedLine()); i != e; ++i) {
			const LineLayout& layout = **i;
			if(layout.longestSublineWidth() > newLongestLineWidth) {
				newLongestLine = (*i)->lineNumber();
				newLongestLineWidth = layout.longestSublineWidth();
			}
		}
		if(longestLineChanged = (newLongestLine != longestLine_))
			updateLongestLine(newLongestLine, newLongestLineWidth);
	}

	listeners_.notify<length_t, length_t, signed_length_t>(
		&IVisualLinesListener::visualLinesModified, first, last,
		static_cast<signed_length_t>(newSublines) - static_cast<signed_length_t>(oldSublines), documentChanged, longestLineChanged);
}

/// Invalidates all layouts.
void LineLayoutBuffer::invalidate() /*throw()*/ {
	clearCaches(0, lip_->presentation().document().numberOfLines(), autoRepair_);
}

/**
 * Invalidates the layouts of the specified lines.
 * @param first the start of the lines
 * @param last the end of the lines (exclusive. this line will not be cleared)
 * @throw std#invalid_argument @a first &gt;= @a last
 */
void LineLayoutBuffer::invalidate(length_t first, length_t last) {
	if(first >= last)
		throw invalid_argument("Any line number is invalid.");
	clearCaches(first, last, autoRepair_);
}

/**
 * Resets the cached layout of the specified line and repairs if necessary.
 * @param line the line to invalidate layout
 */
inline void LineLayoutBuffer::invalidate(length_t line) {
	for(list<LineLayout*>::iterator i(layouts_.begin()), e(layouts_.end()); i != e; ++i) {
		LineLayout*& p = *i;
		if(p->lineNumber() == line) {
			const length_t oldSublines = p->numberOfSublines();
			delete p;
			if(autoRepair_) {
				p = new LineLayout(deviceContext(), *lip_, line);
				fireVisualLinesModified(line, line + 1, p->numberOfSublines(), oldSublines, documentChangePhase_ == CHANGING);
			} else {
				layouts_.erase(i);
				fireVisualLinesModified(line, line + 1, 1, oldSublines, documentChangePhase_ == CHANGING);
			}
			break;
		}
	}
}

/**
 * Returns the layout of the specified line.
 * @param line the line
 * @return the layout
 * @throw kernel#BadPositionException @a line is greater than the number of the lines
 */
const LineLayout& LineLayoutBuffer::lineLayout(length_t line) const {
#ifdef ASCENSION_TRACE_LAYOUT_CACHES
	manah::win32::DumpContext dout;
	dout << "finding layout for line " << line;
#endif
	if(line > lip_->presentation().document().numberOfLines())
		throw kernel::BadPositionException(kernel::Position(line, 0));
	LineLayoutBuffer& self = *const_cast<LineLayoutBuffer*>(this);
	list<LineLayout*>::iterator i(self.layouts_.begin());
	for(const list<LineLayout*>::iterator e(self.layouts_.end()); i != e; ++i) {
		if((*i)->lineNumber_ == line)
			break;
	}

	if(i != layouts_.end()) {
#ifdef ASCENSION_TRACE_LAYOUT_CACHES
		dout << "... cache found\n";
#endif
		LineLayout* layout = *i;
		if(layout != layouts_.front()) {
			// bring to the top
			self.layouts_.erase(i);
			self.layouts_.push_front(layout);
		}
		return *layout;
	} else {
#ifdef ASCENSION_TRACE_LAYOUT_CACHES
		dout << "... cache not found\n";
#endif
		if(layouts_.size() == bufferSize_) {
			// delete the last
			LineLayout* p = layouts_.back();
			self.layouts_.pop_back();
			self.fireVisualLinesModified(p->lineNumber(), p->lineNumber() + 1,
				1, p->numberOfSublines(), documentChangePhase_ == CHANGING);
			delete p;
		}
		LineLayout* const layout = new LineLayout(deviceContext(), *lip_, line);
		self.layouts_.push_front(layout);
		self.fireVisualLinesModified(line, line + 1, layout->numberOfSublines(), 1, documentChangePhase_ == CHANGING);
		return *layout;
	}
}

/**
 * Returns the first visual line number of the specified logical line.
 * @param line the logical line
 * @return the first visual line of @a line
 * @throw kernel#BadPositionException @a line is outside of the document
 * @see #mapLogicalPositionToVisualPosition
 */
length_t LineLayoutBuffer::mapLogicalLineToVisualLine(length_t line) const {
	if(line >= lip_->presentation().document().numberOfLines())
		throw kernel::BadPositionException(kernel::Position(line, 0));
	else if(!lip_->layoutSettings().lineWrap.wraps())
		return line;
	length_t result = 0, cachedLines = 0;
	for(Iterator i(firstCachedLine()), e(lastCachedLine()); i != e; ++i) {
		if((*i)->lineNumber() < line) {
			result += (*i)->numberOfSublines();
			++cachedLines;
		}
	}
	return result + line - cachedLines;
}

/**
 * Returns the visual line number and the visual column number of the specified logical position.
 * @param position the logical coordinates of the position to be mapped
 * @param[out] column the visual column of @a position. can be @c null if not needed
 * @return the visual line of @a position
 * @throw kernel#BadPositionException @a position is outside of the document
 * @see #mapLogicalLineToVisualLine
 */
length_t LineLayoutBuffer::mapLogicalPositionToVisualPosition(const Position& position, length_t* column) const {
	if(!lip_->layoutSettings().lineWrap.wraps()) {
		if(column != 0)
			*column = position.column;
		return position.line;
	}
	const LineLayout& layout = lineLayout(position.line);
	const length_t subline = layout.subline(position.column);
	if(column != 0)
		*column = position.column - layout.sublineOffset(subline);
	return mapLogicalLineToVisualLine(position.line) + subline;
}

#if 0
/**
 * Returns the logical line number and the visual subline number of the specified visual line.
 * @param line the visual line
 * @param[out] subline the visual subline of @a line. can be @c null if not needed
 * @return the logical line
 * @throw kernel#BadPositionException @a line is outside of the document
 * @see #mapLogicalLineToVisualLine, #mapVisualPositionToLogicalPosition
 */
length_t LineLayoutBuffer::mapVisualLineToLogicalLine(length_t line, length_t* subline) const {
	if(!getTextViewer().getConfiguration().lineWrap.wraps()) {
		if(subline != 0)
			*subline = 0;
		return line;
	}
	length_t c = getCacheFirstLine();
	for(length_t i = getCacheFirstLine(); ; ++i) {
		if(c + getNumberOfSublinesOfLine(i) > line) {
			if(subline != 0)
				*subline = line - c;
			return i;
		}
		c += getNumberOfSublinesOfLine(i);
	}
	assert(false);
	return getCacheLastLine();	// ここには来ない
}

/**
 * Returns the logical line number and the logical column number of the specified visual position.
 * @param position the visual coordinates of the position to be mapped
 * @return the logical coordinates of @a position
 * @throw kernel#BadPositionException @a position is outside of the document
 * @see #mapLogicalPositionToVisualPosition, #mapVisualLineToLogicalLine
 */
Position LineLayoutBuffer::mapVisualPositionToLogicalPosition(const Position& position) const {
	if(!getTextViewer().getConfiguration().lineWrap.wraps())
		return position;
	Position result;
	length_t subline;
	result.line = mapVisualLineToLogicalLine(position.line, &subline);
	result.column = getLineLayout(result.line).getSublineOffset(subline) + position.column;
	return result;
}
#endif // 0

/**
 * Offsets visual line.
 * @param[in,out] line the logical line
 * @param[in,out] subline the visual subline
 * @param[in] offset the offset
 * @param[out] overflowedOrUnderflowed @c true if absolute value of @a offset is too large so that
 * the results were snapped to the beginning or the end of the document. optional
 */
void LineLayoutBuffer::offsetVisualLine(length_t& line, length_t& subline,
		signed_length_t offset, bool* overflowedOrUnderflowed) const /*throw()*/ {
	if(offset > 0) {
		if(subline + offset < numberOfSublinesOfLine(line))
			subline += offset;
		else {
			const length_t lines = document().numberOfLines();
			offset -= static_cast<signed_length_t>(numberOfSublinesOfLine(line) - subline) - 1;
			while(offset > 0 && line < lines - 1)
				offset -= static_cast<signed_length_t>(numberOfSublinesOfLine(++line));
			subline = numberOfSublinesOfLine(line) - 1;
			if(offset < 0)
				subline += offset;
			if(overflowedOrUnderflowed != 0)
				*overflowedOrUnderflowed = offset > 0;
		}
	} else if(offset < 0) {
		if(static_cast<length_t>(-offset) <= subline)
			subline += offset;
		else {
			offset += static_cast<signed_length_t>(subline);
			while(offset < 0 && line > 0)
				offset += static_cast<signed_length_t>(numberOfSublinesOfLine(--line));
			subline = (offset > 0) ? offset : 0;
			if(overflowedOrUnderflowed != 0)
				*overflowedOrUnderflowed = offset > 0;
		}
	}
}

/// @see presentation#IPresentationStylistListener
void LineLayoutBuffer::presentationStylistChanged() {
	invalidate();
}

/**
 * Sets the new layout information provider.
 * @param newProvider the layout information provider
 * @param delegateOwnership set @c true to transfer the ownership of @a newProvider into the callee
 */
void LineLayoutBuffer::setLayoutInformation(const ILayoutInformationProvider* newProvider, bool delegateOwnership) {
	lip_.reset(newProvider, delegateOwnership);
	invalidate();
}

/**
 * Updates the longest line and invokes @c ILongestLineListener#longestLineChanged.
 * @param line the new longest line. set -1 to recalculate
 * @param width the width of the longest line. if @a line is -1, this value is ignored
 */
void LineLayoutBuffer::updateLongestLine(length_t line, int width) /*throw()*/ {
	if(line != -1) {
		longestLine_ = line;
		longestLineWidth_ = width;
	} else {
		longestLine_ = static_cast<length_t>(-1);
		longestLineWidth_ = 0;
		for(Iterator i(firstCachedLine()), e(lastCachedLine()); i != e; ++i) {
			if((*i)->longestSublineWidth() > longestLineWidth_) {
				longestLine_ = (*i)->lineNumber();
				longestLineWidth_ = (*i)->longestSublineWidth();
			}
		}
	}
}


// ISpecialCharacterRenderer ////////////////////////////////////////////////

/**
 * @class ascension::layout::ISpecialCharacterRenderer
 * Interface for objects which draw special characters.
 *
 * @c ISpecialCharacterRenderer hooks shaping and drawing processes of @c LineLayout about some
 * special characters. These include:
 * - C0 controls
 * - C1 controls
 * - End of line (Line terminators)
 * - White space characters
 * - Line wrapping marks
 *
 * <h2>Characters @c ISpecialCharacterRenderer can render</h2>
 *
 * <em>C0 controls</em> include characters whose code point is U+0000..001F or U+007F. But U+0009,
 * U+000A, and U+000D are excluded. These characters can be found in "White space characters" and
 * "End of line".
 *
 * <em>C1 controls</em> include characters whose code point is U+0080..009F. But only U+0085 is
 * excluded. This is one of "End of line" character.
 *
 * <em>End of line</em> includes any NLFs in Unicode. Identified by @c kernel#Newline enumeration.
 *
 * <em>White space characters</em> include all Unicode white spaces and horizontal tab (U+0009). An
 * instance of @c ISpecialCharacterRenderer can't set the width of these glyphs.
 *
 * <em>Line wrapping marks</em> indicate a logical is wrapped visually. Note that this is not an
 * actual character.
 *
 * <h2>Process</h2>
 *
 * @c ISpecialCharacterRenderer will be invoked at the following two stages.
 * -# To layout a special character.
 * -# To draw a special character.
 *
 * (1) When layout of a line is needed, @c TextRenderer creates and initializes a @c LineLayout.
 * In this process, the widths of the all characters in the line are calculated by Unicode script
 * processor (Uniscribe). For the above special characters, @c LineLayout queries the widths to
 * @c ISpecialCharacterRenderer (However, for white spaces, this query is not performed).
 *
 * (2) When a line is drawn, @c LineLayout#draw calls @c ISpecialCharacterRenderer::drawXxxx
 * methods to draw special characters with the device context, the orientation, and the rectangle
 * to paint.
 *
 * @see TextRenderer, TextRenderer#setSpecialCharacterRenderer
 */


// DefaultSpecialCharacterRenderer //////////////////////////////////////////

namespace {
	inline void getControlPresentationString(CodePoint c, Char* buffer) {
		buffer[0] = L'^';
		buffer[1] = (c != 0x7f) ? static_cast<Char>(c) + 0x40 : L'?';
	}
}

/**
 * @class ascension::layout::DefaultSpecialCharacterRenderer
 *
 * Default implementation of @c ISpecialCharacterRenderer interface. This renders special
 * characters with the glyphs provided by the standard international font "Lucida Sans Unicode".
 * The mapping special characters to characters provide glyphs are as follows:
 * - Horizontal tab (LTR) : U+2192 Rightwards Arrow (&#x2192;)
 * - Horizontal tab (RTL) : U+2190 Leftwards Arrow (&#x2190;)
 * - Line terminator : U+2193 Downwards Arrow (&#x2193;)
 * - Line wrapping mark (LTR) : U+21A9 Leftwards Arrow With Hook (&#x21A9;)
 * - Line wrapping mark (RTL) : U+21AA Rightwards Arrow With Hook (&#x21AA;)
 * - White space : U+00B7 Middle Dot (&#x00B7;)
 *
 * Default foreground colors of glyphs are as follows:
 * - Control characters : RGB(0x80, 0x80, 0x00)
 * - Line terminators : RGB(0x00, 0x80, 0x80)
 * - Line wrapping markers: RGB(0x00, 0x80, 0x80)
 * - White space characters : RGB(0x00, 0x80, 0x80)
 */

/// Default constructor.
DefaultSpecialCharacterRenderer::DefaultSpecialCharacterRenderer() /*throw()*/ : renderer_(0),
		controlColor_(RGB(0x80, 0x80, 0x00)), eolColor_(RGB(0x00, 0x80, 0x80)), wrapMarkColor_(RGB(0x00, 0x80, 0x80)),
		whiteSpaceColor_(RGB(0x00, 0x80, 0x80)), showsEOLs_(true), showsWhiteSpaces_(true), font_(0) {
}

/// Destructor.
DefaultSpecialCharacterRenderer::~DefaultSpecialCharacterRenderer() /*throw()*/ {
	::DeleteObject(font_);
	font_ = 0;
}

/// @see ISpecialCharacterRenderer#drawControlCharacter
void DefaultSpecialCharacterRenderer::drawControlCharacter(const DrawingContext& context, CodePoint c) const {
	tr1::shared_ptr<const AbstractFont> primaryFont(renderer_->primaryFont());
	HFONT oldFont = context.dc.selectObject(primaryFont->handle().get());
	context.dc.setTextColor(controlColor_);
	Char buffer[2];
	getControlPresentationString(c, buffer);
	context.dc.extTextOut(context.rect.left, context.rect.top + primaryFont->metrics().ascent(), 0, 0, buffer, 2, 0);
	context.dc.selectObject(oldFont);
}

/// @see ISpecialCharacterRenderer#drawLineTerminator
void DefaultSpecialCharacterRenderer::drawLineTerminator(const DrawingContext& context, kernel::Newline) const {
	if(showsEOLs_ && glyphs_[LINE_TERMINATOR] != 0xffffu) {
		tr1::shared_ptr<const AbstractFont> primaryFont(renderer_->primaryFont());
		HFONT oldFont = context.dc.selectObject(
			manah::toBoolean(glyphWidths_[LINE_TERMINATOR] & 0x80000000ul) ? font_ : primaryFont->handle().get());
		context.dc.setTextColor(eolColor_);
		context.dc.extTextOut(context.rect.left, context.rect.top + primaryFont->metrics().ascent(),
			ETO_GLYPH_INDEX, 0, reinterpret_cast<const WCHAR*>(&glyphs_[LINE_TERMINATOR]), 1, 0);
		context.dc.selectObject(oldFont);
	}
}

/// @see ISpecialCharacterRenderer#drawLineWrappingMark
void DefaultSpecialCharacterRenderer::drawLineWrappingMark(const DrawingContext& context) const {
	const int id = (context.readingDirection == LEFT_TO_RIGHT) ? LTR_WRAPPING_MARK : RTL_WRAPPING_MARK;
	const WCHAR glyph = glyphs_[id];
	if(glyph != 0xffffu) {
		tr1::shared_ptr<const AbstractFont> primaryFont(renderer_->primaryFont());
		HFONT oldFont = context.dc.selectObject(
			manah::toBoolean(glyphWidths_[id] & 0x80000000ul) ? font_ : primaryFont->handle().get());
		context.dc.setTextColor(wrapMarkColor_);
		context.dc.extTextOut(context.rect.left, context.rect.top + primaryFont->metrics().ascent(), ETO_GLYPH_INDEX, 0, &glyph, 1, 0);
		context.dc.selectObject(oldFont);
	}
}

/// @see ISpecialCharacterRenderer#drawWhiteSpaceCharacter
void DefaultSpecialCharacterRenderer::drawWhiteSpaceCharacter(const DrawingContext& context, CodePoint c) const {
	if(!showsWhiteSpaces_)
		return;
	else if(c == 0x0009u) {
		const int id = (context.readingDirection == LEFT_TO_RIGHT) ? LTR_HORIZONTAL_TAB : RTL_HORIZONTAL_TAB;
		const WCHAR glyph = glyphs_[id];
		if(glyph != 0xffffu) {
			tr1::shared_ptr<const AbstractFont> primaryFont(renderer_->primaryFont());
			HFONT oldFont = context.dc.selectObject(
				manah::toBoolean(glyphWidths_[id] & 0x80000000ul) ? font_ : primaryFont->handle().get());
			const int glyphWidth = glyphWidths_[id] & 0x7ffffffful;
			const int x =
				((context.readingDirection == LEFT_TO_RIGHT && glyphWidth < context.rect.right - context.rect.left)
					|| (context.readingDirection == RIGHT_TO_LEFT && glyphWidth > context.rect.right - context.rect.left)) ?
				context.rect.left : context.rect.right - glyphWidth;
			context.dc.setTextColor(whiteSpaceColor_);
			context.dc.extTextOut(x, context.rect.top + primaryFont->metrics().ascent(), ETO_CLIPPED | ETO_GLYPH_INDEX, &context.rect, &glyph, 1, 0);
			context.dc.selectObject(oldFont);
		}
	} else if(glyphs_[WHITE_SPACE] != 0xffffu) {
		tr1::shared_ptr<const AbstractFont> primaryFont(renderer_->primaryFont());
		HFONT oldFont = context.dc.selectObject(
			manah::toBoolean(glyphWidths_[WHITE_SPACE] & 0x80000000ul) ? font_ : primaryFont->handle().get());
		context.dc.setTextColor(whiteSpaceColor_);
		context.dc.extTextOut((context.rect.left + context.rect.right - (glyphWidths_[WHITE_SPACE] & 0x7ffffffful)) / 2,
			context.rect.top + primaryFont->metrics().ascent(), ETO_CLIPPED | ETO_GLYPH_INDEX, &context.rect,
			reinterpret_cast<const WCHAR*>(&glyphs_[WHITE_SPACE]), 1, 0);
		context.dc.selectObject(oldFont);
	}
}

/// @see IDefaultFontListener#defaultFontChanged
void DefaultSpecialCharacterRenderer::defaultFontChanged() {
	static const Char codes[] = {0x2192u, 0x2190u, 0x2193u, 0x21a9u, 0x21aau, 0x00b7u};

	// using the primary font
	ScreenDC dc;
	HFONT oldFont = dc.selectObject(renderer_->primaryFont()->handle().get());
	dc.getGlyphIndices(codes, MANAH_COUNTOF(codes), glyphs_, GGI_MARK_NONEXISTING_GLYPHS);
	dc.getCharWidthI(glyphs_, MANAH_COUNTOF(codes), glyphWidths_);

	// using the fallback font
	::DeleteObject(font_);
	font_ = 0;
	if(find(glyphs_, MANAH_ENDOF(glyphs_), 0xffffu) != MANAH_ENDOF(glyphs_)) {
		LOGFONTW lf;
		::GetObjectW(renderer_->primaryFont()->handle().get(), sizeof(LOGFONTW), &lf);
		lf.lfWeight = FW_REGULAR;
		lf.lfItalic = lf.lfUnderline = lf.lfStrikeOut = false;
		wcscpy(lf.lfFaceName, L"Lucida Sans Unicode");
		dc.selectObject(font_ = ::CreateFontIndirectW(&lf));
		WORD g[MANAH_COUNTOF(glyphs_)];
		int w[MANAH_COUNTOF(glyphWidths_)];
		dc.getGlyphIndices(codes, MANAH_COUNTOF(codes), g, GGI_MARK_NONEXISTING_GLYPHS);
		dc.getCharWidthI(g, MANAH_COUNTOF(codes), w);
		for(int i = 0; i < MANAH_COUNTOF(glyphs_); ++i) {
			if(glyphs_[i] == 0xffffu) {
				if(g[i] != 0xffff) {
					glyphs_[i] = g[i];
					glyphWidths_[i] = w[i] | 0x80000000ul;
				} else
					glyphWidths_[i] = 0;	// missing
			}
		}
	}

	dc.selectObject(oldFont);
}

/// @see ISpecialCharacterRenderer#getControlCharacterWidth
int DefaultSpecialCharacterRenderer::getControlCharacterWidth(const LayoutContext& context, CodePoint c) const {
	Char buffer[2];
	getControlPresentationString(c, buffer);
	HFONT oldFont = context.dc.selectObject(renderer_->primaryFont()->handle().get());
	const int result = context.dc.getTextExtent(buffer, 2).cx;
	context.dc.selectObject(oldFont);
	return result;
}

/// @see ISpecialCharacterRenderer#getLineTerminatorWidth
int DefaultSpecialCharacterRenderer::getLineTerminatorWidth(const LayoutContext&, kernel::Newline) const {
	return showsEOLs_ ? (glyphWidths_[LINE_TERMINATOR] & 0x7ffffffful) : 0;
}

/// @see ISpecialCharacterRenderer#getLineWrappingMarkWidth
int DefaultSpecialCharacterRenderer::getLineWrappingMarkWidth(const LayoutContext& context) const {
	return glyphWidths_[(context.readingDirection == LEFT_TO_RIGHT) ? LTR_WRAPPING_MARK : RTL_WRAPPING_MARK] & 0x7ffffffful;
}

/// @see ISpecialCharacterRenderer#install
void DefaultSpecialCharacterRenderer::install(TextRenderer& renderer) {
	(renderer_ = &renderer)->addDefaultFontListener(*this);
	defaultFontChanged();
}

/// @see ISpecialCharacterRenderer#uninstall
void DefaultSpecialCharacterRenderer::uninstall() {
	renderer_->removeDefaultFontListener(*this);
	renderer_ = 0;
}


// FontSelector /////////////////////////////////////////////////////////////

namespace {
	manah::AutoBuffer<WCHAR> ASCENSION_FASTCALL mapFontFileNameToTypeface(const WCHAR* fileName) {
		assert(fileName != 0);
		static const WCHAR KEY_NAME[] = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts";
		HKEY key;
		long e = ::RegOpenKeyExW(HKEY_CURRENT_USER, KEY_NAME, 0, KEY_QUERY_VALUE, &key);
		if(e != ERROR_SUCCESS)
			e = ::RegOpenKeyExW(HKEY_LOCAL_MACHINE, KEY_NAME, 0, KEY_QUERY_VALUE, &key);
		if(e == ERROR_SUCCESS) {
			const size_t fileNameLength = wcslen(fileName);
			DWORD maximumValueNameLength, maximumValueBytes;
			e = ::RegQueryInfoKeyW(key, 0, 0, 0, 0, 0, 0, 0, &maximumValueNameLength, &maximumValueBytes, 0, 0);
			if(e == ERROR_SUCCESS && (maximumValueBytes / sizeof(WCHAR)) - 1 >= fileNameLength) {
				const size_t fileNameLength = wcslen(fileName);
				manah::AutoBuffer<WCHAR> valueName(new WCHAR[maximumValueNameLength + 1]);
				manah::AutoBuffer<BYTE> value(new BYTE[maximumValueBytes]);
				DWORD valueNameLength = maximumValueNameLength + 1, valueBytes = maximumValueBytes, type;
				for(DWORD index = 0; ; ++index, valueNameLength = maximumValueNameLength + 1, valueBytes = maximumValueBytes) {
					e = ::RegEnumValueW(key, index, valueName.get(), &valueNameLength, 0, &type, value.get(), &valueBytes);
					if(e == ERROR_SUCCESS) {
						if(type == REG_SZ && (valueBytes / sizeof(WCHAR)) - 1 == fileNameLength
								&& wmemcmp(fileName, reinterpret_cast<WCHAR*>(value.get()), fileNameLength) == 0) {
							::RegCloseKey(key);
							size_t nameLength = valueNameLength;
							if(valueName[nameLength - 1] == L')') {
								if(const WCHAR* const opening = wcsrchr(valueName.get(), L'(')) {
									nameLength = opening - valueName.get();
									if(nameLength > 1 && valueName[nameLength - 1] == L' ')
										--nameLength;
								}
							}
							if(nameLength > 0) {
								manah::AutoBuffer<WCHAR> temp(new WCHAR[nameLength + 1]);
								wmemcpy(temp.get(), valueName.get(), nameLength);
								temp[nameLength] = 0;
								return temp;
							} else
								return manah::AutoBuffer<WCHAR>(0);
						}
					} else	// ERROR_NO_MORE_ITEMS
						break;
				}
			}
			::RegCloseKey(key);
		}
		return manah::AutoBuffer<WCHAR>(0);
	}
} // namespace @0
#if 0
void FontSelector::linkPrimaryFont() /*throw()*/ {
	// TODO: this does not support nested font linking.
	assert(linkedFonts_ != 0);
	for(vector<Fontset*>::iterator i(linkedFonts_->begin()), e(linkedFonts_->end()); i != e; ++i)
		delete *i;
	linkedFonts_->clear();

	// read font link settings from registry
	static const WCHAR KEY_NAME[] = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\FontLink\\SystemLink";
	HKEY key;
	if(ERROR_SUCCESS == ::RegOpenKeyExW(HKEY_CURRENT_USER, KEY_NAME, 0, KEY_QUERY_VALUE, &key)
			|| ERROR_SUCCESS == ::RegOpenKeyExW(HKEY_LOCAL_MACHINE, KEY_NAME, 0, KEY_QUERY_VALUE, &key)) {
		DWORD type, bytes;
		if(ERROR_SUCCESS == ::RegQueryValueExW(key, primaryFont_->faceName, 0, &type, 0, &bytes)) {
			manah::AutoBuffer<BYTE> data(new BYTE[bytes]);
			if(ERROR_SUCCESS == ::RegQueryValueExW(key, primaryFont_->faceName, 0, &type, data.get(), &bytes)) {
				const WCHAR* sz = reinterpret_cast<WCHAR*>(data.get());
				const WCHAR* const e = sz + bytes / sizeof(WCHAR);
				for(; sz < e; sz += wcslen(sz) + 1) {
					const WCHAR* comma = wcschr(sz, L',');
					if(comma != 0 && comma[1] != 0)	// "<file-name>,<typeface>"
						linkedFonts_->push_back(new Fontset(comma + 1));
					else {	// "<file-name>"
						manah::AutoBuffer<WCHAR> typeface(mapFontFileNameToTypeface(sz));
						if(typeface.get() != 0)
							linkedFonts_->push_back(new Fontset(typeface.get()));
					}
				}
			}
		}
		::RegCloseKey(key);
	}
	fireFontChanged();
}
#endif

// TextRenderer /////////////////////////////////////////////////////////////

namespace {
	inline int calculateMemoryBitmapSize(int src) /*throw()*/ {
		const int UNIT = 32;
		return (src % UNIT != 0) ? src + UNIT - src % UNIT : src;
	}
} // namespace @0

/**
 * @class ascension::layout::TextRenderer
 * @c TextRenderer renders styled text to the display or to a printer. Although this class
 * extends @c LineLayoutBuffer class and implements @c ILayoutInformationProvider interface,
 * @c LineLayoutBuffer#deviceContext, @c ILayoutInformationProvider#layoutSettings, and
 * @c ILayoutInformationProvider#width methods are not defined (An internal extension
 * @c TextViewer#Renderer class implements these).
 * @see LineLayout, LineLayoutBuffer, Presentation
 */

/**
 * Constructor.
 * @param presentation the presentation
 * @param fontCollection the font collection provides fonts this renderer uses
 * @param enableDoubleBuffering set @c true to use double-buffering for non-flicker drawing
 */
TextRenderer::TextRenderer(Presentation& presentation, const IFontCollection& fontCollection, bool enableDoubleBuffering) :
		LineLayoutBuffer(presentation.document(), ASCENSION_DEFAULT_LINE_LAYOUT_CACHE_SIZE, true),
		presentation_(presentation), fontCollection_(fontCollection), enablesDoubleBuffering_(enableDoubleBuffering) {
	setLayoutInformation(this, false);
	updateTextMetrics();
/*	switch(PRIMARYLANGID(getUserDefaultUILanguage())) {
	case LANG_CHINESE:
	case LANG_JAPANESE:
	case LANG_KOREAN:
		enableFontLinking();
		break;
	}*/
//	updateViewerSize(); ???
}

/// Copy-constructor.
TextRenderer::TextRenderer(const TextRenderer& other) :
		LineLayoutBuffer(other.presentation_.document(), ASCENSION_DEFAULT_LINE_LAYOUT_CACHE_SIZE, true),
		presentation_(other.presentation_), fontCollection_(other.fontCollection_),
		enablesDoubleBuffering_(other.enablesDoubleBuffering_), primaryFont_() {
	setLayoutInformation(this, false);
	updateTextMetrics();
//	updateViewerSize(); ???
}

/// Destructor.
TextRenderer::~TextRenderer() /*throw()*/ {
//	getTextViewer().removeDisplaySizeListener(*this);
//	layouts_.removeVisualLinesListener(*this);
}

/**
 * Registers the default font selector listener.
 * @param listener the listener to be registered
 * @throw std#invalid_argument @a listener is already registered
 */
void TextRenderer::addDefaultFontListener(IDefaultFontListener& listener) {
	listeners_.add(listener);
}

void TextRenderer::fireDefaultFontChanged() {
	invalidate();
	if(enablesDoubleBuffering_ && memoryBitmap_.get() != 0) {
		BITMAP b;
		memoryBitmap_.getBitmap(b);
		if(b.bmHeight != calculateMemoryBitmapSize(primaryFont()->metrics().linePitch()))
			memoryBitmap_.reset();
	}
	listeners_.notify(&IDefaultFontListener::defaultFontChanged);
}

/// @see ILayoutInformationProvider#fontCollection
const IFontCollection& TextRenderer::fontCollection() const {
	return fontCollection_;
}

/**
 * Returns the indentation of the specified visual line from the left most.
 * @param line the line number
 * @param subline the visual subline number
 * @return the indentation in pixel
 * @throw kernel#BadPositionException @a line is invalid
 * @throw IndexOutOfBoundsException @a subline is invalid
 */
int TextRenderer::lineIndent(length_t line, length_t subline) const {
	const LineLayout& layout = lineLayout(line);
	const TextAlignment resolvedAlignment = resolveTextAlignment(layout.alignment(), layout.readingDirection());
	if(resolvedAlignment == ALIGN_LEFT || resolvedAlignment == JUSTIFY)	// TODO: recognize the last subline of a justified line.
		return 0;
	else {
		int w = width();
		switch(resolvedAlignment) {
		case ALIGN_RIGHT:
			return w - layout.sublineWidth(subline);
		case ALIGN_CENTER:
			return (w - layout.sublineWidth(subline)) / 2;
		default:
			return 0;
		}
	}
}

/// @see ILayoutInformationProvider#presentation
const Presentation& TextRenderer::presentation() const /*throw()*/ {
	return presentation_;
}

/**
 * Removes the default font selector listener.
 * @param listener the listener to be removed
 * @throw std#invalid_argument @a listener is not registered
 */
void TextRenderer::removeDefaultFontListener(IDefaultFontListener& listener) {
	listeners_.remove(listener);
}

/**
 * Renders the specified logical line to the output device.
 * @param line the line number
 * @param dc the device context
 * @param x the x-coordinate of the position to draw
 * @param y the y-coordinate of the position to draw
 * @param paintRect the region to draw
 * @param clipRect the clipping region
 * @param selection the selection
 */
void TextRenderer::renderLine(length_t line, DC& dc, int x, int y,
		const RECT& paintRect, const RECT& clipRect, const LineLayout::Selection* selection) const /*throw()*/ {
	if(!enablesDoubleBuffering_) {
		lineLayout(line).draw(dc, x, y, paintRect, clipRect, selection);
		return;
	}

	const LineLayout& layout = lineLayout(line);
	const int dy = textMetrics().linePitch();

	// skip to the subline needs to draw
	const int top = max(paintRect.top, clipRect.top);
	length_t subline = (y + dy >= top) ? 0 : (top - (y + dy)) / dy;
	if(subline >= layout.numberOfSublines())
		return;	// this logical line does not need to draw
	y += static_cast<int>(dy * subline);

	if(memoryDC_.get() == 0)		
		memoryDC_ = deviceContext().createCompatibleDC();
	const int horizontalResolution = calculateMemoryBitmapSize(dc.getDeviceCaps(HORZRES));
	if(memoryBitmap_.get() != 0) {
		BITMAP b;
		memoryBitmap_.getBitmap(b);
		if(b.bmWidth < horizontalResolution)
			memoryBitmap_.reset();
	}
	if(memoryBitmap_.get() == 0)
		memoryBitmap_ = Bitmap::createCompatibleBitmap(deviceContext(), horizontalResolution, calculateMemoryBitmapSize(dy));
	memoryDC_.selectObject(memoryBitmap_.use());

	const long left = max(paintRect.left, clipRect.left), right = min(paintRect.right, clipRect.right);
	x -= left;
	manah::win32::Rect offsetedPaintRect(paintRect), offsetedClipRect(clipRect);
	offsetedPaintRect.offset(-left, -y);
	offsetedClipRect.offset(-left, -y);
	for(; subline < layout.numberOfSublines() && offsetedPaintRect.bottom >= 0;
			++subline, y += dy, offsetedPaintRect.offset(0, -dy), offsetedClipRect.offset(0, -dy)) {
		layout.draw(subline, memoryDC_, x, 0, offsetedPaintRect, offsetedClipRect, selection);
		dc.bitBlt(left, y, right - left, dy, memoryDC_.get(), 0, 0, SRCCOPY);
	}
}

/**
 * Sets the special character renderer.
 * @param newRenderer the new renderer or @c null
 * @param delegateOwnership set @c true to transfer the ownership into the callee
 * @throw std#invalid_argument @a newRenderer is already registered
 */
void TextRenderer::setSpecialCharacterRenderer(ISpecialCharacterRenderer* newRenderer, bool delegateOwnership) {
	if(newRenderer != 0 && newRenderer == specialCharacterRenderer_.get())
		throw invalid_argument("the specified renderer is already registered.");
	if(specialCharacterRenderer_.get() != 0)
		specialCharacterRenderer_->uninstall();
	specialCharacterRenderer_.reset(newRenderer, delegateOwnership);
	newRenderer->install(*this);
	invalidate();
}

/// @see ILayoutInformationProvider#specialCharacterRenderer
ISpecialCharacterRenderer* TextRenderer::specialCharacterRenderer() const /*throw()*/ {
	return specialCharacterRenderer_.get();
}

bool TextRenderer::updateTextMetrics() {
	// select the primary font
	tr1::shared_ptr<const RunStyle> defaultStyle(presentation_.defaultTextRunStyle());
	String fontFamily;
	FontProperties fontProperties;
	if(defaultStyle.get() != 0 && !defaultStyle->fontFamily.empty()) {
		fontFamily = defaultStyle->fontFamily;
		fontProperties = defaultStyle->fontProperties;
	} else {
		LOGFONTW lf;
		if(::GetObjectW(static_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT)), sizeof(LOGFONTW), &lf) == 0)
			throw runtime_error("");
		fontFamily = lf.lfFaceName;
		fontProperties.weight = static_cast<FontProperties::Weight>(lf.lfWeight);
		fontProperties.style = (lf.lfItalic != 0) ? FontProperties::ITALIC : FontProperties::NORMAL_STYLE;
		fontProperties.size = (lf.lfHeight < 0) ? -lf.lfHeight : 0;
	}
	primaryFont_ = fontCollection().get(fontFamily, fontProperties);
	fireDefaultFontChanged();
	return true;
}


// TextViewer.VerticalRulerDrawer ///////////////////////////////////////////

// TODO: support locale-dependent number format.

using viewers::TextViewer;

HRESULT drawLineNumber(DC& dc, int x, int y, length_t lineNumber/*, const SCRIPT_CONTROL& control, const SCRIPT_STATE& initialState*/) {
	// format number string
	wchar_t s[128];	// oops, is this sufficient?
#if(_MSC_VER < 1400)
	const int length = swprintf(s, L"%lu", lineNumber);
#else
	const int length = swprintf(s, MANAH_COUNTOF(s), L"%lu", lineNumber);
#endif // _MSC_VER < 1400

	UINT option;
#if 0
	if(!ignoreUserOverride)
		option = 0;
	else {
		switch(???) {
			case CONTEXTUAL:
			case NONE:
				option = ETO_NUMERICSLATIN;
				break;
			case FROM_LOCALE:
			case NATIONAL:
			case TRADITIONAL:
				option = ETO_NUMERICSLOCAL;
				break;
		}
	}
#else
	option = 0;
#endif
	dc.extTextOut(x, y, option, 0, s, length, 0);
	return S_OK;
}

/**
 * Draws the vertical ruler.
 * @param dc the device context
 */
void TextViewer::VerticalRulerDrawer::draw(PaintDC& dc) {
	if(width() == 0)
		return;

	const RECT& paintRect = dc.paintStruct().rcPaint;
	const TextRenderer& renderer = viewer_.textRenderer();
	RECT clientRect;
	viewer_.getClientRect(clientRect);
	const TextAlignment alignment = utils::computeVerticalRulerAlignment(viewer_);
	if((alignment == ALIGN_LEFT && paintRect.left >= clientRect.left + width())
			|| (alignment == ALIGN_RIGHT && paintRect.right < clientRect.right - width()))
		return;

#ifdef _DEBUG
	if(DIAGNOSE_INHERENT_DRAWING)
		manah::win32::DumpContext() << L"@VerticalRulerDrawer.draw draws y = " << paintRect.top << L" ~ " << paintRect.bottom << L"\n";
#endif // _DEBUG

	const int savedCookie = dc.save();
	const bool alignLeft = alignment == ALIGN_LEFT;
	const int imWidth = configuration_.indicatorMargin.visible ? configuration_.indicatorMargin.width : 0;

	int left;
	DC* dcex;
	if(enablesDoubleBuffering_) {
		if(memoryDC_.get() == 0)
			memoryDC_ = viewer_.getDC().createCompatibleDC();
		if(memoryBitmap_.get() == 0)
			memoryBitmap_ = Bitmap::createCompatibleBitmap(dc,
				width(), clientRect.bottom - clientRect.top + ::GetSystemMetrics(SM_CYHSCROLL));
		memoryDC_.selectObject(memoryBitmap_.get());
		dcex = &memoryDC_;
		left = 0;
	} else {
		dcex = &dc;
		left = alignLeft ? clientRect.left : clientRect.right - width();
	}
	const int right = left + width();

	// first of all, paint the drawing area
	if(configuration_.indicatorMargin.visible) {
		// border and inside of the indicator margin
		const int borderX = alignLeft ? left + imWidth - 1 : right - imWidth;
		HPEN oldPen = dcex->selectObject(indicatorMarginPen_.use());
		HBRUSH oldBrush = dcex->selectObject(indicatorMarginBrush_.use());
		dcex->patBlt(alignLeft ? left : borderX + 1, paintRect.top, imWidth, paintRect.bottom - paintRect.top, PATCOPY);
		dcex->moveTo(borderX, paintRect.top);
		dcex->lineTo(borderX, paintRect.bottom);
		dcex->selectObject(oldPen);
		dcex->selectObject(oldBrush);
	}
	if(configuration_.lineNumbers.visible) {
		// background of the line numbers
		HBRUSH oldBrush = dcex->selectObject(lineNumbersBrush_.use());
		dcex->patBlt(alignLeft ? left + imWidth : left, paintRect.top, right - imWidth, paintRect.bottom, PATCOPY);
		// border of the line numbers
		if(configuration_.lineNumbers.borderStyle != VerticalRulerConfiguration::LineNumbers::NONE) {
			HPEN oldPen = dcex->selectObject(lineNumbersPen_.use());
			const int x = (alignLeft ? right : left + 1) - configuration_.lineNumbers.borderWidth;
			dcex->moveTo(x, 0/*paintRect.top*/);
			dcex->lineTo(x, paintRect.bottom);
			dcex->selectObject(oldPen);
		}
		dcex->selectObject(oldBrush);

		// for next...
		dcex->setBkMode(TRANSPARENT);
		dcex->setTextColor(systemColors.serve(configuration_.lineNumbers.textColor.foreground, COLOR_WINDOWTEXT));
		dcex->setTextCharacterExtra(0);	// line numbers ignore character extra
		dcex->selectObject(renderer.primaryFont()->handle().get());
	}

	// prepare to draw the line numbers
	ReadingDirection lineNumbersReadingDirection;
	TextAlignment lineNumbersAlignment;
	int lineNumbersX;
	if(configuration_.lineNumbers.visible) {
		// compute reading direction of the line numbers from 'configuration_.lineNumbers.readingDirection'
		switch(configuration_.lineNumbers.readingDirection) {
			case LEFT_TO_RIGHT:
			case RIGHT_TO_LEFT:
				lineNumbersReadingDirection = configuration_.lineNumbers.readingDirection;
				break;
			case INHERIT_READING_DIRECTION: {
				tr1::shared_ptr<const LineStyle> defaultLineStyle(viewer_.presentation().defaultLineStyle());
				if(defaultLineStyle.get() != 0)
					lineNumbersReadingDirection = defaultLineStyle->readingDirection;
				if(lineNumbersReadingDirection == INHERIT_READING_DIRECTION)
					lineNumbersReadingDirection = renderer.defaultUIReadingDirection();
				if(lineNumbersReadingDirection == INHERIT_READING_DIRECTION)
					lineNumbersReadingDirection = ASCENSION_DEFAULT_TEXT_READING_DIRECTION;
				assert(lineNumbersReadingDirection == LEFT_TO_RIGHT || lineNumbersReadingDirection == RIGHT_TO_LEFT);
				break;
			}
			default:
				throw runtime_error("can't resolve reading direction of line numbers in vertical ruler.");
		}
		// compute alignment of the line numbers from 'configuration_.lineNumbers.alignment'
		switch(configuration_.lineNumbers.alignment) {
			case ALIGN_LEFT:
			case ALIGN_RIGHT:
			case ALIGN_CENTER:
			case JUSTIFY:
				lineNumbersAlignment = configuration_.lineNumbers.alignment;
				break;
			case ALIGN_START:
			case ALIGN_END:
				lineNumbersAlignment = resolveTextAlignment(configuration_.lineNumbers.alignment, lineNumbersReadingDirection);
				break;
			default:
				throw runtime_error("can't resolve alignment of line numbers in vertical ruler.");
		}
		switch(lineNumbersAlignment) {
		case ALIGN_LEFT:
			lineNumbersX = alignLeft ?
				left + imWidth + configuration_.lineNumbers.leadingMargin : left + configuration_.lineNumbers.trailingMargin + 1;
			dcex->setTextAlign(TA_LEFT | TA_TOP | TA_NOUPDATECP);
			break;
		case ALIGN_RIGHT:
			lineNumbersX = alignLeft ?
				right - configuration_.lineNumbers.trailingMargin - 1 : right - imWidth - configuration_.lineNumbers.leadingMargin;
			dcex->setTextAlign(TA_RIGHT | TA_TOP | TA_NOUPDATECP);
			break;
		case ALIGN_CENTER:
			lineNumbersX = alignLeft ?
				left + (imWidth + configuration_.lineNumbers.leadingMargin + width() - configuration_.lineNumbers.trailingMargin) / 2
				: right - (width() - configuration_.lineNumbers.trailingMargin + imWidth + configuration_.lineNumbers.leadingMargin) / 2;
			dcex->setTextAlign(TA_CENTER | TA_TOP | TA_NOUPDATECP);
			break;
		}
	}

	// 1 行ずつ細かい描画
	length_t line, visualSublineOffset;
	const length_t lines = viewer_.document().numberOfLines();
	viewer_.mapClientYToLine(paintRect.top, &line, &visualSublineOffset);	// $friendly-access
	if(visualSublineOffset > 0)	// 描画開始は次の論理行から...
		++line;
	int y = viewer_.mapLineToClientY(line, false);
	if(y != 32767 && y != -32768) {
		const int dy = renderer.textMetrics().linePitch();
		while(y < paintRect.bottom && line < lines) {
			const LineLayout& layout = renderer.lineLayout(line);
			const int nextY = y + static_cast<int>(layout.numberOfSublines() * dy);
			if(nextY >= paintRect.top) {
				// 派生クラスにインジケータマージンの描画機会を与える
				if(configuration_.indicatorMargin.visible) {
					RECT rect = {
						alignLeft ? left : right - configuration_.indicatorMargin.width, y,
						alignLeft ? left + configuration_.indicatorMargin.width : right, y + dy
					};
					viewer_.drawIndicatorMargin(line, *dcex, rect);
				}

				// draw line number digits
				if(configuration_.lineNumbers.visible)
					drawLineNumber(*dcex, lineNumbersX, y, line + configuration_.lineNumbers.startValue);
			}
			++line;
			y = nextY;
		}
	}

	if(enablesDoubleBuffering_)
		dc.bitBlt(alignLeft ? clientRect.left : clientRect.right - width(), paintRect.top,
			right - left, paintRect.bottom - paintRect.top, memoryDC_.get(), 0, paintRect.top, SRCCOPY);
	dc.restore(savedCookie);
}

/// Recalculates the width of the vertical ruler.
void TextViewer::VerticalRulerDrawer::recalculateWidth() /*throw()*/ {
	int newWidth = 0;
	if(configuration_.lineNumbers.visible) {
		const uchar newLineNumberDigits = getLineNumberMaxDigits();
		if(newLineNumberDigits != lineNumberDigitsCache_) {
			// the width of the line numbers area is determined by the maximum width of glyphs of 0..9
			ClientDC dc = viewer_.getDC();
			HFONT oldFont = dc.selectObject(viewer_.textRenderer().primaryFont()->handle().get());
			SCRIPT_STRING_ANALYSIS ssa;
			AutoZero<SCRIPT_CONTROL> sc;
			AutoZero<SCRIPT_STATE> ss;
			HRESULT hr;
/*			switch(configuration_.lineNumbers.digitSubstitution) {
			case DST_CONTEXTUAL:
			case DST_NOMINAL:
				break;
			case DST_NATIONAL:
				ss.fDigitSubstitute = 1;
				break;
			case DST_USER_DEFAULT:
*/				hr = ::ScriptApplyDigitSubstitution(&userSettings.digitSubstitution(false), &sc, &ss);
/*				break;
			}
*/			dc.setTextCharacterExtra(0);
			hr = ::ScriptStringAnalyse(dc.use(), L"0123456789", 10,
				estimateNumberOfGlyphs(10), -1, SSA_FALLBACK | SSA_GLYPHS | SSA_LINK, 0, &sc, &ss, 0, 0, 0, &ssa);
			dc.selectObject(oldFont);
			int glyphWidths[10];
			hr = ::ScriptStringGetLogicalWidths(ssa, glyphWidths);
			int maxGlyphWidth = *max_element(glyphWidths, MANAH_ENDOF(glyphWidths));
			lineNumberDigitsCache_ = newLineNumberDigits;
			if(maxGlyphWidth != 0) {
				newWidth += max<uchar>(newLineNumberDigits, configuration_.lineNumbers.minimumDigits) * maxGlyphWidth;
				newWidth += configuration_.lineNumbers.leadingMargin + configuration_.lineNumbers.trailingMargin;
				if(configuration_.lineNumbers.borderStyle != VerticalRulerConfiguration::LineNumbers::NONE)
					newWidth += configuration_.lineNumbers.borderWidth;
			}
		}
	}
	if(configuration_.indicatorMargin.visible)
		newWidth += configuration_.indicatorMargin.width;
	if(newWidth != width_) {
		width_ = newWidth;
		viewer_.invalidateRect(0, false);
		viewer_.updateCaretPosition();
	}
}

///
void TextViewer::VerticalRulerDrawer::updateGDIObjects() /*throw()*/ {
	indicatorMarginPen_.reset();
	indicatorMarginBrush_.reset();
	if(configuration_.indicatorMargin.visible) {
		indicatorMarginPen_ = Pen::create(PS_SOLID, 1,
			systemColors.serve(configuration_.indicatorMargin.borderColor, COLOR_3DSHADOW));
		indicatorMarginBrush_ = Brush::create(systemColors.serve(configuration_.indicatorMargin.color, COLOR_3DFACE));
	}

	lineNumbersPen_.reset();
	lineNumbersBrush_.reset();
	if(configuration_.lineNumbers.visible) {
		if(configuration_.lineNumbers.borderStyle == VerticalRulerConfiguration::LineNumbers::SOLID)	// 実線
			lineNumbersPen_ = Pen::create(PS_SOLID, configuration_.lineNumbers.borderWidth,
				systemColors.serve(configuration_.lineNumbers.borderColor, COLOR_WINDOWTEXT));
		else if(configuration_.lineNumbers.borderStyle != VerticalRulerConfiguration::LineNumbers::NONE) {
			LOGBRUSH brush;
			brush.lbColor = systemColors.serve(configuration_.lineNumbers.borderColor, COLOR_WINDOWTEXT);
			brush.lbStyle = BS_SOLID;
			if(configuration_.lineNumbers.borderStyle == VerticalRulerConfiguration::LineNumbers::DASHED)	// 破線
				lineNumbersPen_ = Pen::create(
					PS_GEOMETRIC | PS_DASH | PS_ENDCAP_FLAT, configuration_.lineNumbers.borderWidth, brush, 0, 0);
			else if(configuration_.lineNumbers.borderStyle == VerticalRulerConfiguration::LineNumbers::DASHED_ROUNDED)	// 丸破線
				lineNumbersPen_ = Pen::create(
					PS_GEOMETRIC | PS_DASH | PS_ENDCAP_ROUND, configuration_.lineNumbers.borderWidth, brush, 0, 0);
			else if(configuration_.lineNumbers.borderStyle == VerticalRulerConfiguration::LineNumbers::DOTTED)	// 点線
				lineNumbersPen_ = Pen::create(PS_GEOMETRIC | PS_DOT, configuration_.lineNumbers.borderWidth, brush, 0, 0);
		}
		lineNumbersBrush_ = Brush::create(systemColors.serve(configuration_.lineNumbers.textColor.background, COLOR_WINDOW));
	}
}
