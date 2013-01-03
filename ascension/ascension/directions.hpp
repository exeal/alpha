/**
 * @file directions.hpp
 * Defines abstract and physical directional terms.
 * @date 2012-03-31 created
 * @see geometry.hpp, writing-mode.hpp
 */

#ifndef ASCENSION_DIRECTIONS_HPP
#define ASCENSION_DIRECTIONS_HPP
#include <ascension/corelib/basic-exceptions.hpp>	// UnknownValueException
#include <ascension/corelib/range.hpp>
#include <ascension/graphics/geometry.hpp>
#include <array>
#include <iterator>		// std.end
#include <type_traits>	// std.extent
#include <boost/operators.hpp>
#include <boost/parameter.hpp>

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

		BOOST_PARAMETER_NAME(x)
		BOOST_PARAMETER_NAME(y)

		/// Base type of @c PhysicalTwoAxes class template.
		template<typename T>
		class PhysicalTwoAxesBase : public std::array<T, 2> {
		public:
			/// Default constructor initializes nothing.
			PhysicalTwoAxesBase() {}
			/// Constructor takes named parameters as initial values.
			template<typename Arguments>
			PhysicalTwoAxesBase(const Arguments& arguments) {
				x() = arguments[_x | value_type()];
				y() = arguments[_y | value_type()];
			}
			/// Returns a reference 'x' (horizontal position) value.
			value_type& x() BOOST_NOEXCEPT {return std::get<0>(*this);}
			/// Returns a reference 'x' (horizontal position) value.
			const value_type& x() const BOOST_NOEXCEPT {return std::get<0>(*this);}
			/// Returns a reference 'y' (vertical position) value.
			value_type& y() BOOST_NOEXCEPT {return std::get<1>(*this);}
			/// Returns a reference 'y' (vertical position) value.
			const value_type& y() BOOST_NOEXCEPT const {return std::get<1>(*this);}
		};

		/**
		 * A correction of all physical dimensions. This is a cartesian point.
		 * @tparam T The coordinate type
		 * @see presentation#AbstractTwoAxes
		 */
		template<typename T>
		class PhysicalTwoAxes : public PhysicalTwoAxesBase<T> {
		public:
			/// Default constructor initializes nothing.
			PhysicalTwoAxes() {}
			/// Constructor takes a physical point.
			template<typename Point>
			PhysicalTwoAxes(const Point& point) :
				PhysicalTwoAxesBase<T>((_x = geometry::x(point), _y = geometry::y(point))) {}
			/// Constructor takes named parameters as initial values (default value is zero).
			BOOST_PARAMETER_CONSTRUCTOR(
				PhysicalTwoAxes, (PhysicalTwoAxesBase<T>), tag,
				(required
					(x, (value_type))
					(y, (value_type))))
		};

		BOOST_PARAMETER_NAME(top)
		BOOST_PARAMETER_NAME(right)
		BOOST_PARAMETER_NAME(bottom)
		BOOST_PARAMETER_NAME(left)

		/// Base type of @c PhysicalFourSides class template.
		template<typename T>
		struct PhysicalFourSidesBase : public std::array<T, 4> {
			/// Default constructor initializes nothing.
			PhysicalFourSidesBase() {}
			/// Constructor takes named parameters as initial values (default value is zero).
			template<typename Arguments>
			PhysicalFourSidesBase(const Arguments& arguments) {
				top() = arguments[_top | value_type()];
				right() = arguments[_right | value_type()];
				bottom() = arguments[_bottom | value_type()];
				left() = arguments[_left | value_type()];
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
			reference top() BOOST_NOEXCEPT {return std::get<TOP>(*this);}
			/// Returns a reference to 'top' value.
			const_reference top() const BOOST_NOEXCEPT {return std::get<TOP>(*this);}
			/// Returns a reference to 'right' value.
			reference right() BOOST_NOEXCEPT {return std::get<RIGHT>(*this);}
			/// Returns a reference to 'right' value.
			const_reference right() const BOOST_NOEXCEPT {return std::get<RIGHT>(*this);}
			/// Returns a reference to 'bottom' value.
			reference bottom() BOOST_NOEXCEPT {return std::get<BOTTOM>(*this);}
			/// Returns a reference to 'bottom' value.
			const_reference bottom() const BOOST_NOEXCEPT {return std::get<BOTTOM>(*this);}
			/// Returns a reference to 'left' value.
			reference left() BOOST_NOEXCEPT {return std::get<LEFT>(*this);}
			/// Returns a reference to 'left' value.
			const_reference left() const BOOST_NOEXCEPT {return std::get<LEFT>(*this);}
		};

		/**
		 * A correction of all physical directions.
		 * @tparam T Element type
		 * @see presentation#FlowRelativeFourSides
		 */
		template<typename T>
		struct PhysicalFourSides : public PhysicalFourSidesBase<T> {
			/// Default constructor initializes nothing.
			PhysicalFourSides() {}
			/// Constructor takes a physical rectangle.
			template<typename Rectangle>
			PhysicalFourSides(const Rectangle& rectangle) {
				top() = geometry::top(rectangle);
				right() = geometry::right(rectangle);
				bottom() = geometry::bottom(rectangle);
				left() = geometry::left(rectangle);
			}
			/// Constructor takes named parameters as initial values (default value is zero).
			BOOST_PARAMETER_CONSTRUCTOR(
				PhysicalFourSides, (PhysicalFourSidesBase<T>), tag,
				(required
					(top, (value_type))
					(right, (value_type))
					(bottom, (value_type))
					(left, (value_type))))
		};

		/**
		 * Returns a range in horizontal direction of the given physical four sides.
		 * @tparam T Element type
		 * @param sides The physical four sides
		 * @return A range
		 * @see verticalRange, blockFlowRange, inlineFlowRange
		 */
		template<typename T>
		inline Range<T> horizontalRange(const PhysicalFourSides<T>& sides) {
			return makeRange(sides.left(), sides.right());
		}

		/**
		 * Returns a range in vertical direction of the given physical four sides.
		 * @tparam T Element type
		 * @param sides The physical four sides
		 * @return A range
		 * @see horizontalRange, blockFlowRange, inlineFlowRange
		 */
		template<typename T>
		inline Range<T> verticalRange(const PhysicalFourSides<T>& sides) {
			return makeRange(sides.top(), sides.bottom());
		}
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

		BOOST_PARAMETER_NAME(bpd)
		BOOST_PARAMETER_NAME(ipd)

		/// Base type of @c AbstractTwoAxes class template.
		template<typename T>
		class AbstractTwoAxesBase : public std::array<T, 2> {
		public:
			/// Default constructor initializes nothing.
			AbstractTwoAxesBase() {}
			/// Constructor takes named parameters as initial values
			template<typename Arguments>
			AbstractTwoAxesBase(const Arguments& arguments) {
				bpd() = arguments[_bpd | value_type()];
				ipd() = arguments[_ipd | value_type()];
			}
			/// Returns a reference to 'block-dimension' value.
			value_type& bpd() BOOST_NOEXCEPT {return std::get<0>(*this);}
			/// Returns a reference to 'block-dimension' value.
			const value_type& bpd() const BOOST_NOEXCEPT {return std::get<0>(*this);}
			/// Returns a reference to 'inline-dimension' value.
			value_type& ipd() BOOST_NOEXCEPT {return std::get<1>(*this);}
			/// Returns a reference to 'inline-dimension' value.
			const value_type& ipd() const BOOST_NOEXCEPT {return std::get<1>(*this);}
		};

		/**
		 * A correction of all abstract dimensions.
		 * @tparam T The coordinate type
		 * @see graphics#PhysicalTwoAxes
		 */
		template<typename T>
		class AbstractTwoAxes : public AbstractTwoAxesBase<T> {
		public:
			/// Default constructor initializes nothing.
			AbstractTwoAxes() {}
			/// Constructor takes named parameters (default value is zero).
			BOOST_PARAMETER_CONSTRUCTOR(
				AbstractTwoAxes, (AbstractTwoAxesBase<T>), tag,
				(required
					(bpd, (value_type))
					(ipd, (value_type))))
		};

		BOOST_PARAMETER_NAME(before)
		BOOST_PARAMETER_NAME(after)
		BOOST_PARAMETER_NAME(start)
		BOOST_PARAMETER_NAME(end)

		/// Base type of @c FlowRelativeFourSides class template.
		template<typename T>
		class FlowRelativeFourSidesBase : public std::array<T, 4> {
		public:
			/// Default constructor initializes nothing.
			FlowRelativeFourSidesBase() {}
//			FlowRelativeFourSidesBase(FlowRelativeFourSidesBase&&) {}
//			FlowRelativeFourSidesBase(const FlowRelativeFourSidesBase&) {}
			/// Constructor takes named parameters as initial values.
			template<typename Arguments>
			FlowRelativeFourSidesBase(const Arguments& arguments) {
				before() = arguments[_before | value_type()];
				after() = arguments[_after | value_type()];
				start() = arguments[_start | value_type()];
				end() = arguments[_end | value_type()];
			}
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
			reference before() BOOST_NOEXCEPT {return std::get<BEFORE>(*this);}
			/// Returns a reference to 'before' value.
			const_reference before() const BOOST_NOEXCEPT {return std::get<BEFORE>(*this);}
			/// Returns a reference to 'after' value.
			reference after() BOOST_NOEXCEPT {return std::get<AFTER>(*this);}
			/// Returns a reference to 'after' value.
			const_reference after() const BOOST_NOEXCEPT {return std::get<AFTER>(*this);}
			/// Returns a reference to 'start' value.
			reference start() BOOST_NOEXCEPT {return std::get<START>(*this);}
			/// Returns a reference to 'start' value.
			const_reference start() const BOOST_NOEXCEPT {return std::get<START>(*this);}
			/// Returns a reference to 'end' value.
			/// @note This method hides @c std#array#end.
			reference end() BOOST_NOEXCEPT {return std::get<END>(*this);}
			/// Returns a reference to 'end' value.
			/// @note This method hides @c std#array#end.
			const_reference end() const BOOST_NOEXCEPT {return std::get<END>(*this);}
		};

		/**
		 * A correction of all flow-relative directions.
		 * @tparam T The element type
		 * @see graphics#PhysicalFourSides
		 */
		template<typename T>
		class FlowRelativeFourSides : public FlowRelativeFourSidesBase<T> {
		public:
			/// Default constructor initializes nothing.
			FlowRelativeFourSides() {}
			FlowRelativeFourSides(const FlowRelativeFourSides&);
			FlowRelativeFourSides(FlowRelativeFourSides&&);
			/// Constructor takes named parameters as initial values.
			BOOST_PARAMETER_CONSTRUCTOR(
				FlowRelativeFourSides, (FlowRelativeFourSidesBase<T>), tag,
				(required
					(before, (value_type))
					(after, (value_type))
					(start, (value_type))
					(end, (value_type))))
		};

		/**
		 * Returns a range in block flow direction of the given abstract four sides.
		 * @tparam T Element type
		 * @param sides The abstract four sides
		 * @return A range
		 * @see inlineFlowRange, horizontalRange, verticalRange
		 */
		template<typename T>
		inline Range<T> blockFlowRange(const FlowRelativeFourSides<T>& sides) {
			return makeRange(sides.before(), sides.after());
		}

		/**
		 * Returns a range in inline flow direction of the given abstract four sides.
		 * @tparam T Element type
		 * @param sides The abstract four sides
		 * @return A range
		 * @see blockFlowRange, horizontalRange, verticalRange
		 */
		template<typename T>
		inline Range<T> inlineFlowRange(const FlowRelativeFourSides<T>& sides) {
			return makeRange(sides.start(), sides.end());
		}
	}

	/// @name Free Functions to Convert Between Geometries and Abstract/Flow-Relative Instances
	/// @{

	namespace graphics {
		namespace geometry {
			/**
			 * Converts a @c PhysicalTwoAxes into a point.
			 * @tparam Point Type of return value
			 * @tparam Coordinate Type of coordinate of @a axes
			 * @param axes A @c PhysicalTwoAxes object
			 * @return A converted point
			 */
			template<typename Point, typename Coordinate>
			inline Point make(const PhysicalTwoAxes<Coordinate>& axes,
					typename detail::EnableIfTagIs<Point, PointTag>::type* = nullptr) {
				return make<Point>(axes.x(), axes.y());
			}
			/**
			 * Converts a @c PhysicalFourSides into a rectangle.
			 * @tparam Rectangle Type of return value
			 * @tparam Coordinate Type of coordinate of @a sides
			 * @param sides A @c PhysicalFourSides object
			 * @return A converted rectangle
			 */
			template<typename Rectangle, typename Coordinate>
			inline Rectangle make(const PhysicalFourSides<Coordinate>& sides,
					typename detail::EnableIfTagIs<Rectangle, RectangleTag>::type* = nullptr) {
				return make<Rectangle>(horizontalRange(sides), verticalRange(sides));
			}
		}
	}

	/// @}
}

// specialize std.end for presentation.FlowRelativeFourSides.end duplication
namespace std {
	template<typename T>
	inline typename ascension::presentation::FlowRelativeFourSides<T>::iterator end(ascension::presentation::FlowRelativeFourSides<T>& v) {
		return std::end(static_cast<std::array<T, 4>&>(v));
	}
	template<typename T>
	inline typename ascension::presentation::FlowRelativeFourSides<T>::const_iterator end(const ascension::presentation::FlowRelativeFourSides<T>& v) {
		return std::end(static_cast<const std::array<T, 4>&>(v));
	}
}

#endif // !ASCENSION_DIRECTIONS_HPP
