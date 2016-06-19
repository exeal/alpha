#define BOOST_TEST_MODULE regex_test
#include <boost/test/included/unit_test.hpp>

#include <ascension/corelib/regex.hpp>
#include <ascension/corelib/text/string-character-iterator.hpp>

BOOST_AUTO_TEST_CASE(ucs4_match_test) {
	BOOST_CHECK(ascension::regex::Pattern::matches(L".", L"\xD800\xDC00"));
}

BOOST_AUTO_TEST_CASE(transparent_bounds_test) {
	// see Jeffrey E.F. Friedl's "Mastering Regular Expressions 3rd edition", page 388, 389
	auto pattern(ascension::regex::Pattern::compile(L"\\bcar\\b"));
	ascension::String text(L"Madagascar is best seen by car or bike.");
	const ascension::text::StringCharacterIterator e(text, text.end());
	auto match(pattern->matcher(ascension::text::StringCharacterIterator(text), e));
	match->region(ascension::text::StringCharacterIterator(text, text.begin() + 7), e);
	match->find();
	BOOST_TEST(match->start().tell() - text.data() == 7);

	match->useTransparentBounds(true);
	match->region(ascension::text::StringCharacterIterator(text, text.begin() + 7), e);
	match->find();
	BOOST_TEST(match->start().tell() - text.data() == 27);
}

BOOST_AUTO_TEST_CASE(zero_width_test) {
	ascension::String input(L"abcde");
	auto pattern(ascension::regex::Pattern::compile(L"x?"));
	auto match(pattern->matcher(ascension::text::StringCharacterIterator(input), ascension::text::StringCharacterIterator(input, input.end())));
	BOOST_TEST(match->replaceAll(L"!") == L"!a!b!c!d!e!");

	std::basic_ostringstream<ascension::Char> oss;
	std::ostream_iterator<ascension::Char, ascension::Char> out(oss);
	while(match->find())
		match->appendReplacement(out, L"!");
	match->appendTail(out);
//	checkEqualStrings(oss.str(), L"!a!b!c!d!e!");
}
