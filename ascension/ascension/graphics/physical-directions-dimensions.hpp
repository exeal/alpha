/**
 * @file physical-directions-dimensions.hpp
 * Defines physical directional and dimensional terms.
 * @date 2012-03-31 created
 * @date 2012-2014 was directions.hpp
 * @date 2015-01-09 Separated from directions.hpp
 * @see direction.hpp, flow-relative-directions-dimensions.hpp, line-relative-directions-dimensions.hpp, writing-mode.hpp
 */

#ifndef ASCENSION_PHYSICAL_DIRECTIONS_DIMENSIONS_HPP
#define ASCENSION_PHYSICAL_DIRECTIONS_DIMENSIONS_HPP
#include <ascension/corelib/basic-exceptions.hpp>	// UnknownValueException
#include <ascension/corelib/detail/decay-or-refer.hpp>
#include <ascension/corelib/future/scoped-enum-emulation.hpp>
#include <ascension/graphics/geometry/algorithm.hpp>
#include <array>
#include <type_traits>
#include <boost/operators.hpp>
#include <boost/parameter.hpp>
#include <boost/range/irange.hpp>

namespace ascension {
	namespace graphics {
		/// @defgroup physical_directions_dimensions Physical Directions and Dimensions
		/// @see CSS Writing Modes Module Level 3, 6 Abstract Box Terminology
		///      (http://www.w3.org/TR/css-writing-modes-3/#abstract-box)
		/// @{
		/**
		 * Defines physical directions.
		 * @see font#LineRelativeDirection, presentation#FlowRelativeDirection
		 */
		ASCENSION_SCOPED_ENUM_DECLARE_BEGIN(PhysicalDirection)
			TOP,	///< Physical top.
			RIGHT,	///< Physical right.
			BOTTOM,	///< Physical bottom.
			LEFT	///< Physical left.
		ASCENSION_SCOPED_ENUM_DECLARE_END(PhysicalDirection)

		/**
		 * Returns direction opposite @a direction.
		 * @throw UnknownValueException @a direction is invalid
		 */
		inline PhysicalDirection operator!(PhysicalDirection direction) {
			static const PhysicalDirection opposites[4] = {
				PhysicalDirection::BOTTOM, PhysicalDirection::LEFT,
				PhysicalDirection::TOP, PhysicalDirection::RIGHT
			};
			const std::size_t index = boost::underlying_cast<std::size_t>(direction);
			if(index >= std::extent<decltype(opposites)>::value)
				throw UnknownValueException("direction");
			return opposites[index];
		}

#ifndef ASCENSION_DOXYGEN_SHOULD_SKIP_THIS
		BOOST_PARAMETER_NAME(x)
		BOOST_PARAMETER_NAME(y)
#endif // !ASCENSION_DOXYGEN_SHOULD_SKIP_THIS

		/// Base type of @c PhysicalTwoAxes class template.
		template<typename T>
		class PhysicalTwoAxesBase : public std::array<T, 2> {
		public:
			/// Default constructor initializes nothing.
			PhysicalTwoAxesBase() {}
			/// Copy-constructor.
			PhysicalTwoAxesBase(const PhysicalTwoAxesBase<T>& other) : std::array<T, 2>(other) {}
			/// Constructor takes named parameters as initial values.
			template<typename Arguments>
			PhysicalTwoAxesBase(const Arguments& arguments) {
//				x() = arguments[_x | value_type()];
//				y() = arguments[_y | value_type()];
				x() = arguments[_x.operator|(value_type())];
				y() = arguments[_y.operator|(value_type())];
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
		 * A collection of all physical dimensions. This is a cartesian point.
		 * @tparam T The coordinate type
		 * @see presentation#FlowRelativeTwoAxes
		 */
		template<typename T>
		class PhysicalTwoAxes : public PhysicalTwoAxesBase<T>, private boost::additive<PhysicalTwoAxes<T>> {
		public:
			/// Default constructor initializes nothing.
			PhysicalTwoAxes() {}
			/// Copy-constructor.
			PhysicalTwoAxes(const PhysicalTwoAxes<T>& other) : PhysicalTwoAxesBase<T>(static_cast<const PhysicalTwoAxesBase<T>&>(other)) {}
			/// Constructor takes a physical point.
			template<typename Point>
			PhysicalTwoAxes(const Point& point, typename geometry::detail::EnableIfTagIs<Point, boost::geometry::point_tag>::type* = nullptr) :
				PhysicalTwoAxesBase<T>((_x = geometry::x(point), _y = geometry::y(point))) {}
			/// Constructor takes named parameters as initial values (default value is zero).
			BOOST_PARAMETER_CONSTRUCTOR(
				PhysicalTwoAxes, (PhysicalTwoAxesBase<T>), tag,
				(required
					(x, (value_type))
					(y, (value_type))))
			/// Compound-add operator calls same operators of @c T for @c #x and @c #y.
			PhysicalTwoAxes& operator+=(const PhysicalTwoAxes<T>& other) {
				x() += other.x();
				y() += other.y();
				return *this;
			}
			/// Compound-subtract operator calls same operators of @c T for @c #x and @c #y.
			PhysicalTwoAxes& operator-=(const PhysicalTwoAxes<T>& other) {
				x() -= other.x();
				y() -= other.y();
				return *this;
			}
		};

		/**
		 * Creates a @c PhysicalTwoAxes object, deducing the target type from the types of arguments.
		 * @tparam ArgumentPack The type of @a arguments
		 * @param arguments The named arguments same as the constructor of @c PhysicalTwoAxes class
		 */
		template<typename ArgumentPack>
		inline auto makePhysicalTwoAxes(const ArgumentPack& arguments)
				-> PhysicalTwoAxes<typename ascension::detail::DecayOrRefer<decltype(arguments[_x])>::Type> {
			typedef typename ascension::detail::DecayOrRefer<decltype(arguments[_x])>::Type Coordinate;
			static_assert(std::is_same<ascension::detail::DecayOrRefer<decltype(arguments[_y])>::Type, Coordinate>::value, "");
			return PhysicalTwoAxes<Coordinate>(_x = arguments[_x], _y = arguments[_y]);
		}

#ifndef ASCENSION_DOXYGEN_SHOULD_SKIP_THIS
		BOOST_PARAMETER_NAME(top)
		BOOST_PARAMETER_NAME(right)
		BOOST_PARAMETER_NAME(bottom)
		BOOST_PARAMETER_NAME(left)
#endif // !ASCENSION_DOXYGEN_SHOULD_SKIP_THIS

		/// Base type of @c PhysicalFourSides class template.
		template<typename T>
		class PhysicalFourSidesBase : public std::array<T, 4> {
		public:
			/// Default constructor initializes nothing.
			PhysicalFourSidesBase() {}
			/// Constructor takes named parameters as initial values (default value is zero).
			template<typename Arguments>
			PhysicalFourSidesBase(const Arguments& arguments) {
//				top() = arguments[_top | value_type()];
//				right() = arguments[_right | value_type()];
//				bottom() = arguments[_bottom | value_type()];
//				left() = arguments[_left | value_type()];
				top() = arguments[_top.operator|(value_type())];
				right() = arguments[_right.operator|(value_type())];
				bottom() = arguments[_bottom.operator|(value_type())];
				left() = arguments[_left.operator|(value_type())];
			}
			/// Returns a reference to value of @a direction.
			reference operator[](PhysicalDirection direction) {
				return (*this)[boost::underlying_cast<size_type>(direction)];
			}
			/// Returns a reference to value of @a direction.
			const_reference operator[](PhysicalDirection direction) const {
				return (*this)[boost::underlying_cast<size_type>(direction)];
			}
			using std::array<T, 4>::operator[];
			/// Returns a reference to 'top' value.
			reference top() BOOST_NOEXCEPT {return std::get<PhysicalDirection::TOP>(*this);}
			/// Returns a reference to 'top' value.
			const_reference top() const BOOST_NOEXCEPT {return std::get<PhysicalDirection::TOP>(*this);}
			/// Returns a reference to 'right' value.
			reference right() BOOST_NOEXCEPT {return std::get<PhysicalDirection::RIGHT>(*this);}
			/// Returns a reference to 'right' value.
			const_reference right() const BOOST_NOEXCEPT {return std::get<PhysicalDirection::RIGHT>(*this);}
			/// Returns a reference to 'bottom' value.
			reference bottom() BOOST_NOEXCEPT {return std::get<PhysicalDirection::BOTTOM>(*this);}
			/// Returns a reference to 'bottom' value.
			const_reference bottom() const BOOST_NOEXCEPT {return std::get<PhysicalDirection::BOTTOM>(*this);}
			/// Returns a reference to 'left' value.
			reference left() BOOST_NOEXCEPT {return std::get<PhysicalDirection::LEFT>(*this);}
			/// Returns a reference to 'left' value.
			const_reference left() const BOOST_NOEXCEPT {return std::get<PhysicalDirection::LEFT>(*this);}
		};

		/**
		 * A collection of all physical directions.
		 * @tparam T Element type
		 * @see font#LineRelativeFourSides, presentation#FlowRelativeFourSides
		 */
		template<typename T>
		class PhysicalFourSides : public PhysicalFourSidesBase<T>,
			private boost::additive<PhysicalFourSides<T>, PhysicalTwoAxes<T>> {
		public:
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
			/// Compound-add operator calls same operators of @c T for the all elements.
			PhysicalFourSides& operator+=(const PhysicalTwoAxes<T>& other) {
				top() += other.y();
				right() += other.x();
				bottom() += other.y();
				left() += other.x();
				return *this;
			}
			/// Compound-subtract operator calls same operators of @c T for the all elements.
			PhysicalFourSides& operator-=(const PhysicalTwoAxes<T>& other) {
				top() -= other.y();
				right() -= other.x();
				bottom() -= other.y();
				left() -= other.x();
				return *this;
			}
		};

		/**
		 * Creates a @c PhysicalFourSides object, deducing the target type from the types of arguments.
		 * @tparam ArgumentPack The type of @a arguments
		 * @param arguments The named arguments same as the constructor of @c PhysicalFourSides class
		 */
		template<typename ArgumentPack>
		inline auto makePhysicalFourSides(const ArgumentPack& arguments)
				-> PhysicalFourSides<typename ascension::detail::DecayOrRefer<decltype(arguments[_top])>::Type> {
			typedef typename ascension::detail::DecayOrRefer<decltype(arguments[_top])>::Type Coordinate;
			static_assert(std::is_same<ascension::detail::DecayOrRefer<decltype(arguments[_right])>::Type, Coordinate>::value, "");
			static_assert(std::is_same<ascension::detail::DecayOrRefer<decltype(arguments[_bottom])>::Type, Coordinate>::value, "");
			static_assert(std::is_same<ascension::detail::DecayOrRefer<decltype(arguments[_left])>::Type, Coordinate>::value, "");
			return PhysicalFourSides<Coordinate>(_top = arguments[_top], _right = arguments[_right], _bottom = arguments[_bottom], _left = arguments[_left]);
		}

		/**
		 * Returns a range in horizontal direction of the given physical four sides.
		 * @tparam T An arithmetic type
		 * @param sides The physical four sides
		 * @return A range
		 * @see verticalRange, blockRange, inlineRange
		 */
		template<typename T>
		inline NumericRange<T> horizontalRange(const PhysicalFourSides<T>& sides) {
			static_assert(std::is_arithmetic<T>::value, "T is not arithmetic.");
			return nrange(sides.left(), sides.right());
		}

		/**
		 * Returns a range in vertical direction of the given physical four sides.
		 * @tparam T An arithmetic type
		 * @param sides The physical four sides
		 * @return A range
		 * @see horizontalRange, blockRange, inlineRange
		 */
		template<typename T>
		inline NumericRange<T> verticalRange(const PhysicalFourSides<T>& sides) {
			static_assert(std::is_arithmetic<T>::value, "T is not arithmetic.");
			return nrange(sides.top(), sides.bottom());
		}

		/**
		 * Returns 'width' of the given @c PhysicalFourSides.
		 * @tparam T An arithmetic type
		 * @param sides The physical four sides
		 * @return The 'width'
		 * @see extent, height, measure
		 */
		template<typename T>
		inline auto width(const PhysicalFourSides<T>& sides) -> decltype(boost::size(horizontalRange(sides))) {
			return boost::size(horizontalRange(sides));
		}

		/**
		 * Returns 'height' of the given @c PhysicalFourSides.
		 * @tparam T An arithmetic type
		 * @param sides The physical four sides
		 * @return The 'height'
		 * @see extent, measure, width
		 */
		template<typename T>
		inline auto height(const PhysicalFourSides<T>& sides) -> decltype(boost::size(verticalRange(sides))) {
			return boost::size(verticalRange(sides));
		}
		/// @}

		/// @name Free Functions to Convert Between Geometries and Abstract/Flow-Relative Instances
		/// @{

		namespace geometry {
			/**
			 * Converts a @c PhysicalTwoAxes into a point.
			 * @tparam Geometry Type of return value
			 * @tparam Coordinate Type of coordinate of @a axes
			 * @param axes A @c PhysicalTwoAxes object
			 * @return A converted point
			 */
			template<typename Geometry, typename Coordinate>
			inline Geometry make(const PhysicalTwoAxes<Coordinate>& axes,
					typename detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr) {
				return boost::geometry::make<Geometry>(axes.x(), axes.y());
			}
			/**
			 * Converts a @c PhysicalFourSides into a rectangle.
			 * @tparam Geometry Type of return value
			 * @tparam Coordinate Type of coordinate of @a sides
			 * @param sides A @c PhysicalFourSides object
			 * @return A converted rectangle
			 */
			template<typename Geometry, typename Coordinate>
			inline Geometry make(const PhysicalFourSides<Coordinate>& sides,
					typename detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
				Geometry result;
				boost::geometry::assign_values(result, sides.left(), sides.top(), sides.right(), sides.bottom());
				return result;
			}
		}
	}

	/// @}
}

namespace boost {
	namespace geometry {
		namespace traits {
			template<typename T>
			struct tag<ascension::graphics::PhysicalTwoAxes<T>> {
				typedef point_tag type;
			};
			template<typename T>
			struct dimension<ascension::graphics::PhysicalTwoAxes<T>> : boost::mpl::int_<2> {
			};
			template<typename T>
			struct coordinate_type<ascension::graphics::PhysicalTwoAxes<T>> {
				typedef T type;
			};
			template<typename T>
			struct coordinate_system<ascension::graphics::PhysicalTwoAxes<T>> {
				typedef cs::cartesian type;
			};
			template<typename T>
			struct access<ascension::graphics::PhysicalTwoAxes<T>, 0> {
				static T get(const ascension::graphics::PhysicalTwoAxes<T>& p) {
					return p.x();
				}
				static void set(ascension::graphics::PhysicalTwoAxes<T>& p, const T& value) {
					p.x() = value;
				}
			};
			template<typename T>
			struct access<ascension::graphics::PhysicalTwoAxes<T>, 1> {
				static T get(const ascension::graphics::PhysicalTwoAxes<T>& p) {
					return p.y();
				}
				static void set(ascension::graphics::PhysicalTwoAxes<T>& p, const T& value) {
					p.y() = value;
				}
			};
		}
	}
}

#endif // !ASCENSION_PHYSICAL_DIRECTIONS_DIMENSIONS_HPP
