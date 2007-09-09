/**
 * @file common.hpp
 * @brief Common header file used by Ascension library.
 * @author exeal
 * @date 2004-2007
 */

#ifndef ASCENSION_COMMON_HPP
#define ASCENSION_COMMON_HPP

/// Version of Ascension library
#define ASCENSION_LIBRARY_VERSION 0x0080	// 0.8.0

/// Version of Unicode we're tracking
#define ASCENSION_UNICODE_VERSION 0x0500	// 5.0.0

#include "config.hpp"
#include <string>	// std.string
#include <sstream>	// std.basic_stringbuf, std.basic_stringstream, ...
#include <iterator>

#ifdef _DEBUG
#include "../../manah/win32/timer.hpp"
using manah::win32::Timer;
#endif /* _DEBUG */

namespace ascension {

	// character and string
	typedef wchar_t					Char;		///< Type for characters as UTF-16 code unit.
	typedef std::basic_string<Char>	String;		///< Type for strings as UTF-16.
	typedef std::size_t				length_t;	///< Length of string or index.
	typedef std::basic_stringbuf<String::value_type>		StringBuffer;		///< String buffer.
	typedef std::basic_stringstream<String::value_type>		StringStream;		///< String stream.
	typedef std::basic_istringstream<String::value_type>	InputStringStream;	///< Input string stream.
	typedef std::basic_ostringstream<String::value_type>	OutputStringStream;	///< Output string stream.
	typedef std::basic_istream<Char>	InputStream;	///< Abstract input stream.
	typedef std::basic_ostream<Char>	OutputStream;	///< Abstract output stream.
	typedef std::ostream_iterator<Char>	OutputStreamIterator;	///< Output stream iterator.

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
	/// Set of line break characters.
	const Char LINE_BREAK_CHARACTERS[] = {LINE_FEED, CARRIAGE_RETURN, NEXT_LINE, LINE_SEPARATOR, PARAGRAPH_SEPARATOR};

	/**
	 * Represents direction in a text or a document (not visual orientation. See @c #viewers#Orientation).
	 * @see ascension#text, ascension#searcher
	 */
	enum Direction {
		FORWARD,	///< Direction to the end.
		BACKWARD	///< Direction to the start.
	};

	/**
	 * Notifies about the system parameter changes.
	 * Clients of Ascension should call this function when the system settings are changed
	 * (for example, received @c WM_SETTINGCHANGE window message on Win32 platform).
	 */
	void updateSystemSettings() throw();

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

	// static assertion
	template<std::size_t> struct StaticAssertTest {};
	template<int> struct StaticAssertionFailureAtLine;
	template<> struct StaticAssertionFailureAtLine<-1> {};
	#define ASCENSION_STATIC_ASSERT(expression)	\
		typedef StaticAssertTest<sizeof(StaticAssertionFailureAtLine<(expression) ? -1 : __LINE__>)> oh_static_assertion_shippaidayo_orz

	// basic assertions
	ASCENSION_STATIC_ASSERT(sizeof(Char) == 2);

} // namespace ascension

#endif /* !ASCENSION_COMMON_HPP */
