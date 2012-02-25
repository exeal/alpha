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
#include <boost/signals2.hpp>

namespace ascension {

	template<typename Signal>
	class SignalConnector {
	public:
		SignalConnector(Signal& signal) /*throw()*/ : signal_(signal) {
		}
		boost::signals2::connection connect(const typename Signal::slot_type& slot,
				boost::signals2::connect_position where = boost::signals2::at_back) {
			return signal_.connect(slot, where);
		}
		template<typename Slot>
		void disconnect(const Slot& slot) {
			return signal_.disconnect(slot);
		}
	private:
		Signal& signal_;
	};

#define ASCENSION_DEFINE_SIGNAL(signalTypeName, signature, signalName)	\
public:																	\
	typedef boost::signals2::signal<signature> signalTypeName;			\
	SignalConnector<signalTypeName> signalName() const /*throw()*/ {	\
		return const_cast<signalTypeName&>(signalName##_);				\
	}																	\
private:																\
	signalTypeName signalName##_

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
			StrategyPointer() /*throw()*/ : pointee_(nullptr), manages_(false) {}
			StrategyPointer(Strategy* pointee, bool manage) /*throw()*/ : pointee_(pointee), manages_(manage) {}
			~StrategyPointer() /*throw()*/ {if(manages_) delete pointee_;}
			Strategy& operator*() const /*throw()*/ {return *pointee_;}
			Strategy* operator->() const /*throw()*/ {return get();}
			Strategy* get() const /*throw()*/ {return pointee_;}
			void reset(Strategy* newValue, bool manage) /*throw()*/ {
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
			Listeners() /*throw()*/ {}
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
			void clear() /*throw()*/ {listeners_.clear();}
			bool isEmpty() const /*throw()*/ {return listeners_.empty();}
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
