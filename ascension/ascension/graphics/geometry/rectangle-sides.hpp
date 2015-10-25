/**
 * @file rectangle-sides.hpp
 * @exeal author
 * @date 2015-10-23 Separated from algorithm.hpp.
 */

#ifndef ASCENSION_RECTANGLE_SIDES_HPP
#define ASCENSION_RECTANGLE_SIDES_HPP
#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/coordinate_type.hpp>
#include <utility>

namespace ascension {
	namespace graphics {
		namespace geometry {
			/// @defgroup geometry_rectangle_sides Geometric Algorithms Access Sides of a Rectangle
			/// @{

			/**
			 * Returns the y-coordinate of the bottom edge of @a rectangle.
			 * @see #bottomLeft, #bottomRight, #top
			 */
			template<typename Geometry>
			inline typename boost::geometry::coordinate_type<Geometry>::type bottom(const Geometry& rectangle) {
				return std::max(
					boost::geometry::get<boost::geometry::min_corner, 1>(rectangle),
					boost::geometry::get<boost::geometry::max_corner, 1>(rectangle));
			}

			/**
			 * Returns the x-coordinate of the left edge of @a rectangle.
			 * @see #bottomLeft, #right, #topLeft
			 */
			template<typename Geometry>
			inline typename boost::geometry::coordinate_type<Geometry>::type left(const Geometry& rectangle) {
				return std::min(
					boost::geometry::get<boost::geometry::min_corner, 0>(rectangle),
					boost::geometry::get<boost::geometry::max_corner, 0>(rectangle));
			}

			/**
			 * Returns the x-coordinate of the right edge of @a rectangle.
			 * @see #bottomRight, #left, #topRight
			 */
			template<typename Geometry>
			inline typename boost::geometry::coordinate_type<Geometry>::type right(const Geometry& rectangle) {
				return std::max(
					boost::geometry::get<boost::geometry::min_corner, 0>(rectangle),
					boost::geometry::get<boost::geometry::max_corner, 0>(rectangle));
			}

			/**
			 * Returns the y-coordinate of the top edge of @a rectangle.
			 * @see #bottom, #topLeft, #topRight
			 */
			template<typename Geometry>
			inline typename boost::geometry::coordinate_type<Geometry>::type top(const Geometry& rectangle) {
				return std::min(
					boost::geometry::get<boost::geometry::min_corner, 1>(rectangle),
					boost::geometry::get<boost::geometry::max_corner, 1>(rectangle));
			}
			/// @}
		}
	}
}

#endif // !ASCENSION_RECTANGLE_SIDES_HPP
