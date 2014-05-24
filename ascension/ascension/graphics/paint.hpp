/**
 * @file paint.hpp
 * @author exeal
 * @date 2011-03-13 created
 * @date 2011-2014
 */

#ifndef ASCENSION_PAINT_HPP
#define ASCENSION_PAINT_HPP

#include <ascension/graphics/color.hpp>
#include <ascension/graphics/geometry.hpp>
#include <ascension/graphics/object.hpp>
#include <memory>
//#include <boost/operators.hpp>
#if ASCENSION_SELECTS_GRAPHICS_SYSTEM(CAIRO)
#	include <cairomm/cairomm.h>
#endif

namespace ascension {
	namespace graphics {

		/**
		 * Defines fill or stroke style of rendering context.
		 * @see RenderingContext2D#fillStyle, RenderingContext2D#strokeStyle,
		 *      RenderingContext2D#setFillStyle, RenderingContext2D#setStrokeStyle
		 */
		class Paint : public Wrapper<Paint>, public std::enable_shared_from_this<Paint>
			 /*, private boost::equality_comparable<Paint>*/ {
		public:
			/// Constructor.
			Paint() BOOST_NOEXCEPT : revisionNumber_(0) {}
			/// Destructor.
			virtual ~Paint() BOOST_NOEXCEPT;
			/// Returns the revision number.
			std::size_t revisionNumber() const BOOST_NOEXCEPT {
				return revisionNumber_;
			}
			/// Returns the native object which implements this object.
#if ASCENSION_SELECTS_GRAPHICS_SYSTEM(CAIRO)
			Cairo::RefPtr<Cairo::Pattern> native() const BOOST_NOEXCEPT {return nativeObject_;}
		protected:
			void reset(Cairo::RefPtr<Cairo::Pattern> nativeObject) {nativeObject_ = nativeObject;}
		private:
			Cairo::RefPtr<Cairo::Pattern> nativeObject_;
#elif ASCENSION_SELECTS_GRAPHICS_SYSTEM(CORE_GRAPHICS)
			???? native() const;
		protected:
			void reset(???? nativeObject) BOOST_NOEXCEPT;
		private:
			???? nativeObject_;
#elif ASCENSION_SELECTS_GRAPHICS_SYSTEM(DIRECT2D)
			win32::com::SmartPointer<ID2D1Brush> native() const BOOST_NOEXCEPT;
		protected:
			void reset(win32::com::SmartPointer<ID2D1Brush> nativeObject) BOOST_NOEXCEPT;
		private:
			win32::com::SmartPointer<ID2D1Brush> nativeObject_;
#elif ASCENSION_SELECTS_GRAPHICS_SYSTEM(QT)
			const std::unique_ptr<QBrush>& native() const BOOST_NOEXCEPT;
		protected:
			void reset(std::unique_ptr<QBrush>&& nativeObject) BOOST_NOEXCEPT;
		private:
			std::unique_ptr<QBrush> nativeObject_;
#elif ASCENSION_SELECTS_GRAPHICS_SYSTEM(WIN32_GDI)
			const LOGBRUSH& native() const BOOST_NOEXCEPT;
		protected:
			void reset(LOGBRUSH&& nativeObject) BOOST_NOEXCEPT;
		private:
			LOGBRUSH nativeObject_;
#elif ASCENSION_SELECTS_GRAPHICS_SYSTEM(WIN32_GDIPLUS)
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
			/**
			 * Creates a solid color pattern with given color value.
			 * @param color The color value
			 */
			explicit SolidColor(const Color& color);
			/// Returns the solid color value for this object.
			const Color& color() const BOOST_NOEXCEPT;
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
			/**
			 * Returns a linear @c Gradient initialized with the specified line.
			 * @param p0 The start point of the gradient
			 * @param p1 The end point of the gradient
			 */
			LinearGradient(const Point& p0, const Point& p1);
			void addColorStop(double offset, const Color& color) override;
		};

		/**
		 * Represents a radial gradient that paints along the cone.
		 * @see LinearGradient, http://www.w3.org/TR/2dcontext/#dom-context-2d-createradialgradient
		 */
		class RadialGradient : public Gradient {
		public:
			/**
			 * Returns a radial @c Gradient initialized with the two specified circles.
			 * @param p0 The origin of the start circle
			 * @param r0 The radius of the start circle
			 * @param p1 The origin of the end circle
			 * @param r1 The radius of the end circle
			 * @throw std#out_of_range Either of @a r0 or @a r1 are negative
			 */
			RadialGradient(const Point& p0, Scalar r0, const Point& p1, Scalar r1);
			void addColorStop(double offset, const Color& color) override;
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
