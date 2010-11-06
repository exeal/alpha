/**
 * @file font.hpp
 * @author exeal
 * @date 2010-11-06 created
 */

#ifndef ASCENSION_FONT_HPP
#define ASCENSION_FONT_HPP

#include <ascension/config.hpp>	// ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
#include <ascension/platforms.hpp>

namespace ascension {
	namespace graphics {

		/// Returns an 32-bit integer represents the given TrueType tag.
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

		struct FontProperties {
			enum Weight {
				NORMAL_WEIGHT = 400, BOLD = 700, BOLDER, LIGHTER,
				THIN = 100, EXTRA_LIGHT = 200, ULTRA_LIGHT = 200, LIGHT = 300, 
				MEDIUM = 500, SEMI_BOLD = 600, DEMI_BOLD = 600,
				EXTRA_BOLD = 800, ULTRA_BOLD = 800, BLACK = 900, HEAVY = 900, INHERIT_WEIGHT
			} weight;
			enum Stretch {
				NORMAL_STRETCH, WIDER, NARROWER, ULTRA_CONDENSED, EXTRA_CONDENSED, CONDENSED, SEMI_CONDENSED,
				SEMI_EXPANDED, EXPANDED, EXTRA_EXPANDED, ULTRA_EXPANDED, INHERIT_STRETCH
			} stretch;
			enum Style {
				NORMAL_STYLE, ITALIC, OBLIQUE, BACKSLANT, INHERIT_STYLE
			} style;
			double size;	///< Font size (em height) in pixels. Zero means inherit the parent.

			/// Constructor.
			explicit FontProperties(Weight weight = INHERIT_WEIGHT,
				Stretch stretch = INHERIT_STRETCH, Style style = INHERIT_STYLE, double size = 0, double sizeAdjust = 0.0)
				: weight(weight), stretch(stretch), style(style), size(size) {}
			/// Equality operator.
			bool operator==(const FontProperties& other) const /*throw()*/ {
				return weight == other.weight && stretch == other.stretch && style == other.style && equals(size, other.size);}
			/// Inequality operator.
			bool operator!=(const FontProperties& other) const /*throw()*/ {return !(*this == other);}
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
			virtual win32::Handle<HFONT> nativeHandle() const /*throw()*/ = 0;
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
			 * @param familyName the font family name
			 * @param properties the font properties
			 * @param sizeAdjust 
			 * @return the font has the requested properties or the default one
			 */
			virtual std::tr1::shared_ptr<const Font> get(const String& familyName,
				const FontProperties& properties, double sizeAdjust = 0.0) const = 0;
		};

		const FontCollection& systemFonts() /*throw()*/;

	}
}

#endif // !ASCENSION_FONT_HPP
