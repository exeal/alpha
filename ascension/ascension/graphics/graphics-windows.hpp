/**
 * @file graphics-windows.hpp
 * @author exeal
 * @date 2010-11-04 created
 */

#ifndef ASCENSION_GRAPHICS_WINDOWS_HPP
#define ASCENSION_GRAPHICS_WINDOWS_HPP

#include <ascension/graphics/color.hpp>
#include <ascension/graphics/rendering-context.hpp>
#include <ascension/graphics/rendering-device.hpp>
#include <ascension/corelib/basic-exceptions.hpp>
#include <ascension/win32/windows.hpp>	// win32.Handle
#include <functional>			// std.bind1st, std.ptr_fun

namespace ascension {
	namespace win32 {

		template<typename Base>
		class GraphicsContext : public Base {
		public:
			virtual ~GraphicsContext() /*throw()*/ {}
			std::shared_ptr<std::remove_pointer<HDC>::type>& nativeHandle() const {return dc_;}
			void fillRectangle(const graphics::NativeRectangle& rect, const graphics::Color& color) {
				const COLORREF oldBackground = ::GetBkColor(dc_.get());
				if(oldBackground != CLR_INVALID) {
					if(CLR_INVALID != ::SetBkColor(dc_.get(), color.asCOLORREF())) {
						const RECT rc = toNative(rect);
						if(0 != ::ExtTextOutW(dc_.get(), 0, 0, ETO_IGNORELANGUAGE | ETO_OPAQUE, &rc, L"", 0, nullptr)) {
							::SetBkColor(dc_.get(), oldBackground);
							return;
						}
					}
				}
				throw PlatformDependentError<>();
			}
			unsigned int logicalDpiX() const {return ::GetDeviceCaps(dc_.get(), LOGPIXELSX);}
			unsigned int logicalDpiY() const {return ::GetDeviceCaps(dc_.get(), LOGPIXELSY);}
			graphics::NativeSize size() const {
				return graphics::geometry::make<NativeSize>(
					::GetDeviceCaps(dc_.get(), HORZRES), ::GetDeviceCaps(dc_.get(), VERTRES));
			}
		protected:
			GraphicsContext() /*throw()*/ {}
			void initialize(std::shared_ptr<std::remove_pointer<HDC>::type>&& deviceContext) {
				assert(deviceContext.get() != nullptr);
				dc_ = deviceContext;
			}
		private:
			std::shared_ptr<std::remove_pointer<HDC>::type> dc_;
		};

		class ClientAreaGraphicsContext : public GraphicsContext<graphics::RenderingContext2D> {
		public:
			explicit ClientAreaGraphicsContext(std::shared_ptr<std::remove_pointer<HWND>::type> window) {
				if(window.get() == nullptr)
					throw NullPointerException("window");
				std::shared_ptr<std::remove_pointer<HDC>::type> dc(::GetDC(window.get()), std::bind(&::ReleaseDC, window.get(), std::placeholders::_1));
				if(dc.get() == nullptr)
					throw PlatformDependentError<>();
				initialize(std::move(dc));
			}
		};

		class EntireWindowGraphicsContext : public GraphicsContext<graphics::RenderingContext2D> {
		public:
			explicit EntireWindowGraphicsContext(std::shared_ptr<std::remove_pointer<HWND>::type> window) {
				if(window.get() == nullptr)
					throw NullPointerException("window");
				std::shared_ptr<std::remove_pointer<HDC>::type> dc(::GetWindowDC(window.get()), std::bind(&::ReleaseDC, window.get(), std::placeholders::_1));
				if(dc.get() == nullptr)
					throw PlatformDependentError<>();
				initialize(std::move(dc));
			}
		};

		class PaintContext : public GraphicsContext<graphics::PaintContext> {
		public:
			explicit PaintContext(std::shared_ptr<std::remove_pointer<HWND>::type> window) {
				if(window.get() == nullptr)
					throw NullPointerException("window");
				std::shared_ptr<std::remove_pointer<HDC>::type> dc(::BeginPaint(window.get(), &ps_), X(window.get(), ps_));
				if(dc.get() == nullptr)
					throw PlatformDependentError<>();
				initialize(std::move(dc));
			}
			graphics::NativeRectangle boundsToPaint() const {return ps_.rcPaint;}
		private:
			class X {
			public:
				X(HWND hwnd, PAINTSTRUCT& ps) : hwnd_(hwnd), ps_(ps) {}
				void operator()(HDC) {::EndPaint(hwnd_, &ps_);}
			private:
				HWND hwnd_;
				PAINTSTRUCT& ps_;
			};
			PAINTSTRUCT ps_;
		};

	}

	namespace detail {
		inline std::shared_ptr<std::remove_pointer<HDC>::type> screenDC() {
			return std::shared_ptr<std::remove_pointer<HDC>::type>(::GetDC(nullptr), std::bind(&::ReleaseDC, static_cast<HWND>(nullptr), std::placeholders::_1));
		}
	}
}

#endif // !ASCENSION_GRAPHICS_WINDOWS_HPP
