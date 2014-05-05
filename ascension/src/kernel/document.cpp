/**
 * @file document.cpp
 * @author exeal
 * @date 2003-2006 (was EditDoc.h)
 * @date 2006-2014
 */

#include <ascension/kernel/document.hpp>
#include <ascension/kernel/document-character-iterator.hpp>
#include <ascension/kernel/point.hpp>
#include <ascension/corelib/text/identifier-syntax.hpp>
#include <boost/foreach.hpp>
#include <boost/range/algorithm/find.hpp>
#include <boost/range/algorithm/lower_bound.hpp>
#include <algorithm>
#include <limits>	// std.numeric_limits


namespace ascension {
	namespace text {
		namespace {
			inline Newline resolveNewline(const kernel::Document& document, const Newline& newline) {
				if(newline == Newline::USE_DOCUMENT_INPUT) {
					// fallback
					std::shared_ptr<kernel::DocumentInput> input(document.input().lock());
					const Newline resolved((input.get() != nullptr) ? input->newline() : ASCENSION_DEFAULT_NEWLINE);
					assert(resolved.isLiteral());
					return resolved;
				}
				return newline;
			}
		} // namespace @0


		// text.Newline ///////////////////////////////////////////////////////////////////////////////////

		/// Line feed. Standard of Unix (Lf, U+000A).
		const Newline Newline::LINE_FEED(LINE_FEED);

		/// Carriage return. Old standard of Macintosh (Cr, U+000D).
		const Newline Newline::CARRIAGE_RETURN(CARRIAGE_RETURN);

		/// CR+LF. Standard of Windows (CrLf, U+000D U+000A).
		const Newline Newline::CARRIAGE_RETURN_FOLLOWED_BY_LINE_FEED((text::CARRIAGE_RETURN << 16) | text::LINE_FEED);

		/// Next line. Standard of EBCDIC-based OS (U+0085).
		const Newline Newline::NEXT_LINE(NEXT_LINE);

		/// Line separator (U+2028).
		const Newline Newline::LINE_SEPARATOR(LINE_SEPARATOR);

		/// Paragraph separator (U+2029).
		const Newline Newline::PARAGRAPH_SEPARATOR(PARAGRAPH_SEPARATOR);

		/// Represents any NLF as the actual newline of the line (@c kernel#Document#Line#newline()).
		const Newline Newline::USE_INTRINSIC_VALUE(0x80000000u);

		/// Represents any NLF as the value of @c kernel#DocumentInput#newline().
		const Newline Newline::USE_DOCUMENT_INPUT(0x80000001u);

		Newline::Newline(std::uint32_t value) BOOST_NOEXCEPT : value_(value) {
		}
	}

	namespace kernel {
		// kernel free functions //////////////////////////////////////////////////////////////////////////////////////

		/**
		 * Writes the content of the document to the specified output stream.
		 * <p>This method does not write Unicode byte order mark.</p>
		 * <p>This method explicitly flushes the output stream.</p>
		 * @param out The output stream
		 * @param document The document
		 * @param region The region to be written (This region is not restricted with narrowing)
		 * @param newline The newline representation
		 * @return @a out
		 * @throw ... Any exceptions out.operator bool, out.write and out.flush throw
		 * @see Newline#asString, Document#insert
		 */
		std::basic_ostream<Char>& kernel::writeDocumentToStream(std::basic_ostream<Char>& out,
				const Document& document, const Region& region, const text::Newline& newline /* = text::Newline::USE_INTRINSIC_VALUE */) {
			const Position& beginning = region.beginning();
			const Position end(std::min(region.end(), document.region().second));
			if(beginning.line == end.line) {	// shortcut for single-line
				if(out) {
					// TODO: this cast may be danger.
					out.write(document.line(end.line).data() + beginning.offsetInLine, static_cast<std::streamsize>(end.offsetInLine - beginning.offsetInLine));
				}
			} else {
				const text::Newline resolvedNewline(text::resolveNewline(document, newline));
				const String eol(resolvedNewline.isLiteral() ? resolvedNewline.asString() : String());
				assert(!eol.empty() || resolvedNewline == text::Newline::USE_INTRINSIC_VALUE);
				for(Index i = beginning.line; out; ++i) {
					const Document::Line& line = document.getLineInformation(i);
					const Index first = (i == beginning.line) ? beginning.offsetInLine : 0;
					const Index last = (i == end.line) ? end.offsetInLine : line.text().length();
					out.write(line.text().data() + first, static_cast<std::streamsize>(last - first));
					if(i == end.line)
						break;
					if(resolvedNewline == text::Newline::USE_INTRINSIC_VALUE) {
						const String intrinsicEol(line.newline().asString());
						out.write(intrinsicEol.data(), static_cast<std::streamsize>(intrinsicEol.length()));
					} else
						out.write(eol.data(), static_cast<std::streamsize>(eol.length()));
				}
			}
			return out.flush();
		}


		// kernel.positions free functions ////////////////////////////////////////////////////////////////////////////

		/**
		 * Returns absolute character offset of the specified position from the start of the document.
		 * @param document The document
		 * @param at The position
		 * @param fromAccessibleStart
		 * @throw BadPositionException @a at is outside of the document
		 */
		Index positions::absoluteOffset(const Document& document, const Position& at, bool fromAccessibleStart) {
			if(at > document.region().second)
				throw BadPositionException(at);
			Index offset = 0;
			const Position start((fromAccessibleStart ? document.accessibleRegion() : document.region()).first);
			for(Index line = start.line; ; ++line) {
				if(line == at.line) {
					offset += at.offsetInLine;
					break;
				} else {
					offset += document.lineLength(line) + 1;	// +1 is for a newline character
					if(line == start.line)
						offset -= start.offsetInLine;
				}
			}
			return offset;
		}

		/**
		 * Adapts the specified position to the document change.
		 * @param position The original position
		 * @param change The content of the document change
		 * @param gravity The gravity which determines the direction to which the position should move if a text was
		 *                inserted at the position. If @c FORWARD is specified, the position will move to the start of
		 *                the inserted text (no movement occur). Otherwise, move to the end of the inserted text
		 * @return The result position
		 */
		Position positions::updatePosition(const Position& position, const DocumentChange& change, Direction gravity) BOOST_NOEXCEPT {
			Position newPosition(position);
			if(!change.erasedRegion().isEmpty()) {	// deletion
				if(position < change.erasedRegion().second) {	// the end is behind the current line
					if(position <= change.erasedRegion().first)
						return newPosition;
					else	// in the region
						newPosition = change.erasedRegion().first;
				} else if(position.line > change.erasedRegion().second.line)	// in front of the current line
					newPosition.line -= change.erasedRegion().second.line - change.erasedRegion().first.line;
				else {	// the end is the current line
					if(position.line == change.erasedRegion().first.line)	// the region is single-line
						newPosition.offsetInLine -= change.erasedRegion().second.offsetInLine - change.erasedRegion().first.offsetInLine;
					else {	// the region is multiline
						newPosition.line -= change.erasedRegion().second.line - change.erasedRegion().first.line;
						newPosition.offsetInLine -= change.erasedRegion().second.offsetInLine - change.erasedRegion().first.offsetInLine;
					}
				}
			}
			if(!change.insertedRegion().isEmpty()) {	// insertion
				if(position < change.insertedRegion().first)	// behind the current position
					return newPosition;
				else if(position == change.insertedRegion().first && gravity == Direction::BACKWARD) // the current position + backward gravity
					return newPosition;
				else if(position.line > change.insertedRegion().first.line)	// in front of the current line
					newPosition.line += change.insertedRegion().second.line - change.insertedRegion().first.line;
				else {	// in the current line
					newPosition.line += change.insertedRegion().second.line - change.insertedRegion().first.line;
					newPosition.offsetInLine += change.insertedRegion().second.offsetInLine - change.insertedRegion().first.offsetInLine;
				}
			}
			return newPosition;
		}


		namespace {
#ifdef _DEBUG
			// for Document.length_ diagnostic
			Index calculateDocumentLength(const Document& document) {
				Index c = 0;
				const Index lines = document.numberOfLines();
				for(Index i = 0; i < lines; ++i)
					c += document.lineLength(i);
				return c;
			}
#endif // _DEBUG
		} // namespace @0


		// exception classes //////////////////////////////////////////////////////////////////////////////

		/// Protected default constructor.
		DocumentCantChangeException::DocumentCantChangeException() {
		}

		/// Destructor.
		DocumentCantChangeException::~DocumentCantChangeException() {
		}

		/// Default constructor.
		ReadOnlyDocumentException::ReadOnlyDocumentException() :
			IllegalStateException("The document is readonly. Any edit process is denied.") {
		}

		/// Destructor.
		ReadOnlyDocumentException::~ReadOnlyDocumentException() BOOST_NOEXCEPT {
		}

		/// Default constructor.
		DocumentAccessViolationException::DocumentAccessViolationException() :
			invalid_argument("The specified position or region is inaccessible.") {
		}

		/// Destructor.
		DocumentAccessViolationException::~DocumentAccessViolationException() BOOST_NOEXCEPT {
		}

		/// Constructor.
		DocumentInput::ChangeRejectedException::ChangeRejectedException() {
		}


		// DocumentChange /////////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * Private constructor.
		 * @param erasedRegion The erased region in the change
		 * @param insertedRegion The inserted region in the change
		 */
		DocumentChange::DocumentChange(const Region& erasedRegion, const Region& insertedRegion)
				BOOST_NOEXCEPT : erasedRegion_(erasedRegion), insertedRegion_(insertedRegion) {
			const_cast<Region&>(erasedRegion_).normalize();
			const_cast<Region&>(erasedRegion_).normalize();
		}

		/// Private destructor.
		DocumentChange::~DocumentChange() BOOST_NOEXCEPT {
		}


		// Bookmarker /////////////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * Private constructor.
		 * @param document The document
		 */
		Bookmarker::Bookmarker(Document& document) BOOST_NOEXCEPT : document_(document) {
			document.addListener(*this);
		}

		/// Destructor.
		Bookmarker::~Bookmarker() BOOST_NOEXCEPT {
			document_.removeListener(*this);
		}

		/**
		 * Registers the listener.
		 * @param listener The listener to be registered
		 * @throw std#invalid_argument @a listener is already registered
		 */
		void Bookmarker::addListener(BookmarkListener& listener) {
			listeners_.add(listener);
		}

		/**
		 * Returns a bidirectional iterator to addresses the first marked line.
		 * @see #end, #next
		 */
		Bookmarker::Iterator Bookmarker::begin() const {
			return Iterator(std::begin(markedLines_));
		}

		/// Deletes all bookmarks.
		void Bookmarker::clear() BOOST_NOEXCEPT {
			if(!markedLines_.empty()) {
				markedLines_.clear();
				listeners_.notify(&BookmarkListener::bookmarkCleared);
			}
		}

		/// @see DocumentListener#documentAboutToBeChanged
		void Bookmarker::documentAboutToBeChanged(const Document&) {
			// do nothing
		}

		/// @see DocumentListener#documentChanged
		void Bookmarker::documentChanged(const Document& document, const DocumentChange& change) {
			// update markedLines_ based on the change
			if(&document_ != &document || markedLines_.empty())
				return;
			if(change.erasedRegion().first.line != change.erasedRegion().second.line) {
				// remove the marks on the deleted lines
				const Index lines = change.erasedRegion().second.line - change.erasedRegion().first.line;
				const auto e(std::end(markedLines_));
				auto top(find(change.erasedRegion().first.line));
				if(top != e) {
					if(*top == change.erasedRegion().first.line)
						++top;
					auto bottom(find(change.erasedRegion().second.line));
					if(bottom != e && *bottom == change.erasedRegion().second.line)
						++bottom;
					// slide the following lines before removing
					if(bottom != e) {
						for(auto i(bottom); i != e; ++i)
							*i -= lines;	// ??? C4267@MSVC9
					}
					markedLines_.erase(top, bottom);	// GapVector<>.erase does not return an iterator
				}
			}
			if(change.insertedRegion().first.line != change.insertedRegion().second.line) {
				const Index lines = change.insertedRegion().second.line - change.insertedRegion().first.line;
				auto i(find(change.insertedRegion().first.line));
				if(i != std::end(markedLines_)) {
					if(*i == change.insertedRegion().first.line && change.insertedRegion().first.offsetInLine != 0)
						++i;
					for(const auto e(std::end(markedLines_)); i != e; ++i)
						*i += lines;	// ??? - C4267@MSVC9
				}
			}
		}

		/**
		 * Returns a bidirectional iterator to addresses just beyond the last marked line.
		 * @see #begin, #next
		 */
		Bookmarker::Iterator Bookmarker::end() const {
			return Iterator(std::end(markedLines_));
		}

		inline ascension::detail::GapVector<Index>::iterator Bookmarker::find(Index line) const BOOST_NOEXCEPT {
			// TODO: can write faster implementation (and design) by internal.searchBound().
			Bookmarker& self = const_cast<Bookmarker&>(*this);
			return boost::lower_bound(self.markedLines_, line);
		}

		/**
		 * Returns @c true if the specified line is bookmarked.
		 * @param line The line
		 * @throw BadPositionException @a line is outside of the document
		 */
		bool Bookmarker::isMarked(Index line) const {
			if(line >= document_.numberOfLines())
				throw BadPositionException(Position(line, 0));
			const auto i(find(line));
			return i != std::end(markedLines_) && *i == line;
		}

		/**
		 * Sets or clears the bookmark of the specified line.
		 * @param line The line
		 * @param set @c true to set bookmark, @c false to clear
		 * @throw BadPositionException @a line is outside of the document
		 */
		void Bookmarker::mark(Index line, bool set) {
			if(line >= document_.numberOfLines())
				throw BadPositionException(Position(line, 0));
			const auto i(find(line));
			if(i != std::end(markedLines_) && *i == line) {
				if(!set) {
					markedLines_.erase(i);
					listeners_.notify<Index>(&BookmarkListener::bookmarkChanged, line);
				}
			} else {
				if(set) {
					markedLines_.insert(i, line);
					listeners_.notify<Index>(&BookmarkListener::bookmarkChanged, line);
				}
			}
		}

		/**
		 * Returns the line number of the next/previous bookmarked line.
		 * @param from The start line number to search
		 * @param direction The direction to search
		 * @param wrapAround Set @c true to enable "wrapping around". If set, this method starts again from the end (in
		 *                   forward case) or beginning (in backward case) of the document when the reached the end or
		 *                   beginning of the document
		 * @param marks 
		 * @return The next bookmarked line or @c boost#none if not found
		 * @throw BadPositionException @a line is outside of the document
		 * @see #begin, #end
		 */
		boost::optional<Index> Bookmarker::next(Index from, Direction direction, bool wrapAround /* = true */, std::size_t marks /* = 1 */) const {
			// this code is tested by 'test/document-test.cpp'
			if(from >= document_.numberOfLines())
				throw BadPositionException(Position(from, 0));
			else if(marks == 0 || markedLines_.empty())
				return boost::none;
			else if(marks > markedLines_.size()) {
				if(!wrapAround)
					return boost::none;
				marks = marks % markedLines_.size();
				if(marks == 0)
					marks = markedLines_.size();
			}

			std::size_t i = static_cast<ascension::detail::GapVector<Index>::const_iterator>(find(from)) - std::begin(markedLines_);
			if(direction == Direction::FORWARD) {
				if(i == markedLines_.size()) {
					if(!wrapAround)
						return boost::none;
					i = 0;
					--marks;
				} else if(markedLines_[i] != from)
					--marks;
				if((i += marks) >= markedLines_.size()) {
					if(wrapAround)
						i -= markedLines_.size();
					else
						return boost::none;
				}
			} else {
				if(i < marks) {
					if(wrapAround)
						i += markedLines_.size();
					else
						return boost::none;
				}
				i -= marks;
			}
			return markedLines_[i];
		}

		/// Returns the number of the lines bookmarked.
		std::size_t Bookmarker::numberOfMarks() const BOOST_NOEXCEPT {
			return markedLines_.size();
		}

		/**
		 * Removes the listener.
		 * @param listener The listener to be removed
		 * @throw std#invalid_argument @a listener is not registered
		 */
		void Bookmarker::removeListener(BookmarkListener& listener) {
			listeners_.remove(listener);
		}

		/**
		 * Toggles the bookmark of the spcified line.
		 * @param line The line
		 * @throw BadPositionException @a line is outside of the document
		 */
		void Bookmarker::toggle(Index line) {
			if(line >= document_.numberOfLines())
				throw BadPositionException(Position(line, 0));
			const auto i(find(line));
			if(i == std::end(markedLines_) || *i != line)
				markedLines_.insert(i, line);
			else
				markedLines_.erase(i);
			listeners_.notify<Index>(&BookmarkListener::bookmarkChanged, line);
		}


		// DocumentPartitioner ////////////////////////////////////////////////////////////////////////////////////////

		/// Constructor.
		DocumentPartitioner::DocumentPartitioner() BOOST_NOEXCEPT : document_(nullptr) {
		}

		/// Destructor.
		DocumentPartitioner::~DocumentPartitioner() BOOST_NOEXCEPT {
		}


		// Document ///////////////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * @class ascension::kernel::Document
		 * A document manages a text content and supports text manipulations.
		 *
		 * All text content is represented in UTF-16. To treat this as UTF-32, use @c DocumentCharacterIterator.
		 *
		 * A document manages also its operation history, encoding, and newlines and writes to or reads the content
		 * from files or streams.
		 *
		 * @c #insert inserts a text string into any position. @c #erase deletes any text region. Other classes also
		 * provide text manipulation for the document.
		 *
		 * @c #insert and @c #erase methods throw @c DocumentCantChangeException when the change was rejected. This
		 * occurs if the the document was not marked modified and the document input's @c DocumentInput#isChangeable
		 * returned @c false.
		 *
		 * <h3>Revision and Modification Signature</h3>
		 *
		 * A document manages the revision number indicates how many times the document was changed. This value is
		 * initially zero. @c #replace, @c #redo and @c #resetContent methods increment and @c #undo method decrements
		 * the revision number. The current revision number can be obtained by @c #revisionNumber method. It is
		 * guarenteed that the contents of a document correspond to same revision number are equivalent.
		 *
		 * Clients of @c Document can query if "the document has been changed" by @c #isModified method. The
		 * modification signature (state) is determined based on the revision of the document. In a word, the document
		 * is treated as "modified" if the revision number is different from the one at "unmodified". For example
		 * (parenthesized numbers are the revisions),
		 *
		 * @code
		 * Document d;       // a document is unmodified initially (0)
		 * insert(d, text);  // increment the revision (1)
		 * d.isModified();   // true
		 * d.undo();         // decrement the revision (0)
		 * d.isModified();   // false
		 * @endcode
		 *
		 * Use @c #markUnmodified method to set the current revision as unmodified.
		 *
		 * To set the modification state as modified explicitly, use @c #setModified method. Called this method, the
		 * document never becomes unmodified unless @c #markUnmodified called.
		 *
		 * @code
		 * Document d;
		 * insert(d, text);
		 * d.markUnmodified();  // the revision 1 is as unmodified
		 * d.undo();            // modified (0)
		 * d.redo();            // unmodified (1)
		 * d.setModified();     // modified (1)
		 * d.undo();            // modified (0)
		 * @endcode
		 *
		 * <h3>Partitions</h3>
		 *
		 * A document can be devides into a sequence of semantic segments called partition. Document partitioners
		 * expressed by @c DocumentPartitioner class define these partitioning. Each partitions have its content type
		 * and region (see @c DocumentPartition). To set the new partitioner, use @c #setPartitioner method. The
		 * partitioner's ownership will be transferred to the document.
		 *
		 * @see DocumentPartitioner, Point, EditPoint
		 */

		/**
		 * @typedef ascension::kernel::Document::AccessibleRegionChangedSignal
		 * The signal which gets emitted when the accessible region of the document was changed.
		 * @param document The document
		 * @see #accessibleRegion, #accessibleRegionChangedSignal, #isNarrowed, #narrowToRegion, #widen
		 */

		/**
		 * @typedef ascension::kernel::Document::ModificationSignChangedSignal
		 * The signal which gets emitted when the modification flag of the document was changed.
		 * @param document The document
		 * @see #isModified, #markUnmodified, #modificationSignChangedSignal, #setModified
		 */

		/**
		 * @typedef ascension::kernel::Document::PropertyChangedSignal
		 * The signal which gets emitted when the property has @a key associated with the document was changed.
		 * @param document The document
		 * @param key The property key
		 * @see #property, #propertyChangedSignal, #setProperty
		 */

		/**
		 * @typedef ascension::kernel::Document::ReadOnlySignChangedSignal
		 * The signal which gets emitted when the read only mode of the document was changed.
		 * @param document The document
		 * @see #isReadOnly, #readOnlySignChangedSignal, #setReadOnly
		 */

		const DocumentPropertyKey Document::TITLE_PROPERTY;

		/**
		 * Returns the accessible region of the document. The returned region is normalized.
		 * @see #region, DocumentAccessViolationException
		 */
		Region Document::accessibleRegion() const BOOST_NOEXCEPT {
			return (accessibleRegion_.get() != nullptr) ? Region(accessibleRegion_->first, *accessibleRegion_->second) : region();
		}

		/// Returns the @c AccessibleRegionChangedSignal signal connector.
		SignalConnector<Document::AccessibleRegionChangedSignal> Document::accessibleRegionChangedSignal() BOOST_NOEXCEPT {
			return makeSignalConnector(accessibleRegionChangedSignal_);
		}

#if 0
		/**
		 * Registers the compound change listener.
		 * @param listener the listener to be registered
		 * @throw std#invalid_argument @a listener is already registered
		 */
		void Document::addCompoundChangeListener(CompoundChangeListener& listener) {
			compoundChangeListeners_.add(listener);
		}
#endif

		/**
		 * Registers the document listener with the document. After registration @a listener is notified about each
		 * modification of this document.
		 * @param listener The listener to be registered
		 * @throw std#invalid_argument @a listener is already registered
		 */
		void Document::addListener(DocumentListener& listener) {
			if(boost::range::find(listeners_, &listener) != boost::end(listeners_))
				throw std::invalid_argument("the listener already has been registered.");
			listeners_.push_back(&listener);
		}

		/**
		 * Registers the document partitioning listener with the document.
		 * @param listener The listener to be registered
		 * @throw std#invalid_argument @a listener is already registered
		 */
		void Document::addPartitioningListener(DocumentPartitioningListener& listener) {
			partitioningListeners_.add(listener);
		}

		/**
		 * Registers the document listener as one which is notified before those document listeners registered with
		 * @c #addListener are notified.
		 * @internal This method is not for public use.
		 * @param listener The listener to be registered
		 * @throw std#invalid_argument @a listener is already registered
		 */
		void Document::addPrenotifiedListener(DocumentListener& listener) {
			if(boost::range::find(prenotifiedListeners_, &listener) != boost::end(prenotifiedListeners_))
				throw std::invalid_argument("the listener already has been registered.");
			prenotifiedListeners_.push_back(&listener);
		}

		/**
		 * Registers the rollback listener.
		 * @param listener The listener to be registered
		 * @throw std#invalid_argument @a listener is already registered
		 */
		void Document::addRollbackListener(DocumentRollbackListener& listener) {
			rollbackListeners_.add(listener);
		}

		/// @c #resetContent invokes this method finally. Default implementation does nothing.
		void Document::doResetContent() {
		}

		void Document::fireDocumentAboutToBeChanged() BOOST_NOEXCEPT {
			if(partitioner_.get() != nullptr)
				partitioner_->documentAboutToBeChanged();
			BOOST_FOREACH(DocumentListener* listener, prenotifiedListeners_)
				listener->documentAboutToBeChanged(*this);
			BOOST_FOREACH(DocumentListener* listener, listeners_)
				listener->documentAboutToBeChanged(*this);
		}

		void Document::fireDocumentChanged(const DocumentChange& c, bool updateAllPoints /* = true */) BOOST_NOEXCEPT {
			if(partitioner_.get() != nullptr)
				partitioner_->documentChanged(c);
			if(updateAllPoints)
				updatePoints(c);
			BOOST_FOREACH(DocumentListener* listener, prenotifiedListeners_)
				listener->documentChanged(*this, c);
			BOOST_FOREACH(DocumentListener* listener, listeners_)
				listener->documentChanged(*this, c);
		}

		/**
		 * Returns the number of characters (UTF-16 code units) in the document.
		 * @param newline The method to count newlines
		 * @return The number of characters
		 */
		Index Document::length(const text::Newline& newline /* = Newline::USE_INTRINSIC_VALUE */) const BOOST_NOEXCEPT {
			const text::Newline resolvedNewline(text::resolveNewline(*this, newline));
			if(resolvedNewline.isLiteral())
				return length_ + (numberOfLines() - 1) * ((resolvedNewline != text::Newline::CARRIAGE_RETURN_FOLLOWED_BY_LINE_FEED) ? 1 : 2);
			assert(resolvedNewline == text::Newline::USE_INTRINSIC_VALUE);
			Index len = length_;
			const Index lines = numberOfLines();
			assert(lines > 0);
			for(Index i = 0; i < lines - 1; ++i)
				len += lines_[i]->newline_.asString().length();
			return len;
		}

		/**
		 * Returns the offset of the line.
		 * @param line The line
		 * @param newline The line representation policy for character counting
		 * @throw BadPostionException @a line is outside of the document
		 */
		Index Document::lineOffset(Index line, const text::Newline& newline) const {
			if(line >= numberOfLines())
				throw BadPositionException(Position(line, 0));
		
			const text::Newline resolvedNewline(text::resolveNewline(*this, newline));
			Index offset = 0, eolLength = resolvedNewline.isLiteral() ? resolvedNewline.asString().length() : 0;
			assert(eolLength != 0 || resolvedNewline == text::Newline::USE_INTRINSIC_VALUE);
			for(Index i = 0; i < line; ++i) {
				const Line& ln = *lines_[i];
				offset += ln.text_.length();
				if(newline == text::Newline::USE_INTRINSIC_VALUE)
					offset += ln.newline_.asString().length();
				else
					offset += eolLength;
			}
			return offset;
		}
#if 0
		/**
		 * Locks the document.
		 * @param locker the object locks the document. this object is used with later @c #unlock call
		 * @retval @c true Succeeded to lock
		 * @retval @c false Failed to lock because the other object already had locked, or the document's
		 *         input rejected the lock (only when the document was not marked as modified)
		 * @throw ReadOnlyDocumentException The document is read only
		 * @throw NullPointerException @a locker is @c null
		 * @see #unlock
		 */
		bool Document::lock(const void* locker) {
			// TODO: should support exclusive operation.
			if(isReadOnly())
				throw ReadOnlyDocumentException();
			else if(locker == nullptr)
				throw NullPointerException("locker");
			else if(locker_ != nullptr || (isModified() && input_.get() != nullptr && !input_->isChangeable()))
				return false;
			locker_ = locker;
			return true;
		}
#endif
		/**
		 * Marks the document unmodified at the current revision.
		 * For details about modification signature, see the documentation of @c Document class.
		 * @see #isModified, #setModified, #ModificationSignChangedSignal
		 */
		void Document::markUnmodified() BOOST_NOEXCEPT {
			if(isModified()) {
				lastUnmodifiedRevisionNumber_ = revisionNumber();
				modificationSignChangedSignal_(*this);
			}
		}

		/// Returns the @c ModificationSignChangedSignal signal connector.
		SignalConnector<Document::ModificationSignChangedSignal> Document::modificationSignChangedSignal() BOOST_NOEXCEPT {
			return makeSignalConnector(modificationSignChangedSignal_);
		}

		/**
		 * Narrows the accessible area to the specified region.
		 * If the document is already narrowed, the accessible region will just change to @a region. In this case,
		 * @a region can be wider than the current accessible region.
		 * @param region The region
		 * @throw BadRegionException @a region intersects with the outside of the document
		 * @see #isNarrowed, #widen, #AccessibleRegionChangedSignal
		 */
		void Document::narrowToRegion(const Region& region) {
			if(region.end() > this->region().end())
				throw BadRegionException(region);
			else if(region == accessibleRegion())
				return;
			if(accessibleRegion_.get() == 0) {
				accessibleRegion_.reset(new std::pair<Position, std::unique_ptr<Point>>);
				accessibleRegion_->second.reset(new Point(*this, region.end()));
			} else
				accessibleRegion_->second->moveTo(region.end());
			accessibleRegion_->first = region.beginning();
//			BOOST_FOREACH(Point* p, points_) {
//				if(p->isExcludedFromRestriction())
//					p->normalize();
//			}
			accessibleRegionChangedSignal_(*this);
		}

		/// Returns the @c PropertyChangedSignal signal connector.
		SignalConnector<Document::PropertyChangedSignal> Document::propertyChangedSignal() BOOST_NOEXCEPT {
			return makeSignalConnector(propertyChangedSignal_);
		}

		/// Returns the @c ReadOnlySignChangedSignal signal connector.
		SignalConnector<Document::ReadOnlySignChangedSignal> Document::readOnlySignChangedSignal() BOOST_NOEXCEPT {
			return makeSignalConnector(readOnlySignChangedSignal_);
		}

		/**
		 * Removes the document listener from the document.
		 * @param listener The listener to be removed
		 * @throw std#invalid_argument @a listener is not registered
		 */
		void Document::removeListener(DocumentListener& listener) {
			const std::list<DocumentListener*>::iterator i(boost::range::find(listeners_, &listener));
			if(i == boost::end(listeners_))
				throw std::invalid_argument("the listener is not registered.");
			listeners_.erase(i);
		}

		/**
		 * Removes the document partitioning listener from the document.
		 * @param listener The listener to be removed
		 * @throw std#invalid_argument @a listener is not registered
		 */
		void Document::removePartitioningListener(DocumentPartitioningListener& listener) {
			partitioningListeners_.remove(listener);
		}

		/**
		 * Removes the pre-notified document listener from the document.
		 * @internal This method is not for public use.
		 * @param listener The listener to be removed
		 * @throw std#invalid_argument @a listener is not registered
		 */
		void Document::removePrenotifiedListener(DocumentListener& listener) {
			const std::list<DocumentListener*>::iterator i(boost::range::find(prenotifiedListeners_, &listener));
			if(i == boost::end(prenotifiedListeners_))
				throw std::invalid_argument("the listener is not registered.");
			prenotifiedListeners_.erase(i);
		}

		/**
		 * Removes the rollback listener.
		 * @param listener The listener to be removed
		 * @throw std#invalid_argument @a listener is not registered
		 */
		void Document::removeRollbackListener(DocumentRollbackListener& listener) {
			rollbackListeners_.remove(listener);
		}

		/**
		 * Resets and initializes the content of the document. Does the following:
		 * - Clears the text buffer, invokes the two methods of @c DocumentListener and increments the revision number
		 *   even if the document was empty.
		 * - Moves the all point to the beginning of the document.
		 * - Clears the undo/redo buffers.
		 * - Resets the modification flag to @c false.
		 * - Resets the read-only flag to @c false.
		 * - Revokes the narrowing.
		 * - Removes the all bookmarks.
		 * @note This method does not call @c DocumentInput#isChangeable for rejection.
		 * @see #doResetContent
		 */
		void Document::resetContent() {
			if(lines_.empty())	// called by constructor
				lines_.insert(std::begin(lines_), new Line(0));
			else {
				widen();
				BOOST_FOREACH(Point* p, points_)
					p->moveTo(Position(0, 0));
				bookmarker_->clear();
		
				fireDocumentAboutToBeChanged();
				if(length_ != 0) {
					assert(!lines_.empty());
					for(std::size_t i = 0, c = lines_.size(); i < c; ++i)
						delete lines_[i];
					lines_.clear();
					lines_.insert(std::begin(lines_), new Line(revisionNumber_ + 1));
					length_ = 0;
					++revisionNumber_;
				}
				const DocumentChange ca(region(), Region(region().beginning()));
				fireDocumentChanged(ca, false);
			}
		
			setReadOnly(false);
			markUnmodified();
			clearUndoBuffer();
			onceUndoBufferCleared_ = false;
			doResetContent();
		}

		/**
		 * Sets the new document input.
		 * @param newInput The new document input. Can be @c null
		 */
		void Document::setInput(std::weak_ptr<DocumentInput> newInput) BOOST_NOEXCEPT {
			input_ = newInput;
		}

		/**
		 * Marks the document modified.
		 * For details about modification signature, see the documentation of @c Document class.
		 * @see #isModified, #markUnmodified, #dModificationSignChangedSignal
		 */
		void Document::setModified() BOOST_NOEXCEPT {
			const bool modified = isModified();
			lastUnmodifiedRevisionNumber_ = std::numeric_limits<std::size_t>::max();
			if(!modified)
				modificationSignChangedSignal_(*this);
		}

		/**
		 * Sets the new document partitioner.
		 * @param newPartitioner The new partitioner. The ownership will be transferred to the callee
		 */
		void Document::setPartitioner(std::unique_ptr<DocumentPartitioner> newPartitioner) BOOST_NOEXCEPT {
			partitioner_ = std::move(newPartitioner);
			if(partitioner_.get() != nullptr)
				partitioner_->install(*this);
			partitioningChanged(region());
		}

		/**
		 * Associates the given property with the document.
		 * @param key The key of the property
		 * @param property The property value
		 * @see #property, #PropertyChangedSignal
		 */
		void Document::setProperty(const DocumentPropertyKey& key, const String& property) {
			std::map<const DocumentPropertyKey*, std::unique_ptr<String>>::iterator i(properties_.find(&key));
			if(i == std::end(properties_))
				properties_.insert(std::make_pair(&key, new String(property)));
			else
				i->second->assign(property);
			propertyChangedSignal_(*this, key);
		}

		/**
		 * Makes the document read only or not.
		 * @see ReadOnlyDocumentException, #isReadOnly, #ReadOnlySignChangedSignal
		 */
		void Document::setReadOnly(bool readOnly /* = true */) BOOST_NOEXCEPT {
			if(readOnly != isReadOnly()) {
				readOnly_ = readOnly;
				readOnlySignChangedSignal_(*this);
			}
		}
#if 0
		/**
		 * Unlocks the document.
		 * @param locker The object used with previous @c #lock call
		 * @throw IllegalStateException The document is not locked
		 * @throw NullPointerException @a locker is @c null
		 * @throw std#invalid_argument @a locker is not the object used with @c #lock call
		 * @see #lock
		 */
		void Document::unlock(const void* locker) {
			// TODO: support exclusive operation.
			if(locker_ == nullptr)
				throw IllegalStateException("the document's input is not locked.");
			else if(locker == nullptr)
				throw NullPointerException("locker");
			else if(locker != locker_)
				throw invalid_argument("locker");
			locker_ = nullptr;
		}
#endif
		/**
		 * Informs the document change to the adapting points.
		 * @param change The document change
		 */
		inline void Document::updatePoints(const DocumentChange& change) BOOST_NOEXCEPT {
			BOOST_FOREACH(Point* p, points_) {
				if(p->adaptsToDocument())
					p->update(change);
			}
		}

		/**
		 * Revokes the narrowing.
		 * @see #isNarrowed, #narrow, #AccessibleRegionChangedSignal
		 */
		void Document::widen() BOOST_NOEXCEPT {
			if(accessibleRegion_.get() != nullptr) {
				accessibleRegion_.reset();
				accessibleRegionChangedSignal_(*this);
			}
		}


		// Document.Line //////////////////////////////////////////////////////////////////////////////////////////////

		Document::Line::Line(std::size_t revisionNumber) BOOST_NOEXCEPT : newline_(ASCENSION_DEFAULT_NEWLINE), revisionNumber_(revisionNumber) {
		}

		Document::Line::Line(std::size_t revisionNumber, const String& text,
				const text::Newline& newline /* = ASCENSION_DEFAULT_NEWLINE */) : text_(text), newline_(newline), revisionNumber_(revisionNumber) {
		}


		// Document.DefaultContentTypeInformationProvider /////////////////////////////////////////////////////////////

		Document::DefaultContentTypeInformationProvider::DefaultContentTypeInformationProvider() : syntax_(new text::IdentifierSyntax()) {
		}

		Document::DefaultContentTypeInformationProvider::~DefaultContentTypeInformationProvider() BOOST_NOEXCEPT {
			delete syntax_;
		}


		// CompoundChangeSaver ////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * @class ascension::kernel::CompoundChangeSaver
		 *
		 * Calls automatically @c Document#beginCompoundChange and @c Document#endCompoundChange.
		 *
		 * @code
		 * extern Document* target;
		 * CompoundChangeSaver saver(target);
		 * target-&gt;mayThrow();
		 * // target-&gt;endCompoundChange() will be called automatically
		 * @endcode
		 *
		 * @note This class is not intended to be subclassed.
		 */


#if 0
		// DocumentLocker /////////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * @class ascension::kernel::DocumentLocker
		 *
		 * Calls @c Document#lock and @c Document#unlock automatically.
		 *
		 * @note This class is not intended to be subclassed.
		 */

		/**
		 * Constructor calls @c Document#lock.
		 * @param document The document to lock
		 * @throw ReadOnlyDocumentException The document is read only
		 * @throw DocumentCantChangeException Failed to lock the document
		 */
		DocumentLocker::DocumentLocker(Document& document) : document_((document.locker() == nullptr) ? &document : nullptr) {
			if(document_ != nullptr && !document_->lock(this))	// manage lock if there is not an active lock
				throw DocumentCantChangeException();
		}

		/// Destructor calls @c Document#unlock.
		DocumentLocker::~DocumentLocker() BOOST_NOEXCEPT {
			if(document_ != nullptr)
				document_->unlock(this);
		}
#endif

		// DocumentCharacterIterator //////////////////////////////////////////////////////////////////////////////////

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
		
		const text::CharacterIterator::ConcreteTypeTag
			DocumentCharacterIterator::CONCRETE_TYPE_TAG_ = CharacterIterator::ConcreteTypeTag();

		/// Default constructor makes an invalid iterator object.
		DocumentCharacterIterator::DocumentCharacterIterator() BOOST_NOEXCEPT
				: CharacterIterator(CONCRETE_TYPE_TAG_), document_(nullptr), line_(0) {
		}

		/**
		 * Constructor. The iteration region is the accessible area of the document.
		 * @param document The document to iterate
		 * @param position The position at which the iteration starts
		 * @throw BadPositionException @a position is outside of the accessible area of the document
		 */
		DocumentCharacterIterator::DocumentCharacterIterator(const Document& document, const Position& position) :
				CharacterIterator(CONCRETE_TYPE_TAG_), document_(&document),
				region_(document.region()), line_(&document.line(position.line)), p_(position) {
			if(!region_.includes(p_))
				throw BadPositionException(p_);
		}

		/**
		 * Constructor. The iteration is started at @a region.beginning().
		 * @param document The document to iterate
		 * @param region The region to iterate
		 * @throw BadRegionException @a region intersects outside of the document
		 */
		DocumentCharacterIterator::DocumentCharacterIterator(const Document& document, const Region& region) :
				CharacterIterator(CONCRETE_TYPE_TAG_), document_(&document), region_(region),
				line_(&document.line(region.beginning().line)), p_(region.beginning()) {
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
		DocumentCharacterIterator::DocumentCharacterIterator(const Document& document, const Region& region, const Position& position) :
				CharacterIterator(CONCRETE_TYPE_TAG_), document_(&document), region_(region), line_(&document.line(position.line)), p_(position) {
			region_.normalize();
			if(!document.region().encompasses(region_))
				throw BadRegionException(region_);
			else if(!region_.includes(p_))
				throw BadPositionException(p_);
		}

		/// Copy-constructor.
		DocumentCharacterIterator::DocumentCharacterIterator(const DocumentCharacterIterator& other) BOOST_NOEXCEPT :
				text::CharacterIterator(other), document_(other.document_), region_(other.region_), line_(other.line_), p_(other.p_) {
		}

		/// @see text#CharacterIterator#current
		CodePoint DocumentCharacterIterator::current() const BOOST_NOEXCEPT {
			if(document() == nullptr || tell() == region().second)
				return DONE;
			else if(tell().offsetInLine == line().length())
				return text::LINE_SEPARATOR;
			else
				return (text::surrogates::isHighSurrogate((*line_)[tell().offsetInLine])
					&& tell().offsetInLine + 1 < line_->length() && text::surrogates::isLowSurrogate((*line_)[tell().offsetInLine + 1])) ?
					text::surrogates::decode((*line_)[tell().offsetInLine], (*line_)[tell().offsetInLine + 1]) : (*line_)[tell().offsetInLine];
		}

		/// @see text#CharacterIterator#doAssign
		void DocumentCharacterIterator::doAssign(const CharacterIterator& other) {
			CharacterIterator::operator=(other);
			const DocumentCharacterIterator& r = static_cast<const DocumentCharacterIterator&>(other);
			document_ = r.document_;
			line_ = r.line_;
			p_ = r.p_;
			region_ = r.region_;
		}

		/// @see text#CharacterIterator#doClone
		std::unique_ptr<text::CharacterIterator> DocumentCharacterIterator::doClone() const {
			return std::unique_ptr<text::CharacterIterator>(new DocumentCharacterIterator(*this));
		}

		/// @see text#CharacterIterator#doFirst
		void DocumentCharacterIterator::doFirst() {
			seek(region_.first);
		}

		/// @see text#CharacterIterator#doLast
		void DocumentCharacterIterator::doLast() {
			seek(region_.second);
		}

		/// @see text#CharacterIterator#doEquals
		bool DocumentCharacterIterator::doEquals(const CharacterIterator& other) const {
			const DocumentCharacterIterator& o = static_cast<const DocumentCharacterIterator&>(other);
			if(document() != o.document())
				return false;
			return document() == nullptr || tell() == o.tell();
		}

		/// @see text#CharacterIterator#doLess
		bool DocumentCharacterIterator::doLess(const CharacterIterator& other) const {
			return tell() < static_cast<const DocumentCharacterIterator&>(other).tell();
		}

		/// @see text#CharacterIterator#doNext
		void DocumentCharacterIterator::doNext() {
			if(!hasNext())
		//		throw out_of_range("the iterator is at the last.");
				return;
			else if(tell().offsetInLine == line_->length()) {
				line_ = &document_->line(++p_.line);
				p_.offsetInLine = 0;
			} else if(++p_.offsetInLine < line_->length()
					&& text::surrogates::isLowSurrogate((*line_)[tell().offsetInLine])
					&& text::surrogates::isHighSurrogate((*line_)[tell().offsetInLine - 1]))
				++p_.offsetInLine;
		}

		/// @see text#CharacterIterator#doPrevious
		void DocumentCharacterIterator::doPrevious() {
			if(!hasPrevious())
		//		throw out_of_range("the iterator is at the first.");
				return;
			else if(tell().offsetInLine == 0)
				p_.offsetInLine = (line_ = &document_->line(--p_.line))->length();
			else if(--p_.offsetInLine > 0
					&& text::surrogates::isLowSurrogate((*line_)[tell().offsetInLine])
					&& text::surrogates::isHighSurrogate((*line_)[tell().offsetInLine - 1]))
				--p_.offsetInLine;
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


		// NullPartitioner ////////////////////////////////////////////////////////////////////////////////////////////

		/// Constructor.
		NullPartitioner::NullPartitioner() BOOST_NOEXCEPT : p_(DEFAULT_CONTENT_TYPE, Region(Position(0, 0), Position(0, 0))), changed_(true) {
		}

		/// @see DocumentPartitioner#documentAboutToBeChanged
		void NullPartitioner::documentAboutToBeChanged() BOOST_NOEXCEPT {
		}

		/// @see DocumentPartitioner#documentChanged
		void NullPartitioner::documentChanged(const DocumentChange&) BOOST_NOEXCEPT {
			changed_ = true;
		}

		/// @see DocumentPartitioner#doGetPartition
		void NullPartitioner::doGetPartition(const Position&, DocumentPartition& partition) const BOOST_NOEXCEPT {
			if(changed_) {
				const_cast<NullPartitioner*>(this)->p_.region.second = document()->region().second;
				changed_ = false;
			}
			partition = p_;
		}

		/// @see DocumentPartitioner#doInstall
		void NullPartitioner::doInstall() BOOST_NOEXCEPT {
			changed_ = true;
		}
	}
}
