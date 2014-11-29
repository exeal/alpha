/**
 * @file string-character-iterator.hpp
 * @author exeal
 * @date 2005-2010 was unicode.hpp
 * @date 2010-2014 was character-iterator.hpp
 * @date 2014-11-22 Separated from character-iterator.hpp
 * @see utf-iterator.hpp
 */

#ifndef ASCENSION_STRING_CHARACTER_ITERATOR_HPP
#define ASCENSION_STRING_CHARACTER_ITERATOR_HPP
#include <ascension/corelib/string-piece.hpp>
#include <ascension/corelib/text/utf-iterator.hpp>
#include <boost/iterator/iterator_facade.hpp>

namespace ascension {
	namespace text {
		/**
		 * Implementation of @c CharacterIterator for C string or @c String.
		 * @note This class is not intended to be subclassed.
		 * @note This class satisfies @c detail#CharacterIteratorConcepts concepts.
		 */
		class StringCharacterIterator : public boost::iterators::iterator_facade<
			StringCharacterIterator, CodePoint,
			boost::iterators::bidirectional_traversal_tag, const CodePoint, std::ptrdiff_t
		> {
		public:
			/// Default constructor initializes nothing.
			StringCharacterIterator() BOOST_NOEXCEPT {}
			StringCharacterIterator(const StringPiece& text) : impl_(text.cbegin(), text.cend()), offset_(0) {}
			StringCharacterIterator(const StringPiece& text, StringPiece::const_iterator start) : impl_(text.cbegin(), text.cend(), start), offset_(0) {
				if(tell() < beginning() || tell() > end())
					throw std::invalid_argument("invalid input.");
			}
			StringCharacterIterator(const String& text) : impl_(text.data(), text.data() + text.length()), offset_(0) {}
			StringCharacterIterator(const String& text, String::const_iterator start) :
					impl_(text.data(), text.data() + text.length(), text.data() + (start - std::begin(text))), offset_(0) {
				if(tell() < beginning() || tell() > end())
					throw std::invalid_argument("invalid input.");
			}
			/// Copy constructor.
			StringCharacterIterator(const StringCharacterIterator& other) BOOST_NOEXCEPT : impl_(other.impl_), offset_(other.offset_) {}

			/// Returns the beginning position.
			StringPiece::const_iterator beginning() const BOOST_NOEXCEPT {return impl_.first();}
			/// Returns the end position.
			StringPiece::const_iterator end() const BOOST_NOEXCEPT {return impl_.last();}
			/// Returns the current position.
			StringPiece::const_iterator tell() const BOOST_NOEXCEPT {return impl_.tell();}

			/**
			 * @name CharacterIterator Concepts
			 * @{
			 */
			/// Returns @c true if the iterator has the next element.
			bool hasNext() const BOOST_NOEXCEPT {return tell() < impl_.last();}
			/// Returns @c true if the iterator has the previous element.
			bool hasPrevious() const BOOST_NOEXCEPT {return tell() > impl_.first();}
			/// Returns the offset of the iterator from the beginning of the target sequence.
			std::ptrdiff_t offset() const BOOST_NOEXCEPT {return offset_;}
			/// @}

		private:
			// boost.iterator_facade
			friend class boost::iterators::iterator_core_access;
			CodePoint dereference() const {
				return utf::checkedDecodeFirst(tell(), end());	// this may throw
			}
			void decrement() {
				if(!hasPrevious())
//					throw std::out_of_range("the iterator is the first.");
					return;
				--impl_;
				--offset_;
			}
			bool equal(const StringCharacterIterator& other) const {
				return impl_ == other.impl_;
			}
			void increment() {
				if(tell() == end())
//					throw std::out_of_range("the iterator is the last.");
					return;
				++impl_;
				++offset_;
			}
		private:
			utf::CharacterDecodeIterator<StringPiece::const_iterator> impl_;
			std::ptrdiff_t offset_;
		};
	}
} // namespace ascension.text

#endif // !ASCENSION_STRING_CHARACTER_ITERATOR_HPP
