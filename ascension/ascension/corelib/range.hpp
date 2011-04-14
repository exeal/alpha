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

		template<typename T, bool b>
		struct DifferenceTypeBase;
		template<typename T>
		struct DifferenceTypeBase<T, true> {
			typedef typename std::iterator_traits<T>::difference_type Type;
		};
		template<typename T>
		struct DifferenceTypeBase<T, false> {
			typedef T Type;
		};

		template<typename T>
		struct DifferenceType {
			typedef typename
				DifferenceTypeBase<T, HasDifferenceType<T>::value>::Type Type;
		};
	}

	/**
	 * Represents an invariant range.
	 * @tparam T The element type
	 * @note This class is not compatible with Boost.Range.
	 * @see BasicStringPiece, kernel#Region, graphics#Rectangle
	 */
	template<typename T> class Range : protected std::pair<T, T> {
	public:
		typedef T value_type;
	public:
		/// Default constructor.
		Range() {}
		/**
		 * Constructor takes the beginning and the end of the range.
		 * @param v1, v2 The beginning and the end. Both std::min(v1, v2) and
		 *               std::max(v1, v2) should be valid
		 */
		Range(value_type v1, value_type v2) :
//			std::pair<value_type, value_type>(std::minmax(v1, v2)) {}
			std::pair<value_type, value_type>(std::min(v1, v2), std::max(v1, v2)) {}
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
		bool includes(const U& v) const {return v >= beginning() && v < end();}
		/**
		 * Returns @c true if the given range is included by this range.
		 * @tparam The type of @a other
		 * @param other The other range to test
		 * @return true if @a other is included by this range
		 */
		template<typename Other> bool includes(const Range<Other>& other) const {
			return other.beginning() >= beginning() && other.end() <= end();
		}
		/**
		 * Returns the intersection of this range and the given one.
		 * @tparam Other The type of @a other
		 * @param other The other range
		 * @return The intersection or empty range
		 */
		template<typename Other>
		Range<value_type> intersected(const Range<Other>& other) const {
			const value_type b(std::max<value_type>(beginning(), other.beginning()));
			const value_type e(std::min<value_type>(end(), other.end()));
			return Range<value_type>(b, std::max(b, e));
		}
		/**
		 * Returns @c true if this range intersects with the given one.
		 * @tparam Other The type of @a other
		 * @param other The other range to test
		 * @return true if this range intersects with @a other
		 */
		template<typename Other>
		bool intersects(const Range<Other>& other) const {return !intersected(other).isEmpty();}
		/// Returns @c true if the range is empty.
		bool isEmpty() const {return beginning() == end();}
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
		template<typename Other> Range<value_type> united(const Range<Other>& other) const {
			return Range<value_type>(
				std::min(beginning(), other.beginning()), std::max(end(), other.end()));
		}
	};

	/// Returns a @c Range object using the given two values.
	template<typename T> inline Range<T> makeRange(T v1, T v2) {return Range<T>(v1, v2);}

} // namespace ascension

#endif // !ASCENSION_RANGE_HPP
