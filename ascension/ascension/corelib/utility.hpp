/**
 * @file utility.hpp
 * @author exeal
 * @date 2006-2011 was internal.hpp
 * @date 2011-04-03 separated from internal.hpp
 */

#ifndef ASCENSION_UTILITY_HPP
#define ASCENSION_UTILITY_HPP
#include <ascension/corelib/future/type-traits.hpp>	// RemoveSigned
#include <algorithm>	// std.upper_bound
#include <cstddef>		// std.size_t

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

		/// @internal
		template<typename T>
		class ValueSaver {
		public:
			/// Constructor saves the value.
			ValueSaver(T& value) : value_(value), originalValue_(value) {}
			/// Destructor restores the value.
			~ValueSaver() {value_ = originalValue_;}
		private:
			T& value_;
			T originalValue_;
		};

#ifdef ASCENSION_ABANDONED_AT_VERSION_08
		/// @internal Produces @c operator&gt;, @c operator&lt;= and @c operator&gt;= from @c operator&lt;.
		template<typename Derived>
		struct LessThanComparable {
			friend bool operator>(const Derived& lhs, const Derived& rhs) {return rhs < lhs;}
			friend bool operator<=(const Derived& lhs, const Derived& rhs) {return !(rhs < lhs);}
			friend bool operator>=(const Derived& lhs, const Derived& rhs) {return !(lhs < rhs);}
		};

		/// @internal Produces @c operator!= from @c operator==.
		template<typename Derived>
		struct EqualityComparable {
			friend bool operator!=(const Derived& lhs, const Derived& rhs) {return !(lhs == rhs);}
		};
#endif // ASCENSION_ABANDONED_AT_VERSION_08

	} // namespace detail

} // namespace ascension

#endif // !ASCENSION_UTILITY_HPP
