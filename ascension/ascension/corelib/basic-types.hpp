/**
 * @file basic-types.hpp
 * @author exeal
 * @date 2004-2011
 * @date 2010-10-21 separated from common.hpp
 * @date 2010-11-07 joined with common.hpp
 */

#ifndef ASCENSION_BASIC_TYPES_HPP
#define ASCENSION_BASIC_TYPES_HPP

#include <ascension/config.hpp>	// ASCENSION_FILE_NAME_CHARACTER_TYPE
#include <ascension/platforms.hpp>
#ifdef ASCENSION_HAS_CSTDINT
#	if defined(ASCENSION_OS_AIX) || defined(ASCENSION_OS_BSD4) || defined(ASCENSION_OS_HPUX)
#		include <inttypes.h>
#	else
#		include <stdint.h>
#	endif
#endif
#ifdef ASCENSION_CUSTOM_SHARED_PTR_HPP
#	include ASCENSION_CUSTOM_SHARED_PTR_HPP
#elif defined(ASCENSION_COMPILER_MSVC) && _MSC_VER >= 1500
#	include <memory>
#elif defined(ASCENSION_COMPILER_GCC) && __GNUC__ >= 4
#	include <tr1/memory>
#else
#	include <boost/tr1/memory.hpp>
#endif
#include <cmath>	// std.abs(double)
#include <iterator>	// std.back_insert_iterator, std.front_insert_iterator, std.ostream_iterator
#include <string>

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

namespace ascension {

	namespace detail {
		template<unsigned> struct StaticAssertTest {};
		template<int> struct StaticAssertionFailureAtLine;
		template<> struct StaticAssertionFailureAtLine<-1> {};
	} // namespace detail

	#define ASCENSION_STATIC_ASSERT(expression)														\
		typedef ascension::detail::StaticAssertTest<												\
			sizeof(ascension::detail::StaticAssertionFailureAtLine<(expression) ? -1 : __LINE__>)	\
		> oh_static_assertion_shippaidayo_orz

	// sized integer types
#if defined(ASCENSION_HAS_CSTDINT)
	using ::int8_t;
	using ::int16_t;
	using ::int32_t;
	using ::int64_t;
	using ::uint8_t;
	using ::uint16_t;
	using ::uint32_t;
	using ::uint64_t;
#elif defined(ASCENSION_COMPILER_MSVC)
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
	typedef unsigned char Byte;		///< Another short synonym for @c unsigned @c char.
//	typedef unsigned char uchar;	///< A short synonym for @c unsigned @c char.
//	typedef unsigned short ushort;	///< A short synonym for @c unsigned @c short.
//	typedef unsigned int uint;		///< A short synonym for @c unsigned @c int.
//	typedef unsigned long ulong;	///< A short synonym for @c unsigned @c long.

	// character and string
#ifdef ASCENSION_USE_INTRINSIC_WCHAR_T
	typedef wchar_t Char;					///< Type for characters as UTF-16 code unit.
#else
	typedef uint16_t Char;					///< Type for characters as UTF-16 code unit.
#endif
	typedef uint32_t CodePoint;				///< Unicode code point.
	typedef std::basic_string<Char> String;	///< Type for strings as UTF-16.
	ASCENSION_STATIC_ASSERT(sizeof(Char) == 2);
	ASCENSION_STATIC_ASSERT(sizeof(CodePoint) == 4);

	typedef std::size_t length_t;					///< Length of string or index.
	typedef std::ptrdiff_t signed_length_t;			///< Signed @c length_t.
	const length_t INVALID_INDEX = 0xfffffffful;	///< Invalid value of @c length_t.

	/// Returns @c true if the given floating-point numbers are (approximately) equal.
	inline bool equals(double n1, double n2, double epsilon = 1.0e-5) {return std::abs(n1 - n2) <= epsilon;}

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

	/**
	 * Notifies about the system parameter changes.
	 * Clients of Ascension should call this function when the system settings are changed
	 * (for example, received @c WM_SETTINGCHANGE window message on Win32 platform).
	 * @deprecated 0.8
	 */
	void updateSystemSettings() /*throw()*/;

	// see session.hpp
	namespace texteditor {class Session;}
	namespace detail {
		class SessionElement {
		protected:
			virtual void setSession(texteditor::Session& session) /*throw()*/ = 0;
			friend class texteditor::Session;
		};
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

#endif // !ASCENSION_BASIC_TYPES_HPP
