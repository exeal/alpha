/**
 * @file string-piece.hpp
 * Defines @c StringPiece class.
 * @author exeal
 * @date 2010-10-21 separated from common.hpp
 * @date 2010-11-06 separated from basic-types.hpp
 * @date 2012-2013
 */

#ifndef ASCENSION_STRING_PIECE_HPP
#define ASCENSION_STRING_PIECE_HPP
#include <ascension/corelib/basic-types.hpp>	// Char
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
#	include <ascension/corelib/range.hpp>
#	include <stdexcept>							// std.out_of_range
#	include <string>
#else
#	include <cassert>
#	include <boost/utility/string_ref.hpp>
#endif	// ASCENSION_ABANDONED_AT_VERSION_08

namespace ascension {

#ifdef ASCENSION_ABANDONED_AT_VERSION_08
	/**
	 * String-like object addresses a sized piece of memory.
	 * @tparam Character The character type
	 * @tparam CharacterTraits The character traits type gives @c length class method returns a
	 *                         length of a string
	 * @note Constructors do <strong>not</strong> check if their parameters are @c null.
	 */
	template<typename Character, typename CharacterTraits = std::char_traits<Character>>
	class BasicStringPiece : public Range<const Character*> {
	public:
		typedef Character value_type;
		typedef const value_type* const_pointer;
		typedef const value_type& const_reference;
		typedef std::basic_string<value_type> string_type;
		typedef typename string_type::size_type size_type;
		typedef CharacterTraits traits_type;
	public:
		/// Default constructor.
		BasicStringPiece() : Range<const Character*>(nullptr, nullptr) {}
		/**
		 * Implicit constructor. The length of the string is calculated by using
		 * @c traits_type#length function.
		 * @param p A pointer addresses the beginning of the string
		 */
		BasicStringPiece(const_pointer p) :
			Range<const Character*>(p, (p != nullptr) ? p + traits_type::length(p) : nullptr) {}
		/**
		 * Constructor takes the beginning and the end of a string.
		 * @param first A pointer addresses the beginning of the string
		 * @param last A pointer addresses the end of the string
		 */
		BasicStringPiece(const_pointer first, const_pointer last) : Range<const Character*>(first, last) {}
		/**
		 * Constructor takes a pointer to the beginning of the string and the length.
		 * @param p A pointer addresses the beginning of the string
		 * @param n The length of the string
		 */
		BasicStringPiece(const_pointer p, size_type n) : Range<const Character*>(p, p + n) {}
		/**
		 * Implicit constructor takes a standard C++ string object.
		 * @param s The string object
		 */
		BasicStringPiece(const string_type& s) : Range<const Character*>(s.data(), s.data() + s.length()) {}
		/**
		 * Returns the character at the specified position in the string.
		 * @param i The index of the position of the character to get. if @a i is equal to or
		 *          greater than the length of the string
		 * @return The character
		 */
		value_type operator[](size_type i) const {return Range<const Character*>::beginning()[i];}
		/**
		 * Returns the character at the specified position in the string.
		 * @param i The index of the position of the character to get
		 * @return The character
		 * @throw std#out_of_range @a i is equal to or greater than the length of the string
		 */
		value_type at(size_type i) const {
			if(i >= static_cast<size_type>(length(*this)))
				throw std::out_of_range("i");
			return operator[](i);
		}
	};
#endif	// ASCENSION_ABANDONED_AT_VERSION_08

#ifdef ASCENSION_ABANDONED_AT_VERSION_08
	/// Specialization of @c BasicStringPiece for @c Char type.
	typedef BasicStringPiece<Char> StringPiece;
#else
	/// Specialization of @c boost#basic_string_ref for @c Char type.
	typedef boost::basic_string_ref<Char, std::char_traits<Char>> StringPiece;

	/// Creates and returns a @c StringPiece with the given two iterators.
	inline StringPiece makeStringPiece(StringPiece::const_iterator first, StringPiece::const_iterator last) {
		assert(first <= last);
		return StringPiece(first, last - first);
	}
#endif	// ASCENSION_ABANDONED_AT_VERSION_08

} // namespace ascension

#endif // !ASCENSION_STRING_PIECE_HPP
