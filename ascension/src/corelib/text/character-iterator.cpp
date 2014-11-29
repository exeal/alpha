/**
 * @file character-iterator.cpp
 * @author exeal
 * @date 2005-2011 was unicode-property.cpp
 * @date 2011-05-07 renamed from unicode-property.cpp
 * @date 2011-09-21 split from character-property.cpp
 * @date 2014
 */

#include <ascension/directions.hpp>
#include <ascension/corelib/text/character-iterator.hpp>


namespace ascension {
	// Direction //////////////////////////////////////////////////////////////////////////////////////

	// oh, why are these here???
	const Direction Direction::FORWARD(true);
	const Direction Direction::BACKWARD(false);

	namespace text {
		// CharacterIterator //////////////////////////////////////////////////////////////////////////////
		
		/**
		 * @class ascension::text::CharacterIterator
		 * Abstract class defines an interface for bidirectional iteration on text.
		 *
		 * <h3>Code point-based interface</h3>
		 *
		 * The operations perform using code point (not code unit). @c #current returns a code point (not
		 * code unit value) of the character the iterator adresses, and @c #next skips a legal low
		 * surrogate code unit.
		 *
		 * Following example prints all code points of the text.
		 *
		 * @code
		 * void printAllCharacters(CharacterIterator& i) {
		 *   for(i.first(); i.hasNext(); i.next())
		 *     print(i.current());
		 * }
		 * @endcode
		 *
		 * Relational operations (@c #equals and @c #less) must be applied to the same types.
		 *
		 * @code
		 * StringCharacterIterator i1 = ...;
		 * unique_ptr<CharacterIterator> i2 = i1.clone(); // i2 is a clone of i1
		 * StringCharacterIterator i3 = ...;              // i3 is not a clone of i1, but has a same type
		 * DocumentCharacterIterator i4 = ...;            // i4 is not a clone of i1, and has a different type
		 *
		 * i1.equals(*i2); // ok
		 * i2->less(i1);   // ok
		 * i1.equals(i3);  // ok
		 * i1.equals(i4);  // error! (std::invalid_argument exception)
		 * @endcode
		 *
		 * Also, @c #assign has a like restricton. Any partial assignments are not allowed.
		 *
		 * <h3>Offsets</h3>
		 *
		 * A @c CharacterIterator has a position in the character sequence (offset). Initial offset value
		 * is 0, and will be decremented or incremented when the iterator moves.
		 *
		 * The offset will be reset to 0 also when @c first or @c last is called.
		 *
		 * @code
		 * CharacterIterator i = ...;
		 * i.first();    // offset == 0
		 * i.next();     // offset == 1 (or 2 if the first character is not in BMP)
		 * i.last();     // offset == 0
		 * i.previous(); // offset == -1 (or -2)
		 * @endcode
		 *
		 * <h3>Implementation of @c CharacterIterator class</h3>
		 *
		 * A concrete iterator class must implement the following private methods:
		 *
		 * - @c #doAssign for @c #assign.
		 * - @c #doClone for @c #clone.
		 * - @c #doFirst and @c #doLast for @c #first and @c #last.
		 * - @c #doEquals and @c #doLess for @c #equals and @c #less.
		 * - @c #doNext and @c #doPrevious for @c #next and @c #previous
		 *
		 * And also must implement the following public methods: @c #current, @c #hasNext, @c #hasPrevious.
		 *
		 * @c #doClone must be implemented by protected copy-constructor of @c CharacterIterator.
		 * @c #doAssign must be implemented by protected <code>CharacterIterator#operator=</code>.
		 *
		 * <h3>Type-safety</h3>
		 *
		 * @c CharacterIterator is an abstract type and actual types of two concrete instances may be
		 * different. This makes comparison of iterators difficult.
		 *
		 * Instances of @c CharacterIterator know the derived class (by using @c ConcreteTypeTag). So the
		 * right-hand-sides of @c #doAssign, @c #doEquals, and @c #doLess have same types of the callee.
		 * This means that the following implementation with down-cast is safe.
		 *
		 * @code
		 * bool MyIterator::equals(const CharacterIterator& other) const {
		 *   // 'other' actually refers a MyIterator.
		 *   const MyIterator& concrete = static_cast<const MyIterator&>(other);
		 *   // compare this and concrete...
		 * }
		 * @endcode
		 */
	}
}
