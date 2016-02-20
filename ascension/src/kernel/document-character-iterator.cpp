/**
 * @file document.cpp
 * @author exeal
 * @date 2003-2006 (was EditDoc.h)
 * @date 2006-2014 was document.cpp
 * @date 2014-11-23 Separated from document.cpp
 */

#include <ascension/kernel/document.hpp>
#include <ascension/kernel/document-character-iterator.hpp>


namespace ascension {
	namespace kernel {
		/**
		 * @class ascension::kernel::DocumentCharacterIterator
		 * Bidirectional iterator scans characters in the specified document.
		 *
		 * @c #current implementation of this class returns a character at which the iterator addresses. A returned
		 * character is as a UTF-32 code unit (not UTF-16). In the following cases, returns a special value depending
		 * on the context:
		 *
		 * - @c CharacterIterator#DONE at the end of the region of the iterator
		 * - @c LINE_SEPARATOR at the end of the line
		 * - a raw code unit value at any unpaired surrogate
		 *
		 * This class can't detect any change of the document. When the document changed, the existing iterators may be
		 * invalid.
		 *
		 * @note This class is not intended to be subclassed.
		 */
		
		/// Default constructor makes an invalid iterator object.
		DocumentCharacterIterator::DocumentCharacterIterator() BOOST_NOEXCEPT : document_(nullptr) {
		}

		/**
		 * Constructor. The iteration region is the accessible area of the document.
		 * @param document The document to iterate
		 * @param position The position at which the iteration starts
		 * @throw BadPositionException @a position is outside of the accessible area of the document
		 */
		DocumentCharacterIterator::DocumentCharacterIterator(const Document& document, const Position& position)
				: document_(&document), region_(document.region()), position_(position) {
			if(!region_.includes(position_))
				throw BadPositionException(position_);
		}

		/**
		 * Constructor. The iteration is started at @a region.beginning().
		 * @param document The document to iterate
		 * @param region The region to iterate
		 * @throw BadRegionException @a region intersects outside of the document
		 */
		DocumentCharacterIterator::DocumentCharacterIterator(const Document& document, const Region& region)
				: document_(&document), region_(region), position_(region.beginning()) {
			region_.normalize();
			if(!document.region().encompasses(region_))
				throw BadRegionException(region_);
		}
		
		/**
		 * Constructor.
		 * @param document The document to iterate
		 * @param region The region to iterate
		 * @param position The position at which the iteration starts
		 * @throw BadRegionException @a region intersects outside of the document
		 * @throw BadPositionException @a position is outside of @a region
		 */
		DocumentCharacterIterator::DocumentCharacterIterator(const Document& document, const Region& region, const Position& position)
				: document_(&document), region_(region), position_(position) {
			region_.normalize();
			if(!document.region().encompasses(region_))
				throw BadRegionException(region_);
			else if(!region_.includes(position_))
				throw BadPositionException(position_);
		}

		/// Copy-constructor.
		DocumentCharacterIterator::DocumentCharacterIterator(const DocumentCharacterIterator& other) BOOST_NOEXCEPT
				: document_(other.document_), region_(other.region_), position_(other.position_) {
		}

		void DocumentCharacterIterator::decrement() {
			if(!hasPrevious())
//				throw NoSuchElementException("the iterator is at the first.");
				return;

			if(kernel::offsetInLine(tell()) == 0) {
				--position_.line;
				position_.offsetInLine = line().length();
			} else if(--position_.offsetInLine > 0) {
				const String& lineString = line();
				if(text::surrogates::isLowSurrogate(lineString[kernel::offsetInLine(tell())])
						&& text::surrogates::isHighSurrogate(lineString[kernel::offsetInLine(tell()) - 1]))
					--position_.offsetInLine;
			}
			--offset_;
		}

		CodePoint DocumentCharacterIterator::dereference() const BOOST_NOEXCEPT {
			assert(document_ != nullptr && tell() != region().second);
			const String& lineString = line();
			if(kernel::offsetInLine(tell()) == lineString.length())
				return text::LINE_SEPARATOR;
			else
				return text::utf::decodeFirst(std::begin(lineString) + kernel::offsetInLine(tell()), std::end(lineString));
		}
#if 0
		/// @see text#CharacterIterator#doLess
		bool DocumentCharacterIterator::doLess(const CharacterIterator& other) const {
			return tell() < static_cast<const DocumentCharacterIterator&>(other).tell();
		}
#endif
		bool DocumentCharacterIterator::equal(const DocumentCharacterIterator& other) const {
			if(document_ != other.document_)
				return false;
			return document_ == nullptr || tell() == other.tell();
		}

		void DocumentCharacterIterator::increment() {
			if(!hasNext())
//				throw NoSuchElementException("the iterator is at the last.");
				return;

			const String& lineString = line();
			if(kernel::offsetInLine(tell()) == lineString.length()) {
				++position_.line;
				position_.offsetInLine = 0;
			} else if(++position_.offsetInLine < lineString.length()
					&& text::surrogates::isLowSurrogate(lineString[kernel::offsetInLine(tell())])
					&& text::surrogates::isHighSurrogate(lineString[kernel::offsetInLine(tell()) - 1]))
				++position_.offsetInLine;
			++offset_;
		}
		
		/// Returns the line text string.
		const String& DocumentCharacterIterator::line() const BOOST_NOEXCEPT {
			return document().lineString(kernel::line(tell()));
		}

		/**
		 * Sets the region of the iterator. The current position will adjusted.
		 * @param newRegion The new region to set
		 * @throw BadRegionException @a newRegion intersects outside of the document
		 */
		void DocumentCharacterIterator::setRegion(const Region& newRegion) {
			const Position e(document_->region().second);
			if(newRegion.first > e || newRegion.second > e)
				throw BadRegionException(newRegion);
			if(!(region_ = newRegion).includes(tell()))
				seek(tell());
		}
	}
}
