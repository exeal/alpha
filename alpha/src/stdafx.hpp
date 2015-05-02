/**
 * @file stdafx.hpp
 * Header file for precompiled header files.
 * @author exeal
 * @date 2014-05-10 Created.
 */

#ifndef ALPHA_STDAFX_HPP
#define ALPHA_STDAFX_HPP

#include <ascension/presentation/detail/style-sequence.hpp>
#include <boost/flyweight.hpp>
#include <boost/fusion/container/vector.hpp>

#include <ascension/platforms.hpp>

#if ASCENSION_SUPPORTS_WINDOW_SYSTEM(GTK)
#	include <gtkmm.h>
#endif
#if ASCENSION_SUPPORTS_WINDOW_SYSTEM(WIN32)
#	include <ascension/win32/windows.hpp>
#endif

#if ASCENSION_SUPPORTS_GRAPHICS_SYSTEM(CAIRO)
#	include <cairomm/cairomm.h>
#endif

#if ASCENSION_SUPPORTS_SHAPING_ENGINE(PANGO)
#	include <pangomm.h>
#endif

#endif // !ALPHA_STDAFX_HPP
