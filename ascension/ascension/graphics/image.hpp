/**
 * @file image.hpp
 * @author exeal
 * @date 2011-10-01
 */

#ifndef ASCENSION_IMAGE_HPP
#define ASCENSION_IMAGE_HPP
#include <ascension/platforms.hpp>
#include <ascension/graphics/rendering-device.hpp>
#if defined(ASCENSION_GRAPHICS_SYSTEM_CAIRO)
#	include <cairomm.h>
#elif defined(ASCENSION_GRAPHICS_SYSTEM_CORE_GRAPHICS)
#	include <>
#elif defined(ASCENSION_GRAPHICS_SYSTEM_QT)
#	include <QImage.h>
#elif defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI)
#	include <ascension/win32/windows.hpp>
#endif

namespace ascension {
	namespace graphics {
		class Image : public RenderingDevice {
		public:
			enum Format {
				ARGB_32, RGB_24, RGB_16
			};
		public:
			Image(const NativeSize& size, Format format);
			Image(const uint8_t* data, const NativeSize& size, Format format);
			std::auto_ptr<RenderingContext2D> createRenderingContext() const;
			static int depth(Format format);
		private:
 #if defined(ASCENSION_GRAPHICS_SYSTEM_CAIRO)
			Cairo::RefPtr<Cairo::ImageSurface> impl_;
 #elif defined(ASCENSION_GRAPHICS_SYSTEM_CORE_GRAPHICS)
			CGImageRef impl_;
 #elif defined(ASCENSION_GRAPHICS_SYSTEM_QT)
			QImage impl_;
 #elif defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI)
			win32::Handle<HBITMAP> impl_;
 #endif
		};
	}
}

#endif // !ASCENSION_IMAGE_HPP
