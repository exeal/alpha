/**
 * @file layout.cpp
 * @author exeal
 * @date 2003-2006 (was TextLayout.cpp)
 * @date 2006-2010
 * @date 2010-11-20 renamed from ascension/layout.cpp
 */

#include <ascension/config.hpp>			// ASCENSION_DEFAULT_LINE_LAYOUT_CACHE_SIZE, ...
#include <ascension/graphics/text-layout.hpp>
#include <ascension/graphics/graphics-windows.hpp>
//#include <ascension/graphics/special-character-renderer.hpp>
#include <ascension/corelib/shared-library.hpp>
#include <ascension/corelib/unicode-property.hpp>
#include <ascension/viewer/caret.hpp>	// Caret.isSelectionRectangle, viewers.selectedRangeOnVisualLine
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
	auto_ptr<detail::SharedLibrary<Uniscribe16> > uspLib(
		new detail::SharedLibrary<Uniscribe16>("usp10.dll"));
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

	inline int estimateNumberOfGlyphs(length_t length) {
		return static_cast<int>(length) * 3 / 2 + 16;
	}

	LANGID userCJKLanguage() /*throw()*/;

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

	inline bool isC0orC1Control(CodePoint c) /*throw()*/ {
		return c < 0x20 || c == 0x7f || (c >= 0x80 && c < 0xa0);
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

	inline Scalar readingDirectionInt(ReadingDirection direction) {
		switch(direction) {
			case LEFT_TO_RIGHT:
				return +1;
			case RIGHT_TO_LEFT:
				return -1;
			default:
				throw UnknownValueException("direction");
		}
	}

	HRESULT resolveNumberSubstitution(
			const NumberSubstitution* configuration, SCRIPT_CONTROL& sc, SCRIPT_STATE& ss) {
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

	template<typename T> inline T& shrinkToFit(T& v) {
		swap(v, T(v));
		return v;
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


// layout.* free functions ////////////////////////////////////////////////////////////////////////

/// Returns @c true if complex scripts are supported.
bool font::supportsComplexScripts() /*throw()*/ {
	return true;
}

/// Returns @c true if OpenType features are supported.
bool font::supportsOpenTypeFeatures() /*throw()*/ {
	return uspLib->get<0>() != 0;
}


// FixedWidthTabExpander //////////////////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param width The fixed width in pixels
 */
FixedWidthTabExpander::FixedWidthTabExpander(Scalar width) /*throw()*/ : width_(width) {
}

/// @see TabExpander#nextTabStop
Scalar FixedWidthTabExpander::nextTabStop(Scalar x, length_t) const /*throw()*/ {
	return x - x % width_ + width_;
}


// TextLayout.LineArea ////////////////////////////////////////////////////////////////////////////

class TextLayout::LineArea {
public:
	// block-area
	Scalar endIndent() const;
	Scalar lineHeight() const;
	LineStackingStrategy lineStackingStrategy() const;
	Scalar spaceAfter() const;
	Scalar spaceBefore() const;
	Scalar startIndent() const;
	// line-area
	Rect<> maximumLineRectangle() const;
	Rect<> nominalRequestedLineRectangle() const;
	Rect<> perInlineHeightRectangle() const;
};


// TextLayout.InlineArea //////////////////////////////////////////////////////////////////////////

class TextLayout::InlineArea : public StyledTextRun {
public:
	Rect<> allocationRectangle() const;
	Rect<> borderRectangle() const;
	Rect<> contentRectangle() const;
	Rect<> largeAllocationRectangle() const;
};


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
	class SimpleStyledTextRunIterator : public StyledTextRunIterator {
	public:
		SimpleStyledTextRunIterator(const vector<const StyledTextRun>& styledRanges, length_t start) : styledRanges_(styledRanges) {
			current_ = detail::searchBound(styledRanges_.begin(), styledRanges_.end(), start, BeginningOfStyledTextRun());
		}
		// presentation.StyledTextRunIterator
		StyledTextRun current() const {
			if(!hasNext())
				throw IllegalStateException("");
			return *current_;
		}
		bool hasNext() const {
			return current_ != styledRanges_.end();
		}
		void next() {
			if(!hasNext())
				throw IllegalStateException("");
			++current_;
		}
	private:
		struct BeginningOfStyledTextRun {
			bool operator()(length_t v, const StyledTextRun& range) const {
				return v < range.position();
			}
		};
		const vector<const StyledTextRun>& styledRanges_;
		vector<const StyledTextRun>::const_iterator current_;
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
	// attributes
	uchar bidiEmbeddingLevel() const /*throw()*/ {return static_cast<uchar>(analysis_.s.uBidiLevel);}
	tr1::shared_ptr<const Font> font() const {return glyphs_->font;}
	HRESULT logicalAttributes(const String& layoutString, SCRIPT_LOGATTR attributes[]) const;
	int numberOfGlyphs() const /*throw()*/ {return glyphRange_.length();}
	ReadingDirection readingDirection() const /*throw()*/ {
		return ((analysis_.s.uBidiLevel & 0x01) == 0x00) ? LEFT_TO_RIGHT : RIGHT_TO_LEFT;}
	// geometry
	void blackBoxBounds(const Range<length_t>& range, Rect<>& bounds) const;
	HRESULT logicalWidths(int widths[]) const;
	HRESULT hitTest(int x, int& cp, int& trailing) const;
	int totalWidth() const /*throw()*/ {return accumulate(advances(), advances() + numberOfGlyphs(), 0);}
	int x(length_t at, bool trailing) const;
	// layout
	auto_ptr<TextRun> breakAt(length_t at, const String& layoutString);
	bool expandTabCharacters(const TabExpander& tabExpander,
		const String& layoutString, Scalar x, Scalar maximumWidth);
	HRESULT justify(int width);
	static void mergeScriptsAndStyles(const String& layoutString, const SCRIPT_ITEM scriptRuns[],
		const OPENTYPE_TAG scriptTags[], size_t numberOfScriptRuns, const FontCollection& fontCollection,
		tr1::shared_ptr<const TextRunStyle> defaultStyle, auto_ptr<StyledTextRunIterator> styles,
		vector<TextRun*>& textRuns, vector<const StyledTextRun>& styledRanges);
	void shape(const win32::Handle<HDC>& dc, const String& layoutString);
	void positionGlyphs(const win32::Handle<HDC>& dc, const String& layoutString, SimpleStyledTextRunIterator& styles);
	auto_ptr<TextRun> splitIfTooLong(const String& layoutString);
	static void substituteGlyphs(const Range<TextRun**>& runs, const String& layoutString);
	// drawing and painting
	void drawGlyphs(PaintContext& context, const Point<>& p, const Range<length_t>& range) const;
	void paintBackground(PaintContext& context, const Point<>& p,
		const Range<length_t>& range, Rect<>* paintedBounds) const;
	void paintBorder() const;
	void paintLineDecorations() const;
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
		void vanish(const Font& font, size_t at);	// 'at' is distance from the beginning of this run
	};
private:
	TextRun(TextRun& leading, length_t characterBoundary) /*throw()*/;
	const int* advances() const /*throw()*/ {
		if(const int* const p = glyphs_->advances.get()) return p + glyphRange_.beginning(); return 0;}
	const WORD* clusters() const /*throw()*/ {
		if(const WORD* const p = glyphs_->clusters.get())
			return p + (beginning() - glyphs_->characters.beginning()); return 0;}
	pair<int, HRESULT> countMissingGlyphs(const Context& context, const Char* text) const /*throw()*/;
	static HRESULT generateGlyphs(const win32::Handle<HDC>& dc,
		const StringPiece& text, const SCRIPT_ANALYSIS& analysis, Glyphs& glyphs, int& numberOfGlyphs);
	static void generateDefaultGlyphs(const win32::Handle<HDC>& dc,
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

void TextLayout::TextRun::Glyphs::vanish(const Font& font, size_t at) {
	assert(advances.get() == 0);
	win32::Handle<HDC> dc(detail::screenDC());
	HFONT oldFont = 0;
	WORD blankGlyph;
	HRESULT hr = ::ScriptGetCMap(dc.get(), &fontCache, L"\x0020", 1, 0, &blankGlyph);
	if(hr == E_PENDING) {
		oldFont = static_cast<HFONT>(::SelectObject(dc.get(), font.nativeHandle().get()));
		hr = ::ScriptGetCMap(dc.get(), &fontCache, L"\x0020", 1, 0, &blankGlyph);
	}
	if(hr == S_OK) {
		SCRIPT_FONTPROPERTIES fp;
		fp.cBytes = sizeof(SCRIPT_FONTPROPERTIES);
		if(FAILED(hr = ::ScriptGetFontProperties(dc.get(), &fontCache, &fp)))
			fp.wgBlank = 0;	/* hmm... */
		blankGlyph = fp.wgBlank;
	}
	if(oldFont != 0)
		::SelectObject(dc.get(), oldFont);
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

auto_ptr<TextLayout::TextRun> TextLayout::TextRun::breakAt(length_t at, const String& layoutString) {
	// 'at' is from the beginning of the line
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
 * Draws the glyphs of the specified character range in this run.
 * This method uses the stroke and fill styles which are set in @a context.
 * @param context The graphics context
 * @param p The base point of this run (, does not corresponds to @c range.beginning())
 * @param range The character range to paint. If the edges addressed outside of this run, they are
 *              truncated
 */
void TextLayout::TextRun::drawGlyphs(PaintContext& context, const Point<>& p, const Range<length_t>& range) const {
	const Range<length_t> truncatedRange(max(range.beginning(), beginning()), min(range.end(), end()));
	if(truncatedRange.isEmpty())
		return;
	const Range<size_t> glyphRange(
		characterPositionToGlyphPosition(clusters(), length(), numberOfGlyphs(), truncatedRange.beginning() - beginning(), analysis_),
		characterPositionToGlyphPosition(clusters(), length(), numberOfGlyphs(), truncatedRange.end() - beginning(), analysis_));
	if(!glyphRange.isEmpty()) {
		context.setFont(glyphs_->font);
//		RECT temp;
//		if(dirtyRect != 0)
//			::SetRect(&temp, dirtyRect->left(), dirtyRect->top(), dirtyRect->right(), dirtyRect->bottom());
		const HRESULT hr = ::ScriptTextOut(context.nativeHandle().get(), &glyphs_->fontCache,
			p.x + x((analysis_.fRTL == 0) ? truncatedRange.beginning() : (truncatedRange.end() - 1), analysis_.fRTL != 0),
			p.y - glyphs_->font->metrics().ascent(), 0, &toNative(context.boundsToPaint()), &analysis_, 0, 0,
			glyphs() + glyphRange.beginning(), glyphRange.length(), advances() + glyphRange.beginning(),
			(justifiedAdvances() != 0) ? justifiedAdvances() + glyphRange.beginning() : 0,
			glyphOffsets() + glyphRange.beginning());
	}
}

/**
 * Expands tab characters in this run and modifies the width.
 * @param tabExpander The tab expander
 * @param layoutString the whole string of the layout this run belongs to
 * @param x the position in writing direction this run begins, in pixels
 * @param maximumWidth the maximum width this run can take place, in pixels
 * @return @c true if expanded tab characters
 * @throw std#invalid_argument @a maximumWidth &lt;= 0
 */
inline bool TextLayout::TextRun::expandTabCharacters(
		const TabExpander& tabExpander, const String& layoutString, Scalar x, Scalar maximumWidth) {
	if(maximumWidth <= 0)
		throw invalid_argument("maximumWidth");
	if(layoutString[beginning()] != L'\t')
		return false;
	assert(length() == 1 && glyphs_.unique());
	glyphs_->advances[0] = min(tabExpander.nextTabStop(x, beginning()), maximumWidth);
	glyphs_->justifiedAdvances.reset();
	return true;
}

/// Fills the glyph array with default index, instead of using @c ScriptShape.
inline void TextLayout::TextRun::generateDefaultGlyphs(const win32::Handle<HDC>& dc,
		const StringPiece& text, const SCRIPT_ANALYSIS& analysis, Glyphs& glyphs) {
	SCRIPT_CACHE fontCache(0);
	SCRIPT_FONTPROPERTIES fp;
	fp.cBytes = sizeof(SCRIPT_FONTPROPERTIES);
	if(FAILED(::ScriptGetFontProperties(dc.get(), &fontCache, &fp)))
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
	using std::swap;
	swap(glyphs.fontCache, fontCache);
	swap(glyphs.indices, indices);
	swap(glyphs.clusters, clusters);
	swap(glyphs.visualAttributes, visualAttributes);
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
HRESULT TextLayout::TextRun::generateGlyphs(const win32::Handle<HDC>& dc,
		const StringPiece& text, const SCRIPT_ANALYSIS& analysis, Glyphs& glyphs, int& numberOfGlyphs) {
#ifdef _DEBUG
	if(HFONT currentFont = static_cast<HFONT>(::GetCurrentObject(dc.get(), OBJ_FONT))) {
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
		hr = ::ScriptShape(dc.get(), &fontCache,
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
		using std::swap;
		swap(glyphs.fontCache, fontCache);
		swap(glyphs.indices, indices);
		swap(glyphs.clusters, clusters);
		swap(glyphs.visualAttributes, visualAttributes);
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
	void resolveFontSpecifications(const FontCollection& fontCollection,
			tr1::shared_ptr<const TextRunStyle> requestedStyle,
			tr1::shared_ptr<const TextRunStyle> defaultStyle, String* computedFamilyName,
			FontProperties* computedProperties, double* computedSizeAdjust) {
		// family name
		if(computedFamilyName != 0) {
			*computedFamilyName = (requestedStyle.get() != 0) ? requestedStyle->fontFamily : String();
			if(computedFamilyName->empty()) {
				if(defaultStyle.get() != 0)
					*computedFamilyName = defaultStyle->fontFamily;
				if(computedFamilyName->empty())
					*computedFamilyName = fontCollection.lastResortFallback(FontProperties())->familyName();
			}
		}
		// properties
		if(computedProperties != 0) {
			*computedProperties = (requestedStyle.get() != 0) ? requestedStyle->fontProperties : FontProperties();
			double computedSize = computedProperties->size();
			if(computedSize == 0.0f) {
				if(defaultStyle.get() != 0)
					computedSize = defaultStyle->fontProperties.size();
				if(computedSize == 0.0f)
					computedSize = fontCollection.lastResortFallback(FontProperties())->metrics().emHeight();
			}
			*computedProperties = FontProperties(
				(computedProperties->weight() != FontProperties::INHERIT_WEIGHT) ? computedProperties->weight()
					: ((defaultStyle.get() != 0) ? defaultStyle->fontProperties.weight() : FontProperties::NORMAL_WEIGHT),
				(computedProperties->stretch() != FontProperties::INHERIT_STRETCH) ? computedProperties->stretch()
					: ((defaultStyle.get() != 0) ? defaultStyle->fontProperties.stretch() : FontProperties::NORMAL_STRETCH),
				(computedProperties->style() != FontProperties::INHERIT_STYLE) ? computedProperties->style()
					: ((defaultStyle.get() != 0) ? defaultStyle->fontProperties.style() : FontProperties::NORMAL_STYLE),
				(computedProperties->orientation() != FontProperties::INHERIT_ORIENTATION) ? computedProperties->orientation()
					: ((defaultStyle.get() != 0) ? defaultStyle->fontProperties.orientation() : FontProperties::HORIZONTAL),
				computedSize);
		}
		// size-adjust
		if(computedSizeAdjust != 0) {
			*computedSizeAdjust = (requestedStyle.get() != 0) ? requestedStyle->fontSizeAdjust : -1.0;
			if(*computedSizeAdjust < 0.0)
				*computedSizeAdjust = (defaultStyle.get() != 0) ? defaultStyle->fontSizeAdjust : 0.0;
		}
	}
	pair<const Char*, tr1::shared_ptr<const Font> > findNextFontRun(const Range<const Char*>& text,
			const FontCollection& fontCollection, tr1::shared_ptr<const TextRunStyle> requestedStyle,
			tr1::shared_ptr<const TextRunStyle> defaultStyle, tr1::shared_ptr<const Font> previousFont) {
		String familyName;
		FontProperties properties;
		double sizeAdjust;
		resolveFontSpecifications(fontCollection, requestedStyle, defaultStyle, &familyName, &properties, &sizeAdjust);
#if 1
		familyName.assign(L"Times New Roman");
//		properties.style = FontProperties::ITALIC;
#endif // 1
		return make_pair(static_cast<const Char*>(0), fontCollection.get(familyName, properties, sizeAdjust));
	}
} // namespace @0

/**
 * Merges the given item runs and the given style runs.
 * @param layoutString
 * @param items The items itemized by @c #itemize()
 * @param numberOfItems The length of the array @a items
 * @param styles The iterator returns the styled runs in the line. Can be @c null
 * @param[out] textRuns
 * @param[out] styledRanges
 * @see presentation#Presentation#getLineStyle
 */
void TextLayout::TextRun::mergeScriptsAndStyles(
		const String& layoutString, const SCRIPT_ITEM scriptRuns[],
		const OPENTYPE_TAG scriptTags[], size_t numberOfScriptRuns,
		const FontCollection& fontCollection, tr1::shared_ptr<const TextRunStyle> defaultStyle,
		auto_ptr<StyledTextRunIterator> styles, vector<TextRun*>& textRuns,
		vector<const StyledTextRun>& styledRanges) {
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
		styleRun.first = styles->current();
		styles->next();
		results.second.push_back(styleRun.first);
	}
	pair<StyledTextRun, bool> nextStyleRun;	// 'second' is false if 'first' is invalid
	if(nextStyleRun.second = styles.get() != 0 && styles->hasNext())
		nextStyleRun.first = styles->current();
	length_t beginningOfNextStyleRun = nextStyleRun.second ? nextStyleRun.first.position() : layoutString.length();
	tr1::shared_ptr<const Font> font;	// font for current glyph run
	do {
		const length_t previousRunEnd = max<length_t>(
			scriptRun->iCharPos, styleRun.second ? styleRun.first.position() : 0);
		assert(
			(previousRunEnd == 0 && results.first.empty() && results.second.empty())
			|| (!results.first.empty() && previousRunEnd == results.first.back()->end())
			|| (!results.second.empty() && previousRunEnd == results.second.back().position()));
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
					fontCollection, styleRun.second ? styleRun.first.style() : tr1::shared_ptr<const TextRunStyle>(),
					defaultStyle, font));
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
					nextStyleRun.first = styles->current();
				beginningOfNextStyleRun = nextStyleRun.second ? nextStyleRun.first.position() : layoutString.length();
			}
		}
	} while(scriptRun != 0 || styleRun.second);

	// commit
	using std::swap;
	swap(textRuns, results.first);
	styledRanges = results.second;	// will be shrunk to fit

#undef ASCENSION_SPLIT_LAST_RUN
}

/**
 * Paints the background of the specified character range in this run.
 * This method uses the fill style which is set in @a context.
 * @param context The graphics context
 * @param p The base point of this run (, does not corresponds to @c range.beginning())
 * @param range The character range to paint. If the edges addressed outside of this run, they are
 *              truncated
 * @param[out] paintedBounds The rectangle this method painted. Can be @c null
 */
void TextLayout::TextRun::paintBackground(PaintContext& context,
		const Point<>& p, const Range<length_t>& range, Rect<>* paintedBounds) const {
	if(range.isEmpty() || p.x + totalWidth() < context.boundsToPaint().left())
		return;
	Rect<> r;
	blackBoxBounds(range, r);
	context.fillRectangle(r.translate(p));
	if(paintedBounds != 0)
		*paintedBounds = r;
}

/**
 * 
 * @see #merge, #substituteGlyphs
 */
void TextLayout::TextRun::positionGlyphs(const win32::Handle<HDC>& dc, const String& layoutString, SimpleStyledTextRunIterator& styles) {
	assert(glyphs_.get() != 0 && glyphs_.unique());
	assert(glyphs_->indices.get() != 0 && glyphs_->advances.get() == 0);

	AutoBuffer<int> advances(new int[numberOfGlyphs()]);
	AutoBuffer<GOFFSET> offsets(new GOFFSET[numberOfGlyphs()]);
//	ABC width;
	HRESULT hr = ::ScriptPlace(0, &glyphs_->fontCache, glyphs_->indices.get(), numberOfGlyphs(),
		glyphs_->visualAttributes.get(), &analysis_, advances.get(), offsets.get(), 0/*&width*/);
	if(hr == E_PENDING) {
		HFONT oldFont = static_cast<HFONT>(::SelectObject(dc.get(), glyphs_->font->nativeHandle().get()));
		hr = ::ScriptPlace(dc.get(), &glyphs_->fontCache, glyphs_->indices.get(), numberOfGlyphs(),
			glyphs_->visualAttributes.get(), &analysis_, advances.get(), offsets.get(), 0/*&width*/);
		::SelectObject(dc.get(), oldFont);
	}
	if(FAILED(hr))
		throw hr;

	// apply text run styles
	for(; styles.hasNext(); styles.next()) {
		StyledTextRun styledRange(styles.current());
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

void TextLayout::TextRun::shape(const win32::Handle<HDC>& dc, const String& layoutString) {
	assert(glyphs_.unique());

	// TODO: check if the requested style (or the default one) disables shaping.

	HFONT oldFont = static_cast<HFONT>(::SelectObject(dc.get(), glyphs_->font->nativeHandle().get()));
	const StringPiece text(layoutString.data() + beginning(), layoutString.data() + end());
	int numberOfGlyphs;
	HRESULT hr = generateGlyphs(dc, text, analysis_, *glyphs_, numberOfGlyphs);
	if(hr == USP_E_SCRIPT_NOT_IN_FONT) {
		analysis_.eScript = SCRIPT_UNDEFINED;
		hr = generateGlyphs(dc, text, analysis_, *glyphs_, numberOfGlyphs);
	}
	if(FAILED(hr))
		generateDefaultGlyphs(dc, text, analysis_, *glyphs_);
	::SelectObject(dc.get(), oldFont);

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
 * @param runs the minimal runs
 * @param layoutString the whole string of the layout
 * @see #merge, #positionGlyphs
 */
void TextLayout::TextRun::substituteGlyphs(const Range<TextRun**>& runs, const String& layoutString) {
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
							run.glyphs_->vanish(*run.glyphs_->font, i.tell() - layoutString.data() - run.beginning());
							run.glyphs_->vanish(*run.glyphs_->font, i.tell() - layoutString.data() - run.beginning() + 1);
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
						next.glyphs_->vanish(*run.glyphs_->font, 0);
						next.glyphs_->vanish(*run.glyphs_->font, 1);
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


// InlineProgressionDimensionRangeIterator file-local class ///////////////////////////////////////

namespace {
	class InlineProgressionDimensionRangeIterator :
		public detail::IteratorAdapter<
			InlineProgressionDimensionRangeIterator, iterator<
				input_iterator_tag, Range<Scalar>, ptrdiff_t, Range<Scalar>*, Range<Scalar>
			>
		> {
	public:
		InlineProgressionDimensionRangeIterator() /*throw()*/ : currentRun_(0), lastRun_(0) {}
		InlineProgressionDimensionRangeIterator(
			const Range<const TextLayout::TextRun* const*>& textRuns, const Range<length_t>& characterRange,
			ReadingDirection scanningDirection, Scalar initialIpd);
		const Range<length_t> characterRange() const /*throw()*/ {return characterRange_;}
		Range<Scalar> current() const;
		bool equals(const InlineProgressionDimensionRangeIterator& other) const /*throw()*/ {
			if(currentRun_ == 0)
				return other.isDone();
			else if(other.currentRun_ == 0)
				return isDone();
			return currentRun_ == other.currentRun_;
		}
		void next() {return doNext(false);}
		ReadingDirection scanningDirection() const /*throw()*/ {
			return (currentRun_ <= lastRun_) ? LEFT_TO_RIGHT : RIGHT_TO_LEFT;
		}
	private:
		void doNext(bool initializing);
		bool isDone() const /*throw()*/ {return currentRun_ == lastRun_;}
	private:
		/*const*/ Range<length_t> characterRange_;
		const TextLayout::TextRun* const* currentRun_;
		const TextLayout::TextRun* const* /*const*/ lastRun_;
		Scalar ipd_;
	};
}

InlineProgressionDimensionRangeIterator::InlineProgressionDimensionRangeIterator(
		const Range<const TextLayout::TextRun* const*>& textRuns, const Range<length_t>& characterRange,
		ReadingDirection direction, Scalar initialIpd) : characterRange_(characterRange),
		currentRun_((direction == LEFT_TO_RIGHT) ? textRuns.beginning() - 1 : textRuns.end()),
		lastRun_((direction == LEFT_TO_RIGHT) ? textRuns.end() : textRuns.beginning() - 1),
		ipd_(initialIpd) {
	doNext(true);
}

Range<Scalar> InlineProgressionDimensionRangeIterator::current() const {
	if(isDone())
		throw NoSuchElementException();
	assert((*currentRun_)->intersects(characterRange()));
	Scalar start, end;
	if(characterRange().beginning() > (*currentRun_)->beginning())
		start = (*currentRun_)->x(characterRange().beginning(), false);
	else
		start = ((*currentRun_)->readingDirection() == LEFT_TO_RIGHT) ? 0 : (*currentRun_)->totalWidth();
	if(characterRange().end() < (*currentRun_)->end())
		end = (*currentRun_)->x(characterRange().end(), false);
	else
		end = ((*currentRun_)->readingDirection() == LEFT_TO_RIGHT) ? (*currentRun_)->totalWidth() : 0;

	if(scanningDirection() == RIGHT_TO_LEFT) {
		start -= (*currentRun_)->totalWidth();
		end -= (*currentRun_)->totalWidth();
	}
	return Range<Scalar>(start + ipd_, end + ipd_);
}

void InlineProgressionDimensionRangeIterator::doNext(bool initializing) {
	if(isDone())
		throw NoSuchElementException();
	const TextLayout::TextRun* const* nextRun = currentRun_;
	Scalar nextIpd = ipd_;
	while(true) {
		if(scanningDirection() == LEFT_TO_RIGHT) {
			if(!initializing)
				nextIpd += (*nextRun)->totalWidth();
			++nextRun;
		} else {
			if(!initializing)
				nextIpd -= (*nextRun)->totalWidth();
			--nextRun;
		}
		if(nextRun != lastRun_ || (*nextRun)->intersects(characterRange()))
			break;
	}
	// commit
	currentRun_ = nextRun;
	ipd_ = nextIpd;
}


// TextLayout /////////////////////////////////////////////////////////////////////////////////////

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
 * @c TextLayout is an immutable graphical representation of styled text. Provides support for
 * drawing, cursor navigation, hit testing, text wrapping, etc.
 *
 * <h3>Coordinate system</h3>
 * All graphical information returned from a @c TextLayout object' method is relative to the origin
 * of @c TextLayout, which is the intersection of the start edge with the baseline of the first
 * line of @c TextLayout. The start edge is determined by the reading direction (inline progression
 * dimension) of the line. Also, coordinates passed into a @c TextLayout object's method are
 * assumed to be relative to the @c TextLayout object's origin.
 *
 * <h3>Constraints by Win32/Uniscribe</h3>
 * <del>A long run will be split into smaller runs automatically because Uniscribe rejects too long
 * text (especially @c ScriptShape and @c ScriptTextOut). For this reason, a combining character
 * will be rendered incorrectly if it is presented at the boundary. The maximum length of a run is
 * 1024.
 *
 * In present, this class supports only text layout horizontal against the output device.
 *
 * @note This class is not intended to be derived.
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

	// TODO: this implementation is temporary, and should rewrite later
	class SillyLineMetrics : public LineMetrics {
	public:
		void setHeight(Scalar height) /*throw()*/ {height_ = height;}
	private:
		Scalar ascent() const /*throw()*/ {return height_;}
		DominantBaseline baseline() const /*throw()*/ {return DOMINANT_BASELINE_ALPHABETIC;}
		Scalar baselineOffset(AlignmentBaseline baseline) const /*throw()*/ {return 0;}
		Scalar descent() const /*throw()*/ {return 0;}
		Scalar leading() const /*throw()*/ {return 0;}
	private:
		Scalar height_;
	};
}

/**
 * Constructor.
 * @param text The text string to display
 * @param style The text line style
 * @param readingDirection The reading direction of the text layout. This should be either
 *                         @c LEFT_TO_RIGHT or @c RIGHT_TO_LEFT
 * @param anchor The text anchor. This should be either @c TEXT_ANCHOR_START,
 *               @c TEXT_ANCHOR_MIDDLE or @c TEXT_ANCHOR_END
 * @param justification The text justification method
 * @param dominantBaseline The dominant baseline
 * @param fontCollection The font collection this text layout uses
 * @param defaultTextRunStyle The default text run style. Can be @c null
 * @param textRunStyles The text run styles. Can be @c null
 * @param tabExpander The tab expander object. If this is @c null, the default object is used
 * @param width The maximum width
 * @param numberSubstitution Defines number substitution process. Can be @c null
 * @param displayShapingControls Set @c true to shape zero width control characters as
 *                               representative glyphs
 * @param disableDeprecatedFormatCharacters Set @c true to make the deprecated format characters
 *                                          (NADS, NODS, ASS, and ISS) not effective
 * @param inhibitSymmetricSwapping Set c true to inhibit from generating mirrored glyphs
 * @throw UnknownValueException @a readingDirection or @a anchor is invalid
 */
TextLayout::TextLayout(const String& text, ReadingDirection readingDirection,
		TextAnchor anchor /* = TEXT_ANCHOR_START */, 
		TextJustification justification /* = NO_JUSTIFICATION */,
		DominantBaseline dominantBaseline /* = DOMINANT_BASELINE_AUTO */,
		const FontCollection& fontCollection /* = systemFonts() */,
		tr1::shared_ptr<const presentation::TextRunStyle> defaultTextRunStyle /* = null */,
		auto_ptr<presentation::StyledTextRunIterator> textRunStyles /* null */,
		const TabExpander* tabExpander /* = 0 */, Scalar width /* = numeric_limits<Scalar>::max() */,
		const NumberSubstitution* numberSubstitution /* = 0 */, bool displayShapingControls /* = false */,
		bool inhibitSymmetricSwapping /* = false */, bool disableDeprecatedFormatCharacters /* = false */)
		: text_(text), readingDirection_(readingDirection), anchor_(anchor),
		dominantBaseline_(dominantBaseline), numberOfRuns_(0), numberOfLines_(0),
		maximumInlineProgressionDimension_(-1), wrapWidth_(width) {

	// sanity checks...
	if(readingDirection != LEFT_TO_RIGHT && readingDirection != RIGHT_TO_LEFT)
		throw UnknownValueException("readingDirection");
	if(anchor != TEXT_ANCHOR_START && anchor != TEXT_ANCHOR_MIDDLE && anchor != TEXT_ANCHOR_END)
		throw UnknownValueException("anchor");

	// handle logically empty line
	if(text_.empty()) {
		numberOfRuns_ = 0;
		numberOfLines_ = 1;
		maximumInlineProgressionDimension_ = 0;
		assert(isEmpty());
		return;
	}
#if 0
	// calculate the wrapping width
	if(layoutInformation.layoutSettings().lineWrap.wraps()) {
		wrapWidth_ = layoutInformation.width();
		if(ISpecialCharacterRenderer* scr = layoutInformation.specialCharacterRenderer()) {
			ISpecialCharacterRenderer::LayoutContext lc(context);
			lc.readingDirection = readingDirection();
			wrapWidth_ -= scr->getLineWrappingMarkWidth(lc);
		}
	}
#endif
	// split the text line into text runs as following steps:
	// 1. split the text into script runs (SCRIPT_ITEMs) by Uniscribe
	// 2. split each script runs into atomically-shapable runs (TextRuns) with StyledRunIterator
	// 3. generate glyphs for each text runs
	// 4. position glyphs for each text runs
	// 5. position each text runs
	// 6. justify each text runs if specified
	// 7. create the line metrics

	// 1. split the text into script runs by Uniscribe
	HRESULT hr;

	// 1-1. configure Uniscribe's itemize
	win32::AutoZero<SCRIPT_CONTROL> control;
	win32::AutoZero<SCRIPT_STATE> initialState;
	initialState.uBidiLevel = (readingDirection == RIGHT_TO_LEFT) ? 1 : 0;
//	initialState.fOverrideDirection = 1;
	initialState.fInhibitSymSwap = inhibitSymmetricSwapping;
	initialState.fDisplayZWG = displayShapingControls;
	resolveNumberSubstitution(numberSubstitution, control, initialState);	// ignore result...

	// 1-2. itemize
	// note that ScriptItemize can cause a buffer overflow (see Mozilla bug 366643)
	AutoArray<SCRIPT_ITEM, 128> scriptRuns;
	AutoArray<OPENTYPE_TAG, scriptRuns.STATIC_CAPACITY> scriptTags;
	int estimatedNumberOfScriptRuns = max(static_cast<int>(text_.length()) / 4, 2), numberOfScriptRuns;
	HRESULT(WINAPI* scriptItemizeOpenType)(const WCHAR*, int, int,
		const SCRIPT_CONTROL*, const SCRIPT_STATE*, SCRIPT_ITEM*, OPENTYPE_TAG*, int*) = uspLib->get<0>();
	while(true) {
		scriptRuns.reallocate(estimatedNumberOfScriptRuns);
		scriptTags.reallocate(estimatedNumberOfScriptRuns);
		if(scriptItemizeOpenType != 0)
			hr = (*scriptItemizeOpenType)(text_.data(), static_cast<int>(text_.length()),
				estimatedNumberOfScriptRuns, &control, &initialState, scriptRuns.get(), scriptTags.get(), &numberOfScriptRuns);
		else
			hr = ::ScriptItemize(text_.data(), static_cast<int>(text_.length()),
				estimatedNumberOfScriptRuns, &control, &initialState, scriptRuns.get(), &numberOfScriptRuns);
		if(hr != E_OUTOFMEMORY)	// estimatedNumberOfRuns was enough...
			break;
		estimatedNumberOfScriptRuns *= 2;
	}
	if(disableDeprecatedFormatCharacters) {
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
	TextRun::mergeScriptsAndStyles(text_.data(),
		scriptRuns.get(), scriptTags.get(), numberOfScriptRuns,
		fontCollection, defaultTextRunStyle, textRunStyles, textRuns, styledRanges);
	runs_.reset(new TextRun*[numberOfRuns_ = textRuns.size()]);
	copy(textRuns.begin(), textRuns.end(), runs_.get());
//	shrinkToFit(styledRanges_);

	// 3. generate glyphs for each text runs
	const win32::Handle<HDC> dc(detail::screenDC());
	for(size_t i = 0; i < numberOfRuns_; ++i)
		runs_[i]->shape(dc, text_);
	TextRun::substituteGlyphs(Range<TextRun**>(runs_.get(), runs_.get() + numberOfRuns_), text_);

	// 4. position glyphs for each text runs
	for(size_t i = 0; i < numberOfRuns_; ++i)
		runs_[i]->positionGlyphs(dc, text_, SimpleStyledTextRunIterator(styledRanges, runs_[i]->beginning()));

	// 5. position each text runs
	// wrap into visual lines and reorder runs in each lines
	if(numberOfRuns_ == 0 || wrapWidth_ == numeric_limits<Scalar>::max()) {
		numberOfLines_ = 1;
		lineOffsets_.reset(&SINGLE_LINE_OFFSETS);
		lineFirstRuns_.reset(&SINGLE_LINE_OFFSETS);
		// 5-2. reorder each text runs
		reorder();
		// 5-3. reexpand horizontal tabs
		expandTabsWithoutWrapping();
	} else {
		// 5-1. expand horizontal tabs and wrap into lines
		auto_ptr<TabExpander> temp;
		if(tabExpander == 0) {
			// create default tab expander
			String fontFamilyName;
			FontProperties fontProperties;
			resolveFontSpecifications(fontCollection,
				tr1::shared_ptr<const TextRunStyle>(), defaultTextRunStyle, &fontFamilyName, &fontProperties, 0);
			temp.reset(new FixedWidthTabExpander(
				fontCollection.get(fontFamilyName, fontProperties)->metrics().averageCharacterWidth() * 8));
			tabExpander = temp.get();
		}
		wrap(*tabExpander);
		// 5-2. reorder each text runs
		reorder();
		// 5-3. reexpand horizontal tabs
		// TODO: not implemented.
		// 6. justify each text runs if specified
		if(justification != NO_JUSTIFICATION)
			justify(justification);
	}

	// 7. create line metrics
	// TODO: this code is temporary. should rewrite later.
	lineMetrics_.reset(new LineMetrics*[numberOfLines()]);
	for(length_t line = 0; line < numberOfLines(); ++line) {
		try {
			const TextRun* const lastRun = runs_[(line + 1 < numberOfLines()) ? lineFirstRuns_[line + 1] : numberOfRuns_];
			Scalar height = 0;
			for(const TextRun* run = runs_[lineFirstRuns_[line]]; run != lastRun; ++run)
				height = max(run->font()->metrics().cellHeight(), height);
			auto_ptr<SillyLineMetrics> lineMetrics(new SillyLineMetrics);
			lineMetrics->setHeight(height);
			lineMetrics_[line] = lineMetrics.release();
		} catch(...) {
			while(line > 0)
				delete lineMetrics_[--line];
			throw;
		}
	}
}

/// Destructor.
TextLayout::~TextLayout() /*throw()*/ {
	for(size_t i = 0; i < numberOfRuns_; ++i)
		delete runs_[i];
	for(vector<const InlineArea*>::const_iterator i(inlineAreas_.begin()), e(inlineAreas_.end()); i != e; ++i)
		delete *i;
	if(numberOfLines() == 1) {
		assert(lineOffsets_.get() == &SINGLE_LINE_OFFSETS);
		lineOffsets_.release();
		assert(lineFirstRuns_.get() == &SINGLE_LINE_OFFSETS);
		lineFirstRuns_.release();
	}
	for(size_t i = 0; i < numberOfLines(); ++i)
		delete lineMetrics_[i];
}
#if 0
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
#endif
/**
 * Returns the bidirectional embedding level at specified position.
 * @param column the column
 * @return the embedding level
 * @throw kernel#BadPositionException @a column is greater than the length of the line
 */
ascension::byte TextLayout::bidiEmbeddingLevel(length_t column) const {
	if(numberOfRuns_ == 0) {
		if(column != 0)
			throw kernel::BadPositionException(kernel::Position(INVALID_INDEX, column));
		// use the default level
		return (readingDirection() == RIGHT_TO_LEFT) ? 1 : 0;
	}
	const size_t i = findRunForPosition(column);
	if(i == numberOfRuns_)
		throw kernel::BadPositionException(kernel::Position(INVALID_INDEX, column));
	return runs_[i]->bidiEmbeddingLevel();
}

/**
 * Returns the black box bounds of the characters in the specified range. The black box bounds is
 * an area consisting of the union of the bounding boxes of the all of the characters in the range.
 * The result region can be disjoint.
 * @param range The character range
 * @return The native polygon object encompasses the black box bounds
 * @throw kernel#BadPositionException @a range intersects with the outside of the line
 * @see #bounds(void), #bounds(length_t, length_t), #lineBounds, #lineStartEdge
 */
NativePolygon TextLayout::blackBoxBounds(const Range<length_t>& range) const {
	if(range.end() > text_.length())
		throw kernel::BadPositionException(kernel::Position(INVALID_INDEX, range.end()));

	// handle empty line
	if(numberOfRuns_ == 0)
		return win32::Handle<HRGN>(::CreateRectRgn(0, 0, 0, lineMetrics_[0]->height()), &::DeleteObject);

	// TODO: this implementation can't handle vertical text.
	const length_t firstLine = lineAt(range.beginning()), lastLine = lineAt(range.end());
	vector<Rect<> > rectangles;
	Scalar before = blockProgressionDistance(0, firstLine)
		- lineMetrics_[firstLine]->leading() - lineMetrics_[firstLine]->ascent();
	Scalar after = before + lineMetrics_[firstLine]->height();
	for(length_t line = firstLine; line <= lastLine; before = after, after += lineMetrics_[++line]->height()) {
		const size_t lastRun = (line + 1 < numberOfLines()) ? lineFirstRuns_[line + 1] : numberOfRuns_;
		const Scalar leftEdge = (readingDirection() == LEFT_TO_RIGHT) ?
			lineStartEdge(line) : (-lineStartEdge(line) - lineInlineProgressionDimension(line));

		// is the whole line encompassed by the range?
		if(range.beginning() <= lineOffset(line) && range.end() >= lineOffset(line) + lineLength(line))
			rectangles.push_back(Rect<>(
				Point<>(leftEdge, before), Point<>(leftEdge + lineInlineProgressionDimension(line), after)));
		else {
			for(InlineProgressionDimensionRangeIterator i(
					Range<const TextRun* const*>(runs_.get() + lineFirstRuns_[line], runs_.get() + lastRun),
					range, LEFT_TO_RIGHT, leftEdge), e; i != e; ++i)
				rectangles.push_back(Rect<>(Point<>(i->beginning(), before), Point<>(i->end(), after)));
		}
	}

	// create the result region
	AutoBuffer<POINT> vertices(new POINT[rectangles.size() * 4]);
	AutoBuffer<int> numbersOfVertices(new int[rectangles.size()]);
	for(size_t i = 0, c = rectangles.size(); i < c; ++i) {
		vertices[i * 4 + 0].x = vertices[i * 4 + 3].x = rectangles[i].left();
		vertices[i * 4 + 0].y = vertices[i * 4 + 1].y = rectangles[i].top();
		vertices[i * 4 + 1].x = vertices[i * 4 + 2].x = rectangles[i].right();
		vertices[i * 4 + 2].y = vertices[i * 4 + 3].y = rectangles[i].bottom();
	}
	fill_n(numbersOfVertices.get(), rectangles.size(), 4);
	return win32::Handle<HRGN>(::CreatePolyPolygonRgn(vertices.get(),
		numbersOfVertices.get(), static_cast<int>(rectangles.size()), WINDING), &::DeleteObject);
}

inline Scalar TextLayout::blockProgressionDistance(length_t from, length_t to) const /*throw()*/ {
	Scalar result = 0;
	while(from < to) {
		result += lineMetrics_[from]->descent();
		result += lineMetrics_[++from]->leading();
		result += lineMetrics_[from]->ascent();
	}
	while(from > to) {
		result -= lineMetrics_[from]->ascent();
		result -= lineMetrics_[from]->leading();
		result -= lineMetrics_[--from]->descent();
	}
	return result;
}

/**
 * Returns the smallest rectangle emcompasses the whole text of the line. It might not coincide
 * exactly the ascent, descent or overhangs of the text.
 * @return The size of the bounds
 * @see #blackBoxBounds, #bounds(length_t, length_t), #lineBounds
 */
Rect<> TextLayout::bounds() const /*throw()*/ {
	// TODO: this implementation can't handle vertical text.
	const Scalar before = -lineMetrics_[0]->leading() - lineMetrics_[0]->ascent();
	Scalar after = before, start = numeric_limits<Scalar>::max(), end = numeric_limits<Scalar>::min();
	for(length_t line = 0; line < numberOfLines(); ++line) {
		after += lineMetrics_[line]->height();
		const Scalar lineStart = lineStartEdge(line);
		start = min(lineStart, start);
		end = max(lineStart + lineInlineProgressionDimension(line), end);
	}
	return Rect<>(
		Point<>((readingDirection() == LEFT_TO_RIGHT) ? start : -end, before),
		Point<>((readingDirection() == LEFT_TO_RIGHT) ? end : -start, after));
}

/**
 * Returns the smallest rectangle emcompasses all characters in the range. It might not coincide
 * exactly the ascent, descent or overhangs of the specified region of the text.
 * @param range The range
 * @return The bounds
 * @throw kernel#BadPositionException @a range intersects with the outside of the line
 * @see #blackBoxBounds, #bounds(void), #lineBounds, #lineIndent
 */
Rect<> TextLayout::bounds(const Range<length_t>& range) const {
	if(range.end() > text_.length())
		throw kernel::BadPositionException(kernel::Position(INVALID_INDEX, range.end()));

	Scalar start, end, before, after;

	// TODO: this implementation can't handle vertical text.

	if(isEmpty()) {	// empty line
		start = end = 0;
		before = -lineMetrics_[0]->ascent() - lineMetrics_[0]->leading();
		after = lineMetrics_[0]->descent();
	} else if(range.isEmpty()) {	// an empty rectangle for an empty range
		const LineMetrics& line = *lineMetrics_[lineAt(range.beginning())];
		return Rect<>(
			location(range.beginning()) -= Dimension<>(0, line.ascent() + line.leading()),
			Dimension<>(0, line.height()));
	} else {
		const length_t firstLine = lineAt(range.beginning()), lastLine = lineAt(range.end());

		// calculate the block-progression-edges ('before' and 'after'; it's so easy)
		{
			const Scalar firstBaseline = blockProgressionDistance(0, firstLine);
			before = firstBaseline - lineMetrics_[firstLine]->ascent() - lineMetrics_[firstLine]->leading();
			after = firstBaseline + blockProgressionDistance(firstLine, lastLine) + lineMetrics_[lastLine]->descent();
		}

		// calculate start-edge and end-edge of fully covered lines
		const bool firstLineIsFullyCovered = range.includes(
			makeRange(lineOffset(firstLine), lineOffset(firstLine) + lineLength(firstLine)));
		const bool lastLineIsFullyCovered = range.includes(
			makeRange(lineOffset(lastLine), lineOffset(lastLine) + lineLength(lastLine)));
		start = numeric_limits<Scalar>::max();
		end = numeric_limits<Scalar>::min();
		for(length_t line = firstLine + firstLineIsFullyCovered ? 0 : 1;
				line < lastLine + lastLineIsFullyCovered ? 1 : 0; ++line) {
			const Scalar lineStart = lineStartEdge(line);
			start = min(lineStart, start);
			end = max(lineStart + lineInlineProgressionDimension(line), end);
		}

		// calculate start and end-edge of partially covered lines
		vector<length_t> partiallyCoveredLines;
		if(!firstLineIsFullyCovered)
			partiallyCoveredLines.push_back(firstLine);
		if(!lastLineIsFullyCovered && (partiallyCoveredLines.empty() || partiallyCoveredLines[0] != lastLine))
			partiallyCoveredLines.push_back(lastLine);
		if(!partiallyCoveredLines.empty()) {
			Scalar left = (readingDirection() == LEFT_TO_RIGHT) ? start : -end;
			Scalar right = (readingDirection() == LEFT_TO_RIGHT) ? end : -start;
			for(vector<length_t>::const_iterator
					line(partiallyCoveredLines.begin()), e(partiallyCoveredLines.end()); line != e; ++line) {
				const length_t lastRun = (*line + 1 < numberOfLines()) ? lineFirstRuns_[*line + 1] : numberOfRuns_;

				// find left-edge
				InlineProgressionDimensionRangeIterator i(
					Range<const TextRun* const*>(runs_.get() + lineFirstRuns_[*line], runs_.get() + lastRun),
					range, LEFT_TO_RIGHT, (readingDirection() == LEFT_TO_RIGHT) ?
						lineStartEdge(*line) : -lineStartEdge(*line) - lineInlineProgressionDimension(*line));
				assert(i != InlineProgressionDimensionRangeIterator());
				left = min(i->beginning(), left);

				Scalar x = (readingDirection() == LEFT_TO_RIGHT) ?
					lineStartEdge(*line) : -lineStartEdge(*line) - lineInlineProgressionDimension(*line);
				for(length_t i = lineFirstRuns_[*line];
						i < lastRun && x < left; x += runs_[i++]->totalWidth()) {
					const TextRun& run = *runs_[i];
					if(range.intersects(run)) {
						const length_t leftEdge = (run.readingDirection() == LEFT_TO_RIGHT) ?
							max(range.beginning(), run.beginning()) : min(range.end(), run.end());
						left = min(x + run.x(leftEdge, false), left);
						break;
					}
				}

				// find right-edge
				i = InlineProgressionDimensionRangeIterator(
					Range<const TextRun* const*>(runs_.get() + lineFirstRuns_[*line], runs_.get() + lastRun),
					range, RIGHT_TO_LEFT, (readingDirection() == LEFT_TO_RIGHT) ?
						lineStartEdge(*line) + lineInlineProgressionDimension(*line) : -lineStartEdge(*line));
				assert(i != InlineProgressionDimensionRangeIterator());
				right = max(i->end(), right);

				x = (readingDirection() == LEFT_TO_RIGHT) ?
					lineStartEdge(*line) + lineInlineProgressionDimension(*line) : -lineStartEdge(*line);
				for(length_t i = lastRun - 1; x > right; x -= runs_[i--]->totalWidth()) {
					const TextRun& run = *runs_[i];
					if(range.intersects(run)) {
						const length_t rightEdge = (run.readingDirection() == LEFT_TO_RIGHT) ?
							min(range.end(), run.end()) : max(range.beginning(), run.beginning());
						right = max(x - run.totalWidth() + run.x(rightEdge, false), right);
						break;
					}
					if(i == lineFirstRuns_[*line])
						break;
				}
			}

			start = (readingDirection() == LEFT_TO_RIGHT) ? left : -right;
			end = (readingDirection() == LEFT_TO_RIGHT) ? right : -left;
		}
	}

	return Rect<>(Point<>(start, before), Point<>(end, after));
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

/**
 * Draws the specified line layout to the output device.
 * @param context The rendering context
 * @param origin The alignment point of the text layout
 * @param paintOverride Can be @c null
 * @param endOfLine The inline object which paints an end-of-line. Can be @c null
 * @param lineWrappingMark The inline object which paints line-wrapping-mark. Can be @c null
 */
void TextLayout::draw(PaintContext& context,
		const Point<>& origin, const TextPaintOverride* paintOverride /* = 0 */,
		const InlineObject* endOfLine/* = 0 */, const InlineObject* lineWrappingMark /* = 0 */) const {

#if /*defined(_DEBUG)*/ 0
	if(DIAGNOSE_INHERENT_DRAWING)
		win32::DumpContext() << L"@TextLayout.draw draws line " << lineNumber_ << L" (" << line << L")\n";
#endif // defined(_DEBUG)

	if(isEmpty() || context.boundsToPaint().height() == 0)
		return;

	// TODO: this code can't handle vertical text.

	// calculate line range to draw
	Range<length_t> linesToDraw(0, numberOfLines());
	Point<> p(origin);
	for(length_t line = linesToDraw.beginning(); line < linesToDraw.end(); ++line) {
		const Scalar lineBeforeEdge = p.y - lineMetrics_[line]->ascent();
		const Scalar lineAfterEdge = p.y + lineMetrics_[line]->descent();
		if(context.boundsToPaint().top() >= lineBeforeEdge && context.boundsToPaint().top() < lineAfterEdge)
			linesToDraw = makeRange(line, linesToDraw.end());
		if(context.boundsToPaint().bottom() >= lineBeforeEdge && context.boundsToPaint().bottom() < lineAfterEdge) {
			linesToDraw = makeRange(linesToDraw.beginning(), line + 1);
			break;
		}
		p.y += blockProgressionDistance(line - 1, line);
	}

	// calculate inline area range to draw
	Range<vector<const InlineArea*>::const_iterator> inlineAreasToDraw(inlineAreas_.begin(), inlineAreas_.end());
	for(vector<const InlineArea*>::const_iterator i(inlineAreasToDraw.beginning()); i != inlineAreasToDraw.end(); ++i) {
		const length_t endOfInlineArea = (i != inlineAreasToDraw.end()) ? i[1]->position() : text_.length();
		if(endOfInlineArea > lineOffset(linesToDraw.beginning())) {
			inlineAreasToDraw = makeRange(i, inlineAreasToDraw.end());
			break;
		}
	}
	for(vector<const InlineArea*>::const_iterator i(inlineAreasToDraw.beginning()); i != inlineAreasToDraw.end(); ++i) {
		const length_t endOfInlineArea = (i != inlineAreasToDraw.end()) ? i[1]->position() : text_.length();
		if(endOfInlineArea >= lineOffset(linesToDraw.beginning())) {
			inlineAreasToDraw = makeRange(inlineAreasToDraw.beginning(), i + 1);
			break;
		}
	}

	// this code paints the line in the following steps:
	// 1. calculate range of runs to paint
	// 2. paint backgrounds and borders:
	//   2-1. paint background if the property is specified
	//   2-2. paint border if the property is specified
	// 3. for each text runs:
	//   3-1. calculate range of runs to paint
	//   3-2. paint the glyphs of the text run
	//   3-3. paint the overhanging glyphs of the around text runs
	//   3-4. paint the text decoration
	// 4. paint the end of line mark
	//
	// the following topics describe how to draw a styled and selected text using masking by clipping
	// Catch 22 : Design and Implementation of a Win32 Text Editor
	// Part 10 - Transparent Text and Selection Highlighting (http://www.catch22.net/tuts/neatpad/10)
	// Part 14 - Drawing styled text with Uniscribe (http://www.catch22.net/tuts/neatpad/14)

	context.save();
	::SetTextAlign(context.nativeHandle().get(), TA_TOP | TA_LEFT | TA_NOUPDATECP);

	// 2. paint backgrounds and borders
	for(vector<const InlineArea*>::const_iterator i(inlineAreasToDraw.beginning()), e; i != inlineAreasToDraw.end(); ++i) {
		// TODO: recognize the override.
		// TODO: this code can't handle sparse inline areas (with bidirectionality).
		pair<Rect<>, bool> borderRectangle;

		// 2-1. paint background if the property is specified
		if((*i)->style()->background != Paint()) {
			borderRectangle = make_pair((*i)->borderRectangle(), true);
			if(context.boundsToPaint().includes(borderRectangle.first)) {
				context.setFillStyle((*i)->style()->background);
				context.fillRectangle(borderRectangle.first);
			}
		}

		// 2-2. paint border if the property is specified
		pair<Color, bool> currentColor;
		const Border::Part* borders[4] = {
			&(*i)->style()->border.before, &(*i)->style()->border.after,
			&(*i)->style()->border.start, &(*i)->style()->border.end};
		for(const Border::Part** border = border = borders; border != ASCENSION_ENDOF(borders); ++border) {
			if(!(*border)->hasVisibleStyle() || (*border)->computedWidth().value <= 0.0)
				continue;
			if((*border)->color == Color()) {
				if(!currentColor.second)
					currentColor = make_pair(Color(), true);
			}
			if(!borderRectangle.second)
				borderRectangle = make_pair((*i)->borderRectangle(), true);
			if(!context.boundsToPaint().includes(borderRectangle.first))
				continue;
//			context.setStrokeStyle();
//			context.setStrokeDashArray();
//			context.setStrokeDashOffset();
			context.beginPath();
			if(border == &borders[0])	// top
				context
					.moveTo(Point<>(borderRectangle.first.left(), borderRectangle.first.top()))
					.lineTo(Point<>(borderRectangle.first.right() + 1, borderRectangle.first.top()));
			else if(border == &borders[1])	// bottom
				context
					.moveTo(Point<>(borderRectangle.first.left(), borderRectangle.first.bottom()))
					.lineTo(Point<>(borderRectangle.first.right() + 1, borderRectangle.first.bottom()));
			else if((readingDirection() == LEFT_TO_RIGHT && border == &borders[2])
					|| (readingDirection() == RIGHT_TO_LEFT && border == &borders[3]))	// left
				context
					.moveTo(Point<>(borderRectangle.first.left(), borderRectangle.first.top()))
					.lineTo(Point<>(borderRectangle.first.left(), borderRectangle.first.bottom() + 1));
			else if((readingDirection() == LEFT_TO_RIGHT && border == &borders[3])
					|| (readingDirection() == RIGHT_TO_LEFT && border == &borders[2]))	// right
				context
					.moveTo(Point<>(borderRectangle.first.right(), borderRectangle.first.top()))
					.lineTo(Point<>(borderRectangle.first.right(), borderRectangle.first.bottom() + 1));
			context.stroke();
		}

		::ExcludeClipRect(context.nativeHandle().get(),
			borderRectangle.first.left(), borderRectangle.first.top(),
			borderRectangle.first.right(), borderRectangle.first.bottom());
	}

	// 3. for each text runs
	for(length_t line = linesToDraw.beginning(); line < linesToDraw.end(); ++line) {
		if(!isEmpty()) {
			// 3-1. calculate range of runs to paint
			Range<const TextRun* const*> runs(runs_.get() + lineFirstRuns_[line],
				runs_.get() + ((line < numberOfLines() - 1) ? lineFirstRuns_[line + 1] : numberOfRuns_));
			p = origin;
			p.x += readingDirectionInt(readingDirection());
			if(readingDirection() == RIGHT_TO_LEFT)
				p.x -= lineInlineProgressionDimension(line);
			Scalar leftEdgeOfFirstRun = p.x, rightEdgeOfLastRun = p.x + lineInlineProgressionDimension(line);
			for(const TextRun* const* run = runs.beginning(); run < runs.end(); ++run) {
				if(p.x + (*run)->totalWidth() < context.boundsToPaint().left()) {
					runs = makeRange(run + 1, runs.end());
					leftEdgeOfFirstRun = p.x + (*run)->totalWidth();
				} else if(p.x > context.boundsToPaint().right()) {
					runs = makeRange(runs.beginning(), run);
					rightEdgeOfLastRun = p.x;
				}
			}
			if(!runs.isEmpty()) {
				const Range<length_t> characterRange(runs.beginning()[0]->beginning(), runs.end()[-1]->end());
				auto_ptr<TextPaintOverride::Iterator> paintOverrideIterator;
				if(paintOverride != 0)
					paintOverrideIterator = paintOverride->queryTextPaintOverride(characterRange);
			}

			// 3-2. paint the glyphs of the text run

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
#if 0
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
#endif
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
	if(column == text_.length())
		return numberOfRuns_ - 1;
	const length_t sl = lineAt(column);
	const size_t lastRun = (sl + 1 < numberOfLines()) ? lineFirstRuns_[sl + 1] : numberOfRuns_;
	for(size_t i = lineFirstRuns_[sl]; i < lastRun; ++i) {
		if(runs_[i]->beginning() <= column && runs_[i]->end() > column)	// TODO: replace with includes().
			return i;
	}
	ASCENSION_ASSERT_NOT_REACHED();
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
inline void TextLayout::justify(TextJustification) /*throw()*/ {
	assert(wrapWidth_ != -1);
	for(length_t line = 0; line < numberOfLines(); ++line) {
		const int ipd = lineInlineProgressionDimension(line);
		const size_t last = (line + 1 < numberOfLines()) ? lineFirstRuns_[line + 1] : numberOfRuns_;
		for(size_t i = lineFirstRuns_[line]; i < last; ++i) {
			TextRun& run = *runs_[i];
			const int newRunWidth = ::MulDiv(run.totalWidth(), wrapWidth_, ipd);	// TODO: there is more precise way.
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
 * Returns the smallest rectangle emcompasses the specified line. It might not coincide exactly the
 * ascent, descent or overhangs of the specified line.
 * @param line The line number
 * @return The line bounds in pixels
 * @throw IndexOutOfBoundsException @a line is greater than the number of the lines
 * @see #lineStartEdge
 */
Rect<> TextLayout::lineBounds(length_t line) const {
	if(line >= numberOfLines())
		throw IndexOutOfBoundsException("line");

	const Scalar start = lineStartEdge(line);
	const Scalar end = start + lineInlineProgressionDimension(line);
	const Scalar before = blockProgressionDistance(0, line)
		- lineMetrics_[line]->ascent() - lineMetrics_[line]->leading();
	const Scalar after = before + lineMetrics_[line]->height();

	// TODO: this implementation can't handle vertical text.
	const Dimension<> size(end - start, after - before);
	const Point<> origin((readingDirection() == LEFT_TO_RIGHT) ? start : start - size.cx, before);
	return Rect<>(origin, size);
}

/**
 * Returns the length in inline-progression-dimension without the indentations (the distance from
 * the start-edge to the end-edge) of the specified line in pixels.
 * @param line The line number
 * @return The width. Must be equal to or greater than zero
 * @throw IndexOutOfBoundsException @a line is greater than the number of lines
 * @see #maximumInlineProgressionDimension
 */
Scalar TextLayout::lineInlineProgressionDimension(length_t line) const {
	if(line >= numberOfLines())
		throw IndexOutOfBoundsException("line");
	else if(isEmpty())
		return const_cast<TextLayout*>(this)->maximumInlineProgressionDimension_ = 0;
	else {
		TextLayout& self = const_cast<TextLayout&>(*this);
		if(numberOfLines() == 1) {
			if(maximumInlineProgressionDimension_ >= 0)
				return maximumInlineProgressionDimension_;
		} else {
			if(lineInlineProgressionDimensions_.get() == 0) {
				self.lineInlineProgressionDimensions_.reset(new Scalar[numberOfLines()]);
				fill_n(self.lineInlineProgressionDimensions_.get(), numberOfLines(), -1);
			}
			if(lineInlineProgressionDimensions_[line] >= 0)
				return lineInlineProgressionDimensions_[line];
		}
		const size_t lastRun = (line + 1 < numberOfLines()) ? lineFirstRuns_[line + 1] : numberOfRuns_;
		Scalar ipd = 0;
		for(size_t i = lineFirstRuns_[line]; i < lastRun; ++i)
			ipd += runs_[i]->totalWidth();
		assert(ipd >= 0);
		if(numberOfLines() == 1)
			self.maximumInlineProgressionDimension_ = ipd;
		else
			self.lineInlineProgressionDimensions_[line] = ipd;
		return ipd;
	}
}

/**
 * Returns the start-edge of the specified line without the start-indent.
 * @par This is distance from the origin (the alignment point of the first line) to @a line in
 * inline-progression-dimension. Therefore, returns always zero when @a line is zero or the anchor
 * is @c TEXT_ANCHOR_START.
 * @par A positive value means positive indentation. For example, if the start-edge of a RTL line
 * is x = -10, this method returns +10.
 * @param line The line number
 * @return The start-indentation in pixels
 * @throw IndexOutOfBoundsException @a line is invalid
 */
Scalar TextLayout::lineStartEdge(length_t line) const {
	if(line == 0)
		return 0;
	switch(anchor()) {
	case TEXT_ANCHOR_START:
		return 0;
	case TEXT_ANCHOR_MIDDLE:
		return (lineInlineProgressionDimension(0) - lineInlineProgressionDimension(line)) / 2;
	case TEXT_ANCHOR_END:
		return lineInlineProgressionDimension(0) - lineInlineProgressionDimension(line);
	default:
		ASCENSION_ASSERT_NOT_REACHED();
	}
}

/**
 * @internal Converts a block-progression-dimension into the corresponding line.
 * @param bpd The block-progression-dimension
 * @param[out] outside @c true if @a bpd is outside of the line content
 * @return The line number
 */
length_t TextLayout::locateLine(Scalar bpd, bool& outside) const /*throw()*/ {
	// TODO: this implementation can't handle vertical text.

	// beyond the before-edge ?
	if(bpd < -lineMetrics_[0]->ascent() - lineMetrics_[0]->leading())
		return (outside = true), 0;

	length_t line = 0;
	for(Scalar lineAfter = 0; line < numberOfLines() - 1; ++line) {
		if(bpd < (lineAfter += lineMetrics_[line]->height()))
			return (outside = false), line;
	}

	// beyond the after-edge
	return (outside = true), numberOfLines() - 1;
}

/**
 * @internal Converts an inline-progression-dimension into character offset(s) in the line.
 * @param line The line number
 * @param ipd The inline-progression-dimension
 * @param[out] outside @c true if @a ipd is outside of the line content
 * @return See the documentation of @c #offset method
 * @throw IndexOutOfBoundsException @a line is invalid
 */
pair<length_t, length_t> TextLayout::locateOffsets(length_t line, Scalar ipd, bool& outside) const {
	if(isEmpty())
		return (outside = true), make_pair(static_cast<length_t>(0), static_cast<length_t>(0));
	const size_t lastRun = (line + 1 < numberOfLines()) ? lineFirstRuns_[line + 1] : numberOfRuns_;

	if(readingDirection() == LEFT_TO_RIGHT) {
		Scalar x = lineStartEdge(line);
		if(ipd < x) {	// beyond the left-edge => the start of the first run
			const length_t column = runs_[lineFirstRuns_[line]]->beginning();
			return (outside = true), make_pair(column, column);
		}
		for(size_t i = lineFirstRuns_[line]; i < lastRun; ++i) {	// scan left to right
			const TextRun& run = *runs_[i];
			if(ipd >= x && ipd <= x + run.totalWidth()) {
				int cp, trailing;
				run.hitTest(ipd - x, cp, trailing);	// TODO: check the returned value.
				const length_t temp = run.beginning() + static_cast<length_t>(cp);
				return (outside = false), make_pair(temp, temp + static_cast<length_t>(trailing));
			}
			x += run.totalWidth();
		}
		// beyond the right-edge => the end of last run
		const length_t column = runs_[lastRun - 1]->end();
		return (outside = true), make_pair(column, column);
	} else {
		Scalar x = -lineStartEdge(line);
		if(ipd > x) {	// beyond the right-edge => the start of the last run
			const length_t column = runs_[lastRun - 1]->beginning();
			return (outside = true), make_pair(column, column);
		}
		// beyond the left-edge => the end of the first run
		const length_t column = runs_[lineFirstRuns_[line]]->end();
		return (outside = true), make_pair(column, column);
	}
}

// implements public location methods
void TextLayout::locations(length_t column, Point<>* leading, Point<>* trailing) const {
	assert(leading != 0 || trailing != 0);
	if(column > text_.length())
		throw kernel::BadPositionException(kernel::Position(INVALID_INDEX, column));

	Scalar leadingIpd, trailingIpd, bpd = lineMetrics_[0]->ascent() + lineMetrics_[0]->leading();
	if(isEmpty())
		leadingIpd = trailingIpd = 0;
	else {
		// inline-progression-dimension
		const length_t line = lineAt(column);
		const length_t firstRun = lineFirstRuns_[line];
		const length_t lastRun = (line + 1 < numberOfLines()) ? lineFirstRuns_[line + 1] : numberOfRuns_;
		if(readingDirection() == LEFT_TO_RIGHT) {	// LTR
			Scalar x = lineStartEdge(line);
			for(size_t i = firstRun; i < lastRun; ++i) {
				const TextRun& run = *runs_[i];
				if(column >= run.beginning() && column <= run.end()) {
					if(leading != 0)
						leadingIpd = x + run.x(column, false);
					if(trailing != 0)
						trailingIpd = x + run.x(column, true);
					break;
				}
				x += run.totalWidth();
			}
		} else {	// RTL
			Scalar x = -lineStartEdge(line);
			for(size_t i = lastRun - 1; ; --i) {
				const TextRun& run = *runs_[i];
				x -= run.totalWidth();
				if(column >= run.beginning() && column <= run.end()) {
					if(leading != 0)
						leadingIpd = -(x + run.x(column, false));
					if(trailing)
						trailingIpd = -(x + run.x(column, true));
					break;
				}
				if(i == firstRun) {
					ASCENSION_ASSERT_NOT_REACHED();
					break;
				}
			}
		}

		// block-progression-dimension
		bpd += blockProgressionDistance(0, line);
	}
		
	// TODO: this implementation can't handle vertical text.
	if(leading != 0) {
		leading->x = leadingIpd;
		leading->y = bpd;
	}
	if(trailing != 0) {
		trailing->x = trailingIpd;
		trailing->y = bpd;
	}
}

/**
 * Returns the inline-progression-dimension of the longest line.
 * @see #lineInlineProgressionDimension
 */
Scalar TextLayout::maximumInlineProgressionDimension() const /*throw()*/ {
	if(maximumInlineProgressionDimension_ < 0) {
		Scalar ipd = 0;
		for(length_t line = 0; line < numberOfLines(); ++line)
			ipd = max(lineInlineProgressionDimension(line), ipd);
		const_cast<TextLayout*>(this)->maximumInlineProgressionDimension_ = ipd;
	}
	return maximumInlineProgressionDimension_;
}

/// Reorders the runs in visual order.
inline void TextLayout::reorder() /*throw()*/ {
	if(numberOfRuns_ == 0)
		return;
	AutoBuffer<TextRun*> temp(new TextRun*[numberOfRuns_]);
	copy(runs_.get(), runs_.get() + numberOfRuns_, temp.get());
	for(length_t line = 0; line < numberOfLines(); ++line) {
		const size_t numberOfRunsInLine = ((line < numberOfLines() - 1) ?
			lineFirstRuns_[line + 1] : numberOfRuns_) - lineFirstRuns_[line];
		const AutoBuffer<BYTE> levels(new BYTE[numberOfRunsInLine]);
		for(size_t i = 0; i < numberOfRunsInLine; ++i)
			levels[i] = static_cast<BYTE>(runs_[i + lineFirstRuns_[line]]->bidiEmbeddingLevel() & 0x1f);
		const AutoBuffer<int> log2vis(new int[numberOfRunsInLine]);
		const HRESULT hr = ::ScriptLayout(static_cast<int>(numberOfRunsInLine), levels.get(), 0, log2vis.get());
		assert(SUCCEEDED(hr));
		for(size_t i = lineFirstRuns_[line]; i < lineFirstRuns_[line] + numberOfRunsInLine; ++i)
			runs_[lineFirstRuns_[line] + log2vis[i - lineFirstRuns_[line]]] = temp[i];
	}
}
#if 0
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
#endif

#if 0
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
#endif
#if 0
/**
 * Returns the styled text run containing the specified column.
 * @param column the column
 * @return the styled segment
 * @throw kernel#BadPositionException @a column is greater than the length of the line
 */
StyledRun TextLayout::styledTextRun(length_t column) const {
	if(column > text().length())
		throw kernel::BadPositionException(kernel::Position(INVALID_INDEX, column));
	const TextRun& run = *runs_[findRunForPosition(column)];
	return StyledRun(run.column(), run.requestedStyle());
}
#endif

/// Locates the wrap points and resolves tab expansions.
void TextLayout::wrap(const TabExpander& tabExpander) /*throw()*/ {
	assert(numberOfRuns_ != 0 && wrapWidth_ != numeric_limits<Scalar>::max());
	assert(numberOfLines_ == 0 && lineOffsets_.get() == 0 && lineFirstRuns_.get() == 0);

	vector<length_t> lineFirstRuns;
	lineFirstRuns.push_back(0);
	int x1 = 0;	// addresses the beginning of the run. see x2
	AutoBuffer<int> logicalWidths;
	AutoBuffer<SCRIPT_LOGATTR> logicalAttributes;
	length_t longestRunLength = 0;	// for efficient allocation
	vector<TextRun*> newRuns;
	newRuns.reserve(numberOfRuns_ * 3 / 2);
	// for each runs... (at this time, runs_ is in logical order)
	for(size_t i = 0; i < numberOfRuns_; ++i) {
		TextRun* run = runs_[i];

		// if the run is a tab, expand and calculate actual width
		if(run->expandTabCharacters(tabExpander, text_,
				(x1 < wrapWidth_) ? x1 : 0, wrapWidth_ - (x1 < wrapWidth_) ? x1 : 0)) {
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
		hr = run->logicalAttributes(text_, logicalAttributes.get());
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
					if(lastBreakable < text_.length()) {
						assert(lineFirstRuns.empty() || newRuns.size() != lineFirstRuns.back());
						lineFirstRuns.push_back(newRuns.size() + 1);
//dout << L"broke the line at " << lastBreakable << L" where the run end.\n";
					}
					break;
				}
				// case 3: break at the middle of the run -> split the run (run -> newRun + run)
				else {
					auto_ptr<TextRun> followingRun(run->breakAt(lastBreakable, text_));
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
	if(newRuns.empty())
		newRuns.push_back(0);
	runs_.reset(new TextRun*[numberOfRuns_ = newRuns.size()]);
	copy(newRuns.begin(), newRuns.end(), runs_.get());

	{
		assert(numberOfLines() > 1);
		AutoBuffer<length_t> temp(new length_t[numberOfLines_ = lineFirstRuns.size()]);
		copy(lineFirstRuns.begin(), lineFirstRuns.end(), temp.get());
		lineFirstRuns_.reset(temp.release());
	}

	lineOffsets_.reset(new length_t[numberOfLines()]);
	for(size_t i = 0; i < numberOfLines(); ++i)
		const_cast<length_t&>(lineOffsets_[i]) = runs_[lineFirstRuns_[i]]->beginning();
}

#if 0
// TextLayout.StyledSegmentIterator ///////////////////////////////////////////////////////////////

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
