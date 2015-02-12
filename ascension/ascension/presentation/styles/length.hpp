/**
 * @file length.hpp
 * @author exeal
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2011 was presentation.hpp
 * @date 2011-05-04 separated from presentation.hpp
 * @date 2011-05-13 separated from text-style.hpp
 * @date 2011-2014
 */

#ifndef ASCENSION_LENGTH_HPP
#define ASCENSION_LENGTH_HPP
#include <ascension/graphics/geometry/dimension.hpp>
#include <ascension/presentation/styles/numeric-data-types.hpp>
#include <stdexcept>			// std.invalid_argument
#include <boost/functional/hash/hash.hpp>
#include <boost/operators.hpp>	// boost.equality_comparable

namespace ascension {
	namespace graphics {
		class RenderingContext2D;
	}

	namespace presentation {
		namespace styles {
			/**
			 * [Copied from CSS3] Lengthes refer to distance measurements.
			 * @note The "Computed Value" of almost all style properties which have @c Length as "Specified Value" are
			 *       also @c Length types, because a @c Presentation instance is shared by multiple @c TextViewer s.
			 *       The computation into physical units is delayed until generation of concrete layouts.
			 * @see CSS Values and Units Module Level 3, 5. Distance Units: the ‘<length>’ type
			 *      (http://www.w3.org/TR/css3-values/#lengths)
			 * @see 4.2 Basic data types - SVG (Second Edition)
			 *      (http://www.w3.org/TR/SVG/types.html#DataTypeLength)
			 * @see 4.5.11 Interface SVGLength - SVG (Second Edition)
			 *      (http://www.w3.org/TR/SVG/types.html#InterfaceSVGLength)
			 */
			class Length : private boost::equality_comparable<Length> {
			public:
				typedef std::invalid_argument NotSupportedError;

				/**
				 * Units in this class.
				 * @note Values are not same as @c SVGLength interface in SVG 1.1.
				 */
				enum Unit {
					/**
					 * @name SVG 1.1
					 * @note The description of values are copied from SVG11.
					 * @{
					 */
					/// The unit type is not one of predefined unit types.
					/// @note Ascension does not support this value at all.
					UNKNOWN,
					/// No unit type was provided (i.e., a unitless value was specified), which indicates a value in
					/// user units.
					/// @note Ascension does not support this value at all. Use @c Number.
					NUMBER,
					/// @}

					/**
					 * @name Relative Length Units
					 * @note The description of values are copied from CSS3.
					 * @see CSS Values and Units Module Level 3, 5.1. Relative lengths
					 *      (http://www.w3.org/TR/css3-values/#relative-lengths)
					 * @{
					 */
					/// Equal to the computed value of the ‘font-size’ property of the element on which it is used.
					EM_HEIGHT,
					/// Equal to the font's x-height.
					X_HEIGHT,
					/// Equal to the advance measure of the "0" (ZERO, U+0030) glyph found in the font used to render
					/// it.
					/// @note The average character width is used by Ascension if not found.
					CHARACTERS,
					/// Equal to the computed value of ‘font-size’ on the root element.
					/// @note Refers to the global primary font in Ascension.
					ROOT_EM_HEIGHT,
					/// Equal to 1% of the width of the initial containing block.
					VIEWPORT_WIDTH,
					/// Equal to 1% of the height of the initial containing block.
					VIEWPORT_HEIGHT,
					/// Equal to the smaller of ‘vw’ or ‘vh’.
					VIEWPORT_MINIMUM,
					/// Equal to the larger of ‘vw’ or ‘vh’.
					VIEWPORT_MAXIMUM,
//					GRIDS,			///< The grid.
					/// @}

					/**
					 * @name Absolute Length Units
					 * @note The description of values are copied from CSS3.
					 * @see CSS Values and Units Module Level 3, 5.2. Absolute lengths: the ‘cm’, ‘mm’, ‘in’, ‘pt’,
					 *      ‘pc’, ‘px’ units (http://www.w3.org/TR/css3-values/#absolute-lengths)
					 * @see AbsoluteLength, absolute-length.hpp
					 * @{
					 */
					/// Centimeters.
					CENTIMETERS,
					/// Millimeters.
					MILLIMETERS,
					/// Inches; 1 in is equal to 2.54 cm.
					INCHES,
					/// Pixels; 1 px is equal to 1 / 96th of 1 in.
					/// @note <strong>Relative</strong> to the viewing device in Ascension.
					PIXELS,
					/// Points; 1 pt is equal to 1 / 72nd of 1 in.
					POINTS,
					/// Picas; 1 pc is equal to 12 pt.
					PICAS,
					/// @}

					/**
					 * @name DirectWrite
					 * @see DIP and Device-Independent Pixels (Windows)
					 *      (http://msdn.microsoft.com/en-us/library/windows/desktop/ff684173.aspx)
					 * @note Ascension treats these units as absolute.
					 * @{
					 */
					DEVICE_INDEPENDENT_PIXELS, ///< Device independent pixels; 1 DIP is equal to 1 / 96th of 1 in.
					/// @}
#if 0
					/**
					 * @name Percentages (exactly not a length)
					 * @see CSS Values and Units Module Level 3, 4.3. Percentages: the ‘<percentage>’ type
					 *      (http://www.w3.org/TR/css3-values/#percentages)
					 * @{
					 */
					PERCENTAGE,	///< Percentage.
					/// @}
#endif
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

				struct Context {
					/// The rendering context used to resolve relative value. Can be @c null if
					/// @c Length#unitType() is absolute.
					const graphics::RenderingContext2D* graphics2D;
					/// The size of the viewport in user units. This is used to resolve
					/// viewport-relative or percentage values. Can be @c null.
					const graphics::Dimension* viewport;
					Context(const graphics::RenderingContext2D* graphics2D,
						const graphics::Dimension* viewport) BOOST_NOEXCEPT
						: graphics2D(graphics2D), viewport(viewport) {}
				};

			public:
				explicit Length(Number valueInSpecifiedUnits = 0.0, Unit unitType = PIXELS, Mode = OTHER);
				bool operator==(const Length& other) const BOOST_NOEXCEPT;

				/// @name Attributes
				/// @{
				Unit unitType() const BOOST_NOEXCEPT;
				Number value(const Context& context) const;
				Number valueInSpecifiedUnits() const;
//				String valueAsString() const;
				/// @}

				/// @name Operations
				/// @{
				void convertToSpecifiedUnits(Unit unitType, const Context& context);
				void newValueSpecifiedUnits(Unit unitType, Number valueInSpecifiedUnits);
				void setValue(Number value, const Context& context);
//				void setValueAsString(const StringPiece&);
				void setValueInSpecifiedUnits(Number value) BOOST_NOEXCEPT;
				/// @}

				static bool isValidUnit(Unit unit) BOOST_NOEXCEPT;

				/// Extends @c boost#hash_value for @c Length.
				friend std::size_t hash_value(const Length& length) {
					std::size_t seed = 0;
					boost::hash_combine(seed, length.valueInSpecifiedUnits_);
					boost::hash_combine(seed, length.unit_);
					boost::hash_combine(seed, length.mode_);
					return seed;
				}

			private:
				Number valueInSpecifiedUnits_;
				Unit unit_;
				Mode mode_;
				///
				static bool isAbsolute(Unit unit);
			};

			/// Equality operator returns @c true if and only if ....
			inline bool Length::operator==(const Length& other) const BOOST_NOEXCEPT {
				return valueInSpecifiedUnits() == other.valueInSpecifiedUnits()
					&& unitType() == other.unitType() && mode_ == other.mode_;
			}

			/**
			 * [Copied from SVG 1.1 documentation] Preserves the same underlying stored value, but resets the stored
			 * unit identifier to the given @a unitType. Object attributes @c #unitType(), @c #valueInSpecifiedUnits()
			 * and @c #valueAsString() might be modified as a result of this method. For example, if the original value
			 * were "0.5cm" and the method was invoked to convert to millimeters, then the unitType would be changed to
			 * @c #MILIMIETERS, @c #valueInSpecifiedUnits() would be changed to the numeric value 5 and
			 * @c #valueAsString() would be changed to "5mm".
			 * @param unitType The unit type to switch to
			 * @param context The rendering context used to resolve relative value. Can be @c null if both
			 *                @c #unitType() and @a unitType are absolute
			 * @param contextSize The size used to resolve percentage value. Can be @c null
			 * @throw NotSupportedError @a unitType is not a valid unit type constant (one of the other @c #Unit
			 *                          constants defined on this class)
			 * @throw NullPointerException @a context is @c null although @c #unitType() and/or @a unitType is relative
			 * @see http://www.w3.org/TR/SVG11/types.html#__svg__SVGLength__convertToSpecifiedUnits
			 */
			inline void Length::convertToSpecifiedUnits(Unit unitType, const Context& context) {
				Length temp(0, unitType, mode_);		// this may throw NotSupportedError
				temp.setValue(value(context), context);	// this may throw NullPointerException
				*this = temp;
			}

			/// Returns @c true if the specified @c #Unit value is valid.
			inline bool Length::isValidUnit(Length::Unit unit) BOOST_NOEXCEPT {
				return unit >= EM_HEIGHT && unit <= DEVICE_INDEPENDENT_PIXELS;
			}

			/**
			 * [Copied from SVG 1.1 documentation] Resets the value as a number with an associated @a unitType, thereby
			 * replacing the values for all of the attributes on the object.
			 * @param unitType The unit type for the value
			 * @param valueInSpecifiedUnits The new value
			 * @throw NotSupportedError @a unitType is not a valid unit type constant (one of the other
			 *                          @c #Unit constants defined on this class)
			 * @see http://www.w3.org/TR/SVG11/types.html#__svg__SVGLength__newValueSpecifiedUnits
			 */
			inline void Length::newValueSpecifiedUnits(Unit unitType, Number valueInSpecifiedUnits) {
				if(!isValidUnit(unitType))
					throw NotSupportedError("unitType");
				unit_ = unitType;
				valueInSpecifiedUnits_ = valueInSpecifiedUnits;
			}

			/**
			 * [Copied from SVG 1.1 documentation] Sets the value as a floating point value, in the units expressed by
			 * @c #unitType(). Setting this attribute will cause @c #value() and @c #valueAsString() to be updated
			 * automatically to reflect this setting.
			 * @param value The new value
			 * @see #valueInSpecifiedUnits
			 * @see http://www.w3.org/TR/SVG11/types.html#__svg__SVGLength__valueInSpecifiedUnits
			 */
			inline void Length::setValueInSpecifiedUnits(Number value) BOOST_NOEXCEPT {
				valueInSpecifiedUnits_ = value;
			}

			/**
			 * [Copied from SVG 1.1 documentation] Returns the type of the value by one of the @c #Unit constants
			 * defined on this class.
			 * @see http://www.w3.org/TR/SVG11/types.html#__svg__SVGLength__unitType
			 */
			inline Length::Unit Length::unitType() const BOOST_NOEXCEPT {
				return unit_;
			}

			/**
			 * [Copied from SVG 1.1 documentation] Returns the value as a floating point value, in the units expressed
			 * by @c #unitType().
			 * @see #setValueInSpecifiedUnits
			 * @see http://www.w3.org/TR/SVG11/types.html#__svg__SVGLength__valueInSpecifiedUnits
			 */
			inline Number Length::valueInSpecifiedUnits() const BOOST_NOEXCEPT {
				return valueInSpecifiedUnits_;
			}
		}
	}
} // namespace ascension.presentation.styles

#endif // !ASCENSION_LENGTH_HPP
