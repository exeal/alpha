/**
 * @file string-piece-gtk.cpp
 * @author exeal
 * @date 2015-04-26 Separated from string-piece.hpp.
 */

#if ASCENSION_SUPPORTS_WINDOW_SYSTEM(GTK) || ASCENSION_SUPPORTS_GRAPHICS_SYSTEM(CAIRO) || ASCENSION_SUPPORTS_SHAPING_ENGINE(PANGO)
#	include <ascension/corelib/string-piece.hpp>
#	include <glib.h>

namespace ascension {
	/**
	 * Converts the given @c StringPiece into a @c Glib#ustring.
	 * @param s The @c StringPiece to convert
	 * @return The converted @c Glib#ustring
	 * @throw Glib#Error @c g_utf16_to_utf8 failed
	 */
	Glib::ustring toGlibUstring(const StringPiece& s) {
		GError* error;
		std::shared_ptr<const char> p(::g_utf16_to_utf8(reinterpret_cast<const gunichar2*>(s.cbegin()), s.length(), nullptr, nullptr, &error), &::g_free);
		if(p.get() == nullptr)
			Glib::Error::throw_exception(error);
		return Glib::ustring(p.get());
	}

	/**
	 * Converts the given @c Glib#ustring into a @c String.
	 * @param s The @c Glib::string to convert
	 * @return The converted @c String
	 * @throw Glib#Error @c g_utf8_to_utf16 failed
	 */
	String fromGlibUstring(const Glib::ustring& s) {
		GError* error;
		std::shared_ptr<const gunichar2> p(::g_utf8_to_utf16(s.data(), s.length(), nullptr, nullptr, &error), &::g_free);
		if(p.get() == nullptr)
			Glib::Error::throw_exception(error);
		return String(reinterpret_cast<const Char*>(p.get()));
	}
}
#endif