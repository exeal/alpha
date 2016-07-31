#define BOOST_TEST_MODULE physical-directional_test
#include <boost/test/included/unit_test.hpp>

#include <ascension/graphics/physical-four-sides.hpp>

BOOST_AUTO_TEST_CASE(negation_test) {
	BOOST_TEST(!ascension::graphics::PhysicalDirection::TOP == ascension::graphics::PhysicalDirection::BOTTOM);
	BOOST_TEST(!ascension::graphics::PhysicalDirection::RIGHT == ascension::graphics::PhysicalDirection::LEFT);
	BOOST_TEST(!ascension::graphics::PhysicalDirection::BOTTOM == ascension::graphics::PhysicalDirection::TOP);
	BOOST_TEST(!ascension::graphics::PhysicalDirection::LEFT == ascension::graphics::PhysicalDirection::RIGHT);
}

BOOST_AUTO_TEST_SUITE(two_axes)
	BOOST_AUTO_TEST_CASE(construction_test) {
		const ascension::graphics::PhysicalTwoAxes<int> p1(23, 42);
		BOOST_TEST(p1.x() == 23);
		BOOST_TEST(p1.y() == 42);

		const ascension::graphics::PhysicalTwoAxes<int> p2(ascension::graphics::_x = 23, ascension::graphics::_y = 42);
		BOOST_TEST(p2.x() == 23);
		BOOST_TEST(p2.y() == 42);

		const ascension::graphics::PhysicalTwoAxes<int> partiallyOptional1(ascension::graphics::_x = 23);
		const ascension::graphics::PhysicalTwoAxes<int> partiallyOptional2(ascension::graphics::_y = 42);

		const auto p3(ascension::graphics::makePhysicalTwoAxes((ascension::graphics::_x = 23, ascension::graphics::_y = 42)));
		BOOST_TEST(p3.x() == 23);
		BOOST_TEST(p3.y() == 42);
	}

	BOOST_AUTO_TEST_CASE(value_assignment_test) {
		ascension::graphics::PhysicalTwoAxes<int> p;
		p.x() = 23;
		p.y() = 42;
		BOOST_TEST(p.x() == 23);
		BOOST_TEST(p.y() == 42);

		++p.x();	// 24
		p.x() /= 4;	// 6
		--p.x();	// 5
		p.x() *= 9;	// 45
		BOOST_TEST(p.x() == ((23 + 1) / 4 - 1) * 9);
		p.y() += 2;	// 44
		p.y() -= 4;	// 40
		BOOST_TEST(p.y() == 42 + 2 - 4);
	}

	BOOST_AUTO_TEST_CASE(additive_test) {
		auto p1(ascension::graphics::makePhysicalTwoAxes((ascension::graphics::_x = 2, ascension::graphics::_y = 3)));
		auto p2(ascension::graphics::makePhysicalTwoAxes((ascension::graphics::_x = 3, ascension::graphics::_y = 2)));
		auto d(ascension::graphics::makePhysicalTwoAxes((ascension::graphics::_x = 10, ascension::graphics::_y = 10)));

		const auto p3(p1 + p2);
		BOOST_TEST(p3.x() == p1.x() + p2.x());
		BOOST_TEST(p3.y() == p1.y() + p2.y());

		const auto p4(p1 - p2);
		BOOST_TEST(p4.x() == p1.x() - p2.x());
		BOOST_TEST(p4.y() == p1.y() - p2.y());

		p1 += d;
		BOOST_TEST(p1.x() == 12);
		BOOST_TEST(p1.y() == 13);

		p2 -= d;
		BOOST_TEST(p2.x() == -7);
		BOOST_TEST(p2.y() == -8);
	}
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(four_sides)
	BOOST_AUTO_TEST_CASE(construction_test) {
		const ascension::graphics::PhysicalFourSides<int> box1(
			ascension::graphics::_top = 1, ascension::graphics::_right = 2,
			ascension::graphics::_bottom = 3, ascension::graphics::_left = 4);
		BOOST_TEST(box1.top() == 1);
		BOOST_TEST(box1.right() == 2);
		BOOST_TEST(box1.bottom() == 3);
		BOOST_TEST(box1.left() == 4);

		const auto box2(ascension::graphics::makePhysicalFourSides((
			ascension::graphics::_top = 1, ascension::graphics::_right = 2,
			ascension::graphics::_bottom = 3, ascension::graphics::_left = 4)));
		BOOST_CHECK_EQUAL_COLLECTIONS(box2.cbegin(), box2.cend(), box1.cbegin(), box1.cend());

		const ascension::graphics::PhysicalFourSides<int> b(ascension::graphics::_top = 42);
		BOOST_TEST(b.top() == 42);
	}

	BOOST_AUTO_TEST_CASE(value_assignment_test) {
		ascension::graphics::PhysicalFourSides<int> box;
		box.top() = 1;
		box.right() = 2;
		box.bottom() = 3;
		box.left() = 4;
		BOOST_TEST(box.top() == 1);
		BOOST_TEST(box.right() == 2);
		BOOST_TEST(box.bottom() == 3);
		BOOST_TEST(box.left() == 4);

		box.top() += box.right();
		box.top() -= box.right();
		box.top() *= box.right();
		box.top() /= box.right();
		box.top() %= box.right();
		++box.top();
		--box.top();
	}

	BOOST_AUTO_TEST_CASE(additive_test) {
		auto box1(ascension::graphics::makePhysicalFourSides((
			ascension::graphics::_top = 1, ascension::graphics::_right = 22,
			ascension::graphics::_bottom = 11, ascension::graphics::_left = 2)));
		auto d(ascension::graphics::makePhysicalTwoAxes((
			ascension::graphics::_x = 1, ascension::graphics::_y = 10)));

		const auto box2(box1 + d);
		BOOST_TEST(box2.top() == box1.top() + d.y());
		BOOST_TEST(box2.right() == box1.right() + d.x());
		BOOST_TEST(box2.bottom() == box1.bottom() + d.y());
		BOOST_TEST(box2.left() == box1.left() + d.x());

		const auto box3(box1 - d);
		BOOST_TEST(box3.top() == box1.top() - d.y());
		BOOST_TEST(box3.right() == box1.right() - d.x());
		BOOST_TEST(box3.bottom() == box1.bottom() - d.y());
		BOOST_TEST(box3.left() == box1.left() - d.x());

		box1 += d;
		BOOST_TEST(box1.top() == box2.top());
		BOOST_TEST(box1.right() == box2.right());
		BOOST_TEST(box1.bottom() == box2.bottom());
		BOOST_TEST(box1.left() == box2.left());

		box1 -= d;
		BOOST_TEST(box1.top() == 1);
		BOOST_TEST(box1.right() == 22);
		BOOST_TEST(box1.bottom() == 11);
		BOOST_TEST(box1.left() == 2);
	}

	BOOST_AUTO_TEST_CASE(range_test) {
		const int top = 1, left = 2, bottom = 11, right = 22;
		const auto box(ascension::graphics::makePhysicalFourSides((
			ascension::graphics::_top = top, ascension::graphics::_right = right,
			ascension::graphics::_bottom = bottom, ascension::graphics::_left = left)));

		BOOST_CHECK_EQUAL(ascension::graphics::horizontalRange(box), ascension::nrange(left, right));
		BOOST_CHECK_EQUAL(ascension::graphics::verticalRange(box), ascension::nrange(top, bottom));

		BOOST_TEST(ascension::graphics::width(box) == right - left);
		BOOST_TEST(ascension::graphics::height(box) == bottom - top);
	}
BOOST_AUTO_TEST_SUITE_END()
