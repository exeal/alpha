/**
 * @file type-list.hpp
 * @author exeal
 * @date 2009-2010
 * @date 2010-10-21 renamed from manah/type-list.hpp
 */

#ifndef ASCENSION_TYPE_LIST_HPP
#define ASCENSION_TYPE_LIST_HPP
#include <ascension/internal.hpp>	// detail.Select

namespace ascension {
	namespace typelist {

		/// Concatenates type @a U to the type @a T and generates a type list.
		template<typename T, typename U = void> struct Cat {
			typedef T Car;	///< The first type of the type list.
			typedef U Cdr;	///< The remaining types of type list.
		};

		/// Returns the length of the type list.
		template<typename Types> struct Length {
			static const unsigned result = 1 + Length<typename Types::Cdr>::result;};
		template<> struct Length<void> {static const unsigned result = 0;};

		/// Removes the first type @a T in the given type list.
		template<typename Types, typename T> struct RemoveFirst;
		template<typename Cdr, typename T> struct RemoveFirst<Cat<T, Cdr>, T> {typedef Cdr Result;};
		template<typename Car, typename Cdr, typename T>
		struct RemoveFirst<Cat<Car, Cdr>, T> {typedef Cat<Car, typename RemoveFirst<Cdr, T>::Result> Result;};
		template<typename T> struct RemoveFirst<void, T> {typedef void Result;};

		/// Removes the duplicated types in the given type list.
		template<typename Types> class Unique {
			typedef typename Unique<typename Types::Cdr>::Result Temp1_;
			typedef typename RemoveFirst<Temp1_, typename Types::Car>::Result Temp2_;
		public:
			typedef Cat<typename Types::Car, Temp2_> Result;
		};
		template<> class Unique<void> {public: typedef void Result;};

		/// Retuens the most derived (from type @a T) type in the given type list.
		template<typename Types, typename T> class MostDerived;
		template<typename Car, typename Cdr, typename T> class MostDerived<Cat<Car, Cdr>, T> {
			typedef typename MostDerived<Cdr, T>::Result Candidate_;
		public:
			typedef typename detail::Select<
				detail::IsBaseAndDerived<Candidate_, Car>::result, Car, Candidate_
			>::Result Result;
		};
		template<typename T> class MostDerived<void, T> {public: typedef T Result;};

		/// Returns true if the type @a T is the most derived in the given type list.
		template<typename Types, typename T> struct IsMostDerived {
			static const bool result = detail::IsSame<typename MostDerived<Types, T>::Result, T>::result;
		};

	}

	namespace detail {
		template<typename Types, typename Current> struct RemoveBasesImpl {
			typedef typename Select<
				typelist::IsMostDerived<Types, typename Current::Car>::result,
				typelist::Cat<typename Current::Car, typename RemoveBasesImpl<Types, typename Current::Cdr>::Result>,
				typename RemoveBasesImpl<Types, typename Current::Cdr>::Result
			>::Result Result;
		};
		template<typename Types> struct RemoveBasesImpl<Types, void> {typedef void Result;};
	}

	namespace typelist {

		/// Removes the all types not most derived in the given type list.
		template<typename Types> struct RemoveBases {
			typedef typename detail::RemoveBasesImpl<Types, Types>::Result Result;
		};

	}
} // namespace ascension.typelist

#endif // !ASCENSION_TYPE_LIST_HPP
