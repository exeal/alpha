/**
 * @file font.hpp
 * @author exeal
 * @date 2010-11-06 created
 * @date 2010-2012
 */

#ifndef ASCENSION_FONT_HPP
#define ASCENSION_FONT_HPP

#include <ascension/graphics/font-description.hpp>
#include <ascension/graphics/glyph-vector.hpp>
#include <locale>
#include <set>
#ifdef ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
#	include <unordered_map>
#endif // ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
#include <boost/optional.hpp>

namespace ascension {
#ifdef ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
	namespace detail {
		struct IdeographicVariationSequences {
			std::vector<std::uint32_t> defaultMappings;
			std::unordered_map<std::uint32_t, std::uint16_t> nonDefaultMappings;
		};
	}
#endif // ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND

	namespace graphics {
		namespace font {

			/// Returns @c true if complex scripts are supported.
			bool supportsComplexScripts() BOOST_NOEXCEPT;
			/// Returns @c true if OpenType features are supported.
			bool supportsOpenTypeFeatures() BOOST_NOEXCEPT;

			/**
			 * Represents a single physical instance of a font, or a set of fonts.
			 * @see FontFamily, FontDescription, Fontset, FontFace, FontCollection
			 */
			class Font : public std::enable_shared_from_this<Font> {
			public:
				/**
				 * Abstract class provides physical font metrics information.
				 * @see Font#metrics
				 */
				class Metrics {
				public:
					/// Returns the ascent of the text in font coordinate units.
					virtual int ascent() const BOOST_NOEXCEPT = 0;
					/// Returns the average width of a character in font coordinate units.
					virtual int averageCharacterWidth() const BOOST_NOEXCEPT = 0;
					/// Returns the cell height in font coordinate units.
					int cellHeight() const BOOST_NOEXCEPT {return ascent() + descent();}
					/// Returns the descent of the text in font coordinate units.
					virtual int descent() const BOOST_NOEXCEPT = 0;
					/// Returns the em height in font coordinate units.
					int emHeight() const BOOST_NOEXCEPT {return cellHeight() - internalLeading();}
					/// Returns the external leading in font coordinate units.
					/// @note In Ascension, external leadings are placed below characters.
					virtual int externalLeading() const BOOST_NOEXCEPT = 0;
					/// Returns the internal leading in font coordinate units.
					virtual int internalLeading() const BOOST_NOEXCEPT = 0;
					/// Returns the gap of the lines (external leading) in font coordinate units.
					int lineGap() const BOOST_NOEXCEPT {return externalLeading();}
					/// Returns the pitch of lines in pixels.
					/// @note This method ignores @c LayoutSettings#lineSpacing value.
					int linePitch() const BOOST_NOEXCEPT {return cellHeight() + lineGap();}
					/// Returns the x-height of the font in font coordinate units.
					virtual int xHeight() const BOOST_NOEXCEPT = 0;
				};
			public:
#if defined(ASCENSION_SHAPING_ENGINE_CAIRO)
				explicit Font(Cairo::RefPtr<Cairo::ScaledFont> nativeObject);
				Glib::RefPtr<Cairo::ScaledFont> asNativeObject();
				Glib::RefPtr<const Cairo::ScaledFont> asNativeObject() const;
#elif defined(ASCENSION_SHAPING_ENGINE_CORE_GRAPHICS)
				explicit Font(cg::Reference<CGFontRef>&& nativeObject);
				cg::Reference<CGFontRef>& asNativeObject() const;
#elif defined(ASCENSION_SHAPING_ENGINE_CORE_TEXT)
				explicit Font(cg::Reference<CTFontRef>&& nativeObject);
				cg::Reference<CTFontRef>& asNativeObject() const;
#elif defined(ASCENSION_SHAPING_ENGINE_DIRECT_WRITE)
				explicit Font(win32::com::SmartPointer<IDWriteFont> nativeObject);
				win32::com::SmartPointer<IDWriteFont> asNativeObject() const;
#elif defined(ASCENSION_SHAPING_ENGINE_HARFBUZZ)
				explicit Font(boost::intrusive_ptr<hb_font_t> nativeObject);
				boost::intrusive_ptr<hb_font_t> asNativeObject() const;
#elif defined(ASCENSION_SHAPING_ENGINE_PANGO)
//				explicit Font(Glib::RefPtr<Pango::Font> nativeObject);
//				Glib::RefPtr<Pango::Font> asNativeObject();
//				Glib::RefPtr<const Pango::Font> asNativeObject() const;
				explicit Font(Glib::RefPtr<Pango::Fontset> nativeObject);
				Glib::RefPtr<Pango::Fontset> asNativeObject();
				Glib::RefPtr<const Pango::Fontset> asNativeObject() const;
#elif defined(ASCENSION_SHAPING_ENGINE_QT)
//				explicit Font(std::shared_ptr<QRawFont> nativeObject);
//				std::shared_ptr<QRawFont> asNativeObject();
//				std::shared_ptr<const QRawFont> asNativeObject() const;
				explicit Font(std::shared_ptr<QFont> nativeObject);
				std::shared_ptr<QFont> asNativeObject();
				std::shared_ptr<const QFont> asNativeObject() const;
#elif defined(ASCENSION_SHAPING_ENGINE_UNISCRIBE) || defined(ASCENSION_SHAPING_ENGINE_WIN32_GDI)
				explicit Font(win32::Handle<HFONT>::Type nativeObject) BOOST_NOEXCEPT;
				win32::Handle<HFONT>::Type asNativeObject() const BOOST_NOEXCEPT;
#elif defined(ASCENSION_SHAPING_ENGINE_WIN32_GDIPLUS)
				explicit Font(std::shared_ptr<Gdiplus::Font> nativeObject);
				std::shared_ptr<Gdiplus::Font> asNativeObject();
				std::shared_ptr<const Gdiplus::Font> asNativeObject() const;
#endif
#if 0
				/**
				 * Creates a @c GlyphVector by mapping characters to glyphs one-to-one based on the
				 * Unicode cmap in this font. This method does no other processing besides the
				 * mapping of glyphs to characters. This means that this method is not useful for
				 * some scripts, such as Arabic, Hebrew, Thai, and Indic, that require reordering,
				 * shaping, or ligature substitution.
				 * @param text The text string
				 * @return A new @c GlyphVector created with the specified string
				 */
				std::unique_ptr<const GlyphVector> createGlyphVector(const String& text) const;
#endif
				/// Returns the description of this font.
				FontDescription&& describe() const BOOST_NOEXCEPT;
				/// Returns the family name of this font.
				FontFamily&& family() const;
#ifdef ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
				boost::optional<GlyphCode> ivsGlyph(CodePoint baseCharacter,
					CodePoint variationSelector, GlyphCode defaultGlyph) const;
#endif //ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
				/// Returns the metrics of the font.
				std::shared_ptr<const Metrics> metrics() const BOOST_NOEXCEPT {
					if(metrics_.get() == nullptr)
						const_cast<Font*>(this)->buildMetrics();
					return metrics_;
				}
			private:
				void buildMetrics() BOOST_NOEXCEPT;
#if defined(ASCENSION_SHAPING_ENGINE_CAIRO)
				Cairo::RefPtr<Cairo::ScaledFont> nativeObject_;
#elif defined(ASCENSION_SHAPING_ENGINE_CORE_GRAPHICS)
				cg::Reference<CGFontRef> nativeObject_;
#elif defined(ASCENSION_SHAPING_ENGINE_CORE_TEXT)
				cg::Reference<CTFontRef> nativeObject_;
#elif defined(ASCENSION_SHAPING_ENGINE_DIRECT_WRITE)
				win32::com::SmartPointer<IDWriteFont> nativeObject_;
#elif defined(ASCENSION_SHAPING_ENGINE_HARFBUZZ)
				boost::intrusive_ptr<hb_font_t> nativeObject_;
#elif defined(ASCENSION_SHAPING_ENGINE_PANGO)
//				Glib::RefPtr<Pango::Font> nativeObject_;
				Glib::RefPtr<Pango::Fontset> nativeObject_;
#elif defined(ASCENSION_SHAPING_ENGINE_QT)
//				std::shared_ptr<QRawFont> nativeObject_;
				std::shared_ptr<QFont> nativeObject_;
#elif defined(ASCENSION_SHAPING_ENGINE_UNISCRIBE) || defined(ASCENSION_SHAPING_ENGINE_WIN32_GDI)
				win32::Handle<HFONT>::Type nativeObject_;
#ifdef ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
				std::unique_ptr<detail::IdeographicVariationSequences> ivs_;
#endif //ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
#elif defined(ASCENSION_SHAPING_ENGINE_WIN32_GDIPLUS)
				std::shared_ptr<Gdiplus::Font> nativeObject_;
#endif
				std::shared_ptr<const Metrics> metrics_;
			};

			/**
			 * @c FontCollection represents the set of fonts available for a particular graphics
			 * context, and provides a method to enumerate font families.
			 * @see Fontset, RenderingContext2D
			 */
			class FontCollection {
			public:
#if defined(ASCENSION_SHAPING_ENGINE_CORE_TEXT)
#elif defined(ASCENSION_SHAPING_ENGINE_DIRECT_WRITE)
#elif defined(ASCENSION_SHAPING_ENGINE_HARFBUZZ)
#elif defined(ASCENSION_SHAPING_ENGINE_PANGO)
#elif defined(ASCENSION_SHAPING_ENGINE_QT)
#elif defined(ASCENSION_SHAPING_ENGINE_UNISCRIBE) || defined(ASCENSION_SHAPING_ENGINE_WIN32_GDI)
				explicit FontCollection(win32::Handle<HDC>::Type deviceContext) BOOST_NOEXCEPT;
#elif defined(ASCENSION_SHAPING_ENGINE_WIN32_GDIPLUS)
#endif
				/// Returns a set of font families available in this collection.
				std::set<FontFamily>&& families() const;
				/**
				 * Returns the fontset matches the given description.
				 * @param description The font description
				 * @param sizeAdjust The 'font-size-adjust' value. Set @c boost#none for 'none'
				 * @return The font has the requested description or the default one
				 */
				std::shared_ptr<const Font> get(
					const FontDescription& description,
					boost::optional<double> sizeAdjust = boost::none) const;
				/**
				 * Returns the fontset for last resort fallback.
				 * @param description The font description
				 * @param sizeAdjust The 'font-size-adjust' value. Set @c boost#none for 'none'
				 * @return The font has the requested property
				 */
				std::shared_ptr<const Font> lastResortFallback(
					const FontDescription& description,
					boost::optional<double> sizeAdjust = boost::none) const;
			private:
#if defined(ASCENSION_SHAPING_ENGINE_CORE_TEXT)
				cg::Reference<CTFontCollectionRef> nativeObject_;
#elif defined(ASCENSION_SHAPING_ENGINE_DIRECT_WRITE)
				win32::com::SmartPointer<IDWriteFontCollection> nativeObject_;
#elif defined(ASCENSION_SHAPING_ENGINE_HARFBUZZ)
#elif defined(ASCENSION_SHAPING_ENGINE_PANGO)
				Glib::RefPtr<Pango::FontMap> nativeObject_;
#elif defined(ASCENSION_SHAPING_ENGINE_QT)
				std::shared_ptr<QFontDatabase> nativeObject_;
#elif defined(ASCENSION_SHAPING_ENGINE_UNISCRIBE) || defined(ASCENSION_SHAPING_ENGINE_WIN32_GDI)
				win32::Handle<HDC>::Type deviceContext_;
#elif defined(ASCENSION_SHAPING_ENGINE_WIN32_GDIPLUS)
				std::shared_ptr<Gdiplus::FontCollection> nativeObject_;
#endif
			};

			template<typename InputIterator>
			InputIterator findMatchingFontFamily(
				const FontCollection& fontCollection, InputIterator first, InputIterator last);

			/**
			 * Used to represent a group of fonts with the same family, slant, weight, width, but
			 * varying sizes.
			 */
			class FontFace {
			public:
				const FontDescription& describe() const;
				/// Returns the face name.
				const String& name() const;
			private:
				const FontDescription description_;
				const String name_;
			};

			class FontFaceIterator {};
			class FontSizeIterator {};

			FontFaceIterator availableFaces(const FontCollection& collection, const FontFamily& family);
			FontSizeIterator availablePointSizes(const FontFace& fontFace);

			inline presentation::FlowRelativeFourSides<Scalar> GlyphVector::glyphLogicalBounds(const Range<std::size_t>& range) const {
				presentation::FlowRelativeFourSides<Scalar> sides;
				sides.start() = glyphPosition(range.beginning());
				sides.end() = glyphPosition(range.end());
				std::shared_ptr<const Font::Metrics> fontMetrics(font()->metrics());
				sides.before() = -fontMetrics->ascent();
				sides.after() = fontMetrics->descent();
				return sides;
			}

		}
	}
}

#endif // !ASCENSION_FONT_HPP
