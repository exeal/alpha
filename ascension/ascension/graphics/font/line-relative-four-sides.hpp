/**
 * @file line-relative-four-sides.hpp
 * Defines line-relative directional and dimensional terms.
 * @date 2012-03-31 created
 * @date 2012-2014 was directions.hpp
 * @date 2015-01-09 Separated from directions.hpp
 * @date 2016-06-26 Separated from line-relative-directions-dimensions.hpp
 * @see flow-relative-four-sides.hpp, physical-four-sides.hpp, writing-mode.hpp
 */

#ifndef ASCENSION_LINE_RELATIVE_FOUR_SIDES_HPP
#define ASCENSION_LINE_RELATIVE_FOUR_SIDES_HPP
#include <ascension/corelib/detail/decay-or-refer.hpp>
#include <ascension/graphics/font/line-relative-direction.hpp>
#include <boost/parameter.hpp>
#include <array>

namespace ascension {
	namespace graphics {
		namespace font {
			/// @addtogroup line_relative_directions_dimensions
			/// @{
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
				reference over() BOOST_NOEXCEPT {
					return std::get<LineRelativeDirection::OVER>(*this);
				}
				/// Returns a reference to 'over' value.
				const_reference over() const BOOST_NOEXCEPT {
					return std::get<LineRelativeDirection::OVER>(*this);
				}
				/// Returns a reference to 'under' value.
				reference under() BOOST_NOEXCEPT {
					return std::get<LineRelativeDirection::UNDER>(*this);
				}
				/// Returns a reference to 'under' value.
				const_reference under() const BOOST_NOEXCEPT {
					return std::get<LineRelativeDirection::UNDER>(*this);
				}
				/// Returns a reference to 'line-left' value.
				reference lineLeft() BOOST_NOEXCEPT {
					return std::get<LineRelativeDirection::LINE_LEFT>(*this);
				}
				/// Returns a reference to 'line-left' value.
				const_reference lineLeft() const BOOST_NOEXCEPT {
					return std::get<LineRelativeDirection::LINE_LEFT>(*this);
				}
				/// Returns a reference to 'line-right' value.
				reference lineRight() BOOST_NOEXCEPT {
					return std::get<LineRelativeDirection::LINE_RIGHT>(*this);
				}
				/// Returns a reference to 'line-right' value.
				const_reference lineRight() const BOOST_NOEXCEPT {
					return std::get<LineRelativeDirection::LINE_RIGHT>(*this);
				}
				/// Returns a reference to 'line-over' value.
				reference lineOver() BOOST_NOEXCEPT {
					return std::get<LineRelativeDirection::LINE_OVER>(*this);
				}
				/// Returns a reference to 'line-over' value.
				const_reference lineOver() const BOOST_NOEXCEPT {
					return std::get<LineRelativeDirection::LINE_OVER>(*this);
				}
				/// Returns a reference to 'line-under' value.
				reference lineUnder() BOOST_NOEXCEPT {
					return std::get<LineRelativeDirection::LINE_UNDER>(*this);
				}
				/// Returns a reference to 'line-under' value.
				const_reference lineUnder() const BOOST_NOEXCEPT {
					return std::get<LineRelativeDirection::LINE_UNDER>(*this);
				}
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
				/**
				 * Constructor which takes named parameters as initial values.
				 * @param over The initial value of 'over'
				 * @param under The initial value of 'under'
				 * @param lineLeft The initial value of 'lineLeft'
				 * @param lineRight The initial value of 'lineRight'
				 */
#ifndef ASCENSION_DETAIL_DOXYGEN_IS_PREPROCESSING
				BOOST_PARAMETER_CONSTRUCTOR(
					LineRelativeFourSides, (LineRelativeFourSidesBase<T>), tag,
					(required
						(over, (value_type))
						(under, (value_type))
						(lineLeft, (value_type))
						(lineRight, (value_type))))
#else
				LineRelativeFourSides(value_type over, value_type under, value_type lineLeft, value_type lineRight);
#endif
			};

			/**
			 * Creates a @c LineRelativeFourSides object, deducing the target type from the types of arguments.
			 * @tparam Arguments The type of @a arguments
			 * @param arguments The named arguments same as the constructor of @c LineRelativeFourSides class
			 * @return A @c LineRelativeFourSides object
			 */
			template<typename Arguments>
			inline auto makeLineRelativeFourSides(const Arguments& arguments)
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

#endif // !ASCENSION_LINE_RELATIVE_FOUR_SIDES_HPP
