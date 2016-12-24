/**
 * @file custom-control.hpp
 * @author exeal
 * @date 2011-03-26 Separated from window-windows.hpp.
 * @date 2012-03-07 Renamed from widget-windows.hpp.
 * @date 2016-12-24 Separated from window.hpp.
 */

#ifndef ASCENSION_CUSTOM_CONTROL_HPP
#define ASCENSION_CUSTOM_CONTROL_HPP
#include <ascension/win32/window/window.hpp>
#include <ascension/win32/window/detail/message-dispatcher.hpp>

namespace ascension {
	namespace win32 {
		class CustomControl : public Window {
		public:
			struct ClassInformation {
				UINT style;	// corresponds to WNDCLASSEXW.style
				/// Makes a brush handle parameter from either a brush handle or @c COLORREF value. 
				class Background {
				public:
					/// Constructor makes @c null @c HBRUSH value.
					Background() BOOST_NOEXCEPT : brush_(nullptr) {}
					/// Constructor takes a brush handle.
					Background(Handle<HBRUSH> handle) BOOST_NOEXCEPT : brush_(handle) {}
					/// Constructor takes a @c COLORREF value used to make the brush handle.
					Background(int systemColor) BOOST_NOEXCEPT
						: brush_(reinterpret_cast<HBRUSH>(static_cast<HANDLE_PTR>(systemColor + 1))) {}
					/// Move-constructor.
					Background(Background&& other) BOOST_NOEXCEPT : brush_(std::move(other.brush_)) {}
					/// Move-assignment operator.
					Background& operator=(Background&& other) BOOST_NOEXCEPT {
						return (brush_ = std::move(other.brush_)), *this;
					}
					/// Returns the brush handle.
					Handle<HBRUSH> get() const BOOST_NOEXCEPT {return brush_;}
				private:
					Handle<HBRUSH> brush_;
				} background;
				Handle<HICON> icon, smallIcon;
				/// Makes a cursor handle parameter from either a cursor handle or numeric identifier.
				class CursorHandleOrID {
				public:
					/// Constructor makes @c null @c HCURSOR value.
					CursorHandleOrID() BOOST_NOEXCEPT : cursor_(nullptr) {}
					/// Constructor takes a cursor handle.
					CursorHandleOrID(Handle<HCURSOR> handle) BOOST_NOEXCEPT : cursor_(handle) {}
					/// Constructor takes a numeric identifier for system cursor.
					CursorHandleOrID(const WCHAR* systemCursorID) BOOST_NOEXCEPT : cursor_(::LoadCursorW(nullptr, systemCursorID)) {}
					/// Move-constructor.
					CursorHandleOrID(CursorHandleOrID&& other) BOOST_NOEXCEPT : cursor_(std::move(other.cursor_)) {}
					/// Move-assignment operator.
					CursorHandleOrID& operator=(CursorHandleOrID&& other) BOOST_NOEXCEPT {
						return (cursor_ = std::move(other.cursor_)), *this;
					}
					/// Returns the cursor handle.
					Handle<HCURSOR> get() const BOOST_NOEXCEPT {return cursor_;}
				private:
					Handle<HCURSOR> cursor_;
				} cursor;
				ClassInformation() : style(0) {}
			};
			CustomControl();
		protected:
			virtual LRESULT processMessage(UINT message, WPARAM wp, LPARAM lp, bool& consumed) = 0;
			virtual void provideClassInformation(ClassInformation& classInfomation) const {}
			virtual std::basic_string<WCHAR> provideClassName() const = 0;
		private:
			static LRESULT CALLBACK windowProcedure(HWND window, UINT message, WPARAM wp, LPARAM lp) {
				return messageDispatcher_.dispatch(window, message, wp, lp);
			}
		private:
			static detail::MessageDispatcher<CustomControl> messageDispatcher_;
			friend class detail::MessageDispatcher<CustomControl>;
		};
	}
}

#endif // !ASCENSION_CUSTOM_CONTROL_HPP
