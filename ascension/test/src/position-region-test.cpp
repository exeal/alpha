#define BOOST_TEST_MODULE position_region_test
#include <boost/test/included/unit_test.hpp>

#include <ascension/kernel/position.hpp>
#include <ascension/kernel/region.hpp>
#include <array>

namespace k = ascension::kernel;

BOOST_AUTO_TEST_SUITE(position)
	BOOST_AUTO_TEST_CASE(construction_test) {
		const k::Position d;	// just default-constructible

		const k::Position p(23u, 42u);
		BOOST_TEST(p.line == 23u);
		BOOST_TEST(p.offsetInLine == 42u);

		const k::Position p2(p);
		BOOST_TEST(p2.line == 23u);
		BOOST_TEST(p2.offsetInLine == 42u);
	}

	BOOST_AUTO_TEST_CASE(copy_assignment_test) {
		const k::Position p(23u, 42u);
		k::Position p2;
		p2 = p;
		BOOST_TEST(p2.line == 23u);
		BOOST_TEST(p2.offsetInLine == 42u);
	}

	BOOST_AUTO_TEST_CASE(comparisons_test) {
		const k::Position p(23u, 42u);
		BOOST_TEST(p == p);
		BOOST_TEST(!(p == k::Position(99u, 42u)));
		BOOST_TEST(!(p == k::Position(23u, 99u)));

		BOOST_TEST(!(p != p));
		BOOST_TEST(p != k::Position(99u, 42u));
		BOOST_TEST(p != k::Position(23u, 99u));

		BOOST_TEST(!(p < p));
		BOOST_TEST(p < k::Position(24u, 42u));
		BOOST_TEST(p < k::Position(23u, 43u));
		BOOST_TEST(!(p < k::Position(22u, 42u)));
		BOOST_TEST(!(p < k::Position(23u, 41u)));

		BOOST_TEST(p <= p);
		BOOST_TEST(p <= k::Position(24u, 42u));
		BOOST_TEST(p <= k::Position(23u, 43u));
		BOOST_TEST(!(p <= k::Position(22u, 42u)));
		BOOST_TEST(!(p <= k::Position(23u, 41u)));

		BOOST_TEST(!(p > p));
		BOOST_TEST(!(p > k::Position(24u, 42u)));
		BOOST_TEST(!(p > k::Position(23u, 43u)));
		BOOST_TEST(p > k::Position(22u, 42u));
		BOOST_TEST(p > k::Position(23u, 41u));

		BOOST_TEST(p >= p);
		BOOST_TEST(!(p >= k::Position(24u, 42u)));
		BOOST_TEST(!(p >= k::Position(23u, 43u)));
		BOOST_TEST(p >= k::Position(22u, 42u));
		BOOST_TEST(p >= k::Position(23u, 41u));
	}

	BOOST_AUTO_TEST_CASE(factories_test) {
		BOOST_TEST(k::Position::zero() == k::Position(0u, 0u));

		BOOST_TEST(k::Position::bol(42u) == k::Position(42u, 0u));
		BOOST_TEST(k::Position::bol(k::Position(23u, 42u)) == k::Position(23u, 0u));
	}
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(region)
	BOOST_AUTO_TEST_CASE(construction_test) {
		const k::Region d;	// just default-constructible

		const k::Region r(k::Position(1u, 2u), k::Position(3u, 4u));
		BOOST_TEST(*boost::const_begin(r) == k::Position(1u, 2u));
		BOOST_TEST(*boost::const_end(r) == k::Position(3u, 4u));

		const k::Region r2(r);
		BOOST_TEST(*boost::const_begin(r2) == k::Position(1u, 2u));
		BOOST_TEST(*boost::const_end(r2) == k::Position(3u, 4u));

		const k::Region r3(k::Position(3u, 4u), k::Position(1u, 2u));
		BOOST_TEST(*boost::const_begin(r3) == k::Position(1u, 2u));
		BOOST_TEST(*boost::const_end(r3) == k::Position(3u, 4u));
	}

	BOOST_AUTO_TEST_CASE(copy_assignment_test) {
		const k::Region r(k::Position(1u, 2u), k::Position(3u, 4u));
		k::Region r2;
		r2 = r;
		BOOST_TEST(*boost::const_begin(r2) == k::Position(1u, 2u));
		BOOST_TEST(*boost::const_end(r2) == k::Position(3u, 4u));
	}

	BOOST_AUTO_TEST_CASE(comparisons_test) {
		const k::Region r(k::Position(1u, 2u), k::Position(3u, 4u));

		bool eq = r == k::Region(k::Position(1u, 2u), k::Position(3u, 4u));
		BOOST_TEST(eq);
		eq = r == k::Region(k::Position(0u, 0u), k::Position(3u, 4u));
		BOOST_TEST(!eq);
		eq = r == k::Region(k::Position(1u, 2u), k::Position(33u, 44u));
		BOOST_TEST(!eq);

		bool ne = r != k::Region(k::Position(1u, 2u), k::Position(3u, 4u));
		BOOST_TEST(!ne);
		ne = r != k::Region(k::Position(0u, 0u), k::Position(3u, 4u));
		BOOST_TEST(ne);
		ne = r != k::Region(k::Position(1u, 2u), k::Position(33u, 44u));
		BOOST_TEST(ne);
	}

	BOOST_AUTO_TEST_CASE(factories_test) {
		const std::array<k::Position, 2> positions = {{k::Position(1u, 2u), k::Position(3u, 4u)}};
		BOOST_CHECK_EQUAL(
			k::Region::fromRange(k::Region(std::get<0>(positions), std::get<1>(positions))),
			k::Region(std::get<0>(positions), std::get<1>(positions)));
		BOOST_CHECK_EQUAL(
			k::Region::fromTuple(positions),
			k::Region(std::get<0>(positions), std::get<1>(positions)));

		const k::Position p(23u, 42u);
		BOOST_CHECK_EQUAL(k::Region::makeEmpty(p), k::Region(p, p));

		BOOST_CHECK_EQUAL(
			k::Region::makeSingleLine(1u, boost::irange(2u, 3u)),
			k::Region(k::Position(1u, 2u), k::Position(1u, 3u)));

		BOOST_CHECK_EQUAL(
			k::Region::zero(),
			k::Region(k::Position(0u, 0u), k::Position(0u, 0u)));
	}

	BOOST_AUTO_TEST_CASE(line_counting_test) {
		BOOST_TEST(*boost::const_begin(k::Region::zero().lines()) == 0u);
		BOOST_TEST(*boost::const_end(k::Region::zero().lines()) == 1u);

		const k::Region r(k::Position(1u, 2u), k::Position(3u, 4u));
		BOOST_TEST(*boost::const_begin(r.lines()) == 1u);
		BOOST_TEST(*boost::const_end(r.lines()) == 4u);
		BOOST_TEST(boost::size(r.lines()) == 3u);
	}
BOOST_AUTO_TEST_SUITE_END()
