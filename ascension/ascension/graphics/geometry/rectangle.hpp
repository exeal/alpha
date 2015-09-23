/**
 * @file rectangle.hpp
 * Defines 2D rectangle types for geometry.
 * @author exeal
 * @date 2010-11-06 Created (as geometry.hpp)
 * @date 2014-09-07 Separated from geometry.hpp
 */

#ifndef ASCENSION_GEOMETRY_RECTANGLE_HPP
#define ASCENSION_GEOMETRY_RECTANGLE_HPP
#include <ascension/corelib/numeric-range.hpp>
#include <ascension/graphics/geometry/dimension.hpp>
#include <ascension/graphics/geometry/point.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/parameter.hpp>
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

#ifndef ASCENSION_DOXYGEN_SHOULD_SKIP_THIS
			BOOST_PARAMETER_NAME(left)
			BOOST_PARAMETER_NAME(top)
			BOOST_PARAMETER_NAME(right)
			BOOST_PARAMETER_NAME(bottom)
#endif	// !ASCENSION_DOXYGEN_SHOULD_SKIP_THIS

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
			 * @tparam Coordinate The coordinate type of @a ranges
			 * @param ranges The pair gives the two ranges. The @c first element is the range in x-coordinate. The
			 *               @c second element is the range in y-coordinate
			 * @return The constructed geometry
			 */
			template<typename Geometry, typename Coordinate>
			inline Geometry make(const std::pair<NumericRange<Coordinate>, NumericRange<Coordinate>>& ranges,
					typename detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
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
			 * @tparam SizeCoordinate The type of @a size
			 * @param origin The origin point of the new rectangle
			 * @param size The size dimension of the new rectangle
			 * @return The constructed geometry
			 */
			template<typename Geometry, typename Origin, typename SizeCoordinate>
			inline Geometry make(const Origin& origin, const BasicDimension<SizeCoordinate>& size,
					typename detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr,
					typename detail::EnableIfTagIs<Origin, boost::geometry::point_tag>::type* = nullptr) {
				return make<Geometry>((
					_left = boost::geometry::get<0>(origin), _top = boost::geometry::get<1>(origin),
					_right = boost::geometry::get<0>(origin) + dx(size), _bottom = boost::geometry::get<1>(origin) + dy(size)));
			}

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

		using geometry::Rectangle;
	}
}

#endif // !ASCENSION_GEOMETRY_RECTANGLE_HPP
