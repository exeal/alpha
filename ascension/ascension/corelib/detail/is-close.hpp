/**
 * @file is-close.hpp
 * @internal Defines and implements @c isClose function template.
 * @author exeal
 * @date 2015-03-24 Separated from basic-types.hpp.
 */

#ifndef ASCENSION_IS_CLOSE_HPP
#define ASCENSION_IS_CLOSE_HPP
#include <cmath>
#include <type_traits>
#include <boost/config.hpp>

namespace ascension {
	namespace detail {
		/**
		 * Returns @c true if the given floating-point numbers are (approximately) equal.
		 * @tparam T The type of the all parameters
		 * @param n1 The first floating-point number
		 * @param n2 The second floating-point number
		 * @param epsilon The tolerance
		 * @return @c true if @a n1 and @a n2 are close
		 */
		template<typename T>
		inline BOOST_CONSTEXPR bool isClose(T n1, T n2, T epsilon = 1.0e-5, typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr) {
			return std::fabs(n1 - n2) <= epsilon;
		}
	}
}

#endif // !ASCENSION_IS_CLOSE_HPP
