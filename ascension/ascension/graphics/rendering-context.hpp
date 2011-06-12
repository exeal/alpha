/**
 * @file rendering-context.hpp
 * @author exeal
 * @date 2011-03-06 created
 */

#ifndef ASCENSION_RENDERING_CONTEXT_HPP
#define ASCENSION_RENDERING_CONTEXT_HPP

#include <ascension/corelib/basic-types.hpp>	// std.tr1.shared_ptr
#include <ascension/graphics/color.hpp>
#include <ascension/graphics/font.hpp>
#include <ascension/graphics/geometry.hpp>
#include <ascension/graphics/paint.hpp>
#include <stack>
#include <vector>

namespace ascension {
	namespace graphics {

		enum CompositeOperation {
			SOURCE_ATOP, SOURCE_IN, SOURCE_OUT, SOURCE_OVER,
			DESTINATION_ATOP, DESTINATION_IN, DESTINATION_OUT, DESTINATION_OVER,
			LIGHTER, COPY, XOR
		};

		class ImageData : public NativeSize {
		};

		/**
		 * @c CanvasRenderingContext2D interface defined in "HTML Canvas 2D Context"
		 * (http://dev.w3.org/html5/2dcontext/).
		 */
		class RenderingContext2D {
		public:
			RenderingContext2D() {}
			virtual ~RenderingContext2D() /*throw()*/ {}

//			virtual ??? canvas() const = 0;

			/// Pushes the current state onto the stack.
			virtual RenderingContext2D& save() = 0;
			/// Pops the top state on the stack, restoring the context to that state.
			virtual RenderingContext2D& restore() = 0;

			virtual RenderingContext2D& scale() = 0;
			virtual RenderingContext2D& rotate() = 0;
			virtual RenderingContext2D& translate() = 0;
			virtual RenderingContext2D& transform() = 0;
			virtual RenderingContext2D& setTransform() = 0;

			double globalAlpha() const /*throw()*/ {
				return states_.top().globalAlpha;
			}
			RenderingContext2D& setGlobalAlpha(double value) {
				if(value < 0.0 || value > 1.0)
					throw std::invalid_argument("value");
				states_.top().globalAlpha = value;
				return *this;
			}
			CompositeOperation globalCompositeOperation() const /*throw()*/ {
				return states_.top().compositeOperation;
			}
			RenderingContext2D& setGlobalCompositeOperation(CompositeOperation co) {
				// TODO: sanity check...
				states_.top().compositeOperation = co;
				return *this;
			}

			const Paint& strokeStyle() const /*throw()*/ {
				return states_.top().strokeStyle;
			}
			RenderingContext2D& setStrokeStyle(const Paint& style) {
				states_.top().strokeStyle = style;
				return *this;
			}
			const Paint& fillStyle() const /*throw()*/ {
				return states_.top().fillStyle;
			}
			RenderingContext2D& setFillStyle(const Paint& style) {
				states_.top().fillStyle = style;
				return *this;
			}
//			virtual std::auto_ptr<Gradient> createLinearGradient() = 0;
//			virtual std::auto_ptr<Gradient> createRadicalGradient() = 0;
//			virtual Pattern createPattern() = 0;

			FillRule fillRule() const /*throw()*/ {
				return states_.top().fillRule;
			}
			RenderingContext2D& setFillRule(FillRule rule) {
				states_.top().fillRule = rule;
				return *this;
			}
			LineCap lineCap() const /*throw()*/ {
				return states_.top().strokeLineCap;
			}
			RenderingContext2D& setLineCap(LineCap value) {
				states_.top().strokeLineCap = value;
				return *this;
			}
			LineJoin lineJoin() const /*throw()*/ {
				return states_.top().strokeLineJoin;
			}
			RenderingContext2D& setLineJoin(LineJoin value) {
				states_.top().strokeLineJoin = value;
				return *this;
			}

			NativeSize shadowOffset() const /*throw()*/ {
				return states_.top().shadowOffset;
			}
			RenderingContext2D& setShadowOffset(const NativeSize& offset) /*throw()*/ {
				states_.top().shadowOffset = offset;
				return *this;
			}
			Scalar shadowBlur() const /*throw()*/ {
				return states_.top().shadowBlur;
			}
			RenderingContext2D& setShadowBlur(Scalar blur) /*throw()*/ {
				states_.top().shadowBlur = blur;
				return *this;
			}
			Color shadowColor() const /*throw()*/ {
				return states_.top().shadowColor;
			}
			RenderingContext2D& setShadowColor(const Color& color) /*throw()*/ {
				states_.top().shadowColor = color;
				return *this;
			}

			virtual RenderingContext2D& clearRectangle(const NativeRectangle&) = 0;
			virtual RenderingContext2D& fillRectangle(const NativeRectangle&) = 0;
			virtual RenderingContext2D& strokeRectangle(const NativeRectangle&) = 0;

			virtual RenderingContext2D& beginPath() = 0;
			virtual RenderingContext2D& closePath() = 0;
			virtual RenderingContext2D& moveTo(const NativePoint&) = 0;
			virtual RenderingContext2D& lineTo(const NativePoint&) = 0;
			virtual RenderingContext2D& quadraticCurveTo() = 0;
			virtual RenderingContext2D& bezierCurveTo() = 0;
			virtual RenderingContext2D& arcTo() = 0;
			virtual RenderingContext2D& rectangle() = 0;
			virtual RenderingContext2D& arc() = 0;
			virtual RenderingContext2D& fill() = 0;
			virtual RenderingContext2D& stroke() = 0;
			virtual RenderingContext2D& clip() = 0;
			virtual bool isPointInPath() const /*throw()*/ = 0;

			virtual bool drawFocusRing() = 0;

			std::tr1::shared_ptr<const font::Font> font() const /*throw()*/ {
				return states_.top().font;
			}
			RenderingContext2D& setFont(std::tr1::shared_ptr<const font::Font> font) {
				// TODO: check if null. 
				states_.top().font = font;
				return *this;
			}
//			virtual TextAnchor& textAnchor() const /*throw()*/ = 0;
//			virtual Baseline& textBaseline() const /*throw()*/ = 0;
			virtual RenderingContext2D& fillText(const String& text, const NativePoint& p, Scalar maxWidth) = 0;
			virtual RenderingContext2D& strokeText(const String& text, const NativePoint& p, Scalar maxWidth) = 0;
			virtual NativeSize measureText(const String& text) = 0;

			virtual RenderingContext2D& drawImage() = 0;

			virtual ImageData createImageData(const NativeSize&) const = 0;
			virtual ImageData createImageData(const ImageData&) const = 0;
			virtual ImageData getImageData() const = 0;
			virtual ImageData putImageData() = 0;

			virtual unsigned int logicalDpiX() const = 0;
			virtual unsigned int logicalDpiY() const = 0;
			virtual NativeSize size() const = 0;

#if defined(ASCENSION_GRAPHICS_SYSTEM_CAIRO)
			const CairoObject<cairo_t>& nativeObject() const /*throw()*/;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_CORE_GRAPHICS)
			const CGObject<CGContext>& nativeObject() const /*throw()*/;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_QT)
			QPainter& nativeObject() const /*throw()*/;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI)
			const win32::Handle<HDC>& nativeObject() const /*throw()*/;
#endif

		private:
			struct State {
				double globalAlpha;
				CompositeOperation compositeOperation;
				Paint fillStyle, strokeStyle;
				FillRule fillRule;
				LineCap strokeLineCap;
				LineJoin strokeLineJoin;
//				std::vector<Scalar> dashArray;
//				Scalar dashOffset;
				Color shadowColor;
				NativeSize shadowOffset;
				Scalar shadowBlur;
				std::tr1::shared_ptr<const font::Font> font;
			};
			std::stack<State> states_;
		};

		class Context : public RenderingContext2D {
		public:
			virtual ~Context() /*throw()*/ {}
		};

		class PaintContext : public Context {
		public:
			/// Destructor.
			virtual ~PaintContext() /*throw()*/ {}
			/// Returns a rectangle in which the painting is requested.
			virtual NativeRectangle boundsToPaint() const = 0;
		};

	}
}

#endif //!ASCENSION_RENDERING_CONTEXT_HPP
