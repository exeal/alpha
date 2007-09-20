// regex-test.cpp

#include "test.hpp"
#include "../regex.hpp"

void testRegex() {
	// zero-width matches
	ascension::String input(L"abcde");
	std::auto_ptr<ascension::regex::Pattern> p(ascension::regex::Pattern::compile(L"x?"));
	std::auto_ptr<ascension::regex::Matcher<ascension::unicode::StringCharacterIterator> >
		m(p->matcher(ascension::unicode::StringCharacterIterator(input), ascension::unicode::StringCharacterIterator(input, input.end())));
	BOOST_CHECK_EQUAL(m->replaceAll(L"!"), L"!a!b!c!d!e!");

	std::basic_ostringstream<ascension::Char> oss;
	std::ostream_iterator<ascension::Char, ascension::Char> out(oss);
	while(m->find())
		m->appendReplacement(out, L"!");
	m->appendTail(out);
	BOOST_CHECK_EQUAL(oss.str(), L"!a!b!c!d!e!");
}
