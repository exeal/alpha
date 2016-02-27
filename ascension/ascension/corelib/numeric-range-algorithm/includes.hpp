/**
 * @file includes.hpp
 * Defines @c includes functions.
 * @author exeal 
 * @date 2015-03-28 Separated from numeric-range.hpp and range.hpp.
 * @see encompasses.hpp
 */

#ifndef ASCENSION_INCLUDES_HPP
#define ASCENSION_INCLUDES_HPP
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/value_type.hpp>

namespace ascension {
	namespace algorithm {
		/**
		 * Returns @c true if the numeric range includes the given value.
		 * @tparam NumericRange The type of @a bounds
		 * @param bounds The numeric range to test
		 * @param value The numeric value to test
		 * @return @c true if @a value in [lo, hi)
		 * @note This function assumes that @a bounds is ordered.
		 */
		template<typename NumericRange>
		inline bool includes(const NumericRange& bounds, typename boost::range_value<NumericRange>::type value) {
			return value >= *boost::const_begin(bounds) && value < *boost::const_end(bounds);
		}

		/**
		 * Returns @c true if the numeric range includes the given value.
		 * @tparam NumericRange The type of @a bounds
		 * @tparam BinaryPredicate The type of @a pred
		 * @param bounds The numeric range to test
		 * @param value The numeric value to test
		 * @param pred The binary predicate to compare the values
		 * @return @c true if @a value in [lo, hi)
		 * @note This function assumes that @a bounds is ordered.
		 */
		template<typename NumericRange, typename BinaryPredicate>
		inline bool includes(const NumericRange& bounds, typename boost::range_value<NumericRange>::type value, BinaryPredicate pred) {
			return !pred(value, *boost::const_begin(bounds)) && pred(value, *boost::const_end(bounds));
		}
	}

	using algorithm::includes;
}

#endif // !ASCENSION_INCLUDES_HPP
