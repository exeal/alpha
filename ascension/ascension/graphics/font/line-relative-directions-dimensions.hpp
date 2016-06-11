/**
 * @file line-relative-directions-dimensions.hpp
 * Defines line-relative directional and dimensional terms.
 * @date 2012-03-31 created
 * @date 2012-2014 was directions.hpp
 * @date 2015-01-09 Separated from directions.hpp
 * @see direction.hpp, flow-relative-directions-dimensions.hpp, physical-directions-dimensions.hpp, writing-mode.hpp
 */

#ifndef ASCENSION_DIRECTIONS_HPP
#define ASCENSION_DIRECTIONS_HPP
#include <ascension/corelib/basic-exceptions.hpp>	// UnknownValueException
#include <ascension/corelib/detail/decay-or-refer.hpp>
#include <ascension/corelib/future/scoped-enum-emulation.hpp>
#include <boost/operators.hpp>
#include <boost/parameter.hpp>
//#include <boost/range/irange.hpp>
#include <array>
#include <type_traits>

namespace ascension {
	namespace graphics {
		namespace font {
			/// @defgroup line_relative_directions_dimensions Line-relative Directions and Dimensions
			/// @see CSS Writing Modes Module Level 3, 6.3 Line-relative Directions
			///      (http://www.w3.org/TR/css-writing-modes-3/#line-directions)
			/// @{
			/**
			 * Defines line-relative directions.
			 * @see PhysicalDirection, presentation#FlowRelativeDirection
			 */
			ASCENSION_SCOPED_ENUM_DECLARE_BEGIN(LineRelativeDirection)
				/// 'over' means nominally the side that corresponds to the ascender side or ÅgtopÅh side of a line box.
				OVER,
				/// 'under' means opposite of 'over': the line-relative ÅgbottomÅh or descender side.
				UNDER,
				/// 'line-left' means nominally the side from which LTR text would start.
				LINE_LEFT,
				/// 'line-right' means nominally the side from which RTL text would start.
				LINE_RIGHT,
				/// 'line-over' is an alias of 'over'.
				LINE_OVER = OVER,
				/// 'line-under' is an alias of 'under'.
				LINE_UNDER = UNDER
			ASCENSION_SCOPED_ENUM_DECLARE_END(LineRelativeDirection)

			/**
			 * Returns direction opposite @a direction.
			 * @throw UnknownValueException @a direction is invalid
			 */
			inline LineRelativeDirection operator!(LineRelativeDirection direction) {
				static const LineRelativeDirection opposites[4] = {
					LineRelativeDirection::UNDER, LineRelativeDirection::OVER,
					LineRelativeDirection::LINE_RIGHT, LineRelativeDirection::LINE_LEFT
				};
				const std::size_t index = boost::underlying_cast<std::size_t>(direction);
				if(index >= std::extent<decltype(opposites)>::value)
					throw UnknownValueException("direction");
				return opposites[index];
			}

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
				/// Constructor takes named parameters as initial values (default value is zero).
				BOOST_PARAMETER_CONSTRUCTOR(
					LineRelativePoint, (LineRelativePointBase<T>), tag,
					(required
						(u, (value_type))
						(v, (value_type))))
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

#ifndef ASCENSION_DETAIL_DOXYGEN_IS_PREPROCESSING
			BOOST_PARAMETER_NAME(over)
			BOOST_PARAMETER_NAME(under)
			BOOST_PARAMETER_NAME(lineLeft)
			BOOST_PARAMETER_NAME(lineRight)
#endif // !ASCENSION_DETAIL_DOXYGEN_IS_PREPROCESSING

			/// Base type of @c LineRelativeFourSides class template.
			template<typename T>
			class LineRelativeFourSidesBase : public std::array<T, 4> {
			public:
				/// Default constructor initializes nothing.
				LineRelativeFourSidesBase() {}
				/// Constructor takes named parameters as initial values.
				template<typename Arguments>
				LineRelativeFourSidesBase(const Arguments& arguments) {
//					over() = arguments[_over | value_type()];
//					under() = arguments[_under | value_type()];
//					lineLeft() = arguments[_lineLeft | value_type()];
//					lineRight() = arguments[_lineRight | value_type()];
					over() = arguments[_over.operator|(value_type())];
					under() = arguments[_under.operator|(value_type())];
					lineLeft() = arguments[_lineLeft.operator|(value_type())];
					lineRight() = arguments[_lineRight.operator|(value_type())];
				}
				/// Returns a reference to value of @a direction.
				reference operator[](LineRelativeDirection direction) {
					return (*this)[static_cast<size_type>(direction)];
				}
				/// Returns a reference to value of @a direction.
				const_reference operator[](LineRelativeDirection direction) const {
					return (*this)[static_cast<size_type>(direction)];
				}
				using std::array<T, 4>::operator[];
				/// Returns a reference to 'over' value.
				reference over() BOOST_NOEXCEPT {return std::get<LineRelativeDirection::OVER>(*this);}
				/// Returns a reference to 'over' value.
				const_reference over() const BOOST_NOEXCEPT {return std::get<LineRelativeDirection::OVER>(*this);}
				/// Returns a reference to 'under' value.
				reference under() BOOST_NOEXCEPT {return std::get<LineRelativeDirection::UNDER>(*this);}
				/// Returns a reference to 'under' value.
				const_reference under() const BOOST_NOEXCEPT {return std::get<LineRelativeDirection::UNDER>(*this);}
				/// Returns a reference to 'line-left' value.
				reference lineLeft() BOOST_NOEXCEPT {return std::get<LineRelativeDirection::LINE_LEFT>(*this);}
				/// Returns a reference to 'line-left' value.
				const_reference lineLeft() const BOOST_NOEXCEPT {return std::get<LineRelativeDirection::LINE_LEFT>(*this);}
				/// Returns a reference to 'line-right' value.
				/// @note This method hides @c std#array#end.
				reference lineRight() BOOST_NOEXCEPT {return std::get<LineRelativeDirection::LINE_RIGHT>(*this);}
				/// Returns a reference to 'line-right' value.
				/// @note This method hides @c std#array#end.
				const_reference lineRight() const BOOST_NOEXCEPT {return std::get<LineRelativeDirection::LINE_RIGHT>(*this);}
				/// Returns a reference to 'line-over' value.
				reference lineOver() BOOST_NOEXCEPT {return std::get<LineRelativeDirection::LINE_OVER>(*this);}
				/// Returns a reference to 'line-over' value.
				const_reference lineOver() const BOOST_NOEXCEPT {return std::get<LineRelativeDirection::LINE_OVER>(*this);}
				/// Returns a reference to 'line-under' value.
				reference lineUnder() BOOST_NOEXCEPT {return std::get<LineRelativeDirection::LINE_UNDER>(*this);}
				/// Returns a reference to 'line-under' value.
				const_reference lineUnder() const BOOST_NOEXCEPT {return std::get<LineRelativeDirection::LINE_UNDER>(*this);}
			};

			/**
			 * A collection of all line-relative directions.
			 * @tparam T The element type
			 * @see PhysicalFourSides, presentation#FlowRelativeFourSides
			 */
			template<typename T>
			class LineRelativeFourSides : public LineRelativeFourSidesBase<T> {
			public:
				/// Default constructor initializes nothing.
				LineRelativeFourSides() {}
				LineRelativeFourSides(const LineRelativeFourSides&);
				LineRelativeFourSides(LineRelativeFourSides&&);
				/// Constructor takes named parameters as initial values.
				BOOST_PARAMETER_CONSTRUCTOR(
					LineRelativeFourSides, (LineRelativeFourSidesBase<T>), tag,
					(required
						(over, (value_type))
						(under, (value_type))
						(lineLeft, (value_type))
						(lineRight, (value_type))))
			};

			/**
			 * Creates a @c LineRelativeFourSides object, deducing the target type from the types of arguments.
			 * @tparam ArgumentPack The type of @a arguments
			 * @param arguments The named arguments same as the constructor of @c LineRelativeFourSides class
			 */
			template<typename ArgumentPack>
			inline auto makeLineRelativeFourSides(const ArgumentPack& arguments)
					-> LineRelativeFourSides<typename ascension::detail::DecayOrRefer<decltype(arguments[_over])>::Type> {
				typedef typename ascension::detail::DecayOrRefer<decltype(arguments[_over])>::Type Coordinate;
				static_assert(std::is_same<ascension::detail::DecayOrRefer<decltype(arguments[_under])>::Type, Coordinate>::value, "");
				static_assert(std::is_same<ascension::detail::DecayOrRefer<decltype(arguments[_lineLeft])>::Type, Coordinate>::value, "");
				static_assert(std::is_same<ascension::detail::DecayOrRefer<decltype(arguments[_lineRight])>::Type, Coordinate>::value, "");
				return LineRelativeFourSides<Coordinate>(_over = arguments[_over], _under = arguments[_under], _lineLeft = arguments[_lineLeft], _lineRight = arguments[_lineRight]);
			}

			/**
			 * Returns 'extent' of the given @c LineRelativeFourSides.
			 * @tparam T An arithmetic type
			 * @param sides The line-relative four sides
			 * @return The 'extent'
			 * @see height, measure, width
			 */
			template<typename T>
			inline auto extent(const LineRelativeFourSides<T>& sides) -> decltype(sides.under() - sides.over()) {
				return sides.under() - sides.over();
			}

			/**
			 * Returns 'measure' of the given @c LineRelativeFourSides.
			 * @tparam T An arithmetic type
			 * @param sides The line-relative four sides
			 * @return The 'measure'
			 * @see extent, height, width
			 */
			template<typename T>
			inline auto measure(const LineRelativeFourSides<T>& sides) -> decltype(sides.lineRight() - sides.lineLeft()) {
				return sides.lineRight() - sides.lineLeft();
			}
			/// @}
		}
	}
}

#endif // !ASCENSION_DIRECTIONS_HPP
