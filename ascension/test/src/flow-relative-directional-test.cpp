#define BOOST_TEST_MODULE flow-relative-directional_test
#include <boost/test/included/unit_test.hpp>

#include <ascension/presentation/flow-relative-four-sides.hpp>

BOOST_AUTO_TEST_CASE(negation_test) {
	BOOST_TEST(!ascension::presentation::FlowRelativeDirection::BLOCK_START == ascension::presentation::FlowRelativeDirection::BLOCK_END);
	BOOST_TEST(!ascension::presentation::FlowRelativeDirection::BLOCK_END == ascension::presentation::FlowRelativeDirection::BLOCK_START);
	BOOST_TEST(!ascension::presentation::FlowRelativeDirection::INLINE_START == ascension::presentation::FlowRelativeDirection::INLINE_END);
	BOOST_TEST(!ascension::presentation::FlowRelativeDirection::INLINE_END == ascension::presentation::FlowRelativeDirection::INLINE_START);
}

BOOST_AUTO_TEST_SUITE(two_axes)
	BOOST_AUTO_TEST_CASE(construction_test) {
		const ascension::presentation::FlowRelativeTwoAxes<int> p1(23, 42);
		BOOST_TEST(p1.bpd() == 23);
		BOOST_TEST(p1.ipd() == 42);

		const ascension::presentation::FlowRelativeTwoAxes<int> p2(ascension::presentation::_bpd = 23, ascension::presentation::_ipd = 42);
		BOOST_TEST(p2.bpd() == 23);
		BOOST_TEST(p2.ipd() == 42);

		const ascension::presentation::FlowRelativeTwoAxes<int> partiallyOptional1(ascension::presentation::_bpd = 23);
		const ascension::presentation::FlowRelativeTwoAxes<int> partiallyOptional2(ascension::presentation::_ipd = 42);

		const auto p3(ascension::presentation::makeFlowRelativeTwoAxes((ascension::presentation::_bpd = 23, ascension::presentation::_ipd = 42)));
		BOOST_TEST(p3.bpd() == 23);
		BOOST_TEST(p3.ipd() == 42);
	}

	BOOST_AUTO_TEST_CASE(value_assignment_test) {
		ascension::presentation::FlowRelativeTwoAxes<int> p;
		p.bpd() = 23;
		p.ipd() = 42;
		BOOST_TEST(p.bpd() == 23);
		BOOST_TEST(p.ipd() == 42);

		++p.bpd();	// 24
		p.bpd() /= 4;	// 6
		--p.bpd();	// 5
		p.bpd() *= 9;	// 45
		BOOST_TEST(p.bpd() == ((23 + 1) / 4 - 1) * 9);
		p.ipd() += 2;	// 44
		p.ipd() -= 4;	// 40
		BOOST_TEST(p.ipd() == 42 + 2 - 4);
	}

	BOOST_AUTO_TEST_CASE(additive_test) {
		auto p1(ascension::presentation::makeFlowRelativeTwoAxes((ascension::presentation::_bpd = 2, ascension::presentation::_ipd = 3)));
		auto p2(ascension::presentation::makeFlowRelativeTwoAxes((ascension::presentation::_bpd = 3, ascension::presentation::_ipd = 2)));
		auto d(ascension::presentation::makeFlowRelativeTwoAxes((ascension::presentation::_bpd = 10, ascension::presentation::_ipd = 10)));

		const auto p3(p1 + p2);
		BOOST_TEST(p3.bpd() == p1.bpd() + p2.bpd());
		BOOST_TEST(p3.ipd() == p1.ipd() + p2.ipd());

		const auto p4(p1 - p2);
		BOOST_TEST(p4.bpd() == p1.bpd() - p2.bpd());
		BOOST_TEST(p4.ipd() == p1.ipd() - p2.ipd());

		p1 += d;
		BOOST_TEST(p1.bpd() == 12);
		BOOST_TEST(p1.ipd() == 13);

		p2 -= d;
		BOOST_TEST(p2.bpd() == -7);
		BOOST_TEST(p2.ipd() == -8);
	}
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(four_sides)
	BOOST_AUTO_TEST_CASE(construction_test) {
		const ascension::presentation::FlowRelativeFourSides<int> box1(
			ascension::presentation::_blockStart = 1, ascension::presentation::_inlineStart = 2,
			ascension::presentation::_blockEnd = 3, ascension::presentation::_inlineEnd = 4);
		BOOST_TEST(box1.blockStart() == 1);
		BOOST_TEST(box1.inlineStart() == 2);
		BOOST_TEST(box1.blockEnd() == 3);
		BOOST_TEST(box1.inlineEnd() == 4);

		const auto box2(ascension::presentation::makeFlowRelativeFourSides((
			ascension::presentation::_blockStart = 1, ascension::presentation::_inlineStart = 2,
			ascension::presentation::_blockEnd = 3, ascension::presentation::_inlineEnd = 4)));
		BOOST_CHECK_EQUAL_COLLECTIONS(box2.cbegin(), box2.cend(), box1.cbegin(), box1.cend());

		const ascension::presentation::FlowRelativeFourSides<int> b(ascension::presentation::_blockStart = 42);
		BOOST_TEST(b.blockStart() == 42);
	}

	BOOST_AUTO_TEST_CASE(value_assignment_test) {
		ascension::presentation::FlowRelativeFourSides<int> box;
		box.blockStart() = 1;
		box.inlineStart() = 2;
		box.blockEnd() = 3;
		box.inlineEnd() = 4;
		BOOST_TEST(box.blockStart() == 1);
		BOOST_TEST(box.inlineStart() == 2);
		BOOST_TEST(box.blockEnd() == 3);
		BOOST_TEST(box.inlineEnd() == 4);

		box.blockStart() += box.inlineStart();
		box.blockStart() -= box.inlineStart();
		box.blockStart() *= box.inlineStart();
		box.blockStart() /= box.inlineStart();
		box.blockStart() %= box.inlineStart();
		++box.blockStart();
		--box.blockStart();
	}

	BOOST_AUTO_TEST_CASE(additive_test) {
		auto box1(ascension::presentation::makeFlowRelativeFourSides((
			ascension::presentation::_blockStart = 1, ascension::presentation::_inlineEnd = 22,
			ascension::presentation::_blockEnd = 11, ascension::presentation::_inlineStart = 2)));
		auto d(ascension::presentation::makeFlowRelativeTwoAxes((
			ascension::presentation::_bpd = 1, ascension::presentation::_ipd = 10)));

		const auto box2(box1 + d);
		BOOST_TEST(box2.blockStart() == box1.blockStart() + d.bpd());
		BOOST_TEST(box2.inlineStart() == box1.inlineStart() + d.ipd());
		BOOST_TEST(box2.blockEnd() == box1.blockEnd() + d.bpd());
		BOOST_TEST(box2.inlineEnd() == box1.inlineEnd() + d.ipd());

		const auto box3(box1 - d);
		BOOST_TEST(box3.blockStart() == box1.blockStart() - d.bpd());
		BOOST_TEST(box3.inlineStart() == box1.inlineStart() - d.ipd());
		BOOST_TEST(box3.blockEnd() == box1.blockEnd() - d.bpd());
		BOOST_TEST(box3.inlineEnd() == box1.inlineEnd() - d.ipd());

		box1 += d;
		BOOST_TEST(box1.blockStart() == box2.blockStart());
		BOOST_TEST(box1.inlineStart() == box2.inlineStart());
		BOOST_TEST(box1.blockEnd() == box2.blockEnd());
		BOOST_TEST(box1.inlineEnd() == box2.inlineEnd());

		box1 -= d;
		BOOST_TEST(box1.blockStart() == 1);
		BOOST_TEST(box1.inlineStart() == 2);
		BOOST_TEST(box1.blockEnd() == 11);
		BOOST_TEST(box1.inlineEnd() == 22);
	}

	BOOST_AUTO_TEST_CASE(range_test) {
		const int bs = 1, is = 2, be = 11, ie = 22;
		const auto box(ascension::presentation::makeFlowRelativeFourSides((
			ascension::presentation::_blockStart = bs, ascension::presentation::_inlineStart = is,
			ascension::presentation::_blockEnd = be, ascension::presentation::_inlineEnd = ie)));

		BOOST_CHECK_EQUAL(ascension::presentation::blockFlowRange(box), ascension::nrange(bs, be));
		BOOST_CHECK_EQUAL(ascension::presentation::inlineFlowRange(box), ascension::nrange(is, ie));

		BOOST_TEST(ascension::presentation::extent(box) == be - bs);
		BOOST_TEST(ascension::presentation::measure(box) == ie - is);
	}
BOOST_AUTO_TEST_SUITE_END()
