/**
 * @file fastcall.hpp
 * Defines @c ASCENSION_DETAIL_FASTCALL symbol.
 * @author exeal
 * @date 2016-12-23 Created.
 */

#ifndef ASCENSION_FASTCALL_HPP
#define ASCENSION_FASTCALL_HPP
#include <boost/predef.h>

/**
 * @internal
 * @def ASCENSION_DETAIL_FASTCALL
 * 'fastcall' calling convention.
 */

#if defined(__i386__) || defined(_WIN32) || defined(_WIN32_WCE)
#	if defined(BOOST_COMP_GNUC) && (BOOST_COMP_GNUC >= BOOST_VERSION_NUMBER(3, 3, 2))
#		define ASCENSION_DETAIL_FASTCALL __attribute__((regparm(3)))
#	elif defined(BOOST_COMP_MSVC) && (BOOST_COMP_MSVC > BOOST_VERSION_NUMBER(7, 0, 0))
#		define ASCENSION_DETAIL_FASTCALL __fastcall
#	endif
#endif

#ifndef ASCENSION_DETAIL_FASTCALL
#	define ASCENSION_DETAIL_FASTCALL
#endif

#endif // !ASCENSION_FASTCALL_HPP
