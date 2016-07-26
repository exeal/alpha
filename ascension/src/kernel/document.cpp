/**
 * @file document.cpp
 * @author exeal
 * @date 2003-2006 (was EditDoc.h)
 * @date 2006-2016
 */

#include <ascension/kernel/bookmarker.hpp>
#include <ascension/kernel/document.hpp>
#include <ascension/kernel/document-character-iterator.hpp>
#include <ascension/kernel/document-input.hpp>
#include <ascension/kernel/point.hpp>
#include <ascension/corelib/text/identifier-syntax.hpp>
#include <boost/foreach.hpp>
#include <boost/range/algorithm/find.hpp>
#include <algorithm>
#include <limits>	// std.numeric_limits


namespace ascension {
	namespace kernel {
		namespace {
			inline text::Newline resolveNewline(const Document& document, const text::Newline& newline) {
				if(newline == text::Newline::USE_DOCUMENT_INPUT) {
					// fallback
					std::shared_ptr<DocumentInput> input(document.input().lock());
					const text::Newline resolved((input.get() != nullptr) ? input->newline() : ASCENSION_DEFAULT_NEWLINE);
					assert(resolved.isLiteral());
					return resolved;
				}
				return newline;
			}
		} // namespace @0


		// kernel free functions //////////////////////////////////////////////////////////////////////////////////////

		/// Returns the content type of the document partition contains the point.
		ContentType contentType(const std::pair<const Document&, const Position&>& p) {
			return std::get<0>(p).partitioner().contentType(std::get<1>(p));
		}

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
			const Position& beginning = *boost::const_begin(region);
			const Position end(std::min(*boost::const_end(region), *boost::const_end(document.region())));
			if(line(beginning) == line(end)) {	// shortcut for single-line
				if(out) {
					// TODO: this cast may be danger.
					out.write(document.lineString(line(end)).data() + offsetInLine(beginning), static_cast<std::streamsize>(offsetInLine(end) - offsetInLine(beginning)));
				}
			} else {
				const text::Newline resolvedNewline(resolveNewline(document, newline));
				const String eol(resolvedNewline.isLiteral() ? resolvedNewline.asString() : String());
				assert(!eol.empty() || resolvedNewline == text::Newline::USE_INTRINSIC_VALUE);
				for(Index i = beginning.line; out; ++i) {
					const Document::Line& lineContent = document.lineContent(i);
					const Index first = (i == line(beginning)) ? offsetInLine(beginning) : 0;
					const Index last = (i == line(end)) ? offsetInLine(end) : lineContent.text().length();
					out.write(lineContent.text().data() + first, static_cast<std::streamsize>(last - first));
					if(i == line(end))
						break;
					if(resolvedNewline == text::Newline::USE_INTRINSIC_VALUE) {
						const String intrinsicEol(lineContent.newline().asString());
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
			if(at > *boost::const_end(document.region()))
				throw BadPositionException(at);
			Index offset = 0;
			const Position start(*boost::const_begin(fromAccessibleStart ? document.accessibleRegion() : document.region()));
			for(Index i = line(start); ; ++i) {
				if(i == line(at)) {
					offset += offsetInLine(at);
					break;
				} else {
					offset += document.lineLength(i) + 1;	// +1 is for a newline character
					if(i == line(start))
						offset -= offsetInLine(start);
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
			if(!boost::empty(change.erasedRegion())) {	// deletion
				if(position < *boost::const_end(change.erasedRegion())) {	// the end is behind the current line
					if(position <= *boost::const_begin(change.erasedRegion()))
						return newPosition;
					else	// in the region
						newPosition = *boost::const_begin(change.erasedRegion());
				} else if(line(position) > line(*boost::const_end(change.erasedRegion())))	// in front of the current line
					newPosition.line -= boost::size(change.erasedRegion().lines()) - 1;
				else {	// the end is the current line
					if(line(position) == line(*boost::const_begin(change.erasedRegion())))	// the region is single-line
						newPosition.offsetInLine -= offsetInLine(*boost::const_end(change.erasedRegion())) - offsetInLine(*boost::const_begin(change.erasedRegion()));
					else {	// the region is multiline
						newPosition.line -= boost::size(change.erasedRegion().lines()) - 1;
						newPosition.offsetInLine -= offsetInLine(*boost::const_end(change.erasedRegion())) - offsetInLine(*boost::const_begin(change.erasedRegion()));
					}
				}
			}
			if(!boost::empty(change.insertedRegion())) {	// insertion
				if(newPosition == *boost::const_begin(change.insertedRegion())) {
					if(gravity == Direction::FORWARD)
						newPosition = *boost::const_end(change.insertedRegion());
				} else if(newPosition > *boost::const_begin(change.insertedRegion())) {
					if(line(*boost::const_begin(change.insertedRegion())) == line(newPosition))
						newPosition.offsetInLine += offsetInLine(*boost::const_end(change.insertedRegion())) - offsetInLine(*boost::const_begin(change.insertedRegion()));
					newPosition.line += boost::size(change.insertedRegion().lines()) - 1;
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
		}

		/// Private destructor.
		DocumentChange::~DocumentChange() BOOST_NOEXCEPT {
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
		 * @c #replace methods throw @c DocumentCantChangeException when the change was rejected. This occurs if the
		 * the document was not marked modified and the document input's @c DocumentInput#isChangeable returned
		 * @c false.
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
			return (accessibleRegion_.get() != nullptr) ? Region(std::get<0>(*accessibleRegion_), std::get<1>(*accessibleRegion_)->position()) : region();
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
			if(updateAllPoints) {
				BOOST_FOREACH(AbstractPoint* p, points_) {
					if(p->adaptsToDocument())
						p->documentChanged(c);
				}
			}
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
			const text::Newline resolvedNewline(resolveNewline(*this, newline));
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
				throw BadPositionException(Position::bol(line));
		
			const text::Newline resolvedNewline(resolveNewline(*this, newline));
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
			if(*boost::const_end(region) > *boost::const_end(this->region()))
				throw BadRegionException(region);
			else if(region == accessibleRegion())
				return;
			if(accessibleRegion_.get() == 0) {
				accessibleRegion_.reset(new std::pair<Position, std::unique_ptr<Point>>);
				accessibleRegion_->second.reset(new Point(*this, *boost::const_end(region)));
			} else
				accessibleRegion_->second->moveTo(*boost::const_end(region));
			accessibleRegion_->first = *boost::const_begin(region);
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
				BOOST_FOREACH(AbstractPoint* p, points_) {
					if(p->adaptsToDocument())
						p->contentReset();
				}
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
				const DocumentChange ca(region(), Region::makeEmpty(*boost::const_begin(region())));
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
		 * @see #isModified, #markUnmodified, #ModificationSignChangedSignal
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
		 * Revokes the narrowing.
		 * @see #isNarrowed, #narrowToRegion, #AccessibleRegionChangedSignal
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

		// NullPartitioner ////////////////////////////////////////////////////////////////////////////////////////////

		/// Constructor.
		NullPartitioner::NullPartitioner() BOOST_NOEXCEPT : p_(DEFAULT_CONTENT_TYPE, Region::zero()), changed_(true) {
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
				const_cast<NullPartitioner*>(this)->p_.region = Region(*boost::const_begin(p_.region), *boost::const_end(document()->region()));
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
