/**
 * @file geometry.hpp
 * Defines basic data types for geometry.
 * @author exeal
 * @date 2010-11-06 created
 */

#ifndef ASCENSION_GEOMETRY_HPP
#define ASCENSION_GEOMETRY_HPP

#include <ascension/corelib/memory.hpp>	// FastArenaObject
#include <ascension/corelib/future.hpp>
#include <ascension/platforms.hpp>
#if defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI)
#	include <ascension/win32/handle.hpp>
#elif defined(ASCENSION_GRAPHICS_SYSTEM_CAIRO)
#	include <gdk/gdk.h>
#endif
#include <boost/geometry/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/register/box.hpp>
#include <boost/geometry/geometries/register/point.hpp>
#include <boost/parameter.hpp>
#include <boost/range/irange.hpp>

namespace ascension {
	namespace detail {
		template<typename Geometry, typename GeometryTag, typename T = void>
		struct EnableIfTagIs : std::enable_if<
			std::is_same<
				typename boost::geometry::tag<typename std::remove_cv<Geometry>::type>::type,
				GeometryTag
			>::value, T> {};
	}

	namespace graphics {
		namespace geometry {
			/// @name Geometric Primitives
			/// @{

			typedef float Scalar;

#ifndef ASCENSION_DOXYGEN_SHOULD_SKIP_THIS
			BOOST_PARAMETER_NAME(x)
			BOOST_PARAMETER_NAME(y)
#endif	// !ASCENSION_DOXYGEN_SHOULD_SKIP_THIS

			template<typename Coordinate>
			class BasicPointBase {
			protected:
				BasicPointBase() {}
				template<typename Arguments>
				BasicPointBase(const Arguments& arguments) : x_(arguments[_x]), y_(arguments[_y]) {}
			protected:
				Coordinate x_, y_;
			};

			template<typename Coordinate>
			class BasicPoint : public BasicPointBase<Coordinate> {
			public:
				BasicPoint() {}
				template<typename Other>
				BasicPoint(const Other& other) : BasicPointBase<Coordinate>((_x = boost::geometry::get<0>(other), _y = boost::geometry::get<1>(other))) {}
				BOOST_PARAMETER_CONSTRUCTOR(
					BasicPoint, (BasicPointBase<Coordinate>), tag,
					(required
						(x, (Coordinate))
						(y, (Coordinate))))
#if defined(ASCENSION_GRAPHICS_SYSTEM_CAIRO)
#elif defined(ASCENSION_GRAPHICS_SYSTEM_CORE_GRAPHICS)
#elif defined(ASCENSION_GRAPHICS_SYSTEM_QT)
#elif defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI)
				BasicPoint(const POINT& nativeObject);
				BasicPoint(const POINTL& nativeObject);
				BasicPoint(const POINTS& nativeObject);
				operator POINT() const;
				operator POINTL() const;
				operator POINTS() const;
#else
#endif
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
			class BasicDimensionBase {
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

			template<typename Coordinate>
			class BasicDimension : public BasicDimensionBase<Coordinate> {
			public:
				BasicDimension() {}
//				template<typename Other>
//				BasicDimension(const Other& other) : BasicDimensionBase<Coordinate>((_dx = dx(other), _dy = dy(other))) {}
				BOOST_PARAMETER_CONSTRUCTOR(
					BasicDimension, (BasicDimensionBase<Coordinate>), tag,
					(required
						(dx, (Coordinate))
						(dy, (Coordinate))))
#if defined(ASCENSION_GRAPHICS_SYSTEM_CAIRO)
#elif defined(ASCENSION_GRAPHICS_SYSTEM_CORE_GRAPHICS)
#elif defined(ASCENSION_GRAPHICS_SYSTEM_QT)
#elif defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI)
				BasicDimension(const SIZE& nativeObject);
				operator SIZE() const;
#else
#endif
			private:
				using BasicDimensionBase<Coordinate>::dx_;
				using BasicDimensionBase<Coordinate>::dy_;
				friend Coordinate& dx(BasicDimension<Coordinate>&);
				friend Coordinate dx(const BasicDimension<Coordinate>&);
				friend Coordinate& dy(BasicDimension<Coordinate>&);
				friend Coordinate dy(const BasicDimension<Coordinate>&);
			};

			struct DimensionTag {};

#ifndef ASCENSION_DOXYGEN_SHOULD_SKIP_THIS
			BOOST_PARAMETER_NAME(left)
			BOOST_PARAMETER_NAME(top)
			BOOST_PARAMETER_NAME(right)
			BOOST_PARAMETER_NAME(bottom)
#endif	// !ASCENSION_DOXYGEN_SHOULD_SKIP_THIS

			template<typename Coordinate>
			class BasicRectangleBase {
			protected:
				BasicRectangleBase() {}
				template<typename Arguments>
				BasicRectangleBase(const Arguments& arguments) : minimumCorner_(_x = arguments[_left], _y = arguments[_top]), maximumCorner_(_x = arguments[_right], _y = arguments[_bottom]) {}
			protected:
				BasicPoint<Coordinate> minimumCorner_, maximumCorner_;
			};

			template<typename Coordinate>
			class BasicRectangle : public BasicRectangleBase<Coordinate> {
			public:
				BasicRectangle() {}
				template<typename Point1, typename Point2>
				explicit BasicRectangle(const std::pair<Point1, Point2>& points,
					typename detail::EnableIfTagIs<Point1, boost::geometry::point_tag>::type* = nullptr,
					typename detail::EnableIfTagIs<Point2, boost::geometry::point_tag>::type* = nullptr)
					: BasicRectangleBase<Coordinate>((
						_left = boost::geometry::get<0>(points.first), _top = boost::geometry::get<1>(points.first),
						_right = boost::geometry::get<0>(points.second), _bottom = boost::geometry::get<1>(points.second))) {}
				template<typename Origin, typename SizeCoordinate>
				BasicRectangle(const Origin& origin, const BasicDimension<SizeCoordinate>& size,
					typename detail::EnableIfTagIs<Origin, boost::geometry::point_tag>::type* = nullptr);
				template<typename ScalarType>
				BasicRectangle(const boost::integer_range<ScalarType>& xrange, const boost::integer_range<ScalarType>& yrange)
					: BasicRectangleBase<Coordinate>((
						_left = *xrange.begin(), _top = *yrange.begin(),
						_right = *xrange.end(), _bottom = *yrange.end())) {}
				template<typename Other>
				BasicRectangle(const Other& other) : BasicRectangleBase<Coordinate>((_dx = dx(other), _dy = dy(other))) {}
				BOOST_PARAMETER_CONSTRUCTOR(
					BasicRectangle, (BasicRectangleBase<Coordinate>), tag,
					(required
						(left, (Coordinate))
						(top, (Coordinate))
						(right, (Coordinate))
						(bottom, (Coordinate))))
#if defined(ASCENSION_GRAPHICS_SYSTEM_CAIRO)
#elif defined(ASCENSION_GRAPHICS_SYSTEM_CORE_GRAPHICS)
#elif defined(ASCENSION_GRAPHICS_SYSTEM_QT)
#elif defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI)
				BasicRectangle(const RECT& nativeObject);
				BasicRectangle(const RECTL& nativeObject);
				BasicRectangle(const SMALL_RECT& nativeObject);
				operator RECT() const;
				operator RECTL() const;
				operator SMALL_RECT() const;
#else
#endif
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

#ifndef ASCENSION_DOXYGEN_SHOULD_SKIP_THIS
BOOST_GEOMETRY_REGISTER_POINT_2D(
	ascension::graphics::geometry::BasicPoint<ascension::graphics::geometry::Scalar>,
	ascension::graphics::geometry::Scalar, boost::geometry::cs::cartesian, x_, y_)
BOOST_GEOMETRY_REGISTER_POINT_2D(
	ascension::graphics::geometry::BasicPoint<std::int16_t>,
	std::int16_t, boost::geometry::cs::cartesian, x_, y_)
BOOST_GEOMETRY_REGISTER_POINT_2D(
	ascension::graphics::geometry::BasicPoint<std::uint16_t>,
	std::uint16_t, boost::geometry::cs::cartesian, x_, y_)
BOOST_GEOMETRY_REGISTER_BOX(
	ascension::graphics::geometry::BasicRectangle<ascension::graphics::geometry::Scalar>,
	ascension::graphics::geometry::BasicPoint<ascension::graphics::geometry::Scalar>, minimumCorner_, maximumCorner_)
//BOOST_GEOMETRY_REGISTER_BOX_TEMPLATED(
//	ascension::graphics::geometry::BasicRectangle, minimumCorner_, maximumCorner_)
#endif	// !ASCENSION_DOXYGEN_SHOULD_SKIP_THIS

namespace boost {
	namespace geometry {
		namespace traits {
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
}

namespace ascension {
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
//			AccessProxy<Geometry, dimension>& operator%=(CoordinateType other) {*this = *this % other; return *this;}
		private:
			Geometry& geometry_;
		};

		template<typename Geometry, std::size_t dimension>
		class RectangleRangeProxy /*: public Range<
			typename graphics::geometry::Coordinate<
				typename graphics::geometry::Coordinate<Rectangle>::Type
			>::Type
		>*/ {
		private:
			typedef typename boost::geometry::point_type<Geometry>::type PointType;
			typedef typename boost::geometry::coordinate_type<PointType>::type CoordinateType;
		public:
			explicit RectangleRangeProxy(Geometry& rectangle) BOOST_NOEXCEPT :
				/*Range<CoordinateType>(graphics::geometry::range<dimension>(const_cast<const Geometry&>(rectangle))),*/ rectangle_(rectangle) {}
			template<typename T>
			RectangleRangeProxy<Geometry, dimension>& operator=(const boost::integer_range<T>& range) {
				boost::geometry::set<boost::geometry::min_corner, dimension>(rectangle_, *range.begin());
				boost::geometry::set<boost::geometry::max_corner, dimension>(rectangle_, *range.end());
//				Range<Scalar>::operator=(range);
				return *this;
			}
		private:
			Geometry& rectangle_;
		};
	}

	namespace graphics {
		namespace geometry {
			/// @name Additional Access Functions
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

			/// @name Additional Algorithms
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
				return isEmpty(size(rectangle));
			}

			template<typename Coordinate>
			inline bool isNormalized(const BasicDimension<Coordinate>& dimension) {
				return dx(dimension) >= 0 && dy(dimension) >= 0;
			}

			template<typename Geometry>
			inline bool isNormalized(const Geometry& rectangle, typename detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
				return isNormalized(size(rectangle));
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
			// 'translate' for point, rectangle and region

			template<typename Geometry, typename DimensionCoordinate>
			inline Geometry& translate(Geometry& point, const BasicDimension<DimensionCoordinate>& offset,
					typename detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr) {
				x(point) += dx(offset);
				y(point) += dy(offset);
				return point;
			}

			template<typename Geometry, typename DimensionCoordinate>
			inline Geometry& translate(Geometry& rectangle, const BasicDimension<DimensionCoordinate>& offset,
					typename detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
				boost::geometry::point_type<Geometry>::type o(origin(rectangle));
				translate(o, offset);
				boost::geometry::set<boost::geometry::min_corner, 0>(rectangle, x(o));
				boost::geometry::set<boost::geometry::min_corner, 1>(rectangle, y(o));
				return rectangle;
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
