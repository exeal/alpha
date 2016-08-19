#define BOOST_TEST_MODULE character_test
#include <boost/test/included/unit_test.hpp>

#include <ascension/corelib/text/code-point.hpp>
#include "unicode-surrogates.hpp"

BOOST_AUTO_TEST_CASE(surrogate_category_test) {
	BOOST_TEST(!ascension::text::surrogates::isSupplemental(0x0000u));
	BOOST_TEST(!ascension::text::surrogates::isSupplemental(NON_PRIVATE_USE_HIGH_SURROGATE_FIRST));
	BOOST_TEST(!ascension::text::surrogates::isSupplemental(NON_PRIVATE_USE_HIGH_SURROGATE_LAST));
	BOOST_TEST(!ascension::text::surrogates::isSupplemental(PRIVATE_USE_HIGH_SURROGATE_FIRST));
	BOOST_TEST(!ascension::text::surrogates::isSupplemental(PRIVATE_USE_HIGH_SURROGATE_LAST));
	BOOST_TEST(!ascension::text::surrogates::isSupplemental(LOW_SURROGATE_FIRST));
	BOOST_TEST(!ascension::text::surrogates::isSupplemental(LOW_SURROGATE_LAST));
	BOOST_TEST(!ascension::text::surrogates::isSupplemental(0xffffu));
	BOOST_TEST( ascension::text::surrogates::isSupplemental(0x10000u));
	BOOST_TEST( ascension::text::surrogates::isSupplemental(0x10ffffu));
	BOOST_TEST( ascension::text::surrogates::isSupplemental(0xffffffu));

	BOOST_TEST(!ascension::text::surrogates::isHighSurrogate(0x0000u));
	BOOST_TEST( ascension::text::surrogates::isHighSurrogate(NON_PRIVATE_USE_HIGH_SURROGATE_FIRST));
	BOOST_TEST( ascension::text::surrogates::isHighSurrogate(NON_PRIVATE_USE_HIGH_SURROGATE_LAST));
	BOOST_TEST( ascension::text::surrogates::isHighSurrogate(PRIVATE_USE_HIGH_SURROGATE_FIRST));
	BOOST_TEST( ascension::text::surrogates::isHighSurrogate(PRIVATE_USE_HIGH_SURROGATE_LAST));
	BOOST_TEST(!ascension::text::surrogates::isHighSurrogate(LOW_SURROGATE_FIRST));
	BOOST_TEST(!ascension::text::surrogates::isHighSurrogate(LOW_SURROGATE_LAST));
	BOOST_TEST(!ascension::text::surrogates::isHighSurrogate(0xffffu));
	BOOST_TEST(!ascension::text::surrogates::isHighSurrogate(0x10000u));
	BOOST_TEST(!ascension::text::surrogates::isHighSurrogate(0x10ffffu));
	BOOST_TEST(!ascension::text::surrogates::isHighSurrogate(0xffffffu));

	BOOST_TEST(!ascension::text::surrogates::isLowSurrogate(0x0000u));
	BOOST_TEST(!ascension::text::surrogates::isLowSurrogate(NON_PRIVATE_USE_HIGH_SURROGATE_FIRST));
	BOOST_TEST(!ascension::text::surrogates::isLowSurrogate(NON_PRIVATE_USE_HIGH_SURROGATE_LAST));
	BOOST_TEST(!ascension::text::surrogates::isLowSurrogate(PRIVATE_USE_HIGH_SURROGATE_FIRST));
	BOOST_TEST(!ascension::text::surrogates::isLowSurrogate(PRIVATE_USE_HIGH_SURROGATE_LAST));
	BOOST_TEST( ascension::text::surrogates::isLowSurrogate(LOW_SURROGATE_FIRST));
	BOOST_TEST( ascension::text::surrogates::isLowSurrogate(LOW_SURROGATE_LAST));
	BOOST_TEST(!ascension::text::surrogates::isLowSurrogate(0xffffu));
	BOOST_TEST(!ascension::text::surrogates::isLowSurrogate(0x10000u));
	BOOST_TEST(!ascension::text::surrogates::isLowSurrogate(0x10ffffu));
	BOOST_TEST(!ascension::text::surrogates::isLowSurrogate(0xffffffu));

	BOOST_TEST(!ascension::text::surrogates::isSurrogate(0x0000u));
	BOOST_TEST( ascension::text::surrogates::isSurrogate(NON_PRIVATE_USE_HIGH_SURROGATE_FIRST));
	BOOST_TEST( ascension::text::surrogates::isSurrogate(NON_PRIVATE_USE_HIGH_SURROGATE_LAST));
	BOOST_TEST( ascension::text::surrogates::isSurrogate(PRIVATE_USE_HIGH_SURROGATE_FIRST));
	BOOST_TEST( ascension::text::surrogates::isSurrogate(PRIVATE_USE_HIGH_SURROGATE_LAST));
	BOOST_TEST( ascension::text::surrogates::isSurrogate(LOW_SURROGATE_FIRST));
	BOOST_TEST( ascension::text::surrogates::isSurrogate(LOW_SURROGATE_LAST));
	BOOST_TEST(!ascension::text::surrogates::isSurrogate(0xffffu));
	BOOST_TEST(!ascension::text::surrogates::isSurrogate(0x10000u));
	BOOST_TEST(!ascension::text::surrogates::isSurrogate(0x10ffffu));
	BOOST_TEST(!ascension::text::surrogates::isSurrogate(0xffffffu));

	BOOST_TEST(ascension::text::surrogates::highSurrogate(0x10000u) == NON_PRIVATE_USE_HIGH_SURROGATE_FIRST);
	BOOST_TEST(ascension::text::surrogates::lowSurrogate(0x10000u) == LOW_SURROGATE_FIRST);
	BOOST_TEST(ascension::text::surrogates::highSurrogate(0xeffffu) == NON_PRIVATE_USE_HIGH_SURROGATE_LAST);
	BOOST_TEST(ascension::text::surrogates::lowSurrogate(0xeffffu) == LOW_SURROGATE_LAST);
	BOOST_TEST(ascension::text::surrogates::highSurrogate(0xf0000u) == PRIVATE_USE_HIGH_SURROGATE_FIRST);
	BOOST_TEST(ascension::text::surrogates::lowSurrogate(0xf0000u) == LOW_SURROGATE_FIRST);
	BOOST_TEST(ascension::text::surrogates::highSurrogate(0x10ffffu) == PRIVATE_USE_HIGH_SURROGATE_LAST);
	BOOST_TEST(ascension::text::surrogates::lowSurrogate(0x10ffffu) == LOW_SURROGATE_LAST);
}

BOOST_AUTO_TEST_CASE(surrogate_decode_test) {
	BOOST_TEST(ascension::text::surrogates::decode(NON_PRIVATE_USE_HIGH_SURROGATE_FIRST, LOW_SURROGATE_FIRST) == 0x10000u);
	BOOST_TEST(ascension::text::surrogates::decode(NON_PRIVATE_USE_HIGH_SURROGATE_LAST, LOW_SURROGATE_LAST) == 0xeffffu);
	BOOST_TEST(ascension::text::surrogates::decode(PRIVATE_USE_HIGH_SURROGATE_FIRST, LOW_SURROGATE_FIRST) == 0xf0000u);
	BOOST_TEST(ascension::text::surrogates::decode(PRIVATE_USE_HIGH_SURROGATE_LAST, LOW_SURROGATE_LAST) == 0x10ffffu);

	BOOST_TEST(ascension::text::surrogates::checkedDecode(NON_PRIVATE_USE_HIGH_SURROGATE_FIRST, LOW_SURROGATE_FIRST) == 0x10000u);
	BOOST_TEST(ascension::text::surrogates::checkedDecode(NON_PRIVATE_USE_HIGH_SURROGATE_LAST, LOW_SURROGATE_LAST) == 0xeffffu);
	BOOST_TEST(ascension::text::surrogates::checkedDecode(PRIVATE_USE_HIGH_SURROGATE_FIRST, LOW_SURROGATE_FIRST) == 0xf0000u);
	BOOST_TEST(ascension::text::surrogates::checkedDecode(PRIVATE_USE_HIGH_SURROGATE_LAST, LOW_SURROGATE_LAST) == 0x10ffffu);
}

BOOST_AUTO_TEST_CASE(code_point_category_test) {
	BOOST_TEST( ascension::text::isValidCodePoint(0x0000u));
	BOOST_TEST( ascension::text::isValidCodePoint(NON_PRIVATE_USE_HIGH_SURROGATE_FIRST));
	BOOST_TEST( ascension::text::isValidCodePoint(NON_PRIVATE_USE_HIGH_SURROGATE_LAST));
	BOOST_TEST( ascension::text::isValidCodePoint(PRIVATE_USE_HIGH_SURROGATE_FIRST));
	BOOST_TEST( ascension::text::isValidCodePoint(PRIVATE_USE_HIGH_SURROGATE_LAST));
	BOOST_TEST( ascension::text::isValidCodePoint(LOW_SURROGATE_FIRST));
	BOOST_TEST( ascension::text::isValidCodePoint(LOW_SURROGATE_LAST));
	BOOST_TEST( ascension::text::isValidCodePoint(0xffffu));
	BOOST_TEST( ascension::text::isValidCodePoint(0x10000u));
	BOOST_TEST( ascension::text::isValidCodePoint(0x10ffffu));
	BOOST_TEST(!ascension::text::isValidCodePoint(0xffffffu));

	BOOST_TEST( ascension::text::isScalarValue(0x0000u));
	BOOST_TEST(!ascension::text::isScalarValue(NON_PRIVATE_USE_HIGH_SURROGATE_FIRST));
	BOOST_TEST(!ascension::text::isScalarValue(NON_PRIVATE_USE_HIGH_SURROGATE_LAST));
	BOOST_TEST(!ascension::text::isScalarValue(PRIVATE_USE_HIGH_SURROGATE_FIRST));
	BOOST_TEST(!ascension::text::isScalarValue(PRIVATE_USE_HIGH_SURROGATE_LAST));
	BOOST_TEST(!ascension::text::isScalarValue(LOW_SURROGATE_FIRST));
	BOOST_TEST(!ascension::text::isScalarValue(LOW_SURROGATE_LAST));
	BOOST_TEST( ascension::text::isScalarValue(0xffffu));
	BOOST_TEST( ascension::text::isScalarValue(0x10000u));
	BOOST_TEST( ascension::text::isScalarValue(0x10ffffu));
	BOOST_TEST(!ascension::text::isValidCodePoint(0xffffffu));
}
