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
#include <boost/operators.hpp>

namespace ascension {
	namespace graphics {

		/**
		 *
		 * @see CanvasGradient interface in HTML Canvas 2D Context
		 *      (http://www.w3.org/TR/2dcontext/#canvasgradient)
		 */
		class Gradient {
		public:
			virtual ~Gradient() /*throw()*/ {}
			virtual void addColorStop() = 0;
			static std::unique_ptr<Gradient> createLinearGradient(const NativePoint& p0, const NativePoint& p1);
			static std::unique_ptr<Gradient> createRadialGradient(
				const NativePoint& p0, Scalar r0, const NativePoint& p1, Scalar r1);
		};

		/**
		 *
		 * @see CanvasPattern interface in HTML Canvas 2D Context
		 *      (http://www.w3.org/TR/2dcontext/#canvaspattern)
		 */
		class Pattern {
		public:
			virtual ~Pattern() /*throw()*/ {}
		};

		class Paint : private boost::equality_comparable<Paint> {
		public:
			Paint() /*throw()*/ {}
			explicit Paint(const Color& color) : color_(color) {
				if(color == Color())
					throw std::invalid_argument("color");
			}
			explicit Paint(std::unique_ptr<const Gradient> gradient) : color_(Color::TRANSPARENT_COLOR), gradient_(std::move(gradient)) {}
			explicit Paint(const Color& color, std::unique_ptr<const Gradient> gradient) : color_(color), gradient_(std::move(gradient)) {}
			explicit Paint(std::unique_ptr<const Pattern> pattern) : color_(Color::TRANSPARENT_COLOR), pattern_(std::move(pattern)) {}
			explicit Paint(const Color& color, std::unique_ptr<const Pattern> pattern) : color_(color), pattern_(std::move(pattern)) {}
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

	}
}

#endif //!ASCENSION_RENDERING_CONTEXT_HPP
