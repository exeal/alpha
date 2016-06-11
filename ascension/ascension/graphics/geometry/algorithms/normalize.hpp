/**
 * @file normalize.hpp
 * @author exeal
 * @date 2015-10-23 Separated from algorithm.hpp.
 */

#ifndef ASCENSION_GEOMETRY_NORMALIZE_HPP
#define ASCENSION_GEOMETRY_NORMALIZE_HPP
#include <ascension/graphics/geometry/common.hpp>
#include <boost/geometry/algorithms/assign.hpp>
#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/coordinate_type.hpp>
#include <utility>

namespace ascension {
	namespace graphics {
		namespace geometry {
			/// @defgroup geometry_normalize Geometric Algorithms Related to Normalization
			/// @{

			template<typename Geometry>
			inline bool isNormalized(const Geometry& dimension, typename detail::EnableIfTagIs<Geometry, DimensionTag>::type* = nullptr) {
				return boost::geometry::get<0>(dimension) >= 0 && boost::geometry::get<1>(dimension) >= 0;
			}

			template<typename Geometry>
			inline bool isNormalized(const Geometry& rectangle, typename detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
				return boost::geometry::get<boost::geometry::min_corner, 0>(rectangle) <= boost::geometry::get<boost::geometry::max_corner, 0>(rectangle)
					&& boost::geometry::get<boost::geometry::min_corner, 1>(rectangle) <= boost::geometry::get<boost::geometry::max_corner, 1>(rectangle);
			}

			template<typename Geometry>
			inline Geometry& normalize(const Geometry& dimension, typename detail::EnableIfTagIs<Geometry, DimensionTag>::type* = nullptr) {
				if(boost::geometry::get<0>(dimension) < 0)
					boost::geometry::set<0>(dimension, -boost::geometry::get<0>(dimension));
				if(boost::geometry::get<1>(dimension) < 0)
					boost::geometry::set<1>(dimension, -boost::geometry::get<1>(dimension));
				return dimension;
			}

			template<typename Geometry>
			inline Geometry& normalize(Geometry& rectangle, typename detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
				typedef typename boost::geometry::coordinate_type<Geometry>::type Coordinate;
				Coordinate minimumX(boost::geometry::get<boost::geometry::min_corner, 0>(rectangle));
				Coordinate minimumY(boost::geometry::get<boost::geometry::min_corner, 1>(rectangle));
				Coordinate maximumX(boost::geometry::get<boost::geometry::max_corner, 0>(rectangle));
				Coordinate maximumY(boost::geometry::get<boost::geometry::max_corner, 1>(rectangle));
				if(minimumX > maximumX)
					std::swap(minimumX, maximumX);
				if(minimumY > maximumY)
					std::swap(minimumY, maximumY);
				boost::geometry::assign_values(rectangle, minimumX, minimumY, maximumX, maximumY);
				return rectangle;
			}
			/// @}
		}
	}
}

#endif // !ASCENSION_GEOMETRY_SIZE_HPP
