/**
 * @file paint.hpp
 * @author exeal
 * @date 2011-03-13 created
 * @date 2011-2012
 */

#ifndef ASCENSION_PAINT_HPP
#define ASCENSION_PAINT_HPP

#include <ascension/corelib/basic-types.hpp>	// std.tr1.shared_ptr
#include <ascension/graphics/color.hpp>
#include <ascension/graphics/geometry.hpp>
#include <ascension/graphics/rendering-context.hpp>
#include <memory>	// std.unique_ptr
//#include <boost/operators.hpp>

namespace ascension {
	namespace graphics {

		/**
		 * Defines fill or stroke style of rendering context.
		 * @see RenderingContext2D#fillStyle, RenderingContext2D#strokeStyle,
		 *      RenderingContext2D#setFillStyle, RenderingContext2D#setStrokeStyle
		 */
		class Paint /*: private boost::equality_comparable<Paint>*/ {
			ASCENSION_NONCOPYABLE_TAG(Paint);
		public:
			/// Constructor.
			Paint() /*noexcept*/ : revisionNumber_(0) {}
			/// Destructor.
			virtual ~Paint() /*noexcept*/;
			std::size_t revisionNumber() const /*noexcept*/ {
				return revisionNumber_;
			}
#if defined(ASCENSION_GRAPHICS_SYSTEM_CAIRO)
			Cairo::PtrRef<Cairo::Pattern> asNativeObject() const /*noexcept*/;
		protected:
			void reset(Cairo::PtrRef<Cairo::Pattern> nativeObject);
		private:
			Cairo::PtrRef<Cairo::Pattern> nativeObject_;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_CORE_GRAPHICS)
			???? asNativeObject() const;
		protected:
			void reset(???? nativeObject) /*noexcept*/;
		private:
			???? nativeObject_;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_DIRECT2D)
			win32::com::SmartPointer<ID2D1Brush> asNativeObject() const /*noexcept*/;
		protected:
			void reset(win32::com::SmartPointer<ID2D1Brush> nativeObject) /*noexcept*/;
		private:
			win32::com::SmartPointer<ID2D1Brush> nativeObject_;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_QT)
			const std::unique_ptr<QBrush>& asNativeObject() const /*noexcept*/;
		protected:
			void reset(std::unique_ptr<QBrush>&& nativeObject) /*noexcept*/;
		private:
			std::unique_ptr<QBrush> nativeObject_;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI)
			const LOGBRUSH& asNativeObject() const /*noexcept*/;
		protected:
			void reset(LOGBRUSH&& nativeObject) /*noexcept*/;
		private:
			LOGBRUSH nativeObject_;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDIPLUS)
			const std::unique_ptr<Gdiplus::Brush>& nativeObject() const /*noexcept*/;
		protected:
			void reset(std::unique_ptr<Gdiplus::Brush>&& nativeObject) /*noexcept*/;
		private:
			std::unique_ptr<Gdiplus::Brush> nativeObject_;
#endif
			std::size_t revisionNumber_;
		};

		class SolidColor : public Paint {
		public:
			explicit SolidColor(const Color& color);
			const Color& color() const /*noexcept*/;
			void setColor(const Color& color);
		private:
			Color color_;
		};

		/**
		 *
		 * @see CanvasGradient interface in HTML Canvas 2D Context
		 *      (http://www.w3.org/TR/2dcontext/#canvasgradient)
		 */
		class Gradient : public Paint {
		public:
			/// Destructor.
			virtual ~Gradient() /*noexcept()*/ {}
			/**
			 * Adds a color stop with the given color to the gradient at the given offset.
			 * @param offset The offset. 0.0 is the offset at one end of the gradient, 1.0 is the
			 *               offset at the other end
			 * @param color The color
			 * @throw std#out_of_range @a offset is invalid
			 * @throw std#invalid_argument @a color is invalid
			 */
			virtual void addColorStop(double offset, const Color& color) = 0;
		};

		/**
		 * Represents a linear gradient that paints along the line.
		 * @see RadialGradient, http://www.w3.org/TR/2dcontext/#dom-context-2d-createlineargradient
		 */
		class LinearGradient : public Gradient {
		public:
			LinearGradient(const NativePoint& p0, const NativePoint& p1);
		};

		/**
		 * Represents a radial gradient that paints along the cone.
		 * @see LinearGradient, http://www.w3.org/TR/2dcontext/#dom-context-2d-createradialgradient
		 */
		class RadialGradient : public Gradient {
		public:
			RadialGradient(const NativePoint& p0, Scalar r0, const NativePoint& p1, Scalar r1);
		};

		/**
		 *
		 * @see CanvasPattern interface in HTML Canvas 2D Context
		 *      (http://www.w3.org/TR/2dcontext/#canvaspattern)
		 */
		class Pattern : public Paint {
		public:
			virtual ~Pattern() /*throw()*/ {}
		};

	}
}

#endif //!ASCENSION_RENDERING_CONTEXT_HPP
