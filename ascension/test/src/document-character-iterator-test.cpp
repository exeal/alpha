#define BOOST_TEST_MODULE document_character_iterator_test
#include <boost/test/included/unit_test.hpp>

#include <ascension/kernel/document.hpp>
#include <ascension/kernel/document-character-iterator.hpp>
#include "from-latin1.hpp"

namespace k = ascension::kernel;

BOOST_AUTO_TEST_CASE(default_construction_test) {
	k::DocumentCharacterIterator i1, i2;
	BOOST_TEST((i1 == i2));
}

BOOST_AUTO_TEST_CASE(construction_with_parameters_test) {
	const auto bob(k::Position::zero());
	const auto outside(k::Position::bol(2u));
	k::Document d;
	k::insert(d, bob, fromLatin1("abc\ndef"));

	BOOST_CHECK_THROW(k::DocumentCharacterIterator(d, outside), k::BadPositionException);
	const k::DocumentCharacterIterator i1(d, *boost::const_begin(d.region()));
	BOOST_TEST(&i1.document() == &d);
	BOOST_TEST((i1.region() == d.region()));
	BOOST_TEST(i1.tell() == bob);
	BOOST_TEST(i1.offset() == 0);
	BOOST_TEST(i1.lineString() == fromLatin1("abc"));

	BOOST_CHECK_THROW(k::DocumentCharacterIterator(d, k::Region(bob, outside)), k::BadRegionException);
	const k::Region region(k::Position(0u, 2u), k::Position(1u, 1u));
	const k::DocumentCharacterIterator i2(d, region);
	BOOST_TEST(&i2.document() == &d);
	BOOST_TEST((i2.region() == region));
	BOOST_TEST(i2.tell() == *boost::const_begin(region));
	BOOST_TEST(i2.offset() == 0);
	BOOST_TEST(i2.lineString() == fromLatin1("abc"));

	BOOST_CHECK_THROW(k::DocumentCharacterIterator(d, d.region(), outside), k::BadPositionException);
	BOOST_CHECK_THROW(k::DocumentCharacterIterator(d, k::Region(bob, outside), k::Position::zero()), k::BadRegionException);
	BOOST_CHECK_THROW(k::DocumentCharacterIterator(d, k::Region::makeSingleLine(0u, boost::irange(0u, 3u)), k::Position::bol(1u)), k::BadPositionException);
	const k::Position p(1u, 0u);
	const k::DocumentCharacterIterator i3(d, region, p);
	BOOST_TEST(&i3.document() == &d);
	BOOST_TEST((i3.region() == region));
	BOOST_TEST(i3.tell() == p);
	BOOST_TEST(i3.offset() == 0);
	BOOST_TEST(i3.lineString() == fromLatin1("def"));
}

BOOST_AUTO_TEST_CASE(assignment_test) {
	k::Document d;
	k::insert(d, k::Position::zero(), fromLatin1("xyzzy"));

	const k::DocumentCharacterIterator i(d, k::Region::makeSingleLine(0u, boost::irange(2u, 4u)), k::Position(0u, 3u));
	k::DocumentCharacterIterator i2;
	i2 = i;

	BOOST_TEST(&i2.document() == &i.document());
	BOOST_TEST((i2.region() == i.region()));
	BOOST_TEST(i2.tell() == i.tell());
	BOOST_TEST(i2.offset() == i.offset());
}

BOOST_AUTO_TEST_CASE(factories_test) {
	k::Document d;
	k::insert(d, k::Position::zero(), fromLatin1("xyzzy"));

	auto i(k::begin(d));
	BOOST_TEST(&i.document() == &d);
	BOOST_TEST((i.region() == d.region()));
	BOOST_TEST(i.tell() == *boost::const_begin(d.region()));
	BOOST_TEST(i.offset() == 0);

	i = k::cbegin(d);
	BOOST_TEST(&i.document() == &d);
	BOOST_TEST((i.region() == d.region()));
	BOOST_TEST(i.tell() == *boost::const_begin(d.region()));
	BOOST_TEST(i.offset() == 0);

	i = k::end(d);
	BOOST_TEST(&i.document() == &d);
	BOOST_TEST((i.region() == d.region()));
	BOOST_TEST(i.tell() == *boost::const_end(d.region()));
	BOOST_TEST(i.offset() == 0);

	i = k::cend(d);
	BOOST_TEST(&i.document() == &d);
	BOOST_TEST((i.region() == d.region()));
	BOOST_TEST(i.tell() == *boost::const_end(d.region()));
	BOOST_TEST(i.offset() == 0);

	using std::begin;
	using std::end;
	begin(d);
	end(d);
#if __cplusplus >= 201402L
	using std::cbegin;
	using std::cend;
	cbegin(d);
	cend(d);
#endif
}

BOOST_AUTO_TEST_CASE(equality_test) {
	k::Document d1, d2;
	k::insert(d1, k::Position::zero(), fromLatin1("xyzzy"));
	k::insert(d2, k::Position::zero(), fromLatin1("xyzzy"));

	BOOST_TEST((k::cbegin(d1) == k::cbegin(d1)));
	BOOST_TEST((k::cbegin(d1) != k::cend(d1)));
	BOOST_TEST((k::cbegin(d1) != k::cbegin(d2)));
}

BOOST_AUTO_TEST_CASE(iteration_test) {
	k::Document d;
	k::insert(d, k::Position::zero(), fromLatin1("abc\ndef"));

	k::DocumentCharacterIterator i1(d, k::Position::zero());
	BOOST_REQUIRE(i1.offset() == 0u);
	BOOST_REQUIRE(i1.tell() == k::Position::zero());
	BOOST_TEST(k::position(i1) == k::Position::zero());
	BOOST_TEST(*i1 == static_cast<ascension::CodePoint>('a'));
	BOOST_TEST(i1.hasNext());
	BOOST_TEST(!i1.hasPrevious());

	++i1;
	BOOST_TEST(i1.offset() == 1);
	BOOST_TEST(i1.tell() == k::Position(0u, 1u));
	BOOST_TEST(k::position(i1) == k::Position(0u, 1u));
	BOOST_TEST(*i1 == static_cast<ascension::CodePoint>('b'));
	BOOST_TEST(i1.hasNext());
	BOOST_TEST(i1.hasPrevious());

	--i1;
	BOOST_TEST(i1.offset() == 0);
	BOOST_TEST(i1.tell() == k::Position::zero());
	BOOST_TEST(k::position(i1) == k::Position::zero());
	BOOST_TEST(*i1 == static_cast<ascension::CodePoint>('a'));
	BOOST_TEST(i1.hasNext());
	BOOST_TEST(!i1.hasPrevious());

	std::advance(i1, 7);
	BOOST_TEST(i1.offset() == 7);
	BOOST_TEST(i1.tell() == k::Position(1u, 3u));
	BOOST_TEST(k::position(i1) == k::Position(1u, 3u));
	BOOST_TEST(!i1.hasNext());
	BOOST_TEST(i1.hasPrevious());

	for(int i = 0; i < 4; ++i)
		--i1;
	BOOST_TEST(i1.offset() == 3);
	BOOST_TEST(i1.tell() == k::Position(0u, 3u));
	BOOST_TEST(k::position(i1) == k::Position(0u, 3u));
	BOOST_TEST(*i1 == ascension::text::LINE_SEPARATOR);
	BOOST_TEST(i1.hasNext());
	BOOST_TEST(i1.hasPrevious());

	const auto region(k::Region::makeSingleLine(0u, boost::irange(1u, 3u)));
	k::DocumentCharacterIterator i2(d, region, *boost::const_begin(region));
	BOOST_TEST(i2.hasNext());
	BOOST_TEST(!i2.hasPrevious());

	BOOST_CHECK_THROW(--i2, ascension::NoSuchElementException);
	BOOST_TEST(i2.tell() == *boost::const_begin(region));

	BOOST_CHECK_THROW(std::advance(i2, 100), ascension::NoSuchElementException);
	BOOST_TEST(i2.tell() == *boost::const_end(region));
	BOOST_TEST(*i2 == ascension::text::INVALID_CODE_POINT);
}

BOOST_AUTO_TEST_CASE(copy_construction_test) {
	k::Document d;
	k::insert(d, k::Position::zero(), fromLatin1("abc"));
	k::DocumentCharacterIterator i(d, k::Position::zero());
	std::advance(i, 2);

	k::DocumentCharacterIterator i2(i);
	BOOST_TEST((i2 == i));
	BOOST_TEST(*i2 == *i);
	BOOST_TEST(&i2.document() == &i.document());
	BOOST_TEST(i2.offset() == 0);
	BOOST_TEST(i2.tell() == i.tell());
	BOOST_TEST(k::position(i2) == k::position(i));
	BOOST_TEST((i2.region() == i.region()));
}

BOOST_AUTO_TEST_CASE(seek_test) {
	k::Document d;
	k::insert(d, k::Position::zero(), fromLatin1("abc\ndef"));
	const k::Region region(k::Position(0u, 1u), k::Position(1u, 2u));
	k::DocumentCharacterIterator i(d, region, *boost::const_begin(region));

	i.seek(k::Position::bol(1u));
	BOOST_TEST(i.tell() == k::Position(1u, 0u));
	BOOST_TEST(i.offset() == 0);

	i.seek(k::Position::bol(1u));
	BOOST_TEST(i.tell() == k::Position(1u, 0u));
	BOOST_TEST(i.offset() == 0);

	BOOST_CHECK_THROW(i.seek(k::Position::bol(2u)), k::BadPositionException);
}

BOOST_AUTO_TEST_CASE(region_update_test) {
	k::Document d;
	k::insert(d, k::Position::zero(), fromLatin1("xyzzy"));
	auto i(k::cbegin(d));
	BOOST_REQUIRE(i.tell() == k::Position::zero());

	const auto scope(k::Region::makeSingleLine(0u, boost::irange(2u, 4u)));
	i.setRegion(scope);
	BOOST_TEST((i.region() == scope));
	BOOST_TEST(i.tell() == *boost::const_begin(scope));
}
