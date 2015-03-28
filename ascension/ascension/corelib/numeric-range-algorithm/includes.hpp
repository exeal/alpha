/**
 * @file includes.hpp
 * Defines @c includes functions.
 * @author exeal 
 * @date 2015-03-28 Separated from numeric-range.hpp and range.hpp.
 */

#ifndef ASCENSION_INCLUDES_HPP
#define ASCENSION_INCLUDES_HPP
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/value_type.hpp>
#include <type_traits>

namespace ascension {
	namespace algorithm {
		/**
		 * Returns @c true if the numeric range includes the given value.
		 * @tparam NumericRange The type of @a bounds
		 * @tparam Value The type of @a value
		 * @param bounds The numeric range to test
		 * @param value The numeric value to test
		 * @return @c true if @a value in [lo, hi)
		 * @note This function assumes that @a bounds is ordered.
		 */
		template<typename NumericRange, typename Value>
		inline bool includes(const NumericRange& bounds, Value value,
				typename std::enable_if<std::is_convertible<Value, typename boost::range_value<NumericRange>::type>::value>::type* = nullptr) {
			return value >= *boost::const_begin(bounds) && value < *boost::const_end(bounds);
		}

		/**
		 * Returns @c true if the numeric range includes the given value.
		 * @tparam NumericRange The type of @a bounds
		 * @tparam Value The type of @a value
		 * @tparam BinaryPredicate The type of @a pred
		 * @param bounds The numeric range to test
		 * @param value The numeric value to test
		 * @param pred The binary predicate to compare the values
		 * @return @c true if @a value in [lo, hi)
		 * @note This function assumes that @a bounds is ordered.
		 */
		template<typename NumericRange, typename Value, typename BinaryPredicate>
		inline bool includes(const NumericRange& bounds, Value value, BinaryPredicate pred,
				typename std::enable_if<std::is_convertible<Value, typename boost::range_value<NumericRange>::type>::value>::type* = nullptr) {
			return !pred(value, *boost::const_begin(bounds)) && pred(value, *boost::const_end(bounds));
		}

		/**
		 * Returns @c true if the numeric range includes the other numeric range.
		 * @tparam NumericRange1 The type of @a bounds
		 * @tparam NumericRange2 The type of @a range
		 * @param bounds The numeric range to test
		 * @param range The numeric range to test
		 * @return @c true if @a bounds includes @a range
		 * @note This function assumes that both @a bounds and @a range are ordered.
		 */
		template<typename NumericRange1, typename NumericRange2>
		inline bool includes(const NumericRange1& bounds, const NumericRange2& range,
				typename std::enable_if<!std::is_convertible<NumericRange2, typename boost::range_value<NumericRange1>::type>::value>::type* = nullptr) {
			return *boost::const_begin(range) >= *boost::const_begin(bounds) && *boost::const_end(range) <= *boost::const_end(bounds);
		}

		/**
		 * Returns @c true if the numeric range includes the other numeric range.
		 * @tparam NumericRange1 The type of @a bounds
		 * @tparam NumericRange2 The type of @a range
		 * @param bounds The numeric range to test
		 * @param range The numeric range to test
		 * @return @c true if @a bounds includes @a range
		 * @note This function assumes that both @a bounds and @a range are ordered.
		 */
		template<typename NumericRange1, typename NumericRange2, typename BinaryPredicate>
		inline bool includes(const NumericRange1& bounds, const NumericRange2& range, BinaryPredicate pred,
				typename std::enable_if<!std::is_convertible<NumericRange2, typename boost::range_value<NumericRange1>::type>::value>::type* = nullptr) {
			return !pred(*boost::const_begin(range), *boost::const_begin(bounds)) && !pred(*boost::const_end(range), *boost::const_end(bounds));
		}
	}

	using algorithm::includes;
}

#endif // !ASCENSION_INCLUDES_HPP
