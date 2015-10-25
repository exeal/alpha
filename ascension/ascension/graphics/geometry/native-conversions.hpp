/**
 * @file native-conversions.hpp
 * Defines 
 * @date 2015-10-24 Created.
 * @see graphics/native-conversion.hpp
 */

#ifndef ASCENSION_GEOMETRY_NATIVE_CONVERSIONS_HPP
#define ASCENSION_GEOMETRY_NATIVE_CONVERSIONS_HPP
#include <ascension/graphics/geometry/common.hpp>
#include <ascension/graphics/geometry/rectangle-odxdy.hpp>
#include <ascension/graphics/geometry/rectangle-sides.hpp>
#include <ascension/graphics/geometry/algorithms/make.hpp>
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

namespace ascension {
	namespace graphics {
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
				return Gdk::Point(static_cast<int>(boost::geometry::get<0>(g)), static_cast<int>(boost::geometry::get<1>(g)));
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

#endif // !ASCENSION_GEOMETRY_NATIVE_CONVERSIONS_HPP
