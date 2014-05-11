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
#include <ascension/graphics/native-conversion.hpp>
#include <ascension/graphics/rendering-context.hpp>
#include <ascension/graphics/rendering-device.hpp>
#include <ascension/graphics/font/font-metrics.hpp>
#include <ascension/graphics/font/glyph-metrics.hpp>
#include <ascension/graphics/font/text-layout.hpp>
#include <ascension/graphics/font/text-layout-styles.hpp>
#include <ascension/graphics/font/text-run.hpp>
//#include <ascension/graphics/special-character-renderer.hpp>
#include <ascension/corelib/range.hpp>	// ascension.includes
#include <ascension/corelib/shared-library.hpp>
#include <ascension/corelib/text/character-iterator.hpp>
#include <ascension/corelib/text/character-property.hpp>
#include <ascension/presentation/writing-mode-mappings.hpp>
#include <limits>	// std.numeric_limits
#include <numeric>	// std.accumulate
#include <tuple>
#include <boost/flyweight.hpp>
#include <boost/foreach.hpp>
#include <boost/range/algorithm/find.hpp>
#include <boost/range/numeric.hpp>	// boost.accumulate
#include <usp10.h>
#ifdef _DEBUG
#	include <boost/log/trivial.hpp>
#endif

#pragma comment(lib, "usp10.lib")

namespace ascension {
//#define TRACE_LAYOUT_CACHES
	extern bool DIAGNOSE_INHERENT_DRAWING;

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
						return color ? color->as<COLORREF>() : get(index);
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
				std::unique_ptr<ascension::detail::SharedLibrary<Uniscribe16>> uspLib(
					new ascension::detail::SharedLibrary<Uniscribe16>("usp10.dll"));
			} // namespace @0

			// file-local free functions //////////////////////////////////////////////////////////////////////////////
			namespace {
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
					if(associations.empty()) {
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
						if(associations.find(Script::HAN) != associations.end()) {
							associations.insert(make_pair(Script::HIRAGANA, associations[Script::HAN]));
							associations.insert(make_pair(Script::KATAKANA, associations[Script::HAN]));
						}
					}

					std::map<int, const String>::const_iterator i(associations.find(script));
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

				SCRIPT_DIGITSUBSTITUTE&& convertNumberSubstitutionToUniscribe(const presentation::NumberSubstitution& from) {
					using presentation::NumberSubstitution;
					SCRIPT_DIGITSUBSTITUTE to;
					std::memset(&to, 0, sizeof(SCRIPT_DIGITSUBSTITUTE));
					switch(from.localeSource) {
						case NumberSubstitution::TEXT:
						case NumberSubstitution::USER: {
							// TODO: This code should not run frequently.
							const HRESULT hr = ::ScriptRecordDigitSubstitution(LOCALE_USER_DEFAULT, &to);
							if(FAILED(hr))
								throw makePlatformError(hr);
						}
//						case NumberSubstitution::OVERRIDE:
//							to.NationalDigitLanguage = to.TraditionalDigitLanguage = ????(from.localeOverride);
//							break;
						default:
							throw UnknownValueException("from.localeSource");
					}

					switch(from.method) {
						case NumberSubstitution::AS_LOCALE:
							to.DigitSubstitute = static_cast<DWORD>(-1);
							break;
						case NumberSubstitution::CONTEXT:
							to.DigitSubstitute = SCRIPT_DIGITSUBSTITUTE_CONTEXT;
							break;
						case NumberSubstitution::EUROPEAN:
							to.DigitSubstitute = SCRIPT_DIGITSUBSTITUTE_NONE;
							break;
						case NumberSubstitution::NATIVE_NATIONAL:
							to.DigitSubstitute = SCRIPT_DIGITSUBSTITUTE_NATIONAL;
							break;
						case NumberSubstitution::TRADITIONAL:
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
					return std::move(to);
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
								items.size(), nullptr, nullptr, items.data(), &numberOfItems)) && numberOfItems == 1)
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
				return uspLib->get<0>() != nullptr;
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

				class LogicalClusterIterator : public boost::iterator_facade<LogicalClusterIterator, std::nullptr_t, boost::bidirectional_traversal_tag> {
				public:
					LogicalClusterIterator() : clusters_(nullptr, nullptr), glyphIndices_(nullptr, nullptr), currentCluster_(nullptr, nullptr) {
					}
					LogicalClusterIterator(const boost::iterator_range<const WORD*>& clusters,
							const boost::iterator_range<const WORD*>& glyphIndices, std::size_t position) : clusters_(clusters), glyphIndices_(glyphIndices) {
						if(clusters.empty())
							throw std::invalid_argument("clusters");
						if(glyphIndices.empty())
							throw std::invalid_argument("glyphIndices");
						if(position > static_cast<std::size_t>(clusters_.size()))
							throw std::out_of_range("position");
						if(position < static_cast<std::size_t>(clusters_.size())) {
							currentCluster_ = boost::make_iterator_range(clusters_.begin() + position, clusters_.begin() + position + 1);
							decrement();
							increment();
						} else
							currentCluster_ = boost::make_iterator_range(clusters_.end(), clusters_.end());
					}
					const boost::iterator_range<const WORD*>& currentCluster() const BOOST_NOEXCEPT {
						return currentCluster_;
					}
					boost::iterator_range<const WORD*> currentGlyphRange() const BOOST_NOEXCEPT {
						assertNotInvalid();
						assertNotDone();
						if(readingDirection() == presentation::LEFT_TO_RIGHT) {
							const WORD nextGlyph = (currentCluster().end() < clusters_.end()) ? *currentCluster().end() : glyphIndices_.size();
							return boost::make_iterator_range(glyphIndices_.begin() + currentCluster().front(), glyphIndices_.begin() + nextGlyph);
						} else {
							const WORD first = (currentCluster().end() < clusters_.end()) ? *currentCluster().end() + 1 : 0;
							return boost::make_iterator_range(glyphIndices_.begin() + first, glyphIndices_.begin() + currentCluster().front() + 1);
						}
					}
					WORD glyphIndex() const BOOST_NOEXCEPT {
						assertNotInvalid();
						assertNotDone();
						return glyphIndices_[*currentCluster().begin()];
					}
					presentation::ReadingDirection readingDirection() const BOOST_NOEXCEPT {
						return readingDirection(clusters_);
					}
					static presentation::ReadingDirection readingDirection(const boost::iterator_range<const WORD*>& clusters) {
						if(clusters.begin() == nullptr || clusters.end() == nullptr)
							throw NullPointerException("clusters");
						if(clusters.begin() >= clusters.end())
							throw std::invalid_argument("clusters");
						return (clusters.front() <= clusters.back()) ? presentation::LEFT_TO_RIGHT : presentation::RIGHT_TO_LEFT;
					}
				private:
					void assertNotDone() const BOOST_NOEXCEPT {
						assert(!currentCluster().empty());
					}
					void assertNotInvalid() const BOOST_NOEXCEPT {
						assert(!glyphIndices_.empty() && glyphIndices_.begin() != nullptr && glyphIndices_.end() != nullptr);
					}
					void decrement() {
						assert(currentCluster().end() > clusters_.begin());
						if(currentCluster().begin() == clusters_.begin()) {
							currentCluster_ = boost::make_iterator_range(clusters_.begin(), clusters_.begin());
							return;
						}
						std::pair<const WORD*, const WORD*> previous(currentCluster().begin() - 1, currentCluster().begin());
						for(; previous.first > clusters_.begin() && *previous.first == previous.second[-1]; --previous.first);
					}
					bool equal(const LogicalClusterIterator& other) const BOOST_NOEXCEPT {
						return currentCluster() == other.currentCluster()
							|| (currentCluster().empty() && other.currentCluster().begin() == nullptr)
							|| (currentCluster().begin() == nullptr && other.currentCluster().empty());
					}
					void increment() {
						assert(currentCluster().begin() < clusters_.end());
						if(currentCluster().end() == clusters_.end()) {
							currentCluster_ = boost::make_iterator_range(clusters_.end(), clusters_.end());
							return;
						}
						std::pair<const WORD*, const WORD*> next(currentCluster().end(), currentCluster().end());
						for(; next.second < clusters_.end() && *next.second == *next.first; ++next.second);
						currentCluster_ = boost::make_iterator_range(next);
					}
					const boost::iterator_range<const WORD*> clusters_, glyphIndices_;
					boost::iterator_range<const WORD*> currentCluster_;
					friend class boost::iterator_core_access;
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
					if(textString.cbegin() == nullptr)
						throw NullPointerException(parameterName);
					else if(textString.empty())
						throw std::invalid_argument(parameterName);
				}

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
						attribute = move(other.attribute);
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

				class TextRunImpl : public TextRun, public StringPiece, private boost::noncopyable {
				public:
					struct Overlay {
						Color color;
						boost::integer_range<Index> range;
					};
				public:
					TextRunImpl(const StringPiece& characterRange, const SCRIPT_ANALYSIS& script,
						std::shared_ptr<const Font> font, const FontRenderContext& frc,
						OpenTypeFontTag scriptTag, const ComputedTextRunStyleCore& coreStyle);
					~TextRunImpl() BOOST_NOEXCEPT;
					static void generate(const StringPiece& textString,
						const ComputedTextLineStyle& lineStyle, std::unique_ptr<ComputedStyledTextRunIterator> textRunStyles,
						const FontCollection& fontCollection, const FontRenderContext& frc,
						std::vector<TextRunImpl*>& textRuns, std::vector<AttributedCharacterRange<ComputedTextRunStyle>>& calculatedStyles);
					// GlyphVector
					void fillGlyphs(PaintContext& context, const Point& origin/*,
						boost::optional<boost::integer_range<std::size_t>> range = boost::none*/) const;
					std::shared_ptr<const Font> font() const BOOST_NOEXCEPT;
					const FontRenderContext& fontRenderContext() const;
					Index glyphCharacterIndex(std::size_t index) const;
					GlyphCode glyphCode(std::size_t index) const;
					graphics::Rectangle glyphLogicalBounds(std::size_t index) const;
					GlyphMetrics&& glyphMetrics(std::size_t index) const;
					Point glyphPosition(std::size_t index) const;
					std::vector<Point>&& glyphPositions(const boost::integer_range<std::size_t>& range) const;
					graphics::Rectangle glyphVisualBounds(std::size_t index) const;
					graphics::Rectangle logicalBounds() const;
					std::size_t numberOfGlyphs() const BOOST_NOEXCEPT;
					void setGlyphPosition(std::size_t index, const Point& position);
					void strokeGlyphs(PaintContext& context, const Point& origin/*,
						boost::optional<boost::integer_range<std::size_t>> range = boost::none*/) const;
					graphics::Rectangle visualBounds() const;
					// TextRun
					const presentation::FlowRelativeFourSides<ComputedBorderSide>* border() const BOOST_NOEXCEPT override {return &coreStyle_.get().border;}
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
					boost::optional<Index> characterEncompassesPosition(float ipd) const BOOST_NOEXCEPT;
					Index characterHasClosestLeadingEdge(float ipd) const;
#endif // ASCENSION_ABANDONED_AT_VERSION_08
					std::uint8_t characterLevel() const BOOST_NOEXCEPT override;
					StringPiece characterRange() const BOOST_NOEXCEPT override;
					TextHit<>&& hitTestCharacter(Scalar ipd, const boost::optional<boost::integer_range<Scalar>>& bounds, bool* outOfBounds) const override;
					Scalar hitToLogicalPosition(const TextHit<>& hit) const override;
					const presentation::FlowRelativeFourSides<Scalar>* margin() const BOOST_NOEXCEPT override {return &coreStyle_.get().margin;}
					const presentation::FlowRelativeFourSides<Scalar>* padding() const BOOST_NOEXCEPT override {return &coreStyle_.get().padding;}
					// attributes
					const ComputedTextRunStyleCore& style() const BOOST_NOEXCEPT {return coreStyle_;}
					HRESULT logicalAttributes(SCRIPT_LOGATTR attributes[]) const;
					// geometry
					std::vector<graphics::Rectangle>&& charactersBounds(const boost::integer_range<Index>& characterRange) const;
					HRESULT logicalWidths(int widths[]) const;
//					int totalAdvance() const BOOST_NOEXCEPT {return boost::accumulate(advances(), 0);}
					// layout
					std::unique_ptr<TextRunImpl> breakAt(StringPiece::const_iterator at);
					bool expandTabCharacters(const TabExpander& tabExpander,
						const String& layoutString, Scalar ipd, Scalar maximumMeasure);
					HRESULT justify(int width);
#if 0
					static void mergeScriptsAndStyles(const StringPiece& layoutString, const SCRIPT_ITEM scriptRuns[],
						const OPENTYPE_TAG scriptTags[], std::size_t numberOfScriptRuns, const FontCollection& fontCollection,
						shared_ptr<const TextRunStyle> defaultStyle, unique_ptr<ComputedStyledTextRunIterator> styles,
						vector<TextRunImpl*>& textRuns, vector<const ComputedTextRunStyle>& computedStyles,
						vector<vector<const ComputedTextRunStyle>::size_type>& computedStylesIndices);
#endif
					void shape(win32::Handle<HDC>::Type dc);
					void positionGlyphs(win32::Handle<HDC>::Type dc, const ComputedTextRunStyle& style);
					std::unique_ptr<TextRunImpl> splitIfTooLong();
					static void substituteGlyphs(const boost::iterator_range<std::vector<TextRunImpl*>::iterator>& runs);
					// drawing and painting
					void drawGlyphs(PaintContext& context, const Point& p, const boost::integer_range<Index>& range) const;
					void paintLineDecorations() const;
				private:
					// this data is shared text runs separated by (only) line breaks and computed styles
					struct RawGlyphVector /*: public StringPiece*/ : private boost::noncopyable {
						const StringPiece::const_iterator position;
						boost::flyweight<FontAndRenderContext> font;
						const OpenTypeFontTag scriptTag;	// as OPENTYPE_TAG
						mutable SCRIPT_CACHE fontCache;
						std::size_t numberOfGlyphs;
						// only 'clusters' is character-base. others are glyph-base
						std::unique_ptr<WORD[]> indices, clusters;
						std::unique_ptr<SCRIPT_VISATTR[]> visualAttributes;
						std::unique_ptr<int[]> advances, justifiedAdvances;
						std::unique_ptr<GOFFSET[]> offsets;
						RawGlyphVector(StringPiece::const_iterator position, std::shared_ptr<const Font> font, const FontRenderContext& frc,
								OpenTypeFontTag scriptTag) : position(position), font(font, frc), scriptTag(scriptTag), fontCache(nullptr) {
							raiseIfNull(position, "position");
							raiseIfNull(font.get(), "font");
						}
						RawGlyphVector(RawGlyphVector&& other) BOOST_NOEXCEPT;
						RawGlyphVector& operator=(RawGlyphVector&& other) BOOST_NOEXCEPT;
						~RawGlyphVector() BOOST_NOEXCEPT {::ScriptFreeCache(&fontCache);}
						void vanish(const Font& font, StringPiece::const_iterator at);
					};
				private:
					TextRunImpl(const StringPiece& characterRange, const SCRIPT_ANALYSIS& script,
						std::unique_ptr<RawGlyphVector> glyphs, const ComputedTextRunStyleCore& coreStyle);
					TextRunImpl(TextRunImpl& leading, StringPiece::const_iterator beginningOfNewRun);
					boost::iterator_range<const int*> advances() const BOOST_NOEXCEPT {
						if(const int* const p = glyphs_->advances.get())
							return boost::make_iterator_range(p + *glyphRange().begin(), p + *glyphRange().end());
						return boost::make_iterator_range<const int*>(nullptr, nullptr);
					}
					boost::iterator_range<const WORD*> clusters() const BOOST_NOEXCEPT {
						if(const WORD* const p = glyphs_->clusters.get())
							return boost::make_iterator_range(p + (begin() - glyphs_->position), p + (end() - glyphs_->position));
						return boost::make_iterator_range<const WORD*>(nullptr, nullptr);
					}
					std::size_t countMissingGlyphs(const RenderingContext2D& context) const;
					boost::iterator_range<const int*> effectiveAdvances() const BOOST_NOEXCEPT {
						if(const int* const p = glyphs_->justifiedAdvances.get())
							return boost::make_iterator_range(p + *glyphRange().begin(), p + *glyphRange().end());
						else if(const int* const p = glyphs_->advances.get())
							return boost::make_iterator_range(p + *glyphRange().begin(), p + *glyphRange().end());
						return boost::make_iterator_range<const int*>(nullptr, nullptr);
					}
					static void generateDefaultGlyphs(win32::Handle<HDC>::Type dc,
						const StringPiece& text, const SCRIPT_ANALYSIS& analysis, RawGlyphVector& glyphs);
					static HRESULT generateGlyphs(win32::Handle<HDC>::Type dc,
						const StringPiece& text, const SCRIPT_ANALYSIS& analysis, RawGlyphVector& glyphs);
					Scalar glyphLogicalPosition(std::size_t index) const;
					boost::iterator_range<const WORD*> glyphs() const BOOST_NOEXCEPT {
						if(const WORD* const p = glyphs_->indices.get())
							return boost::make_iterator_range(p + *glyphRange().begin(), p + *glyphRange().end());
						return boost::make_iterator_range<const WORD*>(nullptr, nullptr);
					}
					boost::iterator_range<const GOFFSET*> glyphOffsets() const BOOST_NOEXCEPT {
						if(const GOFFSET* const p = glyphs_->offsets.get())
							return boost::make_iterator_range(p + *glyphRange().begin(), p + *glyphRange().end());
						return boost::make_iterator_range<const GOFFSET*>(nullptr, nullptr);
					}
					boost::integer_range<std::size_t> glyphRange(const StringPiece& characterRange = StringPiece()) const;
#if 0
					void hitTest(Scalar ipd, int& encompasses, int* trailing) const;
#endif
					boost::iterator_range<const int*> justifiedAdvances() const BOOST_NOEXCEPT {
						if(const int* const p = glyphs_->justifiedAdvances.get())
							return boost::make_iterator_range(p + *glyphRange().begin(), p + *glyphRange().end());
						return boost::make_iterator_range<const int*>(nullptr, nullptr);
					}
					boost::integer_range<Scalar> logicalExtents() const {
						RenderingContext2D context(win32::detail::screenDC());
						std::unique_ptr<const FontMetrics<Scalar>> fm(context.fontMetrics(font()));
						const double sy = geometry::scaleY(fontRenderContext().transform()) / geometry::scaleY(context.fontRenderContext().transform());
						return boost::irange(-static_cast<Scalar>(fm->ascent() * sy), static_cast<Scalar>(fm->descent() * sy + fm->internalLeading() * sy));
					}
					void paintGlyphs(PaintContext& context, const Point& origin/*,
						boost::optional<boost::integer_range<std::size_t>> range*/, bool onlyStroke) const;
					boost::iterator_range<const SCRIPT_VISATTR*> visualAttributes() const BOOST_NOEXCEPT {
						if(const SCRIPT_VISATTR* const p = glyphs_->visualAttributes.get())
							return boost::make_iterator_range(p + *glyphRange().begin(), p + *glyphRange().end());
						return boost::make_iterator_range<const SCRIPT_VISATTR*>(nullptr, nullptr);
					}
				private:
					boost::flyweight<ComputedTextRunStyleCore> coreStyle_;
					SCRIPT_ANALYSIS analysis_;	// fLogicalOrder member is always 0 (however see shape())
					std::shared_ptr<RawGlyphVector> glyphs_;
				};
			}

			void TextRunImpl::RawGlyphVector::vanish(const Font& font, StringPiece::const_iterator at) {
				assert(advances.get() == nullptr);
				assert(at != nullptr);
				assert(at >= position);
				win32::Handle<HDC>::Type dc(win32::detail::screenDC());
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
			 * Constructor.
			 * @param characterRange The string this text run covers
			 * @param script @c SCRIPT_ANALYSIS The object obtained by @c ScriptItemize(OpenType)
			 * @param font The font renders this text run. Can't be @c null
			 * @param scriptTag An OpenType script tag describes the script of this text run
			 * @param coreStyle The core text style
			 * @throw NullPointerException @a characterRange and/or @a font are @c null
			 * @throw std#invalid_argument @a characterRange is empty
			 * @note This constructor is called by only @c #splitIfTooLong.
			 */
			TextRunImpl::TextRunImpl(const StringPiece& characterRange, const SCRIPT_ANALYSIS& script,
					std::shared_ptr<const Font> font, const FontRenderContext& frc, OpenTypeFontTag scriptTag, const ComputedTextRunStyleCore& coreStyle)
					: StringPiece(characterRange), coreStyle_(coreStyle), analysis_(script),
					glyphs_(new RawGlyphVector(characterRange.cbegin(), font, frc, scriptTag)) {	// may throw NullPointerException for 'font'
				raiseIfNullOrEmpty(characterRange, "characterRange");
//				raiseIfNull(font.get(), "font");
			}

			/**
			 * Private constructor.
			 * @param characterRange The string this text run covers
			 * @param script @c SCRIPT_ANALYSIS The object obtained by @c ScriptItemize(OpenType)
			 * @param glyphs The glyph vector
			 * @param coreStyle The core text style
			 * @throw NullPointerException @a characterRange and/or @a glyphs are @c null
			 * @throw std#invalid_argument @a characterRange is empty
			 * @note This constructor is called by only @c #generate.
			 */
			TextRunImpl::TextRunImpl(const StringPiece& characterRange, const SCRIPT_ANALYSIS& script,
					std::unique_ptr<RawGlyphVector> glyphs, const ComputedTextRunStyleCore& coreStyle) :
					StringPiece(characterRange), coreStyle_(coreStyle), analysis_(script), glyphs_(std::move(glyphs)) {
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
			 * @note This constructor is called by only @c #breakAt.
			 */
			TextRunImpl::TextRunImpl(TextRunImpl& leading, StringPiece::const_iterator beginningOfNewRun) :
					StringPiece(makeStringPiece(beginningOfNewRun, leading.end())),
					coreStyle_(leading.style()), analysis_(leading.analysis_), glyphs_(leading.glyphs_) {
				if(leading.glyphs_.get() == nullptr)
					throw std::invalid_argument("leading has not been shaped");
				raiseIfNull(beginningOfNewRun, "beginningOfNewRun");
				if(!includes(leading.characterRange(), beginningOfNewRun))
					throw std::out_of_range("beginningOfNewRun");

				// compute 'glyphRange_'

				// modify clusters
//				TextRun& target = ltr ? *this : leading;
//				WORD* const clusters = glyphs_->clusters.get();
//				transform(target.clusters(), target.clusters() + target.length(),
//					clusters + target.beginning(), bind2nd(minus<WORD>(), clusters[ltr ? target.beginning() : (target.end() - 1)]));
			}

			/// Destructor.
			TextRunImpl::~TextRunImpl() BOOST_NOEXCEPT {
			//	if(cache_ != nullptr)
			//		::ScriptFreeCache(&cache_);
			}

			/**
			 * Breaks the text run into two runs at the specified position.
			 * @param at The position at which break this run
			 * @return The new text run following this run
			 * @note This method is called by only @c #wrap.
			 */
			std::unique_ptr<TextRunImpl> TextRunImpl::breakAt(StringPiece::const_iterator at) {
				raiseIfNull(at, "at");
				if(!includes(*this, at))
					throw std::out_of_range("at");
				else if(glyphs_->clusters[at - begin()] == glyphs_->clusters[at - begin() - 1])
					throw std::invalid_argument("at");
			
				const bool ltr = direction() == presentation::LEFT_TO_RIGHT;
				assert(ltr == (analysis_.fRTL == 0));
			
				// create the new following run
				std::unique_ptr<TextRunImpl> following(new TextRunImpl(*this, at));
			
				// update placements
//				place(context, layoutString, lip);
//				following->place(dc, layoutString, lip);
			
				return following;
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

			/// @see TextRun#characterLevel
			std::uint8_t TextRunImpl::characterLevel() const BOOST_NOEXCEPT {
				return static_cast<std::uint8_t>(analysis_.s.uBidiLevel);
			}

			std::vector<graphics::Rectangle>&& TextRunImpl::charactersBounds(const boost::integer_range<Index>& characterRange) const {
				if(characterRange.empty())
					return std::vector<graphics::Rectangle>();
				// 'characterRange' are offsets from the beginning of this text run

				// measure glyph black box bounds
				const boost::iterator_range<const WORD*> glyphIndices(glyphs());
				const boost::iterator_range<const int*> glyphAdvances(effectiveAdvances());
				const bool rtl = LogicalClusterIterator::readingDirection(clusters()) == presentation::RIGHT_TO_LEFT;
				LogicalClusterIterator cluster(clusters(), glyphs(), !rtl ? characterRange.front() : characterRange.back());
				Scalar x = 0;
				for(std::size_t i = 0, firstGlyph = cluster.currentGlyphRange().begin() - glyphIndices.begin(); i < firstGlyph; ++i)
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
						right = std::numeric_limits<Scalar>::min(), bottom = std::numeric_limits<Scalar>::min();
					const boost::iterator_range<const WORD*> glyphRange(cluster.currentGlyphRange());
					for(std::size_t i = 0; i < static_cast<std::size_t>(glyphRange.size()); ++i, x += glyphAdvances[i]) {
						GLYPHMETRICS gm;
						if(GDI_ERROR == ::GetGlyphOutlineW(context.asNativeObject().get(), glyphRange[i], GGO_GLYPH_INDEX | GGO_METRICS, &gm, 0, nullptr, &matrix)) {
							lastError = ::GetLastError();
							break;
						}
						left = std::min(x - static_cast<Scalar>(gm.gmptGlyphOrigin.x * sx) + glyphOffsets2D[i].du, left);
						top = std::min(0 - static_cast<Scalar>(gm.gmptGlyphOrigin.y * sy) + glyphOffsets2D[i].dv, top);
						right = std::max(x + static_cast<Scalar>(gm.gmBlackBoxX * sx) + glyphOffsets2D[i].du, right);
						bottom = std::max(0 + static_cast<Scalar>(gm.gmBlackBoxY * sy) + glyphOffsets2D[i].dv, bottom);
					}
					bounds.push_back(graphics::Rectangle(geometry::_left = left, geometry::_top = top, geometry::_right = right, geometry::_bottom = bottom));
				}
				context.restore();
				if(lastError != ERROR_SUCCESS)
					throw makePlatformError(lastError);
				return std::move(bounds);
			}

			/// @see TextRun#characterRange
			inline StringPiece TextRunImpl::characterRange() const BOOST_NOEXCEPT {
				return *this;
			}

			/**
			 * Returns the number of missing glyphs in this run.
			 * @param context The graphics context
			 * @return The number of missing glyphs
			 * @throw PlatformError
			 */
			inline std::size_t TextRunImpl::countMissingGlyphs(const RenderingContext2D& context) const {
				SCRIPT_FONTPROPERTIES fp;
				fp.cBytes = sizeof(SCRIPT_FONTPROPERTIES);
				const HRESULT hr = ::ScriptGetFontProperties(context.asNativeObject().get(), &glyphs_->fontCache, &fp);
				if(FAILED(hr))
					throw makePlatformError(hr);	// can't handle
				// following is not offical way, but from Mozilla (gfxWindowsFonts.cpp)
				std::size_t c = 0;
				for(text::StringCharacterIterator i(*this); i.hasNext(); i.next()) {
					if(!text::ucd::BinaryProperty::is<text::ucd::BinaryProperty::DEFAULT_IGNORABLE_CODE_POINT>(i.current())) {
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
			 * @param layoutString The text string for the layout to which this text run belongs
			 * @param ipd The position in writing direction this text run begins, in pixels
			 * @param maximumMeasure The maximum measure this text run can take place, in pixels
			 * @return @c true if expanded tab characters
			 * @throw std#invalid_argument @a maximumMeasure &lt;= 0
			 */
			inline bool TextRunImpl::expandTabCharacters(
					const TabExpander& tabExpander, const String& layoutString, Scalar ipd, Scalar maximumMeasure) {
				if(maximumMeasure <= 0)
					throw std::invalid_argument("maximumMeasure");
				if(front() != '\t')
					return false;
				assert(length() == 1 && glyphs_.unique());
				glyphs_->advances[0] = static_cast<int>(std::min(tabExpander.nextTabStop(ipd, begin() - layoutString.data()), maximumMeasure));
				glyphs_->justifiedAdvances.reset();
				return true;
			}

			/// @see GlyphVector#fillGlyphs
			void TextRunImpl::fillGlyphs(PaintContext& context, const Point& origin/*, boost::optional<boost::integer_range<std::size_t>> range = boost::none*/) const {
				return paintGlyphs(context, origin/*, range*/, false);
			}

			/// @see GlyphVector#font
			std::shared_ptr<const Font> TextRunImpl::font() const BOOST_NOEXCEPT {
				return glyphs_->font.get().font();
			}

			/// @see GlyphVector#fontRenderContext
			const FontRenderContext& TextRunImpl::fontRenderContext() const BOOST_NOEXCEPT {
				return glyphs_->font.get().fontRenderContext();
			}

			namespace {
				inline HRESULT callScriptItemize(const WCHAR* text, int length, int estimatedNumberOfItems,
						const SCRIPT_CONTROL& control, const SCRIPT_STATE& initialState, SCRIPT_ITEM items[], OPENTYPE_TAG scriptTags[], int& numberOfItems) BOOST_NOEXCEPT {
					static HRESULT(WINAPI* scriptItemizeOpenType)(const WCHAR*, int, int,
						const SCRIPT_CONTROL*, const SCRIPT_STATE*, SCRIPT_ITEM*, OPENTYPE_TAG*, int*) = uspLib->get<0>();
					if(scriptItemizeOpenType != nullptr && scriptTags != nullptr)
						return (*scriptItemizeOpenType)(text, length, estimatedNumberOfItems, &control, &initialState, items, scriptTags, &numberOfItems);
					else
						return ::ScriptItemize(text, length, estimatedNumberOfItems, &control, &initialState, items, &numberOfItems);
				}
				std::shared_ptr<const Font> selectFont(const StringPiece& textString, const FontCollection& fontCollection, const ComputedFontSpecification& specification);
			}

			/**
			 * @param textString
			 * @param lineStyle
			 * @param textRunStyles
			 * @param fontCollection
			 * @param frc
			 * @param[out] textRuns
			 * @param[out] calculatedStyles
			 */
			void TextRunImpl::generate(const StringPiece& textString,
					const ComputedTextLineStyle& lineStyle, std::unique_ptr<ComputedStyledTextRunIterator> textRunStyles,
					const FontCollection& fontCollection, const FontRenderContext& frc,
					std::vector<TextRunImpl*>& textRuns, std::vector<AttributedCharacterRange<ComputedTextRunStyle>>& calculatedStyles) {
				raiseIfNullOrEmpty(textString, "textString");

				// split the text line into text runs as following steps:
				// 1. split the text into script runs (SCRIPT_ITEMs) by Uniscribe
				// 2. split each script runs into atomically-shapable runs (TextRuns) with StyledRunIterator

				// 1. split the text into script runs by Uniscribe
				HRESULT hr;

				// 1-1. configure Uniscribe's itemize
				win32::AutoZero<SCRIPT_CONTROL> control;
				win32::AutoZero<SCRIPT_STATE> initialState;
				initialState.uBidiLevel = (lineStyle.writingMode.inlineFlowDirection == presentation::RIGHT_TO_LEFT) ? 1 : 0;
//				initialState.fOverrideDirection = 1;
				initialState.fInhibitSymSwap = lineStyle.inhibitSymmetricSwapping;
				initialState.fDisplayZWG = lineStyle.displayShapingControls;
				const SCRIPT_DIGITSUBSTITUTE sds(convertNumberSubstitutionToUniscribe(lineStyle.numberSubstitution));
				hr = ::ScriptApplyDigitSubstitution(&sds, &control, &initialState);
				if(FAILED(hr))
					throw makePlatformError(hr);

				// 1-2. itemize
				// note that ScriptItemize can cause a buffer overflow (see Mozilla bug 366643)
				AutoArray<SCRIPT_ITEM, 128> scriptRuns;
				AutoArray<OPENTYPE_TAG, scriptRuns.STATIC_CAPACITY> scriptTags;
				int estimatedNumberOfScriptRuns = std::max(static_cast<int>(textString.length()) / 4, 2), numberOfScriptRuns;
				HRESULT(WINAPI* scriptItemizeOpenType)(const WCHAR*, int, int,
					const SCRIPT_CONTROL*, const SCRIPT_STATE*, SCRIPT_ITEM*, OPENTYPE_TAG*, int*) = uspLib->get<0>();
				while(true) {
					scriptRuns.reallocate(estimatedNumberOfScriptRuns);
					scriptTags.reallocate(estimatedNumberOfScriptRuns);
					hr = callScriptItemize(std::begin(textString), static_cast<int>(textString.length()),
						estimatedNumberOfScriptRuns, control, initialState, scriptRuns.get(), scriptTags.get(), numberOfScriptRuns);
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
					std::fill_n(scriptTags.get(), numberOfScriptRuns, SCRIPT_TAG_UNKNOWN);

				// 2. generate raw glyph vectors and computed styled text runs
				std::vector<std::unique_ptr<RawGlyphVector>> glyphRuns;
				glyphRuns.reserve(numberOfScriptRuns);
				std::vector<const SCRIPT_ANALYSIS*> scriptPointers;
				scriptPointers.reserve(numberOfScriptRuns);
				std::vector<AttributedCharacterRange<ComputedTextRunStyle>> styleRuns;
				{
					StringPiece::const_iterator lastGlyphRunEnd = nullptr;
					// script cursors
					AttributedCharacterRange<const SCRIPT_ITEM*>
						scriptRun(std::begin(textString) + scriptRuns[0].iCharPos, &scriptRuns[0]),
						nextScriptRun((numberOfScriptRuns > 1) ?
							std::begin(textString) + scriptRuns[1].iCharPos : textString.end(), scriptRun.attribute + 1);
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
					styleRuns.push_back(AttributedCharacterRange<ComputedTextRunStyle>(styleRun.position, styleRun.attribute));

					do {
						const StringPiece::const_iterator next = std::min(nextScriptRun.position, nextStyleRun.position);
						const bool advanceScriptRun = next == nextScriptRun.position;
						const bool advanceStyleRun = next == nextStyleRun.position;

						if(advanceScriptRun) {
							const StringPiece subRange(scriptRun.position, next - scriptRun.position);
							assert(glyphRuns.empty() || subRange.cbegin() == lastGlyphRunEnd);
							glyphRuns.push_back(
								std::unique_ptr<RawGlyphVector>(
									new RawGlyphVector(subRange.cbegin(),
										selectFont(subRange, fontCollection, styleRun.attribute.font),
										frc, scriptTags[scriptRun.attribute - scriptRuns.get()])));
							scriptPointers.push_back(&scriptRuns[scriptRun.attribute - scriptRuns.get()].a);
							assert(nextScriptRun.position < textString.end());
							scriptRun = nextScriptRun;
							if(++nextScriptRun.attribute < scriptRuns.get() + numberOfScriptRuns)
								nextScriptRun.position = textString.cbegin() + nextScriptRun.attribute->iCharPos;
							else
								nextScriptRun.position = textString.end();
						}
						if(advanceStyleRun) {
							if(!advanceScriptRun) {
								const StringPiece subRange(makeStringPiece(!glyphRuns.empty() ? lastGlyphRunEnd : textString.cbegin(), next));
								glyphRuns.push_back(
									std::unique_ptr<RawGlyphVector>(
										new RawGlyphVector(subRange.cbegin(),
											selectFont(subRange, fontCollection, styleRun.attribute.font),
											frc, scriptTags[scriptRun.attribute - scriptRuns.get()])));
							}
							assert(nextStyleRun.position < textString.end());
							styleRun = std::move(nextStyleRun);	// C2668 if included <boost/log/trivial.hpp> without 'std::' ???
							styleRuns.push_back(AttributedCharacterRange<ComputedTextRunStyle>(styleRun.position, styleRun.attribute));
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
				std::vector<TextRunImpl*> mergedTextRuns;
				mergedTextRuns.reserve(glyphRuns.size() + styleRuns.size());
				{
					auto glyphRun(std::begin(glyphRuns)), lastGlyphRun(std::end(glyphRuns));
					auto styleRun(std::begin(styleRuns)), lastStyleRun(std::end(styleRuns));
					do {
						auto nextGlyphRun(glyphRun + 1);
						auto nextStyleRun(styleRun + 1);
						const StringPiece::const_iterator
							nextGlyphRunPosition((nextGlyphRun != lastGlyphRun) ? (*nextGlyphRun)->position : textString.end()),
							nextStyleRunPosition((nextStyleRun != lastStyleRun) ? nextStyleRun->position : textString.end());
						const StringPiece::const_iterator nextPosition(std::min(nextGlyphRunPosition, nextStyleRunPosition));
						const StringPiece::const_iterator previousPosition(!mergedTextRuns.empty() ? mergedTextRuns.back()->end() : textString.cbegin());

						mergedTextRuns.push_back(new TextRunImpl(
							makeStringPiece(previousPosition, nextPosition),
							*scriptPointers[glyphRuns.size() - (lastGlyphRun - glyphRun)], std::move(*glyphRun), styleRun->attribute));
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
			inline void TextRunImpl::generateDefaultGlyphs(win32::Handle<HDC>::Type dc,
					const StringPiece& text, const SCRIPT_ANALYSIS& analysis, RawGlyphVector& glyphs) {
				SCRIPT_CACHE fontCache(nullptr);
				SCRIPT_FONTPROPERTIES fp;
				fp.cBytes = sizeof(SCRIPT_FONTPROPERTIES);
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
			HRESULT TextRunImpl::generateGlyphs(win32::Handle<HDC>::Type dc,
					const StringPiece& text, const SCRIPT_ANALYSIS& analysis, RawGlyphVector& glyphs) {
#ifdef _DEBUG
				if(HFONT currentFont = static_cast<HFONT>(::GetCurrentObject(dc.get(), OBJ_FONT))) {
					LOGFONTW lf;
					if(::GetObjectW(currentFont, sizeof(LOGFONTW), &lf) > 0)
						BOOST_LOG_TRIVIAL(debug) << L"[TextLayout.TextRun.generateGlyphs] Selected font is '" << lf.lfFaceName << L"'.\n";
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
						text.cbegin(), static_cast<int>(text.length()),
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

			/// @see GlyphVector#glyphCharacterIndex
			Index TextRunImpl::glyphCharacterIndex(std::size_t index) const {
				if(index >= numberOfGlyphs())
					throw std::out_of_range("index");
				const auto glyphIndices = glyphs();
				const LogicalClusterIterator e;
				for(LogicalClusterIterator i(clusters(), glyphIndices, 0); i != e; ++i) {
					if(index >= static_cast<std::size_t>(i.currentGlyphRange().begin() - glyphIndices.begin())
							&& index < static_cast<std::size_t>(i.currentGlyphRange().end() - glyphIndices.begin()))
						return i.currentCluster().front();
				}
				return length();
			}

			/// @see GlyphVector#glyphCode
			GlyphCode TextRunImpl::glyphCode(std::size_t index) const {
				if(index >= numberOfGlyphs())
					throw std::out_of_range("index");
				return glyphs()[index];
			}

			/// @see GlyphVector#glyphLogicalBounds
			graphics::Rectangle TextRunImpl::glyphLogicalBounds(std::size_t index) const {
				if(index >= numberOfGlyphs())
					throw std::out_of_range("index");
				const Scalar x = glyphLogicalPosition(index);
				const auto yrange = logicalExtents();
				return graphics::Rectangle(
					geometry::_top = *yrange.begin(), geometry::_bottom = *yrange.end(),
					geometry::_left = x, geometry::_right = static_cast<Scalar>(x + effectiveAdvances()[index]));
			}

			inline Scalar TextRunImpl::glyphLogicalPosition(std::size_t index) const {
				assert(index <= numberOfGlyphs());
				const boost::iterator_range<const int*> glyphAdvances(effectiveAdvances());
				assert(glyphAdvances.begin() != nullptr);
				int x = 0;
				for(std::size_t i = 0, c = numberOfGlyphs(); i < c; ++i) {
					if(i == index)
						break;
					x += glyphAdvances[i];
				}
				return static_cast<Scalar>(x);
			}

			/// @see GlyphVector#glyphMetrics
			GlyphMetrics&& TextRunImpl::glyphMetrics(std::size_t index) const {
				if(index >= numberOfGlyphs())
					throw IndexOutOfBoundsException("index");
	
				RenderingContext2D context(win32::detail::screenDC());
				std::shared_ptr<const Font> oldFont(context.font());
				context.setFont(font());
				GLYPHMETRICS gm;
				const MAT2 matrix = {1, 0, 0, 1};	// TODO: Consider glyph transform.
				const DWORD lastError = (::GetGlyphOutlineW(context.asNativeObject().get(),
					glyphCode(index), GGO_GLYPH_INDEX | GGO_METRICS, &gm, 0, nullptr, &matrix) == GDI_ERROR) ? ::GetLastError() : ERROR_SUCCESS;
				context.setFont(oldFont);
				if(lastError != ERROR_SUCCESS)
					throw makePlatformError(lastError);
				const auto sx = geometry::scaleX(fontRenderContext().transform()) / geometry::scaleX(context.fontRenderContext().transform());
				const auto sy = geometry::scaleY(fontRenderContext().transform()) / geometry::scaleY(context.fontRenderContext().transform());
				return GlyphMetrics(gm.gmCellIncY == 0,
					Dimension(geometry::_dx = static_cast<Scalar>(gm.gmCellIncX * sx), geometry::_dy = static_cast<Scalar>(gm.gmCellIncY * sy)),
					graphics::Rectangle(
						Point(geometry::_x = static_cast<Scalar>(gm.gmptGlyphOrigin.x * sx), geometry::_y = -static_cast<Scalar>(gm.gmptGlyphOrigin.y * sy)),
						Dimension(geometry::_dx = static_cast<Scalar>(gm.gmBlackBoxX * sx), geometry::_dy = static_cast<Scalar>(gm.gmBlackBoxY * sy))),
					static_cast<GlyphMetrics::Type>(0));
			}

			/// @see GlyphVector#glyphPosition
			Point TextRunImpl::glyphPosition(std::size_t index) const {
				if(index > numberOfGlyphs())
					throw IndexOutOfBoundsException("index");
				const Scalar logicalPosition = glyphLogicalPosition(index);
				const GOFFSET& glyphOffset = glyphOffsets()[index];
				return Point(geometry::_x = static_cast<Scalar>(logicalPosition + glyphOffset.du), geometry::_y = static_cast<Scalar>(glyphOffset.dv));
			}

			/// @see GlyphVector#glyphPositions
			std::vector<Point>&& TextRunImpl::glyphPositions(const boost::integer_range<std::size_t>& range) const {
				const auto orderedRange = ordered(range);
				if(*orderedRange.end() > numberOfGlyphs())
					throw IndexOutOfBoundsException("range");

				std::vector<Point> positions;
				positions.reserve(range.size());
				for(std::size_t i = *std::begin(orderedRange); i < *std::end(orderedRange); ++i) {
					const Scalar logicalPosition = glyphLogicalPosition(i);
					const GOFFSET& glyphOffset = glyphOffsets()[i];
					geometry::x(positions[i]) = static_cast<Scalar>(logicalPosition + glyphOffset.du);
					geometry::y(positions[i]) = static_cast<Scalar>(glyphOffset.dv);
				}
				return std::move(positions);
			}

			inline boost::integer_range<std::size_t> TextRunImpl::glyphRange(const StringPiece& range /* = StringPiece() */) const {
				assert(glyphs_.get() != nullptr);
				assert(analysis_.fLogicalOrder == 0);
				boost::integer_range<ptrdiff_t> characterRange((range != StringPiece()) ?
					boost::irange(range.cbegin() - begin(), range.cend() - begin()) : boost::irange<std::ptrdiff_t>(0, length()));
				assert(includes(boost::irange<ptrdiff_t>(0, length()), characterRange));
				assert(*characterRange.begin() == 0 || *characterRange.begin() == length()
					|| glyphs_->clusters[*characterRange.begin()] != glyphs_->clusters[*characterRange.begin() - 1]);
				assert(*characterRange.end() == 0 || *characterRange.end() == length()
					|| glyphs_->clusters[*characterRange.end()] != glyphs_->clusters[*characterRange.end() + 1]);

				if(analysis_.fRTL == 0)	// LTR
					return boost::irange(
						(range.cbegin() < end()) ? glyphs_->clusters[range.cbegin() - begin()] : glyphs_->numberOfGlyphs,
						(range.cend() < end()) ? glyphs_->clusters[range.cend() - begin() + 1] : glyphs_->numberOfGlyphs);
				else					// RTL
					return boost::irange(
						(range.cend() > begin()) ? glyphs_->clusters[range.cend() - begin() - 1] : glyphs_->numberOfGlyphs,
						(range.cbegin() > begin()) ? glyphs_->clusters[range.cbegin() - begin() - 1] : glyphs_->numberOfGlyphs
					);
			}

			/// @see GlyphVector#glyphVisualBounds
			graphics::Rectangle TextRunImpl::glyphVisualBounds(std::size_t index) const {
				if(index >= numberOfGlyphs())
					throw std::out_of_range("index");
				Scalar originX = glyphLogicalPosition(index);
				const GlyphMetrics gm(glyphMetrics(index));
				const GOFFSET& offset = glyphOffsets()[index];
				graphics::Rectangle temp(gm.bounds());
				return geometry::translate(temp, Dimension(geometry::_dx = static_cast<Scalar>(originX + offset.du), geometry::_dy = static_cast<Scalar>(offset.dv)));
			}

#if 0
			inline void TextRunImpl::hitTest(Scalar ipd, int& encompasses, int* trailing) const {
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
			TextHit<>&& TextRunImpl::hitTestCharacter(Scalar position, const boost::optional<boost::integer_range<Scalar>>& bounds, bool* outOfBounds) const {
				bool beyondLineLeft = false, beyondLineRight = false;
				if(bounds) {
					if(position < std::min(*bounds->begin(), *bounds->end()))
						beyondLineLeft = true;
					else if(position >= std::max(*bounds->begin(), *bounds->end()))
						beyondLineRight = true;
				}

				if(!beyondLineLeft && !beyondLineRight) {
					int cp, trailing;
					const HRESULT hr = ::ScriptXtoCP(static_cast<int>(position), static_cast<int>(length()), numberOfGlyphs(),
						clusters().begin(), visualAttributes().begin(), effectiveAdvances().begin(), &analysis_, &cp, &trailing);
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
				if(beyondLineLeft)
					return (direction() == presentation::LEFT_TO_RIGHT) ? TextHit<>::leading(0) : TextHit<>::beforeOffset(length());
				else if(beyondLineRight)
					return (direction() == presentation::LEFT_TO_RIGHT) ? TextHit<>::beforeOffset(length()) : TextHit<>::leading(0);
				ASCENSION_ASSERT_NOT_REACHED();
			}

			/// @see TextRun#hitToLogicalPosition
			Scalar TextRunImpl::hitToLogicalPosition(const TextHit<>& hit) const {
				if(hit.insertionIndex() > characterRange().length())
					throw IndexOutOfBoundsException("hit");
				int logicalPosition;
				const HRESULT hr = ::ScriptCPtoX(static_cast<int>(hit.characterIndex()), !hit.isLeadingEdge(),
					static_cast<int>(length()), numberOfGlyphs(), clusters().begin(), visualAttributes().begin(),
					effectiveAdvances().begin(), &analysis_, &logicalPosition);
				if(FAILED(hr))
					throw makePlatformError(hr);
				// TODO: handle letter-spacing correctly.
//				if(visualAttributes()[offset].fClusterStart == 0) {	// oops, i can't remember what this code means...
//				}
				return static_cast<Scalar>(logicalPosition);
			}

			inline HRESULT TextRunImpl::justify(int width) {
				assert(glyphs_->indices.get() != nullptr && advances().begin() != nullptr);
				HRESULT hr = S_OK;
				const int totalAdvances = boost::accumulate(advances(), 0);
				if(width != totalAdvances) {
					if(glyphs_->justifiedAdvances.get() == nullptr)
						glyphs_->justifiedAdvances.reset(new int[numberOfGlyphs()]);
					hr = ::ScriptJustify(visualAttributes().begin(), advances().begin(), numberOfGlyphs(), width - totalAdvances,
						2, glyphs_->justifiedAdvances.get() + (begin() - glyphs_->position));
				}
				return hr;
			}

			inline HRESULT TextRunImpl::logicalAttributes(SCRIPT_LOGATTR attributes[]) const {
				raiseIfNull(attributes, "attributes");
				return ::ScriptBreak(begin(), static_cast<int>(length()), &analysis_, attributes);
			}

			/// @see GlyphVector#logicalBounds
			graphics::Rectangle TextRunImpl::logicalBounds() const {
				const auto xs = effectiveAdvances();
				Scalar left = std::numeric_limits<Scalar>::max(), right = std::numeric_limits<Scalar>::min();
				for(std::size_t i = 0, c = numberOfGlyphs(); i < c; ++i) {
					const Scalar x = glyphLogicalPosition(i);
					left = std::min(x, left);
					right = std::max(x + xs[i], right);
				}
				return graphics::Rectangle(boost::irange(left, right), logicalExtents());
			}

			inline HRESULT TextRunImpl::logicalWidths(int widths[]) const {
				raiseIfNull(widths, "widths");
				return ::ScriptGetLogicalWidths(&analysis_, static_cast<int>(length()),
					numberOfGlyphs(), advances().begin(), clusters().begin(), visualAttributes().begin(), widths);
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
		std::unique_ptr<TextRunImpl> piece(new SimpleRun(back.style));				\
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

				std::shared_ptr<const Font> font;	// font for current glyph run
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
							new TextRunImpl(Range<Index>(!calculatedRuns.empty() ? calculatedRuns.back()->end() : 0, newRunEnd - layoutString.beginning()),
								scriptRun.attribute->a, font,
								(scriptTags != nullptr) ? scriptTags[scriptRun.attribute - scriptRuns] : SCRIPT_TAG_UNKNOWN));	// TODO: 'DFLT' is preferred?
						calculatedStylesIndices.push_back(calculatedStyles.size());
						while(true) {
							std::unique_ptr<TextRunImpl> piece(calculatedRuns.back()->splitIfTooLong());
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
			std::size_t TextRunImpl::numberOfGlyphs() const BOOST_NOEXCEPT {
				return glyphRange().size();
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
			inline void TextRunImpl::paintGlyphs(PaintContext& context, const Point& origin, const StringPiece& range, bool onlyStroke) const {
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
			void TextRunImpl::paintGlyphs(PaintContext& context, const Point& origin/*, boost::optional<boost::integer_range<std::size_t>> range*/, bool onlyStroke) const {
//				if(!range)
//					return paintGlyphs(context, origin, *this, onlyStroke);
//				else if(range->empty())
//					return;

				context.setFont(font());
//				RECT temp;
//				if(dirtyRect != nullptr)
//					::SetRect(&temp, dirtyRect->left(), dirtyRect->top(), dirtyRect->right(), dirtyRect->bottom());
				if(onlyStroke && !win32::boole(::BeginPath(context.asNativeObject().get())))
					throw makePlatformError();
				assert(analysis_.fLogicalOrder == 0);
				// paint glyphs
				const RECT boundsToPaint(toNative<RECT>(context.boundsToPaint()));
				const boost::iterator_range<const int*> justifiedGlyphAdvances(justifiedAdvances());
				const HRESULT hr = ::ScriptTextOut(context.asNativeObject().get(), &glyphs_->fontCache,
					static_cast<int>(geometry::x(origin)), static_cast<int>(geometry::y(origin)),
					0, &boundsToPaint, &analysis_, nullptr, 0,
					glyphs().begin(), numberOfGlyphs(), advances().begin(),
					(justifiedGlyphAdvances.begin() != nullptr) ? justifiedGlyphAdvances.begin() : nullptr,
					glyphOffsets().begin());
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
			void TextRunImpl::positionGlyphs(win32::Handle<HDC>::Type dc, const ComputedTextRunStyle& style) {
				assert(glyphs_.get() != nullptr && glyphs_.unique());
				assert(glyphs_->indices.get() != nullptr && glyphs_->advances.get() == nullptr);

				std::unique_ptr<int[]> advances(new int[numberOfGlyphs()]);
				std::unique_ptr<GOFFSET[]> offsets(new GOFFSET[numberOfGlyphs()]);
//				ABC width;
				HRESULT hr = ::ScriptPlace(nullptr, &glyphs_->fontCache, glyphs_->indices.get(), numberOfGlyphs(),
					glyphs_->visualAttributes.get(), &analysis_, advances.get(), offsets.get(), nullptr/*&width*/);
				if(hr == E_PENDING) {
					HFONT oldFont = static_cast<HFONT>(::SelectObject(dc.get(), font()->asNativeObject().get()));
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
									std::memcpy(glyphIndices.get(), glyphs(), sizeof(WORD) * numberOfGlyphs());
								}
								glyphIndices[i] = fp.wgBlank;
							}
						}
					}
				}
*/
/*				// handle letter spacing
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
*/

				// commit
				glyphs_->advances = std::move(advances);
				glyphs_->offsets = std::move(offsets);
//				glyphs_->width = width;
			}

			/// @see GlyphVector#setGlyphPosition
			void TextRunImpl::setGlyphPosition(std::size_t index, const Point& position) {
				if(index > numberOfGlyphs())
					throw IndexOutOfBoundsException("index");
				const Scalar logicalPosition = glyphLogicalPosition(index);
				GOFFSET& glyphOffset = glyphs_->offsets[glyphOffsets().begin() - glyphs_->offsets.get()];
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

			void TextRunImpl::shape(win32::Handle<HDC>::Type dc) {
				assert(glyphs_.unique());

				// TODO: check if the requested style (or the default one) disables shaping.

				RawGlyphVector glyphs(glyphs_->position, glyphs_->font.get().font(), glyphs_->font.get().fontRenderContext(), glyphs_->scriptTag);
				HFONT oldFont = static_cast<HFONT>(::SelectObject(dc.get(), font()->asNativeObject().get()));
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
	std::wmemcpy(safeString.get(), layoutString.data() + beginning(), length());				\
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
//							if(font_ == nullptr && previousRun != nullptr) {
//								// use the previous run setting (but this will copy the style of the font...)
//								analysis_.eScript = previousRun->analysis_.eScript;
//								font_ = previousRun->font_;
//							}
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

			std::unique_ptr<TextRunImpl> TextRunImpl::splitIfTooLong() {
				if(estimateNumberOfGlyphs(length()) <= 65535)
					return std::unique_ptr<TextRunImpl>();

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
				std::unique_ptr<TextRunImpl> following(new TextRunImpl(
					followingRange, analysis_, glyphs_->font.get().font(), glyphs_->font.get().fontRenderContext(), glyphs_->scriptTag, style()));
				static_cast<StringPiece&>(*this) = StringPiece(begin(), opportunity);
				analysis_.fLinkAfter = following->analysis_.fLinkBefore = 0;
				return following;
			}

			/// @see GlyphVector#strokeGlyphs
			void TextRunImpl::strokeGlyphs(PaintContext& context, const Point& origin/*, boost::optional<boost::integer_range<std::size_t>> range = boost::none*/) const {
				return paintGlyphs(context, origin/*, range*/, true);
			}

			/**
			 * 
			 * @param runs the minimal runs
			 * @param layoutString the whole string of the layout
			 * @see #merge, #positionGlyphs
			 */
			void TextRunImpl::substituteGlyphs(const boost::iterator_range<std::vector<TextRunImpl*>::iterator>& runs) {
				// this method processes the following substitutions:
				// 1. missing glyphs
				// 2. ideographic variation sequences (if Uniscribe did not support)

				// 1. Presentative glyphs for missing ones

				// TODO: generate missing glyphs.

				// 2. Ideographic Variation Sequences (Uniscribe workaround)
				// Older Uniscribe (version < 1.626.7100.0) does not support IVS.

#ifdef ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
				if(!uniscribeSupportsIVS()) {
					for(auto i(runs.begin()); i != runs.end(); ++i) {
						TextRunImpl& run = **i;

						// process IVSes in a glyph run
						if(run.analysis_.eScript != SCRIPT_UNDEFINED && run.length() > 3
								&& text::surrogates::isHighSurrogate(run[0]) && text::surrogates::isLowSurrogate(run[1])) {
							for(text::StringCharacterIterator i(run, run.begin() + 2); i.hasNext(); i.next()) {
								const CodePoint variationSelector = i.current();
								if(variationSelector >= 0xe0100ul && variationSelector <= 0xe01eful) {
									text::StringCharacterIterator baseCharacter(i);
									baseCharacter.previous();
									if(run.font()->ivsGlyph(
											baseCharacter.current(), variationSelector,
											run.glyphs_->indices[run.glyphs_->clusters[baseCharacter.tell() - run.begin()]])) {
										run.glyphs_->vanish(*run.font(), i.tell());
										run.glyphs_->vanish(*run.font(), i.tell() + 1);
									}
								}
							}
						}

						// process an IVS across two glyph runs
						if(i + 1 != runs.end() && i[1]->length() > 1) {
							TextRunImpl& next = *i[1];
							const CodePoint variationSelector = text::utf::decodeFirst(next.begin(), next.begin() + 2);
							if(variationSelector >= 0xe0100ul && variationSelector <= 0xe01eful) {
								const CodePoint baseCharacter = text::utf::decodeLast(run.begin(), run.end());
								if(run.font()->ivsGlyph(baseCharacter, variationSelector,
										run.glyphs_->indices[run.glyphs_->clusters[run.length() - 1]])) {
									next.glyphs_->vanish(*run.font(), next.begin());
									next.glyphs_->vanish(*run.font(), next.begin() + 1);
								}
							}
						}
					}
#undef ASCENSION_VANISH_VARIATION_SELECTOR
				}
#endif // ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
			}

			/// @see GlyphVector#visualBounds
			graphics::Rectangle TextRunImpl::visualBounds() const {
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
				return graphics::Rectangle(geometry::_top = top, geometry::_right = right, geometry::_bottom = bottom, geometry::_left = left);
			}


			// InlineProgressionDimensionRangeIterator file-local class ///////////////////////////////////////////////

			namespace {
				class InlineProgressionDimensionRangeIterator :
					public boost::iterator_facade<InlineProgressionDimensionRangeIterator,
						boost::integer_range<Scalar>, std::input_iterator_tag, boost::integer_range<Scalar>, ptrdiff_t
					> {
				public:
					InlineProgressionDimensionRangeIterator() BOOST_NOEXCEPT
						: currentRun_(std::begin(dummy_)), lastRun_(std::begin(dummy_)) {}
					InlineProgressionDimensionRangeIterator(
						const boost::iterator_range<std::vector<std::unique_ptr<const TextRun>>::const_iterator>& textRunsOfLine,
						presentation::ReadingDirection layoutDirection, const StringPiece& effectiveCharacterRange,
						const Direction& scanningDirection, Scalar firstLineEdgeIpd);
					value_type dereference() const;
					const StringPiece& effectiveCharacterRange() const BOOST_NOEXCEPT {
						return effectiveCharacterRange_;
					}
					bool equal(const InlineProgressionDimensionRangeIterator& other) const BOOST_NOEXCEPT {
						return isDone() && other.isDone();
					}
					void increment() {
						return next(false);
					}
					Direction scanningDirection() const BOOST_NOEXCEPT {
						int temp = (currentRun_ <= lastRun_) ? 0 : 1;
						temp += (layoutDirection_ == presentation::LEFT_TO_RIGHT) ? 0 : 1;
						return (temp % 2 == 0) ? Direction::FORWARD : Direction::BACKWARD;
					}
				private:
					static presentation::ReadingDirection computeScanningReadingDirection(
							presentation::ReadingDirection layoutDirection, const Direction& scanningDirection) {
						presentation::ReadingDirection computed = layoutDirection;
						if(scanningDirection == Direction::BACKWARD)
							computed = !computed;
						return computed;
					}
					void next(bool initializing);
					bool isDone() const BOOST_NOEXCEPT {return currentRun_ == lastRun_;}
				private:
					static const std::vector<std::unique_ptr<const TextRun>> dummy_;
					friend class boost::iterator_core_access;
					/*const*/ presentation::ReadingDirection layoutDirection_;
					/*const*/ StringPiece effectiveCharacterRange_;
					std::vector<std::unique_ptr<const TextRun>>::const_iterator currentRun_;
					/*const*/ std::vector<std::unique_ptr<const TextRun>>::const_iterator lastRun_;
					Scalar currentRunAllocationStartEdge_;	// 'start' means for 'layoutDirection_'
				};

				InlineProgressionDimensionRangeIterator::InlineProgressionDimensionRangeIterator(
						const boost::iterator_range<std::vector<std::unique_ptr<const TextRun>>::const_iterator>& textRunsOfLine,
						presentation::ReadingDirection layoutDirection, const StringPiece& effectiveCharacterRange,
						const Direction& scanningDirection, Scalar firstLineEdgeIpd) :
						effectiveCharacterRange_(effectiveCharacterRange), layoutDirection_(layoutDirection),
						currentRunAllocationStartEdge_(firstLineEdgeIpd) {
					const presentation::ReadingDirection scanningReadingDirection = computeScanningReadingDirection(layoutDirection, scanningDirection);
					currentRun_ = (scanningReadingDirection == presentation::LEFT_TO_RIGHT) ? textRunsOfLine.begin() : textRunsOfLine.end() - 1;
					lastRun_ = (scanningReadingDirection == presentation::LEFT_TO_RIGHT) ? textRunsOfLine.end() : textRunsOfLine.begin() - 1;
					next(true);
				}

				InlineProgressionDimensionRangeIterator::value_type InlineProgressionDimensionRangeIterator::dereference() const {
					if(isDone())
						throw NoSuchElementException();
					const TextRunImpl& currentRun = static_cast<const TextRunImpl&>(**currentRun_);
					const presentation::FlowRelativeFourSides<Scalar>* const padding = currentRun.padding();
					const presentation::FlowRelativeFourSides<ComputedBorderSide>* const border = currentRun.border();
					const presentation::FlowRelativeFourSides<Scalar>* const margin = currentRun.margin();
					const Scalar allocationStartOffset =
						(padding != nullptr) ? padding->start() : 0
						+ (margin != nullptr) ? margin->start() : 0
						+ (border != nullptr) ? border->start().computedWidth() : 0;
					const auto subrange(intersection(boost::make_iterator_range(currentRun.characterRange()), boost::make_iterator_range(effectiveCharacterRange())));
					assert(!subrange.empty());
					Scalar startInRun = currentRun.hitToLogicalPosition(TextHit<>::leading(subrange.begin() - currentRun.begin()));
					Scalar endInRun = currentRun.hitToLogicalPosition(TextHit<>::trailing(subrange.end() - currentRun.begin()));
					if(currentRun.direction() == presentation::RIGHT_TO_LEFT) {
						const Scalar runMeasure = measure(currentRun);
						startInRun = runMeasure - startInRun;
						endInRun = runMeasure - endInRun;
					}
					startInRun += allocationStartOffset;
					endInRun += allocationStartOffset;
					assert(startInRun <= endInRun);
					const Scalar startOffset = (currentRun.direction() == layoutDirection_) ? startInRun : allocationMeasure(currentRun) - endInRun;
					const Scalar endOffset = (currentRun.direction() == layoutDirection_) ? endInRun : allocationMeasure(currentRun) - startInRun;
					assert(startOffset <= endOffset);
					return boost::irange(currentRunAllocationStartEdge_ + startOffset, currentRunAllocationStartEdge_ + endOffset);
				}

				void InlineProgressionDimensionRangeIterator::next(bool initializing) {
					if(isDone())
						throw NoSuchElementException();
					std::vector<std::unique_ptr<const TextRun>>::const_iterator nextRun(currentRun_);
					Scalar nextIpd = currentRunAllocationStartEdge_;
					const Direction sd = scanningDirection();
					const presentation::ReadingDirection srd = computeScanningReadingDirection(layoutDirection_, sd);
					while(nextRun != lastRun_) {
						if(sd == Direction::FORWARD) {
							if(initializing && intersects(boost::make_iterator_range((*nextRun)->characterRange()), boost::make_iterator_range(effectiveCharacterRange())))
								break;
							nextIpd += allocationMeasure(**nextRun);
						} else {
							nextIpd -= allocationMeasure(**nextRun);
							if(initializing && intersects(boost::make_iterator_range((*nextRun)->characterRange()), boost::make_iterator_range(effectiveCharacterRange())))
								break;
						}
						if(srd == presentation::LEFT_TO_RIGHT)
							++nextRun;
						else
							--nextRun;
						if(!initializing)
							break;
					}
					// commit
					currentRun_ = nextRun;
					currentRunAllocationStartEdge_ = nextIpd;
				}
			}


			// TextLayout /////////////////////////////////////////////////////////////////////////////////////////////

			// helpers for TextLayout.draw
			namespace {
				const std::size_t MAXIMUM_RUN_LENGTH = 1024;
				inline win32::Handle<HPEN>::Type createPen(const Color& color, int width, int style) {
					if(color.alpha() < 0xff)
						throw std::invalid_argument("color");
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
			 * Constructor.
			 * @param textString The text string to display
			 * @param lineStyle The computed text line style
			 * @param textRunStyles The computed text runs styles
			 * @param fontCollection The font collection
			 * @param fontRenderContext Information about a graphics device which is needed to measure the text correctly
			 */
			TextLayout::TextLayout(const String& textString, const ComputedTextLineStyle& lineStyle,
					std::unique_ptr<ComputedStyledTextRunIterator> textRunStyles, const FontCollection& fontCollection, const FontRenderContext& fontRenderContext)
					: textString_(textString), lineStyle_(lineStyle), numberOfLines_(0) {

				// handle logically empty line
				if(textString_.empty()) {
					numberOfLines_ = 1;
					maximumMeasure_ = 0.0f;
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
				std::vector<TextRunImpl*> textRuns;
				std::vector<AttributedCharacterRange<ComputedTextRunStyle>> calculatedStyles;
				TextRunImpl::generate(textString_, lineStyle_, move(textRunStyles), fontCollection, fontRenderContext, textRuns, calculatedStyles);
//				runs_.reset(new TextRun*[numberOfRuns_ = textRuns.size()]);
//				std::copy(textRuns.begin(), textRuns.end(), runs_.get());
//				shrinkToFit(styledRanges_);

				// 3. generate glyphs for each text runs
				const RenderingContext2D context(win32::detail::screenDC());
				BOOST_FOREACH(TextRunImpl* run, textRuns)
					run->shape(context.asNativeObject());
				TextRunImpl::substituteGlyphs(boost::make_iterator_range(textRuns));

				// 4. position glyphs for each text runs
				for(auto run(std::begin(textRuns)), b(std::begin(textRuns)), e(std::end(textRuns)); run != e; ++run)
					(*run)->positionGlyphs(context.asNativeObject(), calculatedStyles[run - b].attribute);

				// 5. position each text runs
				const FontDescription nominalFontDescription(
					!lineStyle.nominalFont.families.empty() ? lineStyle.nominalFont.families.front() : FontFamily(String()),
					lineStyle.nominalFont.pointSize, lineStyle.nominalFont.properties);
				boost::optional<double> nominalFontSizeAdjust;
				if(const presentation::FontSizeAdjustEnums* const keyword = boost::get<presentation::FontSizeAdjustEnums>(&lineStyle.nominalFont.sizeAdjust)) {
					if(*keyword == presentation::FontSizeAdjustEnums::NONE)
						nominalFontSizeAdjust = boost::none;
					else if(*keyword == presentation::FontSizeAdjustEnums::AUTO)
						nominalFontSizeAdjust = 1.0;
					else
						throw UnknownValueException("lineStyle.nominalFont.sizeAdjust");
				}
				const std::shared_ptr<const Font> nominalFont(fontCollection.get(nominalFontDescription,
					fontRotationForWritingMode(writingMode().blockFlowDirection), nominalFontSizeAdjust));
				// wrap into visual lines and reorder runs in each lines
				if(runs_.empty() || !wrapsText(lineStyle.whiteSpace)) {
					numberOfLines_ = 1;
					assert(firstRunsInLines_.get() == nullptr);
					// 5-2. reorder each text runs
					reorder();
					// 5-3. reexpand horizontal tabs
					expandTabsWithoutWrapping();
				} else {
					// 5-1. expand horizontal tabs and wrap into lines
					if(const std::shared_ptr<const TabExpander> tabExpander = lineStyle.tabExpander)
						wrap(lineStyle.measure, *tabExpander);
					else
						// create default tab expander
						wrap(lineStyle.measure, FixedWidthTabExpander(context.fontMetrics(nominalFont)->averageCharacterWidth() * 8));
					// 5-2. reorder each text runs
					reorder();
					// 5-3. reexpand horizontal tabs
					// TODO: not implemented.
					// 6. justify each text runs if specified
					if(lineStyle.justification != presentation::TextJustification::NONE)
						justify(lineStyle.measure, lineStyle.justification);
				}

				// 7. stack the lines
				stackLines(context, lineStyle.lineHeight, lineStyle.lineBoxContain, *nominalFont);
			}

			/**
			 * Returns the black box bounds of the characters in the specified range. The black box bounds is
			 * an area consisting of the union of the bounding boxes of the all of the characters in the range.
			 * The result region can be disjoint.
			 * @param characterRange The character range
			 * @return The native polygon object encompasses the black box bounds
			 * @throw IndexOutOfBoundsException @a range intersects with the outside of the line
			 * @see #bounds(void), #bounds(Index, Index), #lineBounds, #lineStartEdge
			 */
			boost::geometry::model::multi_polygon<boost::geometry::model::polygon<Point>>&& TextLayout::blackBoxBounds(const boost::integer_range<Index>& characterRange) const {
				const Index firstCharacter = std::min(*characterRange.begin(), *characterRange.end());
				const Index lastCharacter = std::max(*characterRange.begin(), *characterRange.end());
				if(lastCharacter > numberOfCharacters())
					throw IndexOutOfBoundsException("characterRange");
				boost::geometry::model::multi_polygon<boost::geometry::model::polygon<Point>> result;

				// handle empty line
				if(isEmpty())
					return std::move(result);

				// traverse all text runs intersect with 'characterRange'
				const boost::integer_range<Index> lines(lineAt(firstCharacter), lineAt(lastCharacter) + 1);
				LineMetricsIterator lm(lineMetrics(lines.front()));
				BOOST_FOREACH(Index line, lines) {
					// move to line-left edge of the line
					Point runTypographicOrigin = lm.baselineOffsetInPhysicalCoordinates();
					const Point ll = lineLeft(line);
					if(geometry::x(ll) != 0)
						geometry::x(runTypographicOrigin) = geometry::x(ll);
					else if(geometry::y(ll) != 0)
						geometry::y(runTypographicOrigin) = geometry::y(ll);

					const boost::iterator_range<RunVector::const_iterator> runs(runsForLine(line));
					for(RunVector::const_iterator run(runs.begin()); run != runs.end(); ++run, ++lm) {
						const boost::integer_range<Index> runRange = boost::irange<Index>(
							(*run)->characterRange().begin() - textString_.data(), (*run)->characterRange().end() - textString_.data());
						const auto intersection = ascension::intersection(runRange, characterRange);
						if(!boost::empty(intersection)) {
							const std::ptrdiff_t beginningOfRun = (*run)->characterRange().begin() - textString_.data();
							const boost::integer_range<Index> offsetsInRun = boost::irange(*intersection.begin() - beginningOfRun, *intersection.end() - beginningOfRun);
							std::vector<graphics::Rectangle> runBlackBoxBounds(static_cast<const TextRunImpl&>(**run).charactersBounds(offsetsInRun));
							AffineTransform typographicalToPhysicalMapping(geometry::makeTranslationTransform(
								geometry::_tx = geometry::x(runTypographicOrigin), geometry::_ty = geometry::y(runTypographicOrigin)));
							if(isVertical(writingMode().blockFlowDirection)) {
//								geometry::quadrantRotate(typographicalToPhysicalMapping, (resolveTextOrientation(writingMode()) != presentation::SIDEWAYS_LEFT) ? -1 : +1);
								typographicalToPhysicalMapping = AffineTransform(
									boost::numeric::ublas::prod(
										typographicalToPhysicalMapping.matrix(), 
										geometry::makeQuadrantRotationTransform((resolveTextOrientation(writingMode()) != presentation::SIDEWAYS_LEFT) ? -1 : +1).matrix()));
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
						if(isHorizontal(writingMode().blockFlowDirection))
							geometry::x(runTypographicOrigin) += allocationMeasure(**run);
						else if(resolveTextOrientation(writingMode()) != presentation::SIDEWAYS_LEFT)
							geometry::y(runTypographicOrigin) += allocationMeasure(**run);
						else
							geometry::y(runTypographicOrigin) -= allocationMeasure(**run);
					}
				}
				return std::move(result);
			}

			/**
			 * Returns the smallest rectangle emcompasses all characters in the range. It might not coincide
			 * exactly the ascent, descent or overhangs of the specified region of the text.
			 * @param characterRange The character range
			 * @return The bounds
			 * @throw IndexOutOfBoundsException @a characterRange intersects with the outside of the line
			 * @see #blackBoxBounds, #bounds(void), #lineBounds
			 */
			presentation::FlowRelativeFourSides<Scalar> TextLayout::bounds(const boost::integer_range<Index>& characterRange) const {
				const auto orderedCharacterRange(ordered(characterRange));
				if(*orderedCharacterRange.end() > numberOfCharacters())
					throw IndexOutOfBoundsException("characterRange");

				presentation::FlowRelativeFourSides<Scalar> result;

				if(isEmpty()) {	// empty line
					result.start() = result.end() = 0;
					const LineMetricsIterator lm(lineMetrics(0));
					result.before() = -lm.ascent();
					result.after() = lm.descent() + lm.leading();
				} else if(orderedCharacterRange.empty()) {	// an empty rectangle for an empty range
					const LineMetricsIterator lm(lineMetrics(lineAt(orderedCharacterRange.front())));
					const presentation::AbstractTwoAxes<Scalar> leading(hitToPoint(TextHit<>::leading(orderedCharacterRange.front())));
					presentation::FlowRelativeFourSides<Scalar> sides;
					sides.before() = leading.bpd() - lm.ascent();
					sides.after() = leading.bpd() + lm.descent() + lm.leading();
					sides.start() = sides.end() = leading.ipd();
					return sides;
				} else {
					const Index firstLine = lineAt(*orderedCharacterRange.begin()), lastLine = lineAt(*orderedCharacterRange.end());
					const LineMetricsIterator firstLineMetrics(lineMetrics(firstLine)), lastLineMetrics(lineMetrics(lastLine));

					// calculate the block-progression-edges ('before' and 'after'; it's so easy)
					result.before() = firstLineMetrics.baselineOffset() - firstLineMetrics.ascent();
					result.after() = lastLineMetrics.baselineOffset() + lastLineMetrics.descent() + lastLineMetrics.leading();

					// calculate start-edge and end-edge of fully covered lines
					const bool firstLineIsFullyCovered = includes(orderedCharacterRange,
						boost::irange(lineOffset(firstLine), lineOffset(firstLine) + lineLength(firstLine)));
					const bool lastLineIsFullyCovered = includes(orderedCharacterRange,
						boost::irange(lineOffset(lastLine), lineOffset(lastLine) + lineLength(lastLine)));
					result.start() = std::numeric_limits<Scalar>::max();
					result.end() = std::numeric_limits<Scalar>::min();
					for(Index line = firstLine + firstLineIsFullyCovered ? 0 : 1;
							line < lastLine + lastLineIsFullyCovered ? 1 : 0; ++line) {
						const Scalar lineStart = lineStartEdge(line);
						result.start() = std::min(lineStart, result.start());
						result.end() = std::max(lineStart + measure(line), result.end());
					}

					// calculate start and end-edge of partially covered lines
					std::vector<Index> partiallyCoveredLines;
					if(!firstLineIsFullyCovered)
						partiallyCoveredLines.push_back(firstLine);
					if(!lastLineIsFullyCovered && (partiallyCoveredLines.empty() || partiallyCoveredLines[0] != lastLine))
						partiallyCoveredLines.push_back(lastLine);
					if(!partiallyCoveredLines.empty()) {
						Scalar start = result.start(), end = result.end();
						const StringPiece effectiveCharacterRange(textString_.data() + orderedCharacterRange.front(), orderedCharacterRange.size());
						BOOST_FOREACH(Index line, partiallyCoveredLines) {
							const boost::iterator_range<RunVector::const_iterator> runs(runsForLine(line));

							// find 'start-edge'
							InlineProgressionDimensionRangeIterator i(runs,
								writingMode().inlineFlowDirection, effectiveCharacterRange, Direction::FORWARD, lineStartEdge(line));
							assert(i != InlineProgressionDimensionRangeIterator());
							start = std::min(*i->begin(), start);

							// find 'end-edge'
							i = InlineProgressionDimensionRangeIterator(runs,
								writingMode().inlineFlowDirection, effectiveCharacterRange, Direction::BACKWARD, lineStartEdge(line) + measure(line));
							assert(i != InlineProgressionDimensionRangeIterator());
							end = std::max(*i->end(), end);
						}

						result.start() = start;
						result.end() = end;
					}
				}

				return result;
			}

			namespace {
				inline bool borderShouldBePainted(const presentation::FlowRelativeFourSides<ComputedBorderSide>& borders) {
					const std::array<ComputedBorderSide, 4>& temp = borders;	// sigh...
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
			 * @param paintOverride Can be @c null
			 * @param endOfLine The inline object which paints an end-of-line. Can be @c null
			 * @param lineWrappingMark The inline object which paints line-wrapping-mark. Can be @c null
			 */
			void TextLayout::draw(PaintContext& context,
					const Point& origin, const TextPaintOverride* paintOverride /* = nullptr */,
					const InlineObject* endOfLine/* = nullptr */, const InlineObject* lineWrappingMark /* = nullptr */) const {

#if /*defined(_DEBUG)*/ 0
				if(DIAGNOSE_INHERENT_DRAWING)
					BOOST_LOG_TRIVIAL(debug) << L"@TextLayout.draw draws line " << lineNumber_ << L" (" << line << L")\n";
#endif // defined(_DEBUG)

				if(isEmpty() || geometry::dy(context.boundsToPaint()) == 0)
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
				boost::integer_range<Index> linesToPaint(0, numberOfLines());
				{
					Dimension originDistance(geometry::_dx = geometry::x(origin), geometry::_dy = geometry::y(origin));
					geometry::negate(originDistance);
					graphics::Rectangle boundsToPaint(context.boundsToPaint());
					geometry::translate(boundsToPaint, originDistance);
					const presentation::FlowRelativeFourSides<Scalar> abstractBoundsToPaint(	// relative to the alignment point of this layout
						presentation::mapPhysicalToFlowRelative<Scalar>(writingMode(), PhysicalFourSides<Scalar>(boundsToPaint)));
					for(LineMetricsIterator line(lineMetrics(linesToPaint.front())); line.line() != *linesToPaint.end(); ++line) {
						const Scalar bpd = line.baselineOffset();
						const Scalar lineBeforeEdge = bpd - line.ascent();
						const Scalar lineAfterEdge = bpd + line.descent();
						if(lineBeforeEdge <= abstractBoundsToPaint.before() && lineAfterEdge > abstractBoundsToPaint.before())
							linesToPaint = boost::irange(line.line(), *linesToPaint.end());
						if(lineBeforeEdge <= abstractBoundsToPaint.after() && lineAfterEdge > abstractBoundsToPaint.after()) {
							linesToPaint = boost::irange(*linesToPaint.begin(), line.line() + 1);
							break;
						}
					}
				}
#if 0
				// calculate inline area range to draw
				const Range<const RunVector::const_iterator> textRunsToPaint(
					firstRunInLine(linesToPaint.beginning()),
					(linesToPaint.end() < numberOfLines()) ? firstRunInLine(linesToPaint.end()) : runs_.end());
				AbstractTwoAxes<Scalar> alignmentPoint;	// alignment-point of text run relative to this layout
#endif
				context.save();
//				context.setTextAlign();
//				context.setTextBaseline();
//				::SetTextAlign(context.nativeObject().get(), TA_TOP | TA_LEFT | TA_NOUPDATECP);

				// 2. paint backgrounds and borders
				const bool horizontalLayout = isHorizontal(writingMode().blockFlowDirection);
				std::vector<std::tuple<const std::reference_wrapper<const TextRunImpl>, const graphics::Rectangle, const Point>> textRunsToPaint;
				for(LineMetricsIterator line(lineMetrics(linesToPaint.front())); line.line() != *linesToPaint.end(); ++line) {
					Point p(origin);	// a point at which baseline and (logical) 'line-left' edge of 'allocation-rectangle' of text run
					Scalar over, under;		// 'over' and 'under' edges of this line (x for vertical layout or y for horizontal layout)

					// move 'p' to start of line and compute 'over/under' of line
					if(horizontalLayout) {
						geometry::x(p) += (writingMode().inlineFlowDirection == presentation::LEFT_TO_RIGHT) ?
							lineStartEdge(line.line()) : -(lineStartEdge(line.line()) + measure(line.line()));
						geometry::y(p) += line.baselineOffset();
						over = geometry::y(p) - line.ascent();
						under = geometry::y(p) + line.descent();
					} else {
						assert(isVertical(writingMode().blockFlowDirection));
						geometry::x(p) += (writingMode().blockFlowDirection == presentation::VERTICAL_RL) ? -line.baselineOffset() : line.baselineOffset();
						over = geometry::x(p) + line.ascent();
						under = geometry::x(p) - line.descent();
						if(resolveTextOrientation(writingMode()) != presentation::SIDEWAYS_LEFT)
							geometry::y(p) += (writingMode().inlineFlowDirection == presentation::LEFT_TO_RIGHT) ?
								lineStartEdge(line.line()) : -(lineStartEdge(line.line()) + measure(line.line()));
						else {
							geometry::y(p) -= (writingMode().inlineFlowDirection == presentation::LEFT_TO_RIGHT) ?
								lineStartEdge(line.line()) : -(lineStartEdge(line.line()) + measure(line.line()));
							std::swap(over, under);
						}
					}

					graphics::Rectangle allocationRectangle;
					if(horizontalLayout)
						geometry::range<1>(allocationRectangle) = boost::irange(over, under);
					else
						geometry::range<0>(allocationRectangle) = boost::irange(over, under);
//					context.setGlobalAlpha(1.0);
//					context.setGlobalCompositeOperation(SOURCE_OVER);
					BOOST_FOREACH(const std::unique_ptr<const TextRun>& run, runsForLine(line.line())) {
						// check if this text run is beyond bounds to paint
						// TODO: Consider overhangs.
						if(horizontalLayout) {
							if(geometry::x(p) >= geometry::right(context.boundsToPaint()))
								break;
						} else {
							if(geometry::y(p) >= geometry::bottom(context.boundsToPaint()))
								break;
						}

						// compute next position of 'p', 'border-box' and 'allocation-box'
						Point q(p);
						if(horizontalLayout)
							geometry::x(q) += allocationMeasure(*run);
						else if(resolveTextOrientation(writingMode()) != presentation::SIDEWAYS_LEFT)
							geometry::y(q) += allocationMeasure(*run);
						else
							geometry::y(q) -= allocationMeasure(*run);
						bool skipThisRun = boost::geometry::equals(q, p);	// skip empty box

						// check if this text run intersects with bounds to paint
						// TODO: Consider overhangs.
						if(!skipThisRun)
							skipThisRun = horizontalLayout ?
								(geometry::x(q) < geometry::left(context.boundsToPaint()))
								: (geometry::y(q) < geometry::top(context.boundsToPaint()));
						if(!skipThisRun) {
							// 2-1. paint 'allocation-rectangle'
							if(horizontalLayout)
								geometry::range<0>(allocationRectangle) = boost::irange(geometry::x(p), geometry::x(q));
							else
								geometry::range<1>(allocationRectangle) = boost::irange(geometry::y(p), geometry::y(q));
							context.setFillStyle(lineStyle_.get().background);
							context.fillRectangle(allocationRectangle);

							// 2-2. compute 'content-rectangle'
							const presentation::FlowRelativeFourSides<Scalar> abstractContentBox(contentBox(*run)), abstractAllocationBox(allocationBox(*run));
							graphics::Rectangle contentRectangle;
							if(horizontalLayout) {
								if(writingMode().inlineFlowDirection == presentation::LEFT_TO_RIGHT)
									geometry::range<0>(contentRectangle) = boost::irange(
										geometry::x(p) + abstractContentBox.start() - abstractAllocationBox.start(),
										geometry::x(p) + abstractContentBox.end() - abstractAllocationBox.start());
								else
									geometry::range<0>(contentRectangle) = boost::irange(
										geometry::x(p) + abstractContentBox.end() - abstractAllocationBox.end(),
										geometry::x(p) + abstractContentBox.start() - abstractAllocationBox.end());
								geometry::range<1>(contentRectangle) =
									boost::irange(geometry::y(p) + abstractContentBox.before(), geometry::y(p) + abstractContentBox.after());
							} else {
								if(writingMode().blockFlowDirection == presentation::VERTICAL_RL)
									geometry::range<0>(contentRectangle) =
										boost::irange(geometry::x(p) - abstractContentBox.before(), geometry::x(p) - abstractContentBox.after());
								else {
									assert(writingMode().blockFlowDirection == presentation::VERTICAL_LR);
									geometry::range<0>(contentRectangle) =
										boost::irange(geometry::x(p) + abstractContentBox.before(), geometry::x(p) + abstractContentBox.after());
								}
								bool ttb = writingMode().inlineFlowDirection == presentation::LEFT_TO_RIGHT;
								ttb = (resolveTextOrientation(writingMode()) != presentation::SIDEWAYS_LEFT) ? ttb : !ttb;
								if(ttb)	// ttb
									geometry::range<1>(contentRectangle) = boost::irange(
										geometry::y(p) + abstractContentBox.start() - abstractAllocationBox.start(),
										geometry::y(p) + abstractContentBox.end() - abstractAllocationBox.start());
								else	// btt
									geometry::range<1>(contentRectangle) = boost::irange(
										geometry::y(p) + abstractContentBox.end() - abstractAllocationBox.end(),
										geometry::y(p) + abstractContentBox.start() - abstractAllocationBox.end());
							}

							// compute 'border-rectangle' if needed
							const auto& runStyle = static_cast<const TextRunImpl&>(*run).style();
							const Point& alignmentPoint = (writingMode().inlineFlowDirection == presentation::LEFT_TO_RIGHT) ? p : q;
							graphics::Rectangle borderRectangle;
							if(runStyle.background || borderShouldBePainted(runStyle.border))
								borderRectangle = geometry::make<graphics::Rectangle>(
									mapFlowRelativeToPhysical(writingMode(), borderBox(*run)) + PhysicalTwoAxes<Scalar>(alignmentPoint));

							// 2-3. paint background
							if(runStyle.background) {
								context.setFillStyle(runStyle.background);
								context.fillRectangle(borderRectangle);
							}

							// 2-4. paint borders
							PhysicalFourSides<const ComputedBorderSide*> physicalBorders;
							for(auto border(std::begin(runStyle.border)), e(std::end(runStyle.border)); border != e; ++border) {
								const presentation::FlowRelativeDirection direction = static_cast<presentation::FlowRelativeDirection>(border - std::begin(runStyle.border));
								physicalBorders[mapFlowRelativeToPhysical(writingMode(), direction)] = &*border;
							}
							for(auto border(std::begin(physicalBorders)), e(std::end(physicalBorders)); border != e; ++border) {
								if(!(*border)->hasVisibleStyle()) {
									*border = nullptr;
									continue;
								}
								const PhysicalDirection direction = static_cast<PhysicalDirection>(border - std::begin(physicalBorders));
							}

							// store this text run to paint the glyphs
							textRunsToPaint.push_back(std::make_tuple(std::cref(static_cast<const TextRunImpl&>(*run)), contentRectangle, alignmentPoint));
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

#if 0
					// draw outside of the selection
					Rect<> runRect;
					runRect.top = y;
					runRect.bottom = y + dy;
					runRect.left = x = startX;
					dc.setBkMode(TRANSPARENT);
					for(std::size_t i = firstRun; i < lastRun; ++i) {
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
						for(std::size_t i = firstRun; i < lastRun; ++i) {
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
						for(std::size_t i = firstRun; i < lastRun; ++i) {
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
						const kernel::Newline nlf(document.getLineInformation(lineNumber_).newline());
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
#if 0
			/// Expands the all tabs and resolves each width.
			inline void TextLayout::expandTabsWithoutWrapping() BOOST_NOEXCEPT {
				const String& s = text();
				const int fullTabWidth = lip_.textMetrics().averageCharacterWidth() * lip_.layoutSettings().tabWidth;
				int x = 0;

				if(lineTerminatorOrientation(style(), lip_.presentation().defaultTextLineStyle()) == LEFT_TO_RIGHT) {	// expand from the left most
					for(std::size_t i = 0; i < numberOfRuns_; ++i) {
						TextRun& run = *runs_[i];
						run.expandTabCharacters(s, x, fullTabWidth, numeric_limits<int>::max());
						x += run.totalWidth();
					}
				} else {	// expand from the right most
					for(std::size_t i = numberOfRuns_; i > 0; --i) {
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

			/// Justifies the wrapped visual lines.
			inline void TextLayout::justify(Scalar lineMeasure, presentation::TextJustification) BOOST_NOEXCEPT {
				for(Index line = 0; line < numberOfLines(); ++line) {
					const Scalar ipd = measure(line);
					for(auto i(firstRunInLine(line)), e(firstRunInLine(line + 1)); i != e; ++i) {
						TextRunImpl& run = *const_cast<TextRunImpl*>(static_cast<const TextRunImpl*>(i->get()));
						const Scalar newRunMeasure = allocationMeasure(run) * lineMeasure / ipd;	// TODO: There is more precise way.
						run.justify(static_cast<int>(newRunMeasure));
					}
				}
			}

			/// Reorders the runs in visual order.
			inline void TextLayout::reorder() {
				if(isEmpty())
					return;
				std::vector<const TextRun*> reordered(runs_.size());
				for(Index line = 0; line < numberOfLines(); ++line) {
					const boost::iterator_range<const RunVector::const_iterator> runsInLine(firstRunInLine(line), firstRunInLine(line + 1));
					const std::unique_ptr<BYTE[]> levels(new BYTE[runsInLine.size()]);
					for(RunVector::const_iterator i(runsInLine.begin()); i != runsInLine.end(); ++i)
						levels[i - runsInLine.begin()] = static_cast<BYTE>((*i)->characterLevel() & 0x1f);
					const std::unique_ptr<int[]> log2vis(new int[runsInLine.size()]);
					const HRESULT hr = ::ScriptLayout(static_cast<int>(runsInLine.size()), levels.get(), nullptr, log2vis.get());
					if(FAILED(hr))
						throw makePlatformError(hr);
					for(RunVector::const_iterator i(runsInLine.begin()); i != runsInLine.end(); ++i)
						reordered[runsInLine.begin() - begin(runs_) + log2vis[i - runsInLine.begin()]] = i->get();
				}

				// commit
				for(RunVector::iterator i(begin(runs_)), e(end(runs_)); i != e; ++i)
					i->release();
				for(RunVector::size_type i = 0, c(runs_.size()); i < c; ++i)
					runs_[i].reset(reordered[i]);
			}

			/**
			 * @internal Locates the wrap points and resolves tab expansions.
			 * @param measure
			 * @param tabExpander
			 */
			void TextLayout::wrap(Scalar measure, const TabExpander& tabExpander) BOOST_NOEXCEPT {
				assert(!isEmpty());
				assert(numberOfLines_ == 0 && firstRunsInLines_.get() == nullptr);

				std::vector<Index> firstRunsInLines;
				firstRunsInLines.push_back(0);
				Scalar ipd1 = 0;	// addresses the beginning of the run. see x2
				std::unique_ptr<int[]> logicalWidths;
				std::unique_ptr<SCRIPT_LOGATTR[]> logicalAttributes;
				Index longestRunLength = 0;	// for efficient allocation
				std::vector<TextRun*> runs;
				runs.reserve(runs_.size() * 3 / 2);
				std::vector<std::unique_ptr<TextRunImpl>> createdRuns;	// for only exception safety
				// for each runs... (at this time, 'runs_' is in logical order)
				BOOST_FOREACH(RunVector::const_reference p, runs_) {
					TextRunImpl* run = const_cast<TextRunImpl*>(static_cast<const TextRunImpl*>(p.get()));

					// if the run is a tab, expand and calculate actual width
					if(run->expandTabCharacters(tabExpander, textString_,
							(ipd1 < measure) ? ipd1 : 0, measure - (ipd1 < measure) ? ipd1 : 0)) {
						if(ipd1 < measure) {
							ipd1 += allocationMeasure(*run);
							runs.push_back(run);
						} else {
							ipd1 = allocationMeasure(*run);
							runs.push_back(run);
							firstRunsInLines.push_back(runs.size());
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
					const String::const_pointer originalRunPosition = run->begin();
					Scalar measureInThisRun = 0;
					String::const_pointer lastBreakable = run->begin(), lastGlyphEnd = run->begin();
					Scalar lastBreakableIpd = ipd1, lastGlyphEndIpd = ipd1;
					// for each characters in the run...
					for(StringPiece::const_iterator j = run->begin(); j < run->end(); ) {	// j is position in the LOGICAL line
						const Scalar ipd2 = ipd1 + measureInThisRun;
						// remember this opportunity
						if(logicalAttributes[j - run->begin()].fCharStop != 0) {
							lastGlyphEnd = j;
							lastGlyphEndIpd = ipd2;
							if(logicalAttributes[j - run->begin()].fSoftBreak != 0
									|| logicalAttributes[j - run->begin()].fWhiteSpace != 0) {
								lastBreakable = j;
								lastBreakableIpd = ipd2;
							}
						}
						// break if the width of the visual line overs the wrap width
						if(ipd2 + logicalWidths[j - run->begin()] > measure) {
							// the opportunity is the start of this run
							if(lastBreakable == run->begin()) {
								// break at the last glyph boundary if no opportunities
								if(firstRunsInLines.empty() || firstRunsInLines.back() == runs.size()) {
									if(lastGlyphEnd == run->begin()) {	// break here if no glyph boundaries
										lastBreakable = j;
										lastBreakableIpd = ipd2;
									} else {
										lastBreakable = lastGlyphEnd;
										lastBreakableIpd = lastGlyphEndIpd;
									}
								}
							}

							// case 1: break at the start of the run
							if(lastBreakable == run->begin()) {
								assert(firstRunsInLines.empty() || runs.size() != firstRunsInLines.back());
								firstRunsInLines.push_back(runs.size());
//BOOST_LOG_TRIVIAL(debug) << L"broke the line at " << lastBreakable << L" where the run start.\n";
							}
							// case 2: break at the end of the run
							else if(lastBreakable == run->end()) {
								if(lastBreakable < textString_.data() + numberOfCharacters()) {
									assert(firstRunsInLines.empty() || runs.size() != firstRunsInLines.back());
									firstRunsInLines.push_back(runs.size() + 1);
//BOOST_LOG_TRIVIAL(debug) << L"broke the line at " << lastBreakable << L" where the run end.\n";
								}
								break;
							}
							// case 3: break at the middle of the run -> split the run (run -> newRun + run)
							else {
								std::unique_ptr<TextRunImpl> followingRun(run->breakAt(lastBreakable));
								runs.push_back(run);
								assert(firstRunsInLines.empty() || runs.size() != firstRunsInLines.back());
								firstRunsInLines.push_back(runs.size());
//BOOST_LOG_TRIVIAL(debug) << L"broke the line at " << lastBreakable << L" where the run meddle.\n";
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
//BOOST_LOG_TRIVIAL(debug) << L"...broke the all lines.\n";
#if 0
				if(runs.empty())
					runs.push_back(nullptr);
#else
				assert(!runs.empty());
#endif

				// commit
				decltype(runs_) newRuns(runs.size());
				assert(numberOfLines() > 1);	// ???
				firstRunsInLines_.reset(new RunVector::const_iterator[firstRunsInLines.size()]);

				for(auto i(begin(runs_)), e(end(runs_)); i != e; ++i)
					i->release();
				for(RunVector::size_type i = 0, c = runs.size(); i < c; ++i)
					newRuns[i].reset(runs[i]);
				runs_ = std::move(newRuns);
				for(std::vector<Index>::size_type i = 0, c = firstRunsInLines.size(); i < c; ++i)
					firstRunsInLines_[i] = std::begin(runs_) + firstRunsInLines[i];
			}
		}
	}

	void updateSystemSettings() {
		graphics::font::systemColors.update();
		graphics::font::userSettings.update();
	}
}
