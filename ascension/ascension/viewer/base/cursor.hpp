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
		namespace base {

			typedef
#if defined(ASCENSION_WINDOW_SYSTEM_GTK)
				Glib::PefPtr<Gdk::Cursor>
#elif defined(ASCENSION_WINDOW_SYSTEM_QT)
				QCursor
#elif defined(ASCENSION_WINDOW_SYSTEM_QUARTZ)
				NSCursor
#elif defined(ASCENSION_WINDOW_SYSTEM_WIN32)
				std::shared_ptr<std::remove_pointer<HCURSOR>::type>
#endif
				NativeCursor;

			class Cursor {
			public:
				enum Shape {};
			public:
				explicit Cursor(Shape shape);
				explicit Cursor(const graphics::Image& shape);
				Cursor(const graphics::Image& shape, const graphics::NativePoint& hotspot);
				explicit Cursor(const NativeCursor&);
				Cursor(const Cursor& other);
				Cursor& operator=(const Cursor& other);
				const NativeCursor& asNativeObject() const /*throw()*/;
			public:
				static void hide();
				static graphics::NativePoint position();
				static void setPosition(const graphics::NativePoint& p);
				static void show();
			private:
				NativeCursor impl_;
			};

		}
	}
}

#endif // !ASCENSION_CURSOR_HPP
