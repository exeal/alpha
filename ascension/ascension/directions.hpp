/**
 * @file directions.hpp
 * Defines abstract and physical directional terms.
 * @date 2012-03-31 created
 * @date 2012-2014
 * @see flow-relative-directions-dimensions.hpp, line-relative-directions-dimensions.hpp,
 *      physical-directions-dimensions.hpp, writing-mode.hpp
 */

#ifndef ASCENSION_DIRECTIONS_HPP
#define ASCENSION_DIRECTIONS_HPP
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

#endif // !ASCENSION_DIRECTIONS_HPP
