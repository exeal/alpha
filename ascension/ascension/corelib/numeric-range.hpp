/**
 * @file numeric-range.hpp
 * Defines @c NumericRange class template.
 * @author exeal
 * @date 2015-03-10 Created.
 * @see range.hpp
 */

#ifndef ASCENSION_NUMERIC_RANGE_HPP
#define ASCENSION_NUMERIC_RANGE_HPP
#include <boost/iterator/iterator_facade.hpp>
#include <boost/operators.hpp>
#include <boost/range/iterator_range.hpp>
//#include <cassert>
#include <type_traits>
#include <utility>

namespace ascension {
	namespace detail {
		template<typename Value, typename DifferenceType = Value>
		class NumericIterator : public boost::iterator_facade<
			NumericIterator<Value>, Value, boost::random_access_traversal_tag, Value&, DifferenceType
		> {
			typedef boost::iterator_facade<
				NumericIterator<Value>, Value, boost::random_access_traversal_tag, Value&, DifferenceType
			> Super;

		public:
			typedef typename Super::value_type value_type;
			typedef typename Super::difference_type difference_type;
			typedef typename Super::reference reference;
			typedef std::random_access_iterator_tag iterator_category;
		public:
			NumericIterator() {}
			explicit NumericIterator(value_type value) : value_(value) {}

		private:
			void advance(difference_type delta) {value_ += delta;}
			void decrement() {--value_;}
			reference dereference() const {return const_cast<NumericIterator*>(this)->value_;}
			difference_type distance_to(const NumericIterator& other) const {
				if(std::is_signed<value_type>::value)
					return other.value_ - value_;
				return (other.value_ < value_) ?
					-static_cast<difference_type>(value_ - other.value_) : static_cast<difference_type>(other.value_ - value_);
			}
			bool equal(const NumericIterator& other) const {return value_ == other.value_;}
			void increment() {++value_;}
		private:
			friend class boost::iterator_core_access;
			value_type value_;
		};
	}

	/// Return type of @a nrange function template.
	template<typename Value>
	class NumericRange : public boost::iterator_range<detail::NumericIterator<Value>>, private boost::equality_comparable<NumericRange<Value>> {
		typedef detail::NumericIterator<Value> Iterator;
		typedef boost::iterator_range<Iterator> Super;

	public:
		typedef difference_type size_type;	// override to handle floating-point numbers
	public:
		NumericRange() {}
		NumericRange(Value first, Value last) : Super(Iterator(first), Iterator(last)) {}
		template<typename RandomAccessTraversalRange>
		NumericRange(const RandomAccessTraversalRange& otherRange) :
			Super(Iterator(boost::const_begin(otherRange)), Iterator(boost::const_end(otherRange))) {}
		bool operator==(const NumericRange& other) const {
			return boost::equal(*this, other);
		}
		size_type size() const {
			return *boost::const_end(*this) - *boost::const_begin(*this);
		}
	};

	/**
	 * Writes a @c NumericRange into the given output stream.
	 * @tparam Value The numeric type
	 * @tparam CharType The character type of the output stream
	 * @tparam CharTraits The character traits type of the output stream
	 */
	template<typename Value, typename CharType, typename CharTraits>
	inline std::basic_ostream<CharType, CharTraits>& operator<<(
			std::basic_ostream<CharType, CharTraits>& out, const NumericRange<Value>& range) {
		const std::ctype<CharType>& ct = std::use_facet<std::ctype<CharType>>(out.getloc());
		std::basic_ostringstream<CharType, CharTraits> s;
		s.flags(out.flags());
		s.imbue(out.getloc());
		s.precision(out.precision());
		s << ct.widen('[') << *boost::const_begin(range) << ct.widen(',') << *boost::const_end(range) << ct.widen(')');
		return out << s.str().c_str();
	}

	template<typename Value>
	inline bool includes(const NumericRange<Value>& range, typename NumericRange<Value>::value_type value) {
		return value >= *boost::const_begin(range) && value < *boost::const_end(range);
	}

	template<typename Value, typename BinaryPredicate>
	inline bool includes(const NumericRange<Value>& range, typename NumericRange<Value>::value_type value, BinaryPredicate pred) {
		return !pred(value, *boost::const_begin(range)) && pred(value, *boost::const_end(range));
	}

	/**
	 * Generates a Numeric Range (half-open range).
	 * @tparam Value The type of @a first and @a last
	 * @param first The first value of the range
	 * @param last The last value of the range
	 * @return A Numeric Range
	 * @note This is designed based on @c boost#irange function template.
	 */
	template<typename Value>
	inline NumericRange<Value> nrange(Value first, Value last) {
//		assert(first <= last);
		return NumericRange<Value>(first, last);
	}

	/**
	 * Generates an ordered @c NumericRange.
	 * @tparam Value The numeric type
	 * @param range The original range
	 * @return The ordered range
	 */
	template<typename Value>
	inline NumericRange<Value> ordered(const NumericRange<Value>& range) {
		return NumericRange<Value>(std::minmax(*boost::const_begin(range), *boost::const_end(range)));
	}
}

#endif // !ASCENSION_NUMERIC_RANGE_HPP
