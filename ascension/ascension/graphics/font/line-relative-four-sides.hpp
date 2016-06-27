/**
 * @file line-relative-four-sides.hpp
 * Defines @c LineRelativeFourSides class template and related free functions.
 * @date 2012-03-31 created
 * @date 2012-2014 was directions.hpp
 * @date 2015-01-09 Separated from directions.hpp
 * @date 2016-06-26 Separated from line-relative-directions-dimensions.hpp
 * @see flow-relative-four-sides.hpp, physical-four-sides.hpp, writing-mode.hpp
 */

#ifndef ASCENSION_LINE_RELATIVE_FOUR_SIDES_HPP
#define ASCENSION_LINE_RELATIVE_FOUR_SIDES_HPP
#include <ascension/corelib/detail/decay-or-refer.hpp>
#include <ascension/corelib/detail/named-argument-exists.hpp>
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
			BOOST_PARAMETER_NAME(lineOver)
			BOOST_PARAMETER_NAME(lineUnder)
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
					const bool shortKeyword =
						ascension::detail::NamedArgumentExists<Arguments, tag::over>::value
						|| ascension::detail::NamedArgumentExists<Arguments, tag::under>::value;
					const bool longKeyword =
						ascension::detail::NamedArgumentExists<Arguments, tag::lineOver>::value
						|| ascension::detail::NamedArgumentExists<Arguments, tag::lineUnder>::value;
					const int m = shortKeyword ? 1 : 0;
					const int n = m + (longKeyword ? 2 : 0);
					initializeOverAndUnder(arguments, boost::mpl::int_<(n != 0) ? n : 1>());
					if(ascension::detail::NamedArgumentExists<Arguments, tag::lineLeft>::value)
						lineLeft() = arguments[_lineLeft | value_type()];
					if(ascension::detail::NamedArgumentExists<Arguments, tag::lineRight>::value)
						lineRight() = arguments[_lineRight | value_type()];
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

			private:
				template<typename Arguments>
				void initializeOverAndUnder(const Arguments& arguments, boost::mpl::int_<1>) {
					if(ascension::detail::NamedArgumentExists<Arguments, tag::over>::value)
						over() = arguments[_over | value_type()];
					if(ascension::detail::NamedArgumentExists<Arguments, tag::under>::value)
						under() = arguments[_under | value_type()];
				}
				template<typename Arguments>
				void initializeOverAndUnder(const Arguments& arguments, boost::mpl::int_<2>) {
					if(ascension::detail::NamedArgumentExists<Arguments, tag::lineOver>::value)
						lineOver() = arguments[_lineOver | value_type()];
					if(ascension::detail::NamedArgumentExists<Arguments, tag::lineUnder>::value)
						lineUnder() = arguments[_lineUnder | value_type()];
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
				LineRelativeFourSides(const LineRelativeFourSides&);
				LineRelativeFourSides(LineRelativeFourSides&&);
				/**
				 * Constructor which takes named parameters as initial values.
				 * Omitted elements are initialized by the default constructor.
				 * @param over The initial value of 'over' (optional)
				 * @param under The initial value of 'under' (optional)
				 * @param lineOver The initial value of 'lineOver' (optional)
				 * @param lineUnder The initial value of 'lineUnder' (optional)
				 * @param lineLeft The initial value of 'lineLeft' (optional)
				 * @param lineRight The initial value of 'lineRight' (optional)
				 * @note You can't give both @a over and @a lineOver.
				 * @note You can't give both @a under and @a lineUnder.
				 */
#ifndef ASCENSION_DETAIL_DOXYGEN_IS_PREPROCESSING
				BOOST_PARAMETER_CONSTRUCTOR(
					LineRelativeFourSides, (LineRelativeFourSidesBase<T>), tag,
					(optional
						(over, (value_type))
						(under, (value_type))
						(lineOver, (value_type))
						(lineUnder, (value_type))
						(lineLeft, (value_type))
						(lineRight, (value_type))))
#else
				LineRelativeFourSides(value_type over, value_type under, value_type lineLeft, value_type lineRight);
#endif
			};

			namespace detail {
				template<typename Arguments>
				struct LineRelativeFourSidesFactoryResult {
					typedef typename boost::parameter::value_type<Arguments, tag::over, void>::type OverType;
					typedef typename boost::parameter::value_type<Arguments, tag::under, void>::type UnderType;
					typedef typename boost::parameter::value_type<Arguments, tag::lineOver, void>::type LineOverType;
					typedef typename boost::parameter::value_type<Arguments, tag::lineUnder, void>::type LineUnderType;
					typedef typename boost::parameter::value_type<Arguments, tag::lineLeft, void>::type LineLeftType;
					typedef typename boost::parameter::value_type<Arguments, tag::lineRight, void>::type LineRightType;
					typedef typename ascension::detail::DecayOrRefer<
						typename std::conditional<!std::is_same<OverType, void>::value, OverType,
							typename std::conditional<!std::is_same<UnderType, void>::value, UnderType,
								typename std::conditional<!std::is_same<LineOverType, void>::value, LineOverType,
									typename std::conditional<!std::is_same<LineUnderType, void>::value, LineUnderType,
										typename std::conditional<!std::is_same<LineLeftType, void>::value, LineLeftType,
											typename std::conditional<!std::is_same<LineRightType, void>::value, LineRightType, void>
										>
									>
								>
							>
						>::type
					>::Type Type;
					static_assert(!std::is_same<Type, void>::value, "");
				};
			}

			/**
			 * Creates a @c LineRelativeFourSides object, deducing the target type from the types of arguments.
			 * @tparam Arguments The type of @a arguments
			 * @param arguments The named arguments same as the constructor of @c LineRelativeFourSides class
			 * @return A @c LineRelativeFourSides object
			 */
			template<typename Arguments>
			inline LineRelativeFourSides<typename detail::LineRelativeFourSidesFactoryResult<Arguments>::Type> makeLineRelativeFourSides(const Arguments& arguments) {
				typedef typename detail::LineRelativeFourSidesFactoryResult<Arguments>::Type Coordinate;
				return LineRelativeFourSides<Coordinate>(arguments);
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
