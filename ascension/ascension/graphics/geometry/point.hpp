/**
 * @file point.hpp
 * Defines 2D point types for geometry.
 * @author exeal
 * @date 2010-11-06 Created (as geometry.hpp)
 * @date 2014-09-07 Separated from geometry.hpp
 */

#ifndef ASCENSION_GEOMETRY_POINT_HPP
#define ASCENSION_GEOMETRY_POINT_HPP
#include <ascension/graphics/geometry/common.hpp>
#include <boost/geometry/algorithms/make.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/parameter.hpp>

namespace ascension {
	namespace graphics {
		namespace geometry {
			/**
			 * Defines a point representing a location in Cartesian coordinate system.
			 * @see graphics#PhysicalTwoAxesBase, graphics#PhysicalTwoAxes
			 * @addtogroup geometric_primitives
			 */
			typedef boost::geometry::model::d2::point_xy<Scalar> Point;

#ifndef ASCENSION_DOXYGEN_SHOULD_SKIP_THIS
			BOOST_PARAMETER_NAME(x)
			BOOST_PARAMETER_NAME(y)
#endif	// !ASCENSION_DOXYGEN_SHOULD_SKIP_THIS

			/**
			 * Constructs a point geometry with named parameters.
			 * @tparam Geometry
			 * @tparam Arguments
			 * @return The constructed geometry
			 */
			template<typename Geometry, typename Arguments>
			inline Geometry make(const Arguments& arguments, typename detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr) {
				return boost::geometry::make<Geometry>(arguments[_x], arguments[_y]);
			}
		}

		using geometry::Point;
	}
}

#endif // !ASCENSION_GEOMETRY_POINT_HPP
