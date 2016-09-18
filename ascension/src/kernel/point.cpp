/**
 * @file point.cpp
 * @author exeal
 * @date 2003-2014, 2016
 */

#include <ascension/corelib/numeric-range-algorithm/encompasses.hpp>
#include <ascension/corelib/text/break-iterator.hpp>
#include <ascension/corelib/text/character-property.hpp>	// text.ucd.BinaryProperty
#include <ascension/kernel/document.hpp>
#include <ascension/kernel/document-character-iterator.hpp>
#include <ascension/kernel/point.hpp>
#include <boost/core/ignore_unused.hpp>


namespace ascension {
	namespace kernel {
		// DocumentDisposedException //////////////////////////////////////////////////////////////////////////////////

		/// Default constructor.
		DocumentDisposedException::DocumentDisposedException() :
				IllegalStateException("The document the object connecting to has been already disposed.") {
		}


		// Point //////////////////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * @class ascension::kernel::Point
		 *
		 * A point represents a document position and adapts to the document change.
		 *
		 * When the document change occurred, @c Point moves automatically as follows:
		 *
		 * - If text was inserted or deleted before the point, the point will move accordingly.
		 * - If text was inserted or deleted after the point, the point will not move.
		 * - If region includes the point was deleted, the point will move to the beginning (= end) of the region.
		 * - If text was inserted at the point, the point will or will not move according to the gravity.
		 *
		 * When the document was reset (by @c Document#resetContent), the all points move to the beginning of the
		 * document.
		 *
		 * Almost all methods of this or derived classes will throw @c DocumentDisposedException if the document is
		 * already disposed. Call @c #isDocumentDisposed to check if the document is exist or not.
		 *
		 * @c Point is unaffected by narrowing and can moves outside of the accessible region.
		 *
		 * @see Position, Document, locations, viewer#VisualPoint, viewer#Caret
		 */

		/**
		 * @typedef ascension::kernel::Point::DestructionSignal
		 * The signal which gets emitted when the point was destructed.
		 * @param point A pointer to the destructed point. Don't access the point by this pointer
		 * @see Point#destructionSignal, MotionSignal
		 */

		/**
		 * @typedef ascension::kernel::Point::MotionSignal
		 * The signal which gets emitted when the point was moved.
		 * @param self The point
		 * @param oldPosition The position from which the point moved
		 * @see Point#motionSignal, DestructionSignal
		 */

		/**
		 * Constructor.
		 * @param document The document to which the point attaches
		 * @param position The initial position of the point
		 * @throw BadPositionException @a position is outside of the document
		 * @post position() == position
		 */
		Point::Point(Document& document, const Position& position /* = kernel::Position::zero() */) : AbstractPoint(document), position_(position) {
			if(!encompasses(document.region(), position))
				throw BadPositionException(position);
		}

		/**
		 * Copy-constructor.
		 * @param other The source object
		 * @throw DocumentDisposedException The document to which @a other belongs had been disposed
		 * @post position() == other.position()
		 */
		Point::Point(const Point& other) : AbstractPoint(other), position_(other.position_) {
		}

		/// Destructor does nothing.
		Point::~Point() BOOST_NOEXCEPT {
		}

		/**
		 * This overridable method is called by @c #moveTo to check and adjust the desitination position.
		 * If you override this, consider the followings:
		 *
		 * - To change the destination, modify the value of @a to.
		 * - Call @c #aboutToMove method of the super class with the same parameter.
		 * - Throw any exceptions to interrupt the movement.
		 *
		 * @c Point#aboutToMove does nothing.
		 * @param to The destination position. Implementation can modify this value
		 * @throw DocumentDisposedException The document to which the point belongs is already disposed
		 * @see #moved, #moveTo, viewer#VisualPoint#aboutToMove
		 */
		void Point::aboutToMove(Position& to) {
			boost::ignore_unused(to);
		}

		/// @see AbstractPoint#contentReset
		void Point::contentReset() {
			assert(!isDocumentDisposed());
			assert(adaptsToDocument());
			moveTo(Position::zero());
		}

		/// @see AbstractPoint#documentChanged
		void Point::documentChanged(const DocumentChange& change) {
			assert(!isDocumentDisposed());
			assert(adaptsToDocument());
//			normalize();
			const Position newPosition(positions::updatePosition(position(), change, gravity()));
			if(newPosition != position())
				moveTo(newPosition);	// TODO: this may throw...
		}

		/**
		 * This overridable method is called by @c #moveTo to notify the motion was finished.
		 * If you override this, call @c #moved method of the super class with the same parameter. And don't throw any
		 * exceptions. Note that this method is not called if @c #aboutToMove threw an exception.
		 * @param from The position before the point moved. This value may equal to the current position
		 * @see #aboutToMove, #moveTo, viewer#VisualPoint#moved
		 */
		void Point::moved(const Position& from) BOOST_NOEXCEPT {
			boost::ignore_unused(from);
		}

		/**
		 * Moves to the specified position.
		 * While this method fails when @a to was outside of the document, whether it depends on the derived class when
		 * @a to was outside of the accessible region. @c Point succeeds in the latter case. For other classes, see the
		 * documentations of the classes.
		 * @param to The destination position
		 * @throw BadPositionException @a to is outside of the document
		 * @return This point
		 * @see viewer#VisualPoint#moveTo
		 */
		Point& Point::moveTo(const Position& to) {
			if(isDocumentDisposed())
				throw DocumentDisposedException();
			else if(positions::isOutsideOfDocumentRegion(document(), to))
				throw BadPositionException(to);
			Position destination(to);
			aboutToMove(destination);
			destination = positions::shrinkToDocumentRegion(document(), destination);
			const Position from(position());
			position_ = destination;
			moved(from);
			if(destination != from)
				motionSignal_(*this, from);
			return *this;
		}


		// free functions /////////////////////////////////////////////////////////////////////////////////////////////
#if 0
		/**
		 * Moves to the specified offset.
		 * @param offset The offset from the start of the document
		 * @deprecated 0.8
		 */
		void EditPoint::moveToAbsoluteCharacterOffset(Index offset) {
			verifyDocument();
		
			Index readCount = 0;
			const Region region(document()->region());
		
			if(document()->lineLength(line(region.first)) + 1 - offsetInLine(region.first) >= offset) {
				moveTo(Position(line(region.first), offsetInLine(region.first) + offset));
				return;
			}
			readCount += document()->lineLength(line(region.first)) + 1 - offsetInLine(region.first);
			for(Index i = line(region.first) + 1; i <= line(region.second); ++i) {
				const Index lineLength = document()->lineLength(i) + 1;	// +1 is for a newline
				if(readCount + lineLength >= offset) {
					moveTo(Position(i, readCount + lineLength - offset));
					return;
				}
				readCount += lineLength;
			}
			moveTo(Position(line(region.second), document()->lineLength(line(region.second))));
		}
#endif
	}
}
