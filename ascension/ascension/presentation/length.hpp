/**
 * @file length.hpp
 * @author exeal
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2011 was presentation.hpp
 * @date 2011-05-04 separated from presentation.hpp
 * @date 2011-05-13 separated from text-style.hpp
 * @date 2011-2012
 */

#ifndef ASCENSION_LENGTH_HPP
#define ASCENSION_LENGTH_HPP
#include <ascension/graphics/geometry.hpp>	// graphics.NativeSize
#include <stdexcept>						// std.invalid_argument
#include <boost/operators.hpp>				// boost.equality_comparable

namespace ascension {
	namespace graphics {
		class RenderingContext2D;
	}
	namespace presentation {

		/**
		 * [Copied from CSS3] Lengthes refer to distance measurements.
		 * @see CSS Values and Units Module Level 3, 5. Distance Units: the Åe<length>Åf type
		 *      (http://www.w3.org/TR/2012/CR-css3-values-20120828/#lengths)
		 * @see CSS Values and Units Module Level 3, Percentages: the Åe<percentage>Åf type
		 *      (http://www.w3.org/TR/2012/CR-css3-values-20120828/#percentages)
		 * @see 4.2 Basic data types - SVG (Second Edition)
		 *      (http://www.w3.org/TR/SVG/types.html#DataTypeLength)
		 * @see 4.5.11 Interface SVGLength - SVG (Second Edition)
		 *      (http://www.w3.org/TR/SVG/types.html#InterfaceSVGLength)
		 */
		class Length : private boost::equality_comparable<Length> {
		public:
			typedef std::invalid_argument NotSupportedError;

			enum Unit {
				// relative length units
				/// [Copied from CSS3] Equal to the computed value of the Åefont-sizeÅf property of
				/// the element on which it is used.
				EM_HEIGHT,
				/// [Copied from CSS3] Equal to the font's x-height.
				X_HEIGHT,
				/// [Copied from CSS3] Equal to the advance measure of the "0" (ZERO, U+0030) glyph
				/// found in the font used to render it.
				/// @note The average character width is used by Ascension if not found.
				CHARACTERS,
				/// [Copied from CSS3] Equal to the computed value of Åefont-sizeÅf on the root
				/// element.
				/// @note Refers to the global primary font in Ascension.
				ROOT_EM_HEIGHT,
				/// [Copied from CSS3] Equal to 1% of the width of the initial containing block.
				VIEWPORT_WIDTH,
				/// [Copied from CSS3] Equal to 1% of the height of the initial containing block.
				VIEWPORT_HEIGHT,
				/// [Copied from CSS3] Equal to the smaller of ÅevwÅf or ÅevhÅf.
				VIEWPORT_MINIMUM,
				/// [Copied from CSS3] Equal to the larger of ÅevwÅf or ÅevhÅf.
				VIEWPORT_MAXIMUM,
//				GRIDS,			///< The grid.
				// absolute length units
				/// [Copied from CSS3] Centimeters.
				CENTIMETERS,
				/// [Copied from CSS3] Millimeters.
				MILLIMETERS,
				/// [Copied from CSS3] Inches; 1 in is equal to 2.54 cm.
				INCHES,
				/// [Copied from CSS3] Pixels; 1 px is equal to 1 / 96th of 1 in.
				/// @note <strong>Relative</strong> to the viewing device in Ascension.
				PIXELS,
				/// [Copied from CSS3] Points; 1 pt is equal to 1 / 72nd of 1 in.
				POINTS,
				/// [Copied from CSS3] Picas; 1 pc is equal to 12 pt.
				PICAS,
				// used in DirectWrite
				DEVICE_INDEPENDENT_PIXELS, ///< Device independent pixels; 1 DIP is equal to 1 / 96th of 1 in.
				// percentages (exactly not a length)
				PERCENTAGE,		///< Percentage.
#if 0
				// abbreviations
				EM = EM_HEIGHT,
				EX = X_HEIGHT,
				CH = CHARACTERS,
				REM = ROOT_EM_HEIGHT,
				VW = VIEWPORT_WIDTH,
				VH = VIEWPORT_HEIGHT,
				VMIN = VIEWPORT_MINIMUM,
				VMAX = VIEWPORT_MAXIMUM,
				CM = CENTIMETERS,
				MM = MILLIMETERS,
				IN = INCHES,
				PX = PIXELS,
				PT = POINTS,
				PC = PICAS,
				DIPS = DEVICE_INDEPENDENT_PIXELS
#endif
			};

			enum Mode {
				WIDTH, HEIGHT, OTHER
			};

		public:
			explicit Length(double valueInSpecifiedUnits = 0.0, Unit unitType = PIXELS, Mode = OTHER);
			bool operator==(const Length& other) const /*noexcept*/;
			void convertToSpecifiedUnits(Unit unitType,
				const graphics::RenderingContext2D* context, const graphics::NativeSize* contextSize);
			void newValueSpecifiedUnits(Unit unitType, double valueInSpecifiedUnits);
			void setValue(double value,
				const graphics::RenderingContext2D* context, const graphics::NativeSize* contextSize);
//			void setValueAsString(const StringPiece&);
			void setValueInSpecifiedUnits(double value) /*noexcept*/;
			Unit unitType() const /*noexcept*/;
			double value(const graphics::RenderingContext2D* context, const graphics::NativeSize* contextSize) const;
			double valueInSpecifiedUnits() const;
//			String valueAsString() const;

		private:
			double valueInSpecifiedUnits_;
			Unit unit_;
			Mode mode_;
			///
			static bool isAbsolute(Unit unit);
		};

		/// Equality operator returns @c true if and only if ....
		bool Length::operator==(const Length& other) const /*noexcept*/ {
			return valueInSpecifiedUnits() == other.valueInSpecifiedUnits()
				&& unitType() == other.unitType() && mode_ == other.mode_;
		}

		/**
		 * [Copied from SVG 1.1 documentation] Sets the value as a floating point value, in the
		 * units expressed by @c #unitType(). Setting this attribute will cause @c #value() and
		 * @c #valueAsString() to be updated automatically to reflect this setting.
		 * @param value The new value
		 * @see http://www.w3.org/TR/SVG11/types.html#__svg__SVGLength__valueInSpecifiedUnits
		 */
		inline void Length::setValueInSpecifiedUnits(double value) /*noexcept*/ {
			valueInSpecifiedUnits_ = value;
		}

		/**
		 * Returns the type of the value.
		 * @see http://www.w3.org/TR/SVG11/types.html#__svg__SVGLength__unitType
		 */
		inline Length::Unit Length::unitType() const /*noexcept*/ {
			return unit_;
		}

		/**
		 * Returns the value as a floating point value, in the units expressed by @c unitType().
		 * @see #setValueInSpecifiedUnits
		 * @see http://www.w3.org/TR/SVG11/types.html#__svg__SVGLength__valueInSpecifiedUnits
		 */
		inline double Length::valueInSpecifiedUnits() const /*noexcept*/ {
			return valueInSpecifiedUnits_;
		}

	}
} // namespace ascension.presentation

#endif // !ASCENSION_LENGTH_HPP
