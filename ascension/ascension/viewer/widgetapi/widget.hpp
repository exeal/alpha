/**
 * @file widget.hpp
 * @author exeal
 * @date 2011-03-27
 */

#ifndef ASCENSION_WIDGET_HPP
#define ASCENSION_WIDGET_HPP
#include <ascension/corelib/basic-exceptions.hpp>	// IllegalStateException
#include <ascension/corelib/detail/scope-guard.hpp>
#include <ascension/graphics/geometry/dimension.hpp>
#include <ascension/graphics/geometry/rectangle.hpp>
#include <ascension/viewer/widgetapi/widget-proxy.hpp>
#include <boost/geometry/algorithms/make.hpp>
#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/point_type.hpp>
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

	namespace viewer {
		namespace widgetapi {
			/// @defgroup widgetapi_free_functions Free Functions of WidgetAPI
			/// @{
			/**
			 * Returns the window of the widget.
			 * @param widget The widget
			 * @return The window or @c null
			 */
			Proxy<Window> window(Proxy<Widget> widget);
			/**
			 * Returns the window of the widget.
			 * @param widget The widget
			 * @return The window or @c null
			 */
			Proxy<const Window> cwindow(Proxy<const Widget> widget);

			/// @defgroup window_geometries Window Geometries
			/// @{
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
			Point mapToGlobal(Proxy<const Widget> widget, const Point& position,
				typename graphics::geometry::detail::EnableIfTagIs<Point, boost::geometry::point_tag>::type* = nullptr);
			/**
			 * Translates the rectangle in the widget coordinates into global screen coordinates.
			 * @tparam Box The type of @a rectangle
			 * @param widget The widget
			 * @param rectangle The rectangle to map
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
			/// @}

			/// @defgroup visibilities Visibilities
			/// @{
			/**
			 * Closes the widget.
			 * @param widget
			 */
			void close(Proxy<Widget> widget);
			/**
			 * Hides the widget.
			 * @param widget
			 * @see close, isVisible, show
			 */
			void hide(Proxy<Widget> widget);
			/**
			 * Returns true if the widget is visible.
			 * @param widget
			 * @see close, hide, show
			 */
			bool isVisible(Proxy<const Widget> widget);
			/**
			 * Lowers the widget to the bottom of the parent widget's stack.
			 * @param widget The widget to lower
			 * @see raise
			 */
			void lower(Proxy<Widget> widget);
			/**
			 * Raises the window to the top of the parent window's stack.
			 * @param window The window to raise
			 * @see lower
			 */
			void raise(Proxy<Window> window);
			/**
			 * Returns the level of opacity for the window.
			 * @param widget The widget
			 * @return The level of opacity from 1.0 (completely opaque) to 0.0 (completely
			 *         transparent)
			 * @see setWindowOpacity
			 */
			double windowOpacity(Proxy<const Widget> widget);
			/**
			 * Sets the level of opacity for the window.
			 * @param widget The widget
			 * @param opacity The level of opacity from 1.0 (completely opaque) to 0.0 (completely
			 *         transparent)
			 * @see windowOpacity
			 */
			void setWindowOpacity(Proxy<Widget> widget, double opacity);
			void setAlwaysOnTop(Proxy<Widget> widget, bool set);
			/**
			 * Shows the widget.
			 * @param widget
			 * @see close, hide, isVisible
			 */
			void show(Proxy<Widget> widget);
			/// @}

			enum State {
				NORMAL, MAXIMIZED, MINIMIZED
			};
			enum Style {WIDGET = 0};

			/// @defgroup paints Paint
			/// @{
			void forcePaint(Proxy<Widget> widget, const graphics::Rectangle& bounds);
			void redrawScheduledRegion(Proxy<Widget> widget);
			void scheduleRedraw(Proxy<Widget> widget, bool eraseBackground);
			void scheduleRedraw(Proxy<Widget> widget, const graphics::Rectangle& rect, bool eraseBackground);
			void scrollPixels(Proxy<Window> window, const graphics::geometry::BasicDimension<int>& delta);
			void scrollPixels(Proxy<Window> window,
				const graphics::Rectangle& rect, const graphics::geometry::BasicDimension<int>& delta);
			/// @}

			/// @defgroup rendering_context Rendering Context
			/// @{
			/**
			 * Creates and returns a new @c RenderingContext object for the given widget.
			 * @param widget The widget
			 * @return The new @c RenderingContext object
			 */
			std::unique_ptr<graphics::RenderingContext2D> createRenderingContext(Proxy<const Widget> widget);
			/// @}

			/// @defgroup focus_input Focus and Input
			/// @{
			ascension::detail::ScopeGuard grabInput(Proxy<Widget> widget);
			bool hasFocus(Proxy<const Widget> widget);
			bool isActive(Proxy<const Widget> widget);
			void releaseInput(Proxy<Widget> widget);
			void setFocus(Proxy<Widget> widget);
			void unsetFocus(Proxy<Widget> widget);
			/// @}

			class Cursor;

			/// @defgroup window_cursor Window Cursor
			/// @{
#if 0
			/**
			 * Returns the cursor of the given window.
			 * @param window The window
			 * @return The cursor
			 */
			Cursor cursor(Proxy<const Window> window);
#endif
			/**
			 * Sets the cursor for the given window.
			 * @param window The window
			 * @param cursor The cursor to set
			 */
			void setCursor(Proxy<Window> window, const Cursor& cursor);
			/// @}

			/// @defgroup toplevel_windows Top-level Windows
			/// @{
			/**
			 * Returns @c true if the window is maximized.
			 * @param window The window
			 * @return true if @a window is maximized
			 * @see isMinimized, showMaximized
			 */
			bool isMaximized(Proxy<const Window> window);
			/**
			 * Returns @c true if the window is minimized.
			 * @param window The window
			 * @return true if @a window is minimized
			 * @see isMaximized, showMinimized
			 */
			bool isMinimized(Proxy<const Window> window);
			/**
			 * Shows the window maximized.
			 * @param window The window
			 * @see isMaximized, showMinimized, showNormal
			 */
			void showMaximized(Proxy<Window> window);
			/**
			 * Shows the window minimized.
			 * @param window The window
			 * @see isMinimized, showMaximized, showNormal
			 */
			void showMinimized(Proxy<Window> window);
			/**
			 * Restores the maximized or minimized window.
			 * @param window The window
			 * @see isMaximized, isMinimized, showMaximized, showMinimized
			 */
			void showNormal(Proxy<Window> window);
			/// @}

			/// @defgroup hierarchy Hierarchy
			/// @{
			/**
			 * Returns the parent of the widget.
			 * @param widget The widget
			 * @return The parent widget or @c boost#none
			 * @see setParent
			 */
			Widget::pointer parentWidget(Proxy<const Widget> widget);
			/**
			 * Returns the parent of the widget.
			 * @param widget The widget
			 * @return The parent window or @c boost#none
			 * @see setParent
			 */
			Window::pointer parentWindow(Proxy<const Widget> widget);
			/**
			 * Sets the parent of the widget.
			 * @param widget The widget
			 * @param newParent The new parent widget or @c null
			 * @see parent
			 */
			void setParentWidget(Proxy<Widget> widget, Proxy<Widget> newParent);
			/**
			 * Sets the parent of the widget.
			 * @param widget The widget
			 * @param newParent The new parent window or @c null
			 * @see parent
			 */
			void setParentWindow(Proxy<Widget> widget, Proxy<Window> newParent);
			/***/
			Proxy<Window> rootWindow(Proxy<Widget> widget);
			/***/
			Proxy<const Window> rootWindow(Proxy<const Widget>);
			/// @}

			/// @defgroup drag_and_drop Drag And Drop
			/// @{
			/**
			 * Enables or disables drop events for the widget.
			 * @param widget The widget
			 * @param accept Set @c true to enable drop events
			 * @see acceptsDrops
			 */
			void acceptDrops(Proxy<Widget> widget, bool accept = true);
			/**
			 * Returns @c true if drop events are enabled for the widget.
			 * @param widget The widget
			 * @return true if drop events are enabled for @a widget
			 * @see acceptDrops
			 */
			bool acceptsDrops(Proxy<const Widget> widget);
			/// @}

			/// Returns the desktop widget.
			Proxy<Widget> desktop();

#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
			template<typename Point>
			Point mapFromGlobal(Proxy<const Widget> widget, const Point& position,
					typename graphics::geometry::detail::EnableIfTagIs<Point, boost::geometry::point_tag>::type* /* = nullptr */) {
				const Glib::RefPtr<const Gdk::Window> window(widget->get_window());
				if(!window)
					throw IllegalStateException("The widget passed to widgetapi.mapFromGlobal does not have a window.");
				int rootOriginX, rootOriginY;
				window->get_root_origin(rootOriginX, rootOriginY);
				return boost::geometry::make<Point>(boost::geometry::get<0>(position) - rootOriginX, boost::geometry::get<1>(position) - rootOriginY);
			}

			template<typename Point>
			inline Point mapToGlobal(Proxy<const Widget> widget, const Point& position,
					typename graphics::geometry::detail::EnableIfTagIs<Point, boost::geometry::point_tag>::type* /* = nullptr */) {
				const Glib::RefPtr<const Gdk::Window> window(widget->get_window());
				if(!window)
					throw IllegalStateException("The widget passed to widgetapi.mapToGlobal does not have a window.");
				const int localX = static_cast<int>(boost::geometry::get<0>(position)), localY = static_cast<int>(boost::geometry::get<1>(position));
				int rootX, rootY;
				Glib::RefPtr<Gdk::Window>::cast_const(window)->get_root_coords(localX, localX, rootX, rootY);	// damn! why is this method not const???
				return boost::geometry::make<Point>(rootX, rootY);
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
			/// @}
		}
	}

#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
	namespace win32 {
		inline Handle<HIMC> inputMethod(const viewer::widgetapi::Proxy<viewer::widgetapi::Widget> widget) {
			return Handle<HIMC>(::ImmGetContext(widget.get()->handle().get()),
				std::bind(&::ImmReleaseContext, widget.get()->handle().get(), std::placeholders::_1));
		}
	}
#endif
}

#endif // !ASCENSION_WIDGET_HPP
