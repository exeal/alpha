/**
 * @file case-folder.cpp
 * @date 2007-2011
 * @date 2011-04-25 separated from normalizer.cpp
 * @date 2011-2012
 */

#include <ascension/corelib/text/case-folder.hpp>

using namespace ascension;
using namespace ascension::text;
using namespace std;


/// The maximum number of characters folded from one character.
const Index CaseFolder::MAXIMUM_EXPANSION_CHARACTERS = 3;

/**
 * Compares the two character sequences case-insensitively.
 * @param s1 The character sequence
 * @param s2 The the other
 * @param excludeTurkishI Set @c true to perform "Turkish I mapping"
 * @retval &lt;0 The first character sequence is less than the second
 * @retval 0 The both sequences are same
 * @retval &gt;0 The first character sequence is greater than the second
 */
int CaseFolder::compare(const CharacterIterator& s1, const CharacterIterator& s2, bool excludeTurkishI /* = false */) {
	unique_ptr<CharacterIterator> i1(s1.clone()), i2(s2.clone());
	CodePoint c1 = CharacterIterator::DONE, c2 = CharacterIterator::DONE;
	CodePoint folded1[MAXIMUM_EXPANSION_CHARACTERS], folded2[MAXIMUM_EXPANSION_CHARACTERS];
	const CodePoint* p1 = folded1 - 1;
	const CodePoint* p2 = folded2 - 1;
	const CodePoint* last1 = p1;
	const CodePoint* last2 = p2;

	while(true) {
		if(c1 == CharacterIterator::DONE) {
			if(p1 >= folded1 && p1 < last1)
				c1 = *p1++;
			else if(CharacterIterator::DONE != (c1 = i1->current())) {
				i1->next();
				p1 = folded1 - 1;
			}
		}
		if(c2 == CharacterIterator::DONE) {
			if(p2 >= folded2 && p2 < last2)
				c2 = *p2++;
			else if(CharacterIterator::DONE != (c2 = i2->current())) {
				i2->next();
				p2 = folded2 - 1;
			}
		}

		if(c1 == c2) {
			if(c1 == CharacterIterator::DONE)
				return 0;
			c1 = c2 = CharacterIterator::DONE;
			continue;
		} else if(c1 == CharacterIterator::DONE)
			return -1;
		else if(c2 == CharacterIterator::DONE)
			return +1;

		// fold c1
		if(p1 == folded1 - 1) {
			p1 = folded1;
			last1 = p1 + foldFull(c1, excludeTurkishI, folded1);
			c1 = CharacterIterator::DONE;
			continue;
		}
		// fold c2
		if(p2 == folded2 - 1) {
			p2 = folded2;
			last2 = p2 + foldFull(c2, excludeTurkishI, folded2);
			c2 = CharacterIterator::DONE;
			continue;
		}

		return static_cast<int>(c1) - static_cast<int>(c2);
	}
}
