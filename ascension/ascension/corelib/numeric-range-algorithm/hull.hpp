/**
 * @file hull.hpp
 * Defines @c hull free functions.
 * @author exeal
 * @date 2015-03-28 Created.
 */

#ifndef ASCENSION_HULL_HPP
#define ASCENSION_HULL_HPP
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/value_type.hpp>
#include <utility>	// std.max, std.min

namespace ascension {
	namespace algorithm {
		/**
		 * Returns the smallest range which contains the two given numeric ranges.
		 * @tparam NumericRange The type of @a range1 and @a range2. This should have the constructor which takes the
		 *                      two parameters to initialize the result range
		 * @param range1 The first numeric range
		 * @param range2 The second numeric range
		 * @return The result range
		 * @note This function assumes that both @a range1 and @a range2 are ordered.
		 */
		template<typename NumericRange>
		inline NumericRange hull(const NumericRange& range1, const NumericRange& range2) {
			return hull(range1, range2, std::less<typename boost::range_value<NumericRange>::type>());
		}

		/**
		 * Returns the smallest range which contains the two given numeric ranges.
		 * @tparam NumericRange The type of @a range1 and @a range2. This should have the constructor which takes the
		 *                      two parameters to initialize the result range
		 * @tparam BinaryPredicate The type of @a pred
		 * @param range1 The first numeric range
		 * @param range2 The second numeric range
		 * @param pred The binary predicate to compare the values
		 * @return The result range
		 * @note This function assumes that both @a range1 and @a range2 are ordered.
		 */
		template<typename NumericRange, typename BinaryPredicate>
		inline NumericRange hull(const NumericRange& range1, const NumericRange& range2, BinaryPredicate pred) {
			return NumericRange(
				std::min(*boost::const_begin(range1), *boost::const_begin(range2), pred),
				std::min(*boost::const_end(range1), *boost::const_end(range2), pred));
		}

#if 0
		template<typename NumericRange, typename Value>
		inline NumericRange hull(const NumericRange& range, Value value);
		template<typename Value, typename NumericRange>
		inline NumericRange hull(Value value, const NumericRange& range);
		template<typename NumericRange, typename Value, typename BinaryPredicate>
		inline NumericRange hull(const NumericRange& range, Value value, BinaryPredicate pred);
		template<typename Value, typename NumericRange, typename BinaryPredicate>
		inline NumericRange hull(Value value, const NumericRange& range, BinaryPredicate pred);
#endif
	}

	using algorithm::hull;
}

#endif // !ASCENSION_HULL_HPP
