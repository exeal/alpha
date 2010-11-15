/**
 * @file graphics-windows.hpp
 * @author exeal
 * @date 2010-11-04 created
 */

#ifndef ASCENSION_GRAPHICS_WINDOWS_HPP
#define ASCENSION_GRAPHICS_WINDOWS_HPP

#include <ascension/graphics/graphics.hpp>
#include <ascension/graphics/color.hpp>
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
			void fillRectangle(const graphics::Rect<>& rect, const graphics::Color& color) {
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
			uint logicalDpiX() const {return ::GetDeviceCaps(dc_.get(), LOGPIXELSX);}
			uint logicalDpiY() const {return ::GetDeviceCaps(dc_.get(), LOGPIXELSY);}
			graphics::Dimension<uint> size() const {
				return graphics::Dimension<uint>(::GetDeviceCaps(dc_.get(), HORZRES), ::GetDeviceCaps(dc_.get(), VERTRES));
			}
		protected:
			GraphicsContext() /*throw()*/ {}
			void initialize(Handle<HDC> deviceContext) {assert(deviceContext.get() != 0); dc_ = deviceContext;}
		private:
			Handle<HDC> dc_;
		};

		class ClientAreaGraphicsContext : public GraphicsContext<graphics::Context> {
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

		class EntireWindowGraphicsContext : public GraphicsContext<graphics::Context> {
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
			graphics::Rect<> boundsToPaint() const {return graphics::fromNative(ps_.rcPaint);}
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
}

#endif // !ASCENSION_GRAPHICS_WINDOWS_HPP
