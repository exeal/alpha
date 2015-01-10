/**
 * @internal
 * @file decay-or-refer
 * @date 2012-03-31 created
 * @date 2012-2014 was directions.hpp
 * @date 2015-01-09 Separated from directions.hpp
 */

#ifndef ASCENSION_DECAY_OR_REFER_HPP
#define ASCENSION_DECAY_OR_REFER_HPP

#include <functional>
#include <type_traits>

namespace ascension {
	namespace detail {
		template<typename T> struct DecayOrRefer {
			typedef typename std::decay<T>::type Type;
		};
		template<typename T> struct DecayOrRefer<std::reference_wrapper<T>> {
			typedef T& Type;
		};
	}
}

#endif // !ASCENSION_DECAY_OR_REFER_HPP
