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

namespace ascension {

	namespace graphics {
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
			graphics::NativeRectangle bounds(const NativeWidget& widget, bool includeFrame);
			template<typename Point>
			graphics::NativePoint mapFromGlobal(const NativeWidget& widget, const Point& position,
				typename detail::EnableIfTagIs<Point, graphics::geometry::PointTag>::type* = nullptr);
			inline graphics::NativeRectangle mapFromGlobal(const NativeWidget& widget, const graphics::NativeRectangle& rectangle) {
				return graphics::geometry::make<graphics::NativeRectangle>(
					mapFromGlobal(widget, graphics::geometry::get<0>(rectangle)),
					mapFromGlobal(widget, graphics::geometry::get<1>(rectangle)));
			}
			template<typename Point>
			graphics::NativePoint mapToGlobal(const NativeWidget& widget, const Point& position,
				typename detail::EnableIfTagIs<Point, graphics::geometry::PointTag>::type* = nullptr);
			inline graphics::NativeRectangle mapToGlobal(const NativeWidget& widget, const graphics::NativeRectangle& rectangle) {
				return graphics::geometry::make<graphics::NativeRectangle>(
					mapToGlobal(widget, graphics::geometry::get<0>(rectangle)),
					mapToGlobal(widget, graphics::geometry::get<1>(rectangle)));
			}
			void move(const NativeWidget& widget, const graphics::NativePoint& newOrigin);
			void resize(const NativeWidget& widget, const graphics::NativeSize& newSize);
			void setBounds(const NativeWidget& widget, const graphics::NativeRectangle& bounds);
			void setShape(const NativeWidget& widget, const graphics::NativeRegion& shape);

			// visibilities
			void close(NativeWidget& widget);
			void hide(NativeWidget& widget);
			bool isVisible(const NativeWidget& widget);
			void lower(NativeWidget& widget);
			void raise(NativeWidget& widget);
			void setOpacity(NativeWidget& widget, double opacity);
			void setAlwaysOnTop(NativeWidget& widget, bool set);
			void show(NativeWidget& widget);

			enum State {
				NORMAL, MAXIMIZED, MINIMIZED
			};
			enum Style {WIDGET = 0};

			// paints
			void forcePaint(NativeWidget& widget, const graphics::NativeRectangle& bounds);
			void redrawScheduledRegion(NativeWidget& widget);
			void scheduleRedraw(NativeWidget& widget, bool eraseBackground);
			void scheduleRedraw(NativeWidget& widget, const graphics::NativeRectangle& rect, bool eraseBackground);

			// focus/input
			void grabInput(NativeWidget& widget);
			bool hasFocus(const NativeWidget& widget);
			bool isActive(const NativeWidget& widget);
			void releaseInput(NativeWidget& widget);
			void setFocus(NativeWidget& widget);
			class InputGrabLocker {
			public:
				~InputGrabLocker() {releaseInput(widget_);}
			private:
				explicit InputGrabLocker(NativeWidget& widget) : widget_(widget) {}
				NativeWidget& widget_;
			};

			// hierarchy
			NativeWidget* parent(const NativeWidget& widget);
			void setParent();
			void setParent(NativeWidget& newParent);

			// drag and drop
			void acceptDrops(NativeWidget& widget, bool accept = true);
			bool acceptsDrops(const NativeWidget& widget);

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
				WidgetNotInitializedException() /*throw()*/
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
				uint32_t numberOfColors();
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
