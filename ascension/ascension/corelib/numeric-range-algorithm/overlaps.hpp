/**
 * @file overlaps.hpp
 * Defines @c overlaps free functions.
 * @author exeal
 * @date 2015-03-29 Created.
 */

#ifndef ASCENSION_OVERLAPS_HPP
#define ASCENSION_OVERLAPS_HPP
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>

namespace ascension {
	namespace algorithm {
		/**
		 * Returns @c true if the given two numeric range have some common subset.
		 * @tparam NumericRange1 The type of @a range1
		 * @tparam NumericRange2 The type of @a range2
		 * @param range1 The first numeric range
		 * @param range2 The second numeric range
		 * @return @c true if @a range1 and @a range2 overlap
		 * @note This function assumes that both @a range1 and @a range2 are ordered.
		 */
		template<typename NumericRange1, typename NumericRange2>
		inline bool overlaps(const NumericRange1& range1, const NumericRange2& range2) {
			return !((*boost::const_end(range1) <= *boost::const_begin(range2)) || (*boost::const_end(range2) <= *boost::const_begin(range1)));
		}

		/**
		 * Returns @c true if the given two numeric range have some common subset.
		 * @tparam NumericRange1 The type of @a range1
		 * @tparam NumericRange2 The type of @a range2
		 * @tparam BinaryPredicate The type of @a pred
		 * @param range1 The first numeric range
		 * @param range2 The second numeric range
		 * @param pred The binary predicate to compare the values
		 * @return @c true if @a range1 and @a range2 overlap
		 * @note This function assumes that both @a range1 and @a range2 are ordered.
		 */
		template<typename NumericRange1, typename NumericRange2, typename BinaryPredicate>
		inline bool overlaps(const NumericRange1& range1, const NumericRange2& range2, BinaryPredicate pred) {
			return !(pred(*boost::const_end(range2), *boost::const_begin(range1)) || pred(*boost::const_end(range1), *boost::const_begin(range2)));
		}
	}

	using algorithm::overlaps;
}

#endif // !ASCENSION_OVERLAPS_HPP
