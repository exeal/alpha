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

	namespace detail {
		template<typename T>
		class HasDifferenceType {
			typedef char True;
			typedef struct {char temp[2];} False;
			template<typename U> static True test(typename U::difference_type*);
			template<typename U> static False test(...);
		public:
			static const bool value = sizeof(test<std::iterator_traits<T>>(nullptr)) == sizeof(True);
		};

		template<typename T, bool hasDifferenceType>
		struct IteratorHasDifferenceTypeOrNot {typedef T Type;};

		template<typename T>
		struct IteratorHasDifferenceTypeOrNot<T, true> {typedef typename std::iterator_traits<T>::difference_type Type;};

		template<typename T, bool isArithmetic>
		struct ArithmeticOrNot {typedef T Type;};

		template<typename T>
		struct ArithmeticOrNot<T, false> : IteratorHasDifferenceTypeOrNot<T, HasDifferenceType<T>::value> {};

		template<typename T>
		struct DifferenceType : ArithmeticOrNot<T, std::is_arithmetic<T>::value> {};
	}

#ifdef ASCENSION_ABANDONED_AT_VERSION_08
	/**
	 * Represents an invariant range.
	 * @tparam T The element type. This type shall be @c LessThanComparable
	 * @tparam Comp The type for comparisons
	 * @note This class is not compatible with Boost.Range.
	 * @see BasicStringPiece, kernel#Region
	 */
	template<typename T, typename Comp = std::less<T>>
	class Range : protected std::pair<T, T>, private boost::equality_comparable<Range<T, Comp>> {
	public:
		typedef T value_type;
	public:
		/// Default constructor.
		Range() {}
		/**
		 * Constructor takes the beginning and the end of the range.
		 * @tparam T1, T2 The types for @a pair
		 * @param pair The beginning and the end. Both std::min(pair.first, pair.second) and
		 *             std::max(pair.first, pair.second) should be valid
		 */
		template<typename T1, typename T2>
		explicit Range(const std::pair<T1, T2>& pair) :
//			std::pair<value_type, value_type>(std::minmax(pair.first, pair.second, Comp())) {}
			std::pair<value_type, value_type>(
				std::min(pair.first, pair.second, Comp()), std::max(pair.first, pair.second, Comp())) {}
		/**
		 * Constructor takes the beginning and the end of the range.
		 * @param v1, v2 The beginning and the end. Both std::min(v1, v2, Comp()) and
		 *               std::max(v1, v2, Comp()) should be valid
		 */
		Range(value_type v1, value_type v2) :
//			std::pair<value_type, value_type>(std::minmax(v1, v2, Comp())) {}
			std::pair<value_type, value_type>(std::min(v1, v2, Comp()), std::max(v1, v2, Comp())) {}
		/// Equality operator.
		bool operator==(const Range& other) const {
			return static_cast<const std::pair<T, T>&>(*this) == static_cast<const std::pair<T, T>&>(other);}
		/// Returns the beginning (minimum) of the range.
		value_type beginning() const {return std::pair<T, T>::first;}
		/// Returns the end (maximum) of the range.
		value_type end() const {return std::pair<T, T>::second;}
	};
#endif	// ASCENSION_ABANDONED_AT_VERSION_08

#ifdef ASCENSION_ABANDONED_AT_VERSION_08
	/**
	 * Returns @c true if the given value is included by the range.
	 * @tparam T The element type of the range
	 * @tparam Comp The comparison type of the range
	 * @tparam U The type of @a value
	 * @param range The range
	 * @param value The value to test
	 * @return true if @a value is included by @a range
	 */
	template<typename T, typename Comp, typename U>
	inline bool includes(const Range<T, Comp>& range, const U& value) {
		const Comp lessThan;
//		return value >= range.beginning() && value < range.end();
		return !lessThan(value, range.beginning()) && lessThan(value, range.end());
	}
#else
	template<typename RandomAccessTraversalRange>
	inline bool includes(const RandomAccessTraversalRange& range,
			typename boost::range_iterator<RandomAccessTraversalRange>::type position) {
		return position >= boost::const_begin(range) && position < boost::const_end(range);
	}
	template<typename RandomAccessTraversalRange, typename BinaryPredicate>
	inline bool includes(const RandomAccessTraversalRange& range,
			typename boost::range_iterator<RandomAccessTraversalRange>::type position, BinaryPredicate pred) {
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
#endif	// ASCENSION_ABANDONED_AT_VERSION_08

#ifdef ASCENSION_ABANDONED_AT_VERSION_08
	/**
	 * Returns @c true if the given range is included by the range.
	 * @tparam T The element type of the range
	 * @tparam Comp The comparison type of the range
	 * @tparam Other The type of @a other
	 * @param range The range
	 * @param other The other range to test
	 * @return true if @a other is included by the range
	 */
	template<typename T, typename Comp, typename Other>
	inline bool includes(const Range<T, Comp>& range, const Range<Other, Comp>& other) {
		const Comp lessThan;
//		return other.beginning() >= range.beginning() && other.end() <= range.end();
		return !lessThan(other.beginning(), range.beginning()) && !lessThan(range.end(), other.end());
	}
#else
	template<typename RandomAccessTraversalRange1, typename RandomAccessTraversalRange2>
	inline bool includes(const RandomAccessTraversalRange1& range, const RandomAccessTraversalRange2& subrange) {
		return boost::const_begin(subrange) >= boost::const_begin(range) && boost::const_end(subrange) <= boost::const_begin(range);
	}
	template<typename RandomAccessTraversalRange1, typename RandomAccessTraversalRange2, typename BinaryPredicate>
	inline bool includes(const RandomAccessTraversalRange1& range, const RandomAccessTraversalRange2& subrange, BinaryPredicate pred) {
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
#endif	// ASCENSION_ABANDONED_AT_VERSION_08

#ifdef ASCENSION_ABANDONED_AT_VERSION_08
	/**
	 * Returns the intersection of the two ranges.
	 * @tparam T The element type of the range
	 * @tparam Comp The comparison type of the range
	 * @tparam Other The type of @a other
	 * @param range The range
	 * @param other The other range
	 * @return The intersection, or empty range if the two ranges were not intersected
	 * @see intersects, merged
	 */
	template<typename T, typename Comp, typename Other>
	inline Range<T, Comp> intersection(const Range<T, Comp>& range, const Range<Other, Comp>& other) {
		const Comp lessThan;
		const T b(std::max<T, Comp>(range.beginning(), other.beginning(), lessThan));
		const T e(std::min<T, Comp>(range.end(), other.end(), lessThan));
		return Range<T, Comp>(b, std::max(b, e, lessThan));
	}
#else
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
#endif	// ASCENSION_ABANDONED_AT_VERSION_08

#ifdef ASCENSION_ABANDONED_AT_VERSION_08
	/**
	 * Returns @c true if the range intersects with the other.
	 * @tparam T The element type of the range
	 * @tparam Comp The comparison type of the range
	 * @tparam Other The type of @a other
	 * @param range The range
	 * @param other The other range to test
	 * @return true if @a range intersects with @a other
	 * @see intersection
	 */
	template<typename T, typename Comp, typename Other>
	inline bool intersects(const Range<T, Comp>& range, const Range<Other, Comp>& other) {
		return !isEmpty(intersection(range, other));
	}
#else
	template<typename RandomAccessTraversalRange>
	inline bool intersects(const RandomAccessTraversalRange& range1, const RandomAccessTraversalRange& range2) {
		return boost::empty(intersection(range1, range2));
	}
	template<typename RandomAccessTraversalRange, typename BinaryPredicate>
	inline bool intersects(const RandomAccessTraversalRange& range1, const RandomAccessTraversalRange& range2, BinaryPredicate pred) {
		return boost::empty(intersection(range1, range2, pred));
	}
#endif	// ASCENSION_ABANDONED_AT_VERSION_08

#ifdef ASCENSION_ABANDONED_AT_VERSION_08
	/**
	 * Returns @c true if the range is empty.
	 * @tparam T The element type of the range
	 * @tparam Comp The comparison type of the range
	 * @param range The range to test
	 * @return true if @a range is empty
	 */
	template<typename T, typename Comp>
	inline bool isEmpty(const Range<T, Comp>& range,
			typename std::tr1::enable_if<std::is_same<Comp, std::less<T>>::value>::type* = nullptr) {
		return range.beginning() == range.end();
	}
#endif	// ASCENSION_ABANDONED_AT_VERSION_08

#ifdef ASCENSION_ABANDONED_AT_VERSION_08
	/**
	 * Returns @c true if the range is empty.
	 * @tparam T The element type of the range
	 * @tparam Comp The comparison type of the range
	 * @param range The range to test
	 * @return true if @a range is empty
	 */
	template<typename T, typename Comp>
	inline bool isEmpty(const Range<T, Comp>& range,
			typename std::tr1::enable_if<!std::is_same<Comp, std::less<T>>::value>::type* = nullptr) {
		const Comp lessThan;
		return !lessThan(range.beginning(), range.end()) && !lessThan(range.end(), range.beginning());
	}
#endif	// ASCENSION_ABANDONED_AT_VERSION_08

#ifdef ASCENSION_ABANDONED_AT_VERSION_08
	/**
	 * Returns the length of the range.
	 * @tparam T The element type of the range
	 * @tparam Comp The comparison type of the range
	 * @param range The range to test
	 * @return The length of @a range
	 * @note This class does not define a method named "size".
	 */s
	template<typename T, typename Comp>
	inline typename detail::DifferenceType<T>::Type length(const Range<T, Comp>& range) {
		return range.end() - range.beginning();
	}
#endif	// ASCENSION_ABANDONED_AT_VERSION_08

#ifdef ASCENSION_ABANDONED_AT_VERSION_08
	/// Returns a @c Range object using the @c std#pair object.
	template<typename T>
	inline Range<T> makeRange(const std::pair<T, T>& pair) {return Range<T>(pair);}

	/// Returns a @c Range object using the @c std#pair object.
	template<typename T, typename Comp>
	inline Range<T, Comp> makeRange(const std::pair<T, T>& pair, const Comp&) {return Range<T, Comp>(pair);}

	/// Returns a @c Range object using the given two values.
	template<typename T>
	inline Range<T> makeRange(T v1, T v2) {return Range<T>(v1, v2);}

	/// Returns a @c Range object using the given two values.
	template<typename T, typename Comp>
	inline Range<T, Comp> makeRange(T v1, T v2, const Comp&) {return Range<T, Comp>(v1, v2);}
#endif	// ASCENSION_ABANDONED_AT_VERSION_08

#ifdef ASCENSION_ABANDONED_AT_VERSION_08
	/**
	 * Returns the new merged range of this and the given ones.
	 * @tparam T The element type of the range
	 * @tparam Comp The comparison type of the range
	 * @tparam Other The type of @a other
	 * @param range The range
	 * @param other The other range
	 * @return The merged or empty range
	 */
	template<typename T, typename Comp, typename Other>
	inline Range<T, Comp> merged(const Range<T, Comp>& range, const Range<Other, Comp>& other) {
		if(isEmpty(other))
			return range;
		else if(isEmpty(range))
			return other;
		return Range<T, Comp>(
			std::min(range.beginning(), other.beginning(), Comp()), std::max(range.end(), other.end(), Comp()));
	}
#else
#endif	// ASCENSION_ABANDONED_AT_VERSION_08

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

#ifdef ASCENSION_ABANDONED_AT_VERSION_08
	/**
	 * Writes the into the given output stream.
	 * @tparam T The element type of the range
	 * @tparam Comp The comparison type of the range
	 * @tparam CharType The character type of the output stream
	 * @tparam CharTraits The character traits type of the output stream
	 */
	template<typename T, typename Comp, typename CharType, typename CharTraits>
	inline std::basic_ostream<CharType, CharTraits>& operator<<(
			std::basic_ostream<CharType, CharTraits>& out, const Range<T, Comp>& range) {
		const std::ctype<CharType>& ct = std::use_facet<std::ctype<CharType>>(out.getloc());
		std::basic_ostringstream<CharType, CharTraits> s;
		s.flags(out.flags());
		s.imbue(out.getloc());
		s.precision(out.precision());
		s << ct.widen('[') << range.beginning() << ct.widen(',') << range.end() << ct.widen(')');
		return out << s.str().c_str();
	}
#endif	// ASCENSION_ABANDONED_AT_VERSION_08

} // namespace ascension

#endif // !ASCENSION_RANGE_HPP
