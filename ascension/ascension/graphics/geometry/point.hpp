/**
 * @file geometry/point.hpp
 * Defines 2D point types for geometry.
 * @author exeal
 * @date 2010-11-06 Created (as geometry.hpp)
 * @date 2014-09-07 Separated from geometry.hpp
 */

#ifndef ASCENSION_GEOMETRY_POINT_HPP
#define ASCENSION_GEOMETRY_POINT_HPP
#include <ascension/graphics/geometry/common.hpp>
#include <boost/geometry/geometries/point_xy.hpp>

namespace ascension {
	namespace graphics {
		namespace geometry {
			/**
			 * Defines a point representing a location in Cartesian coordinate system.
			 * @see graphics#PhysicalTwoAxesBase, graphics#PhysicalTwoAxes
			 * @addtogroup geometric_primitives
			 */
			typedef boost::geometry::model::d2::point_xy<Scalar> Point;
		}

		using geometry::Point;
	}
}

#endif // !ASCENSION_GEOMETRY_POINT_HPP
