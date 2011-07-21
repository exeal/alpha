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
#include <ascension/graphics/rendering-device.hpp>	// graphics.RenderingDevice, ...
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


			class Widget : public graphics::RenderingDevice {
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

				virtual graphics::NativeRectangle bounds(bool includeFrame) const = 0;
				virtual graphics::NativePoint mapFromGlobal(const graphics::NativePoint& position) const = 0;
				graphics::NativeRectangle mapFromGlobal(const graphics::NativeRectangle& rectangle) const;
				virtual graphics::NativePoint mapToGlobal(const graphics::NativePoint& position) const = 0;
				graphics::NativeRectangle mapToGlobal(const graphics::NativeRectangle& rectangle) const;
				void move(const graphics::NativePoint& newOrigin);
				void resize(const graphics::NativeSize& newSize);
				virtual void setBounds(const graphics::NativeRectangle& bounds) = 0;
				virtual void setShape(const graphics::NativeRegion& shape) = 0;
 
				virtual void close() = 0;
				virtual void hide() = 0;
				void lower();
				void raise();
				virtual void show() = 0;

				virtual void forcePaint(const graphics::NativeRectangle& bounds) = 0;
				void redrawScheduledRegion();
				void scheduleRedraw(bool eraseBackground);
				void scheduleRedraw(const graphics::NativeRectangle& rect, bool eraseBackground);

				virtual void setOpacity(double opacity) = 0;
				virtual void setAlwaysOnTop(bool set) = 0;

				bool hasFocus() const;
				virtual bool isVisible() const = 0;
				virtual bool isActive() const = 0;
				void setFocus();

				virtual std::auto_ptr<InputGrabLocker> grabInput() = 0;
				virtual void releaseInput() = 0;

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
				Identifier identifier_;
			};

			class ScrollBar {
			public:
				int pageStep() const;
				int position() const;
				Range<int> range() const;
				void setPageStep(int pageStep);
				void setPosition(int position);
				void setRange(const Range<int>& range);
			};

			class ScrollableWidget : public Widget {
			public:
				ScrollableWidget(Widget* parent = 0, Style styles = WIDGET);
				ScrollBar& horizontalScrollBar() const;
				ScrollBar& verticalScrollBar() const;
			};

		}
	}
}

#endif // !ASCENSION_WIDGET_HPP
