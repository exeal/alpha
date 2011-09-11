// geometry-test.cpp

#include <ascension/graphics/geometry.hpp>
#include <boost/test/included/test_exec_monitor.hpp>
#include <string>
#include <typeinfo>
namespace a = ascension;
namespace gfx = ascension::graphics;
namespace g = ascension::graphics::geometry;


// from boost/lib/geometry/test/core/access.cpp
template<typename Geometry>
void testGetSet() {
	typedef typename g::Coordinate<Geometry>::Type CoordinateType;

	Geometry geometry;
	g::set<0>(geometry, CoordinateType(1));
	g::set<1>(geometry, CoordinateType(2));

	CoordinateType x(g::get<g::X_COORDINATE>(geometry));
	CoordinateType y(g::get<g::Y_COORDINATE>(geometry));

	BOOST_CHECK_CLOSE(double(x), 1.0, 0.0001);
	BOOST_CHECK_CLOSE(double(y), 2.0, 0.0001);
}

// from boost/lib/geometry/test/core/tag.cpp
template<typename Geometry, typename Expected>
void testTag() {
	// see "FIX: Using Run-Time Type Info May Cause Memory Leak Report"
	// (http://support.microsoft.com/kb/140670/en-gb)
	if(typeid(g::Tag<Geometry>::Type) != typeid(Expected))
		BOOST_FAIL(typeid(g::Tag<Geometry>::Type).name() + std::string(" != ") + typeid(Expected).name());
}

template<typename Geometry>
void testConstruction() {
	typedef typename g::Coordinate<Geometry>::Type CoordinateType;

	const Geometry geometry(g::make<Geometry>(CoordinateType(1), CoordinateType(2)));
	CoordinateType x(g::get<g::X_COORDINATE>(geometry));
	CoordinateType y(g::get<g::Y_COORDINATE>(geometry));

	BOOST_CHECK_CLOSE(double(x), 1.0, 0.0001);
	BOOST_CHECK_CLOSE(double(y), 2.0, 0.0001);
}

void testRectangleConstruction() {
	typedef g::Coordinate<gfx::NativeRectangle>::Type Point;
	typedef g::Coordinate<Point>::Type Scalar;

	gfx::NativeRectangle r = g::make<gfx::NativeRectangle>(
		a::makeRange(Scalar(1), Scalar(2)),
		a::makeRange(Scalar(3), Scalar(4)));

	Point p1 = g::get<0>(r);
	Point p2 = g::get<1>(r);

	BOOST_CHECK_CLOSE(g::get<0>(p1), 1.0, 0.0001);
	BOOST_CHECK_CLOSE(g::get<1>(p1), 3.0, 0.0001);
	BOOST_CHECK_CLOSE(g::get<0>(p2), 2.0, 0.0001);
	BOOST_CHECK_CLOSE(g::get<1>(p2), 4.0, 0.0001);
}

void testXY() {
	typedef g::Coordinate<gfx::NativePoint>::Type Scalar;

	gfx::NativePoint p(g::make<gfx::NativePoint>(Scalar(1), Scalar(2)));
	BOOST_CHECK_CLOSE(static_cast<Scalar>(g::x(p)), 1.0, 0.0001);
	BOOST_CHECK_CLOSE(static_cast<Scalar>(g::y(p)), 2.0, 0.0001);

	g::x(p) = Scalar(3);
	g::y(p) = Scalar(4);
	BOOST_CHECK_CLOSE(static_cast<Scalar>(g::x(p)), 3.0, 0.0001);
	BOOST_CHECK_CLOSE(static_cast<Scalar>(g::y(p)), 4.0, 0.0001);

	g::x(p) = g::y(p) = 5;
	BOOST_CHECK_CLOSE(static_cast<Scalar>(g::x(p)), 5.0, 0.0001);
	BOOST_CHECK_CLOSE(static_cast<Scalar>(g::y(p)), 5.0, 0.0001);
	g::y(p) = g::x(p) = 6;
	BOOST_CHECK_CLOSE(static_cast<Scalar>(g::x(p)), 6.0, 0.0001);
	BOOST_CHECK_CLOSE(static_cast<Scalar>(g::y(p)), 6.0, 0.0001);

	BOOST_CHECK_CLOSE(static_cast<Scalar>(+g::x(p)), 6.0, 0.0001);
	BOOST_CHECK_CLOSE(static_cast<Scalar>(-g::x(p)), -6.0, 0.0001);
	
	g::x(p) += Scalar(1);
	BOOST_CHECK_CLOSE(static_cast<Scalar>(g::x(p)), 7.0, 0.0001);
	g::x(p) -= Scalar(2);
	BOOST_CHECK_CLOSE(static_cast<Scalar>(g::x(p)), 5.0, 0.0001);
	g::x(p) *= Scalar(3);
	BOOST_CHECK_CLOSE(static_cast<Scalar>(g::x(p)), 15.0, 0.0001);
	g::x(p) /= Scalar(5);
	BOOST_CHECK_CLOSE(static_cast<Scalar>(g::x(p)), 3.0, 0.0001);
}

void testDxDy() {
	typedef g::Coordinate<gfx::NativeSize>::Type Scalar;

	// size
	{

		gfx::NativeSize s(g::make<gfx::NativeSize>(Scalar(1), Scalar(2)));
		BOOST_CHECK_CLOSE(static_cast<Scalar>(g::dx(s)), 1.0, 0.0001);
		BOOST_CHECK_CLOSE(static_cast<Scalar>(g::dy(s)), 2.0, 0.0001);

		g::dx(s) = Scalar(3);
		g::dy(s) = Scalar(4);
		BOOST_CHECK_CLOSE(static_cast<Scalar>(g::dx(s)), 3.0, 0.0001);
		BOOST_CHECK_CLOSE(static_cast<Scalar>(g::dy(s)), 4.0, 0.0001);

		g::dx(s) = g::dy(s) = 5;
		BOOST_CHECK_CLOSE(static_cast<Scalar>(g::dx(s)), 5.0, 0.0001);
		BOOST_CHECK_CLOSE(static_cast<Scalar>(g::dy(s)), 5.0, 0.0001);
		g::dy(s) = g::dx(s) = 6;
		BOOST_CHECK_CLOSE(static_cast<Scalar>(g::dx(s)), 6.0, 0.0001);
		BOOST_CHECK_CLOSE(static_cast<Scalar>(g::dy(s)), 6.0, 0.0001);

		BOOST_CHECK_CLOSE(static_cast<Scalar>(+g::dx(s)), 6.0, 0.0001);
		BOOST_CHECK_CLOSE(static_cast<Scalar>(-g::dx(s)), -6.0, 0.0001);
	
		g::dx(s) += Scalar(1);
		BOOST_CHECK_CLOSE(static_cast<Scalar>(g::dx(s)), 7.0, 0.0001);
		g::dx(s) -= Scalar(2);
		BOOST_CHECK_CLOSE(static_cast<Scalar>(g::dx(s)), 5.0, 0.0001);
		g::dx(s) *= Scalar(3);
		BOOST_CHECK_CLOSE(static_cast<Scalar>(g::dx(s)), 15.0, 0.0001);
		g::dx(s) /= Scalar(5);
		BOOST_CHECK_CLOSE(static_cast<Scalar>(g::dx(s)), 3.0, 0.0001);
	}

	// rectangle
	{
		gfx::NativeRectangle r(g::make<gfx::NativeRectangle>(
			g::make<gfx::NativePoint>(0, 0), g::make<gfx::NativeSize>(Scalar(1), Scalar(2))));
		BOOST_CHECK_CLOSE(static_cast<Scalar>(g::dx(r)), 1.0, 0.0001);
		BOOST_CHECK_CLOSE(static_cast<Scalar>(g::dy(r)), 2.0, 0.0001);
	}
}

void testAlgorithms() {
}

int test_main(int, char*[]) {
	testGetSet<gfx::NativePoint>();
	testGetSet<gfx::NativeSize>();
	testTag<gfx::NativePoint, g::PointTag>();
	testTag<gfx::NativeSize, g::SizeTag>();
	testTag<gfx::NativeRectangle, g::RectangleTag>();
//	testTag<gfx::NativeRegion, g::RegionTag>();
	testConstruction<gfx::NativePoint>();
	testConstruction<gfx::NativeSize>();
	testRectangleConstruction();
	testXY();
	testDxDy();
	testAlgorithms();

	return 0;
}
