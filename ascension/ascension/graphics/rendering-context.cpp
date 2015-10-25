/**
 * @file rendering-context.cpp
 * Implements @c ascension#graphics#RenderingContext2D class.
 * @author exeal
 * @date 2015-10-24 Created.
 */

#include <ascension/graphics/rendering-context.hpp>
#include <ascension/graphics/geometry/algorithms/make.hpp>

namespace ascension {
	namespace graphics {
		RenderingContext2D& RenderingContext2D::putImageData(const ImageData& image, const Point& destination) {
			return putImageData(image, destination, geometry::make<Rectangle>(
				boost::geometry::make_zero<Point>(),
				Dimension(
					geometry::_dx = static_cast<Scalar>(image.width()),
					geometry::_dy = static_cast<Scalar>(image.height()))));
		}
	}
}
