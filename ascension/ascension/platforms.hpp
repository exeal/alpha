/**
 * @file platforms.hpp
 * @author exeal
 * @date 2010-11-06 separated from common.hpp
 */

#ifndef ASCENSION_PLATFORMS_HPP
#define ASCENSION_PLATFORMS_HPP


/*
	operating system (ASCENSION_OS_*)
	- ASCENSION_OS_AIX : AIX
	- ASCENSION_OS_BSD4 : Any BSD 4.4 system
	- ASCENSION_OS_DARWIN : Darwin OS
	- ASCENSION_OS_HPUX : HP-UX
	- ASCENSION_OS_LINUX : Linux
	- ASCENSION_OS_SOLARIS : Sun Solaris
	- ASCENSION_OS_WINDOWS : Windows
 */

#if defined(__APPLE__) && defined(__GNUC__)
#	define ASCENSION_OS_DARWIN
#	define ASCENSION_OS_BSD4
#	ifdef __LP64__
#		define ASCENSION_OS_DARWIN64
#	else
#		define ASCENSION_OS_DARWIN32
#	endif
#elif defined(__CYGWIN__)
#	define ASCENSION_OS_WINDOWS
#elif defined(_WIN64) || defined(WIN64)
#	define ASCENSION_OS_WIN64
#	define ASCENSION_OS_WIN32
#elif defined(_WIN32) || defined(WIN32)
#	if defined(WINCE) || defined(_WIN32_WCE)
#		define ASCENSION_OS_WINCE
#	else
#		define ASCENSION_OS_WIN32
#	endif
#elif defined(__sun) || defined(sun)
#	define ASCENSION_OS_SOLARIS
#elif defined(__hpux) || defined(hpux)
#	define ASCENSION_OS_HPUX
#elif defined(__linux) || defined(__linux__)
#	define ASCENSION_OS_LINUX
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#	define ASCENSION_OS_BSD4
#elif defined(_AIX)
#	define ASCENSION_OS_AIX
#else
#	define ASCENSION_OS_POSIX
#endif

#ifdef ASCENSION_OS_DARWIN
#	define ASCENSION_OS_MACOSX
#	if defined(ASCENSION_OS_DARWIN64)
#		define ASCENSION_OS_MAC64
#	elif defined(ASCENSION_OS_DARWIN32)
#		define ASCENSION_OS_MAC32
#	endif
#endif

#if defined(ASCENSION_OS_WIN64) || defined(ASCENSION_OS_WIN32) || defined(ASCENSION_OS_WINCE)
#	define ASCENSION_OS_WINDOWS
#endif


/*
	graphics system (ASCENSION_GS_*)
	- ASCENSION_GS_CORE_GRAPHICS : Mac OS X Core Graphics
	- ASCENSION_GS_GTK : GTK+ 2
	- ASCENSION_GS_QT : Nokia Qt
	- ASCENSION_GS_WIN32_GDI : Windows GDI + Uniscribe
 */
#if defined(ASCENSION_OS_DARWIN)
#	define ASCENSION_GS_CORE_GRAPHICS
#elif defined(ASCENSION_OS_WINDOWS)
#	define ASCENSION_GS_WIN32_GDI
#else
#	define ASCENSION_GS_GTK
#	define ASCENSION_GS_QT
#endif


/*
	compiler (ASCENSION_CC_*)
	- ASCENSION_CC_COMEAU : Comeau C++
	- ASCENSION_CC_GCC : GNU C++
	- ASCENSION_CC_MSVC : Microsoft Visual C++
	- ASCENSION_CC_WATCOM : Watcom C++
 */
#if defined(_MSC_VER)
#	define ASCENSION_CC_MSVC
#elif defined(__WATCOMC__)
#	define ASCENSION_CC_WATCOM
#elif defined(__GNUC__)
#	define ASCENSION_CC_GCC
#elif defined(__COMO__)
#	define ASCENSION_CC_COMEAU
#endif

#if defined(__i386__) || defined(_WIN32) || defined(_WIN32_WCE)
#	if defined(ASCENSION_CC_GCC) && (__GNUC__ * 100 + __GNUC_MINOR__ + 10 + __GNUC_PATCHLEVEL__ >= 332)
#		define ASCENSION_FASTCALL __attribute__((regparm(3)))
#	elif defined(ASCENSION_CC_MSVC) && (_MSC_VER > 1300)
#		define ASCENSION_FASTCALL __fastcall
#	endif
#endif
#ifndef ASCENSION_FASTCALL
#	define ASCENSION_FASTCALL
#endif

#ifdef ASCENSION_OS_WINDOWS
#	ifndef _GLIBCXX_USE_WCHAR_T
#		define _GLIBCXX_USE_WCHAR_T 1
#	endif
#	ifndef _GLIBCXX_USE_WSTRING
#		define _GLIBCXX_USE_WSTRING 1
#	endif
#endif // ASCENSION_OS_WINDOWS

#if defined(ASCENSION_OS_WINDOWS)
#	define ASCENSION_USE_INTRINSIC_WCHAR_T
#endif

#endif // !ASCENSION_PLATFORMS_HPP
