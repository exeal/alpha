/**
 * @file common.hpp
 * @brief Common header file used by Ascension library.
 * @author exeal
 * @date 2004-2008
 */

#ifndef ASCENSION_COMMON_HPP
#define ASCENSION_COMMON_HPP

/// Version of Ascension library
#define ASCENSION_LIBRARY_VERSION 0x0080	// 0.8.0

/// Version of Unicode we're tracking
#define ASCENSION_UNICODE_VERSION 0x0500	// 5.0.0

// platform
#ifdef _WIN32
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
#		define ASCENSION_FASTCALL __attribute__(regparm(3))
#	else
#		define ASCENSION_FASTCALL
#	endif
#else
#	define ASCENSION_FASTCALL
#endif // __i386__

#include "../../manah/object.hpp"
#include "config.hpp"
#include <string>	// std.string
#include <sstream>	// std.basic_stringbuf, std.basic_stringstream, ...
#include <iterator>
#include <stdexcept>

#if defined(ASCENSION_WINDOWS) && defined(_DEBUG)
#include "../../manah/win32/timer.hpp"
using manah::win32::Timer;
#endif // defined(ASCENSION_WINDOWS) && defined(_DEBUG)

namespace ascension {

	// shorten type names
	using manah::byte;
	using manah::uchar;
	using manah::ushort;
	using manah::uint;
	using manah::ulong;

	// character and string
	typedef wchar_t Char;					///< Type for characters as UTF-16 code unit.
	typedef std::basic_string<Char> String;	///< Type for strings as UTF-16.
	typedef std::size_t length_t;			///< Length of string or index.

	/// Invalid value of @c length_t.
	const length_t INVALID_INDEX = 0xFFFFFFFFUL;

	/// Unicode code point.
	typedef unsigned long CodePoint;	// uint32_t
	/// Code point of LINE FEED (U+000A).
	const Char LINE_FEED = 0x000A;
	/// Code point of CARRIAGE RETURN (U+000D).
	const Char CARRIAGE_RETURN = 0x000D;
	/// Code point of NEXT LINE (U+0085).
	const Char NEXT_LINE = 0x0085;
	/// Code point of SUBSTITUTE (U+001A).
	const Char C0_SUBSTITUTE = 0x001A;
	/// Code point of ZERO WIDTH NON-JOINER (U+200C).
	const Char ZERO_WIDTH_NON_JOINER = 0x200C;
	/// Code point of ZERO WIDTH JOINER (U+200D).
	const Char ZERO_WIDTH_JOINER = 0x200D;
	/// Code point of LINE SEPARATOR (U+2028).
	const Char LINE_SEPARATOR = 0x2028;
	/// Code point of PARAGRAPH SEPARATOR (U+2029).
	const Char PARAGRAPH_SEPARATOR = 0x2029;
	/// Code point of REPLACEMENT CHARACTER (U+FFFD).
	const Char REPLACEMENT_CHARACTER = 0xFFFD;
	/// Code point of non-character (U+FFFF).
	const Char NONCHARACTER = 0xFFFF;
	/// Invalid code point value.
	const CodePoint INVALID_CODE_POINT = 0xFFFFFFFFUL;
	/// Set of newline characters.
	/// @see kernel#Newline
	const Char NEWLINE_CHARACTERS[] = {LINE_FEED, CARRIAGE_RETURN, NEXT_LINE, LINE_SEPARATOR, PARAGRAPH_SEPARATOR};

	/**
	 * Represents direction in a text or a document (not visual orientation. See @c #viewers#Orientation).
	 * @see ascension#text, ascension#searcher
	 */
	enum Direction {
		FORWARD,	///< Direction to the end.
		BACKWARD	///< Direction to the start.
	};

	/// Negation operator for @c Direction value.
	inline Direction operator!(Direction v) throw() {return (v == FORWARD) ? BACKWARD : FORWARD;}

	/**
	 * Notifies about the system parameter changes.
	 * Clients of Ascension should call this function when the system settings are changed
	 * (for example, received @c WM_SETTINGCHANGE window message on Win32 platform).
	 */
	void updateSystemSettings() throw();

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
		ConcreteIterator& concrete() throw() {return static_cast<ConcreteIterator&>(*this);}
		const ConcreteIterator& concrete() const throw() {return static_cast<const ConcreteIterator&>(*this);}
	};

	namespace texteditor {	// see session.hpp
		class Session;
		namespace internal {
			class ISessionElement {
			protected:
				virtual void setSession(Session& session) throw() = 0;
				friend class Session;
			};
		}
	} // namespace texteditor

	// static assertion
	template<std::size_t> struct StaticAssertTest {};
	template<int> struct StaticAssertionFailureAtLine;
	template<> struct StaticAssertionFailureAtLine<-1> {};
	#define ASCENSION_STATIC_ASSERT(expression)	\
		typedef StaticAssertTest<sizeof(StaticAssertionFailureAtLine<(expression) ? -1 : __LINE__>)> oh_static_assertion_shippaidayo_orz

	// basic assertions
	ASCENSION_STATIC_ASSERT(sizeof(Char) == 2);
	ASCENSION_STATIC_ASSERT(sizeof(CodePoint) == 4);

} // namespace ascension

#ifdef ASCENSION_TEST
namespace std {
	inline ostream& operator<<(ostream& out, const ascension::String& value) {
		const char prefix[2] = {'\\', 'u'};
		char digits[5];
		for(ascension::String::const_iterator i(value.begin()), e(value.end()); i != e; ++i) {
			sprintf(digits, "%04x", *i);
			out.write(prefix, countof(prefix)).write(digits, 4);
		}
		return out;
	}
}
#endif // ASCENSION_TEST

#endif // !ASCENSION_COMMON_HPP
