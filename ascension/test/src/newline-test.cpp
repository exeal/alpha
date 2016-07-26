#define BOOST_TEST_MODULE newline_test
#include <boost/test/included/unit_test.hpp>

#include <ascension/corelib/text/newline.hpp>

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
	BOOST_TEST(ascension::text::Newline::LINE_FEED.asString() == L"\n");
	BOOST_TEST(ascension::text::Newline::CARRIAGE_RETURN.asString() == L"\r");
	BOOST_TEST(ascension::text::Newline::CARRIAGE_RETURN_FOLLOWED_BY_LINE_FEED.asString() == L"\r\n");
	BOOST_TEST(ascension::text::Newline::NEXT_LINE.asString() == L"\x0085");
	BOOST_TEST(ascension::text::Newline::LINE_SEPARATOR.asString() == L"\x2028");
	BOOST_TEST(ascension::text::Newline::PARAGRAPH_SEPARATOR.asString() == L"\x2029");
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
	BOOST_TEST(ascension::text::calculateNumberOfLines(std::string()) == static_cast<ascension::Index>(0));
	BOOST_TEST(ascension::text::calculateNumberOfLines(std::string("xyzzy")) == static_cast<ascension::Index>(1));
	BOOST_TEST(ascension::text::calculateNumberOfLines(std::string("\n")) == static_cast<ascension::Index>(2));
	BOOST_TEST(ascension::text::calculateNumberOfLines(std::string("\r\n")) == static_cast<ascension::Index>(2));
	BOOST_TEST(ascension::text::calculateNumberOfLines(std::string("\n\r")) == static_cast<ascension::Index>(3));
	BOOST_TEST(ascension::text::calculateNumberOfLines(std::string("1\n2\n3")) == static_cast<ascension::Index>(3));
	BOOST_TEST(ascension::text::calculateNumberOfLines(std::wstring(L"first\x0085second")) == static_cast<ascension::Index>(2));
	BOOST_TEST(ascension::text::calculateNumberOfLines(std::wstring(L"first\x2028second")) == static_cast<ascension::Index>(2));
	BOOST_TEST(ascension::text::calculateNumberOfLines(std::wstring(L"first\x2029second")) == static_cast<ascension::Index>(2));
}

BOOST_AUTO_TEST_CASE(scan_test) {
	BOOST_TEST((ascension::text::eatNewline(std::string()) == boost::none));
	BOOST_TEST((ascension::text::eatNewline(std::string("xyzzy")) == boost::none));

	auto newline(ascension::text::eatNewline(std::string("\n")));
	BOOST_REQUIRE(newline != boost::none);
	BOOST_TEST(boost::get(newline) == ascension::text::Newline::LINE_FEED);

	newline = ascension::text::eatNewline(std::string("\r\n"));
	BOOST_REQUIRE(newline != boost::none);
	BOOST_TEST(boost::get(newline) == ascension::text::Newline::CARRIAGE_RETURN_FOLLOWED_BY_LINE_FEED);

	newline = ascension::text::eatNewline(std::string("\n\r"));
	BOOST_REQUIRE(newline != boost::none);
	BOOST_TEST(boost::get(newline) == ascension::text::Newline::LINE_FEED);
}
