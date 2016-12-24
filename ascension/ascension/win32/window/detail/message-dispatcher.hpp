/**
 * @file message-dispatcher.hpp
 * Defines @c MessageDispatcher internal class.
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
			 * @internal
			 * @tparam Window The window class
			 */
			template<typename Window>
			class MessageDispatcher : private boost::noncopyable {
			public:
				LRESULT dispatch(HWND window, UINT message, WPARAM wp, LPARAM lp) {
					bool dummy;
					return dispatch(window, message, wp, lp, dummy);
				}
				template<typename DefaultProcedure>
				LRESULT dispatch(HWND window, UINT message, WPARAM wp, LPARAM lp, DefaultProcedure defaultProcedure) {
					bool consumed;
					const LRESULT result = dispatch(window, message, wp, lp, consumed);
					if(consumed)
						return result;
					auto i(handleToObjects_.find(window));
					assert(i != std::end(handleToObjects_));
					return ::CallWindowProcW(std::get<1>(*i)->*defaultProcedure, window, message, wp, lp);
				}
				LRESULT dispatch(HWND window, UINT message, WPARAM wp, LPARAM lp, bool& consumed) {
					if(message == WM_NCCREATE) {
						void* const p = reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams;
						assert(p != nullptr);
						handleToObjects_.insert(std::make_pair(window, static_cast<Window*>(p)));
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
