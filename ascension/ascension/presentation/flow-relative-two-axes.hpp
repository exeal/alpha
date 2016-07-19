/**
 * @file flow-relative-two-axes.hpp
 * Defines @c FlowRelativeTwoAxes class template and related free functions.
 * @date 2012-03-31 created
 * @date 2012-2014 was directions.hpp
 * @date 2015-01-09 Separated from directions.hpp
 * @date 2016-06-20 Separated from flow-relative-directions-dimensions.hpp
 * @see line-relative-point.hpp, physical-two-axes.hpp
 */

#ifndef ASCENSION_FLOW_RELATIVE_TWO_AXES_HPP
#define ASCENSION_FLOW_RELATIVE_TWO_AXES_HPP
#include <ascension/corelib/detail/decay-or-refer.hpp>
#include <ascension/corelib/detail/named-argument-exists.hpp>
#include <boost/operators.hpp>
#include <boost/parameter.hpp>
#include <array>

namespace ascension {
	namespace presentation {
		/// @addtogroup flow_relative_directions_dimensions
		/// @{
#ifndef ASCENSION_DETAIL_DOXYGEN_IS_PREPROCESSING
		BOOST_PARAMETER_NAME(bpd)
		BOOST_PARAMETER_NAME(ipd)
#endif // !ASCENSION_DETAIL_DOXYGEN_IS_PREPROCESSING

		/// Base type of @c FlowRelativeTwoAxes class template.
		template<typename T>
		class FlowRelativeTwoAxesBase : public std::array<T, 2> {
		public:
			/// Default constructor initializes nothing.
			FlowRelativeTwoAxesBase() {}
#ifdef BOOST_COMP_MSVC
			FlowRelativeTwoAxesBase(const FlowRelativeTwoAxesBase& other) : std::array<T, 2>(other) {}
#endif	// BOOST_COMP_MSVC
			/// Constructor takes named parameters as initial values.
			template<typename Arguments>
			FlowRelativeTwoAxesBase(const Arguments& arguments) {
				if(ascension::detail::NamedArgumentExists<Arguments, tag::bpd>::value)
					bpd() = arguments[_bpd | value_type()];
				if(ascension::detail::NamedArgumentExists<Arguments, tag::ipd>::value)
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
		 * A collection of all abstract dimensions.
		 * @tparam T The coordinate type
		 * @note This entity is not described by W3C Writing Modes
		 * @see graphics#PhysicalTwoAxes, graphics#font#LineRelativeTwoAxes
		 */
		template<typename T>
		class FlowRelativeTwoAxes : public FlowRelativeTwoAxesBase<T>, private boost::additive<FlowRelativeTwoAxes<T>> {
		public:
#ifdef BOOST_COMP_MSVC
			FlowRelativeTwoAxes(const FlowRelativeTwoAxes& other) : FlowRelativeTwoAxesBase<T>(static_cast<const FlowRelativeTwoAxesBase<T>&>(other)) {}
#endif	// BOOST_COMP_MSVC
			/**
			 * Creates a @c FlowRelativeTwoAxes instance with the given initial values by named parameters.
			 * Omitted elements are initialized by the default constructor.
			 * @param bpd The initial value of 'block-dimension' (optional)
			 * @param ipd The initial value of 'inline-dimension' (optional)
			 */
			BOOST_PARAMETER_CONSTRUCTOR(
				FlowRelativeTwoAxes, (FlowRelativeTwoAxesBase<T>), tag,
				(optional
					(bpd, (value_type))
					(ipd, (value_type))))
			/// Compound-add operator calls same operators of @c T for @c #bpd() and @c #ipd().
			FlowRelativeTwoAxes& operator+=(const FlowRelativeTwoAxes& other) {
				bpd() += other.bpd();
				ipd() += other.ipd();
				return *this;
			}
			/// Compound-subtract operator calls same operators of @c T for @c #bpd() and @c #ipd().
			FlowRelativeTwoAxes& operator-=(const FlowRelativeTwoAxes& other) {
				bpd() -= other.bpd();
				ipd() -= other.ipd();
				return *this;
			}
		};

		namespace detail {
			template<typename Arguments>
			struct FlowRelativeTwoAxesFactoryResult {
				typedef typename boost::parameter::value_type<Arguments, tag::bpd, void>::type BpdType;
				typedef typename boost::parameter::value_type<Arguments, tag::ipd, void>::type IpdType;
				typedef typename ascension::detail::DecayOrRefer<
					typename std::conditional<!std::is_same<BpdType, void>::value, BpdType,
						typename std::conditional<!std::is_same<IpdType, void>::value, IpdType,
							void
						>::type
					>::type
				>::Type Type;
				static_assert(!std::is_same<Type, void>::value, "ascension.graphics.detail.FlowRelativeTwoAxesFactoryResult.Type");
			};
		}

		/**
		 * Creates a @c FlowRelativeTwoAxes object, deducing the target type from the types of arguments.
		 * @tparam Arguments The type of @a arguments
		 * @param arguments The named arguments same as the constructor of @c FlowRelativeTwoAxes class template
		 * @return A created @c FlowRelativeTwoAxes object
		 */
		template<typename Arguments>
		inline FlowRelativeTwoAxes<typename detail::FlowRelativeTwoAxesFactoryResult<Arguments>::Type> makeFlowRelativeTwoAxes(const Arguments& arguments) {
			typedef typename detail::FlowRelativeTwoAxesFactoryResult<Arguments>::Type Coordinate;
			return FlowRelativeTwoAxes<Coordinate>(_bpd = arguments[_bpd], _ipd = arguments[_ipd]);
		}
		/// @}
	}
}

#endif // !ASCENSION_FLOW_RELATIVE_TWO_AXES_HPP