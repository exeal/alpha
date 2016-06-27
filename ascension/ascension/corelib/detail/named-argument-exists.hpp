/**
 * @file named-argument-exists.hpp
 * Defines ArgumentExists internal class template.
 * @author exeal
 * @date 2016-06-27 Created.
 */

#ifndef ASCENSION_NAMED_ARGUMENT_EXISTS_HPP
#define ASCENSION_NAMED_ARGUMENT_EXISTS_HPP
#include <boost/parameter/value_type.hpp>
#include <type_traits>

namespace ascension {
	namespace detail {
		/**
		 * Returns @c true if the given argument pack has the specified keyword tag type.
		 * @tparam Arguments A model of ArgumentPack
		 * @tparam Keyword The keyword tag type
		 */
		template<typename Arguments, typename Keyword>
		struct NamedArgumentExists : std::integral_constant<
			bool, 
			!std::is_same<
				typename boost::parameter::value_type<
					Arguments, Keyword, void
				>::type, void
			>::value
		> {};
	}
}

#endif // !ASCENSION_NAMED_ARGUMENT_EXISTS_HPP
