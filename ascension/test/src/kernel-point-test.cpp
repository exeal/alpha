#define BOOST_TEST_MODULE locations_test
#include <boost/test/included/unit_test.hpp>

#include <ascension/kernel/document.hpp>
#include <ascension/kernel/point.hpp>
#include "from-latin1.hpp"

namespace k = ascension::kernel;

namespace {
	struct Fixture {
		Fixture() {
			ascension::kernel::insert(d, k::Position::zero(), fromLatin1(
				"abc\n"
				"def\n"
				"ghi"));
		}
		k::Document d;
	};
}

BOOST_FIXTURE_TEST_CASE(construction_test, Fixture) {
	const ascension::kernel::Point p(d, k::Position(1u, 2u));

	BOOST_TEST(&p.document() == &d);
	BOOST_TEST(p.adaptsToDocument());
	BOOST_TEST((p.gravity() == ascension::Direction::forward()));
	BOOST_TEST(p.position() == k::Position(1u, 2u));
}

BOOST_FIXTURE_TEST_CASE(copy_construction_test, Fixture) {
	const k::Point p(d, k::Position(1u, 2u));
	const k::Point p2(p);

	BOOST_TEST(&p.document() == &p2.document());
	BOOST_TEST(p.adaptsToDocument() == p2.adaptsToDocument());
	BOOST_TEST((p.gravity() == p2.gravity()));
	BOOST_TEST(p.position() == p2.position());
}

BOOST_FIXTURE_TEST_CASE(comparisons_test, Fixture) {
	BOOST_TEST((k::Point(d, k::Position(1u, 2u)) == k::Point(d, k::Position(1u, 2u))));
	BOOST_TEST((k::Point(d, k::Position(1u, 2u)) < k::Point(d, k::Position(1u, 3u))));
	BOOST_TEST((k::Point(d, k::Position(0u, 2u)) < k::Point(d, k::Position(1u, 0u))));

	k::Document d2;
	BOOST_TEST((k::Point(d, k::Position::zero()) == k::Point(d2, k::Position::zero())));
}

BOOST_FIXTURE_TEST_CASE(adaption_test, Fixture) {
	k::Point p(d, k::Position(1u, 2u));
	BOOST_REQUIRE(p.adaptsToDocument());
	BOOST_REQUIRE(p.position() == k::Position(1u, 2u));

	k::insert(d, k::Position::zero(), fromLatin1("0"));	// insert a character in the line before 'p'
	BOOST_TEST(p.position() == k::Position(1u, 2u));
	k::insert(d, k::Position(2u, 3u), fromLatin1("z"));	// insert a character in the line after 'p'
	BOOST_TEST(p.position() == k::Position(1u, 2u));

	k::erase(d, k::Region::makeSingleLine(0u, boost::irange(3u, 4u)));	// erase a character after 'p'
	BOOST_TEST(p.position() == k::Position(1u, 2u));
	k::erase(d, k::Region::makeSingleLine(2u, boost::irange(0u, 1u)));	// erase a character after 'p'
	BOOST_TEST(p.position() == k::Position(1u, 2u));

	k::insert(d, k::Position(1u, 0u), fromLatin1("BC"));	// insert two characters before 'p'
	BOOST_TEST(p.position() == k::Position(1u, 4u));
	k::erase(d, k::Region::makeSingleLine(1u, boost::irange(1u, 3u)));	// erase two characters before 'p'
	BOOST_TEST(p.position() == k::Position(1u, 2u));

	BOOST_REQUIRE(d.lineLength(0u) == 3u);
	k::erase(d, k::Region(k::Position(0u, 3u), k::Position::bol(1u)));	// join line 0 and line 1 before 'p'
	BOOST_TEST(p.position() == k::Position(0u, 5u));

	k::insert(d, k::Position::zero(), fromLatin1("111\n222\n333"));	// insert two lines before 'p'
	BOOST_TEST(p.position() == k::Position(2u, 8u));

	BOOST_REQUIRE(p.gravity() == ascension::Direction::forward());
	k::insert(d, p.position(), fromLatin1("F"));	// insert a character at 'p'
	BOOST_TEST(p.position() == k::Position(2u, 9u));
	p.setGravity(ascension::Direction::backward());
	k::insert(d, p.position(), fromLatin1("B"));	// insert a character at 'p'
	BOOST_TEST(p.position() == k::Position(2u, 9u));

	k::erase(d, k::Region::makeSingleLine(2u, boost::irange(8u, 10u)));	// erase a region encompasses 'p'
	BOOST_TEST(p.position() == k::Position(2u, 8u));

	p.adaptToDocument(false);
	d.resetContent();
	BOOST_TEST(p.position() == k::Position(2u, 8u));
	p.adaptToDocument(true);
	d.resetContent();
	BOOST_TEST(p.position() == k::Position::zero());
}

BOOST_FIXTURE_TEST_CASE(motion_test, Fixture) {
	k::Point p(d);

	const auto& self = p.moveTo(k::Position(1u, 2u));
	BOOST_TEST(p.position() == k::Position(1u, 2u));
	BOOST_TEST(&self == &p);

	BOOST_CHECK_THROW(p.moveTo(k::Position(3u, 4u)), k::BadPositionException);
}
