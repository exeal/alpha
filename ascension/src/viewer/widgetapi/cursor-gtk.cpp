/**
 * @file cursor-gtk.cpp
 * Implements @c ascension#viewers#widgetapi#Cursor class on gtkmm window system.
 * @date 2014-02-01 Created.
 */

#include <ascension/viewer/widgetapi/cursor.hpp>
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
#include <ascension/graphics/image.hpp>
#if ASCENSION_SUPPORTS_GRAPHICS_SYSTEM(WIN32_GDI)
#	include <ascension/graphics/rendering-context.hpp>
#	include <cairomm/win32_surface.h>
#endif

namespace ascension {
	namespace viewers {
		namespace widgetapi {
			Cursor::Cursor(Cursor::BuiltinShape shape) : impl_(Gdk::Cursor::create(shape)) {
			}

			Cursor::Cursor(const graphics::Image& shape,
					const boost::optional<graphics::geometry::BasicPoint<Cursor::Coordinate>>& hotspot /* = boost::none */) {
				Cairo::RefPtr<Cairo::Surface> surface;
#if ASCENSION_SELECTS_GRAPHICS_SYSTEM(CAIRO)
				surface = shape.asNativeObject();
#elif ASCENSION_SELECTS_GRAPHICS_SYSTEM(WIN32_GDI)
				const std::unique_ptr<graphics::RenderingContext2D> context(graphics::Screen::instance().createGraphicsContext());
				const win32::Handle<HDC>::Type dc(context->asNativeObject());
				HBITMAP oldBitmap = static_cast<HBITMAP>(::SelectObject(dc.get(), shape.asNativeObject().get()));
				surface = Cairo::Win32Surface::create(context->asNativeObject().get());
#else
				ASCENSION_CANT_DETECT_PLATFORM();
#endif
#if GTKMM_MAJOR_VERSION >= 3 && GTKMM_MINOR_VERSION >= 10
				impl_ = Gdk::Cursor::create(Gdk::Display::get_default(), surface,
					(hotspot != boost::none) ? graphics::geometry::x(*hotspot) : shape.width() / 2,
					(hotspot != boost::none) ? graphics::geometry::y(*hotspot) : shape.height() / 2);
#else
#endif

#if ASCENSION_SELECTS_GRAPHICS_SYSTEM(WIN32_GDI)
				::SelectObject(dc.get(), oldBitmap);
#endif
			}

			Cursor::Cursor(const Cursor& other) {
				const Glib::RefPtr<const Gdk::Cursor> native(other.asNativeObject());
				const Gdk::CursorType type = native->get_cursor_type();
				if(type != Gdk::CURSOR_IS_PIXMAP)
					impl_ = Gdk::Cursor::create(type);
				else {
#if GTKMM_MAJOR_VERSION >= 3 && GTKMM_MINOR_VERSION >= 10
					// TODO: Write code using Cairo.Surface.
#else
					impl_ = Gdk::Cursor::create(const_cast<Cursor&>(other).asNativeObject()->get_display(), other.impl_->get_image()->copy(), -1, -1);
#endif
				}
			}
		}
	}
}

#endif // ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
