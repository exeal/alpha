#define BOOST_TEST_MODULE token_rules_test
#include <boost/test/included/unit_test.hpp>

#include <ascension/direction.hpp>

namespace ascension {
	inline std::ostream& operator<<(std::ostream& out, const ascension::Direction& v) {
		return out << ((v == ascension::Direction::forward()) ? "<forward>" : "<backward>");
	}
}

BOOST_AUTO_TEST_CASE(equality_test) {
	BOOST_TEST(ascension::Direction::forward() == ascension::Direction::forward());
	BOOST_TEST(ascension::Direction::forward() != ascension::Direction::backward());
	BOOST_TEST(ascension::Direction::backward() != ascension::Direction::forward());
	BOOST_TEST(ascension::Direction::backward() == ascension::Direction::backward());
}

BOOST_AUTO_TEST_CASE(copy_construction_test) {
	const auto f(ascension::Direction::forward());
	BOOST_TEST(f == ascension::Direction::forward());
	const auto b(ascension::Direction::backward());
	BOOST_TEST(b == ascension::Direction::backward());
}

BOOST_AUTO_TEST_CASE(copy_assignment_test) {
	auto v(ascension::Direction::forward());
	v = ascension::Direction::backward();
	BOOST_TEST(v == ascension::Direction::backward());
	v = ascension::Direction::forward();
	BOOST_TEST(v == ascension::Direction::forward());
}

BOOST_AUTO_TEST_CASE(negation_test) {
	auto v(ascension::Direction::forward());
	BOOST_TEST(!v == ascension::Direction::backward());
	BOOST_TEST(!!v == ascension::Direction::forward());
}
