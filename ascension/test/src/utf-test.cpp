#define BOOST_TEST_MODULE utf_test
#include <boost/test/included/unit_test.hpp>

#include <ascension/corelib/text/utf.hpp>
#include <boost/range/iterator_range_core.hpp>	// boost.make_iterator_range_n
#include <vector>
#include "unicode-string-sample.hpp"
#include "unicode-surrogates.hpp"

BOOST_AUTO_TEST_CASE(trivial_test) {
	BOOST_TEST(ascension::text::utf::numberOfEncodedBytes<1>(0x0000u) == 1u);
	BOOST_TEST(ascension::text::utf::numberOfEncodedBytes<1>(0x007fu) == 1u);
	BOOST_TEST(ascension::text::utf::numberOfEncodedBytes<1>(0x0080u) == 2u);
	BOOST_TEST(ascension::text::utf::numberOfEncodedBytes<1>(0x07ffu) == 2u);
	BOOST_TEST(ascension::text::utf::numberOfEncodedBytes<1>(0x0800u) == 3u);
	BOOST_CHECK_THROW(ascension::text::utf::numberOfEncodedBytes<1>(NON_PRIVATE_USE_HIGH_SURROGATE_FIRST), ascension::text::InvalidScalarValueException);
	BOOST_CHECK_THROW(ascension::text::utf::numberOfEncodedBytes<1>(NON_PRIVATE_USE_HIGH_SURROGATE_LAST), ascension::text::InvalidScalarValueException);
	BOOST_CHECK_THROW(ascension::text::utf::numberOfEncodedBytes<1>(PRIVATE_USE_HIGH_SURROGATE_FIRST), ascension::text::InvalidScalarValueException);
	BOOST_CHECK_THROW(ascension::text::utf::numberOfEncodedBytes<1>(PRIVATE_USE_HIGH_SURROGATE_LAST), ascension::text::InvalidScalarValueException);
	BOOST_CHECK_THROW(ascension::text::utf::numberOfEncodedBytes<1>(LOW_SURROGATE_FIRST), ascension::text::InvalidScalarValueException);
	BOOST_CHECK_THROW(ascension::text::utf::numberOfEncodedBytes<1>(LOW_SURROGATE_LAST), ascension::text::InvalidScalarValueException);
	BOOST_TEST(ascension::text::utf::numberOfEncodedBytes<1>(0xffffu) == 3u);
	BOOST_TEST(ascension::text::utf::numberOfEncodedBytes<1>(0x10000u) == 4u);
	BOOST_TEST(ascension::text::utf::numberOfEncodedBytes<1>(0x10ffffu) == 4u);
	BOOST_CHECK_THROW(ascension::text::utf::numberOfEncodedBytes<1>(0x110000u), ascension::text::InvalidScalarValueException);

	BOOST_TEST(ascension::text::utf::numberOfEncodedBytes<2>(0x0000u) == 1u);
	BOOST_TEST(ascension::text::utf::numberOfEncodedBytes<2>(0xffffu) == 1u);
	BOOST_CHECK_THROW(ascension::text::utf::numberOfEncodedBytes<2>(NON_PRIVATE_USE_HIGH_SURROGATE_FIRST), ascension::text::InvalidScalarValueException);
	BOOST_CHECK_THROW(ascension::text::utf::numberOfEncodedBytes<2>(NON_PRIVATE_USE_HIGH_SURROGATE_LAST), ascension::text::InvalidScalarValueException);
	BOOST_CHECK_THROW(ascension::text::utf::numberOfEncodedBytes<2>(PRIVATE_USE_HIGH_SURROGATE_FIRST), ascension::text::InvalidScalarValueException);
	BOOST_CHECK_THROW(ascension::text::utf::numberOfEncodedBytes<2>(PRIVATE_USE_HIGH_SURROGATE_LAST), ascension::text::InvalidScalarValueException);
	BOOST_CHECK_THROW(ascension::text::utf::numberOfEncodedBytes<2>(LOW_SURROGATE_FIRST), ascension::text::InvalidScalarValueException);
	BOOST_CHECK_THROW(ascension::text::utf::numberOfEncodedBytes<2>(LOW_SURROGATE_LAST), ascension::text::InvalidScalarValueException);
	BOOST_TEST(ascension::text::utf::numberOfEncodedBytes<2>(0x10000u) == 2u);
	BOOST_TEST(ascension::text::utf::numberOfEncodedBytes<2>(0x10ffffu) == 2u);
	BOOST_CHECK_THROW(ascension::text::utf::numberOfEncodedBytes<2>(0x110000u), ascension::text::InvalidScalarValueException);

	BOOST_TEST(ascension::text::utf::numberOfEncodedBytes<4>(0x0000u) == 1u);
	BOOST_CHECK_THROW(ascension::text::utf::numberOfEncodedBytes<4>(NON_PRIVATE_USE_HIGH_SURROGATE_FIRST), ascension::text::InvalidScalarValueException);
	BOOST_CHECK_THROW(ascension::text::utf::numberOfEncodedBytes<4>(NON_PRIVATE_USE_HIGH_SURROGATE_LAST), ascension::text::InvalidScalarValueException);
	BOOST_CHECK_THROW(ascension::text::utf::numberOfEncodedBytes<4>(PRIVATE_USE_HIGH_SURROGATE_FIRST), ascension::text::InvalidScalarValueException);
	BOOST_CHECK_THROW(ascension::text::utf::numberOfEncodedBytes<4>(PRIVATE_USE_HIGH_SURROGATE_LAST), ascension::text::InvalidScalarValueException);
	BOOST_CHECK_THROW(ascension::text::utf::numberOfEncodedBytes<4>(LOW_SURROGATE_FIRST), ascension::text::InvalidScalarValueException);
	BOOST_CHECK_THROW(ascension::text::utf::numberOfEncodedBytes<4>(LOW_SURROGATE_LAST), ascension::text::InvalidScalarValueException);
	BOOST_TEST(ascension::text::utf::numberOfEncodedBytes<4>(0x10ffffu) == 1u);
	BOOST_CHECK_THROW(ascension::text::utf::numberOfEncodedBytes<4>(0x110000u), ascension::text::InvalidScalarValueException);
}

BOOST_AUTO_TEST_SUITE(utf8_trivials)
	BOOST_AUTO_TEST_CASE(class_test) {
		BOOST_TEST( ascension::text::utf::isValidByte(0x00));
		BOOST_TEST( ascension::text::utf::isValidByte(0xbf));
		BOOST_TEST(!ascension::text::utf::isValidByte(0xc0));
		BOOST_TEST(!ascension::text::utf::isValidByte(0xc1));
		BOOST_TEST( ascension::text::utf::isValidByte(0xc2));
		BOOST_TEST(!ascension::text::utf::isValidByte(0xf5));
		BOOST_TEST(!ascension::text::utf::isValidByte(0xff));

		BOOST_TEST( ascension::text::utf::isSingleByte(0x00));
		BOOST_TEST( ascension::text::utf::isSingleByte(0x7f));
		BOOST_TEST(!ascension::text::utf::isSingleByte(0x80));
		BOOST_TEST(!ascension::text::utf::isSingleByte(0xff));

		BOOST_TEST( ascension::text::utf::isLeadingByte(0x00));
		BOOST_TEST( ascension::text::utf::isLeadingByte(0x7f));
		BOOST_TEST(!ascension::text::utf::isLeadingByte(0x80));
		BOOST_TEST(!ascension::text::utf::isLeadingByte(0xc1));
		BOOST_TEST( ascension::text::utf::isLeadingByte(0xc2));
		BOOST_TEST( ascension::text::utf::isLeadingByte(0xf4));
		BOOST_TEST(!ascension::text::utf::isLeadingByte(0xf5));
		BOOST_TEST(!ascension::text::utf::isLeadingByte(0xff));

		BOOST_TEST(!ascension::text::utf::maybeTrailingByte(0x00));
		BOOST_TEST(!ascension::text::utf::maybeTrailingByte(0x7f));
		BOOST_TEST( ascension::text::utf::maybeTrailingByte(0x80));
		BOOST_TEST( ascension::text::utf::maybeTrailingByte(0xbf));
		BOOST_TEST(!ascension::text::utf::maybeTrailingByte(0xc0));
		BOOST_TEST(!ascension::text::utf::maybeTrailingByte(0xff));
	}

	BOOST_AUTO_TEST_CASE(length_test) {
		BOOST_TEST(ascension::text::utf::length(0x00) == 1u);
		BOOST_TEST(ascension::text::utf::length(0x7f) == 1u);
		BOOST_TEST(ascension::text::utf::length(0x80) == 0u);
		BOOST_TEST(ascension::text::utf::length(0xc1) == 0u);
		BOOST_TEST(ascension::text::utf::length(0xc2) == 2u);
		BOOST_TEST(ascension::text::utf::length(0xdf) == 2u);
		BOOST_TEST(ascension::text::utf::length(0xe0) == 3u);
		BOOST_TEST(ascension::text::utf::length(0xef) == 3u);
		BOOST_TEST(ascension::text::utf::length(0xf0) == 4u);
		BOOST_TEST(ascension::text::utf::length(0xf4) == 4u);
		BOOST_TEST(ascension::text::utf::length(0xf5) == 0u);
		BOOST_TEST(ascension::text::utf::length(0xff) == 0u);

		BOOST_TEST(ascension::text::utf::numberOfTrailingBytes(0x00) == 0u);
		BOOST_TEST(ascension::text::utf::numberOfTrailingBytes(0x7f) == 0u);
		BOOST_TEST(ascension::text::utf::numberOfTrailingBytes(0x80) == static_cast<std::size_t>(-1));
		BOOST_TEST(ascension::text::utf::numberOfTrailingBytes(0xc1) == static_cast<std::size_t>(-1));
		BOOST_TEST(ascension::text::utf::numberOfTrailingBytes(0xc2) == 1u);
		BOOST_TEST(ascension::text::utf::numberOfTrailingBytes(0xdf) == 1u);
		BOOST_TEST(ascension::text::utf::numberOfTrailingBytes(0xe0) == 2u);
		BOOST_TEST(ascension::text::utf::numberOfTrailingBytes(0xef) == 2u);
		BOOST_TEST(ascension::text::utf::numberOfTrailingBytes(0xf0) == 3u);
		BOOST_TEST(ascension::text::utf::numberOfTrailingBytes(0xf4) == 3u);
		BOOST_TEST(ascension::text::utf::numberOfTrailingBytes(0xf5) == static_cast<std::size_t>(-1));
		BOOST_TEST(ascension::text::utf::numberOfTrailingBytes(0xff) == static_cast<std::size_t>(-1));
	}
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(decode)
	BOOST_AUTO_TEST_CASE(utf8_decode_test) {
		BOOST_TEST(ascension::text::utf::decodeFirst(SPOT8_IN_UTF8) == SPOT8[0]);
		BOOST_TEST(ascension::text::utf::decodeFirst(boost::make_iterator_range_n(SPOT8_IN_UTF8 + 1, 9)) == SPOT8[1]);
		BOOST_TEST(ascension::text::utf::decodeFirst(boost::make_iterator_range_n(SPOT8_IN_UTF8 + 3, 7)) == SPOT8[2]);
		BOOST_TEST(ascension::text::utf::decodeFirst(boost::make_iterator_range_n(SPOT8_IN_UTF8 + 6, 4)) == SPOT8[3]);
/*
		BOOST_TEST(ascension::text::utf::decodeLast(SPOT8_IN_UTF8) == SPOT8[3]);
		BOOST_TEST(ascension::text::utf::decodeLast(boost::make_iterator_range_n(SPOT8_IN_UTF8, 6)) == SPOT8[2]);
		BOOST_TEST(ascension::text::utf::decodeLast(boost::make_iterator_range_n(SPOT8_IN_UTF8, 3)) == SPOT8[1]);
		BOOST_TEST(ascension::text::utf::decodeLast(boost::make_iterator_range_n(SPOT8_IN_UTF8, 1)) == SPOT8[0]);
*/	}

	BOOST_AUTO_TEST_CASE(utf16_decode_test) {
		BOOST_TEST(ascension::text::utf::decodeFirst(SPOT16_IN_UTF16) == SPOT16[0]);

//		BOOST_TEST(ascension::text::utf::decodeLast(SPOT16_IN_UTF16) == SPOT16[0]);
	}

	BOOST_AUTO_TEST_CASE(utf32_decode_test) {
		BOOST_TEST(ascension::text::utf::decodeFirst(SPOT8) == SPOT8[0]);
		BOOST_TEST(ascension::text::utf::decodeFirst(boost::make_iterator_range_n(SPOT8 + 1, 3)) == SPOT8[1]);
		BOOST_TEST(ascension::text::utf::decodeFirst(boost::make_iterator_range_n(SPOT8 + 2, 2)) == SPOT8[2]);
		BOOST_TEST(ascension::text::utf::decodeFirst(boost::make_iterator_range_n(SPOT8 + 3, 1)) == SPOT8[3]);
		BOOST_TEST(ascension::text::utf::decodeFirst(SPOT16) == SPOT16[0]);

		BOOST_TEST(ascension::text::utf::decodeLast(SPOT8) == SPOT8[3]);
		BOOST_TEST(ascension::text::utf::decodeLast(boost::make_iterator_range_n(SPOT8, 3)) == SPOT8[2]);
		BOOST_TEST(ascension::text::utf::decodeLast(boost::make_iterator_range_n(SPOT8, 2)) == SPOT8[1]);
		BOOST_TEST(ascension::text::utf::decodeLast(boost::make_iterator_range_n(SPOT8, 1)) == SPOT8[0]);
		BOOST_TEST(ascension::text::utf::decodeLast(SPOT16) == SPOT16[0]);
	}
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(encode)
	BOOST_AUTO_TEST_CASE(utf8_encode_test) {
		std::vector<std::uint8_t> buffer;
		auto inserter(std::back_inserter(buffer));
		ascension::text::utf::encode(SPOT8[0], inserter);
		ascension::text::utf::encode(SPOT8[1], inserter);
		ascension::text::utf::encode(SPOT8[2], inserter);
		ascension::text::utf::encode(SPOT8[3], inserter);
		BOOST_CHECK_EQUAL_COLLECTIONS(buffer.cbegin(), buffer.cend(), SPOT8_IN_UTF8, SPOT8_IN_UTF8 + 10);
	}

	BOOST_AUTO_TEST_CASE(utf16_encode_test) {
		std::vector<std::uint16_t> buffer;
		auto inserter(std::back_inserter(buffer));
		ascension::text::utf::encode(SPOT16[0], inserter);
		BOOST_CHECK_EQUAL_COLLECTIONS(buffer.cbegin(), buffer.cend(), SPOT16_IN_UTF16, SPOT16_IN_UTF16 + 2);
	}

	BOOST_AUTO_TEST_CASE(utf32_encode_test) {
		std::vector<std::uint32_t> buffer;
		auto inserter(std::back_inserter(buffer));
		ascension::text::utf::encode(SPOT8[0], inserter);
		ascension::text::utf::encode(SPOT8[1], inserter);
		ascension::text::utf::encode(SPOT8[2], inserter);
		ascension::text::utf::encode(SPOT8[3], inserter);
		BOOST_CHECK_EQUAL_COLLECTIONS(buffer.cbegin(), buffer.cend(), SPOT8, SPOT8 + 4);
	}
BOOST_AUTO_TEST_SUITE_END()
