/**
 * @file subclassed-window.hpp
 * Defines @c win32#SubclassedWindow class.
 * @author exeal
 * @date 2011-03-26 Separated from window-windows.hpp.
 * @date 2012-03-07 Renamed from widget-windows.hpp.
 * @date 2016-12-24 Separated from window.hpp.
 */

#ifndef ASCENSION_SUBCLASSED_WINDOW_HPP
#define ASCENSION_SUBCLASSED_WINDOW_HPP
#include <ascension/graphics/geometry/dimension.hpp>
#include <ascension/graphics/geometry/point.hpp>
#include <ascension/win32/window/window.hpp>
#include <ascension/win32/window/detail/message-dispatcher.hpp>
#include <boost/optional.hpp>

namespace ascension {
	namespace win32 {
		class SubclassedWindow : public Window {
		public:
			/// Move-constructor.
			SubclassedWindow(SubclassedWindow&& other) BOOST_NOEXCEPT :
					Window(std::move(other)), originalWindowProcedure_(other.originalWindowProcedure_) {
				other.originalWindowProcedure_ = nullptr;
			}
			/// Move-assignment operator.
			SubclassedWindow& operator=(SubclassedWindow&& other) BOOST_NOEXCEPT {
				Window::operator=(std::move(other));
				originalWindowProcedure_ = other.originalWindowProcedure_;
				other.originalWindowProcedure_ = nullptr;
			}

		protected:
			/**
			 * Creates @c SubclassedWindow instance.
			 * @param parent The parent or owner window
			 * @param className The window class name
			 * @param position The initial position of the window
			 * @param size The initial size of the window in device units
			 * @param style The window style
			 * @param extendedStyle The extended window style
			 */
			SubclassedWindow(const Handle<HWND>& parent, const std::basic_string<WCHAR>& className,
					const boost::optional<graphics::Point>& position = boost::none, const boost::optional<graphics::Dimension>& size = boost::none,
					const boost::optional<DWORD>& style = boost::none, const boost::optional<DWORD>& extendedStyle = boost::none) : Window(::CreateWindowExW(
						boost::get_optional_value_or(extendedStyle, 0), className.c_str(), nullptr, boost::get_optional_value_or(style, 0),
						(position != boost::none) ? static_cast<int>(boost::geometry::get<0>(boost::get(position))) : CW_USEDEFAULT,
						(position != boost::none) ? static_cast<int>(boost::geometry::get<1>(boost::get(position))) : CW_USEDEFAULT,
						(size != boost::none) ? static_cast<int>(graphics::geometry::dx(boost::get(size))) : CW_USEDEFAULT,
						(size != boost::none) ? static_cast<int>(graphics::geometry::dy(boost::get(size))) : CW_USEDEFAULT,
						parent.get(), nullptr, ::GetModuleHandleW(nullptr), nullptr)) {
				if(handle().get() == nullptr)
					throw makePlatformError();
				originalWindowProcedure_ = reinterpret_cast<WNDPROC>(::GetWindowLongPtrW(handle().get(), GWLP_WNDPROC));
				::SetLastError(0);
				::SetWindowLongPtrW(handle().get(), GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
				if(::GetLastError() != 0)
					throw makePlatformError();
				::SetWindowLongPtrW(handle().get(), GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(windowProcedure));
				if(::GetLastError() != 0)
					throw makePlatformError();
			}
			/**
			 * Hooks and process window messages.
			 * @param message The message
			 * @param wp The additional message information of type @c WPARAM
			 * @param lp The additional message information of type @c LPARAM
			 * @param[out] consumed Set to @c true if the subclass consumed this message
			 * @return The result of message processing
			 */
			virtual LRESULT processMessage(UINT message, WPARAM wp, LPARAM lp, bool& consumed) {
				return 0;
			}

		private:
			static LRESULT CALLBACK windowProcedure(HWND window, UINT message, WPARAM wp, LPARAM lp) {
				return messageDispatcher_.dispatch(window, message, wp, lp, &SubclassedWindow::originalWindowProcedure_);
			}
		private:
			static detail::MessageDispatcher<SubclassedWindow> messageDispatcher_;
			WNDPROC originalWindowProcedure_;
			friend class detail::MessageDispatcher<SubclassedWindow>;
		};
	}
}

#endif // !ASCENSION_SUBCLASSED_WINDOW_HPP
