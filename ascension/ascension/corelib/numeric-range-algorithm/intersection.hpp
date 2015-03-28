/**
 * @file intersection.hpp
 * Defines @c intersection functions.
 * @author exeal 
 * @date 2015-03-28 Separated from range.hpp.
 */

#ifndef ASCENSION_INTERSECTION_HPP
#define ASCENSION_INTERSECTION_HPP
#include <ascension/corelib/numeric-range-algorithm/order.hpp>
#include <boost/optional.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <functional>	// std.less
#include <utility>		// std.max, std.min

namespace ascension {
	namespace algorithm {
		/**
		 * Returns the ordered intersection of the two given numeric ranges.
		 * @tparam NumericRange The type of @a range1 and @a range2. This should have the constructor which takes the
		 *                      two parameters to initialize the result range
		 * @param range1 The first numeric range
		 * @param range2 The second numeric range
		 * @return The intersection, or @c boost#none if the two ranges does not intersect
		 * @note This function assumes that both @a range1 and @a range2 are ordered.
		 */
		template<typename NumericRange>
		inline boost::optional<NumericRange> intersection(const NumericRange& range1, const NumericRange& range2) {
			return intersection(range1, range2, std::less<typename boost::range_value<NumericRange>::type>());
		}

		/**
		 * Returns the ordered intersection of the two given numeric ranges.
		 * @tparam NumericRange The type of @a range1 and @a range2. This should have the constructor which takes the
		 *                      two parameters to initialize the result range
		 * @tparam BinaryPredicate The type of @a pred
		 * @param range1 The first numeric range
		 * @param range2 The second numeric range
		 * @param pred The binary predicate to compare the values
		 * @return The intersection
		 * @note This function assumes that both @a range1 and @a range2 are ordered.
		 */
		template<typename NumericRange, typename BinaryPredicate>
		inline boost::optional<NumericRange> intersection(const NumericRange& range1, const NumericRange& range2, BinaryPredicate pred) {
			const NumericRange temp(
				std::max(*boost::const_begin(range1), *boost::const_begin(range2), pred),
				std::min(*boost::const_end(range1), *boost::const_end(range2), pred));
			return isOrdered(temp, pred) ? boost::make_optional(temp) : boost::none;
		}
	}

	using algorithm::intersection;
}

#endif // !ASCENSION_INTERSECTION_HPP
