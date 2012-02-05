/**
 * @file document-character-iterator.hpp
 * @author exeal
 * @date 2003-2006 (was EditDoc.h)
 * @date 2006-2010 (was document.hpp)
 * @date 2010-11-06 separated from document.hpp
 * @date 2012
 */

#ifndef ASCENSION_DOCUMENT_CHARACTER_ITERATOR_HPP
#define ASCENSION_DOCUMENT_CHARACTER_ITERATOR_HPP

#include <ascension/kernel/document.hpp>
#include <ascension/corelib/text/character-iterator.hpp>	// text.CharacterIterator
#include <boost/iterator/iterator_facade.hpp>

namespace ascension {

	namespace kernel {

		class DocumentCharacterIterator :
			public text::CharacterIterator,
			public boost::iterator_facade<
				DocumentCharacterIterator, CodePoint,
				std::bidirectional_iterator_tag, const CodePoint, std::ptrdiff_t
			> {
		public:
			// constructors
			DocumentCharacterIterator() /*throw()*/;
			DocumentCharacterIterator(const Document& document, const Position& position);
			DocumentCharacterIterator(const Document& document, const Region& region);
			DocumentCharacterIterator(const Document& document, const Region& region, const Position& position);
			DocumentCharacterIterator(const DocumentCharacterIterator& other) /*throw()*/;
			// attributes
			const Document* document() const /*throw()*/;
			const String& line() const /*throw()*/;
			const Region& region() const /*throw()*/;
			void setRegion(const Region& newRegion);
			const Position& tell() const /*throw()*/;
			// operation
			DocumentCharacterIterator& seek(const Position& to);
			// CharacterIterator
			CodePoint current() const /*throw()*/;
			bool hasNext() const /*throw()*/;
			bool hasPrevious() const /*throw()*/;
		private:
			void doAssign(const CharacterIterator& other);
			std::unique_ptr<CharacterIterator> doClone() const;
			void doFirst();
			void doLast();
			bool doEquals(const CharacterIterator& other) const;
			bool doLess(const CharacterIterator& other) const;
			void doNext();
			void doPrevious();
			// boost.iterator_facade
			friend class boost::iterator_core_access;
			CodePoint dereference() const {return current();}
			void decrement() {previous();}
			bool equal(const DocumentCharacterIterator& other) const {return equals(other);}
			void increment() {next();}
		private:
			static const ConcreteTypeTag CONCRETE_TYPE_TAG_;
			const Document* document_;
			Region region_;
			const String* line_;
			Position p_;
		};


		// inline implementation //////////////////////////////////////////////////////////////////
		
		/// Returns the document.
		inline const Document* DocumentCharacterIterator::document() const /*throw()*/ {
			return document_;
		}
		
		/// @see text#CharacterIterator#hasNext
		inline bool DocumentCharacterIterator::hasNext() const /*throw()*/ {
			return tell() != region().second;
		}
		
		/// @see text#CharacterIterator#hasPrevious
		inline bool DocumentCharacterIterator::hasPrevious() const /*throw()*/ {
			return tell() != region().first;
		}
		
		/// Returns the line.
		inline const String& DocumentCharacterIterator::line() const /*throw()*/ {
			return *line_;
		}
		
		/// Returns the iteration region.
		inline const Region& DocumentCharacterIterator::region() const /*throw()*/ {
			return region_;
		}
		
		/**
		 * Moves to the specified position.
		 * @param to The position. if this is outside of the iteration region, the start/end of the
		 *           region will be used
		 * @return This iterator
		 */
		inline DocumentCharacterIterator& DocumentCharacterIterator::seek(const Position& to) {
			line_ = &document_->line((p_ = std::max(std::min(to, region().second), region().first)).line);
			return *this;
		}
		
		/// Returns the document position the iterator addresses.
		inline const Position& DocumentCharacterIterator::tell() const /*throw()*/ {
			return p_;
		}

	}
} // namespace ascension.kernel

#endif // !ASCENSION_DOCUMENT_CHARACTER_ITERATOR_HPP
