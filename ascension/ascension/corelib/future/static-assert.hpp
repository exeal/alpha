/**
 * @file static-assert.hpp
 * @author exeal
 * @date 2012-05-31 separated from future.hpp
 * @deprecated 0.8 Use C++11 @c static_assert
 */

#ifndef ASCENSION_STATIC_ASSERT_HPP
#define ASCENSION_STATIC_ASSERT_HPP

namespace ascension {
	namespace detail {
		template<unsigned> struct StaticAssertTest {};
		template<int> struct StaticAssertionFailureAtLine;
		template<> struct StaticAssertionFailureAtLine<-1> {};
	} // namespace detail

	#define ASCENSION_STATIC_ASSERT(expression)														\
		typedef ascension::detail::StaticAssertTest<												\
			sizeof(ascension::detail::StaticAssertionFailureAtLine<(expression) ? -1 : __LINE__>)	\
		> oh_static_assertion_shippaidayo_orz
}

#endif // !ASCENSION_STATIC_ASSERT_HPP
