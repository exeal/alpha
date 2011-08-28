/**
 * @file utf-16.hpp
 * @author exeal
 * @date 2005-2010 was unicode.hpp
 * @date 2010 was character-iterator.hpp
 * @date 2010-11-06 separated from character-iterator.hpp to unicode-surrogates.hpp and
 *                  unicode-utf.hpp
 * @date 2011-08-20 joined unicode-surrogates.hpp and unicode-utf.hpp
 */

#ifndef ASCENSION_UTF_16_HPP
#define ASCENSION_UTF_16_HPP

#include <ascension/corelib/text/character.hpp>	// CodePoint, ASCENSION_STATIC_ASSERT, surrogates.*
#include <cassert>								// assert

#if ASCENSION_UNICODE_VERSION > 0x0510
#error These class definitions and implementations are based on old version of Unicode.
#endif

namespace ascension {

	namespace text {
		namespace utf16 {

		} // namespace utf16
	}
} // namespace ascension.text

#endif // !ASCENSION_UTF_16_HPP
