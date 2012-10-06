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
#include <unordered_map>
#include <boost/optional.hpp>

namespace ascension {
#ifdef ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
	namespace detail {
		struct IdeographicVariationSequences {
			std::vector<uint32_t> defaultMappings;
			std::unordered_map<uint32_t, uint16_t> nonDefaultMappings;
		};
	}
#endif // ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND

	namespace graphics {
		namespace font {

			/// Returns @c true if complex scripts are supported.
			bool supportsComplexScripts() /*noexcept*/;
			/// Returns @c true if OpenType features are supported.
			bool supportsOpenTypeFeatures() /*noexcept*/;

			/**
			 * Represents a single physical instance of a font.
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
					virtual int ascent() const /*noexcept*/ = 0;
					/// Returns the average width of a character in font coordinate units.
					virtual int averageCharacterWidth() const /*noexcept*/ = 0;
					/// Returns the cell height in font coordinate units.
					int cellHeight() const /*throw()*/ {return ascent() + descent();}
					/// Returns the descent of the text in font coordinate units.
					virtual int descent() const /*noexcept*/ = 0;
					/// Returns the em height in font coordinate units.
					int emHeight() const /*noexcept*/ {return cellHeight() - internalLeading();}
					/// Returns the external leading in font coordinate units.
					/// @note In Ascension, external leadings are placed below characters.
					virtual int externalLeading() const /*noexcept*/ = 0;
					/// Returns the internal leading in font coordinate units.
					virtual int internalLeading() const /*noexcept*/ = 0;
					/// Returns the gap of the lines (external leading) in font coordinate units.
					int lineGap() const /*noexcept*/ {return externalLeading();}
					/// Returns the pitch of lines in pixels.
					/// @note This method ignores @c LayoutSettings#lineSpacing value.
					int linePitch() const /*noexcept*/ {return cellHeight() + lineGap();}
					/// Returns the x-height of the font in font coordinate units.
					virtual int xHeight() const /*noexcept*/ = 0;
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
				explicit Font(Glib::RefPtr<Pango::Font> nativeObject);
				Glib::RefPtr<Pango::Font> asNativeObject();
				Glib::RefPtr<const Pango::Font> asNativeObject() const;
#elif defined(ASCENSION_SHAPING_ENGINE_QT)
				explicit Font(std::shared_ptr<QRawFont> nativeObject);
				std::shared_ptr<QRawFont> asNativeObject();
				std::shared_ptr<const QRawFont> asNativeObject() const;
#elif defined(ASCENSION_SHAPING_ENGINE_UNISCRIBE) || defined(ASCENSION_SHAPING_ENGINE_WIN32_GDI)
				explicit Font(win32::Handle<HFONT>&& nativeObject) /*noexcept*/;
				const win32::Handle<HFONT>& asNativeObject() const /*noexcept*/;
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
				FontDescription&& describe() const /*noexcept*/;
				/// Returns the family name of this font.
				FontFamily&& family() const;
#ifdef ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
				boost::optional<GlyphCode> ivsGlyph(CodePoint baseCharacter,
					CodePoint variationSelector, GlyphCode defaultGlyph) const;
#endif //ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
				/// Returns the metrics of the font.
				std::shared_ptr<const Metrics> metrics() const /*noexcept*/ {
					if(metrics_.get() == nullptr)
						const_cast<Font*>(this)->buildMetrics();
					return metrics_;
				}
			private:
				void buildMetrics() /*noexcept*/;
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
				Glib::RefPtr<Pango::Font> nativeObject_;
#elif defined(ASCENSION_SHAPING_ENGINE_QT)
				std::shared_ptr<QRawFont> nativeObject_;
#elif defined(ASCENSION_SHAPING_ENGINE_UNISCRIBE) || defined(ASCENSION_SHAPING_ENGINE_WIN32_GDI)
				win32::Handle<HFONT> nativeObject_;
#ifdef ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
				std::unique_ptr<detail::IdeographicVariationSequences> ivs_;
#endif //ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
#elif defined(ASCENSION_SHAPING_ENGINE_WIN32_GDIPLUS)
				std::shared_ptr<Gdiplus::Font> nativeObject_;
#endif
				std::shared_ptr<const Metrics> metrics_;
			};

			class Fontset : public std::enable_shared_from_this<Fontset> {
			public:
#if defined(ASCENSION_SHAPING_ENGINE_PANGO)
				explicit Font(Glib::RefPtr<Pango::Fontset> nativeObject);
				Glib::RefPtr<Pango::Fontset> asNativeObject();
				Glib::RefPtr<const Pango::Fontset> asNativeObject() const;
#elif defined(ASCENSION_SHAPING_ENGINE_QT)
				explicit Font(std::shared_ptr<QFont> nativeObject);
				std::shared_ptr<QFont> asNativeObject();
				std::shared_ptr<const QFont> asNativeObject() const;
#endif
			private:
#if defined(ASCENSION_SHAPING_ENGINE_PANGO)
				Glib::RefPtr<Pango::Fontset> nativeObject_;
#elif defined(ASCENSION_SHAPING_ENGINE_QT)
				std::shared_ptr<QFont> nativeObject_;
#endif
			};

			/// An interface represents an object provides a set of fonts.
			class FontCollection {
			public:
				/// Destructor.
				virtual ~FontCollection() /*throw()*/ {}
				std::set<FontFamily>&& families() const;
				/**
				 * Returns the font matches the given properties.
				 * @param familyName The font family name
				 * @param properties The font properties
				 * @param sizeAdjust 
				 * @return The font has the requested properties or the default one
				 */
				std::shared_ptr<const Font> get(
						const FontDescription& description, double sizeAdjust = 0.0) const {
					CachedFonts::const_iterator i(cachedFonts_.find(description));
					if(i != cachedFonts_.end())
						return i->second;
					return const_cast<FontCollection*>(this)->cache(description, sizeAdjust);
				}
				/**
				 * Returns the font for last resort fallback.
				 * @param properties The font properties
				 * @param sizeAdjust 
				 * @return The font has the requested property
				 */
				std::shared_ptr<const Font> lastResortFallback(
					const FontDescription& description, double sizeAdjust = 0.0) const;
			private:
				std::shared_ptr<const Font> cache(
					const FontDescription& description, double sizeAdjust);
#if defined(ASCENSION_SHAPING_ENGINE_CORE_TEXT)
				cg::Reference<CTFontCollectionRef> nativeObject_;
#elif defined(ASCENSION_SHAPING_ENGINE_DIRECT_WRITE)
				win32::com::SmartPointer<IDWriteFontCollection> nativeObject_;
#elif defined(ASCENSION_SHAPING_ENGINE_HARFBUZZ)
#elif defined(ASCENSION_SHAPING_ENGINE_PANGO)
				Glib::RefPtr<Pango::FontMap> nativeObject_;
#elif defined(ASCENSION_SHAPING_ENGINE_QT)
				std::shared_ptr<QFontDatabase> nativeObject_;
#elif defined(ASCENSION_SHAPING_ENGINE_WIN32_GDIPLUS)
				std::shared_ptr<Gdiplus::FontCollection> nativeObject_;
#endif
				typedef std::unordered_map<FontDescription, std::shared_ptr<Font>> CachedFonts;
				CachedFonts cachedFonts_;
			};

			const FontCollection& installedFonts();

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
