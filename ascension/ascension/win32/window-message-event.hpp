/**
 * @file win32/window-message-event.hpp
 * Defines @c WindowMessageEvent class.
 * @author exeal
 * @date 2017-04-28 Created.
 */

#ifndef ASCENSION_WIN32_WINDOW_MESSAGE_EVENT_HPP
#define ASCENSION_WIN32_WINDOW_MESSAGE_EVENT_HPP
#include <ascension/viewer/widgetapi/event/event.hpp>

namespace ascension {
	namespace win32 {
		/// Packs a message and additional parameters of window procedure.
		class WindowMessageEvent : public viewer::widgetapi::event::Event {
		public:
			/**
			 * Creates a @c WindowMessageEvent instance.
			 * @param message The message
			 * @param wp The @c WPARAM parameter
			 * @param lp The @c LPARAM parameter
			 */
			WindowMessageEvent(UINT message, WPARAM wp, LPARAM lp) BOOST_NOEXCEPT : message_(message), wp_(wp), lp_(lp) {
			}
			/// Returns the @c LPARAM parameter.
			LPARAM lp() const BOOST_NOEXCEPT {
				return lp_;
			}
			/**
			 * Returns the @c LPARAM parameter as @c T using @c reinterpret_cast.
			 * @tparam T The return type
			 */
			template<typename T> T lp() const BOOST_NOEXCEPT {
				return reinterpret_cast<T>(lp());
			}
			/// Returns the message.
			UINT message() const BOOST_NOEXCEPT {
				return message_;
			}
			/// Returns the @c WPARAM parameter.
			WPARAM wp() const BOOST_NOEXCEPT {
				return wp_;
			}
			/**
			 * Returns the @c WPARAM parameter as @c T using @c reinterpret_cast.
			 * @tparam T The return type
			 */
			template<typename T> T wp() const BOOST_NOEXCEPT {
				return reinterpret_cast<T>(wp());
			}

		private:
			UINT message_;
			WPARAM wp_;
			LPARAM lp_;
		};
	}
}

#endif // !ASCENSION_WIN32_WINDOW_MESSAGE_EVENT_HPP
