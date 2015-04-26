/**
 * @file flow-relative-directions-dimensions.hpp
 * Defines flow-relative directional and dimensional terms.
 * @date 2012-03-31 created
 * @date 2012-2014 was directions.hpp
 * @date 2015-01-09 Separated from directions.hpp
 * @see direction.hpp, line-relative-directions-dimensions.hpp, physical-directions-dimensions.hpp, writing-mode.hpp
 */

#ifndef ASCENSION_FLOW_RELATIVE_DIRECTIONS_DIMENSIONS_HPP
#define ASCENSION_FLOW_RELATIVE_DIRECTIONS_DIMENSIONS_HPP
#include <ascension/corelib/basic-exceptions.hpp>	// UnknownValueException
#include <ascension/corelib/detail/decay-or-refer.hpp>
#include <ascension/corelib/future/scoped-enum-emulation.hpp>
#include <ascension/corelib/numeric-range.hpp>
#include <array>
#include <functional>	// std.hash
#ifndef ASCENSION_NO_XSL_FLOW_RELATIVE_DIRECTIONS
#	include <iterator>	// std.end
#endif
#include <type_traits>	// std.extent
#include <boost/functional/hash.hpp>
#include <boost/operators.hpp>
#include <boost/parameter.hpp>

namespace ascension {
	namespace presentation {
		/// @defgroup flow_relative_directions_dimensions Flow-relative Directions and Dimensions
		/// @see CSS Writing Modes Module Level 3, 6.1 Abstract Dimensions
		///      (http://www.w3.org/TR/css-writing-modes-3/#abstract-box)
		/// @see CSS Writing Modes Module Level 3, 6.2 Flow-relative Directions
		///      (http://www.w3.org/TR/css-writing-modes-3/#logical-directions)
		/// @{
		/**
		 * Defines the 'flow-relative directions' which are defined relative to the flow of content on the page.
		 * @see graphics#PhysicalDirection, graphics#font#LineRelativeDirection
		 */
		ASCENSION_SCOPED_ENUM_DECLARE_BEGIN(FlowRelativeDirection)
			/// 'block-start' means the side that comes earlier in the block progression.
			BLOCK_START,
			/// 'block-end' means the side opposite 'block-start'.
			BLOCK_END,
			/// 'inline-start' means the side from which text of the inline base direction would start.
			INLINE_START,
			/// 'inline-end' means the side opposite 'inline-start'.
			INLINE_END
#ifndef ASCENSION_NO_XSL_FLOW_RELATIVE_DIRECTIONS
			,
			/// 'before' -- Nominally the side that comes earlier in the block progression.
			BEFORE = BLOCK_START,
			/// 'after' -- The side opposite 'before'.
			AFTER = BLOCK_END,
			/// 'start' -- Nominally the side from which text of its inline base direction will start.
			START = INLINE_START,
			/// 'end' -- The side opposite 'start'.
			END = INLINE_END
#endif // !ASCENSION_NO_XSL_FLOW_RELATIVE_DIRECTIONS
		ASCENSION_SCOPED_ENUM_DECLARE_END(FlowRelativeDirection)

		/**
		 * Returns direction opposite @a direction.
		 * @throw UnknownValueException @a direction is invalid
		 */
		inline FlowRelativeDirection operator!(FlowRelativeDirection direction) {
			static const FlowRelativeDirection opposites[4] = {
				FlowRelativeDirection::BLOCK_END, FlowRelativeDirection::BLOCK_START,
				FlowRelativeDirection::INLINE_END, FlowRelativeDirection::INLINE_START
			};
			const std::size_t index = boost::underlying_cast<std::size_t>(direction);
			if(index >= std::extent<decltype(opposites)>::value)
				throw UnknownValueException("direction");
			return opposites[index];
		}

#ifndef ASCENSION_DOXYGEN_SHOULD_SKIP_THIS
		BOOST_PARAMETER_NAME(bpd)
		BOOST_PARAMETER_NAME(ipd)
#endif // !ASCENSION_DOXYGEN_SHOULD_SKIP_THIS

		/// Base type of @c FlowRelativeTwoAxes class template.
		template<typename T>
		class FlowRelativeTwoAxesBase : public std::array<T, 2> {
		public:
			/// Default constructor initializes nothing.
			FlowRelativeTwoAxesBase() {}
#ifdef BOOST_COMP_MSVC
			FlowRelativeTwoAxesBase(const FlowRelativeTwoAxesBase& other) : std::array<T, 2>(other) {}
#endif	// BOOST_COMP_MSVC
			/// Constructor takes named parameters as initial values
			template<typename Arguments>
			FlowRelativeTwoAxesBase(const Arguments& arguments) {
//				bpd() = arguments[_bpd | value_type()];
//				ipd() = arguments[_ipd | value_type()];
				bpd() = arguments[_bpd.operator|(value_type())];
				ipd() = arguments[_ipd.operator|(value_type())];
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
			/// Default constructor initializes nothing.
			FlowRelativeTwoAxes() {}
#ifdef BOOST_COMP_MSVC
			FlowRelativeTwoAxes(const FlowRelativeTwoAxes& other) : FlowRelativeTwoAxesBase<T>(static_cast<const FlowRelativeTwoAxesBase<T>&>(other)) {}
#endif	// BOOST_COMP_MSVC
			/// Constructor takes named parameters (default value is zero).
			BOOST_PARAMETER_CONSTRUCTOR(
				FlowRelativeTwoAxes, (FlowRelativeTwoAxesBase<T>), tag,
				(required
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

		/**
		 * Creates a @c FlowRelativeTwoAxes object, deducing the target type from the types of arguments.
		 * @tparam ArgumentPack The type of @a arguments
		 * @param arguments The named arguments same as the constructor of @c FlowRelativeTwoAxes class template
		 */
		template<typename ArgumentPack>
		inline auto makeFlowRelativeTwoAxes(const ArgumentPack& arguments)
				-> FlowRelativeTwoAxes<typename ascension::detail::DecayOrRefer<decltype(arguments[_bpd])>::Type> {
			typedef typename ascension::detail::DecayOrRefer<decltype(arguments[_bpd])>::Type Coordinate;
			static_assert(std::is_same<ascension::detail::DecayOrRefer<decltype(arguments[_ipd])>::Type, Coordinate>::value, "");
			return FlowRelativeTwoAxes<Coordinate>(_bpd = arguments[_bpd], _ipd = arguments[_ipd]);
		}

#ifndef ASCENSION_DOXYGEN_SHOULD_SKIP_THIS
		BOOST_PARAMETER_NAME(blockStart)
		BOOST_PARAMETER_NAME(blockEnd)
		BOOST_PARAMETER_NAME(inlineStart)
		BOOST_PARAMETER_NAME(inlineEnd)
#endif // !ASCENSION_DOXYGEN_SHOULD_SKIP_THIS

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
//				blockStart() = arguments[_blockStart | value_type()];
//				blockEnd() = arguments[_blockEnd | value_type()];
//				inlineStart() = arguments[_inlineStart | value_type()];
//				inlineEnd() = arguments[_inlineEnd | value_type()];
				blockStart() = arguments[_blockStart.operator|(value_type())];
				blockEnd() = arguments[_blockEnd.operator|(value_type())];
				inlineStart() = arguments[_inlineStart.operator|(value_type())];
				inlineEnd() = arguments[_inlineEnd.operator|(value_type())];
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
			/// Default constructor initializes nothing.
			FlowRelativeFourSides() {}
#ifdef BOOST_COMP_MSVC
			FlowRelativeFourSides(const FlowRelativeFourSides& other) : FlowRelativeFourSidesBase(static_cast<const FlowRelativeFourSidesBase<T>&>(other)) {}
			FlowRelativeFourSides(FlowRelativeFourSides&& other) : FlowRelativeFourSidesBase(std::forward<FlowRelativeFourSidesBase>(other)) {}
#endif	// BOOST_COMP_MSVC
			/// Constructor takes named parameters as initial values.
			BOOST_PARAMETER_CONSTRUCTOR(
				FlowRelativeFourSides, (FlowRelativeFourSidesBase<T>), tag,
				(required
					(blockStart, (value_type))
					(blockEnd, (value_type))
					(inlineStart, (value_type))
					(inlineEnd, (value_type))))
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
			return boost::hash_range(object);
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

#endif // !ASCENSION_FLOW_RELATIVE_DIRECTIONS_DIMENSIONS_HPP
