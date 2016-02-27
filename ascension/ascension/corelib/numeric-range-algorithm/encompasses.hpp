/**
 * @file encompasses.hpp
 * Defines @c encompasses functions.
 * @author exeal
 * @see includes.hpp
 */

#ifndef ASCENSION_ENCOMPASSES_HPP
#define ASCENSION_ENCOMPASSES_HPP
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/value_type.hpp>

namespace ascension {
	namespace algorithm {
		/**
		 * Returns @c true if the numeric range encompasses the given value.
		 * @tparam NumericRange The type of @a bounds
		 * @param bounds The numeric range to test
		 * @param value The numeric value to test
		 * @return @c true if @a value in [lo, hi]
		 * @note This function assumes that @a bounds is ordered.
		 */
		template<typename NumericRange>
		inline bool encompasses(const NumericRange& bounds, typename boost::range_value<NumericRange>::type value) {
			return value >= *boost::const_begin(bounds) && value <= *boost::const_end(bounds);
		}

		/**
		 * Returns @c true if the numeric range encompasses the given value.
		 * @tparam NumericRange The type of @a bounds
		 * @tparam BinaryPredicate The type of @a pred
		 * @param bounds The numeric range to test
		 * @param value The numeric value to test
		 * @param pred The binary predicate to compare the values
		 * @return @c true if @a value in [lo, hi]
		 * @note This function assumes that @a bounds is ordered.
		 */
		template<typename NumericRange, typename BinaryPredicate>
		inline bool encompasses(const NumericRange& bounds, typename boost::range_value<NumericRange>::type value, BinaryPredicate pred) {
			return !pred(value, *boost::const_begin(bounds)) && !pred(*boost::const_end(bounds), value);
		}

		/**
		 * Returns @c true if the numeric range encompasses the other numeric range.
		 * @tparam NumericRange The type of @a bounds and @a range
		 * @param bounds The numeric range to test
		 * @param range The numeric range to test
		 * @return @c true if @a bounds encompasses @a range
		 * @note This function assumes that both @a bounds and @a range are ordered.
		 */
		template<typename NumericRange>
		inline bool encompasses(const NumericRange& bounds, const NumericRange& range) {
			return *boost::const_begin(range) >= *boost::const_begin(bounds) && *boost::const_end(range) <= *boost::const_end(bounds);
		}

		/**
		 * Returns @c true if the numeric range encompasses the other numeric range.
		 * @tparam NumericRange The type of @a bounds and @a range
		 * @param bounds The numeric range to test
		 * @param range The numeric range to test
		 * @return @c true if @a bounds encompasses @a range
		 * @note This function assumes that both @a bounds and @a range are ordered.
		 */
		template<typename NumericRange, typename BinaryPredicate>
		inline bool encompasses(const NumericRange& bounds, const NumericRange& range, BinaryPredicate pred) {
			return !pred(*boost::const_begin(range), *boost::const_begin(bounds)) && !pred(*boost::const_end(range), *boost::const_end(bounds));
		}
	}

	using algorithm::encompasses;
}

#endif // !ASCENSION_ENCOMPASSES_HPP
