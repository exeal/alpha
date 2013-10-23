/**
 * @file cursor.hpp
 * @author exeal
 * @date 2011-06-25 created
 */

#ifndef ASCENSION_CURSOR_HPP
#define ASCENSION_CURSOR_HPP

#include <ascension/graphics/geometry.hpp>
#include <ascension/platforms.hpp>
#if defined(ASCENSION_WINDOW_SYSTEM_GTK)
#	include <gdkmm/cursor.h>
#elif defined(ASCENSION_WINDOW_SYSTEM_QT)
#	include <QCursor>
#elif defined(ASCENSION_WINDOW_SYSTEM_QUARTZ)
#	include <NSCursor.h>
#elif defined(ASCENSION_WINDOW_SYSTEM_WIN32)
#	include <ascension/win32/windows.hpp>
#endif
#include <boost/optional.hpp>

namespace ascension {

	namespace graphics {
		class Image;
	}

	namespace viewers {
		namespace widgetapi {
			class Cursor {
			public:
				enum Shape {};
			public:
				explicit Cursor(Shape shape);
				explicit Cursor(const graphics::Image& shape);
				Cursor(const graphics::Image& shape, const graphics::Point& hotspot);
				Cursor(const Cursor& other);
				Cursor& operator=(const Cursor& other);
#if defined(ASCENSION_WINDOW_SYSTEM_GTK)
				Glib::RefPtr<Gdk::Cursor> asNativeObject() const BOOST_NOEXCEPT;
#elif defined(ASCENSION_WINDOW_SYSTEM_QT)
				const QCursor& asNativeObject() const BOOST_NOEXCEPT;
#elif defined(ASCENSION_WINDOW_SYSTEM_QUARTZ)
				NSCursor??? asNativeObject() const BOOST_NOEXCEPT;
#elif defined(ASCENSION_WINDOW_SYSTEM_WIN32)
				win32::Handle<HCURSOR>::Type asNativeObject() const BOOST_NOEXCEPT;
#else
				ASCENSION_CANT_DETECT_PLATFORM();
#endif
				static Cursor&& createMonochrome(
					const graphics::geometry::BasicDimension<std::uint16_t>& size,
					const std::uint8_t* bitmap, const std::uint8_t* mask,
					boost::optional<graphics::geometry::BasicPoint<std::uint16_t>> hotspot
						= boost::optional<graphics::geometry::BasicPoint<std::uint16_t>>());
			public:
				static void hide();
				static graphics::Point position();
				static void setPosition(const graphics::Point& p);
				static void show();
			private:
#if defined(ASCENSION_WINDOW_SYSTEM_GTK)
				Glib::RefPtr<Gdk::Cursor>
#elif defined(ASCENSION_WINDOW_SYSTEM_QT)
				QCursor
#elif defined(ASCENSION_WINDOW_SYSTEM_QUARTZ)
				NSCursor
#elif defined(ASCENSION_WINDOW_SYSTEM_WIN32)
				win32::Handle<HCURSOR>::Type
#else
				ASCENSION_CANT_DETECT_PLATFORM();
#endif
					impl_;
			};

		}
	}
}

#endif // !ASCENSION_CURSOR_HPP
