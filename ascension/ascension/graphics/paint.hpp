/**
 * @file paint.hpp
 * @author exeal
 * @date 2011-03-13 created
 */

#ifndef ASCENSION_PAINT_HPP
#define ASCENSION_PAINT_HPP

#include <ascension/corelib/basic-types.hpp>	// std.tr1.shared_ptr
#include <ascension/graphics/color.hpp>
#include <ascension/graphics/geometry.hpp>
#include <memory>	// std.auto_ptr

namespace ascension {
	namespace graphics {

		class RenderingContext2D;

		class PaintServer {
		public:
			/// Destructor.
			virtual ~PaintServer() /*throw()*/ {}
			/**
			 * Paints the rectangle.
			 * @param context The graphic context
			 * @param rectangle The rectangle to fill
			 */
			virtual void fillRectangle(
				RenderingContext2D& context, const NativeRectangle& rectangle) const /*throw()*/ = 0;
		};

		/**
		 *
		 * @see CanvasGradient interface in HTML Canvas 2D Context
		 *      (http://www.w3.org/TR/2dcontext/#canvasgradient)
		 */
		class Gradient : public PaintServer {
		public:
			virtual ~Gradient() /*throw()*/ {}
			virtual void addColorStop() = 0;
			static std::auto_ptr<Gradient> createLinearGradient(const NativePoint& p0, const NativePoint& p1);
			static std::auto_ptr<Gradient> createRadialGradient(
				const NativePoint& p0, Scalar r0, const NativePoint& p1, Scalar r1);
		};

		/**
		 *
		 * @see CanvasPattern interface in HTML Canvas 2D Context
		 *      (http://www.w3.org/TR/2dcontext/#canvaspattern)
		 */
		class Pattern : public PaintServer {
		public:
			virtual ~Pattern() /*throw()*/ {}
		};

		class Paint {
		public:
			Paint() /*throw()*/ {}
			explicit Paint(const Color& color) : color_(color) {if(color == Color()) throw std::invalid_argument("color");}
			explicit Paint(std::auto_ptr<const Gradient> gradient) : color_(Color::TRANSPARENT_COLOR), gradient_(gradient) {}
			explicit Paint(const Color& color, std::auto_ptr<const Gradient> gradient) : color_(color), gradient_(gradient) {}
			explicit Paint(std::auto_ptr<const Pattern> pattern) : color_(Color::TRANSPARENT_COLOR), pattern_(pattern) {}
			explicit Paint(const Color& color, std::auto_ptr<const Pattern> pattern) : color_(color), pattern_(pattern) {}
			const Color& color() const /*throw()*/ {return color_;}
			const Gradient* gradient() const /*throw()*/ {return gradient_.get();}
			const Pattern* pattern() const /*throw()*/ {return pattern_.get();}
		private:
			Color color_;
			std::shared_ptr<const Gradient> gradient_;
			std::shared_ptr<const Pattern> pattern_;
		};

		inline bool operator==(const Paint& lhs, const Paint& rhs) /*throw()*/ {
			return lhs.color() == rhs.color()
				&& lhs.gradient() == rhs.gradient() && lhs.pattern() == rhs.pattern();
		}
		inline bool operator!=(const Paint& lhs, const Paint& rhs) /*throw()*/ {return !(lhs == rhs);}

		enum FillRule {NONZERO, EVENODD};

		enum LineCap {BUTT_LINE_CAP, ROUND_LINE_CAP, SQUARE_LINE_CAP};

		enum LineJoin {BEVEL_LINE_JOIN, ROUND_LINE_JOIN, MITER_LINE_JOIN};

	}
}

#endif //!ASCENSION_RENDERING_CONTEXT_HPP
