/**
 * @file direction.hpp
 * Defines abstract and physical directional terms.
 * @date 2012-03-31 created
 * @date 2012-2014 was directions.hpp
 * @date 2015-01-09 Renamed from directions.hpp
 * @see flow-relative-direction.hpp, line-relative-direction.hpp, physical-direction.hpp, writing-mode.hpp
 */

#ifndef ASCENSION_DIRECTION_HPP
#define ASCENSION_DIRECTION_HPP
#include <boost/operators.hpp>

namespace ascension {
	/**
	 * Represents direction in a text or a document (not visual orientation. See @c #presentation#ReadingDirection).
	 * @see ascension#text, ascension#searcher
	 */
	class Direction : private boost::equality_comparable<Direction> {
	public:
		static const Direction FORWARD;		///< Direction to the end.
		static const Direction BACKWARD;	///< Direction to the start.
		/// Copy-constructor.
		Direction(const Direction& other) BOOST_NOEXCEPT : value_(other.value_) {}
		/// Assignment operator.
		Direction& operator=(const Direction& other) BOOST_NOEXCEPT {
			return (value_ = other.value_), *this;
		}
		/// Negation operator returns the complement of this.
		Direction operator!() const BOOST_NOEXCEPT {
			return (*this == FORWARD) ? BACKWARD : FORWARD;
		}
		/// Equality operator.
		bool operator==(const Direction& other) const BOOST_NOEXCEPT {
			return value_ == other.value_;
		}
	private:
		explicit Direction(bool value) BOOST_NOEXCEPT : value_(value) {}
		bool value_;
	};
}

#endif // !ASCENSION_DIRECTION_HPP
