#ifndef ASCENSION_TEST_FROM_LATIN1_HPP
#define ASCENSION_TEST_FROM_LATIN1_HPP

#include <ascension/corelib/string-piece.hpp>

namespace {
	template<typename Character>
	inline ascension::String fromLatin1(const boost::basic_string_ref<Character, std::char_traits<Character>>& from) {
		static_assert(sizeof(Character) == 1, "");
		return ascension::String(from.cbegin(), from.cend());
	}

	template<typename Character>
	inline ascension::String fromLatin1(const Character* from) {
		return fromLatin1(boost::basic_string_ref<Character, std::char_traits<Character>>(from));
	}
}

#endif // !ASCENSION_TEST_FROM_LATIN1_HPP
