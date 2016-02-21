/**
 * @file point.hpp
 * @author exeal
 * @date 2003-2015
 */

#ifndef ASCENSION_POINT_HPP
#define ASCENSION_POINT_HPP
#include <ascension/corelib/signals.hpp>
#include <ascension/direction.hpp>
#include <ascension/kernel/partition.hpp>
#include <ascension/kernel/position.hpp>
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

		class Document;

		// documentation is point.cpp
		class Point : private boost::totally_ordered<Point> {
		public:
			// constructors
			explicit Point(Document& document, const Position& position = kernel::Position::zero());
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

			/// @name Signals
			/// @{
			typedef boost::signals2::signal<void(const Point*)> DestructionSignal;
			SignalConnector<DestructionSignal> destructionSignal() BOOST_NOEXCEPT;
			typedef boost::signals2::signal<void(const Point&, const Position&)> MotionSignal;
			SignalConnector<MotionSignal> motionSignal() BOOST_NOEXCEPT;
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
			DestructionSignal destructionSignal_;
			MotionSignal motionSignal_;
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

		ContentType contentType(const Point& p);

		/// @overload
		inline const Position& position(const Point& p) BOOST_NOEXCEPT {
			return p.position();
		}


		// Point method inline implementation /////////////////////////////////////////////////////

		/// Conversion operator for convenience.
		inline Point::operator Position() const {
			return position();
		}

		/**
		 * Protected assignment operator moves the point to @a other.
		 * @see #moveTo
		 */
		inline Point& Point::operator=(const Position& other) BOOST_NOEXCEPT {
			position_ = other;
			return *this;
		}

		/// Returns @c true if the point is adapting to the document change.
		inline bool Point::adaptsToDocument() const BOOST_NOEXCEPT {
			return adapting_;
		}

		/// Adapts the point to the document change.
		inline Point& Point::adaptToDocument(bool adapt) BOOST_NOEXCEPT {
			adapting_ = adapt;
			return *this;
		}

		/// Returns the document or throws @c DocumentDisposedException if the document is already disposed.
		inline Document& Point::document() {
			if(document_ == nullptr)
				throw DocumentDisposedException();
			return *document_;
		}

		/// Returns the document or throws @c DocumentDisposedException if the document is already disposed.
		inline const Document& Point::document() const {
			if(document_ == nullptr)
				throw DocumentDisposedException();
			return *document_;
		}

		/// Called when the document is disposed.
		inline void Point::documentDisposed() BOOST_NOEXCEPT {
			document_ = nullptr;
		}

		/// Returns the gravity.
		inline Direction Point::gravity() const BOOST_NOEXCEPT {
			return gravity_;
		}

		/// Returns @c true if the document is already disposed.
		inline bool Point::isDocumentDisposed() const BOOST_NOEXCEPT {
			return document_ == nullptr;
		}

		/**
		 * Normalizes the position of the point.
		 * This method does <strong>not</strong> inform to the listeners about any movement.
		 */
		inline void Point::normalize() const {
			const_cast<Point*>(this)->position_ = normalized();
		}

		/// Returns the position.
		inline const Position& Point::position() const BOOST_NOEXCEPT {
			return position_;
		}

		namespace detail {
			/// @internal Returns the @c IdentifierSyntax object corresponds to the given point.
			template<typename Point>
			inline const text::IdentifierSyntax& identifierSyntax(const Point& p) {
				return p.document().contentTypeInformation().getIdentifierSyntax(contentType(p));
			}
		}

	} // namespace kernel
} // namespace ascension

#endif // !ASCENSION_POINT_HPP
