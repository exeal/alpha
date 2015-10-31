/**
 * @file within.hpp
 * Defines @c ascension#graphics#geometry#within free function.
 * @author exeal
 * @date 2015-10-31 Created.
 */

#ifndef ASCENSION_GEOMETRY_WITHIN_HPP
#define ASCENSION_GEOMETRY_WITHIN_HPP
#include <ascension/graphics/geometry/common.hpp>
#include <boost/geometry/core/access.hpp>

namespace ascension {
	namespace graphics {
		namespace geometry {
			/**
			 * Checks the point is inside the rectangle.
			 * @tparam Point The type of @a point
			 * @tparam Rectangle The type of @a rectangle
			 * @param point The point to test
			 * @param rectangle The rectangle to test
			 * @return true if @a point is within @a rectangle
			 * @see boost#geometry#covered_by, boost#geometry#within
			 */
			template<typename Point, typename Rectangle>
			inline bool within(const Point& point, const Rectangle& rectangle,
					typename detail::EnableIfTagIs<Point, boost::geometry::point_tag>::type* = nullptr,
					typename detail::EnableIfTagIs<Rectangle, boost::geometry::box_tag>::type* = nullptr) {
				return boost::geometry::get<0>(point) >= boost::geometry::get<boost::geometry::min_corner, 0>(rectangle)
					&& boost::geometry::get<0>(point) < boost::geometry::get<boost::geometry::max_corner, 0>(rectangle)
					&& boost::geometry::get<1>(point) >= boost::geometry::get<boost::geometry::min_corner, 1>(rectangle)
					&& boost::geometry::get<1>(point) < boost::geometry::get<boost::geometry::max_corner, 1>(rectangle);
			}
		}
	}
}

#endif // !ASCENSION_GEOMETRY_WITHIN_HPP
