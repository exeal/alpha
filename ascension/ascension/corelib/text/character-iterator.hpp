/**
 * @file character-iterator.hpp
 * Defines iterator classes traverse Unicode character sequence.
 * @author exeal
 * @date 2005-2010 was unicode.hpp
 * @date 2010-2014
 * @see utf-iterator.hpp
 */

#ifndef ASCENSION_CHARACTER_ITERATOR_HPP
#define ASCENSION_CHARACTER_ITERATOR_HPP
#include <ascension/corelib/string-piece.hpp>
#include <ascension/corelib/text/utf.hpp>
#include <iterator>
#include <stdexcept>
#include <boost/iterator/iterator_facade.hpp>

#if ASCENSION_UNICODE_VERSION > 0x0510
#error These class definitions and implementations are based on old version of Unicode.
#endif
/// Tracking revision number of UAX #14 ("Line Breaking Properties")
#define ASCENSION_UAX14_REVISION_NUMBER 19	// 2006-05-23
/// Tracking revision number of UAX #29 ("Text Boundary")
#define ASCENSION_UAX29_REVISION_NUMBER 11	// 2006-10-12

namespace ascension {
	namespace text {

		// documentation is character-iterator.cpp
		class CharacterIterator {
		public:
			static const CodePoint DONE;
		public:
			/// Destructor.
			virtual ~CharacterIterator() BOOST_NOEXCEPT {}

			/// Returns the position in the character sequence.
			std::ptrdiff_t offset() const BOOST_NOEXCEPT {return offset_;}

			/// @name Assignment and Copy
			/// @{
			/**
			 * Assigns the other iterator.
			 * @param other The source iterator of the assignment
			 * @return This iterator
			 */
			CharacterIterator& assign(const CharacterIterator& other) {
				verifyOther(other);
				doAssign(other);
				return *this;
			}
			/**
			 * Creates a copy of the iterator.
			 * @return The copied iterator
			 */
			std::unique_ptr<CharacterIterator> clone() const {
				std::unique_ptr<CharacterIterator> p(doClone());
				if(p.get() != nullptr)
					p->offset_ = offset_;
				return p;
			}
			/// @}

			/// @name Comparisons
			/// @{
			/**
			 * Returns @c true if the iterator equals @a other.
			 * @param other The other iterator
			 * @return @c true if this iterator equals to @a other
			 */
			bool equals(const CharacterIterator& other) const {
				verifyOther(other);
				return doEquals(other);
			}
			/**
			 * Returns @c true if the iterator is less than @a other.
			 * @param other The other iterator
			 * @return @c true if this iterator is less than @a other
			 */
			bool less(const CharacterIterator& other) const {
				verifyOther(other);
				return doLess(other);
			}
			/// @}

			/// @name Traverses
			/// @{
			/**
			 * Moves to the beginning of the character sequence.
			 * @return This iterator
			 */
			CharacterIterator& first() {
				doFirst();
				offset_ = 0;
				return *this;
			}
			/**
			 * Moves to the end of the character sequence.
			 * @return This iterator
			 */
			CharacterIterator& last() {
				doLast();
				offset_ = 0;
				return *this;
			}
			/**
			 * Moves to the next code unit.
			 * @return This iterator
			 */
			CharacterIterator& next() {
				doNext();
				++offset_;
				return *this;
			}
			/**
			 * Moves to the previous code unit.
			 * @return This iterator
			 */
			CharacterIterator& previous() {
				doPrevious();
				--offset_;
				return *this;
			}
			/// @}

			/// @name Virtual Methods the Concrete Class Should Implement
			/// @{
		public:
			/// Returns the current code point value.
			virtual CodePoint current() const BOOST_NOEXCEPT = 0;
			/// Returns @c true if the iterator is not last.
			virtual bool hasNext() const BOOST_NOEXCEPT = 0;
			/// Returns @c true if the iterator is not first.
			virtual bool hasPrevious() const BOOST_NOEXCEPT = 0;

		private:
			/// Called by @c #assign method.
			virtual void doAssign(const CharacterIterator& other) = 0;
			/// Called by @c #clone method.
			virtual std::unique_ptr<CharacterIterator> doClone() const = 0;
			/// Called by @c #equals method.
			virtual bool doEquals(const CharacterIterator& other) const = 0;
			/// Called by @c #first method.
			virtual void doFirst() = 0;
			/// Called by @c #last method.
			virtual void doLast() = 0;
			/// Called by @c #less method.
			virtual bool doLess(const CharacterIterator& other) const = 0;
			/// Called by @c #next method.
			virtual void doNext() = 0;
			/// Called by @c #previous method.
			virtual void doPrevious() = 0;
			/// @}

		protected:
			/// Identifies a concrete type of the derived class for relational operations.
			struct ConcreteTypeTag {};
			/// Protected constructor.
			explicit CharacterIterator(const ConcreteTypeTag& classID) BOOST_NOEXCEPT
				: offset_(0), classID_(&classID) {}
			/// Protected copy-constructor.
			CharacterIterator(const CharacterIterator& other) BOOST_NOEXCEPT
				: offset_(other.offset_), classID_(other.classID_) {}
			/// Protected assignment operator.
			CharacterIterator& operator=(const CharacterIterator& other) {
				offset_ = other.offset_; return *this;
			}
		private:
			void verifyOther(const CharacterIterator& other) const {
				if(classID_ != other.classID_)
					throw std::invalid_argument("type mismatch.");
			}
			std::ptrdiff_t offset_;
			const ConcreteTypeTag* const classID_;
		};

		/**
		 * Implementation of @c CharacterIterator for C string or @c String.
		 * @note This class is not intended to be subclassed.
		 */
		class StringCharacterIterator :
			public CharacterIterator,
			public boost::iterators::iterator_facade<
				StringCharacterIterator, CodePoint,
				boost::iterators::bidirectional_traversal_tag, const CodePoint, std::ptrdiff_t
			> {
		public:
			StringCharacterIterator() BOOST_NOEXCEPT;
			StringCharacterIterator(const StringPiece& text);
			StringCharacterIterator(const StringPiece& text, StringPiece::const_iterator start);
			StringCharacterIterator(const String& text);
			StringCharacterIterator(const String& text, String::const_iterator start);
			StringCharacterIterator(const StringCharacterIterator& other) BOOST_NOEXCEPT;

			// attributes
			/// Returns the beginning position.
			StringPiece::const_iterator beginning() const BOOST_NOEXCEPT {return range_.cend();}
			/// Returns the end position.
			StringPiece::const_iterator end() const BOOST_NOEXCEPT {return range_.cend();}
			/// Returns the current position.
			StringPiece::const_iterator tell() const BOOST_NOEXCEPT {return current_;}

			// CharacterIterator
			/// @see CharacterIterator#current
			CodePoint current() const BOOST_NOEXCEPT {
				return (tell() != end()) ? utf::checkedDecodeFirst(tell(), end()) : DONE;
			}
			/// @see CharacterIterator#hasNext
			bool hasNext() const BOOST_NOEXCEPT {return tell() != end();}
			/// @see CharacterIterator#hasPrevious
			bool hasPrevious() const BOOST_NOEXCEPT {return tell() != end();}

		private:
			void doAssign(const CharacterIterator& other);
			std::unique_ptr<CharacterIterator> doClone() const;
			void doFirst();
			bool doEquals(const CharacterIterator& other) const;
			void doLast();
			bool doLess(const CharacterIterator& other) const;
			void doNext();
			void doPrevious();
			// boost.iterator_facade
			friend class boost::iterators::iterator_core_access;
			CodePoint dereference() const {return current();}
			void decrement() {previous();}
			bool equal(const StringCharacterIterator& other) const {return equals(other);}
			void increment() {next();}
		private:
			static const ConcreteTypeTag CONCRETE_TYPE_TAG_;
			StringPiece::const_iterator current_;
			StringPiece range_;
		};

	}
} // namespace ascension.text

#endif // !ASCENSION_CHARACTER_ITERATOR_HPP
