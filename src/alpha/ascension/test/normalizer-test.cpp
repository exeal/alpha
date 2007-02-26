// unicode-iterator-test.cpp

#include "stdafx.h"
#include "../unicode.hpp"

using namespace ascension;
using namespace ascension::unicode;

namespace {
	void testCompare() {
		BOOST_CHECK_EQUAL(Normalizer::compare(L"", L"", CASE_SENSITIVE), 0);
		BOOST_CHECK_EQUAL(Normalizer::compare(L"abc", L"abc", CASE_SENSITIVE), 0);
		BOOST_CHECK_EQUAL(Normalizer::compare(L"C\x0301\x0327", L"C\x0327\x0301", CASE_SENSITIVE), 0);
//		BOOST_CHECK_EQUAL(Normalizer::compare(L"\x1E69", L"s\x0323\x0307", CASE_SENSITIVE), 0);
	}
	void testNormalize() {
	}
} // namespace @0

void testNormalizer() {
	testCompare();
	testNormalize();

	const String source(L"\x1E69");
	Normalizer n(StringCharacterIterator(source), Normalizer::FORM_D);
	BOOST_CHECK_EQUAL(*n, 0x0073);
	BOOST_CHECK_EQUAL(*++n, 0x0323);
	BOOST_CHECK_EQUAL(*++n, 0x0307);
	BOOST_CHECK((++n).isLast());
	BOOST_CHECK_EQUAL(*--n, 0x0307);
	BOOST_CHECK_EQUAL(*--n, 0x0323);
	BOOST_CHECK_EQUAL(*--n, 0x0073);
	BOOST_CHECK(n.isFirst());

	const String source2(L"s\x0307\x0323");
	Normalizer n2(StringCharacterIterator(source2), Normalizer::FORM_D);
	BOOST_CHECK_EQUAL(*(n++), *(n2++));
	BOOST_CHECK_EQUAL(*(n++), *(n2++));
	BOOST_CHECK_EQUAL(*(n++), *(n2++));
}
