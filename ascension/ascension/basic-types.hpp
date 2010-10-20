/**
 * @file basic-types.hpp
 * @author exeal
 * @date 2010-10-21 separated from common.hpp
 */

#ifndef ASCENSION_BASIC_TYPES_HPP
#define ASCENSION_BASIC_TYPES_HPP
#include "common.hpp"

namespace ascension {

	/**
	 * OR-combinations of enum values (from Qt.QFlags).
	 * @deprecated 0.8 Bad design idea.
	 */
	template<typename Enum> class Flags {
	public:
		Flags(Enum value) : value_(value) {}
		Flags(int value = 0) : value_(static_cast<Enum>(value)) {}
		Flags(const Flags<Enum>& rhs) /*throw()*/ : value_(rhs.value_) {}
		Flags<Enum>& operator=(const Flags<Enum>& rhs) /*throw()*/ {value_ = rhs.value_; return *this;}
		Flags<Enum> operator&(int rhs) const /*throw()*/ {Flags<Enum> temp(*this); return temp &= rhs;}
		Flags<Enum> operator&(uint rhs) const /*throw()*/ {Flags<Enum> temp(*this); return temp &= rhs;}
		Flags<Enum> operator|(Enum rhs) const {Flags<Enum> temp(*this); return temp |= rhs;}
		Flags<Enum> operator^(Enum rhs) const {Flags<Enum> temp(*this); return temp ^= rhs;}
		Flags<Enum>& operator&=(int rhs) /*throw()*/ {value_ &= rhs; return *this;}
		Flags<Enum>& operator&=(uint rhs) /*throw()*/ {value_ &= rhs; return *this;}
		Flags<Enum>& operator|=(Enum rhs) {value_ |= rhs; return *this;}
		Flags<Enum>& operator^=(Enum rhs) {value_ ^= rhs; return *this;}
		Flags<Enum>& operator~() const /*throw()*/ {return ~value_;}
		bool operator!() const /*throw()*/ {return value_ == 0;}
		operator Enum() const {return static_cast<Enum>(value_);}
		void clear() /*throw()*/ {value_ = 0;}
		bool has(Enum e) const {return (value_ & e) != 0;}
		Flags<Enum>& set(Enum e, bool value = true) {if(value) value_ |= e; else value_ &= ~e; return *this;}
	private:
		int value_;
	};

	/**
	 * Represents direction in a text or a document (not visual orientation. See
	 * @c #presentation#ReadingDirection).
	 * @see ascension#text, ascension#searcher
	 */
	class Direction {
	public:
		static const Direction FORWARD;		///< Direction to the end.
		static const Direction BACKWARD;	///< Direction to the start.
		/// Copy-constructor.
		Direction(const Direction& other) /*throw()*/ : value_(other.value_) {}
		/// Assignment operator.
		Direction& operator=(const Direction& other) /*throw()*/ {value_ = other.value_; return *this;}
		/// Negation operator returns the complement of this.
		Direction operator!() const /*throw()*/ {return (*this == FORWARD) ? BACKWARD : FORWARD;}
		/// Equality operator.
		bool operator==(const Direction& other) const /*throw()*/ {return value_ == other.value_;}
		/// Inequality operator.
		bool operator!=(const Direction& other) const /*throw()*/ {return !(*this == other);}
	private:
		explicit Direction(bool value) /*throw()*/ : value_(value) {}
		bool value_;
	};

	/// Pointer argument is @c null but that is not allowed.
	class NullPointerException : public std::invalid_argument {
	public:
		/// Constructor.
		explicit NullPointerException(const std::string& message) : std::invalid_argument(message) {}
		/// Destructor.
		~NullPointerException() throw() {}
	};

	/// The operation was performed in an illegal state.
	class IllegalStateException : public std::logic_error {
	public:
		/// Constructor.
		explicit IllegalStateException(const std::string& message) : std::logic_error(message) {}
	};

	/// The specified index was out of bounds.
	class IndexOutOfBoundsException : public std::out_of_range {
	public:
		/// Default constructor.
		IndexOutOfBoundsException() : std::out_of_range("the index is out of range.") {}
		/// Constructor.
		explicit IndexOutOfBoundsException(const std::string& message) : std::out_of_range(message) {}
	};

	/**
	 * The iterator has reached the end of the enumeration.
	 * @note Not all iterator classes defined in Ascension throw this exception.
	 */
	class NoSuchElementException : public std::runtime_error {
	public:
		/// Default constructor.
		NoSuchElementException() : std::runtime_error("the iterator is end.") {}
		/// Constructor takes an error message.
		explicit NoSuchElementException(const std::string& message) : std::runtime_error(message) {}
	};

	/// Specified value is invalid for enumeration or constant.
	class UnknownValueException : public std::invalid_argument {
	public:
		/// Constructor.
		explicit UnknownValueException(const std::string& message) : invalid_argument(message) {}
	};

	/**
	 * A platform-dependent error whose detail can be obtained by POSIX @c errno or Win32
	 * @c GetLastError.
	 * @tparam Base the base exception class
	 */
	template<typename Base = std::runtime_error>
	class PlatformDependentError : public Base {
	public:
#ifdef ASCENSION_WINDOWS
		typedef DWORD Code;
#else
		typedef int Code;
#endif
	public:
		/**
		 * Constructor.
		 * @param code the error code
		 */
		explicit PlatformDependentError(Code code
#ifdef ASCENSION_WINDOWS
			= ::GetLastError()
#else
			= errno
#endif
			) : Base("platform-dependent error occurred."), code_(code) {}
		/// Returns the error code.
		Code code() const /*throw()*/;
	private:
		const Code code_;
	};

	/**
	 * Represents an invariant range.
	 * @tparam T the element type
	 * @note This class is not compatible with Boost.Range.
	 * @see kernel#Region
	 */
	template<typename T> class Range : protected std::pair<T, T> {
	public:
		typedef T ValueType;
	public:
		/// Default constructor.
		Range() {}
		/// Constructor.
		Range(ValueType v1, ValueType v2) :
			std::pair<ValueType, ValueType>(std::min(v1, v2), std::max(v1, v2)) {}
		/// Returns the beginning (minimum) of the range.
		ValueType beginning() const {return std::pair<T, T>::first;}
		/// Returns the end (maximum) of the range.
		ValueType end() const {return std::pair<T, T>::second;}
		/// Returns @c true if the given value is included by this range.
		bool includes(ValueType v) const {return v >= beginning() && v < end();}
		/// Returns @c true if the given range is included by this range.
		template<typename U> bool includes(const Range<U>& other) const {
			return other.beginning() >= beginning() && other.end() <= end();}
		/// Returns @c true if the range is empty.
		bool isEmpty() const {return beginning() == end();}
		/// Returns the length of the range.
		/// @note This class does not define a method named "size".
		typename std::iterator_traits<T>::difference_type length() const {return end() - beginning();}
	};

	/// Returns a @c Range object using the given two values.
	template<typename T> inline Range<T> makeRange(T v1, T v2) {return Range<T>(v1, v2);}

	/**
	 * String-like object addresses a sized piece of memory.
	 * @tparam Character the character type
	 * @tparam CharacterTraits the character traits type gives @c length class method returns a
	 *                         length of a string
	 * @note Constructors do <strong>not</strong> check their parameters if are @c null.
	 */
	template<typename Character, typename CharacterTraits = std::char_traits<Character> >
	class BasicStringPiece : public Range<const Character*> {
	public:
		typedef Character value_type;
		typedef const value_type* const_pointer;
		typedef const value_type& const_reference;
		typedef std::basic_string<value_type> string_type;
		typedef typename string_type::size_type size_type;
		typedef CharacterTraits traits_type;
	public:
		/// Default constructor.
		BasicStringPiece() : Range<const Character*>(0, 0) {}
		/**
		 * Implicit constructor. The length of the string is calculated by using
		 * @c traits_type#length function.
		 * @param p a pointer addresses the beginning of the string
		 */
		BasicStringPiece(const_pointer p) : Range<const Character*>(p, (p != 0) ? p + traits_type::length(p) : 0) {}
		/**
		 * Constructor takes the beginning and the end of a string.
		 * @param first a pointer addresses the beginning of the string
		 * @param last a pointer addresses the end of the string
		 */
		BasicStringPiece(const_pointer first, const_pointer last) : Range<const Character*>(first, last) {}
		/**
		 * Constructor takes a pointer to the beginning of the string and the length.
		 * @param p a pointer addresses the beginning of the string
		 * @param n the length of the string
		 */
		BasicStringPiece(const_pointer p, size_type n) : Range<const Character*>(p, p + n) {}
		/**
		 * Implicit constructor takes a standard C++ string object.
		 * @param s the string object
		 */
		BasicStringPiece(const string_type& s) : Range<const Character*>(s.data(), s.data() + s.length()) {}
		/**
		 * Returns the character at the specified position in the string.
		 * @param i the index of the position of the character to get. if @a i is equal to or
		 *          greater than the length of the string
		 * @return the character
		 */
		value_type operator[](size_type i) const {beginning() + i;}
		/**
		 * Returns the character at the specified position in the string.
		 * @param i the index of the position of the character to get
		 * @return the character
		 * @throw std#out_of_range @a i is equal to or greater than the length of the string
		 */
		value_type at(size_type i) const {
			if(i >= length()) throw std::out_of_range("i"); return operator[](i);}
	};
	/// Specialization of @c BasicStringPiece for @c Char type.
	typedef BasicStringPiece<Char> StringPiece;

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
		const ConcreteIterator operator++(int) {ConcreteIterator temp(concrete()); ++*this; return temp;}
		/// Equality operator.
		bool operator==(const ConcreteIterator& other) const {return concrete().equals(other.concrete());}
		/// Inequality operator.
		bool operator!=(const ConcreteIterator& other) const {return !operator==(other);}
	protected:
		ConcreteIterator& concrete() /*throw()*/ {return static_cast<ConcreteIterator&>(*this);}
		const ConcreteIterator& concrete() const /*throw()*/ {return static_cast<const ConcreteIterator&>(*this);}
	};

	namespace internal {
		template<typename T> struct RemovePointer {typedef T Type;};
		template<typename T> struct RemovePointer<T*> {typedef T Type;};
		template<typename T> struct RemoveReference {typedef T Type;};
		template<typename T> struct RemoveReference<T&> {typedef T Type;};
	}

	template<typename ConcreteIterator, typename Reference>
	class StandardConstBidirectionalIteratorAdapterBase : public StandardInputIteratorAdapterBase<ConcreteIterator, Reference> {
	public:
		/// Pre-fix decrement operator.
		ConcreteIterator& operator--() {concrete().previous(); return concrete();}
		/// Post-fix decrement operator.
		const ConcreteIterator operator--(int) {ConcreteIterator temp(concrete()); --*this; return temp;}
	};

	template<class ConcreteIterator>
	class StandardIteratorLessGreaterDecoratorBase {
	public:
		/// Relational operator.
		bool operator<(const ConcreteIterator& other) const {return static_cast<const ConcreteIterator*>(this)->less(rhs.concrete());}
		/// Relational operator.
		bool operator<=(const ConcreteIterator& other) const {return operator<(rhs) || operator==(rhs);}
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
			std::input_iterator_tag, typename internal::RemoveReference<Reference>::Type,
			Distance, const typename internal::RemoveReference<Reference>::Type*, const Reference> {};

	template<typename ConcreteIterator, typename Reference, typename Distance = std::ptrdiff_t>
	class StandardConstBidirectionalIteratorAdapter :
		public StandardConstBidirectionalIteratorAdapterBase<ConcreteIterator, Reference>,
		public StandardIteratorLessGreaterDecoratorBase<ConcreteIterator>,
		public std::iterator<
			std::bidirectional_iterator_tag, typename internal::RemoveReference<Reference>::Type,
			Distance, const typename internal::RemoveReference<Reference>::Type*, const Reference> {};

	/**
	 * Converts an Ascension basic bidirectional iterator class into the corresponding C++
	 * standard-compliant one.
	 * @tparam ConcreteIterator the iterator class converted. see the next section
	 * @tparam Type the element type
	 * @tparam Reference the reference type
	 * @tparam Pointer the pointer type
	 * @tparam Distance the distance type
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

#endif // !ASCENSION_BASIC_TYPES_HPP
