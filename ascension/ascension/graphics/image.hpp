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
			Image(const geometry::BasicDimension<std::uint16_t>& size, Format format);
			Image(const std::uint8_t* data, const geometry::BasicDimension<std::uint16_t>& size, Format format);
			const NativeImage& asNativeObject() const BOOST_NOEXCEPT {return impl_;}
			static int depth(Format format);
			// RenderingDevice
			std::unique_ptr<RenderingContext2D> createRenderingContext() const;
			std::uint16_t depth();
			std::uint32_t numberOfColors();
			std::uint16_t height() const;
			Scalar heightInMillimeters() const;
			std::uint16_t logicalDpiX() const;
			std::uint16_t logicalDpiY() const;
			std::uint16_t width() const;
			Scalar widthInMillimeters() const;
			std::uint16_t physicalDpiX() const;
			std::uint16_t physicalDpiY() const;
		private:
			NativeImage impl_;
		};
	}
}

#endif // !ASCENSION_IMAGE_HPP
