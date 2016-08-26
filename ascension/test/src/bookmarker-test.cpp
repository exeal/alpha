#define BOOST_TEST_MODULE bookmarker_test
#include <boost/test/included/unit_test.hpp>

#include <ascension/kernel/bookmarker.hpp>
#include <ascension/kernel/document.hpp>
#include "from-latin1.hpp"

namespace k = ascension::kernel;

BOOST_AUTO_TEST_CASE(marks_test) {
	k::Document d;
	k::insert(d, k::Position::zero(), fromLatin1(
		"0\n"
		"1\n"
		"2\n"
		"3\n"
		"4"
	));
	auto& bm = d.bookmarker();

	BOOST_TEST(!bm.isMarked(0u));
	BOOST_TEST(!bm.isMarked(1u));
	BOOST_TEST(!bm.isMarked(2u));
	BOOST_TEST(!bm.isMarked(3u));
	BOOST_TEST(!bm.isMarked(4u));
	BOOST_CHECK_THROW(bm.isMarked(5u), k::BadPositionException);
	BOOST_TEST(bm.numberOfMarks() == 0u);

	bm.mark(0u);
	bm.mark(1u);
	bm.mark(1u, false);
	bm.mark(2u, true);
	bm.toggle(3u);
	bm.toggle(3u);
	bm.toggle(4u);
	BOOST_CHECK_THROW(bm.mark(5u), k::BadPositionException);
	BOOST_CHECK_THROW(bm.toggle(5u), k::BadPositionException);
	BOOST_TEST( bm.isMarked(0u));
	BOOST_TEST(!bm.isMarked(1u));
	BOOST_TEST( bm.isMarked(2u));
	BOOST_TEST(!bm.isMarked(3u));
	BOOST_TEST( bm.isMarked(4u));
	BOOST_TEST(bm.numberOfMarks() == 3u);

	bm.clear();
	BOOST_TEST(bm.numberOfMarks() == 0u);
}

BOOST_AUTO_TEST_CASE(document_modification_test) {
	k::Document d;
	k::insert(d, k::Position::zero(), fromLatin1(
		"0\n"	// *
		"1\n"
		"2\n"	// *
		"3\n"
		"4"		// *
	));
	auto& bm = d.bookmarker();
	bm.mark(0u);
	bm.mark(2u);
	bm.mark(4u);

	k::erase(d, k::Region::makeSingleLine(0u, boost::irange(0u, 1u)));
	BOOST_TEST(bm.isMarked(0u));

	k::erase(d, k::Region(k::Position::zero(), k::Position(1u, 0u)));
	BOOST_TEST(!bm.isMarked(0u));
	BOOST_TEST( bm.isMarked(1u));
	BOOST_TEST(!bm.isMarked(2u));
	BOOST_TEST( bm.isMarked(3u));
	BOOST_TEST(bm.numberOfMarks() == 2u);

	k::insert(d, k::Position(2u, 0u), fromLatin1("2.5\n"));
	BOOST_TEST(!bm.isMarked(0u));
	BOOST_TEST( bm.isMarked(1u));
	BOOST_TEST(!bm.isMarked(2u));
	BOOST_TEST(!bm.isMarked(3u));
	BOOST_TEST( bm.isMarked(4u));
}

BOOST_AUTO_TEST_CASE(enumeration_test) {
	k::Document d;
	k::insert(d, k::Position::zero(), fromLatin1(
		"0\n"	// *
		"1\n"
		"2\n"	// *
		"3\n"
		"4"		// *
	));
	auto& bm = d.bookmarker();
	bm.mark(0u);
	bm.mark(2u);
	bm.mark(4u);

	auto i(bm.begin());
	BOOST_TEST(*i == 0u);
	BOOST_TEST((i == bm.begin()));
	BOOST_TEST(*++i == 2u);
	BOOST_TEST(*++i == 4u);
	BOOST_TEST((++i == bm.end()));
	BOOST_TEST(*--i == 4u);
	BOOST_TEST(*--i == 2u);
	BOOST_TEST(*--i == 0u);
	BOOST_TEST((i == bm.begin()));

	auto next(bm.next(0u, ascension::Direction::forward()));
	BOOST_REQUIRE((next != boost::none));
	BOOST_TEST(boost::get(next) == 2u);
	next = bm.next(3u, ascension::Direction::forward(), false, 1u);
	BOOST_REQUIRE((next != boost::none));
	BOOST_TEST(boost::get(next) == 4u);
	BOOST_TEST((bm.next(4u, ascension::Direction::forward(), false, 1u) == boost::none));
	next = bm.next(4u, ascension::Direction::backward(), false, 1u);
	BOOST_REQUIRE((next != boost::none));
	BOOST_TEST(boost::get(next) == 2u);
	BOOST_TEST((bm.next(0u, ascension::Direction::backward(), false, 1u) == boost::none));
	BOOST_CHECK_THROW(bm.next(5u, ascension::Direction::backward()), k::BadPositionException);

	// multi-step
	next = bm.next(0u, ascension::Direction::forward(), false, 2u);
	BOOST_REQUIRE((next != boost::none));
	BOOST_TEST(boost::get(next) == 4u);
	BOOST_TEST((bm.next(0u, ascension::Direction::forward(), false, 3u) == boost::none));
	next = bm.next(4u, ascension::Direction::backward(), false, 2u);
	BOOST_REQUIRE((next != boost::none));
	BOOST_TEST(boost::get(next) == 0u);
	BOOST_TEST((bm.next(4u, ascension::Direction::backward(), false, 3u) == boost::none));

	// wrap around
	next = bm.next(0u, ascension::Direction::forward(), true, 3u);
	BOOST_REQUIRE((next != boost::none));
	BOOST_TEST(boost::get(next) == 0u);
	next = bm.next(0u, ascension::Direction::backward(), true, 10u);
	BOOST_REQUIRE((next != boost::none));
	BOOST_TEST(boost::get(next) == 4u);

	next = bm.next(2u, ascension::Direction::forward(), false, 0u);
	BOOST_REQUIRE((next != boost::none));
	BOOST_TEST(boost::get(next) == 2u);
	next = bm.next(3u, ascension::Direction::forward(), false, 0u);
	BOOST_TEST((next == boost::none));
}
