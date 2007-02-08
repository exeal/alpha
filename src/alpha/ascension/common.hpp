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
#include <string>
#include <sstream>
#include <memory>
#include <iterator>

#ifdef _DEBUG
#include "../../manah/win32/timer.hpp"
using manah::windows::dout;
using manah::windows::Timer;
#endif /* _DEBUG */

namespace ascension {

	// character and string
	typedef wchar_t					Char;		///< Type for characters as UTF-16 code unit
	typedef std::basic_string<Char>	String;		///< Type for strings as UTF-16
	typedef std::size_t				length_t;	///< Length of string or index
	typedef std::basic_stringstream<String::value_type>		StringStream;		///< String stream
	typedef std::basic_istringstream<String::value_type>	InputStringStream;	///< Input string stream
	typedef std::basic_ostringstream<String::value_type>	OutputStringStream;	///< Output string stream
	typedef std::basic_istream<Char>	InputStream;	///< input stream
	typedef std::basic_ostream<Char>	OutputStream;	///< output stream

	/// Invalid value of @c length_t.
	const length_t INVALID_INDEX = -1;

	/// Unicode code point.
	typedef unsigned long CodePoint;	// uint32_t
	/// Code point of LINE FEED (U+000A).
	const Char LINE_FEED = 0x000A;
	/// Code point of CARRIAGE RETURN (U+000D).
	const Char CARRIAGE_RETURN = 0x000D;
	/// Code point of NEXT LINE (U+0085).
	const Char NEXT_LINE = 0x0085U;
	/// Code point of ZERO WIDTH NON-JOINER (U+200C).
	const Char ZERO_WIDTH_NON_JOINER = 0x200C;
	/// Code point of ZERO WIDTH JOINER (U+200D).
	const Char ZERO_WIDTH_JOINER = 0x200D;
	/// Code point of LINE SEPARATOR (U+2028).
	const Char LINE_SEPARATOR = 0x2028U;
	/// Code point of PARAGRAPH SEPARATOR (U+2029).
	const Char PARAGRAPH_SEPARATOR = 0x2029U;
	/// Code point of non-character (U+FFFF).
	const Char NONCHARACTER = 0xFFFFU;
	/// Invalid code point value.
	const CodePoint INVALID_CODE_POINT = 0xFFFFFFFFU;
	/// Set of line break characters.
	const Char LINE_BREAK_CHARACTERS[] = {LINE_FEED, CARRIAGE_RETURN, NEXT_LINE, LINE_SEPARATOR, PARAGRAPH_SEPARATOR};

	/**
	 * Represents direction in a text or a document (not visual orientation. See @c #viewers#Orientation).
	 * @see #text, #searcher
	 */
	enum Direction {
		FORWARD,	///< Direction to the end.
		BACKWARD	///< Direction to the start.
	};

	/**
	 * Notifies about the system parameter changes.
	 * Clients of Ascension should call this function when received @c WM_SETTINGCHANGE window message.
	 */
	void updateSystemSettings() throw();

} // namespace ascension

#endif /* ASCENSION_COMMON_HPP */
