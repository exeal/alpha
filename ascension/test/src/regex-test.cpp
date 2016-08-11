#define BOOST_TEST_MODULE regex_test
#include <boost/test/included/unit_test.hpp>

#include <ascension/corelib/regex.hpp>
#include <ascension/corelib/text/string-character-iterator.hpp>
#include "from-latin1.hpp"

BOOST_AUTO_TEST_CASE(ucs4_match_test) {
	const ascension::Char pattern[] = {'.', '\0'};
	const ascension::Char input[] = {0xd800u, 0xdc00u, '\0'};
	BOOST_CHECK(ascension::regex::Pattern::matches(pattern, input));
}

BOOST_AUTO_TEST_CASE(transparent_bounds_test) {
	// see Jeffrey E.F. Friedl's "Mastering Regular Expressions 3rd edition", page 388, 389
	auto pattern(ascension::regex::Pattern::compile(fromLatin1("\\bcar\\b")));
	ascension::String text(fromLatin1("Madagascar is best seen by car or bike."));
	const ascension::text::StringCharacterIterator e(text, text.end());
	auto match(pattern->matcher(ascension::text::StringCharacterIterator(text), e));
	match->region(ascension::text::StringCharacterIterator(text, text.begin() + 7), e);
	bool found = match->find();
	BOOST_REQUIRE(found);
	BOOST_TEST(match->start().tell() - text.data() == 7);

	match->useTransparentBounds(true);
	match->region(ascension::text::StringCharacterIterator(text, text.begin() + 7), e);
	found = match->find();
	BOOST_REQUIRE(found);
	BOOST_TEST(match->start().tell() - text.data() == 27);
}

BOOST_AUTO_TEST_CASE(zero_width_test) {
	ascension::String input(fromLatin1("abcde"));
	auto pattern(ascension::regex::Pattern::compile(fromLatin1("x?")));
	auto match(pattern->matcher(ascension::text::StringCharacterIterator(input), ascension::text::StringCharacterIterator(input, input.end())));
	BOOST_TEST(match->replaceAll(fromLatin1("!")) == fromLatin1("!a!b!c!d!e!"));

	ascension::String s;
	while(match->find())
		match->appendReplacement(std::back_inserter(s), fromLatin1("!"));
	match->appendTail(std::back_inserter(s));
//	BOOST_TEST(s == fromLatin1("!a!b!c!d!e!"));
}
