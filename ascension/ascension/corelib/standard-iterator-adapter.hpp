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

		template<typename Derived, typename Base>
		struct InputOutputIteratorOperators : public Base {
			/// Dereference operator.
			typename std::iterator_traits<Base>::reference operator->() {
				return *static_cast<Derived&>(*this);
			}
			/// Dereference operator.
			const typename std::iterator_traits<Base>::reference operator->() const {
				return *static_cast<const Derived&>(*this);
			}
			/// Inequality operator.
			bool operator!=(const Derived& other) const {
				return static_cast<const Derived&>(*this) == other;
			}
			/// Post-fix increment operator.
			Derived& operator++(int) {
				Derived temp(static_cast<Derived&>(*this));
				++*this;
				return temp;
			}
		};

		template<typename Derived, typename Base>
		struct BidirectionalIteratorOperators : public Base {
			/// Post-fix decrement operator.
			Derived& operator--(int) {
				Derived temp(static_cast<Derived&>(*this));
				--*this;
				return temp;
			}
		};

		template<typename Derived, typename Base>
		struct LessGreaterOperators : public Base {
		public:
			/// Relational operator.
			bool operator<=(const Derived& other) const {
				return operator<(other) || operator==(other);
			}
			/// Relational operator.
			bool operator>(const Derived& other) const {return !operator<=(other);}
			/// Relational operator.
			bool operator>=(const Derived& other) const {return !operator<(other);}
		};

		template<typename Derived, typename Base>
		class InputOutputIteratorAdapterBase : public InputOutputIteratorOperators<Derived, Base> {
		public:
			/// Dereference operator.
			typename std::iterator_traits<Base>::reference operator*() {
				return derived().current();
			}
			/// Dereference operator.
			const typename std::iterator_traits<Base>::reference operator*() const {
				return derived().current();
			}
			/// Pre-fix increment operator.
			Derived& operator++() {
				derived().next();
				return derived();
			}
			/// Equality operator.
			bool operator==(const Derived& other) const {
				return derived().equals(other.derived());
			}
		protected:
			Derived& derived() /*throw()*/ {
				return static_cast<Derived&>(*this);
			}
			const Derived& derived() const /*throw()*/ {
				return static_cast<const Derived&>(*this);
			}
		};
/*
		template<typename Iterator>
		struct IsBidirectionalIterator {
			static const bool value =
				IsSame<
					typename std::iterator_traits<Iterator>::iterator_category,
					std::bidirectional_iterator_tag
				>::result || IsSame<
					typename std::iterator_traits<Iterator>::iterator_category,
					std::random_access_iterator_tag
				>::result;
		};
*/
		template<typename Derived, typename Base>
		class BidirectionalIteratorAdapterBase : public BidirectionalIteratorOperators<Derived, Base> {
		public:
			/// Pre-fix decrement operator.
			Derived& operator--() {
//				ASCENSION_STATIC_ASSERT((
//					IsBidirectionalIterator<BidirectionalIteratorAdapterBase<Derived, Base> >::value));
				static_cast<Derived*>(this)->previous();
				return *static_cast<Derived*>(this);
			}
		};

		template<typename Derived, typename Base>
		class IteratorLessGreaterDecoratorBase : public LessGreaterOperators<Derived, Base> {
		public:
			/// Relational operator.
			bool operator<(const Derived& other) const {
//				ASCENSION_STATIC_ASSERT((
//					IsBidirectionalIterator<IteratorLessGreaterDecoratorBase<Derived, Base> >::value));
				return static_cast<const Derived*>(this)->less(other.derived());
			}
		};
/*		
		template<typename T> struct RemovePointer {typedef T Type;};
		template<typename T> struct RemovePointer<T*> {typedef T Type;};
		template<typename T> struct RemoveReference {typedef T Type;};
		template<typename T> struct RemoveReference<T&> {typedef T Type;};
*/
		template<typename Derived, typename Base>
		class IteratorAdapter :
			public InputOutputIteratorAdapterBase<Derived, Base>,
			public BidirectionalIteratorAdapterBase<Derived, Base>,
			public IteratorLessGreaterDecoratorBase<Derived, Base> {};

		/**
		 * Converts an Ascension basic bidirectional iterator class into the corresponding C++
		 * standard-compliant one.
		 * @tparam ConcreteIterator The iterator class converted. see the next section
		 * @tparam Type The element type
		 * @tparam Reference The reference type
		 * @tparam Pointer The pointer type
		 * @tparam Distance The distance type
		 * @note This class is not intended to be subclassed.
		 * @see StringCharacterIterator, DocumentCharacterIterator
		 *
		 * Ascension basic bidirectional iterator classes don't have C++ standard iterator
		 * interface (operator overloadings). By using this class, you can write the following:
		 *
		 * @code
		 * // find the first LF in the document
		 * const Document& = ...;
		 * // DocumentCharacterIterator is derived from StandardBidirectionalIteratorAdapter
		 * std::find(
		 *   document.begin(), document.end(),
		 *   LINE_SEPARATOR  // DocumentCharacterIterator returns LINE_SEPARATOR at EOL
		 * );
		 * @endcode
		 *
		 * <h3>Concept -- @a ConcreteIterator requirements</h3>
		 *
		 * @a ConcreteIterator template parameter must satisfy some of the following requirements:
		 * <table border="1">
		 *   <tr><th>Expression</th><th>Return type</th><th>Assertion / Note / Pre- / Post-condition</th></tr>
		 *   <tr><td>i.current()</td><td>@a Type</td><td>Returns the current value for @c #operator= and @c #operator-&gt;.</td></tr>
		 *   <tr><td>i.next()</td><td>not used</td><td>Moves the iterator to the next position for @c #operator++ and @c #operator++(int).</td></tr>
		 *   <tr><td>i.previous()</td><td>not used</td><td>Moves the iterator to the previous position for @c #operator-- and @c #operator--(int).</td></tr>
		 *   <tr><td>i1.equals(i2)</td><td>bool</td><td>true if @c i1 equals @c i2. For @c #operator==, @c #operator!=, ...</td></tr>
		 *   <tr><td>i1.less(i2)</td><td>bool</td><td>true if @c i1 is less than @c i2. For @c #operator&lt;, @c #operator&gt;, ... This is not required if you don't use relation operators.</td></tr>
		 * </table>
		 */

	}
} // namespace ascension.detail

#endif // !ASCENSION_STANDARD_ITERATOR_ADAPTER_HPP
