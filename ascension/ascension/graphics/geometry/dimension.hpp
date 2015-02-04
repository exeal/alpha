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
#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/operators.hpp>	// boost.equality_comparable
#include <boost/parameter.hpp>

namespace ascension {
	namespace graphics {
		namespace geometry {

			/// @addtogroup geometric_primitives
			/// @{

#ifndef ASCENSION_DOXYGEN_SHOULD_SKIP_THIS
			BOOST_PARAMETER_NAME(dx)
			BOOST_PARAMETER_NAME(dy)
#endif	// !ASCENSION_DOXYGEN_SHOULD_SKIP_THIS

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
					std::swap(*this, BasicDimension(other));
					return *this;
				}
				/// Copy-assignment operator for different template parameter.
				template<typename U>
				BasicDimension& operator=(const BasicDimension<U>& other) {
					std::swap(*this, BasicDimension(other));
					return *this;
				}

			private:
				using BasicDimensionBase<Coordinate>::dx_;
				using BasicDimensionBase<Coordinate>::dy_;
				template<typename T> friend Coordinate& dx(BasicDimension<T>&);
				template<typename T> friend Coordinate dx(const BasicDimension<T>&);
				template<typename T> friend Coordinate& dy(BasicDimension<T>&);
				template<typename T> friend Coordinate dy(const BasicDimension<T>&);
			};

			struct DimensionTag {};
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
			template<typename Coordinate>
			struct coordinate_type<ascension::graphics::geometry::DimensionTag, ascension::graphics::geometry::BasicDimension<Coordinate>> {
				typedef void point_type;
				typedef Coordinate type;
			};
		}
	}
}	// boost.geometry.traits

namespace ascension {
	namespace graphics {
		typedef geometry::BasicDimension<Scalar> Dimension;
	}
}

#endif // !ASCENSION_GEOMETRY_DIMENSION_HPP
