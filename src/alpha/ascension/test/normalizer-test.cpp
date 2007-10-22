// unicode-iterator-test.cpp

#include "../unicode.hpp"
#include <boost/test/included/test_exec_monitor.hpp>

namespace {
	void testCompare() {
		using namespace ascension::unicode;
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

	const ascension::String source(L"\x1E69");
	ascension::unicode::Normalizer n(ascension::unicode::StringCharacterIterator(source), ascension::unicode::Normalizer::FORM_D);
	BOOST_CHECK_EQUAL(*n, 0x0073);
	BOOST_CHECK_EQUAL(*++n, 0x0323);
	BOOST_CHECK_EQUAL(*++n, 0x0307);
	BOOST_CHECK(!(++n).hasNext());
	BOOST_CHECK_EQUAL(*--n, 0x0307);
	BOOST_CHECK_EQUAL(*--n, 0x0323);
	BOOST_CHECK_EQUAL(*--n, 0x0073);
	BOOST_CHECK(!n.hasPrevious());

	const ascension::String source2(L"s\x0307\x0323");
	ascension::unicode::Normalizer n2(ascension::unicode::StringCharacterIterator(source2), ascension::unicode::Normalizer::FORM_D);
	BOOST_CHECK_EQUAL(*(n++), *(n2++));
	BOOST_CHECK_EQUAL(*(n++), *(n2++));
	BOOST_CHECK_EQUAL(*(n++), *(n2++));
}

int test_main(int, char*[]) {
	testNormalizer();
	return 0;
}
