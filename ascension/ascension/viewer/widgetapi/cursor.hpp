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

namespace ascension {

	namespace graphics {class Image;}

	namespace viewers {
		namespace widgetapi {

			typedef
#if defined(ASCENSION_WINDOW_SYSTEM_GTK)
				Glib::PefPtr<Gdk::Cursor>
#elif defined(ASCENSION_WINDOW_SYSTEM_QT)
				QCursor
#elif defined(ASCENSION_WINDOW_SYSTEM_QUARTZ)
				NSCursor
#elif defined(ASCENSION_WINDOW_SYSTEM_WIN32)
				win32::Handle<HCURSOR>
#endif
				NativeCursor;

			class Cursor {
			public:
				enum Shape {};
			public:
				explicit Cursor(Shape shape);
				explicit Cursor(const graphics::Image& shape);
				Cursor(const graphics::Image& shape, const graphics::Point& hotspot);
				explicit Cursor(const NativeCursor&);
				Cursor(const Cursor& other);
				Cursor& operator=(const Cursor& other);
				const NativeCursor& asNativeObject() const BOOST_NOEXCEPT;
			public:
				static void hide();
				static graphics::Point position();
				static void setPosition(const graphics::Point& p);
				static void show();
			private:
				NativeCursor impl_;
			};

		}
	}
}

#endif // !ASCENSION_CURSOR_HPP
