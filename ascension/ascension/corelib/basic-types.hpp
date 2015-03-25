/**
 * @file basic-types.hpp
 * @author exeal
 * @date 2004-2010
 * @date 2010-10-21 separated from common.hpp
 * @date 2010-11-07 joined with common.hpp
 * @date 2011-2015
 */

#ifndef ASCENSION_BASIC_TYPES_HPP
#define ASCENSION_BASIC_TYPES_HPP
#include <ascension/platforms.hpp>	// ASCENSION_USE_INTRINSIC_WCHAR_T
#include <boost/config.hpp>	// BOOST_NOEXCEPT
#include <cstdint>
#include <string>
#ifdef ASCENSION_TEST
#	include <iomanip>
#endif // !ASCENSION_TEST

/// Version of Ascension library
#define ASCENSION_LIBRARY_VERSION 0x0080	// 0.8.0

/// Version of Unicode we're tracking
#define ASCENSION_UNICODE_VERSION 0x0510	// 5.1.0

#ifdef ASCENSION_ABANDONED_AT_VERSION_08
/// Makes the specified class unassignable. Used in class definition.
#	define ASCENSION_UNASSIGNABLE_TAG(className) private: className& operator=(const className&)
/// Makes the specified class uncopyable. Used in class definition
#	define ASCENSION_NONCOPYABLE_TAG(className) ASCENSION_UNASSIGNABLE_TAG(className); className(const className&)
#endif

namespace ascension {
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
	typedef std::uint16_t Char;				///< Type for characters as UTF-16 code unit.
#endif
	typedef std::uint32_t CodePoint;		///< Unicode code point.
	typedef std::basic_string<Char> String;	///< Type for strings as UTF-16.
	static_assert(sizeof(Char) == 2, "");
	static_assert(sizeof(CodePoint) == 4, "");

	typedef std::size_t Index;			///< Length of string or index.
	typedef std::ptrdiff_t SignedIndex;	///< Signed @c Index.
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
	// use boost::optional<Index>, instead
	const Index INVALID_INDEX = 0xfffffffful;	///< Invalid value of @c Index.
#endif // ASCENSION_ABANDONED_AT_VERSION_08
	static_assert(sizeof(Index) == sizeof(SignedIndex), "");
//	static_assert(std::is_unsigned<Index>::value, "");
//	static_assert(std::is_signed<SignedIndex>::value, "");

	/**
	 * Notifies about the system parameter changes.
	 * Clients of Ascension should call this function when the system settings are changed
	 * (for example, received @c WM_SETTINGCHANGE window message on Win32 platform).
	 * @deprecated 0.8
	 */
	void updateSystemSettings() BOOST_NOEXCEPT;

	// see session.hpp
	namespace texteditor {
		class Session;
		namespace detail {
			class SessionElement {
			protected:
				virtual void setSession(Session& session) BOOST_NOEXCEPT = 0;
				friend class Session;
			};
		}
	}

} // namespace ascension

#ifdef ASCENSION_TEST
namespace std {
	template<typename CharType, typename CharTraits>
	inline basic_ostream<CharType, CharTraits>& operator<<(basic_ostream<CharType, CharTraits>& out, const ascension::String& value) {
		out << std::setfill(out.widen('0'));
		for(ascension::String::const_iterator i(begin(value)), e(end(value)); i != e; ++i) {
			if(*i < 0x80)
				out << *i;
			else
				out << std::setw(4) << static_cast<std::uint16_t>(*i);
		}
		return out;
	}
}
#endif // ASCENSION_TEST

#endif // !ASCENSION_BASIC_TYPES_HPP
