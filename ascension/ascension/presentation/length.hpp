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
#include <stdexcept>	// std.invalid_argument
#include <boost/operators.hpp>

namespace ascension {

	namespace graphics {class RenderingContext2D;}

	namespace presentation {

		/**
		 *
		 * @see "CSS3 Values and Units 3.4 Numbers and units identifiers"
		 *      (http://www.w3.org/TR/2006/WD-css3-values-20060919/#numbers0)
		 * @see "4.2 Basic data types - SVG (Second Edition)"
		 *      (http://www.w3.org/TR/SVG/types.html#DataTypeLength)
		 * @see "4.5.11 Interface SVGLength - SVG (Second Edition)"
		 *      (http://www.w3.org/TR/SVG/types.html#InterfaceSVGLength)
		 */
		class Length : private boost::equality_comparable<Length> {
		public:
			typedef std::invalid_argument NotSupportedError;

			enum Unit {
				// relative length units
				EM_HEIGHT,		///< The font size of the relevant font.
				X_HEIGHT,		///< The x-height of the relevant font.
				PIXELS,			///< Pixels, relative to the viewing device.
				// relative length units introduced by CSS 3
				GRIDS,				///< The grid.
				REMS,				///< The font size of the primary font.
				VIEWPORT_WIDTH,		///< The viewport's width.
				VIEWPORT_HEIGHT,	///< The viewport's height.
				VIEWPORT_MINIMUM,	///< The viewport's height or width, whichever is smaller of the two.
				/**
				 * The width of the "0" (ZERO, U+0030) glyph found in the font for the font size
				 * used to render. If the "0" glyph is not found in the font, the average character
				 * width may be used.
				 */
				CHARACTERS,
				// absolute length units
				INCHES,			///< Inches -- 1 inch is equal to 2.54 centimeters.
				CENTIMETERS,	///< Centimeters.
				MILLIMETERS,	///< Millimeters.
				POINTS,			///< Points -- the point used by CSS 2.1 are equal to 1/72nd of an inch.
				PICAS,			///< Picas -- 1 pica is equal to 12 points.
				// used in DirectWrite
				DIPS,			///< Device independent pixels. 1 DIP is equal to 1/96th of an inch.
				// percentages (exactly not a length)
				PERCENTAGE,		///< Percentage.
			};

			enum Mode {
				WIDTH, HEIGHT, OTHER
			};

		public:
			explicit Length(double valueInSpecifiedUnits = 0.0, Unit unitType = PIXELS, Mode = OTHER);
			bool operator==(const Length& other) const /*throw()*/;
			void convertToSpecifiedUnits(Unit unitType,
				const graphics::RenderingContext2D* context, const graphics::NativeSize* contextSize);
			void newValueSpecifiedUnits(Unit unitType, double valueInSpecifiedUnits);
			void setValue(double value,
				const graphics::RenderingContext2D* context, const graphics::NativeSize* contextSize);
//			void setValueAsString(const StringPiece&);
			void setValueInSpecifiedUnits(double value) /*throw()*/;
			Unit unitType() const /*throw()*/;
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
		bool Length::operator==(const Length& other) const /*throw()*/ {
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
		inline void Length::setValueInSpecifiedUnits(double value) /*throw()*/ {valueInSpecifiedUnits_ = value;}

		/**
		 * Returns the type of the value.
		 * @see http://www.w3.org/TR/SVG11/types.html#__svg__SVGLength__unitType
		 */
		inline Length::Unit Length::unitType() const /*throw()*/ {return unit_;}

		/**
		 * Returns the value as a floating point value, in the units expressed by @c unitType().
		 * @see #setValueInSpecifiedUnits
		 * @see http://www.w3.org/TR/SVG11/types.html#__svg__SVGLength__valueInSpecifiedUnits
		 */
		inline double Length::valueInSpecifiedUnits() const /*throw()*/ {return valueInSpecifiedUnits_;}

	}
} // namespace ascension.presentation

#endif // !ASCENSION_LENGTH_HPP
