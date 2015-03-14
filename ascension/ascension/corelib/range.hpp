/**
 * @file range.hpp
 * @author exeal
 * @date 2004-2010 (was basic-types.hpp)
 * @date 2011-03-25 separated from basic-types.hpp
 * @date 2011-2013
 */

#ifndef ASCENSION_RANGE_HPP
#define ASCENSION_RANGE_HPP
#include <functional>	// std.less
#include <iterator>		// std.iterator_traits
#include <locale>		// std.use_facet, ...
#include <sstream>		// std.basic_ostream, std.ostringstream
#include <utility>		// std.max, std.min, std.pair
#include <ascension/corelib/future.hpp>
#include <boost/operators.hpp>
#include <boost/range/irange.hpp>

namespace ascension {
	template<typename RandomAccessTraversalRange>
	inline bool includes(const RandomAccessTraversalRange& range,
			typename boost::range_iterator<RandomAccessTraversalRange>::type position) {
		return position >= boost::const_begin(range) && position < boost::const_end(range);
	}

	template<typename RandomAccessTraversalRange, typename BinaryPredicate>
	inline bool includes(const RandomAccessTraversalRange& range,
			typename boost::range_iterator<RandomAccessTraversalRange>::type position, BinaryPredicate pred) {
		BOOST_CONCEPT_ASSERT((boost::RandomAccessRangeConcept<RandomAccessTraversalRange>));
		return !pred(position, range.begin()) && pred(position, range.end());
	}

	template<typename Integer>
	inline bool includes(const boost::integer_range<Integer>& range,
			typename boost::integer_range<Integer>::value_type value) {
		return value >= *range.begin() && value < *range.end();
	}

	template<typename Integer, typename BinaryPredicate>
	inline bool includes(const boost::integer_range<Integer>& range,
			typename boost::integer_range<Integer>::value_type value, BinaryPredicate pred) {
		return !pred(value, *range.begin()) && pred(value, *range.end());
	}

	template<typename RandomAccessTraversalRange1, typename RandomAccessTraversalRange2>
	inline bool includes(const RandomAccessTraversalRange1& range, const RandomAccessTraversalRange2& subrange) {
		BOOST_CONCEPT_ASSERT((boost::RandomAccessRangeConcept<RandomAccessTraversalRange1>));
		BOOST_CONCEPT_ASSERT((boost::RandomAccessRangeConcept<RandomAccessTraversalRange2>));
		return boost::const_begin(subrange) >= boost::const_begin(range) && boost::const_end(subrange) <= boost::const_begin(range);
	}

	template<typename RandomAccessTraversalRange1, typename RandomAccessTraversalRange2, typename BinaryPredicate>
	inline bool includes(const RandomAccessTraversalRange1& range, const RandomAccessTraversalRange2& subrange, BinaryPredicate pred) {
		BOOST_CONCEPT_ASSERT((boost::RandomAccessRangeConcept<RandomAccessTraversalRange1>));
		BOOST_CONCEPT_ASSERT((boost::RandomAccessRangeConcept<RandomAccessTraversalRange2>));
		return !pred(boost::const_begin(subrange), boost::const_begin(range)) && !pred(boost::const_end(range), boost::const_end(subrange));
	}

	template<typename Integer1, typename Integer2>
	inline bool includes(const boost::integer_range<Integer1>& range, const boost::integer_range<Integer2>& subrange) {
		return *subrange.begin() >= *range.begin() && *subrange.end() <= *range.end();
	}

	template<typename Integer1, typename Integer2, typename BinaryPredicate>
	inline bool includes(const boost::integer_range<Integer1>& range, const boost::integer_range<Integer2>& subrange, BinaryPredicate pred) {
		return !pred(*subrange.begin(), *range.begin()) && !pred(*range.end(), *subrange.end());
	}

	template<typename RandomAccessTraversalRange>
	inline RandomAccessTraversalRange intersection(
			const RandomAccessTraversalRange& range1, const RandomAccessTraversalRange& range2) {
		const typename boost::range_iterator<RandomAccessTraversalRange>::type
			b(std::max(boost::const_begin(range1), boost::const_begin(range2))),
			e(std::min(boost::const_end(range1), boost::const_end(range2)));
		return boost::make_iterator_range(b, std::max(b, e));
	}

	template<typename RandomAccessTraversalRange, typename BinaryPredicate>
	inline RandomAccessTraversalRange intersection(
			const RandomAccessTraversalRange& range1, const RandomAccessTraversalRange& range2,
			BinaryPredicate pred) {
		const typename boost::range_iterator<RandomAccessTraversalRange>::type
			b(std::max(boost::const_begin(range1), boost::const_begin(range2), pred)),
			e(std::min(boost::const_end(range1), boost::const_end(range2), pred));
		return boost::make_iterator_range(b, std::max(b, e, pred));
	}

	template<typename Integer>
	inline boost::integer_range<Integer> intersection(
			const boost::integer_range<Integer>& range1, const boost::integer_range<Integer>& range2) {
		const Integer b(std::max(*range1.begin(), *range2.begin())), e(std::min(*range1.end(), *range2.end()));
		return boost::irange(b, std::max(b, e));
	}

	template<typename Integer, typename BinaryPredicate>
	inline boost::integer_range<Integer> intersection(const boost::integer_range<Integer>& range1,
			const boost::integer_range<Integer>& range2, BinaryPredicate pred) {
		const Integer
			b(std::max(*range1.begin(), *range2.begin(), pred)),
			e(std::min(*range1.end(), *range2.end(), pred));
		return boost::irange(b, std::max(b, e, pred));
	}

	template<typename RandomAccessTraversalRange>
	inline bool intersects(const RandomAccessTraversalRange& range1, const RandomAccessTraversalRange& range2) {
		return boost::empty(intersection(range1, range2));
	}

	template<typename RandomAccessTraversalRange, typename BinaryPredicate>
	inline bool intersects(const RandomAccessTraversalRange& range1, const RandomAccessTraversalRange& range2, BinaryPredicate pred) {
		return boost::empty(intersection(range1, range2, pred));
	}

	template<typename RandomAccessTraversalRange>
	inline boost::iterator_range<typename boost::range_iterator<RandomAccessTraversalRange>::type> ordered(const RandomAccessTraversalRange& range) {
		const auto temp = std::minmax(*boost::const_begin(range), *boost::const_end(range));
		return boost::make_iterator_range<typename boost::range_iterator<RandomAccessTraversalRange>::type>(temp);
	}

	template<typename Integer>
	inline boost::integer_range<Integer> ordered(const boost::integer_range<Integer>& range) {
		const auto temp = std::minmax(*range.begin(), *range.end());
		return boost::irange(temp.first, temp.second);
	}
} // namespace ascension

#endif // !ASCENSION_RANGE_HPP
