/**
 * @file line-relative-point.hpp
 * Defines line-relative directional and dimensional terms.
 * @date 2012-03-31 created
 * @date 2012-2014 was directions.hpp
 * @date 2015-01-09 Separated from directions.hpp
 * @date 2016-06-26 Separated from line-relative-directions-dimensions.hpp
 * @see flow-relative-two-axes.hpp, physical-two-axes.hpp, writing-mode.hpp
 */

#ifndef ASCENSION_LINE_RELATIVE_POINT_HPP
#define ASCENSION_LINE_RELATIVE_POINT_HPP
#include <ascension/corelib/future/scoped-enum-emulation.hpp>
#include <boost/operators.hpp>
#include <boost/parameter.hpp>
#include <array>

namespace ascension {
	namespace graphics {
		namespace font {
			/// @addtogroup line_relative_directions_dimensions
			/// @{
#ifndef ASCENSION_DETAIL_DOXYGEN_IS_PREPROCESSING
			BOOST_PARAMETER_NAME(u)
			BOOST_PARAMETER_NAME(v)
#endif // !ASCENSION_DETAIL_DOXYGEN_IS_PREPROCESSING

			/// Base type of @c LineRelativePoint class template.
			template<typename T>
			class LineRelativePointBase : public std::array<T, 2> {
			public:
				/// Default constructor initializes nothing.
				LineRelativePointBase() {}
				/// Copy-constructor.
				LineRelativePointBase(const LineRelativePointBase<T>& other) : std::array<T, 2>(other) {}
				/// Constructor takes named parameters as initial values.
				template<typename Arguments>
				LineRelativePointBase(const Arguments& arguments) {
//					u() = arguments[_u | value_type()];
//					v() = arguments[_v | value_type()];
					u() = arguments[_u.operator|(value_type())];
					v() = arguments[_v.operator|(value_type())];
				}
				/// Returns a reference 'u' value.
				value_type& u() BOOST_NOEXCEPT {return std::get<0>(*this);}
				/// Returns a reference 'u' value.
				const value_type& u() const BOOST_NOEXCEPT {return std::get<0>(*this);}
				/// Returns a reference 'v' value.
				value_type& v() BOOST_NOEXCEPT {return std::get<1>(*this);}
				/// Returns a reference 'v' value.
				const value_type& v() BOOST_NOEXCEPT const {return std::get<1>(*this);}
			};

			/**
			 * A collection of all physical dimensions. This is a point in the line box. 'u' corresponds to the 'x'
			 * coordinate and 'v' corresponds to the 'y' coordinate in 'horizontal-tb' writing-mode.
			 * @tparam T The coordinate type
			 * @see presentation#FlowRelativeTwoAxes
			 */
			template<typename T>
			class LineRelativePoint : public LineRelativePointBase<T>, private boost::additive<LineRelativePoint<T>> {
			public:
				/// Default constructor initializes nothing.
				LineRelativePoint() {}
				/// Copy-constructor.
				LineRelativePoint(const LineRelativePoint<T>& other) : LineRelativePointBase<T>(static_cast<const LineRelativePointBase<T>&>(other)) {}
				/**
				 * Constructor which takes named parameters as initial values (default value is zero).
				 * @param u The initial value of 'u'
				 * @param v The initial value of 'v'
				 */
#ifndef ASCENSION_DETAIL_DOXYGEN_IS_PREPROCESSING
				BOOST_PARAMETER_CONSTRUCTOR(
					LineRelativePoint, (LineRelativePointBase<T>), tag,
					(required
						(u, (value_type))
						(v, (value_type))))
#else
				LineRelativePoint(value_type u, value_type v);
#endif
				/// Compound-add operator calls same operators of @c T for @c #u and @c #v.
				LineRelativePoint& operator+=(const LineRelativePoint<T>& other) {
					u() += other.u();
					v() += other.v();
					return *this;
				}
				/// Compound-subtract operator calls same operators of @c T for @c #u and @c #v.
				LineRelativePoint& operator-=(const PhysicalTwoAxes<T>& other) {
					u() -= other.u();
					v() -= other.v();
					return *this;
				}
			};
			/// @}
		}
	}
}

#endif // !ASCENSION_LINE_RELATIVE_POINT_HPP
