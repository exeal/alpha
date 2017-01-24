/**
 * @file string-piece.hpp
 * Defines @c StringPiece class.
 * @author exeal
 * @date 2010-10-21 separated from common.hpp
 * @date 2010-11-06 separated from basic-types.hpp
 * @date 2012-2015
 */

#ifndef ASCENSION_STRING_PIECE_HPP
#define ASCENSION_STRING_PIECE_HPP
#include <ascension/corelib/text/character.hpp>	// Char
#include <boost/utility/string_ref.hpp>
#if ASCENSION_SUPPORTS_WINDOW_SYSTEM(GTK) || ASCENSION_SUPPORTS_GRAPHICS_SYSTEM(CAIRO) || ASCENSION_SUPPORTS_SHAPING_ENGINE(PANGO)
#	include <glibmm/ustring.h>
#endif
#include <cassert>

namespace ascension {
	/// Specialization of @c boost#basic_string_ref for @c Char type.
	typedef boost::basic_string_ref<Char, std::char_traits<Char>> StringPiece;

	/**
	 * Creates and returns a @c StringPiece with the given two iterators.
	 * @param first The iterator addresses the beginning of the string
	 * @param last The iterator addresses the end of the string
	 * @return A @c StringPiece
	 */
	inline StringPiece makeStringPiece(StringPiece::const_iterator first, StringPiece::const_iterator last) {
		assert(first <= last);
		return StringPiece(first, last - first);
	}

#if ASCENSION_SUPPORTS_WINDOW_SYSTEM(GTK) || ASCENSION_SUPPORTS_GRAPHICS_SYSTEM(CAIRO) || ASCENSION_SUPPORTS_SHAPING_ENGINE(PANGO)
	static_assert(sizeof(StringPiece::value_type) == 2, "");
	static_assert(sizeof(StringPiece::value_type) == sizeof(gunichar2), "");

	/**
	 * Converts the given @c StringPiece into a @c Glib#ustring.
	 * @param s The @c StringPiece to convert
	 * @return The converted @c Glib#ustring
	 * @throw Glib#Error @c g_utf16_to_utf8 failed
	 */
	Glib::ustring toGlibUstring(const StringPiece& s);

	/**
	 * Converts the given @c Glib#ustring into a @c String.
	 * @param s The @c Glib::string to convert
	 * @return The converted @c String
	 * @throw Glib#Error @c g_utf8_to_utf16 failed
	 */
	String fromGlibUstring(const Glib::ustring& s);
#endif

} // namespace ascension

#endif // !ASCENSION_STRING_PIECE_HPP
