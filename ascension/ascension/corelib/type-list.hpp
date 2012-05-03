/**
 * @file type-list.hpp
 * @author exeal
 * @date 2009-2011
 * @date 2010-10-21 renamed from manah/type-list.hpp
 */

#ifndef ASCENSION_TYPE_LIST_HPP
#define ASCENSION_TYPE_LIST_HPP
#include <ascension/corelib/type-traits.hpp>

namespace ascension {
	/**
	 * Provides "type list" features.
	 * This is emulation of "Variadic Templates" in C++11 for old or broken compilers.
	 */
	namespace typelist {
		/**
		 * Concatenates type @a U to the type @a T and generates a type list.
		 * @tparam T The first type of the type list. Can be @c void
		 * @tparam U The remaining types of the type list
		 */
		template<typename T, typename U = void> struct Cat {
			typedef T Car;	///< The first type of the type list. This may be @c void.
			typedef U Cdr;	///< The remaining types of type list.
		};

		/// Returns the first type of the given type list.
		template<typename T> struct Car : public detail::Type2Type<T> {};
		template<typename T, typename U> struct Car<Cat<T, U>> : public detail::Type2Type<T> {};

		/// Returns the types other than the first one of the given type list.
		template<typename T> struct Cdr : public detail::Type2Type<void> {};
		template<typename T, typename U> struct Cdr<Cat<T, U>> : public detail::Type2Type<U> {};

		/// Returns the length of the type list.
		template<typename Types> struct Length {
			static const unsigned value = 1 + Length<typename Cdr<Types>::Type>::value;
		};
		template<> struct Length<void> {static const unsigned value = 0;};
/*
		/// Searches the type @a T in the given type list.
		template<typename Types, typename T>
		struct Find {
		private:
			static const int temp = Find<Types::Cdr, T>::value;
		public:
			static const int value = (value != -1) ? value + 1 : -1;
		};
		template<typename T> struct Find<void, T> {static const int value = -1;};
		template<typename Cdr, typename T> struct Find<Cat<T, Cdr>, T> {static const int value = 0;};
*/
		/// Removes the first type @a T in the given type list.
		template<typename Types, typename T> struct RemoveFirst;
		template<typename Cdr, typename T> struct RemoveFirst<Cat<T, Cdr>, T> {typedef Cdr Type;};
		template<typename Car, typename Cdr, typename T>
		struct RemoveFirst<Cat<Car, Cdr>, T> {typedef Cat<Car, typename RemoveFirst<Cdr, T>::Type> Type;};
		template<typename T> struct RemoveFirst<void, T> : public detail::Type2Type<void> {};

		/// Removes the duplicated types in the given type list.
		template<typename Types> class Unique {
			typedef typename Unique<typename Types::Cdr>::Type Temp1_;
			typedef typename RemoveFirst<Temp1_, typename Types::Car>::Type Temp2_;
		public:
			typedef Cat<typename Types::Car, Temp2_> Type;
		};
		template<> class Unique<void> {
		public:
			typedef void Type;
		};

		/// Retuens the most derived (from type @a T) type in the given type list.
		template<typename Types, typename T> class MostDerived;
		template<typename Car, typename Cdr, typename T> class MostDerived<Cat<Car, Cdr>, T> {
			typedef typename MostDerived<Cdr, T>::Type Candidate_;
		public:
			typedef typename std::conditional<
				std::is_base_of<Candidate_, Car>::value, Car, Candidate_
			>::type Type;
		};
		template<typename T> class MostDerived<void, T> {
		public:
			typedef T Type;
		};

		/// Returns true if the type @a T is the most derived in the given type list.
		template<typename Types, typename T> struct IsMostDerived {
			static const bool value = std::is_same<typename MostDerived<Types, T>::Type, T>::value;
		};

	}

	namespace detail {
		template<typename Types, typename Current> struct RemoveBasesImpl {
			typedef typename std::conditional<
				typelist::IsMostDerived<
					Types,
					typename typelist::Car<Current>::Type
				>::value,
				typename typelist::Cat<
					typename typelist::Car<Current>::Type,
					typename RemoveBasesImpl<
						Types,
						typename typelist::Cdr<Current>::Type
					>::Type
				>,
				typename RemoveBasesImpl<
					Types,
					typename typelist::Cdr<Current>::Type
				>::Type
			>::type Type;
		};
		template<typename Types> struct RemoveBasesImpl<Types, void> {
			typedef void Type;
		};
	}

	namespace typelist {
		/// Removes the all types not most derived in the given type list.
		template<typename Types> struct RemoveBases {
			typedef typename detail::RemoveBasesImpl<Types, Types>::Type Type;
		};
	}
} // namespace ascension.typelist

#endif // !ASCENSION_TYPE_LIST_HPP
