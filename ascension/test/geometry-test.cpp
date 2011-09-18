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

template<typename PointCoordinate, typename SizeCoordinate>
inline gfx::NativeRectangle rectangle(PointCoordinate x, PointCoordinate y, SizeCoordinate dx, SizeCoordinate dy) {
	return g::make<gfx::NativeRectangle>(g::make<gfx::NativePoint>(x, y), g::make<gfx::NativeSize>(dx, dy));
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
		gfx::NativeRectangle r(rectangle(0, 0, Scalar(1), Scalar(2)));
		BOOST_CHECK_CLOSE(static_cast<Scalar>(g::dx(r)), 1.0, 0.0001);
		BOOST_CHECK_CLOSE(static_cast<Scalar>(g::dy(r)), 2.0, 0.0001);
	}
}

void testEquals() {
	typedef g::Coordinate<gfx::NativePoint>::Type PointCoordinate;
	typedef g::Coordinate<gfx::NativeSize>::Type SizeCoordinate;

	const gfx::NativePoint p(g::make<gfx::NativePoint>(PointCoordinate(1), PointCoordinate(1)));
	BOOST_CHECK(g::equals(p,
		g::make<gfx::NativePoint>(PointCoordinate(1), PointCoordinate(1))));
	BOOST_CHECK(!g::equals(p,
		g::make<gfx::NativePoint>(PointCoordinate(1), PointCoordinate(2))));

	const gfx::NativeSize s(g::make<gfx::NativeSize>(SizeCoordinate(1), SizeCoordinate(1)));
	BOOST_CHECK(g::equals(s,
		g::make<gfx::NativeSize>(SizeCoordinate(1), SizeCoordinate(1))));
	BOOST_CHECK(!g::equals(s,
		g::make<gfx::NativeSize>(SizeCoordinate(1), SizeCoordinate(2))));
	BOOST_CHECK(!g::equals(s,
		g::make<gfx::NativeSize>(SizeCoordinate(-1), SizeCoordinate(-1))));

	const gfx::NativeRectangle r(rectangle(
		PointCoordinate(1), PointCoordinate(1), SizeCoordinate(1), SizeCoordinate(1)));
	BOOST_CHECK(g::equals(r, rectangle(
		PointCoordinate(1), PointCoordinate(1), SizeCoordinate(1), SizeCoordinate(1))));
	BOOST_CHECK(!g::equals(r, rectangle(
		PointCoordinate(1), PointCoordinate(1), SizeCoordinate(1), SizeCoordinate(2))));
	BOOST_CHECK(!g::equals(r, rectangle(
		PointCoordinate(1), PointCoordinate(2), SizeCoordinate(1), SizeCoordinate(1))));
	BOOST_CHECK(!g::equals(r, rectangle(
		PointCoordinate(2), PointCoordinate(2), SizeCoordinate(-1), SizeCoordinate(-1))));
}

template<typename Geometry1, typename Geometry2>
void testIfEqual(const Geometry1& geometry1, const Geometry2& geometry2, bool expected) {
	if(expected)
		BOOST_CHECK(g::equals(geometry1, geometry2));
	else
		BOOST_CHECK(!g::equals(geometry1, geometry2));
}

void testRange() {
	typedef g::Coordinate<g::Coordinate<gfx::NativeRectangle>::Type>::Type Scalar;

	gfx::NativeRectangle r(rectangle(Scalar(0), Scalar(0), Scalar(0), Scalar(0)));
	BOOST_CHECK_CLOSE(g::range<g::X_COORDINATE>(r).beginning(), 0.0, 0.0001);
	BOOST_CHECK_CLOSE(g::range<g::X_COORDINATE>(r).end(), 0.0, 0.0001);
	BOOST_CHECK_CLOSE(g::range<g::Y_COORDINATE>(r).beginning(), 0.0, 0.0001);
	BOOST_CHECK_CLOSE(g::range<g::Y_COORDINATE>(r).end(), 0.0, 0.0001);

	g::range<g::X_COORDINATE>(r) = a::makeRange(Scalar(1), Scalar(3));
	BOOST_CHECK_CLOSE(g::x(g::get<0>(r)), 1.0, 0.0001);
	BOOST_CHECK_CLOSE(g::x(g::get<1>(r)), 3.0, 0.0001);

	g::range<g::Y_COORDINATE>(r) = a::makeRange(Scalar(-2), Scalar(4));
	BOOST_CHECK_CLOSE(g::y(g::get<0>(r)), -2.0, 0.0001);
	BOOST_CHECK_CLOSE(g::y(g::get<1>(r)), 4.0, 0.0001);

	g::range<g::X_COORDINATE>(r) = g::range<g::Y_COORDINATE>(r) = a::makeRange(Scalar(5), Scalar(10));
	BOOST_CHECK_CLOSE(g::range<g::X_COORDINATE>(r).beginning(), 5.0, 0.0001);
	BOOST_CHECK_CLOSE(g::range<g::X_COORDINATE>(r).end(), 10.0, 0.0001);
	BOOST_CHECK_CLOSE(g::range<g::Y_COORDINATE>(r).beginning(), 5.0, 0.0001);
	BOOST_CHECK_CLOSE(g::range<g::Y_COORDINATE>(r).end(), 10.0, 0.0001);
}

void testAlgorithms() {
	gfx::NativePoint p(g::make<gfx::NativePoint>(1, 1));
	gfx::NativeSize s(g::make<gfx::NativeSize>(1, 1));

	// add
	testIfEqual(g::add(p, g::make<gfx::NativePoint>(2, 3)), g::make<gfx::NativePoint>(3, 4), true);
	testIfEqual(g::add(s, g::make<gfx::NativeSize>(2, 3)), g::make<gfx::NativeSize>(3, 4), true);

	// bottom
	BOOST_CHECK_CLOSE(g::bottom(rectangle(1, 2, 3, 4)), 6.0, 0.0001);
	BOOST_CHECK_CLOSE(g::bottom(rectangle(1, 2, -3, -4)), 2.0, 0.0001);

	// bottomLeft
	BOOST_CHECK(g::equals(g::bottomLeft(rectangle(1, 2, 3, 4)), g::make<gfx::NativePoint>(1, 6)));
	BOOST_CHECK(g::equals(g::bottomLeft(rectangle(1, 2, -3, -4)), g::make<gfx::NativePoint>(-2, 2)));

	// bottomRight
	BOOST_CHECK(g::equals(g::bottomRight(rectangle(1, 2, 3, 4)), g::make<gfx::NativePoint>(4, 6)));
	BOOST_CHECK(g::equals(g::bottomRight(rectangle(1, 2, -3, -4)), g::make<gfx::NativePoint>(1, 2)));

	// divide

	// expandTo

	// includes
	BOOST_CHECK(!g::includes(rectangle(0, 0, 0, 0), g::make<gfx::NativePoint>(0, 0)));
	BOOST_CHECK(g::includes(rectangle(1, 2, 3, 4), g::make<gfx::NativePoint>(2, 3)));
	BOOST_CHECK(!g::includes(rectangle(1, 2, 3, 4), g::make<gfx::NativePoint>(4, 3)));
	BOOST_CHECK(!g::includes(rectangle(1, 2, 3, 4), g::make<gfx::NativePoint>(2, 6)));
	BOOST_CHECK(g::includes(rectangle(1, 2, -3, -4), g::make<gfx::NativePoint>(0, 0)));
	BOOST_CHECK(g::includes(rectangle(0, 0, 0, 0), rectangle(0, 0, 0, 0)));
	BOOST_CHECK(g::includes(rectangle(1, 2, 3, 4), rectangle(1, 2, 3, 4)));
	BOOST_CHECK(g::includes(rectangle(1, 2, 3, 4), rectangle(1, 2, 1, 2)));
	BOOST_CHECK(g::includes(rectangle(1, 2, 3, 4), rectangle(3, 4, 1, 2)));
	BOOST_CHECK(g::includes(rectangle(1, 2, 3, 4), rectangle(3, 3, 1, 1)));
	BOOST_CHECK(!g::includes(rectangle(1, 2, 3, 4), rectangle(3, 3, 10, 1)));
	BOOST_CHECK(!g::includes(rectangle(1, 2, 3, 4), rectangle(3, 3, 1, 10)));
	BOOST_CHECK(g::includes(rectangle(-1, -2, 3, 4), rectangle(-1, -2, 1, 1)));

	// intersected
	gfx::NativeRectangle r(rectangle(0, 0, 3, 4));
	g::intersected(r, rectangle(0, 0, 0, 0));

	// intersects

	// isEmpty
	BOOST_CHECK(g::isEmpty(g::make<gfx::NativeSize>(0, 0)));
	BOOST_CHECK(g::isEmpty(g::make<gfx::NativeSize>(1, 0)));
	BOOST_CHECK(g::isEmpty(g::make<gfx::NativeSize>(0, 1)));
	BOOST_CHECK(!g::isEmpty(g::make<gfx::NativeSize>(1, 1)));
	BOOST_CHECK(g::isEmpty(g::make<gfx::NativeSize>(-1, 0)));
	BOOST_CHECK(g::isEmpty(g::make<gfx::NativeSize>(0, -1)));
	BOOST_CHECK(g::isEmpty(g::make<gfx::NativeSize>(-1, -1)));
	BOOST_CHECK(g::isEmpty(rectangle(0, 0, 0, 0)));
	BOOST_CHECK(g::isEmpty(rectangle(0, 0, 1, 0)));
	BOOST_CHECK(g::isEmpty(rectangle(0, 0, 0, 1)));
	BOOST_CHECK(!g::isEmpty(rectangle(0, 0, 1, 1)));
	BOOST_CHECK(g::isEmpty(rectangle(0, 0, -1, 0)));
	BOOST_CHECK(g::isEmpty(rectangle(0, 0, 0, -1)));
	BOOST_CHECK(g::isEmpty(rectangle(0, 0, -1, -1)));

	// isNormalized
	BOOST_CHECK(g::isNormalized(g::make<gfx::NativeSize>(0, 0)));
	BOOST_CHECK(g::isNormalized(g::make<gfx::NativeSize>(1, 2)));
	BOOST_CHECK(!g::isNormalized(g::make<gfx::NativeSize>(-1, 2)));
	BOOST_CHECK(!g::isNormalized(g::make<gfx::NativeSize>(1, -2)));
	BOOST_CHECK(!g::isNormalized(g::make<gfx::NativeSize>(-1, -2)));
	BOOST_CHECK(g::isNormalized(rectangle(0, 0, 0, 0)));
	BOOST_CHECK(g::isNormalized(rectangle(1, 2, 3, 4)));
	BOOST_CHECK(g::isNormalized(rectangle(-1, -2, 3, 4)));
	BOOST_CHECK(g::isNormalized(rectangle(-1, -2, 3, 4)));
	BOOST_CHECK(!g::isNormalized(rectangle(1, 2, -3, 4)));
	BOOST_CHECK(!g::isNormalized(rectangle(1, 2, 3, -4)));
	BOOST_CHECK(!g::isNormalized(rectangle(1, 2, -3, -4)));

	// left
	BOOST_CHECK_CLOSE(g::left(rectangle(1, 2, 3, 4)), 1.0, 0.0001);
	BOOST_CHECK_CLOSE(g::left(rectangle(1, 2, -3, -4)), -2.0, 0.0001);

	// makeBoundedTo

	// manhattanLength
	BOOST_CHECK_CLOSE(g::manhattanLength(g::make<gfx::NativePoint>(0, 0)), 0.0, 0.0001);
	BOOST_CHECK_CLOSE(g::manhattanLength(g::make<gfx::NativePoint>(1, 2)), 3.0, 0.0001);
	BOOST_CHECK_CLOSE(g::manhattanLength(g::make<gfx::NativePoint>(-3, -4)), 7.0, 0.0001);

	// multiply

	// negate
	p = g::make<gfx::NativePoint>(1, 2);
	BOOST_CHECK(g::equals(g::negate(p), g::make<gfx::NativePoint>(-1, -2)));
	BOOST_CHECK(g::equals(g::negate(p), g::make<gfx::NativePoint>(1, 2)));
	s = g::make<gfx::NativeSize>(1, 2);
	BOOST_CHECK(g::equals(g::negate(s), g::make<gfx::NativeSize>(-1, -2)));
	BOOST_CHECK(g::equals(g::negate(s), g::make<gfx::NativeSize>(1, 2)));

	// normalize
	s = g::make<gfx::NativeSize>(0, 0);
	BOOST_CHECK(g::equals(g::normalize(s), g::make<gfx::NativeSize>(0, 0)));
	s = g::make<gfx::NativeSize>(1, 2);
	BOOST_CHECK(g::equals(g::normalize(s), g::make<gfx::NativeSize>(1, 2)));
	s = g::make<gfx::NativeSize>(-1, -2);
	BOOST_CHECK(g::equals(g::normalize(s), g::make<gfx::NativeSize>(1, 2)));
	r = rectangle(0, 0, 0, 0);
	BOOST_CHECK(g::equals(g::normalize(r), rectangle(0, 0, 0, 0)));
	r = rectangle(1, 2, 3, 4);
	BOOST_CHECK(g::equals(g::normalize(r), rectangle(1, 2, 3, 4)));
	r = rectangle(-1, -2, -3, -4);
	BOOST_CHECK(g::equals(g::normalize(r), rectangle(-4, -6, 3, 4)));

	// origin
	BOOST_CHECK(g::equals(g::origin(rectangle(1, 2, 3, 4)), g::make<gfx::NativePoint>(1, 2)));
	BOOST_CHECK(g::equals(g::origin(rectangle(-1, -2, 0, 0)), g::make<gfx::NativePoint>(-1, -2)));

	// resize
	r = rectangle(1, 2, 3, 4);
	BOOST_CHECK(g::equals(g::resize(r, g::make<gfx::NativeSize>(5, 6)), rectangle(1, 2, 5, 6)));
	r = rectangle(-1, -2, -3, -4);
	BOOST_CHECK(g::equals(g::resize(r, g::make<gfx::NativeSize>(-5, -6)), rectangle(-1, -2, -5, -6)));

	// right
	BOOST_CHECK_CLOSE(g::right(rectangle(1, 2, 3, 4)), 4.0, 0.0001);
	BOOST_CHECK_CLOSE(g::right(rectangle(1, 2, -3, -4)), 1.0, 0.0001);

	// size
	BOOST_CHECK(g::equals(g::size(rectangle(0, 0, 1, 2)), g::make<gfx::NativeSize>(1, 2)));
	BOOST_CHECK(g::equals(g::size(rectangle(-1, -2, -3, -4)), g::make<gfx::NativeSize>(-3, -4)));

	// scale

	// subtract
	p = g::make<gfx::NativePoint>(1, 2);
	BOOST_CHECK(g::equals(g::subtract(p, g::make<gfx::NativePoint>(3, 4)), g::make<gfx::NativePoint>(-2, -2)));
	p = g::make<gfx::NativePoint>(-1, -2);
	BOOST_CHECK(g::equals(g::subtract(p, g::make<gfx::NativePoint>(-3, -4)), g::make<gfx::NativePoint>(2, 2)));
	s = g::make<gfx::NativeSize>(1, 2);
	BOOST_CHECK(g::equals(g::subtract(s, g::make<gfx::NativeSize>(3, 4)), g::make<gfx::NativeSize>(-2, -2)));
	s = g::make<gfx::NativeSize>(-1, -2);
	BOOST_CHECK(g::equals(g::subtract(s, g::make<gfx::NativeSize>(-3, -4)), g::make<gfx::NativeSize>(2, 2)));

	// top
	BOOST_CHECK_CLOSE(g::top(rectangle(1, 2, 3, 4)), 2.0, 0.0001);
	BOOST_CHECK_CLOSE(g::top(rectangle(1, 2, -3, -4)), -2.0, 0.0001);

	// topLeft
	BOOST_CHECK(g::equals(g::topLeft(rectangle(1, 2, 3, 4)), g::make<gfx::NativePoint>(1, 2)));
	BOOST_CHECK(g::equals(g::topLeft(rectangle(1, 2, -3, -4)), g::make<gfx::NativePoint>(-2, -2)));

	// topRight
	BOOST_CHECK(g::equals(g::topRight(rectangle(1, 2, 3, 4)), g::make<gfx::NativePoint>(4, 2)));
	BOOST_CHECK(g::equals(g::topRight(rectangle(1, 2, -3, -4)), g::make<gfx::NativePoint>(1, -2)));

	// translate
	p = g::make<gfx::NativePoint>(1, 2);
	BOOST_CHECK(g::equals(g::translate(p, g::make<gfx::NativeSize>(3, 4)), g::make<gfx::NativePoint>(4, 6)));
	p = g::make<gfx::NativePoint>(1, 2);
	BOOST_CHECK(g::equals(g::translate(p, g::make<gfx::NativeSize>(-3, -4)), g::make<gfx::NativePoint>(-2, -2)));
	r = rectangle(1, 2, 3, 4);
	BOOST_CHECK(g::equals(g::translate(r, g::make<gfx::NativeSize>(5, 6)), rectangle(6, 8, 3, 4)));
	r = rectangle(-1, -2, -3, -4);
	BOOST_CHECK(g::equals(g::translate(r, g::make<gfx::NativeSize>(-5, -6)), rectangle(-6, -8, -3, -4)));

	// transpose
	s = g::make<gfx::NativeSize>(1, 2);
	BOOST_CHECK(g::equals(g::transpose(s), g::make<gfx::NativeSize>(2, 1)));

	// united
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
	testEquals();
	testRange();
	testAlgorithms();

	return 0;
}
