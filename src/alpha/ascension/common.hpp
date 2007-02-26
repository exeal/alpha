/**
 *	@file common.hpp
 *	@brief Common header file used by Ascension library.
 *	@author exeal
 *	@date 2004-2007
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
using manah::windows::dout;
using manah::windows::Timer;
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
	 * Base class for bidirectional iterator. This provides C++ standard iterator interface.
	 * @param ConcreteIterator the derived iterator class. this class should have the following methods:
	 * - copy constructor
	 * - dereference(void) const : returns the addressed element of type @a Type
	 * - increment(void) : increments the position of the iterator
	 * - decrement(void) : decrements the position of the iterator
	 * - equals(const ConcreteIterator&amp;) const : return true if the iterator is equal to the other
	 * in addition, the iterator supports relational operations should have:
	 * - isLessThan(const ConcreteIterator&amp;) const : return true if the iterator is less than the other
	 * @param Type the element type
	 * @param Distance the distance type
	 * @param Pointer the pointer type
	 * @param Reference the reference type
	 */
	template<class ConcreteIterator, typename Type, typename Distance = std::ptrdiff_t, typename Pointer = Type*, typename Reference = Type&>
	class BidirectionalIteratorFacade : public std::iterator<std::bidirectional_iterator_tag, Type, Distance, Pointer, Reference> {
	public:
		/// Dereference operator returns a copy of the current element, not a reference.
		value_type operator*() const {return getConcrete().dereference();}
		/// Dereference operator.
		reference operator->() const {return getConcrete().dereference();}
		/// Pre-fix increment operator.
		ConcreteIterator& operator++() {getConcrete().increment(); return getConcrete();}
		/// Post-fix increment operator.
		ConcreteIterator operator++(int) {ConcreteIterator temp(getConcrete()); ++*this; return temp;}
		/// Pre-fix decrement operator.
		ConcreteIterator& operator--() {getConcrete().decrement(); return getConcrete();}
		/// Post-fix decrement operator.
		ConcreteIterator operator--(int) {ConcreteIterator temp(getConcrete()); --*this; return temp;}
		/// Equality operator.
		bool operator==(const ConcreteIterator& rhs) const {return getConcrete().equals(rhs);}
		/// Inequality operator.
		bool operator!=(const ConcreteIterator& rhs) const {return !operator==(rhs);}
		/// Relational operator.
		bool operator<(const ConcreteIterator& rhs) const {return getConcrete().isLessThan(rhs);}
		/// Relational operator.
		bool operator<=(const ConcreteIterator& rhs) const {return operator<(rhs) || operator==(rhs);}
		/// Relational operator.
		bool operator>(const ConcreteIterator& rhs) const {return !operator<=(rhs);}
		/// Relational operator.
		bool operator>=(const ConcreteIterator& rhs) const {return !operator<(rhs);}
	protected:
		typedef BidirectionalIteratorFacade<ConcreteIterator, Type, Distance, Pointer, Reference> Facade;
	private:
		ConcreteIterator& getConcrete() throw() {return *static_cast<ConcreteIterator*>(this);}
		const ConcreteIterator& getConcrete() const throw() {return *static_cast<const ConcreteIterator*>(this);}
	};

} // namespace ascension

#endif /* !ASCENSION_COMMON_HPP */
