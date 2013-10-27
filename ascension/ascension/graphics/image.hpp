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
#	include <cairomm/surface.h>
#elif defined(ASCENSION_GRAPHICS_SYSTEM_CORE_GRAPHICS)
#	include <>
#elif defined(ASCENSION_GRAPHICS_SYSTEM_QT)
#	include <QImage.h>
#elif defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI)
#	include <ascension/win32/handle.hpp>
#endif
#include <boost/range/iterator_range.hpp>

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
			win32::Handle<HBITMAP>::Type
#endif
			NativeImage;

		class Image : public RenderingDevice {
		public:
			/**
			 * <table>
			 *   <tr><th>Image.Format</th><th>cairomm</th><th>QtGui</th><th>Microsoft Win32<br/>(BITMAPV5HEADER#bV5BitCount)</th></tr>
			 *   <tr><td>ARGB32</td><td>Cairo#FORMAT_ARGB32</td><td>QImage#Format_ARGB32</td><td>32</td></tr>
			 *   <tr><td>RGB24</td><td>Cairo#FORMAT_RGB24</td><td>QImage#Format_RGB888</td><td>24</td></tr>
			 *   <tr><td>RGB16</td><td>Cairo#FORMAT_RGB16_565</td><td>Image#Format_RGB16</td><td>16</td></tr>
			 *   <tr><td>A1</td><td>Cairo#FORMAT_A1</td><td>QImage#Format_Mono</td><td>1</td></tr>
			 * </table>
			 */
			enum Format {
				ARGB32,	///< 
				RGB24,	///< 
				RGB16,	///< 
				A1		///< 
			};
		public:
			/**
			 * Creates an image with the specified format and dimensions.
			 * @param size The size of the image in pixels
			 * @param format The format of the image
			 * @throw UnknownValueException @a format is unknown
			 */
			Image(const geometry::BasicDimension<std::uint16_t>& size, Format format);
			/**
			 * Creates an image with the specified format, dimensions and pixel data.
			 * @param data The pi
			 * @param size The size of the image in pixels
			 * @param format The format of the image
			 * @throw UnknownValueException @a format is unknown
			 */
			Image(std::unique_ptr<std::uint8_t[]> data, const geometry::BasicDimension<std::uint16_t>& size, Format format);
			const NativeImage& asNativeObject() const BOOST_NOEXCEPT {return impl_;}
			static int depth(Format format);
			/// Returns the format of the image.
			Format format() const;
			boost::iterator_range<std::uint8_t*> pixels();
			boost::iterator_range<const std::uint8_t*> pixels() const;
			// RenderingDevice
			std::unique_ptr<RenderingContext2D> createRenderingContext() const;
			std::uint16_t depth() const;
			std::uint32_t numberOfColors() const;
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
			std::unique_ptr<std::uint8_t[]> buffer_;
		};
	}
}

#endif // !ASCENSION_IMAGE_HPP
