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
			virtual std::auto_ptr<RenderingContext2D> createGraphicContext() const = 0;
			virtual NativeSize viewportSize() const = 0;
		};

		class Screen : public RenderingDevice {
		public:
			static Screen& instance();
			std::auto_ptr<RenderingContext2D> createGraphicContext() const;
			NativeSize viewportSize() const = 0;
		};

	}
}

#endif // !ASCENSION_RENDERING_DEVICE_HPP
