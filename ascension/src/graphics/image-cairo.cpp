/**
 * @file image-cairo.cpp
 * Implements @c graphics#Image class on cairomm.
 * @author exeal
 * @date 2013-10-27 Created.
 */

#include <ascension/graphics/image.hpp>
#ifdef ASCENSION_GRAPHICS_SYSTEM_CAIRO

#include <ascension/corelib/basic-exceptions.hpp>
#include <ascension/graphics/rendering-context.hpp>
#include <cairomm/context.h>

using namespace ascension;
using namespace ascension::graphics;
using namespace std;

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

Image::Image(const geometry::BasicDimension<uint16_t>& size, Image::Format format)
		: impl_(Cairo::ImageSurface::create(formatToNative(format), geometry::dx(size), geometry::dy(size))) {
}

Image::Image(unique_ptr<uint8_t[]> data, const geometry::BasicDimension<uint16_t>& size, Format format)
		: impl_(Cairo::ImageSurface::create(reinterpret_cast<unsigned char*>(data.get()), formatToNative(format), geometry::dx(size),
			geometry::dy(size), Cairo::ImageSurface::format_stride_for_width(formatToNative(format), geometry::dx(size)))) {
}

unique_ptr<RenderingContext2D> Image::createRenderingContext() const {
	return unique_ptr<RenderingContext2D>(new RenderingContext2D(Cairo::Context::create(impl_)));
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

uint16_t Image::height() const {
	return impl_->get_height();
}

uint16_t Image::logicalDpiX() const {
	return width();
}

uint16_t Image::logicalDpiY() const {
	return height();
}

uint32_t Image::numberOfColors() const {
	switch(format()) {
		case ARGB32:
			return numeric_limits<uint32_t>::max();
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

boost::iterator_range<uint8_t*> Image::pixels() {
	uint8_t* const p = impl_->get_data();
	return boost::make_iterator_range(p, p + Cairo::ImageSurface::format_stride_for_width(impl_->get_format(), impl_->get_width()));
}

boost::iterator_range<const uint8_t*> Image::pixels() const {
	const uint8_t* const p = impl_->get_data();
	return boost::make_iterator_range(p, p + Cairo::ImageSurface::format_stride_for_width(impl_->get_format(), impl_->get_width()));
}

uint16_t Image::physicalDpiX() const {
	return width();
}

uint16_t Image::physicalDpiY() const {
	return height();
}

uint16_t Image::width() const {
	return impl_->get_width();
}

#endif	// ASCENSION_GRAPHICS_SYSTEM_CAIRO
