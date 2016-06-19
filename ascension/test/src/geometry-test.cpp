#define BOOST_TEST_MODULE geometry_test
#include <boost/test/included/unit_test.hpp>

#include <ascension/graphics/geometry/dimension.hpp>
#include <ascension/graphics/geometry/point.hpp>
#include <ascension/graphics/geometry/point-xy.hpp>
#include <ascension/graphics/geometry/algorithms/make.hpp>
#include <boost/geometry/algorithms/area.hpp>

BOOST_AUTO_TEST_SUITE(geometry)
	BOOST_AUTO_TEST_CASE(point_test) {
		auto p(ascension::graphics::geometry::make<ascension::graphics::Point>((
			ascension::graphics::geometry::_x = 23, ascension::graphics::geometry::_y = 42)));

		BOOST_TEST(boost::geometry::get<0>(p) == 23);
		BOOST_TEST(boost::geometry::get<1>(p) == 42);

		BOOST_TEST(ascension::graphics::geometry::x(p) == 23);
		BOOST_TEST(ascension::graphics::geometry::y(p) == 42);

		ascension::graphics::geometry::x(p) *= 10;
		ascension::graphics::geometry::y(p) *= 10;
		BOOST_TEST(ascension::graphics::geometry::x(p) == 230);
		BOOST_TEST(ascension::graphics::geometry::y(p) == 420);
	}

	BOOST_AUTO_TEST_SUITE(dimension)
		BOOST_AUTO_TEST_CASE(construction_test) {
			// constructor with initial values
			const ascension::graphics::geometry::BasicDimension<int> d(ascension::graphics::geometry::_dx = 23, ascension::graphics::geometry::_dy = 42);
			BOOST_TEST(boost::geometry::get<0>(d) == 23);
			BOOST_TEST(boost::geometry::get<1>(d) == 42);

			// copy-constructor
			const ascension::graphics::Dimension d2(d);
			BOOST_TEST(boost::geometry::get<0>(d2) == 23);
			BOOST_TEST(boost::geometry::get<1>(d2) == 42);

			// assignment operator
			ascension::graphics::Dimension d3;
			d3 = d;
			BOOST_TEST(boost::geometry::get<0>(d3) == 23);
			BOOST_TEST(boost::geometry::get<1>(d3) == 42);

			// make
//			const auto d4(ascension::graphics::geometry::make<ascension::graphics::Dimension>((
//				ascension::graphics::geometry::_dx = 23, ascension::graphics::geometry::_dy = 42)));
//			BOOST_TEST(boost::geometry::get<0>(d4) == 23);
//			BOOST_TEST(boost::geometry::get<1>(d4) == 42);
		}

		BOOST_AUTO_TEST_CASE(access_test) {
			ascension::graphics::geometry::BasicDimension<int> d(ascension::graphics::geometry::_dx = 23, ascension::graphics::geometry::_dy = 42);

			BOOST_TEST(ascension::graphics::geometry::dx(d) == 23);
			BOOST_TEST(ascension::graphics::geometry::dy(d) == 42);

			ascension::graphics::geometry::dx(d) *= 10;
			ascension::graphics::geometry::dy(d) *= 10;
			BOOST_TEST(ascension::graphics::geometry::dx(d) == 230);
			BOOST_TEST(ascension::graphics::geometry::dy(d) == 420);
		}

//		BOOST_AUTO_TEST_CASE(boost_algorithms_test) {
//			const ascension::graphics::geometry::BasicDimension<int> d(ascension::graphics::geometry::_dx = 23, ascension::graphics::geometry::_dy = 42);
//			BOOST_TEST(boost::geometry::area(d) == 23 * 42);
//
//			boost::geometry::assign_zero(d);
//			BOOST_TEST(ascension::graphics::geometry::dx(d) == 0);
//			BOOST_TEST(ascension::graphics::geometry::dy(d) == 0);
//		}
	BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
