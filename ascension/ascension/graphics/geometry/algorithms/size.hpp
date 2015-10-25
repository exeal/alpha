/**
 * @file size.hpp
 * @exeal author
 * @date 2015-10-23 Separated from algorithm.hpp.
 */

#ifndef ASCENSION_GEOMETRY_SIZE_HPP
#define ASCENSION_GEOMETRY_SIZE_HPP
#include <ascension/graphics/geometry/dimension.hpp>
#include <ascension/graphics/geometry/rectangle-odxdy.hpp>
#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/coordinate_type.hpp>

namespace ascension {
	namespace graphics {
		namespace geometry {
			/// @defgroup geometry_areal_size Geometric Areal and Size-Related Algorithms
			/// @{

			template<typename Geometry>
			inline typename boost::geometry::coordinate_type<Geometry>::type area(const Geometry& dimension, typename detail::EnableIfTagIs<Geometry, DimensionTag>::type* = nullptr) {
				return dx(dimension) * dy(dimension);
			}

			template<typename Geometry>
			inline bool isEmpty(const Geometry& dimension, typename detail::EnableIfTagIs<Geometry, DimensionTag>::type* = nullptr) {
				return dx(dimension) == 0 || dy(dimension) == 0;
			}

			template<typename Geometry>
			inline bool isEmpty(const Geometry& rectangle, typename detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
				return isEmpty(size(rectangle));
			}

			/// Returns the size of the @a rectangle.
			template<typename Geometry>
			inline BasicDimension<typename boost::geometry::coordinate_type<Geometry>::type> size(const Geometry& rectangle, typename detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
				return BasicDimension<typename boost::geometry::coordinate_type<Geometry>::type>(_dx = dx(rectangle), _dy = dy(rectangle));
			}
			/// @}
		}
	}
}

#endif // !ASCENSION_GEOMETRY_SIZE_HPP
