/**
 * @file directions.hpp
 * Defines abstract and physical directional terms.
 * @date 2012-03-31 created
 * @date 2012-2014
 * @see geometry.hpp, writing-mode.hpp
 * @see CSS Writing Modes Module Level 3, 6. Abstract Box Terminology
 *      (http://www.w3.org/TR/css3-writing-modes/#abstract-box)
 */

#ifndef ASCENSION_DIRECTIONS_HPP
#define ASCENSION_DIRECTIONS_HPP
#include <ascension/corelib/basic-exceptions.hpp>	// UnknownValueException
#include <ascension/corelib/future/scoped-enum-emulation.hpp>
#include <ascension/graphics/geometry.hpp>
#include <array>
#include <iterator>		// std.end
#include <type_traits>	// std.extent
#include <boost/operators.hpp>
#include <boost/parameter.hpp>
#include <boost/range/irange.hpp>

namespace ascension {
	/**
	 * Represents direction in a text or a document (not visual orientation. See
	 * @c #presentation#ReadingDirection).
	 * @see ascension#text, ascension#searcher
	 */
	class Direction : private boost::equality_comparable<Direction> {
	public:
		static const Direction FORWARD;		///< Direction to the end.
		static const Direction BACKWARD;	///< Direction to the start.
		/// Copy-constructor.
		Direction(const Direction& other) BOOST_NOEXCEPT : value_(other.value_) {}
		/// Assignment operator.
		Direction& operator=(const Direction& other) BOOST_NOEXCEPT {
			return (value_ = other.value_), *this;
		}
		/// Negation operator returns the complement of this.
		Direction operator!() const BOOST_NOEXCEPT {
			return (*this == FORWARD) ? BACKWARD : FORWARD;
		}
		/// Equality operator.
		bool operator==(const Direction& other) const BOOST_NOEXCEPT {
			return value_ == other.value_;
		}
	private:
		explicit Direction(bool value) BOOST_NOEXCEPT : value_(value) {}
		bool value_;
	};

	namespace detail {
		template<typename T> struct DecayOrRefer {
			typedef typename std::decay<T>::type Type;
		};
		template<typename T> struct DecayOrRefer<std::reference_wrapper<T>> {
			typedef T& Type;
		};
	}

	namespace graphics {
		/// @defgroup physical_directions Physical Directions
		/// @{
		/**
		 * Defines physical directions.
		 * @see font#LineRelativeDirection, presentation#FlowRelativeDirection
		 */
		ASCENSION_SCOPED_ENUMS_BEGIN(PhysicalDirection)
			TOP,	///< Physical top.
			RIGHT,	///< Physical right.
			BOTTOM,	///< Physical bottom.
			LEFT	///< Physical left.
		ASCENSION_SCOPED_ENUMS_END

		/**
		 * Returns direction opposite @a direction.
		 * @throw UnknownValueException @a direction is invalid
		 */
		inline PhysicalDirection operator!(PhysicalDirection direction) {
			static const PhysicalDirection opposites[4] = {
				PhysicalDirection::BOTTOM, PhysicalDirection::LEFT,
				PhysicalDirection::TOP, PhysicalDirection::RIGHT
			};
			if(direction >= std::extent<decltype(opposites)>::value)
				throw UnknownValueException("direction");
			return opposites[direction];
		}

#ifndef ASCENSION_DOXYGEN_SHOULD_SKIP_THIS
		BOOST_PARAMETER_NAME(x)
		BOOST_PARAMETER_NAME(y)
#endif // !ASCENSION_DOXYGEN_SHOULD_SKIP_THIS

		/// Base type of @c PhysicalTwoAxes class template.
		template<typename T>
		class PhysicalTwoAxesBase : public std::array<T, 2> {
		public:
			/// Default constructor initializes nothing.
			PhysicalTwoAxesBase() {}
			/// Copy-constructor.
			PhysicalTwoAxesBase(const PhysicalTwoAxesBase<T>& other) : std::array<T, 2>(other) {}
			/// Constructor takes named parameters as initial values.
			template<typename Arguments>
			PhysicalTwoAxesBase(const Arguments& arguments) {
//				x() = arguments[_x | value_type()];
//				y() = arguments[_y | value_type()];
				x() = arguments[_x.operator|(value_type())];
				y() = arguments[_y.operator|(value_type())];
			}
			/// Returns a reference 'x' (horizontal position) value.
			value_type& x() BOOST_NOEXCEPT {return std::get<0>(*this);}
			/// Returns a reference 'x' (horizontal position) value.
			const value_type& x() const BOOST_NOEXCEPT {return std::get<0>(*this);}
			/// Returns a reference 'y' (vertical position) value.
			value_type& y() BOOST_NOEXCEPT {return std::get<1>(*this);}
			/// Returns a reference 'y' (vertical position) value.
			const value_type& y() BOOST_NOEXCEPT const {return std::get<1>(*this);}
		};

		/**
		 * A collection of all physical dimensions. This is a cartesian point.
		 * @tparam T The coordinate type
		 * @see presentation#AbstractTwoAxes
		 */
		template<typename T>
		class PhysicalTwoAxes : public PhysicalTwoAxesBase<T>, private boost::additive<PhysicalTwoAxes<T>> {
		public:
			/// Default constructor initializes nothing.
			PhysicalTwoAxes() {}
			/// Copy-constructor.
			PhysicalTwoAxes(const PhysicalTwoAxes<T>& other) : PhysicalTwoAxesBase<T>(static_cast<const PhysicalTwoAxesBase<T>&>(other)) {}
			/// Constructor takes a physical point.
			template<typename Point>
			PhysicalTwoAxes(const Point& point, typename geometry::detail::EnableIfTagIs<Point, boost::geometry::point_tag>::type* = nullptr) :
				PhysicalTwoAxesBase<T>((_x = geometry::x(point), _y = geometry::y(point))) {}
			/// Constructor takes named parameters as initial values (default value is zero).
			BOOST_PARAMETER_CONSTRUCTOR(
				PhysicalTwoAxes, (PhysicalTwoAxesBase<T>), tag,
				(required
					(x, (value_type))
					(y, (value_type))))
			/// Compound-add operator calls same operators of @c T for @c #x and @c #y.
			PhysicalTwoAxes& operator+=(const PhysicalTwoAxes<T>& other) {
				x() += other.x();
				y() += other.y();
				return *this;
			}
			/// Compound-subtract operator calls same operators of @c T for @c #x and @c #y.
			PhysicalTwoAxes& operator-=(const PhysicalTwoAxes<T>& other) {
				x() -= other.x();
				y() -= other.y();
				return *this;
			}
		};

		/**
		 * Creates a @c PhysicalTwoAxes object, deducing the target type from the types of arguments.
		 * @tparam ArgumentPack The type of @a arguments
		 * @param arguments The named arguments same as the constructor of @c PhysicalTwoAxes class
		 */
		template<typename ArgumentPack>
		inline auto makePhysicalTwoAxes(const ArgumentPack& arguments)
				-> PhysicalTwoAxes<typename ascension::detail::DecayOrRefer<decltype(arguments[_x])>::Type> {
			typedef typename ascension::detail::DecayOrRefer<decltype(arguments[_x])>::Type Coordinate;
			static_assert(std::is_same<ascension::detail::DecayOrRefer<decltype(arguments[_y])>::Type, Coordinate>::value, "");
			return PhysicalTwoAxes<Coordinate>(_x = arguments[_x], _y = arguments[_y]);
		}

#ifndef ASCENSION_DOXYGEN_SHOULD_SKIP_THIS
		BOOST_PARAMETER_NAME(top)
		BOOST_PARAMETER_NAME(right)
		BOOST_PARAMETER_NAME(bottom)
		BOOST_PARAMETER_NAME(left)
#endif // !ASCENSION_DOXYGEN_SHOULD_SKIP_THIS

		/// Base type of @c PhysicalFourSides class template.
		template<typename T>
		class PhysicalFourSidesBase : public std::array<T, 4> {
		public:
			/// Default constructor initializes nothing.
			PhysicalFourSidesBase() {}
			/// Constructor takes named parameters as initial values (default value is zero).
			template<typename Arguments>
			PhysicalFourSidesBase(const Arguments& arguments) {
//				top() = arguments[_top | value_type()];
//				right() = arguments[_right | value_type()];
//				bottom() = arguments[_bottom | value_type()];
//				left() = arguments[_left | value_type()];
				top() = arguments[_top.operator|(value_type())];
				right() = arguments[_right.operator|(value_type())];
				bottom() = arguments[_bottom.operator|(value_type())];
				left() = arguments[_left.operator|(value_type())];
			}
			/// Returns a reference to value of @a direction.
			reference operator[](PhysicalDirection direction) {
				return (*this)[static_cast<size_type>(direction)];
			}
			/// Returns a reference to value of @a direction.
			const_reference operator[](PhysicalDirection direction) const {
				return (*this)[static_cast<size_type>(direction)];
			}
			using std::array<T, 4>::operator[];
			/// Returns a reference to 'top' value.
			reference top() BOOST_NOEXCEPT {return std::get<PhysicalDirection::TOP>(*this);}
			/// Returns a reference to 'top' value.
			const_reference top() const BOOST_NOEXCEPT {return std::get<PhysicalDirection::TOP>(*this);}
			/// Returns a reference to 'right' value.
			reference right() BOOST_NOEXCEPT {return std::get<PhysicalDirection::RIGHT>(*this);}
			/// Returns a reference to 'right' value.
			const_reference right() const BOOST_NOEXCEPT {return std::get<PhysicalDirection::RIGHT>(*this);}
			/// Returns a reference to 'bottom' value.
			reference bottom() BOOST_NOEXCEPT {return std::get<PhysicalDirection::BOTTOM>(*this);}
			/// Returns a reference to 'bottom' value.
			const_reference bottom() const BOOST_NOEXCEPT {return std::get<PhysicalDirection::BOTTOM>(*this);}
			/// Returns a reference to 'left' value.
			reference left() BOOST_NOEXCEPT {return std::get<PhysicalDirection::LEFT>(*this);}
			/// Returns a reference to 'left' value.
			const_reference left() const BOOST_NOEXCEPT {return std::get<PhysicalDirection::LEFT>(*this);}
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
			/// Default constructor initializes nothing.
			PhysicalFourSides() {}
			/// Constructor takes a physical rectangle.
			template<typename Rectangle>
			PhysicalFourSides(const Rectangle& rectangle) {
				top() = geometry::top(rectangle);
				right() = geometry::right(rectangle);
				bottom() = geometry::bottom(rectangle);
				left() = geometry::left(rectangle);
			}
			/// Constructor takes named parameters as initial values (default value is zero).
			BOOST_PARAMETER_CONSTRUCTOR(
				PhysicalFourSides, (PhysicalFourSidesBase<T>), tag,
				(required
					(top, (value_type))
					(right, (value_type))
					(bottom, (value_type))
					(left, (value_type))))
			/// Compound-add operator calls same operators of @c T for the all elements.
			PhysicalFourSides& operator+=(const PhysicalTwoAxes<T>& other) {
				top() += other.y();
				right() += other.x();
				bottom() += other.y();
				left() += other.x();
				return *this;
			}
			/// Compound-subtract operator calls same operators of @c T for the all elements.
			PhysicalFourSides& operator-=(const PhysicalTwoAxes<T>& other) {
				top() -= other.y();
				right() -= other.x();
				bottom() -= other.y();
				left() -= other.x();
				return *this;
			}
		};

		/**
		 * Creates a @c PhysicalFourSides object, deducing the target type from the types of arguments.
		 * @tparam ArgumentPack The type of @a arguments
		 * @param arguments The named arguments same as the constructor of @c PhysicalFourSides class
		 */
		template<typename ArgumentPack>
		inline auto makePhysicalFourSides(const ArgumentPack& arguments)
				-> PhysicalFourSides<typename ascension::detail::DecayOrRefer<decltype(arguments[_top])>::Type> {
			typedef typename ascension::detail::DecayOrRefer<decltype(arguments[_top])>::Type Coordinate;
			static_assert(std::is_same<ascension::detail::DecayOrRefer<decltype(arguments[_right])>::Type, Coordinate>::value, "");
			static_assert(std::is_same<ascension::detail::DecayOrRefer<decltype(arguments[_bottom])>::Type, Coordinate>::value, "");
			static_assert(std::is_same<ascension::detail::DecayOrRefer<decltype(arguments[_left])>::Type, Coordinate>::value, "");
			return PhysicalFourSides<Coordinate>(_top = arguments[_top], _right = arguments[_right], _bottom = arguments[_bottom], _left = arguments[_left]);
		}

		/**
		 * Returns a range in horizontal direction of the given physical four sides.
		 * @tparam T Element type
		 * @param sides The physical four sides
		 * @return A range
		 * @see verticalRange, blockFlowRange, inlineFlowRange
		 */
		template<typename T>
		inline boost::integer_range<T> horizontalRange(const PhysicalFourSides<T>& sides) {
			static_assert(std::is_arithmetic<T>::value, "T is not arithmetic.");
			return boost::irange(sides.left(), sides.right());
		}

		/**
		 * Returns a range in vertical direction of the given physical four sides.
		 * @tparam T Element type
		 * @param sides The physical four sides
		 * @return A range
		 * @see horizontalRange, blockFlowRange, inlineFlowRange
		 */
		template<typename T>
		inline boost::integer_range<T> verticalRange(const PhysicalFourSides<T>& sides) {
			static_assert(std::is_arithmetic<T>::value, "T is not arithmetic.");
			return boost::irange(sides.top(), sides.bottom());
		}
		/// @}

		namespace font {
			/// @defgroup line_relative_direction Line-relative Directions
			/// @see CSS Writing Modes Module Level 3, 6.3. Line-relative Directions
			///      (http://www.w3.org/TR/css3-writing-modes/#line-directions)
			/// @{
			/**
			 * Defines line-relative directions.
			 * @see PhysicalDirection, presentation#FlowRelativeDirection
			 */
			ASCENSION_SCOPED_ENUMS_BEGIN(LineRelativeDirection)
				/// 'over' -- Nominally the side that corresponds to the ascender side or gtoph
				/// side of a line box.
				OVER,
				/// 'under' -- Opposite of over: the line-relative gbottomh or descender side.
				UNDER,
				/// 'line-left' -- Nominally the side from which LTR text would start.
				LINE_LEFT,
				/// 'line-right' -- Nominally the side from which RTL text would start.
				LINE_RIGHT
			ASCENSION_SCOPED_ENUMS_END

			/**
			 * Returns direction opposite @a direction.
			 * @throw UnknownValueException @a direction is invalid
			 */
			inline LineRelativeDirection operator!(LineRelativeDirection direction) {
				static const LineRelativeDirection opposites[4] = {
					LineRelativeDirection::UNDER, LineRelativeDirection::OVER,
					LineRelativeDirection::LINE_RIGHT, LineRelativeDirection::LINE_LEFT
				};
				if(direction >= std::extent<decltype(opposites)>::value)
					throw UnknownValueException("direction");
				return opposites[direction];
			}

#ifndef ASCENSION_DOXYGEN_SHOULD_SKIP_THIS
			BOOST_PARAMETER_NAME(over)
			BOOST_PARAMETER_NAME(under)
			BOOST_PARAMETER_NAME(lineLeft)
			BOOST_PARAMETER_NAME(lineRight)
#endif // !ASCENSION_DOXYGEN_SHOULD_SKIP_THIS

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
			/// @}
		}
	}

	namespace presentation {
		/// @defgroup flow_relative_directions Flow-relative Directions
		/// @see CSS Writing Modes Module Level 3, 6.2. Flow-relative Directions
		///      (http://www.w3.org/TR/css3-writing-modes/#logical-directions)
		/// @{
		/**
		 * Defines flow-relative directions.
		 * @see graphics#PhysicalDirection, graphics#font#LineRelativeDirection
		 */
		ASCENSION_SCOPED_ENUMS_BEGIN(FlowRelativeDirection)
			/// 'before' -- Nominally the side that comes earlier in the block progression.
			BEFORE,
			/// 'after' -- The side opposite 'before'.
			AFTER,
			/// 'start' -- Nominally the side from which text of its inline base direction will start.
			START,
			/// 'end' -- The side opposite 'start'.
			END
		ASCENSION_SCOPED_ENUMS_END

		/**
		 * Returns direction opposite @a direction.
		 * @throw UnknownValueException @a direction is invalid
		 */
		inline FlowRelativeDirection operator!(FlowRelativeDirection direction) {
			static const FlowRelativeDirection opposites[4] = {
				FlowRelativeDirection::AFTER, FlowRelativeDirection::BEFORE,
				FlowRelativeDirection::END, FlowRelativeDirection::START
			};
			if(direction >= std::extent<decltype(opposites)>::value)
				throw UnknownValueException("direction");
			return opposites[direction];
		}

#ifndef ASCENSION_DOXYGEN_SHOULD_SKIP_THIS
		BOOST_PARAMETER_NAME(bpd)
		BOOST_PARAMETER_NAME(ipd)
#endif // !ASCENSION_DOXYGEN_SHOULD_SKIP_THIS

		/// Base type of @c AbstractTwoAxes class template.
		template<typename T>
		class AbstractTwoAxesBase : public std::array<T, 2> {
		public:
			/// Default constructor initializes nothing.
			AbstractTwoAxesBase() {}
#ifdef BOOST_COMP_MSVC
			AbstractTwoAxesBase(const AbstractTwoAxesBase& other) : std::array<T, 2>(other) {}
#endif	// BOOST_COMP_MSVC
			/// Constructor takes named parameters as initial values
			template<typename Arguments>
			AbstractTwoAxesBase(const Arguments& arguments) {
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
		 * @see graphics#PhysicalTwoAxes
		 */
		template<typename T>
		class AbstractTwoAxes : public AbstractTwoAxesBase<T>, private boost::additive<AbstractTwoAxes<T>> {
		public:
			/// Default constructor initializes nothing.
			AbstractTwoAxes() {}
#ifdef BOOST_COMP_MSVC
			AbstractTwoAxes(const AbstractTwoAxes& other) : AbstractTwoAxesBase<T>(static_cast<const AbstractTwoAxesBase<T>&>(other)) {}
#endif	// BOOST_COMP_MSVC
			/// Constructor takes named parameters (default value is zero).
			BOOST_PARAMETER_CONSTRUCTOR(
				AbstractTwoAxes, (AbstractTwoAxesBase<T>), tag,
				(required
					(bpd, (value_type))
					(ipd, (value_type))))
			/// Compound-add operator calls same operators of @c T for @c #bpd() and @c #ipd().
			AbstractTwoAxes& operator+=(const AbstractTwoAxes& other) {
				bpd() += other.bpd();
				ipd() += other.ipd();
				return *this;
			}
			/// Compound-subtract operator calls same operators of @c T for @c #bpd() and @c #ipd().
			AbstractTwoAxes& operator-=(const AbstractTwoAxes& other) {
				bpd() -= other.bpd();
				ipd() -= other.ipd();
				return *this;
			}
		};

		/**
		 * Creates a @c AbstractTwoAxes object, deducing the target type from the types of arguments.
		 * @tparam ArgumentPack The type of @a arguments
		 * @param arguments The named arguments same as the constructor of @c AbstractTwoAxes class
		 */
		template<typename ArgumentPack>
		inline auto makeAbstractTwoAxes(const ArgumentPack& arguments)
				-> AbstractTwoAxes<typename ascension::detail::DecayOrRefer<decltype(arguments[_bpd])>::Type> {
			typedef typename ascension::detail::DecayOrRefer<decltype(arguments[_bpd])>::Type Coordinate;
			static_assert(std::is_same<ascension::detail::DecayOrRefer<decltype(arguments[_ipd])>::Type, Coordinate>::value, "");
			return AbstractTwoAxes<Coordinate>(_bpd = arguments[_bpd], _ipd = arguments[_ipd]);
		}

#ifndef ASCENSION_DOXYGEN_SHOULD_SKIP_THIS
		BOOST_PARAMETER_NAME(before)
		BOOST_PARAMETER_NAME(after)
		BOOST_PARAMETER_NAME(start)
		BOOST_PARAMETER_NAME(end)
#endif // !ASCENSION_DOXYGEN_SHOULD_SKIP_THIS

		/// Base type of @c FlowRelativeFourSides class template.
		template<typename T>
		class FlowRelativeFourSidesBase : public std::array<T, 4> {
		public:
			/// Default constructor initializes nothing.
			FlowRelativeFourSidesBase() {}
			/// Constructor takes named parameters as initial values.
			template<typename Arguments>
			FlowRelativeFourSidesBase(const Arguments& arguments) {
//				before() = arguments[_before | value_type()];
//				after() = arguments[_after | value_type()];
//				start() = arguments[_start | value_type()];
//				end() = arguments[_end | value_type()];
				before() = arguments[_before.operator|(value_type())];
				after() = arguments[_after.operator|(value_type())];
				start() = arguments[_start.operator|(value_type())];
				end() = arguments[_end.operator|(value_type())];
			}
			/// Returns a reference to value of @a direction.
			reference operator[](FlowRelativeDirection direction) {
				return (*this)[static_cast<size_type>(direction)];
			}
			/// Returns a reference to value of @a direction.
			const_reference operator[](FlowRelativeDirection direction) const {
				return (*this)[static_cast<size_type>(direction)];
			}
			using std::array<T, 4>::operator[];
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
		};

		/**
		 * A collection of all flow-relative directions.
		 * @tparam T The element type
		 * @see graphics#PhysicalFourSides, graphics#font#LineRelativeFourSides
		 */
		template<typename T>
		class FlowRelativeFourSides : public FlowRelativeFourSidesBase<T>,
			private boost::additive<FlowRelativeFourSides<T>, AbstractTwoAxes<T>> {
		public:
			/// Default constructor initializes nothing.
			FlowRelativeFourSides() {}
			FlowRelativeFourSides(const FlowRelativeFourSides&);
			FlowRelativeFourSides(FlowRelativeFourSides&&);
			/// Constructor takes named parameters as initial values.
			BOOST_PARAMETER_CONSTRUCTOR(
				FlowRelativeFourSides, (FlowRelativeFourSidesBase<T>), tag,
				(required
					(before, (value_type))
					(after, (value_type))
					(start, (value_type))
					(end, (value_type))))
			/// Compound-add operator calls same operators of @c T for the all elements.
			FlowRelativeFourSides& operator+=(const AbstractTwoAxes<T>& other) {
				before() += other.bpd();
				after() += other.bpd();
				start() += other.ipd();
				end() += other.ipd();
				return *this;
			}
			/// Compound-subtract operator calls same operators of @c T for the all elements.
			FlowRelativeFourSides& operator-=(const AbstractTwoAxes<T>& other) {
				before() -= other.bpd();
				after() -= other.bpd();
				start() -= other.ipd();
				end() -= other.ipd();
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
				-> FlowRelativeFourSides<typename ascension::detail::DecayOrRefer<decltype(arguments[_before])>::Type> {
			typedef typename ascension::detail::DecayOrRefer<decltype(arguments[_before])>::Type Coordinate;
			static_assert(std::is_same<ascension::detail::DecayOrRefer<decltype(arguments[_after])>::Type, Coordinate>::value, "");
			static_assert(std::is_same<ascension::detail::DecayOrRefer<decltype(arguments[_start])>::Type, Coordinate>::value, "");
			static_assert(std::is_same<ascension::detail::DecayOrRefer<decltype(arguments[_end])>::Type, Coordinate>::value, "");
			return FlowRelativeFourSides<Coordinate>(_before = arguments[_before], _after = arguments[_after], _start = arguments[_start], _end = arguments[_end]);
		}

		/**
		 * Returns a range in block flow direction of the given abstract four sides.
		 * @tparam T Element type
		 * @param sides The abstract four sides
		 * @return A range
		 * @see inlineFlowRange, horizontalRange, verticalRange
		 */
		template<typename T>
		inline boost::integer_range<T> blockFlowRange(const FlowRelativeFourSides<T>& sides) {
			static_assert(std::is_arithmetic<T>::value, "T is not arithmetic.");
			return boost::irange(sides.before(), sides.after());
		}

		/**
		 * Returns a range in inline flow direction of the given abstract four sides.
		 * @tparam T Element type
		 * @param sides The abstract four sides
		 * @return A range
		 * @see blockFlowRange, horizontalRange, verticalRange
		 */
		template<typename T>
		inline boost::integer_range<T> inlineFlowRange(const FlowRelativeFourSides<T>& sides) {
			static_assert(std::is_arithmetic<T>::value, "T is not arithmetic.");
			return boost::irange(sides.start(), sides.end());
		}
		/// @}
	}

	/// @name Free Functions to Convert Between Geometries and Abstract/Flow-Relative Instances
	/// @{

	namespace graphics {
		namespace geometry {
			/**
			 * Converts a @c PhysicalTwoAxes into a point.
			 * @tparam Geometry Type of return value
			 * @tparam Coordinate Type of coordinate of @a axes
			 * @param axes A @c PhysicalTwoAxes object
			 * @return A converted point
			 */
			template<typename Geometry, typename Coordinate>
			inline Geometry make(const PhysicalTwoAxes<Coordinate>& axes,
					typename detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr) {
				return boost::geometry::make<Geometry>(axes.x(), axes.y());
			}
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

	/// @}
}

// specialize std.end for presentation.FlowRelativeFourSides.end duplication
namespace std {
	template<typename T>
	inline typename ascension::presentation::FlowRelativeFourSides<T>::iterator end(ascension::presentation::FlowRelativeFourSides<T>& v) {
		return std::end(static_cast<std::array<T, 4>&>(v));
	}
	template<typename T>
	inline typename ascension::presentation::FlowRelativeFourSides<T>::const_iterator end(const ascension::presentation::FlowRelativeFourSides<T>& v) {
		return std::end(static_cast<const std::array<T, 4>&>(v));
	}
}

namespace boost {
	namespace geometry {
		namespace traits {
			template<typename T>
			struct tag<ascension::graphics::PhysicalTwoAxes<T>> {
				typedef point_tag type;
			};
			template<typename T>
			struct dimension<ascension::graphics::PhysicalTwoAxes<T>> : boost::mpl::int_<2> {
			};
			template<typename T>
			struct coordinate_type<ascension::graphics::PhysicalTwoAxes<T>> {
				typedef T type;
			};
			template<typename T>
			struct coordinate_system<ascension::graphics::PhysicalTwoAxes<T>> {
				typedef cs::cartesian type;
			};
			template<typename T>
			struct access<ascension::graphics::PhysicalTwoAxes<T>, 0> {
				static T get(const ascension::graphics::PhysicalTwoAxes<T>& p) {
					return p.x();
				}
				static void set(ascension::graphics::PhysicalTwoAxes<T>& p, const T& value) {
					p.x() = value;
				}
			};
			template<typename T>
			struct access<ascension::graphics::PhysicalTwoAxes<T>, 1> {
				static T get(const ascension::graphics::PhysicalTwoAxes<T>& p) {
					return p.y();
				}
				static void set(ascension::graphics::PhysicalTwoAxes<T>& p, const T& value) {
					p.y() = value;
				}
			};
		}
	}
}

#endif // !ASCENSION_DIRECTIONS_HPP
