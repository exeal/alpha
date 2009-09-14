/**
 * @file common.hpp
 * @brief Common header file used by Ascension library.
 * @author exeal
 * @date 2004-2009
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
#include <new>
#include <string>	// std.basic_string
#include <sstream>	// std.basic_stringbuf, std.basic_stringstream, ...
#include <iterator>
#include <stdexcept>

#if !defined(ASCENSION_WINDOWS) || defined(__BORLANDC__) || defined(__MINGW32__)
#	include <cinttypes>
#	define ASCENSION_HAS_CINTTYPES
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
#ifdef ASCENSION_WINDOWS
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

	/// Represents an invariant range.
	/// @note This class is not compatible with Boost.Range.
	/// @see kernel#Region
	template<typename T> class Range : protected std::pair<T, T> {
	public:
		typedef T ValueType;
	public:
		/// Constructor.
		Range(ValueType v1, ValueType v2) : std::pair<ValueType, ValueType>(std::min(v1, v2), std::max(v1, v2)) {}
		/// Returns the beginning (minimum) of the range.
		ValueType beginning() const {return std::pair<T, T>::first;}
		/// Returns the end (maximum) of the range.
		ValueType end() const {return std::pair<T, T>::second;}
		/// Returns the given value is included by the range.
		bool includes(ValueType v) const {return v >= std::pair<T, T>::first && v < std::pair<T, T>::second;}
		/// Returns true if the range is empty.
		bool isEmpty() const {return std::pair<T, T>::first == std::pair<T, T>::second;}
	};

	/**
	 * Converts an Ascension basic bidirectional iterator class into the corresponding C++
	 * standard-compliant one.
	 * @param ConcreteIterator the iterator class converted. see the next section
	 * @param Type the element type
	 * @param Reference the reference type
	 * @param Pointer the pointer type
	 * @param Distance the distance type
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
	 *   DocumentCharacterIterator(document, document.getStartPosition()),
	 *   DocumentCharacterIterator(document, document.getEndPosition()),
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
	template<class ConcreteIterator, typename Type, typename Reference = Type&, typename Pointer = Type*, typename Distance = std::ptrdiff_t>
	class StandardBidirectionalIteratorAdapter : public std::iterator<std::bidirectional_iterator_tag, Type, Distance, Pointer, Reference> {
	public:
		/// Dereference operator.
		Reference operator*() const {return concrete().current();}
		/// Dereference operator.
		Reference operator->() const {return concrete().current();}
		/// Pre-fix increment operator.
		ConcreteIterator& operator++() {concrete().next(); return concrete();}
		/// Post-fix increment operator.
		const ConcreteIterator operator++(int) {ConcreteIterator temp(concrete()); ++*this; return temp;}
		/// Pre-fix decrement operator.
		ConcreteIterator& operator--() {concrete().previous(); return concrete();}
		/// Post-fix decrement operator.
		const ConcreteIterator operator--(int) {ConcreteIterator temp(concrete()); --*this; return temp;}
		/// Equality operator.
		bool operator==(const ConcreteIterator& rhs) const {return concrete().equals(rhs.concrete());}
		/// Inequality operator.
		bool operator!=(const ConcreteIterator& rhs) const {return !operator==(rhs);}
		/// Relational operator.
		bool operator<(const ConcreteIterator& rhs) const {return concrete().less(rhs.concrete());}
		/// Relational operator.
		bool operator<=(const ConcreteIterator& rhs) const {return operator<(rhs) || operator==(rhs);}
		/// Relational operator.
		bool operator>(const ConcreteIterator& rhs) const {return !operator<=(rhs);}
		/// Relational operator.
		bool operator>=(const ConcreteIterator& rhs) const {return !operator<(rhs);}
	private:
		ConcreteIterator& concrete() /*throw()*/ {return static_cast<ConcreteIterator&>(*this);}
		const ConcreteIterator& concrete() const /*throw()*/ {return static_cast<const ConcreteIterator&>(*this);}
	};

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
