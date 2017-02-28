/**
 * @file widget-windows.cpp
 * @author exeal
 * @date 2011-03-27 created
 */

#include <ascension/viewer/widgetapi/widget.hpp>

#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
#include <ascension/corelib/native-conversion.hpp>
#include <ascension/graphics/rendering-context.hpp>
#include <ascension/graphics/geometry/native-conversions.hpp>
#include <ascension/graphics/geometry/point-xy.hpp>
#include <ascension/graphics/geometry/rectangle-odxdy.hpp>
#include <ascension/graphics/geometry/rectangle-sides.hpp>
#include <ascension/win32/windows.hpp>
#include <ShellAPI.h>

namespace ascension {
	namespace viewer {
		namespace widgetapi {
			void acceptDrops(Proxy<Widget> widget, bool accept /* = true */) {
				::DragAcceptFiles(widget->handle().get(), accept);
			}

			bool acceptsDrops(Proxy<const Widget>) {
				return true;
			}

			graphics::Rectangle bounds(Proxy<const Widget> widget, bool includeFrame) {
				RECT temp;
				if(includeFrame) {
					if(!win32::boole(::GetWindowRect(widget->handle().get(), &temp)))
						throw makePlatformError();
				}
				else {
					if(!win32::boole(::GetClientRect(widget->handle().get(), &temp)))
						throw makePlatformError();
				}
				return fromNative<graphics::Rectangle>(temp);
			}

			void close(Proxy<Widget> widget) {
				::SendMessageW(widget->handle().get(), WM_CLOSE, 0, 0);	// ignore an error
			}

			std::unique_ptr<graphics::RenderingContext2D> createRenderingContext(Proxy<const Widget> widget) {
				auto dc(win32::makeHandle(::GetDC(widget->handle().get()), std::bind(&::ReleaseDC, widget->handle().get(), std::placeholders::_1)));
				return std::unique_ptr<graphics::RenderingContext2D>(new graphics::RenderingContext2D(std::move(dc)));
			}

			Proxy<const Window> cwindow(Proxy<const Widget> widget) {
				return Proxy<const Window>(*widget.get());
			}

			Proxy<Widget> desktop() {
				return win32::Window(win32::borrowed(::GetDesktopWindow()));
			}

			void forcePaint(Proxy<Widget> widget, const graphics::Rectangle& bounds) {
				const auto temp(toNative<RECT>(bounds));
				if(!win32::boole(::RedrawWindow(widget->handle().get(), &temp, nullptr, RDW_INTERNALPAINT | RDW_INVALIDATE)))
					throw makePlatformError();
			}

			ascension::detail::ScopeGuard grabInput(Proxy<Widget> widget) {
				::SetCapture(widget->handle().get());
				return ascension::detail::ScopeGuard(std::bind(&releaseInput, widget));
			}

			bool hasFocus(Proxy<const Widget> widget) {
				return ::GetFocus() == widget->handle().get();
			}

			void hide(Proxy<Widget> widget) {
				if(!win32::boole(::SetWindowPos(widget->handle().get(), nullptr, 0, 0, 0, 0,
					SWP_HIDEWINDOW | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOREPOSITION | SWP_NOSIZE | SWP_NOZORDER)))
					throw makePlatformError();
			}

			bool isActive(Proxy<const Widget> widget) {
				return ::GetActiveWindow() == widget->handle().get();
			}

			bool isMaximized(Proxy<const Window> widget) {
				return win32::boole(::IsZoomed(widget->handle().get()));
			}

			bool isMinimized(Proxy<const Window> widget) {
				return win32::boole(::IsIconic(widget->handle().get()));
			}

			bool isVisible(Proxy<const Widget> widget) {
				return win32::boole(::IsWindowVisible(widget->handle().get()));
			}

			void lower(Proxy<Widget> widget) {
				if(!win32::boole(::SetWindowPos(widget->handle().get(),
					HWND_BOTTOM, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE)))
					throw makePlatformError();
			}

			void move(Proxy<Window> widget, const graphics::Point& newOrigin) {
				if(!win32::boole(::SetWindowPos(widget->handle().get(), nullptr,
					static_cast<int>(graphics::geometry::x(newOrigin)), static_cast<int>(graphics::geometry::y(newOrigin)), 0, 0,
					SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER)))
					throw makePlatformError();
			}

			Widget::pointer parentWidget(Proxy<const Widget> widget) {
				static win32::Window parent((win32::Handle<HWND>()));
				auto temp(win32::borrowed(::GetParent(widget->handle().get())));
				if(temp.get() == nullptr)
					return nullptr;
				parent = win32::Window(temp);
				return &parent;
			}

			Window::pointer parentWindow(Proxy<const Widget> widget) {
				return parentWidget(widget);
			}

			void raise(Proxy<Window> widget) {
				if(!win32::boole(::SetWindowPos(widget->handle().get(),
					HWND_TOP, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE)))
					throw makePlatformError();
			}

			void redrawScheduledRegion(Proxy<Widget> widget) {
				if(!win32::boole(::UpdateWindow(widget->handle().get())))
					throw makePlatformError();
			}

			void releaseInput(Proxy<Widget>) {
				if(!win32::boole(::ReleaseCapture()))
					throw makePlatformError();
			}

			void resize(Proxy<Window> widget, const graphics::Dimension& newSize) {
				if(!win32::boole(::SetWindowPos(widget->handle().get(), nullptr,
					0, 0, static_cast<int>(graphics::geometry::dx(newSize)), static_cast<int>(graphics::geometry::dy(newSize)),
					SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER)))
					throw makePlatformError();
			}

			void scheduleRedraw(Proxy<Widget> widget, bool eraseBackground) {
				if(!win32::boole(::InvalidateRect(widget->handle().get(), nullptr, eraseBackground)))
					throw makePlatformError();
			}

			void scheduleRedraw(Proxy<Widget> widget, const graphics::Rectangle& rect, bool eraseBackground) {
				const auto temp(toNative<RECT>(rect));
				if(!win32::boole(::InvalidateRect(widget->handle().get(), &temp, eraseBackground)))
					throw makePlatformError();
			}

			void setAlwaysOnTop(Proxy<Widget> widget, bool set) {
				if(!win32::boole(::SetWindowPos(widget->handle().get(),
					set ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE)))
					throw makePlatformError();
			}

			void setBounds(Proxy<Widget> widget, const graphics::Rectangle& bounds) {
				if(!win32::boole(::SetWindowPos(widget->handle().get(), nullptr,
					static_cast<int>(graphics::geometry::left(bounds)), static_cast<int>(graphics::geometry::top(bounds)),
					static_cast<int>(graphics::geometry::dx(bounds)), static_cast<int>(graphics::geometry::dy(bounds)), SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER)))
					throw makePlatformError();
			}

			void setFocus() {
				if(::SetFocus(nullptr) == nullptr)
					throw makePlatformError();
			}

			void setFocus(Proxy<Widget> widget) {
				if(::SetFocus(widget->handle().get()) == nullptr)
					throw makePlatformError();
			}
#if 0
			void setShape(Proxy<Widget> widget, const NativeRegion& shape) {
				if(::SetWindowRgn(widget.handle().get(), shape.get(), true) == 0)
					throw makePlatformError();
			}

			void WidgetBase::scrollInformation(int bar, SCROLLINFO& scrollInfo, UINT mask /* = SIF_ALL */) const {
				scrollInfo.cbSize = sizeof(SCROLLINFO);
				scrollInfo.fMask = mask;
				if(!boole(::GetScrollInfo(handle().get(), bar, &scrollInfo)))
					throw PlatformDependentError<>();
			}

			int WidgetBase::scrollPosition(int bar) const {
				return ::GetScrollPos(handle().get(), bar);
			}

			Range<int> WidgetBase::scrollRange(int bar) const {
				int minPos, maxPos;
				if(!boole(::GetScrollRange(handle().get(), bar, &minPos, &maxPos)))
					throw PlatformDependentError<>();
				return makeRange(minPos, maxPos);
			}

			int WidgetBase::scrollTrackPosition(int bar) const {
				SCROLLINFO si;
				scrollInformation(bar, si, SIF_TRACKPOS);
				return si.nTrackPos;
			}

			void WidgetBase::setScrollInformation(int bar, const SCROLLINFO& scrollInfo, bool redraw /* = true */) {
				if(!boole(::SetScrollInfo(handle().get(), bar, &scrollInfo, redraw)))
					throw PlatformDependentError<>();
			}

			int WidgetBase::setScrollPosition(int bar, int pos, bool redraw /* = true */) {
				return ::SetScrollPos(handle().get(), bar, pos, redraw);
			}

			void WidgetBase::setScrollRange(int bar, const Range<int>& range, bool redraw /* = true */) {
				::SetScrollRange(handle().get(), bar, range.beginning(), range.end(), redraw);
			}
#endif
			void setParentWidget(Proxy<Widget> widget, Proxy<Widget> newParent) {
				if(::SetParent(widget->handle().get(), (newParent.get() != nullptr) ? newParent->handle().get() : nullptr) == nullptr)
					throw makePlatformError();
			}

			void setParentWindow(Proxy<Widget> widget, Proxy<Window> newParent) {
				if(::SetParent(widget->handle().get(), (newParent.get() != nullptr) ? newParent->handle().get() : nullptr) == nullptr)
					throw makePlatformError();
			}

			void setWindowOpacity(Proxy<Widget> widget, double opacity) {
				const LONG_PTR style = win32::getWindowLong(widget->handle().get(), GWL_EXSTYLE);
				if((style & WS_EX_LAYERED) == 0)
					win32::setWindowLong(widget->handle().get(), GWL_EXSTYLE, style | WS_EX_LAYERED);
				COLORREF mask;
				DWORD flags;
				if(!win32::boole(::GetLayeredWindowAttributes(widget->handle().get(), &mask, nullptr, &flags)))
					throw makePlatformError();
				if(!win32::boole(::SetLayeredWindowAttributes(widget->handle().get(), mask, static_cast<BYTE>(opacity * 255), flags | LWA_ALPHA)))
					throw makePlatformError();
			}

			void show(Proxy<Widget> widget) {
				if(!win32::boole(::ShowWindow(widget->handle().get(), SW_SHOWNOACTIVATE)))
					throw makePlatformError();
			}

			void showMaximized(Proxy<Widget> widget) {
				::ShowWindow(widget->handle().get(), SW_MAXIMIZE);
			}

			void showMinimized(Proxy<Widget> widget) {
				::ShowWindow(widget->handle().get(), SW_MINIMIZE);
			}

			void showNormal(Proxy<Widget> widget) {
				::ShowWindow(widget->handle().get(), SW_RESTORE);
			}

			void unsetFocus(Proxy<Widget> widget) {
				if(hasFocus(widget))
					::SetFocus(nullptr);
			}

			Proxy<Window> window(Proxy<Widget> widget) {
				return Proxy<Window>(*widget.get());
			}

			double windowOpacity(Proxy<const Widget> widget) {
				BYTE alpha;
				DWORD flags;
				if(!win32::boole(::GetLayeredWindowAttributes(widget->handle().get(), nullptr, &alpha, &flags)))
					throw makePlatformError();
				return ((flags & LWA_ALPHA) != 0) ? alpha / 255.0 : 1.0;
			}
		}
	}

	namespace graphics {
		boost::optional<Color> SystemColors::get(Value value) {
			switch(value) {
				case ACTIVE_BORDER:
					return fromNative<Color>(::GetSysColor(COLOR_ACTIVEBORDER));
				case ACTIVE_CAPTION:
					return fromNative<Color>(::GetSysColor(COLOR_ACTIVECAPTION));
				case APP_WORKSPACE:
					return fromNative<Color>(::GetSysColor(COLOR_APPWORKSPACE));
				case BACKGROUND:
					return fromNative<Color>(::GetSysColor(COLOR_BACKGROUND));
				case BUTTON_FACE:
					return fromNative<Color>(::GetSysColor(COLOR_BTNFACE));
				case BUTTON_HIGHLIGHT:
					return fromNative<Color>(::GetSysColor(COLOR_BTNHIGHLIGHT));
				case BUTTON_SHADOW:
					return fromNative<Color>(::GetSysColor(COLOR_BTNSHADOW));
				case BUTTON_TEXT:
					return fromNative<Color>(::GetSysColor(COLOR_BTNTEXT));
				case CAPTION_TEXT:
					return fromNative<Color>(::GetSysColor(COLOR_CAPTIONTEXT));
				case GRAY_TEXT:
					return fromNative<Color>(::GetSysColor(COLOR_GRAYTEXT));
				case HIGHLIGHT:
					return fromNative<Color>(::GetSysColor(COLOR_HIGHLIGHT));
				case HIGHLIGHT_TEXT:
					return fromNative<Color>(::GetSysColor(COLOR_HIGHLIGHTTEXT));
				case INACTIVE_BORDER:
					return fromNative<Color>(::GetSysColor(COLOR_INACTIVEBORDER));
				case INACTIVE_CAPTION:
					return fromNative<Color>(::GetSysColor(COLOR_INACTIVECAPTION));
				case INACTIVE_CAPTION_TEXT:
					return fromNative<Color>(::GetSysColor(COLOR_INACTIVECAPTIONTEXT));
				case INFO_BACKGROUND:
					return fromNative<Color>(::GetSysColor(COLOR_INFOBK));
				case INFO_TEXT:
					return fromNative<Color>(::GetSysColor(COLOR_INFOTEXT));
				case MENU:
					return fromNative<Color>(::GetSysColor(COLOR_MENU));
				case MENU_TEXT:
					return fromNative<Color>(::GetSysColor(COLOR_MENUTEXT));
				case SCROLLBAR:
					return fromNative<Color>(::GetSysColor(COLOR_SCROLLBAR));
				case THREE_D_DARK_SHADOW:
					return fromNative<Color>(::GetSysColor(COLOR_3DDKSHADOW));
				case THREE_D_FACE:
					return fromNative<Color>(::GetSysColor(COLOR_3DFACE));
				case THREE_D_HIGHLIGHT:
					return fromNative<Color>(::GetSysColor(COLOR_3DHIGHLIGHT));
				case THREE_D_LIGHT_SHADOW:
					return fromNative<Color>(::GetSysColor(COLOR_3DLIGHT));
				case THREE_D_SHADOW:
					return fromNative<Color>(::GetSysColor(COLOR_3DSHADOW));
				case WINDOW:
					return fromNative<Color>(::GetSysColor(COLOR_WINDOW));
				case WINDOW_FRAME:
					return fromNative<Color>(::GetSysColor(COLOR_WINDOWFRAME));
				case WINDOW_TEXT:
					return fromNative<Color>(::GetSysColor(COLOR_WINDOWTEXT));
				default:
					throw UnknownValueException("value");
			}
		}
	}
}

#endif // ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
