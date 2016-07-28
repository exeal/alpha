#define BOOST_TEST_MODULE utf_iterator_test
#include <boost/test/included/unit_test.hpp>

#include <ascension/corelib/text/utf-iterator.hpp>
#include <algorithm>
#include <vector>
#include "unicode-string-sample.hpp"

// from boost/libs/regex/test/unicode/unicode_iterator_test.cpp
BOOST_AUTO_TEST_CASE(spot_checks) {
	ascension::text::utf::CharacterEncodeIterator<const ascension::CodePoint*, std::uint16_t> i(SPOT16);

	BOOST_TEST(*i++ == SPOT16_IN_UTF16[0]);
	BOOST_TEST(*i++ == SPOT16_IN_UTF16[1]);
	BOOST_TEST(*--i == SPOT16_IN_UTF16[1]);
	BOOST_TEST(*--i == SPOT16_IN_UTF16[0]);

	ascension::text::utf::CharacterEncodeIterator<const ascension::CodePoint*, std::uint8_t> i8(SPOT8);

	BOOST_TEST(*i8++ == SPOT8_IN_UTF8[0]);
	BOOST_TEST(*i8++ == SPOT8_IN_UTF8[1]);
	BOOST_TEST(*i8++ == SPOT8_IN_UTF8[2]);
	BOOST_TEST(*i8++ == SPOT8_IN_UTF8[3]);
	BOOST_TEST(*i8++ == SPOT8_IN_UTF8[4]);
	BOOST_TEST(*i8++ == SPOT8_IN_UTF8[5]);
	BOOST_TEST(*i8++ == SPOT8_IN_UTF8[6]);
	BOOST_TEST(*i8++ == SPOT8_IN_UTF8[7]);
	BOOST_TEST(*i8++ == SPOT8_IN_UTF8[8]);
	BOOST_TEST(*i8++ == SPOT8_IN_UTF8[9]);
	
	BOOST_TEST(*--i8 == SPOT8_IN_UTF8[9]);
	BOOST_TEST(*--i8 == SPOT8_IN_UTF8[8]);
	BOOST_TEST(*--i8 == SPOT8_IN_UTF8[7]);
	BOOST_TEST(*--i8 == SPOT8_IN_UTF8[6]);
	BOOST_TEST(*--i8 == SPOT8_IN_UTF8[5]);
	BOOST_TEST(*--i8 == SPOT8_IN_UTF8[4]);
	BOOST_TEST(*--i8 == SPOT8_IN_UTF8[3]);
	BOOST_TEST(*--i8 == SPOT8_IN_UTF8[2]);
	BOOST_TEST(*--i8 == SPOT8_IN_UTF8[1]);
	BOOST_TEST(*--i8 == SPOT8_IN_UTF8[0]);
}

BOOST_AUTO_TEST_CASE(boundaries_test) {
	std::vector<ascension::CodePoint> v;
	v.push_back(0);
	v.push_back(0xd7ffu);
	v.push_back(0xe000u);
	v.push_back(0xffffu);
	v.push_back(0x10000u);
	v.push_back(0x10ffffu);
	v.push_back(0x80u);
	v.push_back(0x80u - 1);
	v.push_back(0x800u);
	v.push_back(0x800u - 1);
	v.push_back(0x10000u);
	v.push_back(0x10000u - 1);

	typedef ascension::text::utf::CharacterEncodeIterator<std::vector<ascension::CodePoint>::const_iterator, ascension::Char> U32to16;
	typedef ascension::text::utf::CharacterDecodeIterator<std::vector<ascension::Char>::const_iterator> U16to32;
	typedef std::reverse_iterator<U32to16> RU32to16;
	typedef std::reverse_iterator<U16to32> RU16to32;
	typedef ascension::text::utf::CharacterEncodeIterator<std::vector<ascension::CodePoint>::const_iterator, std::uint8_t> U32to8;
	typedef ascension::text::utf::CharacterDecodeIterator<std::vector<std::uint8_t>::const_iterator> U8to32;
	typedef std::reverse_iterator<U32to8> RU32to8;
	typedef std::reverse_iterator<U8to32> RU8to32;

	// begin by testing forward iteration, of 32-16 bit interconversions
	std::vector<ascension::Char> v16(U32to16(v.cbegin()), U32to16(v.cend()));
	BOOST_TEST(std::distance(U32to16(v.cbegin()), U32to16(v.cend())) == static_cast<std::ptrdiff_t>(v16.size()));
	std::vector<ascension::CodePoint> v32(U16to32(v16.cbegin(), v16.cend()), U16to32(v16.cbegin(), v16.cend(), v16.cend()));
	BOOST_TEST(std::distance(U16to32(v16.cbegin(), v16.cend()), U16to32(v16.cbegin(), v16.cend(), v16.cend())) == static_cast<std::ptrdiff_t>(v32.size()));
	BOOST_TEST(v.size() == v32.size());
	auto i(v.cbegin()), j(v.cbegin());
	std::advance(j, std::min(v.size(), v32.size()));
	auto k(v32.cbegin());
	BOOST_CHECK_EQUAL_COLLECTIONS(v.cbegin(), v.cend(), v32.cbegin(), v32.cend());

	// test backward iteration, of 32-16 bit interconversions
	v16.assign(RU32to16(U32to16(v.cend())), RU32to16(U32to16(v.cbegin())));
	BOOST_TEST(std::distance(RU32to16(U32to16(v.cend())), RU32to16(U32to16(v.cbegin()))) == static_cast<std::ptrdiff_t>(v16.size()));
	std::reverse(v16.begin(), v16.end());
	v32.assign(RU16to32(U16to32(v16.cbegin(), v16.cend(), v16.cend())), RU16to32(U16to32(v16.cbegin(), v16.cend())));
	BOOST_TEST(std::distance(RU16to32(U16to32(v16.cbegin(), v16.cend(), v16.cend())), RU16to32(U16to32(v16.cbegin(), v16.cend()))) == static_cast<std::ptrdiff_t>(v32.size()));
	BOOST_TEST(v.size() == v32.size());
	std::reverse(v32.begin(), v32.end());
	i = v.cbegin();
	j = i;
	std::advance(j, std::min(v.size(), v32.size()));
	k = v32.cbegin();
	BOOST_CHECK_EQUAL_COLLECTIONS(v.cbegin(), v.cend(), v32.cbegin(), v32.cend());

	// test forward iteration, of 32-8 bit interconversions
	std::vector<std::uint8_t> v8(U32to8(v.cbegin()), U32to8(v.cend()));
	BOOST_TEST(std::distance(U32to8(v.cbegin()), U32to8(v.cend())) == static_cast<std::ptrdiff_t>(v8.size()));
	v32.assign(U8to32(v8.cbegin(), v8.cend()), U8to32(v8.cbegin(), v8.cend(), v8.cend()));
	BOOST_TEST(std::distance(U8to32(v8.cbegin(), v8.cend()), U8to32(v8.cbegin(), v8.cend(), v8.cend())) == static_cast<std::ptrdiff_t>(v32.size()));
	BOOST_TEST(v.size() == v32.size());
	i = v.cbegin();
	j = i;
	std::advance(j, std::min(v.size(), v32.size()));
	k = v32.cbegin();
	BOOST_CHECK_EQUAL_COLLECTIONS(v.cbegin(), v.cend(), v32.cbegin(), v32.cend());

	// test backward iteration, of 32-8 bit interconversions
	v8.assign(RU32to8(U32to8(v.cend())), RU32to8(U32to8(v.cbegin())));
	BOOST_TEST(std::distance(RU32to8(U32to8(v.cend())), RU32to8(U32to8(v.cbegin()))) == static_cast<std::ptrdiff_t>(v8.size()));
	std::reverse(v8.begin(), v8.end());
	v32.assign(RU8to32(U8to32(v8.cbegin(), v8.cend(), v8.cend())), RU8to32(U8to32(v8.cbegin(), v8.cend())));
	BOOST_TEST(std::distance(RU8to32(U8to32(v8.cbegin(), v8.cend(), v8.cend())), RU8to32(U8to32(v8.cbegin(), v8.cend()))) == static_cast<std::ptrdiff_t>(v32.size()));
	BOOST_TEST(v.size() == v32.size());
	std::reverse(v32.begin(), v32.end());
	i = v.begin();
	j = i;
	std::advance(j, std::min(v.size(), v32.size()));
	k = v32.begin();
	BOOST_CHECK_EQUAL_COLLECTIONS(v.cbegin(), v.cend(), v32.cbegin(), v32.cend());
}

BOOST_AUTO_TEST_CASE(utf8_decode_test) {
	static const char UTF_8[] = "abcdef";
	ascension::String utf16(ascension::text::utf::decode(UTF_8));
	BOOST_CHECK_EQUAL_COLLECTIONS(utf16.cbegin(), utf16.cend(), UTF_8, UTF_8 + 6);
}

BOOST_AUTO_TEST_CASE(malformed_inputs_test) {
}
