/**
 * @file layout.cpp
 * @author exeal
 * @date 2003-2006 (was LineLayout.cpp)
 * @date 2006-2010
 */

#include <ascension/config.hpp>			// ASCENSION_DEFAULT_LINE_LAYOUT_CACHE_SIZE, ...
#include <ascension/viewer/caret.hpp>	// Caret.isSelectionRectangle, viewers.selectedRangeOnVisualLine
#include <ascension/layout.hpp>
#include <limits>	// std.numeric_limits
#include <numeric>	// std.accumulate
#include <usp10.h>

using namespace ascension;
using namespace ascension::graphics;
using namespace ascension::graphics::font;
using namespace ascension::presentation;
using namespace ascension::text;
using namespace ascension::text::ucd;
using namespace std;
namespace k = ascension::kernel;

#pragma comment(lib, "usp10.lib")

//#define TRACE_LAYOUT_CACHES
extern bool DIAGNOSE_INHERENT_DRAWING;

namespace {
	template<typename Coordinate> inline RECT toNative(const Rect<Coordinate>& source) {
		RECT temp;
		::SetRect(&temp, source.x().beginning(), source.y().beginning(), source.x().end(), source.y().end());
		return temp;
	}
}

namespace {
	// SystemColors caches the system colors.
	class SystemColors {
	public:
		SystemColors() /*throw()*/ {update();}
		COLORREF get(int index) const {assert(index >= 0 && index < ASCENSION_COUNTOF(c_)); return c_[index];}
		COLORREF serve(const Color& color, int index) const {return (color != Color()) ? color.asCOLORREF() : get(index);}
		void update() /*throw()*/ {for(int i = 0; i < ASCENSION_COUNTOF(c_); ++i) c_[i] = ::GetSysColor(i);}
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
	int pixels(const Context& context, const Length& length, bool vertical, const Font::Metrics& fontMetrics);
	HRESULT resolveNumberSubstitution(const NumberSubstitution* configuration, SCRIPT_CONTROL& sc, SCRIPT_STATE& ss);
	bool uniscribeSupportsVSS() /*throw()*/;
	LANGID userCJKLanguage() /*throw()*/;

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
	ASCENSION_SHARED_LIB_ENTRY(Uniscribe16, 0, "ScriptItemizeOpenType",
		HRESULT(WINAPI* signature)(
			const WCHAR*, int, int, const SCRIPT_CONTROL*, const SCRIPT_STATE*, SCRIPT_ITEM*,
			OPENTYPE_TAG*, int*));
	ASCENSION_SHARED_LIB_ENTRY(Uniscribe16, 1, "ScriptPlaceOpenType",
		HRESULT(WINAPI* signature)(
			HDC, SCRIPT_CACHE*, SCRIPT_ANALYSIS*, OPENTYPE_TAG, OPENTYPE_TAG, int*,
			TEXTRANGE_PROPERTIES**, int, const WCHAR*, WORD*, SCRIPT_CHARPROP*, int, const WORD*,
			const SCRIPT_GLYPHPROP*, int, int*, GOFFSET*, ABC*));
	ASCENSION_SHARED_LIB_ENTRY(Uniscribe16, 2, "ScriptShapeOpenType",
		HRESULT(WINAPI* signature)(
			HDC, SCRIPT_CACHE*, SCRIPT_ANALYSIS*, OPENTYPE_TAG, OPENTYPE_TAG, int*,
			TEXTRANGE_PROPERTIES**, int, const WCHAR*, int, int, WORD*, SCRIPT_CHARPROP*, WORD*,
			SCRIPT_GLYPHPROP*, int*));
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
					win32::Handle<HDC> dc(::GetDC(0), bind1st(ptr_fun(&::ReleaseDC), static_cast<HWND>(0)));
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

	int pixels(const Context& context, const Length& length, bool vertical, const Font::Metrics& fontMetrics) {
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
				const double dpi = vertical ? context.device()->logicalDpiY() : context.device()->logicalDpiX();
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
			if(SUCCEEDED(::ScriptItemize(text, ASCENSION_COUNTOF(text) - 1,
					ASCENSION_COUNTOF(items), 0, 0, items, &numberOfItems)) && numberOfItems == 1)
				supports = true;
			checked = true;
		}
		return supports;
	}

	LANGID userCJKLanguage() /*throw()*/ {
		// this code is preliminary...
		static const WORD CJK_LANGUAGES[] = {LANG_CHINESE, LANG_JAPANESE, LANG_KOREAN};	// sorted by numeric values
		LANGID result = win32::userDefaultUILanguage();
		if(find(CJK_LANGUAGES, ASCENSION_ENDOF(CJK_LANGUAGES), PRIMARYLANGID(result)) != ASCENSION_ENDOF(CJK_LANGUAGES))
			return result;
		result = ::GetUserDefaultLangID();
		if(find(CJK_LANGUAGES, ASCENSION_ENDOF(CJK_LANGUAGES), PRIMARYLANGID(result)) != ASCENSION_ENDOF(CJK_LANGUAGES))
			return result;
		result = ::GetSystemDefaultLangID();
		if(find(CJK_LANGUAGES, ASCENSION_ENDOF(CJK_LANGUAGES), PRIMARYLANGID(result)) != ASCENSION_ENDOF(CJK_LANGUAGES))
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
bool graphics::getDecorationLineMetrics(const win32::Handle<HDC>& dc, int* baselineOffset,
		int* underlineOffset, int* underlineThickness, int* strikethroughOffset, int* strikethroughThickness) /*throw()*/ {
	OUTLINETEXTMETRICW* otm = 0;
	TEXTMETRICW tm;
	if(const UINT c = ::GetOutlineTextMetricsW(dc.use(), 0, 0)) {
		otm = static_cast<OUTLINETEXTMETRICW*>(::operator new(c));
		if(!manah::toBoolean(::GetOutlineTextMetricsW(dc.get(), c, otm)))
			return false;
	} else if(!manah::toBoolean(::GetTextMetricsW(dc.get(), &tm)))
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
bool graphics::supportsComplexScripts() /*throw()*/ {
	return true;
}

/// Returns @c true if OpenType features are supported.
bool graphics::supportsOpenTypeFeatures() /*throw()*/ {
	return uspLib->get<0>() != 0;
}


// LineLayout.TextRun /////////////////////////////////////////////////////////////////////////////

// Uniscribe conventions
namespace {
	inline size_t characterPositionToGlyphPosition(const WORD clusters[],
			size_t length, size_t numberOfGlyphs, size_t at, const SCRIPT_ANALYSIS& a) {
		assert(a.fLogicalOrder == 0 && at <= length);
		if(a.fRTL == 0)	// LTR
			return (at < length) ? clusters[at] : numberOfGlyphs;
		else	// RTL
			return (at < length) ? clusters[at] + 1 : 0;
	}
	inline bool overhangs(const ABC& width) /*throw()*/ {return width.abcA < 0 || width.abcC < 0;}
} // namespace

namespace {
	class SimpleStyledRunIterator : public IStyledRunIterator {
	public:
		SimpleStyledRunIterator(const Range<const StyledRun*>& range, length_t start) : range_(range) {
			current_ = range_.beginning() + ascension::internal::searchBound(
				static_cast<ptrdiff_t>(0), range_.length(), start, BeginningOfStyledRun(range_.beginning()));
		}
		// presentation.IStyledRunIterator
		void current(StyledRun& run) const {
			if(!hasNext())
				throw IllegalStateException("");
			run = *current_;
		}
		bool hasNext() const {
			return current_ != range_.end();
		}
		void next() {
			if(!hasNext())
				throw IllegalStateException("");
			++current_;
		}
	private:
		struct BeginningOfStyledRun {
			explicit BeginningOfStyledRun(const StyledRun* range) : range(range) {}
			length_t operator()(ptrdiff_t i) {return range[i].column;}
			const StyledRun* range;
		};
		const Range<const StyledRun*> range_;
		const StyledRun* current_;
	};
}

/// TextRun class represents minimum text run whose characters can shaped by single font.
class LineLayout::TextRun : public Range<length_t> {	// beginning() and end() return position in the line
	ASCENSION_NONCOPYABLE_TAG(TextRun);
public:
	struct Overlay {
		Color color;
		Range<length_t> range;
	};
public:
	TextRun(const Range<length_t>& characterRange, const SCRIPT_ANALYSIS& script,
		tr1::shared_ptr<const Font> font, OPENTYPE_TAG scriptTag) /*throw()*/;
	virtual ~TextRun() /*throw()*/;
	uchar bidiEmbeddingLevel() const /*throw()*/ {return static_cast<uchar>(analysis_.s.uBidiLevel);}
	auto_ptr<TextRun> breakAt(Context& context, length_t at,	// 'at' is from the beginning of the line
		const String& lineString, const ILayoutInformationProvider& lip);
	void blackBoxBounds(const Range<length_t>& range, Rect<>& bounds) const;
	void drawBackground(Context& context, const Point<>& p,
		const Range<length_t>& range, const Color& color, const Rect<>* dirtyRect, Rect<>* bounds) const;
	void drawForeground(Context& context, const Point<>& p,
		const Range<length_t>& range, const Color& color, const Rect<>* dirtyRect, const Overlay* overlay) const;
	bool expandTabCharacters(const String& lineString, int x, int tabWidth, int maximumWidth);
	tr1::shared_ptr<const Font> font() const {return glyphs_->font;}
	HRESULT hitTest(int x, int& cp, int& trailing) const;
	HRESULT justify(int width);
	HRESULT logicalAttributes(const String& lineString, SCRIPT_LOGATTR attributes[]) const;
	HRESULT logicalWidths(int widths[]) const;
	static void mergeScriptsAndStyles(Context& context, const String& lineString, const SCRIPT_ITEM scriptRuns[],
		const OPENTYPE_TAG scriptTags[], size_t numberOfScriptRuns, auto_ptr<IStyledRunIterator> styles,
		const ILayoutInformationProvider& lip, vector<TextRun*>& textRuns, vector<const StyledRun>& styledRanges);
	int numberOfGlyphs() const /*throw()*/ {return glyphRange_.length();}
	void positionGlyphs(const Context& context, const String& lineString, SimpleStyledRunIterator& styles);
	ReadingDirection readingDirection() const /*throw()*/ {
		return ((analysis_.s.uBidiLevel & 0x01) == 0x00) ? LEFT_TO_RIGHT : RIGHT_TO_LEFT;}
	void shape(Context& context, const String& lineString, const ILayoutInformationProvider& lip);
	auto_ptr<TextRun> splitIfTooLong(const String& lineString);
	static void substituteGlyphs(const Context& context, const Range<TextRun**>& runs, const String& lineString);
	int totalWidth() const /*throw()*/ {return accumulate(advances(), advances() + numberOfGlyphs(), 0);}
	int x(length_t at, bool trailing) const;
private:
	struct Glyphs {	// this data is shared text runs separated by (only) line breaks
		const Range<length_t> characters;	// character range for this glyph arrays in the line
		const tr1::shared_ptr<const Font> font;
		const OPENTYPE_TAG scriptTag;
		mutable SCRIPT_CACHE fontCache;
		// only 'clusters' is character-base. others are glyph-base
		AutoBuffer<WORD> indices, clusters;
		AutoBuffer<SCRIPT_VISATTR> visualAttributes;
		AutoBuffer<int> advances, justifiedAdvances;
		AutoBuffer<GOFFSET> offsets;
		Glyphs(const Range<length_t>& characters, tr1::shared_ptr<const Font> font,
				OPENTYPE_TAG scriptTag) : characters(characters), font(font), scriptTag(scriptTag), fontCache(0) {
			if(font.get() == 0)
				throw NullPointerException("font");
		}
		~Glyphs() /*throw()*/ {::ScriptFreeCache(&fontCache);}
		void vanish(const Context& context, size_t at);	// 'at' is distance from the beginning of this run
	};
private:
	TextRun(TextRun& leading, length_t characterBoundary) /*throw()*/;
	const int* advances() const /*throw()*/ {
		if(const int* const p = glyphs_->advances.get()) return p + glyphRange_.beginning(); return 0;}
	const WORD* clusters() const /*throw()*/ {
		if(const WORD* const p = glyphs_->clusters.get())
			return p + (beginning() - glyphs_->characters.beginning()); return 0;}
	pair<int, HRESULT> countMissingGlyphs(const Context& context, const Char* text) const /*throw()*/;
	static HRESULT generateGlyphs(const Context& context, const StringPiece& text,
		const SCRIPT_ANALYSIS& analysis, Glyphs& glyphs, int& numberOfGlyphs);
	static void generateDefaultGlyphs(const Context& context,
		const StringPiece& text, const SCRIPT_ANALYSIS& analysis, Glyphs& glyphs);
	const WORD* glyphs() const /*throw()*/ {
		if(const WORD* const p = glyphs_->indices.get()) return p + glyphRange_.beginning(); return 0;}
	const GOFFSET* glyphOffsets() const /*throw()*/ {
		if(const GOFFSET* const p = glyphs_->offsets.get()) return p + glyphRange_.beginning(); return 0;}
	const int* justifiedAdvances() const /*throw()*/ {
		if(const int* const p = glyphs_->justifiedAdvances.get()) return p + glyphRange_.beginning(); return 0;}
	const SCRIPT_VISATTR* visualAttributes() const /*throw()*/ {
		if(const SCRIPT_VISATTR* const p = glyphs_->visualAttributes.get()) return p + glyphRange_.beginning(); return 0;}
private:
	SCRIPT_ANALYSIS analysis_;	// fLogicalOrder member is always 0 (however see shape())
	tr1::shared_ptr<Glyphs> glyphs_;
	Range<WORD> glyphRange_;	// range of this run in 'glyphs_' arrays
	int width_ : sizeof(int) - 1;
	bool mayOverhang_ : 1;
};

void LineLayout::TextRun::Glyphs::vanish(const Context& context, size_t at) {
	assert(advances.get() == 0);
	WORD blankGlyph;
	HRESULT hr = ::ScriptGetCMap(context.engine()->nativeHandle().get(), &fontCache, L"\x0020", 1, 0, &blankGlyph);
	if(hr == S_OK) {
		SCRIPT_FONTPROPERTIES fp;
		fp.cBytes = sizeof(SCRIPT_FONTPROPERTIES);
		if(FAILED(hr = ::ScriptGetFontProperties(context.engine()->nativeHandle().get(), &fontCache, &fp)))
			fp.wgBlank = 0;	/* hmm... */
		blankGlyph = fp.wgBlank;
	}
	indices[clusters[at]] = indices[clusters[at + 1]] = blankGlyph;
	SCRIPT_VISATTR* const va = visualAttributes.get();
	va[clusters[at]].uJustification = SCRIPT_JUSTIFY_BLANK;
	va[clusters[at]].fZeroWidth = 1;
}

/**
 * Constructor.
 * @param characterRange the character range this text run covers
 * @param script @c SCRIPT_ANALYSIS object obtained by @c ScriptItemize(OpenType)
 * @param font the font renders this text run. can't be @c null
 * @param scriptTag an OpenType script tag describes the script of this text run
 * @throw NullPointerException @a font is @c null
 */
LineLayout::TextRun::TextRun(const Range<length_t>& characterRange,
		const SCRIPT_ANALYSIS& script, tr1::shared_ptr<const Font> font, OPENTYPE_TAG scriptTag) /*throw()*/
		: Range<length_t>(characterRange), analysis_(script) {
	glyphs_.reset(new Glyphs(*this, font, scriptTag));
	if(font.get() == 0)
		throw NullPointerException("font");
}

/**
 * Another private constructor separates an existing text run.
 * @param leading the original text run
 * @param characterBoundary
 * @throw std#invalid_argument @a leading has not been shaped
 * @throw std#out_of_range @a characterBoundary is outside of the character range @a leading covers
 * @see #splitIfTooLong
 */
LineLayout::TextRun::TextRun(TextRun& leading, length_t characterBoundary) /*throw()*/ :
		Range<length_t>(characterBoundary, leading.end()), analysis_(leading.analysis_), glyphs_(leading.glyphs_) {
	if(leading.glyphs_.get() == 0)
		throw invalid_argument("leading has not been shaped");
	if(characterBoundary >= leading.length())
		throw out_of_range("firstCharacter");

	// compute 'glyphRange_'

	// modify clusters
//	TextRun& target = ltr ? *this : leading;
//	WORD* const clusters = glyphs_->clusters.get();
//	transform(target.clusters(), target.clusters() + target.length(),
//		clusters + target.beginning(), bind2nd(minus<WORD>(), clusters[ltr ? target.beginning() : (target.end() - 1)]));
}

LineLayout::TextRun::~TextRun() /*throw()*/ {
//	if(cache_ != 0)
//		::ScriptFreeCache(&cache_);
}

void LineLayout::TextRun::blackBoxBounds(const Range<length_t>& range, Rect<>& bounds) const {
	const int left = x(max(range.beginning(), beginning()), false);
	const int right = x(min(range.end(), end()) - 1, true);
	const Font::Metrics& fontMetrics = glyphs_->font->metrics();
	bounds = Rect<>(
		Point<>(left, -fontMetrics.ascent()),
		Dimension<>(right - left, fontMetrics.cellHeight())).normalize();
}

auto_ptr<LineLayout::TextRun> LineLayout::TextRun::breakAt(
		Context& context, length_t at, const String& lineString, const ILayoutInformationProvider& lip) {
	assert(at > beginning() && at < end());
	assert(glyphs_->clusters[at - beginning()] != glyphs_->clusters[at - beginning() - 1]);

	const bool ltr = readingDirection() == LEFT_TO_RIGHT;
	const length_t newLength = at - beginning();
	assert(ltr == (analysis_.fRTL == 0));

	// create the new following run
	auto_ptr<TextRun> following(new TextRun(*this, newLength));

	// update placements
//	place(context, lineString, lip);
//	following->place(dc, lineString, lip);

	return following;
}

/**
 * Returns the number of missing glyphs in this run.
 * @param dc the device context
 * @param run the run
 * @return the number of missing glyphs
 */
inline pair<int, HRESULT> LineLayout::TextRun::countMissingGlyphs(
		const Context& context, const Char* text) const /*throw()*/ {
	SCRIPT_FONTPROPERTIES fp;
	fp.cBytes = sizeof(SCRIPT_FONTPROPERTIES);
	const HRESULT hr = ::ScriptGetFontProperties(context.engine()->nativeHandle().get(), &glyphs_->fontCache, &fp);
	if(FAILED(hr))
		return make_pair(0, hr);	// can't handle
	// following is not offical way, but from Mozilla (gfxWindowsFonts.cpp)
	int result = 0;
	for(StringCharacterIterator i(StringPiece(text + beginning(), length())); i.hasNext(); i.next()) {
		if(!BinaryProperty::is<BinaryProperty::DEFAULT_IGNORABLE_CODE_POINT>(i.current())) {
			const WORD glyph = glyphs_->indices[glyphs_->clusters[i.tell() - i.beginning()]];
			if(glyph == fp.wgDefault || (glyph == fp.wgInvalid && glyph != fp.wgBlank))
				++result;
			else if(glyphs_->visualAttributes[i.tell() - i.beginning()].fZeroWidth == 1
					&& scriptProperties.get(analysis_.eScript).fComplex == 0)
				++result;
		}
	}
	return make_pair(result, S_OK);
}

/**
 * Paints the background of the specified character range in this run.
 * @param context the graphics context
 * @param p the base point of this run (, does not corresponds to @c range.beginning())
 * @param range the character range to paint. if the edges addressed outside of this run, they are
 *              truncated
 * @param color the background color. should be valid
 * @param dirtyRect can be @c null
 * @param[out] bounds the rectangle this method painted. can be @c null
 * @throw std#invalid_argument @a color is not valid
 */
void LineLayout::TextRun::drawBackground(Context& context, const Point<>& p,
		const Range<length_t>& range, const Color& color, const Rect<>* dirtyRect, Rect<>* bounds) const {
	if(color == Color())
		throw invalid_argument("color");
	if(range.isEmpty() || (dirtyRect != 0 && p.x + totalWidth() < dirtyRect->x().beginning()))
		return;
	Rect<> r;
	blackBoxBounds(range, r);
	context.fillRectangle(r.translate(p), color);
	if(bounds != 0)
		*bounds = r;
}

void LineLayout::TextRun::drawForeground(Context& context, const Point<>& p,
		const Range<length_t>& range, const Color& color, const Rect<>* dirtyRect, const Overlay* overlay) const {
	const Range<length_t> truncatedRange(max(range.beginning(), beginning()), min(range.end(), end()));
	if(truncatedRange.isEmpty())
		return;
	const Range<size_t> glyphRange(
		characterPositionToGlyphPosition(clusters(), length(), numberOfGlyphs(), truncatedRange.beginning() - beginning(), analysis_),
		characterPositionToGlyphPosition(clusters(), length(), numberOfGlyphs(), truncatedRange.end() - beginning(), analysis_));
	if(!glyphRange.isEmpty()) {
		context.setFont(*glyphs_->font);
		context.setBackgroundMode(Context::TRANSPARENT_MODE);
		::SetTextColor(context.engine()->nativeHandle().get(), color.asCOLORREF());
		RECT temp;
		if(dirtyRect != 0)
			::SetRect(&temp, dirtyRect->x().beginning(), dirtyRect->y().beginning(), dirtyRect->x().end(), dirtyRect->y().end());
		const HRESULT hr = ::ScriptTextOut(context.engine()->nativeHandle().get(), &glyphs_->fontCache,
			p.x + x((analysis_.fRTL == 0) ? truncatedRange.beginning() : (truncatedRange.end() - 1), analysis_.fRTL != 0),
			p.y - glyphs_->font->metrics().ascent(), 0, (dirtyRect != 0) ? &temp : 0, &analysis_, 0, 0,
			glyphs() + glyphRange.beginning(), glyphRange.length(), advances() + glyphRange.beginning(),
			(justifiedAdvances() != 0) ? justifiedAdvances() + glyphRange.beginning() : 0,
			glyphOffsets() + glyphRange.beginning());
	}
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
inline bool LineLayout::TextRun::expandTabCharacters(const String& lineString, int x, int tabWidth, int maximumWidth) {
	if(maximumWidth <= 0)
		throw invalid_argument("maximumWidth");
	if(lineString[beginning()] != L'\t')
		return false;
	assert(length() == 1 && glyphs_.unique());
	glyphs_->advances[0] = min(tabWidth - x % tabWidth, maximumWidth);
	glyphs_->justifiedAdvances.reset();
	return true;
}

/// Fills the glyph array with default index, instead of using @c ScriptShape.
inline void LineLayout::TextRun::generateDefaultGlyphs(const Context& context,
		const StringPiece& text, const SCRIPT_ANALYSIS& analysis, Glyphs& glyphs) {
	SCRIPT_CACHE fontCache(0);
	SCRIPT_FONTPROPERTIES fp;
	fp.cBytes = sizeof(SCRIPT_FONTPROPERTIES);
	if(FAILED(::ScriptGetFontProperties(context.engine()->nativeHandle().get(), &fontCache, &fp)))
		fp.wgDefault = 0;	// hmm...

	AutoBuffer<WORD> indices, clusters;
	AutoBuffer<SCRIPT_VISATTR> visualAttributes;
	const int numberOfGlyphs = static_cast<int>(text.length());
	indices.reset(new WORD[numberOfGlyphs]);
	clusters.reset(new WORD[text.length()]);
	visualAttributes.reset(new SCRIPT_VISATTR[numberOfGlyphs]);
	fill_n(indices.get(), numberOfGlyphs, fp.wgDefault);
	const bool ltr = analysis.fRTL == 0 || analysis.fLogicalOrder == 1;
	for(size_t i = 0, c = text.length(); i < c; ++i)
		clusters[i] = static_cast<WORD>(ltr ? i : (c - i));
	const SCRIPT_VISATTR va = {SCRIPT_JUSTIFY_NONE, 1, 0, 0, 0, 0};
	fill_n(visualAttributes.get(), numberOfGlyphs, va);

	// commit
	std::swap(glyphs.fontCache, fontCache);
	std::swap(glyphs.indices, indices);
	std::swap(glyphs.clusters, clusters);
	std::swap(glyphs.visualAttributes, visualAttributes);
	::ScriptFreeCache(&fontCache);
}

/**
 * Generates glyphs for the text.
 * @param context the graphics context
 * @param text the text to generate glyphs
 * @param analysis the Uniscribe's @c SCRIPT_ANALYSIS object
 * @param[out] glyphs
 * @param[out] numberOfGlyphs the number of glyphs written to @a run
 * @retval S_OK succeeded
 * @retval USP_E_SCRIPT_NOT_IN_FONT the font does not support the required script
 * @retval E_INVALIDARG other Uniscribe error. usually, too long run was specified
 * @retval HRESULT other Uniscribe error
 * @throw std#bad_alloc failed to allocate buffer for glyph indices or visual attributes array
 */
HRESULT LineLayout::TextRun::generateGlyphs(const Context& context,
		const StringPiece& text, const SCRIPT_ANALYSIS& analysis, Glyphs& glyphs, int& numberOfGlyphs) {
#ifdef _DEBUG
	if(HFONT currentFont = static_cast<HFONT>(::GetCurrentObject(context.engine()->nativeHandle().get(), OBJ_FONT))) {
		LOGFONTW lf;
		if(::GetObjectW(currentFont, sizeof(LOGFONTW), &lf) > 0) {
			win32::DumpContext dout;
			dout << L"[LineLayout.TextRun.generateGlyphs] Selected font is '" << lf.lfFaceName << L"'.\n";
		}
	}
#endif

	SCRIPT_CACHE fontCache(0);	// TODO: this object should belong to a font, not glyph run???
	AutoBuffer<WORD> indices, clusters;
	AutoBuffer<SCRIPT_VISATTR> visualAttributes;
	clusters.reset(new WORD[text.length()]);
	numberOfGlyphs = estimateNumberOfGlyphs(text.length());
	HRESULT hr;
	while(true) {
		indices.reset(new WORD[numberOfGlyphs]);
		visualAttributes.reset(new SCRIPT_VISATTR[numberOfGlyphs]);
		hr = ::ScriptShape(context.engine()->nativeHandle().get(), &fontCache,
			text.beginning(), static_cast<int>(text.length()),
			numberOfGlyphs, const_cast<SCRIPT_ANALYSIS*>(&analysis),
			indices.get(), clusters.get(), visualAttributes.get(), &numberOfGlyphs);
		if(hr != E_OUTOFMEMORY)
			break;
		// repeat until a large enough buffer is provided
		numberOfGlyphs *= 2;
	}

	if(analysis.fNoGlyphIndex != 0)
		hr = GDI_ERROR;	// the caller should try other fonts or disable shaping

	// commit
	if(SUCCEEDED(hr)) {
		std::swap(glyphs.fontCache, fontCache);
		std::swap(glyphs.indices, indices);
		std::swap(glyphs.clusters, clusters);
		std::swap(glyphs.visualAttributes, visualAttributes);
	}
	::ScriptFreeCache(&fontCache);
	return hr;
}

inline HRESULT LineLayout::TextRun::hitTest(int x, int& cp, int& trailing) const {
	return ::ScriptXtoCP(x, static_cast<int>(length()), numberOfGlyphs(), clusters(),
		visualAttributes(), (justifiedAdvances() == 0) ? advances() : justifiedAdvances(), &analysis_, &cp, &trailing);
}

inline HRESULT LineLayout::TextRun::justify(int width) {
	assert(glyphs_->indices.get() != 0 && advances() != 0);
	HRESULT hr = S_OK;
	if(width != totalWidth()) {
		if(glyphs_->justifiedAdvances.get() == 0)
			glyphs_->justifiedAdvances.reset(new int[numberOfGlyphs()]);
		hr = ::ScriptJustify(visualAttributes(), advances(), numberOfGlyphs(), width - totalWidth(),
			2, glyphs_->justifiedAdvances.get() + (beginning() - glyphs_->characters.beginning()));
	}
	return hr;
}

inline HRESULT LineLayout::TextRun::logicalAttributes(const String& lineString, SCRIPT_LOGATTR attributes[]) const {
	if(attributes == 0)
		throw NullPointerException("attributes");
	return ::ScriptBreak(lineString.data() + beginning(), static_cast<int>(length()), &analysis_, attributes);
}

inline HRESULT LineLayout::TextRun::logicalWidths(int widths[]) const {
	if(widths == 0)
		throw NullPointerException("widths");
	return ::ScriptGetLogicalWidths(&analysis_, static_cast<int>(length()),
		numberOfGlyphs(), advances(), clusters(), visualAttributes(), widths);
}

namespace {
	void resolveFontSpecifications(const ILayoutInformationProvider& lip,
			tr1::shared_ptr<const RunStyle> requestedStyle, String& computedFamilyName,
			FontProperties& computedProperties, double& computedSizeAdjust) {
		tr1::shared_ptr<const RunStyle> defaultStyle(lip.presentation().defaultTextRunStyle());
		// family name
		computedFamilyName = (requestedStyle.get() != 0) ? requestedStyle->fontFamily : String();
		if(computedFamilyName.empty()) {
			if(defaultStyle.get() != 0)
				computedFamilyName = lip.presentation().defaultTextRunStyle()->fontFamily;
			if(computedFamilyName.empty())
				computedFamilyName = lip.textMetrics().familyName();
		}
		// properties
		computedProperties = (requestedStyle.get() != 0) ? requestedStyle->fontProperties : FontProperties();
		if(computedProperties.weight == FontProperties::INHERIT_WEIGHT)
			computedProperties.weight = (defaultStyle.get() != 0) ? defaultStyle->fontProperties.weight : FontProperties::NORMAL_WEIGHT;
		if(computedProperties.stretch == FontProperties::INHERIT_STRETCH)
			computedProperties.stretch = (defaultStyle.get() != 0) ? defaultStyle->fontProperties.stretch : FontProperties::NORMAL_STRETCH;
		if(computedProperties.style == FontProperties::INHERIT_STYLE)
			computedProperties.style = (defaultStyle.get() != 0) ? defaultStyle->fontProperties.style : FontProperties::NORMAL_STYLE;
		if(computedProperties.size == 0.0f) {
			if(defaultStyle.get() != 0)
				computedProperties.size = defaultStyle->fontProperties.size;
			if(computedProperties.size == 0.0f)
				computedProperties.size = lip.textMetrics().emHeight();
		}
		// size-adjust
		computedSizeAdjust = (requestedStyle.get() != 0) ? requestedStyle->fontSizeAdjust : -1.0;
		if(computedSizeAdjust < 0.0)
			computedSizeAdjust = (defaultStyle.get() != 0) ? defaultStyle->fontSizeAdjust : 0.0;
	}
	pair<const Char*, tr1::shared_ptr<const Font> > findNextFontRun(
			const Range<const Char*>& text, tr1::shared_ptr<const RunStyle> requestedStyle,
			tr1::shared_ptr<const Font> previousFont, const ILayoutInformationProvider& lip) {
		String familyName;
		FontProperties properties;
		double sizeAdjust;
		resolveFontSpecifications(lip, requestedStyle, familyName, properties, sizeAdjust);
		familyName.assign(L"Times New Roman");
//		properties.style = FontProperties::ITALIC;
		return make_pair(static_cast<const Char*>(0), lip.fontCollection().get(familyName, properties, sizeAdjust));
	}
} // namespace @0

/**
 * Merges the given item runs and the given style runs.
 * @param context the graphics context
 * @param lineString
 * @param items the items itemized by @c #itemize()
 * @param numberOfItems the length of the array @a items
 * @param styles the iterator returns the styled runs in the line. can be @c null
 * @param lip
 * @param[out] textRuns
 * @param[out] styledRanges
 * @see presentation#Presentation#getLineStyle
 */
void LineLayout::TextRun::mergeScriptsAndStyles(Context& context, const String& lineString, const SCRIPT_ITEM scriptRuns[],
		const OPENTYPE_TAG scriptTags[], size_t numberOfScriptRuns, auto_ptr<IStyledRunIterator> styles,
		const ILayoutInformationProvider& lip, vector<TextRun*>& textRuns, vector<const StyledRun>& styledRanges) {
	if(scriptRuns == 0)
		throw NullPointerException("scriptRuns");
	else if(numberOfScriptRuns == 0)
		throw invalid_argument("numberOfScriptRuns");

#define ASCENSION_SPLIT_LAST_RUN()										\
	while(runs.back()->length() > MAXIMUM_RUN_LENGTH) {					\
		TextRun& back = *runs.back();									\
		TextRun* piece = new SimpleRun(back.style);						\
		length_t pieceLength = MAXIMUM_RUN_LENGTH;						\
		if(surrogates::isLowSurrogate(line[back.column + pieceLength]))	\
			--pieceLength;												\
		piece->analysis = back.analysis;								\
		piece->column = back.column + pieceLength;						\
		piece->setLength(back.length() - pieceLength);					\
		back.setLength(pieceLength);									\
		runs.push_back(piece);											\
	}

	pair<vector<TextRun*>, vector<const StyledRun> > results;
	results.first.reserve(static_cast<size_t>(numberOfScriptRuns * ((styles.get() != 0) ? 1.2 : 1)));	// hmm...

	const SCRIPT_ITEM* scriptRun = scriptRuns;
	pair<const SCRIPT_ITEM*, length_t> nextScriptRun;	// 'second' is the beginning position
	nextScriptRun.first = (numberOfScriptRuns > 1) ? (scriptRuns + 1) : 0;
	nextScriptRun.second = (nextScriptRun.first != 0) ? nextScriptRun.first->iCharPos : lineString.length();
	pair<StyledRun, bool> styleRun;	// 'second' is false if 'first' is invalid
	if(styleRun.second = styles.get() != 0 && styles->hasNext()) {
		styles->current(styleRun.first);
		styles->next();
		results.second.push_back(styleRun.first);
	}
	pair<StyledRun, bool> nextStyleRun;	// 'second' is false if 'first' is invalid
	if(nextStyleRun.second = styles.get() != 0 && styles->hasNext())
		styles->current(nextStyleRun.first);
	length_t beginningOfNextStyleRun = nextStyleRun.second ? nextStyleRun.first.column : lineString.length();
	tr1::shared_ptr<const Font> font;	// font for current glyph run
	do {
		const length_t previousRunEnd = max<length_t>(scriptRun->iCharPos, styleRun.second ? styleRun.first.column : 0);
		assert(
			(previousRunEnd == 0 && results.first.empty() && results.second.empty())
			|| (!results.first.empty() && previousRunEnd == results.first.back()->end())
			|| (!results.second.empty() && previousRunEnd == results.second.back().column));
		length_t newRunEnd;
		bool forwardScriptRun = false, forwardStyleRun = false, forwardGlyphRun = false;

		if(nextScriptRun.second == beginningOfNextStyleRun) {
			newRunEnd = nextScriptRun.second;
			forwardScriptRun = forwardStyleRun = true;
		} else if(nextScriptRun.second < beginningOfNextStyleRun) {
			newRunEnd = nextScriptRun.second;
			forwardScriptRun = true;
		} else {	// nextScriptRun.second > beginningOfNextStyleRun
			newRunEnd = beginningOfNextStyleRun;
			forwardStyleRun = true;
		}

		if(surrogates::next(lineString.data() + previousRunEnd,
				lineString.data() + newRunEnd) < lineString.data() + newRunEnd || font.get() == 0) {
			const pair<const Char*, tr1::shared_ptr<const Font> > nextFontRun(
				findNextFontRun(
					Range<const Char*>(lineString.data() + previousRunEnd, lineString.data() + newRunEnd),
					styleRun.second ? styleRun.first.style : tr1::shared_ptr<const RunStyle>(), font, lip));
			font = nextFontRun.second;
			if(nextFontRun.first != 0) {
				forwardGlyphRun = true;
				newRunEnd = nextFontRun.first - lineString.data();
				forwardScriptRun = forwardStyleRun = false;
			}
		}
		if(!forwardGlyphRun && forwardScriptRun)
			forwardGlyphRun = true;

		if(forwardGlyphRun) {
			const bool breakScriptRun = newRunEnd < nextScriptRun.second;
			if(breakScriptRun)
				const_cast<SCRIPT_ITEM*>(scriptRun)->a.fLinkAfter = 0;
			results.first.push_back(
				new TextRun(Range<length_t>(!results.first.empty() ? results.first.back()->end() : 0, newRunEnd),
					scriptRun->a, font,
					(scriptTags != 0) ? scriptTags[scriptRun - scriptRuns] : SCRIPT_TAG_UNKNOWN));	// TODO: 'DFLT' is preferred?
			while(true) {
				auto_ptr<TextRun> piece(results.first.back()->splitIfTooLong(lineString));
				if(piece.get() == 0)
					break;
				results.first.push_back(piece.release());
			}
			if(breakScriptRun)
				const_cast<SCRIPT_ITEM*>(scriptRun)->a.fLinkBefore = 0;
		}
		if(forwardScriptRun) {
			if((scriptRun = nextScriptRun.first) != 0) {
				if(++nextScriptRun.first == scriptRuns + numberOfScriptRuns)
					nextScriptRun.first = 0;
				nextScriptRun.second = (nextScriptRun.first != 0) ? nextScriptRun.first->iCharPos : lineString.length();
			}
		}
		if(forwardStyleRun) {
			if(styleRun.second = nextStyleRun.second) {
				styleRun.first = nextStyleRun.first;
				results.second.push_back(styleRun.first);
				styles->next();
				if(nextStyleRun.second = styles->hasNext())
					styles->current(nextStyleRun.first);
				beginningOfNextStyleRun = nextStyleRun.second ? nextStyleRun.first.column : lineString.length();
			}
		}
	} while(scriptRun != 0 || styleRun.second);

	// commit
	std::swap(textRuns, results.first);
	std::swap(styledRanges, results.second);

#undef ASCENSION_SPLIT_LAST_RUN
}

/**
 * 
 * @see #merge, #substituteGlyphs
 */
void LineLayout::TextRun::positionGlyphs(const Context& context, const String& lineString, SimpleStyledRunIterator& styles) {
	assert(glyphs_.get() != 0 && glyphs_.unique());
	assert(glyphs_->indices.get() != 0 && glyphs_->advances.get() == 0);

	AutoBuffer<int> advances(new int[numberOfGlyphs()]);
	AutoBuffer<GOFFSET> offsets(new GOFFSET[numberOfGlyphs()]);
//	ABC width;
	HRESULT hr = ::ScriptPlace(0, &glyphs_->fontCache, glyphs_->indices.get(), numberOfGlyphs(),
		glyphs_->visualAttributes.get(), &analysis_, advances.get(), offsets.get(), 0/*&width*/);
	if(hr == E_PENDING) {
		HFONT oldFont = static_cast<HFONT>(::SelectObject(context.engine()->nativeHandle().get(), glyphs_->font->nativeHandle().get()));
		hr = ::ScriptPlace(context.engine()->nativeHandle().get(), &glyphs_->fontCache, glyphs_->indices.get(), numberOfGlyphs(),
			glyphs_->visualAttributes.get(), &analysis_, advances.get(), offsets.get(), 0/*&width*/);
		::SelectObject(context.engine()->nativeHandle().get(), oldFont);
	}
	if(FAILED(hr))
		throw hr;

	// apply text run styles
	for(; styles.hasNext(); styles.next()) {
		StyledRun styledRange;
		styles.current(styledRange);
/*
		// query widths of C0 and C1 controls in this run
		AutoBuffer<WORD> glyphIndices;
		if(ISpecialCharacterRenderer* scr = lip.specialCharacterRenderer()) {
			ISpecialCharacterRenderer::LayoutContext context(dc);
			context.readingDirection = readingDirection();
			dc.selectObject(glyphs_->font->handle().get());
			SCRIPT_FONTPROPERTIES fp;
			fp.cBytes = 0;
			for(length_t i = beginning(); i < end(); ++i) {
				if(isC0orC1Control(lineString[i])) {
					if(const int w = scr->getControlCharacterWidth(context, lineString[i])) {
						// substitute the glyph
						width.abcB += w - glyphs_->advances[i - beginning()];
						glyphs_->advances[i] = w;
						if(fp.cBytes == 0) {
							fp.cBytes = sizeof(SCRIPT_FONTPROPERTIES);
							const HRESULT hr2 = ::ScriptGetFontProperties(dc.get(), &glyphs_->fontCache, &fp);
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
*/
/*		// handle letter spacing
		if(styledRange.style.get() != 0 && styledRange.style->letterSpacing.unit != Length::INHERIT) {
			if(const int letterSpacing = pixels(dc, styledRange.style->letterSpacing, false, glyphs_->font->metrics())) {
				const bool rtl = readingDirection() == RIGHT_TO_LEFT;
				for(size_t i = textRun.glyphRange_.beginning(), e = textRun.glyphRange_.end(); i < e; ++i) {
					if((!rtl && (i + 1 == e || glyphs_->visualAttributes[i + 1].fClusterStart != 0))
							|| (rtl && (i == 0 || glyphs_->visualAttributes[i - 1].fClusterStart != 0))) {
						advances[i] += letterSpacing;
						if(rtl)
							offsets[i].du += letterSpacing;
					}
				}
			}
		}
*/	}

	// commit
	glyphs_->advances = advances;
	glyphs_->offsets = offsets;
//	glyphs_->width = width;
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
} // namespace @0

void LineLayout::TextRun::shape(Context& context, const String& lineString, const ILayoutInformationProvider& lip) {
	assert(glyphs_.unique());

	// TODO: check if the requested style (or the default one) disables shaping.

	context.setFont(*glyphs_->font);
	const StringPiece text(lineString.data() + beginning(), lineString.data() + end());
	int numberOfGlyphs;
	HRESULT hr = generateGlyphs(context, text, analysis_, *glyphs_, numberOfGlyphs);
	if(hr == USP_E_SCRIPT_NOT_IN_FONT) {
		analysis_.eScript = SCRIPT_UNDEFINED;
		hr = generateGlyphs(context, text, analysis_, *glyphs_, numberOfGlyphs);
	}
	if(FAILED(hr))
		generateDefaultGlyphs(context, text, analysis_, *glyphs_);

	// commit
	glyphRange_ = Range<WORD>(0, static_cast<WORD>(numberOfGlyphs));
}
#if 0
void LineLayout::TextRun::shape(DC& dc, const String& lineString, const ILayoutInformationProvider& lip, TextRun* nextRun) {
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

#ifdef ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
	bool firstVariationSelectorWasted = false;
	if(analysis_.fLogicalOrder != 0) {
		analysis_.fLogicalOrder = 0;
		firstVariationSelectorWasted = true;
	}
#endif // ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND

	if(analysis_.s.fDisplayZWG != 0 && scriptProperties.get(analysis_.eScript).fControl != 0) {
		// bidirectional format controls
		FontProperties fp;
		fp.size = computedFontProperties.size;
		tr1::shared_ptr<const Font> font(lip.fontCollection().get(L"Arial", fp, computedFontSizeAdjust));
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
		typedef vector<pair<tr1::shared_ptr<const Font>, int> > FailedFonts;
		FailedFonts failedFonts;	// failed fonts (font handle vs. # of missings)
		int numberOfMissingGlyphs;

		const Char* textString = lineString.data() + beginning();

#define ASCENSION_MAKE_TEXT_STRING_SAFE()												\
	assert(safeString.get() == 0);														\
	safeString.reset(new Char[length()]);												\
	wmemcpy(safeString.get(), lineString.data() + beginning(), length());				\
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
		AutoBuffer<Char> safeString;
		if(analysis_.eScript == SCRIPT_UNDEFINED
				&& find_if(textString, textString + length(), surrogates::isSurrogate) != textString + length()) {
			ASCENSION_MAKE_TEXT_STRING_SAFE();
		}

		// 1. the primary font
		oldFont = dc.selectObject((font_ = lip.fontCollection().get(computedFontFamily, computedFontProperties))->handle().get());
		hr = generateGlyphs(dc, textString, &numberOfMissingGlyphs);
		if(hr == USP_E_SCRIPT_NOT_IN_FONT) {
			::ScriptFreeCache(&glyphs_->fontCache);
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
							::ScriptFreeCache(&glyphs_->fontCache);
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
			for(StringCharacterIterator i(StringPiece(textString, textString + length())); i.hasNext(); i.next()) {
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
//				if(font_ == 0 && previousRun != 0) {
//					// use the previous run setting (but this will copy the style of the font...)
//					analysis_.eScript = previousRun->analysis_.eScript;
//					font_ = previousRun->font_;
//				}
			}
			if(font_ != 0) {
				ASCENSION_CHECK_FAILED_FONTS()
				if(!skip) {
					dc.selectObject(font_->handle().get());
					hr = generateGlyphs(dc, textString, &numberOfMissingGlyphs);
					if(hr != S_OK) {
						::ScriptFreeCache(&glyphs_->fontCache);
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
						::ScriptFreeCache(&glyphs_->fontCache);
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
					::ScriptFreeCache(&glyphs_->fontCache);
					generateDefaultGlyphs(dc);	// worst case...
				}
			}
		}

#undef ASCENSION_MAKE_TEXT_STRING_SAFE
#undef ASCENSION_CHECK_FAILED_FONTS
	}

#ifdef ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
	if(!uniscribeSupportsIVS()) {

#define ASCENSION_VANISH_VARIATION_SELECTOR(index)														\
	WORD blankGlyph;																					\
	if(S_OK != (hr = ::ScriptGetCMap(dc.get(), &glyphs_->fontCache, L"\x0020", 1, 0, &blankGlyph))) {	\
		SCRIPT_FONTPROPERTIES fp;																		\
		fp.cBytes = sizeof(SCRIPT_FONTPROPERTIES);														\
		if(FAILED(::ScriptGetFontProperties(dc.get(), &glyphs_->fontCache, &fp)))						\
			fp.wgBlank = 0;	/* hmm... */																\
		blankGlyph = fp.wgBlank;																		\
	}																									\
	glyphs_->indices[glyphs_->clusters[index]]															\
		= glyphs_->indices[glyphs_->clusters[index + 1]] = blankGlyph;									\
	SCRIPT_VISATTR* const va = glyphs_->visualAttributes.get();											\
	va[glyphs_->clusters[index]].uJustification															\
		= va[glyphs_->clusters[index + 1]].uJustification = SCRIPT_JUSTIFY_BLANK;						\
	va[glyphs_->clusters[index]].fZeroWidth																\
		= va[glyphs_->clusters[index + 1]].fZeroWidth = 1

		if(firstVariationSelectorWasted) {
			// remove glyphs correspond to the first character which is conjuncted with the last
			// character as a variation character
			assert(length() > 1);
			ASCENSION_VANISH_VARIATION_SELECTOR(0);
		} else if(analysis_.eScript != SCRIPT_UNDEFINED && length() > 3
				&& surrogates::isHighSurrogate(lineString[beginning()]) && surrogates::isLowSurrogate(lineString[beginning() + 1])) {
			for(StringCharacterIterator i(
					StringPiece(lineString.data() + beginning(), length()), lineString.data() + beginning() + 2); i.hasNext(); i.next()) {
				const CodePoint variationSelector = i.current();
				if(variationSelector >= 0xe0100ul && variationSelector <= 0xe01eful) {
					StringCharacterIterator baseCharacter(i);
					baseCharacter.previous();
					if(static_cast<const SystemFont*>(font_.get())->ivsGlyph(
							baseCharacter.current(), variationSelector,
							glyphs_->indices[glyphs_->clusters[baseCharacter.tell() - lineString.data()]])) {
						ASCENSION_VANISH_VARIATION_SELECTOR(i.tell() - lineString.data());
					}
				}
			}
		}
		if(nextRun != 0 && nextRun->length() > 1) {
			const CodePoint variationSelector = surrogates::decodeFirst(
				lineString.begin() + nextRun->beginning(), lineString.begin() + nextRun->beginning() + 2);
			if(variationSelector >= 0xe0100ul && variationSelector <= 0xe01eful) {
				const CodePoint baseCharacter = surrogates::decodeLast(
					lineString.data() + beginning(), lineString.data() + end());
				if(static_cast<const SystemFont*>(font_.get())->ivsGlyph(
						baseCharacter, variationSelector, glyphs_->indices[glyphs_->clusters[length() - 1]]))
					nextRun->analysis_.fLogicalOrder = 1;
			}
		}
#undef ASCENSION_VANISH_VARIATION_SELECTOR
	}
#endif // ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
}
#endif
auto_ptr<LineLayout::TextRun> LineLayout::TextRun::splitIfTooLong(const String& lineString) {
	if(estimateNumberOfGlyphs(length()) <= 65535)
		return auto_ptr<TextRun>();

	// split this run, because the length would cause ScriptShape to fail (see also Mozilla bug 366643).
	static const length_t MAXIMUM_RUN_LENGTH = 43680;	// estimateNumberOfGlyphs(43680) == 65536
	length_t opportunity = 0;
	AutoBuffer<SCRIPT_LOGATTR> la(new SCRIPT_LOGATTR[length()]);
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

	auto_ptr<TextRun> following(new TextRun(Range<length_t>(
		opportunity, length() - opportunity), analysis_, glyphs_->font, glyphs_->scriptTag));
	static_cast<Range<length_t>&>(*this) = Range<length_t>(0, opportunity);
	analysis_.fLinkAfter = following->analysis_.fLinkBefore = 0;
	return following;
}

/**
 * 
 * @param context the graphics context
 * @param runs the minimal runs
 * @param lineString the line string
 * @see #merge, #positionGlyphs
 */
void LineLayout::TextRun::substituteGlyphs(const Context& context, const Range<TextRun**>& runs, const String& lineString) {
	// this method processes the following substitutions:
	// 1. missing glyphs
	// 2. ideographic variation sequences (if Uniscribe did not support)

	// 1. Presentative glyphs for missing ones

	// TODO: generate missing glyphs.

	// 2. Ideographic Variation Sequences (Uniscribe workaround)
	// Older Uniscribe (version < 1.626.7100.0) does not support IVS.

#ifdef ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
	if(!uniscribeSupportsIVS()) {
		for(TextRun** p = runs.beginning(); p < runs.end(); ++p) {
			TextRun& run = **p;

			// process IVSes in a glyph run
			if(run.analysis_.eScript != SCRIPT_UNDEFINED && run.length() > 3
					&& surrogates::isHighSurrogate(lineString[run.beginning()])
					&& surrogates::isLowSurrogate(lineString[run.beginning() + 1])) {
				for(StringCharacterIterator i(StringPiece(lineString.data() + run.beginning(),
						run.length()), lineString.data() + run.beginning() + 2); i.hasNext(); i.next()) {
					const CodePoint variationSelector = i.current();
					if(variationSelector >= 0xe0100ul && variationSelector <= 0xe01eful) {
						StringCharacterIterator baseCharacter(i);
						baseCharacter.previous();
						if(run.glyphs_->font->ivsGlyph(
								baseCharacter.current(), variationSelector,
								run.glyphs_->indices[run.glyphs_->clusters[baseCharacter.tell() - lineString.data()]])) {
							run.glyphs_->vanish(context, i.tell() - lineString.data() - run.beginning());
							run.glyphs_->vanish(context, i.tell() - lineString.data() - run.beginning() + 1);
						}
					}
				}
			}

			// process an IVS across two glyph runs
			if(p + 1 != runs.end() && p[1]->length() > 1) {
				TextRun& next = *p[1];
				const CodePoint variationSelector = surrogates::decodeFirst(
					lineString.begin() + next.beginning(), lineString.begin() + next.beginning() + 2);
				if(variationSelector >= 0xe0100ul && variationSelector <= 0xe01eful) {
					const CodePoint baseCharacter = surrogates::decodeLast(
						lineString.data() + run.beginning(), lineString.data() + run.end());
					if(run.glyphs_->font->ivsGlyph(baseCharacter, variationSelector,
							run.glyphs_->indices[run.glyphs_->clusters[run.length() - 1]])) {
						next.glyphs_->vanish(context, 0);
						next.glyphs_->vanish(context, 1);
					}
				}
			}
		}
#undef ASCENSION_VANISH_VARIATION_SELECTOR
	}
#endif // ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
}

inline int LineLayout::TextRun::x(length_t at, bool trailing) const {
	if(at < beginning() || at > end())
		throw k::BadPositionException(k::Position(INVALID_INDEX, at));
	int result;
	const HRESULT hr = ::ScriptCPtoX(static_cast<int>(at - beginning()), trailing,
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
	inline win32::Handle<HPEN> createPen(const Color& color, int width, int style) {
		if(color == Color() || color.alpha() < 0xff)
			throw invalid_argument("color");
		LOGBRUSH brush;
		brush.lbColor = color.asCOLORREF();
		brush.lbStyle = BS_SOLID;
		HPEN pen = 0;
		switch(style) {
		case 1:	// solid
			pen = (width == 1) ? ::CreatePen(PS_SOLID, 1, color.asCOLORREF())
				: ::ExtCreatePen(PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_FLAT, width, &brush, 0, 0);
		case 2:	// dashed
			pen = ::ExtCreatePen(PS_GEOMETRIC | PS_DASH | PS_ENDCAP_FLAT, width, &brush, 0, 0);
		case 3:	// dotted
			pen = ::ExtCreatePen(PS_GEOMETRIC | PS_DOT | PS_ENDCAP_FLAT, width, &brush, 0, 0);
		}
		if(pen == 0)
			throw UnknownValueException("style");
		return win32::Handle<HPEN>(pen, &::DeleteObject);
	}
	inline void drawDecorationLines(Context& context, const RunStyle& style, const Color& foregroundColor, int x, int y, int width, int height) {
		if(style.decorations.underline.style != Decorations::NONE || style.decorations.strikethrough.style != Decorations::NONE) {
			const win32::Handle<HDC> dc(context.engine()->nativeHandle());
			int baselineOffset, underlineOffset, underlineThickness, linethroughOffset, linethroughThickness;
			if(getDecorationLineMetrics(dc, &baselineOffset, &underlineOffset, &underlineThickness, &linethroughOffset, &linethroughThickness)) {
				// draw underline
				if(style.decorations.underline.style != Decorations::NONE) {
					win32::Handle<HPEN> pen(createPen((style.decorations.underline.color != Color()) ?
						style.decorations.underline.color : foregroundColor, underlineThickness, style.decorations.underline.style));
					HPEN oldPen = static_cast<HPEN>(::SelectObject(dc.get(), pen.use()));
					const int underlineY = y + baselineOffset - underlineOffset + underlineThickness / 2;
					::MoveToEx(dc.get(), x, underlineY, 0);
					::LineTo(dc.get(), x + width, underlineY);
					::SelectObject(dc.get(), oldPen);
				}
				// draw strikethrough line
				if(style.decorations.strikethrough.style != Decorations::NONE) {
					win32::Handle<HPEN> pen(createPen((style.decorations.strikethrough.color != Color()) ?
						style.decorations.strikethrough.color : foregroundColor, linethroughThickness, 1));
					HPEN oldPen = static_cast<HPEN>(::SelectObject(dc.get(), pen.use()));
					const int strikeoutY = y + baselineOffset - linethroughOffset + linethroughThickness / 2;
					::MoveToEx(dc.get(), x, strikeoutY, 0);
					::LineTo(dc.get(), x + width, strikeoutY);
					::SelectObject(dc.get(), oldPen);
				}
			}
		}
	}
	inline void drawBorder(Context& context, const Border& style,
			const Font::Metrics& fontMetrics, const Color& currentColor, int start, int before, int end, int after) {
		// TODO: rewrite later.
		const win32::Handle<HDC> dc(context.engine()->nativeHandle());
		const Border::Part* const styles[] = {&style.before, &style.after, &style.start, &style.end};
		const POINT points[4][2] = {
			{{start, before}, {end, before}},
			{{start, after}, {end, after}},
			{{start, before}, {start, after}},
			{{end, before}, {end, after}}
		};
		for(size_t i = 0; i < ASCENSION_COUNTOF(styles); ++i) {
			if(styles[i]->style != Border::NONE && styles[i]->style != Border::HIDDEN) {
				const int width = pixels(context, styles[i]->width, true, fontMetrics);
				if(width != 0) {
					win32::Handle<HPEN> pen(createPen(
						((styles[i]->color != Color()) ? styles[i]->color : currentColor), width, styles[i]->style));
					HPEN oldPen = static_cast<HPEN>(::SelectObject(dc.get(), pen.use()));
					::MoveToEx(dc.get(), points[i][0].x, points[i][0].y, 0);
					::LineTo(dc.get(), points[i][1].x, points[i][1].y);
					::SelectObject(dc.get(), oldPen);
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

namespace {
	template<typename T, size_t staticCapacity>
	class AutoArray {
	public:
		typedef T ElementType;
		static const size_t STATIC_CAPACITY = staticCapacity;
	public:
		AutoArray() : capacity_(STATIC_CAPACITY) {
		}
		ElementType& operator[](size_t i) {
			return p_[i];
		}
		const ElementType& operator[](size_t i) const {
			return p_[i];
		}
		ElementType& at(size_t i) {
			if(i >= capacity_)
				throw out_of_range("i");
			return operator[](i);
		}
		const ElementType& at(size_t i) const {
			if(i >= capacity_)
				throw out_of_range("i");
			return operator[](i);
		}
		ElementType* get() const {
			return p_;
		}
		void reallocate(size_t n) {
			if(n <= STATIC_CAPACITY) {
				allocated_.reset();
				p_ = auto_;
				capacity_ = STATIC_CAPACITY;
			} else {
				if(n > capacity_) {
					allocated_.reset(new ElementType[n]);
					capacity_ = n;
				}
				p_ = allocated_.get();
			}
		}
	private:
		ElementType auto_[STATIC_CAPACITY];
		AutoBuffer<ElementType> allocated_;
		size_t capacity_;
		ElementType* p_;
	};
}

/**
 * Constructor.
 * @param dc the device context
 * @param layoutInformation the layout information
 * @param line the line
 * @throw kernel#BadPositionException @a line is invalid
 */
LineLayout::LineLayout(Context& context, const ILayoutInformationProvider& layoutInformation,
		length_t line) : lip_(layoutInformation), lineNumber_(line), style_(layoutInformation.presentation().lineStyle(line)),
		runs_(0), numberOfRuns_(0), sublineOffsets_(0), sublineFirstRuns_(0), numberOfSublines_(0), longestSublineWidth_(-1), wrapWidth_(-1) {
	assert(style_.get() != 0);

	// calculate the wrapping width
	if(layoutInformation.layoutSettings().lineWrap.wraps()) {
		wrapWidth_ = layoutInformation.width();
		if(ISpecialCharacterRenderer* scr = layoutInformation.specialCharacterRenderer()) {
			ISpecialCharacterRenderer::LayoutContext lc(context);
			lc.readingDirection = readingDirection();
			wrapWidth_ -= scr->getLineWrappingMarkWidth(lc);
		}
	}

	const String& lineString = text();
	if(lineString.empty()) {	// an empty line
		numberOfRuns_ = 0;
		numberOfSublines_ = 1;
		longestSublineWidth_ = 0;
		return;
	}

	// split the text line into text runs as following steps:
	// 1. split the text into script runs (SCRIPT_ITEMs) by Uniscribe
	// 2. split each script runs into atomically-shapable runs (TextRuns) with StyledRunIterator
	// 3. generate glyphs for each text runs
	// 4. position glyphs for each text runs

	// 1. split the text into script runs by Uniscribe
	HRESULT hr;
	const LayoutSettings& c = lip_.layoutSettings();
	const Presentation& presentation = lip_.presentation();

	// 1-1. configure Uniscribe's itemize
	win32::AutoZero<SCRIPT_CONTROL> control;
	win32::AutoZero<SCRIPT_STATE> initialState;
	initialState.uBidiLevel = (readingDirection() == RIGHT_TO_LEFT) ? 1 : 0;
//	initialState.fOverrideDirection = 1;
	initialState.fInhibitSymSwap = c.inhibitsSymmetricSwapping;
	initialState.fDisplayZWG = c.displaysShapingControls;
	resolveNumberSubstitution(
		(style_.get() != 0) ? &style_->numberSubstitution : 0, control, initialState);	// ignore result...

	// 1-2. itemize
	// note that ScriptItemize can cause a buffer overflow (see Mozilla bug 366643)
	AutoArray<SCRIPT_ITEM, 128> scriptRuns;
	AutoArray<OPENTYPE_TAG, scriptRuns.STATIC_CAPACITY> scriptTags;
	int estimatedNumberOfScriptRuns = max(static_cast<int>(lineString.length()) / 4, 2), numberOfScriptRuns;
	HRESULT(WINAPI* scriptItemizeOpenType)(const WCHAR*, int, int,
		const SCRIPT_CONTROL*, const SCRIPT_STATE*, SCRIPT_ITEM*, OPENTYPE_TAG*, int*) = uspLib->get<0>();
	while(true) {
		scriptRuns.reallocate(estimatedNumberOfScriptRuns);
		scriptTags.reallocate(estimatedNumberOfScriptRuns);
		if(scriptItemizeOpenType != 0)
			hr = (*scriptItemizeOpenType)(lineString.data(), static_cast<int>(lineString.length()),
				estimatedNumberOfScriptRuns, &control, &initialState, scriptRuns.get(), scriptTags.get(), &numberOfScriptRuns);
		else
			hr = ::ScriptItemize(lineString.data(), static_cast<int>(lineString.length()),
				estimatedNumberOfScriptRuns, &control, &initialState, scriptRuns.get(), &numberOfScriptRuns);
		if(hr != E_OUTOFMEMORY)	// estimatedNumberOfRuns was enough...
			break;
		estimatedNumberOfScriptRuns *= 2;
	}
	if(c.disablesDeprecatedFormatCharacters) {
		for(int i = 0; i < numberOfScriptRuns; ++i) {
			scriptRuns[i].a.s.fInhibitSymSwap = initialState.fInhibitSymSwap;
			scriptRuns[i].a.s.fDigitSubstitute = initialState.fDigitSubstitute;
		}
	}
	if(scriptItemizeOpenType == 0)
		fill_n(scriptTags.get(), numberOfScriptRuns, SCRIPT_TAG_UNKNOWN);

	// 2. split each script runs into text runs with StyledRunIterator
	vector<TextRun*> textRuns;
	vector<const StyledRun> styledRanges;
	TextRun::mergeScriptsAndStyles(context, lineString.data(),
		scriptRuns.get(), scriptTags.get(), numberOfScriptRuns,
		presentation.textRunStyles(lineNumber()), lip_, textRuns, styledRanges);
	runs_ = new TextRun*[numberOfRuns_ = textRuns.size()];
	copy(textRuns.begin(), textRuns.end(), runs_);
	styledRanges_.reset(new StyledRun[numberOfStyledRanges_ = styledRanges.size()]);
	copy(styledRanges.begin(), styledRanges.end(), styledRanges_.get());

	// 3. generate glyphs for each text runs
	for(size_t i = 0; i < numberOfRuns_; ++i)
		runs_[i]->shape(context, lineString, lip_);
	TextRun::substituteGlyphs(context, Range<TextRun**>(runs_, runs_ + numberOfRuns_), lineString);

	// 4. position glyphs for each text runs
	for(size_t i = 0; i < numberOfRuns_; ++i)
		runs_[i]->positionGlyphs(context, lineString, SimpleStyledRunIterator(Range<const StyledRun*>(
			styledRanges_.get(), styledRanges_.get() + numberOfStyledRanges_), runs_[i]->beginning()));

	// wrap into visual sublines and reorder runs in each sublines
	if(numberOfRuns_ == 0 || wrapWidth_ == -1) {
		numberOfSublines_ = 1;
		sublineFirstRuns_ = new size_t[1];
		sublineFirstRuns_[0] = 0;
		reorder();
		expandTabsWithoutWrapping();
	} else {
		wrap(context);
		reorder();
		if(style_->alignment == JUSTIFY)
			justify();
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
NativePolygon LineLayout::blackBoxBounds(length_t first, length_t last) const {
	if(first > last)
		throw invalid_argument("first is greater than last.");
	else if(last > text().length())
		throw kernel::BadPositionException(kernel::Position(lineNumber_, last));

	// handle empty line
	if(numberOfRuns_ == 0)
		return win32::Handle<HRGN>(::CreateRectRgn(0, 0, 0, linePitch()), &::DeleteObject);

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
			const TextRun& run = *runs_[i];
			if(first <= run.end() && last >= run.beginning()) {
				rectangle.left = cx + ((first > run.beginning()) ?
					run.x(first, false) : ((run.readingDirection() == LEFT_TO_RIGHT) ? 0 : run.totalWidth()));
				rectangle.right = cx + ((last < run.end()) ?
					run.x(last, false) : ((run.readingDirection() == LEFT_TO_RIGHT) ? run.totalWidth() : 0));
				if(rectangle.left != rectangle.right) {
					if(rectangle.left > rectangle.right)
						std::swap(rectangle.left, rectangle.right);
					rectangles.push_back(rectangle);
				}
			}
			cx += run.totalWidth();
		}
	}

	// create the result region
	AutoBuffer<POINT> vertices(new POINT[rectangles.size() * 4]);
	AutoBuffer<int> numbersOfVertices(new int[rectangles.size()]);
	for(size_t i = 0, c = rectangles.size(); i < c; ++i) {
		vertices[i * 4 + 0].x = vertices[i * 4 + 3].x = rectangles[i].left;
		vertices[i * 4 + 0].y = vertices[i * 4 + 1].y = rectangles[i].top;
		vertices[i * 4 + 1].x = vertices[i * 4 + 2].x = rectangles[i].right;
		vertices[i * 4 + 2].y = vertices[i * 4 + 3].y = rectangles[i].bottom;
	}
	fill_n(numbersOfVertices.get(), rectangles.size(), 4);
	return win32::Handle<HRGN>(::CreatePolyPolygonRgn(vertices.get(),
		numbersOfVertices.get(), static_cast<int>(rectangles.size()), WINDING), &::DeleteObject);
}

/**
 * Returns the smallest rectangle emcompasses the whole text of the line. It might not coincide
 * exactly the ascent, descent or overhangs of the text.
 * @return the size of the bounds
 * @see #blackBoxBounds, #bounds(length_t, length_t), #sublineBounds
 */
Dimension<> LineLayout::bounds() const /*throw()*/ {
	return Dimension<>(longestSublineWidth(), static_cast<long>(linePitch() * numberOfSublines_));
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
Rect<> LineLayout::bounds(length_t first, length_t last) const {
	if(first > last)
		throw invalid_argument("first is greater than last.");
	else if(last > text().length())
		throw kernel::BadPositionException(kernel::Position(lineNumber_, last));

	// handle empty line
	if(numberOfRuns_ == 0)
		return Rect<>(Point<>(0, 0), Dimension<>(0, linePitch()));

	// determine the top and the bottom (it's so easy)
	Rect<> bounds;	// the result
	const length_t firstSubline = subline(first), lastSubline = subline(last);
	bounds.setY(Range<>(
		static_cast<int>(linePitch() * firstSubline), static_cast<LONG>(linePitch() * (lastSubline + 1))));

	// find side bounds between 'firstSubline' and 'lastSubline'
	int left = numeric_limits<LONG>::max(), right = numeric_limits<LONG>::min();
	for(length_t subline = firstSubline + 1; subline < lastSubline; ++subline) {
		const int indent = sublineIndent(subline);
		left = min(indent, left);
		right = max(indent + sublineWidth(subline), right);
	}

	// find side bounds in 'firstSubline' and 'lastSubline'
	int cx;
	const length_t firstAndLast[2] = {firstSubline, lastSubline};
	for(size_t i = 0; i < ASCENSION_COUNTOF(firstAndLast); ++i) {
		const length_t subline = firstAndLast[i];
		const size_t endOfRuns = (subline + 1 < numberOfSublines_) ? sublineFirstRuns_[subline + 1] : numberOfRuns_;
		// find left bound
		cx = sublineIndent(subline);
		for(size_t j = sublineFirstRuns_[subline]; j < endOfRuns; ++j) {
			if(cx >= left)
				break;
			const TextRun& run = *runs_[j];
			if(first <= run.end() && last >= run.beginning()) {
				const int x = run.x((
					(run.readingDirection() == LEFT_TO_RIGHT) ? max(first, run.beginning()) : min(last, run.end())), false);
				left = min(cx + x, left);
				break;
			}
			cx += run.totalWidth();
		}
		// find right bound
		cx = sublineIndent(firstSubline) + sublineWidth(lastSubline);
		for(size_t j = endOfRuns - 1; ; --j) {
			if(cx <= right)
				break;
			const TextRun& run = *runs_[j];
			if(first <= run.end() && last >= run.beginning()) {
				const int x = run.x((
					(run.readingDirection() == LEFT_TO_RIGHT) ? min(last, run.end()) : max(first, run.beginning())), false);
				right = max(cx - run.totalWidth() + x, right);
				break;
			}
			if(j == sublineFirstRuns_[subline])
				break;
			cx -= run.totalWidth();
		}
	}
	bounds.setX(Range<int>(left, right));

	return bounds;
}

namespace {
	inline HRESULT callScriptItemize(const WCHAR* text, int length, int estimatedNumberOfItems,
			const SCRIPT_CONTROL& control, const SCRIPT_STATE& initialState, SCRIPT_ITEM items[], OPENTYPE_TAG scriptTags[], int& numberOfItems) {
		static HRESULT(WINAPI* scriptItemizeOpenType)(const WCHAR*, int, int,
			const SCRIPT_CONTROL*, const SCRIPT_STATE*, SCRIPT_ITEM*, OPENTYPE_TAG*, int*) = uspLib->get<0>();
		if(scriptItemizeOpenType != 0 && scriptTags != 0)
			return (*scriptItemizeOpenType)(text, length, estimatedNumberOfItems, &control, &initialState, items, scriptTags, &numberOfItems);
		else
			return ::ScriptItemize(text, length, estimatedNumberOfItems, &control, &initialState, items, &numberOfItems);
	}
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
 * @param context the rendering context
 * @param x the x-coordinate of the position to draw
 * @param y the y-coordinate of the position to draw
 * @param paintRect the region to draw
 * @param clipRect the clipping region
 * @param selection defines the region and the color of the selection
 */
void LineLayout::draw(Context& context, int x, int y, const Rect<>& paintRect,
		const Rect<>& clipRect, const Selection* selection) const /*throw()*/ {
	const int dy = linePitch();

	// empty line
	if(isDisposed()) {
		const Colors lineColor = lip_.presentation().getLineColor(lineNumber_);
		context.fillRectangle(
			Rect<>(
				Point<>(
					max(paintRect.x().beginning(), clipRect.x().beginning()),
					max(clipRect.y().beginning(), max(paintRect.y().beginning(), y))),
				Point<>(
					min(paintRect.x().end(), clipRect.x().end()),
					min(clipRect.y().end(), min(paintRect.y().end(), y + dy)))),
			Color::fromCOLORREF(systemColors.serve(lineColor.background, COLOR_WINDOW)));
		return;
	}

	// skip to the subline needs to draw
	length_t subline = (y + dy >= paintRect.y().beginning()) ? 0 : (paintRect.y().beginning() - (y + dy)) / dy;
	if(subline >= numberOfSublines_)
		return;	// this logical line does not need to draw
	y += static_cast<int>(dy * subline);

	for(; subline < numberOfSublines_; ++subline) {
		draw(subline, context, x, y, paintRect, clipRect, selection);
		if((y += dy) >= paintRect.y().end())	// to next subline
			break;
	}
}

/**
 * Draws the specified subline layout to the output device.
 * @param subline the visual subline
 * @param context the graphics context
 * @param x the x-coordinate of the position to draw
 * @param y the y-coordinate of the position to draw
 * @param paintRect the region to draw
 * @param clipRect the clipping region
 * @param selection defines the region and the color of the selection. can be @c null
 * @throw IndexOutOfBoundsException @a subline is invalid
 */
void LineLayout::draw(length_t subline, Context& context,
		int x, int y, const Rect<>& paintRect, const Rect<>& clipRect, const Selection* selection) const {
	if(subline >= numberOfSublines_)
		throw IndexOutOfBoundsException("subline");

#ifdef _DEBUG
	if(DIAGNOSE_INHERENT_DRAWING)
		win32::DumpContext() << L"@LineLayout.draw draws line " << lineNumber_ << L" (" << subline << L")\n";
#endif // _DEBUG

	// the following topic describes how to draw a selected text using masking by clipping
	// Catch 22 : Design and Implementation of a Win32 Text Editor
	// Part 10 - Transparent Text and Selection Highlighting (http://www.catch22.net/tuts/editor10.asp)

	const int dy = linePitch();
	const int lineHeight = lip_.textMetrics().cellHeight();
	const Colors lineColor = lip_.presentation().getLineColor(lineNumber_);
	const Color marginColor(Color::fromCOLORREF(systemColors.serve(lineColor.background, COLOR_WINDOW)));
	ISpecialCharacterRenderer::DrawingContext dc(context);
	ISpecialCharacterRenderer* specialCharacterRenderer = lip_.specialCharacterRenderer();

	if(specialCharacterRenderer != 0)
		context.rect.setY(Range<int>(y, y + lineHeight));

	context.save();
	::SetTextAlign(context.engine()->nativeHandle().get(), TA_TOP | TA_LEFT | TA_NOUPDATECP);
	if(isDisposed())	// empty line
		context.fillRectangle(
			Rect<>(
				Point<>(
					max(paintRect.x().beginning(), clipRect.x().beginning()),
					max(clipRect.y().beginning(), max(paintRect.y().beginning(), y))),
				Point<>(
					min(paintRect.x().end(), clipRect.x().end()),
					min(clipRect.y().end(), min(paintRect.y().end(), y + dy)))
			),
			marginColor);
	else {
		const String& line = text();
		HRESULT hr;
		Range<length_t> selectedRange;
		if(selection != 0) {
			if(!selectedRangeOnVisualLine(selection->caret(), lineNumber_, subline, selectedRange))
				selection = 0;
		}

		// 1. paint gap of sublines
		// 2. paint the left margin
		// 3. paint background of the text runs
		// 4. paint the right margin
		// 5. draw the foreground glyphs

		// 1. paint gap of sublines
		Point<> basePoint(x, y);
		win32::Handle<HRGN> clipRegion(::CreateRectRgn(
			clipRect.x().beginning(), max(basePoint.y, clipRect.y().beginning()),
			clipRect.x().end(), min<long>(basePoint.y + dy, clipRect.y().end())), &::DeleteObject);
//		dc.selectClipRgn(clipRegion.getHandle());
		if(dy - lineHeight > 0)
			context.fillRectangle(
				Rect<>(
					Point<>(paintRect.x().beginning(), basePoint.y + lineHeight),
					Dimension<>(paintRect.x().end() - paintRect.x().beginning(), dy - lineHeight)),
				marginColor);

		basePoint.x += sublineIndent(subline);

		tr1::shared_ptr<const RunStyle> defaultStyle(lip_.presentation().defaultTextRunStyle());
		const COLORREF defaultForeground = systemColors.serve((defaultStyle.get() != 0) ? defaultStyle->foreground : Color(), COLOR_WINDOWTEXT);
		const COLORREF defaultBackground = systemColors.serve((defaultStyle.get() != 0) ? defaultStyle->background : Color(), COLOR_WINDOW);
		size_t firstRun = sublineFirstRuns_[subline];
		size_t lastRun = (subline < numberOfSublines_ - 1) ? sublineFirstRuns_[subline + 1] : numberOfRuns_;

		// 2. paint the left margin
		if(basePoint.x > paintRect.x().beginning())
			context.fillRectangle(
				Rect<>(
					Point<>(paintRect.x().beginning(), basePoint.y),
					Dimension<>(basePoint.x - paintRect.x().beginning(), lineHeight)),
				marginColor);

		// 3. paint background of the text runs
		int startX = basePoint.x;
		for(size_t i = firstRun; i < lastRun; ++i) {
			const TextRun& run = *runs_[i];
			if(basePoint.x + run.totalWidth() < paintRect.x().beginning()) {	// this run does not need to draw
				++firstRun;
				startX = basePoint.x + run.totalWidth();
			} else {
				basePoint.y += run.font()->metrics().ascent();
				if(selection != 0 && selectedRange.includes(run)) {
					Rect<> selectedBounds;
					run.drawBackground(context, basePoint, run, selection->background(), &paintRect, &selectedBounds);
					::ExcludeClipRect(context.engine()->nativeHandle().get(),
						selectedBounds.x().beginning(), selectedBounds.y().beginning(),
						selectedBounds.x().end(), selectedBounds.y().end());
				} else {
					StyledRunEnumerator i(auto_ptr<IStyledRunIterator>(
						new SimpleStyledRunIterator(Range<const StyledRun*>(
							styledRanges_.get(), styledRanges_.get() + numberOfStyledRanges_), run.beginning())), run.end());
					assert(i.hasNext());
					for(; i.hasNext(); i.next()) {
						Range<length_t> range(i.currentRange());
						if(range.beginning() < run.beginning())
							range = Range<length_t>(run.beginning(), range.end());
						if(selection == 0 || range.end() <= selectedRange.beginning() || range.beginning() >= selectedRange.end())
							run.drawBackground(context, basePoint, range,
								(i.currentStyle()->background != Color()) ? i.currentStyle()->background : marginColor, &paintRect, 0);
						else {
							// paint before selection
							if(selectedRange.beginning() > range.beginning())
								run.drawBackground(context, basePoint, Range<length_t>(range.beginning(), selectedRange.beginning()),
									(i.currentStyle()->background != Color()) ? i.currentStyle()->background : marginColor, &paintRect, 0);
							// paint selection
							Rect<> selectedBounds;
							run.drawBackground(context, basePoint, selectedRange, selection->background(), &paintRect, &selectedBounds);
							::ExcludeClipRect(context.engine()->nativeHandle().get(),
								selectedBounds.x().beginning(), selectedBounds.y().beginning(),
								selectedBounds.x().end(), selectedBounds.y().end());
							// paint after selection
							if(selectedRange.end() < range.end())
								run.drawBackground(context, basePoint, Range<length_t>(selectedRange.end(), range.end()),
									(i.currentStyle()->background != Color()) ? i.currentStyle()->background : marginColor, &paintRect, 0);
						}
					}
				}
				basePoint.y -= run.font()->metrics().ascent();
			}
			basePoint.x += run.totalWidth();
			if(basePoint.x >= paintRect.x().end()) {
				lastRun = i + 1;
				break;
			}
		}

		// 4. paint the right margin
		if(basePoint.x < paintRect.x().end())
			context.fillRectangle(Rect<>(basePoint, Dimension<>(paintRect.x().end() - basePoint.x, dy)), marginColor);

		// 5. draw the foreground glyphs
		basePoint.x = startX;
		TextRun::Overlay selectionOverlay;
		if(selection != 0) {
			selectionOverlay.color = selection->foreground();
			selectionOverlay.range = selectedRange;
		}
		for(size_t i = firstRun; i < lastRun; ++i) {
			const TextRun& run = *runs_[i];
			basePoint.y += run.font()->metrics().ascent();
			StyledRunEnumerator j(auto_ptr<IStyledRunIterator>(
				new SimpleStyledRunIterator(Range<const StyledRun*>(
					styledRanges_.get(), styledRanges_.get() + numberOfStyledRanges_), run.beginning())), run.end());
			for(; j.hasNext(); j.next())
				run.drawForeground(context, basePoint, j.currentRange(), j.currentStyle()->foreground, &paintRect, 0);
			basePoint.y -= run.font()->metrics().ascent();
			basePoint.x += run.totalWidth();
		}

		// 6. draw the selected foreground glyphs
		if(selection != 0) {
			basePoint.x = startX;
			::ExtSelectClipRgn(context.engine()->nativeHandle().get(),
				win32::Handle<HRGN>(::CreateRectRgnIndirect(&toNative(paintRect))).get(), RGN_XOR);
			for(size_t i = firstRun; i < lastRun; ++i) {
				const TextRun& run = *runs_[i];
				if(run.beginning() < selectedRange.end() && run.end() > selectedRange.beginning()) {
					basePoint.y += run.font()->metrics().ascent();
					run.drawForeground(context, basePoint, selectedRange, selection->foreground(), &paintRect, 0);
					basePoint.y -= run.font()->metrics().ascent();
				}
				basePoint.x += run.totalWidth();
			}
		}
#if 0
		// draw outside of the selection
		Rect<> runRect;
		runRect.top = y;
		runRect.bottom = y + dy;
		runRect.left = x = startX;
		dc.setBkMode(TRANSPARENT);
		for(size_t i = firstRun; i < lastRun; ++i) {
			TextRun& run = *runs_[i];
			COLORREF foreground;
			if(lineColor.foreground != Color())
				foreground = lineColor.foreground.asCOLORREF();
			else if(run.requestedStyle().get() != 0 && run.requestedStyle()->foreground != Color())
				foreground = run.requestedStyle()->foreground.asCOLORREF();
			else
				foreground = defaultForeground;
			if(line[run.beginning()] != L'\t') {
				if(selection == 0 /*|| run.overhangs()*/
						|| !(run.beginning() >= selectedRange.beginning() && run.end() <= selectedRange.end())) {
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
				TextRun& run = *runs_[i];
				// text
				if(selection != 0 && line[run.beginning()] != L'\t'
						&& (/*run.overhangs() ||*/ (run.beginning() < selectedRange.end() && run.end() > selectedRange.beginning()))) {
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
				TextRun& run = *runs_[i];
				context.readingDirection = run.readingDirection();
				for(length_t j = run.beginning(); j < run.end(); ++j) {
					if(BinaryProperty::is(line[j], BinaryProperty::WHITE_SPACE)) {	// IdentifierSyntax.isWhiteSpace() is preferred?
						context.rect.setX(makeRange(x + run.x(j, false), x + run.x(j, true)));
						specialCharacterRenderer->drawWhiteSpaceCharacter(context, line[j]);
					} else if(isC0orC1Control(line[j])) {
						context.rect.setX(makeRange(x + run.x(j, false), x + run.x(j, true)));
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
			if(context.readingDirection == LEFT_TO_RIGHT)
				context.rect.setX(makeRange(lip_.width() - markWidth, lip_.width());
			else
				context.rect.setX(makeRange(0, markWidth));
			specialCharacterRenderer->drawLineWrappingMark(context);
		} else if(lineNumber_ < document.numberOfLines() - 1) {	// line teminator
			const kernel::Newline nlf = document.getLineInformation(lineNumber_).newline();
			const int nlfWidth = specialCharacterRenderer->getLineTerminatorWidth(context, nlf);
			if(context.readingDirection == LEFT_TO_RIGHT)
				context.rect.setX(makeRange(x, x + nlfWidth));
			else
				context.rect.setX(makeRange(x - nlfWidth, x));
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
#endif
	}
	context.restore();
}

#ifdef _DEBUG
/**
 * Dumps the all runs to the specified output stream.
 * @param out the output stream
 */
void LineLayout::dumpRuns(ostream& out) const {
	for(size_t i = 0; i < numberOfRuns_; ++i) {
		const TextRun& run = *runs_[i];
		out << static_cast<uint>(i)
			<< ":beginning=" << static_cast<uint>(run.beginning())
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
			TextRun& run = *runs_[i];
			run.expandTabCharacters(lineString, x, fullTabWidth, numeric_limits<int>::max());
			x += run.totalWidth();
		}
	} else {	// expand from the right most
		for(size_t i = numberOfRuns_; i > 0; --i) {
			TextRun& run = *runs_[i - 1];
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
		if(runs_[i]->beginning() <= column && runs_[i]->end() > column)	// TODO: replace with includes().
			return i;
	}
	assert(false);
	return lastRun - 1;	// never reachable...
}
#if 0
/// Returns an iterator addresses the first styled segment.
LineLayout::StyledSegmentIterator LineLayout::firstStyledSegment() const /*throw()*/ {
	const TextRun* temp = *runs_;
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

/// Justifies the wrapped visual lines.
inline void LineLayout::justify() /*throw()*/ {
	assert(wrapWidth_ != -1);
	for(length_t subline = 0; subline < numberOfSublines_; ++subline) {
		const int lineWidth = sublineWidth(subline);
		const size_t last = (subline + 1 < numberOfSublines_) ? sublineFirstRuns_[subline + 1] : numberOfRuns_;
		for(size_t i = sublineFirstRuns_[subline]; i < last; ++i) {
			TextRun& run = *runs_[i];
			const int newRunWidth = ::MulDiv(run.totalWidth(), wrapWidth_, lineWidth);	// TODO: there is more precise way.
			run.justify(newRunWidth);
		}
	}
}
#if 0
/// Returns an iterator addresses the last styled segment.
LineLayout::StyledSegmentIterator LineLayout::lastStyledSegment() const /*throw()*/ {
	const TextRun* temp = runs_[numberOfRuns_];
	return StyledSegmentIterator(temp);
}
#endif
/// Returns the line pitch in pixels.
inline int LineLayout::linePitch() const /*throw()*/ {
	return lip_.textMetrics().cellHeight() + max(lip_.layoutSettings().lineSpacing, lip_.textMetrics().lineGap());
}

// implements public location methods
void LineLayout::locations(length_t column, Point<>* leading, Point<>* trailing) const {
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
			const TextRun& run = *runs_[i];
			if(column >= run.beginning() && column <= run.end()) {
				if(leading != 0)
					leading->x = x + run.x(column, false);
				if(trailing != 0)
					trailing->x = x + run.x(column, true);
				break;
			}
			x += run.totalWidth();
		}
	} else {	// RTL
		int x = sublineIndent(sl) + sublineWidth(sl);
		for(size_t i = lastRun - 1; ; --i) {
			const TextRun& run = *runs_[i];
			x -= run.totalWidth();
			if(column >= run.beginning() && column <= run.end()) {
				if(leading != 0)
					leading->x = x + run.x(column, false);
				if(trailing)
					trailing->x = x + run.x(column, true);
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

/// Reorders the runs in visual order.
inline void LineLayout::reorder() /*throw()*/ {
	if(numberOfRuns_ == 0)
		return;
	TextRun** temp = new TextRun*[numberOfRuns_];
	memcpy(temp, runs_, sizeof(TextRun*) * numberOfRuns_);
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
		const TextRun& firstRun = *runs_[sublineFirstRuns_[subline]];
		result.first = result.second = firstRun.beginning()
			+ ((firstRun.readingDirection() == LEFT_TO_RIGHT) ? 0 : firstRun.length());	// TODO: used SCRIPT_ANALYSIS.fRTL past...
		return result;
	}
	for(size_t i = sublineFirstRuns_[subline]; i < lastRun; ++i) {
		const TextRun& run = *runs_[i];
		if(x >= cx && x <= cx + run.totalWidth()) {
			int cp, trailing;
			run.hitTest(x - cx, cp, trailing);	// TODO: check the returned value.
			if(outside != 0)
				*outside = false;
			result.first = run.beginning() + static_cast<length_t>(cp);
			result.second = result.first + static_cast<length_t>(trailing);
			return result;
		}
		cx += run.totalWidth();
	}
	// on the right margin
	if(outside != 0)
		*outside = true;
	result.first = result.second = runs_[lastRun - 1]->beginning()
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
	const TextRun& run = *runs_[findRunForPosition(column)];
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
Rect<> LineLayout::sublineBounds(length_t subline) const {
	if(subline >= numberOfSublines_)
		throw IndexOutOfBoundsException("subline");
	return Rect<>(
		Point<>(sublineIndent(subline), linePitch() * static_cast<long>(subline)),
		Dimension<>(sublineWidth(subline), linePitch()));
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

/// Locates the wrap points and resolves tab expansions.
void LineLayout::wrap(Context& context) /*throw()*/ {
	assert(numberOfRuns_ != 0 && lip_.layoutSettings().lineWrap.wraps());
	assert(numberOfSublines_ == 0 && sublineOffsets_ == 0 && sublineFirstRuns_ == 0);

	const String& lineString = text();
	vector<length_t> sublineFirstRuns;
	sublineFirstRuns.push_back(0);
	context.save();
	int x1 = 0;	// addresses the beginning of the run. see x2
	const int fullTabWidth = lip_.textMetrics().averageCharacterWidth() * lip_.layoutSettings().tabWidth;
	AutoBuffer<int> logicalWidths;
	AutoBuffer<SCRIPT_LOGATTR> logicalAttributes;
	length_t longestRunLength = 0;	// for efficient allocation
	vector<TextRun*> newRuns;
	newRuns.reserve(numberOfRuns_ * 3 / 2);
	// for each runs... (at this time, runs_ is in logical order)
	for(size_t i = 0; i < numberOfRuns_; ++i) {
		TextRun* run = runs_[i];

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
		const length_t originalRunPosition = run->beginning();
		int widthInThisRun = 0;
		length_t lastBreakable = run->beginning(), lastGlyphEnd = run->beginning();
		int lastBreakableX = x1, lastGlyphEndX = x1;
		// for each characters in the run...
		for(length_t j = run->beginning(); j < run->end(); ) {	// j is position in the LOGICAL line
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
				if(lastBreakable == run->beginning()) {
					// break at the last glyph boundary if no opportunities
					if(sublineFirstRuns.empty() || sublineFirstRuns.back() == newRuns.size()) {
						if(lastGlyphEnd == run->beginning()) {	// break here if no glyph boundaries
							lastBreakable = j;
							lastBreakableX = x2;
						} else {
							lastBreakable = lastGlyphEnd;
							lastBreakableX = lastGlyphEndX;
						}
					}
				}

				// case 1: break at the start of the run
				if(lastBreakable == run->beginning()) {
					assert(sublineFirstRuns.empty() || newRuns.size() != sublineFirstRuns.back());
					sublineFirstRuns.push_back(newRuns.size());
//dout << L"broke the line at " << lastBreakable << L" where the run start.\n";
				}
				// case 2: break at the end of the run
				else if(lastBreakable == run->end()) {
					if(lastBreakable < lineString.length()) {
						assert(sublineFirstRuns.empty() || newRuns.size() != sublineFirstRuns.back());
						sublineFirstRuns.push_back(newRuns.size() + 1);
//dout << L"broke the line at " << lastBreakable << L" where the run end.\n";
					}
					break;
				}
				// case 3: break at the middle of the run -> split the run (run -> newRun + run)
				else {
					auto_ptr<TextRun> followingRun(run->breakAt(context, lastBreakable, lineString, lip_));
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
	context.restore();
	if(newRuns.empty())
		newRuns.push_back(0);
	delete[] runs_;
	runs_ = new TextRun*[numberOfRuns_ = newRuns.size()];
	copy(newRuns.begin(), newRuns.end(), runs_);
	sublineFirstRuns_ = new length_t[numberOfSublines_ = sublineFirstRuns.size()];
	copy(sublineFirstRuns.begin(), sublineFirstRuns.end(), sublineFirstRuns_);
	sublineOffsets_ = new length_t[numberOfSublines_];
	for(size_t i = 0; i < numberOfSublines_; ++i)
		sublineOffsets_[i] = runs_[sublineFirstRuns_[i]]->beginning();
}


// LineLayout.Selection /////////////////////////////////////////////////////

/**
 * Constructor.
 * @param caret The caret holds a selection
 * @param foreground The foreground color
 * @param background The background color
 * @throw std#invalid_argument @a foreground and/or @c background is invalid
 */
LineLayout::Selection::Selection(const viewers::Caret& caret,
		const Color& foreground, const Color& background) /*throw()*/ : caret_(caret), foreground_(background_) {
	if(foreground == Color())
		throw invalid_argument("foreground");
	else if(background == Color())
		throw invalid_argument("background");
}

#if 0
// LineLayout.StyledSegmentIterator /////////////////////////////////////////

/**
 * Private constructor.
 * @param start
 */
LineLayout::StyledSegmentIterator::StyledSegmentIterator(const TextRun*& start) /*throw()*/ : p_(&start) {
}

/// Copy-constructor.
LineLayout::StyledSegmentIterator::StyledSegmentIterator(const StyledSegmentIterator& rhs) /*throw()*/ : p_(rhs.p_) {
}

/// Returns the current segment.
StyledRun LineLayout::StyledSegmentIterator::current() const /*throw()*/ {
	const TextRun& run = **p_;
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
LineLayoutBuffer::LineLayoutBuffer(k::Document& document, length_t bufferSize, bool autoRepair) :
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
		auto_ptr<Context> context;
		length_t newSublines = 0, actualFirst = last, actualLast = first;
		for(list<LineLayout*>::iterator i(layouts_.begin()); i != layouts_.end(); ++i) {
			LineLayout*& layout = *i;
			const length_t lineNumber = layout->lineNumber();
			if(lineNumber >= first && lineNumber < last) {
				oldSublines += layout->numberOfSublines();
				delete layout;
				if(context.get() == 0)
					context = renderingContext();
				layout = new LineLayout(*context, *lip_, lineNumber);
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
		const k::Region& region = change.erasedRegion();
		clearCaches(region.first.line + 1, region.second.line + 1, false);
		for(list<LineLayout*>::iterator i(layouts_.begin()), e(layouts_.end()); i != e; ++i) {
			if((*i)->lineNumber() > region.first.line)
				(*i)->lineNumber_ -= region.second.line - region.first.line;	// $friendly-access
		}
	}
	if(change.insertedRegion().first.line != change.insertedRegion().second.line) {	// inserted text is multiline
		const k::Region& region = change.insertedRegion();
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
	auto_ptr<Context> context(renderingContext());
	for(list<LineLayout*>::iterator i(layouts_.begin()), e(layouts_.end()); i != e; ++i) {
		LineLayout*& p = *i;
		if(p->lineNumber() == line) {
			const length_t oldSublines = p->numberOfSublines();
			delete p;
			if(autoRepair_) {
				p = new LineLayout(*context, *lip_, line);
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
		LineLayout* const layout = new LineLayout(*renderingContext(), *lip_, line);
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
length_t LineLayoutBuffer::mapLogicalPositionToVisualPosition(const k::Position& position, length_t* column) const {
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
k::Position LineLayoutBuffer::mapVisualPositionToLogicalPosition(const k::Position& position) const {
	if(!getTextViewer().getConfiguration().lineWrap.wraps())
		return position;
	k::Position result;
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
		whiteSpaceColor_(RGB(0x00, 0x80, 0x80)), showsEOLs_(true), showsWhiteSpaces_(true), font_() {
}

/// @see ISpecialCharacterRenderer#drawControlCharacter
void DefaultSpecialCharacterRenderer::drawControlCharacter(const DrawingContext& context, CodePoint c) const {
	win32::Handle<HDC> dc(context.renderingContext.engine()->nativeHandle());
	tr1::shared_ptr<const Font> primaryFont(renderer_->primaryFont());
	HFONT oldFont = static_cast<HFONT>(::SelectObject(dc.get(), primaryFont->nativeHandle().get()));
	::SetTextColor(dc.get(), controlColor_);
	Char buffer[2];
	getControlPresentationString(c, buffer);
	::ExtTextOutW(dc.get(), context.rect.x().beginning(), context.rect.y().beginning() + primaryFont->metrics().ascent(), 0, 0, buffer, 2, 0);
	::SelectObject(dc.get(), oldFont);
}

/// @see ISpecialCharacterRenderer#drawLineTerminator
void DefaultSpecialCharacterRenderer::drawLineTerminator(const DrawingContext& context, kernel::Newline) const {
	if(showsEOLs_ && glyphs_[LINE_TERMINATOR] != 0xffffu) {
		win32::Handle<HDC> dc(context.renderingContext.engine()->nativeHandle());
		tr1::shared_ptr<const Font> primaryFont(renderer_->primaryFont());
		HFONT oldFont = static_cast<HFONT>(::SelectObject(dc.get(),
			(((glyphWidths_[LINE_TERMINATOR] & 0x80000000ul) != 0) ? font_ : primaryFont)->nativeHandle().get()));
		::SetTextColor(dc.get(), eolColor_);
		::ExtTextOutW(dc.get(),
			context.rect.x().beginning(), context.rect.y().beginning() + primaryFont->metrics().ascent(),
			ETO_GLYPH_INDEX, 0, reinterpret_cast<const WCHAR*>(&glyphs_[LINE_TERMINATOR]), 1, 0);
		::SelectObject(dc.get(), oldFont);
	}
}

/// @see ISpecialCharacterRenderer#drawLineWrappingMark
void DefaultSpecialCharacterRenderer::drawLineWrappingMark(const DrawingContext& context) const {
	const int id = (context.readingDirection == LEFT_TO_RIGHT) ? LTR_WRAPPING_MARK : RTL_WRAPPING_MARK;
	const WCHAR glyph = glyphs_[id];
	if(glyph != 0xffffu) {
		win32::Handle<HDC> dc(context.renderingContext.engine()->nativeHandle());
		tr1::shared_ptr<const Font> primaryFont(renderer_->primaryFont());
		HFONT oldFont = static_cast<HFONT>(::SelectObject(dc.get(),
			(manah::toBoolean(glyphWidths_[id] & 0x80000000ul) ? font_ : primaryFont)->nativeHandle().get()));
		::SetTextColor(dc.get(), wrapMarkColor_);
		::ExtTextOutW(dc.get(),
			context.rect.x().beginning(), context.rect.y().beginning() + primaryFont->metrics().ascent(),
			ETO_GLYPH_INDEX, 0, &glyph, 1, 0);
		::SelectObject(dc.get(), oldFont);
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
			win32::Handle<HDC> dc(context.renderingContext.engine()->nativeHandle());
			tr1::shared_ptr<const Font> primaryFont(renderer_->primaryFont());
			HFONT oldFont = static_cast<HFONT>(::SelectObject(dc.get(),
				(manah::toBoolean(glyphWidths_[id] & 0x80000000ul) ? font_ : primaryFont)->nativeHandle().get()));
			const int glyphWidth = glyphWidths_[id] & 0x7ffffffful;
			const int x =
				((context.readingDirection == LEFT_TO_RIGHT && glyphWidth < context.rect.size().cx)
					|| (context.readingDirection == RIGHT_TO_LEFT && glyphWidth > context.rect.size().cx)) ?
				context.rect.x().beginning() : context.rect.x().end() - glyphWidth;
			::SetTextColor(dc.get(), whiteSpaceColor_);
			::ExtTextOutW(dc.get(),
				x, context.rect.y().beginning() + primaryFont->metrics().ascent(),
				ETO_CLIPPED | ETO_GLYPH_INDEX, &toNative(context.rect), &glyph, 1, 0);
			::SelectObject(dc.get(), oldFont);
		}
	} else if(glyphs_[WHITE_SPACE] != 0xffffu) {
		win32::Handle<HDC> dc(context.renderingContext.engine()->nativeHandle());
		tr1::shared_ptr<const Font> primaryFont(renderer_->primaryFont());
		HFONT oldFont = static_cast<HFONT>(::SelectObject(dc.get(),
			(manah::toBoolean(glyphWidths_[WHITE_SPACE] & 0x80000000ul) ? font_ : primaryFont)->nativeHandle().get()));
		::SetTextColor(dc.get(), whiteSpaceColor_);
		::ExtTextOutW(dc.get(),
			(context.rect.x().beginning() + context.rect.x().end() - (glyphWidths_[WHITE_SPACE] & 0x7ffffffful)) / 2,
			context.rect.y().beginning() + primaryFont->metrics().ascent(), ETO_CLIPPED | ETO_GLYPH_INDEX, &toNative(context.rect),
			reinterpret_cast<const WCHAR*>(&glyphs_[WHITE_SPACE]), 1, 0);
		::SelectObject(dc.get(), oldFont);
	}
}

/// @see IDefaultFontListener#defaultFontChanged
void DefaultSpecialCharacterRenderer::defaultFontChanged() {
	static const Char codes[] = {0x2192u, 0x2190u, 0x2193u, 0x21a9u, 0x21aau, 0x00b7u};

	// using the primary font
	win32::Handle<HDC> dc(::GetDC(0), bind1st(ptr_fun(&::ReleaseDC), 0));
	HFONT oldFont = static_cast<HFONT>(::SelectObject(dc.use(), renderer_->primaryFont()->nativeHandle().get()));
	::GetGlyphIndicesW(dc.get(), codes, ASCENSION_COUNTOF(codes), glyphs_, GGI_MARK_NONEXISTING_GLYPHS);
	::GetCharWidthI(dc.get(), 0, ASCENSION_COUNTOF(codes), glyphs_, glyphWidths_);

	// using the fallback font
	font_.reset();
	if(find(glyphs_, ASCENSION_ENDOF(glyphs_), 0xffffu) != ASCENSION_ENDOF(glyphs_)) {
		LOGFONTW lf;
		::GetObjectW(renderer_->primaryFont()->nativeHandle().get(), sizeof(LOGFONTW), &lf);
		lf.lfWeight = FW_REGULAR;
		lf.lfItalic = lf.lfUnderline = lf.lfStrikeOut = false;
		wcscpy(lf.lfFaceName, L"Lucida Sans Unicode");
		font_.reset(::CreateFontIndirectW(&lf), &::DeleteObject);
		::SelectObject(dc.get(), font_.get()->nativeHandle().use());
		WORD g[ASCENSION_COUNTOF(glyphs_)];
		int w[ASCENSION_COUNTOF(glyphWidths_)];
		::GetGlyphIndicesW(dc.get(), codes, ASCENSION_COUNTOF(codes), g, GGI_MARK_NONEXISTING_GLYPHS);
		::GetCharWidthI(dc.get(), 0, ASCENSION_COUNTOF(codes), g, w);
		for(int i = 0; i < ASCENSION_COUNTOF(glyphs_); ++i) {
			if(glyphs_[i] == 0xffffu) {
				if(g[i] != 0xffff) {
					glyphs_[i] = g[i];
					glyphWidths_[i] = w[i] | 0x80000000ul;
				} else
					glyphWidths_[i] = 0;	// missing
			}
		}
	}

	::SelectObject(dc.get(), oldFont);
}

/// @see ISpecialCharacterRenderer#getControlCharacterWidth
int DefaultSpecialCharacterRenderer::getControlCharacterWidth(const LayoutContext& context, CodePoint c) const {
	Char buffer[2];
	getControlPresentationString(c, buffer);
	win32::Handle<HDC> dc(context.renderingContext.engine()->nativeHandle());
	HFONT oldFont = static_cast<HFONT>(::SelectObject(dc.get(), renderer_->primaryFont()->nativeHandle().get()));
	SIZE temp;
	::GetTextExtentPoint32W(dc.get(), buffer, 2, &temp);
	::SelectObject(dc.get(), oldFont);
	return temp.cx;
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
	AutoBuffer<WCHAR> ASCENSION_FASTCALL mapFontFileNameToTypeface(const WCHAR* fileName) {
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
				AutoBuffer<WCHAR> valueName(new WCHAR[maximumValueNameLength + 1]);
				AutoBuffer<BYTE> value(new BYTE[maximumValueBytes]);
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
								AutoBuffer<WCHAR> temp(new WCHAR[nameLength + 1]);
								wmemcpy(temp.get(), valueName.get(), nameLength);
								temp[nameLength] = 0;
								return temp;
							} else
								return AutoBuffer<WCHAR>(0);
						}
					} else	// ERROR_NO_MORE_ITEMS
						break;
				}
			}
			::RegCloseKey(key);
		}
		return AutoBuffer<WCHAR>(0);
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
			AutoBuffer<BYTE> data(new BYTE[bytes]);
			if(ERROR_SUCCESS == ::RegQueryValueExW(key, primaryFont_->faceName, 0, &type, data.get(), &bytes)) {
				const WCHAR* sz = reinterpret_cast<WCHAR*>(data.get());
				const WCHAR* const e = sz + bytes / sizeof(WCHAR);
				for(; sz < e; sz += wcslen(sz) + 1) {
					const WCHAR* comma = wcschr(sz, L',');
					if(comma != 0 && comma[1] != 0)	// "<file-name>,<typeface>"
						linkedFonts_->push_back(new Fontset(comma + 1));
					else {	// "<file-name>"
						AutoBuffer<WCHAR> typeface(mapFontFileNameToTypeface(sz));
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
TextRenderer::TextRenderer(Presentation& presentation, const FontCollection& fontCollection, bool enableDoubleBuffering) :
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
	static_cast<presentation::internal::ITextRendererCollection&>(presentation).addTextRenderer(*this);
}

/// Copy-constructor.
TextRenderer::TextRenderer(const TextRenderer& other) :
		LineLayoutBuffer(other.presentation_.document(), ASCENSION_DEFAULT_LINE_LAYOUT_CACHE_SIZE, true),
		presentation_(other.presentation_), fontCollection_(other.fontCollection_),
		enablesDoubleBuffering_(other.enablesDoubleBuffering_), primaryFont_() {
	setLayoutInformation(this, false);
	updateTextMetrics();
//	updateViewerSize(); ???
	static_cast<presentation::internal::ITextRendererCollection&>(presentation_).addTextRenderer(*this);
}

/// Destructor.
TextRenderer::~TextRenderer() /*throw()*/ {
	static_cast<presentation::internal::ITextRendererCollection&>(presentation_).removeTextRenderer(*this);
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
		BITMAP temp;
		::GetObjectW(memoryBitmap_.use(), sizeof(HBITMAP), &temp);
		if(temp.bmHeight != calculateMemoryBitmapSize(primaryFont()->metrics().linePitch()))
			memoryBitmap_.reset();
	}
	listeners_.notify(&IDefaultFontListener::defaultFontChanged);
}

/// @see ILayoutInformationProvider#fontCollection
const FontCollection& TextRenderer::fontCollection() const {
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
 * @param context the graphics context
 * @param x the x-coordinate of the position to draw
 * @param y the y-coordinate of the position to draw
 * @param paintRect the region to draw
 * @param clipRect the clipping region
 * @param selection the selection
 */
void TextRenderer::renderLine(length_t line, Context& context, int x, int y,
		const Rect<>& paintRect, const Rect<>& clipRect, const LineLayout::Selection* selection) const /*throw()*/ {
	if(!enablesDoubleBuffering_) {
		lineLayout(line).draw(context, x, y, paintRect, clipRect, selection);
		return;
	}

	const LineLayout& layout = lineLayout(line);
	const int dy = textMetrics().linePitch();

	// skip to the subline needs to draw
	const int top = max(paintRect.y().beginning(), clipRect.y().beginning());
	length_t subline = (y + dy >= top) ? 0 : (top - (y + dy)) / dy;
	if(subline >= layout.numberOfSublines())
		return;	// this logical line does not need to draw
	y += static_cast<int>(dy * subline);

	if(memoryDC_.get() == 0)		
		memoryDC_.reset(::CreateCompatibleDC(renderingContext()->engine()->nativeHandle().get()), &::DeleteDC);
	const int horizontalResolution = calculateMemoryBitmapSize(context.device()->size().cx);
	if(memoryBitmap_.get() != 0) {
		BITMAP temp;
		::GetObjectW(memoryBitmap_.use(), sizeof(HBITMAP), &temp);
		if(temp.bmWidth < horizontalResolution)
			memoryBitmap_.reset();
	}
	if(memoryBitmap_.get() == 0)
		memoryBitmap_.reset(::CreateCompatibleBitmap(
			renderingContext()->engine()->nativeHandle().get(),
			horizontalResolution, calculateMemoryBitmapSize(dy)), &::DeleteObject);
	::SelectObject(memoryDC_.get(), memoryBitmap_.use());

	const int left = max(paintRect.x().beginning(), clipRect.x().beginning());
	const int right = min(paintRect.x().end(), clipRect.x().end());
	x -= left;
	Rect<> offsetedPaintRect(paintRect), offsetedClipRect(clipRect);
	offsetedPaintRect.translate(Dimension<>(-left, -y));
	offsetedClipRect.translate(Dimension<>(-left, -y));
	for(; subline < layout.numberOfSublines() && offsetedPaintRect.y().end() >= 0;
			++subline, y += dy, offsetedPaintRect.translate(Dimension<>(0, -dy)), offsetedClipRect.translate(Dimension<>(0, -dy))) {
		layout.draw(subline, memoryDC_, x, 0, offsetedPaintRect, offsetedClipRect, selection);
		::BitBlt(context.engine()->nativeHandle().get(), left, y, right - left, dy, memoryDC_.get(), 0, 0, SRCCOPY);
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
