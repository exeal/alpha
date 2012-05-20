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
#include <ascension/presentation/text-line-style.hpp>	// presentation.AlignmentBaseline, presentation.TextAnchor
#include <memory>
#include <boost/optional.hpp>
#if defined(ASCENSION_GRAPHICS_SYSTEM_CAIRO)
#	include <cairomm/context.h>
#elif defined(ASCENSION_GRAPHICS_SYSTEM_CORE_GRAPHICS)
#	include <CGContext.h>
#elif defined(ASCENSION_GRAPHICS_SYSTEM_DIRECT2D)
#	include <d2d1.h>
#elif defined(ASCENSION_GRAPHICS_SYSTEM_QT)
#	include <QPainter>
#elif defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI)
#	include <ascension/win32/handle.hpp>
#elif defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDIPLUS)
#	include <GdiPlus.h>
#endif

namespace ascension {
	namespace graphics {
#if defined(ASCENSION_GRAPHICS_SYSTEM_CAIRO)
		typedef Cairo::RefPtr<Cairo::Context> NativeRenderingContext2D;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_CORE_GRAPHICS)
		typedef CGContextRef NativeRenderingContext2D;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_DIRECT2D)
		typedef win32::com::SmartPointer<ID2D1RenderTarget> NativeRenderingContext2D;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_QT)
		typedef QPainter& NativeRenderingContext2D;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI)
		typedef win32::Handle<HDC> NativeRenderingContext2D;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDIPLUS)
		typedef Gdiplus::Graphics& NativeRenderingContext2D;
#endif

		enum CompositeOperation {
			SOURCE_ATOP, SOURCE_IN, SOURCE_OUT, SOURCE_OVER,
			DESTINATION_ATOP, DESTINATION_IN, DESTINATION_OUT, DESTINATION_OVER,
			LIGHTER, COPY, XOR
		};

		enum FillRule {NONZERO, EVENODD};

		enum LineCap {BUTT_LINE_CAP, ROUND_LINE_CAP, SQUARE_LINE_CAP};

		enum LineJoin {BEVEL_LINE_JOIN, ROUND_LINE_JOIN, MITER_LINE_JOIN};

		class ImageData : public NativeSize {
		};

		class Image;
		class Paint;
		class RenderingDevice;

		/**
		 * @c CanvasRenderingContext2D interface defined in "HTML Canvas 2D Context"
		 * (http://dev.w3.org/html5/2dcontext/).
		 */
		class RenderingContext2D {
			ASCENSION_NONCOPYABLE_TAG(RenderingContext2D);
		public:
			// platform-native interfaces
#if defined(ASCENSION_GRAPHICS_SYSTEM_CAIRO)
			explicit RenderingContext2D(Cairo::RefPtr<Cairo::Context> nativeObject);
			Cairo::RefPtr<Cairo::Context> asNativeObject();
			Cairo::RefPtr<const Cairo::Context> asNativeObject() const;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_CORE_GRAPHICS)
			CGContextRef asNativeObject();
			const CGContextRef asNativeObject() const;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_DIRECT2D)
			explicit RenderingContext2D(win32::com::SmartPointer<ID2D1RenderTarget> nativeObject);
			win32::com::SmartPointer<ID2D1RenderTarget> asNativeObject() const;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_QT)
			explicit RenderingContext2D(QPainter& nativeObject);	// weak ref.
			explicit RenderingContext2D(std::unique_ptr<QPainter> nativeObject);
			explicit RenderingContext2D(std::shared_ptr<QPainter> nativeObject);
			QPainter& asNativeObject();
			const QPainter& asNativeObject() const;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI)
			explicit RenderingContext2D(win32::Handle<HDC>&& nativeObject);
			explicit RenderingContext2D(const win32::Handle<HDC>& nativeObject);	// weak ref.
			const win32::Handle<HDC>& asNativeObject() const;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDIPLUS)
			explicit RenderingContext2D(Gdiplus::Graphics& nativeObject);	// weak ref.
			explicit RenderingContext2D(std::unique_ptr<Gdiplus::Graphics> nativeObject);
			explicit RenderingContext2D(std::shared_ptr<Gdiplus::Graphics> nativeObject);
			Gdiplus::Graphics& asNativeObject();
			const Gdiplus::Graphics& asNativeObject() const;
#endif
#ifdef ASCENSION_COMPILER_MSVC
			RenderingContext2D(RenderingContext2D&& other) /*noexcept*/ : nativeObject_(std::move(other.nativeObject_)) {}
			RenderingContext2D& operator=(RenderingContext2D&& other) {
				nativeObject_ = std::move(other.nativeObject_);
				return *this;
			}
#endif // ASCENSION_COMPILER_MSVC
		public:
			// back-reference to the canvas
//			???? canvas() const;
			const RenderingDevice& device() const;
			// state
			RenderingContext2D& save();
			RenderingContext2D& restore();
			// compositing
			double globalAlpha() const;
			RenderingContext2D& setGlobalAlpha(double globalAlpha);
			CompositeOperation globalCompositeOperation() const;
			RenderingContext2D& setGlobalCompositeOperation(CompositeOperation compositeOperation);
			// colors and styles
			Paint& strokeStyle() const;
			RenderingContext2D& setStrokeStyle(const Paint& strokeStyle);
			Paint& fillStyle() const;
			RenderingContext2D& setFillStyle(const Paint& fillStyle);
//			std::unique_ptr<Gradient> createLinearGradient();
//			std::unique_ptr<Gradient> createRadialGradient();
//			std::unique_ptr<Pattern> createPattern();
			// shadows
			NativeSize shadowOffset() const;
			RenderingContext2D& setShadowOffset(NativeRenderingContext2D& context, const NativeSize& shadowOffset);
			Scalar shadowBlur() const;
			RenderingContext2D& setShadowBlur(Scalar shadowBlur);
			Color shadowColor() const;
			RenderingContext2D& setShadowColor(const Color& shadowColor);
			// rects
			RenderingContext2D& clearRectangle(const NativeRectangle& rectangle);
			RenderingContext2D& fillRectangle(const NativeRectangle& rectangle);
			RenderingContext2D& strokeRectangle(const NativeRectangle& rectangle);
			// current default path API
			RenderingContext2D& beginPath();
			RenderingContext2D& fill();
			RenderingContext2D& stroke();
			void drawSystemFocusRing(const NativeRectangle& bounds);
			bool drawCustomFocusRing(const NativeRectangle& bounds);
			RenderingContext2D& scrollPathIntoView();
			RenderingContext2D& clip();
			bool isPointInPath(const NativePoint& point) const;
			// text
			RenderingContext2D& fillText(const StringPiece& text,
				const NativePoint& origin, boost::optional<Scalar> maximumMeasure = boost::none);
			RenderingContext2D& strokeText(const StringPiece& text,
				const NativePoint& origin, boost::optional<Scalar> maximumMeasure = boost::none);
			NativeSize measureText(const StringPiece& text);
			// drawing images
			RenderingContext2D& drawImage(const Image& image, const NativePoint& position);
			RenderingContext2D& drawImage(const Image& image, const NativeRectangle& destinationBounds);
			RenderingContext2D& drawImage(const Image& image, const NativeRectangle& sourceBounds, const NativeRectangle& destinationBounds);
			// pixel manipulation
			std::unique_ptr<ImageData> createImageData(const NativeSize& size) const;
			std::unique_ptr<ImageData> createImageData(const ImageData& image) const;
			std::unique_ptr<ImageData> getImageData(const NativeRectangle& rectangle) const;
			void putImageData(const NativeSize& size);
			void putImageData(const NativeSize& size, const NativeRectangle& dirtyRectangle);
			// transformations (CanvasTransformation interface)
			RenderingContext2D& scale(const NativeSize& s);
			RenderingContext2D& rotate(double angle);
			RenderingContext2D& translate(const NativeSize& delta);
			RenderingContext2D& transform(const NativeAffineTransform& matrix);
			RenderingContext2D& setTransform(const NativeAffineTransform& matrix);
			// line caps/joins (CanvasLineStyles interface)
			Scalar lineWidth() const;
			RenderingContext2D& setLineWidth(NativeRenderingContext2D& context, Scalar lineWidth);
			LineCap lineCap() const;
			RenderingContext2D& setLineCap(NativeRenderingContext2D& context, LineCap lineCap);
			LineJoin lineJoin() const;
			RenderingContext2D& setLineJoin(LineJoin lineJoin);
			Scalar miterLimit() const;
			RenderingContext2D& setMiterLimit(Scalar miterLimit);
			// text (CanvasText interface)
			std::shared_ptr<const font::Font> font() const;
			RenderingContext2D& setFont(std::shared_ptr<const font::Font> font);
			presentation::TextAnchor textAlign() const;
			RenderingContext2D& setTextAlign(presentation::TextAnchor anchor);
			presentation::AlignmentBaseline textBaseline() const;
			RenderingContext2D& setBaseline(presentation::AlignmentBaseline baseline);
			// shared path API methods (CanvasPathMethods interface)
			RenderingContext2D& closePath();
			RenderingContext2D& moveTo(const NativePoint& to);
			RenderingContext2D& lineTo(const NativePoint& to);
			RenderingContext2D& quadraticCurveTo(double cpx, double cpy, double x, double y);
			RenderingContext2D& bezierCurveTo(double cp1x, double cp1y, double cp2x, double cp2y, double x, double y);
			RenderingContext2D& arcTo(double x1, double y1, double x2, double y2, double radius); 
			RenderingContext2D& rectangle(const NativeRectangle& rect);
			RenderingContext2D& arc(const NativePoint& to, Scalar radius, double startAngle, double endAngle, bool counterClockwise = false);
		private:
#if defined(ASCENSION_GRAPHICS_SYSTEM_CAIRO)
			Cairo::RefPtr<Cairo::Context> nativeObject_;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_CORE_GRAPHICS)
			CGContextRef nativeObject_;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_DIRECT2D)
			win32::com::SmartPointer<ID2D1RenderTarget> nativeObject_;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_QT)
			std::shared_ptr<QPainter> nativeObject_;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI)
			win32::Handle<HDC> nativeObject_;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDIPLUS)
			std::shared_ptr<Gdiplus::Graphics> nativeObject_;
#endif
		};

		class PaintContext {
		public:
			/**
			 * Constructor.
			 * @param context The rendering context
			 * @param boundsToPaint The rectangle in which the painting is requested
			 */
			PaintContext(RenderingContext2D&& context, const NativeRectangle& boundsToPaint)
				: context_(std::move(context)), boundsToPaint_(boundsToPaint) {}
			/// Returns the rendering context.
			RenderingContext2D& operator*() /*noexcept*/ {return context_;}
			/// Returns the rendering context.
			const RenderingContext2D& operator*() const /*noexcept*/ {return context_;}
			/// Returns the rendering context.
			RenderingContext2D* operator->() /*noexcept*/ {return &context_;}
			/// Returns the rendering context.
			const RenderingContext2D* operator->() const /*noexcept*/ {return &context_;}
			/// Returns a rectangle in which the painting is requested.
			const NativeRectangle& boundsToPaint() const /*noexcept*/ {return boundsToPaint_;}
		private:
			RenderingContext2D context_;
			const NativeRectangle boundsToPaint_;
		};
	}
}

#endif //!ASCENSION_RENDERING_CONTEXT_HPP
