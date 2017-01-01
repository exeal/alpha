/**
 * @file native-conversions.hpp
 * Defines 
 * @date 2015-10-24 Created.
 * @see graphics/native-conversion.hpp
 */

#ifndef ASCENSION_GEOMETRY_NATIVE_CONVERSIONS_HPP
#define ASCENSION_GEOMETRY_NATIVE_CONVERSIONS_HPP
#include <ascension/graphics/geometry/common.hpp>
#include <ascension/graphics/geometry/point-xy.hpp>
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
#if ASCENSION_SUPPORTS_GRAPHICS_SYSTEM(CAIRO)
	template<typename Geometry>
	inline Geometry _fromNative(const Gdk::Point& native, const Geometry* = nullptr, typename graphics::geometry::detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr) {
		return graphics::geometry::make<Geometry>((geometry::_x = native.get_x(), geometry::_y = native.get_y()));
	}
	template<typename Geometry>
	inline Geometry _fromNative(const Gdk::Rectangle& native, const Geometry* = nullptr, typename graphics::geometry::detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
		return graphics::geometry::make<Geometry>((
			graphics::geometry::_left = native.get_x(), graphics::geometry::_top = native.get_y(),
			graphics::geometry::_right = native.get_x() + native.get_width(), graphics::geometry::_bottom = native.get_y() + native.get_height()));
	}
	template<typename Geometry>
	inline Geometry _fromNative(const Cairo::Rectangle& native, const Geometry* = nullptr, typename graphics::geometry::detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
		return graphics::geometry::make<Geometry>((
			graphics::geometry::_left = native.x, graphics::geometry::_top = native.y,
			graphics::geometry::_right = native.x + native.width, graphics::geometry::_bottom = native.y + native.height));
	}
	template<typename Geometry>
	inline Geometry _fromNative(const Cairo::RectangleInt& native, const Geometry* = nullptr, typename graphics::geometry::detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
		return graphics::geometry::make<Geometry>((
			graphics::geometry::_left = native.x, graphics::geometry::_top = native.y,
			graphics::geometry::_right = native.x + native.width, graphics::geometry::_bottom = native.y + native.height));
	}

	template<typename Geometry>
	inline Gdk::Point _toNative(const Geometry& g, const Gdk::Point* = nullptr, typename graphics::geometry::detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr) {
		return Gdk::Point(static_cast<int>(boost::geometry::get<0>(g)), static_cast<int>(boost::geometry::get<1>(g)));
	}
	template<typename Geometry>
	inline Gdk::Rectangle _toNative(const Geometry& g, const Gdk::Rectangle* = nullptr, typename graphics::geometry::detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
		return Gdk::Rectangle(
			static_cast<int>(graphics::geometry::left(g)), static_cast<int>(graphics::geometry::top(g)),
			static_cast<int>(graphics::geometry::dx(g)), static_cast<int>(graphics::geometry::dy(g)));
	}
	template<typename Geometry>
	inline Cairo::Rectangle _toNative(const Geometry& g, const Cairo::Rectangle* = nullptr, typename graphics::geometry::detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
		Cairo::Rectangle native;
		native.x = graphics::geometry::left(g);
		native.y = graphics::geometry::top(g);
		native.width = graphics::geometry::dx(g);
		native.height = graphics::geometry::dy(g);
		return native;
	}
	template<typename Geometry>
	inline Cairo::RectangleInt _toNative(const Geometry& g, const Cairo::RectangleInt* = nullptr, typename graphics::geometry::detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
		Cairo::RectangleInt native;
		native.x = static_cast<int>(graphics::geometry::left(g));
		native.y = static_cast<int>(graphics::geometry::top(g));
		native.width = static_cast<int>(graphics::geometry::dx(g));
		native.height = static_cast<int>(graphics::geometry::dy(g));
		return native;
	}
#endif
#if ASCENSION_SUPPORTS_GRAPHICS_SYSTEM(CORE_GRAPHICS)
#endif
#if ASCENSION_SUPPORTS_GRAPHICS_SYSTEM(QT)
#endif
#if ASCENSION_SUPPORTS_GRAPHICS_SYSTEM(WIN32_GDI)
	template<typename Geometry>
	inline Geometry _fromNative(const COORD& native, const Geometry* = nullptr, typename graphics::geometry::detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr) {
		return graphics::geometry::make<Geometry>((graphics::geometry::_x = native.x, graphics::geometry::_y = native.y));
	}
	template<typename Geometry>
	inline Geometry _fromNative(const POINT& native, const Geometry* = nullptr, typename graphics::geometry::detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr) {
		return graphics::geometry::make<Geometry>((graphics::geometry::_x = native.x, graphics::geometry::_y = native.y));
	}
	template<typename Geometry>
	inline Geometry _fromNative(const POINTL& native, const Geometry* = nullptr, typename graphics::geometry::detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr) {
		return graphics::geometry::make<Geometry>((graphics::geometry::_x = native.x, graphics::geometry::_y = native.y));
	}
	template<typename Geometry>
	inline Geometry _fromNative(const POINTS& native, const Geometry* = nullptr, typename graphics::geometry::detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr) {
		return geometry::make<Geometry>((graphics::geometry::_x = native.x, graphics::geometry::_y = native.y));
	}
	template<typename Geometry>
	inline Geometry _fromNative(const SIZE& native, const Geometry* = nullptr, typename graphics::geometry::detail::EnableIfTagIs<Geometry, graphics::geometry::DimensionTag>::type* = nullptr) {
		return Geometry(graphics::geometry::_dx = native.cx, graphics::geometry::_dy = native.cy);
	}
	template<typename Geometry>
	inline Geometry _fromNative(const RECT& native, const Geometry* = nullptr, typename graphics::geometry::detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
		return graphics::geometry::make<Geometry>((
			graphics::geometry::_left = native.left, graphics::geometry::_top = native.top,
			graphics::geometry::_right = native.right, graphics::geometry::_bottom = native.bottom));
	}
	template<typename Geometry>
	inline Geometry _fromNative(const RECTL& native, const Geometry* = nullptr, typename graphics::geometry::detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
		return graphics::geometry::make<Geometry>((
			graphics::geometry::_left = native.left, graphics::geometry::_top = native.top,
			graphics::geometry::_right = native.right, graphics::geometry::_bottom = native.bottom));
	}
	template<typename Geometry>
	inline Geometry _fromNative(const SMALL_RECT& native, const Geometry* = nullptr, typename graphics::geometry::detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
		return graphics::geometry::make<Geometry>((
			graphics::geometry::_left = native.left, graphics::geometry::_top = native.top,
			graphics::geometry::_right = native.right, graphics::geometry::_bottom = native.bottom));
	}

	template<typename Geometry>
	inline COORD _toNative(const Geometry& g, const COORD* = nullptr, typename graphics::geometry::detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr) {
		COORD native;
		native.X = static_cast<SHORT>(graphics::geometry::x(g));
		native.Y = static_cast<SHORT>(graphics::geometry::y(g));
		return native;
	}
	template<typename Geometry>
	inline POINT _toNative(const Geometry& g, const POINT* = nullptr, typename graphics::geometry::detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr) {
		POINT native;
		native.x = static_cast<LONG>(graphics::geometry::x(g));
		native.y = static_cast<LONG>(graphics::geometry::y(g));
		return native;
	}
	template<typename Geometry>
	inline POINTL _toNative(const Geometry& g, const POINTL* = nullptr, typename graphics::geometry::detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr) {
		POINTL native;
		native.x = static_cast<LONG>(graphics::geometry::x(g));
		native.y = static_cast<LONG>(graphics::geometry::y(g));
		return native;
	}
	template<typename Geometry>
	inline POINTS _toNative(const Geometry& g, const POINTS* = nullptr, typename graphics::geometry::detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr) {
		POINTS native;
		native.x = static_cast<SHORT>(graphics::geometry::x(g));
		native.y = static_cast<SHORT>(graphics::geometry::y(g));
		return native;
	}

	template<typename Geometry>
	inline SIZE _toNative(const Geometry& g, const SIZE* = nullptr, typename graphics::geometry::detail::EnableIfTagIs<Geometry, graphics::geometry::DimensionTag>::type* = nullptr) {
		SIZE native;
		native.cx = static_cast<LONG>(graphics::geometry::dx(g));
		native.cy = static_cast<LONG>(graphics::geometry::dy(g));
		return native;
	}

	template<typename Geometry>
	inline RECT _toNative(const Geometry& g, const RECT* = nullptr, typename graphics::geometry::detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
		RECT native;
		native.left = static_cast<LONG>(graphics::geometry::left(g));
		native.top = static_cast<LONG>(graphics::geometry::top(g));
		native.right = static_cast<LONG>(graphics::geometry::right(g));
		native.bottom = static_cast<LONG>(graphics::geometry::bottom(g));
		return native;
	}
	template<typename Geometry>
	inline RECTL _toNative(const Geometry& g, const RECTL* = nullptr, typename graphics::geometry::detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
		RECTL native;
		native.left = static_cast<LONG>(graphics::geometry::left(g));
		native.top = static_cast<LONG>(graphics::geometry::top(g));
		native.right = static_cast<LONG>(graphics::geometry::right(g));
		native.bottom = static_cast<LONG>(graphics::geometry::bottom(g));
		return native;
	}
	template<typename Geometry>
	inline SMALL_RECT _toNative(const Geometry& g, const SMALL_RECT* = nullptr, typename graphics::geometry::detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
		SMALL_RECT native;
		native.Left = static_cast<SHORT>(graphics::geometry::left(g));
		native.Top = static_cast<SHORT>(graphics::geometry::top(g));
		native.Right = static_cast<SHORT>(graphics::geometry::right(g));
		native.Bottom = static_cast<SHORT>(graphics::geometry::bottom(g));
		return native;
	}
#endif
#if ASCENSION_SUPPORTS_SHAPING_ENGINE(PANGO)
	template<typename Geometry>
	inline Geometry _fromNative(const Pango::Rectangle& native, const Geometry* = nullptr, typename graphics::geometry::detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
		return graphics::geometry::make<Geometry>((
			graphics::geometry::_left = native.get_x(), graphics::geometry::_top = native.get_y(),
			graphics::geometry::_right = native.get_x() + native.get_width(), graphics::geometry::_bottom = native.get_y() + native.get_height()));
	}
	template<typename Geometry>
	inline Pango::Rectangle _toNative(const Geometry& g, const Pango::Rectangle* = nullptr, typename graphics::geometry::detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
		return Pango::Rectangle(graphics::geometry::left(g), graphics::geometry::top(g), graphics::geometry::dx(g), graphics::geometry::dy(g));
	}
#endif
#if ASCENSION_SUPPORTS_GRAPHICS_SYSTEM(WIN32_GDIPLUS)
#endif
}

#endif // !ASCENSION_GEOMETRY_NATIVE_CONVERSIONS_HPP
