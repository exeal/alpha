/**
 * @file widget-windows.hpp
 * @author exeal
 * @date 2011-03-26 separated from window-windows.hpp
 */

#ifndef ASCENSION_WIDGET_WINDOWS_HPP
#define ASCENSION_WIDGET_WINDOWS_HPP
#include <ascension/viewer/base/widget.hpp>
#include <ascension/viewer/base/message-dispatcher-windows.hpp>

namespace ascension {
	namespace win32 {
#if 0
		/// Makes a menu handle parameter from either a menu handle or numeric identifier.
		class MenuHandleOrControlID {
		public:
			/// Constructor takes a menu handle.
			MenuHandleOrControlID(HMENU handle) /*throw()*/ : handle_(handle) {}
			/// Constructor takes a numeric identifier.
			MenuHandleOrControlID(UINT_PTR id) /*throw()*/ : handle_(reinterpret_cast<HMENU>(id)) {}
			/// Returns the menu handle.
			HMENU get() const /*throw()*/ {return handle_;}
		private:
			HMENU handle_;
		};
#endif
		class WidgetBase : public viewers::base::Widget {
		public:
			static const DWORD defaultStyle = WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE;
		public:
			const Handle<HWND>& handle() const;
			void initialize(const Handle<HWND>& parent,
				const graphics::Point<>& position = graphics::Point<>(CW_USEDEFAULT, CW_USEDEFAULT),
				const graphics::Dimension<>& size = graphics::Dimension<>(CW_USEDEFAULT, CW_USEDEFAULT),
				DWORD style = 0, DWORD extendedStyle = 0);
			bool isWindow() const /*throw()*/;
			void scheduleRedraw(bool eraseBackground);
			void scheduleRedraw(const graphics::Rect<>& rect, bool eraseBackground);
			// Win32-specific scrolling methods
			void scrollInformation(int bar, SCROLLINFO& scrollInfo, UINT mask = SIF_ALL) const;
			int scrollPosition(int bar) const;
			Range<int> scrollRange(int bar) const;
			int scrollTrackPosition(int bar) const;
			void setScrollInformation(int bar, const SCROLLINFO& scrollInfo, bool redraw = true);
			int setScrollPosition(int bar, int pos, bool redraw = true);
			void setScrollRange(int bar, const Range<int>& range, bool redraw = true);
			bool hasFocus() const /*throw()*/;
			void redrawScheduledRegion();
			// viewers.base.Widget
			graphics::Rect<> bounds(bool includeFrame) const;
			void hide();
			bool isVisible() const /*throw()*/;
			void setBounds(const graphics::Rect<>& bounds);
			void show();
		protected:
			struct ClassInformation {
				UINT style;	// corresponds to WNDCLASSEXW.style
				/// Makes a brush handle parameter from either a brush handle or @c COLORREF value. 
				class Background {
				public:
					/// Constructor makes @c null @c HBRUSH value.
					Background() /*throw()*/ : brush_(nullptr) {}
					/// Constructor takes a brush handle.
					Background(Handle<HBRUSH> handle) /*throw()*/ : brush_(handle.release()) {}
					/// Constructor takes a @c COLORREF value used to make the brush handle.
					Background(int systemColor) /*throw()*/
						: brush_(reinterpret_cast<HBRUSH>(static_cast<HANDLE_PTR>(systemColor + 1))) {}
					/// Returns the brush handle.
					HBRUSH get() const /*throw()*/ {return brush_;}
				private:
					HBRUSH brush_;
				} background;
				Handle<HICON> icon, smallIcon;
				/// Makes a cursor handle parameter from either a cursor handle or numeric identifier.
				class CursorHandleOrID {
				public:
					/// Constructor makes @c null @c HCURSOR value.
					CursorHandleOrID() /*throw()*/ : cursor_(nullptr) {}
					/// Constructor takes a cursor handle.
					CursorHandleOrID(const Handle<HCURSOR>& handle) /*throw()*/ : cursor_(handle.get()) {}
					/// Constructor takes a numeric identifier for system cursor.
					CursorHandleOrID(const WCHAR* systemCursorID) : cursor_(::LoadCursorW(nullptr, systemCursorID)) {}
					/// Returns the cursor handle.
					HCURSOR get() const /*throw()*/ {return cursor_;}
				private:
					HCURSOR cursor_;
				} cursor;
				ClassInformation() : style(0) {}
			};
		protected:
			virtual LRESULT processMessage(UINT message, WPARAM wp, LPARAM lp, bool& consumed) = 0;
			virtual void provideClassInformation(ClassInformation& classInfomation) const {}
			virtual std::basic_string<WCHAR> provideClassName() const = 0;
		private:
			graphics::Point<> clientToScreen(const graphics::Point<>& p) const;
			graphics::Point<> screenToClient(const graphics::Point<>& p) const;
			static LRESULT CALLBACK windowProcedure(HWND window, UINT message, WPARAM wp, LPARAM lp);
		private:
			Handle<HWND> handle_;
		};

		template<typename Derived>
		class Widget : public WidgetBase {
		private:
			LRESULT processMessage(UINT message, WPARAM wp, LPARAM lp, bool& consumed) {
				MessageDispatcher<Derived>::processMessage(*this, message, wp, lp, consumed);
			}
		};

	}
}

#endif // !ASCENSION_WIDGET_WINDOWS_HPP
