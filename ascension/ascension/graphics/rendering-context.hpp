/**
 * @file rendering-context.hpp
 * @author exeal
 * @date 2011-03-06 created
 */

#ifndef ASCENSION_RENDERING_CONTEXT_HPP
#define ASCENSION_RENDERING_CONTEXT_HPP

#include <ascension/corelib/basic-types.hpp>	// std.tr1.shared_ptr
#include <ascension/corelib/string-piece.hpp>
#include <ascension/graphics/affine-transform.hpp>
#include <ascension/graphics/color.hpp>
#include <ascension/graphics/font.hpp>
#include <ascension/graphics/geometry.hpp>
#include <ascension/graphics/paint.hpp>
#include <ascension/graphics/rendering-device.hpp>
#include <ascension/presentation/text-style.hpp>	// presentation.AlignmentBaseline, presentation.TextAnchor
#include <stack>
#include <vector>

namespace ascension {
	namespace graphics {

		/**
		 * @c CanvasRenderingContext2D interface defined in "HTML Canvas 2D Context"
		 * (http://dev.w3.org/html5/2dcontext/).
		 */
		namespace renderingapi {
#if defined(ASCENSION_GRAPHICS_SYSTEM_CAIRO)
			typedef Cairo::RefPtr<Cairo::Context> RenderingContext2D;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_CORE_GRAPHICS)
			typedef CGContextRef RenderingContext2D;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_DIRECT2D)
			typedef win32::com::SmartPointer<ID2D1RenderTarget> RenderingContext2D;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_QT)
			typedef QPainter RenderingContext2D;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI)
			typedef win32::Handle<HDC> RenderingContext2D;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDIPLUS)
			typedef Gdiplus::Graphics RenderingContext2D;
#endif

			enum CompositeOperation {
				SOURCE_ATOP, SOURCE_IN, SOURCE_OUT, SOURCE_OVER,
				DESTINATION_ATOP, DESTINATION_IN, DESTINATION_OUT, DESTINATION_OVER,
				LIGHTER, COPY, XOR
			};

			class ImageData : public NativeSize {
			};

			// back-reference to the canvas

			// state
			RenderingContext2D& save(RenderingContext2D& context);
			RenderingContext2D& restore(RenderingContext2D& context);

			// compositing
			double globalAlpha(const RenderingContext2D& context);
			RenderingContext2D& setGlobalAlpha(RenderingContext2D& context, double globalAlpha);
			CompositeOperation globalCompositeOperation(const RenderingContext2D& context);
			RenderingContext2D& setGlobalCompositeOperation(RenderingContext2D& context, CompositeOperation compositeOperation);

			// colors and styles
			Paint& strokeStyle(const RenderingContext2D& context);
			RenderingContext2D& setStrokeStyle(RenderingContext2D& context, const Paint& strokeStyle);
			Paint& fillStyle(const RenderingContext2D& context);
			RenderingContext2D& setFillStyle(RenderingContext2D& context, const Paint& fillStyle);
			std::unique_ptr<Gradient> createLinearGradient();
			std::unique_ptr<Gradient> createRadialGradient();
			std::unique_ptr<Pattern> createPattern();

			// shadows
			NativeSize shadowOffset(const RenderingContext2D& context);
			RenderingContext2D& setShadowOffset(RenderingContext2D& context, const NativeSize& shadowOffset);
			Scalar shadowBlur(const RenderingContext2D& context);
			RenderingContext2D& setShadowBlur(RenderingContext2D& context, Scalar shadowBlur);
			Color shadowColor(const RenderingContext2D& context);
			RenderingContext2D& setShadowColor(RenderingContext2D& context, const Color& shadowColor);

			// rects
			RenderingContext2D& clearRectangle(RenderingContext2D& context, const NativeRectangle& rectangle);
			RenderingContext2D& fillRectangle(RenderingContext2D& context, const NativeRectangle& rectangle);
			RenderingContext2D& strokeRectangle(RenderingContext2D& context, const NativeRectangle& rectangle);

			// current default path API
			RenderingContext2D& beginPath(RenderingContext2D& context);
			RenderingContext2D& fill(RenderingContext2D& context);
			RenderingContext2D& stroke(RenderingContext2D& context);
			void drawSystemFocusRing(RenderingContext2D& context);
			bool drawCustomFocusRing(RenderingContext2D& context);
			RenderingContext2D& scrollPathIntoView(RenderingContext2D& context);
			RenderingContext2D& clip(RenderingContext2D& context);
			bool isPointInPath(RenderingContext2D& context, const NativePoint& point);

			// text
			RenderingContext2D& fillText(RenderingContext2D& context, const StringPiece& text,
				const NativePoint& origin, boost::optional<Scalar> maximumMeasure = boost::none);
			RenderingContext2D& strokeText(RenderingContext2D& context, const StringPiece& text,
				const NativePoint& origin, boost::optional<Scalar> maximumMeasure = boost::none);
			NativeSize measureText(RenderingContext2D& context, const StringPiece& text);

			// drawing images
			RenderingContext2D& drawImage(RenderingContext2D& context);

			// pixel manipulation
			std::unique_ptr<ImageData> createImageData(const RenderingContext2D& context, const NativeSize& size);
			std::unique_ptr<ImageData> createImageData(const RenderingContext2D& context, const ImageData& image);
			std::unique_ptr<ImageData> getImageData(const RenderingContext2D& context, const NativeRectangle& rectangle);
			void putImageData(RenderingContext2D& context, const NativeSize& size);
			void putImageData(RenderingContext2D& context, const NativeSize& size, const NativeRectangle& dirtyRectangle);

			// transformations (CanvasTransformation interface)
			RenderingContext2D& scale(RenderingContext2D& context, const NativeSize& s);
			RenderingContext2D& rotate(RenderingContext2D& context, double angle);
			RenderingContext2D& translate(RenderingContext2D& context, const NativeSize& delta);
			RenderingContext2D& transform(RenderingContext2D& context, const NativeAffineTransform& matrix);
			RenderingContext2D& setTransform(RenderingContext2D& context, const NativeAffineTransform& matrix);

			// line caps/joins (CanvasLineStyles interface)
			Scalar lineWidth(const RenderingContext2D& context);
			RenderingContext2D& setLineWidth(RenderingContext2D& context, Scalar lineWidth);
			LineCap lineCap(const RenderingContext2D& context);
			RenderingContext2D& setLineCap(RenderingContext2D& context, LineCap lineCap);
			LineJoin lineJoin(const RenderingContext2D& context);
			RenderingContext2D& setLineJoin(RenderingContext2D& context, LineJoin lineJoin);
			Scalar miterLimit(const RenderingContext2D& context);
			RenderingContext2D& setMiterLimit(RenderingContext2D& context, Scalar miterLimit);

			// text (CanvasText interface)
			std::shared_ptr<const font::Font> font(const RenderingContext2D& context);
			RenderingContext2D& setFont(RenderingContext2D& context, std::shared_ptr<const font::Font> font);
			presentation::TextAnchor textAlign(const RenderingContext2D& context);
			RenderingContext2D& setTextAlign(RenderingContext2D& context, presentation::TextAnchor anchor);
			presentation::DominantBaseline textBaseline(const RenderingContext2D& context);
			RenderingContext2D& setBaseline(RenderingContext2D& context, presentation::DominantBaseline baseline);

			// shared path API methods (CanvasPathMethods interface)
			RenderingContext2D& closePath(RenderingContext2D& context);
			RenderingContext2D& moveTo(RenderingContext2D& context, const NativePoint& to);
			RenderingContext2D& lineTo(RenderingContext2D& context, const NativePoint& to);
			RenderingContext2D& quadraticCurveTo(RenderingContext2D& context, double cpx, double cpy, double x, double y);
			RenderingContext2D& bezierCurveTo(RenderingContext2D& context, double cp1x, double cp1y, double cp2x, double cp2y, double x, double y);
			RenderingContext2D& arcTo(RenderingContext2D& context, double x1, double y1, double x2, double y2, double radius); 
			RenderingContext2D& rectangle(RenderingContext2D& context, const NativeRectangle& rect);
			RenderingContext2D& arc(RenderingContext2D& context, const NativePoint& to, Scalar radius, double startAngle, double endAngle, bool counterClockwise = false); 
		}

		class PaintContext {
		public:
			/**
			 * Constructor.
			 * @param context The rendering context
			 * @param boundsToPaint The rectangle in which the painting is requested
			 */
			PaintContext(renderingapi::RenderingContext2D& context,
				const NativeRectangle& boundsToPaint) : context_(context), boundsToPaint_(boundsToPaint) {}
			/// Returns the rendering context.
			renderingapi::RenderingContext2D& operator*() /*noexcept*/ {return context_;}
			/// Returns the rendering context.
			const renderingapi::RenderingContext2D& operator*() const /*noexcept*/ {return context_;}
			/// Returns the rendering context.
			renderingapi::RenderingContext2D* operator->() /*noexcept*/ {return &context_;}
			/// Returns the rendering context.
			const renderingapi::RenderingContext2D* operator->() const /*noexcept*/ {return &context_;}
			/// Returns a rectangle in which the painting is requested.
			const NativeRectangle& boundsToPaint() const /*noexcept*/ {return boundsToPaint_;}
		private:
			renderingapi::RenderingContext2D& context_;
			const NativeRectangle boundsToPaint_;
		};

	}
}

#endif //!ASCENSION_RENDERING_CONTEXT_HPP
