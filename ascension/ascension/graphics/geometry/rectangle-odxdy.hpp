/**
 * @file rectangle-odxdy.hpp
 * Defines free function accesses the origin, dx and dy of a rectangular geometry.
 * @author exeal
 * @date 2015-10-24
 */

#ifndef ASCENSION_GEOMETRY_RECTANGLE_ODXDY_HPP
#define ASCENSION_GEOMETRY_RECTANGLE_ODXDY_HPP
#include <ascension/graphics/geometry/common.hpp>
#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/coordinate_type.hpp>

namespace ascension {
	namespace graphics {
		namespace geometry {
			/// @addtogroup geometry_additional_accessors
			/// @{

			/// Returns the size of the @a rectangle in x-coordinate. The result can be negative value.
			template<typename Geometry>
			inline typename boost::geometry::coordinate_type<Geometry>::type dx(const Geometry& rectangle, typename detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
				return boost::geometry::get<boost::geometry::max_corner, 0>(rectangle) - boost::geometry::get<boost::geometry::min_corner, 0>(rectangle);
			}

			/// Returns the size of the @a rectangle in y-coordinate. The result can be negative value.
			template<typename Geometry>
			inline typename boost::geometry::coordinate_type<Geometry>::type dy(const Geometry& rectangle, typename detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
				return boost::geometry::get<boost::geometry::max_corner, 1>(rectangle) - boost::geometry::get<boost::geometry::min_corner, 1>(rectangle);
			}

			/// Returns the origin of the @a rectangle.
			template<typename Geometry>
			inline typename boost::geometry::point_type<Geometry>::type origin(const Geometry& rectangle, typename detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
				typename boost::geometry::point_type<Geometry>::type temp;
				boost::geometry::assign_values(temp, boost::geometry::get<boost::geometry::min_corner, 0>(rectangle), boost::geometry::get<boost::geometry::min_corner, 1>(rectangle));
				return temp;
			}
			/// @}
		}
	}
}

#endif // !ASCENSION_GEOMETRY_RECTANGLE_ODXDY_HPP
