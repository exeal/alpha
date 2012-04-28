/**
 * @file window.hpp
 * @author exeal
 * @date 2011-03-26 separated from window-windows.hpp
 * @date 2012-03-07 renamed from widget-windows.hpp
 */

#ifndef ASCENSION_WIN32_WINDOW_HPP
#define ASCENSION_WIN32_WINDOW_HPP
#include <ascension/corelib/basic-exceptions.hpp>
#include <ascension/graphics/geometry.hpp>
#include <ascension/win32/handle.hpp>
#include <memory>	// std.shared_ptr

namespace ascension {
	namespace win32 {
		class Window {
		public:
			static const DWORD defaultStyle = WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE;
			const Handle<HWND>& handle() const {
				return handle_;
			}
		protected:
			explicit Window(HWND&& handle) : handle_(handle) {
				if(handle_.get() == nullptr)
					throw NullPointerException("handle");
			}
		private:
			Handle<HWND> handle_;
		};

		class SubclassedWindow : public Window {
		protected:
			SubclassedWindow(const Handle<HWND>& parent, const WCHAR className[],
					const graphics::NativePoint* position = nullptr, const graphics::NativeSize* size = nullptr,
					DWORD style = 0, DWORD extendedStyle = 0) : Window(::CreateWindowExW(
						extendedStyle, className, nullptr, style,
						(position != nullptr) ? graphics::geometry::x(*position) : CW_USEDEFAULT,
						(position != nullptr) ? graphics::geometry::y(*position) : CW_USEDEFAULT,
						(size != nullptr) ? graphics::geometry::dx(*size) : CW_USEDEFAULT,
						(size != nullptr) ? graphics::geometry::dy(*size) : CW_USEDEFAULT,
						parent.get(), nullptr, ::GetModuleHandleW(nullptr), nullptr)) {
				originalWindowProcedure_ = reinterpret_cast<WNDPROC>(::GetWindowLongPtrW(handle().get(), GWLP_WNDPROC));
				::SetWindowLongPtrW(handle().get(), GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
				::SetWindowLongPtrW(handle().get(), GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(windowProcedure));
			}
			virtual LRESULT processMessage(UINT message, WPARAM wp, LPARAM lp, bool& consumed) {
				return 0;
			}
		private:
			static LRESULT CALLBACK windowProcedure(HWND window, UINT message, WPARAM wp, LPARAM lp) {
				if(SubclassedWindow* const p = reinterpret_cast<SubclassedWindow*>(::GetWindowLongPtrW(window, GWLP_USERDATA))) {
					bool consumed = false;
					const LRESULT result = p->processMessage(message, wp, lp, consumed);
					return consumed ? result : ::CallWindowProcW(p->originalWindowProcedure_, window, message, wp, lp);
				}
				return ::DefWindowProcW(window, message, wp, lp);
			}
		private:
			WNDPROC originalWindowProcedure_;
		};

		class CustomControl : public Window {
		public:
			struct ClassInformation {
				UINT style;	// corresponds to WNDCLASSEXW.style
				/// Makes a brush handle parameter from either a brush handle or @c COLORREF value. 
				class Background {
				public:
					/// Constructor makes @c null @c HBRUSH value.
					Background() /*throw()*/ : brush_(nullptr) {}
					/// Constructor takes a brush handle.
					Background(Handle<HBRUSH>&& handle) /*throw()*/ : brush_(std::move(handle)) {}
					/// Constructor takes a @c COLORREF value used to make the brush handle.
					Background(int systemColor) /*throw()*/
						: brush_(reinterpret_cast<HBRUSH>(static_cast<HANDLE_PTR>(systemColor + 1))) {}
					/// Returns the brush handle.
					HBRUSH get() const /*throw()*/ {return brush_.get();}
				private:
					Handle<HBRUSH> brush_;
				} background;
				Handle<HICON> icon, smallIcon;
				/// Makes a cursor handle parameter from either a cursor handle or numeric identifier.
				class CursorHandleOrID {
				public:
					/// Constructor makes @c null @c HCURSOR value.
					CursorHandleOrID() /*throw()*/ : cursor_(nullptr) {}
					/// Constructor takes a cursor handle.
					CursorHandleOrID(Handle<HCURSOR>&& handle) /*throw()*/ : cursor_(std::move(handle)) {}
					/// Constructor takes a numeric identifier for system cursor.
					CursorHandleOrID(const WCHAR* systemCursorID) : cursor_(::LoadCursorW(nullptr, systemCursorID), detail::NullDeleter()) {}
					/// Returns the cursor handle.
					HCURSOR get() const /*throw()*/ {return cursor_.get();}
				private:
					Handle<HCURSOR> cursor_;
				} cursor;
				ClassInformation() : style(0) {}
			};
			virtual void provideClassInformation(ClassInformation& classInfomation) const {}
			virtual std::basic_string<WCHAR> provideClassName() const = 0;
		};
	}
}

#endif // !ASCENSION_WIN32_WINDOW_HPP
