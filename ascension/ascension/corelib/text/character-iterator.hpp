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
#include <boost/iterator/iterator_facade.hpp>
#if 1
#	include <boost/mpl/vector.hpp>
#	include <boost/iterator/iterator_adaptor.hpp>	// place here because of C2874 at Boost 1.57
#	include <boost/type_erasure/iterator.hpp>
#	include <boost/type_erasure/member.hpp>
#else
#	include <boost/core/null_deleter.hpp>
#	include <boost/optional.hpp>
#endif
#include <iterator>
#include <memory>
#include <stdexcept>

#if 1
// hasNext method takes no argument and returns true if the iterator has the next element
BOOST_TYPE_ERASURE_MEMBER((ascension)(text)(detail)(hasHasNext), hasNext, 0)
// hasPrevious method takes no argument and returns true if the iterator has the previous element
BOOST_TYPE_ERASURE_MEMBER((ascension)(text)(detail)(hasHasPrevious), hasPrevious, 0)
// hasNext method takes no argument returns the position of the iterator in the target sequence
BOOST_TYPE_ERASURE_MEMBER((ascension)(text)(detail)(hasOffset), offset, 0)
#endif

namespace ascension {
	namespace text {
#if 1
		namespace detail {
			typedef boost::mpl::vector<
				hasHasNext<bool(void), const boost::type_erasure::_self>,
				hasHasPrevious<bool(void), const boost::type_erasure::_self>,
				hasOffset<std::ptrdiff_t(void), const boost::type_erasure::_self>,
				boost::type_erasure::bidirectional_iterator<boost::type_erasure::_self, const CodePoint>,
				boost::type_erasure::typeid_<>,
				boost::type_erasure::relaxed
			> CharacterIteratorConcepts;
			typedef boost::type_erasure::any<CharacterIteratorConcepts> CharacterIterator;
		}
#else
		// documentation is character-iterator.cpp
		class CharacterIterator {
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
			/// Returns the current code point value, or @c boost#none if the iterator is done.
			virtual boost::optional<CodePoint> current() const BOOST_NOEXCEPT = 0;
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
			// boost.iterator_facade
			friend class boost::iterators::iterator_core_access;
			CodePoint dereference() const {return boost::get(current());}
			void decrement() {previous();}
			bool equal(const CharacterIterator& other) const {return equals(other);}
			void increment() {next();}
			std::ptrdiff_t offset_;
			const ConcreteTypeTag* const classID_;
		};

		class CharacterIteratorFacade :
			public boost::iterators::iterator_facade<
				CharacterIteratorFacade, CodePoint,
				boost::iterators::bidirectional_traversal_tag, const CodePoint, std::ptrdiff_t
			> {
		public:
			explicit CharacterIteratorFacade(CharacterIterator& base) : base_(&base, boost::null_deleter()) BOOST_NOEXCEPT {}
			explicit CharacterIteratorFacade(std::shared_ptr<CharacterIterator> base) : base_(base) BOOST_NOEXCEPT {}
			explicit CharacterIteratorFacade(std::unique_ptr<CharacterIterator> base) : base_(std::move(base)) BOOST_NOEXCEPT {}
			explicit CharacterIteratorFacade(const std::weak_ptr<CharacterIterator>& base) : base_(base) BOOST_NOEXCEPT {}
			CharacterIterator& base() BOOST_NOEXCEPT {return *base_;}
			const CharacterIterator& base() const BOOST_NOEXCEPT {return *base_;}

		private:
			// boost.iterator_facade
			friend class boost::iterators::iterator_core_access;
			CodePoint dereference() const {return boost::get(base().current());}
			void decrement() {base().previous();}
			bool equal(const CharacterIteratorFacade& other) const {return base().equals(other.base());}
			void increment() {base().next();}
		private:
			std::shared_ptr<CharacterIterator> base_;
		};
#endif

	}
} // namespace ascension.text

#endif // !ASCENSION_CHARACTER_ITERATOR_HPP
