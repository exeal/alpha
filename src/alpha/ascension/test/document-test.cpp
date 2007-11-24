// document-test.cpp

#include "../document.hpp"
#include <boost/test/included/test_exec_monitor.hpp>
using namespace ascension::kernel;

void testDocument() {
	Document d;

	// initial state
	BOOST_CHECK_EQUAL(d.length(), 0);
	BOOST_CHECK_EQUAL(d.accessibleRegion(), Region());
	BOOST_CHECK_EQUAL(d.region(), Region());
	BOOST_CHECK_EQUAL(d.begin(), d.end());
	BOOST_CHECK(!d.isChanging());
	BOOST_CHECK(!d.isModified());
	BOOST_CHECK(!d.isNarrowed());
	BOOST_CHECK(!d.isReadOnly());
	BOOST_CHECK(!d.isSequentialEdit());
	BOOST_CHECK(d.line(0).empty());
	BOOST_CHECK_EQUAL(d.lineLength(0), 0);
	BOOST_CHECK_EQUAL(d.lineOffset(0), 0);
	BOOST_CHECK_EQUAL(d.numberOfLines(), 1);
	BOOST_CHECK_EQUAL(d.session(), 0);
}

void testStreams() {
}

int test_main(int, char*[]) {
	testDocument();
	testStreams();
	return 0;
}
