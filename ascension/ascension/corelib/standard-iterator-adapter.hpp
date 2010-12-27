/**
 * @file standard-iterator-adapter.hpp
 * @author exeal
 * @date 2010-10-21 separated from common.hpp
 * @date 2010-11-06 separated from basic-types.hpp
 */

#ifndef ASCENSION_STANDARD_ITERATOR_ADAPTER_HPP
#define ASCENSION_STANDARD_ITERATOR_ADAPTER_HPP
#include <iterator>

namespace ascension {

	template<typename ConcreteIterator, typename Reference>
	class StandardInputIteratorAdapterBase {
	public:
		/// Dereference operator.
		const Reference operator*() const {return concrete().current();}
		/// Dereference operator.
		const Reference operator->() const {return concrete().current();}
		/// Pre-fix increment operator.
		ConcreteIterator& operator++() {concrete().next(); return concrete();}
		/// Post-fix increment operator.
		const ConcreteIterator operator++(int) {
			ConcreteIterator temp(concrete());
			++*this;
			return temp;
		}
		/// Equality operator.
		bool operator==(const ConcreteIterator& other) const {
			return concrete().equals(other.concrete());
		}
		/// Inequality operator.
		bool operator!=(const ConcreteIterator& other) const {return !operator==(other);}
	protected:
		ConcreteIterator& concrete() /*throw()*/ {
			return static_cast<ConcreteIterator&>(*this);
		}
		const ConcreteIterator& concrete() const /*throw()*/ {
			return static_cast<const ConcreteIterator&>(*this);
		}
	};

	namespace detail {
		template<typename T> struct RemovePointer {typedef T Type;};
		template<typename T> struct RemovePointer<T*> {typedef T Type;};
		template<typename T> struct RemoveReference {typedef T Type;};
		template<typename T> struct RemoveReference<T&> {typedef T Type;};
	}

	template<typename ConcreteIterator, typename Reference>
	class StandardConstBidirectionalIteratorAdapterBase :
		public StandardInputIteratorAdapterBase<ConcreteIterator, Reference> {
	public:
		/// Pre-fix decrement operator.
		ConcreteIterator& operator--() {concrete().previous(); return concrete();}
		/// Post-fix decrement operator.
		const ConcreteIterator operator--(int) {
			ConcreteIterator temp(concrete());
			--*this;
			return temp;
		}
	};

	template<class ConcreteIterator>
	class StandardIteratorLessGreaterDecoratorBase {
	public:
		/// Relational operator.
		bool operator<(const ConcreteIterator& other) const {
			return static_cast<const ConcreteIterator*>(this)->less(rhs.concrete());
		}
		/// Relational operator.
		bool operator<=(const ConcreteIterator& other) const {
			return operator<(rhs) || operator==(rhs);
		}
		/// Relational operator.
		bool operator>(const ConcreteIterator& other) const {return !operator<=(rhs);}
		/// Relational operator.
		bool operator>=(const ConcreteIterator& other) const {return !operator<(rhs);}
	};

	template<typename ConcreteIterator, typename Reference, typename Distance = std::ptrdiff_t>
	class StandardInputIteratorAdapter :
		public StandardInputIteratorAdapterBase<ConcreteIterator, Reference>,
		public StandardIteratorLessGreaterDecoratorBase<ConcreteIterator>,
		public std::iterator<
			std::input_iterator_tag, typename detail::RemoveReference<Reference>::Type,
			Distance, const typename detail::RemoveReference<Reference>::Type*, const Reference> {};

	template<typename ConcreteIterator, typename Reference, typename Distance = std::ptrdiff_t>
	class StandardConstBidirectionalIteratorAdapter :
		public StandardConstBidirectionalIteratorAdapterBase<ConcreteIterator, Reference>,
		public StandardIteratorLessGreaterDecoratorBase<ConcreteIterator>,
		public std::iterator<
			std::bidirectional_iterator_tag, typename detail::RemoveReference<Reference>::Type,
			Distance, const typename detail::RemoveReference<Reference>::Type*, const Reference> {};

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
	 * Ascension basic bidirectional iterator classes don't have C++ standard iterator interface
	 * (operator overloadings). By using this class, you can write the following:
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

} // namespace ascension

#endif // !ASCENSION_STANDARD_ITERATOR_ADAPTER_HPP
