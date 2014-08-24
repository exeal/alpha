/**
 * @file image.hpp
 * @author exeal
 * @date 2011-10-01
 */

#ifndef ASCENSION_IMAGE_HPP
#define ASCENSION_IMAGE_HPP
#include <ascension/platforms.hpp>
#include <ascension/graphics/object.hpp>
#include <ascension/graphics/rendering-device.hpp>
#if ASCENSION_SELECTS_GRAPHICS_SYSTEM(CAIRO)
#	include <cairomm/surface.h>
#elif ASCENSION_SELECTS_GRAPHICS_SYSTEM(CORE_GRAPHICS)
#	include <>
#elif ASCENSION_SELECTS_GRAPHICS_SYSTEM(QT)
#	include <QImage.h>
#elif ASCENSION_SELECTS_GRAPHICS_SYSTEM(WIN32_GDI)
#	include <ascension/win32/handle.hpp>
#	include <boost/core/null_deleter.hpp>
#endif
#include <boost/range/iterator_range.hpp>

namespace ascension {
	namespace graphics {
		class Image : public RenderingDevice, public Wrapper<Image> {
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
			Image(const geometry::BasicDimension<std::uint32_t>& size, Format format);
			Image(const std::uint8_t* data, const geometry::BasicDimension<std::uint32_t>& size, Format format);
			Image(std::unique_ptr<std::uint8_t[]> data, const geometry::BasicDimension<std::uint32_t>& size, Format format);
			Image(const Image& other);

#if ASCENSION_SELECTS_GRAPHICS_SYSTEM(CAIRO)
			Cairo::RefPtr<Cairo::ImageSurface> asNative();
			Cairo::RefPtr<const Cairo::ImageSurface> asNative() const;
#elif ASCENSION_SELECTS_GRAPHICS_SYSTEM(CORE_GRAPHICS)
			CGImageRef asNative() const;
#elif ASCENSION_SELECTS_GRAPHICS_SYSTEM(QT)
			QImage& asNative();
			const QImage& asNative() const;
#elif ASCENSION_SELECTS_GRAPHICS_SYSTEM(WIN32_GDI)
			win32::Handle<HBITMAP>::Type asNative() const BOOST_NOEXCEPT;
#endif

			static std::uint8_t depth(Format format);
			Format format() const;
			std::uint32_t numberOfBytes() const {return stride() * height();}
			boost::iterator_range<std::uint8_t*> pixels();
			boost::iterator_range<const std::uint8_t*> pixels() const;
			std::uint32_t stride() const;
			static std::uint32_t stride(std::uint32_t width, Format format);
			// RenderingDevice
			std::unique_ptr<RenderingContext2D> createRenderingContext() const;
			std::uint8_t depth() const;
			std::uint32_t numberOfColors() const;
			std::uint32_t height() const;
			std::uint16_t logicalDpiX() const;
			std::uint16_t logicalDpiY() const;
			std::uint32_t width() const;
			std::uint16_t physicalDpiX() const;
			std::uint16_t physicalDpiY() const;

		private:
			void initialize(std::unique_ptr<uint8_t[]> data, const geometry::BasicDimension<std::uint32_t>& size, Format format);
			void initialize(const std::uint8_t* data, const geometry::BasicDimension<std::uint32_t>& size, Format format);
		private:
#if ASCENSION_SELECTS_GRAPHICS_SYSTEM(CAIRO)
			Cairo::RefPtr<Cairo::ImageSurface> impl_;
			std::unique_ptr<std::uint8_t[]> buffer_;
#elif ASCENSION_SELECTS_GRAPHICS_SYSTEM(CORE_GRAPHICS)
			CGImageRef impl_;
#elif ASCENSION_SELECTS_GRAPHICS_SYSTEM(QT)
			QImage impl_;
			std::unique_ptr<std::uint8_t[]> buffer_;
#elif ASCENSION_SELECTS_GRAPHICS_SYSTEM(WIN32_GDI)
			win32::Handle<HBITMAP>::Type impl_;
			std::unique_ptr<std::uint8_t[], boost::null_deleter> buffer_;
#endif
		};
	}
}

#endif // !ASCENSION_IMAGE_HPP
