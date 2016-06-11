/**
 * @file dimension.hpp
 * Defines 2D dimension types for geometry.
 * @author exeal
 * @date 2010-11-06 Created (as geometry.hpp)
 * @date 2014-09-07 Separated from geometry.hpp
 */

#ifndef ASCENSION_GEOMETRY_DIMENSION_HPP
#define ASCENSION_GEOMETRY_DIMENSION_HPP
#include <ascension/graphics/geometry/common.hpp>
#include <ascension/graphics/geometry/named-parameters.hpp>
#include <boost/geometry/algorithms/detail/assign_values.hpp>	// oops...
#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/closure.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/geometry_id.hpp>
#include <boost/geometry/core/is_areal.hpp>
#include <boost/geometry/core/point_order.hpp>
#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/geometries/concepts/check.hpp>
#include <boost/operators.hpp>	// boost.equality_comparable
#include <boost/parameter.hpp>

namespace ascension {
	namespace graphics {
		namespace geometry {
			/// @addtogroup geometric_primitives
			/// @{

			template<typename Coordinate>
			class BasicDimensionBase : private boost::equality_comparable<BasicDimensionBase<Coordinate>> {
			public:
				bool operator==(const BasicDimensionBase& other) const {
					return dx_ == other.dx_ && dy_ == other.dy_;
				}
			protected:
				BasicDimensionBase() {}
				template<typename Arguments>
				BasicDimensionBase(const Arguments& arguments) : dx_(arguments[_dx]), dy_(arguments[_dy]) {}
				void swap(BasicDimensionBase& other) {
					using std::swap;
					swap(dx_, other.dx_);
					swap(dy_, other.dy_);
				}
			protected:
				Coordinate dx_, dy_;
			};

			template<typename Coordinate> class BasicDimension;
			template<typename Coordinate> Coordinate& dx(BasicDimension<Coordinate>& dimension);
			template<typename Coordinate> Coordinate dx(const BasicDimension<Coordinate>& dimension);
			template<typename Coordinate> Coordinate& dy(BasicDimension<Coordinate>& dimension);
			template<typename Coordinate> Coordinate dy(const BasicDimension<Coordinate>& dimension);

			/**
			 * Encapsulates a width and a height dimension in Cartesian coordinate system.
			 * @tparam Coordinate The coordinate type
			 */
			template<typename Coordinate>
			class BasicDimension : public BasicDimensionBase<Coordinate> {
			public:
				/// Default constructor does not initialize anything.
				BasicDimension() {}
				/// Copy-constructor.
				BasicDimension(const BasicDimension& other) : BasicDimensionBase<Coordinate>((_dx = dx(other), _dy = dy(other))) {}
				/// Copy-constructor for different coordinate type.
				template<typename OtherCoordinate>
				explicit BasicDimension(const BasicDimension<OtherCoordinate>& other) :
					BasicDimensionBase<Coordinate>((_dx = static_cast<Coordinate>(dx(other)), _dy = static_cast<Coordinate>(dy(other)))) {}
				BOOST_PARAMETER_CONSTRUCTOR(
					BasicDimension, (BasicDimensionBase<Coordinate>), tag,
					(required
						(dx, (Coordinate))
						(dy, (Coordinate))))

				/// Copy-assignment operator.
				BasicDimension& operator=(const BasicDimension& other) {
					BasicDimension(other).swap(*this);
					return *this;
				}
				/// Copy-assignment operator for different template parameter.
				template<typename U>
				BasicDimension& operator=(const BasicDimension<U>& other) {
					BasicDimension(other).swap(*this);
					return *this;
				}
				/// Swaps the two @c BasicDimension.
				friend void swap(BasicDimension& lhs, BasicDimension& rhs) {
					lhs.swap(rhs);
				}

			private:
				using BasicDimensionBase<Coordinate>::dx_;
				using BasicDimensionBase<Coordinate>::dy_;
				template<typename T> friend Coordinate& dx(BasicDimension<T>&);
				template<typename T> friend Coordinate dx(const BasicDimension<T>&);
				template<typename T> friend Coordinate& dy(BasicDimension<T>&);
				template<typename T> friend Coordinate dy(const BasicDimension<T>&);
			};
			/// @}

			/// @addtogroup geometry_additional_aceessors
			/// @{

			/// Returns the size of the @a dimension in x-coordinate.
			template<typename Coordinate>
			inline Coordinate dx(const BasicDimension<Coordinate>& dimension) {
				return dimension.dx_;	// $friendly-access$
			}

			/// Returns the size of the @a dimension in x-coordinate.
			template<typename Coordinate>
			inline Coordinate& dx(BasicDimension<Coordinate>& dimension) {
				return dimension.dx_;	// $friendly-access$
			}

			/// Returns the size of the @a dimensione in y-coordinate.
			template<typename Coordinate>
			inline Coordinate dy(const BasicDimension<Coordinate>& dimension) {
				return dimension.dy_;	// $friendly-access$
			}

			/// Returns the size of the @a dimensione in y-coordinate.
			template<typename Coordinate>
			inline Coordinate& dy(BasicDimension<Coordinate>& dimension) {
				return dimension.dy_;	// $friendly-access$
			}

			/// @}
		}
	}
}

namespace boost {
	namespace geometry {
		namespace traits {
			template<typename Coordinate>
			struct tag<ascension::graphics::geometry::BasicDimension<Coordinate>> {
				typedef ascension::graphics::geometry::DimensionTag type;
			};
		}

		namespace core_dispatch {
			template<typename Geometry, typename Coordinate, std::size_t dimension>
			struct access<ascension::graphics::geometry::DimensionTag, Geometry, Coordinate, dimension, boost::false_type> {
				static Coordinate get(const Geometry& g) {
					return get(g, std::integral_constant<std::size_t, dimension>());
				}
				static void set(Geometry& g, const Coordinate& value) {
					return set(g, value, std::integral_constant<std::size_t, dimension>());
				}
			private:
				static Coordinate get(const Geometry& g, std::integral_constant<std::size_t, 0>) {
					return ascension::graphics::geometry::dx(g);
				}
				static Coordinate get(const Geometry& g, std::integral_constant<std::size_t, 1>) {
					return ascension::graphics::geometry::dy(g);
				}
				static void set(Geometry& g, const Coordinate& value, std::integral_constant<std::size_t, 0>) {
					ascension::graphics::geometry::dx(g) = value;
				}
				static void set(Geometry& g, const Coordinate& value, std::integral_constant<std::size_t, 1>) {
					ascension::graphics::geometry::dy(g) = value;
				}
			};
			template<typename Geometry>
			struct closure<ascension::graphics::geometry::DimensionTag, Geometry> {
				static const closure_selector value = closure_undertermined;
			};
			template<typename Geometry>
			struct coordinate_system<ascension::graphics::geometry::DimensionTag, Geometry> {
				typedef cs::cartesian type;
			};
			template<typename Coordinate>
			struct coordinate_type<ascension::graphics::geometry::DimensionTag, ascension::graphics::geometry::BasicDimension<Coordinate>> {
				typedef void point_type;
				typedef Coordinate type;
			};
			template<typename Geometry>
			struct dimension<ascension::graphics::geometry::DimensionTag, Geometry> : boost::mpl::int_<2> {};
			template<>
			struct geometry_id<ascension::graphics::geometry::DimensionTag> : boost::mpl::int_<493875> {};
			template<>
			struct is_areal<ascension::graphics::geometry::DimensionTag> : boost::true_type {};
			template<typename Geometry>
			struct point_order<ascension::graphics::geometry::DimensionTag, Geometry> {
				static const order_selector value = order_undetermined;
			};
			template<typename Geometry>
			struct point_type<ascension::graphics::geometry::DimensionTag, Geometry> {
				typedef void/*boost::geometry::model::d2::point_xy<typename geometry::coordinate_type<Geometry>::type>*/ type;
			};
		}

		namespace dispatch {
			template<typename Geometry>
			struct assign_zero<ascension::graphics::geometry::DimensionTag, Geometry> {
				static void apply(Geometry& dimension) {
					geometry::assign_value(dimension, 0);
				}
			};
			template<typename Geometry, bool isConst>
			struct check<Geometry, ascension::graphics::geometry::DimensionTag, isConst> /*: check<Geometry, point_tag, isConst>*/ {};
		}
	}
}	// boost.geometry.traits

namespace ascension {
	namespace graphics {
		namespace geometry {
			/// @defgroup geometry_dimension_specific_algorithms BasicDimension Specific Algorithms
			/// @{
			template<typename Geometry1, typename Geometry2>
			inline bool equals(const Geometry1& dimension1, const Geometry2& dimension2,
					typename detail::EnableIfTagIs<Geometry1, DimensionTag>::type* = nullptr,
					typename detail::EnableIfTagIs<Geometry2, DimensionTag>::type* = nullptr) {
				return dx(dimension1) == dx(dimension2) && dy(dimension1) == dy(dimension2);
			}
			/// @}
		}

		typedef geometry::BasicDimension<Scalar> Dimension;
	}
}

#endif // !ASCENSION_GEOMETRY_DIMENSION_HPP
