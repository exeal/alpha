/**
 * @file range.hpp
 * @author exeal
 * @date 2004-2010 (was basic-types.hpp)
 * @date 2011-03-25 separated from basic-types.hpp
 */

#ifndef ASCENSION_RANGE_HPP
#define ASCENSION_RANGE_HPP
#include <iterator>	// std.iterator_traits
#include <utility>	// std.max, std.min, std.pair
#include <ascension/corelib/type-traits.hpp>

namespace ascension {

	namespace detail {
		template<typename T>
		class HasDifferenceType {
			typedef char True;
			typedef struct {char temp[2];} False;
			template<typename U> static True test(typename U::difference_type*);
			template<typename U> static False test(...);
		public:
			static const bool value = sizeof(test<std::iterator_traits<T> >(0)) == sizeof(True);
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
		struct DifferenceType : ArithmeticOrNot<T, std::tr1::is_arithmetic<T>::value> {};
	}

	/**
	 * Represents an invariant range.
	 * @tparam T The element type. This type shall be @c LessThanComparable
	 * @tparam Comp The type for comparisons
	 * @note This class is not compatible with Boost.Range.
	 * @see BasicStringPiece, kernel#Region
	 */
	template<typename T, typename Comp = std::less<T> > class Range : protected std::pair<T, T> {
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
		/// Returns the beginning (minimum) of the range.
		value_type beginning() const {return std::pair<T, T>::first;}
		/// Returns the end (maximum) of the range.
		value_type end() const {return std::pair<T, T>::second;}
		/**
		 * Returns @c true if the given value is included by this range.
		 * @tparam U The type of @a v
		 * @param v The value to test
		 * @return true if @a v is included by this range
		 */
		template<typename U>
		bool includes(const U& v) const {
			const Comp lessThan;
			return !lessThan(beginning(), v) && lessThan(v, end());	// beginning() >= v && v < end
		}
		/**
		 * Returns @c true if the given range is included by this range.
		 * @tparam The type of @a other
		 * @param other The other range to test
		 * @return true if @a other is included by this range
		 */
		template<typename Other> bool includes(const Range<Other, Comp>& other) const {
			const Comp lessThan;
			return !lessThan(other.beginning(), beginning()) && !lessThan(end(), other.end());	// other.beginning() >= beginning() && other.end() <= end()
		}
		/**
		 * Returns the intersection of this range and the given one.
		 * @tparam Other The type of @a other
		 * @param other The other range
		 * @return The intersection or empty range
		 */
		template<typename Other>
		Range<value_type> intersected(const Range<Other, Comp>& other) const {
			const Comp lessThan;
			const value_type b(std::max<value_type, Comp>(beginning(), other.beginning(), lessThan));
			const value_type e(std::min<value_type, Comp>(end(), other.end(), lessThan));
			return Range<value_type>(b, std::max(b, e, lessThan));
		}
		/**
		 * Returns @c true if this range intersects with the given one.
		 * @tparam Other The type of @a other
		 * @param other The other range to test
		 * @return true if this range intersects with @a other
		 */
		template<typename Other>
		bool intersects(const Range<Other, Comp>& other) const {return !intersected(other).isEmpty();}
		/// Returns @c true if the range is empty.
		bool isEmpty() const {return internalIsEmpty<Comp>();}
		/**
		 * Returns the length of the range.
		 * @note This class does not define a method named "size".
		 */
		typename detail::DifferenceType<value_type>::Type length() const {
			return end() - beginning();
		}
		/**
		 * Returns the union of this range and the given one.
		 * @tparam Other The type of @a other
		 * @param other The other range
		 * @return The union or empty range
		 */
		template<typename Other> Range<value_type> united(const Range<Other, Comp>& other) const {
			if(other.isEmpty())
				return *this;
			else if(isEmpty())
				return other;
			return Range<value_type, Comp>(
				std::min(beginning(), other.beginning(), Comp()), std::max(end(), other.end(), Comp()));
		}
		private:
			template<typename X>
			bool internalIsEmpty(typename std::tr1::enable_if<std::tr1::is_same<X, std::less<T> >::value>::type* = 0) const {
				return beginning() == end();
			}
			template<typename X>
			bool internalIsEmpty(typename std::tr1::enable_if<!std::tr1::is_same<X, std::less<T> >::value>::type* = 0) const {
				const Comp lessThan;
				return !lessThan(beginning(), end()) && !lessThan(end(), beginning());
			}
	};

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

} // namespace ascension

#endif // !ASCENSION_RANGE_HPP
