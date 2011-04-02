/**
 * @file document.hpp
 * @author exeal
 * @date 2003-2006 (was EditDoc.h)
 * @date 2006-2011
 */

#ifndef ASCENSION_DOCUMENT_HPP
#define ASCENSION_DOCUMENT_HPP

#include <ascension/config.hpp>				// ASCENSION_DEFAULT_NEWLINE
#include <ascension/corelib/basic-exceptions.hpp>
#include <ascension/corelib/basic-types.hpp>
#include <ascension/corelib/gap-vector.hpp>	// detail.GapVector
#include <ascension/corelib/listeners.hpp>
#include <ascension/corelib/memory.hpp>		// FastArenaObject
#include <ascension/corelib/standard-iterator-adapter.hpp>	// detail.IteratorAdapter
#include <ascension/corelib/string-piece.hpp>
#include <ascension/kernel/document-observers.hpp>
#include <ascension/kernel/partition.hpp>
#ifdef ASCENSION_POSIX
#	include <sys/stat.h>	// for POSIX environment
#endif
#include <iostream>
#include <map>
#include <set>

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
		 * Value represents a newline in document. @c NLF_RAW_VALUE and @c NLF_DOCUMENT_INPUT are
		 * special values indicate how to interpret newlines during any text I/O.
		 * @see newlineStringLength, newlineString, Document, ASCENSION_DEFAULT_NEWLINE,
		 *      NEWLINE_CHARACTERS
		 */
		enum Newline {
			/// Line feed. Standard of Unix (Lf, U+000A).
			NLF_LINE_FEED = 0,
			/// Carriage return. Old standard of Macintosh (Cr, U+000D).
			NLF_CARRIAGE_RETURN,
			/// CR+LF. Standard of Windows (CrLf, U+000D U+000A).
			NLF_CR_LF,
			/// Next line. Standard of EBCDIC-based OS (U+0085).
			NLF_NEXT_LINE,
			/// Line separator (U+2028).
			NLF_LINE_SEPARATOR,
			/// Paragraph separator (U+2029).
			NLF_PARAGRAPH_SEPARATOR,
			/// Represents any NLF as the actual newline of the line (@c Document#Line#newline()).
			NLF_RAW_VALUE,
			/// Represents any NLF as the value of @c IDocumentInput#newline().
			NLF_DOCUMENT_INPUT,
			NLF_COUNT
		};

		/**
		 * A changed content of the document.
		 * @see IDocumentListener, PositionUpdater
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
		 * @see ReadOnlyDocumentException, IDocumentInput#ChangeRejectedException
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
		};

		/**
		 * Thrown when the caller accessed inaccessible region of the document.
		 * Document#accessibleRegion, Document#erase, Document#insert
		 */
		class DocumentAccessViolationException : public DocumentCantChangeException, public std::invalid_argument {
		public:
			DocumentAccessViolationException();
		};

		/**
		 * Provides information about a document input.
		 * @see Document
		 */
		class DocumentInput {
		public:
			/**
			 * Thrown if @c IDocumentInput rejected the change of the document. For details, see
			 * the documentation of @c Document class.
			 * @see Document#redo, Document#replace, Document#resetContent, Document#undo,
			 *      IDocumentInput#documentAboutToBeChanged
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
			/// @c NLF_RAW_VALUE nor @c NLF_DOCUMENT_INPUT.
			virtual Newline newline() const /*throw()*/ = 0;
		private:
			virtual bool isChangeable(const Document& document) const /*throw()*/ = 0;
			virtual void postFirstDocumentChange(const Document& document) /*throw()*/ = 0;
			friend class Document;
		};

		/**
		 * A @c Bookmarker manages bookmarks of the document.
		 * @note This class is not intended to be subclassed.
		 * @see Document#bookmarker, EditPoint#forwardBookmark, EditPoint#backwardBookmark
		 */
		class Bookmarker : private DocumentListener {
			ASCENSION_NONCOPYABLE_TAG(Bookmarker);
		public:
			/// A @c Bookmarker#Iterator enumerates the all marked lines.
			class Iterator : public detail::IteratorAdapter<
				Iterator, std::iterator<std::bidirectional_iterator_tag, length_t> > {
			public:
				// detail.IteratorAdapter requirements
				value_type current() const {return *impl_;}
				bool equals(const Iterator& other) const {return impl_ == other.impl_;}
				bool less(const Iterator& other) const {return impl_ < other.impl_;}
				void next() {++impl_;}
				void previous() {--impl_;}
			private:
				Iterator(detail::GapVector<length_t>::const_iterator impl) : impl_(impl) {}
				detail::GapVector<length_t>::const_iterator impl_;
				friend class Bookmarker;
			};
			// destructor
			~Bookmarker() /*throw()*/;
			// listeners
			void addListener(BookmarkListener& listener);
			void removeListener(BookmarkListener& listener);
			// attributes
			bool isMarked(length_t line) const;
			length_t next(length_t from, Direction direction, bool wrapAround = true, std::size_t marks = 1) const;
			std::size_t numberOfMarks() const /*throw()*/;
			// enumerations
			Iterator begin() const;
			Iterator end() const;
			// operations
			void clear() /*throw()*/;
			void mark(length_t line, bool set = true);
			void toggle(length_t line);
		private:
			detail::GapVector<length_t>::iterator find(length_t line) const /*throw()*/;
			// IDocumentListener
			void documentAboutToBeChanged(const Document& document);
			void documentChanged(const Document& document, const DocumentChange& change);
		private:
			explicit Bookmarker(Document& document) /*throw()*/;
			Document& document_;
			detail::GapVector<length_t> markedLines_;
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
				Newline newline() const /*throw()*/ {return newline_;}
				/// Returns the revision number when this last was changed previously.
				std::size_t revisionNumber() const /*throw()*/ {return revisionNumber_;}
				/// Returns the text of the line.
				const String& text() const /*throw()*/ {return text_;}
			private:
				explicit Line(std::size_t revisionNumber) /*throw()*/;
				Line(std::size_t revisionNumber, const String& text, Newline newline = ASCENSION_DEFAULT_NEWLINE);
				String text_;
				Newline newline_;
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
			DocumentInput* input() const /*throw()*/;
			bool isModified() const /*throw()*/;
			bool isReadOnly() const /*throw()*/;
			void markUnmodified() /*throw()*/;
			const DocumentPartitioner& partitioner() const /*throw()*/;
			const String* property(const DocumentPropertyKey& key) const /*throw()*/;
			texteditor::Session* session() /*throw()*/;
			const texteditor::Session* session() const /*throw()*/;
			void setInput(DocumentInput* newInput, bool delegateOwnership) /*throw()*/;
			void setModified() /*throw()*/;
			void setPartitioner(std::auto_ptr<DocumentPartitioner> newPartitioner) /*throw()*/;
			void setProperty(const DocumentPropertyKey& key, const String& property);
			void setReadOnly(bool readOnly = true) /*throw()*/;
			// contents
			Region accessibleRegion() const /*throw()*/;
			const Line& getLineInformation(length_t line) const;
			length_t length(Newline newline = NLF_RAW_VALUE) const;
			const String& line(length_t line) const;
			length_t lineLength(length_t line) const;
			length_t lineOffset(length_t line, Newline newline = NLF_RAW_VALUE) const;
			length_t numberOfLines() const /*throw()*/;
			Region region() const /*throw()*/;
			std::size_t revisionNumber() const /*throw()*/;
			// content type information
			ContentTypeInformationProvider& contentTypeInformation() const /*throw()*/;
			void setContentTypeInformation(std::auto_ptr<ContentTypeInformationProvider> newProvider) /*throw()*/;
			// manipulations
			bool isChanging() const /*throw()*/;
			void replace(const Region& region, const StringPiece& text, Position* eos = 0);
			void replace(const Region& region, std::basic_istream<Char>& in, Position* eos = 0);
#if 0
			// locks
			bool lock(const void* locker);
			const void* locker() const /*throw()*/;
			void unlock(const void* locker);
#endif
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
			detail::StrategyPointer<DocumentInput> input_;
			std::auto_ptr<DocumentPartitioner> partitioner_;
			std::auto_ptr<Bookmarker> bookmarker_;
			std::auto_ptr<ContentTypeInformationProvider> contentTypeInformationProvider_;
			bool readOnly_;
			LineList lines_;
			length_t length_;
			std::size_t revisionNumber_, lastUnmodifiedRevisionNumber_;
			std::set<Point*> points_;
			UndoManager* undoManager_;
			std::map<const DocumentPropertyKey*, String*> properties_;
			bool onceUndoBufferCleared_, recordingChanges_, changing_, rollbacking_;

			std::pair<Position, Point*>* accessibleArea_;

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

		// free functions about newlines
		template<typename ForwardIterator> length_t calculateNumberOfLines(ForwardIterator first, ForwardIterator last);
		length_t calculateNumberOfLines(const String& text) /*throw()*/;
		template<typename ForwardIterator> Newline eatNewline(ForwardIterator first, ForwardIterator last);
		bool isLiteralNewline(Newline newline);
		const Char* newlineString(Newline newline);
		length_t newlineStringLength(Newline newline);

		// free functions to change document
		void erase(Document& document, const Region& region);
		void erase(Document& document, const Position& first, const Position& second);
		void insert(Document& document, const Position& at, const StringPiece& text, Position* endOfInsertedString = 0);
		void insert(Document& document, const Position& at, std::basic_istream<Char>& in, Position* endOfInsertedString = 0);

		// other free functions related to document
		std::basic_ostream<Char>& writeDocumentToStream(std::basic_ostream<Char>& out,
			const Document& document, const Region& region, Newline newline = NLF_RAW_VALUE);

		namespace positions {
			length_t absoluteOffset(const Document& document, const Position& at, bool fromAccessibleStart);
//			bool isOutsideOfAccessibleRegion(const Document& document, const Position& position) /*throw()*/;
			bool isOutsideOfDocumentRegion(const Document& document, const Position& position) /*throw()*/;
			Position shrinkToAccessibleRegion(const Document& document, const Position& position) /*throw()*/;
			Region shrinkToAccessibleRegion(const Document& document, const Region& region) /*throw()*/;
			Position shrinkToDocumentRegion(const Document& document, const Position& position) /*throw()*/;
			Region shrinkToDocumentRegion(const Document& document, const Region& region) /*throw()*/;
			Position updatePosition(const Position& position, const DocumentChange& change, Direction gravity) /*throw()*/;
		} // namespace positions


// inline implementation ////////////////////////////////////////////////////

/**
 * Returns the number of lines in the specified character sequence.
 * This method is exception-neutral (does not throw if @a ForwardIterator does not).
 * @param first the beginning of the character sequence
 * @param last the end of the character sequence
 * @return the number of lines. zero if and only if the input sequence is empty
 */
template<typename ForwardIterator>
inline length_t calculateNumberOfLines(ForwardIterator first, ForwardIterator last) {
	if(first == last)
		return 0;
	length_t lines = 1;
	while(true) {
		first = std::find_first_of(first, last, NEWLINE_CHARACTERS, ASCENSION_ENDOF(NEWLINE_CHARACTERS));
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
inline length_t calculateNumberOfLines(const String& text) /*throw()*/ {return calculateNumberOfLines(text.begin(), text.end());}

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

/// Calls @c Document#replace.
inline void erase(Document& document, const Region& region) {return document.replace(region, 0, 0);}

/// Calls @c Document#replace.
inline void erase(Document& document, const Position& first, const Position& second) {return erase(document, Region(first, second));}

/// Calls @c Document#replace.
inline void insert(Document& document, const Position& at, const StringPiece& text,
	Position* endOfInsertedString /* = 0 */) {return document.replace(Region(at), text, endOfInsertedString);}

/// Calls @c Document#replace.
inline void insert(Document& document, const Position& at, std::basic_istream<Char>& in,
	Position* endOfInsertedString /* = 0 */) {return document.replace(Region(at), in, endOfInsertedString);}

/**
 * Returns @c true if the given newline value is a literal.
 * @throw UnknownValueException @a newline is invalid (undefined value)
 */
inline bool isLiteralNewline(Newline newline) /*throw()*/ {
#if NLF_LINE_FEED != 0 //|| NLF_COUNT != 8
#	error "Check the definition of Newline and revise this code."
#endif
	if(newline < NLF_LINE_FEED || newline >= NLF_COUNT)
		throw UnknownValueException("newline");
	return newline <= NLF_PARAGRAPH_SEPARATOR;
}

/**
 * Returns the null-terminated string represents the specified newline.
 * @param newline the newline
 * @return the string
 * @throw std#invalid_argument @a newline is not a literal value
 * @throw UnknownValueException @a newline is undefined
 * @see #newlineStringLength
 */
inline const Char* newlineString(Newline newline) {
	switch(newline) {
	case NLF_LINE_FEED:				return L"\n";
	case NLF_CARRIAGE_RETURN:		return L"\r";
	case NLF_CR_LF:					return L"\r\n";
	case NLF_NEXT_LINE:				return L"\x0085";
	case NLF_LINE_SEPARATOR:		return L"\x2028";
	case NLF_PARAGRAPH_SEPARATOR:	return L"\x2029";
	default:
		if(newline < NLF_COUNT)		throw std::invalid_argument("newline");
		else						throw UnknownValueException("newline");
	}
}

/**
 * Returns the length of the string represents the specified newline.
 * @param newline the newline
 * @return the length
 * @throw std#invalid_argument @a newline is not a literal value
 * @throw UnknownValueException @a newline is undefined
 * @see #newlineString
 */
inline length_t newlineStringLength(Newline newline) {
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
		if(newline < NLF_COUNT)
			throw std::invalid_argument("newline");
		else
			throw UnknownValueException("newline");
	}
}

/// Returns @c true if the given position is outside of the document.
inline bool positions::isOutsideOfDocumentRegion(const Document& document, const Position& position) /*throw()*/ {
	return position.line >= document.numberOfLines() || position.column > document.lineLength(position.line);}

/** 
 * Shrinks the given position into the accessible region of the document.
 * @param document the document
 * @param position the source position. this value can be outside of the document
 * @return the result
 */
inline Position positions::shrinkToAccessibleRegion(const Document& document, const Position& position) /*throw()*/ {
	if(position == Position())
		return position;
	else if(!document.isNarrowed())
		return shrinkToDocumentRegion(document, position);
	const Region accessibleRegion(document.accessibleRegion());
	if(position < accessibleRegion.first)
		return accessibleRegion.first;
	else if(position > accessibleRegion.second)
		return accessibleRegion.second;
	return Position(position.line, std::min(position.column, document.lineLength(position.line)));
}

/** 
 * Shrinks the given region into the accessible region of the document.
 * @param document the document
 * @param region the source region. this value can intersect with outside of the document
 * @return the result. this may not be normalized
 */
inline Region positions::shrinkToAccessibleRegion(const Document& document, const Region& region) /*throw()*/ {
	return Region(shrinkToAccessibleRegion(document, region.first), shrinkToAccessibleRegion(document, region.second));}

/// Shrinks the given position into the document region.
inline Position positions::shrinkToDocumentRegion(const Document& document, const Position& position) /*throw()*/ {
	if(position == Position())
		return position;
	Position p(std::min(position.line, document.numberOfLines() - 1), 0);
	p.column = std::min(position.column, document.lineLength(p.line));
	return p;
}

/// Shrinks the given region into the document region. The result may not be normalized.
inline Region positions::shrinkToDocumentRegion(const Document& document, const Region& region) /*throw()*/ {
	return Region(shrinkToDocumentRegion(document, region.first), shrinkToDocumentRegion(document, region.second));}

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
inline const Document::Line& Document::getLineInformation(length_t line) const {
	if(line >= lines_.size()) throw BadPositionException(Position(line, 0)); return *lines_[line];}

#if 0
inline Document::LineIterator Document::getLineIterator(length_t line) const {
	assertValid();
	if(line >= lines_.getSize())
		throw BadPositionException();
	return lines_.begin() + line;
}
#endif

/// Returns the document input or @c null.
inline DocumentInput* Document::input() const /*throw()*/ {return input_.get();}

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
inline bool Document::isNarrowed() const /*throw()*/ {return accessibleArea_ != 0;}

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
inline const String& Document::line(length_t line) const {return getLineInformation(line).text_;}

/**
 * Returns the length of the specified line. The line break is not included.
 * @param line the line
 * @return the length of @a line
 * @throw BadLocationException @a line is outside of the document
 */
inline length_t Document::lineLength(length_t line) const {return this->line(line).length();}
#if 0
/// Returns the object locks the document or @c null if the document is not locked.
inline const void* Document::locker() const /*throw()*/ {return locker_;}
#endif
/// Returns the number of lines in the document.
inline length_t Document::numberOfLines() const /*throw()*/ {return lines_.size();}

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
inline void Document::setContentTypeInformation(std::auto_ptr<ContentTypeInformationProvider> newProvider) /*throw()*/ {
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
