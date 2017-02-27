/**
 * @file window.hpp
 * Defines @c win32#Window class.
 * @author exeal
 * @date 2011-03-26 separated from window-windows.hpp
 * @date 2012-03-07 renamed from widget-windows.hpp
 */

#ifndef ASCENSION_WIN32_WINDOW_HPP
#define ASCENSION_WIN32_WINDOW_HPP
#include <ascension/corelib/basic-exceptions.hpp>
#include <ascension/win32/handle.hpp>
#include <boost/noncopyable.hpp>
#include <string>

namespace ascension {
	namespace win32 {
		/// Holds a handle to the window.
		class Window : private boost::noncopyable {
		public:
			/// The window types.
			enum Type {
				WIDGET,		///< A widget.
				TOPLEVEL,	///< A toplevel window.
				POPUP		///< A pop-up window.
			};
			/// Constructor takes a borrowed window handle.
			explicit Window(const Handle<HWND>& handle) BOOST_NOEXCEPT : handle_(handle) {}
			/**
			 * Creates @c Window instance.
			 * @param className The window class name
			 * @param styles The window style(s)
			 */
			Window(const std::basic_string<WCHAR>& className, Type type) : handle_(::CreateWindowExW(
					0, className.c_str(), nullptr, typeToStyles(type),
					CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
					nullptr, nullptr, ::GetModuleHandleW(nullptr), nullptr), &::DestroyWindow) {
				if(handle().get() == nullptr)
					throw makePlatformError();
			}
			/// Move-constructor.
			Window(Window&& other) BOOST_NOEXCEPT : handle_(std::move(other.handle_)) {}
			/// Move-assignment operator.
			Window& operator=(Window&& other) BOOST_NOEXCEPT {
				std::swap(*this, Window(std::move(other)));
				return *this;
			}
			/// Returns the held window handle.
			Handle<HWND> handle() const {return handle_;}

		protected:
			/**
			 * Creates a @c Window instance with the owned handle.
			 * @param handle A handle to the window
			 * @throw NullPointerException @a handle is @c null
			 */
			explicit Window(HWND&& handle) : handle_(handle, &::DestroyWindow) {
				if(handle_.get() == nullptr)
					throw NullPointerException("handle");
			}

		private:
			static DWORD typeToStyles(Type type) {
				switch(type) {
					case WIDGET:
						return WS_CHILD | WS_VISIBLE;
					case TOPLEVEL:
						return WS_OVERLAPPEDWINDOW;
					case POPUP:
						return WS_POPUPWINDOW;
				}
				ASCENSION_ASSERT_NOT_REACHED();
			}
			Handle<HWND> handle_;
		};
	}
}

#endif // !ASCENSION_WIN32_WINDOW_HPP
