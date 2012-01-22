/**
 * @file point.hpp
 * @author exeal
 * @date 2003-2012
 */

#ifndef ASCENSION_POINT_HPP
#define ASCENSION_POINT_HPP
#include <ascension/kernel/document.hpp>

namespace ascension {

	namespace kernel {

		/**
		 * Tried to use some object but the document used by the object had already been disposed.
		 * @see Point
		 */
		class DocumentDisposedException : public IllegalStateException {
		public:
			DocumentDisposedException();
		};

		/**
		 * A listener for @c Point.
		 * @see PointLifeCycleListener, viewers#CaretListener, Point#Point
		 */
		class PointListener {
		private:
			/**
			 * The point was moved.
			 * @param self The point
			 * @param oldPosition The position from which the point moved
			 */
			virtual void pointMoved(const Point& self, const Position& oldPosition) = 0;
			friend class Point;
		};

		/**
		 * Interface for objects which are interested in lifecycle of the point.
		 * @see Point#addLifeCycleListener, Point#removeLifeCycleListener, PointListener
		 */
		class PointLifeCycleListener {
		protected:
			/// Destructor.
			virtual ~PointLifeCycleListener() /*throw()*/ {}
		private:
			/// The point was destroyed. After this, don't call @c Point#addLifeCycleListener.
			virtual void pointDestroyed() = 0;
			friend class Point;
		};

		// documentation is point.cpp
		class Point {
			ASCENSION_UNASSIGNABLE_TAG(Point);
		public:
			// constructors
			explicit Point(Document& document,
				const Position& position = Position(), PointListener* listener = 0);
			Point(const Point& rhs);
			virtual ~Point() /*throw()*/;
			// operators
			operator Position() /*throw()*/;
			operator const Position() const /*throw()*/;
			// core attributes
			Document& document();
			const Document& document() const;
			bool isDocumentDisposed() const /*throw()*/;
			Position normalized() const;
			const Position& position() const /*throw()*/;
			// behaviors
			bool adaptsToDocument() const /*throw()*/;
			Point& adaptToDocument(bool adapt) /*throw()*/;
			Direction gravity() const /*throw()*/;
			Point& setGravity(Direction gravity) /*throw()*/;
			// listeners
			void addLifeCycleListener(PointLifeCycleListener& listener);
			void removeLifeCycleListener(PointLifeCycleListener& listener);
			// short-circuits
			Index column() const /*throw()*/;
			ContentType contentType() const;
			Index line() const /*throw()*/;
			// operations
			void moveTo(const Position& to);
			void moveTo(Index line, Index column);

		protected:
			Point& operator=(const Position& other) /*throw()*/;
			virtual void aboutToMove(Position& to);
			void documentDisposed() /*throw()*/;
			virtual void moved(const Position& from) /*throw()*/;
			void normalize() const;
			virtual void update(const DocumentChange& change);
		private:
			Document* document_;
			Position position_;
			bool adapting_;
			Direction gravity_;
			PointListener* listener_;
			detail::Listeners<PointLifeCycleListener> lifeCycleListeners_;
			friend class Document;
		};

		bool operator==(const Point& lhs, const Point& rhs) /*throw()*/;
		bool operator!=(const Point& lhs, const Point& rhs) /*throw()*/;
		bool operator<(const Point& lhs, const Point& rhs) /*throw()*/;
		bool operator<=(const Point& lhs, const Point& rhs) /*throw()*/;
		bool operator>(const Point& lhs, const Point& rhs) /*throw()*/;
		bool operator>=(const Point& lhs, const Point& rhs) /*throw()*/;

		// documentation is point.cpp
		namespace locations {
			/// Character unit defines what is one character.
			enum CharacterUnit {
				UTF16_CODE_UNIT,	///< UTF-16 code unit.
				UTF32_CODE_UNIT,	///< UTF-32 code unit. A surrogate pair is treated as one character.
				GRAPHEME_CLUSTER,	///< A grapheme cluster is a character.
				GLYPH_CLUSTER		///< A glyph is a character (not implemented).
			};

			Position backwardBookmark(const Point& p, Index marks = 1);
			Position backwardCharacter(const Point& p, CharacterUnit unit, Index characters = 1);
			Position backwardLine(const Point& p, Index lines = 1);
			Position backwardWord(const Point& p, Index words = 1);
			Position backwardWordEnd(const Point& p, Index words = 1);
			Position beginningOfDocument(const Point& p);
			Position beginningOfLine(const Point& p);
			CodePoint characterAt(const Point& p, bool useLineFeed = false);
			Position endOfDocument(const Point& p);
			Position endOfLine(const Point& p);
			Position forwardBookmark(const Point& p, Index marks = 1);
			Position forwardCharacter(const Point& p, CharacterUnit unit, Index characters = 1);
			Position forwardLine(const Point& p, Index lines = 1);
			Position forwardWord(const Point& p, Index words = 1);
			Position forwardWordEnd(const Point& p, Index words = 1);
			bool isBeginningOfDocument(const Point& p);
			bool isBeginningOfLine(const Point& p);
			bool isEndOfDocument(const Point& p);
			bool isEndOfLine(const Point& p);
			Position nextCharacter(const Document& document, const Position& p,
				Direction direction, CharacterUnit characterUnit, Index offset = 1);
		} // namespace locations


		// inline implementations /////////////////////////////////////////////////////////////////

		/// Equality operator for @c Point objects.
		inline bool operator==(const Point& lhs, const Point& rhs) /*throw()*/ {return lhs.position() == rhs.position();}
		/// Unequality operator for @c Point objects.
		inline bool operator!=(const Point& lhs, const Point& rhs) /*throw()*/ {return !(lhs == rhs);}
		/// Less-than operator for @c Point objects.
		inline bool operator<(const Point& lhs, const Point& rhs) /*throw()*/ {return lhs.position() < rhs.position();}
		/// Less-than-or-equal-to operator for @c Point objects.
		inline bool operator<=(const Point& lhs, const Point& rhs) /*throw()*/ {return lhs < rhs || lhs == rhs;}
		/// Greater-than operator for @c Point objects.
		inline bool operator>(const Point& lhs, const Point& rhs) /*throw()*/ {return !(lhs >= rhs);}
		/// Greater-than-or-equal-to operator for @c Point objects.
		inline bool operator>=(const Point& lhs, const Point& rhs) /*throw()*/ {return !(lhs > rhs);}

		/// Conversion operator for convenience.
		inline Point::operator Position() /*throw()*/ {return position_;}
		/// Conversion operator for convenience.
		inline Point::operator const Position() const /*throw()*/ {return position_;}
		/**
		 * Protected assignment operator moves the point to @a other.
		 * @see #moveTo
		 */
		inline Point& Point::operator=(const Position& other) /*throw()*/ {position_ = other; return *this;}
		/// Returns @c true if the point is adapting to the document change.
		inline bool Point::adaptsToDocument() const /*throw()*/ {return adapting_;}
		/// Adapts the point to the document change.
		inline Point& Point::adaptToDocument(bool adapt) /*throw()*/ {adapting_ = adapt; return *this;}
		/// Returns the column number.
		inline Index Point::column() const /*throw()*/ {return position_.column;}
		/// Returns the content type of the document partition contains the point.
		inline ContentType Point::contentType() const {return document().partitioner().contentType(*this);}
		/// Returns the document or throw @c DocumentDisposedException if the document is already disposed.
		inline Document& Point::document() {if(document_ == 0) throw DocumentDisposedException(); return *document_;}
		/// Returns the document or throw @c DocumentDisposedException if the document is already disposed.
		inline const Document& Point::document() const {if(document_ == 0) throw DocumentDisposedException(); return *document_;}
		/// Called when the document is disposed.
		inline void Point::documentDisposed() /*throw()*/ {document_ = 0;}
		/// Returns the gravity.
		inline Direction Point::gravity() const /*throw()*/ {return gravity_;}
		/// Returns @c true if the document is already disposed.
		inline bool Point::isDocumentDisposed() const /*throw()*/ {return document_ == 0;}
		/// Returns the line number.
		inline Index Point::line() const /*throw()*/ {return position_.line;}
		/// Moves to the specified position.
		inline void Point::moveTo(Index line, Index column) {moveTo(Position(line, column));}
		/**
		 * Normalizes the position of the point.
		 * This method does <strong>not</strong> inform to the listeners about any movement.
		 */
		inline void Point::normalize() const {const_cast<Point*>(this)->position_ = normalized();}
		/// Returns the normalized position of the point.
		inline Position Point::normalized() const {return positions::shrinkToDocumentRegion(document(), position());}
		/// Returns the position.
		inline const Position& Point::position() const /*throw()*/ {return position_;}

	} // namespace kernel
} // namespace ascension

#endif // !ASCENSION_POINT_HPP
