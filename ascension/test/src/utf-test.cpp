#define BOOST_TEST_MODULE utf_test
#include <boost/test/included/unit_test.hpp>

#include <ascension/corelib/text/utf.hpp>
#include <boost/optional/optional_io.hpp>
#include <boost/range/iterator_range_core.hpp>	// boost.make_iterator_range_n
#include <vector>
#include "unicode-string-sample.hpp"
#include "unicode-surrogates.hpp"


BOOST_AUTO_TEST_CASE(trivial_test) {
	BOOST_TEST(ascension::text::utf::CodeUnitTraits<1>::length(0x0000u) == 1u);
	BOOST_TEST(ascension::text::utf::CodeUnitTraits<1>::length(0x007fu) == 1u);
	BOOST_TEST(ascension::text::utf::CodeUnitTraits<1>::length(0x0080u) == 2u);
	BOOST_TEST(ascension::text::utf::CodeUnitTraits<1>::length(0x07ffu) == 2u);
	BOOST_TEST(ascension::text::utf::CodeUnitTraits<1>::length(0x0800u) == 3u);
	BOOST_CHECK_THROW(ascension::text::utf::CodeUnitTraits<1>::length(NON_PRIVATE_USE_HIGH_SURROGATE_FIRST), ascension::text::InvalidScalarValueException);
	BOOST_CHECK_THROW(ascension::text::utf::CodeUnitTraits<1>::length(NON_PRIVATE_USE_HIGH_SURROGATE_LAST), ascension::text::InvalidScalarValueException);
	BOOST_CHECK_THROW(ascension::text::utf::CodeUnitTraits<1>::length(PRIVATE_USE_HIGH_SURROGATE_FIRST), ascension::text::InvalidScalarValueException);
	BOOST_CHECK_THROW(ascension::text::utf::CodeUnitTraits<1>::length(PRIVATE_USE_HIGH_SURROGATE_LAST), ascension::text::InvalidScalarValueException);
	BOOST_CHECK_THROW(ascension::text::utf::CodeUnitTraits<1>::length(LOW_SURROGATE_FIRST), ascension::text::InvalidScalarValueException);
	BOOST_CHECK_THROW(ascension::text::utf::CodeUnitTraits<1>::length(LOW_SURROGATE_LAST), ascension::text::InvalidScalarValueException);
	BOOST_TEST(ascension::text::utf::CodeUnitTraits<1>::length(0xffffu) == 3u);
	BOOST_TEST(ascension::text::utf::CodeUnitTraits<1>::length(0x10000u) == 4u);
	BOOST_TEST(ascension::text::utf::CodeUnitTraits<1>::length(0x10ffffu) == 4u);
	BOOST_CHECK_THROW(ascension::text::utf::CodeUnitTraits<1>::length(0x110000u), ascension::text::InvalidScalarValueException);

	BOOST_TEST(ascension::text::utf::CodeUnitTraits<2>::length(0x0000u) == 1u);
	BOOST_TEST(ascension::text::utf::CodeUnitTraits<2>::length(0xffffu) == 1u);
	BOOST_CHECK_THROW(ascension::text::utf::CodeUnitTraits<2>::length(NON_PRIVATE_USE_HIGH_SURROGATE_FIRST), ascension::text::InvalidScalarValueException);
	BOOST_CHECK_THROW(ascension::text::utf::CodeUnitTraits<2>::length(NON_PRIVATE_USE_HIGH_SURROGATE_LAST), ascension::text::InvalidScalarValueException);
	BOOST_CHECK_THROW(ascension::text::utf::CodeUnitTraits<2>::length(PRIVATE_USE_HIGH_SURROGATE_FIRST), ascension::text::InvalidScalarValueException);
	BOOST_CHECK_THROW(ascension::text::utf::CodeUnitTraits<2>::length(PRIVATE_USE_HIGH_SURROGATE_LAST), ascension::text::InvalidScalarValueException);
	BOOST_CHECK_THROW(ascension::text::utf::CodeUnitTraits<2>::length(LOW_SURROGATE_FIRST), ascension::text::InvalidScalarValueException);
	BOOST_CHECK_THROW(ascension::text::utf::CodeUnitTraits<2>::length(LOW_SURROGATE_LAST), ascension::text::InvalidScalarValueException);
	BOOST_TEST(ascension::text::utf::CodeUnitTraits<2>::length(0x10000u) == 2u);
	BOOST_TEST(ascension::text::utf::CodeUnitTraits<2>::length(0x10ffffu) == 2u);
	BOOST_CHECK_THROW(ascension::text::utf::CodeUnitTraits<2>::length(0x110000u), ascension::text::InvalidScalarValueException);

	BOOST_TEST(ascension::text::utf::CodeUnitTraits<4>::length(0x0000u) == 1u);
	BOOST_CHECK_THROW(ascension::text::utf::CodeUnitTraits<4>::length(NON_PRIVATE_USE_HIGH_SURROGATE_FIRST), ascension::text::InvalidScalarValueException);
	BOOST_CHECK_THROW(ascension::text::utf::CodeUnitTraits<4>::length(NON_PRIVATE_USE_HIGH_SURROGATE_LAST), ascension::text::InvalidScalarValueException);
	BOOST_CHECK_THROW(ascension::text::utf::CodeUnitTraits<4>::length(PRIVATE_USE_HIGH_SURROGATE_FIRST), ascension::text::InvalidScalarValueException);
	BOOST_CHECK_THROW(ascension::text::utf::CodeUnitTraits<4>::length(PRIVATE_USE_HIGH_SURROGATE_LAST), ascension::text::InvalidScalarValueException);
	BOOST_CHECK_THROW(ascension::text::utf::CodeUnitTraits<4>::length(LOW_SURROGATE_FIRST), ascension::text::InvalidScalarValueException);
	BOOST_CHECK_THROW(ascension::text::utf::CodeUnitTraits<4>::length(LOW_SURROGATE_LAST), ascension::text::InvalidScalarValueException);
	BOOST_TEST(ascension::text::utf::CodeUnitTraits<4>::length(0x10ffffu) == 1u);
	BOOST_CHECK_THROW(ascension::text::utf::CodeUnitTraits<4>::length(0x110000u), ascension::text::InvalidScalarValueException);
}

BOOST_AUTO_TEST_SUITE(utf8_trivials)
	BOOST_AUTO_TEST_CASE(class_test) {
		BOOST_TEST( ascension::text::utf::CodeUnitTraits<1>::isValid(0x00));
		BOOST_TEST( ascension::text::utf::CodeUnitTraits<1>::isValid(0xbf));
		BOOST_TEST(!ascension::text::utf::CodeUnitTraits<1>::isValid(0xc0));
		BOOST_TEST(!ascension::text::utf::CodeUnitTraits<1>::isValid(0xc1));
		BOOST_TEST( ascension::text::utf::CodeUnitTraits<1>::isValid(0xc2));
		BOOST_TEST(!ascension::text::utf::CodeUnitTraits<1>::isValid(0xf5));
		BOOST_TEST(!ascension::text::utf::CodeUnitTraits<1>::isValid(0xff));

		BOOST_TEST( ascension::text::utf::CodeUnitTraits<1>::isSingle(0x00));
		BOOST_TEST( ascension::text::utf::CodeUnitTraits<1>::isSingle(0x7f));
		BOOST_TEST(!ascension::text::utf::CodeUnitTraits<1>::isSingle(0x80));
		BOOST_TEST(!ascension::text::utf::CodeUnitTraits<1>::isSingle(0xff));

		BOOST_TEST( ascension::text::utf::CodeUnitTraits<1>::isLeading(0x00));
		BOOST_TEST( ascension::text::utf::CodeUnitTraits<1>::isLeading(0x7f));
		BOOST_TEST(!ascension::text::utf::CodeUnitTraits<1>::isLeading(0x80));
		BOOST_TEST(!ascension::text::utf::CodeUnitTraits<1>::isLeading(0xc1));
		BOOST_TEST( ascension::text::utf::CodeUnitTraits<1>::isLeading(0xc2));
		BOOST_TEST( ascension::text::utf::CodeUnitTraits<1>::isLeading(0xf4));
		BOOST_TEST(!ascension::text::utf::CodeUnitTraits<1>::isLeading(0xf5));
		BOOST_TEST(!ascension::text::utf::CodeUnitTraits<1>::isLeading(0xff));

		BOOST_TEST(!ascension::text::utf::CodeUnitTraits<1>::maybeTrailing(0x00));
		BOOST_TEST(!ascension::text::utf::CodeUnitTraits<1>::maybeTrailing(0x7f));
		BOOST_TEST( ascension::text::utf::CodeUnitTraits<1>::maybeTrailing(0x80));
		BOOST_TEST( ascension::text::utf::CodeUnitTraits<1>::maybeTrailing(0xbf));
		BOOST_TEST(!ascension::text::utf::CodeUnitTraits<1>::maybeTrailing(0xc0));
		BOOST_TEST(!ascension::text::utf::CodeUnitTraits<1>::maybeTrailing(0xff));
	}

	BOOST_AUTO_TEST_CASE(length_test) {
/*		BOOST_TEST(boost::get_optional_value_or(ascension::text::utf::CodeUnitTraits<1>::lengthForLeading(0x00), 0u) == 1u);
		BOOST_TEST(boost::get_optional_value_or(ascension::text::utf::CodeUnitTraits<1>::lengthForLeading(0x7f), 0u) == 1u);
		BOOST_TEST(boost::get_optional_value_or(ascension::text::utf::CodeUnitTraits<1>::lengthForLeading(0x80), 0u) == 0u);
		BOOST_TEST(boost::get_optional_value_or(ascension::text::utf::CodeUnitTraits<1>::lengthForLeading(0xc1), 0u) == 0u);
		BOOST_TEST(boost::get_optional_value_or(ascension::text::utf::CodeUnitTraits<1>::lengthForLeading(0xc2), 0u) == 2u);
		BOOST_TEST(boost::get_optional_value_or(ascension::text::utf::CodeUnitTraits<1>::lengthForLeading(0xdf), 0u) == 2u);
		BOOST_TEST(boost::get_optional_value_or(ascension::text::utf::CodeUnitTraits<1>::lengthForLeading(0xe0), 0u) == 3u);
		BOOST_TEST(boost::get_optional_value_or(ascension::text::utf::CodeUnitTraits<1>::lengthForLeading(0xef), 0u) == 3u);
		BOOST_TEST(boost::get_optional_value_or(ascension::text::utf::CodeUnitTraits<1>::lengthForLeading(0xf0), 0u) == 4u);
		BOOST_TEST(boost::get_optional_value_or(ascension::text::utf::CodeUnitTraits<1>::lengthForLeading(0xf4), 0u) == 4u);
		BOOST_TEST(boost::get_optional_value_or(ascension::text::utf::CodeUnitTraits<1>::lengthForLeading(0xf5), 0u) == 0u);
		BOOST_TEST(boost::get_optional_value_or(ascension::text::utf::CodeUnitTraits<1>::lengthForLeading(0xff), 0u) == 0u);

		BOOST_TEST(boost::get_optional_value_or(ascension::text::utf::CodeUnitTraits<1>::trailingLengthForLeading(0x00), 0u) == 0u);
		BOOST_TEST(boost::get_optional_value_or(ascension::text::utf::CodeUnitTraits<1>::trailingLengthForLeading(0x7f), 0u) == 0u);
		BOOST_TEST(boost::get_optional_value_or(ascension::text::utf::CodeUnitTraits<1>::trailingLengthForLeading(0x80), 0u) == static_cast<std::size_t>(-1));
		BOOST_TEST(boost::get_optional_value_or(ascension::text::utf::CodeUnitTraits<1>::trailingLengthForLeading(0xc1), 0u) == static_cast<std::size_t>(-1));
		BOOST_TEST(boost::get_optional_value_or(ascension::text::utf::CodeUnitTraits<1>::trailingLengthForLeading(0xc2), 0u) == 1u);
		BOOST_TEST(boost::get_optional_value_or(ascension::text::utf::CodeUnitTraits<1>::trailingLengthForLeading(0xdf), 0u) == 1u);
		BOOST_TEST(boost::get_optional_value_or(ascension::text::utf::CodeUnitTraits<1>::trailingLengthForLeading(0xe0), 0u) == 2u);
		BOOST_TEST(boost::get_optional_value_or(ascension::text::utf::CodeUnitTraits<1>::trailingLengthForLeading(0xef), 0u) == 2u);
		BOOST_TEST(boost::get_optional_value_or(ascension::text::utf::CodeUnitTraits<1>::trailingLengthForLeading(0xf0), 0u) == 3u);
		BOOST_TEST(boost::get_optional_value_or(ascension::text::utf::CodeUnitTraits<1>::trailingLengthForLeading(0xf4), 0u) == 3u);
		BOOST_TEST(boost::get_optional_value_or(ascension::text::utf::CodeUnitTraits<1>::trailingLengthForLeading(0xf5), 0u) == static_cast<std::size_t>(-1));
		BOOST_TEST(boost::get_optional_value_or(ascension::text::utf::CodeUnitTraits<1>::trailingLengthForLeading(0xff), 0u) == static_cast<std::size_t>(-1));
*/	}
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
/*
BOOST_AUTO_TEST_SUITE(round_trip)
	BOOST_AUTO_TEST_CASE(utf8_round_trip_test) {
		for(ascension::CodePoint c = 0x0000u; c <= 0x10ffffu; ++c) {
			if(ascension::text::isScalarValue(c)) {
				std::vector<std::uint8_t> v;
				auto i(std::back_inserter(v));
				ascension::text::utf::encode(c, i);
				BOOST_TEST(ascension::text::utf::decodeFirst(v.cbegin(), v.cend()) == c);
				if(c % 0x10000 == 0)
					BOOST_TEST_MESSAGE("round_trip/utf8_round_trip_test: Checking for U+" << std::hex << c);
			}
		}
	}

	BOOST_AUTO_TEST_CASE(utf16_round_trip_test) {
		for(ascension::CodePoint c = 0x0000u; c <= 0x10ffffu; ++c) {
			if(ascension::text::isScalarValue(c)) {
				std::vector<std::uint16_t> v;
				auto i(std::back_inserter(v));
				ascension::text::utf::encode(c, i);
				BOOST_TEST(ascension::text::utf::decodeFirst(v.cbegin(), v.cend()) == c);
				if(c % 0x10000 == 0)
					BOOST_TEST_MESSAGE("round_trip/utf16_round_trip_test: Checking for U+" << std::hex << c);
			}
		}
	}

	BOOST_AUTO_TEST_CASE(utf32_round_trip_test) {
		for(ascension::CodePoint c = 0x0000u; c <= 0x10ffffu; ++c) {
			if(ascension::text::isScalarValue(c)) {
				std::vector<std::uint32_t> v;
				auto i(std::back_inserter(v));
				ascension::text::utf::encode(c, i);
				BOOST_TEST(ascension::text::utf::decodeFirst(v.cbegin(), v.cend()) == c);
				if(c % 0x10000 == 0)
					BOOST_TEST_MESSAGE("round_trip/utf32_round_trip_test: Checking for U+" << std::hex << c);
			}
		}
	}
BOOST_AUTO_TEST_SUITE_END()
*/