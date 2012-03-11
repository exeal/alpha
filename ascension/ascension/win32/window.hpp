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
#include <ascension/win32/windows.hpp>
#include <memory>	// std.shared_ptr

namespace ascension {
	namespace win32 {
		class WindowBase {
		public:
			static const DWORD defaultStyle = WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE;
			std::shared_ptr<std::remove_pointer<HWND>::type> handle() const {
				return handle_;
			}
		protected:
			explicit WindowBase(HWND handle) : handle_(handle) {
				if(handle == nullptr)
					throw NullPointerException("handle");
			}
		private:
			std::shared_ptr<std::remove_pointer<HWND>::type> handle_;
		};

		class Window : public WindowBase {
		protected:
			Window(std::shared_ptr<std::remove_pointer<HWND>::type> parent, const WCHAR className[],
					const graphics::NativePoint& position = graphics::geometry::make<graphics::NativePoint>(CW_USEDEFAULT, CW_USEDEFAULT),
					const graphics::NativeSize& size = graphics::geometry::make<graphics::NativeSize>(CW_USEDEFAULT, CW_USEDEFAULT),
					DWORD style = 0, DWORD extendedStyle = 0) : WindowBase(::CreateWindowExW(
						extendedStyle, className, nullptr, style,
						graphics::geometry::x(position), graphics::geometry::y(position),
						graphics::geometry::dx(size), graphics::geometry::dy(size),
						parent.get(), nullptr, ::GetModuleHandleW(nullptr), nullptr)) {
				originalWindowProcedure_ = reinterpret_cast<WNDPROC>(::GetWindowLongPtrW(handle_.get(), GWLP_WNDPROC));
				::SetWindowLongPtrW(handle_.get(), GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
				::SetWindowLongPtrW(handle_.get(), GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(windowProcedure));
			}
			virtual LRESULT processMessage(UINT message, WPARAM wp, LPARAM lp, bool& consumed) {
				return 0;
			}
		private:
			static LRESULT CALLBACK windowProcedure(HWND window, UINT message, WPARAM wp, LPARAM lp) {
				if(Window* const p = reinterpret_cast<Window*>(::GetWindowLongPtrW(window, GWLP_USERDATA))) {
					bool consumed = false;
					const LRESULT result = p->processMessage(message, wp, lp, consumed);
					return consumed ? result : ::CallWindowProcW(p->originalWindowProcedure_, window, message, wp, lp);
				}
				return ::DefWindowProc(window, message, wp, lp);
			}
		private:
			std::shared_ptr<std::remove_pointer<HWND>::type> handle_;
			WNDPROC originalWindowProcedure_;
		};

		class CustomControl : public WindowBase {
		};
	}
}

#endif // !ASCENSION_WIN32_WINDOW_HPP
