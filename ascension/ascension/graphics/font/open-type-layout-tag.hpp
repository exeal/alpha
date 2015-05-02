/**
 * @file font-properties.hpp
 * @author exeal
 * @date 2010-11-06 created
 * @date 2010-2012 was font.hpp
 * @date 2012-08-19 separated from font.hpp
 * @date 2015-02-07 Separated from font-description.hpp
 * @date 2015-04-26 Separated from font-properties.hpp
 */

#ifndef ASCENSION_OPEN_TYPE_LAYOUT_TAG_HPP
#define ASCENSION_OPEN_TYPE_LAYOUT_TAG_HPP
#include <boost/mpl/at.hpp>
#include <boost/mpl/size.hpp>
#include <iosfwd>	// std.char_traits
#include <stdexcept>
#include <type_traits>

namespace ascension {
	namespace graphics {
		namespace font {
			/// TrueType/OpenType layout tag.
			typedef std::uint32_t OpenTypeLayoutTag;

			/**
			 * Makes an 32-bit integer represents the given TrueType/OpenType font tag.
			 * @tparam c1, c2, c3, c4 Characters consist of the tag name
			 * @see makeOpenTypeFontTag
			 */
			template<typename CharacterSequence>
			struct MakeOpenTypeLayoutTag : std::integral_constant<
				OpenTypeLayoutTag,
				(((boost::mpl::size<CharacterSequence>::type::value > 0) ?
					boost::mpl::at_c<CharacterSequence, 0>::type::value : ' ') << 24)
				| (((boost::mpl::size<CharacterSequence>::type::value > 1) ?
					boost::mpl::at_c<CharacterSequence, 1>::type::value : ' ') << 16)
				| (((boost::mpl::size<CharacterSequence>::type::value > 2) ?
					boost::mpl::at_c<CharacterSequence, 2>::type::value : ' ') << 8)
				| (((boost::mpl::size<CharacterSequence>::type::value > 3) ?
					boost::mpl::at_c<CharacterSequence, 3>::type::value : ' ') << 0)
			> {
			private:
				static const std::size_t length = boost::mpl::size<CharacterSequence>::type::value;
				static_assert(length > 0 && length < 5, "");
				static_assert(boost::mpl::at_c<CharacterSequence, 0>::type::value >= 32 && boost::mpl::at_c<CharacterSequence, 0>::type::value <= 126, "");
				static_assert(length < 2
					|| (boost::mpl::at_c<CharacterSequence, 1>::type::value >= 32 && boost::mpl::at_c<CharacterSequence, 1>::type::value <= 126), "");
				static_assert(length < 3
					|| (boost::mpl::at_c<CharacterSequence, 2>::type::value >= 32 && boost::mpl::at_c<CharacterSequence, 2>::type::value <= 126), "");
				static_assert(length < 4
					|| (boost::mpl::at_c<CharacterSequence, 3>::type::value >= 32 && boost::mpl::at_c<CharacterSequence, 3>::type::value <= 126), "");
			};

			/**
			 * Returns an 32-bit integer represents the given TrueType/OpenType layout tag.
			 * @tparam Character The character type of @a name
			 * @param name The TrueType tag name
			 * @param validate Set @c true to validate characters in @a name
			 * @return The 32-bit integral TrueType tag value
			 * @throw std#length_error The length of @a name is zero or greater four
			 * @throw std#invalid_argument @a validate is @c true and any character in @a name was invalid
			 * @see MakeOpenTypeLayoutTag
			 */
			template<typename Character>
			inline OpenTypeLayoutTag makeOpenTypeLayoutTag(const Character name[], bool validate = true) {
				const std::size_t len = std::char_traits<Character>::length(name);
				if(len == 0 || len > 4)
					throw std::length_error("name");
				OpenTypeFontTag tag = 0;
				std::size_t i = 0;
				for(; i < len; ++i) {
					if(validate && (name[i] < 32 || name[i] > 126))
						throw std::invalid_argument("name");
					tag |= name[i] << ((3 - i) * 8);
				}
				for(; i < 4; ++i)
					tag |= ' ' << ((3 - i) * 8);
				return tag;
			}
		}
	}
}

#endif // !ASCENSION_OPEN_TYPE_LAYOUT_TAG_HPP
