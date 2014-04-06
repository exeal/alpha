/**
 * @file corelib/timer.hpp
 * @author exeal
 * @date 2011-06-29 created
 */

#ifndef ASCENSION_TIMER_HPP
#define ASCENSION_TIMER_HPP
#include <ascension/platforms.hpp>
#include <ascension/corelib/basic-exceptions.hpp>
#include <boost/config.hpp>	// BOOST_NOEXCEPT
#include <boost/optional.hpp>
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
#	include <glibmm/main.h>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
#	include <QTimer>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
#	include <ascension/win32/windows.hpp>
#endif

namespace ascension {

	class Timer;

	/// @c HasTimer is an abstract base class of type which is a receiver called by @c Timer instance.
	class HasTimer {
	private:
		/**
		 * Called by @c Timer instance.
		 * @param timer The timer
		 */
		virtual void timeElapsed(Timer& timer) = 0;
		friend class Timer;
	};

	class Timer
#if ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
		: protected QTimer
#endif
	{
	public:
		/// Default constructor.
		Timer() : object_(nullptr) {}
		/**
		 * Creates a timer and starts.
		 * @param interval The interval in milliseconds
		 * @param object The object receives the timer event
		 * @throw PlatformDependentError&lt;&gt;
		 */
		Timer(unsigned int interval, HasTimer& object) : object_(nullptr) {
			start(interval, object);
		}
		/// Destructor.
		~Timer() {stop();}
		/// Returns the timeout interval in milliseconds or @c boost#none if not active.
		boost::optional<unsigned int> Timer::interval() const BOOST_NOEXCEPT {
			return isActive() ? boost::make_optional(interval_) : boost::none;
		}
		/// Returns @c true if this timer is running.
		bool isActive() const BOOST_NOEXCEPT {
			return object_ != nullptr;
		}
		/**
		 * Starts or restarts this timer.
		 * @param interval The interval in milliseconds
		 * @param object The object receives the timer event
		 * @throw PlatformDependentError&lt;&gt;
		 */
		void start(unsigned int interval, HasTimer& object) {
			stop();
			object_ = &object;
			interval_ = interval;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
			connection_ = Glib::signal_timeout().connect(sigc::mem_fun(*this, &Timer::function), interval);
#elif ASCENSION_SELECTS_WINDOW_SYSTEM_(QT)
			setSingleShot(false);
			start(interval);
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			identifier_ = ::SetTimer(nullptr, 0, interval, &function);
			if(identifier_ == 0)
				throw makePlatformError();
#else
			ASCENSION_CANT_DETECT_PLATFORM();
#endif
		}
		/// Stops this timer.
		void stop() {
			if(isActive()) {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				connection_.disconnect();
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				QTimer::stop();
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				if(!win32::boole(::KillTimer(nullptr, identifier_)))
					throw makePlatformError();
#else
				ASCENSION_CANT_DETECT_PLATFORM();
#endif
				object_ = nullptr;
			}
		}

	private:
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
		bool function() {
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
		void timerEvent(QTimerEvent* event) override {
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
		static void CALLBACK function(HWND, UINT, UINT_PTR, DWORD) {
#else
		ASCENSION_CANT_DETECT_PLATFORM();
#endif
			object_->timeElapsed(*this);
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
			return true;
#endif
		}
	private:
		HasTimer* object_;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
		sigc::connection connection_;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
		UINT_PTR identifier_;
#endif
		unsigned int interval_;
	};

}

#endif // !ASCENSION_TIMER_HPP
