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
			/**
			 * Creates a @c PaintContext object from @c RenderingContext2D.
			 * @param context The rendering context
			 * @param boundsToPaint The rectangle in which the painting is requested
			 */
			PaintContext(RenderingContext2D&& context, const Rectangle& boundsToPaint)
				: RenderingContext2D(std::move(context)), boundsToPaint_(boundsToPaint) {}
			/**
			 * @overload
			 * @param context The rendering context
			 * @param boundsToPaint The rectangle in which the painting is requested
			 */
			PaintContext(std::unique_ptr<RenderingContext2D> context, const Rectangle& boundsToPaint)
				: RenderingContext2D(context->native()), boundsToPaint_(boundsToPaint) {}
			/// Returns a rectangle in which the painting is requested.
			const Rectangle& boundsToPaint() const BOOST_NOEXCEPT {return boundsToPaint_;}

		private:
			const Rectangle boundsToPaint_;
		};
	}
}

#endif // !ASCENSION_PAINT_CONTEXT_HPP
