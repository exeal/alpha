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
#include <ascension/viewer/base/drag-and-drop.hpp>
#include <ascension/viewer/base/user-input.hpp>
#if defined(ASCENSION_WINDOW_SYSTEM_GTK)
#	include <gtkmm/widget.h>
#elif defined(ASCENSION_WINDOW_SYSTEM_QT)
#	include <QWidget>
#elif defined(ASCENSION_WINDOW_SYSTEM_QUARTZ)
#elif defined(ASCENSION_WINDOW_SYSTEM_WIN32)
#	include <ascension/win32/windows.hpp>
#	include <ObjIdl.h>	// IDataObject
#endif

namespace ascension {

	namespace graphics {
		class PaintContext;
	}

	namespace viewers {
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
					win32::Handle<HWND>
#endif
					Identifier;

				class InputGrabLocker {
				public:
					~InputGrabLocker() {widget_.releaseInput();}
				private:
					explicit InputGrabLocker(Widget& widget) : widget_(widget) {}
					Widget& widget_;
					friend class Widget;
				};
			public:
				Widget(Widget* parent = 0, Style styles = WIDGET);
				virtual ~Widget();

				const Identifier& identifier() const;

				graphics::NativeRectangle bounds(bool includeFrame) const;
				graphics::NativePoint mapFromGlobal(const graphics::NativePoint& position) const;
				graphics::NativeRectangle mapFromGlobal(const graphics::NativeRectangle& rectangle) const {
					return graphics::geometry::make<graphics::NativeRectangle>(
						mapFromGlobal(graphics::geometry::get<0>(rectangle)),
						mapFromGlobal(graphics::geometry::get<1>(rectangle)));
				}
				graphics::NativePoint mapToGlobal(const graphics::NativePoint& position) const;
				graphics::NativeRectangle mapToGlobal(const graphics::NativeRectangle& rectangle) const {
					return graphics::geometry::make<graphics::NativeRectangle>(
						mapToGlobal(graphics::geometry::get<0>(rectangle)),
						mapToGlobal(graphics::geometry::get<1>(rectangle)));
				}
				void move(const graphics::NativePoint& newOrigin);
				void resize(const graphics::NativeSize& newSize);
				void setBounds(const graphics::NativeRectangle& bounds);
				void setShape(const graphics::NativeRegion& shape);
 
				void close();
				void hide();
				void lower();
				void raise();
				void show();

				void forcePaint(const graphics::NativeRectangle& bounds);
				void redrawScheduledRegion();
				void scheduleRedraw(bool eraseBackground);
				void scheduleRedraw(const graphics::NativeRectangle& rect, bool eraseBackground);

				void setOpacity(double opacity);
				void setAlwaysOnTop(bool set);

				bool hasFocus() const;
				bool isVisible() const;
				bool isActive() const;
				void setFocus();

				std::auto_ptr<InputGrabLocker> grabInput();
				void releaseInput();

				void acceptDrops(bool accept = true);
				bool acceptsDrops() const;

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
				struct ClassInformation {
					UINT style;	// corresponds to WNDCLASSEXW.style
					/// Makes a brush handle parameter from either a brush handle or @c COLORREF value. 
					class Background {
					public:
						/// Constructor makes @c null @c HBRUSH value.
						Background() /*throw()*/ : brush_(0) {}
						/// Constructor takes a brush handle.
						Background(win32::Handle<HBRUSH> handle) /*throw()*/ : brush_(handle.release()) {}
						/// Constructor takes a @c COLORREF value used to make the brush handle.
						Background(int systemColor) /*throw()*/
							: brush_(reinterpret_cast<HBRUSH>(static_cast<HANDLE_PTR>(systemColor + 1))) {}
						/// Returns the brush handle.
						HBRUSH get() const /*throw()*/ {return brush_;}
					private:
						HBRUSH brush_;
					} background;
					win32::Handle<HICON> icon, smallIcon;
					/// Makes a cursor handle parameter from either a cursor handle or numeric identifier.
					class CursorHandleOrID {
					public:
						/// Constructor makes @c null @c HCURSOR value.
						CursorHandleOrID() /*throw()*/ : cursor_(0) {}
						/// Constructor takes a cursor handle.
						CursorHandleOrID(const win32::Handle<HCURSOR>& handle) /*throw()*/ : cursor_(handle.get()) {}
						/// Constructor takes a numeric identifier for system cursor.
						CursorHandleOrID(const WCHAR* systemCursorID) : cursor_(::LoadCursorW(0, systemCursorID)) {}
						/// Returns the cursor handle.
						HCURSOR get() const /*throw()*/ {return cursor_;}
					private:
						HCURSOR cursor_;
					} cursor;
					ClassInformation() : style(0) {}
				};
				virtual void provideClassInformation(ClassInformation& classInfomation) const {}
				virtual std::basic_string<WCHAR> provideClassName() const = 0;
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
	}
}

#endif // !ASCENSION_WIDGET_HPP
