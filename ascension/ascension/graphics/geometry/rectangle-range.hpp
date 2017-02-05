/**
 * @file rectangle-range.hpp
 * Defines 2D rectangle types for geometry.
 * @author exeal
 * @date 2010-11-06 Created (as geometry.hpp)
 * @date 2014-09-07 Separated from geometry.hpp
 * @date 2015-10-24 Separated from rectangle.hpp
 */

#ifndef ASCENSION_GEOMETRY_RECTANGLE_RANGE_HPP
#define ASCENSION_GEOMETRY_RECTANGLE_RANGE_HPP
#include <ascension/corelib/numeric-range.hpp>
#include <ascension/graphics/geometry/common.hpp>
#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/coordinate_type.hpp>
#include <utility>	// std.begin, std.end

namespace ascension {
	namespace graphics {
		namespace geometry {
			namespace detail {
				template<typename Geometry, std::size_t dimension>
				class RectangleRangeAccessProxy {
				public:
					RectangleRangeAccessProxy(Geometry& rectangle) : rectangle_(rectangle) {}
					RectangleRangeAccessProxy& operator=(const NumericRange<typename boost::geometry::coordinate_type<Geometry>::type>& other) {
						boost::geometry::set<boost::geometry::min_corner, dimension>(rectangle_, *std::begin(other));
						boost::geometry::set<boost::geometry::max_corner, dimension>(rectangle_, *std::end(other));
						return *this;
					}
					RectangleRangeAccessProxy& operator=(const RectangleRangeAccessProxy& other) {
						return *this = range<dimension>(other.rectangle_);
					}
					template<typename OtherGeometry, std::size_t otherDimension>
					RectangleRangeAccessProxy& operator=(const RectangleRangeAccessProxy<OtherGeometry, otherDimension>& other) {
						return *this = range<otherDimension>(other.rectangle_);
					}
					operator NumericRange<typename boost::geometry::coordinate_type<Geometry>::type>() const {
						return crange<dimension>(rectangle_);
					}
				private:
					Geometry& rectangle_;
				};
			}

			template<std::size_t dimension, typename Geometry>
			inline detail::RectangleRangeAccessProxy<Geometry, dimension> range(
					Geometry& rectangle, typename detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
				return detail::RectangleRangeAccessProxy<Geometry, dimension>(rectangle);
			}

			template<std::size_t dimension, typename Geometry>
			inline NumericRange<typename boost::geometry::coordinate_type<Geometry>::type> range(
					const Geometry& rectangle, typename detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
				return nrange(
					boost::geometry::get<boost::geometry::min_corner, dimension>(rectangle),
					boost::geometry::get<boost::geometry::max_corner, dimension>(rectangle));
			}

			template<std::size_t dimension, typename Geometry>
			inline NumericRange<typename boost::geometry::coordinate_type<Geometry>::type> crange(
					const Geometry& rectangle, typename detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
				return range<dimension>(rectangle);
			}
		}
	}
}

#endif // !ASCENSION_GEOMETRY_RECTANGLE_HPP
