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

namespace ascension {
	namespace win32 {
		/// Holds a handle to the window.
		class Window : private boost::noncopyable {
		public:
			static const DWORD DEFAULT_STYLE = WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE;
		public:
			/// Constructor takes a borrowed window handle.
			explicit Window(const Handle<HWND>::Type& handle) BOOST_NOEXCEPT : handle_(handle.get()) {}
			/// Move-constructor.
			Window(Window&& other) BOOST_NOEXCEPT : handle_(std::move(other.handle_)) {}
			/// Move-assignment operator.
			Window& operator=(Window&& other) BOOST_NOEXCEPT {std::swap(*this, Window(other));}
			/// Returns the held window handle.
			Handle<HWND>::Type handle() const {return handle_;}

		protected:
			/// Constructor takes a window handle.
			explicit Window(HWND&& handle) : handle_(handle, &::DestroyWindow) {
				if(handle_.get() == nullptr)
					throw NullPointerException("handle");
			}

		private:
			Handle<HWND>::Type handle_;
		};
	}
}

#endif // !ASCENSION_WIN32_WINDOW_HPP
