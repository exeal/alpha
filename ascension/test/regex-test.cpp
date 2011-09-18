// regex-test.cpp

#include <ascension/corelib/regex.hpp>
#include <boost/test/included/test_exec_monitor.hpp>

namespace a = ascension;
namespace re = ascension::regex;

inline void checkEqualStrings(const a::String& s1, const a::String& s2) {
	BOOST_CHECK_EQUAL_COLLECTIONS(s1.begin(), s1.end(), s2.begin(), s2.end());
}

void testUCS4Matches() {
	BOOST_CHECK(re::Pattern::matches(L".", L"\xD800\xDC00"));
}

// tests transparent bounds-related features.
// see Jeffrey E.F. Friedl's "Mastering Regular Expressions 3rd edition", page 388, 389
void testTransparentBounds() {
	std::auto_ptr<const re::Pattern> p(re::Pattern::compile(L"\\bcar\\b"));
	a::String text(L"Madagascar is best seen by car or bike.");
	const a::text::StringCharacterIterator e(text, text.end());
	std::auto_ptr<re::Matcher<a::text::StringCharacterIterator> > m(p->matcher(a::text::StringCharacterIterator(text), e));
	m->region(a::text::StringCharacterIterator(text, text.begin() + 7), e);
	m->find();
	BOOST_CHECK_EQUAL(m->start().tell() - text.data(), 7);
	m->useTransparentBounds(true);
	m->region(a::text::StringCharacterIterator(text, text.begin() + 7), e);
	m->find();
	BOOST_CHECK_EQUAL(m->start().tell() - text.data(), 27);
}

void testZeroWidth() {
	a::String input(L"abcde");
	std::auto_ptr<const re::Pattern> p(re::Pattern::compile(L"x?"));
	std::auto_ptr<re::Matcher<a::text::StringCharacterIterator> >
		m(p->matcher(a::text::StringCharacterIterator(input), a::text::StringCharacterIterator(input, input.end())));
	checkEqualStrings(m->replaceAll(L"!"), L"!a!b!c!d!e!");

	std::basic_ostringstream<a::Char> oss;
	std::ostream_iterator<a::Char, a::Char> out(oss);
	while(m->find())
		m->appendReplacement(out, L"!");
	m->appendTail(out);
//	checkEqualStrings(oss.str(), L"!a!b!c!d!e!");
}

int test_main(int, char*[]) {
	testUCS4Matches();
	testTransparentBounds();
	testZeroWidth();
	return 0;
}
