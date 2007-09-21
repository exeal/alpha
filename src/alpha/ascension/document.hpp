/**
 * @file document.hpp
 * @author exeal
 * @date 2003-2006 (was EditDoc.h)
 * @date 2006-2007
 */

#ifndef ASCENSION_DOCUMENT_HPP
#define ASCENSION_DOCUMENT_HPP

#include <stack>
#include <set>
#include "session.hpp"
#include "encoder.hpp"
#include "../../manah/memory.hpp"		// manah.FastArenaObject
#include "../../manah/gap-buffer.hpp"	// manah.GapBuffer
#include "../../manah/win32/file.hpp"


namespace ascension {

	namespace presentation {class Presentation;}

	namespace text {

		namespace internal {
			class IOperation;
			class OperationUnit;
			/**
			 * Interface for objects which are managing the set of points.
			 * @see Document
			 */
			template<class PointType> class IPointCollection {
			private:
				/// Adds the newly created point.
				virtual void addNewPoint(PointType& point) = 0;
				/// Deletes the point about to be destroyed (@a point is in its destructor call).
				virtual void removePoint(PointType& point) = 0;
				friend typename PointType;
			};
			const Char NEWLINE_STRINGS[][7] = {
				L"", {LINE_FEED, 0}, {CARRIAGE_RETURN, 0}, {CARRIAGE_RETURN, LINE_FEED, 0},
				{NEXT_LINE, 0}, {LINE_SEPARATOR, 0}, {PARAGRAPH_SEPARATOR, 0}
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
		 * Newlines in document.
		 * @see getNewlineStringLength, getNewlineString, Document
		 */
		enum Newline {
			NLF_AUTO,	///< Indicates no-conversion or unspecified.
			NLF_LF,		///< Line feed. Standard of Unix (Lf, U+000A).
			NLF_CR,		///< Carriage return. Old standard of Macintosh (Cr, U+000D).
			NLF_CRLF,	///< CR+LF. Standard of Windows (CrLf, U+000D U+000A).
			NLF_NEL,	///< Next line. Standard of EBCDIC-based OS (U+0085).
			NLF_LS,		///< Line separator (U+2028).
			NLF_PS,		///< Paragraph separator (U+2029).
			NLF_COUNT
		};

		/**
		 * Representation of a line break. Defines how an interpreter treats line breaks. For
		 * example, @c Document#getLength method which calculates the length of the document refers
		 * this setting. Some methods can select prefer setting to <> its efficiency.
		 * @see Document#getLength, Document#getLineIndex, Document#writeToStream,
		 * EditPoint#getText, viewers#Selection#getText
		 */
		enum NewlineRepresentation {
			NLR_LINE_FEED,			///< Represents any NLF as LF (U+000D).
			NLR_CRLF,				///< Represents any NLF as CRLF (U+000D+000A).
			NLR_LINE_SEPARATOR,		///< Represents any NLF as LS (U+2028).
			NLR_PHYSICAL_DATA,		///< Represents any NLF as the actual newline of the line (@c Document#Line#getNewline()).
			NLR_DOCUMENT_DEFAULT,	///< Represents any NLF as the document default newline.
			NLR_SKIP,				///< Skips any NLF.
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
		 * @see DocumentPartitioner#getPartition
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
		class DocumentChange : public manah::Noncopyable {
		public:
			/// Returns the deleted region if the modification is deletion, otherwise the region of the inserted text.
			const Region& getRegion() const throw() {return region_;}
			/// Returns true if the modification is deletion or insertion.
			bool isDeletion() const throw() {return deletion_;}
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
		class Point : private manah::Unassignable {
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
			Document*		getDocument() throw();
			const Document*	getDocument() const throw();
			Direction		getGravity() const throw();
			const Position&	getPosition() const throw();
			bool			isDocumentDisposed() const throw();
			bool			isExcludedFromRestriction() const throw();
			void			setGravity(Direction gravity) throw();
			// listeners
			void	addLifeCycleListener(IPointLifeCycleListener& listener);
			void	removeLifeCycleListener(IPointLifeCycleListener& listener);
			// short-circuits
			length_t	getColumnNumber() const throw();
			ContentType	getContentType() const;
			length_t	getLineNumber() const throw();
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
		 * @see Document#getBookmarker, Document#Line#isBookmarked
		 */
		class Bookmarker : public manah::Noncopyable {
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
		 * A listener notified about the document change.
		 * @see DocumentUpdate, Document#addListener, Document#addPrenotifiedListener,
		 * Document#removeListener, Document#removePrenotifiedListener
		 */
		class IDocumentListener {
		private:
			/// The document is about to be changed.
			virtual void documentAboutToBeChanged(const Document& document) = 0;
			/**
			 * The text was deleted or inserted.
			 * @param document the document
			 * @param change the modification content
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
			/// The accessible region of the document is changed.
			virtual void documentAccessibleRegionChanged(Document& document) = 0;
			/// The code page and/or the line break of the document is changed.
			virtual void documentEncodingChanged(Document& document) = 0;
			/// The file name of the document is changed.
			virtual void documentFileNameChanged(Document& document) = 0;
			/// The modification flag of the document is changed.
			virtual void documentModificationSignChanged(Document& document) = 0;
			/// The read only mode of the document is changed.
			virtual void documentReadOnlySignChanged(Document& document) = 0;
			friend class Document;
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
			friend class Document;
		};

		/// Interface for objects which are interested in getting informed about progression of file IO.
		class IFileIOProgressListener {
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

		/// Used in @c Document#load and @c Document#save methods.
		class IFileIOListener : virtual public encodings::IUnconvertableCharCallback {
		protected:
			/// Destructor.
			virtual ~IFileIOListener() throw() {}
		private:
			/// Returns an @c IFileIOProgressCallback object or @c null if not needed.
			virtual IFileIOProgressListener* queryProgressCallback() = 0;
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
			virtual void documentSequentialEditStarted(Document& document) = 0;
			/**
			 * The sequential edit is stopped.
			 * @param document the document
			 */
			virtual void documentSequentialEditStopped(Document& document) = 0;
			/**
			 * The undo/redo operation is started.
			 * @param document the document
			 */
			virtual void documentUndoSequenceStarted(Document& document) = 0;
			/**
			 * The undo/redo operation is stopped.
			 * @param document the document
			 * @param resultPosition preferable position to put the caret
			 */
			virtual void documentUndoSequenceStopped(Document& document, const Position& resultPosition) = 0;
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
			virtual const unicode::IdentifierSyntax& getIdentifierSyntax(ContentType contentType) const throw() = 0;
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
		 * @see ContentType, Document, DocumentPartition, Document#getDocumentPartitioner,
		 * Document#setDocumentPartitioner, NullPartitioner
		 */
		class DocumentPartitioner {
		public:
			virtual			~DocumentPartitioner() throw();
			void			getPartition(const Position& at, DocumentPartition& partition) const;
			ContentType		getContentType(const Position& at) const;
			Document*		getDocument() throw();
			const Document*	getDocument() const throw();
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

		class DocumentCharacterIterator : public unicode::CharacterIterator,
			public StandardBidirectionalIteratorAdapter<DocumentCharacterIterator, CodePoint, CodePoint> {
		public:
			// constructors
			DocumentCharacterIterator() throw();
			DocumentCharacterIterator(const Document& document, const Position& position);
			DocumentCharacterIterator(const Document& document, const Region& region);
			DocumentCharacterIterator(const Document& document, const Region& region, const Position& position);
			DocumentCharacterIterator(const DocumentCharacterIterator& rhs) throw();
			// attributes
			const Document*	getDocument() const throw();
			const String&	getLine() const throw();
			const Region&	getRegion() const throw();
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
		class Document : public manah::Noncopyable,
				virtual public internal::IPointCollection<Point>, virtual public texteditor::internal::ISessionElement {
		public:
			/// Values returned by @c Document#load and @c Document#save.
			enum FileIOResult {
				/// Succeeded.
				FIR_OK,
				/// The specified encoding is invalid.
				FIR_INVALID_CODE_PAGE,
				/// The specified newline is based on Unicode but the encoding is not Unicode.
				FIR_INVALID_NEWLINE,
				/**
				 * The caller aborted (ex. unconvertable character found).
				 * @see Document#IFileIOListener
				 */
				FIR_CALLER_ABORTED,
				/// Failed for out of memory.
				FIR_OUT_OF_MEMORY,
				/// The file to be opend is too huge.
				FIR_HUGE_FILE,
				/**
				 * Tried to write the read only document.
				 * If the disk file is read only, returns @c FIR_STANDARD_WIN32_ERROR(@c ERROR_FILE_READ_ONLY) instead.
				 */
				FIR_READ_ONLY_MODE,
				/// Failed to create the temporary file for writing.
				FIR_CANNOT_CREATE_TEMPORARY_FILE,
				/// Failed to write to the file and the file was <strong>lost</strong>.
				FIR_LOST_DISK_FILE,
				/// Win32 standard error whose detail can be obtained by @c GetLastError.
				FIR_STANDARD_WIN32_ERROR,
			};

			/// Option flags for @c Document#save.
			struct SaveParameters {
				encodings::CodePage codePage;	///< The code page.
				Newline newline;				///< The newline.
				enum Option {
					WRITE_UNICODE_BOM	= 0x01,	///< Writes a UTF byte order mark.
					BY_COPYING			= 0x02,	///< Not implemented.
					CREATE_BACKUP		= 0x04	///< Creates backup files.
				};
				manah::Flags<Option> options;	///< Miscellaneous options.
			};

			/// Lock modes for opened file.
			struct FileLockMode {
				enum {
					LOCK_TYPE_NONE		= 0x00,	///< Does not lock.
					LOCK_TYPE_SHARED	= 0x01,	///< Uses shared lock.
					LOCK_TYPE_EXCLUSIVE	= 0x02	///< Uses exclusive lock.
				} type;
				bool onlyAsEditing;	///< If true, the lock will not be performed unless modification occurs.
			};

			/// Content of a line.
			class Line : public manah::FastArenaObject<Line> {
			public:
				/// Returns the text of the line.
				const String& getLine() const throw() {return text_;}
				/// Returns the newline of the line.
				Newline getNewline() const throw() {return newline_;}
				/// Returns true if the line is bookmarked.
				bool isBookmarked() const throw() {return bookmarked_;}
				/// Returns true if the line has been changed.
				bool isModified() const throw() {return operationHistory_ != 0;}
			private:
				Line() throw() : operationHistory_(0), newline_(NLF_AUTO), bookmarked_(false) {}
				explicit Line(String& text, Newline newline = NLF_AUTO, bool modified = false)
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
				manah::GapBuffer_DeletePointer<Line*> >	LineList;	///< List of lines

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
			void	setUnexpectedFileTimeStampDirector(IUnexpectedFileTimeStampDirector* newDirector) throw();
			// encodings
			encodings::CodePage	getCodePage() const throw();
			Newline				getNewline() const throw();
			void				setCodePage(encodings::CodePage cp);
			static void			setDefaultCode(encodings::CodePage cp, Newline newline);
			void				setNewline(Newline newline);
			// bound file name
			const WCHAR*	getFileExtensionName() const throw();
			const WCHAR*	getFileName() const throw();
			const WCHAR*	getFilePathName() const throw();
			bool			isBoundToFile() const throw();
			void			setFilePathName(const WCHAR* pathName);
			// attributes
			Bookmarker&					getBookmarker() throw();
			const Bookmarker&			getBookmarker() const throw();
			const FileLockMode&			getLockMode() const throw();
			const DocumentPartitioner&	getPartitioner() const throw();
			texteditor::Session*		getSession() throw();
			const texteditor::Session*	getSession() const throw();
			std::size_t					getUndoHistoryLength(bool redo = false) const throw();
			bool						isModified() const throw();
			bool						isReadOnly() const throw();
			bool						isSavable() const throw();
			virtual void				setModified(bool modified = true) throw();
			void						setPartitioner(std::auto_ptr<DocumentPartitioner> newPartitioner) throw();
			void						setReadOnly(bool readOnly = true);
			// contents
			Region			accessibleRegion() const throw();
			const String&	getLine(length_t line) const;
			const Line&		getLineInfo(length_t line) const;
			length_t		getLineLength(length_t line) const;
			length_t		getLineOffset(length_t line, NewlineRepresentation nlr) const;
			length_t		getNumberOfLines() const throw();
			ulong			getRevisionNumber() const throw();
			length_t		length(NewlineRepresentation nlr = NLR_PHYSICAL_DATA) const;
			Region			region() const throw();
			// content type information
			IContentTypeInformationProvider&	getContentTypeInformation() const throw();
			void								setContentTypeInformation(std::auto_ptr<IContentTypeInformationProvider> newProvider) throw();
			// manipulations
			Position	erase(const Region& region);
			Position	erase(const Position& pos1, const Position& pos2);
			Position	insert(const Position& position, const String& text);
			Position	insert(const Position& position, const Char* first, const Char* last);
			bool		isChanging() const throw();
			// undo/redo
			void	clearUndoBuffer();
			bool	isRecordingOperation() const throw();
			void	recordOperations(bool record);
			bool	redo();
			bool	undo();
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
			// writing and reading
			FileIOResult	load(const std::basic_string<WCHAR>& fileName,
								const FileLockMode& lockMode, encodings::CodePage codePage, IFileIOListener* callback = 0);
			FileIOResult	save(const std::basic_string<WCHAR>& fileName, const SaveParameters& params, IFileIOListener* callback = 0);
			FileIOResult	writeRegion(const std::basic_string<WCHAR>& fileName,
								const Region& region, const SaveParameters& params, bool append, IFileIOListener* callback = 0);
			// streams
			Position	insertFromStream(const Position& position, InputStream& in);
			void		writeToStream(OutputStream& out, NewlineRepresentation nlr = NLR_PHYSICAL_DATA) const;
			void		writeToStream(OutputStream& out, const Region& region, NewlineRepresentation nlr = NLR_PHYSICAL_DATA) const;
			// operations
			bool	checkTimeStamp();
			bool	renameFile(const WCHAR* newName);
			bool	sendFile(bool asAttachment, bool showDialog = true);

		private:
			void						doSetModified(bool modified) throw();
			void						fireDocumentAboutToBeChanged() throw();
			void						fireDocumentChanged(const DocumentChange& c, bool updateAllPoints = true) throw();
			void						initialize();
			void						partitioningChanged(const Region& changedRegion) throw();
			void						updatePoints(const DocumentChange& change) throw();
			static encodings::CodePage	translateSpecialCodePage(encodings::CodePage cp);
			bool						verifyTimeStamp(bool internal, ::FILETIME& newTimeStamp) throw();
			// internal::ISessionElement
			void	setSession(texteditor::Session& session) throw() {session_ = &session;}
			// internal::IPointCollection<Point>
			void	addNewPoint(Point& point) {points_.insert(&point);}
			void	removePoint(Point& point) {points_.erase(&point);}

		private:
			/// Manages undo/redo of the document.
			class UndoManager {
			public:
				// constructors
				UndoManager(Document& document) throw();
				virtual ~UndoManager() throw();
				// attributes
				std::size_t	getRedoBufferLength() const throw();
				std::size_t	getUndoBufferLength() const throw();
				bool		isStackingCompoundOperation() const throw();
				bool		isModifiedSinceLastSave() const throw();
				// operations
				void	beginCompoundOperation() throw();
				void	clear() throw();
				void	documentSaved() throw();
				void	endCompoundOperation() throw();
				template<class Operation>
				void	pushUndoBuffer(Operation& operation);
				bool	redo(Position& resultPosition);
				bool	undo(Position& resultPosition);

			private:
				Document& document_;
				std::stack<internal::OperationUnit*> undoStack_;
				std::stack<internal::OperationUnit*> redoStack_;
				enum {
					NONE, WAIT_FOR_FIRST_PUSH, WAIT_FOR_CONTINUATION
				} compoundOperationStackingState_;
				bool virtualOperation_;
				internal::OperationUnit* virtualUnit_;
				internal::OperationUnit* lastUnit_;
				internal::IOperation* savedOperation_;
			};

			class DefaultContentTypeInformationProvider : virtual public IContentTypeInformationProvider {
			public:
				const unicode::IdentifierSyntax& getIdentifierSyntax(ContentType) const throw() {return syntax_;}
			private:
				unicode::IdentifierSyntax syntax_;
			};

			class ModificationGuard {
			public:
				ModificationGuard(Document& document) throw() : document_(document) {document_.changing_ = true;}
				~ModificationGuard() throw() {document_.changing_= false;}
			private:
				Document& document_;
			};
			friend class ModificationGuard;

			struct DiskFile {
				std::auto_ptr<manah::win32::io::File<true> > lockingFile;
				WCHAR* pathName;
				bool unsavable;
				FileLockMode lockMode;
				struct {
					::FILETIME internal, user;
				} lastWriteTimes;
				DiskFile() throw() : pathName(0), unsavable(false) {
					std::memset(&lastWriteTimes.internal, 0, sizeof(::FILETIME));
					std::memset(&lastWriteTimes.user, 0, sizeof(::FILETIME));}
				bool isLocked() const throw();
				bool lock(const WCHAR* fileName) throw();
				bool unlock() throw();
			} diskFile_;
			texteditor::Session* session_;
			std::auto_ptr<DocumentPartitioner> partitioner_;
			std::auto_ptr<Bookmarker> bookmarker_;
			std::auto_ptr<IContentTypeInformationProvider> contentTypeInformationProvider_;
			bool readOnly_;
			bool modified_;
			encodings::CodePage codePage_;
			Newline newline_;
			LineList lines_;
			length_t length_;
			ulong revisionNumber_;
			std::set<Point*> points_;
			std::auto_ptr<UndoManager> undoManager_;
			bool onceUndoBufferCleared_;
			bool recordingOperations_;

			bool virtualOperating_;
			bool changing_;

			std::pair<Position, Point*>* accessibleArea_;

			ascension::internal::Listeners<IDocumentListener> listeners_;
			ascension::internal::Listeners<IDocumentListener> prenotifiedListeners_;
			ascension::internal::Listeners<IDocumentStateListener> stateListeners_;
			ascension::internal::Listeners<ISequentialEditListener> sequentialEditListeners_;
			ascension::internal::Listeners<IDocumentPartitioningListener> partitioningListeners_;
			IUnexpectedFileTimeStampDirector* timeStampDirector_;

			static encodings::CodePage defaultCodePage_;
			static Newline defaultNewline_;

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
				NewlineRepresentation nlr = NLR_PHYSICAL_DATA, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out);
			~DocumentBuffer() throw();
			const Position&	tell() const throw();
		private:
			int_type	overflow(int_type c);
			int			sync();
			int_type	uflow();
			int_type	underflow();
		private:
			Document& document_;
			const NewlineRepresentation nlr_;
			const std::ios_base::openmode mode_;
			Position current_;
			char_type buffer_[8192];
		};

		// free functions related to document
		Newline		eatNewline(const Char* first, const Char* last);
		length_t	getAbsoluteOffset(const Document& document, const Position& at, bool fromAccessibleStart);
		const Char*	getNewlineString(Newline newline);
		length_t	getNewlineStringLength(Newline newline);
		length_t	getNumberOfLines(const Char* first, const Char* last) throw();
		length_t	getNumberOfLines(const String& text) throw();
		Position	updatePosition(const Position& position, const DocumentChange& change, Direction gravity);
		// free functions related file path name
		std::wstring	canonicalizePathName(const wchar_t* pathName);
		bool			comparePathNames(const wchar_t* s1, const wchar_t* s2);


// inline implementation ////////////////////////////////////////////////////

/**
 * Returns the newline at the start of the specified buffer.
 * @param first the start of the buffer
 * @param last the end of the buffer
 * @return the newline or @c NLF_AUTO if the start of the buffer is not line break
 */
inline Newline eatNewline(const Char* first, const Char* last) {
	assert(first != 0 && last != 0 && first <= last);
	switch(*first) {
	case LINE_FEED:				return NLF_LF;
	case CARRIAGE_RETURN:		return (last - first > 1 && first[1] == LINE_FEED) ? NLF_CRLF : NLF_CR;
	case NEXT_LINE:				return NLF_NEL;
	case LINE_SEPARATOR:		return NLF_LS;
	case PARAGRAPH_SEPARATOR:	return NLF_PS;
	default:					return NLF_AUTO;
	}
}

/**
 * Returns the string represents the specified newline.
 * @param newline the newline
 * @return the string. an empty string if @a newline is @c NLF_AUTO
 * @throw std#invalid_argument @a newline is invalid
 * @see #getNewlineStringLength
 */
inline const Char* getNewlineString(Newline newline) {
	if(newline >= static_cast<Newline>(countof(internal::NEWLINE_STRINGS)))
		throw std::invalid_argument("Unknown newline specified.");
	return internal::NEWLINE_STRINGS[newline];
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
	case NLF_CRLF:
		return 2;
	case NLF_LF: case NLF_CR: case NLF_NEL: case NLF_LS: case NLF_PS:
		return 1;
	default:
		throw std::invalid_argument("Unknown newline specified.");
	}
}

/**
 * Returns the number of lines in the specified text.
 * @param text the text string
 * @return the number of lines
 */
inline length_t getNumberOfLines(const String& text) throw() {return getNumberOfLines(text.data(), text.data() + text.length());}

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
inline bool Point::operator==(const Point& rhs) const throw() {return getPosition() == rhs.getPosition();}
/// Unequality operator.
inline bool Point::operator!=(const Point& rhs) const throw() {return !(*this == rhs);}
/// Less-than operator.
inline bool Point::operator<(const Point& rhs) const throw() {return getPosition() < rhs.getPosition();}
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
/// Called when the document is disposed.
inline void Point::documentDisposed() throw() {document_ = 0;}
/// ...
inline void Point::excludeFromRestriction(bool exclude) {verifyDocument(); if(excludedFromRestriction_ = exclude) normalize();}
/// Returns the column.
inline length_t Point::getColumnNumber() const throw() {return position_.column;}
/// Returns the content type of the document partition contains the point.
inline ContentType Point::getContentType() const {verifyDocument(); return document_->getPartitioner().getContentType(*this);}
/// Returns the document or @c null if the document is already disposed.
inline Document* Point::getDocument() throw() {return document_;}
/// Returns the document or @c null if the document is already disposed.
inline const Document* Point::getDocument() const throw() {return document_;}
/// Returns the line number.
inline length_t Point::getLineNumber() const throw() {return position_.line;}
/// Returns the gravity.
inline Direction Point::getGravity() const throw() {return gravity_;}
/// Returns the position.
inline const Position& Point::getPosition() const throw() {return position_;}
/// Returns true if the document is already disposed.
inline bool Point::isDocumentDisposed() const throw() {return document_ == 0;}
/// Returns true if the point can't enter the inaccessible area of the document.
inline bool Point::isExcludedFromRestriction() const throw() {return excludedFromRestriction_;}
/// Moves to the specified position.
inline void Point::moveTo(length_t line, length_t column) {moveTo(Position(line, column));}
/// Sets the gravity.
inline void Point::setGravity(Direction gravity) throw() {verifyDocument(); gravity_ = gravity;}
/// Throws @c DisposedDocumentException if the document is already disposed.
inline void Point::verifyDocument() const {if(isDocumentDisposed()) throw DisposedDocumentException();}


/// Returns the accessible region of the document. The returned region is normalized.
/// @see #region
inline Region Document::accessibleRegion() const throw() {
	return (accessibleArea_ != 0) ? Region(accessibleArea_->first, *accessibleArea_->second) : region();}

/**
 * Registers the document listener with the document.
 * After registration @a listener is notified about each modification of this document.
 * @param listener the listener to be registered
 * @throw std#invalid_argument @a listener is already registered
 */
inline void Document::addListener(IDocumentListener& listener) {listeners_.add(listener);}

/**
 * Registers the document partitioning listener with the document.
 * @param listener the listener to be registered
 * @throw std#invalid_argument @a listener is already registered
 */
inline void Document::addPartitioningListener(IDocumentPartitioningListener& listener) {partitioningListeners_.add(listener);}

/**
 * Registers the document listener as one which is notified before those document listeners registered with @c #addListener are notified.
 * @internal This method is not for public use.
 * @param listener the listener to be registered
 * @throw std#invalid_argument @a listener is already registered
 */
inline void Document::addPrenotifiedListener(IDocumentListener& listener) {prenotifiedListeners_.add(listener);}

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

/// Clears the undo/redo stacks and deletes the history.
inline void Document::clearUndoBuffer() {undoManager_->clear(); onceUndoBufferCleared_ = true;}

/// Returns the @c DocumentCharacterIterator addresses the end of the document.
inline DocumentCharacterIterator Document::end() const throw() {return DocumentCharacterIterator(*this, region().second);}

/// @see #erase(const Region&)
inline Position Document::erase(const Position& pos1, const Position& pos2) {return erase(Region(pos1, pos2));}

/// Returns the bookmarker of the document.
inline Bookmarker& Document::getBookmarker() throw() {return *bookmarker_;}

/// Returns the bookmarker of the document.
inline const Bookmarker& Document::getBookmarker() const throw() {return *bookmarker_;}

/// Returns the code page of the document.
inline encodings::CodePage Document::getCodePage() const throw() {return codePage_;}

/// Returns the content information provider.
inline IContentTypeInformationProvider& Document::getContentTypeInformation() const throw() {return *contentTypeInformationProvider_;}

/// Returns the file extension name or @c null if the document is not bound to any of the files.
inline const WCHAR* Document::getFileExtensionName() const throw() {
	if(getFilePathName() == 0) return 0;
	else if(const WCHAR* const e = std::wcsrchr(getFilePathName(), L'.')) return e + 1;
	else return diskFile_.pathName + std::wcslen(getFilePathName());
}

/// Returns the file name or @c null if the document is not bound to any of the files.
inline const WCHAR* Document::getFileName() const throw() {
	if(getFilePathName() == 0) return 0;
	else if(const WCHAR* const e = std::wcsrchr(getFilePathName(), L'\\')) return e + 1;
	else return diskFile_.pathName + std::wcslen(getFilePathName());
}

/// Returns the file full name or @c null if the document is not bound to any of the files.
inline const WCHAR* Document::getFilePathName() const throw() {return diskFile_.pathName;}

/**
 * Returns the text of the specified line.
 * @param line the line
 * @return the text
 * @throw BadPostionException @a line is outside of the document
 */
inline const String& Document::getLine(length_t line) const {return getLineInfo(line).text_;}

/// Returns the default newline of the document.
inline Newline Document::getNewline() const throw() {return newline_;}

/**
 * Returns the information of the specified line.
 * @param line the line
 * @return the information about @a line
 * @throw BadPostionException @a line is outside of the document
 */
inline const Document::Line& Document::getLineInfo(length_t line) const {
	if(line >= lines_.getSize()) throw BadPositionException(); return *lines_[line];}
#if 0
inline Document::LineIterator Document::getLineIterator(length_t line) const {
	assertValid();
	if(line >= lines_.getSize())
		throw BadPositionException();
	return lines_.begin() + line;
}
#endif
/**
 * Returns the length of the specified line. The line break is not included.
 * @param line the line
 * @return the length of @a line
 * @throw BadLocationException @a line is outside of the document
 */
inline length_t Document::getLineLength(length_t line) const {return getLine(line).length();}

/// Returns the file lock mode.
inline const Document::FileLockMode& Document::getLockMode() const throw() {return diskFile_.lockMode;}

/// Returns the number of lines in the document.
inline length_t Document::getNumberOfLines() const throw() {return lines_.getSize();}

/// Returns the document partitioner of the document.
inline const DocumentPartitioner& Document::getPartitioner() const throw() {
	if(partitioner_.get() == 0) {
		Document& self = *const_cast<Document*>(this);
		self.partitioner_.reset(static_cast<DocumentPartitioner*>(new NullPartitioner));
		self.partitioner_->install(self);
	}
	return *partitioner_;
}

/// Returns the revision number.
inline ulong Document::getRevisionNumber() const throw() {return revisionNumber_;}

/// Returns the session to which the document belongs.
inline texteditor::Session* Document::getSession() throw() {return session_;}

/// Returns the session to which the document belongs.
inline const texteditor::Session* Document::getSession() const throw() {return session_;}

/// ...
inline std::size_t Document::getUndoHistoryLength(bool redo /* = false */) const throw() {
	return redo ? undoManager_->getRedoBufferLength() : undoManager_->getUndoBufferLength();}

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

/// Returns true if the document is bound to any file.
inline bool Document::isBoundToFile() const throw() {return getFilePathName() != 0;}

/// Returns true if the document is in changing.
inline bool Document::isChanging() const throw() {return changing_;}

/**
 * Returns true if the document has been modified.
 * @see #setModified, IDocumentStateListener#documentModificationSignChanged
 */
inline bool Document::isModified() const throw() {return modified_;}

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
 * 
 * @see #recordOperations, #getUndoHistoryLength
 */
inline bool Document::isRecordingOperation() const throw() {return recordingOperations_;}

/// Returns true if the document can be written to the file.
inline bool Document::isSavable() const throw() {return !diskFile_.unsavable;}

/**
 * Returns the number of characters (UTF-16 code units) in the document.
 * @param nlr the method to count newlines
 * @return the number of characters
 * @throw std#invalid_argument @a nlr is invalid
 */
inline length_t Document::length(NewlineRepresentation nlr) const {
	if(nlr == NLR_DOCUMENT_DEFAULT)
		nlr = (getNewline() == NLF_CRLF) ? NLR_CRLF : NLR_LINE_FEED;
	switch(nlr) {
	case NLR_LINE_FEED:
	case NLR_LINE_SEPARATOR:
		return length_ + getNumberOfLines() - 1;
	case NLR_CRLF:
		return length_ + getNumberOfLines() * 2 - 1;
	case NLR_PHYSICAL_DATA: {
		length_t len = length_;
		const length_t lines = getNumberOfLines();
		for(length_t i = 0; i < lines; ++i)
			len += getNewlineStringLength(lines_[i]->newline_);
		return len;
	}
	case NLR_SKIP:
		return length_;
	}
	throw std::invalid_argument("invalid parameter.");
}

/**
 * Transfers the partitioning change to the listeners.
 * @param changedRegion the changed region
 */
inline void Document::partitioningChanged(const Region& changedRegion) throw() {
	partitioningListeners_.notify<const Region&>(IDocumentPartitioningListener::documentPartitioningChanged, changedRegion);}

/// Returns the entire region of the document. The returned region is normalized.
/// @see #accessibleRegion
inline Region Document::region() const throw() {
	return Region(Position::ZERO_POSITION, Position(lines_.getSize() - 1, getLineLength(lines_.getSize() - 1)));}

/**
 * Removes the document listener from the document.
 * @param listener the listener to be removed
 * @throw std#invalid_argument @a listener is not registered
 */
inline void Document::removeListener(IDocumentListener& listener) {listeners_.remove(listener);}

/**
 * Removes the document partitioning listener from the document.
 * @param listener the listener to be removed
 * @throw std#invalid_argument @a listener is not registered
 */
inline void Document::removePartitioningListener(IDocumentPartitioningListener& listener) {partitioningListeners_.remove(listener);}

/**
 * Removes the pre-notified document listener from the document.
 * @internal This method is not for public use.
 * @param listener the listener to be removed
 * @throw std#invalid_argument @a listener is not registered
 */
inline void Document::removePrenotifiedListener(IDocumentListener& listener) {prenotifiedListeners_.remove(listener);}

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

/**
 * Sets the encoding of the document.
 * @param cp the code page of the encoding
 * @throw std#invalid_argument @a cp is invalid
 */
inline void Document::setCodePage(encodings::CodePage cp) {
	cp = translateSpecialCodePage(cp);
	if(cp != codePage_) {
		if(!encodings::EncoderFactory::getInstance().isValidCodePage(cp)
				|| encodings::EncoderFactory::getInstance().isCodePageForAutoDetection(cp))
			throw std::invalid_argument("Specified code page is not available.");
		codePage_ = cp;
		stateListeners_.notify<Document&>(IDocumentStateListener::documentEncodingChanged, *this);
	}
}

/**
 * Sets the content type information provider.
 * @param newProvider the new content type information provider. the ownership will be transferred
 * to the callee. can be @c null
 */
inline void Document::setContentTypeInformation(std::auto_ptr<IContentTypeInformationProvider> newProvider) throw() {
	contentTypeInformationProvider_.reset((newProvider.get() != 0) ? newProvider.release() : new DefaultContentTypeInformationProvider);}

/**
 * Sets the default newline of the document.
 * @param newline the newline
 * @throw std#invalid_argument @a newline is invalid
 */
inline void Document::setNewline(Newline newline) {
	switch(newline) {
	case NLF_LF:	case NLF_CR:
	case NLF_CRLF:	case NLF_NEL:
	case NLF_LS:	case NLF_PS:
		if(newline != newline_) {
			newline_ = newline;
			stateListeners_.notify<Document&>(IDocumentStateListener::documentEncodingChanged, *this);
		}
		break;
	default:
		throw std::invalid_argument("Unknown newline specified.");
	}
}

/**
 * Makes the document read only or not.
 * @see ReadOnlyDocumentException, #isReadOnly
 */
inline void Document::setReadOnly(bool readOnly /* = true */) {
	if(readOnly != isReadOnly()) {
		readOnly_ = readOnly;
		stateListeners_.notify<Document&>(IDocumentStateListener::documentReadOnlySignChanged, *this);
	}
}

/// ...
inline void Document::setUnexpectedFileTimeStampDirector(IUnexpectedFileTimeStampDirector* newDirector) throw() {timeStampDirector_ = newDirector;}

/**
 * Writes the content of the document to the specified output stream.
 * @param out the output stream
 * @param nlr the newline to be used
 */
inline void Document::writeToStream(OutputStream& out, NewlineRepresentation nlr /* = NLR_PHYSICAL_DATA */) const {
	writeToStream(out, Region(Position::ZERO_POSITION, Position(getNumberOfLines() - 1, getLineLength(getNumberOfLines() - 1))), nlr);}


/**
 * Returns the content type of the partition contains the specified position.
 * @param at the position
 * @throw BadPositionException @a position is outside of the document
 * @throw IllegalStateException the partitioner is not connected to any document
 * @return the content type
 */
inline ContentType DocumentPartitioner::getContentType(const Position& at) const {
	DocumentPartition p;
	getPartition(at, p);
	return p.contentType;
}

/// Returns the document to which the partitioner connects or @c null.
inline Document* DocumentPartitioner::getDocument() throw() {return document_;}

/// Returns the document to which the partitioner connects or @c null.
inline const Document* DocumentPartitioner::getDocument() const throw() {return document_;}

/**
 * Returns the document partition contains the specified position.
 * @param at the position
 * @param[out] partition the partition
 * @throw BadPositionException @a position is outside of the document
 * @throw IllegalStateException the partitioner is not connected to any document
 */
inline void DocumentPartitioner::getPartition(const Position& at, DocumentPartition& partition) const {
	if(document_ == 0)
		throw IllegalStateException("the partitioner is not connected to any document.");
	else if(at > document_->region().second)
		throw BadPositionException();
	return doGetPartition(at, partition);
}

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

/// Returns the document.
inline const Document* DocumentCharacterIterator::getDocument() const throw() {return document_;}

/// Returns the iteration region.
inline const Region& DocumentCharacterIterator::getRegion() const throw() {return region_;}

/// Returns the line.
inline const String& DocumentCharacterIterator::getLine() const throw() {return *line_;}

/// @see unicode#CharacterIterator#hasNext
inline bool DocumentCharacterIterator::hasNext() const throw() {return p_ != region_.second;}

/// @see unicode#CharacterIterator#hasPrevious
inline bool DocumentCharacterIterator::hasPrevious() const throw() {return p_ != region_.first;}

/**
 * Moves to the specified position.
 * @param to the position. if this is outside of the iteration region, the start/end of the region will be used
 */
inline DocumentCharacterIterator& DocumentCharacterIterator::seek(const Position& to) {
	line_ = &document_->getLine((p_ = std::max(std::min(to, region_.second), region_.first)).line); return *this;}

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

}} // namespace ascension.text

#endif /* !ASCENSION_DOCUMENT_HPP */
