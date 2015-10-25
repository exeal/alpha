/**
 * @file rectangle.hpp
 * Defines 2D rectangle types for geometry.
 * @author exeal
 * @date 2010-11-06 Created (as geometry.hpp)
 * @date 2014-09-07 Separated from geometry.hpp
 */

#ifndef ASCENSION_GEOMETRY_RECTANGLE_HPP
#define ASCENSION_GEOMETRY_RECTANGLE_HPP
#include <ascension/graphics/geometry/point.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <utility>	// std.pair

namespace ascension {
	namespace graphics {
		namespace geometry {
			/**
			 * Describes a rectangle defined by two @c BasicPoint instances.
			 * @see graphics#PhysicalFourSidesBase, graphics#PhysicalFourSides
			 * @addtogroup geometric_primitives
			 */
			typedef boost::geometry::model::box<Point> Rectangle;
		}

		using geometry::Rectangle;
	}
}

#endif // !ASCENSION_GEOMETRY_RECTANGLE_HPP
