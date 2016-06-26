/**
 * @file physical-two-axes.hpp
 * Defines @c PhysicalTwoAxes class template and related free functions.
 * @date 2012-03-31 created
 * @date 2012-2014 was directions.hpp
 * @date 2015-01-09 Separated from directions.hpp
 * @date 2016-06-26 Separated from physical-directions-dimensions.hpp
 * @see flow-relative-two-axes.hpp, line-relative-point.hpp, writing-mode.hpp
 */

#ifndef ASCENSION_PHYSICAL_TWO_AXES_HPP
#define ASCENSION_PHYSICAL_TWO_AXES_HPP
#include <ascension/corelib/detail/decay-or-refer.hpp>
#include <ascension/graphics/geometry/point-xy.hpp>
#include <boost/geometry/algorithms/make.hpp>
#include <boost/operators.hpp>
#include <boost/parameter.hpp>
#include <array>
#include <type_traits>

namespace ascension {
	namespace graphics {
		/// @addtogroup physical_directions_dimensions
		/// @{
#ifndef ASCENSION_DETAIL_DOXYGEN_IS_PREPROCESSING
		BOOST_PARAMETER_NAME(x)
		BOOST_PARAMETER_NAME(y)
#endif // !ASCENSION_DETAIL_DOXYGEN_IS_PREPROCESSING

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
		/// @}

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
		}
	}
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

#endif // !ASCENSION_PHYSICAL_TWO_AXES_HPP
