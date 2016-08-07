/**
 * @file line-relative-point.hpp
 * Defines @c LineRelative class template and related free functions.
 * @date 2012-03-31 created
 * @date 2012-2014 was directions.hpp
 * @date 2015-01-09 Separated from directions.hpp
 * @date 2016-06-26 Separated from line-relative-directions-dimensions.hpp
 * @see flow-relative-two-axes.hpp, physical-two-axes.hpp, writing-mode.hpp
 */

#ifndef ASCENSION_LINE_RELATIVE_POINT_HPP
#define ASCENSION_LINE_RELATIVE_POINT_HPP
#include <ascension/corelib/detail/named-argument-exists.hpp>
#include <ascension/corelib/detail/named-arguments-single-type.hpp>
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
					if(const boost::optional<value_type> value = arguments[_u | boost::none])
						u() = boost::get(value);
					if(const boost::optional<value_type> value = arguments[_v | boost::none])
						v() = boost::get(value);
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
				/// Copy-constructor.
				LineRelativePoint(const LineRelativePoint<T>& other) : LineRelativePointBase<T>(static_cast<const LineRelativePointBase<T>&>(other)) {}
				/**
				 * Creates a @c LineRelativePoint with the given initial values as named parameters.
				 * @param u The initial value of 'u' (optional)
				 * @param v The initial value of 'v' (optional)
				 */
#ifndef ASCENSION_DETAIL_DOXYGEN_IS_PREPROCESSING
				BOOST_PARAMETER_CONSTRUCTOR(
					LineRelativePoint, (LineRelativePointBase<T>), tag,
					(optional
						(u, (boost::optional<value_type>))
						(v, (boost::optional<value_type>))))
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
				LineRelativePoint& operator-=(const LineRelativePoint<T>& other) {
					u() -= other.u();
					v() -= other.v();
					return *this;
				}
			};

			/**
			 * Creates a @c LineRelativePoint object, deducing the target type from the types of arguments.
			 * @tparam Arguments The type of @a arguments
			 * @param arguments The named arguments same as the constructor of @c LineRelativePoint class template
			 * @return A created @c LineRelativePoint object
			 */
			template<typename Arguments>
			inline LineRelativePoint<
				typename ascension::detail::NamedArgumentsSingleType<
					Arguments, tag::u, tag::v
				>::Type
			> makeLineRelativePoint(const Arguments& arguments) {
				typedef typename ascension::detail::NamedArgumentsSingleType<Arguments, tag::u, tag::v>::Type Coordinate;
				boost::optional<Coordinate> u, v;
				if(ascension::detail::NamedArgumentExists<Arguments, tag::u>::value)
					u = arguments[_u];
				if(ascension::detail::NamedArgumentExists<Arguments, tag::v>::value)
					v = arguments[_v];
				return LineRelativePoint<Coordinate>(_u = u, _v = v);
			}
			/// @}
		}
	}
}

#endif // !ASCENSION_LINE_RELATIVE_POINT_HPP
