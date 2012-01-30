/**
 * @file rendering-device.hpp
 * @author exeal
 * @date 2010-2011
 * @date 2011-06-20
 */

#ifndef ASCENSION_RENDERING_DEVICE_HPP
#define ASCENSION_RENDERING_DEVICE_HPP

#include <ascension/graphics/geometry.hpp>	// NativeSize

namespace ascension {
	
	namespace graphics {

		class RenderingContext2D;

		class RenderingDevice {
		public:
			/// Destructor.
			virtual ~RenderingDevice() /*throw()*/ {}

			/// Creates and returns the rendering context.
			virtual std::unique_ptr<RenderingContext2D> createRenderingContext() const = 0;

			/// Returns the bit depth (number of bit planes) of the device.
			virtual int depth() = 0;
			/// Returns the number of colors available for the rendering device or
			/// @c std#numeric_limits&lt;uint32_t&gt;::max().
			virtual uint32_t numberOfColors() = 0;

			/// Returns the width of the rendering device in device units.
			virtual geometry::Coordinate<NativeSize>::Type height() const = 0;
			/// Returns the height of the rendering device in millimeters.
			virtual geometry::Coordinate<NativeSize>::Type heightInMillimeters() const = 0;
			/// Returns the horizontal resolution of the device in dots per inch.
			virtual geometry::Coordinate<NativeSize>::Type logicalDpiX() const = 0;
			/// Returns the vertical resolution of the device in dots per inch.
			virtual geometry::Coordinate<NativeSize>::Type logicalDpiY() const = 0;
			/// Returns the width of the rendering device in device units.
			virtual geometry::Coordinate<NativeSize>::Type width() const = 0;
			/// Returns the width of the rendering device in millimeters.
			virtual geometry::Coordinate<NativeSize>::Type widthInMillimeters() const = 0;
			/// Returns the horizontal resolution of the device in dots per inch.
			virtual geometry::Coordinate<NativeSize>::Type physicalDpiX() const = 0;
			/// Returns the vertical resolution of the device in dots per inch.
			virtual geometry::Coordinate<NativeSize>::Type physicalDpiY() const = 0;

			NativeSize size() const {
				return geometry::make<NativeSize>(width(), height());
			}
			NativeSize sizeInMillimeters() const {
				return geometry::make<NativeSize>(widthInMillimeters(), heightInMillimeters());
			}
		};

		class Screen : public RenderingDevice {
		public:
			static Screen& instance();
			std::unique_ptr<RenderingContext2D> createGraphicContext() const;
		};

	}
}

#endif // !ASCENSION_RENDERING_DEVICE_HPP
