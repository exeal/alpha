/**
 * @file corelib/timer.hpp
 * @author exeal
 * @date 2011-06-29 created
 */

#ifndef ASCENSION_TIMER_HPP
#define ASCENSION_TIMER_HPP
#include <ascension/platforms.hpp>
#include <ascension/corelib/basic-exceptions.hpp>
#include <boost/chrono/duration.hpp>
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
	template<typename T> class Timer;

	/**
	 * @c HasTimer is an abstract base class of type which is a receiver called by @c Timer instance.
	 * @tparam T The unique type to prevent the duplication in the derived class
	 */
	template<typename T = void>
	class HasTimer {
	private:
		/**
		 * Called by @c Timer instance.
		 * @param timer The timer
		 */
		virtual void timeElapsed(Timer<T>& timer) = 0;
		friend class Timer<T>;
	};

	/**
	 * @tparam T The type for uniqueness. See the description of @c HasTimer class template
	 */
	template<typename T = void>
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
		Timer(const boost::chrono::milliseconds& interval, HasTimer<T>& object) : object_(nullptr) {
			start(interval, object);
		}
		/// Destructor.
		~Timer() {stop();}
		/// Returns the timeout interval in milliseconds or @c boost#none if not active.
		boost::optional<boost::chrono::milliseconds> Timer::interval() const BOOST_NOEXCEPT {
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
		void start(const boost::chrono::milliseconds& interval, HasTimer<T>& object) {
			stop();
			object_ = &object;
			interval_ = interval;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
			connection_ = Glib::signal_timeout().connect(sigc::mem_fun(*this, &Timer::function), static_cast<unsigned int>(interval.count()));
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
			setSingleShot(false);
			start(interval.count());
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			identifier_ = ::SetTimer(nullptr, 0, static_cast<UINT>(interval.count()), &function);
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
		HasTimer<T>* object_;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
		sigc::connection connection_;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
		UINT_PTR identifier_;
#endif
		boost::chrono::milliseconds interval_;
	};

}

#endif // !ASCENSION_TIMER_HPP
