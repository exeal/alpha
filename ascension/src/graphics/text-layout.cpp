/**
 * @file layout.cpp
 * @author exeal
 * @date 2003-2006 (was TextLayout.cpp)
 * @date 2006-2010
 * @date 2010-11-20 renamed from ascension/layout.cpp
 */

#include <ascension/config.hpp>			// ASCENSION_DEFAULT_LINE_LAYOUT_CACHE_SIZE, ...
#include <ascension/graphics/text-layout.hpp>
#include <ascension/graphics/graphics.hpp>
#include <ascension/graphics/special-character-renderer.hpp>
#include <ascension/corelib/unicode-property.hpp>
#include <ascension/viewer/caret.hpp>	// Caret.isSelectionRectangle, viewers.selectedRangeOnVisualLine
#include <limits>	// std.numeric_limits
#include <numeric>	// std.accumulate
#include <vector>
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
	ReadingDirection lineTerminatorOrientation(const TextLineStyle& style, tr1::shared_ptr<const TextLineStyle> defaultStyle) /*throw()*/;
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

	void dumpRuns(const TextLayout& layout) {
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

	/**
	 * Returns metrics of underline and/or strikethrough for the currently selected font.
	 * @param dc the device context
	 * @param[out] baselineOffset The baseline position relative to the top in pixels
	 * @param[out] underlineOffset The underline position relative to the baseline in pixels
	 * @param[out] underlineThickness The thickness of underline in pixels
	 * @param[out] strikethroughOffset The linethrough position relative to the baseline in pixels
	 * @param[out] strikethroughThickness The thickness of linethrough in pixels
	 * @return Succeeded or not
	 */
	bool getDecorationLineMetrics(const win32::Handle<HDC>& dc, int* baselineOffset,
			int* underlineOffset, int* underlineThickness, int* strikethroughOffset, int* strikethroughThickness) /*throw()*/ {
		OUTLINETEXTMETRICW* otm = 0;
		TEXTMETRICW tm;
		if(const UINT c = ::GetOutlineTextMetricsW(dc.get(), 0, 0)) {
			otm = static_cast<OUTLINETEXTMETRICW*>(::operator new(c));
			if(!win32::boole(::GetOutlineTextMetricsW(dc.get(), c, otm)))
				return false;
		} else if(!win32::boole(::GetTextMetricsW(dc.get(), &tm)))
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


	inline ReadingDirection lineTerminatorOrientation(const TextLineStyle& style, tr1::shared_ptr<const TextLineStyle> defaultStyle) /*throw()*/ {
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
				const double dpi = vertical ? context.logicalDpiY() : context.logicalDpiX();
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

/// Returns @c true if complex scripts are supported.
bool font::supportsComplexScripts() /*throw()*/ {
	return true;
}

/// Returns @c true if OpenType features are supported.
bool font::supportsOpenTypeFeatures() /*throw()*/ {
	return uspLib->get<0>() != 0;
}


// TextLayout.TextRun /////////////////////////////////////////////////////////////////////////////

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
	class SimpleStyledTextRunIterator : public IStyledTextRunIterator {
	public:
		SimpleStyledTextRunIterator(const Range<const StyledTextRun*>& range, length_t start) : range_(range) {
			current_ = range_.beginning() + ascension::internal::searchBound(
				static_cast<ptrdiff_t>(0), range_.length(), start, BeginningOfStyledTextRun(range_.beginning()));
		}
		// presentation.IStyledTextRunIterator
		void current(StyledTextRun& run) const {
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
		struct BeginningOfStyledTextRun {
			explicit BeginningOfStyledTextRun(const StyledTextRun* range) : range(range) {}
			length_t operator()(ptrdiff_t i) {return range[i].column;}
			const StyledTextRun* range;
		};
		const Range<const StyledTextRun*> range_;
		const StyledTextRun* current_;
	};
}

/// TextRun class represents minimum text run whose characters can shaped by single font.
class TextLayout::TextRun : public Range<length_t> {	// beginning() and end() return position in the line
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
		const String& layoutString, const ILayoutInformationProvider& lip);
	void blackBoxBounds(const Range<length_t>& range, Rect<>& bounds) const;
	void drawBackground(Context& context, const Point<>& p,
		const Range<length_t>& range, const Color& color, const Rect<>* dirtyRect, Rect<>* bounds) const;
	void drawForeground(Context& context, const Point<>& p,
		const Range<length_t>& range, const Color& color, const Rect<>* dirtyRect, const Overlay* overlay) const;
	bool expandTabCharacters(const String& layoutString, int x, int tabWidth, int maximumWidth);
	tr1::shared_ptr<const Font> font() const {return glyphs_->font;}
	HRESULT hitTest(int x, int& cp, int& trailing) const;
	HRESULT justify(int width);
	HRESULT logicalAttributes(const String& layoutString, SCRIPT_LOGATTR attributes[]) const;
	HRESULT logicalWidths(int widths[]) const;
	static void mergeScriptsAndStyles(Context& context, const String& layoutString, const SCRIPT_ITEM scriptRuns[],
		const OPENTYPE_TAG scriptTags[], size_t numberOfScriptRuns, auto_ptr<IStyledTextRunIterator> styles,
		const ILayoutInformationProvider& lip, vector<TextRun*>& textRuns, vector<const StyledTextRun>& styledRanges);
	int numberOfGlyphs() const /*throw()*/ {return glyphRange_.length();}
	void positionGlyphs(const Context& context, const String& layoutString, SimpleStyledTextRunIterator& styles);
	ReadingDirection readingDirection() const /*throw()*/ {
		return ((analysis_.s.uBidiLevel & 0x01) == 0x00) ? LEFT_TO_RIGHT : RIGHT_TO_LEFT;}
	void shape(Context& context, const String& layoutString, const ILayoutInformationProvider& lip);
	auto_ptr<TextRun> splitIfTooLong(const String& layoutString);
	static void substituteGlyphs(const Context& context, const Range<TextRun**>& runs, const String& layoutString);
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

void TextLayout::TextRun::Glyphs::vanish(const Context& context, size_t at) {
	assert(advances.get() == 0);
	WORD blankGlyph;
	HRESULT hr = ::ScriptGetCMap(context.nativeHandle().get(), &fontCache, L"\x0020", 1, 0, &blankGlyph);
	if(hr == S_OK) {
		SCRIPT_FONTPROPERTIES fp;
		fp.cBytes = sizeof(SCRIPT_FONTPROPERTIES);
		if(FAILED(hr = ::ScriptGetFontProperties(context.nativeHandle().get(), &fontCache, &fp)))
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
TextLayout::TextRun::TextRun(const Range<length_t>& characterRange,
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
TextLayout::TextRun::TextRun(TextRun& leading, length_t characterBoundary) /*throw()*/ :
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

TextLayout::TextRun::~TextRun() /*throw()*/ {
//	if(cache_ != 0)
//		::ScriptFreeCache(&cache_);
}

void TextLayout::TextRun::blackBoxBounds(const Range<length_t>& range, Rect<>& bounds) const {
	const int left = x(max(range.beginning(), beginning()), false);
	const int right = x(min(range.end(), end()) - 1, true);
	const Font::Metrics& fontMetrics = glyphs_->font->metrics();
	bounds = Rect<>(
		Point<>(left, -fontMetrics.ascent()),
		Dimension<>(right - left, fontMetrics.cellHeight())).normalize();
}

auto_ptr<TextLayout::TextRun> TextLayout::TextRun::breakAt(
		Context& context, length_t at, const String& layoutString, const ILayoutInformationProvider& lip) {
	assert(at > beginning() && at < end());
	assert(glyphs_->clusters[at - beginning()] != glyphs_->clusters[at - beginning() - 1]);

	const bool ltr = readingDirection() == LEFT_TO_RIGHT;
	const length_t newLength = at - beginning();
	assert(ltr == (analysis_.fRTL == 0));

	// create the new following run
	auto_ptr<TextRun> following(new TextRun(*this, newLength));

	// update placements
//	place(context, layoutString, lip);
//	following->place(dc, layoutString, lip);

	return following;
}

/**
 * Returns the number of missing glyphs in this run.
 * @param dc the device context
 * @param run the run
 * @return the number of missing glyphs
 */
inline pair<int, HRESULT> TextLayout::TextRun::countMissingGlyphs(
		const Context& context, const Char* text) const /*throw()*/ {
	SCRIPT_FONTPROPERTIES fp;
	fp.cBytes = sizeof(SCRIPT_FONTPROPERTIES);
	const HRESULT hr = ::ScriptGetFontProperties(context.nativeHandle().get(), &glyphs_->fontCache, &fp);
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
void TextLayout::TextRun::drawBackground(Context& context, const Point<>& p,
		const Range<length_t>& range, const Color& color, const Rect<>* dirtyRect, Rect<>* bounds) const {
	if(color == Color())
		throw invalid_argument("color");
	if(range.isEmpty() || (dirtyRect != 0 && p.x + totalWidth() < dirtyRect->left()))
		return;
	Rect<> r;
	blackBoxBounds(range, r);
	context.fillRectangle(r.translate(p), color);
	if(bounds != 0)
		*bounds = r;
}

void TextLayout::TextRun::drawForeground(Context& context, const Point<>& p,
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
		::SetTextColor(context.nativeHandle().get(), color.asCOLORREF());
		RECT temp;
		if(dirtyRect != 0)
			::SetRect(&temp, dirtyRect->left(), dirtyRect->top(), dirtyRect->right(), dirtyRect->bottom());
		const HRESULT hr = ::ScriptTextOut(context.nativeHandle().get(), &glyphs_->fontCache,
			p.x + x((analysis_.fRTL == 0) ? truncatedRange.beginning() : (truncatedRange.end() - 1), analysis_.fRTL != 0),
			p.y - glyphs_->font->metrics().ascent(), 0, (dirtyRect != 0) ? &temp : 0, &analysis_, 0, 0,
			glyphs() + glyphRange.beginning(), glyphRange.length(), advances() + glyphRange.beginning(),
			(justifiedAdvances() != 0) ? justifiedAdvances() + glyphRange.beginning() : 0,
			glyphOffsets() + glyphRange.beginning());
	}
}

/**
 * Expands tab characters in this run and modifies the width.
 * @param layoutString the whole string of the layout this run belongs to
 * @param x the position in writing direction this run begins, in pixels
 * @param tabWidth the maximum expanded tab width, in pixels
 * @param maximumWidth the maximum width this run can take place, in pixels
 * @return @c true if expanded tab characters
 * @throw std#invalid_argument @a maximumWidth &lt;= 0
 */
inline bool TextLayout::TextRun::expandTabCharacters(const String& layoutString, int x, int tabWidth, int maximumWidth) {
	if(maximumWidth <= 0)
		throw invalid_argument("maximumWidth");
	if(layoutString[beginning()] != L'\t')
		return false;
	assert(length() == 1 && glyphs_.unique());
	glyphs_->advances[0] = min(tabWidth - x % tabWidth, maximumWidth);
	glyphs_->justifiedAdvances.reset();
	return true;
}

/// Fills the glyph array with default index, instead of using @c ScriptShape.
inline void TextLayout::TextRun::generateDefaultGlyphs(const Context& context,
		const StringPiece& text, const SCRIPT_ANALYSIS& analysis, Glyphs& glyphs) {
	SCRIPT_CACHE fontCache(0);
	SCRIPT_FONTPROPERTIES fp;
	fp.cBytes = sizeof(SCRIPT_FONTPROPERTIES);
	if(FAILED(::ScriptGetFontProperties(context.nativeHandle().get(), &fontCache, &fp)))
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
HRESULT TextLayout::TextRun::generateGlyphs(const Context& context,
		const StringPiece& text, const SCRIPT_ANALYSIS& analysis, Glyphs& glyphs, int& numberOfGlyphs) {
#ifdef _DEBUG
	if(HFONT currentFont = static_cast<HFONT>(::GetCurrentObject(context.nativeHandle().get(), OBJ_FONT))) {
		LOGFONTW lf;
		if(::GetObjectW(currentFont, sizeof(LOGFONTW), &lf) > 0) {
			win32::DumpContext dout;
			dout << L"[TextLayout.TextRun.generateGlyphs] Selected font is '" << lf.lfFaceName << L"'.\n";
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
		hr = ::ScriptShape(context.nativeHandle().get(), &fontCache,
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

inline HRESULT TextLayout::TextRun::hitTest(int x, int& cp, int& trailing) const {
	return ::ScriptXtoCP(x, static_cast<int>(length()), numberOfGlyphs(), clusters(),
		visualAttributes(), (justifiedAdvances() == 0) ? advances() : justifiedAdvances(), &analysis_, &cp, &trailing);
}

inline HRESULT TextLayout::TextRun::justify(int width) {
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

inline HRESULT TextLayout::TextRun::logicalAttributes(const String& layoutString, SCRIPT_LOGATTR attributes[]) const {
	if(attributes == 0)
		throw NullPointerException("attributes");
	return ::ScriptBreak(layoutString.data() + beginning(), static_cast<int>(length()), &analysis_, attributes);
}

inline HRESULT TextLayout::TextRun::logicalWidths(int widths[]) const {
	if(widths == 0)
		throw NullPointerException("widths");
	return ::ScriptGetLogicalWidths(&analysis_, static_cast<int>(length()),
		numberOfGlyphs(), advances(), clusters(), visualAttributes(), widths);
}

namespace {
	void resolveFontSpecifications(const ILayoutInformationProvider& lip,
			tr1::shared_ptr<const TextRunStyle> requestedStyle, String& computedFamilyName,
			FontProperties& computedProperties, double& computedSizeAdjust) {
		tr1::shared_ptr<const TextRunStyle> defaultStyle(lip.presentation().defaultTextRunStyle());
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
		double computedSize = computedProperties.size();
		if(computedSize == 0.0f) {
			if(defaultStyle.get() != 0)
				computedSize = defaultStyle->fontProperties.size();
			if(computedSize == 0.0f)
				computedSize = lip.textMetrics().emHeight();
		}
		computedProperties = FontProperties(
			(computedProperties.weight() != FontProperties::INHERIT_WEIGHT) ? computedProperties.weight()
				: ((defaultStyle.get() != 0) ? defaultStyle->fontProperties.weight() : FontProperties::NORMAL_WEIGHT),
			(computedProperties.stretch() != FontProperties::INHERIT_STRETCH) ? computedProperties.stretch()
				: ((defaultStyle.get() != 0) ? defaultStyle->fontProperties.stretch() : FontProperties::NORMAL_STRETCH),
			(computedProperties.style() != FontProperties::INHERIT_STYLE) ? computedProperties.style()
				: ((defaultStyle.get() != 0) ? defaultStyle->fontProperties.style() : FontProperties::NORMAL_STYLE),
			(computedProperties.orientation() != FontProperties::INHERIT_ORIENTATION) ? computedProperties.orientation()
				: ((defaultStyle.get() != 0) ? defaultStyle->fontProperties.orientation() : FontProperties::HORIZONTAL),
			computedSize);
		// size-adjust
		computedSizeAdjust = (requestedStyle.get() != 0) ? requestedStyle->fontSizeAdjust : -1.0;
		if(computedSizeAdjust < 0.0)
			computedSizeAdjust = (defaultStyle.get() != 0) ? defaultStyle->fontSizeAdjust : 0.0;
	}
	pair<const Char*, tr1::shared_ptr<const Font> > findNextFontRun(
			const Range<const Char*>& text, tr1::shared_ptr<const TextRunStyle> requestedStyle,
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
 * @param context The graphics context
 * @param layoutString
 * @param items The items itemized by @c #itemize()
 * @param numberOfItems The length of the array @a items
 * @param styles The iterator returns the styled runs in the line. Can be @c null
 * @param lip
 * @param[out] textRuns
 * @param[out] styledRanges
 * @see presentation#Presentation#getLineStyle
 */
void TextLayout::TextRun::mergeScriptsAndStyles(Context& context, const String& layoutString, const SCRIPT_ITEM scriptRuns[],
		const OPENTYPE_TAG scriptTags[], size_t numberOfScriptRuns, auto_ptr<IStyledTextRunIterator> styles,
		const ILayoutInformationProvider& lip, vector<TextRun*>& textRuns, vector<const StyledTextRun>& styledRanges) {
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

	pair<vector<TextRun*>, vector<const StyledTextRun> > results;
	results.first.reserve(static_cast<size_t>(numberOfScriptRuns * ((styles.get() != 0) ? 1.2 : 1)));	// hmm...

	const SCRIPT_ITEM* scriptRun = scriptRuns;
	pair<const SCRIPT_ITEM*, length_t> nextScriptRun;	// 'second' is the beginning position
	nextScriptRun.first = (numberOfScriptRuns > 1) ? (scriptRuns + 1) : 0;
	nextScriptRun.second = (nextScriptRun.first != 0) ? nextScriptRun.first->iCharPos : layoutString.length();
	pair<StyledTextRun, bool> styleRun;	// 'second' is false if 'first' is invalid
	if(styleRun.second = styles.get() != 0 && styles->hasNext()) {
		styles->current(styleRun.first);
		styles->next();
		results.second.push_back(styleRun.first);
	}
	pair<StyledTextRun, bool> nextStyleRun;	// 'second' is false if 'first' is invalid
	if(nextStyleRun.second = styles.get() != 0 && styles->hasNext())
		styles->current(nextStyleRun.first);
	length_t beginningOfNextStyleRun = nextStyleRun.second ? nextStyleRun.first.column : layoutString.length();
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

		if(surrogates::next(layoutString.data() + previousRunEnd,
				layoutString.data() + newRunEnd) < layoutString.data() + newRunEnd || font.get() == 0) {
			const pair<const Char*, tr1::shared_ptr<const Font> > nextFontRun(
				findNextFontRun(
					Range<const Char*>(layoutString.data() + previousRunEnd, layoutString.data() + newRunEnd),
					styleRun.second ? styleRun.first.style : tr1::shared_ptr<const TextRunStyle>(), font, lip));
			font = nextFontRun.second;
			if(nextFontRun.first != 0) {
				forwardGlyphRun = true;
				newRunEnd = nextFontRun.first - layoutString.data();
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
				auto_ptr<TextRun> piece(results.first.back()->splitIfTooLong(layoutString));
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
				nextScriptRun.second = (nextScriptRun.first != 0) ? nextScriptRun.first->iCharPos : layoutString.length();
			}
		}
		if(forwardStyleRun) {
			if(styleRun.second = nextStyleRun.second) {
				styleRun.first = nextStyleRun.first;
				results.second.push_back(styleRun.first);
				styles->next();
				if(nextStyleRun.second = styles->hasNext())
					styles->current(nextStyleRun.first);
				beginningOfNextStyleRun = nextStyleRun.second ? nextStyleRun.first.column : layoutString.length();
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
void TextLayout::TextRun::positionGlyphs(const Context& context, const String& layoutString, SimpleStyledTextRunIterator& styles) {
	assert(glyphs_.get() != 0 && glyphs_.unique());
	assert(glyphs_->indices.get() != 0 && glyphs_->advances.get() == 0);

	AutoBuffer<int> advances(new int[numberOfGlyphs()]);
	AutoBuffer<GOFFSET> offsets(new GOFFSET[numberOfGlyphs()]);
//	ABC width;
	HRESULT hr = ::ScriptPlace(0, &glyphs_->fontCache, glyphs_->indices.get(), numberOfGlyphs(),
		glyphs_->visualAttributes.get(), &analysis_, advances.get(), offsets.get(), 0/*&width*/);
	if(hr == E_PENDING) {
		HFONT oldFont = static_cast<HFONT>(::SelectObject(context.nativeHandle().get(), glyphs_->font->nativeHandle().get()));
		hr = ::ScriptPlace(context.nativeHandle().get(), &glyphs_->fontCache, glyphs_->indices.get(), numberOfGlyphs(),
			glyphs_->visualAttributes.get(), &analysis_, advances.get(), offsets.get(), 0/*&width*/);
		::SelectObject(context.nativeHandle().get(), oldFont);
	}
	if(FAILED(hr))
		throw hr;

	// apply text run styles
	for(; styles.hasNext(); styles.next()) {
		StyledTextRun styledRange;
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
				if(isC0orC1Control(layoutString[i])) {
					if(const int w = scr->getControlCharacterWidth(context, layoutString[i])) {
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

void TextLayout::TextRun::shape(Context& context, const String& layoutString, const ILayoutInformationProvider& lip) {
	assert(glyphs_.unique());

	// TODO: check if the requested style (or the default one) disables shaping.

	context.setFont(*glyphs_->font);
	const StringPiece text(layoutString.data() + beginning(), layoutString.data() + end());
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
void TextLayout::TextRun::shape(DC& dc, const String& layoutString, const ILayoutInformationProvider& lip, TextRun* nextRun) {
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
		if(USP_E_SCRIPT_NOT_IN_FONT == (hr = buildGlyphs(dc, layoutString.data()))) {
			analysis_.eScript = SCRIPT_UNDEFINED;
			hr = buildGlyphs(dc, layoutString.data());
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

		const Char* textString = layoutString.data() + beginning();

#define ASCENSION_MAKE_TEXT_STRING_SAFE()												\
	assert(safeString.get() == 0);														\
	safeString.reset(new Char[length()]);												\
	wmemcpy(safeString.get(), layoutString.data() + beginning(), length());				\
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
				&& surrogates::isHighSurrogate(layoutString[beginning()]) && surrogates::isLowSurrogate(layoutString[beginning() + 1])) {
			for(StringCharacterIterator i(
					StringPiece(layoutString.data() + beginning(), length()), layoutString.data() + beginning() + 2); i.hasNext(); i.next()) {
				const CodePoint variationSelector = i.current();
				if(variationSelector >= 0xe0100ul && variationSelector <= 0xe01eful) {
					StringCharacterIterator baseCharacter(i);
					baseCharacter.previous();
					if(static_cast<const SystemFont*>(font_.get())->ivsGlyph(
							baseCharacter.current(), variationSelector,
							glyphs_->indices[glyphs_->clusters[baseCharacter.tell() - layoutString.data()]])) {
						ASCENSION_VANISH_VARIATION_SELECTOR(i.tell() - layoutString.data());
					}
				}
			}
		}
		if(nextRun != 0 && nextRun->length() > 1) {
			const CodePoint variationSelector = surrogates::decodeFirst(
				layoutString.begin() + nextRun->beginning(), layoutString.begin() + nextRun->beginning() + 2);
			if(variationSelector >= 0xe0100ul && variationSelector <= 0xe01eful) {
				const CodePoint baseCharacter = surrogates::decodeLast(
					layoutString.data() + beginning(), layoutString.data() + end());
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
auto_ptr<TextLayout::TextRun> TextLayout::TextRun::splitIfTooLong(const String& layoutString) {
	if(estimateNumberOfGlyphs(length()) <= 65535)
		return auto_ptr<TextRun>();

	// split this run, because the length would cause ScriptShape to fail (see also Mozilla bug 366643).
	static const length_t MAXIMUM_RUN_LENGTH = 43680;	// estimateNumberOfGlyphs(43680) == 65536
	length_t opportunity = 0;
	AutoBuffer<SCRIPT_LOGATTR> la(new SCRIPT_LOGATTR[length()]);
	const HRESULT hr = logicalAttributes(layoutString, la.get());
	if(SUCCEEDED(hr)) {
		for(length_t i = MAXIMUM_RUN_LENGTH; i > 0; --i) {
			if(la[i].fCharStop != 0) {
				if(legacyctype::isspace(layoutString[i]) || legacyctype::isspace(layoutString[i - 1])) {
					opportunity = i;
					break;
				}
				opportunity = max(i, opportunity);
			}
		}
	}
	if(opportunity == 0) {
		opportunity = MAXIMUM_RUN_LENGTH;
		if(surrogates::isLowSurrogate(layoutString[opportunity]) && surrogates::isHighSurrogate(layoutString[opportunity - 1]))
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
 * @param layoutString the whole string of the layout
 * @see #merge, #positionGlyphs
 */
void TextLayout::TextRun::substituteGlyphs(const Context& context, const Range<TextRun**>& runs, const String& layoutString) {
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
					&& surrogates::isHighSurrogate(layoutString[run.beginning()])
					&& surrogates::isLowSurrogate(layoutString[run.beginning() + 1])) {
				for(StringCharacterIterator i(StringPiece(layoutString.data() + run.beginning(),
						run.length()), layoutString.data() + run.beginning() + 2); i.hasNext(); i.next()) {
					const CodePoint variationSelector = i.current();
					if(variationSelector >= 0xe0100ul && variationSelector <= 0xe01eful) {
						StringCharacterIterator baseCharacter(i);
						baseCharacter.previous();
						if(run.glyphs_->font->ivsGlyph(
								baseCharacter.current(), variationSelector,
								run.glyphs_->indices[run.glyphs_->clusters[baseCharacter.tell() - layoutString.data()]])) {
							run.glyphs_->vanish(context, i.tell() - layoutString.data() - run.beginning());
							run.glyphs_->vanish(context, i.tell() - layoutString.data() - run.beginning() + 1);
						}
					}
				}
			}

			// process an IVS across two glyph runs
			if(p + 1 != runs.end() && p[1]->length() > 1) {
				TextRun& next = *p[1];
				const CodePoint variationSelector = surrogates::decodeFirst(
					layoutString.begin() + next.beginning(), layoutString.begin() + next.beginning() + 2);
				if(variationSelector >= 0xe0100ul && variationSelector <= 0xe01eful) {
					const CodePoint baseCharacter = surrogates::decodeLast(
						layoutString.data() + run.beginning(), layoutString.data() + run.end());
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

inline int TextLayout::TextRun::x(length_t at, bool trailing) const {
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


// TextLayout ///////////////////////////////////////////////////////////////

// helpers for TextLayout.draw
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
	inline void drawDecorationLines(Context& context, const TextRunStyle& style, const Color& foregroundColor, int x, int y, int width, int height) {
		if(style.decorations.underline.style != Decorations::NONE || style.decorations.strikethrough.style != Decorations::NONE) {
			const win32::Handle<HDC>& dc = context.nativeHandle();
			int baselineOffset, underlineOffset, underlineThickness, linethroughOffset, linethroughThickness;
			if(getDecorationLineMetrics(dc, &baselineOffset, &underlineOffset, &underlineThickness, &linethroughOffset, &linethroughThickness)) {
				// draw underline
				if(style.decorations.underline.style != Decorations::NONE) {
					win32::Handle<HPEN> pen(createPen((style.decorations.underline.color != Color()) ?
						style.decorations.underline.color : foregroundColor, underlineThickness, style.decorations.underline.style));
					HPEN oldPen = static_cast<HPEN>(::SelectObject(dc.get(), pen.get()));
					const int underlineY = y + baselineOffset - underlineOffset + underlineThickness / 2;
					::MoveToEx(dc.get(), x, underlineY, 0);
					::LineTo(dc.get(), x + width, underlineY);
					::SelectObject(dc.get(), oldPen);
				}
				// draw strikethrough line
				if(style.decorations.strikethrough.style != Decorations::NONE) {
					win32::Handle<HPEN> pen(createPen((style.decorations.strikethrough.color != Color()) ?
						style.decorations.strikethrough.color : foregroundColor, linethroughThickness, 1));
					HPEN oldPen = static_cast<HPEN>(::SelectObject(dc.get(), pen.get()));
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
		const win32::Handle<HDC>& dc = context.nativeHandle();
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
					HPEN oldPen = static_cast<HPEN>(::SelectObject(dc.get(), pen.get()));
					::MoveToEx(dc.get(), points[i][0].x, points[i][0].y, 0);
					::LineTo(dc.get(), points[i][1].x, points[i][1].y);
					::SelectObject(dc.get(), oldPen);
				}
			}
		}
	}
} // namespace @0

/**
 * @class ascension::layout::TextLayout
 * @c TextLayout represents a layout of styled line text. Provides support for drawing, cursor
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
 * @see TextLayoutBuffer#lineLayout, TextLayoutBuffer#lineLayoutIfCached
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
TextLayout::TextLayout(Context& context, const ILayoutInformationProvider& layoutInformation,
		length_t line) : lip_(layoutInformation), lineNumber_(line), style_(layoutInformation.presentation().textLineStyle(line)),
		runs_(0), numberOfRuns_(0), lineOffsets_(0), lineFirstRuns_(0), numberOfLines_(0), longestLineWidth_(-1), wrapWidth_(-1) {
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

	const String& s = text();
	if(s.empty()) {	// an empty line
		numberOfRuns_ = 0;
		numberOfLines_ = 1;
		longestLineWidth_ = 0;
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
	int estimatedNumberOfScriptRuns = max(static_cast<int>(s.length()) / 4, 2), numberOfScriptRuns;
	HRESULT(WINAPI* scriptItemizeOpenType)(const WCHAR*, int, int,
		const SCRIPT_CONTROL*, const SCRIPT_STATE*, SCRIPT_ITEM*, OPENTYPE_TAG*, int*) = uspLib->get<0>();
	while(true) {
		scriptRuns.reallocate(estimatedNumberOfScriptRuns);
		scriptTags.reallocate(estimatedNumberOfScriptRuns);
		if(scriptItemizeOpenType != 0)
			hr = (*scriptItemizeOpenType)(s.data(), static_cast<int>(s.length()),
				estimatedNumberOfScriptRuns, &control, &initialState, scriptRuns.get(), scriptTags.get(), &numberOfScriptRuns);
		else
			hr = ::ScriptItemize(s.data(), static_cast<int>(s.length()),
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
	vector<const StyledTextRun> styledRanges;
	TextRun::mergeScriptsAndStyles(context, s.data(),
		scriptRuns.get(), scriptTags.get(), numberOfScriptRuns,
		presentation.textRunStyles(lineNumber()), lip_, textRuns, styledRanges);
	runs_ = new TextRun*[numberOfRuns_ = textRuns.size()];
	copy(textRuns.begin(), textRuns.end(), runs_);
	styledRanges_.reset(new StyledTextRun[numberOfStyledRanges_ = styledRanges.size()]);
	copy(styledRanges.begin(), styledRanges.end(), styledRanges_.get());

	// 3. generate glyphs for each text runs
	for(size_t i = 0; i < numberOfRuns_; ++i)
		runs_[i]->shape(context, s, lip_);
	TextRun::substituteGlyphs(context, Range<TextRun**>(runs_, runs_ + numberOfRuns_), s);

	// 4. position glyphs for each text runs
	for(size_t i = 0; i < numberOfRuns_; ++i)
		runs_[i]->positionGlyphs(context, s, SimpleStyledTextRunIterator(Range<const StyledTextRun*>(
			styledRanges_.get(), styledRanges_.get() + numberOfStyledRanges_), runs_[i]->beginning()));

	// wrap into visual lines and reorder runs in each lines
	if(numberOfRuns_ == 0 || wrapWidth_ == -1) {
		numberOfLines_ = 1;
		lineFirstRuns_ = new size_t[1];
		lineFirstRuns_[0] = 0;
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
TextLayout::~TextLayout() /*throw()*/ {
	dispose();
}

/**
 * Returns the computed text alignment of the line. The returned value may be
 * @c presentation#ALIGN_START or @c presentation#ALIGN_END.
 * @see #readingDirection, presentation#resolveTextAlignment
 */
TextAlignment TextLayout::alignment() const /*throw()*/ {
	if(style_.get() != 0 && style_->readingDirection != INHERIT_TEXT_ALIGNMENT)
		style_->readingDirection;
	tr1::shared_ptr<const TextLineStyle> defaultStyle(lip_.presentation().defaultTextLineStyle());
	return (defaultStyle.get() != 0
		&& defaultStyle->alignment != INHERIT_TEXT_ALIGNMENT) ? defaultStyle->alignment : ASCENSION_DEFAULT_TEXT_ALIGNMENT;
}

/**
 * Returns the bidirectional embedding level at specified position.
 * @param column the column
 * @return the embedding level
 * @throw kernel#BadPositionException @a column is greater than the length of the line
 */
ascension::byte TextLayout::bidiEmbeddingLevel(length_t column) const {
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
 * @param range The character range
 * @return the Win32 GDI region object encompasses the black box bounds. the coordinates are based
 *         on the left-top of the first visual line in the layout
 * @throw kernel#BadPositionException @a range intersects with the outside of the line
 * @see #bounds(void), #bounds(length_t, length_t), #lineBounds, #lineIndent
 */
NativePolygon TextLayout::blackBoxBounds(const Range<length_t>& range) const {
	if(range.end() > text().length())
		throw kernel::BadPositionException(kernel::Position(lineNumber_, range.end()));

	// handle empty line
	if(numberOfRuns_ == 0)
		return win32::Handle<HRGN>(::CreateRectRgn(0, 0, 0, linePitch()), &::DeleteObject);

	const length_t firstLine = lineAt(range.beginning()), lastLine = lineAt(range.end());
	vector<RECT> rectangles;
	RECT rectangle;
	rectangle.top = 0;
	rectangle.bottom = rectangle.top + linePitch();
	for(length_t line = firstLine; line <= lastLine;
			++line, rectangle.top = rectangle.bottom, rectangle.bottom += linePitch()) {
		const size_t endOfRuns = (line + 1 < numberOfLines()) ? lineFirstRuns_[line + 1] : numberOfRuns_;
		int cx = lineIndent(line);
		if(range.beginning() <= lineOffset(line) && range.end() >= lineOffset(line) + lineLength(line)) {
			// whole visual line is encompassed by the range
			rectangle.left = cx;
			rectangle.right = rectangle.left + lineWidth(line);
			rectangles.push_back(rectangle);
			continue;
		}
		for(size_t i = lineFirstRuns_[line]; i < endOfRuns; ++i) {
			const TextRun& run = *runs_[i];
			if(range.beginning() <= run.end() && range.end() >= run.beginning()) {
				rectangle.left = cx + ((range.beginning() > run.beginning()) ?
					run.x(range.beginning(), false) : ((run.readingDirection() == LEFT_TO_RIGHT) ? 0 : run.totalWidth()));
				rectangle.right = cx + ((range.end() < run.end()) ?
					run.x(range.end(), false) : ((run.readingDirection() == LEFT_TO_RIGHT) ? run.totalWidth() : 0));
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
 * @see #blackBoxBounds, #bounds(length_t, length_t), #lineBounds
 */
Dimension<> TextLayout::bounds() const /*throw()*/ {
	return Dimension<>(longestLineWidth(), static_cast<long>(linePitch() * numberOfLines()));
}

/**
 * Returns the smallest rectangle emcompasses all characters in the range. It might not coincide
 * exactly the ascent, descent or overhangs of the specified region of the text.
 * @param range The range
 * @return The rectangle whose @c left value is the indentation of the bounds and @c top value is
 *         the distance from the top of the whole line
 * @throw kernel#BadPositionException @a range intersects with the outside of the line
 * @see #blackBoxBounds, #bounds(void), #lineBounds, #lineIndent
 */
Rect<> TextLayout::bounds(const Range<length_t>& range) const {
	if(range.end() > text().length())
		throw kernel::BadPositionException(kernel::Position(lineNumber_, range.end()));

	// handle empty line
	if(numberOfRuns_ == 0)
		return Rect<>(Point<>(0, 0), Dimension<>(0, linePitch()));

	// determine the top and the bottom (it's so easy)
	Rect<> bounds;	// the result
	const length_t firstLine = lineAt(range.beginning()), lastLine = lineAt(range.end());
	bounds.setY(makeRange(
		static_cast<Scalar>(linePitch() * firstLine), static_cast<Scalar>(linePitch() * (lastLine + 1))));

	// find side bounds between 'firstLine' and 'lastLine'
	int left = numeric_limits<LONG>::max(), right = numeric_limits<LONG>::min();
	for(length_t line = firstLine + 1; line < lastLine; ++line) {
		const Scalar indent = lineIndent(line);
		left = min(indent, left);
		right = max(indent + lineWidth(line), right);
	}

	// find side bounds in 'firstLine' and 'lastLine'
	int cx;
	const length_t firstAndLast[2] = {firstLine, lastLine};
	for(size_t i = 0; i < ASCENSION_COUNTOF(firstAndLast); ++i) {
		const length_t line = firstAndLast[i];
		const size_t endOfRuns = (line + 1 < numberOfLines()) ? lineFirstRuns_[line + 1] : numberOfRuns_;
		// find left bound
		cx = lineIndent(line);
		for(size_t j = lineFirstRuns_[line]; j < endOfRuns; ++j) {
			if(cx >= left)
				break;
			const TextRun& run = *runs_[j];
			if(range.beginning() <= run.end() && range.end() >= run.beginning()) {
				const int x = run.x((
					(run.readingDirection() == LEFT_TO_RIGHT) ? max(range.beginning(), run.beginning()) : min(range.end(), run.end())), false);
				left = min(cx + x, left);
				break;
			}
			cx += run.totalWidth();
		}
		// find right bound
		cx = lineIndent(firstLine) + lineWidth(lastLine);
		for(size_t j = endOfRuns - 1; ; --j) {
			if(cx <= right)
				break;
			const TextRun& run = *runs_[j];
			if(range.beginning() <= run.end() && range.end() >= run.beginning()) {
				const int x = run.x((
					(run.readingDirection() == LEFT_TO_RIGHT) ? min(range.end(), run.end()) : max(range.beginning(), run.beginning())), false);
				right = max(cx - run.totalWidth() + x, right);
				break;
			}
			if(j == lineFirstRuns_[line])
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
inline void TextLayout::dispose() /*throw()*/ {
	for(size_t i = 0; i < numberOfRuns_; ++i)
		delete runs_[i];
	delete[] runs_;
	runs_ = 0;
	numberOfRuns_ = 0;
	delete[] lineOffsets_;
	delete[] lineFirstRuns_;
	lineFirstRuns_ = 0;
	numberOfLines_ = 0;
}

/**
 * Draws the layout to the output device.
 * @param context The rendering context
 * @param origin The position to draw
 * @param paintRect The region to draw
 * @param clipRect The clipping region
 * @param selection Defines the region and the color of the selection
 */
void TextLayout::draw(PaintContext& context, const Point<>& origin,
		const Rect<>& paintRect, const Rect<>& clipRect, const Selection* selection) const /*throw()*/ {
	const int dy = linePitch();

	// empty line
	if(isDisposed()) {
		Color background, dummy;
		lip_.presentation().textLineColors(lineNumber_, dummy, background);
		context.fillRectangle(
			Rect<>(
				Point<>(
					max(paintRect.left(), clipRect.left()),
					max<Scalar>(clipRect.top(), max<Scalar>(paintRect.top(), origin.y))),
				Point<>(
					min(paintRect.right(), clipRect.right()),
					min<Scalar>(clipRect.bottom(), min<Scalar>(paintRect.bottom(), origin.y + dy)))),
			Color::fromCOLORREF(systemColors.serve(background, COLOR_WINDOW)));
		return;
	}

	// skip to the line needs to draw
	length_t line = (origin.y + dy >= paintRect.top()) ? 0 : (paintRect.top() - (origin.y + dy)) / dy;
	if(line >= numberOfLines())
		return;	// this logical line does not need to draw

	Point<> p(origin.x, origin.y + static_cast<Scalar>(dy * line));
	for(; line < numberOfLines(); ++line) {
		draw(line, context, p, paintRect, clipRect, selection);
		if((p.y += dy) >= paintRect.bottom())	// to next line
			break;
	}
}

/**
 * Draws the specified line layout to the output device.
 * @param line The visual line
 * @param context The graphics context
 * @param origin The position to draw
 * @param paintRect The region to draw
 * @param clipRect The clipping region
 * @param selection Defines the region and the color of the selection. Can be @c null
 * @throw IndexOutOfBoundsException @a line is invalid
 */
void TextLayout::draw(length_t line, PaintContext& context,
		const Point<>& origin, const Rect<>& paintRect, const Rect<>& clipRect, const Selection* selection) const {
	if(line >= numberOfLines())
		throw IndexOutOfBoundsException("line");

#ifdef _DEBUG
	if(DIAGNOSE_INHERENT_DRAWING)
		win32::DumpContext() << L"@TextLayout.draw draws line " << lineNumber_ << L" (" << line << L")\n";
#endif // _DEBUG

	// the following topic describes how to draw a selected text using masking by clipping
	// Catch 22 : Design and Implementation of a Win32 Text Editor
	// Part 10 - Transparent Text and Selection Highlighting (http://www.catch22.net/tuts/editor10.asp)

	const int dy = linePitch();
	const int lineHeight = lip_.textMetrics().cellHeight();
	Color lineForeground, lineBackground;
	lip_.presentation().textLineColors(lineNumber_, lineForeground, lineBackground);
	const Color marginColor(Color::fromCOLORREF(systemColors.serve(lineBackground, COLOR_WINDOW)));
	ISpecialCharacterRenderer::DrawingContext dc(context);
	ISpecialCharacterRenderer* specialCharacterRenderer = lip_.specialCharacterRenderer();

	if(specialCharacterRenderer != 0)
		context.boundsToPaint().setY(Range<int>(origin.y, origin.y + lineHeight));

	context.save();
	::SetTextAlign(context.nativeHandle().get(), TA_TOP | TA_LEFT | TA_NOUPDATECP);
	if(isDisposed())	// empty line
		context.fillRectangle(
			Rect<>(
				Point<>(
					max(paintRect.left(), clipRect.left()),
					max<Scalar>(clipRect.top(), max<Scalar>(paintRect.top(), origin.y))),
				Point<>(
					min(paintRect.right(), clipRect.right()),
					min<Scalar>(clipRect.bottom(), min<Scalar>(paintRect.bottom(), origin.y + dy)))
			),
			marginColor);
	else {
//		const String& s = text();
		Range<length_t> selectedRange;
		if(selection != 0) {
			if(!selectedRangeOnVisualLine(selection->caret(), lineNumber_, line, selectedRange))
				selection = 0;
		}

		// 1. paint gap of lines
		// 2. paint the left margin
		// 3. paint background of the text runs
		// 4. paint the right margin
		// 5. draw the foreground glyphs

		// 1. paint gap of lines
		Point<> basePoint(origin);
		win32::Handle<HRGN> clipRegion(::CreateRectRgn(
			clipRect.left(), max<Scalar>(basePoint.y, clipRect.top()),
			clipRect.right(), min<Scalar>(basePoint.y + dy, clipRect.bottom())), &::DeleteObject);
//		dc.selectClipRgn(clipRegion.getHandle());
		if(dy - lineHeight > 0)
			context.fillRectangle(
				Rect<>(
					Point<>(paintRect.left(), basePoint.y + lineHeight),
					Dimension<>(paintRect.right() - paintRect.left(), dy - lineHeight)),
				marginColor);

		basePoint.x += lineIndent(line);

		tr1::shared_ptr<const TextRunStyle> defaultStyle(lip_.presentation().defaultTextRunStyle());
		const COLORREF defaultForeground =
			systemColors.serve((defaultStyle.get() != 0) ? defaultStyle->foreground : Color(), COLOR_WINDOWTEXT);
		const COLORREF defaultBackground =
			systemColors.serve((defaultStyle.get() != 0) ? defaultStyle->background : Color(), COLOR_WINDOW);
		size_t firstRun = lineFirstRuns_[line];
		size_t lastRun = (line < numberOfLines() - 1) ? lineFirstRuns_[line + 1] : numberOfRuns_;

		// 2. paint the left margin
		if(basePoint.x > paintRect.left())
			context.fillRectangle(
				Rect<>(
					Point<>(paintRect.left(), basePoint.y),
					Dimension<>(basePoint.x - paintRect.left(), lineHeight)),
				marginColor);

		// 3. paint background of the text runs
		int startX = basePoint.x;
		for(size_t i = firstRun; i < lastRun; ++i) {
			const TextRun& run = *runs_[i];
			if(basePoint.x + run.totalWidth() < paintRect.left()) {	// this run does not need to draw
				++firstRun;
				startX = basePoint.x + run.totalWidth();
			} else {
				basePoint.y += run.font()->metrics().ascent();
				if(selection != 0 && selectedRange.includes(static_cast<const Range<length_t>&>(run))) {
					Rect<> selectedBounds;
					run.drawBackground(context, basePoint, run, selection->background(), &paintRect, &selectedBounds);
					::ExcludeClipRect(context.nativeHandle().get(),
						selectedBounds.left(), selectedBounds.top(),
						selectedBounds.right(), selectedBounds.bottom());
				} else {
					StyledTextRunEnumerator i(auto_ptr<IStyledTextRunIterator>(
						new SimpleStyledTextRunIterator(Range<const StyledTextRun*>(
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
							::ExcludeClipRect(context.nativeHandle().get(),
								selectedBounds.left(), selectedBounds.top(),
								selectedBounds.right(), selectedBounds.bottom());
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
			if(basePoint.x >= paintRect.right()) {
				lastRun = i + 1;
				break;
			}
		}

		// 4. paint the right margin
		if(basePoint.x < paintRect.right())
			context.fillRectangle(Rect<>(basePoint, Dimension<>(paintRect.right() - basePoint.x, dy)), marginColor);

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
			StyledTextRunEnumerator j(auto_ptr<IStyledTextRunIterator>(
				new SimpleStyledTextRunIterator(Range<const StyledTextRun*>(
					styledRanges_.get(), styledRanges_.get() + numberOfStyledRanges_), run.beginning())), run.end());
			for(; j.hasNext(); j.next())
				run.drawForeground(context, basePoint, j.currentRange(), j.currentStyle()->foreground, &paintRect, 0);
			basePoint.y -= run.font()->metrics().ascent();
			basePoint.x += run.totalWidth();
		}

		// 6. draw the selected foreground glyphs
		if(selection != 0) {
			basePoint.x = startX;
			::ExtSelectClipRgn(context.nativeHandle().get(),
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
			if(lineForeground != Color())
				foreground = lineForeground.asCOLORREF();
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
		if(line == numberOfLines_ - 1
				&& resolveTextAlignment(alignment(), readingDirection()) == ALIGN_RIGHT)
			x = startX;
	} // end of nonempty line case
	
	// line terminator and line wrapping mark
	const Document& document = lip_.presentation().document();
	if(specialCharacterRenderer != 0) {
		context.readingDirection = lineTerminatorOrientation(style(), lip_.presentation().defaultLineStyle());
		if(line < numberOfLines() - 1) {	// line wrapping mark
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
void TextLayout::dumpRuns(ostream& out) const {
	for(size_t i = 0; i < numberOfRuns_; ++i) {
		const TextRun& run = *runs_[i];
		out << static_cast<uint>(i)
			<< ":beginning=" << static_cast<uint>(run.beginning())
			<< ",length=" << static_cast<uint>(run.length()) << endl;
	}
}
#endif // _DEBUG

/// Expands the all tabs and resolves each width.
inline void TextLayout::expandTabsWithoutWrapping() /*throw()*/ {
	const String& s = text();
	const int fullTabWidth = lip_.textMetrics().averageCharacterWidth() * lip_.layoutSettings().tabWidth;
	int x = 0;

	if(lineTerminatorOrientation(style(), lip_.presentation().defaultTextLineStyle()) == LEFT_TO_RIGHT) {	// expand from the left most
		for(size_t i = 0; i < numberOfRuns_; ++i) {
			TextRun& run = *runs_[i];
			run.expandTabCharacters(s, x, fullTabWidth, numeric_limits<int>::max());
			x += run.totalWidth();
		}
	} else {	// expand from the right most
		for(size_t i = numberOfRuns_; i > 0; --i) {
			TextRun& run = *runs_[i - 1];
			run.expandTabCharacters(s, x, fullTabWidth, numeric_limits<int>::max());
			x += run.totalWidth();
		}
	}
	longestLineWidth_ = x;
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
String TextLayout::fillToX(int x) const {
#if 0
	int cx = longestLineWidth();
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
inline size_t TextLayout::findRunForPosition(length_t column) const /*throw()*/ {
	assert(numberOfRuns_ > 0);
	if(column == text().length())
		return numberOfRuns_ - 1;
	const length_t sl = lineAt(column);
	const size_t lastRun = (sl + 1 < numberOfLines()) ? lineFirstRuns_[sl + 1] : numberOfRuns_;
	for(size_t i = lineFirstRuns_[sl]; i < lastRun; ++i) {
		if(runs_[i]->beginning() <= column && runs_[i]->end() > column)	// TODO: replace with includes().
			return i;
	}
	assert(false);
	return lastRun - 1;	// never reachable...
}
#if 0
/// Returns an iterator addresses the first styled segment.
TextLayout::StyledSegmentIterator TextLayout::firstStyledSegment() const /*throw()*/ {
	const TextRun* temp = *runs_;
	return StyledSegmentIterator(temp);
}
#endif
/// Returns if the line contains right-to-left run.
bool TextLayout::isBidirectional() const /*throw()*/ {
	if(readingDirection() == RIGHT_TO_LEFT)
		return true;
	for(size_t i = 0; i < numberOfRuns_; ++i) {
		if(runs_[i]->readingDirection() == RIGHT_TO_LEFT)
			return true;
	}
	return false;
}

/// Justifies the wrapped visual lines.
inline void TextLayout::justify() /*throw()*/ {
	assert(wrapWidth_ != -1);
	for(length_t line = 0; line < numberOfLines(); ++line) {
		const int w = lineWidth(line);
		const size_t last = (line + 1 < numberOfLines()) ? lineFirstRuns_[line + 1] : numberOfRuns_;
		for(size_t i = lineFirstRuns_[line]; i < last; ++i) {
			TextRun& run = *runs_[i];
			const int newRunWidth = ::MulDiv(run.totalWidth(), wrapWidth_, w);	// TODO: there is more precise way.
			run.justify(newRunWidth);
		}
	}
}
#if 0
/// Returns an iterator addresses the last styled segment.
TextLayout::StyledSegmentIterator TextLayout::lastStyledSegment() const /*throw()*/ {
	const TextRun* temp = runs_[numberOfRuns_];
	return StyledSegmentIterator(temp);
}
#endif
/**
 * Returns the smallest rectangle emcompasses the specified visual line. It might not coincide
 * exactly the ascent, descent or overhangs of the specified line.
 * @param line The wrapped line
 * @return The rectangle whose @c left value is the indentation of the line and @c top value is the
 * distance from the top of the whole line
 * @throw IndexOutOfBoundsException @a line is greater than the number of the wrapped lines
 * @see #lineIndent
 */
Rect<> TextLayout::lineBounds(length_t line) const {
	if(line >= numberOfLines())
		throw IndexOutOfBoundsException("line");
	return Rect<>(
		Point<>(lineIndent(line), linePitch() * static_cast<Scalar>(line)),
		Dimension<>(lineWidth(line), linePitch()));
}

/**
 * Returns the indentation of the specified line. An indent is a horizontal distance from the
 * leftmost of the first line to the leftmost of the given line. If the line is longer than the
 * first line, the result is negative. The first line's indent is always zero.
 * @param line The visual line
 * @return The indentation in pixel
 * @throw IndexOutOfBoundsException @a line is invalid
 */
int TextLayout::lineIndent(length_t line) const {
	if(line == 0)
		return 0;
	const TextAlignment resolvedAlignment = resolveTextAlignment(alignment(), readingDirection());
	if(resolvedAlignment == ALIGN_LEFT || resolvedAlignment == JUSTIFY)	// TODO: recognize the last line if justified.
		return 0;
	switch(resolvedAlignment) {
	case ALIGN_LEFT:
	default:
		return 0;
	case ALIGN_RIGHT:
		return lineWidth(0) - lineWidth(line);
	case ALIGN_CENTER:
		return (lineWidth(0) - lineWidth(line)) / 2;
	}
}
/// Returns the line pitch in pixels.
inline int TextLayout::linePitch() const /*throw()*/ {
	return lip_.textMetrics().cellHeight() + max(lip_.layoutSettings().lineSpacing, lip_.textMetrics().lineGap());
}

/**
 * Returns the width of the specified wrapped line.
 * @param line The visual line
 * @return The width
 * @throw IndexOutOfBoundsException @a line is greater than the number of visual lines
 */
int TextLayout::lineWidth(length_t line) const {
	if(line >= numberOfLines())
		throw IndexOutOfBoundsException("line");
	else if(isDisposed())
		return 0;
	else if(numberOfLines() == 1 && longestLineWidth_ != -1)
		return longestLineWidth_;
	else {
		const size_t lastRun = (line + 1 < numberOfLines()) ? lineFirstRuns_[line + 1] : numberOfRuns_;
		int cx = 0;
		for(size_t i = lineFirstRuns_[line]; i < lastRun; ++i)
			cx += runs_[i]->totalWidth();
		return cx;
	}
}

// implements public location methods
void TextLayout::locations(length_t column, Point<>* leading, Point<>* trailing) const {
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
	const length_t sl = lineAt(column);
	const length_t firstRun = lineFirstRuns_[sl];
	const length_t lastRun = (sl + 1 < numberOfLines()) ? lineFirstRuns_[sl + 1] : numberOfRuns_;
	// about x
	if(readingDirection() == LEFT_TO_RIGHT) {	// LTR
		int x = lineIndent(sl);
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
		int x = lineIndent(sl) + lineWidth(sl);
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

/// Returns the width of the longest line.
int TextLayout::longestLineWidth() const /*throw()*/ {
	if(longestLineWidth_ == -1) {
		int width = 0;
		for(length_t line = 0; line < numberOfLines(); ++line)
			width = max<long>(lineWidth(line), width);
		const_cast<TextLayout*>(this)->longestLineWidth_ = width;
	}
	return longestLineWidth_;
}

/// Reorders the runs in visual order.
inline void TextLayout::reorder() /*throw()*/ {
	if(numberOfRuns_ == 0)
		return;
	TextRun** temp = new TextRun*[numberOfRuns_];
	memcpy(temp, runs_, sizeof(TextRun*) * numberOfRuns_);
	for(length_t line = 0; line < numberOfLines(); ++line) {
		const size_t numberOfRunsInLine = ((line < numberOfLines() - 1) ?
			lineFirstRuns_[line + 1] : numberOfRuns_) - lineFirstRuns_[line];
		BYTE* const levels = new BYTE[numberOfRunsInLine];
		for(size_t i = 0; i < numberOfRunsInLine; ++i)
			levels[i] = static_cast<BYTE>(runs_[i + lineFirstRuns_[line]]->bidiEmbeddingLevel() & 0x1f);
		int* const log2vis = new int[numberOfRunsInLine];
		const HRESULT hr = ::ScriptLayout(static_cast<int>(numberOfRunsInLine), levels, 0, log2vis);
		assert(SUCCEEDED(hr));
		delete[] levels;
		for(size_t i = lineFirstRuns_[line]; i < lineFirstRuns_[line] + numberOfRunsInLine; ++i)
			runs_[lineFirstRuns_[line] + log2vis[i - lineFirstRuns_[line]]] = temp[i];
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
inline int TextLayout::nextTabStop(int x, Direction direction) const /*throw()*/ {
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
int TextLayout::nextTabStopBasedLeftEdge(int x, bool right) const /*throw()*/ {
	assert(x >= 0);
	const LayoutSettings& c = lip_.layoutSettings();
	const int tabWidth = lip_.textMetrics().averageCharacterWidth() * c.tabWidth;
	if(lineTerminatorOrientation(style(), lip_.presentation().defaultTextLineStyle()) == LEFT_TO_RIGHT)
		return nextTabStop(x, right ? Direction::FORWARD : Direction::BACKWARD);
	else
		return right ? x + (x - longestLineWidth()) % tabWidth : x - (tabWidth - (x - longestLineWidth()) % tabWidth);
}

/**
 * Returns the hit test information corresponding to the specified point.
 * @param x The x offset from the left edge of the first line
 * @param y The y offset from the top edge of the first line
 * @param[out] outside @c true if the specified point is outside of the layout. Optional
 * @return A pair of the character offsets. The first element addresses the character whose black
 *         box (bounding box) encompasses the specified point. The second element addresses the
 *         character whose leading point is the closest to the specified point in the line
 * @see #location
 */
pair<length_t, length_t> TextLayout::offset(const Point<>& p, bool* outside /* = 0 */) const /*throw()*/ {
	if(text().empty())
		return make_pair(0, 0);

	// determine the line number
	length_t line = 0;
	for(; line < numberOfLines() - 1; ++line) {
		if(static_cast<Scalar>(linePitch() * line) >= p.y)
			break;
	}

	pair<length_t, length_t> result;

	// determine the column
	assert(numberOfRuns_ > 0);
	const size_t lastRun = (line + 1 < numberOfLines()) ? lineFirstRuns_[line + 1] : numberOfRuns_;
	int cx = lineIndent(line);
	if(p.x <= cx) {	// on the left margin
		if(outside != 0)
			*outside = true;
		const TextRun& firstRun = *runs_[lineFirstRuns_[line]];
		result.first = result.second = firstRun.beginning()
			+ ((firstRun.readingDirection() == LEFT_TO_RIGHT) ? 0 : firstRun.length());	// TODO: used SCRIPT_ANALYSIS.fRTL past...
		return result;
	}
	for(size_t i = lineFirstRuns_[line]; i < lastRun; ++i) {
		const TextRun& run = *runs_[i];
		if(p.x >= cx && p.x <= cx + run.totalWidth()) {
			int cp, trailing;
			run.hitTest(p.x - cx, cp, trailing);	// TODO: check the returned value.
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
ReadingDirection TextLayout::readingDirection() const /*throw()*/ {
	ReadingDirection result = INHERIT_READING_DIRECTION;
	// try the requested line style
	if(style_.get() != 0)
		result = style_->readingDirection;
	// try the default line style
	if(result == INHERIT_READING_DIRECTION) {
		tr1::shared_ptr<const TextLineStyle> defaultLineStyle(lip_.presentation().defaultTextLineStyle());
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
StyledRun TextLayout::styledTextRun(length_t column) const {
	if(column > text().length())
		throw kernel::BadPositionException(kernel::Position(lineNumber_, column));
	const TextRun& run = *runs_[findRunForPosition(column)];
	return StyledRun(run.column(), run.requestedStyle());
}
#endif

/// Returns the text of the line.
inline const String& TextLayout::text() const /*throw()*/ {
	return lip_.presentation().document().line(lineNumber_);
}

/// Locates the wrap points and resolves tab expansions.
void TextLayout::wrap(Context& context) /*throw()*/ {
	assert(numberOfRuns_ != 0 && lip_.layoutSettings().lineWrap.wraps());
	assert(numberOfLines_ == 0 && lineOffsets_ == 0 && lineFirstRuns_ == 0);

	const String& s = text();
	vector<length_t> lineFirstRuns;
	lineFirstRuns.push_back(0);
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
		if(run->expandTabCharacters(s, (x1 < wrapWidth_) ? x1 : 0, fullTabWidth, wrapWidth_ - (x1 < wrapWidth_) ? x1 : 0)) {
			if(x1 < wrapWidth_) {
				x1 += run->totalWidth();
				newRuns.push_back(run);
			} else {
				x1 = run->totalWidth();
				newRuns.push_back(run);
				lineFirstRuns.push_back(newRuns.size());
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
		hr = run->logicalAttributes(s, logicalAttributes.get());
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
					if(lineFirstRuns.empty() || lineFirstRuns.back() == newRuns.size()) {
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
					assert(lineFirstRuns.empty() || newRuns.size() != lineFirstRuns.back());
					lineFirstRuns.push_back(newRuns.size());
//dout << L"broke the line at " << lastBreakable << L" where the run start.\n";
				}
				// case 2: break at the end of the run
				else if(lastBreakable == run->end()) {
					if(lastBreakable < s.length()) {
						assert(lineFirstRuns.empty() || newRuns.size() != lineFirstRuns.back());
						lineFirstRuns.push_back(newRuns.size() + 1);
//dout << L"broke the line at " << lastBreakable << L" where the run end.\n";
					}
					break;
				}
				// case 3: break at the middle of the run -> split the run (run -> newRun + run)
				else {
					auto_ptr<TextRun> followingRun(run->breakAt(context, lastBreakable, s, lip_));
					newRuns.push_back(run);
					assert(lineFirstRuns.empty() || newRuns.size() != lineFirstRuns.back());
					lineFirstRuns.push_back(newRuns.size());
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
	lineFirstRuns_ = new length_t[numberOfLines_ = lineFirstRuns.size()];
	copy(lineFirstRuns.begin(), lineFirstRuns.end(), lineFirstRuns_);
	lineOffsets_ = new length_t[numberOfLines()];
	for(size_t i = 0; i < numberOfLines(); ++i)
		lineOffsets_[i] = runs_[lineFirstRuns_[i]]->beginning();
}


// TextLayout.Selection /////////////////////////////////////////////////////

/**
 * Constructor.
 * @param caret The caret holds a selection
 * @param foreground The foreground color
 * @param background The background color
 * @throw std#invalid_argument @a foreground and/or @c background is invalid
 */
TextLayout::Selection::Selection(const viewers::Caret& caret,
		const Color& foreground, const Color& background) /*throw()*/ : caret_(caret), foreground_(background_) {
	if(foreground == Color())
		throw invalid_argument("foreground");
	else if(background == Color())
		throw invalid_argument("background");
}

#if 0
// TextLayout.StyledSegmentIterator /////////////////////////////////////////

/**
 * Private constructor.
 * @param start
 */
TextLayout::StyledSegmentIterator::StyledSegmentIterator(const TextRun*& start) /*throw()*/ : p_(&start) {
}

/// Copy-constructor.
TextLayout::StyledSegmentIterator::StyledSegmentIterator(const StyledSegmentIterator& rhs) /*throw()*/ : p_(rhs.p_) {
}

/// Returns the current segment.
StyledRun TextLayout::StyledSegmentIterator::current() const /*throw()*/ {
	const TextRun& run = **p_;
	return StyledRun(run.column, run.style);
}
#endif
