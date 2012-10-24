/**
 * @file text-layout.cpp
 * @author exeal
 * @date 2003-2006 (was TextLayout.cpp)
 * @date 2006-2011
 * @date 2010-11-20 renamed from ascension/layout.cpp
 * @date 2012-09-01 separated from text-layout.cpp
 */

#include <ascension/config.hpp>	// ASCENSION_DEFAULT_LINE_LAYOUT_CACHE_SIZE, ...
#include <ascension/graphics/text-layout.hpp>
#include <ascension/graphics/text-layout-styles.hpp>
#include <ascension/graphics/text-run.hpp>
#include <ascension/graphics/rendering-context.hpp>
#include <ascension/graphics/rendering-device.hpp>
//#include <ascension/graphics/special-character-renderer.hpp>
#include <ascension/corelib/shared-library.hpp>
#include <ascension/corelib/text/character-iterator.hpp>
#include <ascension/corelib/text/character-property.hpp>
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
		COLORREF serve(const boost::optional<Color>& color, int index) const {return color ? color->as<COLORREF>() : get(index);}
		void update() /*throw()*/ {for(int i = 0; i < ASCENSION_COUNTOF(c_); ++i) c_[i] = ::GetSysColor(i);}
	private:
		COLORREF c_[128];
	} systemColors;

	const class ScriptProperties {
	public:
		ScriptProperties() /*throw()*/ : p_(nullptr), c_(0) {::ScriptGetProperties(&p_, &c_);}
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
	unique_ptr<detail::SharedLibrary<Uniscribe16>> uspLib(
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

	inline int estimateNumberOfGlyphs(Index length) {
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
					win32::Handle<HDC> dc(::GetDC(nullptr), bind(&::ReleaseDC, static_cast<HWND>(nullptr), placeholders::_1));
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
		OUTLINETEXTMETRICW* otm = nullptr;
		TEXTMETRICW tm;
		if(const UINT c = ::GetOutlineTextMetricsW(dc.get(), 0, nullptr)) {
			otm = static_cast<OUTLINETEXTMETRICW*>(::operator new(c));
			if(!win32::boole(::GetOutlineTextMetricsW(dc.get(), c, otm)))
				return false;
		} else if(!win32::boole(::GetTextMetricsW(dc.get(), &tm)))
			return false;
		const int baseline = (otm != nullptr) ? otm->otmTextMetrics.tmAscent : tm.tmAscent;
		if(baselineOffset != nullptr)
			*baselineOffset = baseline;
		if(underlineOffset != nullptr)
			*underlineOffset = (otm != nullptr) ? otm->otmsUnderscorePosition : baseline;
		if(underlineThickness != nullptr)
			*underlineThickness = (otm != nullptr) ? otm->otmsUnderscoreSize : 1;
		if(strikethroughOffset != nullptr)
			*strikethroughOffset = (otm != nullptr) ? otm->otmsStrikeoutPosition : (baseline / 3);
		if(strikethroughThickness != nullptr)
			*strikethroughThickness = (otm != nullptr) ? otm->otmsStrikeoutSize : 1;
		::operator delete(otm);
		return true;
	}

	inline bool isC0orC1Control(CodePoint c) /*throw()*/ {
		return c < 0x20 || c == 0x7f || (c >= 0x80 && c < 0xa0);
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
			const ComputedNumberSubstitution* configuration, SCRIPT_CONTROL& sc, SCRIPT_STATE& ss) {
		if(configuration == nullptr || configuration->method == NumberSubstitution::USER_SETTING)
			return ::ScriptApplyDigitSubstitution(&userSettings.digitSubstitution(
				(configuration != nullptr) ? configuration->ignoreUserOverride : false), &sc, &ss);

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
					ASCENSION_COUNTOF(items), nullptr, nullptr, items, &numberOfItems)) && numberOfItems == 1)
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


// graphics.font.* free functions /////////////////////////////////////////////////////////////////

bool font::supportsComplexScripts() /*noexcept*/ {
	return true;
}

bool font::supportsOpenTypeFeatures() /*noexcept*/ {
	return uspLib->get<0>() != nullptr;
}


// TextLayout.TextRun /////////////////////////////////////////////////////////////////////////////

// Uniscribe conventions
namespace {
#if 0
	/**
	 * @internal Converts a character index into glyph index.
	 * @param clusters The logical cluster array returned by @c ScriptShape(OpenType)
	 * @param length The number of characters in the run
	 * @param numberOfGlyphs The number of glyphs in the run
	 * @param at The character index to convert
	 * @param a A @c SCRIPT_ANALYSIS provides the direction of the glyph vector
	 * @return The glyph index in the glyph vector
	 */
	inline size_t characterPositionToGlyphPosition(const WORD clusters[],
			size_t length, size_t numberOfGlyphs, size_t at, const SCRIPT_ANALYSIS& a) {
		assert(clusters != nullptr);
		assert(at <= length);
		assert(a.fLogicalOrder == 0);
		if(a.fRTL == 0)	// LTR
			return (at < length) ? clusters[at] : numberOfGlyphs;
		else	// RTL
			return (at < length) ? clusters[at] + 1 : 0;
	}
#endif
	inline bool overhangs(const ABC& width) /*noexcept*/ {
		return width.abcA < 0 || width.abcC < 0;
	}
} // namespace @0

namespace {
	// bad ideas :(
	template<typename T>
	inline void raiseIfNull(T* p, const char parameterName[]) {
		if(p == nullptr)
			throw NullPointerException(parameterName);
	}
	inline void raiseIfNullOrEmpty(const StringPiece& textString, const char parameterName[]) {
		if(textString.beginning() == nullptr)
			throw NullPointerException(parameterName + string(".beginning()"));
		else if(textString.end() == nullptr)
			throw NullPointerException(parameterName + string(".end()"));
		else if(isEmpty(textString))
			throw invalid_argument(parameterName);
	}

	template<typename Attribute>
	struct AttributedCharacterRange {
		StringPiece::const_pointer position;
		Attribute attribute;
		AttributedCharacterRange() {}
		AttributedCharacterRange(StringPiece::const_pointer position,
			const Attribute& attribute) : position(position), attribute(attribute) {}
	};

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
		unique_ptr<ElementType[]> allocated_;
		size_t capacity_;
		ElementType* p_;
	};

	class TextRunImpl : public TextRun, public StringPiece {
		ASCENSION_NONCOPYABLE_TAG(TextRunImpl);
	public:
		struct Overlay {
			Color color;
			Range<Index> range;
		};
	public:
		TextRunImpl(const StringPiece& characterRange,
			const SCRIPT_ANALYSIS& script, shared_ptr<const Font> font, OpenTypeFontTag scriptTag);
		~TextRunImpl() /*noexcept*/;
		static void generate(const StringPiece& textString, const FontCollection& fontCollection,
			const ComputedTextLineStyle& lineStyle, unique_ptr<ComputedStyledTextRunIterator> textRunStyles,
			vector<TextRunImpl*>& textRuns, vector<AttributedCharacterRange<const ComputedTextRunStyle>>& calculatedStyles);
		// GlyphVector
		void fillGlyphs(PaintContext& context, const NativePoint& origin,
			boost::optional<Range<std::size_t>> range /* = boost::none */) const;
		FlowRelativeFourSides<Scalar> glyphVisualBounds(const Range<size_t>& range) const;
		size_t numberOfGlyphs() const /*noexcept*/;
		void strokeGlyphs(PaintContext& context, const NativePoint& origin,
			boost::optional<Range<std::size_t>> range /* = boost::none */) const;
		// TextRun
		const FlowRelativeFourSides<ComputedBorderSide>* borders() const /*noexcept*/;
		boost::optional<Index> characterEncompassesPosition(Scalar ipd) const /*noexcept*/;
		Index characterHasClosestLeadingEdge(Scalar ipd) const;
		uint8_t characterLevel() const /*noexcept*/;
		shared_ptr<const Font> font() const /*noexcept*/;
		Scalar leadingEdge(Index character) const;
		Index length() const /*noexcept*/;
		Scalar trailingEdge(Index character) const;
		// attributes
		HRESULT logicalAttributes(SCRIPT_LOGATTR attributes[]) const;
		// geometry
		HRESULT logicalWidths(int widths[]) const;
		int totalWidth() const /*throw()*/ {return accumulate(advances(), advances() + numberOfGlyphs(), 0);}
		// layout
		unique_ptr<TextRunImpl> breakAt(StringPiece::const_pointer at);
		bool expandTabCharacters(const TabExpander& tabExpander,
			StringPiece::const_pointer layoutString, Scalar x, Scalar maximumMeasure);
		HRESULT justify(int width);
#if 0
		static void mergeScriptsAndStyles(const StringPiece& layoutString, const SCRIPT_ITEM scriptRuns[],
			const OPENTYPE_TAG scriptTags[], size_t numberOfScriptRuns, const FontCollection& fontCollection,
			shared_ptr<const TextRunStyle> defaultStyle, unique_ptr<ComputedStyledTextRunIterator> styles,
			vector<TextRunImpl*>& textRuns, vector<const ComputedTextRunStyle>& computedStyles,
			vector<vector<const ComputedTextRunStyle>::size_type>& computedStylesIndices);
#endif
		void shape(const win32::Handle<HDC>& dc);
		void positionGlyphs(const win32::Handle<HDC>& dc, const ComputedTextRunStyle& style);
		unique_ptr<TextRunImpl> splitIfTooLong();
		static void substituteGlyphs(const Range<vector<TextRunImpl*>::iterator>& runs);
		// drawing and painting
		void drawGlyphs(PaintContext& context, const NativePoint& p, const Range<Index>& range) const;
		void paintBackground(PaintContext& context, const NativePoint& p,
			const Range<Index>& range, NativeRectangle* paintedBounds) const;
		void paintBorder() const;
		void paintLineDecorations() const;
	private:
		// this data is shared text runs separated by (only) line breaks and computed styles
		struct RawGlyphVector /*: public StringPiece*/ {
			const StringPiece::const_pointer position;
			const shared_ptr<const Font> font;
			const OpenTypeFontTag scriptTag;	// as OPENTYPE_TAG
			mutable SCRIPT_CACHE fontCache;
			size_t numberOfGlyphs;
			// only 'clusters' is character-base. others are glyph-base
			unique_ptr<WORD[]> indices, clusters;
			unique_ptr<SCRIPT_VISATTR[]> visualAttributes;
			unique_ptr<int[]> advances, justifiedAdvances;
			unique_ptr<GOFFSET[]> offsets;
			RawGlyphVector(StringPiece::const_pointer position, shared_ptr<const Font> font,
					OpenTypeFontTag scriptTag) : position(position), font(font), scriptTag(scriptTag), fontCache(nullptr) {
				raiseIfNull(position, "position");
				raiseIfNull(font.get(), "font");
			}
			~RawGlyphVector() /*noexcept*/ {::ScriptFreeCache(&fontCache);}
			void vanish(const Font& font, StringPiece::const_pointer at);
		};
	private:
		TextRunImpl(const StringPiece& characterRange,
			const SCRIPT_ANALYSIS& script, unique_ptr<RawGlyphVector> glyphs);
		TextRunImpl(TextRunImpl& leading, StringPiece::const_pointer beginningOfNewRun);
		const int* advances() const /*noexcept*/ {
			if(const int* const p = glyphs_->advances.get())
				return p + glyphRange().beginning();
			return nullptr;
		}
		const WORD* clusters() const /*noexcept*/ {
			if(const WORD* const p = glyphs_->clusters.get())
				return p + (beginning() - glyphs_->position);
			return nullptr;
		}
		size_t countMissingGlyphs(const RenderingContext2D& context) const;
		static void generateDefaultGlyphs(const win32::Handle<HDC>& dc,
			const StringPiece& text, const SCRIPT_ANALYSIS& analysis, RawGlyphVector& glyphs);
		static HRESULT generateGlyphs(const win32::Handle<HDC>& dc,
			const StringPiece& text, const SCRIPT_ANALYSIS& analysis, RawGlyphVector& glyphs);
		const WORD* glyphs() const /*noexcept*/ {
			if(const WORD* const p = glyphs_->indices.get())
				return p + glyphRange().beginning();
			return nullptr;
		}
		const GOFFSET* glyphOffsets() const /*noexcept*/ {
			if(const GOFFSET* const p = glyphs_->offsets.get())
				return p + glyphRange().beginning();
			return nullptr;
		}
		Range<size_t> glyphRange(const StringPiece& characterRange = StringPiece(nullptr)) const;
		void hitTest(Scalar ipd, int& encompasses, int* trailing) const;
		Scalar ipd(StringPiece::const_pointer character, bool trailing) const;
		const int* justifiedAdvances() const /*noexcept*/ {
			if(const int* const p = glyphs_->justifiedAdvances.get())
				return p + glyphRange().beginning();
			return nullptr;
		}
		void paintGlyphs(PaintContext& context,
			const NativePoint& origin, const StringPiece& range, bool onlyStroke) const;
		void paintGlyphs(PaintContext& context,
			const NativePoint& origin, boost::optional<Range<size_t>> range, bool onlyStroke) const;
		const SCRIPT_VISATTR* visualAttributes() const /*noexcept*/ {
			if(const SCRIPT_VISATTR* const p = glyphs_->visualAttributes.get())
				return p + glyphRange().beginning();
			return nullptr;
		}
	private:
		SCRIPT_ANALYSIS analysis_;	// fLogicalOrder member is always 0 (however see shape())
		shared_ptr<RawGlyphVector> glyphs_;
		int width_ : sizeof(int) - 1;
		bool mayOverhang_ : 1;
	};
}

void TextRunImpl::RawGlyphVector::vanish(const Font& font, StringPiece::const_pointer at) {
	assert(advances.get() == nullptr);
	assert(at != nullptr);
	assert(at >= position);
	win32::Handle<HDC> dc(detail::screenDC());
	HFONT oldFont = nullptr;
	WORD blankGlyph;
	HRESULT hr = ::ScriptGetCMap(dc.get(), &fontCache, L"\x0020", 1, 0, &blankGlyph);
	if(hr == E_PENDING) {
		oldFont = static_cast<HFONT>(::SelectObject(dc.get(), font.asNativeObject().get()));
		hr = ::ScriptGetCMap(dc.get(), &fontCache, L"\x0020", 1, 0, &blankGlyph);
	}
	if(hr == S_OK) {
		SCRIPT_FONTPROPERTIES fp;
		fp.cBytes = sizeof(SCRIPT_FONTPROPERTIES);
		if(FAILED(hr = ::ScriptGetFontProperties(dc.get(), &fontCache, &fp)))
			fp.wgBlank = 0;	/* hmm... */
		blankGlyph = fp.wgBlank;
	}
	if(oldFont != nullptr)
		::SelectObject(dc.get(), oldFont);
	indices[clusters[at - position]] = indices[clusters[at - position + 1]] = blankGlyph;
	SCRIPT_VISATTR* const va = visualAttributes.get();
	va[clusters[at - position]].uJustification = SCRIPT_JUSTIFY_BLANK;
	va[clusters[at - position]].fZeroWidth = 1;
}

/**
 * Constructor.
 * @param characterRange The string this text run covers
 * @param script @c SCRIPT_ANALYSIS The object obtained by @c ScriptItemize(OpenType)
 * @param font The font renders this text run. Can't be @c null
 * @param scriptTag An OpenType script tag describes the script of this text run
 * @throw NullPointerException @a characterRange and/or @a font are @c null
 * @throw std#invalid_argument @a characterRange is empty
 */
TextRunImpl::TextRunImpl(const StringPiece& characterRange, const SCRIPT_ANALYSIS& script,
		shared_ptr<const Font> font, OpenTypeFontTag scriptTag)
		: StringPiece(characterRange), analysis_(script),
		glyphs_(new RawGlyphVector(characterRange.beginning(), font, scriptTag)) {	// may throw NullPointerException for 'font'
	raiseIfNullOrEmpty(characterRange, "characterRange");
//	raiseIfNull(font.get(), "font");
}

/**
 * Private constructor.
 * @param characterRange The string this text run covers
 * @param script @c SCRIPT_ANALYSIS The object obtained by @c ScriptItemize(OpenType)
 * @throw NullPointerException @a characterRange and/or @a glyphs are @c null
 * @throw std#invalid_argument @a characterRange is empty
 */
TextRunImpl::TextRunImpl(const StringPiece& characterRange, const SCRIPT_ANALYSIS& script,
		unique_ptr<RawGlyphVector> glyphs) : StringPiece(characterRange), analysis_(script), glyphs_(move(glyphs)) {
	raiseIfNullOrEmpty(characterRange, "characterRange");
	raiseIfNull(glyphs_.get(), "glyphs");
}

/**
 * Another private constructor separates an existing text run.
 * @param leading The original text run
 * @param beginningOfNewRun
 * @throw std#invalid_argument @a leading has not been shaped
 * @throw NullPointerException @a beginningOfNewRun is @c null
 * @throw std#out_of_range @a beginningOfNewRun is outside of the character range @a leading covers
 * @see #splitIfTooLong
 */
TextRunImpl::TextRunImpl(TextRunImpl& leading, StringPiece::const_pointer beginningOfNewRun) :
		StringPiece(beginningOfNewRun, leading.end()), analysis_(leading.analysis_), glyphs_(leading.glyphs_) {
	if(leading.glyphs_.get() == nullptr)
		throw invalid_argument("leading has not been shaped");
	raiseIfNull(beginningOfNewRun, "beginningOfNewRun");
	if(!includes(leading, beginningOfNewRun))
		throw out_of_range("beginningOfNewRun");

	// compute 'glyphRange_'

	// modify clusters
//	TextRun& target = ltr ? *this : leading;
//	WORD* const clusters = glyphs_->clusters.get();
//	transform(target.clusters(), target.clusters() + target.length(),
//		clusters + target.beginning(), bind2nd(minus<WORD>(), clusters[ltr ? target.beginning() : (target.end() - 1)]));
}

/// Destructor.
TextRunImpl::~TextRunImpl() /*noexcept*/ {
//	if(cache_ != nullptr)
//		::ScriptFreeCache(&cache_);
}

/**
 * Breaks the text run into two runs at the specified position.
 * @param at The position at which break this run
 * @return The new text run following this run
 */
unique_ptr<TextRunImpl> TextRunImpl::breakAt(StringPiece::const_pointer at) {
	raiseIfNull(at, "at");
	if(!includes(*this, at))
		throw out_of_range("at");
	else if(glyphs_->clusters[at - beginning()] == glyphs_->clusters[at - beginning() - 1])
		throw invalid_argument("at");

	const bool ltr = direction() == LEFT_TO_RIGHT;
	assert(ltr == (analysis_.fRTL == 0));

	// create the new following run
	unique_ptr<TextRunImpl> following(new TextRunImpl(*this, at));

	// update placements
//	place(context, layoutString, lip);
//	following->place(dc, layoutString, lip);

	return following;
}

/// @see TextRun#characterEncompassesPosition
boost::optional<Index> TextRunImpl::characterEncompassesPosition(Scalar ipd) const /*noexcept*/ {
	int character;
	hitTest(ipd, character, nullptr);
	if(character == -1 || character == ascension::length(*this))
		return boost::none;
	assert(character >= 0);
	return character;
}

/// @see TextRun#characterHasClosestLeadingEdge
Index TextRunImpl::characterHasClosestLeadingEdge(Scalar ipd) const {
	int character, trailing;
	hitTest(ipd, character, &trailing);
	if(character == -1)
		return 0;
	const int result = (character == ascension::length(*this)) ? ascension::length(*this) : (character + trailing);
	assert(result >= 0);
	return result;
}

/// @see TextRun#characterLevel
uint8_t TextRunImpl::characterLevel() const /*noexcept*/ {
	return static_cast<uint8_t>(analysis_.s.uBidiLevel);
}

/**
 * Returns the number of missing glyphs in this run.
 * @param context The graphics context
 * @return The number of missing glyphs
 * @throw PlatformError
 */
inline size_t TextRunImpl::countMissingGlyphs(const RenderingContext2D& context) const {
	SCRIPT_FONTPROPERTIES fp;
	fp.cBytes = sizeof(SCRIPT_FONTPROPERTIES);
	const HRESULT hr = ::ScriptGetFontProperties(context.asNativeObject().get(), &glyphs_->fontCache, &fp);
	if(FAILED(hr))
		throw makePlatformError(hr);	// can't handle
	// following is not offical way, but from Mozilla (gfxWindowsFonts.cpp)
	size_t c = 0;
	for(StringCharacterIterator i(*this); i.hasNext(); i.next()) {
		if(!BinaryProperty::is<BinaryProperty::DEFAULT_IGNORABLE_CODE_POINT>(i.current())) {
			const WORD glyph = glyphs_->indices[glyphs_->clusters[i.tell() - i.beginning()]];
			if(glyph == fp.wgDefault || (glyph == fp.wgInvalid && glyph != fp.wgBlank))
				++c;
			else if(glyphs_->visualAttributes[i.tell() - i.beginning()].fZeroWidth == 1
					&& scriptProperties.get(analysis_.eScript).fComplex == 0)
				++c;
		}
	}
	return c;
}

/**
 * Expands tab characters in this run and modifies the measure (advance).
 * @param tabExpander The tab expander
 * @param layoutString A pointer to the whole string of the layout this run belongs to
 * @param x The position in writing direction this run begins, in pixels
 * @param maximumMeasure The maximum measure this run can take place, in pixels
 * @return @c true if expanded tab characters
 * @throw NullPointerException @a layoutString is @c null
 * @throw std#invalid_argument @a maximumMeasure &lt;= 0
 */
inline bool TextRunImpl::expandTabCharacters(
		const TabExpander& tabExpander, StringPiece::const_pointer layoutString, Scalar x, Scalar maximumMeasure) {
	raiseIfNull(layoutString, "layoutString");
	if(maximumMeasure <= 0)
		throw invalid_argument("maximumMeasure");
	if(*beginning() != '\t')
		return false;
	assert(ascension::length(*this) == 1 && glyphs_.unique());
	glyphs_->advances[0] = min(tabExpander.nextTabStop(x, beginning() - layoutString), maximumMeasure);
	glyphs_->justifiedAdvances.reset();
	return true;
}

/// @see GlyphVector#fillGlyphs
void TextRunImpl::fillGlyphs(PaintContext& context, const NativePoint& origin, boost::optional<Range<size_t>> range /* = boost::none */) const {
	return paintGlyphs(context, origin, range, false);
}

/// @see TextRun#font
shared_ptr<const Font> TextRunImpl::font() const /*noexcept*/ {
	return glyphs_->font;
}

namespace {
	shared_ptr<const Font> selectFont(const StringPiece& textString, const FontCollection& fontCollection, const ComputedFontSpecification& specification);
}

/**
 * @param textString
 * @param fontCollection
 * @param lineStyle
 * @param textRunStyles
 * @param[out] textRuns
 * @param[out] calculatedStyles
 */
void TextRunImpl::generate(const StringPiece& textString, const FontCollection& fontCollection,
		const ComputedTextLineStyle& lineStyle, unique_ptr<ComputedStyledTextRunIterator> textRunStyles,
		vector<TextRunImpl*>& textRuns, vector<AttributedCharacterRange<const ComputedTextRunStyle>>& calculatedStyles) {
	raiseIfNullOrEmpty(textString, "textString");

	// split the text line into text runs as following steps:
	// 1. split the text into script runs (SCRIPT_ITEMs) by Uniscribe
	// 2. split each script runs into atomically-shapable runs (TextRuns) with StyledRunIterator

	// 1. split the text into script runs by Uniscribe
	HRESULT hr;

	// 1-1. configure Uniscribe's itemize
	win32::AutoZero<SCRIPT_CONTROL> control;
	win32::AutoZero<SCRIPT_STATE> initialState;
	initialState.uBidiLevel = (lineStyle.writingMode.inlineFlowDirection == RIGHT_TO_LEFT) ? 1 : 0;
//	initialState.fOverrideDirection = 1;
	initialState.fInhibitSymSwap = lineStyle.inhibitSymmetricSwapping;
	initialState.fDisplayZWG = lineStyle.displayShapingControls;
	resolveNumberSubstitution(&lineStyle.numberSubstitution, control, initialState);	// ignore result...

	// 1-2. itemize
	// note that ScriptItemize can cause a buffer overflow (see Mozilla bug 366643)
	AutoArray<SCRIPT_ITEM, 128> scriptRuns;
	AutoArray<OPENTYPE_TAG, scriptRuns.STATIC_CAPACITY> scriptTags;
	using ascension::length;
	int estimatedNumberOfScriptRuns = max(static_cast<int>(length(textString)) / 4, 2), numberOfScriptRuns;
	HRESULT(WINAPI* scriptItemizeOpenType)(const WCHAR*, int, int,
		const SCRIPT_CONTROL*, const SCRIPT_STATE*, SCRIPT_ITEM*, OPENTYPE_TAG*, int*) = uspLib->get<0>();
	while(true) {
		scriptRuns.reallocate(estimatedNumberOfScriptRuns);
		scriptTags.reallocate(estimatedNumberOfScriptRuns);
		if(scriptItemizeOpenType != nullptr)
			hr = (*scriptItemizeOpenType)(textString.beginning(), static_cast<int>(length(textString)),
				estimatedNumberOfScriptRuns, &control, &initialState, scriptRuns.get(), scriptTags.get(), &numberOfScriptRuns);
		else
			hr = ::ScriptItemize(textString.beginning(), static_cast<int>(length(textString)),
				estimatedNumberOfScriptRuns, &control, &initialState, scriptRuns.get(), &numberOfScriptRuns);
		if(hr != E_OUTOFMEMORY)	// estimatedNumberOfRuns was enough...
			break;
		estimatedNumberOfScriptRuns *= 2;
	}
	if(lineStyle.disableDeprecatedFormatCharacters) {
		for(int i = 0; i < numberOfScriptRuns; ++i) {
			scriptRuns[i].a.s.fInhibitSymSwap = initialState.fInhibitSymSwap;
			scriptRuns[i].a.s.fDigitSubstitute = initialState.fDigitSubstitute;
		}
	}
	if(scriptItemizeOpenType == nullptr)
		fill_n(scriptTags.get(), numberOfScriptRuns, SCRIPT_TAG_UNKNOWN);

	// 2. generate raw glyph vectors and computed styled text runs
	vector<unique_ptr<RawGlyphVector>> glyphRuns;
	glyphRuns.reserve(numberOfScriptRuns);
	vector<const SCRIPT_ANALYSIS*> scriptPointers;
	scriptPointers.reserve(numberOfScriptRuns);
	vector<AttributedCharacterRange<const ComputedTextRunStyle>> styleRuns;
	{
		StringPiece::const_pointer lastGlyphRunEnd = nullptr;
		// script cursors
		AttributedCharacterRange<const SCRIPT_ITEM*>
			scriptRun(textString.beginning() + scriptRuns[0].iCharPos, &scriptRuns[0]),
			nextScriptRun((numberOfScriptRuns > 1) ?
				textString.beginning() + scriptRuns[1].iCharPos : textString.end(), scriptRun.attribute + 1);
		// style cursors
		detail::ComputedStyledTextRunEnumerator styledTextRunEnumerator(textString, move(textRunStyles));
		assert(!styledTextRunEnumerator.isDone());
		AttributedCharacterRange<ComputedTextRunStyle> styleRun, nextStyleRun;
		styledTextRunEnumerator.style(styleRun.attribute);
		styleRun.position = styledTextRunEnumerator.position();
		styledTextRunEnumerator.next();
		if(!styledTextRunEnumerator.isDone()) {
			styledTextRunEnumerator.style(nextStyleRun.attribute);
			nextStyleRun.position = styledTextRunEnumerator.position();
		} else
			nextStyleRun.position = textString.end();
		styleRuns.push_back(AttributedCharacterRange<const ComputedTextRunStyle>(styleRun.position, styleRun.attribute));

		do {
			const StringPiece::const_pointer next = min(nextScriptRun.position, nextStyleRun.position);
			const bool advanceScriptRun = next == nextScriptRun.position;
			const bool advanceStyleRun = next == nextStyleRun.position;

			if(advanceScriptRun) {
				const StringPiece subRange(scriptRun.position, next);
				assert(glyphRuns.empty() || subRange.beginning() == lastGlyphRunEnd);
				glyphRuns.push_back(
					unique_ptr<RawGlyphVector>(
						new RawGlyphVector(subRange.beginning(),
							selectFont(subRange, fontCollection, styleRun.attribute.font),
							scriptTags[scriptRun.attribute - scriptRuns.get()])));
				scriptPointers.push_back(&scriptRuns[scriptRun.attribute - scriptRuns.get()].a);
				assert(nextScriptRun.position < textString.end());
				scriptRun = nextScriptRun;
				if(++nextScriptRun.attribute < scriptRuns.get() + numberOfScriptRuns)
					nextScriptRun.position = textString.beginning() + nextScriptRun.attribute->iCharPos;
				else
					nextScriptRun.position = textString.end();
			}
			if(advanceStyleRun) {
				if(!advanceScriptRun) {
					const StringPiece subRange(!glyphRuns.empty() ? lastGlyphRunEnd : textString.beginning(), next);
					glyphRuns.push_back(
						unique_ptr<RawGlyphVector>(
							new RawGlyphVector(subRange.beginning(),
								selectFont(subRange, fontCollection, styleRun.attribute.font),
								scriptTags[scriptRun.attribute - scriptRuns.get()])));
				}
				assert(nextStyleRun.position < textString.end());
				styleRun = move(nextStyleRun);
				styleRuns.push_back(AttributedCharacterRange<const ComputedTextRunStyle>(styleRun.position, styleRun.attribute));
				assert(!styledTextRunEnumerator.isDone());
				styledTextRunEnumerator.next();
				if(!styledTextRunEnumerator.isDone()) {
					styledTextRunEnumerator.style(nextStyleRun.attribute);
					nextStyleRun.position = styledTextRunEnumerator.position();
				} else
					nextStyleRun.position = textString.end();
			}
			lastGlyphRunEnd = next;
		} while(scriptRun.position < textString.end() || styleRun.position < textString.end());
		assert(glyphRuns.size() == scriptPointers.size());
	}

	// 3. merge script runs and style runs into TextRunImpls
	vector<TextRunImpl*> mergedTextRuns;
	mergedTextRuns.reserve(glyphRuns.size() + styleRuns.size());
	{
		using std::end;
		auto glyphRun(begin(glyphRuns)), lastGlyphRun(end(glyphRuns));
		auto styleRun(begin(styleRuns)), lastStyleRun(end(styleRuns));
		do {
			auto nextGlyphRun(glyphRun + 1);
			auto nextStyleRun(styleRun + 1);
			const StringPiece::const_pointer
				nextGlyphRunPosition = (nextGlyphRun != lastGlyphRun) ? (*nextGlyphRun)->position : textString.end(),
				nextStyleRunPosition = (nextStyleRun != lastStyleRun) ? nextStyleRun->position : textString.end();
			const StringPiece::const_pointer nextPosition = min(nextGlyphRunPosition, nextStyleRunPosition);
			const StringPiece::const_pointer previousPosition =
				!mergedTextRuns.empty() ? mergedTextRuns.back()->end() : textString.beginning();

			mergedTextRuns.push_back(new TextRunImpl(
				StringPiece(previousPosition, nextPosition),
				*scriptPointers[glyphRuns.size() - (lastGlyphRun - glyphRun)], move(*glyphRun)));
			if(nextPosition == nextGlyphRunPosition)
				++glyphRun;
			if(nextPosition == nextStyleRunPosition)
				++styleRun;
		} while(glyphRun != lastGlyphRun && styleRun != lastStyleRun);
	}

	// 4. generate results
	using std::swap;
	swap(mergedTextRuns, textRuns);
	swap(styleRuns, calculatedStyles);
}

/// Fills the glyph array with default index, instead of using @c ScriptShape.
inline void TextRunImpl::generateDefaultGlyphs(const win32::Handle<HDC>& dc,
		const StringPiece& text, const SCRIPT_ANALYSIS& analysis, RawGlyphVector& glyphs) {
	SCRIPT_CACHE fontCache(nullptr);
	SCRIPT_FONTPROPERTIES fp;
	fp.cBytes = sizeof(SCRIPT_FONTPROPERTIES);
	if(FAILED(::ScriptGetFontProperties(dc.get(), &fontCache, &fp)))
		fp.wgDefault = 0;	// hmm...

	unique_ptr<WORD[]> indices, clusters;
	unique_ptr<SCRIPT_VISATTR[]> visualAttributes;
	using ascension::length;
	const int numberOfGlyphs = static_cast<int>(length(text));
	indices.reset(new WORD[numberOfGlyphs]);
	clusters.reset(new WORD[length(text)]);
	visualAttributes.reset(new SCRIPT_VISATTR[numberOfGlyphs]);
	fill_n(indices.get(), numberOfGlyphs, fp.wgDefault);
	const bool ltr = analysis.fRTL == 0 || analysis.fLogicalOrder == 1;
	for(size_t i = 0, c = length(text); i < c; ++i)
		clusters[i] = static_cast<WORD>(ltr ? i : (c - i));
	const SCRIPT_VISATTR va = {SCRIPT_JUSTIFY_NONE, 1, 0, 0, 0, 0};
	fill_n(visualAttributes.get(), numberOfGlyphs, va);

	// commit
	glyphs.numberOfGlyphs = numberOfGlyphs;
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
 * @retval S_OK succeeded
 * @retval USP_E_SCRIPT_NOT_IN_FONT the font does not support the required script
 * @retval E_INVALIDARG other Uniscribe error. usually, too long run was specified
 * @retval HRESULT other Uniscribe error
 * @throw std#bad_alloc failed to allocate buffer for glyph indices or visual attributes array
 */
HRESULT TextRunImpl::generateGlyphs(const win32::Handle<HDC>& dc,
		const StringPiece& text, const SCRIPT_ANALYSIS& analysis, RawGlyphVector& glyphs) {
#ifdef _DEBUG
	if(HFONT currentFont = static_cast<HFONT>(::GetCurrentObject(dc.get(), OBJ_FONT))) {
		LOGFONTW lf;
		if(::GetObjectW(currentFont, sizeof(LOGFONTW), &lf) > 0) {
			win32::DumpContext dout;
			dout << L"[TextLayout.TextRun.generateGlyphs] Selected font is '" << lf.lfFaceName << L"'.\n";
		}
	}
#endif

	SCRIPT_CACHE fontCache(nullptr);	// TODO: this object should belong to a font, not glyph run???
	unique_ptr<WORD[]> indices, clusters;
	unique_ptr<SCRIPT_VISATTR[]> visualAttributes;
	using ascension::length;
	clusters.reset(new WORD[length(text)]);
	int numberOfGlyphs = estimateNumberOfGlyphs(length(text));
	HRESULT hr;
	while(true) {
		indices.reset(new WORD[numberOfGlyphs]);
		visualAttributes.reset(new SCRIPT_VISATTR[numberOfGlyphs]);
		hr = ::ScriptShape(dc.get(), &fontCache,
			text.beginning(), static_cast<int>(length(text)),
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
		glyphs.numberOfGlyphs = numberOfGlyphs;
		using std::swap;
		swap(glyphs.fontCache, fontCache);
		swap(glyphs.indices, indices);
		swap(glyphs.clusters, clusters);
		swap(glyphs.visualAttributes, visualAttributes);
	}
	::ScriptFreeCache(&fontCache);
	return hr;
}

inline Range<size_t> TextRunImpl::glyphRange(const StringPiece& range /* = StringPiece(nullptr) */) const {
	assert(glyphs_.get() != nullptr);
	assert(analysis_.fLogicalOrder == 0);
	using ascension::length;
	Range<ptrdiff_t> characterRange((range != StringPiece(nullptr)) ?
		makeRange(range.beginning() - beginning(), range.end() - beginning()) : makeRange(0, length(*this)));
	assert(includes(makeRange<ptrdiff_t>(0, length(*this)), characterRange));
	assert(characterRange.beginning() == 0 || characterRange.beginning() == length(*this)
		|| glyphs_->clusters[characterRange.beginning()] != glyphs_->clusters[characterRange.beginning() - 1]);
	assert(characterRange.end() == 0 || characterRange.end() == length(*this)
		|| glyphs_->clusters[characterRange.end()] != glyphs_->clusters[characterRange.end() + 1]);

	if(analysis_.fRTL == 0)	// LTR
		return makeRange(
			(range.beginning() < end()) ? glyphs_->clusters[range.beginning() - beginning()] : glyphs_->numberOfGlyphs,
			(range.end() < end()) ? glyphs_->clusters[range.end() - beginning() + 1] : glyphs_->numberOfGlyphs);
	else					// RTL
		return makeRange(
			(range.end() > beginning()) ? glyphs_->clusters[range.end() - beginning() - 1] : glyphs_->numberOfGlyphs,
			(range.beginning() > beginning()) ? glyphs_->clusters[range.beginning() - beginning() - 1] : glyphs_->numberOfGlyphs
		);
}

/// @see GlyphVector#glyphVisualBounds
FlowRelativeFourSides<Scalar> TextRunImpl::glyphVisualBounds(const Range<size_t>& range) const {
	FlowRelativeFourSides<Scalar> bounds(glyphLogicalBounds(range));
	if(isEmpty(range))
		return bounds;

	ABC glyphMeasure;
	win32::Handle<HDC> dc;
	HFONT oldFont = nullptr;
	HRESULT hr;
	for(FlowRelativeFourSides<Scalar>::iterator i(begin(bounds)), e(std::end(bounds)); i != e; ++i) {
		FlowRelativeDirection d = static_cast<FlowRelativeDirection>(i - begin(bounds));
		if(d != START && d != END)
			continue;
		hr = ::ScriptGetGlyphABCWidth(dc.get(), &glyphs_->fontCache, glyphs_->indices[range.beginning()], &glyphMeasure);
		if(hr == E_PENDING) {
			dc = detail::screenDC();
			oldFont = static_cast<HFONT>(::SelectObject(dc.get(), glyphs_->font->asNativeObject().get()));
			hr = ::ScriptGetGlyphABCWidth(dc.get(), &glyphs_->fontCache, glyphs_->indices[range.beginning()], &glyphMeasure);
		}
		if(FAILED(hr)) {
			if(oldFont != nullptr)
				::SelectObject(dc.get(), oldFont);
			throw makePlatformError(hr);
		}
		if(d == START)
			bounds.start() += (direction() == LEFT_TO_RIGHT) ? glyphMeasure.abcA : glyphMeasure.abcC;
		else
			bounds.end() -= (direction() == LEFT_TO_RIGHT) ? glyphMeasure.abcC : glyphMeasure.abcA;
	}
	if(oldFont != nullptr)
		::SelectObject(dc.get(), oldFont);
	return bounds;
}

inline void TextRunImpl::hitTest(Scalar ipd, int& encompasses, int* trailing) const {
	int tr;
	const int x = (direction() == LEFT_TO_RIGHT) ? ipd : (measure() - ipd);
	const HRESULT hr = ::ScriptXtoCP(x, static_cast<int>(ascension::length(*this)), numberOfGlyphs(), clusters(),
		visualAttributes(), (justifiedAdvances() == nullptr) ? advances() : justifiedAdvances(), &analysis_, &encompasses, &tr);
	if(FAILED(hr))
		throw makePlatformError(hr);
	if(trailing != nullptr)
		*trailing = encompasses + tr;
}

inline Scalar TextRunImpl::ipd(StringPiece::const_pointer character, bool trailing) const {
	raiseIfNull(character, "character");
	if(character < beginning() || character > end())
		throw out_of_range("character");
	int result;
	const HRESULT hr = ::ScriptCPtoX(static_cast<int>(character - beginning()), trailing,
		static_cast<int>(ascension::length(*this)), numberOfGlyphs(), clusters(), visualAttributes(),
		((justifiedAdvances() == nullptr) ? advances() : justifiedAdvances()), &analysis_, &result);
	if(FAILED(hr))
		throw makePlatformError(hr);
	// TODO: handle letter-spacing correctly.
//	if(visualAttributes()[offset].fClusterStart == 0) {
//	}
	return (direction() == LEFT_TO_RIGHT) ? result : (measure() - result);
}

inline HRESULT TextRunImpl::justify(int width) {
	assert(glyphs_->indices.get() != nullptr && advances() != nullptr);
	HRESULT hr = S_OK;
	if(width != totalWidth()) {
		if(glyphs_->justifiedAdvances.get() == nullptr)
			glyphs_->justifiedAdvances.reset(new int[numberOfGlyphs()]);
		hr = ::ScriptJustify(visualAttributes(), advances(), numberOfGlyphs(), width - totalWidth(),
			2, glyphs_->justifiedAdvances.get() + (beginning() - glyphs_->position));
	}
	return hr;
}

/// @see TextRun#leadingEdge
Scalar TextRunImpl::leadingEdge(Index character) const {
	return ipd(beginning() + character, false);	// TODO: this ignores HRESULT.
}

inline HRESULT TextRunImpl::logicalAttributes(SCRIPT_LOGATTR attributes[]) const {
	raiseIfNull(attributes, "attributes");
	return ::ScriptBreak(beginning(), static_cast<int>(ascension::length(*this)), &analysis_, attributes);
}

inline HRESULT TextRunImpl::logicalWidths(int widths[]) const {
	raiseIfNull(widths, "widths");
	return ::ScriptGetLogicalWidths(&analysis_, static_cast<int>(ascension::length(*this)),
		numberOfGlyphs(), advances(), clusters(), visualAttributes(), widths);
}

#if 0
namespace {
	pair<StringPiece::const_pointer, shared_ptr<const Font>> findNextFontRun(
		const StringPiece& textString, const FontCollection& fontCollection,
		const ComputedTextRunStyle& style, shared_ptr<const Font> previousFont);
} // namespace @0

/**
 * Merges the given item runs and the given style runs.
 * @param layoutString
 * @param items The items itemized by @c #itemize()
 * @param numberOfItems The length of the array @a items
 * @param styles The iterator returns the styled runs in the line. Can be @c null
 * @param[out] textRuns
 * @param[out] computedStyles
 * @see presentation#Presentation#getLineStyle
 */
void TextRunImpl::mergeScriptsAndStyles(
		const StringPiece& layoutString, const SCRIPT_ITEM scriptRuns[],
		const OPENTYPE_TAG scriptTags[], size_t numberOfScriptRuns,
		const FontCollection& fontCollection, shared_ptr<const TextRunStyle> defaultStyle,
		unique_ptr<ComputedStyledTextRunIterator> styles, vector<TextRunImpl*>& textRuns,
		vector<const ComputedTextRunStyle>& computedStyles,
		vector<vector<const ComputedTextRunStyle>::size_type>& computedStylesIndices) {
	raiseIfNullOrEmpty(layoutString, "layoutString");
	if(scriptRuns == nullptr)
		throw NullPointerException("scriptRuns");
	else if(numberOfScriptRuns == 0)
		throw invalid_argument("numberOfScriptRuns");

#define ASCENSION_SPLIT_LAST_RUN()												\
	while(runs.back()->length() > MAXIMUM_RUN_LENGTH) {							\
		TextRunImpl& back = *runs.back();										\
		unique_ptr<TextRunImpl> piece(new SimpleRun(back.style));				\
		Index pieceLength = MAXIMUM_RUN_LENGTH;									\
		if(surrogates::isLowSurrogate(line[back.offsetInLine + pieceLength]))	\
			--pieceLength;														\
		piece->analysis = back.analysis;										\
		piece->offsetInLine = back.offsetInLine + pieceLength;					\
		piece->setLength(back.length() - pieceLength);							\
		back.setLength(pieceLength);											\
		runs.push_back(piece.release());										\
	}

	// result buffers
	vector<TextRunImpl*> calculatedRuns;
	vector<const ComputedTextRunStyle> calculatedStyles;
	calculatedRuns.reserve(static_cast<size_t>(numberOfScriptRuns * ((styles.get() != nullptr) ? 1.2 : 1)));	// hmm...
	vector<vector<const ComputedTextRunStyle>::size_type> calculatedStylesIndices;
	calculatedStylesIndices.reserve(calculatedRuns.capacity());

	// script cursors
	AttributedCharacterRange<const SCRIPT_ITEM*> scriptRun;
	scriptRun.attribute = scriptRuns;
	scriptRun.position = layoutString.beginning() + scriptRun.attribute->iCharPos;
	AttributedCharacterRange<const SCRIPT_ITEM*> nextScriptRun;
	nextScriptRun.attribute = scriptRuns + 1;
	nextScriptRun.position = (nextScriptRun.attribute < scriptRuns + numberOfScriptRuns) ?
		layoutString.beginning() + nextScriptRun.attribute->iCharPos : layoutString.end();

	// style cursors
	detail::ComputedStyledTextRunEnumerator styleEnumerator(layoutString, move(styles));
	AttributedCharacterRange<ComputedTextRunStyle> styleRun;
	assert(!styleEnumerator.isDone());
	styleRun.position = styleEnumerator.position();
	styleEnumerator.style(styleRun.attribute);
	styleEnumerator.next();
	calculatedStyles.push_back(styleRun.attribute);
	AttributedCharacterRange<ComputedTextRunStyle> nextStyleRun;
	if(!styleEnumerator.isDone()) {
		nextStyleRun.position = styleEnumerator.position();
		styleEnumerator.style(nextStyleRun.attribute);
	} else
		nextStyleRun.position = layoutString.end();

	assert(scriptRun.position == layoutString.beginning());
	assert(styleRun.position == layoutString.beginning());

	shared_ptr<const Font> font;	// font for current glyph run
	do {
		const StringPiece::const_pointer previousRunEnd = max(scriptRun.position, styleRun.position);
		assert(
			(previousRunEnd == layoutString.beginning() && calculatedRuns.empty() && calculatedStyles.empty())
			|| (!calculatedRuns.empty() && previousRunEnd == calculatedRuns.back()->end())
			|| (!calculatedStyles.empty() && previousRunEnd == styleRun.position));
		StringPiece::const_pointer newRunEnd;
		bool forwardScriptRun = false, forwardStyleRun = false, forwardGlyphRun = false;

		if(nextScriptRun.position == nextStyleRun.position) {
			newRunEnd = nextScriptRun.position;
			forwardScriptRun = forwardStyleRun = true;
		} else if(nextScriptRun.position < nextStyleRun.position) {
			newRunEnd = nextScriptRun.position;
			forwardScriptRun = true;
		} else {	// nextScriptRun.position > nextStyleRun.position
			newRunEnd = nextStyleRun.position;
			forwardStyleRun = true;
		}

		if((++utf::makeCharacterDecodeIterator(previousRunEnd, newRunEnd)).tell() < newRunEnd || font.get() == nullptr) {
			const pair<StringPiece::const_pointer, shared_ptr<const Font>> nextFontRun(
				findNextFontRun(StringPiece(previousRunEnd, newRunEnd), fontCollection,
					(styleRun.position != nullptr) ? styleRun.attribute : ComputedTextRunStyle(), font));
			font = nextFontRun.second;
			assert(font.get() != nullptr);
			if(nextFontRun.first != nullptr) {
				forwardGlyphRun = true;
				newRunEnd = nextFontRun.first;
				forwardScriptRun = forwardStyleRun = false;
			}
		}
		if(!forwardGlyphRun && forwardScriptRun)
			forwardGlyphRun = true;

		if(forwardGlyphRun) {
			const bool breakScriptRun = newRunEnd < nextScriptRun.position;
			if(breakScriptRun)
				const_cast<SCRIPT_ITEM*>(scriptRun.attribute)->a.fLinkAfter = 0;
			calculatedRuns.push_back(
				new TextRunImpl(Range<Index>(!calculatedRuns.empty() ? calculatedRuns.back()->end() : 0, newRunEnd - layoutString.beginning()),
					scriptRun.attribute->a, font,
					(scriptTags != nullptr) ? scriptTags[scriptRun.attribute - scriptRuns] : SCRIPT_TAG_UNKNOWN));	// TODO: 'DFLT' is preferred?
			calculatedStylesIndices.push_back(calculatedStyles.size());
			while(true) {
				unique_ptr<TextRunImpl> piece(calculatedRuns.back()->splitIfTooLong());
				if(piece.get() == nullptr)
					break;
				calculatedRuns.push_back(piece.release());
				calculatedStylesIndices.push_back(calculatedStyles.size());
			}
			if(breakScriptRun)
				const_cast<SCRIPT_ITEM*>(scriptRun.attribute)->a.fLinkBefore = 0;
		}
		if(forwardScriptRun) {
			assert(nextScriptRun.position < layoutString.end());
			scriptRun = nextScriptRun;
			nextScriptRun.position =
				(++nextScriptRun.attribute < scriptRuns + numberOfScriptRuns) ?
					layoutString.beginning() + nextScriptRun.attribute->iCharPos : layoutString.end();
		}
		if(forwardStyleRun) {
			assert(nextStyleRun.position < layoutString.end());
			styleRun = move(nextStyleRun);
			calculatedStyles.push_back(styleRun.attribute);
			assert(!styleEnumerator.isDone());
			styleEnumerator.next();
			if(!styleEnumerator.isDone()) {
				nextStyleRun.position = styleEnumerator.position();
				styleEnumerator.style(nextStyleRun.attribute);
			} else
				nextStyleRun.position = layoutString.end();
		}
	} while(scriptRun.position < layoutString.end() || styleRun.position < layoutString.end());

	assert(calculatedRuns.size() == calculatedStylesIndices.size());
	assert(!calculatedStyles.empty());

	// commit
	using std::swap;
	swap(textRuns, calculatedRuns);
	swap(computedStyles, calculatedStyles);
	swap(computedStylesIndices, calculatedStylesIndices);

#undef ASCENSION_SPLIT_LAST_RUN
}
#endif // 0

/// @see GlyphVector#numberOfGlyphs
size_t TextRunImpl::numberOfGlyphs() const /*noexcept*/ {
	return ascension::length(glyphRange());
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
void TextRunImpl::paintBackground(PaintContext& context,
		const NativePoint& p, const Range<Index>& range, NativeRectangle* paintedBounds) const {
	if(ascension::isEmpty(range) || geometry::x(p) + totalWidth() < geometry::left(context.boundsToPaint()))
		return;
#if 1
	extern const WritingMode& wm;
	PhysicalFourSides<Scalar> sides;
	mapFlowRelativeToPhysical(wm, glyphLogicalBounds(range), sides);
	NativeRectangle bounds(geometry::make<NativeRectangle>(sides));
	bounds = geometry::translate(bounds, p);
#else
	blackBoxBounds(range, bounds);
#endif
	context.fillRectangle(bounds);
	if(paintedBounds != nullptr)
		*paintedBounds = bounds;
}

/**
 * @internal Fills or strokes the glyphs of the specified range in this run.
 * This method uses the stroke and fill styles which are set in @a context.
 * @param context The graphics context
 * @param origin The base point of this run (, does not corresponds to @c range-&gt;beginning())
 * @param range The character range to paint. If this is @c boost#none, the all characters are painted
 * @param onlyStroke If @c true, this method only strokes the glyphs without filling
 */
inline void TextRunImpl::paintGlyphs(PaintContext& context, const NativePoint& origin, const StringPiece& range, bool onlyStroke) const {
	return paintGlyphs(context, origin, glyphRange(range), onlyStroke);
}

/**
 * @internal Fills or strokes the glyphs of the specified range in this run.
 * This method uses the stroke and fill styles which are set in @a context.
 * @param context The graphics context
 * @param origin The base point of this run (, does not corresponds to @c range-&gt;beginning())
 * @param range The glyph range to paint. If this is @c boost#none, the all glyphs are painted
 * @param onlyStroke If @c true, this method only strokes the glyphs without filling
 */
void TextRunImpl::paintGlyphs(PaintContext& context, const NativePoint& origin, boost::optional<Range<size_t>> range, bool onlyStroke) const {
	if(!range)
		return paintGlyphs(context, origin, *this, onlyStroke);
	else if(isEmpty(*range))
		return;

	context.setFont(glyphs_->font);
//	RECT temp;
//	if(dirtyRect != nullptr)
//		::SetRect(&temp, dirtyRect->left(), dirtyRect->top(), dirtyRect->right(), dirtyRect->bottom());
	if(onlyStroke && !win32::boole(::BeginPath(context.asNativeObject().get())))
		throw makePlatformError();
	assert(analysis_.fLogicalOrder == 0);
	const HRESULT hr = ::ScriptTextOut(context.asNativeObject().get(), &glyphs_->fontCache,
		geometry::x(origin) + (analysis_.fRTL == 0) ?
			leadingEdge(range->beginning()) : (measure() - leadingEdge(range->end())),
		geometry::y(origin) - glyphs_->font->metrics()->ascent(), 0, &context.boundsToPaint(), &analysis_, nullptr, 0,
		glyphs() + range->beginning(), ascension::length(*range), advances() + range->beginning(),
		(justifiedAdvances() != nullptr) ? justifiedAdvances() + range->beginning() : nullptr,
			glyphOffsets() + range->beginning());
	if(onlyStroke)
		::EndPath(context.asNativeObject().get());
	if(FAILED(hr))
		throw makePlatformError(hr);
	if(onlyStroke && !win32::boole(::StrokePath(context.asNativeObject().get())))
		throw makePlatformError();
}

/**
 * Positions the glyphs in the text run.
 * @param dc The device context
 * @param style The computed text run style
 * @see #generate, #substituteGlyphs
 */
void TextRunImpl::positionGlyphs(const win32::Handle<HDC>& dc, const ComputedTextRunStyle& style) {
	assert(glyphs_.get() != nullptr && glyphs_.unique());
	assert(glyphs_->indices.get() != nullptr && glyphs_->advances.get() == nullptr);

	unique_ptr<int[]> advances(new int[numberOfGlyphs()]);
	unique_ptr<GOFFSET[]> offsets(new GOFFSET[numberOfGlyphs()]);
//	ABC width;
	HRESULT hr = ::ScriptPlace(nullptr, &glyphs_->fontCache, glyphs_->indices.get(), numberOfGlyphs(),
		glyphs_->visualAttributes.get(), &analysis_, advances.get(), offsets.get(), nullptr/*&width*/);
	if(hr == E_PENDING) {
		HFONT oldFont = static_cast<HFONT>(::SelectObject(dc.get(), glyphs_->font->asNativeObject().get()));
		hr = ::ScriptPlace(dc.get(), &glyphs_->fontCache, glyphs_->indices.get(), numberOfGlyphs(),
			glyphs_->visualAttributes.get(), &analysis_, advances.get(), offsets.get(), nullptr/*&width*/);
		::SelectObject(dc.get(), oldFont);
	}
	if(FAILED(hr))
		throw hr;

	// apply text run styles
/*
	// query widths of C0 and C1 controls in this run
	unique_ptr<WORD[]> glyphIndices;
	if(ISpecialCharacterRenderer* scr = lip.specialCharacterRenderer()) {
		ISpecialCharacterRenderer::LayoutContext context(dc);
		context.readingDirection = readingDirection();
		dc.selectObject(glyphs_->font->handle().get());
		SCRIPT_FONTPROPERTIES fp;
		fp.cBytes = 0;
		for(Index i = beginning(); i < end(); ++i) {
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
					if(glyphIndices.get() == nullptr) {
						glyphIndices.reset(new WORD[numberOfGlyphs()]);
						memcpy(glyphIndices.get(), glyphs(), sizeof(WORD) * numberOfGlyphs());
					}
					glyphIndices[i] = fp.wgBlank;
				}
			}
		}
	}
*/
/*	// handle letter spacing
	if(styledRange.style.get() != nullptr && styledRange.style->letterSpacing.unit != Length::INHERIT) {
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
*/

	// commit
	glyphs_->advances = move(advances);
	glyphs_->offsets = move(offsets);
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

void TextRunImpl::shape(const win32::Handle<HDC>& dc) {
	assert(glyphs_.unique());

	// TODO: check if the requested style (or the default one) disables shaping.

	RawGlyphVector glyphs(*glyphs_);
	HFONT oldFont = static_cast<HFONT>(::SelectObject(dc.get(), glyphs_->font->asNativeObject().get()));
	int numberOfGlyphs;
	HRESULT hr = generateGlyphs(dc, *this, analysis_, glyphs);
	if(hr == USP_E_SCRIPT_NOT_IN_FONT) {
		analysis_.eScript = SCRIPT_UNDEFINED;
		hr = generateGlyphs(dc, *this, analysis_, glyphs);
	}
	if(FAILED(hr))
		generateDefaultGlyphs(dc, *this, analysis_, glyphs);
	::SelectObject(dc.get(), oldFont);

	// commit
	using std::swap;
	swap(*glyphs_, glyphs);
}
#if 0
void TextRunImpl::shape(DC& dc, const String& layoutString, const ILayoutInformationProvider& lip, TextRun* nextRun) {
	if(glyphs_->clusters.get() != nullptr)
		throw IllegalStateException("");
	if(requestedStyle().get() != nullptr) {
		if(!requestedStyle()->shapingEnabled)
			analysis_.eScript = SCRIPT_UNDEFINED;
	} else {
		shared_ptr<const RunStyle> defaultStyle(lip.presentation().defaultTextRunStyle());
		if(defaultStyle.get() != nullptr && !defaultStyle->shapingEnabled)
			analysis_.eScript = SCRIPT_UNDEFINED;
	}

	HRESULT hr;
	const WORD originalScript = analysis_.eScript;
	HFONT oldFont;

	// compute font properties
	String computedFontFamily((requestedStyle().get() != nullptr) ?
		requestedStyle()->fontFamily : String(L"\x5c0f\x585a\x660e\x671d Pr6N R"));
	FontProperties computedFontProperties((requestedStyle().get() != nullptr) ? requestedStyle()->fontProperties : FontProperties());
	double computedFontSizeAdjust = (requestedStyle().get() != nullptr) ? requestedStyle()->fontSizeAdjust : -1.0;
	shared_ptr<const RunStyle> defaultStyle(lip.presentation().defaultTextRunStyle());
	if(computedFontFamily.empty()) {
		if(defaultStyle.get() != nullptr)
			computedFontFamily = lip.presentation().defaultTextRunStyle()->fontFamily;
		if(computedFontFamily.empty())
			computedFontFamily = lip.textMetrics().familyName();
	}
	if(computedFontProperties.weight == FontProperties::INHERIT_WEIGHT)
		computedFontProperties.weight = (defaultStyle.get() != nullptr) ? defaultStyle->fontProperties.weight : FontProperties::NORMAL_WEIGHT;
	if(computedFontProperties.stretch == FontProperties::INHERIT_STRETCH)
		computedFontProperties.stretch = (defaultStyle.get() != nullptr) ? defaultStyle->fontProperties.stretch : FontProperties::NORMAL_STRETCH;
	if(computedFontProperties.style == FontProperties::INHERIT_STYLE)
		computedFontProperties.style = (defaultStyle.get() != nullptr) ? defaultStyle->fontProperties.style : FontProperties::NORMAL_STYLE;
	if(computedFontProperties.size == 0.0f) {
		if(defaultStyle.get() != nullptr)
			computedFontProperties.size = defaultStyle->fontProperties.size;
		if(computedFontProperties.size == 0.0f)
			computedFontProperties.size = lip.textMetrics().emHeight();
	}
	if(computedFontSizeAdjust < 0.0)
		computedFontSizeAdjust = (defaultStyle.get() != nullptr) ? defaultStyle->fontSizeAdjust : 0.0;

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
		shared_ptr<const Font> font(lip.fontCollection().get(L"Arial", fp, computedFontSizeAdjust));
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
		typedef vector<pair<shared_ptr<const Font>, int>> FailedFonts;
		FailedFonts failedFonts;	// failed fonts (font handle vs. # of missings)
		int numberOfMissingGlyphs;

		const Char* textString = layoutString.data() + beginning();

#define ASCENSION_MAKE_TEXT_STRING_SAFE()												\
	assert(safeString.get() == nullptr);												\
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
		unique_ptr<Char[]> safeString;
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
//				if(font_ == nullptr && previousRun != nullptr) {
//					// use the previous run setting (but this will copy the style of the font...)
//					analysis_.eScript = previousRun->analysis_.eScript;
//					font_ = previousRun->font_;
//				}
			}
			if(font_ != nullptr) {
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
		if(nextRun != nullptr && nextRun->length() > 1) {
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

unique_ptr<TextRunImpl> TextRunImpl::splitIfTooLong() {
	using ascension::length;
	if(estimateNumberOfGlyphs(length(*this)) <= 65535)
		return unique_ptr<TextRun>();

	// split this run, because the length would cause ScriptShape to fail (see also Mozilla bug 366643).
	static const Index MAXIMUM_RUN_LENGTH = 43680;	// estimateNumberOfGlyphs(43680) == 65536
	Index opportunity = 0;
	unique_ptr<SCRIPT_LOGATTR[]> la(new SCRIPT_LOGATTR[length(*this)]);
	const HRESULT hr = logicalAttributes(la.get());
	if(SUCCEEDED(hr)) {
		for(Index i = MAXIMUM_RUN_LENGTH; i > 0; --i) {
			if(la[i].fCharStop != 0) {
				if(legacyctype::isspace(beginning()[i]) || legacyctype::isspace(beginning()[i - 1])) {
					opportunity = i;
					break;
				}
				opportunity = max(i, opportunity);
			}
		}
	}
	if(opportunity == 0) {
		opportunity = MAXIMUM_RUN_LENGTH;
		if(surrogates::isLowSurrogate(beginning()[opportunity]) && surrogates::isHighSurrogate(beginning()[opportunity - 1]))
			--opportunity;
	}

	unique_ptr<TextRunImpl> following(new TextRunImpl(
		StringPiece(beginning() + opportunity, end()), analysis_, glyphs_->font, glyphs_->scriptTag));
	static_cast<StringPiece&>(*this) = StringPiece(beginning(), beginning() + opportunity);
	analysis_.fLinkAfter = following->analysis_.fLinkBefore = 0;
	return following;
}

/// @see GlyphVector#strokeGlyphs
void TextRunImpl::strokeGlyphs(PaintContext& context, const NativePoint& origin, boost::optional<Range<size_t>> range /* = boost::none */) const {
	return paintGlyphs(context, origin, range, true);
}

/**
 * 
 * @param runs the minimal runs
 * @param layoutString the whole string of the layout
 * @see #merge, #positionGlyphs
 */
void TextRunImpl::substituteGlyphs(const Range<vector<TextRunImpl*>::iterator>& runs) {
	// this method processes the following substitutions:
	// 1. missing glyphs
	// 2. ideographic variation sequences (if Uniscribe did not support)

	// 1. Presentative glyphs for missing ones

	// TODO: generate missing glyphs.

	// 2. Ideographic Variation Sequences (Uniscribe workaround)
	// Older Uniscribe (version < 1.626.7100.0) does not support IVS.

#ifdef ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
	if(!uniscribeSupportsIVS()) {
		for(auto i(runs.beginning()); i != runs.end(); ++i) {
			TextRunImpl& run = **i;

			// process IVSes in a glyph run
			using ascension::length;
			if(run.analysis_.eScript != SCRIPT_UNDEFINED && length(run) > 3
					&& surrogates::isHighSurrogate(run.beginning()[0]) && surrogates::isLowSurrogate(run.beginning()[1])) {
				for(StringCharacterIterator i(run, run.beginning() + 2); i.hasNext(); i.next()) {
					const CodePoint variationSelector = i.current();
					if(variationSelector >= 0xe0100ul && variationSelector <= 0xe01eful) {
						StringCharacterIterator baseCharacter(i);
						baseCharacter.previous();
						if(run.glyphs_->font->ivsGlyph(
								baseCharacter.current(), variationSelector,
								run.glyphs_->indices[run.glyphs_->clusters[baseCharacter.tell() - run.beginning()]])) {
							run.glyphs_->vanish(*run.glyphs_->font, i.tell());
							run.glyphs_->vanish(*run.glyphs_->font, i.tell() + 1);
						}
					}
				}
			}

			// process an IVS across two glyph runs
			if(i + 1 != runs.end() && length(*i[1]) > 1) {
				TextRunImpl& next = *i[1];
				const CodePoint variationSelector = utf::decodeFirst(next.beginning(), next.beginning() + 2);
				if(variationSelector >= 0xe0100ul && variationSelector <= 0xe01eful) {
					const CodePoint baseCharacter = utf::decodeLast(run.beginning(), run.end());
					if(run.glyphs_->font->ivsGlyph(baseCharacter, variationSelector,
							run.glyphs_->indices[run.glyphs_->clusters[length(run) - 1]])) {
						next.glyphs_->vanish(*run.glyphs_->font, next.beginning());
						next.glyphs_->vanish(*run.glyphs_->font, next.beginning() + 1);
					}
				}
			}
		}
#undef ASCENSION_VANISH_VARIATION_SELECTOR
	}
#endif // ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
}

/// @see TextRun#trailingEdge
Scalar TextRunImpl::trailingEdge(Index character) const {
	return ipd(beginning() + character, true);	// TODO: this ignores HRESULT.
}


// InlineProgressionDimensionRangeIterator file-local class ///////////////////////////////////////

namespace {
	class InlineProgressionDimensionRangeIterator :
		public boost::iterator_facade<InlineProgressionDimensionRangeIterator,
			Range<Scalar>, input_iterator_tag, Range<Scalar>, ptrdiff_t
		> {
	public:
		InlineProgressionDimensionRangeIterator() /*noexcept*/
			: currentRun_(begin(dummy_)), lastRun_(begin(dummy_)) {}
		InlineProgressionDimensionRangeIterator(
			const Range<vector<unique_ptr<const TextRun>>::const_iterator>& textRunsOfLine,
			ReadingDirection layoutDirection, const StringPiece& effectiveCharacterRange,
			const Direction& scanningDirection, Scalar firstLineEdgeIpd);
		Range<Scalar> dereference() const;
		const StringPiece& effectiveCharacterRange() const /*noexcept*/ {
			return effectiveCharacterRange_;
		}
		bool equal(const InlineProgressionDimensionRangeIterator& other) const /*noexcept*/ {
			return isDone() && other.isDone();
		}
		void increment() {
			return next(false);
		}
		Direction scanningDirection() const /*noexcept*/ {
			int temp = (currentRun_ <= lastRun_) ? 0 : 1;
			temp += (layoutDirection_ == LEFT_TO_RIGHT) ? 0 : 1;
			return (temp % 2 == 0) ? Direction::FORWARD : Direction::BACKWARD;
		}
	private:
		static ReadingDirection computeScanningReadingDirection(
				ReadingDirection layoutDirection, const Direction& scanningDirection) {
			ReadingDirection computed = layoutDirection;
			if(scanningDirection == Direction::BACKWARD)
				computed = !computed;
			return computed;
		}
		void next(bool initializing);
		bool isDone() const /*noexcept*/ {return currentRun_ == lastRun_;}
	private:
		static const vector<unique_ptr<const TextRun>> dummy_;
		friend class boost::iterator_core_access;
		/*const*/ ReadingDirection layoutDirection_;
		/*const*/ StringPiece effectiveCharacterRange_;
		vector<unique_ptr<const TextRun>>::const_iterator currentRun_;
		/*const*/ vector<unique_ptr<const TextRun>>::const_iterator lastRun_;
		Scalar currentRunStartEdge_;	// 'start' means for 'layoutDirection_'
	};
}

InlineProgressionDimensionRangeIterator::InlineProgressionDimensionRangeIterator(
		const Range<vector<unique_ptr<const TextRun>>::const_iterator>& textRunsOfLine,
		ReadingDirection layoutDirection, const StringPiece& effectiveCharacterRange,
		const Direction& scanningDirection, Scalar firstLineEdgeIpd) :
		effectiveCharacterRange_(effectiveCharacterRange), layoutDirection_(layoutDirection),
		currentRunStartEdge_(firstLineEdgeIpd) {
	const ReadingDirection scanningReadingDirection = computeScanningReadingDirection(layoutDirection, scanningDirection);
	currentRun_ = (scanningReadingDirection == LEFT_TO_RIGHT) ? textRunsOfLine.beginning() : textRunsOfLine.end() - 1;
	lastRun_ = (scanningReadingDirection == LEFT_TO_RIGHT) ? textRunsOfLine.end() : textRunsOfLine.beginning() - 1;
	next(true);
}

Range<Scalar> InlineProgressionDimensionRangeIterator::dereference() const {
	if(isDone())
		throw NoSuchElementException();
	const TextRunImpl& currentRun = static_cast<const TextRunImpl&>(**currentRun_);
	const Range<StringPiece::const_pointer> subrange(intersected(currentRun, effectiveCharacterRange()));
	assert(!isEmpty(subrange));
	const Scalar startInRun = currentRun.leadingEdge(subrange.beginning() - currentRun.beginning());
	const Scalar endInRun = currentRun.trailingEdge(subrange.end() - currentRun.beginning());
	assert(startInRun <= endInRun);
	const Scalar startOffset = (currentRun.direction() == layoutDirection_) ? startInRun : currentRun.measure() - endInRun;
	const Scalar endOffset = (currentRun.direction() == layoutDirection_) ? endInRun : currentRun.measure() - startInRun;
	assert(startOffset <= endOffset);
	return makeRange(currentRunStartEdge_ + startOffset, currentRunStartEdge_ + endOffset);
}

void InlineProgressionDimensionRangeIterator::next(bool initializing) {
	if(isDone())
		throw NoSuchElementException();
	vector<unique_ptr<const TextRun>>::const_iterator nextRun(currentRun_);
	Scalar nextIpd = currentRunStartEdge_;
	const Direction sd = scanningDirection();
	const ReadingDirection srd = computeScanningReadingDirection(layoutDirection_, sd);
	while(nextRun != lastRun_) {
		if(sd == Direction::FORWARD) {
			if(intersects(static_cast<const TextRunImpl&>(**nextRun), effectiveCharacterRange()))
				break;
			nextIpd += (*nextRun)->measure();
		} else {
			nextIpd -= (*nextRun)->measure();
			if(intersects(static_cast<const TextRunImpl&>(**nextRun), effectiveCharacterRange()))
				break;
		}
		if(srd == LEFT_TO_RIGHT)
			++nextRun;
		else
			--nextRun;
	}
	// commit
	currentRun_ = nextRun;
	currentRunStartEdge_ = nextIpd;
}


// TextLayout /////////////////////////////////////////////////////////////////////////////////////

// helpers for TextLayout.draw
namespace {
	const size_t MAXIMUM_RUN_LENGTH = 1024;
	inline win32::Handle<HPEN> createPen(const Color& color, int width, int style) {
		if(color.alpha() < 0xff)
			throw invalid_argument("color");
		LOGBRUSH brush;
		brush.lbColor = color.as<COLORREF>();
		brush.lbStyle = BS_SOLID;
		HPEN pen = nullptr;
		switch(style) {
		case 1:	// solid
			pen = (width == 1) ? ::CreatePen(PS_SOLID, 1, color.as<COLORREF>())
				: ::ExtCreatePen(PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_FLAT, width, &brush, 0, nullptr);
		case 2:	// dashed
			pen = ::ExtCreatePen(PS_GEOMETRIC | PS_DASH | PS_ENDCAP_FLAT, width, &brush, 0, nullptr);
		case 3:	// dotted
			pen = ::ExtCreatePen(PS_GEOMETRIC | PS_DOT | PS_ENDCAP_FLAT, width, &brush, 0, nullptr);
		}
		if(pen == nullptr)
			throw UnknownValueException("style");
		return win32::Handle<HPEN>(pen, &::DeleteObject);
	}
} // namespace @0

/**
 * @class ascension::graphics::font::TextLayout
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
	// TODO: this implementation is temporary, and should rewrite later
	class SillyLineMetrics : public LineMetrics {
	public:
		SillyLineMetrics(Scalar ascent, Scalar descent) /*throw()*/ : ascent_(ascent), descent_(descent) {}
	private:
		Scalar ascent() const /*throw()*/ {return ascent_;}
		DominantBaseline baseline() const /*throw()*/ {return DominantBaseline::ALPHABETIC;}
		Scalar baselineOffset(AlignmentBaseline baseline) const /*throw()*/ {return 0;}
		Scalar descent() const /*throw()*/ {return descent_;}
//		Scalar leading() const /*throw()*/ {return 0;}
	private:
		Scalar ascent_, descent_;
	};
}

/**
 * Constructor.
 * @param textString The text string to display
 * @param lineStyle The computed text line style
 * @param textRunStyles The computed text runs styles
 */
TextLayout::TextLayout(const String& textString,
		const ComputedTextLineStyle& lineStyle, std::unique_ptr<ComputedStyledTextRunIterator> textRunStyles)
		: textString_(textString), lineStyle_(lineStyle), numberOfLines_(0) {

	// handle logically empty line
	if(textString_.empty()) {
		numberOfLines_ = 1;
		maximumMeasure_ = 0;
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
	// 7. stack the lines

	// 2. split each script runs into text runs with StyledRunIterator
	vector<TextRunImpl*> textRuns;
	vector<AttributedCharacterRange<const ComputedTextRunStyle>> calculatedStyles;
	const FontCollection& fontCollection = (otherParameters.fontCollection != nullptr) ? *otherParameters.fontCollection : installedFonts();
	TextRunImpl::generate(textString_, fontCollection, lineStyle_, move(textRunStyles), textRuns, calculatedStyles);
//	runs_.reset(new TextRun*[numberOfRuns_ = textRuns.size()]);
//	copy(textRuns.begin(), textRuns.end(), runs_.get());
//	shrinkToFit(styledRanges_);

	// 3. generate glyphs for each text runs
	const win32::Handle<HDC> dc(detail::screenDC());
	for(auto run(begin(textRuns)), e(end(textRuns)); run != e; ++run)
		(*run)->shape(dc);
	TextRunImpl::substituteGlyphs(makeRange(begin(textRuns), end(textRuns)));

	// 4. position glyphs for each text runs
	for(auto run(begin(textRuns)), b(begin(textRuns)), e(end(textRuns)); run != e; ++run)
		(*run)->positionGlyphs(dc, calculatedStyles[run - b].attribute);

	// 5. position each text runs
	String nominalFontFamilyName;
	FontProperties<> nominalFontProperties;
	resolveFontSpecifications(fontCollection,
		shared_ptr<const TextRunStyle>(), otherParameters.defaultTextRunStyle, &nominalFontFamilyName, &nominalFontProperties, nullptr);
	const shared_ptr<const Font> nominalFont(fontCollection.get(nominalFontFamilyName, nominalFontProperties));
	// wrap into visual lines and reorder runs in each lines
	if(runs_.empty() || !wrapsText(lineStyle.whiteSpace)) {
		numberOfLines_ = 1;
		lineOffsets_.reset(&SINGLE_LINE_OFFSETS);
		lineFirstRuns_.reset(&SINGLE_LINE_OFFSETS);
		// 5-2. reorder each text runs
		reorder();
		// 5-3. reexpand horizontal tabs
		expandTabsWithoutWrapping();
	} else {
		// 5-1. expand horizontal tabs and wrap into lines
		const TabExpander* tabExpander = otherParameters.tabExpander;
		unique_ptr<TabExpander> temp;
		if(tabExpander == nullptr) {
			// create default tab expander
			temp.reset(new FixedWidthTabExpander(nominalFont->metrics()->averageCharacterWidth() * 8));
			tabExpander = temp.get();
		}
		wrap(*tabExpander);
		// 5-2. reorder each text runs
		reorder();
		// 5-3. reexpand horizontal tabs
		// TODO: not implemented.
		// 6. justify each text runs if specified
		if(lineStyle.justification != TextJustification::NONE)
			justify(lineStyle.justification);
	}

	// 7. stack the lines
	stackLines(otherParameters.lineStackingStrategy, *nominalFont, otherParameters.lineHeight);
}

/// Destructor.
TextLayout::~TextLayout() /*throw()*/ {
//	for(size_t i = 0; i < numberOfRuns_; ++i)
//		delete runs_[i];
//	for(vector<const InlineArea*>::const_iterator i(inlineAreas_.begin()), e(inlineAreas_.end()); i != e; ++i)
//		delete *i;
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
	if(style_.get() != nullptr && style_->readingDirection != INHERIT_TEXT_ALIGNMENT)
		style_->readingDirection;
	shared_ptr<const TextLineStyle> defaultStyle(lip_.presentation().defaultTextLineStyle());
	return (defaultStyle.get() != nullptr
		&& defaultStyle->alignment != INHERIT_TEXT_ALIGNMENT) ? defaultStyle->alignment : ASCENSION_DEFAULT_TEXT_ALIGNMENT;
}
#endif
/**
 * Returns distance from the baseline of the first line to the baseline of the
 * specified line in pixels.
 * @param line The line number
 * @return The baseline position 
 * @throw BadPositionException @a line is greater than the count of lines
 */
Scalar TextLayout::baseline(Index line) const {
	if(line >= numberOfLines())
		throw kernel::BadPositionException(k::Position(line, 0));
	else if(line == 0)
		return 0;
	Scalar result = 0;
	for(Index i = 1; i <= line; ++i) {
		result += lineMetrics_[i - 1]->descent();
		result += lineMetrics_[i]->ascent();
	}
	return result;
}

/**
 * Returns the black box bounds of the characters in the specified range. The black box bounds is
 * an area consisting of the union of the bounding boxes of the all of the characters in the range.
 * The result region can be disjoint.
 * @param range The character range
 * @return The native polygon object encompasses the black box bounds
 * @throw kernel#BadPositionException @a range intersects with the outside of the line
 * @see #bounds(void), #bounds(Index, Index), #lineBounds, #lineStartEdge
 */
NativeRegion TextLayout::blackBoxBounds(const Range<Index>& range) const {
	if(range.end() > textString_.length())
		throw kernel::BadPositionException(kernel::Position(0, range.end()));

	// handle empty line
	if(isEmpty())
		return win32::Handle<HRGN>(::CreateRectRgn(0, 0, 0, lineMetrics_[0]->height()), &::DeleteObject);

	// TODO: this implementation can't handle vertical text.
	const Index firstLine = lineAt(range.beginning()), lastLine = lineAt(range.end());
	vector<NativeRectangle> rectangles;
	Scalar before = baseline(firstLine)
		/*- lineMetrics_[firstLine]->leading()*/ - lineMetrics_[firstLine]->ascent();
	Scalar after = before + lineMetrics_[firstLine]->height();
	for(Index line = firstLine; line <= lastLine; before = after, after += lineMetrics_[++line]->height()) {
		const size_t lastRun = (line + 1 < numberOfLines()) ? lineFirstRuns_[line + 1] : numberOfRuns_;
		const Scalar leftEdge = (writingMode().inlineFlowDirection == LEFT_TO_RIGHT) ?
			lineStartEdge(line) : (-lineStartEdge(line) - measure(line));

		// is the whole line encompassed by the range?
		if(range.beginning() <= lineOffset(line) && range.end() >= lineOffset(line) + lineLength(line))
			rectangles.push_back(
				geometry::make<NativeRectangle>(
					geometry::make<NativePoint>(leftEdge, before),
					geometry::make<NativePoint>(leftEdge + measure(line), after)));
		else {
			for(InlineProgressionDimensionRangeIterator i(
					Range<const TextRun* const*>(runs_.get() + lineFirstRuns_[line], runs_.get() + lastRun),
					range, LEFT_TO_RIGHT, leftEdge), e; i != e; ++i)
				rectangles.push_back(
					geometry::make<NativeRectangle>(
						geometry::make<NativePoint>(i->beginning(), before),
						geometry::make<NativePoint>(i->end(), after)));
		}
	}

	// create the result region
	unique_ptr<POINT[]> vertices(new POINT[rectangles.size() * 4]);
	unique_ptr<int[]> numbersOfVertices(new int[rectangles.size()]);
	for(size_t i = 0, c = rectangles.size(); i < c; ++i) {
		geometry::x(vertices[i * 4 + 0]) = geometry::x(vertices[i * 4 + 3]) = geometry::left(rectangles[i]);
		geometry::y(vertices[i * 4 + 0]) = geometry::y(vertices[i * 4 + 1]) = geometry::top(rectangles[i]);
		geometry::x(vertices[i * 4 + 1]) = geometry::x(vertices[i * 4 + 2]) = geometry::right(rectangles[i]);
		geometry::y(vertices[i * 4 + 2]) = geometry::y(vertices[i * 4 + 3]) = geometry::bottom(rectangles[i]);
	}
	fill_n(numbersOfVertices.get(), rectangles.size(), 4);
	return win32::Handle<HRGN>(::CreatePolyPolygonRgn(vertices.get(),
		numbersOfVertices.get(), static_cast<int>(rectangles.size()), WINDING), &::DeleteObject);
}

/**
 * Returns the smallest rectangle emcompasses the whole text of the line. It might not coincide
 * exactly the ascent, descent or overhangs of the text.
 * @return The size of the bounds
 * @see #blackBoxBounds, #bounds(const Range&lt;Index&gt;&amp;), #lineBounds
 */
FlowRelativeFourSides<Scalar> TextLayout::bounds() const /*noexcept*/ {
	// TODO: this implementation can't handle vertical text.
	FlowRelativeFourSides<Scalar> result;
	result.before() = /*-lineMetrics(0).leading()*/ - lineMetrics(0).ascent();
	result.after() = result.before();
	result.start() = numeric_limits<Scalar>::max();
	result.end() = numeric_limits<Scalar>::min();
	for(Index line = 0; line < numberOfLines(); ++line) {
		result.after() += lineMetrics(line).height();
		const Scalar lineStart = lineStartEdge(line);
		result.start() = min(lineStart, result.start());
		result.end() = max(lineStart + measure(line), result.end());
	}
	return result;
}

/**
 * Returns the smallest rectangle emcompasses all characters in the range. It might not coincide
 * exactly the ascent, descent or overhangs of the specified region of the text.
 * @param characterRange The character range
 * @return The bounds
 * @throw kernel#BadPositionException @a characterRange intersects with the outside of the line
 * @see #blackBoxBounds, #bounds(void), #lineBounds
 */
FlowRelativeFourSides<Scalar> TextLayout::bounds(const Range<Index>& characterRange) const {
	if(characterRange.end() > textString_.length())
		throw kernel::BadPositionException(kernel::Position(0, characterRange.end()));

	FlowRelativeFourSides<Scalar> result;

	if(isEmpty()) {	// empty line
		result.start() = result.end() = 0;
		result.before() = -lineMetrics(0).ascent()/* - lineMetrics(0).leading()*/;
		result.after() = lineMetrics(0).descent();
	} else if(ascension::isEmpty(characterRange)) {	// an empty rectangle for an empty range
		const LineMetrics& line = lineMetrics(lineAt(characterRange.beginning()));
		const AbstractTwoAxes<Scalar> leading(location(characterRange.beginning()));
		FlowRelativeFourSides<Scalar> sides;
		sides.before() = leading.bpd() - line.ascent();
		sides.after() = leading.bpd() + line.descent();
		sides.start() = sides.end() = leading.ipd();
		return sides;
	} else {
		const Index firstLine = lineAt(characterRange.beginning()), lastLine = lineAt(characterRange.end());

		// calculate the block-progression-edges ('before' and 'after'; it's so easy)
		result.before() = baseline(firstLine) - lineMetrics(firstLine).ascent()/* - lineMetrics(firstLine).leading()*/;
		result.after() = baseline(lastLine) + lineMetrics(lastLine).descent();

		// calculate start-edge and end-edge of fully covered lines
		const bool firstLineIsFullyCovered = includes(characterRange,
			makeRange(lineOffset(firstLine), lineOffset(firstLine) + lineLength(firstLine)));
		const bool lastLineIsFullyCovered = includes(characterRange,
			makeRange(lineOffset(lastLine), lineOffset(lastLine) + lineLength(lastLine)));
		result.start() = numeric_limits<Scalar>::max();
		result.end() = numeric_limits<Scalar>::min();
		for(Index line = firstLine + firstLineIsFullyCovered ? 0 : 1;
				line < lastLine + lastLineIsFullyCovered ? 1 : 0; ++line) {
			const Scalar lineStart = lineStartEdge(line);
			result.start() = min(lineStart, result.start());
			result.end() = max(lineStart + measure(line), result.end());
		}

		// calculate start and end-edge of partially covered lines
		vector<Index> partiallyCoveredLines;
		if(!firstLineIsFullyCovered)
			partiallyCoveredLines.push_back(firstLine);
		if(!lastLineIsFullyCovered && (partiallyCoveredLines.empty() || partiallyCoveredLines[0] != lastLine))
			partiallyCoveredLines.push_back(lastLine);
		if(!partiallyCoveredLines.empty()) {
			Scalar start = result.start(), end = result.end();
			const StringPiece effectiveCharacterRange(textString_.data() + characterRange.beginning(), length(characterRange));
			for(vector<Index>::const_iterator
					line(begin(partiallyCoveredLines)), e(std::end(partiallyCoveredLines)); line != e; ++line) {
				const RunVector::const_iterator lastRun(
					(*line + 1 < numberOfLines()) ? begin(runs_) + lineFirstRuns_[*line + 1] : std::end(runs_));

				// find 'start-edge'
				InlineProgressionDimensionRangeIterator i(
					makeRange(begin(runs_) + lineFirstRuns_[*line], lastRun),
					writingMode().inlineFlowDirection, effectiveCharacterRange, Direction::FORWARD, lineStartEdge(*line));
				assert(i != InlineProgressionDimensionRangeIterator());
				start = min(i->beginning(), start);

				// find 'end-edge'
				i = InlineProgressionDimensionRangeIterator(
					makeRange(begin(runs_) + lineFirstRuns_[*line], lastRun),
					writingMode().inlineFlowDirection, effectiveCharacterRange, Direction::BACKWARD, lineStartEdge(*line) + measure(*line));
				assert(i != InlineProgressionDimensionRangeIterator());
				end = max(i->end(), end);
			}

			result.start() = start;
			result.end() = end;
		}
	}

	return result;
}

/**
 * Returns the bidirectional embedding level at specified position.
 * @param offsetInLine The offset in the line
 * @return The embedding level
 * @throw kernel#BadPositionException @a offsetInLine is greater than the length of the line
 */
uint8_t TextLayout::characterLevel(Index offsetInLine) const {
	if(isEmpty()) {
		if(offsetInLine != 0)
			throw kernel::BadPositionException(kernel::Position(0, offsetInLine));
		// use the default level
		return (writingMode().inlineFlowDirection == RIGHT_TO_LEFT) ? 1 : 0;
	}
	const auto run(findRunForPosition(offsetInLine));
	if(run == end(runs_))
		throw kernel::BadPositionException(kernel::Position(0, offsetInLine));
	return (*run)->characterLevel();
}

namespace {
	inline HRESULT callScriptItemize(const WCHAR* text, int length, int estimatedNumberOfItems,
			const SCRIPT_CONTROL& control, const SCRIPT_STATE& initialState, SCRIPT_ITEM items[], OPENTYPE_TAG scriptTags[], int& numberOfItems) {
		static HRESULT(WINAPI* scriptItemizeOpenType)(const WCHAR*, int, int,
			const SCRIPT_CONTROL*, const SCRIPT_STATE*, SCRIPT_ITEM*, OPENTYPE_TAG*, int*) = uspLib->get<0>();
		if(scriptItemizeOpenType != nullptr && scriptTags != nullptr)
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
		const NativePoint& origin, const TextPaintOverride* paintOverride /* = nullptr */,
		const InlineObject* endOfLine/* = nullptr */, const InlineObject* lineWrappingMark /* = nullptr */) const {

#if /*defined(_DEBUG)*/ 0
	if(DIAGNOSE_INHERENT_DRAWING)
		win32::DumpContext() << L"@TextLayout.draw draws line " << lineNumber_ << L" (" << line << L")\n";
#endif // defined(_DEBUG)

	if(isEmpty() || geometry::dy(context.boundsToPaint()) == 0)
		return;

	// TODO: this code can't handle vertical text.

	// calculate line range to draw
	Range<Index> linesToDraw(0, numberOfLines());
	NativePoint p(origin);
	for(Index line = linesToDraw.beginning(); line < linesToDraw.end(); ++line) {
		geometry::y(p) = baseline(line);
		const Scalar lineBeforeEdge = geometry::y(p) - lineMetrics_[line]->ascent();
		const Scalar lineAfterEdge = geometry::y(p) + lineMetrics_[line]->descent();
		if(geometry::top(context.boundsToPaint()) >= lineBeforeEdge && geometry::top(context.boundsToPaint()) < lineAfterEdge)
			linesToDraw = makeRange(line, linesToDraw.end());
		if(geometry::bottom(context.boundsToPaint()) >= lineBeforeEdge && geometry::bottom(context.boundsToPaint()) < lineAfterEdge) {
			linesToDraw = makeRange(linesToDraw.beginning(), line + 1);
			break;
		}
	}

	// calculate inline area range to draw
	Range<vector<const InlineArea*>::const_iterator> inlineAreasToDraw(inlineAreas_.begin(), inlineAreas_.end());
	for(vector<const InlineArea*>::const_iterator i(inlineAreasToDraw.beginning()); i != inlineAreasToDraw.end(); ++i) {
		const Index endOfInlineArea = (i != inlineAreasToDraw.end()) ? i[1]->position() : textString_.length();
		if(endOfInlineArea > lineOffset(linesToDraw.beginning())) {
			inlineAreasToDraw = makeRange(i, inlineAreasToDraw.end());
			break;
		}
	}
	for(vector<const InlineArea*>::const_iterator i(inlineAreasToDraw.beginning()); i != inlineAreasToDraw.end(); ++i) {
		const Index endOfInlineArea = (i != inlineAreasToDraw.end()) ? i[1]->position() : textString_.length();
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
//	context.setTextAlign();
//	context.setTextBaseline();
//	::SetTextAlign(context.nativeObject().get(), TA_TOP | TA_LEFT | TA_NOUPDATECP);

	// 2. paint backgrounds and borders
	for(vector<const InlineArea*>::const_iterator i(inlineAreasToDraw.beginning()), e; i != inlineAreasToDraw.end(); ++i) {
		// TODO: recognize the override.
		// TODO: this code can't handle sparse inline areas (with bidirectionality).
		boost::optional<NativeRectangle> borderRectangle;

		// 2-1. paint background if the property is specified (= if not transparent)
		if((*i)->style()->background) {
			borderRectangle = (*i)->borderRectangle();
			if(geometry::includes(context.boundsToPaint(), *borderRectangle)) {
				context.setFillStyle((*i)->style()->background);
				context.fillRectangle(*borderRectangle);
			}
		}

		// 2-2. paint border if the property is specified
		assert((*i)->style()->color);
		detail::paintBorder(context, (*i)->borderRectangle(), (*i)->style()->border, *(*i)->style()->color, writingMode());

		::ExcludeClipRect(context.asNativeObject().get(),
			geometry::left(*borderRectangle), geometry::top(*borderRectangle),
			geometry::right(*borderRectangle), geometry::bottom(*borderRectangle));
	}

	// 3. for each text runs
	for(Index line = linesToDraw.beginning(); line < linesToDraw.end(); ++line) {
		if(!isEmpty()) {
			// 3-1. calculate range of runs to paint
			Range<const TextRun* const*> runs(runs_.get() + lineFirstRuns_[line],
				runs_.get() + ((line < numberOfLines() - 1) ? lineFirstRuns_[line + 1] : numberOfRuns_));
			p = origin;
			geometry::x(p) += readingDirectionInt(writingMode().inlineFlowDirection);
			if(writingMode().inlineFlowDirection == RIGHT_TO_LEFT)
				geometry::x(p) -= measure(line);
			Scalar leftEdgeOfFirstRun = geometry::x(p), rightEdgeOfLastRun = geometry::x(p) + measure(line);
			for(const TextRun* const* run = runs.beginning(); run < runs.end(); ++run) {
				if(geometry::x(p) + (*run)->totalWidth() < geometry::left(context.boundsToPaint())) {
					runs = makeRange(run + 1, runs.end());
					leftEdgeOfFirstRun = geometry::x(p) + (*run)->totalWidth();
				} else if(p.x > geometry::right(context.boundsToPaint())) {
					runs = makeRange(runs.beginning(), run);
					rightEdgeOfLastRun = geometry::x(p);
				}
			}
			if(!ascension::isEmpty(runs)) {
				const Range<Index> characterRange(runs.beginning()[0]->beginning(), runs.end()[-1]->end());
				unique_ptr<TextPaintOverride::Iterator> paintOverrideIterator;
				if(paintOverride != nullptr)
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
			else if(run.requestedStyle().get() != nullptr && run.requestedStyle()->foreground != Color())
				foreground = run.requestedStyle()->foreground.asCOLORREF();
			else
				foreground = defaultForeground;
			if(line[run.beginning()] != L'\t') {
				if(selection == nullptr /*|| run.overhangs()*/
						|| !(run.beginning() >= selectedRange.beginning() && run.end() <= selectedRange.end())) {
					dc.setTextColor(foreground);
					runRect.left = x;
					runRect.right = runRect.left + run.totalWidth();
					hr = run.draw(dc, x, y + lip_.textMetrics().ascent(), false, &runRect);
				}
			}
			// decoration (underline and border)
			if(run.requestedStyle().get() != nullptr)
				drawDecorationLines(dc, *run.requestedStyle(), foreground, x, y, run.totalWidth(), dy);
			x += run.totalWidth();
			runRect.left = x;
		}

		// draw selected text segment (also underline and border)
		if(selection != nullptr) {
			x = startX;
			clipRegion.setRect(clipRect);
			dc.selectClipRgn(clipRegion.get(), RGN_XOR);
			for(size_t i = firstRun; i < lastRun; ++i) {
				TextRun& run = *runs_[i];
				// text
				if(selection != nullptr && line[run.beginning()] != L'\t'
						&& (/*run.overhangs() ||*/ (run.beginning() < selectedRange.end() && run.end() > selectedRange.beginning()))) {
					dc.setTextColor(selection->color().foreground.asCOLORREF());
					runRect.left = x;
					runRect.right = runRect.left + run.totalWidth();
					hr = run.draw(dc, x, y + lip_.textMetrics().ascent(), false, &runRect);
				}
				// decoration (underline and border)
				if(run.requestedStyle().get() != nullptr)
					drawDecorationLines(dc, *run.requestedStyle(), selection->color().foreground.asCOLORREF(), x, y, run.totalWidth(), dy);
				x += run.totalWidth();
			}
		}

		// special character substitution
		if(specialCharacterRenderer != nullptr) {
			// white spaces and C0/C1 control characters
			dc.selectClipRgn(clipRegion.get());
			x = startX;
			for(size_t i = firstRun; i < lastRun; ++i) {
				TextRun& run = *runs_[i];
				context.readingDirection = run.writingMode().inlineFlowDirection;
				for(Index j = run.beginning(); j < run.end(); ++j) {
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
				&& resolveTextAlignment(alignment(), writingMode().inlineFlowDirection) == ALIGN_RIGHT)
			x = startX;
	} // end of nonempty line case
	
	// line terminator and line wrapping mark
	const Document& document = lip_.presentation().document();
	if(specialCharacterRenderer != nullptr) {
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
			if(selection != nullptr) {
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
		out << static_cast<unsigned int>(i)
			<< ":beginning=" << static_cast<unsigned int>(run.beginning())
			<< ",length=" << static_cast<unsigned int>(length(run)) << endl;
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

	unique_ptr<DC> dc(lip_.getFontSelector().deviceContext());
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
 * Returns the index of run containing the specified offset in the line.
 * @param offsetInLine The offset in the line
 * @return The index of the run
 */
inline TextLayout::RunVector::const_iterator TextLayout::findRunForPosition(Index offsetInLine) const /*throw()*/ {
	assert(!isEmpty());
	if(offsetInLine == textString_.length())
		return numberOfRuns_ - 1;
	const Index sl = lineAt(offsetInLine);
	const size_t lastRun = (sl + 1 < numberOfLines()) ? lineFirstRuns_[sl + 1] : numberOfRuns_;
	for(size_t i = lineFirstRuns_[sl]; i < lastRun; ++i) {
		if(runs_[i]->beginning() <= offsetInLine && runs_[i]->end() > offsetInLine)	// TODO: replace with includes().
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
	if(writingMode().inlineFlowDirection == RIGHT_TO_LEFT)
		return true;
	for(size_t i = 0; i < numberOfRuns_; ++i) {
		if(runs_[i]->readingDirection() == RIGHT_TO_LEFT)
			return true;
	}
	return false;
}

/// Justifies the wrapped visual lines.
inline void TextLayout::justify(TextJustification) /*throw()*/ {
	assert(wrappingMeasure_ != -1);
	for(Index line = 0; line < numberOfLines(); ++line) {
		const int ipd = measure(line);
		const size_t last = (line + 1 < numberOfLines()) ? lineFirstRuns_[line + 1] : numberOfRuns_;
		for(size_t i = lineFirstRuns_[line]; i < last; ++i) {
			TextRun& run = *runs_[i];
			const int newRunMeasure = ::MulDiv(run.totalWidth(), wrappingMeasure_, ipd);	// TODO: there is more precise way.
			run.justify(newRunMeasure);
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
 * @see #basline, #lineStartEdge, #measure
 */
FlowRelativeFourSides<Scalar> TextLayout::lineBounds(Index line) const {
	if(line >= numberOfLines())
		throw IndexOutOfBoundsException("line");

	FlowRelativeFourSides<Scalar> sides;
	sides.start() = lineStartEdge(line);
	sides.end() = sides.start() + measure(line);
	sides.before() = baseline(line) - lineMetrics_[line]->ascent()/* - lineMetrics_[line]->leading()*/;
	sides.after() = sides.before() + lineMetrics_[line]->height();
	return sides;
/*
	// TODO: this implementation can't handle vertical text.
	const NativeSize size(geometry::make<NativeSize>(end - start, after - before));
	const NativePoint origin(geometry::make<NativePoint>(
		(writingMode().inlineFlowDirection == LEFT_TO_RIGHT) ? start : start - geometry::dx(size), before));
	return geometry::make<NativeRectangle>(origin, size);
*/
}

/**
 * Returns the start-edge of the specified line without the start-indent in pixels.
 * @par This is distance from the start-edge of the first line to the one of @a line in
 * inline-progression-dimension. Therefore, returns always zero when @a line is zero or the anchor
 * is @c TEXT_ANCHOR_START.
 * @par A positive value means positive indentation. For example, if the start-edge of a RTL line
 * is x = -10, this method returns +10.
 * @param line The line number
 * @return The start-indentation in pixels
 * @throw IndexOutOfBoundsException @a line is invalid
 * @see TextRenderer#lineStartEdge
 */
Scalar TextLayout::lineStartEdge(Index line) const {
	if(line == 0)
		return 0;
	switch(anchor()) {
	case TEXT_ANCHOR_START:
		return 0;
	case TEXT_ANCHOR_MIDDLE:
		return (measure(0) - measure(line)) / 2;
	case TEXT_ANCHOR_END:
		return measure(0) - measure(line);
	default:
		ASCENSION_ASSERT_NOT_REACHED();
	}
}

/**
 * Converts a position in the block-progression-direction into the corresponding line.
 * @param bpd The position in block-progression-dimension in pixels
 * @param[out] outside @c true if @a bpd is outside of the line content
 * @return The line number
 * @see #basline, #lineAt, #offset
 */
Index TextLayout::locateLine(Scalar bpd, bool& outside) const /*throw()*/ {
	// TODO: This implementation can't handle tricky 'text-orientation'.

	// beyond the before-edge ?
	if(bpd < -lineMetrics_[0]->ascent()/* - lineMetrics_[0]->leading()*/)
		return (outside = true), 0;

	Index line = 0;
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
pair<Index, Index> TextLayout::locateOffsets(Index line, Scalar ipd, bool& outside) const {
	if(isEmpty())
		return (outside = true), make_pair(static_cast<Index>(0), static_cast<Index>(0));
	const size_t lastRun = (line + 1 < numberOfLines()) ? lineFirstRuns_[line + 1] : numberOfRuns_;

	if(writingMode().inlineFlowDirection == LEFT_TO_RIGHT) {
		Scalar x = lineStartEdge(line);
		if(ipd < x) {	// beyond the left-edge => the start of the first run
			const Index offsetInLine = runs_[lineFirstRuns_[line]]->beginning();
			return (outside = true), make_pair(offsetInLine, offsetInLine);
		}
		for(size_t i = lineFirstRuns_[line]; i < lastRun; ++i) {	// scan left to right
			const TextRun& run = *runs_[i];
			if(ipd >= x && ipd <= x + run.totalWidth()) {
				int cp, trailing;
				run.hitTest(ipd - x, cp, trailing);	// TODO: check the returned value.
				const Index temp = run.beginning() + static_cast<Index>(cp);
				return (outside = false), make_pair(temp, temp + static_cast<Index>(trailing));
			}
			x += run.totalWidth();
		}
		// beyond the right-edge => the end of last run
		const Index offsetInLine = runs_[lastRun - 1]->end();
		return (outside = true), make_pair(offsetInLine, offsetInLine);
	} else {
		Scalar x = -lineStartEdge(line);
		if(ipd > x) {	// beyond the right-edge => the start of the last run
			const Index offsetInLine = runs_[lastRun - 1]->beginning();
			return (outside = true), make_pair(offsetInLine, offsetInLine);
		}
		// beyond the left-edge => the end of the first run
		const Index offsetInLine = runs_[lineFirstRuns_[line]]->end();
		return (outside = true), make_pair(offsetInLine, offsetInLine);
	}
}

// implements public location methods
void TextLayout::locations(Index offsetInLine, AbstractTwoAxes<Scalar>* leading, AbstractTwoAxes<Scalar>* trailing) const {
	assert(leading != nullptr || trailing != nullptr);
	if(offsetInLine > textString_.length())
		throw kernel::BadPositionException(kernel::Position(0, offsetInLine));

	Scalar leadingIpd, trailingIpd, bpd = 0/* + lineMetrics_[0]->leading()*/;
	if(isEmpty()) {
		leadingIpd = trailingIpd = 0;
		bpd += lineMetrics_[0]->ascent();
	} else {
		// inline-progression-dimension
		const StringPiece::const_pointer at = textString_.data() + offsetInLine;
		const Index line = lineAt(offsetInLine);
		const Index firstRun = lineFirstRuns_[line];
		const Index lastRun = (line + 1 < numberOfLines()) ? lineFirstRuns_[line + 1] : numberOfRuns_;
		if(writingMode().inlineFlowDirection == LEFT_TO_RIGHT) {	// LTR
			Scalar ipd = lineStartEdge(line);
			for(size_t i = firstRun; i < lastRun; ++i) {
				const TextRunImpl& run = *static_cast<const TextRunImpl*>(runs_[i].get());	// TODO: Down-cast.
				if(at >= run.beginning() && at <= run.end()) {
					if(leading != nullptr)
						leadingIpd = ipd + run.leadingEdge(at - run.beginning());
					if(trailing != nullptr)
						trailingIpd = ipd + run.trailingEdge(at - run.beginning());
					break;
				}
				ipd += run.measure();
			}
		} else {	// RTL
			Scalar ipd = lineStartEdge(line);
			for(size_t i = lastRun - 1; ; --i) {
				const TextRunImpl& run = *static_cast<const TextRunImpl*>(runs_[i].get());	// TODO: Down-cast.
				if(at >= run.beginning() && at <= run.end()) {
					if(leading != nullptr)
						leadingIpd = ipd + run.leadingEdge(at - run.beginning());
					if(trailing != nullptr)
						trailingIpd = ipd + run.trailingEdge(at - run.beginning());
					break;
				}
				if(i == firstRun) {
					ASCENSION_ASSERT_NOT_REACHED();
					break;
				}
				ipd += run.measure();
			}
		}

		// block-progression-dimension
		bpd += baseline(line);
	}
		
	// return the result(s)
	if(leading != nullptr) {
		leading->ipd() = leadingIpd;
		leading->bpd() = bpd;
	}
	if(trailing != nullptr) {
		trailing->ipd() = trailingIpd;
		trailing->bpd() = bpd;
	}
}

/**
 * Returns the inline-progression-dimension of the longest line.
 * @see #measure(Index)
 */
Scalar TextLayout::measure() const /*noexcept*/ {
	if(!maximumMeasure_) {
		Scalar ipd = 0;
		for(Index line = 0; line < numberOfLines(); ++line)
			ipd = max(measure(line), ipd);
		const_cast<TextLayout*>(this)->maximumMeasure_ = ipd;
	}
	return boost::get(maximumMeasure_);
}

/**
 * Returns the length in inline-progression-dimension without the indentations (the distance from
 * the start-edge to the end-edge) of the specified line in pixels.
 * @param line The line number
 * @return The width. Must be equal to or greater than zero
 * @throw IndexOutOfBoundsException @a line is greater than the number of lines
 * @see #measure(void)
 */
Scalar TextLayout::measure(Index line) const {
	if(line >= numberOfLines())
		throw IndexOutOfBoundsException("line");
	else if(isEmpty())
		return const_cast<TextLayout*>(this)->maximumMeasure_ = 0;
	else {
		TextLayout& self = const_cast<TextLayout&>(*this);
		if(numberOfLines() == 1) {
			if(maximumMeasure_)
				return boost::get(maximumMeasure_);
		} else {
			if(measures_.get() == nullptr) {
				self.measures_.reset(new Scalar[numberOfLines()]);
				fill_n(self.measures_.get(), numberOfLines(), -1);
			}
			if(measures_[line] >= 0)
				return measures_[line];
		}
		const size_t lastRun = (line + 1 < numberOfLines()) ? lineFirstRuns_[line + 1] : numberOfRuns_;
		Scalar ipd = 0;
		for(size_t i = lineFirstRuns_[line]; i < lastRun; ++i)
			ipd += runs_[i]->totalWidth();
		assert(ipd >= 0);
		if(numberOfLines() == 1)
			self.maximumMeasure_ = ipd;
		else
			self.measures_[line] = ipd;
		return ipd;
	}
}

/**
 * Returns the hit test information corresponding to the specified point.
 * @param p The point
 * @param[out] outside @c true if the specified point is outside of the layout
 * @return A pair of the character offsets. The first element addresses the character whose black
 *         box (bounding box) encompasses the specified point. The second element addresses the
 *         character whose leading point is the closest to the specified point in the line
 * @see #locateLine, #location
 */
pair<Index, Index> TextLayout::offset(const NativePoint& p, bool* outside /* = nullptr */) const /*throw()*/ {
	const bool vertical = isVertical(writingMode().blockFlowDirection);
	bool outsides[2];
	const std::pair<Index, Index> result(locateOffsets(locateLine(
		vertical ? geometry::x(p) : geometry::y(p), outsides[0]), vertical ? geometry::y(p) : geometry::x(p), outsides[1]));
	if(outside != nullptr)
		*outside = outsides[0] | outsides[1];
	return result;
}

/// Reorders the runs in visual order.
inline void TextLayout::reorder() /*throw()*/ {
	if(isEmpty())
		return;
	unique_ptr<TextRun*[]> temp(new TextRun*[numberOfRuns_]);
	copy(runs_.get(), runs_.get() + numberOfRuns_, temp.get());
	for(Index line = 0; line < numberOfLines(); ++line) {
		const size_t numberOfRunsInLine = ((line < numberOfLines() - 1) ?
			lineFirstRuns_[line + 1] : numberOfRuns_) - lineFirstRuns_[line];
		const unique_ptr<BYTE[]> levels(new BYTE[numberOfRunsInLine]);
		for(size_t i = 0; i < numberOfRunsInLine; ++i)
			levels[i] = static_cast<BYTE>(runs_[i + lineFirstRuns_[line]]->bidiEmbeddingLevel() & 0x1f);
		const unique_ptr<int[]> log2vis(new int[numberOfRunsInLine]);
		const HRESULT hr = ::ScriptLayout(static_cast<int>(numberOfRunsInLine), levels.get(), nullptr, log2vis.get());
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
	if(style_.get() != nullptr)
		result = style_->readingDirection;
	// try the default line style
	if(result == INHERIT_READING_DIRECTION) {
		shared_ptr<const TextLineStyle> defaultLineStyle(lip_.presentation().defaultTextLineStyle());
		if(defaultLineStyle.get() != nullptr)
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

/**
 * Stacks the line boxes and compute the line metrics.
 * @param lineStackingStrategy
 * @param nominalFont
 * @param lineHeight
 */
void TextLayout::stackLines(LineStackingStrategy lineStackingStrategy, const Font& nominalFont, Scalar lineHeight) {
	// TODO: this code is temporary. should rewrite later.
	// calculate allocation-rectangle of the lines according to line-stacking-strategy
	const Scalar textAltitude = nominalFont.metrics().ascent();
	const Scalar textDepth = nominalFont.metrics().descent();
	vector<pair<Scalar, Scalar>> v;
	for(Index line = 0; line < numberOfLines(); ++line) {
		// calculate extent of the line in block-progression-direction
		Scalar ascent, descent;
		switch(lineStackingStrategy) {
			case LINE_HEIGHT: {
				// allocation-rectangle of line is per-inline-height-rectangle
				Scalar leading = lineHeight - (textAltitude + textDepth);
				ascent = textAltitude + (leading - leading / 2);
				descent = textDepth + leading / 2;
				const TextRun* const lastRun = runs_[(line + 1 < numberOfLines()) ? lineFirstRuns_[line + 1] : numberOfRuns_];
				for(const TextRun* run = runs_[lineFirstRuns_[line]]; run != lastRun; ++run) {
					leading = lineHeight - nominalFont.metrics().cellHeight();
					ascent = max(run->font()->metrics().ascent() - (leading - leading / 2), ascent);
					descent = max(run->font()->metrics().descent() - leading / 2, descent);
				}
				break;
			}
			case FONT_HEIGHT:
				// allocation-rectangle of line is nominal-requested-line-rectangle
				ascent = textAltitude;
				descent = textDepth;
				break;
			case MAX_HEIGHT: {
				// allocation-rectangle of line is maximum-line-rectangle
				ascent = textAltitude;
				descent = textDepth;
				const TextRun* const lastRun = runs_[(line + 1 < numberOfLines()) ? lineFirstRuns_[line + 1] : numberOfRuns_];
				for(const TextRun* run = runs_[lineFirstRuns_[line]]; run != lastRun; ++run) {
					ascent = max(run->font()->metrics().ascent(), ascent);
					descent = max(run->font()->metrics().descent(), descent);
				}
				break;
			}
			default:
				ASCENSION_ASSERT_NOT_REACHED();
		}
		v.push_back(make_pair(ascent, descent));
	}

	lineMetrics_.reset(new LineMetrics*[numberOfLines()]);
	for(size_t line = 0; line != v.size(); ++line) {
		try {
			unique_ptr<SillyLineMetrics> lineMetrics(new SillyLineMetrics(v[line].first, v[line].second));
			lineMetrics_[line] = lineMetrics.release();
		} catch(...) {
			while(line > 0)
				delete lineMetrics_[--line];
			throw;
		}
	}
}

#if 0
/**
 * Returns the styled text run containing the specified offset in the line.
 * @param offsetInLine The offset in the line
 * @return the styled segment
 * @throw kernel#BadPositionException @a offsetInLine is greater than the length of the line
 */
StyledRun TextLayout::styledTextRun(Index offsetInLine) const {
	if(offsetInLine > text().length())
		throw kernel::BadPositionException(kernel::Position(INVALID_INDEX, offsetInLine));
	const TextRun& run = *runs_[findRunForPosition(offsetInLine)];
	return StyledRun(run.offsetInLine(), run.requestedStyle());
}
#endif

/// Locates the wrap points and resolves tab expansions.
void TextLayout::wrap(const TabExpander& tabExpander) /*throw()*/ {
	assert(!isEmpty() && wrappingMeasure_ != numeric_limits<Scalar>::max());
	assert(numberOfLines_ == 0 && lineOffsets_.get() == nullptr && lineFirstRuns_.get() == nullptr);

	vector<Index> lineFirstRuns;
	lineFirstRuns.push_back(0);
	int x1 = 0;	// addresses the beginning of the run. see x2
	unique_ptr<int[]> logicalWidths;
	unique_ptr<SCRIPT_LOGATTR[]> logicalAttributes;
	Index longestRunLength = 0;	// for efficient allocation
	vector<TextRun*> newRuns;
	newRuns.reserve(numberOfRuns_ * 3 / 2);
	// for each runs... (at this time, runs_ is in logical order)
	for(size_t i = 0; i < numberOfRuns_; ++i) {
		TextRun* run = runs_[i];

		// if the run is a tab, expand and calculate actual width
		if(run->expandTabCharacters(tabExpander, textString_,
				(x1 < wrappingMeasure_) ? x1 : 0, wrappingMeasure_ - (x1 < wrappingMeasure_) ? x1 : 0)) {
			if(x1 < wrappingMeasure_) {
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
		if(length(*run) > longestRunLength) {
			longestRunLength = length(*run);
			longestRunLength += 16 - longestRunLength % 16;
			logicalWidths.reset(new int[longestRunLength]);
			logicalAttributes.reset(new SCRIPT_LOGATTR[longestRunLength]);
		}
		HRESULT hr = run->logicalWidths(logicalWidths.get());
		hr = run->logicalAttributes(textString_, logicalAttributes.get());
		const Index originalRunPosition = run->beginning();
		int widthInThisRun = 0;
		Index lastBreakable = run->beginning(), lastGlyphEnd = run->beginning();
		int lastBreakableX = x1, lastGlyphEndX = x1;
		// for each characters in the run...
		for(Index j = run->beginning(); j < run->end(); ) {	// j is position in the LOGICAL line
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
			if(x2 + logicalWidths[j - originalRunPosition] > wrappingMeasure_) {
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
					if(lastBreakable < textString_.length()) {
						assert(lineFirstRuns.empty() || newRuns.size() != lineFirstRuns.back());
						lineFirstRuns.push_back(newRuns.size() + 1);
//dout << L"broke the line at " << lastBreakable << L" where the run end.\n";
					}
					break;
				}
				// case 3: break at the middle of the run -> split the run (run -> newRun + run)
				else {
					unique_ptr<TextRun> followingRun(run->breakAt(lastBreakable, textString_));
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
		newRuns.push_back(nullptr);
	runs_.reset(new TextRun*[numberOfRuns_ = newRuns.size()]);
	copy(newRuns.begin(), newRuns.end(), runs_.get());

	{
		assert(numberOfLines() > 1);
		unique_ptr<Index[]> temp(new Index[numberOfLines_ = lineFirstRuns.size()]);
		copy(lineFirstRuns.begin(), lineFirstRuns.end(), temp.get());
		lineFirstRuns_ = move(temp);
	}

	lineOffsets_ = move(unique_ptr<Index[]>(new Index[numberOfLines()]));
	for(size_t i = 0; i < numberOfLines(); ++i)
		const_cast<Index&>(lineOffsets_[i]) = runs_[lineFirstRuns_[i]]->beginning();
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
	return StyledRun(run.offsetInLine, run.style);
}
#endif
