/**
 * @file platform-string.hpp
 * Defines @c PlatformString type.
 * @author exeal
 * @date 2017-01-19 Created.
 */

#ifndef ALPHA_PLATFORM_STRING_HPP
#define ALPHA_PLATFORM_STRING_HPP
#include <ascension/platforms.hpp>
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
#	include <glibmm/ustring.h>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
#	include <Qstring>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#	include <???>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
#	include <string>
#	include <winnt.h>
#endif

namespace alpha {
	/// String type on the selected window system.
	typedef
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
		Glib::ustring
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
		QString
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
		???
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
		std::basic_string<WCHAR>
#endif
		PlatformString;
}

#endif // !ALPHA_PLATFORM_STRING_HPP
