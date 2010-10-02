/**
 * @file common.hpp
 * @brief Common header file used by Ascension library.
 * @author exeal
 * @date 2004-2010
 */

#ifndef ASCENSION_COMMON_HPP
#define ASCENSION_COMMON_HPP

/// Version of Ascension library
#define ASCENSION_LIBRARY_VERSION 0x0080	// 0.8.0

/// Version of Unicode we're tracking
#define ASCENSION_UNICODE_VERSION 0x0510	// 5.1.0

// platform
#if defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
#	define ASCENSION_WINDOWS
#	include <manah/win32/windows.hpp>
#else
#	define ASCENSION_POSIX
#endif

// compiler
#if defined(_MSC_VER)
#	define ASCENSION_MSVC
#elif defined(__GNUC__)
#	define ASCENSION_GCC
#endif

#ifdef __i386__
#	if defined(ASCENSION_MSVC)
#		define ASCENSION_FASTCALL __fastcall
#	elif defined(ASCENSION_GCC)
#		define ASCENSION_FASTCALL __attribute__((regparm(3)))
#	else
#		define ASCENSION_FASTCALL
#	endif
#else
#	define ASCENSION_FASTCALL
#endif // __i386__

#ifdef ASCENSION_WINDOWS
#	ifndef _GLIBCXX_USE_WCHAR_T
#		define _GLIBCXX_USE_WCHAR_T 1
#	endif
#	ifndef _GLIBCXX_USE_WSTRING
#		define _GLIBCXX_USE_WSTRING 1
#	endif
#endif // ASCENSION_WINDOWS

/**
 * @def ASCENSION_NOFAIL The method which has this tag guarentees "no-fail".
 * On debug releaese, this symbol is replaced with "throw()" empty exception-specification.
 * However, see "Exceptional C++ Style" Item 13 about guideline for exception-specifications.
 * @note Because some parsers (including MSVC Class View) confuse, this symbol is now not used.
 */
#ifdef _DEBUG
#	define ASCENSION_NOFAIL /*throw()*/
#else
#	define ASCENSION_NOFAIL
#endif // _DEBUG
/// @def ASC_NOFAIL Short version of @c ASCENSION_NOFAIL.
#define ASC_NOFAIL ASCENSION_NOFAIL

#include <manah/object.hpp>
#include "config.hpp"
#include <string>	// std.basic_string
#include <iterator>
#include <stdexcept>
#include <utility>	// std.pair, std.min, std.max
#include <new>		// std.bad_alloc, std::nothrow_t, ...
#include <cmath>	// std.abs(double)

#ifdef ASCENSION_CUSTOM_SHARED_PTR_HPP
#	include ASCENSION_CUSTOM_SHARED_PTR_HPP
#elif defined(ASCENSION_MSVC) && _MSC_VER >= 1500
#	include <memory>
#elif defined(ASCENSION_GCC) && __GNUC__ >= 4
#	include <tr1/memory>
#else
#	include <boost/tr1/memory.hpp>
#endif

#if !defined(ASCENSION_WINDOWS) || defined(__BORLANDC__) || defined(__MINGW32__)
#	include <cinttypes>
#	define ASCENSION_HAS_CINTTYPES
#endif

#if defined(ASCENSION_WINDOWS)
#	define ASCENSION_USE_INTRINSIC_WCHAR_T
#endif

#if defined(ASCENSION_WINDOWS) && defined(_DEBUG)
#include <manah/win32/timer.hpp>
using manah::win32::Timer;
#endif // defined(ASCENSION_WINDOWS) && defined(_DEBUG)

namespace ascension {

	// sized integer types
#if defined(ASCENSION_HAS_CINTTYPES)
	using std::int8_t;
	using std::int16_t;
	using std::int32_t;
	using std::int64_t;
	using std::uint8_t;
	using std::uint16_t;
	using std::uint32_t;
	using std::uint64_t;
#elif defined (ASCENSION_MSVC)
	typedef signed char int8_t;
	typedef short int16_t;
	typedef long int32_t;
	typedef __int64 int64_t;
	typedef unsigned char uint8_t;
	typedef unsigned short uint16_t;
	typedef unsigned int uint32_t;
	typedef unsigned __int64 uint64_t;
#else
#	error "Could not define sized integer types."
#endif

	// shorten type names
	using manah::byte;
	using manah::uchar;
	using manah::ushort;
	using manah::uint;
	using manah::ulong;

	// character and string
#ifdef ASCENSION_USE_INTRINSIC_WCHAR_T
	typedef wchar_t Char;					///< Type for characters as UTF-16 code unit.
#else
	typedef uint16_t;						///< Type for characters as UTF-16 code unit.
#endif
	typedef std::basic_string<Char> String;	///< Type for strings as UTF-16.
	typedef std::size_t length_t;			///< Length of string or index.

	/// Invalid value of @c length_t.
	const length_t INVALID_INDEX = 0xfffffffful;

	/// Unicode code point.
	typedef uint32_t CodePoint;
	/// Code point of LINE FEED (U+000A).
	const Char LINE_FEED = 0x000au;
	/// Code point of CARRIAGE RETURN (U+000D).
	const Char CARRIAGE_RETURN = 0x000du;
	/// Code point of NEXT LINE (U+0085).
	const Char NEXT_LINE = 0x0085u;
	/// Code point of SUBSTITUTE (U+001A).
	const Char C0_SUBSTITUTE = 0x001au;
	/// Code point of ZERO WIDTH NON-JOINER (U+200C).
	const Char ZERO_WIDTH_NON_JOINER = 0x200cu;
	/// Code point of ZERO WIDTH JOINER (U+200D).
	const Char ZERO_WIDTH_JOINER = 0x200du;
	/// Code point of LINE SEPARATOR (U+2028).
	const Char LINE_SEPARATOR = 0x2028u;
	/// Code point of PARAGRAPH SEPARATOR (U+2029).
	const Char PARAGRAPH_SEPARATOR = 0x2029u;
	/// Code point of REPLACEMENT CHARACTER (U+FFFD).
	const Char REPLACEMENT_CHARACTER = 0xfffdu;
	/// Code point of non-character (U+FFFF).
	const Char NONCHARACTER = 0xffffu;
	/// Invalid code point value.
	const CodePoint INVALID_CODE_POINT = 0xfffffffful;
	/// Set of newline characters.
	/// @see kernel#Newline
	const Char NEWLINE_CHARACTERS[] = {LINE_FEED, CARRIAGE_RETURN, NEXT_LINE, LINE_SEPARATOR, PARAGRAPH_SEPARATOR};

	/// Returns @c true if the given floating-point numbers are (approximately) equal.
	inline bool equals(double n1, double n2, double epsilon = 1.0e-5) {return std::abs(n1 - n2) <= epsilon;}

	/**
	 * Represents direction in a text or a document (not visual orientation. See @c #viewers#Orientation).
	 * @see ascension#text, ascension#searcher
	 */
	class Direction {
	public:
		static const Direction FORWARD;		///< Direction to the end.
		static const Direction BACKWARD;	///< Direction to the start.
		/// Copy-constructor.
		Direction(const Direction& rhs) /*throw()*/ : value_(rhs.value_) {}
		/// Assignment operator.
		Direction& operator=(const Direction& rhs) /*throw()*/ {value_ = rhs.value_; return *this;}
		/// Negation operator returns the complement of this.
		Direction operator!() const /*throw()*/ {return (*this == FORWARD) ? BACKWARD : FORWARD;}
		/// Equality operator.
		bool operator==(const Direction& rhs) const /*throw()*/ {return value_ == rhs.value_;}
		/// Inequality operator.
		bool operator!=(const Direction& rhs) const /*throw()*/ {return !(*this == rhs);}
	private:
		explicit Direction(bool value) /*throw()*/ : value_(value) {}
		bool value_;
	};

	/**
	 * Notifies about the system parameter changes.
	 * Clients of Ascension should call this function when the system settings are changed
	 * (for example, received @c WM_SETTINGCHANGE window message on Win32 platform).
	 */
	void updateSystemSettings() /*throw()*/;

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

	namespace texteditor {	// see session.hpp
		class Session;
		namespace internal {
			class ISessionElement {
			protected:
				virtual void setSession(Session& session) /*throw()*/ = 0;
				friend class Session;
			};
		}
	} // namespace texteditor

	// basic assertions
	MANAH_STATIC_ASSERT(sizeof(Char) == 2);
	MANAH_STATIC_ASSERT(sizeof(CodePoint) == 4);

	namespace kernel {
		namespace fileio {
			/**
			 * Character type for file names. This is equivalent to
			 * @c ASCENSION_FILE_NAME_CHARACTER_TYPE configuration symbol.
			 */
			typedef ASCENSION_FILE_NAME_CHARACTER_TYPE PathCharacter;
			/// String type for file names.
			typedef std::basic_string<PathCharacter> PathString;
		}
	}

} // namespace ascension

#ifdef ASCENSION_TEST
namespace std {
	inline ostream& operator<<(ostream& out, const ascension::String& value) {
		const char prefix[2] = {'\\', 'u'};
		char digits[5];
		for(ascension::String::const_iterator i(value.begin()), e(value.end()); i != e; ++i) {
			sprintf(digits, "%04x", *i);
			out.write(prefix, MANAH_COUNTOF(prefix)).write(digits, 4);
		}
		return out;
	}
}
#endif // ASCENSION_TEST

#endif // !ASCENSION_COMMON_HPP
