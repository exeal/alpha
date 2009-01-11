/**
 * @file point.hpp
 * @author exeal
 * @date 2003-2008
 */

#ifndef ASCENSION_POINT_HPP
#define ASCENSION_POINT_HPP
#include "document.hpp"

namespace ascension {

	namespace kernel {

		/**
		 * Interface for objects which are interested in lifecycle of the point.
		 * @see Point#addLifeCycleListener, Point#removeLifeCycleListener, IPointListener
		 */
		class IPointLifeCycleListener {
		protected:
			/// Destructor.
			virtual ~IPointLifeCycleListener() /*throw()*/ {}
		private:
			/// The point was destroyed. After this, don't call @c Point#addLifeCycleListener.
			virtual void pointDestroyed() = 0;
			friend class Point;
		};

		// documentation is point.cpp
		class Point {
			MANAH_UNASSIGNABLE_TAG(Point);
		public:
			// constructors
			explicit Point(Document& document, const Position& position = Position());
			Point(const Point& rhs);
			virtual ~Point() /*throw()*/;
			// operators
			operator Position() /*throw()*/;
			operator const Position() const /*throw()*/;
			bool operator==(const Point& rhs) const /*throw()*/;
			bool operator!=(const Point& rhs) const /*throw()*/;
			bool operator<(const Point& rhs) const /*throw()*/;
			bool operator<=(const Point& rhs) const /*throw()*/;
			bool operator>(const Point& rhs) const /*throw()*/;
			bool operator>=(const Point& rhs) const /*throw()*/;
			// core attributes
			Document* document() /*throw()*/;
			const Document* document() const /*throw()*/;
			bool isDocumentDisposed() const /*throw()*/;
			const Position& position() const /*throw()*/;
			// behaviors
			bool adaptsToDocument() const /*throw()*/;
			Point& adaptToDocument(bool adapt) /*throw()*/;
			Point& excludeFromRestriction(bool exclude);
			Direction gravity() const /*throw()*/;
			bool isExcludedFromRestriction() const /*throw()*/;
			Point& setGravity(Direction gravity) /*throw()*/;
			// listeners
			void addLifeCycleListener(IPointLifeCycleListener& listener);
			void removeLifeCycleListener(IPointLifeCycleListener& listener);
			// short-circuits
			length_t columnNumber() const /*throw()*/;
			ContentType getContentType() const;
			length_t lineNumber() const /*throw()*/;
			// operations
			void moveTo(const Position& to);
			void moveTo(length_t line, length_t column);

		protected:
			Point& operator=(const Position& rhs) /*throw()*/;
			void documentDisposed() /*throw()*/;
			virtual void doMoveTo(const Position& to);
			void normalize() const;
			Position normalized() const;
			virtual void update(const DocumentChange& change);
			void verifyDocument() const;
		private:
			Document* document_;
			Position position_;
			bool adapting_;
			bool excludedFromRestriction_;
			Direction gravity_;
			ascension::internal::Listeners<IPointLifeCycleListener> lifeCycleListeners_;
			friend class Document;
		};
	}

	namespace viewers {class VisualPoint;}

	namespace kernel {
		class EditPoint;

		/**
		 * A listener for @c EditPoint and @c viewers#VisualPoint.
		 * @see IPointLifeCycleListener, viewers#ICaretListener
		 */
		class IPointListener {
		private:
			/**
			 * The point was moved.
			 * @param self the point
			 * @param oldPosition the position from which the point moved
			 */
			virtual void pointMoved(const EditPoint& self, const Position& oldPosition) = 0;
			friend class EditPoint;
			friend class ascension::viewers::VisualPoint;
		};

		// document is point.cpp
		class EditPoint : public Point {
		public:
			/// Character unit defines what is one character.
			enum CharacterUnit {
				UTF16_CODE_UNIT,	///< UTF-16 code unit.
				UTF32_CODE_UNIT,	///< UTF-32 code unit. A surrogate pair is treated as one character.
				GRAPHEME_CLUSTER,	///< A grapheme cluster is a character.
				GLYPH_CLUSTER,		///< A glyph is a character. (not implemented).
				DEFAULT_UNIT		///< Default behavior. Some methods reject this value.
			};

			// constructors
			explicit EditPoint(Document& document, const Position& position = Position(), IPointListener* listener = 0);
			EditPoint(const EditPoint& rhs);
			virtual ~EditPoint() /*throw()*/;
			// attributes
			CodePoint character(bool useLineFeed = false) const;
			CharacterUnit characterUnit() const /*throw()*/;
			bool isBeginningOfDocument() const;
			bool isBeginningOfLine() const;
			bool isEndOfDocument() const;
			bool isEndOfLine() const;
			EditPoint& setCharacterUnit(CharacterUnit unit);
			// movement destinations
			Position backwardBookmark(length_t marks = 1) const;
			Position backwardCharacter(length_t offset = 1) const;
			Position backwardLine(length_t lines = 1) const;
			Position backwardWord(length_t words = 1) const;
			Position backwardWordEnd(length_t words = 1) const;
			Position beginningOfDocument() const;
			Position beginningOfLine() const;
			Position endOfDocument() const;
			Position endOfLine() const;
			Position forwardBookmark(length_t marks = 1) const;
			Position forwardCharacter(length_t offset = 1) const;
			Position forwardLine(length_t lines = 1) const;
			Position forwardWord(length_t words = 1) const;
			Position forwardWordEnd(length_t words = 1) const;
//			void moveToAbsoluteCharacterOffset(length_t offset);
			// text manipulations
			bool destructiveInsert(const String& text, bool keepNewline = true);
			bool destructiveInsert(const Char* first, const Char* last, bool keepNewline = true);
			bool erase(signed_length_t length = 1, CharacterUnit cu = DEFAULT_UNIT);
			bool insert(const String& text);
			bool insert(const Char* first, const Char* last);
			virtual bool newLine(std::size_t newlines = 1);
			bool transposeCharacters();
			bool transposeLines();
//			bool transposeParagraphs();
//			bool transposeSentences();
			bool transposeWords();

		protected:
			virtual void doMoveTo(const Position& to);
			const text::IdentifierSyntax& identifierSyntax() const;
			Position offsetCharacterPosition(Direction direction, length_t offset = 1, CharacterUnit cu = DEFAULT_UNIT) const;
			static Position offsetCharacterPosition(const Document& document,
				const Position& position, Direction direction, CharacterUnit cu, length_t offset = 1);
			IPointListener* getListener() const /*throw()*/;
			String getText(signed_length_t length, Newline newline = NLF_RAW_VALUE) const;
			String getText(const Position& other, Newline newline = NLF_RAW_VALUE) const;
		private:
			using Point::excludeFromRestriction;	// explicit hide
		private:
			IPointListener* listener_;
			CharacterUnit characterUnit_;
		};


		// inline implementations ///////////////////////////////////////////

		/// Conversion operator for convenience.
		inline Point::operator Position() /*throw()*/ {return position_;}
		/// Conversion operator for convenience.
		inline Point::operator const Position() const /*throw()*/ {return position_;}
		/**
		 * Protected assignment operator moves the point to @a rhs.
		 * @see #moveTo
		 */
		inline Point& Point::operator=(const Position& rhs) /*throw()*/ {position_ = rhs; return *this;}
		/// Equality operator.
		inline bool Point::operator==(const Point& rhs) const /*throw()*/ {return position() == rhs.position();}
		/// Unequality operator.
		inline bool Point::operator!=(const Point& rhs) const /*throw()*/ {return !(*this == rhs);}
		/// Less-than operator.
		inline bool Point::operator<(const Point& rhs) const /*throw()*/ {return position() < rhs.position();}
		/// Less-than-or-equal-to operator.
		inline bool Point::operator<=(const Point& rhs) const /*throw()*/ {return *this < rhs || *this == rhs;}
		/// Greater-than operator.
		inline bool Point::operator>(const Point& rhs) const /*throw()*/ {return !(*this >= rhs);}
		/// Greater-than-or-equal-to operator.
		inline bool Point::operator>=(const Point& rhs) const /*throw()*/ {return !(*this > rhs);}
		/// Returns true if the point is adapting to the document change.
		inline bool Point::adaptsToDocument() const /*throw()*/ {return adapting_;}
		/// Adapts the point to the document change.
		inline Point& Point::adaptToDocument(bool adapt) /*throw()*/ {adapting_ = adapt; return *this;}
		/// Returns the column.
		inline length_t Point::columnNumber() const /*throw()*/ {return position_.column;}
		/// Returns the document or @c null if the document is already disposed.
		inline Document* Point::document() /*throw()*/ {return document_;}
		/// Returns the document or @c null if the document is already disposed.
		inline const Document* Point::document() const /*throw()*/ {return document_;}
		/// Called when the document is disposed.
		inline void Point::documentDisposed() /*throw()*/ {document_ = 0;}
		/// ...
		inline Point& Point::excludeFromRestriction(bool exclude) {verifyDocument(); if(excludedFromRestriction_ = exclude) normalize(); return *this;}
		/// Returns the content type of the document partition contains the point.
		inline ContentType Point::getContentType() const {verifyDocument(); return document_->partitioner().contentType(*this);}
		/// Returns the gravity.
		inline Direction Point::gravity() const /*throw()*/ {return gravity_;}
		/// Returns true if the document is already disposed.
		inline bool Point::isDocumentDisposed() const /*throw()*/ {return document_ == 0;}
		/// Returns true if the point can't enter the inaccessible area of the document.
		inline bool Point::isExcludedFromRestriction() const /*throw()*/ {return excludedFromRestriction_;}
		/// Returns the line number.
		inline length_t Point::lineNumber() const /*throw()*/ {return position_.line;}
		/// Moves to the specified position.
		inline void Point::moveTo(length_t line, length_t column) {moveTo(Position(line, column));}
		/**
		 * Normalizes the position of the point.
		 * This method does <strong>not</strong> inform to the listeners about any movement.
		 */
		inline void Point::normalize() const {const_cast<Point*>(this)->position_ = normalized();}
		/// Returns the normalized position of the point.
		inline Position Point::normalized() const {
			verifyDocument();
			if(document_->isNarrowed() && isExcludedFromRestriction()) {
				const Region accessibleRegion(document_->accessibleRegion());
				return std::max(std::min(position_, accessibleRegion.second), accessibleRegion.first);
			} else
				return std::min(position_, document_->region().second);
		}
		/// Returns the position.
		inline const Position& Point::position() const /*throw()*/ {return position_;}
		/// Sets the gravity.
		inline Point& Point::setGravity(Direction gravity) /*throw()*/ {verifyDocument(); gravity_ = gravity; return *this;}
		/// Throws @c DisposedDocumentException if the document is already disposed.
		inline void Point::verifyDocument() const {if(isDocumentDisposed()) throw DisposedDocumentException();}

		/// Returns the character unit.
		inline EditPoint::CharacterUnit EditPoint::characterUnit() const /*throw()*/ {return characterUnit_;}
		/**
		 * Deletes the current character and inserts the specified text.
		 * @param text the text to be inserted
		 * @param keepNewline set false to overwrite a newline characer
		 * @return false if the change was interrupted
		 * @throw ReadOnlyDocumentException the document is read-only
		 */
		inline bool EditPoint::destructiveInsert(const String& text, bool keepNewline /* = true */) {
			return destructiveInsert(text.data(), text.data() + text.length(), keepNewline);}
		/// Returns the listener.
		inline IPointListener* EditPoint::getListener() const /*throw()*/ {return const_cast<EditPoint*>(this)->listener_;}
		inline const text::IdentifierSyntax& EditPoint::identifierSyntax() const {
			return document()->contentTypeInformation().getIdentifierSyntax(getContentType());}
		/**
		 * Inserts the specified text at the current position.
		 * @param text the text to be inserted
		 * @return false if the change was interrupted
		 * @throw ReadOnlyDocumentException the document is read only
		 */
		inline bool EditPoint::insert(const String& text) {return insert(text.data(), text.data() + text.length());}
		/***/
		inline Position EditPoint::offsetCharacterPosition(Direction direction, length_t offset /* = 1 */, CharacterUnit cu /* = DEFAULT_UNIT */) const {
			return offsetCharacterPosition(*document(), position(), direction, (cu != DEFAULT_UNIT) ? cu : characterUnit(), offset);}

	} // namespace kernel
} // namespace ascension

#endif // !ASCENSION_POINT_HPP
