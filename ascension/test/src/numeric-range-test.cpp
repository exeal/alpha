#define BOOST_TEST_MODULE numeric_range_test
#include <boost/test/included/unit_test.hpp>

#include <ascension/corelib/numeric-range.hpp>
#include <ascension/corelib/numeric-range-algorithm/clamp.hpp>
#include <ascension/corelib/numeric-range-algorithm/encompasses.hpp>
#include <ascension/corelib/numeric-range-algorithm/hull.hpp>
#include <ascension/corelib/numeric-range-algorithm/includes.hpp>
#include <ascension/corelib/numeric-range-algorithm/intersection.hpp>
#include <ascension/corelib/numeric-range-algorithm/order.hpp>
#include <ascension/corelib/numeric-range-algorithm/overlaps.hpp>

namespace {
	struct Fixture {
		const ascension::NumericRange<int> bounds;
		ascension::NumericRange<int> ranges[9];

		Fixture() BOOST_NOEXCEPT : bounds(ascension::nrange(23, 42)) {
			ranges[0] = ascension::nrange(-9, 11);
			ranges[1] = ascension::nrange(-9, 23);
			ranges[2] = ascension::nrange(11, 31);
			ranges[3] = ascension::nrange(23, 31);
			ranges[4] = ascension::nrange(23, 42);
			ranges[5] = ascension::nrange(31, 31);
			ranges[6] = ascension::nrange(31, 42);
			ranges[7] = ascension::nrange(31, 99);
			ranges[8] = ascension::nrange(99, 99);
		}
	};
}

namespace boost {
	std::ostream& operator<<(std::ostream& out, const optional<ascension::NumericRange<int>>& object) {
		return out << "<" << ((object == boost::none) ? "none" : "not-none") << ">";
	}
}

BOOST_AUTO_TEST_SUITE(construction)
	BOOST_AUTO_TEST_CASE(default_constructor_test) {
		ascension::NumericRange<int> nr;
		BOOST_TEST(boost::empty(nr));

		ascension::NumericRange<double> nr2;
		BOOST_TEST(boost::empty(nr2));
	}

	BOOST_AUTO_TEST_CASE(constructor_with_values_test) {
		ascension::NumericRange<int> nr(23, 42);
		BOOST_TEST(*boost::begin(nr) == 23);
		BOOST_TEST(*boost::end(nr) == 42);
		BOOST_TEST(*boost::const_begin(nr) == 23);
		BOOST_TEST(*boost::const_end(nr) == 42);

		nr = ascension::NumericRange<int>(42, 23);
		BOOST_TEST(*boost::const_begin(nr) == 42);
		BOOST_TEST(*boost::const_end(nr) == 23);
	}

	BOOST_AUTO_TEST_CASE(copy_constructor_test) {
		const ascension::NumericRange<int> nr(23, 42);
		const auto nr2(nr);
		BOOST_TEST(*boost::const_begin(nr) == *boost::const_begin(nr2));
		BOOST_TEST(*boost::const_end(nr) == *boost::const_end(nr2));
	}

	BOOST_AUTO_TEST_CASE(nrange_test) {
		auto nr(ascension::nrange(23, 42));
		BOOST_TEST(*boost::const_begin(nr) == 23);
		BOOST_TEST(*boost::const_end(nr) == 42);
	}
BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(algorithms, Fixture)
	BOOST_AUTO_TEST_CASE(clamp_test) {
		BOOST_TEST(ascension::clamp(-9, bounds) == 23);
		BOOST_TEST(ascension::clamp(23, bounds) == 23);
		BOOST_TEST(ascension::clamp(31, bounds) == 31);
		BOOST_TEST(ascension::clamp(42, bounds) == 42);
		BOOST_TEST(ascension::clamp(99, bounds) == 42);

		// oops, BOOST_TEST(a == b) is ambiguous in these context...
		BOOST_CHECK_EQUAL(ascension::clampRange(ranges[0], bounds), ascension::nrange(23, 23));
		BOOST_CHECK_EQUAL(ascension::clampRange(ranges[1], bounds), ascension::nrange(23, 23));
		BOOST_CHECK_EQUAL(ascension::clampRange(ranges[2], bounds), ascension::nrange(23, 31));
		BOOST_CHECK_EQUAL(ascension::clampRange(ranges[3], bounds), ascension::nrange(23, 31));
		BOOST_CHECK_EQUAL(ascension::clampRange(ranges[4], bounds), ascension::nrange(23, 42));
		BOOST_CHECK_EQUAL(ascension::clampRange(ranges[5], bounds), ascension::nrange(31, 31));
		BOOST_CHECK_EQUAL(ascension::clampRange(ranges[6], bounds), ascension::nrange(31, 42));
		BOOST_CHECK_EQUAL(ascension::clampRange(ranges[7], bounds), ascension::nrange(31, 42));
		BOOST_CHECK_EQUAL(ascension::clampRange(ranges[8], bounds), ascension::nrange(42, 42));

		BOOST_CHECK_EQUAL((ranges[0] | ascension::adaptors::clamped(bounds)), ascension::nrange(23, 23));
		BOOST_CHECK_EQUAL((ranges[1] | ascension::adaptors::clamped(bounds)), ascension::nrange(23, 23));
		BOOST_CHECK_EQUAL((ranges[2] | ascension::adaptors::clamped(bounds)), ascension::nrange(23, 31));
		BOOST_CHECK_EQUAL((ranges[3] | ascension::adaptors::clamped(bounds)), ascension::nrange(23, 31));
		BOOST_CHECK_EQUAL((ranges[4] | ascension::adaptors::clamped(bounds)), ascension::nrange(23, 42));
		BOOST_CHECK_EQUAL((ranges[5] | ascension::adaptors::clamped(bounds)), ascension::nrange(31, 31));
		BOOST_CHECK_EQUAL((ranges[6] | ascension::adaptors::clamped(bounds)), ascension::nrange(31, 42));
		BOOST_CHECK_EQUAL((ranges[7] | ascension::adaptors::clamped(bounds)), ascension::nrange(31, 42));
		BOOST_CHECK_EQUAL((ranges[8] | ascension::adaptors::clamped(bounds)), ascension::nrange(42, 42));
	}

	BOOST_AUTO_TEST_CASE(encompasses_test) {
		BOOST_TEST(!ascension::encompasses(bounds, 11));
		BOOST_TEST( ascension::encompasses(bounds, 23));
		BOOST_TEST( ascension::encompasses(bounds, 31));
		BOOST_TEST( ascension::encompasses(bounds, 42));
		BOOST_TEST(!ascension::encompasses(bounds, 99));

		BOOST_TEST(!ascension::encompasses(bounds, ranges[0]));
		BOOST_TEST(!ascension::encompasses(bounds, ranges[1]));
		BOOST_TEST(!ascension::encompasses(bounds, ranges[2]));
		BOOST_TEST( ascension::encompasses(bounds, ranges[3]));
		BOOST_TEST( ascension::encompasses(bounds, ranges[4]));
		BOOST_TEST( ascension::encompasses(bounds, ranges[5]));
		BOOST_TEST( ascension::encompasses(bounds, ranges[6]));
		BOOST_TEST(!ascension::encompasses(bounds, ranges[7]));
		BOOST_TEST(!ascension::encompasses(bounds, ranges[8]));
	}

	BOOST_AUTO_TEST_CASE(hull_test) {
		BOOST_CHECK_EQUAL(ascension::hull(ranges[0], bounds), ascension::nrange(-9, 42));
		BOOST_CHECK_EQUAL(ascension::hull(ranges[1], bounds), ascension::nrange(-9, 42));
		BOOST_CHECK_EQUAL(ascension::hull(ranges[2], bounds), ascension::nrange(11, 42));
		BOOST_CHECK_EQUAL(ascension::hull(ranges[3], bounds), ascension::nrange(23, 42));
		BOOST_CHECK_EQUAL(ascension::hull(ranges[4], bounds), ascension::nrange(23, 42));
		BOOST_CHECK_EQUAL(ascension::hull(ranges[5], bounds), ascension::nrange(23, 42));
		BOOST_CHECK_EQUAL(ascension::hull(ranges[6], bounds), ascension::nrange(23, 42));
		BOOST_CHECK_EQUAL(ascension::hull(ranges[7], bounds), ascension::nrange(23, 99));
		BOOST_CHECK_EQUAL(ascension::hull(ranges[8], bounds), ascension::nrange(23, 99));
	}

	BOOST_AUTO_TEST_CASE(includes_test) {
		BOOST_TEST(!ascension::includes(bounds, 11));
		BOOST_TEST( ascension::includes(bounds, 23));
		BOOST_TEST( ascension::includes(bounds, 31));
		BOOST_TEST(!ascension::includes(bounds, 42));
		BOOST_TEST(!ascension::includes(bounds, 99));
	}

	BOOST_AUTO_TEST_CASE(intersection_test) {
		auto intersection(ascension::intersection(ranges[0], bounds));
		BOOST_TEST(intersection == boost::none);
		intersection = ascension::intersection(ranges[1], bounds);
		BOOST_TEST(intersection != boost::none);
		BOOST_CHECK_EQUAL(*intersection, ascension::nrange(23, 23));
		intersection = ascension::intersection(ranges[2], bounds);
		BOOST_TEST(intersection != boost::none);
		BOOST_CHECK_EQUAL(*intersection, ascension::nrange(23, 31));
		intersection = ascension::intersection(ranges[3], bounds);
		BOOST_TEST(intersection != boost::none);
		BOOST_CHECK_EQUAL(*intersection, ascension::nrange(23, 31));
		intersection = ascension::intersection(ranges[4], bounds);
		BOOST_TEST(intersection != boost::none);
		BOOST_CHECK_EQUAL(*intersection, ascension::nrange(23, 42));
		intersection = ascension::intersection(ranges[5], bounds);
		BOOST_TEST(intersection != boost::none);
		BOOST_CHECK_EQUAL(*intersection, ascension::nrange(31, 31));
		intersection = ascension::intersection(ranges[6], bounds);
		BOOST_TEST(intersection != boost::none);
		BOOST_CHECK_EQUAL(*intersection, ascension::nrange(31, 42));
		intersection = ascension::intersection(ranges[7], bounds);
		BOOST_TEST(intersection != boost::none);
		BOOST_CHECK_EQUAL(*intersection, ascension::nrange(31, 42));
		intersection = ascension::intersection(ranges[8], bounds);
		BOOST_TEST(intersection == boost::none);
	}

	BOOST_AUTO_TEST_CASE(order_test) {
		BOOST_TEST( ascension::isOrdered(ascension::nrange(23, 42)));
		BOOST_TEST(!ascension::isOrdered(ascension::nrange(42, 23)));
		BOOST_TEST( ascension::isOrdered(ascension::nrange(31, 31)));

		BOOST_CHECK_EQUAL(ascension::order(ascension::nrange(23, 42)), ascension::nrange(23, 42));
		BOOST_CHECK_EQUAL(ascension::order(ascension::nrange(42, 23)), ascension::nrange(23, 42));
		BOOST_CHECK_EQUAL(ascension::order(ascension::nrange(31, 31)), ascension::nrange(31, 31));

		BOOST_CHECK_EQUAL(ascension::nrange(23, 42) | ascension::adaptors::ordered(), ascension::nrange(23, 42));
		BOOST_CHECK_EQUAL(ascension::nrange(42, 23) | ascension::adaptors::ordered(), ascension::nrange(23, 42));
		BOOST_CHECK_EQUAL(ascension::nrange(31, 31) | ascension::adaptors::ordered(), ascension::nrange(31, 31));
	}

	BOOST_AUTO_TEST_CASE(overlaps_test) {
		BOOST_TEST(!ascension::overlaps(ranges[0], bounds));
		BOOST_TEST(!ascension::overlaps(ranges[1], bounds));
		BOOST_TEST( ascension::overlaps(ranges[2], bounds));
		BOOST_TEST( ascension::overlaps(ranges[3], bounds));
		BOOST_TEST( ascension::overlaps(ranges[4], bounds));
		BOOST_TEST( ascension::overlaps(ranges[5], bounds));
		BOOST_TEST( ascension::overlaps(ranges[6], bounds));
		BOOST_TEST( ascension::overlaps(ranges[7], bounds));
		BOOST_TEST(!ascension::overlaps(ranges[8], bounds));
	}
BOOST_AUTO_TEST_SUITE_END()
