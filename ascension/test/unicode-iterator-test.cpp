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
	x::utf::CharacterEncodeIterator<a::uint16_t> i(spot16, ASCENSION_END_OF(spot16));

	BOOST_CHECK_EQUAL(*i++, 0xd800u);
	BOOST_CHECK_EQUAL(*i++, 0xdf02u);
	BOOST_CHECK_EQUAL(*--i, 0xdf02u);
	BOOST_CHECK_EQUAL(*--i, 0xd800u);

	a::CodePoint spot8[] = {0x004du, 0x0430u, 0x4e8cu, 0x10302u};
	x::utf::CharacterEncodeIterator<a::uint8_t> i8(spot8, ASCENSION_END_OF(spot8));

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
	typedef t::UTF32To16Iterator<std::vector<a::CodePoint>::const_iterator> U32to16;
	typedef t::UTF16To32IteratorUnsafe<std::vector<a::Char>::const_iterator> U16to32;
	std::vector<a::Char> v16(U32to16(v.begin()), U32to16(v.end()));
	BOOST_CHECK_EQUAL(std::distance(U32to16(v.begin()), U32to16(v.end())), v16.size());
	std::vector<a::CodePoint> v32(U16to32(v16.begin()), U16to32(v16.end()));
	BOOST_CHECK_EQUAL(std::distance(U16to32(v16.begin()), U16to32(v16.end())), v32.size());
	BOOST_CHECK_EQUAL(v.size(), v32.size());
	std::vector<a::CodePoint>::const_iterator i = v.begin(), j = v.begin(), k;
	std::advance(j, std::min(v.size(), v32.size()));
	k = v32.begin();
	BOOST_CHECK_EQUAL_COLLECTIONS(v.begin(), v.end(), v32.begin(), v32.end());

	typedef std::reverse_iterator<U32to16> RU32to16;
	typedef std::reverse_iterator<U16to32> RU16to32;
	v16.assign(RU32to16(U32to16(v.end())), RU32to16(U32to16(v.begin())));
	BOOST_CHECK_EQUAL(std::distance(RU32to16(U32to16(v.end())), RU32to16(U32to16(v.begin()))), v16.size());
	std::reverse(v16.begin(), v16.end());
	v32.assign(RU16to32(U16to32(v16.end())), RU16to32(U16to32(v16.begin())));
	BOOST_CHECK_EQUAL(std::distance(RU16to32(U16to32(v16.end())), RU16to32(U16to32(v16.begin()))), v32.size());
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
