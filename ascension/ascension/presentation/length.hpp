/**
 * @file length.hpp
 * @author exeal
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2011 was presentation.hpp
 * @date 2011-05-04 separated from presentation.hpp
 * @date 2011-05-13 separated from text-style.hpp
 */

#ifndef ASCENSION_LENGTH_HPP
#define ASCENSION_LENGTH_HPP

namespace ascension {
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
		struct Length {
			double value;	///< Value of the length.
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
			Unit unit;	///< Unit of the length.

			enum Mode {
				WIDTH, HEIGHT, OTHER
			};
			Mode mode;	///< Mode of the length.

			/// Default constructor.
			Length() /*throw()*/ {}
			/// Constructor.
			explicit Length(double value, Unit unit = PIXELS, Mode = OTHER) : value(value), unit(unit), mode(mode) {}
			///
			static bool isAbsolute(Unit unit);
		};

	}
} // namespace ascension.presentation

#endif // !ASCENSION_LENGTH_HPP
