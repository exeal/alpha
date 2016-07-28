#ifndef ASCENSION_TEST_UNICODE_STRING_SAMPLE_HPP
#define ASCENSION_TEST_UNICODE_STRING_SAMPLE_HPP
#include <ascension/corelib/basic-types.hpp>

namespace {
	const ascension::CodePoint SPOT16[] = {0x10302u};
	const std::uint16_t SPOT16_IN_UTF16[] = {0xd800u, 0xdf02u};
	const ascension::CodePoint SPOT8[] = {0x004du, 0x0430u, 0x4e8cu, 0x10302u};
	const std::uint8_t SPOT8_IN_UTF8[] = {
		0x4du,
		0xd0u, 0xb0u,
		0xe4u, 0xbau, 0x8cu,
		0xf0u, 0x90u, 0x8cu, 0x82u
	};
}

#endif // !ASCENSION_TEST_UNICODE_STRING_SAMPLE_HPP
