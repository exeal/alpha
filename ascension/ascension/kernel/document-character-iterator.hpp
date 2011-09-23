/**
 * @file document-character-iterator.hpp
 * @author exeal
 * @date 2003-2006 (was EditDoc.h)
 * @date 2006-2010 (was document.hpp)
 * @date 2010-11-06 separated from document.hpp
 */

#ifndef ASCENSION_DOCUMENT_CHARACTER_ITERATOR_HPP
#define ASCENSION_DOCUMENT_CHARACTER_ITERATOR_HPP

#include <ascension/kernel/document.hpp>
#include <ascension/corelib/text/character-iterator.hpp>			// text.CharacterIterator
#include <ascension/corelib/standard-iterator-adapter.hpp>	// detail.IteratorAdapter

namespace ascension {

	namespace kernel {

		class DocumentCharacterIterator :
			public text::CharacterIterator,
			public detail::IteratorAdapter<
				DocumentCharacterIterator,
				std::iterator<std::bidirectional_iterator_tag,
				CodePoint, std::ptrdiff_t, const CodePoint*, const CodePoint>
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
			std::auto_ptr<CharacterIterator> doClone() const;
			void doFirst();
			void doLast();
			bool doEquals(const CharacterIterator& other) const;
			bool doLess(const CharacterIterator& other) const;
			void doNext();
			void doPrevious();
		private:
			static const ConcreteTypeTag CONCRETE_TYPE_TAG_;
			const Document* document_;
			Region region_;
			const String* line_;
			Position p_;
		};


// inline implementation ////////////////////////////////////////////////////

/// Returns the document.
inline const Document* DocumentCharacterIterator::document() const /*throw()*/ {return document_;}

/// @see text#CharacterIterator#hasNext
inline bool DocumentCharacterIterator::hasNext() const /*throw()*/ {return p_ != region_.second;}

/// @see text#CharacterIterator#hasPrevious
inline bool DocumentCharacterIterator::hasPrevious() const /*throw()*/ {return p_ != region_.first;}

/// Returns the line.
inline const String& DocumentCharacterIterator::line() const /*throw()*/ {return *line_;}

/// Returns the iteration region.
inline const Region& DocumentCharacterIterator::region() const /*throw()*/ {return region_;}

/**
 * Moves to the specified position.
 * @param to the position. if this is outside of the iteration region, the start/end of the region will be used
 */
inline DocumentCharacterIterator& DocumentCharacterIterator::seek(const Position& to) {
	line_ = &document_->line((p_ = std::max(std::min(to, region_.second), region_.first)).line); return *this;}

/// Returns the document position the iterator addresses.
inline const Position& DocumentCharacterIterator::tell() const /*throw()*/ {return p_;}

	}
} // namespace ascension.kernel

#endif // !ASCENSION_DOCUMENT_CHARACTER_ITERATOR_HPP
