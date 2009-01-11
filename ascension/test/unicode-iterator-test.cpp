// unicode-iterator-test.cpp

#include "../unicode.hpp"
#include <boost/test/included/test_exec_monitor.hpp>
#include <vector>
#include <algorithm>
namespace a = ascension;
namespace t = ascension::text;

// from boost/libs/regex/test/unicode/unicode_iterator_test.cpp
void testUTFIterator() {
	// spot checks
	a::CodePoint spot16[] = {0x10302U};
	t::UTF32To16Iterator<> it(spot16);
	BOOST_CHECK_EQUAL(*it++, 0xD800U);
	BOOST_CHECK_EQUAL(*it++, 0xDF02U);
	BOOST_CHECK_EQUAL(*--it, 0xDF02U);
	BOOST_CHECK_EQUAL(*--it, 0xD800U);

	std::vector<a::CodePoint> v;
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

void testStringCharacterIterator() {
	// simple test
	const a::String s1(L"test");
	t::StringCharacterIterator i1(s1);
	BOOST_CHECK(!i1.hasPrevious());
	BOOST_CHECK_EQUAL(i1.getOffset(), 0);
	BOOST_CHECK_EQUAL(i1.current(), L't');
	i1.next();
	BOOST_CHECK(i1.hasNext() && i1.hasPrevious());
	BOOST_CHECK_EQUAL(i1.getOffset(), 1);
	BOOST_CHECK_EQUAL(*i1, L'e');
	i1.last();
	BOOST_CHECK(!i1.hasNext());
	BOOST_CHECK_EQUAL(i1.getOffset(), 0);
	BOOST_CHECK_EQUAL(i1.current(), t::CharacterIterator::DONE);

	// out of BMP
	const a::String s2(L"\xD800\xDC00");
	t::StringCharacterIterator i2(s2);
	BOOST_CHECK_EQUAL(i2.current(), 0x010000);
	++i2;
	BOOST_CHECK(!i2.hasNext());
	BOOST_CHECK_EQUAL(i2.getOffset(), 1);
	--i2;
	BOOST_CHECK(!i2.hasPrevious());

	// malformed UTF-16 input
	const a::Char s3[] = L"\xDC00\xD800";
	t::StringCharacterIterator i3(s3, s3 + 2);
	BOOST_CHECK_EQUAL(*i3, 0xDC00);
	BOOST_CHECK(i3.hasNext());
	++i3;
	BOOST_CHECK_EQUAL(*i3, 0xD800);
	++i3;
	BOOST_CHECK_EQUAL(*i3, t::CharacterIterator::DONE);
	std::advance(i3, -2);
	BOOST_CHECK(!i3.hasPrevious());
}

int test_main(int, char*[]) {
	testUTFIterator();
	testStringCharacterIterator();
	return 0;
}
