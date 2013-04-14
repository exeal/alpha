/**
 * @file point.hpp
 * @author exeal
 * @date 2003-2013
 */

#ifndef ASCENSION_POINT_HPP
#define ASCENSION_POINT_HPP
#include <ascension/kernel/document.hpp>
#include <boost/operators.hpp>
#include <boost/optional.hpp>

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
			virtual ~PointLifeCycleListener() BOOST_NOEXCEPT {}
		private:
			/// The point was destroyed. After this, don't call @c Point#addLifeCycleListener.
			virtual void pointDestroyed() = 0;
			friend class Point;
		};

		// documentation is point.cpp
		class Point : private boost::totally_ordered<Point> {
			ASCENSION_UNASSIGNABLE_TAG(Point);
		public:
			// constructors
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
			explicit Point(Document& document, PointListener* listener = nullptr);
#endif // ASCENSION_ABANDONED_AT_VERSION_08
			Point(Document& document, const Position& position, PointListener* listener = nullptr);
			Point(const Point& other);
			virtual ~Point() BOOST_NOEXCEPT;
			operator Position() const BOOST_NOEXCEPT;

			/// @name Core Attributes
			/// @{
			Document& document();
			const Document& document() const;
			bool isDocumentDisposed() const BOOST_NOEXCEPT;
			Position normalized() const;
			const Position& position() const BOOST_NOEXCEPT;
			/// @}

			/// @name Behaviors
			/// @{
			bool adaptsToDocument() const BOOST_NOEXCEPT;
			Point& adaptToDocument(bool adapt) BOOST_NOEXCEPT;
			Direction gravity() const BOOST_NOEXCEPT;
			Point& setGravity(Direction gravity) BOOST_NOEXCEPT;
			/// @}

			/// @name Listeners
			/// @{
			void addLifeCycleListener(PointLifeCycleListener& listener);
			void removeLifeCycleListener(PointLifeCycleListener& listener);
			/// @}

			/// @name Operations
			/// @{
			Point& moveTo(const Position& to);
			/// @}

		protected:
			Point& operator=(const Position& other) BOOST_NOEXCEPT;
			virtual void aboutToMove(Position& to);
			void documentDisposed() BOOST_NOEXCEPT;
			virtual void moved(const Position& from) BOOST_NOEXCEPT;
			void normalize() const;
			virtual void update(const DocumentChange& change);
		private:
			Document* document_;	// weak reference
			Position position_;
			bool adapting_;
			Direction gravity_;
			PointListener* listener_;
			detail::Listeners<PointLifeCycleListener> lifeCycleListeners_;
			friend class Document;
		};

		// documentation is point.cpp
		namespace locations {
			/// Character unit defines what is one character.
			enum CharacterUnit {
				UTF16_CODE_UNIT,	///< UTF-16 code unit.
				UTF32_CODE_UNIT,	///< UTF-32 code unit. A surrogate pair is treated as one character.
				GRAPHEME_CLUSTER,	///< A grapheme cluster is a character.
				GLYPH_CLUSTER		///< A glyph is a character (not implemented).
			};

#ifdef ASCENSION_ABANDONED_AT_VERSION_08
			boost::optional<Position> backwardBookmark(const Point& p, Index marks = 1);
			Position backwardCharacter(const Point& p, CharacterUnit unit, Index characters = 1);
			Position backwardLine(const Point& p, Index lines = 1);
			Position backwardWord(const Point& p, Index words = 1);
			Position backwardWordEnd(const Point& p, Index words = 1);
#endif // ASCENSION_ABANDONED_AT_VERSION_08
			Position beginningOfDocument(const Point& p);
			Position beginningOfLine(const Point& p);
			CodePoint characterAt(const Point& p, bool useLineFeed = false);
			Position endOfDocument(const Point& p);
			Position endOfLine(const Point& p);
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
			boost::optional<Position> forwardBookmark(const Point& p, Index marks = 1);
			Position forwardCharacter(const Point& p, CharacterUnit unit, Index characters = 1);
			Position forwardLine(const Point& p, Index lines = 1);
			Position forwardWord(const Point& p, Index words = 1);
			Position forwardWordEnd(const Point& p, Index words = 1);
#endif // ASCENSION_ABANDONED_AT_VERSION_08
			bool isBeginningOfDocument(const Point& p);
			bool isBeginningOfLine(const Point& p);
			bool isEndOfDocument(const Point& p);
			bool isEndOfLine(const Point& p);
			boost::optional<Position> nextBookmark(const Point& p, Direction direction, Index marks = 1);
			Position nextCharacter(const Document& document, const Position& p,
				Direction direction, CharacterUnit characterUnit, Index offset = 1);
			inline Position nextCharacter(const Point& p,
					Direction direction, CharacterUnit characterUnit, Index offset = 1) {
				return nextCharacter(p.document(), p, direction, characterUnit, offset);
			}
			Position nextLine(const Point& p, Direction direction, Index lines = 1);
			Position nextWord(const Point& p, Direction direction, Index words = 1);
			Position nextWordEnd(const Point& p, Direction direction, Index words = 1);
		} // namespace locations


		// non-member functions ///////////////////////////////////////////////////////////////////

		/// Equality operator for @c Point objects.
		inline bool operator==(const Point& lhs, const Point& rhs) BOOST_NOEXCEPT {
			return lhs.position() == rhs.position();
		}
		/// Less-than operator for @c Point objects.
		inline bool operator<(const Point& lhs, const Point& rhs) BOOST_NOEXCEPT {
			return lhs.position() < rhs.position();
		}
		/// Returns the content type of the document partition contains the point.
		inline ContentType contentType(const Point& p) {
			return p.document().partitioner().contentType(p);
		}
		/// Returns the line number of @a p.
		inline Index line(const Point& p) BOOST_NOEXCEPT {
			return p.position().line;
		}
		/// Returns the offset in the line of @a p.
		inline Index offsetInLine(const Point& p) BOOST_NOEXCEPT {
			return p.position().offsetInLine;
		}


		// Point method inline implementation /////////////////////////////////////////////////////

		/// Conversion operator for convenience.
		inline Point::operator Position() const {return position();}
		/**
		 * Protected assignment operator moves the point to @a other.
		 * @see #moveTo
		 */
		inline Point& Point::operator=(const Position& other) BOOST_NOEXCEPT {
			position_ = other;
			return *this;
		}
		/// Returns @c true if the point is adapting to the document change.
		inline bool Point::adaptsToDocument() const BOOST_NOEXCEPT {return adapting_;}
		/// Adapts the point to the document change.
		inline Point& Point::adaptToDocument(bool adapt) BOOST_NOEXCEPT {
			adapting_ = adapt;
			return *this;
		}
		/// Returns the document or throw @c DocumentDisposedException if the document is already disposed.
		inline Document& Point::document() {
			if(document_ == nullptr)
				throw DocumentDisposedException();
			return *document_;
		}
		/// Returns the document or throw @c DocumentDisposedException if the document is already disposed.
		inline const Document& Point::document() const {
			if(document_ == nullptr)
				throw DocumentDisposedException();
			return *document_;
		}
		/// Called when the document is disposed.
		inline void Point::documentDisposed() BOOST_NOEXCEPT {document_ = nullptr;}
		/// Returns the gravity.
		inline Direction Point::gravity() const BOOST_NOEXCEPT {return gravity_;}
		/// Returns @c true if the document is already disposed.
		inline bool Point::isDocumentDisposed() const BOOST_NOEXCEPT {return document_ == nullptr;}
		/**
		 * Normalizes the position of the point.
		 * This method does <strong>not</strong> inform to the listeners about any movement.
		 */
		inline void Point::normalize() const {const_cast<Point*>(this)->position_ = normalized();}
		/// Returns the normalized position of the point.
		inline Position Point::normalized() const {return positions::shrinkToDocumentRegion(document(), position());}
		/// Returns the position.
		inline const Position& Point::position() const BOOST_NOEXCEPT {return position_;}

	} // namespace kernel
} // namespace ascension

#endif // !ASCENSION_POINT_HPP
