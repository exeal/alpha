/**
 * @file length.cpp
 * @author exeal
 * @date 2011-06-20 created.
 * @date 2011-2013
 */

#include <ascension/corelib/basic-exceptions.hpp>	// NullPointerException
#include <ascension/graphics/rendering-context.hpp>
#include <ascension/graphics/rendering-device.hpp>
#include <ascension/graphics/font/font-metrics.hpp>
#include <ascension/presentation/length.hpp>
using namespace ascension;
using namespace ascension::graphics;
using namespace ascension::presentation;
using namespace std;


namespace {
	inline Scalar pixelsPerInch(const RenderingContext2D* graphics2D, Length::Mode mode) {
#if 1
		return 96.0;
#else
		if(graphics2D == nullptr)
			throw NullPointerException("context.graphics2D");
		return (mode != Length::HEIGHT) ? graphics2D->logicalDpiX() : graphics2D->logicalDpiY();
#endif
	}
}

/**
 * Constructor.
 * @param valueInSpecifiedUnits The initial value
 * @param unitType The initial unit type for the value
 * @param mode The initial mode
 * @throw NotSupportedError @a unitType is not a valid unit type constant (one of the other
 *                          @c #Unit constants defined on this class)
 */
Length::Length(Scalar valueInSpecifiedUnits /* = 0.0 */, Unit unitType /* = PIXELS */, Mode mode /* = OTHER */) : mode_(mode) {
	newValueSpecifiedUnits(unitType, valueInSpecifiedUnits);
}

/**
 * [Copied from SVG 1.1 documentation] Sets the value as a floating point value, in user units.
 * Setting this attribute will cause @c #valueInSpecifiedUnits() and @c #valueAsString() to be
 * updated automatically to reflect this setting.
 * @param value The new value
 * @param context The rendering context used to resolve relative value. Can be @c null if
 *                @c #unitType() is absolute
 * @param contextSize The size used to resolve percentage value. Can be @c null
 * @throw NullPointerException @a context is @c null although @c #unitType() is relative
 * @see http://www.w3.org/TR/SVG11/types.html#__svg__SVGLength__value
 */
void Length::setValue(Scalar value, const Context& context) {
	switch(unitType()) {
		case EM_HEIGHT:
			if(context.graphics2D == nullptr)
				throw NullPointerException("context.graphics2D");
			valueInSpecifiedUnits_ = value / context.graphics2D->fontMetrics()->emHeight();
			break;
		case X_HEIGHT:
			if(context.graphics2D == nullptr)
				throw NullPointerException("context.graphics2D");
			valueInSpecifiedUnits_ = value / context.graphics2D->fontMetrics()->xHeight();
			break;
		case CHARACTERS:
			if(context.graphics2D == nullptr)
				throw NullPointerException("context.graphics2D");
			valueInSpecifiedUnits_ = value / context.graphics2D->fontMetrics()->averageCharacterWidth();
			break;
//		case GRIDS:
//		case ROOT_EM_HEIGHT:
		case VIEWPORT_WIDTH:
			if(context.viewport == nullptr)
				throw NullPointerException("context.viewport");
			valueInSpecifiedUnits_ = value / geometry::dx(*context.viewport);
			break;
		case VIEWPORT_HEIGHT:
			if(context.viewport == nullptr)
				throw NullPointerException("context.viewport");
			valueInSpecifiedUnits_ = value / geometry::dy(*context.viewport);
			break;
		case VIEWPORT_MINIMUM:
			if(context.viewport == nullptr)
				throw NullPointerException("context.viewport");
			valueInSpecifiedUnits_ = value / min(geometry::dx(*context.viewport), geometry::dy(*context.viewport));
			break;
		case VIEWPORT_MAXIMUM:
			if(context.viewport == nullptr)
				throw NullPointerException("context.viewport");
			valueInSpecifiedUnits_ = value / max(geometry::dx(*context.viewport), geometry::dy(*context.viewport));
			break;
		case CENTIMETERS:
			valueInSpecifiedUnits_ = value / pixelsPerInch(context.graphics2D, mode_) * 2.54f;
			break;
		case MILLIMETERS:
			valueInSpecifiedUnits_ = value / pixelsPerInch(context.graphics2D, mode_) * 25.4f;
			break;
		case INCHES:
			valueInSpecifiedUnits_ = value / pixelsPerInch(context.graphics2D, mode_);
			break;
		case PIXELS:
			valueInSpecifiedUnits_ = value;
			break;
		case POINTS:
			valueInSpecifiedUnits_ = value / pixelsPerInch(context.graphics2D, mode_) * 72;
			break;
		case PICAS:
			valueInSpecifiedUnits_ = value / pixelsPerInch(context.graphics2D, mode_) * 72 / 12;
			break;
		case DEVICE_INDEPENDENT_PIXELS:
			valueInSpecifiedUnits_ = value / pixelsPerInch(context.graphics2D, mode_) * 96;
			break;
		case PERCENTAGE:
			if(context.viewport == nullptr)
				throw NullPointerException("context.viewport");
			if(mode_ == WIDTH)
				valueInSpecifiedUnits_ = value / geometry::dx(*context.viewport) * 100;
			else if(mode_ == HEIGHT)
				valueInSpecifiedUnits_ = value / geometry::dy(*context.viewport) * 100;
			else if(mode_ == OTHER)
				valueInSpecifiedUnits_ = value / sqrt(static_cast<Scalar>(
					geometry::dx(*context.viewport) * geometry::dx(*context.viewport)
					+ geometry::dy(*context.viewport) * geometry::dy(*context.viewport)) / 2) * 100;
			break;
		default:
			ASCENSION_ASSERT_NOT_REACHED();
	}
}

/**
 * [Copied from SVG 1.1 documentation] Returns the value as a floating point value, in user units.
 * @param context The rendering context used to resolve relative value. Can be @c null if
 *                @c #unitType() is absolute
 * @param contextSize The size used to resolve percentage value
 * @throw NullPointerException @a context is @c null although @c #unitType() is relative
 * @see http://www.w3.org/TR/SVG11/types.html#__svg__SVGLength__value
 */
Scalar Length::value(const Context& context) const {
	switch(unitType()) {
		case EM_HEIGHT:
			if(context.graphics2D == nullptr)
				throw NullPointerException("context.graphics2D");
			return valueInSpecifiedUnits() * context.graphics2D->fontMetrics()->emHeight();
		case X_HEIGHT:
			if(context.graphics2D == nullptr)
				throw NullPointerException("context.graphics2D");
			return valueInSpecifiedUnits() * context.graphics2D->fontMetrics()->xHeight();
		case CHARACTERS:
			if(context.graphics2D == nullptr)
				throw NullPointerException("context.graphics2D");
			return valueInSpecifiedUnits() * context.graphics2D->fontMetrics()->averageCharacterWidth();
//		case GRIDS:
//		case ROOT_EM_HEIGHT:
		case VIEWPORT_WIDTH:
			if(context.viewport == nullptr)
				throw NullPointerException("context.viewport");
			return valueInSpecifiedUnits() * geometry::dx(*context.viewport);
		case VIEWPORT_HEIGHT:
			if(context.viewport == nullptr)
				throw NullPointerException("context.viewport");
			return valueInSpecifiedUnits() * geometry::dy(*context.viewport);
		case VIEWPORT_MINIMUM:
			if(context.viewport == nullptr)
				throw NullPointerException("context.viewport");
			return valueInSpecifiedUnits() * min(geometry::dx(*context.viewport), geometry::dy(*context.viewport));
		case VIEWPORT_MAXIMUM:
			if(context.viewport == nullptr)
				throw NullPointerException("context.viewport");
			return valueInSpecifiedUnits() * max(geometry::dx(*context.viewport), geometry::dy(*context.viewport));
		case CENTIMETERS:
			return valueInSpecifiedUnits() * pixelsPerInch(context.graphics2D, mode_) / 2.54f;
		case MILLIMETERS:
			return valueInSpecifiedUnits() * pixelsPerInch(context.graphics2D, mode_) / 25.4f;
		case INCHES:
			return valueInSpecifiedUnits() * pixelsPerInch(context.graphics2D, mode_);
		case PIXELS:
			return valueInSpecifiedUnits();
		case POINTS:
			return valueInSpecifiedUnits() * pixelsPerInch(context.graphics2D, mode_) / 72;
		case PICAS:
			return valueInSpecifiedUnits() * pixelsPerInch(context.graphics2D, mode_) / 72 * 12;
		case DEVICE_INDEPENDENT_PIXELS:
			return valueInSpecifiedUnits() * pixelsPerInch(context.graphics2D, mode_) / 96;
		case PERCENTAGE: {
			if(context.viewport == nullptr)
				throw NullPointerException("context.viewport");
			if(mode_ == WIDTH)
				return valueInSpecifiedUnits() * geometry::dx(*context.viewport) / 100;
			else if(mode_ == HEIGHT)
				return valueInSpecifiedUnits() * geometry::dy(*context.viewport) / 100;
			else if(mode_ == OTHER)
				return valueInSpecifiedUnits() * sqrt(static_cast<Scalar>(
					geometry::dx(*context.viewport) * geometry::dx(*context.viewport)
					+ geometry::dy(*context.viewport) * geometry::dy(*context.viewport)) / 2) / 100;
		}
	}
	ASCENSION_ASSERT_NOT_REACHED();
}
