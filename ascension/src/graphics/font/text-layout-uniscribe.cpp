/**
 * @file text-layout.cpp
 * @author exeal
 * @date 2003-2006 (was TextLayout.cpp)
 * @date 2006-2011
 * @date 2010-11-20 renamed from ascension/layout.cpp
 * @date 2012-09-01 separated from text-layout.cpp
 * @date 2012, 2014
 */

#include <ascension/config.hpp>	// ASCENSION_DEFAULT_LINE_LAYOUT_CACHE_SIZE, ...
#include <ascension/corelib/numeric-range-algorithm/includes.hpp>
#include <ascension/corelib/numeric-range-algorithm/intersection.hpp>
#include <ascension/corelib/numeric-range-algorithm/order.hpp>
#include <ascension/corelib/text/character-property.hpp>
#include <ascension/corelib/text/string-character-iterator.hpp>
#include <ascension/graphics/native-conversion.hpp>
#include <ascension/graphics/rendering-context.hpp>
#include <ascension/graphics/rendering-device.hpp>
#include <ascension/graphics/font/actual-text-styles.hpp>
#include <ascension/graphics/font/font.hpp>
#include <ascension/graphics/font/font-collection.hpp>
#include <ascension/graphics/font/font-metrics.hpp>
#include <ascension/graphics/font/font-render-context.hpp>
#include <ascension/graphics/font/glyph-metrics.hpp>
#include <ascension/graphics/font/line-rendering-options.hpp>
#include <ascension/graphics/font/tab-expander.hpp>
#include <ascension/graphics/font/text-layout.hpp>
#include <ascension/graphics/font/text-run.hpp>
#include <ascension/graphics/geometry/native-conversions.hpp>
#include <ascension/graphics/geometry/point-xy.hpp>
#include <ascension/graphics/geometry/rectangle-corners.hpp>
#include <ascension/graphics/geometry/rectangle-odxdy.hpp>
#include <ascension/graphics/geometry/rectangle-range.hpp>
#include <ascension/graphics/geometry/algorithms/make.hpp>
#include <ascension/graphics/geometry/algorithms/translate.hpp>
#include <ascension/log.hpp>
#include <ascension/presentation/styled-text-run-iterator.hpp>
#include <ascension/presentation/text-line-style.hpp>
#include <ascension/presentation/text-run-style.hpp>
#include <ascension/presentation/text-toplevel-style.hpp>
#include <ascension/presentation/writing-mode-mappings.hpp>
#include <boost/core/null_deleter.hpp>
#include <boost/dll/shared_library.hpp>
#include <boost/flyweight.hpp>
#include <boost/flyweight/key_value.hpp>
#include <boost/foreach.hpp>
#include <boost/range/algorithm/find.hpp>
#include <boost/range/algorithm/for_each.hpp>
#include <boost/range/numeric.hpp>	// boost.accumulate
#include <limits>	// std.numeric_limits
#include <numeric>	// std.accumulate
#include <tuple>
#include <usp10.h>
#ifdef _DEBUG
//#	define ASCENSION_TRACE_LAYOUT_CACHES
//#	define ASCENSION_DIAGNOSE_INHERENT_DRAWING
#endif

#pragma comment(lib, "usp10.lib")

namespace ascension {
	namespace graphics {
		namespace font {
			namespace {
				// SystemColors caches the system colors.
				class SystemColors {
				public:
					SystemColors() BOOST_NOEXCEPT {
						update();
					}
					COLORREF get(std::size_t index) const {
						assert(index >= 0 && index < std::tuple_size<decltype(values_)>::value);
						return values_[index];
					}
					COLORREF serve(const boost::optional<Color>& color, int index) const {
						return (color != boost::none) ? toNative<COLORREF>(boost::get(color)) : get(index);
					}
					void update() BOOST_NOEXCEPT {
						for(std::size_t i = 0; i < std::tuple_size<decltype(values_)>::value; ++i)
							values_[i] = ::GetSysColor(i);
					}
				private:
					std::array<COLORREF, 128> values_;
				} systemColors;

				const class ScriptProperties {
				public:
					ScriptProperties() BOOST_NOEXCEPT : p_(nullptr), c_(0) {
						::ScriptGetProperties(&p_, &c_);
					}
					const SCRIPT_PROPERTIES& get(int script) const {
						if(script >= c_)
							throw std::out_of_range("script");
						return *p_[script];
					}
					int numberOfOfScripts() const BOOST_NOEXCEPT {return c_;}
				private:
					const SCRIPT_PROPERTIES** p_;
					int c_;
				} scriptProperties;

				class UserSettings {
				public:
					UserSettings() BOOST_NOEXCEPT {update();}
					LANGID defaultLanguage() const BOOST_NOEXCEPT {return languageID_;}
					const SCRIPT_DIGITSUBSTITUTE& digitSubstitution(bool ignoreUserOverride) const BOOST_NOEXCEPT {
						return ignoreUserOverride ? digitSubstitutionNoUserOverride_ : digitSubstitution_;
					}
					void update() BOOST_NOEXCEPT {
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
				class Uniscribe16 {
				public:
					Uniscribe16() BOOST_NOEXCEPT : itemizeOpenType_(nullptr), placeOpenType_(nullptr), shapeOpenType_(nullptr), substituteSingleGlyph_(nullptr) {
						try {
							library_.load("usp10.dll", boost::dll::load_mode::search_system_folders);
							itemizeOpenType_ = library_.get<ItemizeOpenTypeSignature>("ScriptItemizeOpenType");
							placeOpenType_ = library_.get<PlaceOpenTypeSignature>("ScriptPlaceOpenType");
							shapeOpenType_ = library_.get<ShapeOpenTypeSignature>("ScriptShapeOpenType");
							substituteSingleGlyph_ = library_.get<SubstituteSingleGlyphSignature>("ScriptSubstituteSingleGlyph");
						} catch(...) {
							library_.unload();
						}
					}
					static Uniscribe16& instance() BOOST_NOEXCEPT {
						static Uniscribe16 singleton;
						return singleton;
					}
					HRESULT itemize(const WCHAR* text, int length, int estimatedNumberOfItems,
							const SCRIPT_CONTROL& control, const SCRIPT_STATE& initialState, SCRIPT_ITEM items[], OPENTYPE_TAG scriptTags[], int& numberOfItems) BOOST_NOEXCEPT {
						if(supportsOpenType() && scriptTags != nullptr)
							return (*itemizeOpenType_)(text, length, estimatedNumberOfItems, &control, &initialState, items, scriptTags, &numberOfItems);
						else
							return ::ScriptItemize(text, length, estimatedNumberOfItems, &control, &initialState, items, &numberOfItems);
					}
					HRESULT place();
					HRESULT shape();
					HRESULT substituteSingleGlyph();
					bool supportsOpenType() const BOOST_NOEXCEPT {
						return library_.is_loaded();
					}

				private:
					typedef HRESULT(WINAPI ItemizeOpenTypeSignature)(
						const WCHAR*, int, int, const SCRIPT_CONTROL*, const SCRIPT_STATE*, SCRIPT_ITEM*, OPENTYPE_TAG*, int*);
					typedef HRESULT(WINAPI PlaceOpenTypeSignature)(
						HDC, SCRIPT_CACHE*, SCRIPT_ANALYSIS*, OPENTYPE_TAG, OPENTYPE_TAG, int*,
						TEXTRANGE_PROPERTIES**, int, const WCHAR*, WORD*, SCRIPT_CHARPROP*, int, const WORD*,
						const SCRIPT_GLYPHPROP*, int, int*, GOFFSET*, ABC*);
					typedef HRESULT(WINAPI ShapeOpenTypeSignature)(
						HDC, SCRIPT_CACHE*, SCRIPT_ANALYSIS*, OPENTYPE_TAG, OPENTYPE_TAG, int*,
						TEXTRANGE_PROPERTIES**, int, const WCHAR*, int, int, WORD*, SCRIPT_CHARPROP*, WORD*,
						SCRIPT_GLYPHPROP*, int*);
//#ifdef ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
					typedef HRESULT(WINAPI SubstituteSingleGlyphSignature)(
						HDC, SCRIPT_CACHE*, SCRIPT_ANALYSIS*, OPENTYPE_TAG, OPENTYPE_TAG, OPENTYPE_TAG, LONG,
						WORD, WORD*);
//#endif // ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
					boost::dll::shared_library library_;
					ItemizeOpenTypeSignature* itemizeOpenType_;
					PlaceOpenTypeSignature* placeOpenType_;
					ShapeOpenTypeSignature* shapeOpenType_;
					SubstituteSingleGlyphSignature* substituteSingleGlyph_;
				};
			} // namespace @0

			// file-local free functions //////////////////////////////////////////////////////////////////////////////
			namespace {
				inline auto characterIndices(const TextRun& textRun, const String& textString) -> decltype(boost::irange<Index>(0, 0)) {
					return boost::irange<Index>(
						boost::const_begin(textRun.characterRange()) - textString.data(),
						boost::const_end(textRun.characterRange()) - textString.data());
				}

				void dumpRuns(const TextLayout& layout) {
#ifdef _DEBUG
					std::ostringstream s;
					layout.dumpRuns(s);
					::OutputDebugStringA(s.str().c_str());
#endif // _DEBUG
				}

				inline int estimateNumberOfGlyphs(Index length) BOOST_NOEXCEPT {
					return static_cast<int>(length) * 3 / 2 + 16;
				}

				LANGID userCJKLanguage() BOOST_NOEXCEPT;

				String fallback(int script) {
					using text::ucd::Script;
					if(script <= Script::FIRST_VALUE || script == Script::INHERITED
							|| script == Script::KATAKANA_OR_HIRAGANA || script >= Script::LAST_VALUE)
						throw UnknownValueException("script");

					static std::map<int, const String> associations;
					static const WCHAR MS_P_GOTHIC[] = L"\xff2d\xff33 \xff30\x30b4\x30b7\x30c3\x30af";	// "ＭＳ Ｐゴシック"
					if(boost::empty(associations)) {
						associations.insert(std::make_pair(Script::ARABIC, L"Microsoft Sans Serif"));
						associations.insert(std::make_pair(Script::CYRILLIC, L"Microsoft Sans Serif"));
						associations.insert(std::make_pair(Script::GREEK, L"Microsoft Sans Serif"));
						associations.insert(std::make_pair(Script::HANGUL, L"Gulim"));
						associations.insert(std::make_pair(Script::HEBREW, L"Microsoft Sans Serif"));
//						associations.insert(std::make_pair(Script::HIRAGANA, MS_P_GOTHIC));
//						associations.insert(std::make_pair(Script::KATAKANA, MS_P_GOTHIC));
						associations.insert(std::make_pair(Script::LATIN, L"Tahoma"));
						associations.insert(std::make_pair(Script::THAI, L"Tahoma"));
						// Windows 2000
						associations.insert(std::make_pair(Script::ARMENIAN, L"Sylfaen"));
						associations.insert(std::make_pair(Script::DEVANAGARI, L"Mangal"));
						associations.insert(std::make_pair(Script::GEORGIAN, L"Sylfaen"));	// partial support?
						associations.insert(std::make_pair(Script::TAMIL, L"Latha"));
						// Windows XP
						associations.insert(std::make_pair(Script::GUJARATI, L"Shruti"));
						associations.insert(std::make_pair(Script::GURMUKHI, L"Raavi"));
						associations.insert(std::make_pair(Script::KANNADA, L"Tunga"));
						associations.insert(std::make_pair(Script::SYRIAC, L"Estrangelo Edessa"));
						associations.insert(std::make_pair(Script::TELUGU, L"Gautami"));
						associations.insert(std::make_pair(Script::THAANA, L"MV Boli"));
						// Windows XP SP2
						associations.insert(std::make_pair(Script::BENGALI, L"Vrinda"));
						associations.insert(std::make_pair(Script::MALAYALAM, L"Kartika"));
						// Windows Vista
						associations.insert(std::make_pair(Script::CANADIAN_ABORIGINAL, L"Euphemia"));
						associations.insert(std::make_pair(Script::CHEROKEE, L"Plantagenet Cherokee"));
						associations.insert(std::make_pair(Script::ETHIOPIC, L"Nyala"));
						associations.insert(std::make_pair(Script::KHMER, L"DaunPenh"));	// or "MoolBoran"
						associations.insert(std::make_pair(Script::LAO, L"DokChampa"));
						associations.insert(std::make_pair(Script::MONGOLIAN, L"Mongolian Baiti"));
						associations.insert(std::make_pair(Script::ORIYA, L"Kalinga"));
						associations.insert(std::make_pair(Script::SINHALA, L"Iskoola Pota"));
						associations.insert(std::make_pair(Script::TIBETAN, L"Microsoft Himalaya"));
						associations.insert(std::make_pair(Script::YI, L"Microsoft Yi Baiti"));
						// CJK
						const LANGID uiLang = userCJKLanguage();
						switch(PRIMARYLANGID(uiLang)) {	// yes, this is not enough...
							case LANG_CHINESE:
								associations.insert(std::make_pair(Script::HAN, (SUBLANGID(uiLang) == SUBLANG_CHINESE_TRADITIONAL
									&& SUBLANGID(uiLang) == SUBLANG_CHINESE_HONGKONG) ? L"PMingLiu" : L"SimSun")); break;
							case LANG_JAPANESE:
								associations.insert(std::make_pair(Script::HAN, MS_P_GOTHIC)); break;
							case LANG_KOREAN:
								associations.insert(std::make_pair(Script::HAN, L"Gulim")); break;
							default:
								{
									win32::Handle<HDC>::Type dc(win32::detail::screenDC());
									bool installed = false;
									LOGFONTW lf;
									std::memset(&lf, 0, sizeof(LOGFONTW));
#define ASCENSION_SELECT_INSTALLED_FONT(charset, fontname)					\
	lf.lfCharSet = charset;													\
	std::wcscpy(lf.lfFaceName, fontname);									\
	::EnumFontFamiliesExW(dc.get(), &lf,									\
		reinterpret_cast<FONTENUMPROCW>(checkFontInstalled),				\
		reinterpret_cast<LPARAM>(&installed), 0);							\
	if(installed) {															\
		associations.insert(std::make_pair(Script::HAN, lf.lfFaceName));	\
		break;																\
	}
									ASCENSION_SELECT_INSTALLED_FONT(GB2312_CHARSET, L"SimSun")
									ASCENSION_SELECT_INSTALLED_FONT(SHIFTJIS_CHARSET, MS_P_GOTHIC)
									ASCENSION_SELECT_INSTALLED_FONT(HANGUL_CHARSET, L"Gulim")
									ASCENSION_SELECT_INSTALLED_FONT(CHINESEBIG5_CHARSET, L"PMingLiu")
#undef ASCENSION_SELECT_INSTALLED_FONT
							}
							break;
						}
						if(associations.find(Script::HAN) != boost::const_end(associations)) {
							associations.insert(make_pair(Script::HIRAGANA, associations[Script::HAN]));
							associations.insert(make_pair(Script::KATAKANA, associations[Script::HAN]));
						}
					}

					std::map<int, const String>::const_iterator i(associations.find(script));
					return (i != boost::const_end(associations)) ? i->second : String();
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
				bool getDecorationLineMetrics(const win32::Handle<HDC>::Type& dc, int* baselineOffset,
						int* underlineOffset, int* underlineThickness, int* strikethroughOffset, int* strikethroughThickness) BOOST_NOEXCEPT {
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

				inline bool isC0orC1Control(CodePoint c) BOOST_NOEXCEPT {
					return c < 0x20 || c == 0x7f || (c >= 0x80 && c < 0xa0);
				}

				inline void recordUserDefaultLocaleDigitSubstitution(SCRIPT_DIGITSUBSTITUTE& sds) {
					const HRESULT hr = ::ScriptRecordDigitSubstitution(LOCALE_USER_DEFAULT, &sds);
					if(FAILED(hr))
						throw makePlatformError(hr);
				}

				void convertNumberSubstitutionToUniscribe(const NumberSubstitution& from, SCRIPT_DIGITSUBSTITUTE& to) {
					boost::optional<win32::AutoZero<SCRIPT_DIGITSUBSTITUTE>> userLocale;
					switch(boost::native_value(from.localeSource)) {
						case NumberSubstitution::LocaleSource::TEXT:
						case NumberSubstitution::LocaleSource::USER: {
							// TODO: This code should not run frequently.
							userLocale = win32::AutoZero<SCRIPT_DIGITSUBSTITUTE>();
							recordUserDefaultLocaleDigitSubstitution(boost::get(userLocale));
							to = boost::get(userLocale);
							break;
						}
//						case NumberSubstitution::LocaleSource::OVERRIDE:
//							result.NationalDigitLanguage = result.TraditionalDigitLanguage = ????(from.localeOverride);
//							break;
						default:
							throw UnknownValueException("from.localeSource");
					}

					switch(boost::native_value(from.method)) {
						case NumberSubstitution::Method::AS_LOCALE:
							if(userLocale == boost::none) {
								userLocale = win32::AutoZero<SCRIPT_DIGITSUBSTITUTE>();
								recordUserDefaultLocaleDigitSubstitution(boost::get(userLocale));
							}
							to.DigitSubstitute = userLocale->DigitSubstitute;
							break;
						case NumberSubstitution::Method::CONTEXT:
							to.DigitSubstitute = SCRIPT_DIGITSUBSTITUTE_CONTEXT;
							break;
						case NumberSubstitution::Method::EUROPEAN:
							to.DigitSubstitute = SCRIPT_DIGITSUBSTITUTE_NONE;
							break;
						case NumberSubstitution::Method::NATIVE_NATIONAL:
							to.DigitSubstitute = SCRIPT_DIGITSUBSTITUTE_NATIONAL;
							break;
						case NumberSubstitution::Method::TRADITIONAL:
							to.DigitSubstitute = SCRIPT_DIGITSUBSTITUTE_TRADITIONAL;
							break;
						default:
							throw UnknownValueException("from.method");
					}

#ifdef ASCENSION_ABANDONED_AT_VERSION_08
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
#endif
				}

				inline DWORD localeIntrinsicDigitSubstitution(LCID locale) {
					DWORD n;
					if(::GetLocaleInfoW(locale, LOCALE_IDIGITSUBSTITUTION | LOCALE_RETURN_NUMBER, reinterpret_cast<LPWSTR>(&n), 2) == 0)
						throw makePlatformError();
					switch(n) {
						case 0:
							return SCRIPT_DIGITSUBSTITUTE_CONTEXT;
						case 1:
							return SCRIPT_DIGITSUBSTITUTE_NONE;
						case 2:
							return SCRIPT_DIGITSUBSTITUTE_NATIONAL;
						default:
							ASCENSION_ASSERT_NOT_REACHED();
					}
				}

				inline bool uniscribeSupportsIVS() BOOST_NOEXCEPT {
					static bool checked = false, supports = false;
					if(!checked) {
						static const std::basic_string<WCHAR> text(L"\x82a6\xdb40\xdd00");	// <芦, U+E0100>
						std::array<SCRIPT_ITEM, 4> items;
						int numberOfItems;
						if(SUCCEEDED(::ScriptItemize(text.c_str(), static_cast<int>(text.length()),
								boost::size(items), nullptr, nullptr, items.data(), &numberOfItems)) && numberOfItems == 1)
							supports = true;
						checked = true;
					}
					return supports;
				}

				LANGID userCJKLanguage() {
					// this code is preliminary...
					static const std::array<WORD, 3> CJK_LANGUAGES = {LANG_CHINESE, LANG_JAPANESE, LANG_KOREAN};	// sorted by numeric values
					LANGID result = win32::userDefaultUILanguage();
					if(boost::find(CJK_LANGUAGES, PRIMARYLANGID(result)) != boost::end(CJK_LANGUAGES))
						return result;
					result = ::GetUserDefaultLangID();
					if(boost::find(CJK_LANGUAGES, PRIMARYLANGID(result)) != boost::end(CJK_LANGUAGES))
						return result;
					result = ::GetSystemDefaultLangID();
					if(boost::find(CJK_LANGUAGES, PRIMARYLANGID(result)) != boost::end(CJK_LANGUAGES))
						return result;
					switch(::GetACP()) {
						case 932:
							return MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT);
						case 936:
							return MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED);
						case 949:
							return MAKELANGID(LANG_KOREAN, SUBLANG_KOREAN);
						case 950:
							return MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL);
					}
					return result;
				}
			} // namespace @0


			// graphics.font.* free functions /////////////////////////////////////////////////////////////////////////

			bool supportsComplexScripts() BOOST_NOEXCEPT {
				return true;
			}

			bool supportsOpenTypeFeatures() BOOST_NOEXCEPT {
				return Uniscribe16::instance().supportsOpenType();
			}


			// TextLayout.TextRun /////////////////////////////////////////////////////////////////////////////////////

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
				inline std::size_t characterPositionToGlyphPosition(const WORD clusters[],
						std::size_t length, std::size_t numberOfGlyphs, std::size_t at, const SCRIPT_ANALYSIS& a) {
					assert(clusters != nullptr);
					assert(at <= length);
					assert(a.fLogicalOrder == 0);
					if(a.fRTL == 0)	// LTR
						return (at < length) ? clusters[at] : numberOfGlyphs;
					else	// RTL
						return (at < length) ? clusters[at] + 1 : 0;
				}
#endif
				inline bool overhangs(const ABC& width) BOOST_NOEXCEPT {
					return width.abcA < 0 || width.abcC < 0;
				}

				struct LogicalCluster : boost::iterator_range<const WORD*> {
					boost::iterator_range<const WORD*> glyphs;
					explicit LogicalCluster(const boost::iterator_range<const WORD*>& base) : boost::iterator_range<const WORD*>(base) {
					}
					WORD glyphIndex() const BOOST_NOEXCEPT {
						return glyphs.front();
					}
				};

				class LogicalClusterIterator : public boost::iterators::iterator_facade<
					LogicalClusterIterator, LogicalCluster, boost::iterators::bidirectional_traversal_tag, LogicalCluster
				> {
				public:
					LogicalClusterIterator() : clusters_(nullptr, nullptr), glyphIndices_(nullptr, nullptr), current_(nullptr, nullptr) {
					}
					LogicalClusterIterator(const boost::iterator_range<const WORD*>& clusters,
							const boost::iterator_range<const WORD*>& glyphIndices, std::size_t position) : clusters_(clusters), glyphIndices_(glyphIndices) {
						if(boost::empty(clusters))
							throw std::invalid_argument("clusters");
						if(boost::empty(glyphIndices))
							throw std::invalid_argument("glyphIndices");
						if(position > static_cast<std::size_t>(boost::size(clusters_)))
							throw std::out_of_range("position");
						if(position < static_cast<std::size_t>(boost::size(clusters_))) {
							current_ = boost::make_iterator_range_n(boost::const_begin(clusters_) + position, 1);
							decrement();
							increment();
						} else
							current_ = boost::make_iterator_range_n(boost::const_end(clusters_), 0);
					}
					presentation::ReadingDirection readingDirection() const BOOST_NOEXCEPT {
						return readingDirection(clusters_);
					}
					static presentation::ReadingDirection readingDirection(const boost::iterator_range<const WORD*>& clusters) {
						if(boost::const_begin(clusters) == nullptr || boost::const_end(clusters) == nullptr)
							throw NullPointerException("clusters");
						if(boost::const_begin(clusters) >= boost::const_end(clusters))
							throw std::invalid_argument("clusters");
						return (clusters.front() <= clusters.back()) ? presentation::LEFT_TO_RIGHT : presentation::RIGHT_TO_LEFT;
					}

				private:
					void assertNotDone() const BOOST_NOEXCEPT {
						assert(!boost::empty(current_));
					}
					void assertNotInvalid() const BOOST_NOEXCEPT {
						assert(!boost::empty(glyphIndices_) && boost::const_begin(glyphIndices_) != nullptr && boost::const_end(glyphIndices_) != nullptr);
					}
					void decrement() {
						assert(boost::const_end(current_) > boost::const_begin(clusters_));
						if(boost::const_begin(current_) == boost::const_begin(clusters_)) {
							current_ = boost::make_iterator_range(boost::const_begin(clusters_), boost::const_begin(clusters_));
							return;
						}
						auto previous(boost::make_iterator_range_n(boost::const_begin(current_) - 1, 1));
						for(; boost::const_begin(previous) > boost::const_begin(clusters_) && previous.front() == previous.back(); previous.advance_begin(-1));
						current_ = previous;
					}
					value_type dereference() const BOOST_NOEXCEPT {
						assertNotInvalid();
						assertNotDone();
						LogicalCluster lc(current_);
						if(readingDirection() == presentation::LEFT_TO_RIGHT) {
							const WORD nextGlyph = (boost::const_end(current_) < boost::const_end(clusters_)) ? *boost::const_end(current_) : boost::size(glyphIndices_);
							lc.glyphs = boost::make_iterator_range(boost::const_begin(glyphIndices_) + current_.front(), boost::const_begin(glyphIndices_) + nextGlyph);
						} else {
							const WORD first = (boost::const_end(current_) < boost::const_end(clusters_)) ? *boost::const_end(current_) + 1 : 0;
							lc.glyphs = boost::make_iterator_range(boost::const_begin(glyphIndices_) + first, boost::const_begin(glyphIndices_) + current_.front() + 1);
						}
						return lc;
					}
					bool equal(const LogicalClusterIterator& other) const BOOST_NOEXCEPT {
						return current_ == other.current_
							|| (boost::empty(current_) && boost::const_begin(other.current_) == nullptr)
							|| (boost::const_begin(current_) == nullptr && boost::empty(other.current_));
					}
					void increment() {
						assert(boost::const_begin(current_) < boost::const_end(clusters_));
						if(boost::const_end(current_) == boost::const_end(clusters_)) {
							current_ = boost::make_iterator_range_n(boost::const_end(clusters_), 0);
							return;
						}
						auto next(boost::make_iterator_range_n(boost::const_end(current_), 0));
						for(; boost::const_end(next) < boost::const_end(clusters_) && *boost::const_end(next) == *boost::const_begin(next); next.advance_end(+1));
						current_ = next;
					}
					const boost::iterator_range<const WORD*> clusters_, glyphIndices_;
					boost::iterator_range<const WORD*> current_;
					friend class boost::iterators::iterator_core_access;
				};
			} // namespace @0

			namespace {
				// bad ideas :(
				template<typename T>
				inline void raiseIfNull(T* p, const char parameterName[]) {
					if(p == nullptr)
						throw NullPointerException(parameterName);
				}
				inline void raiseIfNullOrEmpty(const StringPiece& textString, const char parameterName[]) {
					if(boost::const_begin(textString) == nullptr)
						throw NullPointerException(parameterName);
					else if(boost::empty(textString))
						throw std::invalid_argument(parameterName);
				}

				/// @internal A character range with the @a Attribute.
				template<typename Attribute>
				struct AttributedCharacterRange {
					StringPiece::const_iterator position;
					Attribute attribute;
					AttributedCharacterRange() {}
					AttributedCharacterRange(StringPiece::const_iterator position,
						const Attribute& attribute) : position(position), attribute(attribute) {}
#if defined(BOOST_COMP_MSVC) && 0
					AttributedCharacterRange& operator=(AttributedCharacterRange&& other) BOOST_NOEXCEPT {
						position = other.position;
						attribute = std::move(other.attribute);
						return *this;
					}
#endif
				};

				template<typename T, std::size_t staticCapacity>
				class AutoArray {
				public:
					typedef T ElementType;
					static const std::size_t STATIC_CAPACITY = staticCapacity;
				public:
					AutoArray() : capacity_(STATIC_CAPACITY) {
					}
					ElementType& operator[](std::size_t i) {
						return p_[i];
					}
					const ElementType& operator[](std::size_t i) const {
						return p_[i];
					}
					ElementType& at(std::size_t i) {
						if(i >= capacity_)
							throw out_of_range("i");
						return operator[](i);
					}
					const ElementType& at(std::size_t i) const {
						if(i >= capacity_)
							throw out_of_range("i");
						return operator[](i);
					}
					ElementType* get() const {
						return p_;
					}
					void reallocate(std::size_t n) {
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
					std::unique_ptr<ElementType[]> allocated_;
					std::size_t capacity_;
					ElementType* p_;
				};
			}

			 
			// GlyphVectorImpl file-local class ///////////////////////////////////////////////////////////////////////

			namespace {
				class GlyphVectorImpl : public TextRun, public StringPiece {
				public:
					GlyphVectorImpl(const StringPiece& characterRange, const SCRIPT_ANALYSIS& script,
						std::shared_ptr<const Font> font, const FontRenderContext& frc, OpenTypeLayoutTag scriptTag);

					// GlyphVector
					void fillGlyphs(PaintContext& context, const Point& origin/*,
						boost::optional<boost::integer_range<std::size_t>> range = boost::none*/) const override;
					std::shared_ptr<const Font> font() const BOOST_NOEXCEPT override;
					const FontRenderContext& fontRenderContext() const override;
					Index glyphCharacterIndex(std::size_t index) const override;
					GlyphCode glyphCode(std::size_t index) const override;
					graphics::Rectangle glyphLogicalBounds(std::size_t index) const override;
					GlyphMetrics glyphMetrics(std::size_t index) const override;
					Point glyphPosition(std::size_t index) const override;
					void glyphPositions(const boost::integer_range<std::size_t>& range, std::vector<Point>& out) const override;
					graphics::Rectangle glyphVisualBounds(std::size_t index) const override;
					graphics::Rectangle logicalBounds() const override;
					std::size_t numberOfGlyphs() const BOOST_NOEXCEPT override;
					void setGlyphPosition(std::size_t index, const Point& position) override;
					void strokeGlyphs(PaintContext& context, const Point& origin/*,
						boost::optional<boost::integer_range<std::size_t>> range = boost::none*/) const override;
					graphics::Rectangle visualBounds() const override;
					// TextRun
					const presentation::FlowRelativeFourSides<ActualBorderSide>* border() const BOOST_NOEXCEPT override {return nullptr;}
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
					boost::optional<Index> characterEncompassesPosition(float ipd) const BOOST_NOEXCEPT;
					Index characterHasClosestLeadingEdge(float ipd) const;
#endif // ASCENSION_ABANDONED_AT_VERSION_08
					std::uint8_t characterLevel() const BOOST_NOEXCEPT override;
					StringPiece characterRange() const BOOST_NOEXCEPT override;
					TextHit<> hitTestCharacter(Scalar ipd, const boost::optional<NumericRange<Scalar>>& bounds, bool* outOfBounds) const override;
					Scalar hitToLogicalPosition(const TextHit<>& hit) const override;
					const presentation::FlowRelativeFourSides<Scalar>* margin() const BOOST_NOEXCEPT override {return nullptr;}
					const presentation::FlowRelativeFourSides<Scalar>* padding() const BOOST_NOEXCEPT override {return nullptr;}
					// attributes
					HRESULT logicalAttributes(SCRIPT_LOGATTR attributes[]) const;
					// geometry
					void charactersBounds(const boost::integer_range<Index>& characterRange, std::vector<graphics::Rectangle>& result) const;
					HRESULT logicalWidths(int widths[]) const;
//					int totalAdvance() const BOOST_NOEXCEPT {return boost::accumulate(advances(), 0);}
					// layout
					std::unique_ptr<GlyphVectorImpl> breakAt(StringPiece::const_iterator at);
					std::unique_ptr<GlyphVectorImpl> breakIfTooLong();
					bool expandTabCharacters(const RenderingContext2D& context,
						const presentation::styles::ComputedValue<presentation::styles::TabSize>::type& tabSize,
						const presentation::styles::Length::Context& lengthContext,
						const String& layoutString, Scalar ipd, boost::optional<Scalar> maximumMeasure);
					HRESULT justify(int width);
					void shape(win32::Handle<HDC>::Type dc);
					void positionGlyphs(win32::Handle<HDC>::Type dc);
					void reserveJustification();
					template<typename SinglePassReadableRange>
					static void substituteGlyphs(const SinglePassReadableRange& runs);
					// drawing and painting
					void drawGlyphs(PaintContext& context, const Point& p, const boost::integer_range<Index>& range) const;

				protected:
					// this data is shared text runs separated by (only) line breaks and computed styles
					struct RawGlyphVector /*: public StringPiece*/ : private boost::noncopyable {
						StringPiece::const_iterator position;
						boost::flyweight<FontAndRenderContext> font;
						OpenTypeLayoutTag scriptTag;	// as OPENTYPE_TAG
						mutable SCRIPT_CACHE fontCache;
						// only 'clusters' is character-base. others are glyph-base
						std::unique_ptr<WORD[]> indices, clusters;
						std::unique_ptr<SCRIPT_VISATTR[]> visualAttributes;
						std::unique_ptr<int[]> advances, justifiedAdvances;
						std::unique_ptr<GOFFSET[]> offsets;
						RawGlyphVector(StringPiece::const_iterator position, std::shared_ptr<const Font> font, const FontRenderContext& frc,
								OpenTypeLayoutTag scriptTag) : position(position), font(font, frc), scriptTag(scriptTag), fontCache(nullptr) {
							raiseIfNull(position, "position");
							raiseIfNull(font.get(), "font");
						}
						RawGlyphVector(RawGlyphVector&& other) BOOST_NOEXCEPT;
						RawGlyphVector& operator=(RawGlyphVector&& other) BOOST_NOEXCEPT;
						~RawGlyphVector() BOOST_NOEXCEPT {::ScriptFreeCache(&fontCache);}
						void vanish(const Font& font, StringPiece::const_iterator at);
					};
				protected:
					GlyphVectorImpl(const StringPiece& characterRange, const SCRIPT_ANALYSIS& script, std::unique_ptr<RawGlyphVector> glyphs);
					GlyphVectorImpl(GlyphVectorImpl& leading, StringPiece::const_iterator beginningOfNewRun);
					boost::iterator_range<const int*> advances() const BOOST_NOEXCEPT {
						if(const int* const p = glyphs_->advances.get()) {
							const auto range(glyphRange());
							return boost::make_iterator_range(p + *boost::const_begin(range), p + *boost::const_end(range));
						}
						return boost::make_iterator_range<const int*>(nullptr, nullptr);
					}
					WORD cluster(StringPiece::const_iterator at) const {
						if(at < boost::const_begin(*this) || at >= boost::const_end(*this))
							throw IndexOutOfBoundsException("at");
						return (clusters()[at - boost::const_begin(*this)] + clusterOffset_) & 0xffffu;
					}
					WORD cluster(std::size_t at) const {
						if(at >= length())
							throw IndexOutOfBoundsException("at");
						return clusters()[at] + clusterOffset_;
					}
					boost::iterator_range<const WORD*> clusters() const BOOST_NOEXCEPT {
						if(const WORD* const p = glyphs_->clusters.get())
							return boost::make_iterator_range_n(p + (boost::const_begin(*this) - glyphs_->position), boost::size(*this));
						return boost::make_iterator_range<const WORD*>(nullptr, nullptr);
					}
					std::size_t countMissingGlyphs(const RenderingContext2D& context) const;
					boost::iterator_range<const int*> effectiveAdvances() const BOOST_NOEXCEPT {
						if(justified_) {
							const int* const p = glyphs_->justifiedAdvances.get();
							assert(p != nullptr);
							const auto range(glyphRange());
							return boost::make_iterator_range(p + *boost::const_begin(range), p + *boost::const_end(range));
						} else if(const int* const p = glyphs_->advances.get()) {
							const auto range(glyphRange());
							return boost::make_iterator_range(p + *boost::const_begin(range), p + *boost::const_end(range));
						}
						return boost::make_iterator_range<const int*>(nullptr, nullptr);
					}
					static std::size_t generateDefaultGlyphs(win32::Handle<HDC>::Type dc,
						const StringPiece& text, const SCRIPT_ANALYSIS& analysis, RawGlyphVector& glyphs);
					static std::pair<std::size_t, HRESULT> generateGlyphs(win32::Handle<HDC>::Type dc,
						const StringPiece& text, const SCRIPT_ANALYSIS& analysis, RawGlyphVector& glyphs);
					Scalar glyphLogicalPosition(std::size_t index) const;
					boost::iterator_range<const WORD*> glyphs() const BOOST_NOEXCEPT {
						if(const WORD* const p = glyphs_->indices.get()) {
							const auto range(glyphRange());
							return boost::make_iterator_range(p + *boost::const_begin(range), p + *boost::const_end(range));
						}
						return boost::make_iterator_range<const WORD*>(nullptr, nullptr);
					}
					boost::iterator_range<const GOFFSET*> glyphOffsets() const BOOST_NOEXCEPT {
						if(const GOFFSET* const p = glyphs_->offsets.get()) {
							const auto range(glyphRange());
							return boost::make_iterator_range(p + *boost::const_begin(range), p + *boost::const_end(range));
						}
						return boost::make_iterator_range<const GOFFSET*>(nullptr, nullptr);
					}
					boost::integer_range<std::size_t> glyphRange(const StringPiece& characterRange = StringPiece()) const;
#if 0
					void hitTest(Scalar ipd, int& encompasses, int* trailing) const;
#endif
					boost::iterator_range<const int*> justifiedAdvances() const BOOST_NOEXCEPT {
						if(justified_) {
							const int* const p = glyphs_->justifiedAdvances.get();
							assert(p != nullptr);
							const auto range(glyphRange());
							return boost::make_iterator_range(p + *boost::const_begin(range), p + *boost::const_end(range));
						}
						return boost::make_iterator_range<const int*>(nullptr, nullptr);
					}
					NumericRange<Scalar> logicalExtents() const {
						RenderingContext2D context(win32::detail::screenDC());
						std::unique_ptr<const FontMetrics<Scalar>> fm(context.fontMetrics(font()));
						const double sy = geometry::scaleY(fontRenderContext().transform()) / geometry::scaleY(context.fontRenderContext().transform());
						return boost::irange(-static_cast<Scalar>(fm->ascent() * sy), static_cast<Scalar>(fm->descent() * sy + fm->internalLeading() * sy));
					}
					void paintGlyphs(PaintContext& context, const Point& origin/*,
						boost::optional<boost::integer_range<std::size_t>> range*/, bool onlyStroke) const;
					boost::iterator_range<const SCRIPT_VISATTR*> visualAttributes() const BOOST_NOEXCEPT {
						if(const SCRIPT_VISATTR* const p = glyphs_->visualAttributes.get()) {
							const auto range(glyphRange());
							return boost::make_iterator_range(p + *boost::const_begin(range), p + *boost::const_end(range));
						}
						return boost::make_iterator_range<const SCRIPT_VISATTR*>(nullptr, nullptr);
					}

				private:
					SCRIPT_ANALYSIS analysis_;	// fLogicalOrder member is always 0 (however see shape())
					std::shared_ptr<RawGlyphVector> glyphs_;
					std::size_t numberOfGlyphs_ : 31;
					bool justified_ : 1;
					WORD clusterOffset_;	// see copy-constructor
				};

				GlyphVectorImpl::RawGlyphVector::RawGlyphVector(RawGlyphVector&& other) BOOST_NOEXCEPT :
						position(other.position), font(std::move(other.font)), scriptTag(other.scriptTag),
						fontCache(other.fontCache), indices(std::move(other.indices)), clusters(std::move(other.clusters)),
						visualAttributes(std::move(other.visualAttributes)), justifiedAdvances(std::move(other.justifiedAdvances)),
						offsets(std::move(other.offsets)) {
					other.fontCache = nullptr;
				}

				GlyphVectorImpl::RawGlyphVector& GlyphVectorImpl::RawGlyphVector::operator=(RawGlyphVector&& other) BOOST_NOEXCEPT {
					position = other.position;
					font = std::move(other.font);
					scriptTag = other.scriptTag;
					fontCache = other.fontCache;
					indices = std::move(other.indices);
					clusters = std::move(other.clusters);
					visualAttributes = std::move(other.visualAttributes);
					justifiedAdvances = std::move(other.justifiedAdvances);
					offsets = std::move(other.offsets);
					return *this;
				}

				void GlyphVectorImpl::RawGlyphVector::vanish(const Font& font, StringPiece::const_iterator at) {
					assert(advances.get() == nullptr);
					assert(at != nullptr);
					assert(at >= position);
					win32::Handle<HDC>::Type dc(win32::detail::screenDC());
					HFONT oldFont = nullptr;
					WORD blankGlyph;
					HRESULT hr = ::ScriptGetCMap(dc.get(), &fontCache, L"\x0020", 1, 0, &blankGlyph);
					if(hr == E_PENDING) {
						oldFont = static_cast<HFONT>(::SelectObject(dc.get(), font.native().get()));
						hr = ::ScriptGetCMap(dc.get(), &fontCache, L"\x0020", 1, 0, &blankGlyph);
					}
					if(hr == S_OK) {
						SCRIPT_FONTPROPERTIES fp;
						fp.cBytes = sizeof(SCRIPT_FONTPROPERTIES);
						if(FAILED(hr = ::ScriptGetFontProperties(dc.get(), &fontCache, &fp)))
							fp.wgBlank = 0;	// hmm...
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
				 * Creates a @c GlyphVectorImpl instance with a text string, script information and font rendering context.
				 * @param characterRange The string this text run covers
				 * @param script @c SCRIPT_ANALYSIS The object obtained by @c ScriptItemize(OpenType)
				 * @param font The font renders this text run. Can't be @c null
				 * @param scriptTag An OpenType script tag describes the script of this text run
				 * @throw NullPointerException @a characterRange and/or @a font are @c null
				 * @throw std#invalid_argument @a characterRange is empty
				 * @note This constructor is called by only @c #breakIfTooLong.
				 */
				GlyphVectorImpl::GlyphVectorImpl(const StringPiece& characterRange, const SCRIPT_ANALYSIS& script,
						std::shared_ptr<const Font> font, const FontRenderContext& frc, OpenTypeLayoutTag scriptTag)
						: StringPiece(characterRange), analysis_(script),
						glyphs_(new RawGlyphVector(boost::const_begin(characterRange), font, frc, scriptTag)),	// may throw NullPointerException for 'font'
						justified_(false), clusterOffset_(0) {
					raiseIfNullOrEmpty(characterRange, "characterRange");
//					raiseIfNull(font.get(), "font");
				}

				/**
				 * Creates a @c GlyphVectorImpl instance with a text string, script information and a computed glyph
				 * vector.
				 * @param characterRange The string this text run covers
				 * @param script @c SCRIPT_ANALYSIS The object obtained by @c ScriptItemize(OpenType)
				 * @param glyphs The glyph vector
				 * @throw NullPointerException @a characterRange and/or @a glyphs are @c null
				 * @throw std#invalid_argument @a characterRange is empty
				 * @note This constructor is called by only @c #generate.
				 */
				GlyphVectorImpl::GlyphVectorImpl(const StringPiece& characterRange, const SCRIPT_ANALYSIS& script, std::unique_ptr<RawGlyphVector> glyphs)
						: StringPiece(characterRange), analysis_(script), glyphs_(std::move(glyphs)), justified_(false), clusterOffset_(0) {
					raiseIfNullOrEmpty(characterRange, "characterRange");
					raiseIfNull(glyphs_.get(), "glyphs");
				}

				/**
				 * Creates a @c GlyphVectorImpl instance by breaking an existing one.
				 * @param leading The original @c GlyphVectorImpl object
				 * @param beginningOfNewRun The beginning position of the newly created vector
				 * @throw std#invalid_argument @a leading has not been shaped
				 * @throw std#invalid_argument @a beginningOfNewRun intervenes a glyph
				 * @throw NullPointerException @a beginningOfNewRun is @c null
				 * @throw std#out_of_range @a beginningOfNewRun is outside of the character range @a leading covers
				 * @see #breakAt, breakIfTooLong
				 * @note This constructor is called by only @c #breakAt.
				 */
				GlyphVectorImpl::GlyphVectorImpl(GlyphVectorImpl& leading, StringPiece::const_iterator beginningOfNewRun) :
						StringPiece(makeStringPiece(beginningOfNewRun, boost::const_end(leading))), analysis_(leading.analysis_), glyphs_(leading.glyphs_), justified_(false) {
					if(leading.glyphs_.get() == nullptr)
						throw std::invalid_argument("'leading' has not been shaped");
					raiseIfNull(beginningOfNewRun, "beginningOfNewRun");
					if(beginningOfNewRun <= boost::const_begin(leading) || beginningOfNewRun >= boost::const_end(leading))
						throw std::out_of_range("beginningOfNewRun");
					else if(leading.cluster(beginningOfNewRun) == leading.cluster(beginningOfNewRun - 1))
						throw std::invalid_argument("beginningOfNewRun");

					// offset values in 'glyphs_->clusters' array
					const WORD newNumberOfGlyphsOfLeading = (analysis_.fRTL == 0) ? *boost::const_begin(clusters()) : *(boost::const_end(clusters()) - 1);
					boost::for_each(
						boost::make_iterator_range_n(
							glyphs_->clusters.get() + std::distance(glyphs_->position, boost::const_begin(*this)),
							length()
						),
						[newNumberOfGlyphsOfLeading](WORD& cluster) {
							cluster -= newNumberOfGlyphsOfLeading;
					});
					clusterOffset_ = leading.clusterOffset_ + newNumberOfGlyphsOfLeading;
					numberOfGlyphs_ = leading.numberOfGlyphs_ - newNumberOfGlyphsOfLeading;

					// chop 'leading' at 'beginningOfNewRun'
					leading.remove_suffix(std::distance(beginningOfNewRun, boost::const_end(leading)));
					leading.numberOfGlyphs_ = newNumberOfGlyphsOfLeading;
				}

				/**
				 * Breaks this vector into two vectors at the specified position.
				 * @param at The position at which break this vector
				 * @return The new vector following this one
				 * @throw ... Any exception thrown by the pseudo copy-constructor
				 * @note This method is called by only @c #wrap.
				 */
				std::unique_ptr<GlyphVectorImpl> GlyphVectorImpl::breakAt(StringPiece::const_iterator at) {
					assert(direction() == presentation::LEFT_TO_RIGHT == (analysis_.fRTL == 0));

					// create the new following run
					return std::unique_ptr<GlyphVectorImpl>(new GlyphVectorImpl(*this, at));
				}

				std::unique_ptr<GlyphVectorImpl> GlyphVectorImpl::breakIfTooLong() {
					if(estimateNumberOfGlyphs(length()) <= 65535)
						return std::unique_ptr<GlyphVectorImpl>();

					// split this run, because the length would cause ScriptShape to fail (see also Mozilla bug 366643).
					static const Index MAXIMUM_RUN_LENGTH = 43680;	// estimateNumberOfGlyphs(43680) == 65536
					Index opportunity = 0;
					std::unique_ptr<SCRIPT_LOGATTR[]> la(new SCRIPT_LOGATTR[length()]);
					const HRESULT hr = logicalAttributes(la.get());
					if(SUCCEEDED(hr)) {
						for(Index i = MAXIMUM_RUN_LENGTH; i > 0; --i) {
							if(la[i].fCharStop != 0) {
								if(text::ucd::legacyctype::isspace((*this)[i]) || text::ucd::legacyctype::isspace((*this)[i - 1])) {
									opportunity = i;
									break;
								}
								opportunity = std::max(i, opportunity);
							}
						}
					}
					if(opportunity == 0) {
						opportunity = MAXIMUM_RUN_LENGTH;
						if(text::surrogates::isLowSurrogate((*this)[opportunity]) && text::surrogates::isHighSurrogate((*this)[opportunity - 1]))
							--opportunity;
					}

					StringPiece followingRange(*this);
					followingRange.remove_prefix(opportunity);
					std::unique_ptr<GlyphVectorImpl> following(new GlyphVectorImpl(
						followingRange, analysis_, glyphs_->font.get().font(), glyphs_->font.get().fontRenderContext(), glyphs_->scriptTag));
					static_cast<StringPiece&>(*this) = StringPiece(boost::const_begin(*this), opportunity);
					analysis_.fLinkAfter = following->analysis_.fLinkBefore = 0;
					return following;
				}

				/// @see TextRun#characterLevel
				std::uint8_t GlyphVectorImpl::characterLevel() const BOOST_NOEXCEPT {
					return static_cast<std::uint8_t>(analysis_.s.uBidiLevel);
				}

				void GlyphVectorImpl::charactersBounds(const boost::integer_range<Index>& characterRange, std::vector<graphics::Rectangle>& result) const {
					if(boost::empty(characterRange)) {
						result.clear();
						return;
					}
					// 'characterRange' are offsets from the beginning of this vector

					// measure glyph black box bounds
					const boost::iterator_range<const WORD*> glyphIndices(glyphs());
					const boost::iterator_range<const int*> glyphAdvances(effectiveAdvances());
					const bool rtl = LogicalClusterIterator::readingDirection(clusters()) == presentation::RIGHT_TO_LEFT;
					LogicalClusterIterator cluster(clusters(), glyphs(), !rtl ? characterRange.front() : characterRange.back());
					Scalar x = 0;
					for(std::size_t i = 0, firstGlyph = boost::const_begin(cluster->glyphs) - boost::const_begin(glyphIndices); i < firstGlyph; ++i)
						x += glyphAdvances[i];

					std::vector<graphics::Rectangle> bounds;
					RenderingContext2D context(win32::detail::screenDC());
					context.save();
					context.setFont(font());
					const MAT2 matrix = {1, 0, 0, 1};	// TODO: Consider glyph transform.
					const auto sx = geometry::scaleX(fontRenderContext().transform()) / geometry::scaleX(context.fontRenderContext().transform());
					const auto sy = geometry::scaleY(fontRenderContext().transform()) / geometry::scaleY(context.fontRenderContext().transform());
					DWORD lastError = ERROR_SUCCESS;
					const boost::iterator_range<const GOFFSET*> glyphOffsets2D(glyphOffsets());
					for(const LogicalClusterIterator e; cluster != e; !rtl ? ++cluster : --cluster) {
						Scalar left = std::numeric_limits<Scalar>::max(), top = std::numeric_limits<Scalar>::max(),
							right = std::numeric_limits<Scalar>::lowest(), bottom = std::numeric_limits<Scalar>::lowest();
						const auto glyphRange(cluster->glyphs);
						for(std::size_t i = 0; i < static_cast<std::size_t>(boost::size(glyphRange)); ++i, x += glyphAdvances[i]) {
							GLYPHMETRICS gm;
							if(GDI_ERROR == ::GetGlyphOutlineW(context.native().get(), glyphRange[i], GGO_GLYPH_INDEX | GGO_METRICS, &gm, 0, nullptr, &matrix)) {
								lastError = ::GetLastError();
								break;
							}
							left = std::min(x - static_cast<Scalar>(gm.gmptGlyphOrigin.x * sx) + glyphOffsets2D[i].du, left);
							top = std::min(0 - static_cast<Scalar>(gm.gmptGlyphOrigin.y * sy) + glyphOffsets2D[i].dv, top);
							right = std::max(x + static_cast<Scalar>(gm.gmBlackBoxX * sx) + glyphOffsets2D[i].du, right);
							bottom = std::max(0 + static_cast<Scalar>(gm.gmBlackBoxY * sy) + glyphOffsets2D[i].dv, bottom);
						}
						bounds.push_back(graphics::geometry::make<graphics::Rectangle>((
							geometry::_left = left, geometry::_top = top, geometry::_right = right, geometry::_bottom = bottom)));
					}
					context.restore();
					if(lastError != ERROR_SUCCESS)
						throw makePlatformError(lastError);
					using std::swap;
					return swap(bounds, result);
				}

				/// @see TextRun#characterRange
				inline StringPiece GlyphVectorImpl::characterRange() const BOOST_NOEXCEPT {
					return *this;
				}

				/**
				 * Returns the number of missing glyphs in this vector.
				 * @param context The graphics context
				 * @return The number of missing glyphs
				 * @throw PlatformError
				 */
				inline std::size_t GlyphVectorImpl::countMissingGlyphs(const RenderingContext2D& context) const {
					SCRIPT_FONTPROPERTIES fp;
					fp.cBytes = sizeof(decltype(fp));
					const HRESULT hr = ::ScriptGetFontProperties(context.native().get(), &glyphs_->fontCache, &fp);
					if(FAILED(hr))
						throw makePlatformError(hr);	// can't handle
					// following is not offical way, but from Mozilla (gfxWindowsFonts.cpp)
					std::size_t c = 0;
					for(text::StringCharacterIterator i(*this); i.hasNext(); ++i) {
						if(!text::ucd::BinaryProperty::is<text::ucd::BinaryProperty::DEFAULT_IGNORABLE_CODE_POINT>(*i)) {
							const WORD glyph = glyphs_->indices[cluster(i.tell())];
							if(glyph == fp.wgDefault || (glyph == fp.wgInvalid && glyph != fp.wgBlank))
								++c;
							else if(glyphs_->visualAttributes[i.tell() - i.beginning()].fZeroWidth == 1
									&& scriptProperties.get(analysis_.eScript).fComplex == 0)
								++c;
						}
					}
					return c;
				}

				namespace {
					typedef boost::flyweight<boost::flyweights::key_value<Scalar, FixedWidthTabExpander<Scalar>>> FlyweightTabExpander;
					FlyweightTabExpander makeFixedWidthTabExpander(
							const presentation::styles::ComputedValue<presentation::styles::TabSize>::type& computedValue,
							const FontMetrics<Scalar>& fontMetrics, const presentation::styles::Length::Context& lengthContext) {
						Scalar tabWidth;
						if(const presentation::styles::Integer* const integer = boost::get<presentation::styles::Integer>(&computedValue))
							tabWidth = fontMetrics.averageCharacterWidth() * *integer;
						else if(const presentation::styles::Length* const length = boost::get<presentation::styles::Length>(&computedValue))
							tabWidth = length->value(lengthContext);
						else
							ASCENSION_ASSERT_NOT_REACHED();
						return FlyweightTabExpander(tabWidth);
					}
				}

				/**
				 * Expands tab characters in this vector and modifies the measure (advance).
				 * @param context The rendering context
				 * @param tabSize The computed value of 'tab-size' style property
				 * @param lengthContext The @c Length#Context used to calculate @a tabSize
				 * @param layoutString The text string for the layout to which this text run belongs
				 * @param ipd The position in writing direction this text run begins, in pixels
				 * @param maximumMeasure The maximum measure this text run can take place, in pixels
				 * @return @c true if expanded tab characters
				 * @throw std#invalid_argument @a maximumMeasure &lt;= 0
				 */
				inline bool GlyphVectorImpl::expandTabCharacters(const RenderingContext2D& context,
						const presentation::styles::ComputedValue<presentation::styles::TabSize>::type& tabSize,
						const presentation::styles::Length::Context& lengthContext,
						const String& layoutString, Scalar ipd, boost::optional<Scalar> maximumMeasure) {
					if(maximumMeasure != boost::none && boost::get(maximumMeasure) <= 0)
						throw std::invalid_argument("maximumMeasure");
					if(front() != '\t')
						return false;
					assert(length() == 1 && glyphs_.unique());

					const std::unique_ptr<const FontMetrics<Scalar>> fontMetrics(context.fontMetrics(glyphs_->font.get().font()));
					const auto tabExpander(makeFixedWidthTabExpander(tabSize, *fontMetrics, lengthContext));
					glyphs_->advances[0] = static_cast<int>(tabExpander.get().nextTabStop(ipd, boost::const_begin(*this) - layoutString.data()));
					if(maximumMeasure != boost::none)
						glyphs_->advances[0] = std::min(glyphs_->advances[0], static_cast<int>(boost::get(maximumMeasure)));
					justified_ = false;
					return true;
				}

				/// @see GlyphVector#fillGlyphs
				void GlyphVectorImpl::fillGlyphs(PaintContext& context, const Point& origin/*, boost::optional<boost::integer_range<std::size_t>> range = boost::none*/) const {
					return paintGlyphs(context, origin/*, range*/, false);
				}

				/// @see GlyphVector#font
				std::shared_ptr<const Font> GlyphVectorImpl::font() const BOOST_NOEXCEPT {
					return glyphs_->font.get().font();
				}

				/// @see GlyphVector#fontRenderContext
				const FontRenderContext& GlyphVectorImpl::fontRenderContext() const BOOST_NOEXCEPT {
					return glyphs_->font.get().fontRenderContext();
				}

				namespace {
					std::shared_ptr<const Font> selectFont(const StringPiece& textString, const FontCollection& fontCollection, const ActualFontSpecification& specification) {
						const auto& families = boost::fusion::at_key<presentation::styles::FontFamily>(specification);
						const auto& pointSize = boost::fusion::at_key<presentation::styles::FontSize>(specification);
						const auto& properties = boost::fusion::at_key<void>(specification);
						const auto& sizeAdjust = boost::fusion::at_key<presentation::styles::FontSizeAdjust>(specification);
						if(!boost::empty(families)) {
							const auto family(findMatchingFontFamily(fontCollection, families));
							const FontDescription description(FontFamily(*family), pointSize, properties);
							return fontCollection.get(description, geometry::makeIdentityTransform(), sizeAdjust);
						} else
							return fontCollection.lastResortFallback(pointSize, properties, geometry::makeIdentityTransform(), sizeAdjust);
					}
				}

				/**
				 * Fills the glyph array with default index, instead of using @c ScriptShape.
				 * @param dc The device context
				 * @param text The text to generate glyphs
				 * @param analysis The @c SCRIPT_ANALYSIS object
				 * @param[out] glyphs The result
				 * @return The number of generated glyphs
				 */
				inline std::size_t GlyphVectorImpl::generateDefaultGlyphs(win32::Handle<HDC>::Type dc,
						const StringPiece& text, const SCRIPT_ANALYSIS& analysis, RawGlyphVector& glyphs) {
					SCRIPT_CACHE fontCache(nullptr);
					SCRIPT_FONTPROPERTIES fp;
					fp.cBytes = sizeof(decltype(fp));
					if(FAILED(::ScriptGetFontProperties(dc.get(), &fontCache, &fp)))
						fp.wgDefault = 0;	// hmm...

					std::unique_ptr<WORD[]> indices, clusters;
					std::unique_ptr<SCRIPT_VISATTR[]> visualAttributes;
					const int numberOfGlyphs = static_cast<int>(text.length());
					indices.reset(new WORD[numberOfGlyphs]);
					clusters.reset(new WORD[text.length()]);
					visualAttributes.reset(new SCRIPT_VISATTR[numberOfGlyphs]);
					std::fill_n(indices.get(), numberOfGlyphs, fp.wgDefault);
					const bool ltr = analysis.fRTL == 0 || analysis.fLogicalOrder == 1;
					for(std::size_t i = 0, c = text.length(); i < c; ++i)
						clusters[i] = static_cast<WORD>(ltr ? i : (c - i));
					const SCRIPT_VISATTR va = {SCRIPT_JUSTIFY_NONE, 1, 0, 0, 0, 0};
					std::fill_n(visualAttributes.get(), numberOfGlyphs, va);

					// commit
					using std::swap;
					swap(glyphs.fontCache, fontCache);
					swap(glyphs.indices, indices);
					swap(glyphs.clusters, clusters);
					swap(glyphs.visualAttributes, visualAttributes);
					::ScriptFreeCache(&fontCache);

					return numberOfGlyphs;
				}

				/**
				 * Generates glyphs for the text.
				 * @param dc The device context
				 * @param text The text to generate glyphs
				 * @param analysis The @c SCRIPT_ANALYSIS object
				 * @param[out] glyphs The result
				 * @return The pair of the number of generated glyphs and the error
				 * @retval S_OK succeeded
				 * @retval USP_E_SCRIPT_NOT_IN_FONT the font does not support the required script
				 * @retval E_INVALIDARG other Uniscribe error. usually, too long run was specified
				 * @retval HRESULT other Uniscribe error
				 * @throw std#bad_alloc failed to allocate buffer for glyph indices or visual attributes array
				 */
				std::pair<std::size_t, HRESULT> GlyphVectorImpl::generateGlyphs(win32::Handle<HDC>::Type dc,
						const StringPiece& text, const SCRIPT_ANALYSIS& analysis, RawGlyphVector& glyphs) {
#ifdef _DEBUG
					if(HFONT currentFont = static_cast<HFONT>(::GetCurrentObject(dc.get(), OBJ_FONT))) {
						LOGFONTW lf;
						if(::GetObjectW(currentFont, sizeof(LOGFONTW), &lf) > 0)
							ASCENSION_LOG_TRIVIAL(debug) << L"[TextLayout.TextRun.generateGlyphs] Selected font is '" << lf.lfFaceName << L"'.\n";
					}
#endif

					SCRIPT_CACHE fontCache(nullptr);	// TODO: this object should belong to a font, not glyph run???
					std::unique_ptr<WORD[]> indices, clusters;
					std::unique_ptr<SCRIPT_VISATTR[]> visualAttributes;
					clusters.reset(new WORD[text.length()]);
					int numberOfGlyphs = estimateNumberOfGlyphs(text.length());
					HRESULT hr;
					while(true) {
						indices.reset(new WORD[numberOfGlyphs]);
						visualAttributes.reset(new SCRIPT_VISATTR[numberOfGlyphs]);
						hr = ::ScriptShape(dc.get(), &fontCache,
							boost::const_begin(text), static_cast<int>(boost::size(text)),
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
					return std::make_pair(static_cast<std::size_t>(numberOfGlyphs), hr);
				}

				/// @see GlyphVector#glyphCharacterIndex
				Index GlyphVectorImpl::glyphCharacterIndex(std::size_t index) const {
					if(index >= numberOfGlyphs())
						throw std::out_of_range("index");
					const auto glyphIndices = glyphs();
					const LogicalClusterIterator e;
					for(LogicalClusterIterator i(clusters(), glyphIndices, 0); i != e; ++i) {
						const auto cluster(*i);
						if(index >= static_cast<std::size_t>(boost::const_begin(cluster.glyphs) - boost::const_begin(glyphIndices))
								&& index < static_cast<std::size_t>(boost::const_end(cluster.glyphs) - boost::const_begin(glyphIndices)))
							return cluster.front();
					}
					return length();
				}

				/// @see GlyphVector#glyphCode
				GlyphCode GlyphVectorImpl::glyphCode(std::size_t index) const {
					if(index >= numberOfGlyphs())
						throw std::out_of_range("index");
					return glyphs()[index];
				}

				/// @see GlyphVector#glyphLogicalBounds
				graphics::Rectangle GlyphVectorImpl::glyphLogicalBounds(std::size_t index) const {
					if(index >= numberOfGlyphs())
						throw std::out_of_range("index");
					const Scalar x = glyphLogicalPosition(index);
					const auto yrange = logicalExtents();
					return graphics::geometry::make<graphics::Rectangle>((
						geometry::_top = *boost::const_begin(yrange), geometry::_bottom = *boost::const_end(yrange),
						geometry::_left = x, geometry::_right = static_cast<Scalar>(x + effectiveAdvances()[index])));
				}

				inline Scalar GlyphVectorImpl::glyphLogicalPosition(std::size_t index) const {
					assert(index <= numberOfGlyphs());
					const boost::iterator_range<const int*> glyphAdvances(effectiveAdvances());
					assert(boost::const_begin(glyphAdvances) != nullptr);
					int x = 0;
					for(std::size_t i = 0, c = numberOfGlyphs(); i < c; ++i) {
						if(i == index)
							break;
						x += glyphAdvances[i];
					}
					return static_cast<Scalar>(x);
				}

				/// @see GlyphVector#glyphMetrics
				GlyphMetrics GlyphVectorImpl::glyphMetrics(std::size_t index) const {
					if(index >= numberOfGlyphs())
						throw IndexOutOfBoundsException("index");
	
					RenderingContext2D context(win32::detail::screenDC());
					std::shared_ptr<const Font> oldFont(context.font());
					context.setFont(font());
					GLYPHMETRICS gm;
					const MAT2 matrix = {1, 0, 0, 1};	// TODO: Consider glyph transform.
					const DWORD lastError = (::GetGlyphOutlineW(context.native().get(),
						glyphCode(index), GGO_GLYPH_INDEX | GGO_METRICS, &gm, 0, nullptr, &matrix) == GDI_ERROR) ? ::GetLastError() : ERROR_SUCCESS;
					context.setFont(oldFont);
					if(lastError != ERROR_SUCCESS)
						throw makePlatformError(lastError);
					const auto sx = geometry::scaleX(fontRenderContext().transform()) / geometry::scaleX(context.fontRenderContext().transform());
					const auto sy = geometry::scaleY(fontRenderContext().transform()) / geometry::scaleY(context.fontRenderContext().transform());
					return GlyphMetrics(gm.gmCellIncY == 0,
						Dimension(geometry::_dx = static_cast<Scalar>(gm.gmCellIncX * sx), geometry::_dy = static_cast<Scalar>(gm.gmCellIncY * sy)),
						geometry::make<Rectangle>(
							geometry::make<Point>((
								geometry::_x = static_cast<Scalar>(gm.gmptGlyphOrigin.x * sx), geometry::_y = -static_cast<Scalar>(gm.gmptGlyphOrigin.y * sy))),
							Dimension(geometry::_dx = static_cast<Scalar>(gm.gmBlackBoxX * sx), geometry::_dy = static_cast<Scalar>(gm.gmBlackBoxY * sy))),
						static_cast<GlyphMetrics::Type>(0));
				}

				/// @see GlyphVector#glyphPosition
				Point GlyphVectorImpl::glyphPosition(std::size_t index) const {
					if(index > numberOfGlyphs())
						throw IndexOutOfBoundsException("index");
					const Scalar logicalPosition = glyphLogicalPosition(index);
					static const GOFFSET zeroOffset = {0, 0};
					const GOFFSET& glyphOffset = (index != numberOfGlyphs()) ? glyphOffsets()[index] : zeroOffset;
					return geometry::make<Point>((
						geometry::_x = static_cast<Scalar>(logicalPosition + glyphOffset.du), geometry::_y = static_cast<Scalar>(glyphOffset.dv)));
				}

				/// @see GlyphVector#glyphPositions
				void GlyphVectorImpl::glyphPositions(const boost::integer_range<std::size_t>& range, std::vector<Point>& out) const {
					const auto orderedRange = range | adaptors::ordered();
					if(*boost::const_end(orderedRange) > numberOfGlyphs())
						throw IndexOutOfBoundsException("range");

					std::vector<Point> positions;
					positions.reserve(boost::size(range));
					for(std::size_t i = *std::begin(orderedRange); i < *std::end(orderedRange); ++i) {
						const Scalar logicalPosition = glyphLogicalPosition(i);
						const GOFFSET& glyphOffset = glyphOffsets()[i];
						geometry::x(positions[i]) = static_cast<Scalar>(logicalPosition + glyphOffset.du);
						geometry::y(positions[i]) = static_cast<Scalar>(glyphOffset.dv);
					}
					std::swap(positions, out);
				}

				inline boost::integer_range<std::size_t> GlyphVectorImpl::glyphRange(const StringPiece& range /* = StringPiece() */) const {
					assert(glyphs_.get() != nullptr);
					assert(analysis_.fLogicalOrder == 0);
					const StringPiece characterRange((range != StringPiece()) ? range : *this);
					assert(boost::const_begin(characterRange) >= boost::const_begin(*this) && boost::const_end(characterRange) <= boost::const_end(*this));
					assert(boost::const_begin(characterRange) == boost::const_begin(*this)
						|| boost::const_begin(characterRange) == boost::const_end(*this)
						|| cluster(boost::const_begin(characterRange)) != cluster(boost::const_begin(characterRange) - 1));
					assert(boost::const_end(characterRange) == boost::const_begin(*this)
						|| boost::const_end(characterRange) == boost::const_end(*this)
						|| cluster(boost::const_end(characterRange)) != cluster(boost::const_end(characterRange) + 1));

					boost::optional<StringPiece::const_iterator> b, e;
					if(analysis_.fRTL == 0) {	// LTR
						b = boost::const_begin(characterRange);
						e = boost::const_end(characterRange);
						if(b >= boost::const_end(*this))
							b = boost::none;;
						if(e >= boost::const_end(*this))
							e = boost::none;
					} else {					// RTL
						if(boost::const_end(characterRange) > boost::const_begin(*this))
							b = boost::const_end(characterRange) - 1;
						if(boost::const_begin(characterRange) > boost::const_begin(*this))
							e = boost::const_begin(characterRange) - 1;
					}
					return boost::irange(
						(b != boost::none) ? cluster(boost::get(b)) : numberOfGlyphs_ + clusterOffset_,
						(e != boost::none) ? cluster(boost::get(e)) : numberOfGlyphs_ + clusterOffset_);
				}

				/// @see GlyphVector#glyphVisualBounds
				graphics::Rectangle GlyphVectorImpl::glyphVisualBounds(std::size_t index) const {
					if(index >= numberOfGlyphs())
						throw std::out_of_range("index");
					Scalar originX = glyphLogicalPosition(index);
					const GlyphMetrics gm(glyphMetrics(index));
					const GOFFSET& offset = glyphOffsets()[index];
					graphics::Rectangle result;
					geometry::translate(geometry::_from = gm.bounds(), geometry::_to = result,
						geometry::_tx = static_cast<Scalar>(originX + offset.du), geometry::_ty = static_cast<Scalar>(offset.dv));
					return result;
				}

#if 0
				inline void GlyphVectorImpl::hitTest(Scalar ipd, int& encompasses, int* trailing) const {
					int tr;
					const int x = static_cast<int>((direction() == LEFT_TO_RIGHT) ? ipd : (measure(*this) - ipd));
					const HRESULT hr = ::ScriptXtoCP(x, static_cast<int>(length()), numberOfGlyphs(), clusters().begin(),
						visualAttributes().begin(), effectiveAdvances().begin(), &analysis_, &encompasses, &tr);
					if(FAILED(hr))
						throw makePlatformError(hr);
					if(trailing != nullptr)
						*trailing = encompasses + tr;
				}
#endif

				/// @see TextRun#hitTestCharacter
				TextHit<> GlyphVectorImpl::hitTestCharacter(Scalar position, const boost::optional<NumericRange<Scalar>>& bounds, bool* outOfBounds) const {
					bool beyondLineLeft = false, beyondLineRight = false;
					if(bounds != boost::none) {
						const auto b(boost::get(bounds));
						if(position < std::min(*boost::const_begin(b), *boost::const_end(b)))
							beyondLineLeft = true;
						else if(position >= std::max(*boost::const_begin(b), *boost::const_end(b)))
							beyondLineRight = true;
					}

					if(!beyondLineLeft && !beyondLineRight) {
						int cp, trailing;
						const HRESULT hr = ::ScriptXtoCP(static_cast<int>(position), static_cast<int>(length()), numberOfGlyphs(),
							boost::const_begin(clusters()), boost::const_begin(visualAttributes()), boost::const_begin(effectiveAdvances()), &analysis_, &cp, &trailing);
						if(FAILED(hr))
							throw makePlatformError(hr);
						if(cp == -1)
							beyondLineLeft = true;	// 'trailing' should be 0
						else if(cp == length() && trailing == 1)
							beyondLineRight = true;
						else
							return (trailing == 0) ? TextHit<>::leading(cp) : TextHit<>::beforeOffset(cp + trailing);
					}

					if((beyondLineLeft || beyondLineRight) && outOfBounds != nullptr)
						*outOfBounds = true;
					assert(length() != 0);
					if(beyondLineLeft)
						return (direction() == presentation::LEFT_TO_RIGHT) ? TextHit<>::leading(0) : TextHit<>::beforeOffset(length());
					else if(beyondLineRight)
						return (direction() == presentation::LEFT_TO_RIGHT) ? TextHit<>::beforeOffset(length()) : TextHit<>::leading(0);
					ASCENSION_ASSERT_NOT_REACHED();
				}

				/// @see TextRun#hitToLogicalPosition
				Scalar GlyphVectorImpl::hitToLogicalPosition(const TextHit<>& hit) const {
					if(hit.insertionIndex() > characterRange().length())
						throw IndexOutOfBoundsException("hit");
					auto c(boost::const_begin(clusters()));
					auto va(boost::const_begin(visualAttributes()));
					auto ea(boost::const_begin(effectiveAdvances()));
					int logicalPosition;
					const HRESULT hr = ::ScriptCPtoX(static_cast<int>(hit.characterIndex()), !hit.isLeadingEdge(),
						static_cast<int>(length()), numberOfGlyphs(), c, va, ea, &analysis_, &logicalPosition);
					if(FAILED(hr))
						throw makePlatformError(hr);
					// TODO: handle letter-spacing correctly.
//					if(visualAttributes()[offset].fClusterStart == 0) {	// oops, i can't remember what this code means...
//					}
					return static_cast<Scalar>(logicalPosition);
				}

				inline HRESULT GlyphVectorImpl::justify(int width) {
					assert(glyphs_->indices.get() != nullptr);
					assert(boost::const_begin(advances()) != nullptr);
					assert(glyphs_->justifiedAdvances.get() != nullptr);

					HRESULT hr = S_OK;
					const int totalAdvances = boost::accumulate(advances(), 0);
					if(width != totalAdvances) {
						hr = ::ScriptJustify(boost::const_begin(visualAttributes()), boost::const_begin(advances()), numberOfGlyphs(),
							width - totalAdvances, 2, glyphs_->justifiedAdvances.get() + (boost::const_begin(*this) - glyphs_->position));
						justified_ = SUCCEEDED(hr);
					} else
						justified_ = false;
					return hr;
				}

				inline HRESULT GlyphVectorImpl::logicalAttributes(SCRIPT_LOGATTR attributes[]) const {
					raiseIfNull(attributes, "attributes");
					return ::ScriptBreak(boost::const_begin(*this), static_cast<int>(length()), &analysis_, attributes);
				}

				/// @see GlyphVector#logicalBounds
				graphics::Rectangle GlyphVectorImpl::logicalBounds() const {
					const auto xs = effectiveAdvances();
					Scalar left = std::numeric_limits<Scalar>::max(), right = std::numeric_limits<Scalar>::lowest();
					for(std::size_t i = 0, c = numberOfGlyphs(); i < c; ++i) {
						const Scalar x = glyphLogicalPosition(i);
						left = std::min(x, left);
						right = std::max(x + xs[i], right);
					}
					return geometry::make<graphics::Rectangle>(std::make_pair(nrange(left, right), logicalExtents()));
				}

				inline HRESULT GlyphVectorImpl::logicalWidths(int widths[]) const {
					raiseIfNull(widths, "widths");
					return ::ScriptGetLogicalWidths(&analysis_, static_cast<int>(length()), numberOfGlyphs(),
						boost::const_begin(advances()), boost::const_begin(clusters()), boost::const_begin(visualAttributes()), widths);
				}

				/// @see GlyphVector#numberOfGlyphs
				std::size_t GlyphVectorImpl::numberOfGlyphs() const BOOST_NOEXCEPT {
					return numberOfGlyphs_;
				}

#if 0
				/**
				 * @internal Fills or strokes the glyphs of the specified range in this run.
				 * This method uses the stroke and fill styles which are set in @a context.
				 * @param context The graphics context
				 * @param origin The base point of this run (, does not corresponds to @c range-&gt;beginning())
				 * @param range The character range to paint. If this is @c boost#none, the all characters are painted
				 * @param onlyStroke If @c true, this method only strokes the glyphs without filling
				 */
				inline void GlyphVectorImpl::paintGlyphs(PaintContext& context, const Point& origin, const StringPiece& range, bool onlyStroke) const {
					return paintGlyphs(context, origin, glyphRange(range), onlyStroke);
				}
#endif

				/**
				 * @internal Fills or strokes the glyphs of the specified range in this run.
				 * This method uses the stroke and fill styles which are set in @a context.
				 * @param context The graphics context
				 * @param origin The base point of this run (, does not corresponds to @c range-&gt;beginning())
#if 0
				 * @param range The glyph range to paint. If this is @c boost#none, the all glyphs are painted
#endif
				 * @param onlyStroke If @c true, this method only strokes the glyphs without filling
				 */
				void GlyphVectorImpl::paintGlyphs(PaintContext& context, const Point& origin/*, boost::optional<boost::integer_range<std::size_t>> range*/, bool onlyStroke) const {
//					if(range == boost::none)
//						return paintGlyphs(context, origin, *this, onlyStroke);
//					else if(boost::empty(boost::get(range)))
//						return;

					context.setFont(font());
					if(onlyStroke) {
						if(!win32::boole(::BeginPath(context.native().get())))
							throw makePlatformError();
					} else
						::SetTextColor(context.native().get(), context.fillStyle()->native().lbColor);
					assert(analysis_.fLogicalOrder == 0);
					// paint glyphs
					const RECT boundsToPaint(toNative<RECT>(context.boundsToPaint()));
					const HRESULT hr = ::ScriptTextOut(context.native().get(), &glyphs_->fontCache,
						static_cast<int>(geometry::x(origin)), static_cast<int>(geometry::y(origin)),
						0, &boundsToPaint, &analysis_, nullptr, 0,
						boost::const_begin(glyphs()), numberOfGlyphs(), boost::const_begin(advances()),
						boost::const_begin(effectiveAdvances()), boost::const_begin(glyphOffsets()));
					if(onlyStroke)
						::EndPath(context.native().get());
					if(FAILED(hr))
						throw makePlatformError(hr);
					if(onlyStroke && !win32::boole(::StrokePath(context.native().get())))
						throw makePlatformError();
				}

				/**
				 * Positions the glyphs in the vector.
				 * @param dc The device context
				 * @see #substituteGlyphs, TextRunImpl#positionGlyphs
				 * @note This method should be called after shaping and before breaking.
				 */
				void GlyphVectorImpl::positionGlyphs(win32::Handle<HDC>::Type dc) {
					assert(glyphs_.get() != nullptr);
					assert(glyphs_.unique());
					assert(glyphs_->indices.get() != nullptr);
					assert(glyphs_->advances.get() == nullptr);

					std::unique_ptr<int[]> advances(new int[numberOfGlyphs()]);
					std::unique_ptr<GOFFSET[]> offsets(new GOFFSET[numberOfGlyphs()]);
//					ABC width;
					HRESULT hr = ::ScriptPlace(nullptr, &glyphs_->fontCache, glyphs_->indices.get(), numberOfGlyphs(),
						glyphs_->visualAttributes.get(), &analysis_, advances.get(), offsets.get(), nullptr/*&width*/);
					if(hr == E_PENDING) {
						HFONT oldFont = static_cast<HFONT>(::SelectObject(dc.get(), font()->native().get()));
						hr = ::ScriptPlace(dc.get(), &glyphs_->fontCache, glyphs_->indices.get(), numberOfGlyphs(),
							glyphs_->visualAttributes.get(), &analysis_, advances.get(), offsets.get(), nullptr/*&width*/);
						::SelectObject(dc.get(), oldFont);
					}
					if(FAILED(hr))
						throw hr;

					// commit
					glyphs_->advances = std::move(advances);
					glyphs_->offsets = std::move(offsets);
//					glyphs_->width = width;
				}

				/**
				 * Reserves to @c #justify.
				 * @note This method should be called after shaping and before breaking.
				 */
				void GlyphVectorImpl::reserveJustification() {
					assert(glyphs_.get() != nullptr);
					assert(glyphs_.unique());
					assert(glyphs_->indices.get() != nullptr);
					assert(glyphs_->justifiedAdvances.get() == nullptr);

					glyphs_->justifiedAdvances.reset(new int[numberOfGlyphs()]);
				}

				/// @see GlyphVector#setGlyphPosition
				void GlyphVectorImpl::setGlyphPosition(std::size_t index, const Point& position) {
					if(index > numberOfGlyphs())
						throw IndexOutOfBoundsException("index");
					const Scalar logicalPosition = glyphLogicalPosition(index);
					GOFFSET& glyphOffset = glyphs_->offsets[boost::const_begin(glyphOffsets()) - glyphs_->offsets.get()];
					glyphOffset.du = static_cast<LONG>(geometry::x(position) - logicalPosition);
					glyphOffset.dv = static_cast<LONG>(geometry::y(position));
				}

				// shaping stuffs
				namespace {
					/**
					 * Returns a Unicode script corresponds to Win32 language identifier for digit substitution.
					 * @param id the language identifier
					 * @return the script or @c NOT_PROPERTY
					 */
					inline int convertWin32LangIDtoUnicodeScript(LANGID id) BOOST_NOEXCEPT {
						using text::ucd::Script;
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
						return text::ucd::NOT_PROPERTY;
					}
				} // namespace @0

				void GlyphVectorImpl::shape(win32::Handle<HDC>::Type dc) {
					assert(glyphs_.unique());

					// TODO: check if the requested style (or the default one) disables shaping.

					RawGlyphVector glyphs(glyphs_->position, glyphs_->font.get().font(), glyphs_->font.get().fontRenderContext(), glyphs_->scriptTag);
					HFONT oldFont = static_cast<HFONT>(::SelectObject(dc.get(), font()->native().get()));
					std::size_t numberOfGlyphs;
					HRESULT hr;
					std::tie(numberOfGlyphs, hr) = generateGlyphs(dc, *this, analysis_, glyphs);
					if(hr == USP_E_SCRIPT_NOT_IN_FONT) {
						analysis_.eScript = SCRIPT_UNDEFINED;
						std::tie(numberOfGlyphs, hr) = generateGlyphs(dc, *this, analysis_, glyphs);
					}
					if(FAILED(hr))
						numberOfGlyphs = generateDefaultGlyphs(dc, *this, analysis_, glyphs);
					::SelectObject(dc.get(), oldFont);

					// commit
					using std::swap;
					swap(*glyphs_, glyphs);
					numberOfGlyphs_ = numberOfGlyphs;
				}
#if 0
				void GlyphVectorImpl::shape(DC& dc, const String& layoutString, const ILayoutInformationProvider& lip, TextRun* nextRun) {
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
					std::shared_ptr<const RunStyle> defaultStyle(lip.presentation().defaultTextRunStyle());
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
						std::shared_ptr<const Font> font(lip.fontCollection().get(L"Arial", fp, computedFontSizeAdjust));
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
						typedef std::vector<std::pair<std::shared_ptr<const Font>, int>> FailedFonts;
						FailedFonts failedFonts;	// failed fonts (font handle vs. # of missings)
						int numberOfMissingGlyphs;

						const Char* textString = layoutString.data() + beginning();

#define ASCENSION_MAKE_TEXT_STRING_SAFE()												\
	assert(safeString.get() == nullptr);												\
	safeString.reset(new Char[length()]);												\
	std::wmemcpy(safeString.get(), layoutString.data() + beginning(), length());		\
	replace_if(safeString.get(),														\
		safeString.get() + length(), surrogates::isSurrogate, REPLACEMENT_CHARACTER);	\
	textString = safeString.get();

#define ASCENSION_CHECK_FAILED_FONTS()																\
	bool skip = false;																				\
	for(FailedFonts::const_iterator																	\
			i(boost::const_begin(failedFonts)), e(boost::const_end(failedFonts)); i != e; ++i) {	\
		if(i->first == font_) {																		\
			skip = true;																			\
			break;																					\
		}																							\
	}

						// ScriptShape may crash if the shaping is disabled (see Mozilla bug 341500).
						// Following technique is also from Mozilla (gfxWindowsFonts.cpp).
						std::unique_ptr<Char[]> safeString;
						if(analysis_.eScript == SCRIPT_UNDEFINED
								&& std::find_if(textString, textString + length(), surrogates::isSurrogate) != textString + length()) {
							ASCENSION_MAKE_TEXT_STRING_SAFE();
						}

						// 1. the primary font
						oldFont = dc.selectObject((font_ = lip.fontCollection().get(computedFontFamily, computedFontProperties))->handle().get());
						hr = generateGlyphs(dc, textString, &numberOfMissingGlyphs);
						if(hr == USP_E_SCRIPT_NOT_IN_FONT) {
							::ScriptFreeCache(&glyphs_->fontCache);
							failedFonts.push_back(std::make_pair(font_, (hr == S_FALSE) ? numberOfMissingGlyphs : std::numeric_limits<int>::max()));
						}

						// 2. the national font for digit substitution
						if(hr == USP_E_SCRIPT_NOT_IN_FONT && analysis_.eScript != SCRIPT_UNDEFINED && analysis_.s.fDigitSubstitute != 0) {
							script = convertWin32LangIDtoUnicodeScript(scriptProperties.get(analysis_.eScript).langid);
							if(script != NOT_PROPERTY) {
								const std::basic_string<WCHAR> fallbackFontFamily(fallback(script));
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
							for(std::size_t i = 0; i < lip.getFontSelector().numberOfLinkedFonts(); ++i) {
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
							for(StringCharacterIterator i(StringPiece(textString, length())); i.hasNext(); i.next()) {
								script = Script::of(i.current());
								if(script != Script::UNKNOWN && script != Script::COMMON && script != Script::INHERITED)
									break;
							}
							if(script != Script::UNKNOWN && script != Script::COMMON && script != Script::INHERITED) {
								const std::basic_string<WCHAR> fallbackFontFamily(fallback(script));
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
//								if(font_ == nullptr && previousRun != nullptr) {
//									// use the previous run setting (but this will copy the style of the font...)
//									analysis_.eScript = previousRun->analysis_.eScript;
//									font_ = previousRun->font_;
//								}
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
								if(std::find_if(textString, textString + length(), surrogates::isSurrogate) != textString + length()) {
									ASCENSION_MAKE_TEXT_STRING_SAFE();
								}
								for(FailedFonts::iterator i(std::begin(failedFonts)), e(std::end(failedFonts)); i != e; ++i) {
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
								assert(!boost::empty(failedFonts));
								FailedFonts::const_iterator bestFont(std::begin(failedFonts));
								for(FailedFonts::const_iterator i(bestFont + 1), e(std::end(failedFonts)); i != e; ++i) {
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
								std::begin(layoutString) + nextRun->beginning(), std::begin(layoutString) + nextRun->beginning() + 2);
							if(variationSelector >= 0xe0100ul && variationSelector <= 0xe01eful) {
								const CodePoint baseCharacter = surrogates::decodeLast(
									layoutString.data() + beginning(), layoutString.data() + boost::const_end(*this));
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
				/// @see GlyphVector#strokeGlyphs
				void GlyphVectorImpl::strokeGlyphs(PaintContext& context, const Point& origin/*, boost::optional<boost::integer_range<std::size_t>> range = boost::none*/) const {
					return paintGlyphs(context, origin/*, range*/, true);
				}

				/**
				 * 
				 * @tparam SinglePassReadableRange The type of @a runs
				 * @param runs the minimal runs
				 * @param layoutString the whole string of the layout
				 * @see #merge, #positionGlyphs
				 */
				template<typename SinglePassReadableRange>
				void GlyphVectorImpl::substituteGlyphs(const SinglePassReadableRange/*boost::iterator_range<std::vector<GlyphVectorImpl*>::iterator>*/& runs) {
					// this method processes the following substitutions:
					// 1. missing glyphs
					// 2. ideographic variation sequences (if Uniscribe did not support)

					// 1. Presentative glyphs for missing ones

					// TODO: generate missing glyphs.

					// 2. Ideographic Variation Sequences (Uniscribe workaround)
					// Older Uniscribe (version < 1.626.7100.0) does not support IVS.

#ifdef ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
					if(!uniscribeSupportsIVS()) {
						for(auto i(std::begin(runs)); i != std::end(runs); ++i) {
							GlyphVectorImpl& run = **i;

							// process IVSes in a glyph run
							if(run.analysis_.eScript != SCRIPT_UNDEFINED && run.length() > 3
									&& text::surrogates::isHighSurrogate(run[0]) && text::surrogates::isLowSurrogate(run[1])) {
								for(text::StringCharacterIterator i(run, boost::const_begin(run) + 2); i.hasNext(); ++i) {
									const CodePoint variationSelector = *i;
									if(variationSelector >= 0xe0100ul && variationSelector <= 0xe01eful) {
										text::StringCharacterIterator baseCharacter(i);
										--baseCharacter;
										if(run.font()->ivsGlyph(
												*baseCharacter, variationSelector,
												run.glyphs_->indices[run.glyphs_->clusters[baseCharacter.tell() - boost::const_begin(run)]])) {
											run.glyphs_->vanish(*run.font(), i.tell());
											run.glyphs_->vanish(*run.font(), i.tell() + 1);
										}
									}
								}
							}

							// process an IVS across two glyph runs
							if(i + 1 != boost::const_end(runs) && i[1]->length() > 1) {
								GlyphVectorImpl& next = *i[1];
								const CodePoint variationSelector = text::utf::decodeFirst(boost::const_begin(next), boost::const_begin(next) + 2);
								if(variationSelector >= 0xe0100ul && variationSelector <= 0xe01eful) {
									const CodePoint baseCharacter = text::utf::decodeLast(boost::const_begin(run), boost::const_end(run));
									if(run.font()->ivsGlyph(baseCharacter, variationSelector,
											run.glyphs_->indices[run.glyphs_->clusters[boost::size(run) - 1]])) {
										next.glyphs_->vanish(*run.font(), boost::const_begin(next));
										next.glyphs_->vanish(*run.font(), boost::const_begin(next) + 1);
									}
								}
							}
						}
#undef ASCENSION_VANISH_VARIATION_SELECTOR
					}
#endif // ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
				}

				/// @see GlyphVector#visualBounds
				graphics::Rectangle GlyphVectorImpl::visualBounds() const {
					Scalar top, right, bottom, left;
					left = top = std::numeric_limits<Scalar>::max();
					right = bottom = std::numeric_limits<Scalar>::min();
					for(std::size_t i = 0, c = numberOfGlyphs(); i < c; ++i) {
						const auto gvb(glyphVisualBounds(i));
						top = std::min(geometry::top(gvb), top);
						right = std::max(geometry::right(gvb), right);
						bottom = std::max(geometry::bottom(gvb), bottom);
						left = std::min(geometry::left(gvb), left);
					}
					return geometry::make<graphics::Rectangle>((
						geometry::_top = top, geometry::_right = right, geometry::_bottom = bottom, geometry::_left = left));
				}
			}


			// TextRunImpl file-local class ///////////////////////////////////////////////////////////////////////////

			namespace {
				class TextRunImpl : public GlyphVectorImpl {
				public:
					struct Overlay {
						Color color;
						boost::integer_range<Index> range;
					};
				public:
					TextRunImpl(const StringPiece& characterRange, const SCRIPT_ANALYSIS& script,
						std::shared_ptr<const Font> font, const FontRenderContext& frc,
						OpenTypeLayoutTag scriptTag, const ActualTextRunStyleCore& coreStyle);
					static void generate(const StringPiece& textString,
						const presentation::styles::ComputedValue<presentation::TextLineStyle>::type& lineStyle,
						std::unique_ptr<presentation::ComputedStyledTextRunIterator> textRunStyles,
						const presentation::styles::Length::Context& lengthContext, Scalar measure,
						const FontCollection& fontCollection, const FontRenderContext& frc,
						const presentation::Pixels& parentFontSize, std::vector<TextRunImpl*>& textRuns,
						std::vector<AttributedCharacterRange<presentation::ComputedTextRunStyle>>& calculatedStyles);
					// TextRun
					const presentation::FlowRelativeFourSides<ActualBorderSide>* border() const BOOST_NOEXCEPT override {return &coreStyle_.get().borders;}
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
					boost::optional<Index> characterEncompassesPosition(float ipd) const BOOST_NOEXCEPT;
					Index characterHasClosestLeadingEdge(float ipd) const;
#endif // ASCENSION_ABANDONED_AT_VERSION_08
					const presentation::FlowRelativeFourSides<Scalar>* margin() const BOOST_NOEXCEPT override {return &coreStyle_.get().margins;}
					const presentation::FlowRelativeFourSides<Scalar>* padding() const BOOST_NOEXCEPT override {return &coreStyle_.get().paddings;}
					// attributes
					const ActualTextRunStyleCore& style() const BOOST_NOEXCEPT {return coreStyle_;}
					// layout
					std::unique_ptr<TextRunImpl> breakAt(StringPiece::const_iterator at);
					std::unique_ptr<TextRunImpl> breakIfTooLong();
#if 0
					static void mergeScriptsAndStyles(const StringPiece& layoutString, const SCRIPT_ITEM scriptRuns[],
						const OPENTYPE_TAG scriptTags[], std::size_t numberOfScriptRuns, const FontCollection& fontCollection,
						shared_ptr<const TextRunStyle> defaultStyle, unique_ptr<ComputedStyledTextRunIterator> styles,
						vector<TextRunImpl*>& textRuns, vector<const ComputedTextRunStyle>& computedStyles,
						vector<vector<const ComputedTextRunStyle>::size_type>& computedStylesIndices);
#endif
					void positionGlyphs(win32::Handle<HDC>::Type dc, const presentation::ComputedTextRunStyle& style);
					// drawing and painting
					void drawGlyphs(PaintContext& context, const Point& p, const boost::integer_range<Index>& range) const;
					void paintLineDecorations() const;

				private:
					TextRunImpl(const StringPiece& characterRange, const SCRIPT_ANALYSIS& script,
						std::unique_ptr<RawGlyphVector> glyphs, const ActualTextRunStyleCore& coreStyle);
					TextRunImpl(const TextRunImpl& other, std::unique_ptr<GlyphVectorImpl> leading);
					using GlyphVectorImpl::breakAt;
					using GlyphVectorImpl::breakIfTooLong;
					using GlyphVectorImpl::positionGlyphs;
				private:
					boost::flyweight<ActualTextRunStyleCore> coreStyle_;
				};

				/**
				 * Creates a @c TextRunImpl instance with a text string, script information, font rendering context and
				 * styles.
				 * @param characterRange The string this text run covers
				 * @param script @c SCRIPT_ANALYSIS The object obtained by @c ScriptItemize(OpenType)
				 * @param font The font renders this text run. Can't be @c null
				 * @param scriptTag An OpenType script tag describes the script of this text run
				 * @param coreStyle The core text style
				 * @throw NullPointerException @a characterRange and/or @a font are @c null
				 * @throw std#invalid_argument @a characterRange is empty
				 * @note This constructor is called by only @c #breakIfTooLong.
				 */
				TextRunImpl::TextRunImpl(const StringPiece& characterRange, const SCRIPT_ANALYSIS& script,
						std::shared_ptr<const Font> font, const FontRenderContext& frc, OpenTypeLayoutTag scriptTag, const ActualTextRunStyleCore& coreStyle)
						: GlyphVectorImpl(characterRange, script, font, frc, scriptTag), coreStyle_(coreStyle) {	// may throw NullPointerException for 'font'
				}

				/**
				 * Creates a @c TextRunImpl instance with a text string, script information, a computed glyph vector
				 * and styles.
				 * @param characterRange The string this text run covers
				 * @param script @c SCRIPT_ANALYSIS The object obtained by @c ScriptItemize(OpenType)
				 * @param glyphs The glyph vector
				 * @param coreStyle The core text style
				 * @throw NullPointerException @a characterRange and/or @a glyphs are @c null
				 * @throw std#invalid_argument @a characterRange is empty
				 * @note This constructor is called by only @c #generate.
				 */
				TextRunImpl::TextRunImpl(const StringPiece& characterRange, const SCRIPT_ANALYSIS& script,
						std::unique_ptr<RawGlyphVector> glyphs, const ActualTextRunStyleCore& coreStyle)
						: GlyphVectorImpl(characterRange, script, std::move(glyphs)), coreStyle_(coreStyle) {
				}

				/**
				 * Creates a @c TextRunImpl instance with a @c GlyphVectorImpl object.
				 * @param other The original @c TextRunImpl object
				 * @param leading The original @c GlyphVectorImpl object
				 * @see #breakAt, #breakIfTooLong
				 * @note This constructor is called by only @c #breakAt.
				 */
				TextRunImpl::TextRunImpl(const TextRunImpl& other, std::unique_ptr<GlyphVectorImpl> leading)
						: GlyphVectorImpl(*leading), coreStyle_(other.style()) {
				}

				/**
				 * Breaks the text run into two runs at the specified position.
				 * @param at The position at which break this run
				 * @return The new text run following this run
				 * @note This method is called by only @c #wrap.
				 */
				std::unique_ptr<TextRunImpl> TextRunImpl::breakAt(StringPiece::const_iterator at) {
					// create the new following run
					return std::unique_ptr<TextRunImpl>(new TextRunImpl(*this, GlyphVectorImpl::breakAt(at)));
				}

				std::unique_ptr<TextRunImpl> TextRunImpl::breakIfTooLong() {
					return std::unique_ptr<TextRunImpl>(new TextRunImpl(*this, GlyphVectorImpl::breakIfTooLong()));
				}

#ifdef ASCENSION_ABANDONED_AT_VERSION_08
				/// @see TextRun#characterEncompassesPosition
				boost::optional<Index> TextRunImpl::characterEncompassesPosition(float ipd) const BOOST_NOEXCEPT {
					int character;
					hitTest(ipd, character, nullptr);
					if(character == -1 || character == length())
						return boost::none;
					assert(character >= 0);
					return character;
				}

				/// @see TextRun#characterHasClosestLeadingEdge
				Index TextRunImpl::characterHasClosestLeadingEdge(float ipd) const {
					int character, trailing;
					hitTest(ipd, character, &trailing);
					if(character == -1)
						return 0;
					const int result = (character == length()) ? length() : (character + trailing);
					assert(result >= 0);
					return result;
				}
#endif // ASCENSION_ABANDONED_AT_VERSION_08

				namespace {
					template<typename ComputedFontSpecification>
					inline void buildActualFontSpecification(const ComputedFontSpecification& computed,
							const presentation::styles::Length::Context& context, const presentation::Pixels& computedParentSize, ActualFontSpecification& actual) {
						boost::fusion::at_key<presentation::styles::FontFamily>(actual) = boost::fusion::at_key<presentation::styles::FontFamily>(computed);
						boost::fusion::at_key<presentation::styles::FontSize>(actual) =
							presentation::styles::useFontSize(
								boost::fusion::at_key<presentation::styles::FontSize>(computed), context, computedParentSize).value();
						boost::fusion::at_key<void>(actual) = FontProperties(
							boost::fusion::at_key<presentation::styles::FontWeight>(computed),
							boost::fusion::at_key<presentation::styles::FontStretch>(computed),
							boost::fusion::at_key<presentation::styles::FontStyle>(computed));
						boost::fusion::at_key<presentation::styles::FontSizeAdjust>(actual) =
							boost::fusion::at_key<presentation::styles::FontSizeAdjust>(computed);
					}
				}

				/**
				 * @param textString
				 * @param lineStyle
				 * @param textRunStyles
				 * @param lengthContext
				 * @param measure
				 * @param fontCollection
				 * @param frc
				 * @param parentFontSize
				 * @param[out] textRuns
				 * @param[out] calculatedStyles
				 */
				void TextRunImpl::generate(const StringPiece& textString,
						const presentation::styles::ComputedValue<presentation::TextLineStyle>::type& lineStyle,
						std::unique_ptr<presentation::ComputedStyledTextRunIterator> textRunStyles,
						const presentation::styles::Length::Context& lengthContext, Scalar measure,
						const FontCollection& fontCollection, const FontRenderContext& frc, const presentation::Pixels& parentFontSize,
						std::vector<TextRunImpl*>& textRuns, std::vector<AttributedCharacterRange<presentation::ComputedTextRunStyle>>& calculatedStyles) {
					raiseIfNullOrEmpty(textString, "textString");

					// split the text line into text runs as following steps:
					// 1. split the text into script runs (SCRIPT_ITEMs) by Uniscribe
					// 2. split each script runs into atomically-shapable runs (TextRuns) with StyledRunIterator

					// 1. split the text into script runs by Uniscribe
					HRESULT hr;

					// 1-1. configure Uniscribe's itemize
					win32::AutoZero<SCRIPT_CONTROL> control;
					win32::AutoZero<SCRIPT_STATE> initialState;
					initialState.uBidiLevel = (boost::fusion::at_key<presentation::styles::Direction>(lineStyle) == presentation::RIGHT_TO_LEFT) ? 1 : 0;
//					initialState.fOverrideDirection = 1;
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
					initialState.fInhibitSymSwap = boost::fusion::at_key<presentation::styles::SymmetricSwappingInhibited>(lineStyle);
					initialState.fDisplayZWG = boost::fusion::at_key<presentation::styles::DeprecatedFormatCharactersDisabled>(lineStyle);
#endif // ASCENSION_ABANDONED_AT_VERSION_08
					SCRIPT_DIGITSUBSTITUTE sds;
					convertNumberSubstitutionToUniscribe(boost::fusion::at_key<presentation::styles::NumberSubstitution>(lineStyle), sds);
					hr = ::ScriptApplyDigitSubstitution(&sds, &control, &initialState);
					if(FAILED(hr))
						throw makePlatformError(hr);

					// 1-2. itemize
					// note that ScriptItemize can cause a buffer overflow (see Mozilla bug 366643)
					AutoArray<SCRIPT_ITEM, 128> scriptRuns;
					AutoArray<OPENTYPE_TAG, scriptRuns.STATIC_CAPACITY> scriptTags;
					int estimatedNumberOfScriptRuns = std::max(static_cast<int>(textString.length()) / 4, 2), numberOfScriptRuns;
					while(true) {
						scriptRuns.reallocate(estimatedNumberOfScriptRuns);
						scriptTags.reallocate(estimatedNumberOfScriptRuns);
						hr = Uniscribe16::instance().itemize(std::begin(textString), static_cast<int>(textString.length()),
							estimatedNumberOfScriptRuns, control, initialState, scriptRuns.get(), scriptTags.get(), numberOfScriptRuns);
						if(hr != E_OUTOFMEMORY)	// estimatedNumberOfRuns was enough...
							break;
						estimatedNumberOfScriptRuns *= 2;
					}
//					if(boost::fusion::at_key<presentation::styles::DeprecatedFormatCharactersDisabled>(lineStyle)) {
//						for(int i = 0; i < numberOfScriptRuns; ++i) {
//							scriptRuns[i].a.s.fInhibitSymSwap = initialState.fInhibitSymSwap;
//							scriptRuns[i].a.s.fDigitSubstitute = initialState.fDigitSubstitute;
//						}
//					}
					if(!Uniscribe16::instance().supportsOpenType())
						std::fill_n(scriptTags.get(), numberOfScriptRuns, SCRIPT_TAG_UNKNOWN);

					// 2. generate raw glyph vectors and computed styled text runs
					std::vector<std::unique_ptr<RawGlyphVector>> glyphRuns;
					glyphRuns.reserve(numberOfScriptRuns);
					std::vector<const SCRIPT_ANALYSIS*> scriptPointers;
					scriptPointers.reserve(numberOfScriptRuns);
					std::vector<AttributedCharacterRange<presentation::ComputedTextRunStyle>> styleRuns;
					{
						StringPiece::const_iterator lastGlyphRunEnd(nullptr);
						// script cursors
						AttributedCharacterRange<const SCRIPT_ITEM*>
							scriptRun(boost::const_begin(textString) + scriptRuns[0].iCharPos, &scriptRuns[0]),
							nextScriptRun((numberOfScriptRuns > 1) ?
								boost::const_begin(textString) + scriptRuns[1].iCharPos : boost::const_end(textString), scriptRun.attribute + 1);
						// style cursors
						AttributedCharacterRange<presentation::ComputedTextRunStyle> styleRun, nextStyleRun;
						if(textRunStyles.get() != nullptr) {
							styleRun.attribute = textRunStyles->style();
							styleRun.position = boost::const_begin(textString) + kernel::offsetInLine(textRunStyles->position());
							textRunStyles->next();
						} else {
							styleRun.attribute = presentation::ComputedTextRunStyle();
							styleRun.position = boost::const_begin(textString);
						}
						if(textRunStyles.get() != nullptr && !textRunStyles->isDone()) {
							nextStyleRun.attribute = textRunStyles->style();
							nextStyleRun.position = boost::const_begin(textString) + kernel::offsetInLine(textRunStyles->position());
						} else
							nextStyleRun.position = boost::const_end(textString);
						styleRuns.push_back(AttributedCharacterRange<presentation::ComputedTextRunStyle>(styleRun.position, styleRun.attribute));

						ActualFontSpecification fontSpecification;
						do {
							const StringPiece::const_iterator next(std::min(nextScriptRun.position, nextStyleRun.position));
							const bool advanceScriptRun = next == nextScriptRun.position;
							const bool advanceStyleRun = next == nextStyleRun.position;

							if(advanceScriptRun) {
								const StringPiece subRange(scriptRun.position, next - scriptRun.position);
								assert(boost::empty(glyphRuns) || boost::const_begin(subRange) == lastGlyphRunEnd);
								buildActualFontSpecification(styleRun.attribute.fonts, lengthContext, parentFontSize, fontSpecification);
								glyphRuns.push_back(
									std::unique_ptr<RawGlyphVector>(
										new RawGlyphVector(boost::const_begin(subRange),
											selectFont(subRange, fontCollection, fontSpecification),
											frc, scriptTags[scriptRun.attribute - scriptRuns.get()])));
								scriptPointers.push_back(&scriptRuns[scriptRun.attribute - scriptRuns.get()].a);
								assert(nextScriptRun.position <= boost::const_end(textString));
								scriptRun = nextScriptRun;
								if(scriptRun.position != boost::const_end(textString)) {
									if(++nextScriptRun.attribute < scriptRuns.get() + numberOfScriptRuns)
										nextScriptRun.position = boost::const_begin(textString) + nextScriptRun.attribute->iCharPos;
									else
										nextScriptRun.position = boost::const_end(textString);
								}
							}
							if(advanceStyleRun) {
								if(!advanceScriptRun) {
									const StringPiece subRange(makeStringPiece(!boost::empty(glyphRuns) ? lastGlyphRunEnd : boost::const_begin(textString), next));
									buildActualFontSpecification(styleRun.attribute.fonts, lengthContext, parentFontSize, fontSpecification);
									glyphRuns.push_back(
										std::unique_ptr<RawGlyphVector>(
											new RawGlyphVector(boost::const_begin(subRange),
												selectFont(subRange, fontCollection, fontSpecification),
												frc, scriptTags[scriptRun.attribute - scriptRuns.get()])));
								}
								assert(nextStyleRun.position <= boost::const_end(textString));
								styleRun = std::move(nextStyleRun);	// C2668 if included <boost/log/trivial.hpp> without 'std::' ???
								styleRuns.push_back(AttributedCharacterRange<presentation::ComputedTextRunStyle>(styleRun.position, styleRun.attribute));
								if(textRunStyles.get() != nullptr && !textRunStyles->isDone()) {
									textRunStyles->next();
									if(!textRunStyles->isDone()) {
										nextStyleRun.attribute = textRunStyles->style();
										nextStyleRun.position = boost::const_begin(textString) + kernel::offsetInLine(textRunStyles->position());
									} else
										nextStyleRun.position = boost::const_end(textString);
								}
							}
							lastGlyphRunEnd = next;
						} while(scriptRun.position < boost::const_end(textString) || styleRun.position < boost::const_end(textString));
						assert(boost::size(glyphRuns) == boost::size(scriptPointers));
					}

					// 3. merge script runs and style runs into TextRunImpls
					std::vector<TextRunImpl*> mergedTextRuns;
					mergedTextRuns.reserve(boost::size(glyphRuns) + boost::size(styleRuns));
					{
						auto glyphRun(std::begin(glyphRuns));
						auto lastGlyphRun(boost::const_end(glyphRuns));
						auto styleRun(boost::const_begin(styleRuns)), lastStyleRun(boost::const_end(styleRuns));
						do {
							auto nextGlyphRun(glyphRun + 1);
							auto nextStyleRun(styleRun + 1);
							const StringPiece::const_iterator
								nextGlyphRunPosition((nextGlyphRun != lastGlyphRun) ? (*nextGlyphRun)->position : boost::const_end(textString)),
								nextStyleRunPosition((nextStyleRun != lastStyleRun) ? nextStyleRun->position : boost::const_end(textString));
							const StringPiece::const_iterator nextPosition(std::min(nextGlyphRunPosition, nextStyleRunPosition));
							const StringPiece::const_iterator previousPosition(!boost::empty(mergedTextRuns) ? mergedTextRuns.back()->end() : boost::const_begin(textString));

							mergedTextRuns.push_back(new TextRunImpl(
								makeStringPiece(previousPosition, nextPosition),
								*scriptPointers[boost::size(glyphRuns) - (lastGlyphRun - glyphRun)], std::move(*glyphRun),
								ActualTextRunStyleCore(styleRun->attribute, lengthContext, measure)));
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
#if 0
				pair<StringPiece::const_pointer, shared_ptr<const Font>> findNextFontRun(
					const StringPiece& textString, const FontCollection& fontCollection,
					const ComputedTextRunStyle& style, shared_ptr<const Font> previousFont);

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
						const OPENTYPE_TAG scriptTags[], std::size_t numberOfScriptRuns,
						const FontCollection& fontCollection, shared_ptr<const TextRunStyle> defaultStyle,
						std::unique_ptr<ComputedStyledTextRunIterator> styles, vector<TextRunImpl*>& textRuns,
						std::vector<const ComputedTextRunStyle>& computedStyles,
						std::vector<std::vector<const ComputedTextRunStyle>::size_type>& computedStylesIndices) {
					raiseIfNullOrEmpty(layoutString, "layoutString");
					if(scriptRuns == nullptr)
						throw NullPointerException("scriptRuns");
					else if(numberOfScriptRuns == 0)
						throw invalid_argument("numberOfScriptRuns");

#define ASCENSION_SPLIT_LAST_RUN()												\
	while(runs.back()->length() > MAXIMUM_RUN_LENGTH) {							\
		TextRunImpl& back = *runs.back();										\
		std::unique_ptr<TextRunImpl> piece(new SimpleRun(back.style));			\
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
					std::vector<TextRunImpl*> calculatedRuns;
					std::vector<const ComputedTextRunStyle> calculatedStyles;
					calculatedRuns.reserve(static_cast<std::size_t>(numberOfScriptRuns * ((styles.get() != nullptr) ? 1.2 : 1)));	// hmm...
					std::vector<std::vector<const ComputedTextRunStyle>::size_type> calculatedStylesIndices;
					calculatedStylesIndices.reserve(calculatedRuns.capacity());

					// script cursors
					AttributedCharacterRange<const SCRIPT_ITEM*> scriptRun;
					scriptRun.attribute = scriptRuns;
					scriptRun.position = layoutString.beginning() + scriptRun.attribute->iCharPos;
					AttributedCharacterRange<const SCRIPT_ITEM*> nextScriptRun;
					nextScriptRun.attribute = scriptRuns + 1;
					nextScriptRun.position = (nextScriptRun.attribute < scriptRuns + numberOfScriptRuns) ?
						layoutString.beginning() + nextScriptRun.attribute->iCharPos : boost::const_end(layoutString);

					// style cursors
					detail::ComputedStyledTextRunEnumerator styleEnumerator(layoutString, std::move(styles));
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
						nextStyleRun.position = boost::const_end(layoutString);

					assert(scriptRun.position == layoutString.beginning());
					assert(styleRun.position == layoutString.beginning());

					std::shared_ptr<const Font> font;	// font for current glyph run
					do {
						const StringPiece::const_pointer previousRunEnd = max(scriptRun.position, styleRun.position);
						assert(
							(previousRunEnd == layoutString.beginning() && boost::empty(calculatedRuns) && boost::empty(calculatedStyles))
							|| (!boost::empty(calculatedRuns) && previousRunEnd == calculatedRuns.back()->end())
							|| (!boost::empty(calculatedStyles) && previousRunEnd == styleRun.position));
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
							const std::pair<StringPiece::const_pointer, std::shared_ptr<const Font>> nextFontRun(
								findNextFontRun(makeStringPiece(previousRunEnd, newRunEnd), fontCollection,
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
								new TextRunImpl(Range<Index>(!boost::empty(calculatedRuns) ? calculatedRuns.back()->end() : 0, newRunEnd - layoutString.beginning()),
									scriptRun.attribute->a, font,
									(scriptTags != nullptr) ? scriptTags[scriptRun.attribute - scriptRuns] : SCRIPT_TAG_UNKNOWN));	// TODO: 'DFLT' is preferred?
							calculatedStylesIndices.push_back(boost::size(calculatedStyles));
							while(true) {
								std::unique_ptr<TextRunImpl> piece(calculatedRuns.back()->breakIfTooLong());
								if(piece.get() == nullptr)
									break;
								calculatedRuns.push_back(piece.release());
								calculatedStylesIndices.push_back(boost::size(calculatedStyles));
							}
							if(breakScriptRun)
								const_cast<SCRIPT_ITEM*>(scriptRun.attribute)->a.fLinkBefore = 0;
						}
						if(forwardScriptRun) {
							assert(nextScriptRun.position < boost::const_end(layoutString));
							scriptRun = nextScriptRun;
							nextScriptRun.position =
								(++nextScriptRun.attribute < scriptRuns + numberOfScriptRuns) ?
									layoutString.beginning() + nextScriptRun.attribute->iCharPos : boost::const_end(layoutString);
						}
						if(forwardStyleRun) {
							assert(nextStyleRun.position < boost::const_end(layoutString));
							styleRun = std::move(nextStyleRun);
							calculatedStyles.push_back(styleRun.attribute);
							assert(!styleEnumerator.isDone());
							styleEnumerator.next();
							if(!styleEnumerator.isDone()) {
								nextStyleRun.position = styleEnumerator.position();
								styleEnumerator.style(nextStyleRun.attribute);
							} else
								nextStyleRun.position = boost::const_end(layoutString);
						}
					} while(scriptRun.position < boost::const_end(layoutString) || styleRun.position < boost::const_end(layoutString));

					assert(boost::size(calculatedRuns) == boost::size(calculatedStylesIndices));
					assert(!boost::empty(calculatedStyles));

					// commit
					using std::swap;
					swap(textRuns, calculatedRuns);
					swap(computedStyles, calculatedStyles);
					swap(computedStylesIndices, calculatedStylesIndices);

#undef ASCENSION_SPLIT_LAST_RUN
				}
#endif // 0

				/**
				 * Positions the glyphs in the text run.
				 * @param dc The device context
				 * @param style The computed text run style
				 * @see #generate, GlyphVectorImpl#positionGlyphs, GlyphVectorImpl#substituteGlyphs
				 */
				void TextRunImpl::positionGlyphs(win32::Handle<HDC>::Type dc, const presentation::ComputedTextRunStyle& style) {
					return positionGlyphs(dc);

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
										std::memcpy(glyphIndices.get(), glyphs(), sizeof(WORD) * numberOfGlyphs());
									}
									glyphIndices[i] = fp.wgBlank;
								}
							}
						}
					}
*/
/*					// handle letter spacing
					if(styledRange.style.get() != nullptr && styledRange.style->letterSpacing.unit != Length::INHERIT) {
						if(const int letterSpacing = pixels(dc, styledRange.style->letterSpacing, false, glyphs_->font->metrics())) {
							const bool rtl = readingDirection() == RIGHT_TO_LEFT;
							for(std::size_t i = textRun.glyphRange_.beginning(), e = textRun.glyphRange_.end(); i < e; ++i) {
								if((!rtl && (i + 1 == e || glyphs_->visualAttributes[i + 1].fClusterStart != 0))
										|| (rtl && (i == 0 || glyphs_->visualAttributes[i - 1].fClusterStart != 0))) {
									advances[i] += letterSpacing;
									if(rtl)
										offsets[i].du += letterSpacing;
								}
							}
						}
					}
*/				}
			}


			// TextLayout /////////////////////////////////////////////////////////////////////////////////////////////

			// helpers for TextLayout.draw
			namespace {
				const std::size_t MAXIMUM_RUN_LENGTH = 1024;
				inline win32::Handle<HPEN>::Type createPen(const Color& color, int width, int style) {
					if(color.alpha() < 0xff)
						throw std::invalid_argument("color");
					LOGBRUSH brush;
					brush.lbColor = toNative<COLORREF>(color);
					brush.lbStyle = BS_SOLID;
					HPEN pen = nullptr;
					switch(style) {
					case 1:	// solid
						pen = (width == 1) ? ::CreatePen(PS_SOLID, 1, toNative<COLORREF>(color))
							: ::ExtCreatePen(PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_FLAT, width, &brush, 0, nullptr);
					case 2:	// dashed
						pen = ::ExtCreatePen(PS_GEOMETRIC | PS_DASH | PS_ENDCAP_FLAT, width, &brush, 0, nullptr);
					case 3:	// dotted
						pen = ::ExtCreatePen(PS_GEOMETRIC | PS_DOT | PS_ENDCAP_FLAT, width, &brush, 0, nullptr);
					}
					if(pen == nullptr)
						throw UnknownValueException("style");
					return win32::Handle<HPEN>::Type(pen, &::DeleteObject);
				}
			} // namespace @0

			namespace {
				inline AffineTransform&& fontRotationForWritingMode(presentation::BlockFlowDirection blockFlowDirection) {
					switch(blockFlowDirection) {
						case presentation::HORIZONTAL_TB:
							return AffineTransform();
						case presentation::VERTICAL_RL:
							return geometry::makeQuadrantRotationTransform(1);
						case presentation::VERTICAL_LR:
							return geometry::makeQuadrantRotationTransform(3);
						default:
							throw UnknownValueException("blockFlowDirection");
					}
				}
			}

			/**
			 * Returns the black box bounds of the characters in the specified range. The black box bounds is
			 * an area consisting of the union of the bounding boxes of the all of the characters in the range.
			 * The result region can be disjoint.
			 * @param characterRange The character range
			 * @param[out] bounds The polygon object encompasses the black box bounds
			 * @throw IndexOutOfBoundsException @a range intersects with the outside of the line
			 * @see #bounds(void), #bounds(Index, Index), #lineBounds, #lineStartEdge
			 */
			void TextLayout::blackBoxBounds(const boost::integer_range<Index>& characterRange,
					boost::geometry::model::multi_polygon<boost::geometry::model::polygon<Point>>& bounds) const {
				const Index firstCharacter = std::min(*boost::const_begin(characterRange), *boost::const_end(characterRange));
				const Index lastCharacter = std::max(*boost::const_begin(characterRange), *boost::const_end(characterRange));
				if(lastCharacter > numberOfCharacters())
					throw IndexOutOfBoundsException("characterRange");
				std::remove_reference<decltype(bounds)>::type result;

				// handle empty line
				if(isEmpty())
					return std::swap(result, bounds);

				// traverse all text runs intersect with 'characterRange'
				const presentation::WritingMode wm(writingMode(*this));
				const auto lines(boost::irange(lineAt(TextHit<>::afterOffset(firstCharacter)), lineAt(TextHit<>::beforeOffset(lastCharacter)) + 1));
				LineMetricsIterator lm(*this, lines.front());
				BOOST_FOREACH(Index line, lines) {
					// move to line-left edge of the line
					Point runTypographicOrigin = lm.baselineOffsetInPhysicalCoordinates();
					const Point ll = lineLeft(line);
					if(geometry::x(ll) != 0)
						geometry::x(runTypographicOrigin) = geometry::x(ll);
					else if(geometry::y(ll) != 0)
						geometry::y(runTypographicOrigin) = geometry::y(ll);

					const boost::iterator_range<RunVector::const_iterator> runs(runsForLine(line));
					for(RunVector::const_iterator run(boost::const_begin(runs)); run != boost::const_end(runs); ++run, ++lm) {
						const auto runRange(characterIndices(**run, textString_));
						const auto intersection = ascension::intersection(runRange, characterRange);
						if(intersection != boost::none) {
							const std::ptrdiff_t beginningOfRun = boost::const_begin((*run)->characterRange()) - textString_.data();
							const boost::integer_range<Index> offsetsInRun = boost::irange(*boost::const_begin(boost::get(intersection)) - beginningOfRun, *boost::const_end(boost::get(intersection)) - beginningOfRun);
							std::vector<graphics::Rectangle> runBlackBoxBounds;
							static_cast<const TextRunImpl&>(**run).charactersBounds(offsetsInRun, runBlackBoxBounds);
							AffineTransform typographicalToPhysicalMapping(geometry::makeTranslationTransform(
								geometry::_tx = geometry::x(runTypographicOrigin), geometry::_ty = geometry::y(runTypographicOrigin)));
							if(isVertical(wm.blockFlowDirection)) {
//								geometry::quadrantRotate(typographicalToPhysicalMapping, (resolveTextOrientation(writingMode()) != presentation::SIDEWAYS_LEFT) ? -1 : +1);
								typographicalToPhysicalMapping = AffineTransform(
									boost::numeric::ublas::prod(
										typographicalToPhysicalMapping.matrix(), 
										geometry::makeQuadrantRotationTransform((resolveTextOrientation(wm) != presentation::SIDEWAYS_LEFT) ? -1 : +1).matrix()));
							}
							BOOST_FOREACH(const graphics::Rectangle& typographicBounds, runBlackBoxBounds) {
								// map typographic rectangle into physical coordinates

								graphics::Rectangle physicalBounds;
								boost::geometry::transform(typographicBounds, physicalBounds, typographicalToPhysicalMapping);
								boost::geometry::model::polygon<Point> temp;
								boost::geometry::append(temp, boost::geometry::box_view<graphics::Rectangle>(physicalBounds));
								result.push_back(temp);
							}
						}

						// move to the line-left edge of the next run
						if(isHorizontal(wm.blockFlowDirection))
							geometry::x(runTypographicOrigin) += boost::size(allocationMeasure(**run));
						else if(resolveTextOrientation(wm) != presentation::SIDEWAYS_LEFT)
							geometry::y(runTypographicOrigin) += boost::size(allocationMeasure(**run));
						else
							geometry::y(runTypographicOrigin) -= boost::size(allocationMeasure(**run));
					}
				}
				return std::swap(result, bounds);
			}

			namespace {
				template<template<typename> class FourSides>
				inline bool borderShouldBePainted(const FourSides<ActualBorderSide>& borders) {
					const std::array<ActualBorderSide, 4>& temp = borders;	// sigh...
					BOOST_FOREACH(auto& border, temp) {
						if(border.hasVisibleStyle())
							return true;
					}
					return false;
				}
			}

			/**
			 * Draws the specified line layout to the output device.
			 * @param context The rendering context
			 * @param origin The alignment point of the text layout
			 * @param overriddenSegments A sequence of @c OverriddenSegment
			 * @param endOfLine The inline object which paints an end-of-line. Can be @c null
			 * @param lineWrappingMark The inline object which paints line-wrapping-mark. Can be @c null
			 */
			void TextLayout::draw(PaintContext& context,
					const Point& origin, const std::vector<OverriddenSegment>& overriddenSegments /* = empty */,
					const InlineObject* endOfLine/* = nullptr */, const InlineObject* lineWrappingMark /* = nullptr */) const {
#if defined(_DEBUG) && defined(ASCENSION_DIAGNOSE_INHERENT_DRAWING)
				ASCENSION_LOG_TRIVIAL(debug) << L"@TextLayout.draw draws line " << lineNumber_ << L" (" << line << L")\n";
#endif
				if(!isVertical(*this)) {
					if(geometry::dy(context.boundsToPaint()) == 0)
						return;
				} else {
					if(geometry::dx(context.boundsToPaint()) == 0)
						return;
				}
				if(isEmpty())
					return;

				// this code paints the line in the following steps:
				// 1. calculate lines to paint
				// 2. paint backgrounds and borders:
				//   2-1. paint 'allocation-rectangle'
				//   2-2. compute 'content-rectangle'
				//   2-3. paint background (if the property is specified)
				//   2-4. paint borders (if the property is specified)
				// 3. for each text runs:
				//   3-1. calculate range of runs to paint
				//   3-2. paint the glyphs of the text run
				//   3-3. paint the overhanging glyphs of the around text runs
				//   3-4. paint the text decoration
				// 4. paint the end of line mark
				//
				// the following topics describe how to draw a styled and selected text using masking by clipping
				// Catch 22 : Design & Implementation of a Win32 Text Editor
				// - Transparent Text (http://www.catch22.net/tuts/transparent-text)
				// - Drawing styled text with Uniscribe (http://www.catch22.net/tuts/drawing-styled-text-uniscribe)

				// 1. calculate lines to paint
				const presentation::WritingMode wm(writingMode(*this));
				boost::integer_range<Index> linesToPaint(0, numberOfLines());
				{
					graphics::Rectangle boundsToPaint;
					geometry::translate(
						geometry::_from = context.boundsToPaint(), geometry::_to = boundsToPaint,
						geometry::_tx = -geometry::x(origin), geometry::_ty = -geometry::y(origin));
					presentation::FlowRelativeFourSides<Scalar> abstractBoundsToPaint;	// relative to the alignment point of this layout
					presentation::mapDimensions(wm, presentation::_from = PhysicalFourSides<Scalar>(boundsToPaint), presentation::_to = abstractBoundsToPaint);
					for(LineMetricsIterator line(*this, linesToPaint.front()); line.line() != *boost::const_end(linesToPaint); ++line) {
						const Scalar bpd = line.baselineOffset();
						const Scalar lineBeforeEdge = bpd - line.ascent();
						const Scalar lineAfterEdge = bpd + line.descent();
						if(lineBeforeEdge <= abstractBoundsToPaint.before() && lineAfterEdge > abstractBoundsToPaint.before())
							linesToPaint = boost::irange(line.line(), *boost::const_end(linesToPaint));
						if(lineBeforeEdge <= abstractBoundsToPaint.after() && lineAfterEdge > abstractBoundsToPaint.after()) {
							linesToPaint = boost::irange(*boost::const_begin(linesToPaint), line.line() + 1);
							break;
						}
					}
				}
				context.save();
//				context.setTextAlign();
//				context.setTextBaseline();
//				::SetTextAlign(context.nativeObject().get(), TA_TOP | TA_LEFT | TA_NOUPDATECP);

				// 2. paint backgrounds and borders
				const bool horizontalLayout = isHorizontal(wm.blockFlowDirection);
				assert(horizontalLayout || isVertical(wm.blockFlowDirection));
				struct TextRunToPaint {
					const TextRunImpl& impl;
					const graphics::Rectangle contentRectangle;
					const Point alignmentPoint;
					TextRunToPaint(const TextRunImpl& textRun, const graphics::Rectangle& contentRectangle,
						const Point& alignmentPoint) : impl(textRun), contentRectangle(contentRectangle), alignmentPoint(alignmentPoint) {}
				};
				std::vector<TextRunToPaint> textRunsToPaint;
				struct OverriddenSegmentToPaint {
					const std::size_t indexInTextRunsToPaint;
					const OverriddenSegment& segment;
					const Rectangle bounds;
					OverriddenSegmentToPaint(const std::size_t i, const OverriddenSegment& segment,
						const Rectangle bounds) : indexInTextRunsToPaint(i), segment(segment), bounds(bounds) {}
				};
				std::vector<OverriddenSegmentToPaint> overriddenSegmentsToPaint;
				for(LineMetricsIterator line(*this, linesToPaint.front()); line.line() != *boost::const_end(linesToPaint); ++line) {
					Point lineLeftPoint(origin);	// position of the baseline on the 'line-left' edge of this 'line-area'
					{
						PhysicalTwoAxes<Scalar> delta;
						presentation::mapDimensions(wm,
							presentation::_from = LineRelativePoint<Scalar>(
								_u = (wm.inlineFlowDirection == presentation::LEFT_TO_RIGHT) ?
									lineStartEdge(line.line()) : (measure(line.line()) - lineStartEdge(line.line())),
								_v = line.baselineOffset()),
							presentation::_to = delta);
						geometry::translate(lineLeftPoint, (geometry::_tx = delta.x(), geometry::_ty = delta.y()));
					}

					LineRelativePoint<Scalar> p(
						_u = horizontalLayout ? geometry::x(origin) : geometry::y(origin),
						_v = horizontalLayout ? geometry::y(origin) : geometry::x(origin));
					if(wm.inlineFlowDirection == presentation::LEFT_TO_RIGHT)
						p.u() += lineStartEdge(line.line());
					else
						p.u() += measure(line.line()) - lineStartEdge(line.line());
					p.v() += line.baselineOffset();

					LineRelativeFourSides<Scalar> runAllocationBox;	// relative to 'lineLeftPoint'
					runAllocationBox.lineOver() = p.v() - line.ascent();
					runAllocationBox.lineUnder() = p.v() + line.descent();
//					context.setGlobalAlpha(1.0);
//					context.setGlobalCompositeOperation(SOURCE_OVER);
					BOOST_FOREACH(const std::unique_ptr<const TextRun>& run, runsForLine(line.line())) {
						// check if this text run is beyond bounds to paint
						// TODO: Consider overhangs.
						if((horizontalLayout && p.u() >= geometry::right(context.boundsToPaint()))
								|| (!horizontalLayout && p.v() >= geometry::bottom(context.boundsToPaint())))
							break;

						// compute next position of 'p', 'border-box' and 'allocation-box'
						auto q(p);
						q.u() += boost::size(allocationMeasure(*run));
						bool skipThisRun = p == q;	// skip empty box

						// compute 'allocation-rectangle' of this text run
						Rectangle runAllocationRectangle;
						if(!skipThisRun) {
							runAllocationBox.lineLeft() = p.u();
							runAllocationBox.lineRight() = q.u();
							PhysicalFourSides<Scalar> r;
							presentation::mapDimensions(wm, presentation::_from = runAllocationBox, presentation::_to = r);
							runAllocationRectangle = geometry::make<Rectangle>(r);

							// check if this text run intersects with bounds to paint
							// TODO: Consider overhangs.
							skipThisRun = !boost::geometry::intersects(runAllocationRectangle, context.boundsToPaint());
						}
						if(!skipThisRun) {
							// 2-1. paint 'allocation-rectangle'
							const auto background(boost::fusion::at_key<presentation::styles::BackgroundColor>(defaultRunStyle().backgroundsAndBorders));
							if(!background.isFullyTransparent()) {
								const SolidColor backgroundColor(background);
								context.setFillStyle(std::shared_ptr<const SolidColor>(&backgroundColor, boost::null_deleter()));
								context.fillRectangle(runAllocationRectangle);
							}

							// 2-2. compute 'alignment-point' and 'content-rectangle'
							Point runAlignmentPoint;
							{
								PhysicalFourSides<Scalar> temp;
								presentation::mapDimensions(wm, presentation::_from = allocationBox(*run), presentation::_to = temp);
								geometry::translate(
									geometry::_from = geometry::topLeft(runAllocationRectangle), geometry::_to = runAlignmentPoint,
									geometry::_tx = -temp.left(), geometry::_ty = -temp.top());
							}
							Rectangle runContentRectangle;
							{
								PhysicalFourSides<Scalar> temp;
								presentation::mapDimensions(wm, presentation::_from = contentBox(*run), presentation::_to = temp);
								geometry::translate(
									geometry::_from = geometry::make<Rectangle>(temp), geometry::_to = runContentRectangle,
									geometry::_tx = geometry::x(runAlignmentPoint), geometry::_ty = geometry::y(runAlignmentPoint));
							}

							// 2-3. store this text run to paint the glyphs
#ifdef BOOST_NO_CXX11_VARIADIC_TEMPLATES
							textRunsToPaint.push_back(TextRunToPaint(static_cast<const TextRunImpl&>(*run), runContentRectangle, runAlignmentPoint));
#else
							textRunsToPaint.emplace_back(static_cast<const TextRunImpl&>(*run), runContentRectangle, runAlignmentPoint);
#endif

							// 2-3. compute 'border-rectangle' if needed
							const auto& runStyle = static_cast<const TextRunImpl&>(*run).style();
							graphics::Rectangle runBorderRectangle;
							if(!runStyle.backgroundColor.isFullyTransparent() || borderShouldBePainted(runStyle.borders)) {
								PhysicalFourSides<Scalar> temp;
								presentation::mapDimensions(wm, presentation::_from = borderBox(*run), presentation::_to = temp);
								geometry::translate(
									geometry::_from = geometry::make<Rectangle>(temp), geometry::_to = runBorderRectangle,
									geometry::_tx = geometry::x(runAlignmentPoint), geometry::_ty = geometry::y(runAlignmentPoint));
							}

							// 2-4. paint background
							if(!runStyle.backgroundColor.isFullyTransparent()) {
								const SolidColor fill(runStyle.backgroundColor);
								context.setFillStyle(std::shared_ptr<const Paint>(&fill, boost::null_deleter()));
								context.fillRectangle(runBorderRectangle);
							}

							// 2-5. paint overridden segments background
							{
								const auto runCharacterIndices(characterIndices(*run, textString_));
								BOOST_FOREACH(const OverriddenSegment& segment, overriddenSegments) {
									const auto overriddenRange(intersection(segment.range, runCharacterIndices));
									if(overriddenRange == boost::none)
										continue;

									LineRelativeFourSides<Scalar> abstractOverriddenRectangle;
									abstractOverriddenRectangle.lineLeft() =
										run->hitToLogicalPosition(TextHit<>::afterOffset(*boost::const_begin(boost::get(overriddenRange)) - *boost::const_begin(runCharacterIndices)));
									abstractOverriddenRectangle.lineRight() =
										run->hitToLogicalPosition(TextHit<>::beforeOffset(*boost::const_end(boost::get(overriddenRange)) - *boost::const_begin(runCharacterIndices)));
									if(segment.usesLogicalHighlightBounds) {
										const auto extent(line.extentWithHalfLeadings());
										abstractOverriddenRectangle.lineOver() = *boost::const_begin(extent);
										abstractOverriddenRectangle.lineUnder() = *boost::const_end(extent);
									} else {
										// TODO:
									}
									PhysicalFourSides<Scalar> physicalOverriddenRectangle;
									presentation::mapDimensions(wm, presentation::_from = abstractOverriddenRectangle, presentation::_to = physicalOverriddenRectangle);
									Rectangle overriddenRectangle;
									geometry::translate(
										geometry::_from = geometry::make<Rectangle>(physicalOverriddenRectangle), geometry::_to = overriddenRectangle,
										geometry::_tx = geometry::x(runAlignmentPoint), geometry::_ty = geometry::y(origin));
									if(segment.background.get() != nullptr) {
										context.setFillStyle(segment.background);
										context.fillRectangle(overriddenRectangle);
									}

									// mark that this text run has an overridden segment
#ifdef BOOST_NO_CXX11_VARIADIC_TEMPLATES
									overriddenSegmentsToPaint.push_back(OverriddenSegmentToPaint(textRunsToPaint.size() - 1, segment, overriddenRectangle));
#else
									overriddenSegmentsToPaint.emplace_back(textRunsToPaint.size() - 1, segment, overriddenRectangle);
#endif
								}
							}

							// 2-6. paint borders
							PhysicalFourSides<const ActualBorderSide*> physicalBorders;
							for(auto border(std::begin(runStyle.borders)), e(std::end(runStyle.borders)); border != e; ++border) {
								const presentation::FlowRelativeDirection direction = static_cast<presentation::FlowRelativeDirection>(border - std::begin(runStyle.borders));
								physicalBorders[mapDirection(writingMode(*this), direction)] = &*border;
							}
							for(auto border(std::begin(physicalBorders)), e(std::end(physicalBorders)); border != e; ++border) {
								if(!(*border)->hasVisibleStyle()) {
									*border = nullptr;
									continue;
								}
								const PhysicalDirection direction = static_cast<PhysicalDirection>(border - std::begin(physicalBorders));
							}
						}

//						::ExcludeClipRect(context.asNativeObject().get(),
//							geometry::left(paintedRectangle), geometry::top(paintedRectangle),
//							geometry::right(paintedRectangle), geometry::bottom(paintedRectangle));

						// move 'p' to next text run
						p = q;
					}
				}

				// 3. for each text runs
				BOOST_FOREACH(auto& textRun, textRunsToPaint) {
					const SolidColor foreground(textRun.impl.style().color);
					context.setFillStyle(std::shared_ptr<const SolidColor>(&foreground, boost::null_deleter()));
					textRun.impl.fillGlyphs(context, textRun.alignmentPoint);
				}

				// . paint overridden segments glyphs
				BOOST_FOREACH(auto& segment, overriddenSegmentsToPaint) {
#if 0
					const auto& foreground = segment.segment.foreground;
					if(foreground.get() == nullptr)
						continue;
#else
					const auto& foregroundColor = segment.segment.color;
					if(foregroundColor == boost::none)
						continue;
					const auto foreground(std::make_shared<SolidColor>(boost::get(foregroundColor)));
#endif
					context.save();
					context.beginPath();
					context.rectangle(segment.bounds);
					context.clip();
					const TextRunImpl& textRun = textRunsToPaint[segment.indexInTextRunsToPaint].impl;
					context.setFillStyle(foreground);
					textRun.fillGlyphs(context, textRunsToPaint[segment.indexInTextRunsToPaint].alignmentPoint);
					context.restore();
				}
				context.restore();
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
			String TextLayout::fillToX(Scalar x) const {
#if 0
				int cx = longestLineWidth();
				if(cx >= x)
					return L"";

				std::size_t numberOfTabs = 0;
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
				std::size_t numberOfSpaces = 0;
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

			class TextLayout::TabSize {
			public:
				typedef presentation::styles::ComputedValue<presentation::styles::TabSize>::type Object;
				explicit TabSize(const Object& object) BOOST_NOEXCEPT : object_(object) {}
				BOOST_CONSTEXPR const Object& get() const BOOST_NOEXCEPT {return object_;}
			private:
				const Object& object_;
			};

			void TextLayout::initialize(
					std::unique_ptr<presentation::ComputedStyledTextRunIterator> textRunStyles,
					const presentation::styles::Length::Context& lengthContext,
					const Dimension& parentContentArea,
					const FontCollection& fontCollection, const FontRenderContext& fontRenderContext) {
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
				// calculate the nominal font
				std::shared_ptr<const Font> nominalFont;
				presentation::Pixels nominalFontSize;
				{
					const auto& nominalFontStyles = defaultRunStyle().fonts;
					const auto& nominalFontFamilies(boost::fusion::at_key<presentation::styles::FontFamily>(nominalFontStyles));
					const FontProperties nominalFontProperties(
						boost::fusion::at_key<presentation::styles::FontWeight>(nominalFontStyles),
						boost::fusion::at_key<presentation::styles::FontStretch>(nominalFontStyles),
						boost::fusion::at_key<presentation::styles::FontStyle>(nominalFontStyles));
					nominalFontSize = presentation::Pixels(
						presentation::styles::useFontSize(boost::fusion::at_key<presentation::styles::FontSize>(nominalFontStyles), lengthContext,
							presentation::styles::useFontSize(boost::fusion::at_key<presentation::styles::FontSize>(defaultRunStyle().fonts),
								lengthContext, presentation::styles::HANDLE_AS_ROOT)));
					if(!nominalFontFamilies.empty()) {
						const FontDescription nominalFontDescription(FontFamily(nominalFontFamilies.front()), nominalFontSize.value(), nominalFontProperties);
						nominalFont = fontCollection.get(nominalFontDescription,
							fontRotationForWritingMode(boost::fusion::at_key<presentation::styles::WritingMode>(parentStyle())),
							boost::fusion::at_key<presentation::styles::FontSizeAdjust>(nominalFontStyles));
					} else
						nominalFont = fontCollection.lastResortFallback(nominalFontSize.value(),
							nominalFontProperties, geometry::makeIdentityTransform(), boost::fusion::at_key<presentation::styles::FontSizeAdjust>(nominalFontStyles));
				}
				assert(nominalFont.get() != nullptr);

				// split the text line into text runs as following steps:
				// 1. split the text into script runs (SCRIPT_ITEMs) by Uniscribe
				// 2. split each script runs into atomically-shapable runs (TextRuns) with StyledRunIterator
				// 3. generate glyphs for each text runs
				// 4. position glyphs for each text runs
				// 5. position each text runs
				// 6. justify each text runs if specified
				// 7. stack the lines

				const RenderingContext2D context(win32::detail::screenDC());
				if(!textString_.empty()) {
					const auto computedMeasure(boost::fusion::at_key<presentation::styles::Measure>(style()));
					Scalar actualMeasure;
					if(const presentation::styles::Length* const length = boost::get<presentation::styles::Length>(&computedMeasure))
						actualMeasure = length->value(lengthContext);
					else {
						presentation::styles::Percentage percentage;
						if(const presentation::styles::Percentage* const computed = boost::get<presentation::styles::Percentage>(&computedMeasure))
							percentage = *computed;
						else
							percentage = 1;	// 100% as default
						if(presentation::isHorizontal(boost::fusion::at_key<presentation::styles::WritingMode>(parentStyle())))
							actualMeasure = geometry::dx(parentContentArea) * boost::rational_cast<Scalar>(percentage);
						else
							actualMeasure = geometry::dy(parentContentArea) * boost::rational_cast<Scalar>(percentage);
					}

					// 2. split each script runs into text runs with StyledRunIterator
					std::vector<TextRunImpl*> textRuns;
					std::vector<AttributedCharacterRange<presentation::ComputedTextRunStyle>> calculatedStyles;
					TextRunImpl::generate(textString_, style(), std::move(textRunStyles),
						lengthContext, actualMeasure, fontCollection, fontRenderContext, nominalFontSize, textRuns, calculatedStyles);

					// 3. generate glyphs for each text runs
					BOOST_FOREACH(TextRunImpl* run, textRuns)
						run->shape(context.native());
					TextRunImpl::substituteGlyphs(boost::make_iterator_range(textRuns));

					// 4. position glyphs for each text runs
					{
						auto calculatedStyle(boost::const_begin(calculatedStyles));
						BOOST_FOREACH(TextRunImpl* run, textRuns) {
							while(calculatedStyle < boost::const_end(calculatedStyles) && calculatedStyle->position < boost::const_begin(*run))
								++calculatedStyle;
							run->positionGlyphs(context.native(), calculatedStyle->attribute);
						}
					}

					// 5. position each text runs
					const auto tabSize(boost::fusion::at_key<presentation::styles::TabSize>(style()));

					// wrap into visual lines and reorder runs in each lines
					if(boost::empty(textRuns) || !wrapsText(boost::fusion::at_key<presentation::styles::WhiteSpace>(style()))) {
						numberOfLines_ = 1;
						assert(firstRunsInLines_.get() == nullptr);
						// 5-1. expand horizontal tabs (with logical ordered runs)
						{
							Scalar ipd = 0;
							// for each runs... (at this time, 'textRuns' is in logical order)
							BOOST_FOREACH(TextRunImpl* run, textRuns) {
								run->expandTabCharacters(context, tabSize, lengthContext, textString_, ipd, boost::none);
								ipd += boost::size(allocationMeasure(*run));
							}
							maximumMeasure_ = ipd;
						}
						// 5-2. reorder each text runs
						runs_.reserve(boost::size(textRuns));
						BOOST_FOREACH(TextRunImpl* run, textRuns)
							runs_.push_back(std::unique_ptr<const TextRun>(run));
						reorder();
						// 5-3. reexpand horizontal tabs
//						expandTabsWithoutWrapping();
					} else {
						const auto textJustification(boost::fusion::at_key<presentation::styles::TextJustification>(style()));
						const bool justifyRuns = textJustification != TextJustification::NONE;
						// 5-1. expand horizontal tabs and wrap into lines
						runs_.reserve(boost::size(textRuns));
						BOOST_FOREACH(TextRunImpl* run, textRuns) {
							if(justifyRuns)
								run->reserveJustification();
							runs_.push_back(std::unique_ptr<const TextRun>(run));
						}
						wrap(context, TabSize(tabSize), lengthContext, actualMeasure);
						// 5-2. reorder each text runs
						reorder();
						// 5-3. reexpand horizontal tabs
						// TODO: not implemented.
						// 6. justify each text runs if specified
						if(justifyRuns)
							justify(actualMeasure, textJustification);
					}
				} else {
					// handle logically empty line
					numberOfLines_ = 1;
					maximumMeasure_ = 0.0f;
					assert(isEmpty());
				}

				// 7. stack the lines
				const auto& computedLineHeight = boost::fusion::at_key<presentation::styles::LineHeight>(style());
				presentation::styles::Length lineHeight;
				if(const presentation::styles::Number* number = boost::get<presentation::styles::Number>(&computedLineHeight))
					lineHeight.newValueSpecifiedUnits(presentation::styles::Length::EM_HEIGHT, *number);
				else if(const presentation::styles::Length* length = boost::get<presentation::styles::Length>(&computedLineHeight))
					lineHeight = *length;
				else if(const presentation::styles::Percentage* percentage = boost::get<presentation::styles::Percentage>(&computedLineHeight))
					lineHeight.newValueSpecifiedUnits(presentation::styles::Length::EM_HEIGHT, boost::rational_cast<presentation::styles::Number>(*percentage));
				else
					lineHeight.newValueSpecifiedUnits(presentation::styles::Length::EM_HEIGHT, 1.2f);
				stackLines(context, lineHeight, lengthContext,
					boost::fusion::at_key<presentation::styles::LineBoxContain>(style()), *nominalFont);
			}

			/// Justifies the wrapped visual lines.
			inline void TextLayout::justify(Scalar lineMeasure, TextJustification) BOOST_NOEXCEPT {
				for(Index line = 0; line < numberOfLines(); ++line) {
					const Scalar ipd = measure(line);
					for(auto i(firstRunInLine(line)), e(firstRunInLine(line + 1)); i != e; ++i) {
						TextRunImpl& run = *const_cast<TextRunImpl*>(static_cast<const TextRunImpl*>(i->get()));
						const Scalar newRunMeasure = boost::size(allocationMeasure(run)) * lineMeasure / ipd;	// TODO: There is more precise way.
						run.justify(static_cast<int>(newRunMeasure));
					}
				}

				// clear measures caches
				maximumMeasure_ = boost::none;
				lineMeasures_.reset();
			}

			/// Reorders the runs in visual order.
			inline void TextLayout::reorder() {
				assert(!boost::empty(runs_));
				std::vector<const TextRun*> reordered(boost::size(runs_));
				for(Index line = 0; line < numberOfLines(); ++line) {
					const boost::iterator_range<RunVector::const_iterator> runsInLine(firstRunInLine(line), firstRunInLine(line + 1));
					const std::unique_ptr<BYTE[]> levels(new BYTE[boost::size(runsInLine)]);
					for(RunVector::const_iterator i(boost::const_begin(runsInLine)); i != boost::const_end(runsInLine); ++i)
						levels[i - boost::const_begin(runsInLine)] = static_cast<BYTE>((*i)->characterLevel() & 0x1f);
					const std::unique_ptr<int[]> log2vis(new int[boost::size(runsInLine)]);
					const HRESULT hr = ::ScriptLayout(static_cast<int>(boost::size(runsInLine)), levels.get(), nullptr, log2vis.get());
					if(FAILED(hr))
						throw makePlatformError(hr);
					for(RunVector::const_iterator i(boost::const_begin(runsInLine)); i != boost::const_end(runsInLine); ++i)
						reordered[boost::const_begin(runsInLine) - boost::const_begin(runs_) + log2vis[i - boost::const_begin(runsInLine)]] = i->get();
				}

				// commit
				BOOST_FOREACH(RunVector::reference run, runs_)
					run.release();
				for(RunVector::size_type i = 0, c(boost::size(runs_)); i < c; ++i)
					runs_[i].reset(reordered[i]);
			}

			/**
			 * @internal Locates the wrap points and resolves tab expansions.
			 * @param context
			 * @param tabSize
			 * @param lengthContext
			 * @param measure
			 */
			void TextLayout::wrap(const RenderingContext2D& context, const TabSize& tabSize,
					const presentation::styles::Length::Context& lengthContext, Scalar measure) BOOST_NOEXCEPT {
				assert(!isEmpty());
				assert(numberOfLines() == 0 && firstRunsInLines_.get() == nullptr);

				std::vector<Index> firstRunsInLines;
				firstRunsInLines.push_back(0);
				Scalar ipd1 = 0;	// addresses the beginning of the run. see x2
				std::unique_ptr<int[]> logicalWidths;
				std::unique_ptr<SCRIPT_LOGATTR[]> logicalAttributes;
				Index longestRunLength = 0;	// for efficient allocation
				std::vector<TextRun*> runs;
				runs.reserve(boost::size(runs_) * 3 / 2);
				std::vector<std::unique_ptr<TextRunImpl>> createdRuns;	// for only exception safety
				// for each runs... (at this time, 'runs_' is in logical order)
				BOOST_FOREACH(RunVector::const_reference p, runs_) {
					TextRunImpl* run = const_cast<TextRunImpl*>(static_cast<const TextRunImpl*>(p.get()));

					// if the run is a tab, expand and calculate actual width
					if(run->expandTabCharacters(context, tabSize.get(), lengthContext,
							textString_, (ipd1 < measure) ? ipd1 : 0, measure - ((ipd1 < measure) ? ipd1 : 0))) {
						if(ipd1 < measure) {
							ipd1 += boost::size(allocationMeasure(*run));
							runs.push_back(run);
						} else {
							ipd1 = boost::size(allocationMeasure(*run));
							runs.push_back(run);
							firstRunsInLines.push_back(boost::size(runs));
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
					hr = run->logicalAttributes(logicalAttributes.get());
					const String::const_pointer originalRunPosition = boost::const_begin(*run);
					Scalar measureInThisRun = 0;
					String::const_pointer lastBreakable = boost::const_begin(*run), lastGlyphEnd = boost::const_begin(*run);
					Scalar lastBreakableIpd = ipd1, lastGlyphEndIpd = ipd1;
					// for each characters in the run...
					for(StringPiece::const_iterator j = boost::const_begin(*run); j < boost::const_end(*run); ) {	// j is position in the LOGICAL line
						const Scalar ipd2 = ipd1 + measureInThisRun;
						// remember this opportunity
						if(logicalAttributes[j - boost::const_begin(*run)].fCharStop != 0) {
							lastGlyphEnd = j;
							lastGlyphEndIpd = ipd2;
							if(logicalAttributes[j - boost::const_begin(*run)].fSoftBreak != 0
									|| logicalAttributes[j - boost::const_begin(*run)].fWhiteSpace != 0) {
								lastBreakable = j;
								lastBreakableIpd = ipd2;
							}
						}
						// break if the width of the visual line overs the wrap width
						if(ipd2 + logicalWidths[j - boost::const_begin(*run)] > measure) {
							// the opportunity is the start of this run
							if(lastBreakable == boost::const_begin(*run)) {
								// break at the last glyph boundary if no opportunities
								if(boost::size(firstRunsInLines) || firstRunsInLines.back() == boost::size(runs)) {
									if(lastGlyphEnd == boost::const_begin(*run)) {	// break here if no glyph boundaries
										lastBreakable = j;
										lastBreakableIpd = ipd2;
									} else {
										lastBreakable = lastGlyphEnd;
										lastBreakableIpd = lastGlyphEndIpd;
									}
								}
							}

							// case 1: break at the start of the run
							if(lastBreakable == boost::const_begin(*run)) {
								assert(boost::empty(firstRunsInLines) || boost::size(runs) != firstRunsInLines.back());
								firstRunsInLines.push_back(boost::size(runs));
//								ASCENSION_LOG_TRIVIAL(debug) << L"broke the line at " << lastBreakable << L" where the run start.\n";
							}
							// case 2: break at the end of the run
							else if(lastBreakable == boost::const_end(*run)) {
								if(lastBreakable < textString_.data() + numberOfCharacters()) {
									assert(boost::empty(firstRunsInLines) || boost::size(runs) != firstRunsInLines.back());
									firstRunsInLines.push_back(boost::size(runs) + 1);
//									ASCENSION_LOG_TRIVIAL(debug) << L"broke the line at " << lastBreakable << L" where the run end.\n";
								}
								break;
							}
							// case 3: break at the middle of the run -> split the run (run -> newRun + run)
							else {
								std::unique_ptr<TextRunImpl> followingRun(run->breakAt(lastBreakable));
								runs.push_back(run);
								assert(boost::empty(firstRunsInLines) || boost::size(runs) != firstRunsInLines.back());
								firstRunsInLines.push_back(boost::size(runs));
//								ASCENSION_LOG_TRIVIAL(debug) << L"broke the line at " << lastBreakable << L" where the run meddle.\n";
								createdRuns.push_back(std::move(followingRun));	// C2668 if included <boost/log/trivial.hpp> without 'std::' ???
								run = createdRuns.back().get();	// continue the process about this run
							}
							measureInThisRun = ipd1 + measureInThisRun - lastBreakableIpd;
							lastBreakableIpd -= ipd1;
							lastGlyphEndIpd -= ipd1;
							ipd1 = 0;
							j = std::max(lastBreakable, j);
						} else
							measureInThisRun += logicalWidths[j++ - originalRunPosition];
					}
					runs.push_back(run);
					ipd1 += measureInThisRun;
				}
//				ASCENSION_LOG_TRIVIAL(debug) << L"...broke the all lines.\n";
#if 0
				if(boost::empty(runs))
					runs.push_back(nullptr);
#else
				assert(!boost::empty(runs));
#endif

				// commit
				decltype(runs_) newRuns(boost::size(runs));
				firstRunsInLines_.reset(new RunVector::const_iterator[numberOfLines_ = boost::size(firstRunsInLines)]);

				BOOST_FOREACH(RunVector::reference run, runs_)
					run.release();
				BOOST_FOREACH(auto& run, createdRuns)
					run.release();
				for(RunVector::size_type i = 0, c = boost::size(runs); i < c; ++i)
					newRuns[i].reset(runs[i]);
				std::swap(runs_, newRuns);
				for(std::vector<Index>::size_type i = 0, c = boost::size(firstRunsInLines); i < c; ++i)
					firstRunsInLines_[i] = boost::const_begin(runs_) + firstRunsInLines[i];
			}


			// Font ///////////////////////////////////////////////////////////////////////////////////////////////////

			std::unique_ptr<const GlyphVector> Font::createGlyphVector(const FontRenderContext& frc, const StringPiece& text) const {
				win32::AutoZero<SCRIPT_ANALYSIS> script;
				script.eScript = SCRIPT_UNDEFINED;
				std::unique_ptr<GlyphVectorImpl> gv(new GlyphVectorImpl(text, script, shared_from_this(), frc, SCRIPT_TAG_UNKNOWN));
				auto dc(win32::detail::screenDC());
				gv->shape(dc);
				gv->positionGlyphs(dc);
				return std::move(gv);
			}
		}
	}

	void updateSystemSettings() {
		graphics::font::systemColors.update();
		graphics::font::userSettings.update();
	}
}
