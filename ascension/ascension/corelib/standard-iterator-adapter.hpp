/**
 * @file standard-iterator-adapter.hpp
 * @author exeal
 * @date 2010-10-21 separated from common.hpp
 * @date 2010-11-06 separated from basic-types.hpp
 */

#ifndef ASCENSION_STANDARD_ITERATOR_ADAPTER_HPP
#define ASCENSION_STANDARD_ITERATOR_ADAPTER_HPP
#include <iterator>	// std.iterator, std.iterator_traits

namespace ascension {
	namespace detail {

		class IteratorCoreAccess {
		public:
			template<typename Adapter>
			static typename Adapter::reference dereference(const Adapter& adapter) {return adapter.current();}
			template<typename Adapter>
			static void increment(Adapter& adapter) {adapter.next();}
			template<typename Adapter>
			static void decrement(Adapter& adapter) {adapter.previous();}
			template<typename Adapter1, typename Adapter2>
			static bool equal(const Adapter1& lhs, const Adapter2& rhs) {return lhs.equals(rhs);}
			template<typename Adapter1, typename Adapter2>
			static bool less(const Adapter1& lhs, const Adapter2& rhs) {return lhs.less(rhs);}
			template<typename Adapter>
			static void advance(Adapter& adapter, typename Adapter::difference_type n) {adapter.advance(n);}
			template<typename Adapter1, typename Adapter2>
			static typename Adapter1::difference_type distanceTo(const Adapter1& from, const Adapter2& to) {return from.distanceTo(to);}
		};

		/**
		 * @c IteratorAdapter converts an Ascension-style iterator class into the corresponding C++
		 * standard-compliant one.
		 * @tparam Derived The iterator class to convert. See the next section
		 * @tparam Base @c std#iterator type defines the iterator category, the element, the
		 *              distance, the pointer and the reference types of the derived iterator
		 * @note This class is not intended to be subclassed.
		 * @see StringCharacterIterator, DocumentCharacterIterator
		 *
		 * Ascension-style iterator classes don't have C++ standard iterator interface (operator
		 * overloadings). By using this class, you can write the following:
		 *
		 * @code
		 * // find the first LF in the document
		 * const Document& = ...;
		 * // DocumentCharacterIterator is derived from detail.IteratorAdapter
		 * std::find(
		 *   document.begin(), document.end(),
		 *   LINE_SEPARATOR  // DocumentCharacterIterator returns LINE_SEPARATOR at EOL
		 * );
		 * @endcode
		 *
		 * <h3>Concept -- @a Derived type requirements</h3>
		 *
		 * @a Derived template parameter must satisfy some of the following requirements:
		 * <table border="1">
		 *   <tr>
		 *     <th>Expression</th>
		 *     <th>Return type</th>
		 *     <th>Assertion / Note / Pre- / Post-condition</th>
		 *   </tr>
		 *   <tr>
		 *     <td>i.current()</td>
		 *     <td>@c IteratorAdapter#reference</td>
		 *     <td>Returns the current value for @c #operator= and @c #operator-&gt;.</td>
		 *   </tr>
		 *   <tr>
		 *     <td>i.next()</td>
		 *     <td>not used</td>
		 *     <td>Moves the iterator to the next position for @c #operator++ and @c #operator++(int).</td>
		 *   </tr>
		 *   <tr>
		 *     <td>i.previous()</td>
		 *     <td>not used</td>
		 *     <td>Moves the iterator to the previous position for @c #operator-- and @c #operator--(int).</td>
		 *   </tr>
		 *   <tr>
		 *     <td>i1.equals(i2)</td>
		 *     <td>convertible to @c bool</td>
		 *     <td>true if @c i1 equals @c i2. For @c #operator==, @c #operator!=, ...</td>
		 *   </tr>
		 *   <tr>
		 *     <td>i1.less(i2)</td>
		 *     <td>convertible to @c bool</td>
		 *     <td>true if @a i1 is less than @a i2. For @c #operator&lt;, @c #operator&gt;, ... This is not required if you don't use relation operators.</td>
		 *   </tr>
		 *   <tr>
		 *     <td>i.advance(n)</td>
		 *     <td>not used</td>
		 *     <td>Advances the iterator by @a n position.</td>
		 *   </tr>
		 *   <tr>
		 *     <td>i1.distanceTo(i2)</td>
		 *     <td>convertible to @c IteratorAdapter#difference_type</td>
		 *     <td>Returns the distance from @a i1 to @a i2.</td>
		 *   </tr>
		 * </table>
		 */
		template<typename Derived, typename Base>
		class IteratorAdapter : public Base {
		public:
			typedef typename std::iterator_traits<Base>::iterator_category iterator_category;
			typedef typename std::iterator_traits<Base>::value_type value_type;
			typedef typename std::iterator_traits<Base>::difference_type difference_type;
			typedef typename std::iterator_traits<Base>::distance_type distance_type;
			typedef typename std::iterator_traits<Base>::pointer pointer;
			typedef typename std::iterator_traits<Base>::reference reference;
		public:
			// input/output iterator
			/// Dereference operator.
			reference operator*() {
				return IteratorCoreAccess::dereference(derived());
			}
			/// Dereference operator.
			const reference operator*() const {
				return IteratorCoreAccess::dereference(derived());
			}
			/// Dereference operator.
			typename pointer operator->() {
				return &*static_cast<Derived&>(*this);
			}
			/// Dereference operator.
			const pointer operator->() const {
				return &*static_cast<const Derived&>(*this);
			}
			/// Equality operator.
			bool operator==(const IteratorAdapter<Derived, Base>& other) const {
				return IteratorCoreAccess::equal(derived(), other.derived());
			}
			/// Inequality operator.
			bool operator!=(const IteratorAdapter<Derived, Base>& other) const {
				return !(*this == other);
			}
			/// Pre-fix increment operator.
			Derived& operator++() {
				IteratorCoreAccess::increment(derived());
				return derived();
			}
			/// Post-fix increment operator.
			Derived& operator++(int) {
				Derived temp(derived());
				++*this;
				return temp;
			}
			// bidirectional iterator
			/// Pre-fix decrement operator.
			Derived& operator--() {
//				ASCENSION_STATIC_ASSERT((
//					IsBidirectionalIterator<BidirectionalIteratorAdapterBase<Derived, Base> >::value));
				IteratorCoreAccess::decrement(derived());
				return derived();
			}
			/// Post-fix decrement operator.
			Derived& operator--(int) {
				Derived temp(derived());
				--*this;
				return temp;
			}
			// random access iterator
			/// Bracket operator.
			const reference operator[](difference_type index) const {
				return *(*this + n);
			}
			/// Compound-add operator.
			Derived& operator+=(difference_type n) {
				IteratorCoreAccess::advance(derived(), n);
				return derived();
			}
			/// Compound-subtract operator.
			Derived& operator-=(difference_type n) {
				IteratorCoreAccess::advance(derived(), -n);
				return derived();
			}
			/// Binary-add operator.
			Derived operator+(difference_type n) {
				Derived temp(derived());
				return temp += n;
			}
			/// Binary-subtract operator.
			Derived operator-(difference_type n) {
				Derived temp(derived());
				return temp -= n;
			}
			/// Relational operator.
			bool operator<(const IteratorAdapter<Derived, Base>& other) const {
//				ASCENSION_STATIC_ASSERT((
//					IsBidirectionalIterator<IteratorLessGreaterDecoratorBase<Derived, Base> >::value));
				return IteratorCoreAccess::less(derived(), other.derived());
			}
			/// Relational operator.
			bool operator<=(const IteratorAdapter<Derived, Base>& other) const {
				return operator<(other) || operator==(other);
			}
			/// Relational operator.
			bool operator>(const IteratorAdapter<Derived, Base>& other) const {return !operator<=(other);}
			/// Relational operator.
			bool operator>=(const IteratorAdapter<Derived, Base>& other) const {return !operator<(other);}
		protected:
			bool less(const IteratorAdapter<Derived, Base>& other) const {
				return IteratorCoreAccess::distanceTo(other) > 0;
			}
		private:
			Derived& derived() /*throw()*/ {return static_cast<Derived&>(*this);}
			const Derived& derived() const /*throw()*/ {return static_cast<const Derived&>(*this);}
		};

	}
} // namespace ascension.detail

#endif // !ASCENSION_STANDARD_ITERATOR_ADAPTER_HPP
