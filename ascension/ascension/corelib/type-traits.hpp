/**
 * @file type-traits.hpp
 * @author exeal
 * @date 2011-03-06 created
 */

#ifndef ASCENSION_TYPE_TRAITS_HPP
#define ASCENSION_TYPE_TRAITS_HPP

namespace ascension {
	namespace detail {

		/// From @c std#integral_constant.
		template<typename T, T v>
		struct IntegralConstant {
			typedef IntegralConstant<T, v> Type;
			typedef T ValueType;
			static const ValueType value = v;
		};

		/// From @c std#true_type.
		typedef IntegralConstant<bool, true> TrueType;
		/// From @c std#false_type.
		typedef IntegralConstant<bool, false> FalseType;

		/// Returns the type @a T if @a condition is @c true, otherwise type @a U.
		template<bool condition, typename T, typename U> struct Select {typedef T Type;};
		template<typename T, typename U> struct Select<false, T, U> {typedef U Type;};

		/// Returns @c true if the given two types @a T and @a U are same.
		template<typename T, typename U> struct IsSame : FalseType {};
		template<typename T> struct IsSame<T, T> : TrueType {};

		/// Returns @c true if the type @a D is derived from the type @a B.
		template<typename B, typename D> class IsBaseAndDerived {
			typedef char Y;
			class N {char padding_[8];};
			static Y test(const volatile B*);
			static N test(...);
			static const volatile D* makeD();
		public:
			static const bool value = !IsSame<B, D>::value && sizeof(test(makeD())) == sizeof(Y);
		};

		/// Generates signed numeral types.
		template<typename T> struct RemoveSigned;
		template<> struct RemoveSigned<unsigned char> {typedef char Type;};
		template<> struct RemoveSigned<unsigned short> {typedef short Type;};
		template<> struct RemoveSigned<unsigned int> {typedef int Type;};
		template<> struct RemoveSigned<unsigned long> {typedef long Type;};
//		template<> struct RemoveSigned<unsigned __int64> {typedef __int64 Type;};

		/**
		 * @def ASCENSION_DEFINE_HAS_METHOD
		 * @internal
		 */
#		define ASCENSION_DEFINE_HAS_METHOD(methodName, methodSignature)							\
			template<typename T> class Has_##methodName {										\
				typedef char Yes;																\
				typedef char(&No)[2];															\
				template<typename U, U> struct Test;											\
				template<typename U> static Yes test(Test<methodSignature, &U::methodName>*);	\
				template<typename U> static No test(...);										\
			public:																				\
				static const bool value = sizeof(test<T>(0)) == sizeof(Yes);					\
			}

	}
}

#endif // !ASCENSION_TYPE_TRAITS_HPP
