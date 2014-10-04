/**
 * @file rectangle.hpp
 * Defines 2D rectangle types for geometry.
 * @author exeal
 * @date 2010-11-06 Created (as geometry.hpp)
 * @date 2014-09-07 Separated from geometry.hpp
 */

#ifndef ASCENSION_GEOMETRY_RECTANGLE_HPP
#define ASCENSION_GEOMETRY_RECTANGLE_HPP

#include <ascension/graphics/geometry/dimension.hpp>
#include <ascension/graphics/geometry/point.hpp>
#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/tags.hpp>
#include <boost/operators.hpp>	// boost.equality_comparable
#include <boost/parameter.hpp>
#include <boost/range/irange.hpp>

namespace ascension {
	namespace graphics {
		namespace geometry {

			/// @addtogroup geometric_primitives
			/// @{

#ifndef ASCENSION_DOXYGEN_SHOULD_SKIP_THIS
			BOOST_PARAMETER_NAME(left)
			BOOST_PARAMETER_NAME(top)
			BOOST_PARAMETER_NAME(right)
			BOOST_PARAMETER_NAME(bottom)
#endif	// !ASCENSION_DOXYGEN_SHOULD_SKIP_THIS

			template<typename Coordinate>
			class BasicRectangleBase : private boost::equality_comparable<BasicRectangleBase<Coordinate>> {
			public:
				bool operator==(const BasicRectangleBase& other) const {
//					return boost::geometry::equals(*this, other);
					return minimumCorner_ == other.minimumCorner_ && maximumCorner_ == other.maximumCorner_;
				}
			protected:
				BasicRectangleBase() {}
				BasicRectangleBase(const BasicPoint<Coordinate>& minimumCorner, const BasicPoint<Coordinate>& maximumCorner) : minimumCorner_(minimumCorner), maximumCorner_(maximumCorner) {}
				template<typename Arguments>
				BasicRectangleBase(const Arguments& arguments) : minimumCorner_(_x = arguments[_left], _y = arguments[_top]), maximumCorner_(_x = arguments[_right], _y = arguments[_bottom]) {}
			protected:
				BasicPoint<Coordinate> minimumCorner_, maximumCorner_;
			};

			/**
			 * Describes a rectangle defined by two @c BasicPoint instances.
			 * @tparam Coordinate The coordinate type
			 * @note Some specializations of this class are registered into Boost.Geometry.
			 * @see graphics#PhysicalFourSidesBase, graphics#PhysicalFourSides
			 */
			template<typename Coordinate>
			class BasicRectangle : public BasicRectangleBase<Coordinate> {
			public:
				/// Default constructor does not initialize anything.
				BasicRectangle() {}
				/// Copy-constructor.
				BasicRectangle(const BasicRectangle& other) : BasicRectangleBase<Coordinate>((
					_left = boost::geometry::get<boost::geometry::min_corner, 0>(other), _top = boost::geometry::get<boost::geometry::min_corner, 1>(other),
					_right = boost::geometry::get<boost::geometry::max_corner, 0>(other), _bottom = boost::geometry::get<boost::geometry::max_corner, 1>(other))) {}
				/// Copy-constructor for different template parameter.
				template<typename U>
				BasicRectangle(const BasicRectangle<U>& other) : BasicRectangleBase<Coordinate>((
					_left = boost::geometry::get<boost::geometry::min_corner, 0>(other), _top = boost::geometry::get<boost::geometry::min_corner, 1>(other),
					_right = boost::geometry::get<boost::geometry::max_corner, 0>(other), _bottom = boost::geometry::get<boost::geometry::max_corner, 1>(other))) {}
				/// Copy-constructor for different rectangle type.
				template<typename Other>
				BasicRectangle(const Other& other) : BasicRectangleBase<Coordinate>((
					_left = boost::geometry::get<boost::geometry::min_corner, 0>(other), _top = boost::geometry::get<boost::geometry::min_corner, 1>(other),
					_right = boost::geometry::get<boost::geometry::max_corner, 0>(other), _bottom = boost::geometry::get<boost::geometry::max_corner, 1>(other))) {}
				/// Constructor creates a rectangle described by the given two points.
				template<typename Point1, typename Point2>
				explicit BasicRectangle(const std::pair<Point1, Point2>& points,
					typename detail::EnableIfTagIs<Point1, boost::geometry::point_tag>::type* = nullptr,
					typename detail::EnableIfTagIs<Point2, boost::geometry::point_tag>::type* = nullptr)
					: BasicRectangleBase<Coordinate>((
						_left = boost::geometry::get<0>(points.first), _top = boost::geometry::get<1>(points.first),
						_right = boost::geometry::get<0>(points.second), _bottom = boost::geometry::get<1>(points.second))) {}
				/// Constructor creates a rectangle described by the given origin and size.
				template<typename Origin, typename SizeCoordinate>
				BasicRectangle(const Origin& origin, const BasicDimension<SizeCoordinate>& size,
						typename detail::EnableIfTagIs<Origin, boost::geometry::point_tag>::type* = nullptr)
					: BasicRectangleBase<Coordinate>((
						_left = boost::geometry::get<0>(origin), _top = boost::geometry::get<1>(origin),
						_right = boost::geometry::get<0>(origin) + dx(size), _bottom = boost::geometry::get<1>(origin) + dy(size))) {}
				/// Constructor creates a rectangle described by the two ranges in x and y-coordinates.
				template<typename ScalarType>
				BasicRectangle(const boost::integer_range<ScalarType>& xrange, const boost::integer_range<ScalarType>& yrange)
					: BasicRectangleBase<Coordinate>((
						_left = *xrange.begin(), _top = *yrange.begin(),
						_right = *xrange.end(), _bottom = *yrange.end())) {}
				BOOST_PARAMETER_CONSTRUCTOR(
					BasicRectangle, (BasicRectangleBase<Coordinate>), tag,
					(required
						(left, (Coordinate))
						(top, (Coordinate))
						(right, (Coordinate))
						(bottom, (Coordinate))))

				/// Copy-assignment operator.
				BasicRectangle& operator=(const BasicRectangle& other) {
					std::swap(*this, BasicRectangle(other));
					return *this;
				}
				/// Copy-assignment operator for different template parameter.
				template<typename U>
				BasicRectangle& operator=(const BasicRectangle<U>& other) {
					std::swap(*this, BasicRectangle(other));
					return *this;
				}

			private:
				using BasicRectangleBase<Coordinate>::minimumCorner_;
				using BasicRectangleBase<Coordinate>::maximumCorner_;
				friend struct boost::geometry::traits::indexed_access<BasicRectangle<Coordinate>, boost::geometry::min_corner, 0>;
				friend struct boost::geometry::traits::indexed_access<BasicRectangle<Coordinate>, boost::geometry::min_corner, 1>;
				friend struct boost::geometry::traits::indexed_access<BasicRectangle<Coordinate>, boost::geometry::max_corner, 0>;
				friend struct boost::geometry::traits::indexed_access<BasicRectangle<Coordinate>, boost::geometry::max_corner, 1>;
			};
			/// @}
		}
	}
}

namespace boost {
	namespace geometry {
		namespace traits {			
			template<typename Coordinate>
			struct tag<ascension::graphics::geometry::BasicRectangle<Coordinate>> {
				typedef box_tag type;
			};
			template<typename Coordinate>
			struct point_type<ascension::graphics::geometry::BasicRectangle<Coordinate>> {
				typedef ascension::graphics::geometry::BasicPoint<Coordinate> type;
			};
			template <typename Coordinate, std::size_t dimension>
			struct indexed_access<ascension::graphics::geometry::BasicRectangle<Coordinate>, min_corner, dimension> {
			private:
				typedef typename coordinate_type<ascension::graphics::geometry::BasicPoint<Coordinate>>::type CoordinateType;
			public:
				static CoordinateType get(const ascension::graphics::geometry::BasicRectangle<Coordinate>& b) {
					return geometry::get<dimension>(b.minimumCorner_);
				}
				static void set(ascension::graphics::geometry::BasicRectangle<Coordinate>& b, const CoordinateType& value) {
					geometry::set<dimension>(b.minimumCorner_, value);
				}
			};
			template <typename Coordinate, std::size_t dimension>
			struct indexed_access<ascension::graphics::geometry::BasicRectangle<Coordinate>, max_corner, dimension> {
			private:
				typedef typename coordinate_type<ascension::graphics::geometry::BasicPoint<Coordinate>>::type CoordinateType;
			public:
				static CoordinateType get(const ascension::graphics::geometry::BasicRectangle<Coordinate>& b) {
					return geometry::get<dimension>(b.maximumCorner_);
				}
				static void set(ascension::graphics::geometry::BasicRectangle<Coordinate>& b, const CoordinateType& value) {
					geometry::set<dimension>(b.maximumCorner_, value);
				}
			};
		}
	}
}	// boost.geometry.traits

namespace ascension {
	namespace graphics {
		typedef geometry::BasicRectangle<Scalar> Rectangle;
	}
}

#endif // !ASCENSION_GEOMETRY_RECTANGLE_HPP
