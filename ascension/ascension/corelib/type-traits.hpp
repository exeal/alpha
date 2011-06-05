/**
 * @file type-traits.hpp
 * @author exeal
 * @date 2011-03-06 created
 */

#ifndef ASCENSION_TYPE_TRAITS_HPP
#define ASCENSION_TYPE_TRAITS_HPP

#include <ascension/config.hpp>
#if defined(ASCENSION_COMPILER_MSVC) && _MSC_VER >= 1500
#	include <type_traits>
#elif defined(ASCENSION_COMPILER_GCC) && __GNUC__ >= 4
#	include <tr1/type_traits>
#else
#	include <boost/tr1/type_traits.hpp>
#endif

namespace ascension {
	namespace detail {

		/**
		 * Returns the type @a T if @a condition is @c true, otherwise type @a U.
		 * @deprecated 0.8 Use @c std#conditional out of TR1.
		 */
		template<bool condition, typename T, typename U> struct Select {typedef T Type;};
		template<typename T, typename U> struct Select<false, T, U> {typedef U Type;};

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
