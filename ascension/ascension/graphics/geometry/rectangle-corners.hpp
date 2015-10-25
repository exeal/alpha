/**
 * @file rectangle-corners.hpp
 * @exeal author
 * @date 2015-10-23 Separated from algorithm.hpp.
 */

#ifndef ASCENSION_RECTANGLE_CORNERS_HPP
#define ASCENSION_RECTANGLE_CORNERS_HPP
#include <ascension/graphics/geometry/rectangle-sides.hpp>
#include <boost/geometry/algorithms/assign.hpp>
#include <boost/geometry/algorithms/make.hpp>
#include <boost/geometry/core/point_type.hpp>

namespace ascension {
	namespace graphics {
		namespace geometry {
			/// @defgroup geometry_rectangle_corners Geometric Algorithms Access the Corners of a Rectangle
			/// @{

			/**
			 * Returns the bottom-left corner of @a rectangle.
			 * @see #bottom, #left
			 */
			template<typename Geometry>
			inline typename boost::geometry::point_type<Geometry>::type bottomLeft(const Geometry& rectangle) {
				return boost::geometry::make<typename boost::geometry::point_type<Geometry>::type>(left(rectangle), bottom(rectangle));
			}

			/**
			 * Returns the bottom-right corner of @a rectangle.
			 * @see #bottom, #right
			 */
			template<typename Geometry>
			inline typename boost::geometry::point_type<Geometry>::type bottomRight(const Geometry& rectangle) {
				return boost::geometry::make<typename boost::geometry::point_type<Geometry>::type>(right(rectangle), bottom(rectangle));
			}

			/**
			 * Returns the top-left corner of @a rectangle.
			 * @see #top, #left
			 */
			template<typename Geometry>
			inline typename boost::geometry::point_type<Geometry>::type topLeft(const Geometry& rectangle) {
				return boost::geometry::make<typename boost::geometry::point_type<Geometry>::type>(left(rectangle), top(rectangle));
			}

			/**
			 * Returns the top-right corner of @a rectangle.
			 * @see #top, #right
			 */
			template<typename Geometry>
			inline typename boost::geometry::point_type<Geometry>::type topRight(const Geometry& rectangle) {
				return boost::geometry::make<typename boost::geometry::point_type<Geometry>::type>(right(rectangle), top(rectangle));
			}
			/// @}
		}
	}
}

#endif // !ASCENSION_RECTANGLE_COORDINATES_HPP
