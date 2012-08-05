/**
 * @file font-gdi.cpp
 * @author exeal
 * @date 2010-10-15 created
 * @date 2012-06-30 renamed from font-windows.cpp
 */

#include <ascension/graphics/font.hpp>
#include <ascension/graphics/rendering-context.hpp>
#include <ascension/config.hpp>
#include <vector>

#ifdef ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
#	include <ascension/corelib/text/character.hpp>	// text.isValidCodePoint
#endif //ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND

using namespace ascension;
using namespace ascension::graphics;
using namespace ascension::graphics::font;
using namespace std;

#ifdef ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
namespace {
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

	void generateIVSMappings(const void* cmapData, size_t length, detail::IdeographicVariationSequences& ivs) {
//	if(length > ) {
			const uint8_t* p = static_cast<const uint8_t*>(cmapData);
			const uint32_t numberOfSubtables = readBytes<2>(p += 2);
			const uint8_t* uvsSubtable = nullptr;
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
			if(uvsSubtable != nullptr) {
				p = uvsSubtable + 6;
				const uint32_t numberOfRecords = readBytes<4>(p);
				for(uint32_t i = 0; i < numberOfRecords; ++i) {
					const uint32_t varSelector = readBytes<3>(p);
					if(const uint32_t defaultUVSOffset = readBytes<4>(p)) {
						const uint8_t* q = uvsSubtable/*static_cast<const uint8_t*>(cmapData)*/ + defaultUVSOffset;
						const uint32_t numUnicodeValueRanges = readBytes<4>(q);
						for(uint32_t j = 0; j < numUnicodeValueRanges; ++j) {
							const uint32_t startUnicodeValue = readBytes<3>(q);
							const uint8_t additionalCount = readBytes<1>(q);
							for(uint32_t c = startUnicodeValue; c <= startUnicodeValue + additionalCount; ++c)
								ivs.defaultMappings.push_back(((varSelector - 0x0e0100u) << 24) | c);
						}
					}
					if(const uint32_t nonDefaultUVSOffset = readBytes<4>(p)) {
						const uint8_t* q = uvsSubtable/*static_cast<const uint8_t*>(cmapData)*/ + nonDefaultUVSOffset;
						const uint32_t numberOfMappings = readBytes<4>(q);
						for(uint32_t j = 0; j < numberOfMappings; ++j) {
							const uint32_t unicodeValue = readBytes<3>(q);
							const uint32_t glyphID = readBytes<2>(q);
							ivs.nonDefaultMappings.insert(
								make_pair(((varSelector - 0x0e0100u) << 24) | unicodeValue, static_cast<uint16_t>(glyphID)));
						}
					}
				}
				sort(ivs.defaultMappings.begin(), ivs.defaultMappings.end());
			}
//		}
	}
} // namespace @0
#endif // ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND

Font::Font(win32::Handle<HFONT>&& handle) /*noexcept*/ : nativeObject_(move(handle)) {
}

const win32::Handle<HFONT>& Font::asNativeObject() const /*noexcept*/ {
	return nativeObject_;
}

#ifdef ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
boost::optional<GlyphCode> Font::ivsGlyph(CodePoint baseCharacter, CodePoint variationSelector, GlyphCode defaultGlyph) const {
	if(!text::isValidCodePoint(baseCharacter))
		throw invalid_argument("baseCharacter");
	else if(!text::isValidCodePoint(variationSelector))
		throw invalid_argument("variationSelector");
	else if(variationSelector < 0x0e0100ul || variationSelector > 0x0e01eful)
		return boost::none;
	if(ivs_.get() == nullptr) {
		const_cast<Font*>(this)->ivs_.reset(new detail::IdeographicVariationSequences);
		win32::Handle<HDC> dc(detail::screenDC());
		win32::Handle<HFONT> oldFont(static_cast<HFONT>(::SelectObject(dc.get(), nativeObject_.get())));
		static const TrueTypeFontTag CMAP_TAG = MakeTrueTypeFontTag<'c', 'm', 'a', 'p'>::value;
		const DWORD bytes = ::GetFontData(dc.get(), CMAP_TAG, 0, nullptr, 0);
		if(bytes != GDI_ERROR) {
			unique_ptr<uint8_t[]> data(new uint8_t[bytes]);
			if(GDI_ERROR != ::GetFontData(dc.get(), CMAP_TAG, 0, data.get(), bytes))
				generateIVSMappings(data.get(), bytes, *const_cast<Font*>(this)->ivs_);
		}
		::SelectObject(dc.get(), oldFont.get());
	}

	const uint32_t v = ((variationSelector - 0x0e0100ul) << 24) | baseCharacter;
	if(binary_search(ivs_->defaultMappings.begin(), ivs_->defaultMappings.end(), v))
		return boost::make_optional(defaultGlyph);
	unordered_map<uint32_t, uint16_t>::const_iterator i(ivs_->nonDefaultMappings.find(v));
	if(i == ivs_->nonDefaultMappings.end())
		return boost::none;
	return boost::make_optional(i->second);
}
#endif //ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND

namespace {
	class FontMetrics : public Font::Metrics {
	public:
		explicit FontMetrics(const win32::Handle<HFONT>& font) {
			win32::Handle<HDC> dc(detail::screenDC());
			win32::Handle<HFONT> oldFont(static_cast<HFONT>(::SelectObject(dc.get(), font.get())));
			::SetGraphicsMode(dc.get(), GM_ADVANCED);
//			const double xdpi = dc.getDeviceCaps(LOGPIXELSX);
//			const double ydpi = dc.getDeviceCaps(LOGPIXELSY);

			// generic font metrics
			TEXTMETRICW tm;
			if(!win32::boole(::GetTextMetricsW(dc.get(), &tm))) {
				::SelectObject(dc.get(), oldFont.get());
				throw makePlatformError();
			}
			ascent_ = tm.tmAscent/* * 96.0 / ydpi*/;
			descent_ = tm.tmDescent/* * 96.0 / ydpi*/;
			internalLeading_ = tm.tmInternalLeading/* * 96.0 / ydpi*/;
			externalLeading_ = tm.tmExternalLeading/* * 96.0 / ydpi*/;
			averageCharacterWidth_ = max<int>(((tm.tmAveCharWidth > 0) ? tm.tmAveCharWidth : ::MulDiv(tm.tmHeight, 56, 100)), 1)/* * 96.0 / xdpi*/;

			// x-height
			GLYPHMETRICS gm;
			const MAT2 temp = {{0, 1}, {0, 0}, {0, 0}, {0, 1}};
			xHeight_ = (::GetGlyphOutlineW(dc.get(), L'x', GGO_METRICS, &gm, 0, nullptr, nullptr) != GDI_ERROR
				&& gm.gmptGlyphOrigin.y > 0) ? gm.gmptGlyphOrigin.y : round(static_cast<double>(ascent_) * 0.56);
			::SelectObject(dc.get(), oldFont.get());
		}
		int ascent() const /*noexcept*/ {return ascent_;}
		int averageCharacterWidth() /*noexcept*/ const {return averageCharacterWidth_;}
		int descent() const /*noexcept*/ {return descent_;}
		int externalLeading() const /*noexcept*/ {return externalLeading_;}
		int internalLeading() const /*noexcept*/ {return internalLeading_;}
		int xHeight() const /*noexcept*/ {return xHeight_;}
	private:
		int ascent_, descent_, internalLeading_, externalLeading_, averageCharacterWidth_, xHeight_;
	};
}

void Font::buildMetrics() {
	metrics_.reset(new FontMetrics(nativeObject_));
}

shared_ptr<const Font> FontCollection::cache(const FontDescription<>& description, double sizeAdjust) {
	if(description.familyName().length() >= LF_FACESIZE)
		throw length_error("");

	const String& familyName = description.familyName();
	const FontProperties<>& properties = description.properties();

	// TODO: handle properties.orientation().

	LOGFONTW lf;
	memset(&lf, 0, sizeof(LOGFONTW));
	lf.lfHeight = -round(description.pixelSize());
	lf.lfWeight = properties.weight;
	lf.lfItalic = (properties.style == FontStyle::ITALIC) || (properties.style == FontStyle::OBLIQUE);
	wcscpy(lf.lfFaceName, familyName.c_str());
	win32::Handle<HFONT> font(::CreateFontIndirectW(&lf), &::DeleteObject);
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
		win32::Handle<HDC> dc(detail::screenDC());
		win32::Handle<HFONT> oldFont(static_cast<HFONT>(::SelectObject(dc.get(), font.get())));
		TEXTMETRICW tm;
		if(::GetTextMetricsW(dc.get(), &tm)) {
			GLYPHMETRICS gm;
			const MAT2 temp = {{0, 1}, {0, 0}, {0, 0}, {0, 1}};
			const int xHeight =
				(::GetGlyphOutlineW(dc.get(), L'x', GGO_METRICS, &gm, 0, nullptr, nullptr) != GDI_ERROR && gm.gmptGlyphOrigin.y > 0) ?
					gm.gmptGlyphOrigin.y : round(static_cast<double>(tm.tmAscent) * 0.56);
			const double aspect = static_cast<double>(xHeight) / static_cast<double>(tm.tmHeight - tm.tmInternalLeading);
			FontDescription<> adjustedDescription(description);
			adjustedDescription.setPixelSize(max(description.pixelSize() * (sizeAdjust / aspect), 1.0));
			::SelectObject(dc.get(), oldFont.get());
			return cache(description, 0.0);
		}
		::SelectObject(dc.get(), oldFont.get());
	}

	// handle 'font-stretch'
	if(properties.stretch != FontStretch::NORMAL) {
		// TODO: this implementation is too simple...
		if(::GetObjectW(font.get(), sizeof(LOGFONTW), &lf) > 0) {
			static const int WIDTH_RATIOS[] = {1000, 1000, 1000, 500, 625, 750, 875, 1125, 1250, 1500, 2000, 1000};
			lf.lfWidth = ::MulDiv(lf.lfWidth, WIDTH_RATIOS[properties.stretch], 1000);
			if(win32::Handle<HFONT> temp = win32::Handle<HFONT>(::CreateFontIndirectW(&lf), &::DeleteObject))
				font = move(temp);
		}
	}

	shared_ptr<Font> newFont(new Font(move(font)));
	cachedFonts_.insert(make_pair(description, newFont));
	return newFont;
}

shared_ptr<const Font> FontCollection::lastResortFallback(const FontDescription<>& description, double sizeAdjust) const {
	static String familyName;
	// TODO: 'familyName' should update when system property changed.
	if(familyName.empty()) {
		LOGFONTW lf;
		if(::GetObjectW(static_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT)), sizeof(LOGFONTW), &lf) != 0)
			familyName = lf.lfFaceName;
		else {
			win32::AutoZeroSize<NONCLIENTMETRICSW> ncm;
			if(!win32::boole(::SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICSW), &ncm, 0)))
				throw makePlatformError();
			familyName = ncm.lfMessageFont.lfFaceName;
		}
	}

	FontDescription<> modified(description);
	return get(modified.setFamilyName(familyName), sizeAdjust);
}

/// Returns the object implements @c FontCollection interface.
const FontCollection& font::installedFonts() {
	static FontCollection instance;
	return instance;
}
