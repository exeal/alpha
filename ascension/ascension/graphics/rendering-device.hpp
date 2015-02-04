/**
 * @file rendering-device.hpp
 * @author exeal
 * @date 2010-2011
 * @date 2011-06-20
 */

#ifndef ASCENSION_RENDERING_DEVICE_HPP
#define ASCENSION_RENDERING_DEVICE_HPP

#include <ascension/graphics/geometry/dimension.hpp>

namespace ascension {	
	namespace graphics {
		/// @defgroup default_dpi Default DPIs in Ascension
		/// @{
		/**
		 * Returns the default DPI in x-coordinate.
		 * @see defaultDpiY
		 */
		inline std::uint16_t defaultDpiX() BOOST_NOEXCEPT {return 96;}
		/**
		 * Returns the default DPI in y-coordinate.
		 * @see defaultDpiX
		 */
		inline std::uint16_t defaultDpiY() BOOST_NOEXCEPT {return 96;}
		/// @}

		class RenderingContext2D;

		class RenderingDevice {
		public:
			/// Destructor.
			virtual ~RenderingDevice() BOOST_NOEXCEPT {}

			/// Creates and returns the rendering context.
			virtual std::unique_ptr<RenderingContext2D> createRenderingContext() const = 0;

			/// Returns the bit depth (number of bit planes) of the device.
			virtual std::uint8_t depth() const = 0;
			/// Returns the number of colors available for the rendering device or
			/// @c std#numeric_limits&lt;std#uint32_t&gt;::max().
			virtual std::uint32_t numberOfColors() const = 0;

			/// Returns the width of the rendering device in device units.
			virtual std::uint32_t height() const = 0;
			/// Returns the height of the rendering device in millimeters.
			virtual Scalar heightInMillimeters() const {
				return static_cast<Scalar>(static_cast<double>(height()) / static_cast<double>(physicalDpiX()) * 25.4);
			}
			/// Returns the horizontal resolution of the device in dots per inch.
			virtual std::uint16_t logicalDpiX() const = 0;
			/// Returns the vertical resolution of the device in dots per inch.
			virtual std::uint16_t logicalDpiY() const = 0;
			/// Returns the width of the rendering device in device units.
			virtual std::uint32_t width() const = 0;
			/// Returns the width of the rendering device in millimeters.
			virtual Scalar widthInMillimeters() const {
				return static_cast<Scalar>(static_cast<double>(width()) / static_cast<double>(physicalDpiY()) * 25.4);
			}
			/// Returns the horizontal resolution of the device in dots per inch.
			virtual std::uint16_t physicalDpiX() const = 0;
			/// Returns the vertical resolution of the device in dots per inch.
			virtual std::uint16_t physicalDpiY() const = 0;

			geometry::BasicDimension<std::uint32_t> size() const {
				return geometry::BasicDimension<std::uint32_t>(geometry::_dx = width(), geometry::_dy = height());
			}
			Dimension sizeInMillimeters() const {
				return Dimension(geometry::_dx = widthInMillimeters(), geometry::_dy = heightInMillimeters());
			}
		};
	}
}

#endif // !ASCENSION_RENDERING_DEVICE_HPP
