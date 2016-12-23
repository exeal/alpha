/**
 * @file basic-types.hpp
 * @author exeal
 * @date 2004-2010
 * @date 2010-10-21 separated from common.hpp
 * @date 2010-11-07 joined with common.hpp
 * @date 2011-2016
 */

#ifndef ASCENSION_BASIC_TYPES_HPP
#define ASCENSION_BASIC_TYPES_HPP
#include <boost/config.hpp>	// BOOST_NOEXCEPT
#include <cstddef>

/// Version of Ascension library
#define ASCENSION_LIBRARY_VERSION 0x0080	// 0.8.0

/// Version of Unicode we're tracking
#define ASCENSION_UNICODE_VERSION 0x0510	// 5.1.0

namespace ascension {
	// shorten type names
	typedef unsigned char Byte;		///< Another short synonym for @c unsigned @c char.
//	typedef unsigned char uchar;	///< A short synonym for @c unsigned @c char.
//	typedef unsigned short ushort;	///< A short synonym for @c unsigned @c short.
//	typedef unsigned int uint;		///< A short synonym for @c unsigned @c int.
//	typedef unsigned long ulong;	///< A short synonym for @c unsigned @c long.

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
	void updateSystemSettings();

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

#endif // !ASCENSION_BASIC_TYPES_HPP
