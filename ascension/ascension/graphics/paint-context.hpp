/**
 * @file paint-context.hpp
 * @author exeal
 * @date 2011-03-06 created
 * @date 2017-01-15 Separated from rendering-context.hpp.
 */

#ifndef ASCENSION_PAINT_CONTEXT_HPP
#define ASCENSION_PAINT_CONTEXT_HPP
#include <ascension/graphics/rendering-context.hpp>

namespace ascension {
	namespace graphics {
		class PaintContext : public RenderingContext2D {
		public:
			/// @name Platform-native Interfaces
			/// @{
#if ASCENSION_SELECTS_GRAPHICS_SYSTEM(CAIRO)
			/**
			 * Creates a @c PaintContext object from a @c Cairo#Context.
			 * @param context The native rendering context
			 * @param boundsToPaint The rectangle in which the painting is requested
			 */
			PaintContext(Cairo::RefPtr<Cairo::Context> nativeObject, const Rectangle& boundsToPaint) : RenderingContext2D(nativeObject), boundsToPaint_(boundsToPaint) {}
#elif ASCENSION_SELECTS_GRAPHICS_SYSTEM(CORE_GRAPHICS)
			/**
			 * Creates a @c PaintContext object from a @c CGContext.
			 * @param context The native rendering context
			 * @param boundsToPaint The rectangle in which the painting is requested
			 */
			PaintContext(CGContextRef nativeObject, const Rectangle& boundsToPaint) : RenderingContext2D(nativeObject), boundsToPaint_(boundsToPaint) {}
#elif ASCENSION_SELECTS_GRAPHICS_SYSTEM(DIRECT2D)
			/**
			 * Creates a @c PaintContext object from an @c ID2D1RenderTarget.
			 * @param context The native rendering context
			 * @param boundsToPaint The rectangle in which the painting is requested
			 */
			PaintContext(win32::com::SmartPointer<ID2D1RenderTarget> nativeObject, const Rectangle& boundsToPaint) : RenderingContext2D(nativeObject), boundsToPaint_(boundsToPaint) {}
#elif ASCENSION_SELECTS_GRAPHICS_SYSTEM(QT)
			/**
			 * Creates a @c PaintContext object from a @c QPainter as weak reference.
			 * @param context The native rendering context
			 * @param boundsToPaint The rectangle in which the painting is requested
			 */
			PaintContext(QPainter& nativeObject, const Rectangle& boundsToPaint) : RenderingContext2D(nativeObject), boundsToPaint_(boundsToPaint) {}
			/**
			 * Creates a @c PaintContext object from a @c QPainter.
			 * @param context The native rendering context
			 * @param boundsToPaint The rectangle in which the painting is requested
			 */
			PaintContext(std::shared_ptr<QPainter> nativeObject, const Rectangle& boundsToPaint) : RenderingContext2D(nativeObject), boundsToPaint_(boundsToPaint) {}
#elif ASCENSION_SELECTS_GRAPHICS_SYSTEM(WIN32_GDI)
			/**
			 * Creates a @c PaintContext object from an @c HDC.
			 * @param context The native rendering context
			 * @param boundsToPaint The rectangle in which the painting is requested
			 */
			PaintContext(win32::Handle<HDC> nativeObject, const Rectangle& boundsToPaint) : RenderingContext2D(nativeObject), boundsToPaint_(boundsToPaint) {}
#elif ASCENSION_SELECTS_GRAPHICS_SYSTEM(WIN32_GDIPLUS)
			/**
			 * Creates a @c PaintContext object from a @c Gdiplus#Graphics as weak reference.
			 * @param context The native rendering context
			 * @param boundsToPaint The rectangle in which the painting is requested
			 */
			PaintContext(Gdiplus::Graphics& nativeObject, const Rectangle& boundsToPaint) : RenderingContext2D(nativeObject), boundsToPaint_(boundsToPaint) {}
			/**
			 * Creates a @c PaintContext object from a @c Gdiplus#Graphics.
			 * @param context The native rendering context
			 * @param boundsToPaint The rectangle in which the painting is requested
			 */
			PaintContext(std::shared_ptr<Gdiplus::Graphics> nativeObject, const Rectangle& boundsToPaint) : RenderingContext2D(nativeObject), boundsToPaint_(boundsToPaint) {}
#endif
			/// @}

			/// Returns a rectangle in which the painting is requested.
			const Rectangle& boundsToPaint() const BOOST_NOEXCEPT {return boundsToPaint_;}

		private:
			const Rectangle boundsToPaint_;
		};
	}
}

#endif // !ASCENSION_PAINT_CONTEXT_HPP
