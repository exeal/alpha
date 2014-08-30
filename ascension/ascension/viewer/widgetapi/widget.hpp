/**
 * @file widget.hpp
 * @author exeal
 * @date 2011-03-27
 */

#ifndef ASCENSION_WIDGET_HPP
#define ASCENSION_WIDGET_HPP
#include <ascension/corelib/basic-exceptions.hpp>	// IllegalStateException
#include <ascension/corelib/scope-guard.hpp>
#include <ascension/graphics/geometry.hpp>
#include <ascension/graphics/rendering-device.hpp>	// graphics.RenderingDevice, ...
//#include <ascension/viewer/widgetapi/drag-and-drop.hpp>
#include <ascension/viewer/widgetapi/user-input.hpp>
#include <ascension/viewer/widgetapi/widget-proxy.hpp>
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
#	include <gtkmm/widget.h>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
#	include <QWidget>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
#	include <ascension/win32/window.hpp>	// win32.Window
#	include <ObjIdl.h>	// IDataObject
#endif
#include <boost/optional.hpp>

namespace ascension {

	namespace graphics {
		class RenderingContext2D;
		class PaintContext;
	}

	namespace viewers {
		namespace widgetapi {
			/**
			 * Returns the window of the widget.
			 * @param widget The widget
			 * @return The window or @c null
			 */
			Proxy<Window> window(Proxy<Widget> widget);
			Proxy<const Window> cwindow(Proxy<const Widget> widget);

			/**
			 * Returns a bounds of the widget relative to its parent and including/excluding the
			 * window frame.
			 * @param widget The widget
			 * @param includeFrame Set true to include the window frame
			 */
			graphics::Rectangle bounds(Proxy<const Widget> widget, bool includeFrame);
			/**
			 * Translates the point in the global screen coordinates into widget coordinates.
			 * @tparam Point The type of @a position
			 * @param widget The widget
			 * @param position The position to map
			 * @see mapToGlobal
			 */
			template<typename Point>
			Point mapFromGlobal(Proxy<const Widget> widget, const Point& position,
				typename graphics::geometry::detail::EnableIfTagIs<Point, boost::geometry::point_tag>::type* = nullptr);
			/**
			 * Translates the rectangle in the global screen coordinates into widget coordinates.
			 * @tparam Box The type of @a rectangle
			 * @param widget The widget
			 * @param rectangle The rectangle to map
			 * @see mapToGlobal
			 */
			template<typename Box>
			inline Box mapFromGlobal(Proxy<const Widget> widget, const Box& rectangle,
					typename graphics::geometry::detail::EnableIfTagIs<Box, boost::geometry::box_tag>::type* = nullptr) {
				typedef typename boost::geometry::point_type<Box>::type PointType;
				PointType minimumCorner(boost::geometry::make<PointType>(
					boost::geometry::get<boost::geometry::min_corner, 0>(rectangle),
					boost::geometry::get<boost::geometry::min_corner, 1>(rectangle)));
				PointType maximumCorner(boost::geometry::make<PointType>(
					boost::geometry::get<boost::geometry::max_corner, 0>(rectangle),
					boost::geometry::get<boost::geometry::max_corner, 1>(rectangle)));
				boost::geometry::assign(minimumCorner, mapFromGlobal(widget, minimumCorner));
				boost::geometry::assign(maximumCorner, mapFromGlobal(widget, maximumCorner));
				return boost::geometry::make<Box>(
					boost::geometry::get<0>(minimumCorner), boost::geometry::get<1>(minimumCorner),
					boost::geometry::get<0>(maximumCorner), boost::geometry::get<1>(maximumCorner));
			}
			/**
			 * Translates the point in the widget coordinates into global screen coordinates.
			 * @tparam Point The type of @a position
			 * @param widget The widget
			 * @param position The position to map
			 * @see mapFromGlobal
			 */
			template<typename Point>
			Point mapToGlobal(Proxy<const Widget>, const Point& position,
				typename graphics::geometry::detail::EnableIfTagIs<Point, boost::geometry::point_tag>::type* = nullptr);
			/**
			 * Translates the point in the widget coordinates into global screen coordinates.
			 * @tparam Box The type of @a rectangle
			 * @param widget The widget
			 * @param position The position to map
			 * @see mapFromGlobal
			 */
			template<typename Box>
			inline graphics::Rectangle mapToGlobal(Proxy<const Widget> widget, const Box& rectangle,
					typename graphics::geometry::detail::EnableIfTagIs<Box, boost::geometry::box_tag>::type* = nullptr) {
				typedef typename boost::geometry::point_type<Box>::type PointType;
				PointType minimumCorner(boost::geometry::make<PointType>(
					boost::geometry::get<boost::geometry::min_corner, 0>(rectangle),
					boost::geometry::get<boost::geometry::min_corner, 1>(rectangle)));
				PointType maximumCorner(boost::geometry::make<PointType>(
					boost::geometry::get<boost::geometry::max_corner, 0>(rectangle),
					boost::geometry::get<boost::geometry::max_corner, 1>(rectangle)));
				minimumCorner = mapToGlobal(widget, minimumCorner);
				maximumCorner = mapToGlobal(widget, maximumCorner);
				return boost::geometry::make<Box>(
					boost::geometry::get<0>(minimumCorner), boost::geometry::get<1>(minimumCorner),
					boost::geometry::get<0>(maximumCorner), boost::geometry::get<1>(maximumCorner));
			}
			/**
			 * Moves the window to the specified position.
			 * @param window
			 * @param newOrigin The new origin of the window in parent-relative coordinates
			 */
			void move(Proxy<Window> window, const graphics::Point& newOrigin);
			/**
			 * Resizes the window.
			 * @param window
			 * @param newSize The new size of the window excluding any frame
			 */
			void resize(Proxy<Window> window, const graphics::Dimension& newSize);
			/**
			 * Sets the bounds of the widget.
			 * @param widget
			 * @param bounds The new bounds of the widget in parent-relative coordinates excluding
			 *               any window frame
			 */
			void setBounds(Proxy<Widget> widget, const graphics::Rectangle& bounds);
//			/**
//			 * Sets the shape of the widget.
//			 * @param widget
//			 * @param shape The new shape of the widget in parent-relative coordinates excluding
//			 *              any window frame
//			 */
//			void setShape(Proxy<Widget> widget, const graphics::Region& shape);

			// visibilities
			/**
			 * Closes the widget.
			 * @param widget
			 */
			void close(Proxy<Widget> widget);
			/**
			 * Hides the widget.
			 * @param widget
			 * @see #close, #isVisible, #show
			 */
			void hide(Proxy<Widget> widget);
			/**
			 * Returns true if the widget is visible.
			 * @param widget
			 * @see #close, #hide, #show
			 */
			bool isVisible(Proxy<const Widget> widget);
			/**
			 * Lowers the widget to the bottom of the parent widget's stack.
			 * @param widget The widget to lower
			 * @see #raise
			 */
			void lower(Proxy<Widget> widget);
			/**
			 * Raises the window to the top of the parent window's stack.
			 * @param window The window to raise
			 * @see #lower
			 */
			void raise(Proxy<Window> window);
			/**
			 * Returns the level of opacity for the window.
			 * @param widget The widget
			 * @return The level of opacity from 1.0 (completely opaque) to 0.0 (completely
			 *         transparent)
			 * @return #setWindowOpacity
			 */
			double windowOpacity(Proxy<const Widget> widget);
			/**
			 * Sets the level of opacity for the window.
			 * @param widget The widget
			 * @param opacity The level of opacity from 1.0 (completely opaque) to 0.0 (completely
			 *         transparent)
			 * @return #windowOpacity
			 */
			void setWindowOpacity(Proxy<Widget> widget, double opacity);
			void setAlwaysOnTop(Proxy<Widget> widget, bool set);
			/**
			 * Shows the widget.
			 * @param widget
			 * @see #close, #hide, #isVisible
			 */
			void show(Proxy<Widget> widget);

			enum State {
				NORMAL, MAXIMIZED, MINIMIZED
			};
			enum Style {WIDGET = 0};

			// paints
			void forcePaint(Proxy<Widget> widget, const graphics::Rectangle& bounds);
			void redrawScheduledRegion(Proxy<Widget> widget);
			void scheduleRedraw(Proxy<Widget> widget, bool eraseBackground);
			void scheduleRedraw(Proxy<Widget> widget, const graphics::Rectangle& rect, bool eraseBackground);

			// rendering context
			std::unique_ptr<graphics::RenderingContext2D> createRenderingContext(Proxy<const Widget> widget);

			// focus/input
			ascension::detail::ScopeGuard grabInput(Proxy<Widget> widget);
			bool hasFocus(Proxy<const Widget> widget);
			bool isActive(Proxy<const Widget> widget);
			void releaseInput(Proxy<Widget> widget);
			void setFocus(Proxy<Widget> widget);
			void unsetFocus(Proxy<Widget> widget);

			// top-level windows
			/**
			 * Returns @c true if the window is maximized.
			 * @param window The window
			 * @return true if @a window is maximized
			 * @see #isMinimized, #showMaximized
			 */
			bool isMaximized(Proxy<const Window> window);
			/**
			 * Returns @c true if the window is minimized.
			 * @param window The window
			 * @return true if @a window is minimized
			 * @see #isMaximized, #showMinimized
			 */
			bool isMinimized(Proxy<const Window> window);
			/**
			 * Shows the window maximized.
			 * @param window The window
			 * @see #isMaximized, #showMinimized, #showNormal
			 */
			void showMaximized(Proxy<Window> window);
			/**
			 * Shows the window minimized.
			 * @param window The window
			 * @see #isMinimized, #showMaximized, #showNormal
			 */
			void showMinimized(Proxy<Window> window);
			/**
			 * Restores the maximized or minimized window.
			 * @param window The window
			 * @see #isMaximized, #isMinimized, #showMaximized, #showMinimized
			 */
			void showNormal(Proxy<Window> window);

			// hierarchy
			/**
			 * Returns the parent of the widget.
			 * @param widget The widget
			 * @return The parent widget or @c boost#none
			 * @see #setParent
			 */
			Widget::pointer parentWidget(Proxy<const Widget> widget);
			/**
			 * Returns the parent of the widget.
			 * @param widget The widget
			 * @return The parent window or @c boost#none
			 * @see #setParent
			 */
			Window::pointer parentWindow(Proxy<const Widget> widget);
			/**
			 * Sets the parent of the widget.
			 * @param widget The widget
			 * @param newParent The new parent widget or @c null
			 * @see #parent
			 */
			void setParentWidget(Proxy<Widget> widget, Proxy<Widget> newParent);
			/**
			 * Sets the parent of the widget.
			 * @param widget The widget
			 * @param newParent The new parent window or @c null
			 * @see #parent
			 */
			void setParentWindow(Proxy<Widget> widget, Proxy<Window> newParent);
			/***/
			Proxy<Window> rootWindow(Proxy<Widget> widget);
			/***/
			Proxy<const Window> rootWindow(Proxy<const Widget>);

			// drag and drop
			/**
			 * Enables or disables drop events for the widget.
			 * @param widget The widget
			 * @param accept Set @c true to enable drop events
			 * @see #acceptsDrops
			 */
			void acceptDrops(Proxy<Widget> widget, bool accept = true);
			/**
			 * Returns @c true if drop events are enabled for the widget.
			 * @param widget The widget
			 * @return true if drop events are enabled for @a widget
			 * @see #acceptDrops
			 */
			bool acceptsDrops(Proxy<const Widget> widget);

			/// Returns the desktop widget.
			Proxy<Widget> desktop();
		}	// namespace widgetapi
#if 0
		namespace base {

			/**
			 * Thrown by a window object when the method should be called after the initialization.
			 * @see Widget
			 */
			class WidgetNotInitializedException : public IllegalStateException {
			public:
				/// Default constructor.
				WidgetNotInitializedException() BOOST_NOEXCEPT
					: IllegalStateException("this widget is not initialized.") {}
			};


			class Widget : protected DropTarget, public graphics::RenderingDevice {
			public:
				enum State {
					NORMAL, MAXIMIZED, MINIMIZED
				};
				enum Style {WIDGET = 0};
				typedef
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
					std::shared_ptr<std::remove_pointer<HWND>::type>
#endif
					Identifier;
			public:
				// graphics.RenderingDevice
				std::unique_ptr<graphics::RenderingContext2D> createRenderingContext() const;
				int depth();
				std::uint32_t numberOfColors();
				graphics::geometry::Coordinate<graphics::NativeSize>::Type height() const;
				graphics::geometry::Coordinate<graphics::NativeSize>::Type heightInMillimeters() const;
				graphics::geometry::Coordinate<graphics::NativeSize>::Type logicalDpiX() const;
				graphics::geometry::Coordinate<graphics::NativeSize>::Type logicalDpiY() const;
				graphics::geometry::Coordinate<graphics::NativeSize>::Type width() const;
				graphics::geometry::Coordinate<graphics::NativeSize>::Type widthInMillimeters() const;
				graphics::geometry::Coordinate<graphics::NativeSize>::Type physicalDpiX() const;
				graphics::geometry::Coordinate<graphics::NativeSize>::Type physicalDpiY() const;

			protected:
				// DropTarget (default implementations do nothing)
				virtual void dragEntered(DragEnterInput& input) {}
				virtual void dragLeft(DragLeaveInput& input) {}
				virtual void dragMoved(DragMoveInput& input) {}
				virtual void dropped(DropInput& input) {}
				// message handlers
				// TODO: these methods should not be virtual?
				virtual void aboutToClose(bool& reject);
				virtual void aboutToLoseFocus();
				virtual void focusGained();
				virtual void keyPressed(const base::KeyInput& input);
				virtual void keyReleased(const base::KeyInput& input);
				virtual void mouseDoubleClicked(const base::MouseButtonInput& input);
				virtual void mouseHovered(const base::LocatedUserInput& input);
				virtual void mouseLeft(const base::LocatedUserInput& input);
				virtual void mouseMoved(const base::LocatedUserInput& input);
				virtual void mousePressed(const base::MouseButtonInput& input);
				virtual void mouseReleased(const base::MouseButtonInput& input);
				virtual void mouseWheelChanged(const base::MouseWheelInput& input);
				virtual void moved();
				virtual void moving();
				virtual void paint(graphics::PaintContext& context) = 0;
				virtual void resized(State state, const graphics::NativeSize& newSize);
				virtual void resizing();
				virtual void showContextMenu(const base::LocatedUserInput& input, bool byKeyboard);
				virtual void visibilityChanged(bool visible);
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				virtual LRESULT handleWindowSystemEvent(UINT message, WPARAM wp, LPARAM lp, bool& consumed);
#endif
				// window system specific
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
#endif
			private:
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				// Gtk.Widget
				virtual void on_drag_leave(const Glib::RefPtr<Gdk::DragContext>& context, guint time);
				virtual bool on_drag_motion(const Glib::RefPtr<Gdk::DragContext>& context, int x, int y, guint time);
				virtual bool on_drag_drop(const Glib::RefPtr<Gdk::DragContext>& context, int x, int y, guint time);
				virtual void on_drag_data_received(const Glib::RefPtr<Gdk::DragContext>& context, int x, int y, const SelectionData& selection_data, guint info, guint time);
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				// QWidget
				virtual void dragEnterEvent(QDragEnterEvent* event);
				virtual void dragLeaveEvent(QDragLeaveEvent* event);
				virtual void dragMoveEvent(QDragMoveEvent* event);
				virtual void dropEvent(QDropEvent* event);
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				// IDropTarget
				virtual STDMETHODIMP DragEnter(IDataObject* data, DWORD keyState, POINTL pt, DWORD* effect);
				virtual STDMETHODIMP DragOver(DWORD keyState, POINTL pt, DWORD* effect);
				virtual STDMETHODIMP DragLeave();
				virtual STDMETHODIMP Drop(IDataObject* data, DWORD keyState, POINTL pt, DWORD* effect);
#endif

			private:
				Identifier identifier_;
#if !ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				bool acceptsDrops_;
#endif // !ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
			};

		}	// namespace base
#endif // 0

		namespace widgetapi {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
			template<typename Point>
			Point mapFromGlobal(Proxy<const Widget> widget, const Point& position,
					typename graphics::geometry::detail::EnableIfTagIs<Point, boost::geometry::point_tag>::type* /* = nullptr */) {
				const Glib::RefPtr<const Gdk::Window> window(widget->get_window());
				if(!window)
					throw IllegalStateException("The widget passed to widgetapi.mapFromGlobal does not have a window.");
				int rootOriginX, rootOriginY;
				window->get_root_origin(rootOriginX, rootOriginY);
				return graphics::geometry::make<Point>((
					graphics::geometry::_x = graphics::geometry::x(position) - rootOriginX,
					graphics::geometry::_y = graphics::geometry::y(position) - rootOriginY));
			}

			template<typename Point>
			inline Point mapToGlobal(Proxy<const Widget> widget, const Point& position,
					typename graphics::geometry::detail::EnableIfTagIs<Point, boost::geometry::point_tag>::type* /* = nullptr */) {
				const Glib::RefPtr<const Gdk::Window> window(widget->get_window());
				if(!window)
					throw IllegalStateException("The widget passed to widgetapi.mapToGlobal does not have a window.");
				const int localX = static_cast<int>(graphics::geometry::x(position)), localY = static_cast<int>(graphics::geometry::x(position));
				int rootX, rootY;
				Glib::RefPtr<Gdk::Window>::cast_const(window)->get_root_coords(localX, localX, rootX, rootY);	// damn! why is this method not const???
				return graphics::geometry::make<Point>((graphics::geometry::_x = rootX, graphics::geometry::_y = rootY));
			}
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			template<typename Point>
			Point mapFromGlobal(Proxy<const Widget> widget, const Point& position,
					typename detail::EnableIfTagIs<Point, boost::geometry::point_tag>::type* /* = nullptr */) {
				Point temp(position);
				if(!win32::boole(::ScreenToClient(widget.handle().get(), &temp)))
					throw makePlatformError();
				return temp;
			}

			template<typename Point>
			Point mapToGlobal(Proxy<const Widget> widget, const Point& position,
					typename detail::EnableIfTagIs<Point, boost::geometry::point_tag>::type* /* = nullptr */) {
				Point temp(position);
				if(!win32::boole(::ClientToScreen(widget.handle().get(), &temp)))
					throw makePlatformError();
				return temp;
			}
#else
			ASCENSION_CANT_DETECT_PLATFORM();
#endif
		}
	}

#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
	namespace win32 {
		inline Handle<HIMC>::Type inputMethod(const viewers::widgetapi::Proxy<Widget> widget) {
			return Handle<HIMC>::Type(::ImmGetContext(widget.handle().get()),
				std::bind(&::ImmReleaseContext, widget.handle().get(), std::placeholders::_1));
		}
	}
#endif
}

#endif // !ASCENSION_WIDGET_HPP
