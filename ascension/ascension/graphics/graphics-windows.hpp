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
			const Handle<HDC>& nativeHandle() const {return dc_;}
			void fillRectangle(const graphics::NativeRectangle& rect, const graphics::Color& color) {
				const COLORREF oldBackground = ::GetBkColor(dc_.get());
				if(oldBackground != CLR_INVALID) {
					if(CLR_INVALID != ::SetBkColor(dc_.get(), color.asCOLORREF())) {
						const RECT rc = toNative(rect);
						if(0 != ::ExtTextOutW(dc_.get(), 0, 0, ETO_IGNORELANGUAGE | ETO_OPAQUE, &rc, L"", 0, 0)) {
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
			void initialize(Handle<HDC> deviceContext) {assert(deviceContext.get() != 0); dc_ = deviceContext;}
		private:
			Handle<HDC> dc_;
		};

		class ClientAreaGraphicsContext : public GraphicsContext<graphics::RenderingContext2D> {
		public:
			explicit ClientAreaGraphicsContext(const Handle<HWND>& window) {
				if(window.get() == 0)
					throw NullPointerException("window");
				Handle<HDC> dc(::GetDC(window.get()), std::bind1st(std::ptr_fun(&::ReleaseDC), window.get()));
				if(dc.get() == 0)
					throw PlatformDependentError<>();
				initialize(dc);
			}
		};

		class EntireWindowGraphicsContext : public GraphicsContext<graphics::RenderingContext2D> {
		public:
			explicit EntireWindowGraphicsContext(const Handle<HWND>& window) {
				if(window.get() == 0)
					throw NullPointerException("window");
				Handle<HDC> dc(::GetWindowDC(window.get()), std::bind1st(std::ptr_fun(&::ReleaseDC), window.get()));
				if(dc.get() == 0)
					throw PlatformDependentError<>();
				initialize(dc);
			}
		};

		class PaintContext : public GraphicsContext<graphics::PaintContext> {
		public:
			explicit PaintContext(const Handle<HWND>& window) {
				if(window.get() == 0)
					throw NullPointerException("window");
				Handle<HDC> dc(::BeginPaint(window.get(), &ps_), X(window.get(), ps_));
				if(dc.get() == 0)
					throw PlatformDependentError<>();
				initialize(dc);
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
		inline win32::Handle<HDC> screenDC() {
			return win32::Handle<HDC>(::GetDC(0), std::bind1st(std::ptr_fun(&::ReleaseDC), static_cast<HWND>(0)));
		}
	}
}

#endif // !ASCENSION_GRAPHICS_WINDOWS_HPP
