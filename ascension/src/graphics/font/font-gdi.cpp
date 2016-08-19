/**
 * @file font-gdi.cpp
 * @author exeal
 * @date 2010-10-15 created
 * @date 2012-06-30 renamed from font-windows.cpp
 * @date 2012, 2014
 */

#include <ascension/graphics/font/font.hpp>
#include <ascension/graphics/font/font-collection.hpp>

#if ASCENSION_SELECTS_SHAPING_ENGINE(UNISCRIBE) || ASCENSION_SELECTS_SHAPING_ENGINE(WIN32_GDI)
#include <ascension/graphics/font/font-description.hpp>
#include <ascension/graphics/font/font-render-context.hpp>
#include <ascension/graphics/native-conversion.hpp>
#include <ascension/graphics/rendering-context.hpp>
#include <ascension/config.hpp>
#include <ascension/win32/system-default-font.hpp>
#include <boost/core/null_deleter.hpp>
#include <boost/functional/hash.hpp>	// boost.hash_combine, boost.hash_value
#include <boost/math/special_functions/round.hpp>
#include <boost/mpl/string.hpp>
#include <boost/range/algorithm/binary_search.hpp>
#include <boost/range/algorithm/sort.hpp>
#include <vector>
#if ASCENSION_SELECTS_SHAPING_ENGINE(UNISCRIBE)
#	include <usp10.h>
#	pragma comment(lib, "usp10.lib")
#endif // ASCENSION_SELECTS_SHAPING_ENGINE(UNISCRIBE)

namespace ascension {
	namespace graphics {
		namespace {
			void fillLogFont(win32::Handle<HDC>::Type deviceContext,
					const font::FontDescription& description, const AffineTransform& transform, boost::optional<Scalar> sizeAdjust, LOGFONT& out) {
				const String& familyName = description.family().name();
				if(familyName.length() >= LF_FACESIZE)
					throw std::length_error("description.family().name()");

				LONG orientation;
				if(geometry::equals(transform, geometry::makeQuadrantRotationTransform(0)))
					orientation = 0;
				else if(geometry::equals(transform, geometry::makeQuadrantRotationTransform(1)))
					orientation = 2700;
				else if(geometry::equals(transform, geometry::makeQuadrantRotationTransform(2)))
					orientation = 1800;
				else if(geometry::equals(transform, geometry::makeQuadrantRotationTransform(3)))
					orientation = 900;
				else
					throw std::invalid_argument("transform");	// TODO: This version of code supports only simple quadrant-rotations.

				const int oldMapMode = ::SetMapMode(deviceContext.get(), MM_TEXT);
				if(oldMapMode == 0)
					throw makePlatformError();
				int dpi;
				if(orientation == 0 || orientation == 1800)
					dpi = ::GetDeviceCaps(deviceContext.get(), LOGPIXELSY);
				else if(orientation == 900 || orientation == 2700)
					dpi = ::GetDeviceCaps(deviceContext.get(), LOGPIXELSX);
				else
					dpi = static_cast<int>(std::sqrt(
						std::pow(static_cast<float>(::GetDeviceCaps(deviceContext.get(), LOGPIXELSX)), 2)
						+ std::pow(static_cast<float>(::GetDeviceCaps(deviceContext.get(), LOGPIXELSY)), 2)));
				::SetMapMode(deviceContext.get(), oldMapMode);

				// TODO: handle properties.orientation().

				LOGFONTW lf;
				std::memset(&lf, 0, sizeof(decltype(lf)));
				lf.lfHeight = -boost::math::lround(description.pointSize() * dpi / 72.0);
				lf.lfEscapement = lf.lfOrientation = orientation;
				lf.lfWeight = boost::underlying_cast<LONG>(description.properties().weight);
				lf.lfItalic = (description.properties().style == font::FontStyle::ITALIC) || (description.properties().style == font::FontStyle::OBLIQUE);
				std::wcscpy(lf.lfFaceName, familyName.c_str());

				// handle 'font-size-adjust'
				if(sizeAdjust != boost::none && boost::get(sizeAdjust) > 0.0) {
					win32::Handle<HFONT>::Type font(::CreateFontIndirectW(&lf), &::DeleteObject);
					win32::Handle<HFONT>::Type oldFont(static_cast<HFONT>(::SelectObject(deviceContext.get(), font.get())), boost::null_deleter());
					TEXTMETRICW tm;
					if(win32::boole(::GetTextMetricsW(deviceContext.get(), &tm))) {
						GLYPHMETRICS gm;
						const MAT2 temp = {{0, 1}, {0, 0}, {0, 0}, {0, 1}};
						const int xHeight =
							(::GetGlyphOutlineW(deviceContext.get(), L'x', GGO_METRICS, &gm, 0, nullptr, nullptr) != GDI_ERROR && gm.gmptGlyphOrigin.y > 0) ?
								gm.gmptGlyphOrigin.y : boost::math::iround(static_cast<double>(tm.tmAscent) * 0.56);
						const double aspect = static_cast<double>(xHeight) / static_cast<double>(tm.tmHeight - tm.tmInternalLeading);
						font::FontDescription adjustedDescription(description);
						adjustedDescription.setPointSize(std::max(description.pointSize() * (boost::get(sizeAdjust) / aspect), 1.0));
						::SelectObject(deviceContext.get(), oldFont.get());
						return fillLogFont(deviceContext, adjustedDescription, transform, boost::none, out);
					}
					::SelectObject(deviceContext.get(), oldFont.get());
				}

				// handle 'font-stretch'
				if(description.properties().stretch != font::FontStretch::NORMAL) {
					// TODO: this implementation is too simple...
					win32::Handle<HFONT>::Type font(::CreateFontIndirectW(&lf), &::DeleteObject);
					if(::GetObjectW(font.get(), sizeof(decltype(lf)), &lf) > 0) {
						static const int WIDTH_RATIOS[] = {1000, 1000, 1000, 500, 625, 750, 875, 1125, 1250, 1500, 2000, 1000};
						lf.lfWidth = ::MulDiv(lf.lfWidth, WIDTH_RATIOS[boost::underlying_cast<std::size_t>(description.properties().stretch)], 1000);
					}
				}

				std::swap(lf, out);
			}
		}

		namespace font {
#ifdef ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
			namespace {
				template<std::size_t bytes> std::uint32_t readBytes(const std::uint8_t*& p);
				template<> inline std::uint32_t readBytes<1>(const std::uint8_t*& p) {
					return *(p++);
				}
				template<> inline std::uint32_t readBytes<2>(const std::uint8_t*& p) {
					p += 2;
					return (p[-2] << 8) | (p[-1] << 0);
				}
				template<> inline std::uint32_t readBytes<3>(const std::uint8_t*& p) {
					p += 3;
					return (p[-3] << 16) | (p[-2] << 8) | (p[-1] << 0);
				}
				template<> inline std::uint32_t readBytes<4>(const std::uint8_t*& p) {
					p += 4;
					return (p[-4] << 24) | (p[-3] << 16) | (p[-2] << 8) | (p[-1] << 0);
				}

				void generateIVSMappings(const void* cmapData, std::size_t length, detail::IdeographicVariationSequences& ivs) {
//					if(length > ) {
						const std::uint8_t* p = static_cast<const std::uint8_t*>(cmapData);
						const std::uint32_t numberOfSubtables = readBytes<2>(p += 2);
						const std::uint8_t* uvsSubtable = nullptr;
						for(std::uint16_t i = 0; i < numberOfSubtables; ++i) {
							const std::uint32_t platformID = readBytes<2>(p);
							const std::uint32_t encodingID = readBytes<2>(p);
							const std::uint32_t offset = readBytes<4>(p);
							const std::uint8_t* temp = static_cast<const std::uint8_t*>(cmapData) + offset;
							const std::uint32_t format = readBytes<2>(temp);
							if(format == 14 && platformID == 0 && encodingID == 5) {
								uvsSubtable = temp - 2;
								break;
							}
						}
						if(uvsSubtable != nullptr) {
							p = uvsSubtable + 6;
							const std::uint32_t numberOfRecords = readBytes<4>(p);
							for(std::uint32_t i = 0; i < numberOfRecords; ++i) {
								const std::uint32_t varSelector = readBytes<3>(p);
								if(const std::uint32_t defaultUVSOffset = readBytes<4>(p)) {
									const std::uint8_t* q = uvsSubtable/*static_cast<const std::uint8_t*>(cmapData)*/ + defaultUVSOffset;
									const std::uint32_t numUnicodeValueRanges = readBytes<4>(q);
									for(std::uint32_t j = 0; j < numUnicodeValueRanges; ++j) {
										const std::uint32_t startUnicodeValue = readBytes<3>(q);
										const std::uint8_t additionalCount = readBytes<1>(q);
										for(std::uint32_t c = startUnicodeValue; c <= startUnicodeValue + additionalCount; ++c)
											ivs.defaultMappings.push_back(((varSelector - 0x0e0100u) << 24) | c);
									}
								}
								if(const std::uint32_t nonDefaultUVSOffset = readBytes<4>(p)) {
									const std::uint8_t* q = uvsSubtable/*static_cast<const std::uint8_t*>(cmapData)*/ + nonDefaultUVSOffset;
									const std::uint32_t numberOfMappings = readBytes<4>(q);
									for(std::uint32_t j = 0; j < numberOfMappings; ++j) {
										const std::uint32_t unicodeValue = readBytes<3>(q);
										const std::uint32_t glyphID = readBytes<2>(q);
										ivs.nonDefaultMappings.insert(
											std::make_pair(((varSelector - 0x0e0100u) << 24) | unicodeValue, static_cast<std::uint16_t>(glyphID)));
									}
								}
							}
							boost::sort(ivs.defaultMappings);
						}
//					}
				}
			} // namespace @0
#endif // ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND

			Font::Font(win32::Handle<HFONT>::Type handle) BOOST_NOEXCEPT : nativeObject_(std::move(handle)) {
			}

			void Font::buildDescription() BOOST_NOEXCEPT {
				assert(description_.get() == nullptr);

				LOGFONTW lf;
				if(::GetObjectW(native().get(), sizeof(decltype(lf)), &lf) == 0)
					throw makePlatformError();
				description_.reset(new FontDescription(fromNative<FontDescription>(lf)));
//				description_.reset(new FontDescription(graphics::detail::fromNative<FontDescription>(lf)));
			}
#if 0
			std::unique_ptr<const GlyphVector> Font::createGlyphVector(const FontRenderContext& frc, const StringPiece& text) const {
				const win32::Handle<HDC>::Type dc(win32::detail::screenDC());
				const int cookie = ::SaveDC(dc.get());
				::SelectObject(dc.get(), native().get());
				static_assert(sizeof(WORD) == sizeof(GlyphCode), "");
				std::vector<GlyphCode> glyphCodes(text.length());
#if ASCENSION_SELECTS_SHAPING_ENGINE(UNISCRIBE)
				SCRIPT_CACHE fontCache(nullptr);
				const HRESULT hr = ::ScriptGetCMap(dc.get(), &fontCache, text.data(), text.length(), 0, glyphCodes.data());
				::ScriptFreeCache(&fontCache);
				::RestoreDC(dc.get(), cookie);
				if(FAILED(hr))
					throw makePlatformError(hr);
#elif ASCENSION_SELECTS_SHAPING_ENGINE(WIN32_GDI)
				const DWORD n = ::GetGlyphIndicesW(dc.get(), text.data(), text.length(), glyphCodes.data(), GGI_MARK_NONEXISTING_GLYPHS);
				::RestoreDC(dc.get(), cookie);
				if(n == GDI_ERROR)
					throw makePlatformError();
#else
				ASCENSION_CANT_DETECT_PLATFORM();
#endif
				return createGlyphVector(frc, glyphCodes);
			}
#endif

#ifdef ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
			boost::optional<GlyphCode> Font::ivsGlyph(CodePoint baseCharacter, CodePoint variationSelector, GlyphCode defaultGlyph) const {
				if(!text::isValidCodePoint(baseCharacter))
					throw std::invalid_argument("baseCharacter");
				else if(!text::isValidCodePoint(variationSelector))
					throw std::invalid_argument("variationSelector");
				else if(variationSelector < 0x0e0100ul || variationSelector > 0x0e01eful)
					return boost::none;
				if(ivs_.get() == nullptr) {
					const_cast<Font*>(this)->ivs_.reset(new detail::IdeographicVariationSequences);
					win32::Handle<HDC>::Type dc(win32::detail::screenDC());
					win32::Handle<HFONT>::Type oldFont(static_cast<HFONT>(::SelectObject(dc.get(), nativeObject_.get())));
					static const OpenTypeLayoutTag CMAP_TAG = MakeOpenTypeLayoutTag<boost::mpl::string<'cmap'>>::value;
					const DWORD bytes = ::GetFontData(dc.get(), CMAP_TAG, 0, nullptr, 0);
					if(bytes != GDI_ERROR) {
						std::unique_ptr<std::uint8_t[]> data(new std::uint8_t[bytes]);
						if(GDI_ERROR != ::GetFontData(dc.get(), CMAP_TAG, 0, data.get(), bytes))
							generateIVSMappings(data.get(), bytes, *const_cast<Font*>(this)->ivs_);
					}
					::SelectObject(dc.get(), oldFont.get());
				}

				const std::uint32_t v = ((variationSelector - 0x0e0100ul) << 24) | baseCharacter;
				if(boost::binary_search(ivs_->defaultMappings, v))
					return boost::make_optional(defaultGlyph);
				std::unordered_map<std::uint32_t, std::uint16_t>::const_iterator i(ivs_->nonDefaultMappings.find(v));
				if(i == ivs_->nonDefaultMappings.end())
					return boost::none;
				return boost::make_optional(i->second);
			}
#endif //ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND

			std::unique_ptr<const LineMetrics> Font::lineMetrics(const StringPiece& text, const FontRenderContext& frc) const {
				struct LineMetricsImpl : public LineMetrics {
					Scalar ascent() const BOOST_NOEXCEPT override {return std::get<0>(adl);}
					DominantBaseline baseline() const BOOST_NOEXCEPT override {return DominantBaseline::ALPHABETIC;}
					Scalar baselineOffset(AlignmentBaseline baseline) const BOOST_NOEXCEPT override {return 0;}
					Scalar descent() const BOOST_NOEXCEPT override {return std::get<1>(adl);}
					Scalar leading() const BOOST_NOEXCEPT override {return std::get<2>(adl);}
					Scalar strikeThroughOffset() const BOOST_NOEXCEPT override {return std::get<0>(strikeThrough);}
					Scalar strikeThroughThickness() const BOOST_NOEXCEPT override {return std::get<1>(strikeThrough);}
					Scalar underlineOffset() const BOOST_NOEXCEPT override {return std::get<0>(underline);}
					Scalar underlineThickness() const BOOST_NOEXCEPT override {return std::get<1>(underline);}

					std::tuple<Scalar, Scalar, Scalar> adl;
					std::tuple<Scalar, Scalar> strikeThrough, underline;
				};

				win32::Handle<HDC>::Type dc(win32::detail::screenDC());
				const int cookie = ::SaveDC(dc.get());
				const XFORM xform(graphics::toNative<XFORM>(frc.transform()));
				std::unique_ptr<LineMetricsImpl> lm;
				if(::SetGraphicsMode(dc.get(), GM_ADVANCED) != 0 && ::SetMapMode(dc.get(), MM_TEXT) != 0 && ::SetWorldTransform(dc.get(), &xform)) {
					::SelectObject(dc.get(), native().get());
					if(const UINT bytes = ::GetOutlineTextMetricsW(dc.get(), 0, nullptr)) {
						OUTLINETEXTMETRICW* const otm = static_cast<OUTLINETEXTMETRICW*>(::operator new(bytes));
						if(::GetOutlineTextMetricsW(dc.get(), bytes, otm) != 0) {
							lm.reset(new LineMetricsImpl);
							std::get<0>(lm->adl) = static_cast<Scalar>(otm->otmAscent);
							std::get<1>(lm->adl) = static_cast<Scalar>(otm->otmDescent);
							std::get<2>(lm->adl) = static_cast<Scalar>(otm->otmTextMetrics.tmInternalLeading);
							std::get<0>(lm->strikeThrough) = static_cast<Scalar>(otm->otmsStrikeoutPosition);
							std::get<1>(lm->strikeThrough) = static_cast<Scalar>(otm->otmsStrikeoutSize);
							std::get<0>(lm->underline) = static_cast<Scalar>(otm->otmsUnderscorePosition);
							std::get<1>(lm->underline) = static_cast<Scalar>(otm->otmsUnderscoreSize);
						}
					} else {
						TEXTMETRICW tm;
						if(win32::boole(::GetTextMetricsW(dc.get(), &tm))) {
							lm.reset(new LineMetricsImpl);
							std::get<0>(lm->adl) = static_cast<Scalar>(tm.tmAscent);
							std::get<1>(lm->adl) = static_cast<Scalar>(tm.tmDescent);
							std::get<2>(lm->adl) = static_cast<Scalar>(tm.tmInternalLeading);
							std::get<0>(lm->strikeThrough) = static_cast<Scalar>(tm.tmAscent / 3.0);
							std::get<1>(lm->strikeThrough) = 1.0;
							std::get<0>(lm->underline) = static_cast<Scalar>(tm.tmAscent);
							std::get<1>(lm->underline) = 1.0;
						}
					}
				}
				::RestoreDC(dc.get(), cookie);

				if(lm.get() == nullptr)
					throw makePlatformError();
				return std::move(lm);
			}

			win32::Handle<HFONT>::Type Font::native() const BOOST_NOEXCEPT {
				return nativeObject_;
			}

			FontCollection::FontCollection(win32::Handle<HDC>::Type deviceContext) BOOST_NOEXCEPT : deviceContext_(deviceContext) {
				assert(::GetCurrentObject(deviceContext_.get(), OBJ_FONT) != nullptr);
			}

			std::shared_ptr<const Font> FontCollection::get(const FontDescription& description, const AffineTransform& transform, boost::optional<Scalar> sizeAdjust) const {
				LOGFONT lf;
				fillLogFont(deviceContext_, description, transform, sizeAdjust, lf);
				struct LogFontHash {
					std::size_t operator()(const LOGFONTW& v) const {
						std::size_t n = boost::hash_value(v.lfHeight);
						boost::hash_combine(n, v.lfEscapement);
						boost::hash_combine(n, v.lfWeight);
						boost::hash_combine(n, v.lfItalic);
						return n;
					}
				};
				struct LogFontComp {
					bool operator()(const LOGFONTW& lhs, const LOGFONTW& rhs) const BOOST_NOEXCEPT {
						return lhs.lfHeight == rhs.lfHeight
							&& lhs.lfEscapement == rhs.lfEscapement && lhs.lfWeight == rhs.lfWeight && lhs.lfItalic == rhs.lfItalic;
					}
				};
				static std::unordered_map<LOGFONTW, std::shared_ptr<const Font>, LogFontHash, LogFontComp> cachedFonts;
				const auto cache(cachedFonts.find(lf));
				if(cache != cachedFonts.end())
					return cache->second;

				win32::Handle<HFONT>::Type font(::CreateFontIndirectW(&lf), &::DeleteObject);
#ifdef _DEBUG
				LOGFONT lf2;
				if(::GetObjectW(font.get(), sizeof(decltype(lf2)), &lf2) > 0) {
					::OutputDebugStringW(L"[SystemFonts.cache] Created font '");
					::OutputDebugStringW(lf2.lfFaceName);
					::OutputDebugStringW(L"' for request '");
					::OutputDebugStringW(description.family().name().c_str());
					::OutputDebugStringW(L"'.\n");
				}
#endif
				std::shared_ptr<Font> newFont(new Font(std::move(font)));
				cachedFonts.insert(std::make_pair(lf, newFont));
				return newFont;
			}

			std::shared_ptr<const Font> FontCollection::lastResortFallback(double pointSize,
					const FontProperties& properties, const AffineTransform& transform, boost::optional<Scalar> sizeAdjust) const {
				static String familyName;
				// TODO: 'familyName' should update when system property changed.
				if(familyName.empty()) {
					LOGFONT lf;
					win32::systemDefaultFont(lf);
					familyName = lf.lfFaceName;
				}

				const FontDescription description(FontFamily(familyName), pointSize, properties);
				return get(description, transform, sizeAdjust);
			}
		}
	}
}

#endif	// ASCENSION_SELECTS_SHAPING_ENGINE(UNISCRIBE) || ASCENSION_SELECTS_SHAPING_ENGINE(WIN32_GDI)

#if ASCENSION_SUPPORTS_SHAPING_ENGINE(UNISCRIBE) || ASCENSION_SUPPORTS_SHAPING_ENGINE(WIN32_GDI) || ASCENSION_SUPPORTS_SHAPING_ENGINE(WIN32_GDIPLUS)
#include <ascension/graphics/rendering-device.hpp>

namespace ascension {
	namespace graphics {
		namespace detail {
			template<> font::FontDescription fromNative<font::FontDescription>(const LOGFONTW& object) {
				return font::FontDescription(font::FontFamily(object.lfFaceName), -object.lfHeight * 72 / defaultDpiY(),
					font::FontProperties(static_cast<font::FontWeight>(object.lfWeight),
						font::FontStretch::NORMAL, win32::boole(object.lfItalic) ? font::FontStyle::ITALIC : font::FontStyle::NORMAL));
			}

			LOGFONTW toNative(const font::FontDescription& object, const LOGFONTW* /* = nullptr */) {
//				fillLogFont(win32::detail::screenDC(), object, geometry::makeIdentityTransform(), boost::none);
				win32::AutoZero<LOGFONT> result;
				LONG orientation = 0;
#if 0
				if(object.properties().orientation == font::FontOrientation::VERTICAL)
					orientation = 900;
#endif
				result.lfHeight = static_cast<LONG>(-object.pointSize() * defaultDpiY() / 72);
				result.lfWeight = boost::underlying_cast<LONG>(object.properties().weight);
				result.lfItalic = object.properties().style != font::FontStyle::ITALIC || object.properties().style != font::FontStyle::OBLIQUE;
				std::wcsncpy(result.lfFaceName, object.family().name().c_str(), std::extent<decltype(result.lfFaceName)>::value);
				return result;
			}
		}
	}
}
#endif
