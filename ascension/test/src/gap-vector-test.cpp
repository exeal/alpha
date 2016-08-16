#define BOOST_TEST_MODULE gap_vector_test
#include <boost/test/included/unit_test.hpp>

#include <ascension/corelib/detail/gap-vector.hpp>
#include <array>
#include <cstdint>
#include <iomanip>
#include <string>

namespace {
	std::array<int, 8> make_random_array() {
		const std::array<int, 8> a = {{12, 23, 34, 45, 56, 67, 78, 89}};
		return a;
	}
	ascension::detail::GapVector<int> make_random_gap_vector() {
		const auto a(make_random_array());
		return ascension::detail::GapVector<int>(a.cbegin(), a.cend());
	}
	std::string to_string(const ascension::detail::GapVector<char>& gv) {
		return std::string(gv.cbegin(), gv.cend());
	}
}

namespace ascension {
	namespace detail {
		template<typename T, typename P, typename R>
		inline std::ostream& operator<<(std::ostream& out, ascension::detail::GapVectorIterator<T, P, R> i) {
			return out << "iterator{@" << std::hex << static_cast<unsigned int>(reinterpret_cast<std::uintptr_t>(&*i)) << "}";
		}
		template<typename T, typename P, typename R>
		inline std::ostream& operator<<(std::ostream& out, std::reverse_iterator<ascension::detail::GapVectorIterator<T, P, R>> i) {
			return out << "iterator{@" << std::hex << static_cast<unsigned int>(reinterpret_cast<std::uintptr_t>(&*i)) << "}";
		}
		template<typename T, typename A>
		inline std::ostream& operator<<(std::ostream& out, const ascension::detail::GapVector<T, A>& gv) {
			out << "gap_vector[";
			std::copy(gv.cbegin(), gv.cend(), std::ostream_iterator<char>(out));
			return out << "]";
		}
	}
}

BOOST_AUTO_TEST_SUITE(construction)
	BOOST_AUTO_TEST_CASE(default_construction) {
		const ascension::detail::GapVector<int> gv;
		BOOST_TEST(gv.empty());
		BOOST_TEST(gv.size() == static_cast<std::size_t>(0));
		BOOST_TEST(gv.begin() == gv.end());
		BOOST_TEST(gv.cbegin() == gv.cend());
		BOOST_TEST(gv.rbegin() == gv.rend());
		BOOST_TEST(gv.crbegin() == gv.crend());
	}

	BOOST_AUTO_TEST_CASE(nv_construction) {
		const ascension::detail::GapVector<int> gv(static_cast<std::size_t>(3), 42);
		BOOST_TEST(!gv.empty());
		BOOST_TEST(gv.size() == static_cast<std::size_t>(3));
		BOOST_TEST(gv.at(0) == 42);
		BOOST_TEST(gv.at(1) == 42);
		BOOST_TEST(gv.at(2) == 42);
	}

	BOOST_AUTO_TEST_CASE(iterators_construction) {
		const std::array<int, 8> a(make_random_array());
		const ascension::detail::GapVector<int> gv(a.cbegin(), a.cend());
		BOOST_TEST(!gv.empty());
		BOOST_TEST(gv.size() == a.size());
		BOOST_CHECK_EQUAL_COLLECTIONS(gv.cbegin(), gv.cend(), a.cbegin(), a.cend());
	}

	BOOST_AUTO_TEST_CASE(copy_construction) {
		const auto gv(make_random_gap_vector());
		const auto gv2(gv);
		BOOST_TEST(!gv2.empty());
		BOOST_TEST(gv2.size() == gv.size());
		BOOST_CHECK_EQUAL_COLLECTIONS(gv2.cbegin(), gv2.cend(), gv.cbegin(), gv.cend());
	}

	BOOST_AUTO_TEST_CASE(move_construction) {
		const std::array<int, 8> a(make_random_array());
		ascension::detail::GapVector<int> gv(a.cbegin(), a.cend());
		const ascension::detail::GapVector<int> gv2(std::move(gv));
		BOOST_TEST(!gv2.empty());
		BOOST_TEST(gv2.size() == a.size());
		BOOST_CHECK_EQUAL_COLLECTIONS(gv2.cbegin(), gv2.cend(), a.cbegin(), a.cend());
	}
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(assignment)
	BOOST_AUTO_TEST_CASE(copy_assignment_operator_test) {
		ascension::detail::GapVector<int> gv(make_random_gap_vector()), gv2;
		auto& r = (gv2 = gv);
		BOOST_TEST(gv2.size() == gv.size());
		BOOST_CHECK_EQUAL_COLLECTIONS(gv2.cbegin(), gv2.cend(), gv.cbegin(), gv.cend());
		BOOST_TEST(&r == &gv2);
	}

	BOOST_AUTO_TEST_CASE(move_assignment_operator_test) {
		const auto a(make_random_array());
		ascension::detail::GapVector<int> gv(a.cbegin(), a.cend()), gv2;
		auto& r = (gv2 = std::move(gv));
		BOOST_TEST(gv2.size() == a.size());
		BOOST_CHECK_EQUAL_COLLECTIONS(gv2.cbegin(), gv2.cend(), a.cbegin(), a.cend());
		BOOST_TEST(&r == &gv2);
	}

	BOOST_AUTO_TEST_CASE(assign_test) {
		const auto a(make_random_array());
		ascension::detail::GapVector<int> gv;
		gv.assign(a.cbegin(), a.cend());
		BOOST_TEST(gv.size() == a.size());
		BOOST_CHECK_EQUAL_COLLECTIONS(gv.cbegin(), gv.cend(), a.cbegin(), a.cend());

		gv.assign(static_cast<std::size_t>(3), 42);
		BOOST_TEST(gv.size() == static_cast<std::size_t>(3));
		BOOST_TEST(gv.at(0) == 42);
		BOOST_TEST(gv.at(1) == 42);
		BOOST_TEST(gv.at(2) == 42);
	}
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(element_access)
	BOOST_AUTO_TEST_CASE(at_test) {
		ascension::detail::GapVector<int> gv;
		BOOST_CHECK_EXCEPTION(gv.at(0), std::out_of_range, [](const std::out_of_range&) {return true;});
	}

	BOOST_AUTO_TEST_CASE(front_back_test) {
		auto gv(make_random_gap_vector());
		BOOST_TEST(gv.front() == make_random_array().front());
		BOOST_TEST(gv.back() == make_random_array().back());
		BOOST_TEST(&gv.front() == &gv.front());
		BOOST_TEST(&gv.back() == &gv.back());

		gv.assign(static_cast<std::size_t>(1), 42);
		BOOST_TEST(&gv.front() == &gv.back());
	}
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_CASE(iterators_test) {
	ascension::detail::GapVector<int> gv, gv2;
	BOOST_TEST(gv.begin() == gv.end());
	BOOST_TEST(gv.cbegin() == gv.cend());
	BOOST_TEST(gv.begin() != gv2.begin());

	gv = make_random_gap_vector();
	auto i(gv.cbegin());
	BOOST_TEST(i < gv.cend());

	BOOST_TEST(std::next(i, gv.size()) == gv.cend());
	std::advance(i, gv.size());
	BOOST_TEST(i == gv.cend());
	BOOST_TEST(i > gv.cbegin());

	BOOST_TEST(std::prev(i, gv.size()) == gv.cbegin());
	std::advance(i, -static_cast<std::ptrdiff_t>(gv.size()));
	BOOST_TEST(i == gv.cbegin());
	BOOST_TEST(static_cast<std::size_t>(std::distance(i, gv.cend())) == gv.size());

	i = gv.cend();
	BOOST_TEST(i == gv.cend());

	auto a(make_random_array());
	BOOST_CHECK_EQUAL_COLLECTIONS(gv.rbegin(), gv.rend(), a.rbegin(), a.rend());
	BOOST_CHECK_EQUAL_COLLECTIONS(gv.crbegin(), gv.crend(), a.crbegin(), a.crend());

	for(auto i(gv.begin()); i != gv.end(); ++i)
		*i *= 2;
	for(auto i(a.begin()); i != a.end(); ++i)
		*i *= 2;
	BOOST_CHECK_EQUAL_COLLECTIONS(gv.cbegin(), gv.cend(), a.cbegin(), a.cend());
}

BOOST_AUTO_TEST_SUITE(modifications)
	BOOST_AUTO_TEST_CASE(insert_test1) {
		ascension::detail::GapVector<char> gv;
		char c = 'A';
		auto i(gv.insert(gv.begin(), c));
		BOOST_TEST(gv.size() == static_cast<std::size_t>(1));
		BOOST_TEST(to_string(gv) == "A");
		BOOST_TEST(i == gv.begin());

		c = 'B';
		i = gv.insert(gv.begin(), std::move(c));
		BOOST_TEST(gv.size() == static_cast<std::size_t>(2));
		BOOST_TEST(to_string(gv) == "BA");
		BOOST_TEST(i == gv.begin());

		i = gv.insert(gv.end(), 0, 'C');
		BOOST_TEST(to_string(gv) == "BA");
		BOOST_TEST(i == gv.end());

		i = gv.insert(gv.end(), 3, 'D');
		BOOST_TEST(to_string(gv) == "BADDD");
		BOOST_TEST(i == std::prev(gv.end(), 3));

		std::string s("EFG");
		i = gv.insert(gv.begin() + 3, s.cbegin(), s.cend());
		BOOST_TEST(to_string(gv) == "BADEFGDD");
		BOOST_TEST(i == std::next(gv.begin(), 3));

		s = "HIJ";
		i = gv.insert(std::prev(gv.end(), 1), s.cbegin(), s.cend());
		BOOST_TEST(to_string(gv) == "BADEFGDHIJD");
		BOOST_TEST(i == std::prev(gv.end(), 4));
	}

	BOOST_AUTO_TEST_CASE(insert_test2) {
		auto gv(make_random_gap_vector());
		std::vector<int> v(gv.cbegin(), gv.cend());
		BOOST_REQUIRE_EQUAL_COLLECTIONS(gv.cbegin(), gv.cend(), v.cbegin(), v.cend());

		gv.insert(gv.begin() + 2, 111);
		v.insert(v.begin() + 2, 111);
		BOOST_REQUIRE(gv.size() == v.size());
		for(std::size_t i = 0; i < v.size(); ++i)
			BOOST_TEST(*(gv.cbegin() + i) == v[i]);
	}

	BOOST_AUTO_TEST_CASE(erase_test) {
		const std::string source("ABCDEFGHIJKLMNOP");
		ascension::detail::GapVector<char> gv(source.cbegin(), source.cend());
		BOOST_REQUIRE_EQUAL_COLLECTIONS(gv.cbegin(), gv.cend(), source.cbegin(), source.cend());

		auto i(gv.erase(gv.cbegin()));
		BOOST_TEST(to_string(gv) == "BCDEFGHIJKLMNOP");
		BOOST_TEST(i == gv.begin());

		i = gv.erase(std::prev(gv.cend(), 1));
		BOOST_TEST(to_string(gv) == "BCDEFGHIJKLMNO");
		BOOST_TEST(i == gv.end());

		i = gv.erase(std::next(gv.cbegin(), 3), std::prev(gv.cend(), 3));
		BOOST_TEST(to_string(gv) == "BCDMNO");
		BOOST_TEST(i == std::next(gv.begin(), 3));

		i = gv.erase(gv.begin(), gv.end());
		BOOST_TEST(gv.empty());
		BOOST_TEST(i == gv.end());
	}

	BOOST_AUTO_TEST_CASE(clear_test) {
		auto gv(make_random_gap_vector());
		gv.clear();
		BOOST_TEST(gv.empty());
	}

	BOOST_AUTO_TEST_CASE(comparisons_test) {
		auto gv(make_random_gap_vector());
		auto gv2(gv);
		BOOST_TEST(gv == gv2);
		BOOST_TEST(!(gv < gv2));

		--gv.back();
		BOOST_TEST(gv < gv2);

		++gv.front();
		BOOST_TEST(gv > gv2);
	}

	BOOST_AUTO_TEST_CASE(swap_test) {
		const auto a(make_random_array());
		ascension::detail::GapVector<int> gv(a.cbegin(), a.cend()), gv2;
		std::swap(gv, gv2);
		BOOST_TEST(gv.empty());
		BOOST_CHECK_EQUAL_COLLECTIONS(gv2.cbegin(), gv2.cend(), a.cbegin(), a.cend());
	}
BOOST_AUTO_TEST_SUITE_END()
