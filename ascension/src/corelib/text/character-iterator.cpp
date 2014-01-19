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
#include <ascension/corelib/text/utf-iterator.hpp>	// utf.makeCharacterDecodeIterator


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

		/// Indicates the iterator is the last.
		const CodePoint CharacterIterator::DONE = 0xfffffffful;


		// StringCharacterIterator ////////////////////////////////////////////////////////////////////////

		const CharacterIterator::ConcreteTypeTag
			StringCharacterIterator::CONCRETE_TYPE_TAG_ = CharacterIterator::ConcreteTypeTag();

		/// Default constructor.
		StringCharacterIterator::StringCharacterIterator() BOOST_NOEXCEPT
				: CharacterIterator(CONCRETE_TYPE_TAG_) {
		}

		StringCharacterIterator::StringCharacterIterator(const StringPiece& text) :
				CharacterIterator(CONCRETE_TYPE_TAG_), current_(text.cbegin()), range_(text) {
		}

		StringCharacterIterator::StringCharacterIterator(const StringPiece& text, const StringPiece::const_iterator start) :
				CharacterIterator(CONCRETE_TYPE_TAG_), current_(start), range_(text) {
			if(tell() < beginning() || tell() > end())
				throw std::invalid_argument("invalid input.");
		}

		StringCharacterIterator::StringCharacterIterator(const String& text) :
				CharacterIterator(CONCRETE_TYPE_TAG_), current_(text.data()), range_(text) {
			if(tell() < beginning() || tell() > end())
				throw std::invalid_argument("invalid input.");
		}

		StringCharacterIterator::StringCharacterIterator(const String& text, String::const_iterator start) :
				CharacterIterator(CONCRETE_TYPE_TAG_), current_(text.data() + (start - std::begin(text))), range_(text) {
			if(tell() < beginning() || tell() > end())
				throw std::invalid_argument("invalid input.");
		}

		/// Copy-constructor.
		StringCharacterIterator::StringCharacterIterator(const StringCharacterIterator& other) BOOST_NOEXCEPT
				: CharacterIterator(other), current_(other.current_), range_(other.range_) {
		}

		/// @see CharacterIterator#doAssign
		void StringCharacterIterator::doAssign(const CharacterIterator& other) {
			CharacterIterator::operator=(other);
			current_ = static_cast<const StringCharacterIterator&>(other).current_;
			range_ = static_cast<const StringCharacterIterator&>(other).range_;
		}

		/// @see CharacterIterator#doClone
		std::unique_ptr<CharacterIterator> StringCharacterIterator::doClone() const {
			return std::unique_ptr<CharacterIterator>(new StringCharacterIterator(*this));
		}

		/// @see CharacterIterator#doEquals
		bool StringCharacterIterator::doEquals(const CharacterIterator& other) const {
			return tell() == static_cast<const StringCharacterIterator&>(other).tell();
		}

		/// @see CharacterIterator#doFirst
		void StringCharacterIterator::doFirst() {
			current_ = beginning();
		}

		/// @see CharacterIterator#doLast
		void StringCharacterIterator::doLast() {
			current_ = end();
		}

		/// @see CharacterIterator#doLess
		bool StringCharacterIterator::doLess(const CharacterIterator& other) const {
			return tell() < static_cast<const StringCharacterIterator&>(other).tell();
		}

		/// @see CharacterIterator#doNext
		void StringCharacterIterator::doNext() {
			if(tell() == end())
//				throw std::out_of_range("the iterator is the last.");
				return;
			current_ = (++utf::makeCharacterDecodeIterator(beginning(), end(), tell())).tell();
		}

		/// @see CharacterIterator#doPrevious
		void StringCharacterIterator::doPrevious() {
			if(tell() == beginning())
//				throw std::out_of_range("the iterator is the first.");
				return;
			current_ = (--utf::makeCharacterDecodeIterator(beginning(), end(), tell())).tell();
		}
	}
}
