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
#include <type_traits>

namespace ascension {
	namespace presentation {
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

			/// Returns the value as a floating point.
			BOOST_CONSTEXPR Representation value() const BOOST_NOEXCEPT {return value_;}

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
