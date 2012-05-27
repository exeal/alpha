/**
 * @file platforms.hpp
 * @author exeal
 * @date 2010-11-06 separated from common.hpp
 */

#ifndef ASCENSION_PLATFORMS_HPP
#define ASCENSION_PLATFORMS_HPP


/*
	Operating system (ASCENSION_OS_*)
	- AIX : AIX
	- BSD4 : Any BSD 4.4 system
	- DARWIN : Darwin OS
	- HPUX : HP-UX
	- LINUX : Linux
	- SOLARIS : Sun Solaris
	- UNIX : Any Unix-like system
	- WINDOWS : Windows
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
#elif defined(ANDROID)
/*#	define ASCENSION_OS_ANDROID*/
#elif defined(__HAIKU__)
/*#	define ASCENSION_OS_HAIKU*/
#elif defined(__QNXNTO__)
/*#	define ASCENSION_OS_QNX*/
#elif defined(__SYMBIAN32__)
/*#	define ASCENSION_OS_SYMBIAN*/
#else
#	define ASCENSION_OS_POSIX
#endif

#ifdef ASCENSION_OS_DARWIN
#	define ASCENSION_OS_MACOSX
#	if ASCENSION_OS(DARWIN64)
#		define ASCENSION_OS_MAC64
#	elif ASCENSION_OS(DARWIN32)
#		define ASCENSION_OS_MAC32
#	endif
#endif // ASCENSION_OS_DARWIN

#if defined(ASCENSION_OS_AIX)			\
/*	|| defined(ASCENSION_OS_ANDROID)*/	\
	|| defined(ASCENSION_OS_BSD4)		\
	|| defined(ASCENSION_OS_DARWIN)		\
/*	|| defined(ASCENSION_OS_HAIKU)*/	\
	|| defined(ASCENSION_OS_LINUX)		\
/*	|| defined(ASCENSION_OS_QNX)*/		\
	|| defined(ASCENSION_OS_SOLARIS)	\
/*	|| defined(ASCENSION_OS_SYMBIAN)*/	\
	|| defined(unix)					\
	|| defined(__unix)					\
	|| defined(__unix__)
#	define ASCENSION_OS_UNIX
#endif

#if defined(ASCENSION_OS_WIN64) || defined(ASCENSION_OS_WIN32) || defined(ASCENSION_OS_WINCE)
#	define ASCENSION_OS_WINDOWS
#endif


/*
	Window system (ASCENSION_WINDOW_SYSTEM_*)
	- GTK : GTK+ 3 (gtkmm 3.x)
	- QUARTZ : Quartz Compositor of Mac OS X
	- QT : Nokia Qt
	- WIN32 : Windows Win32
	- X : X Window System (not supported directly)
 */
#if defined(ASCENSION_OS_DARWIN)
#	define ASCENSION_WINDOW_SYSTEM_QUARTZ
#elif defined(ASCENSION_OS_WINDOWS)
#	define ASCENSION_WINDOW_SYSTEM_WIN32
#endif


/*
	Graphics system (ASCENSION_GRAPHICS_SYSTEM_*)
	- CAIRO : Cairo
	- CORE_GRAPHICS : Mac OS X Core Graphics
	- DIRECT2D : Windows Direct2D
	- QT : Nokia Qt
	- WIN32_GDI : Windows GDI
	- WIN32_GDIPLUS : Windows GDI+
 */
#if !defined(ASCENSION_GRAPHICS_SYSTEM_CAIRO)				\
	&& !defined(ASCENSION_GRAPHICS_SYSTEM_CORE_GRAPHICS)	\
	&& !defined(ASCENSION_GRAPHICS_SYSTEM_DIRECT2D)			\
	&& !defined(ASCENSION_GRAPHICS_SYSTEM_QT)				\
	&& !defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI)		\
	&& !defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDIPLUS)
#	if defined(ASCENSION_WINDOW_SYSTEM_GTK)
#		define ASCENSION_GRAPHICS_SYSTEM_CAIRO
#	elif defined(ASCENSION_WINDOW_SYSTEM_QUARTZ)
#		define ASCENSION_GRAPHICS_SYSTEM_CORE_GRAPHICS
#	elif defined(ASCENSION_WINDOW_SYSTEM_QT)
#		define ASCENSION_GRAPHICS_SYSTEM_QT
#	elif defined(ASCENSION_WINDOW_SYSTEM_WIN32)
#		define ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI
#	endif
#endif


/*
	Text/Glyph shaping engine (ASCENSION_SHAPING_ENGINE_*)
	- CORE_GRAPHICS : Mac OS X Core Graphics
	- CORE_TEXT : Mac OS X Core Text
	- DIRECT_WRITE : Windows DirectWrite
	- HARFBUZZ : HarfBuzz
	- PANGO : Pango
	- QT : Nokia Qt
	- UNISCRIBE : Windows Uniscribe
	- WIN32_GDI : Windows GDI
	- WIN32_GDIPLUS : Windows GDI+
 */
#if !defined(ASCENSION_SHAPING_ENGINE_CORE_GRAPHICS)	\
	&& !defined(ASCENSION_SHAPING_ENGINE_CORE_TEXT)		\
	&& !defined(ASCENSION_SHAPING_ENGINE_DIRECT_WRITE)	\
	&& !defined(ASCENSION_SHAPING_ENGINE_HARFBUZZ)		\
	&& !defined(ASCENSION_SHAPING_ENGINE_PANGO)			\
	&& !defined(ASCENSION_SHAPING_ENGINE_QT)			\
	&& !defined(ASCENSION_SHAPING_ENGINE_UNISCRIBE)		\
	&& !defined(ASCENSION_SHAPING_ENGINE_WIN32_GDI)		\
	&& !defined(ASCENSION_SHAPING_ENGINE_WIN32_GDIPLUS)
#	if defined(ASCENSION_GRAPHICS_SYSTEM_CORE_GRAPHICS)
#		define ASCENSION_SHAPING_ENGINE_CORE_TEXT
#	elif defined(ASCENSION_GRAPHICS_SYSTEM_CAIRO)
#		define ASCENSION_SHAPING_ENGINE_PANGO
#	elif defined(ASCENSION_WINDOW_SYSTEM_QT)
#		define ASCENSION_SHAPING_ENGINE_QT
#	elif defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI)
#		define ASCENSION_SHAPING_ENGINE_UNISCRIBE
#	endif
#endif


/*
	C++ compiler (ASCENSION_COMPILER_*)
	- COMEAU : Comeau C++
	- GCC : GNU C++
	- MSVC : Microsoft Visual C++
	- WATCOM : Watcom C++
 */
#if defined(_MSC_VER)
#	define ASCENSION_COMPILER_MSVC
#elif defined(__WATCOMC__)
#	define ASCENSION_COMPILER_WATCOM
#elif defined(__GNUC__)
#	define ASCENSION_COMPILER_GCC
#elif defined(__COMO__)
#	define ASCENSION_COMPILER_COMEAU
#endif


// ASCENSION_FASTCALL

#if defined(__i386__) || defined(_WIN32) || defined(_WIN32_WCE)
#	if defined(ASCENSION_COMPILER_GCC) && (__GNUC__ * 100 + __GNUC_MINOR__ + 10 + __GNUC_PATCHLEVEL__ >= 332)
#		define ASCENSION_FASTCALL __attribute__((regparm(3)))
#	elif defined(ASCENSION_COMPILER_MSVC) && (_MSC_VER > 1300)
#		define ASCENSION_FASTCALL __fastcall
#	endif
#endif
#ifndef ASCENSION_FASTCALL
#	define ASCENSION_FASTCALL
#endif


// ASCENSION_HAS_CSTDINT and ASCENSION_HAS_UNISTD_H

#if defined(ASCENSION_OS_AIX)
#	define ASCENSION_HAS_CSTDINT
#	define ASCENSION_HAS_UNISTD_H
#elif defined(ASCENSION_OS_BSD4)
#	define ASCENSION_HAS_UNISTD_H
#elif defined(ASCENSION_OS_HPUX)
#	if defined(ASCENSION_COMPILER_GCC) && (__GNUC__ >= 3)
#		define ASCENSION_CSTDINT
#	endif
#	define ASCENSION_HAS_UNISTD_H
#elif defined(ASCENSION_OS_LINUX)
#	define ASCENSION_HAS_CSTDINT
#	define ASCENSION_HAS_UNISTD_H
#elif defined(ASCENSION_OS_MACOSX)
#	define ASCENSION_HAS_UNISTD_H
#elif defined(ASCENSION_OS_WINDOWS)
#	if defined(__MINGW__)
#		define ASCENSION_HAS_CSTDINT
#		define ASCENSION_HAS_UNISTD_H
#	endif
#elif defined(ASCENSION_OS_SOLARIS)
#	define ASCENSION_HAS_UNISTD_H
#endif

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901)
#	define ASCENSION_HAS_CSTDINT
#elif defined(ASCENSION_COMPILER_MSVC) && (_MSC_VER >= 1600)
#	define ASCENSION_HAS_CSTDINT
#elif defined(ASCENSION_HAS_UNISTD_H)
#	include <unistd.h>
#	if defined(_POSIX_VERSION) && (_POSIX_VERSION >= 200100)
#		define ASCENSION_HAS_STDINT_H
#	endif
#endif

#if defined(ASCENSION_HAS_UNISTD_H) && !defined(ASCENSION_OS_WINDOWS)
#	define ASCENSION_OS_POSIX
#endif


#ifdef ASCENSION_OS_WINDOWS
#	ifndef _GLIBCXX_USE_WCHAR_T
#		define _GLIBCXX_USE_WCHAR_T 1
#	endif
#	ifndef _GLIBCXX_USE_WSTRING
#		define _GLIBCXX_USE_WSTRING 1
#	endif
#endif // ASCENSION_OS_WINDOWS

#ifdef ASCENSION_OS_WINDOWS
#	define ASCENSION_USE_INTRINSIC_WCHAR_T
#endif

#endif // !ASCENSION_PLATFORMS_HPP
