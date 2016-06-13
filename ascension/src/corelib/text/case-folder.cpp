/**
 * @file case-folder.cpp
 * @date 2007-2011
 * @date 2011-04-25 separated from normalizer.cpp
 * @date 2011-2012, 2014
 */

#include <ascension/corelib/text/case-folder.hpp>
#include <ascension/corelib/text/string-character-iterator.hpp>


namespace ascension {
	namespace text {
		/// The maximum number of characters folded from one character.
		const Index CaseFolder::MAXIMUM_EXPANSION_CHARACTERS = 3;

		/// @internal Implements @c #compare methods.
		int CaseFolder::compare(detail::CharacterIterator i1, detail::CharacterIterator i2, bool excludeTurkishI) {
			boost::optional<CodePoint> c1, c2;
			CodePoint folded1[MAXIMUM_EXPANSION_CHARACTERS], folded2[MAXIMUM_EXPANSION_CHARACTERS];
			const CodePoint* p1 = folded1 - 1;
			const CodePoint* p2 = folded2 - 1;
			const CodePoint* last1 = p1;
			const CodePoint* last2 = p2;
		
			while(true) {
				if(c1 == boost::none) {
					if(p1 >= folded1 && p1 < last1)
						c1 = *p1++;
					else {
						c1 = i1.hasNext() ? boost::make_optional(*i1) : boost::none;
						if(c1 != boost::none) {
							++i1;
							p1 = folded1 - 1;
						}
					}
				}
				if(c2 == boost::none) {
					if(p2 >= folded2 && p2 < last2)
						c2 = *p2++;
					else {
						c2 = i2.hasNext() ? boost::make_optional(*i2) : boost::none;
						if(c2 != boost::none) {
							++i2;
							p2 = folded2 - 1;
						}
					}
				}
		
				if(c1 == c2) {
					if(c1 == boost::none)
						return 0;
					c1 = c2 = boost::none;
					continue;
				} else if(c1 == boost::none)
					return -1;
				else if(c2 == boost::none)
					return +1;
		
				// fold c1
				if(p1 == folded1 - 1) {
					p1 = folded1;
					last1 = p1 + foldFull(boost::get(c1), excludeTurkishI, folded1);
					c1 = boost::none;
					continue;
				}
				// fold c2
				if(p2 == folded2 - 1) {
					p2 = folded2;
					last2 = p2 + foldFull(boost::get(c2), excludeTurkishI, folded2);
					c2 = boost::none;
					continue;
				}
		
				return static_cast<int>(boost::get(c1)) - static_cast<int>(boost::get(c2));
			}
		}

		/**
		 * Compares the two character sequences case-insensitively.
		 * @param s1 The character sequence
		 * @param s2 The the other
		 * @param excludeTurkishI Set @c true to perform "Turkish I mapping"
		 * @retval &lt;0 The first character sequence is less than the second
		 * @retval 0 The both sequences are same
		 * @retval &gt;0 The first character sequence is greater than the second
		 */
		int CaseFolder::compare(const StringPiece& s1, const StringPiece& s2, bool excludeTurkishI /* = false */) {
			return compare(StringCharacterIterator(s1), StringCharacterIterator(s2), excludeTurkishI);
		}
	}
}
