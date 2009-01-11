/**
 * @file collator.cpp
 * @author exeal
 * @date 2007-2009
 */

#ifndef ASCENSION_NO_UNICODE_COLLATION
#include <ascension/unicode-property.hpp>
using namespace ascension;
using namespace ascension::text;
using namespace manah;
using namespace std;


// Collator /////////////////////////////////////////////////////////////////

/// Destructor.
Collator::~Collator() /*throw()*/ {
}


// CollationElementIterator /////////////////////////////////////////////////

const int CollationElementIterator::NULL_ORDER = 0xFFFFFFFFU;

/// Protected default constructor.
CollationElementIterator::CollationElementIterator() /*throw()*/ {
}

/// Destructor.
CollationElementIterator::~CollationElementIterator() /*throw()*/ {
}


// NullCollator /////////////////////////////////////////////////////////////

/// Constructor.
NullCollator::NullCollator() /*throw()*/ {
}

/// @see Collator#compare
int NullCollator::compare(const CharacterIterator& s1, const CharacterIterator& s2) const {
//	if(getStrength() == PRIMARY)
//		return CaseFolder::compare(s1, s2);
	auto_ptr<CharacterIterator> i1(s1.clone()), i2(s2.clone());
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
std::auto_ptr<CollationKey> NullCollator::collationKey(const String& s) const {
	const size_t len = s.length() * sizeof(Char) / sizeof(uchar);
	AutoBuffer<uchar> temp(new uchar[len]);
	memcpy(temp.get(), s.data(), len);
	AutoBuffer<const uchar> buffer(temp);
	return auto_ptr<CollationKey>(new CollationKey(buffer, len));
}

/// @see Collator#createCollationElementIterator
std::auto_ptr<CollationElementIterator> NullCollator::createCollationElementIterator(const CharacterIterator& source) const {
	return auto_ptr<CollationElementIterator>(new ElementIterator(source.clone()));
}

#endif // !ASCENSION_NO_UNICODE_COLLATION
