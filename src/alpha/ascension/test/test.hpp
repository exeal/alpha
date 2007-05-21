// test.hpp

#ifndef ASCENSION_TEST_HPP
#define ASCENSION_TEST_HPP
#include <boost/test/unit_test.hpp>

boost::unit_test::test_suite* init_unit_test_suite(int, char*[]);

template<class Container>
class ContainerInitializeContext {
public:
	explicit ContainerInitializeContext(Container& c) throw() : c_(c) {}
	ContainerInitializeContext& operator,(typename Container::value_type e) {c_.push_back(e); return *this;}
private:
	Container& c_;
};

template<class Container>
class ContainerInitializer {
public:
	explicit ContainerInitializer(Container& c) throw() : c_(c) {}
	ContainerInitializeContext<Container> operator=(typename Container::value_type e) {
		c_.clear(); c_.push_back(e); return ContainerInitializeContext<Container>(c_);}
private:
	Container& c_;
};

template<class Container>
inline ContainerInitializer<Container> initializeContainer(Container& c) {
	return ContainerInitializer<Container>(c);
}

void testUnicodeIterator();
void testCaseFolder();
void testNormalizer();
void testGraphemeBreakIterator();
void testWordBreakIterator();
void testSentenceBreakIterator();
//void testTextBuffer();

inline boost::unit_test::test_suite* init_unit_test_suite(int, char*[]) {
	boost::unit_test::test_suite* p = BOOST_TEST_SUITE("ascension test suite.");
	p->add(BOOST_TEST_CASE(&testUnicodeIterator));
	p->add(BOOST_TEST_CASE(&testCaseFolder));
	p->add(BOOST_TEST_CASE(&testNormalizer));
	p->add(BOOST_TEST_CASE(&testGraphemeBreakIterator));
	p->add(BOOST_TEST_CASE(&testWordBreakIterator));
	p->add(BOOST_TEST_CASE(&testSentenceBreakIterator));
//	p->add(BOOST_TEST_CASE(&testTextBuffer));
	return p;
}

#endif /* !ASCENSION_TEST_HPP */
