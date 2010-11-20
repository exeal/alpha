/**
 * @file font.hpp
 * @author exeal
 * @date 2010-11-06 created
 */

#ifndef ASCENSION_FONT_HPP
#define ASCENSION_FONT_HPP

#include <ascension/config.hpp>	// ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
#include <ascension/platforms.hpp>
#include <ascension/corelib/basic-types.hpp>	// uint32_t
#include <ascension/graphics/geometry.hpp>
#include <cstring>								// std.strlen
#ifdef ASCENSION_WINDOWS
#	include <ascension/win32/windows.hpp>		// win32.Handle
#endif // ASCENSION_WINDOWS

namespace ascension {
	namespace graphics {
		namespace font {

			/**
			 * Returns an 32-bit integer represents the given TrueType tag.
			 * @param name The TrueType tag name
			 * @return The 32-bit integral TrueType tag value
			 * @throw std#length_error The length of @a name is greater four
			 */
			inline uint32_t makeTrueTypeTag(const char name[]) {
				const size_t len = std::strlen(name);
				if(len == 0 || len > 4)
					throw std::length_error("name");
				uint32_t tag = name[0];
				if(len > 1)
					tag |= name[1] << 8;
				if(len > 2)
					tag |= name[2] << 16;
				if(len > 3)
					tag |= name[3] << 24;
				return tag;
			}

			template<typename T> inline int round(T value) {
				return static_cast<int>(std::floor(value + 0.5));
			}

			typedef uint16_t GlyphCode;

			/**
			 * Represents information for a single glyph.
			 * @see GlyphVector#metrics
			 */
			class GlyphMetrics {
			public:
				virtual Scalar advanceX() const = 0;
				virtual Scalar advanceY() const = 0;
				virtual Dimension<> bounds() const = 0;
				virtual Scalar leftTopSideBearing() const = 0;
				virtual Scalar rightBottomSideBearing() const = 0;
			};

			class GlyphVector {
			public:
				/// Destructor.
				virtual ~GlyphVector() /*throw()*/ {}
				GlyphCode operator[](std::ptrdiff_t index) const {}
				virtual GlyphCode at(std::size_t index) const = 0;
				std::size_t length() const {return size();}
				virtual Dimension<> logicalBounds() const = 0;
				virtual Dimension<> logicalGlyphBounds(std::size_t index) const = 0;
				virtual std::tr1::shared_ptr<GlyphMetrics> metrics(std::size_t index) const = 0;
//				virtual std::tr1::shared_ptr<Shape> outline(std::size_t index) const = 0;
				virtual Point<> position(std::size_t index) const = 0;
				/// Returns
				virtual std::size_t size() const = 0;
				virtual Dimension<> visualGlyphBounds(std::size_t index) const = 0;
				virtual Dimension<> visualBounds() const = 0;
			};

			class FontProperties {
			public:
				enum Weight {
					NORMAL_WEIGHT = 400,
					BOLD = 700,
					BOLDER,
					LIGHTER,
					THIN = 100,
					EXTRA_LIGHT = 200,
					ULTRA_LIGHT = 200,
					LIGHT = 300, 
					MEDIUM = 500,
					SEMI_BOLD = 600,
					DEMI_BOLD = 600,
					EXTRA_BOLD = 800,
					ULTRA_BOLD = 800,
					BLACK = 900,
					HEAVY = 900,
					INHERIT_WEIGHT
				};
				enum Stretch {
					NORMAL_STRETCH,
					WIDER,
					NARROWER,
					ULTRA_CONDENSED,
					EXTRA_CONDENSED,
					CONDENSED,
					SEMI_CONDENSED,
					SEMI_EXPANDED,
					EXPANDED,
					EXTRA_EXPANDED,
					ULTRA_EXPANDED,
					INHERIT_STRETCH
				};
				enum Style {
					NORMAL_STYLE,
					ITALIC,
					OBLIQUE,
					BACKSLANT,
					INHERIT_STYLE
				};
			public:
				/**
				 * Constructor.
				 * @param weight
				 * @param stretch
				 * @param style
				 * @param size
				 */
				explicit FontProperties(Weight weight = INHERIT_WEIGHT,
					Stretch stretch = INHERIT_STRETCH, Style style = INHERIT_STYLE, double size = 0.0)
					: weight_(weight), stretch_(stretch), style_(style), size_(size) {}
				/// Equality operator.
				bool operator==(const FontProperties& other) const /*throw()*/ {
					return weight_ == other.weight_ && stretch_ == other.stretch_
						&& style_ == other.style_ && equals(size_, other.size_);
				}
				/// Inequality operator.
				bool operator!=(const FontProperties& other) const /*throw()*/ {
					return !(*this == other);
				}
				/// Returns the size in pixels. Zero means that inherit the parent.
				double size() const /*throw()*/ {return size_;}
				/// Returns the stretch.
				Stretch stretch() const /*throw()*/ {return stretch_;}
				/// Returns the style.
				Style style() const /*throw()*/ {return style_;}
				/// Returns the weight.
				Weight weight() const /*throw()*/ {return weight_;}
			private:
				const Weight weight_;
				const Stretch stretch_;
				const Style style_;
				const double size_;
			};

			class Font {
			public:
				/// Provides physical font metrics information.
				class Metrics {
				public:
					/// Destructor.
					virtual ~Metrics() /*throw()*/ {}
					/// Returns the ascent of the text in pixels.
					virtual int ascent() const /*throw()*/ = 0;
					/// Returns the average width of a character in pixels.
					virtual int averageCharacterWidth() const /*throw()*/ = 0;
					/// Returns the cell height in pixels.
					int cellHeight() const /*throw()*/ {return ascent() + descent();}
					/// Returns the descent of the text in pixels.
					virtual int descent() const /*throw()*/ = 0;
					/// Returns the em height.
					int emHeight() const /*throw()*/ {return cellHeight() - internalLeading();}
					/// Returns the external leading in pixels.
					/// @note In Ascension, external leadings are placed below characters.
					virtual int externalLeading() const /*throw()*/ = 0;
					/// Returns the font family name.
					virtual String familyName() const /*throw()*/ = 0;
					/// Returns the internal leading in pixels.
					virtual int internalLeading() const /*throw()*/ = 0;
					/// Returns the gap of the lines (external leading) in pixels.
					int lineGap() const /*throw()*/ {return externalLeading();}
					/// Returns the pitch of lines in pixels.
					/// @note This method ignores @c LayoutSettings#lineSpacing value.
					int linePitch() const /*throw()*/ {return cellHeight() + lineGap();}
					/// Returns the x-height of the font in pixels.
					virtual int xHeight() const /*throw()*/ = 0;
				};
			public:
				/// Destructor.
				virtual ~Font() /*throw()*/ {}
#ifdef ASCENSION_WINDOWS
				/// Returns the Win32 @c HFONT handle object.
				virtual const win32::Handle<HFONT>& nativeHandle() const /*throw()*/ = 0;
#endif // ASCENSION_WINDOWS
#ifdef ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
				virtual bool ivsGlyph(CodePoint baseCharacter, CodePoint variationSelector, uint16_t& glyph) const = 0;
#endif //ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
				/// Returns the metrics of the font.
				virtual const Metrics& metrics() const /*throw()*/ = 0;
			};

			/// An interface represents an object provides a set of fonts.
			class FontCollection {
			public:
				/// Destructor.
				virtual ~FontCollection() /*throw()*/ {}
				/**
				 * Returns the font matches the given properties.
				 * @param familyName The font family name
				 * @param properties The font properties
				 * @param sizeAdjust 
				 * @return The font has the requested properties or the default one
				 */
				virtual std::tr1::shared_ptr<const Font> get(const String& familyName,
					const FontProperties& properties, double sizeAdjust = 0.0) const = 0;
			};

			const FontCollection& systemFonts() /*throw()*/;

		}
	}
}

#endif // !ASCENSION_FONT_HPP
