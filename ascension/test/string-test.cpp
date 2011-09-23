// string-test.cpp

#include <ascension/corelib/string-piece.hpp>
#include <ascension/corelib/ustring.hpp>
#include <boost/test/included/test_exec_monitor.hpp>
namespace a = ascension;


void testStringAPIs() {
}

// from ~/source/test/intltest/strtest.cpp of ICU
void testStringPiece() {
	// default constructor
	a::BasicStringPiece<char> empty;
	BOOST_CHECK(a::isEmpty(empty));
	BOOST_CHECK_EQUAL(empty.beginning(), static_cast<const char*>(0));
	BOOST_CHECK_EQUAL(a::length(empty), 0);

	// construct from null pointer
	a::BasicStringPiece<char> null(0);
	BOOST_CHECK(a::isEmpty(null));
	BOOST_CHECK_EQUAL(null.beginning(), static_cast<const char*>(0));
	BOOST_CHECK_EQUAL(null.end(), static_cast<const char*>(0));
	BOOST_CHECK_EQUAL(a::length(null), 0);

	// construct from character pointer and length
	static const char* ABCDEFG_CHARS = "abcdefg";
	a::BasicStringPiece<char> abcd(ABCDEFG_CHARS, 4);
	BOOST_CHECK(!a::isEmpty(abcd));
	BOOST_CHECK_EQUAL(abcd.beginning(), ABCDEFG_CHARS);
	BOOST_CHECK_EQUAL(abcd.end(), ABCDEFG_CHARS + 4);
	BOOST_CHECK_EQUAL(a::length(abcd), 4);

	// construct from two character pointers
	a::BasicStringPiece<char> abcdef(ABCDEFG_CHARS, ABCDEFG_CHARS + 6);
	BOOST_CHECK(!a::isEmpty(abcdef));
	BOOST_CHECK_EQUAL(abcdef.beginning(), ABCDEFG_CHARS);
	BOOST_CHECK_EQUAL(abcdef.end(), ABCDEFG_CHARS + 6);
	BOOST_CHECK_EQUAL(a::length(abcd), 4);

	// construct from std.string
	std::string uvwxyzString("uvwxyz");
	a::BasicStringPiece<char> uvwxyz(uvwxyzString);
	BOOST_CHECK(!a::isEmpty(uvwxyz));
	BOOST_CHECK_EQUAL(uvwxyz.beginning(), uvwxyzString.data());
	BOOST_CHECK_EQUAL(uvwxyz.end(), uvwxyzString.data() + 6);
	BOOST_CHECK_EQUAL(a::length(uvwxyz), 6);

	// operator[]
	BOOST_CHECK_EQUAL(abcd[0], ABCDEFG_CHARS[0]);
	BOOST_CHECK_EQUAL(abcd[1], ABCDEFG_CHARS[1]);
	BOOST_CHECK_EQUAL(abcd[2], ABCDEFG_CHARS[2]);
	BOOST_CHECK_EQUAL(abcd[3], ABCDEFG_CHARS[3]);

	// at
	BOOST_CHECK_EQUAL(abcd.at(0), ABCDEFG_CHARS[0]);
	BOOST_CHECK_EQUAL(abcd.at(1), ABCDEFG_CHARS[1]);
	BOOST_CHECK_EQUAL(abcd.at(2), ABCDEFG_CHARS[2]);
	BOOST_CHECK_EQUAL(abcd.at(3), ABCDEFG_CHARS[3]);
	BOOST_CHECK_THROW(abcd.at(4), std::out_of_range);
}

int test_main(int, char*[]) {
	testStringPiece();
	testStringAPIs();
	return 0;
}
