/**
 * @file win32/status-bar.cpp
 * Implements @c win32#StatusBar class.
 * @author exeal
 * @date 2017-02-13 Created.
 */

#include "win32/status-bar.hpp"
#include <boost/core/ignore_unused.hpp>
#include <boost/foreach.hpp>
#include <boost/range/algorithm_ext/erase.hpp>
#ifndef BOOST_NO_CXX11_HDR_MUTEX
#	include <mutex>
#else
#	include <boost/thread/mutex.hpp>
#endif

namespace alpha {
	namespace win32 {
		/**
		 * @typedef alpha::win32::StatusBar::PushedSignal
		 * The signal which gets emitted when a new message was pushed onto the message stack.
		 * @param context The context identifier
		 * @param message The message identifier
		 * @see #push, #pushed
		 */

		/**
		 * @typedef alpha::win32::StatusBar::PoppedSignal
		 * The signal which gets emitted when the first message in the stack was popped.
		 * @param context The context identifier
		 * @param message The message identifier
		 * @see #pop, #popped
		 */

		namespace {
			const WCHAR* className() {
				static bool loaded = false;
				if(!loaded) {
					auto icc(ascension::win32::makeZeroSize<INITCOMMONCONTROLSEX, DWORD>());
					icc.dwICC = ICC_BAR_CLASSES;
					if(!ascension::win32::boole(::InitCommonControlsEx(&icc)))
						throw ascension::makePlatformError();
					loaded = true;
				}
				return STATUSCLASSNAMEW;
			}
		}

		/**
		 * Creates a @c StatusBar widget.
		 * @param type The window type
		 */
		StatusBar::StatusBar(const Type& type) : ascension::win32::Window(className(), type) {
			const auto styles = ascension::win32::getWindowLong(handle().get(), GWL_STYLE);
			ascension::win32::setWindowLong(handle().get(), GWL_STYLE, styles | WS_VISIBLE | CCS_BOTTOM | SBARS_SIZEGRIP);
		}

		/// Destructor.
		StatusBar::~StatusBar() BOOST_NOEXCEPT {
		}

		/**
		 * Sets @c SBARS_SIZEGRIP style value.
		 * @param enable Set @c true to enable
		 * @see #isSizeGripEnabled
		 */
		void StatusBar::enableSizeGrip(bool enable) {
			auto styles = ascension::win32::getWindowLong(handle().get(), GWL_STYLE);
			if(enable)
				styles |= SBARS_SIZEGRIP;
			else
				styles &= ~SBARS_SIZEGRIP;
			ascension::win32::setWindowLong(handle().get(), GWL_STYLE, styles);
		}

		/**
		 * Returns @c true if simple mode.
		 * @see #setSimple
		 */
		bool StatusBar::isSimple() const BOOST_NOEXCEPT {
			return ::SendMessageW(handle().get(), SB_ISSIMPLE, 0, 0) != 0;
		}

		/**
		 * Returns if this @c StatusBar has @c SBARS_SIZEGRIP style.
		 * @see #enableSizeGrip
		 */
		bool StatusBar::isSizeGripEnabled() const {
			return (ascension::win32::getWindowLong(handle().get(), GWL_STYLE) & SBARS_SIZEGRIP) != 0;
		}

		/// @internal
		inline StatusBar::MessageID StatusBar::newMessageID() const BOOST_NOEXCEPT {
#ifndef BOOST_NO_CXX11_HDR_MUTEX
			static std::mutex mutex;
#else
			static boost::mutex mutex;
#endif
			static MessageID next = 0;
			mutex.lock();
			const auto v = ++next;
			mutex.unlock();
			return v;
		}

		/**
		 * Removes the first message in the stack with the given context identifier.
		 * @param context The context identifier
		 * @see #popped, #poppedSignal, #push
		 */
		void StatusBar::pop(Context context /* = DEFAULT_CONTEXT */) {
			for(auto i(std::begin(messages_)), e(std::end(messages_)); i != e; ++i) {
				if(std::get<1>(*i) == context) {
					const bool front = i == std::begin(messages_);
					const PlatformString message(std::get<2>(*i));
					messages_.erase(i);
					popped(context, message);
					poppedSignal_(context, message);
					if(front)
						update();
					break;
				}
			}
		}

		/**
		 * Called after a message was popped.
		 * @param context The context identifier
		 * @param message The message string
		 * @note Default implementation does nothing.
		 * @see #pop, #poppedSignal
		 */
		void StatusBar::popped(Context context, const PlatformString& message) {
			boost::ignore_unused(context, message);
		}

		/// Returns @c PoppedSignal signal connector.
		ascension::SignalConnector<StatusBar::PoppedSignal> StatusBar::poppedSignal() BOOST_NOEXCEPT {
			return ascension::makeSignalConnector(poppedSignal_);
		}

		/**
		 * Pushes a new message onto the message stack.
		 * @param message The text message to push
		 * @param context The context identifier of the message
		 * @return The message identifier
		 * @see #pop, #pushed, #pushedSignal
		 */
		StatusBar::MessageID StatusBar::push(const PlatformString& message, Context context /* = DEFAULT_CONTEXT */) {
			return push(message, nullptr, context);
		}

		/**
		 * Pushes a new message onto the message stack. The pushed message is automatically removed after the specified duration.
		 * @param message The text message to push
		 * @param timeout The timeout duration for automatic pop
		 * @param context The context identifier of the message
		 * @return The message identifier
		 * @see #pop, #pushed, #pushedSignal
		 */
		StatusBar::MessageID StatusBar::push(const PlatformString& message, const boost::chrono::milliseconds& timeout, Context context /* = DEFAULT_CONTEXT */) {
			return push(message, &timeout, context);
		}

		/**
		 * @internal Implements public @c #push methods.
		 * @param message The text message to push
		 * @param timeout The timeout duration for automatic pop
		 * @param context The context identifier of the message
		 * @return The message identifier
		 */
		StatusBar::MessageID StatusBar::push(const PlatformString& message, const boost::chrono::milliseconds* timeout, Context context) {
			const auto id(newMessageID());
			messages_.push_front(std::make_tuple(id, context, message));
			pushed(context, message);
			pushedSignal_(context, message);
			update();
			if(timeout != nullptr) {
				this->timeout();
				timedMessage_ = std::make_tuple(id, context);
				timer_.start(*timeout, *this);
			}
			return id;
		}

		/**
		 * Called after a message was pushed.
		 * @param context The context identifier
		 * @param message The message string
		 * @note Default implementation does nothing.
		 * @see #push, #pushedSignal
		 */
		void StatusBar::pushed(Context context, const PlatformString& message) {
			boost::ignore_unused(context, message);
		}

		/// Returns @c PushedSignal signal connector.
		ascension::SignalConnector<StatusBar::PushedSignal> StatusBar::pushedSignal() BOOST_NOEXCEPT {
			return ascension::makeSignalConnector(pushedSignal_);
		}

		/**
		 * Forces the removal of the message from the stack.
		 * @param messageID The message identifier, as returned by @c #push()
		 * @param context The context identifier
		 * @note This method neither calls @c #popped nor invokes @c PoppedSignal signal.
		 */
		void StatusBar::remove(MessageID messageID, Context context) {
			removeIf([messageID, context](const Message& message) {
				return std::get<0>(message) == messageID && std::get<1>(message) == context;
			});
		}

		/**
		 * Forces the removal of all messages from the stack with the given context identifier.
		 * @param context The context identifier
		 * @note This method neither calls @c #popped nor invokes @c PoppedSignal signal.
		 */
		void StatusBar::removeAll(Context context) {
			removeIf([context](const Message& message) {
				return std::get<1>(message) == context;
			});
		}

		/**
		 * @internal
		 * @tparam Function The type of @a function
		 * @param function
		 */
		template<typename Function>
		inline void StatusBar::removeIf(Function function) {
			if(!messages_.empty()) {
				const auto& front = messages_.front();
				boost::remove_erase_if(messages_, function);
				if(messages_.empty() || &front != &messages_.front())
					update();
			}
		}

		/**
		 * Sets the simple mode.
		 * @param simple The display type flag
		 * @see #isSimple
		 */
		void StatusBar::setSimple(bool simple) {
			::SendMessageW(handle().get(), SB_SIMPLE, simple ? TRUE : FALSE, 0);
		}

		/// @see ascension#HasTimer#timeElapsed
		void StatusBar::timeElapsed(ascension::Timer<StatusBar>&) {
			timeout();
		}

		/// @internal
		void StatusBar::timeout() {
			if(timer_.isActive()) {
				timer_.stop();
				remove(std::get<0>(timedMessage_), std::get<1>(timedMessage_));
			}
		}

		/// @internal
		void StatusBar::update() {
			if(!messages_.empty())
				::SetWindowTextW(handle().get(), std::get<2>(messages_.back()).c_str());
			else
				::SetWindowTextW(handle().get(), L"");
		}
	}
}
