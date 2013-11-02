/**
 * @file image.cpp
 * Implements @c graphics#Image class.
 * @author exeal
 * @date 2013-11-02 Created.
 */

#include <ascension/graphics/image.hpp>

using namespace ascension;
using namespace ascension::graphics;
using namespace std;


/// @see RenderingDevice#depth
uint16_t Image::depth() const {
	return depth(format());
}

/**
 * Returns the depth (the number of bits used to store a single pixel (bpp)) of the given image format.
 * @param format The image format
 * @return The depth
 * @throw UnknownValueException @a format is unknown
 */
int Image::depth(Image::Format format) {
	switch(format) {
		case ARGB32:
			return 32;
		case RGB24:
			return 24;
		case RGB16:
			return 16;
		case A1:
			return 1;
		default:
			throw UnknownValueException("format");
	}
}

/// @see RenderingDevice#heightInMillimeters
Scalar Image::heightInMillimeters() const {
	return height() * 25.4f / physicalDpiY();
}

/**
 * @fn ascension::graphics::Image::Format ascension::graphics::Image::format() const
 * Returns the format of the image.
 */

/// @see RenderingDevice#logicalDpiX
uint16_t Image::logicalDpiX() const {
	return defaultDpiX();
}

/// @see RenderingDevice#logicalDpiY
uint16_t Image::logicalDpiY() const {
	return defaultDpiY();
}

/**
 * @fn std::uint32_t ascension::graphics::Image::numberOfBytes() const
 * Returns the number of bytes occupied by the image data.
 */

/// @see RenderingDevice#numberOfColors
uint32_t Image::numberOfColors() const {
	switch(format()) {
		case ARGB32:
			return numeric_limits<uint32_t>::max();
		case RGB24:
			return 256 * 256 * 256;
		case RGB16:
			return 256 * 256;
		case A1:
			return 2;
		default:
			throw UnknownValueException("format");
	}
}

/**
 * @fn std::uint16_t ascension::graphics::Image::numberOfBytesPerLine() const
 * Returns the number of bytes per the image scanline.
 */

/// @see RenderingDevice#physicalDpiX
uint16_t Image::physicalDpiX() const {
	return defaultDpiX();
}

/// @see RenderingDevice#physicalDpiY
uint16_t Image::physicalDpiY() const {
	return defaultDpiY();
}

/**
 * @fn boost::iterator_range<std::uint8_t*> ascension::graphics::Image::pixels()
 * Returns a pointer to the pixel data of the image, for direct inspection or modification.
 */

/**
 * @fn boost::iterator_range<const std::uint8_t*> ascension::graphics::Image::pixels() const
 * Returns a pointer to the pixel data of the image, for direct inspection.
 */
