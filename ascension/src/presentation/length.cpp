/**
 * @file length.cpp
 * @author exeal
 * @date 2011-06-20 created.
 * @date 2011-2014
 */

#include <ascension/corelib/basic-exceptions.hpp>	// NullPointerException
#include <ascension/graphics/rendering-context.hpp>
#include <ascension/graphics/font/font-metrics.hpp>
#include <ascension/graphics/geometry/algorithm.hpp>
#include <ascension/presentation/absolute-length.hpp>
#include <ascension/presentation/styles/length.hpp>


namespace ascension {
	namespace presentation {
		namespace styles {
			namespace {
				inline graphics::Scalar pixelsPerInch(const graphics::RenderingContext2D* graphics2D, Length::Mode mode) {
#if 1
					return static_cast<graphics::Scalar>(Inches::Scale::num) / Inches::Scale::den;
#elif 0
					return (mode != Length::HEIGHT) ? graphics::defaultDpiX() : graphics::defaultDpiY();
#else
					if(graphics2D == nullptr)
						throw NullPointerException("context.graphics2D");
					return (mode != Length::HEIGHT) ? graphics2D->logicalDpiX() : graphics2D->logicalDpiY();
#endif
				}

				template<typename AbsoluteLengthType>
				inline BOOST_CONSTEXPR graphics::Scalar fromPixels(graphics::Scalar pixels, const graphics::RenderingContext2D*, Length::Mode) BOOST_NOEXCEPT {
					return pixels / AbsoluteLengthType::Scale::num * AbsoluteLengthType::Scale::den;
				}

				template<typename AbsoluteLengthType>
				inline BOOST_CONSTEXPR graphics::Scalar toPixels(graphics::Scalar length, const graphics::RenderingContext2D*, Length::Mode) BOOST_NOEXCEPT {
					return length * AbsoluteLengthType::Scale::num / AbsoluteLengthType::Scale::den;
				}
			}

			/**
			 * Constructor.
			 * @param valueInSpecifiedUnits The initial value
			 * @param unitType The initial unit type for the value
			 * @param mode The initial mode
			 * @throw NotSupportedError @a unitType is not a valid unit type constant (one of the other @c #Unit
			 *                          constants defined on this class)
			 */
			Length::Length(graphics::Scalar valueInSpecifiedUnits /* = 0.0 */, Unit unitType /* = PIXELS */, Mode mode /* = OTHER */) : mode_(mode) {
				newValueSpecifiedUnits(unitType, valueInSpecifiedUnits);	// may throw NotSupportedError
			}

			/**
			 * [Copied from SVG 1.1 documentation] Sets the value as a floating point value, in user units. Setting
			 * this attribute will cause @c #valueInSpecifiedUnits() and @c #valueAsString() to be updated
			 * automatically to reflect this setting.
			 * @param value The new value
			 * @param context The rendering context used to resolve relative value. Can be @c null if @c #unitType() is
			 *                absolute
			 * @param contextSize The size used to resolve percentage value. Can be @c null
			 * @throw NullPointerException @a context is @c null although @c #unitType() is relative
			 * @see http://www.w3.org/TR/SVG11/types.html#__svg__SVGLength__value
			 */
			void Length::setValue(graphics::Scalar value, const Context& context) {
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
//					case GRIDS:
//					case ROOT_EM_HEIGHT:
					case VIEWPORT_WIDTH:
						if(context.viewport == nullptr)
							throw NullPointerException("context.viewport");
						valueInSpecifiedUnits_ = value / graphics::geometry::dx(*context.viewport);
						break;
					case VIEWPORT_HEIGHT:
						if(context.viewport == nullptr)
							throw NullPointerException("context.viewport");
						valueInSpecifiedUnits_ = value / graphics::geometry::dy(*context.viewport);
						break;
					case VIEWPORT_MINIMUM:
						if(context.viewport == nullptr)
							throw NullPointerException("context.viewport");
						valueInSpecifiedUnits_ = value / std::min(graphics::geometry::dx(*context.viewport), graphics::geometry::dy(*context.viewport));
						break;
					case VIEWPORT_MAXIMUM:
						if(context.viewport == nullptr)
							throw NullPointerException("context.viewport");
						valueInSpecifiedUnits_ = value / std::max(graphics::geometry::dx(*context.viewport), graphics::geometry::dy(*context.viewport));
						break;
					case CENTIMETERS:
						valueInSpecifiedUnits_ = fromPixels<Centimeters>(value, context.graphics2D, mode_);
						break;
					case MILLIMETERS:
						valueInSpecifiedUnits_ = fromPixels<Millimeters>(value, context.graphics2D, mode_);
						break;
					case INCHES:
						valueInSpecifiedUnits_ = fromPixels<Inches>(value, context.graphics2D, mode_);
						break;
					case PIXELS:
						valueInSpecifiedUnits_ = fromPixels<Pixels>(value, context.graphics2D, mode_);
						break;
					case POINTS:
						valueInSpecifiedUnits_ = fromPixels<Points>(value, context.graphics2D, mode_);
						break;
					case PICAS:
						valueInSpecifiedUnits_ = fromPixels<Picas>(value, context.graphics2D, mode_);
						break;
					case DEVICE_INDEPENDENT_PIXELS:
						valueInSpecifiedUnits_ = fromPixels<DeviceIndependentPixels>(value, context.graphics2D, mode_);
						break;
					case PERCENTAGE:
						if(context.viewport == nullptr)
							throw NullPointerException("context.viewport");
						if(mode_ == WIDTH)
							valueInSpecifiedUnits_ = value / graphics::geometry::dx(*context.viewport) * 100;
						else if(mode_ == HEIGHT)
							valueInSpecifiedUnits_ = value / graphics::geometry::dy(*context.viewport) * 100;
						else if(mode_ == OTHER)
							valueInSpecifiedUnits_ = value / sqrt(static_cast<graphics::Scalar>(
								graphics::geometry::dx(*context.viewport) * graphics::geometry::dx(*context.viewport)
								+ graphics::geometry::dy(*context.viewport) * graphics::geometry::dy(*context.viewport)) / 2) * 100;
						break;
					default:
						ASCENSION_ASSERT_NOT_REACHED();
				}
			}

			/**
			 * [Copied from SVG 1.1 documentation] Returns the value as a floating point value, in user units.
			 * @param context The rendering context used to resolve relative value. Can be @c null if @c #unitType() is
			 *                absolute
			 * @param contextSize The size used to resolve percentage value
			 * @throw NullPointerException @a context is @c null although @c #unitType() is relative
			 * @see http://www.w3.org/TR/SVG11/types.html#__svg__SVGLength__value
			 */
			graphics::Scalar Length::value(const Context& context) const {
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
//					case GRIDS:
//					case ROOT_EM_HEIGHT:
					case VIEWPORT_WIDTH:
						if(context.viewport == nullptr)
							throw NullPointerException("context.viewport");
						return valueInSpecifiedUnits() * graphics::geometry::dx(*context.viewport);
					case VIEWPORT_HEIGHT:
						if(context.viewport == nullptr)
							throw NullPointerException("context.viewport");
						return valueInSpecifiedUnits() * graphics::geometry::dy(*context.viewport);
					case VIEWPORT_MINIMUM:
						if(context.viewport == nullptr)
							throw NullPointerException("context.viewport");
						return valueInSpecifiedUnits() * std::min(graphics::geometry::dx(*context.viewport), graphics::geometry::dy(*context.viewport));
					case VIEWPORT_MAXIMUM:
						if(context.viewport == nullptr)
							throw NullPointerException("context.viewport");
						return valueInSpecifiedUnits() * std::max(graphics::geometry::dx(*context.viewport), graphics::geometry::dy(*context.viewport));
					case CENTIMETERS:
						return toPixels<Centimeters>(valueInSpecifiedUnits(), context.graphics2D, mode_);
					case MILLIMETERS:
						return toPixels<Millimeters>(valueInSpecifiedUnits(), context.graphics2D, mode_);
					case INCHES:
						return toPixels<Inches>(valueInSpecifiedUnits(), context.graphics2D, mode_);
					case PIXELS:
						return toPixels<Pixels>(valueInSpecifiedUnits(), context.graphics2D, mode_);
					case POINTS:
						return toPixels<Points>(valueInSpecifiedUnits(), context.graphics2D, mode_);
					case PICAS:
						return toPixels<Picas>(valueInSpecifiedUnits(), context.graphics2D, mode_);
					case DEVICE_INDEPENDENT_PIXELS:
						return toPixels<DeviceIndependentPixels>(valueInSpecifiedUnits(), context.graphics2D, mode_);
					case PERCENTAGE: {
						if(context.viewport == nullptr)
							throw NullPointerException("context.viewport");
						if(mode_ == WIDTH)
							return valueInSpecifiedUnits() * graphics::geometry::dx(*context.viewport) / 100;
						else if(mode_ == HEIGHT)
							return valueInSpecifiedUnits() * graphics::geometry::dy(*context.viewport) / 100;
						else if(mode_ == OTHER)
							return valueInSpecifiedUnits() * sqrt(static_cast<graphics::Scalar>(
								graphics::geometry::dx(*context.viewport) * graphics::geometry::dx(*context.viewport)
								+ graphics::geometry::dy(*context.viewport) * graphics::geometry::dy(*context.viewport)) / 2) / 100;
					}
				}
				ASCENSION_ASSERT_NOT_REACHED();
			}
		}
	}
}
