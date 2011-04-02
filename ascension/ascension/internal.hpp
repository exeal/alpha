/**
 * @file internal.hpp
 * @brief Private entries used by Ascension internally.
 * @author exeal
 * @date 2006-2011
 */

#ifndef ASCENSION_INTERNAL_HPP
#define ASCENSION_INTERNAL_HPP

#include <ascension/corelib/basic-types.hpp>
#include <ascension/corelib/type-traits.hpp>
#include <algorithm>	// std.fill, std.find, std.upper_bound
#include <list>
#include <stdexcept>	// std.invalid_argument

namespace ascension {

	/**
	 * Defines entities the clients of Ascension do not access.
	 * @internal
	 */
	namespace detail {

		/**
		 * Returns the iterator addresses the first element in the sorted range which satisfies
		 * comp(value, *i) (@a i is the iterator).
		 * @tparam BidirectionalIterator The type of @a first and @a last
		 * @tparam T The type of @a value
		 * @tparam Comp The type of @a compare
		 * @param first, last The bidirectional iterators addresses the beginning and the end of
		 *                    the sequence
		 * @param value The value to search
		 * @param compare The comparison function object
		 * @return The result, or @a last if comp(value, *first) returned @c true
		 */
		template<typename BidirectionalIterator, typename T, typename Comp>
		inline BidirectionalIterator searchBound(BidirectionalIterator first, BidirectionalIterator last, const T& value, Comp compare) {
			BidirectionalIterator temp(std::upper_bound(first, last, value, compare));
			return (temp != first) ? --temp : last;
		}

		/// The overloaded version uses @c operator&lt;.
		template<typename BidirectionalIterator, typename T>
		inline BidirectionalIterator searchBound(BidirectionalIterator first, BidirectionalIterator last, const T& value) {
			BidirectionalIterator temp(std::upper_bound(first, last, value));
			return (temp != first) ? --temp : last;
		}

		/// Returns absolute difference of two numerals.
		template<typename T> inline std::size_t distance(T i0, T i1) {return (i0 > i1) ? i0 - i1 : i1 - i0;}

		/// @internal
		template<typename T> class ValueSaver {
		public:
			/// Constructor saves the value.
			ValueSaver(T& value) : value_(value), originalValue_(value) {}
			/// Destructor restores the value.
			~ValueSaver() {value_ = originalValue_;}
		private:
			T& value_;
			T originalValue_;
		};

		/// Manages a strategy object.
		template<typename Strategy> class StrategyPointer {
			ASCENSION_NONCOPYABLE_TAG(StrategyPointer);
		public:
			StrategyPointer() /*throw()*/ : pointee_(0), manages_(false) {}
			StrategyPointer(Strategy* pointee, bool manage) /*throw()*/ : pointee_(pointee), manages_(manage) {}
			~StrategyPointer() /*throw()*/ {if(manages_) delete pointee_;}
			Strategy& operator*() const /*throw()*/ {return *pointee_;}
			Strategy* operator->() const /*throw()*/ {return get();}
			Strategy* get() const /*throw()*/ {return pointee_;}
			void reset(Strategy* newValue, bool manage) /*throw()*/ {
				if(manages_ && newValue != pointee_) delete pointee_; pointee_ = newValue; manages_ = manage;}
			void reset() {reset(0, false);}
		private:
			Strategy* pointee_;
			bool manages_;
		};

		/// Manages the listeners.
		template<class Listener> class Listeners {
			ASCENSION_NONCOPYABLE_TAG(Listeners);
		public:
			Listeners() /*throw()*/ {}
			void add(Listener& listener) {
				if(std::find(listeners_.begin(), listeners_.end(), &listener) != listeners_.end())
					throw std::invalid_argument("The listener already has been registered.");
				listeners_.push_back(&listener);
			}
			void remove(Listener& listener) {
				const Iterator i = std::find(listeners_.begin(), listeners_.end(), &listener);
				if(i == listeners_.end()) throw std::invalid_argument("The listener is not registered.");
				listeners_.erase(i);
			}
			void clear() /*throw()*/ {listeners_.clear();}
			bool isEmpty() const /*throw()*/ {return listeners_.empty();}
			void notify(void(Listener::*method)()) {
				for(Iterator i(listeners_.begin()), e(listeners_.end()), next; i != e; i = next) {next = i; ++next; ((*i)->*method)();}}
			template<typename Argument> void notify(void(Listener::*method)(Argument), Argument argument) {
				for(Iterator i(listeners_.begin()), e(listeners_.end()), next; i != e; i = next) {next = i; ++next; ((*i)->*method)(argument);}}
			template<typename Arg1, typename Arg2>
			void notify(void(Listener::*method)(Arg1, Arg2), Arg1 arg1, Arg2 arg2) {
				for(Iterator i(listeners_.begin()), e(listeners_.end()), next; i != e; i = next) {next = i; ++next;  ((*i)->*method)(arg1, arg2);}}
			template<typename Arg1, typename Arg2, typename Arg3>
			void notify(void(Listener::*method)(Arg1, Arg2, Arg3), Arg1 arg1, Arg2 arg2, Arg3 arg3) {
				for(Iterator i(listeners_.begin()), e(listeners_.end()), next; i != e; i = next)  {next = i; ++next; ((*i)->*method)(arg1, arg2, arg3);}}
			template<typename Arg1, typename Arg2, typename Arg3, typename Arg4>
			void notify(void(Listener::*method)(Arg1, Arg2, Arg3, Arg4), Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4) {
				for(Iterator i(listeners_.begin()), e(listeners_.end()), next; i != e; i = next)  {next = i; ++next; ((*i)->*method)(arg1, arg2, arg3, arg4);}}
			template<typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
			void notify(void(Listener::*method)(Arg1, Arg2, Arg3, Arg4, Arg5), Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5) {
				for(Iterator i(listeners_.begin()), e(listeners_.end()), next; i != e; i = next) {next = i; ++next;  ((*i)->*method)(arg1, arg2, arg3, arg4, arg5);}}
		private:
			std::list<Listener*> listeners_;
			typedef typename std::list<Listener*>::iterator Iterator;
		};

	} // namespace detail

	/// Signed @c length_t
	typedef detail::RemoveSigned<length_t>::Type signed_length_t;

} // namespace ascension

#endif // !ASCENSION_INTERNAL_HPP
