/**
 * @file platforms.hpp
 * @author exeal
 * @date 2010-11-06 separated from common.hpp
 */

#ifndef ASCENSION_PLATFORMS_HPP
#define ASCENSION_PLATFORMS_HPP

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

#if defined(ASCENSION_WINDOWS)
#	define ASCENSION_USE_INTRINSIC_WCHAR_T
#endif

namespace ascension {
	namespace graphics {
		typedef
#ifdef ASCENSION_WINDOWS
		int
#else
#endif
		Scalar;
	}
}

#endif // !ASCENSION_PLATFORMS_HPP
