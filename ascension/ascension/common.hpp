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

/// Returns the number of the elements of the given array.
#define ASCENSION_COUNTOF(array) (sizeof(array) / sizeof((array)[0]))
/// Returns the end of the given array.
#define ASCENSION_ENDOF(array) ((array) + ASCENSION_COUNTOF(array))
/// Makes the specified class unassignable. Used in class definition.
#define ASCENSION_UNASSIGNABLE_TAG(className) private: className& operator=(const className&)
/// Makes the specified class uncopyable. Used in class definition
#define ASCENSION_NONCOPYABLE_TAG(className) ASCENSION_UNASSIGNABLE_TAG(className); className(const className&)

// platform
#if defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
#	define ASCENSION_WINDOWS
#	include "win32/windows.hpp"
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

namespace ascension {

	namespace internal {
		template<unsigned> struct StaticAssertTest {};
		template<int> struct StaticAssertionFailureAtLine;
		template<> struct StaticAssertionFailureAtLine<-1> {};
	} // namespace internal

	#define ASCENSION_STATIC_ASSERT(expression)														\
		typedef ascension::internal::StaticAssertTest<												\
			sizeof(ascension::internal::StaticAssertionFailureAtLine<(expression) ? -1 : __LINE__>)	\
		> oh_static_assertion_shippaidayo_orz

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
	typedef unsigned char byte;		///< Another short synonym for @c unsigned @c char.
	typedef unsigned char uchar;	///< A short synonym for @c unsigned @c char.
	typedef unsigned short ushort;	///< A short synonym for @c unsigned @c short.
	typedef unsigned int uint;		///< A short synonym for @c unsigned @c int.
	typedef unsigned long ulong;	///< A short synonym for @c unsigned @c long.

	// character and string
#ifdef ASCENSION_USE_INTRINSIC_WCHAR_T
	typedef wchar_t Char;					///< Type for characters as UTF-16 code unit.
#else
	typedef uint16_t;						///< Type for characters as UTF-16 code unit.
#endif
	typedef uint32_t CodePoint;				///< Unicode code point.
	typedef std::basic_string<Char> String;	///< Type for strings as UTF-16.
	ASCENSION_STATIC_ASSERT(sizeof(Char) == 2);
	ASCENSION_STATIC_ASSERT(sizeof(CodePoint) == 4);

	typedef std::size_t length_t;					///< Length of string or index.
	const length_t INVALID_INDEX = 0xfffffffful;	///< Invalid value of @c length_t.

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
	 * Notifies about the system parameter changes.
	 * Clients of Ascension should call this function when the system settings are changed
	 * (for example, received @c WM_SETTINGCHANGE window message on Win32 platform).
	 */
	void updateSystemSettings() /*throw()*/;

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
