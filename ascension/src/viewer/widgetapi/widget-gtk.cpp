/**
 * @file widget-gtk.cpp
 * @author exeal
 * @date 2014-04-07 Created.
 */

#include <ascension/viewer/widgetapi/widget.hpp>

#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
#	include <ascension/graphics/native-conversion.hpp>
#	include <ascension/graphics/rendering-context.hpp>
#	include <gtkmm/window.h>
#	if ASCENSION_SELECTS_GRAPHICS_SYSTEM(WIN32_GDI)
#		include <gdk/win32/gdkwin32.h>
#	endif

namespace ascension {
	namespace viewers {
		namespace widgetapi {
			graphics::Rectangle bounds(Proxy<const Widget> widget, bool includeFrame) {
				if(includeFrame && widget->get_has_window()) {
					Gdk::Rectangle extents;
#if GTKMM_MAJOR_VERSION >= 3 && GTKMM_MINOR_VERSION >= 8
					widget->get_window()->get_frame_extents(extents);
#else
					Glib::RefPtr<Gtk::Widget>::cast_const(widget.get())->get_window()->get_frame_extents(extents);
#endif
					return graphics::fromNative<graphics::Rectangle>(extents);
				} else
					return graphics::fromNative<graphics::Rectangle>(widget->get_allocation());
			}

			std::unique_ptr<graphics::RenderingContext2D> createRenderingContext(Proxy<const Widget> widget) {
#if ASCENSION_SELECTS_GRAPHICS_SYSTEM(CAIRO)
#elif ASCENSION_SELECTS_GRAPHICS_SYSTEM(WIN32_GDI)
				const Glib::RefPtr<const Gdk::Window> window(widget->get_window());
				if(!window)
					throw NullPointerException("widget");
				win32::Handle<HWND>::Type hwnd(::gdk_win32_window_get_impl_hwnd(const_cast<GdkWindow*>(window->gobj())), ascension::detail::NullDeleter());
				win32::Handle<HDC>::Type dc(::GetDC(hwnd.get()), std::bind(&::ReleaseDC, hwnd.get(), std::placeholders::_1));
				return std::unique_ptr<graphics::RenderingContext2D>(new graphics::RenderingContext2D(dc));
#endif
			}

			Proxy<const Window> cwindow(Proxy<const Widget> widget) {
				return Proxy<const Window>(widget->get_window());
			}

//			Proxy<Widget> widgetapi::desktop() {
////			return Proxy<Window>(Gdk::Window::get_default_root_window());
//			}

			bool hasFocus(Proxy<const Widget> widget) {
				return widget->has_focus();
			}

			void hide(Proxy<Widget> widget) {
				widget->hide();
			}

			bool isMaximized(Proxy<const Window> window) {
				return (window->get_state() & Gdk::WINDOW_STATE_MAXIMIZED) != 0;
			}

			bool isMinimized(Proxy<const Window> window) {
				return (window->get_state() & Gdk::WINDOW_STATE_ICONIFIED) != 0;
			}

			bool isVisible(Proxy<const Widget> widget) {
				return widget->get_visible();
			}

			void move(Proxy<Window> widget, const graphics::Point& newOrigin) {
				widget->move(static_cast<int>(graphics::geometry::x(newOrigin)), static_cast<int>(graphics::geometry::y(newOrigin)));
			}

			void raise(Proxy<Window> window) {
				window->raise();
			}

			void redrawScheduledRegion(Proxy<Widget> widget) {
				if(const Glib::RefPtr<Gdk::Window> window = widget->get_window())
					window->process_updates(true);
			}

			void resize(Proxy<Window> window, const graphics::Dimension& newSize) {
				window->resize(static_cast<int>(graphics::geometry::dx(newSize)), static_cast<int>(graphics::geometry::dy(newSize)));
			}

			void scheduleRedraw(Proxy<Widget> widget, bool eraseBackground) {
				widget->queue_draw();
			}

			void scheduleRedraw(Proxy<Widget> widget, const graphics::Rectangle& rect, bool eraseBackground) {
				widget->queue_draw_area(
					static_cast<int>(graphics::geometry::left(rect)), static_cast<int>(graphics::geometry::top(rect)),
					static_cast<int>(graphics::geometry::dx(rect)), static_cast<int>(graphics::geometry::dy(rect)));
			}

			void setBounds(Proxy<Widget> widget, const graphics::Rectangle& bounds) {
				widget->set_allocation(graphics::toNative<Gtk::Allocation>(bounds));
			}

			void setFocus(Proxy<Widget> widget) {
//				widget->grab_focus();
				widget->set_state(Gtk::STATE_FOCUSED);
			}

			void showMaximized(Proxy<Window> window) {
				window->maximize();
			}

			void showMinimized(Proxy<Window> window) {
				window->iconify();
			}

			void showNormal(Proxy<Window> window) {
				window->show_unraised();
			}

			void unsetFocus(Proxy<Widget> widget) {
				widget->set_state(Gtk::STATE_NORMAL);
			}

			Proxy<Window> window(Proxy<Widget> widget) {
				return Proxy<Window>(widget->get_window());
			}
		}
	}
}

#endif // ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
