/**
 * @file types.hpp
 * @date 2009 exeal
 */

#ifndef MANAH_TYPES_HPP
#define MANAH_TYPES_HPP

namespace manah {

	/// Generates a type from the constant integer.
	template<int v> struct Int2Type {static const int value = v;};

	/// Returns the type @a T if @a condition is true, otherwise type @a U.
	template<bool condition, typename T, typename U> struct Select {typedef T Result;};
	template<typename T, typename U> struct Select<false, T, U> {typedef U Result;};

	/// Returns true if the given two types @a T and @a U are same.
	template<typename T, typename U> struct IsSame {static const bool result = false;};
	template<typename T> struct IsSame<T, T> {static const bool result = true;};

	/// Returns true if the type @a D is derived from the type @a B.
	template<typename B, typename D>
	class IsBaseAndDerived {
		typedef char Y;
		class N {char padding_[8];};
		static Y test(const volatile B*);
		static N test(...);
		static const volatile D* makeD();
	public:
		static const bool result = !IsSame<B, D>::result && sizeof(test(makeD())) == sizeof(Y);
	};
} // namespace manah

#endif // !MANAH_TYPES_HPP
