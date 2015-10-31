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
#if 0
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
#endif
		}
	}
}

#endif // !ASCENSION_GEOMETRY_HPP
