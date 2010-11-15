/**
 * @file graphics.hpp
 * @author exeal
 * @date 2010
 */

#ifndef ASCENSION_GRAPHICS_HPP
#define ASCENSION_GRAPHICS_HPP

#include <ascension/graphics/geometry.hpp>

namespace ascension {
	namespace graphics {

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
