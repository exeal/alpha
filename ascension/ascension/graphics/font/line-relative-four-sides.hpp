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
#include <ascension/corelib/detail/named-arguments-single-type.hpp>
#include <ascension/graphics/font/line-relative-direction.hpp>
#include <boost/parameter.hpp>
#include <array>

namespace ascension {
	namespace graphics {
		namespace font {
			/// @addtogroup line_relative_directions_dimensions
			/// @{
#ifndef ASCENSION_DETAIL_DOXYGEN_IS_PREPROCESSING
			BOOST_PARAMETER_NAME(lineOver)
			BOOST_PARAMETER_NAME(lineUnder)
			BOOST_PARAMETER_NAME(lineLeft)
			BOOST_PARAMETER_NAME(lineRight)
#endif // !ASCENSION_DETAIL_DOXYGEN_IS_PREPROCESSING

			/// Base type of @c LineRelativeFourSides class template.
			template<typename T>
			class LineRelativeFourSidesBase : public std::array<T, 4> {
			public:
				typedef std::array<T, 4> Super;	///< The base type.
			public:
				/// Default constructor initializes nothing.
				LineRelativeFourSidesBase() {}
				/// Constructor takes named parameters as initial values.
				template<typename Arguments>
				LineRelativeFourSidesBase(const Arguments& arguments) {
					if(const boost::optional<typename Super::value_type> v = arguments[_lineOver | boost::none])
						lineOver() = boost::get(v);
					if(const boost::optional<typename Super::value_type> v = arguments[_lineUnder | boost::none])
						lineUnder() = boost::get(v);
					if(const boost::optional<typename Super::value_type> v = arguments[_lineLeft | boost::none])
						lineLeft() = boost::get(v);
					if(const boost::optional<typename Super::value_type> v = arguments[_lineRight | boost::none])
						lineRight() = boost::get(v);
				}
				/// Returns a reference to value of @a direction.
				typename Super::reference operator[](LineRelativeDirection direction) {
					return (*this)[static_cast<typename Super::size_type>(direction)];
				}
				/// Returns a reference to value of @a direction.
				typename Super::const_reference operator[](LineRelativeDirection direction) const {
					return (*this)[static_cast<typename Super::size_type>(direction)];
				}
				using std::array<T, 4>::operator[];
				/// Returns a reference to 'line-left' value.
				typename Super::reference lineLeft() BOOST_NOEXCEPT {
					return std::get<static_cast<std::size_t>(LineRelativeDirection::LINE_LEFT)>(*this);
				}
				/// Returns a reference to 'line-left' value.
				typename Super::const_reference lineLeft() const BOOST_NOEXCEPT {
					return std::get<static_cast<std::size_t>(LineRelativeDirection::LINE_LEFT)>(*this);
				}
				/// Returns a reference to 'line-right' value.
				typename Super::reference lineRight() BOOST_NOEXCEPT {
					return std::get<static_cast<std::size_t>(LineRelativeDirection::LINE_RIGHT)>(*this);
				}
				/// Returns a reference to 'line-right' value.
				typename Super::const_reference lineRight() const BOOST_NOEXCEPT {
					return std::get<static_cast<std::size_t>(LineRelativeDirection::LINE_RIGHT)>(*this);
				}
				/// Returns a reference to 'line-over' value.
				typename Super::reference lineOver() BOOST_NOEXCEPT {
					return std::get<static_cast<std::size_t>(LineRelativeDirection::LINE_OVER)>(*this);
				}
				/// Returns a reference to 'line-over' value.
				typename Super::const_reference lineOver() const BOOST_NOEXCEPT {
					return std::get<static_cast<std::size_t>(LineRelativeDirection::LINE_OVER)>(*this);
				}
				/// Returns a reference to 'line-under' value.
				typename Super::reference lineUnder() BOOST_NOEXCEPT {
					return std::get<static_cast<std::size_t>(LineRelativeDirection::LINE_UNDER)>(*this);
				}
				/// Returns a reference to 'line-under' value.
				typename Super::const_reference lineUnder() const BOOST_NOEXCEPT {
					return std::get<static_cast<std::size_t>(LineRelativeDirection::LINE_UNDER)>(*this);
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
				 * @param lineOver The initial value of 'lineOver' (optional)
				 * @param lineUnder The initial value of 'lineUnder' (optional)
				 * @param lineLeft The initial value of 'lineLeft' (optional)
				 * @param lineRight The initial value of 'lineRight' (optional)
				 */
#ifndef ASCENSION_DETAIL_DOXYGEN_IS_PREPROCESSING
				BOOST_PARAMETER_CONSTRUCTOR(
					LineRelativeFourSides, (LineRelativeFourSidesBase<T>), tag,
					(optional
						(lineOver, (boost::optional<T>))
						(lineUnder, (boost::optional<T>))
						(lineLeft, (boost::optional<T>))
						(lineRight, (boost::optional<T>))))
#else
				LineRelativeFourSides(T lineOver, T lineUnder, T lineLeft, T lineRight);
#endif
			};

			/**
			 * Creates a @c LineRelativeFourSides object, deducing the target type from the types of arguments.
			 * @tparam Arguments The type of @a arguments
			 * @param arguments The named arguments same as the constructor of @c LineRelativeFourSides class
			 * @return A @c LineRelativeFourSides object
			 */
			template<typename Arguments>
			inline LineRelativeFourSides<
				typename ascension::detail::NamedArgumentsSingleType<
					Arguments, tag::lineOver, tag::lineUnder, tag::lineLeft, tag::lineRight
				>::Type
			> makeLineRelativeFourSides(const Arguments& arguments) {
				typedef typename ascension::detail::NamedArgumentsSingleType<Arguments, tag::lineOver, tag::lineUnder, tag::lineLeft, tag::lineRight>::Type Coordinate;
				boost::optional<Coordinate> over, under, left, right;
				if(ascension::detail::NamedArgumentExists<Arguments, tag::lineOver>::value)
					over = arguments[_lineOver];
				if(ascension::detail::NamedArgumentExists<Arguments, tag::lineUnder>::value)
					under = arguments[_lineUnder];
				if(ascension::detail::NamedArgumentExists<Arguments, tag::lineLeft>::value)
					left = arguments[_lineLeft];
				if(ascension::detail::NamedArgumentExists<Arguments, tag::lineRight>::value)
					right = arguments[_lineRight];
				return LineRelativeFourSides<Coordinate>(_lineOver = over, _lineUnder = under, _lineLeft = left, _lineRight = right);
			}

			/**
			 * Returns 'extent' of the given @c LineRelativeFourSides.
			 * @tparam T An arithmetic type
			 * @param sides The line-relative four sides
			 * @return The 'extent'
			 * @see height, measure, width
			 */
			template<typename T>
			inline auto extent(const LineRelativeFourSides<T>& sides) -> decltype(sides.lineUnder() - sides.lineOver()) {
				return sides.lineUnder() - sides.lineOver();
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
