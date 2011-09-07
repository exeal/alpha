// range-test.cpp

#include <ascension/corelib/range.hpp>
#include <boost/test/included/test_exec_monitor.hpp>
#include <string>
namespace a = ascension;


void testConstructions() {
	a::Range<int> ir(23, 42);
	BOOST_CHECK_EQUAL(ir.beginning(), 23);
	BOOST_CHECK_EQUAL(ir.end(), 42);
	BOOST_CHECK_EQUAL(ir, a::Range<int>(std::make_pair(42, 23)));

	a::Range<std::string> sr(a::makeRange(std::string("alice"), std::string("bob")));
	BOOST_CHECK_EQUAL(sr.beginning(), std::string("alice"));
	BOOST_CHECK_EQUAL(sr.end(), std::string("bob"));

	const std::string s("xyzzy");
	a::Range<std::string::const_iterator> sir;
	sir = a::makeRange(s.end(), s.begin());
	BOOST_CHECK_EQUAL(*sir.beginning(), 'x');
	BOOST_CHECK_EQUAL(sir.end()[-1], 'y');
}

void testAlgorithms() {
	// includes(value)
	a::Range<int> r(23, 42);
	const a::Range<int> zero(0, 0);
	BOOST_CHECK(!a::includes(r, 22));
	BOOST_CHECK(a::includes(r, 23));
	BOOST_CHECK(a::includes(r, 24));
	BOOST_CHECK(!a::includes(r, 42));
	BOOST_CHECK(!a::includes(r, 43));
	BOOST_CHECK(!a::includes(zero, 0));

	// includes(range)
	BOOST_CHECK(!a::includes(r, a::makeRange(0, 0)));
	BOOST_CHECK(!a::includes(r, a::makeRange(22, 23)));
	BOOST_CHECK(a::includes(r, a::makeRange(23, 23)));
	BOOST_CHECK(!a::includes(r, a::makeRange(22, 24)));
	BOOST_CHECK(a::includes(r, a::makeRange(23, 24)));
	BOOST_CHECK(!a::includes(r, a::makeRange(0, 100)));
	BOOST_CHECK(a::includes(r, a::makeRange(23, 42)));
	BOOST_CHECK(a::includes(r, a::makeRange(30, 42)));
	BOOST_CHECK(a::includes(r, a::makeRange(42, 42)));
	BOOST_CHECK(!a::includes(r, a::makeRange(42, 50)));
	BOOST_CHECK(!a::includes(r, a::makeRange(30, 50)));
	BOOST_CHECK(!a::includes(r, a::makeRange(50, 100)));
	BOOST_CHECK(!a::includes(r, a::makeRange(50, 50)));
	BOOST_CHECK(!a::includes(zero, a::makeRange(-1, -1)));
	BOOST_CHECK(a::includes(zero, a::makeRange(0, 0)));
	BOOST_CHECK(!a::includes(zero, a::makeRange(1, 1)));

	// intersected

	// intersects

	// isEmpty
	BOOST_CHECK(a::isEmpty(a::makeRange(0, 0)));
	BOOST_CHECK(!a::isEmpty(a::makeRange(23, 42)));

	// length
	BOOST_CHECK_EQUAL(a::length(a::makeRange(0, 0)), 0);
	BOOST_CHECK_EQUAL(a::length(a::makeRange(23, 42)), 19);
	BOOST_CHECK_EQUAL(a::length(a::makeRange(42, 23)), 19);

	// merged
	BOOST_CHECK_EQUAL(a::merged(a::makeRange(0, 10), a::makeRange(20, 30)), a::makeRange(0, 30));
	BOOST_CHECK_EQUAL(a::merged(a::makeRange(23, 42), a::makeRange(30, 40)), a::makeRange(23, 42));
	BOOST_CHECK_EQUAL(a::merged(a::makeRange(23, 42), a::makeRange(20, 40)), a::makeRange(20, 42));
	BOOST_CHECK_EQUAL(a::merged(a::makeRange(23, 42), a::makeRange(30, 50)), a::makeRange(23, 50));
	BOOST_CHECK_EQUAL(a::merged(a::makeRange(23, 42), a::makeRange(30, 30)), a::makeRange(23, 42));
	BOOST_CHECK_EQUAL(a::merged(a::makeRange(23, 30), a::makeRange(30, 42)), a::makeRange(23, 42));
	BOOST_CHECK_EQUAL(a::merged(a::makeRange(0, 0), a::makeRange(23, 42)), a::makeRange(23, 42));
	BOOST_CHECK_EQUAL(a::merged(a::makeRange(23, 42), a::makeRange(50, 50)), a::makeRange(23, 42));
	BOOST_CHECK(a::isEmpty(a::merged(a::makeRange(0, 0), a::makeRange(50, 50))));
}

int test_main(int, char*[]) {
	testConstructions();
	testAlgorithms();

	return 0;
}
