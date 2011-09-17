/**
 * @file geometry.hpp
 * Defines basic data types for geometry.
 * @author exeal
 * @date 2010-11-06 created
 */

#ifndef ASCENSION_GEOMETRY_HPP
#define ASCENSION_GEOMETRY_HPP

#include <ascension/corelib/memory.hpp>	// FastArenaObject
#include <ascension/corelib/range.hpp>
#include <ascension/corelib/type-traits.hpp>
#include <ascension/platforms.hpp>
#if defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI)
#	include <ascension/win32/windows.hpp>
#elif defined(ASCENSION_GRAPHICS_SYSTEM_CAIRO)
#	include <gdk/gdk.h>
#endif

namespace ascension {

	namespace graphics {

		template<typename T>
		struct PhysicalFourSides {
			T left, top, right, bottom;
		};

		namespace geometry {
			template<typename Geometry> struct Coordinate;
			template<typename Geometry> struct Tag;

			// Tags define geometry types.

			/// Point identifying tag.
			struct PointTag {};
			/// Size identifying tag.
			struct SizeTag {};
			/// Rectangle identifying tag.
			struct RectangleTag {};

			const std::size_t X_COORDINATE = 0;
			const std::size_t Y_COORDINATE = 1;

/*
	A point represents a point in the (x, y) coordinate plane.
	A size represents the size of a two-dimensional primitive.
 */

			namespace traits {
				template<typename Tag, typename Geometry>
				struct Maker {};
				template<typename Tag, typename Geometry, std::size_t dimension>
				struct Accessor {};
			}

#if defined(ASCENSION_GRAPHICS_SYSTEM_CAIRO)
			typedef double Scalar;
			typedef GdkRectangle NativeRectangle;
			typedef cairo_region_t* NativePolygon;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_CORE_GRAPHICS)
#elif defined(ASCENSION_GRAPHICS_SYSTEM_QT)
#elif defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI)
			namespace nativetypes {
				typedef int Scalar;
				typedef POINT NativePoint;
				typedef SIZE NativeSize;
				typedef RECT NativeRectangle;
				typedef win32::Handle<HRGN> NativeRegion;
			}

			template<> struct Coordinate<POINT> {typedef LONG Type;};
			template<> struct Coordinate<POINTS> {typedef SHORT Type;};
			template<> struct Coordinate<SIZE> {typedef LONG Type;};
			template<> struct Coordinate<RECT> {typedef POINT Type;};
			template<> struct Coordinate<RECTL> {typedef POINT Type;};
			template<> struct Coordinate<SMALL_RECT> {typedef POINTS Type;};
			template<> struct Tag<POINT> {typedef PointTag Type;};
			template<> struct Tag<POINTS> {typedef PointTag Type;};
			template<> struct Tag<SIZE> {typedef SizeTag Type;};
			template<> struct Tag<RECT> {typedef RectangleTag Type;};
			template<> struct Tag<RECTL> {typedef RectangleTag Type;};
			template<> struct Tag<SMALL_RECT> {typedef RectangleTag Type;};

			namespace traits {
				template<typename Point> struct Maker<PointTag, Point> {
					static Point make(typename Coordinate<Point>::Type x, typename Coordinate<Point>::Type y) {
						const Point temp = {x, y};
						return temp;
					}
				};
				template<typename Size> struct Maker<SizeTag, Size> {
					static Size make(typename Coordinate<Size>::Type dx, typename Coordinate<Size>::Type dy) {
						const Size temp = {dx, dy};
						return temp;
					}
				};
				template<typename Rectangle> struct Maker<RectangleTag, Rectangle> {
					template<typename Size>
					static Rectangle make(const typename Coordinate<Rectangle>::Type& origin, const Size& size,
							typename std::tr1::enable_if<std::tr1::is_same<typename Tag<Size>::Type, SizeTag>::value>::type* = 0) {
						const Rectangle temp = {origin.x, origin.y, origin.x + size.cx, origin.y + size.cy};
						return temp;
					}
					template<typename Point>
					static Rectangle make(const typename Coordinate<Rectangle>::Type& first, const Point& second,
							typename std::tr1::enable_if<std::tr1::is_same<typename Tag<Point>::Type, PointTag>::value>::type* = 0) {
						const Rectangle temp = {first.x, first.y, second.x, second.y};
						return temp;
					}
				};
				template<typename Point> struct Accessor<PointTag, Point, 0> {
					static typename Coordinate<Point>::Type get(const Point& geometry) {return geometry.x;}
					static void set(Point& geometry, typename Coordinate<Point>::Type value) {geometry.x = value;}
				};
				template<typename Point> struct Accessor<PointTag, Point, 1> {
					static typename Coordinate<Point>::Type get(const Point& geometry) {return geometry.y;}
					static void set(Point& geometry, typename Coordinate<Point>::Type value) {geometry.y = value;}
				};
				template<typename Size> struct Accessor<SizeTag, Size, 0> {
					static typename Coordinate<Size>::Type get(const Size& geometry) {return geometry.cx;}
					static void set(Size& geometry, typename Coordinate<Size>::Type value) {geometry.cx = value;}
				};
				template<typename Size> struct Accessor<SizeTag, Size, 1> {
					static typename Coordinate<Size>::Type get(const Size& geometry) {return geometry.cy;}
					static void set(Size& geometry, typename Coordinate<Size>::Type value) {geometry.cy = value;}
				};
				template<typename Rectangle> struct Accessor<RectangleTag, Rectangle, 0> {
					static typename Coordinate<Rectangle>::Type get(const Rectangle& geometry) {
						return make<typename Coordinate<Rectangle>::Type>(geometry.left, geometry.top);
					}
					static void set(Rectangle& geometry, const typename Coordinate<Rectangle>::Type& value) {
						geometry.left = value.x;
						geometry.top = value.y;
					}
				};
				template<typename Rect> struct Accessor<RectangleTag, Rect, 1> {
					static typename Coordinate<Rect>::Type get(const Rect& geometry) {
						return make<typename Coordinate<Rect>::Type>(geometry.right, geometry.bottom);
					}
					static void set(Rect& geometry, const typename Coordinate<Rect>::Type& value) {
						geometry.right = value.x;
						geometry.bottom = value.y;
					}
				};
			}
#else
#endif
		}
	}

	namespace detail {
		template<typename Geometry, typename GeometryTag, typename T = void>
		class EnableIfTagIs : std::tr1::enable_if<
			std::tr1::is_same<
				typename graphics::geometry::Tag<typename std::tr1::remove_cv<Geometry>::type>::Type,
				GeometryTag
			>::value, T> {};

		template<typename Geometry, std::size_t dimension>
		class AccessProxy {
		public:
			typedef typename graphics::geometry::Coordinate<Geometry>::Type CoordinateType;
		public:
			explicit AccessProxy(Geometry& geometry) /*throw()*/ : geometry_(geometry) {}
			const AccessProxy<Geometry, dimension>& operator=(CoordinateType value) {
				graphics::geometry::set<dimension>(geometry_, value);
				return *this;
			}
			const AccessProxy& operator=(const AccessProxy& other) {
				return *this = static_cast<CoordinateType>(other);
			}
			operator CoordinateType() const {
				return graphics::geometry::get<dimension>(geometry_);
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

		template<typename Rectangle, std::size_t dimension>
		class RectangleRangeProxy {
		public:
			explicit RectangleRangeProxy(Rectangle& rectangle) /*throw()*/ : rectangle_(rectangle) {}
			template<typename T>
			RectangleRangeProxy<Rectangle, dimension>& operator=(const Range<T>& range) {
				typename graphics::geometry::Coordinate<Rectangle>::Type
					b(graphics::geometry::get<0>(rectangle)), e(graphics::geometry::get<1>(rectangle));
				graphics::geometry::set<dimension>(b, range.beginning());
				graphics::geometry::set<dimension>(e, range.end());
				graphics::geometry::set<0>(rectangle_, b);
				graphics::geometry::set<1>(rectangle_, e);
				return *this;
			}
			operator const Range<typename graphics::geometry::Coordinate<typename graphics::geometry::Coordinate<Rectangle>::Type>::Type>() const {
				return graphics::geometry::range<dimension>(rectangle_);
			}
		private:
			Rectangle& rectangle_;
		};
	}

	namespace graphics {

		using namespace geometry::nativetypes;

		namespace geometry {

			// fundamental operations

			template<typename Geometry0, typename Geometry1, typename Geometry2>
			inline Geometry0 make(const Geometry1& geometry1, const Geometry2& geometry2) {
//					typename detail::Select<std::tr1::is_scalar<Geometry1>::value, Geometry1,
//						typename std::tr1::add_const<typename std::tr1::add_reference<Geometry1>::type>::type>::Type geometry1,
//					typename detail::Select<std::tr1::is_scalar<Geometry2>::value, Geometry2,
//						typename std::tr1::add_const<typename std::tr1::add_reference<Geometry2>::type>::type>::Type geometry2) {
				return traits::Maker<typename Tag<Geometry0>::Type, Geometry0>::make(geometry1, geometry2);
			}

			template<typename Rectangle>
			inline Rectangle make(
					const Range<typename Coordinate<typename Coordinate<Rectangle>::Type>::Type>& xrange,
					const Range<typename Coordinate<typename Coordinate<Rectangle>::Type>::Type>& yrange,
					typename detail::EnableIfTagIs<Rectangle, RectangleTag>::type* = 0) {
				return make<Rectangle>(
					make<Coordinate<Rectangle>::Type>(xrange.beginning(), yrange.beginning()),
					make<Coordinate<Rectangle>::Type>(xrange.end(), yrange.end()));
			}

			template<std::size_t dimension, typename Geometry>
			inline typename Coordinate<Geometry>::Type get(const Geometry& geometry) {
				return traits::Accessor<typename Tag<Geometry>::Type, Geometry, dimension>::get(geometry);
			}

			template<std::size_t dimension, typename Geometry>
			inline void set(Geometry& geometry, typename Coordinate<Geometry>::Type value) {
				traits::Accessor<typename Tag<Geometry>::Type, Geometry, dimension>::set(geometry, value);
			}

			// 'add' for point and size

			template<typename Point>
			inline Point& add(Point& point1, const Point& point2, typename detail::EnableIfTagIs<Point, PointTag>::type* = 0) {
				x(point1) += x(point2);
				y(point1) += y(point2);
				return point1;
			}

			template<typename Size>
			inline Size& add(Size& size1, const Size& size2, typename detail::EnableIfTagIs<Size, SizeTag>::type* = 0) {
				dx(size1) += dx(size2);
				dy(size1) += dy(size2);
				return size1;
			}

			// 'divide' for point and size

			// 'dx' for size and rectangle

			/// Returns the size of the @a size in x-coordinate.
			template<typename Size>
			inline typename Coordinate<Size>::Type dx(const Size& size, typename detail::EnableIfTagIs<Size, SizeTag>::type* = 0) {
				return get<X_COORDINATE>(size);
			}

			template<typename Size>
			inline detail::AccessProxy<Size, X_COORDINATE> dx(Size& size, typename detail::EnableIfTagIs<Size, SizeTag>::type* = 0) {
				return detail::AccessProxy<Size, 0>(size);
			}

			/// Returns the size of the @a rectangle in x-coordinate.
			template<typename Rectangle>
			inline typename Coordinate<typename Coordinate<Rectangle>::Type>::Type dx(const Rectangle& rectangle, typename detail::EnableIfTagIs<Rectangle, RectangleTag>::type* = 0) {
				return dx(size(rectangle));
			}

			// 'dy' for size and rectangle

			/// Returns the size of the @a size in y-coordinate.
			template<typename Size>
			inline typename Coordinate<Size>::Type dy(const Size& size, typename detail::EnableIfTagIs<Size, SizeTag>::type* = 0) {
				return get<Y_COORDINATE>(size);
			}

			template<typename Size>
			inline detail::AccessProxy<Size, Y_COORDINATE> dy(Size& size, typename detail::EnableIfTagIs<Size, SizeTag>::type* = 0) {
				return detail::AccessProxy<Size, 1>(size);
			}

			/// Returns the size of the @a rectangle in x-coordinate.
			template<typename Rectangle>
			inline typename Coordinate<typename Coordinate<Rectangle>::Type>::Type dy(const Rectangle& rectangle, typename detail::EnableIfTagIs<Rectangle, RectangleTag>::type* = 0) {
				return dy(size(rectangle));
			}

			// 'equals'

			template<typename Geometry>
			inline bool equals(const Geometry& geometry1, const Geometry& geometry2,
					typename std::tr1::enable_if<std::tr1::is_arithmetic<typename Coordinate<Geometry>::Type>::value>::type* = 0) {
				return get<0>(geometry1) == get<0>(geometry2) && get<1>(geometry1) == get<1>(geometry2);
			}

			template<typename Geometry>
			inline bool equals(const Geometry& geometry1, const Geometry& geometry2,
					typename std::tr1::enable_if<!std::tr1::is_arithmetic<typename Coordinate<Geometry>::Type>::value>::type* = 0) {
				return equals(get<0>(geometry1), get<0>(geometry2)) && equals(get<1>(geometry1), get<1>(geometry2));
			}

			// 'intersected' for rectangle and region

			template<typename Rectangle1, typename Rectangle2>
			inline Rectangle1 intersected(const Rectangle1& rectangle1, const Rectangle2& rectangle2,
					typename detail::EnableIfTagIs<Rectangle1, RectangleTag>::type* = 0,
					typename detail::EnableIfTagIs<Rectangle2, RectangleTag>::type* = 0) {
				return make<Rectangle1>(
					ascension::intersected(range<X_COORDINATE>(rectangle1), range<X_COORDINATE>(rectangle2)),
					ascension::intersected(range<Y_COORDINATE>(rectangle1), range<Y_COORDINATE>(rectangle2)));
			}

			// 'intersects' for rectangle and region

			template<typename Rectangle1, typename Rectangle2>
			inline bool _intersects(const Rectangle1& rectangle1, const Rectangle2& rectangle2, const RectangleTag&, const RectangleTag&) {
				return ascension::intersects(range<X_COORDINATE>(rectangle1), range<X_COORDINATE>(rectangle2))
					&& ascension::intersects(range<Y_COORDINATE>(rectangle1), range<Y_COORDINATE>(rectangle2));
			}

			template<typename Geometry1, typename Geometry2>
			inline bool intersects(const Geometry1& geometry1, const Geometry2& geometry2) {
				return _intersects(geometry1, geometry2, Tag<Geometry1>::Type(), Tag<Geometry2>::Type());
			}

			// 'includes' for rectangle and region

			template<typename Rectangle, typename Point>
			inline bool includes(const Rectangle& rectangle, const Point& point,
					typename detail::EnableIfTagIs<Rectangle, RectangleTag>::type* = 0,
					typename detail::EnableIfTagIs<Point, PointTag>::type* = 0) {
				return ascension::includes(makeRange(left(rectangle), right(rectangle)), x(point))
					&& ascension::includes(makeRange(top(rectangle), bottom(rectangle)), y(point));
			}

			template<typename Rectangle1, typename Rectangle2>
			inline bool includes(const Rectangle1& rectangle1, const Rectangle2& rectangle2,
					typename detail::EnableIfTagIs<Rectangle1, RectangleTag>::type* = 0,
					typename detail::EnableIfTagIs<Rectangle2, RectangleTag>::type* = 0) {
				return ascension::includes(range<X_COORDINATE>(rectangle1), range<X_COORDINATE>(rectangle2))
					&& ascension::includes(range<Y_COORDINATE>(rectangle1), range<Y_COORDINATE>(rectangle2));
			}

			// 'isEmpty' for size, rectangle and region

			template<typename Size>
			inline bool isEmpty(const Size& size, typename detail::EnableIfTagIs<Size, SizeTag>::type* = 0) {
				return dx(size) <= 0 || dy(size) <= 0;
			}

			template<typename Rectangle>
			inline bool isEmpty(const Rectangle& rectangle, typename detail::EnableIfTagIs<Rectangle, RectangleTag>::type* = 0) {
				return isEmpty(size(rectangle));
			}

			// 'isNormalized' for size and rectangle

			template<typename Size>
			inline bool isNormalized(const Size& size, typename detail::EnableIfTagIs<Size, SizeTag>::type* = 0) {
				return dx(size) >= 0 && dy(size) >= 0;
			}

			template<typename Rectangle>
			inline bool isNormalized(const Rectangle& rectangle, typename detail::EnableIfTagIs<Rectangle, RectangleTag>::type* = 0) {
				return isNormalized(size(rectangle));
			}

			// 'multiply' for size

			// 'negate' for point and size

			template<typename Point>
			inline Point& negate(Point& point, typename detail::EnableIfTagIs<Point, PointTag>::type* = 0) {
				x(point) = -x(point);
				y(point) = -y(point);
				return point;
			}

			template<typename Size>
			inline Size& negate(Size& size, typename detail::EnableIfTagIs<Size, SizeTag>::type* = 0) {
				dx(size) = -dx(size);
				dy(size) = -dy(size);
				return size;
			}

			// 'normalize' for size and rectangle

			template<typename Size>
			inline Size& normalize(Size& size, typename detail::EnableIfTagIs<Size, SizeTag>::type* = 0) {
				if(dx(size) < 0)
					dx(size) = -dx(size);
				if(dy(size) < 0)
					dy(size) = -dy(size);
				return size;
			}

			template<typename Rectangle>
			inline Rectangle& normalize(Rectangle& rectangle, typename detail::EnableIfTagIs<Rectangle, RectangleTag>::type* = 0) {
				typedef typename Coordinate<typename Coordinate<Rectangle>::Type>::Type Scalar;
				std::pair<Scalar, Scalar> minimumCorner(x(get<0>(rectangle)), y(get<0>(rectangle)));
				std::pair<Scalar, Scalar> maximumCorner(x(get<1>(rectangle)), y(get<1>(rectangle)));
				if(minimumCorner.first > maximumCorner.first)
					std::swap(minimumCorner.first, maximumCorner.first);
				if(minimumCorner.second > maximumCorner.second)
					std::swap(minimumCorner.second, maximumCorner.second);
				return rectangle = make<Rectangle>(
					make<Coordinate<Rectangle>::Type>(minimumCorner.first, minimumCorner.second),
					make<Coordinate<Rectangle>::Type>(maximumCorner.first, maximumCorner.second));
			}

			// 'subtract' for point and size

			template<typename Point>
			inline Point& subtract(Point& point1, const Point& point2, typename detail::EnableIfTagIs<Point, PointTag>::type* = 0) {
				x(point1) -= x(point2);
				y(point1) -= y(point2);
				return point1;
			}

			template<typename Size>
			inline Size& subtract(Size& size1, const Size& size2, typename detail::EnableIfTagIs<Size, SizeTag>::type* = 0) {
				dx(size1) -= dx(size2);
				dy(size1) -= dy(size2);
				return size1;
			}

			// 'translate' for point, rectangle and region

			template<typename Point, typename Size>
			inline Point& translate(Point& p, const Size& offset, typename detail::EnableIfTagIs<Point, PointTag>::type* = 0) {
				x(p) += dx(offset);
				y(p) += dy(offset);
				return p;
			}

			template<typename Rectangle, typename Offset>
			inline Rectangle& translate(Rectangle& rectangle, const Offset& offset, typename detail::EnableIfTagIs<Rectangle, RectangleTag>::type* = 0) {
				Coordinate<Rectangle>::Type p[2] = {get<0>(rectangle), get<1>(rectangle)};
				return rectangle = make<Rectangle>(translate(p[0], offset), translate(p[1], offset));
			}

			// 'united' for ...

			// 'x' for point and rectangle

			/// Returns the x-coordinate of @a point.
			template<typename Point>
			inline typename Coordinate<Point>::Type x(const Point& p, typename detail::EnableIfTagIs<Point, PointTag>::type* = 0) {
				return get<X_COORDINATE>(p);
			}

			template<typename Point>
			inline detail::AccessProxy<Point, 0> x(Point& p, typename detail::EnableIfTagIs<Point, PointTag>::type* = 0) {
				return detail::AccessProxy<Point, X_COORDINATE>(p);
			}

			// 'y' for point and rectangle

			/// Returns the y-coordinate of @a point.
			template<typename Point>
			inline typename Coordinate<Point>::Type y(const Point& p, typename detail::EnableIfTagIs<Point, PointTag>::type* = 0) {
				return get<Y_COORDINATE>(p);
			}

			template<typename Point>
			inline detail::AccessProxy<Point, 1> y(Point& p, typename detail::EnableIfTagIs<Point, PointTag>::type* = 0) {
				return detail::AccessProxy<Point, Y_COORDINATE>(p);
			}

			// writing to standard output stream

			template<typename Point, typename Character, typename CharacterTraits>
			inline std::basic_ostream<Character, CharacterTraits>&
					write(std::basic_ostream<Character, CharacterTraits>& out, const Point& point, const PointTag&) {
				const std::ctype<Character>& ct = std::use_facet<std::ctype<Character> >(out.getloc());
				return out << x(point) << ct.widen(',') << y(point);
			}

			template<typename Size, typename Character, typename CharacterTraits>
			inline std::basic_ostream<Character, CharacterTraits>&
					write(std::basic_ostream<Character, CharacterTraits>& out, const Size& size, const SizeTag&) {
				const std::ctype<Character>& ct = std::use_facet<std::ctype<Character> >(out.getloc());
				return out << dx(size) << ct.widen('x') << dy(size);
			}

			template<typename Rectangle, typename Character, typename CharacterTraits>
			inline std::basic_ostream<Character, CharacterTraits>&
					write(std::basic_ostream<Character, CharacterTraits>& out, const Rectangle& rectangle, const RectangleTag&) {
				const std::ctype<Character>& ct = std::use_facet<std::ctype<Character> >(out.getloc());
				return out << origin(rectangle) << ct.widen(' ') << size(rectangle);
			}

			template<typename Geometry, typename Character, typename CharacterTraits>
			inline std::basic_ostream<Character, CharacterTraits>&
					operator<<(std::basic_ostream<Character, CharacterTraits>& out, const Geometry& geometry) {
				return write(out, geometry, typename Tag<Geometry>::Type());
			}

			// special operations

			/**
			 * Returns the y-coordinate of the bottom edge of @a rectangle.
			 * @see #bottomLeft, #bottomRight, #top
			 */
			template<typename Rectangle>
			inline typename Coordinate<typename Coordinate<Rectangle>::Type>::Type bottom(const Rectangle& rectangle) {
				return std::max(y(get<0>(rectangle)), y(get<1>(rectangle)));
			}

			/**
			 * Returns the bottom-left corner of @a rectangle.
			 * @see #bottom, #left
			 */
			template<typename Rectangle>
			inline typename Coordinate<Rectangle>::Type bottomLeft(const Rectangle& rectangle) {
				return make<typename Coordinate<Rectangle>::Type>(left(rectangle), bottom(rectangle));
			}

			/**
			 * Returns the bottom-right corner of @a rectangle.
			 * @see #bottom, #right
			 */
			template<typename Rectangle>
			inline typename Coordinate<Rectangle>::Type bottomRight(const Rectangle& rectangle) {
				return make<typename Coordinate<Rectangle>::Type>(right(rectangle), bottom(rectangle));
			}

			/// Returns the Manhattan-length of @a point.
			template<typename Point>
			inline typename Coordinate<Point>::Type manhattanLength(const Point& p, typename detail::EnableIfTagIs<Point, PointTag>::type* = 0) {
				return std::abs(x(p)) + std::abs(y(p));
			}

			template<typename Size>
			inline Size& expandTo(Size&, const Size& other, typename detail::EnableIfTagIs<Size, SizeTag>::type* = 0);

			/**
			 * Returns the x-coordinate of the left edge of @a rectangle.
			 * @see #bottomLeft, #right, #topLeft
			 */
			template<typename Rectangle>
			inline typename Coordinate<typename Coordinate<Rectangle>::Type>::Type left(const Rectangle& rectangle) {
				return std::min(x(get<0>(rectangle)), x(get<1>(rectangle)));
			}

			template<typename Size>
			inline Size& makeBoundedTo(Size&, const Size& other, typename detail::EnableIfTagIs<Size, SizeTag>::type* = 0);

			template<std::size_t dimension, typename Rectangle>
			inline const Range<typename Coordinate<typename Coordinate<Rectangle>::Type>::Type> range(const Rectangle& rectangle) {
				return makeRange(get<dimension>(get<0>(rectangle)), get<dimension>(get<1>(rectangle)));
			}

			template<std::size_t dimension, typename Rectangle>
			inline detail::RectangleRangeProxy<Rectangle, dimension> range(Rectangle& rectangle) {
				return detail::RectangleRangeProxy<Rectangle, dimension>(rectangle);
			}

			template<typename Rectangle, typename Size>
			inline Rectangle& resize(Rectangle& rectangle, const Size& size, typename detail::EnableIfTagIs<Size, SizeTag>::type* = 0) {
				Coordinate<Rectangle>::Type o(get<0>(rectangle));
				set<1>(rectangle, translate(o, size));
				return rectangle;
			}

			/**
			 * Returns the x-coordinate of the right edge of @a rectangle.
			 * @see #bottomRight, #left, #topRight
			 */
			template<typename Rectangle>
			inline typename Coordinate<typename Coordinate<Rectangle>::Type>::Type right(const Rectangle& rectangle) {
				return std::max(x(get<0>(rectangle)), x(get<1>(rectangle)));
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
			template<typename Size>
			Size& scale(const Size& size, bool keepAspectRatioByExpanding);

			/**
			 * Returns the y-coordinate of the top edge of @a rectangle.
			 * @see #bottom, #topLeft, #topRight
			 */
			template<typename Rectangle>
			inline typename Coordinate<typename Coordinate<Rectangle>::Type>::Type top(const Rectangle& rectangle) {
				return std::min(y(get<0>(rectangle)), y(get<1>(rectangle)));
			}

			/**
			 * Returns the top-left corner of @a rectangle.
			 * @see #top, #left
			 */
			template<typename Rectangle>
			inline typename Coordinate<Rectangle>::Type topLeft(const Rectangle& rectangle) {
				return make<typename Coordinate<Rectangle>::Type>(left(rectangle), top(rectangle));
			}

			/**
			 * Returns the top-right corner of @a rectangle.
			 * @see #top, #right
			 */
			template<typename Rectangle>
			inline typename Coordinate<Rectangle>::Type topRight(const Rectangle& rectangle) {
				return make<typename Coordinate<Rectangle>::Type>(right(rectangle), top(rectangle));
			}

			/**
			 * Swaps the dx and dy values.
			 * @return @
			 */
			template<typename Size>
			inline Size& transpose(Size& size) {
				return size = make<Size>(dy(size), dx(size));
			}

			/// Returns the origin of the @a rectangle.
			template<typename Rectangle>
			inline const typename Coordinate<Rectangle>::Type origin(const Rectangle& rectangle, typename detail::EnableIfTagIs<Rectangle, RectangleTag>::type* = 0) {
				return get<0>(rectangle);
			}

			/// Returns the size of the @a rectangle.
			template<typename Rectangle>
			inline const NativeSize size(const Rectangle& rectangle, typename detail::EnableIfTagIs<Rectangle, RectangleTag>::type* = 0) {
				const std::pair<typename Coordinate<Rectangle>::Type,
					typename Coordinate<Rectangle>::Type> points(std::make_pair(get<0>(rectangle), get<1>(rectangle)));
				return make<NativeSize>(x(points.second) - x(points.first), y(points.second) - y(points.first));
			}
		}

	}
}

#endif // !ASCENSION_GEOMETRY_HPP
