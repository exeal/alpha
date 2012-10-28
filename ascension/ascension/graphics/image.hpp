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
#	include <ascension/win32/handle.hpp>
#endif

namespace ascension {
	namespace graphics {
		typedef
#if defined(ASCENSION_GRAPHICS_SYSTEM_CAIRO)
			Cairo::RefPtr<Cairo::ImageSurface>
#elif defined(ASCENSION_GRAPHICS_SYSTEM_CORE_GRAPHICS)
			CGImageRef
#elif defined(ASCENSION_GRAPHICS_SYSTEM_QT)
			QImage
#elif defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI)
			win32::Handle<HBITMAP>
#endif
			NativeImage;

		class Image : public RenderingDevice {
		public:
			enum Format {
				ARGB_32, RGB_24, RGB_16
			};
		public:
			Image(const NativeSize& size, Format format);
			Image(const std::uint8_t* data, const NativeSize& size, Format format);
			const NativeImage& asNativeObject() const /*throw()*/ {return impl_;}
			static int depth(Format format);
			// RenderingDevice
			std::unique_ptr<RenderingContext2D> createRenderingContext() const;
			int depth();
			std::uint32_t numberOfColors();
			geometry::Coordinate<NativeSize>::Type height() const;
			geometry::Coordinate<NativeSize>::Type heightInMillimeters() const;
			geometry::Coordinate<NativeSize>::Type logicalDpiX() const;
			geometry::Coordinate<NativeSize>::Type logicalDpiY() const;
			geometry::Coordinate<NativeSize>::Type width() const;
			geometry::Coordinate<NativeSize>::Type widthInMillimeters() const;
			geometry::Coordinate<NativeSize>::Type physicalDpiX() const;
			geometry::Coordinate<NativeSize>::Type physicalDpiY() const;
		private:
			NativeImage impl_;
		};
	}
}

#endif // !ASCENSION_IMAGE_HPP
