/**
 * @file make.hpp
 * Defines @c ascension#graphics#geometry#make free functions.
 * @author exeal
 * @date 2015-10-24 Separated and joined from point.hpp, dimension.hpp and rectangle.hpp.
 */

#ifndef ASCENSION_GEOMETRY_MAKE_HPP
#define ASCENSION_GEOMETRY_MAKE_HPP
#include <ascension/graphics/geometry/common.hpp>
#include <ascension/graphics/geometry/named-parameters.hpp>
#include <boost/geometry/algorithms/make.hpp>
#include <boost/geometry/core/access.hpp>
#include <utility>	// std.begin, std.end
#include <utility>	// std.pair

namespace ascension {
	namespace graphics {
		namespace geometry {
			/**
			 * Constructs a point geometry with named parameters.
			 * @tparam Geometry The point type
			 * @tparam Arguments The type of @a arguments
			 * @param arguments The named parameters: x and y
			 * @return The constructed geometry
			 */
			template<typename Geometry, typename Arguments>
			inline Geometry make(const Arguments& arguments, typename detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr) {
				return boost::geometry::make<Geometry>(arguments[_x], arguments[_y]);
			}

			/**
			 * Constructs a rectangular geometry with named parameters.
			 * @tparam Geometry
			 * @tparam Arguments
			 * @return The constructed geometry
			 */
			template<typename Geometry, typename Arguments>
			inline Geometry make(const Arguments& arguments, typename detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
				// TODO: boost::geometry::make function which takes four arguments is undocumented.
				return boost::geometry::make<Geometry>(arguments[_left], arguments[_top], arguments[_right], arguments[_bottom]);
			}


			/**
			 * Constructs a rectangular geometry with two numeric ranges.
			 * @tparam Geometry
			 * @tparam Range The types of @a ranges.first @a ranges.second
			 * @param ranges The pair gives the two ranges. The @c first element is the range in x-coordinate. The
			 *               @c second element is the range in y-coordinate
			 * @return The constructed geometry
			 */
			template<typename Geometry, typename Range1, typename Range2>
			inline Geometry make(const std::pair<Range1, Range2>& ranges,
					typename detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr,
					typename detail::DisableIfTagIs<Range1, boost::geometry::point_tag>::type* = nullptr,
					typename detail::DisableIfTagIs<Range2, boost::geometry::point_tag>::type* = nullptr) {
				return make<Geometry>((
					_left = *std::begin(std::get<0>(ranges)), _top = *std::begin(std::get<1>(ranges)),
					_right = *std::end(std::get<0>(ranges)), _bottom = *std::end(std::get<1>(ranges))));
			}

			/**
			 * Constructs a rectangular with the given two points.
			 * @tparam Geometry
			 * @tparam Point1 The type of the @c first element of @a points
			 * @tparam Point2 The type of the @c second element of @a points
			 * @param points The pair gives the two corners of the new rectangle
			 * @return The constructed geometry
			 */
			template<typename Geometry, typename Point1, typename Point2>
			inline Geometry make(const std::pair<Point1, Point2>& points,
					typename detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr,
					typename detail::EnableIfTagIs<Point1, boost::geometry::point_tag>::type* = nullptr,
					typename detail::EnableIfTagIs<Point2, boost::geometry::point_tag>::type* = nullptr) {
				return make<Geometry>((
					_left = boost::geometry::get<0>(std::get<0>(points)), _top = boost::geometry::get<1>(std::get<0>(points)),
					_right = boost::geometry::get<0>(std::get<1>(points)), _bottom = boost::geometry::get<1>(std::get<1>(points))));
			}

			/**
			 * Constructs a rectangular geometry with the given origin and size.
			 * @tparam Geometry
			 * @tparam Origin The type of @a origin
			 * @tparam Size The type of @a size
			 * @param origin The origin point of the new rectangle
			 * @param size The size dimension of the new rectangle
			 * @return The constructed geometry
			 */
			template<typename Geometry, typename Origin, typename Size>
			inline Geometry make(const Origin& origin, const Size& size,
					typename detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr,
					typename detail::EnableIfTagIs<Origin, boost::geometry::point_tag>::type* = nullptr,
					typename detail::EnableIfTagIs<Size, DimensionTag>::type* = nullptr) {
				return make<Geometry>((
					_left = boost::geometry::get<0>(origin), _top = boost::geometry::get<1>(origin),
					_right = boost::geometry::get<0>(origin) + dx(size), _bottom = boost::geometry::get<1>(origin) + dy(size)));
			}
		}
	}
}

#endif // !ASCENSION_GEOMETRY_MAKE_HPP
