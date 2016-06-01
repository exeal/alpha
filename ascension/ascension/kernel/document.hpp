/**
 * @file document.hpp
 * @author exeal
 * @date 2003-2006 (was EditDoc.h)
 * @date 2006-2012, 2014-2016
 */

#ifndef ASCENSION_DOCUMENT_HPP
#define ASCENSION_DOCUMENT_HPP
#include <ascension/config.hpp>				// ASCENSION_DEFAULT_NEWLINE
#include <ascension/direction.hpp>
#include <ascension/corelib/basic-exceptions.hpp>
#include <ascension/corelib/detail/gap-vector.hpp>	// detail.GapVector
#include <ascension/corelib/detail/listeners.hpp>
#include <ascension/corelib/detail/scope-guard.hpp>
#include <ascension/corelib/memory.hpp>		// FastArenaObject
#include <ascension/corelib/signals.hpp>
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
#include <boost/iterator/iterator_facade.hpp>
#include <boost/optional.hpp>

namespace ascension {
	namespace text {
		class IdentifierSyntax;
	}

	namespace kernel {
		namespace detail {
			/// @internal Interface for objects which manage the set of points.
			template<typename PointType> class PointCollection {
			public:
				/// Adds the newly created point.
				virtual void addNewPoint(PointType& point) = 0;
				/// Deletes the point about to be destroyed (@a point is in its destructor call).
				virtual void removePoint(PointType& point) = 0;
			};
		} // namespace detail

		class AbstractPoint;
		class Point;
		class Document;

		/**
		 * A changed content of the document.
		 * @see DocumentListener, PositionUpdater
		 */
		class DocumentChange : private boost::noncopyable {
		public:
			/**
			 * Returns the erased region in the change.
			 * @return The normalized erased region in the change, or empty if no content was erased
			 */
			const Region& erasedRegion() const BOOST_NOEXCEPT {return erasedRegion_;}
			/**
			 * Returns the inserted region in the change.
			 * @return The normalized inserted region in the change, or empty if no string was inserted
			 */
			const Region& insertedRegion() const BOOST_NOEXCEPT {return insertedRegion_;}
		private:
			explicit DocumentChange(const Region& erasedRegion, const Region& insertedRegion) BOOST_NOEXCEPT;
			~DocumentChange() BOOST_NOEXCEPT;
			const Region erasedRegion_, insertedRegion_;
			friend class Document;
		};

		/**
		 * A key of document property.
		 * @see Document#property, Document#setProperty
		 */
		class DocumentPropertyKey : private boost::noncopyable {};

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
			~ReadOnlyDocumentException() BOOST_NOEXCEPT;
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
			typedef
#ifdef BOOST_OS_WINDOWS
				std::wstring
#else // ASCENSION_OS_POSIX
				std::string
#endif
				LocationType;
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
			virtual ~DocumentInput() BOOST_NOEXCEPT {}
			/// Returns the character encoding of the document input.
			virtual std::string encoding() const BOOST_NOEXCEPT = 0;
			/// Returns a string represents the location of the document input or an empty string.
			virtual LocationType location() const BOOST_NOEXCEPT = 0;
			/// Returns the default newline of the document. The returned value can be neighter
			/// @c text#Newline#USE_INTRINSIC_VALUE nor @c text#Newline#USE_DOCUMENT_INPUT.
			virtual text::Newline newline() const BOOST_NOEXCEPT = 0;
		private:
			virtual bool isChangeable(const Document& document) const BOOST_NOEXCEPT = 0;
			virtual void postFirstDocumentChange(const Document& document) BOOST_NOEXCEPT = 0;
			friend class Document;
		};

		/**
		 * A @c Bookmarker manages bookmarks of the document.
		 * @note This class is not intended to be subclassed.
		 * @see Document#bookmarker, locations#nextBookmark
		 */
		class Bookmarker : private DocumentListener, private boost::noncopyable {
		public:
			/// A @c Bookmarker#Iterator enumerates the all marked lines.
			class Iterator : public boost::iterators::iterator_facade<
				Iterator, Index, boost::bidirectional_traversal_tag, Index, std::ptrdiff_t> {
			private:
				Iterator(ascension::detail::GapVector<Index>::const_iterator impl) : impl_(impl) {}
				ascension::detail::GapVector<Index>::const_iterator impl_;
				// boost.iterators.iterator_facade requirements
				friend class boost::iterators::iterator_core_access;
				void decrement() {--impl_;}
				value_type dereference() const {return *impl_;}
				bool equal(const Iterator& other) const {return impl_ == other.impl_;}
				void increment() {++impl_;}
//				bool less(const Iterator& other) const {return impl_ < other.impl_;}
				friend class Bookmarker;
			};

		public:
			~Bookmarker() BOOST_NOEXCEPT;

			/// @name Marks
			/// @{
			void clear() BOOST_NOEXCEPT;
			bool isMarked(Index line) const;
			void mark(Index line, bool set = true);
			std::size_t numberOfMarks() const BOOST_NOEXCEPT;
			void toggle(Index line);
			/// @}

			/// @name Enumerations
			/// @{
			Iterator begin() const;
			Iterator end() const;
			boost::optional<Index> next(Index from, Direction direction, bool wrapAround = true, std::size_t marks = 1) const;
			/// @}

			/// @name Listeners
			/// @{
			void addListener(BookmarkListener& listener);
			void removeListener(BookmarkListener& listener);
			/// @}
		private:
			ascension::detail::GapVector<Index>::iterator find(Index line) const BOOST_NOEXCEPT;
			// DocumentListener
			void documentAboutToBeChanged(const Document& document);
			void documentChanged(const Document& document, const DocumentChange& change);
		private:
			explicit Bookmarker(Document& document) BOOST_NOEXCEPT;
			Document& document_;
			ascension::detail::GapVector<Index> markedLines_;
			ascension::detail::Listeners<BookmarkListener> listeners_;
			friend class Document;
		};

		// the documentation is at document.cpp
		class Document : public detail::PointCollection<AbstractPoint>,
			public texteditor::detail::SessionElement, private boost::noncopyable {
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
				text::Newline newline() const BOOST_NOEXCEPT {return newline_;}
				/// Returns the revision number when this last was changed previously.
				std::size_t revisionNumber() const BOOST_NOEXCEPT {return revisionNumber_;}
				/// Returns the text of the line.
				const String& text() const BOOST_NOEXCEPT {return text_;}
			private:
				explicit Line(std::size_t revisionNumber) BOOST_NOEXCEPT;
				Line(std::size_t revisionNumber, const String& text,
					const text::Newline& newline = ASCENSION_DEFAULT_NEWLINE);
				String text_;
				text::Newline newline_;
				std::size_t revisionNumber_;
				friend class Document;
			};
			typedef ascension::detail::GapVector<Line*> LineList;	///< List of lines.

		public:
			Document();
			virtual ~Document();

			/// @name Listeners and Strategies
			/// @{
//			void addCompoundChangeListener(CompoundChangeListener& listener);
			void addListener(DocumentListener& listener);
			void addPartitioningListener(DocumentPartitioningListener& listener);
			void addPrenotifiedListener(DocumentListener& listener);
			void addRollbackListener(DocumentRollbackListener& listener);
//			void removeCompoundChangeListener(CompoundChangeListener& listener);
			void removeListener(DocumentListener& listener);
			void removePartitioningListener(DocumentPartitioningListener& listener);
			void removePrenotifiedListener(DocumentListener& listener);
			void removeRollbackListener(DocumentRollbackListener& listener);
			/// @}

			/// @name Attributes
			/// @{
			Bookmarker& bookmarker() BOOST_NOEXCEPT;
			const Bookmarker& bookmarker() const BOOST_NOEXCEPT;
			std::weak_ptr<DocumentInput> input() const BOOST_NOEXCEPT;
			bool isModified() const BOOST_NOEXCEPT;
			bool isReadOnly() const BOOST_NOEXCEPT;
			void markUnmodified() BOOST_NOEXCEPT;
			const DocumentPartitioner& partitioner() const BOOST_NOEXCEPT;
			const String* property(const DocumentPropertyKey& key) const BOOST_NOEXCEPT;
			texteditor::Session* session() BOOST_NOEXCEPT;
			const texteditor::Session* session() const BOOST_NOEXCEPT;
			void setInput(std::weak_ptr<DocumentInput> newInput) BOOST_NOEXCEPT;
			void setModified() BOOST_NOEXCEPT;
			void setPartitioner(std::unique_ptr<DocumentPartitioner> newPartitioner) BOOST_NOEXCEPT;
			void setProperty(const DocumentPropertyKey& key, const String& property);
			void setReadOnly(bool readOnly = true) BOOST_NOEXCEPT;
			/// @}

			/// @name Contents
			/// @{
			Region accessibleRegion() const BOOST_NOEXCEPT;
			Index length(const text::Newline& newline = text::Newline::USE_INTRINSIC_VALUE) const BOOST_NOEXCEPT;
			const Line& lineContent(Index line) const;
			Index lineLength(Index line) const;
			Index lineOffset(Index line, const text::Newline& newline = text::Newline::USE_INTRINSIC_VALUE) const;
			const String& lineString(Index line) const;
			Index numberOfLines() const BOOST_NOEXCEPT;
			Region region() const BOOST_NOEXCEPT;
			std::size_t revisionNumber() const BOOST_NOEXCEPT;
			/// @}

			/// @name Content Type Information
			/// @{
			ContentTypeInformationProvider& contentTypeInformation() const BOOST_NOEXCEPT;
			void setContentTypeInformation(std::unique_ptr<ContentTypeInformationProvider> newProvider) BOOST_NOEXCEPT;
			/// @}

			/// @name Manipulations
			/// @{
			bool isChanging() const BOOST_NOEXCEPT;
			void replace(const Region& region, const StringPiece& text, Position* eos = nullptr);
			void replace(const Region& region, std::basic_istream<Char>& in, Position* eos = nullptr);
			virtual void resetContent();
			/// @}

#if ASCENSION_ABANDONED_AT_VERSION_08
			/// @name Locks
			/// @{
			bool lock(const void* locker);
			const void* locker() const BOOST_NOEXCEPT;
			void unlock(const void* locker);
			/// @}
#endif // ASCENSION_ABANDONED_AT_VERSION_08

			/// @name Undo/Redo and Compound Changes
			void beginCompoundChange();
			void clearUndoBuffer() BOOST_NOEXCEPT;
			void endCompoundChange();
			void insertUndoBoundary();
			bool isCompoundChanging() const BOOST_NOEXCEPT;
			bool isRecordingChanges() const BOOST_NOEXCEPT;
			std::size_t numberOfUndoableChanges() const BOOST_NOEXCEPT;
			std::size_t numberOfRedoableChanges() const BOOST_NOEXCEPT;
			void recordChanges(bool record) BOOST_NOEXCEPT;
			bool redo(std::size_t n = 1);
			bool undo(std::size_t n = 1);
			/// @}

			/// @name Narrowing
			/// @{
			bool isNarrowed() const BOOST_NOEXCEPT;
			void narrowToRegion(const Region& region);
			void widen() BOOST_NOEXCEPT;
			/// @}

			/// @name Signals
			/// @{
			typedef boost::signals2::signal<void(const Document&)> AccessibleRegionChangedSignal;
			SignalConnector<AccessibleRegionChangedSignal> accessibleRegionChangedSignal() BOOST_NOEXCEPT;
			typedef boost::signals2::signal<void(const Document&)> ModificationSignChangedSignal;
			SignalConnector<ModificationSignChangedSignal> modificationSignChangedSignal() BOOST_NOEXCEPT;
			typedef boost::signals2::signal<void(const Document&, const DocumentPropertyKey& key)> PropertyChangedSignal;
			SignalConnector<PropertyChangedSignal> propertyChangedSignal() BOOST_NOEXCEPT;
			typedef boost::signals2::signal<void(const Document&)> ReadOnlySignChangedSignal;
			SignalConnector<ReadOnlySignChangedSignal> readOnlySignChangedSignal() BOOST_NOEXCEPT;
			/// @}

			// overridables
		protected:
			virtual void doResetContent();

		private:
//			void doSetModified(bool modified) BOOST_NOEXCEPT;
			void fireDocumentAboutToBeChanged() BOOST_NOEXCEPT;
			void fireDocumentChanged(const DocumentChange& c, bool updateAllPoints = true) BOOST_NOEXCEPT;
			void initialize();
			void partitioningChanged(const Region& changedRegion) BOOST_NOEXCEPT;
			// detail.SessionElement
			void setSession(texteditor::Session& session) override BOOST_NOEXCEPT {session_ = &session;}
			// detail.PointCollection<AbstractPoint>
			void addNewPoint(AbstractPoint& point) override {points_.insert(&point);}
			void removePoint(AbstractPoint& point) override {points_.erase(&point);}

		private:
			class UndoManager;
			class DefaultContentTypeInformationProvider : public ContentTypeInformationProvider {
			public:
				DefaultContentTypeInformationProvider();
				~DefaultContentTypeInformationProvider() BOOST_NOEXCEPT;
				const text::IdentifierSyntax& getIdentifierSyntax(ContentType) const BOOST_NOEXCEPT {return *syntax_;}
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
			std::set<AbstractPoint*> points_;
			std::unique_ptr<UndoManager> undoManager_;
			std::map<const DocumentPropertyKey*, std::unique_ptr<String>> properties_;
			bool onceUndoBufferCleared_, recordingChanges_, changing_, rollbacking_;

			std::unique_ptr<std::pair<Position, std::unique_ptr<Point>>> accessibleRegion_;

			std::list<DocumentListener*> listeners_, prenotifiedListeners_;
//			ascension::detail::Listeners<CompoundChangeListener> compoundChangeListeners_;
			ascension::detail::Listeners<DocumentRollbackListener> rollbackListeners_;
			ascension::detail::Listeners<DocumentPartitioningListener> partitioningListeners_;
			AccessibleRegionChangedSignal accessibleRegionChangedSignal_;
			ModificationSignChangedSignal modificationSignChangedSignal_;
			PropertyChangedSignal propertyChangedSignal_;
			ReadOnlySignChangedSignal readOnlySignChangedSignal_;

			friend class DocumentPartitioner;
		};

		// the documentation is document.cpp
		typedef ascension::detail::MutexWithClass<
			Document, &Document::beginCompoundChange, &Document::endCompoundChange
		> CompoundChangeSaver;
#if 0
		// the documentation is document.cpp
		class DocumentLocker : private boost::noncopyable {
		public:
			DocumentLocker(Document& document);
			~DocumentLocker() BOOST_NOEXCEPT;
		private:
			Document* const document_;
		};
#endif

		/// @defgroup free_functions_to_change_document Free Functions to Change Document
		/// @see Document#replace
		/// @{
		void erase(Document& document, const Region& region);
		template<typename SinglePassReadableRange> void erase(Document& document, const SinglePassReadableRange& range);
		void insert(Document& document, const Position& at, const StringPiece& text, Position* endOfInsertedString = nullptr);
		void insert(Document& document, const Position& at, std::basic_istream<Char>& in, Position* endOfInsertedString = nullptr);
		/// @}

		/// @defgroup other_free_functions_related_to_document Other Free Functions Related to Document
		/// @{
		std::basic_ostream<Char>& writeDocumentToStream(
			std::basic_ostream<Char>& out, const Document& document,
			const Region& region, const text::Newline& newline = text::Newline::USE_INTRINSIC_VALUE);
		/// @}

		namespace positions {
			Index absoluteOffset(const Document& document, const Position& at, bool fromAccessibleStart);
//			bool isOutsideOfAccessibleRegion(const Document& document, const Position& position) BOOST_NOEXCEPT;
			bool isOutsideOfDocumentRegion(const Document& document, const Position& position) BOOST_NOEXCEPT;
			Position shrinkToAccessibleRegion(const Document& document, const Position& position) BOOST_NOEXCEPT;
			Region shrinkToAccessibleRegion(const Document& document, const Region& region) BOOST_NOEXCEPT;
			Position shrinkToDocumentRegion(const Document& document, const Position& position) BOOST_NOEXCEPT;
			Region shrinkToDocumentRegion(const Document& document, const Region& region) BOOST_NOEXCEPT;
			Position updatePosition(const Position& position, const DocumentChange& change, Direction gravity) BOOST_NOEXCEPT;
		} // namespace positions


		// inline implementation //////////////////////////////////////////////////////////////////

		/// Calls @c Document#replace.
		inline void erase(Document& document, const Region& region) {
			return document.replace(region, String(), nullptr);
		}

		/// Calls @c Document#replace.
		template<typename SinglePassReadableRange>
		inline void erase(Document& document, const SinglePassReadableRange& range) {
			return erase(document, Region::fromRange(range));
		}

		/// Calls @c Document#replace.
		inline void insert(Document& document, const Position& at,
				const StringPiece& text, Position* endOfInsertedString /* = nullptr */) {
			return document.replace(Region::makeEmpty(at), text, endOfInsertedString);
		}

		/// Calls @c Document#replace.
		inline void insert(Document& document, const Position& at,
				std::basic_istream<Char>& in, Position* endOfInsertedString /* = nullptr */) {
			return document.replace(Region::makeEmpty(at), in, endOfInsertedString);
		}

		/// Returns @c true if the given position is outside of the document.
		inline bool positions::isOutsideOfDocumentRegion(
				const Document& document, const Position& position) BOOST_NOEXCEPT {
			return line(position) >= document.numberOfLines()
				|| offsetInLine(position) > document.lineLength(line(position));
		}

		/** 
		 * Shrinks the given position into the accessible region of the document.
		 * @param document The document
		 * @param position The source position. This value can be outside of the document
		 * @return The result
		 */
		inline Position positions::shrinkToAccessibleRegion(const Document& document, const Position& position) BOOST_NOEXCEPT {
			if(!document.isNarrowed())
				return shrinkToDocumentRegion(document, position);
			const Region accessibleRegion(document.accessibleRegion());
			if(position < *boost::const_begin(accessibleRegion))
				return *boost::const_begin(accessibleRegion);
			else if(position > *boost::const_end(accessibleRegion))
				return *boost::const_end(accessibleRegion);
			return Position(line(position), std::min(offsetInLine(position), document.lineLength(line(position))));
		}
		
		/** 
		 * Shrinks the given region into the accessible region of the document.
		 * @param document The document
		 * @param region The source region. This value can intersect with outside of the document
		 * @return The result. This may not be normalized
		 */
		inline Region positions::shrinkToAccessibleRegion(const Document& document, const Region& region) BOOST_NOEXCEPT {
			return Region(shrinkToAccessibleRegion(document, *boost::const_begin(region)), shrinkToAccessibleRegion(document, *boost::const_end(region)));
		}
		
		/// Shrinks the given position into the document region.
		inline Position positions::shrinkToDocumentRegion(const Document& document, const Position& position) BOOST_NOEXCEPT {
			Position p(std::min(line(position), document.numberOfLines() - 1), 0);
			p.offsetInLine = std::min(offsetInLine(position), document.lineLength(line(p)));
			return p;
		}
		
		/// Shrinks the given region into the document region. The result may not be normalized.
		inline Region positions::shrinkToDocumentRegion(const Document& document, const Region& region) BOOST_NOEXCEPT {
			return Region(shrinkToDocumentRegion(document, *boost::const_begin(region)), shrinkToDocumentRegion(document, *boost::const_end(region)));
		}

		/// Returns the bookmarker of the document.
		inline Bookmarker& Document::bookmarker() BOOST_NOEXCEPT {return *bookmarker_;}

		/// Returns the bookmarker of the document.
		inline const Bookmarker& Document::bookmarker() const BOOST_NOEXCEPT {return *bookmarker_;}

		/// Returns the content information provider.
		inline ContentTypeInformationProvider& Document::contentTypeInformation() const BOOST_NOEXCEPT {
			return *contentTypeInformationProvider_;
		}
#if 0
		inline Document::LineIterator Document::getLineIterator(Index line) const {
			assertValid();
			if(line >= lines_.getSize())
				throw BadPositionException();
			return lines_.begin() + line;
		}
#endif
		/// Returns the document input or @c null.
		inline std::weak_ptr<DocumentInput> Document::input() const BOOST_NOEXCEPT {return input_;}

		/**
		 * Returns @c true if the document is changing (this means the document is in @c #insert or
		 * @c #insert call).
		 */
		inline bool Document::isChanging() const BOOST_NOEXCEPT {return changing_;}
		
		/**
		 * Returns @c true if the document has been modified.
		 * @see #setModified, DocumentStateListener#documentModificationSignChanged
		 */
		inline bool Document::isModified() const BOOST_NOEXCEPT {
			return revisionNumber() != lastUnmodifiedRevisionNumber_;
		}
		
		/**
		 * Returns @c true if the document is narrowed.
		 * @see #narrow, #widen
		 */
		inline bool Document::isNarrowed() const BOOST_NOEXCEPT {
			return accessibleRegion_.get() != nullptr;
		}

		/**
		 * Returns @c true if the document is read only.
		 * @see ReadOnlyDocumentException, #setReadOnly
		 */
		inline bool Document::isReadOnly() const BOOST_NOEXCEPT {return readOnly_;}
		
		/**
		 * Returns @c true if the document is recording the changes for undo/redo.
		 * @see #recordChanges, #numberOfUndoableChanges, #numberOfRedoableChanges
		 */
		inline bool Document::isRecordingChanges() const BOOST_NOEXCEPT {return recordingChanges_;}

		/**
		 * Returns the @c #Line value of the specified line.
		 * @param line The line
		 * @return The content of @a line
		 * @throw BadPostionException @a line is outside of the document
		 */
		inline const Document::Line& Document::lineContent(Index line) const {
			if(line >= lines_.size())
				throw BadPositionException(Position::bol(line));
			return *lines_[line];
		}

		/**
		 * Returns the length of the specified line. The line break is not included.
		 * @param line the line
		 * @return the length of @a line
		 * @throw BadLocationException @a line is outside of the document
		 */
		inline Index Document::lineLength(Index line) const {return lineString(line).length();}
		
		/**
		 * Returns the text string of the specified line.
		 * @param line the line
		 * @return the text
		 * @throw BadPostionException @a line is outside of the document
		 */
		inline const String& Document::lineString(Index line) const {return lineContent(line).text_;}
#if 0
		/// Returns the object locks the document or @c null if the document is not locked.
		inline const void* Document::locker() const BOOST_NOEXCEPT {return locker_;}
#endif
		/// Returns the number of lines in the document.
		inline Index Document::numberOfLines() const BOOST_NOEXCEPT {return lines_.size();}
		
		/// Returns the document partitioner of the document.
		inline const DocumentPartitioner& Document::partitioner() const BOOST_NOEXCEPT {
			if(partitioner_.get() == nullptr) {
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
		inline void Document::partitioningChanged(const Region& changedRegion) BOOST_NOEXCEPT {
			partitioningListeners_.notify<const Region&>(&DocumentPartitioningListener::documentPartitioningChanged, changedRegion);
		}
		
		/**
		 * Returns the property associated with the document.
		 * @param key the key of the property
		 * @return the property value or @c null if the specified property is not registered
		 * @see #setProperty
		 */
		inline const String* Document::property(const DocumentPropertyKey& key) const BOOST_NOEXCEPT {
			const std::map<const DocumentPropertyKey*, std::unique_ptr<String>>::const_iterator i(properties_.find(&key));
			return (i != std::end(properties_)) ? i->second.get() : nullptr;
		}

		/// Returns the entire region of the document. The returned region is normalized.
		/// @see #accessibleRegion
		inline Region Document::region() const BOOST_NOEXCEPT {
			return Region(Position::zero(), Position(numberOfLines() - 1, lineLength(numberOfLines() - 1)));
		}
		
		/// Returns the revision number.
		inline std::size_t Document::revisionNumber() const BOOST_NOEXCEPT {return revisionNumber_;}
		
		/// Returns the session to which the document belongs.
		inline texteditor::Session* Document::session() BOOST_NOEXCEPT {return session_;}
		
		/// Returns the session to which the document belongs.
		inline const texteditor::Session* Document::session() const BOOST_NOEXCEPT {return session_;}

		/**
		 * Sets the content type information provider.
		 * @param newProvider the new content type information provider. the ownership will be transferred
		 * to the callee. can be @c null
		 */
		inline void Document::setContentTypeInformation(std::unique_ptr<ContentTypeInformationProvider> newProvider) BOOST_NOEXCEPT {
			contentTypeInformationProvider_.reset((newProvider.get() != nullptr) ? newProvider.release() : new DefaultContentTypeInformationProvider);
		}

		/**
		 * Returns the content type of the partition contains the specified position.
		 * @param p The position in the document
		 * @throw ... Any exception thrown by @c DocumentPartitioner#contentType method
		 * @return The content type
		 */
		inline ContentType contentType(const std::pair<const Document&, Position>& p) {
			return std::get<0>(p).partitioner().contentType(std::get<1>(p));
		}
		
		/**
		 * Notifies the partitioning change to the listeners.
		 * Implementation of @c DocumentPartitioner *must* call this when the partitioning is changed.
		 * @param changedRegion the changed region
		 * @throw IllegalStateException the partitioner is not connected any document
		 */
		inline void DocumentPartitioner::notifyDocument(const Region& changedRegion) {
			if(document_ == nullptr)
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
			if(document_ == nullptr)
				throw IllegalStateException("the partitioner is not connected to any document.");
			else if(at > *boost::const_end(document_->region()))
				throw BadPositionException(at);
			return doGetPartition(at, partition);
		}

		namespace detail {
			/// @internal Returns the @c text#IdentifierSyntax object corresponds to the given point.
			inline const text::IdentifierSyntax& identifierSyntax(const std::pair<const Document&, Position>& p) {
				return std::get<0>(p).contentTypeInformation().getIdentifierSyntax(contentType(p));
			}
		}
	}
} // namespace ascension.kernel

#endif // !ASCENSION_DOCUMENT_HPP
