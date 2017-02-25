#define BOOST_TEST_MODULE token_rules_test
#include <boost/test/included/unit_test.hpp>

#include <ascension/corelib/text/identifier-syntax.hpp>
#include <ascension/rules/number-token-rule.hpp>
#include <ascension/rules/regex-token-rule.hpp>
#include <ascension/rules/region-token-rule.hpp>
#include <ascension/rules/uri-token-rule.hpp>
#include <ascension/rules/word-token-rule.hpp>
#include <ascension/rules/word-set-token-rule.hpp>
#include <boost/optional/optional_io.hpp>
#include <boost/range/irange.hpp>
#include "from-latin1.hpp"

namespace {
	const ascension::rules::Token::Identifier DUMMY_ID = 100;	// TODO: Use safe nextIdentifier()...
	const auto& IDS = ascension::text::IdentifierSyntax::defaultInstance();
}

namespace {
	void testNumberTokenRule(const char* text, std::size_t position, boost::optional<std::size_t> expectedLength) {
		const ascension::rules::NumberTokenRule rule(DUMMY_ID);
		const auto s(fromLatin1(text));
		const ascension::StringPiece input(s);
		const auto length(rule.matches(input, input.cbegin() + position, IDS));

		if(expectedLength == boost::none)
			BOOST_TEST(length == boost::none);
		else {
			BOOST_REQUIRE(length != boost::none);
			BOOST_TEST(boost::get(length) == boost::get(expectedLength));
		}
	}
}

BOOST_AUTO_TEST_SUITE(number_token_rule)
	BOOST_AUTO_TEST_CASE(decimal_literal_test1) {
		testNumberTokenRule("0", 0u, 1u);
	}

	BOOST_AUTO_TEST_CASE(decimal_literal_test2) {
		testNumberTokenRule(".", 0u, boost::none);
		testNumberTokenRule(".o", 0u, boost::none);
		testNumberTokenRule(".693147", 0u, 7u);
		testNumberTokenRule(".693147i", 0u, boost::none);
		testNumberTokenRule(".e+1", 0u, boost::none);
		testNumberTokenRule(".ea", 0u, boost::none);
		testNumberTokenRule(".314e1", 0u, 6u);
		testNumberTokenRule(".314e+1", 0u, 7u);
		testNumberTokenRule(".314e-1", 0u, 7u);
		testNumberTokenRule(".314e+-0", 0u, boost::none);
		testNumberTokenRule(".602E+24", 0u, 8u);
//		testNumberTokenRule(".0.0", 0u, boost::none);
		testNumberTokenRule("0.0", 1u, boost::none);
		testNumberTokenRule("a.0", 1u, boost::none);
		testNumberTokenRule("@.0", 1u, 2u);
	}

	BOOST_AUTO_TEST_CASE(hex_integer_literal_test) {
		testNumberTokenRule("0x", 0u, boost::none);
		testNumberTokenRule("0x0", 0u, 3u);
		testNumberTokenRule("0XA", 0u, 3u);
		testNumberTokenRule("0xDEADBEEF", 0u, 10u);
		testNumberTokenRule("0xDEADCODE", 0u, boost::none);
		testNumberTokenRule("0x00e+0", 0u, 5u);
		testNumberTokenRule("0x00.0", 0u, 4u);
		testNumberTokenRule("00x0", 1u, boost::none);
	}
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_CASE(regex_token_rule) {
	// TODO: Write the tests.
}

namespace {
	void testRegionTokenRule(const char* text, std::size_t position, boost::optional<char> escapeCharacter, bool caseSensitive, boost::optional<std::size_t> expectedLength) {
		boost::optional<ascension::Char> ec;
		if(escapeCharacter != boost::none)
			ec = boost::get(escapeCharacter);
		const ascension::rules::RegionTokenRule rule(DUMMY_ID, fromLatin1("begin"), fromLatin1("end"), ec, caseSensitive);
		const auto s(fromLatin1(text));
		const ascension::StringPiece input(s);
		const auto length(rule.matches(input, input.cbegin() + position, IDS));

		if(expectedLength == boost::none)
			BOOST_TEST(length == boost::none);
		else {
			BOOST_REQUIRE(length != boost::none);
			BOOST_TEST(boost::get(length) == boost::get(expectedLength));
		}
	}
}

BOOST_AUTO_TEST_SUITE(region_token_rule)
	BOOST_AUTO_TEST_CASE(basic_test) {
		testRegionTokenRule("----begin++++end", 0u, boost::none, true, boost::none);
		testRegionTokenRule("----begin++++end", 4u, boost::none, true, 12u);
	}

	BOOST_AUTO_TEST_CASE(escape_sequences_test) {
		testRegionTokenRule("begin++++\\end", 0u, '\\', true, boost::none);
		testRegionTokenRule("\\begin++++end", 0u, '\\', true, boost::none);
		testRegionTokenRule("\\begin++++end", 1u, '\\', true, boost::none);
	}

	BOOST_AUTO_TEST_CASE(nocase_test) {
		testRegionTokenRule("----bEGIn++++End", 4u, boost::none, true, 12u);

		testRegionTokenRule("begin++++Xend", 0u, 'x', true, boost::none);
		testRegionTokenRule("Xbegin++++end", 0u, 'x', true, boost::none);
		testRegionTokenRule("xbegin++++end", 1u, 'X', true, boost::none);
	}
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_CASE(uri_token_rule) {
	// TODO: Write the tests.
}

namespace {
	bool matchWordSetTokenRule(const ascension::rules::WordSetTokenRule& rule, const char* text, const boost::integer_range<int>& wordRange) {
		const auto s(fromLatin1(text));
		const ascension::StringPiece input(s);
		const ascension::StringPiece word(input.substr(wordRange.front(), wordRange.size()));
		return rule.matches(input, word, IDS);
	}
}

BOOST_AUTO_TEST_SUITE(word_set_token_rule)
	BOOST_AUTO_TEST_CASE(basic_test) {
		const ascension::String words[] = {fromLatin1("begin"), fromLatin1("end")};
		const ascension::rules::WordSetTokenRule rule(DUMMY_ID, words, words + 2, true);

		BOOST_TEST(!matchWordSetTokenRule(rule, "", boost::irange(0, 0)));
		BOOST_TEST(!matchWordSetTokenRule(rule, "xxxx", boost::irange(0, 4)));
		BOOST_TEST( matchWordSetTokenRule(rule, "begin----end", boost::irange(0, 5)));
		BOOST_TEST( matchWordSetTokenRule(rule, "begin----end", boost::irange(9, 12)));
		BOOST_TEST(!matchWordSetTokenRule(rule, "begin----end", boost::irange(0, 6)));
	}
BOOST_AUTO_TEST_SUITE_END()


