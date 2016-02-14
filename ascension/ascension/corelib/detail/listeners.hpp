/**
 * @file listeners.hpp
 * @author exeal
 * @date 2006-2011 was internal.hpp
 * @date 2011-04-03 separated from internal.hpp
 */

#ifndef ASCENSION_LISTENERS_HPP
#define ASCENSION_LISTENERS_HPP
#include <boost/core/noncopyable.hpp>
#include <algorithm>	// std.find
#include <list>
#include <stdexcept>	// std.invalid_argument

namespace ascension {
	namespace detail {
		/**
		 * @internal Manages the listeners (observers).
		 * @tparam Listener The type of listener/observer object
		 */
		template<class Listener>
		class Listeners : private boost::noncopyable {
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
