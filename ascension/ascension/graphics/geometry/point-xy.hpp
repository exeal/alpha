/**
 * @file point-xy.hpp
 * Defines the additional accessors for 2D point geometry.
 * @author exeal
 * @date 2010-11-06 Created (as geometry.hpp)
 * @date 2014-09-07 Separated from geometry.hpp
 * @date 2015-10-24 Separated from algorithm.hpp
 */

#ifndef ASCENSION_GEOMETRY_POINT_XY_HPP
#define ASCENSION_GEOMETRY_POINT_XY_HPP
#include <ascension/graphics/geometry/common.hpp>
#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/coordinate_type.hpp>

namespace ascension {
	namespace graphics {
		namespace geometry {
			namespace detail {
				template<typename Geometry, std::size_t dimension>
				class AccessProxy {
				public:
					typedef typename boost::geometry::coordinate_type<Geometry>::type CoordinateType;
				public:
					explicit AccessProxy(Geometry& geometry) BOOST_NOEXCEPT : geometry_(geometry) {}
					const AccessProxy<Geometry, dimension>& operator=(CoordinateType value) {
						boost::geometry::set<dimension>(geometry_, value);
						return *this;
					}
					const AccessProxy& operator=(const AccessProxy& other) {
						return *this = static_cast<CoordinateType>(other);
					}
					operator CoordinateType() const {
						return boost::geometry::get<dimension>(geometry_);
					};
					CoordinateType operator+() const {return +static_cast<CoordinateType>(*this);}
					CoordinateType operator-() const {return -static_cast<CoordinateType>(*this);}
					AccessProxy<Geometry, dimension>& operator+=(CoordinateType other) {*this = *this + other; return *this;}
					AccessProxy<Geometry, dimension>& operator-=(CoordinateType other) {*this = *this - other; return *this;}
					AccessProxy<Geometry, dimension>& operator*=(CoordinateType other) {*this = *this * other; return *this;}
					AccessProxy<Geometry, dimension>& operator/=(CoordinateType other) {*this = *this / other; return *this;}
//					AccessProxy<Geometry, dimension>& operator%=(CoordinateType other) {*this = *this % other; return *this;}
				private:
					Geometry& geometry_;
				};
			}	// namespace detail

			/// @addtogroup geometry_additional_accessors
			/// @{

			/// Returns the x-coordinate of @a point.
			template<typename Geometry>
			inline typename boost::geometry::coordinate_type<Geometry>::type x(const Geometry& point, typename detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr) {
				return boost::geometry::get<0>(point);
			}

			/// Returns the x-coordinate of @a point.
			template<typename Geometry>
			inline detail::AccessProxy<Geometry, 0> x(Geometry& point, typename detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr) {
				return detail::AccessProxy<Geometry, 0>(point);
			}

			/// Returns the y-coordinate of @a point.
			template<typename Geometry>
			inline typename boost::geometry::coordinate_type<Geometry>::type y(const Geometry& point, typename detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr) {
				return boost::geometry::get<1>(point);
			}

			/// Returns the y-coordinate of @a point.
			template<typename Geometry>
			inline detail::AccessProxy<Geometry, 1> y(Geometry& point, typename detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr) {
				return detail::AccessProxy<Geometry, 1>(point);
			}
			/// @}
		}
	}
}

#endif // !ASCENSION_GEOMETRY_POINT_XY_HPP
