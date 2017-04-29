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
#include <ascension/win32/window/window.hpp>
#include <ascension/win32/window/detail/message-dispatcher.hpp>
#include <functional>

namespace ascension {
	namespace win32 {
		/**
		 * @tparam Derived The derived type
		 */
		template<typename Derived>
		class SubclassedWindow : public Window {
		public:
			/// Move-constructor.
			SubclassedWindow(SubclassedWindow&& other) BOOST_NOEXCEPT :
					Window(std::move(other)), originalWindowProcedure_(other.originalWindowProcedure_) {
				other.originalWindowProcedure_ = nullptr;
			}
			/// Destructor.
			virtual ~SubclassedWindow() BOOST_NOEXCEPT {}
			/// Move-assignment operator.
			SubclassedWindow& operator=(SubclassedWindow&& other) BOOST_NOEXCEPT {
				Window::operator=(std::move(other));
				originalWindowProcedure_ = other.originalWindowProcedure_;
				other.originalWindowProcedure_ = nullptr;
			}

		protected:
			/**
			 * Creates @c SubclassedWindow instance.
			 * @param className The window class name
			 * @param type The window type
			 */
			SubclassedWindow(const std::basic_string<WCHAR>& className, const Type& type) : Window(className, type) {
				originalWindowProcedure_ = reinterpret_cast<WNDPROC>(getWindowLong(handle().get(), GWLP_WNDPROC));
				setWindowLong(handle().get(), GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
				setWindowLong(handle().get(), GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(SubclassedWindow<Derived>::windowProcedure));
			}
			/**
			 * Hooks and process window messages.
			 * @param event The window message event
			 * @return The result of message processing
			 */
			virtual LRESULT processMessage(WindowMessageEvent& event) {
				return 0;
			}

		private:
			static LRESULT CALLBACK windowProcedure(HWND window, UINT message, WPARAM wp, LPARAM lp) {
				return messageDispatcher_.dispatch(window, message, wp, lp, &SubclassedWindow<Derived>::originalWindowProcedure_);
			}
		private:
			static detail::MessageDispatcher<SubclassedWindow<Derived>> messageDispatcher_;
			WNDPROC originalWindowProcedure_;
			friend class detail::MessageDispatcher<SubclassedWindow<Derived>>;
		};

		template<typename Derived> detail::MessageDispatcher<SubclassedWindow<Derived>> SubclassedWindow<Derived>::messageDispatcher_;
	}
}

#endif // !ASCENSION_SUBCLASSED_WINDOW_HPP
