/**
 * @file physical-four-sides.hpp
 * Defines @c PhysicalFourSides class template and related free functions.
 * @date 2012-03-31 created
 * @date 2012-2014 was directions.hpp
 * @date 2015-01-09 Separated from directions.hpp
 * @date 2016-06-26 Separated from physical-directions-dimensions.hpp
 * @see flow-relative-four-sides.hpp, line-relative-four-sides.hpp, writing-mode.hpp
 */

#ifndef ASCENSION_PHYSICAL_FOUR_SIDES_HPP
#define ASCENSION_PHYSICAL_FOUR_SIDES_HPP
#include <ascension/corelib/detail/named-argument-exists.hpp>
#include <ascension/corelib/detail/named-arguments-single-type.hpp>
#include <ascension/corelib/numeric-range.hpp>
#include <ascension/graphics/geometry/rectangle-sides.hpp>
#include <ascension/graphics/physical-direction.hpp>
#include <ascension/graphics/physical-two-axes.hpp>
#include <boost/geometry/algorithms/assign.hpp>

namespace ascension {
	namespace graphics {
		/// @addtogroup physical_directions_dimensions
		/// @{
#ifndef ASCENSION_DETAIL_DOXYGEN_IS_PREPROCESSING
		BOOST_PARAMETER_NAME(top)
		BOOST_PARAMETER_NAME(right)
		BOOST_PARAMETER_NAME(bottom)
		BOOST_PARAMETER_NAME(left)
#endif // !ASCENSION_DETAIL_DOXYGEN_IS_PREPROCESSING

		/// Base type of @c PhysicalFourSides class template.
		template<typename T>
		class PhysicalFourSidesBase : public std::array<T, 4> {
		public:
			typedef std::array<T, 4> Super;	///< The base type.
		public:
			/// Default constructor initializes nothing.
			PhysicalFourSidesBase() {}
			/// Constructor takes named parameters as initial values.
			template<typename Arguments>
			PhysicalFourSidesBase(const Arguments& arguments) {
				if(const boost::optional<typename Super::value_type> v = arguments[_top | boost::none])
					top() = boost::get(v);
				if(const boost::optional<typename Super::value_type> v = arguments[_right | boost::none])
					right() = boost::get(v);
				if(const boost::optional<typename Super::value_type> v = arguments[_bottom | boost::none])
					bottom() = boost::get(v);
				if(const boost::optional<typename Super::value_type> v = arguments[_left | boost::none])
					left() = boost::get(v);
			}
			/// Returns a reference to value of @a direction.
			typename Super::reference operator[](PhysicalDirection direction) {
				return (*this)[boost::underlying_cast<typename Super::size_type>(direction)];
			}
			/// Returns a reference to value of @a direction.
			typename Super::const_reference operator[](PhysicalDirection direction) const {
				return (*this)[boost::underlying_cast<typename Super::size_type>(direction)];
			}
			using std::array<T, 4>::operator[];
			/// Returns a reference to 'top' value.
			typename Super::reference top() BOOST_NOEXCEPT {
				return std::get<static_cast<std::size_t>(PhysicalDirection::TOP)>(*this);
			}
			/// Returns a reference to 'top' value.
			typename Super::const_reference top() const BOOST_NOEXCEPT {
				return std::get<static_cast<std::size_t>(PhysicalDirection::TOP)>(*this);
			}
			/// Returns a reference to 'right' value.
			typename Super::reference right() BOOST_NOEXCEPT {
				return std::get<static_cast<std::size_t>(PhysicalDirection::RIGHT)>(*this);
			}
			/// Returns a reference to 'right' value.
			typename Super::const_reference right() const BOOST_NOEXCEPT {
				return std::get<static_cast<std::size_t>(PhysicalDirection::RIGHT)>(*this);
			}
			/// Returns a reference to 'bottom' value.
			typename Super::reference bottom() BOOST_NOEXCEPT {
				return std::get<static_cast<std::size_t>(PhysicalDirection::BOTTOM)>(*this);
			}
			/// Returns a reference to 'bottom' value.
			typename Super::const_reference bottom() const BOOST_NOEXCEPT {
				return std::get<static_cast<std::size_t>(PhysicalDirection::BOTTOM)>(*this);
			}
			/// Returns a reference to 'left' value.
			typename Super::reference left() BOOST_NOEXCEPT {
				return std::get<static_cast<std::size_t>(PhysicalDirection::LEFT)>(*this);
			}
			/// Returns a reference to 'left' value.
			typename Super::const_reference left() const BOOST_NOEXCEPT {
				return std::get<static_cast<std::size_t>(PhysicalDirection::LEFT)>(*this);
			}
		};

		/**
		 * A collection of all physical directions.
		 * @tparam T Element type
		 * @see font#LineRelativeFourSides, presentation#FlowRelativeFourSides
		 */
		template<typename T>
		class PhysicalFourSides : public PhysicalFourSidesBase<T>,
			private boost::additive<PhysicalFourSides<T>, PhysicalTwoAxes<T>> {
		public:
			/// Constructor takes a physical rectangle.
			template<typename Rectangle>
			PhysicalFourSides(const Rectangle& rectangle, typename std::enable_if<
					std::is_same<
						typename boost::geometry::tag<typename std::remove_cv<Rectangle>::type>::type,
						boost::geometry::box_tag
					>::value>::type* = nullptr) {
				this->top() = geometry::top(rectangle);
				this->right() = geometry::right(rectangle);
				this->bottom() = geometry::bottom(rectangle);
				this->left() = geometry::left(rectangle);
			}
			/**
			 * Creates a @c PhysicalFourSides with the given initial values by named parameters.
			 * Omitted elements are initialized by the default constructor.
			 * @param top The initial value of 'top' (optional)
			 * @param right The initial value of 'right' (optional)
			 * @param bottom The initial value of 'bottom' (optional)
			 * @param left The initial value of 'left' (optional)
			 */
			BOOST_PARAMETER_CONSTRUCTOR(
				PhysicalFourSides, (PhysicalFourSidesBase<T>), tag,
				(optional
					(top, (boost::optional<T>))
					(right, (boost::optional<T>))
					(bottom, (boost::optional<T>))
					(left, (boost::optional<T>))))
			/// Compound-add operator calls same operators of @c T for the all elements.
			PhysicalFourSides& operator+=(const PhysicalTwoAxes<T>& other) {
				this->top() += other.y();
				this->right() += other.x();
				this->bottom() += other.y();
				this->left() += other.x();
				return *this;
			}
			/// Compound-subtract operator calls same operators of @c T for the all elements.
			PhysicalFourSides& operator-=(const PhysicalTwoAxes<T>& other) {
				this->top() -= other.y();
				this->right() -= other.x();
				this->bottom() -= other.y();
				this->left() -= other.x();
				return *this;
			}
		};

		/**
		 * Creates a @c PhysicalFourSides object, deducing the target type from the types of arguments.
		 * @tparam ArgumentPack The type of @a arguments
		 * @param arguments The named arguments same as the constructor of @c PhysicalFourSides class
		 * @return A created @c PhysicalFourSides object
		 */
		template<typename Arguments>
		inline PhysicalFourSides<
			typename ascension::detail::NamedArgumentsSingleType<
				Arguments, tag::top, tag::right, tag::bottom, tag::left
			>::Type
		> makePhysicalFourSides(const Arguments& arguments) {
			typedef typename ascension::detail::NamedArgumentsSingleType<Arguments, tag::top, tag::right, tag::bottom, tag::left>::Type Coordinate;
			boost::optional<Coordinate> top, right, bottom, left;
			if(ascension::detail::NamedArgumentExists<Arguments, tag::top>::value)
				top = arguments[_top];
			if(ascension::detail::NamedArgumentExists<Arguments, tag::right>::value)
				right = arguments[_right];
			if(ascension::detail::NamedArgumentExists<Arguments, tag::bottom>::value)
				bottom = arguments[_bottom];
			if(ascension::detail::NamedArgumentExists<Arguments, tag::left>::value)
				left = arguments[_left];
			return PhysicalFourSides<Coordinate>(_top = top, _right = right, _bottom = bottom, _left = left);
		}

		/**
		 * Returns a range in horizontal direction of the given physical four sides.
		 * @tparam T An arithmetic type
		 * @param sides The physical four sides
		 * @return A range
		 * @see verticalRange, blockFlowRange, inlineFlowRange
		 */
		template<typename T>
		inline NumericRange<T> horizontalRange(const PhysicalFourSides<T>& sides) {
			static_assert(std::is_arithmetic<T>::value, "T is not arithmetic.");
			return nrange(sides.left(), sides.right());
		}

		/**
		 * Returns a range in vertical direction of the given physical four sides.
		 * @tparam T An arithmetic type
		 * @param sides The physical four sides
		 * @return A range
		 * @see horizontalRange, blockFlowRange, inlineFlowRange
		 */
		template<typename T>
		inline NumericRange<T> verticalRange(const PhysicalFourSides<T>& sides) {
			static_assert(std::is_arithmetic<T>::value, "T is not arithmetic.");
			return nrange(sides.top(), sides.bottom());
		}

		/**
		 * Returns 'width' of the given @c PhysicalFourSides.
		 * @tparam T An arithmetic type
		 * @param sides The physical four sides
		 * @return The 'width'
		 * @see extent, height, measure
		 */
		template<typename T>
		inline auto width(const PhysicalFourSides<T>& sides) -> decltype(boost::size(horizontalRange(sides))) {
			return boost::size(horizontalRange(sides));
		}

		/**
		 * Returns 'height' of the given @c PhysicalFourSides.
		 * @tparam T An arithmetic type
		 * @param sides The physical four sides
		 * @return The 'height'
		 * @see extent, measure, width
		 */
		template<typename T>
		inline auto height(const PhysicalFourSides<T>& sides) -> decltype(boost::size(verticalRange(sides))) {
			return boost::size(verticalRange(sides));
		}
		/// @}

		namespace geometry {
			/**
			 * Converts a @c PhysicalFourSides into a rectangle.
			 * @tparam Geometry Type of return value
			 * @tparam Coordinate Type of coordinate of @a sides
			 * @param sides A @c PhysicalFourSides object
			 * @return A converted rectangle
			 */
			template<typename Geometry, typename Coordinate>
			inline Geometry make(const PhysicalFourSides<Coordinate>& sides,
					typename detail::EnableIfTagIs<Geometry, boost::geometry::box_tag>::type* = nullptr) {
				Geometry result;
				boost::geometry::assign_values(result, sides.left(), sides.top(), sides.right(), sides.bottom());
				return result;
			}
		}
	}
}

#endif // !ASCENSION_PHYSICAL_FOUR_SIDES_HPP
