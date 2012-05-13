/**
 * @file image-windows.cpp
 * @author exeal
 * @date 2011-10-01
 */

#include <ascension/corelib/basic-exceptions.hpp>	// PlatformDependentError
#include <ascension/graphics/image.hpp>
#include <ascension/win32/windows.hpp>

using namespace ascension;
using namespace ascension::graphics;
using namespace std;


namespace {
	win32::Handle<HBITMAP> createBitmap(const NativeSize& size, Image::Format format, const uint8_t* data) {
		size_t bytesPerPixel;
		switch(format) {
		case Image::ARGB_32:
			bytesPerPixel = 4;
			break;
		case Image::RGB_24:
			bytesPerPixel = 3;
			break;
		case Image::RGB_16:
			bytesPerPixel = 2;
			break;
		default:
			throw UnknownValueException("format");
		}
		win32::Handle<HDC> dc(::GetDC(nullptr));
		if(dc.get() == nullptr)
			throw makePlatformError();
		BITMAPINFO* const info = static_cast<BITMAPINFO*>(::operator new(
			sizeof(BITMAPINFOHEADER) + bytesPerPixel * geometry::dx(size) * geometry::dy(size)));
		BITMAPINFOHEADER& header = info->bmiHeader;
		memset(&header, 0, sizeof(BITMAPINFOHEADER));
		header.biSize = sizeof(BITMAPINFOHEADER);
		header.biWidth = geometry::dx(size);
		header.biHeight = -geometry::dy(size);
		header.biBitCount = sizeof(RGBQUAD) * 8;//::GetDeviceCaps(dc.get(), BITSPIXEL);
		header.biPlanes = static_cast<WORD>(::GetDeviceCaps(dc.get(), PLANES));
		header.biCompression = BI_RGB;
		memcpy(info->bmiColors, data, bytesPerPixel * geometry::dx(size) * geometry::dy(size));
		win32::Handle<HBITMAP> bitmap(::CreateDIBitmap(dc.get(), &header, CBM_INIT, info->bmiColors, info, DIB_RGB_COLORS));
		::operator delete(info);
		return bitmap;
	}
}

Image::Image(const NativeSize& size, Format format) {
}

