/**
 * @file collator.cpp
 * @author exeal
 * @date 2007
 */

#include "stdafx.h"
#ifndef ASCENSION_NO_UNICODE_COLLATION
#include "../unicode-property.hpp"
using namespace ascension;
using namespace ascension::unicode;
using namespace manah;
using namespace std;


// Collator /////////////////////////////////////////////////////////////////

/// Destructor.
Collator::~Collator() throw() {
}


// CollationElementIterator /////////////////////////////////////////////////

const int CollationElementIterator::NULL_ORDER = 0xFFFFFFFFU;


// NullCollator /////////////////////////////////////////////////////////////

/// Constructor.
NullCollator::NullCollator() throw() {
}

/// @see Collator#compare
int NullCollator::compare(const CharacterIterator& s1, const CharacterIterator& s2) const {
//	if(getStrength() == PRIMARY)
//		return CaseFolder::compare(s1, s2);
	auto_ptr<CharacterIterator> i1(s1.clone()), i2(s2.clone());
	while(!i1->isLast() && !i2->isLast()) {
		if(i1->current() < i2->current())
			return -1;
		else if(i1->current() > i2->current())
			return +1;
	}
	if(!i2->isLast()) return -1;
	if(!i1->isLast()) return +1;
	return 0;
}

/// @see Collator#createCollationElementIterator
std::auto_ptr<CollationElementIterator> NullCollator::createCollationElementIterator(const CharacterIterator& source) const {
	return auto_ptr<CollationElementIterator>(new ElementIterator(source.clone()));
}

/// @see Collator#getCollationKey
std::auto_ptr<CollationKey> NullCollator::getCollationKey(const String& s) const {
	const size_t len = s.length() * sizeof(Char) / sizeof(uchar);
	AutoBuffer<uchar> temp(new uchar[len]);
	memcpy(temp.get(), s.data(), len);
	return auto_ptr<CollationKey>(new CollationKey(AutoBuffer<const uchar>(temp.release()), len));
}

#endif /* !ASCENSION_NO_UNICODE_COLLATION */
