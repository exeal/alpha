#define BOOST_TEST_MODULE newline_test
#include <boost/test/included/unit_test.hpp>

#include <ascension/corelib/text/newline.hpp>
#include <boost/range/iterator.hpp>
#include "from-latin1.hpp"

namespace ascension {
	namespace text {
		template<typename OutputStream>
		inline OutputStream& operator<<(OutputStream& out, const Newline& value) {
			return out << std::hex << reinterpret_cast<std::intptr_t>(&value);
		}
	}
}

BOOST_AUTO_TEST_CASE(equality_test) {
	BOOST_TEST(ascension::text::Newline::LINE_FEED == ascension::text::Newline::LINE_FEED);
	BOOST_TEST(ascension::text::Newline::LINE_FEED != ascension::text::Newline::CARRIAGE_RETURN);
}

BOOST_AUTO_TEST_CASE(copy_construction_test) {
	const ascension::text::Newline nl(ascension::text::Newline::NEXT_LINE);
	BOOST_TEST(nl == ascension::text::Newline::NEXT_LINE);
}

BOOST_AUTO_TEST_CASE(assignment_test) {
	ascension::text::Newline nl(ascension::text::Newline::LINE_SEPARATOR);
	nl = ascension::text::Newline::PARAGRAPH_SEPARATOR;
	BOOST_TEST(nl == ascension::text::Newline::PARAGRAPH_SEPARATOR);
}

BOOST_AUTO_TEST_CASE(stringfy_test) {
	BOOST_TEST(ascension::text::Newline::LINE_FEED.asString() == fromLatin1("\n"));
	BOOST_TEST(ascension::text::Newline::CARRIAGE_RETURN.asString() == fromLatin1("\r"));
	BOOST_TEST(ascension::text::Newline::CARRIAGE_RETURN_FOLLOWED_BY_LINE_FEED.asString() == fromLatin1("\r\n"));

	ascension::String s;
	auto inserter(std::back_inserter(s));
	ascension::text::utf::encode(0x0085u, inserter);
	BOOST_TEST(ascension::text::Newline::NEXT_LINE.asString() == s);

	s.clear();
	ascension::text::utf::encode(0x2028u, inserter);
	BOOST_TEST(ascension::text::Newline::LINE_SEPARATOR.asString() == s);

	s.clear();
	ascension::text::utf::encode(0x2029u, inserter);
	BOOST_TEST(ascension::text::Newline::PARAGRAPH_SEPARATOR.asString() == s);

	BOOST_CHECK_THROW(ascension::text::Newline::USE_INTRINSIC_VALUE.asString(), std::logic_error);
	BOOST_CHECK_THROW(ascension::text::Newline::USE_DOCUMENT_INPUT.asString(), std::logic_error);
}

BOOST_AUTO_TEST_CASE(literality_test) {
	BOOST_TEST( ascension::text::Newline::LINE_FEED.isLiteral());
	BOOST_TEST( ascension::text::Newline::CARRIAGE_RETURN.isLiteral());
	BOOST_TEST( ascension::text::Newline::CARRIAGE_RETURN_FOLLOWED_BY_LINE_FEED.isLiteral());
	BOOST_TEST( ascension::text::Newline::NEXT_LINE.isLiteral());
	BOOST_TEST( ascension::text::Newline::LINE_SEPARATOR.isLiteral());
	BOOST_TEST( ascension::text::Newline::PARAGRAPH_SEPARATOR.isLiteral());
	BOOST_TEST(!ascension::text::Newline::USE_INTRINSIC_VALUE.isLiteral());
	BOOST_TEST(!ascension::text::Newline::USE_DOCUMENT_INPUT.isLiteral());
}

BOOST_AUTO_TEST_CASE(line_counting_test) {
	BOOST_TEST(ascension::text::calculateNumberOfLines(ascension::String()) == static_cast<ascension::Index>(1));

	BOOST_TEST(ascension::text::calculateNumberOfLines(ascension::String(), 0) == static_cast<ascension::Index>(0));

	BOOST_TEST(ascension::text::calculateNumberOfLines(fromLatin1("xyzzy")) == static_cast<ascension::Index>(1));

	BOOST_TEST(ascension::text::calculateNumberOfLines(fromLatin1("\n")) == static_cast<ascension::Index>(2));

	BOOST_TEST(ascension::text::calculateNumberOfLines(fromLatin1("\r\n")) == static_cast<ascension::Index>(2));

	BOOST_TEST(ascension::text::calculateNumberOfLines(fromLatin1("\n\r")) == static_cast<ascension::Index>(3));

	BOOST_TEST(ascension::text::calculateNumberOfLines(fromLatin1("1\n2\n3")) == static_cast<ascension::Index>(3));

	const ascension::Char firstNextLineSecond[] = {'1', 0x0085u, '2'};
	BOOST_TEST(ascension::text::calculateNumberOfLines(boost::make_iterator_range(firstNextLineSecond)) == static_cast<ascension::Index>(2));

	const ascension::Char firstLineSeparatorSecond[] = {'1', 0x2028u, '2'};
	BOOST_TEST(ascension::text::calculateNumberOfLines(boost::make_iterator_range(firstLineSeparatorSecond)) == static_cast<ascension::Index>(2));

	const ascension::Char firstParagraphSeparatorSecond[] = {'1', 0x2029u, '2'};
	BOOST_TEST(ascension::text::calculateNumberOfLines(boost::make_iterator_range(firstParagraphSeparatorSecond)) == static_cast<ascension::Index>(2));
}

BOOST_AUTO_TEST_CASE(scan_test) {
	BOOST_TEST((ascension::text::eatNewline(fromLatin1("xyzzy")) == boost::none));

	auto newline(ascension::text::eatNewline(fromLatin1("\n")));
	BOOST_REQUIRE(newline != boost::none);
	BOOST_TEST(boost::get(newline) == ascension::text::Newline::LINE_FEED);

	newline = ascension::text::eatNewline(fromLatin1("\r\n"));
	BOOST_REQUIRE(newline != boost::none);
	BOOST_TEST(boost::get(newline) == ascension::text::Newline::CARRIAGE_RETURN_FOLLOWED_BY_LINE_FEED);

	newline = ascension::text::eatNewline(fromLatin1("\n\r"));
	BOOST_REQUIRE(newline != boost::none);
	BOOST_TEST(boost::get(newline) == ascension::text::Newline::LINE_FEED);
}
