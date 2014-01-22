/**
 * @file font.hpp
 * @author exeal
 * @date 2010-11-06 created
 * @date 2010-2014
 */

#ifndef ASCENSION_FONT_HPP
#define ASCENSION_FONT_HPP

#include <ascension/corelib/string-piece.hpp>
#include <ascension/graphics/font/font-description.hpp>
#include <ascension/graphics/font/glyph-vector.hpp>
#include <ascension/graphics/font/text-alignment.hpp>
#include <locale>
#include <memory>
#include <set>
#ifdef ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
#	include <unordered_map>
#endif // ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
#include <vector>
#include <boost/optional.hpp>

namespace ascension {

	namespace graphics {
		namespace font {
#ifdef ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
			namespace detail {
				struct IdeographicVariationSequences {
					std::vector<std::uint32_t> defaultMappings;
					std::unordered_map<std::uint32_t, std::uint16_t> nonDefaultMappings;
				};
			}
#endif // ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND

			/// Returns @c true if complex scripts are supported.
			bool supportsComplexScripts() BOOST_NOEXCEPT;
			/// Returns @c true if OpenType features are supported.
			bool supportsOpenTypeFeatures() BOOST_NOEXCEPT;

			/**
			 * Encapsulatates the measurement information associated with a text run.
			 * @see FontMetrics, GlyphMetrics, Font#lineMetrics
			 */
			class LineMetrics {
			public:
				/// Destructor.
				virtual ~LineMetrics() BOOST_NOEXCEPT {}
				/// Returns the ascent of the text in user units.
				virtual Scalar ascent() const BOOST_NOEXCEPT = 0;
				/// Returns the dominant baseline of the text.
				virtual DominantBaseline baseline() const BOOST_NOEXCEPT = 0;
				/// Returns the baseline offset of the text, relative to the baseline of the text in user units.
				virtual Scalar baselineOffset(AlignmentBaseline baseline) const BOOST_NOEXCEPT = 0;
				/// Returns the descent of the text in user units.
				virtual Scalar descent() const BOOST_NOEXCEPT = 0;
				/// Returns the height of the text in user units.
				Scalar height() const BOOST_NOEXCEPT {return ascent() + descent() + leading();}
				/// Returns the leading of the text in user units.
				virtual Scalar leading() const BOOST_NOEXCEPT = 0;
				/// Returns the position of the strike-through line relative to the baseline in user units.
				virtual Scalar strikeThroughOffset() const BOOST_NOEXCEPT = 0;
				/// Returns the thickness of the strike-through line in user units.
				virtual Scalar strikeThroughThickness() const BOOST_NOEXCEPT = 0;
				/// Returns the position of the underline relative to the baseline in user units.
				virtual Scalar strikeUnderlineOffset() const BOOST_NOEXCEPT = 0;
				/// Returns the thickness of the underline in user units.
				virtual Scalar strikeUnderlineThickness() const BOOST_NOEXCEPT = 0;
			};

			/**
			 * Represents a single physical instance of a font, or a set of fonts.
			 * @see FontFamily, FontDescription, Fontset, FontFace, FontCollection
			 */
			class Font : public std::enable_shared_from_this<Font> {
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
				/**
				 * Creates a @c GlyphVector by mapping characters to glyphs one-to-one based on the
				 * Unicode cmap in this font. This method does no other processing besides the
				 * mapping of glyphs to characters. This means that this method is not useful for
				 * some scripts, such as Arabic, Hebrew, Thai, and Indic, that require reordering,
				 * shaping, or ligature substitution.
				 * @param frc The font render context
				 * @param text The text string
				 * @return A new @c GlyphVector created with the specified string and the specified
				 *         @c FontRenderContext
				 */
				std::unique_ptr<const GlyphVector> createGlyphVector(
					const FontRenderContext& frc, const StringPiece& text) const;
				/**
				 * Creates a @c GlyphVector by mapping characters to glyphs one-to-one based on the
				 * Unicode cmap in this font. This method does no other processing besides the
				 * mapping of glyphs to characters. This means that this method is not useful for
				 * some scripts, such as Arabic, Hebrew, Thai, and Indic, that require reordering,
				 * shaping, or ligature substitution.
				 * @param frc The font render context
				 * @param text The text string
				 * @return A new @c GlyphVector created with the specified string and the specified
				 *         @c FontRenderContext
				 */
				std::unique_ptr<const GlyphVector> createGlyphVector(
					const FontRenderContext& frc, const std::vector<GlyphCode>& glyphCodes) const;
				/// Returns the description of this font.
				const FontDescription& describe() const BOOST_NOEXCEPT {
					if(description_.get() == nullptr)
						const_cast<Font*>(this)->buildDescription();
					return *description_;
				}
				/// Returns the family name of this font.
				FontFamily&& family() const;
#ifdef ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
				boost::optional<GlyphCode> ivsGlyph(CodePoint baseCharacter,
					CodePoint variationSelector, GlyphCode defaultGlyph) const;
#endif //ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
				/**
				 * Returns a new @c GlyphVector object, performing full layout of the text if
				 * possible. Full layout is required for complex text, such as Arabic or Hindi.
				 * Support for different scripts depends on the font and implementation.
				 * @param frc The font render context
				 * @param text The text to layout
				 * @param flags Control flags
				 * @return A new @c GlyphVector representing the text, with glyphs chosen and
				 *         positioned so as to best represent the text
				 */
				std::unique_ptr<const GlyphVector> layoutGlyphVector(
					const FontRenderContext& frc, const StringPiece& text) const;
				std::unique_ptr<const LineMetrics> lineMetrics(
					const StringPiece& text, const FontRenderContext& frc) const;
				AffineTransform&& transform() const BOOST_NOEXCEPT;
			private:
				void buildDescription() BOOST_NOEXCEPT;
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
				std::unique_ptr<const FontDescription> description_;
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
				 * @param transform The transform associated with the font
				 * @param sizeAdjust The 'font-size-adjust' value. Set @c boost#none for 'none'
				 * @return The font has the requested description or the default one
				 */
				std::shared_ptr<const Font> get(
					const FontDescription& description,
					const AffineTransform& transform = AffineTransform(),
					boost::optional<double> sizeAdjust = boost::none) const;
				/**
				 * Returns the fontset for last resort fallback.
				 * @param description The font description
				 * @param transform The transform associated with the font
				 * @param sizeAdjust The 'font-size-adjust' value. Set @c boost#none for 'none'
				 * @return The font has the requested property
				 */
				std::shared_ptr<const Font> lastResortFallback(
					const FontDescription& description,
					const AffineTransform& transform = AffineTransform(),
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
		}
	}
}

#endif // !ASCENSION_FONT_HPP
