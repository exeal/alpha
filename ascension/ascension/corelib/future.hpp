/**
 * @file future.hpp
 * @author exeal
 * @date 2004-2012 was basic-types.hpp
 * @date 2012-02-12 separated from basic-types.hpp
 */

#ifndef ASCENSION_FUTURE_HPP
#define ASCENSION_FUTURE_HPP

#include <cstddef>
//#include <ascension/config.hpp>
//#include <ascension/platforms.hpp>
#include <ascension/corelib/future/scoped-enum-emulation.hpp>
#include <ascension/corelib/future/static-assert.hpp>
#include <ascension/corelib/future/type-traits.hpp>

namespace ascension {
	template<typename T, std::size_t n>
	/*constexpr*/ inline std::size_t countof(T (&a)[n]) {
		return n;
	}
	template<typename T, std::size_t n>
	/*constexpr*/ inline T* endof(T (&a)[n]) {
		return a + countof(a);
	}
	template<typename T, std::size_t n>
	/*constexpr*/ inline const T* endof(const T (&a)[n]) {
		return a + countof(a);
	}
}

/// Returns the number of the elements of the given array.
#define ASCENSION_COUNTOF(array) (sizeof(array) / sizeof((array)[0]))
/// Returns the end of the given array.
#define ASCENSION_ENDOF(array) ((array) + ASCENSION_COUNTOF(array))

#endif // !ASCENSION_FUTURE_HPP
