/**
 * @file widget-windows.cpp
 * @author exeal
 * @date 2011-03-27 created
 */

#include <ascension/viewer/widgetapi/widget.hpp>

#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
#include <ascension/graphics/rendering-context.hpp>
#include <ascension/win32/windows.hpp>
#include <ShellAPI.h>

using namespace ascension;
using namespace ascension::graphics;
using namespace ascension::viewer;
using namespace std;


void widgetapi::acceptDrops(NativeWidget& widget, bool accept /* = true */) {
	::DragAcceptFiles(widget.handle().get(), accept);
}

bool widgetapi::acceptsDrops(const NativeWidget&) {
	return true;
}

graphics::Rectangle widgetapi::bounds(const NativeWidget& widget, bool includeFrame) {
	RECT temp;
	if(includeFrame) {
		if(!win32::boole(::GetWindowRect(widget.handle().get(), &temp)))
			throw makePlatformError();
	} else {
		if(!win32::boole(::GetClientRect(widget.handle().get(), &temp)))
			throw makePlatformError();
	}
	return graphics::Rectangle(temp);
}

void widgetapi::close(NativeWidget& widget) {
	::SendMessageW(widget.handle().get(), WM_CLOSE, 0, 0);	// ignore an error
}

unique_ptr<RenderingContext2D> widgetapi::createRenderingContext(const NativeWidget& widget) {
	win32::Handle<HDC>::Type dc(::GetDC(widget.handle().get()), bind(&::ReleaseDC, widget.handle().get(), placeholders::_1));
	return unique_ptr<RenderingContext2D>(new RenderingContext2D(std::move(dc)));
}

widgetapi::NativeWindow widgetapi::desktop() {
	return win32::Window(win32::Handle<HWND>::Type(::GetDesktopWindow()));
}

void widgetapi::forcePaint(NativeWidget& widget, const graphics::Rectangle& bounds) {
	if(!win32::boole(::RedrawWindow(widget.handle().get(), &static_cast<const RECT>(bounds), nullptr, RDW_INTERNALPAINT | RDW_INVALIDATE)))
		throw makePlatformError();
}

void widgetapi::grabInput(NativeWidget& widget) {
	::SetCapture(widget.handle().get());
}

bool widgetapi::hasFocus(const NativeWidget& widget) {
	return ::GetFocus() == widget.handle().get();
}

void widgetapi::hide(NativeWidget& widget) {
	if(!win32::boole(::SetWindowPos(widget.handle().get(), nullptr, 0, 0, 0, 0,
			SWP_HIDEWINDOW | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOREPOSITION | SWP_NOSIZE | SWP_NOZORDER)))
		throw makePlatformError();
}

bool widgetapi::isActive(const NativeWidget& widget) {
	return ::GetActiveWindow() == widget.handle().get();
}

bool widgetapi::isMaximized(const NativeWidget& widget) {
	return win32::boole(::IsZoomed(widget.handle().get()));
}

bool widgetapi::isMinimized(const NativeWidget& widget) {
	return win32::boole(::IsIconic(widget.handle().get()));
}

bool widgetapi::isVisible(const NativeWidget& widget) {
	return win32::boole(::IsWindowVisible(widget.handle().get()));
}

void widgetapi::lower(NativeWidget& widget) {
	if(!win32::boole(::SetWindowPos(widget.handle().get(),
			HWND_BOTTOM, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE)))
		throw makePlatformError();
}

void widgetapi::move(NativeWidget& widget, const Point& newOrigin) {
	if(!win32::boole(::SetWindowPos(widget.handle().get(), nullptr,
			static_cast<int>(geometry::x(newOrigin)), static_cast<int>(geometry::y(newOrigin)), 0, 0,
			SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER)))
		throw makePlatformError();
}
/*
boost::optional<widgetapi::NativeWidget> widgetapi::parent(const NativeWidget& widget) {
	HWND parentHandle = ::GetParent(widget.handle().get());
	if(parentHandle == nullptr)
		return boost::none;
	win32::Window temp((win32::Handle<HWND>(parentHandle)));
	return boost::make_optional(temp);
}
*/
void widgetapi::raise(NativeWidget& widget) {
	if(!win32::boole(::SetWindowPos(widget.handle().get(),
			HWND_TOP, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE)))
		throw makePlatformError();
}

void widgetapi::redrawScheduledRegion(NativeWidget& widget) {
	if(!win32::boole(::UpdateWindow(widget.handle().get())))
		throw makePlatformError();
}

void widgetapi::releaseInput(NativeWidget&) {
	if(!win32::boole(::ReleaseCapture()))
		throw makePlatformError();
}

void widgetapi::resize(NativeWidget& widget, const Dimension& newSize) {
	if(!win32::boole(::SetWindowPos(widget.handle().get(), nullptr,
			0, 0, static_cast<int>(geometry::dx(newSize)), static_cast<int>(geometry::dy(newSize)),
			SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER)))
		throw makePlatformError();
}

void widgetapi::scheduleRedraw(NativeWidget& widget, bool eraseBackground) {
	if(!win32::boole(::InvalidateRect(widget.handle().get(), nullptr, eraseBackground)))
		throw makePlatformError();
}

void widgetapi::scheduleRedraw(NativeWidget& widget, const graphics::Rectangle& rect, bool eraseBackground) {
	RECT temp(rect);
	if(!win32::boole(::InvalidateRect(widget.handle().get(), &temp, eraseBackground)))
		throw makePlatformError();
}

void widgetapi::setAlwaysOnTop(NativeWidget& widget, bool set) {
	if(!win32::boole(::SetWindowPos(widget.handle().get(),
			set ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE)))
		throw makePlatformError();
}

void widgetapi::setBounds(NativeWidget& widget, const graphics::Rectangle& bounds) {
	if(!win32::boole(::SetWindowPos(widget.handle().get(), nullptr,
			static_cast<int>(geometry::left(bounds)), static_cast<int>(geometry::top(bounds)),
			static_cast<int>(geometry::dx(bounds)), static_cast<int>(geometry::dy(bounds)), SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER)))
		throw makePlatformError();
}

void widgetapi::setFocus() {
	if(::SetFocus(nullptr) == nullptr)
		throw makePlatformError();
}

void widgetapi::setFocus(NativeWidget& widget) {
	if(::SetFocus(widget.handle().get()) == nullptr)
		throw makePlatformError();
}
#if 0
void widgetapi::setShape(NativeWidget& widget, const NativeRegion& shape) {
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
void widgetapi::setParent(NativeWidget& widget, NativeWidget* newParent) {
	if(::SetParent(widget.handle().get(), (newParent != 0) ? newParent->handle().get() : nullptr) == nullptr)
		throw makePlatformError();
}

void widgetapi::setWindowOpacity(NativeWidget& widget, double opacity) {
	const LONG_PTR style = win32::getWindowLong(widget.handle().get(), GWL_EXSTYLE);
	if((style & WS_EX_LAYERED) == 0)
		win32::setWindowLong(widget.handle().get(), GWL_EXSTYLE, style | WS_EX_LAYERED);
	COLORREF mask;
	DWORD flags;
	if(!win32::boole(::GetLayeredWindowAttributes(widget.handle().get(), &mask, nullptr, &flags)))
		throw makePlatformError();
	if(!win32::boole(::SetLayeredWindowAttributes(widget.handle().get(), mask, static_cast<BYTE>(opacity * 255), flags | LWA_ALPHA)))
		throw makePlatformError();
}

void widgetapi::show(NativeWidget& widget) {
	if(!win32::boole(::ShowWindow(widget.handle().get(), SW_SHOWNOACTIVATE)))
		throw makePlatformError();
}

void widgetapi::showMaximized(NativeWidget& widget) {
	::ShowWindow(widget.handle().get(), SW_MAXIMIZE);
}

void widgetapi::showMinimized(NativeWidget& widget) {
	::ShowWindow(widget.handle().get(), SW_MINIMIZE);
}

void widgetapi::showNormal(NativeWidget& widget) {
	::ShowWindow(widget.handle().get(), SW_RESTORE);
}

double widgetapi::windowOpacity(const NativeWidget& widget) {
	BYTE alpha;
	DWORD flags;
	if(!win32::boole(::GetLayeredWindowAttributes(widget.handle().get(), nullptr, &alpha, &flags)))
		throw makePlatformError();
	return ((flags & LWA_ALPHA) != 0) ? alpha / 255.0 : 1.0;
}

#endif // ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
