/**
 * @file collator.cpp
 * @author exeal
 * @date 2007-2011
 */

#include <ascension/config.hpp>	// ASCENSION_NO_UNICODE_COLLATION
#ifndef ASCENSION_NO_UNICODE_COLLATION
#include <ascension/corelib/text/collator.hpp>
using namespace ascension;
using namespace ascension::text;
using namespace std;


// Collator ///////////////////////////////////////////////////////////////////////////////////////

/// Destructor.
Collator::~Collator() BOOST_NOEXCEPT {
}


// CollationElementIterator ///////////////////////////////////////////////////////////////////////

const int CollationElementIterator::NULL_ORDER = 0xfffffffful;

/// Protected default constructor.
CollationElementIterator::CollationElementIterator() BOOST_NOEXCEPT {
}

/// Destructor.
CollationElementIterator::~CollationElementIterator() BOOST_NOEXCEPT {
}


// NullCollator ///////////////////////////////////////////////////////////////////////////////////

/// Constructor.
NullCollator::NullCollator() BOOST_NOEXCEPT {
}

/// @see Collator#compare
int NullCollator::compare(const CharacterIterator& s1, const CharacterIterator& s2) const {
//	if(getStrength() == PRIMARY)
//		return CaseFolder::compare(s1, s2);
	unique_ptr<CharacterIterator> i1(s1.clone()), i2(s2.clone());
	while(i1->hasNext() && i2->hasNext()) {
		if(i1->current() < i2->current())
			return -1;
		else if(i1->current() > i2->current())
			return +1;
	}
	if(i2->hasNext()) return -1;
	if(i1->hasNext()) return +1;
	return 0;
}

/// @see Collator#getCollationKey
unique_ptr<CollationKey> NullCollator::collationKey(const String& s) const {
	const size_t len = s.length() * sizeof(Char) / sizeof(uint8_t);
	unique_ptr<uint8_t[]> temp(new uint8_t[len]);
	memcpy(temp.get(), s.data(), len);
	unique_ptr<const uint8_t[]> buffer(temp);
	return unique_ptr<CollationKey>(new CollationKey(buffer, len));
}

/// @see Collator#createCollationElementIterator
std::unique_ptr<CollationElementIterator> NullCollator::createCollationElementIterator(const CharacterIterator& source) const {
	return unique_ptr<CollationElementIterator>(new ElementIterator(source.clone()));
}

#endif // !ASCENSION_NO_UNICODE_COLLATION
