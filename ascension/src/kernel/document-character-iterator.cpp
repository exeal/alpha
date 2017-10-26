/**
 * @file document.cpp
 * @author exeal
 * @date 2003-2006 (was EditDoc.h)
 * @date 2006-2014 was document.cpp
 * @date 2014-11-23 Separated from document.cpp
 */

#include <ascension/corelib/numeric-range-algorithm/clamp.hpp>
#include <ascension/corelib/numeric-range-algorithm/encompasses.hpp>
#include <ascension/kernel/document.hpp>
#include <ascension/kernel/document-character-iterator.hpp>
#include <ascension/kernel/point-proxy.hpp>

namespace ascension {
	namespace kernel {
		/**
		 * @class ascension::kernel::DocumentCharacterIterator
		 * Bidirectional iterator scans characters in the specified document.
		 *
		 * This class satisfies @c text#CharacterIterator concept.
		 *
		 * This class can't detect any change of the document. When the document changed, the existing iterators may be
		 * invalid.
		 *
		 * @note This class is not intended to be subclassed.
		 */
		
		/**
		 * Default constructor makes an invalid iterator object.
		 * @post *this == DocumentCharacterIterator()
		 */
		DocumentCharacterIterator::DocumentCharacterIterator() BOOST_NOEXCEPT : document_(nullptr) {
		}

		/**
		 * Constructor. The iteration area is the @c #region of the document.
		 * @param document The document to iterate
		 * @param position The position at which the iteration starts
		 * @throw BadPositionException @a position is outside of the @c #region of the @a document
		 * @post &amp;document() == &amp;document
		 * @post region() == document.region()
		 * @post tell() == position
		 * @post offset() == 0
		 */
		DocumentCharacterIterator::DocumentCharacterIterator(const Document& document, const Position& position)
				: document_(&document), region_(document.region()), position_(position) {
			if(!encompasses(region_, position_))
				throw BadPositionException(position_);
		}

		/**
		 * Constructor. The iteration area is the @c #region of the document.
		 * @param point The document to iterate and the position at which the iteration starts
		 * @throw BadPositionException The given position is outside of the @c #region of the given document
		 * @post &amp;document() == &amp;document(point)
		 * @post region() == document(point).region()
		 * @post tell() == position(point)
		 * @post offset() == 0
		 */
		DocumentCharacterIterator::DocumentCharacterIterator(const locations::PointProxy& point)
				: document_(&kernel::document(point)), region_(kernel::document(point).region()), position_(position(point)) {
			if(!encompasses(region_, position_))
				throw BadPositionException(position_);
		}

		/**
		 * Constructor. The iteration is started at `*boost#const_begin(region)`.
		 * @param document The document to iterate
		 * @param region The region to iterate
		 * @throw BadRegionException @a region intersects outside of the document
		 * @post &amp;document() == &amp;document
		 * @post region() == region
		 * @post tell() == *boost::const_begin(region)
		 * @post offset() == 0
		 */
		DocumentCharacterIterator::DocumentCharacterIterator(const Document& document, const Region& region)
				: document_(&document), region_(region), position_(*boost::const_begin(region)) {
			if(!encompasses(document.region(), region_))
				throw BadRegionException(region_);
		}
		
		/**
		 * Constructor.
		 * @param document The document to iterate
		 * @param region The region to iterate
		 * @param position The position at which the iteration starts
		 * @throw BadRegionException @a region intersects outside of the document
		 * @throw BadPositionException @a position is outside of @a region
		 * @post &amp;document() == &amp;document
		 * @post region() == region
		 * @post tell() == position
		 * @post offset() == 0
		 */
		DocumentCharacterIterator::DocumentCharacterIterator(const Document& document, const Region& region, const Position& position)
				: document_(&document), region_(region), position_(position) {
			if(!encompasses(document.region(), region_))
				throw BadRegionException(region_);
			else if(!encompasses(region_, position_))
				throw BadPositionException(position_);
		}

		/**
		 * Constructor.
		 * @param point The document to iterate and the position at which the iteration starts
		 * @param region The region to iterate
		 * @throw BadRegionException @a region intersects outside of the document
		 * @throw BadPositionException The given position is outside of @a region
		 * @post &amp;document() == &amp;document(point)
		 * @post region() == region
		 * @post tell() == position(point)
		 * @post offset() == 0
		 */
		DocumentCharacterIterator::DocumentCharacterIterator(const locations::PointProxy& point, const Region& region)
				: document_(&kernel::document(point)), region_(region), position_(position(point)) {
			if(!encompasses(document_->region(), region_))
				throw BadRegionException(region_);
			else if(!encompasses(region_, position_))
				throw BadPositionException(position_);
		}

		/**
		 * Copy-constructor.
		 * @param other The source object
		 * @post &amp;document() == &other.document()
		 * @post region() == other.region()
		 * @post tell() == other.tell()
		 * @post offset() == 0
		 */
		DocumentCharacterIterator::DocumentCharacterIterator(const DocumentCharacterIterator& other) BOOST_NOEXCEPT
				: document_(other.document_), region_(other.region_), position_(other.position_) {
		}

		/**
		 * Implements @c #operator--.
		 * Moves to the backward character in UTF-32 code units. This method treats any eol as one character.
		 * @throw NoSuchElementException @c #hasPrevious returned @c false
		 */
		void DocumentCharacterIterator::decrement() {
			if(!hasPrevious())
#if 1
				throw NoSuchElementException("the iterator is at the first.");
#else
				return;
#endif
			if(offsetInLine(*this) == 0) {
				--position_.line;
				position_.offsetInLine = lineString().length();
			} else if(--position_.offsetInLine > 0 && hasPrevious()) {
				const String& s = lineString();
				if(text::surrogates::isLowSurrogate(s[offsetInLine(tell())]) && text::surrogates::isHighSurrogate(s[offsetInLine(tell()) - 1]))
					--position_.offsetInLine;
			}
			--offset_;
		}

		/**
		 * Implements @c #operator*.
		 * Returns a code point (not a UTF-16 code unit value) of the character addressed by this iterator.
		 * @return A code point of the current character
		 * @retval text#LINE_SEPARATOR The iterator is invalid or at the end of the line
		 * @retval text#INVALID_CODE_POINT The iterator is invalid or at the end of the @c #region
		 * @note This returns a raw unit value at any unpaired surrogate
		 */
		CodePoint DocumentCharacterIterator::dereference() const BOOST_NOEXCEPT {
			if(document_ == nullptr || tell() == *boost::const_end(region()))
				return text::INVALID_CODE_POINT;
			const String& s = lineString();
			if(kernel::offsetInLine(tell()) == s.length())
				return text::LINE_SEPARATOR;
			else
				return text::utf::decodeFirst(std::begin(s) + offsetInLine(tell()), std::end(s));
		}

		/**
		 * Implements @c #operator== and @c #operator!=.
		 * @return @c true if:
		 * - Both the two iterators are invalid, or
		 * - The two iterators refers to the same document and at the same position.
		 */
		bool DocumentCharacterIterator::equal(const DocumentCharacterIterator& other) const BOOST_NOEXCEPT {
			if(document_ != other.document_)
				return false;
			return document_ == nullptr || tell() == other.tell();
		}

		/**
		 * Implements @c #operator++.
		 * Moves to the forward character in UTF-32 code units. This method treats any eol as one character.
		 * @throw NoSuchElementException @c #hasNext returned @c false
		 */
		void DocumentCharacterIterator::increment() {
			if(!hasNext())
#if 1
				throw NoSuchElementException("the iterator is at the last.");
#else
				return;
#endif
			const String& s = lineString();
			if(offsetInLine(tell()) == s.length()) {
				++position_.line;
				position_.offsetInLine = 0;
			} else if(++position_.offsetInLine < s.length()
					&& hasNext()
					&& text::surrogates::isLowSurrogate(s[offsetInLine(tell())])
					&& text::surrogates::isHighSurrogate(s[offsetInLine(tell()) - 1]))
				++position_.offsetInLine;
			++offset_;
		}
		
		/// Returns the line text string.
		const String& DocumentCharacterIterator::lineString() const BOOST_NOEXCEPT {
			return document().lineString(line(*this));
		}
		
		/**
		 * Moves to the specified position.
		 * @param to The destination position
		 * @return This iterator
		 * @throw BadPositionException @a to is outside of the @c #region
		 * @note The offset is not change.
		 */
		DocumentCharacterIterator& DocumentCharacterIterator::seek(const Position& to) {
			if(!encompasses(region(), to))
				throw BadPositionException(to);
			position_ = to;
			return *this;
		}

		/**
		 * Sets the region of the iterator. The current position will adjusted.
		 * @param newRegion The new region to set
		 * @throw BadRegionException @a newRegion intersects outside of the document
		 * @note The offset is not change.
		 * @post encompasses(region(), tell())
		 */
		void DocumentCharacterIterator::setRegion(const Region& newRegion) {
			const Position e(*boost::const_end(document_->region()));
			if(*boost::const_begin(newRegion) > e || *boost::const_end(newRegion) > e)
				throw BadRegionException(newRegion);
			if(!encompasses(region_ = newRegion, tell()))
				position_ = clamp(tell(), region());
		}

		/**
		 * Returns @c DocumentCharacterIterator which addresses the beginning of the given document.
		 * @param document The document
		 * @return A @c DocumentCharacterIterator object
		 */
		DocumentCharacterIterator begin(const Document& document) BOOST_NOEXCEPT {
			return DocumentCharacterIterator(document, *boost::const_begin(document.region()));
		}

		/**
		 * Returns @c DocumentCharacterIterator which addresses the beginning of the given document.
		 * @param document The document
		 * @return A @c DocumentCharacterIterator object
		 */
		DocumentCharacterIterator cbegin(const Document& document) BOOST_NOEXCEPT {
			return DocumentCharacterIterator(document, *boost::const_begin(document.region()));
		}

		/**
		 * Returns @c DocumentCharacterIterator which addresses the beginning of the given document.
		 * @param document The document
		 * @return A @c DocumentCharacterIterator object
		 */
		DocumentCharacterIterator cend(const Document& document) BOOST_NOEXCEPT {
			return DocumentCharacterIterator(document, *boost::const_end(document.region()));
		}

		/**
		 * Returns @c DocumentCharacterIterator which addresses the beginning of the given document.
		 * @param document The document
		 * @return A @c DocumentCharacterIterator object
		 */
		DocumentCharacterIterator end(const Document& document) BOOST_NOEXCEPT {
			return DocumentCharacterIterator(document, *boost::const_end(document.region()));
		}
	}
}
