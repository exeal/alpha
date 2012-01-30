/**
 * @file document.hpp
 * @author exeal
 * @date 2003-2006 (was EditDoc.h)
 * @date 2006-2012
 */

#ifndef ASCENSION_DOCUMENT_HPP
#define ASCENSION_DOCUMENT_HPP

#include <ascension/config.hpp>				// ASCENSION_DEFAULT_NEWLINE
#include <ascension/corelib/basic-exceptions.hpp>
#include <ascension/corelib/gap-vector.hpp>	// detail.GapVector
#include <ascension/corelib/listeners.hpp>
#include <ascension/corelib/memory.hpp>		// FastArenaObject
#include <ascension/corelib/standard-iterator-adapter.hpp>	// detail.IteratorAdapter
#include <ascension/corelib/string-piece.hpp>
#include <ascension/corelib/text/newline.hpp>
#include <ascension/kernel/document-observers.hpp>
#include <ascension/kernel/partition.hpp>
#ifdef ASCENSION_OS_POSIX
#	include <sys/stat.h>	// for POSIX environment
#endif
#include <iostream>
#include <map>
#include <set>
#include <boost/optional.hpp>

namespace ascension {

	namespace text {class IdentifierSyntax;}

	namespace detail {
		/// @internal Interface for objects which manage the set of points.
		template<class PointType> class PointCollection {
		public:
			/// Adds the newly created point.
			virtual void addNewPoint(PointType& point) = 0;
			/// Deletes the point about to be destroyed (@a point is in its destructor call).
			virtual void removePoint(PointType& point) = 0;
		};
	} // namespace detail

	namespace kernel {

		class Point;
		class Document;

		/**
		 * A changed content of the document.
		 * @see DocumentListener, PositionUpdater
		 */
		class DocumentChange {
			ASCENSION_NONCOPYABLE_TAG(DocumentChange);
		public:
			const Region& erasedRegion() const /*throw()*/;
			const Region& insertedRegion() const /*throw()*/;
		private:
			explicit DocumentChange(const Region& erasedRegion, const Region& insertedRegion) /*throw()*/;
			~DocumentChange() /*throw()*/;
			const Region erasedRegion_, insertedRegion_;
			friend class Document;
		};

		/**
		 * A key of document property.
		 * @see Document#property, Document#setProperty
		 */
		class DocumentPropertyKey {
			ASCENSION_NONCOPYABLE_TAG(DocumentPropertyKey);
		public:
			/// Default constructor.
			DocumentPropertyKey() /*throw()*/ {}
		};

		/**
		 * Base class of the exceptions represent @c Document#replace could not change the document
		 * because of its property.
		 * @see ReadOnlyDocumentException, DocumentInput#ChangeRejectedException
		 */
		class DocumentCantChangeException {
		public:
			virtual ~DocumentCantChangeException();
		protected:
			DocumentCantChangeException();
		};

		/// Thrown when the read only document is about to be modified.
		class ReadOnlyDocumentException : public DocumentCantChangeException, public IllegalStateException {
		public:
			ReadOnlyDocumentException();
			~ReadOnlyDocumentException() throw();
		};

		/**
		 * Thrown when the caller accessed inaccessible region of the document.
		 * Document#accessibleRegion, Document#erase, Document#insert
		 */
		class DocumentAccessViolationException : public DocumentCantChangeException, public std::invalid_argument {
		public:
			DocumentAccessViolationException();
			~DocumentAccessViolationException() throw();
		};

		/**
		 * Provides information about a document input.
		 * @see Document
		 */
		class DocumentInput {
		public:
			/**
			 * Thrown if @c DocumentInput rejected the change of the document. For details, see the
			 * documentation of @c Document class.
			 * @see Document#redo, Document#replace, Document#resetContent, Document#undo,
			 *      DocumentInput#documentAboutToBeChanged
			 */
			class ChangeRejectedException : public DocumentCantChangeException {
			public:
				ChangeRejectedException();
			};
		public:
			/// Destructor.
			virtual ~DocumentInput() /*throw()*/ {}
			/// Returns the character encoding of the document input.
			virtual std::string encoding() const /*throw()*/ = 0;
			/// Returns a string represents the location of the document input or an empty string.
			virtual String location() const /*throw()*/ = 0;
			/// Returns the default newline of the document. The returned value can be neighter
			/// @c text#NLF_RAW_VALUE nor @c text#NLF_DOCUMENT_INPUT.
			virtual text::Newline newline() const /*throw()*/ = 0;
		private:
			virtual bool isChangeable(const Document& document) const /*throw()*/ = 0;
			virtual void postFirstDocumentChange(const Document& document) /*throw()*/ = 0;
			friend class Document;
		};

		/**
		 * A @c Bookmarker manages bookmarks of the document.
		 * @note This class is not intended to be subclassed.
		 * @see Document#bookmarker, locations#forwardBookmark, locations#backwardBookmark
		 */
		class Bookmarker : private DocumentListener {
			ASCENSION_NONCOPYABLE_TAG(Bookmarker);
		public:
			/// A @c Bookmarker#Iterator enumerates the all marked lines.
			class Iterator : public detail::IteratorAdapter<
				Iterator, std::iterator<std::bidirectional_iterator_tag, Index, std::ptrdiff_t, Index*, Index>> {
			private:
				Iterator(detail::GapVector<Index>::const_iterator impl) : impl_(impl) {}
				detail::GapVector<Index>::const_iterator impl_;
				// detail.IteratorAdapter requirements
				value_type current() const {return *impl_;}
				bool equals(const Iterator& other) const {return impl_ == other.impl_;}
				bool less(const Iterator& other) const {return impl_ < other.impl_;}
				void next() {++impl_;}
				void previous() {--impl_;}
				friend class Bookmarker;
				friend class detail::IteratorCoreAccess;
			};
			// destructor
			~Bookmarker() /*throw()*/;
			// listeners
			void addListener(BookmarkListener& listener);
			void removeListener(BookmarkListener& listener);
			// attributes
			bool isMarked(Index line) const;
			boost::optional<Index> next(Index from, Direction direction, bool wrapAround = true, std::size_t marks = 1) const;
			std::size_t numberOfMarks() const /*throw()*/;
			// enumerations
			Iterator begin() const;
			Iterator end() const;
			// operations
			void clear() /*throw()*/;
			void mark(Index line, bool set = true);
			void toggle(Index line);
		private:
			detail::GapVector<Index>::iterator find(Index line) const /*throw()*/;
			// DocumentListener
			void documentAboutToBeChanged(const Document& document);
			void documentChanged(const Document& document, const DocumentChange& change);
		private:
			explicit Bookmarker(Document& document) /*throw()*/;
			Document& document_;
			detail::GapVector<Index> markedLines_;
			detail::Listeners<BookmarkListener> listeners_;
			friend class Document;
		};

		// the documentation is at document.cpp
		class Document : public detail::PointCollection<Point>, public detail::SessionElement {
			ASCENSION_NONCOPYABLE_TAG(Document);
		public:
			/// The property key for the title of the document.
			static const DocumentPropertyKey TITLE_PROPERTY;

			/**
			 * Content of a line.
			 * @note This class is not intended to be subclassed.
			 */
			class Line : public FastArenaObject<Line> {
			public:
				/// Returns the newline of the line.
				text::Newline newline() const /*throw()*/ {return newline_;}
				/// Returns the revision number when this last was changed previously.
				std::size_t revisionNumber() const /*throw()*/ {return revisionNumber_;}
				/// Returns the text of the line.
				const String& text() const /*throw()*/ {return text_;}
			private:
				explicit Line(std::size_t revisionNumber) /*throw()*/;
				Line(std::size_t revisionNumber, const String& text, text::Newline newline = ASCENSION_DEFAULT_NEWLINE);
				String text_;
				text::Newline newline_;
				std::size_t revisionNumber_;
				friend class Document;
			};
			typedef detail::GapVector<Line*> LineList;	///< List of lines.

			// constructors
			Document();
			virtual ~Document();
			// reconstruct
			virtual void resetContent();
			// listeners and strategies
//			void addCompoundChangeListener(CompoundChangeListener& listener);
			void addListener(DocumentListener& listener);
			void addPartitioningListener(DocumentPartitioningListener& listener);
			void addPrenotifiedListener(DocumentListener& listener);
			void addRollbackListener(DocumentRollbackListener& listener);
			void addStateListener(DocumentStateListener& listener);
//			void removeCompoundChangeListener(CompoundChangeListener& listener);
			void removeListener(DocumentListener& listener);
			void removePartitioningListener(DocumentPartitioningListener& listener);
			void removePrenotifiedListener(DocumentListener& listener);
			void removeRollbackListener(DocumentRollbackListener& listener);
			void removeStateListener(DocumentStateListener& listener);
			// attributes
			Bookmarker& bookmarker() /*throw()*/;
			const Bookmarker& bookmarker() const /*throw()*/;
			std::weak_ptr<DocumentInput> input() const /*throw()*/;
			bool isModified() const /*throw()*/;
			bool isReadOnly() const /*throw()*/;
			void markUnmodified() /*throw()*/;
			const DocumentPartitioner& partitioner() const /*throw()*/;
			const String* property(const DocumentPropertyKey& key) const /*throw()*/;
			texteditor::Session* session() /*throw()*/;
			const texteditor::Session* session() const /*throw()*/;
			void setInput(std::weak_ptr<DocumentInput> newInput) /*throw()*/;
			void setModified() /*throw()*/;
			void setPartitioner(std::unique_ptr<DocumentPartitioner> newPartitioner) /*throw()*/;
			void setProperty(const DocumentPropertyKey& key, const String& property);
			void setReadOnly(bool readOnly = true) /*throw()*/;
			// contents
			Region accessibleRegion() const /*throw()*/;
			const Line& getLineInformation(Index line) const;
			Index length(text::Newline newline = text::NLF_RAW_VALUE) const;
			const String& line(Index line) const;
			Index lineLength(Index line) const;
			Index lineOffset(Index line, text::Newline newline = text::NLF_RAW_VALUE) const;
			Index numberOfLines() const /*throw()*/;
			Region region() const /*throw()*/;
			std::size_t revisionNumber() const /*throw()*/;
			// content type information
			ContentTypeInformationProvider& contentTypeInformation() const /*throw()*/;
			void setContentTypeInformation(std::unique_ptr<ContentTypeInformationProvider> newProvider) /*throw()*/;
			// manipulations
			bool isChanging() const /*throw()*/;
			void replace(const Region& region, const StringPiece& text, Position* eos = 0);
			void replace(const Region& region, std::basic_istream<Char>& in, Position* eos = 0);
#if ASCENSION_ABANDONED_AT_VERSION_08
			// locks
			bool lock(const void* locker);
			const void* locker() const /*throw()*/;
			void unlock(const void* locker);
#endif // ASCENSION_ABANDONED_AT_VERSION_08
			// undo/redo & compound changes
			void beginCompoundChange();
			void clearUndoBuffer() /*throw()*/;
			void endCompoundChange();
			void insertUndoBoundary();
			bool isCompoundChanging() const /*throw()*/;
			bool isRecordingChanges() const /*throw()*/;
			std::size_t numberOfUndoableChanges() const /*throw()*/;
			std::size_t numberOfRedoableChanges() const /*throw()*/;
			void recordChanges(bool record) /*throw()*/;
			bool redo(std::size_t n = 1);
			bool undo(std::size_t n = 1);
			// narrowing
			bool isNarrowed() const /*throw()*/;
			void narrowToRegion(const Region& region);
			void widen() /*throw()*/;
			// overridables
		protected:
			virtual void doResetContent();

		private:
//			void doSetModified(bool modified) /*throw()*/;
			void fireDocumentAboutToBeChanged() /*throw()*/;
			void fireDocumentChanged(const DocumentChange& c, bool updateAllPoints = true) /*throw()*/;
			void initialize();
			void partitioningChanged(const Region& changedRegion) /*throw()*/;
			void updatePoints(const DocumentChange& change) /*throw()*/;
			// detail.SessionElement
			void setSession(texteditor::Session& session) /*throw()*/ {session_ = &session;}
			// detail.IPointCollection<Point>
			void addNewPoint(Point& point) {points_.insert(&point);}
			void removePoint(Point& point) {points_.erase(&point);}

		private:
			class UndoManager;
			class DefaultContentTypeInformationProvider : public ContentTypeInformationProvider {
			public:
				DefaultContentTypeInformationProvider();
				~DefaultContentTypeInformationProvider() /*throw()*/;
				const text::IdentifierSyntax& getIdentifierSyntax(ContentType) const /*throw()*/ {return *syntax_;}
			private:
				text::IdentifierSyntax* syntax_;	// use a pointer to brake dependency
			};

			texteditor::Session* session_;
			std::weak_ptr<DocumentInput> input_;
			std::unique_ptr<DocumentPartitioner> partitioner_;
			std::unique_ptr<Bookmarker> bookmarker_;
			std::unique_ptr<ContentTypeInformationProvider> contentTypeInformationProvider_;
			bool readOnly_;
			LineList lines_;
			Index length_;
			std::size_t revisionNumber_, lastUnmodifiedRevisionNumber_;
			std::set<Point*> points_;
			UndoManager* undoManager_;
			std::map<const DocumentPropertyKey*, String*> properties_;
			bool onceUndoBufferCleared_, recordingChanges_, changing_, rollbacking_;

			std::unique_ptr<std::pair<Position, std::unique_ptr<Point>>> accessibleRegion_;

			std::list<DocumentListener*> listeners_, prenotifiedListeners_;
			detail::Listeners<DocumentStateListener> stateListeners_;
//			detail::Listeners<CompoundChangeListener> compoundChangeListeners_;
			detail::Listeners<DocumentRollbackListener> rollbackListeners_;
			detail::Listeners<DocumentPartitioningListener> partitioningListeners_;

			friend class DocumentPartitioner;
		};

		// the documentation is document.cpp
		class CompoundChangeSaver {
			ASCENSION_NONCOPYABLE_TAG(CompoundChangeSaver);
		public:
			explicit CompoundChangeSaver(Document* document);
			~CompoundChangeSaver();
		private:
			Document* const document_;
		};
#if 0
		// the documentation is document.cpp
		class DocumentLocker {
			ASCENSION_NONCOPYABLE_TAG(DocumentLocker);
		public:
			DocumentLocker(Document& document);
			~DocumentLocker() /*throw()*/;
		private:
			Document* const document_;
		};
#endif

		// free functions to change document
		void erase(Document& document, const Region& region);
		void erase(Document& document, const Position& first, const Position& second);
		void insert(Document& document, const Position& at, const StringPiece& text, Position* endOfInsertedString = 0);
		void insert(Document& document, const Position& at, std::basic_istream<Char>& in, Position* endOfInsertedString = 0);

		// other free functions related to document
		std::basic_ostream<Char>& writeDocumentToStream(std::basic_ostream<Char>& out,
			const Document& document, const Region& region, text::Newline newline = text::NLF_RAW_VALUE);

		namespace positions {
			Index absoluteOffset(const Document& document, const Position& at, bool fromAccessibleStart);
//			bool isOutsideOfAccessibleRegion(const Document& document, const Position& position) /*throw()*/;
			bool isOutsideOfDocumentRegion(const Document& document, const Position& position) /*throw()*/;
			Position shrinkToAccessibleRegion(const Document& document, const Position& position) /*throw()*/;
			Region shrinkToAccessibleRegion(const Document& document, const Region& region) /*throw()*/;
			Position shrinkToDocumentRegion(const Document& document, const Position& position) /*throw()*/;
			Region shrinkToDocumentRegion(const Document& document, const Region& region) /*throw()*/;
			Position updatePosition(const Position& position, const DocumentChange& change, Direction gravity) /*throw()*/;
		} // namespace positions


// inline implementation //////////////////////////////////////////////////////////////////////////

/// Calls @c Document#replace.
inline void erase(Document& document, const Region& region) {
	return document.replace(region, 0, 0);
}

/// Calls @c Document#replace.
inline void erase(Document& document, const Position& first, const Position& second) {
	return erase(document, Region(first, second));
}

/// Calls @c Document#replace.
inline void insert(Document& document, const Position& at,
		const StringPiece& text, Position* endOfInsertedString /* = 0 */) {
	return document.replace(Region(at), text, endOfInsertedString);
}

/// Calls @c Document#replace.
inline void insert(Document& document, const Position& at,
		std::basic_istream<Char>& in, Position* endOfInsertedString /* = 0 */) {
	return document.replace(Region(at), in, endOfInsertedString);
}

/// Returns @c true if the given position is outside of the document.
inline bool positions::isOutsideOfDocumentRegion(
		const Document& document, const Position& position) /*throw()*/ {
	return position.line >= document.numberOfLines()
		|| position.offsetInLine > document.lineLength(position.line);
}

/** 
 * Shrinks the given position into the accessible region of the document.
 * @param document The document
 * @param position The source position. This value can be outside of the document
 * @return The result
 */
inline Position positions::shrinkToAccessibleRegion(const Document& document, const Position& position) /*throw()*/ {
	if(!document.isNarrowed())
		return shrinkToDocumentRegion(document, position);
	const Region accessibleRegion(document.accessibleRegion());
	if(position < accessibleRegion.first)
		return accessibleRegion.first;
	else if(position > accessibleRegion.second)
		return accessibleRegion.second;
	return Position(position.line, std::min(position.offsetInLine, document.lineLength(position.line)));
}

/** 
 * Shrinks the given region into the accessible region of the document.
 * @param document The document
 * @param region The source region. This value can intersect with outside of the document
 * @return The result. This may not be normalized
 */
inline Region positions::shrinkToAccessibleRegion(const Document& document, const Region& region) /*throw()*/ {
	return Region(shrinkToAccessibleRegion(document, region.first), shrinkToAccessibleRegion(document, region.second));
}

/// Shrinks the given position into the document region.
inline Position positions::shrinkToDocumentRegion(const Document& document, const Position& position) /*throw()*/ {
	Position p(std::min(position.line, document.numberOfLines() - 1), 0);
	p.offsetInLine = std::min(position.offsetInLine, document.lineLength(p.line));
	return p;
}

/// Shrinks the given region into the document region. The result may not be normalized.
inline Region positions::shrinkToDocumentRegion(const Document& document, const Region& region) /*throw()*/ {
	return Region(shrinkToDocumentRegion(document, region.first), shrinkToDocumentRegion(document, region.second));
}

/// Returns the erased region in the change. The returned region is normalized. Empty if no content was erased.
inline const Region& DocumentChange::erasedRegion() const /*throw()*/ {return erasedRegion_;}

/// Returns the inserted region in the change. The returned region is normalized. Empty if no string was inserted.
inline const Region& DocumentChange::insertedRegion() const /*throw()*/ {return insertedRegion_;}

/// Returns the bookmarker of the document.
inline Bookmarker& Document::bookmarker() /*throw()*/ {return *bookmarker_;}

/// Returns the bookmarker of the document.
inline const Bookmarker& Document::bookmarker() const /*throw()*/ {return *bookmarker_;}

/// Returns the content information provider.
inline ContentTypeInformationProvider& Document::contentTypeInformation() const /*throw()*/ {return *contentTypeInformationProvider_;}

/**
 * Returns the information of the specified line.
 * @param line the line
 * @return the information about @a line
 * @throw BadPostionException @a line is outside of the document
 */
inline const Document::Line& Document::getLineInformation(Index line) const {
	if(line >= lines_.size()) throw BadPositionException(Position(line, 0)); return *lines_[line];}

#if 0
inline Document::LineIterator Document::getLineIterator(Index line) const {
	assertValid();
	if(line >= lines_.getSize())
		throw BadPositionException();
	return lines_.begin() + line;
}
#endif

/// Returns the document input or @c null.
inline std::weak_ptr<DocumentInput> Document::input() const /*throw()*/ {return input_;}

/**
 * Returns @c true if the document is changing (this means the document is in @c #insert or
 * @c #insert call).
 */
inline bool Document::isChanging() const /*throw()*/ {return changing_;}

/**
 * Returns @c true if the document has been modified.
 * @see #setModified, IDocumentStateListener#documentModificationSignChanged
 */
inline bool Document::isModified() const /*throw()*/ {return revisionNumber() != lastUnmodifiedRevisionNumber_;}

/**
 * Returns @c true if the document is narrowed.
 * @see #narrow, #widen
 */
inline bool Document::isNarrowed() const /*throw()*/ {return accessibleRegion_.get() != 0;}

/**
 * Returns @c true if the document is read only.
 * @see ReadOnlyDocumentException, #setReadOnly
 */
inline bool Document::isReadOnly() const /*throw()*/ {return readOnly_;}

/**
 * Returns @c true if the document is recording the changes for undo/redo.
 * @see #recordChanges, #numberOfUndoableChanges, #numberOfRedoableChanges
 */
inline bool Document::isRecordingChanges() const /*throw()*/ {return recordingChanges_;}

/**
 * Returns the text of the specified line.
 * @param line the line
 * @return the text
 * @throw BadPostionException @a line is outside of the document
 */
inline const String& Document::line(Index line) const {return getLineInformation(line).text_;}

/**
 * Returns the length of the specified line. The line break is not included.
 * @param line the line
 * @return the length of @a line
 * @throw BadLocationException @a line is outside of the document
 */
inline Index Document::lineLength(Index line) const {return this->line(line).length();}
#if 0
/// Returns the object locks the document or @c null if the document is not locked.
inline const void* Document::locker() const /*throw()*/ {return locker_;}
#endif
/// Returns the number of lines in the document.
inline Index Document::numberOfLines() const /*throw()*/ {return lines_.size();}

/// Returns the document partitioner of the document.
inline const DocumentPartitioner& Document::partitioner() const /*throw()*/ {
	if(partitioner_.get() == 0) {
		Document& self = *const_cast<Document*>(this);
		self.partitioner_.reset(static_cast<DocumentPartitioner*>(new NullPartitioner));
		self.partitioner_->install(self);
	}
	return *partitioner_;
}

/**
 * Transfers the partitioning change to the listeners.
 * @param changedRegion the changed region
 */
inline void Document::partitioningChanged(const Region& changedRegion) /*throw()*/ {
	partitioningListeners_.notify<const Region&>(&DocumentPartitioningListener::documentPartitioningChanged, changedRegion);}

/**
 * Returns the property associated with the document.
 * @param key the key of the property
 * @return the property value or @c null if the specified property is not registered
 * @see #setProperty
 */
inline const String* Document::property(const DocumentPropertyKey& key) const /*throw()*/ {
	const std::map<const DocumentPropertyKey*, String*>::const_iterator i(properties_.find(&key));
	return (i != properties_.end()) ? i->second : 0;
}

/// Returns the entire region of the document. The returned region is normalized.
/// @see #accessibleRegion
inline Region Document::region() const /*throw()*/ {
	return Region(Position(0, 0), Position(numberOfLines() - 1, lineLength(numberOfLines() - 1)));}

/// Returns the revision number.
inline std::size_t Document::revisionNumber() const /*throw()*/ {return revisionNumber_;}

/// Returns the session to which the document belongs.
inline texteditor::Session* Document::session() /*throw()*/ {return session_;}

/// Returns the session to which the document belongs.
inline const texteditor::Session* Document::session() const /*throw()*/ {return session_;}

/**
 * Sets the content type information provider.
 * @param newProvider the new content type information provider. the ownership will be transferred
 * to the callee. can be @c null
 */
inline void Document::setContentTypeInformation(std::unique_ptr<ContentTypeInformationProvider> newProvider) /*throw()*/ {
	contentTypeInformationProvider_.reset((newProvider.get() != 0) ? newProvider.release() : new DefaultContentTypeInformationProvider);}

/**
 * Notifies the partitioning change to the listeners.
 * Implementation of @c DocumentPartitioner *must* call this when the partitioning is changed.
 * @param changedRegion the changed region
 * @throw IllegalStateException the partitioner is not connected any document
 */
inline void DocumentPartitioner::notifyDocument(const Region& changedRegion) {
	if(document_ == 0)
		throw IllegalStateException("the partitioner is not connected any document.");
	document_->partitioningChanged(changedRegion);	// $friendly-access
}

/**
 * Returns the document partition contains the specified position.
 * @param at the position
 * @param[out] partition the partition
 * @throw BadPositionException @a position is outside of the document
 * @throw IllegalStateException the partitioner is not connected to any document
 */
inline void DocumentPartitioner::partition(const Position& at, DocumentPartition& partition) const {
	if(document_ == 0)
		throw IllegalStateException("the partitioner is not connected to any document.");
	else if(at > document_->region().second)
		throw BadPositionException(at);
	return doGetPartition(at, partition);
}

	}
} // namespace ascension.kernel

#endif // !ASCENSION_DOCUMENT_HPP
