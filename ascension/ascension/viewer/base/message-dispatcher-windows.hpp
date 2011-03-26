/**
 * @file message-dispatcher-windows.hpp
 * @author exeal
 * @date 2011-03-26 separated from window-windows.hpp
 * @date 2011-03-27 separated from widget-windows.hpp
 */

#ifndef ASCENSION_MESSAGE_DISPATCHER_WINDOWS_HPP
#define ASCENSION_MESSAGE_DISPATCHER_WINDOWS_HPP
#include <ascension/corelib/type-traits.hpp>	// ASCENSION_DEFINED_HAS_METHOD
#include <ascension/win32/windows.hpp>
#include <type_traits>	// std.enable_if


#define ASCENSION_DISPATCH_MESSAGE(messageID)	\
	case messageID:								\
		return dispatch(static_cast<Derived&>(widget), MessageTag<messageID>(), wp, lp, consumed)

#define ASCENSION_IMPLEMENT_MESSAGE_DISPATCH(messageID, methodName)	\
	template<typename T>											\
	static LRESULT dispatch(T& widget, MessageTag<messageID>,		\
		WPARAM wp, LPARAM lp, bool& consumed, typename std::enable_if<detail::Has_##methodName<T>::value>::type* = 0)

namespace ascension {

	namespace detail {
		ASCENSION_DEFINE_HAS_METHOD(aboutToLoseFocus, bool(T::*)());
		ASCENSION_DEFINE_HAS_METHOD(focusGained, bool(T::*)());
	}

	namespace win32 {

		template<UINT message> struct MessageTag {
			static const UINT value = message;
		};

		class WidgetBase;

		template<typename Derived>
		class MessageDispatcher {
		public:
			static LRESULT processMessage(WidgetBase& widget, UINT message, WPARAM wp, LPARAM lp, bool& consumed) {
				switch(message.message) {
					ASCENSION_DISPATCH_MESSAGE(WM_SETFOCUS);
					ASCENSION_DISPATCH_MESSAGE(WM_KILLFOCUS);
				}
			}
		private:
			
			ASCENSION_IMPLEMENT_MESSAGE_DISPATCH(WM_KILLFOCUS, aboutToLoseFocus) {
				return widget.aboutToLoseFocus() ? 0 : 1;
			}
			ASCENSION_IMPLEMENT_MESSAGE_DISPATCH(WM_SETFOCUS, focusGained) {
				return widget.focusGained() ? 0 : 1;
			}
			template<typename T> static LRESULT dispatch(T& widget, ...) {return 0;}
			
		};

	}
}

#undef ASCENSION_DISPATCH_MESSAGE
#undef ASCENSION_IMPLEMENT_MESSAGE_DISPATCH

#endif // !ASCENSION_MESSAGE_DISPATCHER_WINDOWS_HPP
