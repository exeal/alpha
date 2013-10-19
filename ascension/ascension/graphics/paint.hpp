/**
 * @file paint.hpp
 * @author exeal
 * @date 2011-03-13 created
 * @date 2011-2013
 */

#ifndef ASCENSION_PAINT_HPP
#define ASCENSION_PAINT_HPP

#include <ascension/corelib/basic-types.hpp>	// std.tr1.shared_ptr
#include <ascension/graphics/color.hpp>
#include <ascension/graphics/geometry.hpp>
#include <memory>	// std.unique_ptr, std.enable_shared_from_this
//#include <boost/operators.hpp>
#ifdef ASCENSION_GRAPHICS_SYSTEM_CAIRO
#	include <cairomm/cairomm.h>
#endif

namespace ascension {
	namespace graphics {

		/**
		 * Defines fill or stroke style of rendering context.
		 * @see RenderingContext2D#fillStyle, RenderingContext2D#strokeStyle,
		 *      RenderingContext2D#setFillStyle, RenderingContext2D#setStrokeStyle
		 */
		class Paint : public std::enable_shared_from_this<Paint> /*, private boost::equality_comparable<Paint>*/ {
			ASCENSION_NONCOPYABLE_TAG(Paint);
		public:
			/// Constructor.
			Paint() BOOST_NOEXCEPT : revisionNumber_(0) {}
			/// Destructor.
			virtual ~Paint() BOOST_NOEXCEPT;
			std::size_t revisionNumber() const BOOST_NOEXCEPT {
				return revisionNumber_;
			}
#if defined(ASCENSION_GRAPHICS_SYSTEM_CAIRO)
			Cairo::RefPtr<Cairo::Pattern> asNativeObject() const BOOST_NOEXCEPT;
		protected:
			void reset(Cairo::RefPtr<Cairo::Pattern> nativeObject);
		private:
			Cairo::RefPtr<Cairo::Pattern> nativeObject_;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_CORE_GRAPHICS)
			???? asNativeObject() const;
		protected:
			void reset(???? nativeObject) BOOST_NOEXCEPT;
		private:
			???? nativeObject_;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_DIRECT2D)
			win32::com::SmartPointer<ID2D1Brush> asNativeObject() const BOOST_NOEXCEPT;
		protected:
			void reset(win32::com::SmartPointer<ID2D1Brush> nativeObject) BOOST_NOEXCEPT;
		private:
			win32::com::SmartPointer<ID2D1Brush> nativeObject_;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_QT)
			const std::unique_ptr<QBrush>& asNativeObject() const BOOST_NOEXCEPT;
		protected:
			void reset(std::unique_ptr<QBrush>&& nativeObject) BOOST_NOEXCEPT;
		private:
			std::unique_ptr<QBrush> nativeObject_;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI)
			const LOGBRUSH& asNativeObject() const BOOST_NOEXCEPT;
		protected:
			void reset(LOGBRUSH&& nativeObject) BOOST_NOEXCEPT;
		private:
			LOGBRUSH nativeObject_;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDIPLUS)
			const std::unique_ptr<Gdiplus::Brush>& nativeObject() const BOOST_NOEXCEPT;
		protected:
			void reset(std::unique_ptr<Gdiplus::Brush>&& nativeObject) BOOST_NOEXCEPT;
		private:
			std::unique_ptr<Gdiplus::Brush> nativeObject_;
#endif
			std::size_t revisionNumber_;
		};

		class SolidColor : public Paint {
		public:
			explicit SolidColor(const Color& color);
			const Color& color() const BOOST_NOEXCEPT;
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
			virtual ~Gradient() BOOST_NOEXCEPT {}
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
			LinearGradient(const Point& p0, const Point& p1);
		};

		/**
		 * Represents a radial gradient that paints along the cone.
		 * @see LinearGradient, http://www.w3.org/TR/2dcontext/#dom-context-2d-createradialgradient
		 */
		class RadialGradient : public Gradient {
		public:
			RadialGradient(const Point& p0, Scalar r0, const Point& p1, Scalar r1);
		};

		/**
		 *
		 * @see CanvasPattern interface in HTML Canvas 2D Context
		 *      (http://www.w3.org/TR/2dcontext/#canvaspattern)
		 */
		class Pattern : public Paint {
		public:
			virtual ~Pattern() BOOST_NOEXCEPT {}
		};

	}
}

#endif //!ASCENSION_RENDERING_CONTEXT_HPP
