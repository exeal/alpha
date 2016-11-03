/**
 * @file screen-gtk.cpp
 * @author exeal
 * Implements @c ascension#viewer#widgetapi#Screen class on gtkmm.
 * @date 2014-05-28 Created.
 */

#include <ascension/viewer/widgetapi/screen.hpp>
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
#	include <ascension/graphics/rendering-context.hpp>
#	include <boost/core/null_deleter.hpp>
#	include <gdkmm/visual.h>
#	include <gdkmm/window.h>
#	if ASCENSION_SELECTS_GRAPHICS_SYSTEM(WIN32_GDI)
#		include <gdk/gdkwin32.h>
#	endif

namespace ascension {
	namespace viewer {
		namespace widgetapi {
			/**
			 * Wraps a native @c Gdk#Screen object.
			 * @param nativeObject The native object
			 */
			Screen::Screen(Glib::RefPtr<Gdk::Screen> nativeObject) : nativeObject_(nativeObject) {
			}

			std::unique_ptr<graphics::RenderingContext2D> Screen::createRenderingContext() const {
				if(const Glib::RefPtr<Gdk::Window> window = Glib::RefPtr<Gdk::Window>::cast_const(native()->get_root_window())) {
#if ASCENSION_SELECTS_GRAPHICS_SYSTEM(WIN32_GDI)
					win32::Handle<HWND>::Type hwnd(::gdk_win32_window_get_impl_hwnd(window->gobj()), boost::null_deleter());
					win32::Handle<HDC>::Type dc(::GetDC(hwnd.get()), std::bind(&::ReleaseDC, hwnd.get(), std::placeholders::_1));
					return std::unique_ptr<graphics::RenderingContext2D>(new graphics::RenderingContext2D(dc));
#else
					if(const Cairo::RefPtr<Cairo::Context> context = window->create_cairo_context())
						return std::unique_ptr<graphics::RenderingContext2D>(new graphics::RenderingContext2D(context));
#endif
				}
				return std::unique_ptr<graphics::RenderingContext2D>();
			}

			Screen& Screen::defaultInstance() {
				static Screen singleton(Gdk::Screen::get_default());
				return singleton;
			}

			std::uint8_t Screen::depth() const {
				return native()->get_system_visual()->get_depth();
			}

			std::uint32_t Screen::height() const {
				return native()->get_height();
			}

			graphics::Scalar Screen::heightInMillimeters() const {
				return static_cast<graphics::Scalar>(native()->get_height_mm());
			}

			std::uint16_t Screen::logicalDpiX() const {
				return physicalDpiX();
			}

			std::uint16_t Screen::logicalDpiY() const {
				return physicalDpiY();
			}

			Glib::RefPtr<Gdk::Screen> Screen::native() {
				return nativeObject_;
			}

			Glib::RefPtr<const Gdk::Screen> Screen::native() const {
				return nativeObject_;
			}

			std::uint32_t Screen::numberOfColors() const {
				return 1 << native()->get_system_visual()->get_bits_per_rgb();
			}

			std::uint16_t Screen::physicalDpiX() const {
				return static_cast<std::uint16_t>(width() / widthInMillimeters() * 25.4);
			}

			std::uint16_t Screen::physicalDpiY() const {
				return static_cast<std::uint16_t>(height() / heightInMillimeters() * 25.4);
			}

			std::uint32_t Screen::width() const {
				return native()->get_width();
			}

			graphics::Scalar Screen::widthInMillimeters() const {
				return static_cast<graphics::Scalar>(native()->get_width_mm());
			}
		}
	}
}

#endif // ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
