/**
 * @file named-arguments-single-type.hpp
 * @internal Defines @c detail#ArgumentsSingleType meta class.
 * @author exeal
 * @date 2016-08-05
 */

#ifndef ASCENSION_NAMED_ARGUMENTS_SINGLE_TYPE_HPP
#define ASCENSION_NAMED_ARGUMENTS_SINGLE_TYPE_HPP
#include <ascension/corelib/detail/decay-or-refer.hpp>
#include <boost/parameter/value_type.hpp>
#include <type_traits>

namespace ascension {
	namespace detail {
		/// @internal
		template<typename Arguments, typename Keyword>
		struct KeywordType {
			typedef typename std::conditional<
				!std::is_same<Keyword, void>::value,
				typename boost::parameter::value_type<Arguments, Keyword, void>::type,
				void
			>::type Type;
		};

		/// @internal
		template<typename Arguments, typename Keyword0, typename Keyword1, typename Keyword2 = void, typename Keyword3 = void>
		class NamedArgumentsSingleType {	// TODO: Bad name :(
			typedef typename KeywordType<Arguments, Keyword0>::Type T0;
			typedef typename KeywordType<Arguments, Keyword0>::Type T1;
			typedef typename KeywordType<Arguments, Keyword0>::Type T2;
			typedef typename KeywordType<Arguments, Keyword0>::Type T3;
		public:
			typedef typename DecayOrRefer<
				typename std::conditional<!std::is_same<T0, void>::value, T0,
					typename std::conditional<!std::is_same<T1, void>::value, T1,
						typename std::conditional<!std::is_same<T2, void>::value, T2,
							typename std::conditional<!std::is_same<T3, void>::value, T3,
								void
							>::type
						>::type
					>::type
				>::type
			>::Type Type;
			static_assert(!std::is_same<Type, void>::value, "ascension.detail.NamedArgumentsSingleType.Type");
		};
	}
}

#endif // !ASCENSION_NAMED_ARGUMENTS_SINGLE_TYPE_HPP
