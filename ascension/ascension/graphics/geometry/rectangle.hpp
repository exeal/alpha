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
#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/tags.hpp>
#include <boost/operators.hpp>	// boost.equality_comparable
#include <boost/parameter.hpp>
#include <utility>	// std.pair

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
			
			template<typename Coordinate> class BasicRectangleBase;
			template<std::size_t dimension, typename Coordinate>
			NumericRange<Coordinate>& range(BasicRectangleBase<Coordinate>& rectangle);
			template<std::size_t dimension, typename Coordinate>
			const NumericRange<Coordinate>& range(const BasicRectangleBase<Coordinate>& rectangle);
			template<std::size_t dimension, typename Coordinate>
			const NumericRange<Coordinate>& crange(const BasicRectangleBase<Coordinate>& rectangle);

			template<typename Coordinate>
			class BasicRectangleBase : private boost::equality_comparable<BasicRectangleBase<Coordinate>> {
			public:
				bool operator==(const BasicRectangleBase& other) const {
//					return boost::geometry::equals(*this, other);
					return ranges_ == other.ranges_;
				}
			protected:
				BasicRectangleBase() {}
				BasicRectangleBase(const BasicRectangleBase& other) : ranges_(other.ranges_) {}
				template<typename OtherCoordinate>
				BasicRectangleBase(const BasicRectangleBase<OtherCoordinate>& other) : ranges_(other.ranges_) {}
				BasicRectangleBase(BasicRectangleBase&& other) : ranges_(std::move(other.ranges_)) {}
				template<typename OtherCoordinate>
				BasicRectangleBase(BasicRectangleBase<OtherCoordinate>&& other) : ranges_(std::move(other.ranges_)) {}
				BasicRectangleBase(const std::pair<NumericRange<Coordinate>, NumericRange<Coordinate>>& ranges) : ranges_(ranges) {}
				template<typename OtherCoordinate>
				BasicRectangleBase(const std::pair<NumericRange<OtherCoordinate>, NumericRange<OtherCoordinate>>& ranges) : ranges_(ranges) {}
				template<typename Arguments>
				BasicRectangleBase(const Arguments& arguments) :
					ranges_(nrange(arguments[_left], arguments[_right]), nrange(arguments[_top], arguments[_bottom])) {}
			private:
				template<typename OtherCoordinate>
				friend class BasicRectangleBase;
				template<std::size_t dimension, typename Coordinate>
				friend NumericRange<Coordinate>& range(BasicRectangleBase<Coordinate>& rectangle);
				template<std::size_t dimension, typename Coordinate>
				friend const NumericRange<Coordinate>& range(const BasicRectangleBase<Coordinate>& rectangle);
				template<std::size_t dimension, typename Coordinate>
				friend const NumericRange<Coordinate>& crange(const BasicRectangleBase<Coordinate>& rectangle);
				std::pair<NumericRange<Coordinate>, NumericRange<Coordinate>> ranges_;
			};

			template<std::size_t dimension, typename Coordinate>
			inline NumericRange<Coordinate>& range(BasicRectangleBase<Coordinate>& rectangle) {
				return std::get<dimension>(rectangle.ranges_);
			}

			template<std::size_t dimension, typename Coordinate>
			inline const NumericRange<Coordinate>& range(const BasicRectangleBase<Coordinate>& rectangle) {
				return std::get<dimension>(rectangle.ranges_);
			}

			template<std::size_t dimension, typename Coordinate>
			inline const NumericRange<Coordinate>& crange(const BasicRectangleBase<Coordinate>& rectangle) {
				return std::get<dimension>(rectangle.ranges_);
			}

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
				/**
				 * Copy-constructor.
				 * @param other The source object
				 */
				BasicRectangle(const BasicRectangle& other) : BasicRectangleBase(static_cast<const BasicRectangleBase&>(other)) {}
				/**
				 * Copy-constructor for different coordinate type.
				 * @tparam OtherCoordinate The coordinate type of @a other
				 * @param other The source object
				 */
				template<typename OtherCoordinate>
				BasicRectangle(const BasicRectangle<OtherCoordinate>& other) :
					BasicRectangleBase<Coordinate>(static_cast<const BasicRectangleBase<OtherCoordinate>&>(other)) {}
				/**
				 * Copy-constructor for different rectangle type.
				 * @tparam Geometry The type of @a other
				 * @param other The source rectangular geometry
				 */
				template<typename Geometry>
				BasicRectangle(const Geometry& other, typename detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) :
					BasicRectangleBase<Coordinate>((
						_left = boost::geometry::get<boost::geometry::min_corner, 0>(other),
						_top = boost::geometry::get<boost::geometry::min_corner, 1>(other),
						_right = boost::geometry::get<boost::geometry::max_corner, 0>(other),
						_bottom = boost::geometry::get<boost::geometry::max_corner, 1>(other))) {}
				/**
				 * Constructs a @c BasicRectangle with two numeric ranges.
				 * @param ranges The pair gives the two ranges. The @c first element is the range in x-coordinate. The
				 *               @c second element is the range in y-coordinate
				 */
				BasicRectangle(const std::pair<NumericRange<Coordinate>, NumericRange<Coordinate>>& ranges) : BasicRectangleBase(ranges) {}
				/**
				 * Constructs a @c BasicRectangle with two numeric ranges.
				 * @tparam OtherCoordinate The coordinate type of @a ranges
				 * @param ranges The pair gives the two ranges. The @c first element is the range in x-coordinate. The
				 *               @c second element is the range in y-coordinate
				 */
				template<typename OtherCoordinate>
				BasicRectangle(const std::pair<NumericRange<OtherCoordinate>, NumericRange<OtherCoordinate>>& ranges) : BasicRectangleBase(ranges) {}
				/**
				 * Constructor creates a rectangle described by the given two points.
				 * @tparam Point1 The type of the @c first element of @a points
				 * @tparam Point2 The type of the @c second element of @a points
				 * @param points The pair gives the two corners of the new rectangle
				 */
				template<typename Point1, typename Point2>
				explicit BasicRectangle(const std::pair<Point1, Point2>& points,
					typename detail::EnableIfTagIs<Point1, boost::geometry::point_tag>::type* = nullptr,
					typename detail::EnableIfTagIs<Point2, boost::geometry::point_tag>::type* = nullptr)
					: BasicRectangleBase<Coordinate>(
						std::make_pair(
							nrange(boost::geometry::get<0>(points.first), boost::geometry::get<0>(points.second)),
							nrange(boost::geometry::get<1>(points.first), boost::geometry::get<1>(points.second)))) {}
				/**
				 * Constructor creates a rectangle described by the given origin and size.
				 * @tparam Origin The type of @a origin
				 * @tparam SizeCoordinate The type of @a size
				 * @param origin The origin point of the new rectangle
				 * @param size The size dimension of the new rectangle
				 */
				template<typename Origin, typename SizeCoordinate>
				BasicRectangle(const Origin& origin, const BasicDimension<SizeCoordinate>& size,
						typename detail::EnableIfTagIs<Origin, boost::geometry::point_tag>::type* = nullptr)
					: BasicRectangleBase<Coordinate>(
						std::make_pair(
							nrange<Coordinate>(boost::geometry::get<0>(origin), boost::geometry::get<0>(origin) + dx(size)),
							nrange<Coordinate>(boost::geometry::get<1>(origin), boost::geometry::get<1>(origin) + dy(size)))) {}
				/// Constructor takes named parameters.
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
				static CoordinateType get(const ascension::graphics::geometry::BasicRectangle<Coordinate>& box) {
					return *boost::const_begin(ascension::graphics::geometry::crange<dimension>(box));
				}
				static void set(ascension::graphics::geometry::BasicRectangle<Coordinate>& box, const CoordinateType& value) {
//					*boost::begin(ascension::graphics::geometry::range<dimension>(box)) = value;
					ascension::graphics::geometry::range<dimension>(box).front() = value;
				}
			};

			template <typename Coordinate, std::size_t dimension>
			struct indexed_access<ascension::graphics::geometry::BasicRectangle<Coordinate>, max_corner, dimension> {
			private:
				typedef typename coordinate_type<ascension::graphics::geometry::BasicPoint<Coordinate>>::type CoordinateType;
			public:
				static CoordinateType get(const ascension::graphics::geometry::BasicRectangle<Coordinate>& box) {
					return *boost::const_end(ascension::graphics::geometry::crange<dimension>(box));
				}
				static void set(ascension::graphics::geometry::BasicRectangle<Coordinate>& box, const CoordinateType& value) {
					*boost::end(ascension::graphics::geometry::range<dimension>(box)) = value;
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
