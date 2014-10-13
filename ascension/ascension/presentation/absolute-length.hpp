/**
 * @file absolute-length.hpp
 * Defines @c presentation#AbsoluteLength class.
 * @author exeal
 * @date 2014-09-30 Created.
 * @see length.hpp
 */

#ifndef ASCENSION_ABSOLUTE_LENGTH_HPP
#define ASCENSION_ABSOLUTE_LENGTH_HPP

#include <ascension/presentation/styles/numeric-data-types.hpp>
#include <boost/operators.hpp>
#define BOOST_RATIO_EXTENSIONS
#include <boost/ratio.hpp>
#include <limits>
#include <type_traits>

namespace ascension {
	namespace presentation {
		/**
		 * @c AbsoluteLength#max, @c AbsoluteLength#min and @c AbsoluteLength#zero methods call this specializable
		 * class template.
		 * @tparam Representation See the description of @c AbsoluteLength class template
		 * @note This design is based on @c std#chrono#duration_values class template.
		 */
		template<typename Representation>
		struct AbsoluteLengthValues {
			/// Returns the largest possible representation.
			static BOOST_CONSTEXPR Representation max() {return std::numeric_limits<Representation>::max();}
			/// Returns the smallest possible representation.
			static BOOST_CONSTEXPR Representation min() {return std::numeric_limits<Representation>::lowest();}
			/// Returns zero-length representation.
			static BOOST_CONSTEXPR Representation zero() {return Representation(0);}
		};

		/**
		 * @tparam RepresentationType The arithmetic type represents the number of CSS-pixels
		 * @tparam ScaleType @c boost#ratio or @c std#ratio which is the number of CSS-pixels per unit
		 * @see Length
		 */
		template<typename RepresentationType, typename ScaleType = boost::ratio<1>>
		class AbsoluteLength :
			private boost::additive<AbsoluteLength<RepresentationType, ScaleType>>,
			private boost::multiplicative<AbsoluteLength<RepresentationType, ScaleType>, RepresentationType> {
		public:
			/// The arithmetic type represents the number of CSS-pixels.
			typedef RepresentationType Representation;
			/// The number of CSS-pixels per unit.
			typedef ScaleType Scale;

			/// Default constructor which does not initialize the internal value.
			AbsoluteLength() {}

			/**
			 * @tparam OtherRepresentation The type of @a value
			 * @param value The length value in @c Scale
			 */
			template<typename OtherRepresentation>
			BOOST_CONSTEXPR explicit AbsoluteLength(const OtherRepresentation& value,
				typename std::enable_if<std::is_convertible<OtherRepresentation, Representation>::value>::type* = nullptr)
				: value_(static_cast<Representation>(value)) {}

			/**
			 * Copy-constructor.
			 * @tparam OtherRepresentation The template parameter for @a other
			 * @tparam OtherScale The template parameter for @a other
			 * @param other The source of copy
			 */
			template<typename OtherRepresentation, typename OtherScale>
			BOOST_CONSTEXPR AbsoluteLength(const AbsoluteLength<OtherRepresentation, OtherScale>& other) {
				typedef typename boost::ratio_divide<Scale, OtherScale>::type ScaleRatio;
				value_ = other.value() * ScaleRatio::den / ScaleRatio::num;
			}

			/// Unary @c + operator.
			BOOST_CONSTEXPR AbsoluteLength operator+() const {
				return *this;
			}

			/// Unary @c - operator.
			BOOST_CONSTEXPR AbsoluteLength operator-() const {
				return AbsoluteLength(-value());
			}

			/// Compound assignment @c += operator.
			AbsoluteLength& operator+=(const AbsoluteLength& other) {
				return (value_ += other.value()), *this;
			}

			/// Compound assignment @c -= operator.
			AbsoluteLength& operator-=(const AbsoluteLength& other) {
				return (value_ -= other.value()), *this;
			}

			/// Compound assignment @c *= operator.
			AbsoluteLength& operator*=(const Representation& other) {
				return (value_ *= other), *this;
			}

			/// Compound assignment @c /= operator.
			AbsoluteLength& operator/=(const Representation& other) {
				return (value_ /= other), *this;
			}

			/// Equality operator.
			template<typename OtherRepresentation, typename OtherScale>
			BOOST_CONSTEXPR bool operator==(const AbsoluteLength<OtherRepresentation, OtherScale>& other) const {
				typedef typename std::common_type<AbsoluteLength<Representation, Scale>, AbsoluteLength<OtherRepresentation, OtherScale>>::type CommonType;
				return CommonType(*this).value() == CommonType(other).value();
			}

			/// Inequality operator.
			template<typename OtherRepresentation, typename OtherScale>
			BOOST_CONSTEXPR bool operator!=(const AbsoluteLength<OtherRepresentation, OtherScale>& other) const {
				return !(*this == other);
			}

			/// Less-than operator.
			template<typename OtherRepresentation, typename OtherScale>
			BOOST_CONSTEXPR bool operator<(const AbsoluteLength<OtherScale>& other) const {
				typedef typename std::common_type<AbsoluteLength<Representation, Scale>, AbsoluteLength<OtherRepresentation, OtherScale>>::type CommonType;
				return CommonType(*this).value() < CommonType(other).value();
			}

			/// Less-than-or-equal-to operator.
			template<typename OtherRepresentation, typename OtherScale>
			BOOST_CONSTEXPR bool operator<=(const AbsoluteLength<OtherRepresentation, OtherScale>& other) const {
				return !(other >= *this);
			}

			/// Greater-than operator.
			template<typename OtherRepresentation, typename OtherScale>
			BOOST_CONSTEXPR bool operator>(const AbsoluteLength<OtherRepresentation, OtherScale>& other) const {
				return other < *this;
			}

			/// Greater-than-or-equal-to operator.
			template<typename OtherRepresentation, typename OtherScale>
			BOOST_CONSTEXPR bool operator>=(const AbsoluteLength<OtherRepresentation, OtherScale>& other) const {
				return other <= *this;
			}

			/// Returns a @c AbsoluteLength with the largest possible value.
			static BOOST_CONSTEXPR AbsoluteLength max() {return AbsoluteLength(AbsoluteLengthValues<Representation>::max());}

			/// Returns a @c AbsoluteLength with the smallest possible value.
			static BOOST_CONSTEXPR AbsoluteLength min() {return AbsoluteLength(AbsoluteLengthValues<Representation>::min());}

			/// Returns the value as a floating point.
			BOOST_CONSTEXPR Representation value() const {return value_;}

			/// Returns a @c AbsoluteLength with zero-length.
			static BOOST_CONSTEXPR AbsoluteLength zero() {return AbsoluteLength(AbsoluteLengthValues<Representation>::zero());}

		private:
			Representation value_;
		};

		typedef AbsoluteLength<styles::Number> Pixels;
		typedef AbsoluteLength<styles::Number, boost::ratio_multiply<Pixels::Scale, boost::ratio<96>>::type> Inches;	// 1 in = 96 px
		typedef AbsoluteLength<styles::Number, boost::ratio_divide<Inches::Scale, boost::ratio<254, 1000>>::type> Millimeters;	// 1 mm = 1/25.4 in
		typedef AbsoluteLength<styles::Number, boost::ratio_multiply<Millimeters::Scale, boost::ratio<10>>::type> Centimeters;	// 1 cm = 10 mm
		typedef AbsoluteLength<styles::Number, boost::ratio_divide<Inches::Scale, boost::ratio<72>>::type> Points;	// 1 pt = 1/72 in
		typedef AbsoluteLength<styles::Number, boost::ratio_multiply<Points::Scale, boost::ratio<12>>::type> Picas;	// 1 pc = 12 pt
		typedef AbsoluteLength<styles::Number, boost::ratio_divide<Inches::Scale, boost::ratio<96>>::type> DeviceIndependentPixels;	// 1 dip = 1/96 in
	}
}

namespace std {
	template<typename Representation, typename Scale1, typename Scale2>
	struct common_type<ascension::presentation::AbsoluteLength<Representation, Scale1>, ascension::presentation::AbsoluteLength<Representation, Scale2>> {
		typedef ascension::presentation::AbsoluteLength<Representation, typename boost::ratio_gcd<Scale1, Scale2>::type> type;
	};
}

#endif // !ASCENSION_ABSOLUTE_LENGTH_HPP
