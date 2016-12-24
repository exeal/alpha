/**
 * @file image-windows.cpp
 * Implements @c graphics#Image class on Win32 platform.
 * @author exeal
 * @date 2011-10-01 created
 * @date 2012-2014
 */

#include <ascension/graphics/image.hpp>
#if ASCENSION_SELECTS_GRAPHICS_SYSTEM(WIN32_GDI)

#include <ascension/corelib/basic-exceptions.hpp>	// PlatformDependentError
#include <ascension/graphics/rendering-context.hpp>
#include <ascension/win32/windows.hpp>


namespace ascension {
	namespace graphics {
		/// Returns the platform-native underlying @c HBITMAP handle.
		win32::Handle<HBITMAP> Image::asNative() const BOOST_NOEXCEPT {
			return impl_;
		}

		namespace {
			template<typename T>
			inline T&& win32Object(const win32::Handle<HBITMAP>& deviceContext) {
				T temp;
				if(::GetObjectW(deviceContext.get(), sizeof(T), &temp) == 0)
					throw makePlatformError();
				return std::move(temp);
			}
		}

		std::unique_ptr<RenderingContext2D> Image::createRenderingContext() const {
			HDC dc = ::CreateCompatibleDC(nullptr);
			HBITMAP oldBitmap = static_cast<HBITMAP>(::SelectObject(dc, impl_.get()));
			if(oldBitmap == nullptr || oldBitmap == HGDI_ERROR) {
				::DeleteDC(dc);
				throw makePlatformError();
			}
			return std::unique_ptr<RenderingContext2D>(new RenderingContext2D(win32::Handle<HDC>(dc, [&oldBitmap](HDC p) {
				::SelectObject(p, oldBitmap);
				::DeleteDC(p);
			})));
		}

		Image::Format Image::format() const {
			const DIBSECTION section(win32Object<DIBSECTION>(impl_));
			switch(section.dsBmih.biBitCount) {
				case 1:
					return A1;
				case 16:
					if(section.dsBmih.biCompression == BI_RGB)
						return RGB16;
					break;
				case 24:
					if(section.dsBmih.biCompression == BI_BITFIELDS)
						return RGB24;
					break;
				case 32:
					if(section.dsBmih.biCompression == BI_BITFIELDS)
						return ARGB32;
					break;
			}
			throw UnknownValueException("The underlying image format is unknown.");
		}

		std::uint32_t Image::height() const {
			return /*static_cast<std::uint32_t>*/(win32Object<BITMAP>(impl_).bmHeight);
		}

		void Image::initialize(const std::uint8_t* data, const geometry::BasicDimension<std::uint32_t>& size, Format format) {
			win32::AutoZeroSize<BITMAPV5HEADER> header;
			static_assert(sizeof(decltype(header)) >= sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * 3, "");
			switch(format) {
				case Image::ARGB32:
					header.bV5BitCount = 32;
					header.bV5Compression = BI_BITFIELDS;
					header.bV5RedMask = 0x00ff0000u;
					header.bV5GreenMask = 0x0000ff00u;
					header.bV5BlueMask = 0x000000ffu;
					header.bV5AlphaMask = 0xff000000u;
					break;
				case Image::RGB24:
					header.bV5BitCount = 24;
					header.bV5Compression = BI_BITFIELDS;
					header.bV5RedMask = 0x0000f800u;
					header.bV5GreenMask = 0x000007e0u;
					header.bV5BlueMask = 0x0000001fu;
					break;
				case Image::RGB16:
					header.bV5BitCount = 16;
					header.bV5Compression = BI_RGB;
					break;
				case Image::A1:
					header.bV5BitCount = 1;
					memset(reinterpret_cast<BITMAPINFO*>(&header)->bmiColors + 0, 0xff, sizeof(RGBQUAD));
					memset(reinterpret_cast<BITMAPINFO*>(&header)->bmiColors + 1, 0x00, sizeof(RGBQUAD));
				default:
					throw UnknownValueException("format");
			}
			header.bV5Width = geometry::dx(size);
			header.bV5Height = -static_cast<LONG>(geometry::dy(size));
			header.bV5Planes = 1;
//			const auto stride = ((header.bV5Width * header.bV5BitCount + 31) >> 5 ) * 4;

			void* pixels;
			auto bitmap(win32::makeHandle(::CreateDIBSection(nullptr, reinterpret_cast<const BITMAPINFO*>(&header), DIB_RGB_COLORS, &pixels, nullptr, 0), &::DeleteObject));
			if(bitmap.get() == nullptr)
				throw makePlatformError();

			if(data != nullptr) {
				const BITMAP temp(win32Object<BITMAP>(bitmap));
				std::memcpy(buffer_.get(), data, temp.bmWidthBytes * temp.bmHeight);
			}

			// commit
			swap(impl_, bitmap);
			buffer_.reset(static_cast<std::uint8_t*>(pixels));
		}

		void Image::initialize(std::unique_ptr<std::uint8_t[]> data, const geometry::BasicDimension<std::uint32_t>& size, Format format) {
			return initialize(data.get(), size, format);
		}

		boost::iterator_range<std::uint8_t*> Image::pixels() {
			return boost::make_iterator_range(buffer_.get(), buffer_.get() + numberOfBytes());
		}

		boost::iterator_range<const std::uint8_t*> Image::pixels() const {
			return boost::make_iterator_range(buffer_.get(), buffer_.get() + numberOfBytes());
		}

//		std::uint32_t Image::stride() const {
//			return win32Object<BITMAP>(impl_).bmWidthBytes;
//		}

		std::uint32_t Image::stride(std::uint32_t width, Format format) {
			return ((width * depth(format) + 31) >> 5) * 4;
		}

		std::uint32_t Image::width() const {
			return static_cast<std::uint32_t>(win32Object<BITMAP>(impl_).bmWidth);
		}
	}
}
#endif	// ASCENSION_SELECTS_GRAPHICS_SYSTEM(WIN32_GDI)
