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
#include "../../manah/memory.hpp"		// manah::FastArenaObject
#include "../../manah/gap-buffer.hpp"	// manah::GapBuffer
#include "../../manah/win32/file.hpp"


namespace ascension {

	namespace presentation {
		class Presentation;
	}

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
			const Char LINE_BREAK_STRINGS[][7] = {
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
		 * Line break codes.
		 * @see getLineBreakLength, getLineBreakString, Document
		 */
		enum LineBreak {
			LB_AUTO,	///< Indicates no-conversion or unspecified.
			LB_LF,		///< Line feed. Standard of Unix (Lf, U+000A)
			LB_CR,		///< Carriage return. Old standard of Macintosh (Cr, U+000D)
			LB_CRLF,	///< CR+LF. Standard of Windows (CrLf, U+000D U+000A)
			LB_NEL,		///< New line (U+0085)
			LB_LS,		///< Line separator (U+2028)
			LB_PS,		///< Paragraph separator (U+2029)
			LB_COUNT
		};

		/**
		 * Representation of a line break.
		 * Defines how an interpreter treats line breaks.
		 * For example, @c Document#getLength method which calculates the length of the document refers this setting.
		 * Some methods can select prefer setting to <> its efficiency.
		 * @see Document#getLength, Document#getLineIndex, Document#writeToStream,
		 * Document#CharacterIterator, EditPoint#getText, Selection#getText
		 */
		enum LineBreakRepresentation {
			LBR_LINE_FEED,			///< represents any NLF as LF (U+000D)
			LBR_CRLF,				///< represents any NLF as CRLF (U+000D+000A)
			LBR_LINE_SEPARATOR,		///< represents any NLF as LS (U+2028)
			LBR_PHYSICAL_DATA,		///< represents any NLF as the actual newline of the line (@c Document#Line#getLineBreak())
			LBR_DOCUMENT_DEFAULT,	///< represents any NLF as the document default newline
			LBR_SKIP,				///< skips any NLF
		};

		/**
		 * @c Position represents a position in the document by a line number and distance from start of line.
		 * @note This class is not derivable.
		 * @see text#Point, text#EditPoint, viewers#VisualPoint, viewers#Caret
		 */
		class Position : public manah::FastArenaObject<Position> {
		public:
			length_t line;		///< Line number
			length_t column;	///< Position in the line
			static const Position INVALID_POSITION;	///< Unused or invalid position
		public:
			/// Constructor
			explicit Position(length_t lineNumber = 0, length_t columnNumber = 0) throw() : line(lineNumber), column(columnNumber) {}
			/// Equality operator
			bool operator==(const Position& rhs) const throw() {return line == rhs.line && column == rhs.column;}
			/// Unequality operator
			bool operator!=(const Position& rhs) const throw() {return line != rhs.line || column != rhs.column;}
			/// Relational operator
			bool operator<(const Position& rhs) const throw() {return line < rhs.line || (line == rhs.line && column < rhs.column);}
			/// Relational operator
			bool operator<=(const Position& rhs) const throw() {return *this < rhs || *this == rhs;}
			/// Relational operator
			bool operator>(const Position& rhs) const throw() {return line > rhs.line || (line == rhs.line && column > rhs.column);}
			/// Relational operator
			bool operator>=(const Position& rhs) const throw() {return *this > rhs || *this == rhs;}
		};

		/**
		 * A region consists of two positions and represents a linear range in the document.
		 * There are no restriction about greater/less relationship between the two positions.
		 * @note This class is not derivable.
		 */
		class Region : public std::pair<Position, Position>, public manah::FastArenaObject<Region> {
		public:
			/// Default constructor.
			Region() throw() {}
			/// Constructor.
			Region(const Position& one, const Position& other) throw() : std::pair<Position, Position>(one, other) {}
			/// Returns the minimum position.
			Position& getTop() throw() {return (first < second) ? first : second;}
			/// Returns the minimum position.
			const Position& getTop() const throw() {return (first < second) ? first : second;}
			/// Returns the maximum position.
			Position& getBottom() throw() {return (first > second) ? first : second;}
			/// Returns the maximum position.
			const Position& getBottom() const throw() {return (first > second) ? first : second;}
			/// Returns true if @a p is contained by the region.
			bool includes(const Position& p) const throw() {return p >= getTop() && p < getBottom();}
			/// Returns true if the region is empty.
			bool isEmpty() const throw() {return first == second;}
		};

		/**
		 * A document partition.
		 * @see DocumentPartitioner#getPartition
		 */
		struct DocumentPartition {
			ContentType contentType;	///< content type of the partition
			Region region;				///< region of the partition
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
		 * A point represents a document position and adapts to the document change.
		 *
		 * When the document changed occured, @c Point moves automatically as follows
		 * ("forward" means "to the end of the document"):
		 * <ul>
		 *   <li>ÁÔøΩ„¬ÆÂâÅEΩÅEΩ¬´„ÉÅEΩÅEΩ„ÔøΩ„ÉÅEΩ¬Ç¬íÊÅEΩÂÔøΩ„Åó„ÅüÂ†¥ÂêÅEΩ¬ÇÔøΩ¬ÅEøΩÅEΩ„ÔøΩ„ÉÅEΩ¬Ç¬íÂâÔøΩÅEΩ¬ÅEó„ÅüÂ†¥ÂêÅEΩ¬Ø„Ä√£¬ÅEøΩÅEΩ„ÔøΩ„ÉÅEΩ¬ÆÈÔøΩ¬ÅEï„¬†„ÅÅEΩªÂã¬ï„Åô„ÇÅE/li>
		 *   <li>ÁÔøΩ„¬Æ‰¬ΩÅEΩΩÆ„¬´„ÉÅEΩÅEΩ„ÔøΩ„ÉÅEΩ¬Ç¬íÊÅEΩÂÔøΩ„Åó„ÅüÂ†¥ÂêÅEΩ¬Ø„Ä√£¬ÅEøΩÅEΩ„ÔøΩ„ÉÅEΩ¬ÆÈÔøΩ¬ÅEï„¬†„ÅÅEΩÅEΩÅEΩ¬´ÁßªÂãï„Åô„Çã„ÄÅEΩ¬ÅEü„¬†„Åó„ÅEΩ„ÔøΩ„Éì„ÅEøΩÅEΩ„ÅÅE
		 *   @c PositionUpdator#BACKWARD „¬´Ë®≠ÂÆö„Åï„ÇÅEΩ¬¶„ÅÅEΩ¬Ç¬ãÂ†¥ÂêÅEΩ¬ØÁßªÂãï„Åó„¬™„ÅÅE/li>
		 *   <li>ÁÔøΩ„¬ÆÂ¬æÅEΩÅEΩ¬ß„ÉÅEΩÅEΩ„ÔøΩ„ÉÅEΩ¬Ç¬íÊÅEΩÂÔøΩ„ÔøΩÂâÅEΩÅEΩ¬ÅEó„ÅüÂ†¥ÂêÅEΩ¬Ø„Ä√ßßªÂ¬ã¬ï„Åó„¬™„ÅÅE/li>
		 *   <li>ÁÔøΩ„ÇíÂÅEΩ¬Ç¬ÄÁ¬ØÅEΩÅEΩ¬Ç¬íÂâÔøΩÅEΩ¬ÅEó„ÅüÂ†¥ÂêÅEΩ¬Ä√ßØÅEΩÅEΩ¬ÆÂÖÅEΩ†≠„¬´ÁßªÂãï„Åô„ÇÅE/li>
		 * </ul>
		 * For details of gravity, see description of @c updatePosition function.
		 *
		 * When the document was reset (by @c Document#resetContent), the point moves to the start of the document.
		 *
		 * Almost all methods of this or derived classes will throw @c DisposedDocumentException if the document is already disposed.
		 * Call @c #isDocumentDisposed to check if the document is exist or not.
		 *
		 * @see Position, Document, EditPoint, VisualPoint, Caret
		 */
		class Point : public manah::Unassignable {
		public:
			// constructors
			explicit Point(Document& document) throw();
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
			friend class Document;
		};

		/// Thrown when the read only document is about to be modified.
		class ReadOnlyDocumentException : public std::logic_error {
		public:
			/// Constructor
			ReadOnlyDocumentException() : std::logic_error("The document is readonly. Any edit process is denied.") {}
		};

		/// Thrown when the specified line or character position is outside of the document.
		class BadPositionException : public std::invalid_argument {
		public:
			/// Constructor
			BadPositionException() : std::invalid_argument("The position is outside of the document.") {}
		};

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

		/// „Éï„ÅEΩ„ÔøΩ„ÔøΩ„ÅÅEΩ§ñÈÅEΩ„¬ßÂ¬§ÅEΩÅEΩ¬ÅEï„ÇÔøΩ¬ÅEü„¬®„ÅÅEΩ¬ÆÂØæÂÔøΩ
		class IUnexpectedFileTimeStampDirector {
		public:
			/// Context.
			enum Context {
				FIRST_MODIFICATION,	///< Êú™ÊÔøΩÅEΩÅEΩÊÖã„Åã„ÇÅEΩ¬à¬ù„Ç√£¬¶Á∑®ÈõÅEΩ¬ÅEó„ÇÔøΩ¬ÅEøΩ¬®„Åó„¬¶„ÅÅEΩ¬Ç¬ÅE
				OVERWRITE_FILE,		///< ÊÔøΩ≠ò„¬Æ„Éï„ÅEΩ„ÔøΩ„ÔøΩ„¬´‰¬∏ÅEΩÅEΩ¬ÅEøΩ¬ÅEó„ÇÔøΩ¬ÅEøΩ¬®„Åó„¬¶„ÅÅEΩ¬Ç¬ÅE
				CLIENT_INVOCATION	///< @c Document#checkTimeStamp ÂÔøΩ¬≥ÂÔøΩ„Åó„¬´„ÇÅEΩ¬Ç¬ÅE
			};
		private:
			/**
			 * „Éï„ÅEΩ„ÔøΩ„ÔøΩ„ÅÅEΩ§ñÈÅEΩ„¬ßÂ¬§ÅEΩÅEΩ¬ÅEï„ÇÔøΩ¬¶„ÅÅEΩ¬Ç¬ã„¬Æ„ÇÅEΩ∫Ë™ÅEΩ¬ÅEó„Åü„¬®„ÅÅEΩ¬´ÂÔøΩ¬≥ÂÔøΩ„Åï„ÇÔøΩ¬Ç¬ÅE
			 * @param document „Éï„ÅEΩ„ÔøΩ„ÔøΩ„ÇíÈñã„ÅÅEΩ¬¶„ÅÅEΩ¬Ç¬ã„ÅEøΩÅEΩ„ÔøΩ„ÔøΩ„ÔøΩ„ÉÅE
			 * @param context ÁÔøΩÊ≥ÅE
			 * @retval true	ÂÜÅEΩÅEΩÁöÅEΩ¬´ÁÆ°ÁêÅEΩ¬ÅEó„¬¶„ÅÅEΩ¬Ç¬ã„ÅEΩ„ÔøΩ„ÔøΩ„ÔøΩ„ÔøΩ„ÔøΩ„Éó„ÇÔøΩ¬üÈöõ„¬ÆÂÄ§„¬ßÊÔøΩÅEΩ¬ÅEó„ÄÅ@p context „¬ÆÂÔøΩÁêÅEΩ¬ÇÔøΩÅEΩÅEΩ¬ÅEô„ÇãÂ†¥ÂêÅE
			 * @retval false ÂÔøΩ„ÅÅEΩÅEΩ„ÔøΩ„ÔøΩ„ÔøΩ„ÔøΩ„ÔøΩ„Éó„ÇÔøΩ≠Êå√£¬ÅEó„ÄÅ@p context „¬ÆÂÔøΩÁêÅEΩ¬ÇÔøΩ≠¬ÊÔøΩ¬ÅEô„ÇãÂ†¥ÂêÅE
			 */
			virtual bool queryAboutUnexpectedDocumentFileTimeStamp(Document& document, Context context) throw() = 0;
			friend class Document;
		};

		/// „ÉÅEΩÅEΩ„ÔøΩ„ÔøΩ„ÔøΩ„ÉÅEΩ¬Æ‰øùÂ≠ò„Ä√®™≠„¬øËæº„¬ø„¬ÆÈÄ≤Êçó„ÇíÂèó„ÅëÂèñ„Çã (Êú™„ÔøΩ„Éù„ÅEΩ„ÉÅE
		class IFileIOProgressListener {
		public:
			enum ProcessType {};
		private:
			/**
			 * ÈÄ≤Êçó„¬ÆÈÄöÁü•
			 * @param type ÂÔøΩÁêÅEΩ¬ÅEøΩÆ¬π („ÉÅEΩÅEΩ„ÔøΩÈáÅEΩ¬ÆÂ¬ÆÅEΩ©¬„¬ØÂÔøΩÁêÅEΩ¬ÅEøΩÆπ„¬´„ÇÅEΩ¬Ç¬ÅE
			 * @param processedAmount ÊÔøΩ¬´ÂÔøΩÁêÅEΩ¬ÅEó„Åü„ÉÅEΩÅEΩ„ÔøΩÈáÅE
			 * @param totalAmount ÂÔøΩÁêÅEΩ¬ÅEô„¬π„ÅÅEΩÅEΩ„ÉÅEΩÅEΩ„ÔøΩÈáÅE
			 */
			virtual void onProgress(ProcessType type, ULONGLONG processedAmount, ULONGLONG totalAmount) = 0;
			/// ÈÄ≤Êçó„ÇíÈÄöÁü•„Åô„ÇãÈñìÈöî„ÇÅEΩÅEΩÅEΩ¬ßËøî„Åô
			virtual length_t queryIntervalLineCount() const = 0;
			/// Á†¥Ê£ÅE
			virtual void release() = 0;
			friend class Document;
		};

		/// @c Document#load „ÄÅ@c Document#save „¬ß‰Ωø„ÅÅEΩÅEΩ„ÔøΩ„ÔøΩ„Éê„ÅEøΩÅEΩ
		class IFileIOListener : virtual public encodings::IUnconvertableCharCallback {
		protected:
			/// „ÉÅEΩÅEΩ„ÉÅEΩÅEΩ„ÔøΩ„ÔøΩ
			virtual ~IFileIOListener() throw() {}
		private:
			/// ÂÔøΩÁêÅEΩ¬ÆÈÄ≤Êçó„ÇíÂèó„ÅëÂèñ„Çã @c IFileIOProgressCallback „ÔøΩ„ÔøΩ„ÔøΩ„ÔøΩ„ÔøΩ„ÔøΩ„ÇÅEΩ¬î„Åô„ÄÅEc null „ÇÅEΩ¬î„Åó„¬¶„ÇÅEΩ¬ÇÔøΩ¬ÅEÅE
			virtual IFileIOProgressListener* queryProgressCallback() = 0;
			friend class Document;
		};

		/**
		 * Interface for objects which are interested in getting informed about changes of a document's sequential edit.
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
		 * @see ContentType, Document, DocumentPartition, Document#getDocumentPartitioner, Document#setDocumentPartitioner, NullPartitioner
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

		/**
		 * A document manages a text content and supports text manipulations.
		 *
		 * All text content is represented in UTF-16. To treat this as UTF-32, use
		 * @c DocumentCharacterIterator.
		 *
		 * A document manages also its operation history, encoding, and line-breaks
		 * and writes to or reads the content from files or streams.
		 *
		 * @c #insertText inserts a text string into any position.
		 * @c #deleteText deletes any text region.
		 * Other classes also provide text manipulation for the document.
		 *
		 *	Ë¬§ÅEΩÅEΩ¬ÆÊìÅEΩΩú„¬Ç¬ÅE1 Âõû„¬ß„ÔøΩ„ÔøΩ„ÉÅEΩÅEΩ„ÔøΩ„ÔøΩ„ÉÅEΩÅEΩ„¬ß„ÅÅEΩ¬Ç¬ã„ÇÔøΩ¬ÅEøΩ¬´„Åô„Çã„¬´„¬Ø<strong>ÈÄ£Á¬∂ÅEΩ®Èõ¬ÅE/strong>„ÇÅEΩø„ÅÅEΩ¬Ä¬ÅE
		 *	@c beginSequentialEdit ÂÔøΩ¬≥ÂÔøΩ„Åó„¬¶„Åã„Çâ @c #endSequentialEdit
		 *	„ÇíÂÅEΩ¬≥ÂÔøΩ„Åô„¬æ„¬ß„¬´Ë¬°ÅEΩ¬ÇÔøΩ¬ÇÔøΩ¬ÅEüÊìÔøΩΩú„¬Ø„Ä√•¬çÔøΩ¬Ä„¬ÆÁ∑®ÈõÅEΩÅEΩ„ÔøΩ„ÔøΩ„Éó„¬´„¬æ„¬®„Ç√£¬ÇÔøΩ¬ÇÔøΩ¬Ç¬ã„ÄÅEΩæã„¬ÅEøΩ¬∞
		 *	@c Viewer#inputCharacter „¬ØË¬§ÅEΩÅEΩ¬ÆÊñÅEΩ≠ó„¬ÆÂÔøΩÂäõ„Çí„¬æ„¬®„Ç√£¬Ç¬ã„Åü„Ç√£¬´„Ä√£¬ÅEì„¬ÆÊ©üËÅEΩ„ÇÅEΩø¬„¬£„¬¶„ÅÅEΩ¬Ç¬ÅE
		 *
		 *	„ÉÅEΩÅEΩ„ÔøΩ„ÔøΩ„ÔøΩ„ÉÅEΩ¬ÆÂ¬§ÅEΩÅEΩ¬Ç¬íÁÅEΩ¶ñ„¬ÅEô„Çã„Åü„Ç√£¬Æ„ÔøΩ„ÔøΩ„ÉÅEΩ¬ÅEøΩ¬ÅEøΩ¬ÅEøΩ¬§„Åã„ÅEøΩ¬Ç¬ÅE
		 *
		 * A document can be devides into a sequence of semantic segments called partition.
		 * Document partitioners expressed by @c DocumentPartitioner class define these
		 * partitioning. Each partitions have its content type and region (see @c DocumentPartition).
		 * To set the new partitioner, use @c #setPartitioner method. The partitioner's ownership
		 * will be transferred to the document.
		 *
		 * @see Viewer, IDocumentPartitioner, Point, EditPoint
		 */
		class Document : public manah::Noncopyable,
				virtual public internal::IPointCollection<Point>, virtual public texteditor::internal::ISessionElement {
		public:
			/// Values returned by @c Document#load and @c Document#save.
			enum FileIOResult {
				/// Succeeded.
				FIR_OK,
				/// The specified encoding is invalid.
				FIR_INVALID_CODE_PAGE,
				/// The specified line break is based on Unicode but the encoding is not Unicode.
				FIR_INVALID_LINE_BREAK,
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
				encodings::CodePage codePage;	///< The code page
				LineBreak lineBreak;			///< The line break
				enum Option {
					WRITE_UNICODE_BOM	= 0x01,	///< UTF-8„ÄÅE6„ÄÅE2 „¬ß‰øùÂ≠ò„Åô„Çã„¬®„ÅÅEΩ¬´ BOM „ÇíÊÅEΩ¬ÅEøΩæº„¬Ç¬Ä
					BY_COPYING			= 0x02,	///< „Éï„ÅEΩ„ÔøΩ„ÔøΩ„Çí„ÅEΩ„Éî„ÅEΩ„Åô„Çã„Åì„¬®„¬ßÊÔøΩ¬ÅEøΩæº„¬Ç¬Ä
					CREATE_BACKUP		= 0x04	///< ‰øùÂ≠òÂâÔøΩ¬Æ„Éï„ÅEΩ„ÔøΩ„ÔøΩ„¬Æ„Éê„ÅEøΩÅEΩ„ÔøΩ„ÉÅEΩ¬É¬ó„Çí„Åî„¬øÁÆ±„¬´‰ΩúÊÅE„Åô„Çã
				};
				manah::Flags<Option> options;	///< Miscellaneous options
			};

			/// „Éï„ÅEΩ„ÔøΩ„ÔøΩ„¬Æ„ÔøΩ„ÉÅEΩÅEΩÊÔøΩºÅE
			struct FileLockMode {
				enum {
					LOCK_TYPE_NONE		= 0x00,	///< „ÔøΩ„ÉÅEΩÅEΩÁÔøΩ„ÅÅE
					LOCK_TYPE_SHARED	= 0x01,	///< ÂÔøΩÊúÅEΩÅEΩ„ÉÅEΩÅEΩ
					LOCK_TYPE_EXCLUSIVE	= 0x02	///< ÊéÅEΩ¬ñ„ÅEΩ„ÉÅEΩÅEΩ
				} type;
				bool onlyAsEditing;	///< Á∑®ÈõÅEΩ∏≠„¬Æ„¬ø„ÔøΩ„ÉÅEΩÅEΩ
			};

			/// Content of a line.
			class Line : public manah::FastArenaObject<Line> {
			public:
				/// Returns the text of the line.
				const String& getLine() const throw() {return text_;}
				/// Returns the line break of the line.
				LineBreak getLineBreak() const throw() {return lineBreak_;}
				/// Returns true if the line is bookmarked.
				bool isBookmarked() const throw() {return bookmarked_;}
				/// Returns true if the line has been changed.
				bool isModified() const throw() {return operationHistory_ != 0;}
			private:
				Line() throw() : operationHistory_(0), lineBreak_(LB_AUTO), bookmarked_(false) {}
				explicit Line(String& text, LineBreak lineBreak = LB_AUTO, bool modified = false)
					: text_(text), operationHistory_(modified ? 1 : 0), lineBreak_(lineBreak), bookmarked_(false) {}
				String text_;					// Ë°ÅE
				ulong operationHistory_ : 28;	// „ÔøΩ„ÔøΩ„ÉÅEΩÅEΩ„ÔøΩ„ÔøΩ„ÔøΩ„ÔøΩ (0 „¬ßÂ¬§ÅEΩÅEΩÅEΩ„Åó„¬ÆÁÔøΩÊÖÅE
				LineBreak lineBreak_ : 3;		// ÊÔøΩ°ÅEΩ¬ÆÁ®ÆÈ°ÅE
				mutable bool bookmarked_ : 1;	// „Éñ„ÅEøΩÅEΩ„Éû„ÅEΩ„ÔøΩ„Åï„ÇÔøΩ¬ÅEüË¬°ÅEΩ¬ÅEÅE
#if (3 < 2 << LB_COUNT)
#error "lineBreak_ member is not allocated efficient buffer."
#endif
				friend class Document;
				friend class Bookmarker;
			};
			typedef manah::GapBuffer<Line*,
				manah::GapBuffer_DeletePointer<Line*> >	LineList;	///< List of lines
//			typedef LineList::ConstIterator	LineIterator;			///< Ë¬°ÅEΩ¬ÆÂèÅEΩæ©Â≠ÅE

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
			LineBreak			getLineBreak() const throw();
			void				setCodePage(encodings::CodePage cp);
			static void			setDefaultCode(encodings::CodePage cp, LineBreak lineBreak);
			void				setLineBreak(LineBreak lineBreak);
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
			Position		getEndPosition(bool accessibleRegion = true) const throw();
			length_t		getLength(LineBreakRepresentation lbr = LBR_PHYSICAL_DATA) const;
			const String&	getLine(length_t line) const;
			const Line&		getLineInfo(length_t line) const;
//			LineIterator	getLineIterator(length_t line) const;
			length_t		getLineLength(length_t line) const;
			length_t		getLineOffset(length_t line, LineBreakRepresentation lbr) const;
			length_t		getNumberOfLines() const throw();
			Position		getStartPosition(bool accessibleRegion = true) const throw();
			// content type information
			IContentTypeInformationProvider&	getContentTypeInformation() const throw();
			void								setContentTypeInformation(IContentTypeInformationProvider* newProvider) throw();
			// manipulations
			Position	deleteText(const Region& region);
			Position	deleteText(const Position& pos1, const Position& pos2);
			Position	insertText(const Position& position, const String& text);
			Position	insertText(const Position& position, const Char* first, const Char* last);
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
			// writing and reading
			FileIOResult	load(const std::basic_string<WCHAR>& fileName,
								const FileLockMode& lockMode, encodings::CodePage codePage, IFileIOListener* callback = 0);
			FileIOResult	save(const std::basic_string<WCHAR>& fileName, const SaveParameters& params, IFileIOListener* callback = 0);
			FileIOResult	writeRegion(const std::basic_string<WCHAR>& fileName,
								const Region& region, const SaveParameters& params, bool append, IFileIOListener* callback = 0);
			// streams
			Position	insertFromStream(const Position& position, InputStream& in);
			void		writeToStream(OutputStream& out, LineBreakRepresentation lbr = LBR_PHYSICAL_DATA) const;
			void		writeToStream(OutputStream& out, const Region& region, LineBreakRepresentation lbr = LBR_PHYSICAL_DATA) const;
			// operations
			bool	checkTimeStamp();
			bool	renameFile(const WCHAR* newName);
			bool	sendFile(bool asAttachment, bool showDialog = true);
			// utilities
			static bool	areSamePathNames(const WCHAR* s1, const WCHAR* s2) throw();
			static bool	canonicalizePathName(const WCHAR* pathName, WCHAR* dest) throw();

		protected:
			static LineBreak	eatLineBreak(const Char* first, const Char* last);
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

			// „ÉÅEΩÅEΩ„ÔøΩ„ÔøΩ„ÔøΩ„ÉÅE
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

			static class DefaultContentTypeInformationProvider : virtual public IContentTypeInformationProvider {
			public:
				const unicode::IdentifierSyntax& getIdentifierSyntax(ContentType contentType) const throw() {return syntax_;}
			private:
				unicode::IdentifierSyntax syntax_;
			} defaultContentTypeInformationProvider_;

			class ModificationGuard {
			public:
				ModificationGuard(Document& document) throw() : document_(document) {document_.changing_ = true;}
				~ModificationGuard() throw() {document_.changing_= false;}
			private:
				Document& document_;
			};
			friend class ModificationGuard;

			struct DiskFile {
				std::auto_ptr<manah::windows::io::File<true> > lockingFile;
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
			IContentTypeInformationProvider* contentTypeInformationProvider_;
			bool readOnly_;
			bool modified_;
			encodings::CodePage codePage_;
			LineBreak lineBreak_;
			LineList lines_;
			length_t length_;
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
			static LineBreak defaultLineBreak_;

			friend class DocumentPartitioner;
		};

		/**
		 * Random access iterator scans characters in the specified document.
		 * However, this iterator class does *not* have @c operator[].
		 * If an iterator is at the end of line, the dereferece operator returns @c LINE_SEPARATOR.
		 * @note This class is underivable.
		 * @see UTF16To32Iterator, UTF32To16Iterator
		 */
		class DocumentCharacterIterator : public unicode::CharacterIterator {
		public:
			// constructors
			DocumentCharacterIterator() throw();
			DocumentCharacterIterator(const Document& document, const Position& position) throw();
			DocumentCharacterIterator(const DocumentCharacterIterator& rhs) throw();
			// operators for bidirectional iteration
			DocumentCharacterIterator&		operator=(const DocumentCharacterIterator& rhs) throw();
			Char							operator*() const throw();
			DocumentCharacterIterator&		operator++() throw();
			const DocumentCharacterIterator	operator++(int) throw();
			DocumentCharacterIterator&		operator--() throw();
			const DocumentCharacterIterator	operator--(int) throw();
			bool							operator==(const DocumentCharacterIterator& rhs) const throw();
			bool							operator!=(const DocumentCharacterIterator& rhs) const throw();
			// operators for random access iteration
			DocumentCharacterIterator&	operator+=(signed_length_t offset) throw();
			DocumentCharacterIterator&	operator-=(signed_length_t offset) throw();
			DocumentCharacterIterator	operator+(signed_length_t offset) const throw();
			DocumentCharacterIterator	operator-(signed_length_t offset) const throw();
			bool	operator<(const DocumentCharacterIterator& rhs) const throw();
			bool	operator<=(const DocumentCharacterIterator& rhs) const throw();
			bool	operator>(const DocumentCharacterIterator& rhs) const throw();
			bool	operator>=(const DocumentCharacterIterator& rhs) const throw();
			// attributes
			const Document*	getDocument() const throw();
			const String&	getLine() const throw();
			bool			isFirst() const throw();
			bool			isLast() const throw();
			const Position&	tell() const throw();
			// operation
			DocumentCharacterIterator&	seek(const Position& to);
		private:
			std::auto_ptr<CharacterIterator> clone() const {return std::auto_ptr<CharacterIterator>(new DocumentCharacterIterator(*this));}
			void decrement() {--*this;}
			Char dereference() const {return **this;}
			void increment() {++*this;}
		private:
			const Document* document_;
			const String* line_;
			Position p_;
		};

		// free functions
		length_t	getAbsoluteOffset(const Document& document, const Position& at, bool fromAccessibleStart);
		length_t	getLineBreakLength(LineBreak lineBreak);
		const Char*	getLineBreakString(LineBreak lineBreak);
		Position	updatePosition(const Position& position, const DocumentChange& change, Direction gravity);


// inline implementation ////////////////////////////////////////////////////

/**
 * Returns the length of the string represents the specified line break.
 * @param lineBreak the line break
 * @return the length
 * @throw std#invalid_argument @a lineBreak is invalid
 * @see #getLineBreakString
 */
inline length_t getLineBreakLength(LineBreak lineBreak) {
	switch(lineBreak) {
	case LB_CRLF:													return 2;
	case LB_LF: case LB_CR: case LB_NEL: case LB_LS: case LB_PS:	return 1;
	default:														throw std::invalid_argument("Unknown line break specified.");
	}
}

/**
 * Returns the string represents the specified line break.
 * @param lineBreak the line break
 * @return the string. an empty string if @a lineBreak is @c LB_AUTO
 * @throw std#invalid_argument @a lineBreak is invalid
 * @see #getLineBreakLength
 */
inline const Char* getLineBreakString(LineBreak lineBreak) {
	if(lineBreak >= static_cast<LineBreak>(countof(internal::LINE_BREAK_STRINGS)))
		throw std::invalid_argument("Unknown line break specified.");
	return internal::LINE_BREAK_STRINGS[lineBreak];
}

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

/// Clears the undo/redo stacks and deletes the history.
inline void Document::clearUndoBuffer() {
	undoManager_->clear();
	onceUndoBufferCleared_ = true;
}

/// @see #deleteText(const Region&)
inline Position Document::deleteText(const Position& pos1, const Position& pos2) {return deleteText(Region(pos1, pos2));}

/**
 * Returns the line break at the start of the specified buffer.
 * @param first the start of the buffer
 * @param last the end of the buffer
 * @return the line break or @c LB_AUTO if the start of the buffer is not line break
 */
inline LineBreak Document::eatLineBreak(const Char* first, const Char* last) {
	assert(first != 0 && last != 0 && first <= last);
	switch(*first) {
	case LINE_FEED:				return LB_LF;
	case CARRIAGE_RETURN:		return (last - first > 1 && first[1] == LINE_FEED) ? LB_CRLF : LB_CR;
	case NEXT_LINE:				return LB_NEL;
	case LINE_SEPARATOR:		return LB_LS;
	case PARAGRAPH_SEPARATOR:	return LB_PS;
	default:					return LB_AUTO;
	}
}

/// Returns the bookmarker of the document.
inline Bookmarker& Document::getBookmarker() throw() {return *bookmarker_;}

/// Returns the bookmarker of the document.
inline const Bookmarker& Document::getBookmarker() const throw() {return *bookmarker_;}

/// Returns the code page of the document.
inline encodings::CodePage Document::getCodePage() const throw() {return codePage_;}

/// Returns the content information provider.
inline IContentTypeInformationProvider& Document::getContentTypeInformation() const throw() {return *contentTypeInformationProvider_;}

/**
 * Returns the end position of the document.
 * @param accessibleArea true to restrict to the accessible area
 * @return the end position
 * @see #getStartPosition
 */
inline Position Document::getEndPosition(bool accessibleArea /* = true */) const throw() {
	return (accessibleArea && accessibleArea_ != 0) ? *accessibleArea_->second
		: Position(lines_.getSize() - 1, getLineLength(lines_.getSize() - 1));
}

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
 * Returns the count of characters in the document.
 * @param lbr the method to count line breaks
 * @return the count of characters
 * @throw std#invalid_argument @a lbr is invalid
 */
inline length_t Document::getLength(LineBreakRepresentation lbr) const {
	if(lbr == LBR_DOCUMENT_DEFAULT)
		lbr = (getLineBreak() == LB_CRLF) ? LBR_CRLF : LBR_LINE_FEED;
	switch(lbr) {
	case LBR_LINE_FEED:
	case LBR_LINE_SEPARATOR:
		return length_ + getNumberOfLines() - 1;
	case LBR_CRLF:
		return length_ + getNumberOfLines() * 2 - 1;
	case LBR_PHYSICAL_DATA: {
		length_t len = length_;
		const length_t lines = getNumberOfLines();
		for(length_t i = 0; i < lines; ++i)
			len += getLineBreakLength(lines_[i]->lineBreak_);
		return len;
	}
	case LBR_SKIP:
		return length_;
	}
	throw std::invalid_argument("invalid parameter.");
}

/**
 * Returns the text of the specified line.
 * @param line the line
 * @return the text
 * @throw BadPostionException @a line is outside of the document
 */
inline const String& Document::getLine(length_t line) const {return getLineInfo(line).text_;}

/// Returns the line break of the document.
inline LineBreak Document::getLineBreak() const throw() {return lineBreak_;}

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

/// Returns the session to which the document belongs.
inline texteditor::Session* Document::getSession() throw() {return session_;}

/// Returns the session to which the document belongs.
inline const texteditor::Session* Document::getSession() const throw() {return session_;}

/**
 * Returns the start position of the document.
 * @param accessibleArea true to restrict to the accessible area
 * @return the start position
 * @see #getEndPosition
 */
inline Position Document::getStartPosition(bool accessibleArea /* = true */) const throw() {
	return (accessibleArea && accessibleArea_ != 0) ? accessibleArea_->first : Position(0, 0);}

/// ...
inline std::size_t Document::getUndoHistoryLength(bool redo /* = false */) const throw() {
	return redo ? undoManager_->getRedoBufferLength() : undoManager_->getUndoBufferLength();}

/**
 * Inserts the text into the specified position.
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
inline Position Document::insertText(const Position& position, const String& text) {
	return insertText(position, text.data(), text.data() + text.length());}

/// Returns true if the document is bound to any file.
inline bool Document::isBoundToFile() const throw() {return getFilePathName() != 0;}

/**
 * Returns true if the document has been modified.
 * @see #setModified, #IStateListener#documentModificationSignChanged
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
 * Transfers the partitioning change to the listeners.
 * @param changedRegion the changed region
 */
inline void Document::partitioningChanged(const Region& changedRegion) throw() {
	partitioningListeners_.notify<const Region&>(IDocumentPartitioningListener::documentPartitioningChanged, changedRegion);}

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
 * @param newProvider the new content type information provider
 */
inline void Document::setContentTypeInformation(IContentTypeInformationProvider* newProvider) throw() {
	contentTypeInformationProvider_ = (newProvider != 0) ? newProvider : &defaultContentTypeInformationProvider_;}

/**
 * Sets the line break of the document.
 * @param lineBreak the line break
 * @throw std#invalid_argument @a lineBreak is invalid
 */
inline void Document::setLineBreak(LineBreak lineBreak) {
	switch(lineBreak) {
	case LB_LF:		case LB_CR:
	case LB_CRLF:	case LB_NEL:
	case LB_LS:		case LB_PS:
		if(lineBreak != lineBreak_) {
			lineBreak_ = lineBreak;
			stateListeners_.notify<Document&>(IDocumentStateListener::documentEncodingChanged, *this);
		}
		break;
	default:
		throw std::invalid_argument("Unknown line break specified.");
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
 * @param lbr the line break to be used
 */
inline void Document::writeToStream(OutputStream& out, LineBreakRepresentation lbr /* = LBP_PHYSICAL_DATA */) const {
	writeToStream(out, Region(Position(0, 0), Position(getNumberOfLines() - 1, getLineLength(getNumberOfLines() - 1))), lbr);}


/**
 * Returns the content type of the partition contains the specified position.
 * @param at the position
 * @throw BadPositionException @a position is outside of the document
 * @throw std#logic_error the partitioner is not connected to any document
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
 * @throw std#logic_error the partitioner is not connected to any document
 */
inline void DocumentPartitioner::getPartition(const Position& at, DocumentPartition& partition) const {
	if(document_ == 0)
		throw std::logic_error("the partitioner is not connected to any document.");
	else if(at > document_->getEndPosition(false))
		throw BadPositionException();
	return doGetPartition(at, partition);
}

/**
 * Notifies the partitioning change to the listeners.
 * Implementation of @c DocumentPartitioner *must* call this when the partitioning is changed.
 * @param changedRegion the changed region
 * @throw std#logic_error the partitioner is not connected any document
 */
inline void DocumentPartitioner::notifyDocument(const Region& changedRegion) {
	if(document_ == 0)
		throw std::logic_error("the partitioner is not connected any document.");
	document_->partitioningChanged(changedRegion);	// $friendly-access
}

/// Default constructor.
inline DocumentCharacterIterator::DocumentCharacterIterator() throw() : unicode::CharacterIterator(0), document_(0), line_(0) {}

/**
 * Constructor.
 * @param document the document to be iterated
 * @param position the position to start
 */
inline DocumentCharacterIterator::DocumentCharacterIterator(const Document& document, const Position& position) throw() :
		unicode::CharacterIterator(getAbsoluteOffset(document, position, false)),
		document_(&document), line_(&document.getLine(position.line)), p_(position) {}

/// Copy-constructor.
inline DocumentCharacterIterator::DocumentCharacterIterator(const DocumentCharacterIterator& rhs) throw()
	: unicode::CharacterIterator(rhs), document_(rhs.document_), line_(rhs.line_), p_(rhs.p_) {}

/// Assignment operator.
inline DocumentCharacterIterator& DocumentCharacterIterator::operator=(const DocumentCharacterIterator& rhs) throw() {
	unicode::CharacterIterator::operator=(rhs); document_ = rhs.document_; line_ = rhs.line_; p_ = rhs.p_; return *this;}

/// Dereference iterator. If the iterator addresses the end of the document, the result is undefined.
inline Char DocumentCharacterIterator::operator*() const throw() {
	return (p_.column < line_->length()) ? (*line_)[p_.column] : LINE_SEPARATOR;}

/// Prefix increment operator.
inline DocumentCharacterIterator& DocumentCharacterIterator::operator++() throw() {
	if(p_.column < line_->length()) ++p_.column;
	else if(p_.line < document_->getNumberOfLines() - 1) {line_ = &document_->getLine(++p_.line); p_.column = 0;}
	return *this;
}

/// Postfix increment operator.
inline const DocumentCharacterIterator
DocumentCharacterIterator::operator++(int) throw() {DocumentCharacterIterator temp(*this); ++*this; return temp;}

/// Prefix decrement operator.
inline DocumentCharacterIterator& DocumentCharacterIterator::operator--() throw() {
	if(p_.column > 0) --p_.column;
	else if(p_.line > 0) p_.column = (line_ = &document_->getLine(--p_.line))->length();
	return *this;
}

/// Postfix decrement operator.
inline const DocumentCharacterIterator
DocumentCharacterIterator::operator--(int) throw() {DocumentCharacterIterator temp(*this); --*this; return temp;}

/// Equality operator.
inline bool DocumentCharacterIterator::operator==(const DocumentCharacterIterator& rhs) const throw() {return p_ == rhs.p_;}

/// Inequality operator.
inline bool DocumentCharacterIterator::operator!=(const DocumentCharacterIterator& rhs) const throw() {return p_ != rhs.p_;}

/// Additive operator.
inline DocumentCharacterIterator DocumentCharacterIterator::operator+(
	signed_length_t offset) const throw() {DocumentCharacterIterator temp(*this); return temp += offset;}

/// Additive operator.
inline DocumentCharacterIterator DocumentCharacterIterator::operator-(
	signed_length_t offset) const throw() {DocumentCharacterIterator temp(*this); return temp -= offset;}

/// Relational operator.
inline bool DocumentCharacterIterator::operator<(const DocumentCharacterIterator& rhs) const throw() {return p_ < rhs.p_;}

/// Relational operator.
inline bool DocumentCharacterIterator::operator<=(const DocumentCharacterIterator& rhs) const throw() {return p_ <= rhs.p_;}

/// Relational operator.
inline bool DocumentCharacterIterator::operator>(const DocumentCharacterIterator& rhs) const throw() {return p_ > rhs.p_;}

/// Relational operator.
inline bool DocumentCharacterIterator::operator>=(const DocumentCharacterIterator& rhs) const throw() {return p_ >= rhs.p_;}

/// Returns the document.
inline const Document* DocumentCharacterIterator::getDocument() const throw() {return document_;}

/// Returns the line.
inline const String& DocumentCharacterIterator::getLine() const throw() {return *line_;}

/// @see unicode#CharacterIterator#isFirst
inline bool DocumentCharacterIterator::isFirst() const throw() {return p_.line == 0 && p_.column == 0;}

/// @see unicode#CharacterIterator#isLast
inline bool DocumentCharacterIterator::isLast() const throw() {
	return p_.line == document_->getNumberOfLines() - 1 && p_.column == line_->length();}

/**
 * Moves to the specified position.
 * @param to the position
 * @throw BadPositionException @a to is outside of the document
 */
inline DocumentCharacterIterator& DocumentCharacterIterator::seek(const Position& to) {
	if(to > document_->getEndPosition(false)) throw BadPositionException(); line_ = &document_->getLine((p_ = to).line); return *this;}

/// Returns the document position the iterator addresses.
inline const Position& DocumentCharacterIterator::tell() const throw() {return p_;}

}} // namespace ascension::text

#endif /* !ASCENSION_DOCUMENT_HPP */
