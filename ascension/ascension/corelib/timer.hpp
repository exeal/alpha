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
#include <boost/core/noncopyable.hpp>
#include <boost/optional.hpp>
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
#	include <glibmm/main.h>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
#	include <QTimer>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
#	include <ascension/win32/windows.hpp>
#	include <unordered_map>
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
	class Timer : private boost::noncopyable
#if ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
		, protected QTimer
#endif
	{
	public:
		/// Default constructor.
		Timer() : object_(nullptr) {}
		/**
		 * Move constructor.
		 * @param other The source object
		 */
		Timer(Timer&& other) : object_(other.object_)
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
			, connection_(other.connection_)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			, identifier_(other.identifier_)
#endif
		{
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
			other.connection_ = sigc::connection();
#endif
			other.object_ = nullptr;
		}
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
		/**
		 * Move-assignment operator.
		 * @param other The source object
		 */
		Timer& operator=(Timer&& other) {
			object_ = other.object_;
			other.object_ = nullptr;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
			connection_ = other.connection_;
			other.connection_ = sigc::connection();
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			identifier_ = other.identifier_;
#endif
		}
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
			identifier_ = ::SetTimer(nullptr, reinterpret_cast<UINT_PTR>(this), static_cast<UINT>(interval.count()), &function);
			if(identifier_ == 0)
				throw makePlatformError();
			identifiersToTimers_.insert(std::make_pair(identifier_, this));
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
				identifiersToTimers_.erase(identifier_);
#else
				ASCENSION_CANT_DETECT_PLATFORM();
#endif
				object_ = nullptr;
			}
		}

	private:
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
		bool function() {
			return object_->timeElapsed(*this), true;
		}
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
		void timerEvent(QTimerEvent* event) override {
			object_->timeElapsed(*this);
		}
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
		static void CALLBACK function(HWND, UINT, UINT_PTR identifier, DWORD) {
			auto i(identifiersToTimers_.find(identifier));
			if(i != std::end(identifiersToTimers_)) {
				auto& self = *std::get<1>(*i);
				if(auto* const p = self.object_)
					p->timeElapsed(self);
			}
		}
#else
		ASCENSION_CANT_DETECT_PLATFORM();
#endif
	private:
		HasTimer<T>* object_;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
		sigc::connection connection_;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
		UINT_PTR identifier_;
		static std::unordered_map<UINT_PTR, Timer<T>*> identifiersToTimers_;
#endif
		boost::chrono::milliseconds interval_;
	};

	template<typename T>
	std::unordered_map<UINT_PTR, Timer<T>*> Timer<T>::identifiersToTimers_;
}

#endif // !ASCENSION_TIMER_HPP
