#define BOOST_TEST_MODULE encoder_test
#include <boost/test/included/unit_test.hpp>

#include <ascension/corelib/encoding/encoder.hpp>
#include <ascension/corelib/encoding/encoder-factory.hpp>
#include <ascension/corelib/encoding/encoding-detector.hpp>

BOOST_AUTO_TEST_CASE(charset_alias_matching_test) {
	// from http://www.unicode.org/reports/tr22/#Charset_Alias_Matching
	static const std::string UTF_8("UTF-8"), UTF8("utf8"), UTF_008("u.t.f-008");
	static const std::string UTF_80("utf-80"), UT8("ut8");

	BOOST_TEST(ascension::encoding::compareEncodingNames(UTF_8, UTF8) == 0);
	BOOST_TEST(ascension::encoding::compareEncodingNames(UTF_8, UTF_008) == 0);
	BOOST_TEST(ascension::encoding::compareEncodingNames(UTF8, UTF_008) == 0);

	BOOST_TEST(ascension::encoding::compareEncodingNames(UTF_80, UTF_8) != 0);
	BOOST_TEST(ascension::encoding::compareEncodingNames(UTF_80, UTF8) != 0);
	BOOST_TEST(ascension::encoding::compareEncodingNames(UTF_80, UTF_008) != 0);
	BOOST_TEST(ascension::encoding::compareEncodingNames(UT8, UTF_8) != 0);
	BOOST_TEST(ascension::encoding::compareEncodingNames(UT8, UTF8) != 0);
	BOOST_TEST(ascension::encoding::compareEncodingNames(UT8, UTF_008) != 0);
}

BOOST_AUTO_TEST_CASE(minimum_factory_test) {
	namespace e = ascension::encoding;
	BOOST_TEST(e::EncoderRegistry::instance().supports(e::fundamental::US_ASCII));
	BOOST_TEST(e::EncoderRegistry::instance().supports(e::fundamental::ISO_8859_1));
	BOOST_TEST(e::EncoderRegistry::instance().supports(e::fundamental::UTF_8));
	BOOST_TEST(e::EncoderRegistry::instance().supports(e::fundamental::UTF_16BE));
	BOOST_TEST(e::EncoderRegistry::instance().supports(e::fundamental::UTF_16LE));
	BOOST_TEST(e::EncoderRegistry::instance().supports(e::fundamental::UTF_16));

	auto encoder(e::EncoderRegistry::instance().forMIB(e::fundamental::US_ASCII));
	BOOST_REQUIRE(encoder.get() != nullptr);
	BOOST_TEST(encoder->properties().mibEnum() == e::fundamental::US_ASCII);
}
