/**
 * @file widget.hpp
 * @author exeal
 * @date 2011-03-27
 */

#ifndef ASCENSION_WIDGET_HPP
#define ASCENSION_WIDGET_HPP
#include <ascension/platforms.hpp>
#include <ascension/corelib/basic-exceptions.hpp>	// IllegalStateException
#include <ascension/graphics/geometry.hpp>
#include <ascension/graphics/rendering-device.hpp>	// graphics.RenderingDevice, ...
#include <ascension/viewer/widgetapi/drag-and-drop.hpp>
#include <ascension/viewer/widgetapi/user-input.hpp>
#if defined(ASCENSION_WINDOW_SYSTEM_GTK)
#	include <gtkmm/widget.h>
#elif defined(ASCENSION_WINDOW_SYSTEM_QT)
#	include <QWidget>
#elif defined(ASCENSION_WINDOW_SYSTEM_QUARTZ)
#elif defined(ASCENSION_WINDOW_SYSTEM_WIN32)
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
#if defined(ASCENSION_WINDOW_SYSTEM_GTK)
			typedef Gtk::Widget NativeWidget;
		typedef Gtk::Window NativeWindow;
#elif defined(ASCENSION_WINDOW_SYSTEM_QT)
			typedef QWidget NativeWidget;
			typedef QWidget NativeWindow;
#elif defined(ASCENSION_WINDOW_SYSTEM_QUARTZ)
			typedef NSView NativeWidget;
			typedef NSWindow NativeWindow;
#elif defined(ASCENSION_WINDOW_SYSTEM_WIN32)
			typedef win32::Window NativeWidget;
			typedef win32::Window NativeWindow;
#endif
			/**
			 * Returns a bounds of the widget relative to its parent and including/excluding the
			 * window frame.
			 * @param widget The widget
			 * @param includeFrame Set true to include the window frame
			 */
			graphics::Rectangle bounds(const NativeWidget& widget, bool includeFrame);
			/**
			 * Translates the point in the global screen coordinates into widget coordinates.
			 * @tparam Point The type of @a position
			 * @param widget The widget
			 * @param position The position to map
			 * @see mapToGlobal
			 */
			template<typename Point>
			Point mapFromGlobal(const NativeWidget& widget, const Point& position,
				typename detail::EnableIfTagIs<Point, boost::geometry::point_tag>::type* = nullptr);
			/**
			 * Translates the rectangle in the global screen coordinates into widget coordinates.
			 * @tparam Box The type of @a rectangle
			 * @param widget The widget
			 * @param rectangle The rectangle to map
			 * @see mapToGlobal
			 */
			template<typename Box>
			inline Box mapFromGlobal(const NativeWidget& widget, const Box& rectangle,
					typename detail::EnableIfTagIs<Box, boost::geometry::box_tag>::type* = nullptr) {
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
			Point mapToGlobal(const NativeWidget& widget, const Point& position,
				typename detail::EnableIfTagIs<Point, boost::geometry::point_tag>::type* = nullptr);
			/**
			 * Translates the point in the widget coordinates into global screen coordinates.
			 * @tparam Box The type of @a rectangle
			 * @param widget The widget
			 * @param position The position to map
			 * @see mapFromGlobal
			 */
			template<typename Box>
			inline graphics::Rectangle mapToGlobal(const NativeWidget& widget, const Box& rectangle,
					typename detail::EnableIfTagIs<Box, boost::geometry::box_tag>::type* = nullptr) {
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
			 * Moves the widget to the specified position.
			 * @param widget
			 * @param newOrigin The new origin of the widget in parent-relative coordinates
			 */
			void move(NativeWidget& widget, const graphics::Point& newOrigin);
			/**
			 * Resizes the widget.
			 * @param widget
			 * @param newSize The new size of the widget excluding any window frame
			 */
			void resize(NativeWidget& widget, const graphics::Dimension& newSize);
			/**
			 * Sets the bounds of the widget.
			 * @param widget
			 * @param bounds The new bounds of the widget in parent-relative coordinates excluding
			 *               any window frame
			 */
			void setBounds(NativeWidget& widget, const graphics::Rectangle& bounds);
//			/**
//			 * Sets the shape of the widget.
//			 * @param widget
//			 * @param shape The new shape of the widget in parent-relative coordinates excluding
//			 *              any window frame
//			 */
//			void setShape(NativeWidget& widget, const graphics::Region& shape);

			// visibilities
			/**
			 * Closes the widget.
			 * @param widget
			 */
			void close(NativeWidget& widget);
			/**
			 * Hides the widget.
			 * @param widget
			 * @see #close, #isVisible, #show
			 */
			void hide(NativeWidget& widget);
			/**
			 * Returns true if the widget is visible.
			 * @param widget
			 * @see #close, #hide, #show
			 */
			bool isVisible(const NativeWidget& widget);
			/**
			 * Lowers the widget to the bottom of the parent widget's stack.
			 * @param widget The widget to lower
			 * @see #raise
			 */
			void lower(NativeWidget& widget);
			/**
			 * Raises the widget to the top of the parent widget's stack.
			 * @param widget The widget to raise
			 * @see #lower
			 */
			void raise(NativeWidget& widget);
			/**
			 * Returns the level of opacity for the window.
			 * @param widget The widget
			 * @return The level of opacity from 1.0 (completely opaque) to 0.0 (completely
			 *         transparent)
			 * @return #setWindowOpacity
			 */
			double windowOpacity(const NativeWidget& widget);
			/**
			 * Sets the level of opacity for the window.
			 * @param widget The widget
			 * @param opacity The level of opacity from 1.0 (completely opaque) to 0.0 (completely
			 *         transparent)
			 * @return #windowOpacity
			 */
			void setWindowOpacity(NativeWidget& widget, double opacity);
			void setAlwaysOnTop(NativeWidget& widget, bool set);
			/**
			 * Shows the widget.
			 * @param widget
			 * @see #close, #hide, #isVisible
			 */
			void show(NativeWidget& widget);

			enum State {
				NORMAL, MAXIMIZED, MINIMIZED
			};
			enum Style {WIDGET = 0};

			// paints
			void forcePaint(NativeWidget& widget, const graphics::Rectangle& bounds);
			void redrawScheduledRegion(NativeWidget& widget);
			void scheduleRedraw(NativeWidget& widget, bool eraseBackground);
			void scheduleRedraw(NativeWidget& widget, const graphics::Rectangle& rect, bool eraseBackground);

			// rendering context
			std::unique_ptr<graphics::RenderingContext2D> createRenderingContext(const NativeWidget& widget);

			// focus/input
			void grabInput(NativeWidget& widget);
			bool hasFocus(const NativeWidget& widget);
			bool isActive(const NativeWidget& widget);
			void releaseInput(NativeWidget& widget);
			void setFocus();
			void setFocus(NativeWidget& widget);
			class InputGrabLocker {
			public:
				~InputGrabLocker() {releaseInput(widget_);}
			private:
				explicit InputGrabLocker(NativeWidget& widget) : widget_(widget) {}
				NativeWidget& widget_;
			};

			// top-level windows
			/**
			 * Returns @c true if the widget is maximized.
			 * @param widget The widget
			 * @return true if @a widget is maximized
			 * @see #isMinimized, #showMaximized
			 */
			bool isMaximized(const NativeWidget& widget);
			/**
			 * Returns @c true if the widget is minimized.
			 * @param widget The widget
			 * @return true if @a widget is minimized
			 * @see #isMaximized, #showMinimized
			 */
			bool isMinimized(const NativeWidget& widget);
			/**
			 * Shows the widget maximized.
			 * @param widget The widget
			 * @see #isMaximized, #showMinimized, #showNormal
			 */
			void showMaximized(NativeWidget& widget);
			/**
			 * Shows the widget minimized.
			 * @param widget The widget
			 * @see #isMinimized, #showMaximized, #showNormal
			 */
			void showMinimized(NativeWidget& widget);
			/**
			 * Restores the maximized or minimized widget.
			 * @param widget The widget
			 * @see #isMaximized, #isMinimized, #showMaximized, #showMinimized
			 */
			void showNormal(NativeWidget& widget);

			// hierarchy
			/**
			 * Returns the parent of the widget.
			 * @param widget The widget
			 * @return The parent widget or @c boost#none
			 * @see #setParent
			 */
			boost::optional<NativeWidget> parent(const NativeWidget& widget);
			/**
			 * Sets the parent of the widget.
			 * @param widget The widget
			 * @param newParent The new parent widget or @c null
			 * @see #parent
			 */
			void setParent(NativeWidget& widget, NativeWidget* newParent);

			// drag and drop
			/**
			 * Enables or disables drop events for the widget.
			 * @param widget The widget
			 * @param accept Set @c true to enable drop events
			 * @see #acceptsDrops
			 */
			void acceptDrops(NativeWidget& widget, bool accept = true);
			/**
			 * Returns @c true if drop events are enabled for the widget.
			 * @param widget The widget
			 * @return true if drop events are enabled for @a widget
			 * @see #acceptDrops
			 */
			bool acceptsDrops(const NativeWidget& widget);

			/// Returns the desktop window.
			NativeWindow desktop();
		}
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
#if defined(ASCENSION_WINDOW_SYSTEM_WIN32)
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
#if defined(ASCENSION_WINDOW_SYSTEM_WIN32)
				virtual LRESULT handleWindowSystemEvent(UINT message, WPARAM wp, LPARAM lp, bool& consumed);
#endif
				// window system specific
#if defined(ASCENSION_WINDOW_SYSTEM_WIN32)
#endif
			private:
#if defined(ASCENSION_WINDOW_SYSTEM_GTK)
				// Gtk.Widget
				virtual void on_drag_leave(const Glib::RefPtr<Gdk::DragContext>& context, guint time);
				virtual bool on_drag_motion(const Glib::RefPtr<Gdk::DragContext>& context, int x, int y, guint time);
				virtual bool on_drag_drop(const Glib::RefPtr<Gdk::DragContext>& context, int x, int y, guint time);
				virtual void on_drag_data_received(const Glib::RefPtr<Gdk::DragContext>& context, int x, int y, const SelectionData& selection_data, guint info, guint time);
#elif defined(ASCENSION_WINDOW_SYSTEM_QT)
				// QWidget
				virtual void dragEnterEvent(QDragEnterEvent* event);
				virtual void dragLeaveEvent(QDragLeaveEvent* event);
				virtual void dragMoveEvent(QDragMoveEvent* event);
				virtual void dropEvent(QDropEvent* event);
#elif defined(ASCENSION_WINDOW_SYSTEM_QUARTZ)
#elif defined(ASCENSION_WINDOW_SYSTEM_WIN32)
				// IDropTarget
				virtual STDMETHODIMP DragEnter(IDataObject* data, DWORD keyState, POINTL pt, DWORD* effect);
				virtual STDMETHODIMP DragOver(DWORD keyState, POINTL pt, DWORD* effect);
				virtual STDMETHODIMP DragLeave();
				virtual STDMETHODIMP Drop(IDataObject* data, DWORD keyState, POINTL pt, DWORD* effect);
#endif

			private:
				Identifier identifier_;
#ifndef ASCENSION_WINDOW_SYSTEM_QT
				bool acceptsDrops_;
#endif // !ASCENSION_WINDOW_SYSTEM_QT
			};

		}
#endif // 0
	}
}

#if defined(ASCENSION_WINDOW_SYSTEM_GTK)
#	include <ascension/viewer/widgetapi/widget-gtk.hpp>
#elif defined(ASCENSION_WINDOW_SYSTEM_QT)
#	include <ascension/viewer/widgetapi/widget-qt.hpp>
#elif defined(ASCENSION_WINDOW_SYSTEM_QUARTZ)
#	include <ascension/viewer/widgetapi/widget-osx.hpp>
#elif defined(ASCENSION_WINDOW_SYSTEM_WIN32)
#	include <ascension/viewer/widgetapi/widget-windows.hpp>
#endif

#endif // !ASCENSION_WIDGET_HPP
