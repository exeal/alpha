/**
 * @file kernel/point.hpp
 * @author exeal
 * @date 2003-2015
 */

#ifndef ASCENSION_KERNEL_POINT_HPP
#define ASCENSION_KERNEL_POINT_HPP
#include <ascension/kernel/abstract-point.hpp>
#include <ascension/kernel/partition.hpp>
#include <ascension/kernel/position.hpp>
#include <boost/operators.hpp>

namespace ascension {
	namespace kernel {
		// documentation is point.cpp
		class Point : public AbstractPoint, private boost::totally_ordered<Point> {
		public:
			// constructors
			explicit Point(Document& document, const Position& position = kernel::Position::zero());
			Point(const Point& other);
			virtual ~Point() BOOST_NOEXCEPT;
			operator std::pair<const Document&, Position>() const;

			/// @name Core Attribute
			const Position& position() const BOOST_NOEXCEPT;
			/// @}

			/// @name Motions
			/// @{
			typedef boost::signals2::signal<void(const Point&, const Position&)> MotionSignal;
			SignalConnector<MotionSignal> motionSignal() BOOST_NOEXCEPT;
			Point& moveTo(const Position& to);
			/// @}

		protected:
			Point& operator=(const Position& other) BOOST_NOEXCEPT;
			virtual void aboutToMove(Position& to);
			virtual void moved(const Position& from) BOOST_NOEXCEPT;
		private:
			// AbstractPoint
			void contentReset() override;
			void documentChanged(const DocumentChange& change) override;
		private:
			Position position_;
			MotionSignal motionSignal_;
		};


		// non-member functions ///////////////////////////////////////////////////////////////////

		/// Equality operator for @c Point objects.
		inline bool operator==(const Point& lhs, const Point& rhs) BOOST_NOEXCEPT {
			return lhs.position() == rhs.position();
		}

		/// Less-than operator for @c Point objects.
		inline bool operator<(const Point& lhs, const Point& rhs) BOOST_NOEXCEPT {
			return lhs.position() < rhs.position();
		}

		/// @overload
		inline const Position& position(const Point& p) BOOST_NOEXCEPT {
			return p.position();
		}


		// Point method inline implementation /////////////////////////////////////////////////////

		/// Conversion operator for convenience.
		inline Point::operator std::pair<const Document&, Position>() const {
			return std::make_pair(std::ref(document()), position());
		}

		/**
		 * Protected assignment operator moves the point to @a other.
		 * @see #moveTo
		 */
		inline Point& Point::operator=(const Position& other) BOOST_NOEXCEPT {
			position_ = other;
			return *this;
		}

		/// Returns the @c MotionSignal signal connector.
		inline SignalConnector<Point::MotionSignal> Point::motionSignal() BOOST_NOEXCEPT {
			return makeSignalConnector(motionSignal_);
		}

		/**
		 * Returns the position.
		 * @see viewer#VisualPoint#hit
		 */
		inline const Position& Point::position() const BOOST_NOEXCEPT {
			return position_;
		}
	} // namespace kernel
} // namespace ascension

#endif // !ASCENSION_KERNEL_POINT_HPP
