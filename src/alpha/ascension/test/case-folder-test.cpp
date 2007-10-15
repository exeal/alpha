// case-folder-test.cpp

#include <boost/test/unit_test.hpp>
#include <boost/test/included/unit_test.hpp>
#include "../unicode.hpp"

namespace {
	void testEqual(const ascension::String& s1, const ascension::String& s2) {
		BOOST_CHECK_EQUAL(ascension::unicode::CaseFolder::compare(s1, s2), 0);
		BOOST_CHECK_EQUAL(ascension::unicode::CaseFolder::compare(s2, s1), 0);
		BOOST_CHECK_EQUAL(ascension::unicode::CaseFolder::fold(s1).compare(ascension::unicode::CaseFolder::fold(s2)), 0);
	}
} // namespace @0

void testCaseFolder() {
	testEqual(L"", L"");
	// Turkish I
	BOOST_CHECK_EQUAL(ascension::unicode::CaseFolder::compare(L"Ii", L"\x0131\x0130", true), 0);
	// Latin
	testEqual(L"ABCDEFGHIJKLMNOPQRSTUVWXYZ", L"abcdefghijklmnopqrstuvwxyz");
	testEqual(L"\x00B5\x00C6\x00D0\x00DE\x00DF", L"\x03BC\x00E6\x00F0\x00FESS");
	testEqual(L"\x0130\x0149", L"\x0069\x0307\x02BC\x006E");
	testEqual(L"\x1E90\x1E92\x1E94\x1E96\x1E97\x1E98\x1E99\x1E9A\x1E9B",
		L"\x1E91\x1E93\x1E95\x0068\x0331\x0074\x0308\x0077\x030A\x0079\x030A\x0061\x02BE\x1E61");
	// Greek
	testEqual(L"\x0390\x03B0", L"\x03B9\x0308\x0301\x03C5\x0308\x0301");
	testEqual(L"\x1F50\x1F52\x1F54\x1F56\x1F59\x1F5B\x1F5D\x1F5F\x1F68\x1F69"
		L"\x1F6A\x1F6B\x1F6C\x1F6D\x1F6E\x1F6F\x1F80\x1F81\x1F82\x1F83\x1F84\x1F85\x1F86\x1F87",
		L"\x03C5\x0313\x03C5\x0313\x0300\x03C5\x0313\x0301\x03C5\x0313\x0342\x1F51\x1F53\x1F55"
		L"\x1F57\x1F60\x1F61\x1F62\x1F63\x1F64\x1F65\x1F66\x1F67\x1F00\x03B9\x1F01\x03B9\x1F02"
		L"\x03B9\x1F03\x03B9\x1F04\x03B9\x1F05\x03B9\x1F06\x03B9\x1F07\x03B9");
	// Letterlike symbols
	testEqual(L"\x2126\x212A\x212B\x2132", L"\x03C9\x006B\x00E5\x214E");
	// Latin ligatures
	testEqual(L"\xFB00\xFB01\xFB02\xFB03\xFB04\xFB05\xFB06", L"ffFIflFFIfflSTst");
	// Armenian ligatures
	testEqual(L"\xFB13\xFB14\xFB15\xFB16\xFB17", L"\x0574\x0576\x0574\x0565\x0574\x056B\x057E\x0576\x0574\x056D");
	// Deseret
	testEqual(L"\xD801\xDC00\xD801\xDC01\xD801\xDC02\xD801\xDC03\xD801\xDC24\xD801\xDC25\xD801\xDC26\xD801\xDC27",
		L"\xD801\xDC28\xD801\xDC29\xD801\xDC2A\xD801\xDC2B\xD801\xDC4C\xD801\xDC4D\xD801\xDC4E\xD801\xDC4F");
}

boost::unit_test::test_suite* init_unit_test_suite(int, char*[]) {
	boost::unit_test::test_suite* test = BOOST_TEST_SUITE("Case folder test");
	test->add(BOOST_TEST_CASE(&testCaseFolder));
	return test;
}
