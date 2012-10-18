/**
 * @file length.cpp
 * @author exeal
 * @date 2011-06-20 created.
 * @date 2011-2012
 */

#include <ascension/corelib/basic-exceptions.hpp>	// NullPointerException
#include <ascension/graphics/rendering-context.hpp>
#include <ascension/graphics/rendering-device.hpp>
#include <ascension/presentation/length.hpp>
using namespace ascension;
using namespace ascension::graphics;
using namespace ascension::presentation;
using namespace std;


namespace {
	inline double pixelsPerInch(const RenderingContext2D* context, Length::Mode mode) {
#if 1
		return 96.0;
#else
		return (mode != Length::HEIGHT) ? context->logicalDpiX() : context->logicalDpiY();
#endif
	}
}

/**
 * Constructor.
 * @param valueInSpecifiedUnits
 * @param unitType
 * @param mode
 * @throw NotSupportedError
 */
Length::Length(double valueInSpecifiedUnits /* = 0.0 */, Unit unitType /* = PIXELS */, Mode mode /* = OTHER */) : mode_(mode) {
	newValueSpecifiedUnits(unitType, valueInSpecifiedUnits);
}

/**
 * [Copied from SVG 1.1 documentation] Preserves the same underlying stored value, but resets the
 * stored unit identifier to the given @a unitType. Object attributes @c #unitType(),
 * @c #valueInSpecifiedUnits() and @c #valueAsString() might be modified as a result of this
 * method. For example, if the original value were "0.5cm" and the method was invoked to convert to
 * millimeters, then the unitType would be changed to @c #MILIMIETERS, @c #valueInSpecifiedUnits()
 * would be changed to the numeric value 5 and @c #valueAsString() would be changed to "5mm".
 * @param unitType The unit type to switch to
 * @param context The rendering context used to resolve relative value. Can be @c null if both
 *                @c #unitType() and @a unitType are absolute
 * @param contextSize The size used to resolve percentage value. Can be @c null
 * @throw NotSupportedError @a unitType is not a valid unit type constant (one of the other
 *                          @c #Unit constants defined on this class)
 * @throw NullPointerException @a context is @c null although @c #unitType() and/or @a unitType is
 *                             relative
 * @see http://www.w3.org/TR/SVG11/types.html#__svg__SVGLength__convertToSpecifiedUnits
 */
void Length::convertToSpecifiedUnits(Unit unitType, const RenderingContext2D* context, const NativeSize* contextSize) {
	if(unitType > PERCENTAGE)
		throw NotSupportedError("unitType");
	Length temp(0, unitType, mode_);
	temp.setValue(value(context, contextSize), context, contextSize);	// this may throw NullPointerException
	*this = temp;
}

/**
 * [Copied from SVG 1.1 documentation] Resets the value as a number with an associated @a unitType,
 * thereby replacing the values for all of the attributes on the object.
 * @param unitType The unit type for the value
 * @param valueInSpecifiedUnits The new value
 * @throw NotSupportedError @a unitType is not a valid unit type constant (one of the other
 *                          @c #Unit constants defined on this class)
 * @see http://www.w3.org/TR/SVG11/types.html#__svg__SVGLength__newValueSpecifiedUnits
 */
void Length::newValueSpecifiedUnits(Unit unitType, double valueInSpecifiedUnits) {
	if(unitType > PERCENTAGE)
		throw NotSupportedError("unitType");
	unit_ = unitType;
	valueInSpecifiedUnits_ = valueInSpecifiedUnits;
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
void Length::setValue(double value, const RenderingContext2D* context, const NativeSize* contextSize) {
	switch(unitType()) {
		case EM_HEIGHT:
			if(context == nullptr)
				throw NullPointerException("context");
			valueInSpecifiedUnits_ = value / context->font()->metrics()->emHeight();
			break;
		case X_HEIGHT:
			if(context == nullptr)
				throw NullPointerException("context");
			valueInSpecifiedUnits_ = value / context->font()->metrics()->xHeight();
			break;
		case CHARACTERS:
			if(context == nullptr)
				throw NullPointerException("context");
			valueInSpecifiedUnits_ = value / context->font()->metrics()->averageCharacterWidth();
//		case GRIDS:
//		case ROOT_EM_HEIGHT:
		case VIEWPORT_WIDTH:
			if(context == nullptr)
				throw NullPointerException("context");
			valueInSpecifiedUnits_ = value / geometry::dx(context->device().size());
		case VIEWPORT_HEIGHT:
			if(context == nullptr)
				throw NullPointerException("context");
			valueInSpecifiedUnits_ = value / geometry::dy(context->device().size());
		case VIEWPORT_MINIMUM:
			if(context == nullptr)
				throw NullPointerException("context");
			valueInSpecifiedUnits_ = value / min(geometry::dx(context->device().size()), geometry::dy(context->device().size()));
		case VIEWPORT_MAXIMUM:
			if(context == nullptr)
				throw NullPointerException("context");
			valueInSpecifiedUnits_ = value / max(geometry::dx(context->device().size()), geometry::dy(context->device().size()));
		case CENTIMETERS:
			valueInSpecifiedUnits_ = value / pixelsPerInch(context, mode_) * 2.54;
		case MILLIMETERS:
			valueInSpecifiedUnits_ = value / pixelsPerInch(context, mode_) * 25.4;
		case INCHES:
			valueInSpecifiedUnits_ = value / pixelsPerInch(context, mode_);
		case PIXELS:
			valueInSpecifiedUnits_ = value;
		case POINTS:
			valueInSpecifiedUnits_ = value / pixelsPerInch(context, mode_) * 72;
		case PICAS:
			valueInSpecifiedUnits_ = value / pixelsPerInch(context, mode_) * 72 / 12;
		case DEVICE_INDEPENDENT_PIXELS:
			valueInSpecifiedUnits_ = value / pixelsPerInch(context, mode_) * 96;
		case PERCENTAGE: {
			if(context == nullptr)
				throw NullPointerException("context");
			const NativeSize size((contextSize != nullptr) ? *contextSize : context->device().size());
			if(mode_ == WIDTH)
				valueInSpecifiedUnits_ = value / geometry::dx(size) * 100;
			else if(mode_ == HEIGHT)
				valueInSpecifiedUnits_ = value / geometry::dy(size) * 100;
			else if(mode_ == OTHER)
				valueInSpecifiedUnits_ = value / sqrt(static_cast<double>(
					geometry::dx(size) * geometry::dx(size) + geometry::dy(size) * geometry::dy(size)) / 2) * 100;
		}
	}
	ASCENSION_ASSERT_NOT_REACHED();
}

/**
 * [Copied from SVG 1.1 documentation] Returns the value as a floating point value, in user units.
 * @param context The rendering context used to resolve relative value. Can be @c null if
 *                @c #unitType() is absolute
 * @param contextSize The size used to resolve percentage value
 * @throw NullPointerException @a context is @c null although @c #unitType() is relative
 * @see http://www.w3.org/TR/SVG11/types.html#__svg__SVGLength__value
 */
double Length::value(const RenderingContext2D* context, const NativeSize* contextSize) const {
	switch(unitType()) {
		case EM_HEIGHT:
			if(context == nullptr)
				throw NullPointerException("context");
			return valueInSpecifiedUnits() * context->font()->metrics()->emHeight();
		case X_HEIGHT:
			if(context == nullptr)
				throw NullPointerException("context");
			return valueInSpecifiedUnits() * context->font()->metrics()->xHeight();
		case CHARACTERS:
			if(context == nullptr)
				throw NullPointerException("context");
			return valueInSpecifiedUnits() * context->font()->metrics()->averageCharacterWidth();
//		case GRIDS:
//		case ROOT_EM_HEIGHT:
		case VIEWPORT_WIDTH:
			if(context == nullptr)
				throw NullPointerException("context");
			return valueInSpecifiedUnits() * geometry::dx(context->device().size());
		case VIEWPORT_HEIGHT:
			if(context == nullptr)
				throw NullPointerException("context");
			return valueInSpecifiedUnits() * geometry::dy(context->device().size());
		case VIEWPORT_MINIMUM:
			if(context == nullptr)
				throw NullPointerException("context");
			return valueInSpecifiedUnits() * min(geometry::dx(context->device().size()), geometry::dy(context->device().size()));
		case VIEWPORT_MAXIMUM:
			if(context == nullptr)
				throw NullPointerException("context");
			return valueInSpecifiedUnits() * max(geometry::dx(context->device().size()), geometry::dy(context->device().size()));
		case CENTIMETERS:
			return valueInSpecifiedUnits() * pixelsPerInch(context, mode_) / 2.54;
		case MILLIMETERS:
			return valueInSpecifiedUnits() * pixelsPerInch(context, mode_) / 25.4;
		case INCHES:
			return valueInSpecifiedUnits() * pixelsPerInch(context, mode_);
		case PIXELS:
			return valueInSpecifiedUnits();
		case POINTS:
			return valueInSpecifiedUnits() * pixelsPerInch(context, mode_) / 72;
		case PICAS:
			return valueInSpecifiedUnits() * pixelsPerInch(context, mode_) / 72 * 12;
		case DEVICE_INDEPENDENT_PIXELS:
			return valueInSpecifiedUnits() * pixelsPerInch(context, mode_) / 96;
		case PERCENTAGE: {
			if(context == nullptr)
				throw NullPointerException("context");
			const NativeSize size((contextSize != nullptr) ? *contextSize : context->device().size());
			if(mode_ == WIDTH)
				return valueInSpecifiedUnits() * geometry::dx(size);
			else if(mode_ == HEIGHT)
				return valueInSpecifiedUnits() * geometry::dy(size);
			else if(mode_ == OTHER)
				return valueInSpecifiedUnits() * sqrt(static_cast<double>(
					geometry::dx(size) * geometry::dx(size) + geometry::dy(size) * geometry::dy(size)) / 2);
		}
	}
	ASCENSION_ASSERT_NOT_REACHED();
}
