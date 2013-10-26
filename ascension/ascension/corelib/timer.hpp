/**
 * @file corelib/timer.hpp
 * @author exeal
 * @date 2011-06-29 created
 */

#ifndef ASCENSION_TIMER_HPP
#define ASCENSION_TIMER_HPP
#include <ascension/platforms.hpp>
#include <ascension/corelib/basic-exceptions.hpp>
#if defined(ASCENSION_WINDOW_SYSTEM_GTK)
#	include <glibmm/main.h>
#elif defined(ASCENSION_WINDOW_SYSTEM_WIN32)
#	include <ascension/win32/windows.hpp>
#endif

namespace ascension {

	class Timer;

	class HasTimer {
	private:
		virtual void timeElapsed(Timer& timer) = 0;
		friend class Timer;
	};

	class Timer {
	public:
		typedef int Identifier;
	public:
		/// Default constructor.
		Timer() : object_(nullptr) {}
		/***/
		Timer(unsigned int milliseconds, HasTimer& object) : object_(nullptr) {
			start(milliseconds, object);
		}
		/// Destructor.
		~Timer() {stop();}
		/**
		 * Returns the identifier of this timer.
		 * @retval The identifier
		 * @throw IllegalStateException This timer is not active
		 */
		Identifier identifier() const {
			if(!isActive())
				throw IllegalStateException("The timer is not active.");
			return identifier_;
		}
		/// Returns @c true if this timer is running.
		bool isActive() const BOOST_NOEXCEPT {return object_ != nullptr;}
		/**
		 * Starts or restarts this timer.
		 * @param milliseconds The interval in milliseconds
		 * @param object The object receives the timer event
		 * @throw PlatformDependentError&lt;&gt;
		 */
		void start(unsigned int milliseconds, HasTimer& object) {
			stop();
			object_ = &object;
#if defined(ASCENSION_WINDOW_SYSTEM_GTK)
			Glib::signal_timeout().connect_once(sigc::mem_fun(*this, &Timer::function), milliseconds);
#elif defined(ASCENSION_WINDOW_SYSTEM_WIN32)
			identifier_ = ::SetTimer(nullptr, 0, milliseconds, &function);
			if(identifier_ == 0)
				throw makePlatformError();
#else
			ASCENSION_CANT_DETECT_PLATFORM();
#endif
		}
		void stop();
	private:
#if defined(ASCENSION_WINDOW_SYSTEM_GTK)
		void function() {
			object_->timeElapsed(*this);
		}
#elif defined(ASCENSION_WINDOW_SYSTEM_WIN32)
		static void CALLBACK function(HWND, UINT, UINT_PTR identifier, DWORD);
#else
			ASCENSION_CANT_DETECT_PLATFORM();
#endif
	private:
		HasTimer* object_;
		Identifier identifier_;
	};

}

#endif // !ASCENSION_TIMER_HPP
