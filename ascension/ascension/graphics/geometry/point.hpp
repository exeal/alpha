/**
 * @file point.hpp
 * Defines 2D point types for geometry.
 * @author exeal
 * @date 2010-11-06 Created (as geometry.hpp)
 * @date 2014-09-07 Separated from geometry.hpp
 */

#ifndef ASCENSION_GEOMETRY_POINT_HPP
#define ASCENSION_GEOMETRY_POINT_HPP

#include <ascension/graphics/geometry/common.hpp>
#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/core/coordinate_system.hpp>
#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/core/cs.hpp>
#include <boost/mpl/int.hpp>
#include <boost/operators.hpp>	// boost.equality_comparable
#include <boost/parameter.hpp>

namespace ascension {
	namespace graphics {
		namespace geometry {

			/// @addtogroup geometric_primitives
			/// @{

#ifndef ASCENSION_DOXYGEN_SHOULD_SKIP_THIS
			BOOST_PARAMETER_NAME(x)
			BOOST_PARAMETER_NAME(y)
#endif	// !ASCENSION_DOXYGEN_SHOULD_SKIP_THIS

			template<typename Coordinate>
			class BasicPointBase : private boost::equality_comparable<BasicPointBase<Coordinate>> {
			public:
				bool operator==(const BasicPointBase& other) const {
//					return boost::geometry::equals(*this, other);
					return x_ == other.x_ && y_ == other.y_;
				}
			protected:
				BasicPointBase() {}
				template<typename Arguments>
				BasicPointBase(const Arguments& arguments) : x_(arguments[_x]), y_(arguments[_y]) {}
			protected:
				Coordinate x_, y_;
			};

			/**
			 * Defines a point representing a location in Cartesian coordinate system.
			 * @tparam Coordinate The coordinate type
			 * @note Some specializations of this class are registered into Boost.Geometry.
			 * @see graphics#PhysicalTwoAxesBase, graphics#PhysicalTwoAxes
			 */
			template<typename Coordinate>
			class BasicPoint : public BasicPointBase<Coordinate> {
			public:
				/// Default constructor does not initialize anything.
				BasicPoint() {}
				/// Copy-constructor.
				BasicPoint(const BasicPoint& other) : BasicPointBase<Coordinate>((_x = x(other), _y = y(other))) {}
				/// Copy-constructor for different template parameter.
				template<typename U>
				BasicPoint(const BasicPoint<U>& other) : BasicPointBase<Coordinate>((_x = x(other), _y = y(other))) {}
				/// Copy-constructor for different point type.
				template<typename Other>
				BasicPoint(const Other& other) : BasicPointBase<Coordinate>((_x = x(other), _y = y(other))) {}
				BOOST_PARAMETER_CONSTRUCTOR(
					BasicPoint, (BasicPointBase<Coordinate>), tag,
					(required
						(x, (Coordinate))
						(y, (Coordinate))))

				/// Copy-assignment operator.
				BasicPoint& operator=(const BasicPoint& other) {
					std::swap(*this, BasicPoint(other));
					return *this;
				}
				/// Copy-assignment operator for different template parameter.
				template<typename U>
				BasicPoint& operator=(const BasicPoint<U>& other) {
					std::swap(*this, BasicPoint(other));
					return *this;
				}

			private:
				using BasicPointBase<Coordinate>::x_;
				using BasicPointBase<Coordinate>::y_;
				friend struct boost::geometry::traits::access<BasicPoint<Coordinate>, 0>;
				friend struct boost::geometry::traits::access<BasicPoint<Coordinate>, 1>;
			};
			/// @}
		}
	}
}

namespace boost {
	namespace geometry {
		namespace traits {
			template<typename Coordinate>
			struct tag<ascension::graphics::geometry::BasicPoint<Coordinate>> {
				typedef point_tag type;
			};
			template<typename Coordinate>
			struct dimension<ascension::graphics::geometry::BasicPoint<Coordinate>> : boost::mpl::int_<2> {
			};
			template<typename Coordinate>
			struct coordinate_type<ascension::graphics::geometry::BasicPoint<Coordinate>> {
				typedef Coordinate type;
			};
			template<typename Coordinate>
			struct coordinate_system<ascension::graphics::geometry::BasicPoint<Coordinate>> {
				typedef cs::cartesian type;
			};
			template<typename Coordinate>
			struct access<ascension::graphics::geometry::BasicPoint<Coordinate>, 0> {
				static Coordinate get(const ascension::graphics::geometry::BasicPoint<Coordinate>& p) {
					return p.x_;
				}
				static void set(ascension::graphics::geometry::BasicPoint<Coordinate>& p, const Coordinate& value) {
					p.x_ = value;
				}
			};
			template<typename Coordinate>
			struct access<ascension::graphics::geometry::BasicPoint<Coordinate>, 1> {
				static Coordinate get(const ascension::graphics::geometry::BasicPoint<Coordinate>& p) {
					return p.y_;
				}
				static void set(ascension::graphics::geometry::BasicPoint<Coordinate>& p, const Coordinate& value) {
					p.y_ = value;
				}
			};
		}
	}
}	// boost.geometry.traits

namespace ascension {
	namespace graphics {
		typedef geometry::BasicPoint<Scalar> Point;
	}
}

#endif // !ASCENSION_GEOMETRY_POINT_HPP
