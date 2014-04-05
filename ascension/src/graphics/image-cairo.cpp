/**
 * @file image-cairo.cpp
 * Implements @c graphics#Image class on cairomm.
 * @author exeal
 * @date 2013-10-27 Created.
 * @date 2014
 */

#include <ascension/graphics/image.hpp>
#if ASCENSION_SELECTS_GRAPHICS_SYSTEM(CAIRO)

#include <ascension/corelib/basic-exceptions.hpp>
#include <ascension/graphics/rendering-context.hpp>
#include <cairomm/context.h>


namespace ascension {
	namespace graphics {
		namespace {
			inline Cairo::Format formatToNative(Image::Format format) {
				switch(format) {
					case Image::ARGB32:
						return Cairo::FORMAT_ARGB32;
					case Image::RGB24:
						return Cairo::FORMAT_RGB24;
					case Image::RGB16:
						return Cairo::FORMAT_RGB16_565;
					case Image::A1:
						return Cairo::FORMAT_A1;
					default:
						throw UnknownValueException("format");
				}
			}
		}

		Image::Image(const geometry::BasicDimension<std::uint16_t>& size, Image::Format format)
				: impl_(Cairo::ImageSurface::create(formatToNative(format), geometry::dx(size), geometry::dy(size))) {
		}

		Image::Image(std::unique_ptr<std::uint8_t[]> data, const geometry::BasicDimension<std::uint16_t>& size, Format format)
				: impl_(Cairo::ImageSurface::create(reinterpret_cast<unsigned char*>(data.get()), formatToNative(format), geometry::dx(size),
					geometry::dy(size), Cairo::ImageSurface::format_stride_for_width(formatToNative(format), geometry::dx(size)))) {
		}

		std::unique_ptr<RenderingContext2D> Image::createRenderingContext() const {
			return std::unique_ptr<RenderingContext2D>(new RenderingContext2D(Cairo::Context::create(impl_)));
		}

		Image::Format Image::format() const {
			switch(impl_->get_format()) {
				case Cairo::FORMAT_ARGB32:
					return ARGB32;
				case Cairo::FORMAT_RGB24:
					return RGB24;
				case Cairo::FORMAT_RGB16_565:
					return RGB16;
				case Cairo::FORMAT_A1:
					return A1;
				default:
					throw UnknownValueException("impl_->get_format()");
			}
		}

		std::uint16_t Image::height() const {
			return impl_->get_height();
		}

		std::uint16_t Image::logicalDpiX() const {
			return width();
		}

		std::uint16_t Image::logicalDpiY() const {
			return height();
		}

		std::uint32_t Image::numberOfColors() const {
			switch(format()) {
				case ARGB32:
					return std::numeric_limits<std::uint32_t>::max();
				case RGB24:
					return 0x100 * 0x100 * 0x100;
				case Image::RGB16:
					return 0x100 * 0x100;
				case Image::A1:
					return 2;
				default:
					throw UnknownValueException("format()");
			}
		}

		boost::iterator_range<std::uint8_t*> Image::pixels() {
			std::uint8_t* const p = impl_->get_data();
			return boost::make_iterator_range(p, p + Cairo::ImageSurface::format_stride_for_width(impl_->get_format(), impl_->get_width()));
		}

		boost::iterator_range<const std::uint8_t*> Image::pixels() const {
			const std::uint8_t* const p = impl_->get_data();
			return boost::make_iterator_range(p, p + Cairo::ImageSurface::format_stride_for_width(impl_->get_format(), impl_->get_width()));
		}

		std::uint16_t Image::physicalDpiX() const {
			return width();
		}

		std::uint16_t Image::physicalDpiY() const {
			return height();
		}

		std::uint16_t Image::width() const {
			return impl_->get_width();
		}
	}
}
#endif	// ASCENSION_SELECTS_GRAPHICS_SYSTEM(CAIRO)
