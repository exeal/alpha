/**
 * @file directions.hpp
 * Defines abstract and physical directional terms.
 * @date 2012-03-31 created
 * @see geometry.hpp, writing-mode.hpp
 */

#ifndef ASCENSION_DIRECTIONS_HPP
#define ASCENSION_DIRECTIONS_HPP
#include <ascension/corelib/basic-exceptions.hpp>	// UnknownValueException
#include <array>
#include <type_traits>	// std.extent
#include <boost/operators.hpp>

namespace ascension {
	/**
	 * Represents direction in a text or a document (not visual orientation. See
	 * @c #presentation#ReadingDirection).
	 * @see ascension#text, ascension#searcher
	 */
	class Direction : private boost::equality_comparable<Direction> {
	public:
		static const Direction FORWARD;		///< Direction to the end.
		static const Direction BACKWARD;	///< Direction to the start.
		/// Copy-constructor.
		Direction(const Direction& other) /*noexcept*/ : value_(other.value_) {}
		/// Assignment operator.
		Direction& operator=(const Direction& other) /*noexcept*/ {
			return (value_ = other.value_), *this;
		}
		/// Negation operator returns the complement of this.
		Direction operator!() const /*noexcept*/ {
			return (*this == FORWARD) ? BACKWARD : FORWARD;
		}
		/// Equality operator.
		bool operator==(const Direction& other) const /*noexcept*/ {
			return value_ == other.value_;
		}
	private:
		explicit Direction(bool value) /*noexcept*/ : value_(value) {}
		bool value_;
	};

	namespace graphics {
		/**
		 * Defines physical directions.
		 * @see presentation#FlowRelativeDirection
		 */
		enum PhysicalDirection {
			TOP,	///< Physical top.
			RIGHT,	///< Physical right.
			BOTTOM,	///< Physical bottom.
			LEFT	///< Physical left.
		};

		/**
		 * Returns direction opposite @a direction.
		 * @throw UnknownValueException @a direction is invalid
		 */
		inline PhysicalDirection operator!(PhysicalDirection direction) {
			static const PhysicalDirection opposites[4] = {BOTTOM, LEFT, TOP, RIGHT};
			if(direction >= std::extent<decltype(opposites)>::value)
				throw UnknownValueException("direction");
			return opposites[direction];
		}

		/**
		 * A correction of all physical dimensions. This is a cartesian point.
		 * @see presentation#AbstractTwoAxes
		 */
		template<typename T>
		class PhysicalTwoAxes : private std::pair<T, T> {
		public:
			/// Type of directional value.
			typedef T value_type;
			PhysicalTwoAxes(const value_type& x, const value_type& y) : std::pair<T, T>(x, y) {}
			/// Returns a reference 'x' (horizontal position) value.
			value_type& x() /*noexcept*/ {return first;}
			/// Returns a reference 'x' (horizontal position) value.
			const value_type& x() const /*noexcept*/ {return first;}
			/// Returns a reference 'y' (vertical position) value.
			value_type& y() /*noexcept*/ {return second;}
			/// Returns a reference 'y' (vertical position) value.
			const value_type& y() /*noexcept*/ const {return second;}
		};

		/**
		 * A correction of all physical directions.
		 * @see presentation#FlowRelativeFourSides
		 */
		template<typename T>
		struct PhysicalFourSides : public std::array<T, 4> {
			/// Default constructor.
			PhysicalFourSides() {}
			template<typename Rectangle>
			PhysicalFourSides(const Rectangle& rectangle) {
				top() = geometry::top(rectangle);
				right() = geometry::right(rectangle);
				bottom() = geometry::bottom(rectangle);
				left() = geometry::left(rectangle);
			}
			/// Returns a reference to value of @a direction.
			reference operator[](PhysicalDirection direction) {
				return (*this)[static_cast<size_type>(direction)];
			}
			/// Returns a reference to value of @a direction.
			const_reference operator[](PhysicalDirection direction) const {
				return (*this)[static_cast<size_type>(direction)];
			}
			using std::array<T, 4>::operator[];
			/// Returns a reference to 'top' value.
			reference top() {return (*this)[TOP];}
			/// Returns a reference to 'top' value.
			const_reference top() const {return (*this)[TOP];}
			/// Returns a reference to 'right' value.
			reference right() {return (*this)[RIGHT];}
			/// Returns a reference to 'right' value.
			const_reference right() const {return (*this)[RIGHT];}
			/// Returns a reference to 'bottom' value.
			reference bottom() {return (*this)[BOTTOM];}
			/// Returns a reference to 'bottom' value.
			const_reference bottom() const {return (*this)[BOTTOM];}
			/// Returns a reference to 'left' value.
			reference left() {return (*this)[LEFT];}
			/// Returns a reference to 'left' value.
			const_reference left() const {return (*this)[LEFT];}
		};
	}

	namespace presentation {
		/**
		 * Defines flow-relative directions.
		 * @see graphics#PhysicalDirection
		 */
		enum FlowRelativeDirection {
			/// 'before' -- Nominally the side that comes earlier in the block progression.
			BEFORE,
			/// 'after' -- The side opposite 'before'.
			AFTER,
			/// 'start' -- Nominally the side from which text of its inline base direction will start.
			START,
			/// 'end' -- The side opposite 'start'.
			END
		};

		/**
		 * Returns direction opposite @a direction.
		 * @throw UnknownValueException @a direction is invalid
		 */
		inline FlowRelativeDirection operator!(FlowRelativeDirection direction) {
			static const FlowRelativeDirection opposites[4] = {AFTER, BEFORE, END, START};
			if(direction >= std::extent<decltype(opposites)>::value)
				throw UnknownValueException("direction");
			return opposites[direction];
		}

		/**
		 * A correction of all abstract dimensions.
		 * @see graphics#PhysicalTwoAxes
		 */
		template<typename T>
		class AbstractTwoAxes : private std::pair<T, T> {
		public:
			typedef T value_type;
			/// Returns a reference to 'block-dimension' value.
			value_type& bpd() /*noexcept*/ {return first;}
			/// Returns a reference to 'block-dimension' value.
			const value_type& bpd() const /*noexcept*/ {return first;}
			/// Returns a reference to 'inline-dimension' value.
			value_type& ipd() /*noexcept*/ {return second;}
			/// Returns a reference to 'inline-dimension' value.
			const value_type& ipd() const /*noexcept*/ {return second;}
		};

		/**
		 * A correction of all flow-relative directions.
		 * @see graphics#PhysicalFourSides
		 */
		template<typename T>
		class FlowRelativeFourSides : public std::array<T, 4> {
		public:
			/// Returns a reference to value of @a direction.
			reference operator[](FlowRelativeDirection direction) {
				return (*this)[static_cast<size_type>(direction)];
			}
			/// Returns a reference to value of @a direction.
			const_reference operator[](FlowRelativeDirection direction) const {
				return (*this)[static_cast<size_type>(direction)];
			}
			using std::array<T, 4>::operator[];
			/// Returns a reference to 'before' value.
			reference before() {return (*this)[BEFORE];}
			/// Returns a reference to 'before' value.
			const_reference before() const {return (*this)[BEFORE];}
			/// Returns a reference to 'after' value.
			reference after() {return (*this)[AFTER];}
			/// Returns a reference to 'after' value.
			const_reference after() const {return (*this)[AFTER];}
			/// Returns a reference to 'start' value.
			reference start() {return (*this)[START];}
			/// Returns a reference to 'start' value.
			const_reference start() const {return (*this)[START];}
			/// Returns a reference to 'end' value.
			/// @note This method hides @c std#array#end.
			reference end() {return (*this)[END];}
			/// Returns a reference to 'end' value.
			/// @note This method hides @c std#array#end.
			const_reference end() const {return (*this)[END];}
		};
	}
}

#endif // ASCENSION_DIRECTIONS_HPP
