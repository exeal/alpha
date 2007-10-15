// unicode-iterator-test.cpp

#include <boost/test/unit_test.hpp>
#include <boost/test/included/unit_test.hpp>
#include "../unicode.hpp"

// from boost/libs/regex/test/unicode/unicode_iterator_test.cpp
void testUnicodeIterator() {
	// spot checks
	ascension::CodePoint spot16[] = {0x10302U};
	ascension::unicode::UTF32To16Iterator<> it(spot16);
	BOOST_CHECK_EQUAL(*it++, 0xD800U);
	BOOST_CHECK_EQUAL(*it++, 0xDF02U);
	BOOST_CHECK_EQUAL(*--it, 0xDF02U);
	BOOST_CHECK_EQUAL(*--it, 0xD800U);

	std::vector<ascension::CodePoint> v;
	v.push_back(0);
	v.push_back(0xD7FF);
	v.push_back(0xE000);
	v.push_back(0xFFFF);
	v.push_back(0x10000);
	v.push_back(0x10FFFF);
	v.push_back(0x80U);
	v.push_back(0x80U - 1);
	v.push_back(0x800U);
	v.push_back(0x800U - 1);
	v.push_back(0x10000U);
	v.push_back(0x10000U - 1);

	typedef ascension::unicode::UTF32To16Iterator<std::vector<ascension::CodePoint>::const_iterator> U32to16;
	typedef ascension::unicode::UTF16To32IteratorUnsafe<std::vector<ascension::Char>::const_iterator> U16to32;
	std::vector<ascension::Char> v16(U32to16(v.begin()), U32to16(v.end()));
	BOOST_CHECK_EQUAL(std::distance(U32to16(v.begin()), U32to16(v.end())), v16.size());
	std::vector<ascension::CodePoint> v32(U16to32(v16.begin()), U16to32(v16.end()));
	BOOST_CHECK_EQUAL(std::distance(U16to32(v16.begin()), U16to32(v16.end())), v32.size());
	BOOST_CHECK_EQUAL(v.size(), v32.size());
	std::vector<ascension::CodePoint>::const_iterator i = v.begin(), j = v.begin(), k;
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

boost::unit_test::test_suite* init_unit_test_suite(int, char*[]) {
	boost::unit_test::test_suite* test = BOOST_TEST_SUITE("Unicode iterator test");
	test->add(BOOST_TEST_CASE(&testUnicodeIterator));
	return test;
}
