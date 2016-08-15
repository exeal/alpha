#define BOOST_TEST_MODULE token_rules_test
#include <boost/test/included/unit_test.hpp>

#include <ascension/corelib/text/identifier-syntax.hpp>
#include <ascension/rules/number-token-rule.hpp>
#include <ascension/rules/regex-token-rule.hpp>
#include <ascension/rules/region-token-rule.hpp>
#include <ascension/rules/uri-token-rule.hpp>
#include <ascension/rules/word-token-rule.hpp>
#include <ascension/rules/word-set-token-rule.hpp>
#include <boost/range/irange.hpp>
#include "from-latin1.hpp"

namespace {
	const ascension::rules::Token::Identifier DUMMY_ID = 100;	// TODO: Use safe nextIdentifier()...
	const auto& IDS = ascension::text::IdentifierSyntax::defaultInstance();
}

namespace {
	void testNumberTokenRule(const char* text, int position, boost::optional<int> expectedResult) {
		const ascension::rules::NumberTokenRule rule(DUMMY_ID);
		const auto s(fromLatin1(text));
		const ascension::StringPiece input(s);
		const auto result(rule.parse(input, input.cbegin() + position, IDS));

		if(expectedResult == boost::none)
			BOOST_TEST((result == boost::none));
		else {
			BOOST_REQUIRE((result != boost::none));
			BOOST_TEST(boost::get(result) - input.cbegin() == boost::get(expectedResult));
		}
	}
}

BOOST_AUTO_TEST_SUITE(number_token_rule)
	BOOST_AUTO_TEST_CASE(decimal_literal_test1) {
		testNumberTokenRule("0", 0, 1);
	}

	BOOST_AUTO_TEST_CASE(decimal_literal_test2) {
		testNumberTokenRule(".", 0, boost::none);
		testNumberTokenRule(".o", 0, boost::none);
		testNumberTokenRule(".693147", 0, 7);
		testNumberTokenRule(".693147i", 0, boost::none);
		testNumberTokenRule(".e+1", 0, boost::none);
		testNumberTokenRule(".ea", 0, boost::none);
		testNumberTokenRule(".314e1", 0, 6);
		testNumberTokenRule(".314e+1", 0, 7);
		testNumberTokenRule(".314e-1", 0, 7);
		testNumberTokenRule(".314e+-0", 0, boost::none);
		testNumberTokenRule(".602E+24", 0, 8);
//		testNumberTokenRule(".0.0", 0, boost::none);
		testNumberTokenRule("0.0", 1, boost::none);
		testNumberTokenRule("a.0", 1, boost::none);
		testNumberTokenRule("@.0", 1, 3);
	}

	BOOST_AUTO_TEST_CASE(hex_integer_literal_test) {
		testNumberTokenRule("0x", 0, boost::none);
		testNumberTokenRule("0x0", 0, 3);
		testNumberTokenRule("0XA", 0, 3);
		testNumberTokenRule("0xDEADBEEF", 0, 10);
		testNumberTokenRule("0xDEADCODE", 0, boost::none);
		testNumberTokenRule("0x00e+0", 0, 5);
		testNumberTokenRule("0x00.0", 0, 4);
		testNumberTokenRule("00x0", 1, boost::none);
	}
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_CASE(regex_token_rule) {
	// TODO: Write the tests.
}

namespace {
	void testRegionTokenRule(const char* text, std::size_t position, boost::optional<char> escapeCharacter, bool caseSensitive, boost::optional<std::size_t> expectedResult) {
		boost::optional<ascension::Char> ec;
		if(escapeCharacter != boost::none)
			ec = boost::get(escapeCharacter);
		const ascension::rules::RegionTokenRule rule(DUMMY_ID, fromLatin1("begin"), fromLatin1("end"), ec, caseSensitive);
		const auto s(fromLatin1(text));
		const ascension::StringPiece input(s);
		const auto result(rule.parse(input, input.cbegin() + position, IDS));

		if(expectedResult == boost::none)
			BOOST_TEST((result == boost::none));
		else {
			BOOST_REQUIRE((result != boost::none));
			BOOST_TEST(boost::get(result) == input.cbegin() + boost::get(expectedResult));
		}
	}
}

BOOST_AUTO_TEST_SUITE(region_token_rule)
	BOOST_AUTO_TEST_CASE(basic_test) {
		testRegionTokenRule("----begin++++end", 0, boost::none, true, boost::none);
		testRegionTokenRule("----begin++++end", 4, boost::none, true, 16);
	}

	BOOST_AUTO_TEST_CASE(escape_sequences_test) {
		testRegionTokenRule("begin++++\\end", 0, '\\', true, boost::none);
		testRegionTokenRule("\\begin++++end", 0, '\\', true, boost::none);
		testRegionTokenRule("\\begin++++end", 1, '\\', true, boost::none);
	}

	BOOST_AUTO_TEST_CASE(nocase_test) {
		testRegionTokenRule("----bEGIn++++End", 4, boost::none, true, 16);

		testRegionTokenRule("begin++++Xend", 0, 'x', true, boost::none);
		testRegionTokenRule("Xbegin++++end", 0, 'x', true, boost::none);
		testRegionTokenRule("xbegin++++end", 1, 'X', true, boost::none);
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
		return rule.parse(input, word, IDS);
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


