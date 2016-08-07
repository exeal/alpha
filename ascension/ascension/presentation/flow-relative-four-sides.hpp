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
#include <ascension/corelib/detail/named-arguments-single-type.hpp>
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
			typedef std::array<T, 4> Super;	///< The base type.
		public:
			/// Default constructor initializes nothing.
			FlowRelativeFourSidesBase() {}
#if BOOST_COMP_MSVC
			FlowRelativeFourSidesBase(const FlowRelativeFourSidesBase& other) : std::array<T, 4>(other) {}
			FlowRelativeFourSidesBase(FlowRelativeFourSidesBase&& other) : std::array<T, 4>(other) {}
#endif	// BOOST_COMP_MSVC
			/// Constructor takes named parameters as initial values.
			template<typename Arguments>
			FlowRelativeFourSidesBase(const Arguments& arguments) {
				if(const boost::optional<typename Super::value_type> v = arguments[_blockStart | boost::none])
					blockStart() = boost::get(v);
				if(const boost::optional<typename Super::value_type> v = arguments[_blockEnd | boost::none])
					blockEnd() = boost::get(v);
				if(const boost::optional<typename Super::value_type> v = arguments[_inlineStart | boost::none])
					inlineStart() = boost::get(v);
				if(const boost::optional<typename Super::value_type> v = arguments[_inlineEnd | boost::none])
					inlineEnd() = boost::get(v);
#ifndef ASCENSION_NO_XSL_FLOW_RELATIVE_DIRECTIONS
#endif // !ASCENSION_NO_XSL_FLOW_RELATIVE_DIRECTIONS
			}
			/// Returns a reference to value of @a direction.
			typename Super::reference operator[](FlowRelativeDirection direction) {
				return (*this)[boost::underlying_cast<typename Super::size_type>(direction)];
			}
			/// Returns a reference to value of @a direction.
			typename Super::const_reference operator[](FlowRelativeDirection direction) const {
				return (*this)[boost::underlying_cast<typename Super::size_type>(direction)];
			}
			using std::array<T, 4>::operator[];
			/// Returns a reference to 'block-start' value.
			typename Super::reference blockStart() BOOST_NOEXCEPT {
				return std::get<static_cast<std::size_t>(FlowRelativeDirection::BLOCK_START)>(*this);
			}
			/// Returns a reference to 'block-start' value.
			typename Super::const_reference blockStart() const BOOST_NOEXCEPT {
				return std::get<static_cast<std::size_t>(FlowRelativeDirection::BLOCK_START)>(*this);
			}
			/// Returns a reference to 'block-end' value.
			typename Super::reference blockEnd() BOOST_NOEXCEPT {
				return std::get<static_cast<std::size_t>(FlowRelativeDirection::BLOCK_END)>(*this);
			}
			/// Returns a reference to 'block-end' value.
			typename Super::const_reference blockEnd() const BOOST_NOEXCEPT {
				return std::get<static_cast<std::size_t>(FlowRelativeDirection::BLOCK_END)>(*this);
			}
			/// Returns a reference to 'line-start' value.
			typename Super::reference inlineStart() BOOST_NOEXCEPT {
				return std::get<static_cast<std::size_t>(FlowRelativeDirection::INLINE_START)>(*this);
			}
			/// Returns a reference to 'line-start' value.
			typename Super::const_reference inlineStart() const BOOST_NOEXCEPT {
				return std::get<static_cast<std::size_t>(FlowRelativeDirection::INLINE_START)>(*this);
			}
			/// Returns a reference to 'line-end' value.
			typename Super::reference inlineEnd() BOOST_NOEXCEPT {
				return std::get<static_cast<std::size_t>(FlowRelativeDirection::INLINE_END)>(*this);
			}
			/// Returns a reference to 'line-end' value.
			typename Super::const_reference inlineEnd() const BOOST_NOEXCEPT {
				return std::get<static_cast<std::size_t>(FlowRelativeDirection::INLINE_END)>(*this);
			}
#ifndef ASCENSION_NO_XSL_FLOW_RELATIVE_DIRECTIONS
			/// Returns a reference to 'before' value.
			typename Super::reference before() BOOST_NOEXCEPT {
				return std::get<static_cast<std::size_t>(FlowRelativeDirection::BEFORE)>(*this);
			}
			/// Returns a reference to 'before' value.
			typename Super::const_reference before() const BOOST_NOEXCEPT {
				return std::get<static_cast<std::size_t>(FlowRelativeDirection::BEFORE)>(*this);
			}
			/// Returns a reference to 'after' value.
			typename Super::reference after() BOOST_NOEXCEPT {
				return std::get<static_cast<std::size_t>(FlowRelativeDirection::AFTER)>(*this);
			}
			/// Returns a reference to 'after' value.
			typename Super::const_reference after() const BOOST_NOEXCEPT {
				return std::get<static_cast<std::size_t>(FlowRelativeDirection::AFTER)>(*this);
			}
			/// Returns a reference to 'start' value.
			typename Super::reference start() BOOST_NOEXCEPT {
				return std::get<static_cast<std::size_t>(FlowRelativeDirection::START)>(*this);
			}
			/// Returns a reference to 'start' value.
			typename Super::const_reference start() const BOOST_NOEXCEPT {
				return std::get<static_cast<std::size_t>(FlowRelativeDirection::START)>(*this);
			}
			/// Returns a reference to 'end' value.
			/// @note This method hides @c std#array#end.
			typename Super::reference end() BOOST_NOEXCEPT {
				return std::get<static_cast<std::size_t>(FlowRelativeDirection::END)>(*this);
			}
			/// Returns a reference to 'end' value.
			/// @note This method hides @c std#array#end.
			typename Super::const_reference end() const BOOST_NOEXCEPT {
				return std::get<static_cast<std::size_t>(FlowRelativeDirection::END)>(*this);
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
			FlowRelativeFourSides(const FlowRelativeFourSides& other) : FlowRelativeFourSidesBase<T>(static_cast<const FlowRelativeFourSidesBase<T>&>(other)) {}
			FlowRelativeFourSides(FlowRelativeFourSides&& other) : FlowRelativeFourSidesBase<T>(std::forward<FlowRelativeFourSidesBase>(other)) {}
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
					(blockStart, (boost::optional<T>))
					(blockEnd, (boost::optional<T>))
					(inlineStart, (boost::optional<T>))
					(inlineEnd, (boost::optional<T>))
					(before, (boost::optional<T>))
					(after, (boost::optional<T>))
					(start, (boost::optional<T>))
					(end, (boost::optional<T>))))
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
					(blockStart, (boost::optional<T>))
					(blockEnd, (boost::optional<T>))
					(inlineStart, (boost::optional<T>))
					(inlineEnd, (boost::optional<T>))))
#endif // !ASCENSION_NO_XSL_FLOW_RELATIVE_DIRECTIONS
			template<typename U>
#if !BOOST_COMP_MSVC || BOOST_COMP_MSVC >= BOOST_VERSION_NUMBER(11, 0, 0)
			FlowRelativeFourSides(const U& other, typename std::enable_if<std::is_constructible<T, const U&>::value>::type* = nullptr)
#else
			FlowRelativeFourSides(const U& other, typename std::enable_if<std::is_convertible<U, T>::value>::type* = nullptr)
#endif
				: FlowRelativeFourSidesBase<T>((_blockStart = other, _blockEnd = other, _inlineStart = other, _inlineEnd = other)) {}
			/// Compound-add operator calls same operators of @c T for the all elements.
			FlowRelativeFourSides& operator+=(const FlowRelativeTwoAxes<T>& other) {
				this->blockStart() += other.bpd();
				this->blockEnd() += other.bpd();
				this->inlineStart() += other.ipd();
				this->inlineEnd() += other.ipd();
				return *this;
			}
			/// Compound-subtract operator calls same operators of @c T for the all elements.
			FlowRelativeFourSides& operator-=(const FlowRelativeTwoAxes<T>& other) {
				this->blockStart() -= other.bpd();
				this->blockEnd() -= other.bpd();
				this->inlineStart() -= other.ipd();
				this->inlineEnd() -= other.ipd();
				return *this;
			}
		};

		/**
		 * Creates a @c FlowRelativeFourSides object, deducing the target type from the types of arguments.
		 * @tparam Arguments The type of @a arguments
		 * @param arguments The named arguments same as the constructor of @c FlowRelativeFourSides class
		 * @return A created @c FlowRelativeFourSides object
		 */
		template<typename Arguments>
		inline FlowRelativeFourSides<
			typename ascension::detail::NamedArgumentsSingleType<
				Arguments, tag::blockStart, tag::blockEnd, tag::inlineStart, tag::inlineEnd
			>::Type
		> makeFlowRelativeFourSides(const Arguments& arguments) {
			typedef typename ascension::detail::NamedArgumentsSingleType<Arguments, tag::blockStart, tag::blockEnd, tag::inlineStart, tag::inlineEnd>::Type Coordinate;
			boost::optional<Coordinate> blockStart, blockEnd, inlineStart, inlineEnd;
			if(ascension::detail::NamedArgumentExists<Arguments, tag::blockStart>::value)
				blockStart = arguments[_blockStart];
			if(ascension::detail::NamedArgumentExists<Arguments, tag::blockEnd>::value)
				blockEnd = arguments[_blockEnd];
			if(ascension::detail::NamedArgumentExists<Arguments, tag::inlineStart>::value)
				inlineStart = arguments[_inlineStart];
			if(ascension::detail::NamedArgumentExists<Arguments, tag::inlineEnd>::value)
				inlineEnd = arguments[_inlineEnd];
			return FlowRelativeFourSides<Coordinate>(_blockStart = blockStart, _blockEnd = blockEnd, _inlineStart = inlineStart, _inlineEnd = inlineEnd);
		}

		/**
		 * Returns a range of the given @c FlowRelativeFourSides in block axis.
		 * @tparam T An arithmetic type
		 * @param sides The flow-relative four sides
		 * @return A range
		 * @see inlineFlowRange, horizontalRange, verticalRange
		 */
		template<typename T>
		inline NumericRange<T> blockFlowRange(const FlowRelativeFourSides<T>& sides) {
			static_assert(std::is_arithmetic<T>::value, "T is not arithmetic.");
			return nrange(sides.blockStart(), sides.blockEnd());
		}

		/**
		 * Returns a range of the given @c FlowRelativeFourSides in inline axis.
		 * @tparam T An arithmetic type
		 * @param sides The flow-relative four sides
		 * @return A range
		 * @see blockFlowRange, horizontalRange, verticalRange
		 */
		template<typename T>
		inline NumericRange<T> inlineFlowRange(const FlowRelativeFourSides<T>& sides) {
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
		inline auto extent(const FlowRelativeFourSides<T>& sides) -> decltype(boost::size(blockFlowRange(sides))) {
			return boost::size(blockFlowRange(sides));
		}

		/**
		 * Returns 'measure' of the given @c FlowRelativeFourSides.
		 * @tparam T An arithmetic type
		 * @param sides The flow-relative four sides
		 * @return The 'measure'
		 * @see extent, height, width
		 */
		template<typename T>
		inline auto measure(const FlowRelativeFourSides<T>& sides) -> decltype(boost::size(inlineFlowRange(sides))) {
			return boost::size(inlineFlowRange(sides));
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
			typedef std::function<
				std::hash<void*>::result_type(const ascension::presentation::FlowRelativeFourSides<T>&)
			> Super;
			return boost::hash<Super::argument_type>()(key);
		}
	};
}

#endif // !ASCENSION_FLOW_RELATIVE_FOUR_SIDES_HPP
