/**
 * @file utf-8.hpp
 * @author exeal
 * @date 2011-08-19 created
 */

#ifndef ASCENSION_UTF_8_HPP
#define ASCENSION_UTF_8_HPP

#include <ascension/corelib/text/character.hpp>

#if ASCENSION_UNICODE_VERSION > 0x0510
#error These class definitions and implementations are based on old version of Unicode.
#endif

namespace ascension {

	namespace detail {
	}

	namespace text {

		/**
		 * @c utf8 namespace provides low level procedures handle UTF-8 character sequence.
		 * @see utf16
		 */
		namespace utf8 {

		}
	}
} // namespace ascension.text.utf8

#endif // !ASCENSION_UTF_8_HPP
