/**
 * @file win32/status-bar.hpp
 * Defines win32#StatusBar class.
 * @author exeal
 * @date 2017-02-13 Created.
 */

#ifndef ALPHA_WIN32_STATUS_BAR_HPP
#define ALPHA_WIN32_STATUS_BAR_HPP
#include "platform-string.hpp"
#include <ascension/corelib/signals.hpp>
#include <ascension/corelib/timer.hpp>
#include <ascension/win32/window/window.hpp>
#include <list>
#include <tuple>
#include <CommCtrl.h>

namespace alpha {
	namespace win32 {
		class StatusBar : public ascension::win32::Window, private ascension::HasTimer<StatusBar> {
		public:
			typedef unsigned int Context;
			static const Context DEFAULT_CONTEXT = 0;
			typedef std::size_t MessageID;

			StatusBar();
			virtual ~StatusBar() BOOST_NOEXCEPT;

			/// @name Messages
			/// @{
			void pop(Context context = DEFAULT_CONTEXT);
			MessageID push(const PlatformString& message, Context context = DEFAULT_CONTEXT);
			MessageID push(const PlatformString& message, const boost::chrono::milliseconds& timeout, Context context = DEFAULT_CONTEXT);
			void remove(MessageID messageID, Context context = DEFAULT_CONTEXT);
			void removeAll(Context context = DEFAULT_CONTEXT);
			/// @}

			/// @name Simple Modes
			/// @{
			bool isSimple() const BOOST_NOEXCEPT;
			void setSimple(bool simple);
			/// @}

			/// @name Size Grip
			/// @{
			void enableSizeGrip(bool enable);
			bool isSizeGripEnabled() const;
			/// @}

			/// @name Signals
			/// @{
			typedef boost::signals2::signal<void(Context, const PlatformString&)> PoppedSignal;
			ascension::SignalConnector<PoppedSignal> poppedSignal() BOOST_NOEXCEPT;
			typedef boost::signals2::signal<void(Context, const PlatformString&)> PushedSignal;
			ascension::SignalConnector<PushedSignal> pushedSignal() BOOST_NOEXCEPT;
			/// @}

		protected:
			virtual void popped(Context context, const PlatformString& message);
			virtual void pushed(Context context, const PlatformString& message);

		private:
			MessageID newMessageID() const BOOST_NOEXCEPT;
			MessageID push(const PlatformString& message, const boost::chrono::milliseconds* timeout, Context context);
			template<typename Function> void removeIf(Function function);
			void timeElapsed(ascension::Timer<StatusBar>& timer) override;
			void timeout();
			void update();
		private:
			typedef std::tuple<MessageID, Context, PlatformString> Message;
			std::list<Message> messages_;
			ascension::Timer<StatusBar> timer_;
			std::tuple<MessageID, Context> timedMessage_;
			PoppedSignal poppedSignal_;
			PushedSignal pushedSignal_;
		};
	}
}

#endif // !ALPHA_WIN32_STATUS_BAR_HPP
