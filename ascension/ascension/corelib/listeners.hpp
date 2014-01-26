/**
 * @file listeners.hpp
 * @author exeal
 * @date 2006-2011 was internal.hpp
 * @date 2011-04-03 separated from internal.hpp
 */

#ifndef ASCENSION_LISTENERS_HPP
#define ASCENSION_LISTENERS_HPP
#include <ascension/corelib/basic-types.hpp>	// ASCENSION_NONCOPYABLE_TAG
#include <algorithm>	// std.find
#include <list>
#include <stdexcept>	// std.invalid_argument

namespace ascension {
	namespace detail {
#if ASCENSION_ABANDONED_AT_VERSION_08
		/**
		 * @internal Manages a strategy object.
		 * @tparam Strategy The type of strategy object
		 * @deprecated 0.8 Use @c std#shared_ptr instead.
		 */
		template<typename Strategy>
		class StrategyPointer {
			ASCENSION_NONCOPYABLE_TAG(StrategyPointer);
		public:
			StrategyPointer() BOOST_NOEXCEPT : pointee_(nullptr), manages_(false) {}
			StrategyPointer(Strategy* pointee, bool manage) BOOST_NOEXCEPT : pointee_(pointee), manages_(manage) {}
			~StrategyPointer() BOOST_NOEXCEPT {if(manages_) delete pointee_;}
			Strategy& operator*() const BOOST_NOEXCEPT {return *pointee_;}
			Strategy* operator->() const BOOST_NOEXCEPT {return get();}
			Strategy* get() const BOOST_NOEXCEPT {return pointee_;}
			void reset(Strategy* newValue, bool manage) BOOST_NOEXCEPT {
				if(manages_ && newValue != pointee_)
					delete pointee_;
				pointee_ = newValue;
				manages_ = manage;
			}
			void reset() {reset(nullptr, false);}
		private:
			Strategy* pointee_;
			bool manages_;
		};
#endif // ASCENSION_ABANDONED_AT_VERSION_08

		/**
		 * @internal Manages the listeners (observers).
		 * @tparam Listener The type of listener/observer object
		 */
		template<class Listener>
		class Listeners {
			ASCENSION_NONCOPYABLE_TAG(Listeners);
		public:
			Listeners() BOOST_NOEXCEPT {}
			void add(Listener& listener) {
				if(std::find(listeners_.begin(), listeners_.end(), &listener) != listeners_.end())
					throw std::invalid_argument("The listener already has been registered.");
				listeners_.push_back(&listener);
			}
			void remove(Listener& listener) {
				const Iterator i(std::find(listeners_.begin(), listeners_.end(), &listener));
				if(i == listeners_.end())
					throw std::invalid_argument("The listener is not registered.");
				listeners_.erase(i);
			}
			void clear() BOOST_NOEXCEPT {listeners_.clear();}
			bool isEmpty() const BOOST_NOEXCEPT {return listeners_.empty();}
			void notify(void(Listener::*method)()) {
				for(Iterator i(listeners_.begin()), e(listeners_.end()), next; i != e; i = next) {
					next = i;
					++next;
					((*i)->*method)();
				}
			}
			template<typename Argument>
			void notify(void(Listener::*method)(Argument), Argument argument) {
				for(Iterator i(listeners_.begin()), e(listeners_.end()), next; i != e; i = next) {
					next = i;
					++next;
					((*i)->*method)(argument);
				}
			}
			template<typename Arg1, typename Arg2>
			void notify(void(Listener::*method)(Arg1, Arg2), Arg1 arg1, Arg2 arg2) {
				for(Iterator i(listeners_.begin()), e(listeners_.end()), next; i != e; i = next) {
					next = i;
					++next;
					((*i)->*method)(arg1, arg2);
				}
			}
			template<typename Arg1, typename Arg2, typename Arg3>
			void notify(void(Listener::*method)(Arg1, Arg2, Arg3), Arg1 arg1, Arg2 arg2, Arg3 arg3) {
				for(Iterator i(listeners_.begin()), e(listeners_.end()), next; i != e; i = next) {
					next = i;
					++next;
					((*i)->*method)(arg1, arg2, arg3);
				}
			}
			template<typename Arg1, typename Arg2, typename Arg3, typename Arg4>
			void notify(void(Listener::*method)(Arg1, Arg2, Arg3, Arg4), Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4) {
				for(Iterator i(listeners_.begin()), e(listeners_.end()), next; i != e; i = next)  {
					next = i;
					++next;
					((*i)->*method)(arg1, arg2, arg3, arg4);
				}
			}
			template<typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
			void notify(void(Listener::*method)(Arg1, Arg2, Arg3, Arg4, Arg5), Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5) {
				for(Iterator i(listeners_.begin()), e(listeners_.end()), next; i != e; i = next) {
					next = i;
					++next;
					((*i)->*method)(arg1, arg2, arg3, arg4, arg5);
				}
			}
		private:
			std::list<Listener*> listeners_;
			typedef typename std::list<Listener*>::iterator Iterator;
		};
	}
}

#endif // !ASCENSION_LISTENERS_HPP
