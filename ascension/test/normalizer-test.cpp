// unicode-iterator-test.cpp

#include "../unicode.hpp"
#include <boost/test/included/test_exec_monitor.hpp>
namespace a = ascension;
namespace t = ascension::text;

namespace {
	void testCompare() {
		BOOST_CHECK_EQUAL(t::Normalizer::compare(L"", L"", t::CASE_SENSITIVE), 0);
		BOOST_CHECK_EQUAL(t::Normalizer::compare(L"abc", L"abc", t::CASE_SENSITIVE), 0);
		BOOST_CHECK_EQUAL(t::Normalizer::compare(L"C\x0301\x0327", L"C\x0327\x0301", t::CASE_SENSITIVE), 0);
//		BOOST_CHECK_EQUAL(t::Normalizer::compare(L"\x1E69", L"s\x0323\x0307", t::CASE_SENSITIVE), 0);
	}
	void testNormalize() {
	}
} // namespace @0

void testNormalizer() {
	testCompare();
	testNormalize();

	const a::String source(L"\x1E69");
	t::Normalizer n(t::StringCharacterIterator(source), t::Normalizer::FORM_D);
	BOOST_CHECK_EQUAL(*n, 0x0073);
	BOOST_CHECK_EQUAL(*++n, 0x0323);
	BOOST_CHECK_EQUAL(*++n, 0x0307);
	BOOST_CHECK(!(++n).hasNext());
	BOOST_CHECK_EQUAL(*--n, 0x0307);
	BOOST_CHECK_EQUAL(*--n, 0x0323);
	BOOST_CHECK_EQUAL(*--n, 0x0073);
	BOOST_CHECK(!n.hasPrevious());

	const a::String source2(L"s\x0307\x0323");
	t::Normalizer n2(t::StringCharacterIterator(source2), t::Normalizer::FORM_D);
	BOOST_CHECK_EQUAL(*(n++), *(n2++));
	BOOST_CHECK_EQUAL(*(n++), *(n2++));
	BOOST_CHECK_EQUAL(*(n++), *(n2++));
}

int test_main(int, char*[]) {
	testNormalizer();
	return 0;
}
