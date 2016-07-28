#ifndef ASCENSION_TEST_UNICODE_SURROGATES_HPP
#define ASCENSION_TEST_UNICODE_SURROGATES_HPP
#include <ascension/corelib/basic-types.hpp>

namespace {
	const ascension::Char NON_PRIVATE_USE_HIGH_SURROGATE_FIRST = 0xd800u;
	const ascension::Char NON_PRIVATE_USE_HIGH_SURROGATE_LAST = 0xdb7fu;
	const ascension::Char PRIVATE_USE_HIGH_SURROGATE_FIRST = 0xdb80u;
	const ascension::Char PRIVATE_USE_HIGH_SURROGATE_LAST = 0xdbffu;
	const ascension::Char LOW_SURROGATE_FIRST = 0xdc00u;
	const ascension::Char LOW_SURROGATE_LAST = 0xdfffu;
}

#endif // !ASCENSION_TEST_UNICODE_SURROGATES_HPP
