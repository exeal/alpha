/**
 * @file algorithm.hpp
 * Defines algorithms for geometry.
 * @author exeal
 * @date 2010-11-06 Created (as geometry.hpp)
 * @date 2014-09-07 Separated from geometry.hpp
 */

#ifndef ASCENSION_GEOMETRY_ALGORITHM_HPP
#define ASCENSION_GEOMETRY_ALGORITHM_HPP

#include <ascension/corelib/future.hpp>
#include <ascension/corelib/numeric-range.hpp>
#include <ascension/graphics/geometry/dimension.hpp>
#include <boost/geometry/algorithms/assign.hpp>
#include <boost/geometry/algorithms/make.hpp>
#include <boost/geometry/algorithms/transform.hpp>
#include <boost/geometry/strategies/transform/matrix_transformers.hpp>
#include <boost/parameter.hpp>
#include <boost/range/irange.hpp>

namespace ascension {
	namespace graphics {
		namespace geometry {

			/// @defgroup geometry_additional_aceessors Additional Access Functions
			/// @{
			/// @}

			/// @defgroup geometry_additional_algorithms Additional Algorithms
			/// @{

			template<typename Rectangle>
			inline Rectangle joined(const Rectangle& r1, const Rectangle& r2, typename detail::EnableIfTagIs<Rectangle, boost::geometry::box_tag>::type* = nullptr) {
				const auto xrange1(range<0>(r1)), xrange2(range<0>(r2));
				const auto yrange1(range<1>(r1)), yrange2(range<1>(r2));
				Rectangle temp;
				boost::geometry::assign_values(temp,
					std::min(*xrange1.begin(), *xrange2.begin()), std::min(*yrange1.begin(), *yrange2.begin()),
					std::max(*xrange1.end(), *xrange2.end()), std::max(*yrange1.end(), *yrange2.end()));
				return temp;
			}


			template<typename Geometry>
			inline Geometry& negate(Geometry& point, typename detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr) {
				x(point) = -x(point);
				y(point) = -y(point);
				return point;
			}

			template<typename Coordinate>
			inline BasicDimension<Coordinate>& negate(BasicDimension<Coordinate>& dimension) {
				dx(dimension) = -dx(dimension);
				dy(dimension) = -dy(dimension);
				return dimension;
			}

			template<typename Point, typename Rectangle>
			inline bool within(const Point& point, const Rectangle& rectangle,
					typename detail::EnableIfTagIs<Point, boost::geometry::point_tag>::type* = nullptr,
					typename detail::EnableIfTagIs<Rectangle, boost::geometry::box_tag>::type* = nullptr) {
				return boost::geometry::get<0>(point) >= boost::geometry::get<boost::geometry::min_corner, 0>(rectangle)
					&& boost::geometry::get<0>(point) < boost::geometry::get<boost::geometry::max_corner, 0>(rectangle)
					&& boost::geometry::get<1>(point) >= boost::geometry::get<boost::geometry::min_corner, 1>(rectangle)
					&& boost::geometry::get<1>(point) < boost::geometry::get<boost::geometry::max_corner, 1>(rectangle);
			}
			/// @}

			// fundamental operations

#if 0
			template<typename Size>
			inline Size& add(Size& size1, const Size& size2, typename detail::EnableIfTagIs<Size, SizeTag>::type* = nullptr) {
				dx(size1) += dx(size2);
				dy(size1) += dy(size2);
				return size1;
			}

			template<typename Size>
			inline Size& subtract(Size& size1, const Size& size2, typename detail::EnableIfTagIs<Size, SizeTag>::type* = nullptr) {
				dx(size1) -= dx(size2);
				dy(size1) -= dy(size2);
				return size1;
			}
#endif
			template<typename Geometry, typename DimensionCoordinate>
			inline Geometry& translate(Geometry& g, const BasicDimension<DimensionCoordinate>& offset) {
				typedef typename boost::geometry::point_type<Geometry>::type PointType;
				boost::geometry::transform(g, g, boost::geometry::strategy::transform::translate_transformer<DimensionCoordinate, 2, 2>(dx(offset), dy(offset)));
				return g;
			}

			// writing to standard output stream

			template<typename Geometry, typename Character, typename CharacterTraits>
			inline std::basic_ostream<Character, CharacterTraits>&
					write(std::basic_ostream<Character, CharacterTraits>& out, const Geometry& point, const boost::geometry::point_tag&) {
				const std::ctype<Character>& ct = std::use_facet<std::ctype<Character>>(out.getloc());
				return out << x(point) << ct.widen(',') << y(point);
			}

			template<typename Geometry, typename Character, typename CharacterTraits>
			inline std::basic_ostream<Character, CharacterTraits>&
					write(std::basic_ostream<Character, CharacterTraits>& out, const Geometry& rectangle, const boost::geometry::box_tag&) {
				const std::ctype<Character>& ct = std::use_facet<std::ctype<Character>>(out.getloc());
				return out << origin(rectangle) << ct.widen(' ') << size(rectangle);
			}

			template<typename Geometry, typename Character, typename CharacterTraits>
			inline std::basic_ostream<Character, CharacterTraits>&
					operator<<(std::basic_ostream<Character, CharacterTraits>& out, const Geometry& geometry) {
				return write(out, geometry, typename boost::geometry::tag<Geometry>::type());
			}

			template<typename Coordinate, typename Character, typename CharacterTraits>
			inline std::basic_ostream<Character, CharacterTraits>&
					write(std::basic_ostream<Character, CharacterTraits>& out, const BasicDimension<Coordinate>& dimension) {
				const std::ctype<Character>& ct = std::use_facet<std::ctype<Character>>(out.getloc());
				return out << dx(dimension) << ct.widen('x') << dy(dimension);
			}

			// special operations

			/// Returns the Manhattan-length of @a point.
			template<typename Geometry>
			inline typename boost::geometry::coordinate_type<Geometry>::type manhattanLength(
					const Geometry& point, typename detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr) {
				return std::abs(x(point)) + std::abs(y(point));
			}

			template<typename Coordinate1, typename Coordinate2>
			inline BasicDimension<Coordinate1>& expandTo(BasicDimension<Coordinate1>&, const BasicDimension<Coordinate2>& other);

			template<typename Coordinate>
			inline BasicDimension<Coordinate>& makeBoundedTo(BasicDimension<Coordinate>&, const BasicDimension<Coordinate>& other);

			template<typename Geometry, typename DimensionCoordinate>
			inline Geometry& resize(Geometry& rectangle, const BasicDimension<DimensionCoordinate>& size);

			/**
			 * Scales this object to the given rectangle, preserving aspect ratio.
			 * @code
			 * Dimension<int> d1(20, 30);
			 * d2.scale(Dimension<int>(60, 60), false); // -> 40x60
			 * Dimension<int> d2(20, 30);
			 * d1.scale(Dimension<int>(60, 60), true);  // -> 60x90
			 * @endcode
			 * @param size
			 * @param keepAspectRatioByExpanding If @c true, this dimension is scaled to a
			 *                                   rectangle as small as possible outside @a size.
			 *                                   If @c false, this dimension is scaled to a
			 *                                   rectangle as large as possible inside @a size
			 * @return This object
			 */
			template<typename Coordinate>
			BasicDimension<Coordinate>& scale(const BasicDimension<Coordinate>& dimension, bool keepAspectRatioByExpanding);

			/**
			 * Swaps the x and y values.
			 * @return @
			 */
			template<typename Geometry>
			inline Geometry& transpose(Geometry& point, typename detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr) {
				boost::geometry::assign_values<Geometry, typename boost::geometry::coordinate_type<Geometry>::type>(point, y(point), x(point));
				return point;
			}

			/**
			 * Swaps the dx and dy values.
			 * @return @
			 */
			template<typename Coordinate>
			inline BasicDimension<Coordinate>& transpose(BasicDimension<Coordinate>& dimension) {
				return dimension = BasicDimension<Coordinate>(_dx = dy(dimension), _dy = dx(dimension));
			}
		}
	}
}

#endif // !ASCENSION_GEOMETRY_HPP
