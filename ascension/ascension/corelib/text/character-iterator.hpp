/**
 * @file character-iterator.hpp
 * Defines iterator classes traverse Unicode character sequence.
 * @author exeal
 * @date 2005-2010 (was unicode.hpp)
 * @date 2010-2011
 * @see character.hpp
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

		class CharacterIterator {
		public:
			static const CodePoint DONE;
		public:
			/// Destructor.
			virtual ~CharacterIterator() /*throw()*/ {}
			// attributes
			/// Returns @c true if the iterator equals @a other.
			bool equals(const CharacterIterator& other) const {
				verifyOther(other);
				return doEquals(other);
			}
			/// Returns c true if the iterator is less than @a other.
			bool less(const CharacterIterator& other) const {
				verifyOther(other);
				return doLess(other);
			}
			/// Returns the position in the character sequence.
			std::ptrdiff_t offset() const /*throw()*/ {return offset_;}

			// operations
			/// Assigns the other iterator.
			CharacterIterator& assign(const CharacterIterator& other) {
				verifyOther(other);
				doAssign(other);
				return *this;
			}
			/// Creates a copy of the iterator.
			std::unique_ptr<CharacterIterator> clone() const {
				std::unique_ptr<CharacterIterator> p(doClone());
				if(p.get() != nullptr)
					p->offset_ = offset_;
				return p;
			}
			/// Moves to the start of the character sequence.
			CharacterIterator& first() {
				doFirst();
				offset_ = 0;
				return *this;
			}
			/// Moves to the end of the character sequence.
			CharacterIterator& last() {
				doLast();
				offset_ = 0;
				return *this;
			}
			/// Moves to the next code unit.
			CharacterIterator& next() {
				doNext();
				++offset_;
				return *this;
			}
			/// Moves to the previous code unit.
			CharacterIterator& previous() {
				doPrevious();
				--offset_;
				return *this;
			}

			// virtual methods the concrete class should implement
		public:
			/// Returns the current code point value.
			virtual CodePoint current() const /*throw()*/ = 0;
			/// Returns @c true if the iterator is not last.
			virtual bool hasNext() const /*throw()*/ = 0;
			/// Returns @c true if the iterator is not first.
			virtual bool hasPrevious() const /*throw()*/ = 0;
		protected:
			/// Identifies a concrete type of the derived class for relational operations.
			struct ConcreteTypeTag {};
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

		protected:
			/// Protected constructor.
			explicit CharacterIterator(const ConcreteTypeTag& classID) /*throw()*/
				: offset_(0), classID_(&classID) {}
			/// Protected copy-constructor.
			CharacterIterator(const CharacterIterator& other) /*throw()*/
				: offset_(other.offset_), classID_(other.classID_) {}
			/// Protected assignment operator.
			CharacterIterator& operator=(const CharacterIterator& other) {
				offset_ = other.offset_; return *this;
			}
		private:
			void verifyOther(const CharacterIterator& other) const {
				if(classID_ != other.classID_) throw std::invalid_argument("type mismatch.");}
			std::ptrdiff_t offset_;
			const ConcreteTypeTag* const classID_;
		};

		/**
		 * Implementation of @c CharacterIterator for C string or @c String.
		 * @note This class is not intended to be subclassed.
		 */
		class StringCharacterIterator :
			public CharacterIterator,
			public boost::iterator_facade<
				StringCharacterIterator, CodePoint,
				std::bidirectional_iterator_tag, const CodePoint, std::ptrdiff_t
			> {
		public:
			StringCharacterIterator() /*throw()*/;
			StringCharacterIterator(const StringPiece& text);
			StringCharacterIterator(const Range<const Char*>& text, const Char* start);
			StringCharacterIterator(const String& s, String::const_iterator start);
			StringCharacterIterator(const StringCharacterIterator& other) /*throw()*/;

			// attributes
			/// Returns the beginning position.
			const Char* beginning() const /*throw()*/ {return first_;}
			/// Returns the end position.
			const Char* end() const /*throw()*/ {return last_;}
			/// Returns the current position.
			const Char* tell() const /*throw()*/ {return current_;}

			// CharacterIterator
			/// @see CharacterIterator#current
			CodePoint current() const /*throw()*/ {
				return (current_ != last_) ? utf::checkedDecodeFirst(current_, last_) : DONE;
			}
			/// @see CharacterIterator#hasNext
			bool hasNext() const /*throw()*/ {return current_ != last_;}
			/// @see CharacterIterator#hasPrevious
			bool hasPrevious() const /*throw()*/ {return current_ != first_;}
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
			friend class boost::iterator_core_access;
			CodePoint dereference() const {return current();}
			void decrement() {previous();}
			bool equal(const StringCharacterIterator& other) const {return equals(other);}
			void increment() {next();}
		private:
			static const ConcreteTypeTag CONCRETE_TYPE_TAG_;
			const Char* current_;
			const Char* first_;
			const Char* last_;
		};

	}
} // namespace ascension.text

#endif // !ASCENSION_CHARACTER_ITERATOR_HPP
