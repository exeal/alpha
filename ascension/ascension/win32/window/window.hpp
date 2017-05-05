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
		namespace detail {
			template<typename T> class MessageDispatcher;
		}

		/// Holds a handle to the window.
		class Window : public UniqueWrapper<Window>, private boost::noncopyable {
		public:
			/// Describes window type.
			class Type {
			public:
				/// An independent popup-window.
				static Type popup() BOOST_NOEXCEPT {
					return Type(WS_POPUPWINDOW, Handle<HWND>());
				}
				/// A pop-up window with the given @a parent.
				static Type popup(Handle<HWND> parent) {
					return Type(WS_POPUPWINDOW, parent);
				}
				/// A toplevel window.
				static Type toplevel() {
					return Type(WS_OVERLAPPEDWINDOW, Handle<HWND>());
				}
				/// A widget with the given @a parent.
				static Type widget(Handle<HWND> parent) {
					if(parent.get() == nullptr)
						throw NullPointerException("parent");
					return Type(WS_CHILD | WS_VISIBLE, parent);
				}

				Handle<HWND> parent() const BOOST_NOEXCEPT {
					return parent_;
				}
				DWORD styles() const BOOST_NOEXCEPT {
					return styles_;
				}

			private:
				Type(DWORD styles, Handle<HWND> parent) BOOST_NOEXCEPT : styles_(styles), parent_(parent) {}
				DWORD styles_;
				Handle<HWND> parent_;
			};

			/// Constructor takes a borrowed window handle.
			explicit Window(Handle<HWND> handle) BOOST_NOEXCEPT : handle_(handle) {}
			/**
			 * Creates @c Window instance.
			 * @param className The window class name
			 * @param type The window type
			 */
			Window(const std::basic_string<WCHAR>& className, const Type& type) : handle_(::CreateWindowExW(
					0, className.c_str(), nullptr, type.styles(),
					CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
					type.parent().get(), nullptr, ::GetModuleHandleW(nullptr), nullptr), &::DestroyWindow) {
				if(handle().get() == nullptr)
					throw makePlatformError();
			}
			/// Move-constructor.
			Window(Window&& other) BOOST_NOEXCEPT : handle_(std::move(other.handle_)) {}
			/// Destructor.
			virtual ~Window() BOOST_NOEXCEPT {}
			/// Move-assignment operator.
			Window& operator=(Window&& other) BOOST_NOEXCEPT {
				handle_ = std::move(other.handle_);
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
			template<typename T> friend class detail::MessageDispatcher;
			Handle<HWND> handle_;
		};
	}
}

#endif // !ASCENSION_WIN32_WINDOW_HPP
