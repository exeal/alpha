/**
 * @file document.hpp
 * @author exeal
 * @date 2003-2006 (was EditDoc.h)
 * @date 2006-2007
 */

#ifndef ASCENSION_DOCUMENT_HPP
#define ASCENSION_DOCUMENT_HPP

#include <set>
#ifdef ASCENSION_POSIX
#	include <sys/stat.h>	// for POSIX environment
#endif
#include "encoder.hpp"
#include "../../manah/memory.hpp"		// manah.FastArenaObject
#include "../../manah/gap-buffer.hpp"	// manah.GapBuffer


namespace ascension {

	namespace kernel {

		namespace internal {
			/// @internal Interface for objects which manage the set of points.
			template<class PointType> class IPointCollection {
			public:
				/// Adds the newly created point.
				virtual void addNewPoint(PointType& point) = 0;
				/// Deletes the point about to be destroyed (@a point is in its destructor call).
				virtual void removePoint(PointType& point) = 0;
			};
		} // namespace internal

		class Document;
		struct DocumentPartition;

		/// Content type of a document partition.
		typedef ulong ContentType;

		// special content types
		const ContentType
			DEFAULT_CONTENT_TYPE = 0UL,					///< Default content type.
			PARENT_CONTENT_TYPE = 0xFFFFFFFFUL,			///< Type of the parent (means "transition source") content.
			UNDETERMINED_CONTENT_TYPE = 0xFFFFFFFEUL;	///< Type of Undetermined (not calculated) content.

		/**
		 * Value represent a newline in document. @c NLF_RAW_VALUE and @c NLF_DOCUMENT_INPUT are
		 * special values indicate how to interpret newlines during any text I/O.
		 * @see getNewlineStringLength, getNewlineString, Document, ASCENSION_DEFAULT_NEWLINE
		 */
		enum Newline {
			NLF_LINE_FEED,				///< Line feed. Standard of Unix (Lf, U+000A).
			NLF_CARRIAGE_RETURN,		///< Carriage return. Old standard of Macintosh (Cr, U+000D).
			NLF_CR_LF,					///< CR+LF. Standard of Windows (CrLf, U+000D U+000A).
			NLF_NEXT_LINE,				///< Next line. Standard of EBCDIC-based OS (U+0085).
			NLF_LINE_SEPARATOR,			///< Line separator (U+2028).
			NLF_PARAGRAPH_SEPARATOR,	///< Paragraph separator (U+2029).
			NLF_SPECIAL_VALUE_MASK	= 0x1000,
			/// Represents any NLF as the actual newline of the line (@c Document#Line#newline()).
			NLF_RAW_VALUE = 0 | NLF_SPECIAL_VALUE_MASK,
			/// Represents any NLF as the value of @c IDocumentInput#newline().
			NLF_DOCUMENT_INPUT = 1 | NLF_SPECIAL_VALUE_MASK
		};

		/**
		 * @c Position represents a position in the document by a line number and distance from start of line.
		 * @note This class is not derivable.
		 * @see Region, Point, EditPoint, viewers#VisualPoint, viewers#Caret
		 */
		class Position : public manah::FastArenaObject<Position> {
		public:
			length_t line;		///< Line number.
			length_t column;	///< Position in the line.
			static const Position ZERO_POSITION;	///< A position whose line and column number are zero.
			static const Position INVALID_POSITION;	///< Unused or invalid position.
		public:
			/// Constructor.
			explicit Position(length_t lineNumber = 0, length_t columnNumber = 0) throw() : line(lineNumber), column(columnNumber) {}
			/// Equality operator.
			bool operator==(const Position& rhs) const throw() {return line == rhs.line && column == rhs.column;}
			/// Unequality operator.
			bool operator!=(const Position& rhs) const throw() {return line != rhs.line || column != rhs.column;}
			/// Relational operator.
			bool operator<(const Position& rhs) const throw() {return line < rhs.line || (line == rhs.line && column < rhs.column);}
			/// Relational operator.
			bool operator<=(const Position& rhs) const throw() {return *this < rhs || *this == rhs;}
			/// Relational operator.
			bool operator>(const Position& rhs) const throw() {return line > rhs.line || (line == rhs.line && column > rhs.column);}
			/// Relational operator.
			bool operator>=(const Position& rhs) const throw() {return *this > rhs || *this == rhs;}
		};

		/**
		 * A region consists of two positions and represents a linear range in a document. There
		 * are no restriction about greater/less relationship between the two positions, but the
		 * region is called "normalized" when the first position is less than or equal to the second.
		 * @note This class is not derivable.
		 */
		class Region : public std::pair<Position, Position>, public manah::FastArenaObject<Region> {
		public:
			/// Default constructor.
			Region() throw() {}
			/// Constructor.
			Region(const Position& one, const Position& other) throw() : std::pair<Position, Position>(one, other) {}
			/// Constructor creates a region in a line.
			Region(length_t line, const std::pair<length_t, length_t>& columns) throw()
				: std::pair<Position, Position>(Position(line, columns.first), Position(line, columns.second)) {}
			/// Constructor creates an empty region.
			explicit Region(const Position& p) throw() : std::pair<Position, Position>(p, p) {}
			/// Returns an intersection of the two regions. Same as @c #getIntersection.
			Region operator&(const Region& rhs) const throw() {return getIntersection(rhs);}
			/// Returns a union of the two regions. Same as @c #getUnion.
			Region operator|(const Region& rhs) const {return getUnion(rhs);}
			/// Returns the beginning of the region.
			Position& beginning() throw() {return (first < second) ? first : second;}
			/// Returns the beginning of the region.
			const Position& beginning() const throw() {return (first < second) ? first : second;}
			/// Returns true if the region encompasses the other region.
			bool encompasses(const Region& other) const throw() {return beginning() <= other.beginning() && end() >= other.end();}
			/// Returns the end of the region.
			Position& end() throw() {return (first > second) ? first : second;}
			/// Returns the end of the region.
			const Position& end() const throw() {return (first > second) ? first : second;}
			/// Returns an intersection of the two regions. If the regions don't intersect, returns @c Region().
			Region getIntersection(const Region& other) const throw() {
				return intersectsWith(other) ? Region(std::max(beginning(), other.beginning()), std::min(end(), other.end())) : Region();}
			/// Returns a union of the two regions. If the two regions don't intersect, throws @c std#invalid_argument.
			Region getUnion(const Region& other) const {
				if(!intersectsWith(other)) throw std::invalid_argument("can't make a union."); return Region(beginning(), other.end());}
			/// Returns true if @a p is contained by the region.
			bool includes(const Position& p) const throw() {return p >= beginning() && p <= end();}
			/// Returns true if the region intersects with the other region.
			bool intersectsWith(const Region& other) const throw() {return includes(other.first) || includes(other.second);}
			/// Returns true if the region is empty.
			bool isEmpty() const throw() {return first == second;}
			/// Returns true if the region is normalized.
			bool isNormalized() const throw() {return first <= second;}
			/// Normalizes the region.
			Region& normalize() throw() {if(!isNormalized()) std::swap(first, second); return *this;}
		};

		/**
		 * A document partition.
		 * @see DocumentPartitioner#partition
		 */
		struct DocumentPartition {
			ContentType contentType;	///< Content type of the partition.
			Region region;				///< Region of the partition.
			/// Default constructor.
			DocumentPartition() throw() {}
			/// Constructor.
			DocumentPartition(ContentType type, const Region& r) throw() : contentType(type), region(r) {}
		};

		/**
		 * A changed content of the document.
		 * @see IDocumentListener, PositionUpdater
		 */
		class DocumentChange {
			MANAH_NONCOPYABLE_TAG(DocumentChange);
		public:
			/// Returns true if the modification is deletion or insertion.
			bool isDeletion() const throw() {return deletion_;}
			/// Returns the deleted region if the modification is deletion, otherwise the region of the inserted text.
			const Region& region() const throw() {return region_;}
		private:
			explicit DocumentChange(bool deletion, const Region& region) throw() : deletion_(deletion), region_(region) {}
			~DocumentChange() throw() {}
			bool deletion_;
			Region region_;
			friend class Document;
		};

		/**
		 * Exception represents the document is already disposed.
		 * @see Point
		 */
		class DisposedDocumentException : public std::runtime_error {
		public:
			DisposedDocumentException() :
				std::runtime_error("The document the object connecting to has been already disposed.") {}
		};

		/**
		 * A key of document property.
		 * @see Document#property, Document#setProperty
		 */
		class DocumentPropertyKey {
			MANAH_NONCOPYABLE_TAG(DocumentPropertyKey);
		public:
			/// Default constructor.
			DocumentPropertyKey() throw() {}
		};

		/**
		 * Interface for objects which are interested in lifecycle of the point.
		 * @see Point#addLifeCycleListener, Point#removeLifeCycleListener, IPointListener
		 */
		class IPointLifeCycleListener {
		protected:
			/// Destructor.
			virtual ~IPointLifeCycleListener() throw() {}
		private:
			/// The point was destroyed. After this, don't call @c Point#addLifeCycleListener.
			virtual void pointDestroyed() = 0;
			friend class Point;
		};

		/**
		 * A point represents a document position and adapts to the document change.
		 *
		 * When the document change occured, @c Point moves automatically as follows:
		 *
		 * - If text was inserted or deleted before the point, the point will move accordingly.
		 * - If text was inserted or deleted after the point, the point will not move.
		 * - If region includes the point was deleted, the point will move to the start (= end) of
		 *   the region.
		 * - If text was inserted at the point, the point will or will not move according to the
		 *   gravity.
		 *
		 * For details of gravity, see the description of @c updatePosition function.
		 *
		 * When the document was reset (by @c Document#resetContent), the all points move to the
		 * start of the document.
		 *
		 * Almost all methods of this or derived classes will throw @c DisposedDocumentException if
		 * the document is already disposed. Call @c #isDocumentDisposed to check if the document
		 * is exist or not.
		 *
		 * @see Position, Document, EditPoint, viewers#VisualPoint, viewers#Caret
		 */
		class Point {
			MANAH_UNASSIGNABLE_TAG(Point);
		public:
			// constructors
			explicit Point(Document& document, const Position& position = Position());
			Point(const Point& rhs);
			virtual ~Point() throw();
			// operators
			operator Position() throw();
			operator const Position() const throw();
			bool	operator==(const Point& rhs) const throw();
			bool	operator!=(const Point& rhs) const throw();
			bool	operator<(const Point& rhs) const throw();
			bool	operator<=(const Point& rhs) const throw();
			bool	operator>(const Point& rhs) const throw();
			bool	operator>=(const Point& rhs) const throw();
			// attributes
			bool			adaptsToDocument() const throw();
			void			adaptToDocument(bool adapt) throw();
			void			excludeFromRestriction(bool exclude);
			Document*		document() throw();
			const Document*	document() const throw();
			Direction		gravity() const throw();
			bool			isDocumentDisposed() const throw();
			bool			isExcludedFromRestriction() const throw();
			const Position&	position() const throw();
			void			setGravity(Direction gravity) throw();
			// listeners
			void	addLifeCycleListener(IPointLifeCycleListener& listener);
			void	removeLifeCycleListener(IPointLifeCycleListener& listener);
			// short-circuits
			length_t	columnNumber() const throw();
			ContentType	getContentType() const;
			length_t	lineNumber() const throw();
			// operations
			void	moveTo(const Position& to);
			void	moveTo(length_t line, length_t column);

		protected:
			Point&			operator=(const Position& rhs) throw();
			void			documentDisposed() throw();
			virtual void	doMoveTo(const Position& to);
			virtual void	normalize() const;
			virtual void	update(const DocumentChange& change);
			void			verifyDocument() const;

		private:
			Document* document_;
			Position position_;
			bool adapting_;
			bool excludedFromRestriction_;
			Direction gravity_;
			ascension::internal::Listeners<IPointLifeCycleListener> lifeCycleListeners_;
			friend class Document;
		};

		/// Thrown when the read only document is about to be modified.
		class ReadOnlyDocumentException : public IllegalStateException {
		public:
			/// Constructor.
			ReadOnlyDocumentException() : IllegalStateException("The document is readonly. Any edit process is denied.") {}
		};

		/**
		 * Thrown when the specified line or character position is outside of the document.
		 * @see BadRegionException
		 */
		class BadPositionException : public std::invalid_argument {
		public:
			/// Constructor.
			BadPositionException() : std::invalid_argument("The position is outside of the document.") {}
		};

		/**
		 * Thrown when the specified region intersects outside of the document.
		 * @see BadPositionException
		 */
		class BadRegionException : public BadPositionException {};

		/**
		 * Interface for objects which are interested in getting informed about change of bookmarks of the document.
		 * @see Bookmarker, Bookmarker#addListener, Bookmarker#removeListener
		 */
		class IBookmarkListener {
		private:
			/// The bookmark on @a line was set or removed.
			virtual void bookmarkChanged(length_t line) = 0;
			/// All bookmarks were removed.
			virtual void bookmarkCleared() = 0;
			friend class Bookmarker;
		};

		/**
		 * @c Bookmark manages bookmarks of the document.
		 * @see Document#bookmarker, Document#Line#isBookmarked
		 */
		class Bookmarker {
			MANAH_NONCOPYABLE_TAG(Bookmarker);
		public:
			void		addListener(IBookmarkListener& listener);
			void		clear() throw();
			length_t	getNext(length_t startLine, Direction direction) const;
			bool		isMarked(length_t line) const;
			void		mark(length_t line, bool set = true);
			void		removeListener(IBookmarkListener& listener);
			void		toggle(length_t line);
		private:
			explicit Bookmarker(Document& document) throw();
			Document& document_;
			ascension::internal::Listeners<IBookmarkListener> listeners_;
			friend class Document;
		};

		/**
		 * Provides information about a document input.
		 * @see Document
		 */
		class IDocumentInput {
		public:
			/// Destructor.
			virtual ~IDocumentInput() throw() {}
			/// Returns the MIBenum value of the encoding of the document input.
			virtual encoding::MIBenum encoding() const throw() = 0;
			/// Returns the location of the document input or an empty string.
			virtual String location() const throw() = 0;
			/// Returns the default newline of the document. The returned value can be neighter
			/// @c NLF_RAW_VALUE nor @c NLF_DOCUMENT_INPUT.
			virtual Newline newline() const throw() = 0;
		};

		/**
		 * A listener notified about the document change.
		 * @see DocumentUpdate, Document#addListener, Document#addPrenotifiedListener,
		 * Document#removeListener, Document#removePrenotifiedListener
		 */
		class IDocumentListener {
		private:
			/**
			 * The document is about to be changed.
			 * @param document the document
			 * @param change the modification content. @c change.region() may return an empty
			 * @return false to interrupt and cancel the modification
			 */
			virtual bool documentAboutToBeChanged(const Document& document, const DocumentChange& change) = 0;
			/**
			 * The text was deleted or inserted.
			 * @param document the document
			 * @param change the modification content. @c change.region() may return an empty
			 */
			virtual void documentChanged(const Document& document, const DocumentChange& change) = 0;
			friend class Document;
		};

		/**
		 * Interface for objects which are interested in getting informed about changes a document's state.
		 * @see Document#addStateListener, Document#removeStateListener
		 */
		class IDocumentStateListener {
		private:
			/// The accessible region of the document was changed.
			virtual void documentAccessibleRegionChanged(const Document& document) = 0;
			/// The modification flag of the document was changed.
			virtual void documentModificationSignChanged(const Document& document) = 0;
			/// The property has @c key associated with the document was changed.
			virtual void documentPropertyChanged(const Document& document, const DocumentPropertyKey& key) = 0;
			/// The read only mode of the document was changed.
			virtual void documentReadOnlySignChanged(const Document& document) = 0;
			friend class Document;
		};

		/**
		 * Interface for objects which are interested in getting informed about changes of a
		 * document's sequential edit.
		 * @see Document#EditSequence, Document#undo, Document#redo
		 */
		class ISequentialEditListener {
		private:
			/**
			 * The sequential is started.
			 * @param document the document
			 */
			virtual void documentSequentialEditStarted(const Document& document) = 0;
			/**
			 * The sequential edit is stopped.
			 * @param document the document
			 */
			virtual void documentSequentialEditStopped(const Document& document) = 0;
			/**
			 * The undo/redo operation is started.
			 * @param document the document
			 */
			virtual void documentUndoSequenceStarted(const Document& document) = 0;
			/**
			 * The undo/redo operation is stopped.
			 * @param document the document
			 * @param resultPosition preferable position to put the caret
			 */
			virtual void documentUndoSequenceStopped(const Document& document, const Position& resultPosition) = 0;
			friend class Document;
		};

		/**
		 * An @c IContentTypeInformationProvider provides the information about the document's content types.
		 * @see Document#setContentTypeInformation, Document#setContentTypeInformation
		 */
		class IContentTypeInformationProvider {
		public:
			/// Destructor.
			virtual ~IContentTypeInformationProvider() throw() {}
			/**
			 * Returns the identifier syntax for the specified content type.
			 * @param contentType the type of content
			 * @return the identifier syntax
			 */
			virtual const text::IdentifierSyntax& getIdentifierSyntax(ContentType contentType) const throw() = 0;
		};

		/**
		 * Interface for objects which are interested in getting informed about changes of a document's partitioning.
		 * @see DocumentPartitioner, Document#addPartitioningListener, Document#removePartitioningListener
		 */
		class IDocumentPartitioningListener {
		private:
			/**
			 * Document partitions are changed.
			 * @param changedRegion the region whose document partition are changed
			 */
			virtual void documentPartitioningChanged(const Region& changedRegion) = 0;
			friend class Document;
		};

		/**
		 * A document partitioner devides a document into disjoint text partitions.
		 * @see ContentType, Document, DocumentPartition, Document#partitioner,
		 * Document#setPartitioner, NullPartitioner
		 */
		class DocumentPartitioner {
		public:
			virtual			~DocumentPartitioner() throw();
			ContentType		contentType(const Position& at) const;
			Document*		document() throw();
			const Document*	document() const throw();
			void			partition(const Position& at, DocumentPartition& partition) const;
		protected:
			DocumentPartitioner() throw();
			void	notifyDocument(const Region& changedRegion);
		private:
			/// The document is about to be changed.
			virtual void documentAboutToBeChanged() throw() = 0;
			/**
			 * The document was changed.
			 * @param change the modification content
			 */
			virtual void documentChanged(const DocumentChange& change) throw() = 0;
			/**
			 * Returns the partition contains the specified position.
			 * @param at the position. this position is guaranteed to be inside of the document
			 * @param[out] partition the partition
			 */
			virtual void doGetPartition(const Position& at, DocumentPartition& partition) const throw() = 0;
			/**
			 * Called when the partitioner was connected to a document.
			 * There is not method called @c doUninstall, because a partitioner will be destroyed when disconnected.
			 */
			virtual void doInstall() throw() = 0;
		private:
			void install(Document& document) throw() {document_ = &document; doInstall();}
		private:
			Document* document_;
			friend class Document;
		};

		/// @c NullPartitioner always returns one partition covers a whole document.
		class NullPartitioner : public DocumentPartitioner {
		public:
			NullPartitioner() throw();
		private:
			void	documentAboutToBeChanged() throw();
			void	documentChanged(const DocumentChange& change) throw();
			void	doGetPartition(const Position& at, DocumentPartition& partition) const throw();
			void	doInstall() throw();
		private:
			DocumentPartition p_;
		};

		class DocumentCharacterIterator : public text::CharacterIterator,
			public StandardBidirectionalIteratorAdapter<DocumentCharacterIterator, CodePoint, CodePoint> {
		public:
			// constructors
			DocumentCharacterIterator() throw();
			DocumentCharacterIterator(const Document& document, const Position& position);
			DocumentCharacterIterator(const Document& document, const Region& region);
			DocumentCharacterIterator(const Document& document, const Region& region, const Position& position);
			DocumentCharacterIterator(const DocumentCharacterIterator& rhs) throw();
			// attributes
			const Document*	document() const throw();
			const String&	line() const throw();
			const Region&	region() const throw();
			void			setRegion(const Region& newRegion);
			const Position&	tell() const throw();
			// operation
			DocumentCharacterIterator&	seek(const Position& to);

			// CharacterIterator
			CodePoint	current() const throw();
			bool		hasNext() const throw();
			bool		hasPrevious() const throw();
		private:
			void								doAssign(const CharacterIterator& rhs);
			std::auto_ptr<CharacterIterator>	doClone() const;
			void								doFirst();
			void								doLast();
			bool								doEquals(const CharacterIterator& rhs) const;
			bool								doLess(const CharacterIterator& rhs) const;
			void								doNext();
			void								doPrevious();
		private:
			static const ConcreteTypeTag CONCRETE_TYPE_TAG_;
			const Document* document_;
			Region region_;
			const String* line_;
			Position p_;
		};

		// the documentation is at document.cpp
		class Document : virtual public internal::IPointCollection<Point>, virtual public texteditor::internal::ISessionElement {
			MANAH_NONCOPYABLE_TAG(Document);
		public:
			/// The property key for the title of the document.
			static const DocumentPropertyKey TITLE_PROPERTY;

			/// Content of a line.
			class Line : public manah::FastArenaObject<Line> {
			public:
				/// Returns true if the line is bookmarked.
				/// @deprecated 0.8
				bool isBookmarked() const throw() {return bookmarked_;}
				/// Returns true if the line has been changed.
				/// @deprecated 0.8
				bool isModified() const throw() {return operationHistory_ != 0;}
				/// Returns the newline of the line.
				Newline newline() const throw() {return newline_;}
				/// Returns the text of the line.
				const String& text() const throw() {return text_;}
			private:
				Line() throw() : operationHistory_(0), newline_(ASCENSION_DEFAULT_NEWLINE), bookmarked_(false) {}
				explicit Line(const String& text, Newline newline = ASCENSION_DEFAULT_NEWLINE, bool modified = false)
					: text_(text), operationHistory_(modified ? 1 : 0), newline_(newline), bookmarked_(false) {}
				String text_;
				ulong operationHistory_ : 28;
				Newline newline_ : 3;
				mutable bool bookmarked_ : 1;	// true if the line is bookmarked
#if (3 < 2 << NLF_COUNT)
#error "newline_ member is not allocated efficient buffer."
#endif
				friend class Document;
				friend class Bookmarker;
			};
			typedef manah::GapBuffer<Line*,
				manah::GapBuffer_DeletePointer<Line*> >	LineList;	///< List of lines.

			// constructors
			Document();
			virtual ~Document();
			// reconstruct
			virtual void	resetContent();
			// listeners and strategies
			void	addListener(IDocumentListener& listener);
			void	addPartitioningListener(IDocumentPartitioningListener& listener);
			void	addPrenotifiedListener(IDocumentListener& listener);
			void	addStateListener(IDocumentStateListener& listener);
			void	addSequentialEditListener(ISequentialEditListener& listener);
			void	removeListener(IDocumentListener& listener);
			void	removePartitioningListener(IDocumentPartitioningListener& listener);
			void	removePrenotifiedListener(IDocumentListener& listener);
			void	removeStateListener(IDocumentStateListener& listener);
			void	removeSequentialEditListener(ISequentialEditListener& listener);
			// attributes
			Bookmarker&					bookmarker() throw();
			const Bookmarker&			bookmarker() const throw();
			IDocumentInput*				input() const throw();
			bool						isModified() const throw();
			bool						isReadOnly() const throw();
			const DocumentPartitioner&	partitioner() const throw();
			const String*				property(const DocumentPropertyKey& key) const throw();
			texteditor::Session*		session() throw();
			const texteditor::Session*	session() const throw();
			void						setInput(IDocumentInput* newInput, bool delegateOwnership) throw();
			void						setModified(bool modified = true) throw();
			void						setPartitioner(std::auto_ptr<DocumentPartitioner> newPartitioner) throw();
			void						setProperty(const DocumentPropertyKey& key, const String& property);
			void						setReadOnly(bool readOnly = true);
			// contents
			Region			accessibleRegion() const throw();
			const Line&		getLineInformation(length_t line) const;
			length_t		length(Newline newline = NLF_RAW_VALUE) const;
			const String&	line(length_t line) const;
			length_t		lineLength(length_t line) const;
			length_t		lineOffset(length_t line, Newline newline = NLF_RAW_VALUE) const;
			length_t		numberOfLines() const throw();
			Region			region() const throw();
			std::size_t		revisionNumber() const throw();
			// content type information
			IContentTypeInformationProvider&	contentTypeInformation() const throw();
			void								setContentTypeInformation(std::auto_ptr<IContentTypeInformationProvider> newProvider) throw();
			// manipulations
			Position	erase(const Region& region);
			Position	erase(const Position& pos1, const Position& pos2);
			Position	insert(const Position& position, const String& text);
			Position	insert(const Position& position, const Char* first, const Char* last);
			bool		isChanging() const throw();
			// undo/redo
			void		clearUndoBuffer();
			bool		isRecordingOperations() const throw();
			std::size_t	numberOfUndoableEdits() const throw();
			std::size_t	numberOfRedoableEdits() const throw();
			void		recordOperations(bool record);
			bool		redo();
			bool		undo();
			// sequential edit
			void	beginSequentialEdit() throw();
			void	endSequentialEdit() throw();
			bool	isSequentialEditing() const throw();
			// narrowing
			bool	isNarrowed() const throw();
			void	narrow(const Region& region);
			void	widen();
			// iterations
			DocumentCharacterIterator	begin() const throw();
			DocumentCharacterIterator	end() const throw();

			// overridables
		protected:
			virtual void	doResetContent();

		private:
			void		doSetModified(bool modified) throw();
			Position	eraseText(const Region& region);
			bool		fireDocumentAboutToBeChanged(const DocumentChange& c) throw();
			void		fireDocumentChanged(const DocumentChange& c, bool updateAllPoints = true) throw();
			void		initialize();
			Position	insertText(const Position& position, const Char* first, const Char* last);
			void		partitioningChanged(const Region& changedRegion) throw();
			void		updatePoints(const DocumentChange& change) throw();
#ifdef ASCENSION_WINDOWS
			static ::UINT	translateSpecialCodePage(::UINT codePage);
#endif /* ASCENSION_WINDOWS */
			// internal::ISessionElement
			void	setSession(texteditor::Session& session) throw() {session_ = &session;}
			// internal::IPointCollection<Point>
			void	addNewPoint(Point& point) {points_.insert(&point);}
			void	removePoint(Point& point) {points_.erase(&point);}

		private:
			class UndoManager;
			class DefaultContentTypeInformationProvider : virtual public IContentTypeInformationProvider {
			public:
				~DefaultContentTypeInformationProvider() throw() {}
				const text::IdentifierSyntax& getIdentifierSyntax(ContentType) const throw() {return syntax_;}
			private:
				text::IdentifierSyntax syntax_;
			};

			class ModificationGuard {
				MANAH_UNASSIGNABLE_TAG(ModificationGuard);
			public:
				ModificationGuard(Document& document) throw() : document_(document) {document_.changing_ = true;}
				~ModificationGuard() throw() {document_.changing_= false;}
			private:
				Document& document_;
			};
			friend class ModificationGuard;

			texteditor::Session* session_;
			ascension::internal::StrategyPointer<IDocumentInput> input_;
			std::auto_ptr<DocumentPartitioner> partitioner_;
			std::auto_ptr<Bookmarker> bookmarker_;
			std::auto_ptr<IContentTypeInformationProvider> contentTypeInformationProvider_;
			bool readOnly_;
			LineList lines_;
			length_t length_;
			std::size_t revisionNumber_, lastUnmodifiedRevisionNumber_;
			std::set<Point*> points_;
			UndoManager* undoManager_;
			std::map<const DocumentPropertyKey*, String*> properties_;
			bool onceUndoBufferCleared_, recordingOperations_, changing_;

			std::pair<Position, Point*>* accessibleArea_;

			std::list<IDocumentListener*> listeners_, prenotifiedListeners_;
			ascension::internal::Listeners<IDocumentStateListener> stateListeners_;
			ascension::internal::Listeners<ISequentialEditListener> sequentialEditListeners_;
			ascension::internal::Listeners<IDocumentPartitioningListener> partitioningListeners_;

			friend class DocumentPartitioner;
		};

		/**
		 * @c std#basic_streambuf implementation for @c Document. This supports both input and
		 * output streams. Seeking is not supported. Virtual methods this class overrides are:
		 * - @c overflow
		 * - @c sync
		 * - @c uflow
		 * - @c underflow
		 * Destructor automatically flushes the internal buffer.
		 * @note This class is not intended to be subclassed.
		 */
		class DocumentBuffer : public std::basic_streambuf<Char> {
		public:
			explicit DocumentBuffer(Document& document, const Position& initialPosition = Position::ZERO_POSITION,
				Newline newline = NLF_RAW_VALUE, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out);
			~DocumentBuffer() throw();
			const Position&	tell() const throw();
		private:
			int_type	overflow(int_type c);
			int			sync();
			int_type	uflow();
			int_type	underflow();
		private:
			Document& document_;
			const Newline newline_;
			const std::ios_base::openmode mode_;
			Position current_;
			char_type buffer_[8192];
		};

		/// Input stream for @c Document.
		class DocumentInputStream : public std::basic_istream<Char> {
		public:
			explicit DocumentInputStream(Document& document,
				const Position& initialPosition = Position::ZERO_POSITION, Newline newline = NLF_RAW_VALUE);
			DocumentBuffer* rdbuf() const;
		private:
			DocumentBuffer buffer_;
		};

		/// Output stream for @c Document.
		class DocumentOutputStream : public std::basic_ostream<Char> {
		public:
			explicit DocumentOutputStream(Document& document,
				const Position& initialPosition = Position::ZERO_POSITION, Newline newline = NLF_RAW_VALUE);
			DocumentBuffer* rdbuf() const;
		private:
			DocumentBuffer buffer_;
		};

		/// Stream for @c Document.
		class DocumentStream : public std::basic_iostream<Char> {
		public:
			explicit DocumentStream(Document& document,
				const Position& initialPosition = Position::ZERO_POSITION, Newline newline = NLF_RAW_VALUE);
			DocumentBuffer* rdbuf() const;
		private:
			DocumentBuffer buffer_;
		};

		// free functions related to document
		template<typename ForwardIterator>
		Newline						eatNewline(ForwardIterator first, ForwardIterator last);
		length_t					getAbsoluteOffset(const Document& document, const Position& at, bool fromAccessibleStart);
		const Char*					getNewlineString(Newline newline);
		length_t					getNewlineStringLength(Newline newline);
		template<typename ForwardIterator>
		length_t					getNumberOfLines(ForwardIterator first, ForwardIterator last);
		length_t					getNumberOfLines(const String& text) throw();
		bool						isLiteralNewline(Newline newline) throw();
		std::basic_istream<Char>&	readDocumentFromStream(std::basic_istream<Char>& in, Document& document, const Position& at);
		Position					updatePosition(const Position& position, const DocumentChange& change, Direction gravity) throw();
		std::basic_ostream<Char>&	writeDocumentToStream(std::basic_ostream<Char>& out,
										const Document& document, const Region& region, Newline newline = NLF_RAW_VALUE);

		// free functions output a primitive into stream
		template<typename Element, typename Traits>
		std::basic_ostream<Element, Traits>& operator<<(std::basic_ostream<Element, Traits>& out, const Position& value);
		template<typename Element, typename Traits>
		std::basic_ostream<Element, Traits>& operator<<(std::basic_ostream<Element, Traits>& out, const Region& value);

		/// Provides features about file-bound document.
		namespace fileio {
			/// Character type for file names. This is equivalent to
			/// @c ASCENSION_FILE_NAME_CHARACTER_TYPE configuration symbol.
			typedef ASCENSION_FILE_NAME_CHARACTER_TYPE Char;
			/// 
			typedef std::basic_string<Char> String;

			/// File I/O exception.
			class IOException : public std::runtime_error {
			public:
				/// Error types.
				enum Type {
					/// The specified file is not found.
					FILE_NOT_FOUND,
					/// The specified encoding is invalid.
					INVALID_ENCODING,
					/// The specified newline is based on Unicode but the encoding is not Unicode.
					INVALID_NEWLINE,
					/// The encoding failed for unmappable character.
					/// @see encoding#Encoder#UNMAPPABLE_CHARACTER
					UNMAPPABLE_CHARACTER,
					/// The encoding failed for malformed input.
					/// @see encoding#Encoder#MALFORMED_INPUT
					MALFORMED_INPUT,
					/// Failed for out of memory.
					OUT_OF_MEMORY,
					/// The file to be opend is too huge.
					HUGE_FILE,
					/// Tried to write the read only document. If the disk file is read only, POSIX
					/// @c errno is set to @c BBADF, or Win32 @c GetLastError returns
					/// @c ERROR_FILE_READ_ONLY.
					READ_ONLY_MODE,
					/// The file is read only and not writable.
					UNWRITABLE_FILE,
					/// Failed to create the temporary file for writing.
					CANNOT_CREATE_TEMPORARY_FILE,
					/// Failed to write to the file and the file was <strong>lost</strong>.
					LOST_DISK_FILE,
					/// A platform-dependent error whose detail can be obtained by POSIX @c errno
					/// or Win32 @c GetLastError.
					PLATFORM_DEPENDENT_ERROR
				};
			public:
				/// Constructor.
				explicit IOException(Type type) throw() : std::runtime_error(""), type_(type) {}
				/// Returns the error type.
				Type type() const throw() {return type_;}
			private:
				Type type_;
			};

			class TextFileDocumentInput;

			/**
			 * Interface for objects which are interested in getting informed about changes of @c FileBinder.
			 * @see FileBinder#addListener, FileBinder#removeListener
			 */
			class IFilePropertyListener {
			private:
				/// The encoding or newline of the bound file was changed.
				virtual void fileEncodingChanged(const TextFileDocumentInput& textFile) = 0;
				/// The the name of the bound file was changed.
				virtual void fileNameChanged(const TextFileDocumentInput& textFile) = 0;
				friend class TextFileDocumentInput;
			};

			/// Interface for objects which should handle the unexpected time stamp of the file.
			class IUnexpectedFileTimeStampDirector {
			public:
				/// Context.
				enum Context {
					FIRST_MODIFICATION,	///< The call is for the first modification of the document.
					OVERWRITE_FILE,		///< The call is for overwriting the file.
					CLIENT_INVOCATION	///< The call was invoked by @c Document#checkTimeStamp.
				};
			private:
				/**
				 * Handles.
				 * @param document the document
				 * @param context the context
				 * @retval true	the process will be continued and the internal time stamp will be updated
				 * @retval false the process will be aborted
				 */
				virtual bool queryAboutUnexpectedDocumentFileTimeStamp(Document& document, Context context) throw() = 0;
				friend class TextFileDocumentInput;
			};
#if 0
			/// Interface for objects which are interested in getting informed about progression of file IO.
			class IFileIOProgressMonitor {
			public:
				enum ProcessType {};
			private:
				/**
				 * This method will be called when 
				 * @param type the type of the precess
				 * @param processedAmount the amount of the data had processed
				 * @param totalAmount the total amount of the data to process
				 */
				virtual void onProgress(ProcessType type, ULONGLONG processedAmount, ULONGLONG totalAmount) = 0;
				/// Returns the internal number of lines.
				virtual length_t queryIntervalLineCount() const = 0;
				/// Releases the object.
				virtual void release() = 0;
				friend class Document;
			};
#endif
			/**
			 * @c std#basic_streambuf implementation of the text file with encoding conversion.
			 * @note This class is not intended to be subclassed.
			 */
			class TextFileStreamBuffer : public std::basic_streambuf<ascension::Char> {
				MANAH_NONCOPYABLE_TAG(TextFileStreamBuffer);
			public:
				TextFileStreamBuffer(const String& fileName, std::ios_base::openmode mode,
					encoding::MIBenum encoding, encoding::Encoder::Policy encodingPolicy, bool writeByteOrderMark);
				~TextFileStreamBuffer();
				TextFileStreamBuffer* close();
				encoding::MIBenum encoding() const throw();
				bool isOpen() const throw();
			private:
				int_type	overflow(int_type c /* = traits_type::eof() */);
				int_type	pbackfail(int_type c /* = traits_type::eof() */);
				int			sync();
				int_type	underflow();
			private:
				typedef std::basic_streambuf<ascension::Char> Base;
#ifdef ASCENSION_WINDOWS
				::HANDLE fileHandle_, fileMapping_;
#else // ASCENSION_POSIX
				int fileDescriptor_;
#endif
				struct {
					const uchar* first;
					const uchar* last;
					const uchar* current;
				} inputMapping_;
				encoding::Encoder* encoder_;
				encoding::Encoder::State encodingState_;
				ascension::Char ucsBuffer_[8192];
			};

			class TextFileDocumentInput : virtual public IDocumentInput,
					virtual public IDocumentListener, virtual public IDocumentStateListener {
				MANAH_NONCOPYABLE_TAG(TextFileDocumentInput);
			public:
				/// The structure used to represent a file time.
#ifdef ASCENSION_WINDOWS
				typedef ::FILETIME Time;
#else // ASCENSION_POSIX
				typedef ::time_t Time;
#endif

				/// Option flags for @c FileBinder#write and @c FileBinder#writeRegion.
				struct WriteParameters {
					encoding::MIBenum encoding;					///< The MIBenum value of the encoding.
					Newline newline;							///< The newline.
					encoding::Encoder::Policy encodingPolicy;	///< The policy of encodings.
					enum Option {
						WRITE_UNICODE_BYTE_ORDER_SIGNATURE	= 0x01,	///< Writes a UTF byte order signature.
						BY_COPYING							= 0x02,	///< Not implemented.
						CREATE_BACKUP						= 0x04	///< Creates backup files.
					};
					manah::Flags<Option> options;	///< Miscellaneous options.
				};
				/// Lock modes for opened file.
				struct LockMode {
					enum {
						DONT_LOCK		= 0x00,	///< Does not lock.
						SHARED_LOCK		= 0x01,	///< Uses shared lock.
						EXCLUSIVE_LOCK	= 0x02	///< Uses exclusive lock.
					} type;
					bool onlyAsEditing;	///< If true, the lock will not be performed unless modification occurs.
				};
			public:
				explicit TextFileDocumentInput(Document& document);
				~TextFileDocumentInput() throw();
				bool			checkTimeStamp();
				const Document&	document() const throw();
				const LockMode&	lockMode() const throw();
				// listener
				void	addListener(IFilePropertyListener& listener);
				void	removeListener(IFilePropertyListener& listener);
				// bound file name
				String	extensionName() const throw();
				bool	isOpen() const throw();
				String	name() const throw();
				String	pathName() const throw();
				// encodings
				void	setEncoding(encoding::MIBenum mib);
				void	setNewline(Newline newline);
				// I/O
				void	close();
				bool	open(const String& fileName, const LockMode& lockMode,
							encoding::MIBenum encoding, encoding::Encoder::Policy encodingPolicy,
							IUnexpectedFileTimeStampDirector* unexpectedTimeStampDirector = 0);
				bool	write(const String& fileName, const WriteParameters& params);
				bool	writeRegion(const String& fileName, const Region& region, const WriteParameters& params, bool append);
				// IDocumentInput
				encoding::MIBenum	encoding() const throw();
				ascension::String	location() const throw();
				Newline				newline() const throw();
			private:
				bool	lock() throw();
				bool	unlock() throw();
				bool	verifyTimeStamp(bool internal, Time& newTimeStamp) throw();
				// IDocumentListener
				bool	documentAboutToBeChanged(const Document& document, const DocumentChange& change);
				void	documentChanged(const Document& document, const DocumentChange& change);
				// IDocumentStateListener
				void	documentAccessibleRegionChanged(const Document& document);
				void	documentModificationSignChanged(const Document& document);
				void	documentPropertyChanged(const Document& document, const DocumentPropertyKey& key);
				void	documentReadOnlySignChanged(const Document& document);
			private:
				Document& document_;
				String fileName_;
				encoding::MIBenum encoding_;
				Newline newline_;
				LockMode lockMode_;
#ifdef ASCENSION_WINDOWS
				::HANDLE
#else // ASCENSION_POSIX
				int
#endif
					lockingFile_;
				std::size_t savedDocumentRevision_;
				Time userLastWriteTime_, internalLastWriteTime_;
				ascension::internal::Listeners<IFilePropertyListener> listeners_;
				IUnexpectedFileTimeStampDirector* timeStampDirector_;
			};

			// free functions related file path name
			String	canonicalizePathName(const Char* pathName);
			bool	comparePathNames(const Char* s1, const Char* s2);
		} // namespace fileio


// inline implementation ////////////////////////////////////////////////////

/// Write a @c Position into the output stream.
template<typename Element, typename Traits>
inline std::basic_ostream<Element, Traits>& operator<<(std::basic_ostream<Element, Traits>& out, const Position& value) {
	const std::ctype<Element>& ct = std::use_facet<std::ctype<Element> >(out.getloc());
	std::basic_ostringstream<Element, Traits> s;
	s.flags(out.flags());
	s.imbue(out.getloc());
	s.precision(out.precision());
	s << ct.widen('(') << value.line << ct.widen(',') << value.column << ct.widen(')');
	return out << s.str().c_str();
}

/// Write a @c Region into the output stream.
template<typename Element, typename Traits>
inline std::basic_ostream<Element, Traits>& operator<<(std::basic_ostream<Element, Traits>& out, const Region& value) {
	const std::ctype<Element>& ct = std::use_facet<std::ctype<Element> >(out.getloc());
	std::basic_ostringstream<Element, Traits> s;
	s.flags(out.flags());
	s.imbue(out.getloc());
	s.precision(out.precision());
	s << value.first << ct.widen('-') << value.second;
	return out << s.str().c_str();
}

/**
 * Returns the newline at the beginning of the specified buffer.
 * @param first the beginning of the buffer
 * @param last the end of the buffer
 * @return the newline or @c NLF_RAW_VALUE if the beginning of the buffer is not line break
 */
template<typename ForwardIterator>
inline Newline eatNewline(ForwardIterator first, ForwardIterator last) {
	switch(*first) {
	case LINE_FEED:				return NLF_LINE_FEED;
	case CARRIAGE_RETURN:		return (++first != last && *first == LINE_FEED) ? NLF_CR_LF : NLF_CARRIAGE_RETURN;
	case NEXT_LINE:				return NLF_NEXT_LINE;
	case LINE_SEPARATOR:		return NLF_LINE_SEPARATOR;
	case PARAGRAPH_SEPARATOR:	return NLF_PARAGRAPH_SEPARATOR;
	default:					return NLF_RAW_VALUE;
	}
}

/**
 * Returns the null-terminated string represents the specified newline.
 * @param newline the newline
 * @return the string
 * @throw std#invalid_argument @a newline is invalid
 * @see #getNewlineStringLength
 */
inline const Char* getNewlineString(Newline newline) {
	switch(newline) {
	case NLF_LINE_FEED:				return L"\n";
	case NLF_CARRIAGE_RETURN:		return L"\r";
	case NLF_CR_LF:					return L"\r\n";
	case NLF_NEXT_LINE:				return L"\x0085";
	case NLF_LINE_SEPARATOR:		return L"\x2028";
	case NLF_PARAGRAPH_SEPARATOR:	return L"\x2029";
	default:						throw std::invalid_argument("unknown newline specified.");
	}
}

/**
 * Returns the length of the string represents the specified newline.
 * @param newline the newline
 * @return the length
 * @throw std#invalid_argument @a newline is invalid
 * @see #getNewlineString
 */
inline length_t getNewlineStringLength(Newline newline) {
	switch(newline) {
	case NLF_LINE_FEED:
	case NLF_CARRIAGE_RETURN:
	case NLF_NEXT_LINE:
	case NLF_LINE_SEPARATOR:
	case NLF_PARAGRAPH_SEPARATOR:
		return 1;
	case NLF_CR_LF:
		return 2;
	default:
		throw std::invalid_argument("unknown newline specified.");
	}
}

/**
 * Returns the number of lines in the specified character sequence.
 * @param first the beginning of the character sequence
 * @param last the end of the character sequence
 * @return the number of lines. zero if and only if the input sequence is empty
 */
template<typename ForwardIterator>
inline length_t getNumberOfLines(ForwardIterator first, ForwardIterator last) {
	if(first == last)
		return 0;
	length_t lines = 1;
	while(true) {
		first = std::find_first_of(first, last, LINE_BREAK_CHARACTERS, endof(LINE_BREAK_CHARACTERS));
		if(first == last)
			break;
		++lines;
		if(*first == CARRIAGE_RETURN) {
			if(++first == last)
				break;
			else if(*first == LINE_FEED)
				++first;
		} else
			++first;
	}
	return lines;
}

/**
 * Returns the number of lines in the specified text.
 * @param text the text string
 * @return the number of lines
 */
inline length_t getNumberOfLines(const String& text) throw() {return getNumberOfLines(text.begin(), text.end());}

/// Returns true if the given newline value is a literal.
inline bool isLiteralNewline(Newline newline) throw() {return newline >= NLF_LINE_FEED && newline <= NLF_PARAGRAPH_SEPARATOR;}

/// Conversion operator for convenience.
inline Point::operator Position() throw() {return position_;}
/// Conversion operator for convenience.
inline Point::operator const Position() const throw() {return position_;}
/**
 * Protected assignment operator moves the point to @a rhs.
 * @see #moveTo
 */
inline Point& Point::operator=(const Position& rhs) throw() {position_ = rhs; return *this;}
/// Equality operator.
inline bool Point::operator==(const Point& rhs) const throw() {return position() == rhs.position();}
/// Unequality operator.
inline bool Point::operator!=(const Point& rhs) const throw() {return !(*this == rhs);}
/// Less-than operator.
inline bool Point::operator<(const Point& rhs) const throw() {return position() < rhs.position();}
/// Less-than-or-equal-to operator.
inline bool Point::operator<=(const Point& rhs) const throw() {return *this < rhs || *this == rhs;}
/// Greater-than operator.
inline bool Point::operator>(const Point& rhs) const throw() {return !(*this >= rhs);}
/// Greater-than-or-equal-to operator.
inline bool Point::operator>=(const Point& rhs) const throw() {return !(*this > rhs);}
/// Returns true if the point is adapting to the document change.
inline bool Point::adaptsToDocument() const throw() {return adapting_;}
/// Adapts the point to the document change.
inline void Point::adaptToDocument(bool adapt) throw() {adapting_ = adapt;}
/// Returns the column.
inline length_t Point::columnNumber() const throw() {return position_.column;}
/// Returns the document or @c null if the document is already disposed.
inline Document* Point::document() throw() {return document_;}
/// Returns the document or @c null if the document is already disposed.
inline const Document* Point::document() const throw() {return document_;}
/// Called when the document is disposed.
inline void Point::documentDisposed() throw() {document_ = 0;}
/// ...
inline void Point::excludeFromRestriction(bool exclude) {verifyDocument(); if(excludedFromRestriction_ = exclude) normalize();}
/// Returns the content type of the document partition contains the point.
inline ContentType Point::getContentType() const {verifyDocument(); return document_->partitioner().contentType(*this);}
/// Returns the gravity.
inline Direction Point::gravity() const throw() {return gravity_;}
/// Returns true if the document is already disposed.
inline bool Point::isDocumentDisposed() const throw() {return document_ == 0;}
/// Returns true if the point can't enter the inaccessible area of the document.
inline bool Point::isExcludedFromRestriction() const throw() {return excludedFromRestriction_;}
/// Returns the line number.
inline length_t Point::lineNumber() const throw() {return position_.line;}
/// Moves to the specified position.
inline void Point::moveTo(length_t line, length_t column) {moveTo(Position(line, column));}
/// Returns the position.
inline const Position& Point::position() const throw() {return position_;}
/// Sets the gravity.
inline void Point::setGravity(Direction gravity) throw() {verifyDocument(); gravity_ = gravity;}
/// Throws @c DisposedDocumentException if the document is already disposed.
inline void Point::verifyDocument() const {if(isDocumentDisposed()) throw DisposedDocumentException();}


/// Returns the accessible region of the document. The returned region is normalized.
/// @see #region
inline Region Document::accessibleRegion() const throw() {
	return (accessibleArea_ != 0) ? Region(accessibleArea_->first, *accessibleArea_->second) : region();}

/**
 * Registers the document partitioning listener with the document.
 * @param listener the listener to be registered
 * @throw std#invalid_argument @a listener is already registered
 */
inline void Document::addPartitioningListener(IDocumentPartitioningListener& listener) {partitioningListeners_.add(listener);}

/**
 * Registers the sequential edit listener.
 * @param listener the listener to be registered
 * @throw std#invalid_argument @a listener is already registered
 */
inline void Document::addSequentialEditListener(ISequentialEditListener& listener) {sequentialEditListeners_.add(listener);}

/**
 * Registers the state listener.
 * @param listener the listener to be registered
 * @throw std#invalid_argument @a listener is already registered
 */
inline void Document::addStateListener(IDocumentStateListener& listener) {stateListeners_.add(listener);}

/// Returns the @c DocumentCharacterIterator addresses the beginning of the document.
inline DocumentCharacterIterator Document::begin() const throw() {return DocumentCharacterIterator(*this, Position::ZERO_POSITION);}

/// Returns the bookmarker of the document.
inline Bookmarker& Document::bookmarker() throw() {return *bookmarker_;}

/// Returns the bookmarker of the document.
inline const Bookmarker& Document::bookmarker() const throw() {return *bookmarker_;}

/// Returns the content information provider.
inline IContentTypeInformationProvider& Document::contentTypeInformation() const throw() {return *contentTypeInformationProvider_;}

/// Returns the @c DocumentCharacterIterator addresses the end of the document.
inline DocumentCharacterIterator Document::end() const throw() {return DocumentCharacterIterator(*this, region().second);}

/// @see #erase(const Region&)
inline Position Document::erase(const Position& pos1, const Position& pos2) {return erase(Region(pos1, pos2));}

/**
 * Returns the information of the specified line.
 * @param line the line
 * @return the information about @a line
 * @throw BadPostionException @a line is outside of the document
 */
inline const Document::Line& Document::getLineInformation(length_t line) const {
	if(line >= lines_.size()) throw BadPositionException(); return *lines_[line];}

#if 0
inline Document::LineIterator Document::getLineIterator(length_t line) const {
	assertValid();
	if(line >= lines_.getSize())
		throw BadPositionException();
	return lines_.begin() + line;
}
#endif

/// Returns the document input or @c null.
inline IDocumentInput* Document::input() const throw() {return input_.get();}

/**
 * Inserts the text at the specified position.
 *
 * The modification flag is set when this method is called. However, if the position is inaccessible area
 * of the document, the insertion is not performed and the modification flag is not changed.
 *
 * This method calls listeners' @c IDocumentListener#documentChanged methods.
 * @param position the position
 * @param text the text
 * @return the position to where the caret will move
 * @throw ReadOnlyDocumentException the document is read only
 */
inline Position Document::insert(const Position& position, const String& text) {
	return insert(position, text.data(), text.data() + text.length());}

/// Returns true if the document is in changing.
inline bool Document::isChanging() const throw() {return changing_;}

/**
 * Returns true if the document has been modified.
 * @see #setModified, IDocumentStateListener#documentModificationSignChanged
 */
inline bool Document::isModified() const throw() {return revisionNumber() != lastUnmodifiedRevisionNumber_;}

/**
 * Returns true if the document is narrowed.
 * @see #narrow, #widen
 */
inline bool Document::isNarrowed() const throw() {return accessibleArea_ != 0;}

/**
 * Returns true if the document is read only.
 * @see ReadOnlyDocumentException, #setReadOnly
 */
inline bool Document::isReadOnly() const throw() {return readOnly_;}

/**
 * Returns true if the document is recording the operations for undo/redo.
 * @see #recordOperations, #numberOfUndoableEdits, #numberOfRedoableEdits
 */
inline bool Document::isRecordingOperations() const throw() {return recordingOperations_;}

/**
 * Returns the text of the specified line.
 * @param line the line
 * @return the text
 * @throw BadPostionException @a line is outside of the document
 */
inline const String& Document::line(length_t line) const {return getLineInformation(line).text_;}

/**
 * Returns the length of the specified line. The line break is not included.
 * @param line the line
 * @return the length of @a line
 * @throw BadLocationException @a line is outside of the document
 */
inline length_t Document::lineLength(length_t line) const {return this->line(line).length();}

/// Returns the number of lines in the document.
inline length_t Document::numberOfLines() const throw() {return lines_.size();}

/// Returns the document partitioner of the document.
inline const DocumentPartitioner& Document::partitioner() const throw() {
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
inline void Document::partitioningChanged(const Region& changedRegion) throw() {
	partitioningListeners_.notify<const Region&>(&IDocumentPartitioningListener::documentPartitioningChanged, changedRegion);}

/**
 * Returns the property associated with the document.
 * @param key the key of the property
 * @return the property value or @c null if the specified property is not registered
 * @see #setProperty
 */
inline const String* Document::property(const DocumentPropertyKey& key) const throw() {
	const std::map<const DocumentPropertyKey*, String*>::const_iterator i(properties_.find(&key));
	return (i != properties_.end()) ? i->second : 0;
}

/// Returns the entire region of the document. The returned region is normalized.
/// @see #accessibleRegion
inline Region Document::region() const throw() {
	return Region(Position::ZERO_POSITION, Position(numberOfLines() - 1, lineLength(numberOfLines() - 1)));}

/**
 * Removes the document partitioning listener from the document.
 * @param listener the listener to be removed
 * @throw std#invalid_argument @a listener is not registered
 */
inline void Document::removePartitioningListener(IDocumentPartitioningListener& listener) {partitioningListeners_.remove(listener);}

/**
 * Removes the sequential edit listener.
 * @param listener the listener to be removed
 * @throw std#invalid_argument @a listener is not registered
 */
inline void Document::removeSequentialEditListener(ISequentialEditListener& listener) {sequentialEditListeners_.remove(listener);}

/**
 * Removes the state listener.
 * @param listener the listener to be removed
 * @throw std#invalid_argument @a listener is not registered
 */
inline void Document::removeStateListener(IDocumentStateListener& listener) {stateListeners_.remove(listener);}

/// Returns the revision number.
inline std::size_t Document::revisionNumber() const throw() {return revisionNumber_;}

/// Returns the session to which the document belongs.
inline texteditor::Session* Document::session() throw() {return session_;}

/// Returns the session to which the document belongs.
inline const texteditor::Session* Document::session() const throw() {return session_;}

/**
 * Sets the content type information provider.
 * @param newProvider the new content type information provider. the ownership will be transferred
 * to the callee. can be @c null
 */
inline void Document::setContentTypeInformation(std::auto_ptr<IContentTypeInformationProvider> newProvider) throw() {
	contentTypeInformationProvider_.reset((newProvider.get() != 0) ? newProvider.release() : new DefaultContentTypeInformationProvider);}

/**
 * Returns the content type of the partition contains the specified position.
 * @param at the position
 * @throw BadPositionException @a position is outside of the document
 * @throw IllegalStateException the partitioner is not connected to any document
 * @return the content type
 */
inline ContentType DocumentPartitioner::contentType(const Position& at) const {
	DocumentPartition p;
	partition(at, p);
	return p.contentType;
}

/// Returns the document to which the partitioner connects or @c null.
inline Document* DocumentPartitioner::document() throw() {return document_;}

/// Returns the document to which the partitioner connects or @c null.
inline const Document* DocumentPartitioner::document() const throw() {return document_;}

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
		throw BadPositionException();
	return doGetPartition(at, partition);
}

/// Returns the document.
inline const Document* DocumentCharacterIterator::document() const throw() {return document_;}

/// @see text#CharacterIterator#hasNext
inline bool DocumentCharacterIterator::hasNext() const throw() {return p_ != region_.second;}

/// @see text#CharacterIterator#hasPrevious
inline bool DocumentCharacterIterator::hasPrevious() const throw() {return p_ != region_.first;}

/// Returns the line.
inline const String& DocumentCharacterIterator::line() const throw() {return *line_;}

/// Returns the iteration region.
inline const Region& DocumentCharacterIterator::region() const throw() {return region_;}

/**
 * Moves to the specified position.
 * @param to the position. if this is outside of the iteration region, the start/end of the region will be used
 */
inline DocumentCharacterIterator& DocumentCharacterIterator::seek(const Position& to) {
	line_ = &document_->line((p_ = std::max(std::min(to, region_.second), region_.first)).line); return *this;}

/**
 * Sets the region of the iterator. The current position will adjusted.
 * @param newRegion the new region to set
 * @throw BadRegionException @a newRegion intersects outside of the document
 */
inline void DocumentCharacterIterator::setRegion(const Region& newRegion) {
	const Position e(document_->region().second);
	if(newRegion.first > e || newRegion.second > e)
		throw BadRegionException();
	if(!(region_ = newRegion).includes(p_))
		seek(p_);
}

/// Returns the document position the iterator addresses.
inline const Position& DocumentCharacterIterator::tell() const throw() {return p_;}

/// Returns the stored stream buffer.
inline DocumentBuffer* DocumentInputStream::rdbuf() const throw() {return const_cast<DocumentBuffer*>(&buffer_);}

/// Returns the stored stream buffer.
inline DocumentBuffer* DocumentOutputStream::rdbuf() const throw() {return const_cast<DocumentBuffer*>(&buffer_);}

/// Returns the stored stream buffer.
inline DocumentBuffer* DocumentStream::rdbuf() const throw() {return const_cast<DocumentBuffer*>(&buffer_);}

/// Returns the document.
inline const Document& fileio::TextFileDocumentInput::document() const throw() {return document_;}

/// @see IDocumentInput#encoding, #setEncoding
inline encoding::MIBenum fileio::TextFileDocumentInput::encoding() const throw() {return encoding_;}

/// Returns true if the document is bound to any file.
inline bool fileio::TextFileDocumentInput::isOpen() const throw() {return !fileName_.empty();}

/// Returns the file lock mode.
inline const fileio::TextFileDocumentInput::LockMode& fileio::TextFileDocumentInput::lockMode() const throw() {return lockMode_;}

/// @see IDocumentInput#newline, #setNewline
inline Newline fileio::TextFileDocumentInput::newline() const throw() {return newline_;}

/// Returns the file full name or an empty string if the document is not bound to any of the files.
inline fileio::String fileio::TextFileDocumentInput::pathName() const throw() {return fileName_;}

}} // namespace ascension.kernel

#endif /* !ASCENSION_DOCUMENT_HPP */
