/**
 * @file font.hpp
 * @author exeal
 * @date 2010-11-06 created
 * @date 2010-2014
 */

#ifndef ASCENSION_FONT_HPP
#define ASCENSION_FONT_HPP
#include <ascension/config.hpp>
#include <ascension/corelib/native-wrappers.hpp>
#include <ascension/corelib/string-piece.hpp>
#include <ascension/corelib/text/code-point.hpp>
#include <ascension/graphics/font/glyph-code.hpp>
#include <ascension/graphics/font/text-alignment.hpp>
#include <ascension/graphics/geometry/affine-transform.hpp>
#include <locale>
#include <memory>
#ifdef ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
#	include <boost/optional.hpp>
#	include <unordered_map>
#endif // ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
#include <vector>
#if ASCENSION_SELECTS_SHAPING_ENGINE(UNISCRIBE) || ASCENSION_SELECTS_SHAPING_ENGINE(WIN32_GDI)
#	include <ascension/win32/handle.hpp>
#endif

namespace ascension {
	namespace graphics {
		namespace font {
			class FontDescription;
			class FontFamily;
			class FontRenderContext;
			class GlyphVector;

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
				virtual Scalar underlineOffset() const BOOST_NOEXCEPT = 0;
				/// Returns the thickness of the underline in user units.
				virtual Scalar underlineThickness() const BOOST_NOEXCEPT = 0;
			};

			/**
			 * Represents a single physical instance of a font, or a set of fonts.
			 * @see FontFamily, FontDescription, Fontset, FontFace, FontCollection
			 */
			class Font : public SharedWrapper<Font>, public std::enable_shared_from_this<Font> {
			public:
#if ASCENSION_SELECTS_SHAPING_ENGINE(CAIRO)
				explicit Font(Cairo::RefPtr<Cairo::ScaledFont> nativeObject);
				Glib::RefPtr<Cairo::ScaledFont> native();
				Glib::RefPtr<const Cairo::ScaledFont> native() const;
#elif ASCENSION_SELECTS_SHAPING_ENGINE(CORE_GRAPHICS)
				explicit Font(cg::Reference<CGFontRef>&& nativeObject);
				cg::Reference<CGFontRef>& native() const;
#elif ASCENSION_SELECTS_SHAPING_ENGINE(CORE_TEXT)
				explicit Font(cg::Reference<CTFontRef>&& nativeObject);
				cg::Reference<CTFontRef>& native() const;
#elif ASCENSION_SELECTS_SHAPING_ENGINE(DIRECT_WRITE)
				explicit Font(win32::com::SmartPointer<IDWriteFont> nativeObject);
				win32::com::SmartPointer<IDWriteFont> native() const;
#elif ASCENSION_SELECTS_SHAPING_ENGINE(HARFBUZZ)
				explicit Font(boost::intrusive_ptr<hb_font_t> nativeObject);
				boost::intrusive_ptr<hb_font_t> native() const;
#elif ASCENSION_SELECTS_SHAPING_ENGINE(PANGO)
//				explicit Font(Glib::RefPtr<Pango::Font> nativeObject);
//				Glib::RefPtr<Pango::Font> native();
//				Glib::RefPtr<const Pango::Font> native() const;
				explicit Font(Glib::RefPtr<Pango::Fontset> nativeObject);
				Glib::RefPtr<Pango::Fontset> native();
				Glib::RefPtr<const Pango::Fontset> native() const;
#elif ASCENSION_SELECTS_SHAPING_ENGINE(QT)
//				explicit Font(std::shared_ptr<QRawFont> nativeObject);
//				std::shared_ptr<QRawFont> native();
//				std::shared_ptr<const QRawFont> native() const;
				explicit Font(std::shared_ptr<QFont> nativeObject);
				std::shared_ptr<QFont> native();
				std::shared_ptr<const QFont> native() const;
#elif ASCENSION_SELECTS_SHAPING_ENGINE(UNISCRIBE) || ASCENSION_SELECTS_SHAPING_ENGINE(WIN32_GDI)
				explicit Font(win32::Handle<HFONT> nativeObject) BOOST_NOEXCEPT;
				win32::Handle<HFONT> native() const BOOST_NOEXCEPT;
#elif ASCENSION_SELECTS_SHAPING_ENGINE(WIN32_GDIPLUS)
				explicit Font(std::shared_ptr<Gdiplus::Font> nativeObject);
				std::shared_ptr<Gdiplus::Font> native();
				std::shared_ptr<const Gdiplus::Font> native() const;
#endif
				/**
				 * Creates a @c GlyphVector by mapping characters to glyphs one-to-one based on the Unicode cmap in
				 * this font. This method does no other processing besides the mapping of glyphs to characters. This
				 * means that this method is not useful for some scripts, such as Arabic, Hebrew, Thai, and Indic, that
				 * require reordering, shaping, or ligature substitution.
				 * @param frc The font render context
				 * @param text The text string
				 * @return A new @c GlyphVector created with the specified string and the specified
				 *         @c FontRenderContext
				 */
				std::unique_ptr<const GlyphVector> createGlyphVector(
					const FontRenderContext& frc, const StringPiece& text) const;
				/**
				 * Creates a @c GlyphVector by mapping characters to glyphs one-to-one based on the Unicode cmap in
				 * this font. This method does no other processing besides the mapping of glyphs to characters. This
				 * means that this method is not useful for some scripts, such as Arabic, Hebrew, Thai, and Indic, that
				 * require reordering, shaping, or ligature substitution.
				 * @param frc The font render context
				 * @param glyphCodes The vector of glyph codes
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
				 * Returns a new @c GlyphVector object, performing full layout of the text if possible. Full layout is
				 * required for complex text, such as Arabic or Hindi. Support for different scripts depends on the
				 * font and implementation.
				 * @param frc The font render context
				 * @param text The text to layout
				 * @return A new @c GlyphVector representing the text, with glyphs chosen and positioned so as to best
				 *         represent the text
				 */
				std::unique_ptr<const GlyphVector> layoutGlyphVector(
					const FontRenderContext& frc, const StringPiece& text) const;
				std::unique_ptr<const LineMetrics> lineMetrics(
					const StringPiece& text, const FontRenderContext& frc) const;
				AffineTransform&& transform() const BOOST_NOEXCEPT;
			private:
				void buildDescription();
#if ASCENSION_SELECTS_SHAPING_ENGINE(CAIRO)
				Cairo::RefPtr<Cairo::ScaledFont> nativeObject_;
#elif ASCENSION_SELECTS_SHAPING_ENGINE(CORE_GRAPHICS)
				cg::Reference<CGFontRef> nativeObject_;
#elif ASCENSION_SELECTS_SHAPING_ENGINE(CORE_TEXT)
				cg::Reference<CTFontRef> nativeObject_;
#elif ASCENSION_SELECTS_SHAPING_ENGINE(DIRECT_WRITE)
				win32::com::SmartPointer<IDWriteFont> nativeObject_;
#elif ASCENSION_SELECTS_SHAPING_ENGINE(HARFBUZZ)
				boost::intrusive_ptr<hb_font_t> nativeObject_;
#elif ASCENSION_SELECTS_SHAPING_ENGINE(PANGO)
//				Glib::RefPtr<Pango::Font> nativeObject_;
				Glib::RefPtr<Pango::Fontset> nativeObject_;
#elif ASCENSION_SELECTS_SHAPING_ENGINE(QT)
//				std::shared_ptr<QRawFont> nativeObject_;
				std::shared_ptr<QFont> nativeObject_;
#elif ASCENSION_SELECTS_SHAPING_ENGINE(UNISCRIBE) || ASCENSION_SELECTS_SHAPING_ENGINE(WIN32_GDI)
				win32::Handle<HFONT> nativeObject_;
#ifdef ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
				std::shared_ptr<detail::IdeographicVariationSequences> ivs_;
#endif //ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
#elif ASCENSION_SELECTS_SHAPING_ENGINE(WIN32_GDIPLUS)
				std::shared_ptr<Gdiplus::Font> nativeObject_;
#endif
				std::shared_ptr<const FontDescription> description_;
			};
		}
	}
}

#endif // !ASCENSION_FONT_HPP
