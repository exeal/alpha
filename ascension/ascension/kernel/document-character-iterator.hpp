/**
 * @file document-character-iterator.hpp
 * @author exeal
 * @date 2003-2006 (was EditDoc.h)
 * @date 2006-2010 (was document.hpp)
 * @date 2010-11-06 separated from document.hpp
 * @date 2012-2014
 */

#ifndef ASCENSION_DOCUMENT_CHARACTER_ITERATOR_HPP
#define ASCENSION_DOCUMENT_CHARACTER_ITERATOR_HPP

#include <ascension/corelib/text/character-iterator.hpp>	// text.CharacterIterator
#include <ascension/kernel/position.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/utility/value_init.hpp>

namespace ascension {
	namespace kernel {
		class Document;

		class DocumentCharacterIterator : public boost::iterators::iterator_facade<
			DocumentCharacterIterator, CodePoint,
			boost::iterators::bidirectional_traversal_tag, const CodePoint, std::ptrdiff_t
		> {
		public:
			DocumentCharacterIterator() BOOST_NOEXCEPT;
			DocumentCharacterIterator(const Document& document, const Position& position);
			DocumentCharacterIterator(const Document& document, const Region& region);
			DocumentCharacterIterator(const Document& document, const Region& region, const Position& position);
			DocumentCharacterIterator(const DocumentCharacterIterator& other) BOOST_NOEXCEPT;

			/// @name Position
			/// @{
			DocumentCharacterIterator& seek(const Position& to);
			const Position& tell() const BOOST_NOEXCEPT;
			/// @}

			/// @name Other Document-Related Attributes
			/// @{
			const Document& document() const BOOST_NOEXCEPT;
			const String& line() const BOOST_NOEXCEPT;
			const Region& region() const BOOST_NOEXCEPT;
			void setRegion(const Region& newRegion);
			/// @}

			/// @name CharacterIterator Concepts
			/// @{
			bool hasNext() const BOOST_NOEXCEPT;
			bool hasPrevious() const BOOST_NOEXCEPT;
			std::ptrdiff_t offset() const BOOST_NOEXCEPT;
			/// @}

		private:
			// boost.iterator_facade
			friend class boost::iterators::iterator_core_access;
			CodePoint dereference() const;
			void decrement();
			bool equal(const DocumentCharacterIterator& other) const;
			void increment();
		private:
			const Document* document_;
			Region region_;	// should be normalized
			Position position_;
			boost::value_initialized<std::ptrdiff_t> offset_;
		};

		
		/// Returns the document.
		inline const Document& DocumentCharacterIterator::document() const BOOST_NOEXCEPT {
			return *document_;
		}
		
		/// Returns @c true if the iterator has the next character.
		inline bool DocumentCharacterIterator::hasNext() const BOOST_NOEXCEPT {
			return tell() < region().second;
		}
		
		/// Returns @c true if the iterator has the previous character.
		inline bool DocumentCharacterIterator::hasPrevious() const BOOST_NOEXCEPT {
			return tell() > region().first;
		}

		/// Returns the relative position from where the iterator started.
		inline std::ptrdiff_t DocumentCharacterIterator::offset() const BOOST_NOEXCEPT {
			return boost::get(offset_);
		}
		
		/// Returns the iteration region.
		inline const Region& DocumentCharacterIterator::region() const BOOST_NOEXCEPT {
			return region_;
		}
		
		/**
		 * Moves to the specified position.
		 * @param to The position. If this is outside of the iteration region, the start/end of the region will be used
		 * @return This iterator
		 */
		inline DocumentCharacterIterator& DocumentCharacterIterator::seek(const Position& to) {
			position_ = std::max(std::min(to, region().second), region().first);
			return *this;
		}
		
		/// Returns the document position the iterator addresses.
		inline const Position& DocumentCharacterIterator::tell() const BOOST_NOEXCEPT {
			return position_;
		}
	}
} // namespace ascension.kernel

#endif // !ASCENSION_DOCUMENT_CHARACTER_ITERATOR_HPP
