/**
 * @file geometry.hpp
 * Defines basic data types for geometry.
 * @author exeal
 * @date 2010-11-06 created
 * @date 2014
 */

#ifndef ASCENSION_GEOMETRY_HPP
#define ASCENSION_GEOMETRY_HPP

#include <ascension/corelib/memory.hpp>	// FastArenaObject
#include <ascension/corelib/future.hpp>
#include <ascension/platforms.hpp>
#if ASCENSION_SUPPORTS_GRAPHICS_SYSTEM(CAIRO)
#	include <cairomm/types.h>	// Cairo.Rectangle, Cairo.RectangleInt
#	include <gdkmm/rectangle.h>
#	include <gdkmm/types.h>	// Gdk.Point
#endif
#if ASCENSION_SUPPORTS_GRAPHICS_SYSTEM(WIN32_GDI)
#	include <ascension/win32/handle.hpp>
#endif
#if ASCENSION_SUPPORTS_SHAPING_ENGINE(PANGO)
#	include <pangomm/rectangle.h>
#endif
#include <boost/geometry/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/register/box.hpp>
#include <boost/geometry/geometries/register/point.hpp>
#include <boost/operators.hpp>	// boost.equality_comparable
#include <boost/parameter.hpp>
#include <boost/range/irange.hpp>

namespace ascension {
	namespace graphics {
		namespace geometry {
			namespace detail {
				template<typename Geometry, typename GeometryTag, typename T = void>
				struct EnableIfTagIs : std::enable_if<
					std::is_same<
						typename boost::geometry::tag<typename std::remove_cv<Geometry>::type>::type,
						GeometryTag
					>::value, T> {};
			}

			/// @defgroup
			/// @{
			/**
			 * Converts a platform-native geometric type into a platform-independent.
			 * @tparam Geometry The return type
			 * @tparam Native The type of @a native
			 * @param native The native object to convert
			 * @return The converted platform-independent object
			 * @see #toNative
			 */
			template<typename Geometry, typename Native> inline Geometry&& fromNative(const Native& native) {
				return detail::fromNative<Geometry>(native);
			}

			/**
			 * Converts a platform-independent geometric type into a platform-native.
			 * @tparam Native The return type
			 * @tparam Geometry The type of @a g
			 * @param g The platform-independent object to convert
			 * @return The converted native object
			 * @see #fromNative
			 */
			template<typename Native, typename Geometry> inline Native&& toNative(const Geometry& g) {
				return detail::toNative(g, static_cast<const Native*>(nullptr));
			}
			/// @}

			/// @defgroup geometric_primitives Geometric Primitives
			/// Basic primitives of @c ascension#graphics#geometry.
			/// @{

			typedef float Scalar;	/// A scalar value.

#ifndef ASCENSION_DOXYGEN_SHOULD_SKIP_THIS
			BOOST_PARAMETER_NAME(x)
			BOOST_PARAMETER_NAME(y)
#endif	// !ASCENSION_DOXYGEN_SHOULD_SKIP_THIS

			template<typename Coordinate>
			class BasicPointBase : private boost::equality_comparable<BasicPointBase<Coordinate>> {
			public:
				bool operator==(const BasicPointBase& other) const {
//					return boost::geometry::equals(*this, other);
					return x_ == other.x_ && y_ == other.y_;
				}
			protected:
				BasicPointBase() {}
				template<typename Arguments>
				BasicPointBase(const Arguments& arguments) : x_(arguments[_x]), y_(arguments[_y]) {}
			protected:
				Coordinate x_, y_;
			};

			/**
			 * Defines a point representing a location in Cartesian coordinate system.
			 * @tparam Coordinate The coordinate type
			 * @note Some specializations of this class are registered into Boost.Geometry.
			 * @see graphics#PhysicalTwoAxesBase, graphics#PhysicalTwoAxes
			 */
			template<typename Coordinate>
			class BasicPoint : public BasicPointBase<Coordinate> {
			public:
				/// Default constructor does not initialize anything.
				BasicPoint() {}
				/// Copy-constructor.
				BasicPoint(const BasicPoint& other) : BasicPointBase<Coordinate>((_x = x(other), _y = y(other))) {}
				/// Copy-constructor for different template parameter.
				template<typename U>
				BasicPoint(const BasicPoint<U>& other) : BasicPointBase<Coordinate>((_x = x(other), _y = y(other))) {}
				/// Copy-constructor for different point type.
				template<typename Other>
				BasicPoint(const Other& other) : BasicPointBase<Coordinate>((_x = x(other), _y = y(other))) {}
				BOOST_PARAMETER_CONSTRUCTOR(
					BasicPoint, (BasicPointBase<Coordinate>), tag,
					(required
						(x, (Coordinate))
						(y, (Coordinate))))

				/// Copy-assignment operator.
				BasicPoint& operator=(const BasicPoint& other) {
					std::swap(*this, BasicPoint(other));
					return *this;
				}
				/// Copy-assignment operator for different template parameter.
				template<typename U>
				BasicPoint& operator=(const BasicPoint<U>& other) {
					std::swap(*this, BasicPoint(other));
					return *this;
				}

			private:
				using BasicPointBase<Coordinate>::x_;
				using BasicPointBase<Coordinate>::y_;
				friend struct boost::geometry::traits::access<BasicPoint<Coordinate>, 0>;
				friend struct boost::geometry::traits::access<BasicPoint<Coordinate>, 1>;
			};

#ifndef ASCENSION_DOXYGEN_SHOULD_SKIP_THIS
			BOOST_PARAMETER_NAME(dx)
			BOOST_PARAMETER_NAME(dy)
#endif	// !ASCENSION_DOXYGEN_SHOULD_SKIP_THIS

			template<typename Coordinate>
			class BasicDimensionBase : private boost::equality_comparable<BasicDimensionBase<Coordinate>> {
			public:
				bool operator==(const BasicDimensionBase& other) const {
					return dx_ == other.dx_ && dy_ == other.dy_;
				}
			protected:
				BasicDimensionBase() {}
				template<typename Arguments>
				BasicDimensionBase(const Arguments& arguments) : dx_(arguments[_dx]), dy_(arguments[_dy]) {}
			protected:
				Coordinate dx_, dy_;
			};

			template<typename Coordinate> class BasicDimension;
			template<typename Coordinate> Coordinate& dx(BasicDimension<Coordinate>& dimension);
			template<typename Coordinate> Coordinate dx(const BasicDimension<Coordinate>& dimension);
			template<typename Coordinate> Coordinate& dy(BasicDimension<Coordinate>& dimension);
			template<typename Coordinate> Coordinate dy(const BasicDimension<Coordinate>& dimension);

			/**
			 * Encapsulates a width and a height dimension in Cartesian coordinate system.
			 * @tparam Coordinate The coordinate type
			 */
			template<typename Coordinate>
			class BasicDimension : public BasicDimensionBase<Coordinate> {
			public:
				/// Default constructor does not initialize anything.
				BasicDimension() {}
				/// Copy-constructor.
				BasicDimension(const BasicDimension& other) : BasicDimensionBase<Coordinate>((_dx = dx(other), _dy = dy(other))) {}
				/// Copy-constructor for different template parameter.
				template<typename U>
				BasicDimension(const BasicDimension<U>& other) : BasicDimensionBase<Coordinate>((_dx = dx(other), _dy = dy(other))) {}
//				/// Copy-constructor for different dimension type.
//				template<typename Other>
//				BasicDimension(const Other& other) : BasicDimensionBase<Coordinate>((_dx = dx(other), _dy = dy(other))) {}
				BOOST_PARAMETER_CONSTRUCTOR(
					BasicDimension, (BasicDimensionBase<Coordinate>), tag,
					(required
						(dx, (Coordinate))
						(dy, (Coordinate))))

				/// Copy-assignment operator.
				BasicDimension& operator=(const BasicDimension& other) {
					std::swap(*this, BasicDimension(other));
					return *this;
				}
				/// Copy-assignment operator for different template parameter.
				template<typename U>
				BasicDimension& operator=(const BasicDimension<U>& other) {
					std::swap(*this, BasicDimension(other));
					return *this;
				}

			private:
				using BasicDimensionBase<Coordinate>::dx_;
				using BasicDimensionBase<Coordinate>::dy_;
				template<typename T> friend Coordinate& dx(BasicDimension<T>&);
				template<typename T> friend Coordinate dx(const BasicDimension<T>&);
				template<typename T> friend Coordinate& dy(BasicDimension<T>&);
				template<typename T> friend Coordinate dy(const BasicDimension<T>&);
			};

			struct DimensionTag {};

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
						_left = x(origin), _top = y(origin),
						_right = x(origin) + dx(size), _bottom = y(origin) + dy(size))) {}
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
			struct tag<ascension::graphics::geometry::BasicPoint<Coordinate>> {
				typedef point_tag type;
			};
			template<typename Coordinate>
			struct dimension<ascension::graphics::geometry::BasicPoint<Coordinate>> : boost::mpl::int_<2> {
			};
			template<typename Coordinate>
			struct coordinate_type<ascension::graphics::geometry::BasicPoint<Coordinate>> {
				typedef Coordinate type;
			};
			template<typename Coordinate>
			struct coordinate_system<ascension::graphics::geometry::BasicPoint<Coordinate>> {
				typedef cs::cartesian type;
			};
			template<typename Coordinate>
			struct access<ascension::graphics::geometry::BasicPoint<Coordinate>, 0> {
				static Coordinate get(const ascension::graphics::geometry::BasicPoint<Coordinate>& p) {
					return p.x_;
				}
				static void set(ascension::graphics::geometry::BasicPoint<Coordinate>& p, const Coordinate& value) {
					p.x_ = value;
				}
			};
			template<typename Coordinate>
			struct access<ascension::graphics::geometry::BasicPoint<Coordinate>, 1> {
				static Coordinate get(const ascension::graphics::geometry::BasicPoint<Coordinate>& p) {
					return p.y_;
				}
				static void set(ascension::graphics::geometry::BasicPoint<Coordinate>& p, const Coordinate& value) {
					p.y_ = value;
				}
			};
			
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

			template<typename Coordinate>
			struct tag<ascension::graphics::geometry::BasicDimension<Coordinate>> {
				typedef ascension::graphics::geometry::DimensionTag type;
			};
		}

		namespace core_dispatch {
			template<typename Coordinate>
			struct coordinate_type<ascension::graphics::geometry::DimensionTag, ascension::graphics::geometry::BasicDimension<Coordinate>> {
				typedef void point_type;
				typedef Coordinate type;
			};
		}
	}
}	// boost.geometry.traits

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

				template<typename Geometry, std::size_t dimension>
				class RectangleRangeProxy /*: public Range<
					typename Coordinate<
						typename Coordinate<Rectangle>::Type
					>::Type
				>*/ {
				private:
					typedef typename boost::geometry::point_type<Geometry>::type PointType;
					typedef typename boost::geometry::coordinate_type<PointType>::type CoordinateType;
				public:
					explicit RectangleRangeProxy(Geometry& rectangle) BOOST_NOEXCEPT :
						/*Range<CoordinateType>(range<dimension>(const_cast<const Geometry&>(rectangle))),*/ rectangle_(rectangle) {}
					template<typename T>
					RectangleRangeProxy<Geometry, dimension>& operator=(const boost::integer_range<T>& range) {
						boost::geometry::set<boost::geometry::min_corner, dimension>(rectangle_, *range.begin());
						boost::geometry::set<boost::geometry::max_corner, dimension>(rectangle_, *range.end());
//						Range<Scalar>::operator=(range);
						return *this;
					}
					operator boost::integer_range<CoordinateType>() const {
						return range<dimension>(const_cast<const Geometry&>(rectangle_));
					}
				private:
					Geometry& rectangle_;
				};
			}	// namespace detail

			/// @defgroup geometry_additional_aceessors Additional Access Functions
			/// @{

			/// Returns the size of the @a dimension in x-coordinate.
			template<typename Coordinate>
			inline Coordinate dx(const BasicDimension<Coordinate>& dimension) {
				return dimension.dx_;	// $friendly-access$
			}

			template<typename Coordinate>
			inline Coordinate& dx(BasicDimension<Coordinate>& dimension) {
				return dimension.dx_;	// $friendly-access$
			}

			/// Returns the size of the @a rectangle in x-coordinate.
			template<typename Geometry>
			inline typename boost::geometry::coordinate_type<Geometry>::type dx(const Geometry& rectangle, typename detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
				return boost::geometry::get<boost::geometry::max_corner, 0>(rectangle) - boost::geometry::get<boost::geometry::min_corner, 0>(rectangle);
			}

			/// Returns the size of the @a size in y-coordinate.
			template<typename Coordinate>
			inline Coordinate dy(const BasicDimension<Coordinate>& dimension) {
				return dimension.dy_;	// $friendly-access$
			}

			template<typename Coordinate>
			inline Coordinate& dy(BasicDimension<Coordinate>& dimension) {
				return dimension.dy_;	// $friendly-access$
			}

			/// Returns the size of the @a rectangle in y-coordinate.
			template<typename Geometry>
			inline typename boost::geometry::coordinate_type<Geometry>::type dy(const Geometry& rectangle, typename detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
				return boost::geometry::get<boost::geometry::max_corner, 1>(rectangle) - boost::geometry::get<boost::geometry::min_corner, 1>(rectangle);
			}

			/// Returns the origin of the @a rectangle.
			template<typename Geometry>
			inline typename boost::geometry::point_type<Geometry>::type origin(const Geometry& rectangle, typename detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
				typename boost::geometry::point_type<Geometry>::type temp;
				boost::geometry::assign_values(temp, boost::geometry::get<boost::geometry::min_corner, 0>(rectangle), boost::geometry::get<boost::geometry::min_corner, 1>(rectangle));
				return temp;
			}

			template<std::size_t dimension, typename Geometry>
			inline boost::integer_range<typename boost::geometry::coordinate_type<Geometry>::type> range(const Geometry& rectangle) {
				return boost::irange(boost::geometry::get<boost::geometry::min_corner, dimension>(rectangle), boost::geometry::get<boost::geometry::max_corner, dimension>(rectangle));
			}

			template<std::size_t dimension, typename Geometry>
			inline detail::RectangleRangeProxy<Geometry, dimension> range(Geometry& rectangle) {
				return detail::RectangleRangeProxy<Geometry, dimension>(rectangle);
			}

			/// Returns the size of the @a rectangle.
			template<typename Geometry>
			inline BasicDimension<typename boost::geometry::coordinate_type<Geometry>::type> size(const Geometry& rectangle, typename detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
				return BasicDimension<boost::geometry::coordinate_type<Geometry>::type>(_dx = dx(rectangle), _dy = dy(rectangle));
			}

			/// Returns the x-coordinate of @a point.
			template<typename Geometry>
			inline typename boost::geometry::coordinate_type<Geometry>::type x(const Geometry& point, typename detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr) {
				return boost::geometry::get<0>(point);
			}

			template<typename Geometry>
			inline detail::AccessProxy<Geometry, 0> x(Geometry& point, typename detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr) {
				return detail::AccessProxy<Geometry, 0>(point);
			}

			/// Returns the y-coordinate of @a point.
			template<typename Geometry>
			inline typename boost::geometry::coordinate_type<Geometry>::type y(const Geometry& point, typename detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr) {
				return boost::geometry::get<1>(point);
			}

			template<typename Geometry>
			inline detail::AccessProxy<Geometry, 1> y(Geometry& point, typename detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr) {
				return detail::AccessProxy<Geometry, 1>(point);
			}
			/// @}

			/// @defgroup geometry_additional_algorithms Additional Algorithms
			/// @{

			template<typename Coordinate>
			inline Coordinate area(const BasicDimension<Coordinate>& dimension) {
				return dx(dimension) * dy(dimension);
			}

			template<typename Coordinate>
			inline bool isEmpty(const BasicDimension<Coordinate>& dimension) {
				return dx(dimension) == 0 || dy(dimension) == 0;
			}

			template<typename Geometry>
			inline bool isEmpty(const Geometry& rectangle, typename detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
				return isEmpty(geometry::size(rectangle));
			}

			template<typename Coordinate>
			inline bool isNormalized(const BasicDimension<Coordinate>& dimension) {
				return dx(dimension) >= 0 && dy(dimension) >= 0;
			}

			template<typename Geometry>
			inline bool isNormalized(const Geometry& rectangle, typename detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
				return isNormalized(size(rectangle));
			}

			template<typename Rectangle>
			inline Rectangle joined(const Rectangle& r1, const Rectangle& r2, typename detail::EnableIfTagIs<Rectangle, boost::geometry::box_tag>::type* = nullptr) {
				const auto xrange1(range<0>(r1)), xrange2(range<0>(r2));
				const auto yrange1(range<1>(r1)), yrange2(range<1>(r2));
				Rectangle temp;
				boost::geometry::assign_values(temp,
					std::min(*xrange1.begin(), *xrange2.begin()), std::min(*yrange1.begin(), *yrange2.begin()),
					std::max(*xrange1.end(), *xrange2.end()), std::max(*yrange1.end(), *yrange2.end()));
				return std::move(temp);
			}

			template<typename Geometry, typename Arguments>
			inline Geometry&& make(const Arguments& arguments, typename detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr) {
				return boost::geometry::make<Geometry>(arguments[_x], arguments[_y]);
			}

			template<typename Geometry, typename Arguments>
			inline Geometry&& make(const Arguments& arguments, typename detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
				return boost::geometry::make<Geometry>(arguments[_left], arguments[_top], arguments[_right], arguments[_bottom]);
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

			template<typename Coordinate>
			inline BasicDimension<Coordinate>& normalize(BasicDimension<Coordinate>& dimension) {
				if(dx(dimension) < 0)
					dx(dimension) = -dx(dimension);
				if(dy(dimension) < 0)
					dy(dimension) = -dy(dimension);
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

			/**
			 * Returns the y-coordinate of the bottom edge of @a rectangle.
			 * @see #bottomLeft, #bottomRight, #top
			 */
			template<typename Geometry>
			inline typename boost::geometry::coordinate_type<Geometry>::type bottom(const Geometry& rectangle) {
				return std::max(
					boost::geometry::get<boost::geometry::min_corner, 1>(rectangle),
					boost::geometry::get<boost::geometry::max_corner, 1>(rectangle));
			}

			/**
			 * Returns the bottom-left corner of @a rectangle.
			 * @see #bottom, #left
			 */
			template<typename Geometry>
			inline typename boost::geometry::point_type<Geometry>::type bottomLeft(const Geometry& rectangle) {
				return boost::geometry::make<typename boost::geometry::point_type<Geometry>::type>(left(rectangle), bottom(rectangle));
			}

			/**
			 * Returns the bottom-right corner of @a rectangle.
			 * @see #bottom, #right
			 */
			template<typename Geometry>
			inline typename boost::geometry::point_type<Geometry>::type bottomRight(const Geometry& rectangle) {
				return boost::geometry::make<typename boost::geometry::point_type<Geometry>::type>(right(rectangle), bottom(rectangle));
			}

			/// Returns the Manhattan-length of @a point.
			template<typename Geometry>
			inline typename boost::geometry::coordinate_type<Geometry>::type manhattanLength(
					const Geometry& point, typename detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr) {
				return std::abs(x(point)) + std::abs(y(point));
			}

			template<typename Coordinate1, typename Coordinate2>
			inline BasicDimension<Coordinate1>& expandTo(BasicDimension<Coordinate1>&, const BasicDimension<Coordinate2>& other);

			/**
			 * Returns the x-coordinate of the left edge of @a rectangle.
			 * @see #bottomLeft, #right, #topLeft
			 */
			template<typename Geometry>
			inline typename boost::geometry::coordinate_type<Geometry>::type left(const Geometry& rectangle) {
				return std::min(
					boost::geometry::get<boost::geometry::min_corner, 0>(rectangle),
					boost::geometry::get<boost::geometry::max_corner, 0>(rectangle));
			}

			template<typename Coordinate>
			inline BasicDimension<Coordinate>& makeBoundedTo(BasicDimension<Coordinate>&, const BasicDimension<Coordinate>& other);

			template<typename Geometry, typename DimensionCoordinate>
			inline Geometry& resize(Geometry& rectangle, const BasicDimension<DimensionCoordinate>& size);

			/**
			 * Returns the x-coordinate of the right edge of @a rectangle.
			 * @see #bottomRight, #left, #topRight
			 */
			template<typename Geometry>
			inline typename boost::geometry::coordinate_type<Geometry>::type right(const Geometry& rectangle) {
				return std::max(
					boost::geometry::get<boost::geometry::min_corner, 0>(rectangle),
					boost::geometry::get<boost::geometry::max_corner, 0>(rectangle));
			}

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
			 * Returns the y-coordinate of the top edge of @a rectangle.
			 * @see #bottom, #topLeft, #topRight
			 */
			template<typename Geometry>
			inline typename boost::geometry::coordinate_type<Geometry>::type top(const Geometry& rectangle) {
				return std::min(
					boost::geometry::get<boost::geometry::min_corner, 1>(rectangle),
					boost::geometry::get<boost::geometry::max_corner, 1>(rectangle));
			}

			/**
			 * Returns the top-left corner of @a rectangle.
			 * @see #top, #left
			 */
			template<typename Geometry>
			inline typename boost::geometry::point_type<Geometry>::type topLeft(const Geometry& rectangle) {
				return boost::geometry::make<typename boost::geometry::point_type<Geometry>::type>(left(rectangle), top(rectangle));
			}

			/**
			 * Returns the top-right corner of @a rectangle.
			 * @see #top, #right
			 */
			template<typename Geometry>
			inline typename boost::geometry::point_type<Geometry>::type topRight(const Geometry& rectangle) {
				return boost::geometry::make<typename boost::geometry::point_type<Geometry>::type>(right(rectangle), top(rectangle));
			}

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

			// platform-dependent conversions
			namespace detail {
#if ASCENSION_SUPPORTS_GRAPHICS_SYSTEM(CAIRO)
				template<typename Geometry>
				inline Geometry&& fromNative(const Gdk::Point& native, typename detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr) {
					return make<Geometry>((_x = native.get_x(), _y = native.get_y()));
				}
				template<typename Geometry>
				inline Geometry&& fromNative(const Gdk::Rectangle& native, typename detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
					return make<Geometry>((_left = native.get_x(), _top = native.get_y(), _right = native.get_x() /*+ native.get_width()*/, _bottom = native.get_y() /*+ native.get_height()*/));
				}
				template<typename Geometry>
				inline Geometry&& fromNative(const Cairo::Rectangle& native, typename detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
					return make<Geometry>((_left = native.x, _top = native.y, _right = native.x + native.width, _bottom = native.y + native.height));
				}
				template<typename Geometry>
				inline Geometry&& fromNative(const Cairo::RectangleInt& native, typename detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
					return make<Geometry>((_left = native.x, _top = native.y, _right = native.x + native.width, _bottom = native.y + native.height));
				}

				template<typename Geometry>
				inline Gdk::Point&& toNative(const Geometry& g, const Gdk::Point* = nullptr, typename detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr) {
					return Gdk::Point(static_cast<int>(x(g)), static_cast<int>(y(g)));
				}
				template<typename Geometry>
				inline Gdk::Rectangle&& toNative(const Geometry& g, const Gdk::Rectangle* = nullptr, typename detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
					return Gdk::Rectangle(static_cast<int>(left(g)), static_cast<int>(top(g)), static_cast<int>(dx(g)), static_cast<int>(dy(g)));
				}
				template<typename Geometry>
				inline Cairo::Rectangle&& toNative(const Geometry& g, const Cairo::Rectangle* = nullptr, typename detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
					Cairo::Rectangle native;
					native.x = left(g);
					native.y = top(g);
					native.width = dx(g);
					native.height = dy(g);
					return std::move(native);
				}
				template<typename Geometry>
				inline Cairo::RectangleInt&& toNative(const Geometry& g, const Cairo::RectangleInt* = nullptr, typename detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
					Cairo::RectangleInt native;
					native.x = static_cast<int>(left(g));
					native.y = static_cast<int>(top(g));
					native.width = static_cast<int>(dx(g));
					native.height = static_cast<int>(dy(g));
					return std::move(native);
				}
#endif
#if ASCENSION_SUPPORTS_GRAPHICS_SYSTEM(CORE_GRAPHICS)
#endif
#if ASCENSION_SUPPORTS_GRAPHICS_SYSTEM(QT)
#endif
#if ASCENSION_SUPPORTS_GRAPHICS_SYSTEM(WIN32_GDI)
				template<typename Geometry>
				inline Geometry&& fromNative(const COORD& native, typename detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr) {
					return make<Geometry>((_x = native.x, _y = native.y));
				}
				template<typename Geometry>
				inline Geometry&& fromNative(const POINT& native, typename detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr) {
					return make<Geometry>((_x = native.x, _y = native.y));
				}
				template<typename Geometry>
				inline Geometry&& fromNative(const POINTL& native, typename detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr) {
					return make<Geometry>((_x = native.x, _y = native.y));
				}
				template<typename Geometry>
				inline Geometry&& fromNative(const POINTS& native, typename detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr) {
					return make<Geometry>((_x = native.x, _y = native.y));
				}
				template<typename Geometry>
				inline Geometry&& fromNative(const SIZE& native, typename detail::EnableIfTagIs<Geometry, DimensionTag>::type* = nullptr) {
					return Geometry(_dx = native.cx, _dy = native.cy);
				}
				template<typename Geometry>
				inline Geometry&& fromNative(const RECT& native, typename detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
					return make<Geometry>((_left = native.left, _top = native.top, _right = native.right, _bottom = native.bottom));
				}
				template<typename Geometry>
				inline Geometry&& fromNative(const RECTL& native, typename detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
					return make<Geometry>((_left = native.left, _top = native.top, _right = native.right, _bottom = native.bottom));
				}
				template<typename Geometry>
				inline Geometry&& fromNative(const SMALL_RECT& native, typename detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
					return make<Geometry>((_left = native.left, _top = native.top, _right = native.right, _bottom = native.bottom));
				}

				template<typename Geometry>
				inline COORD&& toNative(const Geometry& g, const COORD* = nullptr, typename detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr) {
					COORD native;
					native.X = static_cast<SHORT>(x(g));
					native.Y = static_cast<SHORT>(y(g));
					return std::move(native);
				}
				template<typename Geometry>
				inline POINT&& toNative(const Geometry& g, const POINT* = nullptr, typename detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr) {
					POINT native;
					native.x = static_cast<LONG>(x(g));
					native.y = static_cast<LONG>(y(g));
					return std::move(native);
				}
				template<typename Geometry>
				inline POINTL&& toNative(const Geometry& g, const POINTL* = nullptr, typename detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr) {
					POINTL native;
					native.x = static_cast<LONG>(x(g));
					native.y = static_cast<LONG>(y(g));
					return std::move(native);
				}
				template<typename Geometry>
				inline POINTS&& toNative(const Geometry& g, const POINTS* = nullptr, typename detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr) {
					POINTS native;
					native.x = static_cast<SHORT>(x(g));
					native.y = static_cast<SHORT>(y(g));
					return std::move(native);
				}

				template<typename Geometry>
				inline SIZE&& toNative(const Geometry& g, const SIZE* = nullptr, typename detail::EnableIfTagIs<Geometry, DimensionTag>::type* = nullptr) {
					SIZE native;
					native.cx = static_cast<LONG>(dx(g));
					native.cy = static_cast<LONG>(dy(g));
					return std::move(native);
				}

				template<typename Geometry>
				inline RECT&& toNative(const Geometry& g, const RECT* = nullptr, typename detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
					RECT native;
					native.left = static_cast<LONG>(left(g));
					native.top = static_cast<LONG>(top(g));
					native.right = static_cast<LONG>(right(g));
					native.bottom = static_cast<LONG>(bottom(g));
					return std::move(native);
				}
				template<typename Geometry>
				inline RECTL&& toNative(const Geometry& g, const RECTL* = nullptr, typename detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
					RECTL native;
					native.left = static_cast<LONG>(left(g));
					native.top = static_cast<LONG>(top(g));
					native.right = static_cast<LONG>(right(g));
					native.bottom = static_cast<LONG>(bottom(g));
					return std::move(native);
				}
				template<typename Geometry>
				inline SMALL_RECT&& toNative(const Geometry& g, const SMALL_RECT* = nullptr, typename detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
					SMALL_RECT native;
					native.Left = static_cast<SHORT>(left(g));
					native.Top = static_cast<SHORT>(top(g));
					native.Right = static_cast<SHORT>(right(g));
					native.Bottom = static_cast<SHORT>(bottom(g));
					return std::move(native);
				}
#endif
#if ASCENSION_SUPPORTS_SHAPING_ENGINE(PANGO)
				template<typename Geometry>
				inline Geometry&& fromNative(const Pango::Rectangle& native, typename detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
					return make<Geometry>((_left = native.get_x(), _top = native.get_y(), _right = native.get_x() + native.get_width(), _bottom = native.get_y() + native.get_height()));
				}
				template<typename Geometry>
				inline Pango::Rectangle&& toNative(const Geometry& g, const Pango::Rectangle* = nullptr, typename detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
					return Pango::Rectangle(left(g), top(g), dx(g), dy(g));
				}
#endif
#if ASCENSION_SUPPORTS_GRAPHICS_SYSTEM(WIN32_GDIPLUS)
#endif
			}
		}

		/// @name 
		/// @{
		using geometry::Scalar;
		typedef geometry::BasicPoint<Scalar> Point;
		typedef geometry::BasicDimension<Scalar> Dimension;
		typedef geometry::BasicRectangle<Scalar> Rectangle;
		/// @}
	}
}

#endif // !ASCENSION_GEOMETRY_HPP
