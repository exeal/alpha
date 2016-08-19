/**
 * @file code-unit-size-of.hpp
 * Defines @c CodeUnitSizeOf meta function.
 * @author exeal
 * @date 2005-2011 was unicode.hpp
 * @date 2011-04-27 renamed from unicode.hpp
 * @date 2016-08-15 Separated from character.hpp.
 */

#ifndef ASCENSION_CODE_UNIT_SIZE_OF_HPP
#define ASCENSION_CODE_UNIT_SIZE_OF_HPP
#include <ascension/corelib/future/type-traits.hpp>	// std.integral_constant
#include <boost/mpl/identity.hpp>
#include <boost/optional.hpp>
#include <cstddef>
#include <iterator>

#if ASCENSION_UNICODE_VERSION > 0x0510
#	error These class definitions and implementations are based on old version of Unicode.
#endif

namespace ascension {
	namespace text {
		namespace detail {
			template<typename Iterator> class IteratorValue {
				typedef typename std::iterator_traits<Iterator>::value_type T1;
				template<typename T> struct T2 : boost::mpl::identity<T> {};
				template<typename T> struct T2<boost::optional<T>> : boost::mpl::identity<typename boost::optional<T>::value_type> {};
			public:
				typedef typename T2<T1>::type type;
			};
		}

		/**
		 * Returns the size of a code unit of the specified code unit sequence in bytes.
		 * @tparam CodeUnitSequence The type represents a code unit sequence
		 */
		template<typename CodeUnitSequence> struct CodeUnitSizeOf
			: std::integral_constant<std::size_t, sizeof(typename detail::IteratorValue<CodeUnitSequence>::type)> {};
		template<typename T> struct CodeUnitSizeOf<std::back_insert_iterator<T>>
			: std::integral_constant<std::size_t, sizeof(typename T::value_type)> {};
		template<typename T> struct CodeUnitSizeOf<std::front_insert_iterator<T>>
			: std::integral_constant<std::size_t, sizeof(typename T::value_type)> {};
		template<typename T, typename U> struct CodeUnitSizeOf<std::ostream_iterator<T, U>>
			: std::integral_constant<std::size_t, sizeof(T)> {};
	}
} // namespace ascension.text

#endif // !ASCENSION_CODE_UNIT_SIZE_OF_HPP
