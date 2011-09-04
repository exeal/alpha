// unicode-iterator-test.cpp

#include <ascension/corelib/text/utf-iterator.hpp>
#include <boost/test/included/test_exec_monitor.hpp>
#include <vector>
#include <algorithm>
namespace a = ascension;
namespace x = ascension::text;

// from boost/libs/regex/test/unicode/unicode_iterator_test.cpp
void spotChecks() {
	a::CodePoint spot16[] = {0x10302u};
	x::utf::CharacterEncodeIterator<a::CodePoint*, a::uint16_t> i(spot16);

	BOOST_CHECK_EQUAL(*i++, 0xd800u);
	BOOST_CHECK_EQUAL(*i++, 0xdf02u);
	BOOST_CHECK_EQUAL(*--i, 0xdf02u);
	BOOST_CHECK_EQUAL(*--i, 0xd800u);

	a::CodePoint spot8[] = {0x004du, 0x0430u, 0x4e8cu, 0x10302u};
	x::utf::CharacterEncodeIterator<a::CodePoint*, a::uint8_t> i8(spot8);

	BOOST_CHECK_EQUAL(*i8++, 0x4du);
	BOOST_CHECK_EQUAL(*i8++, 0xd0u);
	BOOST_CHECK_EQUAL(*i8++, 0xb0u);
	BOOST_CHECK_EQUAL(*i8++, 0xe4u);
	BOOST_CHECK_EQUAL(*i8++, 0xbau);
	BOOST_CHECK_EQUAL(*i8++, 0x8cu);
	BOOST_CHECK_EQUAL(*i8++, 0xf0u);
	BOOST_CHECK_EQUAL(*i8++, 0x90u);
	BOOST_CHECK_EQUAL(*i8++, 0x8cu);
	BOOST_CHECK_EQUAL(*i8++, 0x82u);
	
	BOOST_CHECK_EQUAL(*--i8, 0x82u);
	BOOST_CHECK_EQUAL(*--i8, 0x8cu);
	BOOST_CHECK_EQUAL(*--i8, 0x90u);
	BOOST_CHECK_EQUAL(*--i8, 0xf0u);
	BOOST_CHECK_EQUAL(*--i8, 0x8cu);
	BOOST_CHECK_EQUAL(*--i8, 0xbau);
	BOOST_CHECK_EQUAL(*--i8, 0xe4u);
	BOOST_CHECK_EQUAL(*--i8, 0xb0u);
	BOOST_CHECK_EQUAL(*--i8, 0xd0u);
	BOOST_CHECK_EQUAL(*--i8, 0x4du);
}

void testBoundaries(const std::vector<a::uint32_t>& v) {
	typedef x::utf::CharacterEncodeIterator<std::vector<a::CodePoint>::const_iterator, a::Char> U32to16;
	typedef x::utf::CharacterDecodeIterator<std::vector<a::Char>::const_iterator> U16to32;
	typedef std::reverse_iterator<U32to16> RU32to16;
	typedef std::reverse_iterator<U16to32> RU16to32;
	typedef x::utf::CharacterEncodeIterator<std::vector<a::CodePoint>::const_iterator, a::uint8_t> U32to8;
	typedef x::utf::CharacterDecodeIterator<std::vector<a::uint8_t>::const_iterator> U8to32;
	typedef std::reverse_iterator<U32to8> RU32to8;
	typedef std::reverse_iterator<U8to32> RU8to32;

	// begin by testing forward iteration, of 32-16 bit interconversions
	std::vector<a::Char> v16(U32to16(v.begin()), U32to16(v.end()));
	BOOST_CHECK_EQUAL(std::distance(U32to16(v.begin()), U32to16(v.end())), v16.size());
	std::vector<a::CodePoint> v32(U16to32(v16.begin(), v16.end()), U16to32(v16.begin(), v16.end(), v16.end()));
	BOOST_CHECK_EQUAL(std::distance(U16to32(v16.begin(), v16.end()), U16to32(v16.begin(), v16.end(), v16.end())), v32.size());
	BOOST_CHECK_EQUAL(v.size(), v32.size());
	std::vector<a::CodePoint>::const_iterator i = v.begin(), j = v.begin(), k;
	std::advance(j, std::min(v.size(), v32.size()));
	k = v32.begin();
	BOOST_CHECK_EQUAL_COLLECTIONS(v.begin(), v.end(), v32.begin(), v32.end());

	// test backward iteration, of 32-16 bit interconversions
	v16.assign(RU32to16(U32to16(v.end())), RU32to16(U32to16(v.begin())));
	BOOST_CHECK_EQUAL(std::distance(RU32to16(U32to16(v.end())), RU32to16(U32to16(v.begin()))), v16.size());
	std::reverse(v16.begin(), v16.end());
	v32.assign(RU16to32(U16to32(v16.begin(), v16.end(), v16.end())), RU16to32(U16to32(v16.begin(), v16.end())));
	BOOST_CHECK_EQUAL(std::distance(RU16to32(U16to32(v16.begin(), v16.end(), v16.end())), RU16to32(U16to32(v16.begin(), v16.end()))), v32.size());
	BOOST_CHECK_EQUAL(v.size(), v32.size());
	std::reverse(v32.begin(), v32.end());
	i = v.begin();
	j = i;
	std::advance(j, std::min(v.size(), v32.size()));
	k = v32.begin();
	BOOST_CHECK_EQUAL_COLLECTIONS(v.begin(), v.end(), v32.begin(), v32.end());

	// Test forward iteration, of 32-8 bit interconversions
	std::vector<a::uint8_t> v8(U32to8(v.begin()), U32to8(v.end()));
	BOOST_CHECK_EQUAL(std::distance(U32to8(v.begin()), U32to8(v.end())), v8.size());
	v32.assign(U8to32(v8.begin(), v8.end()), U8to32(v8.begin(), v8.end(), v8.end()));
	BOOST_CHECK_EQUAL(std::distance(U8to32(v8.begin(), v8.end()), U8to32(v8.begin(), v8.end(), v8.end())), v32.size());
	BOOST_CHECK_EQUAL(v.size(), v32.size());
	i = v.begin();
	j = i;
	std::advance(j, std::min(v.size(), v32.size()));
	k = v32.begin();
	BOOST_CHECK_EQUAL_COLLECTIONS(v.begin(), v.end(), v32.begin(), v32.end());

	// test backward iteration, of 32-8 bit interconversions
	v8.assign(RU32to8(U32to8(v.end())), RU32to8(U32to8(v.begin())));
	BOOST_CHECK_EQUAL(std::distance(RU32to8(U32to8(v.end())), RU32to8(U32to8(v.begin()))), v8.size());
	std::reverse(v8.begin(), v8.end());
	v32.assign(RU8to32(U8to32(v8.begin(), v8.end(), v8.end())), RU8to32(U8to32(v8.begin(), v8.end())));
	BOOST_CHECK_EQUAL(std::distance(RU8to32(U8to32(v8.begin(), v8.end(), v8.end())), RU8to32(U8to32(v8.begin(), v8.end()))), v32.size());
	BOOST_CHECK_EQUAL(v.size(), v32.size());
	std::reverse(v32.begin(), v32.end());
	i = v.begin();
	j = i;
	std::advance(j, std::min(v.size(), v32.size()));
	k = v32.begin();
	BOOST_CHECK_EQUAL_COLLECTIONS(v.begin(), v.end(), v32.begin(), v32.end());
}

void testMalformedInputs() {}

int test_main(int, char*[]) {
	spotChecks();

	std::vector<a::CodePoint> v;
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
	testBoundaries(v);

	testMalformedInputs();

	return 0;
}
