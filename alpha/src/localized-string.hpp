/**
 * @file localized-string.hpp
 * Defines @c localizedString free function.
 * @author exeal
 * @date 2017-02-19 Created.
 */

#ifndef ALPHA_LOCALIZED_STRING_HPP
#define ALPHA_LOCALIZED_STRING_HPP
#include "platform-string.hpp"
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
#	include <glibmm/i18n.h>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
#	include <QObject>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
#	include <ascension/corelib/text/from-latin1.hpp>
#endif

namespace alpha {
	/**
	 * Under designing.
	 */
	inline PlatformString localizedString(const PlatformString& s) {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
		return _(s.c_str());
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
		return QObject::tr(s.c_str());
#else
		return s;
#endif
	}

#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
	/// @overload
	inline PlatformString localizedString(const char* s) {
		return localizedString(ascension::fromLatin1<PlatformString>(s));
	}
#endif
}

#endif // !ALPHA_LOCALIZED_STRING_HPP
