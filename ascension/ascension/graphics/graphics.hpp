/**
 * @file graphics.hpp
 * @author exeal
 * @date 2010
 */

#ifndef ASCENSION_GRAPHICS_HPP
#define ASCENSION_GRAPHICS_HPP
#include "graphics-datatypes.hpp"

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

		class Context {
		public:
			enum BackgroundMode {
				TRANSPARENT_MODE = 0,
				OPAQUE_MODE = 1
			};
		public:
			/// Destructor.
			virtual ~Context() /*throw()*/ {}
#ifdef ASCENSION_WINDOWS
			virtual const win32::Handle<HDC>& nativeHandle() const = 0;
#endif // ASCENSION_WINDOWS
		public:
			// attributes
			BackgroundMode backgroundMode() const;
			Context& setBackgroundMode(BackgroundMode mode);
			Context& setFont(const Font& font);
			// metrics
			virtual uint logicalDpiX() const = 0;
			virtual uint logicalDpiY() const = 0;
			virtual Dimension<uint> size() const = 0;
			// drawings
			void drawGlyphs(const Point<>& position, const uint16_t glyphs[], std::size_t numberOfGlyphs, const Color& color);
			void drawText(const Point<>& position, const StringPiece& text, const Color& color);
			virtual void fillRectangle(const Rect<>& rectangle, const Color& color) = 0;
			void restore();
			void save();
		};

		class PaintContext : public Context {
		public:
			/// Destructor.
			virtual ~PaintContext() /*throw()*/ {}
			/// Returns a rectangle in which the painting is requested.
			virtual Rect<> boundsToPaint() const = 0;
		};

		class Device {
		public:
			/// Destructor.
			virtual ~Device() /*throw()*/ {}
			virtual std::auto_ptr<Context> createGraphicContext() const = 0;
		};

		class Screen : public Device {
		public:
			static Screen& instance();
			std::auto_ptr<Context> createGraphicContext() const;
		};

	}
}

#endif // !ASCENSION_GRAPHICS_HPP
