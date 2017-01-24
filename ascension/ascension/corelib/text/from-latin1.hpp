/**
* @file from-latin1.hpp
* Defines @c fromLatin1 function templates.
* @author exeal
* @date 2010-10-21 separated from common.hpp
* @date 2010-11-06 separated from basic-types.hpp
* @date 2017-01-24 Separated from string-piece.hpp
*/

#ifndef ASCENSION_FROM_LATIN1_HPP
#define ASCENSION_FROM_LATIN1_HPP
#include <ascension/corelib/text/character.hpp>
#include <boost/range/iterator_range.hpp>

namespace ascension {
	/**
	 * Converts the given Latin-1 character sequence into the specified string type.
	 * @tparam StringType The destination string type
	 * @tparam Sequence The type of @a sequence
	 * @param sequence The source character sequence
	 * @return A converted string
	 */
	template<typename StringType, typename Sequence>
	inline StringType fromLatin1(const Sequence& sequence) {
//		static_assert(sizeof(boost::range_value<Sequence>::type) == 1, "");
//		assert(boost::const_begin(sequence) <= boost::const_end(sequence));
		return StringType(boost::const_begin(sequence), boost::const_end(sequence));
	}

	/**
	 * @overload
	 * @tparam StringType The destination string type
	 * @tparam Character The source character type
	 * @param p A pointer to the source character string
	 * @param length The length of @a p
	 * @return A converted string
	 * @note This function does not check if @a p is not @c null
	 */
	template<typename StringType, typename Character>
	inline StringType fromLatin1(const Character* p, std::size_t length) {
		return fromLatin1<StringType>(boost::make_iterator_range_n(p, length));
	}

	/**
	 * @overload
	 * @tparam StringType The destination string type
	 * @tparam Character The source character type
	 * @param p A pointer to the source character string. The length is calculated by @c std#char_traits#length method
	 * @return A converted string
	 * @note This function does not check if @a p is not @c null
	 */
	template<typename StringType, typename Character>
	inline StringType fromLatin1(const Character* p) {
		return fromLatin1<StringType>(p, std::char_traits<Character>::length(p));
	}

	/**
	 * @overload
	 * @tparam Sequence The type of @a sequence
	 * @param sequence The source character sequence
	 * @return A converted string
	 */
	template<typename Sequence>
	inline String fromLatin1(const Sequence& sequence) {
		return fromLatin1<String>(sequence);
	}

	/**
	 * @overload
	 * @tparam Character The source character type
	 * @param p A pointer to the source character string
	 * @param length The length of @a p
	 * @return A converted string
	 * @note This function does not check if @a p is not @c null
	 */
	template<typename Character>
	inline String fromLatin1(const Character* p, std::size_t length) {
		return fromLatin1<String>(p, length);
	}

	/**
	 * @overload
	 * @tparam Character The source character type
	 * @param p A pointer to the source character string. The length is calculated by @c std#char_traits#length method
	 * @return A converted string
	 * @note This function does not check if @a p is not @c null
	 */
	template<typename Character>
	inline String fromLatin1(const Character* p) {
		return fromLatin1<String>(p);
	}
}

#endif // !ASCENSION_FROM_LATIN1_HPP
