/**
 * @file flow-relative-four-sides.hpp
 * Defines @c FlowRelativeFourSides class template and free functions.
 * @date 2012-03-31 created
 * @date 2012-2014 was directions.hpp
 * @date 2015-01-09 Separated from directions.hpp
 * @date 2016-06-20 Separated from flow-relative-directions-dimensions.hpp
 * @see line-relative-four0-sides.hpp, physical-four-sides.hpp, writing-mode.hpp
 */

#ifndef ASCENSION_FLOW_RELATIVE_FOUR_SIDES_HPP
#define ASCENSION_FLOW_RELATIVE_FOUR_SIDES_HPP
#include <ascension/corelib/detail/named-argument-exists.hpp>
#include <ascension/corelib/numeric-range.hpp>
#include <ascension/presentation/flow-relative-direction.hpp>
#include <ascension/presentation/flow-relative-two-axes.hpp>
#include <boost/functional/hash.hpp>
#include <functional>	// std.hash
#ifndef ASCENSION_NO_XSL_FLOW_RELATIVE_DIRECTIONS
#	include <iterator>	// std.end
#endif

namespace ascension {
	namespace presentation {
		/// @addtogroup flow_relative_directions_dimensions
		/// @{
#ifndef ASCENSION_DETAIL_DOXYGEN_IS_PREPROCESSING
		BOOST_PARAMETER_NAME(blockStart)
		BOOST_PARAMETER_NAME(blockEnd)
		BOOST_PARAMETER_NAME(inlineStart)
		BOOST_PARAMETER_NAME(inlineEnd)
#	ifndef ASCENSION_NO_XSL_FLOW_RELATIVE_DIRECTIONS
		BOOST_PARAMETER_NAME(before)
		BOOST_PARAMETER_NAME(after)
		BOOST_PARAMETER_NAME(start)
		BOOST_PARAMETER_NAME(end)
#	endif // !ASCENSION_NO_XSL_FLOW_RELATIVE_DIRECTIONS
#endif // !ASCENSION_DETAIL_DOXYGEN_IS_PREPROCESSING

		/// Base type of @c FlowRelativeFourSides class template.
		template<typename T>
		class FlowRelativeFourSidesBase : public std::array<T, 4> {
		public:
			/// Default constructor initializes nothing.
			FlowRelativeFourSidesBase() {}
#ifdef BOOST_COMP_MSVC
			FlowRelativeFourSidesBase(const FlowRelativeFourSidesBase& other) : std::array<T, 4>(other) {}
			FlowRelativeFourSidesBase(FlowRelativeFourSidesBase&& other) : std::array<T, 4>(other) {}
#endif	// BOOST_COMP_MSVC
			/// Constructor takes named parameters as initial values.
			template<typename Arguments>
			FlowRelativeFourSidesBase(const Arguments& arguments) {
				const bool cssKeyword =
					ascension::detail::NamedArgumentExists<Arguments, tag::blockStart>::value
					|| ascension::detail::NamedArgumentExists<Arguments, tag::blockEnd>::value
					|| ascension::detail::NamedArgumentExists<Arguments, tag::inlineStart>::value
					|| ascension::detail::NamedArgumentExists<Arguments, tag::inlineEnd>::value;
				const bool xslKeyword =
#ifndef ASCENSION_NO_XSL_FLOW_RELATIVE_DIRECTIONS
					ascension::detail::NamedArgumentExists<Arguments, tag::before>::value
					|| ascension::detail::NamedArgumentExists<Arguments, tag::after>::value
					|| ascension::detail::NamedArgumentExists<Arguments, tag::start>::value
					|| ascension::detail::NamedArgumentExists<Arguments, tag::end>::value;
#else
					false;
#endif
				const int m = cssKeyword ? 1 : 0;
				const int n = m + (xslKeyword ? 2 : 0);
				initialize(arguments, boost::mpl::int_<(n != 0) ? n : 1>());
			}
			/// Returns a reference to value of @a direction.
			reference operator[](FlowRelativeDirection direction) {
				return (*this)[boost::underlying_cast<size_type>(direction)];
			}
			/// Returns a reference to value of @a direction.
			const_reference operator[](FlowRelativeDirection direction) const {
				return (*this)[boost::underlying_cast<size_type>(direction)];
			}
			using std::array<T, 4>::operator[];
			/// Returns a reference to 'block-start' value.
			reference blockStart() BOOST_NOEXCEPT {return std::get<FlowRelativeDirection::BLOCK_START>(*this);}
			/// Returns a reference to 'block-start' value.
			const_reference blockStart() const BOOST_NOEXCEPT {return std::get<FlowRelativeDirection::BLOCK_START>(*this);}
			/// Returns a reference to 'block-end' value.
			reference blockEnd() BOOST_NOEXCEPT {return std::get<FlowRelativeDirection::BLOCK_END>(*this);}
			/// Returns a reference to 'block-end' value.
			const_reference blockEnd() const BOOST_NOEXCEPT {return std::get<FlowRelativeDirection::BLOCK_END>(*this);}
			/// Returns a reference to 'line-start' value.
			reference inlineStart() BOOST_NOEXCEPT {return std::get<FlowRelativeDirection::INLINE_START>(*this);}
			/// Returns a reference to 'line-start' value.
			const_reference inlineStart() const BOOST_NOEXCEPT {return std::get<FlowRelativeDirection::INLINE_START>(*this);}
			/// Returns a reference to 'line-end' value.
			reference inlineEnd() BOOST_NOEXCEPT {return std::get<FlowRelativeDirection::INLINE_END>(*this);}
			/// Returns a reference to 'line-end' value.
			const_reference inlineEnd() const BOOST_NOEXCEPT {return std::get<FlowRelativeDirection::INLINE_END>(*this);}
#ifndef ASCENSION_NO_XSL_FLOW_RELATIVE_DIRECTIONS
			/// Returns a reference to 'before' value.
			reference before() BOOST_NOEXCEPT {return std::get<FlowRelativeDirection::BEFORE>(*this);}
			/// Returns a reference to 'before' value.
			const_reference before() const BOOST_NOEXCEPT {return std::get<FlowRelativeDirection::BEFORE>(*this);}
			/// Returns a reference to 'after' value.
			reference after() BOOST_NOEXCEPT {return std::get<FlowRelativeDirection::AFTER>(*this);}
			/// Returns a reference to 'after' value.
			const_reference after() const BOOST_NOEXCEPT {return std::get<FlowRelativeDirection::AFTER>(*this);}
			/// Returns a reference to 'start' value.
			reference start() BOOST_NOEXCEPT {return std::get<FlowRelativeDirection::START>(*this);}
			/// Returns a reference to 'start' value.
			const_reference start() const BOOST_NOEXCEPT {return std::get<FlowRelativeDirection::START>(*this);}
			/// Returns a reference to 'end' value.
			/// @note This method hides @c std#array#end.
			reference end() BOOST_NOEXCEPT {return std::get<FlowRelativeDirection::END>(*this);}
			/// Returns a reference to 'end' value.
			/// @note This method hides @c std#array#end.
			const_reference end() const BOOST_NOEXCEPT {return std::get<FlowRelativeDirection::END>(*this);}
#endif // !ASCENSION_NO_XSL_FLOW_RELATIVE_DIRECTIONS

		private:
			template<typename Arguments>
			void initialize(const Arguments& arguments, boost::mpl::int_<1>) {
				if(ascension::detail::NamedArgumentExists<Arguments, tag::blockStart>::value)
					blockStart() = arguments[_blockStart | value_type()];
				if(ascension::detail::NamedArgumentExists<Arguments, tag::blockEnd>::value)
					blockEnd() = arguments[_blockEnd | value_type()];
				if(ascension::detail::NamedArgumentExists<Arguments, tag::inlineStart>::value)
					inlineStart() = arguments[_inlineStart | value_type()];
				if(ascension::detail::NamedArgumentExists<Arguments, tag::inlineEnd>::value)
					inlineEnd() = arguments[_inlineEnd | value_type()];
			}
#ifndef ASCENSION_NO_XSL_FLOW_RELATIVE_DIRECTIONS
			template<typename Arguments>
			void initialize(const Arguments& arguments, boost::mpl::int_<2>) {
				if(ascension::detail::NamedArgumentExists<Arguments, tag::before>::value)
					before() = arguments[_before | value_type()];
				if(ascension::detail::NamedArgumentExists<Arguments, tag::after>::value)
					after() = arguments[_after | value_type()];
				if(ascension::detail::NamedArgumentExists<Arguments, tag::start>::value)
					start() = arguments[_start | value_type()];
				if(ascension::detail::NamedArgumentExists<Arguments, tag::end>::value)
					end() = arguments[_end | value_type()];
			}
#endif // !ASCENSION_NO_XSL_FLOW_RELATIVE_DIRECTIONS
		};

		/**
		 * A collection of all flow-relative directions.
		 * @tparam T The element type
		 * @see graphics#PhysicalFourSides, graphics#font#LineRelativeFourSides
		 */
		template<typename T>
		class FlowRelativeFourSides : public FlowRelativeFourSidesBase<T>,
			private boost::additive<FlowRelativeFourSides<T>, FlowRelativeTwoAxes<T>> {
		public:
#ifdef BOOST_COMP_MSVC
			FlowRelativeFourSides(const FlowRelativeFourSides& other) : FlowRelativeFourSidesBase(static_cast<const FlowRelativeFourSidesBase<T>&>(other)) {}
			FlowRelativeFourSides(FlowRelativeFourSides&& other) : FlowRelativeFourSidesBase(std::forward<FlowRelativeFourSidesBase>(other)) {}
#endif	// BOOST_COMP_MSVC
#ifndef ASCENSION_NO_XSL_FLOW_RELATIVE_DIRECTIONS
			/**
			 * Creates a @c FlowRelativeFourSides instance with the given initial values by named parameters.
			 * Omitted elements are initialized by the default constructor.
			 * @param blockStart The initial value of 'block-start' (optional)
			 * @param blockEnd The initial value of 'block-end' (optional)
			 * @param inlineStart The initial value of 'inline-start' (optional)
			 * @param inlineEnd The initial value of 'inlineEnd' (optional)
			 * @param before The initial value of 'before' (alias of 'block-start') (optional)
			 * @param after The initial value of 'after' (alias of 'block-end') (optional)
			 * @param start The initial value of 'start' (alias of 'inline-start') (optional)
			 * @param end The initial value of 'end' (alias of 'inlineEnd') (optional)
			 */
			BOOST_PARAMETER_CONSTRUCTOR(
				FlowRelativeFourSides, (FlowRelativeFourSidesBase<T>), tag,
				(optional
					(blockStart, (value_type))
					(blockEnd, (value_type))
					(inlineStart, (value_type))
					(inlineEnd, (value_type))
					(before, (value_type))
					(after, (value_type))
					(start, (value_type))
					(end, (value_type))))
#else
			/**
			 * Creates a @c FlowRelativeFourSides instance with the given initial values by named parameters.
			 * Omitted elements are initialized by the default constructor.
			 * @param blockStart The initial value of 'block-start' (optional)
			 * @param blockEnd The initial value of 'block-end' (optional)
			 * @param inlineStart The initial value of 'inline-start' (optional)
			 * @param inlineEnd The initial value of 'inlineEnd' (optional)
			 */
			BOOST_PARAMETER_CONSTRUCTOR(
				FlowRelativeFourSides, (FlowRelativeFourSidesBase<T>), tag,
				(optional
					(blockStart, (value_type))
					(blockEnd, (value_type))
					(inlineStart, (value_type))
					(inlineEnd, (value_type))))
#endif // !ASCENSION_NO_XSL_FLOW_RELATIVE_DIRECTIONS
			template<typename U>
#if !defined(BOOST_COMP_MSVC) || BOOST_COMP_MSVC >= BOOST_VERSION_NUMBER(11, 0, 0)
			FlowRelativeFourSides(const U& other, typename std::enable_if<std::is_constructible<T, const U&>::value>::type* = nullptr)
#else
			FlowRelativeFourSides(const U& other, typename std::enable_if<std::is_convertible<U, T>::value>::type* = nullptr)
#endif
				: FlowRelativeFourSidesBase<T>((_blockStart = other, _blockEnd = other, _lineStart = other, _lineEnd = other)) {}
			/// Compound-add operator calls same operators of @c T for the all elements.
			FlowRelativeFourSides& operator+=(const FlowRelativeTwoAxes<T>& other) {
				blockStart() += other.bpd();
				blockEnd() += other.bpd();
				inlineStart() += other.ipd();
				inlineEnd() += other.ipd();
				return *this;
			}
			/// Compound-subtract operator calls same operators of @c T for the all elements.
			FlowRelativeFourSides& operator-=(const FlowRelativeTwoAxes<T>& other) {
				blockStart() -= other.bpd();
				blockEnd() -= other.bpd();
				inlineStart() -= other.ipd();
				inlineEnd() -= other.ipd();
				return *this;
			}
		};

		/**
		 * Creates a @c FlowRelativeFourSides object, deducing the target type from the types of arguments.
		 * @tparam ArgumentPack The type of @a arguments
		 * @param arguments The named arguments same as the constructor of @c FlowRelativeFourSides class
		 */
		template<typename ArgumentPack>
		inline auto makeFlowRelativeFourSides(const ArgumentPack& arguments)
				-> FlowRelativeFourSides<typename ascension::detail::DecayOrRefer<decltype(arguments[_blockStart])>::Type> {
			typedef typename ascension::detail::DecayOrRefer<decltype(arguments[_blockStart])>::Type Coordinate;
			static_assert(std::is_same<ascension::detail::DecayOrRefer<decltype(arguments[_blockEnd])>::Type, Coordinate>::value, "");
			static_assert(std::is_same<ascension::detail::DecayOrRefer<decltype(arguments[_inlineStart])>::Type, Coordinate>::value, "");
			static_assert(std::is_same<ascension::detail::DecayOrRefer<decltype(arguments[_inlineEnd])>::Type, Coordinate>::value, "");
			return FlowRelativeFourSides<Coordinate>(arguments);
		}

		/**
		 * Returns a range of the given @c FlowRelativeFourSides in block axis.
		 * @tparam T An arithmetic type
		 * @param sides The flow-relative four sides
		 * @return A range
		 * @see inlineRange, horizontalRange, verticalRange
		 */
		template<typename T>
		inline NumericRange<T> blockRange(const FlowRelativeFourSides<T>& sides) {
			static_assert(std::is_arithmetic<T>::value, "T is not arithmetic.");
			return nrange(sides.blockStart(), sides.blockEnd());
		}

		/**
		 * Returns a range of the given @c FlowRelativeFourSides in inline axis.
		 * @tparam T An arithmetic type
		 * @param sides The flow-relative four sides
		 * @return A range
		 * @see blockRange, horizontalRange, verticalRange
		 */
		template<typename T>
		inline NumericRange<T> inlineRange(const FlowRelativeFourSides<T>& sides) {
			static_assert(std::is_arithmetic<T>::value, "T is not arithmetic.");
			return nrange(sides.inlineStart(), sides.inlineEnd());
		}

		/**
		 * Returns 'extent' of the given @c FlowRelativeFourSides.
		 * @tparam T An arithmetic type
		 * @param sides The flow-relative four sides
		 * @return The 'extent'
		 * @see height, measure, width
		 */
		template<typename T>
		inline auto extent(const FlowRelativeFourSides<T>& sides) -> decltype(boost::size(blockRange(sides))) {
			return boost::size(blockRange(sides));
		}

		/**
		 * Returns 'measure' of the given @c FlowRelativeFourSides.
		 * @tparam T An arithmetic type
		 * @param sides The flow-relative four sides
		 * @return The 'measure'
		 * @see extent, height, width
		 */
		template<typename T>
		inline auto measure(const FlowRelativeFourSides<T>& sides) -> decltype(boost::size(inlineRange(sides))) {
			return boost::size(inlineRange(sides));
		}

		/// Specialization of @c boost#hash_value function template for @c FlowRelativeFourSides.
		template<typename T>
		inline std::size_t hash_value(const FlowRelativeFourSides<T>& object) BOOST_NOEXCEPT {
			return boost::hash_range(std::begin(object), std::end(object));
		}
		/// @}
	}
}

namespace std {
#ifndef ASCENSION_NO_XSL_FLOW_RELATIVE_DIRECTIONS
	/// Specialization of @c std#end for @c presentation#FlowRelativeFourSides#end duplication.
	template<typename T>
	inline typename ascension::presentation::FlowRelativeFourSides<T>::iterator end(ascension::presentation::FlowRelativeFourSides<T>& v) {
		return std::end(static_cast<std::array<T, 4>&>(v));
	}
	template<typename T>
	inline typename ascension::presentation::FlowRelativeFourSides<T>::const_iterator end(const ascension::presentation::FlowRelativeFourSides<T>& v) {
		return std::end(static_cast<const std::array<T, 4>&>(v));
	}
#endif // !ASCENSION_NO_XSL_FLOW_RELATIVE_DIRECTIONS

	/// Specialization of @c std#hash class template for @c FlowRelativeFourSides.
	template<typename T>
	class hash<ascension::presentation::FlowRelativeFourSides<T>> :
		public std::function<std::hash<void*>::result_type(const ascension::presentation::FlowRelativeFourSides<T>&)> {
	public:
		std::size_t operator()(const ascension::presentation::FlowRelativeFourSides<T>& key) const BOOST_NOEXCEPT {
			return boost::hash<argument_type>()(key);
		}
	};
}

#endif // !ASCENSION_FLOW_RELATIVE_FOUR_SIDES_HPP
