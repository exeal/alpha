/**
 * @file widget.hpp
 * @author exeal
 * @date 2011-03-27
 */

#ifndef ASCENSION_WIDGET_HPP
#define ASCENSION_WIDGET_HPP
#include <ascension/platforms.hpp>
#if defined(ASCENSION_WINDOW_SYSTEM_WIN32)
#	include <ascension/win32/windows.hpp>
#endif
#include <ascension/corelib/basic-exceptions.hpp>	// IllegalStateException
#include <ascension/graphics/geometry.hpp>
#include <ascension/graphics/graphics.hpp>			// graphics.Device, ...
#include <ascension/viewer/base/user-input.hpp>

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


			class Widget {
			public:
				enum State {
					NORMAL, MAXIMIZED, MINIMIZED
				};
				typedef
#if defined(ASCENSION_WINDOW_SYSTEM_WIN32)
					win32::Handle<HWND>
#endif
					Identifier;
			public:
				virtual ~Widget();

				const Identifier& identifier() const;

				virtual void initialize(Widget& parent, const graphics::Rect<>& bounds) = 0;

				virtual graphics::Rect<> bounds(bool includeFrame) const = 0;
				virtual void setBounds(const graphics::Rect<>& bounds) = 0;
				virtual void setShape(const graphics::NativePolygon& shape) = 0;
 
				virtual void close() = 0;
				virtual void show() = 0;
				virtual void hide() = 0;

				virtual void forcePaint(const graphics::Rect<>& bounds) = 0;
				void redrawScheduledRegion();
				void scheduleRedraw(bool eraseBackground);
				void scheduleRedraw(const graphics::Rect<>& rect, bool eraseBackground);

				virtual void setOpacity(double opacity) = 0;
				virtual void setAlwaysOnTop(bool set) = 0;

				virtual bool isVisible() const = 0;
				virtual bool isActive() const = 0;

			protected:
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
				virtual void resized(State state, const graphics::Dimension<>& newSize);
				virtual void resizing();
				virtual void showContextMenu(const base::LocatedUserInput& input);
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
				Identifier identifier_;
			};

		}
	}
}

#endif // !ASCENSION_WIDGET_HPP
