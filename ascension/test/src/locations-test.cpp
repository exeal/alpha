#define BOOST_TEST_MODULE locations_test
#include <boost/test/included/unit_test.hpp>

#include <ascension/kernel/bookmarker.hpp>
#include <ascension/kernel/document.hpp>
#include <ascension/kernel/locations.hpp>
#include "from-latin1.hpp"

namespace k = ascension::kernel;

namespace {
	struct Fixture {
		Fixture() {
			k::insert(d, k::Position::zero(), fromLatin1(
				"The quick\n"
				"brown fox jumps\n"
				"over the lazy dog"));
			d.narrowToRegion(k::Region(k::Position(0u, 4u), k::Position(2u, 13u)));
		}
		k::Document d;
	};
}

BOOST_FIXTURE_TEST_CASE(locations_check_test, Fixture) {
	// ascension.kernel.locations.isBeginningOfDocument
	BOOST_TEST(!k::locations::isBeginningOfDocument(std::make_pair(std::ref(d), k::Position::zero())));
	BOOST_TEST( k::locations::isBeginningOfDocument(std::make_pair(std::ref(d), k::Position(0u, 4u))));
	BOOST_CHECK_THROW(k::locations::isBeginningOfDocument(std::make_pair(std::ref(d), k::Position(3u, 0u))), k::BadPositionException);

	// ascension.kernel.locations.isBeginningOfLine
	BOOST_TEST(!k::locations::isBeginningOfLine(std::make_pair(std::ref(d), k::Position::zero())));
	BOOST_TEST( k::locations::isBeginningOfLine(std::make_pair(std::ref(d), k::Position(0u, 4u))));
	BOOST_TEST( k::locations::isBeginningOfLine(std::make_pair(std::ref(d), k::Position(1u, 0u))));
	BOOST_CHECK_THROW(k::locations::isBeginningOfLine(std::make_pair(std::ref(d), k::Position(3u, 0u))), k::BadPositionException);

	// ascension.kernel.locations.isEndOfDocument
	BOOST_TEST( k::locations::isEndOfDocument(std::make_pair(std::ref(d), k::Position(2u, 13u))));
	BOOST_TEST(!k::locations::isEndOfDocument(std::make_pair(std::ref(d), k::Position(2u, 17u))));
	BOOST_CHECK_THROW(k::locations::isEndOfDocument(std::make_pair(std::ref(d), k::Position(3u, 0u))), k::BadPositionException);

	// ascension.kernel.locations.isEndOfLine
	BOOST_TEST( k::locations::isEndOfLine(std::make_pair(std::ref(d), k::Position(0u, 9u))));
	BOOST_TEST( k::locations::isEndOfLine(std::make_pair(std::ref(d), k::Position(1u, 15u))));
	BOOST_TEST( k::locations::isEndOfLine(std::make_pair(std::ref(d), k::Position(2u, 13u))));
	BOOST_TEST(!k::locations::isEndOfLine(std::make_pair(std::ref(d), k::Position(2u, 17u))));
	BOOST_CHECK_THROW(k::locations::isEndOfLine(std::make_pair(std::ref(d), k::Position(3u, 0u))), k::BadPositionException);
}

BOOST_FIXTURE_TEST_CASE(motions_test, Fixture) {
	// ascension.kernel.locations.beginningOfDocument
	BOOST_TEST(k::locations::beginningOfDocument(std::make_pair(std::ref(d), k::Position())) == k::Position(0u, 4u));

	// ascension.kernel.locations.beginningOfLine
	BOOST_TEST(k::locations::beginningOfLine(std::make_pair(std::ref(d), k::Position::zero())) == k::Position(0u, 4u));
	BOOST_TEST(k::locations::beginningOfLine(std::make_pair(std::ref(d), k::Position(1u, 1u))) == k::Position::bol(1u));
	BOOST_TEST(k::locations::beginningOfLine(std::make_pair(std::ref(d), k::Position(2u, 2u))) == k::Position::bol(2u));
	BOOST_CHECK_THROW(k::locations::beginningOfLine(std::make_pair(std::ref(d), k::Position(3u, 3u))), k::BadPositionException);

	// ascension.kernel.locations.endOfDocument
	BOOST_TEST(k::locations::endOfDocument(std::make_pair(std::ref(d), k::Position())) == k::Position(2u, 13u));

	// ascension.kernel.locations.endOfLine
	BOOST_TEST(k::locations::endOfLine(std::make_pair(std::ref(d), k::Position::zero())) == k::Position(0u, 9u));
	BOOST_TEST(k::locations::endOfLine(std::make_pair(std::ref(d), k::Position(1u, 1u))) == k::Position(1u, 15u));
	BOOST_TEST(k::locations::endOfLine(std::make_pair(std::ref(d), k::Position(2u, 2u))) == k::Position(2u, 13u));
	BOOST_CHECK_THROW(k::locations::endOfLine(std::make_pair(std::ref(d), k::Position(3u, 3u))), k::BadPositionException);

	// ascension.kernel.locations.nextLine
	BOOST_TEST(k::locations::nextLine(std::make_pair(std::ref(d), k::Position(0u, 4u)), ascension::Direction::forward(), 1u) == k::Position(1u, 4u));
	BOOST_TEST(k::locations::nextLine(std::make_pair(std::ref(d), k::Position(0u, 4u)), ascension::Direction::forward(), 2u) == k::Position(2u, 4u));
	BOOST_TEST(k::locations::nextLine(std::make_pair(std::ref(d), k::Position(0u, 4u)), ascension::Direction::forward(), 3u) == k::Position(2u, 4u));
	BOOST_TEST(k::locations::nextLine(std::make_pair(std::ref(d), k::Position(2u, 6u)), ascension::Direction::backward(), 1u) == k::Position(1u, 6u));
	BOOST_TEST(k::locations::nextLine(std::make_pair(std::ref(d), k::Position(2u, 6u)), ascension::Direction::backward(), 2u) == k::Position(0u, 6u));
	BOOST_TEST(k::locations::nextLine(std::make_pair(std::ref(d), k::Position(2u, 6u)), ascension::Direction::backward(), 3u) == k::Position(0u, 6u));
	BOOST_TEST(k::locations::nextLine(std::make_pair(std::ref(d), k::Position(1u, 0u)), ascension::Direction::backward(), 1u) == k::Position(1u, 0u));
	BOOST_TEST(k::locations::nextLine(std::make_pair(std::ref(d), k::Position(1u, 15u)), ascension::Direction::forward(), 1u) == k::Position(1u, 15u));

	// ascension.kernel.locations.nextWord
	BOOST_TEST(k::locations::nextWord(std::make_pair(std::ref(d), k::Position(1u, 0u)), ascension::Direction::forward(), 1u) == k::Position(1u, 6u));
	BOOST_TEST(k::locations::nextWord(std::make_pair(std::ref(d), k::Position(1u, 1u)), ascension::Direction::forward(), 2u) == k::Position(1u, 10u));
	BOOST_TEST(k::locations::nextWord(std::make_pair(std::ref(d), k::Position(1u, 2u)), ascension::Direction::forward(), 3u) == k::Position(2u, 0u));
	BOOST_TEST(k::locations::nextWord(std::make_pair(std::ref(d), k::Position(1u, 2u)), ascension::Direction::forward(), 9u) == k::Position(2u, 13u));
	BOOST_TEST(k::locations::nextWord(std::make_pair(std::ref(d), k::Position(2u, 5u)), ascension::Direction::backward(), 1u) == k::Position(2u, 0u));
	BOOST_TEST(k::locations::nextWord(std::make_pair(std::ref(d), k::Position(2u, 6u)), ascension::Direction::backward(), 2u) == k::Position(2u, 0u));
	BOOST_TEST(k::locations::nextWord(std::make_pair(std::ref(d), k::Position(2u, 7u)), ascension::Direction::backward(), 3u) == k::Position(1u, 10u));
	BOOST_TEST(k::locations::nextWord(std::make_pair(std::ref(d), k::Position(2u, 7u)), ascension::Direction::backward(), 9u) == k::Position(0u, 4u));
	BOOST_CHECK_THROW(k::locations::nextWord(std::make_pair(std::ref(d), k::Position(0u, 0u)), ascension::Direction::forward(), 1u), k::BadPositionException);
	BOOST_CHECK_THROW(k::locations::nextWord(std::make_pair(std::ref(d), k::Position(2u, 17u)), ascension::Direction::backward(), 1u), k::BadPositionException);

	// ascension.kernel.locations.nextWordEnd
	BOOST_TEST(k::locations::nextWordEnd(std::make_pair(std::ref(d), k::Position(1u, 0u)), ascension::Direction::forward(), 1u) == k::Position(1u, 5u));
	BOOST_TEST(k::locations::nextWordEnd(std::make_pair(std::ref(d), k::Position(1u, 1u)), ascension::Direction::forward(), 2u) == k::Position(1u, 9u));
	BOOST_TEST(k::locations::nextWordEnd(std::make_pair(std::ref(d), k::Position(1u, 2u)), ascension::Direction::forward(), 3u) == k::Position(1u, 15u));
	BOOST_TEST(k::locations::nextWordEnd(std::make_pair(std::ref(d), k::Position(1u, 2u)), ascension::Direction::forward(), 9u) == k::Position(2u, 13u));
	BOOST_TEST(k::locations::nextWordEnd(std::make_pair(std::ref(d), k::Position(2u, 5u)), ascension::Direction::backward(), 1u) == k::Position(2u, 4u));
	BOOST_TEST(k::locations::nextWordEnd(std::make_pair(std::ref(d), k::Position(2u, 6u)), ascension::Direction::backward(), 2u) == k::Position(1u, 15u));
	BOOST_TEST(k::locations::nextWordEnd(std::make_pair(std::ref(d), k::Position(2u, 7u)), ascension::Direction::backward(), 3u) == k::Position(1u, 9u));
	BOOST_TEST(k::locations::nextWordEnd(std::make_pair(std::ref(d), k::Position(2u, 7u)), ascension::Direction::backward(), 9u) == k::Position(0u, 4u));
	BOOST_CHECK_THROW(k::locations::nextWordEnd(std::make_pair(std::ref(d), k::Position(0u, 0u)), ascension::Direction::forward(), 1u), k::BadPositionException);
	BOOST_CHECK_THROW(k::locations::nextWordEnd(std::make_pair(std::ref(d), k::Position(2u, 17u)), ascension::Direction::backward(), 1u), k::BadPositionException);
}

BOOST_AUTO_TEST_CASE(next_character_test) {
	static const ascension::Char text[] = {'a', 0xd800u, 0xdc00u, 'A', 0x0300u, 0xd800u, 0xdc00u, 'z'};
	k::Document d;
	k::insert(d, k::Position::zero(), std::begin(text), std::end(text));

	BOOST_TEST(
		k::locations::nextCharacter(
			std::make_pair(std::ref(d), k::Position(0u, 0u)),
			ascension::Direction::forward(), k::locations::UTF16_CODE_UNIT, 1u) == k::Position(0u, 1u));
	BOOST_TEST(
		k::locations::nextCharacter(
			std::make_pair(std::ref(d), k::Position(0u, 0u)),
			ascension::Direction::forward(), k::locations::UTF16_CODE_UNIT, 2u) == k::Position(0u, 2u));
	BOOST_TEST(
		k::locations::nextCharacter(
			std::make_pair(std::ref(d), k::Position(0u, 0u)),
			ascension::Direction::forward(), k::locations::UTF32_CODE_UNIT, 2u) == k::Position(0u, 3u));
	BOOST_TEST(
		k::locations::nextCharacter(
			std::make_pair(std::ref(d), k::Position(0u, 0u)),
			ascension::Direction::forward(), k::locations::GRAPHEME_CLUSTER, 3u) == k::Position(0u, 5u));

	BOOST_TEST(
		k::locations::nextCharacter(
			std::make_pair(std::ref(d), k::Position(0u, 8u)),
			ascension::Direction::backward(), k::locations::UTF16_CODE_UNIT, 1u) == k::Position(0u, 7u));
	BOOST_TEST(
		k::locations::nextCharacter(
			std::make_pair(std::ref(d), k::Position(0u, 8u)),
			ascension::Direction::backward(), k::locations::UTF16_CODE_UNIT, 2u) == k::Position(0u, 6u));
	BOOST_TEST(
		k::locations::nextCharacter(
			std::make_pair(std::ref(d), k::Position(0u, 8u)),
			ascension::Direction::backward(), k::locations::UTF32_CODE_UNIT, 2u) == k::Position(0u, 5u));
	BOOST_TEST(
		k::locations::nextCharacter(
			std::make_pair(std::ref(d), k::Position(0u, 8u)),
			ascension::Direction::backward(), k::locations::GRAPHEME_CLUSTER, 3u) == k::Position(0u, 3u));
}

BOOST_FIXTURE_TEST_CASE(next_bookmark_test, Fixture) {
	d.bookmarker().mark(0u);
	d.bookmarker().mark(2u);

	auto result(k::locations::nextBookmark(std::make_pair(std::ref(d), k::Position(0u, 0u)), ascension::Direction::forward(), 1u));
	BOOST_REQUIRE((result != boost::none));
	BOOST_TEST(boost::get(result) == k::Position(2u, 0u));

	result = k::locations::nextBookmark(std::make_pair(std::ref(d), k::Position(0u, 0u)), ascension::Direction::forward(), 2u);
	BOOST_REQUIRE((result != boost::none));
	BOOST_TEST(boost::get(result) == k::Position(0u, 4u));

	result = k::locations::nextBookmark(std::make_pair(std::ref(d), k::Position(2u, 0u)), ascension::Direction::backward(), 1u);
	BOOST_REQUIRE((result != boost::none));
	BOOST_TEST(boost::get(result) == k::Position(0u, 4u));

	result = k::locations::nextBookmark(std::make_pair(std::ref(d), k::Position(2u, 0u)), ascension::Direction::backward(), 2u);
	BOOST_REQUIRE((result != boost::none));
	BOOST_TEST(boost::get(result) == k::Position(2u, 0u));
}
