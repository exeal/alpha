/**
 * @file combination.hpp
 * Defines @c Combination class template.
 * @author exeal
 * @date 2017-01-01 Separated from keyboard-modifier.hpp.
 */

#ifndef ASCENSION_COMBINATION_HPP
#define ASCENSION_COMBINATION_HPP
#include <boost/config.hpp>
#include <bitset>
#include <type_traits>
#include <utility>	// std.tuple_size

namespace ascension {
	template<typename T, std::size_t N>
	class Combination : public std::bitset<N> {
		static_assert(std::is_enum<T>::value, "");
	public:
		typedef T value_type;

		/// Creates an empty @c Combination object.
		BOOST_CONSTEXPR Combination() BOOST_NOEXCEPT {}

		/**
		 * Creates a @c Combination object which has only the specified singular value.
		 * @param bit The bit
		 * @throw std#out_of_range @a value is invalid
		 */
		Combination(value_type bit) {
			set(bit);
		}

		/**
	 	 * Creates a @c Combination object with the specified bits.
		 * @tparam Sequence The fixed-size sequence of @c value_type
		 * @param sequence The sequence of @c value_type bits
		 * @throw std#out_of_range @a sequence has invalid value
		 */
		template<typename Sequence>
		explicit Combination(const Sequence& sequence) {
			_set(sequence, std::integral_constant<std::size_t, std::tuple_size<Sequence>::value - 1>());
		}

		using std::bitset<N>::operator==;
		using std::bitset<N>::operator!=;
		using std::bitset<N>::operator&=;
		using std::bitset<N>::operator|=;
		using std::bitset<N>::operator^=;

	private:
		bool operator==(T) const;
		bool operator!=(T) const;
		Combination& operator&=(T);
		Combination& operator|=(T);
		Combination& operator^=(T);
		Combination operator&(T) const;
		Combination operator|(T) const;
		Combination operator^(T) const;
		template<typename Sequence, std::size_t M>
		void _set(const Sequence& sequence, std::integral_constant<std::size_t, M>, typename std::enable_if<M != 0>::type* = nullptr) {
			set(std::get<M>(sequence));
			return _set(sequence, std::integral_constant<std::size_t, M - 1>());
		}
		template<typename Sequence>
		void _set(const Sequence& sequence, std::integral_constant<std::size_t, 0>) {
			set(std::get<0>(sequence));
		}
	};
}

#endif // !ASCENSION_COMBINATION_HPP
