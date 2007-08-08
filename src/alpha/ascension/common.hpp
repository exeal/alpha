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
#include <string>	// std::string
#include <sstream>	// std::basic_stringbuf, std::basic_stringstream, ...
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

	/**
	 * @c BidirectionalIteratorFacade template class converts an iterator class into the
	 * corresponding C++ standard-compliant bidirectional iterator.
	 * @param Iterator the iterator class converted. see the next section
	 * @param Type the element type
	 * @param Distance the distance type
	 * @param Pointer the pointer type
	 * @param Reference the reference type
	 * @note This class is not intended to be subclassed.
	 *
	 * Iterator classes Ascension provides don't have C++ standard iterator interface (operator
	 * overloadings). By using this class, you can write the following:
	 *
	 * @code
	 * // find the first LF in the document
	 * const Document& = ...;
	 * std::find(
	 *   DocumentCharacterIterator(document, document.getStartPosition()),
	 *   DocumentCharacterIterator(document, document.getEndPosition()),
	 *   LINE_SEPARATOR  // DocumentCharacterIterator returns LINE_SEPARATOR at EOL
	 * );
	 * @endcode
	 *
	 * <h3>Concept -- @c Iterator requirements</h3>
	 * @c Iterator template parameter must satisfy some of the following requirements:
	 * <table border="1">
	 *   <tr><th>Expression</th><th>Return type</th><th>Assertion / Note / Pre- / Post-condition</th></tr>
	 *   <tr><td>Iterator i</td><td>@a Iterator</td><td>Default constructor for the default constructor.</td></tr>
	 *   <tr><td>Iterator i(i1)</td><td>@a Iterator</td><td>Copy constructor for the copy constructor.</td></tr>
	 *   <tr><td>i1 = i2</td><td>@a Iterator</td><td>Assignment operator for the assignment operator.</td></tr>
	 *   <tr><td>i.current()</td><td>@a Type</td><td>Returns the current value for @c #operator= and @c #operator-&gt;.</td></tr>
	 *   <tr><td>i.next()</td><td>not used</td><td>Moves the iterator to the next position for @c #operator++ and @c #operator++(int).</td></tr>
	 *   <tr><td>i.previous()</td><td>not used</td><td>Moves the iterator to the previous position for @c #operator-- and @c #operator--(int).</td></tr>
	 *   <tr><td>i1.equals(i2)</td><td>bool</td><td>true if @c i1 equals @c i2. For @c #operator==, @c #operator!=, ...</td></tr>
	 *   <tr><td>i1.less(i2)</td><td>bool</td><td>true if @c i1 is less than @c i2. For @c #operator&lt;, @c #operator&gt;, ... This is not required if you don't use relation operators.</td></tr>
	 * </table>
	 */
	template<class Iterator, typename Type, typename Distance = std::ptrdiff_t, typename Pointer = Type*, typename Reference = Type&>
	class BidirectionalIteratorFacade : public std::iterator<std::bidirectional_iterator_tag, Type, Distance, Pointer, Reference> {
	public:
		/// Default constructor.
		BidirectionalIteratorFacade() {}
		/// Constructor.
		BidirectionalIteratorFacade(const Iterator& base) : impl_(base) {}
		/// Copy constructor.
		BidirectionalIteratorFacade(const BidirectionalIteratorFacade& rhs) : impl_(rhs.base()) {}
		/// Assignment operator.
		BidirectionalIteratorFacade& operator=(const BidirectionalIteratorFacade& rhs) {impl_ = rhs.base();}
		/// Dereference operator.
		Reference operator*() const {return impl_.current();}
		/// Dereference operator.
		Reference operator->() const {return impl_.current();}
		/// Pre-fix increment operator.
		BidirectionalIteratorFacade& operator++() {impl_.next(); return *this;}
		/// Post-fix increment operator.
		const BidirectionalIteratorFacade operator++(int) {BidirectionalIteratorFacade temp(*this); ++*this; return temp;}
		/// Pre-fix decrement operator.
		BidirectionalIteratorFacade& operator--() {impl_.previous(); return *this;}
		/// Post-fix decrement operator.
		const BidirectionalIteratorFacade operator--(int) {BidirectionalIteratorFacade temp(*this); --*this; return temp;}
		/// Equality operator.
		bool operator==(const BidirectionalIteratorFacade& rhs) const {return impl_.equals(rhs.base());}
		/// Inequality operator.
		bool operator!=(const BidirectionalIteratorFacade& rhs) const {return !operator==(rhs);}
		/// Relational operator.
		bool operator<(const BidirectionalIteratorFacade& rhs) const {return impl_.less(rhs.base());}
		/// Relational operator.
		bool operator<=(const BidirectionalIteratorFacade& rhs) const {return operator<(rhs) || operator==(rhs);}
		/// Relational operator.
		bool operator>(const BidirectionalIteratorFacade& rhs) const {return !operator<=(rhs);}
		/// Relational operator.
		bool operator>=(const BidirectionalIteratorFacade& rhs) const {return !operator<(rhs);}
		/// Returns the base iterator.
		const Iterator& base() const throw() {return impl_;}
	private:
		Iterator impl_;
	};

	// static assertion
	template<std::size_t> struct StaticAssertTest {};
	template<int> struct StaticAssertionFailureAtLine;
	template<> struct StaticAssertionFailureAtLine<-1> {};
	#define ASCENSION_STATIC_ASSERT(expression)	\
		typedef StaticAssertTest<sizeof(StaticAssertionFailureAtLine<(expression) ? -1 : __LINE__>)> oh_static_assertion_shippaidayo_orz

} // namespace ascension

#endif /* !ASCENSION_COMMON_HPP */
