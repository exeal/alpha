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
#	include <ascension/corelib/text/character.hpp>
#	include <ascension/win32/windows.hpp>	// this includes winnt.h
#endif

namespace alpha {
	/// String type on the selected window system.
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
	typedef Glib::ustring PlatformString;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
	typedef QString PlatformString;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
	typedef ??? PlatformString;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
	typedef std::basic_string<WCHAR> PlatformString;
#endif
}

#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
namespace std {
	basic_ostream<WCHAR>& operator<<(basic_ostream<WCHAR>& out, const ascension::text::String& s);
	basic_istream<WCHAR>& operator>>(basic_istream<WCHAR>& in, ascension::text::String& s);
	basic_istream<WCHAR>& operator>>(basic_istream<WCHAR>& in, std::string& s);
}
#endif

#endif // !ALPHA_PLATFORM_STRING_HPP
