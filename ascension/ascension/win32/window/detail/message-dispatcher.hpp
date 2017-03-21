/**
 * @file message-dispatcher.hpp
 * Defines @c MessageDispatcher internal class template.
 * @author exeal
 * @date 2011-03-26 Separated from window-windows.hpp.
 * @date 2012-03-07 Renamed from widget-windows.hpp.
 * @date 2016-12-24 Separated from window.hpp.
 */

#ifndef ASCENSION_MESSAGE_DISPATCHER_HPP
#define ASCENSION_MESSAGE_DISPATCHER_HPP
#include <ascension/win32/windows.hpp>
#include <map>

namespace ascension {
	namespace win32 {
		namespace detail {
			/**
			 * @internal Dispatches window messages to window.
			 * @tparam Window The window class
			 */
			template<typename Window>
			class MessageDispatcher {
			public:
				/**
				 * Dispatches  the window message to the window.
				 * @param window A handle to the window
				 * @param message The message to dispatch
				 * @param wp The first parameter
				 * @param lp The second parameter
				 * @return The returned value from the window
				 */
				LRESULT dispatch(HWND window, UINT message, WPARAM wp, LPARAM lp) {
					bool dummy;
					return dispatch(window, message, wp, lp, dummy);
				}

				/**
				 * Dispatches If and only if the window didn't consume the message,
				 * @a defaultProcedure is called.
				 * @tparam DefaultProcesure The type of @a defaultProcedure
				 * @param window A handle to the window
				 * @param message The message to dispatch
				 * @param wp The first parameter
				 * @param lp The second parameter
				 * @param defaultProcedure The default window procedure. The first parameter is a @c Window
				 * @return The returned value from the window
				 */
				template<typename DefaultProcedure>
				LRESULT dispatch(HWND window, UINT message, WPARAM wp, LPARAM lp, DefaultProcedure defaultProcedure) {
					assert(defaultProcedure != nullptr);
					bool consumed;
					const LRESULT result = dispatch(window, message, wp, lp, consumed);
					if(consumed)
						return result;
					auto i(handleToObjects_.find(window));
					assert(i != std::end(handleToObjects_));
					return ::CallWindowProcW(std::get<1>(*i)->*defaultProcedure, window, message, wp, lp);
				}

			private:
				LRESULT dispatch(HWND window, UINT message, WPARAM wp, LPARAM lp, bool& consumed) {
					if(message == WM_NCCREATE) {
						Window* const p = static_cast<Window*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);
						assert(p != nullptr);
						handleToObjects_.insert(std::make_pair(window, p));
						p->handle_.reset(window);
					}
					const auto i(handleToObjects_.find(window));
					const auto result = (i != std::end(handleToObjects_)) ?
						std::get<1>(*i)->processMessage(message, wp, lp, consumed) : (::DefWindowProcW(window, message, wp, lp), consumed = true);
					if(message == WM_NCDESTROY)
						handleToObjects_.erase(window);
					return result;
				}

			private:
				std::map<HWND, Window*> handleToObjects_;
			};
		}
	}
}

#endif // !ASCENSION_MESSAGE_DISPATCHER_HPP
