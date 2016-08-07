#define BOOST_TEST_MODULE line_relative_directional_test
#include <boost/test/included/unit_test.hpp>

#include <ascension/graphics/font/line-relative-four-sides.hpp>
#include <ascension/graphics/font/line-relative-point.hpp>

BOOST_AUTO_TEST_CASE(negation_test) {
	BOOST_TEST(
		static_cast<int>(!ascension::graphics::font::LineRelativeDirection::OVER)
		== static_cast<int>(ascension::graphics::font::LineRelativeDirection::UNDER));
	BOOST_TEST(
		static_cast<int>(!ascension::graphics::font::LineRelativeDirection::UNDER)
		== static_cast<int>(ascension::graphics::font::LineRelativeDirection::OVER));
	BOOST_TEST(
		static_cast<int>(!ascension::graphics::font::LineRelativeDirection::LINE_LEFT)
		== static_cast<int>(ascension::graphics::font::LineRelativeDirection::LINE_RIGHT));
	BOOST_TEST(
		static_cast<int>(!ascension::graphics::font::LineRelativeDirection::LINE_RIGHT)
		== static_cast<int>(ascension::graphics::font::LineRelativeDirection::LINE_LEFT));
	BOOST_TEST(
		static_cast<int>(!ascension::graphics::font::LineRelativeDirection::LINE_OVER)
		== static_cast<int>(ascension::graphics::font::LineRelativeDirection::LINE_UNDER));
	BOOST_TEST(
		static_cast<int>(!ascension::graphics::font::LineRelativeDirection::LINE_UNDER)
		== static_cast<int>(ascension::graphics::font::LineRelativeDirection::LINE_OVER));
}

BOOST_AUTO_TEST_SUITE(two_axes)
	BOOST_AUTO_TEST_CASE(construction_test) {
		const ascension::graphics::font::LineRelativePoint<int> p1(23, 42);
		BOOST_TEST(p1.u() == 23);
		BOOST_TEST(p1.v() == 42);

		const ascension::graphics::font::LineRelativePoint<int> p2(ascension::graphics::font::_u = 23, ascension::graphics::font::_v = 42);
		BOOST_TEST(p2.u() == 23);
		BOOST_TEST(p2.v() == 42);

		const ascension::graphics::font::LineRelativePoint<int> partiallyOptional1(ascension::graphics::font::_u = 23);
		const ascension::graphics::font::LineRelativePoint<int> partiallyOptional2(ascension::graphics::font::_v = 42);

		const auto p3(ascension::graphics::font::makeLineRelativePoint((ascension::graphics::font::_u = 23, ascension::graphics::font::_v = 42)));
		BOOST_TEST(p3.u() == 23);
		BOOST_TEST(p3.v() == 42);
	}

	BOOST_AUTO_TEST_CASE(value_assignment_test) {
		ascension::graphics::font::LineRelativePoint<int> p;
		p.u() = 23;
		p.v() = 42;
		BOOST_TEST(p.u() == 23);
		BOOST_TEST(p.v() == 42);

		p.u() += p.v();
		p.u() -= p.v();
		p.u() *= p.v();
		p.u() /= p.v();
		p.u() %= p.v();
		++p.u();
		--p.v();
	}

	BOOST_AUTO_TEST_CASE(additive_test) {
		ascension::graphics::font::LineRelativePoint<int> p1(ascension::graphics::font::_u = 2, ascension::graphics::font::_v = 3);
		ascension::graphics::font::LineRelativePoint<int> p2(ascension::graphics::font::_u = 3, ascension::graphics::font::_v = 2);
		const ascension::graphics::font::LineRelativePoint<int> d(ascension::graphics::font::_u = 10, ascension::graphics::font::_v = 10);

		const auto p3(p1 + p2);
		BOOST_TEST(p3.u() == p1.u() + p2.u());
		BOOST_TEST(p3.v() == p1.v() + p2.v());

		const auto p4(p1 - p2);
		BOOST_TEST(p4.u() == p1.u() - p2.u());
		BOOST_TEST(p4.v() == p1.v() - p2.v());

		p1 += d;
		BOOST_TEST(p1.u() == 12);
		BOOST_TEST(p1.v() == 13);

		p2 -= d;
		BOOST_TEST(p2.u() == -7);
		BOOST_TEST(p2.v() == -8);
	}
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(four_sides)
	BOOST_AUTO_TEST_CASE(construction_test) {
		const ascension::graphics::font::LineRelativeFourSides<int> box1(
			ascension::graphics::font::_lineOver = 1, ascension::graphics::font::_lineUnder = 2,
			ascension::graphics::font::_lineLeft = 3, ascension::graphics::font::_lineRight = 4);
		BOOST_TEST(box1.lineOver() == 1);
		BOOST_TEST(box1.lineUnder() == 2);
		BOOST_TEST(box1.lineLeft() == 3);
		BOOST_TEST(box1.lineRight() == 4);

		const auto box2(ascension::graphics::font::makeLineRelativeFourSides((
			ascension::graphics::font::_lineOver = 1, ascension::graphics::font::_lineUnder = 2,
			ascension::graphics::font::_lineLeft = 3, ascension::graphics::font::_lineRight = 4)));
		BOOST_CHECK_EQUAL_COLLECTIONS(box2.cbegin(), box2.cend(), box1.cbegin(), box1.cend());

		BOOST_TEST(ascension::graphics::font::LineRelativeFourSides<int>(ascension::graphics::font::_lineOver = 42).lineOver() == 42);
	}

	BOOST_AUTO_TEST_CASE(value_assignment_test) {
		ascension::graphics::font::LineRelativeFourSides<int> box;
		box.lineOver() = 1;
		box.lineUnder() = 2;
		box.lineLeft() = 3;
		box.lineRight() = 4;
		BOOST_TEST(box.lineOver() == 1);
		BOOST_TEST(box.lineUnder() == 2);
		BOOST_TEST(box.lineLeft() == 3);
		BOOST_TEST(box.lineRight() == 4);

		box.lineOver() += box.lineUnder();
		box.lineOver() -= box.lineUnder();
		box.lineOver() *= box.lineUnder();
		box.lineOver() /= box.lineUnder();
		box.lineOver() %= box.lineUnder();
		++box.lineOver();
		--box.lineOver();
	}
/*
	BOOST_AUTO_TEST_CASE(additive_test) {
		auto box1(ascension::graphics::font::makeLineRelativeFourSides((
			ascension::graphics::font::_over = 1, ascension::graphics::font::_lineRight = 22,
			ascension::graphics::font::_under = 11, ascension::graphics::font::_lineLeft = 2)));
		auto d(ascension::graphics::font::makeLineRelativePoint((
			ascension::graphics::font::_u = 1, ascension::graphics::font::_v = 10)));

		const auto box2(box1 + d);
		BOOST_TEST(box2.over() == box1.lineOver() + d.v());
		BOOST_TEST(box2.lineRight() == box1.lineRight() + d.u());
		BOOST_TEST(box2.under() == box1.lineUnder() + d.v());
		BOOST_TEST(box2.lineLeft() == box1.lineLeft() + d.u());

		const auto box3(box1 - d);
		BOOST_TEST(box3.over() == box1.lineOver() - d.y());
		BOOST_TEST(box3.lineRight() == box1.lineRight() - d.x());
		BOOST_TEST(box3.under() == box1.lineUnder() - d.y());
		BOOST_TEST(box3.lineLeft() == box1.lineLeft() - d.x());

		box1 += d;
		BOOST_TEST(box1.over() == box2.lineOver());
		BOOST_TEST(box1.lineRight() == box2.lineRight());
		BOOST_TEST(box1.under() == box2.lineUnder());
		BOOST_TEST(box1.lineLeft() == box2.lineLeft());

		box1 -= d;
		BOOST_TEST(box1.over() == 1);
		BOOST_TEST(box1.lineRight() == 22);
		BOOST_TEST(box1.under() == 11);
		BOOST_TEST(box1.lineLeft() == 2);
	}
*/
	BOOST_AUTO_TEST_CASE(range_test) {
		const int lineOver = 1, lineLeft = 2, lineUnder = 11, lineRight = 22;
		const auto box(ascension::graphics::font::makeLineRelativeFourSides((
			ascension::graphics::font::_lineOver = lineOver, ascension::graphics::font::_lineRight = lineRight,
			ascension::graphics::font::_lineUnder = lineUnder, ascension::graphics::font::_lineLeft = lineLeft)));

		BOOST_TEST(ascension::graphics::font::measure(box) == lineRight - lineLeft);
		BOOST_TEST(ascension::graphics::font::extent(box) == lineUnder - lineOver);
	}
BOOST_AUTO_TEST_SUITE_END()
