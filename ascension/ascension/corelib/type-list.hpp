/**
 * @file type-list.hpp
 * @author exeal
 * @date 2009-2011
 * @date 2010-10-21 renamed from manah/type-list.hpp
 */

#ifndef ASCENSION_TYPE_LIST_HPP
#define ASCENSION_TYPE_LIST_HPP
#include <ascension/corelib/future/type-traits.hpp>
#include <boost/mpl/identity.hpp>

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
		template<typename T> struct Car : public boost::mpl::identity<T> {};
		template<typename T, typename U> struct Car<Cat<T, U>> : public boost::mpl::identity<T> {};

		/// Returns the types other than the first one of the given type list.
		template<typename T> struct Cdr : public boost::mpl::identity<void> {};
		template<typename T, typename U> struct Cdr<Cat<T, U>> : public boost::mpl::identity<U> {};

		/// Returns the length of the type list.
		template<typename Types> struct Length {
			static const unsigned value = 1 + Length<typename Cdr<Types>::type>::value;
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
		template<typename Cdr, typename T> struct RemoveFirst<Cat<T, Cdr>, T> {typedef Cdr type;};
		template<typename Car, typename Cdr, typename T>
		struct RemoveFirst<Cat<Car, Cdr>, T> {typedef Cat<Car, typename RemoveFirst<Cdr, T>::type> type;};
		template<typename T> struct RemoveFirst<void, T> : public boost::mpl::identity<void> {};

		/// Removes the duplicated types in the given type list.
		template<typename Types> class Unique {
			typedef typename Unique<typename Types::Cdr>::type Temp1_;
			typedef typename RemoveFirst<Temp1_, typename Types::Car>::type Temp2_;
		public:
			typedef Cat<typename Types::Car, Temp2_> type;
		};
		template<> class Unique<void> {
		public:
			typedef void type;
		};

		/// Retuens the most derived (from type @a T) type in the given type list.
		template<typename Types, typename T> class MostDerived;
		template<typename Car, typename Cdr, typename T> class MostDerived<Cat<Car, Cdr>, T> {
			typedef typename MostDerived<Cdr, T>::type Candidate_;
		public:
			typedef typename std::conditional<
				std::is_base_of<Candidate_, Car>::value, Car, Candidate_
			>::type type;
		};
		template<typename T> class MostDerived<void, T> {
		public:
			typedef T type;
		};

		/// Returns true if the type @a T is the most derived in the given type list.
		template<typename Types, typename T> struct IsMostDerived {
			static const bool value = std::is_same<typename MostDerived<Types, T>::type, T>::value;
		};

	}

	namespace detail {
		template<typename Types, typename Current> struct RemoveBasesImpl {
			typedef typename std::conditional<
				typelist::IsMostDerived<
					Types,
					typename typelist::Car<Current>::type
				>::value,
				typename typelist::Cat<
					typename typelist::Car<Current>::type,
					typename RemoveBasesImpl<
						Types,
						typename typelist::Cdr<Current>::type
					>::type
				>,
				typename RemoveBasesImpl<
					Types,
					typename typelist::Cdr<Current>::type
				>::type
			>::type type;
		};
		template<typename Types> struct RemoveBasesImpl<Types, void> {
			typedef void type;
		};
	}

	namespace typelist {
		/// Removes the all types not most derived in the given type list.
		template<typename Types> struct RemoveBases {
			typedef typename detail::RemoveBasesImpl<Types, Types>::type type;
		};
	}
} // namespace ascension.typelist

#endif // !ASCENSION_TYPE_LIST_HPP
