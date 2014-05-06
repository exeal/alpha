/**
 * @file widget-gtk.cpp
 * @author exeal
 * @date 2014-04-07 Created.
 */

#include <ascension/viewer/widgetapi/widget.hpp>

#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
#	include <ascension/graphics/color.hpp>
#	include <ascension/graphics/native-conversion.hpp>
#	include <ascension/graphics/rendering-context.hpp>
#	include <gtkmm/button.h>
#	include <gtkmm/menu.h>
#	include <gtkmm/menuitem.h>
#	include <gtkmm/scrollbar.h>
#	include <gtkmm/tooltip.h>
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

			ascension::detail::ScopeGuard grabInput(Proxy<Widget> widget) {
				widget->add_modal_grab();
				return ascension::detail::ScopeGuard(std::bind(&releaseInput, widget));
			}

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

			void releaseInput(Proxy<Widget> widget) {
				widget->remove_modal_grab();
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

			void setParentWidget(Proxy<Widget> widget, Proxy<Widget> newParent) {
				if(newParent)
//					widget->set_parent(newParent.get());
					widget->set_parent_window(newParent->get_window());
				else
					widget->unparent();
			}

			void setParentWindow(Proxy<Widget> widget, Proxy<Window> newParent) {
				if(newParent)
					widget->set_parent_window(newParent.get());
				else
					widget->unparent();
			}

			void show(Proxy<Widget> widget) {
				widget->show();
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

	namespace graphics {
		boost::optional<Color> SystemColors::get(Value value) {
			if(value > WINDOW_TEXT)
				throw UnknownValueException("value");

			Gtk::WidgetPath path;
			switch(value) {
#ifdef GTK_STYLE_CLASS_TITLEBAR
				case ACTIVE_CAPTION:
				case CAPTION_TEXT:
				case INACTIVE_CAPTION:
				case INACTIVE_CAPTION_TEXT:
					path.path_append_type(Gtk::Window::get_type());
					path.iter_add_class(0, GTK_STYLE_CLASS_TITLEBAR);
					break;
#endif
				case BUTTON_FACE:
					path.path_append_type(Gtk::Button::get_type());
					path.iter_add_class(0, GTK_STYLE_CLASS_BUTTON);
					break;
				case GRAY_TEXT:
				case HIGHLIGHT:
				case HIGHLIGHT_TEXT:
				case THREE_D_FACE:
				case WINDOW:
				case WINDOW_TEXT:
					path.path_append_type(Gtk::Widget::get_type());
					path.iter_add_class(0, GTK_STYLE_CLASS_BACKGROUND);
					break;
				case INFO_BACKGROUND:
				case INFO_TEXT:
					path.path_append_type(Gtk::Tooltip::get_type());
					path.iter_add_class(0, GTK_STYLE_CLASS_TOOLTIP);
					break;
				case MENU:
				case MENU_TEXT:
					path.path_append_type(Gtk::Menu::get_type());
					path.path_append_type(Gtk::MenuItem::get_type());
					path.iter_add_class(0, GTK_STYLE_CLASS_MENU);
					path.iter_add_class(1, GTK_STYLE_CLASS_MENUITEM);
					break;
				case SCROLLBAR:
					path.path_append_type(Gtk::Scrollbar::get_type());
					path.iter_add_class(0, GTK_STYLE_CLASS_BUTTON);
					break;
				default:
					return boost::none;
			}

			const Glib::RefPtr<Gtk::StyleContext> context(Gtk::StyleContext::create());
			context->set_path(path);
			switch(value) {
#ifdef GTK_STYLE_CLASS_TITLEBAR
				case ACTIVE_CAPTION:
					return Color::from(context->get_background_color(Gtk::STATE_FLAG_ACTIVE));
#endif
				case BUTTON_FACE:
				case INFO_BACKGROUND:
				case MENU:
				case SCROLLBAR:
				case THREE_D_FACE:
				case WINDOW:
					return Color::from(context->get_background_color());
#ifdef GTK_STYLE_CLASS_TITLEBAR
				case CAPTION_TEXT:
					return Color::from(context->get_color(Gtk::STATE_FLAG_ACTIVE));
#endif
				case GRAY_TEXT:
					return Color::from(context->get_color(Gtk::STATE_FLAG_INSENSITIVE));
				case HIGHLIGHT:
					return Color::from(context->get_background_color(Gtk::STATE_FLAG_SELECTED));
				case HIGHLIGHT_TEXT:
					return Color::from(context->get_color(Gtk::STATE_FLAG_SELECTED));
#ifdef GTK_STYLE_CLASS_TITLEBAR
				case INACTIVE_CAPTION:
#endif
#ifdef GTK_STYLE_CLASS_TITLEBAR
				case INACTIVE_CAPTION_TEXT:
#endif
				case INFO_TEXT:
				case MENU_TEXT:
				case WINDOW_TEXT:
					return Color::from(context->get_color());
				default:
					return boost::none;
			}
		}
	}
}

#endif // ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
