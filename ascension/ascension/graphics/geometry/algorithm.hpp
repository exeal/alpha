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
#include <ascension/platforms.hpp>
#include <boost/geometry/algorithms/assign.hpp>
#include <boost/geometry/algorithms/make.hpp>
#include <boost/geometry/algorithms/transform.hpp>
#include <boost/geometry/strategies/transform/matrix_transformers.hpp>
#include <boost/parameter.hpp>
#include <boost/range/irange.hpp>
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

			/// @defgroup geometry_additional_aceessors Additional Access Functions
			/// @{

			/// Returns the size of the @a rectangle in x-coordinate.
			template<typename Geometry>
			inline typename boost::geometry::coordinate_type<Geometry>::type dx(const Geometry& rectangle, typename detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
				return boost::geometry::get<boost::geometry::max_corner, 0>(rectangle) - boost::geometry::get<boost::geometry::min_corner, 0>(rectangle);
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
				return isNormalized(::ascension::graphics::geometry::size(rectangle));
			}

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
		}

		// platform-dependent conversions
		namespace detail {
#if ASCENSION_SUPPORTS_GRAPHICS_SYSTEM(CAIRO)
			template<typename Geometry>
			inline Geometry fromNative(const Gdk::Point& native,
					typename geometry::detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr) {
				return geometry::make<Geometry>((geometry::_x = native.get_x(), geometry::_y = native.get_y()));
			}
			template<typename Geometry>
			inline Geometry fromNative(const Gdk::Rectangle& native,
					typename geometry::detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
				return geometry::make<Geometry>((
					geometry::_left = native.get_x(), geometry::_top = native.get_y(),
					geometry::_right = native.get_x() + native.get_width(), geometry::_bottom = native.get_y() + native.get_height()));
			}
			template<typename Geometry>
			inline Geometry fromNative(const Cairo::Rectangle& native,
					typename geometry::detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
				return geometry::make<Geometry>((
					geometry::_left = native.x, geometry::_top = native.y,
					geometry::_right = native.x + native.width, geometry::_bottom = native.y + native.height));
			}
			template<typename Geometry>
			inline Geometry fromNative(const Cairo::RectangleInt& native,
					typename geometry::detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
				return geometry::make<Geometry>((
					geometry::_left = native.x, geometry::_top = native.y,
					geometry::_right = native.x + native.width, geometry::_bottom = native.y + native.height));
			}

			template<typename Geometry>
			inline Gdk::Point toNative(const Geometry& g, const Gdk::Point* = nullptr,
					typename geometry::detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr) {
				return Gdk::Point(static_cast<int>(geometry::x(g)), static_cast<int>(geometry::y(g)));
			}
			template<typename Geometry>
			inline Gdk::Rectangle toNative(const Geometry& g, const Gdk::Rectangle* = nullptr,
					typename geometry::detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
				return Gdk::Rectangle(
					static_cast<int>(geometry::left(g)), static_cast<int>(geometry::top(g)),
					static_cast<int>(geometry::dx(g)), static_cast<int>(geometry::dy(g)));
			}
			template<typename Geometry>
			inline Cairo::Rectangle toNative(const Geometry& g, const Cairo::Rectangle* = nullptr,
					typename geometry::detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
				Cairo::Rectangle native;
				native.x = geometry::left(g);
				native.y = geometry::top(g);
				native.width = geometry::dx(g);
				native.height = geometry::dy(g);
				return native;
			}
			template<typename Geometry>
			inline Cairo::RectangleInt toNative(const Geometry& g, const Cairo::RectangleInt* = nullptr,
					typename geometry::detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
				Cairo::RectangleInt native;
				native.x = static_cast<int>(geometry::left(g));
				native.y = static_cast<int>(geometry::top(g));
				native.width = static_cast<int>(geometry::dx(g));
				native.height = static_cast<int>(geometry::dy(g));
				return native;
			}
#endif
#if ASCENSION_SUPPORTS_GRAPHICS_SYSTEM(CORE_GRAPHICS)
#endif
#if ASCENSION_SUPPORTS_GRAPHICS_SYSTEM(QT)
#endif
#if ASCENSION_SUPPORTS_GRAPHICS_SYSTEM(WIN32_GDI)
			template<typename Geometry>
			inline Geometry fromNative(const COORD& native,
					typename geometry::detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr) {
				return geometry::make<Geometry>((geometry::_x = native.x, geometry::_y = native.y));
			}
			template<typename Geometry>
			inline Geometry fromNative(const POINT& native,
					typename geometry::detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr) {
				return geometry::make<Geometry>((geometry::_x = native.x, geometry::_y = native.y));
			}
			template<typename Geometry>
			inline Geometry fromNative(const POINTL& native,
					typename geometry::detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr) {
				return geometry::make<Geometry>((geometry::_x = native.x, geometry::_y = native.y));
			}
			template<typename Geometry>
			inline Geometry fromNative(const POINTS& native,
					typename geometry::detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr) {
				return geometry::make<Geometry>((geometry::_x = native.x, geometry::_y = native.y));
			}
			template<typename Geometry>
			inline Geometry fromNative(const SIZE& native,
					typename geometry::detail::EnableIfTagIs<Geometry, geometry::DimensionTag>::type* = nullptr) {
				return Geometry(geometry::_dx = native.cx, geometry::_dy = native.cy);
			}
			template<typename Geometry>
			inline Geometry fromNative(const RECT& native,
					typename geometry::detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
				return geometry::make<Geometry>((
					geometry::_left = native.left, geometry::_top = native.top,
					geometry::_right = native.right, geometry::_bottom = native.bottom));
			}
			template<typename Geometry>
			inline Geometry fromNative(const RECTL& native,
					typename geometry::detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
				return geometry::make<Geometry>((
					geometry::_left = native.left, geometry::_top = native.top,
					geometry::_right = native.right, geometry::_bottom = native.bottom));
			}
			template<typename Geometry>
			inline Geometry fromNative(const SMALL_RECT& native,
					typename geometry::detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
				return geometry::make<Geometry>((
					geometry::_left = native.left, geometry::_top = native.top,
					geometry::_right = native.right, geometry::_bottom = native.bottom));
			}

			template<typename Geometry>
			inline COORD toNative(const Geometry& g, const COORD* = nullptr,
					typename geometry::detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr) {
				COORD native;
				native.X = static_cast<SHORT>(geometry::x(g));
				native.Y = static_cast<SHORT>(geometry::y(g));
				return native;
			}
			template<typename Geometry>
			inline POINT toNative(const Geometry& g, const POINT* = nullptr,
					typename geometry::detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr) {
				POINT native;
				native.x = static_cast<LONG>(geometry::x(g));
				native.y = static_cast<LONG>(geometry::y(g));
				return native;
			}
			template<typename Geometry>
			inline POINTL toNative(const Geometry& g, const POINTL* = nullptr,
					typename geometry::detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr) {
				POINTL native;
				native.x = static_cast<LONG>(geometry::x(g));
				native.y = static_cast<LONG>(geometry::y(g));
				return native;
			}
			template<typename Geometry>
			inline POINTS toNative(const Geometry& g, const POINTS* = nullptr,
					typename geometry::detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr) {
				POINTS native;
				native.x = static_cast<SHORT>(geometry::x(g));
				native.y = static_cast<SHORT>(geometry::y(g));
				return native;
			}

			template<typename Geometry>
			inline SIZE toNative(const Geometry& g, const SIZE* = nullptr,
					typename geometry::detail::EnableIfTagIs<Geometry, geometry::DimensionTag>::type* = nullptr) {
				SIZE native;
				native.cx = static_cast<LONG>(geometry::dx(g));
				native.cy = static_cast<LONG>(geometry::dy(g));
				return native;
			}

			template<typename Geometry>
			inline RECT toNative(const Geometry& g, const RECT* = nullptr,
					typename geometry::detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
				RECT native;
				native.left = static_cast<LONG>(geometry::left(g));
				native.top = static_cast<LONG>(geometry::top(g));
				native.right = static_cast<LONG>(geometry::right(g));
				native.bottom = static_cast<LONG>(geometry::bottom(g));
				return native;
			}
			template<typename Geometry>
			inline RECTL toNative(const Geometry& g, const RECTL* = nullptr,
					typename geometry::detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
				RECTL native;
				native.left = static_cast<LONG>(geometry::left(g));
				native.top = static_cast<LONG>(geometry::top(g));
				native.right = static_cast<LONG>(geometry::right(g));
				native.bottom = static_cast<LONG>(geometry::bottom(g));
				return native;
			}
			template<typename Geometry>
			inline SMALL_RECT toNative(const Geometry& g, const SMALL_RECT* = nullptr,
					typename geometry::detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
				SMALL_RECT native;
				native.Left = static_cast<SHORT>(geometry::left(g));
				native.Top = static_cast<SHORT>(geometry::top(g));
				native.Right = static_cast<SHORT>(geometry::right(g));
				native.Bottom = static_cast<SHORT>(geometry::bottom(g));
				return native;
			}
#endif
#if ASCENSION_SUPPORTS_SHAPING_ENGINE(PANGO)
			template<typename Geometry>
			inline Geometry fromNative(const Pango::Rectangle& native,
					typename geometry::detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
				return geometry::make<Geometry>((
					geometry::_left = native.get_x(), geometry::_top = native.get_y(),
					geometry::_right = native.get_x() + native.get_width(), geometry::_bottom = native.get_y() + native.get_height()));
			}
			template<typename Geometry>
			inline Pango::Rectangle toNative(const Geometry& g, const Pango::Rectangle* = nullptr,
					typename geometry::detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
				return Pango::Rectangle(geometry::left(g), geometry::top(g), geometry::dx(g), geometry::dy(g));
			}
#endif
#if ASCENSION_SUPPORTS_GRAPHICS_SYSTEM(WIN32_GDIPLUS)
#endif
		}
	}
}

#endif // !ASCENSION_GEOMETRY_HPP
